#pragma once

class mx_Monster
{
public:
	mx_Monster( const float* pos );
	~mx_Monster();

	const float* Pos() const;

private:
	mx_Monster(const mx_Monster&);
	mx_Monster& operator=(const mx_Monster&);

private:
	float pos_[3];
};

inline const float* mx_Monster::Pos() const
{
	return pos_;
}