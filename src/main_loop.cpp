#include <cstdio>

#include "gl/funcs.h"
#include "level.h"
#include "level_generator.h"
#include "mx_assert.h"
#include "player.h"
#include "renderer.h"
#include "sound_engine.h"
#include "text.h"

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
	MX_ASSERT( !instance_ );
	new
		mx_MainLoop(
			viewport_width, viewport_height,
			fullscreen, vsync,
			invert_mouse_y,
			mouse_speed_x, mouse_speed_y );
}

void mx_MainLoop::DeleteInstance()
{
	MX_ASSERT( instance_ );

	mx_SoundEngine::DeleteInstance();

	delete instance_->text_;
	delete instance_->renderer_;
	delete instance_->player_;
	delete instance_->level_;

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
	, mouse_speed_y_( invert_mouse_y ? -mouse_speed_y : mouse_speed_y )
	, fullscreen_(fullscreen)
	, quit_(false)
	, prev_cursor_pos_(), mouse_captured_(false)
{
	instance_= this;

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

	fps_calc_.prev_calc_time_ms= 0;
	fps_calc_.frame_count_to_show= 0;
	fps_calc_.current_calc_frame_count= 0;

	player_= new mx_Player();

	mx_LevelGenerator* generator= new mx_LevelGenerator();
	generator->Generate();
	level_= new mx_Level( generator->GetLevelData() );
	delete generator;

	player_->SetLevel(level_);
	
	renderer_= new mx_Renderer( *level_, *player_ );
	text_= new mx_Text();

	mx_SoundEngine::CreateInstance( hwnd_ );

	mx_SoundSource* src= mx_SoundEngine::Instance()->CreateSoundSource( SoundMelody );
	static const float c_zero_vec[3]= { 0.0f, 0.0f, 0.0f };
	src->SetOrientation( c_zero_vec, c_zero_vec );
	src->SetVolume( 40.0f );
	/*
	src->Play();
	*/

	// Start game time calculations after all heavy operations
	start_time_ms_= prev_time_ms_= GetTickCount();
	toatal_time_s_= 0.0f;

	need_capture_mouse_= true;
	CaptureMouse( need_capture_mouse_ );
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
			//char str[32];
			//sprintf( str, "fps: %d", fps_calc_.frame_count_to_show );
		}

		if( mouse_captured_ )
		{
			POINT new_cursor_pos;
			GetCursorPos(&new_cursor_pos);
			player_->Rotate(
				mouse_speed_x_ * float( prev_cursor_pos_.x - new_cursor_pos.x ),
				mouse_speed_y_ * float( prev_cursor_pos_.y - new_cursor_pos.y ) );
			SetCursorPos( prev_cursor_pos_.x, prev_cursor_pos_.y );
		}

		static const float c_min_dt= 1.0f / 128.0f;
		static const float c_max_dt= 1.0f /  16.0f;
		DWORD current_time_ms= GetTickCount() - start_time_ms_;
		float dt_s= float( current_time_ms - prev_time_ms_ ) * 0.001f;
		if( dt_s >= c_min_dt )
		{
			prev_time_ms_= current_time_ms;
			dt_s_= dt_s < c_max_dt ? dt_s : c_max_dt;
			toatal_time_s_+= dt_s_;

			player_->Tick();
			level_->Tick();
			
		}

		{ // Sound here
				float player_mat[16];
				player_->CreateRotationMatrix4( player_mat, true );
				static const float vel[3]= { 0.0f, 0.0f, 0.0f };
				mx_SoundEngine::Instance()->SetListenerOrinetation( player_->Pos(),player_mat, vel );
				mx_SoundEngine::Instance()->Tick();
		}

		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		renderer_->Draw();
		
		{ // texts
			//TODO: remove this, if we reach 32kb limit

			char str[]= "fps:    0";
			str[8]= '0' + fps_calc_.frame_count_to_show % 10;
			if( fps_calc_.frame_count_to_show >=   10 )
				str[7]= '0' + fps_calc_.frame_count_to_show /   10 % 10;
			if( fps_calc_.frame_count_to_show >=  100 )
				str[6]= '0' + fps_calc_.frame_count_to_show /  100 % 10;
			if( fps_calc_.frame_count_to_show >= 1000 )
				str[5]= '0' + fps_calc_.frame_count_to_show / 1000 % 10;

			text_->AddText( 0, 0, 1, mx_Text::default_color, str );

			text_->Draw();
		}

		SwapBuffers( hdc_ );
		CalculateFPS();
	} // while !quit
}

LRESULT CALLBACK mx_MainLoop::WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	mx_Player* player= instance_->player_;
	if( !player ) goto def_proc;

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
		player->ShotButtonPressed();
		break;
	case WM_RBUTTONDOWN:
		break;
	case WM_MBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
		player->ShotButtonReleased();
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
#ifdef MX_DEBUG
		case KEY('N'):
			player->DebugToggleNoclip();
			break;
#endif
		case KEY('M'):
			instance_->need_capture_mouse_= !instance_->need_capture_mouse_;
			instance_->CaptureMouse( instance_->need_capture_mouse_ );
			break;
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
		renderer_->OnFramebufferResize();
	}
}

void mx_MainLoop::FocusChange( bool focus_in )
{
	CaptureMouse( focus_in && need_capture_mouse_ );
}

void mx_MainLoop::CaptureMouse( bool need_capture )
{
	if( need_capture == mouse_captured_ ) return;

	if(mouse_captured_)
	{
		mouse_captured_= false;
		ShowCursor( true );
	}
	else
	{
		mouse_captured_= true;
		ShowCursor( false );

		RECT window_rect;
		GetWindowRect( hwnd_, & window_rect );
		prev_cursor_pos_.x= ( window_rect.left + window_rect.right  ) >> 1;
		prev_cursor_pos_.y= ( window_rect.top  + window_rect.bottom ) >> 1;

		SetCursorPos( prev_cursor_pos_.x, prev_cursor_pos_.y );
	}
}

void mx_MainLoop::CalculateFPS()
{
	// TODO: remove this, if we reach 32kb limit

	const unsigned int fps_calc_interval_ms= 500;

	fps_calc_.current_calc_frame_count++;

	unsigned int current_time= GetTickCount() - start_time_ms_;
	unsigned int dt= current_time - fps_calc_.prev_calc_time_ms;
	if( dt >= fps_calc_interval_ms )
	{
		fps_calc_.frame_count_to_show= fps_calc_.current_calc_frame_count * 1000u / fps_calc_interval_ms;
		fps_calc_.current_calc_frame_count= 0;
		fps_calc_.prev_calc_time_ms+= fps_calc_interval_ms;
	}
}