#pragma once

// 32-byte GPU vertex
struct mx_DrawingModelVertex
{
	float pos[3];
	float normal[3];
	float tex_coord[2];
};

class mx_DrawingModel
{
public:
	mx_DrawingModel();
	~mx_DrawingModel();

	const mx_DrawingModelVertex* GetVertexData() const;
	const unsigned short* GetIndexData() const;
	unsigned int GetVertexCount() const;
	unsigned int GetIndexCount() const;

	void SetVertexData( mx_DrawingModelVertex* vertices, unsigned int vertex_count );
	void SetIndexData( unsigned short* indeces, unsigned int index_count );

	void Copy( const mx_DrawingModel* m );

	void LoadFromMFMD( const unsigned char* model_data );

	void Add( const mx_DrawingModel* m );

	void Scale( const float* scale_vec );
	void Scale( float s );
	void Shift( const float* shift_vec );
	void Rotate( const float* normal_mat );

	void ScaleTexCoord( const float* scale_vec );
	void ShiftTexCoord( const float* shift_vec );
	void TransformTexCoord( const float* transform_mat3x3 );

	void FlipTriangles();

	void CalculateBoundingBox();
	// This parameters valid only after CalculateBoundingBox() call
	const float* BoundingBoxMin() const;
	const float* BoundingBoxMax() const;
	const float* BoundingSphereCenter() const;
	float BoundingSphereRadius() const;

	bool BeamIntersectModel( const float* beam_point, const float* beam_dir, float max_distance, float* out_pos_opt ) const;

private:
	mx_DrawingModelVertex* vertices_;
	unsigned short* indeces_;
	unsigned int vertex_count_;
	unsigned int index_count_;

	float bounding_box_min_[3];
	float bounding_box_max_[3];
	float bounding_sphere_center_[3];
	float bounding_sphere_radius_;
};

inline const mx_DrawingModelVertex* mx_DrawingModel::GetVertexData() const
{
	return vertices_;
}

inline const unsigned short* mx_DrawingModel::GetIndexData() const
{
	return indeces_;
}

inline unsigned int mx_DrawingModel::GetVertexCount() const
{
	return vertex_count_;
}

inline unsigned int mx_DrawingModel::GetIndexCount() const
{
	return index_count_;
}

inline const float* mx_DrawingModel::BoundingBoxMin() const
{
	return bounding_box_min_;
}

inline const float* mx_DrawingModel::BoundingBoxMax() const
{
	return bounding_box_max_;
}

inline const float* mx_DrawingModel::BoundingSphereCenter() const
{
	return bounding_sphere_center_;
}

inline float mx_DrawingModel::BoundingSphereRadius() const
{
	return bounding_sphere_radius_;
}