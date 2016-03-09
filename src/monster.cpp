#include <cstring>

#include "level.h"
#include "main_loop.h"
#include "mx_assert.h"
#include "mx_math.h"
#include "player.h"

#include "monster.h"

static const float g_axises[3][3][3]=
{
	{ {0.0f,-1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
	{ {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
	{ {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} },
};

static const float g_max_attack_moving_time_s= 1.0f;

static const float g_speed_in_patrol= 0.8f;
static const float g_speed_in_attack= 2.0f;

static const float g_rotation_speed_in_patrol= 1.0f;

static const float g_angle_sin_eps= 0.001f;
static const float g_almost_one= 0.9999f;

mx_Monster::mx_Monster(
	MonsterType type,
	const mx_LevelSector& home_sector,
	mx_Level& level,
	const mx_Player& player,
	const float* pos,
	const mx_PatrolPath* patrol_path )
	: mx_Pawn( mx_GameConstants::initial_monsters_health[type] )
	, type_(type)
	, home_sector_(home_sector)
	, level_(level)
	, player_(player)
	, patrol_path_( patrol_path ? *patrol_path : mx_PatrolPath() )
	, have_patrol_path_( patrol_path != NULL )
	, warned_(false)
	, destroy_(false)
{
	if( have_patrol_path_ )
	{
		VEC3_CPY( pos_, patrol_path_.points[0] );

		std::memcpy( axis_, g_axises[ patrol_path_.path_plane ], sizeof(float) * 3 * 3 );
	}
	else
	{
		VEC3_CPY( pos_, pos );
	}
}

mx_Monster::~mx_Monster()
{
	// TODO - remove all deinitialisation
	// for 32kb game it is too expensive
	destroy_= true;
	Exec();
}

void mx_Monster::ExecFunc()
{
	unsigned int target_path_point= 0;
	const mx_MainLoop& main_loop= *mx_MainLoop::Instance();

	move_to_target:
	while( !destroy_ )
	{
		if( have_patrol_path_ )
		{
			Attack();

			float vec_to_target[3];
			mxVec3Sub( patrol_path_.points[ target_path_point ], pos_, vec_to_target );
			float vec_to_target_square_length= mxVec3SquareLen( vec_to_target );

			float tick_step= g_speed_in_patrol * main_loop.GetTickTime();
			if( tick_step * tick_step >= vec_to_target_square_length )
			{
				target_path_point= target_path_point ^ 1;
				goto rotate_to_target;
			}
			else
			{
				float d_pos[3];
				mxVec3Mul( vec_to_target, tick_step / std::sqrtf(vec_to_target_square_length), d_pos );
				mxVec3Add( pos_, d_pos );
			}

			Resume();
		}
	}

rotate_to_target:
	// TODO - this code of rotation is very strange.
	while( !destroy_ )
	{
		Attack();

		bool rotation_finished= false;

		float vec_to_target[3];
		mxVec3Sub( patrol_path_.points[ target_path_point ], pos_, vec_to_target );
		mxVec3Normalize( vec_to_target );

		float rotation_axis[3];
		mxVec3Cross( axis_[1], vec_to_target, rotation_axis );
		float dot= mxVec3Dot( vec_to_target, axis_[1] );

		if( mxVec3SquareLen( rotation_axis ) < g_angle_sin_eps )
		{
			VEC3_CPY( rotation_axis, axis_[2] );
		}
		{
			float angle= mxAcosClamped( dot );
			float d_angle= g_rotation_speed_in_patrol * main_loop.GetTickTime();
			if( d_angle >= angle )
			{
				d_angle= angle;
				rotation_finished= true;
			}

			float rot_mat[16];
			mxMat4RotateAroundVector( rot_mat, rotation_axis, d_angle );
			for( unsigned int i= 0; i < 3; i++ )
				mxVec3Mat4Mul( axis_[i], rot_mat );
			CorrectAxis();
		}

		if( rotation_finished )
			goto move_to_target;

		Resume();
	}
}

bool mx_Monster::NeedStopAttack()
{
	return player_.GetSector() != &home_sector_ || player_.GetHealth() <= 0;
}

bool mx_Monster::CanAttack()
{
	if( player_.GetSector() == &home_sector_ && player_.GetHealth() > 0 )
	{
		if( warned_ )
		{
			warned_= false;
			return true;
		}

		float vec_to_player[3];
		mxVec3Sub( player_.Pos(), Pos(), vec_to_player );
		mxVec3Normalize( vec_to_player );

		float dot= mxVec3Dot( vec_to_player, axis_[1] );

		static const float view_cone_half_angle= 30.0f * MX_DEG2RAD;
		if( dot >= std::cosf(view_cone_half_angle) )
			return true;
	}
	return false;
}

void mx_Monster::Attack()
{
	if( ! CanAttack() )
		return;

	const mx_MainLoop& main_loop= *mx_MainLoop::Instance();

	select_new_target_point:
	float target_pos[3];
	if( NeedStopAttack() ) return;
	if( !SelectTargetPosition( target_pos ) ) goto wait_a_bit;

	while( !destroy_ )
	{
		float vec_to_target[3];
		mxVec3Sub( target_pos, pos_, vec_to_target );
		float vec_to_target_length= mxVec3Len( vec_to_target );

		float tick_step= g_speed_in_attack * main_loop.GetTickTime();
		if( tick_step >= vec_to_target_length )
		{
			VEC3_CPY( pos_, target_pos );
			goto attack_target;
		}
		else
		{
			// Rotatate to player
			float vec_to_player[3];
			float rotation_axis[3];
			mxVec3Sub( player_.Pos(), target_pos, vec_to_player );
			mxVec3Normalize( vec_to_player );
			mxVec3Cross( axis_[1], vec_to_player, rotation_axis );
			float dot= mxVec3Dot( vec_to_player, axis_[1] );

			if( mxVec3SquareLen( rotation_axis ) < g_angle_sin_eps )
			{
				for( unsigned int i= 0; i < 3; i++ )
					rotation_axis[i]= rand_.RandF( -0.1f, 0.1f );
			}
			float time_left= vec_to_target_length / g_speed_in_attack;
			float angle= mxAcosClamped( dot );
			float rot_speed= angle / time_left;
			float d_angle= rot_speed * main_loop.GetTickTime();

			float rot_mat[16];
			mxMat4RotateAroundVector( rot_mat, rotation_axis, d_angle );
			for( unsigned int i= 0; i < 3; i++ )
				mxVec3Mat4Mul( axis_[i], rot_mat );
			CorrectAxis();

			// Move
			float d_pos[3];
			mxVec3Mul( vec_to_target, tick_step / vec_to_target_length, d_pos );
			mxVec3Add( pos_, d_pos );
		}
		Resume();
	}

	wait_a_bit:
	{
		float wait_start_time= main_loop.GetTime();
		static const float c_wait_interval= 1.0f / 4.0f;
		while( main_loop.GetTime() < wait_start_time + c_wait_interval ) Resume();
	}
	goto select_new_target_point;

	attack_target:
	float shot_time= main_loop.GetTime();

	unsigned int shot_count;
	float shot_interval;
	BulletType bullet;

	switch( type_ )
	{
	case MonsterOctoRobot:
		shot_count= 2;
		shot_interval= mx_GameConstants::rocket_launcher_shot_interval;
		bullet= Rocket;
		break;

	case PyramidRobot:
		shot_count= 8;
		shot_interval= mx_GameConstants::machinegun_shot_interval;
		bullet= MachinegunBullet;
		break;

	default:
		// TODO - remove
		shot_count= 8;
		shot_interval= mx_GameConstants::machinegun_shot_interval;
		bullet= MachinegunBullet;
		MX_ASSERT(false);
		break;
	};
	for( unsigned int i= 0; i < shot_count; )
	{
		float t= main_loop.GetTime();
		if( t - shot_time >= shot_interval )
		{
			level_.Shot( this, bullet, pos_, axis_[1] );
			shot_time= t;
			i++;
		}
		Resume();
	}
	goto select_new_target_point;
}

bool mx_Monster::SelectTargetPosition( float* out_pos )
{
	static const float c_min_distance= 1.0f;
	static const float c_max_distance= 6.0f;
	static const float c_radius= 0.5f;

	float square_dist_to_player = mxSquareDistance( pos_, player_.Pos() );
	if( square_dist_to_player >= c_max_distance * c_max_distance )
	{
		float vec_to_player[3];
		mxVec3Sub( player_.Pos(), pos_, vec_to_player );

		mxVec3Mul(
			vec_to_player,
			rand_.RandF( c_max_distance * 0.5f, c_max_distance ) / std::sqrt(square_dist_to_player),
			out_pos );
		mxVec3Add( out_pos, pos_ );
		return true;
	}

	for( unsigned int p= 0; p < 256; p++ )
	{
		float vec[3];
		float square_lengh= 0.0f;
		for( unsigned int i= 0; i < 3; i++ )
		{
			vec[i]= c_max_distance * rand_.RandF( -1.0f, 1.0f );
			square_lengh+= vec[i] * vec[i];
		}
		if( square_lengh > c_max_distance * c_max_distance ) continue; // reject points outside identity sphere
		if( square_lengh < c_min_distance * c_min_distance ) continue; // too close

		mxVec3Add( player_.Pos(), vec, out_pos );

		for( unsigned int i= 0; i < 3; i++ )
			if( out_pos[i] < home_sector_.bb_min[i] + c_radius ||
				out_pos[i] > home_sector_.bb_max[i] - c_radius )
				goto next;

		float movemend_square_dist= mxSquareDistance( out_pos, pos_ );
		if( movemend_square_dist >
			g_speed_in_attack * g_speed_in_attack *
			g_max_attack_moving_time_s * g_max_attack_moving_time_s )
			continue; // too long movement

		if( movemend_square_dist > square_lengh * 2.0f ) // movement segment is to close to center point
			continue;

		return true;
	next:;
	}

	return false;
}