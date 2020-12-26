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
 * @file tr_common.h
 */

#ifndef INCLUDE_TR_COMMON_H
#define INCLUDE_TR_COMMON_H

#include "../qcommon/q_shared.h"
#include "tr_public.h"
#include "iqm.h"
#ifdef FEATURE_RENDERER_GLES
#   include "../rendererGLES/qgl.h"
#else
#   ifdef BUNDLED_GLEW
#      include "GL/glew.h"
#   else
#      include <GL/glew.h>
#   endif
#endif

extern refimport_t ri;

// image buffer
typedef enum
{
	BUFFER_IMAGE,
	BUFFER_SCALED,
	BUFFER_RESAMPLED,
	BUFFER_MAX_TYPES
} bufferMemType_t;
void *R_GetImageBuffer(int size, bufferMemType_t bufferType, const char *filename);
void R_FreeImageBuffer(void);

/*
====================================================================
IMPLEMENTATION SPECIFIC GLIMP FUNCTIONS
====================================================================
*/
extern int gl_NormalFontBase;

#ifdef ETLEGACY_DEBUG
#define RENLOG r_logFile->integer
#define Ren_LogComment(...) if (RENLOG) { ri.Printf(PRINT_DEVELOPER, __VA_ARGS__); }
#define Ren_Developer(...) ri.Printf(PRINT_DEVELOPER, __VA_ARGS__)
#else
#define RENLOG 0
#define Ren_LogComment(...)
#define Ren_Developer(...) ri.Printf(PRINT_DEVELOPER, __VA_ARGS__)
#endif

#define Ren_UpdateScreen() ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n")

#define Ren_Print(...) ri.Printf(PRINT_ALL, __VA_ARGS__)
#define Ren_Warning(...) ri.Printf(PRINT_WARNING, __VA_ARGS__)

#define Ren_Drop(...) ri.Error(ERR_DROP, __VA_ARGS__)
#define Ren_Fatal(...) ri.Error(ERR_FATAL, __VA_ARGS__)

#define Ren_Assert(x) if (x) { etl_assert(qfalse); Ren_Fatal("Ren_Assert: %s failed at %s (%s:%d)\n", #x, __FUNCTION__, __FILE__, __LINE__); }

void RE_InitOpenGl(void);
int RE_InitOpenGlSubsystems(void);

void R_DoGLimpShutdown(void);
void R_PrintLongString(const char *string);

// font stuff
void R_InitFreeType(void);
void R_DoneFreeType(void);
void RE_RegisterFont(const char *fontName, int pointSize, void *output, qboolean extended);

// noise stuff
double R_NoiseGet4f(double x, double y, double z, double t);
void R_NoiseInit(void);

// NOTE: These two variables should live inside glConfig but can't because of compatibility issues to the original ID vms.
// If you release a stand-alone game and your mod uses tr_types.h from this build you can safely move them to
// the glconfig_t struct.
extern qboolean textureFilterAnisotropic;
extern float    maxAnisotropy;

// cvars used by both renderers

extern cvar_t *r_flareSize;
extern cvar_t *r_flareFade;

#define FLARE_STDCOEFF "150"
extern cvar_t *r_flareCoeff;

extern cvar_t *r_railWidth;
extern cvar_t *r_railSegmentLength;

extern cvar_t *r_ignore;                        ///< used for debugging anything

extern cvar_t *r_zNear;                         ///< near Z clip plane
extern cvar_t *r_zFar;                          ///< far Z clip plane

extern cvar_t *r_measureOverdraw;               ///< enables stencil buffer overdraw measurement

extern cvar_t *r_lodBias;                       ///< push/pull LOD transitions
extern cvar_t *r_lodScale;

extern cvar_t *r_fastSky;                       ///< controls whether sky should be cleared or drawn
extern cvar_t *r_drawSun;                       ///< controls drawing of sun quad
												///< "0" no sun
												///< "1" draw sun
												///< "2" also draw lens flare effect centered on sun
extern cvar_t *r_dynamicLight;                  ///< dynamic lights enabled/disabled

extern cvar_t *r_noreFresh;                     ///< bypasses the ref rendering
extern cvar_t *r_drawEntities;                  ///< disable/enable entity rendering
extern cvar_t *r_drawWorld;                     ///< disable/enable world rendering
extern cvar_t *r_drawFoliage;                   ///< disable/enable foliage rendering
extern cvar_t *r_speeds;                        ///< various levels of information display
extern cvar_t *r_detailTextures;                ///< enables/disables detail texturing stages
extern cvar_t *r_noVis;                         ///< disable/enable usage of PVS
extern cvar_t *r_noCull;
extern cvar_t *r_facePlaneCull;                 ///< enables culling of planar surfaces with back side test
extern cvar_t *r_noCurves;
extern cvar_t *r_showCluster;

extern cvar_t *r_noBorder;                      ///< FIXME: use in sdl only !
extern cvar_t *r_gamma;

extern cvar_t *r_allowExtensions;               ///< global enable/disable of OpenGL extensions
extern cvar_t *r_extCompressedTextures;         ///< these control use of specific extensions.  FIXME: not used in GLES ! move it ?
extern cvar_t *r_extMultitexture;               ///< FIXME: not used in GLES ! move it ?
extern cvar_t *r_extTextureEnvAdd;              ///< FIXME: not used in GLES ! move it ?

extern cvar_t *r_extTextureFilterAnisotropic;   ///< FIXME: not used in GLES ! move it ?

extern cvar_t *r_noBind;                        ///< turns off binding to appropriate textures
extern cvar_t *r_singleShader;                  ///< make most world faces use default shader
extern cvar_t *r_roundImagesDown;
extern cvar_t *r_colorMipLevels;                ///< development aid to see texture mip usage
extern cvar_t *r_picMip;                        ///< controls picmip values
extern cvar_t *r_finish;
extern cvar_t *r_drawBuffer;
extern cvar_t *r_textureMode;
extern cvar_t *r_offsetFactor;
extern cvar_t *r_offsetUnits;

extern cvar_t *r_uiFullScreen;                  ///< ui is running fullscreen

extern cvar_t *r_logFile;                       ///< number of frames to emit GL logs
extern cvar_t *r_showTris;                      ///< enables wireframe rendering of the world
extern cvar_t *r_showSky;                       ///< forces sky in front of all surfaces
extern cvar_t *r_clear;                         ///< force screen clear every frame

extern cvar_t *r_shadows;                       ///< controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern cvar_t *r_flares;                        ///< light flares

extern cvar_t *r_portalSky;
extern cvar_t *r_intensity;

extern cvar_t *r_lockPvs;
extern cvar_t *r_noportals;
extern cvar_t *r_portalOnly;

extern cvar_t *r_subDivisions;

extern cvar_t *r_skipBackEnd;

extern cvar_t *r_ignoreGLErrors;

extern cvar_t *r_overBrightBits;
extern cvar_t *r_mapOverBrightBits;

extern cvar_t *r_debugSurface;
extern cvar_t *r_simpleMipMaps;

extern cvar_t *r_ambientScale;
extern cvar_t *r_debugLight;
extern cvar_t *r_showImages;
extern cvar_t *r_debugSort;
extern cvar_t *r_printShaders;
//extern cvar_t *r_saveFontData;

//extern cvar_t *r_screenshotFormat;            ///< FIXME: both!
//extern cvar_t *r_screenshotJpegQuality;       ///< FIXME: both!

extern cvar_t *r_maxPolys;
extern cvar_t *r_maxPolyVerts;

/**
 * @enum renderSpeeds_t for r_speeds
 * @brief
 */
typedef enum
{
	RSPEEDS_GENERAL = 1,
	RSPEEDS_CULLING,
	RSPEEDS_VIEWCLUSTER,
	RSPEEDS_LIGHTS,
	RSPEEDS_FOG,
	RSPEEDS_FLARES,
	RSPEEDS_DECALS, // r1 end
	RSPEEDS_SHADOWCUBE_CULLING,
	RSPEEDS_OCCLUSION_QUERIES,
#if 0
	RSPEEDS_DEPTH_BOUNDS_TESTS,
#endif
	RSPEEDS_SHADING_TIMES,
	RSPEEDS_CHC,
	RSPEEDS_NEAR_FAR

} renderSpeeds_t;

/**
 * @struct imageExtToLoaderMap_s
 * @brief
 */
typedef struct
{
	char *ext;
	void(*ImageLoader)(const char *, unsigned char **, int *, int *, byte);
} imageExtToLoaderMap_t;

extern imageExtToLoaderMap_t imageLoaders[];
extern int numImageLoaders;
/*
=============================================================
IMAGE LOADERS
=============================================================
*/

void R_LoadBMP(const char *name, byte **pic, int *width, int *height, byte alphaByte);
void R_LoadJPG(const char *name, byte **pic, int *width, int *height, byte alphaByte);
void R_LoadPCX(const char *name, byte **pic, int *width, int *height, byte alphaByte);
void R_LoadPNG(const char *name, byte **pic, int *width, int *height, byte alphaByte);
void R_LoadTGA(const char *name, byte **pic, int *width, int *height, byte alphaByte);
void R_LoadSVG(const char *name, byte **pic, int *width, int *height, byte alphaByte);

#endif  // INCLUDE_TR_COMMON_H
