/*
 * Copyright (C) 2022 Gian 'myT' Schellenbaum.
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
 */
/**
 * @file bg_b64.c
 */

#include "../qcommon/q_shared.h"
#include "bg_b64.h"

void BS_Init(bitStream_t *bs, unsigned char *buffer, int bufferSize)
{
	bs->stream = buffer;
	bs->bytes  = bufferSize;
	bs->bit    = 0;

	Com_Memset(buffer, 0, bufferSize);
}

void BS_Write(bitStream_t *bs, int value, int bits)
{
	int i;

	if ((bs->bit + bits + 7) / 8 > bs->bytes)
	{
		return;
	}

	for (i = 0; i < bits; ++i)
	{
		const int bitIdx  = bs->bit & 7;
		const int byteIdx = bs->bit >> 3;
		const int bit     = value & 1;

		bs->stream[byteIdx] |= (unsigned char)(bit << bitIdx);
		++bs->bit;
		value >>= 1;
	}
}

int BS_Read(bitStream_t *bs, int bits)
{
	int i;
	int value = 0;

	if ((bs->bit + bits + 7) / 8 > bs->bytes)
	{
		return 0;
	}

	for (i = 0; i < bits; ++i)
	{
		const int bitIdx  = bs->bit & 7;
		const int byteIdx = bs->bit >> 3;

		value |= ((int)(bs->stream[byteIdx] >> bitIdx) & 1) << i;
		++bs->bit;
	}

	return value;
}

// Space (' ', 5 bits), double quote ('"', 9 bits) and forward slash ('/', 8 bits) can NOT be used
// because of how commands get tokenized by the engine ('/' can be the start of a comment).
// Assuming equal symbol distributions for this base64 table, we would need 557/64 ~ 8.7 bits per symbol.
// If we had picked a more optimal table, we would have needed 533/64 ~ 8.3 bits per symbol.
// Because the improvement is so tiny, we opt for the safest option, guarding against dumb engine behavior.
// The C string for the more optimal base64 table was "(.0123456789:=>@ABCDEFGHLPYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~".
static const char          b64e[64 + 1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
static const unsigned char b64d[256]    =
{
	66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 64, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
	66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 62, 66, 63, 66, 66, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 66, 66, 66, 65, 66, 66,
	66, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 66, 66, 66, 66, 66,
	66, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 66, 66, 66, 66, 66,
	66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
	66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
	66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
	66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66
};

void B64_Encode(char *out, int maxOutChars, const unsigned char *in, int inBits)
{
	const int blocksToWrite = (inBits + 23) / 24;
	const int charsToWrite  = blocksToWrite * 4;
	int       charsThatMatter;
	char      *nullTerm;
	int       i;

	if (charsToWrite >= maxOutChars)   // leaves space for the null-terminator
	{
		*out = '\0';
		return;
	}

	charsThatMatter = (inBits + 5) / 6;
	nullTerm        = out + charsThatMatter;

	for (i = 0; i < blocksToWrite; ++i)
	{
		// the 4 output bytes are:
		// bottom 6 of 0
		// top 2 of 0 and bottom 4 of 1
		// top 4 of 1 and bottom 2 of 2
		// top 6 of 2
		const int in0 = (int)in[0];
		const int in1 = (int)in[1];
		const int in2 = (int)in[2];

		out[0] = b64e[in0 & 63];
		out[1] = b64e[((in1 & 15) << 2) | (in0 >> 6)];
		out[2] = b64e[((in2 &  3) << 4) | (in1 >> 4)];
		out[3] = b64e[in2 >> 2];
		in    += 3;
		out   += 4;
	}

	*nullTerm = '\0';
}


#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wchar-subscripts"
#endif
void B64_Decode(unsigned char *out, int maxOutBytes, const char *in, int inBitsReq)
{
	const int  inBytes      = (int)strlen(in);
	const char *end         = in + inBytes;
	const int  inBits       = inBitsReq <= 0 ? (inBytes * 6) : inBitsReq;
	const int  blocksToRead = (inBits + 23) / 24;
	const int  charsToWrite = blocksToRead * 3;
	int        i;

	if (charsToWrite > maxOutBytes)
	{
		return;
	}

	for (i = 0; i < blocksToRead; ++i)
	{
		const int in0  = (in + 0) >= end ? 0 : (int)b64d[(unsigned char)in[0]];
		const int in1  = (in + 1) >= end ? 0 : (int)b64d[(unsigned char)in[1]];
		const int in2  = (in + 2) >= end ? 0 : (int)b64d[(unsigned char)in[2]];
		const int in3  = (in + 3) >= end ? 0 : (int)b64d[(unsigned char)in[3]];
		const int bits = (in3 << 18) | (in2 << 12) | (in1 << 6) | in0;

		etl_assert((in0 | in1 | in2 | in3) < 64); // input sanity check
		out[0] = (unsigned char)(bits);
		out[1] = (unsigned char)(bits >>  8);
		out[2] = (unsigned char)(bits >> 16);
		in    += 4;
		out   += 3;
	}
}

void B64_DecodeBigEndian(unsigned char *out, int maxOutBytes, const char *in, int inBitsReq)
{
	const int  inBytes      = (int)strlen(in);
	const char *end         = in + inBytes;
	const int  inBits       = inBitsReq <= 0 ? (inBytes * 6) : inBitsReq;
	const int  blocksToRead = (inBits + 23) / 24;
	const int  charsToWrite = blocksToRead * 3;
	int        i;

	if (charsToWrite > maxOutBytes)
	{
		return;
	}

	for (i = 0; i < blocksToRead; ++i)
	{
		const int in0  = (in + 0) >= end ? 0 : (int)b64d[(unsigned char)in[0]];
		const int in1  = (in + 1) >= end ? 0 : (int)b64d[(unsigned char)in[1]];
		const int in2  = (in + 2) >= end ? 0 : (int)b64d[(unsigned char)in[2]];
		const int in3  = (in + 3) >= end ? 0 : (int)b64d[(unsigned char)in[3]];
		const int bits = (in0 << 18) | (in1 << 12) | (in2 << 6) | in3;

		etl_assert((in0 | in1 | in2 | in3) < 64); // input sanity check
		out[0] = (unsigned char)(bits >> 16);
		out[1] = (unsigned char)(bits >> 8);
		out[2] = (unsigned char)(bits);
		in    += 4;
		out   += 3;
	}
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

char B64_Char(int offset)
{
	return b64e[offset & 63];
}

int B64_Offset(char c)
{
	return b64d[(int)c];
}
