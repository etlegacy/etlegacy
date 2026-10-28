/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file vk_gl_compat.h
 * @brief renderer_vk is a pure Vulkan renderer and never links against an
 * OpenGL implementation. A handful of frontend files shared with renderer1
 * (tr_main.c, tr_surface.c, tr_animation_md*.c, tr_sky.c, tr_flares.c,
 * tr_shadows.c, tr_cmds.c) still contain a small number of legacy immediate
 * mode / debug-only GL calls (fog state, on-screen debug polygons/text,
 * display list screenshots). Rather than fork those files, this header
 * defines the minimal set of GL types/constants they reference and turns
 * every gl* call into a no-op, so that logic which has nothing to do with
 * OpenGL (frustum culling, LOD selection, scene setup, matrix math, ...)
 * can keep being reused as-is.
 *
 * None of the stubbed calls are load-bearing for rendering: actual drawing
 * for this renderer happens through vulkan/vk.h.
 */

#ifndef VK_GL_COMPAT_H
#define VK_GL_COMPAT_H

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef int GLsizei;
typedef float GLfloat;
typedef char GLchar;

#define GL_FALSE 0
#define GL_TRUE  1

#define GL_ALWAYS               0x0207
#define GL_BACK                 0x0405
#define GL_BLEND                0x0BE2
#define GL_CLIP_PLANE0          0x3000
#define GL_CULL_FACE            0x0B44
#define GL_DECR                 0x1E03
#define GL_DEPTH_COMPONENT      0x1902
#define GL_DONT_CARE            0x1100
#define GL_EXP                  0x0800
#define GL_FLOAT                0x1406
#define GL_FOG                  0x0B60
#define GL_FOG_COLOR            0x0B66
#define GL_FOG_DENSITY          0x0B62
#define GL_FOG_END              0x0B64
#define GL_FOG_HINT             0x0C54
#define GL_FOG_MODE             0x0B65
#define GL_FOG_START            0x0B63
#define GL_FRONT                0x0404
#define GL_INCR                 0x1E02
#define GL_KEEP                 0x1E00
#define GL_LINEAR               0x2601
#define GL_LINES                0x0001
#define GL_LIST_BIT             0x00020000
#define GL_MODELVIEW            0x1700
#define GL_MODULATE             0x2100
#define GL_NOTEQUAL             0x0205
#define GL_ONE                  1
#define GL_ONE_MINUS_SRC_ALPHA  0x0303
#define GL_POINTS               0x0000
#define GL_POLYGON              0x0009
#define GL_PROJECTION           0x1701
#define GL_QUADS                0x0007
#define GL_SRC_ALPHA            0x0302
#define GL_SRC_ALPHA_SATURATE   0x0308
#define GL_SRC_COLOR            0x0300
#define GL_DST_ALPHA            0x0304
#define GL_ONE_MINUS_SRC_COLOR  0x0301
#define GL_ONE_MINUS_DST_ALPHA  0x0305
#define GL_STENCIL_TEST         0x0B90
#define GL_TRIANGLE_STRIP       0x0005
#define GL_UNSIGNED_BYTE        0x1401
#define GL_ZERO                 0
#define GL_COLOR_BUFFER_BIT     0x00004000
#define GL_DEPTH_BUFFER_BIT     0x00000100
#define GL_STENCIL_BUFFER_BIT   0x00000400
#define GL_ADD                  0x0104
#define GL_DECAL                0x2101
#define GL_REPLACE              0x1E01
#define GL_DST_COLOR            0x0306
#define GL_ONE_MINUS_DST_COLOR  0x0307

// additional state/texture/image constants referenced by tr_backend.c /
// tr_image.c (also stubbed; real Vulkan state/texture handling lives in
// vulkan/vk.h)
#define GL_ALPHA_TEST             0x0BC0
#define GL_BACK_LEFT              0x0402
#define GL_CLAMP                  0x2900
#define GL_CLAMP_TO_EDGE          0x812F
#define GL_DEPTH_TEST             0x0B71
#define GL_EQUAL                  0x0202
#define GL_FILL                   0x1B02
#define GL_GEQUAL                 0x0206
#define GL_GREATER                0x0204
#define GL_LEQUAL                 0x0203
#define GL_LESS                   0x0201
#define GL_LINE                   0x1B01
#define GL_LUMINANCE              0x1909
#define GL_LUMINANCE_ALPHA        0x190A
#define GL_LUMINANCE8             0x8040
#define GL_LUMINANCE16            0x8042
#define GL_LUMINANCE8_ALPHA8      0x8045
#define GL_LUMINANCE16_ALPHA16    0x8048
#define GL_NEAREST                0x2600
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST  0x2701
#define GL_NEAREST_MIPMAP_LINEAR  0x2702
#define GL_LINEAR_MIPMAP_LINEAR   0x2703
#define GL_RGBA8                  0x8058
#define GL_RGB8                   0x8051
#define GL_RGBA4                  0x8056
#define GL_RGB5                   0x8050
#define GL_RGB4_S3TC              0x83A1
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define GL_NO_ERROR               0
#define GL_REPEAT                 0x2901
#define GL_RGB                    0x1907
#define GL_RGBA                   0x1908
#define GL_SCISSOR_TEST           0x0C11
#define GL_TEXTURE0_ARB           0x84C0
#define GL_TEXTURE1_ARB           0x84C1
#define GL_TEXTURE_2D             0x0DE1
#define GL_TEXTURE_BORDER_COLOR   0x1004
#define GL_TEXTURE_MAG_FILTER     0x2800
#define GL_TEXTURE_MIN_FILTER     0x2801
#define GL_TEXTURE_WRAP_S         0x2802
#define GL_TEXTURE_WRAP_T         0x2803
#define GL_TRUE_EXT               GL_TRUE
#define GL_UNPACK_ALIGNMENT       0x0CF5
#define GL_UNSIGNED_INT           0x1405
#define GL_UNSIGNED_SHORT         0x1403

// Primitive modes used by the immediate-mode emulation (VKimm_*, tr_backend.c)
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP             0x0002
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP            0x0003
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES             0x0004
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN          0x0006
#endif
#ifndef GL_QUAD_STRIP
#define GL_QUAD_STRIP            0x0008
#endif

// No gl* functions are declared here: the Vulkan renderer calls its own
// backend directly (vulkan/vk.h, VKimm_* in tr_backend.c). Only the GL
// enums/types still used as plain values by the shared tr_*.c code
// (wrap modes, fog modes, blend/texture-env enums, ...) remain above.

#endif // VK_GL_COMPAT_H
