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
 * @file renderer_vk/tr_shade.c
 * @brief Vulkan placeholder for shader/stage iteration.
 *
 * @note renderer1's tr_shade.c submits tessellated geometry via immediate
 * OpenGL vertex-array calls (glVertexPointer/glDrawElements/etc.), which has
 * no Vulkan equivalent and must be rewritten as a real pipeline/descriptor
 * based submission path later. For now this file only keeps
 * the bookkeeping (tess begin/end, fog iterator selection) needed by the
 * reused frontend (tr_surface.c/tr_world.c/tr_main.c/...) so the engine can
 * build its draw-surface list and run a full frame without crashing; no
 * surfaces are actually rasterized yet.
 */

#include "tr_local.h"

extern void R_Fog(glfog_t *curfog);

shaderCommands_t tess;

/**
 * @brief We must set some things up before beginning any tesselation,
 * because a surface may be forced to perform a RB_End due to overflow.
 *
 * @param[in] shader
 * @param[in] fogNum
 */
void RB_BeginSurface(shader_t *shader, int fogNum)
{
	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes               = 0;
	tess.numVertexes              = 0;
	tess.shader                   = state;
	tess.fogNum                   = fogNum;
	tess.dlightBits               = 0; // will be OR'd in by surface functions
	tess.xstages                  = state->stages;
	tess.numPasses                = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime != 0.0 && tess.shaderTime >= tess.shader->clampTime)
	{
		tess.shaderTime = tess.shader->clampTime;
	}
}

/**
 * @brief Chooses the current fog settings for debug fog state; kept so
 * fog cvars/state tracked by the reused frontend stay consistent even
 * though the Vulkan backend doesn't yet apply fog in a shader.
 */
void SetIteratorFog(void)
{
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		R_FogOff();
		return;
	}

	if (backEnd.refdef.rdflags & RDF_DRAWINGSKY)
	{
		if (glfogsettings[FOG_SKY].registered)
		{
			R_Fog(&glfogsettings[FOG_SKY]);
		}
		else
		{
			R_FogOff();
		}

		return;
	}

	if (skyboxportal && (backEnd.refdef.rdflags & RDF_SKYBOXPORTAL))
	{
		if (glfogsettings[FOG_PORTALVIEW].registered)
		{
			R_Fog(&glfogsettings[FOG_PORTALVIEW]);
		}
		else
		{
			R_FogOff();
		}
	}
	else
	{
		if (glfogNum > FOG_NONE)
		{
			R_Fog(&glfogsettings[FOG_CURRENT]);
		}
		else
		{
			R_FogOff();
		}
	}
}

/**
 * @brief No-op stage iterators (geometry rasterization isn't implemented yet).
 */
void RB_StageIteratorGeneric(void)
{
}

void RB_StageIteratorVertexLitTexture(void)
{
}

void RB_StageIteratorLightmappedMultitexture(void)
{
}

/**
 * @brief RB_EndSurface
 */
void RB_EndSurface(void)
{
	shaderCommands_t *input = &tess;

	if (input->numIndexes == 0)
	{
		return;
	}

	if (input->indexes[SHADER_MAX_INDEXES - 1] != 0)
	{
		Ren_Drop("RB_EndSurface() - input->maxShaderIndicies(%i) hit", SHADER_MAX_INDEXES);
	}
	if (input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0.f)
	{
		Ren_Drop("RB_EndSurface() - input->maxShaderVerts(%i) hit", SHADER_MAX_VERTEXES);
	}

	// update performance counters
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes     += tess.numVertexes;
	backEnd.pc.c_indexes      += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	// call off to shader specific tess end function (currently a no-op)
	tess.currentStageIteratorFunc();

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes  = 0;
	tess.numVertexes = 0;
}
