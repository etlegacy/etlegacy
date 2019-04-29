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
 * @file renderer2/tr_extra.c
 */

#include "tr_local.h"


mat4_t matrixIdentity = { 1.0f, 0.0f, 0.0f, 0.0f,
	                      0.0f, 1.0f, 0.0f, 0.0f,
	                      0.0f, 0.0f, 1.0f, 0.0f,
	                      0.0f, 0.0f, 0.0f, 1.0f };

quat_t quatIdentity = { 0.0f, 0.0f, 0.0f, 1.0f };

/**
 * @brief NearestPowerOfTwo
 * @param[in] val
 * @return
 */
int NearestPowerOfTwo(int val)
{
	int answer;

	for (answer = 1; answer < val; answer <<= 1)
		;
	return answer;
}

#ifndef ETL_SSE
/**
 * @brief PlanesGetIntersectionPoint
 * @param[in] plane1
 * @param[in] plane2
 * @param[in] plane3
 * @param[out] out
 * @return
 */
qboolean PlanesGetIntersectionPoint(const vec4_t plane1, const vec4_t plane2, const vec4_t plane3, vec3_t out)
{
	// http://www.cgafaq.info/wiki/Intersection_of_three_planes

	vec3_t n1, n2, n3, n1n2, n2n3, n3n1;
	vec_t  denom;

	VectorNormalize2Only(plane1, n1);
	VectorNormalize2Only(plane2, n2);
	VectorNormalize2Only(plane3, n3);

	CrossProduct(n1, n2, n1n2);
	CrossProduct(n2, n3, n2n3);
	CrossProduct(n3, n1, n3n1);

	//denom = DotProduct(n1, n2n3);
	Dot(n1, n2n3, denom);

	// check if the denominator is zero (which would mean that no intersection is to be found
	if (denom == 0.f)
	{
		// no intersection could be found, return <0,0,0>
		VectorClear(out);
		return qfalse;
	}

	VectorClear(out);

	VectorMA(out, plane1[3], n2n3, out);
	VectorMA(out, plane2[3], n3n1, out);
	VectorMA(out, plane3[3], n1n2, out);

	VectorScale(out, 1.0f / denom, out);

	return qtrue;
}
#endif

#ifndef ETL_SSE
/**
 * @brief MatrixMultiplyScale
 * @param[in,out] m
 * @param[in] x
 * @param[in] y
 * @param[in] z
 */
void MatrixMultiplyScale(mat4_t m, vec_t x, vec_t y, vec_t z)
{
#if 0
	mat4_t tmp, scale;
	MatrixCopy(m, tmp);
	MatrixSetupScale(scale, x, y, z);
	MatrixMultiplyMOD(tmp, scale, m);
#else
#ifndef ETL_SSE
	m[0] *= x;     m[4] *= y;        m[8] *= z;
	m[1] *= x;     m[5] *= y;        m[9] *= z;
	m[2] *= x;     m[6] *= y;        m[10] *= z;
	m[3] *= x;     m[7] *= y;        m[11] *= z;
#else
	__m128 xmm0, xmm1, xmm2;
	xmm0 = _mm_loadu_ps(&m[0]);
	xmm1 = _mm_loadu_ps(&m[4]);
	xmm2 = _mm_loadu_ps(&m[8]);
	xmm0 = _mm_mul_ps(xmm0, _mm_set_ps1(x));
	xmm1 = _mm_mul_ps(xmm1, _mm_set_ps1(y));
	xmm2 = _mm_mul_ps(xmm2, _mm_set_ps1(z));
	_mm_storeu_ps(&m[0], xmm0);
	_mm_storeu_ps(&m[4], xmm1);
	_mm_storeu_ps(&m[8], xmm2);
#endif
#endif
}
#endif

#ifndef ETL_SSE
/**
 * @brief MatrixSetupTransformFromRotation
 * @param[out] m
 * @param[in] rot
 * @param[in] origin
 */
void MatrixSetupTransformFromRotation(mat4_t m, const mat4_t rot, const vec3_t origin)
{
#ifndef ETL_SSE
	m[0] = rot[0];    m[4] = rot[4];    m[8] = rot[8];     m[12] = origin[0];
	m[1] = rot[1];    m[5] = rot[5];    m[9] = rot[9];     m[13] = origin[1];
	m[2] = rot[2];    m[6] = rot[6];    m[10] = rot[10];   m[14] = origin[2];
	m[3] = 0.0f;      m[7] = 0.0f;      m[11] = 0.0f;      m[15] = 1.0f;
#else
	// provide the columns (in high to low order wzyx)
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, rot[2], rot[1], rot[0]));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, rot[6], rot[5], rot[4]));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, rot[10], rot[9], rot[8]));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, origin[2], origin[1], origin[0]));
#endif
}
#endif

#ifndef ETL_SSE
/**
 * @brief MatrixAffineInverse
 * @param[in] in
 * @param[out] out
 */
void MatrixAffineInverse(const mat4_t in, mat4_t out)
{
#if 0
	MatrixCopy(in, out);
	MatrixInverse(out);
#else
#ifndef ETL_SSE
	// cleaned up
	out[0] = in[0];       out[4] = in[1];       out[8] = in[2];          out[12] = -(in[12] * out[0] + in[13] * out[4] + in[14] * out[8]);
	out[1] = in[4];       out[5] = in[5];       out[9] = in[6];          out[13] = -(in[12] * out[1] + in[13] * out[5] + in[14] * out[9]);
	out[2] = in[8];       out[6] = in[9];       out[10] = in[10];        out[14] = -(in[12] * out[2] + in[13] * out[6] + in[14] * out[10]);
	out[3] = 0.0f;        out[7] = 0.0f;        out[11] = 0.0f;          out[15] = 1.0f;
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	xmm6 = _mm_set_ps(0.0f, 1.0f, 0.0f, 1.0f);						// constants
	xmm3 = _mm_loadu_ps(&in[0]);
	xmm4 = _mm_loadu_ps(&in[4]);
	xmm5 = _mm_loadu_ps(&in[8]);
	// row 0
	xmm0 = _mm_shuffle_ps(xmm4, xmm3, 0b00000000);					// xmm0 = in0 in0 in4 in4
	xmm1 = _mm_shuffle_ps(xmm6, xmm5, 0b00000101);					// xmm1 = in8 in8 0   0
	xmm0 = _mm_shuffle_ps(xmm0, xmm1, 0b00110011);					// xmm0 = 0   in8 in4 in0
	_mm_storeu_ps(&out[0], xmm0);
	// row 1
	xmm1 = _mm_shuffle_ps(xmm4, xmm3, 0b01010101);					// xmm1 = in1 in1 in5 in5
	xmm2 = _mm_shuffle_ps(xmm6, xmm5, 0b01010101);					// xmm2 = in9 in9 0   0
	xmm1 = _mm_shuffle_ps(xmm1, xmm2, 0b00110011);					// xmm0 = 0   in9 in5 in1
	_mm_storeu_ps(&out[4], xmm1);
	// row 2
	xmm2 = _mm_shuffle_ps(xmm4, xmm3, 0b10101010);					// xmm2 = in2  in2  in6 in6
	xmm7 = _mm_shuffle_ps(xmm6, xmm5, 0b10100101);					// xmm7 = in10 in10 0   0
	xmm2 = _mm_shuffle_ps(xmm2, xmm7, 0b00110011);					// xmm2 = 0    in10 in6 in2
	_mm_storeu_ps(&out[8], xmm2);
	// bottom row
	xmm3 = _mm_loadu_ps(&in[12]);									// xmm3 = in15 in14 in13 in12
	xmm4 = _mm_shuffle_ps(xmm3, xmm3, 0b01010101);					// xmm4 = in13 in13 in13 in13
	xmm5 = _mm_shuffle_ps(xmm3, xmm3, 0b10101010);					// xmm5 = in14 in14 in14 in14
	xmm3 = _mm_shuffle_ps(xmm3, xmm3, 0b00000000);					// xmm3 = in12 in12 in12 in12
	xmm5 = _mm_mul_ps(xmm5, xmm2);
	xmm4 = _mm_mul_ps(xmm4, xmm1);
	xmm3 = _mm_mul_ps(xmm3, xmm0);
	xmm4 = _mm_add_ps(xmm4, xmm5);
	xmm3 = _mm_add_ps(xmm3, xmm4);									// xmm3 = _ z y x
	xmm7 = _mm_shuffle_ps(xmm6, xmm6, 0b01010101);					// xmm7 = 0 0 0 0
	xmm3 = _mm_sub_ps(xmm7, xmm3);									// xmm3 = -xmm3
	xmm6 = _mm_shuffle_ps(xmm6, xmm6, 0b00000000);					// xmm6 = 1 1 1 1
	xmm6 = _mm_shuffle_ps(xmm6, xmm3, 0b10100000);					// xmm3 = z z 1 1
	xmm3 = _mm_shuffle_ps(xmm3, xmm6, 0b00110100);					// xmm0 = 1 z y x
	_mm_storeu_ps(&out[12], xmm3);
#endif
#endif
}
#endif

#ifndef ETL_SSE
/**
 * @brief MatrixPerspectiveProjectionFovXYRH
 * @param[out] m
 * @param[in] fovX
 * @param[in] fovY
 * @param[in] nearvec
 * @param[in] farvec
 */
void MatrixPerspectiveProjectionFovXYRH(mat4_t m, vec_t fovX, vec_t fovY, vec_t nearvec, vec_t farvec)
{
	vec_t width = tanf(DEG2RAD(fovX * 0.5f)),
		height = tanf(DEG2RAD(fovY * 0.5f)),
		FarrNearFar = farvec / (nearvec - farvec);
#ifndef ETL_SSE
	m[0] = 1.0f / width;   m[4] = 0.0f;            m[8] = 0.0f;           m[12] = 0.0f;
	m[1] = 0.0f;           m[5] = 1.0f / height;   m[9] = 0.0f;           m[13] = 0.0f;
	m[2] = 0.0f;           m[6] = 0.0f;            m[10] = FarrNearFar;   m[14] = nearvec * FarrNearFar;
	m[3] = 0.0f;           m[7] = 0.0f;            m[11] = -1.0f;         m[15] = 0.0f;
#else
	// provide the columns (in high to low order wzyx)
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f / width));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, 1.0f / height, 0.0f));
	_mm_storeu_ps(&m[8], _mm_set_ps(-1.0f, FarrNearFar, 0.0f, 0.0f));
	_mm_storeu_ps(&m[12], _mm_set_ps(0.0f, nearvec * FarrNearFar, 0.0f, 0.0f));
#endif
}
#endif

#ifndef ETL_SSE
/**
 * @brief MatrixLookAtRH
 * @param[out] m
 * @param[in] eye
 * @param[in] dir
 * @param[in] up
 */
void MatrixLookAtRH(mat4_t m, const vec3_t eye, const vec3_t dir, const vec3_t up)
{
	vec3_t dirN, upN, sideN;
	CrossProduct(dir, up, sideN);
	VectorNormalizeOnly(sideN);
	CrossProduct(sideN, dir, upN);
	VectorNormalizeOnly(upN);
	VectorNormalize2Only(dir, dirN);
#ifndef ETL_SSE
	m[0] = sideN[0];   m[4] = sideN[1];       m[8] = sideN[2];        m[12] = -DotProduct(sideN, eye);
	m[1] = upN[0];     m[5] = upN[1];         m[9] = upN[2];          m[13] = -DotProduct(upN, eye);
	m[2] = -dirN[0];   m[6] = -dirN[1];       m[10] = -dirN[2];       m[14] = DotProduct(dirN, eye);
	m[3] = 0.0f;       m[7] = 0.0f;           m[11] = 0.0f;           m[15] = 1.0f;
#else
	float dotde, dotue, dotse;
	Dot(dirN, eye, dotde);
	Dot(upN, eye, dotue);
	Dot(sideN, eye, dotse);
	// provide the columns (in high to low order wzyx)
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, -dirN[0], upN[0], sideN[0]));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, -dirN[1], upN[1], sideN[1]));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, -dirN[2], upN[2], sideN[2]));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, dotde, -dotue, -dotse));
#endif
}
#endif

/**
 * @brief PlaneIntersectRay
 * @param[in] rayPos
 * @param[in] rayDir
 * @param[in] plane
 * @param[out] res
 */
void PlaneIntersectRay(const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res)
{
	vec3_t dir;
	float  sect, dotprp, dotprd;
	VectorNormalize2Only(rayDir, dir);
	Dot(plane, rayPos, dotprp);
	Dot(plane, rayDir, dotprd);
	sect = -(dotprp - plane[3]) / dotprd;
	VectorScale(dir, sect, dir);
	VectorAdd(rayPos, dir, res);
}


// This function is working well, but replaced with a newer version (with the same name, but other arguments)
/**
 * @brief MatrixOrthogonalProjectionRH
 * @param[out] m
 * @param[in] left
 * @param[in] right
 * @param[in] bottom
 * @param[in] top
 * @param[in] nearvec
 * @param[in] farvec
 *
 * @note same as D3DXMatrixOrthoOffCenterRH
 *
 * http://msdn.microsoft.com/en-us/library/bb205348(VS.85).aspx
 * /
void MatrixOrthogonalProjectionRH(mat4_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec)
{
	float rNearFar = 1.0f / (nearvec - farvec);
	m[0] = 2.0f / (right - left);  m[4] = 0.0f;                   m[8] = 0.0f;            m[12] = (left + right) / (left - right);
	m[1] = 0.0f;                   m[5] = 2.0f / (top - bottom);  m[9] = 0.0f;            m[13] = (top + bottom) / (bottom - top);
	m[2] = 0.0f;                   m[6] = 0.0f;                   m[10] = rNearFar;       m[14] = nearvec * rNearFar;
	m[3] = 0.0f;                   m[7] = 0.0f;                   m[11] = 0.0f;           m[15] = 1.0f;
}*/

#ifndef ETL_SSE
/**
 * @brief MatrixOrthogonalProjectionRH
 */
void MatrixOrthogonalProjectionRH(mat4_t m, vec3_t cropBounds[2])
{
#ifndef ETL_SSE
	float rNearFar = 1.0f / ((-cropBounds[1][2]) - (-cropBounds[0][2]));
	m[0] = 2.0f / (cropBounds[1][0] - cropBounds[0][0]);  m[4] = 0.0f;                                          m[8] = 0.0f;        m[12] = (cropBounds[0][0] + cropBounds[1][0]) / (cropBounds[0][0] - cropBounds[1][0]);
	m[1] = 0.0f;                                          m[5] = 2.0f / (cropBounds[1][1] - cropBounds[0][1]);  m[9] = 0.0f;        m[13] = (cropBounds[1][1] + cropBounds[0][1]) / (cropBounds[0][1] - cropBounds[1][1]);
	m[2] = 0.0f;                                          m[6] = 0.0f;                                          m[10] = rNearFar;   m[14] = (-cropBounds[1][2]) * rNearFar;
	m[3] = 0.0f;                                          m[7] = 0.0f;                                          m[11] = 0.0f;       m[15] = 1.0f;
#else
	/*
	l	cropBounds[0][0]
	r	cropBounds[1][0]
	b	cropBounds[0][1]
	t	cropBounds[1][1]
	n	-cropBounds[1][2]
	f	-cropBounds[0][2]

	cropBounds[0] = l b f
	cropBounds[1] = r t n
*/
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	// load cropBounds
	xmm7 = _mm_loadu_ps(&cropBounds[0][0]);			// xmm7 = r f b l
	xmm6 = _mm_loadu_ps(&cropBounds[0][2]);			// xmm6 = n t r f
	xmm6 = _mm_shuffle_ps(xmm6, xmm6, 0b00111001);	// xmm6 = f n t r
	// f = -f;  n = -n;
	xmm5 = _mm_sub_ps(_mm_setzero_ps(), xmm6);		// xmm5 = -f -n -t -r
	xmm7 = _mm_shuffle_ps(xmm7, xmm5, 0b11110100);	// xmm7 = -f -f b l
	xmm6 = _mm_shuffle_ps(xmm6, xmm5, 0b11100100);	// xmm6 = -f -n t r
	// n & f are negated now. from now on, i call them n & f (and not refer to them as -n & -f).
	// Now calculate the matrix values
	xmm0 = _mm_sub_ps(xmm6, xmm7);					// xmm0 = _ n-f t-b r-l
	xmm1 = _mm_sub_ps(xmm7, xmm6);					// xmm1 = _ _   b-t l-r
	xmm2 = _mm_add_ps(xmm6, xmm7);					// xmm2 = _ _   t+b r+l
	xmm3 = _mm_div_ps(xmm2, xmm1);					// xmm3 = _ _   (t+b)/(b-t) (r+l)/(l-r)
	xmm5 = _mm_rcp_ps(xmm0);						// xmm5 = _ 1/(n-f) 1/(t-b) 1/(r-l)
	xmm4 = _mm_mul_ps(xmm6, xmm5);					// xmm4 = _ n/(n-f) _ _
	xmm7 = _mm_add_ps(xmm5, xmm5);					// xmm7 = _ _ 2/(t-b) 2/(r-l)
	// row[0]
	xmm7 = _mm_shuffle_ps(xmm7, _mm_setzero_ps(), 0b00000100);	// xmm7 = 0 0 2/(t-b) 2/(r-l)
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0b11111100);	// xmm0 = 0 0 0 2/(r-l)
	_mm_storeu_ps(&m[0], xmm0);						// store row[0]
	// row[1]
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0b11110111);	// xmm0 = 0 0 2/(t-b) 0
	_mm_storeu_ps(&m[4], xmm0);						// store row[1]
	// row[2]
	xmm5 = _mm_shuffle_ps(xmm0, xmm5, 0b10100000);	// xmm5 = 1/(n-f) 1/(n-f) 0 0
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, 0b00100000);	// xmm5 = 0 1/(n-f) 0 0
	_mm_storeu_ps(&m[8], xmm5);						// store row[2]
	// row[3]
	xmm4 = _mm_shuffle_ps(xmm4, _mm_set1_ps(1.0f), 0b00001010);	// xmm1 = 1 1 n/(n-f) n/(n-f)
	xmm3 = _mm_shuffle_ps(xmm3, xmm4, 0b11000100);	// xmm3 = 1 n/(n-f) (t+b)/(b-t) (r+l)/(l-r)
	_mm_storeu_ps(&m[12], xmm3);					// store row[3]
#endif
}
#endif


/**
 * @brief MatrixCrop
 * @param[out] m
 * @param[in] mins
 * @param[in] maxs
 */
void MatrixCrop(mat4_t m, const vec3_t mins, const vec3_t maxs)
{
	/*float scaleX, scaleY, scaleZ;
	float offsetX, offsetY, offsetZ;*/
	vec3_t scale, offset;

	/*scaleX = 2.0f / (maxs[0] - mins[0]);
	scaleY = 2.0f / (maxs[1] - mins[1]);
	scaleZ  = 1.0f / (maxs[2] - mins[2]);*/
	VectorSubtract(maxs, mins, scale);
	//scale[0] = 2.0f / scale[0];
	//scale[1] = 2.0f / scale[1];
	//scale[2] = 1.0f / scale[2];
	scale[0] = rcp(scale[0]) * 2.0f;
	scale[1] = rcp(scale[1]) * 2.0f;
	scale[2] = rcp(scale[2]);

	/*offsetX = -0.5f * (maxs[0] + mins[0]) * scaleX;
	offsetY = -0.5f * (maxs[1] + mins[1]) * scaleY;
	offsetZ = -mins[2] * scaleZ;*/
	offset[0] = -(maxs[0] + mins[0]) * 0.5f;
	offset[1] = -(maxs[1] + mins[1]) * 0.5f;
	offset[2] = -mins[2];
	VectorMultiply(offset, scale, offset);

#ifndef ETL_SSE
	m[0] = scale[0];   m[4] = 0.0f;       m[8] = 0.0f;        m[12] = offset[0];
	m[1] = 0.0f;       m[5] = scale[1];   m[9] = 0.0f;        m[13] = offset[1];
	m[2] = 0.0f;       m[6] = 0.0f;       m[10] = scale[2];   m[14] = offset[2];
	m[3] = 0.0f;       m[7] = 0.0f;       m[11] = 0.0f;       m[15] = 1.0f;
#else
	// provide the columns (in high to low order wzyx)
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, scale[0]));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, scale[1], 0.0f));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, scale[2], 0.0f, 0.0f));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, offset[2], offset[1], offset[0]));
#endif
}

#ifndef ETL_SSE
// The ETL_SSE version has an inlined macro for this function

/**
 * @brief MatrixMultiplyTranslation
 * @param[in,out] m
 * @param[in] x
 * @param[in] y
 * @param[in] z
 */
void MatrixMultiplyTranslation(mat4_t m, vec_t x, vec_t y, vec_t z)
{
#if 0
	mat4_t tmp, trans;
	Matrix4Copy(m, tmp);
	Matrix4IdentTranslate(trans, x, y, z);
	Matrix4Multiply(tmp, trans, m);
#else
#ifndef ETL_SSE
	m[12] += m[0] * x + m[4] * y + m[8] * z;
	m[13] += m[1] * x + m[5] * y + m[9] * z;
	m[14] += m[2] * x + m[6] * y + m[10] * z;
	m[15] += m[3] * x + m[7] * y + m[11] * z;
#else
	__m128 xmm0, xmm1, xmm2, xmm3;
	xmm0 = _mm_loadu_ps(&m[0]);
	xmm1 = _mm_loadu_ps(&m[4]);
	xmm2 = _mm_loadu_ps(&m[8]);
	xmm3 = _mm_loadu_ps(&m[12]);
	xmm0 = _mm_mul_ps(xmm0, _mm_set_ps1(x));
	xmm1 = _mm_mul_ps(xmm1, _mm_set_ps1(y));
	xmm2 = _mm_mul_ps(xmm2, _mm_set_ps1(z));
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm3 = _mm_add_ps(xmm3, xmm2);
	xmm3 = _mm_add_ps(xmm3, xmm0);
	_mm_storeu_ps(&m[12], xmm3);
#endif
#endif
}
#endif

#ifndef ETL_SSE
/**
 * @brief MatrixSetupZRotation
 * @param[out] m
 * @param[in] degrees
 */
void MatrixSetupZRotation(mat4_t m, vec_t degrees)
{
	vec_t a = DEG2RAD(degrees);
	//float S = sin(a), C = cos(a);
	float S, C;
	SinCos(a, S, C);
#ifndef ETL_SSE
	m[0] = C;      m[4] = -S;     m[8] = 0.0f;    m[12] = 0.0f;
	m[1] = S;      m[5] = C;      m[9] = 0.0f;    m[13] = 0.0f;
	m[2] = 0.0f;   m[6] = 0.0f;   m[10] = 1.0f;   m[14] = 0.0f;
	m[3] = 0.0f;   m[7] = 0.0f;   m[11] = 0.0f;   m[15] = 1.0f;
#else
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, S, C));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, C, -S));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
#endif
}

/**
 * @brief MatrixMultiplyZRotation
 * @param[in,out] m
 * @param[in] degrees
 */
void MatrixMultiplyZRotation(mat4_t m, vec_t degrees)
{
	/*mat4_t tmp, rot;
	Matrix4Copy(m, tmp);
	MatrixSetupZRotation(rot, degrees);
	Matrix4Multiply(tmp, rot, m);*/
	mat4_t rot;
	MatrixSetupZRotation(rot, degrees);
	Matrix4MultiplyWith(m, rot);
}
#endif

/**
 * @brief BoundsIntersect
 * @param[in] mins
 * @param[in] maxs
 * @param[in] mins2
 * @param[in] maxs2
 * @return
 */
qboolean BoundsIntersect(const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2)
{
#ifndef ETL_SSE
	if (maxs[0] < mins2[0] || maxs[1] < mins2[1] || maxs[2] < mins2[2] ||
		mins[0] > maxs2[0] || mins[1] > maxs2[1] || mins[2] > maxs2[2])
	{
		return qfalse;
	}
	return qtrue;
#else
	__m128 xmm1, xmm2, xmm3;
	xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)maxs), (const __m64 *)(maxs+1));
	xmm2 = _mm_loadh_pi(_mm_load_ss((const float *)mins2), (const __m64 *)(mins2+1));
	xmm3 = _mm_cmplt_ps(xmm1, xmm2);
	if (_mm_movemask_ps(xmm3) != 0) return qfalse;
	xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)mins), (const __m64 *)(mins+1));
	xmm2 = _mm_loadh_pi(_mm_load_ss((const float *)maxs2), (const __m64 *)(maxs2+1));
	xmm3 = _mm_cmpgt_ps(xmm1, xmm2);
	if (_mm_movemask_ps(xmm3) != 0) return qfalse;
	return qtrue;
#endif
}

/**
 * @brief BoundsIntersectSphere
 * @param[in] mins
 * @param[in] maxs
 * @param[in] origin
 * @param[in] radius
 * @return
 *
 * @note Unused
 * /
qboolean BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs, const vec3_t origin, vec_t radius)
{
	if (origin[0] - radius > maxs[0] || origin[0] + radius < mins[0] ||
	    origin[1] - radius > maxs[1] || origin[1] + radius < mins[1] ||
		origin[2] - radius > maxs[2] || origin[2] + radius < mins[2])
	{
		return qfalse;
	}

	return qtrue;
}*/

/**
 * @brief BoundsIntersectPoint
 * @param[in] mins
 * @param[in] maxs
 * @param[in] origin
 * @return
 *
 * @note Unused
 * /
qboolean BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs, const vec3_t origin)
{
	if (origin[0] > maxs[0] || origin[0] < mins[0] ||
		origin[1] > maxs[1] || origin[1] < mins[1] ||
		origin[2] > maxs[2] || origin[2] < mins[2])
	{
		return qfalse;
	}

	return qtrue;
}*/

/**
 * @brief ZeroBounds
 * @param[out] mins
 * @param[out] maxs
 */
void ZeroBounds(vec3_t mins, vec3_t maxs)
{
	/*mins[0] = mins[1] = mins[2] = 0.0f;
	maxs[0] = maxs[1] = maxs[2] = 0.0f;*/
	VectorClear(mins);
	VectorClear(maxs);
}

#ifndef ETL_SSE
/**
 * @brief MatrixSetupTransformFromQuat
 * @param[out] m
 * @param[in] quat
 * @param[in] origin
 */
void MatrixSetupTransformFromQuat(mat4_t m, const quat_t quat, const vec3_t origin)
{

	mat4_t rot;
	Matrix4FromQuaternion(rot, quat);
	//m[0] = rot[0];   m[4] = rot[4];   m[8] = rot[8];     m[12] = origin[0];
	//m[1] = rot[1];   m[5] = rot[5];   m[9] = rot[9];     m[13] = origin[1];
	//m[2] = rot[2];   m[6] = rot[6];   m[10] = rot[10];   m[14] = origin[2];
	//m[3] = 0.0f;     m[7] = 0.0f;     m[11] = 0.0f;      m[15] = 1.0f;
	VectorCopy(&rot[0], &m[0]);
	VectorCopy(&rot[4], &m[4]);
	VectorCopy(&rot[8], &m[8]);
	VectorCopy(&origin[0], &m[12]);
	m[3] = 0.0f;     m[7] = 0.0f;     m[11] = 0.0f;      m[15] = 1.0f;
/*
	Matrix4FromQuaternion(m, quat);
	VectorCopy(&origin[0], &m[12]);
	m[3] = 0.0f;     m[7] = 0.0f;     m[11] = 0.0f;      m[15] = 1.0f;
*/
}
#endif

/**
 * @brief PlaneNormalize
 * @param[in,out] plane
 * @return
 * /
vec_t PlaneNormalize(vec4_t plane)
{
#ifndef ETL_SSE
	vec_t length, ilength;
	length = sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
	if (length == 0.f)
	{
		VectorClear(plane);
		return 0.0f;
	}
	ilength  = 1.0f / length;
	plane[0] = plane[0] * ilength;
	plane[1] = plane[1] * ilength;
	plane[2] = plane[2] * ilength;
	plane[3] = plane[3] * ilength;
	return length;
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6;
	float s;
	xmm4 = _mm_setzero_ps();
	xmm3 = _mm_loadu_ps((const float *)&plane);		// xmm3 = p3 p2 p1 p0
	xmm2 = _mm_shuffle_ps(xmm4, xmm3, 0b10101111);	// xmm2 = p2 p2 0 0
	xmm2 = _mm_shuffle_ps(xmm3, xmm2, 0b00110100);	// xmm2 = 0 p2 p1 p0
	xmm2 = _mm_mul_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm5 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm5);		//
	xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm5);		//
	xmm0 = _mm_sqrt_ss(xmm2);
	_mm_store_ss(&s, xmm0);
	if (s == 0.0f) {
		_mm_storeu_ps((float *)&plane, xmm4);
	} else {
		xmm1 = _mm_rcp_ss(xmm0);
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
		xmm3 = _mm_mul_ps(xmm3, xmm1);
		_mm_storeu_ps((float *)&plane, xmm3);
	}
	return s;
#endif
}
*/
void PlaneNormalize(vec4_t plane)
{
#ifndef ETL_SSE
	vec_t length, ilength;
	length = sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
	if (length == 0.f)
	{
		VectorClear(plane);
		return;
	}
	ilength = 1.0f / length;
	plane[0] = plane[0] * ilength;
	plane[1] = plane[1] * ilength;
	plane[2] = plane[2] * ilength;
	plane[3] = plane[3] * ilength;
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6;
	xmm4 = _mm_setzero_ps();
	xmm3 = _mm_loadu_ps((const float *)&plane);		// xmm3 = p3 p2 p1 p0
	xmm2 = _mm_shuffle_ps(xmm4, xmm3, 0b10101111);	// xmm2 = p2 p2 0 0
	xmm2 = _mm_shuffle_ps(xmm3, xmm2, 0b00110100);	// xmm2 = 0 p2 p1 p0
	xmm2 = _mm_mul_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	//xmm2 = _mm_hadd_ps(xmm2, xmm2);
	xmm5 = _mm_movehdup_ps(xmm2);		// faster way to do: 2 * hadd
	xmm6 = _mm_add_ps(xmm2, xmm5);		//
	xmm5 = _mm_movehl_ps(xmm5, xmm6);	//
	xmm2 = _mm_add_ss(xmm6, xmm5);		//
	xmm0 = _mm_sqrt_ss(xmm2);
	if (_mm_cvtss_f32(xmm0) == 0.0f) {
		_mm_storeu_ps((float *)&plane, xmm4);
	}
	else {
		xmm1 = _mm_rcp_ss(xmm0);
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0);
		xmm3 = _mm_mul_ps(xmm3, xmm1);
		_mm_storeu_ps((float *)&plane, xmm3);
	}
#endif
}


#ifndef ETL_SSE
/**
 * @brief MatrixTransformNormal
 * @param[in] m
 * @param[in] in
 * @param[out] out
 * Transform the vector using only the 3x3 part of the 4x4 matrix
 */
void MatrixTransformNormal(const mat4_t m, const vec3_t in, vec3_t out)
{
#ifndef ETL_SSE
	out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2];
	out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2];
	out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2];
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5;
	xmm1 = _mm_loadh_pi(_mm_load_ss(&in[0]), (const __m64 *)(&in[1]));	// xmm1 = z y _ x
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111);		// xmm2 = z z z z
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010);		// xmm1 = y y y y
	xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b00000000);		// xmm0 = x x x x
	xmm3 = _mm_loadu_ps(&m[0]);							// xmm3 =  m3  m2  m1  m0
	xmm4 = _mm_loadu_ps(&m[4]);							// xmm4 =  m7  m6  m5  m4
	xmm5 = _mm_loadu_ps(&m[8]);							// xmm5 = m11 m10  m9  m8
	xmm0 = _mm_mul_ps(xmm0, xmm3);
	xmm1 = _mm_mul_ps(xmm1, xmm4);
	xmm2 = _mm_mul_ps(xmm2, xmm5);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm2);
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b10010100);
	_mm_store_ss(&out[0], xmm0);
	_mm_storeh_pi((__m64 *)(&out[1]), xmm0);
#endif
}
#endif

/**
 * @brief MatrixTransformNormal2
 * @param[in] m
 * @param[in,out] inout
 * Same as MatrixTransformNormal(), except for the argument(s)
 */
void MatrixTransformNormal2(const mat4_t m, vec3_t inout)
{
#ifndef ETL_SSE
	vec3_t tmp;
	tmp[0] = m[0] * inout[0] + m[4] * inout[1] + m[8] * inout[2];
	tmp[1] = m[1] * inout[0] + m[5] * inout[1] + m[9] * inout[2];
	tmp[2] = m[2] * inout[0] + m[6] * inout[1] + m[10] * inout[2];
	VectorCopy(tmp, inout);
#else
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5;
	xmm1 = _mm_loadh_pi(_mm_load_ss(&inout[0]), (const __m64 *)(&inout[1]));	// xmm1 = z y _ x
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111);			// xmm2 = z z z z
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010);			// xmm1 = y y y y
	xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b00000000);			// xmm0 = x x x x
	xmm3 = _mm_loadu_ps(&m[0]);								// xmm3 =  m3  m2  m1  m0
	xmm4 = _mm_loadu_ps(&m[4]);								// xmm4 =  m7  m6  m5  m4
	xmm5 = _mm_loadu_ps(&m[8]);								// xmm5 = m11 m10  m9  m8
	xmm0 = _mm_mul_ps(xmm0, xmm3);
	xmm1 = _mm_mul_ps(xmm1, xmm4);
	xmm2 = _mm_mul_ps(xmm2, xmm5);
	xmm0 = _mm_add_ps(xmm0, xmm1);
	xmm0 = _mm_add_ps(xmm0, xmm2);
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b10010100);
	_mm_store_ss(&inout[0], xmm0);
	_mm_storeh_pi((__m64 *)(&inout[1]), xmm0);
#endif
}

#ifndef ETL_SSE
/**
 * @brief MatrixTransformPlane
 * @param[in] m
 * @param[in] in
 * @param[out] out
 */
void MatrixTransformPlane(const mat4_t m, const vec4_t in, vec4_t out)
{
	vec3_t translation;
	vec3_t planePos;
	// rotate the plane normal
	MatrixTransformNormal(m, in, out);
	// add new position to current plane position
	VectorSet(translation, m[12], m[13], m[14]);
	VectorMA(translation, in[3], out, planePos);
	// the plane-distance to the origin
	//out[3] = DotProduct(out, planePos);
	Dot(out, planePos, out[3]);
}
#endif

/**
 * @brief MatrixTransformPlane2
 * @param[in] m
 * @param[in,out] inout
 *
 * @note unused
 */
void MatrixTransformPlane2(const mat4_t m, vec4_t inout)
{
	vec4_t tmp;
	MatrixTransformPlane(m, inout, tmp);
	Vector4Copy(tmp, inout);
}

#ifndef ETL_SSE
/**
 * @brief MatrixTransformBounds
 * @param[in] m
 * @param[in] mins
 * @param[in] mins
 * @param[out] omins
 * @param[out] omax
 * Achieves the same result as:
 *
 *   BoundsClear(omins, omaxs);
 *	for each corner c in bounds{imins, imaxs}
 *   {
 *	    vec3_t p;
 *		MatrixTransformPoint(m, c, p);
 *       AddPointToBounds(p, omins, omaxs);
 *   }
 *
 * With fewer operations
 *
 * Pseudocode:
 *	omins = min(mins.x*m.c1, maxs.x*m.c1) + min(mins.y*m.c2, maxs.y*m.c2) + min(mins.z*m.c3, maxs.z*m.c3) + c4
 *	omaxs = max(mins.x*m.c1, maxs.x*m.c1) + max(mins.y*m.c2, maxs.y*m.c2) + max(mins.z*m.c3, maxs.z*m.c3) + c4
 */
void MatrixTransformBounds(const mat4_t m, const vec3_t mins, const vec3_t maxs, vec3_t omins, vec3_t omaxs)
{
	vec3_t tmins, tmaxs;
	vec3_t minx, maxx;
	vec3_t miny, maxy;
	vec3_t minz, maxz;
	vec3_t tmp;

	const float* c0 = m;
	const float* c1 = m + 4;
	const float* c2 = m + 8;
	const float* c3 = m + 12;

	VectorScale(c0, mins[0], minx);
	VectorScale(c0, maxs[0], maxx);

	VectorScale(c1, mins[1], miny);
	VectorScale(c1, maxs[1], maxy);

	VectorScale(c2, mins[2], minz);
	VectorScale(c2, maxs[2], maxz);

	VectorMin(minx, maxx, tmins);
	VectorMax(minx, maxx, tmaxs);
	VectorAdd(tmins, c3, tmins);
	VectorAdd(tmaxs, c3, tmaxs);

	VectorMin(miny, maxy, tmp);
	VectorAdd(tmp, tmins, tmins);

	VectorMax(miny, maxy, tmp);
	VectorAdd(tmp, tmaxs, tmaxs);

	VectorMin(minz, maxz, tmp);
	VectorAdd(tmp, tmins, omins);

	VectorMax(minz, maxz, tmp);
	VectorAdd(tmp, tmaxs, omaxs);
/* this is not the same..
	VectorTransformM4(m, mins, tmins); // VectorRotate
	VectorTransformM4(m, maxs, tmaxs); // VectorRotate
	VectorMin(tmins, tmaxs, omins);
	VectorMax(tmins, tmaxs, omaxs);
*/
}
#endif

#ifndef ETL_SSE
/**
 * @brief MatrixPerspectiveProjectionFovXYInfiniteRH
 * @param[out] m
 * @param[in] fovX
 * @param[in] fovY
 * @param[in] nearvec
 *
 * @note far plane at infinity, see RobustShadowVolumes.pdf by Nvidia
 */
void MatrixPerspectiveProjectionFovXYInfiniteRH(mat4_t m, vec_t fovX, vec_t fovY, vec_t nearvec)
{
	vec_t width = tanf(DEG2RAD(fovX * 0.5f)),
		height = tanf(DEG2RAD(fovY * 0.5f));
#ifndef ETL_SSE
	m[0] = 1.0f / width;   m[4] = 0.0f;           m[8] = 0.0f;      m[12] = 0.0f;
	m[1] = 0.0f;           m[5] = 1.0f / height;  m[9] = 0.0f;      m[13] = 0.0f;
	m[2] = 0.0f;           m[6] = 0.0f;           m[10] = -1.0f;    m[14] = -2.0f * nearvec;
	m[3] = 0.0f;           m[7] = 0.0f;           m[11] = -1.0f;    m[15] = 0.0f;
#else
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f,  0.0f,            0.0f,          1.0f / width));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f,  0.0f,            1.0f / height, 0.0f));
	_mm_storeu_ps(&m[8], _mm_set_ps(-1.0f, -1.0f,           0.0f,          0.0f));
	_mm_storeu_ps(&m[12], _mm_set_ps(0.0f, -2.0f * nearvec, 0.0f,          0.0f));
#endif
}
#endif

/**
 * @brief QuatTransformVector
 * @param[in] q
 * @param[in] in
 * @param[out] out
 */
void QuatTransformVector(const quat_t q, const vec3_t in, vec3_t out)
{
	mat4_t m;
	Matrix4FromQuaternion(m, q);
	MatrixTransformNormal(m, in, out);
}

/**
 * @brief QuatMultiply0
 * @param[in,out] qa
 * @param[in] qb
 *
 * @note Unused
 * /
void QuatMultiply0(quat_t qa, const quat_t qb)
{
	quat_t tmp;
	quat_copy(qa, tmp);
	QuatMultiply1(tmp, qb, qa);
}*/

/**
 * @brief QuatMultiply1
 * @param[in] qa
 * @param[in] qb
 * @param[out] qc
 */
void QuatMultiply1(const quat_t qa, const quat_t qb, quat_t qc)
{
	// from matrix and quaternion faq
	//x = w1x2 + x1w2 + y1z2 - z1y2
	//y = w1y2 + y1w2 + z1x2 - x1z2
	//z = w1z2 + z1w2 + x1y2 - y1x2
	//w = w1w2 - x1x2 - y1y2 - z1z2
	qc[0] = qa[3] * qb[0] + qa[0] * qb[3] + qa[1] * qb[2] - qa[2] * qb[1];
	qc[1] = qa[3] * qb[1] + qa[1] * qb[3] + qa[2] * qb[0] - qa[0] * qb[2];
	qc[2] = qa[3] * qb[2] + qa[2] * qb[3] + qa[0] * qb[1] - qa[1] * qb[0];
	qc[3] = qa[3] * qb[3] - qa[0] * qb[0] - qa[1] * qb[1] - qa[2] * qb[2];
}

/**
 * @brief QuatMultiply2
 * @param[in] qa
 * @param[in] qb
 * @param[out] qc
 *
 * @note Unused
 * /
void QuatMultiply2(const quat_t qa, const quat_t qb, quat_t qc)
{
	qc[0] = qa[3] * qb[0] + qa[0] * qb[3] + qa[1] * qb[2] + qa[2] * qb[1];
	qc[1] = qa[3] * qb[1] - qa[1] * qb[3] - qa[2] * qb[0] + qa[0] * qb[2];
	qc[2] = qa[3] * qb[2] - qa[2] * qb[3] - qa[0] * qb[1] + qa[1] * qb[0];
	qc[3] = qa[3] * qb[3] - qa[0] * qb[0] - qa[1] * qb[1] + qa[2] * qb[2];
}*/

/**
 * @brief QuatMultiply3
 * @param[in] qa
 * @param[in] qb
 * @param[out] qc
 *
 * @note Unused
 * /
void QuatMultiply3(const quat_t qa, const quat_t qb, quat_t qc)
{
	qc[0] = qa[3] * qb[0] + qa[0] * qb[3] + qa[1] * qb[2] + qa[2] * qb[1];
	qc[1] = -qa[3] * qb[1] + qa[1] * qb[3] - qa[2] * qb[0] + qa[0] * qb[2];
	qc[2] = -qa[3] * qb[2] + qa[2] * qb[3] - qa[0] * qb[1] + qa[1] * qb[0];
	qc[3] = -qa[3] * qb[3] + qa[0] * qb[0] - qa[1] * qb[1] + qa[2] * qb[2];
}*/

/**
 * @brief QuatMultiply4
 * @param[in] qa
 * @param[in] qb
 * @param[out] qc
 *
 * @note Unused
 * /
void QuatMultiply4(const quat_t qa, const quat_t qb, quat_t qc)
{
	qc[0] = qa[3] * qb[0] - qa[0] * qb[3] - qa[1] * qb[2] - qa[2] * qb[1];
	qc[1] = -qa[3] * qb[1] - qa[1] * qb[3] + qa[2] * qb[0] - qa[0] * qb[2];
	qc[2] = -qa[3] * qb[2] - qa[2] * qb[3] + qa[0] * qb[1] - qa[1] * qb[0];
	qc[3] = -qa[3] * qb[3] - qa[0] * qb[0] + qa[1] * qb[1] - qa[2] * qb[2];
}*/

/**
 * @brief MatrixSetupShear
 * @param[out] m
 * @param[in] x
 * @param[in] y
 */
void MatrixSetupShear(mat4_t m, vec_t x, vec_t y)
{
#ifndef ETL_SSE
	m[0] = 1.0f;      m[4] = x;         m[8] = 0.0f;       m[12] = 0.0f;
	m[1] = y;         m[5] = 1.0f;      m[9] = 0.0f;       m[13] = 0.0f;
	m[2] = 0.0f;      m[6] = 0.0f;      m[10] = 1.0f;      m[14] = 0.0f;
	m[3] = 0.0f;      m[7] = 0.0f;      m[11] = 0.0f;      m[15] = 1.0f;
#else
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, y, 1.0f));
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, 1.0f, x));
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f));
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
#endif
}

/**
 * @brief MatrixMultiplyShear
 * @param[in,out] m
 * @param[in] x
 * @param[in] y
 */
void MatrixMultiplyShear(mat4_t m, vec_t x, vec_t y)
{
	mat4_t tmp, shear;
	Matrix4Copy(m, tmp);
#ifndef ETL_SSE
	MatrixSetupShear(shear, x, y);
#else
	_mm_storeu_ps(&shear[0], _mm_set_ps(0.0f, 0.0f, y, 1.0f));
	_mm_storeu_ps(&shear[4], _mm_set_ps(0.0f, 0.0f, 1.0f, x));
	_mm_storeu_ps(&shear[8], _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f));
	_mm_storeu_ps(&shear[12], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));
#endif
	Matrix4Multiply(tmp, shear, m);
}

#ifndef ETL_SSE
/**
 * @brief MatrixFromPlanes
 * @param[out] m
 * @param[in] left
 * @param[in] right
 * @param[in] bottom
 * @param[in] top
 * @param[in] nearvec
 * @param[in] farvec
 */
void MatrixFromPlanes(mat4_t m, const vec4_t left, const vec4_t right, const vec4_t bottom, const vec4_t top, const vec4_t nearvec, const vec4_t farvec)
{
	vec4_t vrl, vtb, vfn, col3;
	Vector4Subtract(right, left, vrl);
	Vector4Subtract(top, bottom, vtb);
	Vector4Subtract(farvec, nearvec, vfn);
	Vector4Scale(vrl, 0.5f, vrl);
	Vector4Scale(vtb, 0.5f, vtb);
	Vector4Scale(vfn, 0.5f, vfn);
	Vector4Subtract(right, vrl, col3);
/*
	m[0] = (right[0] - left[0]) * 0.5f;
	m[1] = (top[0] - bottom[0]) * 0.5f;
	m[2] = (farvec[0] - nearvec[0]) * 0.5f;
	m[3] = right[0] - m[0]; // (right[0] - left[0]) * 0.5f;

	m[4] = (right[1] - left[1]) * 0.5f;
	m[5] = (top[1] - bottom[1]) * 0.5f;
	m[6] = (farvec[1] - nearvec[1]) * 0.5f;
	m[7] = right[1] - m[4]; // (right[1] - left[1]) * 0.5f;

	m[8]  = (right[2] - left[2]) * 0.5f;
	m[9]  = (top[2] - bottom[2]) * 0.5f;
	m[10] = (farvec[2] - nearvec[2]) * 0.5f;
	m[11] = right[2] - m[8]; // (right[2] - left[2]) * 0.5f;
*/
#ifndef ETL_SSE
	m[0] = vrl[0];
	m[1] = vtb[0];
	m[2] = vfn[0];
	m[3] = col3[0];

	m[4] = vrl[1];
	m[5] = vtb[1];
	m[6] = vfn[1];
	m[7] = col3[1];

	m[8]  = vrl[2];
	m[9]  = vtb[2];
	m[10] = vfn[2];
	m[11] = col3[2];
#else
	_mm_storeu_ps(&m[0], _mm_set_ps(col3[0], vfn[0], vtb[0], vrl[0]));
	_mm_storeu_ps(&m[4], _mm_set_ps(col3[1], vfn[1], vtb[1], vrl[1]));
	_mm_storeu_ps(&m[8], _mm_set_ps(col3[2], vfn[2], vtb[2], vrl[2]));
#endif

#if 0
	m[12] = (right[3] - left[3]) / 2;
	m[13] = (top[3] - bottom[3]) / 2;
	m[14] = (farvec[3] - nearvec[3]) / 2;
	m[15] = right[3] - (right[3] - left[3]) / 2;
#else
#ifndef ETL_SSE
	m[12] = (-right[3] - -left[3]) * 0.5f;
	m[13] = (-top[3] - -bottom[3]) * 0.5f;
	m[14] = (-farvec[3] - -nearvec[3]) * 0.5f;
	m[15] = -right[3] - m[12]; // (-right[3] - -left[3]) * 0.5f;
#else
	_mm_storeu_ps(&m[12], _mm_set_ps((-right[3]) - m[12], ((-farvec[3]) - (-nearvec[3])) * 0.5f, ((-top[3]) - (-bottom[3])) * 0.5f, ((-right[3]) - (-left[3])) * 0.5f));
#endif
#endif
}
#endif

/**
 * @brief Replaces content of find by replace in dest
 * @param[out] dest
 * @param[in] destsize
 * @param[in] find
 * @param[in] replace
 * @return
 */
qboolean Q_strreplace(char *dest, size_t destsize, const char *find, const char *replace)
{
	size_t lend;
	char   *s;
	char   backup[32000];           // big, but small enough to fit in PPC stack

	lend = strlen(dest);
	if (lend >= destsize)
	{
		Ren_Fatal("Q_strreplace: already overflowed");
	}

	s = strstr(dest, find);
	if (!s)
	{
		return qfalse;
	}
	else
	{
		size_t lstart, lfind, lreplace;

		Q_strncpyz(backup, dest, lend + 1);
		lstart   = s - dest;
		lfind    = strlen(find);
		lreplace = strlen(replace);

		strncpy(s, replace, destsize - lstart - 1);
		strncpy(s + lreplace, backup + lstart + lfind, destsize - lstart - lreplace - 1);

		return qtrue;
	}
}

//=============================================================================

/**
 * @brief AllocMemStream
 * @param[in] buffer
 * @param[in] bufSize
 * @return
 */
memStream_t *AllocMemStream(byte *buffer, int bufSize)
{
	memStream_t *s;

	if (buffer == NULL || bufSize <= 0)
	{
		return NULL;
	}

	s = Com_Allocate(sizeof(memStream_t));
	if (s == NULL)
	{
		return NULL;
	}

	Com_Memset(s, 0, sizeof(memStream_t));

	s->buffer  = buffer;
	s->curPos  = buffer;
	s->bufSize = bufSize;
	s->flags   = 0;

	return s;
}

/**
 * @brief FreeMemStream
 * @param[in] s
 */
void FreeMemStream(memStream_t *s)
{
	Com_Dealloc(s);
}

/**
 * @brief MemStreamRead
 * @param[in,out] s
 * @param[out] buffer
 * @param[in] len
 * @return
 */
qboolean MemStreamRead(memStream_t *s, void *buffer, unsigned int len)
{
	if (s == NULL || buffer == NULL)
	{
		return qfalse;
	}

	if (s->curPos + len > s->buffer + s->bufSize)
	{
		s->flags |= MEMSTREAM_FLAGS_EOF;
		// len       = s->buffer + s->bufSize - s->curPos;

		Ren_Fatal("MemStreamRead: EOF reached");
	}

	Com_Memcpy(buffer, s->curPos, len);
	s->curPos += len;

	return qtrue;
}

/**
 * @brief MemStreamGetC
 * @param[in] s
 * @return
 */
int MemStreamGetC(memStream_t *s)
{
	int c = 0;

	if (s == NULL)
	{
		return -1;
	}

	if (MemStreamRead(s, &c, 1) == qfalse)
	{
		return -1;
	}

	return c;
}

/**
 * @brief MemStreamGetLong
 * @param[in] s
 * @return
 */
int MemStreamGetLong(memStream_t *s)
{
	int c = 0;

	if (s == NULL)
	{
		return -1;
	}

	if (MemStreamRead(s, &c, 4) == qfalse)
	{
		return -1;
	}

	return LittleLong(c);
}

/**
 * @brief MemStreamGetShort
 * @param[in] s
 * @return
 */
int MemStreamGetShort(memStream_t *s)
{
	int c = 0;

	if (s == NULL)
	{
		return -1;
	}

	if (MemStreamRead(s, &c, 2) == qfalse)
	{
		return -1;
	}

	return LittleShort(c);
}

/**
 * @brief MemStreamGetFloat
 * @param[in] s
 * @return
 */
float MemStreamGetFloat(memStream_t *s)
{
	floatint_t c;

	if (s == NULL)
	{
		return -1;
	}

	if (MemStreamRead(s, &c.i, 4) == qfalse)
	{
		return -1;
	}

	return LittleFloat(c.f);
}

//============================================================================

/**
 * @brief printBits
 * @param[in] size
 * @param[in] ptr
 *
 * @note Unused
 */
void printBits(size_t const size, void const *const ptr)
{
	unsigned char *b = (unsigned char *) ptr;
	unsigned char byte;
	int           i, j;

	for (i = size - 1; i >= 0; i--)
	{
		for (j = 7; j >= 0; j--)
		{
			byte   = b[i] & (1 << j);
			byte >>= j;
			Ren_Developer("%u", byte);
		}
	}
}
