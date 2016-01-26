#include "mx_math.h"

namespace Mat4InvertData
{
	static const char sign[]=
	{
		1, -1, 1, -1,
		-1, 1, -1, 1,
		1, -1, 1, -1,
		-1, 1, -1, 1
	};
	static const unsigned char indeces[]=
	{
		5, 6, 7, 9, 10, 11, 13, 14, 15,
		4, 6, 7, 8, 10, 11, 12, 14, 15,
		4, 5, 7, 8,  9, 11, 12, 13, 15,
		4, 5, 6, 8,  9, 10, 12, 13, 14,
	
		1, 2, 3, 9, 10, 11, 13, 14, 15,
		0, 2, 3, 8, 10, 11, 12, 14, 15,
		0, 1, 3, 8,  9, 11, 12, 13, 15,
		0, 1, 2, 8,  9, 10, 12, 13, 14,

		1, 2, 3, 5, 6, 7, 13, 14, 15,
		0, 2, 3, 4, 6, 7, 12, 14, 15,
		0, 1, 3, 4, 5, 7, 12, 13, 15,
		0, 1, 2, 4, 5, 6, 12, 13, 14,

		1, 2, 3, 5, 6, 7, 9, 10, 11,
		0, 2, 3, 4, 6, 7, 8, 10, 11,
		0, 1, 3, 4, 5, 7, 8,  9, 11,
		0, 1, 2, 4, 5, 6, 8,  9, 10
	};
};

void mxVec3Mul( const float* v, float s, float* v_out )
{
	v_out[0]= v[0] * s;
	v_out[1]= v[1] * s;
	v_out[2]= v[2] * s;
}

void mxVec3Mul( float* v, float s )
{
	v[0]*= s;
	v[1]*= s;
	v[2]*= s;
}

void mxVec3Mul( const float* v0, const float* v1, float* v_out )
{
	v_out[0]= v0[0] * v1[0];
	v_out[1]= v0[1] * v1[1];
	v_out[2]= v0[2] * v1[2];
}

void mxVec3Add( float* v0, const float* v1 )
{
	v0[0]+= v1[0];
	v0[1]+= v1[1];
	v0[2]+= v1[2];
}

void mxVec3Add( const float* v0, const float* v1, float* v_dst )
{
	v_dst[0]= v0[0] + v1[0];
	v_dst[1]= v0[1] + v1[1];
	v_dst[2]= v0[2] + v1[2];
}

void mxVec3Sub( float* v0, const float* v1 )
{
	v0[0]-= v1[0];
	v0[1]-= v1[1];
	v0[2]-= v1[2];
}

void mxVec3Sub( const float* v0, const float* v1, float* v_dst )
{
	v_dst[0]= v0[0] - v1[0];
	v_dst[1]= v0[1] - v1[1];
	v_dst[2]= v0[2] - v1[2];
}

float mxVec3Dot( const float* v0, const float* v1 )
{
	return v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2];
}

void mxVec3Cross( const float* v0, const float* v1, float* v_dst )
{
	v_dst[0]= v0[1] * v1[2] - v0[2] * v1[1];
	v_dst[1]= v0[2] * v1[0] - v0[0] * v1[2];
	v_dst[2]= v0[0] * v1[1] - v0[1] * v1[0];
}

float mxVec3SquareLen( const float* v )
{
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

float mxVec3Len( const float* v )
{
	return std::sqrtf( mxVec3SquareLen(v) );
}

float mxSquareDistance( const float* v0, const float* v1 )
{
	float v[3];
	mxVec3Sub( v0, v1, v );
	return mxVec3SquareLen(v);
}

float mxDistance( const float* v0, const float* v1 )
{
	float v[3];
	mxVec3Sub( v0, v1, v );
	return mxVec3Len(v);
}

void mxSphericalCoordinatesToVec( float longitude, float latitude, float* out_vec )
{
	out_vec[0]= out_vec[1]= std::cosf( latitude );
	out_vec[2]= std::sinf( latitude );

	out_vec[0]*= -std::sinf( longitude );
	out_vec[1]*= +std::cosf( longitude );
}

void mxVecToSphericalCoordinates( const float* vec, float* out_longitude, float* out_latitude )
{
	static const float c_almost_one= 0.9999998f;
	*out_latitude= std::asinf( mxClamp( -c_almost_one, c_almost_one, vec[2]) );

	float lat_cos2= 1.0f - mxClamp( -c_almost_one, c_almost_one, vec[2] * vec[2] );
	float vec_y= vec[1] / std::sqrtf(lat_cos2);

	*out_longitude= std::acosf( mxClamp( -c_almost_one, c_almost_one, vec_y) );
	if ( vec[0] > 0.0f ) *out_longitude= MX_2PI - *out_longitude;
}

void mxVec3Normalize( const float* v, float* v_out )
{
	float l= 1.0f / mxVec3Len(v);

	v_out[0]= v[0] * l;
	v_out[1]= v[1] * l;
	v_out[2]= v[2] * l;
}

void mxVec3Normalize( float* v )
{
	float l= 1.0f / mxVec3Len(v);

	v[0]*= l;
	v[1]*= l;
	v[2]*= l;
}

void mxMat4Identity( float* m )
{
	unsigned int i= 0;
	for( i= 1; i< 15; i++ )
		m[i]= 0.0f;
	for( i= 0; i< 16; i+=5 )
		m[i]= 1.0f;
}

void mxMat4Transpose( float* m )
{
	float tmp;
	for( unsigned int j= 0; j< 3; j++ )
		for( unsigned int i= j; i< 4; i++ )
		{
			tmp= m[i + j*4 ];
			m[ i + j*4]= m[ j + i*4 ];
			m[ j + i*4 ]= tmp;
		}
}

void mxMat4Invert( const float* m, float* out_m )
{
	float mat3x3[9];

	for( unsigned int i= 0, i9= 0; i< 16; i++, i9+= 9 )
	{
		for( unsigned int j= 0; j< 9; j++ )
			mat3x3[j]= m[ Mat4InvertData::indeces[i9+j] ];
		out_m[i]= float(Mat4InvertData::sign[i]) * mxMat3Det(mat3x3);
	}

	float det= out_m[0] * m[0] + out_m[1] * m[1] + out_m[2] * m[2] + out_m[3] * m[3];
	float inv_det= 1.0f / det;

	for( unsigned int i= 0; i< 16; i++ )
		out_m[i]*= inv_det;

	mxMat4Transpose(out_m);
}

void mxMat4Scale( float* mat, const float* scale )
{
	mxMat4Identity(mat);
	mat[ 0]= scale[0];
	mat[ 5]= scale[1];
	mat[10]= scale[2];
}

void mxMat4Scale( float* mat, float scale )
{
	mxMat4Identity(mat);
	mat[ 0]= scale;
	mat[ 5]= scale;
	mat[10]= scale;
}

void mxVec3Mat4Mul( const float* v, const float* m, float* v_dst )
{
	for( unsigned int i= 0; i< 3; i++ )
		v_dst[i]= v[0] * m[i] + v[1] * m[i+4] + v[2] * m[i+8] + m[i+12];
}

void mxVec3Mat4Mul( float* v_dst, const float* m )
{
	float v[3];
	for( unsigned int i= 0; i< 3; i++ )
		v[i]= v_dst[i];

	mxVec3Mat4Mul( v, m, v_dst );
}

void mxVec3Mat3Mul( const float* v, const float* m, float* v_dst )
{
	for( unsigned int i= 0; i< 3; i++ )
		v_dst[i]= v[0] * m[i] + v[1] * m[i+3] + v[2] * m[i+6];
}

void mxVec3Mat3Mul( float* v_dst, const float* m )
{
	float v[3];
	for( unsigned int i= 0; i< 3; i++ )
		v[i]= v_dst[i];
	
	mxVec3Mat3Mul( v, m, v_dst );
}

void mxVec4Mat4Mul( const float* v, const float* m, float* v_dst )
{
	for( unsigned int i= 0; i< 4; i++ )
		v_dst[i]= v[0] * m[i] + v[1] * m[i+4] + v[2] * m[i+8] + v[3] * m[i+12];
}

void mxMat4Mul( const float* m0, const float* m1, float* m_dst )
{
	for( unsigned int i= 0; i< 4; i++ )
		for( unsigned int j= 0; j< 16; j+=4 )
		{
			m_dst[ i + j ]=
				m0[ 0 + j ] * m1[ i ]     + m0[ 1 + j ] * m1[ 4 + i ] +
				m0[ 2 + j ] * m1[ 8 + i ] + m0[ 3 + j ] * m1[ 12 + i ];
		}
}

void mxMat4Mul( float* m0_dst, const float* m1 )
{
	float m0[16];
	for( unsigned int i= 0; i< 16; i++ )
		m0[i]= m0_dst[i];

	mxMat4Mul( m0, m1, m0_dst );
}

void mxMat4RotateX( float* m, float a )
{
	mxMat4Identity(m);
	float s= std::sinf(a), c= std::cosf(a);
	m[ 5]= c;
	m[ 9]= -s;
	m[ 6]= s;
	m[10]= c;
}

void mxMat4RotateY( float* m, float a )
{
	mxMat4Identity(m);
	float s= std::sinf(a), c= std::cosf(a);
	m[ 0]= c;
	m[ 8]= s;
	m[ 2]= -s;
	m[10]= c;
}

void mxMat4RotateZ( float* m, float a )
{
	mxMat4Identity(m);
	float s= std::sinf(a), c= std::cosf(a);
	m[0]= c;
	m[4]= -s;
	m[1]= s;
	m[5]= c;
}

void mxMat4Translate( float* m, const float* v )
{
	mxMat4Identity(m);
	m[12]= v[0];
	m[13]= v[1];
	m[14]= v[2];
}

void mxMat4Perspective( float* m, float aspect, float fov, float z_near, float z_far )
{
	float f= 1.0f / std::tanf( fov * 0.5f );

	m[0]= f / aspect;
	m[5]= f;

	f= 1.0f / ( z_far - z_near );
	m[14]= -2.0f * f * z_near * z_far;
	m[10]= ( z_near + z_far ) * f;
	m[11]= 1.0f;

	m[1]= m[2]= m[3]= 0.0f;
	m[4]= m[6]= m[7]= 0.0f;
	m[8]= m[9]= 0.0f;
	m[12]= m[13]= m[15]= 0.0f;
}

void mxMat4RotateAroundVector( float* m, const float* vec, float angle )
{
	float normalized_vec[3];
	VEC3_CPY( normalized_vec, vec );
	mxVec3Normalize( normalized_vec );

	float cos_a= std::cosf( angle );
	float sin_a= std::sinf( -angle );
	float one_minus_cos_a= 1.0f - cos_a;

	mxMat4Identity( m );
	m[ 0]= cos_a + one_minus_cos_a * normalized_vec[0] * normalized_vec[0];
	m[ 1]= one_minus_cos_a * normalized_vec[0] * normalized_vec[1] - sin_a * normalized_vec[2];
	m[ 2]= one_minus_cos_a * normalized_vec[0] * normalized_vec[2] + sin_a * normalized_vec[1];
	m[ 4]= one_minus_cos_a * normalized_vec[0] * normalized_vec[1] + sin_a * normalized_vec[2];
	m[ 5]= cos_a + one_minus_cos_a * normalized_vec[1] * normalized_vec[1];
	m[ 6]= one_minus_cos_a * normalized_vec[1] * normalized_vec[2] - sin_a * normalized_vec[0];
	m[ 8]= one_minus_cos_a * normalized_vec[0] * normalized_vec[2] - sin_a * normalized_vec[1];
	m[ 9]= one_minus_cos_a * normalized_vec[1] * normalized_vec[2] + sin_a * normalized_vec[0];
	m[10]= cos_a + one_minus_cos_a * normalized_vec[2] * normalized_vec[2];
}

void mxMatToRotation( const float* m, float* out_vec, float* out_angle )
{
	float cos_a= ( m[0] + m[5] + m[10] - 1.0f ) * 0.5f;
	float sin_a= std::sqrtf( mxClamp( 0.0f, 1.0f, 1.0f - cos_a * cos_a ) );

	float inv_two_sin_a= 0.5f / sin_a;

	out_vec[0]= ( m[6] - m[9] ) * inv_two_sin_a;
	out_vec[1]= ( m[8] - m[2] ) * inv_two_sin_a;
	out_vec[2]= ( m[1] - m[4] ) * inv_two_sin_a;

	*out_angle= std::asinf( sin_a );
}

float mxMat3Det( const float* m )
{
return
	m[0] * ( m[4] * m[8] - m[5] * m[7] ) -
	m[1] * ( m[3] * m[8] - m[5] * m[6] ) +
	m[2] * ( m[3] * m[7] - m[4] * m[6] );
}

void mxMat4ToMat3( float* m )
{
	for( unsigned int i= 0; i< 3; i++ )
		m[3+i]= m[4+i];
	for( unsigned int i= 0; i< 3; i++ )
		m[6+i]= m[8+i];
}

void mxMat4ToMat3( const float* m_in, float* m_out )
{
	for( unsigned int y= 0; y< 3; y++ )
		for( unsigned int x= 0; x< 3; x++ )
			m_out[x + y*3]= m_in[ x + y*4 ];
}

// double matrix functions

void mxDoubleMat4Transpose( double* m )
{
	double tmp;
	for( unsigned int j= 0; j< 3; j++ )
		for( unsigned int i= j; i< 4; i++ )
		{
			tmp= m[i + j*4 ];
			m[ i + j*4]= m[ j + i*4 ];
			m[ j + i*4 ]= tmp;
		}
}

static double mxDoubleMat3Det( const double* m )
{
return
	m[0] * ( m[4] * m[8] - m[5] * m[7] ) -
	m[1] * ( m[3] * m[8] - m[5] * m[6] ) +
	m[2] * ( m[3] * m[7] - m[4] * m[6] );
}

void mxDoubleMat4Invert( const double* m, double* out_m )
{
	double mat3x3[9];

	for( unsigned int i= 0, i9= 0; i< 16; i++, i9+= 9 )
	{
		for( unsigned int j= 0; j< 9; j++ )
			mat3x3[j]= m[ Mat4InvertData::indeces[i9+j] ];
		out_m[i]= double(Mat4InvertData::sign[i]) * mxDoubleMat3Det(mat3x3);
	}

	double det= out_m[0] * m[0] + out_m[1] * m[1] + out_m[2] * m[2] + out_m[3] * m[3];
	double inv_det= 1.0f / det;

	for( unsigned int i= 0; i< 16; i++ )
		out_m[i]*= inv_det;

	mxDoubleMat4Transpose(out_m);
}

float mxDistanceFromLineToPoint(const float* line_point, const float* line_dir, const float* point )
{
	float vec_from_pomt_to_point[3];
	mxVec3Sub( point, line_point, vec_from_pomt_to_point );

	float projection_to_line[3];
	mxVec3Mul( line_dir, mxVec3Dot(vec_from_pomt_to_point, line_dir), projection_to_line );

	float perpendicular[3];
	mxVec3Sub( vec_from_pomt_to_point, projection_to_line, perpendicular );
	return mxVec3Len( perpendicular );
}

// mf_Rand class

const unsigned int mx_Rand::rand_max_= 0x7FFF;
const unsigned int mx_Rand::rand_max_plus_one_= 0x8000;

mx_Rand::mx_Rand( unsigned int seed )
	: r(seed)
{
}

unsigned int mx_Rand::Rand()
{
	r= ( ( 22695477 * r + 1 ) & 0x7FFFFFFF );
	return r>>16;
}

float mx_Rand::RandF( float max )
{
	return max * float(Rand()) / float(rand_max_plus_one_);
}

float mx_Rand::RandF( float min, float max )
{
	return (max - min) * float(Rand()) / float(rand_max_plus_one_) + min;
}

unsigned int mx_Rand::RandI( unsigned int min, unsigned int max )
{
	return ((unsigned long long int)(max - min)) * Rand() / rand_max_plus_one_ + min;
}

unsigned int mx_Rand::RandI( unsigned int max )
{
	return ((long long int)(max)) * Rand() / rand_max_plus_one_;
}