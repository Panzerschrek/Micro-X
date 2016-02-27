#include "main_loop.h"
#include "models.h"
#include "monster.h"
#include "mx_assert.h"
#include "mx_math.h"
#include "particles_manager.h"
#include "player.h"
#include "sound_engine.h"

#include "level.h"

#define MX_BLAST_LIFETIME 1.0f

static bool CollideWithEdge( const float* v0, const float* v1, float* in_out_pos, float radius )
{
	float v0_to_v1_vec[3];
	mxVec3Sub( v1, v0, v0_to_v1_vec );
	float v0_to_v1_vec_squre_len= mxVec3SquareLen( v0_to_v1_vec );

	float vec_to_v0[3];
	mxVec3Sub( in_out_pos, v0, vec_to_v0 );
	float vec_to_v0_square_len= mxVec3SquareLen( vec_to_v0 );

	float projection= mxVec3Dot( vec_to_v0, v0_to_v1_vec );
	if( projection <= 0.0f )
	{
		float dist= std::sqrtf(vec_to_v0_square_len);
		if( dist >= radius )
			return false;

		float d_pos[3];
		mxVec3Mul( vec_to_v0, radius / dist, d_pos );
		mxVec3Add( in_out_pos, d_pos );
		return true;
	}
	else if( projection >= std::sqrtf( v0_to_v1_vec_squre_len ) )
	{
		float projection_vec[3];
		mxVec3Mul( v0_to_v1_vec, projection / std::sqrtf(v0_to_v1_vec_squre_len), projection_vec );

		float normal_vec[3];
		mxVec3Sub( vec_to_v0, projection_vec, normal_vec );

		float normal_vec_len= mxVec3Len( normal_vec );
		if( normal_vec_len >= radius )
			return false;

		mxVec3Normalize( normal_vec );
		float d_dist= radius - normal_vec_len;
		float d_pos[3];
		mxVec3Mul( normal_vec, d_dist, d_pos );
		mxVec3Add( in_out_pos, d_pos );
		return true;
	}
	else
	{
		float vec_to_v1[3];
		mxVec3Sub( in_out_pos, v1, vec_to_v1 );
		float dist= mxVec3Len( vec_to_v1 );

		if( dist >= radius )
			return false;

		float d_pos[3];
		mxVec3Mul( vec_to_v1, radius / dist, d_pos );
		mxVec3Add( in_out_pos, d_pos );
		return true;
	}
}

mx_Level::mx_Level( const mx_LevelData& level_data, mx_Player& player )
	: player_(player)
	, level_data_(level_data)
	, monster_count_(0)
	, bullet_count_(0)
	, blast_count_(0)
	, particles_manager_(new mx_ParticlesManager)
{
	mx_Rand rand;

	mx_LevelSector* player_sector;
	do
	{
		player_sector= &level_data_.sectors[ rand.Rand() % level_data_.sector_count ];
	} while( !(
		player_sector->type == mx_LevelSector::ROOM  &&
		!player_sector->has_icosahedron &&
		!player_sector->is_central_sector ) );

	{ // spawn player
		float pos[3];
		mxVec3Add( player_sector->bb_min, player_sector->bb_max, pos );
		mxVec3Mul( pos, 0.5f );
		player_.SetPos( pos );

		// Remove all ammo from this sector
		player_sector->ammo_box_count= 0;
	}

	for( unsigned int i= 0; i < level_data_.sector_count && monster_count_ < MX_MAX_MONSTERS; i++ )
	{
		const mx_LevelSector& sector= level_data_.sectors[i];
		if( &sector == player_sector || sector.type != mx_LevelSector::ROOM )
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

void mx_Level::PrepareBlastLights( mx_Light* out_lights ) const
{
	float total_time= mx_MainLoop::Instance()->GetTime();

	for( unsigned int i= 0; i < blast_count_; i++ )
	{
		const Blast& blast= blasts_[i];
		mx_Light& light= out_lights[i];
		VEC3_CPY( light.pos, blast.pos );

		float relative_life_time= ( total_time - blast.start_time ) / MX_BLAST_LIFETIME;
		float luminance=
			20.0f *
			relative_life_time *
			std::cosf( relative_life_time * 1.5f ) /
			std::expf( relative_life_time * 2.0f );

		mxVec3Mul(
			mx_GameConstants::bullets_colors[Rocket],
			luminance,
			light.light_rgb );
	}
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

bool mx_Level::CollideWithSectorTriangles( float* in_out_pos, float radius, const mx_LevelSector* sector ) const
{
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
		mxVec3Sub( in_out_pos, vertex.xyz, vec_from_triangle_to_pos );

		float projection_to_normal= mxVec3Dot( vec_from_triangle_to_pos, normal );
		if( projection_to_normal > radius )
			continue; // pos is too far from triangle plane

		float vect_from_plane_to_pos[3];
		mxVec3Mul( normal, projection_to_normal, vect_from_plane_to_pos );

		float point_on_plane[3];
		mxVec3Sub( in_out_pos, vect_from_plane_to_pos, point_on_plane );

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
		mxVec3Add( point_on_plane, pos_delta, in_out_pos );

outside_triangle:
		for( unsigned int j= 0; j < 3; j++ )
		{
			if( CollideWithEdge(
				level_data_.vertices[ triangle.vertex_index[(j + 1) % 3] ].xyz,
				level_data_.vertices[ triangle.vertex_index[j] ].xyz,
				in_out_pos,
				radius ) )
				collided= true;
		}
	}

	return collided;
}

bool mx_Level::CollideWithMonsters( float* in_out_pos, float radius ) const
{
	bool collided= false;

	for( unsigned int m= 0; m < monster_count_; m++ )
	{
		const mx_Monster* monster= monsters_[m];

		float min_allowed_dist= radius + mx_GameConstants::monsters_radius[monster->GetType()];
		if( mxDistance( monster->Pos(), in_out_pos ) < min_allowed_dist )
		{
			float dir[3];
			mxVec3Sub( in_out_pos, monster->Pos(), dir );
			mxVec3Mul( dir, min_allowed_dist / mxVec3Len(dir) );
			mxVec3Add( monster->Pos(), dir, in_out_pos );
			collided= true;
		}
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

	// Process blasts
	for( unsigned int b= 0; b < blast_count_; )
	{
		Blast& blast= blasts_[b];
		if( blast.start_time + MX_BLAST_LIFETIME <= total_time )
		{
			if( b != blast_count_ - 1 )
				blasts_[b]= blasts_[ blast_count_ - 1 ];
			blast_count_--;
			continue;
		}
		b++;
	}

	// Process particles. Make this BEFORE after logic, wher we can add particles.
	particles_manager_->Tick( dt );

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

		if( bullet.owner != &player_ )
		{
			float new_bullet_pos[3];
			mxVec3Mul( bullet.speed, dt, new_bullet_pos );
			mxVec3Add( new_bullet_pos, bullet.pos );

			if( mxSquareDistance( new_bullet_pos, player_.Pos() )
				<= mx_GameConstants::player_radius * mx_GameConstants::player_radius )
			{
				player_.Hit( mx_GameConstants::bullets_damage[bullet.type] );
				dead= true;
				goto kill;
			}
		}

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
				if( mxBeamIntersectTriangle( triangle, bullet.pos, dir, max_dist, intersection_pos ) )
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
			if( bullet.type == Rocket )
				RocketBlast( bullet.pos );

			if( b != bullet_count_ - 1u )
				bullets_[b]= bullets_[ bullet_count_ - 1u ];
			bullet_count_--;
			continue;
		}

		float d_pos[3];
		mxVec3Mul( bullet.speed, dt, d_pos );
		mxVec3Add( bullet.pos, d_pos );
		particles_manager_->AddBullet( &bullet );

		b++;
	}

	if( mx_LevelSector* sector= const_cast<mx_LevelSector*>( player_.GetSector() ) )
	{
		// Try pickup ammo
		for( unsigned int a= 0; a < sector->ammo_box_count; )
		{
			mx_AmmoBox& box= sector->ammo_boxes[a];

			if( mxSquareDistance( player_.Pos(), box.pos ) <= 
				mx_GameConstants::ammo_pockup_radius * mx_GameConstants::ammo_pockup_radius )
			{
				player_.AddAmmo( box.type, mx_GameConstants::ammo_boxes_bullets_count[box.type] );

				mx_SoundEngine::Instance()->AddSingleSound( SoundPowerupPickup, 1.0f, 1.0f, box.pos );

				if( a != sector->ammo_box_count - 1 )
					sector->ammo_boxes[a]= sector->ammo_boxes[sector->ammo_box_count - 1];

				sector->ammo_box_count--;
				continue;
			}
			a++;
		}

		// Try pickup icosahedrons
		if( sector->has_icosahedron && !sector->icosahedron_picked )
		{
			if( mxSquareDistance( player_.Pos(), sector->icosahedron_pos ) <= 
				mx_GameConstants::icosahedron_pickup_radius * mx_GameConstants::icosahedron_pickup_radius )
			{
				mx_SoundEngine::Instance()->AddSingleSound( SoundPowerupPickup, 1.0f, 1.0f, sector->icosahedron_pos );
				sector->icosahedron_picked= true;
			}
		}
	}

	// Remove killed monsters
	for( unsigned int m= 0; m < monster_count_; )
	{
		if( monsters_[m]->GetHealth() <= 0 )
		{
			AddBlast( monsters_[m]->Pos() );
			delete monsters_[m];

			if( m < monster_count_ - 1 )
				monsters_[m]= monsters_[ monster_count_ - 1 ];

			monster_count_--;
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

	// TODO - remove if, place table
	mx_SoundType sound_type;
	if( bullet_type == MachinegunBullet ) sound_type= SoundMachinegunShot;
	else if( bullet_type == Rocket ) sound_type= SoundAutomaticCannonShot;
	else sound_type= SoundPlasmagunShot;
	mx_SoundEngine::Instance()->AddSingleSound( sound_type, 1.0f, 1.0f, bullet.pos );

	bullet_count_++;
}

void mx_Level::WarnMonsters()
{
	for( unsigned int m= 0; m < monster_count_; m++ )
	{
		mx_Monster* monster= monsters_[m];
		if( &monster->GetSector() == player_.GetSector() )
			monster->Warn();
	}
}

void mx_Level::RocketBlast( const float* pos )
{
	AddBlast( pos );

	for( unsigned int m= 0; m < monster_count_ + 1; m++ )
	{
		mx_Pawn* pawn;
		if( m == monster_count_ )
			pawn= &player_;
		else
			pawn= monsters_[m];

		float square_dist= mxSquareDistance( pos, pawn->Pos() );
		if( square_dist < mx_GameConstants::rocket_blast_max_damage_distance * mx_GameConstants::rocket_blast_max_damage_distance )
		{
			int damage=
				min(
					int( mx_GameConstants::rocket_blast_damage_on_distance_1 / square_dist ),
					mx_GameConstants::rocket_blast_max_damage );
			if( damage != 0 )
				pawn->Hit( damage );
		}
	}
}

void mx_Level::AddBlast( const float* pos )
{
	mx_SoundEngine::Instance()->AddSingleSound( SoundBlast, 1.0f, 1.0f, pos );
	particles_manager_->AddBlast( pos );

	Blast& blast= blasts_[ blast_count_ ];
	VEC3_CPY( blast.pos, pos );
	blast.start_time= mx_MainLoop::Instance()->GetTime();

	blast_count_++;
}