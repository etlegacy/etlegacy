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
 * @file q_math.c
 * @brief Stateless support routines that are included in each code module
 */

#include "q_shared.h"

#pragma warning(disable:4700)

vec3_t vec3_origin = { 0.f, 0.f, 0.f };
vec3_t vec3_111 = { 1.f, 1.f, 1.f };
vec3_t axisDefault[3] = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f } };

vec4_t colorBlack = { 0, 0, 0, 1 };
vec4_t colorRed = { 1, 0, 0, 1 };
vec4_t colorGreen = { 0, 1, 0, 1 };
vec4_t colorBlue = { 0, 0, 1, 1 };
vec4_t colorYellow = { 1, 1, 0, 1 };
vec4_t colorOrange = { 1, 0.5, 0, 1 };
vec4_t colorMagenta = { 1, 0, 1, 1 };
vec4_t colorCyan = { 0, 1, 1, 1 };
vec4_t colorWhite = { 1, 1, 1, 1 };
vec4_t colorLtGrey = { 0.75, 0.75, 0.75, 1 };
vec4_t colorMdGrey = { 0.5, 0.5, 0.5, 1 };
vec4_t colorDkGrey = { 0.25, 0.25, 0.25, 1 };
vec4_t colorMdRed = { 0.5, 0, 0, 1 };
vec4_t colorMdGreen = { 0, 0.5, 0, 1 };
vec4_t colorDkGreen = { 0, 0.20f, 0, 1 };
vec4_t colorMdCyan = { 0, 0.5, 0.5, 1 };
vec4_t colorMdYellow = { 0.5, 0.5, 0, 1 };
vec4_t colorMdOrange = { 0.5, 0.25, 0, 1 };
vec4_t colorMdBlue = { 0, 0, 0.5, 1 };

vec4_t clrBrown = { 0.68f, 0.68f, 0.56f, 1.f };
vec4_t clrBrownDk = { 0.58f * 0.75f, 0.58f * 0.75f, 0.46f * 0.75f, 1.f };
vec4_t clrBrownLine = { 0.0525f, 0.05f, 0.025f, 0.2f };
vec4_t clrBrownLineFull = { 0.0525f, 0.05f, 0.025f, 1.f };

vec4_t clrBrownTextLt2 = { 108 * 1.8f / 255.f, 88 * 1.8f / 255.f, 62 * 1.8f / 255.f, 1.f };
vec4_t clrBrownTextLt = { 108 * 1.3f / 255.f, 88 * 1.3f / 255.f, 62 * 1.3f / 255.f, 1.f };
vec4_t clrBrownText = { 108 / 255.f, 88 / 255.f, 62 / 255.f, 1.f };
vec4_t clrBrownTextDk = { 20 / 255.f, 2 / 255.f, 0 / 255.f, 1.f };
vec4_t clrBrownTextDk2 = { 108 * 0.75f / 255.f, 88 * 0.75f / 255.f, 62 * 0.75f / 255.f, 1.f };

vec4_t g_color_table[32] =
{
	{ 0.0,  0.0,  0.0,  1.0 },          // 0 - black        0
	{ 1.0,  0.0,  0.0,  1.0 },          // 1 - red          1
	{ 0.0,  1.0,  0.0,  1.0 },          // 2 - green        2
	{ 1.0,  1.0,  0.0,  1.0 },          // 3 - yellow       3
	{ 0.0,  0.0,  1.0,  1.0 },          // 4 - blue         4
	{ 0.0,  1.0,  1.0,  1.0 },          // 5 - cyan         5
	{ 1.0,  0.0,  1.0,  1.0 },          // 6 - purple       6
	{ 1.0,  1.0,  1.0,  1.0 },          // 7 - white        7
	{ 1.0,  0.5,  0.0,  1.0 },          // 8 - orange       8
	{ 0.5,  0.5,  0.5,  1.0 },          // 9 - md.grey      9
	{ 0.75, 0.75, 0.75, 1.0 },          // : - lt.grey      10      // lt grey for names
	{ 0.75, 0.75, 0.75, 1.0 },          // ; - lt.grey      11
	{ 0.0,  0.5,  0.0,  1.0 },          // < - md.green     12
	{ 0.5,  0.5,  0.0,  1.0 },          // = - md.yellow    13
	{ 0.0,  0.0,  0.5,  1.0 },          // > - md.blue      14
	{ 0.5,  0.0,  0.0,  1.0 },          // ? - md.red       15
	{ 0.5,  0.25, 0.0,  1.0 },          // @ - md.orange    16
	{ 1.0,  0.6f, 0.1f, 1.0 },          // A - lt.orange    17
	{ 0.0,  0.5,  0.5,  1.0 },          // B - md.cyan      18
	{ 0.5,  0.0,  0.5,  1.0 },          // C - md.purple    19
	{ 0.0,  0.5,  1.0,  1.0 },          // D                20
	{ 0.5,  0.0,  1.0,  1.0 },          // E                21
	{ 0.2f, 0.6f, 0.8f, 1.0 },          // F                22
	{ 0.8f, 1.0,  0.8f, 1.0 },          // G                23
	{ 0.0,  0.4f, 0.2f, 1.0 },          // H                24
	{ 1.0,  0.0,  0.2f, 1.0 },          // I                25
	{ 0.7f, 0.1f, 0.1f, 1.0 },          // J                26
	{ 0.6f, 0.2f, 0.0,  1.0 },          // K                27
	{ 0.8f, 0.6f, 0.2f, 1.0 },          // L                28
	{ 0.6f, 0.6f, 0.2f, 1.0 },          // M                29
	{ 1.0,  1.0,  0.75, 1.0 },          // N                30
	{ 1.0,  1.0,  0.5,  1.0 },          // O                31
};

// This whole bytedirs array can go. (see comments on ByteToDir() & DirToByte())
vec3_t bytedirs[NUMVERTEXNORMALS] =
{
	{ -0.525731f, 0.000000f,  0.850651f  }, { -0.442863f, 0.238856f,  0.864188f  },
	{ -0.295242f, 0.000000f,  0.955423f  }, { -0.309017f, 0.500000f,  0.809017f  },
	{ -0.162460f, 0.262866f,  0.951056f  }, { 0.000000f,  0.000000f,  1.000000f  },
	{ 0.000000f,  0.850651f,  0.525731f  }, { -0.147621f, 0.716567f,  0.681718f  },
	{ 0.147621f,  0.716567f,  0.681718f  }, { 0.000000f,  0.525731f,  0.850651f  },
	{ 0.309017f,  0.500000f,  0.809017f  }, { 0.525731f,  0.000000f,  0.850651f  },
	{ 0.295242f,  0.000000f,  0.955423f  }, { 0.442863f,  0.238856f,  0.864188f  },
	{ 0.162460f,  0.262866f,  0.951056f  }, { -0.681718f, 0.147621f,  0.716567f  },
	{ -0.809017f, 0.309017f,  0.500000f  }, { -0.587785f, 0.425325f,  0.688191f  },
	{ -0.850651f, 0.525731f,  0.000000f  }, { -0.864188f, 0.442863f,  0.238856f  },
	{ -0.716567f, 0.681718f,  0.147621f  }, { -0.688191f, 0.587785f,  0.425325f  },
	{ -0.500000f, 0.809017f,  0.309017f  }, { -0.238856f, 0.864188f,  0.442863f  },
	{ -0.425325f, 0.688191f,  0.587785f  }, { -0.716567f, 0.681718f,  -0.147621f },
	{ -0.500000f, 0.809017f,  -0.309017f }, { -0.525731f, 0.850651f,  0.000000f  },
	{ 0.000000f,  0.850651f,  -0.525731f }, { -0.238856f, 0.864188f,  -0.442863f },
	{ 0.000000f,  0.955423f,  -0.295242f }, { -0.262866f, 0.951056f,  -0.162460f },
	{ 0.000000f,  1.000000f,  0.000000f  }, { 0.000000f,  0.955423f,  0.295242f  },
	{ -0.262866f, 0.951056f,  0.162460f  }, { 0.238856f,  0.864188f,  0.442863f  },
	{ 0.262866f,  0.951056f,  0.162460f  }, { 0.500000f,  0.809017f,  0.309017f  },
	{ 0.238856f,  0.864188f,  -0.442863f }, { 0.262866f,  0.951056f,  -0.162460f },
	{ 0.500000f,  0.809017f,  -0.309017f }, { 0.850651f,  0.525731f,  0.000000f  },
	{ 0.716567f,  0.681718f,  0.147621f  }, { 0.716567f,  0.681718f,  -0.147621f },
	{ 0.525731f,  0.850651f,  0.000000f  }, { 0.425325f,  0.688191f,  0.587785f  },
	{ 0.864188f,  0.442863f,  0.238856f  }, { 0.688191f,  0.587785f,  0.425325f  },
	{ 0.809017f,  0.309017f,  0.500000f  }, { 0.681718f,  0.147621f,  0.716567f  },
	{ 0.587785f,  0.425325f,  0.688191f  }, { 0.955423f,  0.295242f,  0.000000f  },
	{ 1.000000f,  0.000000f,  0.000000f  }, { 0.951056f,  0.162460f,  0.262866f  },
	{ 0.850651f,  -0.525731f, 0.000000f  }, { 0.955423f,  -0.295242f, 0.000000f  },
	{ 0.864188f,  -0.442863f, 0.238856f  }, { 0.951056f,  -0.162460f, 0.262866f  },
	{ 0.809017f,  -0.309017f, 0.500000f  }, { 0.681718f,  -0.147621f, 0.716567f  },
	{ 0.850651f,  0.000000f,  0.525731f  }, { 0.864188f,  0.442863f,  -0.238856f },
	{ 0.809017f,  0.309017f,  -0.500000f }, { 0.951056f,  0.162460f,  -0.262866f },
	{ 0.525731f,  0.000000f,  -0.850651f }, { 0.681718f,  0.147621f,  -0.716567f },
	{ 0.681718f,  -0.147621f, -0.716567f }, { 0.850651f,  0.000000f,  -0.525731f },
	{ 0.809017f,  -0.309017f, -0.500000f }, { 0.864188f,  -0.442863f, -0.238856f },
	{ 0.951056f,  -0.162460f, -0.262866f }, { 0.147621f,  0.716567f,  -0.681718f },
	{ 0.309017f,  0.500000f,  -0.809017f }, { 0.425325f,  0.688191f,  -0.587785f },
	{ 0.442863f,  0.238856f,  -0.864188f }, { 0.587785f,  0.425325f,  -0.688191f },
	{ 0.688191f,  0.587785f,  -0.425325f }, { -0.147621f, 0.716567f,  -0.681718f },
	{ -0.309017f, 0.500000f,  -0.809017f }, { 0.000000f,  0.525731f,  -0.850651f },
	{ -0.525731f, 0.000000f,  -0.850651f }, { -0.442863f, 0.238856f,  -0.864188f },
	{ -0.295242f, 0.000000f,  -0.955423f }, { -0.162460f, 0.262866f,  -0.951056f },
	{ 0.000000f,  0.000000f,  -1.000000f }, { 0.295242f,  0.000000f,  -0.955423f },
	{ 0.162460f,  0.262866f,  -0.951056f }, { -0.442863f, -0.238856f, -0.864188f },
	{ -0.309017f, -0.500000f, -0.809017f }, { -0.162460f, -0.262866f, -0.951056f },
	{ 0.000000f,  -0.850651f, -0.525731f }, { -0.147621f, -0.716567f, -0.681718f },
	{ 0.147621f,  -0.716567f, -0.681718f }, { 0.000000f,  -0.525731f, -0.850651f },
	{ 0.309017f,  -0.500000f, -0.809017f }, { 0.442863f,  -0.238856f, -0.864188f },
	{ 0.162460f,  -0.262866f, -0.951056f }, { 0.238856f,  -0.864188f, -0.442863f },
	{ 0.500000f,  -0.809017f, -0.309017f }, { 0.425325f,  -0.688191f, -0.587785f },
	{ 0.716567f,  -0.681718f, -0.147621f }, { 0.688191f,  -0.587785f, -0.425325f },
	{ 0.587785f,  -0.425325f, -0.688191f }, { 0.000000f,  -0.955423f, -0.295242f },
	{ 0.000000f,  -1.000000f, 0.000000f  }, { 0.262866f,  -0.951056f, -0.162460f },
	{ 0.000000f,  -0.850651f, 0.525731f  }, { 0.000000f,  -0.955423f, 0.295242f  },
	{ 0.238856f,  -0.864188f, 0.442863f  }, { 0.262866f,  -0.951056f, 0.162460f  },
	{ 0.500000f,  -0.809017f, 0.309017f  }, { 0.716567f,  -0.681718f, 0.147621f  },
	{ 0.525731f,  -0.850651f, 0.000000f  }, { -0.238856f, -0.864188f, -0.442863f },
	{ -0.500000f, -0.809017f, -0.309017f }, { -0.262866f, -0.951056f, -0.162460f },
	{ -0.850651f, -0.525731f, 0.000000f  }, { -0.716567f, -0.681718f, -0.147621f },
	{ -0.716567f, -0.681718f, 0.147621f  }, { -0.525731f, -0.850651f, 0.000000f  },
	{ -0.500000f, -0.809017f, 0.309017f  }, { -0.238856f, -0.864188f, 0.442863f  },
	{ -0.262866f, -0.951056f, 0.162460f  }, { -0.864188f, -0.442863f, 0.238856f  },
	{ -0.809017f, -0.309017f, 0.500000f  }, { -0.688191f, -0.587785f, 0.425325f  },
	{ -0.681718f, -0.147621f, 0.716567f  }, { -0.442863f, -0.238856f, 0.864188f  },
	{ -0.587785f, -0.425325f, 0.688191f  }, { -0.309017f, -0.500000f, 0.809017f  },
	{ -0.147621f, -0.716567f, 0.681718f  }, { -0.425325f, -0.688191f, 0.587785f  },
	{ -0.162460f, -0.262866f, 0.951056f  }, { 0.442863f,  -0.238856f, 0.864188f  },
	{ 0.162460f,  -0.262866f, 0.951056f  }, { 0.309017f,  -0.500000f, 0.809017f  },
	{ 0.147621f,  -0.716567f, 0.681718f  }, { 0.000000f,  -0.525731f, 0.850651f  },
	{ 0.425325f,  -0.688191f, 0.587785f  }, { 0.587785f,  -0.425325f, 0.688191f  },
	{ 0.688191f,  -0.587785f, 0.425325f  }, { -0.955423f, 0.295242f,  0.000000f  },
	{ -0.951056f, 0.162460f,  0.262866f  }, { -1.000000f, 0.000000f,  0.000000f  },
	{ -0.850651f, 0.000000f,  0.525731f  }, { -0.955423f, -0.295242f, 0.000000f  },
	{ -0.951056f, -0.162460f, 0.262866f  }, { -0.864188f, 0.442863f,  -0.238856f },
	{ -0.951056f, 0.162460f,  -0.262866f }, { -0.809017f, 0.309017f,  -0.500000f },
	{ -0.864188f, -0.442863f, -0.238856f }, { -0.951056f, -0.162460f, -0.262866f },
	{ -0.809017f, -0.309017f, -0.500000f }, { -0.681718f, 0.147621f,  -0.716567f },
	{ -0.681718f, -0.147621f, -0.716567f }, { -0.850651f, 0.000000f,  -0.525731f },
	{ -0.688191f, 0.587785f,  -0.425325f }, { -0.587785f, 0.425325f,  -0.688191f },
	{ -0.425325f, 0.688191f,  -0.587785f }, { -0.425325f, -0.688191f, -0.587785f },
	{ -0.587785f, -0.425325f, -0.688191f }, { -0.688191f, -0.587785f, -0.425325f }
};

//==============================================================

/**
 * @brief Q_rand
 * @param[in,out] seed
 * @return
 */
int Q_rand(int *seed)
{
	*seed = (69069U * *seed + 1U);
	return *seed;
}

/**
 * @brief Q_random
 * @param[in] seed
 * @return
 */
float Q_random(int *seed)
{
	return (Q_rand(seed) & 0xffff) / (float)0x10000;
}

/**
 * @brief Q_crandom
 * @param[in] seed
 * @return
 */
float Q_crandom(int *seed)
{
	return (Q_random(seed) - 0.5f) * 2.0f;
}


//=======================================================

/**
 * @brief ClampChar
 * @param[in] i
 * @return
 */
signed char ClampChar(int i)
{
	if (i < -128)
	{
		return -128;
	}
	else if (i > 127)
	{
		return 127;
	}
	return i;
}

/*
 * @brief ClampShort
 * @param i
 * @return
 * @note Unused.
signed short ClampShort(int i)
{
    if (i < -32768)
    {
        return -32768;
    }
    if (i > 0x7fff)
    {
        return 0x7fff;
    }
    return i;
}
*/

/**
 * @brief ClampByte
 * @param[in] i
 * @return
 */
byte ClampByte(int i)
{
	if (i < 0)
	{
		i = 0;
	}
	else if (i > 255)
	{
		i = 255;
	}
	return (byte)i;
}

/**
 * @brief ClampColor
 * @param[in,out] color
  *
 * @note Unused.
void ClampColor(vec4_t color)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (color[i] < 0)
		{
			color[i] = 0;
		}

		if (color[i] > 1)
		{
			color[i] = 1;
		}
	}
}
*/

/**
 * @brief DirToByte
 * @param[in] dir
 * @return
 *
 * @note This isn't a real cheap function to call!
 * update: A new version was made. It runs much faster because it does not..
 * ..lookup a vector from an array, compare it with the input vector, the best matching vector's index in the array is returned.
 * But instead, it stores the vector's xyz in 30 bits (10 bits per component).
 * No need to lookup all vectors, compare vectors. Just split the components, and scale to float range values.
 */
int DirToByte(vec3_t dir)
{
#if 0
	int   i, best;
	float d, bestd;

	if (!dir)
	{
		return 0;
	}

	bestd = 0;
	best  = 0;
	for (i = 0 ; i < NUMVERTEXNORMALS ; i++)
	{
		//d = DotProduct(dir, bytedirs[i]);
		Dot(dir, bytedirs[i], d);
		if (d > bestd)
		{
			bestd = d;
			best  = i;
		}
	}

	return best;
#else
	// We use 30 bits of the eventParm integer to store in a vector.
	// convert vector values from [-1,1] to [0,1] range, then to a range that fits 10 bits. 2^10 == 4096
	int x = (int)((dir[0] * 0.5 + 0.5) * 4096.0 - 1.0); // the greatest value that fits is 4096-1
	int y = (int)((dir[1] * 0.5 + 0.5) * 4096.0 - 1.0);
	int z = (int)((dir[2] * 0.5 + 0.5) * 4096.0 - 1.0);
	return (x << 20) | (y << 10) | z;
#endif
}

/**
 * @brief ByteToDir
 * @param[in] b
 * @param[out] dir
 */
void ByteToDir(int b, vec3_t dir)
{
#if 0
	if (b < 0 || b >= NUMVERTEXNORMALS)
	{
		VectorCopy(vec3_origin, dir);
		return;
	}
	VectorCopy(bytedirs[b], dir);
#else
	// now we must go back from an integer to a vector.
	// First seperate to the coordinate components x, y & z.
	// then convert back to a range [-1,1]
	int x = (b >> 20) & 0b1111111111;
	int y = (b >> 10) & 0b1111111111;
	int z = b & 0b1111111111;
	const float r2048 = (1.0f / 2048.0f);
	dir[0] = (float)(x) * r2048 - 1.0f; // / 4096 * 2.0 - 1.0
	dir[1] = (float)(y) * r2048 - 1.0f;
	dir[2] = (float)(z) * r2048 - 1.0f;
#endif
}

/*
 * @brief ColorBytes3
 * @param[in] r
 * @param[in] g
 * @param[in] b
 *
 * @note Unused.
unsigned ColorBytes3(float r, float g, float b)
{
    unsigned i;

    ((byte *)&i)[0] = (byte)(r * 255);
    ((byte *)&i)[1] = (byte)(g * 255);
    ((byte *)&i)[2] = (byte)(b * 255);

    return i;
}
*/

/**
 * @brief ColorBytes4
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] a
 */
unsigned int ColorBytes4(float r, float g, float b, float a)
{
	unsigned int i;
	((byte *)&i)[0] = (byte)(r * 255);
	((byte *)&i)[1] = (byte)(g * 255);
	((byte *)&i)[2] = (byte)(b * 255);
	((byte *)&i)[3] = (byte)(a * 255);
	return i;
}

/*
 * @brief NormalizeColor
 * @param[in] in
 * @param[out] out
 * @return
 *
 * @note Unused.
float NormalizeColor(const vec3_t in, vec3_t out)
{
    float max;

    max = in[0];
    if (in[1] > max)
    {
        max = in[1];
    }
    if (in[2] > max)
    {
        max = in[2];
    }

    if (max == 0.f)
    {
        VectorClear(out);
    }
    else
    {
        out[0] = in[0] / max;
        out[1] = in[1] / max;
        out[2] = in[2] / max;
    }
    return max;
}
*/

/**
 * @brief PlaneFromPoints
 * @param[in,out] plane
 * @param[in] a
 * @param[in] b
 * @param[in] c
 * @return false if the triangle is degenrate.
 * The normal will point out of the clock for clockwise ordered points
 */
qboolean PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c)
{
	vec3_t d1, d2;
	float n;
	VectorSubtract(b, a, d1);
	VectorSubtract(c, a, d2);
	CrossProduct(d2, d1, plane);
	//if (VectorNormalize(plane) != 0.f)
	VectorNorm(plane, &n);
	if (n != 0.f)
	{
		//plane[3] = DotProduct(a, plane);
		Dot(a, plane, plane[3]);
		return qtrue;
	}
	return qfalse;
}

#ifndef ETL_SSE
/**
 * @brief PlaneFromPoints_void
 * @param[in,out] plane
 * @param[in] a
 * @param[in] b
 * @param[in] c
 * @return nothing.
 *
 * Note: same function as PlaneFromPoints(), but does not return any result
 */
void PlaneFromPoints_void(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c)
{
	vec3_t d1, d2;
	float n;
	VectorSubtract(b, a, d1);
	VectorSubtract(c, a, d2);
	CrossProduct(d2, d1, plane);
	//if (VectorNormalize(plane) != 0.f)
	VectorNorm(plane, &n);
	if (n != 0.f)
	{
		//plane[3] = DotProduct(a, plane);
		Dot(a, plane, plane[3]);
	}
}
#endif

/**
 * @brief RotatePoint
 * @param[out] point
 * @param[in] matrix
 * /
void RotatePoint(vec3_t point, vec3_t matrix[3])
{
	vec3_t tvec;
	VectorCopy(point, tvec);

	///point[0] = DotProduct(matrix[0], tvec);
	///point[1] = DotProduct(matrix[1], tvec);
	///point[2] = DotProduct(matrix[2], tvec);
	//Dot(matrix[0], tvec, point[0]);
	//Dot(matrix[1], tvec, point[1]);
	//Dot(matrix[2], tvec, point[2]);

	VectorRotate(tvec, matrix, point);
}*/

/**
 * @brief RotatePointAroundVector
 * @param[out] dst
 * @param[in] dir
 * @param[in] point
 * @param[in] degrees
 *
 * @note This is not implemented very well...
 * @update: There's a new version of this function implemented..
 */
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees)
{
#if 0
	float  m[3][3];
	float  im[3][3];
	float  zrot[3][3];
	float  tmpmat[3][3];
	float  rot[3][3];
	int    i;
	vec3_t vr, vup, vf;
	double rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(vr, dir);
	CrossProduct(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	Com_Memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	Com_Memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad        = DEG2RAD(degrees);
	zrot[0][0] = cos(rad);
	zrot[0][1] = sin(rad);
	zrot[1][0] = -sin(rad);
	zrot[1][1] = cos(rad);

	MatrixMultiply(m, zrot, tmpmat);
	MatrixMultiply(tmpmat, im, rot);

	for (i = 0; i < 3; i++)
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
#else
	// this is a new implementation
#ifndef ETL_SSE
	mat4_t M;
#endif
	float S, C, C1, D, n;
	vec3_t A, as, axa, C1axa, C1a, C1aa;
	D = DEG2RAD(degrees);
	SinCos(D, S, C);
	C1 = 1.0f - C;
	VectorCopy(dir, A);
	VectorNorm(A, &n);
	if (n == 0.0f) // vec3_norm() returns the length of the vector (before it's normalized)
	{
		// if the length of a vector is 0, that vector has no direction (it's just a position, a point).
		// If we have no axis/vector/line to rotate about, we just return the original point.
		VectorCopy(point, dst);
	}
	else
	{
		// create the rotation-matrix that makes the point rotate around the vector
		axa[0] = A[0] * A[1];			// ax*ay
		axa[1] = A[0] * A[2];			// ax*az
		axa[2] = A[1] * A[2];			// ay*az
		VectorScale(A, S, as);			// as = A * S			// vector * scalar
		VectorScale(axa, C1, C1axa);	// C1axa = axa * C1		// vector * scalar
		VectorScale(A, C1, C1a);		// C1a = A * C1			// vector * scalar
		VectorMultiply(C1a, A, C1aa);	// C1aa = C1a * A		// vector * vector
		VectorAddConst(C1aa, C, C1aa);	// C1aa += C			// vector.xyz += constant
#ifndef ETL_SSE
		// fill the matrix
		Vector4Set(&M[0],	C1aa[0],			C1axa[0] + as[2],	C1axa[1] - as[1],	0.0f);
		Vector4Set(&M[4],	C1axa[0] - as[2],	C1aa[1],			C1axa[2] + as[0],	0.0f);
		Vector4Set(&M[8],	C1axa[1] + as[1],	C1axa[2] - as[0],	C1aa[2],			0.0f);
		Vector4Set(&M[12],	0.0f,				0.0f,				0.0f,				1.0f);
		// transform the point
		VectorTransformM4(M, point, dst);
#else
// ^^those 4 vector4set (to store a matrix), followed by the read of the same matrix (in VectorTransformM4),
// is object for optimalization..
		__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6;
		xmm3 = _mm_set_ps(0.f, C1axa[1] - as[1], C1axa[0] + as[2], C1aa[0]);
		xmm4 = _mm_set_ps(0.f, C1axa[2] + as[0], C1aa[1], C1axa[0] - as[2]);
		xmm5 = _mm_set_ps(0.f, C1aa[2], C1axa[2] - as[0], C1axa[1] + as[1]);
		xmm6 = _mm_set_ps(1.f, 0.f, 0.f, 0.f);
		xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)point), (const __m64 *)(point + 1));
		xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111);
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010);
		xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b00000000);
		xmm0 = _mm_mul_ps(xmm0, xmm3);
		xmm1 = _mm_mul_ps(xmm1, xmm4);
		xmm2 = _mm_mul_ps(xmm2, xmm5);
		xmm0 = _mm_add_ps(xmm0, xmm1);
		xmm0 = _mm_add_ps(xmm0, xmm2);
		xmm0 = _mm_add_ps(xmm0, xmm6);
		xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b10010100);
		_mm_store_ss((float *)dst, xmm0);
		_mm_storeh_pi((__m64 *)(dst + 1), xmm0);
#endif
	}
#endif
}

/*
 * @brief Rotate a point around a vertex.
 * @param[in,out] pnt
 * @param[in] rot_x
 * @param[in] rot_y
 * @param[in] rot_z
 * @param[in] origin
 *
 * @note Unused.
void RotatePointAroundVertex(vec3_t pnt, float rot_x, float rot_y, float rot_z, const vec3_t origin)
{
    float tmp[11];

    // move pnt to rel{0,0,0}
    VectorSubtract(pnt, origin, pnt);

    // init temp values
    tmp[0]  = sin(rot_x);
    tmp[1]  = cos(rot_x);
    tmp[2]  = sin(rot_y);
    tmp[3]  = cos(rot_y);
    tmp[4]  = sin(rot_z);
    tmp[5]  = cos(rot_z);
    tmp[6]  = pnt[1] * tmp[5];
    tmp[7]  = pnt[0] * tmp[4];
    tmp[8]  = pnt[0] * tmp[5];
    tmp[9]  = pnt[1] * tmp[4];
    tmp[10] = pnt[2] * tmp[3];

    // rotate point
    pnt[0] = (tmp[3] * (tmp[8] - tmp[9]) + tmp[3] * tmp[2]);
    pnt[1] = (tmp[0] * (tmp[2] * tmp[8] - tmp[2] * tmp[9] - tmp[10]) + tmp[1] * (tmp[7] + tmp[6]));
    pnt[2] = (tmp[1] * (-tmp[2] * tmp[8] + tmp[2] * tmp[9] + tmp[10]) + tmp[0] * (tmp[7] + tmp[6]));

    // move pnt back
    VectorAdd(pnt, origin, pnt);
}
*/

/**
 * @brief RotateAroundDirection
 * @param[in,out] axis
 * @param[in] yaw
 *
 * This function is only called for effects and missiles..
 * .. that's a lot of work to generate a random axis (effects), and make a missile rotate.
 */
void RotateAroundDirection(vec3_t axis[3], float yaw)
{
	// create an arbitrary axis[1]
	PerpendicularVector(axis[1], axis[0]);

	// rotate it around axis[0] by yaw
	if (yaw != 0.f)
	{
		vec3_t temp;

		VectorCopy(axis[1], temp);
		RotatePointAroundVector(axis[1], axis[0], temp, yaw);
	}

	// cross to get axis[2]
	CrossProduct(axis[0], axis[1], axis[2]);
}

/**
 * @brief CreateRotationMatrix
 * @param[in] angles
 * @param[in,out] matrix
 */
void CreateRotationMatrix(const vec3_t angles, vec3_t matrix[3])
{
	angles_vectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}

/**
 * @brief vec3_to_angles
 * @param[in] value1
 * @param[out] angles
 */
void vec3_to_angles(const vec3_t value1, vec3_t angles)
{
#if 1
	float yaw, pitch;

	if (value1[1] == 0.f && value1[0] == 0.f)
	{
		yaw = 0.0f;
		if (value1[2] > 0.f)
		{
			pitch = 90.0f;
		}
		else
		{
			pitch = 270.0f;
		}
	}
	else
	{
		float forward;

		if (value1[0] != 0.f)
		{
			//yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
			yaw = RAD2DEG(atan2(value1[1], value1[0])); // the 2nd argument must never be == 0.0
			if (yaw < 0.f)
			{
				yaw += 360.0f;
			}
		}
		else if (value1[1] > 0.0f)
		{
			yaw = 90.0f;
		}
		else
		{
			yaw = 270.0f;
		}

		forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
		//pitch   = (atan2(value1[2], forward) * 180 / M_PI);
		pitch = RAD2DEG(atan2(value1[2], forward)); // forward is garanteed to be != 0.0
		if (pitch < 0.0f)
		{
			pitch += 360.0;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW]   = yaw;
	angles[ROLL]  = 0;
#else
	// TODO: work in progress.. just test code here..
	//vec3_t xAxis = { 1.0, 0.0, 0.0 }; // axisDefault[0]
	//vec3_t yAxis = { 0.0, 1.0, 0.0 }; // axisDefault[1]
	//vec3_t zAxis = { 0.0, 0.0, 1.0 }; // axisDefault[2]
	// i'm unsure (and did not check) if value1 is always normalized.
	// It probably isn't, because atan2 doesn't need normalized values,
	// and to calculate the pitch, they first get the length of the 'forward' vector.
	// Now we just normalize value1 first, and use the dotproduct to get the angles.
	// The dotproduct of 2 vectors of length 1 (unit vectors), is the cosine value of the angle between those 2 vectors.
	vec3_t V;
	VectorCopy(value1, V);
	vec3_norm_fast(V); //(void)vec3_norm(V);

/*	angles[PITCH] = RAD2DEG(acos(DotProduct(V, axisDefault[0]))); // acos returns the angle
	angles[YAW] = RAD2DEG(acos(DotProduct(V, axisDefault[1])));
//	angles[PITCH] = RAD2DEG(acos(V[2])) - 90.0; // acos returns the angle
//	angles[YAW] = RAD2DEG(acos(V[0]));
	angles[ROLL] = RAD2DEG(acos(DotProduct(V, axisDefault[1])));*/
	angles[PITCH] = RAD2DEG(acos(V[0])); // acos returns the angle
	angles[YAW] = 0.0f;// RAD2DEG(acos(V[0]) - 90.0f);
	angles[ROLL] = 0.0f;
#endif
}

/**
 * @brief angles_to_axis
 * @param[in] angles
 * @param[in,out] axis
 */
void angles_to_axis(const vec3_t angles, vec3_t axis[3])
{
	vec3_t right;
	// angle vectors returns "right" instead of "y axis"
	angles_vectors(angles, axis[0], right, axis[2]);
	VectorSubtract(vec3_origin, right, axis[1]);
}

/**
 * @brief axis_clear
 * @param[out] axis
 */
void axis_clear(axis_t axis)
{
#ifndef ETL_SSE
	axis[0][0] = 1.0f;
	axis[0][1] = 0.0f;
	axis[0][2] = 0.0f;
	axis[1][0] = 0.0f;
	axis[1][1] = 1.0f;
	axis[1][2] = 0.0f;
	axis[2][0] = 0.0f;
	axis[2][1] = 0.0f;
	axis[2][2] = 1.0f;
#else
	__m128 xmm0;
	xmm0 = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
	_mm_storeu_ps(&axis[0][0], xmm0);
	_mm_storeu_ps(&axis[1][1], xmm0);
	_mm_store_ss(&axis[2][2], xmm0);
#endif
}

/**
 * @brief axis_copy
 * @param[in] in
 * @param[out] out
 */
void axis_copy(axis_t in, axis_t out)
{
	VectorCopy(in[0], out[0]);
	VectorCopy(in[1], out[1]);
	VectorCopy(in[2], out[2]);
}

#ifndef ETL_SSE
/**
 * @brief ProjectPointOnPlane
 * @param[out] dst
 * @param[in] p
 * @param[in] normal
 */
void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
/*
	vec3_t n;
	float  inv_denom = 1.0F / DotProduct(normal, normal);
//	float  d = DotProduct(normal, p) * inv_denom;

//	//n[0] = normal[0] * inv_denom;
//	//n[1] = normal[1] * inv_denom;
//	//n[2] = normal[2] * inv_denom;
//	VectorScale(normal, inv_denom, n);

//	//dst[0] = p[0] - d * n[0];
//	//dst[1] = p[1] - d * n[1];
//	//dst[2] = p[2] - d * n[2];
//	VectorScale(n, d, n);
//	VectorSubtract(p, n, dst);

	float d = DotProduct(normal, p) * inv_denom * inv_denom;
	VectorScale(normal, d, n);
	VectorSubtract(p, n, dst);
*/
	vec3_t n;
	float dotnn, dotnp, d;
	Dot(normal, normal, dotnn);
	Dot(normal, p, dotnp);
	d = dotnp / (dotnn * dotnn);
	VectorScale(normal, d, n);
	VectorSubtract(p, n, dst);
}
#endif

#ifndef ETL_SSE
/**
 * @brief Given a normalized forward vector, create two
 * other perpendicular vectors
 * @param[in] forward
 * @param[in,out] right
 * @param[out] up
 */
void MakeNormalVectors(const vec3_t forward, vec3_t right, vec3_t up)
{
	float d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	//d = DotProduct(right, forward);
	Dot(right, forward, d);
	VectorMA(right, -d, forward, right);
	VectorNormalizeOnly(right);
	CrossProduct(right, forward, up);
}
#endif

/**
 * @brief vec3_rotate
 * @param[in] in
 * @param[in] matrix
 * @param[out] out
 *
 * Note: the SSE version can handle equal 'in' & 'out' vectors. (because 'in' is buffered in an xmm register).
 */
void vec3_rotate(const vec3_t in, vec3_t matrix[3], vec3_t out)
{
#ifndef ETL_SSE
	// in * matrix colums (not rows!)
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0];
	out[1] = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1];
	out[2] = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2];
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5;
	// in
	xmm2 = _mm_load_ss(&in[2]);
	xmm2 = _mm_shuffle_ps(xmm2, xmm2, 0);							// xmm2 = in2 in2 in2 in2
	xmm1 = _mm_loadh_pi(xmm2, (const __m64 *)(&in[0]));				// xmm1 = in1 in0 _   _
	xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010);					// xmm0 = in0 in0 in0 in0
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111);					// xmm1 = in1 in1 in1 in1
	// matrix
	xmm3 = _mm_loadu_ps(&matrix[0][0]);								// xmm3 = m10 m02 m01 m00
	xmm4 = _mm_loadu_ps(&matrix[1][1]);								// xmm4 = m21 m20 m12 m11
	xmm5 = _mm_load_ss(&matrix[2][2]);								// xmm5 = 0   0   0   m22
	xmm5 = _mm_shuffle_ps(xmm5, xmm4, 0b11100100);					// xmm5 = m21 m20 0   m22
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, 0b01001110);					// xmm5 = 0   m22 m21 m20
	xmm4 = _mm_shuffle_ps(xmm4, xmm3, 0b11110100);					// xmm4 = m10 m10 m12 m11
	xmm4 = _mm_shuffle_ps(xmm4, xmm4, 0b11010010);					// xmm4 = m10 m12 m11 m10
	// multiply and add
	xmm3 = _mm_mul_ps(xmm3, xmm0);									// xmm3 = _   in0*m02 in0*m01 in0*m00
	xmm4 = _mm_mul_ps(xmm4, xmm1);									// xmm4 = _   in1*m12 in1*m11 in1*m10
	xmm5 = _mm_mul_ps(xmm5, xmm2);									// xmm5 = _   in2*m22 in2*m21 in2*m20
	xmm5 = _mm_add_ps(xmm5, xmm3);
	xmm5 = _mm_add_ps(xmm5, xmm4);
	// store out
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, 0b10011100);					// xmm5 = o2 o1 _ o0
	_mm_store_ss(&out[0], xmm5);
	_mm_storeh_pi((__m64 *)(&out[1]), xmm5);
#endif
}

//============================================================================

#if !idppc

/**
 * @brief Q_rsqrt
 * @param[in] f
 * @return
 */
float Q_rsqrt(float f)
{
	floatint_t  t;
	float       x2, y;
	const float threehalfs = 1.5F;

	x2  = f * 0.5F;
	t.f = f;
	t.i = 0x5f3759df - (t.i >> 1);                  // what the fuck?
	y   = t.f;
	y   = y * (threehalfs - (x2 * y * y));      // 1st iteration
	//y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

/**
 * @brief Q_fabs
 * @param[in] f
 * @return
 */
float Q_fabs(float f)
{
	floatint_t fi;
	fi.f  = f;
	fi.i &= 0x7FFFFFFF;
	return fi.f;
}
#endif

//============================================================

/**
 * @brief angle_lerp
 * @param[in] from
 * @param[in] to
 * @param[in] frac
 * @return
 */
float angle_lerp(float from, float to, float frac)
{
	float to_from = to - from;
	if (to_from > 180.f)
	{
		to -= 360.f;
	}
	if (to_from < -180.f)
	{
		to += 360.f;
	}
	return(from + frac * to_from);
}

/**
 * @brief vec3_lerp
 * @param[in] start
 * @param[in] end
 * @param[in] frac
 * @param[out] out
 * /
void vec3_lerp(vec3_t start, vec3_t end, float frac, vec3_t out)
{
	vec3_t dist;
	VectorSubtract(end, start, dist);
	VectorMA(start, frac, dist, out);
}*/

/**
 * @brief angle_sub
 * @param[in] a1
 * @param[in] a2
 * @return Always returns a value from -180 to 180
 */
float angle_sub(float a1, float a2)
{
	float a = a1 - a2;
	while (a > 180.f)
	{
		a -= 360.f;
	}
	while (a < -180.f)
	{
		a += 360.f;
	}
	return a;
}

/**
 * @brief angles_sub
 * @param[in] v1
 * @param[in] v2
 * @param[out] v3
 */
void angles_sub(vec3_t v1, vec3_t v2, vec3_t v3)
{
	v3[0] = angle_sub(v1[0], v2[0]);
	v3[1] = angle_sub(v1[1], v2[1]);
	v3[2] = angle_sub(v1[2], v2[2]);
}

/**
 * @brief angle_mod
 * @param[in] a
 * @return
 *
 * This is the exact same function as: angle_norm_360()
 */
float angle_mod(float a)
{
	return _360_DIV_65536 * (float)((int)(a * _65536_DIV_360) & 65535);
}

/*
 * @brief Returns angle normalized to the range [0 <= angle < 2*M_PI].
 * @param[in] angle
 * @note Unused.
float angle_norm_pi(float angle)
{
    return DEG2RAD(angle_norm_360(RAD2DEG(angle)));
}
*/

/**
 * @brief angle_norm_360
 * @param[in] angle
 * @return angle normalized to the range [0 <= angle < 360]
  *
 * This is the exact same function as: angle_mod()
*/
float angle_norm_360(float angle)
{
	return _360_DIV_65536 * (float)((int)(angle * _65536_DIV_360) & 65535);
}

/**
 * @brief angle_norm_180
 * @param[in] angle
 * @return angle normalized to the range [-180 < angle <= 180]
 */
float angle_norm_180(float angle)
{
	angle = angle_norm_360(angle);
	if (angle > 180.0f)
	{
		angle -= 360.0f;
	}
	return angle;
}

/**
 * @brief angle_delta
 * @param[in] angle1
 * @param[in] angle2
 * @return the normalized delta from angle1 to angle2
 */
float angle_delta(float angle1, float angle2)
{
	return angle_norm_180(angle1 - angle2);
}

//============================================================

#ifndef ETL_SSE
// the ETL_SSE version of this function has an inlined macro defined..

/**
 * @brief SetPlaneSignbits
 * @param[in,out] out
 */
void SetPlaneSignbits(struct cplane_s *out)
{
	byte bits = 0, j;
	// for fast box on planeside test
	for (j = 0 ; j < 3 ; j++)
	{
		if (out->normal[j] < 0.0f)
		{
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}
#endif

/*
 * @brief BoxOnPlaneSide2
 * @param[in] emins
 * @param[in] emaxs
 * @param[in] p
 * @return 1, 2, or 1 + 2
 *
 * @note This is the slow, general version
 * @note Unused
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
    int     i;
    float   dist1, dist2, dot0, dot1;
    int     sides;
    vec3_t  corners[2];

    for (i=0 ; i<3 ; i++)
    {
        if (p->normal[i] < 0)
        {
            corners[0][i] = emins[i];
            corners[1][i] = emaxs[i];
        }
        else
        {
            corners[1][i] = emins[i];
            corners[0][i] = emaxs[i];
        }
    }
    //dist1 = DotProduct (p->normal, corners[0]) - p->dist;
    //dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	Dot(p->normal, corners[0], dot0);
	Dot(p->normal, corners[1], dot1);
	dist1 = dot0 - p->dist;
	dist2 = dot1 - p->dist;
	sides = 0;
    if (dist1 >= 0)
        sides = 1;
    if (dist2 < 0)
        sides |= 2;

    return sides;
}
*/

#if defined __LCC__ || defined C_ONLY || defined __GNUC__

/**
 * @brief BoxOnPlaneSide
 * @param[in] emins
 * @param[in] emaxs
 * @param[in] p
 * @return
 */
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float dist1, dist2;
	int   sides;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
		{
			return 1;
		}
		if (p->dist >= emaxs[p->type])
		{
			return 2;
		}
		return 3;
	}

	// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 1:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 2:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 3:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 4:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
	{
		sides = 1;
	}
	if (dist2 < p->dist)
	{
		sides |= 2;
	}

	return sides;
}

#else

#ifndef ETL_SSE

#pragma warning( disable: 4035 )
__inline __declspec(naked) int BoxOnPlaneSide_fast(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	static int bops_initialized;
	static int Ljmptab[8];

	__asm {

		push ebx

		cmp bops_initialized, 1
		je initialized
		mov bops_initialized, 1

		mov Ljmptab[0 * 4], offset Lcase0
		mov Ljmptab[1 * 4], offset Lcase1
		mov Ljmptab[2 * 4], offset Lcase2
		mov Ljmptab[3 * 4], offset Lcase3
		mov Ljmptab[4 * 4], offset Lcase4
		mov Ljmptab[5 * 4], offset Lcase5
		mov Ljmptab[6 * 4], offset Lcase6
		mov Ljmptab[7 * 4], offset Lcase7

initialized:

		mov edx, dword ptr[4 + 12 + esp]
		mov ecx, dword ptr[4 + 4 + esp]
		xor eax, eax
		mov ebx, dword ptr[4 + 8 + esp]
		mov al, byte ptr[17 + edx]
		cmp al, 8
		jge Lerror
		fld dword ptr[0 + edx]
		fld st(0)
		jmp dword ptr[Ljmptab + eax * 4]
Lcase0:
		fmul dword ptr[ebx]
		fld dword ptr[0 + 4 + edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4 + ebx]
		fld dword ptr[0 + 8 + edx]
		fxch st(2)
		fmul dword ptr[4 + ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8 + ebx]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr[8 + ecx]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase1:
		fmul dword ptr[ecx]
		fld dword ptr[0 + 4 + edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4 + ebx]
		fld dword ptr[0 + 8 + edx]
		fxch st(2)
		fmul dword ptr[4 + ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8 + ebx]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr[8 + ecx]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase2:
		fmul dword ptr[ebx]
		fld dword ptr[0 + 4 + edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4 + ecx]
		fld dword ptr[0 + 8 + edx]
		fxch st(2)
		fmul dword ptr[4 + ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8 + ebx]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr[8 + ecx]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase3:
		fmul dword ptr[ecx]
		fld dword ptr[0 + 4 + edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4 + ecx]
		fld dword ptr[0 + 8 + edx]
		fxch st(2)
		fmul dword ptr[4 + ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8 + ebx]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr[8 + ecx]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase4:
		fmul dword ptr[ebx]
		fld dword ptr[0 + 4 + edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4 + ebx]
		fld dword ptr[0 + 8 + edx]
		fxch st(2)
		fmul dword ptr[4 + ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8 + ecx]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr[8 + ebx]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase5:
		fmul dword ptr[ecx]
		fld dword ptr[0 + 4 + edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4 + ebx]
		fld dword ptr[0 + 8 + edx]
		fxch st(2)
		fmul dword ptr[4 + ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8 + ecx]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr[8 + ebx]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase6:
		fmul dword ptr[ebx]
		fld dword ptr[0 + 4 + edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4 + ecx]
		fld dword ptr[0 + 8 + edx]
		fxch st(2)
		fmul dword ptr[4 + ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8 + ecx]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr[8 + ebx]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
		jmp LSetSides
Lcase7:
		fmul dword ptr[ecx]
		fld dword ptr[0 + 4 + edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4 + ecx]
		fld dword ptr[0 + 8 + edx]
		fxch st(2)
		fmul dword ptr[4 + ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8 + ecx]
		fxch st(5)
		faddp st(3), st(0)
		fmul dword ptr[8 + ebx]
		fxch st(1)
		faddp st(3), st(0)
		fxch st(3)
		faddp st(2), st(0)
LSetSides:
		faddp st(2), st(0)
		fcomp dword ptr[12 + edx]
		xor ecx, ecx
		fnstsw ax
		fcomp dword ptr[12 + edx]
		and ah, 1
		xor ah, 1
		add cl, ah
		fnstsw ax
		and ah, 1
		add ah, ah
		add cl, ah
		pop ebx
		mov eax, ecx
		ret
Lerror:
		int 3
	}
}

int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
		{
			return 1;
		}
		if (p->dist >= emaxs[p->type])
		{
			return 2;
		}
		return 3;
	}

	return BoxOnPlaneSide_fast(emins, emaxs, p);
}
#pragma warning( default: 4035 )

#else
// the ETL_SSE version
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float dist1, dist2;
	int   sides;
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
		{
			return 1;
		}
		if (p->dist >= emaxs[p->type])
		{
			return 2;
		}
		return 3;
	}

	switch (p->signbits)
	{
	case 0:
		//dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		//dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		Dot(p->normal, emaxs, dist1);
		Dot(p->normal, emins, dist2);
		break;
	case 1:
		//dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		//dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		xmm0 = _mm_loadh_pi(_mm_load_ss(&p->normal[0]), (const __m64 *)(&p->normal[1]));
		xmm1 = _mm_loadh_pi(_mm_load_ss(&emins[0]), (const __m64 *)(&emaxs[1]));
		xmm2 = _mm_loadh_pi(_mm_load_ss(&emaxs[0]), (const __m64 *)(&emins[1]));
		xmm1 = _mm_mul_ps(xmm1, xmm0);
		//xmm1 = _mm_hadd_ps(xmm1, xmm1);
		//xmm1 = _mm_hadd_ps(xmm1, xmm1);
		xmm4 = _mm_movehdup_ps(xmm1);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm1, xmm4);		//
		xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
		xmm1 = _mm_add_ss(xmm6, xmm4);		//
		_mm_store_ss(&dist1, xmm1);
		xmm2 = _mm_mul_ps(xmm2, xmm0);
		//xmm2 = _mm_hadd_ps(xmm2, xmm2);
		//xmm2 = _mm_hadd_ps(xmm2, xmm2);
		xmm4 = _mm_movehdup_ps(xmm2);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm2, xmm4);		//
		xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
		xmm2 = _mm_add_ss(xmm6, xmm4);		//
		_mm_store_ss(&dist2, xmm2);
		break;
	case 2:
		//dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		//dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		xmm0 = _mm_loadh_pi(_mm_load_ss(&p->normal[0]), (const __m64 *)(&p->normal[1]));
		xmm1 = _mm_loadh_pi(_mm_load_ss(&emins[0]), (const __m64 *)(&emins[1]));
		xmm2 = _mm_loadh_pi(_mm_load_ss(&emaxs[0]), (const __m64 *)(&emaxs[1]));
		xmm3 = _mm_shuffle_ps(xmm2, xmm1, 0b10101111); // xmm3 = mins1 mins1 maxs2 maxs2
		xmm4 = _mm_shuffle_ps(xmm2, xmm3, 0b00110000); // xmm4 = maxs2 mins1 _     maxs0
		xmm4 = _mm_mul_ps(xmm4, xmm0);
		//xmm4 = _mm_hadd_ps(xmm4, xmm4);
		//xmm4 = _mm_hadd_ps(xmm4, xmm4);
		xmm5 = _mm_movehdup_ps(xmm4);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm4, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm4 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist1, xmm4);
		xmm3 = _mm_shuffle_ps(xmm1, xmm2, 0b10101111); // xmm3 = maxs1 maxs1 mins2 mins2
		xmm4 = _mm_shuffle_ps(xmm1, xmm3, 0b00110000); // xmm4 = mins2 maxs1 _     mins0
		xmm4 = _mm_mul_ps(xmm4, xmm0);
		//xmm4 = _mm_hadd_ps(xmm4, xmm4);
		//xmm4 = _mm_hadd_ps(xmm4, xmm4);
		xmm5 = _mm_movehdup_ps(xmm4);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm4, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm4 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist2, xmm4);
		break;
	case 3:
		//dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		//dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		xmm0 = _mm_loadh_pi(_mm_load_ss(&p->normal[2]), (const __m64 *)(&p->normal[0]));
		xmm1 = _mm_loadh_pi(_mm_load_ss(&emaxs[2]), (const __m64 *)(&emins[0]));
		xmm2 = _mm_loadh_pi(_mm_load_ss(&emins[2]), (const __m64 *)(&emaxs[0]));
		xmm1 = _mm_mul_ps(xmm1, xmm0);
		//xmm1 = _mm_hadd_ps(xmm1, xmm1);
		//xmm1 = _mm_hadd_ps(xmm1, xmm1);
		xmm5 = _mm_movehdup_ps(xmm1);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm1, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm1 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist1, xmm1);
		xmm2 = _mm_mul_ps(xmm2, xmm0);
		//xmm2 = _mm_hadd_ps(xmm2, xmm2);
		//xmm2 = _mm_hadd_ps(xmm2, xmm2);
		xmm5 = _mm_movehdup_ps(xmm2);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm2, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm2 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist2, xmm2);
		break;
	case 4:
		//dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		//dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		xmm0 = _mm_loadh_pi(_mm_load_ss(&p->normal[2]), (const __m64 *)(&p->normal[0]));
		xmm1 = _mm_loadh_pi(_mm_load_ss(&emins[2]), (const __m64 *)(&emaxs[0]));
		xmm2 = _mm_loadh_pi(_mm_load_ss(&emaxs[2]), (const __m64 *)(&emins[0]));
		xmm1 = _mm_mul_ps(xmm1, xmm0);
		//xmm1 = _mm_hadd_ps(xmm1, xmm1);
		//xmm1 = _mm_hadd_ps(xmm1, xmm1);
		xmm5 = _mm_movehdup_ps(xmm1);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm1, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm1 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist1, xmm1);
		xmm2 = _mm_mul_ps(xmm2, xmm0);
		//xmm2 = _mm_hadd_ps(xmm2, xmm2);
		//xmm2 = _mm_hadd_ps(xmm2, xmm2);
		xmm5 = _mm_movehdup_ps(xmm2);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm2, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm2 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist2, xmm2);
		break;
	case 5:
		//dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		//dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		xmm0 = _mm_loadh_pi(_mm_load_ss(&p->normal[0]), (const __m64 *)(&p->normal[1]));
		xmm1 = _mm_loadh_pi(_mm_load_ss(&emins[0]), (const __m64 *)(&emins[1]));
		xmm2 = _mm_loadh_pi(_mm_load_ss(&emaxs[0]), (const __m64 *)(&emaxs[1]));
		xmm3 = _mm_shuffle_ps(xmm2, xmm1, 0b10101111); // xmm3 = mins1 mins1 maxs2 maxs2
		xmm4 = _mm_shuffle_ps(xmm2, xmm3, 0b00110000); // xmm4 = maxs2 mins1 _     maxs0
		xmm4 = _mm_mul_ps(xmm4, xmm0);
		//xmm4 = _mm_hadd_ps(xmm4, xmm4);
		//xmm4 = _mm_hadd_ps(xmm4, xmm4);
		xmm5 = _mm_movehdup_ps(xmm4);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm4, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm4 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist2, xmm4);
		xmm3 = _mm_shuffle_ps(xmm1, xmm2, 0b10101111); // xmm3 = maxs1 maxs1 mins2 mins2
		xmm4 = _mm_shuffle_ps(xmm1, xmm3, 0b00110000); // xmm4 = mins2 maxs1 _     mins0
		xmm4 = _mm_mul_ps(xmm4, xmm0);
		//xmm4 = _mm_hadd_ps(xmm4, xmm4);
		//xmm4 = _mm_hadd_ps(xmm4, xmm4);
		xmm5 = _mm_movehdup_ps(xmm4);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm4, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm4 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist1, xmm4);
		break;
	case 6:
		//dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		//dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		xmm0 = _mm_loadh_pi(_mm_load_ss(&p->normal[0]), (const __m64 *)(&p->normal[1]));
		xmm1 = _mm_loadh_pi(_mm_load_ss(&emins[0]), (const __m64 *)(&emaxs[1]));
		xmm2 = _mm_loadh_pi(_mm_load_ss(&emaxs[0]), (const __m64 *)(&emins[1]));
		xmm1 = _mm_mul_ps(xmm1, xmm0);
		//xmm1 = _mm_hadd_ps(xmm1, xmm1);
		//xmm1 = _mm_hadd_ps(xmm1, xmm1);
		xmm5 = _mm_movehdup_ps(xmm1);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm1, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm1 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist2, xmm1);
		xmm2 = _mm_mul_ps(xmm2, xmm0);
		//xmm2 = _mm_hadd_ps(xmm2, xmm2);
		//xmm2 = _mm_hadd_ps(xmm2, xmm2);
		xmm5 = _mm_movehdup_ps(xmm2);		// faster version of: 2 * hadd
		xmm6 = _mm_add_ps(xmm2, xmm5);		//
		xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
		xmm2 = _mm_add_ss(xmm6, xmm5);		//
		_mm_store_ss(&dist1, xmm2);
		break;
	case 7:
		//dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		//dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		Dot(p->normal, emaxs, dist2);
		Dot(p->normal, emins, dist1);
		break;
	default:
		dist1 = dist2 = 0.0f;
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
	{
		sides = 1;
	}
	if (dist2 < p->dist)
	{
		sides |= 2;
	}
	return sides;
}
#endif

#endif

/**
 * @brief RadiusFromBounds
 * @param[in] mins
 * @param[in] maxs
 * @return
 */
float RadiusFromBounds(const vec3_t mins, const vec3_t maxs)
{
	/*int    i;
	float  a, b;
	vec3_t corner;
	for (i = 0 ; i < 3 ; i++)
	{
		a         = Q_fabs(mins[i]);
		b         = Q_fabs(maxs[i]);
		corner[i] = a > b ? a : b;
	}
	return VectorLength(corner);*/
	vec3_t a, b, corner;
	VectorAbs(mins, a);
	VectorAbs(maxs, b);
	VectorMax(a, b, corner);
	return VectorLength(corner);
}

#ifndef ETL_SSE
/**
 * @brief ClearBounds
 * @param[in,out] mins
 * @param[in,out] maxs
 */
void ClearBounds(vec3_t mins, vec3_t maxs)
{
#ifndef ETL_SSE
	mins[0] = mins[1] = mins[2] = 99999.0f;
	maxs[0] = maxs[1] = maxs[2] = -99999.0f;
#else
	__m128 xmm0, xmm1;
	xmm0 = _mm_set1_ps(99999.0f);
	xmm1 = _mm_set1_ps(-99999.0f);
	_mm_store_ss(&mins[0], xmm0);
	_mm_storeh_pi((__m64 *)(&mins[1]), xmm0);
	_mm_store_ss(&maxs[0], xmm1);
	_mm_storeh_pi((__m64 *)(&maxs[1]), xmm1);
#endif
}
#endif

#ifndef ETL_SSE
/**
 * @brief AddPointToBounds
 * @param[in] v
 * @param[in,out] mins
 * @param[in,out] maxs
 */
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs)
{
#ifndef ETL_SSE
	if (v[0] < mins[0])
	{
		mins[0] = v[0];
	}
	if (v[0] > maxs[0])
	{
		maxs[0] = v[0];
	}

	if (v[1] < mins[1])
	{
		mins[1] = v[1];
	}
	if (v[1] > maxs[1])
	{
		maxs[1] = v[1];
	}

	if (v[2] < mins[2])
	{
		mins[2] = v[2];
	}
	if (v[2] > maxs[2])
	{
		maxs[2] = v[2];
	}
#else
	__m128 xmm0, xmm1, xmm2;
	xmm0 = _mm_load_ss(&v[0]);
	xmm0 = _mm_loadh_pi(xmm0, (const __m64 *)(&v[1]));
	xmm1 = _mm_load_ss(&mins[0]);
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&mins[1]));
	xmm2 = _mm_load_ss(&maxs[0]);
	xmm2 = _mm_loadh_pi(xmm2, (const __m64 *)(&maxs[1]));
	xmm1 = _mm_min_ps(xmm1, xmm0);
	xmm2 = _mm_max_ps(xmm2, xmm0);
	_mm_store_ss(&mins[0], xmm1);
	_mm_storeh_pi((__m64 *)(&mins[1]), xmm1);
	_mm_store_ss(&maxs[0], xmm2);
	_mm_storeh_pi((__m64 *)(&maxs[1]), xmm2);
#endif
}
#endif

/*
 * @brief PointInBounds
 * @param[in] v
 * @param[in] mins
 * @param[in] maxs
 * @return
 *
 * @note Unused.
qboolean PointInBounds(const vec3_t v, const vec3_t mins, const vec3_t maxs)
{
    if (v[0] < mins[0])
    {
        return qfalse;
    }
    if (v[0] > maxs[0])
    {
        return qfalse;
    }

    if (v[1] < mins[1])
    {
        return qfalse;
    }
    if (v[1] > maxs[1])
    {
        return qfalse;
    }

    if (v[2] < mins[2])
    {
        return qfalse;
    }
    if (v[2] > maxs[2])
    {
        return qfalse;
    }

    return qtrue;
}
*/

#ifndef ETL_SSE
/**
 * @brief BoundsAdd
 * @param[in,out] mins
 * @param[in,out] maxs
 * @param[in] mins2
 * @param[in] maxs2
 */
void BoundsAdd(vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2)
{
#ifndef ETL_SSE
	if (mins2[0] < mins[0])
	{
		mins[0] = mins2[0];
	}

	if (mins2[1] < mins[1])
	{
		mins[1] = mins2[1];
	}

	if (mins2[2] < mins[2])
	{
		mins[2] = mins2[2];
	}

	if (maxs2[0] > maxs[0])
	{
		maxs[0] = maxs2[0];
	}

	if (maxs2[1] > maxs[1])
	{
		maxs[1] = maxs2[1];
	}

	if (maxs2[2] > maxs[2])
	{
		maxs[2] = maxs2[2];
	}
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4;
	xmm1 = _mm_load_ss(&mins[0]);
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&mins[1]));
	xmm2 = _mm_load_ss(&mins2[0]);
	xmm2 = _mm_loadh_pi(xmm2, (const __m64 *)(&mins2[1]));

	xmm3 = _mm_load_ss(&maxs[0]);
	xmm3 = _mm_loadh_pi(xmm3, (const __m64 *)(&maxs[1]));
	xmm4 = _mm_load_ss(&maxs2[0]);
	xmm4 = _mm_loadh_pi(xmm4, (const __m64 *)(&maxs2[1]));

	xmm1 = _mm_min_ps(xmm1, xmm2);
	xmm3 = _mm_max_ps(xmm3, xmm4);

	_mm_store_ss(&mins[0], xmm1);
	_mm_storeh_pi((__m64 *)(&mins[1]), xmm1);
	_mm_store_ss(&maxs[0], xmm3);
	_mm_storeh_pi((__m64 *)(&maxs[1]), xmm3);
#endif
}
#endif

/**
 * @brief vec3_compare
 * @param[in] v1
 * @param[in] v2
 * @return true if vectors are equal. return false if vectors are unequal.
 */
qboolean vec3_compare(const vec3_t v1, const vec3_t v2)
{
#ifndef ETL_SSE
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
	{
		return qfalse;
	}
	return qtrue;
#else
	__m128 xmm1, xmm2, xmm3;
	xmm1 = _mm_load_ss(&v1[2]);							// xmm1 = 000z
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&v1[0]));	// xmm1 = yx0z
	xmm2 = _mm_load_ss(&v2[2]);
	xmm2 = _mm_loadh_pi(xmm2, (const __m64 *)(&v2[0]));
	xmm3 = _mm_cmpneq_ps(xmm1, xmm2);
	return (_mm_movemask_ps(xmm3) == 0);
#endif
}

/**
 * @brief vec3_compare_lt
 * @param[in] v1
 * @param[in] v2
 * @return a bitmask value, where bit0, bit1 & bit2 are 1 if v1.bit < v2.bit   (0,1,2=x,y,z)
 */
int vec3_compare_lt(const vec3_t v1, const vec3_t v2)
{
#ifndef ETL_SSE
	int mask = 0;
	if (v1[0] < v2[0])
	{
		mask |= (1 << 0);
	}
	if (v1[1] < v2[1])
	{
		mask |= (1 << 1);
	}
	if (v1[2] < v2[2])
	{
		mask |= (1 << 2);
	}
	return mask;
#else
	__m128 xmm1, xmm2, xmm3;
	xmm1 = _mm_load_ss(&v1[0]);
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&v1[1]));
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b01111000);
	xmm2 = _mm_load_ss(&v2[0]);
	xmm2 = _mm_loadh_pi(xmm2, (const __m64 *)(&v2[1]));
	xmm2 = _mm_shuffle_ps(xmm2, xmm2, 0b01111000);
	xmm3 = _mm_cmplt_ps(xmm1, xmm2);
	return _mm_movemask_ps(xmm3);
#endif
}

/**
 * @brief vec3_compare_gt
 * @param[in] v1
 * @param[in] v2
 * @return a bitmask value, where bit0, bit1 & bit2 are 1 if v1.bit > v2.bit   (0,1,2=x,y,z)
 */
int vec3_compare_gt(const vec3_t v1, const vec3_t v2)
{
#ifndef ETL_SSE
	int mask = 0;
	if (v1[0] > v2[0])
	{
		mask |= (1 << 0);
	}
	if (v1[1] > v2[1])
	{
		mask |= (1 << 1);
	}
	if (v1[2] > v2[2])
	{
		mask |= (1 << 2);
	}
	return mask;
#else
	__m128 xmm1, xmm2, xmm3;
	xmm1 = _mm_load_ss(&v1[0]);
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&v1[1]));
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b01111000);
	xmm2 = _mm_load_ss(&v2[0]);
	xmm2 = _mm_loadh_pi(xmm2, (const __m64 *)(&v2[1]));
	xmm2 = _mm_shuffle_ps(xmm2, xmm2, 0b01111000);
	xmm3 = _mm_cmpgt_ps(xmm1, xmm2);
	return _mm_movemask_ps(xmm3);
#endif
}

void vec3_abs(const vec3_t v, vec3_t o)
{
#ifndef ETL_SSE
	o[0] = Q_fabs(v[0]);
	o[1] = Q_fabs(v[1]);
	o[2] = Q_fabs(v[2]);
#else
	// like Q_fabs, bitwise-and the float like an integer, with 0x7FFFFFFF, to get rid of the sign-bit
	__m128 xmm0, xmm1, mask;
	__m128i minus1 = _mm_set1_epi32(-1);
	mask = _mm_castsi128_ps(_mm_srli_epi32(minus1, 1)); // shr one bit, to get the highest bit 0, rest 1
	xmm1 = _mm_loadh_pi(_mm_load_ss(&v[0]), (const __m64 *)(&v[1]));
	xmm0 = _mm_and_ps(xmm1, mask);
	_mm_store_ss(&o[0], xmm0);
	_mm_storeh_pi((__m64 *)(&o[1]), xmm0);
	// maybe faster to take the max(v, -v) ?..
#endif
}

/**
 * @brief vec3_norm
 * @param[in] v
 * @return
 */
vec_t vec3_norm(vec3_t v)
{
#ifndef ETL_SSE
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = (float)sqrt((double)length);
	if (length != 0.f)
	{
		float ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
	return length;
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6;
	float s;
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v[2]), (const __m64 *)(&v[0]));
	xmm3 = xmm2;
	xmm2 = _mm_mul_ps(xmm2, xmm3);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm4 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm4);		//
	xmm0 = _mm_sqrt_ss(xmm2);
	_mm_store_ss(&s, xmm0);
	if (s != 0.0) {
		xmm1 = _mm_rcp_ss(xmm0);
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
		xmm3 = _mm_mul_ps(xmm3, xmm1);
		_mm_store_ss(&v[2], xmm3);
		_mm_storeh_pi((__m64 *)(&v[0]), xmm3);
	}
	return s;
#endif
}

/**
 * @brief vec3_norm_inlined
 * @param[in/out]  v    vec3     the vector
 * @param[out]     l    float    the length of the vector (before normalization)
 * @return nothing
 */
void vec3_norm_inlined(vec3_t v, float *l)
{
#ifndef ETL_SSE
	*l = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	*l = (float)sqrt((double)*l);
	if (*l != 0.f)
	{
		float ilength = 1.0f / *l;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6;
	xmm1 = _mm_load_ss(&v[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&v[0]));
	xmm3 = xmm2;
	xmm2 = _mm_mul_ps(xmm2, xmm3);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm4 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm4);		//
	xmm0 = _mm_sqrt_ss(xmm2);
	_mm_store_ss(l, xmm0);
	if (*l != 0.0) {
		xmm1 = _mm_rcp_ss(xmm0);
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
		xmm3 = _mm_mul_ps(xmm3, xmm1);
		_mm_store_ss(&v[2], xmm3);
		_mm_storeh_pi((__m64 *)(&v[0]), xmm3);
	}
#endif
}

/**
 * @brief vec3_norm_void
 * @param[in] v
 * @return nothing
 * The very same function as vec3_norm, but no function-result is returned
 */
void vec3_norm_void(vec3_t v)
{
#ifndef ETL_SSE
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = (float)sqrt((double)length);
	if (length != 0.f)
	{
		float ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6;
	xmm1 = _mm_load_ss(&v[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&v[0]));
	xmm3 = xmm2;
	xmm2 = _mm_mul_ps(xmm2, xmm3);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm4 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm4);		//
	xmm0 = _mm_sqrt_ss(xmm2);
	if (_mm_cvtss_f32(xmm0) != 0.0f) {
		xmm1 = _mm_rcp_ss(xmm0);
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
		xmm3 = _mm_mul_ps(xmm3, xmm1);
		_mm_store_ss(&v[2], xmm3);
		_mm_storeh_pi((__m64 *)(&v[0]), xmm3);
	}
#endif
}

void vec4_norm_void(vec4_t v)
{
#ifndef ETL_SSE
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = (float)sqrt((double)length);
	if (length != 0.f)
	{
		float ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
		v[3] *= ilength;
}
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6;
	xmm3 = _mm_loadu_ps(&v[0]);
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v[2]), (const __m64 *)(&v[0]));
	xmm2 = _mm_mul_ps(xmm2, xmm2);
	xmm4 = _mm_movehdup_ps(xmm2);
	xmm6 = _mm_add_ps(xmm2, xmm4);
	xmm4 = _mm_movehl_ps(xmm4, xmm6);
	xmm2 = _mm_add_ss(xmm6, xmm4);
	xmm0 = _mm_sqrt_ss(xmm2);
	if (_mm_cvtss_f32(xmm0) != 0.0f) {
		xmm1 = _mm_rcp_ss(xmm0);
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
		xmm3 = _mm_mul_ps(xmm3, xmm1);
		_mm_storeu_ps(&v[0], xmm3);
	}
#endif
}

/**
 * @brief fast vector normalize routine that does not check to make sure
 * that length != 0, nor does it return length
 * @param[in,out] v
 */
void vec3_norm_fast(vec3_t v)
{
#ifndef ETL_SSE
	float ilength, dot;
	Dot(v, v, dot);
	ilength = Q_rsqrt(dot);
	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6;
	xmm1 = _mm_load_ss(&v[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&v[0]));
	xmm3 = xmm2;
	xmm2 = _mm_mul_ps(xmm2, xmm3);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm4 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm4);		//
	xmm0 = _mm_rsqrt_ss(xmm2);
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0);
	xmm3 = _mm_mul_ps(xmm3, xmm0);
	_mm_store_ss(&v[2], xmm3);
	_mm_storeh_pi((__m64 *)(&v[0]), xmm3);
#endif
}

/**
 * @brief vec3_norm2
 * @param[in] v
 * @param[out] out
 * @return
 */
vec_t vec3_norm2(const vec3_t v, vec3_t out)
{
#ifndef ETL_SSE
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = (float)sqrt((double)length);
	if (length != 0.f)
	{
		float ilength = 1.0f / length;
		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
	}
	else
	{
		VectorClear(out);
	}
	return length;
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6;
	float s;
	xmm1 = _mm_load_ss(&v[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&v[0]));
	xmm3 = xmm2;
	xmm2 = _mm_mul_ps(xmm2, xmm3);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm4 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm4);		//
	xmm0 = _mm_sqrt_ss(xmm2);
	_mm_store_ss(&s, xmm0);
	if (s != 0.0) {
		xmm1 = _mm_rcp_ss(xmm0);
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
		xmm3 = _mm_mul_ps(xmm3, xmm1);
		_mm_store_ss(&out[2], xmm3);
		_mm_storeh_pi((__m64 *)(&out[0]), xmm3);
	} else {
		xmm3 = _mm_xor_ps(xmm3, xmm3);
		_mm_store_ss(&out[2], xmm3);
		_mm_storeh_pi((__m64 *)(&out[0]), xmm3);
	}
	return s;
#endif

}

/**
 * @brief vec3_norm2_void
 * @param[in] v
 * @param[out] out
 * @return nothing
 * The very same function as vec3_norm2, but no function-result is returned
 */
void vec3_norm2_void(const vec3_t v, vec3_t out)
{
#ifndef ETL_SSE
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = (float)sqrt((double)length);
	if (length != 0.f)
	{
		float ilength = 1.0f / length;
		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
	}
	else
	{
		VectorClear(out);
	}
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6;
	//float s;
	xmm1 = _mm_load_ss(&v[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&v[0]));
	xmm3 = xmm2;
	xmm2 = _mm_mul_ps(xmm2, xmm3);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm4 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm4);		//
	xmm0 = _mm_sqrt_ss(xmm2);
	//_mm_store_ss(&s, xmm0);
	//if (s != 0.0) {
	if (_mm_cvtss_f32(xmm0)) {
		xmm1 = _mm_rcp_ss(xmm0);
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
		xmm3 = _mm_mul_ps(xmm3, xmm1);
		_mm_store_ss(&out[2], xmm3);
		_mm_storeh_pi((__m64 *)(&out[0]), xmm3);
	} else {
		xmm3 = _mm_xor_ps(xmm3, xmm3);
		_mm_store_ss(&out[2], xmm3);
		_mm_storeh_pi((__m64 *)(&out[0]), xmm3);
	}
#endif

}

/**
 * @brief _VectorMA
 * @param[in] veca
 * @param[in] scale
 * @param[in] vecb
 * @param[out] vecc
 *
 * Vector Multiply Add
 * a * s + b
 */
void _VectorMA(const vec3_t veca, const float scale, const vec3_t vecb, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = veca[0] + scale * vecb[0];
	out[1] = veca[1] + scale * vecb[1];
	out[2] = veca[2] + scale * vecb[2];
#else
	__m128 xmm1, xmm2, xmm3, xmm4;
	xmm1 = _mm_load_ss(&scale);
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0);
	xmm3 = _mm_load_ss(&vecb[2]);
	xmm4 = _mm_loadh_pi(xmm3, (const __m64 *)(&vecb[0]));
	xmm4 = _mm_mul_ps(xmm4, xmm2);
	xmm3 = _mm_load_ss(&veca[2]);
	xmm2 = _mm_loadh_pi(xmm3, (const __m64 *)(&veca[0]));
	xmm4 = _mm_add_ps(xmm4, xmm2);
	_mm_store_ss(&out[2], xmm4);
	_mm_storeh_pi((__m64 *)(&out[0]), xmm4);
#endif
}

// Vector Add Multiply
// (a + b) * s
void _VectorAM(const vec3_t veca, const vec3_t vecb, const float scale, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = (veca[0] + vecb[0]) * scale;
	out[1] = (veca[1] + vecb[1]) * scale;
	out[2] = (veca[2] + vecb[2]) * scale;
#else
	__m128 xmm1, xmm2, xmm3;
	xmm1 = _mm_load_ss(&scale);
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
	xmm3 = _mm_load_ss(&vecb[2]);
	xmm3 = _mm_loadh_pi(xmm3, (const __m64 *)(&vecb[0]));
	xmm2 = _mm_load_ss(&veca[2]);
	xmm2 = _mm_loadh_pi(xmm2, (const __m64 *)(&veca[0]));
	xmm2 = _mm_add_ps(xmm2, xmm3);
	xmm2 = _mm_mul_ps(xmm2, xmm1);
	_mm_store_ss(&out[2], xmm2);
	_mm_storeh_pi((__m64 *)(&out[0]), xmm2);
#endif
}

// Vector4 Add Multiply
// (a + b) * s
void _Vector4AM(const vec4_t veca, const vec4_t vecb, const float scale, vec4_t out)
{
#ifndef ETL_SSE
	out[0] = (veca[0] + vecb[0]) * scale;
	out[1] = (veca[1] + vecb[1]) * scale;
	out[2] = (veca[2] + vecb[2]) * scale;
	out[3] = (veca[3] + vecb[3]) * scale;
#else
	__m128 xmm1, xmm2, xmm3;
	xmm1 = _mm_loadu_ps(veca);
	xmm2 = _mm_loadu_ps(vecb);
	xmm3 = _mm_load_ss(&scale);
	xmm1 = _mm_add_ps(xmm1, xmm2);
	xmm3 = _mm_shuffle_ps(xmm3, xmm3, 0);
	xmm1 = _mm_mul_ps(xmm1, xmm3);
	_mm_storeu_ps(out, xmm1);
#endif
}

// Vector2 Add Multiply
// (a + b) * s
//
// We suppress the warning that the compiler would generate (for this function):
//      "warning C4700: uninitialized local variable 'xmm0' used"
// https://docs.microsoft.com/en-us/cpp/preprocessor/warning?view=vs-2017
// compile with: /W1
#pragma warning(disable:4700)
void _Vector2AM(const vec2_t veca, const vec2_t vecb, const float scale, vec2_t out)
{
#ifndef ETL_SSE
	out[0] = (veca[0] + vecb[0]) * scale;
	out[1] = (veca[1] + vecb[1]) * scale;
#else
	__m128 xmm0, xmm1, xmm2;
	xmm0 = _mm_loadl_pi(xmm0, (const __m64 *)veca); // we don't care about xmm0's upper bits
	xmm1 = _mm_loadl_pi(xmm1, (const __m64 *)vecb); // we only use the lower bits
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm2 = _mm_load_ss(&scale);
	xmm2 = _mm_shuffle_ps(xmm2, xmm2, 0);
	xmm0 = _mm_mul_ps(xmm0, xmm2);
	_mm_storel_pi((__m64 *)out, xmm0);
	#pragma warning(default:4700)
#endif
}

// Vector4Set
// out = (x,y,z,w)
void _Vector4Set(const float x, const float y, const float z, const float w, vec4_t out)
{
#ifndef ETL_SSE
	out[0] = x;
	out[1] = y;
	out[2] = z;
	out[3] = w;
#else
	_mm_storeu_ps(out, _mm_set_ps(w, z, y, x));
#endif
}

// Vector4Set4
// out = (value,value,value,value)
void _Vector4Set4(const float value, vec4_t out)
{
#ifndef ETL_SSE
	out[0] = value;
	out[1] = value;
	out[2] = value;
	out[3] = value;
#else
	_mm_storeu_ps(out, _mm_set_ps1(value));
#endif
}

/**
 * @brief _DotProduct
 * @param[in] v1
 * @param[in] v2
 * @return
 */
vec_t _DotProduct(const vec3_t v1, const vec3_t v2)
{
#ifndef ETL_SSE
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
#else
	__m128 xmm0, xmm3, xmm4, xmm6;
	//float s;
	xmm0 = _mm_load_ss(&v1[2]); // xmm0 = 0 0 0 v1z
	xmm0 = _mm_loadh_pi(xmm0, (const __m64 *)(&v1[0]));
	xmm3 = _mm_load_ss(&v2[2]);
	xmm3 = _mm_loadh_pi(xmm3, (const __m64 *)(&v2[0]));
	xmm0 = _mm_mul_ps(xmm0, xmm3);
	//xmm0 = _mm_hadd_ps(xmm0, xmm0);
	//xmm0 = _mm_hadd_ps(xmm0, xmm0);
	xmm4 = _mm_movehdup_ps(xmm0);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm0, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm0 = _mm_add_ss(xmm6, xmm4);		//
	//_mm_store_ss(&s, xmm0);
	//return s; // cdecl returns a float in ST0 (fpu stack top)  or xmm0?
	return _mm_cvtss_f32(xmm0);
#endif
}

/**
 * @brief __DotProduct
 * @param[in] vector v1
 * @param[in] vector v2
 * @param[out] float out
 */
void __DotProduct(const vec3_t v1, const vec3_t v2, float out)
{
#ifndef ETL_SSE
	out = v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
#else
	__m128 xmm0, xmm3, xmm4, xmm6;
	xmm0 = _mm_load_ss(&v1[2]); // xmm0 = 0 0 0 v1z
	xmm0 = _mm_loadh_pi(xmm0, (const __m64 *)(&v1[0]));
	xmm3 = _mm_load_ss(&v2[2]);
	xmm3 = _mm_loadh_pi(xmm3, (const __m64 *)(&v2[0]));
	xmm0 = _mm_mul_ps(xmm0, xmm3);
	//xmm0 = _mm_hadd_ps(xmm0, xmm0);
	//xmm0 = _mm_hadd_ps(xmm0, xmm0);
	xmm4 = _mm_movehdup_ps(xmm0);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm0, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm0 = _mm_add_ss(xmm6, xmm4);		//
	_mm_store_ss(&out, xmm0);
#endif
}

/**
 * @brief _VectorSubtract
 * @param[in] veca
 * @param[in] vecb
 * @param[out] out
 */
void _VectorSubtract(const vec3_t veca, const vec3_t vecb, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
#else
	__m128 xmm1, xmm3;
	xmm1 = _mm_load_ss(&veca[2]);
	xmm3 = _mm_load_ss(&vecb[2]);
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&veca[0]));
	xmm3 = _mm_loadh_pi(xmm3, (const __m64 *)(&vecb[0]));
	xmm1 = _mm_sub_ps(xmm1, xmm3);
	_mm_store_ss(&out[2], xmm1);
	_mm_storeh_pi((__m64 *)(&out[0]), xmm1);
#endif
}

/**
 * @brief _VectorSubtract
 * @param[in] veca
 * @param[in] vecb
 * @param[out] out
 */
#pragma warning(disable:4700)
void _Vector2Subtract(const vec2_t veca, const vec2_t vecb, vec2_t out)
{
#ifndef ETL_SSE
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
#else
	__m128 xmm0, xmm1;
	xmm0 = _mm_loadl_pi(xmm0, (const __m64 *)veca); // we don't care about xmm0's upper bits
	xmm1 = _mm_loadl_pi(xmm1, (const __m64 *)vecb); // we only use the lower bits
	xmm0 = _mm_sub_ps(xmm0, xmm1);
	_mm_storel_pi((__m64 *)out, xmm0);
#pragma warning(default:4700)
#endif
}

/**
 * @brief _VectorAdd
 * @param[in] veca
 * @param[in] vecb
 * @param[out] out
 */
void _VectorAdd(const vec3_t veca, const vec3_t vecb, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
#else
	__m128 xmm1, xmm2, xmm3, xmm4, xmm5;
	xmm1 = _mm_load_ss(&veca[2]);
	xmm3 = _mm_load_ss(&vecb[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&veca[0]));
	xmm4 = _mm_loadh_pi(xmm3, (const __m64 *)(&vecb[0]));
	xmm5 = _mm_add_ps(xmm2, xmm4);
	_mm_store_ss(&out[2], xmm5);
	_mm_storeh_pi((__m64 *)(&out[0]), xmm5);
#endif
}

/**
 * @brief _VectorAddConst
 * @param[in] v        The vector
 * @param[in] value    The constant value to add
 * @param[out] out     out.x+=value   out.y+=value   out.z+=value
 */
void _VectorAddConst(const vec3_t v, const float value, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = v[0] + value;
	out[1] = v[1] + value;
	out[2] = v[2] + value;
#else
	__m128 xmm1, xmm2;
	xmm1 = _mm_load_ss(&v[0]);
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&v[1]));
	//xmm2 = _mm_load_ss(&value);
	//xmm2 = _mm_shuffle_ps(xmm2, xmm2, 0);
	xmm2 = _mm_load_ps1(&value);
	xmm1 = _mm_add_ps(xmm1, xmm2);
	_mm_store_ss(&out[0], xmm1);
	_mm_storeh_pi((__m64 *)(&out[1]), xmm1);
#endif
}

#pragma warning(disable:4700)
void _Vector2AddConst(const vec2_t v, const float value, vec2_t out)
{
#ifndef ETL_SSE
	out[0] = v[0] + value;
	out[1] = v[1] + value;
#else
	__m128 xmm0;
	xmm0 = _mm_loadl_pi(xmm0, (const __m64 *)v);
	xmm0 = _mm_add_ps(xmm0, _mm_set_ps1(value));
	_mm_storel_pi((__m64 *)out, xmm0);
#endif
#pragma warning(default:4700)
}

/**
 * @brief _VectorCopy
 * @param[in] in
 * @param[out] out
 */
void _VectorCopy(const vec3_t in, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
#else
	__m128 xmm0;
	xmm0 = _mm_load_ss(&in[2]);
	xmm0 = _mm_loadh_pi(xmm0, (const __m64 *)(&in[0]));
	_mm_store_ss(&out[2], xmm0);
	_mm_storeh_pi((__m64 *)(&out[0]), xmm0);
#endif
}

/**
 * @brief _VectorScale
 * @param[in] in
 * @param[in] scale
 * @param[out] out
 */
void _VectorScale(const vec3_t in, const vec_t scale, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
#else
	__m128 xmm1, xmm2, xmm3, xmm4, xmm5;
	xmm1 = _mm_load_ss(&scale);
	xmm3 = _mm_load_ss(&in[2]);
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0);
	xmm4 = _mm_loadh_pi(xmm3, (const __m64 *)(&in[0]));
	xmm5 = _mm_mul_ps(xmm4, xmm2);
	_mm_store_ss(&out[2], xmm5);
	_mm_storeh_pi((__m64 *)(&out[0]), xmm5);
#endif
}

// _Vector2Scale
// out = in * scale
void _Vector2Scale(const vec2_t in, const float scale, vec2_t out)
{
#ifndef ETL_SSE
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
#else
	__m128 xmm0, xmm1;
	xmm1 = _mm_load_ss(&scale);
	xmm0 = _mm_loadl_pi(xmm1, (const __m64 *)(&in[0]));
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
	xmm0 = _mm_mul_ps(xmm0, xmm1);
	_mm_storel_pi((__m64 *)(&out[0]), xmm0);
#endif
}

/**
 * @brief vec3_cross
 * @param[in] v1
 * @param[in] v2
 * @param[out] cross
 */
void vec3_cross(const vec3_t v1, const vec3_t v2, vec3_t cross)
{
#ifndef ETL_SSE
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
#else
	// unaligned version (for vec3)
	__m128 xmm1, xmm2, xmm4;
	xmm1 = _mm_load_ss(&v1[2]);								// xmm1 =  _   _  _ v1z
	xmm2 = _mm_load_ss(&v2[0]);								// xmm2 =  -   -  - v2x
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&v1[0]));		// xmm1 = v1y v1x _ v1z
	xmm2 = _mm_loadh_pi(xmm2, (const __m64 *)(&v2[1]));		// xmm2 = v2z v2y - v2x
	xmm4 = xmm2;
	xmm2 = _mm_mul_ps(xmm2, xmm1);							// xmm2 = v1y*v2z v1x*v2y - v1z*v2x
	xmm4 = _mm_shuffle_ps(xmm4, xmm4, 0b00110110);			// xmm4 = v2x v2z - v2y
	xmm4 = _mm_mul_ps(xmm4, xmm1);							// xmm4 = v1y*v2x v1x*v2z - v1z*v2y
	xmm2 = _mm_shuffle_ps(xmm2, xmm2, 0b10000111);			// xmm2 = v1x*v2y v1z*v2x - v1y*v2z
	xmm2 = _mm_sub_ps(xmm2, xmm4);							// xmm2 = v1x*v2y-v1y*v2x  v1z*v2x-v1x*v2z  -  v1y*v2z-v1z*v2y
	_mm_store_ss(&cross[0], xmm2);
	_mm_storeh_pi((__m64 *)(&cross[1]), xmm2);
#endif
}

/**
 * @brief vec3_length
 * @param[in] v
 * @return
 */
vec_t vec3_length(const vec3_t v)
{
#ifndef ETL_SSE
	return (float)sqrt((double)(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
#else
	__m128 xmm0, xmm1, xmm2, xmm4, xmm6;
	xmm1 = _mm_load_ss(&v[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&v[0]));
	xmm2 = _mm_mul_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm4 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm4);		//
	xmm0 = _mm_sqrt_ss(xmm2);
	return _mm_cvtss_f32(xmm0);
#endif
}

/**
 * @brief vec3_length_squared
 * @param[in] v
 * @return
 */
vec_t vec3_length_squared(const vec3_t v)
{
#ifndef ETL_SSE
	return (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
#else
	__m128 xmm1, xmm2, xmm4, xmm6;
	xmm1 = _mm_load_ss(&v[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&v[0]));
	xmm2 = _mm_mul_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm4 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm4);		//
	xmm4 = _mm_movehl_ps(xmm4, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm4);		//
	return _mm_cvtss_f32(xmm2);
#endif
}

/**
 * @brief vec3_distance
 * @param[in] p1
 * @param[in] p2
 * @return
 */
vec_t vec3_distance(const vec3_t p1, const vec3_t p2)
{
#ifndef ETL_SSE
	vec3_t v;
	VectorSubtract(p2, p1, v);
	return VectorLength(v);
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	xmm1 = _mm_load_ss(&p1[2]);
	xmm3 = _mm_load_ss(&p2[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&p1[0]));
	xmm4 = _mm_loadh_pi(xmm3, (const __m64 *)(&p2[0]));
	xmm5 = _mm_sub_ps(xmm2, xmm4);
	xmm5 = _mm_mul_ps(xmm5, xmm5);
	//xmm5 = _mm_hadd_ps(xmm5, xmm5);
	//xmm5 = _mm_hadd_ps(xmm5, xmm5);
	xmm7 = _mm_movehdup_ps(xmm5);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm5, xmm7);		//
	xmm7 = _mm_movehl_ps(xmm7, xmm6);	//
	xmm5 = _mm_add_ss(xmm6, xmm7);		//
	xmm0 = _mm_sqrt_ss(xmm5);
	return _mm_cvtss_f32(xmm0);
#endif
}

/**
 * @brief vec3_distance_squared
 * @param[in] p1
 * @param[in] p2
 * @return
 */
vec_t vec3_distance_squared(const vec3_t p1, const vec3_t p2)
{
#ifndef ETL_SSE
	vec3_t v;
	VectorSubtract(p2, p1, v);
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
#else
	__m128 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	xmm1 = _mm_load_ss(&p1[2]);
	xmm2 = _mm_loadh_pi(xmm1, (const __m64 *)(&p1[0]));
	xmm3 = _mm_load_ss(&p2[2]);
	xmm4 = _mm_loadh_pi(xmm3, (const __m64 *)(&p2[0]));
	xmm5 = _mm_sub_ps(xmm2, xmm4);
	xmm5 = _mm_mul_ps(xmm5, xmm5);
	//xmm5 = _mm_hadd_ps(xmm5, xmm5);
	//xmm5 = _mm_hadd_ps(xmm5, xmm5);
	xmm7 = _mm_movehdup_ps(xmm5);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm5, xmm7);		//
	xmm7 = _mm_movehl_ps(xmm7, xmm6);	//
	xmm5 = _mm_add_ss(xmm6, xmm7);		//
	return _mm_cvtss_f32(xmm5);
#endif
}

/**
 * @brief vec3_inv
 * @param[in,out] v
 */
void vec3_inv(vec3_t v)
{
#ifndef ETL_SSE
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
#else
	__m128 xmm0, xmm1;
	xmm1 = _mm_load_ss(&v[0]);
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&v[1]));
	xmm0 = _mm_sub_ps(_mm_setzero_ps(), xmm1);
	_mm_store_ss(&v[0], xmm0);
	_mm_storeh_pi((__m64 *)(&v[1]), xmm0);
#endif
}

/**
 * @brief _Short3Vector
 * @param[in]  in vector with short integer components
 * @param[out] out vector with float components
 */
void _Short3Vector(const short in[3], vec3_t out)
{
	/*out[0] = (float)in[0] * _360_DIV_65536;
	out[1] = (float)in[1] * _360_DIV_65536;
	out[2] = (float)in[2] * _360_DIV_65536;*/
	out[0] = (float)in[0];
	out[1] = (float)in[1];
	out[2] = (float)in[2];
	VectorScale(out, _360_DIV_65536, out);
}

/**
 * @brief _VectorMultiply
 * @param[in]  v1
 * @param[in]  v2 
 * @param[out] out.x=v1.x*v2.x  out.y=v1.y*v2.y  out.z=v1.z*v2.z
 */
void _VectorMultiply(const vec3_t v1, const vec3_t v2, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = v1[0] * v2[0];
	out[1] = v1[1] * v2[1];
	out[2] = v1[2] * v2[2];
#else
	__m128 xmm1, xmm2;
	xmm1 = _mm_load_ss(&v1[0]);
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&v1[1]));
	xmm2 = _mm_load_ss(&v2[0]);
	xmm2 = _mm_loadh_pi(xmm2, (const __m64 *)(&v2[1]));
	xmm2 = _mm_mul_ps(xmm2, xmm1);
	_mm_store_ss(&out[0], xmm2);
	_mm_storeh_pi((__m64 *)(&out[1]), xmm2);
#endif
}

/*
 * @brief Q_log2
 * @param val
 * @return
 * @note Unused.
int Q_log2(int val)
{
    int answer = 0;

    while ((val >>= 1) != 0)
    {
        answer++;
    }
    return answer;
}
*/

/*
 * @brief PlaneTypeForNormal
 * @param[in] normal
 * @return
 * @note Unused
int PlaneTypeForNormal (vec3_t normal) {
    if ( normal[0] == 1.0 )
        return PLANE_X;
    if ( normal[1] == 1.0 )
        return PLANE_Y;
    if ( normal[2] == 1.0 )
        return PLANE_Z;

    return PLANE_NON_AXIAL;
}
*/

/**
 * @brief _MatrixMultiply
 * @param[in] in1
 * @param[in] in2
 * @param[out] out
 */
void _MatrixMultiply(const float in1[3][3], const float in2[3][3], float out[3][3])
{
#ifndef ETL_SSE
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
#else
	// in1[0][0] = A	in2[0][0] = J		out[0][0] = A*J + B*M + C*P
	// in1[0][1] = B	in2[0][1] = K		out[0][1] = A*K + B*N + C*Q
	// in1[0][2] = C	in2[0][2] = L		out[0][2] = A*L + B*O + C*R
	// in1[1][0] = D	in2[1][0] = M		out[1][0] = D*J + E*M + F*P
	// in1[1][1] = E	in2[1][1] = N		out[1][1] = D*K + E*N + F*Q
	// in1[1][2] = F	in2[1][2] = O		out[1][2] = D*L + E*O + F*R
	// in1[2][0] = G	in2[2][0] = P		out[2][0] = G*J + H*M + I*P
	// in1[2][1] = H	in2[2][1] = Q		out[2][1] = G*K + H*N + I*Q
	// in1[2][2] = I	in2[2][2] = R		out[2][2] = G*L + H*O + I*R
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	xmm1 = _mm_loadu_ps(&in2[0][0]);						// xmm1 = _LKJ
	xmm3 = _mm_loadu_ps(&in2[1][0]);						// xmm3 = _ONM
	xmm5 = _mm_loadu_ps(&in2[1][2]);						// xmm5 = RQPO
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, 0b00111001);			// xmm5 = _RQP = in2[2][0]
	// out[0]
	xmm7 = _mm_loadu_ps(&in1[0][0]);						// xmm7 = DCBA
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0);					// xmm0 = AAAA
	xmm2 = _mm_shuffle_ps(xmm7, xmm7, 0b01010101);			// xmm2 = BBBB
	xmm4 = _mm_shuffle_ps(xmm7, xmm7, 0b10101010);			// xmm4 = CCCC
	xmm0 = _mm_mul_ps(xmm0, xmm1);							// xmm0 = _ AL AK AJ
	xmm2 = _mm_mul_ps(xmm2, xmm3);							// xmm2 = _ BO BN BM
	xmm4 = _mm_mul_ps(xmm4, xmm5);							// xmm4 = _ CR CQ CP
	xmm0 = _mm_add_ps(xmm0, xmm2);							// xmm0 = _ AL+BO    AK+BN    AJ+BM
	xmm0 = _mm_add_ps(xmm0, xmm4);							// xmm0 = _ AL+BO+CR AK+BN+CQ AJ+BM+CP
	_mm_storeu_ps(&out[0][0], xmm0);
	// out[1]
	xmm7 = _mm_loadu_ps(&in1[1][0]);						// xmm7 = GFED
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0);					// xmm0 = DDDD
	xmm2 = _mm_shuffle_ps(xmm7, xmm7, 0b01010101);			// xmm2 = EEEE
	xmm4 = _mm_shuffle_ps(xmm7, xmm7, 0b10101010);			// xmm4 = FFFF
	xmm0 = _mm_mul_ps(xmm0, xmm1);							// xmm0 = _ DL DK DJ
	xmm2 = _mm_mul_ps(xmm2, xmm3);							// xmm2 = _ EO EN EM
	xmm4 = _mm_mul_ps(xmm4, xmm5);							// xmm4 = _ FR FQ FP
	xmm0 = _mm_add_ps(xmm0, xmm2);							// xmm0 = _ DL+EO    DK+EN    DJ+EM
	xmm6 = _mm_add_ps(xmm0, xmm4);							// xmm6 = _ DL+EO+FR DK+EN+FQ DJ+EM+FP
	_mm_storeu_ps(&out[1][0], xmm6);
	// out[2]
	// the last row is handled specially, because we write 4 floats at a time,
	// and the row is basically a vec3. 
	xmm7 = _mm_loadu_ps(&in1[1][2]);						// xmm7 = IHGF
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0b01010101);			// xmm0 = GGGG
	xmm2 = _mm_shuffle_ps(xmm7, xmm7, 0b10101010);			// xmm2 = HHHH
	xmm4 = _mm_shuffle_ps(xmm7, xmm7, 0b11111111);			// xmm4 = IIII
	xmm0 = _mm_mul_ps(xmm0, xmm1);							// xmm0 = _ GL GK GJ
	xmm2 = _mm_mul_ps(xmm2, xmm3);							// xmm2 = _ HO HN HM
	xmm4 = _mm_mul_ps(xmm4, xmm5);							// xmm4 = _ IR IQ IP
	xmm0 = _mm_add_ps(xmm0, xmm2);							// xmm0 = _ GL+HO GK+HN GJ+HM
	xmm0 = _mm_add_ps(xmm0, xmm4);							// xmm0 = _ GL+HO+IR GK+HN+IQ GJ+HM+IP
	xmm6 = _mm_shuffle_ps(xmm6, xmm0, 0b00001010);			// xmm6 = GJ+HM+IP GJ+HM+IP DL+EO+FR DL+EO+FR
	xmm6 = _mm_shuffle_ps(xmm6, xmm0, 0b10011100);			// xmm6 = GL+HO+IR GK+HN+IQ GJ+HM+IP DL+EO+FR
	_mm_storeu_ps(&out[1][2], xmm6);						// store out[2][0] = o20-o21-o22  (and also o12)
#endif
}


/**
 * @brief mat3_transpose
 * @param[in] matrix
 * @param[out] transpose
 */
void mat3_transpose(vec3_t matrix[3], vec3_t transpose[3])
{
#ifndef ETL_SSE
	int i, j;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			transpose[i][j] = matrix[j][i];
		}
	}
#else
/*	transpose[0][0] = matrix[0][0];
	transpose[0][1] = matrix[1][0];
	transpose[0][2] = matrix[2][0];
	transpose[1][0] = matrix[0][1];
	transpose[1][1] = matrix[1][1];
	transpose[1][2] = matrix[2][1];
	transpose[2][0] = matrix[0][2];
	transpose[2][1] = matrix[1][2];
	transpose[2][2] = matrix[2][2];*/
	__m128 xmm0, xmm1, xmm3, xmm4, xmm5;
	xmm0 = _mm_loadu_ps(&matrix[0][0]);							// xmm0 = m10 m02 m01 m00
	xmm1 = _mm_loadu_ps(&matrix[1][1]);							// xmm1 = m21 m20 m12 m11
	xmm3 = _mm_shuffle_ps(xmm0, xmm1, 0b10100100);				// xmm3 = m20 m20 m01 m00
	xmm4 = _mm_shuffle_ps(xmm0, xmm3, 0b01111100);				// xmm4 = m01 m20 m10 m00
	_mm_storeu_ps(&transpose[0][0], xmm4);
	xmm3 = _mm_shuffle_ps(xmm0, xmm1, 0b01011010);				// xmm3 = m12 m12 m02 m02
	xmm5 = _mm_shuffle_ps(xmm1, xmm3, 0b11011100);				// xmm5 = m12 m02 m21 m11
	_mm_storeu_ps(&transpose[1][1], xmm5);
	_mm_store_ss(&transpose[2][2], _mm_load_ss(&matrix[2][2]));
#endif
}

/**
 * @brief angles_vectors
 * @param[in] angles
 * @param[out] forward
 * @param[out] right
 * @param[out] up
 */
void angles_vectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float angle, sr, spp, sy, cr, cp, cy;

	angle = DEG2RAD(angles[YAW]);
	//sy    = sin(angle);
	//cy    = cos(angle);
	SinCos(angle, sy, cy);

	angle = DEG2RAD(angles[PITCH]);
	//sp    = sin(angle);
	//cp    = cos(angle);
	SinCos(angle, spp, cp);

	angle = DEG2RAD(angles[ROLL]);
	//sr    = sin(angle);
	//cr    = cos(angle);
	SinCos(angle, sr, cr);

	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -spp;
	}
	if (right)
	{
		float _srsp = -sr * spp;
		right[0] = _srsp * cy + -cr * -sy;
		right[1] = _srsp * sy + -cr * cy;
		right[2] = -sr * cp;
	}
	if (up)
	{
		float crsp = cr * spp;
		up[0] = crsp * cy + -sr * -sy;
		up[1] = crsp * sy + -sr * cy;
		up[2] = cr * cp;
	}
}

/**
 * @brief vec3_per
 * @param[in] src assumes is normalized
 * @param[out] dst
 */
void vec3_per(const vec3_t src, vec3_t dst)
{
	int    pos;
	int    i;
	float  minelem = 1.0F;
	vec3_t tempvec;
	//float  abs_srci;

	// find the smallest magnitude axially aligned vector
	VectorAbs(src, tempvec);
	for (pos = 0, i = 0; i < 3; i++)
	{
		/*abs_srci = Q_fabs(src[i]);
		if (abs_srci < minelem)
		{
			pos     = i;
			minelem = abs_srci;
		}*/
		if (tempvec[i] < minelem)
		{
			pos = i;
			minelem = tempvec[i];
		}
	}
	//tempvec[0] = tempvec[1] = tempvec[2] = 0.0f;
	VectorClear(tempvec);
	tempvec[pos] = 1.0f;

	// project the point onto the plane defined by src
	ProjectPointOnPlane(dst, tempvec, src);

	// normalize the result
	VectorNormalizeOnly(dst);
}

#ifndef ETL_SSE
/**
 * @brief Used to find an "up" vector for drawing a sprite so that it always faces the view as best as possible
 * @param[in] point
 * @param[in] p1
 * @param[in] p2
 * @param[out] up
 */
void GetPerpendicularViewVector(const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up)
{
	vec3_t v1, v2;

	VectorSubtract(point, p1, v1);
	//VectorNormalizeOnly(v1);

	VectorSubtract(point, p2, v2);
	//VectorNormalizeOnly(v2);

	CrossProduct(v1, v2, up);
	VectorNormalizeOnly(up);
}
#endif

#ifndef ETL_SSE
/**
 * @brief ProjectPointOntoVector
 * @param[in] point
 * @param[in] vStart
 * @param[in] vEnd
 * @param[out] vProj
 */
void ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj)
{
	vec3_t pVec, vec;
	float dot;
	VectorSubtract(point, vStart, pVec);
	VectorSubtract(vEnd, vStart, vec);
	VectorNormalizeOnly(vec);
	// project onto the directional vector for this segment
	//VectorMA(vStart, DotProduct(pVec, vec), vec, vProj);
	Dot(pVec, vec, dot);
	VectorMA(vStart, dot, vec, vProj);
}
#endif

/**
 * @brief ProjectPointOntoVectorBounded
 * @param[in] point
 * @param[in] vStart
 * @param[in] vEnd
 * @param[out] vProj
 *
 * @note Unused
 *
 * /
void ProjectPointOntoVectorBounded(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj)
{
	vec3_t pVec, vec;
	int    j;
	float dot;

	VectorSubtract(point, vStart, pVec);
	VectorSubtract(vEnd, vStart, vec);
	VectorNormalizeOnly(vec);
	// project onto the directional vector for this segment
	//VectorMA(vStart, DotProduct(pVec, vec), vec, vProj);
	Dot(pVec, vec, dot);
	VectorMA(vStart, dot, vec, vProj);
	// check bounds
	for (j = 0; j < 3; j++)
		if ((vProj[j] > vStart[j] && vProj[j] > vEnd[j]) ||
		    (vProj[j] < vStart[j] && vProj[j] < vEnd[j]))
		{
			break;
		}
	if (j < 3)
	{
		if (Q_fabs(vProj[j] - vStart[j]) < Q_fabs(vProj[j] - vEnd[j]))
		{
			VectorCopy(vStart, vProj);
		}
		else
		{
			VectorCopy(vEnd, vProj);
		}
	}
}*/

/**
 * @brief DistanceFromLineSquared
 * @param[in] p
 * @param[in] lp1
 * @param[in] lp2
 * @return
 */
float DistanceFromLineSquared(vec3_t p, vec3_t lp1, vec3_t lp2)
{
	vec3_t proj, t;
	int    j;

	ProjectPointOntoVector(p, lp1, lp2, proj);
	for (j = 0; j < 3; j++)
	{
		if ((proj[j] > lp1[j] && proj[j] > lp2[j]) ||
			(proj[j] < lp1[j] && proj[j] < lp2[j]))
		{
			if (Q_fabs(proj[j] - lp1[j]) < Q_fabs(proj[j] - lp2[j]))
			{
				VectorSubtract(p, lp1, t);
			}
			else
			{
				VectorSubtract(p, lp2, t);
			}
			return VectorLengthSquared(t);
		}
	}
	VectorSubtract(p, proj, t);
	return VectorLengthSquared(t);
}

/**
 * @brief DistanceFromVectorSquared
 * @param[in] p
 * @param[in] lp1
 * @param[in] lp2
 * @return
 *
 * @note Unused
 * /
float DistanceFromVectorSquared(vec3_t p, vec3_t lp1, vec3_t lp2)
{
	vec3_t proj, t;
	ProjectPointOntoVector(p, lp1, lp2, proj);
	VectorSubtract(p, proj, t);
	return VectorLengthSquared(t);
}*/

/**
 * @brief vec3_to_yawn
 * @param[in] vec
 * @return
 */
float vec3_to_yawn(const vec3_t vec)
{
	float yaw;

	if (vec[YAW] == 0.f && vec[PITCH] == 0.f)
	{
		yaw = 0.f;
	}
	else
	{
		if (vec[PITCH] != 0.f)
		{
			//yaw = (float)(atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
			yaw = RAD2DEG(atan2(vec[YAW], vec[PITCH]));
			if (yaw < 0.f)
			{
				yaw += 360.f;
			}
		}
		else if (vec[YAW] > 0.f)
		{
			yaw = 90.f;
		}
		else
		{
			yaw = 270.f;
		}
	}

	return yaw;
}

/**
 * @brief Used to convert the MD3 tag axis to MDC tag angles, which are much smaller
 *
 * @details This doesn't have to be fast, since it's only used for conversion in utils, try to avoid
 * using this during gameplay
 *
 * @param[in] axis
 * @param[out] angles
 */
void axis_to_angles(axis_t axis, vec3_t angles)
{
	vec3_t right, roll_angles, tvec;
	float dot;

	// first get the pitch and yaw from the forward vector
	vec3_to_angles(axis[0], angles);
	// now get the roll from the right vector
	VectorCopy(axis[1], right);
	// get the angle difference between the tmpAxis[2] and axis[2] after they have been reverse-rotated
	RotatePointAroundVector(tvec, axisDefault[2], right, -angles[YAW]);
	RotatePointAroundVector(right, axisDefault[1], tvec, -angles[PITCH]);
	// now find the angles, the PITCH is effectively our ROLL
	vec3_to_angles(right, roll_angles);
	roll_angles[PITCH] = angle_norm_180(roll_angles[PITCH]);
	// if the yaw is more than 90 degrees difference, we should adjust the pitch
	Dot(right, axisDefault[1], dot);
	if (dot < 0.f)
	{
		if (roll_angles[PITCH] < 0.0f)
		{
			//roll_angles[PITCH] = -90 + (-90 - roll_angles[PITCH]);
			roll_angles[PITCH] = -180.0f - roll_angles[PITCH];
		}
		else
		{
			//roll_angles[PITCH] = 90 + (90 - roll_angles[PITCH]);
			roll_angles[PITCH] = 180.0f - roll_angles[PITCH];
		}
	}
	angles[ROLL] = -roll_angles[PITCH];
}

/**
 * @brief vec3_dist
 * @param[in] v1
 * @param[in] v2
 * @return
 */
float vec3_dist(vec3_t v1, vec3_t v2)
{
#ifndef ETL_SSE
	vec3_t dir;
	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
#else
	__m128 xmm1, xmm2, xmm6, xmm7;
	// subtract
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v2[0]), (const __m64 *)(&v2[1]));
	xmm1 = _mm_loadh_pi(_mm_load_ss(&v1[0]), (const __m64 *)(&v1[1]));		// xmm1 = z y _ x
	xmm2 = _mm_sub_ps(xmm2, xmm1);
	// length
	xmm2 = _mm_mul_ps(xmm2, xmm2);											// xmm2 = z*z y*y _ x*x
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);											// xmm2 = zz + yy + xx
	xmm7 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm7);		//
	xmm7 = _mm_movehl_ps(xmm7, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm7);		//
	xmm2 = _mm_sqrt_ss(xmm2);
	return _mm_cvtss_f32(xmm2);
#endif
}

/**
 * @brief vec3_dist_squared
 * @param[in] v1
 * @param[in] v2
 * @return
 */
float vec3_dist_squared(vec3_t v1, vec3_t v2)
{
#ifndef ETL_SSE
	vec3_t dir;
	VectorSubtract(v2, v1, dir);
	return VectorLengthSquared(dir);
#else
	//float s;
	__m128 xmm1, xmm2, xmm6, xmm7;
	// subtract
	xmm1 = _mm_load_ss(&v1[2]);								// xmm1 = _ _ _ z
	xmm2 = _mm_load_ss(&v2[2]);
	xmm1 = _mm_loadh_pi(xmm1, (const __m64 *)(&v1[0]));		// xmm1 = y x _ z
	xmm2 = _mm_loadh_pi(xmm2, (const __m64 *)(&v2[0]));
	xmm1 = _mm_sub_ps(xmm1, xmm2);
	// length
	xmm1 = _mm_mul_ps(xmm1, xmm1);							// xmm1 = y*y x*x _ z*z
	//xmm1 = _mm_hadd_ps(xmm1, xmm1);
	//xmm1 = _mm_hadd_ps(xmm1, xmm1);
	xmm7 = _mm_movehdup_ps(xmm1);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm1, xmm7);		//
	xmm7 = _mm_movehl_ps(xmm7, xmm6);	//
	xmm1 = _mm_add_ss(xmm6, xmm7);		//
	//_mm_store_ss(&s, xmm1);
	//return s;
	return _mm_cvtss_f32(xmm1);
#endif
}

/**
 * @brief Don't pass doubles to this
 * @param[in] x
 * @return
 */
int Q_isnan(float x)
{
	floatint_t fi;

	fi.f   = x;
	fi.ui &= 0x7FFFFFFF;
	fi.ui  = 0x7F800000 - fi.ui;

	return (int)((unsigned int)fi.ui >> 31);
}

#ifndef Q3_VM
/**
 * @brief The msvc acos doesn't always return a value between -PI and PI:
 *
 * <code>
 * int i;
 * i = 1065353246;
 * acos(*(float*) &i) == -1.\#IND0
 * </code>
 *
 * @param[in] c
 * @return
 */
float Q_acos(float c)
{
	float angle;

	angle = acos(c);

	if (angle > M_PI)
	{
		return (float)M_PI; // is this correct?  maybe?   return pi - (angle - pi);
	}
	if (angle < -M_PI)
	{
		return (float)M_PI; // ..and here something alike^^
	}

	return angle;
}
#endif

/************************************************************************/
/* Quaternion                                                           */
/************************************************************************/

/**
 * @brief quat_from_mat4
 * @param[out] q
 * @param[in] m
 */
void quat_from_mat4(quat_t q, const mat4_t m)
{
#if 1
	/*
	From Quaternion to Matrix and Back
	February 27th 2005
	J.M.P. van Waveren

	http://www.intel.com/cd/ids/developer/asmo-na/eng/293748.htm
	*/
	float t/*, s*/, m0510;

	m0510 = m[0] + m[5] + m[10];
	if (m0510 > 0.0f)
	{
		t = m0510 + 1.0f;
//		s = (1.0f / sqrtf(t)) * 0.5f;
/*
		q[3] = s * t;
		q[2] = (m[1] - m[4]) * s;
		q[1] = (m[8] - m[2]) * s;
		q[0] = (m[6] - m[9]) * s;
*/
		q[3] = t;
		q[2] = m[1] - m[4];
		q[1] = m[8] - m[2];
		q[0] = m[6] - m[9];
	}
	else if (m[0] > m[5] && m[0] > m[10])
	{
		t = m[0] - m[5] - m[10] + 1.0f;
//		s = (1.0f / sqrtf(t)) * 0.5f;
/*
		q[0] = s * t;
		q[1] = (m[1] + m[4]) * s;
		q[2] = (m[8] + m[2]) * s;
		q[3] = (m[6] - m[9]) * s;
*/
		q[0] = t;
		q[1] = m[1] + m[4];
		q[2] = m[8] + m[2];
		q[3] = m[6] - m[9];
	}
	else if (m[5] > m[10])
	{
		t = -m[0] + m[5] - m[10] + 1.0f;
//		s = (1.0f / sqrtf(t)) * 0.5f;
/*
		q[1] = s * t;
		q[0] = (m[1] + m[4]) * s;
		q[3] = (m[8] - m[2]) * s;
		q[2] = (m[6] + m[9]) * s;
*/
		q[1] = t;
		q[0] = m[1] + m[4];
		q[3] = m[8] - m[2];
		q[2] = m[6] + m[9];
	}
	else
	{
		t = -m[0] - m[5] + m[10] + 1.0f;
//		s = (1.0f / sqrtf(t)) * 0.5f;
/*
		q[2] = s * t;
		q[3] = (m[1] - m[4]) * s;
		q[0] = (m[8] + m[2]) * s;
		q[1] = (m[6] + m[9]) * s;
*/
		q[2] = t;
		q[3] = m[1] - m[4];
		q[0] = m[8] + m[2];
		q[1] = m[6] + m[9];
	}
//	//s = (1.0f / sqrtf(t)) * 0.5f;
//	//Vector4Scale(q, s, q);
	Vector4Scale(q, 0.5f / sqrtf(t), q);

#else
	float trace;

	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm

	trace = 1.0f + m[0] + m[5] + m[10];

	if (trace > 0.0f)
	{
		vec_t s = 0.5f / sqrt(trace);

		q[0] = (m[6] - m[9]) * s;
		q[1] = (m[8] - m[2]) * s;
		q[2] = (m[1] - m[4]) * s;
		q[3] = 0.25f / s;
	}
	else
	{
		if (m[0] > m[5] && m[0] > m[10])
		{
			// column 0
			float s = sqrt(1.0f + m[0] - m[5] - m[10]) * 2.0f;
			float _s = 1.0 / s;

			q[0] = 0.25f * s;
			q[1] = (m[4] + m[1]) * _s;
			q[2] = (m[8] + m[2]) * _s;
			q[3] = (m[9] - m[6]) * _s;
		}
		else if (m[5] > m[10])
		{
			// column 1
			float s = sqrt(1.0f + m[5] - m[0] - m[10]) * 2.0f;
			float _s = 1.0 / s;

			q[0] = (m[4] + m[1]) * _s;
			q[1] = 0.25f * s;
			q[2] = (m[9] + m[6]) * _s;
			q[3] = (m[8] - m[2]) * _s;
		}
		else
		{
			// column 2
			float s = sqrt(1.0f + m[10] - m[0] - m[5]) * 2.0f;
			float _s = 1.0 / s;

			q[0] = (m[8] + m[2]) * _s;
			q[1] = (m[9] + m[6]) * _s;
			q[2] = 0.25f * s;
			q[3] = (m[4] - m[1]) * _s;
		}
	}

	quat_norm(q);
#endif
}

#ifdef BONE_HITTESTS
/**
 * @brief quat_from_axis
 * @param[in] m
 * @param[out] q
 */
void quat_from_axis(const axis_t m, quat_t q)
{
	vec_t w4;

	q[3] = (float)(sqrt((double)(1.0f + m[0][0] + m[1][1] + m[2][2])) * 0.5); // / 2.0);
	_w4   = 1.0 / (q[3] * 4.0f);

	q[0] = (m[1][2] - m[2][1]) * _w4;
	q[1] = (m[2][0] - m[0][2]) * _w4;
	q[2] = (m[0][1] - m[1][0]) * _w4;
}
#endif

/**
 * @brief quat_from_angles
 * @param[out] q
 * @param[in] pitch
 * @param[in] yaw
 * @param[in] roll
 *
 * @note Unused
 * /
void quat_from_angles(quat_t q, vec_t pitch, vec_t yaw, vec_t roll)
{
#if 1
	mat4_t tmp;

	mat4_from_angles(tmp, pitch, yaw, roll);
	quat_from_mat4(q, tmp);
#else
	float radp, rady, radr, srcy, crsy, crcy, srsy, sr, sp, sy, cr, cp, cy;

	radp = DEG2RAD(pitch);
	sp = sin(radp);
	cp = cos(radp);

	rady = DEG2RAD(yaw);
	sy = sin(rady);
	cy = cos(rady);

	radr = DEG2RAD(roll);
	sr = sin(radr);
	cr = cos(radr);

	srcy = sr * cy;
	crsy = cr * sy;
	crcy = cr * cy;
	srsy = sr * sy;

	q[0] = srcy * cp - crsy * sp; // x
	q[1] = crcy * sp + srsy * cp; // y
	q[2] = crsy * cp - srcy * sp; // z
	q[3] = crcy * cp + srsy * sp; // w
#endif
}*/

/**
 * @brief quat_to_vec3_FLU
 * @param[in] q
 * @param[out] forward
 * @param[out] left
 * @param[out] up
 *
 * @note Unused
 * /
void quat_to_vec3_FLU(const quat_t q, vec3_t forward, vec3_t left, vec3_t up)
{
	mat4_t tmp;

	Matrix4FromQuaternion(tmp, q);
	MatrixToVectorsFRU(tmp, forward, left, up);
}*/

/**
 * @brief quat_to_vec3_FRU
 * @param[in] q
 * @param[out] forward
 * @param[out] right
 * @param[out] up
 */
void quat_to_vec3_FRU(const quat_t q, vec3_t forward, vec3_t right, vec3_t up)
{
	mat4_t tmp;
	Matrix4FromQuaternion(tmp, q);
	MatrixToVectorsFRU(tmp, forward, right, up);
}

#ifndef ETL_SSE
/**
 * @brief quat_to_axis
 * @param[in] q
 * @param[out] axis
 */
void quat_to_axis(const quat_t q, vec3_t axis[3])
{
	mat4_t tmp;
	Matrix4FromQuaternion(tmp, q);
	MatrixToVectorsFLU(tmp, axis[0], axis[1], axis[2]);
}
#endif

/**
 * @brief quat_norm
 * @param[in,out] q
 * @return
 */
vec_t quat_norm(quat_t q)
{
#ifndef ETL_SSE
	float length = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	length = (float)sqrt((double)length);
	if (length != 0.f)
	{
		float ilength = 1.0f / length;
		q[0] *= ilength;
		q[1] *= ilength;
		q[2] *= ilength;
		q[3] *= ilength;
	}
	return length;
#else
	__m128 xmm0, xmm1, xmm6, xmm7;
	float s;
	xmm1 = _mm_loadu_ps(&q[0]);
	xmm0 = xmm1;
	xmm0 = _mm_mul_ps(xmm0, xmm0);
	//xmm0 = _mm_hadd_ps(xmm0, xmm0);
	//xmm0 = _mm_hadd_ps(xmm0, xmm0); // xmm0 = length
	xmm7 = _mm_movehdup_ps(xmm1);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm1, xmm7);		//
	xmm7 = _mm_movehl_ps(xmm7, xmm6);	//
	xmm1 = _mm_add_ss(xmm6, xmm7);		//
	xmm0 = _mm_sqrt_ss(xmm0);
	_mm_store_ss(&s, xmm0); // function result
	if (s != 0.0) {
		xmm0 = _mm_rcp_ss(xmm0); // xmm0 = 1/length
		xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0);
		xmm1 = _mm_mul_ps(xmm1, xmm0);
		_mm_storeu_ps(&q[0], xmm1);
	}
	return s;
#endif
}

/**
 * @brief quat_slerp
 * @param[in] from
 * @param[in] to
 * @param[in] frac
 * @param[out] out
 */
void quat_slerp(const quat_t from, const quat_t to, float frac, quat_t out)
{
#if 0
	quat_t to1;
	double omega, cosom, sinom, scale0, scale1;

	cosom = from[0] * to[0] + from[1] * to[1] + from[2] * to[2] + from[3] * to[3];

	if (cosom < 0.0)
	{
		cosom = -cosom;

		QuatCopy(to, to1);
		QuatAntipodal(to1);
	}
	else
	{
		QuatCopy(to, to1);
	}

	if ((1.0 - cosom) > 0)
	{
		omega  = acos(cosom);
		sinom  = sin(omega);
		scale0 = sin((1.0 - frac) * omega) / sinom;
		scale1 = sin(frac * omega) / sinom;
	}
	else
	{
		scale0 = 1.0 - frac;
		scale1 = frac;
	}

	out[0] = scale0 * from[0] + scale1 * to1[0];
	out[1] = scale0 * from[1] + scale1 * to1[1];
	out[2] = scale0 * from[2] + scale1 * to1[2];
	out[3] = scale0 * from[3] + scale1 * to1[3];
#else
	/*
	Slerping Clock Cycles
	February 27th 2005
	J.M.P. van Waveren

	http://www.intel.com/cd/ids/developer/asmo-na/eng/293747.htm
	*/
	float cosom, absCosom, scale0, scale1;

	if (frac <= 0.0f)
	{
		quat_copy(from, out);
		return;
	}

	if (frac >= 1.0f)
	{
		quat_copy(to, out);
		return;
	}

	if (quat_compare(from, to))
	{
		quat_copy(from, out);
		return;
	}

#ifndef ETL_SSE
	cosom    = from[0] * to[0] + from[1] * to[1] + from[2] * to[2] + from[3] * to[3];
#else
	{
		__m128 xmm0, xmm1, xmm6, xmm7;
		xmm0 = _mm_loadu_ps(&from[0]);
		xmm1 = _mm_loadu_ps(&to[0]);
		xmm0 = _mm_mul_ps(xmm0, xmm1);
		//xmm0 = _mm_hadd_ps(xmm0, xmm0);
		//xmm0 = _mm_hadd_ps(xmm0, xmm0);
		xmm7 = _mm_movehdup_ps(xmm0);		// faster way to do: 2 * hadd
		xmm6 = _mm_add_ps(xmm0, xmm7);		//
		xmm7 = _mm_movehl_ps(xmm7, xmm6);	//
		xmm0 = _mm_add_ss(xmm6, xmm7);		//
		_mm_store_ss(&cosom, xmm0);
	}
#endif
	absCosom = Q_fabs(cosom);

	if ((1.0f - absCosom) > 1e-6f)
	{
		float sinSqr = 1.0f - (absCosom * absCosom);
		float sinom = rcp((float)sqrt(sinSqr)); // float sinom  = 1.0f / sqrt(sinSqr)
		float omega  = atan2(sinSqr * sinom, absCosom);

		scale0 = sin((1.0f - frac) * omega) * sinom;
		scale1 = sin(frac * omega) * sinom;
	}
	else
	{
		scale0 = 1.0f - frac;
		scale1 = frac;
	}

	scale1 = (cosom >= 0.0f) ? scale1 : -scale1;
#ifndef ETL_SSE
	out[0] = scale0 * from[0] + scale1 * to[0];
	out[1] = scale0 * from[1] + scale1 * to[1];
	out[2] = scale0 * from[2] + scale1 * to[2];
	out[3] = scale0 * from[3] + scale1 * to[3];
#else
	{
		__m128 xmm0, xmm1, xmm2, xmm3;
		xmm0 = _mm_loadu_ps(&from[0]);
		xmm1 = _mm_loadu_ps(&to[0]);
		xmm2 = _mm_load_ps1(&scale0);
		xmm3 = _mm_load_ps1(&scale1);
		xmm0 = _mm_mul_ps(xmm0, xmm2); // scale0 * from
		xmm1 = _mm_mul_ps(xmm1, xmm3); // scale1 * to
		xmm0 = _mm_add_ps(xmm0, xmm1);
		_mm_storeu_ps(out, xmm0);
	}
#endif
#endif
}

/************************************************************************/
/* Matrix 4                                                             */
/************************************************************************/

/**
 * @brief mat4_compare
 * @param[in] a
 * @param[in] b
 * @return
 */
qboolean mat4_compare(const mat4_t a, const mat4_t b)
{
#ifndef ETL_SSE
	return (a[0] == b[0] && a[4] == b[4] && a[8] == b[8] && a[12] == b[12] &&
	        a[1] == b[1] && a[5] == b[5] && a[9] == b[9] && a[13] == b[13] &&
	        a[2] == b[2] && a[6] == b[6] && a[10] == b[10] && a[14] == b[14] &&
	        a[3] == b[3] && a[7] == b[7] && a[11] == b[11] && a[15] == b[15]);
#else
	__m128 xmm0, xmm1, xmm2, xmm3;
	xmm0 = _mm_cmpneq_ps(_mm_loadu_ps(&a[0]), _mm_loadu_ps(&b[0]));
	if (_mm_movemask_ps(xmm0)) return qfalse;

	xmm1 = _mm_cmpneq_ps(_mm_loadu_ps(&a[4]), _mm_loadu_ps(&b[4]));
	if (_mm_movemask_ps(xmm1)) return qfalse;

	xmm2 = _mm_cmpneq_ps(_mm_loadu_ps(&a[8]), _mm_loadu_ps(&b[8]));
	if (_mm_movemask_ps(xmm2)) return qfalse;

	xmm3 = _mm_cmpneq_ps(_mm_loadu_ps(&a[12]), _mm_loadu_ps(&b[12]));
	if (_mm_movemask_ps(xmm3)) return qfalse;

	return qtrue;
#endif
}

/**
 * @brief mat4_copy
 * @param[in] in
 * @param[out] out
 */
void mat4_copy(const mat4_t in, mat4_t out)
{
#if id386_sse && defined __GNUC__ && 0
	asm volatile
	(
	    "movups         (%%edx),        %%xmm0\n"
	    "movups         0x10(%%edx),    %%xmm1\n"
	    "movups         0x20(%%edx),    %%xmm2\n"
	    "movups         0x30(%%edx),    %%xmm3\n"

	    "movups         %%xmm0,         (%%eax)\n"
	    "movups         %%xmm1,         0x10(%%eax)\n"
	    "movups         %%xmm2,         0x20(%%eax)\n"
	    "movups         %%xmm3,         0x30(%%eax)\n"
		:
		: "a" (out), "d" (in)
		: "memory"
	);
#elif id386_3dnow && defined __GNUC__
	asm volatile
	(
	    "femms\n"
	    "movq           (%%edx),        %%mm0\n"
	    "movq           8(%%edx),       %%mm1\n"
	    "movq           16(%%edx),      %%mm2\n"
	    "movq           24(%%edx),      %%mm3\n"
	    "movq           32(%%edx),      %%mm4\n"
	    "movq           40(%%edx),      %%mm5\n"
	    "movq           48(%%edx),      %%mm6\n"
	    "movq           56(%%edx),      %%mm7\n"

	    "movq           %%mm0,          (%%eax)\n"
	    "movq           %%mm1,          8(%%eax)\n"
	    "movq           %%mm2,          16(%%eax)\n"
	    "movq           %%mm3,          24(%%eax)\n"
	    "movq           %%mm4,          32(%%eax)\n"
	    "movq           %%mm5,          40(%%eax)\n"
	    "movq           %%mm6,          48(%%eax)\n"
	    "movq           %%mm7,          56(%%eax)\n"
	    "femms\n"
		:
		: "a" (out), "d" (in)
		: "memory"
	);
#else
#ifndef ETL_SSE
	out[0] = in[0];       out[4] = in[4];       out[8] = in[8];       out[12] = in[12];
	out[1] = in[1];       out[5] = in[5];       out[9] = in[9];       out[13] = in[13];
	out[2] = in[2];       out[6] = in[6];       out[10] = in[10];     out[14] = in[14];
	out[3] = in[3];       out[7] = in[7];       out[11] = in[11];     out[15] = in[15];
#else
	_mm_storeu_ps(&out[0], _mm_loadu_ps(&in[0]));
	_mm_storeu_ps(&out[4], _mm_loadu_ps(&in[4]));
	_mm_storeu_ps(&out[8], _mm_loadu_ps(&in[8]));
	_mm_storeu_ps(&out[12], _mm_loadu_ps(&in[12]));
#endif
#endif
}

/**
 * @brief replacement for glOrtho
 * see glspec30.pdf chapter 2.12 Coordinate Transformations
 * @param[out] m
 * @param[in] left
 * @param[in] right
 * @param[in] bottom
 * @param[in] top
 * @param[in] nearvec
 * @param[in] farvec
 */
void MatrixOrthogonalProjection(mat4_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec)
{
	float _far_near = rcp(farvec - nearvec);
	float _top_bottom = rcp(top - bottom);
	float _right_left = rcp(right - left);

#ifndef ETL_SSE
	m[0] = 2.0f * _right_left;  m[4] = 0.0f;                m[8] = 0.0f;                  m[12] = -(right + left) * _right_left;
	m[1] = 0.0f;                m[5] = 2.0f * _top_bottom;  m[9] = 0.0f;                  m[13] = -(top + bottom) * _top_bottom;
	m[2] = 0.0f;                m[6] = 0.0f;                m[10] = -2.0f * _far_near;    m[14] = -(farvec + nearvec) * _far_near;
	m[3] = 0.0f;                m[7] = 0.0f;                m[11] = 0.0f;                 m[15] = 1.0f;
#else
	__m128 xmm0, xmm1, xmm2, xmm3;
	xmm0 = _mm_set_ps(0.0f, 0.0f,                            0.0f,                          2.0f * _right_left);
	xmm1 = _mm_set_ps(0.0f, 0.0f,                            2.0f * _top_bottom,            0.0f);
	xmm2 = _mm_set_ps(0.0f, -2.0f * _far_near,               0.0f,                          0.0f);
	xmm3 = _mm_set_ps(1.0f, -(farvec + nearvec) * _far_near, -(top + bottom) * _top_bottom, -(right + left) * _right_left);
	_mm_storeu_ps(&m[0], xmm0);
	_mm_storeu_ps(&m[4], xmm1);
	_mm_storeu_ps(&m[8], xmm2);
	_mm_storeu_ps(&m[12], xmm3);
#endif
}

/**
 * @brief mat4_transform_vec4
 * @param[in] m
 * @param[in] in
 * @param[out] out
 */
void mat4_transform_vec4(const mat4_t m, const vec4_t in, vec4_t out)
{
#if id386_sse
	//#error MatrixTransform4

	__m128 _t0, _t1, _t2, _x, _y, _z, _w, _m0, _m1, _m2, _m3;

	_m0 = _mm_loadu_ps(&m[0]);
	_m1 = _mm_loadu_ps(&m[4]);
	_m2 = _mm_loadu_ps(&m[8]);
	_m3 = _mm_loadu_ps(&m[12]);

	_t0 = _mm_loadu_ps(in);
	_x  = _mm_shuffle_ps(_t0, _t0, _MM_SHUFFLE(0, 0, 0, 0));
	_y  = _mm_shuffle_ps(_t0, _t0, _MM_SHUFFLE(1, 1, 1, 1));
	_z  = _mm_shuffle_ps(_t0, _t0, _MM_SHUFFLE(2, 2, 2, 2));
	_w  = _mm_shuffle_ps(_t0, _t0, _MM_SHUFFLE(3, 3, 3, 3));

	_t0 = _mm_mul_ps(_m3, _w);
	_t1 = _mm_mul_ps(_m2, _z);
	_t0 = _mm_add_ps(_t0, _t1);

	_t1 = _mm_mul_ps(_m1, _y);
	_t2 = _mm_mul_ps(_m0, _x);
	_t1 = _mm_add_ps(_t1, _t2);

	_t0 = _mm_add_ps(_t0, _t1);

	_mm_storeu_ps(out, _t0);
#else
#ifndef ETL_SSE
	out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12] * in[3];
	out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13] * in[3];
	out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14] * in[3];
	out[3] = m[3] * in[0] + m[7] * in[1] + m[11] * in[2] + m[15] * in[3];
#else
	__m128 _t0, _t1, _t2, _x, _y, _z, _w, _m0, _m1, _m2, _m3;
	_m0 = _mm_loadu_ps(&m[0]);
	_m1 = _mm_loadu_ps(&m[4]);
	_m2 = _mm_loadu_ps(&m[8]);
	_m3 = _mm_loadu_ps(&m[12]);
	_t0 = _mm_loadu_ps(in);
	_x = _mm_shuffle_ps(_t0, _t0, _MM_SHUFFLE(0, 0, 0, 0));
	_y = _mm_shuffle_ps(_t0, _t0, _MM_SHUFFLE(1, 1, 1, 1));
	_z = _mm_shuffle_ps(_t0, _t0, _MM_SHUFFLE(2, 2, 2, 2));
	_w = _mm_shuffle_ps(_t0, _t0, _MM_SHUFFLE(3, 3, 3, 3));
	_t0 = _mm_mul_ps(_m3, _w);
	_t1 = _mm_mul_ps(_m2, _z);
	_t0 = _mm_add_ps(_t0, _t1);
	_t1 = _mm_mul_ps(_m1, _y);
	_t2 = _mm_mul_ps(_m0, _x);
	_t1 = _mm_add_ps(_t1, _t2);
	_t0 = _mm_add_ps(_t0, _t1);
	_mm_storeu_ps(out, _t0);
#endif
#endif
}

/**
 * @brief mat4_reset_translate
 * @param[out] m
 * @param[in] x
 * @param[in] y
 * @param[in] z
 */
void mat4_reset_translate(mat4_t m, vec_t x, vec_t y, vec_t z)
{
#ifndef ETL_SSE
	m[0] = 1;      m[4] = 0;      m[8] = 0;      m[12] = x;
	m[1] = 0;      m[5] = 1;      m[9] = 0;      m[13] = y;
	m[2] = 0;      m[6] = 0;      m[10] = 1;     m[14] = z;
	m[3] = 0;      m[7] = 0;      m[11] = 0;     m[15] = 1;
#else
	__m128 xmm0, xmm1, xmm2, xmm3;
	xmm0 = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);		// xmm0 =  m3  m2  m1  m0	(0.0f, 0.0f, 0.0f, 1.0f)
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b11110011);	// xmm1 =  m7  m6  m5  m4	(0.0f, 0.0f, 1.0f, 0.0f)
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b11001111);	// xmm2 = m11 m10  m9  m8	(0.0f, 1.0f, 0.0f, 0.0f)
	xmm3 = _mm_set_ps(1.0f, z, y, x);				// xmm3 = m15 m14 m13 m12
	_mm_storeu_ps(&m[0], xmm0);
	_mm_storeu_ps(&m[4], xmm1);
	_mm_storeu_ps(&m[8], xmm2);
	_mm_storeu_ps(&m[12], xmm3);
#endif
}

/**
 * @brief mat4_reset_translate_vec3
 * @param[out] m
 * @param[in] position
 */
void mat4_reset_translate_vec3(mat4_t m, vec3_t position)
{
#ifndef ETL_SSE
	mat4_reset_translate(m, position[0], position[1], position[2]);
#else
	__m128 xmm0, xmm1, xmm2, xmm3;
	xmm0 = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);						// xmm0 =  m3  m2  m1  m0
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b11110011);					// xmm1 =  m7  m6  m5  m4	(0.0f, 0.0f, 1.0f, 0.0f)
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b11001111);					// xmm2 = m11 m10  m9  m8	(0.0f, 1.0f, 0.0f, 0.0f)
	xmm3 = _mm_set_ps(1.0f, position[2], position[1], position[0]);	// xmm3 = m15 m14 m13 m12
	_mm_storeu_ps(&m[0], xmm0);
	_mm_storeu_ps(&m[4], xmm1);
	_mm_storeu_ps(&m[8], xmm2);
	_mm_storeu_ps(&m[12], xmm3);
#endif
}

/**
 * @brief mat4_reset_scale
 * @param[out] m
 * @param[in] x
 * @param[in] y
 * @param[in] z
 */
void mat4_reset_scale(mat4_t m, vec_t x, vec_t y, vec_t z)
{
#ifndef ETL_SSE
	m[0] = x;      m[4] = 0;      m[8] = 0;      m[12] = 0;
	m[1] = 0;      m[5] = y;      m[9] = 0;      m[13] = 0;
	m[2] = 0;      m[6] = 0;      m[10] = z;     m[14] = 0;
	m[3] = 0;      m[7] = 0;      m[11] = 0;     m[15] = 1;
#else
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, x));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, y, 0.0f));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, z, 0.0f, 0.0f));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
#endif
}

/**
 * @brief mat4_mult
 * @param[in] a
 * @param[in] b
 * @param[out] out
 */
void mat4_mult(const mat4_t a, const mat4_t b, mat4_t out)
{
#ifdef id386_sse
	//#error MatrixMultiply
	int    i;
	__m128 _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7;

	_t4 = _mm_loadu_ps(&a[0]);
	_t5 = _mm_loadu_ps(&a[4]);
	_t6 = _mm_loadu_ps(&a[8]);
	_t7 = _mm_loadu_ps(&a[12]);

	for (i = 0; i < 4; i++)
	{
		_t0 = _mm_load1_ps(&b[i * 4 + 0]);
		_t0 = _mm_mul_ps(_t4, _t0);

		_t1 = _mm_load1_ps(&b[i * 4 + 1]);
		_t1 = _mm_mul_ps(_t5, _t1);

		_t2 = _mm_load1_ps(&b[i * 4 + 2]);
		_t2 = _mm_mul_ps(_t6, _t2);

		_t3 = _mm_load1_ps(&b[i * 4 + 3]);
		_t3 = _mm_mul_ps(_t7, _t3);

		_t1 = _mm_add_ps(_t0, _t1);
		_t2 = _mm_add_ps(_t1, _t2);
		_t3 = _mm_add_ps(_t2, _t3);

		_mm_storeu_ps(&out[i * 4], _t3);
	}

#else
#ifndef ETL_SSE
	out[0] = b[0] * a[0] + b[1] * a[4] + b[2] * a[8] + b[3] * a[12];
	out[1] = b[0] * a[1] + b[1] * a[5] + b[2] * a[9] + b[3] * a[13];
	out[2] = b[0] * a[2] + b[1] * a[6] + b[2] * a[10] + b[3] * a[14];
	out[3] = b[0] * a[3] + b[1] * a[7] + b[2] * a[11] + b[3] * a[15];

	out[4] = b[4] * a[0] + b[5] * a[4] + b[6] * a[8] + b[7] * a[12];
	out[5] = b[4] * a[1] + b[5] * a[5] + b[6] * a[9] + b[7] * a[13];
	out[6] = b[4] * a[2] + b[5] * a[6] + b[6] * a[10] + b[7] * a[14];
	out[7] = b[4] * a[3] + b[5] * a[7] + b[6] * a[11] + b[7] * a[15];

	out[8]  = b[8] * a[0] + b[9] * a[4] + b[10] * a[8] + b[11] * a[12];
	out[9]  = b[8] * a[1] + b[9] * a[5] + b[10] * a[9] + b[11] * a[13];
	out[10] = b[8] * a[2] + b[9] * a[6] + b[10] * a[10] + b[11] * a[14];
	out[11] = b[8] * a[3] + b[9] * a[7] + b[10] * a[11] + b[11] * a[15];

	out[12] = b[12] * a[0] + b[13] * a[4] + b[14] * a[8] + b[15] * a[12];
	out[13] = b[12] * a[1] + b[13] * a[5] + b[14] * a[9] + b[15] * a[13];
	out[14] = b[12] * a[2] + b[13] * a[6] + b[14] * a[10] + b[15] * a[14];
	out[15] = b[12] * a[3] + b[13] * a[7] + b[14] * a[11] + b[15] * a[15];
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	xmm4 = _mm_loadu_ps(&a[0]);						// xmm4 =  a0  a1  a2  a3
	xmm5 = _mm_loadu_ps(&a[4]);						// xmm5 =  a4  a5  a6  a7
	xmm6 = _mm_loadu_ps(&a[8]);						// xmm6 =  a8  a9 a10 a11
	xmm7 = _mm_loadu_ps(&a[12]);					// xmm7 = a12 a13 a14 a15

	xmm0 = _mm_loadu_ps(&b[0]);						// xmm0 = b0 b1 b2 b3
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b0 b0 b0 b0
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b1 b1 b1 b1
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b2 b2 b2 b2
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b3 b3 b3 b3
	xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b3*a12 b3*a13 b3*a14 b3*a15
	xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b2*a8  b2*a9 b2*a10 b2*a11
	xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =  b1*a4  b1*a5  b1*a6  b1*a7
	xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =  b0*a0  b0*a1  b0*a2  b0*a3
	xmm3 = _mm_add_ps(xmm3, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm3);
	_mm_storeu_ps(&out[0], xmm0);

	xmm0 = _mm_loadu_ps(&b[4]);						// xmm0 = b4 b5 b6 b7
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b4 b4 b4 b4
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b5 b5 b5 b5
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b6 b6 b6 b6
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b7 b7 b7 b7
	xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b7*a12 b7*a13 b7*a14 b7*a15
	xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b6*a8  b6*a9 b6*a10 b6*a11
	xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =  b5*a4  b5*a5  b5*a6  b5*a7
	xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =  b4*a0  b4*a1  b4*a2  b4*a3
	xmm3 = _mm_add_ps(xmm3, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm3);
	_mm_storeu_ps(&out[4], xmm0);

	xmm0 = _mm_loadu_ps(&b[8]);						// xmm0 = b8 b9 b10 b11
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b8 b8 b8 b8
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b9 b9 b9 b9
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b10 b10 b10 b10
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b11 b11 b11 b11
	xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b11*a12 b11*a13 b11*a14 b11*a15
	xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b10*a8  b10*a9 b10*a10 b10*a11
	xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =   b9*a4   b9*a5   b9*a6   b9*a7
	xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =   b8*a0   b8*a1   b8*a2   b8*a3
	xmm3 = _mm_add_ps(xmm3, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm3);
	_mm_storeu_ps(&out[8], xmm0);

	xmm0 = _mm_loadu_ps(&b[12]);					// xmm0 = b12 b13 b14 b15
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b12 b12 b12 b12
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b13 b13 b13 b13
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b14 b14 b14 b14
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b15 b15 b15 b15
	xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b15*a12 b15*a13 b15*a14 b15*a15
	xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b14*a8  b14*a9 b14*a10 b14*a11
	xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =  b13*a4  b13*a5  b13*a6  b13*a7
	xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =  b12*a0  b12*a1  b12*a2  b12*a3
	xmm3 = _mm_add_ps(xmm3, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm3);
	_mm_storeu_ps(&out[12], xmm0);
#endif
#endif
}

/**
 * @brief mat4_mult_self
 * @param[in,out] m
 * @param[in,out] m2
 */
void mat4_mult_self(mat4_t m, const mat4_t m2)
{
#ifndef ETL_SSE
	mat4_t tmp;
	mat4_copy(m, tmp);
	mat4_mult(tmp, m2, m);
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	xmm4 = _mm_loadu_ps(&m[0]);						// xmm4 =  a0  a1  a2  a3
	xmm5 = _mm_loadu_ps(&m[4]);						// xmm5 =  a4  a5  a6  a7
	xmm6 = _mm_loadu_ps(&m[8]);						// xmm6 =  a8  a9 a10 a11
	xmm7 = _mm_loadu_ps(&m[12]);					// xmm7 = a12 a13 a14 a15

	xmm0 = _mm_loadu_ps(&m2[0]);						// xmm0 = b0 b1 b2 b3
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b0 b0 b0 b0
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b1 b1 b1 b1
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b2 b2 b2 b2
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b3 b3 b3 b3
	xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b3*a12 b3*a13 b3*a14 b3*a15
	xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b2*a8  b2*a9 b2*a10 b2*a11
	xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =  b1*a4  b1*a5  b1*a6  b1*a7
	xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =  b0*a0  b0*a1  b0*a2  b0*a3
	xmm3 = _mm_add_ps(xmm3, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm3);
	_mm_storeu_ps(&m[0], xmm0);

	xmm0 = _mm_loadu_ps(&m2[4]);						// xmm0 = b4 b5 b6 b7
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b4 b4 b4 b4
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b5 b5 b5 b5
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b6 b6 b6 b6
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b7 b7 b7 b7
	xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b7*a12 b7*a13 b7*a14 b7*a15
	xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b6*a8  b6*a9 b6*a10 b6*a11
	xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =  b5*a4  b5*a5  b5*a6  b5*a7
	xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =  b4*a0  b4*a1  b4*a2  b4*a3
	xmm3 = _mm_add_ps(xmm3, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm3);
	_mm_storeu_ps(&m[4], xmm0);

	xmm0 = _mm_loadu_ps(&m2[8]);						// xmm0 = b8 b9 b10 b11
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b8 b8 b8 b8
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b9 b9 b9 b9
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b10 b10 b10 b10
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b11 b11 b11 b11
	xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b11*a12 b11*a13 b11*a14 b11*a15
	xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b10*a8  b10*a9 b10*a10 b10*a11
	xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =   b9*a4   b9*a5   b9*a6   b9*a7
	xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =   b8*a0   b8*a1   b8*a2   b8*a3
	xmm3 = _mm_add_ps(xmm3, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm3);
	_mm_storeu_ps(&m[8], xmm0);

	xmm0 = _mm_loadu_ps(&m2[12]);					// xmm0 = b12 b13 b14 b15
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b12 b12 b12 b12
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b13 b13 b13 b13
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b14 b14 b14 b14
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b15 b15 b15 b15
	xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b15*a12 b15*a13 b15*a14 b15*a15
	xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b14*a8  b14*a9 b14*a10 b14*a11
	xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =  b13*a4  b13*a5  b13*a6  b13*a7
	xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =  b12*a0  b12*a1  b12*a2  b12*a3
	xmm3 = _mm_add_ps(xmm3, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm3);
	_mm_storeu_ps(&m[12], xmm0);
#endif
}

/**
 * @brief mat4_ident
 * @param[in] m
 */
void mat4_ident(mat4_t m)
{
#ifndef ETL_SSE
	m[0] = 1;      m[4] = 0;      m[8] = 0;      m[12] = 0;
	m[1] = 0;      m[5] = 1;      m[9] = 0;      m[13] = 0;
	m[2] = 0;      m[6] = 0;      m[10] = 1;     m[14] = 0;
	m[3] = 0;      m[7] = 0;      m[11] = 0;     m[15] = 1;
#else
	__m128 xmm0, xmm1, xmm2, xmm3;
	xmm0 = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);		// (0.0f, 0.0f, 0.0f, 1.0f)
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b11110011);	// (0.0f, 0.0f, 1.0f, 0.0f)
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b11001111);	// (0.0f, 1.0f, 0.0f, 0.0f)
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b00111111);	// (1.0f, 0.0f, 0.0f, 0.0f)
	_mm_storeu_ps(&m[0], xmm0);
	_mm_storeu_ps(&m[4], xmm1);
	_mm_storeu_ps(&m[8], xmm2);
	_mm_storeu_ps(&m[12], xmm3);
#endif
}

/**
 * @brief mat4_transform_vec3
 * @param[in] m
 * @param[in] in
 * @param[out] out
 */
void mat4_transform_vec3(const mat4_t m, const vec3_t in, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12];
	out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13];
	out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14];
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6;
	xmm1 = _mm_loadh_pi(_mm_load_ss(&in[0]), (const __m64 *)(&in[1]));	// xmm1 = z y _ x
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111);		// xmm2 = z z z z
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010);		// xmm1 = y y y y
	xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b00000000);		// xmm0 = x x x x
	xmm3 = _mm_loadu_ps(&m[0]);							// xmm3 =  m3  m2  m1  m0
	xmm4 = _mm_loadu_ps(&m[4]);							// xmm4 =  m7  m6  m5  m4
	xmm5 = _mm_loadu_ps(&m[8]);							// xmm5 = m11 m10  m9  m8
	xmm6 = _mm_loadu_ps(&m[12]);						// xmm6 = m15 m14 m13 m12
	xmm0 = _mm_mul_ps(xmm0, xmm3);
	xmm1 = _mm_mul_ps(xmm1, xmm4);
	xmm2 = _mm_mul_ps(xmm2, xmm5);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm6);
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b10010100);
	_mm_store_ss(&out[0], xmm0);
	_mm_storeh_pi((__m64 *)(&out[1]), xmm0);
#endif
}

/**
 * @brief mat4_transform_vec3_self
 * @param[in] m
 * @param[in,out] inout
 *
 * @note Unused
 * /
void mat4_transform_vec3_self(const mat4_t m, vec3_t inout)
{
#if 0
	vec3_t tmp;
	tmp[0] = m[0] * inout[0] + m[4] * inout[1] + m[8] * inout[2] + m[12];
	tmp[1] = m[1] * inout[0] + m[5] * inout[1] + m[9] * inout[2] + m[13];
	tmp[2] = m[2] * inout[0] + m[6] * inout[1] + m[10] * inout[2] + m[14];
	VectorCopy(tmp, inout);
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6;
	xmm0 = _mm_load_ss(&inout[0]);							// xmm0 = _ _ _ x
	xmm1 = _mm_loadh_pi(xmm0, (const __m64 *)(&inout[1]));	// xmm1 = z y _ _
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111);			// xmm2 = z z z z
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010);			// xmm1 = y y y y
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);			// xmm0 = x x x x

	xmm3 = _mm_loadu_ps(&m[0]);								// xmm3 =  m0  m1  m2  m3
	xmm4 = _mm_loadu_ps(&m[4]);								// xmm4 =  m4  m5  m6  m7
	xmm5 = _mm_loadu_ps(&m[8]);								// xmm5 =  m8  m9 m10 m11
	xmm6 = _mm_loadu_ps(&m[12]);							// xmm6 = m12 m13 m14 m15

	xmm0 = _mm_mul_ps(xmm0, xmm3);
	xmm1 = _mm_mul_ps(xmm1, xmm4);
	xmm2 = _mm_mul_ps(xmm2, xmm5);

	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm2);
	xmm0 = _mm_add_ps(xmm0, xmm6);

	_mm_store_ss(&inout[2], xmm0);
	_mm_storeh_pi((__m64 *)(&inout[0]), xmm0);
#endif
}
*/

/**
 * @brief mat4_transpose
 * @param[in] in
 * @param[out] out
 */
void mat4_transpose(const mat4_t in, mat4_t out)
{
#if id386_sse && defined __GNUC__ && 0
	// transpose the matrix into the xmm4-7
	MatrixTransposeIntoXMM(in);
	asm volatile
	(
	    "movups         %%xmm4,         (%%eax)\n"
	    "movups         %%xmm5,         0x10(%%eax)\n"
	    "movups         %%xmm6,         0x20(%%eax)\n"
	    "movups         %%xmm7,         0x30(%%eax)\n"
		:
		: "a" (out)
		: "memory"
	);
#else
#ifndef ETL_SSE
	out[0]  = in[0];     out[1] = in[4];      out[2] = in[8];       out[3] = in[12];
	out[4]  = in[1];     out[5] = in[5];      out[6] = in[9];       out[7] = in[13];
	out[8]  = in[2];     out[9] = in[6];      out[10] = in[10];     out[11] = in[14];
	out[12] = in[3];     out[13] = in[7];     out[14] = in[11];     out[15] = in[15];
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	xmm0 = _mm_loadu_ps(&in[0]);							// xmm0 = m03 m02 m01 m00
	xmm1 = _mm_loadu_ps(&in[4]);							// xmm1 = m13 m12 m11 m10
	xmm2 = _mm_loadu_ps(&in[8]);							// xmm2 = m23 m22 m21 m20
	xmm3 = _mm_loadu_ps(&in[12]);							// xmm3 = m33 m32 m31 m30
	// row 0
	xmm4 = _mm_shuffle_ps(xmm1, xmm0, 0b11101110);			// xmm4 = m03 m02 m13 m12
	xmm5 = _mm_shuffle_ps(xmm3, xmm2, 0b11101011);			// xmm5 = m23 m22 m32 m33
	xmm6 = _mm_shuffle_ps(xmm5, xmm4, 0b11011100);			// xmm6 = m03 m13 m23 m33
	_mm_storeu_ps((float *)&out[0], xmm6);
	// row 1
	xmm7 = _mm_shuffle_ps(xmm5, xmm4, 0b10001001);			// xmm7 = m02 m12 m22 m32
	_mm_storeu_ps((float *)&out[4], xmm7);
	// row 2
	xmm4 = _mm_shuffle_ps(xmm1, xmm0, 0b01000100);			// xmm4 = m01 m00 m11 m10
	xmm5 = _mm_shuffle_ps(xmm3, xmm2, 0b01000001);			// xmm5 = m21 m20 m30 m31
	xmm6 = _mm_shuffle_ps(xmm5, xmm4, 0b11011100);			// xmm6 = m01 m11 m21 m31
	_mm_storeu_ps((float *)&out[8], xmm6);
	// row 3
	xmm7 = _mm_shuffle_ps(xmm5, xmm4, 0b10001001);			// xmm7 = m01 m11 m21 m31
	_mm_storeu_ps((float *)&out[12], xmm7);
#endif
#endif
}

/**
 * @brief mat4_from_quat
 * @param[out] m
 * @param[in] q
 */
void mat4_from_quat(mat4_t m, const quat_t q)
{
#if 1
#ifndef ETL_SSE
	/*
	From Quaternion to Matrix and Back
	February 27th 2005
	J.M.P. van Waveren

	http://www.intel.com/cd/ids/developer/asmo-na/eng/293748.htm
	*/
/*
	float x2, y2, z2; //w2;
	float yy2, xy2;
	float xz2, yz2, zz2;
	float wz2, wy2, wx2, xx2;

	x2 = q[0] + q[0];
	y2 = q[1] + q[1];
	z2 = q[2] + q[2];
	//w2 = q[3] + q[3];

	yy2 = q[1] * y2;
	xy2 = q[0] * y2;

	xz2 = q[0] * z2;
	yz2 = q[1] * z2;
	zz2 = q[2] * z2;

	wz2 = q[3] * z2;
	wy2 = q[3] * y2;
	wx2 = q[3] * x2;
	xx2 = q[0] * x2;

	m[0] = -yy2 - zz2 + 1.0f;
	m[1] = xy2 + wz2;
	m[2] = xz2 - wy2;

	m[4] = xy2 - wz2;
	m[5] = -xx2 - zz2 + 1.0f;
	m[6] = yz2 + wx2;

	m[8]  = xz2 + wy2;
	m[9]  = yz2 - wx2;
	m[10] = -xx2 - yy2 + 1.0f;

	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0;
	m[15] = 1;
*/
	vec3_t q2, qz2, qq2;
	vec2_t qy2;
	float xx2;

	VectorAdd(q, q, q2); // vec3, so q[3] remains untouched
	Vector2Scale(q, q2[1], qy2); // vec2
	VectorScale(q, q2[2], qz2); // vec3
	VectorScale(q2, q[3], qq2); // vec3
	xx2 = q[0] * q2[0];

	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.f;
	m[15] = 1.f;

	m[0] = -qy2[1] - qz2[2] + 1.0f;
	m[1] = qy2[0] + qq2[2];
	m[2] = qz2[0] - qq2[1];

	m[4] = qy2[0] - qq2[2];
	m[5] = -xx2 - qz2[2] + 1.0f;
	m[6] = qz2[1] + qq2[0];

	m[8] = qz2[0] + qq2[1];
	m[9] = qz2[1] - qq2[0];
	m[10] = -xx2 - qy2[1] + 1.0f;
#else
	vec4_t q2, qz2, qq2, qy2;
	float xx2;
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4;
	xmm0 = _mm_loadu_ps(&q[0]);						//q		// xmm0 = q3 q2 q1 q0
	xmm1 = _mm_add_ps(xmm0, xmm0);					//q2	// xmm1 = q * 2
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b01010101);			// xmm2 = q2[1] q2[1] q2[1] q2[1]
	xmm2 = _mm_mul_ps(xmm2, xmm0);					//qy2	// xmm2 = q * q2[1]
	xmm3 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010);			// xmm2 = q2[2] q2[2] q2[2] q2[2]
	xmm3 = _mm_mul_ps(xmm3, xmm0);					//qz2	// xmm2 = q * q2[2]
	xmm4 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111);			// xmm2 = q2[3] q2[3] q2[3] q2[3]
	xmm4 = _mm_mul_ps(xmm4, xmm1);					//qq2	// xmm2 = q2 * q2[3]
	_mm_storeu_ps((float *)&q2, xmm1);
	_mm_storeu_ps((float *)&qy2, xmm2);
	_mm_storeu_ps((float *)&qz2, xmm3);
	_mm_storeu_ps((float *)&qq2, xmm4);
	xx2 = q[0] * q2[0];
/*
	m[0] = -qy2[1] - qz2[2] + 1.0f;
	m[1] = qy2[0] + qq2[2];
	m[2] = qz2[0] - qq2[1];
	m[3] = 0.0f;

	m[4] = qy2[0] - qq2[2];
	m[5] = -xx2 - qz2[2] + 1.0f;
	m[6] = qz2[1] + qq2[0];
	m[7] = 0.0f;

	m[8] = qz2[0] + qq2[1];
	m[9] = qz2[1] - qq2[0];
	m[10] = -xx2 - qy2[1] + 1.0f;
	m[11] = 0.0f;

	m[12] = m[13] = m[14] = 0.0f;
	m[15] = 1.0f;
*/
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, qz2[0] - qq2[1],        qy2[0] + qq2[2],        -qy2[1] - qz2[2] + 1.0f));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, qz2[1] + qq2[0],        -xx2 - qz2[2] + 1.0f,   qy2[0] - qq2[2]));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, -xx2 - qy2[1] + 1.0f,   qz2[1] - qq2[0],        qz2[0] + qq2[1]));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
#endif
#else
	/*
	http://www.gamedev.net/reference/articles/article1691.asp#Q54
	Q54. How do I convert a quaternion to a rotation matrix?

	Assuming that a quaternion has been created in the form:

	Q = |X Y Z W|

	Then the quaternion can then be converted into a 4x4 rotation
	matrix using the following expression (Warning: you might have to
	transpose this matrix if you (do not) follow the OpenGL order!):

	    ?        2     2                                      ?
	    ? 1 - (2Y  + 2Z )   2XY - 2ZW         2XZ + 2YW       ?
	    ?                                                     ?
	    ?                          2     2                    ?
	M = ? 2XY + 2ZW         1 - (2X  + 2Z )   2YZ - 2XW       ?
	    ?                                                     ?
	    ?                                            2     2  ?
	    ? 2XZ - 2YW         2YZ + 2XW         1 - (2X  + 2Y ) ?
	    ?                                                     ?
	*/

	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm

	float xx, xy, xz, xw, yy, yz, yw, zz, zw;

	xx = q[0] * q[0];
	xy = q[0] * q[1];
	xz = q[0] * q[2];
	xw = q[0] * q[3];
	yy = q[1] * q[1];
	yz = q[1] * q[2];
	yw = q[1] * q[3];
	zz = q[2] * q[2];
	zw = q[2] * q[3];

	m[0]  = 1 - 2 * (yy + zz);
	m[1]  = 2 * (xy + zw);
	m[2]  = 2 * (xz - yw);
	m[4]  = 2 * (xy - zw);
	m[5]  = 1 - 2 * (xx + zz);
	m[6]  = 2 * (yz + xw);
	m[8]  = 2 * (xz + yw);
	m[9]  = 2 * (yz - xw);
	m[10] = 1 - 2 * (xx + yy);

	m[3]  = m[7] = m[11] = m[12] = m[13] = m[14] = 0;
	m[15] = 1;
#endif
}

#ifndef ETL_SSE
/**
 * @brief MatrixFromVectorsFLU
 * @param[out] m
 * @param[in] forward
 * @param[in] left
 * @param[in] up
 */
void MatrixFromVectorsFLU(mat4_t m, const vec3_t forward, const vec3_t left, const vec3_t up)
{
#ifndef ETL_SSE
	m[0] = forward[0];     m[4] = left[0];        m[8] = up[0];   m[12] = 0;
	m[1] = forward[1];     m[5] = left[1];        m[9] = up[1];   m[13] = 0;
	m[2] = forward[2];     m[6] = left[2];        m[10] = up[2];  m[14] = 0;
	m[3] = 0;              m[7] = 0;              m[11] = 0;      m[15] = 1;
#else
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, forward[2], forward[1], forward[0]));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, left[2], left[1], left[0]));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, up[2], up[1], up[0]));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
#endif
}
#endif

#ifndef ETL_SSE
/**
 * @brief MatrixSetupTransformFromVectorsFLU
 * @param[in] m
 * @param[out] forward
 * @param[out] left
 * @param[out] up
 * @param[out] origin
 */
void MatrixSetupTransformFromVectorsFLU(mat4_t m, const vec3_t forward, const vec3_t left, const vec3_t up, const vec3_t origin)
{
#ifndef ETL_SSE
	m[0] = forward[0];     m[4] = left[0];        m[8] = up[0];   m[12] = origin[0];
	m[1] = forward[1];     m[5] = left[1];        m[9] = up[1];   m[13] = origin[1];
	m[2] = forward[2];     m[6] = left[2];        m[10] = up[2];  m[14] = origin[2];
	m[3] = 0;              m[7] = 0;              m[11] = 0;      m[15] = 1;
#else
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, forward[2], forward[1], forward[0]));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, left[2], left[1], left[0]));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, up[2], up[1], up[0]));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, origin[2], origin[1], origin[0]));
#endif
}
#endif

#ifndef ETL_SSE
/**
 * @brief MatrixToVectorsFLU
 * @param[in] m
 * @param[out] forward
 * @param[out] left
 * @param[out] up
 */
void MatrixToVectorsFLU(const mat4_t m, vec3_t forward, vec3_t left, vec3_t up)
{
	if (forward)
	{
		//forward[0] = m[0];      // cp*cy;
		//forward[1] = m[1];      // cp*sy;
		//forward[2] = m[2];      //-sp;
		VectorCopy(&m[0], forward);
	}

	if (left)
	{
		//left[0] = m[4];         // sr*sp*cy+cr*-sy;
		//left[1] = m[5];         // sr*sp*sy+cr*cy;
		//left[2] = m[6];         // sr*cp;
		VectorCopy(&m[4], left);
	}

	if (up)
	{
		//up[0] = m[8];   // cr*sp*cy+-sr*-sy;
		//up[1] = m[9];   // cr*sp*sy+-sr*cy;
		//up[2] = m[10];  // cr*cp;
		VectorCopy(&m[8], up);
	}
}
#endif

/**
 * @brief MatrixSetupTransformFromVectorsFRU
 * @param[out] m
 * @param[in] forward
 * @param[in] right
 * @param[in] up
 * @param[in] origin
 *
 * @note Unused
 * /
void MatrixSetupTransformFromVectorsFRU(mat4_t m, const vec3_t forward, const vec3_t right, const vec3_t up, const vec3_t origin)
{
#ifndef ETL_SSE
	m[0] = forward[0];     m[4] = -right[0];        m[8] = up[0];   m[12] = origin[0];
	m[1] = forward[1];     m[5] = -right[1];        m[9] = up[1];   m[13] = origin[1];
	m[2] = forward[2];     m[6] = -right[2];        m[10] = up[2];  m[14] = origin[2];
	m[3] = 0;              m[7] = 0;                m[11] = 0;      m[15] = 1;
#else
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, forward[2], forward[1], forward[0]));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, -right[2], -right[1], -right[0]));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, up[2], up[1], up[0]));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, origin[2], origin[1], origin[0]));
#endif
}*/

#ifndef ETL_SSE
/**
 * @brief MatrixToVectorsFRU
 * @param[in] m
 * @param[out] forward
 * @param[out] right
 * @param[out] up
 */
void MatrixToVectorsFRU(const mat4_t m, vec3_t forward, vec3_t right, vec3_t up)
{
	if (forward)
	{
		/*forward[0] = m[0];
		forward[1] = m[1];
		forward[2] = m[2];*/
		VectorCopy(&m[0], forward);
	}

	if (right)
	{
		/*right[0] = -m[4];
		right[1] = -m[5];
		right[2] = -m[6];*/
		VectorCopy(&m[4], right);
		VectorInverse(right);
	}

	if (up)
	{
		/*up[0] = m[8];
		up[1] = m[9];
		up[2] = m[10];*/
		VectorCopy(&m[8], up);
	}
}
#endif

/**
 * @brief Based on gluInvertMatrix
 * @param[in] in
 * @param[out] out
 * @return
 */
qboolean mat4_inverse(const mat4_t in, mat4_t out)
{
#if 0
	vec_t inv[16], det;
	int   i;
	inv[0] = in[5] * in[10] * in[15] -
	         in[5] * in[11] * in[14] -
	         in[9] * in[6] * in[15] +
	         in[9] * in[7] * in[14] +
	         in[13] * in[6] * in[11] -
	         in[13] * in[7] * in[10];

	inv[4] = -in[4] * in[10] * in[15] +
	         in[4] * in[11] * in[14] +
	         in[8] * in[6] * in[15] -
	         in[8] * in[7] * in[14] -
	         in[12] * in[6] * in[11] +
	         in[12] * in[7] * in[10];

	inv[8] = in[4] * in[9] * in[15] -
	         in[4] * in[11] * in[13] -
	         in[8] * in[5] * in[15] +
	         in[8] * in[7] * in[13] +
	         in[12] * in[5] * in[11] -
	         in[12] * in[7] * in[9];

	inv[12] = -in[4] * in[9] * in[14] +
	          in[4] * in[10] * in[13] +
	          in[8] * in[5] * in[14] -
	          in[8] * in[6] * in[13] -
	          in[12] * in[5] * in[10] +
	          in[12] * in[6] * in[9];

	det = in[0] * inv[0] + in[1] * inv[4] + in[2] * inv[8] + in[3] * inv[12];

	if (det == 0.f)
	{
		return qfalse;
	}

	inv[1] = -in[1] * in[10] * in[15] +
	         in[1] * in[11] * in[14] +
	         in[9] * in[2] * in[15] -
	         in[9] * in[3] * in[14] -
	         in[13] * in[2] * in[11] +
	         in[13] * in[3] * in[10];

	inv[5] = in[0] * in[10] * in[15] -
	         in[0] * in[11] * in[14] -
	         in[8] * in[2] * in[15] +
	         in[8] * in[3] * in[14] +
	         in[12] * in[2] * in[11] -
	         in[12] * in[3] * in[10];

	inv[9] = -in[0] * in[9] * in[15] +
	         in[0] * in[11] * in[13] +
	         in[8] * in[1] * in[15] -
	         in[8] * in[3] * in[13] -
	         in[12] * in[1] * in[11] +
	         in[12] * in[3] * in[9];

	inv[13] = in[0] * in[9] * in[14] -
	          in[0] * in[10] * in[13] -
	          in[8] * in[1] * in[14] +
	          in[8] * in[2] * in[13] +
	          in[12] * in[1] * in[10] -
	          in[12] * in[2] * in[9];

	inv[2] = in[1] * in[6] * in[15] -
	         in[1] * in[7] * in[14] -
	         in[5] * in[2] * in[15] +
	         in[5] * in[3] * in[14] +
	         in[13] * in[2] * in[7] -
	         in[13] * in[3] * in[6];

	inv[6] = -in[0] * in[6] * in[15] +
	         in[0] * in[7] * in[14] +
	         in[4] * in[2] * in[15] -
	         in[4] * in[3] * in[14] -
	         in[12] * in[2] * in[7] +
	         in[12] * in[3] * in[6];

	inv[10] = in[0] * in[5] * in[15] -
	          in[0] * in[7] * in[13] -
	          in[4] * in[1] * in[15] +
	          in[4] * in[3] * in[13] +
	          in[12] * in[1] * in[7] -
	          in[12] * in[3] * in[5];

	inv[14] = -in[0] * in[5] * in[14] +
	          in[0] * in[6] * in[13] +
	          in[4] * in[1] * in[14] -
	          in[4] * in[2] * in[13] -
	          in[12] * in[1] * in[6] +
	          in[12] * in[2] * in[5];

	inv[3] = -in[1] * in[6] * in[11] +
	         in[1] * in[7] * in[10] +
	         in[5] * in[2] * in[11] -
	         in[5] * in[3] * in[10] -
	         in[9] * in[2] * in[7] +
	         in[9] * in[3] * in[6];

	inv[7] = in[0] * in[6] * in[11] -
	         in[0] * in[7] * in[10] -
	         in[4] * in[2] * in[11] +
	         in[4] * in[3] * in[10] +
	         in[8] * in[2] * in[7] -
	         in[8] * in[3] * in[6];

	inv[11] = -in[0] * in[5] * in[11] +
	          in[0] * in[7] * in[9] +
	          in[4] * in[1] * in[11] -
	          in[4] * in[3] * in[9] -
	          in[8] * in[1] * in[7] +
	          in[8] * in[3] * in[5];

	inv[15] = in[0] * in[5] * in[10] -
	          in[0] * in[6] * in[9] -
	          in[4] * in[1] * in[10] +
	          in[4] * in[2] * in[9] +
	          in[8] * in[1] * in[6] -
	          in[8] * in[2] * in[5];

	det = 1.0f / det;

	for (i = 0; i < 16; i++)
	{
		out[i] = inv[i] * det;
	}

	return qtrue;
#else
	mat4_t inv;
	float det;

	float i4_9 = in[4] * in[9],
			i4_13 = in[4] * in[13],
			i6_11 = in[6] * in[11],
			i6_15 = in[6] * in[15],
			i7_10 = in[7] * in[10],
			i7_14 = in[7] * in[14],
			i8_5 = in[8] * in[5],
			i8_13 = in[8] * in[13],
			i10_15 = in[10] * in[15],
			i11_14 = in[11] * in[14],
			i12_5 = in[12] * in[5],
			i12_9 = in[12] * in[9];

	inv[0]	 = in[5] * i10_15 -
				in[5] * i11_14 -
				in[9] * i6_15 +
				in[9] * i7_14 +
				in[13] * i6_11 -
				in[13] * i7_10;

	inv[4]	 = -in[4] * i10_15 +
				in[4] * i11_14 +
				in[8] * i6_15 -
				in[8] * i7_14 -
				in[12] * i6_11 +
				in[12] * i7_10;

	inv[8]	 = i4_9 * in[15] -
				i4_13 * in[11] -
				i8_5 * in[15] +
				i8_13 * in[7] +
				i12_5 * in[11] -
				i12_9 * in[7];

	inv[12]	 = -i4_9 * in[14] +
				i4_13 * in[10] +
				i8_5 * in[14] -
				i8_13 * in[6] -
				i12_5 * in[10] +
				i12_9 * in[6];

	det = in[0] * inv[0] + in[1] * inv[4] + in[2] * inv[8] + in[3] * inv[12];

	if (det == 0.f)
	{
		return qfalse;
	}

	float i0_5 = in[0] * in[5],
			i0_9 = in[0] * in[9],
			i0_13 = in[0] * in[13],
			i2_7 = in[2] * in[7],
			i2_11 = in[2] * in[11],
			i2_15 = in[2] * in[15],
			i3_6 = in[3] * in[6],
			i3_10 = in[3] * in[10],
			i3_14 = in[3] * in[14],
			i4_1 = in[4] * in[1],
			i4_2 = in[4] * in[2],
			i4_3 = in[4] * in[3],
			i8_1 = in[8] * in[1],
			i12_1 = in[12] * in[1];
	//
	inv[1] = -in[1] * i10_15 +
			in[1] * i11_14 +
			in[9] * i2_15 -
			in[9] * i3_14 -
			in[13] * i2_11 +
			in[13] * i3_10;

	inv[5] = in[0] * i10_15 -
			in[0] * i11_14 -
			in[8] * i2_15 +
			in[8] * i3_14 +
			in[12] * i2_11 -
			in[12] * i3_10;

	inv[9] = -i0_9 * in[15] +
			i0_13 * in[11] +
			i8_1 * in[15] -
			i8_13 * in[3] -
			i12_1 * in[11] +
			i12_9 * in[3];

	inv[13] = i0_9 * in[14] -
			i0_13 * in[10] -
			i8_1 * in[14] +
			i8_13 * in[2] +
			i12_1 * in[10] -
			i12_9 * in[2];
	//
	inv[2] = in[1] * i6_15 -
			in[1] * i7_14 -
			in[5] * i2_15 +
			in[5] * i3_14 +
			in[13] * i2_7 -
			in[13] * i3_6;

	inv[6] = -in[0] * i6_15 +
			in[0] * i7_14 +
			in[4] * i2_15 -
			in[4] * i3_14 -
			in[12] * i2_7 +
			in[12] * i3_6;

	inv[10] = i0_5 * in[15] -
			i0_13 * in[7] -
			i4_1 * in[15] +
			i4_3 * in[13] +
			i12_1 * in[7] -
			i12_5 * in[3];

	inv[14] = -i0_5 * in[14] +
			i0_13 * in[6] +
			i4_1 * in[14] -
			i4_2 * in[13] -
			i12_1 * in[6] +
			i12_5 * in[2];
	//
	inv[3] = -in[1] * i6_11 +
			in[1] * i7_10 +
			in[5] * i2_11 -
			in[5] * i3_10 -
			in[9] * i2_7 +
			in[9] * i3_6;

	inv[7] = in[0] * i6_11 -
			in[0] * i7_10 -
			in[4] * i2_11 +
			in[4] * i3_10 +
			in[8] * i2_7 -
			in[8] * i3_6;

	inv[11] = -i0_5 * in[11] +
			i0_9 * in[7] +
			i4_1 * in[11] -
			i4_3 * in[9] -
			i8_1 * in[7] +
			i8_5 * in[3];

	inv[15] = i0_5 * in[10] -
			i0_9 * in[6] -
			i4_1 * in[10] +
			i4_2 * in[9] +
			i8_1 * in[6] -
			i8_5 * in[2];

	RECIPROCAL(det); // det = 1.0f / det;
	Vector4Scale(&inv[0], det, &out[0]);
	Vector4Scale(&inv[4], det, &out[4]);
	Vector4Scale(&inv[8], det, &out[8]);
	Vector4Scale(&inv[12], det, &out[12]);

	return qtrue;
#endif
}

/**
 * @brief mat4_inverse_self
 * @param[in,out] matrix
 * @return
 */
qboolean mat4_inverse_self(mat4_t matrix)
{
	return mat4_inverse(matrix, matrix);
}

/**
 * @brief mat4_from_angles
 * @param[out] m
 * @param[in] pitch
 * @param[in] yaw
 * @param[in] roll
 */
void mat4_from_angles(mat4_t m, vec_t pitch, vec_t yaw, vec_t roll)
{
	float radp, rady, radr, srcy, srsy, crcy, crsy, sr, spp, sy, cr, cp, cy;

	radp = DEG2RAD(pitch);
	rady = DEG2RAD(yaw);
	radr = DEG2RAD(roll);

	///spp = sin(radp);
	///cp = cos(radp);
	SinCos(radp, spp, cp); // 'sp' is also the name of a CPU-register. We use 'spp' for asm not to get confused..

	if (rady == radp)
	{
		sy = spp;
		cy = cp;
	}
	else
	{
		///sy = sin(rady);
		///cy = cos(rady);
		SinCos(rady, sy, cy);
	}

	if (radr == radp)
	{
		sr = spp;
		cr = cp;
	}
	else
	{
		if (radr == rady)
		{
			sr = sy;
			cr = cy;
		}
		else {
			///sr = sin(radr);
			///cr = cos(radr);
			SinCos(radr, sr, cr);
		}
	}

	srcy = sr * cy;
	srsy = sr * sy;
	crcy = cr * cy;
	crsy = cr * sy;

	m[0] = cp * cy;   m[4] = (srcy * spp - crsy);    m[8] = (crcy * spp + srsy);    m[12] = 0.0f;
	m[1] = cp * sy;   m[5] = (srsy * spp + crcy);    m[9] = (crsy * spp - srcy);    m[13] = 0.0f;
	m[2] = -spp;      m[6] = sr * cp;                m[10] = cr * cp;               m[14] = 0.0f;
	m[3] = 0.0f;      m[7] = 0.0f;                   m[11] = 0.0f;                  m[15] = 1.0f;
}

// calculate the tangent-vectors and the binormal-vectors
// given 3 points of a triangle (the positions and the texture-coordinates of those points).
qboolean CalcTangentBinormal(const vec3_t pos0, const vec3_t pos1, const vec3_t pos2,
						const vec2_t texCoords0, const vec2_t texCoords1, const vec2_t texCoords2,
						vec3_t tangent0, vec3_t binormal0, vec3_t tangent1, vec3_t binormal1, vec3_t tangent2, vec3_t binormal2)
{
	float  bb, bb_1;
	vec2_t v1sub0, v2sub0, dvist, dv0subdvi, dv1subdvi, dv2subdvi;
	vec3_t bary, dv0b0, dv1b1, dv2b2;

	// barycentric basis
	Vector2Subtract(texCoords1, texCoords0, v1sub0); // 2 vectors in the tangentspace plane
	Vector2Subtract(texCoords2, texCoords0, v2sub0);
	bb = (v1sub0[0] * v2sub0[1]) - (v2sub0[0] * v1sub0[1]); // crossproduct
	if (Q_fabs(bb) < 0.00000001f)
	{
		return qfalse;
	}
	bb_1 = rcp(bb); // bb_1 = 1.0f / bb;

	// 0: calculate s tangent vector
	dvist[0] = texCoords0[0] + 10.0;
	dvist[1] = texCoords0[1];
	Vector2Subtract(texCoords0, dvist, dv0subdvi);
	Vector2Subtract(texCoords1, dvist, dv1subdvi);
	Vector2Subtract(texCoords2, dvist, dv2subdvi);
	bary[0] = ((dv1subdvi[0] * dv2subdvi[1]) - (dv2subdvi[0] * dv1subdvi[1])) * bb_1; // cross
	bary[1] = ((dv2subdvi[0] * dv0subdvi[1]) - (dv0subdvi[0] * dv2subdvi[1])) * bb_1;
	bary[2] = ((dv0subdvi[0] * dv1subdvi[1]) - (dv1subdvi[0] * dv0subdvi[1])) * bb_1;
	VectorScale(pos0, bary[0], dv0b0);
	VectorScale(pos1, bary[1], dv1b1);
	VectorScale(pos2, bary[2], dv2b2);
	VectorAdd(dv0b0, dv1b1, tangent0);
	VectorAdd(tangent0, dv2b2, tangent0);
	VectorSubtract(tangent0, pos0, tangent0);
	VectorNormalizeOnly(tangent0);
	// 0: calculate t tangent vector (the binormal)
	dvist[0] = texCoords0[0];
	dvist[1] = texCoords0[1] + 10.0;
	Vector2Subtract(texCoords0, dvist, dv0subdvi);
	Vector2Subtract(texCoords1, dvist, dv1subdvi);
	Vector2Subtract(texCoords2, dvist, dv2subdvi);
	bary[0] = ((dv1subdvi[0] * dv2subdvi[1]) - (dv2subdvi[0] * dv1subdvi[1])) * bb_1; // cross
	bary[1] = ((dv2subdvi[0] * dv0subdvi[1]) - (dv0subdvi[0] * dv2subdvi[1])) * bb_1;
	bary[2] = ((dv0subdvi[0] * dv1subdvi[1]) - (dv1subdvi[0] * dv0subdvi[1])) * bb_1;
	VectorScale(pos0, bary[0], dv0b0);
	VectorScale(pos1, bary[1], dv1b1);
	VectorScale(pos2, bary[2], dv2b2);
	VectorAdd(dv0b0, dv1b1, binormal0);
	VectorAdd(binormal0, dv2b2, binormal0);
	VectorSubtract(binormal0, pos0, binormal0);
	VectorNormalizeOnly(binormal0);

	// 1: calculate s tangent vector
	dvist[0] = texCoords1[0] + 10.0;
	dvist[1] = texCoords1[1];
	Vector2Subtract(texCoords0, dvist, dv0subdvi);
	Vector2Subtract(texCoords1, dvist, dv1subdvi);
	Vector2Subtract(texCoords2, dvist, dv2subdvi);
	bary[0] = ((dv1subdvi[0] * dv2subdvi[1]) - (dv2subdvi[0] * dv1subdvi[1])) * bb_1; // cross
	bary[1] = ((dv2subdvi[0] * dv0subdvi[1]) - (dv0subdvi[0] * dv2subdvi[1])) * bb_1;
	bary[2] = ((dv0subdvi[0] * dv1subdvi[1]) - (dv1subdvi[0] * dv0subdvi[1])) * bb_1;
	VectorScale(pos0, bary[0], dv0b0);
	VectorScale(pos1, bary[1], dv1b1);
	VectorScale(pos2, bary[2], dv2b2);
	VectorAdd(dv0b0, dv1b1, tangent1);
	VectorAdd(tangent1, dv2b2, tangent1);
	VectorSubtract(tangent1, pos1, tangent1);
	VectorNormalizeOnly(tangent1);
	// 1: calculate t tangent vector (the binormal)
	dvist[0] = texCoords1[0];
	dvist[1] = texCoords1[1] + 10.0;
	Vector2Subtract(texCoords0, dvist, dv0subdvi);
	Vector2Subtract(texCoords1, dvist, dv1subdvi);
	Vector2Subtract(texCoords2, dvist, dv2subdvi);
	bary[0] = ((dv1subdvi[0] * dv2subdvi[1]) - (dv2subdvi[0] * dv1subdvi[1])) * bb_1; // cross
	bary[1] = ((dv2subdvi[0] * dv0subdvi[1]) - (dv0subdvi[0] * dv2subdvi[1])) * bb_1;
	bary[2] = ((dv0subdvi[0] * dv1subdvi[1]) - (dv1subdvi[0] * dv0subdvi[1])) * bb_1;
	VectorScale(pos0, bary[0], dv0b0);
	VectorScale(pos1, bary[1], dv1b1);
	VectorScale(pos2, bary[2], dv2b2);
	VectorAdd(dv0b0, dv1b1, binormal1);
	VectorAdd(binormal1, dv2b2, binormal1);
	VectorSubtract(binormal1, pos1, binormal1);
	VectorNormalizeOnly(binormal1);

	// 2: calculate s tangent vector
	dvist[0] = texCoords2[0] + 10.0;
	dvist[1] = texCoords2[1];
	Vector2Subtract(texCoords0, dvist, dv0subdvi);
	Vector2Subtract(texCoords1, dvist, dv1subdvi);
	Vector2Subtract(texCoords2, dvist, dv2subdvi);
	bary[0] = ((dv1subdvi[0] * dv2subdvi[1]) - (dv2subdvi[0] * dv1subdvi[1])) * bb_1; // cross
	bary[1] = ((dv2subdvi[0] * dv0subdvi[1]) - (dv0subdvi[0] * dv2subdvi[1])) * bb_1;
	bary[2] = ((dv0subdvi[0] * dv1subdvi[1]) - (dv1subdvi[0] * dv0subdvi[1])) * bb_1;
	VectorScale(pos0, bary[0], dv0b0);
	VectorScale(pos1, bary[1], dv1b1);
	VectorScale(pos2, bary[2], dv2b2);
	VectorAdd(dv0b0, dv1b1, tangent2);
	VectorAdd(tangent2, dv2b2, tangent2);
	VectorSubtract(tangent2, pos2, tangent2);
	VectorNormalizeOnly(tangent2);
	// 2: calculate t tangent vector (the binormal)
	dvist[0] = texCoords2[0];
	dvist[1] = texCoords2[1] + 10.0;
	Vector2Subtract(texCoords0, dvist, dv0subdvi);
	Vector2Subtract(texCoords1, dvist, dv1subdvi);
	Vector2Subtract(texCoords2, dvist, dv2subdvi);
	bary[0] = ((dv1subdvi[0] * dv2subdvi[1]) - (dv2subdvi[0] * dv1subdvi[1])) * bb_1; // cross
	bary[1] = ((dv2subdvi[0] * dv0subdvi[1]) - (dv0subdvi[0] * dv2subdvi[1])) * bb_1;
	bary[2] = ((dv0subdvi[0] * dv1subdvi[1]) - (dv1subdvi[0] * dv0subdvi[1])) * bb_1;
	VectorScale(pos0, bary[0], dv0b0);
	VectorScale(pos1, bary[1], dv1b1);
	VectorScale(pos2, bary[2], dv2b2);
	VectorAdd(dv0b0, dv1b1, binormal2);
	VectorAdd(binormal2, dv2b2, binormal2);
	VectorSubtract(binormal2, pos2, binormal2);
	VectorNormalizeOnly(binormal2);

	return qtrue;
}

#pragma warning(default:4700)
