﻿#pragma once

#include "pawn.h"

class mx_Level;

class mx_Player : public mx_Pawn
{
public:
	mx_Player();
	~mx_Player();

	void SetLevel( mx_Level* level );

	const float Fov() const;

	void Tick();
	void Rotate( float pixel_delta_x, float pixel_delta_y );

	void ShotButtonPressed();
	void ShotButtonReleased();

	void ForwardPressed();
	void BackwardPressed();
	void LeftPressed();
	void RightPressed();
	void ForwardReleased();
	void BackwardReleased();
	void LeftReleased();
	void RightReleased();

	void UpPressed();
	void DownPressed();
	void UpReleased();
	void DownReleased();

	void RotateUpPressed();
	void RotateDownPressed();
	void RotateLeftPressed();
	void RotateRightPressed();
	void RotateUpReleased();
	void RotateDownReleased();
	void RotateLeftReleased();
	void RotateRightReleased();

	void RotateClockwisePressed();
	void RotateAnticlockwisePressed();

	void RotateClockwiseReleased();
	void RotateAnticlockwiseReleased();

	void ZoomIn();
	void ZoomOut();

#ifdef MX_DEBUG
	void DebugToggleNoclip();
#endif

private:
	void CollideWithSector( const mx_LevelData::Sector* sector );

private:
	// 0 - pitch / tangaž
	// 1 - yaw / ryskanije
	// 2 - roll / kren
	//float angle_[3];
	mx_Level* level_;

	float speed_[3];
	float angular_speed_[3];

	float cam_radius_;
	float aspect_;
	float fov_;
	float target_fov_;

	float controller_rotation_[3];

	bool shot_button_pressed_;
	float last_shot_time_s_;

	bool forward_pressed_, backward_pressed_, left_pressed_, right_pressed_;
	bool up_pressed_, down_pressed_;
	bool rotate_up_pressed_, rotate_down_pressed_, rotate_left_pressed_, rotate_right_pressed_;
	bool rotate_clockwise_pressed_, rotate_anticlockwise_pressed_;

#ifdef MX_DEBUG
	bool debug_noclip_;
#endif
};

inline void mx_Player::SetLevel( mx_Level* level )
{
	level_= level;
}

/*
inline const float* mx_Player::Angle() const
{
	return angle_;
}
*/
inline const float mx_Player::Fov() const
{
	return fov_;
}

inline void mx_Player::ShotButtonPressed()
{
	shot_button_pressed_= true;
}

inline void mx_Player::ShotButtonReleased()
{
	shot_button_pressed_= false;
}

inline void mx_Player::ForwardPressed()
{
	forward_pressed_= true;
}

inline void mx_Player::BackwardPressed()
{
	backward_pressed_= true;
}

inline void mx_Player::LeftPressed()
{
	left_pressed_= true;
}

inline void mx_Player::RightPressed()
{
	right_pressed_= true;
}

inline void mx_Player::ForwardReleased()
{
	forward_pressed_= false;
}

inline void mx_Player::BackwardReleased()
{
	backward_pressed_= false;
}

inline void mx_Player::LeftReleased()
{
	left_pressed_= false;
}

inline void mx_Player::RightReleased()
{
	right_pressed_= false;
}

inline void mx_Player::UpPressed()
{
	up_pressed_= true;
}

inline void mx_Player::DownPressed()
{
	down_pressed_= true;
}

inline void mx_Player::UpReleased()
{
	up_pressed_= false;
}

inline void mx_Player::DownReleased()
{
	down_pressed_= false;
}

inline void mx_Player::RotateUpPressed()
{
	rotate_up_pressed_= true;
}

inline void mx_Player::RotateDownPressed()
{
	rotate_down_pressed_= true;
}

inline void mx_Player::RotateLeftPressed()
{
	rotate_left_pressed_= true;
}

inline void mx_Player::RotateRightPressed()
{
	rotate_right_pressed_= true;
}

inline void mx_Player::RotateUpReleased()
{
	rotate_up_pressed_= false;
}

inline void mx_Player::RotateDownReleased()
{
	rotate_down_pressed_= false;
}

inline void mx_Player::RotateLeftReleased()
{
	rotate_left_pressed_= false;
}

inline void mx_Player::RotateRightReleased()
{
	rotate_right_pressed_= false;
}

inline void mx_Player::RotateClockwisePressed()
{
	rotate_clockwise_pressed_= true;
}

inline void mx_Player::RotateAnticlockwisePressed()
{
	rotate_anticlockwise_pressed_= true;
}

inline void mx_Player::RotateClockwiseReleased()
{
	rotate_clockwise_pressed_= false;
}

inline void mx_Player::RotateAnticlockwiseReleased()
{
	rotate_anticlockwise_pressed_= false;
}

#ifdef MX_DEBUG
inline void mx_Player::DebugToggleNoclip()
{
	debug_noclip_= !debug_noclip_;
}
#endif