/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
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
/**
 * @file renderer2/tr_shade.c
 */

#include "tr_local.h"

/*
=================================================================================
THIS ENTIRE FILE IS BACK END!

This file deals with applying shaders to surface data in the tess struct.
=================================================================================
*/

/*
static void MyMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void* *indices, GLsizei primcount)
{
    int			i;

    for (i = 0; i < primcount; i++)
    {
        if (count[i] > 0)
            glDrawElements(mode, count[i], type, indices[i]);
    }
}
*/

void Tess_DrawElements()
{
	if ((tess.numIndexes == 0 || tess.numVertexes == 0) && tess.multiDrawPrimitives == 0)
	{
		return;
	}

	// move tess data through the GPU, finally
	if (glState.currentVBO && glState.currentIBO)
	{
		if (tess.multiDrawPrimitives)
		{
			int i;

			glMultiDrawElements(GL_TRIANGLES, tess.multiDrawCounts, GL_INDEX_TYPE, (const GLvoid **) tess.multiDrawIndexes, tess.multiDrawPrimitives);

			backEnd.pc.c_multiDrawElements++;
			backEnd.pc.c_multiDrawPrimitives += tess.multiDrawPrimitives;

			backEnd.pc.c_vboVertexes += tess.numVertexes;

			for (i = 0; i < tess.multiDrawPrimitives; i++)
			{
				backEnd.pc.c_multiVboIndexes += tess.multiDrawCounts[i];
				backEnd.pc.c_indexes         += tess.multiDrawCounts[i];
			}
		}
		else
		{
			glDrawElements(GL_TRIANGLES, tess.numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(0));

			backEnd.pc.c_drawElements++;

			backEnd.pc.c_vboVertexes += tess.numVertexes;
			backEnd.pc.c_vboIndexes  += tess.numIndexes;

			backEnd.pc.c_indexes  += tess.numIndexes;
			backEnd.pc.c_vertexes += tess.numVertexes;
		}
	}
	else
	{
		glDrawElements(GL_TRIANGLES, tess.numIndexes, GL_INDEX_TYPE, tess.indexes);

		backEnd.pc.c_drawElements++;

		backEnd.pc.c_indexes  += tess.numIndexes;
		backEnd.pc.c_vertexes += tess.numVertexes;
	}
}

/*
=============================================================
SURFACE SHADERS
=============================================================
*/

shaderCommands_t tess;

static void BindLightMap()
{
	image_t *lightmap;

	if (tess.lightmapNum >= 0 && tess.lightmapNum < tr.lightmaps.currentElements)
	{
		lightmap = (image_t *) Com_GrowListElement(&tr.lightmaps, tess.lightmapNum);
	}
	else
	{
		lightmap = NULL;
	}

	if (!tr.lightmaps.currentElements || !lightmap)
	{
		GL_Bind(tr.whiteImage);
		return;
	}

	GL_Bind(lightmap);
}

/*
=================
BindDeluxeMap
=================
*/
static void BindDeluxeMap()
{
	image_t *deluxemap;

	if (tess.lightmapNum >= 0 && tess.lightmapNum < tr.deluxemaps.currentElements)
	{
		deluxemap = (image_t *) Com_GrowListElement(&tr.deluxemaps, tess.lightmapNum);
	}
	else
	{
		deluxemap = NULL;
	}

	if (!tr.deluxemaps.currentElements || !deluxemap)
	{
		GL_Bind(tr.flatImage);
		return;
	}

	GL_Bind(deluxemap);
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris()
{
	vec_t *color;
	Ren_LogComment("--- DrawTris ---\n");

	SetMacrosAndSelectProgram(trProg.gl_genericShader,
	                          USE_ALPHA_TESTING, qfalse,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, qfalse,
	                          USE_TCGEN_ENVIRONMENT, qfalse,
	                          USE_TCGEN_LIGHTMAP, qfalse);

	GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);

	if (r_showBatches->integer || r_showLightBatches->integer)
	{
		color = g_color_table[backEnd.pc.c_batches % 8];
	}
	else if (glState.currentVBO == tess.vbo)
	{
		color = colorRed;
	}
	else if (glState.currentVBO)
	{
		color = colorBlue;
	}
	else
	{
		color = colorWhite;
	}

	SetUniformVec4(UNIFORM_COLOR, color);

	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CONST, AGEN_CONST);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// u_DeformGen
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(tr.whiteImage);

	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrices[TB_COLORMAP]);

	glDepthRange(0, 0);

	Tess_DrawElements();

	glDepthRange(0, 1);
}

/*
==============
Tess_Begin

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a Tess_End due
to overflow.
==============
*/
// *INDENT-OFF*
void Tess_Begin(void (*stageIteratorFunc)(),
                void (*stageIteratorFunc2)(),
                shader_t *surfaceShader, shader_t *lightShader,
                qboolean skipTangentSpaces,
                qboolean skipVBO,
                int lightmapNum,
                int fogNum)
{
	shader_t *state;
	qboolean isSky;

	tess.numIndexes  = 0;
	tess.numVertexes = 0;

	tess.multiDrawPrimitives = 0;

	// materials are optional
	if (surfaceShader != NULL)
	{
		state = (surfaceShader->remappedShader) ? surfaceShader->remappedShader : surfaceShader;

		tess.surfaceShader    = state;
		tess.surfaceStages    = state->stages;
		tess.numSurfaceStages = state->numStages;
	}
	else
	{
		state = NULL;

		tess.numSurfaceStages = 0;
		tess.surfaceShader    = NULL;
		tess.surfaceStages    = NULL;
	}

	if (state != NULL && state->isSky != qfalse)
	{
		isSky = qtrue;
	}
	else
	{
		isSky = qfalse;
	}

	tess.lightShader = lightShader;

	tess.stageIteratorFunc  = stageIteratorFunc;
	tess.stageIteratorFunc2 = stageIteratorFunc2;

	if (!tess.stageIteratorFunc)
	{
		//tess.stageIteratorFunc = &Tess_StageIteratorGeneric;
		Ren_Fatal("tess.stageIteratorFunc == NULL");
	}

	if (tess.stageIteratorFunc == &Tess_StageIteratorGeneric)
	{
		if (isSky)
		{
			tess.stageIteratorFunc  = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorGeneric;
		}
	}
	else if (tess.stageIteratorFunc == &Tess_StageIteratorDepthFill)
	{
		if (isSky)
		{
			tess.stageIteratorFunc  = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorDepthFill;
		}
	}

	tess.skipTangentSpaces = skipTangentSpaces;
	tess.skipVBO           = skipVBO;
	tess.lightmapNum       = lightmapNum;
	tess.fogNum            = fogNum;

	Ren_LogComment("--- Tess_Begin( surfaceShader = %s, lightShader = %s, skipTangentSpaces = %i, lightmapNum = %i, fogNum = %i) ---\n", tess.surfaceShader->name, tess.lightShader ? tess.lightShader->name : NULL, tess.skipTangentSpaces, tess.lightmapNum, tess.fogNum);
}
// *INDENT-ON*

static void Render_generic(int stage)
{
	shaderStage_t *pStage;
	colorGen_t    rgbGen;
	alphaGen_t    alphaGen;

	Ren_LogComment("--- Render_generic ---\n");

	pStage = tess.surfaceStages[stage];

	GL_State(pStage->stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_genericShader,
	                          USE_ALPHA_TESTING, (pStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_TCGEN_ENVIRONMENT, pStage->tcGen_Environment,
	                          USE_TCGEN_LIGHTMAP, pStage->tcGen_Lightmap);
	// end choose right shader program ------------------------------

	// set uniforms
	if (pStage->tcGen_Environment)
	{
		// calculate the environment texcoords in object space
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.orientation.viewOrigin);
	}

	// u_AlphaTest
	GLSL_SetUniform_AlphaTest(pStage->stateBits);


	// u_ColorGen
	switch (pStage->rgbGen)
	{
	case CGEN_VERTEX:
	case CGEN_ONE_MINUS_VERTEX:
		rgbGen = pStage->rgbGen;
		break;
	default:
		rgbGen = CGEN_CONST;
		break;
	}

	// u_AlphaGen
	switch (pStage->alphaGen)
	{
	case AGEN_VERTEX:
	case AGEN_ONE_MINUS_VERTEX:
		alphaGen = pStage->alphaGen;
		break;
	default:
		alphaGen = AGEN_CONST;
		break;
	}

	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, rgbGen, alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);
	SetUniformMatrix16(UNIFORM_MODELMATRIX, MODEL_MATRIX);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// u_VertexInterpolation
	if (glState.vertexAttribsInterpolation > 0)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	// u_DeformGen
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	BindAnimatedImage(&pStage->bundle[TB_COLORMAP]);
	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrices[TB_COLORMAP]);

	GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_vertexLighting_DBS_entity(int stage)
{
	vec3_t        viewOrigin;
	vec3_t        ambientColor;
	vec3_t        lightDir;
	vec4_t        lightColor;
	uint32_t      stateBits;
	shaderStage_t *pStage       = tess.surfaceStages[stage];
	qboolean      normalMapping = qfalse;

	Ren_LogComment("--- Render_vertexLighting_DBS_entity ---\n");

	stateBits = pStage->stateBits;
	GL_State(stateBits);
	if (r_normalMapping->integer && (pStage->bundle[TB_NORMALMAP].image[0] != NULL))
	{
		normalMapping = qtrue;
	}

	SetMacrosAndSelectProgram(trProg.gl_vertexLightingShader_DBS_entity,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, (pStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, normalMapping,
	                          USE_PARALLAX_MAPPING, normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax,
	                          USE_REFLECTIVE_SPECULAR, normalMapping && tr.cubeHashTable != NULL);

	// now we are ready to set the shader program uniforms
	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// set uniforms
	VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);   // in world space
	VectorCopy(backEnd.currentEntity->ambientLight, ambientColor);
	//ClampColor(ambientColor);
	VectorCopy(backEnd.currentEntity->directedLight, lightColor);
	//ClampColor(directedLight);

	// lightDir = L vector which means surface to light
	VectorCopy(backEnd.currentEntity->lightDir, lightDir);

	// u_AlphaTest
	GLSL_SetUniform_AlphaTest(pStage->stateBits);
	SetUniformVec3(UNIFORM_AMBIENTCOLOR, ambientColor);
	SetUniformVec3(UNIFORM_VIEWORIGIN, viewOrigin);
	SetUniformVec3(UNIFORM_LIGHTDIR, lightDir);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, lightColor);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	if (glState.vertexAttribsInterpolation > 0)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (r_parallaxMapping->integer && tess.surfaceShader->parallax)
	{
		float depthScale;

		depthScale = RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value);
		SetUniformFloat(UNIFORM_DEPTHSCALE, depthScale);
	}

	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;
		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);

	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	if (normalMapping)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		if (pStage->bundle[TB_NORMALMAP].image[0])
		{
			GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.flatImage);
		}

		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		// bind u_SpecularMap
		SelectTexture(TEX_SPECULAR);
		if (pStage->bundle[TB_SPECULARMAP].image[0])
		{
			GL_Bind(pStage->bundle[TB_SPECULARMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.blackImage);
		}

		SetUniformMatrix16(UNIFORM_SPECULARTEXTUREMATRIX, tess.svars.texMatrices[TB_SPECULARMAP]);

		//if(r_reflectionMapping->integer)
		{
			cubemapProbe_t *cubeProbeNearest;
			cubemapProbe_t *cubeProbeSecondNearest;

			if (backEnd.currentEntity && (backEnd.currentEntity != &tr.worldEntity))
			{
				R_FindTwoNearestCubeMaps(backEnd.currentEntity->e.origin, &cubeProbeNearest, &cubeProbeSecondNearest);
			}
			else
			{
				// FIXME position
				R_FindTwoNearestCubeMaps(backEnd.viewParms.orientation.origin, &cubeProbeNearest, &cubeProbeSecondNearest);
			}


			if (cubeProbeNearest == NULL && cubeProbeSecondNearest == NULL)
			{
				Ren_LogComment("cubeProbeNearest && cubeProbeSecondNearest == NULL\n");

				// bind u_EnvironmentMap0
				SelectTexture(TEX_ENVMAP0);
				GL_Bind(tr.whiteCubeImage);

				// bind u_EnvironmentMap1
				SelectTexture(TEX_ENVMAP1);
				GL_Bind(tr.whiteCubeImage);
			}
			else if (cubeProbeNearest == NULL)
			{
				Ren_LogComment("cubeProbeNearest == NULL\n");

				// bind u_EnvironmentMap0
				SelectTexture(TEX_ENVMAP0);
				GL_Bind(cubeProbeSecondNearest->cubemap);

				// u_EnvironmentInterpolation
				SetUniformFloat(UNIFORM_ENVIRONMENTINTERPOLATION, 0.0);
			}
			else if (cubeProbeSecondNearest == NULL)
			{
				Ren_LogComment("cubeProbeSecondNearest == NULL\n");

				// bind u_EnvironmentMap0
				SelectTexture(TEX_ENVMAP0);
				GL_Bind(cubeProbeNearest->cubemap);

				// bind u_EnvironmentMap1
				//SelectTexture(TEX_ENVMAP1);
				//GL_Bind(cubeProbeNearest->cubemap);

				// u_EnvironmentInterpolation
				SetUniformFloat(UNIFORM_ENVIRONMENTINTERPOLATION, 0.0);
			}
			else
			{
				float cubeProbeNearestDistance, cubeProbeSecondNearestDistance, interpolate;

				if (backEnd.currentEntity && (backEnd.currentEntity != &tr.worldEntity))
				{
					cubeProbeNearestDistance       = Distance(backEnd.currentEntity->e.origin, cubeProbeNearest->origin);
					cubeProbeSecondNearestDistance = Distance(backEnd.currentEntity->e.origin, cubeProbeSecondNearest->origin);
				}
				else
				{
					// FIXME position
					cubeProbeNearestDistance       = Distance(backEnd.viewParms.orientation.origin, cubeProbeNearest->origin);
					cubeProbeSecondNearestDistance = Distance(backEnd.viewParms.orientation.origin, cubeProbeSecondNearest->origin);
				}

				interpolate = cubeProbeNearestDistance / (cubeProbeNearestDistance + cubeProbeSecondNearestDistance);

				Ren_LogComment("cubeProbeNearestDistance = %f, cubeProbeSecondNearestDistance = %f, interpolation = %f\n",
				               cubeProbeNearestDistance, cubeProbeSecondNearestDistance, interpolate);

				// bind u_EnvironmentMap0
				SelectTexture(TEX_ENVMAP0);
				GL_Bind(cubeProbeNearest->cubemap);

				// bind u_EnvironmentMap1
				SelectTexture(TEX_ENVMAP1);
				GL_Bind(cubeProbeSecondNearest->cubemap);

				// u_EnvironmentInterpolation
				SetUniformFloat(UNIFORM_ENVIRONMENTINTERPOLATION, interpolate);
			}
		}
	}
	GLSL_SetRequiredVertexPointers(trProg.gl_vertexLightingShader_DBS_entity);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_vertexLighting_DBS_world(int stage)
{
	vec3_t        viewOrigin;
	uint32_t      stateBits;
	colorGen_t    colorGen;
	alphaGen_t    alphaGen;
	shaderStage_t *pStage       = tess.surfaceStages[stage];
	qboolean      normalMapping = qfalse;

	Ren_LogComment("--- Render_vertexLighting_DBS_world ---\n");

	stateBits = pStage->stateBits;

	if (r_normalMapping->integer && (pStage->bundle[TB_NORMALMAP].image[0] != NULL))
	{
		normalMapping = qtrue;
	}

	SetMacrosAndSelectProgram(trProg.gl_vertexLightingShader_DBS_world,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, (pStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, normalMapping,
	                          USE_PARALLAX_MAPPING, normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax);
	// now we are ready to set the shader program uniforms

	// set uniforms
	VectorCopy(backEnd.orientation.viewOrigin, viewOrigin);

	GL_CheckErrors();

	// u_DeformGen
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	// u_ColorModulate
	switch (pStage->rgbGen)
	{
	case CGEN_VERTEX:
	case CGEN_ONE_MINUS_VERTEX:
		colorGen = pStage->rgbGen;
		break;
	default:
		colorGen = CGEN_CONST;
		break;
	}

	switch (pStage->alphaGen)
	{
	case AGEN_VERTEX:
		alphaGen = pStage->alphaGen;
		break;
	case AGEN_ONE_MINUS_VERTEX:
		alphaGen = pStage->alphaGen;

		/*
		alphaGen = AGEN_VERTEX;
		stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
		stateBits |= (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		*/
		break;
	default:
		alphaGen = AGEN_CONST;
		break;
	}

	GL_State(stateBits);

	GLSL_SetUniform_ColorModulate(trProg.gl_vertexLightingShader_DBS_world, colorGen, alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);
	SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&pStage->wrapAroundLightingExp, 0));
	SetUniformVec3(UNIFORM_VIEWORIGIN, viewOrigin);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	GLSL_SetUniform_AlphaTest(pStage->stateBits);

	if (r_parallaxMapping->integer)
	{
		float depthScale;

		depthScale = RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value);
		SetUniformFloat(UNIFORM_DEPTHSCALE, depthScale);
	}

	if (backEnd.viewParms.isPortal)
	{
		float plane[4];

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;
		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);

	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	if (normalMapping)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		if (pStage->bundle[TB_NORMALMAP].image[0])
		{
			GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.flatImage);
		}

		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		// bind u_SpecularMap
		SelectTexture(TEX_SPECULAR);
		if (pStage->bundle[TB_SPECULARMAP].image[0])
		{
			GL_Bind(pStage->bundle[TB_SPECULARMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.blackImage);
		}

		SetUniformMatrix16(UNIFORM_SPECULARTEXTUREMATRIX, tess.svars.texMatrices[TB_SPECULARMAP]);
	}

	GLSL_SetRequiredVertexPointers(trProg.gl_vertexLightingShader_DBS_world);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_lightMapping(int stage, qboolean asColorMap, qboolean normalMapping)
{
	shaderStage_t *pStage;
	uint32_t      stateBits;
	colorGen_t    rgbGen;
	alphaGen_t    alphaGen;

	Ren_LogComment("--- Render_lightMapping ---\n");

	pStage = tess.surfaceStages[stage];

	stateBits = pStage->stateBits;

	switch (pStage->rgbGen)
	{
	case CGEN_VERTEX:
	case CGEN_ONE_MINUS_VERTEX:
		rgbGen = pStage->rgbGen;
		break;
	default:
		rgbGen = CGEN_CONST;
		break;
	}

	switch (pStage->alphaGen)
	{
	case AGEN_VERTEX:
	case AGEN_ONE_MINUS_VERTEX:
		alphaGen = pStage->alphaGen;
		break;
	default:
		alphaGen = AGEN_CONST;
		break;
	}

	if (r_showLightMaps->integer)
	{
		stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_ATEST_BITS);
	}

	GL_State(stateBits);

	if (pStage->bundle[TB_NORMALMAP].image[0] == NULL)
	{
		normalMapping = qfalse;
	}

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_lightMappingShader,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, pStage->stateBits & GLS_ATEST_BITS,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, normalMapping,
	                          USE_PARALLAX_MAPPING, normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax);

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	GLSL_SetUniform_AlphaTest(pStage->stateBits);
	GLSL_SetUniform_ColorModulate(trProg.gl_lightMappingShader, rgbGen, alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	if (r_parallaxMapping->integer)
	{
		float depthScale;

		depthScale = RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value);
		SetUniformFloat(UNIFORM_DEPTHSCALE, depthScale);
	}

	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	if (asColorMap)
	{
		GL_Bind(tr.whiteImage);
	}
	else
	{
		GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);

		SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);
	}

	SetUniformBoolean(UNIFORM_B_SHOW_LIGHTMAP, (r_showLightMaps->integer == 1 ? GL_TRUE : GL_FALSE));
	SetUniformBoolean(UNIFORM_B_SHOW_DELUXEMAP, (r_showDeluxeMaps->integer == 1 ? GL_TRUE : GL_FALSE));

	if (normalMapping)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		if (pStage->bundle[TB_NORMALMAP].image[0])
		{
			GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.flatImage);
		}

		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		// bind u_SpecularMap
		SelectTexture(TEX_SPECULAR);
		if (pStage->bundle[TB_SPECULARMAP].image[0])
		{
			GL_Bind(pStage->bundle[TB_SPECULARMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.blackImage);
		}

		SetUniformMatrix16(UNIFORM_SPECULARTEXTUREMATRIX, tess.svars.texMatrices[TB_SPECULARMAP]);

		// bind u_DeluxeMap
		SelectTexture(TEX_DELUXE);
		BindDeluxeMap();
	}
	else if (r_showDeluxeMaps->integer == 1)
	{
		SelectTexture(TEX_DELUXE);
		BindDeluxeMap();
	}

	// bind u_LightMap
	SelectTexture(TEX_LIGHTMAP);
	BindLightMap();

	GLSL_SetRequiredVertexPointers(trProg.gl_lightMappingShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_depthFill(int stage)
{
	shaderStage_t *pStage;
	vec4_t        ambientColor;
	uint32_t      stateBits;

	Ren_LogComment("--- Render_depthFill ---\n");

	pStage     = tess.surfaceStages[stage];
	stateBits  = pStage->stateBits;
	stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_ATEST_BITS);
	stateBits |= GLS_DEPTHMASK_TRUE;

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_genericShader,
	                          USE_ALPHA_TESTING, (pStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_TCGEN_ENVIRONMENT, pStage->tcGen_Environment);

	// set uniforms
	if (pStage->tcGen_Environment)
	{
		// calculate the environment texcoords in object space
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.orientation.viewOrigin);
	}

	GLSL_SetUniform_AlphaTest(pStage->stateBits);
	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CONST, AGEN_CONST);

	// u_Color
	if (r_precomputedLighting->integer)
	{
		VectorCopy(backEnd.currentEntity->ambientLight, ambientColor);
		ClampColor(ambientColor);
	}
	else if (r_forceAmbient->integer)
	{
		ambientColor[0] = r_forceAmbient->value;
		ambientColor[1] = r_forceAmbient->value;
		ambientColor[2] = r_forceAmbient->value;
	}
	else
	{
		VectorClear(ambientColor);
	}
	ambientColor[3] = 1;

	SetUniformVec4(UNIFORM_COLOR, ambientColor);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// u_DeformGen
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	if (tess.surfaceShader->alphaTest)
	{
		GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);
	}
	else
	{
		//GL_Bind(tr.defaultImage);
		GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);
	}

	GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_shadowFill(int stage)
{
	shaderStage_t *pStage;
	uint32_t      stateBits;

	Ren_LogComment("--- Render_shadowFill ---\n");

	pStage = tess.surfaceStages[stage];

	// remove blend modes
	stateBits  = pStage->stateBits;
	stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);

	GL_State(stateBits);

	SetMacrosAndSelectProgram(trProg.gl_shadowFillShader,
	                          USE_ALPHA_TESTING, (pStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          LIGHT_DIRECTIONAL, backEnd.currentLight->l.rlType == RL_DIRECTIONAL);

	GLSL_SetRequiredVertexPointers(trProg.gl_shadowFillShader);

	if (r_debugShadowMaps->integer)
	{
		vec4_t shadowMapColor;

		Vector4Copy(g_color_table[backEnd.pc.c_batches % 8], shadowMapColor);
		SetUniformVec4(UNIFORM_COLOR, shadowMapColor);
	}

	GLSL_SetUniform_AlphaTest(pStage->stateBits);

	if (backEnd.currentLight->l.rlType != RL_DIRECTIONAL)
	{
		SetUniformVec3(UNIFORM_LIGHTORIGIN, backEnd.currentLight->origin);
		SetUniformFloat(UNIFORM_LIGHTRADIUS, backEnd.currentLight->sphereRadius);
	}

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// u_BoneMatrix
	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// u_VertexInterpolation
	if (glState.vertexAttribsInterpolation > 0)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	// u_DeformGen
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);

	if ((pStage->stateBits & GLS_ATEST_BITS) != 0)
	{
		GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrices[TB_COLORMAP]);
	}
	else
	{
		GL_Bind(tr.whiteImage);
	}

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_forwardLighting_DBS_omni(shaderStage_t *diffuseStage,
                                            shaderStage_t *attenuationXYStage,
                                            shaderStage_t *attenuationZStage, trRefLight_t *light)
{
	vec3_t     viewOrigin;
	vec3_t     lightOrigin;
	vec4_t     lightColor;
	float      shadowTexelSize;
	colorGen_t colorGen;
	alphaGen_t alphaGen;
	qboolean   normalMapping;
	qboolean   shadowCompare;

	Ren_LogComment("--- Render_forwardLighting_DBS_omni ---\n");

	if (r_normalMapping->integer && (diffuseStage->bundle[TB_NORMALMAP].image[0] != NULL))
	{
		normalMapping = qtrue;
	}
	else
	{
		normalMapping = qfalse;
	}

	if (r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0)
	{
		shadowCompare = qtrue;
	}
	else
	{
		shadowCompare = qfalse;
	}

	SetMacrosAndSelectProgram(trProg.gl_forwardLightingShader_omniXYZ,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, (diffuseStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, normalMapping,
	                          USE_PARALLAX_MAPPING, normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax,
	                          USE_SHADOWING, shadowCompare);
	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate
	switch (diffuseStage->rgbGen)
	{
	case CGEN_VERTEX:
	case CGEN_ONE_MINUS_VERTEX:
		colorGen = diffuseStage->rgbGen;
		break;
	default:
		colorGen = CGEN_CONST;
		break;
	}

	switch (diffuseStage->alphaGen)
	{
	case AGEN_VERTEX:
	case AGEN_ONE_MINUS_VERTEX:
		alphaGen = diffuseStage->alphaGen;
		break;
	default:
		alphaGen = AGEN_CONST;
		break;
	}

	GLSL_SetUniform_ColorModulate(trProg.gl_forwardLightingShader_omniXYZ, colorGen, alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	if (r_parallaxMapping->integer)
	{
		float depthScale;

		depthScale = RB_EvalExpression(&diffuseStage->depthScaleExp, r_parallaxDepthScale->value);

		SetUniformFloat(UNIFORM_DEPTHSCALE, depthScale);
	}

	// set uniforms
	VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);
	VectorCopy(light->origin, lightOrigin);
	VectorCopy(tess.svars.color, lightColor);

	if (shadowCompare)
	{
		shadowTexelSize = 1.0f / shadowMapResolutions[light->shadowLOD];
	}
	else
	{
		shadowTexelSize = 1.0f;
	}

	SetUniformVec3(UNIFORM_VIEWORIGIN, viewOrigin);
	SetUniformVec3(UNIFORM_LIGHTORIGIN, lightOrigin);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, lightColor);
	SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
	SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);
	SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&diffuseStage->wrapAroundLightingExp, 0));
	SetUniformMatrix16(UNIFORM_LIGHTATTENUATIONMATRIX, light->attenuationMatrix2);

	GL_CheckErrors();

	if (shadowCompare)
	{
		SetUniformFloat(UNIFORM_SHADOWTEXELSIZE, shadowTexelSize);
		SetUniformFloat(UNIFORM_SHADOWBLUR, r_shadowBlur->value);
	}

	GL_CheckErrors();

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// u_BoneMatrix
	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// u_VertexInterpolation
	if (glState.vertexAttribsInterpolation > 0)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	GL_CheckErrors();

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);

	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	if (normalMapping)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		if (diffuseStage->bundle[TB_NORMALMAP].image[0])
		{
			GL_Bind(diffuseStage->bundle[TB_NORMALMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.flatImage);
		}

		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		// bind u_SpecularMap
		SelectTexture(TEX_SPECULAR);
		if (r_forceSpecular->integer)
		{
			GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
		}
		else if (diffuseStage->bundle[TB_SPECULARMAP].image[0])
		{
			GL_Bind(diffuseStage->bundle[TB_SPECULARMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.blackImage);
		}

		SetUniformMatrix16(UNIFORM_SPECULARTEXTUREMATRIX, tess.svars.texMatrices[TB_SPECULARMAP]);
	}

	// bind u_AttenuationMapXY
	SelectTexture(TEX_ATTEXY);
	BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

	// bind u_AttenuationMapZ
	SelectTexture(TEX_ATTEZ);
	BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

	// bind u_ShadowMap
	if (shadowCompare)
	{
		SelectTexture(TEX_SHADOWMAP);
		GL_Bind(tr.shadowCubeFBOImage[light->shadowLOD]);
	}

	// bind u_RandomMap
	SelectTexture(TEX_RANDOM);
	GL_Bind(tr.randomNormalsImage);

	GLSL_SetRequiredVertexPointers(trProg.gl_forwardLightingShader_omniXYZ);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_forwardLighting_DBS_proj(shaderStage_t *diffuseStage,
                                            shaderStage_t *attenuationXYStage,
                                            shaderStage_t *attenuationZStage, trRefLight_t *light)
{
	vec3_t     viewOrigin;
	vec3_t     lightOrigin;
	vec4_t     lightColor;
	float      shadowTexelSize;
	colorGen_t colorGen;
	alphaGen_t alphaGen;
	qboolean   normalMapping;
	qboolean   shadowCompare;

	Ren_LogComment("--- Render_fowardLighting_DBS_proj ---\n");

	if (r_normalMapping->integer && (diffuseStage->bundle[TB_NORMALMAP].image[0] != NULL))
	{
		normalMapping = qtrue;
	}
	else
	{
		normalMapping = qfalse;
	}

	if (r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0)
	{
		shadowCompare = qtrue;
	}
	else
	{
		shadowCompare = qfalse;
	}

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_forwardLightingShader_projXYZ,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, (diffuseStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, normalMapping,
	                          USE_PARALLAX_MAPPING, normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax,
	                          USE_SHADOWING, shadowCompare);
	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate
	switch (diffuseStage->rgbGen)
	{
	case CGEN_VERTEX:
	case CGEN_ONE_MINUS_VERTEX:
		colorGen = diffuseStage->rgbGen;
		break;
	default:
		colorGen = CGEN_CONST;
		break;
	}

	switch (diffuseStage->alphaGen)
	{
	case AGEN_VERTEX:
	case AGEN_ONE_MINUS_VERTEX:
		alphaGen = diffuseStage->alphaGen;
		break;
	default:
		alphaGen = AGEN_CONST;
		break;
	}

	GLSL_SetUniform_ColorModulate(trProg.gl_forwardLightingShader_projXYZ, colorGen, alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	if (r_parallaxMapping->integer)
	{
		float depthScale;

		depthScale = RB_EvalExpression(&diffuseStage->depthScaleExp, r_parallaxDepthScale->value);

		SetUniformFloat(UNIFORM_DEPTHSCALE, depthScale);
	}

	// set uniforms
	VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);
	VectorCopy(light->origin, lightOrigin);
	VectorCopy(tess.svars.color, lightColor);

	if (shadowCompare)
	{
		shadowTexelSize = 1.0f / shadowMapResolutions[light->shadowLOD];
	}
	else
	{
		shadowTexelSize = 1.0f;
	}

	SetUniformVec3(UNIFORM_VIEWORIGIN, viewOrigin);
	SetUniformVec3(UNIFORM_LIGHTORIGIN, lightOrigin);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, lightColor);
	SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
	SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);
	SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&diffuseStage->wrapAroundLightingExp, 0));
	SetUniformMatrix16(UNIFORM_LIGHTATTENUATIONMATRIX, light->attenuationMatrix2);

	GL_CheckErrors();

	if (shadowCompare)
	{
		SetUniformFloat(UNIFORM_SHADOWTEXELSIZE, shadowTexelSize);
		SetUniformFloat(UNIFORM_SHADOWBLUR, r_shadowBlur->value);
		SetUniformMatrix16ARR(UNIFORM_SHADOWMATRIX, light->shadowMatrices, MAX_SHADOWMAPS);
	}

	GL_CheckErrors();

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// u_BoneMatrix
	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// u_VertexInterpolation
	if (glState.vertexAttribsInterpolation > 0)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	GL_CheckErrors();

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);

	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	if (normalMapping)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		if (diffuseStage->bundle[TB_NORMALMAP].image[0])
		{
			GL_Bind(diffuseStage->bundle[TB_NORMALMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.flatImage);
		}

		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		// bind u_SpecularMap
		SelectTexture(TEX_SPECULAR);
		if (r_forceSpecular->integer)
		{
			GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
		}
		else if (diffuseStage->bundle[TB_SPECULARMAP].image[0])
		{
			GL_Bind(diffuseStage->bundle[TB_SPECULARMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.blackImage);
		}

		SetUniformMatrix16(UNIFORM_SPECULARTEXTUREMATRIX, tess.svars.texMatrices[TB_SPECULARMAP]);
	}

	// bind u_AttenuationMapXY
	SelectTexture(TEX_ATTEXY);
	BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

	// bind u_AttenuationMapZ
	SelectTexture(TEX_ATTEZ);
	BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

	// bind u_ShadowMap
	if (shadowCompare)
	{
		SelectTexture(TEX_SHADOWMAP);
		GL_Bind(tr.shadowMapFBOImage[light->shadowLOD]);
	}

	// bind u_RandomMap
	SelectTexture(TEX_RANDOM);
	GL_Bind(tr.randomNormalsImage);

	GLSL_SetRequiredVertexPointers(trProg.gl_forwardLightingShader_projXYZ);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_forwardLighting_DBS_directional(shaderStage_t *diffuseStage,
                                                   shaderStage_t *attenuationXYStage,
                                                   shaderStage_t *attenuationZStage, trRefLight_t *light)
{
#if 1
	vec3_t     viewOrigin;
	vec3_t     lightDirection;
	vec4_t     lightColor;
	float      shadowTexelSize;
	colorGen_t colorGen;
	alphaGen_t alphaGen;
	qboolean   normalMapping = qfalse;
	qboolean   shadowCompare = qfalse;

	Ren_LogComment("--- Render_forwardLighting_DBS_directional ---\n");

	if (r_normalMapping->integer && (diffuseStage->bundle[TB_NORMALMAP].image[0] != NULL))
	{
		normalMapping = qtrue;
	}

	if (r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0)
	{
		shadowCompare = qtrue;
	}

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_forwardLightingShader_directionalSun,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, (diffuseStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, normalMapping,
	                          USE_PARALLAX_MAPPING, normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax,
	                          USE_SHADOWING, shadowCompare);

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate
	switch (diffuseStage->rgbGen)
	{
	case CGEN_VERTEX:
	case CGEN_ONE_MINUS_VERTEX:
		colorGen = diffuseStage->rgbGen;
		break;
	default:
		colorGen = CGEN_CONST;
		break;
	}

	switch (diffuseStage->alphaGen)
	{
	case AGEN_VERTEX:
	case AGEN_ONE_MINUS_VERTEX:
		alphaGen = diffuseStage->alphaGen;
		break;
	default:
		alphaGen = AGEN_CONST;
		break;
	}

	GLSL_SetUniform_ColorModulate(trProg.gl_forwardLightingShader_directionalSun, colorGen, alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	if (r_parallaxMapping->integer)
	{
		float depthScale;

		depthScale = RB_EvalExpression(&diffuseStage->depthScaleExp, r_parallaxDepthScale->value);
		SetUniformFloat(UNIFORM_DEPTHSCALE, depthScale);
	}

	// set uniforms
	VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);

#if 1
	VectorCopy(tr.sunDirection, lightDirection);
#else
	VectorCopy(light->direction, lightDirection);
#endif

	VectorCopy(tess.svars.color, lightColor);

	if (shadowCompare)
	{
		shadowTexelSize = 1.0f / sunShadowMapResolutions[light->shadowLOD];
	}
	else
	{
		shadowTexelSize = 1.0f;
	}

	SetUniformVec3(UNIFORM_VIEWORIGIN, viewOrigin);
	SetUniformVec3(UNIFORM_LIGHTDIR, lightDirection);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, lightColor);
	SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
	SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);
	SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&diffuseStage->wrapAroundLightingExp, 0));
	SetUniformMatrix16(UNIFORM_LIGHTATTENUATIONMATRIX, light->attenuationMatrix2);

	GL_CheckErrors();

	if (shadowCompare)
	{
		SetUniformMatrix16ARR(UNIFORM_SHADOWMATRIX, light->shadowMatricesBiased, MAX_SHADOWMAPS);
		SetUniformVec4(UNIFORM_SHADOWPARALLELSPLITDISTANCES, backEnd.viewParms.parallelSplitDistances);
		SetUniformFloat(UNIFORM_SHADOWTEXELSIZE, shadowTexelSize);
		SetUniformFloat(UNIFORM_SHADOWBLUR, r_shadowBlur->value);
	}

	GL_CheckErrors();

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_VIEWMATRIX, backEnd.viewParms.world.viewMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// u_BoneMatrix
	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// u_VertexInterpolation
	if (glState.vertexAttribsInterpolation > 0)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	GL_CheckErrors();

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);

	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	if (normalMapping)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		if (diffuseStage->bundle[TB_NORMALMAP].image[0])
		{
			GL_Bind(diffuseStage->bundle[TB_NORMALMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.flatImage);
		}

		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		// bind u_SpecularMap
		SelectTexture(TEX_SPECULAR);
		if (r_forceSpecular->integer)
		{
			GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
		}
		else if (diffuseStage->bundle[TB_SPECULARMAP].image[0])
		{
			GL_Bind(diffuseStage->bundle[TB_SPECULARMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.blackImage);
		}

		SetUniformMatrix16(UNIFORM_SPECULARTEXTUREMATRIX, tess.svars.texMatrices[TB_SPECULARMAP]);
	}

	// bind u_ShadowMap
	if (shadowCompare)
	{
		SelectTexture(TEX_SHADOWMAP0);
		GL_Bind(tr.sunShadowMapFBOImage[0]);

		if (r_parallelShadowSplits->integer >= 1)
		{
			SelectTexture(TEX_SHADOWMAP1);
			GL_Bind(tr.sunShadowMapFBOImage[1]);
		}

		if (r_parallelShadowSplits->integer >= 2)
		{
			SelectTexture(TEX_SHADOWMAP2);
			GL_Bind(tr.sunShadowMapFBOImage[2]);
		}

		if (r_parallelShadowSplits->integer >= 3)
		{
			SelectTexture(TEX_SHADOWMAP3);
			GL_Bind(tr.sunShadowMapFBOImage[3]);
		}

		if (r_parallelShadowSplits->integer >= 4)
		{
			SelectTexture(TEX_SHADOWMAP4);
			GL_Bind(tr.sunShadowMapFBOImage[4]);
		}
	}

	GLSL_SetRequiredVertexPointers(trProg.gl_forwardLightingShader_directionalSun);

	Tess_DrawElements();

	GL_CheckErrors();
#endif
}

static void Render_reflection_CB(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];
	qboolean      normalMapping;

	Ren_LogComment("--- Render_reflection_CB ---\n");

	GL_State(pStage->stateBits);

	if (r_normalMapping->integer && (pStage->bundle[TB_NORMALMAP].image[0] != NULL))
	{
		normalMapping = qtrue;
	}
	else
	{
		normalMapping = qfalse;
	}

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_reflectionShader,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, normalMapping);

	SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	if (glState.vertexAttribsInterpolation > 0)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
#if 1
	if (backEnd.currentEntity && (backEnd.currentEntity != &tr.worldEntity))
	{
		GL_BindNearestCubeMap(backEnd.currentEntity->e.origin);
	}
	else
	{
		GL_BindNearestCubeMap(backEnd.viewParms.orientation.origin);
	}
#else
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
#endif

	// bind u_NormalMap
	if (normalMapping)
	{
		SelectTexture(TEX_NORMAL);
		GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);
	}

	GLSL_SetRequiredVertexPointers(trProg.gl_reflectionShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_refraction_C(int stage)
{
	vec3_t        viewOrigin;
	shaderStage_t *pStage = tess.surfaceStages[stage];

	Ren_LogComment("--- Render_refraction_C ---\n");

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_refractionShader, USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning);

	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_NORMAL);

	// set uniforms
	VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin); // in world space

	SetUniformVec3(UNIFORM_VIEWORIGIN, viewOrigin);
	SetUniformFloat(UNIFORM_REFRACTIONINDEX, RB_EvalExpression(&pStage->refractionIndexExp, 1.0));
	SetUniformFloat(UNIFORM_FRESNELPOWER, RB_EvalExpression(&pStage->fresnelPowerExp, 2.0));
	SetUniformFloat(UNIFORM_FRESNELSCALE, RB_EvalExpression(&pStage->fresnelScaleExp, 2.0));
	SetUniformFloat(UNIFORM_FRESNELBIAS, RB_EvalExpression(&pStage->fresnelBiasExp, 1.0));

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_dispersion_C(int stage)
{
	vec3_t        viewOrigin;
	shaderStage_t *pStage = tess.surfaceStages[stage];
	float         eta;
	float         etaDelta;

	Ren_LogComment("--- Render_dispersion_C ---\n");

	GL_State(pStage->stateBits);

	// enable shader, set arrays
	SetMacrosAndSelectProgram(trProg.gl_dispersionShader, USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning);

	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_NORMAL);

	// set uniforms
	VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);   // in world space
	eta      = RB_EvalExpression(&pStage->etaExp, (float)1.1);
	etaDelta = RB_EvalExpression(&pStage->etaDeltaExp, (float)-0.02);

	SetUniformVec3(UNIFORM_VIEWORIGIN, viewOrigin);
	{
		vec3_t temp = { eta, eta + etaDelta, eta + (etaDelta * 2) };
		SetUniformVec3(UNIFORM_ETARATIO, temp);
	}
	SetUniformFloat(UNIFORM_FRESNELPOWER, RB_EvalExpression(&pStage->fresnelPowerExp, 2.0f));
	SetUniformFloat(UNIFORM_FRESNELSCALE, RB_EvalExpression(&pStage->fresnelScaleExp, 2.0f));
	SetUniformFloat(UNIFORM_FRESNELBIAS, RB_EvalExpression(&pStage->fresnelBiasExp, 1.0f));

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_skybox(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];

	Ren_LogComment("--- Render_skybox ---\n");

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_skyboxShader, USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal);

	SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);   // in world space
	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// u_PortalPlane
	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);

	GLSL_SetRequiredVertexPointers(trProg.gl_skyboxShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_screen(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];

	Ren_LogComment("--- Render_screen ---\n");

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_screenShader);

	/*
	if(pStage->vertexColor || pStage->inverseVertexColor)
	{
	    GL_VertexAttribsState(tr.screenShader.attribs);
	}
	else
	*/
	{
		GLSL_VertexAttribsState(ATTR_POSITION);
		glVertexAttrib4fv(ATTR_INDEX_COLOR, tess.svars.color);
	}

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// bind u_CurrentMap
	SelectTexture(TEX_CURRENT);
	BindAnimatedImage(&pStage->bundle[TB_COLORMAP]);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_portal(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];

	Ren_LogComment("--- Render_portal ---\n");

	GL_State(pStage->stateBits);

	// enable shader, set arrays
	SetMacrosAndSelectProgram(trProg.gl_portalShader);

	/*
	if(pStage->vertexColor || pStage->inverseVertexColor)
	{
	    GL_VertexAttribsState(tr.portalShader.attribs);
	}
	else
	*/
	{
		GLSL_VertexAttribsState(ATTR_POSITION);
		glVertexAttrib4fv(ATTR_INDEX_COLOR, tess.svars.color);
	}

	SetUniformFloat(UNIFORM_PORTALRANGE, tess.surfaceShader->portalRange);
	SetUniformMatrix16(UNIFORM_MODELVIEWMATRIX, GLSTACK_MVM);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// bind u_CurrentMap
	SelectTexture(TEX_CURRENT);
	BindAnimatedImage(&pStage->bundle[TB_COLORMAP]);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_heatHaze(int stage)
{
	uint32_t      stateBits;
	float         deformMagnitude;
	shaderStage_t *pStage = tess.surfaceStages[stage];

	Ren_LogComment("--- Render_heatHaze ---\n");

	if (r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable )
	{
		FBO_t    *previousFBO;
		uint32_t stateBits;

		Ren_LogComment("--- HEATHAZE FIX BEGIN ---\n");

		// capture current color buffer for u_CurrentMap
		/*
		SelectTexture(TEX_CURRENT);
		GL_Bind(tr.currentRenderImage);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth,
		                     tr.currentRenderImage->uploadHeight);

		*/

		previousFBO = glState.currentFBO;

		if (HDR_ENABLED())
		{
			GL_CheckErrors();

			// copy deferredRenderFBO to occlusionRenderFBO
#if 0
			R_CopyToFBO(tr.deferredRenderFBO, tr.occlusionRenderFBO, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
#else
			R_CopyToFBO(tr.deferredRenderFBO, tr.occlusionRenderFBO, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
#endif

			GL_CheckErrors();
		}
		else
		{
			// copy depth of the main context to occlusionRenderFBO
			R_CopyToFBO(NULL, tr.occlusionRenderFBO, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}

		R_BindFBO(tr.occlusionRenderFBO);
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.occlusionRenderFBOImage->texnum, 0);

		// clear color buffer
		GL_Clear(GL_COLOR_BUFFER_BIT);

		// remove blend mode
		stateBits  = pStage->stateBits;
		stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_DEPTHMASK_TRUE);

		GL_State(stateBits);

		// choose right shader program ----------------------------------
		//gl_genericShader->SetAlphaTesting((pStage->stateBits & GLS_ATEST_BITS) != 0);
		SetMacrosAndSelectProgram(trProg.gl_genericShader,
		                          USE_ALPHA_TESTING, qfalse,
		                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
		                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
		                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
		                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
		                          USE_TCGEN_ENVIRONMENT, qfalse);
		// end choose right shader program ------------------------------

		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CONST, AGEN_CONST);
		SetUniformVec4(UNIFORM_COLOR, colorRed);
		SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
		{
			SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
		}

		if (glState.vertexAttribsInterpolation > 0)
		{
			SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
		}

		if (tess.surfaceShader->numDeforms)
		{
			GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
			SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
		}

		if (backEnd.viewParms.isPortal)
		{
			vec4_t plane;

			// clipping plane in world space
			plane[0] = backEnd.viewParms.portalPlane.normal[0];
			plane[1] = backEnd.viewParms.portalPlane.normal[1];
			plane[2] = backEnd.viewParms.portalPlane.normal[2];
			plane[3] = backEnd.viewParms.portalPlane.dist;

			SetUniformVec4(UNIFORM_PORTALPLANE, plane);
		}

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);
		//gl_genericShader->SetUniform_ColorTextureMatrix(tess.svars.texMatrices[TB_COLORMAP]);

		GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

		Tess_DrawElements();

		R_BindFBO(previousFBO);

		GL_CheckErrors();

		Ren_LogComment("--- HEATHAZE FIX END ---\n");
	}

	// remove alpha test
	stateBits  = pStage->stateBits;
	stateBits &= ~GLS_ATEST_BITS;
	stateBits &= ~GLS_DEPTHMASK_TRUE;

	GL_State(stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_heatHazeShader,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms);
	// end choose right shader program ------------------------------

	// set uniforms
	//GLSL_SetUniform_AlphaTest(&tr.heatHazeShader, pStage->stateBits);

	deformMagnitude = RB_EvalExpression(&pStage->deformMagnitudeExp, 1.0);

	SetUniformFloat(UNIFORM_DEFORMMAGNITUDE, deformMagnitude);
	SetUniformMatrix16(UNIFORM_MODELVIEWMATRIXTRANSPOSE, GLSTACK_MVM);
	SetUniformMatrix16(UNIFORM_PROJECTIONMATRIXTRANSPOSE, GLSTACK_PM);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	if (glState.vertexAttribsInterpolation > 0)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	// bind u_NormalMap
	SelectTexture(TEX_NORMAL);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);

	SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_COLORMAP]);

	// bind u_CurrentMap
	SelectTexture(TEX_CURRENT);
	if (HDR_ENABLED())
	{
		GL_Bind(tr.deferredRenderFBOImage);
	}
	else
	{
		ImageCopyBackBuffer(tr.currentRenderImage);
	}

	// bind u_ContrastMap
	if (r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable)
	{
		SelectTexture(TEX_CONTRAST);
		GL_Bind(tr.occlusionRenderFBOImage);
	}

	GLSL_SetRequiredVertexPointers(trProg.gl_heatHazeShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_liquid(int stage)
{
	vec3_t        viewOrigin;
	float         fogDensity;
	shaderStage_t *pStage = tess.surfaceStages[stage];

	Ren_LogComment("--- Render_liquid ---\n");

	GL_State(pStage->stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_liquidShader, USE_PARALLAX_MAPPING, r_parallaxMapping->integer && tess.surfaceShader->parallax);

	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL | ATTR_COLOR);

	// set uniforms
	VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);   // in world space

	fogDensity = RB_EvalExpression(&pStage->fogDensityExp, 0.001);

	SetUniformVec3(UNIFORM_VIEWORIGIN, viewOrigin);
	SetUniformFloat(UNIFORM_REFRACTIONINDEX, RB_EvalExpression(&pStage->refractionIndexExp, 1.0));
	SetUniformFloat(UNIFORM_FRESNELPOWER, RB_EvalExpression(&pStage->fresnelPowerExp, 2.0));
	SetUniformFloat(UNIFORM_FRESNELSCALE, RB_EvalExpression(&pStage->fresnelScaleExp, 1.0));
	SetUniformFloat(UNIFORM_FRESNELBIAS, RB_EvalExpression(&pStage->fresnelBiasExp, 0.05));
	SetUniformFloat(UNIFORM_NORMALSCALE, RB_EvalExpression(&pStage->normalScaleExp, 0.05));
	SetUniformFloat(UNIFORM_FOGDENSITY, fogDensity);
	SetUniformVec3(UNIFORM_FOGCOLOR, tess.svars.color);
	SetUniformMatrix16(UNIFORM_UNPROJECTMATRIX, backEnd.viewParms.unprojectionMatrix);
	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// capture current color buffer for u_CurrentMap
	SelectTexture(TEX_CURRENT);
	if (HDR_ENABLED())
	{
		GL_Bind(tr.deferredRenderFBOImage);
	}
	else
	{
		ImageCopyBackBuffer(tr.currentRenderImage);
	}

	// bind u_PortalMap
	SelectTexture(TEX_PORTAL);
	GL_Bind(tr.portalRenderImage);

	// bind u_DepthMap
	SelectTexture(TEX_DEPTH);
	if (HDR_ENABLED())
	{
		GL_Bind(tr.depthRenderImage);
	}
	else
	{
		// depth texture is not bound to a FBO
		ImageCopyBackBuffer(tr.depthRenderImage);
	}

	// bind u_NormalMap
	SelectTexture(TEX_NORMAL);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);

	SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_COLORMAP]);

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_fog()
{
	fog_t  *fog;
	float  eyeT;
	vec3_t local;
	vec4_t fogDistanceVector, fogDepthVector;

	Ren_LogComment("--- Render_fog ---\n");

	// no fog pass in snooper
	if ((tr.refdef.rdflags & RDF_SNOOPERVIEW) || tess.surfaceShader->noFog || !r_wolfFog->integer)
	{
		return;
	}

	// no world, no fogging
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	fog = tr.world->fogs + tess.fogNum;

#if 1
	// use this only to render fog brushes

	if (fog->originalBrushNumber < 0 && tess.surfaceShader->sort <= SS_OPAQUE)
	{
		return;
	}
#endif

	Ren_LogComment("--- Render_fog( fogNum = %i, originalBrushNumber = %i ) ---\n", tess.fogNum, fog->originalBrushNumber);

	// all fogging distance is based on world Z units
	VectorSubtract(backEnd.orientation.origin, backEnd.viewParms.orientation.origin, local);
	fogDistanceVector[0] = -backEnd.orientation.modelViewMatrix[2];
	fogDistanceVector[1] = -backEnd.orientation.modelViewMatrix[6];
	fogDistanceVector[2] = -backEnd.orientation.modelViewMatrix[10];
	fogDistanceVector[3] = DotProduct(local, backEnd.viewParms.orientation.axis[0]);

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[0] *= fog->tcScale;
	fogDistanceVector[1] *= fog->tcScale;
	fogDistanceVector[2] *= fog->tcScale;
	fogDistanceVector[3] *= fog->tcScale;

	// rotate the gradient vector for this orientation
	if (fog->hasSurface)
	{
		fogDepthVector[0] = fog->surface[0] * backEnd.orientation.axis[0][0] +
		                    fog->surface[1] * backEnd.orientation.axis[0][1] + fog->surface[2] * backEnd.orientation.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.orientation.axis[1][0] +
		                    fog->surface[1] * backEnd.orientation.axis[1][1] + fog->surface[2] * backEnd.orientation.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.orientation.axis[2][0] +
		                    fog->surface[1] * backEnd.orientation.axis[2][1] + fog->surface[2] * backEnd.orientation.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct(backEnd.orientation.origin, fog->surface);

		eyeT = DotProduct(backEnd.orientation.viewOrigin, fogDepthVector) + fogDepthVector[3];
	}
	else
	{
		Vector4Set(fogDepthVector, 0, 0, 0, 1);
		eyeT = 1;               // non-surface fog always has eye inside
	}

	fogDistanceVector[3] += 1.0 / 512;

	if (tess.surfaceShader->fogPass == FP_EQUAL)
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL);
	}
	else
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}

	SetMacrosAndSelectProgram(trProg.gl_fogQuake3Shader,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          EYE_OUTSIDE, eyeT < 0); // viewpoint is outside when eyeT < 0 - needed for clipping distance even for constant fog

	SetUniformVec4(UNIFORM_FOGDISTANCEVECTOR, fogDistanceVector);
	SetUniformVec4(UNIFORM_FOGDEPTHVECTOR, fogDepthVector);
	SetUniformFloat(UNIFORM_FOGEYET, eyeT);
	SetUniformVec4(UNIFORM_COLOR, fog->color);
	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	if (glState.vertexAttribsInterpolation > 0)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, backEnd.refdef.floatTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		vec4_t plane;

		// clipping plane in world space
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		SetUniformVec4(UNIFORM_PORTALPLANE, plane);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(tr.fogImage);
	//gl_fogQuake3Shader->SetUniform_ColorTextureMatrix(tess.svars.texMatrices[TB_COLORMAP]);

	GLSL_SetRequiredVertexPointers(trProg.gl_fogQuake3Shader);

	Tess_DrawElements();

	GL_CheckErrors();
}

// see Fog Polygon Volumes documentation by Nvidia for further information
static void Render_volumetricFog()
{
	vec3_t viewOrigin;
	vec3_t fogColor;

	Ren_LogComment("--- Render_volumetricFog---\n");

	if (glConfig2.framebufferBlitAvailable)
	{
		float fogDensity;
		FBO_t *previousFBO;

		previousFBO = glState.currentFBO;

		if (r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable)
		{
			// copy deferredRenderFBO to occlusionRenderFBO
			R_CopyToFBO(tr.deferredRenderFBO, tr.occlusionRenderFBO, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}
		else
		{
			// copy depth of the main context to occlusionRenderFBO
			R_CopyToFBO(NULL, tr.occlusionRenderFBO, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}

		SetMacrosAndSelectProgram(trProg.gl_depthToColorShader, USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning);

		GLSL_VertexAttribsState(ATTR_POSITION | ATTR_NORMAL);
		GL_State(0); //GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		// might be cool for ghost player effects
		if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
		{
			SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
		}

		// render back faces
		R_BindFBO(tr.occlusionRenderFBO);
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.depthToColorBackFacesFBOImage->texnum, 0);

		GL_ClearColor(GLCOLOR_BLACK);
		GL_Clear(GL_COLOR_BUFFER_BIT);
		GL_Cull(CT_BACK_SIDED);
		Tess_DrawElements();

		// render front faces
		R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.depthToColorFrontFacesFBOImage->texnum, 0);

		GL_Clear(GL_COLOR_BUFFER_BIT);
		GL_Cull(CT_FRONT_SIDED);
		Tess_DrawElements();

		R_BindFBO(previousFBO);

		SetMacrosAndSelectProgram(trProg.gl_volumetricFogShader);
		GLSL_VertexAttribsState(ATTR_POSITION);

		//GL_State(GLS_DEPTHTEST_DISABLE);	// | GLS_DEPTHMASK_TRUE);
		//GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);
		GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA);
		GL_Cull(CT_TWO_SIDED);

		glVertexAttrib4fv(ATTR_INDEX_COLOR, colorWhite);

		// set uniforms
		VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);   // in world space

		{
			fogDensity = tess.surfaceShader->fogParms.density;
			VectorCopy(tess.surfaceShader->fogParms.color, fogColor);
		}

		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
		SetUniformMatrix16(UNIFORM_UNPROJECTMATRIX, backEnd.viewParms.unprojectionMatrix);
		SetUniformVec3(UNIFORM_VIEWORIGIN, viewOrigin);
		SetUniformFloat(UNIFORM_FOGDENSITY, fogDensity);
		SetUniformVec3(UNIFORM_FOGCOLOR, fogColor);

		// bind u_DepthMap
		SelectTexture(TEX_DEPTH);
		if (r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable)
		{
			GL_Bind(tr.depthRenderImage);
		}
		else
		{
			// depth texture is not bound to a FBO
			ImageCopyBackBuffer(tr.depthRenderImage);
		}

		// bind u_DepthMapBack
		SelectTexture(TEX_DEPTHFRONT);
		GL_Bind(tr.depthToColorBackFacesFBOImage);

		// bind u_DepthMapFront
		SelectTexture(TEX_DEPTHBACK);
		GL_Bind(tr.depthToColorFrontFacesFBOImage);

		Tess_DrawElements();
	}

	GL_CheckErrors();
}

void Tess_ComputeColor(shaderStage_t *pStage)
{
	float rgb;
	float red;
	float green;
	float blue;
	float alpha;

	Ren_LogComment("--- Tess_ComputeColor ---\n");

	// rgbGen
	switch (pStage->rgbGen)
	{
	case CGEN_IDENTITY:
	{
		tess.svars.color[0] = 1.0;
		tess.svars.color[1] = 1.0;
		tess.svars.color[2] = 1.0;
		tess.svars.color[3] = 1.0;
		break;
	}
	case CGEN_VERTEX:
	case CGEN_ONE_MINUS_VERTEX:
	{
		tess.svars.color[0] = 0.0;
		tess.svars.color[1] = 0.0;
		tess.svars.color[2] = 0.0;
		tess.svars.color[3] = 0.0;
		break;
	}
	default:
	case CGEN_IDENTITY_LIGHTING:
	{
		tess.svars.color[0] = tr.identityLight;
		tess.svars.color[1] = tr.identityLight;
		tess.svars.color[2] = tr.identityLight;
		tess.svars.color[3] = tr.identityLight;
		break;
	}
	case CGEN_CONST:
	{
		tess.svars.color[0] = pStage->constantColor[0] * (1.0 / 255.0);
		tess.svars.color[1] = pStage->constantColor[1] * (1.0 / 255.0);
		tess.svars.color[2] = pStage->constantColor[2] * (1.0 / 255.0);
		tess.svars.color[3] = pStage->constantColor[3] * (1.0 / 255.0);
		break;
	}
	case CGEN_ENTITY:
	{
		if (backEnd.currentLight)
		{
			tess.svars.color[0] = Q_bound(0.0, backEnd.currentLight->l.color[0], 1.0);
			tess.svars.color[1] = Q_bound(0.0, backEnd.currentLight->l.color[1], 1.0);
			tess.svars.color[2] = Q_bound(0.0, backEnd.currentLight->l.color[2], 1.0);
			tess.svars.color[3] = 1.0;
		}
		else if (backEnd.currentEntity)
		{
			tess.svars.color[0] = Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[0] * (1.0 / 255.0), 1.0);
			tess.svars.color[1] = Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[1] * (1.0 / 255.0), 1.0);
			tess.svars.color[2] = Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[2] * (1.0 / 255.0), 1.0);
			tess.svars.color[3] = Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[3] * (1.0 / 255.0), 1.0);
		}
		else
		{
			tess.svars.color[0] = 1.0;
			tess.svars.color[1] = 1.0;
			tess.svars.color[2] = 1.0;
			tess.svars.color[3] = 1.0;
		}
		break;
	}
	case CGEN_ONE_MINUS_ENTITY:
	{
		if (backEnd.currentLight)
		{
			tess.svars.color[0] = 1.0 - Q_bound(0.0, backEnd.currentLight->l.color[0], 1.0);
			tess.svars.color[1] = 1.0 - Q_bound(0.0, backEnd.currentLight->l.color[1], 1.0);
			tess.svars.color[2] = 1.0 - Q_bound(0.0, backEnd.currentLight->l.color[2], 1.0);
			tess.svars.color[3] = 0.0;      // FIXME
		}
		else if (backEnd.currentEntity)
		{
			tess.svars.color[0] = 1.0 - Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[0] * (1.0 / 255.0), 1.0);
			tess.svars.color[1] = 1.0 - Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[1] * (1.0 / 255.0), 1.0);
			tess.svars.color[2] = 1.0 - Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[2] * (1.0 / 255.0), 1.0);
			tess.svars.color[3] = 1.0 - Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[3] * (1.0 / 255.0), 1.0);
		}
		else
		{
			tess.svars.color[0] = 0.0;
			tess.svars.color[1] = 0.0;
			tess.svars.color[2] = 0.0;
			tess.svars.color[3] = 0.0;
		}
		break;
	}
	case CGEN_WAVEFORM:
	{
		float      glow;
		waveForm_t *wf;

		wf = &pStage->rgbWave;

		if (wf->func == GF_NOISE)
		{
			glow = wf->base + R_NoiseGet4f(0, 0, 0, (backEnd.refdef.floatTime + wf->phase) * wf->frequency) * wf->amplitude;
		}
		else
		{
			glow = RB_EvalWaveForm(wf) * tr.identityLight;
		}

		if (glow < 0)
		{
			glow = 0;
		}
		else if (glow > 1)
		{
			glow = 1;
		}

		tess.svars.color[0] = glow;
		tess.svars.color[1] = glow;
		tess.svars.color[2] = glow;
		tess.svars.color[3] = 1.0;
		break;
	}
	case CGEN_CUSTOM_RGB:
	{
		rgb = Q_bound(0.0, RB_EvalExpression(&pStage->rgbExp, 1.0), 1.0);

		tess.svars.color[0] = rgb;
		tess.svars.color[1] = rgb;
		tess.svars.color[2] = rgb;
		break;
	}
	case CGEN_CUSTOM_RGBs:
	{
		if (backEnd.currentLight)
		{
			red   = Q_bound(0.0, RB_EvalExpression(&pStage->redExp, backEnd.currentLight->l.color[0]), 1.0);
			green = Q_bound(0.0, RB_EvalExpression(&pStage->greenExp, backEnd.currentLight->l.color[1]), 1.0);
			blue  = Q_bound(0.0, RB_EvalExpression(&pStage->blueExp, backEnd.currentLight->l.color[2]), 1.0);
		}
		else if (backEnd.currentEntity)
		{
			red =
			    Q_bound(0.0, RB_EvalExpression(&pStage->redExp, backEnd.currentEntity->e.shaderRGBA[0] * (1.0 / 255.0)), 1.0);
			green =
			    Q_bound(0.0, RB_EvalExpression(&pStage->greenExp, backEnd.currentEntity->e.shaderRGBA[1] * (1.0 / 255.0)),
			            1.0);
			blue =
			    Q_bound(0.0, RB_EvalExpression(&pStage->blueExp, backEnd.currentEntity->e.shaderRGBA[2] * (1.0 / 255.0)),
			            1.0);
		}
		else
		{
			red   = Q_bound(0.0, RB_EvalExpression(&pStage->redExp, 1.0), 1.0);
			green = Q_bound(0.0, RB_EvalExpression(&pStage->greenExp, 1.0), 1.0);
			blue  = Q_bound(0.0, RB_EvalExpression(&pStage->blueExp, 1.0), 1.0);
		}

		tess.svars.color[0] = red;
		tess.svars.color[1] = green;
		tess.svars.color[2] = blue;
		break;
	}
	}

	// alphaGen
	switch (pStage->alphaGen)
	{
	default:
	case AGEN_IDENTITY:
	{
		if (pStage->rgbGen != CGEN_IDENTITY)
		{
			if ((pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1) || pStage->rgbGen != CGEN_VERTEX)
			{
				tess.svars.color[3] = 1.0;
			}
		}
		break;
	}
	case AGEN_VERTEX:
	case AGEN_ONE_MINUS_VERTEX:
	{
		tess.svars.color[3] = 0.0;
		break;
	}
	case AGEN_CONST:
	{
		if (pStage->rgbGen != CGEN_CONST)
		{
			tess.svars.color[3] = pStage->constantColor[3] * (1.0 / 255.0);
		}
		break;
	}
	case AGEN_ENTITY:
	{
		if (backEnd.currentLight)
		{
			tess.svars.color[3] = 1.0;      // FIXME ?
		}
		else if (backEnd.currentEntity)
		{
			tess.svars.color[3] = Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[3] * (1.0 / 255.0), 1.0);
		}
		else
		{
			tess.svars.color[3] = 1.0;
		}
		break;
	}
	case AGEN_ONE_MINUS_ENTITY:
	{
		if (backEnd.currentLight)
		{
			tess.svars.color[3] = 0.0;      // FIXME ?
		}
		else if (backEnd.currentEntity)
		{
			tess.svars.color[3] = 1.0 - Q_bound(0.0, backEnd.currentEntity->e.shaderRGBA[3] * (1.0 / 255.0), 1.0);
		}
		else
		{
			tess.svars.color[3] = 0.0;
		}
		break;
	}
	case AGEN_NORMALZFADE:
	{
#if 0
		// FIX ME
		float    alpha, range, lowest, highest, dot;
		vec3_t   worldUp;
		qboolean zombieEffect = qfalse;
		int      i;

		if (VectorCompare(backEnd.currentEntity->e.fireRiseDir, vec3_origin))
		{
			VectorSet(backEnd.currentEntity->e.fireRiseDir, 0, 0, 1);
		}

		if (backEnd.currentEntity->e.hModel)      // world surfaces dont have an axis
		{
			VectorRotate(backEnd.currentEntity->e.fireRiseDir, backEnd.currentEntity->e.axis, worldUp);
		}
		else
		{
			VectorCopy(backEnd.currentEntity->e.fireRiseDir, worldUp);
		}

		lowest = pStage->zFadeBounds[0];
		if (lowest == -1000)      // use entity alpha
		{
			lowest       = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		highest = pStage->zFadeBounds[1];
		if (highest == -1000)      // use entity alpha
		{
			highest      = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		range = highest - lowest;
		for (i = 0; i < tess.numVertexes; i++)
		{
			//dot = DotProduct(tess.normal[i].v, worldUp);
			dot = DotProduct(tess.normals[i], worldUp);

			// special handling for Zombie fade effect
			if (zombieEffect)
			{
				alpha  = (float)backEnd.currentEntity->e.shaderRGBA[3] * (dot + 1.0) / 2.0;
				alpha += (2.0 * (float)backEnd.currentEntity->e.shaderRGBA[3]) * (1.0 - (dot + 1.0) / 2.0);
				if (alpha > 255.0)
				{
					alpha = 255.0;
				}
				else if (alpha < 0.0)
				{
					alpha = 0.0;
				}
				tess.svars.color[3] = (byte) (alpha);
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
						alpha = ((float)pStage->constantColor[3] * (1.0 - ((dot - lowest - range / 2) / (range / 2))));
					}
					if (alpha > 255.0)
					{
						alpha = 255.0;
					}
					else if (alpha < 0.0)
					{
						alpha = 0.0;
					}

					// finally, scale according to the entity's alpha
					if (backEnd.currentEntity->e.hModel)
					{
						alpha *= (float)backEnd.currentEntity->e.shaderRGBA[3] / 255.0;
					}

					tess.svars.color[3] = (byte) (alpha);
				}
				else
				{
					tess.svars.color[3] = 0;
				}
			}
			else
			{
				tess.svars.color[3] = 0;
			}
		}
#endif
	}
	break;
	case AGEN_WAVEFORM:
	{
		float      glow;
		waveForm_t *wf;

		wf = &pStage->alphaWave;

		glow = RB_EvalWaveFormClamped(wf);

		tess.svars.color[3] = glow;
		break;
	}
	case AGEN_CUSTOM:
	{
		alpha = Q_bound(0.0, RB_EvalExpression(&pStage->alphaExp, 1.0), 1.0);

		tess.svars.color[3] = alpha;
		break;
	}
	}
}


/*
===============
Tess_ComputeTexMatrices
===============
*/
static void Tess_ComputeTexMatrices(shaderStage_t *pStage)
{
	int   i;
	vec_t *matrix;

	Ren_LogComment("--- Tess_ComputeTexMatrices ---\n");

	for (i = 0; i < MAX_TEXTURE_BUNDLES; i++)
	{
		matrix = tess.svars.texMatrices[i];

		RB_CalcTexMatrix(&pStage->bundle[i], matrix);
		if (pStage->tcGen_Lightmap && i == TB_COLORMAP)
		{
			MatrixMultiplyScale(matrix,
			                    tr.fatLightmapStep,
			                    tr.fatLightmapStep,
			                    tr.fatLightmapStep);
		}
	}
}

/*
==============
SetIteratorFog
    set the fog parameters for this pass
==============
*/
static void SetIteratorFog()
{
	Ren_LogComment("--- SetIteratorFog() ---\n");

	// changed for problem when you start the game with r_fastsky set to '1'
//  if(r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		RB_FogOff();
		return;
	}

	if (backEnd.refdef.rdflags & RDF_DRAWINGSKY)
	{
		if (tr.glfogsettings[FOG_SKY].registered)
		{
			RB_Fog(&tr.glfogsettings[FOG_SKY]);
		}
		else
		{
			RB_FogOff();
		}

		return;
	}

	if (tr.world && tr.world->hasSkyboxPortal && (backEnd.refdef.rdflags & RDF_SKYBOXPORTAL))
	{
		if (tr.glfogsettings[FOG_PORTALVIEW].registered)
		{
			RB_Fog(&tr.glfogsettings[FOG_PORTALVIEW]);
		}
		else
		{
			RB_FogOff();
		}
	}
	else
	{
		if (tr.glfogNum > FOG_NONE)
		{
			RB_Fog(&tr.glfogsettings[FOG_CURRENT]);
		}
		else
		{
			RB_FogOff();
		}
	}
}

void Tess_StageIteratorDebug()
{
	Ren_LogComment("--- Tess_StageIteratorDebug( %i vertices, %i triangles ) ---\n", tess.numVertexes, tess.numIndexes / 3);

	GL_CheckErrors();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		// FIXME analyze required vertex attribs by the current material
		Tess_UpdateVBOs(0);
	}

	Tess_DrawElements();
}

static ID_INLINE GLenum RB_StencilOp(int op)
{
	switch (op & STO_MASK)
	{
	case STO_KEEP:
		return GL_KEEP;
	case STO_ZERO:
		return GL_ZERO;
	case STO_REPLACE:
		return GL_REPLACE;
	case STO_INVERT:
		return GL_INVERT;
	case STO_INCR:
		return GL_INCR_WRAP;
	case STO_DECR:
		return GL_DECR_WRAP;
	default:
		return GL_KEEP;
	}
}

static void RB_SetStencil(GLenum side, stencil_t *stencil)
{
	GLenum sfailOp, zfailOp, zpassOp;

	if (!side)
	{
		glDisable(GL_STENCIL_TEST);
		return;
	}

	if (!stencil->flags)
	{
		return;
	}

	glEnable(GL_STENCIL_TEST);
	switch (stencil->flags & STF_MASK)
	{
	case STF_ALWAYS:
		glStencilFuncSeparate(side, GL_ALWAYS, stencil->ref, stencil->mask);
		break;
	case STF_NEVER:
		glStencilFuncSeparate(side, GL_NEVER, stencil->ref, stencil->mask);
		break;
	case STF_LESS:
		glStencilFuncSeparate(side, GL_LESS, stencil->ref, stencil->mask);
		break;
	case STF_LEQUAL:
		glStencilFuncSeparate(side, GL_LEQUAL, stencil->ref, stencil->mask);
		break;
	case STF_GREATER:
		glStencilFuncSeparate(side, GL_GREATER, stencil->ref, stencil->mask);
		break;
	case STF_GEQUAL:
		glStencilFuncSeparate(side, GL_GEQUAL, stencil->ref, stencil->mask);
		break;
	case STF_EQUAL:
		glStencilFuncSeparate(side, GL_EQUAL, stencil->ref, stencil->mask);
		break;
	case STF_NEQUAL:
		glStencilFuncSeparate(side, GL_NOTEQUAL, stencil->ref, stencil->mask);
		break;
	}

	sfailOp = RB_StencilOp(stencil->flags >> STS_SFAIL);
	zfailOp = RB_StencilOp(stencil->flags >> STS_ZFAIL);
	zpassOp = RB_StencilOp(stencil->flags >> STS_ZPASS);
	glStencilOpSeparate(side, sfailOp, zfailOp, zpassOp);
	glStencilMaskSeparate(side, (GLuint) stencil->writeMask);
}

void Tess_StageIteratorGeneric()
{
	int stage;

	Ren_LogComment("--- Tess_StageIteratorGeneric( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.numVertexes, tess.numIndexes / 3);

	GL_CheckErrors();

	Tess_DeformGeometry();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		// FIXME analyze required vertex attribs by the current material
		Tess_UpdateVBOs(0);
	}

	if (tess.surfaceShader->fogVolume)
	{
		Render_volumetricFog();
		return;
	}

	// set GL fog
	SetIteratorFog();

	// set face culling appropriately
	GL_Cull(tess.surfaceShader->cullType);

	// set polygon offset if necessary
	if (tess.surfaceShader->polygonOffset)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// call shader function
	for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t *pStage = tess.surfaceStages[stage];

		if (!pStage)
		{
			break;
		}

		if (!RB_EvalExpression(&pStage->ifExp, 1.0))
		{
			continue;
		}

		// per stage fogging (detail textures)
		if (tess.surfaceShader->noFog && pStage->isFogged)
		{
			RB_FogOn();
		}
		else if (tess.surfaceShader->noFog && !pStage->isFogged)
		{
			RB_FogOff();
		}
		else
		{
			// make sure it's on
			RB_FogOn();
		}

		Tess_ComputeColor(pStage);
		Tess_ComputeTexMatrices(pStage);

		if (pStage->frontStencil.flags || pStage->backStencil.flags)
		{
			RB_SetStencil(GL_FRONT, &pStage->frontStencil);
			RB_SetStencil(GL_BACK, &pStage->backStencil);
		}

		if (pStage->bundle[0].isTcGen == qtrue)
		{
			vec3_t vec;

			VectorCopy(pStage->bundle[0].tcGenVectors[0], vec);
			SetUniformVec3(UNIFORM_TCGEN0VECTOR0, vec);
			VectorCopy(pStage->bundle[0].tcGenVectors[1], vec);
			SetUniformVec3(UNIFORM_TCGEN0VECTOR1, vec);
		}

		switch (pStage->type)
		{
		case ST_COLORMAP:
		{
			Render_generic(stage);
			break;
		}
		case ST_LIGHTMAP:
		{
			Render_lightMapping(stage, qtrue, qfalse);
			break;
		}
		case ST_DIFFUSEMAP:
		case ST_COLLAPSE_lighting_DB:
		case ST_COLLAPSE_lighting_DBS:
		{
			//if(tess.surfaceShader->sort <= SS_OPAQUE)
			{
				if (r_precomputedLighting->integer || r_vertexLighting->integer)
				{
					if (!r_vertexLighting->integer && tess.lightmapNum >= 0 && tess.lightmapNum < tr.lightmaps.currentElements)
					{
						if (tr.worldDeluxeMapping && r_normalMapping->integer)
						{
							Render_lightMapping(stage, qfalse, qtrue);
						}
						else
						{
							Render_lightMapping(stage, qfalse, qfalse);
						}
					}
					else if (backEnd.currentEntity != &tr.worldEntity)
					{
						Render_vertexLighting_DBS_entity(stage);
					}
					else
					{
						Render_vertexLighting_DBS_world(stage);
					}
				}
				else
				{
					Render_depthFill(stage);
				}
			}
			break;
		}
		case ST_COLLAPSE_reflection_CB:
		case ST_REFLECTIONMAP:
		{
			if (r_reflectionMapping->integer)
			{
				Render_reflection_CB(stage);
			}
			break;
		}
		case ST_REFRACTIONMAP:
		{
			Render_refraction_C(stage);
			break;
		}
		case ST_DISPERSIONMAP:
		{
			Render_dispersion_C(stage);
			break;
		}
		case ST_SKYBOXMAP:
		{
			Render_skybox(stage);
			break;
		}
		case ST_SCREENMAP:
		{
			Render_screen(stage);
			break;
		}
		case ST_PORTALMAP:
		{
			Render_portal(stage);
			break;
		}
		case ST_HEATHAZEMAP:
		{
			Render_heatHaze(stage);
			break;
		}
		case ST_LIQUIDMAP:
		{
			Render_liquid(stage);
			break;
		}
		default:
			break;
		}

		if (pStage->frontStencil.flags || pStage->backStencil.flags)
		{
			RB_SetStencil(0, NULL);
		}

		if (r_showLightMaps->integer && pStage->type == ST_LIGHTMAP)
		{
			break;
		}
	}

	if (!r_noFog->integer && tess.fogNum >= 1 && tess.surfaceShader->fogPass)
	{
		Render_fog();
	}

	// reset polygon offset
	if (tess.surfaceShader->polygonOffset)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void Tess_StageIteratorDepthFill()
{
	int stage;

	Ren_LogComment("--- Tess_StageIteratorDepthFill( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.numVertexes, tess.numIndexes / 3);

	GL_CheckErrors();

	Tess_DeformGeometry();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		Tess_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD);
	}

	// set face culling appropriately
	GL_Cull(tess.surfaceShader->cullType);

	// set polygon offset if necessary
	if (tess.surfaceShader->polygonOffset)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// call shader function
	for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t *pStage = tess.surfaceStages[stage];

		if (!pStage)
		{
			break;
		}

		if (!RB_EvalExpression(&pStage->ifExp, 1.0))
		{
			continue;
		}

		Tess_ComputeTexMatrices(pStage);

		switch (pStage->type)
		{
		case ST_COLORMAP:
		{
			if (tess.surfaceShader->sort <= SS_OPAQUE)
			{
				Render_depthFill(stage);
			}
			break;
		}
		case ST_LIGHTMAP:
		{
			Render_depthFill(stage);
			break;
		}
		case ST_DIFFUSEMAP:
		case ST_COLLAPSE_lighting_DB:
		case ST_COLLAPSE_lighting_DBS:
		{
			Render_depthFill(stage);
			break;
		}
		default:
			break;
		}
	}

	// reset polygon offset
	glDisable(GL_POLYGON_OFFSET_FILL);
}

void Tess_StageIteratorShadowFill()
{
	int stage;

	Ren_LogComment("--- Tess_StageIteratorShadowFill( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.numVertexes, tess.numIndexes / 3);

	GL_CheckErrors();

	Tess_DeformGeometry();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		Tess_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD);
	}

	// set face culling appropriately
	GL_Cull(tess.surfaceShader->cullType);

	// set polygon offset if necessary
	if (tess.surfaceShader->polygonOffset)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// call shader function
	for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t *pStage = tess.surfaceStages[stage];

		if (!pStage)
		{
			break;
		}

		if (!RB_EvalExpression(&pStage->ifExp, 1.0))
		{
			continue;
		}

		Tess_ComputeTexMatrices(pStage);

		switch (pStage->type)
		{
		case ST_COLORMAP:
		{
			if (tess.surfaceShader->sort <= SS_OPAQUE)
			{
				Render_shadowFill(stage);
			}
			break;
		}

		case ST_LIGHTMAP:
		case ST_DIFFUSEMAP:
		case ST_COLLAPSE_lighting_DB:
		case ST_COLLAPSE_lighting_DBS:
		{
			Render_shadowFill(stage);
			break;
		}

		default:
			break;
		}
	}

	// reset polygon offset
	glDisable(GL_POLYGON_OFFSET_FILL);
}

void Tess_StageIteratorLighting()
{
	int           i, j;
	trRefLight_t  *light;
	shaderStage_t *attenuationXYStage;
	shaderStage_t *attenuationZStage;

	Ren_LogComment("--- Tess_StageIteratorLighting( %s, %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.lightShader->name, tess.numVertexes, tess.numIndexes / 3);

	GL_CheckErrors();

	light = backEnd.currentLight;

	Tess_DeformGeometry();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		// FIXME analyze required vertex attribs by the current material
		Tess_UpdateVBOs(0);
	}

	// set OpenGL state for lighting
	if (light->l.inverseShadows)
	{
		GL_State(GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);
	}
	else
	{
		if (tess.surfaceShader->sort > SS_OPAQUE)
		{
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		}
		else
		{
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
		}
	}

	// set face culling appropriately
	GL_Cull(tess.surfaceShader->cullType);

	// set polygon offset if necessary
	if (tess.surfaceShader->polygonOffset)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// call shader function
	attenuationZStage = tess.lightShader->stages[0];

	for (i = 0; i < MAX_SHADER_STAGES; i++)
	{
		shaderStage_t *diffuseStage = tess.surfaceStages[i];

		if (!diffuseStage)
		{
			break;
		}

		if (!RB_EvalExpression(&diffuseStage->ifExp, 1.0))
		{
			continue;
		}

		Tess_ComputeTexMatrices(diffuseStage);

		for (j = 1; j < MAX_SHADER_STAGES; j++)
		{
			attenuationXYStage = tess.lightShader->stages[j];

			if (!attenuationXYStage)
			{
				break;
			}

			if (attenuationXYStage->type != ST_ATTENUATIONMAP_XY)
			{
				continue;
			}

			if (!RB_EvalExpression(&attenuationXYStage->ifExp, 1.0))
			{
				continue;
			}

			Tess_ComputeColor(attenuationXYStage);
			R_ComputeFinalAttenuation(attenuationXYStage, light);

			switch (diffuseStage->type)
			{
			case ST_DIFFUSEMAP:
			case ST_COLLAPSE_lighting_DB:
			case ST_COLLAPSE_lighting_DBS:
				if (light->l.rlType == RL_OMNI)
				{
					Render_forwardLighting_DBS_omni(diffuseStage, attenuationXYStage, attenuationZStage, light);
				}
				else if (light->l.rlType == RL_PROJ)
				{
					if (!light->l.inverseShadows)
					{
						Render_forwardLighting_DBS_proj(diffuseStage, attenuationXYStage, attenuationZStage, light);
					}
				}
				else if (light->l.rlType == RL_DIRECTIONAL)
				{
					//if(!light->l.inverseShadows)
					{
						Render_forwardLighting_DBS_directional(diffuseStage, attenuationXYStage, attenuationZStage, light);
					}
				}
				break;

			default:
				break;
			}
		}
	}

	// reset polygon offset
	if (tess.surfaceShader->polygonOffset)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

/*
=================
Tess_End

Render tesselated data
=================
*/
void Tess_End()
{
	if ((tess.numIndexes == 0 || tess.numVertexes == 0) && tess.multiDrawPrimitives == 0)
	{
		return;
	}

	if (tess.indexes[SHADER_MAX_INDEXES - 1] != 0)
	{
		Ren_Drop("Tess_End() - SHADER_MAX_INDEXES hit");
	}
	if (tess.xyz[SHADER_MAX_VERTEXES - 1][0] != 0)
	{
		Ren_Drop("Tess_End() - SHADER_MAX_VERTEXES hit");
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if (r_debugSort->integer && r_debugSort->integer < tess.surfaceShader->sort)
	{
		return;
	}

	// update performance counter
	backEnd.pc.c_batches++;

	GL_CheckErrors();

	// call off to shader specific tess end function
	tess.stageIteratorFunc();

	if ((tess.stageIteratorFunc != Tess_StageIteratorShadowFill) &&
	    (tess.stageIteratorFunc != Tess_StageIteratorDebug))
	{
		// draw debugging stuff
		if (r_showTris->integer || r_showBatches->integer || (r_showLightBatches->integer && (tess.stageIteratorFunc == Tess_StageIteratorLighting)))
		{
			DrawTris();
		}
	}

	tess.vboVertexSkinning = qfalse;

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;

	Ren_LogComment("--- Tess_End ---\n");

	GL_CheckErrors();
}
