#include "monster.h"
#include "mx_math.h"

#include "level.h"

mx_Level::mx_Level( const mx_LevelData& level_data )
	: level_data_(level_data)
	, monster_count_(0)
{
	for( unsigned int i= 0; i < level_data_.sector_count; i++, monster_count_ < MX_MAX_MONSTERS )
	{
		const mx_LevelData::Sector& sector= level_data_.sectors[i];
		if( sector.type != mx_LevelData::Sector::ROOM )
			continue;
		
		float pos[3];
		for( unsigned int j= 0; j < 3; j++ )
			pos[j]= ( sector.bb_min[j] + sector.bb_max[j] ) * 0.5f;

		unsigned int longest_coord= 0;
		float longest_width= 0.0f;
		for( unsigned int j= 0; j < 3; j++ )
		{
			float width= sector.bb_max[j] - sector.bb_min[j];
			if( width > longest_width )
			{
				longest_width= width;
				longest_coord= j;
			}
		}

		mx_PatrolPath path;
		VEC3_CPY( path.points[0], pos );
		VEC3_CPY( path.points[1], pos );
		path.points[0][longest_coord]= sector.bb_min[ longest_coord ] + 0.5f;
		path.points[1][longest_coord]= sector.bb_max[ longest_coord ] - 0.5f;

		monsters_[ monster_count_ ]= new mx_Monster( pos, &path );
		monster_count_++;
	}
}

mx_Level::~mx_Level()
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

bool mx_Level::CollideWithSectorTriangles( const float* in_pos, float radius, const mx_LevelData::Sector* sector, float* out_pos ) const
{
	VEC3_CPY( out_pos, in_pos );

	bool collided= false;

	for( unsigned int t= sector->first_triangle; t < sector->triangles_count + sector->first_triangle; t++ )
	{
		mx_LevelTriangle& triangle= level_data_.triangles[t];
		mx_LevelVertex& vertex= level_data_.vertices[ triangle.vertex_index[0] ];

		// TODO - calculate normal. Normal from triangle vertices may be incorrect
		float normal[3];
		for( unsigned int j= 0; j < 3; j++ )
			normal[j]= float( vertex.normal[j] );
		mxVec3Normalize( normal );

		float vec_from_triangle_to_pos[3];
		mxVec3Sub( out_pos, vertex.xyz, vec_from_triangle_to_pos );

		float projection_to_normal= mxVec3Dot( vec_from_triangle_to_pos, normal );
		if( projection_to_normal > radius )
			continue; // pos is too far from triangle plane

		float vect_from_plane_to_pos[3];
		mxVec3Mul( normal, projection_to_normal, vect_from_plane_to_pos );

		float point_on_plane[3];
		mxVec3Sub( out_pos, vect_from_plane_to_pos, point_on_plane );

		for( unsigned int j= 0; j < 3; j++ )
		{
			float vec_to_vertex[3];
			float edge_vec[3];
			float cross[3];

			mxVec3Sub(
				level_data_.vertices[ triangle.vertex_index[j] ].xyz,
				point_on_plane,
				vec_to_vertex );
			mxVec3Sub(
				level_data_.vertices[ triangle.vertex_index[(j + 1) % 3] ].xyz, 
				level_data_.vertices[ triangle.vertex_index[j] ].xyz,
				edge_vec );

			mxVec3Cross( vec_to_vertex, edge_vec, cross );
			float dot= mxVec3Dot( cross, normal );
			if( dot > 0.0f ) goto outside_triangle;
		}

		collided= true;

		float pos_delta[3];
		mxVec3Mul( normal, radius, pos_delta );
		mxVec3Add( point_on_plane, pos_delta, out_pos );

outside_triangle:;
	}

	return collided;
}

void mx_Level::Tick()
{
	for( unsigned int m= 0; m < monster_count_; m++ )
	{
		monsters_[m]->Exec();
	}
}