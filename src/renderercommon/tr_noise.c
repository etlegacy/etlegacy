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
 * @file renderercommon/tr_noise.c
 */

#include "tr_common.h"

#define NOISE_SIZE 256
#define NOISE_MASK (NOISE_SIZE - 1)

#define VAL(a) s_noise_perm[(a) & (NOISE_MASK)]
#define INDEX(x, y, z, t) VAL(x + VAL(y + VAL(z + VAL(t))))

static double s_noise_table[NOISE_SIZE];
static int   s_noise_perm[NOISE_SIZE];

/**
 * @brief GetNoiseValue
 * @param[in] x
 * @param[in] y
 * @param[in] z
 * @param[in] t
 * @return
 */
static double GetNoiseValue(int x, int y, int z, int t)
{
	int index = INDEX((int)x, (int)y, (int)z, (int)t);

	return s_noise_table[index];
}

/**
 * @brief R_NoiseInit
 */
void R_NoiseInit(void)
{
	int i;

	srand(1001);

	for (i = 0; i < NOISE_SIZE; i++)
	{
		s_noise_table[i] = (rand() / (double)RAND_MAX) * 2.0 - 1.0;
		s_noise_perm[i]  = (unsigned char)(rand() / (double)RAND_MAX * 255);
	}
}

/**
 * @brief R_NoiseGet4f
 * @param[in] x
 * @param[in] y
 * @param[in] z
 * @param[in] t
 * @return
 */
double R_NoiseGet4f(double x, double y, double z, double t)
{
	int    i;
	int    ix, iy, iz, it;
	double fx, fy, fz, ft;
	double front[4];
	double back[4];
	double fvalue, bvalue, value[2], finalvalue;

	ix = (int)(floor(x));
	fx = x - ix;
	iy = (int)(floor(y));
	fy = y - iy;
	iz = (int)(floor(z));
	fz = z - iz;
	it = (int)(floor(t));
	ft = t - it;

	for (i = 0; i < 2; i++)
	{
		front[0] = GetNoiseValue(ix, iy, iz, it + i);
		front[1] = GetNoiseValue(ix + 1, iy, iz, it + i);
		front[2] = GetNoiseValue(ix, iy + 1, iz, it + i);
		front[3] = GetNoiseValue(ix + 1, iy + 1, iz, it + i);

		back[0] = GetNoiseValue(ix, iy, iz + 1, it + i);
		back[1] = GetNoiseValue(ix + 1, iy, iz + 1, it + i);
		back[2] = GetNoiseValue(ix, iy + 1, iz + 1, it + i);
		back[3] = GetNoiseValue(ix + 1, iy + 1, iz + 1, it + i);

		fvalue = LERP(LERP(front[0], front[1], fx), LERP(front[2], front[3], fx), fy);
		bvalue = LERP(LERP(back[0], back[1], fx), LERP(back[2], back[3], fx), fy);

		value[i] = LERP(fvalue, bvalue, fz);
	}

	finalvalue = LERP(value[0], value[1], ft);

	return finalvalue;
}
