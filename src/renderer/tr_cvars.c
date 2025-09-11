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

#include "tr_local.h"

#ifdef USE_RENDERER_DLOPEN
#if idppc
cvar_t *com_altivec;
#endif
#endif

cvar_t *r_flareSize;
cvar_t *r_flareFade;

cvar_t *r_railWidth;
cvar_t *r_railSegmentLength;

cvar_t *r_ignoreFastPath;

cvar_t *r_ignore;

cvar_t *r_detailTextures;

cvar_t *r_zNear;
cvar_t *r_zFar;

cvar_t *r_skipBackEnd;

cvar_t *r_greyScale;

cvar_t *r_measureOverdraw;

cvar_t *r_fastSky;
cvar_t *r_drawSun;
cvar_t *r_dynamicLight;

cvar_t *r_lodBias;
cvar_t *r_lodScale;

cvar_t *r_noreFresh;
cvar_t *r_drawEntities;
cvar_t *r_drawWorld;
cvar_t *r_drawFoliage;
cvar_t *r_speeds;

cvar_t *r_noVis;
cvar_t *r_noCull;
cvar_t *r_facePlaneCull;
cvar_t *r_showCluster;
cvar_t *r_noCurves;

cvar_t *r_allowExtensions;

cvar_t *r_extCompressedTextures;
cvar_t *r_extMultitexture;
cvar_t *r_extTextureEnvAdd;

cvar_t *r_extTextureFilterAnisotropic;
cvar_t *r_extMaxAnisotropy;

cvar_t *r_ignoreGLErrors;
cvar_t *r_logFile;

cvar_t *r_textureBits;

cvar_t *r_drawBuffer;
cvar_t *r_lightMap;
cvar_t *r_uiFullScreen;
cvar_t *r_shadows;
cvar_t *r_portalSky;
cvar_t *r_flares;
cvar_t *r_noBind;
cvar_t *r_singleShader;
cvar_t *r_roundImagesDown;
cvar_t *r_colorMipLevels;
cvar_t *r_picMip;
cvar_t *r_showTris;
cvar_t *r_trisColor;
cvar_t *r_showSky;
cvar_t *r_showNormals;
cvar_t *r_normalLength;
//cvar_t *r_showmodelbounds; // see RB_MDM_SurfaceAnim()
cvar_t *r_finish;
cvar_t *r_clear;
cvar_t *r_textureMode;
cvar_t *r_offsetFactor;
cvar_t *r_offsetUnits;
cvar_t *r_gamma;
cvar_t *r_intensity;
cvar_t *r_lockPvs;
cvar_t *r_noportals;
cvar_t *r_portalOnly;

cvar_t *r_subDivisions;
cvar_t *r_lodCurveError;

cvar_t *r_overBrightBits;
cvar_t *r_mapOverBrightBits;

cvar_t *r_debugSurface;
cvar_t *r_debugShaderSurfaceFlags;
cvar_t *r_simpleMipMaps;

cvar_t *r_showImages;

cvar_t *r_ambientScale;
cvar_t *r_directedScale;
cvar_t *r_debugLight;
cvar_t *r_debugSort;
cvar_t *r_printShaders;
//cvar_t *r_saveFontData;

cvar_t *r_cache;
cvar_t *r_cacheShaders;
cvar_t *r_cacheModels;

cvar_t *r_cacheGathering;

cvar_t *r_bonesDebug;

cvar_t *r_fbo;

cvar_t *r_wolfFog;

cvar_t *r_screenshotFormat;
cvar_t *r_screenshotJpegQuality;

cvar_t *r_maxPolys;
cvar_t *r_maxPolyVerts;

cvar_t *r_gfxInfo;

cvar_t *r_scale;

cvar_t *r_scalesvg;

/**
 * @brief R_Register
 */
void R_Register(void)
{
#ifdef USE_RENDERER_DLOPEN
#if idppc
	com_altivec = ri.Cvar_Get("com_altivec", "1", CVAR_ARCHIVE);
#endif
#endif

	// latched and archived variables
	r_allowExtensions       = ri.Cvar_Get("r_allowExtensions", "1", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	r_extCompressedTextures = ri.Cvar_Get("r_ext_compressed_textures", "1", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	r_extMultitexture       = ri.Cvar_Get("r_ext_multitexture", "1", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	r_extTextureEnvAdd      = ri.Cvar_Get("r_ext_texture_env_add", "1", CVAR_ARCHIVE_ND | CVAR_LATCH);

	r_extTextureFilterAnisotropic = ri.Cvar_Get("r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	r_extMaxAnisotropy            = ri.Cvar_Get("r_ext_max_anisotropy", "2", CVAR_ARCHIVE_ND | CVAR_LATCH);

	r_picMip = ri.Cvar_Get("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH);          // mod for DM and DK for id build.  was "1" - pushed back to 1
	ri.Cvar_CheckRange(r_picMip, 0, 3, qtrue);
	r_roundImagesDown = ri.Cvar_Get("r_roundImagesDown", "1", CVAR_ARCHIVE_ND | CVAR_LATCH);

	r_colorMipLevels = ri.Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH);
	r_detailTextures = ri.Cvar_Get("r_detailtextures", "1", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_textureBits    = ri.Cvar_Get("r_texturebits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);

	r_overBrightBits = ri.Cvar_Get("r_overBrightBits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);        // disable overbrightbits by default
	ri.Cvar_CheckRange(r_overBrightBits, 0, 1, qtrue);                                    // limit to overbrightbits 1 (sorry 1337 players)
	r_simpleMipMaps = ri.Cvar_Get("r_simpleMipMaps", "1", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_uiFullScreen  = ri.Cvar_Get("r_uifullscreen", "0", 0);

	r_subDivisions = ri.Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE_ND | CVAR_LATCH);

	r_ignoreFastPath = ri.Cvar_Get("r_ignoreFastPath", "0", CVAR_ARCHIVE | CVAR_LATCH);    // use fast path by default
	r_greyScale      = ri.Cvar_Get("r_greyscale", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);

	// temporary latched variables that can only change over a restart
	r_mapOverBrightBits = ri.Cvar_Get("r_mapOverBrightBits", "2", CVAR_ARCHIVE_ND | CVAR_LATCH);
	ri.Cvar_CheckRange(r_mapOverBrightBits, 0, 3, qtrue);
	r_intensity = ri.Cvar_Get("r_intensity", "1", CVAR_LATCH);
	ri.Cvar_CheckRange(r_intensity, 0, 1.5, qfalse);
	r_singleShader = ri.Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH);

	// archived variables that can change at any time
	r_lodCurveError = ri.Cvar_Get("r_lodCurveError", "250", CVAR_ARCHIVE_ND);
	r_lodBias       = ri.Cvar_Get("r_lodbias", "0", CVAR_ARCHIVE_ND);
	r_flares        = ri.Cvar_Get("r_flares", "1", CVAR_ARCHIVE);
	r_zNear         = ri.Cvar_Get("r_znear", "3", CVAR_CHEAT); // changed it to 3 (from 4) because of lean/fov cheats
	ri.Cvar_CheckRange(r_zNear, 0.001f, 200, qfalse);
	r_zFar = ri.Cvar_Get("r_zfar", "0", CVAR_CHEAT);

	r_ignoreGLErrors = ri.Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE_ND);
	r_fastSky        = ri.Cvar_Get("r_fastsky", "0", CVAR_ARCHIVE_ND);

	r_drawSun      = ri.Cvar_Get("r_drawSun", "1", CVAR_ARCHIVE_ND);
	r_dynamicLight = ri.Cvar_Get("r_dynamiclight", "1", CVAR_ARCHIVE);
	r_finish       = ri.Cvar_Get("r_finish", "0", CVAR_ARCHIVE_ND);
	r_textureMode  = ri.Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	r_gamma        = ri.Cvar_Get("r_gamma", "1.3", CVAR_ARCHIVE_ND);

	r_facePlaneCull = ri.Cvar_Get("r_facePlaneCull", "1", CVAR_ARCHIVE_ND);

	r_railWidth         = ri.Cvar_Get("r_railWidth", "16", CVAR_ARCHIVE_ND);
	r_railSegmentLength = ri.Cvar_Get("r_railSegmentLength", "32", CVAR_ARCHIVE_ND);

	r_ambientScale  = ri.Cvar_Get("r_ambientScale", "0.5", CVAR_CHEAT);
	r_directedScale = ri.Cvar_Get("r_directedScale", "1", CVAR_CHEAT);

	// temporary variables that can change at any time
	r_showImages = ri.Cvar_Get("r_showImages", "0", CVAR_TEMP);

	r_debugLight   = ri.Cvar_Get("r_debuglight", "0", CVAR_TEMP);
	r_debugSort    = ri.Cvar_Get("r_debugSort", "0", CVAR_CHEAT);
	r_printShaders = ri.Cvar_Get("r_printShaders", "0", 0);
	//r_saveFontData = ri.Cvar_Get("r_saveFontData", "0", 0); // used to generate texture font file

	r_cache        = ri.Cvar_Get("r_cache", "1", CVAR_LATCH); // leaving it as this for backwards compability. but it caches models and shaders also
	r_cacheShaders = ri.Cvar_Get("r_cacheShaders", "1", CVAR_LATCH);

	r_cacheModels    = ri.Cvar_Get("r_cacheModels", "1", CVAR_LATCH);
	r_cacheGathering = ri.Cvar_Get("cl_cacheGathering", "0", 0);
	r_bonesDebug     = ri.Cvar_Get("r_bonesDebug", "0", CVAR_CHEAT);

	r_fbo = ri.Cvar_Get("r_fbo", "1", CVAR_LATCH);

	r_wolfFog = ri.Cvar_Get("r_wolffog", "1", CVAR_ARCHIVE);

	r_noCurves    = ri.Cvar_Get("r_nocurves", "0", CVAR_CHEAT);
	r_drawWorld   = ri.Cvar_Get("r_drawworld", "1", CVAR_CHEAT);
	r_drawFoliage = ri.Cvar_Get("r_drawfoliage", "1", CVAR_CHEAT);
	r_lightMap    = ri.Cvar_Get("r_lightmap", "0", CVAR_CHEAT);
	r_portalOnly  = ri.Cvar_Get("r_portalOnly", "0", CVAR_CHEAT);

	r_flareSize = ri.Cvar_Get("r_flareSize", "40", CVAR_CHEAT);
	ri.Cvar_Set("r_flareFade", "5");    // to force this when people already have "7" in their config
	r_flareFade = ri.Cvar_Get("r_flareFade", "5", CVAR_CHEAT);

	r_skipBackEnd = ri.Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = ri.Cvar_Get("r_measureOverdraw", "0", CVAR_CHEAT);
	r_lodScale        = ri.Cvar_Get("r_lodscale", "5", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_noreFresh       = ri.Cvar_Get("r_norefresh", "0", CVAR_CHEAT);
	r_drawEntities    = ri.Cvar_Get("r_drawentities", "1", CVAR_CHEAT);
	r_ignore          = ri.Cvar_Get("r_ignore", "1", CVAR_CHEAT);
	r_noCull          = ri.Cvar_Get("r_nocull", "0", CVAR_CHEAT);
	r_noVis           = ri.Cvar_Get("r_novis", "0", CVAR_CHEAT);
	r_showCluster     = ri.Cvar_Get("r_showcluster", "0", CVAR_CHEAT);
	r_speeds          = ri.Cvar_Get("r_speeds", "0", CVAR_CHEAT);

	r_logFile                 = ri.Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_debugSurface            = ri.Cvar_Get("r_debugSurface", "0", CVAR_CHEAT);
	r_debugShaderSurfaceFlags = ri.Cvar_Get("r_debugShaderSurfaceFlags", "0", CVAR_CHEAT);
	ri.Cvar_SetDescription(r_debugShaderSurfaceFlags,
	                       "Highlights any shader with passed SURF_* bitflag set.\n"
	                       "\n"
	                       "e.g. for highlighting SURF_LANDMINE shaders:\n"
	                       "    /r_debugShaderSurfaceFlags 2147483648\n"
	                       "\n"
	                       "SURF_* as decimals for reference follow:\n"
	                       "\n"
	                       "SURF_NODAMAGE           1\n"
	                       "SURF_SLICK              2\n"
	                       "SURF_SKY                4\n"
	                       "SURF_LADDER             8\n"
	                       "SURF_NOIMPACT           16\n"
	                       "SURF_NOMARKS            32\n"
	                       "SURF_SPLASH             64\n"
	                       "SURF_NODRAW             128\n"
	                       "SURF_HINT               256\n"
	                       "SURF_SKIP               512\n"
	                       "SURF_NOLIGHTMAP         1024\n"
	                       "SURF_POINTLIGHT         2048\n"
	                       "SURF_METAL              4096\n"
	                       "SURF_NOSTEPS            8192\n"
	                       "SURF_NONSOLID           16384\n"
	                       "SURF_LIGHTFILTER        32768\n"
	                       "SURF_ALPHASHADOW        65536\n"
	                       "SURF_NODLIGHT           131072\n"
	                       "SURF_WOOD               262144\n"
	                       "SURF_GRASS              524288\n"
	                       "SURF_GRAVEL             1048576\n"
	                       "SURF_GLASS              2097152\n"
	                       "SURF_SNOW               4194304\n"
	                       "SURF_ROOF               8388608\n"
	                       "SURF_RUBBLE             16777216\n"
	                       "SURF_CARPET             33554432\n"
	                       "SURF_MONSTERSLICK       67108864\n"
	                       "SURF_MONSLICK_W         134217728\n"
	                       "SURF_MONSLICK_N         268435456\n"
	                       "SURF_MONSLICK_E         536870912\n"
	                       "SURF_MONSLICK_S         1073741824\n"
	                       "SURF_LANDMINE           2147483648\n"
	                       );
	r_noBind       = ri.Cvar_Get("r_nobind", "0", CVAR_CHEAT);
	r_showTris     = ri.Cvar_Get("r_showtris", "0", CVAR_CHEAT);
	r_trisColor    = ri.Cvar_Get("r_trisColor", "1.0 1.0 1.0 1.0", CVAR_ARCHIVE_ND);
	r_showSky      = ri.Cvar_Get("r_showsky", "0", CVAR_CHEAT);
	r_showNormals  = ri.Cvar_Get("r_shownormals", "0", CVAR_CHEAT);
	r_normalLength = ri.Cvar_Get("r_normallength", "0.5", CVAR_ARCHIVE_ND);
	//r_showmodelbounds = ri.Cvar_Get("r_showmodelbounds", "0", CVAR_CHEAT); // see RB_MDM_SurfaceAnim()
	r_clear        = ri.Cvar_Get("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = ri.Cvar_Get("r_offsetfactor", "-1", CVAR_CHEAT);
	r_offsetUnits  = ri.Cvar_Get("r_offsetunits", "-2", CVAR_CHEAT);
	r_drawBuffer   = ri.Cvar_Get("r_drawBuffer", "GL_BACK", CVAR_CHEAT);
	r_lockPvs      = ri.Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals    = ri.Cvar_Get("r_noportals", "0", CVAR_CHEAT);
	r_shadows      = ri.Cvar_Get("cg_shadows", "0", 0);

	r_screenshotFormat      = ri.Cvar_Get("r_screenshotFormat", "2", CVAR_ARCHIVE_ND);
	r_screenshotJpegQuality = ri.Cvar_Get("r_screenshotJpegQuality", "90", CVAR_ARCHIVE_ND);

	r_portalSky = ri.Cvar_Get("cg_skybox", "1", 0);

	// note: MAX_POLYS and MAX_POLYVERTS are heavily increased in ET compared to q3
	//       - but run 20 bots on oasis and you'll see limits reached (developer 1)
	//       - modern computers can deal with more than our old default values -> users can increase this now to MAX_POLYS/MAX_POLYVERTS
	r_maxPolys = ri.Cvar_Get("r_maxpolys", va("%d", DEFAULT_POLYS), CVAR_LATCH);             // now latched to check against used r_maxpolys and not MAX_POLYS
	ri.Cvar_CheckRange(r_maxPolys, MIN_POLYS, MAX_POLYS, qtrue);                        // MIN_POLYS was old static value
	r_maxPolyVerts = ri.Cvar_Get("r_maxpolyverts", va("%d", DEFAULT_POLYVERTS), CVAR_LATCH); // now latched to check against used r_maxpolyverts and not MAX_POLYVERTS
	ri.Cvar_CheckRange(r_maxPolyVerts, MIN_POLYVERTS, MAX_POLYVERTS, qtrue);            // MIN_POLYVERTS was old static value

	r_gfxInfo = ri.Cvar_Get("r_gfxinfo", "0", 0); // less spammy gfx output at start - enable to print full GL_EXTENSION string

	r_scale = ri.Cvar_Get("r_scale", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_scalesvg = ri.Cvar_Get("r_scalesvg", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_CheckRange(r_scalesvg, 0, 2, qtrue);

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddSystemCommand("imagelist", R_ImageList_f, "Print out the list of images loaded", NULL);
	ri.Cmd_AddSystemCommand("shaderlist", R_ShaderList_f, "Print out the list of shaders loaded", NULL);
	ri.Cmd_AddSystemCommand("skinlist", R_SkinList_f, "Print out the list of skins", NULL);
	ri.Cmd_AddSystemCommand("modellist", R_Modellist_f, "Print out the list of loaded models", NULL);
	ri.Cmd_AddSystemCommand("screenshot", R_ScreenShot_f, "Take a screenshot of current frame", NULL);
	ri.Cmd_AddSystemCommand("screenshotJPEG", R_ScreenShot_f, "Take a JPEG screenshot of current frame", NULL);
	ri.Cmd_AddSystemCommand("gfxinfo", GfxInfo_f, "Print GFX info of current system", NULL);
	ri.Cmd_AddSystemCommand("taginfo", R_TagInfo_f, "Print the list of loaded tags", NULL);

	R_RegisterCommon();
}
