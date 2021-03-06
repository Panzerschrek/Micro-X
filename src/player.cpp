#include "level.h"
#include "mx_assert.h"
#include "mx_math.h"
#include "main_loop.h"

#include "player.h"

#define MX_FOV_CHANGE_SPEED MX_PI4
#define MX_MIN_FOV MX_PI6
#define MX_MAX_FOV (MX_PI2 + MX_PI6)
#define MX_FOV_STEP (MX_PI6 * 0.25f)
#define MX_INITIAL_FOV (MX_PI2 - MX_FOV_STEP)

#define MF_START_ROCKET_COUNT 3

mx_Player::mx_Player()
	: mx_Pawn(mx_GameConstants::initial_player_health)
	, level_(NULL)
	, aspect_(1.0f), fov_(MX_INITIAL_FOV), target_fov_(MX_INITIAL_FOV)
	, sector_(NULL)
	, map_mode_(false)
	, shot_button_pressed_(false)
	, last_shot_time_s_(0.0f)
	, shot_side_(0)
	, current_weapon_(MachinegunBullet)
	, lives_(3), is_dead_(false)
	, forward_pressed_(false), backward_pressed_(false), left_pressed_(false), right_pressed_(false)
	, up_pressed_(false), down_pressed_(false)
	, rotate_up_pressed_(false), rotate_down_pressed_(false), rotate_left_pressed_(false), rotate_right_pressed_(false)
	, rotate_clockwise_pressed_(false), rotate_anticlockwise_pressed_(false)
#ifdef MX_DEBUG
	, debug_noclip_(false)
#endif
{
	pos_[0]= pos_[1]= pos_[2]= 0.0f;

	SetupAfterRespawn();
}

mx_Player::~mx_Player()
{
}

void mx_Player::SetPos( const float* pos )
{
	VEC3_CPY( pos_, pos );
}

void mx_Player::AddHealth( int health )
{
	health_+= health;
	if( health_ > mx_GameConstants::player_max_health )
		health_= mx_GameConstants::player_max_health;
}

void mx_Player::Tick()
{
	float dt= mx_MainLoop::Instance()->GetTickTime();
	float total_time= mx_MainLoop::Instance()->GetTime();

	if( is_dead_ )
	{
		if( lives_ != 0 && total_time - death_time_ >= mx_GameConstants::player_respawn_time )
		{
			is_dead_= false;
			lives_--;

			SetupAfterRespawn();

			level_->RespawnPlayer();
		}
		return;
	}
	if( health_ <= 0 )
	{
		is_dead_= true;
		death_time_= total_time;
		return;
	}

	float move_vector[3]= { 0.0f, 0.0f, 0.0f };
	float rotation_angles[3]= { 0.0f, 0.0f, 0.0f };

	if( !map_mode_ )
	{
		if(forward_pressed_) move_vector[1]+= 1.0f;
		if(backward_pressed_) move_vector[1]+= -1.0f;
		if(left_pressed_) move_vector[0]+= -1.0f;
		if(right_pressed_) move_vector[0]+= 1.0f;
		if(up_pressed_) move_vector[2]+= 1.0f;
		if(down_pressed_) move_vector[2]+= -1.0f;
	}

	if(rotate_up_pressed_) rotation_angles[0] += 1.0f;
	if(rotate_down_pressed_) rotation_angles[0] += -1.0f;
	if(rotate_clockwise_pressed_) rotation_angles[1] += 1.0f;
	if(rotate_anticlockwise_pressed_) rotation_angles[1] += -1.0f;
	if(rotate_left_pressed_) rotation_angles[2] += 1.0f;
	if(rotate_right_pressed_) rotation_angles[2] += -1.0f;

	// Calculate rotations
	static const float c_max_angular_speed= 3.0f;
	static const float c_angular_acceleration= 10.0f;
	static const float c_angular_deceleration= -6.0f;
	float rotation_vector[3]= { 0.0f, 0.0f, 0.0f };
	for( unsigned int i= 0; i< 3; i++ )
	{
		angular_speed_[i]= mxClamp( -c_max_angular_speed, c_max_angular_speed, angular_speed_[i] + rotation_angles[i] * dt * c_angular_acceleration );
		if ( std::fabsf(angular_speed_[i]) > 0.0f )
		{
			float ds= mxSign(angular_speed_[i]) * dt * c_angular_deceleration;
			if( std::fabsf(ds) > std::fabsf(angular_speed_[i]) ) angular_speed_[i]= 0.0f;
			else angular_speed_[i]+= ds;
		}
		if( std::fabsf(angular_speed_[i] ) < 0.01f )
			angular_speed_[i]= 0.0f;

		// Mouse moving filtration
		// a(n) = k * a(n-1) + (1-k) * i
		// s= k^n
		static const float c_value_saving_per_second= 0.005f;
		float k= std::pow( c_value_saving_per_second, dt );
		controller_rotation_accumulated_[i]= controller_rotation_accumulated_[i] * k + controller_rotation_[i] * ( 1.0f - k );

		float axis_rotate_vec[3];
		mxVec3Mul( axis_[i], angular_speed_[i] * dt + controller_rotation_accumulated_[i], axis_rotate_vec );
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

		CorrectAxis();
	}

	// Get final move vector
	float move_vec_mat[16];
	CreateRotationMatrix4( move_vec_mat, true );
	mxVec3Mat4Mul( move_vector, move_vec_mat );
	float move_vec_len= mxVec3Len( move_vector );
	if( move_vec_len > 0.00001f ) mxVec3Mul( move_vector, 1.0f / move_vec_len );

	static const float c_acceleration= 15.0f;
	static const float c_deceleration= 5.0f;
	static const float c_max_speed= 3.5f;
	// Accelerate
	for( unsigned int i= 0; i < 3; i++ )
		speed_[i]+= c_acceleration * dt * move_vector[i];
	
	// Decelerate
	float speed_len= mxVec3Len( speed_ );
	if( speed_len > c_max_speed )
	{
		mxVec3Mul( speed_, c_max_speed / speed_len );
		speed_len= c_max_speed;
	}
	float deceleration_vec_len= c_deceleration * dt;
	if( deceleration_vec_len < speed_len )
	{
		float deceleration_vec[3];
		mxVec3Mul( speed_, deceleration_vec_len / speed_len, deceleration_vec );
		mxVec3Sub( speed_, deceleration_vec );
	}
	else
		speed_[0]= speed_[1]= speed_[2]= 0.0f;

	float d_pos[3];
	mxVec3Mul( speed_, dt, d_pos );
	mxVec3Add( pos_, d_pos );

	MX_ASSERT(level_);
	// Collide
#ifdef MX_DEBUG
	if( !debug_noclip_ )
#endif
	{
		sector_= level_->FindSectorForPoint( pos_ );
		if( sector_ )
		{
			CollideWithSector(sector_);
			for( unsigned int i= 0; i < sector_->connections_count; i++ )
				CollideWithSector( sector_->connections[i] );

			level_->CollideWithMonsters( pos_, mx_GameConstants::player_radius );
		}
	}

	bool monsters_warned= false;
	if( mxVec3Len( speed_ ) > c_max_speed * 0.25f )
	{
		monsters_warned= true;
		level_->WarnMonsters();
	}

	if( map_mode_ )
		return;

	// Shot
	float weapon_shot_interval;
	if( current_weapon_ == MachinegunBullet )
		weapon_shot_interval= mx_GameConstants::machinegun_shot_interval;
	else if( current_weapon_ == Rocket )
		weapon_shot_interval= mx_GameConstants::rocket_launcher_shot_interval;
	else//  if( current_weapon_ == PlasmaBall )
		weapon_shot_interval= mx_GameConstants::plasmagun_shot_interval;

	if( ammo_[current_weapon_] > 0 &&
		shot_button_pressed_ && total_time - last_shot_time_s_ > weapon_shot_interval )
	{
		shot_side_^= 1;

		float shot_pos[3];
		float d_pos[3];
		float dir[2][3];
		float result_dir[3];

		static const float c_shot_pos_shift= 0.125f;
		static const float c_focus_distance= 20.0f;
		float angle= std::atanf( c_focus_distance / c_shot_pos_shift );

		float shift= (shot_side_ == 0) ? c_shot_pos_shift : -c_shot_pos_shift;
		mxVec3Mul( axis_[0], shift, d_pos );
		mxVec3Add( pos_, d_pos, shot_pos );

		mxVec3Mul( axis_[0], -mxSign(shift) * std::cosf(angle), dir[0] );
		mxVec3Mul( axis_[1], std::sinf(angle), dir[1] );
		mxVec3Add( dir[0], dir[1], result_dir );

		last_shot_time_s_= total_time;
		level_->Shot( this, current_weapon_, shot_pos, result_dir );

		ammo_[current_weapon_]--;

		if( !monsters_warned )
			level_->WarnMonsters();
	}
}

void mx_Player::Rotate( float pixel_delta_x, float pixel_delta_y )
{
	static const float c_sensitivity= 0.001f;
	controller_rotation_[2]+= c_sensitivity * pixel_delta_x * std::sqrtf( std::tanf( fov_ * 0.5f ) );
	controller_rotation_[0]+= c_sensitivity * pixel_delta_y * std::sqrtf( std::tanf( fov_ * 0.5f ) );
}


void mx_Player::ToggleMapMode()
{
	if( !map_mode_ )
	{
		if( sector_ && mxDistance( sector_->map_screen_pos, pos_ ) <= 0.7f )
			map_mode_= true;
	}
	else
		map_mode_= false;
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

void mx_Player::CollideWithSector( const mx_LevelSector* sector )
{
	level_->CollideWithSectorTriangles( pos_, mx_GameConstants::player_radius, sector );
}

void mx_Player::SetupAfterRespawn()
{
	controller_rotation_[0]= controller_rotation_[1]= controller_rotation_[2]= 0.0f;
	controller_rotation_accumulated_[0]= controller_rotation_accumulated_[1]= controller_rotation_accumulated_[2]= 0.0f;

	axis_[0][0]= 1.0f;
	axis_[0][1]= 0.0f;
	axis_[0][2]= 0.0f;

	axis_[1][0]= 0.0f;
	axis_[1][1]= 1.0f;
	axis_[1][2]= 0.0f;

	axis_[2][0]= 0.0f;
	axis_[2][1]= 0.0f;
	axis_[2][2]= 1.0f;

	angular_speed_[0]= angular_speed_[1]= angular_speed_[2]= 0.0f;
	speed_[0]= speed_[1]= speed_[2]= 0.0f;

	health_= mx_GameConstants::initial_player_health;

	for( unsigned int i= 0; i < LastBullet; i++ )
		ammo_[i]= mx_GameConstants::player_initial_ammo[i];
}