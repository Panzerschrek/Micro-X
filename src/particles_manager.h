#pragma once
#include "level.h"
#include "mx_math.h"

#pragma pack(push, 1)
struct mx_ParticleVertex
{
	float pos_size[4];
	float color[4];

};
#pragma pack(pop)

#define MX_MAX_PARTICLES 8192

class mx_ParticlesManager
{
public:
	mx_ParticlesManager();
	~mx_ParticlesManager();

	void Tick( float dt );

	void AddBullet( const mx_Bullet* bullet );
	void AddBlast( const float* pos );

	unsigned int GetParticlesCount() const;
	void PrepareParticlesVertices( mx_ParticleVertex* out_vertices ) const;

private:
	struct Particle
	{
		enum Type
		{
			RocketTrail,
			RocketBlast,
			PlasmaBall,

		} type;

		float pos[3];
		float direction[3];
		float velocity;
		float acceleration;
		float spawn_time;
		float life_time;
	};

private:
	void AddRocketTrail( const mx_Bullet* rocket );
	void AddPlasmaBall( const mx_Bullet* plasma_ball );

	float prev_tick_time_;
	float current_tick_time_;
	float dt_;
	mx_Rand randomizer_;

	Particle particles_[ MX_MAX_PARTICLES ];
	unsigned int particle_count_;
};

inline unsigned int mx_ParticlesManager::GetParticlesCount() const
{
	return particle_count_;
}
