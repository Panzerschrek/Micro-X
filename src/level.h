#pragma once

#include "level_generator.h"

#define MX_MAX_MONSTERS 256
#define MX_MAX_BULLETS 512

class mx_Monster;

struct mx_Bullet
{
	float pos[3];
	float speed[3];
	float birth_time;
};

class mx_Level
{
public:
	mx_Level( const mx_LevelData& level_data );
	~mx_Level();

	const mx_LevelVertex* GetVertices() const;
	unsigned int GetVertexCount() const;

	const mx_LevelTriangle* GetTriangles() const;
	unsigned int GetTriangleCount() const;

	const mx_Monster* const* GetMonsters() const;
	unsigned int GetMonsterCount() const;

	const mx_Bullet* GetBullets() const;
	unsigned int GetBulletCount() const;

	const mx_LevelData::Sector* FindSectorForPoint( const float* point ) const;

	bool CollideWithSectorTriangles(
		const float* in_pos, float radius,
		const mx_LevelData::Sector* sector,
		float* out_pos ) const;

	void Tick();
	void Shot( const float* pos, const float* normalized_dir );

private:
	mx_Level(const mx_Level&);
	mx_Level& operator=(const mx_Level&);

private:
	const mx_LevelData level_data_;

	unsigned int monster_count_;
	mx_Monster* monsters_[ MX_MAX_MONSTERS ];

	unsigned int bullet_count_;
	mx_Bullet bullets_[ MX_MAX_BULLETS ];
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

inline const mx_Bullet* mx_Level::GetBullets() const
{
	return bullets_;
}

inline unsigned int mx_Level::GetBulletCount() const
{
	return bullet_count_;
}