#pragma once

class mx_Pawn
{
public:
	mx_Pawn( int health );
	~mx_Pawn();

	const float* Pos() const;
	void CreateRotationMatrix4( float* out_mat, bool invert ) const;

	int GetHealth() const;
	void Hit( int damage );

protected:
	// Correct axis after it transformations ( make it orthogonal and with identity length )
	void CorrectAxis();

protected:
	float pos_[3];
	float axis_[3][3];

	int health_;
};

inline const float* mx_Pawn::Pos() const
{
	return pos_;
}

inline int mx_Pawn::GetHealth() const
{
	return health_;
}

inline void mx_Pawn::Hit( int damage )
{
	health_-= damage;
	if( health_ < 0 ) health_= 0;
}
