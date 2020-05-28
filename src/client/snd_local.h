/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file snd_local.h
 * @brief Private sound definitions
 */

#ifndef INCLUDE_SND_LOCAL_H
#define INCLUDE_SND_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"
#include "snd_codec.h"

#define PAINTBUFFER_SIZE        4096                    ///< this is in samples

#define SND_CHUNK_SIZE          1024                    ///< samples
#define SND_CHUNK_SIZE_FLOAT    (SND_CHUNK_SIZE / 2)    ///< floats
#define SND_CHUNK_SIZE_BYTE     (SND_CHUNK_SIZE * 2)    ///< floats

/**
 * @struct portable_samplepair_s
 * @brief
 */
typedef struct
{
	int left;           ///< the final values will be clamped to +/- 0x00ffff00 and shifted down
	int right;
} portable_samplepair_t;

/**
 * @struct adpcm_state
 * @typedef adpcm_state_t
 * @brief
 */
typedef struct adpcm_state
{
	short sample;       ///< Previous output value
	char index;         ///< Index into stepsize table
} adpcm_state_t;

/**
 * @struct sndBuffer_s
 * @typedef sndBuffer
 * @brief
 */
typedef struct sndBuffer_s
{
	short sndChunk[SND_CHUNK_SIZE];
	struct sndBuffer_s *next;
	int size;
	adpcm_state_t adpcm;
} sndBuffer;

/**
 * @struct sfx_s
 * @typedef sfx_t
 * @brief
 */
typedef struct sfx_s
{
	sndBuffer *soundData;
	qboolean defaultSound;          ///< couldn't be loaded, so use buzz
	qboolean inMemory;              ///< not in Memory
	qboolean soundCompressed;       ///< not in Memory
	int soundCompressionMethod;
	int soundLength;
	int soundChannels;
	char soundName[MAX_QPATH];
	int lastTimeUsed;
	struct sfx_s *next;
} sfx_t;

/**
 * @struct dma_s
 * @brief
 */
typedef struct
{
	int channels;
	int samples;                ///< mono samples in buffer
	int submission_chunk;       ///< don't mix less than this #
	int samplebits;
	int speed;
	byte *buffer;
} dma_t;

#define START_SAMPLE_IMMEDIATE  0x7fffffff

#define MAX_DOPPLER_SCALE 50.0f ///< arbitrary

/**
 * @struct loopSound_s
 * @typedef loopSound_t
 * @brief
 */
typedef struct loopSound_s
{
	vec3_t origin;
	vec3_t velocity;
	sfx_t *sfx;
	int mergeFrame;
	qboolean active;
	qboolean kill;
	qboolean doppler;
	float dopplerScale;
	float oldDopplerScale;
	int framenum;
	float range;
	int volume;
	qboolean loudUnderWater;
	int startTime;
	int startSample;
} loopSound_t;

/**
 * @struct channel_s
 * @brief
 */
typedef struct
{
	int allocTime;
	int startSample;            ///< START_SAMPLE_IMMEDIATE = set immediately on next mix
	int entnum;                 ///< to allow overriding a specific sound
	int entchannel;             ///< to allow overriding a specific sound
	int leftvol;                ///< 0-255 volume after spatialization
	int rightvol;               ///< 0-255 volume after spatialization
	int master_vol;             ///< 0-255 volume before spatialization
	float dopplerScale;
	float oldDopplerScale;
	vec3_t origin;              ///< only use if fixed_origin is set
	qboolean fixed_origin;      ///< use origin instead of fetching entnum's origin
	sfx_t *thesfx;              ///< sfx structure
	qboolean doppler;
	int flags;
} channel_t;

#define WAV_FORMAT_PCM      1

/**
 * @struct wavinfo_s
 * @brief
 */
typedef struct
{
	int format;
	int rate;
	int width;
	int channels;
	int samples;
	int dataofs;                ///< chunk starts this many bytes from file start
} wavinfo_t;

/**
 * @struct soundInterface_s
 * @brief Interface between Q3 sound "api" and the sound backend
 */
typedef struct
{
	void (*Shutdown)(void);
	void (*Reload)(void);
	void (*StartSound)(vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx, int volume);
	void (*StartSoundEx)(vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx, int flags, int volume);
	void (*StartLocalSound)(sfxHandle_t sfx, int channelNum, int volume);
	void (*StartBackgroundTrack)(const char *intro, const char *loop, int fadeupTime);
	void (*StopBackgroundTrack)(void);
	float (*StartStreamingSound)(const char *intro, const char *loop, int entnum, int channel, int attenuation);
	void (*StopEntStreamingSound)(int entNum);
	void (*FadeStreamingSound)(float targetvol, int time, int ssNum);
	void (*RawSamples)(int stream, int samples, int rate, int width, int channels, const byte *data, float lvol, float rvol);
	void (*ClearSounds)(qboolean clearStreaming, qboolean clearMusic);
	void (*StopAllSounds)(void);
	void (*FadeAllSounds)(float targetvol, int time, qboolean stopsounds);
	void (*ClearLoopingSounds)(void);
	void (*AddLoopingSound)(const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfx, int volume, int soundTime);
	void (*AddRealLoopingSound)(const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfx, int volume, int soundTime);
	void (*Respatialize)(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater);
	void (*UpdateEntityPosition)(int entityNum, const vec3_t origin);
	void (*Update)(void);
	void (*DisableSounds)(void);
	void (*BeginRegistration)(void);
	sfxHandle_t (*RegisterSound)(const char *sample, qboolean compressed);
	void (*ClearSoundBuffer)(qboolean killStreaming);
	void (*SoundInfo)(void);
	void (*SoundList)(void);
	int (*GetVoiceAmplitude)(int entityNum);
	int (*GetSoundLength)(sfxHandle_t sfxHandle);
	int (*GetCurrentSoundTime)(void);
#ifdef USE_VOIP
	void (*StartCapture)(void);
	int (*AvailableCaptureSamples)(void);
	void (*Capture)(int samples, byte *data);
	void (*StopCapture)(void);
	void (*MasterGain)(float gain);
#endif
} soundInterface_t;

/*
====================================================================
  SYSTEM SPECIFIC FUNCTIONS
====================================================================
*/

// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init(void);

// gets the current DMA position
int SNDDMA_GetDMAPos(void);

// shutdown the DMA xfer.
void SNDDMA_Shutdown(void);

void SNDDMA_BeginPainting(void);

void SNDDMA_Submit(void);

//====================================================================

#define MAX_CHANNELS            96

extern channel_t s_channels[MAX_CHANNELS];
extern channel_t loop_channels[MAX_CHANNELS];
extern int       numLoopChannels;

extern int   s_paintedtime;
extern dma_t dma;

typedef struct
{
	snd_stream_t *stream;
	char name[MAX_QPATH];
	char loopStream[MAX_QPATH];
	char queueStream[MAX_QPATH];
	int queueStreamType;
	int entnum;
	int channel;
	qboolean attenuation;
	int fadeStart;
	int fadeEnd;
	float fadeStartVol;
	float fadeTargetVol;
} streamingSound_t;

#define MAX_RAW_SAMPLES 16384
#define MAX_RAW_STREAMS 128
#define MAX_STREAMING_SOUNDS 12

#define RAW_STREAM_MUSIC 0
#ifdef USE_VOIP
#define RAW_STREAM_VOIP (RAW_STREAM_MUSIC + 1)
#define RAW_STREAM_SOUNDS (RAW_STREAM_VOIP + MAX_CLIENTS)
#else
#define RAW_STREAM_SOUNDS (RAW_STREAM_MUSIC + 1)
#endif
#define RAW_STREAM(x) ((x ? RAW_STREAM_SOUNDS + x - 1 : 0))

extern portable_samplepair_t s_rawsamples[MAX_RAW_STREAMS][MAX_RAW_SAMPLES];
extern int                   s_rawend[MAX_RAW_STREAMS];

#define     MAX_LOOP_SOUNDS 1024

extern cvar_t *s_volume;
extern cvar_t *s_musicVolume;
extern cvar_t *s_muted;
extern cvar_t *s_doppler;

extern cvar_t *s_testsound;
extern cvar_t *s_debugStreams;

extern float s_volCurrent;

qboolean S_LoadSound(sfx_t *sfx);

void SND_Com_Dealloc(sndBuffer *v);
sndBuffer *SND_malloc(void);
void SND_setup(void);
void SND_shutdown(void);

void S_PaintChannels(int endtime);

void S_memoryLoad(sfx_t *sfx);

// adpcm functions
// int S_AdpcmMemoryNeeded(const wavinfo_t *info); // unused
void S_AdpcmEncodeSound(sfx_t *sfx, short *samples);
void S_AdpcmGetSamples(sndBuffer *chunk, short *to);

// wavelet function

#define SENTINEL_MULAW_ZERO_RUN 127
#define SENTINEL_MULAW_FOUR_BIT_RUN 126

void S_FreeOldestSound(void);

void encodeWavelet(sfx_t *sfx, short *packets);
void decodeWavelet(sndBuffer *chunk, short *packets);

void encodeMuLaw(sfx_t *sfx, short *packets);
extern short mulawToShort[256];

extern short *sfxScratchBuffer;
extern sfx_t *sfxScratchPointer;
extern int   sfxScratchIndex;

qboolean S_Base_Init(soundInterface_t *si);

#ifdef FEATURE_OPENAL

/**
 * @enum alSrcPriority_e
 * @brief OpenAL stuff
 */
typedef enum
{
	SRCPRI_AMBIENT = 0, ///< Ambient sound effects
	SRCPRI_ENTITY,      ///< Entity sound effects
	SRCPRI_ONESHOT,     ///< One-shot sounds
	SRCPRI_LOCAL,       ///< Local sounds
	SRCPRI_STREAM       ///< Streams (music, cutscenes)
} alSrcPriority_t;


typedef int srcHandle_t;

qboolean S_AL_Init(soundInterface_t *si);

#endif // FEATURE_OPENAL

#endif // #ifndef INCLUDE_SND_LOCAL_H
