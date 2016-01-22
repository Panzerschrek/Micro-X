#include <algorithm>
#include <cstdlib>

#include "mx_assert.h"

#include "level_generator.h"

static const unsigned char c_cube_vertices[]=
{
	1, 0, 0,   1, 1, 0,   1, 1, 1,   1, 0, 1, // X negative
	0, 0, 1,   0, 1, 1,   0, 1, 0,   0, 0, 0, // X positiove

	0, 1, 0,   0, 1, 1,   1, 1, 1,   1, 1, 0, // Y negative
	1, 0, 0,   1, 0, 1,   0, 0, 1,   0, 0, 0, // Y positiove

	0, 0, 0,   0, 1, 0,   1, 1, 0,   1, 0, 0, // Z negative
	1, 0, 1,   1, 1, 1,   0, 1, 1,   0, 0, 1, // Z positiove
};

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


	PlaceRooms();
	PlaceConnections();
	GenerateMeshes();
	CalculateNormals();
	ClaculateTextureCoordinates();
}

const mx_LevelData& mx_LevelGenerator::GetLevelData()
{
	return out_level_data_;
}

mx_LevelGenerator::Element*& mx_LevelGenerator::ElementMap( int x, int y, int z )
{
	MX_ASSERT( x >= 0 && x < MX_MAX_LEVEL_SIZE_CELLS );
	MX_ASSERT( y >= 0 && y < MX_MAX_LEVEL_SIZE_CELLS );
	MX_ASSERT( z >= 0 && z < MX_MAX_LEVEL_SIZE_CELLS );

	return element_map_[ x + y * MX_MAX_LEVEL_SIZE_CELLS + z * MX_MAX_LEVEL_SIZE_CELLS * MX_MAX_LEVEL_SIZE_CELLS ];
}

void mx_LevelGenerator::PlaceRooms()
{
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

void mx_LevelGenerator::GenerateMeshes()
{
	out_level_data_.vertex_count= 0;
	out_level_data_.triangle_count= 0;

	out_level_data_.vertices_capacity= room_count_ * 4 * 6 + connection_count_ * 4 * 6;
	out_level_data_.triangles_capacity= room_count_ * 2 * 6 + connection_count_ * 2 * 6;

	out_level_data_.vertices= new mx_LevelVertex[ out_level_data_.vertices_capacity ];
	out_level_data_.triangles= new mx_LevelTriangle[ out_level_data_.triangles_capacity ];

	for( unsigned int i= 0; i < room_count_; i++ )
		AddRoomCube( rooms_ + i );

	for( unsigned int i= 0; i < connection_count_; i++ )
		AddConnectionCube( connections_ + i );
}

void mx_LevelGenerator::SpitTriangle( unsigned int triangle_index, const mx_Plane& plane )
{
	unsigned int discarded_vertex_count= 0;
	unsigned int first_discarded_vertex_index= 0; // TODO - remove 0

	mx_LevelTriangle& triangle= out_level_data_.triangles [ triangle_index ];

	float dist[3];
	for( unsigned int i= 0; i < 3; i++ )
	{
		dist[i]= mxVec3Dot( out_level_data_.vertices[ triangle.vertex_index[i] ].xyz, plane.normal ) - plane.dist;
		if( dist[i] < 0.0f )
		{
			if( discarded_vertex_count == 0 )
				first_discarded_vertex_index= i;
			discarded_vertex_count++;
		}
	}

	if( discarded_vertex_count == 0 ) // triangle fully passed
		return;
	if( discarded_vertex_count == 3 ) // fully discarded
	{
		if( triangle_index != out_level_data_.triangle_count - 1 )
			out_level_data_.triangles[ triangle_index ]= out_level_data_.triangles[ out_level_data_.triangle_count - 1 ];
		out_level_data_.triangle_count--;
		return;
	}

	float new_vertices[2][3];
	for( unsigned int i= 0; i < 2; i++ )
	{
		unsigned int ind0, ind1;
		if( discarded_vertex_count == 2 )
		{
			if( i == 0 )
			{
				ind0= (first_discarded_vertex_index    ) % 3;
				ind1= (first_discarded_vertex_index + 2) % 3;
			}
			else
			{
				ind0= (first_discarded_vertex_index + 1) % 3;
				ind1= (first_discarded_vertex_index + 2) % 3;
			}
		}
		else
		{
			if( i == 0 )
			{
				ind0= (first_discarded_vertex_index    ) % 3;
				ind1= (first_discarded_vertex_index + 2) % 3;
			}
			else
			{
				ind0= (first_discarded_vertex_index    ) % 3;
				ind1= (first_discarded_vertex_index + 1) % 3;
			}
		}
		
		float dist_sum= std::fabsf(dist[ ind0 ]) + std::fabsf(dist[ ind1 ]);
		float k0= std::fabsf(dist[ ind0 ]) / dist_sum;
		float k1= 1.0f - k0;

		for( unsigned int j= 0; j < 3; j++ )
			new_vertices[i][j]=
				out_level_data_.vertices[ triangle.vertex_index[ind0] ].xyz[j] * k1 +
				out_level_data_.vertices[ triangle.vertex_index[ind1] ].xyz[j] * k0;
	}

	if( discarded_vertex_count == 2 ) // 1 triangle left
	{
		ReserveTrianglesAndVertices( 0, 2 );
		out_level_data_.vertex_count+= 2;

		// Just replace discarded vertices
		VEC3_CPY(
			out_level_data_.vertices[ out_level_data_.vertex_count - 2 ].xyz,
			new_vertices[0] );
		VEC3_CPY(
			out_level_data_.vertices[ out_level_data_.vertex_count - 1 ].xyz,
			new_vertices[1] );

		triangle.vertex_index[ first_discarded_vertex_index ]= out_level_data_.vertex_count - 2;
		triangle.vertex_index[ ( first_discarded_vertex_index + 1 ) % 3 ]= out_level_data_.vertex_count - 1;

		return;
	}
	//else if( discarded_vertex_count == 1 ) // make 2 triangles
	{
	}
}

void mx_LevelGenerator::ReserveTrianglesAndVertices( unsigned int new_triangle_count, unsigned int new_vertex_count )
{
	// TODO - remove duplicated code

	unsigned int total_triangle_count= out_level_data_.triangle_count + new_triangle_count;
	unsigned int total_vertex_count= out_level_data_.vertex_count + new_vertex_count;

	if( total_triangle_count > out_level_data_.triangles_capacity )
	{
		total_triangle_count= total_triangle_count * 5u / 4u + 1u;

		mx_LevelTriangle* new_triangles= new mx_LevelTriangle[ total_triangle_count ];
		std::memcpy( new_triangles, out_level_data_.triangles, sizeof(mx_LevelTriangle) * out_level_data_.triangle_count );
		delete[] out_level_data_.triangles;

		out_level_data_.triangles= new_triangles;
		out_level_data_.triangles_capacity= total_triangle_count;
	}
	if( total_vertex_count > out_level_data_.vertices_capacity )
	{
		total_vertex_count= total_vertex_count * 5u / 4u + 1u;

		mx_LevelVertex* new_vertices= new mx_LevelVertex[ total_vertex_count ];
		std::memcpy( new_vertices, out_level_data_.vertices, sizeof(mx_LevelVertex) * out_level_data_.vertex_count );
		delete[] out_level_data_.vertices;

		out_level_data_.vertices= new_vertices;
		out_level_data_.triangles_capacity= total_vertex_count;
	}
}

void mx_LevelGenerator::AddRoomCube( const Room* room )
{
	MX_ASSERT( out_level_data_.vertex_count + 6 * 4 <= out_level_data_.vertices_capacity );
	MX_ASSERT( out_level_data_.triangle_count + 6 * 2 <= out_level_data_.triangles_capacity );

	float min_max[2][3]; // 0 - min, 1 - max
	for( unsigned int i= 0; i < 3; i++ )
	{
		min_max[0][i]= float(room->coord_min[i]);
		min_max[1][i]= float(room->coord_max[i]);
	}

	mx_LevelVertex* v= out_level_data_.vertices + out_level_data_.vertex_count;
	for( unsigned int i= 0; i < 4 * 6; i++ , v++ )
	{
		const unsigned char *c= c_cube_vertices + 3 * i;
	
		v->xyz[0]= min_max[ c[0] ][0];
		v->xyz[1]= min_max[ c[1] ][1];
		v->xyz[2]= min_max[ c[2] ][2];
	}

	mx_LevelTriangle* triangle= out_level_data_.triangles + out_level_data_.triangle_count;
	for(
		unsigned int i= out_level_data_.vertex_count;
		i < out_level_data_.vertex_count + 6 * 4; i+=4, triangle+=2 )
	{
		triangle[0].vertex_index[0]= (i    );
		triangle[0].vertex_index[1]= (i + 1);
		triangle[0].vertex_index[2]= (i + 2);

		triangle[1].vertex_index[0]= (i    );
		triangle[1].vertex_index[1]= (i + 2);
		triangle[1].vertex_index[2]= (i + 3);
	}

	out_level_data_.vertex_count+= 6 * 4;
	out_level_data_.triangle_count+= 6 * 2;

	mx_Plane plane;
	plane.normal[0]= 0.5f;
	plane.normal[1]= 0.5f;
	plane.normal[2]= 0.0f;
	mxVec3Normalize(plane.normal);
	plane.dist= 20.0f;
	for( unsigned int i= out_level_data_.triangle_count - 6 * 2; i < out_level_data_.triangle_count; )
	{
		unsigned int before= out_level_data_.triangle_count;
		SpitTriangle( i, plane );
		unsigned int after= out_level_data_.triangle_count;
		if( after < before ){}
		else if( after > before ) i+=2;
		else i++;
	}
}

void mx_LevelGenerator::AddConnectionCube( const Connection* connection )
{
	const unsigned int c_side_count= 4;

	MX_ASSERT( out_level_data_.vertex_count + c_side_count * 4 <= out_level_data_.vertices_capacity );
	MX_ASSERT( out_level_data_.triangle_count + c_side_count * 2 < out_level_data_.triangles_capacity );

	float min_max[2][3]; // 0 - min, 1 - max
	for( unsigned int i= 0; i < 3; i++ )
	{
		float b= float(connection->coord_begin[i]);
		float e= float(connection->coord_end[i]);
		if( b > e )
			min_max[0][i]= e, min_max[1][i]= b;
		else
			min_max[0][i]= b,  min_max[1][i]= e;
		min_max[1][i]++;
	}

	unsigned int skip_side;
	if( connection->coord_begin[0] != connection->coord_end[0] )
		skip_side= 0;
	else if( connection->coord_begin[1] != connection->coord_end[1] )
		skip_side= 1;
	else
	{
		skip_side= 2;
		MX_ASSERT( connection->coord_begin[2] != connection->coord_end[2] );
	}

	mx_LevelVertex* v= out_level_data_.vertices + out_level_data_.vertex_count;
	for( unsigned int i= 0; i < 4 * 6; i++ )
	{
		if( i / 8u == skip_side ) continue;
		const unsigned char *c= c_cube_vertices + 3 * i;
	
		v->xyz[0]= min_max[ c[0] ][0];
		v->xyz[1]= min_max[ c[1] ][1];
		v->xyz[2]= min_max[ c[2] ][2];
		v++;
	}

	mx_LevelTriangle* triangle= out_level_data_.triangles + out_level_data_.triangle_count;
	for(
		unsigned int i= out_level_data_.vertex_count;
		i < out_level_data_.vertex_count + c_side_count * 4; i+=4, triangle+=2 )
	{
		triangle[0].vertex_index[0]= (i    );
		triangle[0].vertex_index[1]= (i + 1);
		triangle[0].vertex_index[2]= (i + 2);

		triangle[1].vertex_index[0]= (i    );
		triangle[1].vertex_index[1]= (i + 2);
		triangle[1].vertex_index[2]= (i + 3);
	}

	out_level_data_.vertex_count+= c_side_count * 4;
	out_level_data_.triangle_count+= c_side_count * 2;
}

void mx_LevelGenerator::CalculateNormals()
{
	mx_LevelVertex* vertices= out_level_data_.vertices;

	mx_LevelTriangle* triangle= out_level_data_.triangles;
	for( unsigned int i= 0; i < out_level_data_.triangle_count; i++, triangle++ )
	{
		float v[2][3];
		mxVec3Sub( vertices[ triangle->vertex_index[0] ].xyz, vertices[ triangle->vertex_index[1] ].xyz, v[0] );
		mxVec3Sub( vertices[ triangle->vertex_index[1] ].xyz, vertices[ triangle->vertex_index[2] ].xyz, v[1] );

		float normal[3];
		mxVec3Cross( v[0], v[1], normal );
		mxVec3Normalize( normal );
		for( unsigned int j= 0; j < 3; j++ )
			for( unsigned int k= 0; k < 3; k++ )
				vertices[ triangle->vertex_index[j] ].normal[k]= (char)( normal[k] * 126.9f );
	}
}

void mx_LevelGenerator::ClaculateTextureCoordinates()
{
	mx_LevelVertex* vertex= out_level_data_.vertices;
	for( unsigned int i= 0; i < out_level_data_.vertex_count; i++, vertex++ )
	{
		float abs_normal[3];
		for( unsigned int j= 0; j < 3; j++ ) abs_normal[j]= std::fabsf(vertex->normal[j]);

		if( abs_normal[0] >= abs_normal[1] && abs_normal[0] >= abs_normal[2] )
		{
			vertex->tex_coord[0]= vertex->xyz[1];
			vertex->tex_coord[1]= vertex->xyz[2];
		}
		else if( abs_normal[1] >= abs_normal[2] )
		{
			vertex->tex_coord[0]= vertex->xyz[0];
			vertex->tex_coord[1]= vertex->xyz[2];
		}
		else
		{
			vertex->tex_coord[0]= vertex->xyz[0];
			vertex->tex_coord[1]= vertex->xyz[1];
		}
	} // for vertices
}