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

#ifndef INCLUDE_TR_COMMON_CVARS_H
#define INCLUDE_TR_COMMON_CVARS_H

#include "../qcommon/q_shared.h"

// cvars used by both renderers

extern cvar_t *r_flareSize;
extern cvar_t *r_flareFade;

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
extern cvar_t *r_debugShaderSurfaceFlags;
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

extern cvar_t *r_ext_multisample;

extern cvar_t *r_scalesvg;

#endif  // INCLUDE_TR_COMMON_CVARS_H
