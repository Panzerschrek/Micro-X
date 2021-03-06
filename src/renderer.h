#pragma once

#include "drawing_model.h"
#include "fwd.h"
#include "glsl_program.h"
#include "level_generator.h"
#include "models.h"
#include "textures_generation.h"
#include "vertex_buffer.h"

class mx_Renderer
{
public:
	mx_Renderer( const mx_Level& level, const mx_Player& player );
	~mx_Renderer();

	void OnFramebufferResize();

	void Draw();

private:
	mx_Renderer(const mx_Renderer&);
	mx_Renderer& operator=(const mx_Renderer&);

	void CreateScreenBuffers();

	void MarkPotentialyVisibleSectors();

	void DrawMap();
	void CalculateMatrices();
	void DrawWorld();

	void DrawModels();
	void DrawModel( mx_Models::Model model_index, ModelTexture texture_index );
	void DrawMonsters();
	void DrawAmmo();
	void DrawIcosahedrons();
	void DrawHealthPacks();
	void DrawParticles();

	void MakeLighting();
	void DrawLightSource( const mx_Light& light_source );
	void MakeTonemapping();

	void DrawGui();

	void MakePowerupsRotationMatrix( float* out_mat, const float* pos );

private:
	const mx_MainLoop& main_loop_;
	const mx_Level& level_;
	const mx_Player& player_;

	mx_GLSLProgram world_shader_;
	mx_GLSLProgram world_map_shader_;
	mx_VertexBuffer world_vertex_buffer_;

	mx_GLSLProgram models_shader_;

	mx_VertexBuffer models_vertex_buffer_;
	unsigned int models_first_index_[ mx_Models::LastModel ];
	unsigned int models_index_count_[ mx_Models::LastModel ];

	mx_GLSLProgram particles_shader_;
	mx_VertexBuffer particles_vertex_buffer_;

	mx_GLSLProgram gui_shader_;
	mx_VertexBuffer gui_vertex_buffer_;

	float perspective_matrix_[16];
	float view_matrix_[16];
	float z_near_, z_far_;

	GLuint world_texture_array_;
	GLuint world_normal_maps_array_;

	// HACK. Here placed, also, ammo boxes textures
	GLuint monsters_textures_array_id_;

	bool screen_buffers_initialized_;
	struct
	{
		GLuint fbo_id;

		GLuint albedo_tex_id;
		GLuint normals_tex_id;
		GLuint depth_tex_id;
	} g_buffer_;

	struct
	{
		GLuint fbo_id;
		GLuint color_texture_id;
		GLuint depth_tex_id;
	} hdr_buffer_;


	mx_GLSLProgram fullscreen_postprocessing_shader_;
	mx_GLSLProgram postprocessing_shader_;

	mx_GLSLProgram tonemapping_shader_;

	mx_DrawingModel light_source_model_;
	mx_VertexBuffer light_source_vertex_buffer_;

	unsigned int visible_sectors_tag_;
};