#include "level.h"
#include "main_loop.h"
#include "models.h"
#include "monster.h"
#include "mx_assert.h"
#include "mx_math.h"
#include "particles_manager.h"
#include "player.h"
#include "shaders.h"
#include "texture.h"
#include "textures_generation.h"

#include "renderer.h"

#define MX_MAX_GUI_VERTICES 8192

struct GuiVertex
{
	short pos[2];
	unsigned char color[4];
};

static const float g_texture_aray_coord_eps= 0.1f;

static const float g_bullets_light_intensity[LastBullet]=
{
	0.05f,
	0.5f,
	0.15f,
};

static void CreateBasisChangeMatrix( float* mat )
{
	mxMat4RotateX( mat, -MX_PI2 );
	float tmp_mat[16];
	mxMat4Identity( tmp_mat );
	tmp_mat[10]= -1.0f;
	mxMat4Mul( mat, tmp_mat );
}

static void MarkSectors_r( mx_LevelSector* sector, unsigned int tag, unsigned int depth )
{
	sector->traverse_id= tag;

	if( depth == 0 )
		return;

	for( unsigned int i= 0; i < sector->connections_count; i++ )
	{
		if( sector->connections[i]->traverse_id != tag )
			MarkSectors_r( sector->connections[i], tag, depth - 1 );
	}
}

static GuiVertex* AddGuiQuad( GuiVertex* v, int x, int y, int width, int height, const unsigned char* color )
{
	v[0].pos[0]= short(x);
	v[0].pos[1]= short(y);
	v[1].pos[0]= short(x);
	v[1].pos[1]= short(y + height);
	v[2].pos[0]= short(x + width );
	v[2].pos[1]= short(y + height);
	v[3].pos[0]= short(x + width );
	v[3].pos[1]= short(y);

	for( unsigned int i= 0; i < 4; i++ )
	for( unsigned int j= 0; j < 4; j++ )
		v[i].color[j]= color[j];

	v[4]= v[0];
	v[5]= v[2];

	return v + 6;
}

static GuiVertex* AddGuiIcosahedronProjection( GuiVertex* v, int center_x, int center_y, int radius, const unsigned char* color )
{
	GuiVertex* v0= v;

	static const float k= MX_2PI / 6.0f;
	float radius_f= float(radius);

	for( unsigned int i= 0; i < 6; i++, v+= 3 )
	{
		for( unsigned int j= 0; j < 2; j++ )
		{
			float a= MX_PI6 + float(i+j) * k;
			v[j].pos[0]= short( center_x + int(mxRound( std::cosf(a) * radius_f )) );
			v[j].pos[1]= short( center_y + int(mxRound( std::sinf(a) * radius_f )) );
		}
		v[2].pos[0]= short(center_x);
		v[2].pos[1]= short(center_y);
	}

	const unsigned int vertex_count= 6 * 3;

	for( unsigned int i= 0; i < vertex_count; i++ )
	for( unsigned int j= 0; j < 4; j++ )
		v0[i].color[j]= color[j];

	return v;
}

static void FloatColorToByte( const float* float_color, unsigned char* byte_color )
{
	for( unsigned int i= 0; i < 3; i++ )
	{
		MX_ASSERT( float_color[i] >= 0.0f && float_color[i] <= 1.0f );
		byte_color[i]= (unsigned char)( float_color[i] * 255.0f );
	}
}

static void SetupFBOTextureParameters()
{
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

static void CreateDeptTexture( unsigned int width, unsigned int height, GLuint& tex )
{
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
		width, height,
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL );
	SetupFBOTextureParameters();
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
	, screen_buffers_initialized_(false)
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
		world_vertex_buffer_.VertexAttrib( 1, 3, GL_BYTE, true, ((char*)v.binormal) - ((char*)&v) );
		world_vertex_buffer_.VertexAttrib( 2, 3, GL_BYTE, true, ((char*)v.tangent) - ((char*)&v) );
		world_vertex_buffer_.VertexAttrib( 3, 3, GL_BYTE, true, ((char*)v.normal) - ((char*)&v) );
		world_vertex_buffer_.VertexAttrib( 4, 2, GL_FLOAT, false, ((char*)v.tex_coord) - ((char*)&v) );
		world_vertex_buffer_.VertexAttrib( 5, 1, GL_UNSIGNED_BYTE, false, ((char*)&v.tex_id) - ((char*)&v) );
	}
	{ // World shader
		world_shader_.SetAttribLocation( "p", 0 );
		world_shader_.SetAttribLocation( "b", 1 );
		world_shader_.SetAttribLocation( "t", 2 );
		world_shader_.SetAttribLocation( "n", 3 );
		world_shader_.SetAttribLocation( "tc", 4 );
		world_shader_.SetAttribLocation( "texn", 5 );
		world_shader_.SetFragDataLocation( "c_", 0 );
		world_shader_.SetFragDataLocation( "n_", 1 );

		world_shader_.Create( mx_Shaders::world_shader_v, mx_Shaders::world_shader_f );
		static const char* const uniforms[]= { "mat", "tex", "nmap" };
		world_shader_.FindUniforms( uniforms, sizeof(uniforms) / sizeof(char*) );
	}
	{ // World map shader
		world_map_shader_.SetAttribLocation( "p", 0 );

		world_map_shader_.Create( mx_Shaders::world_map_shader_v, mx_Shaders::world_map_shader_f );
		world_map_shader_.FindUniform( "mat" );
		world_map_shader_.FindUniform( "m10" );
	}
	{ // partices shader
		particles_shader_.SetAttribLocation( "p", 0 );
		particles_shader_.SetAttribLocation( "c", 1 );
		particles_shader_.Create( mx_Shaders::particles_shader_v, mx_Shaders::particles_shader_f );
		particles_shader_.FindUniform( "mat" );
		particles_shader_.FindUniform( "ss" );
	}
	{ // particles vbo
		particles_vertex_buffer_.VertexData( NULL, MX_MAX_PARTICLES * sizeof(mx_ParticleVertex), sizeof(mx_ParticleVertex) );

		mx_ParticleVertex v;
		particles_vertex_buffer_.VertexAttrib( 0, 4, GL_FLOAT, false, ((char*)v.pos_size) - ((char*)&v) );
		particles_vertex_buffer_.VertexAttrib( 1, 4, GL_FLOAT, false, ((char*)v.color) - ((char*)&v) );
	}
	{ // gui shader
		gui_shader_.SetAttribLocation( "p", 0 );
		gui_shader_.SetAttribLocation( "c", 1 );

		gui_shader_.Create( mx_Shaders::gui_shader_v, mx_Shaders::gui_shader_f );
		gui_shader_.FindUniform( "isz" );
	}
	{ // gui vbo
		gui_vertex_buffer_.VertexData( NULL, MX_MAX_GUI_VERTICES * sizeof(GuiVertex), sizeof(GuiVertex) );

		GuiVertex v;
		gui_vertex_buffer_.VertexAttrib( 0, 2, GL_SHORT, false, ((char*)v.pos) - ((char*)&v) );
		gui_vertex_buffer_.VertexAttrib( 1, 4, GL_UNSIGNED_BYTE, true, ((char*)v.color) - ((char*)&v) );
	}
	{ // monsters vbo
		mx_DrawingModel combined_model;

		for( unsigned int i= 0; i < LastMonster; i++ )
		{
			mx_DrawingModel model;
			model.LoadFromMFMD( mx_Models::monsters_models[i] );
			model.Scale( mx_Models::monsters_models_scale[i] );

			monsters_models_first_index_[i]= combined_model.GetIndexCount();
			monsters_models_index_count_[i]= model.GetIndexCount();

			combined_model.Add( &model );
		}
		for( unsigned int i= 0; i < 2; i++ )
		{
			static const unsigned char* const models[2]= { mx_Models::cube, mx_Models::icosahedron };
			static const float scale[2]= { 0.1f, 0.15f };
			mx_DrawingModel model;
			model.LoadFromMFMD( models[i] );
			model.Scale( scale[i] );

			monsters_models_first_index_[LastMonster + i]= combined_model.GetIndexCount();
			monsters_models_index_count_[LastMonster + i]= model.GetIndexCount();

			combined_model.Add( &model );
		}

		monsters_vertex_buffer_.VertexData(
			combined_model.GetVertexData(),
			combined_model.GetVertexCount() * sizeof(mx_DrawingModelVertex),
			sizeof(mx_DrawingModelVertex) );

		monsters_vertex_buffer_.IndexData( combined_model.GetIndexData(), combined_model.GetIndexCount() * sizeof(unsigned short) );

		mx_DrawingModelVertex v;
		monsters_vertex_buffer_.VertexAttrib( 0, 3, GL_FLOAT, false, ((char*)v.pos) - ((char*)&v) );
		monsters_vertex_buffer_.VertexAttrib( 1, 3, GL_FLOAT, true, ((char*)v.normal) - ((char*)&v) );
		monsters_vertex_buffer_.VertexAttrib( 2, 2, GL_FLOAT, false, ((char*)v.tex_coord) - ((char*)&v) );
	}
	{ // monsters shader
		monsters_shader_.SetAttribLocation( "p", 0 );
		monsters_shader_.SetAttribLocation( "n", 1 );
		monsters_shader_.SetAttribLocation( "tc", 2 );
		monsters_shader_.SetFragDataLocation( "c_", 0 );
		monsters_shader_.SetFragDataLocation( "n_", 1 );

		monsters_shader_.Create( mx_Shaders::monster_shader_v, mx_Shaders::monster_shader_f );
		static const char* const uniforms[]= { "mat", "nmat", "tex", "texn" };
		monsters_shader_.FindUniforms( uniforms, sizeof(uniforms) / sizeof(char*) );
	}
	{ // level textures
		mx_Texture tex( 10, 10 );

		for( unsigned int t= 0; t < 2; t++ )
		{
			GLuint& tex_id= t == 0 ? world_texture_array_ : world_normal_maps_array_;

			glGenTextures( 1, &tex_id );
			glBindTexture( GL_TEXTURE_2D_ARRAY, tex_id );

			glTexImage3D(
				GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
				tex.SizeX(), tex.SizeY(), LastLevelTexture,
				0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );

			for( unsigned int i= 0; i < LastLevelTexture; i++ )
			{
				if( t == 0 )
				{
					gen_level_textures_func_table[i]( &tex );
					tex.LinearNormalization( 1.0f );
				}
				else
				{
					gen_level_textures_height_map_func_table[i]( &tex );
					tex.GenNormalMap();

					// map from range [-1; 1] to range [0; 1]
					static const float k[4]= { 0.5f, 0.5f, 0.5f, 0.5f };
					tex.Mul(k);
					tex.Add(k);
					tex.LinearNormalization(1.0f);
				}

				glTexSubImage3D(
					GL_TEXTURE_2D_ARRAY, 0,
					0, 0, i,
					tex.SizeX(), tex.SizeY(), 1,
					GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
			}
			glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
		} // for diffuse and normal
	}
	{ // monsters and amo boxes textures
		mx_Texture tex( 9, 9 );
		glGenTextures( 1, &monsters_textures_array_id_ );
		glBindTexture( GL_TEXTURE_2D_ARRAY, monsters_textures_array_id_ );

		glTexImage3D(
			GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
			tex.SizeX(), tex.SizeY(), LastMonster + LastBullet + 1,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );

		for( unsigned int i= 0; i < LastMonster + LastBullet + 1; i++ )
		{
			if( i < LastMonster )
				gen_monsters_textures_func_table[i]( &tex );
			else if( i < LastMonster + LastBullet )
				gen_ammo_textures_func_table[ i - LastMonster ]( &tex );
			else
				mxGenIcosahedronTexture( &tex );

			tex.LinearNormalization( 1.0f );

			glTexSubImage3D(
				GL_TEXTURE_2D_ARRAY, 0,
				0, 0, i,
				tex.SizeX(), tex.SizeY(), 1,
				GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		}
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
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

	{ // tonemapping shader
		tonemapping_shader_.Create(
			mx_Shaders::tonemapping_shader_v,
			mx_Shaders::tonemapping_shader_f );

		tonemapping_shader_.FindUniform( "tex" );
	}

	CreateScreenBuffers();
}

mx_Renderer::~mx_Renderer()
{
}

void mx_Renderer::OnFramebufferResize()
{
	CreateScreenBuffers();
}

void mx_Renderer::Draw()
{
	if( player_.IsInMapMode() )
		DrawMap();
	else
	{
		MarkPotentialyVisibleSectors();
		CalculateMatrices();

		// Bind GBuffer. We do not need clear color
		glBindFramebuffer( GL_FRAMEBUFFER, g_buffer_.fbo_id );
		glClear( GL_DEPTH_BUFFER_BIT );

		DrawWorld();
		DrawMonsters();
		DrawAmmo();
		DrawIcosahedrons();

		// Bind HDR screen buffer. Clear cboth buffers.
		glBindFramebuffer( GL_FRAMEBUFFER, hdr_buffer_.fbo_id );
		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		MakeLighting();
		DrawParticles();

		// Bind screen framebuffer. We do non need clear it.
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

		MakeTonemapping();
	}

	DrawGui();
}

void mx_Renderer::CreateScreenBuffers()
{
	mx_MainLoop* main_loop = mx_MainLoop::Instance();
	unsigned int width = main_loop->ViewportWidth ();
	unsigned int height= main_loop->ViewportHeight();

	if( screen_buffers_initialized_ )
	{
		glDeleteFramebuffers( 1, &g_buffer_.fbo_id );
		glDeleteTextures( 1, &g_buffer_.albedo_tex_id );
		glDeleteTextures( 1, &g_buffer_.normals_tex_id );
		glDeleteTextures( 1, &g_buffer_.depth_tex_id );

		glDeleteFramebuffers( 1, &hdr_buffer_.fbo_id );
		glDeleteTextures( 1, &hdr_buffer_.color_texture_id );
		glDeleteTextures( 1, &hdr_buffer_.depth_tex_id );
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

	CreateDeptTexture( width, height, g_buffer_.depth_tex_id );

	glGenFramebuffers( 1, &g_buffer_.fbo_id );
	glBindFramebuffer( GL_FRAMEBUFFER, g_buffer_.fbo_id );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_buffer_.albedo_tex_id , 0 );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, g_buffer_.normals_tex_id, 0 );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, g_buffer_.depth_tex_id, 0 );

	static const GLenum g_bufs[2]= { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers( 2, g_bufs );

	// HDR screen buffer

	glGenTextures( 1, &hdr_buffer_.color_texture_id );
	glBindTexture( GL_TEXTURE_2D, hdr_buffer_.color_texture_id );
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA16F,
		width, height,
		0, GL_RGBA, GL_FLOAT, NULL );
	SetupFBOTextureParameters();

	CreateDeptTexture( width, height, hdr_buffer_.depth_tex_id );

	glGenFramebuffers( 1, &hdr_buffer_.fbo_id );
	glBindFramebuffer( GL_FRAMEBUFFER, hdr_buffer_.fbo_id );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, hdr_buffer_.color_texture_id, 0 );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, hdr_buffer_.depth_tex_id, 0 );

	GLenum hdr_buf= GL_COLOR_ATTACHMENT0;
	glDrawBuffers( 1, &hdr_buf );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void mx_Renderer::MarkPotentialyVisibleSectors()
{
	visible_sectors_tag_= mxGenSectorGraphTraverseId();

	mx_LevelSector* player_sector= const_cast<mx_LevelSector*>(player_.GetSector());
	if( player_sector )
	{
		// We assume, that player can not see through mor tnan "depth" sectors
		const unsigned int depth= 4;
		MarkSectors_r( player_sector, visible_sectors_tag_, depth );
	}
}

void mx_Renderer::DrawMap()
{
	float player_translate_mat[16];
	float rotation_mat[16];
	float cam_translate_mat[16];
	float basis_change_mat[16];

	float translate_vec[3];
	mxVec3Mul( player_.Pos(), -1.0f, translate_vec );
	mxMat4Translate( player_translate_mat, translate_vec );

	player_.CreateRotationMatrix4( rotation_mat, false );

	static const float cam_translate_vec[3]= { 0.0f, 0.0f, 4.0f };
	mxMat4Translate( cam_translate_mat, cam_translate_vec );

	CreateBasisChangeMatrix( basis_change_mat );

	z_near_= 1.0f / 16.0f;
	z_far_= 24.0f;
	mxMat4Perspective( perspective_matrix_,
		float(main_loop_.ViewportWidth())/ float(main_loop_.ViewportHeight()),
		player_.Fov(),
		z_near_, z_far_ );

	mxMat4Mul( player_translate_mat, rotation_mat, view_matrix_ );
	mxMat4Mul( view_matrix_, basis_change_mat );
	mxMat4Mul( view_matrix_, cam_translate_mat );
	mxMat4Mul( view_matrix_, perspective_matrix_ );

	world_map_shader_.Bind();
	world_map_shader_.UniformMat4( "mat", view_matrix_ );
	world_map_shader_.UniformFloat( "m10", perspective_matrix_[10] );

	world_vertex_buffer_.Bind();

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glDrawElements( GL_TRIANGLES, world_vertex_buffer_.IndexDataSize() / sizeof(unsigned int), GL_UNSIGNED_INT, NULL );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
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

	CreateBasisChangeMatrix( basis_change_mat );

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
	glBindTexture( GL_TEXTURE_2D_ARRAY, world_texture_array_ );

	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, world_normal_maps_array_ );

	world_shader_.Bind();
	world_shader_.UniformMat4( "mat", view_matrix_ );
	world_shader_.UniformInt( "tex", 0 );
	world_shader_.UniformInt( "nmap", 1 );

	world_vertex_buffer_.Bind();
	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );
	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glDrawElements( GL_TRIANGLES, world_vertex_buffer_.IndexDataSize() / sizeof(unsigned int), GL_UNSIGNED_INT, NULL );

	glDisable (GL_CULL_FACE );
}

void mx_Renderer::DrawMonsters()
{
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, monsters_textures_array_id_ );

	monsters_shader_.Bind();
	monsters_shader_.UniformInt( "tex", 0 );

	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	monsters_vertex_buffer_.Bind();

	const mx_Monster* const* monsters= level_.GetMonsters();
	for( unsigned int m= 0, m_end= level_.GetMonsterCount(); m < m_end; m++ )
	{
		float rotate_mat[16];
		float normals_mat[16];
		float translate_mat[16];
		float result_mat[16];

		const mx_Monster* monster= monsters[m];
		if( monster->GetSector().traverse_id != visible_sectors_tag_ )
			continue;

		mxMat4Translate( translate_mat, monster->Pos() );
		monster->CreateRotationMatrix4( rotate_mat, true );
		mxMat4Mul( rotate_mat, translate_mat, result_mat );
		mxMat4Mul( result_mat, view_matrix_ );

		monsters_shader_.UniformMat4( "mat", result_mat );

		mxMat4ToMat3( rotate_mat, normals_mat );
		monsters_shader_.UniformMat3( "nmat", normals_mat );

		monsters_shader_.UniformFloat( "texn", float(monster->GetType()) + g_texture_aray_coord_eps );

		glDrawElements(
			GL_TRIANGLES,
			monsters_models_index_count_[monster->GetType()],
			GL_UNSIGNED_SHORT,
			(void*)( monsters_models_first_index_[monster->GetType()] * sizeof(unsigned short) ) );
	}

	glDisable (GL_CULL_FACE );
}

void mx_Renderer::DrawAmmo()
{
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, monsters_textures_array_id_ );

	monsters_shader_.Bind();
	monsters_shader_.UniformInt( "tex", 0 );

	monsters_vertex_buffer_.Bind();

	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	const mx_LevelData& level_data= level_.GetLevelData();
	for( unsigned int s= 0; s < level_data.sector_count; s++ )
	{
		const mx_LevelSector& sector= level_data.sectors[s];
		if( sector.traverse_id != visible_sectors_tag_ )
			continue;

		for( unsigned int a= 0; a < sector.ammo_box_count; a++ )
		{
			const mx_AmmoBox& box= sector.ammo_boxes[a];

			float translate_mat[16];
			float rotate_mat[16];
			float result_mat[16];
			float normals_mat[9];

			MakePowerupsRotationMatrix( rotate_mat );

			mxMat4Translate( translate_mat, box.pos );
			mxMat4Mul( rotate_mat, translate_mat, result_mat );
			mxMat4Mul( result_mat, view_matrix_ );

			monsters_shader_.UniformMat4( "mat", result_mat );

			mxMat4ToMat3( rotate_mat, normals_mat );
			monsters_shader_.UniformMat3( "nmat", normals_mat );

			monsters_shader_.UniformFloat( "texn", float( LastMonster + box.type ) + g_texture_aray_coord_eps );

			glDrawElements(
				GL_TRIANGLES,
				monsters_models_index_count_[LastMonster],
				GL_UNSIGNED_SHORT,
				(void*)( monsters_models_first_index_[LastMonster] * sizeof(unsigned short) ) );
		} // for ammo boxes
	} // for sectors

	glDisable (GL_CULL_FACE );
}

void mx_Renderer::DrawIcosahedrons()
{
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, monsters_textures_array_id_ );

	monsters_shader_.Bind();
	monsters_shader_.UniformInt( "tex", 0 );

	monsters_vertex_buffer_.Bind();

	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	const mx_LevelData& level_data= level_.GetLevelData();
	for( unsigned int s= 0; s < level_data.sector_count; s++ )
	{
		const mx_LevelSector& sector= level_data.sectors[s];
		if( sector.traverse_id != visible_sectors_tag_ )
			continue;

		if( sector.has_icosahedron && !sector.icosahedron_picked )
		{
			float translate_mat[16];
			float rotate_mat[16];
			float result_mat[16];
			float normals_mat[9];

			MakePowerupsRotationMatrix( rotate_mat );

			mxMat4Translate( translate_mat, sector.icosahedron_pos );
			mxMat4Mul( rotate_mat, translate_mat, result_mat );
			mxMat4Mul( result_mat, view_matrix_ );

			monsters_shader_.UniformMat4( "mat", result_mat );

			mxMat4ToMat3( rotate_mat, normals_mat );
			monsters_shader_.UniformMat3( "nmat", normals_mat );

			monsters_shader_.UniformFloat( "texn", float( LastMonster + LastBullet ) + g_texture_aray_coord_eps );

			glDrawElements(
				GL_TRIANGLES,
				monsters_models_index_count_[LastMonster + 1],
				GL_UNSIGNED_SHORT,
				(void*)( monsters_models_first_index_[LastMonster + 1] * sizeof(unsigned short) ) );
		}
	}

	glDisable (GL_CULL_FACE );
}

void mx_Renderer::DrawParticles()
{
	const mx_ParticlesManager* particles_manager= level_.GetParticlesManager();

	mx_ParticleVertex vertices[ MX_MAX_PARTICLES ];
	particles_manager->PrepareParticlesVertices( vertices );
	unsigned int particle_count= particles_manager->GetParticlesCount();

	particles_vertex_buffer_.VertexSubData( vertices, particle_count * sizeof(mx_ParticleVertex), 0 );

	particles_shader_.Bind();
	particles_shader_.UniformMat4( "mat", view_matrix_ );
	particles_shader_.UniformFloat( "ss",  float(main_loop_.ViewportHeight()) / std::tanf(player_.Fov()*0.5f) );

	glDepthMask( 0 );
	glEnable( GL_PROGRAM_POINT_SIZE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ONE );

	glDrawArrays( GL_POINTS, 0, particle_count );

	glDisable( GL_BLEND );
	glDisable( GL_PROGRAM_POINT_SIZE );
	glDepthMask( 1 );
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

	//Draw fullscreen quad
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
	{
		const mx_LevelSector& sector= level_data.sectors[s];

		if( sector.traverse_id != visible_sectors_tag_ )
			continue;

		for( unsigned int l= 0; l < level_data.sectors[s].light_count; l++ )
			DrawLightSource( sector.lights[l] );

		if( sector.has_icosahedron && !sector.icosahedron_picked )
		{
			static const float c_light_intensity= 0.5f;

			mx_Light light_source;

			VEC3_CPY( light_source.pos, sector.icosahedron_pos );
			mxVec3Mul( mx_GameConstants::icosahedron_color, c_light_intensity, light_source.light_rgb );

			DrawLightSource( light_source );
		}
	}

	const mx_Bullet* bullets= level_.GetBullets();
	unsigned int bullet_count= level_.GetBulletCount();
	for( unsigned int b= 0; b < bullet_count; b++ )
	{
		mx_Light light_source;

		VEC3_CPY( light_source.pos, bullets[b].pos );

		mxVec3Mul(
			mx_GameConstants::bullets_colors[ bullets[b].type ],
			g_bullets_light_intensity[ bullets[b].type ],
			light_source.light_rgb );

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

	static const float c_min_valuable_light= 1.0f / 32.0f;
	float min_light_distance= std::sqrt( max_light / c_min_valuable_light );
	min_light_distance*= 1.333f; // extend sphere model for compensation of model shape error

	postprocessing_shader_.UniformVec3( "lp", light_source.pos );
	postprocessing_shader_.UniformVec4( "lc", light_source.light_rgb );
	static const float c_lsb_vec[4]= { c_min_valuable_light, c_min_valuable_light, c_min_valuable_light, c_min_valuable_light };
	postprocessing_shader_.UniformVec4( "lsb", c_lsb_vec );
	
	if( mxSquareDistance( light_source.pos, player_.Pos() ) >= ( min_light_distance + z_near_ ) * ( min_light_distance + z_near_ ) )
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

void mx_Renderer::MakeTonemapping()
{
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, hdr_buffer_.color_texture_id );

	tonemapping_shader_.Bind();
	tonemapping_shader_.UniformInt( "tex", 0 );

	glDisable( GL_DEPTH_TEST );

	//Draw fullscreen quad
	glDrawArrays( GL_TRIANGLES, 0, 6 );

	glEnable( GL_DEPTH_TEST );
}

void mx_Renderer::DrawGui()
{
	GuiVertex vertices[ MX_MAX_GUI_VERTICES ];
	GuiVertex* v= vertices;

	const int c_screen_border_indent= 20;

	static const unsigned char c_gui_main_color[4]= { 255, 255, 255, 64 };

	{ // health
		static const unsigned char c_health_color[4]= { 64, 240, 64, 128 };

		const int c_health_bar_width= 20;
		const int c_health_bar_border_width= 2;
		const int c_health_bar_height= 100;

		v= AddGuiQuad(
			v,
			int(main_loop_.ViewportWidth()) - ( c_screen_border_indent + c_health_bar_border_width * 2 + c_health_bar_width ),
			c_screen_border_indent,
			c_health_bar_width + c_health_bar_border_width * 2,
			c_health_bar_height + c_health_bar_border_width * 2,
			c_gui_main_color );

		v= AddGuiQuad(
			v,
			int(main_loop_.ViewportWidth()) - ( c_screen_border_indent + c_health_bar_border_width + c_health_bar_width ),
			c_screen_border_indent + c_health_bar_border_width,
			c_health_bar_width,
			player_.GetHealth() * c_health_bar_height / mx_GameConstants::player_max_health,
			c_health_color );
	}
	{ // ammo
		const int c_border= 3;
		const int c_current_weapon_quad_size= 24;
		int x0= c_screen_border_indent;
		int y= c_screen_border_indent;

		unsigned char current_weapon_color[4];
		FloatColorToByte( mx_GameConstants::bullets_colors[ player_.GetCurrentWeapon() ], current_weapon_color );
		current_weapon_color[3]= 128;
		v= AddGuiQuad( v, x0, y, c_current_weapon_quad_size, c_current_weapon_quad_size, current_weapon_color );

		y+= c_current_weapon_quad_size + c_border * 3;

		for( unsigned int a= 0; a < LastBullet; a++ )
		{
			unsigned char color[4];
			FloatColorToByte( mx_GameConstants::bullets_colors[a], color );
			color[3]= 128;

			unsigned int ammo= player_.GetAmmo(BulletType(a));
			unsigned int ammo_x64= ammo / 64;
			unsigned int ammo_x8 = ammo / 8 % 8;
			unsigned int ammo_x1 = ammo % 8;

			int y0= y;
			
			for( unsigned int i= 0; i < ammo_x1; i++ )
			{
				v= AddGuiQuad(
					v,
					(2*c_border + x0) + i * (4 + c_border), y,
					4, 4,
					color );
			}
			y+= 4 + c_border;

			for( unsigned int i= 0; i < ammo_x8; i++ )
			{
				v= AddGuiQuad(
					v,
					(2*c_border + x0) + i * (8 + c_border), y,
					8, 8,
					color );
			}
			y+= 8 + c_border;

			for( unsigned int i= 0; i < ammo_x64; i++ )
			{
				v= AddGuiQuad(
					v,
					(2*c_border + x0) + i * (16 + c_border), y,
					16, 16,
					color );
			}
			y+= 16;

			v= AddGuiQuad(
				v,
				x0, y0,
				c_border, y - y0,
				color );

			y+= c_border * 2;
		}
	}
	{ // crosshair
		int center_x= main_loop_.ViewportWidth () / 2;
		int center_y= main_loop_.ViewportHeight() / 2;

		const int c_bar_length= 14;
		const int c_half_bar_width= 2;
		const int c_bar_width= c_half_bar_width * 2;
		const int c_distance_to_center= 10;

		// left
		v= AddGuiQuad(
			v,
			center_x - (c_bar_length + c_distance_to_center),
			center_y - c_half_bar_width,
			c_bar_length,
			c_bar_width,
			c_gui_main_color );
		// right
		v= AddGuiQuad(
			v,
			center_x + c_distance_to_center,
			center_y - c_half_bar_width,
			c_bar_length,
			c_bar_width,
			c_gui_main_color );

		// top
		v= AddGuiQuad(
			v,
			center_x - c_half_bar_width,
			center_y + c_distance_to_center,
			c_bar_width,
			c_bar_length,
			c_gui_main_color );
		// bottom
		v= AddGuiQuad(
			v,
			center_x - c_half_bar_width,
			center_y - (c_distance_to_center + c_bar_length),
			c_bar_width,
			c_bar_length,
			c_gui_main_color );
	}
	{ // draw icosahedrons
		unsigned char icosahedron_color[4]= { 0xFF, 0xFF, 0, 0x7F };
		FloatColorToByte( mx_GameConstants::icosahedron_color, icosahedron_color );
		icosahedron_color[3]= 0x7F;

		const int c_radius= 12;
		const int c_border_size= 2;
		const unsigned int c_items_in_row= 12;

		const int x0= c_screen_border_indent + c_radius + c_border_size;
		int y0= main_loop_.ViewportHeight() - (c_screen_border_indent + c_radius + c_border_size);

		const mx_LevelSector* sectors= level_.GetLevelData().sectors;
		unsigned int sector_count= level_.GetLevelData().sector_count;

		for( unsigned int s= 0, i= 0; s < sector_count; s++ )
		{
			if( !sectors[s].has_icosahedron )
				continue;

			int column= i % c_items_in_row;
			int row= i / c_items_in_row;

			int x= x0 + column * (c_radius + c_border_size) * 2 + (c_radius + c_border_size) * (row&1);
			int y= y0 - row * (c_radius + c_border_size - 1) * 2;

			v= AddGuiIcosahedronProjection( v, x, y, c_radius + c_border_size, c_gui_main_color );
			if( sectors[s].icosahedron_picked )
				v= AddGuiIcosahedronProjection( v, x, y, c_radius, icosahedron_color );

			i++;
		}
	}
	{ // fps
		const int c_fps_bar_height= c_screen_border_indent;

		static const unsigned char c_fps_colors[4][4]=
		{
			{ 0xFF, 0x10, 0x10, 0x7F }, // red
			{ 0xFF, 0x7F, 0x10, 0x7F }, // orange
			{ 0xFF, 0xFF, 0x10, 0x7F }, // yellow
			{ 0x10, 0xFF, 0x10, 0x7F }, // green
		};
		const unsigned char* color;
		int fps= int(main_loop_.FPS());
		if( fps < 20 ) color= c_fps_colors[0];
		else if( fps < 30 ) color= c_fps_colors[1];
		else if( fps < 59 ) color= c_fps_colors[2];
		else color= c_fps_colors[3];

		v= AddGuiQuad(
			v,
			int(main_loop_.ViewportWidth ()) - fps,
			int(main_loop_.ViewportHeight()) - c_fps_bar_height,
			fps,
			c_fps_bar_height,
			color );
	}

	MX_ASSERT( v - vertices <= MX_MAX_GUI_VERTICES );

	gui_vertex_buffer_.Bind();
	gui_vertex_buffer_.VertexSubData( vertices, (v - vertices) * sizeof(GuiVertex), 0 );

	gui_shader_.Bind();

	float inv_size[3]= { 1.0f / float(main_loop_.ViewportWidth()), 1.0f / float(main_loop_.ViewportHeight()), 1.0f };
	gui_shader_.UniformVec3( "isz", inv_size );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDrawArrays( GL_TRIANGLES, 0, v - vertices );

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}

void mx_Renderer::MakePowerupsRotationMatrix( float* out_mat )
{
	float rotation_vector_rotation= mx_MainLoop::Instance()->GetTime();
	float self_rotation= rotation_vector_rotation * ( 9.0f / 16.0f);
	float rotation_vec[3];
	rotation_vec[0]= std::cosf(rotation_vector_rotation);
	rotation_vec[1]= std::sinf(rotation_vector_rotation);
	rotation_vec[2]= 0.0f;
	mxMat4RotateAroundVector( out_mat, rotation_vec, self_rotation );
}