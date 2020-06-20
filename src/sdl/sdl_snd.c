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

/* The audio callback. All the magic happens here. */
static int               dmapos    = 0;
static int               dmasize   = 0;
static SDL_AudioDeviceID device_id = 0;

/**
 * @brief SNDDMA_AudioCallback
 * @param userdata - unused
 * @param[in] stream
 * @param[in] len
 */
static void SNDDMA_AudioCallback(void *userdata, Uint8 *stream, int len)
{
	int pos = (dmapos * (dma.samplebits / 8));

	if (pos >= dmasize)
	{
		dmapos = pos = 0;
	}

	if (!snd_inited)  /* shouldn't happen, but just in case... */
	{
		Com_Memset(stream, '\0', len);
		return;
	}
	else
	{
		int tobufend = dmasize - pos;  /* bytes to buffer's end. */
		int len1     = len;
		int len2     = 0;

		if (len1 > tobufend)
		{
			len1 = tobufend;
			len2 = len - len1;
		}
		Com_Memcpy(stream, dma.buffer + pos, len1);
		if (len2 <= 0)
		{
			dmapos += (len1 / (dma.samplebits / 8));
		}
		else  /* wraparound? */
		{
			Com_Memcpy(stream + len1, dma.buffer, len2);
			dmapos = (len2 / (dma.samplebits / 8));
		}
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

}

formatToStringTable[] =
{
	{ AUDIO_U8,     "AUDIO_U8"     },
	{ AUDIO_S8,     "AUDIO_S8"     },
	{ AUDIO_U16LSB, "AUDIO_U16LSB" },
	{ AUDIO_U16MSB, "AUDIO_U16MSB" },
	{ AUDIO_S16LSB, "AUDIO_S16LSB" },
	{ AUDIO_S16MSB, "AUDIO_S16MSB" },
	{ AUDIO_S32LSB, "AUDIO_S32LSB" },
	{ AUDIO_S32MSB, "AUDIO_S32MSB" },
	{ AUDIO_F32LSB, "AUDIO_F32LSB" },
	{ AUDIO_F32MSB, "AUDIO_F32MSB" },
};

static int formatToStringTableSize = ARRAY_LEN(formatToStringTable);

/**
 * @brief SNDDMA_PrintAudiospec
 * @param[in] str
 * @param[in] spec
 */
static void SNDDMA_PrintAudiospec(const char *str, const SDL_AudioSpec *spec)
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
	Com_Printf("  Samples:  %d\n", (int) spec->samples);
	Com_Printf("  Channels: %d\n", (int) spec->channels);
	Com_Printf("  Silence:  %d\n", (int) spec->silence);
	Com_Printf("  Size:     %d\n", (int) spec->size);
}

/**
 * @brief SNDDMA_KHzToHz
 */
static int SNDDMA_KHzToHz(int khz)
{
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
}

/**
 * @brief SND_DeviceList
 */
static void SND_DeviceList(void)
{
	int i, count = SDL_GetNumAudioDevices(qfalse);

	Com_Printf("Printing audio device list. Number of devices: %i\n\n", count);

	for (i = 0; i < count; ++i)
	{
		Com_Printf("  Audio device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
	}
}

/**
 * @brief SNDDMA_Init
 * @return
 */
qboolean SNDDMA_Init(void)
{
	SDL_AudioSpec desired;
	SDL_AudioSpec obtained;
	const char    *driver_name;
	const char    *device_name;
	int           tmp;

	if (snd_inited)
	{
		return qtrue;
	}

	Cmd_AddCommand("sndlist", SND_DeviceList, "Prints a list of available sound devices.");

	s_bits        = Cvar_Get("s_bits", "16", CVAR_LATCH | CVAR_ARCHIVE);
	s_khz         = Cvar_Get("s_khz", "44", CVAR_LATCH | CVAR_ARCHIVE);
	s_sdlChannels = Cvar_Get("s_channels", "2", CVAR_LATCH | CVAR_ARCHIVE);

	s_sdlDevSamps = Cvar_Get("s_sdlDevSamps", "0", CVAR_LATCH | CVAR_ARCHIVE);
	s_sdlMixSamps = Cvar_Get("s_sdlMixSamps", "0", CVAR_LATCH | CVAR_ARCHIVE);
	s_device      = Cvar_Get("s_device", "-1", CVAR_LATCH | CVAR_ARCHIVE);

	Com_Printf("SDL_Init( SDL_INIT_AUDIO )... ");

	if (!SDL_WasInit(SDL_INIT_AUDIO))
	{
		if (SDL_Init(SDL_INIT_AUDIO) < 0)
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

	desired.freq = SNDDMA_KHzToHz(s_khz->integer); // desired freq expects Hz not kHz

	desired.format = ((tmp == 16) ? AUDIO_S16SYS : AUDIO_U8);

	// I dunno if this is the best idea, but I'll give it a try...
	//  should probably check a cvar for this...
	if (s_sdlDevSamps->value != 0.f)
	{
		desired.samples = s_sdlDevSamps->value;
	}
	else
	{
		// just pick a sane default.
		if (desired.freq <= 11025)
		{
			desired.samples = 256;
		}
		else if (desired.freq <= 22050)
		{
			desired.samples = 512;
		}
		else if (desired.freq <= 44100)
		{
			desired.samples = 1024;
		}
		else
		{
			desired.samples = 2048;  // (*shrug*)
		}
	}

	desired.channels = (int) s_sdlChannels->value;
	desired.callback = SNDDMA_AudioCallback;

	if (s_device->integer >= 0 && s_device->integer < SDL_GetNumAudioDevices(qfalse))
	{
		device_name = SDL_GetAudioDeviceName(s_device->integer, 0);
		Com_Printf("Acquiring audio device: %s\n", device_name);
	}
	else
	{
		device_name = NULL;

		//Reset the cvar just in case
		Cvar_Set("s_device", "-1");
	}

	device_id = SDL_OpenAudioDevice(device_name, qfalse, &desired, &obtained, 0);
	if (device_id == 0)
	{
		Com_Printf("SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return qfalse;
	}

	SNDDMA_PrintAudiospec("SDL_AudioSpec", &obtained);

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
		tmp = (obtained.samples * obtained.channels) * 10;
	}

	if (tmp & (tmp - 1))  // not a power of two? Seems to confuse something.
	{
		int val = 1;
		while (val < tmp)
			val <<= 1;

		tmp = val;
	}

	dmapos               = 0;
	dma.samplebits       = obtained.format & 0xFF; // first byte of format is bits.
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
	SDL_PauseAudioDevice(device_id, 0); // start callback.

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
	SDL_CloseAudioDevice(device_id);
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
	SDL_UnlockAudioDevice(device_id);
}

/**
 * @brief SNDDMA_BeginPainting
 */
void SNDDMA_BeginPainting(void)
{
	SDL_LockAudioDevice(device_id);
}
