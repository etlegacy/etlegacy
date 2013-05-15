/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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

#ifndef TR_EXTRA_H
#define TR_EXTRA_H

#include "tr_local.h"

#ifdef __cplusplus
extern "C" {
#endif

#define Q_max(a, b)      ((a) > (b) ? (a) : (b))
#define Q_min(a, b)      ((a) < (b) ? (a) : (b))
#define Q_bound(a, b, c) (Q_max(a, Q_min(b, c)))
#define Q_clamp(a, b, c) ((b) >= (c) ? (a) = (b) : (a) < (b) ? (a) = (b) : (a) > (c) ? (a) = (c) : (a))
#define Q_lerp(from, to, frac) (from + ((to - from) * frac))

#define BSP_VERSION_Q3      46
#define REF_HARD_LINKED 1

// XreaL BEGIN
typedef struct
{
	qboolean ARBTextureCompressionAvailable;

	int maxCubeMapTextureSize;

	qboolean occlusionQueryAvailable;
	int occlusionQueryBits;

	char shadingLanguageVersion[MAX_STRING_CHARS];

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
} glconfig2_t;
// XreaL END

// XreaL BEGIN

// cg_shadows modes
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
// XreaL END

// light grid
typedef struct
{
	byte ambient[3];
	byte directed[3];
	byte latLong[2];
} dgridPoint_t;

// XreaL BEGIN
#define RDF_NOCUBEMAP       (1 << 7)    // RB: don't use cubemaps
#define RDF_NOBLOOM         (1 << 8)    // RB: disable bloom. useful for hud models
// XreaL END

typedef enum
{
	MI_NONE,
	MI_NVX,
	MI_ATI
} memInfo_t;

typedef enum
{
	TCR_NONE = 0x0000,
	TCR_LATC = 0x0001,
	TCR_BPTC = 0x0002,
} textureCompressionRef_t;

// We can't change glConfig_t without breaking DLL/vms compatibility, so
// store extensions we have here.
typedef struct
{
	qboolean drawRangeElements;
	qboolean multiDrawArrays;
	qboolean occlusionQuery;

	int glslMajorVersion;
	int glslMinorVersion;

	memInfo_t memInfo;

	qboolean framebufferObject;
	int maxRenderbufferSize;
	int maxColorAttachments;

	qboolean multiTexture;
	qboolean textureNonPowerOfTwo;
	qboolean textureFloat;
	qboolean halfFloatPixel;
	qboolean packedDepthStencil;
	textureCompressionRef_t textureCompression;

	qboolean framebufferMultisample;
	qboolean framebufferBlit;

	qboolean texture_srgb;

	qboolean depthClamp;
} glRefConfig_t;

extern glRefConfig_t glRefConfig;

extern matrix_t matrixIdentity;
extern quat_t   quatIdentity;

// plane sides
typedef enum
{
	SIDE_FRONT = 0,
	SIDE_BACK  = 1,
	SIDE_ON    = 2,
	SIDE_CROSS = 3
} planeSide_t;

int NearestPowerOfTwo(int val);
qboolean MatrixCompare(const matrix_t a, const matrix_t b);
void MatrixCopy(const matrix_t in, matrix_t out);
void MatrixOrthogonalProjection(matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec);
void MatrixTransform4(const matrix_t m, const vec4_t in, vec4_t out);
void MatrixSetupTranslation(matrix_t m, vec_t x, vec_t y, vec_t z);
void MatrixSetupScale(matrix_t m, vec_t x, vec_t y, vec_t z);
void MatrixSetupTranslation(matrix_t m, vec_t x, vec_t y, vec_t z);
void MatrixMultiplyMOD(const matrix_t a, const matrix_t b, matrix_t out);
void MatrixMultiply2(matrix_t m, const matrix_t m2);
qboolean PlanesGetIntersectionPoint(const vec4_t plane1, const vec4_t plane2, const vec4_t plane3, vec3_t out);
void MatrixMultiplyScale(matrix_t m, vec_t x, vec_t y, vec_t z);
void MatrixFromAngles(matrix_t m, vec_t pitch, vec_t yaw, vec_t roll);
void MatrixSetupTransformFromRotation(matrix_t m, const matrix_t rot, const vec3_t origin);
void MatrixAffineInverse(const matrix_t in, matrix_t out);
void MatrixPerspectiveProjectionFovXYRH(matrix_t m, vec_t fovX, vec_t fovY, vec_t nearvec, vec_t farvec);
void MatrixLookAtRH(matrix_t m, const vec3_t eye, const vec3_t dir, const vec3_t up);
void PlaneIntersectRay(const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res);
void MatrixOrthogonalProjectionRH(matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec);
void MatrixCrop(matrix_t m, const vec3_t mins, const vec3_t maxs);
void BoundsAdd(vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2);
void MatrixIdentity(matrix_t m);
void MatrixMultiplyTranslation(matrix_t m, vec_t x, vec_t y, vec_t z);
void MatrixMultiplyZRotation(matrix_t m, vec_t degrees);
void MatrixSetupZRotation(matrix_t m, vec_t degrees);
void MatrixFromVectorsFLU(matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up);
void MatrixTransformPoint(const matrix_t m, const vec3_t in, vec3_t out);
void MatrixTransformPoint2(const matrix_t m, vec3_t inout);
void MatrixSetupTransformFromVectorsFLU(matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up, const vec3_t origin);
void MatrixToVectorsFLU(const matrix_t m, vec3_t forward, vec3_t left, vec3_t up);
void ClampColor(vec4_t color);
qboolean BoundsIntersect(const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2);
qboolean BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs, const vec3_t origin, vec_t radius);
qboolean BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs, const vec3_t origin);
void MatrixTranspose(const matrix_t in, matrix_t out);
void COM_StripExtension3(const char *src, char *dest, int destsize);
void QuatFromMatrix(quat_t q, const matrix_t m);
void ZeroBounds(vec3_t mins, vec3_t maxs);
void MatrixSetupTransformFromQuat(matrix_t m, const quat_t quat, const vec3_t origin);
void MatrixFromQuat(matrix_t m, const quat_t q);

vec_t PlaneNormalize(vec4_t plane); // returns normal length

void MatrixTransformNormal(const matrix_t m, const vec3_t in, vec3_t out);
void MatrixTransformNormal2(const matrix_t m, vec3_t inout);
void MatrixTransformPoint(const matrix_t m, const vec3_t in, vec3_t out);
void MatrixTransformPoint2(const matrix_t m, vec3_t inout);

void MatrixTransformPlane(const matrix_t m, const vec4_t in, vec4_t out);
void MatrixTransformPlane2(const matrix_t m, vec4_t inout);

void MatrixPerspectiveProjectionFovXYInfiniteRH(matrix_t m, vec_t fovX, vec_t fovY, vec_t nearvec);
void QuatTransformVector(const quat_t q, const vec3_t in, vec3_t out);
qboolean MatrixInverse(matrix_t matrix);
void MatrixSetupShear(matrix_t m, vec_t x, vec_t y);
void MatrixMultiplyShear(matrix_t m, vec_t x, vec_t y);
void QuatToAxis(const quat_t q, vec3_t axis[3]);
void MatrixFromPlanes(matrix_t m, const vec4_t left, const vec4_t right, const vec4_t bottom, const vec4_t top, const vec4_t nearvec, const vec4_t farvec);
void QuatSlerp(const quat_t from, const quat_t to, float frac, quat_t out);
void QuatMultiply0(quat_t qa, const quat_t qb);
void QuatMultiply1(const quat_t qa, const quat_t qb, quat_t qc);
void QuatMultiply2(const quat_t qa, const quat_t qb, quat_t qc);
void QuatMultiply3(const quat_t qa, const quat_t qb, quat_t qc);
void QuatMultiply4(const quat_t qa, const quat_t qb, quat_t qc);
void QuatFromAngles(quat_t q, vec_t pitch, vec_t yaw, vec_t roll);
vec_t QuatNormalize(quat_t q);
void QuatToVectorsFRU(const quat_t q, vec3_t forward, vec3_t right, vec3_t up);
void QuatToVectorsFLU(const quat_t q, vec3_t forward, vec3_t left, vec3_t up);
void MatrixSetupTransformFromVectorsFRU(matrix_t m, const vec3_t forward, const vec3_t right, const vec3_t up, const vec3_t origin);
void MatrixToVectorsFRU(const matrix_t m, vec3_t forward, vec3_t right, vec3_t up);

byte ClampByte(int i);

qboolean Q_strreplace(char *dest, int destsize, const char *find, const char *replace);

#define DotProduct4(x, y)             ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2] + (x)[3] * (y)[3])

static ID_INLINE int Vector4Compare(const vec4_t v1, const vec4_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3])
	{
		return 0;
	}
	return 1;
}

static ID_INLINE float Q_recip(float in)
{
#if id386_3dnow && defined __GNUC__ && 0
	vec_t out;

	femms();
	asm volatile ("movd		(%%eax),	%%mm0\n""pfrcp		%%mm0,		%%mm1\n"        // (approx)
	                                       "pfrcpit1	%%mm1,		%%mm0\n"// (intermediate)
	                                       "pfrcpit2	%%mm1,		%%mm0\n"// (full 24-bit)
	              // out = mm0[low]
	                                       "movd		%%mm0,		(%%edx)\n"::"a" (&in), "d" (&out) : "memory");

	femms();
	return out;
#else
	return (float)(1.0f / in);
#endif
}

static ID_INLINE void VectorToAngles(const vec3_t value1, vec3_t angles)
{
	vectoangles(value1, angles);
}

// RB: XreaL quaternion math functions required by the renderer

#define QuatSet(q, x, y, z, w)  ((q)[0] = (x), (q)[1] = (y), (q)[2] = (z), (q)[3] = (w))
#define QuatCopy(a, b)       ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])

#define QuatCompare(a, b)    ((a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2] && (a)[3] == (b)[3])

static ID_INLINE void VectorLerp(const vec3_t from, const vec3_t to, float frac, vec3_t out)
{
	out[0] = from[0] + ((to[0] - from[0]) * frac);
	out[1] = from[1] + ((to[1] - from[1]) * frac);
	out[2] = from[2] + ((to[2] - from[2]) * frac);
}

static ID_INLINE void QuatClear(quat_t q)
{
	q[0] = 0;
	q[1] = 0;
	q[2] = 0;
	q[3] = 1;
}

static ID_INLINE void QuatCalcW(quat_t q)
{
#if 1
	vec_t term = 1.0f - (q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);

	if (term < 0.0)
	{
		q[3] = 0.0;
	}
	else
	{
		q[3] = -sqrt(term);
	}
#else
	q[3] = sqrt(fabs(1.0f - (q[0] * q[0] + q[1] * q[1] + q[2] * q[2])));
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

// helper struct for reading binary file formats
typedef struct memStream_s
{
	byte *buffer;
	int bufSize;
	byte *curPos;
	int flags;
}
memStream_t;

memStream_t *AllocMemStream(byte *buffer, int bufSize);
void            FreeMemStream(memStream_t *s);
int             MemStreamRead(memStream_t *s, void *buffer, int len);
int             MemStreamGetC(memStream_t *s);
int             MemStreamGetLong(memStream_t *s);
int             MemStreamGetShort(memStream_t *s);
float           MemStreamGetFloat(memStream_t *s);

//=============================================

#ifdef __cplusplus
}
#endif

#endif
