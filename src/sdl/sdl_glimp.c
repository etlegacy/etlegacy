/**
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
 * @file sdl_glimp.c
 */

#include "sdl_defs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef FEATURE_RENDERER2
#include "../renderer2/tr_local.h"
#else
#include "../renderer/tr_local.h"
#endif

#include "../sys/sys_local.h"
#include "sdl_icon.h"

/* HACK: Just hack it for now. */
#if defined(WIN32)
#include <GL/wglew.h>
#else
#if !defined(FEATURE_RENDERER_GLES) && !defined(__AROS__) && !defined(__MORPHOS__)
#include <GL/glxew.h>
#endif
#endif

//static qboolean SDL_VIDEODRIVER_externallySet = qfalse;

#ifdef __APPLE__
#define MACOS_X_GAMMA_RESET_FIX
#ifdef MACOS_X_GAMMA_RESET_FIX
static int gammaResetTime = 0;
#endif
#endif // __APPLE__

#ifdef FEATURE_RENDERER_GLES
#include "eglport.h"
#endif

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_OLD_GL,

	RSERR_UNKNOWN
} rserr_t;

// @todo SDL 2.0 howto make screen available to cl_keys.c, etc. without extern
SDL_Window           *screen       = NULL;
static SDL_Renderer  *renderer     = NULL;
static SDL_GLContext SDL_glContext = NULL;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable
cvar_t *r_centerWindow;
cvar_t *r_sdlDriver;

void GLimp_Shutdown(void)
{
	ri.IN_Shutdown();

	if (renderer)
	{
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}

	if (screen)
	{
		SDL_DestroyWindow(screen);
		screen = NULL;
	}

#ifdef FEATURE_RENDERER_GLES
	EGL_Close();
#endif

	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	Com_Memset(&glConfig, 0, sizeof(glConfig));
	Com_Memset(&glState, 0, sizeof(glState));
}

void *GLimp_MainWindow(void)
{
	return screen;
}

/**
 * @brief Minimize the game so that user is back at the desktop
 */
void GLimp_Minimize(void)
{
	SDL_MinimizeWindow(screen);
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
	int      i;
	char     buf[MAX_STRING_CHARS] = { 0 };
	SDL_Rect modes[128];
	int      numModes = 0;

	int             display = SDL_GetWindowDisplayIndex(screen);
	SDL_DisplayMode windowMode;

	if (SDL_GetWindowDisplayMode(screen, &windowMode) < 0)
	{
		ri.Printf(PRINT_WARNING, "Couldn't get window display mode, no resolutions detected\n");
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
			ri.Printf(PRINT_ALL, "Display supports any resolution\n");
			return;
		}

		if (windowMode.format != mode.format)
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
			ri.Printf(PRINT_WARNING, "Skipping mode %ux%x, buffer too small\n", modes[i].w, modes[i].h);
		}
	}

	if (*buf)
	{
		buf[strlen(buf) - 1] = 0;
		Ren_Print("Available modes: '%s'\n", buf);
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
		if(!Q_stricmp(ext, glGetStringi(GL_EXTENSIONS, i)))
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

#ifdef FEATURE_RENDERER2
static qboolean GLimp_InitOpenGL3xContext()
{
	int GLmajor, GLminor;

	Ren_Print("Renderer: %s Version: %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

	sscanf(( const char * ) glGetString(GL_VERSION), "%d.%d", &GLmajor, &GLminor);

	if (GLmajor < 2)
	{
		// missing shader support, switch to 1.x renderer
		return qfalse;
	}

	if (GLmajor < 3 || (GLmajor == 3 && GLminor < 2))
	{
		// shaders are supported, but not all GL3.x features
		Ren_Print("Using enhanced (GL3) Renderer in GL 2.x mode...\n");
		return qtrue;
	}

	Ren_Print("Using enhanced (GL3) Renderer in GL 3.x mode...\n");
	glConfig.driverType = GLDRV_OPENGL3;

	return qtrue;
}

#define MSG_ERR_OLD_VIDEO_DRIVER                                                       \
	"\nET:Legacy with OpenGL 3.x renderer can not run on this "                             \
	"machine since it is missing one or more required OpenGL "                             \
	"extensions. Please update your video card drivers and try again.\n"

static void GLimp_InitExtensions2(void)
{
	//>char missingExts[4096];
	static char missingExts[60000];
	missingExts[0] = 0;

#define MissingEXT(x) Q_strcat(missingExts, sizeof(missingExts), x)

	Ren_Print("Initializing OpenGL extensions\n");
	Ren_Print("Extensions %s\n", glConfig.extensions_string);

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
				MissingEXT("GL_ARB_multitexture\n");
				Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_multitexture, < 2 texture units");
			}
		}
		else
		{
			MissingEXT("GL_ARB_multitexture\n");
			Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_multitexture");
		}
	}

	// GL_ARB_depth_texture in core

	if (GL_CheckForExtension("GL_ARB_texture_cube_map"))
	{
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig2.maxCubeMapTextureSize);
		Ren_Print("...found OpenGL extension - GL_ARB_texture_cube_map\n");
	}
	else
	{
		MissingEXT("GL_ARB_texture_cube_map\n");
		Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_texture_cube_map");
	}
	GL_CheckErrors();

	// GL_ARB_vertex_program duh

	if (GL_CheckForExtension("GL_ARB_vertex_buffer_object"))
	{
		Ren_Print("...found OpenGL extension - GL_ARB_vertex_buffer_object\n");
	}
	else
	{
		MissingEXT("GL_ARB_vertex_buffer_object\n");
		Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_vertex_buffer_object");
	}

	// GL_ARB_occlusion_query
	glConfig2.occlusionQueryAvailable = qfalse;
	glConfig2.occlusionQueryBits      = 0;
	if (GL_CheckForExtension("GL_ARB_occlusion_query"))
	{
		if (r_ext_occlusion_query->value)
		{
			glConfig2.occlusionQueryAvailable = qtrue;
			glGetQueryivARB(GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &glConfig2.occlusionQueryBits);
			Ren_Print("...found OpenGL extension - GL_ARB_occlusion_query\n");
		}
		else
		{
			Ren_Print("...ignoring GL_ARB_occlusion_query\n");
		}
	}
	else
	{
		Ren_Print("...GL_ARB_occlusion_query not found\n");
	}
	GL_CheckErrors();

	// GL_ARB_shader_objects
	if (GL_CheckForExtension("GL_ARB_shader_objects"))
	{
		Ren_Print("...found OpenGL extension - GL_ARB_shader_objects\n");
	}
	else
	{
		MissingEXT("GL_ARB_shader_objects\n");
		Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_shader_objects");
	}

	// GL_ARB_vertex_shader
	if (GL_CheckForExtension("GL_ARB_vertex_shader"))
	{
		int reservedComponents;

		GL_CheckErrors();
		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig2.maxVertexUniforms); GL_CheckErrors();
		//glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &glConfig.maxVaryingFloats); GL_CheckErrors();
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig2.maxVertexAttribs); GL_CheckErrors();

		reservedComponents = 16 * 10; // approximation how many uniforms we have besides the bone matrices

		/*
		if(glConfig.driverType == GLDRV_MESA)
		{
		    // HACK
		    // restrict to number of vertex uniforms to 512 because of:
		    // xreal.x86_64: nv50_program.c:4181: nv50_program_validate_data: Assertion `p->param_nr <= 512' failed

		    glConfig2.maxVertexUniforms = Q_bound(0, glConfig2.maxVertexUniforms, 512);
		}
		*/

		glConfig2.maxVertexSkinningBones     = (int) Q_bound(0.0, (Q_max(glConfig2.maxVertexUniforms - reservedComponents, 0) / 16), MAX_BONES);
		glConfig2.vboVertexSkinningAvailable = (qboolean)(r_vboVertexSkinning->integer && ((glConfig2.maxVertexSkinningBones >= 12) ? qtrue : qfalse));

		Ren_Print("...found OpenGL extension - GL_ARB_vertex_shader\n");
	}
	else
	{
		MissingEXT("GL_ARB_vertex_shader\n");
		Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_vertex_shader");
	}
	GL_CheckErrors();

	// GL_ARB_fragment_shader
	if (GL_CheckForExtension("GL_ARB_fragment_shader"))
	{
		Ren_Print("...found OpenGL extension - GL_ARB_fragment_shader\n");
	}
	else
	{
		MissingEXT("GL_ARB_fragment_shader\n");
		Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_fragment_shader");
	}

	// GL_ARB_shading_language_100
	if (GL_CheckForExtension("GL_ARB_shading_language_100"))
	{
		Q_strncpyz(glConfig2.shadingLanguageVersion, (char *)glGetString(GL_SHADING_LANGUAGE_VERSION_ARB), sizeof(glConfig2.shadingLanguageVersion));
		sscanf(glConfig2.shadingLanguageVersion, "%d.%d", &glConfig2.glslMajorVersion, &glConfig2.glslMinorVersion);
		Ren_Print("...found OpenGL extension - GL_ARB_shading_language_100\n");
	}
	else
	{
		MissingEXT("GL_ARB_shading_language_100\n");
		Ren_Fatal(MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for: GL_ARB_shading_language_100");
	}
	GL_CheckErrors();

	// GL_ARB_texture_non_power_of_two
	glConfig2.textureNPOTAvailable = qfalse;
	if (GL_CheckForExtension("GL_ARB_texture_non_power_of_two"))
	{
		if (r_ext_texture_non_power_of_two->integer)
		{
			glConfig2.textureNPOTAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_ARB_texture_non_power_of_two\n");
		}
		else
		{
			Ren_Print("...ignoring GL_ARB_texture_non_power_of_two\n");
		}
	}
	else
	{
		Ren_Print("...GL_ARB_texture_non_power_of_two not found\n");
	}

	// GL_ARB_draw_buffers
	glConfig2.drawBuffersAvailable = qfalse;
	if (GL_CheckForExtension("GL_ARB_draw_buffers"))
	{
		glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &glConfig2.maxDrawBuffers);

		if (r_ext_draw_buffers->integer)
		{
			glConfig2.drawBuffersAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_ARB_draw_buffers\n");
		}
		else
		{
			Ren_Print("...ignoring GL_ARB_draw_buffers\n");
		}
	}
	else
	{
		Ren_Print("...GL_ARB_draw_buffers not found\n");
	}

	// GL_ARB_half_float_pixel
	glConfig2.textureHalfFloatAvailable = qfalse;
	if (GL_CheckForExtension("GL_ARB_half_float_pixel"))
	{
		if (r_ext_half_float_pixel->integer)
		{
			glConfig2.textureHalfFloatAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_ARB_half_float_pixel\n");
		}
		else
		{
			Ren_Print("...ignoring GL_ARB_half_float_pixel\n");
		}
	}
	else
	{
		Ren_Print("...GL_ARB_half_float_pixel not found\n");
	}

	// GL_ARB_texture_float
	glConfig2.textureFloatAvailable = qfalse;
	if (GL_CheckForExtension("GL_ARB_texture_float"))
	{
		if (r_ext_texture_float->integer)
		{
			glConfig2.textureFloatAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_ARB_texture_float\n");
		}
		else
		{
			Ren_Print("...ignoring GL_ARB_texture_float\n");
		}
	}
	else
	{
		Ren_Print("...GL_ARB_texture_float not found\n");
	}

	// GL_ARB_texture_compression
	glConfig.textureCompression = TC_NONE;
	if (GL_CheckForExtension("GL_ARB_texture_compression"))
	{
		if (r_ext_compressed_textures->integer)
		{
			glConfig2.ARBTextureCompressionAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_ARB_texture_compression\n");
		}
		else
		{
			Ren_Print("...ignoring GL_ARB_texture_compression\n");
		}
	}
	else
	{
		Ren_Print("...GL_ARB_texture_compression not found\n");
	}

	// GL_ARB_vertex_array_object
	glConfig2.vertexArrayObjectAvailable = qfalse;
	if (GL_CheckForExtension("GL_ARB_vertex_array_object"))
	{
		if (r_ext_vertex_array_object->integer)
		{
			glConfig2.vertexArrayObjectAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_ARB_vertex_array_object\n");
		}
		else
		{
			Ren_Print("...ignoring GL_ARB_vertex_array_object\n");
		}
	}
	else
	{
		Ren_Print("...GL_ARB_vertex_array_object not found\n");
	}

	// GL_EXT_texture_compression_s3tc
	if (GL_CheckForExtension("GL_EXT_texture_compression_s3tc"))
	{
		if (r_ext_compressed_textures->integer)
		{
			glConfig.textureCompression = TC_S3TC;
			Ren_Print("...found OpenGL extension - GL_EXT_texture_compression_s3tc\n");
		}
		else
		{
			Ren_Print("...ignoring GL_EXT_texture_compression_s3tc\n");
		}
	}
	else
	{
		Ren_Print("...GL_EXT_texture_compression_s3tc not found\n");
	}

	// GL_EXT_texture3D
	glConfig2.texture3DAvailable = qfalse;
	if (GL_CheckForExtension("GL_EXT_texture3D"))
	{
		//if(r_ext_texture3d->value)
		{
			glConfig2.texture3DAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_EXT_texture3D\n");
		}
		/*
		else
		{
		    Ren_Print("...ignoring GL_EXT_texture3D\n");
		}
		*/
	}
	else
	{
		Ren_Print("...GL_EXT_texture3D not found\n");
	}

	// GL_EXT_stencil_wrap
	glConfig2.stencilWrapAvailable = qfalse;
	if (GL_CheckForExtension("GL_EXT_stencil_wrap"))
	{
		if (r_ext_stencil_wrap->value)
		{
			glConfig2.stencilWrapAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_EXT_stencil_wrap\n");
		}
		else
		{
			Ren_Print("...ignoring GL_EXT_stencil_wrap\n");
		}
	}
	else
	{
		Ren_Print("...GL_EXT_stencil_wrap not found\n");
	}

	// GL_EXT_texture_filter_anisotropic
	glConfig2.textureAnisotropyAvailable = qfalse;
	if (GL_CheckForExtension("GL_EXT_texture_filter_anisotropic"))
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig2.maxTextureAnisotropy);

		if (r_ext_texture_filter_anisotropic->value)
		{
			glConfig2.textureAnisotropyAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_EXT_texture_filter_anisotropic\n");
		}
		else
		{
			Ren_Print("...ignoring GL_EXT_texture_filter_anisotropic\n");
		}
	}
	else
	{
		Ren_Print("...GL_EXT_texture_filter_anisotropic not found\n");
	}
	GL_CheckErrors();

	// GL_EXT_stencil_two_side
	if (GL_CheckForExtension("GL_EXT_stencil_two_side"))
	{
		if (r_ext_stencil_two_side->value)
		{
			Ren_Print("...found OpenGL extension - GL_EXT_stencil_two_side\n");
		}
		else
		{
			Ren_Print("...ignoring GL_EXT_stencil_two_side\n");
		}
	}
	else
	{
		Ren_Print("...GL_EXT_stencil_two_side not found\n");
	}

	// GL_EXT_depth_bounds_test
	if (GL_CheckForExtension("GL_EXT_depth_bounds_test"))
	{
		if (r_ext_depth_bounds_test->value)
		{
			Ren_Print("...found OpenGL extension - GL_EXT_depth_bounds_test\n");
		}
		else
		{
			Ren_Print("...ignoring GL_EXT_depth_bounds_test\n");
		}
	}
	else
	{
		Ren_Print("...GL_EXT_depth_bounds_test not found\n");
	}

	// GL_EXT_framebuffer_object
	glConfig2.framebufferObjectAvailable = qfalse;
	if (GL_CheckForExtension("GL_EXT_framebuffer_object"))
	{
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &glConfig2.maxRenderbufferSize);
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &glConfig2.maxColorAttachments);

		if (r_ext_framebuffer_object->value)
		{
			glConfig2.framebufferObjectAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_EXT_framebuffer_object\n");
		}
		else
		{
			Ren_Print("...ignoring GL_EXT_framebuffer_object\n");
		}
	}
	else
	{
		Ren_Print("...GL_EXT_framebuffer_object not found\n");
	}
	GL_CheckErrors();

	// GL_EXT_packed_depth_stencil
	glConfig2.framebufferPackedDepthStencilAvailable = qfalse;
	if (GL_CheckForExtension("GL_EXT_packed_depth_stencil") && glConfig.driverType != GLDRV_MESA)
	{
		if (r_ext_packed_depth_stencil->integer)
		{
			glConfig2.framebufferPackedDepthStencilAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_EXT_packed_depth_stencil\n");
		}
		else
		{
			Ren_Print("...ignoring GL_EXT_packed_depth_stencil\n");
		}
	}
	else
	{
		Ren_Print("...GL_EXT_packed_depth_stencil not found\n");
	}

	// GL_EXT_framebuffer_blit
	glConfig2.framebufferBlitAvailable = qfalse;
	if (GL_CheckForExtension("GL_EXT_framebuffer_blit"))
	{
		if (r_ext_framebuffer_blit->integer)
		{
			glConfig2.framebufferBlitAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_EXT_framebuffer_blit\n");
		}
		else
		{
			Ren_Print("...ignoring GL_EXT_framebuffer_blit\n");
		}
	}
	else
	{
		Ren_Print("...GL_EXT_framebuffer_blit not found\n");
	}

	// GL_EXTX_framebuffer_mixed_formats
	/*
	glConfig2.framebufferMixedFormatsAvailable = qfalse;
	if(Q_stristr(glConfig.extensions_string, "GL_EXTX_framebuffer_mixed_formats")) // note: if you activate this use (char *) qglGetString(GL_EXTENSIONS)
	                                                                                        glConfig.extensions_string is unsafe
	{
	    if(r_extx_framebuffer_mixed_formats->integer)
	    {
	        glConfig2.framebufferMixedFormatsAvailable = qtrue;
	        Ren_Print("...using GL_EXTX_framebuffer_mixed_formats\n");
	    }
	    else
	    {
	        Ren_Print("...ignoring GL_EXTX_framebuffer_mixed_formats\n");
	    }
	}
	else
	{
	    Ren_Print("...GL_EXTX_framebuffer_mixed_formats not found\n");
	}
	*/

	// GL_ATI_separate_stencil
	if (GL_CheckForExtension("GL_ATI_separate_stencil"))
	{
		if (r_ext_separate_stencil->value)
		{
			Ren_Print("...found OpenGL extension - GL_ATI_separate_stencil\n");
		}
		else
		{
			Ren_Print("...ignoring GL_ATI_separate_stencil\n");
		}
	}
	else
	{
		Ren_Print("...GL_ATI_separate_stencil not found\n");
	}

	// GL_SGIS_generate_mipmap
	glConfig2.generateMipmapAvailable = qfalse;
	if (GL_CheckForExtension("GL_SGIS_generate_mipmap"))
	{
		if (r_ext_generate_mipmap->value)
		{
			glConfig2.generateMipmapAvailable = qtrue;
			Ren_Print("...found OpenGL extension - GL_SGIS_generate_mipmap\n");
		}
		else
		{
			Ren_Print("...ignoring GL_SGIS_generate_mipmap\n");
		}
	}
	else
	{
		Ren_Print("...GL_SGIS_generate_mipmap not found\n");
	}

	// GL_GREMEDY_string_marker
	if (GL_CheckForExtension("GL_GREMEDY_string_marker"))
	{
		Ren_Print("...found OpenGL extension - GL_GREMEDY_string_marker\n");
	}
	else
	{
		Ren_Print("...GL_GREMEDY_string_marker not found\n");
	}

	if (GL_CheckForExtension("GL_ARB_get_program_binary"))
	{
		int formats = 0;

		glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);

		if (!formats)
		{
			Ren_Print("...GL_ARB_get_program_binary found, but with no binary formats\n");
			glConfig2.getProgramBinaryAvailable = qfalse;
		}
		else
		{
			Ren_Print("...using GL_ARB_get_program_binary\n");
			glConfig2.getProgramBinaryAvailable = qtrue;
		}
	}
	else
	{
		Ren_Print("...GL_ARB_get_program_binary not found\n");
		glConfig2.getProgramBinaryAvailable = qfalse;
	}
}
#endif

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
#ifdef FEATURE_RENDERER_GLES
	Uint32 flags = 0;
#else
	Uint32 flags = SDL_WINDOW_OPENGL;
#endif
	GLenum glewResult;

	Ren_Print("Initializing OpenGL display\n");

	if (r_allowResize->integer)
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
	if (screen != NULL)
	{
		display = SDL_GetWindowDisplayIndex(screen);
	}

	if (SDL_GetDesktopDisplayMode(display, &desktopMode) == 0)
	{
		displayAspect = (float)desktopMode.w / (float)desktopMode.h;

		ri.Printf(PRINT_ALL, "Estimated display aspect: %.3f\n", displayAspect);
	}
	else
	{
		Com_Memset(&desktopMode, 0, sizeof(SDL_DisplayMode));

		ri.Printf(PRINT_ALL,
		          "Cannot estimate display aspect, assuming 1.333\n");
	}

	GLimp_DetectAvailableModes();

	Ren_Print("...setting mode %d: ", mode);

#ifdef PANDORA
	glConfig.vidWidth     = 800;
	glConfig.vidHeight    = 480;
	glConfig.windowAspect = 800.0 / 480.0;
#else
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
			Ren_Print(
			    "Cannot determine display resolution, assuming 640x480\n");
		}

		glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}
	else if (!R_GetModeInfo(&glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode))
	{
		Ren_Print("invalid mode\n");
		return RSERR_INVALID_MODE;
	}
#endif
	Ren_Print("%dx%d\n", glConfig.vidWidth, glConfig.vidHeight);

#ifdef PANDORA
	flags                |= SDL_FULLSCREEN;
	glConfig.isFullscreen = qtrue;
#else

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

	if (screen != NULL)
	{
		SDL_GetWindowPosition(screen, &x, &y);
		ri.Printf(PRINT_DEVELOPER, "Existing window at %dx%d before being destroyed\n", x, y);
		SDL_DestroyWindow(screen);
		screen = NULL;
	}

	if (fullscreen)
	{
		flags                |= SDL_WINDOW_FULLSCREEN;
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
#endif

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

#ifndef FEATURE_RENDERER_GLES
#ifdef __sgi /* Fix for SGIs grabbing too many bits of color */
		if (perChannelColorBits == 4)
		{
			perChannelColorBits = 0; /* Use minimum size for 16-bit color */

		}
		/* Need alpha or else SGIs choose 36+ bit RGB mode */
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
#endif

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, testDepthBits);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, testStencilBits);

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);

		// SDL 2 uses opengl by default, if we want opengl es we need to set this attribute
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

		if ((screen = SDL_CreateWindow(CLIENT_WINDOW_TITLE, x, y,
		                               glConfig.vidWidth, glConfig.vidHeight, flags | SDL_WINDOW_SHOWN)) == 0)
		{
			ri.Printf(PRINT_DEVELOPER, "SDL_CreateWindow failed: %s\n", SDL_GetError());
			continue;
		}

#endif // FEATURE_RENDERER_GLES

		// We must call SDL_CreateRenderer in order for draw calls to affect this window.
		renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);

		if (!renderer)
		{
			ri.Printf(PRINT_DEVELOPER, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
			continue;
		}

		if (fullscreen)
		{
			SDL_DisplayMode mode;

			switch (testColorBits)
			{
			case 16: mode.format = SDL_PIXELFORMAT_RGB565; break;
			case 24: mode.format = SDL_PIXELFORMAT_RGB24;  break;
			default: ri.Printf(PRINT_DEVELOPER, "testColorBits is %d, can't fullscreen\n", testColorBits); continue;
			}

			mode.w            = glConfig.vidWidth;
			mode.h            = glConfig.vidHeight;
			mode.refresh_rate = glConfig.displayFrequency = ri.Cvar_VariableIntegerValue("r_displayRefresh");
			mode.driverdata   = NULL;

			if (SDL_SetWindowDisplayMode(screen, &mode) < 0)
			{
				ri.Printf(PRINT_DEVELOPER, "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError());
				continue;
			}
		}

		SDL_SetWindowIcon(screen, icon);

#ifdef FEATURE_RENDERER_GLES
		EGL_Open(glConfig.vidWidth, glConfig.vidHeight);
		sdlcolorbits    = eglColorbits;
		testDepthBits   = eglDepthbits;
		testStencilBits = eglStencilbits;
#endif

#ifdef FEATURE_RENDERER2
		glewExperimental = GL_TRUE;

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

		if ((SDL_glContext = SDL_GL_CreateContext(screen)) == NULL)
		{
			ri.Printf(PRINT_DEVELOPER, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
			continue;
		}

		SDL_GL_SetSwapInterval(r_swapInterval->integer);

		glConfig.colorBits   = testColorBits;
		glConfig.depthBits   = testDepthBits;
		glConfig.stencilBits = testStencilBits;

		ri.Printf(PRINT_ALL, "Using %d color bits, %d depth, %d stencil display.\n",
		          glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
		break;
	}

#if !defined(FEATURE_RENDERER_GLES) && !defined(__MORPHOS__)
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

#ifdef FEATURE_RENDERER2
	if (!GLimp_InitOpenGL3xContext())
	{
		return RSERR_OLD_GL;
	}
#endif

	if (!screen || !renderer)
	{
		Ren_Print("Couldn't get a visual\n");
		return RSERR_INVALID_MODE;
	}

	SDL_FreeSurface(icon);

	GLimp_DetectAvailableModes();

	ri.Printf(PRINT_ALL, "GL_RENDERER: %s\n", (char *) qglGetString(GL_RENDERER));

	return RSERR_OK;
}

static qboolean GLimp_StartDriverAndSetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	rserr_t err;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		const char *driverName;

		if (LegacySDL_Init(SDL_INIT_VIDEO) == -1)
		{
			Ren_Print("SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n",
			          SDL_GetError());
			return qfalse;
		}

		driverName = SDL_GetCurrentVideoDriver();
		Ren_Print("SDL using driver \"%s\"\n", driverName);
		ri.Cvar_Set("r_sdlDriver", driverName);
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

#if !defined(FEATURE_RENDERER_GLES) && !defined(__MORPHOS__)
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

#if !defined(FEATURE_RENDERER_GLES) && !defined(__MORPHOS__)
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
#elif defined(__MORPHOS__)
	Ren_Print("...GL_EXT_texture_env_add not found\n");
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
#if defined(FEATURE_RENDERER_GLES) || defined(__MORPHOS__)
	GLint glint = 0;
	qglGetIntegerv(GL_MAX_TEXTURE_UNITS, &glint);
	glConfig.maxActiveTextures = (int)glint;
	//ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, %i texture units\n", glConfig.maxActiveTextures );
	//glConfig.maxActiveTextures=4;
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

void GLimp_SetHardware(void)
{
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
	{
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	}

	Q_strncpyz(glConfig.version_string, ( char * ) glGetString(GL_VERSION), sizeof(glConfig.version_string));

	if (glConfig.driverType != GLDRV_OPENGL3)
	{
		Q_strncpyz(glConfig.extensions_string, ( char * ) glGetString(GL_EXTENSIONS), sizeof(glConfig.extensions_string));
	}

	if (Q_stristr(glConfig.renderer_string, "mesa") ||
	    Q_stristr(glConfig.renderer_string, "gallium") ||
	    Q_stristr(glConfig.vendor_string, "nouveau") ||
	    Q_stristr(glConfig.vendor_string, "mesa"))
	{
		// suckage
		glConfig.driverType = GLDRV_MESA;
	}

	if (Q_stristr(glConfig.renderer_string, "geforce"))
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
		    Q_stristr(glConfig.renderer_string, "gtx 690"))
		{
			glConfig.hardwareType = GLHW_NV_DX10;
		}

	}
	else if (Q_stristr(glConfig.renderer_string, "quadro fx"))
	{
		if (Q_stristr(glConfig.renderer_string, "3600"))
		{
			glConfig.hardwareType = GLHW_NV_DX10;
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
}

#ifdef PANDORA
#define R_MODE_FALLBACK 11 // 800 * 480
#else
#define R_MODE_FALLBACK 3 // 640 * 480
#endif

/**
 * @brief This routine is responsible for initializing the OS specific portions of OpenGL
 */
void GLimp_Init(void)
{
	r_allowSoftwareGL = ri.Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH);
	r_sdlDriver       = ri.Cvar_Get("r_sdlDriver", "", CVAR_ROM);
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
	                               SDL_SetWindowBrightness(screen, 1.0f) >= 0;

	// get our config strings

	Q_strncpyz(glConfig.vendor_string, (char *) qglGetString(GL_VENDOR), sizeof(glConfig.vendor_string));
	Q_strncpyz(glConfig.renderer_string, (char *) qglGetString(GL_RENDERER), sizeof(glConfig.renderer_string));
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
	{
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	}
	Q_strncpyz(glConfig.version_string, (char *) qglGetString(GL_VERSION), sizeof(glConfig.version_string));
	if (glConfig.driverType != GLDRV_OPENGL3)
	{
		Q_strncpyz(glConfig.extensions_string, ( char * ) glGetString(GL_EXTENSIONS), sizeof(glConfig.extensions_string));
	}
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

#ifdef FEATURE_RENDERER2
	GLimp_SetHardware();
	GLimp_InitExtensions2(); // renderer2
#else
	// initialize extensions
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
#ifdef FEATURE_RENDERER_GLES
	EGL_SwapBuffers();
#else
	// don't flip if drawing to front buffer
	if (Q_stricmp(r_drawBuffer->string, "GL_FRONT") != 0)
	{
		SDL_GL_SwapWindow(screen);
	}
#endif

	if (r_fullscreen->modified)
	{
		qboolean fullscreen;
		qboolean needToToggle, sdlToggled = qfalse;

		// Find out the current state
		fullscreen = !!(SDL_GetWindowFlags(screen) & SDL_WINDOW_FULLSCREEN);

		if (r_fullscreen->integer && ri.Cvar_VariableIntegerValue("in_nograb"))
		{
			ri.Printf(PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
			ri.Cvar_Set("r_fullscreen", "0");
			r_fullscreen->modified = qfalse;
		}

		// Is the state we want different from the current state?
		needToToggle = !!r_fullscreen->integer != fullscreen;

		if (needToToggle)
		{
			sdlToggled = SDL_SetWindowFullscreen(screen, r_fullscreen->integer) >= 0;

			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if (!sdlToggled)
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
