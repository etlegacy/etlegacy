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

	int contextCombined;

	int glslMajorVersion;
	int glslMinorVersion;

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

void PlaneIntersectRay(const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res);
void MatrixCrop(mat4_t m, const vec3_t mins, const vec3_t maxs);
qboolean BoundsIntersect(const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2);
//qboolean BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs, const vec3_t origin, vec_t radius);
//qboolean BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs, const vec3_t origin);


#ifndef ETL_SSE
//void MatrixOrthogonalProjectionRH(mat4_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec);
void MatrixOrthogonalProjectionRH(mat4_t m, vec3_t cropBounds[2]);

// this returns a boolean, indicating if there was any intersection
qboolean PlanesGetIntersectionPoint(const vec4_t plane1, const vec4_t plane2, const vec4_t plane3, vec3_t out);

void MatrixLookAtRH(mat4_t m, const vec3_t eye, const vec3_t dir, const vec3_t up);
void MatrixPerspectiveProjectionFovXYRH(mat4_t m, vec_t fovX, vec_t fovY, vec_t nearvec, vec_t farvec);
void MatrixPerspectiveProjectionFovXYInfiniteRH(mat4_t m, vec_t fovX, vec_t fovY, vec_t nearvec);
void MatrixMultiplyTranslation(mat4_t m, vec_t x, vec_t y, vec_t z);
void MatrixMultiplyScale(mat4_t m, vec_t x, vec_t y, vec_t z);
void MatrixSetupTransformFromQuat(mat4_t m, const quat_t quat, const vec3_t origin);
void MatrixSetupTransformFromRotation(mat4_t m, const mat4_t rot, const vec3_t origin);
void MatrixAffineInverse(const mat4_t in, mat4_t out);
void MatrixTransformNormal(const mat4_t m, const vec3_t in, vec3_t out);
void MatrixTransformPlane(const mat4_t m, const vec4_t in, vec4_t out);
void MatrixFromPlanes(mat4_t m, const vec4_t left, const vec4_t right, const vec4_t bottom, const vec4_t top, const vec4_t nearvec, const vec4_t farvec);
void MatrixTransformBounds(const mat4_t m, const vec3_t mins, const vec3_t maxs, vec3_t omins, vec3_t omaxs);
void MatrixSetupZRotation(mat4_t m, vec_t degrees);
void MatrixMultiplyZRotation(mat4_t m, vec_t degrees);


#else

///void MatrixFromPlanes(mat4_t m, const vec4_t left, const vec4_t right, const vec4_t bottom, const vec4_t top, const vec4_t nearvec, const vec4_t farvec);
#define MatrixFromPlanes(m, left, right, bottom, top, nearvec, farvec) \
{ \
	vec4_t vrl, vtb, vfn, col3; \
	Vector4Subtract(right, left, vrl); \
	Vector4Subtract(top, bottom, vtb); \
	Vector4Subtract(farvec, nearvec, vfn); \
	Vector4Scale(vrl, 0.5f, vrl); \
	Vector4Scale(vtb, 0.5f, vtb); \
	Vector4Scale(vfn, 0.5f, vfn); \
	Vector4Subtract(right, vrl, col3); \
	_mm_storeu_ps(&m[0], _mm_set_ps(col3[0], vfn[0], vtb[0], vrl[0])); \
	_mm_storeu_ps(&m[4], _mm_set_ps(col3[1], vfn[1], vtb[1], vrl[1])); \
	_mm_storeu_ps(&m[8], _mm_set_ps(col3[2], vfn[2], vtb[2], vrl[2])); \
	_mm_storeu_ps(&m[12], _mm_set_ps((-right[3]) - m[12], ((-farvec[3]) - (-nearvec[3])) * 0.5f, ((-top[3]) - (-bottom[3])) * 0.5f, ((-right[3]) - (-left[3])) * 0.5f)); \
}

///void MatrixTransformBounds(const mat4_t m, const vec3_t mins, const vec3_t maxs, vec3_t omins, vec3_t omaxs);
#define MatrixTransformBounds(m, mins, maxs, omins, omaxs) \
{ \
	vec3_t tmins, tmaxs, minx, maxx, miny, maxy, minz, maxz, tmp; \
	const float* c0 = m; \
	const float* c1 = m + 4; \
	const float* c2 = m + 8; \
	const float* c3 = m + 12; \
	VectorScale(c0, mins[0], minx); \
	VectorScale(c0, maxs[0], maxx); \
	VectorScale(c1, mins[1], miny); \
	VectorScale(c1, maxs[1], maxy); \
	VectorScale(c2, mins[2], minz); \
	VectorScale(c2, maxs[2], maxz); \
	VectorMin(minx, maxx, tmins); \
	VectorMax(minx, maxx, tmaxs); \
	VectorAdd(tmins, c3, tmins); \
	VectorAdd(tmaxs, c3, tmaxs); \
	VectorMin(miny, maxy, tmp); \
	VectorAdd(tmp, tmins, tmins); \
	VectorMax(miny, maxy, tmp); \
	VectorAdd(tmp, tmaxs, tmaxs); \
	VectorMin(minz, maxz, tmp); \
	VectorAdd(tmp, tmins, omins); \
	VectorMax(minz, maxz, tmp); \
	VectorAdd(tmp, tmaxs, omaxs); \
}

///void MatrixMultiplyZRotation(mat4_t m, vec_t degrees);
#define MatrixMultiplyZRotation(m, degrees) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
	float S, C; \
	vec_t a = DEG2RAD(degrees); \
	SinCos(a, S, C); \
	xmm4 = _mm_loadu_ps(&m[0]); \
	xmm5 = _mm_loadu_ps(&m[4]); \
	xmm6 = _mm_loadu_ps(&m[8]); \
	xmm7 = _mm_loadu_ps(&m[12]); \
	xmm0 = _mm_set_ps(0.0f, 0.0f, S, C); \
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111); \
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010); \
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101); \
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000); \
	xmm3 = _mm_mul_ps(xmm3, xmm7); \
	xmm2 = _mm_mul_ps(xmm2, xmm6); \
	xmm1 = _mm_mul_ps(xmm1, xmm5); \
	xmm0 = _mm_mul_ps(xmm0, xmm4); \
	xmm3 = _mm_add_ps(xmm3, xmm2); \
	xmm0 = _mm_add_ps(xmm0, xmm1); \
	xmm0 = _mm_add_ps(xmm0, xmm3); \
	_mm_storeu_ps(&m[0], xmm0); \
	xmm0 = _mm_set_ps(0.0f, 0.0f, C, -S); \
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111); \
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010); \
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101); \
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000); \
	xmm3 = _mm_mul_ps(xmm3, xmm7); \
	xmm2 = _mm_mul_ps(xmm2, xmm6); \
	xmm1 = _mm_mul_ps(xmm1, xmm5); \
	xmm0 = _mm_mul_ps(xmm0, xmm4); \
	xmm3 = _mm_add_ps(xmm3, xmm2); \
	xmm0 = _mm_add_ps(xmm0, xmm1); \
	xmm0 = _mm_add_ps(xmm0, xmm3); \
	_mm_storeu_ps(&m[4], xmm0); \
	xmm0 = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f); \
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111); \
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010); \
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101); \
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000); \
	xmm3 = _mm_mul_ps(xmm3, xmm7); \
	xmm2 = _mm_mul_ps(xmm2, xmm6); \
	xmm1 = _mm_mul_ps(xmm1, xmm5); \
	xmm0 = _mm_mul_ps(xmm0, xmm4); \
	xmm3 = _mm_add_ps(xmm3, xmm2); \
	xmm0 = _mm_add_ps(xmm0, xmm1); \
	xmm0 = _mm_add_ps(xmm0, xmm3); \
	_mm_storeu_ps(&m[8], xmm0); \
	xmm0 = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f); \
	xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111); \
	xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010); \
	xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101); \
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000); \
	xmm3 = _mm_mul_ps(xmm3, xmm7); \
	xmm2 = _mm_mul_ps(xmm2, xmm6); \
	xmm1 = _mm_mul_ps(xmm1, xmm5); \
	xmm0 = _mm_mul_ps(xmm0, xmm4); \
	xmm3 = _mm_add_ps(xmm3, xmm2); \
	xmm0 = _mm_add_ps(xmm0, xmm1); \
	xmm0 = _mm_add_ps(xmm0, xmm3); \
	_mm_storeu_ps(&m[12], xmm0); \
}

///void MatrixTransformNormal(const mat4_t m, const vec3_t in, vec3_t out);
#define MatrixTransformNormal(m, in, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5; \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&in[0]), (const __m64 *)(&in[1])); \
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111); \
	xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b00000000); \
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010); \
	xmm3 = _mm_loadu_ps(&m[0]); \
	xmm4 = _mm_loadu_ps(&m[4]); \
	xmm5 = _mm_loadu_ps(&m[8]); \
	xmm0 = _mm_mul_ps(xmm0, xmm3); \
	xmm1 = _mm_mul_ps(xmm1, xmm4); \
	xmm2 = _mm_mul_ps(xmm2, xmm5); \
	xmm0 = _mm_add_ps(xmm0, xmm1); \
	xmm0 = _mm_add_ps(xmm0, xmm2); \
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b10010100); \
	_mm_store_ss(&out[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&out[1]), xmm0); \
}

///void MatrixTransformPlane(const mat4_t m, const vec4_t in, vec4_t out);
#define MatrixTransformPlane(m, in, out) \
{ \
	vec3_t translation, planePos; \
	MatrixTransformNormal(m, in, out); \
	VectorSet(translation, m[12], m[13], m[14]); \
	VectorMA(translation, in[3], out, planePos); \
	Dot(out, planePos, out[3]); \
}

///void MatrixAffineInverse(const mat4_t in, mat4_t out);
#define MatrixAffineInverse(in, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
	xmm6 = _mm_set_ps(0.0f, 1.0f, 0.0f, 1.0f); \
	xmm3 = _mm_loadu_ps(&in[0]); \
	xmm4 = _mm_loadu_ps(&in[4]); \
	xmm5 = _mm_loadu_ps(&in[8]); \
	xmm0 = _mm_shuffle_ps(xmm4, xmm3, 0b00000000); \
	xmm1 = _mm_shuffle_ps(xmm6, xmm5, 0b00000101); \
	xmm0 = _mm_shuffle_ps(xmm0, xmm1, 0b00110011); \
	_mm_storeu_ps(&out[0], xmm0); \
	xmm1 = _mm_shuffle_ps(xmm4, xmm3, 0b01010101); \
	xmm2 = _mm_shuffle_ps(xmm6, xmm5, 0b01010101); \
	xmm1 = _mm_shuffle_ps(xmm1, xmm2, 0b00110011); \
	_mm_storeu_ps(&out[4], xmm1); \
	xmm2 = _mm_shuffle_ps(xmm4, xmm3, 0b10101010); \
	xmm7 = _mm_shuffle_ps(xmm6, xmm5, 0b10100101); \
	xmm2 = _mm_shuffle_ps(xmm2, xmm7, 0b00110011); \
	_mm_storeu_ps(&out[8], xmm2); \
	xmm3 = _mm_loadu_ps(&in[12]); \
	xmm4 = _mm_shuffle_ps(xmm3, xmm3, 0b01010101); \
	xmm5 = _mm_shuffle_ps(xmm3, xmm3, 0b10101010); \
	xmm3 = _mm_shuffle_ps(xmm3, xmm3, 0b00000000); \
	xmm5 = _mm_mul_ps(xmm5, xmm2); \
	xmm4 = _mm_mul_ps(xmm4, xmm1); \
	xmm3 = _mm_mul_ps(xmm3, xmm0); \
	xmm4 = _mm_add_ps(xmm4, xmm5); \
	xmm3 = _mm_add_ps(xmm3, xmm4); \
	xmm7 = _mm_shuffle_ps(xmm6, xmm6, 0b01010101); \
	xmm3 = _mm_sub_ps(xmm7, xmm3); \
	xmm6 = _mm_shuffle_ps(xmm6, xmm3, 0b10100000); \
	xmm3 = _mm_shuffle_ps(xmm3, xmm6, 0b00110100); \
	_mm_storeu_ps(&out[12], xmm3); \
}

///void MatrixSetupTransformFromQuat(mat4_t m, const quat_t quat, const vec3_t origin);
#define MatrixSetupTransformFromQuat(m, quat, origin) \
{ \
	Matrix4FromQuaternion(m, quat); \
	VectorCopy(&origin[0], &m[12]); \
	m[3] = 0.0f; \
	m[7] = 0.0f; \
	m[11] = 0.0f; \
	m[15] = 1.0f; \
}

///void MatrixSetupTransformFromRotation(mat4_t m, const mat4_t rot, const vec3_t origin);
#define MatrixSetupTransformFromRotation(m, rot, origin) \
{ \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, rot[2], rot[1], rot[0])); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, rot[6], rot[5], rot[4])); \
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, rot[10], rot[9], rot[8])); \
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, origin[2], origin[1], origin[0])); \
}

///void MatrixOrthogonalProjectionRH(mat4_t m, vec3_t cropBounds[2]);
#define MatrixOrthogonalProjectionRH(m, cropBounds) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
	xmm7 = _mm_loadu_ps(&cropBounds[0][0]); \
	xmm6 = _mm_loadu_ps(&cropBounds[0][2]); \
	xmm6 = _mm_shuffle_ps(xmm6, xmm6, 0b00111001); \
	xmm5 = _mm_sub_ps(_mm_setzero_ps(), xmm6); \
	xmm7 = _mm_shuffle_ps(xmm7, xmm5, 0b11110100); \
	xmm6 = _mm_shuffle_ps(xmm6, xmm5, 0b11100100); \
	xmm0 = _mm_sub_ps(xmm6, xmm7); \
	xmm1 = _mm_sub_ps(xmm7, xmm6); \
	xmm2 = _mm_add_ps(xmm6, xmm7); \
	xmm3 = _mm_div_ps(xmm2, xmm1); \
	xmm5 = _mm_rcp_ps(xmm0); \
	xmm4 = _mm_mul_ps(xmm6, xmm5); \
	xmm7 = _mm_add_ps(xmm5, xmm5); \
	xmm7 = _mm_shuffle_ps(xmm7, _mm_setzero_ps(), 0b00000100); \
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0b11111100); \
	_mm_storeu_ps(&m[0], xmm0); \
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0b11110111); \
	_mm_storeu_ps(&m[4], xmm0); \
	xmm5 = _mm_shuffle_ps(xmm0, xmm5, 0b10100000); \
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, 0b00100000); \
	_mm_storeu_ps(&m[8], xmm5); \
	xmm4 = _mm_shuffle_ps(xmm4, _mm_set_ps1(1.0f), 0b00001010); \
	xmm3 = _mm_shuffle_ps(xmm3, xmm4, 0b11000100); \
	_mm_storeu_ps(&m[12], xmm3); \
}

///void MatrixPerspectiveProjectionFovXYRH(mat4_t m, vec_t fovX, vec_t fovY, vec_t nearvec, vec_t farvec);
#define MatrixPerspectiveProjectionFovXYRH(m, fovX, fovY, nearvec, farvec) \
{ \
	vec_t width = tanf(DEG2RAD(fovX * 0.5f)), height = tanf(DEG2RAD(fovY * 0.5f)), FarrNearFar = farvec / (nearvec - farvec); \
	float rw = rcp(width), rh = rcp(height); \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, rw)); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, rh, 0.0f)); \
	_mm_storeu_ps(&m[8], _mm_set_ps(-1.0f, FarrNearFar, 0.0f, 0.0f)); \
	_mm_storeu_ps(&m[12], _mm_set_ps(0.0f, nearvec * FarrNearFar, 0.0f, 0.0f)); \
}

///void MatrixPerspectiveProjectionFovXYInfiniteRH(mat4_t m, vec_t fovX, vec_t fovY, vec_t nearvec);
#define MatrixPerspectiveProjectionFovXYInfiniteRH(m, fovX, fovY, nearvec) \
{ \
	vec_t width = tanf(DEG2RAD(fovX * 0.5f)), height = tanf(DEG2RAD(fovY * 0.5f)); \
	float rw = rcp(width), rh = rcp(height); \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, rw)); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, rh, 0.0f)); \
	_mm_storeu_ps(&m[8], _mm_set_ps(-1.0f, -1.0f, 0.0f, 0.0f)); \
	_mm_storeu_ps(&m[12], _mm_set_ps(0.0f, -2.0f * nearvec, 0.0f, 0.0f)); \
}

///void MatrixLookAtRH(mat4_t m, const vec3_t eye, const vec3_t dir, const vec3_t up);
#define MatrixLookAtRH(m, eye, dir, up) \
{ \
	vec3_t dirN, upN, sideN; \
	float dotde, dotue, dotse; \
	CrossProduct(dir, up, sideN); \
	VectorNormalizeOnly(sideN); \
	CrossProduct(sideN, dir, upN); \
	VectorNormalizeOnly(upN); \
	VectorNormalize2Only(dir, dirN); \
	Dot(dirN, eye, dotde); \
	Dot(upN, eye, dotue); \
	Dot(sideN, eye, dotse); \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, -dirN[0], upN[0], sideN[0])); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, -dirN[1], upN[1], sideN[1])); \
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, -dirN[2], upN[2], sideN[2])); \
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, dotde, -dotue, -dotse)); \
}

///void MatrixMultiplyTranslation(mat4_t m, vec_t x, vec_t y, vec_t z);
#define MatrixMultiplyTranslation(m, x, y, z) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3; \
	xmm0 = _mm_loadu_ps(&m[0]); \
	xmm1 = _mm_loadu_ps(&m[4]); \
	xmm2 = _mm_loadu_ps(&m[8]); \
	xmm3 = _mm_loadu_ps(&m[12]); \
	xmm0 = _mm_mul_ps(xmm0, _mm_set_ps1(x)); \
	xmm1 = _mm_mul_ps(xmm1, _mm_set_ps1(y)); \
	xmm2 = _mm_mul_ps(xmm2, _mm_set_ps1(z)); \
	xmm0 = _mm_add_ps(xmm0, xmm1); \
	xmm3 = _mm_add_ps(xmm3, xmm2); \
	xmm3 = _mm_add_ps(xmm3, xmm0); \
	_mm_storeu_ps(&m[12], xmm3); \
}

///void MatrixMultiplyScale(mat4_t m, vec_t x, vec_t y, vec_t z);
#define MatrixMultiplyScale(m, x, y, z) \
{ \
	__m128 xmm0, xmm1, xmm2; \
	xmm0 = _mm_loadu_ps(&m[0]); \
	xmm1 = _mm_loadu_ps(&m[4]); \
	xmm2 = _mm_loadu_ps(&m[8]); \
	xmm0 = _mm_mul_ps(xmm0, _mm_set_ps1(x)); \
	xmm1 = _mm_mul_ps(xmm1, _mm_set_ps1(y)); \
	xmm2 = _mm_mul_ps(xmm2, _mm_set_ps1(z)); \
	_mm_storeu_ps(&m[0], xmm0); \
	_mm_storeu_ps(&m[4], xmm1); \
	_mm_storeu_ps(&m[8], xmm2); \
}

///qboolean PlanesGetIntersectionPoint(const vec4_t plane1, const vec4_t plane2, const vec4_t plane3, vec3_t out);
//
// note that this inlined intrinsic doed not return any function-result.
// The reason for this, is that nowhere in code any result of the function is ever used. always ignored.
#define PlanesGetIntersectionPoint(plane1, plane2, plane3, out) \
{ \
	vec3_t n1, n2, n3, n1n2, n2n3, n3n1; \
	vec_t  denom; \
	VectorNormalize2Only(plane1, n1); \
	VectorNormalize2Only(plane2, n2); \
	VectorNormalize2Only(plane3, n3); \
	CrossProduct(n1, n2, n1n2); \
	CrossProduct(n2, n3, n2n3); \
	CrossProduct(n3, n1, n3n1); \
	VectorClear(out); \
	Dot(n1, n2n3, denom); \
	if (denom != 0.f) { \
		VectorMA(out, plane1[3], n2n3, out); \
		VectorMA(out, plane2[3], n3n1, out); \
		VectorMA(out, plane3[3], n1n2, out); \
		RECIPROCAL(denom); \
		VectorScale(out, denom, out); \
	} \
}

#endif


void ZeroBounds(vec3_t mins, vec3_t maxs);

//vec_t PlaneNormalize(vec4_t plane); ///< returns normal length
void PlaneNormalize(vec4_t plane); ///< returns nothing

void MatrixTransformNormal2(const mat4_t m, vec3_t inout);

//void MatrixTransformPlane2(const mat4_t m, vec4_t inout);

void QuatTransformVector(const quat_t q, const vec3_t in, vec3_t out);
void MatrixSetupShear(mat4_t m, vec_t x, vec_t y);
void MatrixMultiplyShear(mat4_t m, vec_t x, vec_t y);


//void QuatMultiply0(quat_t qa, const quat_t qb);
void QuatMultiply1(const quat_t qa, const quat_t qb, quat_t qc);
//void QuatMultiply2(const quat_t qa, const quat_t qb, quat_t qc);
//void QuatMultiply3(const quat_t qa, const quat_t qb, quat_t qc);
//void QuatMultiply4(const quat_t qa, const quat_t qb, quat_t qc);

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
#ifndef ETL_SSE
	return (float)(1.0f / in);
#else
	return _mm_cvtss_f32(_mm_rcp_ss(_mm_load_ss(&in)));
#endif
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
	q[0] = 0.0f;
	q[1] = 0.0f;
	q[2] = 0.0f;
	q[3] = 1.0f;
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

void printBits(size_t const size, void const *const ptr);

#endif
