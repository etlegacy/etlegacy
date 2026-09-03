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
 * Video maps are not supported yet and fall back to image[0].
 */
static image_t *R_GetAnimatedImage(textureBundle_t *bundle)
{
	int64_t index;

	if (bundle->isLightmap && (backEnd.refdef.rdflags & RDF_SNOOPERVIEW))
	{
		return tr.whiteImage;
	}

	if (bundle->isVideoMap || bundle->numImageAnimations <= 1)
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
		RB_CalcDiffuseColor(( unsigned char * ) tess.svars.colors);
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
 * @brief For 2D/UI draws (backEnd.projection2D), submits the tessellated
 * batch via the Vulkan UI pipeline. For 3D surfaces, computes
 * colors/texcoords per stage and submits via VK_Draw3D/VK_Draw3DLightmapped.
 *
 * @note Deferred scope: dynamic lighting/dlights, in-shader fog, polygon
 * offset (decals), r_lightMap debug override, and true multi-pass combiner
 * blending beyond base+lightmap are not yet implemented.
 */
void RB_StageIteratorGeneric(void)
{
	shaderCommands_t *input  = &tess;
	shader_t         *shader = input->shader;
	image_t          *image;

	if (backEnd.projection2D)
	{
		unsigned long stateBits = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;

		image = NULL;
		if (input->xstages && input->xstages[0])
		{
			image     = R_GetAnimatedImage(&input->xstages[0]->bundle[0]);
			stateBits = input->xstages[0]->stateBits;
		}

		VK_Draw2D(input, image, stateBits);
		return;
	}

	// static opaque 3D geometry.
	RB_DeformTessGeometry();

	SetIteratorFog();

	if (shader->numUnfoggedPasses == 0)
	{
		return;
	}

	{
		int stage;

		for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = tess.xstages[stage];

			if (!pStage)
			{
				break;
			}

			ComputeColors(pStage);
			ComputeTexCoords(pStage);

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
		}
	}
}

/**
 * @brief fast-path iterator for single-texture,
 * per-vertex-lit-diffuse-color surfaces (models with CGEN_LIGHTING_DIFFUSE,
 * no tcMods). Ported from renderer1's RB_StageIteratorVertexLitTexture,
 * minus the GL immediate-mode array plumbing.
 */
void RB_StageIteratorVertexLitTexture(void)
{
	shaderCommands_t *input  = &tess;
	shader_t         *shader = input->shader;
	int              i;

	RB_CalcDiffuseColor(( unsigned char * ) tess.svars.colors);

	SetIteratorFog();

	// bundle[0].tcGen is guaranteed TCGEN_TEXTURE for this fast path, so
	// texcoords can be copied directly without going through ComputeTexCoords.
	for (i = 0; i < tess.numVertexes; i++)
	{
		tess.svars.texcoords[0][i][0] = tess.texCoords[i][0][0];
		tess.svars.texcoords[0][i][1] = tess.texCoords[i][0][1];
	}

	VK_Draw3D(input, R_GetAnimatedImage(&tess.xstages[0]->bundle[0]), tess.xstages[0]->stateBits, shader->cullType);
}

/**
 * @brief fast-path iterator for base+lightmap 2-stage
 * surfaces (most world/BSP geometry). Ported from renderer1's
 * RB_StageIteratorLightmappedMultitexture, minus the GL immediate-mode
 * array/multitexture plumbing.
 */
void RB_StageIteratorLightmappedMultitexture(void)
{
	shaderCommands_t *input  = &tess;
	shader_t         *shader = input->shader;
	int              i;

	SetIteratorFog();

	Com_Memcpy(tess.svars.colors, tess.constantColor255, tess.numVertexes * sizeof(tess.constantColor255[0]));

	for (i = 0; i < tess.numVertexes; i++)
	{
		tess.svars.texcoords[0][i][0] = tess.texCoords[i][0][0];
		tess.svars.texcoords[0][i][1] = tess.texCoords[i][0][1];
		tess.svars.texcoords[1][i][0] = tess.texCoords[i][1][0];
		tess.svars.texcoords[1][i][1] = tess.texCoords[i][1][1];
	}

	VK_Draw3DLightmapped(input, R_GetAnimatedImage(&tess.xstages[0]->bundle[0]), R_GetAnimatedImage(&tess.xstages[0]->bundle[1]),
	                     GLS_DEFAULT, shader->cullType);
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
