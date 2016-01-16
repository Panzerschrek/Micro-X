#include <algorithm>
#include <cstdlib>

#include "mx_assert.h"

#include "level_generator.h"

static bool IsPointOutsideMap( int* coord )
{
	return
	coord[0] < 0 || coord[0] >= MX_MAX_LEVEL_SIZE_CELLS ||
	coord[1] < 0 || coord[1] >= MX_MAX_LEVEL_SIZE_CELLS;
}

mx_LevelGenerator::mx_LevelGenerator()
	: room_count_(0)
	, connection_count_(0)
{
	for( unsigned int i= 0; i < MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS; i++ )
		element_map_[i]= NULL;

	for( unsigned int i= 0; i < MX_MAX_ROOMS; i++ )
		rooms_[i].type= Element::ROOM;

	for( unsigned int i= 0; i < MX_MAX_CONNECTIONS; i++ )
		connections_[i].type= Element::CONNECTION;
}

mx_LevelGenerator::~mx_LevelGenerator()
{
}

void mx_LevelGenerator::Generate()
{
	MX_ASSERT( room_count_ == 0 );
	MX_ASSERT( connection_count_ == 0 );

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
		for( int y= std::max( 0, room->coord_min[1] - MX_MIN_ROOM_DISTANCE );
			y < std::min( MX_MAX_LEVEL_SIZE_CELLS, room->coord_max[1] + MX_MIN_ROOM_DISTANCE );
			y++ )
		for( int x= std::max( 0, room->coord_min[0] - MX_MIN_ROOM_DISTANCE );
			x < std::min( MX_MAX_LEVEL_SIZE_CELLS, room->coord_max[0] + MX_MIN_ROOM_DISTANCE );
			x++ )
		if( ElementMap( x, y ) != NULL )
			goto failed;

		// Mark space for this room
		for( int y= room->coord_min[1]; y < room->coord_max[1]; y++ )
		for( int x= room->coord_min[0]; x < room->coord_max[0]; x++ )
			ElementMap( x, y )= room;

		room->connection_count= 0;

		room++;
		room_count_++;
		if( room_count_ == MX_MAX_ROOMS ) break;
	failed:;
	}

	PlaceConnections();
}

mx_LevelMesh mx_LevelGenerator::GenerateLevelMesh() const
{
	mx_LevelMesh mesh;
	mesh.vertex_count= room_count_ * 4 + connection_count_ * 4;
	mesh.triangle_count= room_count_ * 2 + connection_count_ * 2;
	mesh.vertices= new mx_LevelVertex[ mesh.vertex_count ];
	mesh.triangles= new unsigned short[ mesh.triangle_count * 3 ];

	mx_LevelVertex* v= mesh.vertices;
	unsigned short* t= mesh.triangles;

	const Room* room= rooms_;
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

	const Connection* connection= connections_;
	for( unsigned int i= 0; i < connection_count_; i++, connection++, v+=4, t+= 6 )
	{
		//int dx, dy;
		float dx, dy;
		if( connection->coord_begin[0] == connection->coord_end[0] )
		{
			dx= 1 * 0.2f;
			dy= 0;
		}
		else
		{
			MX_ASSERT( connection->coord_begin[1] == connection->coord_end[1] );
			dx= 0;
			dy= 1 * 0.2f;
		}

		v[0].xyz[0]= float(connection->coord_begin[0]) - dx + 0.5f;
		v[0].xyz[1]= float(connection->coord_begin[1]) - dy + 0.5f;
		v[0].xyz[2]= 0.0f;

		v[1].xyz[0]= float(connection->coord_begin[0]) + dx + 0.5f;
		v[1].xyz[1]= float(connection->coord_begin[1]) + dy + 0.5f;
		v[1].xyz[2]= 0.0f;

		v[2].xyz[0]= float(connection->coord_end[0]) + dx + 0.5f;
		v[2].xyz[1]= float(connection->coord_end[1]) + dy + 0.5f;
		v[2].xyz[2]= 0.0f;

		v[3].xyz[0]= float(connection->coord_end[0]) - dx + 0.5f;
		v[3].xyz[1]= float(connection->coord_end[1]) - dy + 0.5f;
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

mx_LevelGenerator::Element*& mx_LevelGenerator::ElementMap( int x, int y )
{
	MX_ASSERT( x >= 0 && x < MX_MAX_LEVEL_SIZE_CELLS );
	MX_ASSERT( y >= 0 && y < MX_MAX_LEVEL_SIZE_CELLS );

	return element_map_[ x + y * MX_MAX_LEVEL_SIZE_CELLS ];
}

void mx_LevelGenerator::PlaceConnections()
{
	static const int normals[4][3]=
	{
		{ +1,  0, 0 },
		{  0, +1, 0 },
		{ -1,  0, 0 },
		{  0, -1, 0 },
	};

	// Try place X or Y connections for random room
	for( unsigned int r=0; r < room_count_ * 256 && connection_count_ < MX_MAX_CONNECTIONS; r++ )
	{
		Room* room= rooms_ + rand_.Rand() % room_count_;
		if( room->connection_count == MX_MAX_ROOM_CONNECTIONS ) continue;

		int coord[2];
		if( rand_.Rand() & 1 )
			for( int x= room->coord_min[0]; x < room->coord_max[0]; x++ )
			{
				coord[0]= x;
				coord[1]= room->coord_min[1] - 1;
				if( TryPlaceConnection( room, coord, normals[3] ) ) goto try_next;
				coord[1]= room->coord_max[1];
				if( TryPlaceConnection( room, coord, normals[1] ) ) goto try_next;
			}
		else
			for( int y= room->coord_min[1]; y < room->coord_max[1]; y++ )
			{
				coord[1]= y;
				coord[0]= room->coord_min[0] - 1;
				if( TryPlaceConnection( room, coord, normals[2] ) ) goto try_next;
				coord[0]= room->coord_max[0];
				if( TryPlaceConnection( room, coord, normals[0] ) ) goto try_next;
			}
	try_next:;
	} // for rooms
}

bool mx_LevelGenerator::TryPlaceConnection( Room* room, const int* begin_coord, const int* direction )
{
	MX_ASSERT( connection_count_ < MX_MAX_CONNECTIONS );
	MX_ASSERT( room->connection_count < MX_MAX_ROOM_CONNECTIONS );

	int coord[2]= { begin_coord[0], begin_coord[1] };

	while( !IsPointOutsideMap(coord) )
	{
		Element* element= ElementMap( coord[0], coord[1] );
		if( element != NULL )
		{
			if( element->type == Element::ROOM )
			{
				Room* end_room= static_cast<Room*>(element);
				if( end_room->connection_count == MX_MAX_ROOM_CONNECTIONS ) return false;

				Connection* connection= connections_ + connection_count_;
				connection_count_++;

				connection->begin= room;
				connection->end= (Room*) element;
				connection->coord_begin[0]= begin_coord[0];
				connection->coord_begin[1]= begin_coord[1];
				connection->coord_end[0]= coord[0] - direction[0];
				connection->coord_end[1]= coord[1] - direction[1];

				room->connections[ room->connection_count++ ]= connection;
				end_room->connections[ end_room->connection_count++ ]= connection;

				coord[0]= connection->coord_begin[0];
				coord[1]= connection->coord_begin[1];
				while( !( coord[0] == connection->coord_end[0] && coord[1] == connection->coord_end[1] ) )
				{
					MX_ASSERT( !IsPointOutsideMap(coord) );
					ElementMap( coord[0], coord[1] )= connection;
					coord[0]+= direction[0];
					coord[1]+= direction[1];
				}
				return true;
			}
			else
			{
				MX_ASSERT( element->type == Element::CONNECTION );
				break;
			}
		}

		int side_cells[2][2];
		if( direction[0] == 0 )
		{
			side_cells[0][0]= coord[0] + 1;
			side_cells[0][1]= coord[1];
			side_cells[1][0]= coord[0] - 1;
			side_cells[1][1]= coord[1];
		}
		else
		{
			side_cells[0][0]= coord[0];
			side_cells[0][1]= coord[1] + 1;
			side_cells[1][0]= coord[0];
			side_cells[1][1]= coord[1] - 1;
		}

		for( unsigned int i= 0; i< 2; i++ )
		{
			if( IsPointOutsideMap( side_cells[i] ) )
				continue;
			if( ElementMap( side_cells[i][0],  side_cells[i][1] ) != NULL )
				return false;
		}

		coord[0]+= direction[0];
		coord[1]+= direction[1];
	}

	return false;
}