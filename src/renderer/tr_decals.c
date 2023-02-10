/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2023 ET:Legacy team <mail@etlegacy.com>
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
 * @file renderer/tr_decals.c
 * @brief Handles projection of decals (nee marks) onto brush model surfaces
 */

#include "tr_local.h"

extern int r_numDecalProjectors;
extern int r_firstSceneDecal;

typedef struct decalVert_s
{
	vec3_t xyz;
	float st[2];
}
decalVert_t;

/**
 * @brief Generates a texture projection matrix for a triangle
 * @param[out] texMat
 * @param[in] projection
 * @param[in] a
 * @param[in] b
 * @param[in] c
 * @return qfalse if a texture matrix cannot be created
 */
static qboolean MakeTextureMatrix(vec4_t texMat[2], vec4_t projection, decalVert_t *a, decalVert_t *b, decalVert_t *c)
{
	int    i, j;
	float  bb, s, t, d;
	vec3_t pa, pb, pc;
	vec3_t bary, origin, xyz;
	vec3_t vecs[3], axis[3], lengths;

	// project triangle onto plane of projection
	d = DotProduct(a->xyz, projection) - projection[3];
	VectorMA(a->xyz, -d, projection, pa);
	d = DotProduct(b->xyz, projection) - projection[3];
	VectorMA(b->xyz, -d, projection, pb);
	d = DotProduct(c->xyz, projection) - projection[3];
	VectorMA(c->xyz, -d, projection, pc);

	// calculate barycentric basis for the triangle
	bb = (b->st[0] - a->st[0]) * (c->st[1] - a->st[1]) - (c->st[0] - a->st[0]) * (b->st[1] - a->st[1]);
	if (Q_fabs(bb) < 0.00000001f)
	{
		return qfalse;
	}

	// calculate texture origin
	s       = 0.0;
	t       = 0.0;
	bary[0] = ((b->st[0] - s) * (c->st[1] - t) - (c->st[0] - s) * (b->st[1] - t)) / bb;
	bary[1] = ((c->st[0] - s) * (a->st[1] - t) - (a->st[0] - s) * (c->st[1] - t)) / bb;
	bary[2] = ((a->st[0] - s) * (b->st[1] - t) - (b->st[0] - s) * (a->st[1] - t)) / bb;

	origin[0] = bary[0] * pa[0] + bary[1] * pb[0] + bary[2] * pc[0];
	origin[1] = bary[0] * pa[1] + bary[1] * pb[1] + bary[2] * pc[1];
	origin[2] = bary[0] * pa[2] + bary[1] * pb[2] + bary[2] * pc[2];

	// calculate s vector
	s       = 1.0;
	t       = 0.0;
	bary[0] = ((b->st[0] - s) * (c->st[1] - t) - (c->st[0] - s) * (b->st[1] - t)) / bb;
	bary[1] = ((c->st[0] - s) * (a->st[1] - t) - (a->st[0] - s) * (c->st[1] - t)) / bb;
	bary[2] = ((a->st[0] - s) * (b->st[1] - t) - (b->st[0] - s) * (a->st[1] - t)) / bb;

	xyz[0] = bary[0] * pa[0] + bary[1] * pb[0] + bary[2] * pc[0];
	xyz[1] = bary[0] * pa[1] + bary[1] * pb[1] + bary[2] * pc[1];
	xyz[2] = bary[0] * pa[2] + bary[1] * pb[2] + bary[2] * pc[2];

	VectorSubtract(xyz, origin, vecs[0]);

	// calculate t vector
	s       = 0.0;
	t       = 1.0;
	bary[0] = ((b->st[0] - s) * (c->st[1] - t) - (c->st[0] - s) * (b->st[1] - t)) / bb;
	bary[1] = ((c->st[0] - s) * (a->st[1] - t) - (a->st[0] - s) * (c->st[1] - t)) / bb;
	bary[2] = ((a->st[0] - s) * (b->st[1] - t) - (b->st[0] - s) * (a->st[1] - t)) / bb;

	xyz[0] = bary[0] * pa[0] + bary[1] * pb[0] + bary[2] * pc[0];
	xyz[1] = bary[0] * pa[1] + bary[1] * pb[1] + bary[2] * pc[1];
	xyz[2] = bary[0] * pa[2] + bary[1] * pb[2] + bary[2] * pc[2];

	VectorSubtract(xyz, origin, vecs[1]);

	// calcuate r vector
	VectorScale(projection, -1.0f, vecs[2]);

	// calculate transform axis
	for (i = 0; i < 3; i++)
		lengths[i] = VectorNormalize2(vecs[i], axis[i]);

	for (i = 0; i < 2; i++)
		for (j = 0; j < 3; j++)
			texMat[i][j] = lengths[i] > 0.0f ? (axis[i][j] / lengths[i]) : 0.0f;
	texMat[0][3] = a->st[0] - DotProduct(pa, texMat[0]);
	texMat[1][3] = a->st[1] - DotProduct(pa, texMat[1]);

	// disco
	return qtrue;
}

/**
 * @brief Creates a new decal projector from a triangle.
 *
 * Projected polygons should be 3 or 4 points.
 *
 * If a single point is passed in (numPoints == 1) then the decal will be omnidirectional
 * omnidirectional decals use points[ 0 ] as center and projection[ 3 ] as radius
 * pass in lifeTime < 0 for a temporary mark.
 *
 * @param[in] hShader
 * @param[in] numPoints
 * @param[in] points
 * @param[in] projection
 * @param[in] color
 * @param[in] lifeTime
 * @param[in] fadeTime
 */
void RE_ProjectDecal(qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime,
                     int fadeTime)
{
	static int       totalProjectors = 0;
	vec3_t           xyz;
	decalVert_t      dv[4];
	int              i;
	decalProjector_t *dp, temp;

	if (r_numDecalProjectors >= MAX_DECAL_PROJECTORS)
	{
		Ren_Print("WARNING: RE_ProjectDecal() Max decal projectors reached (%d)\n", MAX_DECAL_PROJECTORS);
		return;
	}

	// dummy check
	if (numPoints != 1 && numPoints != 3 && numPoints != 4)
	{
		Ren_Print("WARNING: RE_ProjectDecal() Invalid number of decal points (%d)\n", numPoints);
		return;
	}

	// early outs
	if (lifeTime == 0)
	{
		Ren_Developer("WARNING: RE_ProjectDecal() lifeTime == 0\n"); // modders should have a look at this - vanilla does these calls
		return;
	}
	if (projection[3] <= 0.0f)
	{
		Ren_Print("WARNING: RE_ProjectDecal() projection[3] <= 0.0f\n");
		return;
	}

	// set times properly
	if (lifeTime < 0 || fadeTime < 0)
	{
		lifeTime = 0;
		fadeTime = 0;
	}

	// basic setup
	temp.shader        = R_GetShaderByHandle(hShader);
	temp.color[0]      = (byte)(color[0] * 255);
	temp.color[1]      = (byte)(color[1] * 255);
	temp.color[2]      = (byte)(color[2] * 255);
	temp.color[3]      = (byte)(color[3] * 255);
	temp.numPlanes     = numPoints + 2;
	temp.fadeStartTime = tr.refdef.time + lifeTime - fadeTime; // FIXME: stale refdef time
	temp.fadeEndTime   = temp.fadeStartTime + fadeTime;
	temp.projectorNum  = 0;

	// set up decal texcoords (FIXME: support arbitrary projector st coordinates in trapcall)
	dv[0].st[0] = 0.0f;
	dv[0].st[1] = 0.0f;
	dv[1].st[0] = 0.0f;
	dv[1].st[1] = 1.0f;
	dv[2].st[0] = 1.0f;
	dv[2].st[1] = 1.0f;
	dv[3].st[0] = 1.0f;
	dv[3].st[1] = 0.0f;

	// omnidirectional?
	if (numPoints == 1)
	{
		float radius;
		float iDist;

		// set up omnidirectional
		numPoints            = 4;
		temp.numPlanes       = 6;
		temp.omnidirectional = qtrue;
		radius               = projection[3];

		Vector4Set(projection, 0.0f, 0.0f, -1.0f, radius * 2.0f);
		iDist = 1.0f / (radius * 2.0f);

		// set corner
		VectorSet(xyz, points[0][0] - radius, points[0][1] - radius, points[0][2] + radius);

		// make x axis texture matrix (yz)
		VectorSet(temp.texMat[0][0], 0.0f, iDist, 0.0f);
		temp.texMat[0][0][3] = -DotProduct(temp.texMat[0][0], xyz);
		VectorSet(temp.texMat[0][1], 0.0f, 0.0f, iDist);
		temp.texMat[0][1][3] = -DotProduct(temp.texMat[0][1], xyz);

		// make y axis texture matrix (xz)
		VectorSet(temp.texMat[1][0], iDist, 0.0f, 0.0f);
		temp.texMat[1][0][3] = -DotProduct(temp.texMat[1][0], xyz);
		VectorSet(temp.texMat[1][1], 0.0f, 0.0f, iDist);
		temp.texMat[1][1][3] = -DotProduct(temp.texMat[1][1], xyz);

		// make z axis texture matrix (xy)
		VectorSet(temp.texMat[2][0], iDist, 0.0f, 0.0f);
		temp.texMat[2][0][3] = -DotProduct(temp.texMat[2][0], xyz);
		VectorSet(temp.texMat[2][1], 0.0f, iDist, 0.0f);
		temp.texMat[2][1][3] = -DotProduct(temp.texMat[2][1], xyz);

		// setup decal points
		VectorSet(dv[0].xyz, points[0][0] - radius, points[0][1] - radius, points[0][2] + radius);
		VectorSet(dv[1].xyz, points[0][0] - radius, points[0][1] + radius, points[0][2] + radius);
		VectorSet(dv[2].xyz, points[0][0] + radius, points[0][1] + radius, points[0][2] + radius);
		VectorSet(dv[3].xyz, points[0][0] + radius, points[0][1] - radius, points[0][2] + radius);
	}
	else
	{
		// set up unidirectional
		temp.omnidirectional = qfalse;

		// set up decal points
		VectorCopy(points[0], dv[0].xyz);
		VectorCopy(points[1], dv[1].xyz);
		VectorCopy(points[2], dv[2].xyz);
		VectorCopy(points[3], dv[3].xyz);

		// make texture matrix
		if (!MakeTextureMatrix(temp.texMat[0], projection, &dv[0], &dv[1], &dv[2]))
		{
			Ren_Print("WARNING: RE_ProjectDecal() MakeTextureMatrix returns NULL\n");
			return;
		}
	}

	// bound the projector
	ClearBounds(temp.mins, temp.maxs);
	for (i = 0; i < numPoints; i++)
	{
		AddPointToBounds(dv[i].xyz, temp.mins, temp.maxs);
		VectorMA(dv[i].xyz, projection[3], projection, xyz);
		AddPointToBounds(xyz, temp.mins, temp.maxs);
	}

	// make bounding sphere
	VectorAdd(temp.mins, temp.maxs, temp.center);
	VectorScale(temp.center, 0.5f, temp.center);
	VectorSubtract(temp.maxs, temp.center, xyz);
	temp.radius  = VectorLength(xyz);
	temp.radius2 = temp.radius * temp.radius;

	// make the front plane
	if (!PlaneFromPoints(temp.planes[0], dv[0].xyz, dv[1].xyz, dv[2].xyz))
	{
		Ren_Developer("WARNING: RE_ProjectDecal() PlaneFromPoints is NULL\n"); // occurs on UJE_fueldump
		return;
	}

	// make the back plane
	VectorSubtract(vec3_origin, temp.planes[0], temp.planes[1]);
	VectorMA(dv[0].xyz, projection[3], projection, xyz);
	temp.planes[1][3] = DotProduct(xyz, temp.planes[1]);

	// make the side planes
	for (i = 0; i < numPoints; i++)
	{
		VectorMA(dv[i].xyz, projection[3], projection, xyz);
		if (!PlaneFromPoints(temp.planes[i + 2], dv[(i + 1) % numPoints].xyz, dv[i].xyz, xyz))
		{
			Ren_Developer("WARNING: RE_ProjectDecal() a side plane is NULL\n"); // occurs on map venice
			return;
		}
	}

	// create a new projector
	dp = &backEndData->decalProjectors[r_numDecalProjectors];
	Com_Memcpy(dp, &temp, sizeof(*dp));
	dp->projectorNum = totalProjectors++;

	// we have a winner
	r_numDecalProjectors++;
}

/**
 * @brief Adds a simple shadow projector to the scene
 * @param[in] ent
 */
void R_AddModelShadow(const refEntity_t *ent)
{
	model_t *m;
	vec4_t  projection;
	vec3_t  pushedOrigin, points[4];

	// shadows?
	if (!r_drawEntities->integer || r_shadows->integer != 1 || (ent->renderfx & RF_NOSHADOW))
	{
		return;
	}

	// get model
	m = R_GetModelByHandle(ent->hModel);
	if (m == NULL || m->shadowShader == 0)
	{
		return;
	}

	// calculate projection
	VectorSubtract(vec3_origin, ent->axis[2], projection);
	VectorSet(projection, 0, 0, -1.0f);
	projection[3] = m->shadowParms[4];

	// push origin
	VectorMA(ent->origin, m->shadowParms[5], projection, pushedOrigin);

	// make shadow polygon
	VectorMA(pushedOrigin, m->shadowParms[0], ent->axis[1], points[0]);
	VectorMA(points[0], m->shadowParms[1], ent->axis[0], points[0]);
	VectorMA(points[0], m->shadowParms[2], ent->axis[1], points[1]);
	VectorMA(points[1], m->shadowParms[3], ent->axis[0], points[2]);
	VectorMA(points[0], m->shadowParms[3], ent->axis[0], points[3]);

	// add the decal
	RE_ProjectDecal(m->shadowShader, 4, points, projection, colorWhite, -1, -1);
}

/**
 * @brief Clears decals from the world and entities
 */
void RE_ClearDecals(void)
{
	int i, j;

	// dummy check
	if (tr.world == NULL || tr.world->numBModels <= 0)
	{
		return;
	}

	// clear world decals
	for (j = 0; j < MAX_WORLD_DECALS; j++)
	{
		tr.world->bmodels[0].decals[j].shader = NULL;
	}

	// clear entity decals
	for (i = 0; i < tr.world->numBModels; i++)
	{
		for (j = 0; j < MAX_ENTITY_DECALS; j++)
		{
			tr.world->bmodels[i].decals[j].shader = NULL;
		}
	}
}

/**
 * @brief Transforms a decal projector
 *
 * @param[in] in
 * @param[in] axis
 * @param[in] origin
 * @param[out] out
 *
 * @note Non-normalized axes will screw up the plane transform
 */
void R_TransformDecalProjector(decalProjector_t *in, vec3_t axis[3], vec3_t origin, decalProjector_t *out)
{
	int    i, m;
	vec3_t center;

	// copy misc stuff
	out->shader          = in->shader;
	*((int *)out->color) = *((int *)in->color);
	out->fadeStartTime   = in->fadeStartTime;
	out->fadeEndTime     = in->fadeEndTime;
	out->omnidirectional = in->omnidirectional;
	out->numPlanes       = in->numPlanes;
	out->projectorNum    = in->projectorNum;

	// translate bounding box and sphere (note: rotated projector bounding box will be invalid!)
	VectorSubtract(in->mins, origin, out->mins);
	VectorSubtract(in->maxs, origin, out->maxs);
	VectorSubtract(in->center, origin, center);
	out->center[0] = DotProduct(center, axis[0]);
	out->center[1] = DotProduct(center, axis[1]);
	out->center[2] = DotProduct(center, axis[2]);
	out->radius    = in->radius;
	out->radius2   = in->radius2;

	// translate planes
	for (i = 0; i < in->numPlanes; i++)
	{
		// transform by transposed inner 3x3 matrix
		out->planes[i][0] = DotProduct(in->planes[i], axis[0]);
		out->planes[i][1] = DotProduct(in->planes[i], axis[1]);
		out->planes[i][2] = DotProduct(in->planes[i], axis[2]);
		out->planes[i][3] = in->planes[i][3] - DotProduct(in->planes[i], origin);
	}

	//ri.Printf( PRINT_ALL, "plane 0: %f %f %f in dist: %f out dist: %f z: %f\n",
	//  out->planes[ 0 ][ 0 ], out->planes[ 0 ][ 1 ], out->planes[ 0 ][ 2 ],
	//  in->planes[ 0 ][ 3 ], out->planes[ 0 ][ 3 ], origin[ 2 ] );

	// translate texture matrices
	for (m = 0; m < 3; m++)
	{
		for (i = 0; i < 2; i++)
		{
			out->texMat[m][i][0] = DotProduct(in->texMat[m][i], axis[0]);
			out->texMat[m][i][1] = DotProduct(in->texMat[m][i], axis[1]);
			out->texMat[m][i][2] = DotProduct(in->texMat[m][i], axis[2]);
			out->texMat[m][i][3] = in->texMat[m][i][3] + DotProduct(in->texMat[m][i], origin);
		}
	}
}

/**
 * @brief R_TestDecalBoundingBox
 * @param[in] dp
 * @param[in] mins
 * @param[in] maxs
 * @return qtrue if the decal projector intersects the bounding box
 */
qboolean R_TestDecalBoundingBox(decalProjector_t *dp, vec3_t mins, vec3_t maxs)
{
	if (mins[0] >= (dp->center[0] + dp->radius) || maxs[0] <= (dp->center[0] - dp->radius) ||
	    mins[1] >= (dp->center[1] + dp->radius) || maxs[1] <= (dp->center[1] - dp->radius) ||
	    mins[2] >= (dp->center[2] + dp->radius) || maxs[2] <= (dp->center[2] - dp->radius))
	{
		return qfalse;
	}
	return qtrue;
}

/**
 * @brief R_TestDecalBoundingSphere
 * @param[in] dp
 * @param[in] center
 * @param[in] radius2
 * @return qtrue if the decal projector intersects the bounding sphere
 */
qboolean R_TestDecalBoundingSphere(decalProjector_t *dp, vec3_t center, float radius2)
{
	vec3_t delta;
	float  distance2;

	VectorSubtract(center, dp->center, delta);
	distance2 = DotProduct(delta, delta);
	if (distance2 >= (radius2 + dp->radius2))
	{
		return qfalse;
	}
	return qtrue;
}

#define SIDE_FRONT  0
#define SIDE_BACK   1
#define SIDE_ON     2

/**
 * @brief Clips a winding to the fragment behind the plane
 * @param[in] numInPoints
 * @param[in] inPoints
 * @param[out] numOutPoints
 * @param[out] outPoints
 * @param[in] plane
 * @param[in] epsilon
 */
static void ChopWindingBehindPlane(int numInPoints, vec3_t inPoints[MAX_DECAL_VERTS],
                                   int *numOutPoints, vec3_t outPoints[MAX_DECAL_VERTS], vec4_t plane, vec_t epsilon)
{
	int   i, j;
	float dot;
	float *p1, *p2, *clip;
	float d;
	float dists[MAX_DECAL_VERTS + 4];
	int   sides[MAX_DECAL_VERTS + 4];
	int   counts[3];

	// set initial count
	*numOutPoints = 0;

	// don't clip if it might overflow
	if (numInPoints >= MAX_DECAL_VERTS - 1)
	{
		return;
	}

	// determine sides for each point
	counts[SIDE_FRONT] = 0;
	counts[SIDE_BACK]  = 0;
	counts[SIDE_ON]    = 0;
	for (i = 0; i < numInPoints; i++)
	{
		dists[i] = DotProduct(inPoints[i], plane) - plane[3];
		if (dists[i] > epsilon)
		{
			sides[i] = SIDE_FRONT;
		}
		else if (dists[i] < -epsilon)
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

	// all points on front
	if (counts[SIDE_BACK] == 0)
	{
		return;
	}

	// all points on back
	if (counts[SIDE_FRONT] == 0)
	{
		*numOutPoints = numInPoints;
		Com_Memcpy(outPoints, inPoints, numInPoints * sizeof(vec3_t));
		return;
	}

	// clip the winding
	for (i = 0; i < numInPoints; i++)
	{
		p1   = inPoints[i];
		clip = outPoints[*numOutPoints];

		if (sides[i] == SIDE_ON || sides[i] == SIDE_BACK)
		{
			VectorCopy(p1, clip);
			(*numOutPoints)++;
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
		clip = outPoints[*numOutPoints];
		for (j = 0; j < 3; j++)
			clip[j] = p1[j] + dot * (p2[j] - p1[j]);

		(*numOutPoints)++;
	}
}

/**
 * @brief Projects decal onto a polygon
 * @param[in] dp
 * @param[in] numPoints
 * @param[in] points
 * @param[in] surf
 * @param[in] bmodel
 */
static void ProjectDecalOntoWinding(decalProjector_t *dp, int numPoints, vec3_t points[2][MAX_DECAL_VERTS], msurface_t *surf,
                                    bmodel_t *bmodel)
{
	int        i, pingPong, count, axis;
	float      pd, d, d2, alpha = 1.f;
	vec4_t     plane;
	decal_t    *decal, *oldest;
	polyVert_t *vert;

	// make a plane from the winding
	if (!PlaneFromPoints(plane, points[0][0], points[0][1], points[0][2]))
	{
		return;
	}

	// omnidirectional projectors need plane type
	if (dp->omnidirectional)
	{
		vec3_t absNormal;

		// compiler warnings be gone
		pd = 1.0f;

		// fade by distance from plane
		d     = DotProduct(dp->center, plane) - plane[3];
		alpha = 1.0f - (Q_fabs(d) / dp->radius);
		if (alpha < 0.0f)
		{
			return;
		}
		if (alpha > 1.0f)
		{
			alpha = 1.0f;
		}

		// set projection axis
		absNormal[0] = Q_fabs(plane[0]);
		absNormal[1] = Q_fabs(plane[1]);
		absNormal[2] = Q_fabs(plane[2]);
		if (absNormal[2] >= absNormal[0] && absNormal[2] >= absNormal[1])
		{
			axis = 2;
		}
		else if (absNormal[0] >= absNormal[1] && absNormal[0] >= absNormal[2])
		{
			axis = 0;
		}
		else
		{
			axis = 1;
		}
	}
	else
	{
		// backface check
		pd = DotProduct(dp->planes[0], plane);
		if (pd < -0.0001f)
		{
			return;
		}

		// directional decals use first texture matrix
		axis = 0;
	}

	// chop the winding by all the projector planes
	pingPong = 0;
	for (i = 0; i < dp->numPlanes; i++)
	{
		ChopWindingBehindPlane(numPoints, points[pingPong], &numPoints, points[!pingPong], dp->planes[i], 0.0f);
		pingPong ^= 1;
		if (numPoints < 3)
		{
			return;
		}
		if (numPoints == MAX_DECAL_VERTS)
		{
			break;
		}
	}

	// find first free decal (fixme: optimize this)
	count  = (bmodel == tr.world->bmodels ? MAX_WORLD_DECALS : MAX_ENTITY_DECALS);
	oldest = &bmodel->decals[0];
	decal  = bmodel->decals;
	for (i = 0; i < count; i++, decal++)
	{
		// try to find an empty decal slot
		if (decal->shader == NULL && decal->frameAdded != tr.frameCount)
		{
			break;
		}

		// find oldest decal
		if (decal->fadeEndTime < oldest->fadeEndTime)
		{
			oldest = decal;
		}
	}

	// guess we have to use the oldest decal
	if (i >= count)
	{
		decal = oldest;
	}

	// r_speeds useful info
	tr.pc.c_decalSurfacesCreated++;

	// set it up (fixme: get the shader before this happens)
	decal->parent        = surf;
	decal->shader        = dp->shader;
	decal->fadeStartTime = dp->fadeStartTime;
	decal->fadeEndTime   = dp->fadeEndTime;
	decal->fogIndex      = surf->fogIndex;
	decal->projectorNum  = dp->projectorNum;
	decal->frameAdded    = tr.frameCount;

	// add points
	decal->numVerts = numPoints;
	vert            = decal->verts;
	for (i = 0; i < numPoints; i++, vert++)
	{
		// set xyz
		VectorCopy(points[pingPong][i], vert->xyz);

		// set st
		vert->st[0] = DotProduct(vert->xyz, dp->texMat[axis][0]) + dp->texMat[axis][0][3];
		vert->st[1] = DotProduct(vert->xyz, dp->texMat[axis][1]) + dp->texMat[axis][1][3];

		// unidirectional decals fade by half distance from front->back planes
		if (!dp->omnidirectional)
		{
			// set alpha
			d     = DotProduct(vert->xyz, dp->planes[0]) - dp->planes[0][3];
			d2    = DotProduct(vert->xyz, dp->planes[1]) - dp->planes[1][3];
			alpha = 2.0f * d2 / (d + d2);
			if (alpha > 1.0f)
			{
				alpha = 1.0f;
			}
			else if (alpha < 0.0f)
			{
				alpha = 0.0f;
			}
		}

		// set color
		vert->modulate[0] = (byte)(pd * alpha * dp->color[0]);
		vert->modulate[1] = (byte)(pd * alpha * dp->color[1]);
		vert->modulate[2] = (byte)(pd * alpha * dp->color[2]);
		vert->modulate[3] = (byte)(alpha * dp->color[3]);
	}
}

/**
 * @brief Projects a decal onto a triangle surface (brush faces, misc_models, metasurfaces)
 * @param[in] dp
 * @param[in] surf
 * @param[in] bmodel
 */
static void ProjectDecalOntoTriangles(decalProjector_t *dp, msurface_t *surf, bmodel_t *bmodel)
{
	vec3_t         points[2][MAX_DECAL_VERTS];
	int            i;
	srfTriangles_t *srf = (srfTriangles_t *) surf->data; // get surface

	// walk triangle list
	for (i = 0; i < srf->numIndexes; i += 3)
	{
		// make triangle
		VectorCopy(srf->verts[srf->indexes[i]].xyz, points[0][0]);
		VectorCopy(srf->verts[srf->indexes[i + 1]].xyz, points[0][1]);
		VectorCopy(srf->verts[srf->indexes[i + 2]].xyz, points[0][2]);

		// chop it
		ProjectDecalOntoWinding(dp, 3, points, surf, bmodel);
	}
}

/**
 * @brief Projects a decal onto a grid (patch) surface
 * @param[in] dp
 * @param[in] surf
 * @param[in] bmodel
 */
static void ProjectDecalOntoGrid(decalProjector_t *dp, msurface_t *surf, bmodel_t *bmodel)
{
	int           x, y;
	srfGridMesh_t *srf = (srfGridMesh_t *) surf->data; // get surface
	drawVert_t    *dv;
	vec3_t        points[2][MAX_DECAL_VERTS];

	// walk mesh rows
	for (y = 0; y < (srf->height - 1); y++)
	{
		// walk mesh cols
		for (x = 0; x < (srf->width - 1); x++)
		{
			// get vertex
			dv = srf->verts + y * srf->width + x;

			// first triangle
			VectorCopy(dv[0].xyz, points[0][0]);
			VectorCopy(dv[srf->width].xyz, points[0][1]);
			VectorCopy(dv[1].xyz, points[0][2]);
			ProjectDecalOntoWinding(dp, 3, points, surf, bmodel);

			// second triangle
			VectorCopy(dv[1].xyz, points[0][0]);
			VectorCopy(dv[srf->width].xyz, points[0][1]);
			VectorCopy(dv[srf->width + 1].xyz, points[0][2]);
			ProjectDecalOntoWinding(dp, 3, points, surf, bmodel);
		}
	}
}

/**
 * @brief Projects a decal onto a world surface
 * @param[in] dp
 * @param[in] surf
 * @param[in] bmodel
 */
void R_ProjectDecalOntoSurface(decalProjector_t *dp, msurface_t *surf, bmodel_t *bmodel)
{
	srfGeneric_t *gen;
	int          i;

	// early outs
	if (dp->shader == NULL)
	{
		return;
	}
	//if( surf->viewCount == tr.viewCount )
	//  return;
	if ((surf->shader->surfaceFlags & (SURF_NOIMPACT | SURF_NOMARKS)) || (surf->shader->contentFlags & CONTENTS_FOG))
	{
		return;
	}

	// get generic surface
	gen = (srfGeneric_t *) surf->data;

	// ignore certain surfacetypes
	if (gen->surfaceType != SF_FACE && gen->surfaceType != SF_TRIANGLES && gen->surfaceType != SF_GRID)
	{
		return;
	}

	// test bounding sphere
	if (!R_TestDecalBoundingSphere(dp, gen->origin, (gen->radius * gen->radius)))
	{
		return;
	}

	// planar surface
	if (gen->plane.normal[0] != 0.f || gen->plane.normal[1] != 0.f || gen->plane.normal[2] != 0.f)
	{
		float d;

		// backface check
		d = DotProduct(dp->planes[0], gen->plane.normal);
		if (d < -0.0001f)
		{
			return;
		}

		// plane-sphere check
		d = DotProduct(dp->center, gen->plane.normal) - gen->plane.dist;
		if (Q_fabs(d) >= dp->radius)
		{
			return;
		}
	}

	// add to counts
	tr.pc.c_decalClipSurfaces++;

	// check if this projector already has a decal on this surface
	{
		int     count  = (bmodel == tr.world->bmodels ? MAX_WORLD_DECALS : MAX_ENTITY_DECALS);
		decal_t *decal = bmodel->decals;

		for (i = 0; i < count; i++, decal++)
		{
			if (decal->parent == surf && decal->projectorNum == dp->projectorNum)
			{
				return;
			}
		}
		// add to counts
		tr.pc.c_decalTestSurfaces++;
	}

	// switch on type
	switch (gen->surfaceType)
	{
	case SF_FACE:
	case SF_TRIANGLES:
		ProjectDecalOntoTriangles(dp, surf, bmodel);
		break;
	case SF_GRID:
		ProjectDecalOntoGrid(dp, surf, bmodel);
		break;
	default:
		break;
	}
}

/**
 * @brief Adds a decal surface to the scene
 * @param[in,out] decal
 */
void R_AddDecalSurface(decal_t *decal)
{
	int          dlightMap;
	srfDecal_t   *srf;
	srfGeneric_t *gen;

	// early outs
	if (decal->shader == NULL || (decal->parent != NULL && decal->parent->viewCount != tr.viewCount) || r_firstSceneDecal + tr.refdef.numDecals >= MAX_DECALS)
	{
		return;
	}

	// get decal surface
	srf = &tr.refdef.decals[tr.refdef.numDecals];
	tr.refdef.numDecals++;

	// set it up
	srf->surfaceType = SF_DECAL;
	srf->numVerts    = decal->numVerts;
	Com_Memcpy(srf->verts, decal->verts, srf->numVerts * sizeof(*srf->verts));

	// fade colors
	if (decal->fadeStartTime < tr.refdef.time && decal->fadeStartTime < decal->fadeEndTime)
	{
		int   i;
		float fade = (float) (decal->fadeEndTime - tr.refdef.time) /
		             (float) (decal->fadeEndTime - decal->fadeStartTime);

		fade = Com_Clamp(0.0f, 1.0f, fade);

		for (i = 0; i < decal->numVerts; i++)
		{
			decal->verts[i].modulate[0] *= fade;
			decal->verts[i].modulate[1] *= fade;
			decal->verts[i].modulate[2] *= fade;
			decal->verts[i].modulate[3] *= fade;
		}
	}

	// dynamic lights?
	if (decal->parent != NULL)
	{
		gen       = (srfGeneric_t *) decal->parent->data;
		dlightMap = (gen->dlightBits != 0);
	}
	else
	{
		dlightMap = 0;
	}

	// add surface to scene
	R_AddDrawSurf((void *) srf, decal->shader, decal->fogIndex, 0, dlightMap);
	tr.pc.c_decalSurfaces++;

	// free temporary decal
	if (decal->fadeEndTime <= tr.refdef.time)
	{
		decal->shader = NULL;
	}
}

/**
 * @brief Adds decal surfaces to the scene
 * @param[in] bmodel
 */
void R_AddDecalSurfaces(bmodel_t *bmodel)
{
	int     i;
	int     count  = (bmodel == tr.world->bmodels ? MAX_WORLD_DECALS : MAX_ENTITY_DECALS); // get decal count
	decal_t *decal = bmodel->decals;

	// iterate through decals
	for (i = 0; i < count; i++, decal++)
	{
		R_AddDecalSurface(decal);
	}
}

/**
 * @brief Frustum culls decal projector list
 */
void R_CullDecalProjectors(void)
{
	int              i, numDecalProjectors, decalBits;
	decalProjector_t *dp, temp;

	// walk decal projector list
	numDecalProjectors = 0;
	decalBits          = 0;
	for (i = 0, dp = tr.refdef.decalProjectors; i < tr.refdef.numDecalProjectors; i++, dp++)
	{
		if (R_CullPointAndRadius(dp->center, dp->radius) == CULL_OUT)
		{
			continue;
		}

		if (tr.refdef.numDecalProjectors > MAX_USED_DECAL_PROJECTORS)
		{
			// put all active projectors at the beginning
			if (dp != &tr.refdef.decalProjectors[numDecalProjectors])
			{
				// swap them
				temp                                          = tr.refdef.decalProjectors[numDecalProjectors];
				tr.refdef.decalProjectors[numDecalProjectors] = *dp;
				*dp                                           = temp;
			}

			decalBits |= (1 << numDecalProjectors);
			numDecalProjectors++;

			// bitmask limit
			if (numDecalProjectors == MAX_USED_DECAL_PROJECTORS)
			{
				break;
			}
		}
		else
		{
			decalBits         |= (1 << i);
			numDecalProjectors = i + 1;
		}
	}

	// reset count
	tr.refdef.numDecalProjectors = numDecalProjectors;
	tr.pc.c_decalProjectors      = numDecalProjectors;

	// set bits
	tr.refdef.decalBits = decalBits;
}
