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
 * @file renderer/tr_fbo.c
 * @brief framebuffer object handling for r1
 */

#include "tr_local.h"

typedef struct {
	GLuint fbo;
	GLuint color;
	GLuint depth;
	qboolean stencil;

	int width;
	int height;
} frameBuffer_t;

static frameBuffer_t mainFbo;
static frameBuffer_t *current;

static void R_BindFBO(frameBuffer_t *fb)
{
	current = fb;

	if (fb)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

static void R_CreateFBODepthAttachment(frameBuffer_t *fb)
{
	glGenTextures(1, &fb->depth);
	glBindTexture(GL_TEXTURE_2D, fb->depth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int stencil = ri.Cvar_VariableIntegerValue("r_stencilbits");

	if (stencil == 0)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, fb->width, fb->height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
					 NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->depth, 0);
	}
	else
	{
		fb->stencil = qtrue;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, fb->width, fb->height, 0, GL_DEPTH_STENCIL,
					 GL_UNSIGNED_INT_24_8,
					 NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fb->depth, 0);
	}
}

static void R_CreateFBOColorAttachment(frameBuffer_t *fb)
{
	glGenTextures(1, &fb->color);
	glBindTexture(GL_TEXTURE_2D, fb->color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fb->width, fb->height, 0, GL_RGB, GL_UNSIGNED_INT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->color, 0);
}

static void R_DestroyFBO(frameBuffer_t *fb)
{
	if (!fb || !fb->fbo)
	{
		return;
	}

	R_BindFBO(fb);

	glBindTexture(GL_TEXTURE_2D, 0);

	if (fb->color)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glDeleteTextures(1, &fb->color);
		fb->color = 0;
	}

	if (fb->depth)
	{
		if (fb->stencil)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
		}

		glDeleteTextures(1, &fb->depth);
		fb->depth = 0;
	}

	R_BindFBO(NULL);
	glDeleteFramebuffers(1, &fb->fbo);
	fb->fbo = 0;

	memset(fb, 0, sizeof(frameBuffer_t));
}

static void R_CreateFBO(frameBuffer_t *fb, int width, int height)
{
	if (fb->fbo)
	{
		R_DestroyFBO(fb);
	}

	memset(fb, 0, sizeof(frameBuffer_t));

	fb->width = width;
	fb->height = height;

	glGenFramebuffers(1, &fb->fbo);
	R_BindFBO(fb);

	R_CreateFBOColorAttachment(fb);
	R_CreateFBODepthAttachment(fb);

	glBindTexture(GL_TEXTURE_2D, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		Ren_Fatal("Failed to init FBO");
		return;
	}

	R_BindFBO(NULL);
}

void R_ShutdownFBO(void)
{
	R_DestroyFBO(&mainFbo);
}

void R_MainFBO(qboolean bind)
{
	if (!mainFbo.fbo)
	{
		return;
	}

	if (bind)
	{
		R_BindFBO(&mainFbo);
	}
	else
	{
		R_BindFBO(NULL);
	}
}

GLuint R_MainFBOTexture(void)
{
	return mainFbo.color;
}

void R_MainFBOBlit(void)
{
	if (!mainFbo.fbo)
	{
		return;
	}

	// GLint target;
	// glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &target);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFbo.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glBlitFramebuffer(0, 0, mainFbo.width, mainFbo.height, 0, 0, glConfig.realVidWidth, glConfig.realVidHeight,
					  GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// glBindFramebuffer(GL_FRAMEBUFFER, target);
	R_BindFBO(current);
}

void R_InitFBO(void)
{
	memset(&mainFbo, 0, sizeof(frameBuffer_t));
	current = NULL;

	if (!GLEW_ARB_framebuffer_object)
	{
		Ren_Print("WARNING: R_InitFBO() skipped - no GLEW_ARB_framebuffer_object\n");
		return;
	}

	Ren_Print("Setting up FBO\n");

	R_CreateFBO(&mainFbo, glConfig.vidWidth, glConfig.vidHeight);
}
