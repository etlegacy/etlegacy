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
 * @file renderer/tr_marks.c
 * @brief Polygon projection on the world polygons
 */

#include "tr_local.h"

#define MAX_VERTS_ON_POLY       64

#define MARKER_OFFSET           0   // 1

// just make these global to prevent having to add more paramaters, which add overhead
static vec3_t bestnormal;
static float  bestdist;

#define SIDE_FRONT  0
#define SIDE_BACK   1
#define SIDE_ON     2

/**
 * @brief R_ChopPolyBehindPlane
 * @param[in] numInPoints
 * @param[in] inPoints
 * @param[out] numOutPoints
 * @param[out] outPoints Out must have space for two more vertexes than in
 * @param[in] normal
 * @param[in] dist
 * @param[in] epsilon
 */
static void R_ChopPolyBehindPlane(int numInPoints, vec3_t inPoints[MAX_VERTS_ON_POLY],
                                  int *numOutPoints, vec3_t outPoints[MAX_VERTS_ON_POLY],
                                  vec3_t normal, vec_t dist, vec_t epsilon)
{
	float dists[MAX_VERTS_ON_POLY + 4];
	int   sides[MAX_VERTS_ON_POLY + 4];
	int   counts[3];
	float dot;
	int   i, j;
	float *p1, *p2, *clip;
	float d;

	Com_Memset(dists, 0, sizeof(dists));
	Com_Memset(sides, 0, sizeof(sides));

	// don't clip if it might overflow
	if (numInPoints >= MAX_VERTS_ON_POLY - 2)
	{
		*numOutPoints = 0;
		return;
	}

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i = 0; i < numInPoints; i++)
	{
		dot      = DotProduct(inPoints[i], normal);
		dot     -= dist;
		dists[i] = dot;
		if (dot > epsilon)
		{
			sides[i] = SIDE_FRONT;
		}
		else if (dot < -epsilon)
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*numOutPoints = 0;

	if (!counts[0])
	{
		return;
	}
	if (!counts[1])
	{
		*numOutPoints = numInPoints;
		Com_Memcpy(outPoints, inPoints, numInPoints * sizeof(vec3_t));
		return;
	}

	for (i = 0; i < numInPoints; i++)
	{
		p1   = inPoints[i];
		clip = outPoints[*numOutPoints];

		if (sides[i] == SIDE_ON)
		{
			VectorCopy(p1, clip);
			(*numOutPoints)++;
			continue;
		}

		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy(p1, clip);
			(*numOutPoints)++;
			clip = outPoints[*numOutPoints];
		}

		if (sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
		{
			continue;
		}

		// generate a split point
		p2 = inPoints[(i + 1) % numInPoints];

		d = dists[i] - dists[i + 1];
		if (d == 0.f)
		{
			dot = 0;
		}
		else
		{
			dot = dists[i] / d;
		}

		// clip xyz

		for (j = 0; j < 3; j++)
		{
			clip[j] = p1[j] + dot * (p2[j] - p1[j]);
		}

		(*numOutPoints)++;
	}
}

/**
 * @brief R_BoxSurfaces_r
 * @param[in] node
 * @param[in] mins
 * @param[in] maxs
 * @param[in,out] list
 * @param[in] listsize
 * @param[in,out] listlength
 * @param[in] dir
 */
void R_BoxSurfaces_r(mnode_t *node, vec3_t mins, vec3_t maxs, surfaceType_t **list, int listsize, int *listlength, vec3_t dir)
{
	int        s, c;
	msurface_t *surf, **mark;

	// if this node hasn't been rendered recently, ignore it
	if (node->visframe < tr.visCount - 2)     // allow us to be a few frames behind
	{
		return;
	}

	// do the tail recursion in a loop
	while (node->contents == -1)
	{
		s = BoxOnPlaneSide(mins, maxs, node->plane);
		if (s == 1)
		{
			node = node->children[0];
		}
		else if (s == 2)
		{
			node = node->children[1];
		}
		else
		{
			R_BoxSurfaces_r(node->children[0], mins, maxs, list, listsize, listlength, dir);
			node = node->children[1];
		}
	}

	// don't mark alpha surfaces
	if (node->contents & CONTENTS_TRANSLUCENT)
	{
		return;
	}

	// add the individual surfaces
	mark = node->firstmarksurface;
	c    = node->nummarksurfaces;
	while (c--)
	{
		if (*listlength >= listsize)
		{
			break;
		}

		surf = *mark;

		// check if the surface has NOIMPACT or NOMARKS set
		if ((surf->shader->surfaceFlags & (SURF_NOIMPACT | SURF_NOMARKS))
		    || (surf->shader->contentFlags & CONTENTS_FOG))
		{
			surf->viewCount = tr.viewCount;
		}
		// extra check for surfaces to avoid list overflows
		else if (*(surf->data) == SF_FACE)
		{
			if ((( srfSurfaceFace_t * ) surf->data)->plane.type != PLANE_NON_PLANAR)
			{
				// the face plane should go through the box
				s = BoxOnPlaneSide(mins, maxs, &(( srfSurfaceFace_t * ) surf->data)->plane);
				if (s == 1 || s == 2)
				{
					surf->viewCount = tr.viewCount;
				}
				else if (DotProduct((( srfSurfaceFace_t * ) surf->data)->plane.normal, dir) < -0.5f)
				{
					// don't add faces that make sharp angles with the projection direction
					surf->viewCount = tr.viewCount;
				}
			}
		}
		else if (*( surfaceType_t * )(surf->data) != SF_GRID && *( surfaceType_t * )(surf->data) != SF_TRIANGLES)
		{
			surf->viewCount = tr.viewCount;
		}
		// check the viewCount because the surface may have
		// already been added if it spans multiple leafs
		if (surf->viewCount != tr.viewCount)
		{
			surf->viewCount   = tr.viewCount;
			list[*listlength] = (surfaceType_t *) surf->data;
			(*listlength)++;
		}
		mark++;
	}
}

/**
 * @brief R_AddMarkFragments
 * @param[in] numClipPoints
 * @param[in] clipPoints
 * @param[in] numPlanes
 * @param[in] normals
 * @param[in] dists
 * @param[in] maxPoints
 * @param[out] pointBuffer
 * @param maxFragments - unused
 * @param[in] fragmentBuffer
 * @param[in,out] returnedPoints
 * @param[in,out] returnedFragments
 * @param mins - unused
 * @param maxs - unused
 */
void R_AddMarkFragments(int numClipPoints, vec3_t clipPoints[2][MAX_VERTS_ON_POLY],
                        int numPlanes, vec3_t *normals, float *dists,
                        int maxPoints, vec3_t pointBuffer,
                        int maxFragments, markFragment_t *fragmentBuffer,
                        int *returnedPoints, int *returnedFragments,
                        vec3_t mins, vec3_t maxs)
{
	int            pingPong = 0, i;
	markFragment_t *mf;

	// chop the surface by all the bounding planes of the to be projected polygon

	for (i = 0 ; i < numPlanes ; i++)
	{

		R_ChopPolyBehindPlane(numClipPoints, clipPoints[pingPong],
		                      &numClipPoints, clipPoints[!pingPong],
		                      normals[i], dists[i], 0.5);
		pingPong ^= 1;
		if (numClipPoints == 0)
		{
			break;
		}
	}
	// completely clipped away?
	if (numClipPoints == 0)
	{
		return;
	}

	// add this fragment to the returned list
	if (numClipPoints + (*returnedPoints) > maxPoints)
	{
		return; // not enough space for this polygon
	}

	mf             = fragmentBuffer + (*returnedFragments);
	mf->firstPoint = (*returnedPoints);
	mf->numPoints  = numClipPoints;
	//Com_Memcpy( pointBuffer + (*returnedPoints) * 3, clipPoints[pingPong], numClipPoints * sizeof(vec3_t) );
	for (i = 0; i < numClipPoints; i++)
	{
		VectorCopy(clipPoints[pingPong][i], (float *)pointBuffer + 5 * (*returnedPoints + i));
	}

	(*returnedPoints) += numClipPoints;
	(*returnedFragments)++;
}

/**
 * @brief R_MarkFragments
 * @param[in] numPoints
 * @param[in] points
 * @param[in] projection
 * @param[in] maxPoints
 * @param[in] pointBuffer
 * @param[in] maxFragments
 * @param[in,out] fragmentBuffer
 * @return
 */
int R_MarkFragments(int numPoints, const vec3_t *points, const vec3_t projection,
                    int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer)
{
	int              numsurfaces, numPlanes;
	int              i, j, k, m, n;
	surfaceType_t    *surfaces[4096];
	vec3_t           mins, maxs;
	int              returnedFragments;
	int              returnedPoints;
	vec3_t           normals[MAX_VERTS_ON_POLY + 2];
	float            dists[MAX_VERTS_ON_POLY + 2];
	vec3_t           clipPoints[2][MAX_VERTS_ON_POLY];
	int              numClipPoints;
	float            *v;
	srfSurfaceFace_t *surf;
	srfGridMesh_t    *cv;
	drawVert_t       *dv;
	vec3_t           normal;
	vec3_t           projectionDir;
	vec3_t           v1, v2;
	int              *indexes;
	float            radius;
	vec3_t           center; // center of original mark
	//vec3_t            bestCenter; // center point projected onto the closest surface
	//float texCoordScale;
	//float         dot;
	int      numberPoints  = 4;        // we were only ever passing in 4, so I made this local and used the parameter for the orientation
	qboolean oldMapping = qfalse;

	//increment view count for double check prevention
	tr.viewCount++;

	// negative maxFragments means we want original mapping
	if (maxFragments < 0)
	{
		maxFragments = -maxFragments;
		oldMapping   = qtrue;
	}

	VectorClear(center);
	for (i = 0 ; i < numberPoints ; i++)
	{
		VectorAdd(points[i], center, center);
	}
	VectorScale(center, 1.0f / numberPoints, center);

	radius   = VectorNormalize2(projection, projectionDir) / 2.0f;
	bestdist = 0;
	VectorNegate(projectionDir, bestnormal);
	// find all the brushes that are to be considered
	ClearBounds(mins, maxs);
	for (i = 0 ; i < numberPoints ; i++)
	{
		vec3_t temp;

		AddPointToBounds(points[i], mins, maxs);
		VectorMA(points[i], 1 * (1 + oldMapping * radius * 4), projection, temp);
		AddPointToBounds(temp, mins, maxs);
		// make sure we get all the leafs (also the one(s) in front of the hit surface)
		VectorMA(points[i], -20 * (1.0f + (float)oldMapping * (radius / 20.0f) * 4), projectionDir, temp);
		AddPointToBounds(temp, mins, maxs);
	}

	// numPoints has static value of 4 atm
	//if (numPoints > MAX_VERTS_ON_POLY)
	//{
	//	numPoints = MAX_VERTS_ON_POLY;
	//}

	// create the bounding planes for the to be projected polygon
	for (i = 0 ; i < numberPoints ; i++)
	{
		VectorSubtract(points[(i + 1) % numberPoints], points[i], v1);
		VectorAdd(points[i], projection, v2);
		VectorSubtract(points[i], v2, v2);
		CrossProduct(v1, v2, normals[i]);
		VectorNormalize(normals[i]);
		dists[i] = DotProduct(normals[i], points[i]);
	}
	// add near and far clipping planes for projection
	VectorCopy(projectionDir, normals[numberPoints]);
	dists[numberPoints] = DotProduct(normals[numberPoints], points[0]) - radius * (1 + oldMapping * 10);
	VectorCopy(projectionDir, normals[numberPoints + 1]);
	VectorInverse(normals[numberPoints + 1]);
	dists[numberPoints + 1] = DotProduct(normals[numberPoints + 1], points[0]) - radius * (1 + oldMapping * 10);
	numPlanes            = numberPoints + 2;

	numsurfaces = 0;
	R_BoxSurfaces_r(tr.world->nodes, mins, maxs, surfaces, 4096, &numsurfaces, projectionDir);

	//texCoordScale = 0.5 * 1.0 / radius;

	returnedPoints    = 0;
	returnedFragments = 0;

	// find the closest surface to center the decal there, and wrap around other surfaces
	if (!oldMapping)
	{
		VectorNegate(bestnormal, bestnormal);
	}

	for (i = 0 ; i < numsurfaces ; i++)
	{
		if (*surfaces[i] == SF_GRID)
		{
			cv = (srfGridMesh_t *) surfaces[i];
			for (m = 0 ; m < cv->height - 1 ; m++)
			{
				for (n = 0 ; n < cv->width - 1 ; n++)
				{
					// We triangulate the grid and chop all triangles within
					// the bounding planes of the to be projected polygon.
					// LOD is not taken into account, not such a big deal though.
					//
					// It's probably much nicer to chop the grid itself and deal
					// with this grid as a normal SF_GRID surface so LOD will
					// be applied. However the LOD of that chopped grid must
					// be synced with the LOD of the original curve.
					// One way to do this; the chopped grid shares vertices with
					// the original curve. When LOD is applied to the original
					// curve the unused vertices are flagged. Now the chopped curve
					// should skip the flagged vertices. This still leaves the
					// problems with the vertices at the chopped grid edges.
					//
					// To avoid issues when LOD applied to "hollow curves" (like
					// the ones around many jump pads) we now just add a 2 unit
					// offset to the triangle vertices.
					// The offset is added in the vertex normal vector direction
					// so all triangles will still fit together.
					// The 2 unit offset should avoid pretty much all LOD problems.

					numClipPoints = 3;

					dv = cv->verts + m * cv->width + n;

					VectorCopy(dv[0].xyz, clipPoints[0][0]);
					VectorMA(clipPoints[0][0], MARKER_OFFSET, dv[0].normal, clipPoints[0][0]);
					VectorCopy(dv[cv->width].xyz, clipPoints[0][1]);
					VectorMA(clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal, clipPoints[0][1]);
					VectorCopy(dv[1].xyz, clipPoints[0][2]);
					VectorMA(clipPoints[0][2], MARKER_OFFSET, dv[1].normal, clipPoints[0][2]);
					// check the normal of this triangle
					VectorSubtract(clipPoints[0][0], clipPoints[0][1], v1);
					VectorSubtract(clipPoints[0][2], clipPoints[0][1], v2);
					CrossProduct(v1, v2, normal);
					VectorNormalize(normal);
					if (DotProduct(normal, projectionDir) < -0.1f)
					{
						// add the fragments of this triangle
						R_AddMarkFragments(numClipPoints, clipPoints,
						                   numPlanes, normals, dists,
						                   maxPoints, pointBuffer,
						                   maxFragments, fragmentBuffer,
						                   &returnedPoints, &returnedFragments, mins, maxs);

						if (returnedFragments == maxFragments)
						{
							return returnedFragments;   // not enough space for more fragments
						}
					}

					VectorCopy(dv[1].xyz, clipPoints[0][0]);
					VectorMA(clipPoints[0][0], MARKER_OFFSET, dv[1].normal, clipPoints[0][0]);
					VectorCopy(dv[cv->width].xyz, clipPoints[0][1]);
					VectorMA(clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal, clipPoints[0][1]);
					VectorCopy(dv[cv->width + 1].xyz, clipPoints[0][2]);
					VectorMA(clipPoints[0][2], MARKER_OFFSET, dv[cv->width + 1].normal, clipPoints[0][2]);
					// check the normal of this triangle
					VectorSubtract(clipPoints[0][0], clipPoints[0][1], v1);
					VectorSubtract(clipPoints[0][2], clipPoints[0][1], v2);
					CrossProduct(v1, v2, normal);
					VectorNormalize(normal);
					if (DotProduct(normal, projectionDir) < -0.05f)
					{
						// add the fragments of this triangle
						R_AddMarkFragments(numClipPoints, clipPoints,
						                   numPlanes, normals, dists,
						                   maxPoints, pointBuffer,
						                   maxFragments, fragmentBuffer,
						                   &returnedPoints, &returnedFragments, mins, maxs);

						if (returnedFragments == maxFragments)
						{
							return returnedFragments;   // not enough space for more fragments
						}
					}
				}
			}
		}
		else if (*surfaces[i] == SF_FACE)
		{
			vec3_t axis[3];
			vec3_t originalPoints[4];
			vec3_t newCenter;
			// duplicated so we don't mess with the original clips for the curved surfaces
			vec3_t lnormals[MAX_VERTS_ON_POLY + 2];
			float  ldists[MAX_VERTS_ON_POLY + 2];
			vec3_t lmins;
			vec3_t surfnormal;

			surf = ( srfSurfaceFace_t * ) surfaces[i];

			if (surf->plane.type == PLANE_NON_PLANAR)
			{
				VectorCopy(bestnormal, surfnormal);
			}
			else
			{
				VectorCopy(surf->plane.normal, surfnormal);
			}

			if (!oldMapping)
			{
				vec3_t lmaxs;
				float  dot;
				float  texCoordScale;
				float  epsilon = 0.5f;
				int    oldNumPoints;

				// create a new clip box such that this decal surface is mapped onto
				// the current surface without distortion. To find the center of the new clip box,
				// we project the center of the original impact center out along the projection vector,
				// onto the current surface

				if (surf->plane.type == PLANE_NON_PLANAR)
				{
					VectorCopy(center, newCenter);
				}
				else
				{
					// find the center of the new decal
					dot  = DotProduct(center, surfnormal);
					dot -= surf->plane.dist;
					// check the normal of this face
					if (dot < -epsilon && DotProduct(surfnormal, projectionDir) >= 0.01f)
					{
						continue;
					}
					else if (Q_fabs(dot) > radius)
					{
						continue;
					}
					// if the impact point is behind the surface, subtract the projection, otherwise add it
					VectorMA(center, -dot, bestnormal, newCenter);
				}

				// recalc dot from the offset position
				dot  = DotProduct(newCenter, surfnormal);
				dot -= surf->plane.dist;
				VectorMA(newCenter, -dot, surfnormal, newCenter);

				VectorMA(newCenter, MARKER_OFFSET, surfnormal, newCenter);

				// create the texture axis
				VectorNormalize2(surfnormal, axis[0]);
				PerpendicularVector(axis[1], axis[0]);
				RotatePointAroundVector(axis[2], axis[0], axis[1], (float)numPoints);
				CrossProduct(axis[0], axis[2], axis[1]);

				texCoordScale = 0.5f * 1.0f / radius;

				// create the full polygon
				for (j = 0 ; j < 3 ; j++)
				{
					originalPoints[0][j] = newCenter[j] - radius * axis[1][j] - radius * axis[2][j];
					originalPoints[1][j] = newCenter[j] + radius * axis[1][j] - radius * axis[2][j];
					originalPoints[2][j] = newCenter[j] + radius * axis[1][j] + radius * axis[2][j];
					originalPoints[3][j] = newCenter[j] - radius * axis[1][j] + radius * axis[2][j];
				}

				ClearBounds(lmins, lmaxs);

				// create the bounding planes for the to be projected polygon
				for (j = 0 ; j < 4 ; j++)
				{
					AddPointToBounds(originalPoints[j], lmins, lmaxs);

					VectorSubtract(originalPoints[(j + 1) % numberPoints], originalPoints[j], v1);
					VectorSubtract(originalPoints[j], surfnormal, v2);
					VectorSubtract(originalPoints[j], v2, v2);
					CrossProduct(v1, v2, lnormals[j]);
					VectorNormalize(lnormals[j]);
					ldists[j] = DotProduct(lnormals[j], originalPoints[j]);
				}
				numPlanes = numberPoints;


				indexes = ( int * )((byte *)surf + surf->ofsIndices);
				for (k = 0 ; k < surf->numIndices ; k += 3)
				{
					for (j = 0 ; j < 3 ; j++)
					{
						v = surf->points[0] + VERTEXSIZE * indexes[k + j];
						VectorMA(v, MARKER_OFFSET, surfnormal, clipPoints[0][j]);
					}

					oldNumPoints = returnedPoints;

					// add the fragments of this face
					R_AddMarkFragments(3, clipPoints,
					                   numPlanes, lnormals, ldists,
					                   maxPoints, pointBuffer,
					                   maxFragments, fragmentBuffer,
					                   &returnedPoints, &returnedFragments, lmins, lmaxs);

					if (oldNumPoints != returnedPoints)
					{
						vec3_t delta;

						// flag this surface as already having computed ST's
						fragmentBuffer[returnedFragments - 1].numPoints *= -1;

						// calculate ST's
						for (j = 0 ; j < (returnedPoints - oldNumPoints) ; j++)
						{
							VectorSubtract((float *)pointBuffer + 5 * (oldNumPoints + j), newCenter, delta);
							*((float *)pointBuffer + 5 * (oldNumPoints + j) + 3) = 0.5f + DotProduct(delta, axis[1]) * texCoordScale;
							*((float *)pointBuffer + 5 * (oldNumPoints + j) + 4) = 0.5f + DotProduct(delta, axis[2]) * texCoordScale;
						}
					}

					if (returnedFragments == maxFragments)
					{
						return returnedFragments;   // not enough space for more fragments
					}
				}

			}
			else  // old mapping
			{
				indexes = ( int * )((byte *)surf + surf->ofsIndices);
				for (k = 0 ; k < surf->numIndices ; k += 3)
				{
					for (j = 0 ; j < 3 ; j++)
					{
						v = &surf->points[0][0] + VERTEXSIZE * indexes[k + j];
						VectorMA(v, MARKER_OFFSET, surf->plane.normal, clipPoints[0][j]);
					}
					// add the fragments of this face
					R_AddMarkFragments(3, clipPoints,
					                   numPlanes, normals, dists,
					                   maxPoints, pointBuffer,
					                   maxFragments, fragmentBuffer,
					                   &returnedPoints, &returnedFragments, mins, maxs);
					if (returnedFragments == maxFragments)
					{
						return returnedFragments;   // not enough space for more fragments
					}
				}
			}

			continue;
		}
		// projection on models (mainly for terrain though)
		else if (*surfaces[i] == SF_TRIANGLES)
		{
			// duplicated so we don't mess with the original clips for the curved surfaces
			vec3_t         lnormals[MAX_VERTS_ON_POLY + 2];
			float          ldists[MAX_VERTS_ON_POLY + 2];
			srfTriangles_t *cts;
			cts = ( srfTriangles_t * ) surfaces[i];

			if (!oldMapping)
			{
				for (k = 0 ; k < numberPoints ; k++)
				{
					VectorNegate(normals[k], lnormals[k]);
					ldists[k] = -dists[k];
				}
				VectorNegate(normals[numberPoints], lnormals[numberPoints]);
				ldists[numberPoints] = dists[numberPoints + 1];
				VectorNegate(normals[numberPoints + 1], lnormals[numberPoints + 1]);
				ldists[numberPoints + 1] = dists[numberPoints];

				indexes = cts->indexes;
				for (k = 0 ; k < cts->numIndexes ; k += 3)
				{
					for (j = 0 ; j < 3 ; j++)
					{
						v = cts->verts[indexes[k + j]].xyz;
						VectorMA(v, MARKER_OFFSET, cts->verts[indexes[k + j]].normal, clipPoints[0][j]);
					}
					// add the fragments of this face
					R_AddMarkFragments(3, clipPoints,
					                   numPlanes, lnormals, ldists,
					                   maxPoints, pointBuffer,
					                   maxFragments, fragmentBuffer,
					                   &returnedPoints, &returnedFragments, mins, maxs);

					if (returnedFragments == maxFragments)
					{
						return returnedFragments;   // not enough space for more fragments
					}
				}
			}
			else
			{
				indexes = cts->indexes;
				for (k = 0 ; k < cts->numIndexes ; k += 3)
				{
					for (j = 0 ; j < 3 ; j++)
					{
						v = cts->verts[indexes[k + j]].xyz;
						VectorMA(v, MARKER_OFFSET, cts->verts[indexes[k + j]].normal, clipPoints[0][j]);
					}
					// add the fragments of this face
					R_AddMarkFragments(3, clipPoints,
					                   numPlanes, normals, dists,
					                   maxPoints, pointBuffer,
					                   maxFragments, fragmentBuffer,
					                   &returnedPoints, &returnedFragments, mins, maxs);

					if (returnedFragments == maxFragments)
					{
						return returnedFragments;   // not enough space for more fragments
					}
				}
			}

			continue;
		}
		else
		{
			// ignore all other world surfaces
			// might be cool to also project polygons on a triangle soup
			// however this will probably create huge amounts of extra polys
			// even more than the projection onto curves
			continue;
		}
	}
	return returnedFragments;
}
