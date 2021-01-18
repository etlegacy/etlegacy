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
 * @file snd_openal.c
 */

#include "snd_local.h"
#include "snd_codec.h"
#include "client.h"

#ifdef FEATURE_OPENAL

#include "qal.h"

// Console variables specific to OpenAL
cvar_t *s_alPrecache;
cvar_t *s_alGain;
cvar_t *s_alSources;
cvar_t *s_alDopplerFactor;
cvar_t *s_alDopplerSpeed;
cvar_t *s_alMinDistance;
cvar_t *s_alMaxDistance;
cvar_t *s_alRolloff;
cvar_t *s_alGraceDistance;
cvar_t *s_alDriver;
cvar_t *s_alDevice;
//cvar_t *s_alInputDevice;
cvar_t *s_alAvailableDevices;
//cvar_t *s_alAvailableInputDevices;

static qboolean enumeration_ext     = qfalse;
static qboolean enumeration_all_ext = qfalse;

// sound fading
static float    s_volStart, s_volTarget;
static int      s_volTime1, s_volTime2;
static float    s_volFadeFrac;
static qboolean s_stopSounds;

#define NUM_MUSIC_BUFFERS 4
#define MUSIC_BUFFER_SIZE 4096

static qboolean    musicPlaying      = qfalse;
static srcHandle_t musicSourceHandle = -1;
static ALuint      musicSource;
static ALuint      musicBuffers[NUM_MUSIC_BUFFERS];

static snd_stream_t *mus_stream;
static snd_stream_t *intro_stream;
static char         s_backgroundLoop[MAX_QPATH];

#define QAL_EFX_DEDICATED 0
#define QAL_EFX_DEDICATED_LFE 1
#define QAL_EFX_MAX 2

ALuint effect[QAL_EFX_MAX];
ALuint auxslot[QAL_EFX_MAX];

/**
 * @brief S_AL_Format
 * @param[in] width
 * @param[in] channels
 * @return
 */
static ALuint S_AL_Format(int width, int channels)
{
	ALuint format = AL_FORMAT_MONO16;

	// Work out format
	if (width == 1)
	{
		if (channels == 1)
		{
			format = AL_FORMAT_MONO8;
		}
		else if (channels == 2)
		{
			format = AL_FORMAT_STEREO8;
		}
	}
	else if (width == 2)
	{
		if (channels == 1)
		{
			format = AL_FORMAT_MONO16;
		}
		else if (channels == 2)
		{
			format = AL_FORMAT_STEREO16;
		}
	}

	return format;
}

/**
 * @brief S_AL_ErrorMsg
 * @param[in] error
 * @return
 */
static const char *S_AL_ErrorMsg(ALenum error)
{
	switch (error)
	{
	case AL_NO_ERROR:
		return "No error";
	case AL_INVALID_NAME:
		return "Invalid name";
	case AL_INVALID_ENUM:
		return "Invalid enumerator";
	case AL_INVALID_VALUE:
		return "Invalid value";
	case AL_INVALID_OPERATION:
		return "Invalid operation";
	case AL_OUT_OF_MEMORY:
		return "Out of memory";
	default:
		return "Unknown error";
	}
}

/**
 * @brief S_AL_ClearError
 * @param[in] quiet
 */
static void S_AL_ClearError(qboolean quiet)
{
	int error = qalGetError();

	if (quiet)
	{
		return;
	}
	if (error != AL_NO_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING S_AL_ClearError: unhandled AL error: %s\n",
		           S_AL_ErrorMsg(error));
	}
}

//===========================================================================

typedef struct alSfx_s
{
	char filename[MAX_QPATH];
	ALuint buffer;                      // OpenAL buffer
	snd_info_t info;                    // information for this sound like rate, sample count..

	qboolean isDefault;                 // Couldn't be loaded - use default FX
	qboolean isDefaultChecked;          // Sound has been check if it isDefault
	qboolean inMemory;                  // Sound is stored in memory
	qboolean isLocked;                  // Sound is locked (can not be unloaded)
	int lastUsedTime;                   // Time last used

	int loopCnt;                        // number of loops using this sfx
	int loopActiveCnt;                  // number of playing loops using this sfx
	int masterLoopSrc;                  // All other sources looping this buffer are synced to this master src
} alSfx_t;

static qboolean alBuffersInitialised = qfalse;

// Sound effect storage, data structures
#define MAX_SFX 4096
static alSfx_t     knownSfx[MAX_SFX];
static sfxHandle_t numSfx = 0;

static sfxHandle_t default_sfx;

/**
 * @brief S_AL_GetVoiceAmplitude
 * @param entnum - unused
 * @return
 *
 * @note TODO: not implemented
 */
int S_AL_GetVoiceAmplitude(int entnum)
{
	return 0;
}

/**
 * @brief Returns how long the sound lasts in milliseconds
 * @param sfxHandle
 * @return
 */
int S_AL_GetSoundLength(sfxHandle_t sfxHandle)
{
	if (sfxHandle < 0 || sfxHandle >= numSfx)
	{
		Com_DPrintf(S_COLOR_YELLOW "S_AL_GetSoundLength: handle %i out of range\n", sfxHandle);
		return -1;
	}
	return (int)(((float)knownSfx[sfxHandle].info.samples / (float)knownSfx[sfxHandle].info.rate) * 1000.0f);
}

/**
 * @brief For looped sound synchronisation
 * @return
 */
int S_AL_GetCurrentSoundTime(void)
{
	return Sys_Milliseconds(); // FIXME: this causes looping sound issues -> 'S_GetSoundtime' + knownSfx[sfxHandle].info.rate; // FIXME: current handle & sound time
}

/**
 * @brief Find a free handle
 * @return
 */
static sfxHandle_t S_AL_BufferFindFree(void)
{
	int i;

	for (i = 0; i < MAX_SFX; i++)
	{
		// Got one
		if (knownSfx[i].filename[0] == '\0')
		{
			if (i >= numSfx)
			{
				numSfx = i + 1;
			}
			return i;
		}
	}

	// Shit...
	Com_Error(ERR_FATAL, "S_AL_BufferFindFree: No free sound handles");
	return -1;
}

/**
 * @brief Find a sound effect if loaded, set up a handle otherwise
 * @param[in] filename
 * @return
 */
static sfxHandle_t S_AL_BufferFind(const char *filename)
{
	// Look it up in the table
	sfxHandle_t sfx = -1;
	int         i;

	if (!filename[0])
	{
		Com_Printf(S_COLOR_RED "ERROR S_AL_BufferFind: can't find empty sound - returning default sfx\n");
		return default_sfx;
	}

	if (strlen(filename) >= MAX_QPATH)
	{
		Com_Printf(S_COLOR_RED "ERROR S_AL_BufferFind: sound name exceeds MAX_QPATH \"%s\" - returning default sfx\n", filename);
		return default_sfx;
	}

	for (i = 0; i < numSfx; i++)
	{
		if (!Q_stricmp(knownSfx[i].filename, filename))
		{
			sfx = i;
			break;
		}
	}

	// Not found in table?
	if (sfx == -1)
	{
		alSfx_t *ptr;

		sfx = S_AL_BufferFindFree();

		if(sfx == -1)
		{
			return sfx;
		}

		// Clear and copy the filename over
		ptr = &knownSfx[sfx];
		Com_Memset(ptr, 0, sizeof(*ptr));
		ptr->masterLoopSrc = -1;
		strcpy(ptr->filename, filename);
	}

	// Return the handle
	return sfx;
}

/**
 * @brief S_AL_BufferUseDefault
 * @param[in] sfx
 */
static void S_AL_BufferUseDefault(sfxHandle_t sfx)
{
	if (sfx == default_sfx)
	{
		Com_Error(ERR_FATAL, "Can't load default sound effect %s", knownSfx[sfx].filename);
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: [S_AL_BufferUseDefault] Using default sound for %s\n", knownSfx[sfx].filename);
	knownSfx[sfx].isDefault = qtrue;
	knownSfx[sfx].buffer    = knownSfx[default_sfx].buffer;
}

/**
 * @brief S_AL_BufferUnload
 * @param[in] sfx
 */
static void S_AL_BufferUnload(sfxHandle_t sfx)
{
	ALenum error;

	if (knownSfx[sfx].filename[0] == '\0')
	{
		return;
	}

	if (!knownSfx[sfx].inMemory)
	{
		return;
	}

	// Delete it
	S_AL_ClearError(qfalse);
	qalDeleteBuffers(1, &knownSfx[sfx].buffer);
	if ((error = qalGetError()) != AL_NO_ERROR)
	{
		Com_Printf(S_COLOR_RED "ERROR S_AL_BufferUnload: Can't delete sound buffer for %s\n",
		           knownSfx[sfx].filename);
	}

	knownSfx[sfx].inMemory = qfalse;
}

/**
 * @brief S_AL_BufferEvict
 * @return
 */
static qboolean S_AL_BufferEvict(void)
{
	int i, oldestBuffer = -1;
	int oldestTime = Sys_Milliseconds();

	for (i = 0; i < numSfx; i++)
	{
		if (!knownSfx[i].filename[0])
		{
			continue;
		}

		if (!knownSfx[i].inMemory)
		{
			continue;
		}

		if (knownSfx[i].lastUsedTime < oldestTime)
		{
			oldestTime   = knownSfx[i].lastUsedTime;
			oldestBuffer = i;
		}
	}

	if (oldestBuffer >= 0)
	{
		S_AL_BufferUnload(oldestBuffer);
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/**
 * @brief S_AL_GenBuffers
 * @param[in] numBuffers
 * @param[out] buffers
 * @param[in] name
 * @return
 */
static qboolean S_AL_GenBuffers(ALsizei numBuffers, ALuint *buffers, const char *name)
{
	ALenum error;

	S_AL_ClearError(qfalse);
	qalGenBuffers(numBuffers, buffers);
	error = qalGetError();

	// If we ran out of buffers, start evicting the least recently used sounds
	while (error == AL_INVALID_VALUE)
	{
		if (!S_AL_BufferEvict())
		{
			Com_Printf(S_COLOR_RED "ERROR: Out of audio buffers\n");
			return qfalse;
		}

		// Try again
		S_AL_ClearError(qfalse);
		qalGenBuffers(numBuffers, buffers);
		error = qalGetError();
	}

	if (error != AL_NO_ERROR)
	{
		Com_Printf(S_COLOR_RED "ERROR: Can't create a sound buffer for %s - %s\n",
		           name, S_AL_ErrorMsg(error));
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief S_AL_BufferLoad
 * @param[in] sfx
 * @param[in] cache
 */
static void S_AL_BufferLoad(sfxHandle_t sfx, qboolean cache)
{
	ALenum error;
	ALuint format;

	void       *data;
	snd_info_t info;
	alSfx_t    *curSfx = &knownSfx[sfx];

	// Nothing?
	if (curSfx->filename[0] == '\0')
	{
		return;
	}

	// Player SFX
	if (curSfx->filename[0] == '*')
	{
		return;
	}

	// Already done?
	if ((curSfx->inMemory) || (curSfx->isDefault) || (!cache && curSfx->isDefaultChecked))
	{
		return;
	}

	// Try to load
	data = S_CodecLoad(curSfx->filename, &info);
	if (!data)
	{
		S_AL_BufferUseDefault(sfx);
		Com_Printf(S_COLOR_RED "ERROR S_AL_BufferLoad: S_CodecLoad failed for \"%s\"\n", curSfx->filename);
		return;
	}

	curSfx->isDefaultChecked = qtrue;

	if (!cache)
	{
		// Don't create AL cache
		Hunk_FreeTempMemory(data);
		return;
	}

	format = S_AL_Format(info.width, info.channels);

	// Create a buffer
	S_AL_ClearError(qfalse);
	qalGenBuffers(1, &curSfx->buffer);
	if ((error = qalGetError()) != AL_NO_ERROR)
	{
		S_AL_BufferUseDefault(sfx);
		Hunk_FreeTempMemory(data);
		Com_Printf(S_COLOR_RED "ERROR S_AL_BufferLoad: Can't create a sound buffer for %s - %s\n",
		           curSfx->filename, S_AL_ErrorMsg(error));
		return;
	}

	// Fill the buffer
	if (info.size == 0)
	{
		// We have no data to buffer, so buffer silence
		byte dummyData[2] = { 0 };

		qalBufferData(curSfx->buffer, AL_FORMAT_MONO16, (void *)dummyData, 2, 22050);
	}
	else
	{
		qalBufferData(curSfx->buffer, format, data, info.size, info.rate);
	}

	error = qalGetError();

	// If we ran out of memory, start evicting the least recently used sounds
	while (error == AL_OUT_OF_MEMORY)
	{
		if (!S_AL_BufferEvict())
		{
			qalDeleteBuffers(1, &curSfx->buffer);
			S_AL_BufferUseDefault(sfx);
			Hunk_FreeTempMemory(data);
			Com_Printf(S_COLOR_RED "ERROR S_AL_BufferLoad: Out of memory loading %s\n", curSfx->filename);
			return;
		}

		// Try load it again
		qalBufferData(curSfx->buffer, format, data, info.size, info.rate);
		error = qalGetError();
	}

	// Some other error condition
	if (error != AL_NO_ERROR)
	{
		qalDeleteBuffers(1, &curSfx->buffer);
		S_AL_BufferUseDefault(sfx);
		Hunk_FreeTempMemory(data);
		Com_Printf(S_COLOR_RED "ERROR S_AL_BufferLoad: Can't fill sound buffer for %s - %s\n",
		           curSfx->filename, S_AL_ErrorMsg(error));
		return;
	}

	curSfx->info = info;

	// Free the memory
	Hunk_FreeTempMemory(data);

	// Woo!
	curSfx->inMemory = qtrue;
}

/**
 * @brief S_AL_BufferUse
 * @param[in] sfx
 */
static void S_AL_BufferUse(sfxHandle_t sfx)
{
	if (knownSfx[sfx].filename[0] == '\0')
	{
		return;
	}

	if ((!knownSfx[sfx].inMemory) && (!knownSfx[sfx].isDefault))
	{
		S_AL_BufferLoad(sfx, qtrue);
	}
	knownSfx[sfx].lastUsedTime = Sys_Milliseconds();
}

/**
 * @brief S_AL_BufferInit
 * @return
 */
static qboolean S_AL_BufferInit(void)
{
	if (alBuffersInitialised)
	{
		return qtrue;
	}

	// Clear the hash table, and SFX table
	Com_Memset(knownSfx, 0, sizeof(knownSfx));
	numSfx = 0;

	// Load the default sound, and lock it
	default_sfx = S_AL_BufferFind("sound/player/default/blank.wav");
	S_AL_BufferUse(default_sfx);
	knownSfx[default_sfx].isLocked = qtrue;

	// All done
	alBuffersInitialised = qtrue;
	return qtrue;
}

/**
 * @brief S_AL_BufferShutdown
 */
static void S_AL_BufferShutdown(void)
{
	int i;

	if (!alBuffersInitialised)
	{
		return;
	}

	// Unlock the default sound effect
	knownSfx[default_sfx].isLocked = qfalse;

	// Free all used effects
	for (i = 0; i < numSfx; i++)
	{
		S_AL_BufferUnload(i);
	}

	// Clear the tables
	Com_Memset(knownSfx, 0, sizeof(knownSfx));
	numSfx = 0;

	// All undone
	alBuffersInitialised = qfalse;
}

/**
 * @brief S_AL_RegisterSound
 * @param[in] sample
 * @param compressed - unused
 * @return
 */
static sfxHandle_t S_AL_RegisterSound(const char *sample, qboolean compressed)
{
	sfxHandle_t sfx;

	if (!sample)
	{
		Com_DPrintf(S_COLOR_RED "ERROR: [S_AL_RegisterSound: NULL");
		return 0;
	}
	if (!sample[0])
	{
		Com_DPrintf(S_COLOR_RED "ERROR: [S_AL_RegisterSound: empty name");
		return 0;
	}

	if (strlen(sample) >= MAX_QPATH)
	{
		Com_DPrintf(S_COLOR_RED "ERROR: [S_AL_RegisterSound] Sound name exceeds MAX_QPATH - %s\n", sample);
		return 0;
	}

	sfx = S_AL_BufferFind(sample);


	if ((!knownSfx[sfx].inMemory) && (!knownSfx[sfx].isDefault))
	{
		S_AL_BufferLoad(sfx, s_alPrecache->integer);
	}
	knownSfx[sfx].lastUsedTime = Sys_Milliseconds();

	if (knownSfx[sfx].isDefault)
	{
		return 0;
	}

	return sfx;
}

/**
 * @brief S_AL_BufferGet
 * @param sfx
 * @return An sfx's buffer
 */
static ALuint S_AL_BufferGet(sfxHandle_t sfx)
{
	return knownSfx[sfx].buffer;
}

//===========================================================================

typedef struct src_s
{
	ALuint alSource;            // OpenAL source object
	sfxHandle_t sfx;            // Sound effect in use

	int lastUsedTime;           // Last time used
	alSrcPriority_t priority;   // Priority
	int entity;                 // Owning entity (-1 if none)
	int channel;                // Associated channel (-1 if none)

	qboolean isActive;          // Is this source currently in use?
	qboolean isPlaying;         // Is this source currently playing, or stopped?
	qboolean isLocked;          // This is locked (un-allocatable)
	qboolean isLooping;         // Is this a looping effect (attached to an entity)
	qboolean isTracking;        // Is this object tracking its owner

	float curGain;              // gain employed if source is within maxdistance.
	float scaleGain;            // Last gain value for this source. 0 if muted.

	float lastTimePos;          // On stopped loops, the last position in the buffer
	int lastSampleTime;         // Time when this was stopped
	vec3_t loopSpeakerPos;      // Origin of the loop speaker

	qboolean local;             // Is this local (relative to the cam)
} src_t;

#ifdef __APPLE__
#define MAX_SRC 64
#else
#define MAX_SRC 128
#endif
static src_t    srcList[MAX_SRC];
static int      srcCount             = 0;
static int      srcActiveCnt         = 0;
static qboolean alSourcesInitialised = qfalse;
static vec3_t   lastListenerOrigin   = { 0.0f, 0.0f, 0.0f };

typedef struct sentity_s
{
	vec3_t origin;

	qboolean srcAllocated;                        // If a src_t has been allocated to this entity
	int srcIndex;
	int volume;

	qboolean loopAddedThisFrame;
	alSrcPriority_t loopPriority;
	sfxHandle_t loopSfx;
	qboolean startLoopingSound;
} sentity_t;

static vec3_t    entityPositions[MAX_GENTITIES];
static sentity_t loopSounds[MAX_LOOP_SOUNDS];
static int       numLoopingSounds;

#define S_AL_SanitiseVector(v) _S_AL_SanitiseVector(v, __LINE__)

/**
 * @brief _S_AL_SanitiseVector
 * @param[in] v
 * @param[in] line
 */
static void _S_AL_SanitiseVector(vec3_t v, int line)
{
	if (Q_isnan(v[0]) || Q_isnan(v[1]) || Q_isnan(v[2]))
	{
		Com_DPrintf(S_COLOR_YELLOW "WARNING _S_AL_SanitiseVector: vector with one or more NaN components "
		                           "being passed to OpenAL at %s:%d -- zeroing\n", __FILE__, line);
		VectorClear(v);
	}
}

#define AL_THIRD_PERSON_THRESHOLD_SQ (48.0f * 48.0f)

/**
 * @brief Set gain to 0 if muted, otherwise set it to given value.
 * @param[in] source
 * @param[in] gainval
 */
static void S_AL_Gain(ALuint source, float gainval)
{
	if (s_muted->integer)
	{
		qalSourcef(source, AL_GAIN, 0.0f);
	}
	else
	{
		qalSourcef(source, AL_GAIN, gainval);
	}
}

/**
 * @brief Adapt the gain if necessary to get a quicker fadeout when the source is too far away.
 * @param[in,out] chksrc
 * @param[in] origin
 */
static void S_AL_ScaleGain(src_t *chksrc, vec3_t origin)
{
	float distance = 0.0f;

	if (!chksrc->local)
	{
		distance = vec3_distance(origin, lastListenerOrigin);
	}

	// If we exceed a certain distance, scale the gain linearly until the sound
	// vanishes into nothingness.
	if (!chksrc->local && (distance -= s_alMaxDistance->value) > 0)
	{
		float scaleFactor;

		if (distance >= s_alGraceDistance->value)
		{
			scaleFactor = 0.0f;
		}
		else
		{
			scaleFactor = 1.0f - distance / s_alGraceDistance->value;
		}

		scaleFactor *= chksrc->curGain;

		if (chksrc->scaleGain != scaleFactor)
		{
			chksrc->scaleGain = scaleFactor;
			S_AL_Gain(chksrc->alSource, chksrc->scaleGain);
		}
	}
	else if (chksrc->scaleGain != chksrc->curGain)
	{
		chksrc->scaleGain = chksrc->curGain;
		S_AL_Gain(chksrc->alSource, chksrc->scaleGain);
	}
}

/**
 * @brief S_AL_HearingThroughEntity
 * @param[in] entityNum
 * @return
 */
static qboolean S_AL_HearingThroughEntity(int entityNum)
{
	if (clc.clientNum == entityNum)
	{
		float distanceSq;

		// FIXME: <tim@ngus.net> 28/02/06 This is an outrageous hack to detect
		// whether or not the player is rendering in third person or not. We can't
		// ask the renderer because the renderer has no notion of entities and we
		// can't ask cgame since that would involve changing the API and hence mod
		// compatibility. I don't think there is any way around this, but I'll leave
		// the FIXME just in case anyone has a bright idea.
		distanceSq = vec3_distance_squared(entityPositions[entityNum], lastListenerOrigin);

		if (distanceSq > AL_THIRD_PERSON_THRESHOLD_SQ)
		{
			return qfalse; // we're the player, but third person
		}
		else
		{
			return qtrue;  // we're the player
		}
	}
	else
	{
		return qfalse; // not the player
	}
}

/**
 * @brief S_AL_SrcInit
 * @return
 */
static qboolean S_AL_SrcInit(void)
{
	int    i;
	int    limit;
	ALenum error;

	// Clear the sources data structure
	Com_Memset(srcList, 0, sizeof(srcList));
	srcCount     = 0;
	srcActiveCnt = 0;

	// Cap s_alSources to MAX_SRC
	limit = s_alSources->integer;
	if (limit > MAX_SRC)
	{
		limit = MAX_SRC;
	}
	else if (limit < 16)
	{
		limit = 16;
	}

	S_AL_ClearError(qfalse);
	// Allocate as many sources as possible
	for (i = 0; i < limit; i++)
	{
		qalGenSources(1, &srcList[i].alSource);
		if ((error = qalGetError()) != AL_NO_ERROR)
		{
			break;
		}
		srcCount++;
	}

	// All done. Print this for informational purposes
	Com_Printf("Allocated %d sources.\n", srcCount);
	alSourcesInitialised = qtrue;
	return qtrue;
}

/**
 * @brief S_AL_SrcShutdown
 */
static void S_AL_SrcShutdown(void)
{
	int   i;
	src_t *curSource;

	if (!alSourcesInitialised)
	{
		return;
	}

	// Destroy all the sources
	for (i = 0; i < srcCount; i++)
	{
		curSource = &srcList[i];

		if (curSource->isLocked)
		{
			Com_DPrintf(S_COLOR_YELLOW "WARNING S_AL_SrcShutdown: Source %d is locked\n", i);
		}

		if (curSource->entity > 0 && curSource->isLooping)
		{
			loopSounds[curSource->entity].srcAllocated = qfalse;
		}

		qalSourceStop(srcList[i].alSource);
		qalDeleteSources(1, &srcList[i].alSource);
	}

	Com_Memset(srcList, 0, sizeof(srcList));

	alSourcesInitialised = qfalse;
}

/**
 * @brief S_AL_SrcSetup
 * @param[in] src
 * @param[in] sfx
 * @param[in] priority
 * @param[in] entity
 * @param[in] channel
 * @param[in] local
 * @param[in] volume
 */
static void S_AL_SrcSetup(srcHandle_t src, sfxHandle_t sfx, alSrcPriority_t priority,
                          int entity, int channel, qboolean local, int volume)
{
	ALuint buffer;
	src_t  *curSource;

	// Mark the SFX as used, and grab the raw AL buffer
	S_AL_BufferUse(sfx);
	buffer = S_AL_BufferGet(sfx);

	// Set up src struct
	curSource = &srcList[src];

	curSource->lastUsedTime = Sys_Milliseconds();
	curSource->sfx          = sfx;
	curSource->priority     = priority;
	curSource->entity       = entity;
	curSource->channel      = channel;
	curSource->isPlaying    = qfalse;
	curSource->isLocked     = qfalse;
	curSource->isLooping    = qfalse;
	curSource->isTracking   = qfalse;
	curSource->curGain      = s_alGain->value * s_volume->value * ((float)volume / 255.0f);
	curSource->scaleGain    = curSource->curGain;
	curSource->local        = local;

	// Set up OpenAL source
	qalSourcei(curSource->alSource, AL_BUFFER, buffer);
	qalSourcef(curSource->alSource, AL_PITCH, 1.0f);
	S_AL_Gain(curSource->alSource, curSource->curGain);
	qalSourcefv(curSource->alSource, AL_POSITION, vec3_origin);
	qalSourcefv(curSource->alSource, AL_VELOCITY, vec3_origin);
	qalSourcei(curSource->alSource, AL_LOOPING, AL_FALSE);
	qalSourcef(curSource->alSource, AL_REFERENCE_DISTANCE, s_alMinDistance->value);

	if (local)
	{
		qalSource3f(curSource->alSource, AL_POSITION, 0, 0, -1);
		qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_TRUE);
		qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, 0.0f);
	}
	else
	{
#ifdef AL_EFFECT_DEDICATED_LOW_FREQUENCY_EFFECT
		qalSource3i(curSource->alSource, AL_AUXILIARY_SEND_FILTER, auxslot[QAL_EFX_DEDICATED_LFE], 0, AL_FILTER_NULL);
#endif
		qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_FALSE);
		qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value);
	}
}

/**
 * @brief Remove given source as loop master if it is the master and hand off master status to another source in this case.
 * @param dest
 * @param alSource
 */
static void S_AL_SaveLoopPos(src_t *dest, ALuint alSource)
{
	int error;

	S_AL_ClearError(qfalse);

	qalGetSourcef(alSource, AL_SEC_OFFSET, &dest->lastTimePos);
	if ((error = qalGetError()) != AL_NO_ERROR)
	{
		// Old OpenAL implementations don't support AL_SEC_OFFSET

		if (error != AL_INVALID_ENUM)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING S_AL_SaveLoopPos: Could not get time offset for alSource %d: %s\n",
			           alSource, S_AL_ErrorMsg(error));
		}

		dest->lastTimePos = -1;
	}
	else
	{
		dest->lastSampleTime = Sys_Milliseconds();
	}
}

/**
 * @brief Remove given source as loop master if it is the master and hand off master status to another source in this case.
 * @param rmSource
 * @param iskilled
 */
static void S_AL_NewLoopMaster(src_t *rmSource, qboolean iskilled)
{
	alSfx_t *curSfx = &knownSfx[rmSource->sfx];

	if (rmSource->isPlaying)
	{
		curSfx->loopActiveCnt--;
	}
	if (iskilled)
	{
		curSfx->loopCnt--;
	}

	if (curSfx->loopCnt)
	{
		if (rmSource->priority == SRCPRI_ENTITY)
		{
			if (!iskilled && rmSource->isPlaying)
			{
				// only sync ambient loops...
				// It makes more sense to have sounds for weapons/projectiles unsynced
				S_AL_SaveLoopPos(rmSource, rmSource->alSource);
			}
		}
		else if (curSfx->masterLoopSrc != -1 && rmSource == &srcList[curSfx->masterLoopSrc])
		{
			src_t *curSource    = NULL;
			int   firstInactive = -1;

			// Only if rmSource was the master and if there are still playing loops for
			// this sound will we need to find a new master.

			if (iskilled || curSfx->loopActiveCnt)
			{
				int index;

				for (index = 0; index < srcCount; index++)
				{
					curSource = &srcList[index];

					if (curSource->sfx == rmSource->sfx && curSource != rmSource &&
					    curSource->isActive && curSource->isLooping && curSource->priority == SRCPRI_AMBIENT)
					{
						if (curSource->isPlaying)
						{
							curSfx->masterLoopSrc = index;
							break;
						}
						else if (firstInactive < 0)
						{
							firstInactive = index;
						}
					}
				}
			}

			if (!curSfx->loopActiveCnt)
			{
				if (firstInactive < 0)
				{
					if (iskilled)
					{
						curSfx->masterLoopSrc = -1;
						return;
					}
					else
					{
						curSource = rmSource;
					}
				}
				else
				{
					curSource = &srcList[firstInactive];
				}

				if (rmSource->isPlaying)
				{
					// this was the last not stopped source, save last sample position + time
					S_AL_SaveLoopPos(curSource, rmSource->alSource);
				}
				else
				{
					// second case: all loops using this sound have stopped due to listener being of of range,
					// and now the inactive master gets deleted. Just move over the soundpos settings to the
					// new master.
					curSource->lastTimePos    = rmSource->lastTimePos;
					curSource->lastSampleTime = rmSource->lastSampleTime;
				}
			}
		}
	}
	else
	{
		curSfx->masterLoopSrc = -1;
	}
}

/**
 * @brief S_AL_SrcKill
 * @param[in] src
 */
static void S_AL_SrcKill(srcHandle_t src)
{
	src_t *curSource = &srcList[src];

	// I'm not touching it. Unlock it first.
	if (curSource->isLocked)
	{
		return;
	}

	// Remove the entity association and loop master status
	if (curSource->isLooping)
	{
		curSource->isLooping = qfalse;

		if (curSource->entity != -1)
		{
			sentity_t *curEnt = &loopSounds[curSource->entity];

			curEnt->srcAllocated       = qfalse;
			curEnt->srcIndex           = -1;
			curEnt->loopAddedThisFrame = qfalse;
			curEnt->startLoopingSound  = qfalse;
		}

		S_AL_NewLoopMaster(curSource, qtrue);
	}

	// Stop it if it's playing
	if (curSource->isPlaying)
	{
		qalSourceStop(curSource->alSource);
		curSource->isPlaying = qfalse;
	}

	// Remove the buffer
	qalSourcei(curSource->alSource, AL_BUFFER, 0);

	curSource->sfx          = 0;
	curSource->lastUsedTime = 0;
	curSource->priority     = 0;
	curSource->entity       = -1;
	curSource->channel      = -1;
	if (curSource->isActive)
	{
		curSource->isActive = qfalse;
		srcActiveCnt--;
	}
	curSource->isLocked   = qfalse;
	curSource->isTracking = qfalse;
	curSource->local      = qfalse;
}

/**
 * @brief S_AL_SrcAlloc
 * @param[in] priority
 * @param entnum - unused
 * @param channel - unused
 * @return
 */
static srcHandle_t S_AL_SrcAlloc(alSrcPriority_t priority, int entnum, int channel)
{
	int      i;
	int      empty             = -1;
	int      weakest           = -1;
	int      weakest_time      = Sys_Milliseconds();
	int      weakest_pri       = 999;
	float    weakest_gain      = 1000.0;
	qboolean weakest_isplaying = qtrue;
	int      weakest_numloops  = 0;
	src_t    *curSource;

	for (i = 0; i < srcCount; i++)
	{
		curSource = &srcList[i];

		// If it's locked, we aren't even going to look at it
		if (curSource->isLocked)
		{
			continue;
		}

		// Is it empty or not?
		if (!curSource->isActive)
		{
			empty = i;
			break;
		}

		if (curSource->isPlaying)
		{
			if (weakest_isplaying && curSource->priority < priority &&
			    (curSource->priority < weakest_pri ||
			     (!curSource->isLooping && (curSource->scaleGain < weakest_gain || curSource->lastUsedTime < weakest_time))))
			{
				// If it has lower priority, is fainter or older, flag it as weak
				// the last two values are only compared if it's not a looping sound, because we want to prevent two
				// loops (loops are added EVERY frame) fighting for a slot
				weakest_pri  = curSource->priority;
				weakest_time = curSource->lastUsedTime;
				weakest_gain = curSource->scaleGain;
				weakest      = i;
			}
		}
		else
		{
			weakest_isplaying = qfalse;

			if (weakest < 0 ||
			    knownSfx[curSource->sfx].loopCnt > weakest_numloops ||
			    curSource->priority < weakest_pri ||
			    curSource->lastUsedTime < weakest_time)
			{
				// Sources currently not playing of course have lowest priority
				// also try to always keep at least one loop master for every loop sound
				weakest_pri      = curSource->priority;
				weakest_time     = curSource->lastUsedTime;
				weakest_numloops = knownSfx[curSource->sfx].loopCnt;
				weakest          = i;
			}
		}

		// The channel system is not actually adhered to by etmain, and not
		// implemented in snd_dma.c, so while the following is strictly correct, it
		// causes incorrect behaviour versus defacto etmain
#if 0
		// Is it an exact match, and not on channel 0?
		if ((curSource->entity == entnum) && (curSource->channel == channel) && (channel != 0))
		{
			S_AL_SrcKill(i);
			return i;
		}
#endif
	}

	if (empty == -1)
	{
		empty = weakest;
	}

	if (empty >= 0)
	{
		S_AL_SrcKill(empty);
		srcList[empty].isActive = qtrue;
		srcActiveCnt++;
	}

	return empty;
}

/*
 * @brief Finds an active source with matching entity and channel numbers
 * Returns -1 if there isn't one
 * @param entnum
 * @param channel
 * @return
 *
 * @note Unused
static srcHandle_t S_AL_SrcFind(int entnum, int channel)
{
    int i;

    for (i = 0; i < srcCount; i++)
    {
        if (!srcList[i].isActive)
        {
            continue;
        }
        if ((srcList[i].entity == entnum) && (srcList[i].channel == channel))
        {
            return i;
        }
    }
    return -1;
}
*/

/**
 * @brief Locked sources will not be automatically reallocated or managed
 * @param[in] src
 */
static void S_AL_SrcLock(srcHandle_t src)
{
	srcList[src].isLocked = qtrue;
}

/**
 * @brief Once unlocked, the source may be reallocated again
 * @param src
 */
static void S_AL_SrcUnlock(srcHandle_t src)
{
	srcList[src].isLocked = qfalse;
}

/**
 * @brief S_AL_UpdateEntityPosition
 * @param[in] entityNum
 * @param[in] origin
 */
static void S_AL_UpdateEntityPosition(int entityNum, const vec3_t origin)
{
	vec3_t sanOrigin;

	VectorCopy(origin, sanOrigin);
	S_AL_SanitiseVector(sanOrigin);
	if (entityNum < 0 || entityNum >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum);
	}
	VectorCopy(sanOrigin, entityPositions[entityNum]);
}

/**
 * @brief Check whether input values from mods are out of range.
 * Necessary for i.g. Western Quake3 mod which is buggy.
 * @param[in] entityNum
 * @param[in] sfx
 * @return
 */
static qboolean S_AL_CheckInput(int entityNum, sfxHandle_t sfx)
{
	if (entityNum < 0 || entityNum >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "S_AL_CheckInput: bad entitynum %i", entityNum);
	}

	if (sfx < 0 || sfx >= numSfx)
	{
		Com_Printf(S_COLOR_RED "ERROR S_AL_CheckInput: S_AL_CheckInput: handle %i out of range\n", sfx);
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Play a local (non-spatialized) sound effect
 * @param[in] sfx
 * @param[in] channel
 * @param[in] volume
 */
static void S_AL_StartLocalSound(sfxHandle_t sfx, int channel, int volume)
{
	srcHandle_t src;

	if (S_AL_CheckInput(0, sfx))
	{
		return;
	}

	// Try to grab a source
	src = S_AL_SrcAlloc(SRCPRI_LOCAL, -1, channel);

	if (src == -1)
	{
		return;
	}

	// Set up the effect
	S_AL_SrcSetup(src, sfx, SRCPRI_LOCAL, -1, channel, qtrue, volume);

	// Start it playing
	srcList[src].isPlaying = qtrue;
	qalSourcePlay(srcList[src].alSource);
}

/**
 * @brief Play a one-shot sound effect
 * @param[in] origin
 * @param[in] entnum
 * @param[in] entchannel
 * @param[in] sfx
 * @param flags - unused
 * @param[in] volume
 */
static void S_AL_StartSoundEx(vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx, int flags, int volume)
{
	vec3_t      sorigin;
	srcHandle_t src;
	src_t       *curSource;

	if (origin)
	{
		if (S_AL_CheckInput(0, sfx))
		{
			return;
		}

		VectorCopy(origin, sorigin);
	}
	else
	{
		if (S_AL_CheckInput(entnum, sfx))
		{
			return;
		}

		if (S_AL_HearingThroughEntity(entnum))
		{
			S_AL_StartLocalSound(sfx, entchannel, volume);
			return;
		}

		VectorCopy(entityPositions[entnum], sorigin);
	}

	S_AL_SanitiseVector(sorigin);

	if ((srcActiveCnt > 5 * srcCount / 3) &&
	    (vec3_distance_squared(sorigin, lastListenerOrigin) >=
	     (s_alMaxDistance->value + s_alGraceDistance->value) * (s_alMaxDistance->value + s_alGraceDistance->value)))
	{
		// We're getting tight on sources and source is not within hearing distance so don't add it
		return;
	}

	// Try to grab a source
	src = S_AL_SrcAlloc(SRCPRI_ONESHOT, entnum, entchannel);
	if (src == -1)
	{
		return;
	}

	S_AL_SrcSetup(src, sfx, SRCPRI_ONESHOT, entnum, entchannel, qfalse, volume);

	curSource = &srcList[src];

	if (!origin)
	{
		curSource->isTracking = qtrue;
	}

	qalSourcefv(curSource->alSource, AL_POSITION, sorigin);
	S_AL_ScaleGain(curSource, sorigin);

	// Start it playing
	curSource->isPlaying = qtrue;
	qalSourcePlay(curSource->alSource);
}

/**
 * @brief Play a one-shot sound effect
 * @param[in] origin
 * @param[in] entnum
 * @param[in] entchannel
 * @param[in] sfx
 * @param[in] volume
 */
static void S_AL_StartSound(vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx, int volume)
{
	S_AL_StartSoundEx(origin, entnum, entchannel, sfx, 0, volume);
}

/**
 * @brief S_AL_ClearLoopingSounds
 */
static void S_AL_ClearLoopingSounds(void)
{
	int i;

	for (i = 0; i < numLoopingSounds; i++)
	{
		loopSounds[i].loopAddedThisFrame = qfalse;
	}
	numLoopingSounds = 0;
}

/**
 * @brief S_AL_SrcLoop
 * @param[in] priority
 * @param[in] sfx
 * @param[in] origin
 * @param[in] velocity
 * @param range - unused
 * @param[in] volume
 * @param soundTime - unused
 */
static void S_AL_SrcLoop(alSrcPriority_t priority, sfxHandle_t sfx,
                         const vec3_t origin, const vec3_t velocity, int range, int volume, int soundTime)
{
	int       src;
	sentity_t *sent = NULL;
	src_t     *curSource;
	vec3_t    sorigin, svelocity;

	//TODO: implement soundTime
	//if(S_AL_CheckInput(entityNum, sfx))
	//  return;

	if (numLoopingSounds >= MAX_LOOP_SOUNDS)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING S_AL_SrcLoop: Failed to allocate loop sfx %d.", sfx);
		return;
	}
	sent = &loopSounds[numLoopingSounds++];

	// Do we need to allocate a new source for this entity
	if (!sent->srcAllocated)
	{
		// Try to get a channel
		src = S_AL_SrcAlloc(priority, -1, -1);
		if (src == -1)
		{
			Com_DPrintf(S_COLOR_YELLOW "WARNING S_AL_SrcLoop: Failed to allocate source "
			                           "for loop sfx %d\n on loop sound %d.", sfx, numLoopingSounds - 1);
			return;
		}

		curSource = &srcList[src];

		sent->startLoopingSound = qtrue;

		curSource->lastTimePos    = -1.0;
		curSource->lastSampleTime = Sys_Milliseconds();
	}
	else
	{
		src       = sent->srcIndex;
		curSource = &srcList[src];
	}

	VectorCopy(origin, sent->origin);
	sent->srcAllocated = qtrue;
	sent->srcIndex     = src;

	sent->loopPriority = priority;
	sent->loopSfx      = sfx;
	sent->volume       = volume;

	// If this is not set then the looping sound is stopped.
	sent->loopAddedThisFrame = qtrue;

	// UGH
	// These lines should be called via S_AL_SrcSetup, but we
	// can't call that yet as it buffers sfxes that may change
	// with subsequent calls to S_AL_SrcLoop
	curSource->entity    = numLoopingSounds - 1;
	curSource->isLooping = qtrue;

	curSource->local = qfalse;

	VectorCopy(origin, sorigin);

	S_AL_SanitiseVector(sorigin);

	VectorCopy(sorigin, curSource->loopSpeakerPos);

	if (velocity)
	{
		VectorCopy(velocity, svelocity);
		S_AL_SanitiseVector(svelocity);
	}
	else
	{
		VectorClear(svelocity);
	}

	qalSourcefv(curSource->alSource, AL_POSITION, (ALfloat *)sorigin);
	qalSourcefv(curSource->alSource, AL_VELOCITY, (ALfloat *)velocity);
}

/**
 * @brief S_AL_AddLoopingSound
 * @param[in] origin
 * @param[in] velocity
 * @param[in] range
 * @param[in] sfx
 * @param[in] volume
 * @param[in] soundTime
 */
static void S_AL_AddLoopingSound(const vec3_t origin, const vec3_t velocity, int range, sfxHandle_t sfx, int volume, int soundTime)
{
	S_AL_SrcLoop(SRCPRI_ENTITY, sfx, origin, velocity, range, volume, soundTime);
}


/**
 * @brief S_AL_AddRealLoopingSound
 * @param[in] origin
 * @param[in] velocity
 * @param[in] range
 * @param[in] sfx
 * @param[in] volume
 * @param[in] soundTime
 */
static void S_AL_AddRealLoopingSound(const vec3_t origin, const vec3_t velocity, int range, sfxHandle_t sfx, int volume, int soundTime)
{
	S_AL_SrcLoop(SRCPRI_AMBIENT, sfx, origin, velocity, range, volume, soundTime);
}

/**
 * @brief Update state (move things around, manage sources, and so on)
 */
static void S_AL_SrcUpdate(void)
{
	int   i;
	int   entityNum;
	ALint state;
	src_t *curSource;

	for (i = 0; i < srcCount; i++)
	{
		entityNum = srcList[i].entity;
		curSource = &srcList[i];

		if (curSource->isLocked)
		{
			continue;
		}

		if (!curSource->isActive)
		{
			continue;
		}

		// Update source parameters
		if ((s_alGain->modified) || (s_volume->modified))
		{
			curSource->curGain = s_alGain->value * s_volume->value;
		}
		if ((s_alRolloff->modified) && (!curSource->local))
		{
			qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value);
		}
		if (s_alMinDistance->modified)
		{
			qalSourcef(curSource->alSource, AL_REFERENCE_DISTANCE, s_alMinDistance->value);
		}

		if (curSource->isLooping)
		{
			sentity_t *sent = &loopSounds[entityNum];

			// If a looping effect hasn't been touched this frame, pause or kill it
			if (sent->loopAddedThisFrame)
			{
				alSfx_t *curSfx;

				// The sound has changed without an intervening removal
				if (curSource->isActive && !sent->startLoopingSound &&
				    curSource->sfx != sent->loopSfx)
				{
					S_AL_NewLoopMaster(curSource, qtrue);

					curSource->isPlaying = qfalse;
					qalSourceStop(curSource->alSource);
					qalSourcei(curSource->alSource, AL_BUFFER, 0);
					sent->startLoopingSound = qtrue;
				}

				// The sound hasn't been started yet
				if (sent->startLoopingSound)
				{
					S_AL_SrcSetup(i, sent->loopSfx, sent->loopPriority,
					              entityNum, -1, curSource->local, sent->volume);
					curSource->isLooping = qtrue;

					knownSfx[curSource->sfx].loopCnt++;
					sent->startLoopingSound = qfalse;
				}

				curSfx = &knownSfx[curSource->sfx];

				S_AL_ScaleGain(curSource, curSource->loopSpeakerPos);
				if (curSource->scaleGain == 0.f)
				{
					if (curSource->isPlaying)
					{
						// Sound is mute, stop playback until we are in range again
						S_AL_NewLoopMaster(curSource, qfalse);
						qalSourceStop(curSource->alSource);
						curSource->isPlaying = qfalse;
					}
					else if (!curSfx->loopActiveCnt && curSfx->masterLoopSrc < 0)
					{
						curSfx->masterLoopSrc = i;
					}

					continue;
				}

				if (!curSource->isPlaying)
				{
					qalSourcei(curSource->alSource, AL_LOOPING, AL_TRUE);
					curSource->isPlaying = qtrue;
					qalSourcePlay(curSource->alSource);

					if (curSource->priority == SRCPRI_AMBIENT)
					{
						// If there are other ambient looping sources with the same sound,
						// make sure the sound of these sources are in sync.

						if (curSfx->loopActiveCnt)
						{
							int offset, error;

							// we already have a master loop playing, get buffer position.
							S_AL_ClearError(qfalse);
							qalGetSourcei(srcList[curSfx->masterLoopSrc].alSource, AL_SAMPLE_OFFSET, &offset);
							if ((error = qalGetError()) != AL_NO_ERROR)
							{
								if (error != AL_INVALID_ENUM)
								{
									Com_Printf(S_COLOR_YELLOW "WARNING S_AL_SrcUpdate: Cannot get sample offset from source %d: "
									                          "%s\n", i, S_AL_ErrorMsg(error));
								}
							}
							else
							{
								qalSourcei(curSource->alSource, AL_SAMPLE_OFFSET, offset);
							}
						}
						else if (curSfx->loopCnt && curSfx->masterLoopSrc >= 0)
						{
							float secofs;

							src_t *master = &srcList[curSfx->masterLoopSrc];
							// This loop sound used to be played, but all sources are stopped. Use last sample position/time
							// to calculate offset so the player thinks the sources continued playing while they were inaudible.

							if (master->lastTimePos >= 0)
							{
								secofs = master->lastTimePos + (Sys_Milliseconds() - master->lastSampleTime) / 1000.0f;
								secofs = fmodf(secofs, (float) curSfx->info.samples / curSfx->info.rate);

								qalSourcef(curSource->alSource, AL_SEC_OFFSET, secofs);
							}

							// I be the master now
							curSfx->masterLoopSrc = i;
						}
						else
						{
							curSfx->masterLoopSrc = i;
						}
					}
					else if (curSource->lastTimePos >= 0)
					{
						float secofs;

						// For unsynced loops (SRCPRI_ENTITY) just carry on playing as if the sound was never stopped

						secofs = curSource->lastTimePos + (Sys_Milliseconds() - curSource->lastSampleTime) / 1000.0f;
						secofs = fmodf(secofs, (float) curSfx->info.samples / curSfx->info.rate);
						qalSourcef(curSource->alSource, AL_SEC_OFFSET, secofs);
					}

					curSfx->loopActiveCnt++;
				}

				// Update locality
				if (curSource->local)
				{
					qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_TRUE);
					qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, 0.0f);
				}
				else
				{
					qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_FALSE);
					qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value);
				}

			}
			else if (curSource->priority == SRCPRI_AMBIENT)
			{
				if (curSource->isPlaying)
				{
					S_AL_NewLoopMaster(curSource, qfalse);
					qalSourceStop(curSource->alSource);
					curSource->isPlaying = qfalse;
				}
			}
			else
			{
				S_AL_SrcKill(i);
			}

			continue;
		}

		// Check if it's done, and flag it
		qalGetSourcei(curSource->alSource, AL_SOURCE_STATE, &state);
		if (state == AL_STOPPED)
		{
			curSource->isPlaying = qfalse;
			S_AL_SrcKill(i);
			continue;
		}

		// Query relativity of source, don't move if it's true
		qalGetSourcei(curSource->alSource, AL_SOURCE_RELATIVE, &state);

		// See if it needs to be moved
		if (curSource->isTracking && !state)
		{
			qalSourcefv(curSource->alSource, AL_POSITION, entityPositions[entityNum]);
			S_AL_ScaleGain(curSource, entityPositions[entityNum]);
		}
	}
}

/**
 * @brief S_AL_SrcShutup
 */
static void S_AL_SrcShutup(void)
{
	int i;

	for (i = 0; i < srcCount; i++)
	{
		S_AL_SrcKill(i);
	}
}

/**
 * @brief S_AL_SrcGet
 * @param src
 * @return
 */
static ALuint S_AL_SrcGet(srcHandle_t src)
{
	return srcList[src].alSource;
}

//===========================================================================

static srcHandle_t streamSourceHandles[MAX_RAW_STREAMS];
static qboolean    streamPlaying[MAX_RAW_STREAMS];
static ALuint      streamSources[MAX_RAW_STREAMS];

/**
 * @brief S_AL_AllocateStreamChannel
 * @param[in] stream
 */
static void S_AL_AllocateStreamChannel(int stream)
{
	if (stream < 0 || stream >= MAX_RAW_STREAMS)
	{
		return;
	}

	// Allocate a streamSource at high priority
	streamSourceHandles[stream] = S_AL_SrcAlloc(SRCPRI_STREAM, -2, 0);
	if (streamSourceHandles[stream] == -1)
	{
		return;
	}

	// Lock the streamSource so nobody else can use it, and get the raw streamSource
	S_AL_SrcLock(streamSourceHandles[stream]);
	streamSources[stream] = S_AL_SrcGet(streamSourceHandles[stream]);

	// make sure that after unmuting the S_AL_Gain in S_Update() does not turn
	// volume up prematurely for this source
	srcList[streamSourceHandles[stream]].scaleGain = 0.0f;

	// Set some streamSource parameters
	qalSourcei(streamSources[stream], AL_BUFFER, 0);
	qalSourcei(streamSources[stream], AL_LOOPING, AL_FALSE);
	qalSource3f(streamSources[stream], AL_POSITION, 0.0, 0.0, 0.0);
	qalSource3f(streamSources[stream], AL_VELOCITY, 0.0, 0.0, 0.0);
	qalSource3f(streamSources[stream], AL_DIRECTION, 0.0, 0.0, 0.0);
	qalSourcef(streamSources[stream], AL_ROLLOFF_FACTOR, 0.0);
	qalSourcei(streamSources[stream], AL_SOURCE_RELATIVE, AL_TRUE);
}

/**
 * @brief S_AL_FreeStreamChannel
 * @param[in] stream
 */
static void S_AL_FreeStreamChannel(int stream)
{
	if (stream < 0 || stream >= MAX_RAW_STREAMS)
	{
		return;
	}

	// Release the output streamSource
	S_AL_SrcUnlock(streamSourceHandles[stream]);
	streamSources[stream]       = 0;
	streamSourceHandles[stream] = -1;
}

/**
 * @brief S_AL_RawSamples
 * @param[in] stream
 * @param[in] samples
 * @param[in] rate
 * @param[in] width
 * @param[in] channels
 * @param[out] data
 * @param[in] lvol
 * @param[in] rvol
 */
static void S_AL_RawSamples(int stream, int samples, int rate, int width, int channels, const byte *data, float lvol, float rvol)
{
	ALuint buffer;
	ALuint format;
	// TODO: Ugh Gain on either channel???
	float volume = (lvol + rvol) / 2;

	if (stream < 0 || stream >= MAX_RAW_STREAMS)
	{
		return;
	}

	format = S_AL_Format(width, channels);

	// Create the streamSource if necessary
	if (streamSourceHandles[stream] == -1)
	{
		S_AL_AllocateStreamChannel(stream);

		// Failed?
		if (streamSourceHandles[stream] == -1)
		{
			Com_Printf(S_COLOR_RED "ERROR S_AL_RawSamples: Can't allocate streaming streamSource\n");
			return;
		}
	}

	// Create a buffer, and stuff the data into it
	qalGenBuffers(1, &buffer);
	qalBufferData(buffer, format, (ALvoid *)data, (samples * width * channels), rate);

	// Shove the data onto the streamSource
	qalSourceQueueBuffers(streamSources[stream], 1, &buffer);

	// Volume
	S_AL_Gain(streamSources[stream], volume * s_volume->value * s_alGain->value);
}

/**
 * @brief S_AL_StreamUpdate
 * @param[in] stream
 */
static void S_AL_StreamUpdate(int stream)
{
	int   numBuffers;
	ALint state;

	if (stream < 0 || stream >= MAX_RAW_STREAMS)
	{
		return;
	}

	if (streamSourceHandles[stream] == -1)
	{
		return;
	}

	// Un-queue any buffers, and delete them
	qalGetSourcei(streamSources[stream], AL_BUFFERS_PROCESSED, &numBuffers);
	while (numBuffers--)
	{
		ALuint buffer;
		qalSourceUnqueueBuffers(streamSources[stream], 1, &buffer);
		qalDeleteBuffers(1, &buffer);
	}

	// Start the streamSource playing if necessary
	qalGetSourcei(streamSources[stream], AL_BUFFERS_QUEUED, &numBuffers);

	qalGetSourcei(streamSources[stream], AL_SOURCE_STATE, &state);
	if (state == AL_STOPPED)
	{
		streamPlaying[stream] = qfalse;

		// If there are no buffers queued up, release the streamSource
		if (!numBuffers)
		{
			S_AL_FreeStreamChannel(stream);
		}
	}

	if (!streamPlaying[stream] && numBuffers)
	{
		qalSourcePlay(streamSources[stream]);
		streamPlaying[stream] = qtrue;
	}
}

/**
 * @brief S_AL_StreamDie
 * @param[in] stream
 */
static void S_AL_StreamDie(int stream)
{
	int numBuffers;

	if (stream < 0 || stream >= MAX_RAW_STREAMS)
	{
		return;
	}

	if (streamSourceHandles[stream] == -1)
	{
		return;
	}

	streamPlaying[stream] = qfalse;
	qalSourceStop(streamSources[stream]);

	// Un-queue any buffers, and delete them
	qalGetSourcei(streamSources[stream], AL_BUFFERS_PROCESSED, &numBuffers);
	while (numBuffers--)
	{
		ALuint buffer;
		qalSourceUnqueueBuffers(streamSources[stream], 1, &buffer);
		qalDeleteBuffers(1, &buffer);
	}

	S_AL_FreeStreamChannel(stream);
}

//===========================================================================

#define NUM_STREAM_BUFFERS  4
#define STREAM_BUFFER_SIZE 4096

static int              ssMusic = -1;
static qboolean         ssPlaying[MAX_STREAMING_SOUNDS];
static qboolean         ssKill[MAX_STREAMING_SOUNDS];
static unsigned int     ssRestart[MAX_STREAMING_SOUNDS];
static streamingSound_t ssData[MAX_STREAMING_SOUNDS];
static srcHandle_t      ssSourceHandle[MAX_STREAMING_SOUNDS];
static ALuint           ssSource[MAX_STREAMING_SOUNDS];
static ALuint           ssBuffers[MAX_STREAMING_SOUNDS][NUM_STREAM_BUFFERS];

static byte decode_buffer[STREAM_BUFFER_SIZE];

/**
 * @brief S_AL_SSSourceGet
 * @return
 */
static int S_AL_SSSourceGet(void)
{
	int i = 0;

	// Find a source not playing
	for (i = 0; i < MAX_STREAMING_SOUNDS && ssPlaying[i]; i++)
		;

	// Allocate a musicSource at high priority
	ssSourceHandle[i] = S_AL_SrcAlloc(SRCPRI_STREAM, -2, 0);
	if (ssSourceHandle[i] == -1)
	{
		return -1;
	}

	// Lock the musicSource so nobody else can use it, and get the raw musicSource
	S_AL_SrcLock(ssSourceHandle[i]);
	ssSource[i] = S_AL_SrcGet(ssSourceHandle[i]);

	// make sure that after unmuting the S_AL_Gain in S_Update() does not turn
	// volume up prematurely for this source
	srcList[ssSourceHandle[i]].scaleGain = 0.0f;

	// Set some stream source parameters
	qalSource3f(ssSource[i], AL_POSITION, 0.0, 0.0, 0.0);
	qalSource3f(ssSource[i], AL_VELOCITY, 0.0, 0.0, 0.0);
	qalSource3f(ssSource[i], AL_DIRECTION, 0.0, 0.0, 0.0);
	qalSourcef(ssSource[i], AL_ROLLOFF_FACTOR, 0.0);
	qalSourcei(ssSource[i], AL_SOURCE_RELATIVE, AL_TRUE);
	qalSourcei(ssSource[i], AL_LOOPING, AL_FALSE);
	return i;
}

/**
 * @brief S_AL_SSSourceFree
 * @param[in] ss
 */
static void S_AL_SSSourceFree(int ss)
{
	// Release the output streaming sound source
	S_AL_SrcUnlock(ssSourceHandle[ss]);
	S_AL_SrcKill(ssSourceHandle[ss]);
	ssSource[ss]       = 0;
	ssSourceHandle[ss] = -1;
}

/**
 * @brief S_AL_CloseSSFiles
 * @param[in] ss
 */
static void S_AL_CloseSSFiles(int ss)
{
	if (ssData[ss].stream)
	{
		S_CodecCloseStream(ssData[ss].stream);
		ssData[ss].stream = NULL;
	}
}

/**
 * @brief S_AL_StopStreamingSound
 * @param[in] ss
 */
static void S_AL_StopStreamingSound(int ss)
{
    ssRestart[ss] = 0;
	if (!ssPlaying[ss])
	{
		return;
	}

	// Stop playing
	qalSourceStop(ssSource[ss]);

	// De-queue the musicBuffers
	qalSourceUnqueueBuffers(ssSource[ss], NUM_STREAM_BUFFERS, ssBuffers[ss]);

	// Destroy the musicBuffers
	qalDeleteBuffers(NUM_STREAM_BUFFERS, ssBuffers[ss]);

	// Free the musicSource
	S_AL_SSSourceFree(ss);

	// Unload the stream
	S_AL_CloseSSFiles(ss);

	ssPlaying[ss] = qfalse;
}


/**
 * @brief S_AL_MusicSourceGet
 */
static void S_AL_MusicSourceGet(void)
{
	// Allocate a musicSource at high priority
	musicSourceHandle = S_AL_SrcAlloc(SRCPRI_STREAM, -2, 0);
	if (musicSourceHandle == -1)
	{
		return;
	}

	// Lock the musicSource so nobody else can use it, and get the raw musicSource
	S_AL_SrcLock(musicSourceHandle);
	musicSource = S_AL_SrcGet(musicSourceHandle);

	// make sure that after unmuting the S_AL_Gain in S_Update() does not turn
	// volume up prematurely for this source
	srcList[musicSourceHandle].scaleGain = 0.0f;

	// Set some musicSource parameters
	qalSource3f(musicSource, AL_POSITION, 0.0, 0.0, 0.0);
	qalSource3f(musicSource, AL_VELOCITY, 0.0, 0.0, 0.0);
	qalSource3f(musicSource, AL_DIRECTION, 0.0, 0.0, 0.0);
	qalSourcef(musicSource, AL_ROLLOFF_FACTOR, 0.0);
	qalSourcei(musicSource, AL_SOURCE_RELATIVE, AL_TRUE);
}

/**
 * @brief S_AL_MusicSourceFree
 */
static void S_AL_MusicSourceFree(void)
{
	// Release the output musicSource
	S_AL_SrcUnlock(musicSourceHandle);
	S_AL_SrcKill(musicSourceHandle);
	musicSource       = 0;
	musicSourceHandle = -1;
}

/**
 * @brief S_AL_CloseMusicFiles
 */
static void S_AL_CloseMusicFiles(void)
{
	if (intro_stream)
	{
		S_CodecCloseStream(intro_stream);
		intro_stream = NULL;
	}

	if (mus_stream)
	{
		S_CodecCloseStream(mus_stream);
		mus_stream = NULL;
	}
}

/**
 * @brief S_AL_StopBackgroundTrack
 */
static void S_AL_StopBackgroundTrack(void)
{
	if (!musicPlaying)
	{
		return;
	}

	// Stop playing
	qalSourceStop(musicSource);

	// Detach any buffers
	qalSourcei(musicSource, AL_BUFFER, 0);

	// Delete the buffers
	qalDeleteBuffers(NUM_MUSIC_BUFFERS, musicBuffers);

	// Free the musicSource
	S_AL_MusicSourceFree();

	// Unload the stream
	S_AL_CloseMusicFiles();

	musicPlaying = qfalse;
}

/**
 * @brief S_AL_MusicProcess
 * @param[in] b
 */
static void S_AL_MusicProcess(ALuint b)
{
	ALenum       error;
	int          l;
	ALuint       format;
	snd_stream_t *curstream;

	S_AL_ClearError(qfalse);

	if (intro_stream)
	{
		curstream = intro_stream;
	}
	else
	{
		curstream = mus_stream;
	}

	if (!curstream)
	{
		return;
	}

	l = S_CodecReadStream(curstream, MUSIC_BUFFER_SIZE, decode_buffer);

	// Run out data to read, start at the beginning again
	if (l == 0)
	{
		S_CodecCloseStream(curstream);

		// the intro stream just finished playing so we don't need to reopen
		// the music stream.
		if (intro_stream)
		{
			intro_stream = NULL;
		}
		else
		{
			mus_stream = S_CodecOpenStream(s_backgroundLoop);
		}

		curstream = mus_stream;

		if (!curstream)
		{
			S_AL_StopBackgroundTrack();
			return;
		}

		l = S_CodecReadStream(curstream, MUSIC_BUFFER_SIZE, decode_buffer);
	}

	format = S_AL_Format(curstream->info.width, curstream->info.channels);

	if (l == 0)
	{
		// We have no data to buffer, so buffer silence
		byte dummyData[2] = { 0 };

		qalBufferData(b, AL_FORMAT_MONO16, (void *)dummyData, 2, 22050);
	}
	else
	{
		qalBufferData(b, format, decode_buffer, l, curstream->info.rate);
	}

	if ((error = qalGetError()) != AL_NO_ERROR)
	{
		S_AL_StopBackgroundTrack();
		Com_Printf(S_COLOR_RED "ERROR: while buffering data for music stream - %s\n",
		           S_AL_ErrorMsg(error));
		return;
	}
}

/**
 * @brief S_AL_StopEntStreamingSound
 * @param[in] entnum
 */
void S_AL_StopEntStreamingSound(int entnum)
{
	int i;

	for (i = 1; i < MAX_STREAMING_SOUNDS; i++)
	{
		// is the stream active
		if (!ssData[i].stream)
		{
			continue;
		}
		// is it the right entity or is it all
		if (ssData[i].entnum != entnum && entnum != -1)
		{
			continue;
		}
		S_AL_StopStreamingSound(i);
	}
}

/**
 * @brief S_AL_FadeAllSounds
 * @param[in] targetVol
 * @param[in] time
 * @param[in] stopsounds
 */
void S_AL_FadeAllSounds(float targetVol, int time, qboolean stopsounds)
{
	int currentTime = Sys_Milliseconds();

	s_volStart  = s_volCurrent;
	s_volTarget = targetVol;

	s_volTime1 = currentTime;
	s_volTime2 = currentTime + time * 1000;

	s_stopSounds = stopsounds;

	// instant
	if (!time)
	{
		s_volTarget = s_volStart = s_volCurrent = targetVol;  // set it
		s_volTime1  = s_volTime2 = 0; // no fading
	}
}

/**
 * @brief S_AL_FadeStreamingSound
 * @param[in] targetVol
 * @param[in] time
 * @param[in] ss
 */
void S_AL_FadeStreamingSound(float targetVol, int time, int ss)
{
	int currentTime = Sys_Milliseconds();

	if (ss < 0 || ss >= MAX_STREAMING_SOUNDS)
	{
		return;
	}

	if (!ssData[ss].stream)
	{
		return;
	}

	ssData[ss].fadeStartVol = 1.0f;

	if (s_debugStreams->integer)
	{
		Com_Printf("S_AL_FadeStreamingSound: %d: Fade: %0.2f %d\n", ss, (double)targetVol, time);
	}

	// get current fraction if already fading
	if (ssData[ss].fadeStart)
	{
		ssData[ss].fadeStartVol = (ssData[ss].fadeEnd <= currentTime) ? ssData[ss].fadeTargetVol :
		                          ((float)(currentTime - ssData[ss].fadeStart) / (float)(ssData[ss].fadeEnd - ssData[ss].fadeStart));
	}

	ssData[ss].fadeStart     = currentTime;
	ssData[ss].fadeEnd       = currentTime + time * 1000;
	ssData[ss].fadeTargetVol = targetVol;
}


/**
 * @brief S_AL_SSProcess
 * @param[in] ss
 * @param[in] b
 */
static void S_AL_SSProcess(int ss, ALuint b)
{
	ALenum       error;
	int          l;
	ALuint       format;
	snd_stream_t *curstream = ssData[ss].stream;

	S_AL_ClearError(qfalse);

	if (!curstream)
	{
		S_AL_StopStreamingSound(ss);
		return;
	}

	l = S_CodecReadStream(curstream, STREAM_BUFFER_SIZE, decode_buffer);

	// FIXME: background music in game
	// Run out data to read, start at the beginning again
	// if (l == 0)
	if (0)
	{
		S_AL_CloseSSFiles(ss);
		curstream = NULL;

		// queuing music tracks for the music stream
		if (ss == ssMusic && ssData[ss].queueStreamType && *(ssData[ss].queueStream))
		{
			switch (ssData[ss].queueStreamType)
			{
			case QUEUED_PLAY_ONCE_SILENT:
				break;
			case QUEUED_PLAY_ONCE:
				Q_strncpyz(ssData[ss].loopStream, ssData[ss].name, MAX_QPATH);
				Q_strncpyz(ssData[ss].name, ssData[ss].queueStream, MAX_QPATH);
				break;
			case QUEUED_PLAY_LOOPED:
			default:
				Q_strncpyz(ssData[ss].name, ssData[ss].queueStream, MAX_QPATH);
				Q_strncpyz(ssData[ss].loopStream, ssData[ss].queueStream, MAX_QPATH);
				break;
			}
			// queue is done, clear it
			*ssData[ss].queueStream    = '\0';
			ssData[ss].queueStreamType = 0;
			curstream                  = ssData[ss].stream = S_CodecOpenStream(ssData[ss].name);
		}
		else
		{
			// the intro stream just finished playing so we need to open
			// the music/loop stream if there is one.
			// TODO: follow the old method where the loop stream is already opened
			if (*ssData[ss].loopStream)
			{
				Q_strncpyz(ssData[ss].name, ssData[ss].loopStream, MAX_QPATH);
				curstream = ssData[ss].stream = S_CodecOpenStream(ssData[ss].name);
			}
		}

		if (!curstream)
		{
			ssKill[ss] = qtrue;
			return;
		}

		l = S_CodecReadStream(curstream, STREAM_BUFFER_SIZE, decode_buffer);
	}

	if (l == 0)
	{
		// We have no data to buffer, so buffer silence
		byte dummyData[2] = { 0 };

		qalBufferData(b, AL_FORMAT_MONO16, (void *)dummyData, 2, 22050);
	}
	else
	{
		format = S_AL_Format(curstream->info.width, curstream->info.channels);
		qalBufferData(b, format, decode_buffer, l, curstream->info.rate);
	}

	if ((error = qalGetError()) != AL_NO_ERROR)
	{
		Com_Printf(S_COLOR_RED "ERROR S_AL_SSProcess: while buffering data for stream %d - %s\n",
		           ss, S_AL_ErrorMsg(error));
		return;
	}
}

/**
 * @brief S_AL_StartStreamingSoundEx
 * @param[in] intro
 * @param[in] loop
 * @param[in] entnum
 * @param channel - unused
 * @param[in] music
 * @param[in] param
 * @return
 */
static float S_AL_StartStreamingSoundEx(const char *intro, const char *loop, int entnum, int channel, qboolean music, int param)
{
	int i;
	int ss = -1;

	// Stop any existing stream that might be playing
	if (music)
	{
		S_AL_StopStreamingSound(ssMusic);
	}
	else
	{
		S_AL_StopEntStreamingSound(entnum);
	}

	// Nothing to play
	if ((!intro || !*intro) && (!loop || !*loop))
	{
		return 0.0f;
	}

	// Allocate a ssSource
	ss = S_AL_SSSourceGet();

	if (music)
	{
		ssMusic = ss;
	}

	if (ss == -1)
	{
		return 0.0f;
	}

	if (ssSourceHandle[ss] == -1)
	{
		return 0.0f;
	}

	if (music)
	{
		if (param < 0)
		{
			if (intro && *intro)
			{
				strncpy(ssData[ss].queueStream, intro, MAX_QPATH);
				ssData[ss].queueStreamType = param;
				// Cvar for save game
				if (param == -2)
				{
					Cvar_Set("s_currentMusic", intro);
				}
				if (s_debugStreams->integer)
				{
					if (param == -1)
					{
						Com_Printf("S_AL_StartStreamingSoundEx: queueing '%s' for play once\n", intro);
					}
					else if (param == -2)
					{
						Com_Printf("S_AL_StartStreamingSoundEx: queueing '%s' as new loop\n", intro);
					}
				}
			}
			else
			{
				*ssData[ss].loopStream     = '\0';
				*ssData[ss].queueStream    = '\0';
				ssData[ss].queueStreamType = 0;
				if (s_debugStreams->integer)
				{
					Com_Printf("S_AL_StartStreamingSoundEx: queue cleared\n");
				}
			}
		}
	}

	if (!music && (!loop || !*loop))
	{
		loop = intro;
	}

	// Copy the loop over if we don't have the special case for music tracks "onetimeonly"
	if (loop && *loop && (!music || Q_stricmp(loop, "onetimeonly")))
	{
		Q_strncpyz(ssData[ss].loopStream, loop, sizeof(ssData[ss].loopStream));
	}

	// Clear the current music cvar
	if (music)
	{
		Cvar_Set("s_currentMusic", "");
	}

	// Set streaming sound parameters
	// TODO: do something with the attenuation
	if (!music)
	{
		ssData[ss].attenuation = (qboolean)param;
	}

	// If it is the music track, thent he positive parameter is fadeUpTime
	if (music && param > 0)
	{
		ssData[ss].fadeStartVol  = 0.0f;
		ssData[ss].fadeStart     = Sys_Milliseconds();
		ssData[ss].fadeEnd       = ssData[ss].fadeStart + param * 1000;
		ssData[ss].fadeTargetVol = 1.0f;
	}
	else
	{
		ssData[ss].fadeStartVol  = 1.0f;
		ssData[ss].fadeStart     = 0;
		ssData[ss].fadeEnd       = 0;
		ssData[ss].fadeTargetVol = 0.0f;
	}

	// Open the intro and don't mind whether it succeeds.
	// The important part is the loop.
	if (intro && *intro)
	{
		ssData[ss].stream = S_CodecOpenStream(intro);
	}
	else
	{
		ssData[ss].stream = S_CodecOpenStream(loop);
	}

    if (!ssData[ss].stream)
    {
        Com_Printf(S_COLOR_YELLOW "WARNING S_AL_StartStreamingSoundEx: couldn't open stream file %s\n", intro);
        S_AL_SSSourceFree(ss);
        return 0.0f;
    }

	// Generate the musicBuffers
	qalGenBuffers(NUM_STREAM_BUFFERS, ssBuffers[ss]);

	// Queue the musicBuffers up
	for (i = 0; i < NUM_STREAM_BUFFERS; i++)
	{
		S_AL_SSProcess(ss, ssBuffers[ss][i]);
	}

	qalSourceQueueBuffers(ssSource[ss], NUM_STREAM_BUFFERS, ssBuffers[ss]);

	// Set the initial gain property
	S_AL_Gain(ssSource[ss], s_alGain->value * s_musicVolume->value * ssData[ss].fadeStartVol);

	// Start playing
	qalSourcePlay(ssSource[ss]);

	ssPlaying[ss] = qtrue;

	return ((float)ssData[ss].stream->info.samples / (float)ssData[ss].stream->info.rate) * 1000.0f;
}

/**
 * @brief S_AL_StartStreamingSound
 * @param[in] intro
 * @param[in] loop
 * @param[in] entnum
 * @param[in] channel
 * @param[in] attenuation
 * @return
 */
float S_AL_StartStreamingSound(const char *intro, const char *loop, int entnum, int channel, int attenuation)
{
	return S_AL_StartStreamingSoundEx(intro, loop, entnum, channel, qfalse, attenuation);
}

/**
 * @brief S_AL_StartBackgroundTrack
 * @param[in] intro
 * @param[in] loop
 * @param fadeupTime - unused
 */
void S_AL_StartBackgroundTrack(const char *intro, const char *loop, int fadeupTime)
{
	int      i;
	qboolean issame;

	// Stop any existing music that might be playing
	S_AL_StopBackgroundTrack();

	if ((!intro || !*intro) && (!loop || !*loop))
	{
		return;
	}

	// Allocate a musicSource
	S_AL_MusicSourceGet();
	if (musicSourceHandle == -1)
	{
		return;
	}

	if (!loop || !*loop)
	{
		loop   = intro;
		issame = qtrue;
	}
	else if (intro && *intro && !strcmp(intro, loop))
	{
		issame = qtrue;
	}
	else
	{
		issame = qfalse;
	}

	// Copy the loop over
	Q_strncpyz(s_backgroundLoop, loop, sizeof(s_backgroundLoop));

	if (!issame)
	{
		// Open the intro and don't mind whether it succeeds.
		// The important part is the loop.
		intro_stream = S_CodecOpenStream(intro);
	}
	else
	{
		intro_stream = NULL;
	}

	mus_stream = S_CodecOpenStream(s_backgroundLoop);
	if (!mus_stream)
	{
		S_AL_CloseMusicFiles();
		S_AL_MusicSourceFree();

		return;
	}

	// Generate the musicBuffers
	if (!S_AL_GenBuffers(NUM_MUSIC_BUFFERS, musicBuffers, "music"))
	{
		return;
	}

	// Queue the musicBuffers up
	for (i = 0; i < NUM_MUSIC_BUFFERS; i++)
	{
		S_AL_MusicProcess(musicBuffers[i]);
	}

	qalSourceQueueBuffers(musicSource, NUM_MUSIC_BUFFERS, musicBuffers);

	// Set the initial gain property
	S_AL_Gain(musicSource, s_alGain->value * s_musicVolume->value);

	// Start playing
	qalSourcePlay(musicSource);

	musicPlaying = qtrue;
}

/**
 * @brief S_AL_GetStreamingFade
 * @param[in] ss
 * @return
 */
float S_AL_GetStreamingFade(int ss)
{
	float oldfrac, newfrac;

	// no fading, use full volume
	if (!ssData[ss].fadeStart)
	{
		return 1.0f;
	}

	if (ssData[ss].fadeEnd <= Sys_Milliseconds())    // it's hit it's target
	{
		return ssData[ss].fadeTargetVol;
	}

	newfrac = (float)(Sys_Milliseconds() - ssData[ss].fadeStart) / (float)(ssData[ss].fadeEnd - ssData[ss].fadeStart);
	oldfrac = 1.0f - newfrac;

	return (oldfrac * ssData[ss].fadeStartVol) + (newfrac * ssData[ss].fadeTargetVol);
}

/**
 * @brief S_AL_MusicUpdate
 */
static void S_AL_MusicUpdate(void)
{
	int   numBuffers;
	ALint state;

	if (!musicPlaying)
	{
		return;
	}

	qalGetSourcei(musicSource, AL_BUFFERS_PROCESSED, &numBuffers);
	while (numBuffers--)
	{
		ALuint b;
		qalSourceUnqueueBuffers(musicSource, 1, &b);
		S_AL_MusicProcess(b);
		qalSourceQueueBuffers(musicSource, 1, &b);
	}

	// Hitches can cause OpenAL to be starved of buffers when streaming.
	// If this happens, it will stop playback. This restarts the source if
	// it is no longer playing, and if there are buffers available
	qalGetSourcei(musicSource, AL_SOURCE_STATE, &state);
	qalGetSourcei(musicSource, AL_BUFFERS_QUEUED, &numBuffers);
	if (state == AL_STOPPED && numBuffers)
	{
		Com_DPrintf(S_COLOR_YELLOW "Restarted OpenAL music\n");
		qalSourcePlay(musicSource);
	}

	// Set the gain property
	S_AL_Gain(musicSource, s_alGain->value * s_musicVolume->value);
}


/**
 * @brief S_AL_SSUpdate
 * @param ss
 */
static void S_AL_SSUpdate(int ss)
{
	int   numBuffers;
	ALint state;
	float fade;

	if (!ssPlaying[ss])
	{
		return;
	}

	qalGetSourcei(ssSource[ss], AL_BUFFERS_PROCESSED, &numBuffers);
	if (ssKill[ss])
	{
		if (numBuffers == NUM_STREAM_BUFFERS)
		{
			S_AL_StopStreamingSound(ss);
			return;
		}
	}
	else
	{
		while (numBuffers--)
		{
			ALuint b;
			qalSourceUnqueueBuffers(ssSource[ss], 1, &b);
			S_AL_SSProcess(ss, b);
			qalSourceQueueBuffers(ssSource[ss], 1, &b);
		}
	}

	// Hitches can cause OpenAL to be starved of buffers when streaming.
	// If this happens, it will stop playback. This restarts the source if
	// it is no longer playing, and if there are buffers available
	qalGetSourcei(ssSource[ss], AL_SOURCE_STATE, &state);
	qalGetSourcei(ssSource[ss], AL_BUFFERS_QUEUED, &numBuffers);
	if (state == AL_STOPPED && numBuffers)
	{
		Com_DPrintf(S_COLOR_YELLOW "Restarted OpenAL stream %d\n", ss);
        ssRestart[ss]++;
        if(ssRestart[ss] > 5)
        {
            S_AL_StopStreamingSound(ss);
            return;
        }
		qalSourcePlay(ssSource[ss]);
	}

	// Get the fading volume
	fade = S_AL_GetStreamingFade(ss);
	if (fade == 0.0f)
	{
		S_AL_StopStreamingSound(ss);
		return;
	}

	// Set the gain property
	S_AL_Gain(ssSource[ss], s_alGain->value *
	          (ss == ssMusic ? s_musicVolume->value : s_volume->value) * fade);
}

//===========================================================================

// Local state variables
static ALCdevice  *alDevice;
static ALCcontext *alContext;

#ifdef USE_VOIP
static ALCdevice *alCaptureDevice;
static cvar_t    *s_alCapture;
#endif

#if defined(_WIN64)
#define ALDRIVER_DEFAULT "OpenAL64.dll"
#elif defined(_WIN32)
#define ALDRIVER_DEFAULT "OpenAL32.dll"
#elif defined(__APPLE__)
#define ALDRIVER_DEFAULT "/System/Library/Frameworks/OpenAL.framework/OpenAL"
#elif defined(__OpenBSD__)
#define ALDRIVER_DEFAULT "libopenal.so"
#else
#define ALDRIVER_DEFAULT "libopenal.so.1"
#endif

/**
 * @brief S_AL_StopAllSounds
 */
static void S_AL_StopAllSounds(void)
{
	int i;

	S_AL_SrcShutup();
	S_AL_StopBackgroundTrack();
	for (i = 0; i < MAX_STREAMING_SOUNDS; i++)
	{
		S_AL_StopStreamingSound(i);
	}
	for (i = 0; i < MAX_RAW_STREAMS; i++)
	{
		S_AL_StreamDie(i);
	}
}

/**
 * @brief S_AL_ClearSounds
 * @param[in] clearStreaming
 * @param[in] clearMusic
 */
static void S_AL_ClearSounds(qboolean clearStreaming, qboolean clearMusic)
{
	int i;

	S_AL_SrcShutup();
	S_AL_StopBackgroundTrack();
	if (clearStreaming)
	{
		for (i = 0; i < MAX_STREAMING_SOUNDS; i++)
		{
			if (clearMusic || i != ssMusic)
			{
				S_AL_StopStreamingSound(i);
			}
		}
	}
	for (i = 0; i < MAX_RAW_STREAMS; i++)
	{
		S_AL_StreamDie(i);
	}
}

/**
 * @brief S_AL_Respatialize
 * @param entityNum - unused
 * @param[in] origin
 * @param[in] axis
 * @param inwater - unused
 */
static void S_AL_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater)
{
	float  velocity[3] = { 0.0f, 0.0f, 0.0f };
	float  orientation[6];
	vec3_t sorigin;

	VectorCopy(origin, sorigin);
	S_AL_SanitiseVector(sorigin);

	S_AL_SanitiseVector(axis[0]);
	S_AL_SanitiseVector(axis[1]);
	S_AL_SanitiseVector(axis[2]);

	orientation[0] = axis[0][0];
	orientation[1] = axis[0][1];
	orientation[2] = axis[0][2];
	orientation[3] = axis[2][0];
	orientation[4] = axis[2][1];
	orientation[5] = axis[2][2];

	VectorCopy(sorigin, lastListenerOrigin);

	// Set OpenAL listener paramaters
	qalListenerfv(AL_POSITION, (ALfloat *)sorigin);
	qalListenerfv(AL_VELOCITY, velocity);
	qalListenerfv(AL_ORIENTATION, orientation);
}

/**
 * @brief S_AL_Update
 */
static void S_AL_Update(void)
{
	int i;
	int currentTime = Sys_Milliseconds();

	// global volume fading
	if (currentTime < s_volTime2)
	{
		// still has fading to do
		if (currentTime > s_volTime1)
		{
			s_volFadeFrac = ((float)(currentTime - s_volTime1) / (float)(s_volTime2 - s_volTime1));
			s_volCurrent  = ((1.0f - s_volFadeFrac) * s_volStart + s_volFadeFrac * s_volTarget);
		}
		else
		{
			s_volCurrent = s_volStart;
		}
	}
	else
	{
		s_volCurrent = s_volTarget;
		if (s_stopSounds)
		{
			// stop playing any sounds if they are all faded out
			S_AL_StopAllSounds();
			s_stopSounds = qfalse;
		}
	}

	if (s_muted->modified)
	{
		// muted state changed. Let S_AL_Gain turn up all sources again.
		for (i = 0; i < srcCount; i++)
		{
			if (srcList[i].isActive)
			{
				S_AL_Gain(srcList[i].alSource, srcList[i].scaleGain);
			}
		}

		s_muted->modified = qfalse;
	}

	// Update SFX channels
	S_AL_SrcUpdate();

	// Update raw streams
	for (i = 0; i < MAX_RAW_STREAMS; i++)
	{
		S_AL_StreamUpdate(i);
	}
	// Update streaming sounds
	for (i = 0; i < MAX_STREAMING_SOUNDS; i++)
	{
		S_AL_SSUpdate(i);
	}
	S_AL_MusicUpdate();

	// Doppler
	if (s_doppler->modified)
	{
		s_alDopplerFactor->modified = qtrue;
		s_doppler->modified         = qfalse;
	}

	// Doppler parameters
	if (s_alDopplerFactor->modified)
	{
		if (s_doppler->integer)
		{
			qalDopplerFactor(s_alDopplerFactor->value);
		}
		else
		{
			qalDopplerFactor(0.0f);
		}
		s_alDopplerFactor->modified = qfalse;
	}
	if (s_alDopplerSpeed->modified)
	{
		qalDopplerVelocity(s_alDopplerSpeed->value);
		s_alDopplerSpeed->modified = qfalse;
	}

	// Clear the modified flags on the other cvars
	s_alGain->modified        = qfalse;
	s_volume->modified        = qfalse;
	s_musicVolume->modified   = qfalse;
	s_alMinDistance->modified = qfalse;
	s_alRolloff->modified     = qfalse;
}

/**
 * @brief S_AL_DisableSounds
 */
static void S_AL_DisableSounds(void)
{
	S_AL_StopAllSounds();
}

/**
 * @brief S_AL_BeginRegistration
 */
static void S_AL_BeginRegistration(void)
{
}

/*
=================
S_AL_ClearSoundBuffer
=================
*/
static void S_AL_ClearSoundBuffer(qboolean killStreaming)
{
	S_ClearSounds(killStreaming, qtrue);
}

/**
* @brief S_AL_SoundList
*/
static void S_AL_SoundList(void)
{
}

static void S_AL_Reload(void)
{
}

#ifdef USE_VOIP
/**
 * @brief S_AL_StartCapture
 */
static void S_AL_StartCapture(void)
{
	if (alCaptureDevice != NULL)
	{
		qalcCaptureStart(alCaptureDevice);
	}
}

/**
 * @brief S_AL_AvailableCaptureSamples
 * @return
 */
static int S_AL_AvailableCaptureSamples(void)
{
	int retval = 0;

	if (alCaptureDevice != NULL)
	{
		ALint samples = 0;
		qalcGetIntegerv(alCaptureDevice, ALC_CAPTURE_SAMPLES, sizeof(samples), &samples);
		retval = (int) samples;
	}
	return retval;
}

/**
 * @brief S_AL_Capture
 * @param[in] samples
 * @param[in] data
 */
static void S_AL_Capture(int samples, byte *data)
{
	if (alCaptureDevice != NULL)
	{
		qalcCaptureSamples(alCaptureDevice, data, samples);
	}
}

/**
 * @brief S_AL_StopCapture
 */
void S_AL_StopCapture(void)
{
	if (alCaptureDevice != NULL)
	{
		qalcCaptureStop(alCaptureDevice);
	}
}

/**
 * @brief S_AL_MasterGain
 * @param gain
 */
void S_AL_MasterGain(float gain)
{
	qalListenerf(AL_GAIN, gain);
}
#endif


/**
 * @brief S_AL_SoundInfo
 */
static void S_AL_SoundInfo(void)
{
	Com_Printf("OpenAL info:\n");
	Com_Printf("  Vendor:     %s\n", qalGetString(AL_VENDOR));
	Com_Printf("  Version:    %s\n", qalGetString(AL_VERSION));
	Com_Printf("  Renderer:   %s\n", qalGetString(AL_RENDERER));
	Com_Printf("  AL Extensions: %s\n", qalGetString(AL_EXTENSIONS));
	Com_Printf("  ALC Extensions: %s\n", qalcGetString(alDevice, ALC_EXTENSIONS));

	if (enumeration_all_ext)
	{
		Com_Printf("  Device:     %s\n", qalcGetString(alDevice, ALC_ALL_DEVICES_SPECIFIER));
	}
	else if (enumeration_ext)
	{
		Com_Printf("  Device:     %s\n", qalcGetString(alDevice, ALC_DEVICE_SPECIFIER));
	}

	if (enumeration_all_ext || enumeration_ext)
	{
		Com_Printf("  Available Devices:\n%s", s_alAvailableDevices->string);
	}

#ifdef USE_VOIP
	if (capture_ext)
	{
		Com_Printf("  Input Device:   %s\n", qalcGetString(alCaptureDevice, ALC_CAPTURE_DEVICE_SPECIFIER));
		Com_Printf("  Available Input Devices:\n%s", s_alAvailableInputDevices->string);
	}
#endif
}

/**
 * @brief S_AL_Shutdown
 */
static void S_AL_Shutdown(void)
{
	// Shut down everything
	int i;

	for (i = 0; i < MAX_RAW_STREAMS; i++)
	{
		S_AL_StreamDie(i);
	}
	S_AL_StopBackgroundTrack();
	S_AL_SrcShutdown();
	S_AL_BufferShutdown();

	qalcDestroyContext(alContext);
	qalcCloseDevice(alDevice);

#ifdef USE_VOIP
	if (alCaptureDevice != NULL)
	{
		qalcCaptureStop(alCaptureDevice);
		qalcCaptureCloseDevice(alCaptureDevice);
		alCaptureDevice = NULL;
		Com_Printf("OpenAL capture device closed.\n");
	}
#endif

	for (i = 0; i < MAX_RAW_STREAMS; i++)
	{
		streamSourceHandles[i] = -1;
		streamPlaying[i]       = qfalse;
		streamSources[i]       = 0;
	}

	QAL_Shutdown();
}

#endif

/**
 * @brief S_AL_Init
 * @param[out] si
 * @return
 */
qboolean S_AL_Init(soundInterface_t *si)
{
#ifdef FEATURE_OPENAL
	const char *device = NULL;
	//const char *inputdevice = NULL;
	int        i;

	if (!si)
	{
		Com_Printf("Invalid sound interface NULL.\n");
		return qfalse;
	}

	for (i = 0; i < MAX_RAW_STREAMS; i++)
	{
		streamSourceHandles[i] = -1;
		streamPlaying[i]       = qfalse;
		streamSources[i]       = 0;
		//streamNumBuffers[i]    = 0;
		//streamBufIndex[i]      = 0;
	}

	Com_Memset(ssRestart, 0, sizeof(ssRestart));

	// New console variables
	s_alPrecache      = Cvar_Get("s_alPrecache", "1", CVAR_ARCHIVE);
	s_alGain          = Cvar_Get("s_alGain", "1.0", CVAR_ARCHIVE);
	s_alSources       = Cvar_Get("s_alSources", "96", CVAR_ARCHIVE);
	s_alDopplerFactor = Cvar_Get("s_alDopplerFactor", "1.0", CVAR_ARCHIVE);
	s_alDopplerSpeed  = Cvar_Get("s_alDopplerSpeed", "2200", CVAR_ARCHIVE);
	s_alMinDistance   = Cvar_Get("s_alMinDistance", "420", CVAR_CHEAT);
	s_alMaxDistance   = Cvar_Get("s_alMaxDistance", "1250", CVAR_CHEAT);
	s_alRolloff       = Cvar_Get("s_alRolloff", "2", CVAR_CHEAT);
	s_alGraceDistance = Cvar_Get("s_alGraceDistance", "1250", CVAR_CHEAT);

	s_alDriver      = Cvar_Get("s_alDriver", ALDRIVER_DEFAULT, CVAR_ARCHIVE | CVAR_LATCH | CVAR_PROTECTED);
	//s_alInputDevice = Cvar_Get("s_alInputDevice", "", CVAR_ARCHIVE | CVAR_LATCH);
	s_alDevice      = Cvar_Get("s_alDevice", "", CVAR_ARCHIVE | CVAR_LATCH);
	s_debugStreams  = Cvar_Get("s_debugStreams", "0", CVAR_TEMP);

	if (COM_CompareExtension(s_alDriver->string, ".pk3"))
	{
		Com_Printf("Rejecting DLL named \"%s\"\n", s_alDriver->string);
		return qfalse;
	}

	// Load QAL
	if (!QAL_Init(s_alDriver->string))
	{
		Com_Printf("Failed to load library: \"%s\".\n", s_alDriver->string);
		//if( !Q_stricmp( s_alDriver->string, ALDRIVER_DEFAULT ) || !QAL_Init( ALDRIVER_DEFAULT ) ) {
		return qfalse;
	}

	device = s_alDevice->string;
	if (device && !*device)
	{
		device = NULL;
	}

#if 0
	inputdevice = s_alInputDevice->string;
	if (inputdevice && !*inputdevice)
	{
		inputdevice = NULL;
	}
#endif

	// Device enumeration support
	enumeration_all_ext = qalcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT");
	enumeration_ext     = qalcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");

	// Device enumeration support (extension is implemented reasonably only on Windows right now).
	if (enumeration_ext || enumeration_all_ext)
	{
		char       devicenames[1024] = "";
		const char *devicelist;
#ifdef _WIN32
		const char *defaultdevice;
#endif
		int curlen;

		// get all available devices + the default device name.
		if (enumeration_all_ext)
		{
			devicelist = qalcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
#ifdef _WIN32
			defaultdevice = qalcGetString(NULL, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
#endif
		}
		else
		{
			// We don't have ALC_ENUMERATE_ALL_EXT but normal enumeration.
			devicelist = qalcGetString(NULL, ALC_DEVICE_SPECIFIER);
#ifdef _WIN32
			defaultdevice = qalcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
#endif
			enumeration_ext = qtrue;
		}

#ifdef _WIN32
		// check whether the default device is generic hardware. If it is, change to
		// Generic Software as that one works more reliably with various sound systems.
		// If it's not, use OpenAL's default selection as we don't want to ignore
		// native hardware acceleration.
		if (!device && defaultdevice && !strcmp(defaultdevice, "Generic Hardware"))
		{
			device = "Generic Software";
		}
#endif

		if (devicelist)
		{
			// dump a list of available devices to a cvar for the user to see.
			while ((curlen = strlen(devicelist)))
			{
				Q_strcat(devicenames, sizeof(devicenames), devicelist);
				Q_strcat(devicenames, sizeof(devicenames), "\n");

				devicelist += curlen + 1;
			}
		}

		s_alAvailableDevices = Cvar_Get("s_alAvailableDevices", devicenames, CVAR_ROM | CVAR_NORESTART);
	}

	alDevice = qalcOpenDevice(device);
	if (!alDevice && device)
	{
		Com_Printf("Failed to open OpenAL device '%s', trying default.\n", device);
		alDevice = qalcOpenDevice(NULL);
	}

	if (!alDevice)
	{
		QAL_Shutdown();
		Com_Printf("Failed to open OpenAL device.\n");
		return qfalse;
	}

	// Create OpenAL context
	alContext = qalcCreateContext(alDevice, NULL);
	if (!alContext)
	{
		QAL_Shutdown();
		qalcCloseDevice(alDevice);
		Com_Printf("Failed to create OpenAL context.\n");
		return qfalse;
	}
	qalcMakeContextCurrent(alContext);

	// Initialize sources, buffers, music
	S_AL_BufferInit();
	S_AL_SrcInit();

	// Set up OpenAL parameters (doppler, etc)
	qalDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	qalDopplerFactor(s_alDopplerFactor->value);
	qalDopplerVelocity(s_alDopplerSpeed->value);

	qalGenEffects(QAL_EFX_MAX, effect);
	qalGenAuxiliaryEffectSlots(QAL_EFX_MAX, auxslot);

#ifdef AL_EFFECT_DEDICATED_LOW_FREQUENCY_EFFECT
	qalEffecti(effect[QAL_EFX_DEDICATED_LFE], AL_EFFECT_TYPE, AL_EFFECT_DEDICATED_LOW_FREQUENCY_EFFECT);
	qalEffectf(effect[QAL_EFX_DEDICATED_LFE], AL_DEDICATED_GAIN, 0.05f);
	qalAuxiliaryEffectSloti(auxslot[QAL_EFX_DEDICATED_LFE], AL_EFFECTSLOT_EFFECT, effect[QAL_EFX_DEDICATED_LFE]);
#endif

#ifdef USE_VOIP
	// !!! FIXME: some of these alcCaptureOpenDevice() values should be cvars.
	// !!! FIXME: add support for capture device enumeration.
	// !!! FIXME: add some better error reporting.
	s_alCapture = Cvar_Get("s_alCapture", "1", CVAR_ARCHIVE | CVAR_LATCH);
	if (!s_alCapture->integer)
	{
		Com_Printf("OpenAL capture support disabled by user ('+set s_alCapture 1' to enable)\n");
	}
#if USE_MUMBLE
	else if (cl_useMumble->integer)
	{
		Com_Printf("OpenAL capture support disabled for Mumble support\n");
	}
#endif
	else
	{
#ifdef __APPLE__
		// !!! FIXME: Apple has a 1.1-compliant OpenAL, which includes
		// !!! FIXME:  capture support, but they don't list it in the
		// !!! FIXME:  extension string. We need to check the version string,
		// !!! FIXME:  then the extension string, but that's too much trouble,
		// !!! FIXME:  so we'll just check the function pointer for now.
		qboolean test = qalcCaptureOpenDevice == NULL;
#else
		qboolean test = !qalcIsExtensionPresent(NULL, "ALC_EXT_capture");
#endif
		if (test)
		{
			Com_Printf("No ALC_EXT_capture support, can't record audio.\n");
		}
		else
		{
			// !!! FIXME: 8000Hz is what Speex narrowband mode needs, but we
			// !!! FIXME:  should probably open the capture device after
			// !!! FIXME:  initializing Speex so we can change to wideband
			// !!! FIXME:  if we like.
			Com_Printf("OpenAL default capture device is '%s'\n",
			           qalcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER));
			alCaptureDevice = qalcCaptureOpenDevice(NULL, 8000, AL_FORMAT_MONO16, 4096);

#if 0 // voip/input device
			char       inputdevicenames[1024] = "";
			const char *inputdevicelist;
			const char *defaultinputdevice;
			int        curlen;

			capture_ext = qtrue;

			// get all available input devices + the default input device name.
			inputdevicelist = qalcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
			defaultinputdevice = qalcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);

			// dump a list of available devices to a cvar for the user to see.
			if (inputdevicelist)
			{
				while ((curlen = strlen(inputdevicelist)))
				{
					Q_strcat(inputdevicenames, sizeof(inputdevicenames), inputdevicelist);
					Q_strcat(inputdevicenames, sizeof(inputdevicenames), "\n");
					inputdevicelist += curlen + 1;
				}
			}

			s_alAvailableInputDevices = Cvar_Get("s_alAvailableInputDevices", inputdevicenames, CVAR_ROM | CVAR_NORESTART);

			Com_Printf("OpenAL default capture device is '%s'\n", defaultinputdevice ? defaultinputdevice : "none");
			alCaptureDevice = qalcCaptureOpenDevice(inputdevice, 48000, AL_FORMAT_MONO16, VOIP_MAX_PACKET_SAMPLES*4);
			if( !alCaptureDevice && inputdevice )
			{
				Com_Printf("Failed to open OpenAL Input device '%s', trying default.\n", inputdevice);
				alCaptureDevice = qalcCaptureOpenDevice(NULL, 48000, AL_FORMAT_MONO16, VOIP_MAX_PACKET_SAMPLES*4);
			}
#endif
			Com_Printf("OpenAL capture device %s.\n",
			           (alCaptureDevice == NULL) ? "failed to open" : "opened");
		}
	}
#endif

	si->Shutdown              = S_AL_Shutdown;
	si->Reload                = S_AL_Reload;
	si->StartSound            = S_AL_StartSound;
	si->StartSoundEx          = S_AL_StartSoundEx;
	si->StartLocalSound       = S_AL_StartLocalSound;
	si->StartBackgroundTrack  = S_AL_StartBackgroundTrack;
	si->StopBackgroundTrack   = S_AL_StopBackgroundTrack;
	si->StartStreamingSound   = S_AL_StartStreamingSound;
	si->StopEntStreamingSound = S_AL_StopEntStreamingSound;
	si->FadeStreamingSound    = S_AL_FadeStreamingSound;
	si->RawSamples            = S_AL_RawSamples;
	si->ClearSounds           = S_AL_ClearSounds;
	si->StopAllSounds         = S_AL_StopAllSounds;
	si->FadeAllSounds         = S_AL_FadeAllSounds;
	si->ClearLoopingSounds    = S_AL_ClearLoopingSounds;
	si->AddLoopingSound       = S_AL_AddLoopingSound;
	si->AddRealLoopingSound   = S_AL_AddRealLoopingSound;
	si->Respatialize          = S_AL_Respatialize;
	si->UpdateEntityPosition  = S_AL_UpdateEntityPosition;
	si->Update                = S_AL_Update;
	si->DisableSounds         = S_AL_DisableSounds;
	si->BeginRegistration     = S_AL_BeginRegistration;
	si->RegisterSound         = S_AL_RegisterSound;
	si->ClearSoundBuffer      = S_AL_ClearSoundBuffer;
	si->SoundInfo             = S_AL_SoundInfo;
	si->SoundList             = S_AL_SoundList;
	si->GetVoiceAmplitude     = S_AL_GetVoiceAmplitude;
	si->GetSoundLength        = S_AL_GetSoundLength;
	si->GetCurrentSoundTime   = S_AL_GetCurrentSoundTime;

#ifdef USE_VOIP
	si->StartCapture            = S_AL_StartCapture;
	si->AvailableCaptureSamples = S_AL_AvailableCaptureSamples;
	si->Capture                 = S_AL_Capture;
	si->StopCapture             = S_AL_StopCapture;
	si->MasterGain              = S_AL_MasterGain;
#endif

	return qtrue;
#else
	return qfalse;
#endif
}
