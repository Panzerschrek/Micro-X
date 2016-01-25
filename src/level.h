#pragma once

#include "level_generator.h"

#define MX_MAX_MONSTERS 256

class mx_Monster;

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

	const mx_LevelData::Sector* FindSectorForPoint( const float* point ) const;

	bool CollideWithSectorTriangles(
		const float* in_pos, float radius,
		const mx_LevelData::Sector* sector,
		float* out_pos ) const;

private:
	mx_Level(const mx_Level&);
	mx_Level& operator=(const mx_Level&);

private:
	const mx_LevelData level_data_;

	unsigned int monster_count_;
	mx_Monster* monsters_[ MX_MAX_MONSTERS ];
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