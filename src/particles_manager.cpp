#include "game_constants.h"
#include "mx_assert.h"

#include "particles_manager.h"

#define MX_ROCKET_TRAIL_PARTICLE_LIFETIME 2.0f
#define MX_BLAST_FIRE_LIFETIME 0.5f

#define MX_SPAWN_PARTICLE_LIFETIME 2.0f

mx_ParticlesManager::mx_ParticlesManager()
	: particle_count_(0)
	, prev_tick_time_(0.0f), current_tick_time_(0.0000001f), dt_(current_tick_time_ - prev_tick_time_)
{
}

mx_ParticlesManager::~mx_ParticlesManager()
{
}

void mx_ParticlesManager::Tick( float dt )
{
	prev_tick_time_= current_tick_time_;
	current_tick_time_+= dt;
	dt_= current_tick_time_ - prev_tick_time_;

	Particle* particle= particles_;
	for( unsigned int i= 0; i< particle_count_; )
	{
		float d_pos[3];
		mxVec3Mul( particle->direction,
			dt_ * ( particle->velocity + 0.5f * dt_ * particle->acceleration ),
			d_pos );
		mxVec3Add( particle->pos, d_pos );
		particle->velocity+= particle->acceleration * dt_;
		if( particle->velocity < 0.0f ) particle->velocity= 0.0f;

		if( current_tick_time_ - particle->spawn_time >= particle->life_time )
		{
			particle_count_--;
			if( i == particle_count_ )
				break;
			*particle= particles_[ particle_count_ ];
		}
		else
		{
			i++;
			particle++;
		}
	}
}

void mx_ParticlesManager::AddBullet( const mx_Bullet* bullet )
{
	switch( bullet->type )
	{
	case MachinegunBullet:
		break;

	case Rocket:
		AddRocketTrail( bullet );
		break;
	
	case PlasmaBall:
		AddPlasmaBall( bullet );
		break;

	default:
		MX_ASSERT(false);
		break;
	};
}

void mx_ParticlesManager::AddBlast( const float* pos )
{
	const unsigned int c_particles_count= 768;

	Particle* particle= particles_ + particle_count_;
	for( unsigned int i= 0; i< c_particles_count; )
	{
		for( unsigned int j= 0; j< 3; j++ )
			particle->direction[j]= randomizer_.RandF( -1.0f, 1.0f );
		float direction_length = mxVec3Len( particle->direction );
		if( direction_length > 1.0f ) continue;
		mxVec3Mul( particle->direction, 1.0f / direction_length );

		VEC3_CPY( particle->pos, pos );

		particle->velocity= randomizer_.RandF( 1.0f ) + randomizer_.RandF( 1.0f );
		particle->acceleration= -0.9f - randomizer_.RandF( -0.2f, 0.2f );
		particle->type= Particle::RocketBlast;
		particle->spawn_time= current_tick_time_;
		particle->life_time= MX_BLAST_FIRE_LIFETIME;

		i++, particle++;
	}
	particle_count_+= c_particles_count;
}

void mx_ParticlesManager::AddSpawn( const float* pos )
{
	const unsigned int c_particles_count= 256;

	Particle* particle= particles_ + particle_count_;
	for( unsigned int i= 0; i< c_particles_count; )
	{
		for( unsigned int j= 0; j< 3; j++ )
			particle->direction[j]= randomizer_.RandF( -1.0f, 1.0f );
		float direction_length = mxVec3Len( particle->direction );
		if( direction_length > 1.0f && direction_length < 0.5f ) continue;
		mxVec3Mul( particle->direction, 1.0f / direction_length );

		VEC3_CPY( particle->pos, pos );

		particle->velocity= 0.6f;
		particle->acceleration= 0.6f / 2.0f;
		particle->type= Particle::Spawn;
		particle->spawn_time= current_tick_time_;
		particle->life_time= MX_SPAWN_PARTICLE_LIFETIME;
		particle->id= i;

		i++, particle++;
	}
	particle_count_+= c_particles_count;
}

void mx_ParticlesManager::PrepareParticlesVertices( mx_ParticleVertex* out_vertices ) const
{
	const Particle* particle= particles_;
	mx_ParticleVertex* vertex= out_vertices;
	for( unsigned int i= 0; i< particle_count_; i++, particle++, vertex++ )
	{
		VEC3_CPY( vertex->pos_size, particle->pos );
		switch( particle->type )
		{
		case Particle::RocketBlast:
			{
				float lifetime_k= ( current_tick_time_ - particle->spawn_time ) * ( 1.0f / MX_BLAST_FIRE_LIFETIME );
				vertex->pos_size[3]= 0.02f + 0.11f * lifetime_k;

				float luminance= 1.0f - lifetime_k;
				luminance= luminance * luminance;
				mxVec3Mul( mx_GameConstants::bullets_colors[Rocket], 1.3f * luminance, vertex->color );
			}
			break;
		case Particle::RocketTrail:
			{
				float lifetime_k= ( current_tick_time_ - particle->spawn_time ) * ( 1.0f / MX_ROCKET_TRAIL_PARTICLE_LIFETIME  );
				vertex->pos_size[3]= 0.02f + 0.08f * lifetime_k;

				float luminance= std::powf( 1.0f - lifetime_k, 6.0f );
				mxVec3Mul( mx_GameConstants::bullets_colors[Rocket], 0.8f * luminance, vertex->color );
			}
			break;
		case Particle::PlasmaBall:
			{
				vertex->pos_size[3]= 0.05f;
				mxVec3Mul( mx_GameConstants::bullets_colors[PlasmaBall], 0.5f, vertex->color );
			}
			break;
		case Particle::Spawn:
			{
				vertex->pos_size[3]= 0.03f;

				float lifetime_k= ( current_tick_time_ - particle->spawn_time ) * ( 1.0f / MX_SPAWN_PARTICLE_LIFETIME );
				static const float c_colors[2][3]=
				{
					{ 1.0f, 0.1f, 1.0f },
					{ 0.0f, 1.0f, 0.0f },
				};

				mxVec3Mul( c_colors[ particle->id & 1 ], 1.0f - lifetime_k, vertex->color );
			};
		default: MX_ASSERT(false); break;
		} // switch type
	}
}

void mx_ParticlesManager::AddRocketTrail( const mx_Bullet* rocket )
{
	const float c_particles_per_meter= 40.0f;

	float rocket_speed= mxVec3Len(rocket->speed);
	float particles_per_second= c_particles_per_meter * rocket_speed;

	unsigned int particle_count= (unsigned int)
		( std::floorf(current_tick_time_ * particles_per_second) - std::ceilf(prev_tick_time_ * particles_per_second) )
		+ 1u;

	Particle* particle= particles_ + particle_count_;
	float partice_pos[3];
	float particle_step[3];
	float particle_dir[3];
	VEC3_CPY( partice_pos, rocket->pos );
	mxVec3Mul( rocket->speed, -1.0f / rocket_speed, particle_dir );
	mxVec3Mul( particle_dir, 1.0f / c_particles_per_meter, particle_step );
	float dt= rocket_speed / c_particles_per_meter;
	float t= current_tick_time_;

	float unused;
	float part= std::modf(current_tick_time_ * particles_per_second, &unused );
	t-= dt * part;
	float pos_add_vec[3];
	mxVec3Mul( particle_step, part, pos_add_vec );
	mxVec3Add( partice_pos, pos_add_vec );

	for( unsigned int i= 0; i< particle_count; i++, mxVec3Add( partice_pos, particle_step ), t-= dt, particle++ )
	{
		particle->type= Particle::RocketTrail;
		VEC3_CPY( particle->pos, partice_pos );
		VEC3_CPY( particle->direction, particle_dir );

		float rand_dir[3];
		for( unsigned int j= 0; j < 3; j++ )
			rand_dir[j]= randomizer_.RandF( -0.2f, 0.2f );
		mxVec3Add( particle->direction, rand_dir );
		mxVec3Normalize( particle->direction );

		particle->velocity= 0.3f;
		particle->acceleration= -0.15f;
		particle->spawn_time= t;
		particle->life_time= MX_ROCKET_TRAIL_PARTICLE_LIFETIME;
	}
	particle_count_+= particle_count;
}

void mx_ParticlesManager::AddPlasmaBall( const mx_Bullet* plasma_ball )
{
	const unsigned int c_particles_in_ball= 9;
	const float c_step= 0.05f;

	float dir[3];
	mxVec3Normalize( plasma_ball->speed, dir );

	float pos[3];
	float d_pos[3];
	VEC3_CPY( pos, plasma_ball->pos );
	mxVec3Mul( dir, c_step * float(c_particles_in_ball) * 0.5f, d_pos );
	mxVec3Sub( pos, d_pos );

	float pos_step[3];
	mxVec3Mul( dir, c_step, pos_step );

	Particle* p= particles_ + particle_count_;
	for( unsigned int i= 0; i < c_particles_in_ball; i++, p++, mxVec3Add( pos, pos_step ) )
	{
		p->type= Particle::PlasmaBall;
		p->spawn_time= current_tick_time_;
		p->life_time= 0.0f;
		VEC3_CPY( p->pos, pos );
	}

	particle_count_+= c_particles_in_ball;
}