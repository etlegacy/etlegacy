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
 * @file renderer2/tr_shade.c
 *
 * @brief THIS ENTIRE FILE IS BACK END!
 * This file deals with applying shaders to surface data in the tess struct.
 */

#include "tr_local.h"

/*
 * @brief MyMultiDrawElements
 * @param[in] mode
 * @param[in] count
 * @param[in] type
 * @param[in] indices
 * @param[in] primcount
 *
 * @note Unused
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

/**
 * @brief Tess_DrawElements
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

/**
 * @brief BindLightMap
 */
static void BindLightMap()
{
	image_t *lightmap = NULL;

	if (tr.lightmaps.currentElements && tess.lightmapNum >= 0 && tess.lightmapNum < tr.lightmaps.currentElements)
	{
		lightmap = (image_t *)Com_GrowListElement(&tr.lightmaps, tess.lightmapNum);
	}

	if (lightmap)
	{
		GL_Bind(lightmap);
	}
	else
	{
		GL_Bind(tr.whiteImage); // LIGHTMAP_WHITEIMAGE
	}
}

/**
 * @brief BindDeluxeMap
 * unused
static void BindDeluxeMap(shaderStage_t *pStage)
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
		// hack: use TB_NORMALMAP image when r_normalMapping is enabled and there is no deluxe map available
		if (r_normalMapping->integer && !tr.worldDeluxeMapping && pStage->bundle[TB_NORMALMAP].image[0] != NULL)
		{
			GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
		}
		else
		{
			GL_Bind(tr.flatImage);
		}
		return;
	}

	GL_Bind(deluxemap);
}
*/

/**
* @brief BindCubeMaps
*/
static void BindCubeMaps()
{
/*
	//cubemapProbe_t *cubeProbe1, *cubeProbe2;
	qboolean       isEntity  = (backEnd.currentEntity && (backEnd.currentEntity != &tr.worldEntity));
	vec3_t         *position = (isEntity) ? &backEnd.currentEntity->e.origin : &backEnd.viewParms.orientation.origin;
	image_t        *env0, *env1;
	float          interpolate;

	R_FindCubeprobes(position, backEnd.currentEntity, &env0, &env1, &interpolate);

	// bind u_EnvironmentMap0
	SelectTexture(TEX_ENVMAP0);
	GL_Bind(env0);

	// bind u_EnvironmentMap1
	SelectTexture(TEX_ENVMAP1);
	GL_Bind(env1);

	// u_EnvironmentInterpolation
	SetUniformFloat(UNIFORM_ENVIRONMENTINTERPOLATION, interpolate);
*/

	R_FindCubeprobes(backEnd.viewParms.orientation.origin, &tr.worldEntity, &tr.reflectionData.env0, &tr.reflectionData.env1, &tr.reflectionData.interpolate);
	// bind u_EnvironmentMap0
	SelectTexture(TEX_ENVMAP0);
	GL_Bind(tr.reflectionData.env0);
	// bind u_EnvironmentMap1
	SelectTexture(TEX_ENVMAP1);
	GL_Bind(tr.reflectionData.env1);
	// u_EnvironmentInterpolation
	SetUniformFloat(UNIFORM_ENVIRONMENTINTERPOLATION, tr.reflectionData.interpolate);

/*
//if (tr.refdef.pixelTarget == NULL)
//	R_BuildCurrentCubeMap();                       // this is now done in renderscene..   this is now gone..
	// bind u_EnvironmentMap0
	SelectTexture(TEX_ENVMAP0);
	GL_Bind(tr.currentCubemapFBOImage);
	// bind u_EnvironmentMap1
	SelectTexture(TEX_ENVMAP1);
	GL_Bind(tr.currentCubemapFBOImage);
	// u_EnvironmentInterpolation
	SetUniformFloat(UNIFORM_ENVIRONMENTINTERPOLATION, 0.f);
*/
}


bspGridPoint_t *LightgridColor(const vec3_t position)
{
	vec3_t         lightOrigin;
	int            pos[3];
	//float          frac[3];
	int            gridStep[3];
	bspGridPoint_t *gridPoint;
	int            i;
	float          v;

	etl_assert(tr.world->lightGridData);   // NULL with -nolight maps
	//VectorSubtract(lightOrigin, tr.world->lightGridOrigin, lightOrigin);
	lightOrigin[0] = position[0] - tr.world->lightGridOrigin[0];
	lightOrigin[1] = position[1] - tr.world->lightGridOrigin[1];
	lightOrigin[2] = position[2] - tr.world->lightGridOrigin[2];
	// index x,y,z in the lightgrid
	for (i = 0; i < 3; i++)
	{
		v = lightOrigin[i] * tr.world->lightGridInverseSize[i];
		pos[i] = floor(v);
		//frac[i] = v - pos[i];
		if (pos[i] < 0)
		{
			pos[i] = 0;
		}
		else if (pos[i] > tr.world->lightGridBounds[i] - 1)
		{
			pos[i] = tr.world->lightGridBounds[i] - 1;
		}
	}
	gridStep[0] = 1; //sizeof(bspGridPoint_t);
	gridStep[1] = tr.world->lightGridBounds[0]; // * sizeof(bspGridPoint_t);
	gridStep[2] = tr.world->lightGridBounds[0] * tr.world->lightGridBounds[1]; // * sizeof(bspGridPoint_t);
	gridPoint = tr.world->lightGridData + pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];
	return gridPoint;
}

/**
 * @brief Draws triangle outlines for debugging
 */
static void DrawTris()
{
	vec_t *color;

	// exclude 2d from drawing tris
	// we might add a flag for r_showtris to show 2D again but who wants to see this?!
	// (values 2 + 3  are used by r1!) 
	if (tess.surfaceShader->type == SHADER_2D)
	{
		return;
	}

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
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(tr.whiteImage);
	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrix);

	glDepthRange(0, 0);

	Tess_DrawElements();

	glDepthRange(0, 1);
}

// *INDENT-OFF*
/**
 * @brief We must set some things up before beginning any tesselation,
 * because a surface may be forced to perform a Tess_End due
 * to overflow.
 *
 * @param stageIteratorFunc
 * @param stageIteratorFunc2
 * @param[in] surfaceShader
 * @param[in] lightShader
 * @param[in] skipTangentSpaces
 * @param[in] skipVBO
 * @param[in] lightmapNum
 * @param[in] fogNum
 */
void Tess_Begin(void (*stageIteratorFunc)(),
                void (*stageIteratorFunc2)(),
                shader_t *surfaceShader, shader_t *lightShader,
                qboolean skipTangentSpaces,
                qboolean skipVBO,
                int lightmapNum,
                int fogNum)
{
	shader_t *state;

	tess.numIndexes          = 0;
	tess.numVertexes         = 0;
	tess.attribsSet          = 0;
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
		if (state && state->isSky)
		{
			tess.stageIteratorFunc  = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorGeneric;
		}
	}
	/*
	else if (tess.stageIteratorFunc == &Tess_StageIteratorDepthFill)
	{
		if (isSky)
		{
			tess.stageIteratorFunc  = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorDepthFill;
		}
	}
	*/

	tess.skipTangentSpaces = skipTangentSpaces;
	tess.skipVBO           = skipVBO;
	tess.lightmapNum       = lightmapNum;
	tess.fogNum            = fogNum;

	if (tess.surfaceShader)
	{
		tess.shaderTime = backEnd.refdef.floatTime - tess.surfaceShader->timeOffset;
		if (tess.surfaceShader->clampTime && tess.shaderTime >= tess.surfaceShader->clampTime)
		{
			tess.shaderTime = tess.surfaceShader->clampTime;
		}
	}

	Ren_LogComment("--- Tess_Begin( surfaceShader = %s, lightShader = %s, skipTangentSpaces = %i, lightmapNum = %i, fogNum = %i) ---\n", tess.surfaceShader->name, tess.lightShader ? tess.lightShader->name : NULL, tess.skipTangentSpaces, tess.lightmapNum, tess.fogNum);
}
// *INDENT-ON*

/**
 * @brief clipping portal plane in world space
 */
void clipPortalPlane() // static for now - might be used in tr_main.c and tr_sky.c
{
	vec4_t plane;

	plane[0] = backEnd.viewParms.portalPlane.normal[0];
	plane[1] = backEnd.viewParms.portalPlane.normal[1];
	plane[2] = backEnd.viewParms.portalPlane.normal[2];
	plane[3] = backEnd.viewParms.portalPlane.dist;
	SetUniformVec4(UNIFORM_PORTALPLANE, plane);
}

typedef struct rgbaGen_s {
	colorGen_t color;
	alphaGen_t alpha;
} rgbaGen_t;

/**
 * @brief getRgbaGen
 *
 * @param pStage
 * @param lightmapNum
 */
static rgbaGen_t getRgbaGen(shaderStage_t *pStage, int lightmapNum)
{
    // always exclude the sky
    if (tess.surfaceShader->isSky)
    {
        rgbaGen_t rgbaGen = { pStage->rgbGen, pStage->alphaGen };

        return rgbaGen;
    }
    else
    {
        qboolean isVertexLit           = (qboolean) (lightmapNum == LIGHTMAP_BY_VERTEX || lightmapNum == LIGHTMAP_WHITEIMAGE);
        qboolean shouldForceCgenVertex = (qboolean) (isVertexLit && pStage->rgbGen == CGEN_IDENTITY);
        qboolean shouldForceAgenVertex = (qboolean) (isVertexLit && pStage->alphaGen == AGEN_IDENTITY);
        int colorGen                   = shouldForceCgenVertex ? CGEN_VERTEX : pStage->rgbGen;
        int alphaGen                   = shouldForceAgenVertex ? AGEN_VERTEX : pStage->alphaGen;
        rgbaGen_t rgbaGen              = { colorGen, alphaGen };

        return rgbaGen;
    }
}

/**
 * @brief getRgbaGenForColorModulation
 *
 * @param pStage
 * @param lightmapNum
 */
static rgbaGen_t getRgbaGenForColorModulation(shaderStage_t *pStage, int lightmapNum)
{
	rgbaGen_t rgbaGen;

	rgbaGen = getRgbaGen(pStage, lightmapNum);

	// u_ColorGen
	switch (rgbaGen.color)
	{
	case CGEN_VERTEX:
	case CGEN_ONE_MINUS_VERTEX:
		break;
	default:
		rgbaGen.color = CGEN_CONST;
		break;
	}

	// u_AlphaGen
	switch (rgbaGen.alpha)
	{
	case AGEN_VERTEX:
	case AGEN_ONE_MINUS_VERTEX:
		break;
	default:
		rgbaGen.alpha = AGEN_CONST;
		break;
	}

	return rgbaGen;
}

/**
 * @brief Sets light related uniforms (currently sun related only)
 * @param[in]
 *
 * @fixme this should set current light direction (and color) on not always the sun
 * @fixme deal with tess.svars.color?
 */
void SetLightUniforms(qboolean setLightColor)
{
	// note: there is always a default sunDirection set in RE_LoadWorldMap
	//       but sunLight is depending on real sun shader
	{
		SetUniformVec3(UNIFORM_LIGHTDIR, tr.sunDirection);
		if (setLightColor)
		{
			SetUniformVec3(UNIFORM_LIGHTCOLOR, tr.sunLight);
		}
	}
}

/**
 * @brief Render_generic
 * @param[in] stage
 *
 * This function is only called for stage.type: ST_COLORMAP, ST_LIGHTMAP, ST_DIFFUSEMAP, ST_BUNDLE_DB, ST_BUNDLE_DBS
*/
static void Render_generic(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];
	// if this is a tcGen-environment texture, but reflectionmapping is disabled, we can abort this function.
	if (pStage->tcGen_Environment && pStage->type == ST_TCGENENVMAP && !r_reflectionMapping->integer)
	{
		return;
	}
	qboolean use_alphaTesting     = (pStage->stateBits & GLS_ATEST_BITS) != 0;
	qboolean use_vertex_skinning  = glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning;
	qboolean use_vertex_animation = glState.vertexAttribsInterpolation > 0;
	qboolean use_tcgenEnv         = r_reflectionMapping->integer && pStage->tcGen_Environment && pStage->type == ST_TCGENENVMAP;
	rgbaGen_t rgbaGen;

	Ren_LogComment("--- Render_generic ---\n");

	GL_State(pStage->stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_genericShader,
	                          USE_ALPHA_TESTING, use_alphaTesting,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, use_vertex_skinning,
	                          USE_VERTEX_ANIMATION, use_vertex_animation,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_TCGEN_ENVIRONMENT, use_tcgenEnv,
	                          USE_TCGEN_LIGHTMAP, pStage->tcGen_Lightmap);
	// end choose right shader program ------------------------------


	rgbaGen = getRgbaGenForColorModulation(pStage, tess.lightmapNum);
	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, rgbaGen.color, rgbaGen.alpha);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);
	SetUniformMatrix16(UNIFORM_MODELMATRIX, MODEL_MATRIX);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// USE_ALPHA_TESTING
	if (use_alphaTesting)
	{
		GLSL_SetUniform_AlphaTest(pStage->stateBits);
	}

	// USE_VERTEX_SKINNING
	if (use_vertex_skinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// USE_VERTEX_ANIMATION
	if (use_vertex_animation)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	// USE_DEFORM_VERTEXES
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	// USE_PORTAL_CLIPPING
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// USE_TCGEN_ENVIRONMENT
	if (use_tcgenEnv)
	{
		// calculate the environment texcoords in object space
		// origin for object, vieworigin for camera here we need to use object for reflections
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	BindAnimatedImage(&pStage->bundle[TB_COLORMAP]);
	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrix);

	GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_entity
 * @param[in] stage
 *
 * This function is only called for stage.type: ST_DIFFUSEMAP, ST_BUNDLE_DB, ST_BUNDLE_DBS, ST_BUNDLE_DBSR
*/
static void Render_entity(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];
	qboolean use_normalMapping    = r_normalMapping->integer && pStage->type != ST_DIFFUSEMAP;
	qboolean use_parallaxMapping  = use_normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax;
	qboolean use_alphaTesting     = (pStage->stateBits & GLS_ATEST_BITS) != 0;
	qboolean use_deform           = tess.surfaceShader->numDeforms > 0;
	qboolean use_vertex_skinning  = glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning;
	qboolean use_vertex_animation = glState.vertexAttribsInterpolation > 0;
	qboolean use_specular         = use_normalMapping && (pStage->type == ST_BUNDLE_DBS || pStage->type == ST_BUNDLE_DBSR);
	qboolean use_reflections      = use_normalMapping && r_reflectionMapping->integer && pStage->type == ST_BUNDLE_DBSR &&
										tr.cubeProbes.currentElements > 0 && tr.refdef.pixelTarget == NULL;
	qboolean use_reflectionmap    = use_reflections && pStage->type == ST_BUNDLE_DBSR;
	// !tr.refdef.pixelTarget to prevent using reflections before buildcubemaps() has finished. This is anti eye-cancer..
if (tr.refdef.pixelTarget != NULL)
{
	use_normalMapping = qfalse;
	use_parallaxMapping = qfalse;
	use_deform = qfalse;
	use_vertex_animation = qfalse;
	use_specular = qfalse;
	use_reflections = qfalse;
}

	Ren_LogComment("--- Render_entity ---\n");

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_entityShader,
								USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
								USE_ALPHA_TESTING, use_alphaTesting,
								USE_VERTEX_SKINNING, use_vertex_skinning,
								USE_VERTEX_ANIMATION, use_vertex_animation,
								USE_DEFORM_VERTEXES, use_deform, // && !ShaderRequiresCPUDeforms(tess.surfaceShader),
								USE_NORMAL_MAPPING, use_normalMapping,
								USE_PARALLAX_MAPPING, use_parallaxMapping,
								USE_REFLECTIONS, use_reflections,
								USE_REFLECTIONMAP, use_reflectionmap,
								USE_SPECULAR, use_specular);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, MODEL_MATRIX); // same as backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	SetUniformVec3(UNIFORM_AMBIENTCOLOR, backEnd.currentEntity->ambientLight);
	SetUniformVec3(UNIFORM_LIGHTDIR, backEnd.currentEntity->lightDir);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, backEnd.currentEntity->directedLight);

	//SetUniformFloat(UNIFORM_DIFFUSELIGHTING, r_diffuseLighting->value); // entities use a constant value (half-lambert 0.5)

	// set uniforms
	if (tess.surfaceShader->numDeforms)
	{
		// u_DeformGen
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime); // u_time
	}

	if (use_vertex_skinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
		//SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, glConfig2.maxVertexSkinningBones);
	}

	if (use_vertex_animation)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (use_alphaTesting)
	{
		GLSL_SetUniform_AlphaTest(pStage->stateBits);
	}

	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&pStage->wrapAroundLightingExp, 0));
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
//if (pStage->bundle[TB_DIFFUSEMAP].image[0])
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
//else if (pStage->bundle[TB_COLORMAP].image[0])
//	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
//else GL_Bind(tr.whiteImage);
	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrix);

	if (use_normalMapping)
	{
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space

		if (use_parallaxMapping)
		{
			SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value));
			// parallax self shadowing
			SetUniformFloat(UNIFORM_PARALLAXSHADOW, r_parallaxShadow->value);
		}

		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);

		if (use_specular)
		{
/*qboolean isPlayer = (backEnd.currentEntity->e.entityNum < MAX_CLIENTS); // or is bot TODO
if (isPlayer)
{
SetUniformFloat(UNIFORM_SPECULARSCALE, r_specularScalePlayers->value);
SetUniformFloat(UNIFORM_SPECULAREXPONENT, r_specularExponentPlayers->value);
}
else
{*/
			SetUniformFloat(UNIFORM_SPECULARSCALE, r_specularScaleEntities->value);
			SetUniformFloat(UNIFORM_SPECULAREXPONENT, r_specularExponentEntities->value);
/*}*/

			// bind u_SpecularMap
			SelectTexture(TEX_SPECULAR);
			GL_Bind(pStage->bundle[TB_SPECULARMAP].image[0]);
		}
		if (use_reflections)
		{
			SetUniformFloat(UNIFORM_REFLECTIONSCALE, r_reflectionScale->value);
			// bind the 2 nearest cubeProbes
			BindCubeMaps();
			// bind the reflectionmap
			if (use_reflectionmap)
			{
				// bind u_ReflectionMap
				SelectTexture(TEX_REFLECTION);
				GL_Bind(pStage->bundle[TB_REFLECTIONMAP].image[0]);
			}
		}
	}
	GLSL_SetRequiredVertexPointers(trProg.gl_entityShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_vertexLighting_DBS_world
 * @param[in] stage
 *
 * This function is only called for stage.type: ST_DIFFUSEMAP, ST_BUNDLE_DB, ST_BUNDLE_DBS, ST_BUNDLE_DBSR
*/
/*
static void Render_vertexLighting_DBS_world(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];
	rgbaGen_t rgbaGen;
	qboolean use_normalMapping   = r_normalMapping->integer && pStage->type != ST_DIFFUSEMAP;
	qboolean use_alphaTesting    = (pStage->stateBits & GLS_ATEST_BITS) != 0;
	qboolean use_parallaxMapping = use_normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax;
	qboolean use_specular        = use_normalMapping && (pStage->type == ST_BUNDLE_DBS || pStage->type == ST_BUNDLE_DBSR);
	qboolean use_reflections     = r_normalMapping->integer && r_reflectionMapping->integer &&
										pStage->type == ST_BUNDLE_DBSR &&
										tr.cubeProbes.currentElements > 0 && !tr.refdef.pixelTarget;
	qboolean use_reflectionmap    = use_reflections && pStage->type == ST_BUNDLE_DBSR;
	Ren_LogComment("--- Render_vertexLighting_DBS_world ---\n");

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_vertexLightingShader_DBS_world,
								USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
								USE_ALPHA_TESTING, use_alphaTesting,
								USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms, // && !ShaderRequiresCPUDeforms(tess.surfaceShader),
								USE_NORMAL_MAPPING, use_normalMapping,
								USE_PARALLAX_MAPPING, use_parallaxMapping,
								USE_REFLECTIONS, use_reflections,
								USE_REFLECTIONMAP, use_reflectionmap,
								USE_SPECULAR, use_specular);

	GLSL_SetRequiredVertexPointers(trProg.gl_vertexLightingShader_DBS_world);

	rgbaGen = getRgbaGenForColorModulation(pStage, tess.lightmapNum);
	GLSL_SetUniform_ColorModulate(trProg.gl_vertexLightingShader_DBS_world, rgbaGen.color, rgbaGen.alpha);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	SetUniformMatrix16(UNIFORM_MODELMATRIX, MODEL_MATRIX); // same as backEnd.orientation.transformMatrix

	SetUniformFloat(UNIFORM_DIFFUSELIGHTING, r_diffuseLighting->value);

	// SetUniformVec3(UNIFORM_LIGHTDIR, backEnd.currentEntity->lightDir);
	// SetUniformVec3(UNIFORM_LIGHTCOLOR, backEnd.currentEntity->directedLight);

	///SetUniformVec3(UNIFORM_LIGHTDIR, backEnd.currentLight->direction);
	///vec4_t color;
	///color[0] = backEnd.currentLight->l.color[0];
	///color[1] = backEnd.currentLight->l.color[1];
	///color[2] = backEnd.currentLight->l.color[2];
	///color[3] = 1.0;
	///SetUniformVec3(UNIFORM_LIGHTCOLOR, color);
	/////SetLightUniforms(qtrue);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, tr.sunLight);

	// USE_ALPHA_TESTING
	if (use_alphaTesting)
	{
		GLSL_SetUniform_AlphaTest(pStage->stateBits);
	}

	// USE_DEFORM_VERTEXES
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&pStage->wrapAroundLightingExp, 0));
	}

	// USE_PORTAL_CLIPPING
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrix);

	if (use_normalMapping)
	{
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
		SetUniformVec3(UNIFORM_LIGHTDIR, tr.sunDirection);

		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);

		if (use_parallaxMapping)
		{
			SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value));
		}

		if (use_specular)
		{
			SetUniformFloat(UNIFORM_SPECULARSCALE, r_specularScaleWorld->value);
			SetUniformFloat(UNIFORM_SPECULAREXPONENT, r_specularExponentWorld->value);
			// bind u_SpecularMap
			SelectTexture(TEX_SPECULAR);
			GL_Bind(pStage->bundle[TB_SPECULARMAP].image[0]);
		}

		if (use_reflections)
		{
			SetUniformFloat(UNIFORM_REFLECTIONSCALE, r_reflectionScale->value);
			// bind the 2 nearest cubeProbes
			BindCubeMaps();
			// bind the reflectionmap
			if (use_reflectionmap)
			{
				// bind u_ReflectionMap
				SelectTexture(TEX_REFLECTION);
				GL_Bind(pStage->bundle[TB_REFLECTIONMAP].image[0]);
			}
		}
	}

	Tess_DrawElements();

	GL_CheckErrors();
}
*/

/**
 * @brief Render_world
 * @param[in] stage
 *
 * This function is only called for stage.type: ST_DIFFUSEMAP, ST_BUNDLE_DB, ST_BUNDLE_DBS, ST_BUNDLE_DBSR
 * It can render the surface with a lightmap.
*/
static void Render_world(int stage, qboolean use_lightMapping)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];
	uint32_t stateBits = pStage->stateBits;
	rgbaGen_t rgbaGen;
	qboolean use_diffuse         = pStage->bundle[TB_DIFFUSEMAP].image[0] != NULL;
	qboolean use_alphaTesting    = use_diffuse && (pStage->stateBits & GLS_ATEST_BITS);
	qboolean use_normalMapping   = use_diffuse && r_normalMapping->integer && (pStage->type == ST_BUNDLE_DBS || pStage->type == ST_BUNDLE_DB || pStage->type == ST_BUNDLE_DBSR);
	qboolean use_parallaxMapping = use_normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax;
	//qboolean use_deluxeMapping = use_normalMapping && qfalse); // r_showDeluxeMaps->integer == 1 AND a deluxemap exists!    because there is no code that does anything with deluxemaps, we disable it..
	qboolean use_deform          = tess.surfaceShader->numDeforms > 0;
	qboolean use_specular        = use_normalMapping && (pStage->type == ST_BUNDLE_DBS || pStage->type == ST_BUNDLE_DBSR);
	qboolean use_reflections     = use_normalMapping && r_reflectionMapping->integer &&
									pStage->type == ST_BUNDLE_DBSR &&
									tr.cubeProbes.currentElements > 0 && tr.refdef.pixelTarget == NULL;
	// !tr.refdef.pixelTarget to prevent using reflections before buildcubemaps() has finished. This is anti eye-cancer..
	qboolean use_reflectionmap   = use_reflections && (pStage->type == ST_BUNDLE_DBSR);
	// for now we pass USE_REFLECTIONS & USE_REFLECTIONMAP, but the lightmapping shader will only reflect when there's a reflectionmap assigned.
if (tr.refdef.pixelTarget != NULL)
{
	use_normalMapping = qfalse;
	use_parallaxMapping = qfalse;
	use_deform = qfalse;
	use_specular = qfalse;
	use_reflections = qfalse;
}

	Ren_LogComment("--- Render_world ---\n");

	if (r_showLightMaps->integer)
	{
		stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_ATEST_BITS);
	}

	GL_State(stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_worldShader,
								USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
								USE_ALPHA_TESTING, use_alphaTesting,
								USE_DEFORM_VERTEXES, use_deform, // && !ShaderRequiresCPUDeforms(tess.surfaceShader),
								USE_NORMAL_MAPPING, use_normalMapping,
								USE_PARALLAX_MAPPING, use_parallaxMapping,
								//USE_DELUXE_MAPPING, use_deluxeMapping,
								USE_REFLECTIONS, use_reflections,
								USE_REFLECTIONMAP, use_reflectionmap,
								USE_SPECULAR, use_specular,
								USE_DIFFUSE, use_diffuse,
								USE_LIGHT_MAPPING, use_lightMapping);

	GLSL_SetRequiredVertexPointers(trProg.gl_worldShader);

	rgbaGen = getRgbaGenForColorModulation(pStage, tess.lightmapNum);
	GLSL_SetUniform_ColorModulate(trProg.gl_worldShader, rgbaGen.color, rgbaGen.alpha);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	SetUniformFloat(UNIFORM_DIFFUSELIGHTING, r_diffuseLighting->value);

	// ..
	SetUniformBoolean(UNIFORM_B_SHOW_LIGHTMAP, (r_showLightMaps->integer == 1 ? GL_TRUE : GL_FALSE));
	//SetUniformBoolean(UNIFORM_B_SHOW_DELUXEMAP, (r_showDeluxeMaps->integer == 1 ? GL_TRUE : GL_FALSE));

	// USE_PORTAL_CLIPPING
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// USE_DEFORM_VERTEXES
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	// USE_ALPHA_TESTING
	if (use_alphaTesting)
	{
		GLSL_SetUniform_AlphaTest(pStage->stateBits);
	}

	if (use_lightMapping)
	{
		// bind lightMap
		SelectTexture(TEX_LIGHTMAP);
		BindLightMap();
	}

	// bind diffuse texture
	// sometimes the ST_LIGHTMAP stage has no diffusemap.
	if (use_diffuse) {
		SelectTexture(TEX_DIFFUSE);
		GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
		SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrix);

		// render the texture(map)s
		if (use_normalMapping)
		{
			SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space

			//SetUniformVec3(UNIFORM_LIGHTDIR, backEnd.currentEntity->lightDir);
			SetUniformVec3(UNIFORM_LIGHTDIR, tr.sunDirection);
			SetUniformVec3(UNIFORM_LIGHTCOLOR, tr.sunLight);

			// USE_PARALLAX_MAPPING
			if (use_parallaxMapping)
			{
				SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value));
				// parallax self shadowing
				SetUniformFloat(UNIFORM_PARALLAXSHADOW, r_parallaxShadow->value);
			}

			// bind u_NormalMap
			SelectTexture(TEX_NORMAL);
			GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);

			// the scale of the bumps
			SetUniformFloat(UNIFORM_BUMPSCALE, r_bumpScale->value);

			// USE_SPECULAR
			if (use_specular)
			{
				// when underwater we use plenty of specular, so it looks wet
				if (tr.refdef.rdflags & RDF_UNDERWATER)
				{
					SetUniformFloat(UNIFORM_SPECULARSCALE, r_specularScaleWorld->value * 5.f);
					SetUniformFloat(UNIFORM_SPECULAREXPONENT, 64.f); //r_specularExponentWorld->value * 0.25f);
				} else {
					SetUniformFloat(UNIFORM_SPECULARSCALE, r_specularScaleWorld->value);
					SetUniformFloat(UNIFORM_SPECULAREXPONENT, r_specularExponentWorld->value);
				}

				// the specularmap
				SelectTexture(TEX_SPECULAR);
				GL_Bind(pStage->bundle[TB_SPECULARMAP].image[0]);
			}

			// USE_REFLECTIONS
			if (use_reflections) //the lightmap shader only renders reflections if there's a reflectionmap
			{
				SetUniformFloat(UNIFORM_REFLECTIONSCALE, r_reflectionScale->value);
				BindCubeMaps();
				if (use_reflectionmap)
				{
					// bind u_ReflectionMap
					SelectTexture(TEX_REFLECTION);
					GL_Bind(pStage->bundle[TB_REFLECTIONMAP].image[0]);
				}
			}

			//if (use_deluxeMapping) {
			//	SelectTexture(TEX_DELUXE);
			//	BindDeluxeMap(pStage);
			//}
		}
	}

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_depthFill
 * @param[in] stage
 *
 *unused
*/
static void Render_depthFill(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];
	//vec4_t        ambientColor;
	rgbaGen_t     rgbaGen;
	qboolean use_vertex_skinning  = (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning);
	qboolean use_vertex_animation = (glState.vertexAttribsInterpolation > 0);
	qboolean use_alphaTesting     = ((pStage->stateBits & GLS_ATEST_BITS) != 0);

	Ren_LogComment("--- Render_depthFill ---\n");

	rgbaGen = getRgbaGenForColorModulation(pStage, tess.lightmapNum);

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_genericShader,
								USE_ALPHA_TESTING, use_alphaTesting,
								USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
								USE_VERTEX_SKINNING, use_vertex_skinning,
								USE_VERTEX_ANIMATION, use_vertex_animation,
								USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
								USE_TCGEN_ENVIRONMENT, pStage->tcGen_Environment,
								USE_TCGEN_LIGHTMAP, pStage->tcGen_Lightmap);

	GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, rgbaGen.color, rgbaGen.alpha);

	// USE_ALPHA_TESTING
	if (use_alphaTesting)
	{
		GLSL_SetUniform_AlphaTest(pStage->stateBits);
	}

	// USE_VERTEX_SKINNING
	if (use_vertex_skinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// USE_VERTEX_ANIMATION
	if (use_vertex_animation)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	// USE_DEFORM_VERTEXES
	if (tess.surfaceShader->numDeforms)
	{
/*		uniform int   u_DeformGen;
		uniform vec4  u_DeformWave;         // [base amplitude phase freq]
		uniform vec3  u_DeformBulge;        // [width height speed]
		uniform float u_DeformSpread;
		uniform float u_Time;*/
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	// USE_PORTAL_CLIPPING
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// USE_TCGEN_ENVIRONMENT
	if (pStage->tcGen_Environment)
	{
		// calculate the environment texcoords in object space
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.viewOrigin);
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrix);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_shadowFill
 * @param[in] stage
 */
static void Render_shadowFill(int stage)
{
	shaderStage_t *pStage              = tess.surfaceStages[stage];
	uint32_t      stateBits            = pStage->stateBits;
	qboolean      use_alphaTesting     = (pStage->stateBits & GLS_ATEST_BITS) != 0;
	qboolean      use_vertexSkinning   = glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning;
	qboolean      use_vertexAnimation  = glState.vertexAttribsInterpolation > 0;
	qboolean      use_lightDirectional = backEnd.currentLight->l.rlType == RL_DIRECTIONAL;

	Ren_LogComment("--- Render_shadowFill ---\n");

	// remove blend modes
	stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
	GL_State(stateBits);

	SetMacrosAndSelectProgram(trProg.gl_shadowFillShader,
									USE_ALPHA_TESTING, use_alphaTesting,
									USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
									USE_VERTEX_SKINNING, use_vertexSkinning,
									USE_VERTEX_ANIMATION, use_vertexAnimation,
									USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
									LIGHT_DIRECTIONAL, use_lightDirectional);

	GLSL_SetRequiredVertexPointers(trProg.gl_shadowFillShader);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	if (!use_lightDirectional)
	{
		SetUniformVec3(UNIFORM_LIGHTORIGIN, backEnd.currentLight->origin);
		SetUniformFloat(UNIFORM_LIGHTRADIUS, backEnd.currentLight->sphereRadius);
	}

	if (use_alphaTesting)
	{
		GLSL_SetUniform_AlphaTest(pStage->stateBits);
		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrix);
	}

	if (use_vertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	if (use_vertexAnimation)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

/*
	if (r_debugShadowMaps->integer)
	{
		vec4_t shadowMapColor;
		Vector4Copy(g_color_table[backEnd.pc.c_batches % 8], shadowMapColor);
		SetUniformVec4(UNIFORM_COLOR, shadowMapColor);
	}
	else
	{
		SetUniformVec4(UNIFORM_COLOR, colorWhite);
	}
*/
	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_forwardLighting_DBS_omni
 * @param[in] diffuseStage
 * @param[in] attenuationXYStage
 * @param[in] attenuationZStage
 * @param[in] light
 */
static void Render_forwardLighting_DBS_omni(shaderStage_t *diffuseStage,
                                            shaderStage_t *attenuationXYStage,
                                            shaderStage_t *attenuationZStage, trRefLight_t *light)
{
	rgbaGen_t rgbaGen;
	qboolean use_vertexSkinning  = glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning;
	qboolean use_vertexAnimation = glState.vertexAttribsInterpolation > 0;
	qboolean use_shadowCompare   = r_shadows->integer >= SHADOWING_EVSM32 && !light->l.noShadows && light->shadowLOD >= 0;

if (tr.refdef.pixelTarget != NULL) {
	return;
}

	Ren_LogComment("--- Render_forwardLighting_DBS_omni ---\n");

	SetMacrosAndSelectProgram(trProg.gl_forwardLightingShader_omniXYZ,
								USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
								USE_VERTEX_SKINNING, use_vertexSkinning,
								USE_VERTEX_ANIMATION, use_vertexAnimation,
								USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
								USE_SHADOWING, use_shadowCompare);

	GLSL_SetRequiredVertexPointers(trProg.gl_forwardLightingShader_omniXYZ);

	// u_ColorModulate
	rgbaGen = getRgbaGenForColorModulation(diffuseStage, tess.lightmapNum);
	GLSL_SetUniform_ColorModulate(trProg.gl_forwardLightingShader_omniXYZ, rgbaGen.color, rgbaGen.alpha);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	SetUniformVec3(UNIFORM_LIGHTORIGIN, light->origin);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, tess.svars.color);
	SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
	SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);
	SetUniformMatrix16(UNIFORM_LIGHTATTENUATIONMATRIX, light->attenuationMatrix2);

	// USE_PORTAL_CLIPPING
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// USE_VERTEX_SKINNING
	if (use_vertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// USE_VERTEX_ANIMATION
	if (use_vertexAnimation)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	// USE_DEFORM_VERTEXES
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&diffuseStage->wrapAroundLightingExp, 0));
	}

	if (use_shadowCompare)
	{
		//float shadowTexelSize = 1.0f / shadowMapResolutions[light->shadowLOD];
		float shadowTexelSize = rcp((float)shadowMapResolutions[light->shadowLOD]);
		SetUniformFloat(UNIFORM_SHADOWTEXELSIZE, shadowTexelSize);
		SetUniformFloat(UNIFORM_SHADOWBLUR, r_shadowBlur->value);

		// bind u_ShadowMap
		SelectTexture(TEX_SHADOWMAP);
		GL_Bind(tr.shadowCubeFBOImage[light->shadowLOD]);
	}

	// bind u_AttenuationMapXY
	SelectTexture(TEX_ATTEXY);
	BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

	// bind u_AttenuationMapZ
	SelectTexture(TEX_ATTEZ);
	BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_forwardLighting_DBS_proj
 * @param[in] diffuseStage
 * @param[in] attenuationXYStage
 * @param[in] attenuationZStage
 * @param[in] light
 */
static void Render_forwardLighting_DBS_proj(shaderStage_t *diffuseStage,
                                            shaderStage_t *attenuationXYStage,
                                            shaderStage_t *attenuationZStage, trRefLight_t *light)
{
	rgbaGen_t rgbaGen;
	qboolean use_alphaTesting    = (diffuseStage->stateBits & GLS_ATEST_BITS) != 0;
	qboolean use_shadowCompare   = r_shadows->integer >= SHADOWING_EVSM32 && !light->l.noShadows && light->shadowLOD >= 0;
	qboolean use_vertexSkinning  = glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning;
	qboolean use_vertexAnimation = glState.vertexAttribsInterpolation > 0;
	qboolean use_normalMapping   = r_normalMapping->integer;
	qboolean use_parallaxMapping = use_normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax;
	qboolean use_specular        = use_normalMapping && (diffuseStage->type == ST_BUNDLE_DBS || diffuseStage->type == ST_BUNDLE_DBSR);

if (tr.refdef.pixelTarget != NULL) {
	return;
}

	Ren_LogComment("--- Render_fowardLighting_DBS_proj ---\n");

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_forwardLightingShader_projXYZ,
								USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
								USE_ALPHA_TESTING, use_alphaTesting,
								USE_VERTEX_SKINNING, use_vertexSkinning,
								USE_VERTEX_ANIMATION, use_vertexAnimation,
								USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
								USE_NORMAL_MAPPING, use_normalMapping,
								USE_PARALLAX_MAPPING, use_parallaxMapping,
								USE_SHADOWING, use_shadowCompare,
								USE_SPECULAR, use_specular);

	GLSL_SetRequiredVertexPointers(trProg.gl_forwardLightingShader_projXYZ);

	// u_ColorModulate
	rgbaGen = getRgbaGenForColorModulation(diffuseStage, tess.lightmapNum);
	GLSL_SetUniform_ColorModulate(trProg.gl_forwardLightingShader_projXYZ, rgbaGen.color, rgbaGen.alpha);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	SetUniformVec3(UNIFORM_LIGHTORIGIN, light->origin);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, tess.svars.color);
	SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
	SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);
	SetUniformMatrix16(UNIFORM_LIGHTATTENUATIONMATRIX, light->attenuationMatrix2);

	// USE_ALPHA_TESTING
	if (use_alphaTesting)
	{
		GLSL_SetUniform_AlphaTest(diffuseStage->stateBits);
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	if (use_vertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	if (use_vertexAnimation)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&diffuseStage->wrapAroundLightingExp, 0));
	}

	if (use_shadowCompare)
	{
		SetUniformFloat(UNIFORM_SHADOWBLUR, r_shadowBlur->value);
		SetUniformMatrix16ARR(UNIFORM_SHADOWMATRIX, light->shadowMatrices, MAX_SHADOWMAPS);

		//float shadowTexelSize = 1.0f / shadowMapResolutions[light->shadowLOD];
		float shadowTexelSize = rcp((float)shadowMapResolutions[light->shadowLOD]);
		SetUniformFloat(UNIFORM_SHADOWTEXELSIZE, shadowTexelSize);

		SelectTexture(TEX_SHADOWMAP);
		GL_Bind(tr.shadowMapFBOImage[light->shadowLOD]);
	}

	// bind u_RandomMap (not used - see forwardLighting_fp)
	//SelectTexture(TEX_RANDOM);
	//GL_Bind(tr.randomNormalsImage);

	// bind u_AttenuationMapXY
	SelectTexture(TEX_ATTEXY);
	BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

	// bind u_AttenuationMapZ
	SelectTexture(TEX_ATTEZ);
	BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrix);

	if (use_normalMapping)
	{
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);

		if (use_parallaxMapping)
		{
			SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&diffuseStage->depthScaleExp, r_parallaxDepthScale->value));
		}

		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		GL_Bind(diffuseStage->bundle[TB_NORMALMAP].image[0]);

		if (use_specular)
		{
			SetUniformFloat(UNIFORM_SPECULARSCALE, r_specularScaleWorld->value);
			SetUniformFloat(UNIFORM_SPECULAREXPONENT, r_specularExponentWorld->value);
			// bind u_SpecularMap
			SelectTexture(TEX_SPECULAR);
			GL_Bind(diffuseStage->bundle[TB_SPECULARMAP].image[0]);
		}
	}

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_forwardLighting_DBS_directional
 * @param[in] diffuseStage
 * @param attenuationXYStage - unused
 * @param attenuationZStage - unused
 * @param[in] light
 */
static void Render_forwardLighting_DBS_directional(shaderStage_t *diffuseStage,
                                                   shaderStage_t *attenuationXYStage,
                                                   shaderStage_t *attenuationZStage, trRefLight_t *light)
{
	vec3_t lightDirection;
	rgbaGen_t rgbaGen;
	qboolean use_shadowCompare   = r_shadows->integer >= SHADOWING_EVSM32 && !light->l.noShadows && light->shadowLOD >= 0;
	qboolean use_vertexSkinning = glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning;
	qboolean use_vertexAnimation = glState.vertexAttribsInterpolation > 0;

if (tr.refdef.pixelTarget != NULL) {
	return;
}

	Ren_LogComment("--- Render_forwardLighting_DBS_directional ---\n");

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_forwardLightingShader_directionalSun,
								USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
								USE_VERTEX_SKINNING, use_vertexSkinning,
								USE_VERTEX_ANIMATION, use_vertexAnimation,
								USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
								USE_SHADOWING, use_shadowCompare);

	// end choose right shader program ------------------------------

	GLSL_SetRequiredVertexPointers(trProg.gl_forwardLightingShader_directionalSun);

	// color
	rgbaGen = getRgbaGenForColorModulation(diffuseStage, tess.lightmapNum);
	GLSL_SetUniform_ColorModulate(trProg.gl_forwardLightingShader_directionalSun, rgbaGen.color, rgbaGen.alpha);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_VIEWMATRIX, backEnd.viewParms.world.viewMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

#if 1
	VectorCopy(tr.sunDirection, lightDirection);
#else
	VectorCopy(light->direction, lightDirection);
#endif
	SetUniformVec3(UNIFORM_LIGHTDIR, lightDirection);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, light->l.color); // tess.svars.color
	SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
	SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);// r_lightScale->value); // light->l.scale);   //!!!DEBUG!!! for quick 'n easy testing use the cvar value
	//SetUniformMatrix16(UNIFORM_LIGHTATTENUATIONMATRIX, light->attenuationMatrix2);

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	if (use_vertexSkinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	if (use_vertexAnimation)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&diffuseStage->wrapAroundLightingExp, 0));
	}

	if (use_shadowCompare)
	{
		SetUniformFloat(UNIFORM_SHADOWBLUR, r_shadowBlur->value);
		SetUniformMatrix16ARR(UNIFORM_SHADOWMATRIX, light->shadowMatricesBiased, MAX_SHADOWMAPS);
		SetUniformVec4(UNIFORM_SHADOWPARALLELSPLITDISTANCES, backEnd.viewParms.parallelSplitDistances);

		//float shadowTexelSize = 1.0f / sunShadowMapResolutions[light->shadowLOD];
		float shadowTexelSize = rcp((float)sunShadowMapResolutions[light->shadowLOD]);
		SetUniformFloat(UNIFORM_SHADOWTEXELSIZE, shadowTexelSize);

		if (!r_showParallelShadowSplits->integer)
		{
			SelectTexture(TEX_SHADOWMAP0);
			GL_Bind(tr.sunShadowMapFBOImage[0]);

			SelectTexture(TEX_SHADOWMAP1);
			GL_Bind(tr.sunShadowMapFBOImage[1]);

			SelectTexture(TEX_SHADOWMAP2);
			GL_Bind(tr.sunShadowMapFBOImage[2]);

			SelectTexture(TEX_SHADOWMAP3);
			GL_Bind(tr.sunShadowMapFBOImage[3]);

			SelectTexture(TEX_SHADOWMAP4);
			GL_Bind(tr.sunShadowMapFBOImage[4]);
		}
	}

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_reflection_CB
 * @param[in] stage
 *
 * This is only called for: ST_CUBEREFLECTIONS, ST_BUNDLE_CB
 */
static void Render_reflection_CB(int stage)
{
	if (!r_reflectionMapping->integer || tr.refdef.pixelTarget != NULL)
	{
		return;
	}

	shaderStage_t *pStage = tess.surfaceStages[stage];
	qboolean use_normalMapping    = (r_normalMapping->integer && pStage->type != ST_CUBEREFLECTIONS);
	qboolean use_vertex_skinning  = (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning);
	qboolean use_vertex_animation = (glState.vertexAttribsInterpolation > 0);

	Ren_LogComment("--- Render_reflection_CB ---\n");

	GL_State(pStage->stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_reflectionShader,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, use_vertex_skinning,
	                          USE_VERTEX_ANIMATION, use_vertex_animation,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, use_normalMapping);

	GLSL_SetRequiredVertexPointers(trProg.gl_reflectionShader);

	SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	SetUniformFloat(UNIFORM_REFLECTIONSCALE, r_reflectionScale->value);

	// USE_VERTEX_SKINNING
	if (use_vertex_skinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// USE_VERTEX_ANIMATION
	if (use_vertex_animation)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	// USE_PORTAL_CLIPPING
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

#if 1
	// bind 2 cubemaps, and interpolate between them (so you don't see reflections suddenly switch to the next cubemap)
	BindCubeMaps();
#else
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
#endif

	// bind u_NormalMap
	if (use_normalMapping)
	{
		SelectTexture(TEX_NORMAL);
		GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrix);
	}

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_refraction_C
 * @param[in] stage
 */
static void Render_refraction_C(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];

	Ren_LogComment("--- Render_refraction_C ---\n");

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_refractionShader, USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning);

	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_NORMAL);

	// set uniforms
	SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
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

/**
 * @brief Render_dispersion_C
 * @param[in] stage
 */
static void Render_dispersion_C(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];
	float         eta;
	float         etaDelta;

	Ren_LogComment("--- Render_dispersion_C ---\n");

	GL_State(pStage->stateBits);

	// enable shader, set arrays
	SetMacrosAndSelectProgram(trProg.gl_dispersionShader, USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning);

	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_NORMAL);

	// set uniforms
	eta      = RB_EvalExpression(&pStage->etaExp, (float)1.1);
	etaDelta = RB_EvalExpression(&pStage->etaDeltaExp, (float)-0.02);

	SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);  // in world space
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

	// bind u_ColorMap, which is a cubemap
	SelectTexture(TEX_COLOR);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_skybox
 * @param[in] stage
 */
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
		clipPortalPlane();
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);

	GLSL_SetRequiredVertexPointers(trProg.gl_skyboxShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_screen
 * @param[in] stage
 */
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

/**
 * @brief Render_portal
 * @param[in] stage
 */
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

/**
 * @brief Render_heatHaze
 * @param[in] stage
 */
static void Render_heatHaze(int stage)
{
	uint32_t      stateBits;
	float         deformMagnitude;
	shaderStage_t *pStage = tess.surfaceStages[stage];

	Ren_LogComment("--- Render_heatHaze ---\n");

	if (r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable)
	{
		FBO_t     *previousFBO;
		uint32_t  stateBits;
		rgbaGen_t rgbaGen;

		Ren_LogComment("--- HEATHAZE FIX BEGIN ---\n");

		rgbaGen = getRgbaGenForColorModulation(pStage, tess.lightmapNum);

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
		                          USE_ALPHA_TESTING, qfalse,  // (pStage->stateBits & GLS_ATEST_BITS) != 0
		                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
		                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
		                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
		                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
		                          USE_TCGEN_ENVIRONMENT, qfalse);
		// end choose right shader program ------------------------------

		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, rgbaGen.color, rgbaGen.alpha);
		SetUniformVec4(UNIFORM_COLOR, colorBlack);
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
			SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
		}

		if (backEnd.viewParms.isPortal)
		{
			clipPortalPlane();
		}
/*
if ((pStage->stateBits & GLS_ATEST_BITS) != 0)
{
	GLSL_SetUniform_AlphaTest(pStage->stateBits);
}
*/
		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);
		//gl_genericShader->SetUniform_ColorTextureMatrix(tess.svars.texMatrix); //!!!DEBUG!!! check this uncomment

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
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	// bind u_NormalMap
	SelectTexture(TEX_NORMAL);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
	SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrix);

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

/**
 * @brief Render_liquid
 * @param[in] stage
 *
 * This function is only called for stage.type: ST_LIQUIDMAP, ST_BUNDLE_WB, ST_BUNDLE_WDB
*/
static void Render_liquid(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];
	float fogDensity = RB_EvalExpression(&pStage->fogDensityExp, 0.0005f);  // 0.0005f as default?
	rgbaGen_t rgbaGen;
	qboolean use_diffuseMapping  = (pStage->type == ST_BUNDLE_WDB || pStage->type == ST_BUNDLE_WD);
	qboolean use_normalMapping   = r_normalMapping->integer && (pStage->type == ST_BUNDLE_WB || pStage->type == ST_BUNDLE_WDB);
	qboolean use_parallaxMapping = tess.surfaceShader->parallax && use_normalMapping && r_parallaxMapping->integer;
	// liquid has its own calculated specular, and doesn't need a specularmap.. maybe in the future
	qboolean use_reflections     = r_reflectionMapping->integer && use_normalMapping &&
									tr.cubeProbes.currentElements > 0 && tr.refdef.pixelTarget == NULL;
	// fancy water with moving waves.. only when the diffuse texture has some tcMod assigned, otherwise we would get still standing waves.
	qboolean use_water           = use_normalMapping && use_diffuseMapping && pStage->bundle[TB_DIFFUSEMAP].numTexMods;
	uint32_t attributebits;

	Ren_LogComment("--- Render_liquid ---\n");

	GL_State(pStage->stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_liquidShader,
								USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
								USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms, // && !ShaderRequiresCPUDeforms(tess.surfaceShader),
								USE_DIFFUSE, use_diffuseMapping,
								USE_NORMAL_MAPPING, use_normalMapping,
								USE_PARALLAX_MAPPING, use_parallaxMapping,
								USE_REFLECTIONS, use_reflections,
								USE_WATER, use_water);

	// vertex attributes
	attributebits = ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL | ATTR_COLOR;
	if (use_normalMapping)
	{
		attributebits |= (ATTR_TANGENT | ATTR_BINORMAL);
	}
	GLSL_VertexAttribsState(attributebits);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	//SetUniformFloat(UNIFORM_DIFFUSELIGHTING, r_diffuseLighting->value); // liquid uses a constant value..

	rgbaGen = getRgbaGenForColorModulation(pStage, tess.lightmapNum);
	GLSL_SetUniform_ColorModulate(trProg.gl_liquidShader, rgbaGen.color, rgbaGen.alpha);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	// deformVertexes
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	// portal plane
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}
/*if (backEnd.viewParms.isPortal)
{
	glDisable(GL_CLIP_PLANE0);
}*/

	// this is the fog displayed on the watersurface only (not any underwater fog, nor world fog)
	SetUniformFloat(UNIFORM_FOGDENSITY, fogDensity);
	if (fogDensity > 0.0)
	{
		SetUniformVec3(UNIFORM_FOGCOLOR, tess.svars.color);
		SetUniformMatrix16(UNIFORM_UNPROJECTMATRIX, backEnd.viewParms.unprojectionMatrix);

		// bind u_DepthMap
		SelectTexture(TEX_DEPTH);
		if (HDR_ENABLED())
		{
			GL_Bind(tr.depthRenderImage);
		}
		else
		{
			ImageCopyBackBuffer(tr.depthRenderImage); // depth texture is not bound to a FBO
		}
	}

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

	if (use_diffuseMapping)
	{
		// bind diffuse texture
		SelectTexture(TEX_DIFFUSE);
		GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
		SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrix);
	}

	if (use_normalMapping)
	{
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
			// light
		SetUniformVec3(UNIFORM_LIGHTDIR, tr.sunDirection);
		SetUniformVec3(UNIFORM_LIGHTCOLOR, tr.sunLight);

		// fresnel
		SetUniformFloat(UNIFORM_FRESNELBIAS, RB_EvalExpression(&pStage->fresnelBiasExp, 0.05f));
		SetUniformFloat(UNIFORM_FRESNELPOWER, RB_EvalExpression(&pStage->fresnelPowerExp, 2.0f));
		SetUniformFloat(UNIFORM_FRESNELSCALE, RB_EvalExpression(&pStage->fresnelScaleExp, 0.85f));
		SetUniformFloat(UNIFORM_NORMALSCALE, RB_EvalExpression(&pStage->normalScaleExp, 0.05f));

		// specular
		SetUniformFloat(UNIFORM_SPECULARSCALE, r_specularScaleWorld->value * 5.f); // water always more specular
		SetUniformFloat(UNIFORM_SPECULAREXPONENT, r_specularExponentWorld->value);

		// refraction
		//SetUniformFloat(UNIFORM_REFRACTIONINDEX, 1.0f / RB_EvalExpression(&pStage->refractionIndexExp, (1.0f/1.3f))); // 1/refractionIndex
		SetUniformFloat(UNIFORM_REFRACTIONINDEX, rcp(RB_EvalExpression(&pStage->refractionIndexExp, (1.0f / 1.3f)))); // 1/refractionIndex

		// reflection
#if 1 // if you change this #if-0, don't forget to change the liquid shaders also..
		if (use_reflections)
		{
//!			SetUniformFloat(UNIFORM_REFLECTIONSCALE, r_reflectionScale->value);
SetUniformFloat(UNIFORM_REFLECTIONSCALE, 1.0f);
			BindCubeMaps();
		}
		//else
		//	GL_Bind(tr.blackCubeImage);
#else
		// bind u_PortalMap
		// This is used to make the reflections on the water surface
		SelectTexture(TEX_PORTAL);
		GL_Bind(tr.portalRenderImage);
#endif

		// bumpmap
		SelectTexture(TEX_NORMAL);
		GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
		// no need for a normal-matrix, because the shader doesn't apply any tcMod on the bumpmap

		// parallax relief mapping
		if (use_parallaxMapping)
		{
			SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value));
			// parallax self shadowing
			SetUniformFloat(UNIFORM_PARALLAXSHADOW, r_parallaxShadow->value);
		}
	}

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_fog_brushes - used to render fog brushes
 */
static void Render_fog_brushes()
{
	fog_t  *fog;
	float  eyeT;
	vec3_t local;
	vec4_t fogDistanceVector, fogDepthVector;
	qboolean use_vertex_skinning  = glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning;
	qboolean use_vertex_animation = glState.vertexAttribsInterpolation > 0;

if (tr.refdef.pixelTarget != NULL) {
	return;
}

	Ren_LogComment("--- Render_fog_brushes ---\n");

	// we can disable any fogging with the cvar.
	if (r_noFog->integer)
	{
		return;
	}

//	if (tr.world->fogs + tess.fogNum < 1 || !tess.surfaceShader->fogPass) // this produces a compiler warning
	if (!tr.world->fogs || tess.fogNum < 1 || !tess.surfaceShader->fogPass)
	{
		return;
	}

	// no fog pass in snooper
	if ((tr.refdef.rdflags & RDF_SNOOPERVIEW) || tess.surfaceShader->noFog)
	{
		return;
	}

	// no world, no fogging
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	fog = tr.world->fogs + tess.fogNum;

	// use this only to render fog brushes (global fog has a brush number of -1)
	if (fog->originalBrushNumber < 0 && tess.surfaceShader->sort <= SS_OPAQUE)
	{
		return;
	}

	Ren_LogComment("--- Render_fog( fogNum = %i, originalBrushNumber = %i ) ---\n", tess.fogNum, fog->originalBrushNumber);

	// all fogging distance is based on world Z units
	fogDistanceVector[0] = -backEnd.orientation.modelViewMatrix[2];
	fogDistanceVector[1] = -backEnd.orientation.modelViewMatrix[6];
	fogDistanceVector[2] = -backEnd.orientation.modelViewMatrix[10];
	VectorSubtract(backEnd.orientation.origin, backEnd.viewParms.orientation.origin, local);
	//fogDistanceVector[3] = DotProduct(local, backEnd.viewParms.orientation.axis[0]);
	Dot(local, backEnd.viewParms.orientation.axis[0], fogDistanceVector[3]);

	// scale the fog vectors based on the fog's thickness
	/*fogDistanceVector[0] *= fog->tcScale;
	fogDistanceVector[1] *= fog->tcScale;
	fogDistanceVector[2] *= fog->tcScale;
	fogDistanceVector[3] *= fog->tcScale;*/
	Vector4Scale(fogDistanceVector, fog->tcScale, fogDistanceVector);

	// rotate the gradient vector for this orientation
	if (fog->hasSurface)
	{
		/*fogDepthVector[0] = fog->surface[0] * backEnd.orientation.axis[0][0] +
							fog->surface[1] * backEnd.orientation.axis[0][1] +
							fog->surface[2] * backEnd.orientation.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.orientation.axis[1][0] +
							fog->surface[1] * backEnd.orientation.axis[1][1] +
							fog->surface[2] * backEnd.orientation.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.orientation.axis[2][0] +
							fog->surface[1] * backEnd.orientation.axis[2][1] +
							fog->surface[2] * backEnd.orientation.axis[2][2];*/
		Dot(fog->surface, backEnd.orientation.axis[0], fogDepthVector[0]);
		Dot(fog->surface, backEnd.orientation.axis[1], fogDepthVector[1]);
		Dot(fog->surface, backEnd.orientation.axis[2], fogDepthVector[2]);

		//fogDepthVector[3] = -fog->surface[3] + DotProduct(backEnd.orientation.origin, fog->surface);
		Dot(backEnd.orientation.origin, fog->surface, fogDepthVector[3]);
		fogDepthVector[3] -= fog->surface[3];

		//eyeT = DotProduct(backEnd.orientation.viewOrigin, fogDepthVector) + fogDepthVector[3];
		Dot(backEnd.orientation.viewOrigin, fogDepthVector, eyeT);
		eyeT += fogDepthVector[3];
	}
	else
	{
		Vector4Set(fogDepthVector, 0.f, 0.f, 0.f, 1.f);
		eyeT = 1.f; // non-surface fog always has eye inside (viewpoint is outside when eyeT < 0)
	}

//	fogDistanceVector[3] += 0.001953125f; // 1.0 / 512.0; // is this optimized with current compiler settings?..  1/(depth*8)?
//!!!DEBUG!!! test: what is ^^that^^ doing?..

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
	                          USE_VERTEX_SKINNING, use_vertex_skinning,
	                          USE_VERTEX_ANIMATION, use_vertex_animation,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          EYE_OUTSIDE, eyeT < 0); // viewpoint is outside when eyeT < 0 - needed for clipping distance even for constant fog

	GLSL_SetRequiredVertexPointers(trProg.gl_fogQuake3Shader);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	SetUniformVec4(UNIFORM_COLOR, fog->color);
	SetUniformVec4(UNIFORM_FOGDISTANCEVECTOR, fogDistanceVector);
	SetUniformVec4(UNIFORM_FOGDEPTHVECTOR, fogDepthVector);

	// EYE_OUTSIDE
	if (eyeT < 0)
	{
		SetUniformFloat(UNIFORM_FOGEYET, eyeT);
	}

	// USE_VERTEX_SKINNING
	if (use_vertex_skinning)
	{
		SetUniformMatrix16ARR(UNIFORM_BONEMATRIX, tess.boneMatrices, MAX_BONES);
	}

	// USE_VERTEX_ANIMATION
	if (use_vertex_animation)
	{
		SetUniformFloat(UNIFORM_VERTEXINTERPOLATION, glState.vertexAttribsInterpolation);
	}

	// USE_DEFORM_VERTEXES
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	// USE_PORTAL_CLIPPING
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(tr.fogImage);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_volumetricFog
 * @note see Fog Polygon Volumes documentation by Nvidia for further information
 * https://developer.download.nvidia.com/SDK/9.5/Samples/DEMOS/Direct3D9/src/FogPolygonVolumes3/docs/FogPolygonVolumes3.pdf
 */
static void Render_volumetricFog()
{
	Ren_LogComment("--- Render_volumetricFog---\n");

	if (glConfig2.framebufferBlitAvailable)
	{
		FBO_t *previousFBO = glState.currentFBO;

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

		glVertexAttrib4fv(ATTR_INDEX_COLOR, tr.fogColor);

		// set uniforms
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
		SetUniformMatrix16(UNIFORM_UNPROJECTMATRIX, backEnd.viewParms.unprojectionMatrix);
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
		SetUniformFloat(UNIFORM_FOGDENSITY, tess.surfaceShader->fogParms.tcScale); // rcp(  .depthForOpaque));
		SetUniformVec3(UNIFORM_FOGCOLOR, tess.surfaceShader->fogParms.color);

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

/**
 * @brief Tess_ComputeColor
 * @param[in] pStage
 */
void Tess_ComputeColor(shaderStage_t *pStage)
{
	float     red;
	float     green;
	float     blue;
	rgbaGen_t rgbaGen;
	//vec4_t    v;

	rgbaGen = getRgbaGen(pStage, tess.lightmapNum);

	Ren_LogComment("--- Tess_ComputeColor ---\n");

	// rgbGen
	switch (rgbaGen.color)
	{
	case CGEN_IDENTITY:
	{
		/*tess.svars.color[0] = 1.0f;
		tess.svars.color[1] = 1.0f;
		tess.svars.color[2] = 1.0f;
		tess.svars.color[3] = 1.0f;*/
		//Vector4Set(tess.svars.color, 1.f, 1.f, 1.f, 1.f);
		Vector4Set4(tess.svars.color, 1.f);
		break;
	}
	case CGEN_VERTEX:
	case CGEN_ONE_MINUS_VERTEX:
	{
		/*tess.svars.color[0] = 0.0f;
		tess.svars.color[1] = 0.0f;
		tess.svars.color[2] = 0.0f;
		tess.svars.color[3] = 0.0f;*/
		//Vector4Set(tess.svars.color, 0.f, 0.f, 0.f, 0.f);
		Vector4Set4(tess.svars.color, 0.f);
		break;
	}
	default:
	case CGEN_IDENTITY_LIGHTING:
	{
		/*tess.svars.color[0] = tr.identityLight;
		tess.svars.color[1] = tr.identityLight;
		tess.svars.color[2] = tr.identityLight;
		tess.svars.color[3] = tr.identityLight;*/
		Vector4Set4(tess.svars.color, tr.identityLight);
		break;
	}
	case CGEN_CONST:
	{
		tess.svars.color[0] = (float)pStage->constantColor[0];
		tess.svars.color[1] = (float)pStage->constantColor[1];
		tess.svars.color[2] = (float)pStage->constantColor[2];
		tess.svars.color[3] = (float)pStage->constantColor[3];
		Vector4Scale(tess.svars.color, _1div255, tess.svars.color);
		break;
	}
	case CGEN_ENTITY:
	{
		if (backEnd.currentLight)
		{
			/*tess.svars.color[0] = Q_bound(0.0f, backEnd.currentLight->l.color[0], 1.0f);
			tess.svars.color[1] = Q_bound(0.0f, backEnd.currentLight->l.color[1], 1.0f);
			tess.svars.color[2] = Q_bound(0.0f, backEnd.currentLight->l.color[2], 1.0f);*/
			VectorBound(backEnd.currentLight->l.color, vec3_origin, vec3_1, tess.svars.color);
			tess.svars.color[3] = 1.0f;
		}
		else if (backEnd.currentEntity)
		{
			/*tess.svars.color[0] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[0] * _1div255, 1.0f);
			tess.svars.color[1] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[1] * _1div255, 1.0f);
			tess.svars.color[2] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[2] * _1div255, 1.0f);
			tess.svars.color[3] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[3] * _1div255, 1.0f);*/
			tess.svars.color[0] = (float)backEnd.currentEntity->e.shaderRGBA[0];
			tess.svars.color[1] = (float)backEnd.currentEntity->e.shaderRGBA[1];
			tess.svars.color[2] = (float)backEnd.currentEntity->e.shaderRGBA[2];
			tess.svars.color[3] = (float)backEnd.currentEntity->e.shaderRGBA[3];
			Vector4Scale(tess.svars.color, _1div255, tess.svars.color);
			Vector4Bound(tess.svars.color, vec4_origin, vec4_1, tess.svars.color);
		}
		else
		{
			/*tess.svars.color[0] = 1.0f;
			tess.svars.color[1] = 1.0f;
			tess.svars.color[2] = 1.0f;
			tess.svars.color[3] = 1.0f;*/
			Vector4Set4(tess.svars.color, 1.f);
		}
		break;
	}
	case CGEN_ONE_MINUS_ENTITY:
	{
		if (backEnd.currentLight)
		{
			/*tess.svars.color[0] = 1.0f - Q_bound(0.0f, backEnd.currentLight->l.color[0], 1.0f);
			tess.svars.color[1] = 1.0f - Q_bound(0.0f, backEnd.currentLight->l.color[1], 1.0f);
			tess.svars.color[2] = 1.0f - Q_bound(0.0f, backEnd.currentLight->l.color[2], 1.0f);*/
			VectorBound(backEnd.currentLight->l.color, vec3_origin, vec3_1, tess.svars.color);
			VectorSubtract(vec3_1, tess.svars.color, tess.svars.color);
			tess.svars.color[3] = 0.0f;      // FIXME
		}
		else if (backEnd.currentEntity)
		{
			/*tess.svars.color[0] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[0] * _1div255, 1.0f);
			tess.svars.color[1] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[1] * _1div255, 1.0f);
			tess.svars.color[2] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[2] * _1div255, 1.0f);
			tess.svars.color[3] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[3] * _1div255, 1.0f);*/
			tess.svars.color[0] = (float)backEnd.currentEntity->e.shaderRGBA[0];
			tess.svars.color[1] = (float)backEnd.currentEntity->e.shaderRGBA[1];
			tess.svars.color[2] = (float)backEnd.currentEntity->e.shaderRGBA[2];
			tess.svars.color[3] = (float)backEnd.currentEntity->e.shaderRGBA[3];
			Vector4Scale(tess.svars.color, _1div255, tess.svars.color);
			Vector4Bound(tess.svars.color, vec4_origin, vec4_1, tess.svars.color);
			Vector4Subtract(vec4_1, tess.svars.color, tess.svars.color);
		}
		else
		{
			/*tess.svars.color[0] = 0.0f;
			tess.svars.color[1] = 0.0f;
			tess.svars.color[2] = 0.0f;
			tess.svars.color[3] = 0.0f;*/
			Vector4Set4(tess.svars.color, 0.f);
		}
		break;
	}
	case CGEN_WAVEFORM:
	{
		float      glow;
		waveForm_t *wf = &pStage->rgbWave;

		if (wf->func == GF_NOISE)
		{
			glow = wf->base + R_NoiseGet4f(0, 0, 0, (tess.shaderTime + wf->phase) * wf->frequency) * wf->amplitude;
		}
		else
		{
			glow = RB_EvalWaveForm(wf) * tr.identityLight;
		}

		if (glow < 0.f)
		{
			glow = 0.f;
		}
		else if (glow > 1.f)
		{
			glow = 1.f;
		}

		/*tess.svars.color[0] = glow;
		tess.svars.color[1] = glow;
		tess.svars.color[2] = glow;
		tess.svars.color[3] = 1.0f;*/
		Vector4Set(tess.svars.color, glow, glow, glow, 1.f);
		break;
	}
	case CGEN_CUSTOM_RGB:
	{
		float rgb;

		rgb = Q_bound(0.0f, RB_EvalExpression(&pStage->rgbExp, 1.0f), 1.0f);

		/*tess.svars.color[0] = rgb;
		tess.svars.color[1] = rgb;
		tess.svars.color[2] = rgb;*/
		VectorSet(tess.svars.color, rgb, rgb, rgb);
		break;
	}
	case CGEN_CUSTOM_RGBs:
	{
		if (backEnd.currentLight)
		{
			red   = Q_bound(0.0f, RB_EvalExpression(&pStage->redExp, backEnd.currentLight->l.color[0]), 1.0f);
			green = Q_bound(0.0f, RB_EvalExpression(&pStage->greenExp, backEnd.currentLight->l.color[1]), 1.0f);
			blue  = Q_bound(0.0f, RB_EvalExpression(&pStage->blueExp, backEnd.currentLight->l.color[2]), 1.0f);
		}
		else if (backEnd.currentEntity)
		{
			red   = Q_bound(0.0f, RB_EvalExpression(&pStage->redExp, backEnd.currentEntity->e.shaderRGBA[0] * _1div255), 1.0f);
			green = Q_bound(0.0f, RB_EvalExpression(&pStage->greenExp, backEnd.currentEntity->e.shaderRGBA[1] * _1div255), 1.0f);
			blue  = Q_bound(0.0f, RB_EvalExpression(&pStage->blueExp, backEnd.currentEntity->e.shaderRGBA[2] * _1div255), 1.0f);
		}
		else
		{
			red   = Q_bound(0.0f, RB_EvalExpression(&pStage->redExp, 1.0f), 1.0f);
			green = Q_bound(0.0f, RB_EvalExpression(&pStage->greenExp, 1.0f), 1.0f);
			blue  = Q_bound(0.0f, RB_EvalExpression(&pStage->blueExp, 1.0f), 1.0f);
		}

		tess.svars.color[0] = red;
		tess.svars.color[1] = green;
		tess.svars.color[2] = blue;
		break;
	}
	}

	// alphaGen
	switch (rgbaGen.alpha)
	{
	default:
	case AGEN_IDENTITY:
	{
		if (pStage->rgbGen != CGEN_IDENTITY)
		{
			if ((pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1.f) || pStage->rgbGen != CGEN_VERTEX)
			{
				tess.svars.color[3] = 1.0f;
			}
		}
		break;
	}
	case AGEN_VERTEX:
	case AGEN_ONE_MINUS_VERTEX:
	{
		tess.svars.color[3] = 0.0f;
		break;
	}
	case AGEN_CONST:
	{
		if (pStage->rgbGen != CGEN_CONST)
		{
			tess.svars.color[3] = pStage->constantColor[3] * _1div255;
		}
		break;
	}
	case AGEN_ENTITY:
	{
		if (backEnd.currentLight)
		{
			tess.svars.color[3] = 1.0f;      // FIXME ?
		}
		else if (backEnd.currentEntity)
		{
			tess.svars.color[3] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[3] * _1div255, 1.0f);
		}
		else
		{
			tess.svars.color[3] = 1.0f;
		}
		break;
	}
	case AGEN_ONE_MINUS_ENTITY:
	{
		if (backEnd.currentLight)
		{
			tess.svars.color[3] = 0.0f;      // FIXME ?
		}
		else if (backEnd.currentEntity)
		{
			tess.svars.color[3] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[3] * _1div255, 1.0f);
		}
		else
		{
			tess.svars.color[3] = 0.0f;
		}
		break;
	}
	case AGEN_NORMALZFADE:
	{
		// FIXME: rework/move to GL
		float    alpha, range, lowest, highest, dot;
		vec3_t   worldUp;
		qboolean zombieEffect = qfalse;
		int      i;

		if (VectorCompare(backEnd.currentEntity->e.fireRiseDir, vec3_origin))
		{
			VectorSet(backEnd.currentEntity->e.fireRiseDir, 0.f, 0.f, 1.f);
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
		if (lowest == -1000.f)      // use entity alpha
		{
			lowest       = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		highest = pStage->zFadeBounds[1];
		if (highest == -1000.f)      // use entity alpha
		{
			highest      = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		range = highest - lowest;
		for (i = 0; i < tess.numVertexes; i++)
		{
			//dot = DotProduct(tess.normal[i].v, worldUp);
			//dot = DotProduct(tess.normals[i], worldUp);
VectorNormalize(worldUp);
			Dot(tess.normals[i], worldUp, dot);

			// special handling for Zombie fade effect
			if (zombieEffect)
			{
				alpha  = (float)backEnd.currentEntity->e.shaderRGBA[3] * (dot + 1.0f) / 2.0f;
				alpha += (2.0f * (float)backEnd.currentEntity->e.shaderRGBA[3]) * (1.0f - (dot + 1.0f) / 2.0f);
				if (alpha > 255.0f)
				{
					alpha = 255.0;
				}
				else if (alpha < 0.0f)
				{
					alpha = 0.0f;
				}
				tess.svars.color[3] = (byte) (alpha);
				continue;
			}

			if (dot <= highest)
			{
				if (dot >= lowest)
				{
					range *= 0.5f; // range /= 2
					if (dot < lowest + range)
					{
						alpha = ((float)pStage->constantColor[3] * ((dot - lowest) / range));
					}
					else
					{
//						alpha = ((float)pStage->constantColor[3] * (1.0f - ((dot - lowest - range) / range)));
alpha = ((float)pStage->constantColor[3] * ((dot - range) / range));
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
	}
	break;
	case AGEN_WAVEFORM:
	{
		float      glow;
		waveForm_t *wf = &pStage->alphaWave;

		glow = RB_EvalWaveFormClamped(wf);

		tess.svars.color[3] = glow;
		break;
	}
	case AGEN_CUSTOM:
	{
		tess.svars.color[3] = Q_bound(0.0f, RB_EvalExpression(&pStage->alphaExp, 1.0f), 1.0f); // = alpha;
		break;
	}
	}
}

/**
 * @brief Tess_ComputeTexMatrices
 * @param[in] pStage
 */
static void Tess_ComputeTexMatrices(shaderStage_t *pStage)
{
//	int   i;
	vec_t *matrix;

	Ren_LogComment("--- Tess_ComputeTexMatrices ---\n");
/*
	// This is the older code that supported tcMod for any stage (diffuse, and bump seperately, and sprecular...)
	for (i = 0; i < MAX_TEXTURE_BUNDLES; i++)
	{
		matrix = tess.svars.texMatrices[i];
		RB_CalcTexMatrix(&pStage->bundle[i], matrix);

		if (i == TB_COLORMAP && pStage->tcGen_Lightmap)
		{
			MatrixMultiplyScale(matrix, tr.fatLightmapStep, tr.fatLightmapStep, tr.fatLightmapStep);
		}
	}
*/
	// Now we only support tcMod for the diffuse stage, and all the texturemaps "follow" the diffusemap texture.
	// This makes it much easier for mappers to specify tcMod textures (textures that have their Texture Coordinates MODified).
	// In far most cases, a mapper wants the specularmap to stay in-sync with the diffuse, if that diffuse is moving around.. anyway.
	// The same for any other texturemap. A bumpmap specifies where the bumps are on a diffuse texture, for every pixel of that texture.
	// We therefore transform only the diffuse/colormap texture coordinates.
	// In the glsl shaders, when we need to read (say) a bumpmap, we simply use the (possibly) transformed diffuse texture coords.
	// We now only need to pass one diffuseMatrix to a shader, instead of multiple (bump,specular,reflection). performance++
	// However, we do lose some fancy functionality (not being able to make weird, extra ordinary, special materials that are rarely made).
	// The shaders benefit, of course, with less traffic to the GPU.
	matrix = tess.svars.texMatrix;
	RB_CalcTexMatrix(&pStage->bundle[TB_DIFFUSEMAP], matrix);
	if (pStage->tcGen_Lightmap)
	{
		MatrixMultiplyScale(matrix, tr.fatLightmapStep, tr.fatLightmapStep, tr.fatLightmapStep);
	}
}

/**
 * @brief Tess_StageIteratorDebug
 */
void Tess_StageIteratorDebug()
{
	Ren_LogComment("--- Tess_StageIteratorDebug( %i vertices, %i triangles ) ---\n", tess.numVertexes, tess.numIndexes / 3);

	GL_CheckErrors();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		Tess_UpdateVBOs(tess.attribsSet);
	}

	Tess_DrawElements();
}

/**
 * @brief RB_StencilOp
 * @param[in] op
 * @return
 */
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

/**
 * @brief RB_SetStencil
 * @param[in] side
 * @param[in] stencil
 */
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

/**
 * @brief Tess_StageIteratorGeneric
 */
void Tess_StageIteratorGeneric()
{
	int stage;
	qboolean has_lightmap;

	Ren_LogComment("--- Tess_StageIteratorGeneric( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.numVertexes, tess.numIndexes / 3);

	GL_CheckErrors();

	Tess_DeformGeometry();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		Tess_UpdateVBOs(tess.attribsSet);
	}

	if (tess.surfaceShader->fogVolume)
	{
		Render_volumetricFog();
		return;
	}

	// set face culling appropriately
	GL_Cull(tess.surfaceShader->cullType);

	// set polygon offset if necessary
	if (tess.surfaceShader->polygonOffset)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	has_lightmap = (tess.lightmapNum >= 0 && tr.lightmaps.currentElements) && (tess.lightmapNum < tr.lightmaps.currentElements);

	// call shader function
	// !! We know exactly howmany stages there are.. so don't loop through max_stages
	for (stage = 0; stage < tess.surfaceShader->numStages; stage++)
	{
		shaderStage_t *pStage = tess.surfaceStages[stage];

		if (!pStage)
		{
			// Once stages have been optimized, all stages fill the array in sequence (no gaps)..
			// ..so getting an invalid stage is unlikely to happen.
			break;
		}

		if (RB_EvalExpression(&pStage->ifExp, 1.0f) == 0.f)
		{
			continue;
		}

		Tess_ComputeColor(pStage);
		Tess_ComputeTexMatrices(pStage);

		if (pStage->frontStencil.flags || pStage->backStencil.flags)
		{
			RB_SetStencil(GL_FRONT, &pStage->frontStencil);
			RB_SetStencil(GL_BACK, &pStage->backStencil);
		}

		/* // Until there is functional code for this in any shader, this block is commented out.
		if (pStage->bundle[0].isTcGenVector == qtrue)
		{
			vec3_t vec;
			VectorCopy(pStage->bundle[0].tcGenVectors[0], vec);
			SetUniformVec3(UNIFORM_TCGEN0VECTOR0, vec);
			VectorCopy(pStage->bundle[0].tcGenVectors[1], vec);
			SetUniformVec3(UNIFORM_TCGEN0VECTOR1, vec);
		}
		*/

		switch (pStage->type)
		{
		case ST_DIFFUSEMAP:   // diffuse
		case ST_BUNDLE_DB:    // diffuse + bump
		case ST_BUNDLE_DBS:   // diffuse + bump + specular
		case ST_BUNDLE_DBSR:  // diffuse + bump + specular + reflectionmap
			{
				// check if we render the world or an entity
				qboolean isWorld = (backEnd.currentEntity == &tr.worldEntity);
				if (!isWorld)
				{
					// treat brushmodels as world
					model_t *pmodel = R_GetModelByHandle(backEnd.currentEntity->e.hModel);
					isWorld = (pmodel && pmodel->type == MOD_BSP && pmodel->bsp);
				}
				if (isWorld)
				{
					// if there is no ST_LIGHTMAP stage, but a lightmapNum is given: render as lightmapped.
					// if an ST_LIGHTMAP stage does exist in this shader, then now render this diffuse stage vertex lit.
					qboolean renderLightmap = (!tess.surfaceShader->has_lightmapStage) && has_lightmap;
					Render_world(stage, renderLightmap);
				}
				else // this is an entity
				{
					Render_entity(stage);
				}
			}
			break;
		case ST_LIGHTMAP:
			if (has_lightmap)
			{ // in Oasis this is the terrain lightmap
				Render_world(stage, qtrue); // because this stage is only the ST_LIGHTMAP, no normalmapping or more stuff is used for rendering.
			}
			else // LIGHTMAP_BY_VERTEX. Render only the vertex-colors.
			{    // Because we use the generic shader for this, which needs a color map, we pass a whiteImage
				pStage->bundle[TB_COLORMAP].image[0] = tr.whiteImage;
				Render_generic(stage);
			}
			break;
		case ST_COLORMAP:
			Render_generic(stage);
			break;
		case ST_LIQUIDMAP:  // liquid
		case ST_BUNDLE_WB:  // liquid/water + bump
		case ST_BUNDLE_WDB: // liquid/water + diffuse + bump
			Render_liquid(stage);
			break;
		case ST_SKYBOXMAP:
			Render_skybox(stage);
			break;
		case ST_TCGENENVMAP:
			Render_generic(stage); // will render the tcGen environment
			break;
		case ST_CUBEREFLECTIONS:
		case ST_BUNDLE_CB:
			Render_reflection_CB(stage); // will render the cubeProbe reflections
			break;
		case ST_REFRACTIONMAP:
			Render_refraction_C(stage);
			break;
		case ST_DISPERSIONMAP:
			Render_dispersion_C(stage);
			break;
		case ST_SCREENMAP:
			Render_screen(stage);
			break;
		case ST_PORTALMAP:
			Render_portal(stage);
			break;
		case ST_HEATHAZEMAP:
			Render_heatHaze(stage);
			break;
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

	// gl_fogQuake3Shader
	if (tess.fogNum >= 1 && tess.surfaceShader->fogPass)
	{
		Render_fog_brushes();
	}

	// reset polygon offset
	if (tess.surfaceShader->polygonOffset)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

/**
 * @brief Tess_StageIteratorDepthFill
 * @note Unused
void Tess_StageIteratorDepthFill()
{
	int stage;

	Ren_LogComment("--- Tess_StageIteratorDepthFill( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.numVertexes, tess.numIndexes / 3);

	GL_CheckErrors();

	Tess_DeformGeometry();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		Tess_UpdateVBOs(tess.attribsSet);
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

		if (RB_EvalExpression(&pStage->ifExp, 1.0f) == 0.f)
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
		case ST_BUNDLE_DB:
		case ST_BUNDLE_DBS:
		case ST_BUNDLE_DBSR:
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
*/

/**
 * @brief Tess_StageIteratorShadowFill
 */
void Tess_StageIteratorShadowFill()
{
	int stage;

	Ren_LogComment("--- Tess_StageIteratorShadowFill( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.numVertexes, tess.numIndexes / 3);

	GL_CheckErrors();

	Tess_DeformGeometry();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		Tess_UpdateVBOs(tess.attribsSet);
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
	for (stage = 0; stage < tess.surfaceShader->numStages; stage++)
	{
		shaderStage_t *pStage = tess.surfaceStages[stage];

		if (!pStage)
		{
			break;
		}

		if (RB_EvalExpression(&pStage->ifExp, 1.0f) == 0.f)
		{
			continue;
		}

		switch (pStage->type)
		{
		case ST_COLORMAP:
		{
			/*if (tess.surfaceShader->sort <= SS_OPAQUE)
			{
				Tess_ComputeTexMatrices(pStage);
				Render_shadowFill(stage);
			}
			break;*/
			if (tess.surfaceShader->sort > SS_OPAQUE) continue;//for      break;//switch
			// else drop down the next following case statements, and execute that code block..
			// That's how switch/case/break work.. if the break is absent.
		}
//		case ST_LIGHTMAP:
		case ST_DIFFUSEMAP:
		case ST_BUNDLE_DB:
		case ST_BUNDLE_DBS:
		case ST_BUNDLE_DBSR:
//		case ST_BUNDLE_CB:
//		case ST_BUNDLE_WDB: // liquid stages have no texture rendered,
//		case ST_BUNDLE_WB:  // so if you try to add a shadow to it, (by including this 'case'),
//		case ST_BUNDLE_WD:  // you don't see it, because this surface is "marked done", and rest stages skipped.
		{
			Tess_ComputeTexMatrices(pStage);
			Render_shadowFill(stage);
// if we rendered one shadow plane for this surface, we stop further processing.
// We do not want >1 times this effect.
goto done;
			break;
		}

		default:
			break;
		}
	}

done:
	// reset polygon offset
	glDisable(GL_POLYGON_OFFSET_FILL);
}

/**
 * @brief Tess_StageIteratorLighting
 */
void Tess_StageIteratorLighting()
{
	int           i, j;
	trRefLight_t  *light = backEnd.currentLight;
	shaderStage_t *attenuationXYStage;
	shaderStage_t *attenuationZStage;
	qboolean      renderStage;
	qboolean      diffuseStageTexMatrices;
//	qboolean      sunDone, omniDone;

	Ren_LogComment("--- Tess_StageIteratorLighting( %s, %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.lightShader->name, tess.numVertexes, tess.numIndexes / 3);

	if (!light)
	{
		return;
	}

	GL_CheckErrors();

	Tess_DeformGeometry();

	if (!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		Tess_UpdateVBOs(tess.attribsSet);
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
//			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
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

//	sunDone = qfalse; // we want the sun rendered only once per surface
//	omniDone = qfalse;
	// !! We know exactly howmany stages there are.. so don't loop through max_stages
	for (i = 0; i < tess.surfaceShader->numStages; i++)
	{
		shaderStage_t *diffuseStage = tess.surfaceStages[i];

		if (!diffuseStage)
		{
			break;
		}

		if (RB_EvalExpression(&diffuseStage->ifExp, 1.0f) == 0.f)
		{
			continue;
		}

		diffuseStageTexMatrices = qfalse;

		for (j = 1; j < tess.lightShader->numStages; j++)
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

			if (RB_EvalExpression(&attenuationXYStage->ifExp, 1.0f) == 0.f)
			{
				continue;
			}

			renderStage = qfalse;
			switch (diffuseStage->type)
			{
			//case ST_LIGHTMAP:
			//case ST_COLORMAP:
			//case ST_BUNDLE_WD:
			//case ST_BUNDLE_WB:
			//case ST_LIQUIDMAP:
			case ST_DIFFUSEMAP:
			case ST_BUNDLE_DB:
			case ST_BUNDLE_DBS:
			case ST_BUNDLE_DBSR:
				if (light->l.rlType == RL_OMNI)
				{
//					if (omniDone) goto done;
//					omniDone = qtrue;
					renderStage = qtrue;
				}
				else if (light->l.rlType == RL_DIRECTIONAL)
				{
					//if(!light->l.inverseShadows)
					{
//						if (sunDone) goto done;
//						sunDone = qtrue;
						renderStage = qtrue;
					}
				}
				else if (light->l.rlType == RL_PROJ)
				{
					if (!light->l.inverseShadows)
					{
						renderStage = qtrue;
					}
				}
				break;
			default:
				break;
			}

			// only calculate matrices and attenuation when we need to render
			if (renderStage)
			{
				// do we have the diffuseStage TexMatrices calculated?
				if (!diffuseStageTexMatrices)
				{
					Tess_ComputeTexMatrices(diffuseStage);
					diffuseStageTexMatrices = qtrue; // now we have..
				}
				Tess_ComputeColor(attenuationXYStage);
				R_ComputeFinalAttenuation(attenuationXYStage, light);
				// render the appropriate light
				if (light->l.rlType == RL_OMNI)
				{
					Render_forwardLighting_DBS_omni(diffuseStage, attenuationXYStage, attenuationZStage, light);
				}
				else if (light->l.rlType == RL_DIRECTIONAL)
				{
					Render_forwardLighting_DBS_directional(diffuseStage, attenuationXYStage, attenuationZStage, light);
				}
				else
				{
					Render_forwardLighting_DBS_proj(diffuseStage, attenuationXYStage, attenuationZStage, light);
				}
goto done;
			}
		}
	}
done:

	// reset polygon offset
	if (tess.surfaceShader->polygonOffset)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

/**
 * @brief Render tesselated data
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
	if (tess.xyz[SHADER_MAX_VERTEXES - 1][0] != 0.f)
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
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;
	tess.attribsSet          = 0;
	tess.multiDrawPrimitives = 0;

	Ren_LogComment("--- Tess_End ---\n");

	GL_CheckErrors();
}
