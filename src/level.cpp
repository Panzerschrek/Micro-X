#include "main_loop.h"
#include "models.h"
#include "monster.h"
#include "mx_assert.h"
#include "mx_math.h"
#include "sound_engine.h"

#include "level.h"

mx_Level::mx_Level( const mx_LevelData& level_data, mx_Player& player )
	: player_(player)
	, level_data_(level_data)
	, monster_count_(0)
	, bullet_count_(0)
{
	mx_Rand rand;

	for( unsigned int i= 0; i < level_data_.sector_count && monster_count_ < MX_MAX_MONSTERS; i++ )
	{
		const mx_LevelSector& sector= level_data_.sectors[i];
		if( sector.type != mx_LevelSector::ROOM )
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

		monsters_[ monster_count_ ]= new mx_Monster(
			MonsterType(rand.Rand() % LastMonster),
			sector,
			*this,
			player_,
			pos,
			&path );
		monster_count_++;
	}

	for( unsigned int i= 0; i < LastMonster; i++ )
	{
		monsters_models_[i].LoadFromMFMD( mx_Models::monsters_models[i] );
		monsters_models_[i].Scale( mx_Models::monsters_models_scale[i] );
	}
}

mx_Level::~mx_Level()
{
}

const mx_LevelSector* mx_Level::FindSectorForPoint( const float* point ) const
{
	for(const mx_LevelSector* s= level_data_.sectors, *s_end= level_data_.sectors + level_data_.sector_count;
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

bool mx_Level::CollideWithSectorTriangles( const float* in_pos, float radius, const mx_LevelSector* sector, float* out_pos ) const
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

	// Monsters think
	for( unsigned int m= 0; m < monster_count_; m++ )
	{
		monsters_[m]->Exec();
	}

	// Process bullets
	for( unsigned int b= 0; b < bullet_count_; )
	{
		mx_Bullet& bullet= bullets_[b];

		bool dead= false;
		if( total_time - bullet.birth_time > 3.0f )
		{
			dead= true;
			goto kill;
		}

		float dir[3];
		mxVec3Normalize( bullet.speed, dir );
		float max_dist= mxVec3Len( bullet.speed ) * dt;

		float nearest_hit_dist= mxInf();
		mx_Monster* hited_monster= NULL;
		//float nearest_hit_pos[3];

		// Collide with monsters first. Monsters are always inside sector
		for( unsigned int j= 0; j < monster_count_; j++ )
		{
			mx_Monster* monster= monsters_[j];
			if( bullet.owner == monster )
				continue;

			float monster_space_pos[3];
			float monster_space_dir[3];
			float monster_space_hit_pos[3];

			float pos_relative_monster[3];
			mxVec3Sub( bullet.pos, monster->Pos(), pos_relative_monster );

			float monster_rot_mat[16];
			//float monster_rot_mat_invert[16];
			monster->CreateRotationMatrix4( monster_rot_mat, false );
			//monster->CreateRotationMatrix4( monster_rot_mat_invert, true );
			mxVec3Mat4Mul( pos_relative_monster, monster_rot_mat, monster_space_pos );
			mxVec3Mat4Mul( dir, monster_rot_mat, monster_space_dir );
	
			if( monsters_models_[monster->GetType()].BeamIntersectModel( monster_space_pos, monster_space_dir, max_dist, monster_space_hit_pos ) )
			{
				float dist= mxDistance( monster_space_hit_pos, monster_space_pos );
				if( dist < nearest_hit_dist )
				{
					nearest_hit_dist= dist;
					hited_monster= monster;
					//mxVec3Mat4Mul( monster_space_hit_pos, monster_rot_mat_invert, nearest_hit_pos );
					//mxVec3Add( nearest_hit_pos, monster->Pos() );
				}
				dead= true;
			}
		} // for monsters

		if( hited_monster )
			hited_monster->Hit( mx_GameConstants::bullets_damage[bullet.type] );

		const mx_LevelSector* sector= FindSectorForPoint( bullet.pos );
		if( sector )
		{
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
					// TODO - find nearest intersection
					dead= true;
					break;
				}
			}
		} // collide with sector

kill:
		if( dead )
		{
			mx_SoundEngine::Instance()->AddSingleSound( SoundBlast, 1.0f, 1.0f, bullet.pos );

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

	// Remove killed monsters
	for( unsigned int m= 0; m < monster_count_; )
	{
		if( monsters_[m]->GetHealth() <= 0 )
		{
			if( m < monster_count_ - 1 )
				monsters_[m]= monsters_[ monster_count_ - 1 ];
			continue;
		}
		m++;
	}
}

void mx_Level::Shot( mx_Pawn* shooter, BulletType bullet_type, const float* pos, const float* normalized_dir )
{
	MX_ASSERT( bullet_count_ <= MX_MAX_BULLETS );

	mx_Bullet& bullet= bullets_[bullet_count_];

	bullet.owner= shooter;
	bullet.type= bullet_type;
	bullet.birth_time= mx_MainLoop::Instance()->GetTime();

	VEC3_CPY( bullet.pos, pos );

	mxVec3Mul( normalized_dir, mx_GameConstants::bullets_speed[bullet_type], bullet.speed );

	mx_SoundEngine::Instance()->AddSingleSound( SoundPlasmagunShot, 1.0f, 1.0f, bullet.pos );

	bullet_count_++;
}