#include "main_loop.h"
#include "mx_math.h"

#include "monster.h"

static const float g_axises[3][3][3]=
{
	{ {0.0f,-1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
	{ {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
	{ {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} },
};

mx_Monster::mx_Monster( const float* pos, const mx_PatrolPath* patrol_path )
	: type_(MonsterOctoRobot)
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
			float vec_to_target[3];
			mxVec3Sub( patrol_path_.points[ target_path_point ], pos_, vec_to_target );

			static const float c_speed= 0.8f;
			float tick_step= c_speed * main_loop.GetTickTime();
			if( tick_step * tick_step >= mxVec3SquareLen( vec_to_target ) )
			{
				target_path_point= target_path_point ^ 1;
				goto rotate_to_target;
			}
			else
			{
				float d_pos[3];
				mxVec3Mul( vec_to_target, tick_step / mxVec3Len(vec_to_target), d_pos );
				mxVec3Add( pos_, d_pos );
			}

			Resume();
		}
	}

rotate_to_target:
	// TODO - this code of rotation is very strange.
	while( !destroy_ )
	{
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

		static const float c_rot_speed= 1.0f;
		float da= c_rot_speed * main_loop.GetTickTime();

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