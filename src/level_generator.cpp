#include <algorithm>
#include <cstdlib>

#include "mx_assert.h"
#include "textures_generation.h"

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

static const int c_cube_normals[6][3]=
{
	{ -1,  0,  0 },
	{ +1,  0,  0 },
	{  0, -1,  0 },
	{  0, +1,  0 },
	{  0,  0, -1 },
	{  0,  0, +1 },
};

unsigned int mxGenSectorGraphTraverseId()
{
	static unsigned int id= 0;
	id++;
	return id;
}

static bool IsPointOutsideMap( int* coord )
{
	return
	coord[0] < 0 || coord[0] >= MX_MAX_LEVEL_SIZE_CELLS ||
	coord[1] < 0 || coord[1] >= MX_MAX_LEVEL_SIZE_CELLS ||
	coord[2] < 0 || coord[2] >= MX_MAX_LEVEL_SIZE_CELLS;;
}

static void GenQuadIndexation( mx_LevelTriangle* quad_triangles, unsigned int base_vertex )
{
	quad_triangles[0].vertex_index[0]= (base_vertex    );
	quad_triangles[0].vertex_index[1]= (base_vertex + 1);
	quad_triangles[0].vertex_index[2]= (base_vertex + 2);

	quad_triangles[1].vertex_index[0]= (base_vertex    );
	quad_triangles[1].vertex_index[1]= (base_vertex + 2);
	quad_triangles[1].vertex_index[2]= (base_vertex + 3);
}

static const void SwapVertices( float* v0, float* v1 )
{
	for( unsigned int i= 0; i < 3; i++ )
	{
			float tmp= v0[i];
			v0[i]= v1[i];
			v1[i]= tmp;
	}
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
	CalculateLinkage();

	GenerateMeshes();
	CalculateNormals();
	CalculateTextureCoordinates();
	SetupTextures();
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
				if( TryPlaceConnection( room, coord, c_cube_normals[0] ) ) goto try_next;
				coord[0]= room->coord_max[0];
				if( TryPlaceConnection( room, coord, c_cube_normals[1] ) ) goto try_next;
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
				if( TryPlaceConnection( room, coord, c_cube_normals[2] ) ) goto try_next;
				coord[1]= room->coord_max[1];
				if( TryPlaceConnection( room, coord, c_cube_normals[3] ) ) goto try_next;
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
				if( TryPlaceConnection( room, coord, c_cube_normals[4] ) ) goto try_next;
				coord[2]= room->coord_max[2];
				if( TryPlaceConnection( room, coord, c_cube_normals[5] ) ) goto try_next;
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
				ElementMap( connection->coord_end[0], connection->coord_end[1], connection->coord_end[2] )= connection;
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

void mx_LevelGenerator::CalculateLinkage()
{
	// Initial state - all rooms unlinked
	for( unsigned int i= 0; i < room_count_; i++ )
		rooms_[i].linkage_group_id= 0;

	unsigned int linkage_id= 1;

	for( unsigned int i= 0; i < room_count_; i++ )
	{
		Room* room= rooms_ + i;

		if( room->linkage_group_id == 0 )
		{
			room->linkage_group_id= linkage_id;
			SetRoomLinkage_r( room );

			linkage_id++;
		}
	}

	unsigned int* linkage_groups= new unsigned int[ linkage_id ];
	for( unsigned int i= 0; i < linkage_id; i++ )
		linkage_groups[i]= 0;

	for( unsigned int i= 0; i < room_count_; i++ )
		linkage_groups[ rooms_[i].linkage_group_id ] ++;

	MX_ASSERT( linkage_groups[0] == 0 );

	max_linkage_group_id_= 0;
	for( unsigned int i= 0; i < linkage_id; i++ )
	{
		if( linkage_groups[i] > linkage_groups[max_linkage_group_id_] )
			max_linkage_group_id_= i;
	}

	delete[] linkage_groups;
}

void mx_LevelGenerator::SetRoomLinkage_r( Room* room )
{
	for( unsigned int i= 0; i < room->connection_count; i++ )
	{
		Connection* connection= room->connections[i];
		Room* next_room= connection->begin == room ? connection->end : connection->begin;

		if( next_room->linkage_group_id == 0 )
		{
			next_room->linkage_group_id= room->linkage_group_id;
			SetRoomLinkage_r( next_room );
		}
		else
		{
			MX_ASSERT( next_room->linkage_group_id == room->linkage_group_id );
		}
	}
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

	out_level_data_.sector_count= 0;
	out_level_data_.sectors= new mx_LevelSector[ room_count_ + connection_count_ ];

	unsigned int* room_to_sector_index= new unsigned int[ room_count_ ];
	unsigned int* connection_to_sector_index= new unsigned int[ connection_count_ ];

	mx_LevelSector* sector= out_level_data_.sectors;
	for( unsigned int i= 0; i < room_count_; i++ )
	{
		// Discard unlinked level parts
		if( rooms_[i].linkage_group_id == max_linkage_group_id_ )
		{
			SetupRoomSector( rooms_ + i, sector );

			sector->type= mx_LevelSector::ROOM;
			sector->first_triangle= out_level_data_.triangle_count;
			AddRoomCube( rooms_ + i );
			sector->triangles_count= out_level_data_.triangle_count - sector->first_triangle;

			room_to_sector_index[i]= out_level_data_.sector_count;

			out_level_data_.sector_count++;
			sector++;
		}
	}

	for( unsigned int i= 0; i < connection_count_; i++ )
	{
		// Discard unlinked level parts
		if( connections_[i].begin->linkage_group_id == max_linkage_group_id_ )
		{
			SetupConnectionSector( connections_ + i, sector );

			sector->type= mx_LevelSector::CONNECTION;
			sector->first_triangle= out_level_data_.triangle_count;
			AddConnectionCube( connections_ + i );
			sector->triangles_count= out_level_data_.triangle_count - sector->first_triangle;

			connection_to_sector_index[i]= out_level_data_.sector_count;

			out_level_data_.sector_count++;
			sector++;
		}
	}

	// Setup sectors linkage

	for( unsigned int i= 0; i < room_count_; i++ )
	{
		const Room* room= rooms_ + i;
		// Discard unlinked level parts
		if( room->linkage_group_id == max_linkage_group_id_ )
		{
			sector= out_level_data_.sectors + room_to_sector_index[i];
			sector->connections_count= room->connection_count;
			for( unsigned int j= 0; j < room->connection_count; j++ )
				sector->connections[j]= out_level_data_.sectors + connection_to_sector_index[ room->connections[j] - connections_ ];
		}
	}

	for( unsigned int i= 0; i < connection_count_; i++ )
	{
		const Connection* connection= connections_ + i;
		// Discard unlinked level parts
		if( connection->begin->linkage_group_id == max_linkage_group_id_ )
		{
			sector= out_level_data_.sectors + connection_to_sector_index[i];
			sector->connections_count= 2;
			sector->connections[0]= out_level_data_.sectors + room_to_sector_index[ connection->begin - rooms_ ];
			sector->connections[1]= out_level_data_.sectors + room_to_sector_index[ connection->end   - rooms_ ];
		}
	}

	for( unsigned int s= 0; s < out_level_data_.sector_count; s++ )
		out_level_data_.sectors[s].traverse_id= 0;

	delete[] room_to_sector_index;
	delete[] connection_to_sector_index;
}

void mx_LevelGenerator::SpitTriangle( unsigned int triangle_index, const mx_Plane& plane )
{
#define COMPARE(x) ((x) < 0.0f)
	unsigned int discarded_vertex_count= 0;

	mx_LevelTriangle& triangle= out_level_data_.triangles [ triangle_index ];

	float dist[3];
	for( unsigned int i= 0; i < 3; i++ )
	{
		dist[i]= mxVec3Dot( out_level_data_.vertices[ triangle.vertex_index[i] ].xyz, plane.normal ) - plane.dist;
		if( COMPARE(dist[i]) )
			discarded_vertex_count++;
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

	unsigned int first_discarded_vertex_index;
	for( unsigned int i= 0; i < 3; i++ )
		if( COMPARE(dist[i]) && !COMPARE( dist[ (i + (3 - 1)) % 3 ] ) )
			first_discarded_vertex_index= i;
#undef COMPARE

	float new_vertices[2][3];
	for( unsigned int i= 0; i < 2; i++ )
	{
		unsigned int ind0, ind1;
		if( discarded_vertex_count == 2 )
		{
			ind0= (first_discarded_vertex_index + i) % 3;
			ind1= (first_discarded_vertex_index + 2) % 3;
		}
		else
		{
			ind0= first_discarded_vertex_index;
			ind1= (first_discarded_vertex_index + 2 - i) % 3;
		}
		
		float dist_sum= std::fabsf(dist[ ind0 ]) + std::fabsf(dist[ ind1 ]);
		float k0= std::fabsf(dist[ ind0 ]) / dist_sum;
		float k1= 1.0f - k0;

		for( unsigned int j= 0; j < 3; j++ )
			new_vertices[i][j]=
				out_level_data_.vertices[ triangle.vertex_index[ind0] ].xyz[j] * k1 +
				out_level_data_.vertices[ triangle.vertex_index[ind1] ].xyz[j] * k0;
	}

	ReserveTrianglesAndVertices( 0, 2 );
	out_level_data_.vertex_count+= 2;

	VEC3_CPY(
		out_level_data_.vertices[ out_level_data_.vertex_count - 2 ].xyz,
		new_vertices[0] );
	VEC3_CPY(
		out_level_data_.vertices[ out_level_data_.vertex_count - 1 ].xyz,
		new_vertices[1] );

	if( discarded_vertex_count == 2 ) // 1 triangle left
	{
		// Replace old vertices and place new
		triangle.vertex_index[ first_discarded_vertex_index ]= out_level_data_.vertex_count - 2;
		triangle.vertex_index[ ( first_discarded_vertex_index + 1 ) % 3 ]= out_level_data_.vertex_count - 1;
		return;
	}
	//else if( discarded_vertex_count == 1 ) // make 2 triangles
	{
		ReserveTrianglesAndVertices( 1, 0 );
		out_level_data_.triangle_count++;
		// Update reference after buffer reallocation
		mx_LevelTriangle& triangle2= out_level_data_.triangles[ triangle_index ];

		unsigned int ind0= triangle2.vertex_index[ (first_discarded_vertex_index + 2 ) % 3 ];
		unsigned int ind1= triangle2.vertex_index[ (first_discarded_vertex_index + 1 ) % 3 ];

		triangle2.vertex_index[0]= ind0;
		triangle2.vertex_index[1]= out_level_data_.vertex_count - 2;
		triangle2.vertex_index[2]= out_level_data_.vertex_count - 1;

		mx_LevelTriangle& triangle3= out_level_data_.triangles[ out_level_data_.triangle_count - 1 ];

		triangle3.vertex_index[0]= ind0;
		triangle3.vertex_index[1]= out_level_data_.vertex_count - 1;
		triangle3.vertex_index[2]= ind1;

		return;
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
		out_level_data_.vertices_capacity= total_vertex_count;
	}
}

void mx_LevelGenerator::AddRoomCube( const Room* room )
{
	for( int z= room->coord_min[2] - 1; z < room->coord_max[2]; z++ )
	for( int y= room->coord_min[1] - 1; y < room->coord_max[1]; y++ )
	for( int x= room->coord_min[0] - 1; x < room->coord_max[0]; x++ )
	{
		Element* element= ElementMap( x, y, z );
		Element* el_x= ElementMap( x + 1, y, z );
		Element* el_y= ElementMap( x, y + 1, z );
		Element* el_z= ElementMap( x, y, z + 1 );

		bool is_element= element == NULL;
		bool is_el_x= el_x == NULL;
		bool is_el_y= el_y == NULL;
		bool is_el_z= el_z == NULL;

		if( (is_element ^ is_el_x) && (element == room || el_x == room ) )
		{
			ReserveTrianglesAndVertices( 2, 4 );
			mx_LevelVertex* v= out_level_data_.vertices + out_level_data_.vertex_count;

			v[0].xyz[0]= float(x+1);
			v[0].xyz[1]= float(y);
			v[0].xyz[2]= float(z);
			v[1].xyz[0]= float(x+1);
			v[1].xyz[1]= float(y+1);
			v[1].xyz[2]= float(z);
			v[2].xyz[0]= float(x+1);
			v[2].xyz[1]= float(y+1);
			v[2].xyz[2]= float(z+1);
			v[3].xyz[0]= float(x+1);
			v[3].xyz[1]= float(y);
			v[3].xyz[2]= float(z+1);

			if( is_element ) SwapVertices( v[1].xyz, v[3].xyz );

			GenQuadIndexation( out_level_data_.triangles + out_level_data_.triangle_count, out_level_data_.vertex_count );
			out_level_data_.vertex_count+= 4;
			out_level_data_.triangle_count+= 2;
		}
		if( (is_element ^ is_el_y) && (element == room || el_y == room ) )
		{
			ReserveTrianglesAndVertices( 2, 4 );
			mx_LevelVertex* v= out_level_data_.vertices + out_level_data_.vertex_count;

			v[0].xyz[0]= float(x);
			v[0].xyz[1]= float(y+1);
			v[0].xyz[2]= float(z);
			v[1].xyz[0]= float(x);
			v[1].xyz[1]= float(y+1);
			v[1].xyz[2]= float(z+1);
			v[2].xyz[0]= float(x+1);
			v[2].xyz[1]= float(y+1);
			v[2].xyz[2]= float(z+1);
			v[3].xyz[0]= float(x+1);
			v[3].xyz[1]= float(y+1);
			v[3].xyz[2]= float(z);

			if( is_element ) SwapVertices( v[1].xyz, v[3].xyz );

			GenQuadIndexation( out_level_data_.triangles + out_level_data_.triangle_count, out_level_data_.vertex_count );
			out_level_data_.vertex_count+= 4;
			out_level_data_.triangle_count+= 2;
		}
		if( (is_element ^ is_el_z) && (element == room || el_z == room ) )
		{
			ReserveTrianglesAndVertices( 2, 4 );
			mx_LevelVertex* v= out_level_data_.vertices + out_level_data_.vertex_count;

			v[0].xyz[0]= float(x);
			v[0].xyz[1]= float(y);
			v[0].xyz[2]= float(z+1);
			v[1].xyz[0]= float(x+1);
			v[1].xyz[1]= float(y);
			v[1].xyz[2]= float(z+1);
			v[2].xyz[0]= float(x+1);
			v[2].xyz[1]= float(y+1);
			v[2].xyz[2]= float(z+1);
			v[3].xyz[0]= float(x);
			v[3].xyz[1]= float(y+1);
			v[3].xyz[2]= float(z+1);

			if( is_element ) SwapVertices( v[1].xyz, v[3].xyz );

			GenQuadIndexation( out_level_data_.triangles + out_level_data_.triangle_count, out_level_data_.vertex_count );
			out_level_data_.vertex_count+= 4;
			out_level_data_.triangle_count+= 2;
		}
	} // or xyz

}

void mx_LevelGenerator::AddConnectionCube( const Connection* connection )
{
	const unsigned int c_side_count= 4;

	ReserveTrianglesAndVertices( c_side_count * 2, c_side_count * 4 );
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

void mx_LevelGenerator::SetupRoomSector( const Room* room, mx_LevelSector* sector )
{
	sector->planes_count= 6;
	for( unsigned int i= 0; i < 6; i++ )
	{
		for( unsigned int j= 0;j < 3; j++ )
			sector->planes[i].normal[j]= float(c_cube_normals[i][j]);

		unsigned int c= i >> 1;
		sector->planes[i].dist= sector->planes[i].normal[c] * float((i&1) ? room->coord_min[c] : room->coord_max[c]);
	}

	for( unsigned int i= 0; i < 3; i++ )
	{
		sector->bb_min[i]= float(room->coord_min[i]);
		sector->bb_max[i]= float(room->coord_max[i]);
	}

	sector->light_count= rand_.RandI( 1, 4 );
	for( unsigned int l= 0; l < sector->light_count; l++ )
		for( unsigned int i= 0; i < 3; i++ )
		{
			sector->lights[l].pos[i]= rand_.RandF( sector->bb_min[i] + 1.0f, sector->bb_max[i] - 1.0f );
			sector->lights[l].light_rgb[i]= rand_.RandF( 0.5f, 2.0f );
		}
}

void mx_LevelGenerator::SetupConnectionSector( const Connection* connection, mx_LevelSector* sector )
{
	for( unsigned int i= 0; i < 3; i++ )
	{
		if( connection->coord_begin[i] > connection->coord_end[i] )
		{
			sector->bb_min[i]= float(connection->coord_end[i]);
			sector->bb_max[i]= float(connection->coord_begin[i]+1);
		}
		else
		{
			sector->bb_min[i]= float(connection->coord_begin[i]);
			sector->bb_max[i]= float(connection->coord_end[i]+1);
		}
	}

	sector->planes_count= 6;
	for( unsigned int i= 0; i < 6; i++ )
	{
		for( unsigned int j= 0; j < 3; j++ )
			sector->planes[i].normal[j]= float(c_cube_normals[i][j]);

		unsigned int c= i >> 1;
		sector->planes[i].dist= sector->planes[i].normal[c] * float((i&1) ? sector->bb_min[c] : sector->bb_max[c]);
	}

	// Place lights

	sector->light_count= 0;

	// Select direction of ligts chain
	unsigned int connection_axis= 0;
	for( unsigned int i= 1; i < 3; i++ )
		if( connection->coord_begin[i] != connection->coord_end[i] )
			connection_axis= i;

	// Calculate lights count and step in chain
	float connection_length= sector->bb_max[connection_axis] - sector->bb_min[connection_axis];
	unsigned int light_count= (unsigned int) mxRound( connection_length / 1.5f );
	if( light_count > MX_MAX_SECTOR_LIGHTS ) light_count= MX_MAX_SECTOR_LIGHTS;
	float step= connection_length / float( light_count );

	float coord[3];
	for( unsigned int i= 0; i < 3; i++ )
		coord[i]= sector->bb_min[i] + 0.5f;
	coord[connection_axis]= sector->bb_min[connection_axis] + 0.5f * step;

	while( coord[connection_axis] < sector->bb_max[connection_axis] )
	{
		mx_Light& l= sector->lights[sector->light_count];
		sector->light_count++;
		
		static const float c_light[3]= { 0.125f, 0.12f, 0.1f };
		for( unsigned int i= 0; i < 3; i++ )
		{
			l.pos[i]= coord[i];
			l.light_rgb[i]= c_light[i];
		}
		coord[connection_axis]+= step;
	}

	MX_ASSERT( sector->light_count == light_count );
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
		mxVec3Cross( v[1], v[0], normal );
		mxVec3Normalize( normal );
		for( unsigned int j= 0; j < 3; j++ )
			for( unsigned int k= 0; k < 3; k++ )
				vertices[ triangle->vertex_index[j] ].normal[k]= (char)( normal[k] * 126.9f );
	}
}

void mx_LevelGenerator::CalculateTextureCoordinates()
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

			vertex->binormal[0]= 0;
			vertex->binormal[1]= 127;
			vertex->binormal[2]= 0;
			vertex->tangent[0]= 0;
			vertex->tangent[1]= 0;
			vertex->tangent[2]= 127;
		}
		else if( abs_normal[1] >= abs_normal[2] )
		{
			vertex->tex_coord[0]= vertex->xyz[0];
			vertex->tex_coord[1]= vertex->xyz[2];

			vertex->binormal[0]= 127;
			vertex->binormal[1]= 0;
			vertex->binormal[2]= 0;
			vertex->tangent[0]= 0;
			vertex->tangent[1]= 0;
			vertex->tangent[2]= 127;
		}
		else
		{
			vertex->tex_coord[0]= vertex->xyz[0];
			vertex->tex_coord[1]= vertex->xyz[1];

			vertex->binormal[0]= 127;
			vertex->binormal[1]= 0;
			vertex->binormal[2]= 0;
			vertex->tangent[0]= 0;
			vertex->tangent[1]= 127;
			vertex->tangent[2]= 0;
		}
		vertex->tex_coord[0] *= 0.5f;
		vertex->tex_coord[1] *= 0.5f;
	} // for vertices
}

void mx_LevelGenerator::SetupTextures()
{
	for( unsigned int s= 0; s < out_level_data_.sector_count; s++ )
	{
		unsigned char tex_id= rand_.Rand() % LastLevelTexture;

		mx_LevelSector& sector= out_level_data_.sectors[s];
		
		for( unsigned int t= sector.first_triangle; t < sector.first_triangle + sector.triangles_count; t++ )
		{
			mx_LevelTriangle& triangle= out_level_data_.triangles[t];
			for( unsigned int v= 0; v < 3; v++ )
				out_level_data_.vertices[ triangle.vertex_index[v] ].tex_id= tex_id;
		}
	}
}