#include "main_loop.h"
#include "monster.h"
#include "mx_assert.h"
#include "mx_math.h"
#include "sound_engine.h"

#include "level.h"

mx_Level::mx_Level( const mx_LevelData& level_data )
	: level_data_(level_data)
	, monster_count_(0)
	, bullet_count_(0)
{
	for( unsigned int i= 0; i < level_data_.sector_count && monster_count_ < MX_MAX_MONSTERS; i++ )
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
		path.path_plane= longest_coord;

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
	float dt= mx_MainLoop::Instance()->GetTickTime();
	float total_time= mx_MainLoop::Instance()->GetTime();

	for( unsigned int m= 0; m < monster_count_; m++ )
	{
		monsters_[m]->Exec();
	}

	for( unsigned int b= 0; b < bullet_count_; )
	{
		mx_Bullet& bullet= bullets_[b];

		bool dead= false;
		if(  total_time - bullet.birth_time > 3.0f )
		{
			dead= true;
			goto kill;
		}

		const mx_LevelData::Sector* sector= FindSectorForPoint( bullet.pos );
		if( sector )
		{
			float dir[3];
			mxVec3Normalize( bullet.speed, dir );
			float max_dist= mxVec3Len( bullet.speed ) * dt;

			for( unsigned int i= sector->first_triangle , i_end= sector->first_triangle + sector->triangles_count;
				i < i_end;
				i++ )
			{
				const float* triangle[3];
				for( unsigned int j= 0; j < 3; j++ )
					triangle[j]= level_data_.vertices[ level_data_.triangles[i].vertex_index[j] ].xyz;

				float intersection_pos[3];
				if( mxBeamIntersectModel( triangle, bullet.pos, dir, max_dist, intersection_pos ) )
				{
					dead= true;
					mx_SoundEngine::Instance()->AddSingleSound( SoundBlast, 1.0f, 1.0f, bullet.pos );
					break;
				}
			}
		}

kill:
		if( dead )
		{
			if( b != bullet_count_ - 1u )
				bullets_[b]= bullets_[ bullet_count_ - 1u ];
			bullet_count_--;
			continue;
		}

		float d_pos[3];
		mxVec3Mul( bullet.speed, dt, d_pos );
		mxVec3Add( bullet.pos, d_pos );
		
		b++;
	}
}

void mx_Level::Shot( const float* pos, const float* normalized_dir )
{
	MX_ASSERT( bullet_count_ <= MX_MAX_BULLETS );

	mx_Bullet& bullet= bullets_[bullet_count_];

	bullet.birth_time= mx_MainLoop::Instance()->GetTime();

	VEC3_CPY( bullet.pos, pos );

	const float c_speed= 5.0f;
	mxVec3Mul( normalized_dir, c_speed, bullet.speed );

	mx_SoundEngine::Instance()->AddSingleSound( SoundPlasmagunShot, 1.0f, 1.0f, bullet.pos );

	bullet_count_++;
}