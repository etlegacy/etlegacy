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
 */
/**
 * @file sdl_glimp.c
 */

#include "sdl_defs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(FEATURE_RENDERER2)
#   include "../renderer2/tr_local.h"
#elif defined(FEATURE_RENDERER_GLES)
#   include "../rendererGLES/tr_local.h"
#else // OpenGL 2 renderer
#   include "../renderer/tr_local.h"
#endif

#include "../sys/sys_local.h"
#include "sdl_icon.h"

//static qboolean SDL_VIDEODRIVER_externallySet = qfalse;

#ifdef __APPLE__
#define MACOS_X_GAMMA_RESET_FIX
#ifdef MACOS_X_GAMMA_RESET_FIX
static int gammaResetTime = 0;
#endif
#endif // __APPLE__

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_OLD_GL,

	RSERR_UNKNOWN
} rserr_t;

SDL_Window           *main_window   = NULL;
static SDL_Renderer  *main_renderer = NULL;
static SDL_GLContext SDL_glContext  = NULL;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable
cvar_t *r_centerWindow;

void GLimp_Shutdown(void)
{
	ri.IN_Shutdown();

	if (main_renderer)
	{
		SDL_DestroyRenderer(main_renderer);
		main_renderer = NULL;
	}

	if (main_window)
	{
		SDL_DestroyWindow(main_window);
		main_window = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	Com_Memset(&glConfig, 0, sizeof(glConfig));
	Com_Memset(&glState, 0, sizeof(glState));
}

void *GLimp_MainWindow(void)
{
	return main_window;
}

/**
 * @brief Minimize the game so that user is back at the desktop
 */
void GLimp_Minimize(void)
{
	SDL_MinimizeWindow(main_window);
}

/**
 * @brief Logs OpenGL commands when com_developer cvar is enabled
 */
void GLimp_LogComment(const char *comment)
{
	Ren_Developer("%s", comment);
}

static int GLimp_CompareModes(const void *a, const void *b)
{
	const float ASPECT_EPSILON  = 0.001f;
	SDL_Rect    *modeA          = (SDL_Rect *)a;
	SDL_Rect    *modeB          = (SDL_Rect *)b;
	float       aspectA         = (float)modeA->w / (float)modeA->h;
	float       aspectB         = (float)modeB->w / (float)modeB->h;
	int         areaA           = modeA->w * modeA->h;
	int         areaB           = modeB->w * modeB->h;
	float       aspectDiffA     = fabs(aspectA - displayAspect);
	float       aspectDiffB     = fabs(aspectB - displayAspect);
	float       aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if (aspectDiffsDiff > ASPECT_EPSILON)
	{
		return 1;
	}
	else if (aspectDiffsDiff < -ASPECT_EPSILON)
	{
		return -1;
	}
	else
	{
		return areaA - areaB;
	}
}

static void GLimp_DetectAvailableModes(void)
{
	int             i, j;
	char            buf[MAX_STRING_CHARS] = { 0 };
	SDL_Rect        modes[128];
	int             numModes = 0;
	int             display  = 0;
	SDL_DisplayMode windowMode;

	if (!main_window)
	{
		if (!SDL_GetNumVideoDisplays())
		{
			Ren_Fatal("There is no available display to open a game screen - %s", SDL_GetError());
			return;
		}

		// Use the zero display index
		display = 0;
	}
	else
	{
		// Detect the used display
		display = SDL_GetWindowDisplayIndex(main_window);
	}

	// was SDL_GetWindowDisplayMode
	if (SDL_GetDesktopDisplayMode(display, &windowMode) < 0)
	{
		Ren_Warning("Couldn't get desktop display mode, no resolutions detected - %s\n", SDL_GetError());
		return;
	}

	for (i = 0; i < SDL_GetNumDisplayModes(display); i++)
	{
		SDL_DisplayMode mode;

		if (SDL_GetDisplayMode(display, i, &mode) < 0)
		{
			continue;
		}

		if (!mode.w || !mode.h)
		{
			Ren_Print("Display supports any resolution\n");
			return;
		}

		if (windowMode.format != mode.format)
		{
			continue;
		}

		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for (j = 0; j < numModes; j++)
		{
			if (mode.w == modes[j].w && mode.h == modes[j].h)
			{
				break;
			}
		}

		if (j != numModes)
		{
			continue;
		}

		modes[numModes].w = mode.w;
		modes[numModes].h = mode.h;
		numModes++;
	}

	if (numModes > 1)
	{
		qsort(modes, numModes, sizeof(SDL_Rect), GLimp_CompareModes);
	}

	for (i = 0; i < numModes; i++)
	{
		const char *newModeString = va("%ux%u ", modes[i].w, modes[i].h);

		if (strlen(newModeString) < (int)sizeof(buf) - strlen(buf))
		{
			Q_strcat(buf, sizeof(buf), newModeString);
		}
		else
		{
			Ren_Warning("Skipping mode %ux%u, buffer too small\n", modes[i].w, modes[i].h);
		}
	}

	if (*buf)
	{
		buf[strlen(buf) - 1] = 0;
		Ren_Print("Available modes [%i]: '%s'\n", numModes, buf);
		ri.Cvar_Set("r_availableModes", buf);
	}
}

qboolean GL_CheckForExtension(const char *ext)
{
#ifdef FEATURE_RENDERER2
	int i = 0, exts = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &exts);
	for (i = 0; i < exts; i++)
	{
		if (!Q_stricmp(ext, (char *)glGetStringi(GL_EXTENSIONS, i)))
		{
			return qtrue;
		}
	}
	return qfalse;
#else
	const char *ptr = Q_stristr(glConfig.extensions_string, ext);
	if (ptr == NULL)
	{
		return qfalse;
	}
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
#endif
}


#define MSG_ERR_OLD_VIDEO_DRIVER                                                       \
	"\nET: Legacy with OpenGL 3.x renderer can not run on this "                             \
	"machine since it is missing one or more required OpenGL "                             \
	"extensions. Please update your video card drivers and try again.\n"

static qboolean GLimp_InitOpenGLContext()
{

#ifdef FEATURE_RENDERER2
	int GLmajor, GLminor;
#endif

	// get vendor
	Q_strncpyz(glConfig.vendor_string, (char *) qglGetString(GL_VENDOR), sizeof(glConfig.vendor_string));

	// get renderer
	Q_strncpyz(glConfig.renderer_string, (char *) qglGetString(GL_RENDERER), sizeof(glConfig.renderer_string));
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
	{
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	}

	// get GL version
	Q_strncpyz(glConfig.version_string, (char *) qglGetString(GL_VERSION), sizeof(glConfig.version_string));

	Ren_Print("GL_VENDOR: %s\n", glConfig.vendor_string);
	Ren_Print("GL_RENDERER: %s\n", glConfig.renderer_string);
	Ren_Print("GL_VERSION: %s\n", glConfig.version_string);

#ifndef FEATURE_RENDERER2
	Ren_Print("Using vanilla renderer\n");
#else
	// get shading language version
	Q_strncpyz(glConfig2.shadingLanguageVersion, (char *)glGetString(GL_SHADING_LANGUAGE_VERSION), sizeof(glConfig2.shadingLanguageVersion));
	sscanf(glConfig2.shadingLanguageVersion, "%d.%d", &glConfig2.glslMajorVersion, &glConfig2.glslMinorVersion);
	Ren_Print("GL_SHADING_LANGUAGE_VERSION: %s\n", glConfig2.shadingLanguageVersion);

	// get GL context version
	sscanf(( const char * ) glGetString(GL_VERSION), "%d.%d", &GLmajor, &GLminor);
	glConfig2.contextCombined = (GLmajor * 100) + (GLminor * 10);

	if (GLmajor < 2)
	{
		// missing shader support
		return qfalse;
	}

	if (GLmajor < 3 || (GLmajor == 3 && GLminor < 2))
	{
		// shaders are supported, but not all GL3.x features
		Ren_Print("Using enhanced renderer in GL 2.x mode\n");
		return qtrue;
	}

	Ren_Print("Using enhanced renderer in GL 3.x mode\n");
	glConfig.driverType = GLDRV_OPENGL3;
#endif

	return qtrue;
}

#ifdef FEATURE_RENDERER2

void GLAPIENTRY Glimp_DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const GLvoid *userParam)
{
	Ren_Warning("Driver message: %s\n", message);
}

static qboolean GLimp_CheckForVersionExtension(const char *ext, int coresince, qboolean required, cvar_t *var)
{
	qboolean result = qfalse;

	if ((coresince >= 0 && coresince <= glConfig2.contextCombined) || GL_CheckForExtension(ext))
	{
		if (var && var->integer)
		{
			result = qtrue;
		}
		else if (!var)
		{
			result = qtrue;
		}
	}

	if (required && !result)
	{
		Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: %s\n", ext);
	}

	if (result)
	{
		Ren_Print("...found OpenGL extension - %s\n", ext);
	}
	else
	{
		if (var)
		{
			Ren_Print("...ignoring %s\n", ext);
		}
		else
		{
			Ren_Print("...%s not found\n", ext);
		}
	}

	return result;
}

static void GLimp_InitExtensionsR2(void)
{
	Ren_Print("Initializing OpenGL extensions\n");

	// GL_ARB_multitexture
	if (glConfig.driverType != GLDRV_OPENGL3)
	{
		if (GLEW_ARB_multitexture)
		{
			glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glConfig.maxActiveTextures);

			if (glConfig.maxActiveTextures > 1)
			{
				Ren_Print("...found OpenGL extension - GL_ARB_multitexture\n");
			}
			else
			{
				Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_multitexture, < 2 texture units");
			}
		}
		else
		{
			Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_multitexture");
		}
	}

	// GL_ARB_depth_texture
	GLimp_CheckForVersionExtension("GL_ARB_depth_texture", 130, qtrue, NULL);

	if (GLimp_CheckForVersionExtension("GL_ARB_texture_cube_map", 130, qtrue, NULL))
	{
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig2.maxCubeMapTextureSize);
	}
	GL_CheckErrors();

	GLimp_CheckForVersionExtension("GL_ARB_vertex_program", 210, qtrue, NULL);
	GLimp_CheckForVersionExtension("GL_ARB_vertex_buffer_object", 300, qtrue, NULL);

	// GL_ARB_occlusion_query
	glConfig2.occlusionQueryAvailable = qfalse;
	glConfig2.occlusionQueryBits      = 0;
	if (GLimp_CheckForVersionExtension("GL_ARB_occlusion_query", 150, qfalse, r_ext_occlusion_query))
	{
		glConfig2.occlusionQueryAvailable = qtrue;
		glGetQueryivARB(GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &glConfig2.occlusionQueryBits);
	}
	GL_CheckErrors();

	GLimp_CheckForVersionExtension("GL_ARB_shader_objects", 210, qtrue, NULL);

	if (GLimp_CheckForVersionExtension("GL_ARB_vertex_shader", 210, qtrue, NULL))
	{
		int reservedComponents;

		GL_CheckErrors();
		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig2.maxVertexUniforms); GL_CheckErrors();
		//glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &glConfig.maxVaryingFloats); GL_CheckErrors();
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig2.maxVertexAttribs); GL_CheckErrors();

		reservedComponents = 16 * 10; // approximation how many uniforms we have besides the bone matrices

		glConfig2.maxVertexSkinningBones     = (int) Q_bound(0.0, (Q_max(glConfig2.maxVertexUniforms - reservedComponents, 0) / 16), MAX_BONES);
		glConfig2.vboVertexSkinningAvailable = (qboolean)(r_vboVertexSkinning->integer && ((glConfig2.maxVertexSkinningBones >= 12) ? qtrue : qfalse));
	}
	GL_CheckErrors();

	GLimp_CheckForVersionExtension("GL_ARB_fragment_shader", 210, qtrue, NULL);

	// GL_ARB_shading_language_100
	if (GLimp_CheckForVersionExtension("GL_ARB_shading_language_100", 210, qtrue, NULL))
	{
		Q_strncpyz(glConfig2.shadingLanguageVersion, (char *)glGetString(GL_SHADING_LANGUAGE_VERSION_ARB), sizeof(glConfig2.shadingLanguageVersion));
		sscanf(glConfig2.shadingLanguageVersion, "%d.%d", &glConfig2.glslMajorVersion, &glConfig2.glslMinorVersion);
	}
	GL_CheckErrors();

	glConfig2.textureNPOTAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_ARB_texture_non_power_of_two", 300, qfalse, r_ext_texture_non_power_of_two))
	{
		glConfig2.textureNPOTAvailable = qtrue;
	}

	glConfig2.drawBuffersAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_ARB_draw_buffers", /* -1 */ 300, qfalse, r_ext_draw_buffers))
	{
		glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &glConfig2.maxDrawBuffers);
		glConfig2.drawBuffersAvailable = qtrue;
	}

	glConfig2.textureHalfFloatAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_ARB_half_float_pixel", 300, qfalse, r_ext_half_float_pixel))
	{
		glConfig2.textureHalfFloatAvailable = qtrue;
	}

	glConfig2.textureFloatAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_ARB_texture_float", 300, qfalse, r_ext_texture_float))
	{
		glConfig2.textureFloatAvailable = qtrue;
	}

	glConfig2.ARBTextureCompressionAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_ARB_texture_compression", 300, qfalse, r_ext_compressed_textures))
	{
		glConfig2.ARBTextureCompressionAvailable = qtrue;
		glConfig.textureCompression              = TC_NONE;
	}

	glConfig2.vertexArrayObjectAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_ARB_vertex_array_object", 300, qfalse, r_ext_vertex_array_object))
	{
		glConfig2.vertexArrayObjectAvailable = qtrue;
	}

	// GL_EXT_texture_compression_s3tc
	if (GLimp_CheckForVersionExtension("GL_EXT_texture_compression_s3tc", -1, qfalse, r_ext_compressed_textures))
	{
		glConfig.textureCompression = TC_S3TC;
	}

	glConfig2.texture3DAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_EXT_texture3D", 170, qfalse, NULL))
	{
		glConfig2.texture3DAvailable = qtrue;
	}

	glConfig2.stencilWrapAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_EXT_stencil_wrap", 210, qfalse, r_ext_stencil_wrap))
	{
		glConfig2.stencilWrapAvailable = qtrue;
	}

	glConfig2.textureAnisotropyAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_EXT_texture_filter_anisotropic", -1, qfalse, r_ext_texture_filter_anisotropic))
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig2.maxTextureAnisotropy);
		glConfig2.textureAnisotropyAvailable = qtrue;
	}
	GL_CheckErrors();

	GLimp_CheckForVersionExtension("GL_EXT_stencil_two_side", 210, qfalse, r_ext_stencil_two_side);
	GLimp_CheckForVersionExtension("GL_EXT_depth_bounds_test", 170, qfalse, r_ext_depth_bounds_test);

	glConfig2.framebufferObjectAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_EXT_framebuffer_object", 300, qfalse, r_ext_packed_depth_stencil))
	{
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &glConfig2.maxRenderbufferSize);
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &glConfig2.maxColorAttachments);

		glConfig2.framebufferObjectAvailable = qtrue;
	}
	GL_CheckErrors();

	glConfig2.framebufferPackedDepthStencilAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_EXT_packed_depth_stencil", 300, qfalse, r_ext_packed_depth_stencil) && glConfig.driverType != GLDRV_MESA)
	{
		glConfig2.framebufferPackedDepthStencilAvailable = qtrue;
	}

	glConfig2.framebufferBlitAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_EXT_framebuffer_blit", 300, qfalse, r_ext_framebuffer_blit))
	{
		glConfig2.framebufferBlitAvailable = qtrue;
	}

	// GL_EXTX_framebuffer_mixed_formats not used

	glConfig2.generateMipmapAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_SGIS_generate_mipmap", 140, qfalse, r_ext_generate_mipmap))
	{
		glConfig2.generateMipmapAvailable = qtrue;
	}

	glConfig2.getProgramBinaryAvailable = qfalse;
	if (GLimp_CheckForVersionExtension("GL_ARB_get_program_binary", 410, qfalse, NULL))
	{
		int formats = 0;

		glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);

		if (formats)
		{
			glConfig2.getProgramBinaryAvailable = qtrue;
		}
	}

	// If we are in developer mode, then we will print out messages from the gfx driver
	if (GLimp_CheckForVersionExtension("GL_ARB_debug_output", 410, qfalse, NULL) && ri.Cvar_VariableIntegerValue("developer"))
	{
#ifdef GL_DEBUG_OUTPUT
		glEnable(GL_DEBUG_OUTPUT);

		if (410 <= glConfig2.contextCombined)
		{
			glDebugMessageCallback(Glimp_DebugCallback, NULL);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}
		else
		{
			glDebugMessageCallbackARB(Glimp_DebugCallback, NULL);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		}
#endif
	}
}
#endif

/* unused
// TODO: remove the whole 2D rendering code from here?
static int Glimp_Create2DRenderer(SDL_Window *window)
{
    // We must call SDL_CreateRenderer in order for draw calls to affect this window.
    main_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!main_renderer)
    {
        Ren_Developer("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return qfalse;
    }

    return qtrue;
}
*/

static int GLimp_SetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	int             perChannelColorBits;
	int             colorBits, depthBits, stencilBits;
	int             samples;
	int             i     = 0;
	SDL_Surface     *icon = NULL;
	SDL_DisplayMode desktopMode;
	int             display = 0;
	int             x       = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;

	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_GRABBED;

#ifndef FEATURE_RENDERER_GLES
	GLenum glewResult;
#endif

	Ren_Print("Initializing OpenGL display\n");

	if (r_allowResize->integer && !fullscreen)
	{
		flags |= SDL_WINDOW_RESIZABLE;
	}

	icon = SDL_CreateRGBSurfaceFrom(
	    (void *)CLIENT_WINDOW_ICON.pixel_data,
	    CLIENT_WINDOW_ICON.width,
	    CLIENT_WINDOW_ICON.height,
	    CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
	    CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
	    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
	    0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
	    );

	// If a window exists, note its display index
	if (main_window != NULL)
	{
		display = SDL_GetWindowDisplayIndex(main_window);
	}

	if (SDL_GetDesktopDisplayMode(display, &desktopMode) == 0)
	{
		displayAspect = (float)desktopMode.w / (float)desktopMode.h;

		Ren_Print("Estimated display aspect: %.3f\n", displayAspect);
	}
	else
	{
		Com_Memset(&desktopMode, 0, sizeof(SDL_DisplayMode));

		Ren_Print("Cannot estimate display aspect, assuming 1.333\n");
	}

	Ren_Print("...setting mode %d: ", mode);

	if (mode == -2)
	{
		// use desktop video resolution
		if (desktopMode.h > 0)
		{
			glConfig.vidWidth  = desktopMode.w;
			glConfig.vidHeight = desktopMode.h;
		}
		else
		{
			glConfig.vidWidth  = 640;
			glConfig.vidHeight = 480;
			Ren_Print("Cannot determine display resolution, assuming 640x480\n");
		}

		glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}
	else if (!R_GetModeInfo(&glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode))
	{
		Ren_Print("invalid mode\n");
		return RSERR_INVALID_MODE;
	}
	Ren_Print("%dx%d\n", glConfig.vidWidth, glConfig.vidHeight);

	// Center window
	if (r_centerWindow->integer && !fullscreen)
	{
		x = (desktopMode.w / 2) - (glConfig.vidWidth / 2);
		y = (desktopMode.h / 2) - (glConfig.vidHeight / 2);
	}

	// Destroy existing state if it exists
	if (SDL_glContext != NULL)
	{
		SDL_GL_DeleteContext(SDL_glContext);
		SDL_glContext = NULL;
	}

	if (main_window != NULL)
	{
		SDL_GetWindowPosition(main_window, &x, &y);
		Ren_Developer("Existing window at %dx%d before being destroyed\n", x, y);
		SDL_DestroyWindow(main_window);
		main_window = NULL;
	}

	if (fullscreen)
	{
		if (r_mode->integer == -2)
		{
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		else
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		glConfig.isFullscreen = qtrue;
	}
	else
	{
		if (noborder)
		{
			flags |= SDL_WINDOW_BORDERLESS;
		}

		glConfig.isFullscreen = qfalse;
	}

	colorBits = r_colorbits->value;
	if ((!colorBits) || (colorBits >= 32))
	{
		colorBits = 24;
	}

	if (!r_depthbits->value)
	{
		depthBits = 24;
	}
	else
	{
		depthBits = r_depthbits->value;
	}
	stencilBits = r_stencilbits->value;
	samples     = r_ext_multisample->value;

	for (i = 0; i < 16; i++)
	{
		int testColorBits, testDepthBits, testStencilBits;

		// 0 - default
		// 1 - minus colorBits
		// 2 - minus depthBits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
			case 2:
				if (colorBits == 24)
				{
					colorBits = 16;
				}
				break;
			case 1:
				if (depthBits == 32)
				{
					depthBits = 24;
				}
				else if (depthBits == 24)
				{
					depthBits = 16;
				}
				else if (depthBits == 16)
				{
					depthBits = 8;
				}
			case 3: // fall through
				if (stencilBits == 24)
				{
					stencilBits = 16;
				}
				else if (stencilBits == 16)
				{
					stencilBits = 8;
				}
			}
		}

		testColorBits   = colorBits;
		testDepthBits   = depthBits;
		testStencilBits = stencilBits;

		if ((i % 4) == 3) // reduce colorbits
		{
			if (testColorBits == 24)
			{
				testColorBits = 16;
			}
		}

		if ((i % 4) == 2) // reduce depthbits
		{
			if (testDepthBits == 24)
			{
				testDepthBits = 16;
			}
			else if (testDepthBits == 16)
			{
				testDepthBits = 8;
			}
		}

		if ((i % 4) == 1) // reduce stencilbits
		{
			if (testStencilBits == 24)
			{
				testStencilBits = 16;
			}
			else if (testStencilBits == 16)
			{
				testStencilBits = 8;
			}
			else
			{
				testStencilBits = 0;
			}
		}

		if (testColorBits == 24)
		{
			perChannelColorBits = 8;
		}
		else
		{
			perChannelColorBits = 4;
		}

#ifdef __sgi // Fix for SGIs grabbing too many bits of color
		if (perChannelColorBits == 4)
		{
			perChannelColorBits = 0; /* Use minimum size for 16-bit color */

		}
		// Need alpha or else SGIs choose 36+ bit RGB mode
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
#endif

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, testDepthBits);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, testStencilBits);

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);

		// SDL2 uses opengl by default, if we want opengl es we need to set this attribute
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);

		if (r_stereoEnabled->integer)
		{
			glConfig.stereoEnabled = qtrue;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
		}
		else
		{
			glConfig.stereoEnabled = qfalse;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		}

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		// If not allowing software GL, demand accelerated
		if (!r_allowSoftwareGL->integer)
		{
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		}

		main_window = SDL_CreateWindow(CLIENT_WINDOW_TITLE, x, y, glConfig.vidWidth, glConfig.vidHeight, flags | SDL_WINDOW_SHOWN);

		if (!main_window)
		{
			Ren_Developer("SDL_CreateWindow failed: %s\n", SDL_GetError());
			continue;
		}

		//This is disabled since at least now we have no use for this
		/*
		if (!Glimp_Create2DRenderer(main_window))
		{
		    continue;
		}
		*/

		if (fullscreen)
		{
			SDL_DisplayMode mode;

			switch (testColorBits)
			{
			case 16: mode.format = SDL_PIXELFORMAT_RGB565; break;
			case 24: mode.format = SDL_PIXELFORMAT_RGB24;  break;
			default: Ren_Developer("testColorBits is %d, can't fullscreen\n", testColorBits); continue;
			}

			mode.w            = glConfig.vidWidth;
			mode.h            = glConfig.vidHeight;
			mode.refresh_rate = glConfig.displayFrequency = ri.Cvar_VariableIntegerValue("r_displayRefresh");
			mode.driverdata   = NULL;

			if (SDL_SetWindowDisplayMode(main_window, &mode) < 0)
			{
				Ren_Developer("SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError());
				continue;
			}
		}

		SDL_SetWindowIcon(main_window, icon);

#if defined(FEATURE_RENDERER2)
		glewExperimental = GL_TRUE;

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

		if ((SDL_glContext = SDL_GL_CreateContext(main_window)) == NULL)
		{
			Ren_Developer("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
			continue;
		}

		SDL_GL_MakeCurrent(main_window, SDL_glContext);
		SDL_GL_SetSwapInterval(r_swapInterval->integer);

		glConfig.colorBits   = testColorBits;
		glConfig.depthBits   = testDepthBits;
		glConfig.stencilBits = testStencilBits;

		ri.Printf(PRINT_ALL, "Using %d color bits, %d depth, %d stencil display.\n",
		          glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
		break;
	}

	GLimp_DetectAvailableModes();

#if !defined(FEATURE_RENDERER_GLES)
	glewResult = glewInit();

	if (GLEW_OK != glewResult)
	{
		// glewInit failed, something is seriously wrong
		Ren_Fatal("GLW_StartOpenGL() - could not load OpenGL subsystem: %s", glewGetErrorString(glewResult));
	}
	else
	{
		Ren_Print("Using GLEW %s\n", glewGetString(GLEW_VERSION));
	}
#endif

	if (!GLimp_InitOpenGLContext())
	{
		return RSERR_OLD_GL;
	}

	if (!main_window) //|| !main_renderer)
	{
		Ren_Print("Couldn't get a visual\n");
		return RSERR_INVALID_MODE;
	}

	SDL_FreeSurface(icon);

	return RSERR_OK;
}

static qboolean GLimp_StartDriverAndSetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	rserr_t err;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			Ren_Print("SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n", SDL_GetError());
			return qfalse;
		}

		Ren_Print("SDL initialized driver \"%s\"\n", SDL_GetCurrentVideoDriver());
	}

	if (fullscreen && ri.Cvar_VariableIntegerValue("in_nograb"))
	{
		Ren_Print("Fullscreen not allowed with in_nograb 1\n");
		ri.Cvar_Set("r_fullscreen", "0");
		r_fullscreen->modified = qfalse;
		fullscreen             = qfalse;
	}

	err = GLimp_SetMode(mode, fullscreen, noborder);

	switch (err)
	{
	case RSERR_INVALID_FULLSCREEN:
		Ren_Print("...WARNING: fullscreen unavailable in this mode\n");
		return qfalse;
	case RSERR_INVALID_MODE:
		Ren_Print("...WARNING: could not set the given mode (%d)\n", mode);
		return qfalse;
	case RSERR_OLD_GL:
		ri.Error(ERR_VID_FATAL, "Could not create opengl 3 context");
		return qfalse;
	default:
		break;
	}

	return qtrue;
}

#ifndef FEATURE_RENDERER2

static void GLimp_InitExtensions(void)
{
	if (!r_allowExtensions->integer)
	{
		Ren_Print("* IGNORING OPENGL EXTENSIONS *\n");
		return;
	}

	Ren_Print("Initializing OpenGL extensions\n");

	glConfig.textureCompression = TC_NONE;

#if !defined(FEATURE_RENDERER_GLES)
	// GL_EXT_texture_compression_s3tc
	if (GLEW_ARB_texture_compression &&
	    GLEW_EXT_texture_compression_s3tc)
	{
		if (r_ext_compressed_textures->value)
		{
			glConfig.textureCompression = TC_S3TC_ARB;
			Ren_Print("...found OpenGL extension - GL_EXT_texture_compression_s3tc\n");
		}
		else
		{
			Ren_Print("...ignoring GL_EXT_texture_compression_s3tc\n");
		}
	}
	else
#endif
	{
		Ren_Print("...GL_EXT_texture_compression_s3tc not found\n");
	}

#if !defined(FEATURE_RENDERER_GLES)
	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if (glConfig.textureCompression == TC_NONE)
	{
		if (GLEW_S3_s3tc)
		{
			if (r_ext_compressed_textures->value)
			{
				glConfig.textureCompression = TC_S3TC;
				Ren_Print("...found OpenGL extension - GL_S3_s3tc\n");
			}
			else
			{
				Ren_Print("...ignoring GL_S3_s3tc\n");
			}
		}
		else
		{
			Ren_Print("...GL_S3_s3tc not found\n");
		}
	}
#endif

	// GL_EXT_texture_env_add
#ifdef FEATURE_RENDERER_GLES
	glConfig.textureEnvAddAvailable = qtrue;
	Ren_Print("...using GL_EXT_texture_env_add\n");
#else
	glConfig.textureEnvAddAvailable = qfalse;
	if (GLEW_EXT_texture_env_add)
	{
		if (r_ext_texture_env_add->integer)
		{
			glConfig.textureEnvAddAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_EXT_texture_env_add\n");
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			Ren_Print("...ignoring GL_EXT_texture_env_add\n");
		}
	}
	else
	{
		Ren_Print("...GL_EXT_texture_env_add not found\n");
	}
#endif

	// GL_ARB_multitexture
	glConfig.maxActiveTextures = 1;
#if defined(FEATURE_RENDERER_GLES)
	GLint glint = 0;
	qglGetIntegerv(GL_MAX_TEXTURE_UNITS, &glint);
	glConfig.maxActiveTextures = (int)glint;

	if (glConfig.maxActiveTextures > 1)
	{
		Ren_Print("...using GL_ARB_multitexture (%i texture units)\n", glConfig.maxActiveTextures);
	}
	else
	{
		Ren_Print("...not using GL_ARB_multitexture, < 2 texture units\n");
	}
#else
	if (GLEW_ARB_multitexture)
	{
		if (r_ext_multitexture->value)
		{
			GLint glint = 0;

			glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glint);

			glConfig.maxActiveTextures = (int) glint;

			if (glConfig.maxActiveTextures > 1)
			{
				Ren_Print("...found OpenGL extension - GL_ARB_multitexture\n");
			}
			else
			{
				Ren_Print("...not using GL_ARB_multitexture, < 2 texture units\n");
			}
		}
		else
		{
			Ren_Print("...ignoring GL_ARB_multitexture\n");
		}
	}
	else
	{
		Ren_Print("...GL_ARB_multitexture not found\n");
	}
#endif
}
#endif

// FIXME: rework & add latest gfx cards
void GLimp_SetHardware(void)
{
	if (glConfig.driverType != GLDRV_OPENGL3 &&
	    (Q_stristr(glConfig.renderer_string, "mesa") ||
	     Q_stristr(glConfig.renderer_string, "gallium") ||
	     Q_stristr(glConfig.vendor_string, "nouveau") ||
	     Q_stristr(glConfig.vendor_string, "mesa")))
	{
		glConfig.driverType = GLDRV_MESA;
	}

	if (Q_stristr(glConfig.vendor_string, "Intel"))
	{
		// FIXME: filter out HD Graphics from first generation ("Westmere")
		if (Q_stristr(glConfig.renderer_string, "HD Graphics") ||     // proprietary driver
		    Q_stristr(glConfig.renderer_string, "Iris Graphics") ||
		    Q_stristr(glConfig.renderer_string, "Iris Pro Graphics") ||
		    Q_stristr(glConfig.renderer_string, "Sandybridge") ||     // open source driver (Mesa DRI)
		    Q_stristr(glConfig.renderer_string, "Ivybridge") ||
		    Q_stristr(glConfig.renderer_string, "Haswell") ||
		    Q_stristr(glConfig.renderer_string, "Broadwell") ||
		    Q_stristr(glConfig.renderer_string, "Skylake") ||
		    Q_stristr(glConfig.renderer_string, "Cannonlake"))
		{
			glConfig.hardwareType = GLHW_GENERIC_GL3;
		}
		else
		{
			Ren_Print("Warning: unknown HD/Iris Graphics series\n");
			//glConfig.hardwareType = GLHW_GENERIC;
		}
	}
	else if (Q_stristr(glConfig.renderer_string, "geforce"))
	{
		if (Q_stristr(glConfig.renderer_string, "8400") ||
		    Q_stristr(glConfig.renderer_string, "8500") ||
		    Q_stristr(glConfig.renderer_string, "8600") ||
		    Q_stristr(glConfig.renderer_string, "8800") ||
		    Q_stristr(glConfig.renderer_string, "9500") ||
		    Q_stristr(glConfig.renderer_string, "9600") ||
		    Q_stristr(glConfig.renderer_string, "9800") ||
		    Q_stristr(glConfig.renderer_string, "gts 240") ||
		    Q_stristr(glConfig.renderer_string, "gts 250") ||
		    Q_stristr(glConfig.renderer_string, "gtx 260") ||
		    Q_stristr(glConfig.renderer_string, "gtx 275") ||
		    Q_stristr(glConfig.renderer_string, "gtx 280") ||
		    Q_stristr(glConfig.renderer_string, "gtx 285") ||
		    Q_stristr(glConfig.renderer_string, "gtx 295") ||
		    Q_stristr(glConfig.renderer_string, "gt 320") ||
		    Q_stristr(glConfig.renderer_string, "gt 330") ||
		    Q_stristr(glConfig.renderer_string, "gt 340") ||
		    Q_stristr(glConfig.renderer_string, "gt 415") ||
		    Q_stristr(glConfig.renderer_string, "gt 420") ||
		    Q_stristr(glConfig.renderer_string, "gt 425") ||
		    Q_stristr(glConfig.renderer_string, "gt 430") ||
		    Q_stristr(glConfig.renderer_string, "gt 435") ||
		    Q_stristr(glConfig.renderer_string, "gt 440") ||
		    Q_stristr(glConfig.renderer_string, "gt 520") ||
		    Q_stristr(glConfig.renderer_string, "gt 525") ||
		    Q_stristr(glConfig.renderer_string, "gt 540") ||
		    Q_stristr(glConfig.renderer_string, "gt 550") ||
		    Q_stristr(glConfig.renderer_string, "gt 555") ||
		    Q_stristr(glConfig.renderer_string, "gts 450") ||
		    Q_stristr(glConfig.renderer_string, "gtx 460") ||
		    Q_stristr(glConfig.renderer_string, "gtx 470") ||
		    Q_stristr(glConfig.renderer_string, "gtx 480") ||
		    Q_stristr(glConfig.renderer_string, "gtx 485") ||
		    Q_stristr(glConfig.renderer_string, "gtx 560") ||
		    Q_stristr(glConfig.renderer_string, "gtx 570") ||
		    Q_stristr(glConfig.renderer_string, "gtx 580") ||
		    Q_stristr(glConfig.renderer_string, "gtx 590") ||
		    Q_stristr(glConfig.renderer_string, "gtx 630") ||
		    Q_stristr(glConfig.renderer_string, "gtx 640") ||
		    Q_stristr(glConfig.renderer_string, "gtx 645") ||
		    Q_stristr(glConfig.renderer_string, "gtx 670") ||
		    Q_stristr(glConfig.renderer_string, "gtx 680") ||
		    Q_stristr(glConfig.renderer_string, "gtx 690") ||
		    Q_stristr(glConfig.renderer_string, "gtx 770"))
		{
			glConfig.hardwareType = GLHW_NV_DX10;
		}
		else
		{
			Ren_Print("Warning: unknown GeForce series\n");
			//glConfig.hardwareType = GLHW_GENERIC;
		}

	}
	else if (Q_stristr(glConfig.renderer_string, "quadro fx"))
	{
		if (Q_stristr(glConfig.renderer_string, "3600"))
		{
			glConfig.hardwareType = GLHW_NV_DX10;
		}
		else
		{
			Ren_Print("Warning: unknown Quadro series\n");
			//glConfig.hardwareType = GLHW_GENERIC;
		}
	}
	else if (Q_stristr(glConfig.renderer_string, "gallium") &&
	         Q_stristr(glConfig.renderer_string, " amd "))
	{
		// anything prior to R600 is listed as ATI.
		glConfig.hardwareType = GLHW_ATI_DX10;
	}
	else if (Q_stristr(glConfig.renderer_string, "rv770") ||
	         Q_stristr(glConfig.renderer_string, "eah4850") ||
	         Q_stristr(glConfig.renderer_string, "eah4870") ||
	         // previous three are too specific?
	         Q_stristr(glConfig.renderer_string, "radeon hd"))
	{
		glConfig.hardwareType = GLHW_ATI_DX10;
	}
	else if (Q_stristr(glConfig.renderer_string, "radeon"))
	{
		glConfig.hardwareType = GLHW_ATI;
	}
	else
	{
		Ren_Print("Warning: unknown gfx card - hardware type not set\n");
		//glConfig.hardwareType = GLHW_GENERIC;
	}
}

void Glimp_ClearScreen(void)
{
	qglClearColor(0, 0, 0, 1);
	qglClear(GL_COLOR_BUFFER_BIT);
	GLimp_EndFrame();
}

#define R_MODE_FALLBACK 3 // 640 * 480

/**
 * @brief This routine is responsible for initializing the OS specific portions of OpenGL
 */
void GLimp_Init(void)
{
	r_allowSoftwareGL = ri.Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH);
	r_allowResize     = ri.Cvar_Get("r_allowResize", "0", CVAR_ARCHIVE);
	r_centerWindow    = ri.Cvar_Get("r_centerWindow", "0", CVAR_ARCHIVE);

	if (ri.Cvar_VariableIntegerValue("com_abnormalExit"))
	{
		ri.Cvar_Set("r_mode", va("%d", R_MODE_FALLBACK));
		ri.Cvar_Set("r_fullscreen", "0");
		ri.Cvar_Set("r_centerWindow", "0");
		ri.Cvar_Set("com_abnormalExit", "0");
	}

	ri.Sys_GLimpInit();

	// Create the window and set up the context
	if (GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, r_noborder->integer))
	{
		goto success;
	}

	// Try again, this time in a platform specific "safe mode"
	ri.Sys_GLimpSafeInit();

	if (GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, qfalse))
	{
		goto success;
	}

	// Finally, try the default screen resolution
	if (r_mode->integer != R_MODE_FALLBACK)
	{
		Ren_Print("Setting r_mode %d failed, falling back on r_mode %d\n",
		          r_mode->integer, R_MODE_FALLBACK);

		if (GLimp_StartDriverAndSetMode(R_MODE_FALLBACK, qfalse, qfalse))
		{
			goto success;
		}
	}

	// Nothing worked, give up
	Ren_Fatal("GLimp_Init() - could not load OpenGL subsystem\n");

success:

	//Clear the screen with a black color thanks
	Glimp_ClearScreen();

#ifdef FEATURE_RENDERER2
	if (glConfig.driverType != GLDRV_OPENGL3)
	{
		glConfig.driverType = GLDRV_ICD;
	}
#else
	// This values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
#endif
	glConfig.hardwareType = GLHW_GENERIC;

	// Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
	glConfig.deviceSupportsGamma = !r_ignorehwgamma->integer &&
	                               SDL_SetWindowBrightness(main_window, 1.0f) >= 0;

	// Get extension strings
	if (glConfig.driverType != GLDRV_OPENGL3)
	{
		Q_strncpyz(glConfig.extensions_string, ( char * ) glGetString(GL_EXTENSIONS), sizeof(glConfig.extensions_string));
	}

#ifndef FEATURE_RENDERER_GLES
	else
	{
		int i = 0, exts = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &exts);
		glConfig.extensions_string[0] = 0;
		for (i = 0; i < exts; i++)
		{
			if (strlen(glConfig.extensions_string) + 100 >= sizeof(glConfig.extensions_string))
			{
				//Just so we wont error out when there are really a lot of extensions
				break;
			}

			Q_strcat(glConfig.extensions_string, sizeof(glConfig.extensions_string), va("%s ", glGetStringi(GL_EXTENSIONS, i)));
		}
	}
#endif // FEATURE_RENDERER_GLES

	// initialize extensions
	GLimp_SetHardware();
#ifdef FEATURE_RENDERER2
	GLimp_InitExtensionsR2(); // renderer2
#else
	GLimp_InitExtensions(); // vanilla renderer
#endif

	ri.Cvar_Get("r_availableModes", "", CVAR_ROM);

	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init();
}

/**
 * @brief Responsible for doing a swapbuffers
 */
void GLimp_EndFrame(void)
{
	// don't flip if drawing to front buffer
	if (Q_stricmp(r_drawBuffer->string, "GL_FRONT") != 0)
	{
		SDL_GL_SwapWindow(main_window);
	}

	if (r_fullscreen->modified)
	{
		qboolean fullscreen;
		qboolean needToToggle;

		// Find out the current state
		fullscreen = !!(SDL_GetWindowFlags(main_window) & SDL_WINDOW_FULLSCREEN);

		if (r_fullscreen->integer && ri.Cvar_VariableIntegerValue("in_nograb"))
		{
			Ren_Print("Fullscreen not allowed with in_nograb 1\n");
			ri.Cvar_Set("r_fullscreen", "0");
			r_fullscreen->modified = qfalse;
		}

		// Is the state we want different from the current state?
		needToToggle = !!r_fullscreen->integer != fullscreen;

		if (needToToggle)
		{
			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if (!(SDL_SetWindowFullscreen(main_window, r_fullscreen->integer) >= 0)) // !sdlToggled
			{
				ri.Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");
			}

			ri.IN_Restart();
		}

#ifdef MACOS_X_GAMMA_RESET_FIX
		// OS X 10.9 has a bug where toggling in or out of fullscreen mode
		// will cause the gamma to reset to the system default after an unknown
		// short delay. This little fix simply causes the gamma to be reset
		// again after a hopefully-long-enough-delay of 3 seconds.
		// Radar 15961845
		gammaResetTime = ri.Milliseconds() + 3000;
#endif
		r_fullscreen->modified = qfalse;
	}

#ifdef MACOS_X_GAMMA_RESET_FIX
	if ((gammaResetTime != 0) && (gammaResetTime < ri.Milliseconds()))
	{
		// Circuitous way of resetting the gamma to its current value.
		char old[6] = { 0 };
		Q_strncpyz(old, va("%i", ri.Cvar_VariableIntegerValue("r_gamma")), 5);
		if (strlen(old))
		{
			ri.Cvar_Set("r_gamma", "1");
			ri.Cvar_Set("r_gamma", old);
		}

		gammaResetTime = 0;
	}
#endif
}
