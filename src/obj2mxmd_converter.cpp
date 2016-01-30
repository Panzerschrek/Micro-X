#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <vector>

#include "mx_model.h"

struct obj_Vertex
{
	float xyz[3];
};

struct obj_TexCoord
{
	float st[2];
};

typedef obj_Vertex obj_Normal;

struct obj_CombinedVertex
{
	// vertex, vertex tex coord, normal
	// if zero - no component
	unsigned int v, vtc, n;
	unsigned int duplicated_vertex; // index of duplicatd vertex in vector of combined vertices, or index to himself
};

struct obj_Face
{
	unsigned int first_vertex; // first vertex in array of combined vertices
	unsigned int vertex_count;
};

std::vector<obj_Vertex> in_vertices;
std::vector<obj_TexCoord> in_tex_coords;
std::vector<obj_Normal> in_normals;
std::vector<obj_CombinedVertex> in_combined_vertices;
std::vector<obj_Face> in_faces;

std::vector<char> file_data;

std::vector<mx_ModelVertex> out_vertices;
std::vector<mx_ModelTexCoord> out_tex_coords;
std::vector<mx_ModelNormal> out_normals;
std::vector<unsigned char> out_indeces_byte;
std::vector<unsigned short> out_indeces_short;
mx_ModelHeader out_model_header;

bool save_normals= true;
bool save_tex_coords= true;


int mxRound( float x )
{
	float intpart, fractpart;
	fractpart= std::modf( x, &intpart );
	if ( x >= 0.0f )
	{
		if( fractpart < 0.5f )
			return int(intpart);
		else
			return int(intpart) + 1;
	}
	else
	{
		if( fractpart < -0.5f )
			return int(intpart) - 1;
		else
			return int(intpart);
	}
}

void LoadFile( const char* file_name );
void ParseOBJ();
bool IsCharLexemSeparator( char c );
const char* ParseVector( const char* text, float* out_vector, unsigned int vector_size );
const char* ParseNumber( const char* text, float* out_number );

void MarkDuplicatedVertices();
void CalculateBoundingBox();
void NormalizeNormals();

void LoadFile( const char* file_name )
{
	std::FILE* f= std::fopen( file_name, "rb" );
	if( f == NULL )
		return;

	std::fseek( f, 0, SEEK_END );
	unsigned int file_size= std::ftell(f);
	std::fseek( f, 0, SEEK_SET );

	file_data.resize( file_size );
	std::fread( &file_data.front(), file_size, 1, f );
	std::fclose(f);
}

void ParseOBJ()
{
	const char* text= &file_data.front();
	const char* text_end= &file_data.back();

	char lexem[ 128 ];

	while( text <= text_end )
	{
		while(IsCharLexemSeparator(*text)) text++;
		std::sscanf( text, "%s", lexem );
		while(!IsCharLexemSeparator(*text)) text++;

		if( std::strcmp( lexem, "#" ) == 0 ) // commnet
		{
			while( *text != '\n' ) text++;
			text++;
		}
		else if( std::strcmp( lexem, "v" ) == 0 ) // vertex
		{
			obj_Vertex v;
			text= ParseVector( text, v.xyz, 3 );
			in_vertices.push_back(v);
		}
		else if( std::strcmp( lexem, "vt" ) == 0 ) // tex coord
		{
			obj_TexCoord tc;
			text= ParseVector( text, tc.st, 2 );
			in_tex_coords.push_back(tc);
		}
		else if( std::strcmp( lexem, "vn" ) == 0 ) // normal
		{
			obj_Normal n;
			text= ParseVector( text, n.xyz, 3 );
			in_normals.push_back(n);
		}
		else if( std::strcmp( lexem, "f" ) == 0 ) // face
		{
			obj_Face face;
			face.first_vertex= in_combined_vertices.size();
			face.vertex_count= 0;
			while(IsCharLexemSeparator(*text)) text++;
			while( text[0] >= '0' && text[0] <= '9' ) // for face vertices
			{
				obj_CombinedVertex combined_vertex;
				combined_vertex.v= combined_vertex.vtc= combined_vertex.n= 0;

				float num;
				text= ParseNumber( text, &num );
				combined_vertex.v= (unsigned int) num;
				if( *text == '/' )
				{
					// vertex + something
					text++;
					if( *text == '/' )
					{
						text++;
						// vertex + normal
						text= ParseNumber( text, &num );
						combined_vertex.n= (unsigned int) num;
					}
					else
					{
						// vertex + tex_coord
						text= ParseNumber( text, &num );
						combined_vertex.vtc= (unsigned int) num;
						if( *text == '/' )
						{
							text++;
							// vertex + tex_coord + normal
							text= ParseNumber( text, &num );
							combined_vertex.n= (unsigned int) num;
						}
						else
						{
						}
					}
				}
				else
				{
					// only vertex
				}
				in_combined_vertices.push_back(combined_vertex);
				face.vertex_count++;
				while(IsCharLexemSeparator(*text)) text++;
			} // for face vertices
			in_faces.push_back(face);
		} // if face
	} // while not end of file
}

bool IsCharLexemSeparator( char c )
{
	return c == ' ' || c == '\n' || c == 0x09 || c  == '\t';
}

const char* ParseVector( const char* text, float* out_vector, unsigned int vector_size )
{
	for( unsigned int i= 0; i< vector_size; i++ )
	{
		while(IsCharLexemSeparator(*text)) text++;
		out_vector[i]= (float) atof( text );
		while(!IsCharLexemSeparator(*text)) text++;
	}
	return text;
}

const char* ParseNumber( const char* text, float* out_number )
{
	*out_number= (float)atof(text);
	if( *text == '+' || *text == '-' )
		text++;
	while( *text >= '0' && *text <= '9' )
		text++;
	if( *text == '.' )
	{
		text++;
		while( *text >= '0' && *text <= '9' )
		text++;
	}
	return text;
}

void MarkDuplicatedVertices()
{
	unsigned int duplicate_count= 0;

	for( std::vector<obj_CombinedVertex>::iterator v0= in_combined_vertices.begin(); v0< in_combined_vertices.end(); v0++ )
	{
		bool is_duplicate= false;
		for( std::vector<obj_CombinedVertex>::iterator v1= in_combined_vertices.begin(); v1< v0; v1++ )
			if( v0->v == v1->v && v0->vtc == v1->vtc && v0->n == v1->n )
			{
				v0->duplicated_vertex= v1 - in_combined_vertices.begin();
				is_duplicate= true;
				duplicate_count++;
				break;
			}
		
		if( !is_duplicate )
			v0->duplicated_vertex= v0 - in_combined_vertices.begin();
	} // for vertices
}

void CalculateBoundingBox()
{
	const float inf= 1e24f;

	float min[3]= {  inf,  inf,  inf };
	float max[3]= { -inf, -inf, -inf };
	for( std::vector<obj_Vertex>::iterator v= in_vertices.begin(); v< in_vertices.end(); v++ )
		for( unsigned int i= 0; i< 3; i++ )
		{
			if( v->xyz[i] > max[i] ) max[i]= v->xyz[i];
			if( v->xyz[i] < min[i] ) min[i]= v->xyz[i];
		}

	for( unsigned int i= 0; i< 3; i++ )
	{
		out_model_header.scale[i]= ( max[i] - min[i] ) / 255.0f;
		out_model_header.pos[i]= max[i] - 127.0f * out_model_header.scale[i];
	}
}

void NormalizeNormals()
{
	for( std::vector<obj_Normal>::iterator n= in_normals.begin(); n< in_normals.end(); n++ )
	{
		float l= 1.0f / sqrt( n->xyz[0] * n->xyz[0] + n->xyz[1] * n->xyz[1] + n->xyz[2] * n->xyz[2] );
		for( unsigned int i= 0; i< 3; i++ )
			n->xyz[i]= n->xyz[i] * l;
	}
}

void GenOutMesh()
{
	std::vector<unsigned int> old_to_new_vertices( in_combined_vertices.size() );

	for( std::vector<obj_CombinedVertex>::iterator v= in_combined_vertices.begin(); v< in_combined_vertices.end(); v++ )
	{
		if( v->duplicated_vertex == (unsigned int)(v - in_combined_vertices.begin()) )
		{
			mx_ModelVertex out_vertex;
			mx_ModelNormal out_normal;
			mx_ModelTexCoord out_tc;
			old_to_new_vertices[ v - in_combined_vertices.begin() ]= out_vertices.size();

			for( unsigned int i= 0; i< 3; i++ )
			{
				int xyz= mxRound( ( in_vertices[v->v - 1 ].xyz[i] - out_model_header.pos[i] ) / out_model_header.scale[i] );
				if( xyz > 127 ) xyz= 127;
				else if( xyz < -128 ) xyz= -128;
				out_vertex.xyz[i]= (char)xyz;
			}
			if( v->n > 0 )
				for( unsigned int i= 0; i< 3; i++ )
				{
					int xyz= mxRound( in_normals[v->n - 1].xyz[i] * 127.0f );
					if( xyz > 127 ) xyz= 127;
					else if( xyz < -127 ) xyz= -127;
					out_normal.xyz[i]= (char)xyz;
				}
			if( v->vtc > 0 )
				for( unsigned int i= 0; i< 2; i++ )
				{
					int st= mxRound( in_tex_coords[v->vtc - 1].st[i] * 255.0f );
					if( st < 0 ) st= 0;
					else if( st > 255 ) st= 255;
					out_tc.st[i]= (unsigned char)st;
				}
			out_vertices.push_back(out_vertex);
			out_normals.push_back(out_normal);
			out_tex_coords.push_back(out_tc);
		}
		else
		{
			old_to_new_vertices[ v - in_combined_vertices.begin() ]= old_to_new_vertices[ v->duplicated_vertex ];
		} // if duplicated vertex
	}// for combined vertices

	for( std::vector<obj_Face>::iterator face= in_faces.begin(); face < in_faces.end(); face++ )
	{
		for( unsigned int i= 0; i< face->vertex_count - 2; i++ )
		{
			unsigned int v0= old_to_new_vertices[ face->first_vertex         ];
			unsigned int v1= old_to_new_vertices[ face->first_vertex + i + 1 ];
			unsigned int v2= old_to_new_vertices[ face->first_vertex + i + 2 ];
			if( out_vertices.size() <= 255 )
			{
				out_indeces_byte.push_back( (unsigned char)v0 );
				out_indeces_byte.push_back( (unsigned char)v1 );
				out_indeces_byte.push_back( (unsigned char)v2 );
			}
			else
			{
				out_indeces_short.push_back( (unsigned short)v0 );
				out_indeces_short.push_back( (unsigned short)v1 );
				out_indeces_short.push_back( (unsigned short)v2 );
			}
		}
	} // for faces
}

void WriteResultFile( const char* file_name )
{
	bool short_indeces= out_vertices.size() > 255;

	out_model_header.format_code[0]= 'M';
	out_model_header.format_code[1]= 'X';
	out_model_header.format_code[2]= 'M';
	out_model_header.format_code[3]= 'D';
	out_model_header.vertex_count= (unsigned short) out_vertices.size();
	out_model_header.trialgle_count= (unsigned short) ( ( short_indeces ? out_indeces_short.size() : out_indeces_byte.size() ) / 3 );

	out_model_header.bytes_per_index= short_indeces ? 2 : 1;
	out_model_header.vertex_format_flags= MX_MODEL_VERTEX_BIT;
	if( save_normals ) out_model_header.vertex_format_flags|= MX_MODEL_NORMAL_BIT;
	if( save_tex_coords ) out_model_header.vertex_format_flags|= MX_MODEL_TEXCOORD_BIT;

	std::FILE* f= std::fopen( file_name, "wb" );
	if( f == NULL )
	{
		std::printf( "error, can not open file \"%s\" for writing\n", file_name );
		std::exit(-1);
	}

	std::fwrite( &out_model_header, 1, sizeof(out_model_header), f );
	std::fwrite( &out_vertices.front(), 1, sizeof(mx_ModelVertex) * out_vertices.size(), f );

	if( save_normals )
		std::fwrite( &out_normals.front(), 1, sizeof(mx_ModelNormal) * out_normals.size(), f );
	if( save_tex_coords )
		std::fwrite( &out_tex_coords.front(), 1, sizeof(mx_ModelTexCoord) * out_tex_coords.size(), f );

	if( short_indeces )
		std::fwrite( &out_indeces_short.front(), 1, sizeof(unsigned short) * out_indeces_short.size(), f );
	else
		std::fwrite( &out_indeces_byte.front(), 1, sizeof(unsigned char) * out_indeces_byte.size(), f );

	std::fclose(f);
}


static const char* const help_string=
"usage:\n"
"Obj2MDMD_converter [input_file] -o [output_file] [--skip-normals] [--skip-tex-coords]"
"\n";

int main( int argc, char* argv[] )
{
	char* input_file_name= NULL;
	char out_file_name[1024]={0};
	for( int i= 1; i< argc; i++ )
	{
		if( strcmp( argv[i], "-h" ) == 0 )
		{
			printf( help_string );
			return 0;
		}
		else if( strcmp( argv[i], "-o" ) == 0 )
		{
			if( i < argc - 1 )
				strcpy( out_file_name, argv[++i] );
			else
				printf( "warning, missing output file name, after \"-o\"\n" );
		}
		else if( strcmp( argv[i], "--skip-normals" ) == 0 )
			save_normals= false;
		else if( strcmp( argv[i], "--skip-tex-coords" ) == 0 )
			save_tex_coords= false;
		else
			input_file_name= argv[i];
	}

	if( input_file_name == NULL )
	{
		std::printf( "error, no input file specified\n" );
		return -1;
	}

	LoadFile( input_file_name );
	if( file_data.size() == 0 )
	{
		printf( "error, can`t load file \"%s\"\n", input_file_name );
		return -1;
	}

	ParseOBJ();
	MarkDuplicatedVertices();
	CalculateBoundingBox();
	NormalizeNormals();
	GenOutMesh();

	if( out_file_name[0] == 0 )
	{
		int len= strlen( input_file_name );
		int i= len-1;
		while( i>= 0 )
		{
			if( input_file_name[i] == '.' )
				break;
			i--;
		}
		if( i <= 0 )
		{
			strcpy( out_file_name, input_file_name );
			strcat( out_file_name + len, ".mxmd" );
		}
		else
		{
			len= i;
			for( i= 0; i< len; i++ )
				out_file_name[i]= input_file_name[i];
			out_file_name[len+0]= '.';
			out_file_name[len+1]= 'm';
			out_file_name[len+2]= 'x';
			out_file_name[len+3]= 'm';
			out_file_name[len+4]= 'd';
			out_file_name[len+5]= 0;
		}

	}

	WriteResultFile( out_file_name );
	return 0;
}