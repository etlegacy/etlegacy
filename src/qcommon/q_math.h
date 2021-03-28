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
* @file q_math.h
* @brief Math library
*/

#ifndef INCLUDE_Q_MATH_H
#define INCLUDE_Q_MATH_H

#include "q_shared.h"

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef vec3_t axis_t[3];
typedef vec_t mat3_t[9];
typedef vec_t mat4_t[16];
typedef vec_t quat_t[4];        // | x y z w |

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
#define M_PI        3.14159265358979323846f // matches value in gcc v2 math.h
#endif

// Tau = 2 * pi
// calculated 2*M_PI=6.28318530717958623200
#define M_TAU_F      6.28318530717958647693f // 6. 2831853071 7958647692 5286766559 0057683943

#ifndef M_SQRT2
#define M_SQRT2     1.41421356237309504880f
#endif

#define NUMVERTEXNORMALS    162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorOrange;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;
extern vec4_t colorMdRed;
extern vec4_t colorMdGreen;
extern vec4_t colorDkGreen;
extern vec4_t colorMdCyan;
extern vec4_t colorMdYellow;
extern vec4_t colorMdOrange;
extern vec4_t colorMdBlue;

extern vec4_t clrBrown;
extern vec4_t clrBrownDk;
extern vec4_t clrBrownLine;
extern vec4_t clrBrownText;
extern vec4_t clrBrownTextDk;
extern vec4_t clrBrownTextDk2;
extern vec4_t clrBrownTextLt;
extern vec4_t clrBrownTextLt2;
extern vec4_t clrBrownLineFull;

#define GAME_INIT_FRAMES    6
#define FRAMETIME           100                 // msec

extern vec4_t g_color_table[32];

#define MAKERGB(v, r, g, b) v[0]     = r; v[1] = g; v[2] = b
#define MAKERGBA(v, r, g, b, a) v[0] = r; v[1] = g; v[2] = b; v[3] = a // Unused.

// Hex Color string support
#define gethex(ch) ((ch) > '9' ? ((ch) >= 'a' ? ((ch) - 'a' + 10) : ((ch) - '7')) : ((ch) - '0'))
#define ishex(ch)  ((ch) && (((ch) >= '0' && (ch) <= '9') || ((ch) >= 'A' && (ch) <= 'F') || ((ch) >= 'a' && (ch) <= 'f')))
// check if it's format rrggbb r,g,b e {0..9} U {A...F}
#define Q_IsHexColorString(p) (ishex(*(p)) && ishex(*((p) + 1)) && ishex(*((p) + 2)) && ishex(*((p) + 3)) && ishex(*((p) + 4)) && ishex(*((p) + 5)))
#define Q_HexColorStringHasAlpha(p) (ishex(*((p) + 6)) && ishex(*((p) + 7)))

#define DEG2RAD(a) (((a) * M_PI) / 180.0)
#define RAD2DEG(a) (((a) * 180.0) / M_PI)

struct cplane_s;

extern vec3_t vec3_origin;
extern vec3_t axisDefault[3];

#define nanmask (255 << 23)

#define IS_NAN(x) (((*(int *)&x) & nanmask) == nanmask)

int Q_isnan(float x);

#if idx64
extern long qftolsse(float f);
extern int qvmftolsse(void);
extern void qsnapvectorsse(vec3_t vec);

//#define Q_ftol qftolsse
#define Q_SnapVector qsnapvectorsse

extern int (*Q_VMftol)(void); // Unused.
#elif id386
extern long QDECL qftolx87(float f);
extern long QDECL qftolsse(float f);
extern int QDECL qvmftolx87(void);
extern int QDECL qvmftolsse(void);
extern void QDECL qsnapvectorx87(vec3_t vec);
extern void QDECL qsnapvectorsse(vec3_t vec);

//extern long (QDECL *Q_ftol)(float f);
extern int(QDECL * Q_VMftol)(void);  // Unused.
extern void(QDECL * Q_SnapVector)(vec3_t vec);
#else
// Q_ftol must expand to a function name so the pluggable renderer can take
// its address
//#define Q_ftol lrintf
#define Q_SnapVector(vec) \
	do \
	{ \
		vec3_t *temp = (vec); \
\
		(*temp)[0] = round((*temp)[0]); \
		(*temp)[1] = round((*temp)[1]); \
		(*temp)[2] = round((*temp)[2]); \
	} while (0)
#endif

static ID_INLINE long Q_ftol(float f)
{
#if defined(id386_sse) && defined(_MSC_VER)
	static int tmp;
	__asm fld f
	__asm fistp tmp
	__asm mov eax, tmp
#else
	return (long)f;
#endif
}
/*
// if *your system does not have lrintf() and round() you can try this block. Please also open a bug report at bugzilla.icculus.org
// or write a mail to the ioq3 mailing list.
#else
#define Q_ftol(v) ((long) (v))
#define Q_round(v) do { if((v) < 0) (v) -= 0.5f; else (v) += 0.5f; (v) = Q_ftol((v)); } while(0)
#define Q_SnapVector(vec) \
do\
{\
vec3_t *temp = (vec);\
\
Q_round((*temp)[0]);\
Q_round((*temp)[1]);\
Q_round((*temp)[2]);\
} while(0)
#endif
*/

#if idppc

static ID_INLINE float Q_rsqrt(float number)
{
	float x = 0.5f * number;
	float y;
#ifdef __GNUC__
	asm ("frsqrte %0,%1" : "=f" (y) : "f" (number));
#else
	y = __frsqrte(number);
#endif
	return y * (1.5f - (x * y * y));
}

#ifdef __GNUC__
static ID_INLINE float Q_fabs(float x)
{
	float abs_x;

	asm ("fabs %0,%1" : "=f" (abs_x) : "f" (x));
	return abs_x;
}
#else
#define Q_fabs __fabsf
#endif

#else
float Q_fabs(float f);
float Q_rsqrt(float f);         // reciprocal square root
#endif

#define SQRTFAST(x) (1.0f / Q_rsqrt(x))

signed char ClampChar(int i);
//signed short ClampShort(int i); // Unused.
byte ClampByte(int i);
void ClampColor(vec4_t color);

// this isn't a real cheap function to call!
int DirToByte(vec3_t dir);
void ByteToDir(int b, vec3_t dir);

/************************************************************************/
/* Vector 2                                                             */
/************************************************************************/
#define vec2_set(v, x, y)         ((v)[0] = (x), (v)[1] = (y))
#define vec2_copy(a, b)            ((b)[0] = (a)[0], (b)[1] = (a)[1])
// Subtract
#define vec2_sub(a, b, c)      ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1])
#define vec2_snap(v) { v[0] = ((int)(v[0])); v[1] = ((int)(v[1])); }

/************************************************************************/
/* Vector 3                                                             */
/************************************************************************/
#define vec3_clear(a)              ((a)[0] = (a)[1] = (a)[2] = 0)
#define vec3_negate(a, b)           ((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])
#define vec3_set(v, x, y, z)       ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
//dot product
#define vec3_dot(x, y)         ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2])
#define vec3_sub(a, b, c)   ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1], (c)[2] = (a)[2] - (b)[2])
#define vec3_add(a, b, c)        ((c)[0] = (a)[0] + (b)[0], (c)[1] = (a)[1] + (b)[1], (c)[2] = (a)[2] + (b)[2])
#define vec3_copy(a, b)         ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])
#define vec3_scale(v, s, o)    ((o)[0] = (v)[0] * (s), (o)[1] = (v)[1] * (s), (o)[2] = (v)[2] * (s))
// Vector multiply & add
#define vec3_ma(v, s, b, o)    ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), (o)[2] = (v)[2] + (b)[2] * (s))
#define vec3_snap(v) { v[0] = ((int)(v[0])); v[1] = ((int)(v[1])); v[2] = ((int)(v[2])); }
void vec3_to_angles(const vec3_t value1, vec3_t angles);
void vec3_cross(const vec3_t v1, const vec3_t v2, vec3_t cross);
vec_t vec3_length(const vec3_t v);
vec_t vec3_length_squared(const vec3_t v);
vec_t vec3_distance(const vec3_t p1, const vec3_t p2);
vec_t vec3_distance_squared(const vec3_t p1, const vec3_t p2);
// Normalize
vec_t vec3_norm(vec3_t v);         // returns vector length
void vec3_norm_fast(vec3_t v);      // does NOT return vector length, uses rsqrt approximation
vec_t vec3_norm2(const vec3_t v, vec3_t out);
// Inverse
void vec3_inv(vec3_t v);
void vec3_rotate(const vec3_t in, vec3_t matrix[3], vec3_t out);
qboolean vec3_compare(const vec3_t v1, const vec3_t v2);

//FIXME: duplicate functions :D::D:D:D:D:
float vec3_dist(vec3_t v1, vec3_t v2);
float vec3_dist_squared(vec3_t v1, vec3_t v2);

float vec3_to_yawn(const vec3_t vec);
void vec3_lerp(vec3_t start, vec3_t end, float frac, vec3_t out);

// Perpendicular vector of source
void vec3_per(const vec3_t src, vec3_t dst);

static inline void VectorMin(const vec3_t a, const vec3_t b, vec3_t out)
{
	out[0] = a[0] < b[0] ? a[0] : b[0];
	out[1] = a[1] < b[1] ? a[1] : b[1];
	out[2] = a[2] < b[2] ? a[2] : b[2];
}

static inline void VectorMax(const vec3_t a, const vec3_t b, vec3_t out)
{
	out[0] = a[0] > b[0] ? a[0] : b[0];
	out[1] = a[1] > b[1] ? a[1] : b[1];
	out[2] = a[2] > b[2] ? a[2] : b[2];
}

/************************************************************************/
/* Vector 4                                                             */
/************************************************************************/
#define vec4_set(v, x, y, z, n)   ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z), (v)[3] = (n))
#define vec4_copy(a, b)            ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])
#define vec4_scale(v, s, o)    ((o)[0] = (v)[0] * (s), (o)[1] = (v)[1] * (s), (o)[2] = (v)[2] * (s), (o)[3] = (v)[3] * (s))
// Vector multiply & add
#define vec4_ma(v, s, b, o)       ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), (o)[2] = (v)[2] + (b)[2] * (s), (o)[3] = (v)[3] + (b)[3] * (s))
#define vec4_average(v, b, s, o)  ((o)[0] = ((v)[0] * (1 - (s))) + ((b)[0] * (s)), (o)[1] = ((v)[1] * (1 - (s))) + ((b)[1] * (s)), (o)[2] = ((v)[2] * (1 - (s))) + ((b)[2] * (s)), (o)[3] = ((v)[3] * (1 - (s))) + ((b)[3] * (s)))
#define vec4_snap(v) { v[0] = ((int)(v[0])); v[1] = ((int)(v[1])); v[2] = ((int)(v[2])); v[3] = ((int)(v[3])); }

/************************************************************************/
/* Quaternion                                                           */
/************************************************************************/
void quat_from_mat4(quat_t q, const mat4_t m);
void quat_from_axis(const axis_t m, quat_t q);
void quat_from_angles(quat_t q, vec_t pitch, vec_t yaw, vec_t roll);
void quat_to_vec3_FLU(const quat_t q, vec3_t forward, vec3_t left, vec3_t up);
void quat_to_vec3_FRU(const quat_t q, vec3_t forward, vec3_t right, vec3_t up);
void quat_to_axis(const quat_t q, vec3_t axis[3]);
vec_t quat_norm(quat_t q);
void quat_slerp(const quat_t from, const quat_t to, float frac, quat_t out);
#define quat_set(q, x, y, z, w)  ((q)[0] = (x), (q)[1] = (y), (q)[2] = (z), (q)[3] = (w))
#define quat_copy(a, b)       ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])
#define quat_compare(a, b)    ((a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2] && (a)[3] == (b)[3])

/************************************************************************/
/* Axis                                                                 */
/************************************************************************/
void axis_clear(axis_t axis);
void axis_copy(axis_t in, axis_t out);
void axis_to_angles(axis_t axis, vec3_t angles);

/************************************************************************/
/* Angle                                                                */
/************************************************************************/
void angles_to_axis(const vec3_t angles, vec3_t axis[3]);
float angle_mod(float a);
float angle_lerp(float from, float to, float frac);
float angle_sub(float a1, float a2);
void angles_sub(vec3_t v1, vec3_t v2, vec3_t v3);

//float angle_norm_pi(float angle); // Unused.
float angle_norm_360(float angle);
float angle_norm_180(float angle);
float angle_delta(float angle1, float angle2);
void angles_vectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);

/************************************************************************/
/* Matrix3x3                                                            */
/************************************************************************/
#define mat3_mult(in1, in2, o)                                                                          \
	o[0][0] = (in1)[0][0] * (in2)[0][0] + (in1)[0][1] * (in2)[1][0] + (in1)[0][2] * (in2)[2][0],        \
	o[0][1] = (in1)[0][0] * (in2)[0][1] + (in1)[0][1] * (in2)[1][1] + (in1)[0][2] * (in2)[2][1],        \
	o[0][2] = (in1)[0][0] * (in2)[0][2] + (in1)[0][1] * (in2)[1][2] + (in1)[0][2] * (in2)[2][2],        \
	o[1][0] = (in1)[1][0] * (in2)[0][0] + (in1)[1][1] * (in2)[1][0] + (in1)[1][2] * (in2)[2][0],        \
	o[1][1] = (in1)[1][0] * (in2)[0][1] + (in1)[1][1] * (in2)[1][1] + (in1)[1][2] * (in2)[2][1],        \
	o[1][2] = (in1)[1][0] * (in2)[0][2] + (in1)[1][1] * (in2)[1][2] + (in1)[1][2] * (in2)[2][2],        \
	o[2][0] = (in1)[2][0] * (in2)[0][0] + (in1)[2][1] * (in2)[1][0] + (in1)[2][2] * (in2)[2][0],        \
	o[2][1] = (in1)[2][0] * (in2)[0][1] + (in1)[2][1] * (in2)[1][1] + (in1)[2][2] * (in2)[2][1],        \
	o[2][2] = (in1)[2][0] * (in2)[0][2] + (in1)[2][1] * (in2)[1][2] + (in1)[2][2] * (in2)[2][2]

void mat3_transpose(vec3_t matrix[3], vec3_t transpose[3]);

/************************************************************************/
/* Matrix4x4                                                            */
/************************************************************************/
qboolean mat4_compare(const mat4_t a, const mat4_t b);
void mat4_copy(const mat4_t in, mat4_t out);
void MatrixOrthogonalProjection(mat4_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t nearvec, vec_t farvec);
void mat4_transform_vec4(const mat4_t m, const vec4_t in, vec4_t out);
void mat4_reset_translate(mat4_t m, vec_t x, vec_t y, vec_t z);
void mat4_reset_translate_vec3(mat4_t m, vec3_t position);
void mat4_reset_scale(mat4_t m, vec_t x, vec_t y, vec_t z);
void mat4_mult(const mat4_t a, const mat4_t b, mat4_t out);
void mat4_mult_self(mat4_t m, const mat4_t m2);
void mat4_ident(mat4_t m);
void mat4_transform_vec3(const mat4_t m, const vec3_t in, vec3_t out);
void mat4_transform_vec3_self(const mat4_t m, vec3_t inout);
void mat4_transpose(const mat4_t in, mat4_t out);
void mat4_from_quat(mat4_t m, const quat_t q);
void MatrixFromVectorsFLU(mat4_t m, const vec3_t forward, const vec3_t left, const vec3_t up);
void MatrixSetupTransformFromVectorsFLU(mat4_t m, const vec3_t forward, const vec3_t left, const vec3_t up, const vec3_t origin);
void MatrixToVectorsFLU(const mat4_t m, vec3_t forward, vec3_t left, vec3_t up);
void MatrixSetupTransformFromVectorsFRU(mat4_t m, const vec3_t forward, const vec3_t right, const vec3_t up, const vec3_t origin);
void MatrixToVectorsFRU(const mat4_t m, vec3_t forward, vec3_t right, vec3_t up);
qboolean mat4_inverse(const mat4_t in, mat4_t out);
qboolean mat4_inverse_self(mat4_t matrix);
void mat4_from_angles(mat4_t m, vec_t pitch, vec_t yaw, vec_t roll);

#define SinCos(rad, s, c)     \
	(s) = sin((rad));     \
	(c) = cos((rad));

#ifdef __LCC__
#ifdef VectorCopy
#undef VectorCopy
// this is a little hack to get more efficient copies in our interpreter
typedef struct
{
	float v[3];
} vec3struct_t;
#define VectorCopy(a, b) *(vec3struct_t *)b = *(vec3struct_t *)a;
#endif
#endif

//unsigned ColorBytes3(float r, float g, float b); // Unused.
unsigned ColorBytes4(float r, float g, float b, float a);

//float NormalizeColor(const vec3_t in, vec3_t out); // Unused.

float RadiusFromBounds(const vec3_t mins, const vec3_t maxs);
void ClearBounds(vec3_t mins, vec3_t maxs);
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);
//qboolean PointInBounds(const vec3_t v, const vec3_t mins, const vec3_t maxs); // Unused.
void BoundsAdd(vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2);

//int Q_log2(int val); // Unused.

float Q_acos(float c);

int Q_rand(int *seed);
float Q_random(int *seed);
float Q_crandom(int *seed);

#define random()    ((rand() & 0x7fff) / ((float)0x7fff))
#define crandom()   (2.0f * (random() - 0.5f))

void SetPlaneSignbits(struct cplane_s *out);
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p);

qboolean PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c);
void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void RotatePoint(vec3_t point, vec3_t matrix[3]);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);
//void RotatePointAroundVertex(vec3_t pnt, float rot_x, float rot_y, float rot_z, const vec3_t origin); // Unused.
void RotateAroundDirection(vec3_t axis[3], float yaw);
void CreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]);
void MakeNormalVectors(const vec3_t forward, vec3_t right, vec3_t up);
// perpendicular vector could be replaced by this

//int PlaneTypeForNormal(vec3_t normal);

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

void GetPerpendicularViewVector(const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up);
void ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj);
void ProjectPointOntoVectorBounded(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj);
float DistanceFromLineSquared(vec3_t p, vec3_t lp1, vec3_t lp2);
float DistanceFromVectorSquared(vec3_t p, vec3_t lp1, vec3_t lp2);

/************************************************************************/
/* Cleaned up functions (for merging code from other mods/ioq)          */
/************************************************************************/

#if 1

#define DotProduct(x, y) vec3_dot(x, y)
#define VectorSubtract(a, b, c) vec3_sub(a, b, c)
#define VectorAdd(a, b, c) vec3_add(a, b, c)
#define VectorCopy(a, b) vec3_copy(a, b)
#define VectorScale(v, s, o) vec3_scale(v, s, o)
// Vector multiply & add
#define VectorMA(v, s, b, o) vec3_ma(v, s, b, o)

#define MatrixMultiply(in1, in2, o) mat3_mult(in1, in2, o)

#else

#define DotProduct(x, y)         _DotProduct(x, y)
#define VectorSubtract(a, b, c)   _VectorSubtract(a, b, c)
#define VectorAdd(a, b, c)        _VectorAdd(a, b, c)
#define VectorCopy(a, b)         _VectorCopy(a, b)
#define VectorScale(v, s, o)    _VectorScale(v, s, o)
#define VectorMA(v, s, b, o)    _VectorMA(v, s, b, o)
#define MatrixMultiply(in1, in2, o) _MatrixMultiply(in1, in2, o)

#endif // 1

// just in case you don't want to use the macros
vec_t _DotProduct(const vec3_t v1, const vec3_t v2);
void _VectorSubtract(const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorAdd(const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorCopy(const vec3_t in, vec3_t out);
void _VectorScale(const vec3_t in, vec_t scale, vec3_t out);
void _VectorMA(const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);
void _MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]);

#define VectorClear(a) vec3_clear(a)
#define VectorNegate(a, b) vec3_negate(a, b)
#define VectorSet(v, x, y, z) vec3_set(v, x, y, z)

#define Vector2Set(v, x, y) vec2_set(v, x, y)
#define Vector2Copy(a, b) vec2_copy(a, b)
#define Vector2Subtract(a, b, c) vec2_sub(a, b, c)

#define Vector4Set(v, x, y, z, n) vec4_set(v, x, y, z, n)
#define Vector4Copy(a, b) vec4_copy(a, b)
#define Vector4MA(v, s, b, o) vec4_ma(v, s, b, o)
#define Vector4Average(v, b, s, o) vec4_average(v, b, s, o)

#define SnapVector(v) vec3_snap(v)

#define VectorLength vec3_length
#define VectorLengthSquared vec3_length_squared
#define Distance vec3_distance
#define DistanceSquared vec3_distance_squared
#define CrossProduct vec3_cross
#define VectorNormalize vec3_norm
#define VectorNormalizeFast vec3_norm_fast
#define VectorNormalize2 vec3_norm2
#define VectorInverse vec3_inv
#define Vector4Scale vec4_scale
#define VectorRotate vec3_rotate
#define VectorCompare vec3_compare

static ID_INLINE int VectorCompareEpsilon(const vec3_t v1, const vec3_t v2, float epsilon)
{
	vec3_t d;

	VectorSubtract(v1, v2, d);
	d[0] = fabs(d[0]);
	d[1] = fabs(d[1]);
	d[2] = fabs(d[2]);

	if (d[0] > epsilon || d[1] > epsilon || d[2] > epsilon)
	{
		return 0;
	}

	return 1;
}

#define AngleMod angle_mod
#define LerpAngle angle_lerp
#define LerpPosition vec3_lerp
#define AngleSubtract angle_sub
#define AnglesSubtract angles_sub

#define vectoangles vec3_to_angles
#define vectoyaw vec3_to_yawn
#define VectorDistance vec3_dist
#define VectorDistanceSquared vec3_dist_squared

#define AxisClear axis_clear
#define AxisCopy axis_copy
#define AxisToAngles axis_to_angles
#define AnglesToAxis angles_to_axis

#define AngleNormalize2Pi angle_norm_pi
#define AngleNormalize360 angle_norm_360
#define AngleNormalize180 angle_norm_180
#define AngleDelta angle_delta

#define TransposeMatrix mat3_transpose
#define AngleVectors angles_vectors
#define PerpendicularVector(out, src) vec3_per(src, out) // rotated the params to match the way other functions are written

#endif
