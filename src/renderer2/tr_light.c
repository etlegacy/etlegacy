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
 * @file renderer2/tr_light.c
 */

#include "tr_local.h"

/**
 * @brief Determine which dynamic lights may effect this bmodel
 * @param[in] ent
 * @param[in] light
 */
void R_AddBrushModelInteractions(trRefEntity_t *ent, trRefLight_t *light)
{
	unsigned int      i;
	bspSurface_t      *surf;
	bspModel_t        *bspModel = NULL;
	model_t           *pModel   = NULL;
	byte              cubeSideBits;
	interactionType_t iaType = IA_DEFAULT;

	// cull the entire model if it is outside the view frustum
	// and we don't care about proper shadowing
	if (ent->cull == CULL_OUT)
	{
		if (r_shadows->integer <= SHADOWING_BLOB || light->l.noShadows)
		{
			return;
		}
		else
		{
			iaType = IA_SHADOWONLY;
		}
	}

	// avoid drawing of certain objects
#if defined(USE_REFENTITY_NOSHADOWID)
	if (light->l.inverseShadows)
	{
		if (iaType != IA_LIGHTONLY && (light->l.noShadowID && (light->l.noShadowID != ent->e.noShadowID)))
		{
			return;
		}
	}
	else
	{
		if (iaType != IA_LIGHTONLY && (light->l.noShadowID && (light->l.noShadowID == ent->e.noShadowID)))
		{
			return;
		}
	}
#endif

	pModel   = R_GetModelByHandle(ent->e.hModel);
	bspModel = pModel->bsp;

	// do a quick AABB cull
	if (!BoundsIntersect(light->worldBounds[0], light->worldBounds[1], ent->worldBounds[0], ent->worldBounds[1]))
	{
		tr.pc.c_dlightSurfacesCulled += bspModel->numSurfaces;
		return;
	}

	// do a more expensive and precise light frustum cull
	if (!r_noLightFrustums->integer)
	{
		if (R_CullLightWorldBounds(light, ent->worldBounds) == CULL_OUT)
		{
			tr.pc.c_dlightSurfacesCulled += bspModel->numSurfaces;
			return;
		}
	}

	cubeSideBits = R_CalcLightCubeSideBits(light, ent->worldBounds);

	if (r_vboModels->integer && bspModel->numVBOSurfaces)
	{
		srfVBOMesh_t *vboSurface;
		shader_t     *shader;

		// static VBOs are fine for lighting and shadow mapping
		for (i = 0; i < bspModel->numVBOSurfaces; i++)
		{
			vboSurface = bspModel->vboSurfaces[i];
			shader     = vboSurface->shader;

			// skip all surfaces that don't matter for lighting only pass
			if (shader->isSky || (!shader->interactLight && shader->noShadows))
			{
				continue;
			}

			R_AddLightInteraction(light, (surfaceType_t *)vboSurface, shader, cubeSideBits, iaType);
			tr.pc.c_dlightSurfaces++;
		}
	}
	else
	{
		// set the light bits in all the surfaces
		for (i = 0; i < bspModel->numSurfaces; i++)
		{
			surf = bspModel->firstSurface + i;

			// FIXME: do more culling?

			/*
			   if(*surf->data == SF_FACE)
			   {
			   ((srfSurfaceFace_t *) surf->data)->dlightBits = mask;
			   }
			   else if(*surf->data == SF_GRID)
			   {
			   ((srfGridMesh_t *) surf->data)->dlightBits = mask;
			   }
			   else if(*surf->data == SF_TRIANGLES)
			   {
			   ((srfTriangles_t *) surf->data)->dlightBits = mask;
			   }
			   else if(*surf->data == SF_FOLIAGE)
			   {
			   ((srfFoliage_t *) surf->data)->dlightBits = mask;
			   }
			 */

			// skip all surfaces that don't matter for lighting only pass
			if (surf->shader->isSky || (!surf->shader->interactLight && surf->shader->noShadows))
			{
				continue;
			}

			R_AddLightInteraction(light, surf->data, surf->shader, cubeSideBits, iaType);
			tr.pc.c_dlightSurfaces++;
		}
	}
}

/*
=============================================================================
LIGHT SAMPLING
=============================================================================
*/

/**
 * @brief R_SetupEntityLightingGrid
 * @param[in,out] ent
 * @param[in] forcedOrigin
 */
static void R_SetupEntityLightingGrid(trRefEntity_t *ent, vec3_t forcedOrigin)
{
	vec3_t         lightOrigin;
	int            pos[3];
	int            i; // , j;
	bspGridPoint_t *gridPoint;
	bspGridPoint_t *gridPoint2;
	float          frac[3];
	int            gridStep[3];
	vec3_t         direction, vec;
	float          totalFactor;
	//float          v;

	if (forcedOrigin)
	{
		VectorCopy(forcedOrigin, lightOrigin);
	}
	else
	{
		if (ent->e.renderfx & RF_LIGHTING_ORIGIN)
		{
			// seperate lightOrigins are needed so an object that is
			// sinking into the ground can still be lit, and so
			// multi-part models can be lit identically
			VectorCopy(ent->e.lightingOrigin, lightOrigin);
		}
		else
		{
			VectorCopy(ent->e.origin, lightOrigin);
		}
	}

	VectorSubtract(lightOrigin, tr.world->lightGridOrigin, lightOrigin);
	VectorMultiply(lightOrigin, tr.world->lightGridInverseSize, vec);
	for (i = 0; i < 3; i++)
	{
		//v       = lightOrigin[i] * tr.world->lightGridInverseSize[i];
		pos[i]  = floor(vec[i]);
		frac[i] = vec[i] - (float)pos[i];
		if (pos[i] < 0)
		{
			pos[i] = 0;
		}
		else if (pos[i] > tr.world->lightGridBounds[i] - 1)
		{
			pos[i] = tr.world->lightGridBounds[i] - 1;
		}
	}

	VectorClear(ent->ambientLight);
	VectorClear(ent->directedLight);
	VectorClear(direction);

	etl_assert(tr.world->lightGridData);   // NULL with -nolight maps

	// trilerp the light value
	gridStep[0] = 1; //sizeof(bspGridPoint_t);
	gridStep[1] = tr.world->lightGridBounds[0]; // * sizeof(bspGridPoint_t);
	gridStep[2] = tr.world->lightGridBounds[0] * tr.world->lightGridBounds[1]; // * sizeof(bspGridPoint_t);
	gridPoint   = tr.world->lightGridData + pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];

	totalFactor = 0;
	for (i = 0; i < 8; i++)
	{
		float factor = 1.0f;

		gridPoint2 = gridPoint;
		/*for (j = 0; j < 3; j++)
		{
			if (i & (1 << j))
			{
				factor     *= frac[j];
				gridPoint2 += gridStep[j];
			}
			else
			{
				factor *= (1.0f - frac[j]);
			}
		}*/
		// loop j unrolled..
		switch (i)
		{
		case 0:
			factor *= (1.0f - frac[0]);
			factor *= (1.0f - frac[1]);
			factor *= (1.0f - frac[2]);
			break;
		case 1:
			factor *= frac[0];
			gridPoint2 += gridStep[0];
			factor *= (1.0f - frac[1]);
			factor *= (1.0f - frac[2]);
			break;
		case 2:
			factor *= (1.0f - frac[0]);
			factor *= frac[1];
			gridPoint2 += gridStep[1];
			factor *= (1.0f - frac[2]);
			break;
		case 3:
			factor *= frac[0];
			gridPoint2 += gridStep[0];
			factor *= frac[1];
			gridPoint2 += gridStep[1];
			factor *= (1.0f - frac[2]);
			break;
		case 4:
			factor *= (1.0f - frac[0]);
			factor *= (1.0f - frac[1]);
			factor *= frac[2];
			gridPoint2 += gridStep[2];
			break;
		case 5:
			factor *= frac[0];
			gridPoint2 += gridStep[0];
			factor *= (1.0f - frac[1]);
			factor *= frac[2];
			gridPoint2 += gridStep[2];
			break;
		case 6:
			factor *= (1.0f - frac[0]);
			factor *= frac[1];
			gridPoint2 += gridStep[1];
			factor *= frac[2];
			gridPoint2 += gridStep[2];
			break;
		case 7:
			factor *= frac[0];
			gridPoint2 += gridStep[0];
			factor *= frac[1];
			gridPoint2 += gridStep[1];
			factor *= frac[2];
			gridPoint2 += gridStep[2];
			break;
		}


		if (gridPoint2->ambientColor[0] + gridPoint2->ambientColor[1] + gridPoint2->ambientColor[2] == 0.f)
		{
			continue;           // ignore samples in walls
		}

		totalFactor += factor;

		/*ent->ambientLight[0] += factor * gridPoint2->ambientColor[0];
		ent->ambientLight[1] += factor * gridPoint2->ambientColor[1];
		ent->ambientLight[2] += factor * gridPoint2->ambientColor[2];*/
		VectorScale(gridPoint2->ambientColor, factor, vec);
		VectorAdd(ent->ambientLight, vec, ent->ambientLight);

		/*ent->directedLight[0] += factor * gridPoint2->directedColor[0];
		ent->directedLight[1] += factor * gridPoint2->directedColor[1];
		ent->directedLight[2] += factor * gridPoint2->directedColor[2];*/
		VectorScale(gridPoint2->directedColor, factor, vec);
		VectorAdd(ent->directedLight, vec, ent->directedLight);

		VectorMA(direction, factor, gridPoint2->direction, direction);
	}

	//if (totalFactor > 0 && totalFactor < 0.99f) // < 0.99 ?  why not test < 1 ?   when testing < 0.99, when == 1 vectors are scaled by 1 :S
	if (totalFactor > 0 && totalFactor < 1.f)
	{
		//totalFactor = 1.0f / totalFactor;
		RECIPROCAL(totalFactor);
		VectorScale(ent->ambientLight, totalFactor, ent->ambientLight);
		VectorScale(ent->directedLight, totalFactor, ent->directedLight);
	}

	VectorNormalize2Only(direction, ent->lightDir);

	// cheats?  check for single player?
	if (tr.lightGridMulDirected != 0.f)
	{
		VectorScale(ent->directedLight, tr.lightGridMulDirected, ent->directedLight);
	}
	if (tr.lightGridMulAmbient != 0.f)
	{
		VectorScale(ent->ambientLight, tr.lightGridMulAmbient, ent->ambientLight);
	}
}

/**
 * @brief Calculates all the lighting values that will be used by the Calc_* functions
 * @param[in] refdef
 * @param[in,out] ent
 * @param[in] forcedOrigin
 *
 * @note R_SetupEntityLightingGrid deals with forcedOrigin
 */
void R_SetupEntityLighting(const trRefdef_t *refdef, trRefEntity_t *ent, vec3_t forcedOrigin)
{
	float f;

	// lighting calculations
	if (ent->lightingCalculated)
	{
		return;
	}
	ent->lightingCalculated = qtrue;

	// if NOWORLDMODEL, only use dynamic lights (menu system, etc)
	if (!(refdef->rdflags & RDF_NOWORLDMODEL) && tr.world && tr.world->lightGridData)
	{
		R_SetupEntityLightingGrid(ent, forcedOrigin);
	}
	else
	{
		ent->ambientLight[0] = tr.identityLight * (64.0f / 255.0f);
		ent->ambientLight[1] = tr.identityLight * (64.0f / 255.0f);
		ent->ambientLight[2] = tr.identityLight * (96.0f / 255.0f);

		ent->directedLight[0] = tr.identityLight; // * (255.0f / 255.0f);
		ent->directedLight[1] = tr.identityLight * (232.0f / 255.0f);
		ent->directedLight[2] = tr.identityLight * (224.0f / 255.0f);

		//VectorCopy(tr.sunDirection, ent->lightDir);
		VectorSet(ent->lightDir, -1.0f, 1.0f, 1.25f);
		VectorNormalizeOnly(ent->lightDir);
	}

	// ambient light adds
	if (ent->e.hilightIntensity != 0.f)
	{
		// level of intensity was set because the item was looked at
		f = tr.identityLight * (128.0f / 255.0f) * ent->e.hilightIntensity;
		/*ent->ambientLight[0] += f;
		ent->ambientLight[1] += f;
		ent->ambientLight[2] += f;*/
		VectorAddConst(ent->ambientLight, f, ent->ambientLight);
	}
	else if (ent->e.renderfx & RF_MINLIGHT)  // && VectorLength(ent->ambientLight) <= 0)
	{
		// give everything a minimum light add
		f = tr.identityLight * (32.f / 255.0f);
		/*ent->ambientLight[0] += f;
		ent->ambientLight[1] += f;
		ent->ambientLight[2] += f;*/
		VectorAddConst(ent->ambientLight, f, ent->ambientLight);
	}

#if 0
	// clamp ambient
	for (i = 0; i < 3; i++)
	{
		if (ent->ambientLight[i] > tr.identityLight)
		{
			ent->ambientLight[i] = tr.identityLight;
		}
	}
#endif

	// keep it in world space

	// transform the direction to local space
	//d = VectorLength(ent->directedLight);
	//VectorScale(ent->lightDir, d, lightDir);
	//VectorNormalizeOnly(lightDir);

	////ent->lightDir[0] = DotProduct(lightDir, ent->e.axis[0]);
	////ent->lightDir[1] = DotProduct(lightDir, ent->e.axis[1]);
	////ent->lightDir[2] = DotProduct(lightDir, ent->e.axis[2]);
	//Dot(lightDir, ent->e.axis[0], ent->lightDir[0]);
	//Dot(lightDir, ent->e.axis[1], ent->lightDir[1]);
	//Dot(lightDir, ent->e.axis[2], ent->lightDir[2]);


	// force an ambient light value or scale by given r_ambientscale
	// note: this will also affect ambient light for hilightIntensity and RF_MINLIGHT ...
	// ..try to avoid calculating a vectorlength.
	if (r_forceAmbient->value && VectorLength(ent->ambientLight) < r_forceAmbient->value)
	{
		/*ent->ambientLight[0] = r_forceAmbient->value;
		ent->ambientLight[1] = r_forceAmbient->value;
		ent->ambientLight[2] = r_forceAmbient->value;*/
		VectorSet(ent->ambientLight, r_forceAmbient->value, r_forceAmbient->value, r_forceAmbient->value);
	}
	else
	{
		if (refdef->rdflags & RDF_SNOOPERVIEW) // nightscope
		{
			VectorSet(ent->ambientLight, 0.96f, 0.96f, 0.96f);  // allow a little room for flicker from directed light
		}
		else
		{
			if (refdef->rdflags & RDF_NOWORLDMODEL) // no scaling for no world models set world ambient light instead
			{
				VectorCopy(tr.worldEntity.ambientLight, ent->ambientLight);
			}
			VectorScale(ent->ambientLight, r_ambientScale->value, ent->ambientLight);
		}
	}

	if (r_debugLight->integer)
	{
		 Ren_Print("amb: %f %f %f dir: %f %f %f\n",
				 ent->ambientLight[0], ent->ambientLight[1], ent->ambientLight[2],
				 ent->directedLight[0], ent->directedLight[1], ent->directedLight[2]);
	}
}

/**
 * @brief R_SetupLightOrigin
 * @param[in,out] light
 *
 * @note Needs finished transformMatrix
 */
void R_SetupLightOrigin(trRefLight_t *light)
{
	vec3_t transformed;

	if (light->l.rlType == RL_DIRECTIONAL)
	{
		if (!VectorCompare(light->l.center, vec3_origin))
		{
			VectorTransformM4(light->transformMatrix, light->l.center, transformed);
			VectorSubtract(transformed, light->l.origin, light->direction);
			VectorNormalizeOnly(light->direction);
			VectorMA(light->l.origin, 100000.0f, light->direction, light->origin);
		}
		else
		{
			const vec3_t down = { 0.0f, 0.0f, 1.0f };
			VectorTransformM4(light->transformMatrix, down, transformed);
			VectorSubtract(transformed, light->l.origin, light->direction);
			VectorNormalizeOnly(light->direction);
			//VectorMA(light->l.origin, 100000.0f, light->direction, light->origin); // eer?.. the next statement will overwrite light->origin again
			VectorCopy(light->l.origin, light->origin);
		}
	}
	else
	{
		VectorTransformM4(light->transformMatrix, light->l.center, light->origin);
	}
}

/**
 * @brief R_SetupLightLocalBounds
 * @param[in,out] light
 */
void R_SetupLightLocalBounds(trRefLight_t *light)
{
	switch (light->l.rlType)
	{
	case RL_OMNI:
	case RL_DIRECTIONAL:
	{
		/*light->localBounds[0][0] = -light->l.radius[0];
		light->localBounds[0][1] = -light->l.radius[1];
		light->localBounds[0][2] = -light->l.radius[2];*/
		VectorCopy(light->l.radius, light->localBounds[0]);
		VectorInverse(light->localBounds[0]);
		/*light->localBounds[1][0] = light->l.radius[0];
		light->localBounds[1][1] = light->l.radius[1];
		light->localBounds[1][2] = light->l.radius[2];*/
		VectorCopy(light->l.radius, light->localBounds[1]);
		break;
	}
	case RL_PROJ:
	{
		int    j;
		vec3_t farCorners[4];
		//vec4_t			frustum[6];
		vec4_t *frustum = light->localFrustum;

		ClearBounds(light->localBounds[0], light->localBounds[1]);

		// transform frustum from world space to local space
		/*
		for(j = 0; j < 6; j++)
		{
		    VectorCopy(light->frustum[j].normal, frustum[j]);
		    frustum[j][3] = light->frustum[j].dist;

		    MatrixTransformPlane2(light->viewMatrix, frustum[j]);
		}
		*/

		PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[0]);
		PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[1]);
		PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[2]);
		PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[3]);

		if (!VectorCompare(light->l.projStart, vec3_origin))
		{
			vec3_t nearCorners[4];

			// calculate the vertices defining the top area
			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[0]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[1]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[2]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[3]);

			for (j = 0; j < 4; j++)
			{
				AddPointToBounds(farCorners[j], light->localBounds[0], light->localBounds[1]);
				AddPointToBounds(nearCorners[j], light->localBounds[0], light->localBounds[1]);
			}

		}
		else
		{
			vec3_t top;

			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], top);
			AddPointToBounds(top, light->localBounds[0], light->localBounds[1]);

			for (j = 0; j < 4; j++)
			{
				AddPointToBounds(farCorners[j], light->localBounds[0], light->localBounds[1]);
			}
		}
		break;
	}
	default:
		break;
	}

	light->sphereRadius = RadiusFromBounds(light->localBounds[0], light->localBounds[1]);
}

/**
 * @brief R_SetupLightWorldBounds
 * @param[in] light
 *
 * @note Needs finished transformMatrix
 */
void R_SetupLightWorldBounds(trRefLight_t *light)
{
	MatrixTransformBounds(light->transformMatrix, light->localBounds[0], light->localBounds[1], light->worldBounds[0], light->worldBounds[1]);
}

/**
 * @brief R_SetupLightView
 * @param[in] light
 */
void R_SetupLightView(trRefLight_t *light)
{
	switch (light->l.rlType)
	{
	case RL_OMNI:
	case RL_PROJ:
	case RL_DIRECTIONAL:
	{
		MatrixAffineInverse(light->transformMatrix, light->viewMatrix);
		return; // break;
	}
	/*
	case RL_PROJ:
	{
	    mat4_t        viewMatrix;

	    MatrixAffineInverse(light->transformMatrix, viewMatrix);

	    // convert from our coordinate system (looking down X)
	    // to OpenGL's coordinate system (looking down -Z)
	    Matrix4Multiply(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);
	    break;
	}
	*/
	default: // would this ever happen?.. a light of an unknown type
		Ren_Drop("R_SetupLightView: Bad rlType");
		//break;
	}
}

/**
 * @brief R_SetupLightFrustum
 * @param[in,out] light
 */
void R_SetupLightFrustum(trRefLight_t *light)
{
	switch (light->l.rlType)
	{
	case RL_OMNI:
	case RL_DIRECTIONAL:
	{
		int    i;
		axis_t axis;
		vec3_t planeNormal;
		vec3_t planeOrigin;

		quat_to_axis(light->l.rotation, axis);

		for (i = 0; i < 3; i++)
		{
			VectorMA(light->l.origin, light->l.radius[i], axis[i], planeOrigin);
			VectorNegate(axis[i], planeNormal);
			VectorNormalizeOnly(planeNormal);
			VectorCopy(planeNormal, light->frustum[i].normal);
			//light->frustum[i].dist = DotProduct(planeOrigin, planeNormal);
			Dot(planeOrigin, planeNormal, light->frustum[i].dist);
		//}
		//for (i = 0; i < 3; i++)
		//{
			VectorMA(light->l.origin, -light->l.radius[i], axis[i], planeOrigin);
			VectorCopy(axis[i], planeNormal);
			VectorNormalizeOnly(planeNormal);
			VectorCopy(planeNormal, light->frustum[i + 3].normal);
			//light->frustum[i + 3].dist = DotProduct(planeOrigin, planeNormal);
			Dot(planeOrigin, planeNormal, light->frustum[i + 3].dist);
		}

		for (i = 0; i < 6; i++)
		{
			//vec_t length; // , ilength;

			light->frustum[i].type = PLANE_NON_AXIAL;

			// normalize
			/*length = VectorLength(light->frustum[i].normal);
			if (length != 0.f)
			{
				ilength                      = 1.0f / length;
				//light->frustum[i].normal[0] *= ilength;
				//light->frustum[i].normal[1] *= ilength;
				//light->frustum[i].normal[2] *= ilength;
				VectorScale(light->frustum[i].normal, ilength, light->frustum[i].normal);
				light->frustum[i].dist      *= ilength;
			}*/
#if 0
			VectorNorm(light->frustum[i].normal, &length);
			if (length != 0.f)
			{
				//light->frustum[i].dist /= length;
				light->frustum[i].dist *= rcp(length);
			}
#else
			// ^^that^^ is really a Vector4Norm
			// NOTE: vec4 scales the frustum.dist as well
			//       Beware of this! because it is not obvious at first glance..
			Vector4NormalizeOnly(light->frustum[i].normal);
#endif
			SetPlaneSignbits(&light->frustum[i]);
		}
		break;
	}
	case RL_PROJ:
	{
		int    i;
		vec4_t worldFrustum[6];

		// transform local frustum to world space
		for (i = 0; i < 6; i++)
		{
			MatrixTransformPlane(light->transformMatrix, light->localFrustum[i], worldFrustum[i]);
		}

		// normalize all frustum planes
		for (i = 0; i < 6; i++)
		{
			PlaneNormalize(worldFrustum[i]);

			/*VectorCopy(worldFrustum[i], light->frustum[i].normal);
			light->frustum[i].dist = worldFrustum[i][3];*/
			Vector4Copy(worldFrustum[i], light->frustum[i].normal); // normal & dist are stored  in this packed memory order..

			light->frustum[i].type = PLANE_NON_AXIAL;

			SetPlaneSignbits(&light->frustum[i]);
		}
		break;
	}
	default:
		break;
	}

	if (light->isStatic)
	{
		int           i, j, i3;
		vec4_t        quadVerts[4];
		srfVert_t     *verts;
		srfTriangle_t *triangles;

		tess.multiDrawPrimitives = 0;
		tess.numIndexes          = 0;
		tess.numVertexes         = 0;

		switch (light->l.rlType)
		{
		case RL_OMNI:
		case RL_DIRECTIONAL:
		{
			vec3_t worldBounds[2];

#ifndef ETL_SSE
			VectorTransformM4(light->transformMatrix, light->localBounds[0], worldBounds[0]);
			VectorTransformM4(light->transformMatrix, light->localBounds[1], worldBounds[1]);
#else
			__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6;
			xmm3 = _mm_loadu_ps((const float *)&light->transformMatrix[0]);
			xmm4 = _mm_loadu_ps((const float *)&light->transformMatrix[4]);
			xmm5 = _mm_loadu_ps((const float *)&light->transformMatrix[8]);
			xmm6 = _mm_loadu_ps((const float *)&light->transformMatrix[12]);

			xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)&light->localBounds[0][0]), (const __m64 *)&light->localBounds[0][1]);
			xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111);
			xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010);
			xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b00000000);
			xmm0 = _mm_mul_ps(xmm0, xmm3);
			xmm1 = _mm_mul_ps(xmm1, xmm4);
			xmm2 = _mm_mul_ps(xmm2, xmm5);
			xmm0 = _mm_add_ps(xmm0, xmm1);
			xmm0 = _mm_add_ps(xmm0, xmm2);
			xmm0 = _mm_add_ps(xmm0, xmm6);
			xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b10010100);
			_mm_store_ss((float *)&worldBounds[0][0], xmm0);
			_mm_storeh_pi((__m64 *)&worldBounds[0][1], xmm0);

			xmm1 = _mm_loadh_pi(_mm_load_ss((const float *)&light->localBounds[1][0]), (const __m64 *)&light->localBounds[1][1]);
			xmm2 = _mm_shuffle_ps(xmm1, xmm1, 0b11111111);
			xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0b10101010);
			xmm0 = _mm_shuffle_ps(xmm1, xmm1, 0b00000000);
			xmm0 = _mm_mul_ps(xmm0, xmm3);
			xmm1 = _mm_mul_ps(xmm1, xmm4);
			xmm2 = _mm_mul_ps(xmm2, xmm5);
			xmm0 = _mm_add_ps(xmm0, xmm1);
			xmm0 = _mm_add_ps(xmm0, xmm2);
			xmm0 = _mm_add_ps(xmm0, xmm6);
			xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b10010100);
			_mm_store_ss((float *)&worldBounds[1][0], xmm0);
			_mm_storeh_pi((__m64 *)&worldBounds[1][1], xmm0);
#endif

			Tess_AddCube(vec3_origin, worldBounds[0], worldBounds[1], colorWhite);

			verts     = (srfVert_t *)ri.Hunk_AllocateTempMemory(tess.numVertexes * sizeof(srfVert_t));
			triangles = (srfTriangle_t *)ri.Hunk_AllocateTempMemory((tess.numIndexes / 3) * sizeof(srfTriangle_t));

			for (i = 0; i < tess.numVertexes; i++)
			{
				VectorCopy(tess.xyz[i], verts[i].xyz);	// vec4 to vec3 :S
			}

			/*for (i = 0; i < (tess.numIndexes / 3); i++)
			{
				triangles[i].indexes[0] = tess.indexes[i * 3 + 0];
				triangles[i].indexes[1] = tess.indexes[i * 3 + 1];
				triangles[i].indexes[2] = tess.indexes[i * 3 + 2];
			}*/
			for (i = 0, i3 = 0; i3 < tess.numIndexes; i++, i3 += 3)
			{
				triangles[i].indexes[0] = tess.indexes[i3 + 0];
				triangles[i].indexes[1] = tess.indexes[i3 + 1];
				triangles[i].indexes[2] = tess.indexes[i3 + 2];
			}

			light->frustumVBO = R_CreateVBO2("staticLightFrustum_VBO", tess.numVertexes, verts, ATTR_POSITION, VBO_USAGE_STATIC);
			light->frustumIBO = R_CreateIBO2("staticLightFrustum_IBO", tess.numIndexes / 3, triangles, VBO_USAGE_STATIC);

			ri.Hunk_FreeTempMemory(triangles);
			ri.Hunk_FreeTempMemory(verts);

			light->frustumVerts   = tess.numVertexes;
			light->frustumIndexes = tess.numIndexes;
			break;
		}
		case RL_PROJ:
		{
			vec3_t farCorners[4];
			vec4_t frustum[6];

			// transform local frustum to world space
			for (i = 0; i < 6; i++)
			{
				MatrixTransformPlane(light->transformMatrix, light->localFrustum[i], frustum[i]);
			}

			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[0]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[1]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[2]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[3]);

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
					Vector4Set(quadVerts[3], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
					Vector4Set(quadVerts[2], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
					Vector4Set(quadVerts[1], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
					Vector4Set(quadVerts[0], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
					Tess_AddQuadStamp2(quadVerts, colorCyan);
				}

				// draw far cap
				Vector4Set(quadVerts[0], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
				Vector4Set(quadVerts[1], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
				Vector4Set(quadVerts[2], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
				Vector4Set(quadVerts[3], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
				Tess_AddQuadStamp2(quadVerts, colorRed);

				// draw near cap
				Vector4Set(quadVerts[3], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
				Vector4Set(quadVerts[2], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
				Vector4Set(quadVerts[1], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
				Vector4Set(quadVerts[0], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
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
					VectorCopy(top, tess.xyz[tess.numVertexes]);
					Vector4Copy(colorCyan, tess.colors[tess.numVertexes]);
					tess.indexes[tess.numIndexes++] = tess.numVertexes;
					tess.numVertexes++;

					VectorCopy(farCorners[(j + 1) % 4], tess.xyz[tess.numVertexes]);
					Vector4Copy(colorCyan, tess.colors[tess.numVertexes]);
					tess.indexes[tess.numIndexes++] = tess.numVertexes;
					tess.numVertexes++;

					VectorCopy(farCorners[j], tess.xyz[tess.numVertexes]);
					Vector4Copy(colorCyan, tess.colors[tess.numVertexes]);
					tess.indexes[tess.numIndexes++] = tess.numVertexes;
					tess.numVertexes++;
				}

				Vector4Set(quadVerts[0], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1.0f);
				Vector4Set(quadVerts[1], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1.0f);
				Vector4Set(quadVerts[2], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1.0f);
				Vector4Set(quadVerts[3], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1.0f);
				Tess_AddQuadStamp2(quadVerts, colorRed);
			}

			verts     = (srfVert_t *)ri.Hunk_AllocateTempMemory(tess.numVertexes * sizeof(srfVert_t));
			triangles = (srfTriangle_t *)ri.Hunk_AllocateTempMemory((tess.numIndexes / 3) * sizeof(srfTriangle_t));

			for (i = 0; i < tess.numVertexes; i++)
			{
				VectorCopy(tess.xyz[i], verts[i].xyz);
			}

			/*for (i = 0; i < (tess.numIndexes / 3); i++)
			{
				triangles[i].indexes[0] = tess.indexes[i * 3 + 0];
				triangles[i].indexes[1] = tess.indexes[i * 3 + 1];
				triangles[i].indexes[2] = tess.indexes[i * 3 + 2];
			}*/
			for (i = 0, i3 = 0; i3 < tess.numIndexes; i++, i3 += 3)
			{
				triangles[i].indexes[0] = tess.indexes[i3 + 0];
				triangles[i].indexes[1] = tess.indexes[i3 + 1];
				triangles[i].indexes[2] = tess.indexes[i3 + 2];
			}

			light->frustumVBO = R_CreateVBO2("staticLightFrustum_VBO", tess.numVertexes, verts, ATTR_POSITION, VBO_USAGE_STATIC);
			light->frustumIBO = R_CreateIBO2("staticLightFrustum_IBO", tess.numIndexes / 3, triangles, VBO_USAGE_STATIC);

			ri.Hunk_FreeTempMemory(triangles);
			ri.Hunk_FreeTempMemory(verts);

			light->frustumVerts   = tess.numVertexes;
			light->frustumIndexes = tess.numIndexes;
			break;
		}
		default:
			break;
		}

		tess.multiDrawPrimitives = 0;
		tess.numIndexes          = 0;
		tess.numVertexes         = 0;
	}
}

// *INDENT-OFF*
/**
 * @brief R_SetupLightProjection
 * @param[in,out] light
 */
void R_SetupLightProjection(trRefLight_t *light)
{
	switch (light->l.rlType)
	{
	case RL_OMNI:
	case RL_DIRECTIONAL:
	{
		//Matrix4IdentTranslate(light->projectionMatrix, 1.0f / light->l.radius[0], 1.0f / light->l.radius[1], 1.0f / light->l.radius[2]);
		Matrix4IdentTranslate(light->projectionMatrix, rcp(light->l.radius[0]), rcp(light->l.radius[1]), rcp(light->l.radius[2]));
		break;
	}
	case RL_PROJ:
	{
		//int    i;
		float  *proj    = light->projectionMatrix;
		vec4_t *frustum = light->localFrustum;
		vec4_t lightProject[4];
		vec3_t right, up, normal;
		vec3_t start, stop;
		vec3_t falloff;
		float  falloffLen;
		float  rLen;
		float  uLen;
		float  a, b, ofs, dist, dot;
		vec4_t targetGlobal;

		// This transformation remaps the X,Y coordinates from [-1..1] to [0..1],
		// presumably needed because the up/right vectors extend symmetrically
		// either side of the target point.
		//MatrixSetupTranslation(proj, 0.5f, 0.5f, 0);
		//MatrixMultiplyScale(proj, 0.5f, 0.5f, 1);

		rLen = VectorNormalize2(light->l.projRight, right);
		uLen = VectorNormalize2(light->l.projUp, up);

		CrossProduct(up, right, normal);
		VectorNormalizeOnly(normal);

		//dist = DotProduct(light->l.projTarget, normal);
		Dot(light->l.projTarget, normal, dist);
		if (dist < 0.f)
		{
			dist = -dist;
			VectorInverse(normal);
		}

		VectorScale(right, (0.5f * dist) / rLen, right);
		VectorScale(up, -(0.5f * dist) / uLen, up);

		Vector4Set(lightProject[0], right[0], right[1], right[2], 0.0f);
		Vector4Set(lightProject[1], up[0], up[1], up[2], 0.0f);
		Vector4Set(lightProject[2], normal[0], normal[1], normal[2], 0.0f);

		// now offset to center
		VectorCopy(light->l.projTarget, targetGlobal);
		targetGlobal[3] = 1.0f;
		{
			a   = DotProduct4(targetGlobal, lightProject[0]);
			b   = DotProduct4(targetGlobal, lightProject[2]);
			ofs = 0.5f - a / b;

			Vector4MA(lightProject[0], ofs, lightProject[2], lightProject[0]);
		}
		{
			a   = DotProduct4(targetGlobal, lightProject[1]);
			b   = DotProduct4(targetGlobal, lightProject[2]);
			ofs = 0.5f - a / b;

			Vector4MA(lightProject[1], ofs, lightProject[2], lightProject[1]);
		}

		if (!VectorCompare(light->l.projStart, vec3_origin))
		{
			VectorCopy(light->l.projStart, start);
		}
		else
		{
			VectorClear(start);
		}

		if (!VectorCompare(light->l.projEnd, vec3_origin))
		{
			VectorCopy(light->l.projEnd, stop);
		}
		else
		{
			VectorCopy(light->l.projTarget, stop);
		}

		// Calculate the falloff vector
		VectorSubtract(stop, start, falloff);
		//light->falloffLength = falloffLen = VectorNormalize(falloff);
		VectorNorm(falloff, &falloffLen);
		light->falloffLength = falloffLen;
		if (falloffLen <= 0.0f)
		{
			falloffLen = 1.0f;
		}
		//FIXME ?
		//VectorScale(falloff, 1.0f / falloffLen, falloff);
		VectorScale(falloff, rcp(falloffLen), falloff);

		//light->falloffLength = 1.0f;

		//Vector4Set(lightProject[3], falloff[0], falloff[1], falloff[2], -DotProduct(start, falloff));
		Dot(start, falloff, dot);
		Vector4Set(lightProject[3], falloff[0], falloff[1], falloff[2], -dot);

		// we want the planes of s=0, s=q, t=0, and t=q
		Vector4Copy(lightProject[0], frustum[FRUSTUM_LEFT]);
		Vector4Copy(lightProject[1], frustum[FRUSTUM_BOTTOM]);

		VectorSubtract(lightProject[2], lightProject[0], frustum[FRUSTUM_RIGHT]);
		frustum[FRUSTUM_RIGHT][3] = lightProject[2][3] - lightProject[0][3];

		VectorSubtract(lightProject[2], lightProject[1], frustum[FRUSTUM_TOP]);
		frustum[FRUSTUM_TOP][3] = lightProject[2][3] - lightProject[1][3];

		// we want the planes of s=0 and s=1 for front and rear clipping planes
		VectorCopy(lightProject[3], frustum[FRUSTUM_NEAR]);
		frustum[FRUSTUM_NEAR][3] = lightProject[3][3];

		VectorNegate(lightProject[3], frustum[FRUSTUM_FAR]);
		frustum[FRUSTUM_FAR][3] = -lightProject[3][3] - 1.0f;

#if 0
		Ren_Print("light_target: (%5.3f, %5.3f, %5.3f)\n", light->l.projTarget[0], light->l.projTarget[1], light->l.projTarget[2]);
		Ren_Print("light_right: (%5.3f, %5.3f, %5.3f)\n", light->l.projRight[0], light->l.projRight[1], light->l.projRight[2]);
		Ren_Print("light_up: (%5.3f, %5.3f, %5.3f)\n", light->l.projUp[0], light->l.projUp[1], light->l.projUp[2]);
		Ren_Print("light_start: (%5.3f, %5.3f, %5.3f)\n", light->l.projStart[0], light->l.projStart[1], light->l.projStart[2]);
		Ren_Print("light_end: (%5.3f, %5.3f, %5.3f)\n", light->l.projEnd[0], light->l.projEnd[1], light->l.projEnd[2]);

		Ren_Print("unnormalized frustum:\n");
		for (i = 0; i < 6; i++)
			Ren_Print("(%5.6f, %5.6f, %5.6f, %5.6f)\n", frustum[i][0], frustum[i][1], frustum[i][2], frustum[i][3]);
#endif

		// calculate the new projection matrix from the frustum planes
		MatrixFromPlanes(proj, frustum[FRUSTUM_LEFT], frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], frustum[FRUSTUM_FAR]);

		//MatrixMultiply2(proj, newProjection);

		// scale the falloff texture coordinate so that 0.5 is at the apex and 0.0
		// as at the base of the pyramid.
		// TODO: I don't like hacking the matrix like this, but all attempts to use
		// a transformation seemed to affect too many other things.
		//proj[10] *= 0.5f;

		// normalise all frustum planes
		PlaneNormalize(frustum[FRUSTUM_LEFT]);
		PlaneNormalize(frustum[FRUSTUM_RIGHT]);
		PlaneNormalize(frustum[FRUSTUM_BOTTOM]);
		PlaneNormalize(frustum[FRUSTUM_TOP]);
		PlaneNormalize(frustum[FRUSTUM_NEAR]);
		PlaneNormalize(frustum[FRUSTUM_FAR]);

#if 0
		Ren_Print("normalized frustum:\n");
		for (i = 0; i < 6; i++)
			Ren_Print("(%5.3f, %5.3f, %5.3f, %5.3f)\n", light->frustum[i].normal[0], frustum[i][1], frustum[i][2], frustum[i][3]);
#endif
		break;
	}

	default:
		Ren_Drop("R_SetupLightProjection: Bad rlType");
		//break;
	}
}
// *INDENT-ON*

/**
 * @brief R_AddLightInteraction
 * @param[in,out] light
 * @param[in] surface
 * @param[in] surfaceShader
 * @param[in] cubeSideBits
 * @param[in] iaType
 * @return
 */
qboolean R_AddLightInteraction(trRefLight_t *light, surfaceType_t *surface, shader_t *surfaceShader, byte cubeSideBits,
                               interactionType_t iaType)
{
	int           iaIndex;
	interaction_t *ia;

	// skip all surfaces that don't matter for lighting only pass
	if (surfaceShader)
	{
		if (surfaceShader->isSky || (!surfaceShader->interactLight && surfaceShader->noShadows))
		{
			return qfalse;
		}
	}

/*
	// test: don't interact on the same >1..
	// update: If you enable this code, playermodel-shadows are not all/fully drawn.
	for (int i = 0; i < tr.refdef.numInteractions; i++)
	{
		ia = &tr.refdef.interactions[i];
		if (ia->surface == surface && ia->surfaceShader == surfaceShader && ia->light == light) {
			return qfalse;
		}
	}
*/
	// instead of checking for overflow, we just mask the index
	 // so it wraps around
	iaIndex = tr.refdef.numInteractions & INTERACTION_MASK;
	ia      = &tr.refdef.interactions[iaIndex];
	tr.refdef.numInteractions++;

	light->noSort = iaIndex = 0;

	// connect to interaction grid
	if (!light->firstInteraction)
	{
		light->firstInteraction = ia;
	}

	if (light->lastInteraction)
	{
		light->lastInteraction->next = ia;
	}

	light->lastInteraction = ia;

	// update counters
	light->numInteractions++;

	switch (iaType)
	{
	case IA_SHADOWONLY:
		light->numShadowOnlyInteractions++;
		break;
	case IA_LIGHTONLY:
		light->numLightOnlyInteractions++;
		break;
	default:
		break;
	}

	ia->next = NULL;

	ia->type = iaType;

	ia->light         = light;
	ia->entity        = tr.currentEntity;
	ia->surface       = surface;
	ia->surfaceShader = surfaceShader;

	ia->cubeSideBits = cubeSideBits;

	ia->scissorX      = light->scissor.coords[0];
	ia->scissorY      = light->scissor.coords[1];
	ia->scissorWidth  = light->scissor.coords[2] - light->scissor.coords[0];
	ia->scissorHeight = light->scissor.coords[3] - light->scissor.coords[1];

	/*
	if(r_shadows->integer == SHADOWING_STENCIL && glDepthBoundsEXT)
	{
	    ia->depthNear = light->depthNear;
	    ia->depthFar = light->depthFar;
	    ia->noDepthBoundsTest = light->noDepthBoundsTest;
	}
	*/

	if (glConfig2.occlusionQueryAvailable)
	{
		ia->noOcclusionQueries = light->noOcclusionQueries;
	}

	if (light->isStatic)
	{
		tr.pc.c_slightInteractions++;
	}
	else
	{
		tr.pc.c_dlightInteractions++;
	}

	return qtrue;
}

/**
 * @brief Compare function for qsort()
 * @param[in] a
 * @param[in] b
 * @return
 */
static int InteractionCompare(const void *a, const void *b)
{
#if 1
	// shader first
	if (((const interaction_t *) a)->surfaceShader < ((const interaction_t *) b)->surfaceShader)
	{
		return -1;
	}
	else if (((const interaction_t *) a)->surfaceShader > ((const interaction_t *) b)->surfaceShader)
	{
		return 1;
	}
#endif

#if 1
	// then entity
	if (((const interaction_t *) a)->entity == &tr.worldEntity && ((const interaction_t *) b)->entity != &tr.worldEntity)
	{
		return -1;
	}
	else if (((const interaction_t *) a)->entity != &tr.worldEntity && ((const interaction_t *) b)->entity == &tr.worldEntity)
	{
		return 1;
	}
	else if (((const interaction_t *) a)->entity < ((const interaction_t *) b)->entity)
	{
		return -1;
	}
	else if (((const interaction_t *) a)->entity > ((const interaction_t *) b)->entity)
	{
		return 1;
	}
#endif

	return 0;
}

/**
 * @brief R_SortInteractions
 * @param[in] light
 */
void R_SortInteractions(trRefLight_t *light)
{
	int           i;
	int           iaFirstIndex;
	interaction_t *iaFirst;
	interaction_t *ia;
	interaction_t *iaLast;

	if (r_noInteractionSort->integer)
	{
		return;
	}

	if (!light->numInteractions || light->noSort)
	{
		return;
	}

	iaFirst      = light->firstInteraction;
	iaFirstIndex = light->firstInteraction - tr.refdef.interactions;

	// sort by material etc. for geometry batching in the renderer backend
	qsort(iaFirst, light->numInteractions, sizeof(interaction_t), InteractionCompare);

	// fix linked list
	iaLast = NULL;
	for (i = 0; i < light->numInteractions; i++)
	{
		ia = &tr.refdef.interactions[iaFirstIndex + i];

		if (iaLast)
		{
			iaLast->next = ia;
		}

		ia->next = NULL;

		iaLast = ia;
	}
}

/**
 * @brief R_IntersectRayPlane
 * @param[in] v1
 * @param[in] v2
 * @param[in] plane
 * @param[out] res
 */
ID_INLINE void R_IntersectRayPlane(const vec3_t v1, const vec3_t v2, cplane_t *plane, vec3_t res)
{
	vec3_t v;
	float  sect, dotnv1, dotnv;
	VectorSubtract(v1, v2, v);
	//sect = -(DotProduct(plane->normal, v1) - plane->dist) / DotProduct(plane->normal, v);
	Dot(plane->normal, v1, dotnv1);
	Dot(plane->normal, v, dotnv);
	sect = -(dotnv1 - plane->dist) / dotnv;
	VectorScale(v, sect, v);
	VectorAdd(v1, v, res);
}

/**
 * @brief R_TransformWorldToClip
 * @param[in] src
 * @param[in] cameraViewMatrix
 * @param[in] projectionMatrix
 * @param[in] eye
 * @param[out] dst
 * unused
 * /
void R_TransformWorldToClip(const vec3_t src, const float *cameraViewMatrix, const float *projectionMatrix, vec4_t eye,	vec4_t dst)
{
	//vec4_t src2;
	//VectorCopy(src, src2);
	//src2[3] = 1.0f;
	vec4_t src2 = {0.f, 0.f, 0.f, 1.f};
	VectorCopy(src, src2);
	Vector4TransformM4(cameraViewMatrix, src2, eye);
	Vector4TransformM4(projectionMatrix, eye, dst);
}*/

/**
 * @brief R_AddPointToLightScissor
 * @param[in,out] light
 * @param[in] world
 */
#pragma warning(disable:4700)
ID_INLINE void R_AddPointToLightScissor(trRefLight_t *light, const vec3_t world)
{
	vec4_t eye, clip, normalized, window;
	vec4_t src2 = {0.f, 0.f, 0.f, 1.f};
	int windowInt[2];

	///R_TransformWorldToClip(world, tr.viewParms.world.viewMatrix, tr.viewParms.projectionMatrix, eye, clip);
	VectorCopy(world, src2); //Vector4Set(src2, world[0], world[1], world[2], 1.f);
	Vector4TransformM4(tr.viewParms.world.viewMatrix, src2, eye);
	Vector4TransformM4(tr.viewParms.projectionMatrix, eye, clip);
	 
	///R_TransformClipToWindow(clip, &tr.viewParms, normalized, window);
	Vector2Scale(&clip[0], rcp(clip[3]), &normalized[0]);
	//window[0] = ((1.0f + normalized[0]) * 0.5f * (float)tr.viewParms.viewportWidth) + (float)tr.viewParms.viewportX;
	//window[1] = ((1.0f + normalized[1]) * 0.5f * (float)tr.viewParms.viewportHeight) + (float)tr.viewParms.viewportY;
	Vector2AddConst(normalized, 1.f, window);
	Vector2Scale(window, 0.5f, window);
	window[0] = window[0] * (float)tr.viewParms.viewportWidth + (float)tr.viewParms.viewportX;
	window[1] = window[1] * (float)tr.viewParms.viewportHeight + (float)tr.viewParms.viewportY;

	//window[0] = (int)(window[0] + 0.5f);
	//window[1] = (int)(window[1] + 0.5f);
	Vector2AddConst(window, 0.5f, window);
	windowInt[0] = (int)window[0]; // eeer..
	windowInt[1] = (int)window[1];

	if (light->scissor.coords[2] < windowInt[0])
	{
		light->scissor.coords[2] = windowInt[0];
	}
	if (light->scissor.coords[0] > windowInt[0])
	{
		light->scissor.coords[0] = windowInt[0];
	}
	if (light->scissor.coords[3] < windowInt[1])
	{
		light->scissor.coords[3] = windowInt[1];
	}
	if (light->scissor.coords[1] > windowInt[1])
	{
		light->scissor.coords[1] = windowInt[1];
	}
#pragma warning(default:4700)
}

/**
 * @brief R_AddEdgeToLightScissor
 * @param[in,out] light
 * @param[in] local1
 * @param[in] local2
 */
ID_INLINE void R_AddEdgeToLightScissor(trRefLight_t *light, vec3_t local1, vec3_t local2)
{
	int      i;
	qboolean side1, side2;
	float    dot;
	cplane_t *frust;
	vec3_t   world1, world2, intersect;

	VectorTransformM4(tr.orientation.transformMatrix, local1, world1); //R_LocalPointToWorld(local1, world1);
	VectorTransformM4(tr.orientation.transformMatrix, local2, world2); //R_LocalPointToWorld(local2, world2);

	for (i = 0; i < FRUSTUM_PLANES; i++)
	{
/*		// note: i moved these next two lines out of the loop..
		//R_LocalPointToWorld(local1, world1);
		VectorTransformM4(tr.orientation.transformMatrix, local1, world1);

		//R_LocalPointToWorld(local2, world2);
		VectorTransformM4(tr.orientation.transformMatrix, local2, world2);*/

		frust = &tr.viewParms.frustums[0][i];

		// check edge to frustrum plane
		Dot(frust->normal, world1, dot);
		side1 = (dot - frust->dist >= 0.0f);
		Dot(frust->normal, world2, dot);
		side2 = (dot - frust->dist >= 0.0f);

		if (!side1 || !side2)
		{
			if (glConfig2.occlusionQueryAvailable && i == FRUSTUM_NEAR)
			{
				light->noOcclusionQueries = qtrue;
			}
			if (!side1 && !side2)
			{
				continue; // edge behind plane
			}
			R_IntersectRayPlane(world1, world2, frust, intersect);
			if (!side1)
			{
				R_AddPointToLightScissor(light, intersect);
				R_AddPointToLightScissor(light, world2);
			}
			else // if (!side2)
			{
				R_AddPointToLightScissor(light, world1);
				R_AddPointToLightScissor(light, intersect);
			}
		}
	}
}

/**
 * @brief Recturns the screen space rectangle taken by the box.
 * (Clips the box to the near plane to have correct results even if the box intersects the near plane)
 * - recoded from Tenebrae2
 * @param[in,out] light
 */
void R_SetupLightScissor(trRefLight_t *light)
{
	vec3_t v1, v3, v4, v5, v6, v7, v8, v9;
	int w = tr.viewParms.viewportX + tr.viewParms.viewportWidth,
		h = tr.viewParms.viewportY + tr.viewParms.viewportHeight;

	light->scissor.coords[0] = tr.viewParms.viewportX;
	light->scissor.coords[1] = tr.viewParms.viewportY;
	light->scissor.coords[2] = w;
	light->scissor.coords[3] = h;

	light->clipsNearPlane = (BoxOnPlaneSide(light->worldBounds[0], light->worldBounds[1], &tr.viewParms.frustums[0][FRUSTUM_NEAR]) == 3);

	// check if the light volume clips against the near plane
	if (r_noLightScissors->integer || light->clipsNearPlane)
	{
		if (glConfig2.occlusionQueryAvailable)
		{
			light->noOcclusionQueries = qtrue;
		}
		return;
	}

	if (glConfig2.occlusionQueryAvailable)
	{
		light->noOcclusionQueries = qfalse;
	}

	if (!r_occludeBsp->integer)
	{
		// don't calculate the light scissors because there are up to 500 realtime lights in the view frustum
		// that were not killed by the PVS
		return;
	}

	// transform local light corners to world space -> eye space -> clip space -> window space
	// and extend the light scissor's mins maxs by resulting window coords
	light->scissor.coords[0] = 100000000;
	light->scissor.coords[1] = 100000000;
	light->scissor.coords[2] = -100000000;
	light->scissor.coords[3] = -100000000;

	switch (light->l.rlType)
	{
	case RL_OMNI:
	{
// There's really a lot of copying data in this function.. and also lots of function calls. => inline
// Also a lot of double stuff :S  same verts copied
/*
		// top plane
		//VectorSet(v1, light->localBounds[1][0], light->localBounds[1][1], light->localBounds[1][2]);
		VectorCopy(light->localBounds[1], v1);
		VectorSet(v3, light->localBounds[0][0], light->localBounds[1][1], light->localBounds[1][2]);
		R_AddEdgeToLightScissor(light, v1, v3);

		//VectorSet(v1, light->localBounds[1][0], light->localBounds[1][1], light->localBounds[1][2]);
		VectorSet(v4, light->localBounds[1][0], light->localBounds[0][1], light->localBounds[1][2]);
		R_AddEdgeToLightScissor(light, v1, v4);

		VectorSet(v5, light->localBounds[0][0], light->localBounds[0][1], light->localBounds[1][2]);
//		VectorSet(v2, light->localBounds[0][0], light->localBounds[1][1], light->localBounds[1][2]);
//		R_AddEdgeToLightScissor(light, v1, v2);
		R_AddEdgeToLightScissor(light, v5, v3);

//		VectorSet(v1, light->localBounds[0][0], light->localBounds[0][1], light->localBounds[1][2]);
//		VectorSet(v2, light->localBounds[1][0], light->localBounds[0][1], light->localBounds[1][2]);
//		R_AddEdgeToLightScissor(light, v1, v2);
		R_AddEdgeToLightScissor(light, v5, v4);

		// bottom plane
		VectorSet(v7, light->localBounds[1][0], light->localBounds[1][1], light->localBounds[0][2]);
		VectorSet(v8, light->localBounds[0][0], light->localBounds[1][1], light->localBounds[0][2]);
		R_AddEdgeToLightScissor(light, v7, v8);

//		VectorSet(v1, light->localBounds[1][0], light->localBounds[1][1], light->localBounds[0][2]);
		VectorSet(v9, light->localBounds[1][0], light->localBounds[0][1], light->localBounds[0][2]);
		R_AddEdgeToLightScissor(light, v7, v9);

		//VectorSet(v1, light->localBounds[0][0], light->localBounds[0][1], light->localBounds[0][2]);
		VectorCopy(light->localBounds[0], v6);
//		VectorSet(v2, light->localBounds[0][0], light->localBounds[1][1], light->localBounds[0][2]);
		R_AddEdgeToLightScissor(light, v6, v8);

		//VectorSet(v1, light->localBounds[0][0], light->localBounds[0][1], light->localBounds[0][2]);
//		VectorSet(v2, light->localBounds[1][0], light->localBounds[0][1], light->localBounds[0][2]);
		R_AddEdgeToLightScissor(light, v1, v9);

		// sides
//		VectorSet(v1, light->localBounds[0][0], light->localBounds[1][1], light->localBounds[0][2]);
//		VectorSet(v2, light->localBounds[0][0], light->localBounds[1][1], light->localBounds[1][2]);
//		R_AddEdgeToLightScissor(light, v1, v2);
		R_AddEdgeToLightScissor(light, v8, v3);

//		VectorSet(v1, light->localBounds[1][0], light->localBounds[1][1], light->localBounds[0][2]);
		//VectorSet(v2, light->localBounds[1][0], light->localBounds[1][1], light->localBounds[1][2]);
//		VectorCopy(light->localBounds[1], v2);
		R_AddEdgeToLightScissor(light, v7, v1);

		//VectorSet(v1, light->localBounds[0][0], light->localBounds[0][1], light->localBounds[0][2]);
//		VectorCopy(light->localBounds[0], v1);
//		VectorSet(v2, light->localBounds[0][0], light->localBounds[0][1], light->localBounds[1][2]);
//		R_AddEdgeToLightScissor(light, v1, v2);
		R_AddEdgeToLightScissor(light, v6, v5);

//		VectorSet(v1, light->localBounds[1][0], light->localBounds[0][1], light->localBounds[0][2]);
//		VectorSet(v2, light->localBounds[1][0], light->localBounds[0][1], light->localBounds[1][2]);
//		R_AddEdgeToLightScissor(light, v1, v2);
		R_AddEdgeToLightScissor(light, v9, v4);
*/
		// here's the same as the commented code ^^above.. but without the comments
		VectorCopy(light->localBounds[0], v6);
		VectorSet(v5, light->localBounds[0][0], light->localBounds[0][1], light->localBounds[1][2]);
		VectorSet(v8, light->localBounds[0][0], light->localBounds[1][1], light->localBounds[0][2]);
		VectorSet(v3, light->localBounds[0][0], light->localBounds[1][1], light->localBounds[1][2]);
		VectorSet(v9, light->localBounds[1][0], light->localBounds[0][1], light->localBounds[0][2]);
		VectorSet(v4, light->localBounds[1][0], light->localBounds[0][1], light->localBounds[1][2]);
		VectorSet(v7, light->localBounds[1][0], light->localBounds[1][1], light->localBounds[0][2]);
		VectorCopy(light->localBounds[1], v1);
		R_AddEdgeToLightScissor(light, v1, v3);
		R_AddEdgeToLightScissor(light, v1, v4);
		R_AddEdgeToLightScissor(light, v5, v3);
		R_AddEdgeToLightScissor(light, v5, v4);
		R_AddEdgeToLightScissor(light, v7, v8);
		R_AddEdgeToLightScissor(light, v7, v9);
		R_AddEdgeToLightScissor(light, v6, v8);
		R_AddEdgeToLightScissor(light, v1, v9);
		R_AddEdgeToLightScissor(light, v8, v3);
		R_AddEdgeToLightScissor(light, v7, v1);
		R_AddEdgeToLightScissor(light, v6, v5);
		R_AddEdgeToLightScissor(light, v9, v4);
		break;
	}
	case RL_PROJ:
	{
		int    j;
		vec3_t farCorners[4];
		vec4_t *frustum = light->localFrustum;

		PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[0]);
		PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[1]);
		PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[2]);
		PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[3]);
#if 1
		if (!VectorCompare(light->l.projStart, vec3_origin))
		{
			vec3_t nearCorners[4];

			// calculate the vertices defining the top area
			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[0]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[1]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[2]);
			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[3]);

			for (j = 0; j < 4; j++)
			{
				// outer quad
				R_AddEdgeToLightScissor(light, nearCorners[j], farCorners[j]);
				R_AddEdgeToLightScissor(light, farCorners[j], farCorners[(j + 1) % 4]);
				R_AddEdgeToLightScissor(light, farCorners[(j + 1) % 4], nearCorners[(j + 1) % 4]);
				R_AddEdgeToLightScissor(light, nearCorners[(j + 1) % 4], nearCorners[j]);

				// far cap
				R_AddEdgeToLightScissor(light, farCorners[j], farCorners[(j + 1) % 4]);

				// near cap
				R_AddEdgeToLightScissor(light, nearCorners[j], nearCorners[(j + 1) % 4]);
			}
		}
		else
#endif
		{
			vec3_t top;

			// no light_start, just use the top vertex (doesn't need to be mirrored)
			PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], top);

			for (j = 0; j < 4; j++)
			{
				R_AddEdgeToLightScissor(light, farCorners[j], farCorners[(j + 1) % 4]);
				R_AddEdgeToLightScissor(light, top, farCorners[j]);
			}
		}
		break;
	}
	default:
		break;
	}

	Q_clamp(light->scissor.coords[0], tr.viewParms.viewportX, w);
	Q_clamp(light->scissor.coords[2], tr.viewParms.viewportX, w);

	Q_clamp(light->scissor.coords[1], tr.viewParms.viewportY, h);
	Q_clamp(light->scissor.coords[3], tr.viewParms.viewportY, h);
}

/*
 * @brief R_SetupLightDepthBounds
 * @param[in,out] light
 *
 * @note Unused
void R_SetupLightDepthBounds(trRefLight_t *light)
{
    int    i, j;
    vec3_t v, world;
    vec4_t eye, clip, normalized, window;
    float  depthMin, depthMax;

    if (r_shadows->integer == SHADOWING_STENCIL && glDepthBoundsEXT)
    {
        tr.pc.c_depthBoundsTestsRejected++;

        depthMin = 1.0;
        depthMax = 0.0;

        for (j = 0; j < 8; j++)
        {
            v[0] = light->localBounds[j & 1][0];
            v[1] = light->localBounds[(j >> 1) & 1][1];
            v[2] = light->localBounds[(j >> 2) & 1][2];

            // transform local bounds vertices into world space
            MatrixTransformPoint(light->transformMatrix, v, world);
            R_TransformWorldToClip(world, tr.viewParms.world.viewMatrix, tr.viewParms.projectionMatrix, eye, clip);

            //R_TransformModelToClip(v, tr.or.modelViewMatrix, tr.viewParms.projectionMatrix, eye, clip);

            // check to see if the point is completely off screen
            for (i = 0; i < 3; i++)
            {
                if (clip[i] >= clip[3] || clip[i] <= -clip[3])
                {
                    light->noDepthBoundsTest = qtrue;
                    return;
                }
            }

            R_TransformClipToWindow(clip, &tr.viewParms, normalized, window);

            if (window[0] < 0 || window[0] >= tr.viewParms.viewportWidth
                || window[1] < 0 || window[1] >= tr.viewParms.viewportHeight)
            {
                // shouldn't happen, since we check the clip[] above, except for FP rounding
                light->noDepthBoundsTest = qtrue;
                return;
            }

            depthMin = min(normalized[2], depthMin);
            depthMax = max(normalized[2], depthMax);
        }

        if (depthMin > depthMax)
        {
            // light behind near plane or clipped
            light->noDepthBoundsTest = qtrue;
        }
        else
        {
            light->noDepthBoundsTest = qfalse;
            light->depthNear         = depthMin;
            light->depthFar          = depthMax;

            tr.pc.c_depthBoundsTestsRejected--;
            tr.pc.c_depthBoundsTests++;
        }
    }
}
*/


// Some matrices that are used by R_CalcLightCubeSideBits()
// There are two versions of a matrix (_r = "3x3 transposed" version)
mat4_t rotMatrix_0_0_0 = { 1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f };
mat4_t rotMatrix_0_0_0_r = { 1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f };

mat4_t rotMatrix_0_180_0 = { -1.f, 0.f, 0.f, 0.f,
							0.f, -1.f, 0.f, 0.f,
							0.f, 0.f, 1.f, 0.f,
							0.0f, 0.0f, 0.0f, 1.0f };
mat4_t rotMatrix_0_180_0_r = { -1.f, 0.f, 0.f, 0.f,
								0.f, -1.f, 0.f, 0.f,
								0.f, 0.f, 1.f, 0.f,
								0.0f, 0.0f, 0.0f, 1.0f };

mat4_t rotMatrix_0_90_0 = { 0.f, 1.f, 0.f, 0.f,
							-1.f, 0.f, 0.f, 0.f,
							0.f, 0.f, 1.f, 0.f,
							0.0f, 0.0f, 0.0f, 1.0f };
mat4_t rotMatrix_0_90_0_r = { 0.f, -1.f, 0.f, 0.f,
							1.f, 0.f, 0.f, 0.f,
							0.f, 0.f, 1.f, 0.f,
							0.0f, 0.0f, 0.0f, 1.0f };

mat4_t rotMatrix_0_270_0 = { 0.f, -1.f, 0.f, 0.f,
							1.f, 0.f, 0.f, 0.f,
							0.f, 0.f, 1.f, 0.f,
							0.0f, 0.0f, 0.0f, 1.0f };
mat4_t rotMatrix_0_270_0_r = { 0.f, 1.f, 0.f, 0.f,
								-1.f, 0.f, 0.f, 0.f,
								0.f, 0.f, 1.f, 0.f,
								0.0f, 0.0f, 0.0f, 1.0f };

mat4_t rotMatrix_m90_0_0 = { 0.f, 0.f, 1.f, 0.f,
							0.f, 1.f, 0.f, 0.f,
							-1.f, 0.f, 0.f, 0.f,
							0.0f, 0.0f, 0.0f, 1.0f };
mat4_t rotMatrix_m90_0_0_r = { 0.f, 0.f, -1.f, 0.f,
								0.f, 1.f, 0.f, 0.f,
								1.f, 0.f, 0.f, 0.f,
								0.0f, 0.0f, 0.0f, 1.0f };

mat4_t rotMatrix_90_0_0 = { 0.f, 0.f, -1.f, 0.f,
							0.f, 1.f, 0.f, 0.f,
							1.f, 0.f, 0.f, 0.f,
							0.0f, 0.0f, 0.0f, 1.0f };
mat4_t rotMatrix_90_0_0_r = { 0.f, 0.f, 1.f, 0.f,
							0.f, 1.f, 0.f, 0.f,
							-1.f, 0.f, 0.f, 0.f,
							0.0f, 0.0f, 0.0f, 1.0f };


// *INDENT-OFF*
/**
 * @brief R_CalcLightCubeSideBits
 * @param[in] light
 * @param[in] worldBounds
 * @return
 *
 * INFO: This function could use a cleanup. But then all the old code and comments get lost.
 *       Because the old code makes it easier to understand what is going on, i leave in all that for "understandability"..
 */
byte R_CalcLightCubeSideBits(trRefLight_t *light, vec3_t worldBounds[2])
{
	int       i;
	int       cubeSide;
	byte      cubeSideBits;
	float     xMin, xMax, yMin, yMax;
	float     width, height, depth;
	float     zNear, zFar;
	float     fovX, fovY;
	float     *proj;
	vec3_t    angles;
	mat4_t    tmpMatrix, projectionMatrix, rotationMatrix, transformMatrix, viewProjectionMatrix;
//	mat4_t    *rotMatrix, *rotMatrix_r, viewMatrix;
	frustum_t frustum;
	cplane_t  *clipPlane;
	int       r;
	qboolean  anyClip;
	qboolean  culled;

#if 0
	static int count = 0;
	cubeSideBits = 0;
	for (cubeSide = 0; cubeSide < 6; cubeSide++)
	{
		if (count % 2 == 0)
		{
			cubeSideBits |= (1 << cubeSide);
		}
	}
	return cubeSideBits;
#endif

	if (light->l.rlType != RL_OMNI || r_shadows->integer < SHADOWING_EVSM32 || r_noShadowPyramids->integer)
	{
		return CUBESIDE_CLIPALL;
	}
#if 1
	cubeSideBits = 0;
	for (cubeSide = 0; cubeSide < 6; cubeSide++)
	{
		switch (cubeSide)
		{
		case 0:
		{
			// view parameters
			VectorSet(angles, 0.0f, 0.0f, 0.0f);
			break;
		}
		case 1:
		{
			VectorSet(angles, 0.0f, 180.0f, 0.0f);
			break;
		}
		case 2:
		{
			VectorSet(angles, 0.0f, 90.0f, 0.0f);
			break;
		}
		case 3:
		{
			VectorSet(angles, 0.0f, 270.0f, 0.0f);
			break;
		}
		case 4:
		{
			VectorSet(angles, -90.0f, 0.0f, 0.0f);
			break;
		}
		case 5:
		{
			VectorSet(angles, 90.0f, 0.0f, 0.0f);
			break;
		}
		}

		// Quake -> OpenGL view matrix from light perspective
		mat4_from_angles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
		MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, light->origin);

		MatrixAffineInverse(transformMatrix, tmpMatrix);


	/*out[0] = rotMatrix_r[0];      out[4] = rotMatrix_r[4];       out[8] = rotMatrix_r[8];          out[12] = -dot(vec3 in[0], light->origin);
	out[1] = rotMatrix_r[1];      out[5] = rotMatrix_r[5];       out[9] = rotMatrix_r[9];          out[13] = -dot(vec3 in[4], light->origin);
	out[2] = rotMatrix_r[2];      out[6] = rotMatrix_r[6];       out[10] = rotMatrix_r[10];        out[14] = -dot(vec3 in[8], light->origin);
	out[3] = rotMatrix_r[3];      out[7] = rotMatrix_r[7];       out[11] = rotMatrix_r[11];        out[15] = 1.0f;*/
/*	Vector4Copy((float *)&rotMatrix_r[0], &tmpMatrix[0]);
	Vector4Copy((float *)&rotMatrix_r[4], &tmpMatrix[4]);
	Vector4Copy((float *)&rotMatrix_r[8], &tmpMatrix[8]);
	Dot(rotMatrix[0], light->origin, tmpMatrix[12]);
	Dot(rotMatrix[4], light->origin, tmpMatrix[13]);
	Dot(rotMatrix[8], light->origin, tmpMatrix[14]);
	tmpMatrix[15] = 1.0f;*/

/*$	//@ Matrix4Multiply(quakeToOpenGLMatrix, tmpMatrix, viewMatrix);
	//@	new code that does this: Matrix4Multiply(quakeToOpenGLMatrix, tmpMatrix, viewMatrix);
	// but atm it is commented out, because we do all viewMatrix-handling via registers (no need to store in seperate matrx var).
	Vector4Set(&viewMatrix[0], -tmpMatrix[1], tmpMatrix[2], -tmpMatrix[0], tmpMatrix[3]);
	Vector4Set(&viewMatrix[4], -tmpMatrix[5], tmpMatrix[6], -tmpMatrix[4], tmpMatrix[7]);
	Vector4Set(&viewMatrix[8], -tmpMatrix[9], tmpMatrix[10], -tmpMatrix[8], tmpMatrix[11]);
	Vector4Set(&viewMatrix[12], -tmpMatrix[13], tmpMatrix[14], -tmpMatrix[12], tmpMatrix[15]);*/
#else
	cubeSideBits = 0;
	for (cubeSide = 0; cubeSide < 6; cubeSide++)
	{
		switch (cubeSide)
		{
		case 0:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_0_0_0, light->origin);
			rotMatrix = &rotMatrix_0_0_0;
			rotMatrix_r = &rotMatrix_0_0_0_r;
			break;
		}
		case 1:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_0_180_0, light->origin);
			rotMatrix = &rotMatrix_0_180_0;
			rotMatrix_r = &rotMatrix_0_180_0_r;
			break;
		}
		case 2:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_0_90_0, light->origin);
			rotMatrix = &rotMatrix_0_90_0;
			rotMatrix_r = &rotMatrix_0_90_0_r;
			break;
		}
		case 3:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_0_270_0, light->origin);
			rotMatrix = &rotMatrix_0_270_0;
			rotMatrix_r = &rotMatrix_0_270_0_r;
			break;
		}
		case 4:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_m90_0_0, light->origin);
			rotMatrix = &rotMatrix_m90_0_0;
			rotMatrix_r = &rotMatrix_m90_0_0_r;
			break;
		}
		case 5:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_90_0_0, light->origin);
			rotMatrix = &rotMatrix_90_0_0;
			rotMatrix_r = &rotMatrix_90_0_0_r;
			break;
		}
		}
		Vector4Copy((float *)&rotMatrix_r[0], &tmpMatrix[0]);
		Vector4Copy((float *)&rotMatrix_r[4], &tmpMatrix[4]);
		Vector4Copy((float *)&rotMatrix_r[8], &tmpMatrix[8]);
		Dot(rotMatrix[0], light->origin, tmpMatrix[12]);  tmpMatrix[12] = -tmpMatrix[12];
		Dot(rotMatrix[4], light->origin, tmpMatrix[13]);  tmpMatrix[13] = -tmpMatrix[13];
		Dot(rotMatrix[8], light->origin, tmpMatrix[14]);  tmpMatrix[14] = -tmpMatrix[14];
		tmpMatrix[15] = 1.0f;
#endif
		// OpenGL projection matrix

// tan(90 degrees) is always the same constant, zNear is always 1  =>  keep it simple..(for the computer)

		fovX = 90.0f;
		fovY = 90.0f; //R_CalcFov(fovX, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

		zNear = 1.0f;
		zFar  = light->sphereRadius;

		xMax = zNear * tan(DEG2RAD(fovX));
		xMin = -xMax;

		yMax = zNear * tan(DEG2RAD(fovY));
		yMin = -yMax;

		width  = xMax - xMin;
		height = yMax - yMin;
		depth  = zFar - zNear;

		proj    = projectionMatrix;
//		proj[0] = (2 * zNear) / width;  proj[4] = 0;                    proj[8] = (xMax + xMin) / width;    proj[12] = 0;
//		proj[1] = 0;                    proj[5] = (2 * zNear) / height; proj[9] = (yMax + yMin) / height;   proj[13] = 0;
//		proj[2] = 0;                    proj[6] = 0;                    proj[10] = -(zFar + zNear) / depth; proj[14] = -(2 * zFar * zNear) / depth;
//		proj[3] = 0;                    proj[7] = 0;                    proj[11] = -1;                      proj[15] = 0;
float rW = 1.0f / width, rH = 1.0f / height, rD = 1.0f / depth, zN2 = 2.0f * zNear;
proj[0] = zN2 * rW;   proj[4] = 0.0f;       proj[8] = (xMax + xMin) * rW;      proj[12] = 0.0f;
proj[1] = 0.0f;       proj[5] = zN2 * rH;   proj[9] = (yMax + yMin) * rH;      proj[13] = 0.0f;
proj[2] = 0.0f;       proj[6] = 0.0f;       proj[10] = -(zFar + zNear) * rD;   proj[14] = -(zFar * zN2) * rD;
proj[3] = 0.0f;       proj[7] = 0.0f;       proj[11] = -1.0f;                  proj[15] = 0.0f;

		// calculate frustum planes using the modelview projection matrix
//		Matrix4Multiply(projectionMatrix, viewMatrix, viewProjectionMatrix);
Matrix4Multiply(projectionMatrix, tmpMatrix, viewProjectionMatrix);

		R_SetupFrustum2(frustum, viewProjectionMatrix);

		// use the frustum planes to cut off shadowmaps beyond the light volume
		anyClip = qfalse;
		culled  = qfalse;
		for (i = 0; i < 5; i++)
		{
			clipPlane = &frustum[i];

			r = BoxOnPlaneSide(worldBounds[0], worldBounds[1], clipPlane);
			if (r == 2)
			{
				culled = qtrue;
				break;
			}
			if (r == 3)
			{
				anyClip = qtrue;
			}
		}

		if (!culled)
		{
			if (!anyClip)
			{
				// completely inside frustum
				tr.pc.c_pyramid_cull_ent_in++;
			}
			else
			{
				// partially clipped
				tr.pc.c_pyramid_cull_ent_clip++;
			}

			cubeSideBits |= (1 << cubeSide);
		}
		else
		{
			// completely outside frustum
			tr.pc.c_pyramid_cull_ent_out++;
		}
	}

	tr.pc.c_pyramidTests++;

	return cubeSideBits;
}
// *INDENT-ON*


#if 0
// sse version
byte R_CalcLightCubeSideBits(trRefLight_t *light, vec3_t worldBounds[2])
{
	int       i;
	int       cubeSide;
	byte      cubeSideBits;
	//float     xMin, xMax, yMin, yMax;
	float     /*width, height,*/ depth;
	float     /*zNear,*/ zFar;
	//float     fovX, fovY;
	//float     *proj;
	//vec3_t    angles;
#ifndef ETL_SSE
	mat4_t    tmpMatrix, viewMatrix, projectionMatrix, /*rotationMatrix, transformMatrix,*/ viewProjectionMatrix;
#else
	mat4_t    tmpMatrix, viewMatrix, projectionMatrix, viewProjectionMatrix;
	__m128 xmm0, xmm1, xmm2, xmm4, xmm5, xmm6, xmm7, zeroes;
#endif
	mat4_t    *rotMatrix, *rotMatrix_r;
	frustum_t frustum;
	cplane_t  *clipPlane;
	int       r;
	qboolean  anyClip;
	qboolean  culled;

#if 0
	static int count = 0;
	cubeSideBits = 0;
	for (cubeSide = 0; cubeSide < 6; cubeSide++)
	{
		if (count % 2 == 0)
		{
			cubeSideBits |= (1 << cubeSide);
		}
	}
	return cubeSideBits;
#endif

	if (light->l.rlType != RL_OMNI || r_shadows->integer < SHADOWING_EVSM32 || r_noShadowPyramids->integer)
	{
		return CUBESIDE_CLIPALL;
	}

#if 0
	cubeSideBits = 0;
	for (cubeSide = 0; cubeSide < 6; cubeSide++)
	{
		switch (cubeSide)
		{
		case 0:
		{
			// view parameters
			VectorSet(angles, 0.0f, 0.0f, 0.0f);
			break;
		}
		case 1:
		{
			VectorSet(angles, 0.0f, 180.0f, 0.0f);
			break;
		}
		case 2:
		{
			VectorSet(angles, 0.0f, 90.0f, 0.0f);
			break;
		}
		case 3:
		{
			VectorSet(angles, 0.0f, 270.0f, 0.0f);
			break;
		}
		case 4:
		{
			VectorSet(angles, -90.0f, 0.0f, 0.0f);
			break;
		}
		case 5:
		{
			VectorSet(angles, 90.0f, 0.0f, 0.0f);
			break;
		}
		}

		// Quake -> OpenGL view matrix from light perspective
		mat4_from_angles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
		// ^^that is calculating a constant value, from some vectorSet(constant)
		MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, light->origin);
#else
	cubeSideBits = 0;
	for (cubeSide = 0; cubeSide < 6; cubeSide++)
	{
		switch (cubeSide)
		{
		case 0:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_0_0_0, light->origin);
			rotMatrix = &rotMatrix_0_0_0;
			rotMatrix_r = &rotMatrix_0_0_0_r;
			break;
		}
		case 1:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_0_180_0, light->origin);
			rotMatrix = &rotMatrix_0_180_0;
			rotMatrix_r = &rotMatrix_0_180_0_r;
			break;
		}
		case 2:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_0_90_0, light->origin);
			rotMatrix = &rotMatrix_0_90_0;
			rotMatrix_r = &rotMatrix_0_90_0_r;
			break;
		}
		case 3:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_0_270_0, light->origin);
			rotMatrix = &rotMatrix_0_270_0;
			rotMatrix_r = &rotMatrix_0_270_0_r;
			break;
		}
		case 4:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_m90_0_0, light->origin);
			rotMatrix = &rotMatrix_m90_0_0;
			rotMatrix_r = &rotMatrix_m90_0_0_r;
			break;
		}
		case 5:
		{
			//!!			MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_90_0_0, light->origin);
			rotMatrix = &rotMatrix_90_0_0;
			rotMatrix_r = &rotMatrix_90_0_0_r;
			break;
		}
		}
#endif

		//!!		MatrixAffineInverse(transformMatrix, tmpMatrix);

		/*	out[0] = in[0];       out[4] = in[1];       out[8] = in[2];          out[12] = -(in[12] * out[0] + in[13] * out[4] + in[14] * out[8]);
			out[1] = in[4];       out[5] = in[5];       out[9] = in[6];          out[13] = -(in[12] * out[1] + in[13] * out[5] + in[14] * out[9]);
			out[2] = in[8];       out[6] = in[9];       out[10] = in[10];        out[14] = -(in[12] * out[2] + in[13] * out[6] + in[14] * out[10]);
			out[3] = 0.0f;        out[7] = 0.0f;        out[11] = 0.0f;          out[15] = 1.0f;

			out[0] = in[0];       out[4] = in[1];       out[8] = in[2];          out[12] = -(in[12] * in[0] + in[13] * in[1] + in[14] * in[2]);
			out[1] = in[4];       out[5] = in[5];       out[9] = in[6];          out[13] = -(in[12] * in[4] + in[13] * in[5] + in[14] * in[6]);
			out[2] = in[8];       out[6] = in[9];       out[10] = in[10];        out[14] = -(in[12] * in[8] + in[13] * in[9] + in[14] * in[10]);
			out[3] = 0.0f;        out[7] = 0.0f;        out[11] = 0.0f;          out[15] = 1.0f;

			out[0] = in[0];       out[4] = in[1];       out[8] = in[2];          out[12] = -dot(vec3 in[0], vec3 in[12]);  // -dot(in0, light->origin)
			out[1] = in[4];       out[5] = in[5];       out[9] = in[6];          out[13] = -dot(vec3 in[4], vec3 in[12]);  // -dot(in4, light->origin)
			out[2] = in[8];       out[6] = in[9];       out[10] = in[10];        out[14] = -dot(vec3 in[8], vec3 in[12]);  // -dot(in8, light->origin)
			out[3] = 0.0f;        out[7] = 0.0f;        out[11] = 0.0f;          out[15] = 1.0f;

			// a version that uses both rotMatrix and the transposed rotMatrix_r
			out[0] = in_r[0];      out[4] = in_r[4];       out[8] = in_r[8];          out[12] = -dot(vec3 in[0], vec3 in[12]);  // -dot(in0, light->origin)
			out[1] = in_r[1];      out[5] = in_r[5];       out[9] = in_r[9];          out[13] = -dot(vec3 in[4], vec3 in[12]);  // -dot(in4, light->origin)
			out[2] = in_r[2];      out[6] = in_r[6];       out[10] = in_r[10];        out[14] = -dot(vec3 in[8], vec3 in[12]);  // -dot(in8, light->origin)
			out[3] = in_r[3];      out[7] = in_r[7];       out[11] = in_r[11];        out[15] = 1.0f;

			^^ that last version is doing the same as the following SSE3 version:
		*/
#ifndef ETL_SSE
		/*out[0] = rotMatrix_r[0];      out[4] = rotMatrix_r[4];       out[8] = rotMatrix_r[8];          out[12] = -dot(vec3 in[0], light->origin);
		out[1] = rotMatrix_r[1];      out[5] = rotMatrix_r[5];       out[9] = rotMatrix_r[9];          out[13] = -dot(vec3 in[4], light->origin);
		out[2] = rotMatrix_r[2];      out[6] = rotMatrix_r[6];       out[10] = rotMatrix_r[10];        out[14] = -dot(vec3 in[8], light->origin);
		out[3] = rotMatrix_r[3];      out[7] = rotMatrix_r[7];       out[11] = rotMatrix_r[11];        out[15] = 1.0f;*/
		Vector4Copy((float *)&rotMatrix_r[0], &tmpMatrix[0]);
		Vector4Copy((float *)&rotMatrix_r[4], &tmpMatrix[4]);
		Vector4Copy((float *)&rotMatrix_r[8], &tmpMatrix[8]);
		Dot(rotMatrix[0], light->origin, tmpMatrix[12]);
		Dot(rotMatrix[4], light->origin, tmpMatrix[13]);
		Dot(rotMatrix[8], light->origin, tmpMatrix[14]);
		tmpMatrix[15] = 1.0f;
		Matrix4Multiply(quakeToOpenGLMatrix, tmpMatrix, viewMatrix);	//@
#else
		// this is setting tmpMatrix.
		//	MatrixSetupTransformFromRotation(transformMatrix, rotMatrix_x_y_z, light->origin)
		//	MatrixAffineInverse(transformMatrix, tmpMatrix);
		zeroes = _mm_setzero_ps();
		xmm0 = _mm_loadh_pi(_mm_load_ss(&light->origin[0]), (const __m64 *)(&light->origin[1]));	// xmm0 = z y 0 x

		xmm2 = _mm_loadh_pi(_mm_load_ss((const float *)&rotMatrix[0]), (const __m64 *)(&rotMatrix[1]));		// xmm2 = z y 0 x
		xmm1 = _mm_mul_ps(xmm0, xmm2);
		xmm7 = _mm_movehdup_ps(xmm1);		// faster way to do: 2 * hadd
		xmm6 = _mm_add_ps(xmm1, xmm7);		//
		xmm7 = _mm_movehl_ps(xmm7, xmm6);	//
		xmm1 = _mm_add_ss(xmm6, xmm7);		// xmm1 = dot(in0, in12)
		xmm1 = _mm_sub_ps(zeroes, xmm1);
		_mm_store_ss(&tmpMatrix[12], xmm1);

		xmm2 = _mm_loadh_pi(_mm_load_ss((const float *)&rotMatrix[4]), (const __m64 *)(&rotMatrix[5]));		// xmm2 = z y 0 x
		xmm4 = _mm_mul_ps(xmm0, xmm2);
		xmm7 = _mm_movehdup_ps(xmm4);		// faster way to do: 2 * hadd
		xmm6 = _mm_add_ps(xmm4, xmm7);		//
		xmm7 = _mm_movehl_ps(xmm7, xmm6);	//
		xmm4 = _mm_add_ss(xmm6, xmm7);		// xmm4 = dot(in4, in12)
		xmm4 = _mm_sub_ps(zeroes, xmm4);
		_mm_store_ss(&tmpMatrix[13], xmm4);

		xmm2 = _mm_loadh_pi(_mm_load_ss((const float *)&rotMatrix[8]), (const __m64 *)(&rotMatrix[9]));		// xmm2 = z y 0 x
		xmm5 = _mm_mul_ps(xmm0, xmm2);
		xmm7 = _mm_movehdup_ps(xmm5);		// faster way to do: 2 * hadd
		xmm6 = _mm_add_ps(xmm5, xmm7);		//
		xmm7 = _mm_movehl_ps(xmm7, xmm6);	//
		xmm5 = _mm_add_ss(xmm6, xmm7);		// xmm5 = dot(in8, in12)
		xmm5 = _mm_sub_ps(zeroes, xmm5);
		_mm_store_ss(&tmpMatrix[14], xmm5);

		_mm_store_ss(&tmpMatrix[15], _mm_set_ss(1.f));
		_mm_storeu_ps(&tmpMatrix[0], _mm_loadu_ps((const float *)&rotMatrix_r[0]));
		_mm_storeu_ps(&tmpMatrix[4], _mm_loadu_ps((const float *)&rotMatrix_r[4]));
		_mm_storeu_ps(&tmpMatrix[8], _mm_loadu_ps((const float *)&rotMatrix_r[8]));

		// --TODO: do that quakeToOpenGLMatrix matmult also.. still to implement
		// update: it's implemented now, but i leave in all comments.
		// That'll make it easier to trace back some steps, and understand what's going on.
		// If you see the final 4 vector4set statements that handle the trandformation (quakeToOpenGLMatrix * tmpMatrix) only,
		// it would be confusing, and not easy to read code.
		/*
							0.0f, 0.0f, -1.0f, 0.0f,
			viewMatrix =	-1.0f, 0.0f, 0.0f, 0.0f,   *  tmpMatrix
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f


			viewMatrix[0] = b[0] * 0 + b[1] * -1 + b[2] * 0 + b[3] * 0;
			viewMatrix[1] = b[0] * 0 + b[1] * 0 + b[2] * 1 + b[3] * 0;
			viewMatrix[2] = b[0] * -1 + b[1] * 0 + b[2] * 0 + b[3] * 0;
			viewMatrix[3] = b[0] * 0 + b[1] * 0 + b[2] * 0 + b[3] * 1;


			viewMatrix[0] = -b[1];
			viewMatrix[1] = b[2];
			viewMatrix[2] = -b[0];
			viewMatrix[3] = b[3];

			viewMatrix[4] = -b[5];
			viewMatrix[5] = b[6];
			viewMatrix[6] = -b[4];
			viewMatrix[7] = b[7];

			viewMatrix[8] = -b[9];
			viewMatrix[9] = b[10];
			viewMatrix[10] = -b[8];
			viewMatrix[11] = b[11];

			viewMatrix[12] = -b[13];
			viewMatrix[13] = b[14];
			viewMatrix[14] = -b[12];
			viewMatrix[15] = b[15];

		*/
		/*$$	//@	new code that does this: Matrix4Multiply(quakeToOpenGLMatrix, tmpMatrix, viewMatrix);
			// but atm it is commented out, because we do all viewMatrix-handling via registers (no need to store in seperate matrx var).
			Vector4Set(&viewMatrix[0], -tmpMatrix[1], tmpMatrix[2], -tmpMatrix[0], tmpMatrix[3]);
			Vector4Set(&viewMatrix[4], -tmpMatrix[5], tmpMatrix[6], -tmpMatrix[4], tmpMatrix[7]);
			Vector4Set(&viewMatrix[8], -tmpMatrix[9], tmpMatrix[10], -tmpMatrix[8], tmpMatrix[11]);
			Vector4Set(&viewMatrix[12], -tmpMatrix[13], tmpMatrix[14], -tmpMatrix[12], tmpMatrix[15]);*/
#endif

			// convert from our coordinate system (looking down X)
			// to OpenGL's coordinate system (looking down -Z)
		//@	Matrix4Multiply(quakeToOpenGLMatrix, tmpMatrix, viewMatrix);

		// NOTE: You can combine the 3 calculations: MatrixSetupTransformFromRotation, MatrixAffineInverse & Matrix4Multiply
		// It's all "just" to calculate the viewMatrix. The light.origin is the only changing value,
		// all other matrices are constants.
		// TODO: combine into a simpler block, providing the final viewMatrix asap.
		// update: done.




				// OpenGL projection matrix
		/*
		// tan(90 degrees) is always the same constant, zNear is always 1  =>  keep it simple..(for the computer)

				fovX = 90.0f;
				fovY = 90.0f; //R_CalcFov(fovX, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

				zNear = 1.0f;
				zFar  = light->sphereRadius;

				xMax = zNear * tan(DEG2RAD(fovX));
				xMin = -xMax;

				yMax = zNear * tan(DEG2RAD(fovY));
				yMin = -yMax;

				width  = xMax - xMin;
				height = yMax - yMin;
				depth  = zFar - zNear;

				proj    = projectionMatrix;
		//		proj[0] = (2 * zNear) / width;  proj[4] = 0;                    proj[8] = (xMax + xMin) / width;    proj[12] = 0;
		//		proj[1] = 0;                    proj[5] = (2 * zNear) / height; proj[9] = (yMax + yMin) / height;   proj[13] = 0;
		//		proj[2] = 0;                    proj[6] = 0;                    proj[10] = -(zFar + zNear) / depth; proj[14] = -(2 * zFar * zNear) / depth;
		//		proj[3] = 0;                    proj[7] = 0;                    proj[11] = -1;                      proj[15] = 0;
		float rW = 1.0f / width, rH = 1.0f / height, rD = 1.0f / depth, zN2 = 2.0f * zNear;
		proj[0] = zN2 * rW;   proj[4] = 0.0f;       proj[8] = (xMax + xMin) * rW;      proj[12] = 0.0f;
		proj[1] = 0.0f;       proj[5] = zN2 * rH;   proj[9] = (yMax + yMin) * rH;      proj[13] = 0.0f;
		proj[2] = 0.0f;       proj[6] = 0.0f;       proj[10] = -(zFar + zNear) * rD;   proj[14] = -(zFar * zN2) * rD;
		proj[3] = 0.0f;       proj[7] = 0.0f;       proj[11] = -1.0f;                  proj[15] = 0.0f;
		*/
		// ..after optimizing, not much of the old code exists.
		zFar = light->sphereRadius;
		depth = zFar - 1.0f;
		float rWH = 36.466487130706173033023747483407f, rD = rcp(depth);
#if 1 //ndef ETL_SSE
		Vector4Set(&projectionMatrix[0], rWH, 0.f, 0.f, 0.f);
		Vector4Set(&projectionMatrix[4], 0.f, rWH, 0.f, 0.f);
		Vector4Set(&projectionMatrix[8], 0.f, 0.f, -(zFar + 1.0f) * rD, -1.f);
		Vector4Set(&projectionMatrix[12], 0.f, 0.f, -(2.0f * zFar) * rD, 0.f);

		// calculate frustum planes using the modelview projection matrix
		Matrix4Copy(tmpMatrix, viewMatrix);
		Matrix4Multiply(projectionMatrix, viewMatrix, viewProjectionMatrix);
#else // todo: check some bug in the next block..
		//$$ load the projectionMatrix
		xmm4 = _mm_set_ps(0.f, 0.f, 0.f, rWH);
		xmm5 = _mm_set_ps(0.f, 0.f, rWH, 0.f);
		xmm6 = _mm_set_ps(-1.f, -(zFar + 1.0f) * rD, 0.f, 0.f);
		xmm7 = _mm_set_ps(0.f, -(2.0f * zFar) * rD, 0.f, 0.f);

		// Matrix4Multiply(projectionMatrix, viewMatrix, viewProjectionMatrix)
		//$$	xmm0 = _mm_loadu_ps(&viewMatrix[0]);			// xmm0 = b0 b1 b2 b3
		xmm0 = _mm_set_ps(tmpMatrix[3], -tmpMatrix[0], tmpMatrix[2], -tmpMatrix[1]); // this handles the quakeToOpenGLMatrix tranform
		xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b0 b0 b0 b0
		xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b1 b1 b1 b1
		xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b2 b2 b2 b2
		xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b3 b3 b3 b3
		xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b3*a12 b3*a13 b3*a14 b3*a15
		xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b2*a8  b2*a9 b2*a10 b2*a11
		xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =  b1*a4  b1*a5  b1*a6  b1*a7
		xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =  b0*a0  b0*a1  b0*a2  b0*a3
		xmm3 = _mm_add_ps(xmm3, xmm2);
		xmm0 = _mm_add_ps(xmm0, xmm1);
		xmm0 = _mm_add_ps(xmm0, xmm3);
		_mm_storeu_ps(&viewProjectionMatrix[0], xmm0);

		//$$	xmm0 = _mm_loadu_ps(&viewMatrix[4]);			// xmm0 = b4 b5 b6 b7
		xmm0 = _mm_set_ps(tmpMatrix[7], -tmpMatrix[4], tmpMatrix[6], -tmpMatrix[5]);
		xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b4 b4 b4 b4
		xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b5 b5 b5 b5
		xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b6 b6 b6 b6
		xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b7 b7 b7 b7
		xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b7*a12 b7*a13 b7*a14 b7*a15
		xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b6*a8  b6*a9 b6*a10 b6*a11
		xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =  b5*a4  b5*a5  b5*a6  b5*a7
		xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =  b4*a0  b4*a1  b4*a2  b4*a3
		xmm3 = _mm_add_ps(xmm3, xmm2);
		xmm0 = _mm_add_ps(xmm0, xmm1);
		xmm0 = _mm_add_ps(xmm0, xmm3);
		_mm_storeu_ps(&viewProjectionMatrix[4], xmm0);

		//$$	xmm0 = _mm_loadu_ps(&viewMatrix[8]);			// xmm0 = b8 b9 b10 b11
		xmm0 = _mm_set_ps(tmpMatrix[11], -tmpMatrix[8], tmpMatrix[10], -tmpMatrix[9]);
		xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b8 b8 b8 b8
		xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b9 b9 b9 b9
		xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b10 b10 b10 b10
		xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b11 b11 b11 b11
		xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b11*a12 b11*a13 b11*a14 b11*a15
		xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b10*a8  b10*a9 b10*a10 b10*a11
		xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =   b9*a4   b9*a5   b9*a6   b9*a7
		xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =   b8*a0   b8*a1   b8*a2   b8*a3
		xmm3 = _mm_add_ps(xmm3, xmm2);
		xmm0 = _mm_add_ps(xmm0, xmm1);
		xmm0 = _mm_add_ps(xmm0, xmm3);
		_mm_storeu_ps(&viewProjectionMatrix[8], xmm0);

		//xmm0 = _mm_loadu_ps(&viewMatrix[12]);			// xmm0 = b12 b13 b14 b15
		xmm0 = _mm_set_ps(tmpMatrix[15], -tmpMatrix[12], tmpMatrix[14], -tmpMatrix[13]);
		xmm3 = _mm_shuffle_ps(xmm0, xmm0, 0b11111111);	// xmm0 = b12 b12 b12 b12
		xmm2 = _mm_shuffle_ps(xmm0, xmm0, 0b10101010);	// xmm1 = b13 b13 b13 b13
		xmm1 = _mm_shuffle_ps(xmm0, xmm0, 0b01010101);	// xmm2 = b14 b14 b14 b14
		xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0b00000000);	// xmm3 = b15 b15 b15 b15
		xmm3 = _mm_mul_ps(xmm3, xmm7);					// xmm3 = b15*a12 b15*a13 b15*a14 b15*a15
		xmm2 = _mm_mul_ps(xmm2, xmm6);					// xmm2 =  b14*a8  b14*a9 b14*a10 b14*a11
		xmm1 = _mm_mul_ps(xmm1, xmm5);					// xmm1 =  b13*a4  b13*a5  b13*a6  b13*a7
		xmm0 = _mm_mul_ps(xmm0, xmm4);					// xmm0 =  b12*a0  b12*a1  b12*a2  b12*a3
		xmm3 = _mm_add_ps(xmm3, xmm2);
		xmm0 = _mm_add_ps(xmm0, xmm1);
		xmm0 = _mm_add_ps(xmm0, xmm3);
		_mm_storeu_ps(&viewProjectionMatrix[12], xmm0);
#endif

		R_SetupFrustum2(frustum, viewProjectionMatrix);

		// use the frustum planes to cut off shadowmaps beyond the light volume
		anyClip = qfalse;
		culled = qfalse;
		for (i = 0; i < 5; i++)
		{
			clipPlane = &frustum[i];

			r = BoxOnPlaneSide(worldBounds[0], worldBounds[1], clipPlane);
			if (r == 2)
			{
				culled = qtrue;
				break;
			}
			if (r == 3)
			{
				anyClip = qtrue;
			}
		}

		if (!culled)
		{
			if (!anyClip)
			{
				// completely inside frustum
				tr.pc.c_pyramid_cull_ent_in++;
			}
			else
			{
				// partially clipped
				tr.pc.c_pyramid_cull_ent_clip++;
			}

			cubeSideBits |= (1 << cubeSide);
		}
		else
		{
			// completely outside frustum
			tr.pc.c_pyramid_cull_ent_out++;
		}
	}

	tr.pc.c_pyramidTests++;

	return cubeSideBits;
	}
}
#endif










/**
 * @brief R_SetupLightLOD
 * @param[in,out] light
 */
void R_SetupLightLOD(trRefLight_t *light)
{
	float radius;
	float flod, lodscale;
	float projectedRadius;
	int   lod;
	int   numLods;

	if (light->l.noShadows)
	{
		light->shadowLOD = -1;
		return;
	}

	numLods = 5;

	// compute projected bounding sphere
	// and use that as a criteria for selecting LOD
	radius = light->sphereRadius;

	if ((projectedRadius = R_ProjectRadius(radius, light->l.origin)) != 0.f)
	{
		lodscale = r_shadowLodScale->value;

		if (lodscale > 20.f)
		{
			lodscale = 20.f;
		}

		flod = 1.0f - projectedRadius * lodscale;
	}
	else
	{
		// object intersects near view plane, e.g. view weapon
		flod = 0.f;
	}

	flod *= numLods;
	lod   = Q_ftol(flod);

	if (lod < 0)
	{
		lod = 0;
	}
	else if (lod >= numLods)
	{
		lod = numLods - 1; //!!
	}

	lod += r_shadowLodBias->integer;

	if (lod < 0)
	{
		lod = 0;
	}

	if (lod >= numLods)
	{
		// don't draw any shadow
//!!		lod = -1;

		lod = numLods - 1; //!! i still want shadow..
	}

	// never give ultra quality for point lights
	if (lod == 0 && light->l.rlType == RL_OMNI)
	{
		lod = 1;
	}

	light->shadowLOD = lod;
}

/**
 * @brief R_SetupLightShader
 * @param[in,out] light
 */
void R_SetupLightShader(trRefLight_t *light)
{
	if (!light->l.attenuationShader)
	{
		if (light->isStatic)
		{
			switch (light->l.rlType)
			{
			default:
			case RL_OMNI:
				light->shader = tr.defaultPointLightShader;
				break;
			case RL_PROJ:
				light->shader = tr.defaultProjectedLightShader;
				break;
			}
		}
		else
		{
			switch (light->l.rlType)
			{
			default:
			case RL_OMNI:
				light->shader = tr.defaultDynamicLightShader;
				break;
			case RL_PROJ:
				light->shader = tr.defaultProjectedLightShader;
				break;
			}
		}
	}
	else
	{
		light->shader = R_GetShaderByHandle(light->l.attenuationShader);
	}
}

/**
 * @brief R_ComputeFinalAttenuation
 * @param[in] pStage
 * @param[in] light
 */
void R_ComputeFinalAttenuation(shaderStage_t *pStage, trRefLight_t *light)
{
	mat4_t matrix;

	Ren_LogComment("--- R_ComputeFinalAttenuation ---\n");

	RB_CalcTexMatrix(&pStage->bundle[TB_COLORMAP], matrix);

	Matrix4Multiply(matrix, light->attenuationMatrix, light->attenuationMatrix2);
}

/**
 * @brief R_CullLightPoint
 * @param[in] light
 * @param[in] p
 * @return CULL_IN, CULL_CLIP, or CULL_OUT
 *
 * @note Unused
 */
int R_CullLightPoint(trRefLight_t *light, const vec3_t p)
{
	int      i;
	cplane_t *frust;
	float    dist;

	// check against frustum planes
	for (i = 0; i < 6; i++)
	{
		frust = &light->frustum[i];
		//dist = DotProduct(p, frust->normal) - frust->dist;
		Dot(p, frust->normal, dist);
		dist -= frust->dist;
		if (dist < 0.0f)
		{
			// completely outside frustum
			return CULL_OUT;
		}
	}

	// completely inside frustum
	return CULL_IN;
}

/**
 * @brief R_CullLightTriangle
 * @param[in] light
 * @param[in] verts
 * @return CULL_IN, CULL_CLIP, or CULL_OUT
 */
int R_CullLightTriangle(trRefLight_t *light, vec3_t verts[3])
{
	//int    i;
	vec3_t worldBounds[2];

	if (r_noCull->integer)
	{
		return CULL_CLIP;
	}

	// calc AABB of the triangle
	ClearBounds(worldBounds[0], worldBounds[1]);
	/*for (i = 0; i < 3; i++)
	{
		AddPointToBounds(verts[i], worldBounds[0], worldBounds[1]);
	}*/
	AddPointToBounds(verts[0], worldBounds[0], worldBounds[1]);
	AddPointToBounds(verts[1], worldBounds[0], worldBounds[1]);
	AddPointToBounds(verts[2], worldBounds[0], worldBounds[1]);

	return R_CullLightWorldBounds(light, worldBounds);
}

/**
 * @brief R_CullLightWorldBounds
 * @param[in] light
 * @param[in] worldBounds
 * @return CULL_IN, CULL_CLIP, or CULL_OUT
 */
int R_CullLightWorldBounds(trRefLight_t *light, vec3_t worldBounds[2])
{
	int      i;
	cplane_t *frust;
	qboolean anyClip;
	int      r;

	if (r_noCull->integer)
	{
		return CULL_CLIP;
	}

	// check against frustum planes
	anyClip = qfalse;
	for (i = 0; i < 6; i++)
	{
		frust = &light->frustum[i];

		r = BoxOnPlaneSide(worldBounds[0], worldBounds[1], frust);

		if (r == 2)
		{
			// completely outside frustum
			return CULL_OUT;
		}
		if (r == 3)
		{
			anyClip = qtrue;
		}
	}

	if (!anyClip)
	{
		// completely inside frustum
		return CULL_IN;
	}

	// partially clipped
	return CULL_CLIP;
}
