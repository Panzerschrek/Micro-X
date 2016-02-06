#include "drawing_model.h"
#include "level.h"
#include "main_loop.h"
#include "models.h"
#include "monster.h"
#include "mx_math.h"
#include "player.h"
#include "shaders.h"
#include "texture.h"
#include "textures_generation.h"

#include "renderer.h"

static void SetupFBOTextureParameters()
{
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

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
		world_shader_.SetFragDataLocation( "c_", 0 );
		world_shader_.SetFragDataLocation( "n_", 1 );

		world_shader_.Create( mx_Shaders::world_shader_v, mx_Shaders::world_shader_f );
		static const char* const uniforms[]= { "mat", "tex" };
		world_shader_.FindUniforms( uniforms, sizeof(uniforms) / sizeof(char*) );
	}

	{ // plasma ball shader
		plasma_ball_shader_.SetAttribLocation( "p", 0 );
		plasma_ball_shader_.Create( mx_Shaders::plasma_ball_shader_v, mx_Shaders::plasma_ball_shader_f );
		static const char* const uniforms[]= { "mat" };
		plasma_ball_shader_.FindUniforms( uniforms, sizeof(uniforms) / sizeof(char*) );
	}

	{ // plasma ball vbo
		plasma_balls_vertex_buffer_.VertexData( NULL, MX_MAX_BULLETS * sizeof(float) * 3, sizeof(float) * 3 );
		plasma_balls_vertex_buffer_.VertexAttrib( 0, 3, GL_FLOAT, false, 0 );
	}

	{
		mx_DrawingModel model;
		model.LoadFromMFMD( mx_Models::monsters_models[0] );
		model.Scale( mx_Models::monsters_models_scale[0] );

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
	{
		mx_Texture texture( 10, 10 );
		mxGenGraniteTexture( &texture );
		mxGenSteelPlateTexture( &texture );

		glGenTextures( 1, &tex_id_ );
		glBindTexture( GL_TEXTURE_2D, tex_id_ );

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			texture.SizeX(), texture.SizeY(), 0,
			GL_RGBA, GL_UNSIGNED_BYTE, texture.GetNormalizedData() );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D );
	}
	{ // postprocessing shader
		postprocessing_shader_.SetAttribLocation( "p", 0 );
		//postprocessing_shader_.SetAttribLocation( "n", 1 );
		//postprocessing_shader_.SetAttribLocation( "tc", 2 );

		postprocessing_shader_.Create( mx_Shaders::postprocessing_shader_v, mx_Shaders::postprocessing_shader_f );
		static const char* const uniforms[]= { "mat", "tex" };
		postprocessing_shader_.FindUniforms( uniforms, sizeof(uniforms) / sizeof(char*) );
	}
	{ // light sorce vertex buffer
		mx_DrawingModel model;
		model.LoadFromMFMD( mx_Models::icosahedron );

		light_source_vertex_buffer_.VertexData(
			model.GetVertexData(),
			model.GetVertexCount() * sizeof(mx_DrawingModelVertex),
			sizeof(mx_DrawingModelVertex) );

		light_source_vertex_buffer_.IndexData( model.GetIndexData(), model.GetIndexCount() * sizeof(unsigned short) );

		mx_DrawingModelVertex v;
		light_source_vertex_buffer_.VertexAttrib( 0, 3, GL_FLOAT, false, ((char*)v.pos) - ((char*)&v) );
		//light_source_vertex_buffer_.VertexAttrib( 1, 3, GL_FLOAT, true, ((char*)v.normal) - ((char*)&v) );
		//light_source_vertex_buffer_.VertexAttrib( 2, 3, GL_FLOAT, false, ((char*)v.tex_coord) - ((char*)&v) );
	}

	CreateGBuffer();
}

mx_Renderer::~mx_Renderer()
{
}

void mx_Renderer::Draw()
{
	CalculateMatrices();

	glBindFramebuffer( GL_FRAMEBUFFER, g_buffer_.fbo_id );
	glClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	DrawWorld();
	DrawMonsters();
	DrawBullets();

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, g_buffer_.albedo_tex_id );

	postprocessing_shader_.Bind();
	postprocessing_shader_.UniformInt( "tex", 0 );
	postprocessing_shader_.UniformMat4( "mat", view_matrix_ );

	light_source_vertex_buffer_.Bind();
	glDrawElements( GL_TRIANGLES, light_source_vertex_buffer_.IndexDataSize() / sizeof(unsigned short), GL_UNSIGNED_SHORT, NULL );

}

void mx_Renderer::CreateGBuffer()
{
	mx_MainLoop* main_loop = mx_MainLoop::Instance();
	unsigned int width = main_loop->ViewportWidth ();
	unsigned int height= main_loop->ViewportHeight();

	// albedo and normals textures texture
	for( unsigned int i= 0; i < 2; i++ )
	{
		unsigned int& tex= i ==0 ? g_buffer_.albedo_tex_id : g_buffer_.normals_tex_id;

		glGenTextures( 1, &tex );
		glBindTexture( GL_TEXTURE_2D, tex );
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA8,
			width, height,
			0, GL_RGBA, GL_FLOAT, NULL );
		SetupFBOTextureParameters();
		
	}

	// depth texture
	glGenTextures( 1, &g_buffer_.depth_tex_id );
	glBindTexture( GL_TEXTURE_2D, g_buffer_.depth_tex_id );
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
		width, height,
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL );
	SetupFBOTextureParameters();

	glGenFramebuffers( 1, &g_buffer_.fbo_id );
	glBindFramebuffer( GL_FRAMEBUFFER, g_buffer_.fbo_id );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_buffer_.albedo_tex_id , 0 );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, g_buffer_.normals_tex_id, 0 );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, g_buffer_.depth_tex_id, 0 );

	GLenum bufs[2]= { g_buffer_.albedo_tex_id, g_buffer_.normals_tex_id };
	glDrawBuffers( 2, bufs );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
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
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, tex_id_ );

	world_shader_.Bind();
	world_shader_.UniformMat4( "mat", view_matrix_ );
	world_shader_.UniformInt( "tex", 0 );

	world_vertex_buffer_.Bind();
	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );
	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glDrawElements( GL_TRIANGLES, world_vertex_buffer_.IndexDataSize() / sizeof(unsigned int), GL_UNSIGNED_INT, NULL );

	glDisable (GL_CULL_FACE );
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
		float rotate_mat[16];
		float translate_mat[16];
		float result_mat[16];

		const mx_Monster* monster= monsters[m];

		mxMat4Translate( translate_mat, monster->Pos() );
		monster->CreateRotationMatrix4( rotate_mat, true );
		mxMat4Mul( rotate_mat, translate_mat, result_mat );
		mxMat4Mul( result_mat, view_matrix_ );

		world_shader_.UniformMat4( "mat", result_mat );

		glDrawElements( GL_TRIANGLES, model_vertex_buffer_.IndexDataSize() / sizeof(unsigned short), GL_UNSIGNED_SHORT, NULL );
	}

	glDisable (GL_CULL_FACE );
}

void mx_Renderer::DrawBullets()
{
	float bullets_pos[ MX_MAX_BULLETS * 3 ];

	const mx_Bullet* bullets= level_.GetBullets();
	unsigned int bullet_count= level_.GetBulletCount();
	for( unsigned int b= 0; b < bullet_count; b++ )
	{
		VEC3_CPY( bullets_pos + b * 3, bullets[b].pos );
	}

	plasma_balls_vertex_buffer_.VertexSubData( bullets_pos, bullet_count * 3 * sizeof(float), 0 );

	plasma_ball_shader_.Bind();
	plasma_ball_shader_.UniformMat4( "mat", view_matrix_ );

	glEnable( GL_PROGRAM_POINT_SIZE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDrawArrays( GL_POINTS, 0, bullet_count );

	glDisable( GL_BLEND );
	glDisable( GL_PROGRAM_POINT_SIZE );
}