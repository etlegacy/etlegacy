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
 * @file renderer2/tr_world.c
 */

#include "tr_local.h"

/*
 * @brief Returns true if the grid is completely culled away.
 * Also sets the clipped hint bit in tess
 * @param[in] cv
 * @return
 *
 * @note Unused
static qboolean R_CullTriSurf(srfTriangles_t * cv)
{
    int             boxCull;

    boxCull = R_CullLocalBox(cv->bounds);

    if(boxCull == CULL_OUT)
    {
        return qtrue;
    }
    return qfalse;
}
*/

/*
 * @brief Returns true if the grid is completely culled away.
 * Also sets the clipped hint bit in tess
 * @param cv
 * @return
 *
 * @note Unused
static qboolean R_CullGrid(srfGridMesh_t * cv)
{
    int             boxCull;
    int             sphereCull;

    if(r_nocurves->integer)
    {
        return qtrue;
    }

    if(tr.currentEntity != &tr.worldEntity)
    {
        sphereCull = R_CullLocalPointAndRadius(cv->origin, cv->radius);
    }
    else
    {
        sphereCull = R_CullPointAndRadius(cv->origin, cv->radius);
    }
    boxCull = CULL_OUT;

    // check for trivial reject
    if(sphereCull == CULL_OUT)
    {
        tr.pc.c_sphere_cull_patch_out++;
        return qtrue;
    }
    // check bounding box if necessary
    else if(sphereCull == CULL_CLIP)
    {
        tr.pc.c_sphere_cull_patch_clip++;

        boxCull = R_CullLocalBox(cv->bounds);

        if(boxCull == CULL_OUT)
        {
            tr.pc.c_box_cull_patch_out++;
            return qtrue;
        }
        else if(boxCull == CULL_IN)
        {
            tr.pc.c_box_cull_patch_in++;
        }
        else
        {
            tr.pc.c_box_cull_patch_clip++;
        }
    }
    else
    {
        tr.pc.c_sphere_cull_patch_in++;
    }

    return qfalse;
}
*/

/**
 * @brief Tries to back face cull surfaces before they are lighted or
 * added to the sorting list.
 *
 * This will also allow mirrors on both sides of a model without recursion.
 * @param[in] surface
 * @param[in] shader
 * @param[out] frontFace
 * @return
 */
static qboolean R_CullSurface(surfaceType_t *surface, shader_t *shader, int *frontFace)
{
	srfGeneric_t *gen;
	int          cull;

	// force to non-front facing
	*frontFace = 0;

	// allow culling to be disabled
	if (r_noCull->integer)
	{
		return qfalse;
	}

	// made surface culling generic, inline with q3map2 surface classification
	switch (*surface)
	{
	case SF_FACE:
	case SF_TRIANGLES:
		break;
	case SF_GRID:
		if (r_noCurves->integer)
		{
			return qtrue;
		}
		break;
	case SF_FOLIAGE:
		if (r_drawFoliage->value == 0.f)
		{
			return qtrue;
		}
		break;
	default:
		return qtrue;
	}

	// get generic surface
	gen = (srfGeneric_t *) surface;

	// plane cull
	if (gen->plane.type != PLANE_NON_PLANAR && r_facePlaneCull->integer)
	{
		float d;

		d = DotProduct(tr.orientation.viewOrigin, gen->plane.normal) - gen->plane.dist;
		if (d > 0.0f)
		{
			*frontFace = 1;
		}

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if (shader->cullType == CT_FRONT_SIDED)
		{
			if (d < -8.0f)
			{
				tr.pc.c_plane_cull_out++;
				return qtrue;
			}
		}
		else if (shader->cullType == CT_BACK_SIDED)
		{
			if (d > 8.0f)
			{
				tr.pc.c_plane_cull_out++;
				return qtrue;
			}
		}

		tr.pc.c_plane_cull_in++;
	}

	// try sphere cull
	if (tr.currentEntity != &tr.worldEntity)
	{
		cull = R_CullLocalPointAndRadius(gen->origin, gen->radius);
	}
	else
	{
		cull = R_CullPointAndRadius(gen->origin, gen->radius);
	}

	if (cull == CULL_OUT)
	{
		tr.pc.c_sphere_cull_out++;
		return qtrue;
	}

	tr.pc.c_sphere_cull_in++;

	// must be visible
	return qfalse;
}

/**
 * @brief R_LightSurfaceGeneric
 * @param[in] face
 * @param[in] light
 * @param[out] cubeSideBits
 * @return
 */
static qboolean R_LightSurfaceGeneric(srfGeneric_t *face, trRefLight_t *light, byte *cubeSideBits)
{
	// do a quick AABB cull
	if (!BoundsIntersect(light->worldBounds[0], light->worldBounds[1], face->bounds[0], face->bounds[1]))
	{
		return qfalse;
	}

	// do a more expensive and precise light frustum cull
	if (!r_noLightFrustums->integer)
	{
		if (R_CullLightWorldBounds(light, face->bounds) == CULL_OUT)
		{
			return qfalse;
		}
	}

	if (r_cullShadowPyramidFaces->integer)
	{
		*cubeSideBits = R_CalcLightCubeSideBits(light, face->bounds);
	}
	return qtrue;
}

/**
 * @brief R_AddInteractionSurface
 * @param[in,out] surf
 * @param[in] light
 */
static void R_AddInteractionSurface(bspSurface_t *surf, trRefLight_t *light)
{
	qboolean          intersects;
	interactionType_t iaType       = IA_DEFAULT;
	byte              cubeSideBits = CUBESIDE_CLIPALL;

	// this surface is maybe not in this view but it may still cast a shadow
	// into this view
	if (surf->viewCount != tr.viewCountNoReset)
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

	if (surf->lightCount == tr.lightCount)
	{
		// already checked this surface
		return;
	}
	surf->lightCount = tr.lightCount;

	//  skip all surfaces that don't matter for lighting only pass
	if (surf->shader->isSky || (!surf->shader->interactLight && surf->shader->noShadows))
	{
		return;
	}

	switch (*surf->data)
	{
	case SF_FACE:
	case SF_GRID:
	case SF_TRIANGLES:
	case SF_FOLIAGE:
		intersects = R_LightSurfaceGeneric((srfGeneric_t *) surf->data, light, &cubeSideBits);
		break;
	default:
		intersects = qfalse;
		break;
	}

	if (intersects)
	{
		R_AddLightInteraction(light, surf->data, surf->shader, cubeSideBits, iaType);

		if (light->isStatic)
		{
			tr.pc.c_slightSurfaces++;
		}
		else
		{
			tr.pc.c_dlightSurfaces++;
		}
	}
	else
	{
		if (!light->isStatic)
		{
			tr.pc.c_dlightSurfacesCulled++;
		}
	}
}

/**
 * @brief R_AddWorldSurface
 * @param[in,out] surf
 * @param[in] decalBits
 */
static void R_AddWorldSurface(bspSurface_t *surf, int decalBits)
{
	int frontFace;

	if (surf->viewCount == tr.viewCountNoReset)
	{
		return;
	}
	surf->viewCount = tr.viewCountNoReset;

	// add decals
	if (decalBits)
	{
		int i;

		// project any decals
		for (i = 0; i < tr.refdef.numDecalProjectors; i++)
		{
			if (decalBits & (1 << i))
			{
				R_ProjectDecalOntoSurface(&tr.refdef.decalProjectors[i], surf, &tr.world->models[0]);
			}
		}
	}

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
	{
		shader_t *shader = surf->shader;

		if (r_mergeClusterSurfaces->integer &&
		    !r_dynamicBspOcclusionCulling->integer &&
		    ((r_mergeClusterFaces->integer && *surf->data == SF_FACE) ||
		     (r_mergeClusterCurves->integer && *surf->data == SF_GRID) ||
		     (r_mergeClusterTriangles->integer && *surf->data == SF_TRIANGLES)) &&
		    !shader->isSky && !shader->isPortal && !ShaderRequiresCPUDeforms(shader))
		{
			return;
		}
	}
#endif

	// try to cull before lighting or adding
	if (R_CullSurface(surf->data, surf->shader, &frontFace))
	{
		return;
	}

	R_AddDrawSurf(surf->data, surf->shader, surf->lightmapNum, surf->fogIndex);
}

/*
=============================================================
    BRUSH MODELS
=============================================================
*/

/**
 * @brief R_AddBrushModelSurface
 * @param[in,out] surf
 * @param[in] customShader
 * @param[in] fogIndex
 */
static void R_AddBrushModelSurface(bspSurface_t *surf, shader_t *customShader, int fogIndex)
{
	int frontFace;

	if (surf->viewCount == tr.viewCountNoReset)
	{
		return;                 // already in this view
	}
	surf->viewCount = tr.viewCountNoReset;

	// try to cull before lighting or adding
	if (R_CullSurface(surf->data, surf->shader, &frontFace))
	{
		return;
	}

	if (customShader)
	{
		R_AddDrawSurf(surf->data, customShader, surf->lightmapNum, fogIndex);
	}
	else
	{
		R_AddDrawSurf(surf->data, surf->shader, surf->lightmapNum, fogIndex);
	}
}

/**
 * @brief R_AddBSPModelSurfaces
 * @param[in,out] ent
 */
void R_AddBSPModelSurfaces(trRefEntity_t *ent)
{
	bspModel_t   *bspModel;
	model_t      *pModel;
	unsigned int i;
	vec3_t       boundsCenter;
	int          fogNum;
	//float	   boundsRadius;

	pModel   = R_GetModelByHandle(ent->e.hModel);
	bspModel = pModel->bsp;

	// set model state for decals and dynamic fog
	VectorCopy(ent->e.origin, bspModel->orientation.origin);
	VectorCopy(ent->e.axis[0], bspModel->orientation.axis[0]);
	VectorCopy(ent->e.axis[1], bspModel->orientation.axis[1]);
	VectorCopy(ent->e.axis[2], bspModel->orientation.axis[2]);

	// copy local bounds
	for (i = 0; i < 3; i++)
	{
		ent->localBounds[0][i] = bspModel->bounds[0][i];
		ent->localBounds[1][i] = bspModel->bounds[1][i];
	}

#if 0
	boundsRadius = RadiusFromBounds(bspModel->bounds[0], bspModel->bounds[1]);
	ent->cull    = R_CullPointAndRadius(ent->e.origin, boundsRadius);
#else
	ent->cull = R_CullLocalBox(bspModel->bounds);
#endif
	if (ent->cull == CULL_OUT)
	{
		return;
	}

	// setup world bounds for intersection tests
	ClearBounds(ent->worldBounds[0], ent->worldBounds[1]);

	//use a transformmatrix instead as it is faster
	mat4_transform_vec3(tr.orientation.transformMatrix, bspModel->bounds[0], ent->worldBounds[0]);
	mat4_transform_vec3(tr.orientation.transformMatrix, bspModel->bounds[1], ent->worldBounds[1]);
		

	VectorAdd(ent->worldBounds[0], ent->worldBounds[1], boundsCenter);
	//rotate the "forcedorigin to be right?
	VectorScale(boundsCenter,0.5, boundsCenter);
	
	// BSP inline models should always use vertex lighting
	R_SetupEntityLighting(&tr.refdef, ent, boundsCenter);

	fogNum = R_FogWorldBox(ent->worldBounds);

	if (r_vboModels->integer && bspModel->numVBOSurfaces)
	{
		srfVBOMesh_t *vboSurface;

		for (i = 0; i < bspModel->numVBOSurfaces; i++)
		{
			vboSurface = bspModel->vboSurfaces[i];
			if (ent->e.customShader)
			{
				R_AddDrawSurf((surfaceType_t *)vboSurface, R_GetShaderByHandle(ent->e.customShader), vboSurface->lightmapNum, fogNum);
			}
			else
			{
				R_AddDrawSurf((surfaceType_t *)vboSurface, vboSurface->shader, vboSurface->lightmapNum, fogNum);
			}
		}

		// also add surfaces like deform autosprite
		for (i = 0; i < bspModel->numSurfaces; i++)
		{
			bspSurface_t *surf = bspModel->firstSurface + i;

			if (!ShaderRequiresCPUDeforms(surf->shader))
			{
				continue;
			}

			if (ent->e.customShader)
			{
				R_AddBrushModelSurface(surf, R_GetShaderByHandle(ent->e.customShader), fogNum);
			}
			else
			{
				R_AddBrushModelSurface(surf, NULL, fogNum);
			}
		}
	}
	else
	{
		for (i = 0; i < bspModel->numSurfaces; i++)
		{
			if (ent->e.customShader)
			{
				R_AddBrushModelSurface(bspModel->firstSurface + i, R_GetShaderByHandle(ent->e.customShader), fogNum);
			}
			else
			{
				R_AddBrushModelSurface(bspModel->firstSurface + i, NULL, fogNum);
			}
		}
	}
}

/*
=============================================================
    WORLD MODEL
=============================================================
*/

/**
 * @brief R_AddLeafSurfaces
 * @param[in] node
 * @param[in] decalBits
 */
static void R_AddLeafSurfaces(bspNode_t *node, int decalBits)
{
	int          c;
	bspSurface_t *surf, **mark;

	tr.pc.c_leafs++;

	// add to z buffer bounds
	if (node->mins[0] < tr.viewParms.visBounds[0][0])
	{
		tr.viewParms.visBounds[0][0] = node->mins[0];
	}
	if (node->mins[1] < tr.viewParms.visBounds[0][1])
	{
		tr.viewParms.visBounds[0][1] = node->mins[1];
	}
	if (node->mins[2] < tr.viewParms.visBounds[0][2])
	{
		tr.viewParms.visBounds[0][2] = node->mins[2];
	}

	if (node->maxs[0] > tr.viewParms.visBounds[1][0])
	{
		tr.viewParms.visBounds[1][0] = node->maxs[0];
	}
	if (node->maxs[1] > tr.viewParms.visBounds[1][1])
	{
		tr.viewParms.visBounds[1][1] = node->maxs[1];
	}
	if (node->maxs[2] > tr.viewParms.visBounds[1][2])
	{
		tr.viewParms.visBounds[1][2] = node->maxs[2];
	}

	// add the individual surfaces
	mark = node->markSurfaces;
	c    = node->numMarkSurfaces;
	while (c--)
	{
		// the surface may have already been added if it
		// spans multiple leafs
		surf = *mark;
		R_AddWorldSurface(surf, decalBits);
		mark++;
	}
}

/**
 * @brief R_RecursiveWorldNode
 * @param[in] node
 * @param[in] planeBits
 * @param[in] decalBits
 */
static void R_RecursiveWorldNode(bspNode_t *node, int planeBits, int decalBits)
{
	do
	{
		// if the node wasn't marked as potentially visible, exit
		if (node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex])
		{
			return;
		}

		if (node->contents != -1 && !node->numMarkSurfaces)
		{
			// don't waste time dealing with this empty leaf
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?
		if (!r_noCull->integer)
		{
			int i;
			int r;

			for (i = 0; i < FRUSTUM_PLANES; i++)
			{
				if (planeBits & (1 << i))
				{
					r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][i]);
					if (r == 2)
					{
						return; // culled
					}
					if (r == 1)
					{
						planeBits &= ~(1 << i); // all descendants will also be in front
					}
				}
			}
		}

		InsertLink(&node->visChain, &tr.traversalStack);

		// cull decals
		if (decalBits)
		{
			int i;

			for (i = 0; i < tr.refdef.numDecalProjectors; i++)
			{
				if (decalBits & (1 << i))
				{
					// test decal bounds against node surface bounds
					if (tr.refdef.decalProjectors[i].shader == NULL ||
					    !R_TestDecalBoundingBox(&tr.refdef.decalProjectors[i], node->surfMins, node->surfMaxs))
					{
						decalBits &= ~(1 << i);
					}
				}
			}
		}

		if (node->contents != -1)
		{
			break;
		}

		// recurse down the children, front side first
		R_RecursiveWorldNode(node->children[0], planeBits, decalBits);

		// tail recurse
		node = node->children[1];
	}
	while (1);

	if (node->numMarkSurfaces)
	{
		// moved off to separate function
		R_AddLeafSurfaces(node, decalBits);
	}
}

/**
 * @brief R_RecursiveInteractionNode
 * @param[in,out] node
 * @param[in] light
 * @param[in] planeBits
 */
static void R_RecursiveInteractionNode(bspNode_t *node, trRefLight_t *light, int planeBits)
{
	int i;
	int r;

	do
	{
		// if the node wasn't marked as potentially visible, exit
		if (node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex])
		{
			return;
		}

		// light already hit node
		if (node->lightCount == tr.lightCount)
		{
			return;
		}

		node->lightCount = tr.lightCount;

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		// even surfaces that belong to nodes that are outside of the view frustum
		// can cast shadows into the view frustum
		if (!r_noCull->integer && r_shadows->integer <= SHADOWING_BLOB)
		{
			for (i = 0; i < FRUSTUM_PLANES; i++)
			{
				if (planeBits & (1 << i))
				{
					r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][i]);

					if (r == 2)
					{
						return; // culled
					}

					if (r == 1)
					{
						planeBits &= ~(1 << i);    // all descendants will also be in front
					}
				}
			}
		}

		if (node->contents != -1)
		{
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative
		r = BoxOnPlaneSide(light->worldBounds[0], light->worldBounds[1], node->plane);

		switch (r)
		{
		case 1:
			node = node->children[0];
			break;
		case 2:
			node = node->children[1];
			break;
		case 3:
		default:
			// recurse down the children, front side first
			R_RecursiveInteractionNode(node->children[0], light, planeBits);

			// tail recurse
			node = node->children[1];
			break;
		}
	} while (1);


	{
		// leaf node, so add mark surfaces
		int          c;
		bspSurface_t *surf, **mark;

		// add the individual surfaces
		mark = node->markSurfaces;
		c    = node->numMarkSurfaces;

		while (c--)
		{
			// the surface may have already been added if it
			// spans multiple leafs
			surf = *mark;
			R_AddInteractionSurface(surf, light);
			mark++;
		}
	}
}

/**
 * @brief R_PointInLeaf
 * @param[in] p
 * @return
 */
static bspNode_t *R_PointInLeaf(const vec3_t p)
{
	bspNode_t *node;
	float     d;
	cplane_t  *plane;

	if (!tr.world)
	{
		Ren_Drop("R_PointInLeaf: bad model");
	}

	node = tr.world->nodes;
	while (1)
	{
		if (node->contents != -1)
		{
			break;
		}
		plane = node->plane;
		d     = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}

	return node;
}

/**
 * @brief R_ClusterPVS
 * @param[in] cluster
 * @return
 */
static const byte *R_ClusterPVS(int cluster)
{
	if (!tr.world)
	{
		Ren_Drop("R_ClusterPVS: bad model");
	}

	if (!tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters)
	{
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/**
 * @brief R_inPVS
 * @param[in] p1
 * @param[in] p2
 * @return
 */
qboolean R_inPVS(const vec3_t p1, const vec3_t p2)
{
	bspNode_t  *leaf;
	const byte *vis;

	leaf = R_PointInLeaf(p1);
	vis  = R_ClusterPVS(leaf->cluster);
	leaf = R_PointInLeaf(p2);

	if (!(vis[leaf->cluster >> 3] & (1 << (leaf->cluster & 7))))
	{
		return qfalse;
	}
	return qtrue;
}

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
/**
 * @brief R_UpdateClusterSurfaces
 */
static void R_UpdateClusterSurfaces()
{
	int i, k, l;

	int numVerts;
	int numTriangles;
	//static glIndex_t indexes[MAX_MAP_DRAW_INDEXES];
	//static byte     indexes[MAX_MAP_DRAW_INDEXES * sizeof(glIndex_t)];
	glIndex_t *indexes;
	int       indexesSize;

	shader_t *shader, *oldShader;
	int      lightmapNum, oldLightmapNum;

	int          numSurfaces;
	bspSurface_t *surface, *surface2;
	bspSurface_t **surfacesSorted;

	bspCluster_t *cluster;

	srfVBOMesh_t *vboSurf;
	IBO_t        *ibo;

	vec3_t bounds[2];

	if (tr.visClusters[tr.visIndex] < 0 || tr.visClusters[tr.visIndex] >= tr.world->numClusters)
	{
		// this is not a bug, the super cluster is the last one in the array
		cluster = &tr.world->clusters[tr.world->numClusters];
	}
	else
	{
		cluster = &tr.world->clusters[tr.visClusters[tr.visIndex]];
	}

	tr.world->numClusterVBOSurfaces[tr.visIndex] = 0;

	// count number of static cluster surfaces
	numSurfaces = 0;
	for (k = 0; k < cluster->numMarkSurfaces; k++)
	{
		surface = cluster->markSurfaces[k];
		shader  = surface->shader;

		if (shader->isSky)
		{
			continue;
		}

		if (shader->isPortal)
		{
			continue;
		}

		if (ShaderRequiresCPUDeforms(shader))
		{
			continue;
		}

		numSurfaces++;
	}

	if (!numSurfaces)
	{
		return;
	}

	// build interaction caches list
	surfacesSorted = (bspSurface_t **) ri.Hunk_AllocateTempMemory(numSurfaces * sizeof(surfacesSorted[0]));

	numSurfaces = 0;
	for (k = 0; k < cluster->numMarkSurfaces; k++)
	{
		surface = cluster->markSurfaces[k];
		shader  = surface->shader;

		if (shader->isSky)
		{
			continue;
		}

		if (shader->isPortal)
		{
			continue;
		}

		if (ShaderRequiresCPUDeforms(shader))
		{
			continue;
		}

		surfacesSorted[numSurfaces] = surface;
		numSurfaces++;
	}

	// sort surfaces by shader
	qsort(surfacesSorted, numSurfaces, sizeof(surfacesSorted), BSPSurfaceCompare);

	shader      = oldShader = NULL;
	lightmapNum = oldLightmapNum = LIGHTMAP_NONE;

	for (k = 0; k < numSurfaces; k++)
	{
		surface     = surfacesSorted[k];
		shader      = surface->shader;
		lightmapNum = surface->lightmapNum;

		if (shader != oldShader || (r_precomputedLighting->integer ? lightmapNum != oldLightmapNum : 0))
		{
			oldShader      = shader;
			oldLightmapNum = lightmapNum;

			// count vertices and indices
			numVerts     = 0;
			numTriangles = 0;

			for (l = k; l < numSurfaces; l++)
			{
				surface2 = surfacesSorted[l];

				if (surface2->shader != shader || surface2->lightmapNum != lightmapNum)
				{
					continue;
				}

				if (*surface2->data == SF_FACE)
				{
					srfSurfaceFace_t *face = (srfSurfaceFace_t *) surface2->data;

					if (!r_mergeClusterFaces->integer)
					{
						continue;
					}

					if (face->numVerts)
					{
						numVerts += face->numVerts;
					}

					if (face->numTriangles)
					{
						numTriangles += face->numTriangles;
					}
				}
				else if (*surface2->data == SF_GRID)
				{
					srfGridMesh_t *grid = (srfGridMesh_t *) surface2->data;

					if (!r_mergeClusterCurves->integer)
					{
						continue;
					}

					if (grid->numVerts)
					{
						numVerts += grid->numVerts;
					}

					if (grid->numTriangles)
					{
						numTriangles += grid->numTriangles;
					}
				}
				else if (*surface2->data == SF_TRIANGLES)
				{
					srfTriangles_t *tri = (srfTriangles_t *) surface2->data;

					if (!r_mergeClusterTriangles->integer)
					{
						continue;
					}

					if (tri->numVerts)
					{
						numVerts += tri->numVerts;
					}

					if (tri->numTriangles)
					{
						numTriangles += tri->numTriangles;
					}
				}
			}

			if (!numVerts || !numTriangles)
			{
				continue;
			}

			ClearBounds(bounds[0], bounds[1]);

			// build triangle indices
			indexesSize = numTriangles * 3 * sizeof(glIndex_t);
			indexes     = (glIndex_t *) ri.Hunk_AllocateTempMemory(indexesSize);

			numTriangles = 0;
			for (l = k; l < numSurfaces; l++)
			{
				surface2 = surfacesSorted[l];

				if (surface2->shader != shader || surface2->lightmapNum != lightmapNum)
				{
					continue;
				}

				// set up triangle indices
				if (*surface2->data == SF_FACE)
				{
					srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface2->data;

					if (!r_mergeClusterFaces->integer)
					{
						continue;
					}

					if (srf->numTriangles)
					{
						srfTriangle_t *tri;

						for (i = 0, tri = tr.world->triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
						{
							indexes[numTriangles * 3 + i * 3 + 0] = tri->indexes[0];
							indexes[numTriangles * 3 + i * 3 + 1] = tri->indexes[1];
							indexes[numTriangles * 3 + i * 3 + 2] = tri->indexes[2];
						}

						numTriangles += srf->numTriangles;
						BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
					}
				}
				else if (*surface2->data == SF_GRID)
				{
					srfGridMesh_t *srf = (srfGridMesh_t *) surface2->data;

					if (!r_mergeClusterCurves->integer)
					{
						continue;
					}

					if (srf->numTriangles)
					{
						srfTriangle_t *tri;

						for (i = 0, tri = tr.world->triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
						{
							indexes[numTriangles * 3 + i * 3 + 0] = tri->indexes[0];
							indexes[numTriangles * 3 + i * 3 + 1] = tri->indexes[1];
							indexes[numTriangles * 3 + i * 3 + 2] = tri->indexes[2];
						}

						numTriangles += srf->numTriangles;
						BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
					}
				}
				else if (*surface2->data == SF_TRIANGLES)
				{
					srfTriangles_t *srf = (srfTriangles_t *) surface2->data;

					if (!r_mergeClusterTriangles->integer)
					{
						continue;
					}

					if (srf->numTriangles)
					{
						srfTriangle_t *tri;

						for (i = 0, tri = tr.world->triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
						{
							indexes[numTriangles * 3 + i * 3 + 0] = tri->indexes[0];
							indexes[numTriangles * 3 + i * 3 + 1] = tri->indexes[1];
							indexes[numTriangles * 3 + i * 3 + 2] = tri->indexes[2];
						}

						numTriangles += srf->numTriangles;
						BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
					}
				}
			}

			if (tr.world->numClusterVBOSurfaces[tr.visIndex] < tr.world->clusterVBOSurfaces[tr.visIndex].currentElements)
			{
				vboSurf =
				    (srfVBOMesh_t *) Com_GrowListElement(&tr.world->clusterVBOSurfaces[tr.visIndex],
				                                         tr.world->numClusterVBOSurfaces[tr.visIndex]);
				ibo = vboSurf->ibo;

				/*
				   if(ibo->indexesVBO)
				   {
				   glDeleteBuffers(1, &ibo->indexesVBO);
				   ibo->indexesVBO = 0;
				   }
				 */

				//Com_Dealloc(ibo);
				//Com_Dealloc(vboSurf);
			}
			else
			{
				vboSurf              = (srfVBOMesh_t *) ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
				vboSurf->surfaceType = SF_VBO_MESH;

				vboSurf->vbo = tr.world->vbo;
				vboSurf->ibo = ibo = (IBO_t *) ri.Hunk_Alloc(sizeof(*ibo), h_low);

				glGenBuffers(1, &ibo->indexesVBO);

				Com_AddToGrowList(&tr.world->clusterVBOSurfaces[tr.visIndex], vboSurf);
			}

			//Ren_Print("creating VBO cluster surface for shader '%s'\n", shader->name);

			// update surface properties
			vboSurf->numIndexes = numTriangles * 3;
			vboSurf->numVerts   = numVerts;

			vboSurf->shader      = shader;
			vboSurf->lightmapNum = lightmapNum;

			VectorCopy(bounds[0], vboSurf->bounds[0]);
			VectorCopy(bounds[1], vboSurf->bounds[1]);

			R_IssuePendingRenderCommands();

			// update IBO
			Q_strncpyz(ibo->name,
			           va("staticWorldMesh_IBO_visIndex%i_surface%i", tr.visIndex, tr.world->numClusterVBOSurfaces[tr.visIndex]),
			           sizeof(ibo->name));
			ibo->indexesSize = indexesSize;

			R_BindIBO(ibo);

			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexesSize, indexes, GL_DYNAMIC_DRAW);

			R_BindNullIBO();

			//GL_CheckErrors();

			ri.Hunk_FreeTempMemory(indexes);

			tr.world->numClusterVBOSurfaces[tr.visIndex]++;
		}
	}

	ri.Hunk_FreeTempMemory(surfacesSorted);

	if (r_showcluster->modified || r_showcluster->integer)
	{
		r_showcluster->modified = qfalse;
		if (r_showcluster->integer)
		{
			Ren_Print("  surfaces:%i\n", tr.world->numClusterVBOSurfaces[tr.visIndex]);
		}
	}
}
#endif // #if defined(USE_BSP_CLUSTERSURFACE_MERGING)

/**
 * @brief Mark the leaves and nodes that are in the PVS for the current
 */
static void R_MarkLeaves(void)
{
	const byte *vis;
	bspNode_t  *leaf, *parent;
	int        i;
	int        cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if (r_lockPvs->integer) // || r_dynamicBspOcclusionCulling->integer)
	{
		return;
	}

	// current viewcluster
	leaf    = R_PointInLeaf(tr.viewParms.pvsOrigin);
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	for (i = 0; i < MAX_VISCOUNTS; i++)
	{
		if (tr.visClusters[i] == cluster)
		{
			// if r_showcluster was just turned on, remark everything
			if (!tr.refdef.areamaskModified && !r_showCluster->modified) // && !r_dynamicBspOcclusionCulling->modified)
			{
				if (tr.visClusters[i] != tr.visClusters[tr.visIndex] && r_showCluster->integer)
				{
					Ren_Print("found cluster:%i  area:%i  index:%i\n", cluster, leaf->area, i);
				}
				tr.visIndex = i;
				return;
			}

			if (tr.refdef.areamaskModified)
			{
				// invalidate old visclusters so they will be updated next time
				tr.visClusters[i] = -1;
			}
		}
	}

	tr.visIndex = (tr.visIndex + 1) % MAX_VISCOUNTS;
	tr.visCounts[tr.visIndex]++;
	tr.visClusters[tr.visIndex] = cluster;

	if (r_showCluster->modified || r_showCluster->integer)
	{
		r_showCluster->modified = qfalse;
		if (r_showCluster->integer)
		{
			Ren_Print("update cluster:%i  area:%i  index:%i\n", cluster, leaf->area, tr.visIndex);
		}
	}

	/*
	if(r_dynamicBspOcclusionCulling->modified)
	{
	    r_dynamicBspOcclusionCulling->modified = qfalse;
	}
	*/

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
	if (r_mergeClusterSurfaces->integer && !r_dynamicBspOcclusionCulling->integer)
	{
		R_UpdateClusterSurfaces();
	}
#endif

	if (r_noVis->integer || tr.visClusters[tr.visIndex] == -1)
	{
		for (i = 0; i < tr.world->numnodes; i++)
		{
			if (tr.world->nodes[i].contents != CONTENTS_SOLID)
			{
				tr.world->nodes[i].visCounts[tr.visIndex] = tr.visCounts[tr.visIndex];
			}
		}
		return;
	}

	vis = R_ClusterPVS(tr.visClusters[tr.visIndex]);

	for (i = 0, leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++)
	{
		cluster = leaf->cluster;

		if (cluster < 0 || cluster >= tr.world->numClusters)
		{
			continue;
		}

		// check general pvs
		if (!(vis[cluster >> 3] & (1 << (cluster & 7))))
		{
			continue;
		}

		// check for door connection
		if ((tr.refdef.areamask[leaf->area >> 3] & (1 << (leaf->area & 7))))
		{
			// not visible
			continue;
		}

		// don't want to walk the entire bsp to add skybox surfaces
		if (tr.refdef.rdflags & RDF_SKYBOXPORTAL)
		{
			// this only happens once, as game/cgame know the origin of the skybox
			// this also means the skybox portal cannot move, as this list is calculated once and never again
			if (tr.world->numSkyNodes < WORLD_MAX_SKY_NODES)
			{
				tr.world->skyNodes[tr.world->numSkyNodes++] = leaf;
			}
			R_AddLeafSurfaces(leaf, 0);
			continue;
		}

		parent = leaf;
		do
		{
			if (parent->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex])
			{
				break;
			}
			parent->visCounts[tr.visIndex] = tr.visCounts[tr.visIndex];
			parent                         = parent->parent;
		}
		while (parent);
	}
}

/**
 * @brief DrawLeaf
 * @param[in] node
 * @param[in] decalBits
 */
static void DrawLeaf(bspNode_t *node, int decalBits)
{
	// leaf node, so add mark surfaces
	int          c;
	bspSurface_t *surf, **mark;

	tr.pc.c_leafs++;

	// add to z buffer bounds
	if (node->mins[0] < tr.viewParms.visBounds[0][0])
	{
		tr.viewParms.visBounds[0][0] = node->mins[0];
	}
	if (node->mins[1] < tr.viewParms.visBounds[0][1])
	{
		tr.viewParms.visBounds[0][1] = node->mins[1];
	}
	if (node->mins[2] < tr.viewParms.visBounds[0][2])
	{
		tr.viewParms.visBounds[0][2] = node->mins[2];
	}

	if (node->maxs[0] > tr.viewParms.visBounds[1][0])
	{
		tr.viewParms.visBounds[1][0] = node->maxs[0];
	}
	if (node->maxs[1] > tr.viewParms.visBounds[1][1])
	{
		tr.viewParms.visBounds[1][1] = node->maxs[1];
	}
	if (node->maxs[2] > tr.viewParms.visBounds[1][2])
	{
		tr.viewParms.visBounds[1][2] = node->maxs[2];
	}

	// add the individual surfaces
	mark = node->markSurfaces;
	c    = node->numMarkSurfaces;
	while (c--)
	{
		// the surface may have already been added if it
		// spans multiple leafs
		surf = *mark;
		R_AddWorldSurface(surf, decalBits);
		mark++;
	}
}

// ================================================================================================
// BSP OCCLUSION CULLING
// ================================================================================================

/**
 * @brief InsideViewFrustum
 * @param[in] node
 * @param[in] planeBits
 * @return
 */
static qboolean InsideViewFrustum(bspNode_t *node, int planeBits)
{
	if (!r_noCull->integer)
	{
		int i;
		int r;

		for (i = 0; i < FRUSTUM_PLANES; i++)
		{
			if (planeBits & (1 << i))
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][i]);
				if (r == 2)
				{
					return qfalse;  // culled
				}
				if (r == 1)
				{
					planeBits &= ~(1 << i); // all descendants will also be in front
				}
			}
		}
	}

	return qtrue;
}

//#define DEBUG_CHC 1

/*
 * @brief DrawNode_r
 * @param[in,out] node
 * @param[in] planeBits
 *
 * @note Unused
static void DrawNode_r(bspNode_t *node, int planeBits)
{
    do
    {
        // if the bounding volume is outside the frustum, nothing
        // inside can be visible OPTIMIZE: don't do this all the way to leafs?
        if (!r_nocull->integer)
        {
            int i;
            int r;

            for (i = 0; i < FRUSTUM_PLANES; i++)
            {
                if (planeBits & (1 << i))
                {
                    r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][i]);
                    if (r == 2)
                    {
                        return; // culled
                    }
                    if (r == 1)
                    {
                        planeBits &= ~(1 << i); // all descendants will also be in front
                    }
                }
            }
        }

       Ren_LogComment("--- DrawNode_r( node = %li, isLeaf = %i ) ---\n", (long)(node - tr.world->nodes), node->contents == -1);

        if (node->contents != -1) // && !(node->contents & CONTENTS_TRANSLUCENT))
        {
            SetUniformVec4(UNIFORM_COLOR, colorGreen);
        }
        else
        {
            SetUniformVec4(UNIFORM_COLOR, colorMdGrey);
        }

        // draw bsp leave or node
        {
            R_BindVBO(node->volumeVBO);
            R_BindIBO(node->volumeIBO);

            GLSL_VertexAttribsState(ATTR_POSITION);

            tess.numVertexes = node->volumeVerts;
            tess.numIndexes  = node->volumeIndexes;

            Tess_DrawElements();

            tess.multiDrawPrimitives = 0;
            tess.numIndexes          = 0;
            tess.numVertexes         = 0;
        }

        if (node->contents != -1)
        {
            break;
        }

        // recurse down the children, front side first
        DrawNode_r(node->children[0], planeBits);

        // tail recurse
        node = node->children[1];
    }
    while (1);
}
*/

/**
 * @brief IssueOcclusionQuery
 * @param[in] queue
 * @param[in,out] node
 * @param[in] resetMultiQueryLink
 */
static void IssueOcclusionQuery(link_t *queue, bspNode_t *node, qboolean resetMultiQueryLink)
{
#if defined(DEBUG_CHC)
	if (RENLOG)
	{
		if (node->contents != -1) // && !(node->contents & CONTENTS_TRANSLUCENT))
		{
			Ren_LogComment("--- IssueOcclusionQuery( leaf = %i ) ---\n", node - tr.world->nodes);
			gl_genericShader->SetUniform_Color(colorGreen);
		}
		else
		{
			Ren_LogComment("--- IssueOcclusionQuery( node = %i ) ---\n", node - tr.world->nodes);
			gl_genericShader->SetUniform_Color(colorMdGrey);
		}
	}
#endif

	EnQueue(queue, node);

	// tell GetOcclusionQueryResult that this is not a multi query
	if (resetMultiQueryLink)
	{
		QueueInit(&node->multiQuery);
	}

	GL_CheckErrors();

#if 0
	if (glIsQuery(node->occlusionQueryObjects[tr.viewCount]))
	{
		Ren_Fatal("IssueOcclusionQuery: node %i has already an occlusion query object in slot %i: %i", node - tr.world->nodes, tr.viewCount, node->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	// begin the occlusion query
	glBeginQuery(GL_SAMPLES_PASSED, node->occlusionQueryObjects[tr.viewCount]);

	GL_CheckErrors();

	R_BindVBO(node->volumeVBO);
	R_BindIBO(node->volumeIBO);

	GLSL_VertexAttribsState(ATTR_POSITION);

	tess.numVertexes = node->volumeVerts;
	tess.numIndexes  = node->volumeIndexes;

	Tess_DrawElements();

	// end the query
	glEndQuery(GL_SAMPLES_PASSED);

#if 1
	if (!glIsQuery(node->occlusionQueryObjects[tr.viewCount]))
	{
		// Ren_Fatal("IssueOcclusionQuery: node %i has no occlusion query object in slot %i: %i", node - tr.world->nodes, tr.viewCount, node->occlusionQueryObjects[tr.viewCount]);
		Ren_Fatal("IssueOcclusionQuery: node %li has no occlusion query object in slot %i %lu", (long)(node - tr.world->nodes), tr.viewCount, (unsigned long)node->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	node->occlusionQueryNumbers[tr.viewCount] = tr.pc.c_occlusionQueries;
	tr.pc.c_occlusionQueries++;

	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;

	GL_CheckErrors();

	Ren_LogComment("--- IssueOcclusionQuery end ---\n");
}

/**
 * @brief IssueMultiOcclusionQueries
 * @param[in] multiQueue
 * @param[in] individualQueue
 */
static void IssueMultiOcclusionQueries(link_t *multiQueue, link_t *individualQueue)
{
	bspNode_t *node;
	bspNode_t *multiQueryNode;
	link_t    *l;

	if (RENLOG)
	{
		Ren_LogComment("IssueMultiOcclusionQueries([");

		for (l = multiQueue->prev; l != multiQueue; l = l->prev)
		{
			node = (bspNode_t *) l->data;

			Ren_LogComment("%li, ", (long)(node - tr.world->nodes));
		}
		Ren_LogComment("])");
	}

	if (QueueEmpty(multiQueue))
	{
		return;
	}

	multiQueryNode = (bspNode_t *) QueueFront(multiQueue)->data;

	// begin the occlusion query

	GL_CheckErrors();

#if 0
	if (!glIsQuery(multiQueryNode->occlusionQueryObjects[tr.viewCount]))
	{
		Ren_Fatal("IssueMultiOcclusionQueries: node %i has already occlusion query object in slot %i: %i", multiQueryNode - tr.world->nodes, tr.viewCount, multiQueryNode->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	glBeginQuery(GL_SAMPLES_PASSED, multiQueryNode->occlusionQueryObjects[tr.viewCount]);

	GL_CheckErrors();

	Ren_LogComment("rendering nodes:[");
	for (l = multiQueue->prev; l != multiQueue; l = l->prev)
	{
		node = (bspNode_t *) l->data;

		if (node->contents != -1) // && !(node->contents & CONTENTS_TRANSLUCENT))
		{
			SetUniformVec4(UNIFORM_COLOR, colorGreen);
		}
		else
		{
			SetUniformVec4(UNIFORM_COLOR, colorMdGrey);
		}

		Ren_LogComment("%li, ", (long)(node - tr.world->nodes));

		//Tess_EndBegin();

		R_BindVBO(node->volumeVBO);
		R_BindIBO(node->volumeIBO);

		GLSL_VertexAttribsState(ATTR_POSITION);

		tess.multiDrawPrimitives = 0;
		tess.numVertexes         = node->volumeVerts;
		tess.numIndexes          = node->volumeIndexes;

		Tess_DrawElements();

		tess.numIndexes  = 0;
		tess.numVertexes = 0;
	}
	Ren_LogComment("]\n");

	multiQueryNode->occlusionQueryNumbers[tr.viewCount] = tr.pc.c_occlusionQueries;
	tr.pc.c_occlusionQueries++;
	tr.pc.c_occlusionQueriesMulti++;

	// end the query
	glEndQuery(GL_SAMPLES_PASSED);

	GL_CheckErrors();

#if 0
	if (!glIsQuery(multiQueryNode->occlusionQueryObjects[tr.viewCount]))
	{
		Ren_Fatal("IssueMultiOcclusionQueries: node %i has no occlusion query object in slot %i: %i", multiQueryNode - tr.world->nodes, tr.viewCount, multiQueryNode->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	// move queue to node->multiQuery queue
	QueueInit(&multiQueryNode->multiQuery);
	DeQueue(multiQueue);
	while (!QueueEmpty(multiQueue))
	{
		node = (bspNode_t *) DeQueue(multiQueue);
		EnQueue(&multiQueryNode->multiQuery, node);
	}

	EnQueue(individualQueue, multiQueryNode);

	Ren_LogComment("--- IssueMultiOcclusionQueries end ---\n");
}

/**
 * @brief ResultAvailable
 * @param[in] node
 * @return
 */
static GLint ResultAvailable(bspNode_t *node)
{
	GLint available;

	//glFinish();

	available = 0;
	//if(glIsQuery(node->occlusionQueryObjects[tr.viewCount]))
	{
		glGetQueryObjectiv(node->occlusionQueryObjects[tr.viewCount], GL_QUERY_RESULT_AVAILABLE, &available);
		GL_CheckErrors();
	}

	return available;
}

/**
 * @brief GetOcclusionQueryResult
 * @param[in,out] node
 */
static void GetOcclusionQueryResult(bspNode_t *node)
{
	link_t *l, *sentinel;
	int    ocSamples;
	GLint  available = 0;

	Ren_LogComment("--- GetOcclusionQueryResult ---\n");

	//glFinish();

#if 0
	if (!glIsQuery(node->occlusionQueryObjects[tr.viewCount]))
	{
		Ren_Fatal("GetOcclusionQueryResult: node %i has no occlusion query object in slot %i: %i", node - tr.world->nodes, tr.viewCount, node->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	while (!available)
	{
		//if(glIsQuery(node->occlusionQueryObjects[tr.viewCount]))
		{
			glGetQueryObjectiv(node->occlusionQueryObjects[tr.viewCount], GL_QUERY_RESULT_AVAILABLE, &available);
			//GL_CheckErrors();
		}
	}

	glGetQueryObjectiv(node->occlusionQueryObjects[tr.viewCount], GL_QUERY_RESULT, &ocSamples);


	Ren_LogComment("GetOcclusionQueryResult(%li): available = %i, samples = %i\n", (long)(node - tr.world->nodes), available, ocSamples);

	GL_CheckErrors();

	node->occlusionQuerySamples[tr.viewCount] = ocSamples;
	node->lastQueried[tr.viewCount]           = tr.frameCount;

	// copy result to all nodes that were linked to this multi query node
	sentinel = &node->multiQuery;
	for (l = sentinel->prev; l != sentinel; l = l->prev)
	{
		node = (bspNode_t *) l->data;

		node->occlusionQuerySamples[tr.viewCount] = ocSamples;
		node->lastQueried[tr.viewCount]           = tr.frameCount;
	}
}

/**
 * @brief PullUpVisibility
 * @param[in] node
 */
static void PullUpVisibility(bspNode_t *node)
{
	bspNode_t *parent = node;

	while (parent && !parent->visible[tr.viewCount])
	{
		parent->visible[tr.viewCount]     = qtrue;
		parent->lastVisited[tr.viewCount] = tr.frameCount;

		parent = parent->parent;
	}
}

/*
 * @brief PushNode
 * @param[in,out] traversalStack
 * @param[in] node
 *
 * @note Unused
static void PushNode(link_t * traversalStack, bspNode_t * node)
{
    if(node->contents != -1)
    {
        //DrawLeaf(node, tr.refdef.decalBits);
    }
    else
    {
        //float			d1, d2;
        cplane_t       *splitPlane;

        splitPlane = node->plane;

        //d1 = DistanceSquared(tr.viewParms.orientation.origin, node->children[0]->origin);
        //d2 = DistanceSquared(tr.viewParms.orientation.origin, node->children[1]->origin);

        //if(d1 <= d2)
#if 0
        if(DotProduct(splitPlane->normal, tr.viewParms.orientation.axis[0]) <= 0)
        {
            StackPush(traversalStack, node->children[0]);
            StackPush(traversalStack, node->children[1]);

            Ren_Print("--> %i\n", node->children[0] - tr.world->nodes);
            Ren_Print("--> %i\n", node->children[1] - tr.world->nodes);
        }
        else
#endif
        {
#if 1
            StackPush(traversalStack, node->children[0]);
            StackPush(traversalStack, node->children[1]);
#else
            InsertLink(&((bspNode_t*)node->children[0])->visChain, traversalStack);
            InsertLink(&((bspNode_t*)node->children[1])->visChain, traversalStack);

            traversalStack->numElements += 2;
#endif
            Ren_LogComment("traversal-stack <-- node %i\n", node->children[0] - tr.world->nodes);
            Ren_LogComment("traversal-stack <-- node %i\n", node->children[1] - tr.world->nodes);
        }
    }
}
*/

/**
 * @brief TraverseNode
 * @param[in] distanceQueue
 * @param[in] node
 */
static void TraverseNode(link_t *distanceQueue, bspNode_t *node)
{
#if defined(DEBUG_CHC)
	if (RENLOG)
	{
		if (node->contents != -1)
		{
			Ren_LogComment("--- TraverseNode( leaf = %i ) ---\n", node - tr.world->nodes);

			gl_genericShader->SetUniform_Color(colorGreen);
		}
		else
		{
			Ren_LogComment("--- TraverseNode( node = %i ) ---\n", node - tr.world->nodes);

			gl_genericShader->SetUniform_Color(colorMdGrey);
		}

		// draw bsp leave or node
		{
			R_BindVBO(node->volumeVBO);
			R_BindIBO(node->volumeIBO);

			GLSL_VertexAttribsState(ATTR_POSITION);

			tess.numVertexes = node->volumeVerts;
			tess.numIndexes  = node->volumeIndexes;

			Tess_DrawElements();

			tess.multiDrawPrimitives = 0;
			tess.numIndexes          = 0;
			tess.numVertexes         = 0;
		}
	}
#endif

	if (node->contents != -1)
	{
		//DrawLeaf(node, tr.refdef.decalBits);
	}
	else
	{
		EnQueue(distanceQueue, node->children[0]);
		EnQueue(distanceQueue, node->children[1]);

		Ren_LogComment("distance-queue <-- node %li\n", (long)(node->children[0] - tr.world->nodes));
		Ren_LogComment("distance-queue <-- node %li\n", (long)(node->children[1] - tr.world->nodes));
	}
}

/**
 * @brief BuildNodeTraversalStackPost_r
 * @param[in,out] node
 */
static void BuildNodeTraversalStackPost_r(bspNode_t *node)
{
	do
	{
		if (tr.frameCount != node->lastVisited[tr.viewCount])
		{
			return;
		}

		#if defined(DEBUG_CHC)
		if (RENLOG)
		{
			if (node->contents != -1)
			{
				Ren_LogComment("--- BuildNodeTraversalStackPost_r( leaf = %i, visible = %i ) ---\n", node - tr.world->nodes, node->visible[tr.viewCount]);
			}
			else
			{
				Ren_LogComment("--- BuildNodeTraversalStackPost_r( node = %i, visible = %i ) ---\n", node - tr.world->nodes, node->visible[tr.viewCount]);
			}
		}
		#endif

		InsertLink(&node->visChain, &tr.traversalStack);

		if (node->contents != -1)
		{
			if (node->visible[tr.viewCount])
			{
				DrawLeaf(node, tr.refdef.decalBits);
			}
			break;
		}

		// recurse down the children, front side first
		BuildNodeTraversalStackPost_r(node->children[0]);

		// tail recurse
		node = node->children[1];
	}
	while (1);
}

/**
 * @brief WasVisible
 * @param[in] node
 * @return
 */
static qboolean WasVisible(bspNode_t *node)
{
	if (node->visible[tr.viewCount] && ((tr.frameCount - node->lastVisited[tr.viewCount]) <= r_chcMaxVisibleFrames->integer))
	{
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief QueryReasonable
 * @param[in] node
 * @return
 */
static qboolean QueryReasonable(bspNode_t *node)
{
	// if r_chcMaxVisibleFrames 10 then range from 5 to 10
	//return ((tr.frameCount - node->lastQueried[tr.viewCount]) > r_chcMaxVisibleFrames->integer);
	if ((tr.frameCount - node->lastQueried[tr.viewCount]) > Q_min((int)ceil((r_chcMaxVisibleFrames->value * 0.5f) + (r_chcMaxVisibleFrames->value * 0.5f) * random()), r_chcMaxVisibleFrames->integer))
	{
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief R_CoherentHierachicalCulling
 */
static void R_CoherentHierachicalCulling()
{
	bspNode_t *node;
	bspNode_t *multiQueryNode;
	//	link_t traversalStack;
	link_t distanceQueue;
	link_t occlusionQueryQueue;
	link_t visibleQueue;          // CHC++
	link_t invisibleQueue;          // CHC++
	//link_t		renderQueue;
	int startTime = 0;

	Ren_LogComment("--- R_CoherentHierachicalCulling ---\n");

	Ren_LogComment("tr.viewCount = %i, tr.viewCountNoReset = %i\n", tr.viewCount, tr.viewCountNoReset);

	R2_TIMING_SIMPLE()
	{
		startTime = ri.Milliseconds();
	}

	if (HDR_ENABLED())
	{
		R_BindFBO(tr.deferredRenderFBO);
	}
	else
	{
		R_BindNullFBO();
	}

	SetMacrosAndSelectProgram(trProg.gl_genericShader);

	GL_Cull(CT_TWO_SIDED);

	GL_LoadProjectionMatrix(tr.viewParms.projectionMatrix);

	GL_Viewport(tr.viewParms.viewportX, tr.viewParms.viewportY,
	            tr.viewParms.viewportWidth, tr.viewParms.viewportHeight);

	GL_Scissor(tr.viewParms.viewportX, tr.viewParms.viewportY,
	           tr.viewParms.viewportWidth, tr.viewParms.viewportHeight);

	// set uniforms
	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CONST, AGEN_CONST);
	SetUniformVec4(UNIFORM_COLOR, colorWhite);

	// set up the transformation matrix
	GL_LoadModelViewMatrix(tr.orientation.modelViewMatrix);

	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(tr.whiteImage);

	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);

#if 0
	GL_ClearColor(GLCOLOR_BLACK);
	GL_Clear(GL_COLOR_BUFFER_BIT);

	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);

	// draw BSP leaf volumes to color for debugging
	DrawNode_r(&tr.world->nodes[0], FRUSTUM_CLIPALL);
#endif


#if 0
	GL_ClearColor(GLCOLOR_NONE);
	GL_Clear(GL_DEPTH_BUFFER_BIT);

	GL_State(GLS_COLORMASK_BITS | GLS_DEPTHMASK_TRUE);

	// draw BSP leaf volumes to depth
	DrawNode_r(&tr.world->nodes[0], FRUSTUM_CLIPALL);
#endif

	if (RENLOG)
	{
		GL_ClearColor(GLCOLOR_BLACK);
		GL_Clear(GL_COLOR_BUFFER_BIT);

		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	}
	else
	{
		// use the depth buffer of the previous frame for occlusion culling
		GL_State(GLS_COLORMASK_BITS);
	}

	ClearLink(&tr.traversalStack);
	QueueInit(&tr.occlusionQueryQueue);
	ClearLink(&tr.occlusionQueryList);

	//ClearLink(&traversalStack);
	QueueInit(&distanceQueue);
	QueueInit(&occlusionQueryQueue);
	QueueInit(&visibleQueue);
	QueueInit(&invisibleQueue);
	//QueueInit(&renderQueue);

	EnQueue(&distanceQueue, &tr.world->nodes[0]);
	//StackPush(&traversalStack, &tr.world->nodes[0]);

	/*
	ClearLink(&traversalStack);
	traversalStack.numElements = 0;

	node = &tr.world->nodes[0];
	InsertLink(&node->visChain, &traversalStack);
	traversalStack.numElements++;
	*/

	while (!QueueEmpty(&distanceQueue) || !QueueEmpty(&occlusionQueryQueue) || !QueueEmpty(&invisibleQueue) || !QueueEmpty(&visibleQueue))
	{
		Ren_LogComment("--- (distanceQueue = %i, occlusionQueryQueue = %i, invisibleQueue = %i, visibleQueue = %i)\n", QueueSize(&distanceQueue), QueueSize(&occlusionQueryQueue), QueueSize(&invisibleQueue), QueueSize(&visibleQueue));

		//--PART 1: process finished occlusion queries
		while (!QueueEmpty(&occlusionQueryQueue) && (ResultAvailable((bspNode_t *) QueueFront(&occlusionQueryQueue)->data) || QueueEmpty(&distanceQueue)))
		{
			if (ResultAvailable((bspNode_t *) QueueFront(&occlusionQueryQueue)->data))
			{
				node = (bspNode_t *) DeQueue(&occlusionQueryQueue);

				// wait if result not available
				GetOcclusionQueryResult(node);

				if (node->occlusionQuerySamples[tr.viewCount] > r_chcVisibilityThreshold->integer)
				{
					// if a query of multiple previously invisible objects became visible, we need to
					// test all the individual objects ...
					if (!QueueEmpty(&node->multiQuery))
					{
						Ren_LogComment("MULTI query node %li visible\n", (long)(node - tr.world->nodes));

						multiQueryNode = node;

						IssueOcclusionQuery(&occlusionQueryQueue, multiQueryNode, qfalse);

						while (!QueueEmpty(&multiQueryNode->multiQuery))
						{
							node = (bspNode_t *) DeQueue(&multiQueryNode->multiQuery);

							// it might be possible that a leaf caused this node to be visible by a PullUpVisibility() call
							// so avoid a further query
							if (!(node->visible[tr.viewCount] && tr.frameCount == node->lastVisited[tr.viewCount]))
							{
								IssueOcclusionQuery(&occlusionQueryQueue, node, qtrue);
							}
						}
					}
					else
					{
						Ren_LogComment("single query node %li visible\n", (long)(node - tr.world->nodes));

						if (r_dynamicBspOcclusionCulling->integer == 1)
						{
							if (!WasVisible(node))
							{
								TraverseNode(&distanceQueue, node);
							}
						}
						else
						{
							TraverseNode(&distanceQueue, node);
						}

						PullUpVisibility(node);
					}
				}
				else
				{
					node->visible[tr.viewCount] = qfalse;

					if (!QueueEmpty(&node->multiQuery))
					{
						// node was an invisible multi query node so dequeue all its children
						multiQueryNode = node;
						while (!QueueEmpty(&multiQueryNode->multiQuery))
						{
							node = (bspNode_t *) DeQueue(&multiQueryNode->multiQuery);

							node->visible[tr.viewCount] = qfalse;

							tr.pc.c_occlusionQueriesSaved++;
						}
					}
				}
			}
#if 1
			else if (r_dynamicBspOcclusionCulling->integer == 1)
			{
				if (!QueueEmpty(&visibleQueue))
				{
					node = (bspNode_t *) DeQueue(&visibleQueue);

					IssueOcclusionQuery(&occlusionQueryQueue, node, qtrue);
				}
			}
#endif

		} // end while(!QueueEmpty(&occlusionQueryQueue))

		//--PART 2: hierarchical traversal
		if (!QueueEmpty(&distanceQueue)) //if(!StackEmpty(&traversalStack))
		{
			node = (bspNode_t *) DeQueue(&distanceQueue);
			//node = (bspNode_t *) StackPop(&traversalStack);

			/*
			link_t* top = traversalStack.next;
			RemoveLink(top);

			node = (bspNode_t *) top->data;
			*/

			Ren_LogComment("distance-queue --> node %li\n", (long)(node - tr.world->nodes));

			if (node->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex] && // node was marked as potentially visible
			    (node->contents == -1 || node->numMarkSurfaces) &&
			    InsideViewFrustum(node, FRUSTUM_CLIPALL)
			    )
			{
				// identify previously visible nodes
				qboolean wasVisible = WasVisible(node);
				// identify nodes that we cannot skip queries for
				qboolean needsQuery;
				qboolean clipsNearPlane;
				qboolean leafThatNeedsQuery;

				if (r_dynamicBspOcclusionCulling->integer > 1)
				{
					// reset node's visibility classification
					//node->visible[tr.viewCount] = !QueryReasonable(node);
					if (QueryReasonable(node))
					{
						node->visible[tr.viewCount] = qfalse;
					}
					else
					{
						node->visible[tr.viewCount] = qtrue;
					}
				}

				clipsNearPlane = (BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][FRUSTUM_NEAR]) == 3);

				if (clipsNearPlane)
				{
					// node clips near plane so avoid the occlusion query test
					node->occlusionQuerySamples[tr.viewCount] = r_chcVisibilityThreshold->integer + 1;
					node->lastQueried[tr.viewCount]           = tr.frameCount;
					node->visible[tr.viewCount]               = qtrue;

					needsQuery = qfalse;
				}
				#if 1
				else if (r_chcIgnoreLeaves->integer && node->contents != -1)
				{
					// NOTE: this is the fastest dynamic occlusion culling path

					// only very few leaves are invisible if we don't traverse through all bsp nodes
					// so testing these leaves just causes additional occlusion queries which can be avoided
					// by setting all reached leaves to visible
					node->occlusionQuerySamples[tr.viewCount] = r_chcVisibilityThreshold->integer + 1;
					node->lastQueried[tr.viewCount]           = tr.frameCount;
					node->visible[tr.viewCount]               = qtrue;

					needsQuery = qfalse;
				}
				#endif
				else
				{
					// CHC default
					needsQuery = !wasVisible || (node->contents != -1);
				}

				// update node's visited flag
				node->lastVisited[tr.viewCount] = tr.frameCount;

				leafThatNeedsQuery = node->contents != -1;

				if (leafThatNeedsQuery)
				{
					if (r_chcIgnoreLeaves->integer)
					{
						leafThatNeedsQuery = qfalse;
					}
				}
				else
				{
					leafThatNeedsQuery = qtrue;
				}

				if (r_dynamicBspOcclusionCulling->integer == 1)
				{
					// CHC++

					if (!wasVisible && !clipsNearPlane && leafThatNeedsQuery)
					{
						Ren_LogComment("i-queue <-- node %li\n", (long)(node - tr.world->nodes));

						EnQueue(&invisibleQueue, node);

						if (QueueSize(&invisibleQueue) >= r_chcMaxPrevInvisNodesBatchSize->integer)
						{
							IssueMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);
						}
					}
					else
					{
						#if 1
						if ((node->contents != -1) && !clipsNearPlane && QueryReasonable(node) && leafThatNeedsQuery)
						{
							Ren_LogComment("v-queue <-- node %li\n", (long)(node - tr.world->nodes));

							EnQueue(&visibleQueue, node);
						}
						#endif

						// always traverse a node if it was visible
						TraverseNode(&distanceQueue, node);
					}
				}
				else
				{
					// CHC default

					if (needsQuery) //!wasVisible && !clipsNearPlane)
					{
						IssueOcclusionQuery(&occlusionQueryQueue, node, qtrue);
					}

					if (wasVisible)
					{
						// always traverse a node if it was visible
						TraverseNode(&distanceQueue, node);

						//if(clipsNearPlane)
						//{
						//	PullUpVisibility(node);
						//}
					}
				}
			}
		}

		if (r_dynamicBspOcclusionCulling->integer == 1)
		{
			if (QueueEmpty(&distanceQueue))
			//if(StackEmpty(&traversalStack))
			{
				// remaining previously visible node queries
				if (!QueueEmpty(&invisibleQueue))
				{
					IssueMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);
				}

				if (!QueueEmpty(&visibleQueue))
				{
					while (!QueueEmpty(&visibleQueue))
					{
						node = (bspNode_t *) DeQueue(&visibleQueue);

						IssueOcclusionQuery(&occlusionQueryQueue, node, qtrue);
					}
				}
			}
		}
		//Ren_Print("--- (%i, %i, %i)\n", !StackEmpty(&traversalStack), !QueueEmpty(&occlusionQueryQueue), !QueueEmpty(&invisibleQueue));
	}

	ClearLink(&tr.traversalStack);
	BuildNodeTraversalStackPost_r(&tr.world->nodes[0]);

	R_BindNullFBO();

	// reenable color buffer and depth buffer writes
	GL_State(GLS_DEFAULT);

	GL_CheckErrors();

	//Ren_Print("--- R_CHC++ end ---\n");

	R2_TIMING_SIMPLE()
	{
		tr.pc.c_CHCTime = ri.Milliseconds() - startTime;
	}
}

/**
 * @brief R_AddWorldSurfaces
 */
void R_AddWorldSurfaces(void)
{
	if (!r_drawWorld->integer)
	{
		return;
	}

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// render sky or world?
	if ((tr.refdef.rdflags & RDF_SKYBOXPORTAL) && tr.world->numSkyNodes > 0)
	{
		int       i;
		bspNode_t **node;

		for (i = 0, node = tr.world->skyNodes; i < tr.world->numSkyNodes; i++, node++)
		{
			R_AddLeafSurfaces(*node, 0);    // no decals on skybox nodes
		}
	}
	else
	{
		// determine which leaves are in the PVS / areamask
		R_MarkLeaves();

		// update the bsp nodes with the dynamic occlusion query results
		if (glConfig2.occlusionQueryBits && r_dynamicBspOcclusionCulling->integer)
		{
			R_CoherentHierachicalCulling();
		}
		else
		{
			ClearLink(&tr.traversalStack);
			ClearLink(&tr.occlusionQueryQueue);
			ClearLink(&tr.occlusionQueryList);

			// update visbounds and add surfaces that weren't cached with VBOs
			R_RecursiveWorldNode(tr.world->nodes, FRUSTUM_CLIPALL, tr.refdef.decalBits);
		}

		// add decal surfaces
		R_AddDecalSurfaces(tr.world->models);

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
		if (r_mergeClusterSurfaces->integer && !r_dynamicBspOcclusionCulling->integer)
		{
			int          j, i;
			srfVBOMesh_t *srf;
			shader_t     *shader;
			cplane_t     *frust;
			int          r;

			for (j = 0; j < tr.world->numClusterVBOSurfaces[tr.visIndex]; j++)
			{
				srf    = (srfVBOMesh_t *) Com_GrowListElement(&tr.world->clusterVBOSurfaces[tr.visIndex], j);
				shader = srf->shader;

				for (i = 0; i < FRUSTUM_PLANES; i++)
				{
					frust = &tr.viewParms.frustums[0][i];

					r = BoxOnPlaneSide(srf->bounds[0], srf->bounds[1], frust);

					if (r == 2)
					{
						// completely outside frustum
						continue;
					}
				}

				R_AddDrawSurf((surfaceType_t *)srf, shader, srf->lightmapNum, 0);
			}
		}
#endif
	}
}

/**
 * @brief R_AddWorldInteractions
 * @param[in] light
 */
void R_AddWorldInteractions(trRefLight_t *light)
{
	if (!r_drawWorld->integer)
	{
		return;
	}

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	// perform frustum culling and add all the potentially visible surfaces
	tr.lightCount++;
	R_RecursiveInteractionNode(tr.world->nodes, light, FRUSTUM_CLIPALL);
}

/**
 * @brief R_AddPrecachedWorldInteractions
 * @param[in] light
 */
void R_AddPrecachedWorldInteractions(trRefLight_t *light)
{
	interactionType_t iaType = IA_DEFAULT;

	if (!r_drawWorld->integer)
	{
		return;
	}

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	if (!light->firstInteractionCache)
	{
		// this light has no interactions precached
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	if ((r_vboShadows->integer || r_vboLighting->integer)) // && light->l.rlType != RL_DIRECTIONAL)
	{
		interactionCache_t *iaCache;
		interactionVBO_t   *iaVBO;
		srfVBOMesh_t       *srf;
		shader_t           *shader;
		bspSurface_t       *surface;

		// this can be shadow mapping or shadowless lighting
		for (iaVBO = light->firstInteractionVBO; iaVBO; iaVBO = iaVBO->next)
		{
			if (!iaVBO->vboLightMesh)
			{
				continue;
			}

			srf    = iaVBO->vboLightMesh;
			shader = iaVBO->shader;

			switch (light->l.rlType)
			{
			case RL_OMNI:
				R_AddLightInteraction(light, (surfaceType_t *)srf, shader, CUBESIDE_CLIPALL, IA_LIGHTONLY);
				break;
			case RL_DIRECTIONAL:
			case RL_PROJ:
				R_AddLightInteraction(light, (surfaceType_t *)srf, shader, CUBESIDE_CLIPALL, IA_LIGHTONLY);
				break;
			default:
				R_AddLightInteraction(light, (surfaceType_t *)srf, shader, CUBESIDE_CLIPALL, IA_DEFAULT);
				break;
			}
		}

		// add meshes for shadowmap generation if any
		for (iaVBO = light->firstInteractionVBO; iaVBO; iaVBO = iaVBO->next)
		{
			if (!iaVBO->vboShadowMesh)
			{
				continue;
			}

			srf    = iaVBO->vboShadowMesh;
			shader = iaVBO->shader;

			R_AddLightInteraction(light, (surfaceType_t *)srf, shader, iaVBO->cubeSideBits, IA_SHADOWONLY);
		}

		// add interactions that couldn't be merged into VBOs
		for (iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
		{
			if (iaCache->redundant)
			{
				continue;
			}

			if (iaCache->mergedIntoVBO)
			{
				continue;
			}

			surface = iaCache->surface;

			// this surface is maybe not in this view but it may still cast a shadow
			// into this view
			if (surface->viewCount != tr.viewCountNoReset)
			{
				if (r_shadows->integer < SHADOWING_ESM16 || light->l.noShadows)
				{
					continue;
				}
				else
				{
					iaType = IA_SHADOWONLY;
				}
			}
			else
			{
				iaType = iaCache->type;
			}

			R_AddLightInteraction(light, surface->data, surface->shader, iaCache->cubeSideBits, iaType);
		}
	}
	else
	{
		interactionCache_t *iaCache;
		bspSurface_t       *surface;

		for (iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
		{
			if (iaCache->redundant)
			{
				continue;
			}

			surface = iaCache->surface;

			// this surface is maybe not in this view but it may still cast a shadow
			// into this view
			if (surface->viewCount != tr.viewCountNoReset)
			{
				if (r_shadows->integer < SHADOWING_ESM16 || light->l.noShadows)
				{
					continue;
				}
				else
				{
					iaType = IA_SHADOWONLY;
				}
			}
			else
			{
				iaType = iaCache->type;
			}

			R_AddLightInteraction(light, surface->data, surface->shader, iaCache->cubeSideBits, iaType);
		}
	}
}
