#pragma once

class mx_Player
{
public:
	mx_Player();
	~mx_Player();

	const float* Pos() const;
	//const float* Angle() const;
	void CreateRotationMatrix4( float* out_mat ) const;
	const float Fov() const;

	void Tick( float dt );
	void Rotate( float pixel_delta_x, float pixel_delta_y );

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

//private:
	// TEMP HACK REMOVE ME
	float pos_[3];
private:
	// 0 - pitch / tangaž
	// 1 - yaw / ryskanije
	// 2 - roll / kren
	//float angle_[3];
	float axis_[3][3];

	float cam_radius_;
	float aspect_;
	float fov_;
	float target_fov_;

	float controller_rotation_[3];

	bool forward_pressed_, backward_pressed_, left_pressed_, right_pressed_;
	bool up_pressed_, down_pressed_;
	bool rotate_up_pressed_, rotate_down_pressed_, rotate_left_pressed_, rotate_right_pressed_;
	bool rotate_clockwise_pressed_, rotate_anticlockwise_pressed_;
};

inline const float* mx_Player::Pos() const
{
	return pos_;
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