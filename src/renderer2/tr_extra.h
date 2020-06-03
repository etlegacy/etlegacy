/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
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
 * @file renderer2/tr_extra.h
 */

#ifndef TR_EXTRA_H
#define TR_EXTRA_H

#include "tr_local.h"

#define Q_max(a, b)      ((a) > (b) ? (a) : (b))
#define Q_min(a, b)      ((a) < (b) ? (a) : (b))
#define Q_bound(a, b, c) (Q_max(a, Q_min(b, c)))
#define Q_clamp(a, b, c) ((b) >= (c) ? (a) = (b) : (a) < (b) ? (a) = (b) : (a) > (c) ? (a) = (c) : (a))
#define Q_lerp(from, to, frac) (from + ((to - from) * frac))

#define BSP_VERSION_Q3      46
#define REF_HARD_LINKED 1

/**
 * @struct glconfig2_t
 * @brief
 */
typedef struct
{
	qboolean ARBTextureCompressionAvailable;

	int maxCubeMapTextureSize;

	qboolean occlusionQueryAvailable;
	int occlusionQueryBits;

	int maxVertexUniforms;
	//	int             maxVaryingFloats;
	int maxVertexAttribs;
	qboolean vboVertexSkinningAvailable;
	int maxVertexSkinningBones;

	qboolean texture3DAvailable;
	qboolean textureNPOTAvailable;

	qboolean drawBuffersAvailable;
	qboolean textureHalfFloatAvailable;
	qboolean textureFloatAvailable;
	int maxDrawBuffers;

	qboolean vertexArrayObjectAvailable;

	qboolean stencilWrapAvailable;

	float maxTextureAnisotropy;
	qboolean textureAnisotropyAvailable;

	qboolean framebufferObjectAvailable;
	int maxRenderbufferSize;
	int maxColorAttachments;
	qboolean framebufferPackedDepthStencilAvailable;
	qboolean framebufferBlitAvailable;

	qboolean generateMipmapAvailable;

	qboolean getProgramBinaryAvailable;
} glconfig2_t;

/**
 * @enum shadowingMode_t
 * @brief cg_shadows modes
 */
typedef enum
{
	SHADOWING_NONE,
	SHADOWING_BLOB,
	SHADOWING_ESM16,
	SHADOWING_ESM32,
	SHADOWING_VSM16,
	SHADOWING_VSM32,
	SHADOWING_EVSM32,
	//SHADOWING_PLANAR,
	SHADOWING_STENCIL,
} shadowingMode_t;

/**
 * @struct dgridPoint_t
 * @brief Light grid
 */
typedef struct
{
	byte ambient[3];
	byte directed[3];
	byte latLong[2];
} dgridPoint_t;

#define RDF_NOCUBEMAP       (1 << 7)    ///< Don't use cubemaps
#define RDF_NOBLOOM         (1 << 8)    ///< Disable bloom. useful for hud models

/**
 * @enum memInfo_t
 * @brief
 */
typedef enum
{
	MI_NONE,
	MI_NVX,
	MI_ATI
} memInfo_t;

/**
 * @enum textureCompressionRef_t
 * @brief
 */
typedef enum
{
	TCR_NONE = 0x0000,
	TCR_LATC = 0x0001,
	TCR_BPTC = 0x0002,
} textureCompressionRef_t;

extern mat4_t matrixIdentity;
extern quat_t quatIdentity;

/**
 * @enum planeSide_t
 * @brief Plane sides
 */
typedef enum
{
	SIDE_FRONT = 0,
	SIDE_BACK  = 1,
	SIDE_ON    = 2,
	SIDE_CROSS = 3
} planeSide_t;

int NearestPowerOfTwo(int val);

qboolean PlanesGetIntersectionPoint(const vec4_t plane1, const vec4_t plane2, const vec4_t plane3, vec3_t out);
void MatrixMultiplyScale(mat4_t m, vec_t x, vec_t y, vec_t z);
void MatrixSetupTransformFromRotation(mat4_t m, const mat4_t rot, const vec3_t origin);
void MatrixAffineInverse(const mat4_t in, mat4_t out);
void MatrixPerspectiveProjectionFovXYRH(mat4_t m, vec_t fovX, vec_t fovY, vec_t nearvec, vec_t farvec);
void MatrixLookAtRH(mat4_t m, const vec3_t eye, const vec3_t dir, const vec3_t up);
void PlaneIntersectRay(const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res);
void MatrixOrthogonalProjectionRH(mat4_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec);
void MatrixCrop(mat4_t m, const vec3_t mins, const vec3_t maxs);
void MatrixMultiplyTranslation(mat4_t m, vec_t x, vec_t y, vec_t z);
void MatrixMultiplyZRotation(mat4_t m, vec_t degrees);
void MatrixSetupZRotation(mat4_t m, vec_t degrees);
qboolean BoundsIntersect(const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2);
qboolean BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs, const vec3_t origin, vec_t radius);
qboolean BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs, const vec3_t origin);

void ZeroBounds(vec3_t mins, vec3_t maxs);
void MatrixSetupTransformFromQuat(mat4_t m, const quat_t quat, const vec3_t origin);

vec_t PlaneNormalize(vec4_t plane); ///< returns normal length

void MatrixTransformNormal(const mat4_t m, const vec3_t in, vec3_t out);
void MatrixTransformNormal2(const mat4_t m, vec3_t inout);

void MatrixTransformPlane(const mat4_t m, const vec4_t in, vec4_t out);
//void MatrixTransformPlane2(const mat4_t m, vec4_t inout);
void MatrixTransformBounds( const mat4_t m, const vec3_t mins, const vec3_t maxs, vec3_t omins, vec3_t omaxs );

void MatrixPerspectiveProjectionFovXYInfiniteRH(mat4_t m, vec_t fovX, vec_t fovY, vec_t nearvec);
void QuatTransformVector(const quat_t q, const vec3_t in, vec3_t out);
void MatrixSetupShear(mat4_t m, vec_t x, vec_t y);
void MatrixMultiplyShear(mat4_t m, vec_t x, vec_t y);

void MatrixFromPlanes(mat4_t m, const vec4_t left, const vec4_t right, const vec4_t bottom, const vec4_t top, const vec4_t nearvec, const vec4_t farvec);

void QuatMultiply0(quat_t qa, const quat_t qb);
void QuatMultiply1(const quat_t qa, const quat_t qb, quat_t qc);
void QuatMultiply2(const quat_t qa, const quat_t qb, quat_t qc);
void QuatMultiply3(const quat_t qa, const quat_t qb, quat_t qc);
void QuatMultiply4(const quat_t qa, const quat_t qb, quat_t qc);

#define Vector5Copy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3], (b)[4] = (a)[4])

qboolean Q_strreplace(char *dest, size_t destsize, const char *find, const char *replace);

#define DotProduct4(x, y)             ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2] + (x)[3] * (y)[3])

/**
 * @brief Vector4Compare
 * @param[in] v1
 * @param[in] v2
 * @return
 */
static ID_INLINE int Vector4Compare(const vec4_t v1, const vec4_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3])
	{
		return 0;
	}
	return 1;
}

/**
 * @brief Vector5Compare
 * @param[in] v1
 * @param[in] v2
 * @return
 */
static ID_INLINE int Vector5Compare(const vec5_t v1, const vec5_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3] || v1[4] != v2[4])
	{
		return 0;
	}
	return 1;
}

/**
 * @brief Q_recip
 * @param[in] in
 * @return
 */
static ID_INLINE float Q_recip(float in)
{
#if id386_3dnow && defined __GNUC__ && 0
	vec_t out;

	femms();
	asm volatile ("movd		(%%eax),	%%mm0\n""pfrcp		%%mm0,		%%mm1\n"        // (approx)
	                                       "pfrcpit1	%%mm1,		%%mm0\n"// (intermediate)
	                                       "pfrcpit2	%%mm1,		%%mm0\n"// (full 24-bit)
	              // out = mm0[low]
	                                       "movd		%%mm0,		(%%edx)\n": : "a" (&in), "d" (&out) : "memory");

	femms();
	return out;
#else
	return (float)(1.0f / in);
#endif
}

/**
 * @brief VectorToAngles
 * @param[in] value1
 * @param[out] angles
 */
static ID_INLINE void VectorToAngles(const vec3_t value1, vec3_t angles)
{
	vectoangles(value1, angles);
}

// XreaL quaternion math functions required by the renderer

/**
 * @brief VectorLerp
 * @param[in] from
 * @param[in] to
 * @param[in] frac
 * @param[out] out
 */
static ID_INLINE void VectorLerp(const vec3_t from, const vec3_t to, float frac, vec3_t out)
{
	out[0] = from[0] + ((to[0] - from[0]) * frac);
	out[1] = from[1] + ((to[1] - from[1]) * frac);
	out[2] = from[2] + ((to[2] - from[2]) * frac);
}

/**
 * @brief QuatClear
 * @param[out] q
 */
static ID_INLINE void QuatClear(quat_t q)
{
	q[0] = 0;
	q[1] = 0;
	q[2] = 0;
	q[3] = 1;
}

/**
 * @brief QuatCalcW
 * @param[in,out] q
 */
static ID_INLINE void QuatCalcW(quat_t q)
{
#if 1
	vec_t term = 1.0f - (q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);

	if (term < 0.0f)
	{
		q[3] = 0.0f;
	}
	else
	{
		q[3] = -sqrt(term);
	}
#else
	q[3] = sqrt(Q_fabs(1.0f - (q[0] * q[0] + q[1] * q[1] + q[2] * q[2])));
#endif
}

//=============================================================================

enum
{
	MEMSTREAM_SEEK_SET,
	MEMSTREAM_SEEK_CUR,
	MEMSTREAM_SEEK_END
};

enum
{
	MEMSTREAM_FLAGS_EOF = BIT(0),
	MEMSTREAM_FLAGS_ERR = BIT(1),
};

/**
 * @struct memStream_s
 * @typedef memStream_t
 * @brief Helper struct for reading binary file formats
 */
typedef struct memStream_s
{
	byte *buffer;
	int bufSize;
	byte *curPos;
	int flags;
} memStream_t;

memStream_t *AllocMemStream(byte *buffer, int bufSize);
void            FreeMemStream(memStream_t *s);
qboolean MemStreamRead(memStream_t *s, void *buffer, unsigned int len);
int             MemStreamGetC(memStream_t *s);
int             MemStreamGetLong(memStream_t *s);
int             MemStreamGetShort(memStream_t *s);
float           MemStreamGetFloat(memStream_t *s);

//=============================================

//void printBits(size_t const size, void const *const ptr);

#endif
