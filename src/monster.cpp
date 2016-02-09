#include "level.h"
#include "main_loop.h"
#include "mx_math.h"
#include "player.h"

#include "monster.h"

static const float g_axises[3][3][3]=
{
	{ {0.0f,-1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
	{ {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
	{ {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} },
};

static const float g_speed_in_patrol= 0.8f;
static const float g_speed_in_attack= 2.0f;

static const float g_rotation_speed_in_patrol= 1.0f;

mx_Monster::mx_Monster(
	MonsterType type,
	const mx_LevelSector& home_sector,
	mx_Level& level,
	const mx_Player& player,
	const float* pos,
	const mx_PatrolPath* patrol_path )
	: type_(type)
	, home_sector_(home_sector)
	, level_(level)
	, player_(player)
	, health_(100)
	, patrol_path_( patrol_path ? *patrol_path : mx_PatrolPath() )
	, have_patrol_path_( patrol_path != NULL )
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

		float vec_to_target[3];
		mxVec3Sub( patrol_path_.points[ target_path_point ], pos_, vec_to_target );
		mxVec3Normalize( vec_to_target );

		float cross[3];
		float dot= mxVec3Dot( vec_to_target, axis_[1] );
		mxVec3Cross( axis_[1], vec_to_target, cross );
		float projection_to_axis2= mxVec3Dot( cross, axis_[2] );
		float angle= std::asinf( mxClamp( -0.99f, 0.99f, mxVec3Len(cross) * mxSign(projection_to_axis2) ) );

		// Hack for 180deg angle
		if( dot < 0.0f && angle < 0.001f )
			angle= 0.001f;

		float da= g_rotation_speed_in_patrol * main_loop.GetTickTime();

		bool rotation_finished= false;
		if( dot > 0.0f && da >= std::fabsf(angle) )
		{
			da= std::fabsf(angle);
			rotation_finished= true;
		}

		float mat[16];
		mxMat4RotateAroundVector( mat, axis_[2], da );
		mxVec3Mat4Mul( axis_[0], mat );
		mxVec3Mat4Mul( axis_[1], mat );
		CorrectAxis();

		if( rotation_finished )
			goto move_to_target;

		Resume();
	}
}

bool mx_Monster::NeedStopAttack()
{
	return player_.GetSector() != &home_sector_;
}

bool mx_Monster::CanAttack()
{
	if( player_.GetSector() == &home_sector_ )
	{
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
			float rotation_axis_length= mxVec3Len( rotation_axis );

			static const float c_angle_sin_eps= 0.001f;
			if( dot < 0.0f && rotation_axis_length < c_angle_sin_eps )
			{
				// if opposite to needed direction, select random turn direction
				for( unsigned int i= 0; i < 3; i++ )
					rotation_axis[i]= rand_.RandF( -0.1f, 0.1f );
				rotation_axis_length= mxVec3Len( rotation_axis );
			}
			if( rotation_axis_length >= c_angle_sin_eps )
			{
				float time_left= vec_to_target_length / g_speed_in_attack;
				float angle= std::asin( rotation_axis_length );
				float rot_speed= angle / time_left;
				float d_angle= rot_speed * main_loop.GetTickTime();

				float rot_mat[16];
				mxMat4RotateAroundVector( rot_mat, rotation_axis, d_angle );
				for( unsigned int i= 0; i < 3; i++ )
					mxVec3Mat4Mul( axis_[i], rot_mat );
				CorrectAxis();
			}

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
	for( unsigned int i= 0; i < 2; )
	{
		float t= main_loop.GetTime();
		static const float c_shot_interval= 1.0f / 4.0f;
		if( t - shot_time >= c_shot_interval )
		{
			level_.Shot( this, pos_, axis_[1] );
			shot_time+= c_shot_interval;
			i++;
		}
		Resume();
	}
	goto select_new_target_point;
}

bool mx_Monster::SelectTargetPosition( float* out_pos )
{
	for( unsigned int p= 0; p < 128; p++ )
	{
		static const float c_min_distance= 1.0f;
		static const float c_max_distance= 6.0f;
		static const float c_radius= 0.5f;

		float vec[3];
		float square_lengh= 0.0f;
		for( unsigned int i= 0; i < 3; i++ )
		{
			vec[i]= c_max_distance * rand_.RandF( -1.0f, 1.0f );
			square_lengh+= vec[i] * vec[i];
		}
		if( square_lengh > c_max_distance * c_max_distance ) continue; // reject points outside identity sphere
		if( square_lengh < c_min_distance * c_min_distance ) continue;

		mxVec3Add( player_.Pos(), vec, out_pos );

		for( unsigned int i= 0; i < 3; i++ )
			if( out_pos[i] < home_sector_.bb_min[i] + c_radius ||
				out_pos[i] > home_sector_.bb_max[i] - c_radius )
				goto next;

		return true;
	next:;
	}

	return false;
}