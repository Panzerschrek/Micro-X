#include <cstdio>
#include <ctime>

#include "gl/funcs.h"
#include "level_generator.h"
#include "mx_math.h"
#include "player.h"
#include "shaders.h"

#include "main_loop.h"

#define OGL_VERSION_MAJOR 3
#define OGL_VERSION_MINOR 3
#define KEY(x) (65 + x - 'A' )

static const char g_window_class[]= "Micro-X";
// TODO - change name if you change game name
static const char g_window_name[]= "Micro-X";

#ifdef MX_DEBUG
static void APIENTRY GLDebugMessageCallback(
	GLenum source, GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length, const GLchar *message,
	void *userParam )
{
	(void)source;
	(void)type;
	(void)id;
	(void)severity;
	(void)length;
	(void)userParam;
	std::printf( "%s\n", message );
}
#endif

mx_MainLoop* mx_MainLoop::instance_= NULL;

void mx_MainLoop::CreateInstance(
	unsigned int viewport_width, unsigned int viewport_height,
	bool fullscreen, bool vsync,
	bool invert_mouse_y,
	float mouse_speed_x, float mouse_speed_y )
{
	instance_= new
		mx_MainLoop(
			viewport_width, viewport_height,
			fullscreen, vsync,
			invert_mouse_y,
			mouse_speed_x, mouse_speed_y );
}

void mx_MainLoop::DeleteInstance()
{
	delete instance_;
	instance_= NULL;
}

mx_MainLoop::mx_MainLoop(
	unsigned int viewport_width, unsigned int viewport_height,
	bool fullscreen, bool vsync,
	bool invert_mouse_y,
	float mouse_speed_x, float mouse_speed_y )
	: viewport_width_(viewport_width), viewport_height_(viewport_height)
	, mouse_speed_x_( mouse_speed_x )
	, mouse_speed_y_( mouse_speed_y * (invert_mouse_y ? -1.0f :1.0f ) )
	, fullscreen_(fullscreen)
	, quit_(false)
	, prev_cursor_pos_(), mouse_captured_(false)
{
	int border_size, top_border_size, bottom_border_size;

	window_class_.cbSize= sizeof(WNDCLASSEX);
	window_class_.style= CS_OWNDC;
	window_class_.lpfnWndProc= WindowProc;
	window_class_.cbClsExtra= 0;
	window_class_.cbWndExtra= 0;
	window_class_.hInstance= 0;
	window_class_.hIcon= LoadIcon( 0 , IDI_APPLICATION );
	window_class_.hCursor= LoadCursor( NULL, IDC_ARROW );
	window_class_.hbrBackground= (HBRUSH) GetStockObject(BLACK_BRUSH);
	window_class_.lpszMenuName= NULL;
	window_class_.lpszClassName= g_window_class;
	window_class_.hIconSm= LoadIcon( NULL, IDI_APPLICATION );

	RegisterClassEx( &window_class_ );

	border_size= GetSystemMetrics(SM_CXFIXEDFRAME);
	bottom_border_size= GetSystemMetrics(SM_CYFIXEDFRAME);
	top_border_size= bottom_border_size + GetSystemMetrics(SM_CYCAPTION);

	unsigned int window_width = viewport_width_  + border_size * 2 + 2;
	unsigned int window_height= viewport_height_ + top_border_size + bottom_border_size + 2;
	if( fullscreen_ )
	{
		viewport_width_ = window_width = GetSystemMetrics(SM_CXSCREEN);
		viewport_height_= window_height= GetSystemMetrics(SM_CYSCREEN);
	}

	hwnd_= CreateWindowEx(
		0, g_window_class, g_window_name,
		fullscreen_ ? WS_POPUP|WS_SYSMENU : WS_OVERLAPPEDWINDOW,
		0, 0,
		window_width,
		window_height,
		NULL, NULL, 0, NULL);

	ShowWindow( hwnd_, SW_SHOWNORMAL );
	hdc_= GetDC( hwnd_ );

	//wglMakeCurrent( 0, 0 );
	PIXELFORMATDESCRIPTOR pfd;

	ZeroMemory( &pfd, sizeof(pfd) );
	pfd.nVersion= 1;
	pfd.dwFlags= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType= PFD_TYPE_RGBA;
	pfd.cColorBits= 32;
	pfd.cDepthBits= 32;
	pfd.cStencilBits= 8;
	pfd.iLayerType= PFD_MAIN_PLANE;

	int format= ChoosePixelFormat( hdc_, &pfd );
	SetPixelFormat( hdc_, format, &pfd );

	hrc_= wglCreateContext( hdc_ );
	wglMakeCurrent( hdc_, hrc_ );

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB= NULL;
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( hrc_ );

	static const int attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, OGL_VERSION_MAJOR,
		WGL_CONTEXT_MINOR_VERSION_ARB, OGL_VERSION_MINOR,
		WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef MX_DEBUG
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
		0,
	};

	hrc_= wglCreateContextAttribsARB( hdc_, 0, attribs );
	wglMakeCurrent( hdc_, hrc_ );

	mxGetGLFunctions();

	// EXT function can not exist in some cases.
	if( wglSwapIntervalEXT ) wglSwapIntervalEXT( vsync ? 1 : 0 );

#ifdef MX_DEBUG
	if( glDebugMessageCallback )
		glDebugMessageCallback( &GLDebugMessageCallback, NULL );
#endif

	// Initial OpenGL state
	glEnable( GL_DEPTH_TEST );

	start_time_ms_= prev_time_ms_= GetTickCount();

	fps_calc_.prev_calc_time= std::clock();
	fps_calc_.frame_count_to_show= 0;
	fps_calc_.current_calc_frame_count= 0;

	{
		mx_LevelGenerator* generator= new mx_LevelGenerator();
		generator->Generate();
		mx_LevelMesh mesh= generator->GenerateLevelMesh();
		delete generator;

		vertex_buffer_.VertexData( mesh.vertices, sizeof(mx_LevelVertex) * mesh.vertex_count, sizeof(mx_LevelVertex) );
		vertex_buffer_.VertexAttrib( 0, 3, GL_FLOAT, false, 0 );
		vertex_buffer_.IndexData( mesh.triangles, mesh.triangle_count * sizeof(unsigned short) * 3 );
		delete[] mesh.vertices;
		delete[] mesh.triangles;

		shader_.SetAttribLocation( "p", 0 );
		shader_.Create( mx_Shaders::world_shader_v, mx_Shaders::world_shader_f );
		static const char* const uniforms[]= { "mat" };
		shader_.FindUniforms( uniforms, sizeof(uniforms) / sizeof(char*) );
	}

	player_= new mx_Player();
}

mx_MainLoop::~mx_MainLoop()
{
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( hrc_ );

	ReleaseDC( hwnd_, hdc_ );
	DestroyWindow(hwnd_);

	UnregisterClass( g_window_class, 0 );
}

void mx_MainLoop::Loop()
{
	while(!quit_)
	{
		MSG msg;
		while ( PeekMessage(&msg,NULL,0,0,PM_REMOVE) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		{
			char str[32];
			sprintf( str, "fps: %d", fps_calc_.frame_count_to_show );
		}

		static const float c_dt_eps= 1.0f / 512.0f;
		DWORD current_time_ms= GetTickCount() - start_time_ms_;
		float dt_s= float( current_time_ms - prev_time_ms_ ) * 0.001f;
		if( dt_s >= c_dt_eps )
		{
			prev_time_ms_= current_time_ms;
			dt_s_= dt_s;
		}

		if( dt_s >= c_dt_eps )
			player_->Tick( 1.0f / 60.0f );

		glClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		{
			float translate_vec[3];
			float pers_mat[16];
			float rot_z_mat[16];
			float rot_x_mat[16];
			float rot_y_mat[16];
			float basis_change_mat[16];
			float translate_mat[16];
			float result_mat[16];

			mxVec3Mul( player_->Pos(), -1.0f, translate_vec );
			mxMat4Translate( translate_mat, translate_vec );

			mxMat4Perspective( pers_mat,
				float(viewport_width_)/ float(viewport_height_),
				player_->Fov(), 0.5f, 256.0f );

			mxMat4RotateZ( rot_z_mat, -player_->Angle()[2] );
			mxMat4RotateX( rot_x_mat, -player_->Angle()[0] );
			mxMat4RotateY( rot_y_mat, -player_->Angle()[1] );
			{
				mxMat4RotateX( basis_change_mat, -MX_PI2 );
				float tmp_mat[16];
				mxMat4Identity( tmp_mat );
				tmp_mat[10]= -1.0f;
				mxMat4Mul( basis_change_mat, tmp_mat );
			}

			mxMat4Mul( translate_mat, rot_z_mat, result_mat );
			mxMat4Mul( result_mat, rot_x_mat );
			mxMat4Mul( result_mat, rot_y_mat );
			mxMat4Mul( result_mat, basis_change_mat );
			mxMat4Mul( result_mat, pers_mat );

			shader_.Bind();
			shader_.UniformMat4( "mat", result_mat );

			vertex_buffer_.Bind();
			//glEnable( GL_CULL_FACE );
			//glCullFace( GL_FRONT );
			glDrawElements( GL_TRIANGLES, vertex_buffer_.IndexDataSize() / sizeof(unsigned short), GL_UNSIGNED_SHORT, NULL );
		}

		SwapBuffers( hdc_ );
		//CalculateFPS();
	} // while !quit
}

LRESULT CALLBACK mx_MainLoop::WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	// This method calls just after creation of window, in class constructor.
	// At this moment instance_ is nullptr.
	if( !instance_ ) goto def_proc;

	mx_Player* player= instance_->player_;

	switch(uMsg)
	{
	case WM_CLOSE:
	case WM_QUIT:
		instance_->quit_= true;
		break;
	case WM_SIZE:
		instance_->Resize();
		break;
	case WM_SETFOCUS:
		instance_->FocusChange( HWND(wParam) != instance_->hwnd_ );
		break;
	case WM_KILLFOCUS:
		instance_->FocusChange( HWND(wParam) == instance_->hwnd_ );
		break;
	case WM_LBUTTONDOWN:
		break;
	case WM_RBUTTONDOWN:
		break;
	case WM_MBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
		//instance->gui_->MouseClick( lParam&65535, lParam>>16 );
		break;
	case WM_MOUSEMOVE:
		break;
	case WM_MOUSEWHEEL:
		{
			int step= GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			(void) step;
		}
		break;

	case WM_KEYUP:
		switch(wParam)
		{
		case KEY('W'):
			player->ForwardReleased();
			break;
		case KEY('S'):
			player->BackwardReleased();
			break;
		case KEY('A'):
			player->LeftReleased();
			break;
		case KEY('D'):
			player->RightReleased();
			break;
		case KEY(' '):
			player->UpReleased();
			break;
		case KEY('C'):
			player->DownReleased();
			break;
		case VK_LEFT:
			player->RotateLeftReleased();
			break;
		case VK_RIGHT:
			player->RotateRightReleased();
			break;
		case VK_UP:
			player->RotateUpReleased();
			break;
		case VK_DOWN:
			player->RotateDownReleased();
			break;
		case KEY('Q'):
			player->RotateAnticlockwiseReleased();
			break;
		case KEY('E'):
			player->RotateClockwiseReleased();
			break;
		case VK_ESCAPE:
			instance_->quit_= true;
			break;
		default:
			break;
		}
		break; // WM_KEYUP

	case WM_KEYDOWN:
		switch(wParam)
		{
		case KEY('W'):
			player->ForwardPressed();
			break;
		case KEY('S'):
			player->BackwardPressed();
			break;
		case KEY('A'):
			player->LeftPressed();
			break;
		case KEY('D'):
			player->RightPressed();
			break;
		case KEY(' '):
			player->UpPressed();
			break;
		case KEY('C'):
			player->DownPressed();
			break;
		case VK_LEFT:
			player->RotateLeftPressed();
			break;
		case VK_RIGHT:
			player->RotateRightPressed();
			break;
		case VK_UP:
			player->RotateUpPressed();
			break;
		case VK_DOWN:
			player->RotateDownPressed();
			break;
		case KEY('Q'):
			player->RotateAnticlockwisePressed();
			break;
		case KEY('E'):
			player->RotateClockwisePressed();
			break;

		default:
			break;
		};
		break; // WM_KEYDOWN

	default:
		break;
	}; // switch(uMsg)

def_proc:
	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

void mx_MainLoop::Resize()
{
	if(fullscreen_)
	{
		//MoveWindow( hwnd_, 0, 0, viewport_width_, viewport_height_, true );
		return;
	}

	RECT rect;
	GetClientRect( hwnd_, &rect );
	unsigned int new_width = rect.right;
	unsigned int new_height= rect.bottom;

	if( new_width != viewport_width_ || new_height != viewport_height_ )
	{
		if( new_width < MX_MIN_VIEWPORT_WIDTH || new_height < MX_MIN_VIEWPORT_HEIGHT )
		{
			RECT window_rect;
			GetWindowRect( hwnd_, &window_rect );

			unsigned int window_width= window_rect.right - window_rect.left;
			if( new_width < MX_MIN_VIEWPORT_WIDTH )
			{
				window_width+= MX_MIN_VIEWPORT_WIDTH - new_width;
				viewport_width_= MX_MIN_VIEWPORT_WIDTH;
			}
			unsigned int window_height= window_rect.bottom - window_rect.top;
			if( new_height < MX_MIN_VIEWPORT_HEIGHT )
			{
				window_height+= MX_MIN_VIEWPORT_HEIGHT - new_height;
				viewport_height_= MX_MIN_VIEWPORT_HEIGHT;
			}

			MoveWindow( hwnd_, window_rect.left, window_rect.top, window_width, window_height, true );
		}
		else
		{
			viewport_width_ = new_width;
			viewport_height_= new_height;
		}
		glViewport( 0, 0, viewport_width_, viewport_height_ );
	}
}

void mx_MainLoop::FocusChange( bool focus_in )
{
	if( !focus_in )
		CaptureMouse( false );
}

void mx_MainLoop::CaptureMouse( bool need_capture )
{
	if( need_capture == mouse_captured_) return;

	if(mouse_captured_)
	{
		mouse_captured_= false;
		ShowCursor( true );
	}
	else
	{
		mouse_captured_= true;
		ShowCursor( false );
		GetCursorPos( &prev_cursor_pos_ );
	}
}

void mx_MainLoop::CalculateFPS()
{
	const unsigned int fps_calc_interval_ticks= CLOCKS_PER_SEC * 3 / 4;

	fps_calc_.current_calc_frame_count++;

	unsigned int current_time= clock();
	unsigned int dt= current_time - fps_calc_.prev_calc_time;
	if( dt >= fps_calc_interval_ticks )
	{
		fps_calc_.frame_count_to_show= fps_calc_.current_calc_frame_count * CLOCKS_PER_SEC / fps_calc_interval_ticks;
		fps_calc_.current_calc_frame_count= 0;

		fps_calc_.prev_calc_time= current_time;
	}
}