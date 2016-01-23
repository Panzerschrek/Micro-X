#pragma once

#include "level_generator.h"

class mx_Level
{
public:
	mx_Level( const mx_LevelData& level_data );
	~mx_Level();

	const mx_LevelVertex* GetVertices() const;
	unsigned int GetVertexCount() const;

	const mx_LevelTriangle* GetTriangles() const;
	unsigned int GetTriangleCount() const;

private:
	mx_Level(const mx_Level&);
	mx_Level& operator=(const mx_Level&);

private:
	const mx_LevelData level_data_;
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