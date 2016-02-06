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

static void GenFullscreenQuad( mx_DrawingModel* model )
{
	// TODO - optimize?

	mx_DrawingModelVertex* vertices= new mx_DrawingModelVertex[4];
	unsigned short* indeces= new unsigned short[6];

	vertices[0].pos[0]= -1.0f;
	vertices[0].pos[1]= -1.0f;
	vertices[0].pos[2]=  0.0f;

	vertices[1].pos[0]= -1.0f;
	vertices[1].pos[1]= +1.0f;
	vertices[1].pos[2]=  0.0f;

	vertices[2].pos[0]= +1.0f;
	vertices[2].pos[1]= +1.0f;
	vertices[2].pos[2]=  0.0f;

	vertices[3].pos[0]= +1.0f;
	vertices[3].pos[1]= -1.0f;
	vertices[3].pos[2]=  0.0f;

	indeces[0]= 0;
	indeces[1]= 2;
	indeces[2]= 1;
	indeces[3]= 0;
	indeces[4]= 3;
	indeces[5]= 2;

	model->SetVertexData( vertices, 4 );
	model->SetIndexData( indeces, 6 );
}

mx_Renderer::mx_Renderer( const mx_Level& level, const mx_Player& player )
	: main_loop_(*mx_MainLoop::Instance())
	, level_(level)
	, player_(player)
{
	g_buffer_.fbo_id= 0;

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
	{ // fullscreen postprocessing shader
		fullscreen_postprocessing_shader_.Create(
			mx_Shaders::fullscreen_postprocessing_shader_v,
			mx_Shaders::fullscreen_postprocessing_shader_f );

		static const char* const uniforms[]= { "atex", "dtex" };
		fullscreen_postprocessing_shader_.FindUniforms( uniforms, sizeof(uniforms) / sizeof(char*) );
	}
	{ // postprocessing shader
		postprocessing_shader_.SetAttribLocation( "p", 0 );

		postprocessing_shader_.Create( mx_Shaders::postprocessing_shader_v, mx_Shaders::postprocessing_shader_f );
		static const char* const uniforms[]=
		{
			/*vertex*/
			"mat",
			/*fragment*/
			"atex", "ntex", "dtex", "lp", "lsb", "lc", "imat", "m10", "m14"
		};
		postprocessing_shader_.FindUniforms( uniforms, sizeof(uniforms) / sizeof(char*) );
	}
	{ // light sorce vertex buffer
		light_source_model_.LoadFromMFMD( mx_Models::icosahedron );

		// HACK. Put to model data fullscreen quad
		mx_DrawingModel fullscreen_quad_model;
		GenFullscreenQuad( &fullscreen_quad_model );
		light_source_model_.Add( &fullscreen_quad_model );

		light_source_vertex_buffer_.VertexData(
			light_source_model_.GetVertexData(),
			light_source_model_.GetVertexCount() * sizeof(mx_DrawingModelVertex),
			sizeof(mx_DrawingModelVertex) );

		light_source_vertex_buffer_.IndexData( light_source_model_.GetIndexData(), light_source_model_.GetIndexCount() * sizeof(unsigned short) );

		mx_DrawingModelVertex v;
		light_source_vertex_buffer_.VertexAttrib( 0, 3, GL_FLOAT, false, ((char*)v.pos) - ((char*)&v) );
	}

	CreateGBuffer();
}

mx_Renderer::~mx_Renderer()
{
}

void mx_Renderer::OnFramebufferResize()
{
	CreateGBuffer();
}

void mx_Renderer::Draw()
{
	CalculateMatrices();

	glBindFramebuffer( GL_FRAMEBUFFER, g_buffer_.fbo_id );
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	DrawWorld();
	DrawMonsters();

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	MakeLighting();

	DrawBullets();
}

void mx_Renderer::CreateGBuffer()
{
	mx_MainLoop* main_loop = mx_MainLoop::Instance();
	unsigned int width = main_loop->ViewportWidth ();
	unsigned int height= main_loop->ViewportHeight();

	if( g_buffer_.fbo_id != 0 )
	{
		glDeleteFramebuffers( 1, &g_buffer_.fbo_id );
		glDeleteTextures( 1, &g_buffer_.albedo_tex_id );
		glDeleteTextures( 1, &g_buffer_.normals_tex_id );
		glDeleteTextures( 1, &g_buffer_.depth_tex_id );
	}

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

	static const GLenum bufs[2]= { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers( 2, bufs );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void mx_Renderer::CalculateMatrices()
{
	float translate_mat[16];
	float rotation_mat[16];
	float basis_change_mat[16];

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

	z_near_= 1.0f / 16.0f;
	z_far_= 128.0f;
	mxMat4Perspective( perspective_matrix_,
		float(main_loop_.ViewportWidth())/ float(main_loop_.ViewportHeight()),
		player_.Fov(),
		z_near_, z_far_ );
	
	mxMat4Mul( translate_mat, rotation_mat, view_matrix_ );
	mxMat4Mul( view_matrix_, basis_change_mat );
	mxMat4Mul( view_matrix_, perspective_matrix_ );
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

void mx_Renderer::MakeLighting()
{
	glEnable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ONE ); // make light accumulation

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, g_buffer_.albedo_tex_id );
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, g_buffer_.normals_tex_id );
	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_2D, g_buffer_.depth_tex_id );

	// Fill depth buffer and setup ambient light
	fullscreen_postprocessing_shader_.Bind();
	fullscreen_postprocessing_shader_.UniformInt( "atex", 0 );
	fullscreen_postprocessing_shader_.UniformInt( "dtex", 2 );

	glDrawArrays( GL_TRIANGLES, 0, 6 );

	glDepthMask( 0 );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	
	postprocessing_shader_.Bind();

	postprocessing_shader_.UniformInt( "atex", 0 );
	postprocessing_shader_.UniformInt( "ntex", 1 );
	postprocessing_shader_.UniformInt( "dtex", 2 );

	float invert_view_mtrix[16];
	mxMat4Invert( view_matrix_, invert_view_mtrix );
	postprocessing_shader_.UniformMat4( "imat", invert_view_mtrix );

	postprocessing_shader_.UniformFloat( "m10", perspective_matrix_[10] );
	postprocessing_shader_.UniformFloat( "m14", perspective_matrix_[14] );

	light_source_vertex_buffer_.Bind();

	const mx_LevelData& level_data= level_.GetLevelData();
	for( unsigned int s= 0; s < level_data.sector_count; s++ )
		for( unsigned int l= 0; l < level_data.sectors[s].light_count; l++ )
			DrawLightSource( level_data.sectors[s].lights[l] );

	const mx_Bullet* bullets= level_.GetBullets();
	unsigned int bullet_count= level_.GetBulletCount();
	for( unsigned int b= 0; b < bullet_count; b++ )
	{
		mx_Light light_source;
		VEC3_CPY( light_source.pos, bullets[b].pos );
		light_source.light_rgb[0]= 0.4f;
		light_source.light_rgb[1]= 0.3f;
		light_source.light_rgb[2]= 0.1f;
		DrawLightSource( light_source );
	}

	glDepthMask( 1 );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );
}

void mx_Renderer::DrawLightSource( const mx_Light& light_source )
{
	float scale_mat[16];
	float translate_mat[16];
	float final_mat[16];

	float max_light= 0.0f;
	for( unsigned int i= 0; i < 3; i++ )
	{
		if( light_source.light_rgb[i] > max_light ) max_light= light_source.light_rgb[i];
	}

	static const float c_min_valuable_light= 1.0f / 64.0f;
	float min_light_distance= std::sqrt( max_light / c_min_valuable_light );

	postprocessing_shader_.UniformVec3( "lp", light_source.pos );
	postprocessing_shader_.UniformVec4( "lc", light_source.light_rgb );
	postprocessing_shader_.UniformFloat( "lsb", c_min_valuable_light );
	
	if( mxSquareDistance( light_source.pos, player_.Pos() ) >= min_light_distance * min_light_distance )
	{
		mxMat4Scale( scale_mat, min_light_distance );
		mxMat4Translate( translate_mat, light_source.pos );
		mxMat4Mul( scale_mat, translate_mat, final_mat );
		mxMat4Mul( final_mat, view_matrix_ );
		postprocessing_shader_.UniformMat4( "mat", final_mat );

		// Draw all model except fullscreen quad
		glDrawElements(
			GL_TRIANGLES,
			light_source_vertex_buffer_.IndexDataSize() / sizeof(unsigned short) - 6,
			GL_UNSIGNED_SHORT,
			NULL );
	}
	else
	{
		float translate_vec[3]= { 0.0f, 0.0f, -0.999f }; // place fullscreen quad at z_near_
		mxMat4Translate( final_mat, translate_vec );

		postprocessing_shader_.UniformMat4( "mat", final_mat );

		// Draw fullscreen quad, placed in model
		glDrawElements(
			GL_TRIANGLES,
			6,
			GL_UNSIGNED_SHORT,
			(void*)( light_source_vertex_buffer_.IndexDataSize() - 6 * sizeof(unsigned short) ) );
	}
}