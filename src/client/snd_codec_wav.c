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
 * @file snd_codec_wav.c
 */

#include "client.h"
#include "snd_codec.h"

/**
 * @brief FGetLittleLong
 * @param[in] f
 * @return
 */
static int FGetLittleLong(fileHandle_t f)
{
	int v;

	FS_Read(&v, sizeof(v), f);

	return LittleLong(v);
}

/**
 * @brief FGetLittleShort
 * @param[in] f
 * @return
 */
static short FGetLittleShort(fileHandle_t f)
{
	short v;

	FS_Read(&v, sizeof(v), f);

	return LittleShort(v);
}

/**
 * @brief S_ReadChunkInfo
 * @param[in] f
 * @param[in,out] name
 * @return
 */
static int S_ReadChunkInfo(fileHandle_t f, char *name)
{
	int len, r;

	name[4] = 0;

	r = FS_Read(name, 4, f);
	if (r != 4)
	{
		return -1;
	}

	len = FGetLittleLong(f);
	if (len < 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Negative chunk length\n");
		return -1;
	}

	return len;
}

/**
 * @brief S_FindRIFFChunk
 * @param[in] f
 * @param[in] chunk
 * @return The length of the data in the chunk, or -1 if not found
 */
static int S_FindRIFFChunk(fileHandle_t f, const char *chunk)
{
	char name[5];
	int  len;

	while ((len = S_ReadChunkInfo(f, name)) >= 0)
	{
		// If this is the right chunk, return
		if (!Q_strncmp(name, chunk, 4))
		{
			return len;
		}

		len = PAD(len, 2);

		// Not the right chunk - skip it
		(void) FS_Seek(f, len, FS_SEEK_CUR);
	}

	return -1;
}

/**
 * @brief S_ByteSwapRawSamples
 * @param[in] samples
 * @param[in] width
 * @param[in] s_channels
 * @param[in,out] data
 */
static void S_ByteSwapRawSamples(int samples, int width, int s_channels, byte *data)
{
	int i;

	if (width != 2)
	{
		return;
	}
	if (LittleShort(256) == 256)
	{
		return;
	}

	if (s_channels == 2)
	{
		samples <<= 1;
	}
	for (i = 0 ; i < samples ; i++)
	{
		((short *)data)[i] = LittleShort(((short *)data)[i]);
	}
}

/**
 * @brief S_ReadRIFFHeader
 * @param[in] file
 * @param[out] info
 * @return
 */
static qboolean S_ReadRIFFHeader(fileHandle_t file, snd_info_t *info)
{
	char dump[16];
	//int  wav_format;
	int bits;
	int fmtlen = 0;

	// skip the riff wav header
	FS_Read(dump, 12, file);

	// Scan for the format chunk
	if ((fmtlen = S_FindRIFFChunk(file, "fmt ")) < 0)
	{
		Com_Printf(S_COLOR_RED "ERROR: Couldn't find \"fmt\" chunk\n");
		return qfalse;
	}

	// Save the parameters
	//wav_format     = FGetLittleShort(file);
	FGetLittleShort(file);
	info->channels = FGetLittleShort(file);
	info->rate     = FGetLittleLong(file);
	FGetLittleLong(file);
	FGetLittleShort(file);
	bits = FGetLittleShort(file);

	if (bits < 8)
	{
		Com_Printf(S_COLOR_RED "ERROR: Less than 8 bit sound is not supported\n");
		return qfalse;
	}

	info->width   = bits / 8;
	info->dataofs = 0;

	// Skip the rest of the format chunk if required
	if (fmtlen > 16)
	{
		fmtlen -= 16;
		(void) FS_Seek(file, fmtlen, FS_SEEK_CUR);
	}

	// Scan for the data chunk
	if ((info->size = S_FindRIFFChunk(file, "data")) < 0)
	{
		Com_Printf(S_COLOR_RED "ERROR: Couldn't find \"data\" chunk\n");
		return qfalse;
	}
	info->samples = (info->size / info->width) / info->channels;

	return qtrue;
}

// WAV codec
snd_codec_t wav_codec =
{
	".wav",
	S_WAV_CodecLoad,
	S_WAV_CodecOpenStream,
	S_WAV_CodecReadStream,
	S_WAV_CodecCloseStream,
	NULL
};

/**
 * @brief S_WAV_CodecLoad
 * @param[in] filename
 * @param[in] info
 * @return
 */
void *S_WAV_CodecLoad(const char *filename, snd_info_t *info)
{
	fileHandle_t file;
	void         *buffer;

	// Try to open the file
	FS_FOpenFileRead(filename, &file, qtrue);
	if (!file)
	{
		if (com_developer->integer)
		{
			Com_Printf(S_COLOR_RED "ERROR: Could not open \"%s\"\n", filename);
		}
		return NULL;
	}

	// Read the RIFF header
	if (!S_ReadRIFFHeader(file, info))
	{
		FS_FCloseFile(file);
		if (com_developer->integer)
		{
			Com_Printf(S_COLOR_RED "ERROR: Incorrect/unsupported format in \"%s\"\n", filename);
		}
		return NULL;
	}

	// Allocate some memory
	buffer = Hunk_AllocateTempMemory(info->size);
	if (!buffer)
	{
		FS_FCloseFile(file);
		Com_Printf(S_COLOR_RED "ERROR: Out of memory reading \"%s\"\n", filename);
		return NULL;
	}

	// Read, byteswap
	FS_Read(buffer, info->size, file);
	S_ByteSwapRawSamples(info->samples, info->width, info->channels, (byte *)buffer);

	// Close and return
	FS_FCloseFile(file);
	return buffer;
}

/**
 * @brief S_WAV_CodecOpenStream
 * @param[in] filename
 * @return
 */
snd_stream_t *S_WAV_CodecOpenStream(const char *filename)
{
	snd_stream_t *rv;

	// Open
	rv = S_CodecUtilOpen(filename, &wav_codec);
	if (!rv)
	{
		return NULL;
	}

	// Read the RIFF header
	if (!S_ReadRIFFHeader(rv->file, &rv->info))
	{
		S_CodecUtilClose(&rv);
		return NULL;
	}

	return rv;
}

/**
 * @brief S_WAV_CodecCloseStream
 * @param[in,out] stream
 */
void S_WAV_CodecCloseStream(snd_stream_t *stream)
{
	S_CodecUtilClose(&stream);
}

/**
 * @brief S_WAV_CodecReadStream
 * @param[in,out] stream
 * @param[in] bytes
 * @param[out] buffer
 * @return
 */
int S_WAV_CodecReadStream(snd_stream_t *stream, int bytes, void *buffer)
{
	int remaining = stream->info.size - stream->pos;
	int samples;

	if (remaining <= 0)
	{
		return 0;
	}
	if (bytes > remaining)
	{
		bytes = remaining;
	}
	stream->pos += bytes;
	samples      = (bytes / stream->info.width) / stream->info.channels;
	FS_Read(buffer, bytes, stream->file);
	S_ByteSwapRawSamples(samples, stream->info.width, stream->info.channels, buffer);
	return bytes;
}
