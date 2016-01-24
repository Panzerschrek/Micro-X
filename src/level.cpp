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