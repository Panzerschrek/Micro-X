#pragma once
#include "mx_math.h"

#define MX_MAX_LEVEL_SIZE_CELLS 48
#define MX_MAX_ROOMS 64 * 2
#define MX_MAX_CONNECTIONS 256 * 4

#define MX_MAX_ROOM_SIZE 9
#define MX_MIN_ROOM_SIZE 2
#define MX_MAX_ROOM_CONNECTIONS 8

#define MX_MIN_ROOM_DISTANCE 3

struct mx_LevelVertex
{
	float xyz[3];
	float tex_coord[3];
	char normal[3];
};

struct mx_LevelData
{
	struct Plane
	{
		float normal[3];
		float dist;
	};

	struct Triangle
	{
		unsigned vertex_index[3];
	};
/*
	struct Sector
	{
		enum
		{
			ROOM,
			CONNECTION,
		} type;

		Plane planes[16];
		unsigned int planes_count;

		Sector* connections[MX_MAX_ROOM_CONNECTIONS];
		unsigned int connections_count;

		unsigned int first_triangle;
		unsigned int triangles_count;
	};
*/
	//Sector* sectors;

	mx_LevelVertex* vertices;
	unsigned int vertices_capacity;
	unsigned int vertex_count;

	Triangle* triangles;
	unsigned int triangles_capacity;
	unsigned int triangle_count;
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

	const mx_LevelData& GetLevelData();

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
		int coord_min[3];
		int coord_max[3];

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
	Element*& ElementMap( int x, int y, int z );

	void PlaceRooms();
	void PlaceConnections();
	bool TryPlaceConnection( Room* room, const int* begin_coord, const int* direction );

	static bool CheckConnection( const Room* room0, const Room* room1 );
	static void GenCube( const Room* room, mx_LevelVertex* vertices, unsigned short* indeces, unsigned int base_vertex );

	void GenerateMeshes();
	void AddRoomCube( const Room* room );
	void AddConnectionCube( const Connection* connection );
	
	void CalculateNormals();
	void ClaculateTextureCoordinates();

private:
	Element* element_map_[ MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS ];

	Room rooms_[ MX_MAX_ROOMS ];
	unsigned int room_count_;

	Connection connections_[ MX_MAX_CONNECTIONS ];
	unsigned int connection_count_;

	mx_Rand rand_;

	mx_LevelData out_level_data_;
};