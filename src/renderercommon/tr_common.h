/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
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
 *
 * @file tr_public.h
 */

#ifndef __TR_COMMON_H
#define __TR_COMMON_H
#include "../qcommon/q_shared.h"
#include "tr_public.h"
#include "iqm.h"
#include "qgl.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern refimport_t ri;

#ifdef ZONE_DEBUG
#define Ren_Malloc(size)    ri.Z_MallocDebug(size, # size, __FILE__, __LINE__)
#else
#define Ren_Malloc(size)    ri.Z_Malloc(size, # size)
#endif

// image buffer
typedef enum
{
	BUFFER_IMAGE,
	BUFFER_SCALED,
	BUFFER_RESAMPLED,
	BUFFER_MAX_TYPES
} bufferMemType_t;
void *R_GetImageBuffer(int size, bufferMemType_t bufferType);
void R_FreeImageBuffer(void);

// These two variables should live inside glConfig but can't because of compatibility issues to the original ID vms.
// If you release a stand-alone game and your mod uses tr_types.h from this build you can safely move them to
// the glconfig_t struct.
extern qboolean textureFilterAnisotropic;
extern int      maxAnisotropy;
extern float    displayAspect;

// cvars

extern cvar_t *r_flareSize;
extern cvar_t *r_flareFade;

extern cvar_t *r_railWidth;
//extern cvar_t *r_railCoreWidth; // renderer2 only
extern cvar_t *r_railSegmentLength;

extern cvar_t *r_ignore;                // used for debugging anything

extern cvar_t *r_ignoreFastPath;        // FIXME: move out -> renderer1 only - allows us to ignore our Tess fast paths

extern cvar_t *r_znear;                 // near Z clip plane
extern cvar_t *r_zfar;                  // far Z clip plane

extern cvar_t *r_stencilbits;           // number of desired stencil bits
extern cvar_t *r_depthbits;             // number of desired depth bits
extern cvar_t *r_colorbits;             // number of desired color bits, only relevant for fullscreen
extern cvar_t *r_texturebits;           // number of desired texture bits
                                        // 0 = use framebuffer depth
                                        // 16 = use 16-bit textures
                                        // 32 = use 32-bit textures
                                        // all else = error
extern cvar_t *r_ext_multisample;
extern cvar_t *r_measureOverdraw;       // enables stencil buffer overdraw measurement

extern cvar_t *r_lodbias;               // push/pull LOD transitions
extern cvar_t *r_lodscale;

extern cvar_t *r_primitives;            // "0" = based on compiled vertex array existance
                                        // "1" = glDrawElemet tristrips
                                        // "2" = glDrawElements triangles
                                        // "-1" = no drawing

extern cvar_t *r_fastsky;               // controls whether sky should be cleared or drawn
extern cvar_t *r_drawSun;               // controls drawing of sun quad
                                        // "0" no sun
                                        // "1" draw sun
                                        // "2" also draw lens flare effect centered on sun
extern cvar_t *r_dynamiclight;          // dynamic lights enabled/disabled

extern cvar_t *r_norefresh;             // bypasses the ref rendering
extern cvar_t *r_drawentities;          // disable/enable entity rendering
extern cvar_t *r_drawworld;             // disable/enable world rendering
extern cvar_t *r_drawfoliage;           // disable/enable foliage rendering
extern cvar_t *r_speeds;                // various levels of information display
extern cvar_t *r_detailTextures;        // enables/disables detail texturing stages
extern cvar_t *r_novis;                 // disable/enable usage of PVS
extern cvar_t *r_nocull;
extern cvar_t *r_facePlaneCull;         // enables culling of planar surfaces with back side test
extern cvar_t *r_nocurves;
extern cvar_t *r_showcluster;

extern cvar_t *r_mode;                  // video mode
extern cvar_t *r_oldMode;               // previous "good" video mode
extern cvar_t *r_fullscreen;
extern cvar_t *r_noborder;
extern cvar_t *r_gamma;
extern cvar_t *r_ignorehwgamma;         // overrides hardware gamma capabilities

extern cvar_t *r_allowExtensions;               // global enable/disable of OpenGL extensions
extern cvar_t *r_ext_compressed_textures;       // these control use of specific extensions
extern cvar_t *r_ext_multitexture;
extern cvar_t *r_ext_compiled_vertex_array;
extern cvar_t *r_ext_texture_env_add;

extern cvar_t *r_ext_texture_filter_anisotropic;
extern cvar_t *r_ext_max_anisotropy;

extern cvar_t *r_nobind;                        // turns off binding to appropriate textures
extern cvar_t *r_singleShader;                  // make most world faces use default shader
extern cvar_t *r_roundImagesDown;
extern cvar_t *r_colorMipLevels;                // development aid to see texture mip usage
extern cvar_t *r_picmip;                        // controls picmip values
extern cvar_t *r_finish;
extern cvar_t *r_drawBuffer;
extern cvar_t *r_swapInterval;
extern cvar_t *r_textureMode;
extern cvar_t *r_offsetFactor;
extern cvar_t *r_offsetUnits;

extern cvar_t *r_lightmap;                      // render lightmaps only
extern cvar_t *r_uiFullScreen;                  // ui is running fullscreen

extern cvar_t *r_logFile;                       // number of frames to emit GL logs
extern cvar_t *r_showtris;                      // enables wireframe rendering of the world
extern cvar_t *r_trisColor;                     // enables modifying of the wireframe colour (in 0xRRGGBB[AA] format, alpha defaults to FF)
extern cvar_t *r_showsky;                       // forces sky in front of all surfaces
extern cvar_t *r_shownormals;                   // draws wireframe normals
extern cvar_t *r_normallength;                  // length of the normals
//extern cvar_t *r_showmodelbounds;			    // see RB_MDM_SurfaceAnim()
extern cvar_t *r_clear;                         // force screen clear every frame

extern cvar_t *r_shadows;                       // controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern cvar_t *r_flares;                        // light flares

extern cvar_t *r_portalsky;
extern cvar_t *r_intensity;

extern cvar_t *r_lockpvs;
extern cvar_t *r_noportals;
extern cvar_t *r_portalOnly;

extern cvar_t *r_subdivisions;
extern cvar_t *r_lodCurveError;

extern cvar_t *r_skipBackEnd;

extern cvar_t *r_stereoEnabled;

extern cvar_t *r_greyscale; // FIXME: move out -> renderer1 only

extern cvar_t *r_ignoreGLErrors;

extern cvar_t *r_overBrightBits;
extern cvar_t *r_mapOverBrightBits;

extern cvar_t *r_debugSurface;
extern cvar_t *r_simpleMipMaps;

extern cvar_t *r_showImages;
extern cvar_t *r_debugSort;

extern cvar_t *r_printShaders;
extern cvar_t *r_saveFontData;

extern cvar_t *r_cache;           // FIXME: move out -> renderer1 only
extern cvar_t *r_cacheShaders;    // FIXME: move out -> renderer1 only
extern cvar_t *r_cacheModels;     // FIXME: move out -> renderer1 only

extern cvar_t *r_cacheGathering;  // FIXME: move out -> renderer1 only

extern cvar_t *r_bonesDebug;      // FIXME: move out -> renderer1 only

extern cvar_t *r_wolffog;

//extern cvar_t *r_screenshotJpegQuality;

extern cvar_t *r_maxpolys;
extern cvar_t *r_maxpolyverts;

#if defined(__cplusplus)
}
#endif

#endif  // __TR_COMMON_H
