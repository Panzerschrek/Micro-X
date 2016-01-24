#include "mx_math.h"

#include "player.h"

#define MX_FOV_CHANGE_SPEED MX_PI4
#define MX_MIN_FOV MX_PI6
#define MX_MAX_FOV (MX_PI2 + MX_PI6)
#define MX_FOV_STEP (MX_PI6 * 0.25f)
#define MX_INITIAL_FOV (MX_PI2 - MX_FOV_STEP)

#define MF_START_ROCKET_COUNT 3

mx_Player::mx_Player()
	:  aspect_(1.0f), fov_(MX_INITIAL_FOV), target_fov_(MX_INITIAL_FOV)
	, forward_pressed_(false), backward_pressed_(false), left_pressed_(false), right_pressed_(false)
	, up_pressed_(false), down_pressed_(false)
	, rotate_up_pressed_(false), rotate_down_pressed_(false), rotate_left_pressed_(false), rotate_right_pressed_(false)
	, rotate_clockwise_pressed_(false), rotate_anticlockwise_pressed_(false)
{
	pos_[0]= pos_[1]= pos_[2]= 0.0f;
	//angle_[0]= angle_[1]= angle_[2]= 0.0f;
	controller_rotation_[0]= controller_rotation_[1]= controller_rotation_[2]= 0.0f;

	axis_[0][0]= 1.0f;
	axis_[0][1]= 0.0f;
	axis_[0][2]= 0.0f;

	axis_[1][0]= 0.0f;
	axis_[1][1]= 1.0f;
	axis_[1][2]= 0.0f;

	axis_[2][0]= 0.0f;
	axis_[2][1]= 0.0f;
	axis_[2][2]= 1.0f;
}

mx_Player::~mx_Player()
{
}

void mx_Player::CreateRotationMatrix4( float* out_mat ) const
{
	mxMat4Identity( out_mat );
	for( unsigned int i= 0; i < 3; i++ )
	{
		out_mat[ 4 * i + 0 ]= axis_[0][i];
		out_mat[ 4 * i + 1 ]= axis_[1][i];
		out_mat[ 4 * i + 2 ]= axis_[2][i];
	}
}

void mx_Player::Tick( float dt )
{
	float move_vector[3]= { 0.0f, 0.0f, 0.0f };
	float rotation_angles[3]= { 0.0f, 0.0f, 0.0f };

	if(forward_pressed_) move_vector[1]+= 1.0f;
	if(backward_pressed_) move_vector[1]+= -1.0f;
	if(left_pressed_) move_vector[0]+= -1.0f;
	if(right_pressed_) move_vector[0]+= 1.0f;
	if(up_pressed_) move_vector[2]+= 1.0f;
	if(down_pressed_) move_vector[2]+= -1.0f;

	if(rotate_up_pressed_) rotation_angles[0] += 1.0f;
	if(rotate_down_pressed_) rotation_angles[0] += -1.0f;
	if(rotate_clockwise_pressed_) rotation_angles[1] += 1.0f;
	if(rotate_anticlockwise_pressed_) rotation_angles[1] += -1.0f;
	if(rotate_left_pressed_) rotation_angles[2] += 1.0f;
	if(rotate_right_pressed_) rotation_angles[2] += -1.0f;

/*
		float move_vector_rot_mat[16];
		mxMat4RotateZ( move_vector_rot_mat, angle_[2] );
		mxVec3Mat4Mul( move_vector, move_vector_rot_mat );

		const float speed= 20.0f;
		float ds= dt * speed;
		pos_[0]+= ds * move_vector[0];
		pos_[1]+= ds * move_vector[1];
		pos_[2]+= ds * move_vector[2];

		const float rot_speed= 2.0f;
		float da= dt * rot_speed;
		angle_[0]+= da * rotation_angles[0];
		angle_[1]+= da * rotation_angles[1];
		angle_[2]+= da * rotation_angles[2];

		if( angle_[0] >= MX_PI2 ) angle_[0]= MX_PI2;
		else if( angle_[0] <= -MX_PI2 ) angle_[0]= -MX_PI2;

		if( angle_[1] >= MX_PI ) angle_[1]-= MX_2PI;
		else if( angle_[1] < -MX_PI ) angle_[1]+= MX_2PI;

		if( angle_[2] >= MX_PI ) angle_[2]-= MX_2PI;
		else if( angle_[2] < -MX_PI ) angle_[2]+= MX_2PI;
*/

	// Calculate rotations
	float rotation_vector[3]= { 0.0f, 0.0f, 0.0f };
	const float rot_speed= 2.0f;
	float da= dt * rot_speed;
	for( unsigned int i= 0; i< 3; i++ )
	{
		float axis_rotate_vec[3];
		mxVec3Mul( axis_[i], rotation_angles[i] * da + controller_rotation_[i], axis_rotate_vec );
		mxVec3Add( rotation_vector, axis_rotate_vec );

		controller_rotation_[i]= 0.0f;
	}

	float rot_angle=  mxVec3Len(rotation_vector);
	if( rot_angle > 0.0001f )
	{
		float rotate_mat[16];
		mxMat4RotateAroundVector( rotate_mat, rotation_vector, rot_angle );

		// make final rotation
		mxVec3Mat4Mul( axis_[0], rotate_mat );
		mxVec3Mat4Mul( axis_[1], rotate_mat );
		mxVec3Mat4Mul( axis_[2], rotate_mat );

		// make axis orthogonal and with identity vectors
		mxVec3Cross( axis_[0], axis_[1], axis_[2] );
		mxVec3Cross( axis_[1], axis_[2], axis_[0] );
		mxVec3Normalize( axis_[0] );
		mxVec3Normalize( axis_[1] );
		mxVec3Normalize( axis_[2] );
	}

	// Get final move vector
	float move_vec_mat[16];
	CreateRotationMatrix4( move_vec_mat );
	mxMat4Transpose( move_vec_mat );
	mxVec3Mat4Mul( move_vector, move_vec_mat );

	const float speed= 5.0f;
	float ds= dt * speed;
	pos_[0]+= ds * move_vector[0];
	pos_[1]+= ds * move_vector[1];
	pos_[2]+= ds * move_vector[2];
}

void mx_Player::Rotate( float pixel_delta_x, float pixel_delta_y )
{
	controller_rotation_[2]+= pixel_delta_x * 0.001f * std::sqrtf( std::tanf( fov_ * 0.5f ) );
	controller_rotation_[0]+= pixel_delta_y * 0.001f * std::sqrtf( std::tanf( fov_ * 0.5f ) );
}

void mx_Player::ZoomIn()
{
	target_fov_-= MX_FOV_STEP;
	if( target_fov_ < MX_MIN_FOV ) target_fov_= MX_MIN_FOV;
}

void mx_Player::ZoomOut()
{
	target_fov_+= MX_FOV_STEP;
	if( target_fov_ > MX_MAX_FOV ) target_fov_= MX_MAX_FOV;
}