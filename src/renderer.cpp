#include "drawing_model.h"
#include "level.h"
#include "main_loop.h"
#include "monster.h"
#include "mx_math.h"
#include "player.h"
#include "shaders.h"

#include "renderer.h"

static const unsigned char g_test_model_data[]=
#include "../models/robot.h"
;

mx_Renderer::mx_Renderer( const mx_Level& level, const mx_Player& player )
	: main_loop_(*mx_MainLoop::Instance())
	, level_(level)
	, player_(player)
{
	{ // World geometry
		world_vertex_buffer_.VertexData(
			level_.GetVertices(),
			sizeof(mx_LevelVertex) * level_.GetVertexCount(),
			sizeof(mx_LevelVertex) );
		world_vertex_buffer_.IndexData(
			level_.GetTriangles(),
			level_.GetTriangleCount() * sizeof(unsigned int) * 3 );

		mx_LevelVertex v;
		world_vertex_buffer_.VertexAttrib( 0, 3, GL_FLOAT, false, ((char*)v.xyz) - ((char*)&v) );
		world_vertex_buffer_.VertexAttrib( 1, 3, GL_BYTE, true, ((char*)v.normal) - ((char*)&v) );
		world_vertex_buffer_.VertexAttrib( 2, 3, GL_FLOAT, false, ((char*)v.tex_coord) - ((char*)&v) );
	}
	{ // World shader
		world_shader_.SetAttribLocation( "p", 0 );
		world_shader_.SetAttribLocation( "n", 1 );
		world_shader_.SetAttribLocation( "tc", 2 );
		world_shader_.Create( mx_Shaders::world_shader_v, mx_Shaders::world_shader_f );
		static const char* const uniforms[]= { "mat" };
		world_shader_.FindUniforms( uniforms, sizeof(uniforms) / sizeof(char*) );
	}

	{
		mx_DrawingModel model;
		model.LoadFromMFMD( g_test_model_data );

		model_vertex_buffer_.VertexData(
			model.GetVertexData(),
			model.GetVertexCount() * sizeof(mx_DrawingModelVertex),
			sizeof(mx_DrawingModelVertex) );

		model_vertex_buffer_.IndexData( model.GetIndexData(), model.GetIndexCount() * sizeof(unsigned short) );

		mx_DrawingModelVertex v;
		model_vertex_buffer_.VertexAttrib( 0, 3, GL_FLOAT, false, ((char*)v.pos) - ((char*)&v) );
		model_vertex_buffer_.VertexAttrib( 1, 3, GL_FLOAT, true, ((char*)v.normal) - ((char*)&v) );
		model_vertex_buffer_.VertexAttrib( 2, 3, GL_FLOAT, false, ((char*)v.tex_coord) - ((char*)&v) );
	}
}

mx_Renderer::~mx_Renderer()
{
}

void mx_Renderer::Draw()
{
	CalculateMatrices();
	DrawWorld();
	DrawMonsters();
	DrawBullets();
}

void mx_Renderer::CalculateMatrices()
{
	float translate_mat[16];
	float rotation_mat[16];
	float basis_change_mat[16];
	float perspective_mat[16];

	float translate_vec[3];
	mxVec3Mul( player_.Pos(), -1.0f, translate_vec );
	mxMat4Translate( translate_mat, translate_vec );

	player_.CreateRotationMatrix4( rotation_mat, false );

	{
		mxMat4RotateX( basis_change_mat, -MX_PI2 );
		float tmp_mat[16];
		mxMat4Identity( tmp_mat );
		tmp_mat[10]= -1.0f;
		mxMat4Mul( basis_change_mat, tmp_mat );
	}

	mxMat4Perspective( perspective_mat,
		float(main_loop_.ViewportWidth())/ float(main_loop_.ViewportHeight()),
		player_.Fov(),
		1.0f / 16.0f, 128.0f );
	
	mxMat4Mul( translate_mat, rotation_mat, view_matrix_ );
	mxMat4Mul( view_matrix_, basis_change_mat );
	mxMat4Mul( view_matrix_, perspective_mat );
}

void mx_Renderer::DrawWorld()
{
	world_shader_.Bind();
	world_shader_.UniformMat4( "mat", view_matrix_ );

	world_vertex_buffer_.Bind();
	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );
	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glDrawElements( GL_TRIANGLES, world_vertex_buffer_.IndexDataSize() / sizeof(unsigned int), GL_UNSIGNED_INT, NULL );
}

void mx_Renderer::DrawMonsters()
{
	world_shader_.Bind();

	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	model_vertex_buffer_.Bind();

	const mx_Monster* const* monsters= level_.GetMonsters();
	for( unsigned int m= 0, m_end= level_.GetMonsterCount(); m < m_end; m++ )
	{
		float scale_mat[16];
		float rotate_mat[16];
		float translate_mat[16];
		float result_mat[16];

		mxMat4Scale( scale_mat, 0.125f );
		mxMat4Translate( translate_mat, monsters[m]->Pos() );
		monsters[m]->CreateRotationMatrix4( rotate_mat, true );
		mxMat4Mul( scale_mat, rotate_mat, result_mat );
		mxMat4Mul( result_mat, translate_mat );
		mxMat4Mul( result_mat, view_matrix_ );

		world_shader_.UniformMat4( "mat", result_mat );

		glDrawElements( GL_TRIANGLES, model_vertex_buffer_.IndexDataSize() / sizeof(unsigned short), GL_UNSIGNED_SHORT, NULL );
	}
}

void mx_Renderer::DrawBullets()
{
	// Test drawing of bullets using monster model
	world_shader_.Bind();

	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	model_vertex_buffer_.Bind();

	const mx_Bullet* bullets= level_.GetBullets();
	for( unsigned int b= 0, b_end= level_.GetBulletCount(); b < b_end; b++ )
	{
		float scale_mat[16];
		float translate_mat[16];
		float result_mat[16];

		mxMat4Scale( scale_mat, 1.0f / 32.0f );
		mxMat4Translate( translate_mat, bullets[b].pos );
		mxMat4Mul( scale_mat, translate_mat, result_mat );
		mxMat4Mul( result_mat, view_matrix_ );

		world_shader_.UniformMat4( "mat", result_mat );

		glDrawElements( GL_TRIANGLES, model_vertex_buffer_.IndexDataSize() / sizeof(unsigned short), GL_UNSIGNED_SHORT, NULL );
	}
}