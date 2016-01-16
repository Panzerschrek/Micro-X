#pragma once
#include "mx_math.h"

#define MX_MAX_LEVEL_SIZE_CELLS 64
#define MX_MAX_ROOMS 64

#define MX_MAX_ROOM_SIZE 9
#define MX_MIN_ROOM_SIZE 2

#define MX_MIN_ROOM_DISTANCE 3


struct mx_LevelVertex
{
	float xyz[3];
};

struct mx_LevelMesh
{
	mx_LevelVertex* vertices;
	unsigned short* triangles;
	unsigned int vertex_count;
	unsigned int triangle_count;
};

class mx_LevelGenerator
{
public:
	mx_LevelGenerator();
	~mx_LevelGenerator();

	void Generate();

	mx_LevelMesh GenerateLevelMesh() const;

private:
	struct Room
	{
		int coord_min[2];
		int coord_max[2];
	};

	Room* room_map_[ MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS ];

	Room rooms_[ MX_MAX_ROOMS ];
	unsigned int room_count_;

	mx_Rand rand_;
};