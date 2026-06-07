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
 * @file renderer/tr_gamma.c
 * @brief Functions that are not called every frame
 */

#include "tr_local.h"

typedef struct gammaProgram_s
{
	shaderProgram_t *program;

	GLint gammaUniform;
	float gammaValue;
	GLint overBrightBitsUniform;
	int overBrightBits;
	GLint currentMapUniform;
} gammaProgram_t;

image_t        *screenImage = NULL;
gammaProgram_t gammaProgram;

const char *simpleGammaVert = "#version 110\n"
                              "void main(void) {\n"
                              "gl_Position = gl_Vertex;\n"
                              "gl_TexCoord[0] = gl_MultiTexCoord0;\n"
                              "}\n";

const char *simpleGammaFrag = "#version 110\n"
                              "uniform sampler2D u_CurrentMap;\n"
                              "uniform float u_gamma;\n"
                              "uniform float u_overBrightBits;\n"
                              "void main(void) {\n"
                              "gl_FragColor = vec4(pow(texture2D(u_CurrentMap, vec2(gl_TexCoord[0])).rgb, vec3(1.0 / u_gamma)) * u_overBrightBits, 1.0);\n"
                              "}\n";

/**
 * @brief R_BuildGammaProgram
 */
static void R_BuildGammaProgram(void)
{
	gammaProgram.program = R_CreateShaderProgram(simpleGammaVert, simpleGammaFrag);

	R_UseShaderProgram(gammaProgram.program);
	gammaProgram.currentMapUniform     = R_GetShaderProgramUniform(gammaProgram.program, "u_CurrentMap");
	gammaProgram.gammaUniform          = R_GetShaderProgramUniform(gammaProgram.program, "u_gamma");
	gammaProgram.overBrightBitsUniform = R_GetShaderProgramUniform(gammaProgram.program, "u_overBrightBits");
	R_UseShaderProgram(NULL);
}

/**
 * @brief R_ScreenGamma
 */
void R_ScreenGamma(void)
{
	if (gammaProgram.program)
	{
		R_BindFBO(NULL);

		R_UseShaderProgram(gammaProgram.program);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glClientActiveTextureARB(GL_TEXTURE0_ARB);

		// a hack to fix the eye burning gamma bug during map loads
		if (!mainFbo || !mainFbo->fbo)
		{
			GL_Bind(screenImage);
			// We will copy the current buffer into the screenImage
			glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, glConfig.vidWidth, glConfig.vidHeight, 0);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else
		{
			R_FBOSetViewport(mainFbo, NULL);
			glBindTexture(GL_TEXTURE_2D, mainFbo->color);
		}

		// Maybe should use this instead?
		// R_FBOSetViewport(mainFbo, NULL);
		// R_FboCopyToTex(mainFbo, screenImage);

		if (!gammaProgram.gammaValue || gammaProgram.gammaValue != r_gamma->value)
		{
			glUniform1f(gammaProgram.gammaUniform, r_gamma->value);
			gammaProgram.gammaValue = r_gamma->value;
		}

		if (tr.overbrightBits != gammaProgram.overBrightBits)
		{
			glUniform1f(gammaProgram.overBrightBitsUniform, 1 << tr.overbrightBits);
			gammaProgram.overBrightBits = tr.overbrightBits;
		}

		GL_FullscreenQuad();

		R_UseShaderProgram(NULL);
		R_FBOSetViewport(NULL, mainFbo);
	}
	else
	{
		R_FboBlit(mainFbo, NULL);
	}

	GL_CheckErrors();
}

/**
 * @brief R_InitGamma
 */
void R_InitGamma(void)
{
	if (!R_ShaderProgramsAvailable())
	{
		Ren_Print("WARNING: R_InitGamma() skipped - no shader programs available\n");
		return;
	}

	if (ri.Cvar_VariableIntegerValue("r_ignorehwgamma"))
	{
		Ren_Print("INFO: R_InitGamma() skipped - r_ignorehwgamma is set\n");
		return;
	}

	screenImage = R_CreateImage("screenBufferImage_skies", NULL, glConfig.vidWidth, glConfig.vidHeight, qfalse, qfalse,
	                            GL_CLAMP_TO_EDGE);

	if (!screenImage)
	{
		Ren_Print("WARNING: R_InitGamma() screen image is NULL\n");
	}

	Com_Memset(&gammaProgram, 0, sizeof(gammaProgram_t));
	gammaProgram.overBrightBits = -1;

	R_BuildGammaProgram();

	GL_CheckErrors();

	tr.gammaProgramUsed = qtrue;
}

/**
 * @brief R_ShutdownGamma
 */
void R_ShutdownGamma(void)
{
	if (gammaProgram.program)
	{
		R_DestroyShaderProgram(gammaProgram.program);
		Com_Memset(&gammaProgram, 0, sizeof(gammaProgram_t));
	}

	if (screenImage)
	{
		screenImage = NULL;
	}
}
