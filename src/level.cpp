#include "mx_math.h"

#include "level.h"

mx_Level::mx_Level( const mx_LevelData& level_data )
	: level_data_(level_data)
{
}

const mx_LevelData::Sector* mx_Level::FindSectorForPoint( const float* point ) const
{
	for(const  mx_LevelData::Sector* s= level_data_.sectors, *s_end= level_data_.sectors + level_data_.sector_count;
		s < s_end;
		s++ )
	{
		bool inside= s->planes_count > 0;
		for( unsigned int j= 0; j < s->planes_count; j++ )
		{
			if( mxVec3Dot( s->planes[j].normal, point ) < s->planes[j].dist )
			{
				inside= false;
				break;
			}
		}
		if( inside ) return s;
	}

	return NULL;
}