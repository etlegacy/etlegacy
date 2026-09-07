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
 * @file renderer_vk/tr_init.c
 * @brief Functions that are not called every frame: subsystem init/shutdown,
 * REF API export, and an initial (non-capturing) screenshot/video-frame
 * command implementation. Modeled on renderer1's tr_init.c, but with all
 * genuinely OpenGL-specific pieces (context creation, default GL state,
 * pixel readback) replaced or stubbed for the Vulkan backend.
 */

#include "tr_local.h"

glconfig_t glConfig;
qboolean   textureFilterAnisotropic = qfalse;
float      maxAnisotropy            = 2.f;

glstate_t glState;

/**
 * @brief InitVulkan
 *
 * Vulkan-flavored replacement for renderer1's InitOpenGL(): fills in the
 * glConfigString the engine passes down to GLimp_Init, then lets the usual
 * GLimp_Init()/GLimp_SetMode() callback sequence drive window creation and,
 * via RE_InitOpenGlSubsystems()/RE_InitOpenGl() (see tr_common_vulkan.c),
 * Vulkan instance/device/swapchain creation.
 */
static void InitVulkan(void)
{
	if (glConfig.vidWidth == 0)
	{
		char glConfigString[1024] = { 0 };

		Com_Memset(&glConfig, 0, sizeof(glConfig));

		Info_SetValueForKey(glConfigString, "type", "vulkan");
		Info_SetValueForKey(glConfigString, "major", "1");
		Info_SetValueForKey(glConfigString, "minor", "0");
		Info_SetValueForKey(glConfigString, "samples", "0");

		ri.GLimp_Init(&glConfig, glConfigString);

		if (r_scale->value)
		{
			const float scale = Com_Clamp(0.2f, 4.f, r_scale->value);
			glConfig.vidWidth  *= scale;
			glConfig.vidHeight *= scale;
		}
	}

	// print info
	GfxInfo_f();
}

/**
 * @brief GL_CheckErrors
 *
 * No-op for Vulkan: there is no implicit GL error state to poll. Kept so
 * callers copied verbatim from renderer1 (tr_backend.c et al.) still link.
 */
void GL_CheckErrors(void)
{
}

/**
 * @brief RB_TakeScreenshotCmd
 *
 * Pixel readback from the swapchain isn't implemented yet, so
 * this only advances the command buffer past the command's payload.
 * @param[in] data
 * @return
 */
const void *RB_TakeScreenshotCmd(const void *data)
{
	const screenshotCommand_t *cmd = ( const screenshotCommand_t * ) data;

	Ren_Print("RB_TakeScreenshotCmd: screenshots are not yet supported by the Vulkan renderer\n");

	return ( const void * ) (cmd + 1);
}

/**
 * @brief R_TakeScreenshot
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] name
 * @param[in] format
 */
void R_TakeScreenshot(int x, int y, int width, int height, const char *name, ssFormat_t format)
{
	static char         fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t *cmd;

	cmd = R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x      = x;
	cmd->y      = y;
	cmd->width  = width;
	cmd->height = height;
	Q_strncpyz(fileName, name, sizeof(fileName));
	cmd->fileName = fileName;
	cmd->format   = format;
}

/**
 * @brief R_ScreenshotFilename
 * @param[in] lastNumber
 * @param[out] fileName
 */
void R_ScreenshotFilename(int lastNumber, char *fileName, char *ext)
{
	int a, b, c, d;

	if (lastNumber < 0 || lastNumber > 9999)
	{
		Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.%s", ext);
		return;
	}

	a           = lastNumber / 1000;
	lastNumber -= a * 1000;
	b           = lastNumber / 100;
	lastNumber -= b * 100;
	c           = lastNumber / 10;
	lastNumber -= c * 10;
	d           = lastNumber;

	Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.%s"
	            , a, b, c, d, ext);
}

/**
 * @brief RB_TakeVideoFrameCmd
 *
 * Swapchain readback isn't implemented yet, so this only
 * advances the command buffer past the command's payload.
 * @param[in] data
 * @return
 */
const void *RB_TakeVideoFrameCmd(const void *data)
{
	const videoFrameCommand_t *cmd = (const videoFrameCommand_t *)data;

	Ren_Print("RB_TakeVideoFrameCmd: video capture is not yet supported by the Vulkan renderer\n");

	return (const void *)(cmd + 1);
}

/**
 * @brief R_ScreenShot_f
 *
 * @note Doesn't print the pacifier message if there is a second arg
 *
 * screenshot
 * screenshot [silent]
 * screenshot [filename]
 *
 * @note "screenshot levelshot" and actual pixel capture are not yet
 * implemented for the Vulkan renderer (see RB_TakeScreenshotCmd).
 */
void R_ScreenShot_f(void)
{
	char       checkname[MAX_OSPATH];
	static int lastNumber = -1;
	qboolean   silent;
	char       *ext = "";

	ssFormat_t format = r_screenshotFormat->integer;

	// Backwards compatibility
	if (!Q_stricmp(ri.Cmd_Argv(0), "screenshotJPEG"))
	{
		format = SSF_JPEG;
	}

	switch (format)
	{
	case SSF_TGA:
		ext = "tga";
		break;
	case SSF_JPEG:
		ext = "jpg";
		break;
#ifdef FEATURE_PNG
	case SSF_PNG:
		ext = "png";
		break;
#endif
	default:
		return;
	}

	if (!strcmp(ri.Cmd_Argv(1), "silent"))
	{
		silent = qtrue;
	}
	else
	{
		silent = qfalse;
	}

	if (ri.Cmd_Argc() == 2 && !silent)
	{
		// explicit filename
		const char *fileExt = COM_GetExtension(ri.Cmd_Argv(1));
		if (fileExt[0])
		{
			char filename[MAX_QPATH];
			COM_StripExtension(ri.Cmd_Argv(1), filename, MAX_QPATH);

			if (COM_CompareExtension(fileExt, "tga"))
			{
				ext    = "tga";
				format = SSF_TGA;
			}
			else if (COM_CompareExtension(fileExt, "jpg") || COM_CompareExtension(fileExt, "jpeg"))
			{
				ext    = "jpg";
				format = SSF_JPEG;
			}
#ifdef FEATURE_PNG
			else if (COM_CompareExtension(fileExt, "png"))
			{
				ext    = "png";
				format = SSF_PNG;
			}
#endif

			Com_sprintf(checkname, MAX_OSPATH, "screenshots/%s.%s", filename, ext);
		}
		else
		{
			Com_sprintf(checkname, MAX_OSPATH, "screenshots/%s.%s", ri.Cmd_Argv(1), ext);
		}
	}
	else
	{
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if (lastNumber == -1)
		{
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 99999 ; lastNumber++)
		{
			R_ScreenshotFilename(lastNumber, checkname, ext);

			if (!ri.FS_FileExists(checkname))
			{
				break; // file doesn't exist
			}
		}

		if (lastNumber >= 99999)
		{
			Ren_Print("ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot(0, 0, glConfig.windowWidth, glConfig.windowHeight, checkname, format);

	if (!silent)
	{
		Ren_Print("Wrote %s\n", checkname);
	}
}

//============================================================================

/**
 * @brief GfxInfo_f
 */
void GfxInfo_f(void)
{
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	Ren_Print("VULKAN_VENDOR: %s\n", glConfig.vendor_string);
	Ren_Print("VULKAN_DEVICE: %s\n", glConfig.renderer_string);
	Ren_Print("VULKAN_API_VERSION: %s\n", glConfig.version_string);

	Ren_Print("MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);
	Ren_Print("PIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
	Ren_Print("MODE: %d, SCREEN: %d x %d %s (ratio %.4f) Hz:", ri.Cvar_VariableIntegerValue("r_mode"), glConfig.vidWidth, glConfig.vidHeight, fsstrings[ri.Cvar_VariableIntegerValue("r_fullscreen") == 1], glConfig.windowAspect);

	if (glConfig.displayFrequency)
	{
		Ren_Print("%d\n", glConfig.displayFrequency);
	}
	else
	{
		Ren_Print("N/A\n");
	}

	// same lines as renderer1's GfxInfo_f
	if (glConfig.deviceSupportsGamma)
	{
		Ren_Print("GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits);
	}
	else
	{
		Ren_Print("GAMMA: software w/ %d overbright bits\n", tr.overbrightBits);
	}

	Ren_Print("texturemode: %s\n", r_textureMode->string);
	Ren_Print("picmip: %d\n", r_picMip->integer);
	Ren_Print("anisotropy: %.0f (max %.0f)\n", textureFilterAnisotropic ? r_extTextureFilterAnisotropic->value : 0.0f, maxAnisotropy);
}

/**
 * @brief R_Init
 */
void R_Init(void)
{
	int  i;
	byte *ptr;

	Ren_Print("----- Initializing Renderer (Vulkan) ----\n");

	// clear all our internal state
	Com_Memset(&tr, 0, sizeof(tr));
	Com_Memset(&backEnd, 0, sizeof(backEnd));
	Com_Memset(&tess, 0, sizeof(tess));

	if ((intptr_t) tess.xyz & 15)
	{
		Ren_Warning("tess.xyz not 16 byte aligned\n");
	}
	Com_Memset(tess.constantColor255, 255, sizeof(tess.constantColor255));

	// init function tables
	for (i = 0; i < FUNCTABLE_SIZE; i++)
	{
		tr.sinTable[i]             = sin(DEG2RAD(i * 360.0f / (( float ) (FUNCTABLE_SIZE - 1))));
		tr.squareTable[i]          = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
		tr.sawToothTable[i]        = ( float ) i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if (i < FUNCTABLE_SIZE / 2)
		{
			if (i < FUNCTABLE_SIZE / 4)
			{
				tr.triangleTable[i] = ( float ) i / (FUNCTABLE_SIZE / 4);
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}

	// init the virtual memory
	R_Hunk_Begin();

	R_NoiseInit();

	R_Register();

	ptr = ri.Hunk_Alloc(sizeof(*backEndData) + sizeof(srfPoly_t) * r_maxPolys->integer + sizeof(polyVert_t) * r_maxPolyVerts->integer, h_low);

	backEndData            = (backEndData_t *) ptr;
	backEndData->polys     = (srfPoly_t *) ((char *) ptr + sizeof(*backEndData));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof(*backEndData) + sizeof(srfPoly_t) * r_maxPolys->integer);

	R_InitNextFrame();

	InitVulkan();

	R_InitShaderPrograms();

	R_InitFBO();

	R_InitGamma();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

	R_InitSplash();

	Ren_Print("--------------------------------\n");
}

/**
 * @brief R_PurgeCache
 */
void R_PurgeCache(void)
{
	R_PurgeDynamicShaders();
	R_PurgeShaders(9999999);
	R_PurgeBackupImages(9999999);
	R_PurgeModels(9999999);
}

/**
 * @brief RE_Shutdown
 * @param[in] destroyWindow
 */
void RE_Shutdown(qboolean destroyWindow)
{
	Ren_Print("RE_Shutdown( %i )\n", destroyWindow);

	ri.Cmd_RemoveSystemCommand("imagelist");
	ri.Cmd_RemoveSystemCommand("shaderlist");
	ri.Cmd_RemoveSystemCommand("skinlist");
	ri.Cmd_RemoveSystemCommand("modellist");
	ri.Cmd_RemoveSystemCommand("screenshot");
	ri.Cmd_RemoveSystemCommand("screenshotJPEG");
	ri.Cmd_RemoveSystemCommand("gfxinfo");
	ri.Cmd_RemoveSystemCommand("taginfo");

	// keep a backup of the current images if possible
	// clean out any remaining unused media from the last backup
	R_PurgeCache();

	if (r_cache->integer)
	{
		if (tr.registered)
		{
			if (destroyWindow)
			{
				R_IssuePendingRenderCommands();
				R_DeleteTextures();
			}
			else
			{
				// backup the current media
				R_BackupModels();
				R_BackupShaders();
				R_BackupImages();
			}
		}
	}
	else if (tr.registered)
	{
		R_IssuePendingRenderCommands();
		R_DeleteTextures();
	}

	R_DoneFreeType();

	R_ShutdownGamma();

	R_ShutdownFBO();

	R_ShutdownShaderPrograms();

	// shut down platform specific Vulkan stuff
	if (destroyWindow)
	{
		R_DoGLimpShutdown();

		// release the virtual memory
		R_Hunk_End();
		R_FreeImageBuffer();
		ri.Tag_Free();  // wipe all render alloc'd zone memory
	}

	tr.registered = qfalse;
}

/**
 * @brief Touch all images to make sure they are resident
 */
void RE_EndRegistration(void)
{
	R_IssuePendingRenderCommands();
}

void R_DebugPolygon(int color, int numPoints, float *points);

#ifdef USE_RENDERER_DLOPEN
/**
 * @brief GetRefAPI
 * @param[in] apiVersion
 * @param[in] rimp
 * @return
 */
Q_EXPORT refexport_t *QDECL GetRefAPI(int apiVersion, refimport_t *rimp)
#else
refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp)
#endif
{
	static refexport_t re;

	ri = *rimp;

	Com_Memset(&re, 0, sizeof(re));

	if (apiVersion != REF_API_VERSION)
	{
		Ren_Print("Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion);
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel     = RE_RegisterModel;
	re.RegisterSkin      = RE_RegisterSkin;

	re.GetSkinModel       = RE_GetSkinModel;
	re.GetShaderFromModel = RE_GetShaderFromModel;

	re.RegisterShader      = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld           = RE_LoadWorldMap;
	re.SetWorldVisData     = RE_SetWorldVisData;
	re.EndRegistration     = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame   = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.ProjectDecal  = RE_ProjectDecal;
	re.ClearDecals   = RE_ClearDecals;

	re.LerpTag     = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.ClearScene          = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;

	re.AddPolyToScene  = RE_AddPolyToScene;
	re.AddPolysToScene = RE_AddPolysToScene;
	re.AddLightToScene = RE_AddLightToScene;

	re.AddCoronaToScene = RE_AddCoronaToScene;
	re.SetFog           = R_SetFog;

	re.RenderScene = RE_RenderScene;

	re.SetColor               = RE_SetColor;
	re.DrawStretchPic         = RE_StretchPic;
	re.DrawRotatedPic         = RE_RotatedPic;
	re.Add2dPolys             = RE_2DPolyies;
	re.DrawStretchPicGradient = RE_StretchPicGradient;
	re.DrawStretchRaw         = RE_StretchRaw;
	re.UploadCinematic        = RE_UploadCinematic;
	re.RegisterFont           = RE_RegisterFont;
	re.RemapShader            = R_RemapShader;
	re.GetEntityToken         = R_GetEntityToken;

	re.DrawDebugPolygon = R_DebugPolygon;
	re.DrawDebugText    = R_DebugText;

	re.AddPolyBufferToScene = RE_AddPolyBufferToScene;

	re.SetGlobalFog = RE_SetGlobalFog;

	re.inPVS = R_inPVS;

	re.purgeCache = R_PurgeCache;

	re.LoadDynamicShader = RE_LoadDynamicShader;
	re.GetTextureId      = R_GetTextureId;

	re.RenderToTexture = RE_RenderToTexture;

	re.Finish              = RE_Finish;
	re.TakeVideoFrame      = RE_TakeVideoFrame;
	re.InitOpenGL          = RE_InitOpenGl;
	re.InitOpenGLSubSystem = RE_InitOpenGlSubsystems;

	return &re;
}
