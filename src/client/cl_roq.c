/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * Copyright (C) 2015 Team Blur Games.
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
 * @file cl_roq.c
 * @brief roq format decoding
 *
 * A RoQ format is comprised of a series of chunks. These chunks may indicate the
 * format signature or playback parameters, or may contain encoded pieces of video
 * frames or audio wave forms.
 */

#include "client.h"

typedef struct
{
	uint16_t id;
	uint32_t size;
	uint16_t args;
} roqChunkHeader_t;

#define chunkHeader ((roqChunkHeader_t *)cin->data)

/*
 ==============================================================================

 ROQ files are used for cinematics and in-game videos

 ==============================================================================
*/

#define ROQ_ID                            0x1084

#define ROQ_CHUNK_HEADER_SIZE            8
#define ROQ_CHUNK_MAX_DATA_SIZE            131072

#define ROQ_QUAD_INFO                    0x1001
#define ROQ_QUAD_CODEBOOK                0x1002
#define ROQ_QUAD_VQ                        0x1011
#define ROQ_QUAD_JPEG                    0x1012
#define ROQ_QUAD_HANG                    0x1013

#define ROQ_SOUND_MONO_22                0x1020
#define ROQ_SOUND_STEREO_22                0x1021
#define ROQ_SOUND_MONO_48                0x1022
#define ROQ_SOUND_STEREO_48                0x1023

#define ROQ_VQ_MOT                        0x0000
#define ROQ_VQ_FCC                        0x4000
#define ROQ_VQ_SLD                        0x8000
#define ROQ_VQ_CCC                        0xC000

static int16_t cin_cr2rTable[256];
static int16_t cin_cb2gTable[256];
static int16_t cin_cr2gTable[256];
static int16_t cin_cb2bTable[256];

static short cin_sqrTable[256];

static uint32_t cin_quadVQ2[256 << 2];
static uint32_t cin_quadVQ4[256 << 4];
static uint32_t cin_quadVQ8[256 << 6];

static short cin_soundSamples[ROQ_CHUNK_MAX_DATA_SIZE >> 2];

static byte cin_chunkData[ROQ_CHUNK_HEADER_SIZE + ROQ_CHUNK_MAX_DATA_SIZE];

/**
 * @brief CIN_BlitBlock2x2
 * @param[in] cin
 * @param[in] x
 * @param[in] y
 * @param[in] index
 */
static void CIN_BlitBlock2x2(cinematic_t *cin, int x, int y, int index)
{
	uint32_t *src, *dst;

	src = &cin_quadVQ2[index << 2];
	dst = (uint32_t *) cin->frameBuffer[0] + (y * cin->frameWidth + x);

	dst[cin->frameWidth * 0 + 0] = src[0];
	dst[cin->frameWidth * 0 + 1] = src[1];

	dst[cin->frameWidth * 1 + 0] = src[2];
	dst[cin->frameWidth * 1 + 1] = src[3];
}

/**
 * @brief CIN_BlitBlock4x4
 * @param[in] cin
 * @param[in] x
 * @param[in] y
 * @param[in] index
 */
static void CIN_BlitBlock4x4(cinematic_t *cin, int x, int y, int index)
{
	uint32_t *src, *dst;

	src = &cin_quadVQ4[index << 4];
	dst = (uint32_t *) cin->frameBuffer[0] + (y * cin->frameWidth + x);

	dst[cin->frameWidth * 0 + 0] = src[0];
	dst[cin->frameWidth * 0 + 1] = src[1];
	dst[cin->frameWidth * 0 + 2] = src[2];
	dst[cin->frameWidth * 0 + 3] = src[3];

	dst[cin->frameWidth * 1 + 0] = src[4];
	dst[cin->frameWidth * 1 + 1] = src[5];
	dst[cin->frameWidth * 1 + 2] = src[6];
	dst[cin->frameWidth * 1 + 3] = src[7];

	dst[cin->frameWidth * 2 + 0] = src[8];
	dst[cin->frameWidth * 2 + 1] = src[9];
	dst[cin->frameWidth * 2 + 2] = src[10];
	dst[cin->frameWidth * 2 + 3] = src[11];

	dst[cin->frameWidth * 3 + 0] = src[12];
	dst[cin->frameWidth * 3 + 1] = src[13];
	dst[cin->frameWidth * 3 + 2] = src[14];
	dst[cin->frameWidth * 3 + 3] = src[15];
}

/**
 * @brief CIN_BlitBlock8x8
 * @param[in] cin
 * @param[in] x
 * @param[in] y
 * @param[in] index
 */
static void CIN_BlitBlock8x8(cinematic_t *cin, int x, int y, int index)
{
	uint32_t *src, *dst;

	src = &cin_quadVQ8[index << 6];
	dst = (uint32_t *) cin->frameBuffer[0] + (y * cin->frameWidth + x);

	dst[cin->frameWidth * 0 + 0] = src[0];
	dst[cin->frameWidth * 0 + 1] = src[1];
	dst[cin->frameWidth * 0 + 2] = src[2];
	dst[cin->frameWidth * 0 + 3] = src[3];
	dst[cin->frameWidth * 0 + 4] = src[4];
	dst[cin->frameWidth * 0 + 5] = src[5];
	dst[cin->frameWidth * 0 + 6] = src[6];
	dst[cin->frameWidth * 0 + 7] = src[7];

	dst[cin->frameWidth * 1 + 0] = src[8];
	dst[cin->frameWidth * 1 + 1] = src[9];
	dst[cin->frameWidth * 1 + 2] = src[10];
	dst[cin->frameWidth * 1 + 3] = src[11];
	dst[cin->frameWidth * 1 + 4] = src[12];
	dst[cin->frameWidth * 1 + 5] = src[13];
	dst[cin->frameWidth * 1 + 6] = src[14];
	dst[cin->frameWidth * 1 + 7] = src[15];

	dst[cin->frameWidth * 2 + 0] = src[16];
	dst[cin->frameWidth * 2 + 1] = src[17];
	dst[cin->frameWidth * 2 + 2] = src[18];
	dst[cin->frameWidth * 2 + 3] = src[19];
	dst[cin->frameWidth * 2 + 4] = src[20];
	dst[cin->frameWidth * 2 + 5] = src[21];
	dst[cin->frameWidth * 2 + 6] = src[22];
	dst[cin->frameWidth * 2 + 7] = src[23];

	dst[cin->frameWidth * 3 + 0] = src[24];
	dst[cin->frameWidth * 3 + 1] = src[25];
	dst[cin->frameWidth * 3 + 2] = src[26];
	dst[cin->frameWidth * 3 + 3] = src[27];
	dst[cin->frameWidth * 3 + 4] = src[28];
	dst[cin->frameWidth * 3 + 5] = src[29];
	dst[cin->frameWidth * 3 + 6] = src[30];
	dst[cin->frameWidth * 3 + 7] = src[31];

	dst[cin->frameWidth * 4 + 0] = src[32];
	dst[cin->frameWidth * 4 + 1] = src[33];
	dst[cin->frameWidth * 4 + 2] = src[34];
	dst[cin->frameWidth * 4 + 3] = src[35];
	dst[cin->frameWidth * 4 + 4] = src[36];
	dst[cin->frameWidth * 4 + 5] = src[37];
	dst[cin->frameWidth * 4 + 6] = src[38];
	dst[cin->frameWidth * 4 + 7] = src[39];

	dst[cin->frameWidth * 5 + 0] = src[40];
	dst[cin->frameWidth * 5 + 1] = src[41];
	dst[cin->frameWidth * 5 + 2] = src[42];
	dst[cin->frameWidth * 5 + 3] = src[43];
	dst[cin->frameWidth * 5 + 4] = src[44];
	dst[cin->frameWidth * 5 + 5] = src[45];
	dst[cin->frameWidth * 5 + 6] = src[46];
	dst[cin->frameWidth * 5 + 7] = src[47];

	dst[cin->frameWidth * 6 + 0] = src[48];
	dst[cin->frameWidth * 6 + 1] = src[49];
	dst[cin->frameWidth * 6 + 2] = src[50];
	dst[cin->frameWidth * 6 + 3] = src[51];
	dst[cin->frameWidth * 6 + 4] = src[52];
	dst[cin->frameWidth * 6 + 5] = src[53];
	dst[cin->frameWidth * 6 + 6] = src[54];
	dst[cin->frameWidth * 6 + 7] = src[55];

	dst[cin->frameWidth * 7 + 0] = src[56];
	dst[cin->frameWidth * 7 + 1] = src[57];
	dst[cin->frameWidth * 7 + 2] = src[58];
	dst[cin->frameWidth * 7 + 3] = src[59];
	dst[cin->frameWidth * 7 + 4] = src[60];
	dst[cin->frameWidth * 7 + 5] = src[61];
	dst[cin->frameWidth * 7 + 6] = src[62];
	dst[cin->frameWidth * 7 + 7] = src[63];
}

/**
 * @brief CIN_MoveBlock4x4
 * @param[in] cin
 * @param[in] x
 * @param[in] y
 * @param[in] xMean
 * @param[in] yMean
 * @param[in] xyMove
 */
static void CIN_MoveBlock4x4(cinematic_t *cin, int x, int y, int xMean, int yMean, int xyMove)
{
	uint32_t *src, *dst;
	int      xMot, yMot;

	xMot = x + xMean - (xyMove >> 4);
	yMot = y + yMean - (xyMove & 15);

	src = (uint32_t *) cin->frameBuffer[1] + (yMot * cin->frameWidth + xMot);
	dst = (uint32_t *) cin->frameBuffer[0] + (y * cin->frameWidth + x);

	dst[cin->frameWidth * 0 + 0] = src[cin->frameWidth * 0 + 0];
	dst[cin->frameWidth * 0 + 1] = src[cin->frameWidth * 0 + 1];
	dst[cin->frameWidth * 0 + 2] = src[cin->frameWidth * 0 + 2];
	dst[cin->frameWidth * 0 + 3] = src[cin->frameWidth * 0 + 3];

	dst[cin->frameWidth * 1 + 0] = src[cin->frameWidth * 1 + 0];
	dst[cin->frameWidth * 1 + 1] = src[cin->frameWidth * 1 + 1];
	dst[cin->frameWidth * 1 + 2] = src[cin->frameWidth * 1 + 2];
	dst[cin->frameWidth * 1 + 3] = src[cin->frameWidth * 1 + 3];

	dst[cin->frameWidth * 2 + 0] = src[cin->frameWidth * 2 + 0];
	dst[cin->frameWidth * 2 + 1] = src[cin->frameWidth * 2 + 1];
	dst[cin->frameWidth * 2 + 2] = src[cin->frameWidth * 2 + 2];
	dst[cin->frameWidth * 2 + 3] = src[cin->frameWidth * 2 + 3];

	dst[cin->frameWidth * 3 + 0] = src[cin->frameWidth * 3 + 0];
	dst[cin->frameWidth * 3 + 1] = src[cin->frameWidth * 3 + 1];
	dst[cin->frameWidth * 3 + 2] = src[cin->frameWidth * 3 + 2];
	dst[cin->frameWidth * 3 + 3] = src[cin->frameWidth * 3 + 3];
}

/**
 * @brief CIN_MoveBlock8x8
 * @param[in] cin
 * @param[in] x
 * @param[in] y
 * @param[in] xMean
 * @param[in] yMean
 * @param[in] xyMove
 */
static void CIN_MoveBlock8x8(cinematic_t *cin, int x, int y, int xMean, int yMean, int xyMove)
{
	uint32_t *src, *dst;
	int      xMot, yMot;

	xMot = x + xMean - (xyMove >> 4);
	yMot = y + yMean - (xyMove & 15);

	src = (uint32_t *) cin->frameBuffer[1] + (yMot * cin->frameWidth + xMot);
	dst = (uint32_t *) cin->frameBuffer[0] + (y * cin->frameWidth + x);

	dst[cin->frameWidth * 0 + 0] = src[cin->frameWidth * 0 + 0];
	dst[cin->frameWidth * 0 + 1] = src[cin->frameWidth * 0 + 1];
	dst[cin->frameWidth * 0 + 2] = src[cin->frameWidth * 0 + 2];
	dst[cin->frameWidth * 0 + 3] = src[cin->frameWidth * 0 + 3];
	dst[cin->frameWidth * 0 + 4] = src[cin->frameWidth * 0 + 4];
	dst[cin->frameWidth * 0 + 5] = src[cin->frameWidth * 0 + 5];
	dst[cin->frameWidth * 0 + 6] = src[cin->frameWidth * 0 + 6];
	dst[cin->frameWidth * 0 + 7] = src[cin->frameWidth * 0 + 7];

	dst[cin->frameWidth * 1 + 0] = src[cin->frameWidth * 1 + 0];
	dst[cin->frameWidth * 1 + 1] = src[cin->frameWidth * 1 + 1];
	dst[cin->frameWidth * 1 + 2] = src[cin->frameWidth * 1 + 2];
	dst[cin->frameWidth * 1 + 3] = src[cin->frameWidth * 1 + 3];
	dst[cin->frameWidth * 1 + 4] = src[cin->frameWidth * 1 + 4];
	dst[cin->frameWidth * 1 + 5] = src[cin->frameWidth * 1 + 5];
	dst[cin->frameWidth * 1 + 6] = src[cin->frameWidth * 1 + 6];
	dst[cin->frameWidth * 1 + 7] = src[cin->frameWidth * 1 + 7];

	dst[cin->frameWidth * 2 + 0] = src[cin->frameWidth * 2 + 0];
	dst[cin->frameWidth * 2 + 1] = src[cin->frameWidth * 2 + 1];
	dst[cin->frameWidth * 2 + 2] = src[cin->frameWidth * 2 + 2];
	dst[cin->frameWidth * 2 + 3] = src[cin->frameWidth * 2 + 3];
	dst[cin->frameWidth * 2 + 4] = src[cin->frameWidth * 2 + 4];
	dst[cin->frameWidth * 2 + 5] = src[cin->frameWidth * 2 + 5];
	dst[cin->frameWidth * 2 + 6] = src[cin->frameWidth * 2 + 6];
	dst[cin->frameWidth * 2 + 7] = src[cin->frameWidth * 2 + 7];

	dst[cin->frameWidth * 3 + 0] = src[cin->frameWidth * 3 + 0];
	dst[cin->frameWidth * 3 + 1] = src[cin->frameWidth * 3 + 1];
	dst[cin->frameWidth * 3 + 2] = src[cin->frameWidth * 3 + 2];
	dst[cin->frameWidth * 3 + 3] = src[cin->frameWidth * 3 + 3];
	dst[cin->frameWidth * 3 + 4] = src[cin->frameWidth * 3 + 4];
	dst[cin->frameWidth * 3 + 5] = src[cin->frameWidth * 3 + 5];
	dst[cin->frameWidth * 3 + 6] = src[cin->frameWidth * 3 + 6];
	dst[cin->frameWidth * 3 + 7] = src[cin->frameWidth * 3 + 7];

	dst[cin->frameWidth * 4 + 0] = src[cin->frameWidth * 4 + 0];
	dst[cin->frameWidth * 4 + 1] = src[cin->frameWidth * 4 + 1];
	dst[cin->frameWidth * 4 + 2] = src[cin->frameWidth * 4 + 2];
	dst[cin->frameWidth * 4 + 3] = src[cin->frameWidth * 4 + 3];
	dst[cin->frameWidth * 4 + 4] = src[cin->frameWidth * 4 + 4];
	dst[cin->frameWidth * 4 + 5] = src[cin->frameWidth * 4 + 5];
	dst[cin->frameWidth * 4 + 6] = src[cin->frameWidth * 4 + 6];
	dst[cin->frameWidth * 4 + 7] = src[cin->frameWidth * 4 + 7];

	dst[cin->frameWidth * 5 + 0] = src[cin->frameWidth * 5 + 0];
	dst[cin->frameWidth * 5 + 1] = src[cin->frameWidth * 5 + 1];
	dst[cin->frameWidth * 5 + 2] = src[cin->frameWidth * 5 + 2];
	dst[cin->frameWidth * 5 + 3] = src[cin->frameWidth * 5 + 3];
	dst[cin->frameWidth * 5 + 4] = src[cin->frameWidth * 5 + 4];
	dst[cin->frameWidth * 5 + 5] = src[cin->frameWidth * 5 + 5];
	dst[cin->frameWidth * 5 + 6] = src[cin->frameWidth * 5 + 6];
	dst[cin->frameWidth * 5 + 7] = src[cin->frameWidth * 5 + 7];

	dst[cin->frameWidth * 6 + 0] = src[cin->frameWidth * 6 + 0];
	dst[cin->frameWidth * 6 + 1] = src[cin->frameWidth * 6 + 1];
	dst[cin->frameWidth * 6 + 2] = src[cin->frameWidth * 6 + 2];
	dst[cin->frameWidth * 6 + 3] = src[cin->frameWidth * 6 + 3];
	dst[cin->frameWidth * 6 + 4] = src[cin->frameWidth * 6 + 4];
	dst[cin->frameWidth * 6 + 5] = src[cin->frameWidth * 6 + 5];
	dst[cin->frameWidth * 6 + 6] = src[cin->frameWidth * 6 + 6];
	dst[cin->frameWidth * 6 + 7] = src[cin->frameWidth * 6 + 7];

	dst[cin->frameWidth * 7 + 0] = src[cin->frameWidth * 7 + 0];
	dst[cin->frameWidth * 7 + 1] = src[cin->frameWidth * 7 + 1];
	dst[cin->frameWidth * 7 + 2] = src[cin->frameWidth * 7 + 2];
	dst[cin->frameWidth * 7 + 3] = src[cin->frameWidth * 7 + 3];
	dst[cin->frameWidth * 7 + 4] = src[cin->frameWidth * 7 + 4];
	dst[cin->frameWidth * 7 + 5] = src[cin->frameWidth * 7 + 5];
	dst[cin->frameWidth * 7 + 6] = src[cin->frameWidth * 7 + 6];
	dst[cin->frameWidth * 7 + 7] = src[cin->frameWidth * 7 + 7];
}

/**
 * @brief CIN_DecodeQuadInfo
 * @param[in,out] cin
 * @param[in] data
 */
static void CIN_DecodeQuadInfo(cinematic_t *cin, const byte *data)
{
	if (cin->frameBuffer[0] && cin->frameBuffer[1])
	{
		return;
	}

	// Allocate the frame buffers
	cin->frameWidth  = data[0] | (data[1] << 8);
	cin->frameHeight = data[2] | (data[3] << 8);

	if ((cin->frameWidth & 15) || (cin->frameHeight & 15))
	{
		Com_Error(ERR_DROP, "CIN_DecodeQuadInfo: video dimensions not divisible by 16");
	}

	cin->frameBuffer[0] = (byte *) Com_Allocate(cin->frameWidth * cin->frameHeight * 4);
	cin->frameBuffer[1] = (byte *) Com_Allocate(cin->frameWidth * cin->frameHeight * 4);
}

/**
 * @brief CIN_DecodeQuadCodebook
 * @param cin - unused
 * @param[in,out] data
 */
static void CIN_DecodeQuadCodebook(cinematic_t *cin, const byte *data)
{
	uint32_t *quadVQ2, *quadVQ4, *quadVQ8;
	int      index0, index1, index2, index3;
	int      numQuadVecs;
	int      numQuadCels;
	int      i, r, g, b;
	byte     pixels[4][4];

	if (!chunkHeader->args)
	{
		numQuadVecs = 256;
		numQuadCels = 256;
	}
	else
	{
		numQuadVecs = (chunkHeader->args >> 8) & 255;
		numQuadCels = (chunkHeader->args >> 0) & 255;

		if (!numQuadVecs)
		{
			numQuadVecs = 256;
		}
	}

	// Convert 2x2 pixel vectors from YCbCr to RGB
	for (i = 0; i < numQuadVecs; i++)
	{
		quadVQ2 = &cin_quadVQ2[i << 2];

		r = cin_cr2rTable[data[5]];
		g = cin_cb2gTable[data[4]] + cin_cr2gTable[data[5]];
		b = cin_cb2bTable[data[4]];

		pixels[0][0] = ClampByte(r + data[0]);
		pixels[0][1] = ClampByte(g + data[0]);
		pixels[0][2] = ClampByte(b + data[0]);
		pixels[0][3] = 255;

		pixels[1][0] = ClampByte(r + data[1]);
		pixels[1][1] = ClampByte(g + data[1]);
		pixels[1][2] = ClampByte(b + data[1]);
		pixels[1][3] = 255;

		pixels[2][0] = ClampByte(r + data[2]);
		pixels[2][1] = ClampByte(g + data[2]);
		pixels[2][2] = ClampByte(b + data[2]);
		pixels[2][3] = 255;

		pixels[3][0] = ClampByte(r + data[3]);
		pixels[3][1] = ClampByte(g + data[3]);
		pixels[3][2] = ClampByte(b + data[3]);
		pixels[3][3] = 255;

		quadVQ2[0] = *(uint32_t *) (&pixels[0]);
		quadVQ2[1] = *(uint32_t *) (&pixels[1]);
		quadVQ2[2] = *(uint32_t *) (&pixels[2]);
		quadVQ2[3] = *(uint32_t *) (&pixels[3]);

		data += 6;
	}

	// Set up 4x4 and 8x8 pixel vectors
	for (i = 0; i < numQuadCels; i++)
	{
		index0 = data[0] << 2;
		index1 = data[1] << 2;
		index2 = data[2] << 2;
		index3 = data[3] << 2;

		quadVQ4 = &cin_quadVQ4[i << 4];
		quadVQ8 = &cin_quadVQ8[i << 6];

		// Set up 4x4 pixel vectors
		quadVQ4[0]  = cin_quadVQ2[index0 + 0];
		quadVQ4[1]  = cin_quadVQ2[index0 + 1];
		quadVQ4[2]  = cin_quadVQ2[index1 + 0];
		quadVQ4[3]  = cin_quadVQ2[index1 + 1];
		quadVQ4[4]  = cin_quadVQ2[index0 + 2];
		quadVQ4[5]  = cin_quadVQ2[index0 + 3];
		quadVQ4[6]  = cin_quadVQ2[index1 + 2];
		quadVQ4[7]  = cin_quadVQ2[index1 + 3];
		quadVQ4[8]  = cin_quadVQ2[index2 + 0];
		quadVQ4[9]  = cin_quadVQ2[index2 + 1];
		quadVQ4[10] = cin_quadVQ2[index3 + 0];
		quadVQ4[11] = cin_quadVQ2[index3 + 1];
		quadVQ4[12] = cin_quadVQ2[index2 + 2];
		quadVQ4[13] = cin_quadVQ2[index2 + 3];
		quadVQ4[14] = cin_quadVQ2[index3 + 2];
		quadVQ4[15] = cin_quadVQ2[index3 + 3];

		// Set up 8x8 pixel vectors
		quadVQ8[0]  = cin_quadVQ2[index0 + 0];
		quadVQ8[1]  = cin_quadVQ2[index0 + 0];
		quadVQ8[2]  = cin_quadVQ2[index0 + 1];
		quadVQ8[3]  = cin_quadVQ2[index0 + 1];
		quadVQ8[4]  = cin_quadVQ2[index1 + 0];
		quadVQ8[5]  = cin_quadVQ2[index1 + 0];
		quadVQ8[6]  = cin_quadVQ2[index1 + 1];
		quadVQ8[7]  = cin_quadVQ2[index1 + 1];
		quadVQ8[8]  = cin_quadVQ2[index0 + 0];
		quadVQ8[9]  = cin_quadVQ2[index0 + 0];
		quadVQ8[10] = cin_quadVQ2[index0 + 1];
		quadVQ8[11] = cin_quadVQ2[index0 + 1];
		quadVQ8[12] = cin_quadVQ2[index1 + 0];
		quadVQ8[13] = cin_quadVQ2[index1 + 0];
		quadVQ8[14] = cin_quadVQ2[index1 + 1];
		quadVQ8[15] = cin_quadVQ2[index1 + 1];
		quadVQ8[16] = cin_quadVQ2[index0 + 2];
		quadVQ8[17] = cin_quadVQ2[index0 + 2];
		quadVQ8[18] = cin_quadVQ2[index0 + 3];
		quadVQ8[19] = cin_quadVQ2[index0 + 3];
		quadVQ8[20] = cin_quadVQ2[index1 + 2];
		quadVQ8[21] = cin_quadVQ2[index1 + 2];
		quadVQ8[22] = cin_quadVQ2[index1 + 3];
		quadVQ8[23] = cin_quadVQ2[index1 + 3];
		quadVQ8[24] = cin_quadVQ2[index0 + 2];
		quadVQ8[25] = cin_quadVQ2[index0 + 2];
		quadVQ8[26] = cin_quadVQ2[index0 + 3];
		quadVQ8[27] = cin_quadVQ2[index0 + 3];
		quadVQ8[28] = cin_quadVQ2[index1 + 2];
		quadVQ8[29] = cin_quadVQ2[index1 + 2];
		quadVQ8[30] = cin_quadVQ2[index1 + 3];
		quadVQ8[31] = cin_quadVQ2[index1 + 3];
		quadVQ8[32] = cin_quadVQ2[index2 + 0];
		quadVQ8[33] = cin_quadVQ2[index2 + 0];
		quadVQ8[34] = cin_quadVQ2[index2 + 1];
		quadVQ8[35] = cin_quadVQ2[index2 + 1];
		quadVQ8[36] = cin_quadVQ2[index3 + 0];
		quadVQ8[37] = cin_quadVQ2[index3 + 0];
		quadVQ8[38] = cin_quadVQ2[index3 + 1];
		quadVQ8[39] = cin_quadVQ2[index3 + 1];
		quadVQ8[40] = cin_quadVQ2[index2 + 0];
		quadVQ8[41] = cin_quadVQ2[index2 + 0];
		quadVQ8[42] = cin_quadVQ2[index2 + 1];
		quadVQ8[43] = cin_quadVQ2[index2 + 1];
		quadVQ8[44] = cin_quadVQ2[index3 + 0];
		quadVQ8[45] = cin_quadVQ2[index3 + 0];
		quadVQ8[46] = cin_quadVQ2[index3 + 1];
		quadVQ8[47] = cin_quadVQ2[index3 + 1];
		quadVQ8[48] = cin_quadVQ2[index2 + 2];
		quadVQ8[49] = cin_quadVQ2[index2 + 2];
		quadVQ8[50] = cin_quadVQ2[index2 + 3];
		quadVQ8[51] = cin_quadVQ2[index2 + 3];
		quadVQ8[52] = cin_quadVQ2[index3 + 2];
		quadVQ8[53] = cin_quadVQ2[index3 + 2];
		quadVQ8[54] = cin_quadVQ2[index3 + 3];
		quadVQ8[55] = cin_quadVQ2[index3 + 3];
		quadVQ8[56] = cin_quadVQ2[index2 + 2];
		quadVQ8[57] = cin_quadVQ2[index2 + 2];
		quadVQ8[58] = cin_quadVQ2[index2 + 3];
		quadVQ8[59] = cin_quadVQ2[index2 + 3];
		quadVQ8[60] = cin_quadVQ2[index3 + 2];
		quadVQ8[61] = cin_quadVQ2[index3 + 2];
		quadVQ8[62] = cin_quadVQ2[index3 + 3];
		quadVQ8[63] = cin_quadVQ2[index3 + 3];

		data += 4;
	}
}

/**
 * @brief CIN_DecodeQuadVQ
 * @param[in,out] cin
 * @param[in,out] data
 */
static void CIN_DecodeQuadVQ(cinematic_t *cin, const byte *data)
{
	int code, codeBits, codeWord;
	int xPos, yPos, xOfs, yOfs;
	int xMean, yMean;
	int x = 0, y = 0;
	int i;

	if (!cin->frameBuffer[0] || !cin->frameBuffer[1])
	{
		Com_Error(ERR_DROP, "CIN_DecodeQuadVQ: video buffers not allocated");
	}

	codeBits = 0;
	codeWord = 0;

	xMean = 8 - (char) ((chunkHeader->args >> 8) & 255);
	yMean = 8 - (char) ((chunkHeader->args >> 0) & 255);

	while (1)
	{
		// Subdivide the 16x16 pixel macro block into 8x8 pixel blocks
		for (yPos = y; yPos < y + 16; yPos += 8)
		{
			for (xPos = x; xPos < x + 16; xPos += 8)
			{
				// Decode
				if (!codeBits)
				{
					codeBits = 16;
					codeWord = data[0] | (data[1] << 8);

					data += 2;
				}

				code = codeWord & 0xC000;

				codeBits  -= 2;
				codeWord <<= 2;

				switch (code)
				{
				case ROQ_VQ_MOT:
					// Skip over block

					break;
				case ROQ_VQ_FCC:
					// Motion compensation
					CIN_MoveBlock8x8(cin, xPos, yPos, xMean, yMean, *data);

					data += 1;

					break;
				case ROQ_VQ_SLD:
					// Vector quantization
					CIN_BlitBlock8x8(cin, xPos, yPos, *data);

					data += 1;

					break;
				case ROQ_VQ_CCC:
					// Subdivide the 8x8 pixel block into 4x4 pixel sub blocks
					for (i = 0; i < 4; i++)
					{
						xOfs = xPos + 4 * (i & 1);
						yOfs = yPos + 4 * (i >> 1);

						// Decode
						if (!codeBits)
						{
							codeBits = 16;
							codeWord = data[0] | (data[1] << 8);

							data += 2;
						}

						code = codeWord & 0xC000;

						codeBits  -= 2;
						codeWord <<= 2;

						switch (code)
						{
						case ROQ_VQ_MOT:
							// Skip over block

							break;
						case ROQ_VQ_FCC:
							// Motion compensation
							CIN_MoveBlock4x4(cin, xOfs, yOfs, xMean, yMean, *data);

							data += 1;

							break;
						case ROQ_VQ_SLD:
							// Vector quantization
							CIN_BlitBlock4x4(cin, xOfs, yOfs, *data);

							data += 1;

							break;
						case ROQ_VQ_CCC:
							// Vector quantization in 2x2 pixel sub blocks
							CIN_BlitBlock2x2(cin, xOfs + 0, yOfs + 0, data[0]);
							CIN_BlitBlock2x2(cin, xOfs + 2, yOfs + 0, data[1]);
							CIN_BlitBlock2x2(cin, xOfs + 0, yOfs + 2, data[2]);
							CIN_BlitBlock2x2(cin, xOfs + 2, yOfs + 2, data[3]);

							data += 4;

							break;
						default:
							Com_Error(ERR_DROP, "CIN_DecodeQuadVQ: bad code (%i)", code);
						}
					}

					break;
				default:
					Com_Error(ERR_DROP, "CIN_DecodeQuadVQ: bad code (%i)", code);
				}
			}
		}

		// Go to the next 16x16 pixel macro block
		x += 16;
		if (x == cin->frameWidth)
		{
			x = 0;

			y += 16;
			if (y == cin->frameHeight)
			{
				break;
			}
		}
	}

	// Bump frame count
	cin->frameCount++;

	// Swap the frame buffers
	if (cin->frameCount == 1)
	{
		Com_Memcpy(cin->frameBuffer[1], cin->frameBuffer[0], cin->frameWidth * cin->frameHeight * 4);
		return;
	}

	//swap
	{
		byte *tmpData = cin->frameBuffer[0];
		cin->frameBuffer[0] = cin->frameBuffer[1];
		cin->frameBuffer[1] = tmpData;
	}
}

/**
 * @brief CIN_DecodeSoundMono22
 * @param[in] cin
 * @param[in] data
 */
static void CIN_DecodeSoundMono22(cinematic_t *cin, const byte *data)
{
	short prev;
	int   i;

	if (cin->flags & CIN_silent)
	{
		return;
	}

	// Decode the sound samples
	prev = chunkHeader->args;

	for (i = 0; i < chunkHeader->size; i++)
	{
		prev = (short) (prev + cin_sqrTable[data[i]]);

		cin_soundSamples[i] = prev;
	}

	// Submit the sound samples
	S_RawSamples(0, chunkHeader->size, 22050, 2, 1, (byte *) cin_soundSamples, 1.0f, 1.0f);
}

/**
 * @brief CIN_DecodeSoundStereo22
 * @param[in] cin
 * @param[in] data
 */
static void CIN_DecodeSoundStereo22(cinematic_t *cin, const byte *data)
{
	short prevL, prevR;
	int   i;

	if (cin->flags & CIN_silent)
	{
		return;
	}

	// Decode the sound samples
	prevL = (short)(chunkHeader->args & 0xFF00) << 0;
	prevR = (short)(chunkHeader->args & 0x00FF) << 8;

	for (i = 0; i < chunkHeader->size; i += 2)
	{
		prevL = (short) (prevL + cin_sqrTable[data[i + 0]]);
		prevR = (short) (prevR + cin_sqrTable[data[i + 1]]);

		cin_soundSamples[i + 0] = prevL;
		cin_soundSamples[i + 1] = prevR;
	}

	// Submit the sound samples
	S_RawSamples(0, chunkHeader->size >> 1, 22050, 2, 2, (byte *) cin_soundSamples, 1.0f, 1.0f);
}

/**
 * @brief CIN_DecodeSoundMono48
 * @param[in] cin
 * @param[in] data
 */
static void CIN_DecodeSoundMono48(cinematic_t *cin, const byte *data)
{
	short samp;
	int   i, j;

	if (cin->flags & CIN_silent)
	{
		return;
	}

	// Decode the sound samples
	for (i = 0, j = 0; i < chunkHeader->size; i += 2, j++)
	{
		samp = (short) (data[i] | (data[i + 1] << 8));

		cin_soundSamples[j] = samp;
	}

	// Submit the sound samples
	S_RawSamples(0, chunkHeader->size >> 1, 48000, 2, 1, (byte *) cin_soundSamples, 1.0f, 1.0f);
}

/**
 * @brief CIN_DecodeSoundStereo48
 * @param[in] cin
 * @param[in] data
 */
static void CIN_DecodeSoundStereo48(cinematic_t *cin, const byte *data)
{
	short sampL, sampR;
	int   i, j;

	if (cin->flags & CIN_silent)
	{
		return;
	}

	// Decode the sound samples
	for (i = 0, j = 0; i < chunkHeader->size; i += 4, j += 2)
	{
		sampL = (short) (data[i + 0] | (data[i + 1] << 8));
		sampR = (short) (data[i + 2] | (data[i + 3] << 8));

		cin_soundSamples[j + 0] = sampL;
		cin_soundSamples[j + 1] = sampR;
	}

	// Submit the sound samples
	S_RawSamples(0, chunkHeader->size >> 2, 48000, 2, 2, (byte *) cin_soundSamples, 1.0f, 1.0f);
}

/**
 * @brief CIN_DecodeChunk
 * @param[in,out] cin
 * @return
 */
static qboolean CIN_DecodeChunk(cinematic_t *cin)
{
	byte *data;

	if (cin->offset >= cin->size)
	{
		return qfalse;
	}    // Finished

	data = cin_chunkData;

	// Read and decode the first chunk header if needed
	if (cin->offset == ROQ_CHUNK_HEADER_SIZE)
	{
		cin->offset += FS_Read(cin_chunkData, ROQ_CHUNK_HEADER_SIZE, cin->file);

		chunkHeader->id   = data[0] | (data[1] << 8);
		chunkHeader->size = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
		chunkHeader->args = data[6] | (data[7] << 8);
	}

	// Read the chunk data and the next chunk header
	if (chunkHeader->size > ROQ_CHUNK_MAX_DATA_SIZE)
	{
		Com_Error(ERR_DROP, "CIN_DecodeChunk: bad chunk size (%u)", chunkHeader->size);
	}

	if (cin->offset + chunkHeader->size >= cin->size)
	{
		cin->offset += FS_Read(cin_chunkData, chunkHeader->size, cin->file);
	}
	else
	{
		cin->offset += FS_Read(cin_chunkData, chunkHeader->size + ROQ_CHUNK_HEADER_SIZE, cin->file);
	}

	// Decode the chunk data
	switch (chunkHeader->id)
	{
	case ROQ_QUAD_INFO:
		CIN_DecodeQuadInfo(cin, data);
		break;
	case ROQ_QUAD_CODEBOOK:
		CIN_DecodeQuadCodebook(cin, data);
		break;
	case ROQ_QUAD_VQ:
		CIN_DecodeQuadVQ(cin, data);
		break;
	case ROQ_SOUND_MONO_22:
		CIN_DecodeSoundMono22(cin, data);
		break;
	case ROQ_SOUND_STEREO_22:
		CIN_DecodeSoundStereo22(cin, data);
		break;
	case ROQ_SOUND_MONO_48:
		CIN_DecodeSoundMono48(cin, data);
		break;
	case ROQ_SOUND_STEREO_48:
		CIN_DecodeSoundStereo48(cin, data);
		break;
	default:
		Com_Error(ERR_DROP, "CIN_DecodeChunk: bad chunk id (%u)", chunkHeader->id);
	}

	// Decode the next chunk header if needed
	if (cin->offset >= cin->size)
	{
		return qtrue;
	}

	data += chunkHeader->size;

	chunkHeader->id   = data[0] | (data[1] << 8);
	chunkHeader->size = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
	chunkHeader->args = data[6] | (data[7] << 8);

	return qtrue;
}

/**
 * @brief ROQ_UpdateCinematic
 * @param[in,out] cin
 * @param[in] time
 */
void ROQ_UpdateCinematic(cinematic_t *cin, int time)
{
	int frame;

	// If we don't have a frame yet, set the start time
	if (!cin->frameCount)
	{
		cin->startTime = time;
	}

	// Check if a new frame is needed
	frame = (time - cin->startTime) * cin->frameRate / 1000;
	if (frame < 1)
	{
		frame = 1;
	}

	if (frame <= cin->frameCount)
	{
		cin->currentData.image = cin->frameBuffer[1];
		cin->currentData.dirty = qfalse;

		cin->currentData.width  = cin->frameWidth;
		cin->currentData.height = cin->frameHeight;

		return;
	}

	// If we're dropping frames
	if (frame > cin->frameCount + 1)
	{
		if (cin->flags & CIN_system)
		{
			Com_Printf(S_COLOR_YELLOW "Dropped cinematic frame: %i > %i (%s)\n", frame, cin->frameCount + 1, cin->name);
		}
		else
		{
			Com_DPrintf(S_COLOR_YELLOW "Dropped cinematic frame: %i > %i (%s)\n", frame, cin->frameCount + 1, cin->name);
		}

		frame = cin->frameCount + 1;

		// Reset the start time
		cin->startTime = time - frame * 1000 / cin->frameRate;
	}

	// Get the desired frame
	while (frame > cin->frameCount)
	{
		// Decode a chunk
		if (CIN_DecodeChunk(cin))
		{
			continue;
		}

		// If we get here, the cinematic has finished
		if (!cin->frameCount || !(cin->flags & CIN_loop))
		{
			cin->currentData.image = NULL;
			cin->currentData.dirty = qfalse;

			cin->currentData.width  = 0;
			cin->currentData.height = 0;

			return;
		}

		// Reset the cinematic
		(void) FS_Seek(cin->file, ROQ_CHUNK_HEADER_SIZE, FS_SEEK_SET);

		cin->offset     = ROQ_CHUNK_HEADER_SIZE;
		cin->startTime  = time;
		cin->frameCount = 0;

		// Get the first frame
		frame = 1;
	}

	cin->currentData.image = cin->frameBuffer[1];
	cin->currentData.dirty = qtrue;

	cin->currentData.width  = cin->frameWidth;
	cin->currentData.height = cin->frameHeight;

	return;
}

/**
 * @brief ROQ_Reset
 * @param[in,out] cin
 */
void ROQ_Reset(cinematic_t *cin)
{
	(void) FS_Seek(cin->file, ROQ_CHUNK_HEADER_SIZE, FS_SEEK_SET);
	cin->offset = ROQ_CHUNK_HEADER_SIZE;
}

/**
 * @brief ROQ_StartRead
 * @param[in,out] cin
 * @return
 */
qboolean ROQ_StartRead(cinematic_t *cin)
{
	uint16_t id, fps;

	// Read the file header
	FS_Read(cin_chunkData, ROQ_CHUNK_HEADER_SIZE, cin->file);

	id  = cin_chunkData[0] | (cin_chunkData[1] << 8);
	fps = cin_chunkData[6] | (cin_chunkData[7] << 8);

	if (id != ROQ_ID)
	{
		return qfalse;
	}

	cin->offset    = ROQ_CHUNK_HEADER_SIZE;
	cin->frameRate = (fps) ? fps : 30;

	if (cin->data)
	{
		Com_Dealloc(cin->data);
		cin->data = NULL;
	}

	cin->data = Com_Allocate(sizeof(roqChunkHeader_t));
	Com_Memset(cin->data, 0, sizeof(roqChunkHeader_t));

	return qtrue;
}

/**
 * @brief ROQ_StopVideo
 * @param[in,out] cin
 */
void ROQ_StopVideo(cinematic_t *cin)
{
	if (cin->data)
	{
		Com_Dealloc(cin->data);
		cin->data = NULL;
	}
}

/**
 * @brief ROQ_Init
 */
void ROQ_Init(void)
{
	float   f;
	int16_t s;
	int     i;

	// Build YCbCr-to-RGB tables
	for (i = 0; i < 256; i++)
	{
		f = (float) (i - 128);

		cin_cr2rTable[i] = (int16_t) (f * 1.40200f);
		cin_cb2gTable[i] = (int16_t) (f * -0.34414f);
		cin_cr2gTable[i] = (int16_t) (f * -0.71414f);
		cin_cb2bTable[i] = (int16_t) (f * 1.77200f);
	}

	// Build square table
	for (i = 0; i < 128; i++)
	{
		s = (int16_t) Square(i);

		cin_sqrTable[i]       = s;
		cin_sqrTable[i + 128] = -s;
	}
}
