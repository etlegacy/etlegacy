/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file sdl_snd.c
 */

#include "sdl_defs.h"

#include <stdlib.h>
#include <stdio.h>

#include "../qcommon/q_shared.h"
#include "../client/snd_local.h"

qboolean snd_inited = qfalse;


cvar_t *s_bits;     // before rc2 -> *s_sdlBits;
cvar_t *s_khz;      // before rc2 -> *s_sdlSpeed
cvar_t *s_device;
cvar_t *s_sdlChannels; // external s_channels (GPL: cvar_t s_numchannels )
cvar_t *s_sdlDevSamps;
cvar_t *s_sdlMixSamps;
cvar_t *s_sdlLevelSamps;

/* The audio callback. All the magic happens here. */
static int             dmapos  = 0;
static int             dmasize = 0;
static SDL_AudioStream *stream = NULL;

/**
 * @brief SNDDMA_AudioCallback
 * @param userdata - unused
 * @param[in] stream
 * @param[in] additional_amount
 * @param[in] total_amount
 */
static void SNDDMA_AudioCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	(void) userdata;
	(void) total_amount;

	/* shouldn't happen, but just in case... */
	if (!snd_inited || additional_amount <= 0)
	{
		return;
	}

	int pos = (dmapos * (dma.samplebits / 8));
	if (pos >= dmasize)
	{
		dmapos = pos = 0;
	}

	int tobufend = dmasize - pos;  /* bytes to buffer's end. */
	int len1     = additional_amount;
	int len2     = 0;

	if (len1 > tobufend)
	{
		len1 = tobufend;
		len2 = additional_amount - len1;
	}

	SDL_PutAudioStreamData(stream, dma.buffer + pos, len1);

	if (len2 <= 0)
	{
		dmapos += (len1 / (dma.samplebits / 8));
	}
	else  /* wraparound? */
	{
		SDL_PutAudioStreamData(stream, dma.buffer, len2);
		dmapos = (len2 / (dma.samplebits / 8));
	}

	if (dmapos >= dmasize)
	{
		dmapos = 0;
	}
}

static struct
{
	Uint16 enumFormat;
	char *stringFormat;
} formatToStringTable[] =
{
	{ SDL_AUDIO_U8,    "SDL_AUDIO_U8"    },
	{ SDL_AUDIO_S8,    "SDL_AUDIO_S8"    },
	{ SDL_AUDIO_S16LE, "SDL_AUDIO_S16LE" },
	{ SDL_AUDIO_S16BE, "SDL_AUDIO_S16BE" },
	{ SDL_AUDIO_S32LE, "SDL_AUDIO_S32LE" },
	{ SDL_AUDIO_S32BE, "SDL_AUDIO_S32BE" },
	{ SDL_AUDIO_F32LE, "SDL_AUDIO_F32LE" },
	{ SDL_AUDIO_F32BE, "SDL_AUDIO_F32BE" },
};

static int formatToStringTableSize = ARRAY_LEN(formatToStringTable);

/**
 * @brief SNDDMA_PrintAudiospec
 * @param[in] str
 * @param[in] spec
 */
static void SNDDMA_PrintAudiospec(const char *str, const SDL_AudioSpec *spec, const int samples)
{
	int  i;
	char *fmt = NULL;

	Com_Printf("%s:\n", str);

	for (i = 0; i < formatToStringTableSize; i++)
	{
		if (spec->format == formatToStringTable[i].enumFormat)
		{
			fmt = formatToStringTable[i].stringFormat;
		}
	}

	if (fmt)
	{
		Com_Printf("  Format:   %s\n", fmt);
	}
	else
	{
		Com_Printf("  Format:   " S_COLOR_RED "UNKNOWN\n");
	}

	Com_Printf("  Freq:     %d\n", spec->freq);
	Com_Printf("  Samples:  %d\n", samples);
	Com_Printf("  Channels: %d\n", (int) spec->channels);
}

/**
 * @brief SNDDMA_KHzToHz
 */
static int SNDDMA_KHzToHz(int khz)
{
	#if 1 //FIXME: SDL3 only supports 44/48kHz it seems, fix later
	switch (khz)
	{
	case 11:
	case 22:
	case 44:
		return 44100;
	case 48:
		return 48000;
	default:
		return 44100;
	}
	#else
	switch (khz)
	{
	case 11:
		return 11025;
	case 22:
		return 22050;
	case 44:
		return 44100;
	case 48:
		return 48000;
	default:
		return 22050; // default vanilla
	}
	#endif
}

/**
 * @brief SND_DeviceList
 */
static void SND_DeviceList(void)
{
	int               i, count = 0;
	SDL_AudioDeviceID *ids;

	ids = SDL_GetAudioPlaybackDevices(&count);

	Com_Printf("Printing audio device list. Number of devices: %i\n\n", count);

	for (i = 0; i < count; ++i)
	{
		Com_Printf("  Audio device %d: %s\n", i, SDL_GetAudioDeviceName(ids[i]));
	}
	SDL_free(ids);
}

int SND_SamplesForFreq(int freq, int level)
{
	int samples;
	switch (freq)
	{
	case 11025:
		samples = 256;
		break;
	case 22050:
		samples = 512;
		break;
	case 44100:
		samples = 1024;
		break;
	default:     // 48KHz
		samples = 2048;
		break;
	}

	if (level == 1)
	{
		samples /= 2;
	}
	else if (level == 2)
	{
		samples /= 4;
	}

	return samples;
}

/**
 * @brief SNDDMA_Init
 * @return
 */
qboolean SNDDMA_Init(void)
{
	SDL_AudioSpec     desired;
	SDL_AudioSpec     obtained;
	SDL_AudioDeviceID *ids;
	SDL_AudioDeviceID device;
	const char        *driver_name;
	const char        *device_name;
	int               tmp;
	int               count;
	int               desired_samples, obtained_samples = 0;

	if (snd_inited)
	{
		return qtrue;
	}

	Cmd_AddCommand("sndlist", SND_DeviceList, "Prints a list of available sound devices.");

	s_bits        = Cvar_Get("s_bits", "16", CVAR_LATCH | CVAR_ARCHIVE_ND);
	s_khz         = Cvar_Get("s_khz", "44", CVAR_LATCH | CVAR_ARCHIVE_ND);
	s_sdlChannels = Cvar_Get("s_channels", "2", CVAR_LATCH | CVAR_ARCHIVE_ND);

	s_sdlDevSamps   = Cvar_Get("s_sdlDevSamps", "0", CVAR_LATCH | CVAR_ARCHIVE_ND);
	s_sdlMixSamps   = Cvar_Get("s_sdlMixSamps", "0", CVAR_LATCH | CVAR_ARCHIVE_ND);
	s_sdlLevelSamps = Cvar_Get("s_sdlLevelSamps", "0", CVAR_LATCH | CVAR_ARCHIVE_ND);
	s_device        = Cvar_Get("s_device", "-1", CVAR_LATCH | CVAR_ARCHIVE_ND);

	Com_Printf("SDL_Init( SDL_INIT_AUDIO )... ");

	if (!SDL_WasInit(SDL_INIT_AUDIO))
	{
		if (!SDL_Init(SDL_INIT_AUDIO))
		{
			Com_Printf("FAILED (%s)\n", SDL_GetError());
			return qfalse;
		}
	}

	Com_Printf("OK\n");

	driver_name = SDL_GetCurrentAudioDriver();
	if (driver_name)
	{
		Com_Printf("SDL audio driver is \"%s\".\n", driver_name);
	}
	else
	{
		Com_Printf("SDL audio driver isn't initialized.\n");
	}

	Com_Memset(&desired, '\0', sizeof(desired));
	Com_Memset(&obtained, '\0', sizeof(obtained));

	tmp = ((int) s_bits->value);
	if ((tmp != 16) && (tmp != 8))
	{
		tmp = 16;
	}

	desired.freq     = SNDDMA_KHzToHz(s_khz->integer); // desired freq expects Hz not kHz
	desired.format   = ((tmp == 16) ? SDL_AUDIO_S16 : SDL_AUDIO_U8);
	desired.channels = (int) s_sdlChannels->value;

	// I dunno if this is the best idea, but I'll give it a try...
	//  should probably check a cvar for this...
	if (s_sdlDevSamps->value != 0.f)
	{
		desired_samples = s_sdlDevSamps->value;
	}
	else
	{
		desired_samples = SND_SamplesForFreq(desired.freq, s_sdlLevelSamps->integer);
	}

	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, va("%i", desired_samples));

	ids = SDL_GetAudioPlaybackDevices(&count);
	if (s_device->integer < 0 || s_device->integer >= count)
	{
		device_name = NULL;

		//Reset the cvar just in case
		Cvar_Set("s_device", "-1");
	}

	device = s_device->integer >= 0 ? ids[s_device->integer]:SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

	#ifdef ETLEGACY_DEBUG
	SNDDMA_PrintAudiospec("Requested SDL_AudioSpec", &desired, desired_samples);
	#endif

	stream = SDL_OpenAudioDeviceStream(device, &desired, SNDDMA_AudioCallback, NULL);
	if (stream == 0)
	{
		Com_Printf("SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return qfalse;
	}

	device = SDL_GetAudioStreamDevice(stream);
	if (device)
	{
		device_name = SDL_GetAudioDeviceName(device);
		Com_Printf("Acquiring audio device: %s\n", device_name);
	}

	if (!SDL_GetAudioDeviceFormat(device, &obtained, &obtained_samples))
	{
		Com_Printf("SDL_GetAudioDeviceFormat() failed: %s\n", SDL_GetError());
		return qfalse;
	}
	SNDDMA_PrintAudiospec("SDL_AudioSpec", &obtained, obtained_samples);

	// dma.samples needs to be big, or id's mixer will just refuse to
	//  work at all; we need to keep it significantly bigger than the
	//  amount of SDL callback samples, and just copy a little each time
	//  the callback runs.
	// 32768 is what the OSS driver filled in here on my system. I don't
	//  know if it's a good value overall, but at least we know it's
	//  reasonable...this is why I let the user override.
	tmp = s_sdlMixSamps->value;
	if (!tmp)
	{
		if (obtained_samples > 0)
		{
			tmp = (obtained_samples * obtained.channels) * 10;
		}
		else
		{
			tmp = SND_SamplesForFreq(obtained.freq, s_sdlLevelSamps->integer) * obtained.channels * 4;
		}
	}

	if (tmp & (tmp - 1))  // not a power of two? Seems to confuse something.
	{
		int val = 1;
		Com_DPrintf("SDL audio DMA buffer size %d is not a power of two, adjusting...\n", tmp);
		while (val <= tmp)
			val <<= 1;

		tmp = val;
	}

	dmapos               = 0;
	dma.samplebits       = SDL_AUDIO_BITSIZE(obtained.format); // first byte of format is bits.
	dma.channels         = obtained.channels;
	dma.samples          = tmp;
	dma.submission_chunk = 1;
	dma.speed            = obtained.freq;
	dmasize              = (dma.samples * (dma.samplebits / 8));
	dma.buffer           = calloc(1, dmasize);
	if (!dma.buffer)
	{
		Com_Printf("Unable to allocate dma buffer\n");
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return qfalse;
	}

	Com_Printf("Starting SDL audio callback...\n");
	SDL_ResumeAudioStreamDevice(stream);

	Com_Printf("SDL audio initialized.\n");
	snd_inited = qtrue;
	return qtrue;
}

/**
 * @brief SNDDMA_GetDMAPos
 * @return
 */
int SNDDMA_GetDMAPos(void)
{
	return dmapos;
}

/**
 * @brief SNDDMA_Shutdown
 */
void SNDDMA_Shutdown(void)
{
	Com_Printf("Closing SDL audio device...\n");
	if (stream)
	{
		SDL_DestroyAudioStream(stream);
	}
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	Com_Dealloc(dma.buffer);
	dma.buffer = NULL;
	dmapos     = dmasize = 0;
	snd_inited = qfalse;

	Cmd_RemoveCommand("sndlist");

	Com_Printf("SDL audio device shut down.\n");
}

/**
 * @brief Send sound to device if buffer isn't really the dma buffer
 */
void SNDDMA_Submit(void)
{
	if (stream)
	{
		SDL_UnlockAudioStream(stream);
	}
}

/**
 * @brief SNDDMA_BeginPainting
 */
void SNDDMA_BeginPainting(void)
{
	if (stream)
	{
		SDL_LockAudioStream(stream);
	}
}
