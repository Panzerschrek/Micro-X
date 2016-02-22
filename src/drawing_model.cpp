#include <cstring>

#include "mx_math.h"
#include "mx_model.h"

#include "drawing_model.h"

mx_DrawingModel::mx_DrawingModel()
	: vertices_(NULL), indeces_(NULL)
	, vertex_count_(0), index_count_(0)
{
}

mx_DrawingModel::~mx_DrawingModel()
{
	if( vertices_ != NULL )
		delete[] vertices_;
	if( indeces_ != NULL )
		delete[] indeces_;
}

void mx_DrawingModel::SetVertexData( mx_DrawingModelVertex* vertices, unsigned int vertex_count )
{
	if( vertices_ != NULL ) delete[] vertices_;
	vertices_= vertices;
	vertex_count_= vertex_count;
}

void mx_DrawingModel::SetIndexData( unsigned short* indeces, unsigned int index_count )
{
	if( indeces_ != NULL ) delete[] indeces_;
	indeces_= indeces;
	index_count_= index_count;
}

void mx_DrawingModel::Copy( const mx_DrawingModel* m )
{
	if( vertices_ != NULL )
		delete[] vertices_;
	if( indeces_ != NULL )
		delete[] indeces_;

	vertex_count_= m->vertex_count_;
	index_count_= m->index_count_;
	vertices_= new mx_DrawingModelVertex[ vertex_count_ ];
	indeces_= new unsigned short[ index_count_ ];
	std::memcpy( vertices_, m->vertices_, vertex_count_ * sizeof(mx_DrawingModelVertex) );
	std::memcpy( indeces_, m->indeces_, index_count_ * sizeof(unsigned short) );
}

void mx_DrawingModel::LoadFromMFMD( const unsigned char* model_data )
{
	const mx_ModelHeader* header= (const mx_ModelHeader*) model_data;

	mx_ModelVertex* v_p= (mx_ModelVertex*)( model_data + sizeof(mx_ModelHeader) );
	void* next_p= v_p + header->vertex_count;
	mx_ModelNormal* n_p= NULL;
	mx_ModelTexCoord* tc_p= NULL;

	if( header->vertex_format_flags & MX_MODEL_NORMAL_BIT )
	{
		n_p= (mx_ModelNormal*)(next_p);
		next_p= n_p + header->vertex_count;
	}
	if( header->vertex_format_flags & MX_MODEL_TEXCOORD_BIT )
	{
		tc_p= (mx_ModelTexCoord*)(next_p);
		next_p= tc_p + header->vertex_count;
	}
	unsigned short* i_p= (unsigned short*)next_p;

	if( vertices_ != NULL )
		delete[] vertices_;
	vertices_= new mx_DrawingModelVertex[ header->vertex_count ];
	if( indeces_ != NULL )
		delete[] indeces_;
	indeces_= new unsigned short[ header->trialgle_count * 3 ];
	vertex_count_= header->vertex_count;
	index_count_= header->trialgle_count * 3;

	for( unsigned int i= 0; i< vertex_count_; i++ )
	{
		for( unsigned int j= 0; j< 3; j++ )
			vertices_[i].pos[j]= float(v_p[i].xyz[j]) * header->scale[j] + header->pos[j];
		if( n_p != NULL )
		{
			for( unsigned int j= 0; j< 3; j++ )
				vertices_[i].normal[j]= float(n_p[i].xyz[j]) / 127.0f;
		}
		if( tc_p != NULL )
		{
			vertices_[i].tex_coord[0]= float(tc_p[i].st[0]) * (1.0f/255.0f);
			vertices_[i].tex_coord[1]= float(tc_p[i].st[1]) * (1.0f/255.0f);
		}
		else
		{
			vertices_[i].tex_coord[0]= 0.0f;
			vertices_[i].tex_coord[1]= 0.0f;
		}
	} // for out vertices

	if( header->bytes_per_index == 2 )
		std::memcpy( indeces_, i_p, sizeof(unsigned short) * index_count_ );
	else
	{
		unsigned char* byte_p= (unsigned char*)i_p;
		for( unsigned int i= 0; i < index_count_; i++ )
			indeces_[i]= byte_p[i];
	}

	// Make per-triangle normals
	if( n_p == 0 )
	{
		for( unsigned int i= 0; i < index_count_; i+= 3 )
		{
			float vec[2][3];
			mxVec3Sub( vertices_[ indeces_[i  ] ].pos, vertices_[ indeces_[i+1] ].pos, vec[0] );
			mxVec3Sub( vertices_[ indeces_[i+1] ].pos, vertices_[ indeces_[i+2] ].pos, vec[1] );

			float normal[3];
			mxVec3Cross( vec[0], vec[1], normal );
			mxVec3Normalize( normal );
			for( unsigned int j= 0; j < 3; j++ )
			{
				VEC3_CPY( vertices_[ indeces_[i+j] ].normal, normal );
			}
		}
	}
}

void mx_DrawingModel::Add( const mx_DrawingModel* m )
{
	mx_DrawingModelVertex* new_vertices= new mx_DrawingModelVertex[ vertex_count_ + m->vertex_count_ ];
	unsigned short* new_indeces= new unsigned short[ index_count_ + m->index_count_ ];
	
	if( vertices_ != NULL )
		std::memcpy( new_vertices, vertices_, vertex_count_ * sizeof(mx_DrawingModelVertex) );
	memcpy( new_vertices + vertex_count_, m->vertices_, m->vertex_count_ * sizeof(mx_DrawingModelVertex) );

	if( indeces_ != NULL )
		std::memcpy( new_indeces, indeces_, sizeof(unsigned short) * index_count_ );
	for( unsigned int i= 0; i< m->index_count_; i++ )
		new_indeces[ i + index_count_ ]= (unsigned short)( m->indeces_[i] + vertex_count_ );

	if( vertices_ != NULL )
		delete[] vertices_;
	if( indeces_ != NULL )
		delete[] indeces_;
	vertices_= new_vertices;
	indeces_= new_indeces;
	vertex_count_+= m->vertex_count_;
	index_count_+= m->index_count_;
}

void mx_DrawingModel::Scale( const float* scale_vec )
{
	float normal_scale_vec[]= { 1.0f / scale_vec[0], 1.0f / scale_vec[1], 1.0f / scale_vec[2] };

	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		for( unsigned int j= 0; j< 3; j++ )
		{
			vertices_[i].pos[j]*= scale_vec[j];
			vertices_[i].normal[j]*= normal_scale_vec[j];
		}
		mxVec3Normalize( vertices_[i].normal );
	}
}

void mx_DrawingModel::Scale( float s )
{
	float scale_vec[]= { s, s, s };
	Scale( scale_vec );
}

void mx_DrawingModel::Shift( const float* shift_vec )
{
	for( unsigned int i= 0; i < vertex_count_; i++ )
		for( unsigned int j= 0; j< 3; j++ )
			vertices_[i].pos[j]+= shift_vec[j];
}

void mx_DrawingModel::Rotate( const float* normal_mat )
{
	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		float pos[3];
		float normal[3];
		VEC3_CPY( pos, vertices_[i].pos );
		VEC3_CPY( normal, vertices_[i].normal );
		mxVec3Mat3Mul( pos, normal_mat, vertices_[i].pos );
		mxVec3Mat3Mul( normal, normal_mat, vertices_[i].normal );
	}
}

void mx_DrawingModel::ScaleTexCoord( const float* scale_vec )
{
	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		vertices_[i].tex_coord[0]*= scale_vec[0];
		vertices_[i].tex_coord[1]*= scale_vec[1];
	}
}

void mx_DrawingModel::ShiftTexCoord( const float* shift_vec )
{
	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		vertices_[i].tex_coord[0]+= shift_vec[0];
		vertices_[i].tex_coord[1]+= shift_vec[1];
	}
}

void mx_DrawingModel::TransformTexCoord( const float* transform_mat3x3 )
{
	float vec[3];
	vec[2]= 1.0f;
	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		vec[0]= vertices_[i].tex_coord[0];
		vec[1]= vertices_[i].tex_coord[1];
		float result_vec[3];
		mxVec3Mat4Mul( vec, transform_mat3x3, result_vec );
		vertices_[i].tex_coord[0]= result_vec[0];
		vertices_[i].tex_coord[1]= result_vec[1];
	}
}

void mx_DrawingModel::FlipTriangles()
{
	for( unsigned int i= 0; i< index_count_; i+=3 )
	{
		unsigned short tmp= indeces_[i];
		indeces_[i]= indeces_[i+2];
		indeces_[i+2]= tmp;
	}
}

void mx_DrawingModel::CalculateBoundingBox()
{
	const float max_float= 1e32f;
	for( unsigned int i= 0; i< 3; i++ )
	{
		bounding_box_min_[i]= max_float;
		bounding_box_max_[i]= -max_float;
	}

	for( unsigned int i= 0; i< vertex_count_; i++ )
	{
		for( unsigned int j= 0; j< 3; j++ )
		{
			if( vertices_[i].pos[j] > bounding_box_max_[j] ) bounding_box_max_[j]= vertices_[i].pos[j];
			else if( vertices_[i].pos[j] < bounding_box_min_[j] ) bounding_box_min_[j]= vertices_[i].pos[j];
		}
	}
	for( unsigned int i= 0; i< 3; i++ )
		bounding_sphere_center_[i]= (bounding_box_min_[i] + bounding_box_max_[i]) * 0.5f;

	bounding_sphere_radius_= 0.5f * mxDistance( bounding_box_max_, bounding_box_min_ );
}

bool mx_DrawingModel::BeamIntersectModel( const float* beam_point, const float* beam_dir, float max_distance, float* out_pos_opt ) const
{
	float min_distance= 1e24f;
	bool is_intersection= false;

	const unsigned short* ind= indeces_;
	for( unsigned int i= 0; i< index_count_; i+= 3, ind+= 3 )
	{
		float* triangle[3];
		for( unsigned int j= 0; j < 3; j++ )
			triangle[j]= vertices_[ind[j]].pos;

		float intersection_pos[3];
		if( mxBeamIntersectTriangle( triangle, beam_point, beam_dir, max_distance, intersection_pos ) )
		{
			is_intersection= true;

			float dist= mxDistance( beam_point, intersection_pos );
			if( dist < min_distance )
			{
				min_distance= dist;
				if( out_pos_opt )
				{
					VEC3_CPY( out_pos_opt, intersection_pos );
				}
			}
		}
	}
	return is_intersection;
}