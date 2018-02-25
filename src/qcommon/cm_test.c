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
 * @file cm_test.c
 */

#include "cm_local.h"

/**
 * @brief CM_PointLeafnum_r
 * @param[in] p
 * @param[in] num
 * @return
 */
int CM_PointLeafnum_r(const vec3_t p, int num)
{
	float    d;
	cNode_t  *node;
	cplane_t *plane;

	while (num >= 0)
	{
		node  = cm.nodes + num;
		plane = node->plane;

		if (plane->type < 3)
		{
			d = p[plane->type] - plane->dist;
		}
		else
		{
			d = DotProduct(plane->normal, p) - plane->dist;
		}
		if (d < 0)
		{
			num = node->children[1];
		}
		else
		{
			num = node->children[0];
		}
	}

	c_pointcontents++;      // optimize counter

	return -1 - num;
}

/**
 * @brief CM_PointLeafnum
 * @param[in] p
 * @return
 */
int CM_PointLeafnum(const vec3_t p)
{
	if (!cm.numNodes)       // map not loaded
	{
		return 0;
	}

	return CM_PointLeafnum_r(p, 0);
}

/**
======================================================================
LEAF LISTING
======================================================================
*/

/**
 * @brief CM_StoreLeafs
 * @param[in,out] ll
 * @param[in] nodenum
 */
void CM_StoreLeafs(leafList_t *ll, int nodenum)
{
	int leafNum = -1 - nodenum;

	// store the lastLeaf even if the list is overflowed
	if (cm.leafs[leafNum].cluster != -1)
	{
		ll->lastLeaf = leafNum;
	}

	if (ll->count >= ll->maxcount)
	{
		ll->overflowed = qtrue;
		return;
	}

	ll->list[ll->count++] = leafNum;
}

/**
 * @brief CM_StoreBrushes
 * @param[in,out] ll
 * @param[in] nodenum
 */
void CM_StoreBrushes(leafList_t *ll, int nodenum)
{
	int      i, k;
	int      leafnum = -1 - nodenum;
	int      brushnum;
	cLeaf_t  *leaf = &cm.leafs[leafnum];
	cbrush_t *b;

	for (k = 0 ; k < leaf->numLeafBrushes ; k++)
	{
		brushnum = cm.leafbrushes[leaf->firstLeafBrush + k];
		b        = &cm.brushes[brushnum];
		if (b->checkcount == cm.checkcount)
		{
			continue;   // already checked this brush in another leaf
		}
		b->checkcount = cm.checkcount;
		for (i = 0 ; i < 3 ; i++)
		{
			if (b->bounds[0][i] >= ll->bounds[1][i] || b->bounds[1][i] <= ll->bounds[0][i])
			{
				break;
			}
		}
		if (i != 3)
		{
			continue;
		}
		if (ll->count >= ll->maxcount)
		{
			ll->overflowed = qtrue;
			return;
		}

		((cbrush_t **)ll->list)[ll->count++] = b;
	}
#if 0
	// store patches?
	for (k = 0 ; k < leaf->numLeafSurfaces ; k++)
	{
		patch = cm.surfaces[cm.leafsurfaces[leaf->firstleafsurface + k]];
		if (!patch)
		{
			continue;
		}
	}
#endif
}

/**
 * @brief Fills in a list of all the leafs touched
 * @param[in,out] ll
 * @param[in] nodenum
 *
 */
void CM_BoxLeafnums_r(leafList_t *ll, int nodenum)
{
	cplane_t *plane;
	cNode_t  *node;
	int      s;

	while (1)
	{
		if (nodenum < 0)
		{
			ll->storeLeafs(ll, nodenum);
			return;
		}

		node  = &cm.nodes[nodenum];
		plane = node->plane;
		s     = BoxOnPlaneSide(ll->bounds[0], ll->bounds[1], plane);
		if (s == 1)
		{
			nodenum = node->children[0];
		}
		else if (s == 2)
		{
			nodenum = node->children[1];
		}
		else
		{
			// go down both
			CM_BoxLeafnums_r(ll, node->children[0]);
			nodenum = node->children[1];
		}
	}
}

/**
 * @brief CM_BoxLeafnums
 * @param[in] mins
 * @param[in] maxs
 * @param[in] list
 * @param[in] listsize
 * @param[out] lastLeaf
 * @return
 */
int CM_BoxLeafnums(const vec3_t mins, const vec3_t maxs, int *list, int listsize, int *lastLeaf)
{
	leafList_t ll;

	cm.checkcount++;

	VectorCopy(mins, ll.bounds[0]);
	VectorCopy(maxs, ll.bounds[1]);
	ll.count      = 0;
	ll.maxcount   = listsize;
	ll.list       = list;
	ll.storeLeafs = CM_StoreLeafs;
	ll.lastLeaf   = 0;
	ll.overflowed = qfalse;

	CM_BoxLeafnums_r(&ll, 0);

	*lastLeaf = ll.lastLeaf;
	return ll.count;
}

/**
 * @brief CM_BoxBrushes
 * @param[in] mins
 * @param[in] maxs
 * @param[in] list
 * @param[in] listsize
 * @return
 *
 * @note Unused
 */
int CM_BoxBrushes(const vec3_t mins, const vec3_t maxs, cbrush_t **list, int listsize)
{
	leafList_t ll;

	cm.checkcount++;

	VectorCopy(mins, ll.bounds[0]);
	VectorCopy(maxs, ll.bounds[1]);
	ll.count      = 0;
	ll.maxcount   = listsize;
	ll.list       = (void *)list;
	ll.storeLeafs = CM_StoreBrushes;
	ll.lastLeaf   = 0;
	ll.overflowed = qfalse;

	CM_BoxLeafnums_r(&ll, 0);

	return ll.count;
}

//====================================================================

/**
 * @brief CM_PointContents
 * @param[in] p
 * @param[in] model
 * @return
 */
int CM_PointContents(const vec3_t p, clipHandle_t model)
{
	int      leafnum;
	int      i, k;
	int      brushnum;
	cLeaf_t  *leaf;
	cbrush_t *b;
	int      contents;
	float    d;
	cmodel_t *clipm;

	if (!cm.numNodes)     // map not loaded
	{
		return 0;
	}

	if (model)
	{
		clipm = CM_ClipHandleToModel(model);
		leaf  = &clipm->leaf;
	}
	else
	{
		leafnum = CM_PointLeafnum_r(p, 0);
		leaf    = &cm.leafs[leafnum];
	}

	contents = 0;
	for (k = 0 ; k < leaf->numLeafBrushes ; k++)
	{
		brushnum = cm.leafbrushes[leaf->firstLeafBrush + k];
		b        = &cm.brushes[brushnum];

		// see if the point is in the brush
		for (i = 0 ; i < b->numsides ; i++)
		{
			d = DotProduct(p, b->sides[i].plane->normal);
// FIXME test for Cash
//          if ( d >= b->sides[i].plane->dist ) {
			if (d > b->sides[i].plane->dist)
			{
				break;
			}
		}

		if (i == b->numsides)
		{
			contents |= b->contents;
		}
	}

	return contents;
}

/**
 * @brief Handles offseting and rotation of the end points for moving and
 * rotating entities
 * @param[in] p
 * @param[in] model
 * @param[in] origin
 * @param[in] angles
 * @return
 */
int CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles)
{
	vec3_t p_l;

	// subtract origin offset
	VectorSubtract(p, origin, p_l);

	// rotate start and end into the models frame of reference
	if (model != BOX_MODEL_HANDLE &&
	    (angles[0] != 0.f || angles[1] != 0.f || angles[2] != 0.f))
	{
		vec3_t temp, forward, right, up;

		angles_vectors(angles, forward, right, up);

		VectorCopy(p_l, temp);
		p_l[0] = DotProduct(temp, forward);
		p_l[1] = -DotProduct(temp, right);
		p_l[2] = DotProduct(temp, up);
	}

	return CM_PointContents(p_l, model);
}

/**
===============================================================================
PVS
===============================================================================
*/

/**
 * @brief CM_ClusterPVS
 * @param[in] cluster
 * @return
 */
byte *CM_ClusterPVS(int cluster)
{
	if (cluster < 0 || cluster >= cm.numClusters || !cm.vised)
	{
		return cm.visibility;
	}

	return cm.visibility + cluster * cm.clusterBytes;
}

/**
===============================================================================
AREAPORTALS
===============================================================================
*/

/**
 * @brief CM_FloodArea_r
 * @param[in] areaNum
 * @param[in] floodnum
 */
void CM_FloodArea_r(int areaNum, int floodnum)
{
	int     i;
	cArea_t *area = &cm.areas[areaNum];
	int     *con;

	if (area->floodvalid == cm.floodvalid)
	{
		if (area->floodnum == floodnum)
		{
			return;
		}

		Com_Error(ERR_DROP, "CM_FloodArea_r: reflooded");
	}

	area->floodnum   = floodnum;
	area->floodvalid = cm.floodvalid;
	con              = cm.areaPortals + areaNum * cm.numAreas;
	for (i = 0 ; i < cm.numAreas  ; i++)
	{
		if (con[i] > 0)
		{
			CM_FloodArea_r(i, floodnum);
		}
	}
}

/**
 * @brief CM_FloodAreaConnections
 */
void CM_FloodAreaConnections(void)
{
	int     i;
	cArea_t *area    = cm.areas; // optimization
	int     floodnum = 0;

	// all current floods are now invalid
	cm.floodvalid++;

	for (i = 0 ; i < cm.numAreas ; i++, area++)
	{
		if (area->floodvalid == cm.floodvalid)
		{
			continue;       // already flooded into
		}

		floodnum++;
		CM_FloodArea_r(i, floodnum);
	}
}

/**
 * @brief CM_AdjustAreaPortalState
 * @param[in] area1
 * @param[in] area2
 * @param[in] open
 */
void CM_AdjustAreaPortalState(int area1, int area2, qboolean open)
{
	if (area1 < 0 || area2 < 0)
	{
		return;
	}

	if (area1 >= cm.numAreas || area2 >= cm.numAreas)
	{
		Com_Error(ERR_DROP, "CM_ChangeAreaPortalState: bad area number");
	}

	if (open)
	{
		cm.areaPortals[area1 * cm.numAreas + area2]++;
		cm.areaPortals[area2 * cm.numAreas + area1]++;
	}
	else if (cm.areaPortals[area2 * cm.numAreas + area1])         // Ridah, fixes loadgame issue
	{
		cm.areaPortals[area1 * cm.numAreas + area2]--;
		cm.areaPortals[area2 * cm.numAreas + area1]--;
		if (cm.areaPortals[area2 * cm.numAreas + area1] < 0)
		{
			Com_Error(ERR_DROP, "CM_AdjustAreaPortalState: negative reference count");
		}
	}

	CM_FloodAreaConnections();
}

/**
 * @brief CM_AreasConnected
 * @param[in] area1
 * @param[in] area2
 * @return
 */
qboolean CM_AreasConnected(int area1, int area2)
{
	if (cm_noAreas->integer)
	{
		return qtrue;
	}

	if (area1 < 0 || area2 < 0)
	{
		return qfalse;
	}

	if (area1 >= cm.numAreas || area2 >= cm.numAreas)
	{
		Com_Error(ERR_DROP, "CM_AreasConnected: area >= cm.numAreas");
	}

	if (cm.areas[area1].floodnum == cm.areas[area2].floodnum)
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Writes a bit vector of all the areas that are in the same flood as the area parameter
 *
 * @details The bits are OR'd in, so you can CM_WriteAreaBits from multiple
 * viewpoints and get the union of all visible areas.
 *
 * This is used to cull non-visible entities from snapshots
 *
 * @param[out] buffer
 * @param[in] area
 *
 * @return The number of bytes needed to hold all the bits.
 */
int CM_WriteAreaBits(byte *buffer, int area)
{
	int bytes = (cm.numAreas + 7) >> 3;

	if (cm_noAreas->integer || area == -1) // for debugging, send everything
	{
		Com_Memset(buffer, 255, bytes);
	}
	else
	{
		int i;
		int floodnum = cm.areas[area].floodnum;

		for (i = 0 ; i < cm.numAreas ; i++)
		{
			if (cm.areas[i].floodnum == floodnum || area == -1)
			{
				buffer[i >> 3] |= 1 << (i & 7);
			}
		}
	}

	return bytes;
}
