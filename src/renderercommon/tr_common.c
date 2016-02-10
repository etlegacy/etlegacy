/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2016 ET:Legacy team <mail@etlegacy.com>
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
 * @file tr_common.c
 */

#include "tr_common.h"

#if defined(FEATURE_RENDERER2)
#   include "../renderer2/tr_local.h"
#elif defined(FEATURE_RENDERER_GLES)
#   include "../rendererGLES/tr_local.h"
#else // OpenGL 2 renderer
#   include "../renderer/tr_local.h"
#endif

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

	Com_Printf("GL_VENDOR: %s\n", glConfig.vendor_string);
	Com_Printf("GL_RENDERER: %s\n", glConfig.renderer_string);
	Com_Printf("GL_VERSION: %s\n", glConfig.version_string);

#ifndef FEATURE_RENDERER2
	Com_Printf("Using vanilla renderer\n");
#else
	// get shading language version
	Q_strncpyz(glConfig2.shadingLanguageVersion, (char *)glGetString(GL_SHADING_LANGUAGE_VERSION), sizeof(glConfig2.shadingLanguageVersion));
	sscanf(glConfig2.shadingLanguageVersion, "%d.%d", &glConfig2.glslMajorVersion, &glConfig2.glslMinorVersion);
	Com_Printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glConfig2.shadingLanguageVersion);

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
		Com_Printf("Using enhanced renderer in GL 2.x mode\n");
		return qtrue;
	}

	Com_Printf("Using enhanced renderer in GL 3.x mode\n");
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
		Com_Printf("...found OpenGL extension - %s\n", ext);
	}
	else
	{
		if (var)
		{
			Com_Printf("...ignoring %s\n", ext);
		}
		else
		{
			Com_Printf("...%s not found\n", ext);
		}
	}

	return result;
}

static void GLimp_InitExtensionsR2(void)
{
	Com_Printf("Initializing OpenGL extensions\n");

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
		glConfig.textureCompression = TC_S3TC_ARB;
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
	if (GLimp_CheckForVersionExtension("GL_EXT_packed_depth_stencil", 300, qfalse, r_ext_packed_depth_stencil))
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

#ifndef FEATURE_RENDERER2

static void GLimp_InitExtensions(void)
{
	if (!r_allowExtensions->integer)
	{
		Com_Printf("* IGNORING OPENGL EXTENSIONS *\n");
		return;
	}

	Com_Printf("Initializing OpenGL extensions\n");

	glConfig.textureCompression = TC_NONE;

#if !defined(FEATURE_RENDERER_GLES)
	// GL_EXT_texture_compression_s3tc
	if (GLEW_ARB_texture_compression &&
	    GLEW_EXT_texture_compression_s3tc)
	{
		if (r_ext_compressed_textures->value)
		{
			glConfig.textureCompression = TC_S3TC_ARB;
			Com_Printf("...found OpenGL extension - GL_EXT_texture_compression_s3tc\n");
		}
		else
		{
			Com_Printf("...ignoring GL_EXT_texture_compression_s3tc\n");
		}
	}
	else
#endif
	{
		Com_Printf("...GL_EXT_texture_compression_s3tc not found\n");
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
				Com_Printf("...found OpenGL extension - GL_S3_s3tc\n");
			}
			else
			{
				Com_Printf("...ignoring GL_S3_s3tc\n");
			}
		}
		else
		{
			Com_Printf("...GL_S3_s3tc not found\n");
		}
	}
#endif

	// GL_EXT_texture_env_add
#ifdef FEATURE_RENDERER_GLES
	glConfig.textureEnvAddAvailable = qtrue;
	Com_Printf("...using GL_EXT_texture_env_add\n");
#else
	glConfig.textureEnvAddAvailable = qfalse;
	if (GLEW_EXT_texture_env_add)
	{
		if (r_ext_texture_env_add->integer)
		{
			glConfig.textureEnvAddAvailable = qtrue;
			Com_Printf("...found OpenGL extension - GL_EXT_texture_env_add\n");
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			Com_Printf("...ignoring GL_EXT_texture_env_add\n");
		}
	}
	else
	{
		Com_Printf("...GL_EXT_texture_env_add not found\n");
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
		Com_Printf("...using GL_ARB_multitexture (%i texture units)\n", glConfig.maxActiveTextures);
	}
	else
	{
		Com_Printf("...not using GL_ARB_multitexture, < 2 texture units\n");
	}
#elif defined(FEATURE_RENDERER2)
	glConfig.maxActiveTextures = 32;
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
				Com_Printf("...found OpenGL extension - GL_ARB_multitexture\n");
			}
			else
			{
				Com_Printf("...not using GL_ARB_multitexture, < 2 texture units\n");
			}
		}
		else
		{
			Com_Printf("...ignoring GL_ARB_multitexture\n");
		}
	}
	else
	{
		Com_Printf("...GL_ARB_multitexture not found\n");
	}
#endif
}
#endif

void Glimp_ClearScreen(void)
{
	qglClearColor(0, 0, 0, 1);
	qglClear(GL_COLOR_BUFFER_BIT);
	ri.GLimp_SwapFrame();
}

int RE_InitOpenGlSubsystems(void)
{
#ifndef FEATURE_RENDERER_GLES
	GLenum glewResult;
#endif

#if !defined(FEATURE_RENDERER_GLES)

#if defined(FEATURE_RENDERER2)
	glewExperimental = GL_TRUE;
#endif

	glewResult = glewInit();

	if (GLEW_OK != glewResult)
	{
		// glewInit failed, something is seriously wrong
		Ren_Fatal("GLW_StartOpenGL() - could not load OpenGL subsystem: %s", glewGetErrorString(glewResult));
	}
	else
	{
		Com_Printf("Using GLEW %s\n", glewGetString(GLEW_VERSION));
	}
#endif

	if (!GLimp_InitOpenGLContext())
	{
		return qfalse;
	}

	return qtrue;
}

void RE_InitOpenGl(void)
{
	//Clear the screen with a black color thanks
	Glimp_ClearScreen();

	glConfig.driverType   = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	// Get extension strings
#ifndef FEATURE_RENDERER2
	Q_strncpyz(glConfig.extensions_string, ( char * ) glGetString(GL_EXTENSIONS), sizeof(glConfig.extensions_string));
#else
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
#endif

	// initialize extensions
#ifdef FEATURE_RENDERER2
	GLimp_InitExtensionsR2(); // renderer2
#else
	GLimp_InitExtensions(); // vanilla renderer
#endif
}

void R_DoGLimpShutdown(void)
{
	ri.GLimp_Shutdown();
	Com_Memset(&glConfig, 0, sizeof(glConfig));
	Com_Memset(&glState, 0, sizeof(glState));
}

#ifdef USE_RENDERER_DLOPEN
void QDECL Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	Ren_Print("%s", text);
}

void QDECL Com_DPrintf(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	Ren_Developer("%s", text);
}

void QDECL Com_Error(int level, const char *error, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Error(level, "%s", text);
}
#endif
