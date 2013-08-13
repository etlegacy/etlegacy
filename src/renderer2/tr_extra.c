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

#include "tr_local.h"

#ifdef __cplusplus
extern "C" {
#endif

matrix_t matrixIdentity = { 1, 0, 0, 0,
	                        0, 1, 0, 0,
	                        0, 0, 1, 0,
	                        0, 0, 0, 1 };
quat_t   quatIdentity = { 0, 0, 0, 1 };

int NearestPowerOfTwo(int val)
{
	int answer;

	for (answer = 1; answer < val; answer <<= 1)
		;
	return answer;
}

qboolean MatrixCompare(const matrix_t a, const matrix_t b)
{
	return (a[0] == b[0] && a[4] == b[4] && a[8] == b[8] && a[12] == b[12] &&
	        a[1] == b[1] && a[5] == b[5] && a[9] == b[9] && a[13] == b[13] &&
	        a[2] == b[2] && a[6] == b[6] && a[10] == b[10] && a[14] == b[14] &&
	        a[3] == b[3] && a[7] == b[7] && a[11] == b[11] && a[15] == b[15]);
}

void MatrixCopy(const matrix_t in, matrix_t out)
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

/*
replacement for glOrtho
see glspec30.pdf chapter 2.12 Coordinate Transformations
*/
void MatrixOrthogonalProjection(matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec)
{
	m[0] = 2 / (right - left);  m[4] = 0;                   m[8] = 0;                   m[12] = -(right + left) / (right - left);
	m[1] = 0;                   m[5] = 2 / (top - bottom);  m[9] = 0;                   m[13] = -(top + bottom) / (top - bottom);
	m[2] = 0;                   m[6] = 0;                   m[10] = -2 / (farvec - nearvec);    m[14] = -(farvec + nearvec) / (farvec - nearvec);
	m[3] = 0;                   m[7] = 0;                   m[11] = 0;                  m[15] = 1;
}

void MatrixTransform4(const matrix_t m, const vec4_t in, vec4_t out)
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

void MatrixSetupTranslation(matrix_t m, vec_t x, vec_t y, vec_t z)
{
	m[0] = 1;      m[4] = 0;      m[8] = 0;      m[12] = x;
	m[1] = 0;      m[5] = 1;      m[9] = 0;      m[13] = y;
	m[2] = 0;      m[6] = 0;      m[10] = 1;      m[14] = z;
	m[3] = 0;      m[7] = 0;      m[11] = 0;      m[15] = 1;
}

void MatrixSetupScale(matrix_t m, vec_t x, vec_t y, vec_t z)
{
	m[0] = x;      m[4] = 0;      m[8] = 0;      m[12] = 0;
	m[1] = 0;      m[5] = y;      m[9] = 0;      m[13] = 0;
	m[2] = 0;      m[6] = 0;      m[10] = z;      m[14] = 0;
	m[3] = 0;      m[7] = 0;      m[11] = 0;      m[15] = 1;
}

void MatrixMultiplyMOD(const matrix_t a, const matrix_t b, matrix_t out)
{
#if id386_sse
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

void MatrixMultiply2(matrix_t m, const matrix_t m2)
{
	matrix_t tmp;

	MatrixCopy(m, tmp);
	MatrixMultiplyMOD(tmp, m2, m);
}

qboolean PlanesGetIntersectionPoint(const vec4_t plane1, const vec4_t plane2, const vec4_t plane3, vec3_t out)
{
	// http://www.cgafaq.info/wiki/Intersection_of_three_planes

	vec3_t n1, n2, n3;
	vec3_t n1n2, n2n3, n3n1;
	vec_t  denom;

	VectorNormalize2(plane1, n1);
	VectorNormalize2(plane2, n2);
	VectorNormalize2(plane3, n3);

	CrossProduct(n1, n2, n1n2);
	CrossProduct(n2, n3, n2n3);
	CrossProduct(n3, n1, n3n1);

	denom = DotProduct(n1, n2n3);

	// check if the denominator is zero (which would mean that no intersection is to be found
	if (denom == 0)
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

void MatrixMultiplyScale(matrix_t m, vec_t x, vec_t y, vec_t z)
{
#if 0
	matrix_t tmp, scale;

	MatrixCopy(m, tmp);
	MatrixSetupScale(scale, x, y, z);
	MatrixMultiplyMOD(tmp, scale, m);
#else
	m[0] *= x;     m[4] *= y;        m[8] *= z;
	m[1] *= x;     m[5] *= y;        m[9] *= z;
	m[2] *= x;     m[6] *= y;        m[10] *= z;
	m[3] *= x;     m[7] *= y;        m[11] *= z;
#endif
}

void MatrixFromAngles(matrix_t m, vec_t pitch, vec_t yaw, vec_t roll)
{
	static float sr, sp, sy, cr, cp, cy;

	// static to help MS compiler fp bugs
	sp = sin(DEG2RAD(pitch));
	cp = cos(DEG2RAD(pitch));

	sy = sin(DEG2RAD(yaw));
	cy = cos(DEG2RAD(yaw));

	sr = sin(DEG2RAD(roll));
	cr = cos(DEG2RAD(roll));

	m[0] = cp * cy;  m[4] = (sr * sp * cy + cr * -sy);      m[8] = (cr * sp * cy + -sr * -sy);     m[12] = 0;
	m[1] = cp * sy;  m[5] = (sr * sp * sy + cr * cy);       m[9] = (cr * sp * sy + -sr * cy);      m[13] = 0;
	m[2] = -sp;    m[6] = sr * cp;                  m[10] = cr * cp;                  m[14] = 0;
	m[3] = 0;      m[7] = 0;                      m[11] = 0;                      m[15] = 1;
}

void MatrixSetupTransformFromRotation(matrix_t m, const matrix_t rot, const vec3_t origin)
{
	m[0] = rot[0];     m[4] = rot[4];        m[8] = rot[8];  m[12] = origin[0];
	m[1] = rot[1];     m[5] = rot[5];        m[9] = rot[9];  m[13] = origin[1];
	m[2] = rot[2];     m[6] = rot[6];        m[10] = rot[10];  m[14] = origin[2];
	m[3] = 0;           m[7] = 0;              m[11] = 0;        m[15] = 1;
}

void MatrixAffineInverse(const matrix_t in, matrix_t out)
{
#if 0
	MatrixCopy(in, out);
	MatrixInverse(out);
#else
	// Tr3B - cleaned up
	out[0] = in[0];       out[4] = in[1];       out[8] = in[2];
	out[1] = in[4];       out[5] = in[5];       out[9] = in[6];
	out[2] = in[8];       out[6] = in[9];       out[10] = in[10];
	out[3] = 0;            out[7] = 0;            out[11] = 0;            out[15] = 1;

	out[12] = -(in[12] * out[0] + in[13] * out[4] + in[14] * out[8]);
	out[13] = -(in[12] * out[1] + in[13] * out[5] + in[14] * out[9]);
	out[14] = -(in[12] * out[2] + in[13] * out[6] + in[14] * out[10]);
#endif
}

void MatrixPerspectiveProjectionFovXYRH(matrix_t m, vec_t fovX, vec_t fovY, vec_t nearvec, vec_t farvec)
{
	vec_t width, height;

	width  = tanf(DEG2RAD(fovX * 0.5f));
	height = tanf(DEG2RAD(fovY * 0.5f));

	m[0] = 1 / width;   m[4] = 0;           m[8] = 0;                       m[12] = 0;
	m[1] = 0;           m[5] = 1 / height;  m[9] = 0;                       m[13] = 0;
	m[2] = 0;           m[6] = 0;           m[10] = farvec / (nearvec - farvec);        m[14] = (nearvec * farvec) / (nearvec - farvec);
	m[3] = 0;           m[7] = 0;           m[11] = -1;                     m[15] = 0;
}

void MatrixLookAtRH(matrix_t m, const vec3_t eye, const vec3_t dir, const vec3_t up)
{
	vec3_t dirN;
	vec3_t upN;
	vec3_t sideN;

	CrossProduct(dir, up, sideN);
	VectorNormalize(sideN);

	CrossProduct(sideN, dir, upN);
	VectorNormalize(upN);

	VectorNormalize2(dir, dirN);

	m[0] = sideN[0];   m[4] = sideN[1];       m[8] = sideN[2];       m[12] = -DotProduct(sideN, eye);
	m[1] = upN[0];     m[5] = upN[1];         m[9] = upN[2];         m[13] = -DotProduct(upN, eye);
	m[2] = -dirN[0];   m[6] = -dirN[1];       m[10] = -dirN[2];       m[14] = DotProduct(dirN, eye);
	m[3] = 0;          m[7] = 0;              m[11] = 0;              m[15] = 1;
}

void PlaneIntersectRay(const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res)
{
	vec3_t dir;
	float  sect;

	VectorNormalize2(rayDir, dir);

	sect = -(DotProduct(plane, rayPos) - plane[3]) / DotProduct(plane, rayDir);
	VectorScale(dir, sect, dir);
	VectorAdd(rayPos, dir, res);
}

/*
same as D3DXMatrixOrthoOffCenterRH

http://msdn.microsoft.com/en-us/library/bb205348(VS.85).aspx
*/
void MatrixOrthogonalProjectionRH(matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec)
{
	m[0] = 2 / (right - left);  m[4] = 0;                   m[8] = 0;                   m[12] = (left + right) / (left - right);
	m[1] = 0;                   m[5] = 2 / (top - bottom);  m[9] = 0;                   m[13] = (top + bottom) / (bottom - top);
	m[2] = 0;                   m[6] = 0;                   m[10] = 1 / (nearvec - farvec); m[14] = nearvec / (nearvec - farvec);
	m[3] = 0;                   m[7] = 0;                   m[11] = 0;                  m[15] = 1;
}

void MatrixCrop(matrix_t m, const vec3_t mins, const vec3_t maxs)
{
	float scaleX, scaleY, scaleZ;
	float offsetX, offsetY, offsetZ;

	scaleX = 2.0f / (maxs[0] - mins[0]);
	scaleY = 2.0f / (maxs[1] - mins[1]);

	offsetX = -0.5f * (maxs[0] + mins[0]) * scaleX;
	offsetY = -0.5f * (maxs[1] + mins[1]) * scaleY;

	scaleZ  = 1.0f / (maxs[2] - mins[2]);
	offsetZ = -mins[2] * scaleZ;

	m[0] = scaleX;     m[4] = 0;          m[8] = 0;          m[12] = offsetX;
	m[1] = 0;          m[5] = scaleY;     m[9] = 0;          m[13] = offsetY;
	m[2] = 0;          m[6] = 0;          m[10] = scaleZ;     m[14] = offsetZ;
	m[3] = 0;          m[7] = 0;          m[11] = 0;          m[15] = 1;
}

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

// *INDENT-OFF*
void MatrixIdentity(matrix_t m)
{
	m[0] = 1;      m[4] = 0;      m[8] = 0;      m[12] = 0;
	m[1] = 0;      m[5] = 1;      m[9] = 0;      m[13] = 0;
	m[2] = 0;      m[6] = 0;      m[10] = 1;      m[14] = 0;
	m[3] = 0;      m[7] = 0;      m[11] = 0;      m[15] = 1;
}

void MatrixMultiplyTranslation(matrix_t m, vec_t x, vec_t y, vec_t z)
{
#if 1
	matrix_t tmp, trans;

	MatrixCopy(m, tmp);
	MatrixSetupTranslation(trans, x, y, z);
	MatrixMultiplyMOD(tmp, trans, m);
#else
	m[12] += m[0] * x + m[4] * y + m[8] * z;
	m[13] += m[1] * x + m[5] * y + m[9] * z;
	m[14] += m[2] * x + m[6] * y + m[10] * z;
	m[15] += m[3] * x + m[7] * y + m[11] * z;
#endif
}

void MatrixSetupZRotation(matrix_t m, vec_t degrees)
{
	vec_t a = DEG2RAD(degrees);

	m[0] = cos(a);         m[4] = -sin(a);         m[8] = 0;      m[12] = 0;
	m[1] = sin(a);         m[5] = cos(a);         m[9] = 0;      m[13] = 0;
	m[2] = 0;              m[6] = 0;              m[10] = 1;      m[14] = 0;
	m[3] = 0;              m[7] = 0;              m[11] = 0;      m[15] = 1;
}

void MatrixMultiplyZRotation(matrix_t m, vec_t degrees)
{
	matrix_t tmp, rot;

	MatrixCopy(m, tmp);
	MatrixSetupZRotation(rot, degrees);

	MatrixMultiplyMOD(tmp, rot, m);
}

void MatrixFromVectorsFLU(matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up)
{
	m[0] = forward[0];     m[4] = left[0];        m[8] = up[0];  m[12] = 0;
	m[1] = forward[1];     m[5] = left[1];        m[9] = up[1];  m[13] = 0;
	m[2] = forward[2];     m[6] = left[2];        m[10] = up[2];  m[14] = 0;
	m[3] = 0;              m[7] = 0;              m[11] = 0;      m[15] = 1;
}

void MatrixTransformPoint(const matrix_t m, const vec3_t in, vec3_t out)
{
	out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12];
	out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13];
	out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14];
}

void MatrixTransformPoint2(const matrix_t m, vec3_t inout)
{
	vec3_t tmp;

	tmp[0] = m[0] * inout[0] + m[4] * inout[1] + m[8] * inout[2] + m[12];
	tmp[1] = m[1] * inout[0] + m[5] * inout[1] + m[9] * inout[2] + m[13];
	tmp[2] = m[2] * inout[0] + m[6] * inout[1] + m[10] * inout[2] + m[14];

	VectorCopy(tmp, inout);
}

void MatrixSetupTransformFromVectorsFLU(matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up, const vec3_t origin)
{
	m[0] = forward[0];     m[4] = left[0];        m[8] = up[0];  m[12] = origin[0];
	m[1] = forward[1];     m[5] = left[1];        m[9] = up[1];  m[13] = origin[1];
	m[2] = forward[2];     m[6] = left[2];        m[10] = up[2];  m[14] = origin[2];
	m[3] = 0;              m[7] = 0;              m[11] = 0;      m[15] = 1;
}

void MatrixToVectorsFLU(const matrix_t m, vec3_t forward, vec3_t left, vec3_t up)
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

qboolean BoundsIntersect(const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2)
{
	if (maxs[0] < mins2[0] ||
	    maxs[1] < mins2[1] || maxs[2] < mins2[2] || mins[0] > maxs2[0] || mins[1] > maxs2[1] || mins[2] > maxs2[2])
	{
		return qfalse;
	}

	return qtrue;
}

qboolean BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs, const vec3_t origin, vec_t radius)
{
	if (origin[0] - radius > maxs[0] ||
	    origin[0] + radius < mins[0] ||
	                         origin[1] - radius > maxs[1] ||
	    origin[1] + radius < mins[1] || origin[2] - radius > maxs[2] || origin[2] + radius < mins[2])
	{
		return qfalse;
	}

	return qtrue;
}

qboolean BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs, const vec3_t origin)
{
	if (origin[0] > maxs[0] ||
	    origin[0] < mins[0] || origin[1] > maxs[1] || origin[1] < mins[1] || origin[2] > maxs[2] || origin[2] < mins[2])
	{
		return qfalse;
	}

	return qtrue;
}

void MatrixTranspose(const matrix_t in, matrix_t out)
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

/*
============
COM_StripExtension3

RB: ioquake3 version
============
*/
void COM_StripExtension3(const char *src, char *dest, int destsize)
{
	int length;

	Q_strncpyz(dest, src, destsize);

	length = strlen(dest) - 1;

	while (length > 0 && dest[length] != '.')
	{
		length--;

		if (dest[length] == '/')
		{
			return;             // no extension
		}
	}

	if (length)
	{
		dest[length] = 0;
	}
}

void QuatFromMatrix(quat_t q, const matrix_t m)
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

	QuatNormalize(q);
#endif
}

void ZeroBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 0;
	maxs[0] = maxs[1] = maxs[2] = 0;
}

void MatrixFromQuat(matrix_t m, const quat_t q)
{
#if 1
	/*
	From Quaternion to Matrix and Back
	February 27th 2005
	J.M.P. van Waveren

	http://www.intel.com/cd/ids/developer/asmo-na/eng/293748.htm
	*/
	float x2, y2, z2, w2;
	float yy2, xy2;
	float xz2, yz2, zz2;
	float wz2, wy2, wx2, xx2;

	x2 = q[0] + q[0];
	y2 = q[1] + q[1];
	z2 = q[2] + q[2];
	w2 = q[3] + q[3];

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

void MatrixSetupTransformFromQuat(matrix_t m, const quat_t quat, const vec3_t origin)
{
	matrix_t rot;

	MatrixFromQuat(rot, quat);

	m[0] = rot[0];     m[4] = rot[4];        m[8] = rot[8];  m[12] = origin[0];
	m[1] = rot[1];     m[5] = rot[5];        m[9] = rot[9];  m[13] = origin[1];
	m[2] = rot[2];     m[6] = rot[6];        m[10] = rot[10];  m[14] = origin[2];
	m[3] = 0;           m[7] = 0;              m[11] = 0;        m[15] = 1;
}

vec_t PlaneNormalize(vec4_t plane)
{
	vec_t length, ilength;

	length = sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
	if (length == 0)
	{
		VectorClear(plane);
		return 0;
	}

	ilength  = 1.0 / length;
	plane[0] = plane[0] * ilength;
	plane[1] = plane[1] * ilength;
	plane[2] = plane[2] * ilength;
	plane[3] = plane[3] * ilength;

	return length;
}

void MatrixTransformNormal(const matrix_t m, const vec3_t in, vec3_t out)
{
	out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2];
	out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2];
	out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2];
}

void MatrixTransformNormal2(const matrix_t m, vec3_t inout)
{
	vec3_t tmp;

	tmp[0] = m[0] * inout[0] + m[4] * inout[1] + m[8] * inout[2];
	tmp[1] = m[1] * inout[0] + m[5] * inout[1] + m[9] * inout[2];
	tmp[2] = m[2] * inout[0] + m[6] * inout[1] + m[10] * inout[2];

	VectorCopy(tmp, inout);
}

void MatrixTransformPlane(const matrix_t m, const vec4_t in, vec4_t out)
{
	vec3_t translation;
	vec3_t planePos;

	// rotate the plane normal
	MatrixTransformNormal(m, in, out);

	// add new position to current plane position
	VectorSet(translation, m[12], m[13], m[14]);
	VectorMA(translation, in[3], out, planePos);

	out[3] = DotProduct(out, planePos);
}

void MatrixTransformPlane2(const matrix_t m, vec4_t inout)
{
	vec4_t tmp;

	MatrixTransformPlane(m, inout, tmp);
	Vector4Copy(tmp, inout);
}

// Tr3B: far plane at infinity, see RobustShadowVolumes.pdf by Nvidia
void MatrixPerspectiveProjectionFovXYInfiniteRH(matrix_t m, vec_t fovX, vec_t fovY, vec_t nearvec)
{
	vec_t width, height;

	width  = tanf(DEG2RAD(fovX * 0.5f));
	height = tanf(DEG2RAD(fovY * 0.5f));

	m[0] = 1 / width;   m[4] = 0;           m[8] = 0;                       m[12] = 0;
	m[1] = 0;           m[5] = 1 / height;  m[9] = 0;                       m[13] = 0;
	m[2] = 0;           m[6] = 0;           m[10] = -1;                     m[14] = -2 * nearvec;
	m[3] = 0;           m[7] = 0;           m[11] = -1;                     m[15] = 0;
}

// helper functions for MatrixInverse from GtkRadiant C mathlib
static float m3_det(matrix3x3_t mat)
{
	float det;

	det = mat[0] * (mat[4] * mat[8] - mat[7] * mat[5])
	      - mat[1] * (mat[3] * mat[8] - mat[6] * mat[5])
	      + mat[2] * (mat[3] * mat[7] - mat[6] * mat[4]);

	return(det);
}

void QuatTransformVector(const quat_t q, const vec3_t in, vec3_t out)
{
	matrix_t m;

	MatrixFromQuat(m, q);
	MatrixTransformNormal(m, in, out);
}

void QuatSlerp(const quat_t from, const quat_t to, float frac, quat_t out)
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
	float cosom, absCosom, sinom, sinSqr, omega, scale0, scale1;

	if (frac <= 0.0f)
	{
		QuatCopy(from, out);
		return;
	}

	if (frac >= 1.0f)
	{
		QuatCopy(to, out);
		return;
	}

	if (QuatCompare(from, to))
	{
		QuatCopy(from, out);
		return;
	}

	cosom    = from[0] * to[0] + from[1] * to[1] + from[2] * to[2] + from[3] * to[3];
	absCosom = fabs(cosom);

	if ((1.0f - absCosom) > 1e-6f)
	{
		sinSqr = 1.0f - absCosom * absCosom;
		sinom  = 1.0f / sqrt(sinSqr);
		omega  = atan2(sinSqr * sinom, absCosom);

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

void QuatMultiply0(quat_t qa, const quat_t qb)
{
	quat_t tmp;

	QuatCopy(qa, tmp);
	QuatMultiply1(tmp, qb, qa);
}

void QuatMultiply1(const quat_t qa, const quat_t qb, quat_t qc)
{
	/*
	   from matrix and quaternion faq
	   x = w1x2 + x1w2 + y1z2 - z1y2
	   y = w1y2 + y1w2 + z1x2 - x1z2
	   z = w1z2 + z1w2 + x1y2 - y1x2

	   w = w1w2 - x1x2 - y1y2 - z1z2
	 */

	qc[0] = qa[3] * qb[0] + qa[0] * qb[3] + qa[1] * qb[2] - qa[2] * qb[1];
	qc[1] = qa[3] * qb[1] + qa[1] * qb[3] + qa[2] * qb[0] - qa[0] * qb[2];
	qc[2] = qa[3] * qb[2] + qa[2] * qb[3] + qa[0] * qb[1] - qa[1] * qb[0];
	qc[3] = qa[3] * qb[3] - qa[0] * qb[0] - qa[1] * qb[1] - qa[2] * qb[2];
}

void QuatMultiply2(const quat_t qa, const quat_t qb, quat_t qc)
{
	qc[0] = qa[3] * qb[0] + qa[0] * qb[3] + qa[1] * qb[2] + qa[2] * qb[1];
	qc[1] = qa[3] * qb[1] - qa[1] * qb[3] - qa[2] * qb[0] + qa[0] * qb[2];
	qc[2] = qa[3] * qb[2] - qa[2] * qb[3] - qa[0] * qb[1] + qa[1] * qb[0];
	qc[3] = qa[3] * qb[3] - qa[0] * qb[0] - qa[1] * qb[1] + qa[2] * qb[2];
}

void QuatMultiply3(const quat_t qa, const quat_t qb, quat_t qc)
{
	qc[0] = qa[3] * qb[0] + qa[0] * qb[3] + qa[1] * qb[2] + qa[2] * qb[1];
	qc[1] = -qa[3] * qb[1] + qa[1] * qb[3] - qa[2] * qb[0] + qa[0] * qb[2];
	qc[2] = -qa[3] * qb[2] + qa[2] * qb[3] - qa[0] * qb[1] + qa[1] * qb[0];
	qc[3] = -qa[3] * qb[3] + qa[0] * qb[0] - qa[1] * qb[1] + qa[2] * qb[2];
}

void QuatMultiply4(const quat_t qa, const quat_t qb, quat_t qc)
{
	qc[0] = qa[3] * qb[0] - qa[0] * qb[3] - qa[1] * qb[2] - qa[2] * qb[1];
	qc[1] = -qa[3] * qb[1] - qa[1] * qb[3] + qa[2] * qb[0] - qa[0] * qb[2];
	qc[2] = -qa[3] * qb[2] - qa[2] * qb[3] + qa[0] * qb[1] - qa[1] * qb[0];
	qc[3] = -qa[3] * qb[3] - qa[0] * qb[0] + qa[1] * qb[1] - qa[2] * qb[2];
}

vec_t QuatNormalize(quat_t q)
{
	float length, ilength;

	length = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	length = sqrt(length);

	if (length)
	{
		ilength = 1 / length;
		q[0]   *= ilength;
		q[1]   *= ilength;
		q[2]   *= ilength;
		q[3]   *= ilength;
	}

	return length;
}

void QuatFromAngles(quat_t q, vec_t pitch, vec_t yaw, vec_t roll)
{
#if 1
	matrix_t tmp;

	MatrixFromAngles(tmp, pitch, yaw, roll);
	QuatFromMatrix(q, tmp);
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

static void m4_submat(matrix_t mr, matrix3x3_t mb, int i, int j)
{
	int ti, tj, idst = 0, jdst = 0;

	for (ti = 0; ti < 4; ti++)
	{
		if (ti < i)
		{
			idst = ti;
		}
		else
		if (ti > i)
		{
			idst = ti - 1;
		}

		for (tj = 0; tj < 4; tj++)
		{
			if (tj < j)
			{
				jdst = tj;
			}
			else
			if (tj > j)
			{
				jdst = tj - 1;
			}

			if (ti != i && tj != j)
			{
				mb[idst * 3 + jdst] = mr[ti * 4 + tj];
			}
		}
	}
}

static float m4_det(matrix_t mr)
{
	float       det, result = 0, i = 1;
	matrix3x3_t msub3;
	int         n;

	for (n = 0; n < 4; n++, i *= -1)
	{
		m4_submat(mr, msub3, 0, n);

		det     = m3_det(msub3);
		result += mr[n] * det * i;
	}

	return result;
}

qboolean MatrixInverse(matrix_t matrix)
{
	float       mdet = m4_det(matrix);
	matrix3x3_t mtemp;
	int         i, j, sign;
	matrix_t    m4x4_temp;

#if 0
	if (fabs(mdet) < 0.0000000001)
	{
		return qtrue;
	}
#endif

	MatrixCopy(matrix, m4x4_temp);

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
		{
			sign = 1 - ((i + j) % 2) * 2;

			m4_submat(m4x4_temp, mtemp, i, j);

			// FIXME: try using * inverse det and see if speed/accuracy are good enough
			matrix[i + j * 4] = (m3_det(mtemp) * sign) / mdet;
		}

	return qfalse;
}

void MatrixSetupShear(matrix_t m, vec_t x, vec_t y)
{
	m[0] = 1;      m[4] = x;      m[8] = 0;      m[12] = 0;
	m[1] = y;      m[5] = 1;      m[9] = 0;      m[13] = 0;
	m[2] = 0;      m[6] = 0;      m[10] = 1;      m[14] = 0;
	m[3] = 0;      m[7] = 0;      m[11] = 0;      m[15] = 1;
}

void MatrixMultiplyShear(matrix_t m, vec_t x, vec_t y)
{
	matrix_t tmp, shear;

	MatrixCopy(m, tmp);
	MatrixSetupShear(shear, x, y);
	MatrixMultiplyMOD(tmp, shear, m);
}

void MatrixToVectorsFRU(const matrix_t m, vec3_t forward, vec3_t right, vec3_t up)
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

void MatrixSetupTransformFromVectorsFRU(matrix_t m, const vec3_t forward, const vec3_t right, const vec3_t up, const vec3_t origin)
{
	m[0] = forward[0];     m[4] = -right[0];        m[8] = up[0];  m[12] = origin[0];
	m[1] = forward[1];     m[5] = -right[1];        m[9] = up[1];  m[13] = origin[1];
	m[2] = forward[2];     m[6] = -right[2];        m[10] = up[2];  m[14] = origin[2];
	m[3] = 0;              m[7] = 0;                m[11] = 0;      m[15] = 1;
}

void QuatToVectorsFLU(const quat_t q, vec3_t forward, vec3_t left, vec3_t up)
{
	matrix_t tmp;

	MatrixFromQuat(tmp, q);
	MatrixToVectorsFRU(tmp, forward, left, up);
}

void QuatToVectorsFRU(const quat_t q, vec3_t forward, vec3_t right, vec3_t up)
{
	matrix_t tmp;

	MatrixFromQuat(tmp, q);
	MatrixToVectorsFRU(tmp, forward, right, up);
}

void QuatToAxis(const quat_t q, vec3_t axis[3])
{
	matrix_t tmp;

	MatrixFromQuat(tmp, q);
	MatrixToVectorsFLU(tmp, axis[0], axis[1], axis[2]);
}

void MatrixFromPlanes(matrix_t m, const vec4_t left, const vec4_t right, const vec4_t bottom, const vec4_t top, const vec4_t nearvec, const vec4_t farvec)
{
	m[0] = (right[0] - left[0]) / 2;
	m[1] = (top[0] - bottom[0]) / 2;
	m[2] = (farvec[0] - nearvec[0]) / 2;
	m[3] = right[0] - (right[0] - left[0]) / 2;

	m[4] = (right[1] - left[1]) / 2;
	m[5] = (top[1] - bottom[1]) / 2;
	m[6] = (farvec[1] - nearvec[1]) / 2;
	m[7] = right[1] - (right[1] - left[1]) / 2;

	m[8]  = (right[2] - left[2]) / 2;
	m[9]  = (top[2] - bottom[2]) / 2;
	m[10] = (farvec[2] - nearvec[2]) / 2;
	m[11] = right[2] - (right[2] - left[2]) / 2;

#if 0
	m[12] = (right[3] - left[3]) / 2;
	m[13] = (top[3] - bottom[3]) / 2;
	m[14] = (farvec[3] - nearvec[3]) / 2;
	m[15] = right[3] - (right[3] - left[3]) / 2;
#else
	m[12] = (-right[3] - -left[3]) / 2;
	m[13] = (-top[3] - -bottom[3]) / 2;
	m[14] = (-farvec[3] - -nearvec[3]) / 2;
	m[15] = -right[3] - (-right[3] - -left[3]) / 2;
#endif
}

byte ClampByte(int i)
{
	if (i < 0)
	{
		return 0;
	}
	if (i > 255)
	{
		return 255;
	}
	return i;
}

/*
=============
Q_strreplace

replaces content of find by replace in dest
=============
*/
qboolean Q_strreplace(char *dest, int destsize, const char *find, const char *replace)
{
	int  lstart, lfind, lreplace, lend;
	char *s;
	char backup[32000];             // big, but small enough to fit in PPC stack

	lend = strlen(dest);
	if (lend >= destsize)
	{
		Com_Error(ERR_FATAL, "Q_strreplace: already overflowed");
	}

	s = strstr(dest, find);
	if (!s)
	{
		return qfalse;
	}
	else
	{
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

void FreeMemStream(memStream_t *s)
{
	Com_Dealloc(s);
}

int MemStreamRead(memStream_t *s, void *buffer, int len)
{
	int ret = 1;

	if (s == NULL || buffer == NULL)
	{
		return 0;
	}

	if (s->curPos + len > s->buffer + s->bufSize)
	{
		s->flags |= MEMSTREAM_FLAGS_EOF;
		len       = s->buffer + s->bufSize - s->curPos;
		ret       = 0;

		Com_Error(ERR_FATAL, "MemStreamRead: EOF reached");
	}

	Com_Memcpy(buffer, s->curPos, len);
	s->curPos += len;

	return ret;
}

int MemStreamGetC(memStream_t *s)
{
	int c = 0;

	if (s == NULL)
	{
		return -1;
	}

	if (MemStreamRead(s, &c, 1) == 0)
	{
		return -1;
	}

	return c;
}

int MemStreamGetLong(memStream_t *s)
{
	int c = 0;

	if (s == NULL)
	{
		return -1;
	}

	if (MemStreamRead(s, &c, 4) == 0)
	{
		return -1;
	}

	return LittleLong(c);
}

int MemStreamGetShort(memStream_t *s)
{
	int c = 0;

	if (s == NULL)
	{
		return -1;
	}

	if (MemStreamRead(s, &c, 2) == 0)
	{
		return -1;
	}

	return LittleShort(c);
}

float MemStreamGetFloat(memStream_t *s)
{
	floatint_t c;

	if (s == NULL)
	{
		return -1;
	}

	if (MemStreamRead(s, &c.i, 4) == 0)
	{
		return -1;
	}

	return LittleFloat(c.f);
}

//============================================================================

#ifdef __cplusplus
}
#endif