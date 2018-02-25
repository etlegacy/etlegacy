/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
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
 * @file rendererGLES/tr_world.c
 */

#include "tr_local.h"

/**
 * @brief Tries to back face cull surfaces before they are lighted or
 * added to the sorting list.
 *
 * This will also allow mirrors on both sides of a model without recursion.
 *
 * @param[in] surface
 * @param[in] shader
 * @param[out] frontFace
 * @return
 */
static qboolean R_CullSurface(surfaceType_t *surface, shader_t *shader, int *frontFace)
{
	srfGeneric_t *gen;

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
		float d = DotProduct(tr.orientation.viewOrigin, gen->plane.normal) - gen->plane.dist;

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

	{
		int cull;

		// try sphere cull
		if (tr.currentEntityNum != ENTITYNUM_WORLD)
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
	}

	// must be visible
	return qfalse;
}

/**
 * @brief The given surface is going to be drawn, and it touches a leaf
 * that is touched by one or more dlights, so try to throw out
 * more dlights if possible.
 *
 * @param[in] surface
 * @param[in] dlightBits
 * @return
 *
 * @todo Made this use generic surface
 */
static int R_DlightSurface(msurface_t *surface, int dlightBits)
{
	int          i;
	vec3_t       origin;
	float        radius;
	srfGeneric_t *gen;

	// made surface dlighting generic, inline with q3map2 surface classification
	switch ((surfaceType_t) *surface->data)
	{
	case SF_FACE:
	case SF_TRIANGLES:
	case SF_GRID:
	case SF_FOLIAGE:
		break;

	default:
		return 0;
	}

	// get generic surface
	gen = (srfGeneric_t *) surface->data;

	// debug code
	// gen->dlightBits = dlightBits;
	// return dlightBits;

	// try to cull out dlights
	for (i = 0; i < tr.refdef.num_dlights; i++)
	{
		if (!(dlightBits & (1 << i)))
		{
			continue;
		}

		// junior dlights don't affect world surfaces
		if (tr.refdef.dlights[i].flags & REF_JUNIOR_DLIGHT)
		{
			dlightBits &= ~(1 << i);
			continue;
		}

		// lightning dlights affect all surfaces
		if (tr.refdef.dlights[i].flags & REF_DIRECTED_DLIGHT)
		{
			continue;
		}

		// test surface bounding sphere against dlight bounding sphere
		VectorCopy(tr.refdef.dlights[i].transformed, origin);
		radius = tr.refdef.dlights[i].radius;

		if ((gen->origin[0] + gen->radius) < (origin[0] - radius) ||
		    (gen->origin[0] - gen->radius) > (origin[0] + radius) ||
		    (gen->origin[1] + gen->radius) < (origin[1] - radius) ||
		    (gen->origin[1] - gen->radius) > (origin[1] + radius) ||
		    (gen->origin[2] + gen->radius) < (origin[2] - radius) ||
		    (gen->origin[2] - gen->radius) > (origin[2] + radius))
		{
			dlightBits &= ~(1 << i);
		}
	}

	// Ren_Print("Surf: 0x%08X dlightBits: 0x%08X\n", srf, dlightBits );

	// set counters
	if (dlightBits == 0)
	{
		tr.pc.c_dlightSurfacesCulled++;
	}
	else
	{
		tr.pc.c_dlightSurfaces++;
	}

	// set surface dlight bits and return
	gen->dlightBits = dlightBits;
	return dlightBits;
}

/**
 * @brief R_AddWorldSurface
 * @param[in,out] surf
 * @param[in] shader
 * @param[in] dlightMap
 * @param[in] decalBits
 */
static void R_AddWorldSurface(msurface_t *surf, shader_t *shader, int dlightMap, int decalBits)
{
	int frontFace;

	if (surf->viewCount == tr.viewCount)
	{
		return;     // already in this view

	}
	surf->viewCount = tr.viewCount;
	// FIXME: bmodel fog?

	// try to cull before dlighting or adding
	if (R_CullSurface(surf->data, shader, &frontFace))
	{
		return;
	}

	// check for dlighting
	if (dlightMap)
	{
		dlightMap = R_DlightSurface(surf, dlightMap);
		dlightMap = (dlightMap != 0);
	}

	// add decals
	if (decalBits)
	{
		int i;

		// project any decals
		for (i = 0; i < tr.refdef.numDecalProjectors; i++)
		{
			if (decalBits & (1 << i))
			{
				R_ProjectDecalOntoSurface(&tr.refdef.decalProjectors[i], surf, tr.currentBModel);
			}
		}
	}

	R_AddDrawSurf(surf->data, shader, surf->fogIndex, frontFace, dlightMap);
}

/*
=============================================================
    BRUSH MODELS
=============================================================
*/

/**
 * @brief See if a sprite is inside a fog volume
 *
 * @param[in] re
 * @param[in] bmodel
 *
 * @return Positive with /any part/ of the brush falling within a fog volume
 *
 * @note the original implementation of this function is a bit flaky...
 */
int R_BmodelFogNum(trRefEntity_t *re, bmodel_t *bmodel)
{
	int   i, j;
	fog_t *fog;

	for (i = 1; i < tr.world->numfogs; i++)
	{
		fog = &tr.world->fogs[i];
		for (j = 0; j < 3; j++)
		{
			if (re->e.origin[j] + bmodel->bounds[0][j] >= fog->bounds[1][j])
			{
				break;
			}
			if (re->e.origin[j] + bmodel->bounds[1][j] <= fog->bounds[0][j])
			{
				break;
			}
		}
		if (j == 3)
		{
			return i;
		}
	}

	return 0;
}

/**
 * @brief R_AddBrushModelSurfaces
 * @param[in] ent
 */
void R_AddBrushModelSurfaces(trRefEntity_t *ent)
{
	int              i, fognum, decalBits;
	vec3_t           mins, maxs;
	model_t          *pModel;
	bmodel_t         *bmodel;
	int              savedNumDecalProjectors, numLocalProjectors;
	decalProjector_t *savedDecalProjectors, localProjectors[MAX_DECAL_PROJECTORS];

	pModel = R_GetModelByHandle(ent->e.hModel);

	bmodel = pModel->model.bmodel;

	if (R_CullLocalBox(bmodel->bounds) == CULL_OUT)
	{
		return;
	}

	// set current brush model to world
	tr.currentBModel = bmodel;

	// set model state for decals and dynamic fog
	VectorCopy(ent->e.origin, bmodel->orientation.origin);
	VectorCopy(ent->e.axis[0], bmodel->orientation.axis[0]);
	VectorCopy(ent->e.axis[1], bmodel->orientation.axis[1]);
	VectorCopy(ent->e.axis[2], bmodel->orientation.axis[2]);

	R_DlightBmodel(bmodel);

	// determine if in fog
	fognum = R_BmodelFogNum(ent, bmodel);

	// project any decals
	decalBits          = 0;
	numLocalProjectors = 0;
	for (i = 0; i < tr.refdef.numDecalProjectors; i++)
	{
		// early out
		if (tr.refdef.decalProjectors[i].shader == NULL)
		{
			continue;
		}

		// transform entity bbox (fixme: rotated entities have invalid bounding boxes)
		VectorAdd(bmodel->bounds[0], tr.orientation.origin, mins);
		VectorAdd(bmodel->bounds[1], tr.orientation.origin, maxs);

		// set bit
		if (R_TestDecalBoundingBox(&tr.refdef.decalProjectors[i], mins, maxs))
		{
			R_TransformDecalProjector(&tr.refdef.decalProjectors[i], tr.orientation.axis, tr.orientation.origin, &localProjectors[numLocalProjectors]);
			numLocalProjectors++;
			decalBits <<= 1;
			decalBits  |= 1;
		}
	}

	// save old decal projectors
	savedNumDecalProjectors = tr.refdef.numDecalProjectors;
	savedDecalProjectors    = tr.refdef.decalProjectors;

	// set local decal projectors
	tr.refdef.numDecalProjectors = numLocalProjectors;
	tr.refdef.decalProjectors    = localProjectors;

	// add model surfaces
	for (i = 0; i < bmodel->numSurfaces; i++)
	{
		(bmodel->firstSurface + i)->fogIndex = fognum;
		// custom shader support for brushmodels
		if (ent->e.customShader)
		{
			R_AddWorldSurface(bmodel->firstSurface + i, R_GetShaderByHandle(ent->e.customShader), tr.currentEntity->needDlights, decalBits);
		}
		else
		{
			R_AddWorldSurface(bmodel->firstSurface + i, (( msurface_t * )(bmodel->firstSurface + i))->shader, tr.currentEntity->needDlights, decalBits);
		}
	}

	// restore old decal projectors
	tr.refdef.numDecalProjectors = savedNumDecalProjectors;
	tr.refdef.decalProjectors    = savedDecalProjectors;

	// add decal surfaces
	R_AddDecalSurfaces(bmodel);

	// clear current brush model
	tr.currentBModel = NULL;
}

/*
=============================================================
    WORLD MODEL
=============================================================
*/

/**
 * @brief Adds a leaf's drawsurfaces
 * @param[in] node
 * @param[in] dlightBits
 * @param[in] decalBits
 */
static void R_AddLeafSurfaces(mnode_t *node, int dlightBits, int decalBits)
{
	int        c;
	msurface_t *surf, **mark;

	// add to count
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
	mark = node->firstmarksurface;
	c    = node->nummarksurfaces;
	while (c--)
	{
		// the surface may have already been added if it
		// spans multiple leafs
		surf = *mark;
		R_AddWorldSurface(surf, surf->shader, dlightBits, decalBits);
		mark++;
	}
}

/**
 * @brief R_RecursiveWorldNode
 * @param[in] node
 * @param[in] planeBits
 * @param[in] dlightBits
 * @param[in] decalBits
 */
static void R_RecursiveWorldNode(mnode_t *node, int planeBits, int dlightBits, int decalBits)
{
	int      i, r;
	dlight_t *dl;

	do
	{
		// if the node wasn't marked as potentially visible, exit
		if (node->visframe != tr.visCount)
		{
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if (!r_noCull->integer)
		{
			if (planeBits & 1)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2)
				{
					return;                     // culled
				}
				if (r == 1)
				{
					planeBits &= ~1;            // all descendants will also be in front
				}
			}

			if (planeBits & 2)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2)
				{
					return;                     // culled
				}
				if (r == 1)
				{
					planeBits &= ~2;            // all descendants will also be in front
				}
			}

			if (planeBits & 4)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2)
				{
					return;                     // culled
				}
				if (r == 1)
				{
					planeBits &= ~4;            // all descendants will also be in front
				}
			}

			if (planeBits & 8)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2)
				{
					return;                     // culled
				}
				if (r == 1)
				{
					planeBits &= ~8;            // all descendants will also be in front
				}
			}

			// farplane culling
			if (planeBits & 16)
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[4]);
				if (r == 2)
				{
					return;                     // culled
				}
				if (r == 1)
				{
					planeBits &= ~8;            // all descendants will also be in front
				}
			}

		}

		// cull dlights
		if (dlightBits)      //%    && node->contents != -1 )
		{
			for (i = 0; i < tr.refdef.num_dlights; i++)
			{
				if (dlightBits & (1 << i))
				{
					// directional dlights don't get culled
					if (tr.refdef.dlights[i].flags & REF_DIRECTED_DLIGHT)
					{
						continue;
					}

					// test dlight bounds against node surface bounds
					dl = &tr.refdef.dlights[i];
					if (node->surfMins[0] >= (dl->origin[0] + dl->radius) || node->surfMaxs[0] <= (dl->origin[0] - dl->radius) ||
					    node->surfMins[1] >= (dl->origin[1] + dl->radius) || node->surfMaxs[1] <= (dl->origin[1] - dl->radius) ||
					    node->surfMins[2] >= (dl->origin[2] + dl->radius) || node->surfMaxs[2] <= (dl->origin[2] - dl->radius))
					{
						dlightBits &= ~(1 << i);
					}
				}
			}
		}

		// cull decals
		if (decalBits)
		{
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

		// handle leaf nodes
		if (node->contents != -1)
		{
			break;
		}

		// recurse down the children, front side first
		R_RecursiveWorldNode(node->children[0], planeBits, dlightBits, decalBits);

		// tail recurse
		node = node->children[1];
	}
	while (1);

	// short circuit
	if (node->nummarksurfaces == 0)
	{
		return;
	}

	// moved off to separate function
	R_AddLeafSurfaces(node, dlightBits, decalBits);
}

/**
 * @brief R_PointInLeaf
 * @param[in] p
 * @return
 */
static mnode_t *R_PointInLeaf(const vec3_t p)
{
	mnode_t  *node;
	float    d;
	cplane_t *plane;

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
	mnode_t    *leaf;
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

/**
 * @brief Mark the leaves and nodes that are in the PVS for the current cluster
 */
static void R_MarkLeaves(void)
{
	const byte *vis;
	mnode_t    *leaf, *parent;
	int        i;
	int        cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if (r_lockPvs->integer)
	{
		return;
	}

	// current viewcluster
	leaf    = R_PointInLeaf(tr.viewParms.pvsOrigin);
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	// if r_showcluster was just turned on, remark everything
	if (tr.viewCluster == cluster && !tr.refdef.areamaskModified
	    && !r_showCluster->modified)
	{
		return;
	}

	if (r_showCluster->modified || r_showCluster->integer)
	{
		r_showCluster->modified = qfalse;
		if (r_showCluster->integer)
		{
			Ren_Print("cluster:%i  area:%i\n", cluster, leaf->area);
		}
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if (r_noVis->integer || tr.viewCluster == -1)
	{
		for (i = 0 ; i < tr.world->numnodes ; i++)
		{
			if (tr.world->nodes[i].contents != CONTENTS_SOLID)
			{
				tr.world->nodes[i].visframe = tr.visCount;
			}
		}
		return;
	}

	vis = R_ClusterPVS(tr.viewCluster);

	for (i = 0, leaf = tr.world->nodes ; i < tr.world->numnodes ; i++, leaf++)
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
			continue;       // not visible
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
			R_AddLeafSurfaces(leaf, 0, 0);
			continue;
		}

		parent = leaf;
		do
		{
			if (parent->visframe == tr.visCount)
			{
				break;
			}
			parent->visframe = tr.visCount;
			parent           = parent->parent;
		}
		while (parent);
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

	tr.currentEntityNum = ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	// set current brush model to world
	tr.currentBModel = &tr.world->bmodels[0];

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// render sky or world?
	if ((tr.refdef.rdflags & RDF_SKYBOXPORTAL) && tr.world->numSkyNodes > 0)
	{
		int     i;
		mnode_t **node;

		for (i = 0, node = tr.world->skyNodes; i < tr.world->numSkyNodes; i++, node++)
			R_AddLeafSurfaces(*node, tr.refdef.dlightBits, 0);      // no decals on skybox nodes
	}
	else
	{
		// determine which leaves are in the PVS / areamask
		R_MarkLeaves();

		// perform frustum culling and add all the potentially visible surfaces
		R_RecursiveWorldNode(tr.world->nodes, 255, tr.refdef.dlightBits, tr.refdef.decalBits);

		// add decal surfaces
		R_AddDecalSurfaces(tr.world->bmodels);
	}

	// clear brush model
	tr.currentBModel = NULL;
}
