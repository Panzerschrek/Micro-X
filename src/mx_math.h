#pragma once
#include <cmath>
#include <cstring>

#define MX_PI   3.141592653589793238462643383279f
#define MX_2PI  6.283185307179586476925286766559f
#define MX_PI2  1.570796326794896619231321691639f
#define MX_2PI3 2.094395102393195492308428922186f
#define MX_PI3  1.047197551196597746154214461093f
#define MX_PI4  0.785398163397448309615660845819f
#define MX_PI6  0.523598775598298873077107230546f
#define MX_PI8  0.392699081698724154807830422909f
#define MX_PI12 0.261799387799149436538553615273f
#define MX_DEG2RAD 0.017453292519943295769236907684886f
#define MX_RAD2DEG 57.295779513082320876798154814105f
#define MX_SQRT_2 1.4142135623730950488016887242097f
#define MX_INV_SQRT_2 0.70710678118654752440084436210485f

inline float mxInf()
{
	float f;
	unsigned int ui= 0x7F800000;
	std::memcpy( &f, &ui, sizeof(f) );
	return f;
}

inline float mxClamp( float min, float max, float x )
{
	if( x < min ) return min;
	if( x > max ) return max;
	return x;
}

inline float mxSign( float x )
{
	if( x > 0.0f ) return +1.0f;
	if( x < 0.0f ) return -1.0f;
	return 0.0f;
}

inline float mxRound( float x )
{
	return std::ceilf( x - 0.5f );
}

#define VEC3_CPY(dst,src) (dst)[0]= (src)[0]; (dst)[1]= (src)[1]; (dst)[2]= src[2];

void mxVec3Mul( const float* v, float s, float* v_out );
void mxVec3Mul( float* v, float s );
void mxVec3Mul( const float* v0, const float* v1, float* v_out );
void mxVec3Add( float* v0, const float* v1 );
void mxVec3Add( const float* v0, const float* v1, float* v_dst );
void mxVec3Sub( float* v0, const float* v1 );
void mxVec3Sub( const float* v0, const float* v1, float* v_dst );
float mxVec3Dot( const float* v0, const float* v1 );
void mxVec3Cross( const float* v0, const float* v1, float* v_dst );
float mxVec3SquareLen( const float* v );
float mxVec3Len( const float* v );
float mxSquareDistance( const float* v0, const float* v1 );
float mxDistance( const float* v0, const float* v1 );
void mxVec3Normalize( const float* v, float* v_out );
void mxVec3Normalize( float* v );

void mxSphericalCoordinatesToVec( float longitude, float latitude, float* out_vec );
void mxVecToSphericalCoordinates( const float* vec, float* out_longitude, float* out_latitude );

void mxMat4Identity( float* m );
void mxMat4Transpose( float* m );
void mxMat4Invert( const float* m, float* out_m );
void mxMat4Scale( float* mat, const float* scale );
void mxMat4Scale( float* mat, float scale );
void mxVec3Mat4Mul( const float* v, const float* m, float* v_dst );
void mxVec3Mat4Mul( float* v_dst, const float* m );
void mxVec3Mat3Mul( const float* v, const float* m, float* v_dst );
void mxVec3Mat3Mul( float* v_dst, const float* m );
void mxVec4Mat4Mul( const float* v, const float* m, float* v_dst );
void mxMat4Mul( const float* m0, const float* m1, float* m_dst );
void mxMat4Mul( float* m0_dst, const float* m1 );
void mxMat4RotateX( float* m, float a );
void mxMat4RotateY( float* m, float a );
void mxMat4RotateZ( float* m, float a );
void mxMat4Translate( float* m, const float* v );
void mxMat4Perspective( float* m, float aspect, float fov, float z_near, float z_far );
void mxMat4RotateAroundVector( float* m, const float* vec, float angle );
void mxMatToRotation( const float* m, float* out_vec, float* out_angle );

float mxMat3Det( const float* m );
void mxMat4ToMat3( float* m );
void mxMat4ToMat3( const float* m_in, float* m_out );

void mxDoubleMat4Invert( const double* m, double* out_m );

float mxDistanceFromLineToPoint(const float* line_point, const float* line_dir, const float* point );

bool mxBeamIntersectModel(
	const float* const* triangle, // 3 pointers to 3d vectors
	const float* beam_point, const float* beam_dir,
	float max_distance,
	float* out_pos_opt );

class mx_Rand
{
public:
	mx_Rand( unsigned int seeed= 0 );

	unsigned int Rand();
	float RandF( float max );
	float RandF( float min, float max );
	unsigned int RandI( unsigned int min, unsigned int max );
	unsigned int RandI( unsigned int max );

	static const unsigned int rand_max_;
	static const unsigned int rand_max_plus_one_;
private:
	unsigned int r;
};