#pragma once
#include "mx_math.h"

#define MX_MAX_LEVEL_SIZE_CELLS 40
#define MX_MAX_ROOMS 64 * 2
#define MX_MAX_CONNECTIONS 256 * 4

#define MX_MAX_ROOM_SIZE 9
#define MX_MIN_ROOM_SIZE 2
#define MX_CENTRAL_ROOM_HALF_SIZE 4
#define MX_MAX_ROOM_CONNECTIONS 12

#define MX_MIN_ROOM_DISTANCE 3

#define MX_MAX_SECTOR_PLANES 8
#define MX_MAX_SECTOR_LIGHTS 16
#define MX_MAX_SECTOR_AMMO_BOXES 4

// 32bit struct
#pragma pack(push, 1)
struct mx_LevelVertex
{
	float xyz[3];
	float tex_coord[2];
	char binormal[3]; // x texture axis
	char tangent [3]; // y textur eaxis
	char normal  [3];
	unsigned char tex_id;
	unsigned char pad[2];
};
#pragma pack(pop)

struct mx_Light
{
	float pos[3];
	float light_rgb[4]; // 4th component - unused
};

struct mx_AmmoBox
{
	float pos[3];
	BulletType type;
};

struct mx_LevelTriangle
{
	unsigned vertex_index[3];
};

struct mx_Plane
{
	float normal[3];
	float dist;
};

struct mx_LevelSector
{
	enum
	{
		ROOM,
		CONNECTION,
	} type;

	float bb_min[3];
	float bb_max[3];

	mx_Plane planes[MX_MAX_SECTOR_PLANES];
	unsigned int planes_count;

	mx_Light lights[MX_MAX_SECTOR_LIGHTS];
	unsigned int light_count;

	mx_AmmoBox ammo_boxes[MX_MAX_SECTOR_AMMO_BOXES];
	unsigned int ammo_box_count;

	mx_LevelSector* connections[MX_MAX_ROOM_CONNECTIONS];
	unsigned int connections_count;

	unsigned int first_triangle;
	unsigned int triangles_count;

	// Unique number for each sector-graph based algorithms.
	unsigned int traverse_id;

	float icosahedron_pos[3];
	bool has_icosahedron;
	bool icosahedron_picked;

	bool is_central_sector;

	float map_screen_pos[3];
};

struct mx_LevelData
{
	mx_LevelSector* sectors;
	unsigned int sector_count;

	mx_LevelVertex* vertices;
	unsigned int vertices_capacity;
	unsigned int vertex_count;

	mx_LevelTriangle* triangles;
	unsigned int triangles_capacity;
	unsigned int triangle_count;

	unsigned int icosahedron_count;
};

struct mx_LevelMesh
{
	mx_LevelVertex* vertices;
	unsigned short* triangles;
	unsigned int vertex_count;
	unsigned int triangle_count;
};

unsigned int mxGenSectorGraphTraverseId();

class mx_LevelGenerator
{
public:
	mx_LevelGenerator( unsigned int seed );
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

		unsigned int linkage_group_id;
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
	void CalculateLinkage();
	static void SetRoomLinkage_r( Room* room );

	static bool CheckConnection( const Room* room0, const Room* room1 );

	void GenerateMeshes();
	void SpitTriangle( unsigned int triangle_index, const mx_Plane& plane );
	void ReserveTrianglesAndVertices( unsigned int new_triangle_count, unsigned int new_vertex_count );
	void AddRoomCube( const Room* room );
	void AddConnectionCube( const Connection* connection );
	void SetupRoomSector( const Room* room, mx_LevelSector* sector );
	void SetupConnectionSector( const Connection* connection, mx_LevelSector* sector );
	
	void CalculateNormals();
	void CalculateTextureCoordinates();
	void SetupTextures();

private:
	Element* element_map_[ MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS ];

	Room rooms_[ MX_MAX_ROOMS ];
	unsigned int room_count_;
	unsigned int max_linkage_group_id_;

	Connection connections_[ MX_MAX_CONNECTIONS ];
	unsigned int connection_count_;

	mx_Rand rand_;

	mx_LevelData out_level_data_;
};