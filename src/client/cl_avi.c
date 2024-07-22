/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
 * Copyright (C) 2013 Jere "Jacker" S
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
 * @file cl_avi.c
 * @brief Backported AVI recording from ioquake
 */

#include "client.h"
#include "snd_local.h"

#define INDEX_FILE_EXTENSION ".index.dat"

#define MAX_RIFF_CHUNKS 16

typedef struct audioFormat_s
{
	int rate;
	int format;
	int channels;
	int bits;

	int sampleSize;
	int totalBytes;
} audioFormat_t;

typedef struct aviFileData_s
{
	qboolean fileOpen;
	fileHandle_t f;
	char fileName[MAX_QPATH];
	int fileSize;
	int moviOffset;
	int moviSize;

	fileHandle_t idxF;
	int numIndices;

	int frameRate;
	int framePeriod;
	int width, height;
	int numVideoFrames;
	int maxRecordSize;
	qboolean motionJpeg;

	qboolean audio;
	audioFormat_t a;
	int numAudioFrames;

	int chunkStack[MAX_RIFF_CHUNKS];
	int chunkStackTop;

	byte *cBuffer, *eBuffer;

	qboolean pipe;
} aviFileData_t;

static aviFileData_t afd;

#define MAX_AVI_BUFFER 2048

static byte buffer[MAX_AVI_BUFFER];
static int  bufIndex;

/**
 * @brief SafeFS_Write
 * @param[in] buffer
 * @param[in] len
 * @param[in] f
 */
static ID_INLINE void SafeFS_Write(const void *buffer, int len, fileHandle_t f)
{
	if (FS_Write(buffer, len, f) < len)
	{
		if (videoPipe)
		{
			Com_Error(ERR_DROP, "Failed to write avi file.\n\nMake sure the path to ffmpeg binary is part of $PATH environmental variable, or is placed next to the ETL executable.\n");
		}
		else
		{
			Com_Error(ERR_DROP, "Failed to write avi file\n");
		}
	}
}

/**
 * @brief WRITE_STRING
 * @param[in] s
 */
static ID_INLINE void WRITE_STRING(const char *s)
{
	if (bufIndex + strlen(s) >= MAX_AVI_BUFFER)
	{
		Com_Error(ERR_DROP, "Failed to write string : Buffer overflow");
	}

	Com_Memcpy(&buffer[bufIndex], s, strlen(s));
	bufIndex += strlen(s);
}

/**
 * @brief WRITE_4BYTES
 * @param x
 */
static ID_INLINE void WRITE_4BYTES(int x)
{
	if (bufIndex + 4 >= MAX_AVI_BUFFER)
	{
		Com_Error(ERR_DROP, "Failed to write 4 bytes : Buffer overflow");
	}

	buffer[bufIndex + 0] = (byte) ((x >> 0) & 0xFF);
	buffer[bufIndex + 1] = (byte) ((x >> 8) & 0xFF);
	buffer[bufIndex + 2] = (byte) ((x >> 16) & 0xFF);
	buffer[bufIndex + 3] = (byte) ((x >> 24) & 0xFF);
	bufIndex            += 4;
}

/**
 * @brief WRITE_2BYTES
 * @param x
 */
static ID_INLINE void WRITE_2BYTES(int x)
{
	if (bufIndex + 2 >= MAX_AVI_BUFFER)
	{
		Com_Error(ERR_DROP, "Failed to write 2 bytes : Buffer overflow");
	}

	buffer[bufIndex + 0] = (byte) ((x >> 0) & 0xFF);
	buffer[bufIndex + 1] = (byte) ((x >> 8) & 0xFF);
	bufIndex            += 2;
}

/*
 * @brief WRITE_1BYTES
 * @param[in] x
 *
 * @note Unused
static ID_INLINE void WRITE_1BYTES(int x)
{
    if (bufIndex + 1 >= MAX_AVI_BUFFER)
    {
        Com_Error(ERR_DROP, "Failed to write 1 bytes : Buffer overflow");
    }

    buffer[bufIndex] = x;
    bufIndex        += 1;
}
*/

/**
 * @brief START_CHUNK
 * @param[in] s
 */
static ID_INLINE void START_CHUNK(const char *s)
{
	if (afd.chunkStackTop == MAX_RIFF_CHUNKS)
	{
		Com_Error(ERR_DROP, "Top of chunkstack breached.");
	}

	afd.chunkStack[afd.chunkStackTop] = bufIndex;
	afd.chunkStackTop++;
	WRITE_STRING(s);
	WRITE_4BYTES(0);
}

/**
 * @brief END_CHUNK
 */
static ID_INLINE void END_CHUNK(void)
{
	int endIndex = bufIndex;

	if (afd.chunkStackTop <= 0)
	{
		Com_Error(ERR_DROP, "Bottom of chunkstack breached.");
	}

	afd.chunkStackTop--;
	bufIndex  = afd.chunkStack[afd.chunkStackTop];
	bufIndex += 4;
	WRITE_4BYTES(endIndex - bufIndex - 4);
	bufIndex = endIndex;
	bufIndex = PAD(bufIndex, 2);
}

/**
 * @brief CL_WriteAVIHeader
 */
static void CL_WriteAVIHeader(void)
{
	bufIndex          = 0;
	afd.chunkStackTop = 0;

	START_CHUNK("RIFF");
	{
		WRITE_STRING("AVI ");
		{
			START_CHUNK("LIST");
			{
				WRITE_STRING("hdrl");
				WRITE_STRING("avih");
				WRITE_4BYTES(56);   //"avih" "chunk" size
				WRITE_4BYTES(afd.framePeriod);  //dwMicroSecPerFrame
				WRITE_4BYTES(afd.maxRecordSize * afd.frameRate);    //dwMaxBytesPerSec
				WRITE_4BYTES(0);    //dwReserved1

				if (afd.pipe)
				{
					WRITE_4BYTES(0x100); //dwFlags bits IS_INTERLEAVED=0x100
				}
				else
				{
					WRITE_4BYTES(0x110); //dwFlags bits HAS_INDEX and IS_INTERLEAVED
				}

				WRITE_4BYTES(afd.numVideoFrames);   //dwTotalFrames
				WRITE_4BYTES(0);    //dwInitialFrame

				if (afd.audio)   //dwStreams
				{
					WRITE_4BYTES(2);
				}
				else
				{
					WRITE_4BYTES(1);
				}

				WRITE_4BYTES(afd.maxRecordSize);    //dwSuggestedBufferSize
				WRITE_4BYTES(afd.width);    //dwWidth
				WRITE_4BYTES(afd.height);   //dwHeight
				WRITE_4BYTES(0);    //dwReserved[ 0 ]
				WRITE_4BYTES(0);    //dwReserved[ 1 ]
				WRITE_4BYTES(0);    //dwReserved[ 2 ]
				WRITE_4BYTES(0);    //dwReserved[ 3 ]

				START_CHUNK("LIST");
				{
					WRITE_STRING("strl");
					WRITE_STRING("strh");
					WRITE_4BYTES(56);   //"strh" "chunk" size
					WRITE_STRING("vids");

					if (afd.motionJpeg && !afd.pipe)
					{
						WRITE_STRING("MJPG");
					}
					else
					{
						WRITE_4BYTES(0);    // BI_RGB
					}

					WRITE_4BYTES(0);    //dwFlags
					WRITE_4BYTES(0);    //dwPriority
					WRITE_4BYTES(0);    //dwInitialFrame

					WRITE_4BYTES(1);    //dwTimescale
					WRITE_4BYTES(afd.frameRate);    //dwDataRate
					WRITE_4BYTES(0);    //dwStartTime
					WRITE_4BYTES(afd.numVideoFrames);   //dwDataLength

					WRITE_4BYTES(afd.maxRecordSize);    //dwSuggestedBufferSize
					WRITE_4BYTES(0);    //dwQuality
					WRITE_4BYTES(0);    //dwSampleSize
					WRITE_2BYTES(0);    //rcFrame
					WRITE_2BYTES(0);    //rcFrame
					WRITE_2BYTES(afd.width);    //rcFrame
					WRITE_2BYTES(afd.height);   //rcFrame

					WRITE_STRING("strf");
					WRITE_4BYTES(40);   //"strf" "chunk" size
					WRITE_4BYTES(40);   //biSize
					WRITE_4BYTES(afd.width);    //biWidth
					WRITE_4BYTES(afd.height);   //biHeight
					WRITE_2BYTES(1);    //biPlanes
					WRITE_2BYTES(24);   //biBitCount

					if (afd.motionJpeg && !afd.pipe)  //biCompression
					{
						WRITE_STRING("MJPG");
						WRITE_4BYTES(afd.width * afd.height);   //biSizeImage
					}
					else
					{
						WRITE_4BYTES(0);    // BI_RGB
						WRITE_4BYTES(afd.width * afd.height * 3);   //biSizeImage
					}

					WRITE_4BYTES(0);    //biXPelsPetMeter
					WRITE_4BYTES(0);    //biYPelsPetMeter
					WRITE_4BYTES(0);    //biClrUsed
					WRITE_4BYTES(0);    //biClrImportant
				}
				END_CHUNK();

				if (afd.audio)
				{
					START_CHUNK("LIST");
					{
						WRITE_STRING("strl");
						WRITE_STRING("strh");
						WRITE_4BYTES(56);   //"strh" "chunk" size
						WRITE_STRING("auds");
						WRITE_4BYTES(0);    //FCC
						WRITE_4BYTES(0);    //dwFlags
						WRITE_4BYTES(0);    //dwPriority
						WRITE_4BYTES(0);    //dwInitialFrame

						WRITE_4BYTES(afd.a.sampleSize); //dwTimescale
						WRITE_4BYTES(afd.a.sampleSize * afd.a.rate);    //dwDataRate
						WRITE_4BYTES(0);    //dwStartTime
						WRITE_4BYTES(afd.a.totalBytes / afd.a.sampleSize);  //dwDataLength

						WRITE_4BYTES(0);    //dwSuggestedBufferSize
						WRITE_4BYTES(0);    //dwQuality
						WRITE_4BYTES(afd.a.sampleSize); //dwSampleSize
						WRITE_2BYTES(0);    //rcFrame
						WRITE_2BYTES(0);    //rcFrame
						WRITE_2BYTES(0);    //rcFrame
						WRITE_2BYTES(0);    //rcFrame

						WRITE_STRING("strf");
						WRITE_4BYTES(18);   //"strf" "chunk" size
						WRITE_2BYTES(afd.a.format); //wFormatTag
						WRITE_2BYTES(afd.a.channels);   //nChannels
						WRITE_4BYTES(afd.a.rate);   //nSamplesPerSec
						WRITE_4BYTES(afd.a.sampleSize * afd.a.rate);    //nAvgBytesPerSec
						WRITE_2BYTES(afd.a.sampleSize); //nBlockAlign
						WRITE_2BYTES(afd.a.bits);   //wBitsPerSample
						WRITE_2BYTES(0);    //cbSize
					}
					END_CHUNK();
				}
			}
			END_CHUNK();

			afd.moviOffset = bufIndex;

			START_CHUNK("LIST");
			{
				WRITE_STRING("movi");
			}
		}
	}
}

/**
 * @brief CL_ValidatePipeFormat
 * @param[in] fileName
 * @return qtrue if pipe format is valid
 */
static qboolean CL_ValidatePipeFormat(const char *s)
{
	while (*s != '\0')
	{
		if (*s == '.' && *(s + 1) == '.' && (*(s + 2) == '/' || *(s + 2) == '\\'))
		{
			return qfalse;
		}
		if (*s == ':' && *(s + 1) == ':')
		{
			return qfalse;
		}
		if (*s == '>' || *s == '|' || *s == '&')
		{
			return qfalse;
		}
		s++;
	}

	return qtrue;
}

/**
 * @brief Creates an AVI file and gets it into a state where
 * writing the actual data can begin
 *
 * @param[in] fileName
 * @return
 */
qboolean CL_OpenAVIForWriting(const char *fileName, qboolean pipe)
{
	if (afd.fileOpen)
	{
		return qfalse;
	}

	Com_Memset(&afd, 0, sizeof(aviFileData_t));

	if (pipe)
	{
		char       cmd[MAX_OSPATH * 4];
		const char *ospath;
		const char *cmd_fmt = "ffmpeg -f avi -i - -threads 0 -y %s \"%s\" 2> \"%s-log.txt\"";

		if (!CL_ValidatePipeFormat(cl_aviPipeFormat->string))
		{
			Com_Printf(S_COLOR_RED "Invalid pipe format: %s\n", cl_aviPipeFormat->string);
			return qfalse;
		}

		ospath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), "", fileName);
		Com_sprintf(cmd, sizeof(cmd), cmd_fmt, cl_aviPipeFormat->string, ospath, ospath);

		if ((afd.f = FS_PipeOpenWrite(cmd, fileName)) <= 0)
		{
			return qfalse;
		}
	}
	else
	{
		if ((afd.f = FS_FOpenFileWrite(fileName)) <= 0)
		{
			return qfalse;
		}

		if ((afd.idxF = FS_FOpenFileWrite(va("%s" INDEX_FILE_EXTENSION, fileName))) <= 0)
		{
			FS_FCloseFile(afd.f);
			return qfalse;
		}
	}

	Q_strncpyz(afd.fileName, fileName, MAX_QPATH);

	afd.frameRate   = cl_aviFrameRate->integer;
	afd.framePeriod = (int)(1000000.0f / afd.frameRate);
	afd.width       = cls.glconfig.windowWidth;
	afd.height      = cls.glconfig.windowHeight;

	if (cl_aviMotionJpeg->integer && !pipe)
	{
		afd.motionJpeg = qtrue;
	}
	else
	{
		afd.motionJpeg = qfalse;
	}

	afd.cBuffer = Z_Malloc(afd.width * afd.height * 4);
	afd.eBuffer = Z_Malloc(afd.width * afd.height * 4);

	afd.a.rate       = dma.speed;
	afd.a.format     = WAV_FORMAT_PCM;
	afd.a.channels   = dma.channels;
	afd.a.bits       = dma.samplebits;
	afd.a.sampleSize = (afd.a.bits / 8) * afd.a.channels;

	if (afd.a.rate % afd.frameRate)
	{
		int suggestRate = afd.frameRate;

		while ((afd.a.rate % suggestRate) && suggestRate >= 1)
			suggestRate--;

		Com_Printf(S_COLOR_YELLOW "WARNING: cl_avidemo is not a divisor " "of the audio rate, suggest %d\n", suggestRate);
	}

	if (!Cvar_VariableIntegerValue("s_initsound"))
	{
		afd.audio = qfalse;
	}
	else if (Q_stricmp(Cvar_VariableString("s_backend"), "OpenAL"))
	{
		if (afd.a.bits != 16 || afd.a.channels != 2)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Audio format of %d bit/%d channels not supported", afd.a.bits, afd.a.channels);
			afd.audio = qfalse;
		}
		else
		{
			afd.audio = qtrue;
		}
	}
	else
	{
		afd.audio = qfalse;
		Com_Printf(S_COLOR_YELLOW "WARNING: Audio capture is not supported "
		                          "with OpenAL. Set s_useOpenAL to 0 for audio capture\n");
	}

	// This doesn't write a real header, but allocates the
	// correct amount of space at the beginning of the file
	CL_WriteAVIHeader();

	if (pipe)
	{
		afd.pipe = qtrue;
		SafeFS_Write(buffer, bufIndex, afd.f);
		bufIndex = 0;
	}
	else
	{
		SafeFS_Write(buffer, bufIndex, afd.f);
		afd.fileSize = bufIndex;

		bufIndex = 0;
		START_CHUNK("idx1");
		SafeFS_Write(buffer, bufIndex, afd.idxF);

		afd.moviSize = 4;           // For the "movi"
	}

	afd.fileOpen = qtrue;

	return qtrue;
}

/**
 * @brief CL_CheckFileSize
 * @param[in] bytesToAdd
 * @return
 */
static qboolean CL_CheckFileSize(int bytesToAdd)
{
	unsigned int newFileSize;

	if (afd.pipe)
	{
		return qfalse;
	}

	// current file size + what we want to add + the index + the index size
	newFileSize = afd.fileSize + bytesToAdd + (afd.numIndices * 16) + 4;

	// I assume all the operating systems
	// we target can handle a 2Gb file
	if (newFileSize > INT_MAX)
	{
		// Close the current file...
		CL_CloseAVI();

		// ...And open a new one
		CL_OpenAVIForWriting(va("%s_", afd.fileName), videoPipe);

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CL_WriteAVIVideoFrame
 * @param[in] imageBuffer
 * @param[in] size
 */
void CL_WriteAVIVideoFrame(const byte *imageBuffer, int size)
{
	int  chunkOffset = afd.fileSize - afd.moviOffset - 8;
	int  chunkSize   = 8 + size;
	int  paddingSize = PAD(size, 2) - size;
	byte padding[4]  = { 0 };

	if (!afd.fileOpen)
	{
		return;
	}

	// Chunk header + contents + padding
	if (CL_CheckFileSize(8 + size + 2))
	{
		return;
	}

	bufIndex = 0;
	WRITE_STRING("00dc");
	WRITE_4BYTES(size);

	SafeFS_Write(buffer, 8, afd.f);
	SafeFS_Write(imageBuffer, size, afd.f);
	SafeFS_Write(padding, paddingSize, afd.f);

	afd.numVideoFrames++;

	if (size > afd.maxRecordSize)
	{
		afd.maxRecordSize = size;
	}

	if (afd.pipe)
	{
		return;
	}

	afd.fileSize += (chunkSize + paddingSize);
	afd.moviSize += (chunkSize + paddingSize);

	// Index
	bufIndex = 0;
	WRITE_STRING("00dc");       //dwIdentifier
	WRITE_4BYTES(0x00000010);   //dwFlags (all frames are KeyFrames)
	WRITE_4BYTES(chunkOffset);  //dwOffset
	WRITE_4BYTES(size);         //dwLength
	SafeFS_Write(buffer, 16, afd.idxF);

	afd.numIndices++;
}

#define PCM_BUFFER_SIZE 44100

/**
 * @brief CL_WriteAVIAudioFrame
 * @param[in] pcmBuffer
 * @param[in] size
 */
void CL_WriteAVIAudioFrame(const byte *pcmBuffer, int size)
{
	static byte pcmCaptureBuffer[PCM_BUFFER_SIZE] = { 0 };
	static int  bytesInBuffer                     = 0;

	if (!afd.audio)
	{
		return;
	}

	if (!afd.fileOpen)
	{
		return;
	}

	// Chunk header + contents + padding
	if (CL_CheckFileSize(8 + bytesInBuffer + size + 2))
	{
		return;
	}

	if (bytesInBuffer + size > PCM_BUFFER_SIZE)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Audio capture buffer overflow -- truncating\n");
		size = PCM_BUFFER_SIZE - bytesInBuffer;
	}

	Com_Memcpy(&pcmCaptureBuffer[bytesInBuffer], pcmBuffer, (unsigned int)size);
	bytesInBuffer += size;

	// Only write if we have a frame's worth of audio
	if (bytesInBuffer >= (int)(ceil((double)afd.a.rate / (double)afd.frameRate) * afd.a.sampleSize))
	{
		int  chunkOffset = afd.fileSize - afd.moviOffset - 8;
		int  chunkSize   = 8 + bytesInBuffer;
		int  paddingSize = PAD(bytesInBuffer, 2) - bytesInBuffer;
		byte padding[4]  = { 0 };

		bufIndex = 0;
		WRITE_STRING("01wb");
		WRITE_4BYTES(bytesInBuffer);

		SafeFS_Write(buffer, 8, afd.f);
		SafeFS_Write(pcmCaptureBuffer, bytesInBuffer, afd.f);
		SafeFS_Write(padding, paddingSize, afd.f);
		afd.numAudioFrames++;

		if (!afd.pipe)
		{
			afd.fileSize     += (chunkSize + paddingSize);
			afd.moviSize     += (chunkSize + paddingSize);
			afd.a.totalBytes += bytesInBuffer;

			// Index
			bufIndex = 0;
			WRITE_STRING("01wb");   //dwIdentifier
			WRITE_4BYTES(0);        //dwFlags
			WRITE_4BYTES(chunkOffset);  //dwOffset
			WRITE_4BYTES(bytesInBuffer);    //dwLength
			SafeFS_Write(buffer, 16, afd.idxF);

			afd.numIndices++;
		}

		bytesInBuffer = 0;
	}
}

/**
 * @brief CL_TakeVideoFrame
 */
void CL_TakeVideoFrame(void)
{
	// AVI file isn't open
	if (!afd.fileOpen)
	{
		return;
	}

	re.TakeVideoFrame(afd.width, afd.height, afd.cBuffer, afd.eBuffer, afd.motionJpeg);
}

/**
 * @brief Closes the AVI file and writes an index chunk
 * @return
 */
qboolean CL_CloseAVI(void)
{
	int        indexRemainder;
	int        indexSize    = afd.numIndices * 16;
	const char *idxFileName = va("%s" INDEX_FILE_EXTENSION, afd.fileName);

	// AVI file isn't open
	if (!afd.fileOpen)
	{
		return qfalse;
	}

	Z_Free(afd.cBuffer);
	Z_Free(afd.eBuffer);

	if (afd.pipe)
	{
		Com_Printf("Wrote %d:%d (v:a) frames to pipe: %s\n", afd.numVideoFrames, afd.numAudioFrames, afd.fileName);
		FS_FCloseFile(afd.f);
		afd.f        = 0;
		afd.fileOpen = qfalse;
		afd.pipe     = qfalse;
		return qtrue;
	}

	afd.fileOpen = qfalse;

	(void) FS_Seek(afd.idxF, 4, FS_SEEK_SET);
	bufIndex = 0;
	WRITE_4BYTES(indexSize);
	SafeFS_Write(buffer, bufIndex, afd.idxF);
	FS_FCloseFile(afd.idxF);

	// Write index

	// Open the temp index file
	if ((indexSize = FS_FOpenFileRead(idxFileName, &afd.idxF, qtrue)) <= 0)
	{
		FS_FCloseFile(afd.f);
		return qfalse;
	}

	indexRemainder = indexSize;

	// Append index to end of avi file
	while (indexRemainder > MAX_AVI_BUFFER)
	{
		FS_Read(buffer, MAX_AVI_BUFFER, afd.idxF);
		SafeFS_Write(buffer, MAX_AVI_BUFFER, afd.f);
		afd.fileSize   += MAX_AVI_BUFFER;
		indexRemainder -= MAX_AVI_BUFFER;
	}
	FS_Read(buffer, indexRemainder, afd.idxF);
	SafeFS_Write(buffer, indexRemainder, afd.f);
	afd.fileSize += indexRemainder;
	FS_FCloseFile(afd.idxF);

	// Remove temp index file
	FS_HomeRemove(idxFileName);

	// Write the real header
	(void) FS_Seek(afd.f, 0, FS_SEEK_SET);
	CL_WriteAVIHeader();

	bufIndex = 4;
	WRITE_4BYTES(afd.fileSize - 8); // "RIFF" size

	bufIndex = afd.moviOffset + 4;  // Skip "LIST"
	WRITE_4BYTES(afd.moviSize);

	SafeFS_Write(buffer, bufIndex, afd.f);

	FS_FCloseFile(afd.f);

	Com_Printf("Wrote %d:%d (v:a) frames to: %s\n", afd.numVideoFrames, afd.numAudioFrames, afd.fileName);

	return qtrue;
}

/**
 * @brief CL_VideoRecording
 * @return
 */
qboolean CL_VideoRecording(void)
{
	return afd.fileOpen;
}
