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

mat4_t matrixIdentity = { 1, 0, 0, 0,
	                      0, 1, 0, 0,
	                      0, 0, 1, 0,
	                      0, 0, 0, 1 };
quat_t quatIdentity = { 0, 0, 0, 1 };

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
	m[0] *= x;     m[4] *= y;        m[8] *= z;
	m[1] *= x;     m[5] *= y;        m[9] *= z;
	m[2] *= x;     m[6] *= y;        m[10] *= z;
	m[3] *= x;     m[7] *= y;        m[11] *= z;
#endif
}

/**
 * @brief MatrixSetupTransformFromRotation
 * @param[out] m
 * @param[in] rot
 * @param[in] origin
 */
void MatrixSetupTransformFromRotation(mat4_t m, const mat4_t rot, const vec3_t origin)
{
	m[0] = rot[0];     m[4] = rot[4];        m[8] = rot[8];  m[12] = origin[0];
	m[1] = rot[1];     m[5] = rot[5];        m[9] = rot[9];  m[13] = origin[1];
	m[2] = rot[2];     m[6] = rot[6];        m[10] = rot[10];  m[14] = origin[2];
	m[3] = 0;           m[7] = 0;              m[11] = 0;        m[15] = 1;
}

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
	// cleaned up
	out[0] = in[0];       out[4] = in[1];       out[8] = in[2];
	out[1] = in[4];       out[5] = in[5];       out[9] = in[6];
	out[2] = in[8];       out[6] = in[9];       out[10] = in[10];
	out[3] = 0;            out[7] = 0;            out[11] = 0;            out[15] = 1;

	out[12] = -(in[12] * out[0] + in[13] * out[4] + in[14] * out[8]);
	out[13] = -(in[12] * out[1] + in[13] * out[5] + in[14] * out[9]);
	out[14] = -(in[12] * out[2] + in[13] * out[6] + in[14] * out[10]);
#endif
}

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
	vec_t width, height;

	width  = tanf(DEG2RAD(fovX * 0.5f));
	height = tanf(DEG2RAD(fovY * 0.5f));

	m[0] = 1 / width;   m[4] = 0;           m[8] = 0;                       m[12] = 0;
	m[1] = 0;           m[5] = 1 / height;  m[9] = 0;                       m[13] = 0;
	m[2] = 0;           m[6] = 0;           m[10] = farvec / (nearvec - farvec);        m[14] = (nearvec * farvec) / (nearvec - farvec);
	m[3] = 0;           m[7] = 0;           m[11] = -1;                     m[15] = 0;
}

/**
 * @brief MatrixLookAtRH
 * @param[out] m
 * @param[in] eye
 * @param[in] dir
 * @param[in] up
 */
void MatrixLookAtRH(mat4_t m, const vec3_t eye, const vec3_t dir, const vec3_t up)
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
	float  sect;

	VectorNormalize2(rayDir, dir);

	sect = -(DotProduct(plane, rayPos) - plane[3]) / DotProduct(plane, rayDir);
	VectorScale(dir, sect, dir);
	VectorAdd(rayPos, dir, res);
}

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
 */
void MatrixOrthogonalProjectionRH(mat4_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec)
{
	m[0] = 2 / (right - left);  m[4] = 0;                   m[8] = 0;                   m[12] = (left + right) / (left - right);
	m[1] = 0;                   m[5] = 2 / (top - bottom);  m[9] = 0;                   m[13] = (top + bottom) / (bottom - top);
	m[2] = 0;                   m[6] = 0;                   m[10] = 1 / (nearvec - farvec); m[14] = nearvec / (nearvec - farvec);
	m[3] = 0;                   m[7] = 0;                   m[11] = 0;                  m[15] = 1;
}

/**
 * @brief MatrixCrop
 * @param[out] m
 * @param[in] mins
 * @param[in] maxs
 */
void MatrixCrop(mat4_t m, const vec3_t mins, const vec3_t maxs)
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

/**
 * @brief MatrixMultiplyTranslation
 * @param[in,out] m
 * @param[in] x
 * @param[in] y
 * @param[in] z
 */
void MatrixMultiplyTranslation(mat4_t m, vec_t x, vec_t y, vec_t z)
{
#if 1
	mat4_t tmp, trans;

	mat4_copy(m, tmp);
	mat4_reset_translate(trans, x, y, z);
	mat4_mult(tmp, trans, m);
#else
	m[12] += m[0] * x + m[4] * y + m[8] * z;
	m[13] += m[1] * x + m[5] * y + m[9] * z;
	m[14] += m[2] * x + m[6] * y + m[10] * z;
	m[15] += m[3] * x + m[7] * y + m[11] * z;
#endif
}

/**
 * @brief MatrixSetupZRotation
 * @param[out] m
 * @param[in] degrees
 */
void MatrixSetupZRotation(mat4_t m, vec_t degrees)
{
	vec_t a = DEG2RAD(degrees);

	m[0] = cos(a);         m[4] = -sin(a);         m[8] = 0;      m[12] = 0;
	m[1] = sin(a);         m[5] = cos(a);         m[9] = 0;      m[13] = 0;
	m[2] = 0;              m[6] = 0;              m[10] = 1;      m[14] = 0;
	m[3] = 0;              m[7] = 0;              m[11] = 0;      m[15] = 1;
}

/**
 * @brief MatrixMultiplyZRotation
 * @param[in,out] m
 * @param[in] degrees
 */
void MatrixMultiplyZRotation(mat4_t m, vec_t degrees)
{
	mat4_t tmp, rot;

	mat4_copy(m, tmp);
	MatrixSetupZRotation(rot, degrees);

	mat4_mult(tmp, rot, m);
}

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
	if (maxs[0] < mins2[0] ||
	    maxs[1] < mins2[1] || maxs[2] < mins2[2] || mins[0] > maxs2[0] || mins[1] > maxs2[1] || mins[2] > maxs2[2])
	{
		return qfalse;
	}

	return qtrue;
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
 */
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

/**
 * @brief BoundsIntersectPoint
 * @param[in] mins
 * @param[in] maxs
 * @param[in] origin
 * @return
 *
 * @note Unused
 */
qboolean BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs, const vec3_t origin)
{
	if (origin[0] > maxs[0] ||
	    origin[0] < mins[0] || origin[1] > maxs[1] || origin[1] < mins[1] || origin[2] > maxs[2] || origin[2] < mins[2])
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief ZeroBounds
 * @param[out] mins
 * @param[out] maxs
 */
void ZeroBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 0;
	maxs[0] = maxs[1] = maxs[2] = 0;
}

/**
 * @brief MatrixSetupTransformFromQuat
 * @param[out] m
 * @param[in] quat
 * @param[in] origin
 */
void MatrixSetupTransformFromQuat(mat4_t m, const quat_t quat, const vec3_t origin)
{
	mat4_t rot;

	mat4_from_quat(rot, quat);

	m[0] = rot[0];     m[4] = rot[4];        m[8] = rot[8];  m[12] = origin[0];
	m[1] = rot[1];     m[5] = rot[5];        m[9] = rot[9];  m[13] = origin[1];
	m[2] = rot[2];     m[6] = rot[6];        m[10] = rot[10];  m[14] = origin[2];
	m[3] = 0;           m[7] = 0;              m[11] = 0;        m[15] = 1;
}

/**
 * @brief PlaneNormalize
 * @param[in,out] plane
 * @return
 */
vec_t PlaneNormalize(vec4_t plane)
{
	vec_t length, ilength;

	length = sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
	if (length == 0.f)
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

/**
 * @brief MatrixTransformNormal
 * @param[in] m
 * @param[in] in
 * @param[out] out
 */
void MatrixTransformNormal(const mat4_t m, const vec3_t in, vec3_t out)
{
	out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2];
	out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2];
	out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2];
}

/**
 * @brief MatrixTransformNormal2
 * @param[in] m
 * @param[in,out] inout
 */
void MatrixTransformNormal2(const mat4_t m, vec3_t inout)
{
	vec3_t tmp;

	tmp[0] = m[0] * inout[0] + m[4] * inout[1] + m[8] * inout[2];
	tmp[1] = m[1] * inout[0] + m[5] * inout[1] + m[9] * inout[2];
	tmp[2] = m[2] * inout[0] + m[6] * inout[1] + m[10] * inout[2];

	VectorCopy(tmp, inout);
}

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

	out[3] = DotProduct(out, planePos);
}

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
	vec3_t minx, maxx;
	vec3_t miny, maxy;
	vec3_t minz, maxz;

	const float* c1 = m;
	const float* c2 = m + 4;
	const float* c3 = m + 8;
	const float* c4 = m + 12;

	VectorScale(c1, mins[0], minx);
	VectorScale(c1, maxs[0], maxx);

	VectorScale(c2, mins[1], miny);
	VectorScale(c2, maxs[1], maxy);

	VectorScale(c3, mins[2], minz);
	VectorScale(c3, maxs[2], maxz);

	vec3_t tmins, tmaxs;
	vec3_t tmp;

	VectorMin(minx, maxx, tmins);
	VectorMax(minx, maxx, tmaxs);
	VectorAdd(tmins, c4, tmins);
	VectorAdd(tmaxs, c4, tmaxs);

	VectorMin(miny, maxy, tmp);
	VectorAdd(tmp, tmins, tmins);

	VectorMax(miny, maxy, tmp);
	VectorAdd(tmp, tmaxs, tmaxs);

	VectorMin(minz, maxz, tmp);
	VectorAdd(tmp, tmins, omins);

	VectorMax(minz, maxz, tmp);
	VectorAdd(tmp, tmaxs, omaxs);
}

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
	vec_t width, height;

	width  = tanf(DEG2RAD(fovX * 0.5f));
	height = tanf(DEG2RAD(fovY * 0.5f));

	m[0] = 1 / width;   m[4] = 0;           m[8] = 0;                       m[12] = 0;
	m[1] = 0;           m[5] = 1 / height;  m[9] = 0;                       m[13] = 0;
	m[2] = 0;           m[6] = 0;           m[10] = -1;                     m[14] = -2 * nearvec;
	m[3] = 0;           m[7] = 0;           m[11] = -1;                     m[15] = 0;
}

/**
 * @brief QuatTransformVector
 * @param[in] q
 * @param[in] in
 * @param[out] out
 */
void QuatTransformVector(const quat_t q, const vec3_t in, vec3_t out)
{
	mat4_t m;

	mat4_from_quat(m, q);
	MatrixTransformNormal(m, in, out);
}

/**
 * @brief QuatMultiply0
 * @param[in,out] qa
 * @param[in] qb
 *
 * @note Unused
 */
void QuatMultiply0(quat_t qa, const quat_t qb)
{
	quat_t tmp;

	quat_copy(qa, tmp);
	QuatMultiply1(tmp, qb, qa);
}

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
 */
void QuatMultiply2(const quat_t qa, const quat_t qb, quat_t qc)
{
	qc[0] = qa[3] * qb[0] + qa[0] * qb[3] + qa[1] * qb[2] + qa[2] * qb[1];
	qc[1] = qa[3] * qb[1] - qa[1] * qb[3] - qa[2] * qb[0] + qa[0] * qb[2];
	qc[2] = qa[3] * qb[2] - qa[2] * qb[3] - qa[0] * qb[1] + qa[1] * qb[0];
	qc[3] = qa[3] * qb[3] - qa[0] * qb[0] - qa[1] * qb[1] + qa[2] * qb[2];
}

/**
 * @brief QuatMultiply3
 * @param[in] qa
 * @param[in] qb
 * @param[out] qc
 *
 * @note Unused
 */
void QuatMultiply3(const quat_t qa, const quat_t qb, quat_t qc)
{
	qc[0] = qa[3] * qb[0] + qa[0] * qb[3] + qa[1] * qb[2] + qa[2] * qb[1];
	qc[1] = -qa[3] * qb[1] + qa[1] * qb[3] - qa[2] * qb[0] + qa[0] * qb[2];
	qc[2] = -qa[3] * qb[2] + qa[2] * qb[3] - qa[0] * qb[1] + qa[1] * qb[0];
	qc[3] = -qa[3] * qb[3] + qa[0] * qb[0] - qa[1] * qb[1] + qa[2] * qb[2];
}

/**
 * @brief QuatMultiply4
 * @param[in] qa
 * @param[in] qb
 * @param[out] qc
 *
 * @note Unused
 */
void QuatMultiply4(const quat_t qa, const quat_t qb, quat_t qc)
{
	qc[0] = qa[3] * qb[0] - qa[0] * qb[3] - qa[1] * qb[2] - qa[2] * qb[1];
	qc[1] = -qa[3] * qb[1] - qa[1] * qb[3] + qa[2] * qb[0] - qa[0] * qb[2];
	qc[2] = -qa[3] * qb[2] - qa[2] * qb[3] + qa[0] * qb[1] - qa[1] * qb[0];
	qc[3] = -qa[3] * qb[3] - qa[0] * qb[0] + qa[1] * qb[1] - qa[2] * qb[2];
}

/**
 * @brief MatrixSetupShear
 * @param[out] m
 * @param[in] x
 * @param[in] y
 */
void MatrixSetupShear(mat4_t m, vec_t x, vec_t y)
{
	m[0] = 1;      m[4] = x;      m[8] = 0;      m[12] = 0;
	m[1] = y;      m[5] = 1;      m[9] = 0;      m[13] = 0;
	m[2] = 0;      m[6] = 0;      m[10] = 1;      m[14] = 0;
	m[3] = 0;      m[7] = 0;      m[11] = 0;      m[15] = 1;
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

	mat4_copy(m, tmp);
	MatrixSetupShear(shear, x, y);
	mat4_mult(tmp, shear, m);
}

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
 *
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
*/
