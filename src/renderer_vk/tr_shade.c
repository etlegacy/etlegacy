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
 * @brief Shader/stage iteration for the Vulkan backend.
 *
 * @note renderer1's tr_shade.c submits tessellated geometry via immediate
 * OpenGL vertex-array calls (glVertexPointer/glDrawElements/etc.), which has
 * no Vulkan equivalent. 2D/UI batches (backEnd.projection2D) are submitted
 * via VK_Draw2D(); 3D batches via VK_Draw3D()/VK_Draw3DLightmapped() (see
 * vulkan/vk.c). ComputeColors()/ComputeTexCoords() are ported verbatim from
 * renderer1 (pure CPU math, no GL calls).
 */

#include "tr_local.h"
#include "vulkan/vk.h"

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
	tess.vkStatic                 = qfalse; // decided by the first surface (RB_SurfaceTriangles)
	tess.vkDiffuse                = qfalse;
	tess.vkSkin                   = qfalse;
	tess.vkMesh                   = qfalse;
	tess.vkIndirect               = qfalse;
	tess.vkEnvOffset              = 0;
	tess.vkStage                  = NULL;
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
 * @brief Vulkan equivalent of renderer1's R_BindAnimatedImage(): returns
 * the image to sample for this bundle instead of binding it, so animMap
 * stages (flames, explosions, etc.) cycle frames like they do in GL1.
 */
static image_t *R_GetAnimatedImage(textureBundle_t *bundle)
{
	int64_t index;

	if (bundle->isLightmap && (backEnd.refdef.rdflags & RDF_SNOOPERVIEW))
	{
		return tr.whiteImage;
	}

	// Video maps advance and upload their cinematic (renderer1's
	// R_BindAnimatedImage), then sample its scratch image
	if (bundle->isVideoMap)
	{
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);

		if (bundle->videoMapHandle >= 0 && bundle->videoMapHandle < (int)ARRAY_LEN(tr.scratchImage) &&
		    tr.scratchImage[bundle->videoMapHandle])
		{
			return tr.scratchImage[bundle->videoMapHandle];
		}
		return bundle->image[0];
	}

	if (bundle->numImageAnimations <= 1)
	{
		return bundle->image[0];
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	index   = (int64_t)(tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
	index >>= FUNCTABLE_SIZE2;

	if (index < 0)
	{
		index = 0;  // may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	return bundle->image[index];
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
 * @brief Vulkan: true when this rgbGen lightingDiffuse stage's rgb can be
 * computed in the vertex shader (VK_Draw3DDiffuse) instead of by
 * RB_CalcDiffuseColor(): a single-texture 3D stage whose rgb nothing on the
 * CPU changes afterwards - no entity fade, no fog color modulation, no
 * r_debugShaderSurfaceFlags. alphaGen still runs on the CPU.
 */
static qboolean RB_StageGpuDiffuse(const shaderStage_t *pStage)
{
	int fadeStart = backEnd.currentEntity->e.fadeStartTime;

	return (r_vkGpuVertex->integer && pStage->rgbGen == CGEN_LIGHTING_DIFFUSE && !pStage->bundle[1].image[0] &&
	        tess.numVertexes > 0 && !tess.vkStatic && !backEnd.projection2D &&
	        !(fadeStart && fadeStart <= tr.refdef.time) &&
	        !(tess.fogNum && !tess.shader->noFog &&
	          (pStage->adjustColorsForFog == ACFF_MODULATE_RGB || pStage->adjustColorsForFog == ACFF_MODULATE_RGBA)) &&
	        !r_debugShaderSurfaceFlags->integer) ? qtrue : qfalse;
}

/**
 * @brief ComputeColors/ComputeTexCoords ported verbatim from renderer1's
 * tr_shade.c (pure CPU math writing into tess.svars; no GL calls).
 */
static void ComputeColors(shaderStage_t *pStage)
{
	// rgbGen
	switch (pStage->rgbGen)
	{
	case CGEN_IDENTITY:
		Com_Memset(tess.svars.colors, 0xff, tess.numVertexes * 4);
		break;
	default:
	case CGEN_IDENTITY_LIGHTING:
		Com_Memset(tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4);
		break;
	case CGEN_LIGHTING_DIFFUSE:
		if (tess.vkDiffuse)
		{
			// rgb comes from the vertex shader; alpha as the light table's
			Com_Memset(tess.svars.colors, 0xff, tess.numVertexes * 4);
		}
		else
		{
			RB_CalcDiffuseColor(( unsigned char * ) tess.svars.colors);
		}
		break;
	case CGEN_EXACT_VERTEX:
		Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
		break;
	case CGEN_CONST:
	{
		int i;

		for (i = 0; i < tess.numVertexes; i++)
		{
			*(int *)tess.svars.colors[i] = *(int *)pStage->constantColor;
		}
	}
	break;
	case CGEN_VERTEX:
		if (tr.identityLight == 1)
		{
			Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
		}
		else
		{
			int i;

			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
				tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
				tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
		break;
	case CGEN_ONE_MINUS_VERTEX:
	{
		int i;

		if (tr.identityLight == 1)
		{
			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
				tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
				tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
			}
		}
		else
		{
			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][0] = (255 - tess.vertexColors[i][0]) * tr.identityLight;
				tess.svars.colors[i][1] = (255 - tess.vertexColors[i][1]) * tr.identityLight;
				tess.svars.colors[i][2] = (255 - tess.vertexColors[i][2]) * tr.identityLight;
			}
		}
	}
	break;
	case CGEN_FOG:
	{
		int   i;
		fog_t *fog = tr.world->fogs + tess.fogNum;

		for (i = 0; i < tess.numVertexes; i++)
		{
			*( int * )&tess.svars.colors[i] = fog->shader->fogParms.colorInt;
		}
	}
	break;
	case CGEN_WAVEFORM:
		RB_CalcWaveColor(&pStage->rgbWave, ( unsigned char * ) tess.svars.colors);
		break;
	case CGEN_ENTITY:
		RB_CalcColorFromEntity(( unsigned char * ) tess.svars.colors);
		break;
	case CGEN_ONE_MINUS_ENTITY:
		RB_CalcColorFromOneMinusEntity(( unsigned char * ) tess.svars.colors);
		break;
	}

	// alphaGen
	switch (pStage->alphaGen)
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if (pStage->rgbGen != CGEN_IDENTITY)
		{
			if ((pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1) ||
			    pStage->rgbGen != CGEN_VERTEX)
			{
				int i;

				for (i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.colors[i][3] = 0xff;
				}
			}
		}
		break;
	case AGEN_CONST:
		if (pStage->rgbGen != CGEN_CONST)
		{
			int i;

			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha(&pStage->alphaWave, ( unsigned char * ) tess.svars.colors);
		break;
	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha(( unsigned char * ) tess.svars.colors);
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity(( unsigned char * ) tess.svars.colors);
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity(( unsigned char * ) tess.svars.colors);
		break;
	case AGEN_NORMALZFADE:
	{
		float    alpha, range, lowest, highest, dot;
		vec3_t   worldUp;
		int      i;
		qboolean zombieEffect = qfalse;

		if (VectorCompare(backEnd.currentEntity->e.fireRiseDir, vec3_origin))
		{
			VectorSet(backEnd.currentEntity->e.fireRiseDir, 0, 0, 1);
		}

		if (backEnd.currentEntity->e.hModel)        // world surfaces dont have an axis
		{
			VectorRotate(backEnd.currentEntity->e.fireRiseDir, backEnd.currentEntity->e.axis, worldUp);
		}
		else
		{
			VectorCopy(backEnd.currentEntity->e.fireRiseDir, worldUp);
		}

		lowest = pStage->zFadeBounds[0];
		if (lowest == -1000)        // use entity alpha
		{
			lowest       = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		highest = pStage->zFadeBounds[1];
		if (highest == -1000)       // use entity alpha
		{
			highest      = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		range = highest - lowest;
		for (i = 0; i < tess.numVertexes; i++)
		{
			dot = DotProduct(tess.normal[i], worldUp);

			// special handling for Zombie fade effect
			if (zombieEffect)
			{
				alpha  = (float)backEnd.currentEntity->e.shaderRGBA[3] * (dot + 1.0f) / 2.0f;
				alpha += (2.0f * (float)backEnd.currentEntity->e.shaderRGBA[3]) * (1.0f - (dot + 1.0f) / 2.0f);
				if (alpha > 255.0f)
				{
					alpha = 255.0f;
				}
				else if (alpha < 0.0f)
				{
					alpha = 0.0f;
				}
				tess.svars.colors[i][3] = (byte)(alpha);
				continue;
			}

			if (dot < highest)
			{
				if (dot > lowest)
				{
					if (dot < lowest + range / 2)
					{
						alpha = ((float)pStage->constantColor[3] * ((dot - lowest) / (range / 2)));
					}
					else
					{
						alpha = ((float)pStage->constantColor[3] * (1.0f - ((dot - lowest - range / 2) / (range / 2))));
					}
					if (alpha > 255.0f)
					{
						alpha = 255.0f;
					}
					else if (alpha < 0.0f)
					{
						alpha = 0.0f;
					}

					// finally, scale according to the entity's alpha
					if (backEnd.currentEntity->e.hModel)
					{
						alpha *= (float)backEnd.currentEntity->e.shaderRGBA[3] / 255.0f;
					}

					tess.svars.colors[i][3] = (byte)(alpha);
				}
				else
				{
					tess.svars.colors[i][3] = 0;
				}
			}
			else
			{
				tess.svars.colors[i][3] = 0;
			}
		}
	}
	break;
	case AGEN_VERTEX:
		if (pStage->rgbGen != CGEN_VERTEX)
		{
			int i;

			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
		break;
	case AGEN_ONE_MINUS_VERTEX:
	{
		int i;

		for (i = 0; i < tess.numVertexes; i++)
		{
			tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
		}
	}
	break;
	case AGEN_PORTAL:
	{
		unsigned char alpha;
		float         len;
		vec3_t        v;
		int           i;

		for (i = 0; i < tess.numVertexes; i++)
		{
			VectorSubtract(tess.xyz[i], backEnd.viewParms.orientation.origin, v);
			len = VectorLength(v);

			len /= tess.shader->portalRange;

			if (len < 0)
			{
				alpha = 0;
			}
			else if (len > 1)
			{
				alpha = 0xff;
			}
			else
			{
				alpha = len * 0xff;
			}

			tess.svars.colors[i][3] = alpha;
		}
	}
	break;
	}

	// fog adjustment for colors to fade out as fog increases
	if (tess.fogNum && !tess.shader->noFog)
	{
		switch (pStage->adjustColorsForFog)
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog(( unsigned char * ) tess.svars.colors);
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog(( unsigned char * ) tess.svars.colors);
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog(( unsigned char * ) tess.svars.colors);
			break;
		case ACFF_NONE:
			break;
		}
	}
}

/**
 * @brief ComputeTexCoords
 * @param[in] pStage
 */
static void ComputeTexCoords(shaderStage_t *pStage)
{
	int i;
	int b;
	int tm;

	for (b = 0; b < NUM_TEXTURE_BUNDLES; b++)
	{
		// generate the texture coordinates
		switch (pStage->bundle[b].tcGen)
		{
		case TCGEN_IDENTITY:
			Com_Memset(tess.svars.texcoords[b], 0, sizeof(float) * 2 * tess.numVertexes);
			break;
		case TCGEN_TEXTURE:
			for (i = 0 ; i < tess.numVertexes ; i++)
			{
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
			}
			break;
		case TCGEN_LIGHTMAP:
			for (i = 0 ; i < tess.numVertexes ; i++)
			{
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
			}
			break;
		case TCGEN_VECTOR:
			for (i = 0 ; i < tess.numVertexes ; i++)
			{
				tess.svars.texcoords[b][i][0] = DotProduct(tess.xyz[i], pStage->bundle[b].tcGenVectors[0]);
				tess.svars.texcoords[b][i][1] = DotProduct(tess.xyz[i], pStage->bundle[b].tcGenVectors[1]);
			}
			break;
		case TCGEN_FOG:
			RB_CalcFogTexCoords(( float * ) tess.svars.texcoords[b]);
			break;
		case TCGEN_ENVIRONMENT_MAPPED:
			RB_CalcEnvironmentTexCoords(( float * ) tess.svars.texcoords[b]);
			break;
		case TCGEN_FIRERISEENV_MAPPED:
			RB_CalcFireRiseEnvTexCoords(( float * ) tess.svars.texcoords[b]);
			break;
		case TCGEN_BAD:
			return;
		}

		// alter texture coordinates
		for (tm = 0; tm < pStage->bundle[b].numTexMods ; tm++)
		{
			switch (pStage->bundle[b].texMods[tm].type)
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;        // break out of for loop
				break;

			case TMOD_SWAP:
				RB_CalcSwapTexCoords(( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords(&pStage->bundle[b].texMods[tm].wave,
				                          ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords(backEnd.currentEntity->e.shaderTexCoord,
				                       ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords(pStage->bundle[b].texMods[tm].scroll,
				                       ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords(pStage->bundle[b].texMods[tm].scale,
				                      ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_STRETCH:
				RB_CalcStretchTexCoords(&pStage->bundle[b].texMods[tm].wave,
				                        ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords(&pStage->bundle[b].texMods[tm],
				                          ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords(pStage->bundle[b].texMods[tm].rotateSpeed,
				                       ( float * ) tess.svars.texcoords[b]);
				break;

			default:
				Ren_Drop("ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle[b].texMods[tm].type, tess.shader->name);
			}
		}
	}
}


/**
 * @brief renderer1's DrawTris (r_showTris, r_trisColor).
 * GL draws the triangles with glPolygonMode(GL_LINE); here each triangle
 * edge becomes a line. r_showTris 2 draws on top (depth range 0,0) like
 * GL1; mode 1's line polygon offset is approximated by pulling the depth
 * range in very slightly (Vulkan depth bias doesn't apply to lines).
 */
static void DrawTris(shaderCommands_t *input)
{
	static unsigned int lineIndexes[SHADER_MAX_INDEXES * 2];
	static byte         colors[SHADER_MAX_VERTEXES][4];
	char                *s        = r_trisColor->string;
	vec4_t              trisColor = { 1, 1, 1, 1 };
	unsigned int        stateBits = 0;
	int                 i, numLineIndexes = 0;

	// exclude 2d from drawing tris
	if (backEnd.projection2D == qtrue)
	{
		return;
	}

	if (*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X'))
	{
		s += 2;
		if (Q_IsHexColorString(s))
		{
			trisColor[0] = ((float)(gethex(*(s)) * 16 + gethex(*(s + 1)))) / 255.00f;
			trisColor[1] = ((float)(gethex(*(s + 2)) * 16 + gethex(*(s + 3)))) / 255.00f;
			trisColor[2] = ((float)(gethex(*(s + 4)) * 16 + gethex(*(s + 5)))) / 255.00f;

			if (Q_HexColorStringHasAlpha(s))
			{
				trisColor[3] = ((float)(gethex(*(s + 6)) * 16 + gethex(*(s + 7)))) / 255.00f;
			}
		}
	}
	else
	{
		char *token;

		for (i = 0 ; i < 4 ; i++)
		{
			token = COM_Parse(&s);
			if (token[0])
			{
				trisColor[i] = Q_atof(token);
			}
			else
			{
				trisColor[i] = 1.f;
			}
		}

		if (trisColor[3] == 0.f)
		{
			trisColor[3] = 1.f;
		}
	}

	if (trisColor[3] < 1.f)
	{
		stateBits |= (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}

	if (r_showTris->integer == 2)
	{
		stateBits |= GLS_DEPTHMASK_TRUE;
		VK_SetDepthRange(0, 0);
	}
	else
	{
		VK_SetDepthRange(0, 0.99995f);
	}

	for (i = 0; i < input->numVertexes; i++)
	{
		colors[i][0] = (byte)(trisColor[0] * 255.0f);
		colors[i][1] = (byte)(trisColor[1] * 255.0f);
		colors[i][2] = (byte)(trisColor[2] * 255.0f);
		colors[i][3] = (byte)(trisColor[3] * 255.0f);
	}

	for (i = 0; i + 2 < input->numIndexes; i += 3)
	{
		lineIndexes[numLineIndexes++] = input->indexes[i];
		lineIndexes[numLineIndexes++] = input->indexes[i + 1];
		lineIndexes[numLineIndexes++] = input->indexes[i + 1];
		lineIndexes[numLineIndexes++] = input->indexes[i + 2];
		lineIndexes[numLineIndexes++] = input->indexes[i + 2];
		lineIndexes[numLineIndexes++] = input->indexes[i];
	}

	VK_DrawImmediate3D((const float (*)[4]) input->xyz, (const byte(*)[4]) colors, NULL, input->numVertexes,
	                   lineIndexes, numLineIndexes, qtrue, tr.whiteImage, stateBits);

	VK_SetDepthRange(0, 1);
}

/**
 * @brief renderer1's DrawNormals (r_showNormals), unchanged:
 * it uses immediate mode, which is emulated (vk_gl_compat.h).
 */
static void DrawNormals(shaderCommands_t *input)
{
	vec3_t temp;

	GL_Bind(tr.whiteImage);
	VKimm_Color3f(1, 1, 1);
	VK_SetDepthRange(0, 0);    // never occluded
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);

	// light direction
	if (r_showNormals->integer == 2)
	{
		trRefEntity_t *ent = backEnd.currentEntity;
		vec3_t        temp2;

		if (ent->e.renderfx & RF_LIGHTING_ORIGIN)
		{
			VectorSubtract(ent->e.lightingOrigin, backEnd.orientation.origin, temp2);
		}
		else
		{
			VectorClear(temp2);
		}
		temp[0] = DotProduct(temp2, backEnd.orientation.axis[0]);
		temp[1] = DotProduct(temp2, backEnd.orientation.axis[1]);
		temp[2] = DotProduct(temp2, backEnd.orientation.axis[2]);

		VKimm_Color3f(ent->ambientLight[0] / 255, ent->ambientLight[1] / 255, ent->ambientLight[2] / 255);
		VKimm_PointSize(5);
		VKimm_Begin(GL_POINTS);
		VKimm_Vertex3fv(temp);
		VKimm_End();
		VKimm_PointSize(1);

		if (Q_fabs(VectorLengthSquared(ent->lightDir) - 1.0f) > 0.2f)
		{
			VKimm_Color3f(1, 0, 0);
		}
		else
		{
			VKimm_Color3f(ent->directedLight[0] / 255, ent->directedLight[1] / 255, ent->directedLight[2] / 255);
		}
		VKimm_Begin(GL_LINES);
		VKimm_Vertex3fv(temp);
		VectorMA(temp, 32, ent->lightDir, temp);
		VKimm_Vertex3fv(temp);
		VKimm_End();
	}
	// normals drawing
	else
	{
		int i;

		VKimm_Begin(GL_LINES);
		for (i = 0 ; i < input->numVertexes ; i++)
		{
			VKimm_Vertex3fv(input->xyz[i]);
			VectorMA(input->xyz[i], r_normalLength->value, input->normal[i], temp);
			VKimm_Vertex3fv(temp);
		}
		VKimm_End();
	}

	VK_SetDepthRange(0, 1);
}

/**
 * @brief renderer1's DebugShaderSurfaceFlags
 * (r_debugShaderSurfaceFlags) - paints surfaces whose BSP shader has the
 * given surface flags green and skips their remaining stages.
 */
static qboolean DebugShaderSurfaceFlags(shaderCommands_t *input)
{
	// Debug draw shaders with a given Surface Flag
	if (r_debugShaderSurfaceFlags->integer)
	{
		dshader_t *dsh;

		dsh = R_FindBspShaderByName(input->shader->name);
		if (dsh != NULL && dsh->surfaceFlags & strtoul(r_debugShaderSurfaceFlags->string, NULL, 0) /* XXX : needs to be parsed as unsigned */)
		{
			int j;
			for (j = 0; j < input->numVertexes; j++)
			{
				input->svars.colors[j][0] = 0;
				input->svars.colors[j][1] = 255;
				input->svars.colors[j][2] = 0;
				input->svars.colors[j][3] = 255;
			}
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Adds the contribution of every dynamic light in lightBits to
 * tess.svars.colors (renderer1's DynamicLightSinglePass/DynamicLightPass
 * vertex loop), then collects the triangles with any lit vertex.
 * @return number of indexes written to hitIndexes
 */
static int RB_ComputeDlightColors(int lightBits, glIndex_t *hitIndexes)
{
	int      i, l, a, b, c, color, *intColors;
	vec3_t   origin;
	byte     *colors;
	int      numIndexes;
	float    radius, radiusInverseCubed;
	float    intensity, remainder, modulate;
	vec3_t   floatColor, dir;
	dlight_t *dl;

	Com_Memset(tess.svars.colors, 0, tess.numVertexes * sizeof(tess.svars.colors[0]));

	for (l = 0; l < backEnd.refdef.num_dlights; l++)
	{
		if (!(lightBits & (1 << l)))
		{
			continue;
		}

		dl = &backEnd.refdef.dlights[l];
		VectorCopy(dl->transformed, origin);
		radius             = dl->radius;
		radiusInverseCubed = dl->radiusInverseCubed;
		intensity          = dl->intensity;
		floatColor[0]      = dl->color[0] * 255.0f;
		floatColor[1]      = dl->color[1] * 255.0f;
		floatColor[2]      = dl->color[2] * 255.0f;

		// directional lights have max intensity and washout remainder intensity
		if (dl->flags & REF_DIRECTED_DLIGHT)
		{
			remainder = intensity * 0.125f;
		}
		else
		{
			remainder = 0.0f;
		}

		colors = tess.svars.colors[0];
		for (i = 0; i < tess.numVertexes; i++, colors += 4)
		{
			backEnd.pc.c_dlightVertexes++;

			// directional dlight, origin is a directional normal
			if (dl->flags & REF_DIRECTED_DLIGHT)
			{
				// twosided surfaces use absolute value of the calculated lighting
				modulate = intensity * DotProduct(dl->origin, tess.normal[i]);
				if (tess.shader->cullType == CT_TWO_SIDED)
				{
					modulate = Q_fabs(modulate);
				}
				modulate += remainder;
			}
			// ball dlight
			else
			{
				dir[0] = radius - Q_fabs(origin[0] - tess.xyz[i][0]);
				if (dir[0] <= 0.0f)
				{
					continue;
				}
				dir[1] = radius - Q_fabs(origin[1] - tess.xyz[i][1]);
				if (dir[1] <= 0.0f)
				{
					continue;
				}
				dir[2] = radius - Q_fabs(origin[2] - tess.xyz[i][2]);
				if (dir[2] <= 0.0f)
				{
					continue;
				}

				modulate = intensity * dir[0] * dir[1] * dir[2] * radiusInverseCubed;
			}

			if (modulate < (1.0f / 128.0f))
			{
				continue;
			}
			else if (modulate > 1.0f)
			{
				modulate = 1.0f;
			}

			color     = colors[0] + (int)(floatColor[0] * modulate);
			colors[0] = color > 255 ? 255 : (byte)color;
			color     = colors[1] + (int)(floatColor[1] * modulate);
			colors[1] = color > 255 ? 255 : (byte)color;
			color     = colors[2] + (int)(floatColor[2] * modulate);
			colors[2] = color > 255 ? 255 : (byte)color;
		}
	}

	// build a list of triangles that need light
	intColors  = (int *) tess.svars.colors;
	numIndexes = 0;
	for (i = 0; i < tess.numIndexes; i += 3)
	{
		a = tess.indexes[i];
		b = tess.indexes[i + 1];
		c = tess.indexes[i + 2];
		if (!(intColors[a] | intColors[b] | intColors[c]))
		{
			continue;
		}
		hitIndexes[numIndexes++] = a;
		hitIndexes[numIndexes++] = b;
		hitIndexes[numIndexes++] = c;
	}

	return numIndexes;
}

/**
 * @brief Draws one additive dynamic light pass over the hit triangles:
 * white texture * vertex light color, blended DST_COLOR/ONE with depth
 * func equal so it only lands on the surface just drawn.
 */
static void RB_DrawDlightPass(const glIndex_t *hitIndexes, int numIndexes)
{
	if (numIndexes == 0)
	{
		return;
	}

	R_FogOff();
	VK_Draw3DIndexes(&tess, hitIndexes, numIndexes, tr.whiteImage,
	                 GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL, tess.shader->cullType);
	backEnd.pc.c_totalIndexes  += numIndexes;
	backEnd.pc.c_dlightIndexes += numIndexes;
	R_FogOn();
}

/**
 * @brief renderer1's dynamic light step, run after a
 * surface's own stages. r_dynamicLight 2 draws one pass per light
 * (DynamicLightPass), otherwise all lights are summed into one pass
 * (DynamicLightSinglePass).
 */
/**
 * @brief True if renderer1 draws dlight passes for a batch of this shader
 * lit by dlightBits (RB_DynamicLights()'s conditions).
 */
qboolean RB_ShaderTakesDlights(int dlightBits)
{
	return (dlightBits && tess.shader->fogPass && !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY)) &&
	        backEnd.refdef.num_dlights) ? qtrue : qfalse;
}

static void RB_DynamicLights(void)
{
	static glIndex_t hitIndexes[SHADER_MAX_INDEXES];
	int              l;

	// static batches have no tess vertices: their dlights are folded into
	// the static draw (VK_Draw3DStatic, see RB_StaticDlightsOk)
	if (tess.vkStatic || !RB_ShaderTakesDlights(tess.dlightBits))
	{
		return;
	}

	if (r_dynamicLight->integer == 2)
	{
		for (l = 0; l < backEnd.refdef.num_dlights; l++)
		{
			if (tess.dlightBits & (1 << l))
			{
				RB_DrawDlightPass(hitIndexes, RB_ComputeDlightColors(1 << l, hitIndexes));
			}
		}
	}
	else
	{
		RB_DrawDlightPass(hitIndexes, RB_ComputeDlightColors(tess.dlightBits, hitIndexes));
	}
}

/**
 * @brief renderer1's RB_FogPass - blends a Q3 fog volume
 * (tr.world->fogs[tess.fogNum]) over the surface using tr.fogImage with
 * per-vertex fog texcoords from RB_CalcFogTexCoords().
 */
static void RB_FogPass(void)
{
	fog_t *fog;
	int   i;

	if (!tess.fogNum || !tess.shader->fogPass)
	{
		return;
	}

	// no fog pass in snooper
	if ((tr.refdef.rdflags & RDF_SNOOPERVIEW) || tess.shader->noFog || !r_wolfFog->integer)
	{
		return;
	}

	// no world, no fogging
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	fog = tr.world->fogs + tess.fogNum;

	for (i = 0; i < tess.numVertexes; i++)
	{
		*( int * )&tess.svars.colors[i] = fog->shader->fogParms.colorInt;
	}

	RB_CalcFogTexCoords(( float * ) tess.svars.texcoords[0]);

	VK_Draw3D(&tess, tr.fogImage,
	          (tess.shader->fogPass == FP_EQUAL)
	          ? (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL)
	          : (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA),
	          tess.shader->cullType);
}

/**
 * @brief 2D/UI part of renderer1's RB_StageIteratorGeneric: deforms, then
 * per stage ComputeColors/ComputeTexCoords and a draw through the Vulkan
 * UI pipeline with that stage's image and state bits. The UI pipeline
 * samples one texture, so a collapsed multitexture stage is drawn as the
 * two passes it was collapsed from (CollapseMultitexture only merges an
 * opaque stage with a modulating one, or two additive stages): bundle 1
 * goes on top with GL_DST_COLOR/GL_ZERO for GL_MODULATE or GL_ONE/GL_ONE
 * for GL_ADD.
 */
static void RB_IterateStages2D(shaderCommands_t *input)
{
	int stage, i;

	RB_DeformTessGeometry();

	for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t *pStage = tess.xstages[stage];

		if (!pStage)
		{
			break;
		}

		ComputeColors(pStage);
		ComputeTexCoords(pStage);

		VK_Draw2D(input, R_GetAnimatedImage(&pStage->bundle[0]), pStage->stateBits);

		if (pStage->bundle[1].image[0] != 0)
		{
			unsigned long blend = (input->shader->multitextureEnv == GL_ADD)
			                      ? (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE)
			                      : (GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);

			for (i = 0; i < input->numVertexes; i++)
			{
				input->svars.texcoords[0][i][0] = input->svars.texcoords[1][i][0];
				input->svars.texcoords[0][i][1] = input->svars.texcoords[1][i][1];
				input->svars.colors[i][0]       = 255;
				input->svars.colors[i][1]       = 255;
				input->svars.colors[i][2]       = 255;
				input->svars.colors[i][3]       = 255;
			}

			VK_Draw2D(input, R_GetAnimatedImage(&pStage->bundle[1]),
			          (pStage->stateBits & ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) | blend);
		}
	}
}

/**
 * @brief Vulkan: true if this batch's two stages can be drawn in one draw
 * (VK_Draw3DTwoStage): stage 0 opaque (no blend or ONE ZERO), depth writing,
 * so the color stage 1 blends with is exactly stage 0's; both single
 * texture, no alpha test, depth tested, same per-stage fog; nothing changes
 * the colors on the CPU afterwards (entity fade, debug views).
 */
static qboolean RB_TwoStageFoldable(void)
{
	shaderStage_t *s0 = tess.xstages[0], *s1 = tess.xstages[1];
	unsigned long b0, b1, blend0;
	int           fadeStart = backEnd.currentEntity->e.fadeStartTime;

	if (!r_vkGpuVertex->integer || !s0 || !s1 || tess.xstages[2] || tess.vkStatic || tess.vkSkin || !tess.numVertexes ||
	    backEnd.projection2D || (fadeStart && fadeStart <= tr.refdef.time) || r_lightMap->integer || r_debugShaderSurfaceFlags->integer)
	{
		return qfalse;
	}

	if (s0->bundle[1].image[0] || s1->bundle[1].image[0])
	{
		return qfalse;
	}

	b0     = s0->stateBits;
	b1     = s1->stateBits;
	blend0 = b0 & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
	if ((blend0 && blend0 != (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO)) || !(b0 & GLS_DEPTHMASK_TRUE) ||
	    ((b0 | b1) & (GLS_ATEST_BITS | GLS_DEPTHTEST_DISABLE | GLS_POLYMODE_LINE)))
	{
		return qfalse;
	}

	// per stage fogging (detail textures) must agree: one fog state per draw
	return ((tess.shader->noFog && !s0->isFogged) == (tess.shader->noFog && !s1->isFogged)) ? qtrue : qfalse;
}

/**
 * @brief Vulkan: draws an RB_TwoStageFoldable() batch in one draw. Both
 * stages' colors and texcoords are computed on the CPU as for single stages
 * (stage 1 first, saved), then stage 1 is blended over stage 0 in
 * tr3d_2stage.frag with its blendFunc.
 */
static void RB_DrawTwoStage(void)
{
	static byte   colors1[SHADER_MAX_VERTEXES][4];
	static vec2_t texcoords1[SHADER_MAX_VERTEXES];
	shaderStage_t *s0 = tess.xstages[0], *s1 = tess.xstages[1];

	tess.vkDiffuse = qfalse;

	ComputeColors(s1);
	ComputeTexCoords(s1);
	Com_Memcpy(colors1, tess.svars.colors, tess.numVertexes * 4);
	Com_Memcpy(texcoords1, tess.svars.texcoords[0], tess.numVertexes * sizeof(vec2_t));

	ComputeColors(s0);
	ComputeTexCoords(s0);
	Com_Memcpy(tess.svars.texcoords[1], texcoords1, tess.numVertexes * sizeof(vec2_t));

	if (tess.shader->noFog && !s0->isFogged)
	{
		R_FogOff();
	}
	else
	{
		R_FogOn();
	}

	tess.vkStage = s0;
	VK_Draw3DTwoStage(&tess, R_GetAnimatedImage(&s0->bundle[0]), R_GetAnimatedImage(&s1->bundle[0]),
	                  (const byte(*)[4]) tess.svars.colors, (const byte(*)[4]) colors1,
	                  s0->stateBits, s1->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS), tess.shader->cullType);
}

/**
 * @brief renderer1's RB_StageIteratorGeneric: computes colors/texcoords per
 * stage and submits via VK_Draw3D/VK_Draw3DLightmapped, or through the UI
 * pipeline for 2D draws (RB_IterateStages2D).
 *
 * Followed by the dynamic light and fog volume passes (RB_DynamicLights,
 * RB_FogPass), as in renderer1.
 *
 * Debug views (r_showTris, r_showNormals) run from RB_EndSurface, and
 * r_debugShaderSurfaceFlags / r_lightMap are handled per stage, as in
 * renderer1.
 */
void RB_StageIteratorGeneric(void)
{
	shaderCommands_t *input  = &tess;
	shader_t         *shader = input->shader;

	if (backEnd.projection2D)
	{
		RB_IterateStages2D(input);
		return;
	}

	// Static opaque 3D geometry.
	RB_DeformTessGeometry();

	SetIteratorFog();

	if (RB_TwoStageFoldable())
	{
		RB_DrawTwoStage();
		RB_DynamicLights();
		RB_FogPass();
		return;
	}

	// no early-out on numUnfoggedPasses == 0: fog volume surfaces have no
	// stages of their own but still need RB_FogPass() below (renderer1
	// simply iterates zero stages).
	{
		int stage;

		for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = tess.xstages[stage];
			qboolean      skipRemainingStages;

			if (!pStage)
			{
				break;
			}

			tess.vkDiffuse = RB_StageGpuDiffuse(pStage);

			ComputeColors(pStage);
			ComputeTexCoords(pStage);

			// renderer1's r_debugShaderSurfaceFlags
			skipRemainingStages = DebugShaderSurfaceFlags(input);

			// per stage fogging (detail textures)
			if (tess.shader->noFog && !pStage->isFogged)
			{
				R_FogOff();
			}
			else
			{
				R_FogOn();
			}

			input->vkStage = pStage;

			if (pStage->bundle[1].image[0] != 0)
			{
				VK_Draw3DLightmapped(input, R_GetAnimatedImage(&pStage->bundle[0]), R_GetAnimatedImage(&pStage->bundle[1]),
				                     pStage->stateBits, shader->cullType);
			}
			else
			{
				unsigned long stateBits = pStage->stateBits;
				int           cullType  = shader->cullType;
				int           fadeStart = backEnd.currentEntity->e.fadeStartTime;

				// Lightmap stages should be GL_ONE GL_ZERO so they can be seen (r_lightMap)
				if (r_lightMap->integer && pStage->bundle[0].isLightmap)
				{
					stateBits = (stateBits & ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) | (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);
				}

				// fading model stuff (renderer1's RB_IterateStagesGeneric):
				// once the fade has started, force alpha blending without
				// depth writes and scale the vertex colors down to zero.
				if (fadeStart && fadeStart <= tr.refdef.time)
				{
					int   fadeEnd = backEnd.currentEntity->e.fadeEndTime;
					float alphaval;
					int   i;

					if (fadeEnd < tr.refdef.time) // entity faded out completely
					{
						continue;
					}

					alphaval = (float)(fadeEnd - tr.refdef.time) / (float)(fadeEnd - fadeStart);

					stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_DEPTHMASK_TRUE);
					stateBits |= (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
					cullType   = CT_FRONT_SIDED;

					for (i = 0; i < tess.numVertexes; i++)
					{
						tess.svars.colors[i][0] *= alphaval;
						tess.svars.colors[i][1] *= alphaval;
						tess.svars.colors[i][2] *= alphaval;
						tess.svars.colors[i][3] *= alphaval;
					}
				}

				VK_Draw3D(input, R_GetAnimatedImage(&pStage->bundle[0]), stateBits, cullType);
			}
			tess.vkDiffuse = qfalse;

			// allow skipping out to show just lightmaps during development
			if (r_lightMap->integer && (pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap))
			{
				break;
			}

			if (skipRemainingStages)
			{
				break;
			}
		}
	}

	RB_DynamicLights();
	RB_FogPass();
}

/**
 * @brief Fast-path iterator for single-texture,
 * per-vertex-lit-diffuse-color surfaces (models with CGEN_LIGHTING_DIFFUSE,
 * no tcMods). Ported from renderer1's RB_StageIteratorVertexLitTexture,
 * minus the GL immediate-mode array plumbing.
 */
void RB_StageIteratorVertexLitTexture(void)
{
	shaderCommands_t *input  = &tess;
	shader_t         *shader = input->shader;
	int              i;

	// 2D draws always take the full per-stage path (renderer1 draws them
	// with the 2D GL state in any iterator; here only the generic iterator
	// knows the 2D pipeline)
	if (backEnd.projection2D)
	{
		RB_StageIteratorGeneric();
		return;
	}

	tess.vkDiffuse = RB_StageGpuDiffuse(tess.xstages[0]);
	if (tess.vkDiffuse)
	{
		Com_Memset(tess.svars.colors, 0xff, tess.numVertexes * 4); // rgb from the vertex shader
	}
	else
	{
		RB_CalcDiffuseColor(( unsigned char * ) tess.svars.colors);
	}

	SetIteratorFog();

	// renderer1's r_debugShaderSurfaceFlags hook
	if (r_debugShaderSurfaceFlags->integer)
	{
		DebugShaderSurfaceFlags(input);
	}

	// bundle[0].tcGen is guaranteed TCGEN_TEXTURE for this fast path, so
	// texcoords can be copied directly without going through ComputeTexCoords.
	for (i = 0; i < tess.numVertexes; i++)
	{
		tess.svars.texcoords[0][i][0] = tess.texCoords[i][0][0];
		tess.svars.texcoords[0][i][1] = tess.texCoords[i][0][1];
	}

	input->vkStage = tess.xstages[0]; // stage colors for GPU-skinned batches
	VK_Draw3D(input, R_GetAnimatedImage(&tess.xstages[0]->bundle[0]), tess.xstages[0]->stateBits, shader->cullType);
	tess.vkDiffuse = qfalse;

	RB_DynamicLights();
	RB_FogPass();
}

/**
 * @brief Fast-path iterator for base+lightmap 2-stage
 * surfaces (most world/BSP geometry). Ported from renderer1's
 * RB_StageIteratorLightmappedMultitexture, minus the GL immediate-mode
 * array/multitexture plumbing.
 */
void RB_StageIteratorLightmappedMultitexture(void)
{
	shaderCommands_t *input  = &tess;
	shader_t         *shader = input->shader;
	int              i;

	if (backEnd.projection2D)
	{
		RB_StageIteratorGeneric();
		return;
	}

	SetIteratorFog();

	Com_Memcpy(tess.svars.colors, tess.constantColor255, tess.numVertexes * sizeof(tess.constantColor255[0]));

	// renderer1's r_debugShaderSurfaceFlags hook
	if (r_debugShaderSurfaceFlags->integer)
	{
		DebugShaderSurfaceFlags(input);
	}

	for (i = 0; i < tess.numVertexes; i++)
	{
		tess.svars.texcoords[0][i][0] = tess.texCoords[i][0][0];
		tess.svars.texcoords[0][i][1] = tess.texCoords[i][0][1];
		tess.svars.texcoords[1][i][0] = tess.texCoords[i][1][0];
		tess.svars.texcoords[1][i][1] = tess.texCoords[i][1][1];
	}

	input->vkStage = NULL; // constant white (constantColor255)
	VK_Draw3DLightmapped(input, R_GetAnimatedImage(&tess.xstages[0]->bundle[0]), R_GetAnimatedImage(&tess.xstages[0]->bundle[1]),
	                     GLS_DEFAULT, shader->cullType);

	RB_DynamicLights();
	RB_FogPass();
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

	if (tess.shader == tr.shadowShader)
	{
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if (r_debugSort->integer && r_debugSort->integer < tess.shader->sort)
	{
		return;
	}

	// update performance counters
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes     += tess.numVertexes;
	backEnd.pc.c_indexes      += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	// call off to shader specific tess end function
	tess.currentStageIteratorFunc();

	// draw debugging stuff
	if (r_showTris->integer)
	{
		DrawTris(input);
	}
	if (r_showNormals->integer)
	{
		DrawNormals(input);
	}

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes  = 0;
	tess.numVertexes = 0;
	tess.vkStatic    = qfalse;
	tess.vkSkin      = qfalse;
	tess.vkMesh      = qfalse;
	tess.vkIndirect  = qfalse;
}
