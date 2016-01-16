#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main( unsigned int argc, const char* argv[] )
{
	const char* src_file_name;
	const char* dst_file_name;

	unsigned char* bin_data;
	bool is_output_name= false;
	unsigned int text_mode= 0;

	for( unsigned int i= 1; i< argc; i++ )
	{
		if( !strcmp( argv[i], "-o" ) )
		{
			if( (int)i < argc - 1 )
			{
				dst_file_name= argv[++i];
				is_output_name= true;
			}
			else
			{
				printf( "error, missing output file name before -o\n" );
				return 1;
			}
		}
		else if( !strcmp( argv[i], "-t" ) )
			text_mode= 1;
		else
			src_file_name= argv[i];
	}
	if( argc == 1 )
	{
		printf( "error, no input file\n" );
		return 2;
	}

	if( ! is_output_name )
	{
		printf( "error, no output file name\n" );
		return 3;
	}

	FILE* f_src=fopen( src_file_name, "rb" );
	if( f_src == NULL )
	{
		printf( "file %s not found\n", src_file_name );
		return 4;
	}

	FILE* f_dst= fopen( dst_file_name, "wt" );
	if( f_dst == NULL )
	{
		printf( "can not open outpit file %s\n", dst_file_name );
		return 5;
	}

	fseek( f_src, 0, SEEK_END );
	unsigned int src_file_len= ftell( f_src );
	fseek( f_src, 0, SEEK_SET );

	bin_data= (unsigned char*) malloc( src_file_len );

	fread( bin_data, 1, src_file_len, f_src );
	fclose( f_src );

	fprintf( f_dst, "/*data from file \"%s\". Generated automatically*/\n", src_file_name );
	if( text_mode )
	{
		fprintf( f_dst, "\"" );
		for( unsigned int i= 0; i< src_file_len; i++ )
		{
			if( bin_data[i] >= 0x20 && bin_data[i] <= 0x7E )
				fprintf( f_dst, "%c", bin_data[i] );
			else if( bin_data[i] == '\n' )
				fprintf( f_dst, "\\n\"\n\"" );
			else if( bin_data[i] == '\t' )
				fprintf( f_dst, "\\t" );// tab
			else if( bin_data[i] == '"' )
				fprintf( f_dst, "\\\"" );// "
			else if( bin_data[i] == '\\' )
				fprintf( f_dst, "\\" );
			else if( bin_data[i] != 0x0D )//if not mustdie and-of-line symbol
				fprintf( f_dst, "\\x%X", bin_data[i] );
		}
		fprintf( f_dst, "\";" );
	}
	else
	{
		fprintf( f_dst, "{\n" );
		for( unsigned i= 0; i< src_file_len - 1; i++ )
		{
			fprintf( f_dst, " 0x%02X,", bin_data[i] );
			if( (i & 15) == 15 )
				fprintf( f_dst, "\n" );
			else if( (i &7) == 7 )
				fprintf( f_dst, " " );
		}

		fprintf( f_dst, " 0x%02X }", bin_data[ src_file_len - 1 ] );
	}

	fclose( f_dst );
	free(bin_data);

	return 0;
}
