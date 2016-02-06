#pragma once

#include "glsl_program.h"
#include "vertex_buffer.h"

class mx_Level;
class mx_MainLoop;
class mx_Player;

class mx_Renderer
{
public:
	mx_Renderer( const mx_Level& level, const mx_Player& player );
	~mx_Renderer();

	void Draw();

private:
	mx_Renderer(const mx_Renderer&);
	mx_Renderer& operator=(const mx_Renderer&);

	void CreateGBuffer();

	void CalculateMatrices();
	void DrawWorld();
	void DrawMonsters();
	void DrawBullets();

private:
	const mx_MainLoop& main_loop_;
	const mx_Level& level_;
	const mx_Player& player_;

	mx_GLSLProgram world_shader_;
	mx_VertexBuffer world_vertex_buffer_;

	mx_VertexBuffer model_vertex_buffer_;

	mx_GLSLProgram plasma_ball_shader_;
	mx_VertexBuffer plasma_balls_vertex_buffer_;

	float perspective_matrix_[16];
	float view_matrix_[16];

	GLuint tex_id_;

	struct
	{
		GLuint fbo_id;

		GLuint albedo_tex_id;
		GLuint normals_tex_id;
		GLuint depth_tex_id;
	} g_buffer_;

	mx_GLSLProgram postprocessing_shader_;
	mx_VertexBuffer light_source_vertex_buffer_;
};