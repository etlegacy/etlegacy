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

// tr_extensions.c - extensions needed by the renderer not in sdl_glimp.c

#ifdef BUNDLED_SDL
#    include "SDL.h"
#else
#    include <SDL/SDL.h>
#endif

#include "tr_local.h"

static qboolean GLimp_HaveExtension(const char *ext)
{
	const char *ptr = Q_stristr(glConfig.extensions_string, ext);
	if (ptr == NULL)
	{
		return qfalse;
	}
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}

void GLimp_InitExtraExtensions()
{
	char       *extension;
	const char *result[3] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };

	// GL_EXT_draw_range_elements
	extension                     = "GL_EXT_draw_range_elements";
	glRefConfig.drawRangeElements = qfalse;
	qglMultiDrawArraysEXT         = NULL;
	qglMultiDrawElementsEXT       = NULL;
	if (GLimp_HaveExtension(extension))
	{
		qglDrawRangeElementsEXT = (void *) SDL_GL_GetProcAddress("glDrawRangeElementsEXT");

		if (r_ext_draw_range_elements->integer)
		{
			glRefConfig.drawRangeElements = qtrue;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.drawRangeElements], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_multi_draw_arrays
	extension                   = "GL_EXT_multi_draw_arrays";
	glRefConfig.multiDrawArrays = qfalse;
	qglMultiDrawArraysEXT       = NULL;
	qglMultiDrawElementsEXT     = NULL;
	if (GLimp_HaveExtension(extension))
	{
		qglMultiDrawArraysEXT   = (PFNGLMULTIDRAWARRAYSEXTPROC) SDL_GL_GetProcAddress("glMultiDrawArraysEXT");
		qglMultiDrawElementsEXT = (PFNGLMULTIDRAWELEMENTSEXTPROC) SDL_GL_GetProcAddress("glMultiDrawElementsEXT");

		if (r_ext_multi_draw_arrays->integer)
		{
			glRefConfig.multiDrawArrays = qtrue;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.multiDrawArrays], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_vertex_program
	//glRefConfig.vertexProgram = qfalse;
	extension                      = "GL_ARB_vertex_program";
	qglVertexAttrib4fARB           = NULL;
	qglVertexAttrib4fvARB          = NULL;
	qglVertexAttribPointerARB      = NULL;
	qglEnableVertexAttribArrayARB  = NULL;
	qglDisableVertexAttribArrayARB = NULL;
	if (GLimp_HaveExtension(extension))
	{
		qglVertexAttrib4fARB          = (PFNGLVERTEXATTRIB4FARBPROC) SDL_GL_GetProcAddress("glVertexAttrib4fARB");
		qglVertexAttrib4fvARB         = (PFNGLVERTEXATTRIB4FVARBPROC) SDL_GL_GetProcAddress("glVertexAttrib4fvARB");
		qglVertexAttribPointerARB     = (PFNGLVERTEXATTRIBPOINTERARBPROC) SDL_GL_GetProcAddress("glVertexAttribPointerARB");
		qglEnableVertexAttribArrayARB =
		    (PFNGLENABLEVERTEXATTRIBARRAYARBPROC) SDL_GL_GetProcAddress("glEnableVertexAttribArrayARB");
		qglDisableVertexAttribArrayARB =
		    (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) SDL_GL_GetProcAddress("glDisableVertexAttribArrayARB");

		ri.Printf(PRINT_ALL, result[1], extension);
		//glRefConfig.vertexProgram = qtrue;
	}
	else
	{
		ri.Error(ERR_FATAL, result[2], extension);
	}

	// GL_ARB_vertex_buffer_object
	//glRefConfig.vertexBufferObject = qfalse;
	extension                  = "GL_ARB_vertex_buffer_object";
	qglBindBufferARB           = NULL;
	qglDeleteBuffersARB        = NULL;
	qglGenBuffersARB           = NULL;
	qglIsBufferARB             = NULL;
	qglBufferDataARB           = NULL;
	qglBufferSubDataARB        = NULL;
	qglGetBufferSubDataARB     = NULL;
	qglGetBufferParameterivARB = NULL;
	qglGetBufferPointervARB    = NULL;
	if (GLimp_HaveExtension(extension))
	{
		qglBindBufferARB           = (PFNGLBINDBUFFERARBPROC) SDL_GL_GetProcAddress("glBindBufferARB");
		qglDeleteBuffersARB        = (PFNGLDELETEBUFFERSARBPROC) SDL_GL_GetProcAddress("glDeleteBuffersARB");
		qglGenBuffersARB           = (PFNGLGENBUFFERSARBPROC) SDL_GL_GetProcAddress("glGenBuffersARB");
		qglIsBufferARB             = (PFNGLISBUFFERARBPROC) SDL_GL_GetProcAddress("glIsBufferARB");
		qglBufferDataARB           = (PFNGLBUFFERDATAARBPROC) SDL_GL_GetProcAddress("glBufferDataARB");
		qglBufferSubDataARB        = (PFNGLBUFFERSUBDATAARBPROC) SDL_GL_GetProcAddress("glBufferSubDataARB");
		qglGetBufferSubDataARB     = (PFNGLGETBUFFERSUBDATAARBPROC) SDL_GL_GetProcAddress("glGetBufferSubDataARB");
		qglGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC) SDL_GL_GetProcAddress("glGetBufferParameterivARB");
		qglGetBufferPointervARB    = (PFNGLGETBUFFERPOINTERVARBPROC) SDL_GL_GetProcAddress("glGetBufferPointervARB");
		ri.Printf(PRINT_ALL, result[1], extension);
		//glRefConfig.vertexBufferObject = qtrue;
	}
	else
	{
		ri.Error(ERR_FATAL, result[2], extension);
	}

	// GL_ARB_shader_objects
	extension = "GL_ARB_shader_objects";
	//glRefConfig.shaderObjects = qfalse;
	qglDeleteObjectARB         = NULL;
	qglGetHandleARB            = NULL;
	qglDetachObjectARB         = NULL;
	qglCreateShaderObjectARB   = NULL;
	qglShaderSourceARB         = NULL;
	qglCompileShaderARB        = NULL;
	qglCreateProgramObjectARB  = NULL;
	qglAttachObjectARB         = NULL;
	qglLinkProgramARB          = NULL;
	qglUseProgramObjectARB     = NULL;
	qglValidateProgramARB      = NULL;
	qglUniform1fARB            = NULL;
	qglUniform2fARB            = NULL;
	qglUniform3fARB            = NULL;
	qglUniform4fARB            = NULL;
	qglUniform1iARB            = NULL;
	qglUniform2iARB            = NULL;
	qglUniform3iARB            = NULL;
	qglUniform4iARB            = NULL;
	qglUniform1fvARB           = NULL;
	qglUniform2fvARB           = NULL;
	qglUniform3fvARB           = NULL;
	qglUniform4fvARB           = NULL;
	qglUniform2ivARB           = NULL;
	qglUniform3ivARB           = NULL;
	qglUniform4ivARB           = NULL;
	qglUniformMatrix2fvARB     = NULL;
	qglUniformMatrix3fvARB     = NULL;
	qglUniformMatrix4fvARB     = NULL;
	qglGetObjectParameterfvARB = NULL;
	qglGetObjectParameterivARB = NULL;
	qglGetInfoLogARB           = NULL;
	qglGetAttachedObjectsARB   = NULL;
	qglGetUniformLocationARB   = NULL;
	qglGetActiveUniformARB     = NULL;
	qglGetUniformfvARB         = NULL;
	qglGetUniformivARB         = NULL;
	qglGetShaderSourceARB      = NULL;
	if (GLimp_HaveExtension(extension))
	{
		qglDeleteObjectARB         = (PFNGLDELETEOBJECTARBPROC) SDL_GL_GetProcAddress("glDeleteObjectARB");
		qglGetHandleARB            = (PFNGLGETHANDLEARBPROC) SDL_GL_GetProcAddress("glGetHandleARB");
		qglDetachObjectARB         = (PFNGLDETACHOBJECTARBPROC) SDL_GL_GetProcAddress("glDetachObjectARB");
		qglCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC) SDL_GL_GetProcAddress("glCreateShaderObjectARB");
		qglShaderSourceARB         = (PFNGLSHADERSOURCEARBPROC) SDL_GL_GetProcAddress("glShaderSourceARB");
		qglCompileShaderARB        = (PFNGLCOMPILESHADERARBPROC) SDL_GL_GetProcAddress("glCompileShaderARB");
		qglCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC) SDL_GL_GetProcAddress("glCreateProgramObjectARB");
		qglAttachObjectARB         = (PFNGLATTACHOBJECTARBPROC) SDL_GL_GetProcAddress("glAttachObjectARB");
		qglLinkProgramARB          = (PFNGLLINKPROGRAMARBPROC) SDL_GL_GetProcAddress("glLinkProgramARB");
		qglUseProgramObjectARB     = (PFNGLUSEPROGRAMOBJECTARBPROC) SDL_GL_GetProcAddress("glUseProgramObjectARB");
		qglValidateProgramARB      = (PFNGLVALIDATEPROGRAMARBPROC) SDL_GL_GetProcAddress("glValidateProgramARB");
		qglUniform1fARB            = (PFNGLUNIFORM1FARBPROC) SDL_GL_GetProcAddress("glUniform1fARB");
		qglUniform2fARB            = (PFNGLUNIFORM2FARBPROC) SDL_GL_GetProcAddress("glUniform2fARB");
		qglUniform3fARB            = (PFNGLUNIFORM3FARBPROC) SDL_GL_GetProcAddress("glUniform3fARB");
		qglUniform4fARB            = (PFNGLUNIFORM4FARBPROC) SDL_GL_GetProcAddress("glUniform4fARB");
		qglUniform1iARB            = (PFNGLUNIFORM1IARBPROC) SDL_GL_GetProcAddress("glUniform1iARB");
		qglUniform2iARB            = (PFNGLUNIFORM2IARBPROC) SDL_GL_GetProcAddress("glUniform2iARB");
		qglUniform3iARB            = (PFNGLUNIFORM3IARBPROC) SDL_GL_GetProcAddress("glUniform3iARB");
		qglUniform4iARB            = (PFNGLUNIFORM4IARBPROC) SDL_GL_GetProcAddress("glUniform4iARB");
		qglUniform1fvARB           = (PFNGLUNIFORM1FVARBPROC) SDL_GL_GetProcAddress("glUniform1fvARB");
		qglUniform2fvARB           = (PFNGLUNIFORM2FVARBPROC) SDL_GL_GetProcAddress("glUniform2fvARB");
		qglUniform3fvARB           = (PFNGLUNIFORM3FVARBPROC) SDL_GL_GetProcAddress("glUniform3fvARB");
		qglUniform4fvARB           = (PFNGLUNIFORM4FVARBPROC) SDL_GL_GetProcAddress("glUniform4fvARB");
		qglUniform2ivARB           = (PFNGLUNIFORM2IVARBPROC) SDL_GL_GetProcAddress("glUniform2ivARB");
		qglUniform3ivARB           = (PFNGLUNIFORM3IVARBPROC) SDL_GL_GetProcAddress("glUniform3ivARB");
		qglUniform4ivARB           = (PFNGLUNIFORM4IVARBPROC) SDL_GL_GetProcAddress("glUniform4ivARB");
		qglUniformMatrix2fvARB     = (PFNGLUNIFORMMATRIX2FVARBPROC) SDL_GL_GetProcAddress("glUniformMatrix2fvARB");
		qglUniformMatrix3fvARB     = (PFNGLUNIFORMMATRIX3FVARBPROC) SDL_GL_GetProcAddress("glUniformMatrix3fvARB");
		qglUniformMatrix4fvARB     = (PFNGLUNIFORMMATRIX4FVARBPROC) SDL_GL_GetProcAddress("glUniformMatrix4fvARB");
		qglGetObjectParameterfvARB = (PFNGLGETOBJECTPARAMETERFVARBPROC) SDL_GL_GetProcAddress("glGetObjectParameterfvARB");
		qglGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) SDL_GL_GetProcAddress("glGetObjectParameterivARB");
		qglGetInfoLogARB           = (PFNGLGETINFOLOGARBPROC) SDL_GL_GetProcAddress("glGetInfoLogARB");
		qglGetAttachedObjectsARB   = (PFNGLGETATTACHEDOBJECTSARBPROC) SDL_GL_GetProcAddress("glGetAttachedObjectsARB");
		qglGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC) SDL_GL_GetProcAddress("glGetUniformLocationARB");
		qglGetActiveUniformARB     = (PFNGLGETACTIVEUNIFORMARBPROC) SDL_GL_GetProcAddress("glGetActiveUniformARB");
		qglGetUniformfvARB         = (PFNGLGETUNIFORMFVARBPROC) SDL_GL_GetProcAddress("glGetUniformfvARB");
		qglGetUniformivARB         = (PFNGLGETUNIFORMIVARBPROC) SDL_GL_GetProcAddress("glGetUniformivARB");
		qglGetShaderSourceARB      = (PFNGLGETSHADERSOURCEARBPROC) SDL_GL_GetProcAddress("glGetShaderSourceARB");
		ri.Printf(PRINT_ALL, result[1], extension);
		//glRefConfig.shaderObjects = qtrue;
	}
	else
	{
		ri.Error(ERR_FATAL, result[2], extension);
	}

	// GL_ARB_vertex_shader
	//glRefConfig.vertexShader = qfalse;
	extension                = "GL_ARB_vertex_shader";
	qglBindAttribLocationARB = NULL;
	qglGetActiveAttribARB    = NULL;
	qglGetAttribLocationARB  = NULL;
	if (GLimp_HaveExtension(extension))
	{
		//int				reservedComponents;

		//qglGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig.maxVertexUniforms);
		//qglGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &glConfig.maxVaryingFloats);
		//qglGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig.maxVertexAttribs);

		//reservedComponents = 16 * 10; // approximation how many uniforms we have besides the bone matrices

#if 0
		if (glConfig.driverType == GLDRV_MESA)
		{
			// HACK
			// restrict to number of vertex uniforms to 512 because of:
			// xreal.x86_64: nv50_program.c:4181: nv50_program_validate_data: Assertion `p->param_nr <= 512' failed

			glConfig.maxVertexUniforms = Q_bound(0, glConfig.maxVertexUniforms, 512);
		}
#endif

		//glConfig.maxVertexSkinningBones = (int) Q_bound(0.0, (Q_max(glConfig.maxVertexUniforms - reservedComponents, 0) / 16), MAX_BONES);
		//glConfig.vboVertexSkinningAvailable = r_vboVertexSkinning->integer && ((glConfig.maxVertexSkinningBones >= 12) ? qtrue : qfalse);

		qglBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC) SDL_GL_GetProcAddress("glBindAttribLocationARB");
		qglGetActiveAttribARB    = (PFNGLGETACTIVEATTRIBARBPROC) SDL_GL_GetProcAddress("glGetActiveAttribARB");
		qglGetAttribLocationARB  = (PFNGLGETATTRIBLOCATIONARBPROC) SDL_GL_GetProcAddress("glGetAttribLocationARB");
		ri.Printf(PRINT_ALL, result[1], extension);
		//glRefConfig.vertexShader = qtrue;
	}
	else
	{
		ri.Error(ERR_FATAL, result[2], extension);
	}

	// GL_ARB_shading_language_100
	extension                = "GL_ARB_shading_language_100";
	glRefConfig.textureFloat = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		char version[256];

		Q_strncpyz(version, (char *) qglGetString(GL_SHADING_LANGUAGE_VERSION_ARB), sizeof(version));

		sscanf(version, "%d.%d", &glRefConfig.glslMajorVersion, &glRefConfig.glslMinorVersion);

		ri.Printf(PRINT_ALL, "...using GLSL version %s\n", version);
	}
	else
	{
		ri.Error(ERR_FATAL, result[2], extension);
	}

	glRefConfig.memInfo = MI_NONE;

	if (GLimp_HaveExtension("GL_NVX_gpu_memory_info"))
	{
		glRefConfig.memInfo = MI_NVX;
	}
	else if (GLimp_HaveExtension("GL_ATI_meminfo"))
	{
		glRefConfig.memInfo = MI_ATI;
	}

	extension                        = "GL_ARB_texture_non_power_of_two";
	glRefConfig.textureNonPowerOfTwo = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		if (1) //(r_ext_texture_non_power_of_two->integer)
		{
			glRefConfig.textureNonPowerOfTwo = qtrue;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.textureNonPowerOfTwo], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_texture_float
	extension                = "GL_ARB_texture_float";
	glRefConfig.textureFloat = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		if (r_ext_texture_float->integer)
		{
			glRefConfig.textureFloat = qtrue;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.textureFloat], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_half_float_pixel
	extension                  = "GL_ARB_half_float_pixel";
	glRefConfig.halfFloatPixel = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		if (r_arb_half_float_pixel->integer)
		{
			glRefConfig.halfFloatPixel = qtrue;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.halfFloatPixel], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_framebuffer_object
	extension                     = "GL_EXT_framebuffer_object";
	glRefConfig.framebufferObject = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &glRefConfig.maxRenderbufferSize);
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &glRefConfig.maxColorAttachments);

		qglIsRenderbufferEXT                      = (PFNGLISRENDERBUFFEREXTPROC) SDL_GL_GetProcAddress("glIsRenderbufferEXT");
		qglBindRenderbufferEXT                    = (PFNGLBINDRENDERBUFFEREXTPROC) SDL_GL_GetProcAddress("glBindRenderbufferEXT");
		qglDeleteRenderbuffersEXT                 = (PFNGLDELETERENDERBUFFERSEXTPROC) SDL_GL_GetProcAddress("glDeleteRenderbuffersEXT");
		qglGenRenderbuffersEXT                    = (PFNGLGENRENDERBUFFERSEXTPROC) SDL_GL_GetProcAddress("glGenRenderbuffersEXT");
		qglRenderbufferStorageEXT                 = (PFNGLRENDERBUFFERSTORAGEEXTPROC) SDL_GL_GetProcAddress("glRenderbufferStorageEXT");
		qglGetRenderbufferParameterivEXT          = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) SDL_GL_GetProcAddress("glGetRenderbufferParameterivEXT");
		qglIsFramebufferEXT                       = (PFNGLISFRAMEBUFFEREXTPROC) SDL_GL_GetProcAddress("glIsFramebufferEXT");
		qglBindFramebufferEXT                     = (PFNGLBINDFRAMEBUFFEREXTPROC) SDL_GL_GetProcAddress("glBindFramebufferEXT");
		qglDeleteFramebuffersEXT                  = (PFNGLDELETEFRAMEBUFFERSEXTPROC) SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
		qglGenFramebuffersEXT                     = (PFNGLGENFRAMEBUFFERSEXTPROC) SDL_GL_GetProcAddress("glGenFramebuffersEXT");
		qglCheckFramebufferStatusEXT              = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");
		qglFramebufferTexture1DEXT                = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) SDL_GL_GetProcAddress("glFramebufferTexture1DEXT");
		qglFramebufferTexture2DEXT                = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
		qglFramebufferTexture3DEXT                = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) SDL_GL_GetProcAddress("glFramebufferTexture3DEXT");
		qglFramebufferRenderbufferEXT             = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) SDL_GL_GetProcAddress("glFramebufferRenderbufferEXT");
		qglGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) SDL_GL_GetProcAddress("glGetFramebufferAttachmentParameterivEXT");
		qglGenerateMipmapEXT                      = (PFNGLGENERATEMIPMAPEXTPROC) SDL_GL_GetProcAddress("glGenerateMipmapEXT");

		if (r_ext_framebuffer_object->value)
		{
			glRefConfig.framebufferObject = qtrue;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.framebufferObject], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_packed_depth_stencil
	extension                      = "GL_EXT_packed_depth_stencil";
	glRefConfig.packedDepthStencil = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		glRefConfig.packedDepthStencil = qtrue;
		ri.Printf(PRINT_ALL, result[glRefConfig.packedDepthStencil], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_occlusion_query
	extension                  = "GL_ARB_occlusion_query";
	glRefConfig.occlusionQuery = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		qglGenQueriesARB           = (PFNGLGENQUERIESARBPROC) SDL_GL_GetProcAddress("glGenQueriesARB");
		qglDeleteQueriesARB        = (PFNGLDELETEQUERIESARBPROC) SDL_GL_GetProcAddress("glDeleteQueriesARB");
		qglIsQueryARB              = (PFNGLISQUERYARBPROC) SDL_GL_GetProcAddress("glIsQueryARB");
		qglBeginQueryARB           = (PFNGLBEGINQUERYARBPROC) SDL_GL_GetProcAddress("glBeginQueryARB");
		qglEndQueryARB             = (PFNGLENDQUERYARBPROC) SDL_GL_GetProcAddress("glEndQueryARB");
		qglGetQueryivARB           = (PFNGLGETQUERYIVARBPROC) SDL_GL_GetProcAddress("glGetQueryivARB");
		qglGetQueryObjectivARB     = (PFNGLGETQUERYOBJECTIVARBPROC) SDL_GL_GetProcAddress("glGetQueryObjectivARB");
		qglGetQueryObjectuivARB    = (PFNGLGETQUERYOBJECTUIVARBPROC) SDL_GL_GetProcAddress("glGetQueryObjectuivARB");
		glRefConfig.occlusionQuery = qtrue;
		ri.Printf(PRINT_ALL, result[glRefConfig.occlusionQuery], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_framebuffer_blit
	extension                   = "GL_EXT_framebuffer_blit";
	glRefConfig.framebufferBlit = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		qglBlitFramebufferEXT       = (void *)SDL_GL_GetProcAddress("glBlitFramebufferEXT");
		glRefConfig.framebufferBlit = qtrue;
		ri.Printf(PRINT_ALL, result[glRefConfig.framebufferBlit], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_framebuffer_multisample
	extension                          = "GL_EXT_framebuffer_multisample";
	glRefConfig.framebufferMultisample = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		qglRenderbufferStorageMultisampleEXT = (void *)SDL_GL_GetProcAddress("glRenderbufferStorageMultisampleEXT");
		glRefConfig.framebufferMultisample   = qtrue;
		ri.Printf(PRINT_ALL, result[glRefConfig.framebufferMultisample], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_texture_sRGB
	extension                = "GL_EXT_texture_sRGB";
	glRefConfig.texture_srgb = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		if (r_srgb->integer)
		{
			glRefConfig.texture_srgb = qtrue;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.texture_srgb], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	glRefConfig.textureCompression = TCR_NONE;

	// GL_EXT_texture_compression_latc
	extension = "GL_EXT_texture_compression_latc";
	if (GLimp_HaveExtension(extension))
	{
		if (r_ext_compressed_textures->integer)
		{
			glRefConfig.textureCompression |= TCR_LATC;
		}

		ri.Printf(PRINT_ALL, result[r_ext_compressed_textures->integer ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_texture_compression_bptc
	extension = "GL_ARB_texture_compression_bptc";
	if (GLimp_HaveExtension(extension))
	{
		if (r_ext_compressed_textures->integer >= 2)
		{
			glRefConfig.textureCompression |= TCR_BPTC;
		}

		ri.Printf(PRINT_ALL, result[(r_ext_compressed_textures->integer >= 2) ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_draw_buffers
	extension         = "GL_ARB_draw_buffers";
	qglDrawBuffersARB = NULL;
	if (GLimp_HaveExtension(extension))
	{
		qglDrawBuffersARB = (void *) SDL_GL_GetProcAddress("glDrawBuffersARB");

		ri.Printf(PRINT_ALL, result[1], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_depth_clamp
	extension              = "GL_ARB_depth_clamp";
	glRefConfig.depthClamp = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		glRefConfig.depthClamp = qtrue;
		ri.Printf(PRINT_ALL, result[1], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}
}
