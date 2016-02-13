#pragma once

#include "drawing_model.h"
#include "fwd.h"
#include "glsl_program.h"
#include "level_generator.h"
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

	void CalculateMatrices();
	void DrawWorld();
	void DrawMonsters();
	void DrawBullets();

	void MakeLighting();
	void DrawLightSource( const mx_Light& light_source );
	void MakeTonemapping();

private:
	const mx_MainLoop& main_loop_;
	const mx_Level& level_;
	const mx_Player& player_;

	mx_GLSLProgram world_shader_;
	mx_VertexBuffer world_vertex_buffer_;

	mx_GLSLProgram monsters_shader_;
	mx_VertexBuffer monsters_vertex_buffer_;
	unsigned int monsters_models_first_index_[ LastMonster ];
	unsigned int monsters_models_index_count_[ LastMonster ];

	mx_GLSLProgram plasma_ball_shader_;
	mx_VertexBuffer plasma_balls_vertex_buffer_;

	float perspective_matrix_[16];
	float view_matrix_[16];
	float z_near_, z_far_;

	GLuint world_texture_array_;
	GLuint world_normal_maps_array_;
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
};