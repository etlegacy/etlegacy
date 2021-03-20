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
 * @file renderer/tr_gamma.c
 * @brief Functions that are not called every frame
 */

#include "tr_local.h"

typedef struct shaderProgram_s
{
	GLhandleARB program;
	GLhandleARB vertexShader;
	GLhandleARB fragmentShader;

	GLint gammaUniform;
	GLint currentMapUniform;
} shaderProgram_t;

image_t         *screenImage = NULL;
shaderProgram_t gammaProgram;

const char *simpleGammaVert = "#version 110\n"
                              "void main(void) {\n"
                              "gl_Position = gl_Vertex;\n"
                              "gl_TexCoord[0] = gl_MultiTexCoord0;\n"
                              "}\n";

const char *simpleGammaFrag = "#version 110\n"
                              "uniform sampler2D u_CurrentMap;\n"
                              "uniform float u_gamma;\n"
                              "void main(void) {\n"
                              "gl_FragColor = vec4(pow(texture2D(u_CurrentMap, vec2(gl_TexCoord[0])).rgb, vec3(1.0 / u_gamma)), 1);\n"
                              "}\n";

/**
 * @brief R_BuildGammaProgram
 */
static void R_BuildGammaProgram(void)
{
	GLint compiled;

	gammaProgram.vertexShader   = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	gammaProgram.fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	glShaderSourceARB(gammaProgram.vertexShader, 1, (const GLcharARB **)&simpleGammaVert, NULL);
	glShaderSourceARB(gammaProgram.fragmentShader, 1, (const GLcharARB **)&simpleGammaFrag, NULL);

	glCompileShaderARB(gammaProgram.vertexShader);
	glCompileShaderARB(gammaProgram.fragmentShader);

	glGetObjectParameterivARB(gammaProgram.vertexShader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLint   blen = 0;
		GLsizei slen = 0;

		glGetShaderiv(gammaProgram.vertexShader, GL_INFO_LOG_LENGTH, &blen);
		if (blen > 1)
		{
			GLchar *compiler_log;

			compiler_log = (GLchar *) Com_Allocate(blen);

			glGetInfoLogARB(gammaProgram.vertexShader, blen, &slen, compiler_log);
			Ren_Fatal("Failed to compile the gamma vertex shader reason: %s\n", compiler_log);
		}
		else
		{
			Ren_Fatal("Failed to compile the gamma vertex shader\n");
		}
	}

	glGetObjectParameterivARB(gammaProgram.fragmentShader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		Ren_Fatal("Failed to compile the gamma fragment shader\n");
	}

	gammaProgram.program = glCreateProgramObjectARB();
	if (!gammaProgram.program)
	{
		Ren_Fatal("Failed to create program\n");
	}

	glAttachObjectARB(gammaProgram.program, gammaProgram.vertexShader);
	glAttachObjectARB(gammaProgram.program, gammaProgram.fragmentShader);

	glLinkProgramARB(gammaProgram.program);

	glGetObjectParameterivARB(gammaProgram.program, GL_OBJECT_LINK_STATUS_ARB, &compiled);

	if (!compiled)
	{
		Ren_Fatal("Failed to link gamma shaders\n");
	}

	glUseProgramObjectARB(gammaProgram.program);

	gammaProgram.currentMapUniform = glGetUniformLocation(gammaProgram.program, "u_CurrentMap");
	gammaProgram.gammaUniform      = glGetUniformLocation(gammaProgram.program, "u_gamma");

	glUseProgramObjectARB(0);
}

/**
 * @brief R_ScreenGamma
 */
void R_ScreenGamma(void)
{
	if (gammaProgram.program)
	{
		glUseProgramObjectARB(gammaProgram.program);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glClientActiveTextureARB(GL_TEXTURE0_ARB);

		GL_Bind(screenImage);
		// We will copy the current buffer into the screenImage
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, glConfig.vidWidth, glConfig.vidHeight, 0);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glUniform1f(gammaProgram.gammaUniform, r_gamma->value);

		// Draw a simple quad, We could have done this in the GLSL code directly but that is version 130 upwards
		// and we want to be sure that R1 runs even with a toaster.
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0.0, 0.0);
			glVertex3f(-1.0f, -1.0f, 0.0f);
			glTexCoord2f(1.0, 0.0);
			glVertex3f(1.0f, -1.0f, 0.0f);
			glTexCoord2f(1.0, 1.0);
			glVertex3f(1.0f, 1.0f, 0.0f);
			glTexCoord2f(0.0, 1.0);
			glVertex3f(-1.0f, 1.0f, 0.0f);
		}
		glEnd();

		glUseProgramObjectARB(0);
	}
}

/**
 * @brief R_InitGamma
 */
void R_InitGamma(void)
{
	byte *data;

	if (!GLEW_ARB_fragment_program)
	{
		Ren_Print("WARNING: R_InitGamma() skipped - no ARB_fragment_program\n");
		return;
	}

	if (ri.Cvar_VariableIntegerValue("r_ignorehwgamma"))
	{
		Ren_Print("INFO: R_InitGamma() skipped - r_ignorehwgamma is set\n");
		return;
	}

	data = (byte *)ri.Hunk_AllocateTempMemory(glConfig.vidWidth * glConfig.vidHeight * 4);
	if (!data)
	{
		Ren_Print("WARNING: R_InitGamma() can't allocate temp memory\n"); // TODO: fatal?
		return;
	}

	screenImage = R_CreateImage("screenBufferImage_skies", data, glConfig.vidWidth, glConfig.vidHeight, qfalse, qfalse, GL_CLAMP_TO_EDGE);

	if (!screenImage)
	{
		Ren_Print("WARNING: R_InitGamma() screen image is NULL\n");
	}

	ri.Hunk_FreeTempMemory(data);

	Com_Memset(&gammaProgram, 0, sizeof(shaderProgram_t));
	R_BuildGammaProgram();
	tr.gammaProgramUsed = qtrue;
}

/**
 * @brief R_ShutdownGamma
 */
void R_ShutdownGamma(void)
{
	if (gammaProgram.program)
	{
		if (gammaProgram.vertexShader)
		{
			glDetachObjectARB(gammaProgram.program, gammaProgram.vertexShader);
			glDeleteObjectARB(gammaProgram.vertexShader);
		}

		if (gammaProgram.fragmentShader)
		{
			glDetachObjectARB(gammaProgram.program, gammaProgram.fragmentShader);
			glDeleteObjectARB(gammaProgram.fragmentShader);
		}

		glDeleteObjectARB(gammaProgram.program);
		Com_Memset(&gammaProgram, 0, sizeof(shaderProgram_t));
	}

	if (screenImage)
	{
		screenImage = NULL;
	}
}
