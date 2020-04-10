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

			glMultiDrawElements(GL_TRIANGLES, tess.multiDrawCounts, GL_INDEX_TYPE, (const GLvoid *) tess.multiDrawIndexes, tess.multiDrawPrimitives);

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
		GL_Bind(tr.whiteImage);
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
	cubemapProbe_t *cubeProbe1; // nearest
	cubemapProbe_t *cubeProbe2; // 2nd nearest
	float          distance1, distance2, interpolate = 1.0;
	image_t        *env0     = tr.autoCubeImage;
	image_t        *env1     = tr.autoCubeImage;
	qboolean       isEntity  = (backEnd.currentEntity && (backEnd.currentEntity != &tr.worldEntity));
	vec3_t         *position = (isEntity) ? &backEnd.currentEntity->e.origin : &backEnd.viewParms.orientation.origin;

	R_FindTwoNearestCubeMaps(*position, &cubeProbe1, &cubeProbe2, &distance1, &distance2);

	if (cubeProbe1 != NULL && cubeProbe2 != NULL)
	{ // the most likely case..
		env0        = cubeProbe1->cubemap;
		env1        = cubeProbe2->cubemap;
		interpolate = distance1 / (distance1 + distance2);
		//Ren_LogComment("cubeProbeNearestDistance = %f, cubeProbeSecondNearestDistance = %f, interpolation = %f\n", distance1, distance2, interpolate);
	}
	else if (cubeProbe1 != NULL && cubeProbe2 == NULL)
	{
		env0 = cubeProbe1->cubemap;
		env1 = cubeProbe1->cubemap;
	}
	else if (cubeProbe1 == NULL && cubeProbe2 != NULL)
	{
		env0 = cubeProbe2->cubemap;
		env1 = cubeProbe2->cubemap;
	}

	// bind u_EnvironmentMap0
	SelectTexture(TEX_ENVMAP0);
	GL_Bind(env0);

	// bind u_EnvironmentMap1
	SelectTexture(TEX_ENVMAP1);
	GL_Bind(env1);

	// u_EnvironmentInterpolation
	SetUniformFloat(UNIFORM_ENVIRONMENTINTERPOLATION, interpolate);
}

bspGridPoint_t *LightgridColor(const vec3_t position)
{
	vec3_t lightOrigin;
	int    pos[3];
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
		v      = lightOrigin[i] * tr.world->lightGridInverseSize[i];
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
	gridPoint   = tr.world->lightGridData + pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];
	return gridPoint;
}

/**
 * @brief Binds a texture or the given default if image is NULL
 *        This is basically used to ensure a texture for GL_Bind
 * @param[in]
 * @param[in]
 */
static void BindTexture(image_t *image, image_t *defaultImage)
{
	if (!image)
	{
		image = defaultImage;
	}
	GL_Bind(image);
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
	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrices[TB_COLORMAP]);

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
void Tess_Begin(void (*stageIteratorFunc)(void),
                void (*stageIteratorFunc2)(void),
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

typedef struct rgbaGen_s
{
	colorGen_t color;
	alphaGen_t alpha;
} rgbaGen_t;

/*
 * @brief getRgbaGen
 *
 * @param pStage
 * @param lightmapNum

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
 *
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
		SetUniformVec3(UNIFORM_LIGHTDIR, tr.sunDirection); // i need to provide _some_ direction.. not all world is lit by the sun..
		if (setLightColor)
		{    //sun is too bright, we'll use tess.svars.color instead
			//SetUniformVec3(UNIFORM_LIGHTCOLOR, tr.sunLight); // the sun again..
			SetUniformVec3(UNIFORM_LIGHTCOLOR, tess.svars.color);
		}
	}
}

/**
 * @brief Render_generic
 * @param[in] stage
 */
static void Render_generic(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];


	Ren_LogComment("--- Render_generic ---\n");

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
		// origin for object, vieworigin for camera here we need to use object for reflections
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);
	}

	// u_AlphaTest
	GLSL_SetUniform_AlphaTest(pStage->stateBits);



	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, pStage->rgbGen, pStage->alphaGen);
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
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	BindAnimatedImage(&pStage->bundle[TB_COLORMAP]);
	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrices[TB_COLORMAP]);

	GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_vertexLighting_DBS_entity
 * @param[in] stage
 */
static void Render_vertexLighting_DBS_entity(int stage)
{
	shaderStage_t *pStage             = tess.surfaceStages[stage];
	qboolean      use_parallaxMapping = (r_normalMapping->integer && r_parallaxMapping->integer && tess.surfaceShader->parallax);
	qboolean      use_specular        = (r_normalMapping->integer && qtrue);
	qboolean      use_reflections     = (r_normalMapping->integer && r_reflectionMapping->integer && tr.cubeProbes.currentElements > 0 && !tr.refdef.pixelTarget);
	// !tr.refdef.pixelTarget to prevent using reflections before buildcubemaps() has finished. This is anti eye-cancer..
	// TODO: && when surface has NOT assigned tcGen environment (because that will execute a reflection_cb stage).
	qboolean use_vertex_skinning  = (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning);
	qboolean use_vertex_animation = (glState.vertexAttribsInterpolation > 0);

	Ren_LogComment("--- Render_vertexLighting_DBS_entity ---\n");

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_vertexLightingShader_DBS_entity,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, (pStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_VERTEX_SKINNING, use_vertex_skinning,
	                          USE_VERTEX_ANIMATION, use_vertex_animation,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,   // && !ShaderRequiresCPUDeforms(tess.surfaceShader),
	                          USE_NORMAL_MAPPING, r_normalMapping->integer,
	                          USE_PARALLAX_MAPPING, use_parallaxMapping,
	                          USE_REFLECTIONS, use_reflections,
	                          USE_SPECULAR, use_specular);

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

	GLSL_SetUniform_AlphaTest(pStage->stateBits);


	SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space


	SetUniformMatrix16(UNIFORM_MODELMATRIX, MODEL_MATRIX); // same as backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);


	// if ents miss ambient color, use tess
	if (backEnd.currentEntity->ambientLight[0] == 0.f && backEnd.currentEntity->ambientLight[1] == 0.f && backEnd.currentEntity->ambientLight[2] == 0.f)
	{
		//lets use the ambient color in map
		SetUniformVec3(UNIFORM_AMBIENTCOLOR, tr.worldEntity.ambientLight);

	}
	else
	{
		SetUniformVec3(UNIFORM_AMBIENTCOLOR, backEnd.currentEntity->ambientLight);
	}

	// if we miss lightdir from ents, use sun
	if (backEnd.currentEntity->lightDir[0] == 0.f && backEnd.currentEntity->lightDir[1] == 0.f && backEnd.currentEntity->lightDir[2] == 0.f)
	{
		SetUniformVec3(UNIFORM_LIGHTDIR, tr.sunDirection);
	}
	else
	{
		SetUniformVec3(UNIFORM_LIGHTDIR, backEnd.currentEntity->lightDir);
	}

	// if we miss directed light, use tess
	if (backEnd.currentEntity->directedLight[0] == 0.f && backEnd.currentEntity->directedLight[1] == 0.f && backEnd.currentEntity->directedLight[2] == 0.f)
	{
		SetUniformVec3(UNIFORM_LIGHTCOLOR, tess.svars.color);
	}
	else
	{
		SetUniformVec3(UNIFORM_LIGHTCOLOR, backEnd.currentEntity->directedLight);
	}



	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&pStage->wrapAroundLightingExp, 0));
	}

	if (use_parallaxMapping)
	{
		SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value));
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	if (r_normalMapping->integer)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		BindTexture(pStage->bundle[TB_NORMALMAP].image[0], tr.flatImage);
		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		if (use_reflections || use_specular)
		{
			// bind u_SpecularMap
			SelectTexture(TEX_SPECULAR);
			BindTexture(pStage->bundle[TB_SPECULARMAP].image[0], tr.blackImage);
			SetUniformMatrix16(UNIFORM_SPECULARTEXTUREMATRIX, tess.svars.texMatrices[TB_SPECULARMAP]);

			if (use_reflections)
			{
				BindCubeMaps();
			}
		}

	}
	GLSL_SetRequiredVertexPointers(trProg.gl_vertexLightingShader_DBS_entity);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_vertexLighting_DBS_world
 * @param[in] stage
 */
static void Render_vertexLighting_DBS_world(int stage)
{
	shaderStage_t *pStage             = tess.surfaceStages[stage];
	qboolean      use_parallaxMapping = (r_normalMapping->integer && r_parallaxMapping->integer && tess.surfaceShader->parallax);
	qboolean      use_specular        = (qboolean)r_normalMapping->integer;
	qboolean      use_reflections     = (r_normalMapping->integer && r_reflectionMapping->integer && tr.cubeProbes.currentElements > 0 && !tr.refdef.pixelTarget);

	Ren_LogComment("--- Render_vertexLighting_DBS_world ---\n");

	GL_State(pStage->stateBits);

	SetMacrosAndSelectProgram(trProg.gl_vertexLightingShader_DBS_world,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, (pStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms, // && !ShaderRequiresCPUDeforms(tess.surfaceShader),
	                          USE_NORMAL_MAPPING, r_normalMapping->integer,
	                          USE_PARALLAX_MAPPING, use_parallaxMapping,
	                          USE_REFLECTIONS, use_reflections,
	                          USE_SPECULAR, use_specular);

	GL_CheckErrors();

	// set uniforms

	// u_DeformGen
	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}


	GLSL_SetUniform_ColorModulate(trProg.gl_vertexLightingShader_DBS_world, pStage->rgbGen, pStage->alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	GLSL_SetUniform_AlphaTest(pStage->stateBits);

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	SetUniformMatrix16(UNIFORM_MODELMATRIX, MODEL_MATRIX); // same as backEnd.orientation.transformMatrix

	if (r_normalMapping->integer)
	{
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
	}
	// SetUniformVec3(UNIFORM_LIGHTDIR, backEnd.currentEntity->lightDir);
	// SetUniformVec3(UNIFORM_LIGHTCOLOR, backEnd.currentEntity->directedLight);

	/*SetUniformVec3(UNIFORM_LIGHTDIR, backEnd.currentLight->direction);
	vec4_t color;
	color[0] = backEnd.currentLight->l.color[0];
	color[1] = backEnd.currentLight->l.color[1];
	color[2] = backEnd.currentLight->l.color[2];
	color[3] = 1.0;
	SetUniformVec3(UNIFORM_LIGHTCOLOR, color);*/

	SetLightUniforms(qtrue);

	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&pStage->wrapAroundLightingExp, 0));
	}

	if (use_parallaxMapping)
	{
		SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value));
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	if (r_normalMapping->integer)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		BindTexture(pStage->bundle[TB_NORMALMAP].image[0], tr.flatImage);
		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		if (use_reflections || use_specular)
		{
			// bind u_SpecularMap
			SelectTexture(TEX_SPECULAR);
			BindTexture(pStage->bundle[TB_SPECULARMAP].image[0], tr.blackImage);
			SetUniformMatrix16(UNIFORM_SPECULARTEXTUREMATRIX, tess.svars.texMatrices[TB_SPECULARMAP]);

			if (use_reflections)
			{
				BindCubeMaps();
			}
		}
	}

	GLSL_SetRequiredVertexPointers(trProg.gl_vertexLightingShader_DBS_world);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_lightMapping
 * @param[in] stage
 * @param[in] asColorMap
 * @param[in] normalMapping
 */
static void Render_lightMapping(int stage, qboolean asColorMap, qboolean normalMapping)
{
	shaderStage_t *pStage             = tess.surfaceStages[stage];
	uint32_t      stateBits           = pStage->stateBits;
	qboolean      use_parallaxMapping = (normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax);
	//qboolean use_deluxeMapping = (normalMapping && qfalse); // r_showDeluxeMaps->integer == 1 AND a deluxemap exists!    because there is no code that does anything with deluxemaps, we disable it..
	qboolean use_specular    = normalMapping;
	qboolean use_reflections = (normalMapping && r_reflectionMapping->integer && tr.cubeProbes.currentElements > 0 && !tr.refdef.pixelTarget);
	// TODO: and when surface has something environment mappy assigned..
	// !tr.refdef.pixelTarget to prevent using reflections before buildcubemaps() has finished. This is anti eye-cancer..

	Ren_LogComment("--- Render_lightMapping ---\n");

	if (r_showLightMaps->integer)
	{
		stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_ATEST_BITS);
	}

	GL_State(stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_lightMappingShader,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, pStage->stateBits & GLS_ATEST_BITS,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,   // && !ShaderRequiresCPUDeforms(tess.surfaceShader),
	                          USE_NORMAL_MAPPING, normalMapping,
	                          USE_PARALLAX_MAPPING, use_parallaxMapping,
//								USE_DELUXE_MAPPING, use_deluxeMapping,
	                          USE_REFLECTIONS, use_reflections,
	                          USE_SPECULAR, use_specular);

	if (tess.surfaceShader->numDeforms)
	{
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	// set uniforms
	if (normalMapping)
	{
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
	}

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	GLSL_SetUniform_AlphaTest(pStage->stateBits);


	GLSL_SetUniform_ColorModulate(trProg.gl_lightMappingShader, pStage->rgbGen, pStage->alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	if (use_parallaxMapping)
	{
		SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value));
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// ..
	SetUniformBoolean(UNIFORM_B_SHOW_LIGHTMAP, (r_showLightMaps->integer == 1 ? GL_TRUE : GL_FALSE));
	//SetUniformBoolean(UNIFORM_B_SHOW_DELUXEMAP, (r_showDeluxeMaps->integer == 1 ? GL_TRUE : GL_FALSE));

	//SetUniformVec3(UNIFORM_LIGHTDIR, backEnd.currentEntity->lightDir);
	SetUniformVec3(UNIFORM_LIGHTDIR, tr.sunDirection);
	//SetUniformVec3(UNIFORM_LIGHTCOLOR, tr.sunLight);
	//sun too bright lets use tess instead
	SetUniformVec3(UNIFORM_LIGHTCOLOR, tess.svars.color);

	SelectTexture(TEX_DIFFUSE);
	image_t *image = pStage->bundle[TB_DIFFUSEMAP].image[0];

	if (image)
	{
		GL_Bind(image);
		SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);
	}
	else
	{
		GL_Bind(tr.whiteImage);
		SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, matrixIdentity);
	}

	if (normalMapping)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		BindTexture(pStage->bundle[TB_NORMALMAP].image[0], tr.flatImage);
		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		if (use_reflections || use_specular)
		{
			// bind u_SpecularMap
			SelectTexture(TEX_SPECULAR);
			BindTexture(pStage->bundle[TB_SPECULARMAP].image[0], tr.blackImage);
			SetUniformMatrix16(UNIFORM_SPECULARTEXTUREMATRIX, tess.svars.texMatrices[TB_SPECULARMAP]);

			if (use_reflections)
			{
				BindCubeMaps();
			}
		}
		//if (use_deluxeMapping) {
		//SelectTexture(TEX_DELUXE);
		//BindDeluxeMap(pStage);
		//}
	}


	if (asColorMap)
	{
		SelectTexture(TEX_LIGHTMAP);

		BindLightMap(); // bind lightMap
	}


	GLSL_SetRequiredVertexPointers(trProg.gl_lightMappingShader);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_depthFill
 * @param[in] stage
 *
 *unused
static void Render_depthFill(int stage)
{
    shaderStage_t *pStage = tess.surfaceStages[stage];
    vec4_t        ambientColor;
    rgbaGen_t     rgbaGen;

    Ren_LogComment("--- Render_depthFill ---\n");

    rgbaGen = getRgbaGenForColorModulation(pStage, tess.lightmapNum);

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
        SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.viewOrigin);
    }

    GLSL_SetUniform_AlphaTest(pStage->stateBits);
    GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, rgbaGen.color, rgbaGen.alpha);

    // u_Color
    if (r_precomputedLighting->integer)
    {
        VectorCopy(backEnd.currentEntity->ambientLight, ambientColor);
        ClampColor(ambientColor);
    }

    else
    {
        VectorClear(ambientColor);
    }
    ambientColor[3] = 1;

    // FIXME? see u_AmbientColor in depthFill glsl
    SetUniformVec4(UNIFORM_COLOR, ambientColor);
    //SetUniformVec3(UNIFORM_AMBIENTCOLOR, backEnd.currentEntity->ambientLight);

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

    if (backEnd.viewParms.isPortal)
    {
        clipPortalPlane();
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
*/

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
		BindTexture(pStage->bundle[TB_COLORMAP].image[0], tr.whiteImage);
		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, tess.svars.texMatrices[TB_COLORMAP]);
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
	float    shadowTexelSize;
	qboolean shadowCompare;


	Ren_LogComment("--- Render_forwardLighting_DBS_omni ---\n");

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
	                          USE_NORMAL_MAPPING, r_normalMapping->integer,
	                          USE_PARALLAX_MAPPING, r_normalMapping->integer && r_parallaxMapping->integer && tess.surfaceShader->parallax,
	                          USE_SHADOWING, shadowCompare);
	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate

	GLSL_SetUniform_ColorModulate(trProg.gl_forwardLightingShader_omniXYZ, diffuseStage->rgbGen, diffuseStage->alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	if (r_parallaxMapping->integer)
	{
		SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&diffuseStage->depthScaleExp, r_parallaxDepthScale->value));
	}

	if (shadowCompare)
	{
		shadowTexelSize = 1.0f / shadowMapResolutions[light->shadowLOD];
	}
	else
	{
		shadowTexelSize = 1.0f;
	}

	if (r_normalMapping->integer)
	{
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);
	}
	SetUniformVec3(UNIFORM_LIGHTORIGIN, light->origin);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, tess.svars.color);
	SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
	SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);
	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&diffuseStage->wrapAroundLightingExp, 0));
	}

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
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	GL_CheckErrors();

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	// FIXME: don't bind testures for r_DebugShadowMaps
	//r_shadows->integer == SHADOWING_EVSM32 || r_shadows->integer == SHADOWING_ESM16 || r_shadows->integer == SHADOWING_ESM32
	//case r_DebugShadowMaps && (EVSM || ESM)

	if (r_normalMapping->integer)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		BindTexture(diffuseStage->bundle[TB_NORMALMAP].image[0], tr.flatImage);
		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		// bind u_SpecularMap
		SelectTexture(TEX_SPECULAR);

		BindTexture(diffuseStage->bundle[TB_SPECULARMAP].image[0], tr.blackImage);

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

	// bind u_RandomMap (not used - see forwardLighting_fp)
	//SelectTexture(TEX_RANDOM);
	//GL_Bind(tr.randomNormalsImage);

	GLSL_SetRequiredVertexPointers(trProg.gl_forwardLightingShader_omniXYZ);

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
	float    shadowTexelSize;
	qboolean shadowCompare = qfalse;

	Ren_LogComment("--- Render_fowardLighting_DBS_proj ---\n");

	if (r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0)
	{
		shadowCompare = qtrue;
	}

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_forwardLightingShader_projXYZ,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_ALPHA_TESTING, (diffuseStage->stateBits & GLS_ATEST_BITS) != 0,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, r_normalMapping->integer,
	                          USE_PARALLAX_MAPPING, r_normalMapping->integer && r_parallaxMapping->integer && tess.surfaceShader->parallax,
	                          USE_SHADOWING, shadowCompare);
	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate


	GLSL_SetUniform_ColorModulate(trProg.gl_forwardLightingShader_projXYZ, diffuseStage->rgbGen, diffuseStage->alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	if (r_parallaxMapping->integer)
	{
		SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&diffuseStage->depthScaleExp, r_parallaxDepthScale->value));
	}

	// set uniforms

	if (shadowCompare)
	{
		shadowTexelSize = 1.0f / shadowMapResolutions[light->shadowLOD];
	}
	else
	{
		shadowTexelSize = 1.0f;
	}

	if (r_normalMapping->integer)
	{
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);
	}
	SetUniformVec3(UNIFORM_LIGHTORIGIN, light->origin);
	SetUniformVec3(UNIFORM_LIGHTCOLOR, tess.svars.color);
	SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
	SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);
	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&diffuseStage->wrapAroundLightingExp, 0));
	}
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
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	GL_CheckErrors();

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	if (r_normalMapping->integer)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		BindTexture(diffuseStage->bundle[TB_NORMALMAP].image[0], tr.flatImage);
		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		// bind u_SpecularMap
		SelectTexture(TEX_SPECULAR);

		BindTexture(diffuseStage->bundle[TB_SPECULARMAP].image[0], tr.blackImage);


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

	// bind u_RandomMap (not used - see forwardLighting_fp)
	//SelectTexture(TEX_RANDOM);
	//GL_Bind(tr.randomNormalsImage);

	GLSL_SetRequiredVertexPointers(trProg.gl_forwardLightingShader_projXYZ);

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
	float    shadowTexelSize;
	qboolean shadowCompare = qfalse;


	Ren_LogComment("--- Render_forwardLighting_DBS_directional ---\n");

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
	                          USE_NORMAL_MAPPING, r_normalMapping->integer,
	                          USE_PARALLAX_MAPPING, r_normalMapping->integer && r_parallaxMapping->integer && tess.surfaceShader->parallax,
	                          USE_SHADOWING, shadowCompare);

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate

	GLSL_SetUniform_ColorModulate(trProg.gl_forwardLightingShader_directionalSun, diffuseStage->rgbGen, diffuseStage->alphaGen);
	SetUniformVec4(UNIFORM_COLOR, tess.svars.color);

	if (r_parallaxMapping->integer)
	{
		SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&diffuseStage->depthScaleExp, r_parallaxDepthScale->value));
	}

	// set uniforms

#if 1
	SetLightUniforms(qfalse);
#else
	SetUniformVec3(UNIFORM_LIGHTDIR, light->direction);
#endif

	if (shadowCompare)
	{
		shadowTexelSize = 1.0f / sunShadowMapResolutions[light->shadowLOD];
	}
	else
	{
		shadowTexelSize = 1.0f;
	}

	if (r_normalMapping->integer)
	{
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);
	}

	SetUniformVec3(UNIFORM_LIGHTCOLOR, tess.svars.color);
	SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
	SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);
	if (r_wrapAroundLighting->integer)
	{
		SetUniformFloat(UNIFORM_LIGHTWRAPAROUND, RB_EvalExpression(&diffuseStage->wrapAroundLightingExp, 0));
	}
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
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	GL_CheckErrors();

	// bind u_DiffuseMap
	SelectTexture(TEX_DIFFUSE);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
	SetUniformMatrix16(UNIFORM_DIFFUSETEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]);

	if (r_normalMapping->integer)
	{
		// bind u_NormalMap
		SelectTexture(TEX_NORMAL);
		BindTexture(diffuseStage->bundle[TB_NORMALMAP].image[0], tr.flatImage);
		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);

		// bind u_SpecularMap
		SelectTexture(TEX_SPECULAR);

		BindTexture(diffuseStage->bundle[TB_SPECULARMAP].image[0], tr.blackImage);

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
}

/**
 * @brief Render_reflection_CB
 * @param[in] stage
 */
static void Render_reflection_CB(int stage)
{
	shaderStage_t *pStage = tess.surfaceStages[stage];

	Ren_LogComment("--- Render_reflection_CB ---\n");

	GL_State(pStage->stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_reflectionShader,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_VERTEX_SKINNING, glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning,
	                          USE_VERTEX_ANIMATION, glState.vertexAttribsInterpolation > 0,
	                          USE_DEFORM_VERTEXES, tess.surfaceShader->numDeforms,
	                          USE_NORMAL_MAPPING, r_normalMapping->integer);

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

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	if (r_reflectionMapping->integer)
	{
		// bind 2 cubemaps, and interpolate between them (so you don't see reflections suddenly switch to the next cubemap)
		BindCubeMaps();
	}
	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	if (r_reflectionMapping->integer)
	{
		if (backEnd.currentEntity && (backEnd.currentEntity != &tr.worldEntity))
		{
			GL_BindNearestCubeMap(backEnd.currentEntity->e.origin);
		}
		else
		{
			GL_BindNearestCubeMap(backEnd.viewParms.orientation.origin);
		}
	}
	else
	{
		GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
	}

	// bind u_NormalMap
	if (r_normalMapping->integer)
	{
		SelectTexture(TEX_NORMAL);
		GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
		SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]);
	}

	GLSL_SetRequiredVertexPointers(trProg.gl_reflectionShader);

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

	// bind u_ColorMap
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

		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, pStage->rgbGen, pStage->alphaGen);
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
			SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
		}

		if (backEnd.viewParms.isPortal)
		{
			clipPortalPlane();
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
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
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

/**
 * @brief Render_liquid
 * @param[in] stage
 */
static void Render_liquid(int stage)
{
	shaderStage_t *pStage             = tess.surfaceStages[stage];
	qboolean      use_parallaxMapping = (r_normalMapping->integer && r_parallaxMapping->integer && tess.surfaceShader->parallax);
	qboolean      use_specular        = (r_normalMapping->integer && qtrue);
	qboolean      use_reflections     = (r_normalMapping->integer && r_reflectionMapping->integer && tr.cubeProbes.currentElements > 0);
	qboolean      use_water           = r_normalMapping->integer && (pStage->bundle[TB_DIFFUSEMAP].texMods[0].type != TMOD_NONE); // fancy water with moving waves.. only when the diffuse texture has some tcMod assigned
	float         fogDensity;

	fogDensity = RB_EvalExpression(&pStage->fogDensityExp, 0.0005f);

	Ren_LogComment("--- Render_liquid ---\n");

	GL_State(pStage->stateBits);

	// choose right shader program ----------------------------------
	SetMacrosAndSelectProgram(trProg.gl_liquidShader,
	                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
	                          USE_NORMAL_MAPPING, r_normalMapping->integer,
	                          USE_PARALLAX_MAPPING, use_parallaxMapping,
	                          USE_SPECULAR, use_specular,
	                          USE_REFLECTIONS, use_reflections,
	                          USE_WATER, use_water);

	if (r_normalMapping->integer)
	{
		GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL | ATTR_COLOR);
	}
	else
	{
		GLSL_VertexAttribsState(ATTR_POSITION | ATTR_NORMAL | ATTR_COLOR);
	}

	// set uniforms
	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space

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

	if (r_normalMapping->integer)
	{
		// light direction
#if 1
		SetLightUniforms(qfalse);
#else
		SetUniformVec3(UNIFORM_LIGHTDIR, light->direction);
#endif

		// fresnel
		SetUniformFloat(UNIFORM_FRESNELBIAS, RB_EvalExpression(&pStage->fresnelBiasExp, 0.05f));
		SetUniformFloat(UNIFORM_FRESNELPOWER, RB_EvalExpression(&pStage->fresnelPowerExp, 2.0f));
		SetUniformFloat(UNIFORM_FRESNELSCALE, RB_EvalExpression(&pStage->fresnelScaleExp, 0.85f));
		SetUniformFloat(UNIFORM_NORMALSCALE, RB_EvalExpression(&pStage->normalScaleExp, 0.05f));

		// refraction
		SetUniformFloat(UNIFORM_REFRACTIONINDEX, 1.0 / RB_EvalExpression(&pStage->refractionIndexExp, 1.3f)); // 1/refractionIndex

		// reflection
#if 1
		if (use_reflections)
		{
			BindCubeMaps();
		}
		//else
		//	GL_Bind(tr.blackCubeImage);
#else
		// bind u_PortalMap
		// This is used to make the reflections on the water surface
		SelectTexture(TEX_PORTAL);
		GL_Bind(tr.portalRenderImage); // check this texture.. it's woot now
#endif

		if (use_parallaxMapping)
		{
			SetUniformFloat(UNIFORM_DEPTHSCALE, RB_EvalExpression(&pStage->depthScaleExp, r_parallaxDepthScale->value));
		}
	}

	// portal plane
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
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

	// bind u_NormalMap
	if (r_normalMapping->integer)
	{
		SelectTexture(TEX_NORMAL);
		BindTexture(pStage->bundle[TB_NORMALMAP].image[0], tr.flatImage);

		if (use_water)
		{
			SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_DIFFUSEMAP]); // make the bumps move along with the diffuse texture (in case tcMod changes something)
		}
		else
		{
			SetUniformMatrix16(UNIFORM_NORMALTEXTUREMATRIX, tess.svars.texMatrices[TB_NORMALMAP]); // a static bump
		}
	}

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_fog_brushes - used to render fog brushes (not the fog)
 *
 * @note This is required for any 'global' fog (see RB_RenderGlobalFog)
 *       If the brush limit (see comment below) is deactivated it will also render
 *       the 'volumetric' part of r_wolffog
 *	     In other words:
 *	     - gl_fogQuake3Shader does the r_wolffog fog 'wall' and fog brushes
 *	     - gl_fogGlobalShader does the global volumetric fog (with static density see st.t = 0.6; in glsl)
 *
 *	     Why this mix is done?
 *	     Currently it's the best solution until we've figured out which old fog code (and cvar r_wolffog) to drop
 *	     The old transition between volumetric fog and fog 'wall' was always looking bad
 *	     gl_fogGlobalShader has got a more fluent transition which is looking much better and there is no real need
 *	     for the fog 'wall'.
 *	     It's recommended to disable r_wolffog - it just doing the ugly wall
 *
 *	     side note: gl_volumetricFogShader might do the job for gl_fogGlobalShader
 *
 * @fixme Sort out the trouble
 */
static void Render_fog_brushes()
{
	fog_t  *fog;
	float  eyeT;
	vec3_t local;
	vec4_t fogDistanceVector, fogDepthVector;
	// offset fog surface
	//bspModel_t *model; // get fog stuff
	//vec4_t fogSurface;

	Ren_LogComment("--- Render_fog_brushes ---\n");

	if (!r_wolfFog->integer)
	{
		return;
	}

	//if (tr.world->fogs + tess.fogNum < 1 || !tess.surfaceShader->fogPass)
	//{
	//	return;
	//}

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
	// offset fog surface
	//model = tr.world->models + fog->modelNum;

	// use this only to render fog brushes (global fog has a brush number of -1)
	// disable this to get r_wolffog 'volumetric' fog back but also
	// disable RB_RenderGlobalFog which does 'volumetric' fog in r2 - we don't need fog twice
	if (fog->originalBrushNumber < 0 && tess.surfaceShader->sort <= SS_OPAQUE)
	{
		return;
	}

	// offset fog surface
	//VectorCopy(fog->surface, fogSurface);
	//fogSurface[3] = fog->surface[3] + DotProduct(fogSurface, model->orientation.origin);

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
		// offset fog surface
		//fogDepthVector[0] = fogSurface[0] * backEnd.orientation.axis[0][0] +
		//					fogSurface[1] * backEnd.orientation.axis[0][1] + fogSurface[2] * backEnd.orientation.axis[0][2];
		//fogDepthVector[1] = fogSurface[0] * backEnd.orientation.axis[1][0] +
		//					fogSurface[1] * backEnd.orientation.axis[1][1] + fogSurface[2] * backEnd.orientation.axis[1][2];
		//fogDepthVector[2] = fogSurface[0] * backEnd.orientation.axis[2][0] +
		//					fogSurface[1] * backEnd.orientation.axis[2][1] + fogSurface[2] * backEnd.orientation.axis[2][2];
		//fogDepthVector[3] = -fogSurface[3] + DotProduct(backEnd.orientation.origin, fogSurface);

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
		SetUniformFloat(UNIFORM_TIME, tess.shaderTime);
	}

	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(tr.fogImage);

	GLSL_SetRequiredVertexPointers(trProg.gl_fogQuake3Shader);

	Tess_DrawElements();

	GL_CheckErrors();
}

/**
 * @brief Render_volumetricFog
 * @note see Fog Polygon Volumes documentation by Nvidia for further information
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
		SetUniformFloat(UNIFORM_FOGDENSITY, tess.surfaceShader->fogParms.density);
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
	float red;
	float green;
	float blue;




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
		tess.svars.color[0] = pStage->constantColor[0] * (1.0f / 255.0f);
		tess.svars.color[1] = pStage->constantColor[1] * (1.0f / 255.0f);
		tess.svars.color[2] = pStage->constantColor[2] * (1.0f / 255.0f);
		tess.svars.color[3] = pStage->constantColor[3] * (1.0f / 255.0f);
		break;
	}
	case CGEN_ENTITY:
	{
		if (backEnd.currentLight)
		{
			tess.svars.color[0] = Q_bound(0.0f, backEnd.currentLight->l.color[0], 1.0f);
			tess.svars.color[1] = Q_bound(0.0f, backEnd.currentLight->l.color[1], 1.0f);
			tess.svars.color[2] = Q_bound(0.0f, backEnd.currentLight->l.color[2], 1.0f);
			tess.svars.color[3] = 1.0;
		}
		else if (backEnd.currentEntity)
		{
			tess.svars.color[0] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[0] * (1.0f / 255.0f), 1.0f);
			tess.svars.color[1] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[1] * (1.0f / 255.0f), 1.0f);
			tess.svars.color[2] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[2] * (1.0f / 255.0f), 1.0f);
			tess.svars.color[3] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[3] * (1.0f / 255.0f), 1.0f);
		}
		else
		{
			tess.svars.color[0] = 1.0f;
			tess.svars.color[1] = 1.0f;
			tess.svars.color[2] = 1.0f;
			tess.svars.color[3] = 1.0f;
		}
		break;
	}
	case CGEN_ONE_MINUS_ENTITY:
	{
		if (backEnd.currentLight)
		{
			tess.svars.color[0] = 1.0f - Q_bound(0.0f, backEnd.currentLight->l.color[0], 1.0f);
			tess.svars.color[1] = 1.0f - Q_bound(0.0f, backEnd.currentLight->l.color[1], 1.0f);
			tess.svars.color[2] = 1.0f - Q_bound(0.0f, backEnd.currentLight->l.color[2], 1.0f);
			tess.svars.color[3] = 0.0f;      // FIXME
		}
		else if (backEnd.currentEntity)
		{
			tess.svars.color[0] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[0] * (1.0f / 255.0f), 1.0f);
			tess.svars.color[1] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[1] * (1.0f / 255.0f), 1.0f);
			tess.svars.color[2] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[2] * (1.0f / 255.0f), 1.0f);
			tess.svars.color[3] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[3] * (1.0f / 255.0f), 1.0f);
		}
		else
		{
			tess.svars.color[0] = 0.0f;
			tess.svars.color[1] = 0.0f;
			tess.svars.color[2] = 0.0f;
			tess.svars.color[3] = 0.0f;
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
		tess.svars.color[3] = 1.0f;
		break;
	}
	case CGEN_CUSTOM_RGB:
	{
		float rgb;

		rgb = Q_bound(0.0f, RB_EvalExpression(&pStage->rgbExp, 1.0f), 1.0f);

		tess.svars.color[0] = rgb;
		tess.svars.color[1] = rgb;
		tess.svars.color[2] = rgb;
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
			red =
				Q_bound(0.0f, RB_EvalExpression(&pStage->redExp, backEnd.currentEntity->e.shaderRGBA[0] * (1.0f / 255.0)), 1.0f);
			green =
				Q_bound(0.0f, RB_EvalExpression(&pStage->greenExp, backEnd.currentEntity->e.shaderRGBA[1] * (1.0f / 255.0f)),
				        1.0f);
			blue =
				Q_bound(0.0f, RB_EvalExpression(&pStage->blueExp, backEnd.currentEntity->e.shaderRGBA[2] * (1.0f / 255.0f)),
				        1.0f);
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
	switch (pStage->alphaGen)
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
			tess.svars.color[3] = pStage->constantColor[3] * (1.0f / 255.0f);
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
			tess.svars.color[3] = Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[3] * (1.0f / 255.0f), 1.0f);
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
			tess.svars.color[3] = 1.0f - Q_bound(0.0f, backEnd.currentEntity->e.shaderRGBA[3] * (1.0f / 255.0f), 1.0f);
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
	int i;


	Ren_LogComment("--- Tess_ComputeTexMatrices ---\n");

	// This is the older code that supported tcMod for any stage (diffuse, and bump seperately, and sprecular...)
	for (i = 0; i < MAX_TEXTURE_BUNDLES; i++)
	{

		RB_CalcTexMatrix(&pStage->bundle[i], tess.svars.texMatrices[i]);

		if (i == TB_COLORMAP && pStage->tcGen_Lightmap)
		{
			MatrixMultiplyScale(tess.svars.texMatrices[i], tr.fatLightmapStep, tr.fatLightmapStep, tr.fatLightmapStep);
		}
	}

}


/**
 * @brief Set the fog parameters for this pass
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

	//FIXME!! deprecated in opengl3
	// set GL fog
	//SetIteratorFog();

	// set face culling appropriately
	GL_Cull(tess.surfaceShader->cullType);

	// set polygon offset if necessary
	if (tess.surfaceShader->polygonOffset)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

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

		/* per stage fogging (detail textures)
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
		}*/

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
		//2d maps
		case ST_COLORMAP:
		//Envap maps
		case ST_TCGEN:
		{
			Render_generic(stage);
			break;
		}
		case ST_LIGHTMAP:
		{
			if (tess.lightmapNum >= 0 && tess.surfaceShader->has_lightmapStage)
			{
				// render the lightmap
				Render_lightMapping(stage, qtrue, qfalse); // no normalmapping.. it's added in a later stage
			}
			else
			{
				// LIGHTMAP_BY_VERTEX
				//if shader stage references a lightmap, but no lightmap is present
				// (vertex-approximated surfaces), then set cgen to vertex
				// Because we use the generic shader for this, which needs a color map, we pass a "dummy" whiteImage
				//this should be done here and rgbgen should not be changed elsewhere.. see oasis,radar and 1944_beach
				pStage->bundle[TB_COLORMAP].image[0] = tr.whiteImage;

				pStage->rgbGen = CGEN_VERTEX;
				//Render_lightMapping(stage, qtrue, qfalse);
				Render_generic(stage);
			}
			break;
		}

		case ST_DIFFUSEMAP:
		case ST_COLLAPSE_lighting_DB:
		case ST_COLLAPSE_lighting_DBS:
		{
			//if(tess.surfaceShader->sort <= SS_OPAQUE)
			if (r_vertexLighting->integer || r_precomputedLighting->integer)
			{
				// check if we render the world or an entity
				qboolean isWorld = (backEnd.currentEntity == &tr.worldEntity);

				if (!isWorld)
				{
					// treat brushmodels as world
					model_t *pmodel = R_GetModelByHandle(backEnd.currentEntity->e.hModel);
					isWorld = pmodel->bsp && r_worldInlineModels->integer;         // FIXME: remove r_worldInlineModels?
				}
				// vertex lighting superseeds precomputed lighting (lightmap rendering)
				if (r_vertexLighting->integer)
				{
					// render vertex lit
					if (isWorld)
					{
						Render_vertexLighting_DBS_world(stage);
					}
					else
					{
						Render_vertexLighting_DBS_entity(stage);
					}
				}
				else
				//if (r_precomputedLighting->integer)
				{
					// render lightmapped
					if (isWorld)
					{
						// if there is no ST_LIGHTMAP stage, but a lightmapNum is given: render as lightmapped.
						// if an ST_LIGHTMAP stage does exist in this shader, then now render as vertex lit.
						if (!tess.surfaceShader->has_lightmapStage &&
						    tess.lightmapNum >= 0) //&& tr.lightmaps.currentElements && tess.lightmapNum < tr.lightmaps.currentElements)
						{
							if (r_normalMapping->integer)
							{
								Render_lightMapping(stage, qtrue, qtrue); // normalmapped
							}
							else
							{
								Render_lightMapping(stage, qtrue, qfalse); //not normalmapped
							}
							break;
						}
						else
						{
							Render_vertexLighting_DBS_world(stage); // LIGHTMAP_BY_VERTEX
							//Render_generic(stage); // no bump/specular/reflection
							break;
						}
					}
					else if (!isWorld)
					{
						Render_vertexLighting_DBS_entity(stage); // this is an entity
						break;
					}
				}
			}
			else
			{
				//this should be more propper for the renderer
				Render_vertexLighting_DBS_world(stage);


			}
			break;
		}
		case ST_COLLAPSE_reflection_CB:
		case ST_REFLECTIONMAP:
		{
			Render_reflection_CB(stage);
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

		switch (pStage->type)
		{
		case ST_COLORMAP:
		{
			if (tess.surfaceShader->sort <= SS_OPAQUE)
			{
				Tess_ComputeTexMatrices(pStage);
				Render_shadowFill(stage);
			}
			break;
		}

		//case ST_LIGHTMAP:
		case ST_DIFFUSEMAP:
		case ST_COLLAPSE_lighting_DB:
		case ST_COLLAPSE_lighting_DBS:
		{
			Tess_ComputeTexMatrices(pStage);
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

	Ren_LogComment("--- Tess_StageIteratorLighting( %s, %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.lightShader->name, tess.numVertexes, tess.numIndexes / 3);

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

		if (RB_EvalExpression(&diffuseStage->ifExp, 1.0f) == 0.f)
		{
			continue;
		}

		diffuseStageTexMatrices = qfalse;

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

			if (RB_EvalExpression(&attenuationXYStage->ifExp, 1.0f) == 0.f)
			{
				continue;
			}


			renderStage = qfalse;

			switch (diffuseStage->type)
			{
			case ST_DIFFUSEMAP:
			case ST_COLLAPSE_lighting_DB:
			case ST_COLLAPSE_lighting_DBS:
				if (light->l.rlType == RL_OMNI)
				{
					renderStage = qtrue;
				}
				else if (light->l.rlType == RL_PROJ)
				{
					if (!light->l.inverseShadows)
					{
						renderStage = qtrue;
					}
				}
				else if (light->l.rlType == RL_DIRECTIONAL)
				{
					//if(!light->l.inverseShadows)
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
			}
		}
	}

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
