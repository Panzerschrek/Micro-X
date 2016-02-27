#pragma once

void mxMonochromeImageTo8Bit( const unsigned char* in_data, unsigned char* out_data, unsigned int out_data_size );
void mx8BitImageToWhiteWithAlpha( const unsigned char* in_data, unsigned char* out_data, unsigned int out_data_size );

class mx_Texture
{
public:
	mx_Texture( unsigned int size_x_log2, unsigned int size_y_log2 );
	~mx_Texture();

	const float* GetData() const;
	float* GetData();
	const unsigned char* GetNormalizedData() const;
	unsigned int SizeX() const;
	unsigned int SizeY() const;
	unsigned int SizeXLog2() const;
	unsigned int SizeYLog2() const;

	// All input colors - 4float vectors

	void Noise( unsigned int seed= 0, unsigned int octave_count= 8 );

	// Generates cellular texture, based on Poisson disk points.
	// R - distance to neraest point
	// G - distance to nearest cell border
	// B - distance to second nearest point
	// A - number of nearest point
	// param min_distanse_div_sqrt2 - minimal distance between points / sqrt(2)
	void PoissonDiskPoints( unsigned int min_distanse_div_sqrt2, unsigned int rand_seed= 0 );

	void GenHexagonalGrid( float edge_size, float y_scaler );
	void GenNormalMap(); // takes heightmap in alpha channel and writes d/dx in r-channel and d/dy in g-channel
	void Gradient( unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, const float* color0, const float* color1 );
	void RadialGradient( int center_x, int center_y, int radius, const float* color0, const float* color1 );
	void Fill( const float* color );
	void FillRect( unsigned int x, unsigned int y, unsigned int width, unsigned int height, const float* color );
	void FillEllipse( int center_x, int center_y, int radius, const float* color, float scale_x= 1.0f, float scale_y= 1.0f );
	void DrawLine( unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, const float* color );
	void Grayscale();
	void Smooth();
	void Abs();
	void SinWaveDeformX( float amplitude, float freq, float phase );
	void SinWaveDeformY( float amplitude, float freq, float phase );
	void DeformX( float dx_dy );
	void DeformY( float dy_dx );
	void DownscaleX();
	void DownscaleY();
	void FlipX();
	void FlipY();

	void FillTriangle( const float* xy_coords, const float* color );

	void Copy( const mx_Texture* t );
	void CopyRect( const mx_Texture* src, unsigned int width, unsigned int height, unsigned int x_dst, unsigned int y_dst, unsigned int x_src, unsigned int y_src );
	void Rotate( float deg );
	void Shift( unsigned int dx, unsigned int dy );
	void Invert( const float* add_color );
	void Add( const mx_Texture* t );
	void Sub( const mx_Texture* t );
	void Mul( const mx_Texture* t );
	void Max( const mx_Texture* t );
	void Min( const mx_Texture* t );

	void Add( const float* color );
	void Sub( const float* color );
	void Mul( const float* color );
	void Max( const float* color );
	void Min( const float* color );
	void Pow( float p );
	void Mod( const float* mod_color );

	void Mix( const float* color0, const float* color1, const float* sub_color );
	void AlphaBlendSrc( const mx_Texture* t );
	void AlphaBlendDst( const mx_Texture* t );
	void AlphaBlendOneMinusSrc( const mx_Texture* t );
	void AlphaBlendOneMinusDst( const mx_Texture* t );

	//void DrawText( unsigned int x, unsigned int y, unsigned int size, const float* color, const char* text );

	void LinearNormalization( float k );
	void ExpNormalization( float k );

private:
	//mx_Texture( const mx_Texture& );
	//mx_Texture& operator=( const mx_Texture& );

	unsigned int Noise2( unsigned int x, unsigned int y, unsigned int seed, unsigned int mask );
	unsigned int InterpolatedNoise( unsigned int x, unsigned int y, unsigned int seed, unsigned int k );
	unsigned int FinalNoise( unsigned int x, unsigned int y, unsigned int seed, unsigned int octave_count );

	void AllocateNormalizedData();

	void FillTrianglePart( float y_begin, float y_end, float x_left0, float x_left1, float x_right0, float x_right1, const float* color );

private:
	float* data_;
	unsigned char* normalized_data_;
	unsigned int size_[2];
	unsigned int size_log2_[2];
};

inline const float* mx_Texture::GetData() const
{
	return data_;
}

inline float* mx_Texture::GetData()
{
	return data_;
}

inline const unsigned char* mx_Texture::GetNormalizedData() const
{
	return normalized_data_;
}

inline unsigned int mx_Texture::SizeX() const
{
	return size_[0];
}

inline unsigned int mx_Texture::SizeY() const
{
	return size_[1];
}

inline unsigned int mx_Texture::SizeXLog2() const
{
	return size_log2_[0];
}

inline unsigned int mx_Texture::SizeYLog2() const
{
	return size_log2_[1];
}