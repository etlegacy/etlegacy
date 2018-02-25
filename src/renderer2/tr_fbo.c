/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
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
 * @file renderer2/tr_fbo.c
 */

#include "tr_local.h"

/**
 * @brief R_CheckFBO
 * @param[in] fbo
 * @return
 */
qboolean R_CheckFBO(const FBO_t *fbo)
{
	int code;
	int id;

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &id);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->frameBuffer);

	code = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	switch (code)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		glBindFramebuffer(GL_FRAMEBUFFER_EXT, id); // ok
		return qtrue;
	// an error occured
	case GL_FRAMEBUFFER_UNSUPPORTED:
		Ren_Warning("R_CheckFBO: (%s) Unsupported framebuffer format\n", fbo->name);
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		Ren_Warning("R_CheckFBO: (%s) Framebuffer incomplete attachment\n", fbo->name);
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		Ren_Warning("R_CheckFBO: (%s) Framebuffer incomplete, missing attachment\n", fbo->name);
		break;
	//case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
	//  Ren_Warning( "R_CheckFBO: (%s) Framebuffer incomplete, duplicate attachment\n", fbo->name);
	//  break;
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
		Ren_Warning("R_CheckFBO: (%s) Framebuffer incomplete, attached images must have same dimensions\n", fbo->name);
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
		Ren_Warning("R_CheckFBO: (%s) Framebuffer incomplete, attached images must have same format\n", fbo->name);
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		Ren_Warning("R_CheckFBO: (%s) Framebuffer incomplete, missing draw buffer\n", fbo->name);
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		Ren_Warning("R_CheckFBO: (%s) Framebuffer incomplete, missing read buffer\n", fbo->name);
		break;
	default:
		Ren_Warning("R_CheckFBO: (%s) unknown error 0x%X\n", fbo->name, code);
		//Ren_Fatal( "R_CheckFBO: (%s) unknown error 0x%X", fbo->name, code);
		//etl_assert(0);
		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, id);

	return qfalse;
}

/**
 * @brief R_CreateFBO
 * @param[in] name
 * @param[in] width
 * @param[in] height
 * @return
 */
FBO_t *R_CreateFBO(const char *name, int width, int height)
{
	int   i = 0;
	FBO_t *fbo;

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Drop("R_CreateFBO: \"%s\" is too long\n", name);
	}

	if (width <= 0 || width > glConfig2.maxRenderbufferSize)
	{
		Ren_Drop("R_CreateFBO: bad width %i", width);
	}

	if (height <= 0 || height > glConfig2.maxRenderbufferSize)
	{
		Ren_Drop("R_CreateFBO: bad height %i", height);
	}

	if (tr.numFBOs == MAX_FBOS)
	{
		Ren_Drop("R_CreateFBO: MAX_FBOS hit");
	}

	fbo = tr.fbos[tr.numFBOs] = (FBO_t *)ri.Hunk_Alloc(sizeof(*fbo), h_low);
	Q_strncpyz(fbo->name, name, sizeof(fbo->name));
	fbo->index  = tr.numFBOs++;
	fbo->width  = width;
	fbo->height = height;

	for (; i < 16; i++)
	{
		fbo->colorBuffers[i].buffer = 0;
	}
	fbo->depthBuffer.buffer              = 0;
	fbo->stencilBuffer.buffer            = 0;
	fbo->packedDepthStencilBuffer.buffer = 0;

	glGenFramebuffers(1, &fbo->frameBuffer);

	return fbo;
}

/**
 * @brief Framebuffer must be bound
 * @param[in,out] fbo
 * @param[in] format
 * @param[in] index
 */
void R_CreateFBOColorBuffer(FBO_t *fbo, int format, int index)
{
	qboolean      absent;
	BufferImage_t *bufferImage;

	if (index < 0 || index >= glConfig2.maxColorAttachments)
	{
		Ren_Warning("R_CreateFBOColorBuffer: invalid attachment index %i\n", index);
		return;
	}

#if 0
	if (format != GL_RGB &&
	    format != GL_RGBA &&
	    format != GL_RGB16F_ARB && format != GL_RGBA16F_ARB && format != GL_RGB32F_ARB && format != GL_RGBA32F_ARB)
	{
		Ren_Warning("R_CreateFBOColorBuffer: format %i is not color-renderable\n", format);
		//return;
	}
#endif
	bufferImage = &fbo->colorBuffers[index];

	bufferImage->format = format;

	absent = bufferImage->buffer == 0;
	if (absent)
	{
		glGenRenderbuffers(1, &bufferImage->buffer);
	}

	glBindRenderbuffer(GL_RENDERBUFFER, bufferImage->buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, format, fbo->width, fbo->height);

	if (absent)
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_RENDERBUFFER, bufferImage->buffer);
	}

	GL_CheckErrors();
}

/**
 * @brief R_CreateFBODepthBuffer
 * @param[in,out] fbo
 * @param[in] format
 */
void R_CreateFBODepthBuffer(FBO_t *fbo, int format)
{
	qboolean      absent;
	BufferImage_t *bufferImage;

	if (format != GL_DEPTH_COMPONENT &&
	    format != GL_DEPTH_COMPONENT16_ARB && format != GL_DEPTH_COMPONENT24_ARB && format != GL_DEPTH_COMPONENT32_ARB)
	{
		Ren_Warning("R_CreateFBODepthBuffer: format %i is not depth-renderable\n", format);
		return;
	}

	bufferImage = &fbo->depthBuffer;

	bufferImage->format = format;

	absent = bufferImage->buffer == 0;
	if (absent)
	{
		glGenRenderbuffers(1, &bufferImage->buffer);
	}

	glBindRenderbuffer(GL_RENDERBUFFER, bufferImage->buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, format, fbo->width, fbo->height);

	if (absent)
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, bufferImage->buffer);
	}

	GL_CheckErrors();
}

/**
 * @brief R_CreateFBOStencilBuffer
 * @param[in,out] fbo
 * @param[in] format
 */
void R_CreateFBOStencilBuffer(FBO_t *fbo, int format)
{
	qboolean      absent;
	BufferImage_t *bufferImage;

	if (format != GL_STENCIL_INDEX &&
	    //format != GL_STENCIL_INDEX &&
	    format != GL_STENCIL_INDEX1 &&
	    format != GL_STENCIL_INDEX4 && format != GL_STENCIL_INDEX8 && format != GL_STENCIL_INDEX16)
	{
		Ren_Warning("R_CreateFBOStencilBuffer: format %i is not stencil-renderable\n", format);
		return;
	}

	bufferImage         = &fbo->stencilBuffer;
	bufferImage->format = format;

	absent = bufferImage->buffer == 0;
	if (absent)
	{
		glGenRenderbuffers(1, &bufferImage->buffer);
	}

	glBindRenderbuffer(GL_RENDERBUFFER, bufferImage->buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, format, fbo->width, fbo->height);
	GL_CheckErrors();

	if (absent)
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, bufferImage->buffer);
	}

	GL_CheckErrors();
}

/**
 * @brief R_CreateFBOPackedDepthStencilBuffer
 * @param[in,out] fbo
 * @param[in] format
 */
void R_CreateFBOPackedDepthStencilBuffer(FBO_t *fbo, int format)
{
	qboolean      absent;
	BufferImage_t *bufferImage;

	if (format != GL_DEPTH_STENCIL && format != GL_DEPTH24_STENCIL8)
	{
		Ren_Warning("R_CreateFBOPackedDepthStencilBuffer: format %i is not depth-stencil-renderable\n", format);
		return;
	}

	bufferImage         = &fbo->packedDepthStencilBuffer;
	bufferImage->format = format;

	absent = bufferImage->buffer == 0;
	if (absent)
	{
		glGenRenderbuffers(1, &bufferImage->buffer);
	}

	glBindRenderbuffer(GL_RENDERBUFFER, bufferImage->buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, format, fbo->width, fbo->height);
	GL_CheckErrors();

	if (absent)
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, bufferImage->buffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, bufferImage->buffer);
	}

	GL_CheckErrors();
}

/**
 * @brief R_AttachFBOTexture1D
 * @param[in] texId
 * @param[in] index
 */
void R_AttachFBOTexture1D(int texId, int index)
{
	if (index < 0 || index >= glConfig2.maxColorAttachments)
	{
		Ren_Warning("R_AttachFBOTexture1D: invalid attachment index %i\n", index);
		return;
	}

	glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_1D, texId, 0);
}

/**
 * @brief R_AttachFBOTexture2D
 * @param[in] target
 * @param[in] texId
 * @param[in] index
 */
void R_AttachFBOTexture2D(int target, int texId, int index)
{
	if (target != GL_TEXTURE_2D && (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X || target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))
	{
		Ren_Warning("R_AttachFBOTexture2D: invalid target %i\n", target);
		return;
	}

	if (index < 0 || index >= glConfig2.maxColorAttachments)
	{
		Ren_Warning("R_AttachFBOTexture2D: invalid attachment index %i\n", index);
		return;
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target, texId, 0);
}

/**
 * @brief R_AttachFBOTexture3D
 * @param[in] texId
 * @param[in] index
 * @param[in] zOffset
 */
void R_AttachFBOTexture3D(int texId, int index, int zOffset)
{
	if (index < 0 || index >= glConfig2.maxColorAttachments)
	{
		Ren_Warning("R_AttachFBOTexture3D: invalid attachment index %i\n", index);
		return;
	}

	glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_3D, texId, 0, zOffset);
}

/**
 * @brief R_AttachFBOTextureDepth
 * @param[in] texId
 */
void R_AttachFBOTextureDepth(int texId)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
}

/**
 * @brief R_AttachFBOTexturePackedDepthStencil
 * @param[in] texId
 */
void R_AttachFBOTexturePackedDepthStencil(int texId)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
}

/**
 * @brief R_CopyToFBO
 * @param[in] from
 * @param[in] to
 * @param[in] mask
 * @param[in] filter
 */
void R_CopyToFBO(FBO_t *from, FBO_t *to, GLuint mask, GLuint filter)
{
	if (glConfig2.framebufferBlitAvailable)
	{
		vec2_t size;

		if (from)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, from->frameBuffer);
			size[0] = from->width;
			size[1] = from->height;
		}
		else
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			size[0] = glConfig.vidWidth;
			size[1] = glConfig.vidHeight;
		}

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to->frameBuffer);
		glBlitFramebuffer(0, 0, size[0], size[1], 0, 0, to->width, to->height, mask, filter);

		//Just set the read buffer to the target as well otherwise we might get fucked..
		glBindFramebuffer(GL_FRAMEBUFFER, to->frameBuffer);
		glState.currentFBO = to;
	}
	else
	{
		// FIXME add non EXT_framebuffer_blit code
		Ren_Fatal("R_CopyToFBO no framebufferblitting available");
	}
}

/**
 * @brief R_BindFBO
 * @param[in] fbo
 */
void R_BindFBO(FBO_t *fbo)
{
	if (!fbo)
	{
		R_BindNullFBO();
		return;
	}

	Ren_LogComment("--- R_BindFBO( %s ) ---\n", fbo->name);

	if (glState.currentFBO != fbo)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo->frameBuffer);

		if(fbo->colorBuffers[0].buffer)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, fbo->colorBuffers[0].buffer);
		}
		
		if(fbo->depthBuffer.buffer)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, fbo->depthBuffer.buffer);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo->depthBuffer.buffer);
		}

		glState.currentFBO = fbo;
	}
}

/**
 * @brief R_BindNullFBO
 */
void R_BindNullFBO(void)
{
	Ren_LogComment("--- R_BindNullFBO ---\n");

	if (glState.currentFBO && glConfig2.framebufferObjectAvailable)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glState.currentFBO = NULL;
	}
}

/**
 * @brief R_SetDefaultFBO
 */
void R_SetDefaultFBO(void)
{
	if (glConfig2.framebufferObjectAvailable)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glState.currentFBO = NULL;
	}
}

/**
 * @brief R_CheckDefaultBuffer
 */
static void R_CheckDefaultBuffer()
{
	unsigned int fbostatus;
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glState.currentFBO = NULL;

	fbostatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbostatus != GL_FRAMEBUFFER_COMPLETE)
	{
		if (fbostatus == GL_FRAMEBUFFER_UNDEFINED)
		{
			Ren_Fatal("Default framebuffer is undefined!");
		}
		else
		{
			Ren_Fatal("There is an issue with the opengl context:s default framebuffer...%i", fbostatus);
		}
	}
}

/**
 * @brief R_AttachColorBufferToFBO
 * @param[in,out] fbo
 * @param[in] format
 * @param[in] target
 * @param[in] texture
 * @param[in] index
 */
void R_AttachColorBufferToFBO(FBO_t *fbo, int format, int target, image_t *texture, int index)
{
	R_CreateFBOColorBuffer(fbo, format, index);
	R_AttachFBOTexture2D(target, texture->texnum, index);
	fbo->colorBuffers[index].texture = texture;
}

/**
 * @brief R_CreateReadyFBO
 * @param[in] name
 * @param[in] size
 * @return
 */
FBO_t *R_CreateReadyFBO(const char *name, float size)
{
	int   width, height;
	FBO_t *tmp;

	if (glConfig2.textureNPOTAvailable)
	{
		width  = glConfig.vidWidth * size;
		height = glConfig.vidHeight * size;
	}
	else
	{
		width  = NearestPowerOfTwo(glConfig.vidWidth * size);
		height = NearestPowerOfTwo(glConfig.vidHeight * size);
	}

	tmp = R_CreateFBO(name, width, height);

	R_BindFBO(tmp);

	return tmp;
}

/**
 * @brief R_InitFBOs
 */
void R_InitFBOs(void)
{
	int i;
	int width, height;

	Ren_Developer("------- R_InitFBOs -------\n");

	if (!glConfig2.framebufferObjectAvailable)
	{
		return;
	}

	R_CheckDefaultBuffer();

	tr.numFBOs = 0;

	GL_CheckErrors();

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	{
		// forward shading
		if (glConfig2.textureNPOTAvailable)
		{
			width  = glConfig.vidWidth;
			height = glConfig.vidHeight;
		}
		else
		{
			width  = NearestPowerOfTwo(glConfig.vidWidth);
			height = NearestPowerOfTwo(glConfig.vidHeight);
		}

		// deferredRender FBO for the HDR or LDR context
		tr.deferredRenderFBO = R_CreateFBO("_deferredRender", width, height);
		R_BindFBO(tr.deferredRenderFBO);

		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			R_CreateFBOColorBuffer(tr.deferredRenderFBO, GL_RGBA16F_ARB, 0);
		}
		else
		{
			R_CreateFBOColorBuffer(tr.deferredRenderFBO, GL_RGBA, 0);
		}
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.deferredRenderFBOImage->texnum, 0);

		R_CreateFBODepthBuffer(tr.deferredRenderFBO, GL_DEPTH_COMPONENT24_ARB);
		R_AttachFBOTextureDepth(tr.depthRenderImage->texnum);

		R_CheckFBO(tr.deferredRenderFBO);
	}

	if (glConfig2.framebufferBlitAvailable)
	{
		if (glConfig2.textureNPOTAvailable)
		{
			width  = glConfig.vidWidth;
			height = glConfig.vidHeight;
		}
		else
		{
			width  = NearestPowerOfTwo(glConfig.vidWidth);
			height = NearestPowerOfTwo(glConfig.vidHeight);
		}

		tr.occlusionRenderFBO = R_CreateFBO("_occlusionRender", width, height);
		R_BindFBO(tr.occlusionRenderFBO);

#if 0
		if (glConfig2.framebufferPackedDepthStencilAvailable)
		{
			//R_CreateFBOColorBuffer(tr.occlusionRenderFBO, GL_ALPHA32F_ARB, 0);
			R_CreateFBOPackedDepthStencilBuffer(tr.occlusionRenderFBO, GL_DEPTH24_STENCIL8);
		}
		else
		{
			//R_CreateFBOColorBuffer(tr.occlusionRenderFBO, GL_RGBA, 0);
			R_CreateFBODepthBuffer(tr.occlusionRenderFBO, GL_DEPTH_COMPONENT24);
		}
#else
		R_CreateFBODepthBuffer(tr.occlusionRenderFBO, GL_DEPTH_COMPONENT24);
#endif

		R_CreateFBOColorBuffer(tr.occlusionRenderFBO, GL_RGBA, 0);
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.occlusionRenderFBOImage->texnum, 0);

		R_CheckFBO(tr.occlusionRenderFBO);
	}

	if (r_shadows->integer >= SHADOWING_ESM16 && glConfig2.textureFloatAvailable)
	{
		// shadowMap FBOs for shadow mapping offscreen rendering
		for (i = 0; i < MAX_SHADOWMAPS; i++)
		{
			width = height = shadowMapResolutions[i];

			tr.shadowMapFBO[i] = R_CreateFBO(va("_shadowMap%d", i), width, height);
			R_BindFBO(tr.shadowMapFBO[i]);


			if (r_shadows->integer == SHADOWING_ESM32)
			{
				R_CreateFBOColorBuffer(tr.shadowMapFBO[i], GL_ALPHA32F_ARB, 0);
			}
			else if (r_shadows->integer == SHADOWING_VSM32)
			{
				R_CreateFBOColorBuffer(tr.shadowMapFBO[i], GL_LUMINANCE_ALPHA32F_ARB, 0);
			}
			else if (r_shadows->integer == SHADOWING_EVSM32)
			{
				if (r_evsmPostProcess->integer)
				{
					R_CreateFBOColorBuffer(tr.shadowMapFBO[i], GL_ALPHA32F_ARB, 0);
				}
				else
				{
					R_CreateFBOColorBuffer(tr.shadowMapFBO[i], GL_RGBA32F_ARB, 0);
				}
			}
			else
			{
				R_CreateFBOColorBuffer(tr.shadowMapFBO[i], GL_RGBA16F_ARB, 0);
			}

			R_CreateFBODepthBuffer(tr.shadowMapFBO[i], GL_DEPTH_COMPONENT24_ARB);

			R_CheckFBO(tr.shadowMapFBO[i]);
		}

		// sun requires different resolutions
		for (i = 0; i < MAX_SHADOWMAPS; i++)
		{
			width = height = sunShadowMapResolutions[i];

			tr.sunShadowMapFBO[i] = R_CreateFBO(va("_sunShadowMap%d", i), width, height);
			R_BindFBO(tr.sunShadowMapFBO[i]);

			if (r_shadows->integer == SHADOWING_ESM32)
			{
				R_CreateFBOColorBuffer(tr.sunShadowMapFBO[i], GL_ALPHA32F_ARB, 0);
			}
			else if (r_shadows->integer == SHADOWING_VSM32)
			{
				R_CreateFBOColorBuffer(tr.sunShadowMapFBO[i], GL_LUMINANCE_ALPHA32F_ARB, 0);
			}
			else if (r_shadows->integer == SHADOWING_EVSM32)
			{
				if (!r_evsmPostProcess->integer)
				{
					R_CreateFBOColorBuffer(tr.sunShadowMapFBO[i], GL_RGBA32F_ARB, 0);
				}
			}
			else
			{
				R_CreateFBOColorBuffer(tr.sunShadowMapFBO[i], GL_RGBA16F_ARB, 0);
			}

			R_CreateFBODepthBuffer(tr.sunShadowMapFBO[i], GL_DEPTH_COMPONENT24_ARB);

			if (r_shadows->integer == SHADOWING_EVSM32 && r_evsmPostProcess->integer)
			{
				R_AttachFBOTextureDepth(tr.sunShadowMapFBOImage[i]->texnum);

				/*
				Since we don't have a color attachment the framebuffer will be considered incomplete.
				Consequently, we must inform the driver that we do not wish to render to the color buffer.
				We do this with a call to set the draw-buffer and read-buffer to GL_NONE:
				*/
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			}

			R_CheckFBO(tr.sunShadowMapFBO[i]);
		}
	}

	{
		if (glConfig2.textureNPOTAvailable)
		{
			width  = glConfig.vidWidth;
			height = glConfig.vidHeight;
		}
		else
		{
			width  = NearestPowerOfTwo(glConfig.vidWidth);
			height = NearestPowerOfTwo(glConfig.vidHeight);
		}

		// portalRender FBO for portals, mirrors, water, cameras et cetera
		tr.portalRenderFBO = R_CreateFBO("_portalRender", width, height);
		R_BindFBO(tr.portalRenderFBO);

		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			R_CreateFBOColorBuffer(tr.portalRenderFBO, GL_RGBA16F_ARB, 0);
		}
		else
		{
			R_CreateFBOColorBuffer(tr.portalRenderFBO, GL_RGBA, 0);
		}
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.portalRenderImage->texnum, 0);

		R_CheckFBO(tr.portalRenderFBO);
	}


	{
		if (glConfig2.textureNPOTAvailable)
		{
			width  = glConfig.vidWidth * 0.25f;
			height = glConfig.vidHeight * 0.25f;
		}
		else
		{
			width  = NearestPowerOfTwo(glConfig.vidWidth * 0.25f);
			height = NearestPowerOfTwo(glConfig.vidHeight * 0.25f);
		}

		tr.downScaleFBO_quarter = R_CreateFBO("_downScale_quarter", width, height);
		R_BindFBO(tr.downScaleFBO_quarter);
		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_quarter, GL_RGBA16F_ARB, 0);
		}
		else
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_quarter, GL_RGBA, 0);
		}
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.downScaleFBOImage_quarter->texnum, 0);
		R_CheckFBO(tr.downScaleFBO_quarter);


		tr.downScaleFBO_64x64 = R_CreateFBO("_downScale_64x64", 64, 64);
		R_BindFBO(tr.downScaleFBO_64x64);
		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_64x64, GL_RGBA16F_ARB, 0);
		}
		else
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_64x64, GL_RGBA, 0);
		}
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.downScaleFBOImage_64x64->texnum, 0);
		R_CheckFBO(tr.downScaleFBO_64x64);

#if 0
		tr.downScaleFBO_16x16 = R_CreateFBO("_downScale_16x16", 16, 16);
		R_BindFBO(tr.downScaleFBO_16x16);
		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_16x16, GL_RGBA16F_ARB, 0);
		}
		else
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_16x16, GL_RGBA, 0);
		}
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.downScaleFBOImage_16x16->texnum, 0);
		R_CheckFBO(tr.downScaleFBO_16x16);


		tr.downScaleFBO_4x4 = R_CreateFBO("_downScale_4x4", 4, 4);
		R_BindFBO(tr.downScaleFBO_4x4);
		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_4x4, GL_RGBA16F_ARB, 0);
		}
		else
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_4x4, GL_RGBA, 0);
		}
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.downScaleFBOImage_4x4->texnum, 0);
		R_CheckFBO(tr.downScaleFBO_4x4);


		tr.downScaleFBO_1x1 = R_CreateFBO("_downScale_1x1", 1, 1);
		R_BindFBO(tr.downScaleFBO_1x1);
		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_1x1, GL_RGBA16F_ARB, 0);
		}
		else
		{
			R_CreateFBOColorBuffer(tr.downScaleFBO_1x1, GL_RGBA, 0);
		}
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.downScaleFBOImage_1x1->texnum, 0);
		R_CheckFBO(tr.downScaleFBO_1x1);
#endif

		if (glConfig2.textureNPOTAvailable)
		{
			width  = glConfig.vidWidth * 0.25f;
			height = glConfig.vidHeight * 0.25f;
		}
		else
		{
			width  = NearestPowerOfTwo(glConfig.vidWidth * 0.25f);
			height = NearestPowerOfTwo(glConfig.vidHeight * 0.25f);
		}

		tr.contrastRenderFBO = R_CreateFBO("_contrastRender", width, height);
		R_BindFBO(tr.contrastRenderFBO);

		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			R_CreateFBOColorBuffer(tr.contrastRenderFBO, GL_RGBA16F_ARB, 0);
		}
		else
		{
			R_CreateFBOColorBuffer(tr.contrastRenderFBO, GL_RGBA, 0);
		}
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.contrastRenderFBOImage->texnum, 0);

		R_CheckFBO(tr.contrastRenderFBO);


		for (i = 0; i < 2; i++)
		{
			tr.bloomRenderFBO[i] = R_CreateFBO(va("_bloomRender%d", i), width, height);
			R_BindFBO(tr.bloomRenderFBO[i]);

			if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
			{
				R_CreateFBOColorBuffer(tr.bloomRenderFBO[i], GL_RGBA16F_ARB, 0);
			}
			else
			{
				R_CreateFBOColorBuffer(tr.bloomRenderFBO[i], GL_RGBA, 0);
			}
			R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.bloomRenderFBOImage[i]->texnum, 0);

			R_CheckFBO(tr.bloomRenderFBO[i]);
		}
	}

	GL_CheckErrors();

	R_BindNullFBO();
}

/**
 * @brief R_ShutdownFBOs
 */
void R_ShutdownFBOs(void)
{
	int   i, j;
	FBO_t *fbo;

	Ren_Developer("------- R_ShutdownFBOs -------\n");

	if (!glConfig2.framebufferObjectAvailable)
	{
		return;
	}

	R_BindNullFBO();

	for (i = 0; i < tr.numFBOs; i++)
	{
		fbo = tr.fbos[i];

		for (j = 0; j < glConfig2.maxColorAttachments; j++)
		{
			if (fbo->colorBuffers[j].buffer)
			{
				glDeleteRenderbuffers(1, &fbo->colorBuffers[j].buffer);
			}
		}

		if (fbo->depthBuffer.buffer)
		{
			glDeleteRenderbuffers(1, &fbo->depthBuffer.buffer);
		}

		if (fbo->stencilBuffer.buffer)
		{
			glDeleteRenderbuffers(1, &fbo->stencilBuffer.buffer);
		}

		if (fbo->frameBuffer)
		{
			glDeleteFramebuffers(1, &fbo->frameBuffer);
		}
	}
}

/**
 * @brief R_FBOList_f
 */
void R_FBOList_f(void)
{
	int   i;
	FBO_t *fbo;

	if (!glConfig2.framebufferObjectAvailable)
	{
		Ren_Print("GL_EXT_framebuffer_object is not available.\n");
		return;
	}

	Ren_Print("             size       name\n");
	Ren_Print("----------------------------------------------------------\n");

	for (i = 0; i < tr.numFBOs; i++)
	{
		fbo = tr.fbos[i];

		Ren_Print("  %4i: %4i %4i %s\n", i, fbo->width, fbo->height, fbo->name);
	}

	Ren_Print(" %i FBOs\n", tr.numFBOs);
}
