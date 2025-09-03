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

#ifndef INCLUDE_TR_CVARS_H
#define INCLUDE_TR_CVARS_H

#include "../qcommon/q_shared.h"

// cvars

extern cvar_t *r_ignoreFastPath;        ///< allows us to ignore our Tess fast paths

extern cvar_t *r_textureBits;           ///< number of desired texture bits
                                        ///< 0 = use framebuffer depth
                                        ///< 16 = use 16-bit textures
                                        ///< 32 = use 32-bit textures
                                        ///< all else = error

extern cvar_t *r_extMaxAnisotropy;      ///< FIXME: not used in GLES ! move it ?
                                        ///< FIXME: "extern int      maxAnisotropy" founded

extern cvar_t *r_lightMap;              ///< render lightmaps only

extern cvar_t *r_trisColor;             ///< enables modifying of the wireframe colour (in 0xRRGGBB[AA] format, alpha defaults to FF)
extern cvar_t *r_showNormals;           ///< draws wireframe normals
extern cvar_t *r_normalLength;          ///< length of the normals
//extern cvar_t *r_showmodelbounds;		///< see RB_MDM_SurfaceAnim()

extern cvar_t *r_lodCurveError;

extern cvar_t *r_greyScale;

extern cvar_t *r_directedScale;

extern cvar_t *r_cache;
extern cvar_t *r_cacheShaders;
extern cvar_t *r_cacheModels;

extern cvar_t *r_fbo;

extern cvar_t *r_cacheGathering;

extern cvar_t *r_bonesDebug;

extern cvar_t *r_wolfFog;

extern cvar_t *r_gfxInfo;

extern cvar_t *r_scale;

extern cvar_t *r_screenshotFormat;
extern cvar_t *r_screenshotJpegQuality;

#endif  // #ifndef INCLUDE_TR_CVARS_H
