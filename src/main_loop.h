#pragma once

#include <windows.h>

#include "fwd.h"

#define MX_MIN_VIEWPORT_WIDTH  800
#define MX_MIN_VIEWPORT_HEIGHT 600

class mx_MainLoop
{
public:
	static void CreateInstance(
		unsigned int viewport_width, unsigned int viewport_height,
		bool fullscreen, bool vsync,
		bool invert_mouse_y,
		float mouse_speed_x, float mouse_speed_y );

	static mx_MainLoop* Instance();
	static void DeleteInstance();

	unsigned int ViewportWidth () const;
	unsigned int ViewportHeight() const;
	unsigned int FPS() const;

	// Time - in seconds
	float GetTickTime() const; // time of current tick
	float GetTime() const; // total time sinse game start, include this tick

	void Loop();
	void Quit();

private:
	mx_MainLoop(
		unsigned int viewport_width, unsigned int viewport_height,
		bool fullscreen, bool vsync,
		bool invert_mouse_y,
		float mouse_speed_x, float mouse_speed_y );
	~mx_MainLoop();

	mx_MainLoop(const mx_MainLoop&){};
	mx_MainLoop& operator=(const mx_MainLoop&){};

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Resize();
	void FocusChange( bool focus_in );
	void CaptureMouse( bool need_capture );

	void CalculateFPS();

private:
	static mx_MainLoop* instance_;

	unsigned int viewport_width_, viewport_height_;
	float mouse_speed_x_, mouse_speed_y_;
	bool fullscreen_;
	bool quit_;

	HWND hwnd_;
	HDC hdc_;
	HGLRC hrc_;
	WNDCLASSEX window_class_;

	POINT prev_cursor_pos_;
	bool mouse_captured_;
	bool need_capture_mouse_;

	DWORD start_time_ms_;
	DWORD prev_time_ms_;
	float dt_s_;
	float toatal_time_s_;

	struct
	{
		unsigned int prev_calc_time_ms; // time since program start
		unsigned int frame_count_to_show;
		unsigned int current_calc_frame_count;
	}fps_calc_;

	mx_Player* player_;
	mx_Level* level_;
	mx_Renderer* renderer_;
	//mx_Text* text_;
};

inline mx_MainLoop* mx_MainLoop::Instance()
{
	return instance_;
}

inline unsigned int mx_MainLoop::ViewportWidth () const
{
	return viewport_width_;
}

inline unsigned int mx_MainLoop::ViewportHeight() const
{
	return viewport_height_;
}

inline unsigned int mx_MainLoop::FPS() const
{
	return fps_calc_.frame_count_to_show;
}

inline void mx_MainLoop::Quit()
{
	quit_= true;
}

inline float mx_MainLoop::GetTickTime() const
{
	return dt_s_;
}

inline float mx_MainLoop::GetTime() const
{
	return toatal_time_s_;
}