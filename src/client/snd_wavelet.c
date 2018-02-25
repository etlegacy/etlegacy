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
 * @file snd_wavelet.c
 */

#include "snd_local.h"

#define C0 0.4829629131445341
#define C1 0.8365163037378079
#define C2 0.2241438680420134
#define C3 -0.1294095225512604

/**
 * @brief daub4
 * @param b - unused
 * @param[in] n
 * @param[in] isign
 */
void daub4(float b[], unsigned long n, int isign)
{
	float wksp[4097];
#define a(x) b[(x) - 1]                  // numerical recipies so a[1] = b[0]
	unsigned long nh, nh1, i, j;

	if (n < 4)
	{
		return;
	}

	nh1 = (nh = n >> 1) + 1;
	if (isign >= 0)
	{
		for (i = 1, j = 1; j <= n - 3; j += 2, i++)
		{
			wksp[i]      = C0 * a(j) + C1 * a(j + 1) + C2 * a(j + 2) + C3 * a(j + 3);
			wksp[i + nh] = C3 * a(j) - C2 * a(j + 1) + C1 * a(j + 2) - C0 * a(j + 3);
		}
		wksp[i]      = C0 * a(n - 1) + C1 * a(n) + C2 * a(1) + C3 * a(2);
		wksp[i + nh] = C3 * a(n - 1) - C2 * a(n) + C1 * a(1) - C0 * a(2);
	}
	else
	{
		wksp[1] = C2 * a(nh) + C1 * a(n) + C0 * a(1) + C3 * a(nh1);
		wksp[2] = C3 * a(nh) - C0 * a(n) + C1 * a(1) - C2 * a(nh1);
		for (i = 1, j = 3; i < nh; i++)
		{
			wksp[j++] = C2 * a(i) + C1 * a(i + nh) + C0 * a(i + 1) + C3 * a(i + nh1);
			wksp[j++] = C3 * a(i) - C0 * a(i + nh) + C1 * a(i + 1) - C2 * a(i + nh1);
		}
	}
	for (i = 1; i <= n; i++)
	{
		a(i) = wksp[i];
	}
#undef a
}

/**
 * @brief wt1
 * @param[in] a
 * @param[in] n
 * @param[in] isign
 */
void wt1(float a[], unsigned long n, int isign)
{
	unsigned long nn;
	int           inverseStartLength = n / 4;

	if (n < inverseStartLength)
	{
		return;
	}
	if (isign >= 0)
	{
		for (nn = n; nn >= inverseStartLength; nn >>= 1)
			daub4(a, nn, isign);
	}
	else
	{
		for (nn = inverseStartLength; nn <= n; nn <<= 1)
			daub4(a, nn, isign);
	}
}

/* The number of bits required by each value */
static unsigned char numBits[] =
{
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

/**
 * @brief MuLawEncode
 * @param[in] s
 * @return
 */
byte MuLawEncode(short s)
{
	unsigned long adjusted;
	byte          exponent, mantissa;
	byte          sign = (s < 0) ? 0 : 0x80;

	if (s < 0)
	{
		s = -s;
	}
	adjusted  = (long)s << (16 - sizeof(short) * 8);
	adjusted += 128L + 4L;
	if (adjusted > 32767)
	{
		adjusted = 32767;
	}
	exponent = numBits[(adjusted >> 7) & 0xff] - 1;
	mantissa = (adjusted >> (exponent + 3)) & 0xf;
	return ~(sign | (exponent << 4) | mantissa);
}

short mulawToShort[256] =
{
	-32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
	-23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
	-15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
	-11900, -11388, -10876, -10364, -9852,  -9340,  -8828,  -8316,
	-7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
	-5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
	-3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
	-2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
	-1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
	-1372,  -1308,  -1244,  -1180,  -1116,  -1052,  -988,   -924,
	-876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
	-620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
	-372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
	-244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
	-120,   -112,   -104,   -96,    -88,    -80,    -72,    -64,
	-56,    -48,    -40,    -32,    -24,    -16,    -8,     -1,
	32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
	23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
	15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
	11900,  11388,  10876,  10364,  9852,   9340,   8828,   8316,
	7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
	5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
	3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
	2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
	1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
	1372,   1308,   1244,   1180,   1116,   1052,   988,    924,
	876,    844,    812,    780,    748,    716,    684,    652,
	620,    588,    556,    524,    492,    460,    428,    396,
	372,    356,    340,    324,    308,    292,    276,    260,
	244,    228,    212,    196,    180,    164,    148,    132,
	120,    112,    104,    96,     88,     80,     72,     64,
	56,     48,     40,     32,     24,     16,     8,      0
};

/**
 * @brief encodeWavelet
 * @param[out] sfx
 * @param[in,out] packets
 */
void encodeWavelet(sfx_t *sfx, short *packets)
{
	float     wksp[4097], temp;
	int       i, samples = sfx->soundLength, size;
	sndBuffer *newchunk, *chunk = NULL;
	byte      *out;

	while (samples > 0)
	{
		size = samples;
		if (size > (SND_CHUNK_SIZE * 2))
		{
			size = (SND_CHUNK_SIZE * 2);
		}

		if (size < 4)
		{
			size = 4;
		}

		newchunk = SND_malloc();
		if (sfx->soundData == NULL)
		{
			sfx->soundData = newchunk;
		}
		else if(chunk)
		{
			chunk->next = newchunk;
		}
		chunk = newchunk;
		for (i = 0; i < size; i++)
		{
			wksp[i] = *packets;
			packets++;
		}
		wt1(wksp, size, 1);
		out = (byte *)chunk->sndChunk;

		for (i = 0; i < size; i++)
		{
			temp = wksp[i];
			if (temp > 32767)
			{
				temp = 32767;
			}
			else if (temp < -32768)
			{
				temp = -32768;
			}
			out[i] = MuLawEncode((short)temp);
		}

		chunk->size = size;
		samples    -= size;
	}
}

/**
 * @brief decodeWavelet
 * @param[in] chunk
 * @param[out] packets
 */
void decodeWavelet(sndBuffer *chunk, short *packets)
{
	float wksp[4097];
	int   i;
	int   size = chunk->size;
	byte  *out = (byte *)chunk->sndChunk;

	for (i = 0; i < size; i++)
	{
		wksp[i] = mulawToShort[out[i]];
	}

	wt1(wksp, size, -1);

	if (!packets)
	{
		return;
	}

	for (i = 0; i < size; i++)
	{
		packets[i] = wksp[i];
	}
}

/**
 * @brief encodeMuLaw
 * @param[out] sfx
 * @param[in,out] packets
 */
void encodeMuLaw(sfx_t *sfx, short *packets)
{
	int       i, samples = sfx->soundLength, size, grade = 0, poop;
	sndBuffer *newchunk, *chunk = NULL;
	byte      *out;

	while (samples > 0)
	{
		size = samples;
		if (size > (SND_CHUNK_SIZE * 2))
		{
			size = (SND_CHUNK_SIZE * 2);
		}

		newchunk = SND_malloc();
		if (sfx->soundData == NULL)
		{
			sfx->soundData = newchunk;
		}
		else if(chunk)
		{
			chunk->next = newchunk;
		}
		chunk = newchunk;
		out   = (byte *)chunk->sndChunk;
		for (i = 0; i < size; i++)
		{
			poop = packets[0] + grade;
			if (poop > 32767)
			{
				poop = 32767;
			}
			else if (poop < -32768)
			{
				poop = -32768;
			}
			out[i] = MuLawEncode((short)poop);
			grade  = poop - mulawToShort[out[i]];
			packets++;
		}
		chunk->size = size;
		samples    -= size;
	}
}
