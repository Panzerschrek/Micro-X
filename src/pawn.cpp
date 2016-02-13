#include "mx_math.h"

#include "pawn.h"

mx_Pawn::mx_Pawn( int health )
	: health_(health)
{
}

mx_Pawn::~mx_Pawn()
{
}

void mx_Pawn::CreateRotationMatrix4( float* out_mat, bool invert ) const
{
	mxMat4Identity( out_mat );
	for( unsigned int i= 0; i < 3; i++ )
	{
		out_mat[ 4 * i + 0 ]= axis_[0][i];
		out_mat[ 4 * i + 1 ]= axis_[1][i];
		out_mat[ 4 * i + 2 ]= axis_[2][i];
	}
	if( invert )
		mxMat4Transpose( out_mat );
}

void mx_Pawn::CorrectAxis()
{
	mxVec3Cross( axis_[0], axis_[1], axis_[2] );
	mxVec3Cross( axis_[1], axis_[2], axis_[0] );
	mxVec3Normalize( axis_[0] );
	mxVec3Normalize( axis_[1] );
	mxVec3Normalize( axis_[2] );
}