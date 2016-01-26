#include "main_loop.h"
#include "mx_math.h"

#include "monster.h"

mx_Monster::mx_Monster( const float* pos, const mx_PatrolPath* patrol_path )
	: patrol_path_( patrol_path ? *patrol_path : mx_PatrolPath() )
	, have_patrol_path_( patrol_path != NULL )
	, destroy_(false)
{
	if( have_patrol_path_ )
	{
		VEC3_CPY( pos_, patrol_path_.points[0] );
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

	while( !destroy_ )
	{
		if( have_patrol_path_ )
		{
			float vec_to_target[3];
			mxVec3Sub( patrol_path_.points[ target_path_point ], pos_, vec_to_target );

			static const float c_speed= 0.8f;
			float tick_step= c_speed * main_loop.GetTickTime();
			if( tick_step * tick_step >= mxVec3SquareLen( vec_to_target ) )
				target_path_point= target_path_point ^ 1;
			else
			{
				float d_pos[3];
				mxVec3Mul( vec_to_target, tick_step / mxVec3Len(vec_to_target), d_pos );
				mxVec3Add( pos_, d_pos );
			}

			Resume();
		}
	}
}