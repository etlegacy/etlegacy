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
 * @file renderer2/tr_backend.c
 */

#include "tr_local.h"
#include "../renderercommon/tr_public.h"

backEndData_t  *backEndData;
backEndState_t backEnd;

#define DRAWSCREENQUAD() Tess_InstantQuad(RB_GetScreenQuad())
#define DRAWVIEWQUAD() Tess_InstantQuad(backEnd.viewParms.viewportVerts)

/**
 * @brief A player has predicted a teleport, but hasn't arrived yet
 */
static void RB_Hyperspace(void)
{
	float c;

	if (!backEnd.isHyperspace)
	{
		// do initialization shit
	}

	c = (backEnd.refdef.time & 255) / 255.0f;
	GL_ClearColor(c, c, c, 1);
	GL_Clear(GL_COLOR_BUFFER_BIT);

	backEnd.isHyperspace = qtrue;
}

/**
 * @brief SetViewportAndScissor
 */
static void SetViewportAndScissor(void)
{
	GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

	// set the window clipping
	GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	           backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
}

/**
 * @brief RB_SafeState
 */
static void RB_SafeState(void)
{
	// HACK: bring OpenGL into a safe state or strange FBO update problems will occur
	GLSL_BindProgram(NULL);
	GL_State(GLS_DEFAULT);
	//GL_VertexAttribsState(ATTR_POSITION);

	GL_SelectTexture(0);
	GL_Bind(tr.whiteImage);
}

/**
 * @brief RB_SetGL2D
 */
static void RB_SetGL2D(void)
{
	mat4_t proj;

	Ren_LogComment("--- RB_SetGL2D ---\n");

	// disable offscreen rendering
	R_BindNullFBO();

	backEnd.projection2D = qtrue;

	// set 2D virtual screen size
	GL_Viewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	GL_Scissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);

	MatrixOrthogonalProjection(proj, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1);
	GL_LoadProjectionMatrix(proj);
	GL_LoadModelViewMatrix(matrixIdentity);

	GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	GL_Cull(CT_TWO_SIDED);

	// set time for 2D shaders
	backEnd.refdef.time      = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001;
}

/**
 * @brief RB_GetScreenQuad
 * @return
 */
static vec4_t *RB_GetScreenQuad(void)
{
	static vec4_t quad[4];

	Vector4Set(quad[0], 0, 0, 0, 1);
	Vector4Set(quad[1], glConfig.vidWidth, 0, 0, 1);
	Vector4Set(quad[2], glConfig.vidWidth, glConfig.vidHeight, 0, 1);
	Vector4Set(quad[3], 0, glConfig.vidHeight, 0, 1);

	return quad;
}

/**
 * @brief Set the model view projection matrix to match the ingame view
 */
void RB_SetViewMVPM(void)
{
	mat4_t ortho;

	MatrixOrthogonalProjection(ortho,
	                           backEnd.viewParms.viewportX, backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                           backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                           -99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	GL_LoadModelViewMatrix(matrixIdentity);
}

enum renderDrawSurfaces_e
{
	DRAWSURFACES_WORLD_ONLY,
	DRAWSURFACES_ENTITIES_ONLY,
	DRAWSURFACES_ALL
};

/**
 * @brief RB_RenderDrawSurfaces
 * @param[in] opaque
 * @param[in] drawSurfFilter
 */
static void RB_RenderDrawSurfaces(qboolean opaque, int drawSurfFilter)
{
	trRefEntity_t *entity, *oldEntity = NULL;
	shader_t      *shader, *oldShader = NULL;
	int           lightmapNum, oldLightmapNum = LIGHTMAP_NONE;
	int           fogNum, oldFogNum = -1;
	qboolean      depthRange = qfalse, oldDepthRange = qfalse;
	int           i;
	drawSurf_t    *drawSurf;
	double	      originalTime = backEnd.refdef.floatTime; // save original time for entity shader offsets

	Ren_LogComment("--- RB_RenderDrawSurfaces ---\n");

	// draw everything

	backEnd.currentLight = NULL;

	for (i = 0, drawSurf = backEnd.viewParms.drawSurfs; i < backEnd.viewParms.numDrawSurfs; i++, drawSurf++)
	{
		// update locals
		entity      = drawSurf->entity;
		shader      = drawSurf->shader;
		lightmapNum = drawSurf->lightmapNum;
		fogNum      = drawSurf->fogNum;

		switch (drawSurfFilter)
		{
		case DRAWSURFACES_WORLD_ONLY:
			if (entity != &tr.worldEntity)
			{
				continue;
			}
			break;
		case DRAWSURFACES_ENTITIES_ONLY:
			if (entity == &tr.worldEntity)
			{
				continue;
			}
			break;
		case DRAWSURFACES_ALL:
		default: // unknown filter
			break;
		}

		if (glConfig2.occlusionQueryBits && r_dynamicEntityOcclusionCulling->integer && !entity->occlusionQuerySamples)
		{
			continue;
		}

		if (opaque)
		{
			// skip all translucent surfaces that don't matter for this pass
			if (shader->sort > SS_OPAQUE)
			{
				break;
			}
		}
		else
		{
			// skip all opaque surfaces that don't matter for this pass
			if (shader->sort <= SS_OPAQUE)
			{
				continue;
			}
		}

		if (entity == oldEntity && shader == oldShader && lightmapNum == oldLightmapNum && fogNum == oldFogNum)
		{
			// fast path, same as previous sort
			rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
			continue;
		}

		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if (shader != oldShader || lightmapNum != oldLightmapNum || fogNum != oldFogNum || (entity != oldEntity && !shader->entityMergable))
		{
			if (oldShader != NULL)
			{
				Tess_End();
			}

			Tess_Begin(Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, lightmapNum, fogNum);

			oldShader      = shader;
			oldLightmapNum = lightmapNum;
			oldFogNum      = fogNum;
		}

		// change the modelview matrix if needed
		if (entity != oldEntity)
		{
			depthRange = qfalse;

			if (entity != &tr.worldEntity)
			{
				backEnd.currentEntity    = entity;
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.surfaceShader->timeOffset;

				// set up the transformation matrix
				R_RotateEntityForViewParms(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);

				if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.currentEntity    = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.orientation      = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.surfaceShader->timeOffset;
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			// change depthrange if needed
			if (oldDepthRange != depthRange)
			{
				if (depthRange)
				{
					glDepthRange(0, 0.3);
				}
				else
				{
					glDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}

			oldEntity = entity;
		}

		// add the triangles for this surface
		rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
	}

	// draw the contents of the last shader batch
	if (oldShader != NULL)
	{
		Tess_End();
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

	backEnd.refdef.floatTime = originalTime;

	if (depthRange)
	{
		glDepthRange(0, 1);
	}

	GL_CheckErrors();
}

/*
 * @brief RB_RenderOpaqueSurfacesIntoDepth
 * @note Unused
static void RB_RenderOpaqueSurfacesIntoDepth(qboolean onlyWorld)
{
    trRefEntity_t *entity, *oldEntity;
    shader_t      *shader, *oldShader;
    qboolean      depthRange, oldDepthRange;
    qboolean      alphaTest, oldAlphaTest;
    deformType_t  deformType, oldDeformType;
    int           i;
    drawSurf_t    *drawSurf;

    Ren_LogComment("--- RB_RenderOpaqueSurfacesIntoDepth ---\n");

    // draw everything
    oldEntity            = NULL;
    oldShader            = NULL;
    oldDepthRange        = depthRange = qfalse;
    oldAlphaTest         = alphaTest = qfalse;
    oldDeformType        = deformType = DEFORM_TYPE_NONE;
    backEnd.currentLight = NULL;

    for (i = 0, drawSurf = backEnd.viewParms.drawSurfs; i < backEnd.viewParms.numDrawSurfs; i++, drawSurf++)
    {
        // update locals
        entity    = drawSurf->entity;
        shader    = drawSurf->shader;
        alphaTest = shader->alphaTest;

//#if 0
//        if (onlyWorld && (entity != &tr.worldEntity))
//        {
//            continue;
//        }
//#endif

        // skip all translucent surfaces that don't matter for this pass
        if (shader->sort > SS_OPAQUE)
        {
            break;
        }

        if (shader->numDeforms)
        {
            deformType = ShaderRequiresCPUDeforms(shader) ? DEFORM_TYPE_CPU : DEFORM_TYPE_GPU;
        }
        else
        {
            deformType = DEFORM_TYPE_NONE;
        }

        // change the tess parameters if needed
        // a "entityMergable" shader is a shader that can have surfaces from seperate
        // entities merged into a single batch, like smoke and blood puff sprites
        //if(shader != oldShader || lightmapNum != oldLightmapNum || (entity != oldEntity && !shader->entityMergable))

        if (entity == oldEntity && (alphaTest ? shader == oldShader : alphaTest == oldAlphaTest) && deformType == oldDeformType)
        {
            // fast path, same as previous sort
            rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
            continue;
        }
        else
        {
            if (oldShader != NULL)
            {
                Tess_End();
            }

            Tess_Begin(Tess_StageIteratorDepthFill, NULL, shader, NULL, qtrue, qfalse, -1, 0);

            oldShader     = shader;
            oldAlphaTest  = alphaTest;
            oldDeformType = deformType;
        }

        // change the modelview matrix if needed
        if (entity != oldEntity)
        {
            depthRange = qfalse;

            if (entity != &tr.worldEntity)
            {
                backEnd.currentEntity = entity;

                // set up the transformation matrix
                R_RotateEntityForViewParms(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);

                if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
                {
                    // hack the depth range to prevent view model from poking into walls
                    depthRange = qtrue;
                }
            }
            else
            {
                backEnd.currentEntity = &tr.worldEntity;
                backEnd.orientation   = backEnd.viewParms.world;
            }

            GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

            // change depthrange if needed
            if (oldDepthRange != depthRange)
            {
                if (depthRange)
                {
                    glDepthRange(0, 0.3);
                }
                else
                {
                    glDepthRange(0, 1);
                }
                oldDepthRange = depthRange;
            }

            oldEntity = entity;
        }

        // add the triangles for this surface
        rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
    }

    // draw the contents of the last shader batch
    if (oldShader != NULL)
    {
        Tess_End();
    }

    // go back to the world modelview matrix
    GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
    if (depthRange)
    {
        glDepthRange(0, 1);
    }

    GL_CheckErrors();
}
*/

// *INDENT-OFF*
/**
 * @brief Render_lightVolume
 * @param[in] ia
 */
static void Render_lightVolume(interaction_t *ia)
{
	int           j;
	trRefLight_t  *light = ia->light;
	shader_t      *lightShader;
	shaderStage_t *attenuationXYStage;
	shaderStage_t *attenuationZStage;
	vec4_t        quadVerts[4];

	// set the window clipping
	GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	// set light scissor to reduce fillrate
	GL_Scissor(ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight);

	// set 2D virtual screen size
	GL_PushMatrix();
	RB_SetViewMVPM();

	switch (light->l.rlType)
	{
	case RL_PROJ:
	{
		mat4_reset_translate(light->attenuationMatrix, 0.5, 0.5, 0.0);        // bias
		MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 1.0f / Q_min(light->falloffLength, 1.0f));        // scale
		break;
	}
	case RL_OMNI:
	default:
	{
		mat4_reset_translate(light->attenuationMatrix, 0.5, 0.5, 0.5);    // bias
		MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);       // scale
		break;
	}
	}
	mat4_mult_self(light->attenuationMatrix, light->projectionMatrix); // light projection (frustum)
	mat4_mult_self(light->attenuationMatrix, light->viewMatrix);

	lightShader       = light->shader;
	attenuationZStage = lightShader->stages[0];

	// !! We know exactly howmany stages there are.. so don't loop through max_stages
	for (j = 1; j < lightShader->numStages; j++)
	{
		attenuationXYStage = lightShader->stages[j];

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

		Tess_ComputeColor(attenuationXYStage);
		R_ComputeFinalAttenuation(attenuationXYStage, light);

		if (light->l.rlType == RL_OMNI)
		{
			qboolean shadowCompare;

			Ren_LogComment("--- Render_lightVolume_omni ---\n");

			SetMacrosAndSelectProgram(trProg.gl_volumetricLightingShader);
			GL_Cull(CT_TWO_SIDED);
			GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

			shadowCompare = (r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0);

			SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space
			SetUniformVec3(UNIFORM_LIGHTORIGIN, light->origin);
			SetUniformVec3(UNIFORM_LIGHTCOLOR, tess.svars.color);
			SetUniformFloat(UNIFORM_LIGHTRADIUS, light->sphereRadius);
			SetUniformFloat(UNIFORM_LIGHTSCALE, light->l.scale);
			SetUniformMatrix16(UNIFORM_LIGHTATTENUATIONMATRIX, light->attenuationMatrix2);

			// FIXME  gl_volumetricLightingShader->SetUniform_ShadowMatrix(light->attenuationMatrix);

			SetUniformFloat(UNIFORM_SHADOWCOMPARE, shadowCompare);
			SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
			SetUniformMatrix16(UNIFORM_UNPROJECTMATRIX, backEnd.viewParms.unprojectionMatrix);

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

			// draw light scissor rectangle
			Vector4Set(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
			Vector4Set(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
			Vector4Set(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0,
			           1);
			Vector4Set(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
			Tess_InstantQuad(quadVerts);

			GL_CheckErrors();
		}
	}

	GL_PopMatrix();
}
// *INDENT-ON*

/**
 * @brief Helper function for parallel split shadow mapping
 * @param[in] lightViewProjectionMatrix
 * @param[in] ia
 * @param[in] iaCount
 * @param[in,out] bounds
 * @param[in] shadowCasters
 * @return
 */
static int MergeInteractionBounds(const mat4_t lightViewProjectionMatrix, interaction_t *ia, int iaCount, vec3_t bounds[2], qboolean shadowCasters)
{
	int           i;
	int           j;
	surfaceType_t *surface;
	vec4_t        point;
	vec4_t        transf;
	vec3_t        worldBounds[2];
	//vec3_t		viewBounds[2];
	//vec3_t		center;
	//float			radius;
	int       numCasters = 0;
	frustum_t frustum;
	cplane_t  *clipPlane;
	int       r;

	ClearBounds(bounds[0], bounds[1]);

	// calculate frustum planes using the modelview projection matrix
	R_SetupFrustum2(frustum, lightViewProjectionMatrix);

	while (iaCount < backEnd.viewParms.numInteractions)
	{
		surface = ia->surface;

		if (shadowCasters)
		{
			if (ia->type == IA_LIGHTONLY)
			{
				goto skipInteraction;
			}
		}
		else
		{
			// we only merge shadow receivers
			if (ia->type == IA_SHADOWONLY)
			{
				goto skipInteraction;
			}
		}

		if (*surface == SF_FACE || *surface == SF_GRID || *surface == SF_TRIANGLES)
		{
			srfGeneric_t *gen = (srfGeneric_t *) surface;

			VectorCopy(gen->bounds[0], worldBounds[0]);
			VectorCopy(gen->bounds[1], worldBounds[1]);
		}
		else if (*surface == SF_VBO_MESH)
		{
			srfVBOMesh_t *srf = (srfVBOMesh_t *) surface;

			//Ren_Print("merging vbo mesh bounds\n");

			VectorCopy(srf->bounds[0], worldBounds[0]);
			VectorCopy(srf->bounds[1], worldBounds[1]);
		}
		else // if (*surface == SF_MDV) & others
		{
			//Tess_AddCube(vec3_origin, entity->localBounds[0], entity->localBounds[1], lightColor);
			goto skipInteraction;
		}

#if 1
		// use the frustum planes to cut off shadow casters beyond the split frustum
		for (i = 0; i < 6; i++)
		{
			clipPlane = &frustum[i];

			r = BoxOnPlaneSide(worldBounds[0], worldBounds[1], clipPlane);
			if (r == 2)
			{
				goto skipInteraction;
			}
		}
#endif

		if (shadowCasters && ia->type != IA_LIGHTONLY)
		{
			numCasters++;
		}

#if 1
		for (j = 0; j < 8; j++)
		{
			point[0] = worldBounds[j & 1][0];
			point[1] = worldBounds[(j >> 1) & 1][1];
			point[2] = worldBounds[(j >> 2) & 1][2];
			point[3] = 1;

			mat4_transform_vec4(lightViewProjectionMatrix, point, transf);
			transf[0] /= transf[3];
			transf[1] /= transf[3];
			transf[2] /= transf[3];

			AddPointToBounds(transf, bounds[0], bounds[1]);
		}
#elif 0
		ClearBounds(viewBounds[0], viewBounds[1]);
		for (j = 0; j < 8; j++)
		{
			point[0] = worldBounds[j & 1][0];
			point[1] = worldBounds[(j >> 1) & 1][1];
			point[2] = worldBounds[(j >> 2) & 1][2];
			point[3] = 1;

			mat4_transform_vec4(lightViewProjectionMatrix, point, transf);
			transf[0] /= transf[3];
			transf[1] /= transf[3];
			transf[2] /= transf[3];

			AddPointToBounds(transf, viewBounds[0], viewBounds[1]);
		}

		// get sphere of AABB
		VectorAdd(viewBounds[0], viewBounds[1], center);
		VectorScale(center, 0.5, center);

		radius = RadiusFromBounds(viewBounds[0], viewBounds[1]);

		for (j = 0; j < 3; j++)
		{
			if ((transf[j] - radius) < bounds[0][j])
			{
				bounds[0][j] = transf[i] - radius;
			}

			if ((transf[j] + radius) > bounds[1][j])
			{
				bounds[1][j] = transf[i] + radius;
			}
		}
#else

		ClearBounds(viewBounds[0], viewBounds[1]);
		for (j = 0; j < 8; j++)
		{
			point[0] = worldBounds[j & 1][0];
			point[1] = worldBounds[(j >> 1) & 1][1];
			point[2] = worldBounds[(j >> 2) & 1][2];
			point[3] = 1;

			mat4_transform_vec4(lightViewProjectionMatrix, point, transf);
			//transf[0] /= transf[3];
			//transf[1] /= transf[3];
			//transf[2] /= transf[3];

			AddPointToBounds(transf, viewBounds[0], viewBounds[1]);
		}

		// get sphere of AABB
		VectorAdd(viewBounds[0], viewBounds[1], center);
		VectorScale(center, 0.5, center);

		//MatrixTransform4(lightViewProjectionMatrix, center, transf);
		//transf[0] /= transf[3];
		//transf[1] /= transf[3];
		//transf[2] /= transf[3];

		radius = RadiusFromBounds(viewBounds[0], viewBounds[1]);

		if ((transf[2] + radius) > bounds[1][2])
		{
			bounds[1][2] = transf[2] + radius;
		}
#endif

skipInteraction:
		if (!ia->next)
		{
			// this is the last interaction of the current light
			break;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	return numCasters;
}

/**
 * @brief RB_RenderInteractions
 */
static void RB_RenderInteractions()
{
	shader_t      *shader, *oldShader;
	trRefEntity_t *entity, *oldEntity;
	trRefLight_t  *light, *oldLight;
	interaction_t *ia;
	qboolean      depthRange, oldDepthRange;
	int           iaCount;
	surfaceType_t *surface;
	vec3_t        tmp;
	mat4_t        modelToLight;
	int           startTime = 0;

	Ren_LogComment("--- RB_RenderInteractions ---\n");

	R2_TIMING(RSPEEDS_SHADING_TIMES)
	{
		startTime = ri.Milliseconds();
	}

	// draw everything
	oldLight      = NULL;
	oldEntity     = NULL;
	oldShader     = NULL;
	oldDepthRange = qfalse;
	depthRange    = qfalse;

	// render interactions
	for (iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions; )
	{
		backEnd.currentLight  = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface               = ia->surface;
		shader                = ia->surfaceShader;

		if (glConfig2.occlusionQueryBits)
		{
			// skip all interactions of this light because it failed the occlusion query
			if (r_dynamicLightOcclusionCulling->integer && !ia->occlusionQuerySamples)
			{
				goto skipInteraction;
			}

			if (r_dynamicEntityOcclusionCulling->integer && !entity->occlusionQuerySamples)
			{
				goto skipInteraction;
			}
		}

		if (!shader || !shader->interactLight)
		{
			// skip this interaction because the surface shader has no ability to interact with light
			// this will save texcoords and matrix calculations
			goto skipInteraction;
		}

		if (ia->type == IA_SHADOWONLY)
		{
			// skip this interaction because the interaction is meant for shadowing only
			goto skipInteraction;
		}

		if (light != oldLight)
		{
			Ren_LogComment("----- Rendering new light -----\n");

			// set light scissor to reduce fillrate
			GL_Scissor(ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight);
		}

		// this should never happen in the first iteration
		if (light == oldLight && entity == oldEntity && shader == oldShader)
		{
			// fast path, same as previous
			rb_surfaceTable[*surface] (surface);
			goto nextInteraction;
		}

		// draw the contents of the last shader batch
		Tess_End();

		// begin a new batch
		Tess_Begin(Tess_StageIteratorLighting, NULL, shader, light->shader, qfalse, qfalse, LIGHTMAP_NONE, FOG_NONE);

		// change the modelview matrix if needed
		if (entity != oldEntity)
		{
			depthRange = qfalse;

			if (entity != &tr.worldEntity)
			{
				// set up the transformation matrix
				R_RotateEntityForViewParms(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);

				if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			// change depthrange if needed
			if (oldDepthRange != depthRange)
			{
				if (depthRange)
				{
					glDepthRange(0, 0.3);
				}
				else
				{
					glDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}
		}

		// change the attenuation matrix if needed
		if (light != oldLight || entity != oldEntity)
		{
			// transform light origin into model space for u_LightOrigin parameter
			if (entity != &tr.worldEntity)
			{
				VectorSubtract(light->origin, backEnd.orientation.origin, tmp);
				light->transformed[0] = DotProduct(tmp, backEnd.orientation.axis[0]);
				light->transformed[1] = DotProduct(tmp, backEnd.orientation.axis[1]);
				light->transformed[2] = DotProduct(tmp, backEnd.orientation.axis[2]);
			}
			else
			{
				VectorCopy(light->origin, light->transformed);
			}

			// build the attenuation matrix using the entity transform
			mat4_mult(light->viewMatrix, backEnd.orientation.transformMatrix, modelToLight);

			switch (light->l.rlType)
			{
			case RL_PROJ:
			{
				mat4_reset_translate(light->attenuationMatrix, 0.5, 0.5, 0.0);            // bias
				MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 1.0f / Q_min(light->falloffLength, 1.0f));            // scale
				break;
			}
			case RL_OMNI:
			default:
			{
				mat4_reset_translate(light->attenuationMatrix, 0.5, 0.5, 0.5);        // bias
				MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);           // scale
				break;
			}
			}
			mat4_mult_self(light->attenuationMatrix, light->projectionMatrix); // light projection (frustum)
			mat4_mult_self(light->attenuationMatrix, modelToLight);
		}

		// add the triangles for this surface
		rb_surfaceTable[*surface] (surface);

nextInteraction:

		// remember values
		oldLight  = light;
		oldEntity = entity;
		oldShader = shader;

skipInteraction:
		if (!ia->next)
		{
			// draw the contents of the last shader batch
			Tess_End();

			// draw the light volume if needed
			if (light->shader->volumetricLight)
			{
				Render_lightVolume(ia);
			}

			if (iaCount < (backEnd.viewParms.numInteractions - 1))
			{
				// jump to next interaction and continue
				ia++;
				iaCount++;
			}
			else
			{
				// increase last time to leave for loop
				iaCount++;
			}

			// force updates
			oldLight  = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	if (depthRange)
	{
		glDepthRange(0, 1);
	}

	// reset scissor
	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	           backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	GL_CheckErrors();

	R2_TIMING(RSPEEDS_SHADING_TIMES)
	{
		backEnd.pc.c_forwardLightingTime = ri.Milliseconds() - startTime;
	}
}

/**
 * @brief RB_RenderInteractionsShadowMapped
 */
static void RB_RenderInteractionsShadowMapped()
{
	shader_t      *shader, *oldShader;
	trRefEntity_t *entity, *oldEntity;
	trRefLight_t  *light, *oldLight;
	interaction_t *ia;
	int           iaCount;
	int           iaFirst;
	surfaceType_t *surface;
	qboolean      depthRange, oldDepthRange;
	qboolean      alphaTest, oldAlphaTest;
	deformType_t  deformType, oldDeformType;
	vec3_t        tmp;
	mat4_t        modelToLight;
	qboolean      drawShadows;
	int           cubeSide;
	int           splitFrustumIndex;
	int           startTime = 0;
	const mat4_t  bias      = { 0.5, 0.0, 0.0, 0.0,
		                        0.0,       0.5, 0.0, 0.0,
		                        0.0,       0.0, 0.5, 0.0,
		                        0.5,       0.5, 0.5, 1.0 };

	if (!glConfig2.framebufferObjectAvailable || !glConfig2.textureFloatAvailable)
	{
		RB_RenderInteractions();
		return;
	}

	Ren_LogComment("--- RB_RenderInteractionsShadowMapped ---\n");

	R2_TIMING(RSPEEDS_SHADING_TIMES)
	{
		startTime = ri.Milliseconds();
	}

	// draw everything
	oldLight          = NULL;
	oldEntity         = NULL;
	oldShader         = NULL;
	oldDepthRange     = depthRange = qfalse;
	oldAlphaTest      = alphaTest = qfalse;
	oldDeformType     = deformType = DEFORM_TYPE_NONE;
	drawShadows       = qtrue;
	cubeSide          = 0;
	splitFrustumIndex = 0;

	// if we need to clear the FBO color buffers then it should be white
	GL_ClearColor(GLCOLOR_WHITE);

	// render interactions
	for (iaCount = 0, iaFirst = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions; )
	{
		backEnd.currentLight  = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface               = ia->surface;
		shader                = ia->surfaceShader;
		alphaTest             = shader->alphaTest;

		if (shader->numDeforms)
		{
			deformType = ShaderRequiresCPUDeforms(shader) ? DEFORM_TYPE_CPU : DEFORM_TYPE_GPU;
		}
		else
		{
			deformType = DEFORM_TYPE_NONE;
		}

		if (glConfig2.occlusionQueryBits && r_dynamicLightOcclusionCulling->integer && !ia->occlusionQuerySamples)
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

		if (light->l.inverseShadows)
		{
			// handle those lights in RB_RenderInteractionsDeferredInverseShadows
			goto skipInteraction;
		}

		// only iaCount == iaFirst if first iteration or counters were reset
		if (iaCount == iaFirst)
		{
			if (drawShadows)
			{
				RB_SafeState();

				if (light->l.noShadows || light->shadowLOD < 0)
				{
					Ren_LogComment("----- Skipping shadowCube side: %i -----\n", cubeSide);

					goto skipInteraction;
				}
				else
				{
					switch (light->l.rlType)
					{
					case RL_OMNI:
					{
						//float           xMin, xMax, yMin, yMax;
						//float           width, height, depth;
						float    zNear, zFar;
						float    fovX, fovY;
						qboolean flipX, flipY;
						//float          *proj;
						vec3_t angles;
						mat4_t rotationMatrix, transformMatrix, viewMatrix;

						Ren_LogComment("----- Rendering shadowCube side: %i -----\n", cubeSide);

						R_BindFBO(tr.shadowMapFBO[light->shadowLOD]);
						R_AttachFBOTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeSide,
						                     tr.shadowCubeFBOImage[light->shadowLOD]->texnum, 0);
						if (!r_ignoreGLErrors->integer)
						{
							R_CheckFBO(tr.shadowMapFBO[light->shadowLOD]);
						}

						// set the window clipping
						GL_Viewport(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);
						GL_Scissor(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

						GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

						switch (cubeSide)
						{
						case 0:
						{
							// view parameters
							VectorSet(angles, 0, 0, 90);

							// projection parameters
							flipX = qfalse;
							flipY = qfalse;
							break;
						}
						case 1:
						{
							VectorSet(angles, 0, 180, 90);
							flipX = qtrue;
							flipY = qtrue;
							break;
						}
						case 2:
						{
							VectorSet(angles, 0, 90, 0);
							flipX = qfalse;
							flipY = qfalse;
							break;
						}
						case 3:
						{
							VectorSet(angles, 0, -90, 0);
							flipX = qtrue;
							flipY = qtrue;
							break;
						}
						case 4:
						{
							VectorSet(angles, -90, 90, 0);
							flipX = qfalse;
							flipY = qfalse;
							break;
						}
						case 5:
						{
							VectorSet(angles, 90, 90, 0);
							flipX = qtrue;
							flipY = qtrue;
							break;
						}
						default:
						{
							// shut up compiler
							VectorSet(angles, 0, 0, 0);
							flipX = qfalse;
							flipY = qfalse;
							break;
						}
						}

						// Quake -> OpenGL view matrix from light perspective
						mat4_from_angles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
						MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, light->origin);
						MatrixAffineInverse(transformMatrix, viewMatrix);

						// convert from our coordinate system (looking down X)
						// to OpenGL's coordinate system (looking down -Z)
						mat4_mult(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);

						// OpenGL projection matrix
						fovX = 90;
						fovY = 90;

						zNear = 1.0;
						zFar  = light->sphereRadius;

						if (flipX)
						{
							fovX = -fovX;
						}

						if (flipY)
						{
							fovY = -fovY;
						}

						MatrixPerspectiveProjectionFovXYRH(light->projectionMatrix, fovX, fovY, zNear, zFar);

						GL_LoadProjectionMatrix(light->projectionMatrix);
						break;
					}
					case RL_PROJ:
					{
						Ren_LogComment("--- Rendering projective shadowMap ---\n");

						R_BindFBO(tr.shadowMapFBO[light->shadowLOD]);
						R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.shadowMapFBOImage[light->shadowLOD]->texnum, 0);
						if (!r_ignoreGLErrors->integer)
						{
							R_CheckFBO(tr.shadowMapFBO[light->shadowLOD]);
						}

						// set the window clipping
						GL_Viewport(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);
						GL_Scissor(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

						GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

						GL_LoadProjectionMatrix(light->projectionMatrix);
						break;
					}
					case RL_DIRECTIONAL:
					{
						int    j;
						vec3_t angles;
						vec4_t forward, side, up;
						vec3_t lightDirection;
						vec3_t viewOrigin, viewDirection;
						mat4_t rotationMatrix, transformMatrix, viewMatrix, projectionMatrix, viewProjectionMatrix;
						mat4_t cropMatrix;
						vec4_t splitFrustum[6];
						vec3_t splitFrustumCorners[8];
						vec3_t splitFrustumBounds[2];
						//vec3_t   splitFrustumViewBounds[2];
						vec3_t splitFrustumClipBounds[2];
						//float    splitFrustumRadius;
						vec3_t casterBounds[2];
						vec3_t receiverBounds[2];
						vec3_t cropBounds[2];
						vec4_t point;
						vec4_t transf;

						Ren_LogComment("--- Rendering directional shadowMap ---\n");

						R_BindFBO(tr.sunShadowMapFBO[splitFrustumIndex]);
						if (!r_evsmPostProcess->integer)
						{
							R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.sunShadowMapFBOImage[splitFrustumIndex]->texnum, 0);
						}
						else
						{
							R_AttachFBOTextureDepth(tr.sunShadowMapFBOImage[splitFrustumIndex]->texnum);
						}
						if (!r_ignoreGLErrors->integer)
						{
							R_CheckFBO(tr.sunShadowMapFBO[splitFrustumIndex]);
						}

						// set the window clipping
						GL_Viewport(0, 0, sunShadowMapResolutions[splitFrustumIndex], sunShadowMapResolutions[splitFrustumIndex]);
						GL_Scissor(0, 0, sunShadowMapResolutions[splitFrustumIndex], sunShadowMapResolutions[splitFrustumIndex]);

						GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

							#if 1
						VectorCopy(tr.sunDirection, lightDirection);
							#else
						VectorCopy(light->direction, lightDirection);
							#endif

						if (r_parallelShadowSplits->integer)
						{
							int numCasters;

							// original light direction is from surface to light
							VectorInverse(lightDirection);
							VectorNormalize(lightDirection);

							VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);
							VectorCopy(backEnd.viewParms.orientation.axis[0], viewDirection);
							VectorNormalize(viewDirection);

#if 1
							// calculate new up dir
							CrossProduct(lightDirection, viewDirection, side);
							VectorNormalize(side);

							CrossProduct(side, lightDirection, up);
							VectorNormalize(up);

							VectorToAngles(lightDirection, angles);
							mat4_from_angles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
							AngleVectors(angles, forward, side, up);

							MatrixLookAtRH(light->viewMatrix, viewOrigin, lightDirection, up);
#else
							MatrixLookAtRH(light->viewMatrix, viewOrigin, lightDirection, viewDirection);
#endif

							for (j = 0; j < 6; j++)
							{
								VectorCopy(backEnd.viewParms.frustums[1 + splitFrustumIndex][j].normal, splitFrustum[j]);
								splitFrustum[j][3] = backEnd.viewParms.frustums[1 + splitFrustumIndex][j].dist;
							}

							// calculate split frustum corner points
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[0]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[1]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[2]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[3]);

							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[4]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[5]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[6]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[7]);

							if (RENLOG)
							{
								vec3_t rayIntersectionNear, rayIntersectionFar;
								float  zNear, zFar;

								Ren_LogComment("----- Skipping shadowCube side: %i -----\n", cubeSide);

								PlaneIntersectRay(viewOrigin, viewDirection, splitFrustum[FRUSTUM_FAR], rayIntersectionFar);
								zFar = Distance(viewOrigin, rayIntersectionFar);

								VectorInverse(viewDirection);

								PlaneIntersectRay(rayIntersectionFar, viewDirection, splitFrustum[FRUSTUM_NEAR], rayIntersectionNear);
								zNear = Distance(viewOrigin, rayIntersectionNear);

								VectorInverse(viewDirection);

								Ren_LogComment("split frustum %i: near = %5.3f, far = %5.3f\n", splitFrustumIndex, zNear, zFar);
								Ren_LogComment("pyramid nearCorners\n");
								for (j = 0; j < 4; j++)
								{
									Ren_LogComment("(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[j][0], splitFrustumCorners[j][1], splitFrustumCorners[j][2]);
								}

								Ren_LogComment("pyramid farCorners\n");
								for (j = 4; j < 8; j++)
								{
									Ren_LogComment("(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[j][0], splitFrustumCorners[j][1], splitFrustumCorners[j][2]);
								}
							}

							ClearBounds(splitFrustumBounds[0], splitFrustumBounds[1]);
							for (j = 0; j < 8; j++)
							{
								AddPointToBounds(splitFrustumCorners[j], splitFrustumBounds[0], splitFrustumBounds[1]);
							}

#if 0
							// Scene-Independent Projection

							// find the bounding box of the current split in the light's view space
							ClearBounds(splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
							//numCasters = MergeInteractionBounds(light->viewMatrix, ia, iaCount, splitFrustumViewBounds, qtrue);
							for (j = 0; j < 8; j++)
							{
								VectorCopy(splitFrustumCorners[j], point);
								point[3] = 1;

								MatrixTransform4(light->viewMatrix, point, transf);
								transf[0] /= transf[3];
								transf[1] /= transf[3];
								transf[2] /= transf[3];

								AddPointToBounds(transf, splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
							}

							//MatrixScaleTranslateToUnitCube(projectionMatrix, splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
							//MatrixOrthogonalProjectionRH(projectionMatrix, -1, 1, -1, 1, -splitFrustumViewBounds[1][2], -splitFrustumViewBounds[0][2]);
								#if 1
							MatrixOrthogonalProjectionRH(projectionMatrix, splitFrustumViewBounds[0][0],
							                             splitFrustumViewBounds[1][0],
							                             splitFrustumViewBounds[0][1],
							                             splitFrustumViewBounds[1][1],
							                             -splitFrustumViewBounds[1][2],
							                             -splitFrustumViewBounds[0][2]);
								#endif
							mat4_mult(projectionMatrix, light->viewMatrix, viewProjectionMatrix);

							// find the bounding box of the current split in the light's clip space
							ClearBounds(splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
							for (j = 0; j < 8; j++)
							{
								VectorCopy(splitFrustumCorners[j], point);
								point[3] = 1;

								mat4_transform_vec4(viewProjectionMatrix, point, transf);
								transf[0] /= transf[3];
								transf[1] /= transf[3];
								transf[2] /= transf[3];

								AddPointToBounds(transf, splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
							}
							splitFrustumClipBounds[0][2] = 0;
							splitFrustumClipBounds[1][2] = 1;

							MatrixCrop(cropMatrix, splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
							//MatrixIdentity(cropMatrix);

							Ren_LogComment("split frustum light view space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
							               splitFrustumViewBounds[0][0], splitFrustumViewBounds[0][1], splitFrustumViewBounds[0][2],
							               splitFrustumViewBounds[1][0], splitFrustumViewBounds[1][1], splitFrustumViewBounds[1][2]);

							Ren_LogComment("split frustum light clip space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
							               splitFrustumClipBounds[0][0], splitFrustumClipBounds[0][1], splitFrustumClipBounds[0][2],
							               splitFrustumClipBounds[1][0], splitFrustumClipBounds[1][1], splitFrustumClipBounds[1][2]);

#else
							// Scene-Dependent Projection

							// find the bounding box of the current split in the light's view space
							ClearBounds(cropBounds[0], cropBounds[1]);
							for (j = 0; j < 8; j++)
							{
								VectorCopy(splitFrustumCorners[j], point);
								//point[3] = 1.0f; // initialized once, ..no need to keep setting it.
#if 1
								mat4_transform_vec4(light->viewMatrix, point, transf);
								transf[0] /= transf[3];
								transf[1] /= transf[3];
								transf[2] /= transf[3];
#else
								mat4_transform_vec3(light->viewMatrix, point, transf);
#endif

								AddPointToBounds(transf, cropBounds[0], cropBounds[1]);
							}

							MatrixOrthogonalProjectionRH(projectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2]);

							mat4_mult(projectionMatrix, light->viewMatrix, viewProjectionMatrix);

							numCasters = MergeInteractionBounds(viewProjectionMatrix, ia, iaCount, casterBounds, qtrue);
							MergeInteractionBounds(viewProjectionMatrix, ia, iaCount, receiverBounds, qfalse);

							// find the bounding box of the current split in the light's clip space
							ClearBounds(splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
							for (j = 0; j < 8; j++)
							{
								VectorCopy(splitFrustumCorners[j], point);
								point[3] = 1;

								mat4_transform_vec4(viewProjectionMatrix, point, transf);
								transf[0] /= transf[3];
								transf[1] /= transf[3];
								transf[2] /= transf[3];

								AddPointToBounds(transf, splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
							}

							Ren_LogComment("shadow casters = %i\n", numCasters);
							Ren_LogComment("split frustum light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
							               splitFrustumClipBounds[0][0], splitFrustumClipBounds[0][1], splitFrustumClipBounds[0][2],
							               splitFrustumClipBounds[1][0], splitFrustumClipBounds[1][1], splitFrustumClipBounds[1][2]);
							Ren_LogComment("shadow caster light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
							               casterBounds[0][0], casterBounds[0][1], casterBounds[0][2],
							               casterBounds[1][0], casterBounds[1][1], casterBounds[1][2]);
							Ren_LogComment("light receiver light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
							               receiverBounds[0][0], receiverBounds[0][1], receiverBounds[0][2],
							               receiverBounds[1][0], receiverBounds[1][1], receiverBounds[1][2]);

							// scene-dependent bounding volume
							cropBounds[0][0] = Q_max(Q_max(casterBounds[0][0], receiverBounds[0][0]), splitFrustumClipBounds[0][0]);
							cropBounds[0][1] = Q_max(Q_max(casterBounds[0][1], receiverBounds[0][1]), splitFrustumClipBounds[0][1]);

							cropBounds[1][0] = Q_min(Q_min(casterBounds[1][0], receiverBounds[1][0]), splitFrustumClipBounds[1][0]);
							cropBounds[1][1] = Q_min(Q_min(casterBounds[1][1], receiverBounds[1][1]), splitFrustumClipBounds[1][1]);

							cropBounds[0][2] = Q_min(casterBounds[0][2], splitFrustumClipBounds[0][2]);
							//cropBounds[0][2] = casterBounds[0][2];
							//cropBounds[0][2] = splitFrustumClipBounds[0][2];
							cropBounds[1][2] = Q_min(receiverBounds[1][2], splitFrustumClipBounds[1][2]);
							//cropBounds[1][2] = splitFrustumClipBounds[1][2];

							if (numCasters == 0)
							{
								VectorCopy(splitFrustumClipBounds[0], cropBounds[0]);
								VectorCopy(splitFrustumClipBounds[1], cropBounds[1]);
							}

							MatrixCrop(cropMatrix, cropBounds[0], cropBounds[1]);
#endif

							mat4_mult(cropMatrix, projectionMatrix, light->projectionMatrix);

							GL_LoadProjectionMatrix(light->projectionMatrix);
						}
						else
						{
							// original light direction is from surface to light
							VectorInverse(lightDirection);

							// Quake -> OpenGL view matrix from light perspective
#if 1
							VectorToAngles(lightDirection, angles);
							mat4_from_angles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
							MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, backEnd.viewParms.orientation.origin);
							MatrixAffineInverse(transformMatrix, viewMatrix);
							mat4_mult(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);
#else
							MatrixLookAtRH(light->viewMatrix, backEnd.viewParms.orientation.origin, lightDirection, backEnd.viewParms.orientation.axis[0]);
#endif

							ClearBounds(splitFrustumBounds[0], splitFrustumBounds[1]);
							//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], backEnd.viewParms.visBounds[0], backEnd.viewParms.visBounds[1]);
							BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], light->worldBounds[0], light->worldBounds[1]);

							ClearBounds(cropBounds[0], cropBounds[1]);
							for (j = 0; j < 8; j++)
							{
								point[0] = splitFrustumBounds[j & 1][0];
								point[1] = splitFrustumBounds[(j >> 1) & 1][1];
								point[2] = splitFrustumBounds[(j >> 2) & 1][2];
								point[3] = 1;

								mat4_transform_vec4(light->viewMatrix, point, transf);
								transf[0] /= transf[3];
								transf[1] /= transf[3];
								transf[2] /= transf[3];

								AddPointToBounds(transf, cropBounds[0], cropBounds[1]);
							}

							// transform from OpenGL's right handed into D3D's left handed coordinate system
#if 0
							MatrixScaleTranslateToUnitCube(projectionMatrix, cropBounds[0], cropBounds[1]);
							MatrixMultiplyMOD(flipZMatrix, projectionMatrix, light->projectionMatrix);
#else
							MatrixOrthogonalProjectionRH(light->projectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2]);
#endif
							GL_LoadProjectionMatrix(light->projectionMatrix);
						}
						break;
					}

					default:
						break;
					}
				}

				Ren_LogComment("----- First Shadow Interaction: %i -----\n", iaCount);
			}
			else
			{
				Ren_LogComment("--- Rendering lighting ---\n");
				Ren_LogComment("----- First Light Interaction: %i -----\n", iaCount);

				if (r_hdrRendering->integer)
				{
					R_BindFBO(tr.deferredRenderFBO);
				}
				else
				{
					R_BindNullFBO();
				}

				// set the window clipping
				GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				           backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				// restore camera matrices
				GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);
				GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

				// reset light view and projection matrices
				switch (light->l.rlType)
				{
				case RL_OMNI:
				{
					MatrixAffineInverse(light->transformMatrix, light->viewMatrix);
					mat4_reset_scale(light->projectionMatrix, 1.0f / light->l.radius[0], 1.0f / light->l.radius[1],
					                 1.0f / light->l.radius[2]);
					break;
				}
				case RL_DIRECTIONAL:
				{
					// draw split frustum shadow maps
					if (r_showShadowMaps->integer)
					{
						int    frustumIndex;
						float  x, y, w, h;
						vec4_t quadVerts[4];

						// set 2D virtual screen size
						GL_PushMatrix();
						RB_SetViewMVPM();

						for (frustumIndex = 0; frustumIndex <= r_parallelShadowSplits->integer; frustumIndex++)
						{
							GL_Cull(CT_TWO_SIDED);
							GL_State(GLS_DEPTHTEST_DISABLE);

							SetMacrosAndSelectProgram(trProg.gl_debugShadowMapShader);
							SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

							SelectTexture(TEX_CURRENT);
							GL_Bind(tr.sunShadowMapFBOImage[frustumIndex]);

							w = 200;
							h = 200;

							x = 205 * frustumIndex;
							y = 70;

							Vector4Set(quadVerts[0], x, y, 0, 1);
							Vector4Set(quadVerts[1], x + w, y, 0, 1);
							Vector4Set(quadVerts[2], x + w, y + h, 0, 1);
							Vector4Set(quadVerts[3], x, y + h, 0, 1);

							Tess_InstantQuad(quadVerts);

							{
								int    j;
								vec4_t splitFrustum[6];
								vec3_t farCorners[4];
								vec3_t nearCorners[4];

								GL_Viewport(x, y, w, h);
								GL_Scissor(x, y, w, h);

								GL_PushMatrix();

								SetMacrosAndSelectProgram(trProg.gl_genericShader);

								GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
								SetUniformVec4(UNIFORM_COLOR, colorBlack);

								GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
								GL_Cull(CT_TWO_SIDED);

								// bind u_ColorMap
								SelectTexture(TEX_COLOR);
								GL_Bind(tr.whiteImage);

								SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);
								SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, light->shadowMatrices[frustumIndex]);

								tess.multiDrawPrimitives = 0;
								tess.numIndexes          = 0;
								tess.numVertexes         = 0;

								for (j = 0; j < 6; j++)
								{
									VectorCopy(backEnd.viewParms.frustums[1 + frustumIndex][j].normal, splitFrustum[j]);
									splitFrustum[j][3] = backEnd.viewParms.frustums[1 + frustumIndex][j].dist;
								}

								// calculate split frustum corner points
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], nearCorners[0]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], nearCorners[1]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], nearCorners[2]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], nearCorners[3]);

								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], farCorners[0]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], farCorners[1]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], farCorners[2]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], farCorners[3]);

								// draw outer surfaces
								for (j = 0; j < 4; j++)
								{
									Vector4Set(quadVerts[0], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
									Vector4Set(quadVerts[1], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
									Vector4Set(quadVerts[2], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
									Vector4Set(quadVerts[3], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
									Tess_AddQuadStamp2(quadVerts, colorCyan);
								}

								// draw far cap
								Vector4Set(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
								Vector4Set(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
								Vector4Set(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
								Vector4Set(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
								Tess_AddQuadStamp2(quadVerts, colorBlue);

								// draw near cap
								Vector4Set(quadVerts[0], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
								Vector4Set(quadVerts[1], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
								Vector4Set(quadVerts[2], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
								Vector4Set(quadVerts[3], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
								Tess_AddQuadStamp2(quadVerts, colorGreen);

								Tess_UpdateVBOs(tess.attribsSet); // set by Tess_AddQuadStamp2
								Tess_DrawElements();

								// draw light volume
								if (light->isStatic && light->frustumVBO && light->frustumIBO)
								{
									GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CUSTOM_RGB, AGEN_CUSTOM);
									SetUniformVec4(UNIFORM_COLOR, colorYellow);

									R_BindVBO(light->frustumVBO);
									R_BindIBO(light->frustumIBO);

									GLSL_VertexAttribsState(ATTR_POSITION);

									tess.numVertexes = light->frustumVerts;
									tess.numIndexes  = light->frustumIndexes;

									Tess_DrawElements();
								}

								tess.multiDrawPrimitives = 0;
								tess.numIndexes          = 0;
								tess.numVertexes         = 0;

								GL_PopMatrix();

								GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
								            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

								GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
								           backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
							}
						}

						GL_PopMatrix();
					}
				}
				break;
				default:
					break;
				}
			}
		}                       // end if(iaCount == iaFirst)

		if (drawShadows)
		{
			if (entity->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
			{
				goto skipInteraction;
			}

			if (shader->isSky)
			{
				goto skipInteraction;
			}

			if (shader->sort > SS_OPAQUE)
			{
				goto skipInteraction;
			}

			if (shader->noShadows || light->l.noShadows || light->shadowLOD < 0)
			{
				goto skipInteraction;
			}

			/*
			   if(light->l.inverseShadows && (entity == &tr.worldEntity))
			   {
			   // this light only casts shadows by its player and their items
			   goto skipInteraction;
			   }
			 */

			if (ia->type == IA_LIGHTONLY)
			{
				goto skipInteraction;
			}

			if (light->l.rlType == RL_OMNI && !(ia->cubeSideBits & (1 << cubeSide)))
			{
				goto skipInteraction;
			}

			switch (light->l.rlType)
			{
			case RL_OMNI:
			case RL_PROJ:
			case RL_DIRECTIONAL:
			{
				if (light == oldLight && entity == oldEntity && (alphaTest ? shader == oldShader : alphaTest == oldAlphaTest) && deformType == oldDeformType)
				{
					Ren_LogComment("----- Batching Shadow Interaction: %i -----\n", iaCount);

					// fast path, same as previous
					rb_surfaceTable[*surface] (surface);
					goto nextInteraction;
				}
				else
				{
					if (oldLight)
					{
						// draw the contents of the last shader batch
						Tess_End();
					}

					Ren_LogComment("----- Beginning Shadow Interaction: %i -----\n", iaCount);

					// we don't need tangent space calculations here
					Tess_Begin(Tess_StageIteratorShadowFill, NULL, shader, light->shader, qtrue, qfalse, -1, 0);
				}
				break;
			}
			default:
				break;
			}
		}
		else
		{
			if (!shader->interactLight)
			{
				goto skipInteraction;
			}

			if (ia->type == IA_SHADOWONLY)
			{
				goto skipInteraction;
			}

			if (glConfig2.occlusionQueryBits && r_dynamicEntityOcclusionCulling->integer && !entity->occlusionQuerySamples)
			{
				goto skipInteraction;
			}

			if (light == oldLight && entity == oldEntity && shader == oldShader)
			{
				Ren_LogComment("----- Batching Light Interaction: %i -----\n", iaCount);

				// fast path, same as previous
				rb_surfaceTable[*surface] (surface);
				goto nextInteraction;
			}
			else
			{
				if (oldLight)
				{
					// draw the contents of the last shader batch
					Tess_End();
				}

				Ren_LogComment("----- Beginning Light Interaction: %i -----\n", iaCount);

				// begin a new batch
				Tess_Begin(Tess_StageIteratorLighting, NULL, shader, light->shader, light->l.inverseShadows, qfalse, -1, 0);
			}
		}

		// change the modelview matrix if needed
		if (entity != oldEntity)
		{
			depthRange = qfalse;

			if (entity != &tr.worldEntity)
			{
				// set up the transformation matrix
				if (drawShadows)
				{
					R_RotateEntityForLight(entity, light, &backEnd.orientation);
				}
				else
				{
					R_RotateEntityForViewParms(entity, &backEnd.viewParms, &backEnd.orientation);
				}

				if (entity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				// set up the transformation matrix
				if (drawShadows)
				{
					Com_Memset(&backEnd.orientation, 0, sizeof(backEnd.orientation));

					backEnd.orientation.axis[0][0] = 1;
					backEnd.orientation.axis[1][1] = 1;
					backEnd.orientation.axis[2][2] = 1;
					VectorCopy(light->l.origin, backEnd.orientation.viewOrigin);

					mat4_ident(backEnd.orientation.transformMatrix);
					//MatrixAffineInverse(backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					mat4_mult(light->viewMatrix, backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					mat4_copy(backEnd.orientation.viewMatrix, backEnd.orientation.modelViewMatrix);
				}
				else
				{
					// transform by the camera placement
					backEnd.orientation = backEnd.viewParms.world;
				}
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			// change depthrange if needed
			if (oldDepthRange != depthRange)
			{
				if (depthRange)
				{
					glDepthRange(0, 0.3);
				}
				else
				{
					glDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}
		}

		// change the attenuation matrix if needed
		if (light != oldLight || entity != oldEntity)
		{
			// transform light origin into model space for u_LightOrigin parameter
			if (entity != &tr.worldEntity)
			{
				VectorSubtract(light->origin, backEnd.orientation.origin, tmp);
				light->transformed[0] = DotProduct(tmp, backEnd.orientation.axis[0]);
				light->transformed[1] = DotProduct(tmp, backEnd.orientation.axis[1]);
				light->transformed[2] = DotProduct(tmp, backEnd.orientation.axis[2]);
			}
			else
			{
				VectorCopy(light->origin, light->transformed);
			}

			mat4_mult(light->viewMatrix, backEnd.orientation.transformMatrix, modelToLight);

			// build the attenuation matrix using the entity transform
			switch (light->l.rlType)
			{
			case RL_OMNI:
			{
				mat4_reset_translate(light->attenuationMatrix, 0.5, 0.5, 0.5);    // bias
				MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);       // scale
				mat4_mult_self(light->attenuationMatrix, light->projectionMatrix);
				mat4_mult_self(light->attenuationMatrix, modelToLight);

				mat4_copy(light->attenuationMatrix, light->shadowMatrices[0]);
				break;
			}
			case RL_PROJ:
			{
				mat4_reset_translate(light->attenuationMatrix, 0.5, 0.5, 0.0);        // bias
				MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min(light->falloffLength, 1.0));        // scale
				mat4_mult_self(light->attenuationMatrix, light->projectionMatrix);
				mat4_mult_self(light->attenuationMatrix, modelToLight);

				mat4_copy(light->attenuationMatrix, light->shadowMatrices[0]);
				break;
			}
			case RL_DIRECTIONAL:
			{
				mat4_reset_translate(light->attenuationMatrix, 0.5, 0.5, 0.5);    // bias
				MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);       // scale
				mat4_mult_self(light->attenuationMatrix, light->projectionMatrix);
				mat4_mult_self(light->attenuationMatrix, modelToLight);
				break;
			}
			default:
				break;
			}
		}

		if (drawShadows)
		{
			switch (light->l.rlType)
			{
			case RL_OMNI:
			case RL_PROJ:
			case RL_DIRECTIONAL:
			{
				// add the triangles for this surface
				rb_surfaceTable[*surface] (surface);
				break;
			}
			default:
				break;
			}
		}
		else
		{
			// add the triangles for this surface
			rb_surfaceTable[*surface] (surface);
		}

nextInteraction:

		// remember values
		oldLight      = light;
		oldEntity     = entity;
		oldShader     = shader;
		oldAlphaTest  = alphaTest;
		oldDeformType = deformType;

skipInteraction:
		if (!ia->next)
		{
			// if ia->next does not point to any other interaction then
			// this is the last interaction of the current light

			Ren_LogComment("----- Last Interaction: %i -----\n", iaCount);

			// draw the contents of the last shader batch
			Tess_End();

			if (drawShadows)
			{
				switch (light->l.rlType)
				{
				case RL_OMNI:
				{
					if (cubeSide == 5)
					{
						cubeSide    = 0;
						drawShadows = qfalse;
					}
					else
					{
						cubeSide++;
					}

					// jump back to first interaction of this light
					ia      = &backEnd.viewParms.interactions[iaFirst];
					iaCount = iaFirst;
					break;
				}
				case RL_PROJ:
				{
					// jump back to first interaction of this light and start lighting
					ia          = &backEnd.viewParms.interactions[iaFirst];
					iaCount     = iaFirst;
					drawShadows = qfalse;
					break;
				}
				case RL_DIRECTIONAL:
				{
					// set shadow matrix including scale + offset
					mat4_copy(bias, light->shadowMatricesBiased[splitFrustumIndex]);
					mat4_mult_self(light->shadowMatricesBiased[splitFrustumIndex], light->projectionMatrix);
					mat4_mult_self(light->shadowMatricesBiased[splitFrustumIndex], light->viewMatrix);

					mat4_mult(light->projectionMatrix, light->viewMatrix, light->shadowMatrices[splitFrustumIndex]);

					if (r_parallelShadowSplits->integer)
					{
						if (splitFrustumIndex == r_parallelShadowSplits->integer)
						{
							splitFrustumIndex = 0;
							drawShadows       = qfalse;
						}
						else
						{
							splitFrustumIndex++;
						}

						// jump back to first interaction of this light
						ia      = &backEnd.viewParms.interactions[iaFirst];
						iaCount = iaFirst;
					}
					else
					{
						// jump back to first interaction of this light and start lighting
						ia          = &backEnd.viewParms.interactions[iaFirst];
						iaCount     = iaFirst;
						drawShadows = qfalse;
					}
					break;
				}
				default:
					break;
				}
			}
			else
			{
				// draw the light volume if needed
				if (light->shader->volumetricLight)
				{
					Render_lightVolume(ia);
				}

				if (iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and start shadowing
					ia++;
					iaCount++;
					iaFirst     = iaCount;
					drawShadows = qtrue;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}

			// force updates
			oldLight  = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	if (depthRange)
	{
		glDepthRange(0, 1);
	}

	// reset scissor clamping
	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	           backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	// reset clear color
	GL_ClearColor(GLCOLOR_BLACK);

	GL_CheckErrors();

	R2_TIMING(RSPEEDS_SHADING_TIMES)
	{
		backEnd.pc.c_forwardLightingTime = ri.Milliseconds() - startTime;
	}
}

/**
 * @brief RB_RenderScreenSpaceAmbientOcclusion
 */
void RB_RenderScreenSpaceAmbientOcclusion()
{
	Ren_LogComment("--- RB_RenderScreenSpaceAmbientOcclusion ---\n");

	if (!r_screenSpaceAmbientOcclusion->integer)
	{
		return;
	}

	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	// enable shader, set arrays
	SetMacrosAndSelectProgram(trProg.gl_ssao);

	GL_State(GLS_DEPTHTEST_DISABLE);    // | GLS_DEPTHMASK_TRUE);
	GL_Cull(CT_TWO_SIDED);

	glVertexAttrib4fv(ATTR_INDEX_COLOR, colorWhite);

	// capture current color buffer for u_CurrentMap
	SelectTexture(TEX_CURRENT);
	ImageCopyBackBuffer(tr.currentRenderImage);

	// bind u_DepthMap
	SelectTexture(TEX_DEPTH);
	ImageCopyBackBuffer(tr.depthRenderImage);

	// set 2D virtual screen size
	GL_PushMatrix();
	RB_SetViewMVPM();

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// draw viewport
	DRAWVIEWQUAD();

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

/**
 * @brief RB_RenderDepthOfField
 */
void RB_RenderDepthOfField()
{
	Ren_LogComment("--- RB_RenderDepthOfField ---\n");

	if (!r_depthOfField->integer)
	{
		return;
	}

	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	// enable shader, set arrays
	SetMacrosAndSelectProgram(trProg.gl_depthOfField);

	GL_State(GLS_DEPTHTEST_DISABLE);    // | GLS_DEPTHMASK_TRUE);
	GL_Cull(CT_TWO_SIDED);

	glVertexAttrib4fv(ATTR_INDEX_COLOR, colorWhite);

	// capture current color buffer for u_CurrentMap
	SelectTexture(TEX_CURRENT);
	if (r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable)
	{
		GL_Bind(tr.deferredRenderFBOImage);
	}
	else
	{
		ImageCopyBackBuffer(tr.currentRenderImage);
	}

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

	// set 2D virtual screen size
	GL_PushMatrix();
	RB_SetViewMVPM();

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// draw viewport
	DRAWVIEWQUAD();

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

/**
 * @brief RB_RenderGlobalFog
 *
 * This is the fogGlobal glsl shader
 *
 */
void RB_RenderGlobalFog()
{
	vec3_t local;
	vec4_t fogDistanceVector;

	Ren_LogComment("--- RB_RenderGlobalFog ---\n");

	if (!tr.world || tr.world->globalFog < 0||!r_wolfFog->integer)
	{
		return;
	}

	// no fog pass in snooper
	if ((tr.refdef.rdflags & RDF_SNOOPERVIEW) || tess.surfaceShader->noFog)
	{
		return;
	}

	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	
	

	GL_Cull(CT_TWO_SIDED);

	// go back to the world modelview matrix
	backEnd.orientation = backEnd.viewParms.world;

	SetMacrosAndSelectProgram(trProg.gl_fogGlobalShader);
	SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // world space

	{
		fog_t *fog = &tr.world->fogs[tr.world->globalFog];

		Ren_LogComment("--- RB_RenderGlobalFog( fogNum = %i, originalBrushNumber = %i ) ---\n", tr.world->globalFog, fog->originalBrushNumber);

		GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

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

		SetUniformVec4(UNIFORM_FOGDISTANCEVECTOR, fogDistanceVector);
		SetUniformVec4(UNIFORM_COLOR, fog->color);

		//if there is a density set
		if (fog->fogParms.density > 0)
		{
			SetUniformFloat(UNIFORM_FOGDENSITY, fog->fogParms.density);
		}
		else// use the depthforOpaque wich is set in shader fogparms for end of fog aka where fog is 1.0
		{
			SetUniformFloat(UNIFORM_FOGDENSITY, fog->fogParms.depthForOpaque);
		}
		// FIXME: fog->fogParms.density?!.. density dont seem to get any value?!
	}

	SetUniformMatrix16(UNIFORM_VIEWMATRIX, backEnd.viewParms.world.viewMatrix);
	SetUniformMatrix16(UNIFORM_UNPROJECTMATRIX, backEnd.viewParms.unprojectionMatrix);
	
	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(tr.fogImage);
	
	
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

	// set 2D virtual screen size
	GL_PushMatrix();
	RB_SetViewMVPM();

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// draw viewport
	DRAWVIEWQUAD();

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

/**
 * @brief RB_RenderBloom
 */
void RB_RenderBloom()
{
	int    i, j;
	mat4_t ortho;

	Ren_LogComment("--- RB_RenderBloom ---\n");

	// hdr requires bloom renderer
	if (!r_bloom->integer && !HDR_ENABLED())
	{
		return;
	}

	if ((backEnd.refdef.rdflags & (RDF_NOWORLDMODEL | RDF_NOBLOOM)) || backEnd.viewParms.isPortal || !glConfig2.framebufferObjectAvailable)
	{
		return;
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	RB_SetViewMVPM();

	// FIXME
	//if(glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10)
	{
		GL_State(GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		GL_PushMatrix();
		GL_LoadModelViewMatrix(matrixIdentity);

#if 1
		MatrixOrthogonalProjection(ortho, 0, tr.contrastRenderFBO->width, 0, tr.contrastRenderFBO->height, -99999, 99999);
		GL_LoadProjectionMatrix(ortho);
#endif

		if (HDR_ENABLED())
		{
			SetMacrosAndSelectProgram(trProg.gl_toneMappingShader, BRIGHTPASS_FILTER, qtrue);

			SetUniformFloat(UNIFORM_HDRKEY, backEnd.hdrKey);
			SetUniformFloat(UNIFORM_HDRAVERAGELUMINANCE, backEnd.hdrAverageLuminance);
			SetUniformFloat(UNIFORM_HDRMAXLUMINANCE, backEnd.hdrMaxLuminance);
			SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

			SelectTexture(TEX_CURRENT);
			GL_Bind(tr.downScaleFBOImage_quarter);
		}
		else
		{
			// render contrast downscaled to 1/4th of the screen
			SetMacrosAndSelectProgram(trProg.gl_contrastShader);
			SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

			SelectTexture(TEX_COLOR);
			//GL_Bind(tr.downScaleFBOImage_quarter);
			ImageCopyBackBuffer(tr.currentRenderImage);
		}

		GL_PopMatrix(); // special 1/4th of the screen contrastRenderFBO ortho

		R_BindFBO(tr.contrastRenderFBO);
		GL_ClearColor(GLCOLOR_BLACK);
		GL_Clear(GL_COLOR_BUFFER_BIT);

		// draw viewport
		DRAWVIEWQUAD();

		// render bloom in multiple passes
		SetMacrosAndSelectProgram(trProg.gl_bloomShader);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
		SetUniformFloat(UNIFORM_BLURMAGNITUDE, r_bloomBlur->value);

		for (i = 0; i < 2; i++)
		{
			for (j = 0; j < r_bloomPasses->integer; j++)
			{
				R_BindFBO(tr.bloomRenderFBO[(j + 1) % 2]);

				GL_ClearColor(GLCOLOR_BLACK);
				GL_Clear(GL_COLOR_BUFFER_BIT);

				GL_State(GLS_DEPTHTEST_DISABLE);

				GL_PushMatrix();
				GL_LoadModelViewMatrix(matrixIdentity);

				MatrixOrthogonalProjection(ortho, 0, tr.bloomRenderFBO[0]->width, 0, tr.bloomRenderFBO[0]->height, -99999, 99999);
				GL_LoadProjectionMatrix(ortho);

				if (i == 0)
				{
					SetMacrosAndSelectProgram(trProg.gl_blurXShader);

				}
				else
				{
					SetMacrosAndSelectProgram(trProg.gl_blurYShader);
				}

				SelectTexture(TEX_COLOR);
				if (j == 0)
				{
					GL_Bind(tr.contrastRenderFBOImage);
				}
				else
				{
					GL_Bind(tr.bloomRenderFBOImage[j % 2]);
				}

				SetUniformFloat(UNIFORM_DEFORMMAGNITUDE, r_bloomBlur->value);
				SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

				GL_PopMatrix();

				DRAWVIEWQUAD();
			}

			// add offscreen processed bloom to screen
			if (HDR_ENABLED())
			{
				R_BindFBO(tr.deferredRenderFBO);

				SetMacrosAndSelectProgram(trProg.gl_screenShader);
				GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorWhite);

				SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

				SelectTexture(TEX_CURRENT);
				GL_Bind(tr.bloomRenderFBOImage[j % 2]);
				//GL_Bind(tr.contrastRenderFBOImage);
			}
			else
			{
				R_BindNullFBO();

				SetMacrosAndSelectProgram(trProg.gl_screenShader);
				GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorWhite);

				SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

				SelectTexture(TEX_CURRENT);
				GL_Bind(tr.bloomRenderFBOImage[j % 2]);
				//GL_Bind(tr.contrastRenderFBOImage);
			}

			DRAWVIEWQUAD();
		}
	}

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

/**
 * @brief RB_RenderRotoscope
 */
void RB_RenderRotoscope(void)
{
	Ren_LogComment("--- RB_RenderRotoscope ---\n");

	if (!r_rotoscope->integer)
	{
		return;
	}
	
	if ((backEnd.refdef.rdflags & RDF_NOWORLDMODEL) || backEnd.viewParms.isPortal)
	{
		return;
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	RB_SetViewMVPM();

	GL_State(GLS_DEPTHTEST_DISABLE);
	GL_Cull(CT_TWO_SIDED);

	// enable shader, set arrays
	SetMacrosAndSelectProgram(trProg.gl_rotoscopeShader);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	SetUniformFloat(UNIFORM_BLURMAGNITUDE, r_rotoscopeBlur->value);

	SelectTexture(TEX_COLOR);
	ImageCopyBackBuffer(tr.currentRenderImage);

	// draw viewport
	DRAWVIEWQUAD();

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

/**
 * @brief RB_CameraPostFX
 */
void RB_CameraPostFX(void)
{
	mat4_t grain;

	Ren_LogComment("--- RB_CameraPostFX ---\n");

	if (!r_cameraPostFX->integer)
	{
		return;
	}

	if ((backEnd.refdef.rdflags & RDF_NOWORLDMODEL) || backEnd.viewParms.isPortal)
	{
		return;
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	RB_SetViewMVPM();

	GL_State(GLS_DEPTHTEST_DISABLE);
	GL_Cull(CT_TWO_SIDED);

	// enable shader, set arrays
	SetMacrosAndSelectProgram(trProg.gl_cameraEffectsShader);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	//glUniform1f(tr.cameraEffectsShader.u_BlurMagnitude, r_bloomBlur->value);

	mat4_ident(grain);

	MatrixMultiplyScale(grain, r_cameraFilmGrainScale->value, r_cameraFilmGrainScale->value, 0);
	MatrixMultiplyTranslation(grain, tess.shaderTime * 10, backEnd.refdef.floatTime * 10, 0);

	MatrixMultiplyTranslation(grain, 0.5, 0.5, 0.0);
	MatrixMultiplyZRotation(grain, tess.shaderTime * (random() * 7));
	MatrixMultiplyTranslation(grain, -0.5, -0.5, 0.0);

	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, grain);

	// bind u_CurrentMap
	SelectTexture(TEX_CURRENT);
	ImageCopyBackBuffer(tr.occlusionRenderFBOImage);

	// bind u_GrainMap
	SelectTexture(TEX_GRAIN);
	if (tr.grainImage)
	{
		GL_Bind(tr.grainImage);
	}
	else
	{
		GL_Bind(tr.defaultImage);
	}

	// bind u_VignetteMap
	SelectTexture(TEX_VIGNETTE);
	if (r_cameraVignette->integer)
	{
		GL_Bind(tr.vignetteImage);
	}
	else
	{
		GL_Bind(tr.whiteImage);
	}

	// draw viewport
	DRAWVIEWQUAD();

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

/**
 * @brief RB_CalculateAdaptation
 */
static void RB_CalculateAdaptation()
{
	int          i;
	static float image[64 * 64 * 4];
	float        curTime = ri.Milliseconds() / 1000.0f;
	float        deltaTime;
	float        luminance;
	float        avgLuminance;
	float        maxLuminance     = 0.0f;
	double       sum              = 0.0;
	const vec3_t LUMINANCE_VECTOR = { 0.2125f, 0.7154f, 0.0721f };
	vec4_t       color;
	float        newAdaptation;
	float        newMaximum;

	// calculate the average scene luminance
	R_BindFBO(tr.downScaleFBO_64x64);

	// read back the contents
	//glFinish();
	glReadPixels(0, 0, 64, 64, GL_RGBA, GL_FLOAT, image);

	for (i = 0; i < (64 * 64 * 4); i += 4)
	{
		color[0] = image[i + 0];
		color[1] = image[i + 1];
		color[2] = image[i + 2];
		color[3] = image[i + 3];

		luminance = DotProduct(color, LUMINANCE_VECTOR) + 0.0001f;
		if (luminance > maxLuminance)
		{
			maxLuminance = luminance;
		}

		sum += log(luminance);
	}
	sum         /= (64.0 * 64.0);
	avgLuminance = exp(sum);

	// the user's adapted luminance level is simulated by closing the gap between
	// adapted luminance and current luminance by 2% every frame, based on a
	// 30 fps rate. This is not an accurate model of human adaptation, which can
	// take longer than half an hour.
	if (backEnd.hdrTime > curTime)
	{
		backEnd.hdrTime = curTime;
	}

	deltaTime = curTime - backEnd.hdrTime;

	//if(r_hdrMaxLuminance->value)
	{
		Q_clamp(backEnd.hdrAverageLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value);
		Q_clamp(avgLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value);

		Q_clamp(backEnd.hdrMaxLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value);
		Q_clamp(maxLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value);
	}

	newAdaptation = backEnd.hdrAverageLuminance + (avgLuminance - backEnd.hdrAverageLuminance) * (1.0f - powf(0.98f, 30.0f * deltaTime));
	newMaximum    = backEnd.hdrMaxLuminance + (maxLuminance - backEnd.hdrMaxLuminance) * (1.0f - powf(0.98f, 30.0f * deltaTime));

	if (!Q_isnan(newAdaptation) && !Q_isnan(newMaximum))
	{
#if 1
		backEnd.hdrAverageLuminance = newAdaptation;
		backEnd.hdrMaxLuminance     = newMaximum;
#else
		backEnd.hdrAverageLuminance = avgLuminance;
		backEnd.hdrMaxLuminance     = maxLuminance;
#endif
	}

	backEnd.hdrTime = curTime;

	// calculate HDR image key
	if (r_hdrKey->value <= 0)
	{
		// calculation from: Perceptual Effects in Real-time Tone Mapping - Krawczyk et al.
		backEnd.hdrKey = 1.03f - 2.0f / (2.0f + log10f(backEnd.hdrAverageLuminance + 1.0f));
	}
	else
	{
		backEnd.hdrKey = r_hdrKey->value;
	}

	if (r_hdrDebug->integer)
	{
		Ren_Print("HDR luminance avg = %f, max = %f, key = %f\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey);
	}

	GL_CheckErrors();
}

// ================================================================================================
// LIGHTS OCCLUSION CULLING
// ================================================================================================

/**
 * @brief RenderLightOcclusionVolume
 * @param[in] light
 */
static void RenderLightOcclusionVolume(trRefLight_t *light)
{
	int    j;
	vec4_t quadVerts[4];

	GL_CheckErrors();

#if 1
	if (light->isStatic && light->frustumVBO && light->frustumIBO)
	{
		// render in world space
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		R_BindVBO(light->frustumVBO);
		R_BindIBO(light->frustumIBO);

		GLSL_VertexAttribsState(ATTR_POSITION);

		tess.numVertexes = light->frustumVerts;
		tess.numIndexes  = light->frustumIndexes;

		Tess_DrawElements();
	}
	else
#endif
	{
		// render in light space
		R_RotateLightForViewParms(light, &backEnd.viewParms, &backEnd.orientation);
		GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		tess.multiDrawPrimitives = 0;
		tess.numIndexes          = 0;
		tess.numVertexes         = 0;

		switch (light->l.rlType)
		{
		case RL_OMNI:
		{
			Tess_AddCube(vec3_origin, light->localBounds[0], light->localBounds[1], colorWhite);

			Tess_UpdateVBOs(tess.attribsSet); // set by Tess_AddCube
			Tess_DrawElements();
			break;
		}
		case RL_PROJ:
		{
			vec3_t farCorners[4];
			vec4_t *frustum = light->localFrustum;

			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[0]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[1]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[2]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[3]);

			tess.multiDrawPrimitives = 0;
			tess.numVertexes         = 0;
			tess.numIndexes          = 0;

			if (!VectorCompare(light->l.projStart, vec3_origin))
			{
				vec3_t nearCorners[4];

				// calculate the vertices defining the top area
				PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[0]);
				PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[1]);
				PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[2]);
				PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[3]);

				// draw outer surfaces
				for (j = 0; j < 4; j++)
				{
					Vector4Set(quadVerts[0], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
					Vector4Set(quadVerts[1], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
					Vector4Set(quadVerts[2], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
					Vector4Set(quadVerts[3], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
					Tess_AddQuadStamp2(quadVerts, colorCyan);
				}

				// draw far cap
				Vector4Set(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
				Vector4Set(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
				Vector4Set(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
				Vector4Set(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
				Tess_AddQuadStamp2(quadVerts, colorRed);

				// draw near cap
				Vector4Set(quadVerts[0], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
				Vector4Set(quadVerts[1], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
				Vector4Set(quadVerts[2], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
				Vector4Set(quadVerts[3], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
				Tess_AddQuadStamp2(quadVerts, colorGreen);
			}
			else
			{
				vec3_t top;

				// no light_start, just use the top vertex (doesn't need to be mirrored)
				PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], top);

				// draw pyramid
				for (j = 0; j < 4; j++)
				{
					VectorCopy(farCorners[j], tess.xyz[tess.numVertexes]);
					Vector4Copy(colorCyan, tess.colors[tess.numVertexes]);
					tess.indexes[tess.numIndexes++] = tess.numVertexes;
					tess.numVertexes++;

					VectorCopy(farCorners[(j + 1) % 4], tess.xyz[tess.numVertexes]);
					Vector4Copy(colorCyan, tess.colors[tess.numVertexes]);
					tess.indexes[tess.numIndexes++] = tess.numVertexes;
					tess.numVertexes++;

					VectorCopy(top, tess.xyz[tess.numVertexes]);
					Vector4Copy(colorCyan, tess.colors[tess.numVertexes]);
					tess.indexes[tess.numIndexes++] = tess.numVertexes;
					tess.numVertexes++;
				}

				Vector4Set(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
				Vector4Set(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
				Vector4Set(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
				Vector4Set(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
				Tess_AddQuadStamp2(quadVerts, colorRed);
			}

			Tess_UpdateVBOs(tess.attribsSet); // set by Tess_AddQuadStamp2
			Tess_DrawElements();
			break;
		}
		default:
			break;
		}
	}

	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;

	GL_CheckErrors();
}

/**
 * @brief IssueLightOcclusionQuery
 * @param[in] queue
 * @param[in] light
 * @param[in] resetMultiQueryLink
 */
static void IssueLightOcclusionQuery(link_t *queue, trRefLight_t *light, qboolean resetMultiQueryLink)
{
	Ren_LogComment("--- IssueLightOcclusionQuery ---\n");

	//Ren_Print("--- IssueOcclusionQuery(%i) ---\n", node - tr.world->nodes);

	if (tr.numUsedOcclusionQueryObjects < (MAX_OCCLUSION_QUERIES - 1))
	{
		light->occlusionQueryObject = tr.occlusionQueryObjects[tr.numUsedOcclusionQueryObjects++];
	}
	else
	{
		light->occlusionQueryObject = 0;
	}

	EnQueue(queue, light);

	// tell GetOcclusionQueryResult that this is not a multi query
	if (resetMultiQueryLink)
	{
		QueueInit(&light->multiQuery);
	}

	if (light->occlusionQueryObject > 0)
	{
		GL_CheckErrors();

		// begin the occlusion query
		glBeginQuery(GL_SAMPLES_PASSED, light->occlusionQueryObject);

		GL_CheckErrors();

		RenderLightOcclusionVolume(light);

		// end the query
		glEndQuery(GL_SAMPLES_PASSED);

		if (!glIsQuery(light->occlusionQueryObject))
		{
			Ren_Fatal("IssueLightOcclusionQuery: light %li has no occlusion query object in slot %i: %lu", (long)(light - tr.world->lights), backEnd.viewParms.viewCount, (unsigned long)light->occlusionQueryObject);
		}

		//light->occlusionQueryNumbers[backEnd.viewParms.viewCount] = backEnd.pc.c_occlusionQueries;
		backEnd.pc.c_occlusionQueries++;
	}

	GL_CheckErrors();
}

/**
 * @brief IssueLightMultiOcclusionQueries
 * @param[in] multiQueue
 * @param[in] individualQueue
 */
static void IssueLightMultiOcclusionQueries(link_t *multiQueue, link_t *individualQueue)
{
	trRefLight_t *light;
	trRefLight_t *multiQueryLight;
	link_t       *l;

	Ren_LogComment("--- IssueLightMultiOcclusionQueries ---\n");

#if 0
	Ren_Print("IssueLightMultiOcclusionQueries(");
	for (l = multiQueue->prev; l != multiQueue; l = l->prev)
	{
		light = (trRefLight_t *) l->data;

		Ren_Print("%i, ", light - backEnd.refdef.lights);
	}
	Ren_Print(")\n");
#endif

	if (QueueEmpty(multiQueue))
	{
		return;
	}

	multiQueryLight = (trRefLight_t *) QueueFront(multiQueue)->data;

	if (tr.numUsedOcclusionQueryObjects < (MAX_OCCLUSION_QUERIES - 1))
	{
		multiQueryLight->occlusionQueryObject = tr.occlusionQueryObjects[tr.numUsedOcclusionQueryObjects++];
	}
	else
	{
		multiQueryLight->occlusionQueryObject = 0;
	}

	if (multiQueryLight->occlusionQueryObject > 0)
	{
		// begin the occlusion query
		GL_CheckErrors();

		glBeginQuery(GL_SAMPLES_PASSED, multiQueryLight->occlusionQueryObject);

		GL_CheckErrors();

		//Ren_Print("rendering nodes:[");
		for (l = multiQueue->prev; l != multiQueue; l = l->prev)
		{
			light = (trRefLight_t *) l->data;

			//Ren_Print("%i, ", light - backEnd.refdef.lights);

			RenderLightOcclusionVolume(light);
		}
		//Ren_Print("]\n");

		backEnd.pc.c_occlusionQueries++;
		backEnd.pc.c_occlusionQueriesMulti++;

		// end the query
		glEndQuery(GL_SAMPLES_PASSED);

		GL_CheckErrors();

#if 0
		if (!glIsQuery(multiQueryNode->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			Ren_Fatal("IssueMultiOcclusionQueries: node %i has no occlusion query object in slot %i: %i", multiQueryNode - tr.world->nodes, backEnd.viewParms.viewCount, multiQueryNode->occlusionQueryObjects[backEnd.viewParms.viewCount]);
		}
#endif
	}

	// move queue to node->multiQuery queue
	QueueInit(&multiQueryLight->multiQuery);
	DeQueue(multiQueue);
	while (!QueueEmpty(multiQueue))
	{
		light = (trRefLight_t *) DeQueue(multiQueue);
		EnQueue(&multiQueryLight->multiQuery, light);
	}

	EnQueue(individualQueue, multiQueryLight);

	//Ren_Print("--- IssueMultiOcclusionQueries end ---\n");
}

/**
 * @brief LightOcclusionResultAvailable
 * @param[in] light
 * @return
 */
static int LightOcclusionResultAvailable(trRefLight_t *light)
{
	GLint available;

	if (light->occlusionQueryObject > 0)
	{
		GL_JOIN();

		available = 0;
		//if(glIsQuery(light->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			glGetQueryObjectiv(light->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE, &available);
			GL_CheckErrors();
		}

		return available;
	}

	return qtrue;
}

/**
 * @brief GetLightOcclusionQueryResult
 * @param[in] light
 */
static void GetLightOcclusionQueryResult(trRefLight_t *light)
{
	link_t *l, *sentinel;
	int    ocSamples;
	GLint  available;

	Ren_LogComment("--- GetLightOcclusionQueryResult ---\n");

	if (light->occlusionQueryObject > 0)
	{
		GL_JOIN();

#if 0
		if (!glIsQuery(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			Ren_Fatal("GetOcclusionQueryResult: node %i has no occlusion query object in slot %i: %i", node - tr.world->nodes, backEnd.viewParms.viewCount, node->occlusionQueryObjects[backEnd.viewParms.viewCount]);
		}
#endif

		available = 0;
		while (!available)
		{
			//if(glIsQuery(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				glGetQueryObjectiv(light->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE, &available);
				//GL_CheckErrors();
			}
		}

		backEnd.pc.c_occlusionQueriesAvailable++;

		glGetQueryObjectiv(light->occlusionQueryObject, GL_QUERY_RESULT, &ocSamples);

		//Ren_Print("GetOcclusionQueryResult(%i): available = %i, samples = %i\n", node - tr.world->nodes, available, ocSamples);

		GL_CheckErrors();
	}
	else
	{
		ocSamples = 1;
	}

	light->occlusionQuerySamples = ocSamples;

	// copy result to all nodes that were linked to this multi query node
	sentinel = &light->multiQuery;
	for (l = sentinel->prev; l != sentinel; l = l->prev)
	{
		light = (trRefLight_t *) l->data;

		light->occlusionQuerySamples = ocSamples;
	}
}

/**
 * @brief LightCompare
 * @param[in] a
 * @param[in] b
 * @return
 */
static int LightCompare(const void *a, const void *b)
{
	float        d1, d2;
	trRefLight_t *l1 = (trRefLight_t *) *(void **)a;
	trRefLight_t *l2 = (trRefLight_t *) *(void **)b;

	d1 = DistanceSquared(backEnd.viewParms.orientation.origin, l1->l.origin);
	d2 = DistanceSquared(backEnd.viewParms.orientation.origin, l2->l.origin);

	if (d1 < d2)
	{
		return -1;
	}
	if (d1 > d2)
	{
		return 1;
	}

	return 0;
}

/**
 * @brief RB_RenderLightOcclusionQueries
 */
void RB_RenderLightOcclusionQueries()
{
	Ren_LogComment("--- RB_RenderLightOcclusionQueries ---\n");

	if (glConfig2.occlusionQueryBits && r_dynamicLightOcclusionCulling->integer && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		int           i;
		interaction_t *ia;
		int           iaCount;
		int           iaFirst;
		trRefLight_t  *light, *oldLight, *multiQueryLight;
		GLint         ocSamples = 0;
		qboolean      queryObjects;
		link_t        occlusionQueryQueue;
		link_t        invisibleQueue;
		growList_t    invisibleList;
		int           startTime = 0;

		glVertexAttrib4f(ATTR_INDEX_COLOR, 1.0f, 0.0f, 0.0f, 0.05f);

		R2_TIMING(RSPEEDS_OCCLUSION_QUERIES)
		{
			startTime = ri.Milliseconds();
		}

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

		GL_Cull(CT_TWO_SIDED);

		GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

		// set uniforms
		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
		SetUniformVec4(UNIFORM_COLOR, colorBlack);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		// don't write to the color buffer or depth buffer
		if (r_showOcclusionQueries->integer)
		{
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		}
		else
		{
			GL_State(GLS_COLORMASK_BITS);
		}

		tr.numUsedOcclusionQueryObjects = 0;
		QueueInit(&occlusionQueryQueue);
		QueueInit(&invisibleQueue);
		Com_InitGrowList(&invisibleList, 100);

		// loop trough all light interactions and render the light OBB for each last interaction
		for (iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions; )
		{
			backEnd.currentLight      = light = ia->light;
			ia->occlusionQuerySamples = 1;

			if (!ia->next)
			{
				// last interaction of current light
				if (!ia->noOcclusionQueries)
				{
					Com_AddToGrowList(&invisibleList, light);
				}

				if (iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		// sort lights by distance
		qsort(invisibleList.elements, invisibleList.currentElements, sizeof(void *), LightCompare);

		for (i = 0; i < invisibleList.currentElements; i++)
		{
			light = (trRefLight_t *) Com_GrowListElement(&invisibleList, i);

			EnQueue(&invisibleQueue, light);

			if ((invisibleList.currentElements - i) <= 100)
			{
				if (QueueSize(&invisibleQueue) >= 10)
				{
					IssueLightMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);
				}
			}
			else
			{
				if (QueueSize(&invisibleQueue) >= 50)
				{
					IssueLightMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);
				}
			}
		}
		Com_DestroyGrowList(&invisibleList);

		if (!QueueEmpty(&invisibleQueue))
		{
			// remaining previously invisible node queries
			IssueLightMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);

			//Ren_Print("occlusionQueryQueue.empty() = %i\n", QueueEmpty(&occlusionQueryQueue));
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

		while (!QueueEmpty(&occlusionQueryQueue))
		{
			if (LightOcclusionResultAvailable((trRefLight_t *) QueueFront(&occlusionQueryQueue)->data) > 0)
			{
				light = (trRefLight_t *) DeQueue(&occlusionQueryQueue);

				// wait if result not available
				GetLightOcclusionQueryResult(light);

				if (light->occlusionQuerySamples > r_chcVisibilityThreshold->integer)
				{
					// if a query of multiple previously invisible objects became visible, we need to
					// test all the individual objects ...
					if (!QueueEmpty(&light->multiQuery))
					{
						multiQueryLight = light;

						IssueLightOcclusionQuery(&occlusionQueryQueue, multiQueryLight, qfalse);

						while (!QueueEmpty(&multiQueryLight->multiQuery))
						{
							light = (trRefLight_t *) DeQueue(&multiQueryLight->multiQuery);

							IssueLightOcclusionQuery(&occlusionQueryQueue, light, qtrue);
						}
					}
				}
				else
				{
					if (!QueueEmpty(&light->multiQuery))
					{
						backEnd.pc.c_occlusionQueriesLightsCulled++;

						multiQueryLight = light;
						while (!QueueEmpty(&multiQueryLight->multiQuery))
						{
							DeQueue(&multiQueryLight->multiQuery);

							backEnd.pc.c_occlusionQueriesLightsCulled++;
							backEnd.pc.c_occlusionQueriesSaved++;
						}
					}
					else
					{
						backEnd.pc.c_occlusionQueriesLightsCulled++;
					}
				}
			}
		}

		R2_TIMING(RSPEEDS_OCCLUSION_QUERIES)
		{
			backEnd.pc.c_occlusionQueriesResponseTime = ri.Milliseconds() - startTime;
			startTime                                 = ri.Milliseconds();
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

		// reenable writes to depth and color buffers
		GL_State(GLS_DEPTHMASK_TRUE);

		// loop trough all light interactions and fetch results for each last interaction
		// then copy result to all other interactions that belong to the same light
		iaFirst      = 0;
		queryObjects = qtrue;
		oldLight     = NULL;
		for (iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions; )
		{
			backEnd.currentLight = light = ia->light;

			if (light != oldLight)
			{
				iaFirst = iaCount;
			}

			if (!queryObjects)
			{
				ia->occlusionQuerySamples = ocSamples;

				if (ocSamples <= 0)
				{
					backEnd.pc.c_occlusionQueriesInteractionsCulled++;
				}
			}

			if (!ia->next)
			{
				if (queryObjects)
				{
					if (!ia->noOcclusionQueries)
					{
						ocSamples = light->occlusionQuerySamples > r_chcVisibilityThreshold->integer;
					}
					else
					{
						ocSamples = 1;
					}

					// jump back to first interaction of this light copy query result
					ia           = &backEnd.viewParms.interactions[iaFirst];
					iaCount      = iaFirst;
					queryObjects = qfalse;
				}
				else
				{
					if (iaCount < (backEnd.viewParms.numInteractions - 1))
					{
						// jump to next interaction and start querying
						ia++;
						iaCount++;
						queryObjects = qtrue;
					}
					else
					{
						// increase last time to leave for loop
						iaCount++;
					}
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}

			oldLight = light;
		}

		R2_TIMING(RSPEEDS_OCCLUSION_QUERIES)
		{
			backEnd.pc.c_occlusionQueriesFetchTime = ri.Milliseconds() - startTime;
		}
	}

	GL_CheckErrors();
}

// ================================================================================================
// ENTITY OCCLUSION CULLING
// ================================================================================================

/**
 * @brief RenderEntityOcclusionVolume
 * @param[in] entity
 */
static void RenderEntityOcclusionVolume(trRefEntity_t *entity)
{
	vec3_t boundsCenter;
	vec3_t boundsSize;
	mat4_t rot; // transform, scale,
	axis_t axis;

	GL_CheckErrors();

#if 0
	// render in entity space
	R_RotateEntityForViewParms(entity, &backEnd.viewParms, &backEnd.orientation);
	GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
	gl_genericShader->SetUniform_ModelViewProjectionMatrix(GLSTACK_MVPM);

	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;

	Tess_AddCube(vec3_origin, entity->localBounds[0], entity->localBounds[1], colorBlue);

	Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
	Tess_DrawElements();

	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;
#else

#if 0
	VectorSubtract(entity->localBounds[1], entity->localBounds[0], boundsSize);
#else
	boundsSize[0] = Q_fabs(entity->localBounds[0][0]) + Q_fabs(entity->localBounds[1][0]);
	boundsSize[1] = Q_fabs(entity->localBounds[0][1]) + Q_fabs(entity->localBounds[1][1]);
	boundsSize[2] = Q_fabs(entity->localBounds[0][2]) + Q_fabs(entity->localBounds[1][2]);
#endif

	VectorScale(entity->e.axis[0], boundsSize[0] * 0.5f, axis[0]);
	VectorScale(entity->e.axis[1], boundsSize[1] * 0.5f, axis[1]);
	VectorScale(entity->e.axis[2], boundsSize[2] * 0.5f, axis[2]);

	VectorAdd(entity->localBounds[0], entity->localBounds[1], boundsCenter);
	VectorScale(boundsCenter, 0.5f, boundsCenter);

	MatrixFromVectorsFLU(rot, entity->e.axis[0], entity->e.axis[1], entity->e.axis[2]);
	MatrixTransformNormal2(rot, boundsCenter);

	VectorAdd(entity->e.origin, boundsCenter, boundsCenter);

	MatrixSetupTransformFromVectorsFLU(backEnd.orientation.transformMatrix, axis[0], axis[1], axis[2], boundsCenter);

	MatrixAffineInverse(backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
	mat4_mult(backEnd.viewParms.world.viewMatrix, backEnd.orientation.transformMatrix, backEnd.orientation.modelViewMatrix);

	GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	R_BindVBO(tr.unitCubeVBO);
	R_BindIBO(tr.unitCubeIBO);

	GLSL_VertexAttribsState(ATTR_POSITION);

	tess.multiDrawPrimitives = 0;
	tess.numVertexes         = tr.unitCubeVBO->vertexesNum;
	tess.numIndexes          = tr.unitCubeIBO->indexesNum;

	Tess_DrawElements();

	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;

#endif

	GL_CheckErrors();
}

/**
 * @brief IssueEntityOcclusionQuery
 * @param[in] queue
 * @param[in,out] entity
 * @param[in] resetMultiQueryLink
 */
static void IssueEntityOcclusionQuery(link_t *queue, trRefEntity_t *entity, qboolean resetMultiQueryLink)
{
	Ren_LogComment("--- IssueEntityOcclusionQuery ---\n");

	//Ren_Print("--- IssueEntityOcclusionQuery(%i) ---\n", light - backEnd.refdef.lights);

	if (tr.numUsedOcclusionQueryObjects < (MAX_OCCLUSION_QUERIES - 1))
	{
		entity->occlusionQueryObject = tr.occlusionQueryObjects[tr.numUsedOcclusionQueryObjects++];
	}
	else
	{
		entity->occlusionQueryObject = 0;
	}

	EnQueue(queue, entity);

	// tell GetOcclusionQueryResult that this is not a multi query
	if (resetMultiQueryLink)
	{
		QueueInit(&entity->multiQuery);
	}

	if (entity->occlusionQueryObject > 0)
	{
		GL_CheckErrors();

		// begin the occlusion query
		glBeginQuery(GL_SAMPLES_PASSED, entity->occlusionQueryObject);

		GL_CheckErrors();

		RenderEntityOcclusionVolume(entity);

		// end the query
		glEndQuery(GL_SAMPLES_PASSED);

#if 0
		if (!glIsQuery(entity->occlusionQueryObject))
		{
			Ren_Fatal("IssueOcclusionQuery: entity %i has no occlusion query object in slot %i: %i", light - tr.world->lights, backEnd.viewParms.viewCount, light->occlusionQueryObject);
		}
#endif
		backEnd.pc.c_occlusionQueries++;
	}

	GL_CheckErrors();
}

/**
 * @brief RB_RenderHDRResultToFrameBuffer
 */
static void RB_RenderHDRResultToFrameBuffer()
{
	Ren_LogComment("--- RB_RenderHDRResultToFrameBuffer ---\n");
	if (!r_hdrRendering->integer || !glConfig2.framebufferObjectAvailable || !glConfig2.textureFloatAvailable)
	{
		return;
	}

	GL_CheckErrors();
	R_BindNullFBO();

	// bind u_CurrentMap
	SelectTexture(TEX_CURRENT);
	GL_Bind(tr.deferredRenderFBOImage);

	GL_State(GLS_DEPTHTEST_DISABLE);
	GL_Cull(CT_TWO_SIDED);

	// set uniforms
	// set 2D virtual screen size
	GL_PushMatrix();
	RB_SetViewMVPM();

	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		SetMacrosAndSelectProgram(trProg.gl_screenShader);
		glVertexAttrib4fv(ATTR_INDEX_COLOR, colorWhite);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	}
	else
	{
		SetMacrosAndSelectProgram(trProg.gl_toneMappingShader, BRIGHTPASS_FILTER, qfalse);
		SetUniformFloat(UNIFORM_HDRKEY, backEnd.hdrKey);
		SetUniformFloat(UNIFORM_HDRAVERAGELUMINANCE, backEnd.hdrAverageLuminance);
		SetUniformFloat(UNIFORM_HDRMAXLUMINANCE, backEnd.hdrMaxLuminance);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	}

	DRAWVIEWQUAD();
	GL_PopMatrix();
}

/**
 * @brief IssueEntityMultiOcclusionQueries
 * @param[in] multiQueue
 * @param[in] individualQueue
 */
static void IssueEntityMultiOcclusionQueries(link_t *multiQueue, link_t *individualQueue)
{
	trRefEntity_t *entity;
	trRefEntity_t *multiQueryEntity;
	link_t        *l;

	Ren_LogComment("--- IssueEntityMultiOcclusionQueries ---\n");

#if 0
	Ren_Print("IssueEntityMultiOcclusionQueries(");
	for (l = multiQueue->prev; l != multiQueue; l = l->prev)
	{
		light = (trRefEntity_t *) l->data;

		Ren_Print("%i, ", light - backEnd.refdef.entities);
	}
	Ren_Print(")\n");
#endif

	if (QueueEmpty(multiQueue))
	{
		return;
	}

	multiQueryEntity = (trRefEntity_t *) QueueFront(multiQueue)->data;

	if (tr.numUsedOcclusionQueryObjects < (MAX_OCCLUSION_QUERIES - 1))
	{
		multiQueryEntity->occlusionQueryObject = tr.occlusionQueryObjects[tr.numUsedOcclusionQueryObjects++];
	}
	else
	{
		multiQueryEntity->occlusionQueryObject = 0;
	}

	if (multiQueryEntity->occlusionQueryObject > 0)
	{
		// begin the occlusion query
		GL_CheckErrors();

		glBeginQuery(GL_SAMPLES_PASSED, multiQueryEntity->occlusionQueryObject);

		GL_CheckErrors();

		//Ren_Print("rendering nodes:[");
		for (l = multiQueue->prev; l != multiQueue; l = l->prev)
		{
			entity = (trRefEntity_t *) l->data;

			//Ren_Print("%i, ", light - backEnd.refdef.lights);

			RenderEntityOcclusionVolume(entity);
		}
		//Ren_Print("]\n");

		backEnd.pc.c_occlusionQueries++;
		backEnd.pc.c_occlusionQueriesMulti++;

		// end the query
		glEndQuery(GL_SAMPLES_PASSED);

		GL_CheckErrors();

#if 0
		if (!glIsQuery(multiQueryNode->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			Ren_Fatal("IssueEntityMultiOcclusionQueries: node %i has no occlusion query object in slot %i: %i", multiQueryNode - tr.world->nodes, backEnd.viewParms.viewCount, multiQueryNode->occlusionQueryObjects[backEnd.viewParms.viewCount]);
		}
#endif
	}

	// move queue to node->multiQuery queue
	QueueInit(&multiQueryEntity->multiQuery);
	DeQueue(multiQueue);
	while (!QueueEmpty(multiQueue))
	{
		entity = (trRefEntity_t *) DeQueue(multiQueue);
		EnQueue(&multiQueryEntity->multiQuery, entity);
	}

	EnQueue(individualQueue, multiQueryEntity);

	//Ren_Print("--- IssueMultiOcclusionQueries end ---\n");
}

/**
 * @brief EntityOcclusionResultAvailable
 * @param[in] entity
 * @return
 */
static int EntityOcclusionResultAvailable(trRefEntity_t *entity)
{
	GLint available;

	if (entity->occlusionQueryObject > 0)
	{
		GL_JOIN();

		available = 0;
		//if(glIsQuery(light->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			glGetQueryObjectiv(entity->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE, &available);
			if (available)
			{
			GL_CheckErrors();
			}
		}

		return available;
	}

	return qtrue;
}

/**
 * @brief GetEntityOcclusionQueryResult
 * @param[in,out] entity
 */
static void GetEntityOcclusionQueryResult(trRefEntity_t *entity)
{
	link_t *l, *sentinel;
	int    ocSamples;
	GLint  available;

	Ren_LogComment("--- GetEntityOcclusionQueryResult ---\n");

	if (entity->occlusionQueryObject > 0)
	{
		GL_JOIN();

		available = 0;
		while (!available)
		{
			//if(glIsQuery(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				glGetQueryObjectiv(entity->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE, &available);
				//GL_CheckErrors();
			}
		}

		backEnd.pc.c_occlusionQueriesAvailable++;

		glGetQueryObjectiv(entity->occlusionQueryObject, GL_QUERY_RESULT, &ocSamples);

		//Ren_Print("GetOcclusionQueryResult(%i): available = %i, samples = %i\n", node - tr.world->nodes, available, ocSamples);

		GL_CheckErrors();
	}
	else
	{
		ocSamples = 1;
	}

	entity->occlusionQuerySamples = ocSamples;

	// copy result to all nodes that were linked to this multi query node
	sentinel = &entity->multiQuery;
	for (l = sentinel->prev; l != sentinel; l = l->prev)
	{
		entity = (trRefEntity_t *) l->data;

		entity->occlusionQuerySamples = ocSamples;
	}
}

/**
 * @brief EntityCompare
 * @param[in] a
 * @param[in] b
 * @return
 */
static int EntityCompare(const void *a, const void *b)
{
	float         d1, d2;
	trRefEntity_t *e1 = (trRefEntity_t *) *(void **)a;
	trRefEntity_t *e2 = (trRefEntity_t *) *(void **)b;

	d1 = DistanceSquared(backEnd.viewParms.orientation.origin, e1->e.origin);
	d2 = DistanceSquared(backEnd.viewParms.orientation.origin, e2->e.origin);

	if (d1 < d2)
	{
		return -1;
	}
	if (d1 > d2)
	{
		return 1;
	}

	return 0;
}

/**
 * @brief RB_RenderEntityOcclusionQueries
 */
void RB_RenderEntityOcclusionQueries()
{
	Ren_LogComment("--- RB_RenderEntityOcclusionQueries ---\n");

	if (glConfig2.occlusionQueryBits && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		int           i;
		trRefEntity_t *entity, *multiQueryEntity;
		link_t        occlusionQueryQueue;
		link_t        invisibleQueue;
		growList_t    invisibleList;
		int           startTime = 0;

		glVertexAttrib4f(ATTR_INDEX_COLOR, 1.0f, 0.0f, 0.0f, 0.05f);

		R2_TIMING(RSPEEDS_OCCLUSION_QUERIES)
		{
			startTime = ri.Milliseconds();
		}

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

		GL_Cull(CT_TWO_SIDED);

		GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

		// set uniforms
		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CONST, AGEN_CONST);
		SetUniformVec4(UNIFORM_COLOR, colorBlue);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		// don't write to the color buffer or depth buffer
		if (r_showOcclusionQueries->integer)
		{
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		}
		else
		{
			GL_State(GLS_COLORMASK_BITS);
		}

		tr.numUsedOcclusionQueryObjects = 0;
		QueueInit(&occlusionQueryQueue);
		QueueInit(&invisibleQueue);
		Com_InitGrowList(&invisibleList, 100);

		// loop trough all entities and render the entity OBB
		for (i = 0, entity = backEnd.refdef.entities; i < backEnd.refdef.numEntities; i++, entity++)
		{
			if ((entity->e.renderfx & RF_THIRD_PERSON) && !backEnd.viewParms.isPortal)
			{
				continue;
			}

			if (entity->cull == CULL_OUT)
			{
				continue;
			}

			backEnd.currentEntity = entity;

			entity->occlusionQuerySamples = 1;
			entity->noOcclusionQueries    = qfalse;

			// check if the entity volume clips against the near plane
			if (BoxOnPlaneSide(entity->worldBounds[0], entity->worldBounds[1], &backEnd.viewParms.frustums[0][FRUSTUM_NEAR]) == 3)
			{
				entity->noOcclusionQueries = qtrue;
			}
			else
			{
				Com_AddToGrowList(&invisibleList, entity);
			}
		}

		// sort entities by distance
		qsort(invisibleList.elements, invisibleList.currentElements, sizeof(void *), EntityCompare);

		for (i = 0; i < invisibleList.currentElements; i++)
		{
			entity = (trRefEntity_t *) Com_GrowListElement(&invisibleList, i);

			EnQueue(&invisibleQueue, entity);

			if ((invisibleList.currentElements - i) <= 100)
			{
				if (QueueSize(&invisibleQueue) >= 10)
				{
					IssueEntityMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);
				}
			}
			else
			{
				if (QueueSize(&invisibleQueue) >= 50)
				{
					IssueEntityMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);
				}
			}
		}
		Com_DestroyGrowList(&invisibleList);

		if (!QueueEmpty(&invisibleQueue))
		{
			// remaining previously invisible node queries
			IssueEntityMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);

			//Ren_Print("occlusionQueryQueue.empty() = %i\n", QueueEmpty(&occlusionQueryQueue));
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

		while (!QueueEmpty(&occlusionQueryQueue))
		{
			if (EntityOcclusionResultAvailable((trRefEntity_t *) QueueFront(&occlusionQueryQueue)->data) > 0)
			{
				entity = (trRefEntity_t *) DeQueue(&occlusionQueryQueue);

				// wait if result not available
				GetEntityOcclusionQueryResult(entity);

				if (entity->occlusionQuerySamples > r_chcVisibilityThreshold->integer)
				{
					// if a query of multiple previously invisible objects became visible, we need to
					// test all the individual objects ...
					if (!QueueEmpty(&entity->multiQuery))
					{
						multiQueryEntity = entity;

						IssueEntityOcclusionQuery(&occlusionQueryQueue, multiQueryEntity, qfalse);

						while (!QueueEmpty(&multiQueryEntity->multiQuery))
						{
							entity = (trRefEntity_t *) DeQueue(&multiQueryEntity->multiQuery);

							IssueEntityOcclusionQuery(&occlusionQueryQueue, entity, qtrue);
						}
					}
				}
				else
				{
					if (!QueueEmpty(&entity->multiQuery))
					{
						backEnd.pc.c_occlusionQueriesEntitiesCulled++;

						multiQueryEntity = entity;
						while (!QueueEmpty(&multiQueryEntity->multiQuery))
						{
							DeQueue(&multiQueryEntity->multiQuery);

							backEnd.pc.c_occlusionQueriesEntitiesCulled++;
							backEnd.pc.c_occlusionQueriesSaved++;
						}
					}
					else
					{
						backEnd.pc.c_occlusionQueriesEntitiesCulled++;
					}
				}
			}
		}

		R2_TIMING(RSPEEDS_OCCLUSION_QUERIES)
		{
			backEnd.pc.c_occlusionQueriesResponseTime = ri.Milliseconds() - startTime;
			startTime                                 = ri.Milliseconds();  // FIXME: never read
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

		// reenable writes to depth and color buffers
		GL_State(GLS_DEPTHMASK_TRUE);
	}

	GL_CheckErrors();
}

// ================================================================================================
// BSP OCCLUSION CULLING
// ================================================================================================

/**
 * @brief RB_RenderBspOcclusionQueries
 */
void RB_RenderBspOcclusionQueries()
{
	Ren_LogComment("--- RB_RenderBspOcclusionQueries ---\n");

	if (glConfig2.occlusionQueryBits && r_dynamicBspOcclusionCulling->integer)
	{
		//int             j;
		bspNode_t *node;
		link_t    *l, *next, *sentinel;

		SetMacrosAndSelectProgram(trProg.gl_genericShader);
		GL_Cull(CT_TWO_SIDED);

		GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

		// set uniforms
		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CONST, AGEN_CONST);
		SetUniformVec4(UNIFORM_COLOR, colorBlue);
		GLSL_SetUniform_DeformParms(tess.surfaceShader->deforms, tess.surfaceShader->numDeforms);
		SetUniformInt(UNIFORM_ALPHATEST, 0);

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);
		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		// don't write to the color buffer or depth buffer
		GL_State(GLS_COLORMASK_BITS);

		sentinel = &tr.occlusionQueryList;
		for (l = sentinel->next; l != sentinel; l = next)
		{
			next = l->next;
			node = (bspNode_t *) l->data;

			// begin the occlusion query
			glBeginQuery(GL_SAMPLES_PASSED, node->occlusionQueryObjects[backEnd.viewParms.viewCount]);

			R_BindVBO(node->volumeVBO);
			R_BindIBO(node->volumeIBO);

			GLSL_VertexAttribsState(ATTR_POSITION);

			tess.numVertexes = node->volumeVerts;
			tess.numIndexes  = node->volumeIndexes;

			Tess_DrawElements();

			// end the query
			// don't read back immediately so that we give the query time to be ready
			glEndQuery(GL_SAMPLES_PASSED);

#if 0
			if (!glIsQuery(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				Ren_Fatal("node %i has no occlusion query object in slot %i: %i", j, 0, node->occlusionQueryObjects[backEnd.viewParms.viewCount]);
			}
#endif

			backEnd.pc.c_occlusionQueries++;

			tess.multiDrawPrimitives = 0;
			tess.numIndexes          = 0;
			tess.numVertexes         = 0;
		}
	}

	GL_CheckErrors();
}

/**
 * @brief RB_CollectBspOcclusionQueries
 */
void RB_CollectBspOcclusionQueries()
{
	Ren_LogComment("--- RB_CollectBspOcclusionQueries ---\n");

	if (glConfig2.occlusionQueryBits && r_dynamicBspOcclusionCulling->integer)
	{
		// int       j = 0;
		bspNode_t *node;
		link_t    *l, *sentinel;
		int       ocCount = 0;
		int       avCount = 0;
		GLint     available;

		GL_JOIN();

		sentinel = &tr.occlusionQueryList;
		for (l = sentinel->next; l != sentinel; l = l->next)
		{
			node = (bspNode_t *) l->data;

			if (glIsQuery(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				ocCount++;
			}
		}

		//Ren_Print("waiting for %i queries...\n", ocCount);

		do
		{
			for (l = sentinel->next; l != sentinel; l = l->next)
			{
				node = (bspNode_t *) l->data;

				// FIXME: don't know if it should done like this,
				// but this statement was always true.
				// if (node->issueOcclusionQuery)
				// TODO: issueOcclusionQuery is never used
				if (node->issueOcclusionQuery[backEnd.viewParms.viewCount])
				{
					available = 0;
					if (glIsQuery(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
					{
						glGetQueryObjectiv(node->occlusionQueryObjects[backEnd.viewParms.viewCount], GL_QUERY_RESULT_AVAILABLE, &available);
						GL_CheckErrors();
					}

					if (available)
					{
						// FIXME: j is neither incremented not used,
						// node->issueOcclusionQuery[j] = qfalse;
						// TODO: issueOcclusionQuery is never used
						node->issueOcclusionQuery[backEnd.viewParms.viewCount] = qfalse;
						avCount++;

						//if(//avCount % oc)

						//Ren_Print("%i queries...\n", avCount);
					}
				}
			}
		}
		while (avCount < ocCount);

		for (l = sentinel->next; l != sentinel; l = l->next)
		{
			node = (bspNode_t *) l->data;

			available = 0;
			if (glIsQuery(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				glGetQueryObjectiv(node->occlusionQueryObjects[backEnd.viewParms.viewCount], GL_QUERY_RESULT_AVAILABLE, &available);
				GL_CheckErrors();
			}

			if (available)
			{
				backEnd.pc.c_occlusionQueriesAvailable++;

				// get the object and store it in the occlusion bits for the light
				glGetQueryObjectiv(node->occlusionQueryObjects[backEnd.viewParms.viewCount], GL_QUERY_RESULT, &node->occlusionQuerySamples[backEnd.viewParms.viewCount]);

				if (node->occlusionQuerySamples[backEnd.viewParms.viewCount] <= 0)
				{
					backEnd.pc.c_occlusionQueriesLeafsCulled++;
				}
			}
			else
			{
				node->occlusionQuerySamples[backEnd.viewParms.viewCount] = 1;
			}

			GL_CheckErrors();
		}

		//Ren_Print("done\n");
	}
}

/**
 * @brief RB_RenderDebugUtils
 */
static void RB_RenderDebugUtils()
{
	Ren_LogComment("--- RB_RenderDebugUtils ---\n");

	if (r_showLightTransforms->integer || r_showShadowLod->integer)
	{
		interaction_t *ia;
		int           iaCount, j;
		trRefLight_t  *light;
		vec3_t        forward, left, up;
		vec4_t        lightColor;
		vec4_t        quadVerts[4];
		vec3_t        minSize = { -2, -2, -2 };
		vec3_t        maxSize = { 2, 2, 2 };

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		//GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CUSTOM_RGB, AGEN_CUSTOM);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		for (iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions; )
		{
			light = ia->light;

			if (!ia->next)
			{
				if (r_showShadowLod->integer)
				{
					if (light->shadowLOD == 0)
					{
						Vector4Copy(colorRed, lightColor);
					}
					else if (light->shadowLOD == 1)
					{
						Vector4Copy(colorGreen, lightColor);
					}
					else if (light->shadowLOD == 2)
					{
						Vector4Copy(colorBlue, lightColor);
					}
					else if (light->shadowLOD == 3)
					{
						Vector4Copy(colorYellow, lightColor);
					}
					else if (light->shadowLOD == 4)
					{
						Vector4Copy(colorMagenta, lightColor);
					}
					else if (light->shadowLOD == 5)
					{
						Vector4Copy(colorCyan, lightColor);
					}
					else
					{
						Vector4Copy(colorMdGrey, lightColor);
					}
				}
				else if (r_dynamicLightOcclusionCulling->integer)
				{
					if (!ia->occlusionQuerySamples)
					{
						Vector4Copy(colorRed, lightColor);
					}
					else
					{
						Vector4Copy(colorGreen, lightColor);
					}
				}
				else
				{
					//Vector4Copy(g_color_table[iaCount % 8], lightColor);
					Vector4Copy(colorBlue, lightColor);
				}

				lightColor[3] = 0.2f;

				SetUniformVec4(UNIFORM_COLOR, lightColor);

				MatrixToVectorsFLU(matrixIdentity, forward, left, up);
				VectorMA(vec3_origin, 16, forward, forward);
				VectorMA(vec3_origin, 16, left, left);
				VectorMA(vec3_origin, 16, up, up);

				/*
				// draw axis
				glBegin(GL_LINES);

				// draw orientation
				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorRed);
				glVertex3fv(vec3_origin);
				glVertex3fv(forward);

				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorGreen);
				glVertex3fv(vec3_origin);
				glVertex3fv(left);

				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorBlue);
				glVertex3fv(vec3_origin);
				glVertex3fv(up);

				// draw special vectors
				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorYellow);
				glVertex3fv(vec3_origin);
				VectorSubtract(light->origin, backEnd.orientation.origin, tmp);
				light->transformed[0] = DotProduct(tmp, backEnd.orientation.axis[0]);
				light->transformed[1] = DotProduct(tmp, backEnd.orientation.axis[1]);
				light->transformed[2] = DotProduct(tmp, backEnd.orientation.axis[2]);
				glVertex3fv(light->transformed);

				glEnd();
				*/

#if 1
				if (light->isStatic && light->frustumVBO && light->frustumIBO)
				{
					// go back to the world modelview matrix
					backEnd.orientation = backEnd.viewParms.world;
					GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

					SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

					R_BindVBO(light->frustumVBO);
					R_BindIBO(light->frustumIBO);

					GLSL_VertexAttribsState(ATTR_POSITION);

					tess.numVertexes = light->frustumVerts;
					tess.numIndexes  = light->frustumIndexes;

					Tess_DrawElements();

					tess.multiDrawPrimitives = 0;
					tess.numIndexes          = 0;
					tess.numVertexes         = 0;
				}
				else
#endif
				{
					tess.multiDrawPrimitives = 0;
					tess.numIndexes          = 0;
					tess.numVertexes         = 0;

					switch (light->l.rlType)
					{
					case RL_OMNI:
					case RL_DIRECTIONAL:
					{
#if 1
						// set up the transformation matrix
						R_RotateLightForViewParms(light, &backEnd.viewParms, &backEnd.orientation);

						GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

						SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

						Tess_AddCube(vec3_origin, light->localBounds[0], light->localBounds[1], lightColor);

						if (!VectorCompare(light->l.center, vec3_origin))
						{
							Tess_AddCube(light->l.center, minSize, maxSize, colorYellow);
						}

						Tess_UpdateVBOs(tess.attribsSet); // set by Tess_AddCube
						Tess_DrawElements();
#else
						mat4_t transform, scale, rot;

						mat4_reset_scale(scale, light->l.radius[0], light->l.radius[1], light->l.radius[2]);
						mat4_mult(light->transformMatrix, scale, transform);

						GL_LoadModelViewMatrix(transform);
						//GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

						gl_genericShader->SetUniform_ModelViewProjectionMatrix(GLSTACK_MVPM);

						R_BindVBO(tr.unitCubeVBO);
						R_BindIBO(tr.unitCubeIBO);

						GLSL_VertexAttribsState(ATTR_POSITION);

						tess.multiDrawPrimitives = 0;
						tess.numVertexes         = tr.unitCubeVBO->vertexesNum;
						tess.numIndexes          = tr.unitCubeIBO->indexesNum;

						Tess_DrawElements();
#endif
						break;
					}
					case RL_PROJ:
					{
						vec3_t farCorners[4];
						//vec4_t			frustum[6];
						vec4_t *frustum = light->localFrustum;

						// set up the transformation matrix
						R_RotateLightForViewParms(light, &backEnd.viewParms, &backEnd.orientation);
						GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

						SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

#if 0
						// transform frustum from world space to local space
						for (j = 0; j < 6; j++)
						{
							MatrixTransformPlane(light->transformMatrix, light->localFrustum[j], frustum[j]);
							//Vector4Copy(light->localFrustum[j], frustum[j]);
							//MatrixTransformPlane2(light->viewMatrix, frustum[j]);
						}

						// go back to the world modelview matrix
						backEnd.orientation = backEnd.viewParms.world;
						GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
						GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericShader, GLSTACK_MVPM);
#endif

						PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[0]);
						PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[1]);
						PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[2]);
						PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[3]);

						// the planes of the frustum are measured at world 0,0,0 so we have to position the intersection points relative to the light origin
#if 0
						Ren_Print("pyramid farCorners\n");
						for (j = 0; j < 4; j++)
						{
							Ren_Print("(%5.3f, %5.3f, %5.3f)\n", farCorners[j][0], farCorners[j][1], farCorners[j][2]);
						}
#endif

						tess.numVertexes         = 0;
						tess.numIndexes          = 0;
						tess.multiDrawPrimitives = 0;

						if (!VectorCompare(light->l.projStart, vec3_origin))
						{
							vec3_t nearCorners[4];

							// calculate the vertices defining the top area
							PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[0]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[1]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[2]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[3]);

#if 0
							Ren_Print("pyramid nearCorners\n");
							for (j = 0; j < 4; j++)
							{
								Ren_Print("(%5.3f, %5.3f, %5.3f)\n", nearCorners[j][0], nearCorners[j][1], nearCorners[j][2]);
							}
#endif

							// draw outer surfaces
							for (j = 0; j < 4; j++)
							{
								Vector4Set(quadVerts[3], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
								Vector4Set(quadVerts[2], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
								Vector4Set(quadVerts[1], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
								Vector4Set(quadVerts[0], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
								Tess_AddQuadStamp2(quadVerts, lightColor);
							}

							// draw far cap
							Vector4Set(quadVerts[0], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
							Vector4Set(quadVerts[1], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
							Vector4Set(quadVerts[2], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
							Vector4Set(quadVerts[3], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
							Tess_AddQuadStamp2(quadVerts, lightColor);

							// draw near cap
							Vector4Set(quadVerts[3], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
							Vector4Set(quadVerts[2], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
							Vector4Set(quadVerts[1], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
							Vector4Set(quadVerts[0], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
							Tess_AddQuadStamp2(quadVerts, lightColor);
						}
						else
						{
							vec3_t top;

							// no light_start, just use the top vertex (doesn't need to be mirrored)
							PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], top);

							// draw pyramid
							for (j = 0; j < 4; j++)
							{
								VectorCopy(top, tess.xyz[tess.numVertexes]);
								Vector4Copy(lightColor, tess.colors[tess.numVertexes]);
								tess.indexes[tess.numIndexes++] = tess.numVertexes;
								tess.numVertexes++;

								VectorCopy(farCorners[(j + 1) % 4], tess.xyz[tess.numVertexes]);
								Vector4Copy(lightColor, tess.colors[tess.numVertexes]);
								tess.indexes[tess.numIndexes++] = tess.numVertexes;
								tess.numVertexes++;

								VectorCopy(farCorners[j], tess.xyz[tess.numVertexes]);
								Vector4Copy(lightColor, tess.colors[tess.numVertexes]);
								tess.indexes[tess.numIndexes++] = tess.numVertexes;
								tess.numVertexes++;
							}

							// draw far cap
							Vector4Set(quadVerts[0], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
							Vector4Set(quadVerts[1], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
							Vector4Set(quadVerts[2], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
							Vector4Set(quadVerts[3], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
							Tess_AddQuadStamp2(quadVerts, lightColor);
						}

						// draw light_target
						Tess_AddCube(light->l.projTarget, minSize, maxSize, colorRed);
						Tess_AddCube(light->l.projRight, minSize, maxSize, colorGreen);
						Tess_AddCube(light->l.projUp, minSize, maxSize, colorBlue);

						if (!VectorCompare(light->l.projStart, vec3_origin))
						{
							Tess_AddCube(light->l.projStart, minSize, maxSize, colorYellow);
						}

						if (!VectorCompare(light->l.projEnd, vec3_origin))
						{
							Tess_AddCube(light->l.projEnd, minSize, maxSize, colorMagenta);
						}

						Tess_UpdateVBOs(tess.attribsSet); // set by Tess_AddCube
						Tess_DrawElements();
						break;
					}
					default:
						break;
					}

					tess.multiDrawPrimitives = 0;
					tess.numIndexes          = 0;
					tess.numVertexes         = 0;
				}

				if (iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	if (r_showLightInteractions->integer)
	{
		int           i;
		int           cubeSides;
		interaction_t *ia;
		int           iaCount;
		trRefLight_t  *light;
		trRefEntity_t *entity;
		surfaceType_t *surface;
		vec4_t        lightColor;
		vec3_t        mins = { -1, -1, -1 };
		vec3_t        maxs = { 1, 1, 1 };

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
		SetUniformVec4(UNIFORM_COLOR, colorBlack);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		for (iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions; )
		{
			backEnd.currentEntity = entity = ia->entity;
			light                 = ia->light;
			surface               = ia->surface;

			if (entity != &tr.worldEntity)
			{
				// set up the transformation matrix
				R_RotateEntityForViewParms(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);
			}
			else
			{
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

			if (r_shadows->integer >= SHADOWING_ESM16 && light->l.rlType == RL_OMNI)
			{
#if 0
				Vector4Copy(colorMdGrey, lightColor);

				if (ia->cubeSideBits & CUBESIDE_PX)
				{
					Vector4Copy(colorBlack, lightColor);
				}
				if (ia->cubeSideBits & CUBESIDE_PY)
				{
					Vector4Copy(colorRed, lightColor);
				}
				if (ia->cubeSideBits & CUBESIDE_PZ)
				{
					Vector4Copy(colorGreen, lightColor);
				}
				if (ia->cubeSideBits & CUBESIDE_NX)
				{
					Vector4Copy(colorYellow, lightColor);
				}
				if (ia->cubeSideBits & CUBESIDE_NY)
				{
					Vector4Copy(colorBlue, lightColor);
				}
				if (ia->cubeSideBits & CUBESIDE_NZ)
				{
					Vector4Copy(colorCyan, lightColor);
				}
				if (ia->cubeSideBits == CUBESIDE_CLIPALL)
				{
					Vector4Copy(colorMagenta, lightColor);
				}
#else
				// count how many cube sides are in use for this interaction
				cubeSides = 0;
				for (i = 0; i < 6; i++)
				{
					if (ia->cubeSideBits & (1 << i))
					{
						cubeSides++;
					}
				}

				Vector4Copy(g_color_table[cubeSides], lightColor);
#endif
			}
			else
			{
				Vector4Copy(colorMdGrey, lightColor);
			}

			lightColor[0] *= 0.5f;
			lightColor[1] *= 0.5f;
			lightColor[2] *= 0.5f;
			//lightColor[3] *= 0.2f;

			Vector4Copy(colorWhite, lightColor);

			tess.numVertexes         = 0;
			tess.numIndexes          = 0;
			tess.multiDrawPrimitives = 0;

			if (*surface == SF_FACE || *surface == SF_GRID || *surface == SF_TRIANGLES)
			{
				srfGeneric_t *gen;

				gen = (srfGeneric_t *) surface;

				if (*surface == SF_FACE)
				{
					Vector4Copy(colorMdGrey, lightColor);
				}
				else if (*surface == SF_GRID)
				{
					Vector4Copy(colorCyan, lightColor);
				}
				else if (*surface == SF_TRIANGLES)
				{
					Vector4Copy(colorMagenta, lightColor);
				}
				else
				{
					Vector4Copy(colorMdGrey, lightColor);
				}

				Tess_AddCube(vec3_origin, gen->bounds[0], gen->bounds[1], lightColor);

				Tess_AddCube(gen->origin, mins, maxs, colorWhite);
			}
			else if (*surface == SF_VBO_MESH)
			{
				srfVBOMesh_t *srf = (srfVBOMesh_t *) surface;
				Tess_AddCube(vec3_origin, srf->bounds[0], srf->bounds[1], lightColor);
			}
			else if (*surface == SF_MDV)
			{
				Tess_AddCube(vec3_origin, entity->localBounds[0], entity->localBounds[1], lightColor);
			}

			Tess_UpdateVBOs(tess.attribsSet); // set by Tess_AddCube
			Tess_DrawElements();

			tess.multiDrawPrimitives = 0;
			tess.numIndexes          = 0;
			tess.numVertexes         = 0;

			if (!ia->next)
			{
				if (iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	if (r_showEntityTransforms->integer)
	{
		trRefEntity_t *ent;
		int           i;
		vec3_t        mins = { -1, -1, -1 };
		vec3_t        maxs = { 1, 1, 1 };

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
		SetUniformVec4(UNIFORM_COLOR, colorBlack);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		ent = backEnd.refdef.entities;
		for (i = 0; i < backEnd.refdef.numEntities; i++, ent++)
		{
			if ((ent->e.renderfx & RF_THIRD_PERSON) && !backEnd.viewParms.isPortal)
			{
				continue;
			}

			if (ent->cull == CULL_OUT)
			{
				continue;
			}

			// set up the transformation matrix
			R_RotateEntityForViewParms(ent, &backEnd.viewParms, &backEnd.orientation);
			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

			//R_DebugAxis(vec3_origin, matrixIdentity);
			//R_DebugBoundingBox(vec3_origin, ent->localBounds[0], ent->localBounds[1], colorMagenta);

			tess.multiDrawPrimitives = 0;
			tess.numIndexes          = 0;
			tess.numVertexes         = 0;

			if (r_dynamicEntityOcclusionCulling->integer)
			{
				if (!ent->occlusionQuerySamples)
				{
					Tess_AddCube(vec3_origin, ent->localBounds[0], ent->localBounds[1], colorRed);
				}
				else
				{
					Tess_AddCube(vec3_origin, ent->localBounds[0], ent->localBounds[1], colorGreen);
				}
			}
			else
			{
				Tess_AddCube(vec3_origin, ent->localBounds[0], ent->localBounds[1], colorBlue);
			}

			Tess_AddCube(vec3_origin, mins, maxs, colorWhite);

			Tess_UpdateVBOs(tess.attribsSet); // set by Tess_AddCube
			Tess_DrawElements();

			tess.multiDrawPrimitives = 0;
			tess.numIndexes          = 0;
			tess.numVertexes         = 0;


			// go back to the world modelview matrix
			//backEnd.orientation = backEnd.viewParms.world;
			//GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

			//R_DebugBoundingBox(vec3_origin, ent->worldBounds[0], ent->worldBounds[1], colorCyan);
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	if (r_showSkeleton->integer)
	{
		int                  i, j, k, parentIndex;
		trRefEntity_t        *ent;
		vec3_t               origin, offset;
		vec3_t               forward, right, up;
		vec3_t               diff, tmp, tmp2, tmp3;
		vec_t                length;
		vec4_t               tetraVerts[4];
		static refSkeleton_t skeleton;
		refSkeleton_t        *skel;

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
		SetUniformVec4(UNIFORM_COLOR, colorBlack);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.charsetImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		ent = backEnd.refdef.entities;
		for (i = 0; i < backEnd.refdef.numEntities; i++, ent++)
		{
			if ((ent->e.renderfx & RF_THIRD_PERSON) && !backEnd.viewParms.isPortal)
			{
				continue;
			}

			// set up the transformation matrix
			R_RotateEntityForViewParms(ent, &backEnd.viewParms, &backEnd.orientation);
			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

			tess.multiDrawPrimitives = 0;
			tess.numVertexes         = 0;
			tess.numIndexes          = 0;

			skel = NULL;
			if (ent->e.skeleton.type == SK_ABSOLUTE)
			{
				skel = &ent->e.skeleton;
			}
			else
			{
				model_t   *model;
				refBone_t *bone;

				model = R_GetModelByHandle(ent->e.hModel);

				if (model)
				{
					switch (model->type)
					{
					case MOD_MD5:
					{
						// copy absolute bones
						skeleton.numBones = model->md5->numBones;
						for (j = 0, bone = &skeleton.bones[0]; j < skeleton.numBones; j++, bone++)
						{
								#if defined(REFBONE_NAMES)
							Q_strncpyz(bone->name, model->md5->bones[j].name, sizeof(bone->name));
								#endif

							bone->parentIndex = model->md5->bones[j].parentIndex;
							VectorCopy(model->md5->bones[j].origin, bone->origin);
							VectorCopy(model->md5->bones[j].rotation, bone->rotation);
						}

						skel = &skeleton;
						break;
					}
					default:
						break;
					}
				}
			}

			if (skel)
			{
				static vec3_t worldOrigins[MAX_BONES];

				GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);

				for (j = 0; j < skel->numBones; j++)
				{
					parentIndex = skel->bones[j].parentIndex;

					if (parentIndex < 0)
					{
						VectorClear(origin);
					}
					else
					{
						VectorCopy(skel->bones[parentIndex].origin, origin);
					}
					VectorCopy(skel->bones[j].origin, offset);
					quat_to_vec3_FRU(skel->bones[j].rotation, forward, right, up);

					VectorSubtract(offset, origin, diff);
					if ((length = VectorNormalize(diff)))
					{
						PerpendicularVector(tmp, diff);
						//VectorCopy(up, tmp);

						VectorScale(tmp, length * 0.1f, tmp2);
						VectorMA(tmp2, length * 0.2f, diff, tmp2);

						for (k = 0; k < 3; k++)
						{
							RotatePointAroundVector(tmp3, diff, tmp2, k * 120);
							VectorAdd(tmp3, origin, tmp3);
							VectorCopy(tmp3, tetraVerts[k]);
							tetraVerts[k][3] = 1;
						}

						VectorCopy(origin, tetraVerts[3]);
						tetraVerts[3][3] = 1;
						Tess_AddTetrahedron(tetraVerts, g_color_table[ColorIndex(j)]);

						VectorCopy(offset, tetraVerts[3]);
						tetraVerts[3][3] = 1;
						Tess_AddTetrahedron(tetraVerts, g_color_table[ColorIndex(j)]);
					}

					mat4_transform_vec3(backEnd.orientation.transformMatrix, skel->bones[j].origin, worldOrigins[j]);
				}

				Tess_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR);

				Tess_DrawElements();

				tess.multiDrawPrimitives = 0;
				tess.numVertexes         = 0;
				tess.numIndexes          = 0;

#if defined(REFBONE_NAMES)
				{
					GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

					// go back to the world modelview matrix
					backEnd.orientation = backEnd.viewParms.world;
					GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
					gl_genericShader->SetUniform_ModelViewProjectionMatrix(GLSTACK_MVPM);

					// draw names
					for (j = 0; j < skel->numBones; j++)
					{
						vec3_t left, up;
						float  radius;
						vec3_t origin;

						// calculate the xyz locations for the four corners
						radius = 0.4;
						VectorScale(backEnd.viewParms.orientation.axis[1], radius, left);
						VectorScale(backEnd.viewParms.orientation.axis[2], radius, up);

						if (backEnd.viewParms.isMirror)
						{
							VectorSubtract(vec3_origin, left, left);
						}

						for (k = 0; k < strlen(skel->bones[j].name); k++)
						{
							int   ch;
							int   row, col;
							float frow, fcol;
							float size;

							ch  = skel->bones[j].name[k];
							ch &= 255;

							if (ch == ' ')
							{
								break;
							}

							row = ch >> 4;
							col = ch & 15;

							frow = row * 0.0625;
							fcol = col * 0.0625;
							size = 0.0625;

							VectorMA(worldOrigins[j], -(k + 2.0f), left, origin);
							Tess_AddQuadStampExt(origin, left, up, colorWhite, fcol, frow, fcol + size, frow + size);
						}

						Tess_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR);

						Tess_DrawElements();

						tess.multiDrawPrimitives = 0;
						tess.numVertexes         = 0;
						tess.numIndexes          = 0;
					}
				}
#endif // REFBONE_NAMES
			}

			tess.multiDrawPrimitives = 0;
			tess.numVertexes         = 0;
			tess.numIndexes          = 0;
		}
	}
#endif

	if (r_showLightScissors->integer)
	{
		interaction_t *ia;
		int           iaCount;
		vec4_t        quadVerts[4];

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CUSTOM_RGB, AGEN_CUSTOM);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		// set 2D virtual screen size
		GL_PushMatrix();
		RB_SetViewMVPM();

		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		for (iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions; )
		{
			if (glConfig2.occlusionQueryBits)
			{
				if (!ia->occlusionQuerySamples)
				{
					SetUniformVec4(UNIFORM_COLOR, colorRed);
				}
				else
				{
					SetUniformVec4(UNIFORM_COLOR, colorGreen);
				}

				Vector4Set(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
				Vector4Set(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
				Vector4Set(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				Vector4Set(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				Tess_InstantQuad(quadVerts);
			}
			else
			{
				SetUniformVec4(UNIFORM_COLOR, colorWhite);

				Vector4Set(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
				Vector4Set(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
				Vector4Set(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				Vector4Set(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				Tess_InstantQuad(quadVerts);
			}

			if (!ia->next)
			{
				if (iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		GL_PopMatrix();
	}

	if (r_showCubeProbes->integer)
	{
		cubemapProbe_t *cubeProbe;
		int            j;
		//vec4_t         quadVerts[4];
		vec3_t mins = { -8, -8, -8 };
		vec3_t maxs = { 8, 8, 8 };
		//vec3_t			viewOrigin;

		if (backEnd.refdef.rdflags & (RDF_NOWORLDMODEL | RDF_NOCUBEMAP))
		{
			return;
		}

		SetMacrosAndSelectProgram(trProg.gl_reflectionShader,
		                          USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal,
		                          USE_VERTEX_SKINNING, qfalse,
		                          USE_VERTEX_ANIMATION, qfalse,
		                          USE_DEFORM_VERTEXES, qfalse,
		                          USE_NORMAL_MAPPING, qfalse);
		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin); // in world space

		GLSL_SetUniform_ColorModulate(trProg.gl_reflectionShader, CGEN_IDENTITY, AGEN_IDENTITY); //CGEN_CUSTOM_RGB, AGEN_CUSTOM);
		//GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_IDENTITY, AGEN_IDENTITY);
		SetUniformVec4(UNIFORM_COLOR, colorBlack);

		//GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);
		GL_State(0);
		GL_Cull(CT_FRONT_SIDED); // the inside of the cube is textured, and the normals all point to the center of the cube: that's the front side (we don't want to see)
		SetUniformInt(UNIFORM_ALPHATEST, ATEST_NONE);

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		//GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
		//SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
		SetUniformMatrix16(UNIFORM_MODELMATRIX, MODEL_MATRIX);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);
		
		GLSL_SetRequiredVertexPointers(trProg.gl_reflectionShader);
		//GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

		for (j = 0; j < tr.cubeProbes.currentElements; j++)
		{
			cubeProbe = (cubemapProbe_t *) Com_GrowListElement(&tr.cubeProbes, j);

			Tess_Begin(Tess_StageIteratorDebug, NULL, NULL, NULL, qtrue, qtrue, LIGHTMAP_NONE, FOG_NONE);
#if 1
            // the glsl shader needs 2 cubemaps and an interpolation factor,
            // but now we just want to render a single cubemap..
            // so we pass on two of the same textures

            // bind u_EnvironmentMap0
            SelectTexture(TEX_ENVMAP0);
            GL_Bind(cubeProbe->cubemap);

            // bind u_EnvironmentMap1
            SelectTexture(TEX_ENVMAP1);
            GL_Bind(cubeProbe->cubemap);

            // u_EnvironmentInterpolation
            SetUniformFloat(UNIFORM_ENVIRONMENTINTERPOLATION, 1.0);
#else
            SelectTexture(TEX_COLOR);
            GL_Bind(cubeProbe->cubemap);
#endif

			Tess_AddCubeWithNormals(cubeProbe->origin, mins, maxs, colorWhite);
			Tess_End();
		}
		//Tess_UpdateVBOs(tess.attribsSet); // set by Tess_AddCube


#if 0	// color the 2 closest cubeProbes (green/red/yellow?/blue?)
		// (disabled because, when you want to inspect a cubeProbe up close, no textures can be seen.. not handy)
		{
			cubemapProbe_t *cubeProbe1;
			cubemapProbe_t *cubeProbe2;
			float          distance1, distance2;

			SetMacrosAndSelectProgram(trProg.gl_genericShader);

			GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
			SetUniformVec4(UNIFORM_COLOR, colorBlack);

			GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

			GL_State(GLS_DEFAULT);
			GL_Cull(CT_TWO_SIDED);

			// set uniforms

			// set up the transformation matrix
			backEnd.orientation = backEnd.viewParms.world;
			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

			// bind u_ColorMap
			SelectTexture(TEX_COLOR);
			GL_Bind(tr.whiteImage);

			SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

			GL_CheckErrors();

			R_FindTwoNearestCubeMaps(backEnd.viewParms.orientation.origin, &cubeProbe1, &cubeProbe2, &distance1, &distance2);

			Tess_Begin(Tess_StageIteratorDebug, NULL, NULL, NULL, qtrue, qfalse, LIGHTMAP_NONE, FOG_NONE);

			if (cubeProbe1 == NULL && cubeProbe2 == NULL)
			{
				// bad
			}
			else if (cubeProbe1 == NULL)
			{
				Tess_AddCubeWithNormals(cubeProbe2->origin, mins, maxs, colorBlue);
			}
			else if (cubeProbe2 == NULL)
			{
				Tess_AddCubeWithNormals(cubeProbe1->origin, mins, maxs, colorYellow);
			}
			else
			{
				Tess_AddCubeWithNormals(cubeProbe1->origin, mins, maxs, colorGreen);
				Tess_AddCubeWithNormals(cubeProbe2->origin, mins, maxs, colorRed);
			}

			Tess_End();
		}
#endif
		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	if (r_showLightGrid->integer)
	{
		bspGridPoint_t *gridPoint;
		int            j, k;
		vec3_t         offset;
		vec3_t         lightDirection;
		vec3_t         tmp, tmp2, tmp3;
		vec_t          length;
		vec4_t         tetraVerts[4];

		if (backEnd.refdef.rdflags & (RDF_NOWORLDMODEL | RDF_NOCUBEMAP))
		{
			return;
		}

		Ren_LogComment("--- r_showLightGrid > 0: Rendering light grid\n");

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
		SetUniformVec4(UNIFORM_COLOR, colorBlack);

		GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

		GL_State(GLS_DEFAULT);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		GL_CheckErrors();

		Tess_Begin(Tess_StageIteratorDebug, NULL, NULL, NULL, qtrue, qfalse, LIGHTMAP_NONE, FOG_NONE);

		for (j = 0; j < tr.world->numLightGridPoints; j++)
		{
			gridPoint = &tr.world->lightGridData[j];

			if (VectorDistanceSquared(gridPoint->origin, backEnd.viewParms.orientation.origin) > Square(1024))
			{
				continue;
			}

			VectorNegate(gridPoint->direction, lightDirection);

			length = 8;
			VectorMA(gridPoint->origin, 8, lightDirection, offset);

			PerpendicularVector(tmp, lightDirection);
			//VectorCopy(up, tmp);

			VectorScale(tmp, length * 0.1f, tmp2);
			VectorMA(tmp2, length * 0.2f, lightDirection, tmp2);

			for (k = 0; k < 3; k++)
			{
				RotatePointAroundVector(tmp3, lightDirection, tmp2, k * 120);
				VectorAdd(tmp3, gridPoint->origin, tmp3);
				VectorCopy(tmp3, tetraVerts[k]);
				tetraVerts[k][3] = 1;
			}

			VectorCopy(gridPoint->origin, tetraVerts[3]);
			tetraVerts[3][3] = 1;
			Tess_AddTetrahedron(tetraVerts, gridPoint->directedColor);

			VectorCopy(offset, tetraVerts[3]);
			tetraVerts[3][3] = 1;
			Tess_AddTetrahedron(tetraVerts, gridPoint->directedColor);
		}

		Tess_End();

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	if (r_showBspNodes->integer)
	{
		bspNode_t *node;
		link_t    *l, *sentinel;
		int       i;

		if ((backEnd.refdef.rdflags & (RDF_NOWORLDMODEL)) || !tr.world)
		{
			return;
		}

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CUSTOM_RGB, AGEN_CUSTOM);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		GL_CheckErrors();

		for (i = 0; i < 2; i++)
		{
			float  x, y, w, h;
			vec4_t quadVerts[4];

			if (i == 1)
			{
				// set 2D virtual screen size
				GL_PushMatrix();
				RB_SetViewMVPM();
				GL_Cull(CT_TWO_SIDED);
				GL_State(GLS_DEPTHTEST_DISABLE);

				SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
				SetUniformVec4(UNIFORM_COLOR, colorBlack);

				w = 300;
				h = 300;

				x = 20;
				y = 90;

				Vector4Set(quadVerts[0], x, y, 0, 1);
				Vector4Set(quadVerts[1], x + w, y, 0, 1);
				Vector4Set(quadVerts[2], x + w, y + h, 0, 1);
				Vector4Set(quadVerts[3], x, y + h, 0, 1);

				Tess_InstantQuad(quadVerts);

				{
					int    j;
					vec4_t splitFrustum[6];
					vec3_t farCorners[4];
					vec3_t nearCorners[4];
					vec3_t cropBounds[2];
					vec4_t point, transf;

					GL_Viewport(x, y, w, h);
					GL_Scissor(x, y, w, h);

					GL_PushMatrix();

					// calculate top down view projection matrix
					{
						vec3_t forward = { 0, 0, -1 };
						vec3_t up      = { 1, 0, 0 };
						mat4_t viewMatrix, projectionMatrix; // rotationMatrix, transformMatrix,

						// Quake -> OpenGL view matrix from light perspective
#if 0
						VectorToAngles(lightDirection, angles);
						MatrixFromAngles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
						MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, backEnd.viewParms.orientation.origin);
						MatrixAffineInverse(transformMatrix, viewMatrix);
						MatrixMultiplyMOD(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);
#else
						MatrixLookAtRH(viewMatrix, backEnd.viewParms.orientation.origin, forward, up);
#endif

						//ClearBounds(splitFrustumBounds[0], splitFrustumBounds[1]);
						//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], backEnd.viewParms.visBounds[0], backEnd.viewParms.visBounds[1]);
						//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], light->worldBounds[0], light->worldBounds[1]);

						ClearBounds(cropBounds[0], cropBounds[1]);
						for (j = 0; j < 8; j++)
						{
							point[0] = tr.world->models[0].bounds[j & 1][0];
							point[1] = tr.world->models[0].bounds[(j >> 1) & 1][1];
							point[2] = tr.world->models[0].bounds[(j >> 2) & 1][2];
							point[3] = 1;

							mat4_transform_vec4(viewMatrix, point, transf);
							transf[0] /= transf[3];
							transf[1] /= transf[3];
							transf[2] /= transf[3];

							AddPointToBounds(transf, cropBounds[0], cropBounds[1]);
						}


						MatrixOrthogonalProjectionRH(projectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2]);

						GL_LoadModelViewMatrix(viewMatrix);
						GL_LoadProjectionMatrix(projectionMatrix);
					}

					// set uniforms
					GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
					SetUniformVec4(UNIFORM_COLOR, colorBlack);

					GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
					GL_Cull(CT_TWO_SIDED);

					// bind u_ColorMap
					SelectTexture(TEX_COLOR);
					GL_Bind(tr.whiteImage);

					SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);
					SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

					tess.multiDrawPrimitives = 0;
					tess.numIndexes          = 0;
					tess.numVertexes         = 0;

					for (j = 0; j < 6; j++)
					{
						VectorCopy(backEnd.viewParms.frustums[0][j].normal, splitFrustum[j]);
						splitFrustum[j][3] = backEnd.viewParms.frustums[0][j].dist;
					}

					// calculate split frustum corner points
					PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], nearCorners[0]);
					PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], nearCorners[1]);
					PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], nearCorners[2]);
					PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], nearCorners[3]);

					PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], farCorners[0]);
					PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], farCorners[1]);
					PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], farCorners[2]);
					PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], farCorners[3]);

					// draw outer surfaces
					for (j = 0; j < 4; j++)
					{
						Vector4Set(quadVerts[0], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
						Vector4Set(quadVerts[1], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
						Vector4Set(quadVerts[2], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
						Vector4Set(quadVerts[3], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
						Tess_AddQuadStamp2(quadVerts, colorCyan);
					}

					// draw far cap
					Vector4Set(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
					Vector4Set(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
					Vector4Set(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
					Vector4Set(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
					Tess_AddQuadStamp2(quadVerts, colorBlue);

					// draw near cap
					Vector4Set(quadVerts[0], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
					Vector4Set(quadVerts[1], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
					Vector4Set(quadVerts[2], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
					Vector4Set(quadVerts[3], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
					Tess_AddQuadStamp2(quadVerts, colorGreen);

					Tess_UpdateVBOs(tess.attribsSet); // set by Tess_AddQuadStamp2
					Tess_DrawElements();

					GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CUSTOM_RGB, AGEN_CUSTOM);
				}
			} // i == 1
			else
			{
				GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
				GL_Cull(CT_TWO_SIDED);

				// render in world space
				backEnd.orientation = backEnd.viewParms.world;

				GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);
				GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

				SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
			}

			// draw BSP nodes
			sentinel = &tr.traversalStack;
			for (l = sentinel->next; l != sentinel; l = l->next)
			{
				node = (bspNode_t *) l->data;

				if (!r_dynamicBspOcclusionCulling->integer)
				{
					if (node->contents != -1)
					{
						if (r_showBspNodes->integer == 3)
						{
							continue;
						}

						if (node->numMarkSurfaces <= 0)
						{
							continue;
						}

						if (node->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex])
						{
							SetUniformVec4(UNIFORM_COLOR, colorGreen);
						}
						else
						{
							SetUniformVec4(UNIFORM_COLOR, colorRed);
						}
					}
					else
					{
						if (r_showBspNodes->integer == 2)
						{
							continue;
						}

						if (node->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex])
						{
							SetUniformVec4(UNIFORM_COLOR, colorYellow);
						}
						else
						{
							SetUniformVec4(UNIFORM_COLOR, colorBlue);
						}
					}
				}
				else
				{
					if (node->lastVisited[backEnd.viewParms.viewCount] != backEnd.viewParms.frameCount)
					{
						continue;
					}

					if (r_showBspNodes->integer == 5 && node->lastQueried[backEnd.viewParms.viewCount] != backEnd.viewParms.frameCount)
					{
						continue;
					}

					if (node->contents != -1)
					{
						if (r_showBspNodes->integer == 3)
						{
							continue;
						}

						//if(node->occlusionQuerySamples[backEnd.viewParms.viewCount] > 0)
						if (node->visible[backEnd.viewParms.viewCount])
						{
							SetUniformVec4(UNIFORM_COLOR, colorGreen);
						}
						else
						{
							SetUniformVec4(UNIFORM_COLOR, colorRed);
						}
					}
					else
					{
						if (r_showBspNodes->integer == 2)
						{
							continue;
						}

						//if(node->occlusionQuerySamples[backEnd.viewParms.viewCount] > 0)
						if (node->visible[backEnd.viewParms.viewCount])
						{
							SetUniformVec4(UNIFORM_COLOR, colorYellow);
						}
						else
						{
							SetUniformVec4(UNIFORM_COLOR, colorBlue);
						}
					}

					if (r_showBspNodes->integer == 4)
					{
						SetUniformVec4(UNIFORM_COLOR, g_color_table[ColorIndex(node->occlusionQueryNumbers[backEnd.viewParms.viewCount])]);
					}

					GL_CheckErrors();
				}

				if (node->contents != -1)
				{
					glEnable(GL_POLYGON_OFFSET_FILL);
					GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
				}

				R_BindVBO(node->volumeVBO);
				R_BindIBO(node->volumeIBO);

				GLSL_VertexAttribsState(ATTR_POSITION);

				tess.multiDrawPrimitives = 0;
				tess.numVertexes         = node->volumeVerts;
				tess.numIndexes          = node->volumeIndexes;

				Tess_DrawElements();

				tess.numIndexes  = 0;
				tess.numVertexes = 0;

				if (node->contents != -1)
				{
					glDisable(GL_POLYGON_OFFSET_FILL);
				}
			}

			if (i == 1)
			{
				tess.multiDrawPrimitives = 0;
				tess.numIndexes          = 0;
				tess.numVertexes         = 0;

				GL_PopMatrix();

				GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				           backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				GL_PopMatrix();
			}
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;

		GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	if (r_showDecalProjectors->integer)
	{
		int              i;
		decalProjector_t *dp;
		srfDecal_t       *srfDecal;
		vec3_t           mins = { -1, -1, -1 };
		vec3_t           maxs = { 1, 1, 1 };

		if (backEnd.refdef.rdflags & (RDF_NOWORLDMODEL))
		{
			return;
		}

		SetMacrosAndSelectProgram(trProg.gl_genericShader);

		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
		SetUniformVec4(UNIFORM_COLOR, colorBlack);

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		// bind u_ColorMap
		SelectTexture(TEX_COLOR);
		GL_Bind(tr.whiteImage);

		SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

		GL_CheckErrors();

		Tess_Begin(Tess_StageIteratorDebug, NULL, NULL, NULL, qtrue, qfalse, LIGHTMAP_NONE, FOG_NONE);

		for (i = 0, dp = backEnd.refdef.decalProjectors; i < backEnd.refdef.numDecalProjectors; i++, dp++)
		{
			if (VectorDistanceSquared(dp->center, backEnd.viewParms.orientation.origin) > Square(1024))
			{
				continue;
			}

			Tess_AddCube(dp->center, mins, maxs, colorRed);
			Tess_AddCube(vec3_origin, dp->mins, dp->maxs, colorBlue);
		}

		glEnable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->value, r_offsetUnits->value);

		for (i = 0, srfDecal = backEnd.refdef.decals; i < backEnd.refdef.numDecals; i++, srfDecal++)
		{
			rb_surfaceTable[SF_DECAL] (srfDecal);
		}

		glDisable(GL_POLYGON_OFFSET_FILL);

		Tess_End();

		// go back to the world modelview matrix
		//backEnd.orientation = backEnd.viewParms.world;
		//GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	GL_CheckErrors();
}

/**
 * @brief RB_RenderViewFront
 */
static void RB_RenderViewFront(void)
{
	// Forward shading path
	int clearBits = 0;
	int startTime = 0;

	// sync with gl if needed
	if (r_finish->integer == 0)
	{
		glState.finishCalled = qtrue;
	}
	else if (r_finish->integer == 1 && !glState.finishCalled)
	{
		GL_JOIN();
		glState.finishCalled = qtrue;
	}

	// disable offscreen rendering
	if (glConfig2.framebufferObjectAvailable)
	{
		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			R_BindFBO(tr.deferredRenderFBO);
		}
		else
		{
			R_BindNullFBO();
		}
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	// set the modelview matrix for the viewer
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State(GLS_DEFAULT);

	// clear relevant buffers
	clearBits = GL_DEPTH_BUFFER_BIT;

	if (r_measureOverdraw->integer)
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
	// global q3 fog volume
	else if (tr.world && tr.world->globalFog >= 0)
	{
		clearBits |= GL_DEPTH_BUFFER_BIT;

		if (!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{
			clearBits |= GL_COLOR_BUFFER_BIT;

			GL_ClearColor(tr.world->fogs[tr.world->globalFog].color[0],
			              tr.world->fogs[tr.world->globalFog].color[1],
			              tr.world->fogs[tr.world->globalFog].color[2], 1.0);
		}
	}
	else if (tr.world && tr.world->hasSkyboxPortal)
	{
		if (backEnd.refdef.rdflags & RDF_SKYBOXPORTAL)
		{
			// portal scene, clear whatever is necessary
			clearBits |= GL_DEPTH_BUFFER_BIT;

			if (r_fastSky->integer || (backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
			{
				// fastsky: clear color

				// try clearing first with the portal sky fog color, then the world fog color, then finally a default
				clearBits |= GL_COLOR_BUFFER_BIT;
				if (tr.glfogsettings[FOG_PORTALVIEW].registered)
				{
					GL_ClearColor(tr.glfogsettings[FOG_PORTALVIEW].color[0], tr.glfogsettings[FOG_PORTALVIEW].color[1],
					              tr.glfogsettings[FOG_PORTALVIEW].color[2], tr.glfogsettings[FOG_PORTALVIEW].color[3]);
				}
				else if (tr.glfogNum > FOG_NONE && tr.glfogsettings[FOG_CURRENT].registered)
				{
					GL_ClearColor(tr.glfogsettings[FOG_CURRENT].color[0], tr.glfogsettings[FOG_CURRENT].color[1],
					              tr.glfogsettings[FOG_CURRENT].color[2], tr.glfogsettings[FOG_CURRENT].color[3]);
				}
				else
				{
					//GL_ClearColor(GLCOLOR_RED);   // red clear for testing portal sky clear
					GL_ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
				}
			}
			else
			{
				// rendered sky (either clear color or draw quake sky)
				if (tr.glfogsettings[FOG_PORTALVIEW].registered)
				{
					GL_ClearColor(tr.glfogsettings[FOG_PORTALVIEW].color[0], tr.glfogsettings[FOG_PORTALVIEW].color[1],
					              tr.glfogsettings[FOG_PORTALVIEW].color[2], tr.glfogsettings[FOG_PORTALVIEW].color[3]);

					if (tr.glfogsettings[FOG_PORTALVIEW].clearscreen)
					{
						// portal fog requests a screen clear (distance fog rather than quake sky)
						clearBits |= GL_COLOR_BUFFER_BIT;
					}
				}
			}
		}
		else
		{
			// world scene with portal sky, don't clear any buffers, just set the fog color if there is one
			clearBits |= GL_DEPTH_BUFFER_BIT;   // this will go when I get the portal sky rendering way out in the zbuffer (or not writing to zbuffer at all)

			if (tr.glfogNum > FOG_NONE && tr.glfogsettings[FOG_CURRENT].registered)
			{
				if (backEnd.refdef.rdflags & RDF_UNDERWATER)
				{
					if (tr.glfogsettings[FOG_CURRENT].mode == GL_LINEAR)
					{
						clearBits |= GL_COLOR_BUFFER_BIT;
					}
				}
				else if (!r_portalSky->integer)
				{
					// portal skies have been manually turned off, clear bg color
					clearBits |= GL_COLOR_BUFFER_BIT;
				}

				GL_ClearColor(tr.glfogsettings[FOG_CURRENT].color[0], tr.glfogsettings[FOG_CURRENT].color[1],
				              tr.glfogsettings[FOG_CURRENT].color[2], tr.glfogsettings[FOG_CURRENT].color[3]);
			}
			else if (!r_portalSky->integer)
			{
				// portal skies have been manually turned off, clear bg color
				clearBits |= GL_COLOR_BUFFER_BIT;
				GL_ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			}
		}
	}
	else
	{
		// world scene with no portal sky
		clearBits |= GL_DEPTH_BUFFER_BIT;

		// we don't want to clear the buffer when no world model is specified
		if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
		{
			clearBits &= ~GL_COLOR_BUFFER_BIT;
		}
		else if (r_fastSky->integer /*|| (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)*/)
		{
			clearBits |= GL_COLOR_BUFFER_BIT;

			if (tr.glfogsettings[FOG_CURRENT].registered)
			{
				// try to clear fastsky with current fog color
				GL_ClearColor(tr.glfogsettings[FOG_CURRENT].color[0], tr.glfogsettings[FOG_CURRENT].color[1],
				              tr.glfogsettings[FOG_CURRENT].color[2], tr.glfogsettings[FOG_CURRENT].color[3]);
			}
			else
			{
				//GL_ClearColor(GLCOLOR_BLUE);   // blue clear for testing world sky clear
				GL_ClearColor(0.05f, 0.05f, 0.05f, 1.0f);   // changed per id req was 0.5s
			}
		}
		else
		{
			// world scene, no portal sky, not fastsky, clear color if fog says to, otherwise, just set the clearcolor
			if (tr.glfogsettings[FOG_CURRENT].registered)
			{
				// try to clear fastsky with current fog color
				GL_ClearColor(tr.glfogsettings[FOG_CURRENT].color[0], tr.glfogsettings[FOG_CURRENT].color[1],
				              tr.glfogsettings[FOG_CURRENT].color[2], tr.glfogsettings[FOG_CURRENT].color[3]);

				if (tr.glfogsettings[FOG_CURRENT].clearscreen)
				{
					// world fog requests a screen clear (distance fog rather than quake sky)
					clearBits |= GL_COLOR_BUFFER_BIT;
				}
			}
		}

		if (HDR_ENABLED())
		{
			// copy color of the main context to deferredRenderFBO
			R_CopyToFBO(NULL, tr.deferredRenderFBO, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
	}

	GL_Clear(clearBits);

	if ((backEnd.refdef.rdflags & RDF_HYPERSPACE))
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1;   // force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	GL_CheckErrors();

	R2_TIMING(RSPEEDS_SHADING_TIMES)
	{
		startTime = ri.Milliseconds();
	}

	if (r_dynamicEntityOcclusionCulling->integer)
	{
		// draw everything from world that is opaque into black so we can benefit from early-z rejections later
		//RB_RenderOpaqueSurfacesIntoDepth(true);
		RB_RenderDrawSurfaces(qtrue, DRAWSURFACES_WORLD_ONLY);

		// try to cull entities using hardware occlusion queries
		RB_RenderEntityOcclusionQueries();

		// draw everything that is opaque
		RB_RenderDrawSurfaces(qtrue, DRAWSURFACES_ENTITIES_ONLY);
	}
	else
	{
		// draw everything that is opaque
		RB_RenderDrawSurfaces(qtrue, DRAWSURFACES_ALL);

		// try to cull entities using hardware occlusion queries
		//RB_RenderEntityOcclusionQueries();
	}

	// try to cull bsp nodes for the next frame using hardware occlusion queries
	RB_RenderBspOcclusionQueries();

	R2_TIMING(RSPEEDS_SHADING_TIMES)
	{
		backEnd.pc.c_forwardAmbientTime = ri.Milliseconds() - startTime;
	}

	// try to cull lights using hardware occlusion queries
	RB_RenderLightOcclusionQueries();

	if (r_shadows->integer >= SHADOWING_ESM16)
	{
		// render dynamic shadowing and lighting using shadow mapping
		RB_RenderInteractionsShadowMapped();

		// render player shadows if any
		//RB_RenderInteractionsDeferredInverseShadows();
	}
	else
	{
		// render dynamic lighting
		RB_RenderInteractions();
	}

	// render ambient occlusion process effect
	// needs way more work
	RB_RenderScreenSpaceAmbientOcclusion();

	if (HDR_ENABLED())
	{
		R_BindFBO(tr.deferredRenderFBO);
	}

	// render global fog effect
	// This is the fog that fills the world.
	// It is not the fog that is rendered on brushes.
	RB_RenderGlobalFog();

	// draw everything that is translucent
	RB_RenderDrawSurfaces(qfalse, DRAWSURFACES_ALL);

	// scale down rendered HDR scene to 1 / 4th
	if (HDR_ENABLED())
	{
		if (glConfig2.framebufferBlitAvailable)
		{
			R_CopyToFBO(tr.deferredRenderFBO, tr.downScaleFBO_quarter, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			R_CopyToFBO(tr.deferredRenderFBO, tr.downScaleFBO_64x64, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
		else
		{
			// FIXME add non EXT_framebuffer_blit code
		}

		RB_CalculateAdaptation();
	}
	else
	{
		/*
		FIXME this causes: caught OpenGL error:
		GL_INVALID_OPERATION in file code/renderer/tr_backend.c line 6479

		if(glConfig2.framebufferBlitAvailable)
		{
		    // copy deferredRenderFBO to downScaleFBO_quarter
		    R_CopyToFBO(NULL,tr.downScaleFBO_quarter,GL_COLOR_BUFFER_BIT,GL_NEAREST);
		}
		else
		{
		    // FIXME add non EXT_framebuffer_blit code
		}
		*/
	}

	GL_CheckErrors();

	// render depth of field post process effect
	RB_RenderDepthOfField();

	// render bloom post process effect
	RB_RenderBloom();

	// copy offscreen rendered HDR scene to the current OpenGL context
	RB_RenderHDRResultToFrameBuffer();

	// render rotoscope post process effect
	RB_RenderRotoscope();

	// add the sun flare
	RB_DrawSun();

	// add light flares on lights that aren't obscured
	RB_RenderFlares(); // this initiates calls to the very slow glReadPixels()..

	// wait until all bsp node occlusion queries are back
	RB_CollectBspOcclusionQueries();

	// render debug information
	RB_RenderDebugUtils();

	if (backEnd.viewParms.isPortal)
	{
#if 0
		if (r_hdrRendering->integer && glConfig.textureFloatAvailable && glConfig.framebufferObjectAvailable && glConfig.framebufferBlitAvailable)
		{
			// copy deferredRenderFBO to portalRenderFBO
			R_CopyToFBO(tr.deferredRenderFBO, tr.portalRenderFBO, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
#endif
#if 0
		// FIXME: this trashes the OpenGL context for an unknown reason
		if (glConfig2.framebufferObjectAvailable && glConfig2.framebufferBlitAvailable)
		{
			// copy main context to portalRenderFBO
			R_CopyToFBO(NULL, tr.portalRenderFBO, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
#endif
		//else
		{
			// capture current color buffer
			GL_SelectTexture(0);
			ImageCopyBackBuffer(tr.portalRenderImage);
		}
		backEnd.pc.c_portals++;
	}

#if 0
	if (r_dynamicBspOcclusionCulling->integer)
	{
		// copy depth of the main context to deferredRenderFBO
		R_CopyToFBO(NULL, tr.occlusionRenderFBO, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
#endif
}

/**
 * @brief RB_RenderView
 */
static void RB_RenderView(void)
{
	Ren_LogComment("--- RB_RenderView( %i surfaces, %i interactions ) ---\n", backEnd.viewParms.numDrawSurfs, backEnd.viewParms.numInteractions);

	//Ren_Fatal( "test");

	GL_CheckErrors();

	backEnd.pc.c_surfaces += backEnd.viewParms.numDrawSurfs;

	RB_RenderViewFront();

	// render chromatric aberration
	if (tr.refdef.pixelTarget == NULL) // see comment on next code block.. we don't want postFX on cubemaps
	{
	RB_CameraPostFX();
	}
	// copy to given byte buffer that is NOT a FBO.
	// This will copy the current screen content to a texture.
	// The given texture, pixelTarget, is (a pointer to) a cubeProbe's cubemap.
	// R_BuildCubeMaps() is the function that triggers this mechanism.
	else //if (tr.refdef.pixelTarget != NULL)
	{
		// Bugfix: drivers absolutely hate running in high res and using glReadPixels near the top or bottom edge.
		// Soo.. lets do it in the middle.
		// Note: i don't know how old that^^ comment above is. The issue could long be history.. todo: check it
		glReadPixels(glConfig.vidWidth / 2, glConfig.vidHeight / 2, REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, tr.refdef.pixelTarget);
	}

	GL_CheckErrors();

	backEnd.pc.c_views++;
}

/*
============================================================================
RENDER BACK END THREAD FUNCTIONS
============================================================================
*/

/**
 * @brief RE_StretchRaw
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] cols
 * @param[in] rows
 * @param[in] data
 * @param[in] client
 * @param[in] dirty
 *
 * @todo FIXME: not exactly backend
 * Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
 * Used for cinematics.
 */
void RE_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty)
{
	//int i, j;
	int start, end;

	if (!tr.registered)
	{
		return;
	}
	R_IssuePendingRenderCommands();

	// we definately want to sync every frame for the cinematics
	GL_JOIN();

	start = end = 0;
	if (r_speeds->integer)
	{
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	// In opengl 3.2 NPOT textures are a part of the core
	/*
	for (i = 0; (1 << i) < cols; i++)
	{
	}
	for (j = 0; (1 << j) < rows; j++)
	{
	}
	if ((1 << i) != cols || (1 << j) != rows)
	{
	    Ren_Drop("Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}
	*/


	RB_SetGL2D();

	glVertexAttrib4f(ATTR_INDEX_NORMAL, 0, 0, 1, 1);
	glVertexAttrib4f(ATTR_INDEX_COLOR, tr.identityLight, tr.identityLight, tr.identityLight, 1);

	SetMacrosAndSelectProgram(trProg.gl_genericShader);

	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
	SetUniformVec4(UNIFORM_COLOR, colorBlack);

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(tr.scratchImage[client]);

	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

	RE_UploadCinematic(w, h, cols, rows, data, client, dirty);

	R2_TIMING_SIMPLE()
	{
		end = ri.Milliseconds();
		Ren_Print("glTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start);
	}

	tess.multiDrawPrimitives = 0;
	tess.numVertexes         = 0;
	tess.numIndexes          = 0;

	tess.xyz[tess.numVertexes][0]       = x;
	tess.xyz[tess.numVertexes][1]       = y;
	tess.xyz[tess.numVertexes][2]       = 0;
	tess.xyz[tess.numVertexes][3]       = 1;
	tess.texCoords[tess.numVertexes][0] = 0.5f / cols;
	tess.texCoords[tess.numVertexes][1] = 0.5f / rows;
	tess.texCoords[tess.numVertexes][2] = 0;
	tess.texCoords[tess.numVertexes][3] = 1;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0]       = x + w;
	tess.xyz[tess.numVertexes][1]       = y;
	tess.xyz[tess.numVertexes][2]       = 0;
	tess.xyz[tess.numVertexes][3]       = 1;
	tess.texCoords[tess.numVertexes][0] = (cols - 0.5f) / cols;
	tess.texCoords[tess.numVertexes][1] = 0.5f / rows;
	tess.texCoords[tess.numVertexes][2] = 0;
	tess.texCoords[tess.numVertexes][3] = 1;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0]       = x + w;
	tess.xyz[tess.numVertexes][1]       = y + h;
	tess.xyz[tess.numVertexes][2]       = 0;
	tess.xyz[tess.numVertexes][3]       = 1;
	tess.texCoords[tess.numVertexes][0] = (cols - 0.5f) / cols;
	tess.texCoords[tess.numVertexes][1] = (rows - 0.5f) / rows;
	tess.texCoords[tess.numVertexes][2] = 0;
	tess.texCoords[tess.numVertexes][3] = 1;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0]       = x;
	tess.xyz[tess.numVertexes][1]       = y + h;
	tess.xyz[tess.numVertexes][2]       = 0;
	tess.xyz[tess.numVertexes][3]       = 1;
	tess.texCoords[tess.numVertexes][0] = 0.5f / cols;
	tess.texCoords[tess.numVertexes][1] = (rows - 0.5f) / rows;
	tess.texCoords[tess.numVertexes][2] = 0;
	tess.texCoords[tess.numVertexes][3] = 1;
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	Tess_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD);

	Tess_DrawElements();

	tess.multiDrawPrimitives = 0;
	tess.numVertexes         = 0;
	tess.numIndexes          = 0;

	GL_CheckErrors();
}

/**
 * @brief RE_UploadCinematic
 * @param w - unused
 * @param h - unused
 * @param[in] cols
 * @param[in] rows
 * @param[in] data
 * @param[in] client
 * @param[in] dirty
 */
void RE_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty)
{
	GL_Bind(tr.scratchImage[client]);

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if (cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height)
	{
		tr.scratchImage[client]->width  = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if 1
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, colorBlack);
#endif
	}
	else
	{
		if (dirty)
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}

	GL_CheckErrors();
}

/**
 * @brief RB_SetColor
 * @param[in] data
 * @return
 */
const void *RB_SetColor(const void *data)
{
	const setColorCommand_t *cmd = (const setColorCommand_t *)data;

	Ren_LogComment("--- RB_SetColor ---\n");

	backEnd.color2D[0] = cmd->color[0];
	backEnd.color2D[1] = cmd->color[1];
	backEnd.color2D[2] = cmd->color[2];
	backEnd.color2D[3] = cmd->color[3];

	return (const void *)(cmd + 1);
}

/**
 * @brief RB_StretchPic
 * @param[in] data
 * @return
 */
const void *RB_StretchPic(const void *data)
{
	int                       i;
	const stretchPicCommand_t *cmd = (const stretchPicCommand_t *)data;
	shader_t                  *shader;
	int                       numVerts, numIndexes;

	Ren_LogComment("--- RB_StretchPic ---\n");

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if (shader != tess.surfaceShader)
	{
		if (tess.numIndexes)
		{
			Tess_End();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		Tess_Begin(Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, LIGHTMAP_NONE, FOG_NONE);
	}

	Tess_CheckOverflow(4, 6);
	numVerts   = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes  += 6;

	tess.indexes[numIndexes]     = numVerts + 3;
	tess.indexes[numIndexes + 1] = numVerts + 0;
	tess.indexes[numIndexes + 2] = numVerts + 2;
	tess.indexes[numIndexes + 3] = numVerts + 2;
	tess.indexes[numIndexes + 4] = numVerts + 0;
	tess.indexes[numIndexes + 5] = numVerts + 1;


	for (i = 0; i < 4; i++)
	{
		tess.colors[numVerts + i][0] = backEnd.color2D[0];
		tess.colors[numVerts + i][1] = backEnd.color2D[1];
		tess.colors[numVerts + i][2] = backEnd.color2D[2];
		tess.colors[numVerts + i][3] = backEnd.color2D[3];
	}

	tess.xyz[numVerts][0] = cmd->x;
	tess.xyz[numVerts][1] = cmd->y;
	tess.xyz[numVerts][2] = 0;
	tess.xyz[numVerts][3] = 1;

	tess.texCoords[numVerts][0] = cmd->s1;
	tess.texCoords[numVerts][1] = cmd->t1;
	tess.texCoords[numVerts][2] = 0;
	tess.texCoords[numVerts][3] = 1;

	tess.xyz[numVerts + 1][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 1][1] = cmd->y;
	tess.xyz[numVerts + 1][2] = 0;
	tess.xyz[numVerts + 1][3] = 1;

	tess.texCoords[numVerts + 1][0] = cmd->s2;
	tess.texCoords[numVerts + 1][1] = cmd->t1;
	tess.texCoords[numVerts + 1][2] = 0;
	tess.texCoords[numVerts + 1][3] = 1;

	tess.xyz[numVerts + 2][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 2][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 2][2] = 0;
	tess.xyz[numVerts + 2][3] = 1;

	tess.texCoords[numVerts + 2][0] = cmd->s2;
	tess.texCoords[numVerts + 2][1] = cmd->t2;
	tess.texCoords[numVerts + 2][2] = 0;
	tess.texCoords[numVerts + 2][3] = 1;

	tess.xyz[numVerts + 3][0] = cmd->x;
	tess.xyz[numVerts + 3][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 3][2] = 0;
	tess.xyz[numVerts + 3][3] = 1;

	tess.texCoords[numVerts + 3][0] = cmd->s1;
	tess.texCoords[numVerts + 3][1] = cmd->t2;
	tess.texCoords[numVerts + 3][2] = 0;
	tess.texCoords[numVerts + 3][3] = 1;

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR;
	return (const void *)(cmd + 1);
}

/**
 * @brief RB_Draw2dPolys
 * @param[in] data
 * @return
 */
const void *RB_Draw2dPolys(const void *data)
{
	const poly2dCommand_t *cmd = (const poly2dCommand_t *)data;
	shader_t              *shader;
	int                   i;

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if (shader != tess.surfaceShader)
	{
		if (tess.numIndexes)
		{
			Tess_End();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		Tess_Begin(Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, LIGHTMAP_NONE, FOG_NONE);
	}

	Tess_CheckOverflow(cmd->numverts, (cmd->numverts - 2) * 3);

	for (i = 0; i < cmd->numverts - 2; i++)
	{
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes                  += 3;
	}

	for (i = 0; i < cmd->numverts; i++)
	{
		tess.xyz[tess.numVertexes][0] = cmd->verts[i].xyz[0];
		tess.xyz[tess.numVertexes][1] = cmd->verts[i].xyz[1];
		tess.xyz[tess.numVertexes][2] = 0;
		tess.xyz[tess.numVertexes][3] = 1;

		tess.texCoords[tess.numVertexes][0] = cmd->verts[i].st[0];
		tess.texCoords[tess.numVertexes][1] = cmd->verts[i].st[1];

		tess.colors[tess.numVertexes][0] = cmd->verts[i].modulate[0] * (1.0f / 255.0f);
		tess.colors[tess.numVertexes][1] = cmd->verts[i].modulate[1] * (1.0f / 255.0f);
		tess.colors[tess.numVertexes][2] = cmd->verts[i].modulate[2] * (1.0f / 255.0f);
		tess.colors[tess.numVertexes][3] = cmd->verts[i].modulate[3] * (1.0f / 255.0f);
		tess.numVertexes++;
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR;
	return (const void *)(cmd + 1);
}

/**
 * @brief RB_RotatedPic
 * @param[in] data
 * @return
 */
const void *RB_RotatedPic(const void *data)
{
	const stretchPicCommand_t *cmd = (const stretchPicCommand_t *)data;
	shader_t                  *shader;
	int                       numVerts, numIndexes;
	float                     angle;
	float                     mx, my, mw, mh;

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if (shader != tess.surfaceShader)
	{
		if (tess.numIndexes)
		{
			Tess_End();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		Tess_Begin(Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, LIGHTMAP_NONE, FOG_NONE);
	}

	Tess_CheckOverflow(4, 6);
	numVerts   = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes  += 6;

	tess.indexes[numIndexes]     = numVerts + 3;
	tess.indexes[numIndexes + 1] = numVerts + 0;
	tess.indexes[numIndexes + 2] = numVerts + 2;
	tess.indexes[numIndexes + 3] = numVerts + 2;
	tess.indexes[numIndexes + 4] = numVerts + 0;
	tess.indexes[numIndexes + 5] = numVerts + 1;

	Vector4Copy(backEnd.color2D, tess.colors[numVerts + 0]);
	Vector4Copy(backEnd.color2D, tess.colors[numVerts + 1]);
	Vector4Copy(backEnd.color2D, tess.colors[numVerts + 2]);
	Vector4Copy(backEnd.color2D, tess.colors[numVerts + 3]);

#define ROTSCALE 0.725f

	mx = cmd->x + (cmd->w / 2);
	my = cmd->y + (cmd->h / 2);
	mw = cmd->w * ROTSCALE;
	mh = cmd->h * ROTSCALE;

#define COSAN mx + (float)(cos(angle) * mw)
#define SINAN my + (float)(sin(angle) * mh)

	angle                 = cmd->angle * M_TAU_F;
	tess.xyz[numVerts][0] = COSAN;
	tess.xyz[numVerts][1] = SINAN;
	tess.xyz[numVerts][2] = 0;
	tess.xyz[numVerts][3] = 1;

	tess.texCoords[numVerts][0] = cmd->s1;
	tess.texCoords[numVerts][1] = cmd->t1;

	angle                     = cmd->angle * M_TAU_F + 0.25f * M_TAU_F;
	tess.xyz[numVerts + 1][0] = COSAN;
	tess.xyz[numVerts + 1][1] = SINAN;
	tess.xyz[numVerts + 1][2] = 0;
	tess.xyz[numVerts + 1][3] = 1;

	tess.texCoords[numVerts + 1][0] = cmd->s2;
	tess.texCoords[numVerts + 1][1] = cmd->t1;

	angle                     = cmd->angle * M_TAU_F + 0.50f * M_TAU_F;
	tess.xyz[numVerts + 2][0] = COSAN;
	tess.xyz[numVerts + 2][1] = SINAN;
	tess.xyz[numVerts + 2][2] = 0;
	tess.xyz[numVerts + 2][3] = 1;

	tess.texCoords[numVerts + 2][0] = cmd->s2;
	tess.texCoords[numVerts + 2][1] = cmd->t2;

	angle                     = cmd->angle * M_TAU_F + 0.75f * M_TAU_F;
	tess.xyz[numVerts + 3][0] = COSAN;
	tess.xyz[numVerts + 3][1] = SINAN;
	tess.xyz[numVerts + 3][2] = 0;
	tess.xyz[numVerts + 3][3] = 1;

	tess.texCoords[numVerts + 3][0] = cmd->s1;
	tess.texCoords[numVerts + 3][1] = cmd->t2;

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR;
	return (const void *)(cmd + 1);
}

/**
 * @brief RB_StretchPicGradient
 * @param[in] data
 * @return
 */
const void *RB_StretchPicGradient(const void *data)
{
	const stretchPicCommand_t *cmd = (const stretchPicCommand_t *)data;
	shader_t                  *shader;
	int                       numVerts, numIndexes;
	int                       i;

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if (shader != tess.surfaceShader)
	{
		if (tess.numIndexes)
		{
			Tess_End();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		Tess_Begin(Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, LIGHTMAP_NONE, FOG_NONE);
	}

	Tess_CheckOverflow(4, 6);
	numVerts   = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes  += 6;

	tess.indexes[numIndexes]     = numVerts + 3;
	tess.indexes[numIndexes + 1] = numVerts + 0;
	tess.indexes[numIndexes + 2] = numVerts + 2;
	tess.indexes[numIndexes + 3] = numVerts + 2;
	tess.indexes[numIndexes + 4] = numVerts + 0;
	tess.indexes[numIndexes + 5] = numVerts + 1;

	//*(int *)tess.vertexColors[numVerts].v = *(int *)tess.vertexColors[numVerts + 1].v = *(int *)backEnd.color2D;
	//*(int *)tess.vertexColors[numVerts + 2].v = *(int *)tess.vertexColors[numVerts + 3].v = *(int *)cmd->gradientColor;

	Vector4Copy(backEnd.color2D, tess.colors[numVerts + 0]);
	Vector4Copy(backEnd.color2D, tess.colors[numVerts + 1]);

	for (i = 0; i < 4; i++)
	{
		tess.colors[numVerts + 2][i] = cmd->gradientColor[i] * (1.0f / 255.0f);
		tess.colors[numVerts + 3][i] = cmd->gradientColor[i] * (1.0f / 255.0f);
	}

	tess.xyz[numVerts][0] = cmd->x;
	tess.xyz[numVerts][1] = cmd->y;
	tess.xyz[numVerts][2] = 0;
	tess.xyz[numVerts][3] = 1;

	tess.texCoords[numVerts][0] = cmd->s1;
	tess.texCoords[numVerts][1] = cmd->t1;

	tess.xyz[numVerts + 1][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 1][1] = cmd->y;
	tess.xyz[numVerts + 1][2] = 0;
	tess.xyz[numVerts + 1][3] = 1;

	tess.texCoords[numVerts + 1][0] = cmd->s2;
	tess.texCoords[numVerts + 1][1] = cmd->t1;

	tess.xyz[numVerts + 2][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 2][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 2][2] = 0;
	tess.xyz[numVerts + 2][3] = 1;

	tess.texCoords[numVerts + 2][0] = cmd->s2;
	tess.texCoords[numVerts + 2][1] = cmd->t2;

	tess.xyz[numVerts + 3][0] = cmd->x;
	tess.xyz[numVerts + 3][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 3][2] = 0;
	tess.xyz[numVerts + 3][3] = 1;

	tess.texCoords[numVerts + 3][0] = cmd->s1;
	tess.texCoords[numVerts + 3][1] = cmd->t2;

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR;
	return (const void *)(cmd + 1);
}

/**
 * @brief RB_DrawView
 * @param[in] data
 * @return
 */
const void *RB_DrawView(const void *data)
{
	const drawViewCommand_t *cmd;

	Ren_LogComment("--- RB_DrawView ---\n");

	// finish any 2D drawing if needed
	if (tess.numIndexes)
	{
		Tess_End();
	}

	cmd = (const drawViewCommand_t *)data;

	backEnd.refdef    = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderView();

	return (const void *)(cmd + 1);
}

/**
 * @brief RB_DrawBuffer
 * @param[in] data
 * @return
 */
const void *RB_DrawBuffer(const void *data)
{
	const drawBufferCommand_t *cmd = (const drawBufferCommand_t *)data;

	Ren_LogComment("--- RB_DrawBuffer ---\n");

	GL_DrawBuffer(cmd->buffer);

	// clear screen for debugging
	if (r_clear->integer)
	{
		GL_ClearColor(GLCOLOR_BLACK);
		GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	return (const void *)(cmd + 1);
}

/**
 * @brief Draw all the images to the screen, on top of whatever
 * was there.  This is used to test for texture thrashing.
 *
 * Also called by RE_EndRegistration
 */
void RB_ShowImages(void)
{
	int     i;
	image_t *image;
	float   x, y, w, h;
	vec4_t  quadVerts[4];
	int     start, end;

	Ren_LogComment("--- RB_ShowImages ---\n");

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	GL_Clear(GL_COLOR_BUFFER_BIT);

	SetMacrosAndSelectProgram(trProg.gl_genericShader);

	GL_Cull(CT_TWO_SIDED);

	// set uniforms
	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_VERTEX, AGEN_VERTEX);
	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

	SelectTexture(TEX_COLOR);

	start = ri.Milliseconds();

	for (i = 0; i < tr.images.currentElements; i++)
	{
		image = (image_t *) Com_GrowListElement(&tr.images, i);

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if (r_showImages->integer == 2)
		{
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		// bind u_ColorMap
		GL_Bind(image);

		Vector4Set(quadVerts[0], x, y, 0, 1);
		Vector4Set(quadVerts[1], x + w, y, 0, 1);
		Vector4Set(quadVerts[2], x + w, y + h, 0, 1);
		Vector4Set(quadVerts[3], x, y + h, 0, 1);

		Tess_InstantQuad(quadVerts);
	}

	end = ri.Milliseconds();
	Ren_Print("%i msec to draw all images\n", end - start);

	GL_CheckErrors();
}
/**
 * @brief RB_ColorCorrection
 */
static void RB_ColorCorrection()
{
	Ren_LogComment("--- RB_ColorCorrection ---\n");

	// enable shader, set arrays
	SetMacrosAndSelectProgram(trProg.gl_colorCorrection);

	//glVertexAttrib4fv(ATTR_INDEX_COLOR, colorWhite);

	// capture current color buffer for u_CurrentMap
	SelectTexture(TEX_CURRENT);
	ImageCopyBackBuffer(tr.currentRenderImage);

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);
	SetUniformFloat(UNIFORM_GAMMA, (!ri.Cvar_VariableIntegerValue("r_ignorehwgamma") ? r_gamma->value : 1.0f));

	DRAWSCREENQUAD();
	GL_CheckErrors();
}

/************************************************************************/
/* Do all post processing of the back buffer here                       */
/************************************************************************/

/**
 * @brief RB_PostProcess
 */
static void RB_PostProcess()
{
	Ren_LogComment("--- RB_PostProcess ---\n");

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	// We need to reset these out or otherwise we crash out on map load
	R_BindNullVBO();
	R_BindNullIBO();

	GL_State(GLS_DEPTHTEST_DISABLE);    // | GLS_DEPTHMASK_TRUE);
	GL_Cull(CT_TWO_SIDED);

	RB_ColorCorrection();

	// texture swapping test
	if (r_showImages->integer)
	{
		RB_ShowImages();
	}
}

/**
 * @brief RB_CountOverDraw
 */
static void RB_CountOverDraw()
{
	int           i;
	long          sum = 0;
	unsigned char *stencilReadback;

	stencilReadback = (unsigned char *)ri.Hunk_AllocateTempMemory(glConfig.vidWidth * glConfig.vidHeight);
	glReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback);

	for (i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++)
	{
		sum += stencilReadback[i];
	}

	backEnd.pc.c_overDraw += sum;
	ri.Hunk_FreeTempMemory(stencilReadback);
}

/**
 * @brief RB_SwapBuffers
 * @param[in] data
 * @return
 */
const void *RB_SwapBuffers(const void *data)
{
	const swapBuffersCommand_t *cmd;

	// finish any 2D drawing if needed
	if (tess.numIndexes)
	{
		Tess_End();
	}

	RB_PostProcess();

	cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if (r_measureOverdraw->integer)
	{
		RB_CountOverDraw();
	}

	if (!glState.finishCalled)
	{
		GL_JOIN();
	}

	Ren_LogComment("***************** RB_SwapBuffers *****************\n\n\n");

	ri.GLimp_SwapFrame();

	backEnd.projection2D = qfalse;

	glState.finishCalled = qfalse;

	return (const void *)(cmd + 1);
}

/**
 * @brief RB_RenderToTexture
 * @param[in] data
 * @return
 */
const void *RB_RenderToTexture(const void *data)
{
	const renderToTextureCommand_t *cmd = (const renderToTextureCommand_t *)data;

	//ri.Printf( PRINT_ALL, "RB_RenderToTexture\n" );

	GL_Bind(cmd->image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cmd->x, cmd->y, cmd->w, cmd->h, 0);
	//glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cmd->x, cmd->y, cmd->w, cmd->h );

	return (const void *)(cmd + 1);
}

/**
 * @brief RB_Finish
 * @param[in] data
 * @return
 */
const void *RB_Finish(const void *data)
{
	const renderFinishCommand_t *cmd = (const renderFinishCommand_t *)data;

	//ri.Printf( PRINT_ALL, "RB_Finish\n" );

	glFinish();

	return (const void *)(cmd + 1);
}

/**
 * @brief This function will be called synchronously if running without
 * smp extensions, or asynchronously by another thread.
 * @param[in] data
 */
void RB_ExecuteRenderCommands(const void *data)
{
	int t1, t2;

	Ren_LogComment("--- RB_ExecuteRenderCommands ---\n");

	t1 = ri.Milliseconds();

	while (1)
	{
		switch (*(const int *)data)
		{
		case RC_SET_COLOR:
			data = RB_SetColor(data);
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic(data);
			break;
		case RC_2DPOLYS:
			data = RB_Draw2dPolys(data);
			break;
		case RC_ROTATED_PIC:
			data = RB_RotatedPic(data);
			break;
		case RC_STRETCH_PIC_GRADIENT:
			data = RB_StretchPicGradient(data);
			break;
		case RC_DRAW_VIEW:
			data = RB_DrawView(data);
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer(data);
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers(data);
			break;
		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd(data);
			break;
		case RC_VIDEOFRAME:
			data = RB_TakeVideoFrameCmd(data);
			break;
		case RC_RENDERTOTEXTURE:
			data = RB_RenderToTexture(data);
			break;
		case RC_FINISH:
			data = RB_Finish(data);
			break;

		case RC_END_OF_LIST:
		default:
			// stop rendering on this thread
			t2              = ri.Milliseconds();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}
}
