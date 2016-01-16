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
	angle_[0]= angle_[1]= angle_[2]= 0.0f;
}

mx_Player::~mx_Player()
{
}

void mx_Player::Tick( float dt )
{
	// TODO - add 100% free rotation and moving

	float move_vector[3]= { 0.0f, 0.0f, 0.0f };
	float rotate_vector[3]= { 0.0f, 0.0f, 0.0f };

	if(forward_pressed_) move_vector[1]+= 1.0f;
	if(backward_pressed_) move_vector[1]+= -1.0f;
	if(left_pressed_) move_vector[0]+= -1.0f;
	if(right_pressed_) move_vector[0]+= 1.0f;
	if(up_pressed_) move_vector[2]+= 1.0f;
	if(down_pressed_) move_vector[2]+= -1.0f;

	if(rotate_up_pressed_) rotate_vector[0] += 1.0f;
	if(rotate_down_pressed_) rotate_vector[0] += -1.0f;
	if(rotate_clockwise_pressed_) rotate_vector[1] += 1.0f;
	if(rotate_anticlockwise_pressed_) rotate_vector[1] += -1.0f;
	if(rotate_left_pressed_) rotate_vector[2] += 1.0f;
	if(rotate_right_pressed_) rotate_vector[2] += -1.0f;

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
	angle_[0]+= da * rotate_vector[0];
	angle_[1]+= da * rotate_vector[1];
	angle_[2]+= da * rotate_vector[2];

	if( angle_[0] >= MX_PI2 ) angle_[0]= MX_PI2;
	else if( angle_[0] <= -MX_PI2 ) angle_[0]= -MX_PI2;

	if( angle_[1] >= MX_PI ) angle_[1]-= MX_2PI;
	else if( angle_[1] < -MX_PI ) angle_[1]+= MX_2PI;

	if( angle_[2] >= MX_PI ) angle_[2]-= MX_2PI;
	else if( angle_[2] < -MX_PI ) angle_[2]+= MX_2PI;
}

void mx_Player::Rotate( float pixel_delta_x, float pixel_delta_y )
{
	angle_[0]+= pixel_delta_x * 0.01f * std::sqrtf( std::tanf( fov_ * 0.5f ) );
	angle_[2]+= pixel_delta_y * 0.01f * std::sqrtf( std::tanf( fov_ * 0.5f ) );
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