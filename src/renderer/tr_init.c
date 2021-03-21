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
 * @file renderer/tr_init.c
 * @brief Functions that are not called every frame
 */

#include "tr_local.h"

glconfig_t glConfig;
qboolean   textureFilterAnisotropic = qfalse;
float      maxAnisotropy            = 2.f;

glstate_t glState;

static void GfxInfo_f(void);

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

cvar_t *r_wolfFog;

cvar_t *r_screenshotFormat;
cvar_t *r_screenshotJpegQuality;

cvar_t *r_maxPolys;
cvar_t *r_maxPolyVerts;

cvar_t *r_gfxInfo;

/**
 * @brief This function is responsible for initializing a valid OpenGL subsystem
 *
 * This is done by calling GLimp_Init (which gives us a working OGL subsystem)
 * then setting variables, checking GL constants, and reporting the gfx system
 * config to the user.
 */
static void InitOpenGL(void)
{
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//          - r_fullscreen
	//          - r_mode
	//          - r_(color|depth|stencil)bits
	//          - r_ignorehwgamma
	//          - r_gamma

	if (glConfig.vidWidth == 0)
	{
		char  renderer_buffer[1024];
		GLint temp;

		Com_Memset(&glConfig, 0, sizeof(glConfig));
		ri.GLimp_Init(&glConfig, NULL);

		strcpy(renderer_buffer, glConfig.renderer_string);
		Q_strlwr(renderer_buffer);

		// OpenGL driver constants
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if (glConfig.maxTextureSize <= 0)
		{
			glConfig.maxTextureSize = 0;
		}
	}

	// print info
	GfxInfo_f();

	// set default state
	GL_SetDefaultState();
}

/**
 * @brief GL_CheckErrors
 */
void GL_CheckErrors(void)
{
	unsigned int err;
	char         s[64];

	err = glGetError();
	if (err == GL_NO_ERROR)
	{
		return;
	}
	if (r_ignoreGLErrors->integer)
	{
		return;
	}
	switch (err)
	{
	case GL_INVALID_ENUM:
		strcpy(s, "GL_INVALID_ENUM");
		break;
	case GL_INVALID_VALUE:
		strcpy(s, "GL_INVALID_VALUE");
		break;
	case GL_INVALID_OPERATION:
		strcpy(s, "GL_INVALID_OPERATION");
		break;
	case GL_STACK_OVERFLOW:
		strcpy(s, "GL_STACK_OVERFLOW");
		break;
	case GL_STACK_UNDERFLOW:
		strcpy(s, "GL_STACK_UNDERFLOW");
		break;
	case GL_OUT_OF_MEMORY:
		strcpy(s, "GL_OUT_OF_MEMORY");
		break;
	default:
		Com_sprintf(s, sizeof(s), "%i", err);
		break;
	}

	ri.Error(ERR_VID_FATAL, "GL_CheckErrors: %s", s);
}

/*
 * ==============================================================================
 *
 *                                                SCREEN SHOTS
 *
 * NOTE: some thoughts about the screenshots system:
 * screenshots get written in fs_homepath + fs_gamedir
 * vanilla W:ET .. etmain/screenshots/<FILE>.tga
 * ET: Legacy   .. legacy/screenshots/<FILE>.jpg
 *
 * one command: "screenshot"
 * we use statics to store a count and start writing the first screenshot/screenshot????.jpg available
 * (with FS_FileExists / FS_FOpenFileWrite calls)
 * FIXME: the statics don't get a reinit between fs_game changes
 *
 * ==============================================================================
 */

/**
 * @brief Reads an image but takes care of alignment issues for reading RGB images.
 *
 * @details Reads a minimum offset for where the RGB data starts in the image from
 * integer stored at pointer offset. When the function has returned the actual
 * offset was written back to address offset. This address will always have an
 * alignment of packAlign to ensure efficient copying.
 *
 * Stores the length of padding after a line of pixels to address padlen
 *
 * Return value must be freed with ri.Hunk_FreeTempMemory()
 *
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in,out] offset
 * @param[in,out] padlen
 * @return
 */
byte *RB_ReadPixels(int x, int y, int width, int height, size_t *offset, int *padlen)
{
	byte  *buffer, *bufstart;
	int   padwidth, linelen;
	GLint packAlign;

	glGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen  = width * 3;
	padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ri.Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);

	bufstart = PADP(( intptr_t ) buffer + *offset, packAlign);
	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}

/**
 * @brief RB_ReadZBuffer
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in,out] height
 * @param[in,out] padlen
 * @return
 *
 * @note Unused
 */
byte *RB_ReadZBuffer(int x, int y, int width, int height, int *padlen)
{
	byte  *buffer, *bufstart;
	int   padwidth, linelen;
	GLint packAlign;

	glGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen  = width;
	padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ri.Hunk_AllocateTempMemory(padwidth * height + packAlign - 1);

	bufstart = PADP(( intptr_t ) buffer, packAlign);
	glDepthRange(0.0f, 1.0f);
	glReadPixels(x, y, width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, bufstart);

	*padlen = padwidth - linelen;

	return buffer;
}

/*
 * @brief zbuffer writer for the future implementation of the Depth of field effect
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] fileName
 *
 * @note Unused.
void RB_TakeDepthshot(int x, int y, int width, int height, const char *fileName)
{
    byte   *allbuf, *buffer;
    byte   *srcptr, *destptr;
    byte   *endline, *endmem;
    int    linelen, padlen;
    size_t offset = 18, memcount;

    allbuf = RB_ReadZBuffer(x, y, width, height, &padlen);
    buffer = ri.Hunk_AllocateTempMemory(width * height * 3 + offset);

    Com_Memset(buffer, 0, 18);
    buffer[2]  = 2;         // uncompressed type
    buffer[12] = width & 255;
    buffer[13] = width >> 8;
    buffer[14] = height & 255;
    buffer[15] = height >> 8;
    buffer[16] = 24;        // pixel size

    linelen = width;

    srcptr  = allbuf;
    destptr = buffer + offset;
    endmem  = srcptr + (linelen + padlen) * height;
    while (srcptr < endmem)
    {
        endline = srcptr + linelen;

        while (srcptr < endline)
        {
            *destptr++ = srcptr[0];
            *destptr++ = srcptr[0];
            *destptr++ = srcptr[0];

            srcptr++;
        }

        // Skip the pad
        srcptr += padlen;
    }

    memcount = linelen * 3 * height + offset;

    ri.FS_WriteFile(fileName, buffer, memcount);

    ri.Hunk_FreeTempMemory(allbuf);
    ri.Hunk_FreeTempMemory(buffer);
}
*/

/**
 * @brief RB_TakeScreenshotTGA
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] fileName
 */
void RB_TakeScreenshotTGA(int x, int y, int width, int height, const char *fileName)
{
	byte   *allbuf, *buffer;
	byte   *srcptr, *destptr;
	byte   *endline, *endmem;
	byte   temp;
	int    linelen, padlen;
	size_t offset = 18, memcount;

	allbuf = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	buffer = allbuf + offset - 18;

	Com_Memset(buffer, 0, 18);
	buffer[2]  = 2;         // uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;        // pixel size

	// swap rgb to bgr and remove padding from line endings
	linelen = width * 3;

	srcptr = destptr = allbuf + offset;
	endmem = srcptr + (linelen + padlen) * height;

	while (srcptr < endmem)
	{
		endline = srcptr + linelen;

		while (srcptr < endline)
		{
			temp       = srcptr[0];
			*destptr++ = srcptr[2];
			*destptr++ = srcptr[1];
			*destptr++ = temp;

			srcptr += 3;
		}

		// Skip the pad
		srcptr += padlen;
	}

	memcount = linelen * height;

	// gamma correct
	if (glConfig.deviceSupportsGamma && !tr.gammaProgramUsed)
	{
		R_GammaCorrect(allbuf + offset, memcount);
	}

	ri.FS_WriteFile(fileName, buffer, memcount + 18);

	ri.Hunk_FreeTempMemory(allbuf);
}

/**
 * @brief RB_TakeScreenshotJPEG
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] fileName
 */
void RB_TakeScreenshotJPEG(int x, int y, int width, int height, char *fileName)
{
	byte   *buffer;
	size_t offset = 0, memcount;
	int    padlen;

	buffer   = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if (glConfig.deviceSupportsGamma && !tr.gammaProgramUsed)
	{
		R_GammaCorrect(buffer + offset, memcount);
	}

	RE_SaveJPG(fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen);
	ri.Hunk_FreeTempMemory(buffer);
}

#ifdef FEATURE_PNG
/**
 * @brief RB_TakeScreenshotPNG
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] fileName
 */
void RB_TakeScreenshotPNG(int x, int y, int width, int height, char *fileName)
{
	byte   *buffer;
	size_t offset = 0, memcount;
	int    padlen;

	buffer   = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if (glConfig.deviceSupportsGamma && !tr.gammaProgramUsed)
	{
		R_GammaCorrect(buffer + offset, memcount);
	}

	RE_SavePNG(fileName, width, height, buffer + offset, padlen);
	ri.Hunk_FreeTempMemory(buffer);
}
#endif

/**
 * @brief RB_TakeScreenshotCmd
 * @param[in] data
 * @return
 */
const void *RB_TakeScreenshotCmd(const void *data)
{
	const screenshotCommand_t *cmd = ( const screenshotCommand_t * ) data;

	switch (cmd->format)
	{
		case SSF_TGA:
			RB_TakeScreenshotTGA(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
			break;
		case SSF_JPEG:
			RB_TakeScreenshotJPEG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
			break;
#ifdef FEATURE_PNG
		case SSF_PNG:
			RB_TakeScreenshotPNG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
			break;
#endif
	}

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
 * @param[in] data
 * @return
 */
const void *RB_TakeVideoFrameCmd(const void *data)
{
	const videoFrameCommand_t *cmd;
	byte                      *cBuf;
	size_t                    memcount, linelen;
	int                       padwidth, avipadwidth, padlen, avipadlen;
	GLint                     packAlign;

	// finish any 2D drawing if needed
	if (tess.numIndexes)
	{
		RB_EndSurface();
	}

	cmd = (const videoFrameCommand_t *)data;

	glGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = cmd->width * 3;

	// Alignment stuff for glReadPixels
	padwidth = PAD(linelen, packAlign);
	padlen   = padwidth - linelen;
	// AVI line padding
	avipadwidth = PAD(linelen, AVI_LINE_PADDING);
	avipadlen   = avipadwidth - linelen;

	cBuf = PADP(cmd->captureBuffer, packAlign);

	glReadPixels(0, 0, cmd->width, cmd->height, GL_RGB,
	              GL_UNSIGNED_BYTE, cBuf);

	memcount = padwidth * cmd->height;

	// gamma correct
	if (glConfig.deviceSupportsGamma && !tr.gammaProgramUsed)
	{
		R_GammaCorrect(cBuf, memcount);
	}

	if (cmd->motionJpeg)
	{
		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height,
		                              r_screenshotJpegQuality->integer,
		                              cmd->width, cmd->height, cBuf, padlen);
		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	}
	else
	{
		byte *lineend, *memend;
		byte *srcptr, *destptr;

		srcptr  = cBuf;
		destptr = cmd->encodeBuffer;
		memend  = srcptr + memcount;

		// swap R and B and remove line paddings
		while (srcptr < memend)
		{
			lineend = srcptr + linelen;
			while (srcptr < lineend)
			{
				*destptr++ = srcptr[2];
				*destptr++ = srcptr[1];
				*destptr++ = srcptr[0];
				srcptr    += 3;
			}

			Com_Memset(destptr, '\0', avipadlen);
			destptr += avipadlen;

			srcptr += padlen;
		}

		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}

	return (const void *)(cmd + 1);
}

/**
 * @brief Levelshots are specialized 128*128 thumbnails for
 * the menu system, sampled down from full screen distorted images
 */
void R_LevelShot(void)
{
	char   checkname[MAX_OSPATH];
	byte   *buffer;
	byte   *source, *allsource;
	byte   *src, *dst;
	size_t offset = 0;
	int    padlen;
	int    x, y;
	int    r, g, b;
	float  xScale, yScale;
	int    xx, yy;

	Com_sprintf(checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName);

	allsource = RB_ReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen);
	source    = allsource + offset;

	buffer = ri.Hunk_AllocateTempMemory(128 * 128 * 3 + 18);
	Com_Memset(buffer, 0, 18);
	buffer[2]  = 2;         // uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;        // pixel size

	// resample from source
	xScale = glConfig.vidWidth / 512.0f;
	yScale = glConfig.vidHeight / 384.0f;
	for (y = 0 ; y < 128 ; y++)
	{
		for (x = 0 ; x < 128 ; x++)
		{
			r = g = b = 0;
			for (yy = 0 ; yy < 3 ; yy++)
			{
				for (xx = 0 ; xx < 4 ; xx++)
				{
					src = source + (3 * glConfig.vidWidth + padlen) * ( int ) ((y * 3 + yy) * yScale) +
					      3 * ( int ) ((x * 4 + xx) * xScale);
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst    = buffer + 18 + 3 * (y * 128 + x);
			dst[0] = (byte)(b / 12);
			dst[1] = (byte)(g / 12);
			dst[2] = (byte)(r / 12);
		}
	}

	// gamma correct
	if (glConfig.deviceSupportsGamma && !tr.gammaProgramUsed)
	{
		R_GammaCorrect(buffer + 18, 128 * 128 * 3);
	}

	ri.FS_WriteFile(checkname, buffer, 128 * 128 * 3 + 18);

	ri.Hunk_FreeTempMemory(buffer);
	ri.Hunk_FreeTempMemory(allsource);

	Ren_Print("Wrote %s\n", checkname);
}

/**
 * @brief R_ScreenShot_f
 *
 * @note Doesn't print the pacifier message if there is a second arg
 *
 * screenshot
 * screenshot [silent]
 * screenshot [levelshot]
 * screenshot [filename]
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

	if (!strcmp(ri.Cmd_Argv(1), "levelshot"))
	{
		R_LevelShot();
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
		Com_sprintf(checkname, MAX_OSPATH, "screenshots/%s.%s", ri.Cmd_Argv(1), ext);
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

	R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, format);

	if (!silent)
	{
		Ren_Print("Wrote %s\n", checkname);
	}
}

//============================================================================

/**
 * @brief GL_SetDefaultState
 */
void GL_SetDefaultState(void)
{
	glClearDepth(1.0);

	glCullFace(GL_FRONT);

	glColor4f(1, 1, 1, 1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if (glActiveTextureARB)
	{
		GL_SelectTexture(1);
		GL_TextureMode(r_textureMode->string);
		GL_TexEnv(GL_MODULATE);
		glDisable(GL_TEXTURE_2D);
		GL_SelectTexture(0);
	}

	glEnable(GL_TEXTURE_2D);
	GL_TextureMode(r_textureMode->string);
	GL_TexEnv(GL_MODULATE);

	glShadeModel(GL_SMOOTH);
	glDepthFunc(GL_LEQUAL);

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	glEnableClientState(GL_VERTEX_ARRAY);

	// make sure our GL state vector is set correctly
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

/**
 * @brief GfxInfo_f
 */
void GfxInfo_f(void)
{
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	Ren_Print("GL_VENDOR: %s\n", glConfig.vendor_string);
	Ren_Print("GL_RENDERER: %s\n", glConfig.renderer_string);
	Ren_Print("GL_VERSION: %s\n", glConfig.version_string);
	Ren_Print("GL_SHADING_LANGUAGE_VERSION: %s\n", glConfig.shadingLanguageVersion);

	if (r_gfxInfo->integer > 0)
	{
		Ren_Print("GL_EXTENSIONS: ");
		R_PrintLongString((const char *)glGetString(GL_EXTENSIONS));
		Ren_Print("\n");
	}

	Ren_Print("GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);
	Ren_Print("GL_MAX_ACTIVE_TEXTURES_ARB: %d\n", glConfig.maxActiveTextures);
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
	Ren_Print("texture bits: %d\n", r_textureBits->integer);
	Ren_Print("multitexture: %s\n", enablestrings[glActiveTextureARB != 0]);
	Ren_Print("compiled vertex arrays: %s\n", enablestrings[glLockArraysEXT != 0]);
	Ren_Print("texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0]);
	Ren_Print("compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE]);

	if (r_finish->integer)
	{
		Ren_Print("Forcing glFinish\n");
	}
}

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
	r_allowExtensions       = ri.Cvar_Get("r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_extCompressedTextures = ri.Cvar_Get("r_ext_compressed_textures", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_extMultitexture       = ri.Cvar_Get("r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_extTextureEnvAdd      = ri.Cvar_Get("r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_extTextureFilterAnisotropic = ri.Cvar_Get("r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_extMaxAnisotropy            = ri.Cvar_Get("r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH);

	r_picMip = ri.Cvar_Get("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH);          // mod for DM and DK for id build.  was "1" - pushed back to 1
	ri.Cvar_CheckRange(r_picMip, 0, 3, qtrue);
	r_roundImagesDown = ri.Cvar_Get("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_colorMipLevels = ri.Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH);
	r_detailTextures = ri.Cvar_Get("r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_textureBits    = ri.Cvar_Get("r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);

	r_overBrightBits = ri.Cvar_Get("r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH);        // disable overbrightbits by default
	ri.Cvar_CheckRange(r_overBrightBits, 0, 1, qtrue);                                    // limit to overbrightbits 1 (sorry 1337 players)
	r_simpleMipMaps = ri.Cvar_Get("r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_uiFullScreen  = ri.Cvar_Get("r_uifullscreen", "0", 0);

	r_subDivisions = ri.Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);

	r_ignoreFastPath = ri.Cvar_Get("r_ignoreFastPath", "0", CVAR_ARCHIVE | CVAR_LATCH);    // use fast path by default
	r_greyScale      = ri.Cvar_Get("r_greyscale", "0", CVAR_ARCHIVE | CVAR_LATCH);

	// temporary latched variables that can only change over a restart
	r_mapOverBrightBits = ri.Cvar_Get("r_mapOverBrightBits", "2", CVAR_LATCH);
	ri.Cvar_CheckRange(r_mapOverBrightBits, 0, 3, qtrue);
	r_intensity = ri.Cvar_Get("r_intensity", "1", CVAR_LATCH);
	ri.Cvar_CheckRange(r_intensity, 0, 1.5, qfalse);
	r_singleShader = ri.Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH);

	// archived variables that can change at any time
	r_lodCurveError = ri.Cvar_Get("r_lodCurveError", "250", CVAR_ARCHIVE);
	r_lodBias       = ri.Cvar_Get("r_lodbias", "0", CVAR_ARCHIVE);
	r_flares        = ri.Cvar_Get("r_flares", "1", CVAR_ARCHIVE);
	r_zNear         = ri.Cvar_Get("r_znear", "3", CVAR_CHEAT); // changed it to 3 (from 4) because of lean/fov cheats
	ri.Cvar_CheckRange(r_zNear, 0.001f, 200, qfalse);
	r_zFar = ri.Cvar_Get("r_zfar", "0", CVAR_CHEAT);

	r_ignoreGLErrors = ri.Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE);
	r_fastSky        = ri.Cvar_Get("r_fastsky", "0", CVAR_ARCHIVE);

	r_drawSun      = ri.Cvar_Get("r_drawSun", "1", CVAR_ARCHIVE);
	r_dynamicLight = ri.Cvar_Get("r_dynamiclight", "1", CVAR_ARCHIVE);
	r_finish       = ri.Cvar_Get("r_finish", "0", CVAR_ARCHIVE);
	r_textureMode  = ri.Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	r_gamma        = ri.Cvar_Get("r_gamma", "1.3", CVAR_ARCHIVE);

	r_facePlaneCull = ri.Cvar_Get("r_facePlaneCull", "1", CVAR_ARCHIVE);

	r_railWidth         = ri.Cvar_Get("r_railWidth", "16", CVAR_ARCHIVE);
	r_railSegmentLength = ri.Cvar_Get("r_railSegmentLength", "32", CVAR_ARCHIVE);

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
	r_lodScale        = ri.Cvar_Get("r_lodscale", "5", CVAR_CHEAT);
	r_noreFresh       = ri.Cvar_Get("r_norefresh", "0", CVAR_CHEAT);
	r_drawEntities    = ri.Cvar_Get("r_drawentities", "1", CVAR_CHEAT);
	r_ignore          = ri.Cvar_Get("r_ignore", "1", CVAR_CHEAT);
	r_noCull          = ri.Cvar_Get("r_nocull", "0", CVAR_CHEAT);
	r_noVis           = ri.Cvar_Get("r_novis", "0", CVAR_CHEAT);
	r_showCluster     = ri.Cvar_Get("r_showcluster", "0", CVAR_CHEAT);
	r_speeds          = ri.Cvar_Get("r_speeds", "0", CVAR_CHEAT);

	r_logFile      = ri.Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_debugSurface = ri.Cvar_Get("r_debugSurface", "0", CVAR_CHEAT);
	r_noBind       = ri.Cvar_Get("r_nobind", "0", CVAR_CHEAT);
	r_showTris     = ri.Cvar_Get("r_showtris", "0", CVAR_CHEAT);
	r_trisColor    = ri.Cvar_Get("r_trisColor", "1.0 1.0 1.0 1.0", CVAR_ARCHIVE);
	r_showSky      = ri.Cvar_Get("r_showsky", "0", CVAR_CHEAT);
	r_showNormals  = ri.Cvar_Get("r_shownormals", "0", CVAR_CHEAT);
	r_normalLength = ri.Cvar_Get("r_normallength", "0.5", CVAR_ARCHIVE);
	//r_showmodelbounds = ri.Cvar_Get("r_showmodelbounds", "0", CVAR_CHEAT); // see RB_MDM_SurfaceAnim()
	r_clear        = ri.Cvar_Get("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = ri.Cvar_Get("r_offsetfactor", "-1", CVAR_CHEAT);
	r_offsetUnits  = ri.Cvar_Get("r_offsetunits", "-2", CVAR_CHEAT);
	r_drawBuffer   = ri.Cvar_Get("r_drawBuffer", "GL_BACK", CVAR_CHEAT);
	r_lockPvs      = ri.Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals    = ri.Cvar_Get("r_noportals", "0", CVAR_CHEAT);
	r_shadows      = ri.Cvar_Get("cg_shadows", "1", 0);

	r_screenshotFormat      = ri.Cvar_Get("r_screenshotFormat", "2", CVAR_ARCHIVE);
	r_screenshotJpegQuality = ri.Cvar_Get("r_screenshotJpegQuality", "90", CVAR_ARCHIVE);

	r_portalSky = ri.Cvar_Get("cg_skybox", "1", 0);

	// note: MAX_POLYS and MAX_POLYVERTS are heavily increased in ET compared to q3
	//       - but run 20 bots on oasis and you'll see limits reached (developer 1)
	//       - modern computers can deal with more than our old default values -> users can increase this now to MAX_POLYS/MAX_POLYVERTS
	r_maxPolys = ri.Cvar_Get("r_maxpolys", va("%d", DEFAULT_POLYS), CVAR_LATCH);             // now latched to check against used r_maxpolys and not MAX_POLYS
	ri.Cvar_CheckRange(r_maxPolys, MIN_POLYS, MAX_POLYS, qtrue);                        // MIN_POLYS was old static value
	r_maxPolyVerts = ri.Cvar_Get("r_maxpolyverts", va("%d", DEFAULT_POLYVERTS), CVAR_LATCH); // now latched to check against used r_maxpolyverts and not MAX_POLYVERTS
	ri.Cvar_CheckRange(r_maxPolyVerts, MIN_POLYVERTS, MAX_POLYVERTS, qtrue);            // MIN_POLYVERTS was old static value

	r_gfxInfo = ri.Cvar_Get("r_gfxinfo", "0", 0); // less spammy gfx output at start - enable to print full GL_EXTENSION string

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
}

/**
 * @brief R_Init
 */
void R_Init(void)
{
	int  err;
	int  i;
	byte *ptr;

	Ren_Print("----- Initializing Renderer ----\n");

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

	InitOpenGL();

	R_InitGamma();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Ren_Print("R_Init: glGetError() = 0x%x\n", err);
	}

	Ren_Print("--------------------------------\n");
}

void R_PurgeCache(void)
{
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

	// shut down platform specific OpenGL stuff
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
	/*
	RB: disabled unneeded reference to Sys_LowPhysicalMemory
	if (!Sys_LowPhysicalMemory())
	{
	//              RB_ShowImages();
	}
	*/
}

void R_DebugPolygon(int color, int numPoints, float *points);

#ifdef USE_RENDERER_DLOPEN
/**
 * @brief GetRefAPI
 * @param[in] apiVersion
 * @param[in] rimp
 * @return
 */
Q_EXPORT refexport_t * QDECL GetRefAPI(int apiVersion, refimport_t *rimp)
#else
refexport_t * GetRefAPI(int apiVersion, refimport_t * rimp)
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
