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


// SSE3  (Supported from Pentium 4 Prescott, Athlon 64 San Diego, and up.  That's not even a dual core CPU).
// The SSE code added to ETLegacy is not (yet) optimized at all.
// At the moment, there will most likely be a lot of penalties for uOps (micro-ops).
// The provided code is a first attempt to convert some old code into a bit faster code.
// By commenting out the ETL_SSE define below, you have the old code back..
// update: I still need to test usage of ETL_SSE so it compiles on all systems (atm. Windows compile only).
// note: If ETL_SSE is disabled, compiling this branch has no meaning. This version is all about the inlined macro stuff..
//
// The intrinsics header files are available since Visual Studio 2008 (9.0)   That's _MSC_VER 1500
#if defined(_WIN32) && !defined(_WIN64) && defined(_MSC_VER) && (_MSC_VER >= 1500) && !defined(_WIN32_WCE) && !defined(_M_ARM)
// this is the compiler directive that will use the SSE2/SSE3 code
#ifndef ETL_SSE
#define ETL_SSE
// include the SSE intrinsics once
#include "pmmintrin.h"
#endif
#endif


// i don't know how to make pragma 4700 warning suppression work for inlined macros.
// It works with normal functions (see _Vector2AM() on how to successfully disable warnings 4700)
#pragma warning(disable:4700)
#pragma warning(disable:4010)

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
// matches value in gcc v2 math.h
#define M_PI 3.14159265358979323846f
#endif

// Tau = 2 * pi
// calculated 2*M_PI=6.28318530717958623200
#ifndef M_TAU_F
#define M_TAU_F 6.28318530717958647693f
#endif

#ifndef M_2PI
#define M_2PI M_TAU_F
#endif

#ifndef M_halfPI
#define M_halfPI 1.5707963267948966192313216916398f
#endif


#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880f
#endif

#define NUMVERTEXNORMALS    162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define MINICHAR_WIDTH      8
#define MINICHAR_HEIGHT     12

#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16

#define BIGCHAR_WIDTH       16
#define BIGCHAR_HEIGHT      16

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
// msec
#define FRAMETIME           100

#define Q_COLOR_ESCAPE  '^'
#define Q_IsColorString(p) (*p == Q_COLOR_ESCAPE && *(p + 1) && isgraph(*(p + 1)) && *(p + 1) != Q_COLOR_ESCAPE)

#define COLOR_BLACK     '0'
#define COLOR_RED       '1'
#define COLOR_GREEN     '2'
#define COLOR_YELLOW    '3'
#define COLOR_BLUE      '4'
#define COLOR_CYAN      '5'
#define COLOR_MAGENTA   '6'
#define COLOR_WHITE     '7'
#define COLOR_ORANGE    '8'
#define COLOR_MDGREY    '9'
#define COLOR_LTGREY    ':'
//#define COLOR_LTGREY  ';'
#define COLOR_MDGREEN   '<'
#define COLOR_MDYELLOW  '='
#define COLOR_MDBLUE    '>'
#define COLOR_MDRED     '?'
#define COLOR_LTORANGE  'A'
#define COLOR_MDCYAN    'B'
#define COLOR_MDPURPLE  'C'
#define COLOR_NULL      '*'

#define COLOR_BITS  31
#define ColorIndex(c)   (((c) - '0') & COLOR_BITS)

#define S_COLOR_BLACK       "^0"
#define S_COLOR_RED         "^1"
#define S_COLOR_GREEN       "^2"
#define S_COLOR_YELLOW      "^3"
#define S_COLOR_BLUE        "^$" // 4 is unreadable on dark background
#define S_COLOR_CYAN        "^5"
#define S_COLOR_MAGENTA     "^6"
#define S_COLOR_WHITE       "^7"
#define S_COLOR_ORANGE      "^8"
#define S_COLOR_MDGREY      "^9"
#define S_COLOR_LTGREY      "^:"
//#define S_COLOR_LTGREY        "^;"
#define S_COLOR_MDGREEN     "^<"
#define S_COLOR_MDYELLOW    "^="
#define S_COLOR_MDBLUE      "^>"
#define S_COLOR_MDRED       "^?"
#define S_COLOR_LTORANGE    "^A"
#define S_COLOR_MDCYAN      "^B"
#define S_COLOR_MDPURPLE    "^C"
#define S_COLOR_NULL        "^*"

extern vec4_t g_color_table[32];

// unused
//#define MAKERGB(v, r, g, b) v[0] = r; v[1] = g; v[2] = b
//#define MAKERGBA(v, r, g, b, a) v[0] = r; v[1] = g; v[2] = b; v[3] = a

// Hex Color string support
#define gethex(ch) ((ch) > '9' ? ((ch) >= 'a' ? ((ch) - 'a' + 10) : ((ch) - '7')) : ((ch) - '0'))
#define ishex(ch)  ((ch) && (((ch) >= '0' && (ch) <= '9') || ((ch) >= 'A' && (ch) <= 'F') || ((ch) >= 'a' && (ch) <= 'f')))
// check if it's format rrggbb r,g,b e {0..9} U {A...F}
#define Q_IsHexColorString(p) (ishex(*(p)) && ishex(*((p) + 1)) && ishex(*((p) + 2)) && ishex(*((p) + 3)) && ishex(*((p) + 4)) && ishex(*((p) + 5)))
#define Q_HexColorStringHasAlpha(p) (ishex(*((p) + 6)) && ishex(*((p) + 7)))

#define _PI_DIV_180 0.01745329251994329576923690768489f
#define _180_DIV_PI 57.295779513082320876798154814105f
#define DEG2RAD(a) ((a) * _PI_DIV_180)
#define RAD2DEG(a) ((a) * _180_DIV_PI)
//#define DEG2RAD(a) (((a) * M_PI) / 180.0)
//#define RAD2DEG(a) (((a) * 180.0) / M_PI)

// 360.0f / 65536
#define _360_DIV_65536 0.0054931640625f
// 65536 / 360.0f
#define _65536_DIV_360 182.04444444444444444444444444444f

struct cplane_s;

extern vec3_t vec3_origin;		// 0,0,0
extern vec3_t vec3_111;			// 1,1,1
extern vec3_t axisDefault[3];

// unused
//#define nanmask (255 << 23)
//#define IS_NAN(x) (((*(int *)&x) & nanmask) == nanmask)

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
//void ClampColor(vec4_t color);

// this isn't a real cheap function to call!
// update: there are new versions, which are faster..
int DirToByte(vec3_t dir);
void ByteToDir(int b, vec3_t dir);

/************************************************************************/
/* Vector 2                                                             */
/************************************************************************/
#define vec2_set(v, x, y) ((v)[0] = (x), (v)[1] = (y))
#define vec2_copy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1])
// Subtract
#define vec2_sub(a, b, c) ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1])
#define vec2_snap(v) { v[0] = ((int)(v[0])); v[1] = ((int)(v[1])); }

/************************************************************************/
/* Vector 3                                                             */
/************************************************************************/
#define vec3_clear(a) ((a)[0] = (a)[1] = (a)[2] = 0)
#define vec3_negate(a, b) ((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])
#define vec3_set(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
//dot product
#define vec3_dot(x, y) ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2])
#define vec3_sub(a, b, c) ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1], (c)[2] = (a)[2] - (b)[2])
#define vec3_add(a, b, c) ((c)[0] = (a)[0] + (b)[0], (c)[1] = (a)[1] + (b)[1], (c)[2] = (a)[2] + (b)[2])
#define vec3_copy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])
#define vec3_scale(v, s, o) ((o)[0] = (v)[0] * (s), (o)[1] = (v)[1] * (s), (o)[2] = (v)[2] * (s))
// Vector multiply & add
#define vec3_ma(v, s, b, o) ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), (o)[2] = (v)[2] + (b)[2] * (s))
#define vec3_snap(v) { v[0] = ((int)(v[0])); v[1] = ((int)(v[1])); v[2] = ((int)(v[2])); }
void vec3_to_angles(const vec3_t value1, vec3_t angles);
void vec3_cross(const vec3_t v1, const vec3_t v2, vec3_t cross);
vec_t vec3_length(const vec3_t v);
vec_t vec3_length_squared(const vec3_t v);
vec_t vec3_distance(const vec3_t p1, const vec3_t p2);
vec_t vec3_distance_squared(const vec3_t p1, const vec3_t p2);
// Normalize
vec_t vec3_norm(vec3_t v);       // returns vector length
void vec3_norm_void(vec3_t v);   // returns nothing
void vec4_norm_void(vec4_t v);   // returns nothing
void vec3_norm_fast(vec3_t v);   // does NOT return vector length, uses rsqrt approximation
vec_t vec3_norm2(const vec3_t v, vec3_t out);
void vec3_norm2_void(const vec3_t v, vec3_t out);
// Inverse
void vec3_inv(vec3_t v);
void vec3_rotate(const vec3_t in, vec3_t matrix[3], vec3_t out);
qboolean vec3_compare(const vec3_t v1, const vec3_t v2);
int vec3_compare_lt(const vec3_t v1, const vec3_t v2);
int vec3_compare_gt(const vec3_t v1, const vec3_t v2);
void vec3_abs(const vec3_t v, vec3_t o);

//FIXME: duplicate functions :D::D:D:D:D:
float vec3_dist(vec3_t v1, vec3_t v2);
float vec3_dist_squared(vec3_t v1, vec3_t v2);

float vec3_to_yawn(const vec3_t vec);
//void vec3_lerp(vec3_t start, vec3_t end, float frac, vec3_t out);

// Perpendicular vector of source
void vec3_per(const vec3_t src, vec3_t dst);

/************************************************************************/
/* Vector 4                                                             */
/************************************************************************/
#define vec4_set(x, y, z, w, o)   ((o)[0] = (x), (o)[1] = (y), (o)[2] = (z), (o)[3] = (w))
#define vec4_copy(a, b)           ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])
#define vec4_scale(v, s, o)       ((o)[0] = (v)[0] * (s), (o)[1] = (v)[1] * (s), (o)[2] = (v)[2] * (s), (o)[3] = (v)[3] * (s))
#define vec4_average(v, b, s, o)  ((o)[0] = ((v)[0] * (1 - (s))) + ((b)[0] * (s)), (o)[1] = ((v)[1] * (1 - (s))) + ((b)[1] * (s)), (o)[2] = ((v)[2] * (1 - (s))) + ((b)[2] * (s)), (o)[3] = ((v)[3] * (1 - (s))) + ((b)[3] * (s)))
//#define vec4_snap(v) { v[0] = ((int)(v[0])); v[1] = ((int)(v[1])); v[2] = ((int)(v[2])); v[3] = ((int)(v[3])); }
// Vector multiply & add
#define vec4_ma(v, s, b, o)       ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), (o)[2] = (v)[2] + (b)[2] * (s), (o)[3] = (v)[3] + (b)[3] * (s))
#define vec4_sub(a, b, o)         ((o)[0] = (a)[0] - (b)[0], (o)[1] = (a)[1] - (b)[1], (o)[2] = (a)[2] - (b)[2], (o)[3] = (a)[3] - (b)[3])

/************************************************************************/
/* Quaternion                                                           */
/************************************************************************/
void quat_from_mat4(quat_t q, const mat4_t m);
void quat_from_axis(const axis_t m, quat_t q);
//void quat_from_angles(quat_t q, vec_t pitch, vec_t yaw, vec_t roll);
//void quat_to_vec3_FLU(const quat_t q, vec3_t forward, vec3_t left, vec3_t up);
void quat_to_vec3_FRU(const quat_t q, vec3_t forward, vec3_t right, vec3_t up);
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
void mat4_reset_translate(mat4_t m, vec_t x, vec_t y, vec_t z);
void mat4_reset_translate_vec3(mat4_t m, vec3_t position);
void mat4_reset_scale(mat4_t m, vec_t x, vec_t y, vec_t z);
void mat4_mult(const mat4_t a, const mat4_t b, mat4_t out);
void mat4_mult_self(mat4_t m, const mat4_t m2);
void mat4_ident(mat4_t m);
void mat4_transform_vec4(const mat4_t m, const vec4_t in, vec4_t out);
void mat4_transform_vec3(const mat4_t m, const vec3_t in, vec3_t out);
//void mat4_transform_vec3_self(const mat4_t m, vec3_t inout);
void mat4_transpose(const mat4_t in, mat4_t out);
void mat4_from_quat(mat4_t m, const quat_t q);
//void MatrixSetupTransformFromVectorsFRU(mat4_t m, const vec3_t forward, const vec3_t right, const vec3_t up, const vec3_t origin); // unused
qboolean mat4_inverse(const mat4_t in, mat4_t out);
qboolean mat4_inverse_self(mat4_t matrix);
void mat4_from_angles(mat4_t m, vec_t pitch, vec_t yaw, vec_t roll);


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

//qboolean PointInBounds(const vec3_t v, const vec3_t mins, const vec3_t maxs); // Unused.

//int Q_log2(int val); // Unused.

float Q_acos(float c);

int Q_rand(int *seed);
float Q_random(int *seed);
float Q_crandom(int *seed);

#define random()    ((rand() & 0x7fff) / ((float)0x7fff))
#define crandom()   ((random() - 0.5f) * 2.0f)

int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p);

qboolean PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);
//void RotatePointAroundVertex(vec3_t pnt, float rot_x, float rot_y, float rot_z, const vec3_t origin); // Unused.
void RotateAroundDirection(vec3_t axis[3], float yaw);
void CreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]);

// this function is the very same as: VectorRotate(point, matrix, point)
// The (possibly) inlined macro is faster. Use VectorRotate() instead..
//void RotatePoint(vec3_t point, vec3_t matrix[3]);

//int PlaneTypeForNormal(vec3_t normal);

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

//void ProjectPointOntoVectorBounded(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj); // unused
float DistanceFromLineSquared(vec3_t p, vec3_t lp1, vec3_t lp2);
//float DistanceFromVectorSquared(vec3_t p, vec3_t lp1, vec3_t lp2); unused



/************************************************************************/
/*                                                                      */
/************************************************************************/

void _Vector4Set4(const float value, vec4_t out); // set x,y,z & w components to the same given value




#ifndef ETL_SSE

// The next 2 are identical, but the SSE versions are different (but we need to define them both here anyway)
#define RECIPROCAL(x) (1.0f / (x))
#define rcp(x) (1.0f / (x))

#define SinCos(rad, s, c) \
	(s) = sin((rad)); \
	(c) = cos((rad));

#define VectorBound(v, mins, maxs, out) {v[0]=Q_bound(mins[0], v[0], maxs[0]); v[1]=Q_bound(mins[1], v[1], maxs[1]); v[2]=Q_bound(mins[2], v[2], maxs[2]);}

void SetPlaneSignbits(struct cplane_s *out);
void vec3_norm_inlined(vec3_t v, float *l);

void ClearBounds(vec3_t mins, vec3_t maxs);
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);
void BoundsAdd(vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2);

void PlaneFromPoints_void(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c);

void MatrixSetupTransformFromVectorsFLU(mat4_t m, const vec3_t forward, const vec3_t left, const vec3_t up, const vec3_t origin);
void MatrixFromVectorsFLU(mat4_t m, const vec3_t forward, const vec3_t left, const vec3_t up);
void MatrixToVectorsFLU(const mat4_t m, vec3_t forward, vec3_t left, vec3_t up);
void MatrixToVectorsFRU(const mat4_t m, vec3_t forward, vec3_t right, vec3_t up);

void ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj);
void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void GetPerpendicularViewVector(const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up);
void MakeNormalVectors(const vec3_t forward, vec3_t right, vec3_t up);

void quat_to_axis(const quat_t q, vec3_t axis[3]);

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


// use the macro's and functions (no SSE)
///#define DotProduct(x, y) vec3_dot(x, y)
#define DotProduct(x, y) _DotProduct(x,y)
#define Dot(v1, v2, out) { (out) = _DotProduct(v1, v2); }
#define CrossProduct(v1, v2, cross) vec3_cross(v1, v2, cross)

#define VectorSubtract(a, b, c) vec3_sub(a, b, c)
#define Vector2Subtract(a, b, c) vec2_sub(a, b, c)
#define Vector4Subtract(a, b, c) vec4_sub(a, b, c)

#define VectorAdd(a, b, c) vec3_add(a, b, c)
#define VectorAddConst(v, value, o) _VectorAddConst(v, value, o)
#define Vector2AddConst(v, value, o) _Vector2AddConst(v, value, o)

#define VectorCopy(a, b) vec3_copy(a, b)
#define Vector2Copy(a,b) vec2_copy(a, b)
#define Vector4Copy(a, b) vec4_copy(a, b)

#define VectorScale(v, s, o) vec3_scale(v, s, o)
#define Vector2Scale(v, s, o) _Vector2Scale(v, s, o)
#define Vector4Scale(v, s, o) vec4_scale(v, s, o)

#define VectorClear(a) vec3_clear(a)
#define VectorInverse(v) vec3_inv(v)
#define VectorNegate(a, b) vec3_negate(a, b)
#define VectorAbs(v, o) vec3_abs(v, o)
#define VectorMultiply(v1, v2, o) _VectorMultiply(v1, v2, o)

#define VectorSet(v, x, y, z) vec3_set(v, x, y, z)
// !! we swap the output argument in the final vec4_set & _Vector4Set4 call
#define Vector4Set(o, x, y, z, w) vec4_set(x, y, z, w, o)
#define Vector4Set4(o, value) _Vector4Set4(value, o)

// vector Multiply & Add
#define VectorMA(v, s, b, o) vec3_ma(v, s, b, o)
#define Vector4MA(v, s, b, o) vec4_ma(v, s, b, o)

// vector Add & Multiply
#define VectorAM(v, b, s, o) _VectorAM(v, b, s, o)
#define Vector2AM(v, b, s, o) _Vector2AM(v, b, s, o)
#define Vector4AM(v, b, s, o) _Vector4AM(v, b, s, o)

#define VectorNormalizeFast(v) vec3_norm_fast(v)
#define VectorNorm(v, l) vec3_norm_inlined(v, l)
#define VectorNormalizeOnly(v) vec3_norm_void(v)
#define VectorNormalize2Only(v, out) vec3_norm2_void(v, out)
#define Vector4NormalizeOnly(v) vec4_norm_void(v)

#define VectorRotate(in, matrix, out) vec3_rotate(in, matrix, out)
#define VectorTransformM3(m, in, out) VectorRotate(in, m, out)
#define VectorTransformM4(m, in, out) mat4_transform_vec3(m, in, out)
#define Vector4TransformM4(m, in, out) mat4_transform_vec4(m, in, out)

#define MatrixMultiply(in1, in2, o) mat3_mult(in1, in2, o)
#define Matrix4Multiply(a, b, out) mat4_mult(a, b, out)
#define Matrix4MultiplyWith(m, m2) mat4_mult_self(m, m2)

#define Matrix4Identity(m) mat4_ident(m)
#define Matrix4IdentTranslateV3(m, position) mat4_reset_translate_vec3(m, position)
#define Matrix4IdentTranslate(m, x, y, z) mat4_reset_translate(m, x, y, z)
#define Matrix4IdentScale(m, x, y, z) mat4_reset_scale(m, x, y, z)

#define Matrix4Copy(in, out) mat4_copy(in, out)
#define MatrixTranspose(matrix, transpose) mat3_transpose(matrix, transpose)
#define Matrix4Transpose(in, out) mat4_transpose(in, out)
#define Matrix4FromQuaternion(m, q) mat4_from_quat(m, q)


#else

// reciprocal: argument x is a float variable. The result is returned into the same variable
#define RECIPROCAL(x) \
{ \
	_mm_store_ss(&x, _mm_rcp_ss(_mm_load_ss(&x))); \
}

// reciprocal: argument x is a float constant. The result is a float constant.
#define rcp(x) _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(x)))

///#define VectorBound(v, min, max, out)
#define VectorBound(v, mins, maxs, out) \
{ \
	__m128 xmm0, xmm1, xmm2; \
	xmm0 = _mm_loadh_pi(_mm_load_ss(&v[0]), (const __m64 *)(&v[1])); \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&mins[0]), (const __m64 *)(&mins[1])); \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&maxs[0]), (const __m64 *)(&maxs[1])); \
	xmm0 = _mm_min_ps(xmm0, xmm2); \
	xmm0 = _mm_max_ps(xmm0, xmm1); \
	_mm_store_ss(&out[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&out[1]), xmm0); \
}

///void SetPlaneSignbits(struct cplane_s *out);
#define SetPlaneSignbits(plane) \
{ \
	struct cplane_s *ppp = (struct cplane_s *)plane; \
	__m128 xmm1, xmm2; \
	xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)&ppp->normal[0]), (const __m64 *)&ppp->normal[1]); \
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b01111000); \
	xmm2 = _mm_cmplt_ps(xmm1, _mm_setzero_ps()); \
	ppp->signbits = (byte)(_mm_movemask_ps(xmm2)); \
}

///void ClearBounds(vec3_t mins, vec3_t maxs);
#define ClearBounds(mins, maxs) \
{ \
	__m128 xmm0, xmm1; \
	xmm0 = _mm_set_ps1(99999.0f); \
	xmm1 = _mm_sub_ps(_mm_setzero_ps(), xmm0); \
	_mm_store_ss(&mins[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&mins[1]), xmm0); \
	_mm_store_ss(&maxs[0], xmm1); \
	_mm_storeh_pi((__m64 *)(&maxs[1]), xmm1); \
}

///void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);
#define AddPointToBounds(v, mins, maxs) \
{ \
	__m128 xmm0, xmm1, xmm2; \
	xmm0 = _mm_loadh_pi(_mm_load_ss(&v[0]), (const __m64 *)(&v[1])); \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&mins[0]), (const __m64 *)(&mins[1])); \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&maxs[0]), (const __m64 *)(&maxs[1])); \
	xmm1 = _mm_min_ps(xmm1, xmm0); \
	xmm2 = _mm_max_ps(xmm2, xmm0); \
	_mm_store_ss(&mins[0], xmm1); \
	_mm_storeh_pi((__m64 *)(&mins[1]), xmm1); \
	_mm_store_ss(&maxs[0], xmm2); \
	_mm_storeh_pi((__m64 *)(&maxs[1]), xmm2); \
}

///void BoundsAdd(vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2);
#define BoundsAdd(mins, maxs, mins2, maxs2) \
{ \
	__m128 xmm1, xmm2, xmm3, xmm4; \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&mins[0]), (const __m64 *)(&mins[1])); \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&mins2[0]), (const __m64 *)(&mins2[1])); \
	xmm3 = _mm_loadh_pi(_mm_load_ss(&maxs[0]), (const __m64 *)(&maxs[1])); \
	xmm4 = _mm_loadh_pi(_mm_load_ss(&maxs2[0]), (const __m64 *)(&maxs2[1])); \
	xmm1 = _mm_min_ps(xmm1, xmm2); \
	xmm3 = _mm_max_ps(xmm3, xmm4); \
	_mm_store_ss(&mins[0], xmm1); \
	_mm_storeh_pi((__m64 *)(&mins[1]), xmm1); \
	_mm_store_ss(&maxs[0], xmm3); \
	_mm_storeh_pi((__m64 *)(&maxs[1]), xmm3); \
}

///void PlaneFromPoints_void(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c);
#define PlaneFromPoints_void(plane, a, b, c) \
{ \
	vec3_t d1, d2; \
	float n; \
	VectorSubtract(b, a, d1); \
	VectorSubtract(c, a, d2); \
	CrossProduct(d2, d1, plane); \
	VectorNorm(plane, &n); \
	if (n != 0.f) \
	{ \
		Dot(a, plane, plane[3]); \
	} \
}

// These are the SSE3 functions and (inlined) macro's
// DEPRICATED: USE Dot() INSTEAD..
#define DotProduct(x, y) _DotProduct(x, y)

///#define Dot(v1, v2, out) __DotProduct(v1, v2, out)
/*#define Dot(v1, v2, out) \
{ \
	__m128 xmm0, xmm3; \
	xmm0 = _mm_loadh_pi(_mm_load_ss(&v1[0]), (const __m64 *)(&v1[1])); \
	xmm3 = _mm_loadh_pi(_mm_load_ss(&v2[0]), (const __m64 *)(&v2[1])); \
	xmm0 = _mm_mul_ps(xmm0, xmm3); \
	xmm0 = _mm_hadd_ps(xmm0, xmm0); \
	xmm0 = _mm_hadd_ps(xmm0, xmm0); \
	_mm_store_ss(&out, xmm0); \
}*/
#define Dot(v1, v2, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3; \
	xmm0 = _mm_loadh_pi(_mm_load_ss(&v1[0]), (const __m64 *)(&v1[1])); \
	xmm3 = _mm_loadh_pi(_mm_load_ss(&v2[0]), (const __m64 *)(&v2[1])); \
	xmm0 = _mm_mul_ps(xmm0, xmm3); \
	xmm1 = _mm_movehdup_ps(xmm0); \
	xmm2 = _mm_add_ps(xmm0, xmm1); \
	xmm1 = _mm_movehl_ps(xmm1, xmm2); \
	xmm2 = _mm_add_ss(xmm2, xmm1); \
	_mm_store_ss(&out, xmm2); \
}

///#define CrossProduct(v1, v2, cross) vec3_cross(v1, v2, cross)
#define CrossProduct(v1, v2, cross) \
{ \
	__m128 xmm1, xmm2, xmm4; \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&v1[2]), (const __m64 *)(&v1[0])); \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v2[0]), (const __m64 *)(&v2[1])); \
	xmm4 = xmm2; \
	xmm2 = _mm_mul_ps(xmm2, xmm1); \
	xmm4 = _mm_shuffle_ps(xmm4, xmm4, 0b00110110); \
	xmm4 = _mm_mul_ps(xmm4, xmm1); \
	xmm2 = _mm_shuffle_ps(xmm2, xmm2, 0b10000111); \
	xmm2 = _mm_sub_ps(xmm2, xmm4); \
	_mm_store_ss(&cross[0], xmm2); \
	_mm_storeh_pi((__m64 *)(&cross[1]), xmm2); \
}

///#define VectorClear(a) vec3_clear(a)
#define VectorClear(a) \
{ \
	__m128 xmm0; \
	xmm0 = _mm_setzero_ps(); \
	_mm_store_ss(&a[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&a[1]), xmm0); \
}

///#define VectorSet(v, x, y, z) vec3_set(v, x, y, z)
#define VectorSet(v, x, y, z) \
{ \
	__m128 xmm0; \
	xmm0 = _mm_set_ps(z, y, 0.0f, x); \
	_mm_store_ss(&v[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&v[1]), xmm0); \
}

// !! we swap the output argument in the final _VectorSet call
///#define Vector4Set(o, x, y, z, w) _Vector4Set(x, y, z, w, o)
#define Vector4Set(o, x, y, z, w) \
{ \
	_mm_storeu_ps(o, _mm_set_ps(w, z, y, x)); \
}

///#define Vector4Set4(o, value) _Vector4Set4(value, o)
#define Vector4Set4(o, value) \
{ \
	_mm_storeu_ps(&o[0], _mm_set_ps1(value)); \
}

//#define VectorInverse(v) vec3_inv(v)
#define VectorInverse(v) \
{ \
	__m128 xmm0, xmm1; \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&v[0]), (const __m64 *)(&v[1])); \
	xmm0 = _mm_sub_ps(_mm_setzero_ps(), xmm1); \
	_mm_store_ss(&v[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&v[1]), xmm0); \
}

///#define VectorNegate(a, b) vec3_negate(a, b)
#define VectorNegate(a, b) \
{ \
	__m128 xmm0, xmm1; \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&a[0]), (const __m64 *)(&a[1])); \
	xmm0 = _mm_sub_ps(_mm_setzero_ps(), xmm1); \
	_mm_store_ss(&b[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&b[1]), xmm0); \
}

///VectorMin(const vec3_t a, const vec3_t b, vec3_t out)
#define VectorMin(a, b, out) \
{ \
	__m128 xmm0, xmm1, xmm2; \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&a[0]), (const __m64 *)(&a[1])); \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&b[0]), (const __m64 *)(&b[1])); \
	xmm0 = _mm_min_ps(xmm1, xmm2); \
	_mm_store_ss(&out[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&out[1]), xmm0); \
}

///VectorMax(const vec3_t a, const vec3_t b, vec3_t out)
#define VectorMax(a, b, out) \
{ \
	__m128 xmm0, xmm1, xmm2; \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&a[0]), (const __m64 *)(&a[1])); \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&b[0]), (const __m64 *)(&b[1])); \
	xmm0 = _mm_max_ps(xmm1, xmm2); \
	_mm_store_ss(&out[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&out[1]), xmm0); \
}

///#define VectorAbs(v, o) vec3_abs(v, o)
#define VectorAbs(v, o) \
{ \
	__m128 xmm0, xmm1, mask; \
	__m128i minus1 = _mm_set1_epi32(-1); \
	mask = _mm_castsi128_ps(_mm_srli_epi32(minus1, 1)); \
	xmm1 = _mm_loadh_pi(_mm_load_ss(&v[0]), (const __m64 *)(&v[1])); \
	xmm0 = _mm_and_ps(xmm1, mask); \
	_mm_store_ss(&o[0], xmm0); \
	_mm_storeh_pi((__m64 *)(&o[1]), xmm0); \
}

///#define VectorNorm(v, l) vec3_norm_inlined(v, l)
/*#define VectorNorm(v, l) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3; \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v[2]), (const __m64 *)(&v[0])); \
	xmm3 = xmm2; \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm2 = _mm_hadd_ps(xmm2, xmm2); \
	xmm2 = _mm_hadd_ps(xmm2, xmm2); \
	xmm0 = _mm_sqrt_ss(xmm2); \
	_mm_store_ss(l, xmm0); \
	if (*l != 0.0) { \
		xmm1 = _mm_rcp_ss(xmm0); \
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0); \
		xmm3 = _mm_mul_ps(xmm3, xmm1); \
		_mm_store_ss(&v[2], xmm3); \
		_mm_storeh_pi((__m64 *)(&v[0]), xmm3); \
	} \
}*/
#define VectorNorm(v, l) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6; \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v[2]), (const __m64 *)(&v[0])); \
	xmm3 = xmm2; \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm4 = _mm_movehdup_ps(xmm2); \
	xmm6 = _mm_add_ps(xmm2, xmm4); \
	xmm4 = _mm_movehl_ps(xmm4, xmm6); \
	xmm2 = _mm_add_ss(xmm6, xmm4); \
	xmm0 = _mm_sqrt_ss(xmm2); \
	_mm_store_ss(l, xmm0); \
	if (*l != 0.0) { \
		xmm1 = _mm_rcp_ss(xmm0); \
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0); \
		xmm3 = _mm_mul_ps(xmm3, xmm1); \
		_mm_store_ss(&v[2], xmm3); \
		_mm_storeh_pi((__m64 *)(&v[0]), xmm3); \
	} \
}

///#define VectorNormalizeFast(v) vec3_norm_fast(v)
/*#define VectorNormalizeFast(v) \
{ \
	__m128 xmm0, xmm2, xmm3; \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v[2]), (const __m64 *)(&v[0])); \
	xmm3 = xmm2; \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm2 = _mm_hadd_ps(xmm2, xmm2); \
	xmm2 = _mm_hadd_ps(xmm2, xmm2); \
	xmm0 = _mm_rsqrt_ss(xmm2); \
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0); \
	xmm3 = _mm_mul_ps(xmm3, xmm0); \
	_mm_store_ss(&v[2], xmm3); \
	_mm_storeh_pi((__m64 *)(&v[0]), xmm3); \
}*/
#define VectorNormalizeFast(v) \
{ \
	__m128 xmm0, xmm2, xmm3, xmm4, xmm6; \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v[2]), (const __m64 *)(&v[0])); \
	xmm3 = xmm2; \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm4 = _mm_movehdup_ps(xmm2); \
	xmm6 = _mm_add_ps(xmm2, xmm4); \
	xmm4 = _mm_movehl_ps(xmm4, xmm6); \
	xmm2 = _mm_add_ss(xmm6, xmm4); \
	xmm0 = _mm_rsqrt_ss(xmm2); \
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0); \
	xmm3 = _mm_mul_ps(xmm3, xmm0); \
	_mm_store_ss(&v[2], xmm3); \
	_mm_storeh_pi((__m64 *)(&v[0]), xmm3); \
}

// a version of vec3_norm that does not return a function result.
// This makes it easy to implement it as an inlined macro.
///#define VectorNormalizeOnly(v) vec3_norm_void(v)
/*#define VectorNormalizeOnly(v) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3; \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v[2]), (const __m64 *)(&v[0])); \
	xmm3 = xmm2; \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm2 = _mm_hadd_ps(xmm2, xmm2); \
	xmm2 = _mm_hadd_ps(xmm2, xmm2); \
	xmm0 = _mm_sqrt_ss(xmm2); \
	if (_mm_cvtss_f32(xmm0) != 0.0f) { \
		xmm1 = _mm_rcp_ss(xmm0); \
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0); \
		xmm3 = _mm_mul_ps(xmm3, xmm1); \
		_mm_store_ss(&v[2], xmm3); \
		_mm_storeh_pi((__m64 *)(&v[0]), xmm3); \
	} \
}*/
#define VectorNormalizeOnly(v) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6; \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v[2]), (const __m64 *)(&v[0])); \
	xmm3 = xmm2; \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm4 = _mm_movehdup_ps(xmm2); \
	xmm6 = _mm_add_ps(xmm2, xmm4); \
	xmm4 = _mm_movehl_ps(xmm4, xmm6); \
	xmm2 = _mm_add_ss(xmm6, xmm4); \
	xmm0 = _mm_sqrt_ss(xmm2); \
	if (_mm_cvtss_f32(xmm0) != 0.0f) { \
		xmm1 = _mm_rcp_ss(xmm0); \
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0); \
		xmm3 = _mm_mul_ps(xmm3, xmm1); \
		_mm_store_ss(&v[2], xmm3); \
		_mm_storeh_pi((__m64 *)(&v[0]), xmm3); \
	} \
}

// The vector4 version takes the length of the vec4.xyz, but scales the vec4.w also (if length != 0)
#define Vector4NormalizeOnly(v) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6; \
	xmm3 = _mm_loadu_ps(&v[0]); \
	xmm2 = _mm_loadh_pi(_mm_load_ss(&v[2]), (const __m64 *)(&v[0])); \
	xmm2 = _mm_mul_ps(xmm2, xmm2); \
	xmm4 = _mm_movehdup_ps(xmm2); \
	xmm6 = _mm_add_ps(xmm2, xmm4); \
	xmm4 = _mm_movehl_ps(xmm4, xmm6); \
	xmm2 = _mm_add_ss(xmm6, xmm4); \
	xmm0 = _mm_sqrt_ss(xmm2); \
	if (_mm_cvtss_f32(xmm0) != 0.0f) { \
		xmm1 = _mm_rcp_ss(xmm0); \
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0); \
		xmm3 = _mm_mul_ps(xmm3, xmm1); \
		_mm_storeu_ps(&v[0], xmm3); \
	} \
}

///#define VectorNormalize2Only(v, out) vec3_norm2_void(v, out)
/*#define VectorNormalize2Only(v, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3; \
	xmm2 = _mm_loadh_pi( _mm_load_ss(&v[2]), (const __m64 *)(&v[0])); \
	xmm3 = xmm2; \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm2 = _mm_hadd_ps(xmm2, xmm2); \
	xmm2 = _mm_hadd_ps(xmm2, xmm2); \
	xmm0 = _mm_sqrt_ss(xmm2); \
	if (_mm_cvtss_f32(xmm0) != 0.0f) { \
		xmm1 = _mm_rcp_ss(xmm0); \
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0); \
		xmm3 = _mm_mul_ps(xmm3, xmm1); \
		_mm_store_ss(&out[2], xmm3); \
		_mm_storeh_pi((__m64 *)(&out[0]), xmm3); \
	} else { \
		xmm3 = _mm_xor_ps(xmm3, xmm3); \
		_mm_store_ss(&out[2], xmm3); \
		_mm_storeh_pi((__m64 *)(&out[0]), xmm3); \
	} \
}*/
#define VectorNormalize2Only(v, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm6; \
	xmm2 = _mm_loadh_pi( _mm_load_ss(&v[2]), (const __m64 *)(&v[0])); \
	xmm3 = xmm2; \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm4 = _mm_movehdup_ps(xmm2); \
	xmm6 = _mm_add_ps(xmm2, xmm4); \
	xmm4 = _mm_movehl_ps(xmm4, xmm6); \
	xmm2 = _mm_add_ss(xmm6, xmm4); \
	xmm0 = _mm_sqrt_ss(xmm2); \
	if (_mm_cvtss_f32(xmm0) != 0.0f) { \
		xmm1 = _mm_rcp_ss(xmm0); \
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0); \
		xmm3 = _mm_mul_ps(xmm3, xmm1); \
		_mm_store_ss(&out[2], xmm3); \
		_mm_storeh_pi((__m64 *)(&out[0]), xmm3); \
	} else { \
		xmm3 = _mm_xor_ps(xmm3, xmm3); \
		_mm_store_ss(&out[2], xmm3); \
		_mm_storeh_pi((__m64 *)(&out[0]), xmm3); \
	} \
}


///#define VectorSubtract(a, b, c) _VectorSubtract(a, b, c)
#define VectorSubtract(a, b, c) \
{ \
	__m128 xmm1, xmm3; \
	xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)a), (const __m64 *)(a+1)); \
	xmm3 = _mm_loadh_pi(_mm_load_ss((const float *)b), (const __m64 *)(b+1)); \
	xmm1 = _mm_sub_ps(xmm1, xmm3); \
	_mm_store_ss(&c[0], xmm1); \
	_mm_storeh_pi((__m64 *)(&c[1]), xmm1); \
}

//!! You'll get a warning on this macro (xmm0 not initialized)
///#define Vector2Subtract(a, b, c) _Vector2Subtract(a, b, c)
#define Vector2Subtract(a, b, c) \
{ \
	__m128 xmm0, xmm1; \
	xmm0 = _mm_loadl_pi(xmm0, (const __m64 *)&a[0]); \
	xmm1 = _mm_loadl_pi(xmm1, (const __m64 *)&b[0]); \
	xmm0 = _mm_sub_ps(xmm0, xmm1); \
	_mm_storel_pi((__m64 *)&c[0], xmm0); \
}

///#define Vector4Subtract(a, b, c) vec4_sub(a, b, c)
#define Vector4Subtract(a, b, c) \
{ \
	__m128 xmm0, xmm1; \
	xmm0 = _mm_loadu_ps(&a[0]); \
	xmm1 = _mm_loadu_ps(&b[0]); \
	xmm0 = _mm_sub_ps(xmm0, xmm1); \
	_mm_storeu_ps(&c[0], xmm0); \
}

///#define VectorAdd(a, b, c) _VectorAdd(a, b, c)
#define VectorAdd(a, b, c) \
{ \
	__m128 xmm1, xmm3; \
	xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)a), (const __m64 *)(a+1)); \
	xmm3 = _mm_loadh_pi(_mm_load_ss((const float *)b), (const __m64 *)(b+1)); \
	xmm1 = _mm_add_ps(xmm1, xmm3); \
	_mm_store_ss((float *)c, xmm1); \
	_mm_storeh_pi((__m64 *)(c+1), xmm1); \
}

#define Vector2AddConst(v, value, out) \
{ \
	__m128 xmm0; \
	xmm0 = _mm_loadl_pi(xmm0, (const __m64 *)v); \
	xmm0 = _mm_add_ps(xmm0, _mm_set_ps1(value)); \
	_mm_storel_pi((__m64 *)out, xmm0); \
}

///#define VectorCopy(a, b) _VectorCopy(a, b)
#define VectorCopy(a, b) \
{ \
	__m128 xmm0; \
	xmm0 = _mm_loadh_pi(_mm_load_ss((const float *)a), (const __m64 *)(a+1)); \
	_mm_store_ss((float *)b, xmm0); \
	_mm_storeh_pi((__m64 *)(b+1), xmm0); \
}

//!! You'll get a warning on this macro (xmm0 not initialized)
///#define Vector2Copy(a,b) vec2_copy(a, b)
#define Vector2Copy(a,b) \
{ \
	__m128 xmm0; \
	_mm_storeh_pi((__m64 *)b, _mm_loadh_pi(xmm0, (const __m64 *)a)); \
}

///#define Vector4Copy(a, b) vec4_copy(a, b)
#define Vector4Copy(a, b) \
{ \
	_mm_storeu_ps((float *)b, _mm_loadu_ps((const float *)a)); \
}

///#define VectorScale(v, s, o) _VectorScale(v, s, o)
#define VectorScale(v, s, o) \
{ \
	__m128 xmm3; \
	xmm3 = _mm_loadh_pi(_mm_load_ss((const float *)v), (const __m64 *)(v+1)); \
	xmm3 = _mm_mul_ps(xmm3, _mm_set_ps1(s)); \
	_mm_store_ss((float *)o, xmm3); \
	_mm_storeh_pi((__m64 *)(o+1), xmm3); \
}

//!! You'll get a warning on this macro (xmm0 not initialized)
///#define Vector2Scale(v, s, o) _Vector2Scale(v, s, o)
#define Vector2Scale(v, s, o) \
{ \
	__m128 xmm0; \
	xmm0 = _mm_loadl_pi(xmm0, (const __m64 *)v); \
	xmm0 = _mm_mul_ps(xmm0, _mm_set_ps1(s)); \
	_mm_storel_pi((__m64 *)o, xmm0); \
}

///#define Vector4Scale(v, s, o) vec4_scale(v, s, o)
#define Vector4Scale(v, s, o) \
{ \
	__m128 xmm0; \
	xmm0 = _mm_mul_ps(_mm_loadu_ps((const float *)v), _mm_set_ps1(s)); \
	_mm_storeu_ps((float *)o, xmm0); \
}

///#define VectorMA(v, s, b, o) _VectorMA(v, s, b, o)
#define VectorMA(v, s, b, o) \
{ \
	__m128 xmm2, xmm3; \
	xmm3 = _mm_loadh_pi( _mm_load_ss((const float *)b), (const __m64 *)(b+1)); \
	xmm3 = _mm_mul_ps(xmm3, _mm_set_ps1(s)); \
	xmm2 = _mm_loadh_pi(_mm_load_ss((const float *)v), (const __m64 *)(v+1)); \
	xmm2 = _mm_add_ps(xmm2, xmm3); \
	_mm_store_ss((float *)o, xmm2); \
	_mm_storeh_pi((__m64 *)(o+1), xmm2); \
}

///#define Vector4MA(v, s, b, o) vec4_ma(v, s, b, o)
#define Vector4MA(v, s, b, o) \
{ \
	__m128 xmm2, xmm3; \
	xmm3 = _mm_loadu_ps((const float *)b); \
	xmm2 = _mm_loadu_ps((const float *)v); \
	xmm3 = _mm_mul_ps(xmm3, _mm_set_ps1(s)); \
	xmm2 = _mm_add_ps(xmm2, xmm3); \
	_mm_storeu_ps((float *)o, xmm2); \
}

///#define VectorAM(v, b, s, o) _VectorAM(v, b, s, o)
#define VectorAM(v, b, s, o) \
{ \
	__m128 xmm2, xmm3; \
	xmm3 = _mm_loadh_pi(_mm_load_ss((const float *)b), (const __m64 *)(b+1)); \
	xmm2 = _mm_loadh_pi(_mm_load_ss((const float *)v), (const __m64 *)(v+1)); \
	xmm2 = _mm_add_ps(xmm2, xmm3); \
	xmm2 = _mm_mul_ps(xmm2, _mm_set_ps1(s)); \
	_mm_store_ss((float *)o, xmm2); \
	_mm_storeh_pi((__m64 *)(o+1), xmm2); \
}

//!! You'll get a warning on this macro (xmm0 & xmm1 not initialized)
///#define Vector2AM(v, b, s, o) _Vector2AM(v, b, s, o)
#define Vector2AM(v, b, s, o) \
{ \
	__m128 xmm0, xmm1; \
	xmm0 = _mm_loadl_pi(xmm0, (const __m64 *)v); \
	xmm1 = _mm_loadl_pi(xmm1, (const __m64 *)b); \
	xmm0 = _mm_add_ps(xmm0, xmm1); \
	xmm0 = _mm_mul_ps(xmm0, _mm_set_ps1(s)); \
	_mm_storel_pi((__m64 *)o, xmm0); \
}

///#define Vector4AM(v, b, s, o) _Vector4AM(v, b, s, o)
#define Vector4AM(v, b, s, o) \
{ \
	__m128 xmm1, xmm2; \
	xmm1 = _mm_loadu_ps((const float *)v); \
	xmm2 = _mm_loadu_ps((const float *)b); \
	xmm1 = _mm_add_ps(xmm1, xmm2); \
	xmm1 = _mm_mul_ps(xmm1, _mm_set_ps1(s)); \
	_mm_storeu_ps((float *)o, xmm1); \
}

///#define VectorMultiply(v1, v2, o) _VectorMultiply(v1, v2, o)
#define VectorMultiply(v1, v2, o) \
{ \
	__m128 xmm1, xmm2; \
	xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)v1), (const __m64 *)(v1+1)); \
	xmm2 = _mm_loadh_pi(_mm_load_ss((const float *)v2), (const __m64 *)(v2+1)); \
	xmm2 = _mm_mul_ps(xmm2, xmm1); \
	_mm_store_ss((float *)o, xmm2); \
	_mm_storeh_pi((__m64 *)(o+1), xmm2); \
}

///#define VectorAddConst(v, value, o) _VectorAddConst(v, value, o)
#define VectorAddConst(v, value, o) \
{ \
	__m128 xmm1; \
	xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)v), (const __m64 *)(v+1)); \
	xmm1 = _mm_add_ps(xmm1, _mm_set_ps1(value)); \
	_mm_store_ss((float *)o, xmm1); \
	_mm_storeh_pi((__m64 *)(o+1), xmm1); \
}

///#define VectorRotate(in, matrix, out) vec3_rotate(in, matrix, out)
#define VectorRotate(in, matrix, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5; \
	xmm2 = _mm_load_ss(&in[2]); \
	xmm2 = _mm_shuffle_ps(xmm2, xmm2, 0); \
	xmm1 = _mm_loadh_pi(xmm2, (const __m64 *)(&in[0])); \
	xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010); \
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111); \
	xmm3 = _mm_loadu_ps(&matrix[0][0]); \
	xmm4 = _mm_loadu_ps(&matrix[1][1]); \
	xmm5 = _mm_load_ss(&matrix[2][2]); \
	xmm5 = _mm_shuffle_ps(xmm5, xmm4, 0b11100100); \
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, 0b01001110); \
	xmm4 = _mm_shuffle_ps(xmm4, xmm3, 0b11110100); \
	xmm4 = _mm_shuffle_ps(xmm4, xmm4, 0b11010010); \
	xmm3 = _mm_mul_ps(xmm3, xmm0); \
	xmm4 = _mm_mul_ps(xmm4, xmm1); \
	xmm5 = _mm_mul_ps(xmm5, xmm2); \
	xmm5 = _mm_add_ps(xmm5, xmm3); \
	xmm5 = _mm_add_ps(xmm5, xmm4); \
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, 0b10011100); \
	_mm_store_ss(&out[0], xmm5); \
	_mm_storeh_pi((__m64 *)(&out[1]), xmm5); \
}

///#define MatrixMultiply(in1, in2, o) _MatrixMultiply(in1, in2, o)
#define MatrixMultiply(in1, in2, o) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
	xmm1 = _mm_loadu_ps((&in2[0][0])); \
	xmm3 = _mm_loadu_ps((&in2[1][0])); \
	xmm5 = _mm_loadu_ps((&in2[1][2])); \
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, 0b00111001); \
	xmm7 = _mm_loadu_ps((&in1[0][0])); \
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0); \
	xmm2 = _mm_shuffle_ps(xmm7, xmm7, 0b01010101); \
	xmm4 = _mm_shuffle_ps(xmm7, xmm7, 0b10101010); \
	xmm0 = _mm_mul_ps(xmm0, xmm1); \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm4 = _mm_mul_ps(xmm4, xmm5); \
	xmm0 = _mm_add_ps(xmm0, xmm2); \
	xmm0 = _mm_add_ps(xmm0, xmm4); \
	_mm_storeu_ps((float *)(&o[0][0]), xmm0); \
	xmm7 = _mm_loadu_ps((&in1[1][0])); \
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0); \
	xmm2 = _mm_shuffle_ps(xmm7, xmm7, 0b01010101); \
	xmm4 = _mm_shuffle_ps(xmm7, xmm7, 0b10101010); \
	xmm0 = _mm_mul_ps(xmm0, xmm1); \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm4 = _mm_mul_ps(xmm4, xmm5); \
	xmm0 = _mm_add_ps(xmm0, xmm2); \
	xmm6 = _mm_add_ps(xmm0, xmm4); \
	_mm_storeu_ps((float *)(&o[1][0]), xmm6); \
	xmm7 = _mm_loadu_ps((&in1[1][2])); \
	xmm0 = _mm_shuffle_ps(xmm7, xmm7, 0b01010101); \
	xmm2 = _mm_shuffle_ps(xmm7, xmm7, 0b10101010); \
	xmm4 = _mm_shuffle_ps(xmm7, xmm7, 0b11111111); \
	xmm0 = _mm_mul_ps(xmm0, xmm1); \
	xmm2 = _mm_mul_ps(xmm2, xmm3); \
	xmm4 = _mm_mul_ps(xmm4, xmm5); \
	xmm0 = _mm_add_ps(xmm0, xmm2); \
	xmm0 = _mm_add_ps(xmm0, xmm4); \
	xmm6 = _mm_shuffle_ps(xmm6, xmm0, 0b00001010); \
	xmm6 = _mm_shuffle_ps(xmm6, xmm0, 0b10011100); \
	_mm_storeu_ps((float *)(&o[1][2]), xmm6); \
}

///#define Matrix4Multiply(a, b, out) mat4_mult(a, b, out)
#define Matrix4Multiply(a, b, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
	xmm4 = _mm_loadu_ps(&a[0]); \
	xmm5 = _mm_loadu_ps(&a[4]); \
	xmm6 = _mm_loadu_ps(&a[8]); \
	xmm7 = _mm_loadu_ps(&a[12]); \
	xmm0 = _mm_loadu_ps(&b[0]); \
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
	_mm_storeu_ps(&out[0], xmm0); \
	xmm0 = _mm_loadu_ps(&b[4]); \
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
	_mm_storeu_ps(&out[4], xmm0); \
	xmm0 = _mm_loadu_ps(&b[8]); \
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
	_mm_storeu_ps(&out[8], xmm0); \
	xmm0 = _mm_loadu_ps(&b[12]); \
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
	_mm_storeu_ps(&out[12], xmm0); \
}

// m *= m2
///#define Matrix4MultiplyWith(m, m2) mat4_mult_self(m, m2)
#define Matrix4MultiplyWith(m, m2) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
	xmm4 = _mm_loadu_ps(&m[0]); \
	xmm5 = _mm_loadu_ps(&m[4]); \
	xmm6 = _mm_loadu_ps(&m[8]); \
	xmm7 = _mm_loadu_ps(&m[12]); \
	xmm0 = _mm_loadu_ps(&m2[0]); \
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
	xmm0 = _mm_loadu_ps(&m2[4]); \
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
	xmm0 = _mm_loadu_ps(&m2[8]); \
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
	xmm0 = _mm_loadu_ps(&m2[12]); \
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

///#define Vector4TransformM4(m, in, out) mat4_transform_vec4(m, in, out)
#define Vector4TransformM4(m, in, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
	xmm7 = _mm_loadu_ps((const float *)in); \
	xmm0 = _mm_loadu_ps((const float *)m); \
	xmm1 = _mm_loadu_ps((const float *)(m+4)); \
	xmm2 = _mm_loadu_ps((const float *)(m+8)); \
	xmm3 = _mm_loadu_ps((const float *)(m+12)); \
	xmm4 = _mm_shuffle_ps(xmm7, xmm7, 0b00000000); \
	xmm5 = _mm_shuffle_ps(xmm7, xmm7, 0b01010101); \
	xmm6 = _mm_shuffle_ps(xmm7, xmm7, 0b10101010); \
	xmm7 = _mm_shuffle_ps(xmm7, xmm7, 0b11111111); \
	xmm0 = _mm_mul_ps(xmm0, xmm4); \
	xmm1 = _mm_mul_ps(xmm1, xmm5); \
	xmm2 = _mm_mul_ps(xmm2, xmm6); \
	xmm3 = _mm_mul_ps(xmm3, xmm7); \
	xmm1 = _mm_add_ps(xmm1, xmm0); \
	xmm3 = _mm_add_ps(xmm3, xmm2); \
	xmm3 = _mm_add_ps(xmm3, xmm1); \
	_mm_storeu_ps(out, xmm3); \
}

// TODO: handle instruction pairing to minimize uOps..
///#define VectorTransformM4(m, in, out) mat4_transform_vec3(m, in, out)
#define VectorTransformM4(m, in, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6; \
	xmm1 = _mm_loadh_pi( _mm_load_ss((const float *)in), (const __m64 *)(in+1)); \
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111); \
	xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010); \
	xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b00000000); \
	xmm3 = _mm_loadu_ps((const float *)m); \
	xmm4 = _mm_loadu_ps((const float *)(m+4)); \
	xmm5 = _mm_loadu_ps((const float *)(m+8)); \
	xmm6 = _mm_loadu_ps((const float *)(m+12)); \
	xmm0 = _mm_mul_ps(xmm0, xmm3); \
	xmm1 = _mm_mul_ps(xmm1, xmm4); \
	xmm2 = _mm_mul_ps(xmm2, xmm5); \
	xmm0 = _mm_add_ps(xmm0, xmm1); \
	xmm0 = _mm_add_ps(xmm0, xmm2); \
	xmm0 = _mm_add_ps(xmm0, xmm6); \
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b10010100); \
	_mm_store_ss((float *)out, xmm0); \
	_mm_storeh_pi((__m64 *)(out+1), xmm0); \
}

///#define MatrixTranspose(matrix, transpose) mat3_transpose(matrix, transpose)
#define MatrixTranspose(matrix, transpose) \
{ \
	__m128 xmm0, xmm1, xmm3, xmm4, xmm5; \
	xmm0 = _mm_loadu_ps(&matrix[0][0]); \
	xmm1 = _mm_loadu_ps(&matrix[1][1]); \
	xmm3 = _mm_shuffle_ps(xmm0, xmm1, 0b10100100); \
	xmm4 = _mm_shuffle_ps(xmm0, xmm3, 0b01111100); \
	_mm_storeu_ps(&transpose[0][0], xmm4); \
	xmm3 = _mm_shuffle_ps(xmm0, xmm1, 0b01011010); \
	xmm5 = _mm_shuffle_ps(xmm1, xmm3, 0b11011100); \
	_mm_storeu_ps(&transpose[1][1], xmm5); \
	_mm_store_ss(&transpose[2][2], _mm_load_ss(&matrix[2][2])); \
}

///#define Matrix4Transpose(in, out) mat4_transpose(in, out)
#define Matrix4Transpose(in, out) \
{ \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
	xmm0 = _mm_loadu_ps(&in[0]); \
	xmm1 = _mm_loadu_ps(&in[4]); \
	xmm2 = _mm_loadu_ps(&in[8]); \
	xmm3 = _mm_loadu_ps(&in[12]); \
	xmm4 = _mm_shuffle_ps(xmm1, xmm0, 0b11101110); \
	xmm5 = _mm_shuffle_ps(xmm3, xmm2, 0b11101011); \
	xmm6 = _mm_shuffle_ps(xmm5, xmm4, 0b11011100); \
	_mm_storeu_ps((float *)&out[0], xmm6); \
	xmm7 = _mm_shuffle_ps(xmm5, xmm4, 0b10001001); \
	_mm_storeu_ps((float *)&out[4], xmm7); \
	xmm4 = _mm_shuffle_ps(xmm1, xmm0, 0b01000100); \
	xmm5 = _mm_shuffle_ps(xmm3, xmm2, 0b01000001); \
	xmm6 = _mm_shuffle_ps(xmm5, xmm4, 0b11011100); \
	_mm_storeu_ps((float *)&out[8], xmm6); \
	xmm7 = _mm_shuffle_ps(xmm5, xmm4, 0b10001001); \
	_mm_storeu_ps((float *)&out[12], xmm7); \
}

///#define Matrix4Identity(m) mat4_ident(m)
#define Matrix4Identity(m) \
{ \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f)); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f)); \
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f)); \
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f)); \
}

///#define Matrix4IdentTranslateV3(m, position) mat4_reset_translate_vec3(m, position)
#define Matrix4IdentTranslateV3(m, position) \
{ \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f)); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f)); \
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f)); \
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, position[2], position[1], position[0])); \
}

///#define Matrix4IdentTranslate(m, x, y, z) mat4_reset_translate(m, x, y, z)
#define Matrix4IdentTranslate(m, x, y, z) \
{ \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f)); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f)); \
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f)); \
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, z, y, x)); \
}

///#define Matrix4IdentScale(m, x, y, z) mat4_reset_scale(m, x, y, z)
#define Matrix4IdentScale(m, x, y, z) \
{ \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, 0.0f, 0.0f, x)); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, 0.0f, y, 0.0f)); \
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, z, 0.0f, 0.0f)); \
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f)); \
}

///void MatrixSetupTransformFromVectorsFLU(mat4_t m, const vec3_t forward, const vec3_t left, const vec3_t up, const vec3_t origin)
#define MatrixSetupTransformFromVectorsFLU(m, forward, left, up, origin) \
{ \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, forward[2], forward[1], forward[0])); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, left[2], left[1], left[0])); \
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, up[2], up[1], up[0])); \
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, origin[2], origin[1], origin[0])); \
}

//void MatrixFromVectorsFLU(mat4_t m, const vec3_t forward, const vec3_t left, const vec3_t up);
#define MatrixFromVectorsFLU(m, forward, left, up) \
{ \
	_mm_storeu_ps(&m[0], _mm_set_ps(0.0f, forward[2], forward[1], forward[0])); \
	_mm_storeu_ps(&m[4], _mm_set_ps(0.0f, left[2], left[1], left[0])); \
	_mm_storeu_ps(&m[8], _mm_set_ps(0.0f, up[2], up[1], up[0])); \
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f)); \
}

///void MatrixToVectorsFLU(const mat4_t m, vec3_t forward, vec3_t left, vec3_t up);
#define MatrixToVectorsFLU(m, forward, left, up) \
{ \
	if (forward) VectorCopy(&m[0], forward); \
	if (left) VectorCopy(&m[4], left); \
	if (up) VectorCopy(&m[8], up); \
}

///void MatrixToVectorsFRU(const mat4_t m, vec3_t forward, vec3_t right, vec3_t up);
#define MatrixToVectorsFRU(m, forward, right, up) \
{ \
	if (forward) VectorCopy(&m[0], forward); \
	if (right) { \
		VectorCopy(&m[4], right); \
		VectorInverse(right); \
	} \
	if (up) VectorCopy(&m[8], up); \
}

///#define Matrix4Copy(in, out) mat4_copy(in, out)
#define Matrix4Copy(in, out) \
{ \
	_mm_storeu_ps(&out[0], _mm_loadu_ps(&in[0])); \
	_mm_storeu_ps(&out[4], _mm_loadu_ps(&in[4])); \
	_mm_storeu_ps(&out[8], _mm_loadu_ps(&in[8])); \
	_mm_storeu_ps(&out[12], _mm_loadu_ps(&in[12])); \
}

///#define Matrix4FromQuaternion(m, q) mat4_from_quat(m, q)
#define Matrix4FromQuaternion(m, q) \
{ \
	vec4_t q2, qz2, qq2, qy2; \
	float xx2; \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4; \
	xmm0 = _mm_loadu_ps(&q[0]); \
	xmm1 = _mm_add_ps(xmm0, xmm0); \
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b01010101); \
	xmm2 = _mm_mul_ps(xmm2, xmm0); \
	xmm3 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010); \
	xmm3 = _mm_mul_ps(xmm3, xmm0); \
	xmm4 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111); \
	xmm4 = _mm_mul_ps(xmm4, xmm1); \
	_mm_storeu_ps((float *)&q2, xmm1); \
	_mm_storeu_ps((float *)&qy2, xmm2); \
	_mm_storeu_ps((float *)&qz2, xmm3); \
	_mm_storeu_ps((float *)&qq2, xmm4); \
	xx2 = q[0] * q2[0]; \
	_mm_storeu_ps(&m[0],  _mm_set_ps(0.0f,  qz2[0] - qq2[1],       qy2[0] + qq2[2],       -qy2[1] - qz2[2] + 1.0f)); \
	_mm_storeu_ps(&m[4],  _mm_set_ps(0.0f,  qz2[1] + qq2[0],       -xx2 - qz2[2] + 1.0f,  qy2[0] - qq2[2])); \
	_mm_storeu_ps(&m[8],  _mm_set_ps(0.0f,  -xx2 - qy2[1] + 1.0f,  qz2[1] - qq2[0],       qz2[0] + qq2[1])); \
	_mm_storeu_ps(&m[12], _mm_set_ps(1.0f,  0.0f,                  0.0f,                  0.0f)); \
}

///void quat_to_axis(const quat_t q, vec3_t axis[3]);
#define quat_to_axis(q, axis) \
{ \
	vec4_t q2, qz2, qq2, qy2; \
	float xx2; \
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4; \
	xmm0 = _mm_loadu_ps(&q[0]); \
	xmm1 = _mm_add_ps(xmm0, xmm0); \
	xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b01010101); \
	xmm2 = _mm_mul_ps(xmm2, xmm0); \
	xmm3 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010); \
	xmm3 = _mm_mul_ps(xmm3, xmm0); \
	xmm4 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111); \
	xmm4 = _mm_mul_ps(xmm4, xmm1); \
	_mm_storeu_ps((float *)&q2, xmm1); \
	_mm_storeu_ps((float *)&qy2, xmm2); \
	_mm_storeu_ps((float *)&qz2, xmm3); \
	_mm_storeu_ps((float *)&qq2, xmm4); \
	xx2 = q[0] * q2[0]; \
	_mm_storeu_ps(&axis[0][0],  _mm_set_ps(qy2[0] - qq2[2],  qz2[0] - qq2[1], qy2[0] + qq2[2], -qy2[1] - qz2[2] + 1.0f)); \
	_mm_storeu_ps(&axis[1][1],  _mm_set_ps(qz2[1] - qq2[0], qz2[0] + qq2[1], qz2[1] + qq2[0],  -xx2 - qz2[2] + 1.0f)); \
	axis[2][2] =  -xx2 - qy2[1] + 1.0f; \
}

/*
// Because we're unsure of the input data-types being floats or doubles,
// we cast them to doubles first, so we can use the good old FPU.
#define SinCos(rad, S, C) { \
	double radD = rad, SD, CD; \
	__asm { \
		__asm fld QWORD PTR[radD] \
		__asm fsincos \
		__asm fstp QWORD PTR[CD] \
		__asm fstp QWORD PTR[SD] \
		__asm fwait \
	} \
	S = (float)SD; \
	C = (float)CD; \
}
*/
// If we are sure about the input data-types being floats only,
// we don't need to cast, but load the singles/floats directly.
// Note that arguments rad, S & C must be float variables.
// Those float variables can also not be indexed: vec[1]  or such is invalid..
// You can however do the same, like so: vec+1
#define SinCos(rad, S, C) { \
	__asm { \
		__asm fld DWORD PTR[rad] \
		__asm fsincos \
		__asm fstp DWORD PTR[C] \
		__asm fstp DWORD PTR[S] \
		__asm fwait \
	} \
}


// This one is taken from:
// https://dtosoftware.wordpress.com/2013/01/07/fast-sin-and-cos-functions/
// until i created our own version..
// .. this current version is buggy.  precision issue?
/*#define SinCos(rad, S, C) \
{ \
	const float B = 4.f / M_PI; \
	const float CC = -4.f / (M_PI * M_PI); \
	const float P = 0.225f; \
	__m128 m_abs, m_y, m_sincos, m_cos; \
	__m128 m_x = _mm_set_ps(0.f, 0.f, rad + M_halfPI, rad); \
	__m128 m_pi = _mm_set1_ps(M_PI); \
	__m128 m_mpi = _mm_set1_ps(-M_PI); \
	__m128 m_2pi = _mm_set1_ps(M_PI * 2.0f); \
	__m128 m_B = _mm_set1_ps(B); \
	__m128 m_C = _mm_set1_ps(CC); \
	__m128 m_P = _mm_set1_ps(P); \
	__m128 m1 = _mm_cmpnlt_ps(m_x, m_pi); \
	m1 = _mm_and_ps(m1, m_2pi); \
	m_x = _mm_sub_ps(m_x, m1); \
	m1 = _mm_cmpngt_ps(m_x, m_mpi); \
	m1 = _mm_and_ps(m1, m_2pi); \
	m_x = _mm_add_ps(m_x, m1); \
	m_abs = _mm_andnot_ps(_mm_castsi128_ps(_mm_set1_epi32(0x80000000)), m_x); \
	m1 = _mm_mul_ps(m_abs, m_C); \
	m1 = _mm_add_ps(m1, m_B); \
	m_y = _mm_mul_ps(m1, m_x); \
	m_abs = _mm_andnot_ps(_mm_castsi128_ps(_mm_set1_epi32(0x80000000)), m_y); \
	m1 = _mm_mul_ps(m_abs, m_y); \
	m1 = _mm_sub_ps(m1, m_y); \
	m1 = _mm_mul_ps(m1, m_P); \
	m_sincos = _mm_add_ps(m1, m_y); \
	m_cos = _mm_shuffle_ps(m_sincos, m_sincos, _MM_SHUFFLE(0, 0, 0, 1)); \
	_mm_store_ss(&S, m_sincos); \
	_mm_store_ss(&C, m_cos); \
}*/

///void ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj);
#define ProjectPointOntoVector(point, vStart, vEnd, vProj) \
{ \
	vec3_t pVec, vec; \
	float dot; \
	VectorSubtract(point, vStart, pVec); \
	VectorSubtract(vEnd, vStart, vec); \
	VectorNormalizeOnly(vec); \
	Dot(pVec, vec, dot); \
	VectorMA(vStart, dot, vec, vProj); \
}

///void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
#define ProjectPointOnPlane(dst, p, normal) \
{ \
	vec3_t n; \
	float dotnn, dotnp, d; \
	Dot(normal, normal, dotnn); \
	Dot(normal, p, dotnp); \
	d = dotnp / (dotnn * dotnn); \
	VectorScale(normal, d, n); \
	VectorSubtract(p, n, dst); \
}

///void GetPerpendicularViewVector(const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up);
#define GetPerpendicularViewVector(point, p1, p2, up) \
{ \
	vec3_t v1, v2; \
	VectorSubtract(point, p1, v1); \
	VectorSubtract(point, p2, v2); \
	CrossProduct(v1, v2, up); \
	VectorNormalizeOnly(up); \
}

///void MakeNormalVectors(const vec3_t forward, vec3_t right, vec3_t up);
#define MakeNormalVectors(forward, right, up) \
{ \
	float d; \
	right[1] = -forward[0]; \
	right[2] = forward[1]; \
	right[0] = forward[2]; \
	Dot(right, forward, d); \
	VectorMA(right, -d, forward, right); \
	VectorNormalizeOnly(right); \
	CrossProduct(right, forward, up); \
}

#endif //  ETL_SSE

// You are not supposed to use these functions: Use the macro's
vec_t _DotProduct(const vec3_t v1, const vec3_t v2);
void _VectorSubtract(const vec3_t veca, const vec3_t vecb, vec3_t out);
void _Vector2Subtract(const vec2_t veca, const vec2_t vecb, vec2_t out);
void _VectorAdd(const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorAddConst(const vec3_t v, const float value, vec3_t out);
void _Vector2AddConst(const vec2_t v, const float value, vec2_t out);
void _VectorCopy(const vec3_t in, vec3_t out);
void _VectorScale(const vec3_t in, const vec_t scale, vec3_t out);
void _Vector2Scale(const vec2_t in, const float scale, vec2_t out);
void _VectorMA(const vec3_t veca, const float scale, const vec3_t vecb, vec3_t out);
void _VectorAM(const vec3_t veca, const vec3_t vecb, const float scale, vec3_t out);
void _Vector4Set(const float x, const float y, const float z, const float w, vec4_t out);
void _Vector4AM(const vec4_t veca, const vec4_t vecb, const float scale, vec4_t out);
void _Vector2AM(const vec2_t veca, const vec2_t vecb, const float scale, vec2_t out);
void _MatrixMultiply(const float in1[3][3], const float in2[3][3], float out[3][3]);
void _Short3Vector(const short in[3], vec3_t out);
void _VectorMultiply(const vec3_t v1, const vec3_t v2, vec3_t out);

qboolean CalcTangentBinormal(const vec3_t pos0, const vec3_t pos1, const vec3_t pos2,
	const vec2_t texCoords0, const vec2_t texCoords1, const vec2_t texCoords2,
	vec3_t tangent0, vec3_t binormal0, vec3_t tangent1, vec3_t binormal1, vec3_t tangent2, vec3_t binormal2);

//#define VectorClear(a) vec3_clear(a)
//#define VectorNegate(a, b) vec3_negate(a, b)
//#define VectorInverse(v) vec3_inv(v)
//#define VectorSet(v, x, y, z) vec3_set(v, x, y, z)

//#define Vector2Subtract(a, b, c) vec2_sub(a, b, c)
#define Vector2Set(v, x, y) vec2_set(v, x, y)

//unused
//#define Vector2Copy(a, b) vec2_copy(a, b)

//#define Vector4Set(v, x, y, z, n) vec4_set(v, x, y, z, n)
//#define Vector4Copy(a, b) vec4_copy(a, b)
//#define Vector4MA(v, s, b, o) vec4_ma(v, s, b, o)
#define Vector4Average(v, b, s, o) vec4_average(v, b, s, o)

#define SnapVector(v) vec3_snap(v)

// functions that return a value
#define VectorLength vec3_length
#define VectorLengthSquared vec3_length_squared
#define Distance vec3_distance
#define DistanceSquared vec3_distance_squared
#define VectorNormalize vec3_norm
#define VectorNormalize2 vec3_norm2
#define VectorCompare vec3_compare
#define VectorCompareLT vec3_compare_lt
#define VectorCompareGT vec3_compare_gt
//#define CrossProduct vec3_cross
//#define VectorNormalizeFast vec3_norm_fast
//#define VectorRotate vec3_rotate

static ID_INLINE int VectorCompareEpsilon(const vec3_t v1, const vec3_t v2, float epsilon)
{
	vec3_t d;
	VectorSubtract(v1, v2, d);
	/*d[0] = fabs(d[0]);
	d[1] = fabs(d[1]);
	d[2] = fabs(d[2]);*/
	VectorAbs(d, d);
	if(d[0] > epsilon || d[1] > epsilon || d[2] > epsilon)
		return 0;
	return 1;
}

#define AngleMod angle_mod
#define LerpAngle angle_lerp
//#define LerpPosition vec3_lerp
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

//#define MatrixTranspose mat3_transpose
#define AngleVectors angles_vectors
#define PerpendicularVector(out, src) vec3_per(src, out) // rotated the params to match the way other functions are written

#pragma warning(default:4700)
#pragma warning(default:4010)

#endif
