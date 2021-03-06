#pragma once

#include "drawing_model.h"
#include "fwd.h"
#include "game_constants.h"
#include "level_generator.h"
#include "mx_math.h"
#include "pawn.h"

#define MX_MAX_MONSTERS 256
#define MX_MAX_BULLETS 512
#define MX_MAX_BLAST_LIGHTS 64

#define MX_MAX_HEALTH_PACKS MX_MAX_MONSTERS

struct mx_Bullet
{
	BulletType type;
	mx_Pawn* owner;
	float pos[3];
	float speed[3];
	float birth_time;
};

struct mx_HealthPack
{
	float pos[3];
	const mx_LevelSector* sector;
};

class mx_Level
{
public:
	mx_Level( const mx_LevelData& level_data, mx_Player& player );
	~mx_Level();

	const mx_LevelVertex* GetVertices() const;
	unsigned int GetVertexCount() const;

	const mx_LevelTriangle* GetTriangles() const;
	unsigned int GetTriangleCount() const;

	const mx_Monster* const* GetMonsters() const;
	unsigned int GetMonsterCount() const;

	const mx_HealthPack* GetHealthPacks() const;
	unsigned int GetHealthPackCount() const;

	const mx_Bullet* GetBullets() const;
	unsigned int GetBulletCount() const;

	unsigned int GetBlastLightCount() const;
	void PrepareBlastLights( mx_Light* out_lights ) const;

	const mx_ParticlesManager* GetParticlesManager() const;

	const mx_LevelData& GetLevelData() const;

	const mx_LevelSector* FindSectorForPoint( const float* point ) const;

	bool CollideWithSectorTriangles(
		float* in_out_pos, float radius,
		const mx_LevelSector* sector ) const;

	bool CollideWithMonsters( float* in_out_pos, float radius ) const;

	void Tick();
	void Shot( mx_Pawn* shooter, BulletType bullet_type, const float* pos, const float* normalized_dir );
	void RespawnPlayer();

	void WarnMonsters();

private:
	struct Blast
	{
		float start_time;
		float pos[3];
	};

private:

	mx_Level(const mx_Level&);
	mx_Level& operator=(const mx_Level&);

	void RocketBlast( const float* pos );
	void AddBlast( const float* pos );

private:
	mx_Player& player_;
	const mx_LevelData level_data_;

	mx_Rand randomizer_;

	mx_LevelSector* player_sector_;
	unsigned int icosahedrons_left_;

	mx_DrawingModel monsters_models_[ LastMonster ];

	unsigned int monster_count_;
	mx_Monster* monsters_[ MX_MAX_MONSTERS ];

	unsigned int health_pack_count_;
	mx_HealthPack health_packs_[ MX_MAX_HEALTH_PACKS ];

	unsigned int bullet_count_;
	mx_Bullet bullets_[ MX_MAX_BULLETS ];

	unsigned int blast_count_;
	Blast blasts_[ MX_MAX_BLAST_LIGHTS ];

	mx_ParticlesManager* const particles_manager_;
};

inline const mx_LevelVertex* mx_Level::GetVertices() const
{
	return level_data_.vertices;
}

inline unsigned int mx_Level::GetVertexCount() const
{
	return level_data_.vertex_count;
}

inline const mx_LevelTriangle* mx_Level::GetTriangles() const
{
	return level_data_.triangles;
}

inline unsigned int mx_Level::GetTriangleCount() const
{
	return level_data_.triangle_count;
}

inline const mx_Monster* const* mx_Level::GetMonsters() const
{
	return monsters_;
}

inline unsigned int mx_Level::GetMonsterCount() const
{
	return monster_count_;
}

inline const mx_HealthPack* mx_Level::GetHealthPacks() const
{
	return health_packs_;
}

inline unsigned int mx_Level::GetHealthPackCount() const
{
	return health_pack_count_;
}

inline const mx_Bullet* mx_Level::GetBullets() const
{
	return bullets_;
}

inline unsigned int mx_Level::GetBulletCount() const
{
	return bullet_count_;
}

inline unsigned int mx_Level::GetBlastLightCount() const
{
	return blast_count_;
}

inline const mx_ParticlesManager* mx_Level::GetParticlesManager() const
{
	return particles_manager_;
}

inline const mx_LevelData& mx_Level::GetLevelData() const
{
	return level_data_;
}