#include <algorithm>
#include <cstdlib>

#include "mx_assert.h"

#include "level_generator.h"

static bool IsPointOutsideMap( int* coord )
{
	return
	coord[0] < 0 || coord[0] >= MX_MAX_LEVEL_SIZE_CELLS ||
	coord[1] < 0 || coord[1] >= MX_MAX_LEVEL_SIZE_CELLS ||
	coord[2] < 0 || coord[2] >= MX_MAX_LEVEL_SIZE_CELLS;;
}

mx_LevelGenerator::mx_LevelGenerator()
	: room_count_(0)
	, connection_count_(0)
{
	for( unsigned int i= 0; i < MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS; i++ )
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
		for( unsigned int j= 0; j < 3; j++ )
		{
			room->coord_min[j]= rand_.RandI( 1, MX_MAX_LEVEL_SIZE_CELLS - MX_MAX_ROOM_SIZE - 1 );
			room->coord_max[j]= room->coord_min[j] + rand_.RandI( MX_MIN_ROOM_SIZE, MX_MAX_ROOM_SIZE + 1 );

			MX_ASSERT( room->coord_min[j] >= 1 && room->coord_min[j] < MX_MAX_LEVEL_SIZE_CELLS - 1 );
			MX_ASSERT( room->coord_max[j] >= 1 && room->coord_max[j] < MX_MAX_LEVEL_SIZE_CELLS - 1 );
		}

		// Check free space
		for( int z= std::max( 0, room->coord_min[2] - MX_MIN_ROOM_DISTANCE );
			z < std::min( MX_MAX_LEVEL_SIZE_CELLS, room->coord_max[2] + MX_MIN_ROOM_DISTANCE );
			z++ )
		for( int y= std::max( 0, room->coord_min[1] - MX_MIN_ROOM_DISTANCE );
			y < std::min( MX_MAX_LEVEL_SIZE_CELLS, room->coord_max[1] + MX_MIN_ROOM_DISTANCE );
			y++ )
		for( int x= std::max( 0, room->coord_min[0] - MX_MIN_ROOM_DISTANCE );
			x < std::min( MX_MAX_LEVEL_SIZE_CELLS, room->coord_max[0] + MX_MIN_ROOM_DISTANCE );
			x++ )
			if( ElementMap( x, y, z ) != NULL )
				goto failed;

		// Mark space for this room
		for( int z= room->coord_min[2]; z < room->coord_max[2]; z++ )
		for( int y= room->coord_min[1]; y < room->coord_max[1]; y++ )
		for( int x= room->coord_min[0]; x < room->coord_max[0]; x++ )
			ElementMap( x, y, z )= room;

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
	mesh.vertex_count= room_count_ * 4 * 6 + connection_count_ * 4;
	mesh.triangle_count= room_count_ * 2 * 6 + connection_count_ * 2;
	mesh.vertices= new mx_LevelVertex[ mesh.vertex_count ];
	mesh.triangles= new unsigned short[ mesh.triangle_count * 3 ];

	mx_LevelVertex* v= mesh.vertices;
	unsigned short* t= mesh.triangles;

	const Room* room= rooms_;
	for( unsigned int i= 0; i < room_count_; i++, room++, v+=4 * 6, t+= 6 * 6 )
		GenCube( room, v, t, i * 6 * 4 );

	unsigned int base_vertex= v - mesh.vertices;
	const Connection* connection= connections_;
	for( unsigned int i= 0; i < connection_count_; i++, connection++, v+=4, t+= 6 )
	{
		float dx= 0.2f, dy= 0.2f, dz= 0.2f;

		v[0].xyz[0]= float(connection->coord_begin[0]) - dx + 0.5f;
		v[0].xyz[1]= float(connection->coord_begin[1]) - dy + 0.5f;
		v[0].xyz[2]= float(connection->coord_begin[2]) - dz + 0.5f;;

		v[1].xyz[0]= float(connection->coord_begin[0]) + dx + 0.5f;
		v[1].xyz[1]= float(connection->coord_begin[1]) + dy + 0.5f;
		v[1].xyz[2]= float(connection->coord_begin[2]) + dz + 0.5f;

		v[2].xyz[0]= float(connection->coord_end[0]) + dx + 0.5f;
		v[2].xyz[1]= float(connection->coord_end[1]) + dy + 0.5f;
		v[2].xyz[2]= float(connection->coord_end[2]) + dz + 0.5f;

		v[3].xyz[0]= float(connection->coord_end[0]) - dx + 0.5f;
		v[3].xyz[1]= float(connection->coord_end[1]) - dy + 0.5f;
		v[3].xyz[2]= float(connection->coord_end[2]) - dz + 0.5f;

		t[0]= (unsigned short)(base_vertex + 4*i);
		t[1]= (unsigned short)(base_vertex + 4*i + 1);
		t[2]= (unsigned short)(base_vertex + 4*i + 2);

		t[3]= (unsigned short)(base_vertex + 4*i);
		t[4]= (unsigned short)(base_vertex + 4*i + 2);
		t[5]= (unsigned short)(base_vertex + 4*i + 3);
	}

	MX_ASSERT( t == mesh.triangle_count * 3 + mesh.triangles );
	MX_ASSERT( v == mesh.vertices + mesh.vertex_count );

	return mesh;
}

mx_LevelGenerator::Element*& mx_LevelGenerator::ElementMap( int x, int y, int z )
{
	MX_ASSERT( x >= 0 && x < MX_MAX_LEVEL_SIZE_CELLS );
	MX_ASSERT( y >= 0 && y < MX_MAX_LEVEL_SIZE_CELLS );
	MX_ASSERT( z >= 0 && z < MX_MAX_LEVEL_SIZE_CELLS );

	return element_map_[ x + y * MX_MAX_LEVEL_SIZE_CELLS + z * MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS ];
}

void mx_LevelGenerator::PlaceConnections()
{
	static const int normals[6][3]=
	{
		{ -1,  0,  0 },
		{ +1,  0,  0 },
		{  0, -1,  0 },
		{  0, +1,  0 },
		{  0,  0, -1 },
		{  0,  0, +1 },
	};

	// Try place X or Y connections for random room
	for( unsigned int r=0; r < room_count_ * 256 && connection_count_ < MX_MAX_CONNECTIONS; r++ )
	{
		Room* room= rooms_ + rand_.Rand() % room_count_;
		if( room->connection_count == MX_MAX_ROOM_CONNECTIONS ) continue;

		int coord[3];
		int side= rand_.Rand() % 3;
		if( side == 0 )
		{
			for( int y= room->coord_min[1]; y < room->coord_max[1]; y++ )
			for( int z= room->coord_min[2]; z < room->coord_max[2]; z++ )
			{
				coord[1]= y;
				coord[2]= z;

				coord[0]= room->coord_min[0] - 1;
				if( TryPlaceConnection( room, coord, normals[0] ) ) goto try_next;
				coord[0]= room->coord_max[0];
				if( TryPlaceConnection( room, coord, normals[1] ) ) goto try_next;
			}
		}
		else if( side == 1 )
		{
			for( int x= room->coord_min[0]; x < room->coord_max[0]; x++ )
			for( int z= room->coord_min[2]; z < room->coord_max[2]; z++ )
			{
				coord[0]= x;
				coord[2]= z;

				coord[1]= room->coord_min[1] - 1;
				if( TryPlaceConnection( room, coord, normals[2] ) ) goto try_next;
				coord[1]= room->coord_max[1];
				if( TryPlaceConnection( room, coord, normals[3] ) ) goto try_next;
			}
		}
		else // if( coord = 2 )
		{
			for( int x= room->coord_min[0]; x < room->coord_max[0]; x++ )
			for( int y= room->coord_min[1]; y < room->coord_max[1]; y++ )
			{
				coord[0]= x;
				coord[1]= y;

				coord[2]= room->coord_min[2] - 1;
				if( TryPlaceConnection( room, coord, normals[4] ) ) goto try_next;
				coord[2]= room->coord_max[2];
				if( TryPlaceConnection( room, coord, normals[5] ) ) goto try_next;
			}
		}

	try_next:;
	} // for rooms
}

bool mx_LevelGenerator::TryPlaceConnection( Room* room, const int* begin_coord, const int* direction )
{
	MX_ASSERT( connection_count_ < MX_MAX_CONNECTIONS );
	MX_ASSERT( room->connection_count < MX_MAX_ROOM_CONNECTIONS );

	int coord[3]= { begin_coord[0], begin_coord[1], begin_coord[2] };

	while( !IsPointOutsideMap(coord) )
	{
		Element* element= ElementMap( coord[0], coord[1], coord[2] );
		if( element != NULL )
		{
			if( element->type == Element::ROOM )
			{
				Room* end_room= static_cast<Room*>(element);
				if( end_room->connection_count == MX_MAX_ROOM_CONNECTIONS ) return false;
				if( CheckConnection( room, end_room ) ) return false;

				Connection* connection= connections_ + connection_count_;
				connection_count_++;

				connection->begin= room;
				connection->end= (Room*) element;
				for( unsigned int i= 0; i < 3; i++ )
				{
					connection->coord_begin[i]= begin_coord[i];
					connection->coord_end[i]= coord[i] - direction[i];
				}

				room->connections[ room->connection_count++ ]= connection;
				end_room->connections[ end_room->connection_count++ ]= connection;

				coord[0]= connection->coord_begin[0];
				coord[1]= connection->coord_begin[1];
				coord[2]= connection->coord_begin[2];
				while( !(
					coord[0] == connection->coord_end[0] &&
					coord[1] == connection->coord_end[1] &&
					coord[2] == connection->coord_end[2] ) )
				{
					MX_ASSERT( !IsPointOutsideMap(coord) );
					ElementMap( coord[0], coord[1], coord[2] )= connection;
					coord[0]+= direction[0];
					coord[1]+= direction[1];
					coord[2]+= direction[2];
				}
				return true;
			}
			else
			{
				MX_ASSERT( element->type == Element::CONNECTION );
				break;
			}
		}

		int side_cells[4][3];
		for( unsigned int i= 0; i < 4; i++ )
		{
			side_cells[i][0]= coord[0];
			side_cells[i][1]= coord[1];
			side_cells[i][2]= coord[2];
		}
		if( std::abs(direction[0]) == 1 )
		{
			side_cells[0][1] -= 1;
			side_cells[1][1] += 1;
			side_cells[2][2] -= 1;
			side_cells[3][2] += 1;
		}
		else if( std::abs(direction[1]) == 1 )
		{
			side_cells[0][0] -= 1;
			side_cells[1][0] += 1;
			side_cells[2][2] -= 1;
			side_cells[3][2] += 1;
		}
		else
		{
			side_cells[0][0] -= 1;
			side_cells[1][0] += 1;
			side_cells[2][1] -= 1;
			side_cells[3][1] += 1;
		}

		for( unsigned int i= 0; i< 4; i++ )
		{
			if( IsPointOutsideMap( side_cells[i] ) )
				continue;
			if( ElementMap( side_cells[i][0], side_cells[i][1], side_cells[i][2] ) != NULL )
				return false;
		}

		coord[0]+= direction[0];
		coord[1]+= direction[1];
		coord[2]+= direction[2];
	}

	return false;
}

bool mx_LevelGenerator::CheckConnection( const Room* room0, const Room* room1 )
{
	for( unsigned int i= 0; i < room0->connection_count; i++ )
	{
		const Connection* connection= room0->connections[i];
		if( connection->begin == room1 || connection->end == room1 ) return true;
	}
	return false;
}

void mx_LevelGenerator::GenCube( const Room* room, mx_LevelVertex* vertices, unsigned short* indeces, unsigned int base_vertex )
{
	static const unsigned char c_cube_vertices[]=
	{
		0, 0, 0,   0, 1, 0,   1, 1, 0,   1, 0, 0, // Z negative
		1, 0, 1,   1, 1, 1,   0, 1, 1,   0, 0, 1, // Z positiove

		1, 0, 0,   1, 1, 0,   1, 1, 1,   1, 0, 1, // X negative
		0, 0, 1,   0, 1, 1,   0, 1, 0,   0, 0, 0, // X positiove

		0, 1, 0,   0, 1, 1,   1, 1, 1,   1, 1, 0, // Y negative
		1, 0, 0,   1, 0, 1,   0, 0, 1,   0, 0, 0, // Y positiove
	};

	float min_max[2][3]; // 0 - min, 1 - max
	for( unsigned int i= 0; i < 3; i++ )
	{
		min_max[0][i]= float(room->coord_min[i]);
		min_max[1][i]= float(room->coord_max[i]);
	}

	mx_LevelVertex* v= vertices;

	for( unsigned int i= 0; i < 4 * 6; i++ , v++ )
	{
		const unsigned char *c= c_cube_vertices + 3 * i;
	
		v->xyz[0]= min_max[ c[0] ][0];
		v->xyz[1]= min_max[ c[1] ][1];
		v->xyz[2]= min_max[ c[2] ][2];
	}

	for( unsigned int i= base_vertex; i < base_vertex + 6 * 4; i+=4, indeces+= 6 )
	{
		indeces[0]= (unsigned short)(i   );
		indeces[1]= (unsigned short)(i + 1);
		indeces[2]= (unsigned short)(i + 2);

		indeces[3]= (unsigned short)(i    );
		indeces[4]= (unsigned short)(i + 2);
		indeces[5]= (unsigned short)(i + 3);
	}
}