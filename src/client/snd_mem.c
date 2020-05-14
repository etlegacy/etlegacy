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
 * @file snd_mem.c
 * @brief Sound caching
 */

#include "snd_local.h"
#include "snd_codec.h"

#define DEF_COMSOUNDMEGS "160"

/*
===============================================================================
memory management
===============================================================================
*/

static sndBuffer *buffer    = NULL;
static sndBuffer *freelist  = NULL;
static int       inUse      = 0;
static int       totalInUse = 0;

short *sfxScratchBuffer  = NULL;
sfx_t *sfxScratchPointer = NULL;
int   sfxScratchIndex    = 0;

void SND_Com_Dealloc(sndBuffer *v)
{
	*(sndBuffer **)v = freelist;
	freelist         = (sndBuffer *)v;
	inUse           += sizeof(sndBuffer);
}

/**
 * @brief SND_malloc
 * @return
 */
sndBuffer *SND_malloc(void)
{
	sndBuffer *v;
redo:
	if (freelist == NULL)
	{
		S_FreeOldestSound();
		goto redo;
	}

	inUse      -= sizeof(sndBuffer);
	totalInUse += sizeof(sndBuffer);

	v        = freelist;
	freelist = *(sndBuffer **)freelist;
	v->next  = NULL;
	return v;
}

/**
 * @brief SND_setup
 */
void SND_setup(void)
{
	sndBuffer *p, *q;
	cvar_t    *cv;
	int       scs;

	cv  = Cvar_Get("com_soundMegs", DEF_COMSOUNDMEGS, CVAR_LATCH | CVAR_ARCHIVE);
	scs = (cv->integer * 512); // q3 uses a value of 1536 - reverted to genuine ET value

	buffer = Com_Allocate(scs * sizeof(sndBuffer));
	if (!buffer)
	{
		Com_Error(ERR_FATAL, "Sound buffer failed to allocate %1.1f megs", (double)(scs / (1024.f * 1024.f)));
	}

	// allocate the stack based hunk allocator
	sfxScratchBuffer = Com_Allocate(SND_CHUNK_SIZE * sizeof(short) * 4);      //Hunk_Alloc(SND_CHUNK_SIZE * sizeof(short) * 4);
	if (!sfxScratchBuffer)
	{
		Com_Error(ERR_FATAL, "Unable to allocate sound scratch buffer");
	}
	sfxScratchPointer = NULL;

	inUse = scs * sizeof(sndBuffer);
	p     = buffer;
	q     = p + scs;
	while (--q > p)
	{
		*(sndBuffer **)q = q - 1;
	}

	*(sndBuffer **)q = NULL;
	freelist         = p + scs - 1;

	Com_Printf("Sound memory manager started\n");
}

/**
 * @brief SND_shutdown
 */
void SND_shutdown(void)
{
	Com_Dealloc(sfxScratchBuffer);
	Com_Dealloc(buffer);
}

/**
 * @brief Resample / decimate to the current source rate
 * @param[in] sfx
 * @param[in] channels
 * @param[in] inrate
 * @param[in] inwidth
 * @param[in] samples
 * @param[in] data
 * @param compressed - unused
 * @return
 */
static int ResampleSfx(sfx_t *sfx, int channels, int inrate, int inwidth, int samples, byte *data, qboolean compressed)
{
	float     stepscale = (float)inrate / dma.speed;  // this is usually 0.5, 1, or 2
	int       outcount  = (int)(samples / stepscale);
	int       srcsample;
	int       i, j;
	int       sample, samplefrac = 0, fracstep = (int)(stepscale * 256 * channels);
	int       part;
	sndBuffer *chunk = sfx->soundData;

	for (i = 0 ; i < outcount ; i++)
	{
		srcsample   = samplefrac >> 8;
		samplefrac += fracstep;

		for (j = 0 ; j < channels ; j++)
		{
			if (inwidth == 2)
			{
				sample = (((short *)data)[srcsample + j]);
			}
			else
			{
				sample = (unsigned int)((unsigned char)(data[srcsample + j]) - 128) << 8;
			}
			part = (i * channels + j) & (SND_CHUNK_SIZE - 1);
			if (part == 0)
			{
				sndBuffer *newchunk;
				newchunk = SND_malloc();
				if (chunk == NULL)
				{
					sfx->soundData = newchunk;
				}
				else
				{
					chunk->next = newchunk;
				}
				chunk = newchunk;
			}

			chunk->sndChunk[part] = sample;
		}
	}

	return outcount;
}

/**
 * @brief Resample / decimate to the current source rate
 * @param[in] sfx
 * @param[in] channels
 * @param[in] inrate
 * @param[in] inwidth
 * @param[in] samples
 * @param[in] data
 * @return
 */
static int ResampleSfxRaw(short *sfx, int channels, int inrate, int inwidth, int samples, byte *data)
{
	float stepscale = (float)inrate / dma.speed;  // this is usually 0.5, 1, or 2
	int   outcount  = (int)(samples / stepscale);
	int   srcsample, i, j, sample;
	int   samplefrac = 0, fracstep = (int)(stepscale * 256 * channels);

	for (i = 0 ; i < outcount ; i++)
	{
		srcsample   = samplefrac >> 8;
		samplefrac += fracstep;

		for (j = 0 ; j < channels ; j++)
		{
			if (inwidth == 2)
			{
				sample = LittleShort(((short *)data)[srcsample + j]);
			}
			else
			{
				sample = (int)((unsigned char)(data[srcsample + j]) - 128) << 8;
			}
			sfx[i] = sample;
		}
	}
	return outcount;
}

//=============================================================================

/**
 * @brief The filename may be different than sfx->name in the case
 * of a forced fallback of a player specific sound
 * @param[in,out] sfx
 * @return
 */
qboolean S_LoadSound(sfx_t *sfx)
{
	byte       *data;
	short      *samples;
	snd_info_t info;

	// player specific sounds are never directly loaded
	if (sfx->soundName[0] == '*')
	{
		return qfalse;
	}

	if (FS_FOpenFileRead(sfx->soundName, NULL, qfalse) <= 0)
	{
		if (!Q_stricmp(Cvar_VariableString("fs_game"), DEFAULT_MODGAME))
		{
			// changed from debug to common print - let admins know and fix such missing files ...
			// if default mod is printing this - there is a missing sound to fix
			Com_Printf(S_COLOR_RED "ERROR: sound file '%s' does not exist or can't be read\n", sfx->soundName);
		}
		else
		{
			Com_DPrintf(S_COLOR_RED "ERROR: sound file \"%s\" does not exist or can't be read\n", sfx->soundName);
		}
		return qfalse;
	}

	// load it in
	data = S_CodecLoad(sfx->soundName, &info);
	if (!data)
	{
		return qfalse;
	}

	if (info.width == 1)
	{
		Com_DPrintf(S_COLOR_YELLOW "WARNING: %s is a 8 bit audio file\n", sfx->soundName);
	}

	if ((info.rate != 11025) && (info.rate != 22050) && (info.rate != 44100))
	{
		Com_DPrintf(S_COLOR_YELLOW "WARNING: %s is not a 11kHz, 22kHz nor 44kHz audio file. It has sample rate %i\n", sfx->soundName, info.rate);
	}

	samples = Hunk_AllocateTempMemory(info.channels * info.samples * sizeof(short) * 2);

	sfx->lastTimeUsed = Sys_Milliseconds() + 1;

	// each of these compression schemes works just fine
	// but the 16bit quality is much nicer and with a local
	// install assured we can rely upon the sound memory
	// manager to do the right thing for us and page
	// sound in as needed

	if (info.channels == 1 && sfx->soundCompressed == qtrue)
	{
		sfx->soundCompressionMethod = 1;
		sfx->soundData              = NULL;
		sfx->soundLength            = ResampleSfxRaw(samples, info.channels, info.rate, info.width, info.samples, data + info.dataofs);
		S_AdpcmEncodeSound(sfx, samples);
#if 0
	}
	else if (info.channels == 1 && info.samples > (SND_CHUNK_SIZE * 16) && info.width > 1)
	{
		sfx->soundCompressionMethod = 3;
		sfx->soundData              = NULL;
		sfx->soundLength            = ResampleSfxRaw(samples, info.channels, info.rate, info.width, info.samples, (data + info.dataofs));
		encodeMuLaw(sfx, samples);
	}
	else if (info.channels == 1 && info.samples > (SND_CHUNK_SIZE * 6400) && info.width > 1)
	{
		sfx->soundCompressionMethod = 2;
		sfx->soundData              = NULL;
		sfx->soundLength            = ResampleSfxRaw(samples, info.channels, info.rate, info.width, info.samples, (data + info.dataofs));
		encodeWavelet(sfx, samples);
#endif
	}
	else
	{
		sfx->soundCompressionMethod = 0;
		sfx->soundLength            = info.samples;
		sfx->soundData              = NULL;
		sfx->soundLength            = ResampleSfx(sfx, info.channels, info.rate, info.width, info.samples, data + info.dataofs, qfalse);
	}
	sfx->soundChannels = info.channels;

	Hunk_FreeTempMemory(samples);
	Hunk_FreeTempMemory(data);

	return qtrue;
}

/**
 * @brief S_DisplayFreeMemory
 */
void S_DisplayFreeMemory(void)
{
	Com_Printf("%d bytes (%6.2f MB) free sound buffer memory, %d bytes (%6.2f MB) total used.\n", inUse, (double)(inUse / Square(1024.f)), totalInUse, (double)(totalInUse / Square(1024.f)));
}
