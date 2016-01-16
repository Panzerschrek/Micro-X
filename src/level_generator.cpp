#include <algorithm>
#include <cstdlib>

#include "mx_assert.h"

#include "level_generator.h"

mx_LevelGenerator::mx_LevelGenerator()
	: room_count_(0)
{
	for( unsigned int i= 0; i < sizeof(room_map_) / sizeof(Room*); i++ )
		room_map_[i]= NULL;
}

mx_LevelGenerator::~mx_LevelGenerator()
{
}

void mx_LevelGenerator::Generate()
{
	MX_ASSERT( room_count_ == 0 );

	Room* room= rooms_;
	for( unsigned int i= 0; i < MX_MAX_ROOMS * 2; i++ )
	{
		// Create room
		for( unsigned int j= 0; j < 2; j++ )
		{
			room->coord_min[j]= rand_.RandI( 1, MX_MAX_LEVEL_SIZE_CELLS - MX_MAX_ROOM_SIZE - 1 );
			room->coord_max[j]= room->coord_min[j] + rand_.RandI( MX_MIN_ROOM_SIZE, MX_MAX_ROOM_SIZE + 1 );

			MX_ASSERT( room->coord_min[j] >= 1 && room->coord_min[j] < MX_MAX_LEVEL_SIZE_CELLS - 1 );
			MX_ASSERT( room->coord_max[j] >= 1 && room->coord_max[j] < MX_MAX_LEVEL_SIZE_CELLS - 1 );
		}

		// Check free space
		for( int x= std::max( 0, room->coord_min[0] - MX_MIN_ROOM_DISTANCE );
			x < std::min( MX_MAX_LEVEL_SIZE_CELLS, room->coord_max[0] + MX_MIN_ROOM_DISTANCE );
			x++ )
		for( int y= std::max( 0, room->coord_min[1] - MX_MIN_ROOM_DISTANCE );
			y < std::min( MX_MAX_LEVEL_SIZE_CELLS, room->coord_max[1] + MX_MIN_ROOM_DISTANCE );
			y++ )
		if( room_map_[ x + y * MX_MAX_LEVEL_SIZE_CELLS ] != NULL )
			goto failed;

		// Mark space for this room
		for( int x= room->coord_min[0]; x < room->coord_max[0]; x++ )
		for( int y= room->coord_min[1]; y < room->coord_max[1]; y++ )
			room_map_[ x + y * MX_MAX_LEVEL_SIZE_CELLS ]= room;

		room++;
		room_count_++;
		if( room_count_ == MX_MAX_ROOMS ) break;
	failed:;
	}
}

mx_LevelMesh mx_LevelGenerator::GenerateLevelMesh() const
{
	mx_LevelMesh mesh;
	mesh.vertex_count= room_count_ * 4;
	mesh.triangle_count= room_count_ * 2;
	mesh.vertices= new mx_LevelVertex[ room_count_ * 4 ];
	mesh.triangles= new unsigned short[ room_count_ * 2 * 3 ];

	const Room* room= rooms_;
	mx_LevelVertex* v= mesh.vertices;
	unsigned short* t= mesh.triangles;
	for( unsigned int i= 0; i < room_count_; i++, room++, v+=4, t+= 6 )
	{
		v[0].xyz[0]= float(room->coord_min[0]);
		v[0].xyz[1]= float(room->coord_min[1]);
		v[0].xyz[2]= 0.0f;

		v[1].xyz[0]= float(room->coord_min[0]);
		v[1].xyz[1]= float(room->coord_max[1]);
		v[1].xyz[2]= 0.0f;

		v[2].xyz[0]= float(room->coord_max[0]);
		v[2].xyz[1]= float(room->coord_max[1]);
		v[2].xyz[2]= 0.0f;

		v[3].xyz[0]= float(room->coord_max[0]);
		v[3].xyz[1]= float(room->coord_min[1]);
		v[3].xyz[2]= 0.0f;

		t[0]= (unsigned short)(4*i);
		t[1]= (unsigned short)(4*i + 1);
		t[2]= (unsigned short)(4*i + 2);

		t[3]= (unsigned short)(4*i);
		t[4]= (unsigned short)(4*i + 2);
		t[5]= (unsigned short)(4*i + 3);
	}

	return mesh;
}