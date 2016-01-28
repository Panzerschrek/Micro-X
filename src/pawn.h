#pragma once

class mx_Pawn
{
public:
	mx_Pawn();
	~mx_Pawn();

	const float* Pos() const;
	//const float* Angle() const;
	void CreateRotationMatrix4( float* out_mat, bool invert ) const;

protected:
	// Correct axis after it transformations ( make it orthogonal and with identity length )
	void CorrectAxis();

protected:
	float pos_[3];

	float axis_[3][3];
};

inline const float* mx_Pawn::Pos() const
{
	return pos_;
}