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

vec3_t vec3_origin = { 0, 0, 0 };
vec3_t axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

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
	return 2.0f * (Q_random(seed) - 0.5f);
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
	if (i > 127)
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

	if (i > 255)
	{
		i = 255;
	}

	return (byte)i;
}

/**
 * @brief ClampColor
 * @param[in,out] color
 */
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

/**
 * @brief DirToByte
 * @param[in] dir
 * @return
 *
 * @note This isn't a real cheap function to call!
 */
int DirToByte(vec3_t dir)
{
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
		d = DotProduct(dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best  = i;
		}
	}

	return best;
}

/**
 * @brief ByteToDir
 * @param[in] b
 * @param[out] dir
 */
void ByteToDir(int b, vec3_t dir)
{
	if (b < 0 || b >= NUMVERTEXNORMALS)
	{
		VectorCopy(vec3_origin, dir);
		return;
	}
	VectorCopy(bytedirs[b], dir);
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
unsigned ColorBytes4(float r, float g, float b, float a)
{
	unsigned i;

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

	VectorSubtract(b, a, d1);
	VectorSubtract(c, a, d2);
	vec3_cross(d2, d1, plane);
	if (vec3_norm(plane) == 0.f)
	{
		return qfalse;
	}

	plane[3] = DotProduct(a, plane);
	return qtrue;
}

/**
 * @brief RotatePoint
 * @param[out] point
 * @param[in] matrix
 */
void RotatePoint(vec3_t point, vec3_t matrix[3])
{
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

/**
 * @brief RotatePointAroundVector
 * @param[out] dst
 * @param[in] dir
 * @param[in] point
 * @param[in] degrees
 *
 * @note This is not implemented very well...
 */
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point,
                             float degrees)
{
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
	vec3_cross(vr, vf, vup);

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
	vec3_cross(axis[0], axis[1], axis[2]);
}

/**
 * @brief CreateRotationMatrix
 * @param[in] angles
 * @param[in,out] matrix
 */
void CreateRotationMatrix(const vec3_t angles, vec3_t matrix[3])
{
	angles_vectors(angles, matrix[0], matrix[1], matrix[2]);
	vec3_inv(matrix[1]);
}

/**
 * @brief vec3_to_angles
 * @param[in] value1
 * @param[out] angles
 */
void vec3_to_angles(const vec3_t value1, vec3_t angles)
{
	float yaw, pitch;

	if (value1[1] == 0.f && value1[0] == 0.f)
	{
		yaw = 0;
		if (value1[2] > 0)
		{
			pitch = 90;
		}
		else
		{
			pitch = 270;
		}
	}
	else
	{
		float forward;

		if (value1[0] != 0.f)
		{
			yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		}
		else if (value1[1] > 0)
		{
			yaw = 90;
		}
		else
		{
			yaw = 270;
		}
		if (yaw < 0)
		{
			yaw += 360;
		}

		forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch   = (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
		{
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW]   = yaw;
	angles[ROLL]  = 0;
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
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
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

/**
 * @brief ProjectPointOnPlane
 * @param[out] dst
 * @param[in] p
 * @param[in] normal
 */
void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
	float  d;
	vec3_t n;
	float  inv_denom;

	inv_denom = 1.0F / DotProduct(normal, normal);

	d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

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

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	vec3_norm(right);
	vec3_cross(right, forward, up);
}

/**
 * @brief vec3_rotate
 * @param[in] in
 * @param[in] matrix
 * @param[out] out
 */
void vec3_rotate(const vec3_t in, vec3_t matrix[3], vec3_t out)
{
	/*
	out[0] = DotProduct(in, matrix[0]);
	out[1] = DotProduct(in, matrix[1]);
	out[2] = DotProduct(in, matrix[2]);
	*/
	out[0] = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0];
	out[1] = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1];
	out[2] = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2];
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
	if (to - from > 180)
	{
		to -= 360;
	}
	if (to - from < -180)
	{
		to += 360;
	}

	return(from + frac * (to - from));
}

/**
 * @brief vec3_lerp
 * @param[in] start
 * @param[in] end
 * @param[in] frac
 * @param[out] out
 */
void vec3_lerp(vec3_t start, vec3_t end, float frac, vec3_t out)
{
	vec3_t dist;

	VectorSubtract(end, start, dist);
	VectorMA(start, frac, dist, out);
}

/**
 * @brief angle_sub
 * @param[in] a1
 * @param[in] a2
 * @return Always returns a value from -180 to 180
 */
float angle_sub(float a1, float a2)
{
	float a = a1 - a2;

	while (a > 180)
	{
		a -= 360;
	}
	while (a < -180)
	{
		a += 360;
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
 */
float angle_mod(float a)
{
	return((360.0f / 65536) * ((int)(a * (65536 / 360.0f)) & 65535));
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
 */
float angle_norm_360(float angle)
{
	return (360.0f / 65536) * ((int)(angle * (65536 / 360.0f)) & 65535);
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
		if (out->normal[j] < 0)
		{
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}

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
    float   dist1, dist2;
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
    dist1 = DotProduct (p->normal, corners[0]) - p->dist;
    dist2 = DotProduct (p->normal, corners[1]) - p->dist;
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
		dist1 = dist2 = 0;      // shut up compiler
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

#endif

/**
 * @brief RadiusFromBounds
 * @param[in] mins
 * @param[in] maxs
 * @return
 */
float RadiusFromBounds(const vec3_t mins, const vec3_t maxs)
{
	int    i;
	vec3_t corner;
	float  a, b;

	for (i = 0 ; i < 3 ; i++)
	{
		a         = Q_fabs(mins[i]);
		b         = Q_fabs(maxs[i]);
		corner[i] = a > b ? a : b;
	}

	return vec3_length(corner);
}

/**
 * @brief ClearBounds
 * @param[in,out] mins
 * @param[in,out] maxs
 */
void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

/**
 * @brief AddPointToBounds
 * @param[in] v
 * @param[in,out] mins
 * @param[in,out] maxs
 */
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs)
{
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
}

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

/**
 * @brief BoundsAdd
 * @param[in,out] mins
 * @param[in,out] maxs
 * @param[in] mins2
 * @param[in] maxs2
 */
void BoundsAdd(vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2)
{
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
}

/**
 * @brief vec3_compare
 * @param[in] v1
 * @param[in] v2
 * @return
 */
qboolean vec3_compare(const vec3_t v1, const vec3_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief vec3_norm
 * @param[in] v
 * @return
 */
vec_t vec3_norm(vec3_t v)
{
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
}

/**
 * @brief fast vector normalize routine that does not check to make sure
 * that length != 0, nor does it return length
 * @param[in,out] v
 */
void vec3_norm_fast(vec3_t v)
{
	float ilength;

	ilength = Q_rsqrt(DotProduct(v, v));

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

/**
 * @brief vec3_norm2
 * @param[in] v
 * @param[out] out
 * @return
 */
vec_t vec3_norm2(const vec3_t v, vec3_t out)
{
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

	length = (float)sqrt((double)length);

	if (length != 0.f)
	{
		float ilength = 1 / length;

		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
	}
	else
	{
		VectorClear(out);
	}

	return length;

}

/**
 * @brief _VectorMA
 * @param[in] veca
 * @param[in] scale
 * @param[in] vecb
 * @param[out] vecc
 */
void _VectorMA(const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

/**
 * @brief _DotProduct
 * @param[in] v1
 * @param[in] v2
 * @return
 */
vec_t _DotProduct(const vec3_t v1, const vec3_t v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

/**
 * @brief _VectorSubtract
 * @param[in] veca
 * @param[in] vecb
 * @param[out] out
 */
void _VectorSubtract(const vec3_t veca, const vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}

/**
 * @brief _VectorAdd
 * @param[in] veca
 * @param[in] vecb
 * @param[out] out
 */
void _VectorAdd(const vec3_t veca, const vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

/**
 * @brief _VectorCopy
 * @param[in] in
 * @param[out] out
 */
void _VectorCopy(const vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/**
 * @brief _VectorScale
 * @param[in] in
 * @param[in] scale
 * @param[out] out
 */
void _VectorScale(const vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

/**
 * @brief vec3_cross
 * @param[in] v1
 * @param[in] v2
 * @param[out] cross
 */
void vec3_cross(const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

/**
 * @brief vec3_length
 * @param[in] v
 * @return
 */
vec_t vec3_length(const vec3_t v)
{
	return (float)sqrt((double)(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
}

/**
 * @brief vec3_length_squared
 * @param[in] v
 * @return
 */
vec_t vec3_length_squared(const vec3_t v)
{
	return (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/**
 * @brief vec3_distance
 * @param[in] p1
 * @param[in] p2
 * @return
 */
vec_t vec3_distance(const vec3_t p1, const vec3_t p2)
{
	vec3_t v;

	VectorSubtract(p2, p1, v);
	return vec3_length(v);
}

/**
 * @brief vec3_distance_squared
 * @param[in] p1
 * @param[in] p2
 * @return
 */
vec_t vec3_distance_squared(const vec3_t p1, const vec3_t p2)
{
	vec3_t v;

	VectorSubtract(p2, p1, v);
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

/**
 * @brief vec3_inv
 * @param[in,out] v
 */
void vec3_inv(vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
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
void _MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

/**
 * @brief mat3_transpose
 * @param[in] matrix
 * @param[out] transpose
 */
void mat3_transpose(vec3_t matrix[3], vec3_t transpose[3])
{
#if 0
	int i, j;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			transpose[i][j] = matrix[j][i];
		}
	}
#else
	transpose[0][0] = matrix[0][0];
	transpose[0][1] = matrix[1][0];
	transpose[0][2] = matrix[2][0];
	transpose[1][0] = matrix[0][1];
	transpose[1][1] = matrix[1][1];
	transpose[1][2] = matrix[2][1];
	transpose[2][0] = matrix[0][2];
	transpose[2][1] = matrix[1][2];
	transpose[2][2] = matrix[2][2];
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
	float        angle;
	static float sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = (float)((double)angles[YAW] * (M_TAU_F / 360));
	sy    = (float)sin((double)angle);
	cy    = (float)cos((double)angle);

	angle = (float)((double)angles[PITCH] * (M_TAU_F / 360));
	sp    = (float)sin((double)angle);
	cp    = (float)cos((double)angle);

	angle = (float)((double)angles[ROLL] * (M_TAU_F / 360));
	sr    = (float)sin((double)angle);
	cr    = (float)cos((double)angle);

	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
		right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
		right[2] = -1 * sr * cp;
	}
	if (up)
	{
		up[0] = (cr * sp * cy + -sr * -sy);
		up[1] = (cr * sp * sy + -sr * cy);
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

	// find the smallest magnitude axially aligned vector
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (Q_fabs(src[i]) < minelem)
		{
			pos     = i;
			minelem = Q_fabs(src[i]);
		}
	}
	tempvec[0]   = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	// project the point onto the plane defined by src
	ProjectPointOnPlane(dst, tempvec, src);

	// normalize the result
	vec3_norm(dst);
}

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
	vec3_norm(v1);

	VectorSubtract(point, p2, v2);
	vec3_norm(v2);

	vec3_cross(v1, v2, up);
	vec3_norm(up);
}

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

	VectorSubtract(point, vStart, pVec);
	VectorSubtract(vEnd, vStart, vec);
	vec3_norm(vec);
	// project onto the directional vector for this segment
	VectorMA(vStart, DotProduct(pVec, vec), vec, vProj);
}

/**
 * @brief ProjectPointOntoVectorBounded
 * @param[in] point
 * @param[in] vStart
 * @param[in] vEnd
 * @param[out] vProj
 *
 * @note Unused
 *
 */
void ProjectPointOntoVectorBounded(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj)
{
	vec3_t pVec, vec;
	int    j;

	VectorSubtract(point, vStart, pVec);
	VectorSubtract(vEnd, vStart, vec);
	vec3_norm(vec);
	// project onto the directional vector for this segment
	VectorMA(vStart, DotProduct(pVec, vec), vec, vProj);
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
}

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
		if ((proj[j] > lp1[j] && proj[j] > lp2[j]) ||
		    (proj[j] < lp1[j] && proj[j] < lp2[j]))
		{
			break;
		}
	if (j < 3)
	{
		if (Q_fabs(proj[j] - lp1[j]) < Q_fabs(proj[j] - lp2[j]))
		{
			VectorSubtract(p, lp1, t);
		}
		else
		{
			VectorSubtract(p, lp2, t);
		}
		return vec3_length_squared(t);
	}
	VectorSubtract(p, proj, t);
	return vec3_length_squared(t);
}

/**
 * @brief DistanceFromVectorSquared
 * @param[in] p
 * @param[in] lp1
 * @param[in] lp2
 * @return
 *
 * @note Unused
 */
float DistanceFromVectorSquared(vec3_t p, vec3_t lp1, vec3_t lp2)
{
	vec3_t proj, t;

	ProjectPointOntoVector(p, lp1, lp2, proj);
	VectorSubtract(p, proj, t);
	return vec3_length_squared(t);
}

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
		yaw = 0;
	}
	else
	{
		if (vec[PITCH] != 0.f)
		{
			yaw = (float)(atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		}
		else if (vec[YAW] > 0)
		{
			yaw = 90;
		}
		else
		{
			yaw = 270;
		}
		if (yaw < 0)
		{
			yaw += 360;
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
	if (DotProduct(right, axisDefault[1]) < 0)
	{
		if (roll_angles[PITCH] < 0)
		{
			roll_angles[PITCH] = -90 + (-90 - roll_angles[PITCH]);
		}
		else
		{
			roll_angles[PITCH] = 90 + (90 - roll_angles[PITCH]);
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
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return vec3_length(dir);
}

/**
 * @brief vec3_dist_squared
 * @param[in] v1
 * @param[in] v2
 * @return
 */
float vec3_dist_squared(vec3_t v1, vec3_t v2)
{
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return vec3_length_squared(dir);
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
		return (float)M_PI;
	}
	if (angle < -M_PI)
	{
		return (float)M_PI;
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
	float t, s;

	if (m[0] + m[5] + m[10] > 0.0f)
	{
		t = m[0] + m[5] + m[10] + 1.0f;
		s = (1.0f / sqrtf(t)) * 0.5f;

		q[3] = s * t;
		q[2] = (m[1] - m[4]) * s;
		q[1] = (m[8] - m[2]) * s;
		q[0] = (m[6] - m[9]) * s;
	}
	else if (m[0] > m[5] && m[0] > m[10])
	{
		t = m[0] - m[5] - m[10] + 1.0f;
		s = (1.0f / sqrtf(t)) * 0.5f;

		q[0] = s * t;
		q[1] = (m[1] + m[4]) * s;
		q[2] = (m[8] + m[2]) * s;
		q[3] = (m[6] - m[9]) * s;
	}
	else if (m[5] > m[10])
	{
		t = -m[0] + m[5] - m[10] + 1.0f;
		s = (1.0f / sqrtf(t)) * 0.5f;

		q[1] = s * t;
		q[0] = (m[1] + m[4]) * s;
		q[3] = (m[8] - m[2]) * s;
		q[2] = (m[6] + m[9]) * s;
	}
	else
	{
		t = -m[0] - m[5] + m[10] + 1.0f;
		s = (1.0f / sqrtf(t)) * 0.5f;

		q[2] = s * t;
		q[3] = (m[1] - m[4]) * s;
		q[0] = (m[8] + m[2]) * s;
		q[1] = (m[6] + m[9]) * s;
	}

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

			q[0] = 0.25f * s;
			q[1] = (m[4] + m[1]) / s;
			q[2] = (m[8] + m[2]) / s;
			q[3] = (m[9] - m[6]) / s;
		}
		else if (m[5] > m[10])
		{
			// column 1
			float s = sqrt(1.0f + m[5] - m[0] - m[10]) * 2.0f;

			q[0] = (m[4] + m[1]) / s;
			q[1] = 0.25f * s;
			q[2] = (m[9] + m[6]) / s;
			q[3] = (m[8] - m[2]) / s;
		}
		else
		{
			// column 2
			float s = sqrt(1.0f + m[10] - m[0] - m[5]) * 2.0f;

			q[0] = (m[8] + m[2]) / s;
			q[1] = (m[9] + m[6]) / s;
			q[2] = 0.25f * s;
			q[3] = (m[4] - m[1]) / s;
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

	q[3] = (float)(sqrt((double)(1.0f + m[0][0] + m[1][1] + m[2][2])) / 2.0);
	w4   = q[3] * 4.0f;

	q[0] = (m[1][2] - m[2][1]) / w4;
	q[1] = (m[2][0] - m[0][2]) / w4;
	q[2] = (m[0][1] - m[1][0]) / w4;
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
 */
void quat_from_angles(quat_t q, vec_t pitch, vec_t yaw, vec_t roll)
{
#if 1
	mat4_t tmp;

	mat4_from_angles(tmp, pitch, yaw, roll);
	quat_from_mat4(q, tmp);
#else
	static float sr, sp, sy, cr, cp, cy;

	// static to help MS compiler fp bugs
	sp = sin(DEG2RAD(pitch));
	cp = cos(DEG2RAD(pitch));

	sy = sin(DEG2RAD(yaw));
	cy = cos(DEG2RAD(yaw));

	sr = sin(DEG2RAD(roll));
	cr = cos(DEG2RAD(roll));

	q[0] = sr * cp * cy - cr * sp * sy; // x
	q[1] = cr * sp * cy + sr * cp * sy; // y
	q[2] = cr * cp * sy - sr * sp * cy; // z
	q[3] = cr * cp * cy + sr * sp * sy; // w
#endif
}

/**
 * @brief quat_to_vec3_FLU
 * @param[in] q
 * @param[out] forward
 * @param[out] left
 * @param[out] up
 *
 * @note Unused
 */
void quat_to_vec3_FLU(const quat_t q, vec3_t forward, vec3_t left, vec3_t up)
{
	mat4_t tmp;

	mat4_from_quat(tmp, q);
	MatrixToVectorsFRU(tmp, forward, left, up);
}

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

	mat4_from_quat(tmp, q);
	MatrixToVectorsFRU(tmp, forward, right, up);
}

/**
 * @brief quat_to_axis
 * @param[in] q
 * @param[out] axis
 */
void quat_to_axis(const quat_t q, vec3_t axis[3])
{
	mat4_t tmp;

	mat4_from_quat(tmp, q);
	MatrixToVectorsFLU(tmp, axis[0], axis[1], axis[2]);
}

/**
 * @brief quat_norm
 * @param[in,out] q
 * @return
 */
vec_t quat_norm(quat_t q)
{
	float length = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];

	length = (float)sqrt((double)length);

	if (length != 0.f)
	{
		float ilength = 1 / length;

		q[0] *= ilength;
		q[1] *= ilength;
		q[2] *= ilength;
		q[3] *= ilength;
	}

	return length;
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

	cosom    = from[0] * to[0] + from[1] * to[1] + from[2] * to[2] + from[3] * to[3];
	absCosom = Q_fabs(cosom);

	if ((1.0f - absCosom) > 1e-6f)
	{
		float sinSqr = 1.0f - absCosom * absCosom;
		float sinom  = 1.0 / sqrt(sinSqr);
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

	out[0] = scale0 * from[0] + scale1 * to[0];
	out[1] = scale0 * from[1] + scale1 * to[1];
	out[2] = scale0 * from[2] + scale1 * to[2];
	out[3] = scale0 * from[3] + scale1 * to[3];
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
	return (a[0] == b[0] && a[4] == b[4] && a[8] == b[8] && a[12] == b[12] &&
	        a[1] == b[1] && a[5] == b[5] && a[9] == b[9] && a[13] == b[13] &&
	        a[2] == b[2] && a[6] == b[6] && a[10] == b[10] && a[14] == b[14] &&
	        a[3] == b[3] && a[7] == b[7] && a[11] == b[11] && a[15] == b[15]);
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
	out[0] = in[0];       out[4] = in[4];       out[8] = in[8];       out[12] = in[12];
	out[1] = in[1];       out[5] = in[5];       out[9] = in[9];       out[13] = in[13];
	out[2] = in[2];       out[6] = in[6];       out[10] = in[10];       out[14] = in[14];
	out[3] = in[3];       out[7] = in[7];       out[11] = in[11];       out[15] = in[15];
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
	m[0] = 2 / (right - left);  m[4] = 0;                   m[8] = 0;                   m[12] = -(right + left) / (right - left);
	m[1] = 0;                   m[5] = 2 / (top - bottom);  m[9] = 0;                   m[13] = -(top + bottom) / (top - bottom);
	m[2] = 0;                   m[6] = 0;                   m[10] = -2 / (farvec - nearvec);    m[14] = -(farvec + nearvec) / (farvec - nearvec);
	m[3] = 0;                   m[7] = 0;                   m[11] = 0;                  m[15] = 1;
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
	out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12] * in[3];
	out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13] * in[3];
	out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14] * in[3];
	out[3] = m[3] * in[0] + m[7] * in[1] + m[11] * in[2] + m[15] * in[3];
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
	m[0] = 1;      m[4] = 0;      m[8] = 0;      m[12] = x;
	m[1] = 0;      m[5] = 1;      m[9] = 0;      m[13] = y;
	m[2] = 0;      m[6] = 0;      m[10] = 1;      m[14] = z;
	m[3] = 0;      m[7] = 0;      m[11] = 0;      m[15] = 1;
}

/**
 * @brief mat4_reset_translate_vec3
 * @param[out] m
 * @param[in] position
 */
void mat4_reset_translate_vec3(mat4_t m, vec3_t position)
{
	mat4_reset_translate(m, position[0], position[1], position[2]);
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
	m[0] = x;      m[4] = 0;      m[8] = 0;      m[12] = 0;
	m[1] = 0;      m[5] = y;      m[9] = 0;      m[13] = 0;
	m[2] = 0;      m[6] = 0;      m[10] = z;      m[14] = 0;
	m[3] = 0;      m[7] = 0;      m[11] = 0;      m[15] = 1;
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
#endif
}

/**
 * @brief mat4_mult_self
 * @param[in,out] m
 * @param[in,out] m2
 */
void mat4_mult_self(mat4_t m, const mat4_t m2)
{
	mat4_t tmp;

	mat4_copy(m, tmp);
	mat4_mult(tmp, m2, m);
}

/**
 * @brief mat4_ident
 * @param[in] m
 */
void mat4_ident(mat4_t m)
{
	m[0] = 1;      m[4] = 0;      m[8] = 0;      m[12] = 0;
	m[1] = 0;      m[5] = 1;      m[9] = 0;      m[13] = 0;
	m[2] = 0;      m[6] = 0;      m[10] = 1;      m[14] = 0;
	m[3] = 0;      m[7] = 0;      m[11] = 0;      m[15] = 1;
}

/**
 * @brief mat4_transform_vec3
 * @param[in] m
 * @param[in] in
 * @param[out] out
 */
void mat4_transform_vec3(const mat4_t m, const vec3_t in, vec3_t out)
{
	out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12];
	out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13];
	out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14];
}

/**
 * @brief mat4_transform_vec3_self
 * @param[in] m
 * @param[in,out] inout
 *
 * @note Unused
 */
void mat4_transform_vec3_self(const mat4_t m, vec3_t inout)
{
	vec3_t tmp;

	tmp[0] = m[0] * inout[0] + m[4] * inout[1] + m[8] * inout[2] + m[12];
	tmp[1] = m[1] * inout[0] + m[5] * inout[1] + m[9] * inout[2] + m[13];
	tmp[2] = m[2] * inout[0] + m[6] * inout[1] + m[10] * inout[2] + m[14];

	VectorCopy(tmp, inout);
}

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
	out[0]  = in[0];       out[1] = in[4];       out[2] = in[8];       out[3] = in[12];
	out[4]  = in[1];       out[5] = in[5];       out[6] = in[9];       out[7] = in[13];
	out[8]  = in[2];       out[9] = in[6];       out[10] = in[10];       out[11] = in[14];
	out[12] = in[3];       out[13] = in[7];       out[14] = in[11];       out[15] = in[15];
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
	/*
	From Quaternion to Matrix and Back
	February 27th 2005
	J.M.P. van Waveren

	http://www.intel.com/cd/ids/developer/asmo-na/eng/293748.htm
	*/
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

	m[3]  = m[7] = m[11] = m[12] = m[13] = m[14] = 0;
	m[15] = 1;

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

/**
 * @brief MatrixFromVectorsFLU
 * @param[out] m
 * @param[in] forward
 * @param[in] left
 * @param[in] up
 */
void MatrixFromVectorsFLU(mat4_t m, const vec3_t forward, const vec3_t left, const vec3_t up)
{
	m[0] = forward[0];     m[4] = left[0];        m[8] = up[0];  m[12] = 0;
	m[1] = forward[1];     m[5] = left[1];        m[9] = up[1];  m[13] = 0;
	m[2] = forward[2];     m[6] = left[2];        m[10] = up[2];  m[14] = 0;
	m[3] = 0;              m[7] = 0;              m[11] = 0;      m[15] = 1;
}

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
	m[0] = forward[0];     m[4] = left[0];        m[8] = up[0];  m[12] = origin[0];
	m[1] = forward[1];     m[5] = left[1];        m[9] = up[1];  m[13] = origin[1];
	m[2] = forward[2];     m[6] = left[2];        m[10] = up[2];  m[14] = origin[2];
	m[3] = 0;              m[7] = 0;              m[11] = 0;      m[15] = 1;
}

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
		forward[0] = m[0];      // cp*cy;
		forward[1] = m[1];      // cp*sy;
		forward[2] = m[2];      //-sp;
	}

	if (left)
	{
		left[0] = m[4];         // sr*sp*cy+cr*-sy;
		left[1] = m[5];         // sr*sp*sy+cr*cy;
		left[2] = m[6];         // sr*cp;
	}

	if (up)
	{
		up[0] = m[8];   // cr*sp*cy+-sr*-sy;
		up[1] = m[9];   // cr*sp*sy+-sr*cy;
		up[2] = m[10];  // cr*cp;
	}
}

/**
 * @brief MatrixSetupTransformFromVectorsFRU
 * @param[out] m
 * @param[in] forward
 * @param[in] right
 * @param[in] up
 * @param[in] origin
 *
 * @note Unused
 */
void MatrixSetupTransformFromVectorsFRU(mat4_t m, const vec3_t forward, const vec3_t right, const vec3_t up, const vec3_t origin)
{
	m[0] = forward[0];     m[4] = -right[0];        m[8] = up[0];  m[12] = origin[0];
	m[1] = forward[1];     m[5] = -right[1];        m[9] = up[1];  m[13] = origin[1];
	m[2] = forward[2];     m[6] = -right[2];        m[10] = up[2];  m[14] = origin[2];
	m[3] = 0;              m[7] = 0;                m[11] = 0;      m[15] = 1;
}

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
		forward[0] = m[0];
		forward[1] = m[1];
		forward[2] = m[2];
	}

	if (right)
	{
		right[0] = -m[4];
		right[1] = -m[5];
		right[2] = -m[6];
	}

	if (up)
	{
		up[0] = m[8];
		up[1] = m[9];
		up[2] = m[10];
	}
}

/**
 * @brief Based on gluInvertMatrix
 * @param[in] in
 * @param[out] out
 * @return
 */
qboolean mat4_inverse(const mat4_t in, mat4_t out)
{
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

	det = in[0] * inv[0] + in[1] * inv[4] + in[2] * inv[8] + in[3] * inv[12];

	if (det == 0.f)
	{
		return qfalse;
	}

	det = 1.0f / det;

	for (i = 0; i < 16; i++)
	{
		out[i] = inv[i] * det;
	}

	return qtrue;
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
	static float sr, sp, sy, cr, cp, cy;

	// static to help MS compiler fp bugs
	sp = (float)sin(DEG2RAD((double)pitch));
	cp = (float)cos(DEG2RAD((double)pitch));

	sy = (float)sin(DEG2RAD((double)yaw));
	cy = (float)cos(DEG2RAD((double)yaw));

	sr = (float)sin(DEG2RAD((double)roll));
	cr = (float)cos(DEG2RAD((double)roll));

	m[0] = cp * cy;  m[4] = (sr * sp * cy + cr * -sy);      m[8] = (cr * sp * cy + -sr * -sy);     m[12] = 0;
	m[1] = cp * sy;  m[5] = (sr * sp * sy + cr * cy);       m[9] = (cr * sp * sy + -sr * cy);      m[13] = 0;
	m[2] = -sp;    m[6] = sr * cp;                  m[10] = cr * cp;                  m[14] = 0;
	m[3] = 0;      m[7] = 0;                      m[11] = 0;                      m[15] = 1;
}
