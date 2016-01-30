#pragma once
#include <windows.h>
#pragma warning(push)
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#include <mmsystem.h>
#pragma warning(pop)
#include <dsound.h>

#define MX_MAX_PARALLEL_SOUNDS 64

// TODO: remove unused sounds, add new.
enum mx_SoundType
{
	SoundPulsejetEngine,
	SoundPlasmajetEngine,
	SoundPowerupPickup,
	SoundMachinegunShot,
	SoundAutomaticCannonShot,
	SoundPlasmagunShot,
	SoundBlast,
	LastSound
};

class mx_SoundSource
{
public:
	void Play();
	void Pause();
	void Stop();

	void SetOrientation( const float* pos, const float* vel );
	void SetPitch( float pitch );
	void SetVolume( float volume ); // 1.0f means original volume in distance 1n, 4.0f in distance 2m, etc. square root law.

private:
	friend class mx_SoundEngine;

	LPDIRECTSOUNDBUFFER source_buffer_;
	LPDIRECTSOUND3DBUFFER source_buffer_3d_;
	unsigned int samples_per_second_;
};

class mx_SoundEngine
{
public:
	static void CreateInstance( HWND hwnd );
	static mx_SoundEngine* Instance();
	static void DeleteInstance();
	void Tick();

	// input - xyz of position, vec3 of angles, vec3 of velocity
	void SetListenerOrinetation( const float* pos, const float* rotation_mat4x4, const float* vel );

	mx_SoundSource* CreateSoundSource( mx_SoundType sound_type );
	void DestroySoundSource( mx_SoundSource* source );

	// if position is NULL - sound is player relative
	void AddSingleSound( mx_SoundType sound_type, float volume, float pitch, const float* opt_pos= NULL, const float* opt_speed= NULL );

private:
	mx_SoundEngine( HWND hwnd );
	~mx_SoundEngine();

	void GenSounds();

private:
	static mx_SoundEngine* instance_;

	LPDIRECTSOUND8 direct_sound_p_;
	LPDIRECTSOUNDBUFFER primary_sound_buffer_p_;
	LPDIRECTSOUND3DLISTENER listener_p_;
	unsigned int samples_per_second_;

	struct
	{
		LPDIRECTSOUNDBUFFER buffer;
		float length; // in seconds
	} sound_buffers_[LastSound];

	mx_SoundSource* sound_sources_[ MX_MAX_PARALLEL_SOUNDS ];
	unsigned int sound_source_count_;

	struct
	{
		mx_SoundSource* source;
		float death_time;
	} single_sounds_[ MX_MAX_PARALLEL_SOUNDS ];
	unsigned int single_sounds_count_;
};

inline mx_SoundEngine* mx_SoundEngine::Instance()
{
	return instance_;
}