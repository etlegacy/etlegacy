/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
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
cvar_t *s_alAvailableDevices;
cvar_t *s_debugStreams;

// sound fading
static float    s_volStart, s_volTarget;
static int      s_volTime1, s_volTime2;
static float    s_volFadeFrac;
static qboolean s_stopSounds;

/*
=================
S_AL_Format
=================
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

/*
=================
S_AL_ErrorMsg
=================
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

/*
=================
S_AL_ClearError
=================
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
static alSfx_t knownSfx[MAX_SFX];
static int     numSfx = 0;

static sfxHandle_t default_sfx;

/*
======================
S_GetVoiceAmplitude
======================
*/
int S_AL_GetVoiceAmplitude(int entnum)
{
	return 0;
}

/*
======================
S_AL_GetSoundLength
Returns how long the sound lasts in milliseconds
======================
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

/*
======================
S_AL_GetCurrentSoundTime
For looped sound synchronisation
======================
*/
int S_AL_GetCurrentSoundTime(void)
{
	return Sys_Milliseconds();
}

/*
=================
S_AL_BufferFindFree

Find a free handle
=================
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

/*
=================
S_AL_BufferFind

Find a sound effect if loaded, set up a handle otherwise
=================
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

		// Clear and copy the filename over
		ptr = &knownSfx[sfx];
		memset(ptr, 0, sizeof(*ptr));
		ptr->masterLoopSrc = -1;
		strcpy(ptr->filename, filename);
	}

	// Return the handle
	return sfx;
}

/*
=================
S_AL_BufferUseDefault
=================
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

/*
=================
S_AL_BufferUnload
=================
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

/*
=================
S_AL_BufferEvict
=================
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

/*
=================
S_AL_BufferLoad
=================
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
		Z_Free(data);
		return;
	}

	format = S_AL_Format(info.width, info.channels);

	// Create a buffer
	S_AL_ClearError(qfalse);
	qalGenBuffers(1, &curSfx->buffer);
	if ((error = qalGetError()) != AL_NO_ERROR)
	{
		S_AL_BufferUseDefault(sfx);
		Z_Free(data);
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
			S_AL_BufferUseDefault(sfx);
			Z_Free(data);
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
		S_AL_BufferUseDefault(sfx);
		Z_Free(data);
		Com_Printf(S_COLOR_RED "ERROR S_AL_BufferLoad: Can't fill sound buffer for %s - %s\n",
		           curSfx->filename, S_AL_ErrorMsg(error));
		return;
	}

	curSfx->info = info;

	// Free the memory
	Z_Free(data);

	// Woo!
	curSfx->inMemory = qtrue;
}

/*
=================
S_AL_BufferUse
=================
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

/*
=================
S_AL_BufferInit
=================
*/
static qboolean S_AL_BufferInit(void)
{
	if (alBuffersInitialised)
	{
		return qtrue;
	}

	// Clear the hash table, and SFX table
	memset(knownSfx, 0, sizeof(knownSfx));
	numSfx = 0;

	// Load the default sound, and lock it
	default_sfx = S_AL_BufferFind("sound/player/default/blank.wav");
	S_AL_BufferUse(default_sfx);
	knownSfx[default_sfx].isLocked = qtrue;

	// All done
	alBuffersInitialised = qtrue;
	return qtrue;
}

/*
=================
S_AL_BufferShutdown
=================
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
	memset(knownSfx, 0, sizeof(knownSfx));

	// All undone
	alBuffersInitialised = qfalse;
}

/*
=================
S_AL_RegisterSound
=================
*/
static sfxHandle_t S_AL_RegisterSound(const char *sample, qboolean compressed)
{
	sfxHandle_t sfx = S_AL_BufferFind(sample);

	if ((!knownSfx[sfx].inMemory) && (!knownSfx[sfx].isDefault))
	{
		S_AL_BufferLoad(sfx, s_alPrecache->integer);
	}
	knownSfx[sfx].lastUsedTime = Com_Milliseconds();

	if (knownSfx[sfx].isDefault)
	{
		return 0;
	}

	return sfx;
}

/*
=================
S_AL_BufferGet

Return's an sfx's buffer
=================
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

/*
=================
S_AL_SanitiseVector
=================
*/
#define S_AL_SanitiseVector(v) _S_AL_SanitiseVector(v, __LINE__)
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

/*
=================
S_AL_Gain
Set gain to 0 if muted, otherwise set it to given value.
=================
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

/*
=================
S_AL_ScaleGain
Adapt the gain if necessary to get a quicker fadeout when the source is too far away.
=================
*/
static void S_AL_ScaleGain(src_t *chksrc, vec3_t origin)
{
	float distance;

	if (!chksrc->local)
	{
		distance = Distance(origin, lastListenerOrigin);
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

/*
=================
S_AL_HearingThroughEntity
=================
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
		distanceSq = DistanceSquared(
		    entityPositions[entityNum],
		    lastListenerOrigin);

		if (distanceSq > AL_THIRD_PERSON_THRESHOLD_SQ)
		{
			return qfalse; //we're the player, but third person
		}
		else
		{
			return qtrue;  //we're the player
		}
	}
	else
	{
		return qfalse; //not the player
	}
}

/*
=================
S_AL_SrcInit
=================
*/
static qboolean S_AL_SrcInit(void)
{
	int    i;
	int    limit;
	ALenum error;

	// Clear the sources data structure
	memset(srcList, 0, sizeof(srcList));
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

/*
=================
S_AL_SrcShutdown
=================
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

	memset(srcList, 0, sizeof(srcList));

	alSourcesInitialised = qfalse;
}

/*
=================
S_AL_SrcSetup
=================
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
		qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_TRUE);
		qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, 0.0f);
	}
	else
	{
		qalSourcei(curSource->alSource, AL_SOURCE_RELATIVE, AL_FALSE);
		qalSourcef(curSource->alSource, AL_ROLLOFF_FACTOR, s_alRolloff->value);
	}
}

/*
=================
S_AL_SaveLoopPos
Remove given source as loop master if it is the master and hand off master status to another source in this case.
=================
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

/*
=================
S_AL_NewLoopMaster
Remove given source as loop master if it is the master and hand off master status to another source in this case.
=================
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
		else if (rmSource == &srcList[curSfx->masterLoopSrc])
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

/*
=================
S_AL_SrcKill
=================
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

/*
=================
S_AL_SrcAlloc
=================
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
=================
S_AL_SrcFind

Finds an active source with matching entity and channel numbers
Returns -1 if there isn't one
=================
*/
#if 0
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
#endif

/*
=================
S_AL_SrcLock

Locked sources will not be automatically reallocated or managed
=================
*/
static void S_AL_SrcLock(srcHandle_t src)
{
	srcList[src].isLocked = qtrue;
}

/*
=================
S_AL_SrcUnlock

Once unlocked, the source may be reallocated again
=================
*/
static void S_AL_SrcUnlock(srcHandle_t src)
{
	srcList[src].isLocked = qfalse;
}

/*
=================
S_AL_UpdateEntityPosition
=================
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

/*
=================
S_AL_CheckInput
Check whether input values from mods are out of range.
Necessary for i.g. Western Quake3 mod which is buggy.
=================
*/
static qboolean S_AL_CheckInput(int entityNum, sfxHandle_t sfx)
{
	if (entityNum < 0 || entityNum >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "ERROR S_AL_CheckInput: S_AL_CheckInput: bad entitynum %i", entityNum);
	}

	if (sfx < 0 || sfx >= numSfx)
	{
		Com_Printf(S_COLOR_RED "ERROR S_AL_CheckInput: S_AL_CheckInput: handle %i out of range\n", sfx);
		return qtrue;
	}

	return qfalse;
}

/*
=================
S_AL_StartLocalSound

Play a local (non-spatialized) sound effect
=================
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

/*
=================
S_AL_StartSoundEx

Play a one-shot sound effect
=================
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
	    (DistanceSquared(sorigin, lastListenerOrigin) >=
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

/*
=================
S_AL_StartSound

Play a one-shot sound effect
=================
*/
static void S_AL_StartSound(vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx, int volume)
{
	S_AL_StartSoundEx(origin, entnum, entchannel, sfx, 0, volume);
}

/*
=================
S_AL_ClearLoopingSounds
=================
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

/*
=================
S_AL_SrcLoop
=================
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

/*
=================
S_AL_AddLoopingSound
=================
*/
static void S_AL_AddLoopingSound(const vec3_t origin, const vec3_t velocity, int range, sfxHandle_t sfx, int volume, int soundTime)
{
	S_AL_SrcLoop(SRCPRI_ENTITY, sfx, origin, velocity, range, volume, soundTime);
}

/*
=================
S_AL_AddRealLoopingSound
=================
*/
static void S_AL_AddRealLoopingSound(const vec3_t origin, const vec3_t velocity, int range, sfxHandle_t sfx, int volume, int soundTime)
{
	S_AL_SrcLoop(SRCPRI_AMBIENT, sfx, origin, velocity, range, volume, soundTime);
}

/*
=================
S_AL_SrcUpdate

Update state (move things around, manage sources, and so on)
=================
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
				if (!curSource->scaleGain)
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

					qalSourcei(curSource->alSource, AL_LOOPING, AL_TRUE);
					curSource->isPlaying = qtrue;
					qalSourcePlay(curSource->alSource);
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

/*
=================
S_AL_SrcShutup
=================
*/
static void S_AL_SrcShutup(void)
{
	int i;

	for (i = 0; i < srcCount; i++)
	{
		S_AL_SrcKill(i);
	}
}

/*
=================
S_AL_SrcGet
=================
*/
static ALuint S_AL_SrcGet(srcHandle_t src)
{
	return srcList[src].alSource;
}

//===========================================================================

static srcHandle_t streamSourceHandles[MAX_RAW_STREAMS];
static qboolean    streamPlaying[MAX_RAW_STREAMS];
static ALuint      streamSources[MAX_RAW_STREAMS];

/*
=================
S_AL_AllocateStreamChannel
=================
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

/*
=================
S_AL_FreeStreamChannel
=================
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

/*
=================
S_AL_RawSamples
=================
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

/*
=================
S_AL_StreamUpdate
=================
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

/*
=================
S_AL_StreamDie
=================
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
static streamingSound_t ssData[MAX_STREAMING_SOUNDS];
static srcHandle_t      ssSourceHandle[MAX_STREAMING_SOUNDS];
static ALuint           ssSource[MAX_STREAMING_SOUNDS];
static ALuint           ssBuffers[MAX_STREAMING_SOUNDS][NUM_STREAM_BUFFERS];

static byte decode_buffer[STREAM_BUFFER_SIZE];

/*
=================
S_AL_SSSourceGet
=================
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

/*
=================
S_AL_SSSourceFree
=================
*/
static void S_AL_SSSourceFree(int ss)
{
	// Release the output streaming sound source
	S_AL_SrcUnlock(ssSourceHandle[ss]);
	ssSource[ss]       = 0;
	ssSourceHandle[ss] = -1;
}

/*
=================
S_AL_CloseSSFiles
=================
*/
static void S_AL_CloseSSFiles(int ss)
{
	if (ssData[ss].stream)
	{
		S_CodecCloseStream(ssData[ss].stream);
		ssData[ss].stream = NULL;
	}
}

/*
=================
S_AL_StopStreamingSound
=================
*/
static void S_AL_StopStreamingSound(int ss)
{
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

/*
=================
S_AL_StopBackgroundTrack
=================
*/
static void S_AL_StopBackgroundTrack(void)
{
	S_AL_StopStreamingSound(ssMusic);
}

/*
==============
S_AL_StopEntStreamingSound
==============
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

/*
==============
S_AL_FadeAllSounds
==============
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

/*
==============
S_AL_FadeStreamingSound
==============
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
		Com_Printf("S_AL_FadeStreamingSound: %d: Fade: %0.2f %d\n", ss, targetVol, time);
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

/*
=================
S_AL_SSProcess
=================
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

/*
=================
S_AL_StartStreamingSoundEx
=================
*/
static float S_AL_StartStreamingSoundEx(const char *intro, const char *loop, int entnum, int channel, qboolean music, int param)
{
	int      i;
	int      ss = -1;
	qboolean issame;

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

	if (!music)
	{
		issame = qfalse;
	}
	else if ((!loop || !*loop))
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

	// Copy the loop over if we don't have the special case for music tracks "onetimeonly"
	if (loop && *loop && (!music || Q_stricmp(loop, "onetimeonly")))
	{
		strncpy(ssData[ss].loopStream, loop, sizeof(ssData[ss].loopStream));
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
		ssData[ss].attenuation = param;
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

/*
==============
S_AL_StartStreamingSound
==============
*/
float S_AL_StartStreamingSound(const char *intro, const char *loop, int entnum, int channel, int attenuation)
{
	return S_AL_StartStreamingSoundEx(intro, loop, entnum, channel, qfalse, attenuation);
}

/*
==============
S_AL_StartBackgroundTrack
==============
*/
void S_AL_StartBackgroundTrack(const char *intro, const char *loop, int fadeupTime)
{
	S_AL_StartStreamingSoundEx(intro, loop, -1, -1, qtrue, fadeupTime);
}

/*
==============
S_AL_GetStreamingFade
==============
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


/*
=================
S_AL_MusicUpdate
=================
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

#ifdef _WIN32
#define ALDRIVER_DEFAULT "OpenAL32.dll"
#elif defined(__APPLE__)
#define ALDRIVER_DEFAULT "/System/Library/Frameworks/OpenAL.framework/OpenAL"
#else
#define ALDRIVER_DEFAULT "libopenal.so.1"
#endif

/*
=================
S_AL_StopAllSounds
=================
*/
static void S_AL_StopAllSounds(void)
{
	int i;

	S_AL_SrcShutup();
	for (i = 0; i < MAX_STREAMING_SOUNDS; i++)
	{
		S_AL_StopStreamingSound(i);
	}
	for (i = 0; i < MAX_RAW_STREAMS; i++)
	{
		S_AL_StreamDie(i);
	}
}

/*
=================
S_AL_ClearSounds
=================
*/
static void S_AL_ClearSounds(qboolean clearStreaming, qboolean clearMusic)
{
	int i;

	S_AL_SrcShutup();
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

/*
=================
S_AL_Respatialize
=================
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

/*
=================
S_AL_Update
=================
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

/*
=================
S_AL_DisableSounds
=================
*/
static void S_AL_DisableSounds(void)
{
	S_AL_StopAllSounds();
}

/*
=================
S_AL_BeginRegistration
=================
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

/*
=================
S_AL_SoundList
=================
*/
static void S_AL_SoundList(void)
{
}

static void S_AL_Reload(void)
{
}

#ifdef USE_VOIP
static void S_AL_StartCapture(void)
{
	if (alCaptureDevice != NULL)
	{
		qalcCaptureStart(alCaptureDevice);
	}
}

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

static void S_AL_Capture(int samples, byte *data)
{
	if (alCaptureDevice != NULL)
	{
		qalcCaptureSamples(alCaptureDevice, data, samples);
	}
}

void S_AL_StopCapture(void)
{
	if (alCaptureDevice != NULL)
	{
		qalcCaptureStop(alCaptureDevice);
	}
}

void S_AL_MasterGain(float gain)
{
	qalListenerf(AL_GAIN, gain);
}
#endif

/*
=================
S_AL_SoundInfo
=================
*/
static void S_AL_SoundInfo(void)
{
	Com_Printf("OpenAL info:\n");
	Com_Printf("  Vendor:     %s\n", qalGetString(AL_VENDOR));
	Com_Printf("  Version:    %s\n", qalGetString(AL_VERSION));
	Com_Printf("  Renderer:   %s\n", qalGetString(AL_RENDERER));
	Com_Printf("  AL Extensions: %s\n", qalGetString(AL_EXTENSIONS));
	Com_Printf("  ALC Extensions: %s\n", qalcGetString(alDevice, ALC_EXTENSIONS));
	if (qalcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
	{
		Com_Printf("  Device:     %s\n", qalcGetString(alDevice, ALC_DEVICE_SPECIFIER));
		Com_Printf("Available Devices:\n%s", s_alAvailableDevices->string);
	}
}

/*
=================
S_AL_Shutdown
=================
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

/*
=================
S_AL_Init
=================
*/
qboolean S_AL_Init(soundInterface_t *si)
{
#ifdef FEATURE_OPENAL
	const char *device = NULL;
	int        i;

	if (!si)
	{
		return qfalse;
	}

	for (i = 0; i < MAX_RAW_STREAMS; i++)
	{
		streamSourceHandles[i] = -1;
		streamPlaying[i]       = qfalse;
		streamSources[i]       = 0;
	}

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

	s_alDriver = Cvar_Get("s_alDriver", ALDRIVER_DEFAULT, CVAR_ARCHIVE | CVAR_LATCH);

	s_alDevice     = Cvar_Get("s_alDevice", "", CVAR_ARCHIVE | CVAR_LATCH);
	s_debugStreams = Cvar_Get("s_debugStreams", "0", CVAR_TEMP);

	// Load QAL
	if (!QAL_Init(s_alDriver->string))
	{
		Com_Printf("Failed to load library: \"%s\".\n", s_alDriver->string);
		return qfalse;
	}

	device = s_alDevice->string;
	if (device && !*device)
	{
		device = NULL;
	}

	// Device enumeration support (extension is implemented reasonably only on Windows right now).
	if (qalcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
	{
		char       devicenames[1024] = "";
		const char *devicelist;
		const char *defaultdevice;
		int        curlen;

		// get all available devices + the default device name.
		devicelist    = qalcGetString(NULL, ALC_DEVICE_SPECIFIER);
		defaultdevice = qalcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

#ifdef _WIN32
		// check whether the default device is generic hardware. If it is, change to
		// Generic Software as that one works more reliably with various sound systems.
		// If it's not, use OpenAL's default selection as we don't want to ignore
		// native hardware acceleration.
		if (!device && !strcmp(defaultdevice, "Generic Hardware"))
		{
			device = "Generic Software";
		}
#endif

		// dump a list of available devices to a cvar for the user to see.
		while ((curlen = strlen(devicelist)))
		{
			Q_strcat(devicenames, sizeof(devicenames), devicelist);
			Q_strcat(devicenames, sizeof(devicenames), "\n");

			devicelist += curlen + 1;
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
		if (qalcCaptureOpenDevice == NULL)
#else
		if (!qalcIsExtensionPresent(NULL, "ALC_EXT_capture"))
#endif
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
