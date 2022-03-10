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
 * @file tr_public.h
 */

#ifndef INCLUDE_TR_PUBLIC_H
#define INCLUDE_TR_PUBLIC_H

#include "tr_types.h"

#define REF_API_VERSION     10

#ifdef FEATURE_PNG
#include "zlib.h"
#endif

/**
 * @struct refexport_s
 * @brief these are the functions exported by the refresh module
 */
typedef struct
{
	/// called before the library is unloaded
	/// if the system is just reconfiguring, pass destroyWindow = qfalse,
	/// which will keep the screen from flashing to the desktop.
	void (*Shutdown)(qboolean destroyWindow);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.

	/// BeginRegistration makes any existing media pointers invalid
	/// and returns the current gl configuration, including screen width
	/// and height, which can be used by the client to intelligently
	///  size display elements
	void (*BeginRegistration)(glconfig_t *config);
	qhandle_t (*RegisterModel)(const char *name);
	qhandle_t (*RegisterModelAllLODs)(const char *name);
	qhandle_t (*RegisterSkin)(const char *name);
	qhandle_t (*RegisterShader)(const char *name);
	qhandle_t (*RegisterShaderNoMip)(const char *name);
	void (*RegisterFont)(const char *fontName, int pointSize, void *font, qboolean extended);

	void (*LoadWorld)(const char *name);
	qboolean (*GetSkinModel)(qhandle_t skinid, const char *type, char *name);
	qhandle_t (*GetShaderFromModel)(qhandle_t modelid, int surfnum, int withlightmap);

	/// the vis data is a large enough block of data that we go to the trouble
	/// of sharing it with the clipmodel subsystem
	void (*SetWorldVisData)(const byte *vis);

	/// EndRegistration will draw a tiny polygon with each texture, forcing
	/// them to be loaded into card memory
	void (*EndRegistration)(void);

	/// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	/// Nothing is drawn until R_RenderScene is called.
	void (*ClearScene)(void);
	void (*AddRefEntityToScene)(const refEntity_t *re);
	int (*LightForPoint)(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
	void (*AddPolyToScene)(qhandle_t hShader, int numVerts, const polyVert_t *verts);
	void (*AddPolysToScene)(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys);
	void (*AddLightToScene)(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags);
	void (*AddCoronaToScene)(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible);
	void (*SetFog)(int fogvar, int var1, int var2, float r, float g, float b, float density);
	void (*RenderScene)(const refdef_t *fd);

	void (*SetColor)(const float *rgba);        /// NULL = 1,1,1,1
	void (*DrawStretchPic)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);          /// 0 = white
	void (*DrawRotatedPic)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle);         /// NERVE - SMF
	void (*DrawStretchPicGradient)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType);
	void (*Add2dPolys)(polyVert_t *polys, int numverts, qhandle_t hShader);

	/// Draw images for cinematic rendering, pass as 32 bit rgba
	void (*DrawStretchRaw)(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
	void (*UploadCinematic)(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

	void (*BeginFrame)(void);

	/// if the pointers are not NULL, timing info will be returned
	void (*EndFrame)(int *frontEndMsec, int *backEndMsec);


	int (*MarkFragments)(int numPoints, const vec3_t *points, const vec3_t projection,
	                     int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer);

	void (*ProjectDecal)(qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime);
	void (*ClearDecals)(void);

	int (*LerpTag)(orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex);
	void (*ModelBounds)(qhandle_t model, vec3_t mins, vec3_t maxs);

	void (*RemapShader)(const char *oldShader, const char *newShader, const char *offsetTime);

	void (*DrawDebugPolygon)(int color, int numpoints, float *points);

	void (*DrawDebugText)(const vec3_t org, float r, float g, float b, const char *text, qboolean neverOcclude);

	qboolean (*GetEntityToken)(char *buffer, size_t size);

	void (*AddPolyBufferToScene)(polyBuffer_t *pPolyBuffer);

	void (*SetGlobalFog)(qboolean restore, int duration, float r, float g, float b, float depthForOpaque);

	qboolean (*inPVS)(const vec3_t p1, const vec3_t p2);

	void (*purgeCache)(void);

	qboolean (*LoadDynamicShader)(const char *shadername, const char *shadertext);

	void (*RenderToTexture)(int textureid, int x, int y, int w, int h);

	int (*GetTextureId)(const char *imagename);
	void (*Finish)(void);

	/// avi output stuff
	void (*TakeVideoFrame)(int h, int w, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg);

	void (*InitOpenGL)(void);
	int (*InitOpenGLSubSystem)(void);

#if defined(USE_REFLIGHT)
	void (*AddRefLightToScene)(const refLight_t *light);
	qhandle_t (*RegisterShaderLightAttenuation)(const char *name);
#endif

	/// alternative skeletal animation system
#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	qhandle_t (*RegisterAnimation)(const char *name);
	int (*CheckSkeleton)(refSkeleton_t *skel, qhandle_t model, qhandle_t anim);
	int (*BuildSkeleton)(refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac,
	                     qboolean clearOrigin);
	int (*BlendSkeleton)(refSkeleton_t *skel, const refSkeleton_t *blend, float frac);
	int (*BoneIndex)(qhandle_t hModel, const char *boneName);
	int (*AnimNumFrames)(qhandle_t hAnim);
	int (*AnimFrameRate)(qhandle_t hAnim);
#endif

} refexport_t;

/**
 * @struct refimport_s
 * @brief These are the functions imported by the refresh module
 */
typedef struct
{
	/// print message on the local console
	void(QDECL * Printf)(int printLevel, const char *fmt, ...) _attribute ((format(printf, 2, 3)));

	/// abort the game
	void(QDECL * Error)(int errorLevel, const char *fmt, ...) _attribute ((noreturn, format(printf, 2, 3)));

	/// milliseconds should only be used for profiling, never
	/// for anything game related. Get time from the refdef
	int (*Milliseconds)(void);
	int (*RealTime)(qtime_t *qtime);

	/// stack based memory allocation for per-level things that
	/// won't be freed
	void (*Hunk_Clear)(void);
#ifdef HUNK_DEBUG
	void * (*Hunk_AllocDebug)(size_t size, ha_pref pref, char *label, char *file, int line);
#else
	void * (*Hunk_Alloc)(size_t size, ha_pref pref);
#endif
	void * (*Hunk_AllocateTempMemory)(size_t size);
	void (*Hunk_FreeTempMemory)(void *block);

	/// dynamic memory allocator for things that need to be freed
#ifdef ZONE_DEBUG
	void * (*Z_MallocDebug)(int bytes, char *label, char *file, int line);
#else
	void * (*Z_Malloc)(int bytes);
#endif
	void (*Free)(void *buf);
	void (*Tag_Free)(void);

	cvar_t * (*Cvar_Get)(const char *name, const char *value, int flags);
	void (*Cvar_Set)(const char *name, const char *value);
	void (*Cvar_CheckRange)(cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral);
	void (*Cvar_SetDescription)(cvar_t *cv, const char *description);
	int (*Cvar_VariableIntegerValue)(const char *var_name);

	void (*Cmd_AddSystemCommand)(const char *name, void (*cmd)(void), const char *description, void (*complete)(char *args, int argNum));
	void (*Cmd_RemoveSystemCommand)(const char *name);

	int (*Cmd_Argc)(void);
	char * (*Cmd_Argv)(int i);

	void (*Cmd_ExecuteText)(int exec_when, const char *text);

	/// visualization for debugging collision detection
	int (*CM_PointContents)(const vec3_t p, clipHandle_t model);
	void (*CM_DrawDebugSurface)(void (*drawPoly)(int color, int numPoints, float *points));

	/// a -1 return means the file does not exist
	/// NULL can be passed for buf to just determine existance
	int (*FS_FileIsInPAK)(const char *name, int *pChecksum);
	int (*FS_ReadFile)(const char *name, void **buf);
	void (*FS_FreeFile)(void *buf);
	char ** (*FS_ListFiles)(const char *name, const char *extension, int *numfilesfound);
	void (*FS_FreeFileList)(char **filelist);
	void (*FS_WriteFile)(const char *qpath, const void *buffer, int size);
	qboolean (*FS_FileExists)(const char *file);

	long (*FS_FOpenFileRead)(const char *filename, fileHandle_t *file, qboolean uniqueFILE);
	int (*FS_Read)(void *buffer, int len, fileHandle_t f);

	/// cinematic stuff
	void (*CIN_UploadCinematic)(int handle);
	int (*CIN_PlayCinematic)(const char *arg0, int xpos, int ypos, int width, int height, int bits);
	e_status (*CIN_RunCinematic)(int handle);

	/// avi output stuff
	qboolean (*CL_VideoRecording)(void);
	void (*CL_WriteAVIVideoFrame)(const byte *buffer, int size);

#ifdef FEATURE_PNG
	int (*zlib_compress) (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
	uLong (*zlib_crc32) (uLong crc, const Bytef *buf, uInt len);
#endif

	void (*Sys_GLimpSafeInit)(void);
	void (*Sys_GLimpInit)(void);
	void (*Sys_SetEnv)(const char *name, const char *value);

	/// input event handling
	void (*IN_Init)(void);
	void (*IN_Shutdown)(void);
	void (*IN_Restart)(void);

	void (*GLimp_Init)(glconfig_t *glConfig, windowContext_t *context);
	void (*GLimp_Shutdown)(void);
	void (*GLimp_SwapFrame)(void);
	void (*GLimp_SetGamma)(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);
} refimport_t;

/// this is the only function actually exported at the linker level
/// If the module can't init to a valid rendering state, NULL will be
/// returned.
#ifdef USE_RENDERER_DLOPEN
typedef refexport_t * (QDECL * GetRefAPI_t)(int apiVersion, refimport_t *rimp);
#else
//refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp);
#endif

#endif  // INCLUDE_TR_PUBLIC_H
