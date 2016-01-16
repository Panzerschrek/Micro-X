#pragma once
#include "mx_math.h"

#define MX_MAX_LEVEL_SIZE_CELLS 64 * 2
#define MX_MAX_ROOMS 64 * 2
#define MX_MAX_CONNECTIONS 256 * 4

#define MX_MAX_ROOM_SIZE 9
#define MX_MIN_ROOM_SIZE 2
#define MX_MAX_ROOM_CONNECTIONS 8

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
	struct Element
	{
		enum
		{
			ROOM,
			CONNECTION,
		} type;
	};

	struct Room;
	struct Connection;

	struct Room : public Element
	{
		int coord_min[2];
		int coord_max[2];

		Connection* connections[ MX_MAX_ROOM_CONNECTIONS ];
		unsigned int connection_count;
	};

	struct Connection : public Element
	{
		int coord_begin[3];
		int coord_end[3];
		Room* begin;
		Room* end;
	};

private:
	Element*& ElementMap( int x, int y );

	void PlaceConnections();
	bool TryPlaceConnection( Room* room, const int* begin_coord, const int* direction );

private:
	Element* element_map_[ MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS ];

	Room rooms_[ MX_MAX_ROOMS ];
	unsigned int room_count_;

	Connection connections_[ MX_MAX_CONNECTIONS ];
	unsigned int connection_count_;

	mx_Rand rand_;
};