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
 * @file rendererGLES/tr_surface.c
 *
 * THIS ENTIRE FILE IS BACK END
 * b ackEnd.currentEntity *will be valid.
 * Tess_Begin has already been called for the surface's shader.
 *
 * The modelview matrix will be set.
 *
 * It is safe to actually issue drawing commands here if you don't want to
 * use the shader system.
 */

#include "tr_local.h"

/**
 * @brief RB_CheckOverflow
 * @param[in] verts
 * @param[in] indexes
 */
void RB_CheckOverflow(int verts, int indexes)
{
	if (tess.numVertexes + verts < SHADER_MAX_VERTEXES && tess.numIndexes + indexes < SHADER_MAX_INDEXES)
	{
		return;
	}

	RB_EndSurface();

	if (verts >= SHADER_MAX_VERTEXES)
	{
		Ren_Drop("RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES);
	}
	if (indexes >= SHADER_MAX_INDEXES)
	{
		Ren_Drop("RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES);
	}

	RB_BeginSurface(tess.shader, tess.fogNum);
}

/*
 * @brief Creates a sprite with the center at colors[3] alpha, and the corners all 0 alpha
 * @param[in] origin
 * @param[in] left
 * @param[in] up
 * @param[in] color
 * @param[in] s1
 * @param[in] t1
 * @param[in] s2
 * @param[in] t2
 *
 * @note Unused.
void RB_AddQuadStampFadingCornersExt(vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2)
{
    vec3_t normal;
    int    ndx;
    byte   lColor[4];

    RB_CHECKOVERFLOW(5, 12);

    ndx = tess.numVertexes;

    // triangle indexes for a simple quad
    tess.indexes[tess.numIndexes + 0] = ndx + 0;
    tess.indexes[tess.numIndexes + 1] = ndx + 1;
    tess.indexes[tess.numIndexes + 2] = ndx + 4;

    tess.indexes[tess.numIndexes + 3] = ndx + 1;
    tess.indexes[tess.numIndexes + 4] = ndx + 2;
    tess.indexes[tess.numIndexes + 5] = ndx + 4;

    tess.indexes[tess.numIndexes + 6] = ndx + 2;
    tess.indexes[tess.numIndexes + 7] = ndx + 3;
    tess.indexes[tess.numIndexes + 8] = ndx + 4;

    tess.indexes[tess.numIndexes + 9]  = ndx + 3;
    tess.indexes[tess.numIndexes + 10] = ndx + 0;
    tess.indexes[tess.numIndexes + 11] = ndx + 4;

    tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
    tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
    tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

    tess.xyz[ndx + 1][0] = origin[0] - left[0] + up[0];
    tess.xyz[ndx + 1][1] = origin[1] - left[1] + up[1];
    tess.xyz[ndx + 1][2] = origin[2] - left[2] + up[2];

    tess.xyz[ndx + 2][0] = origin[0] - left[0] - up[0];
    tess.xyz[ndx + 2][1] = origin[1] - left[1] - up[1];
    tess.xyz[ndx + 2][2] = origin[2] - left[2] - up[2];

    tess.xyz[ndx + 3][0] = origin[0] + left[0] - up[0];
    tess.xyz[ndx + 3][1] = origin[1] + left[1] - up[1];
    tess.xyz[ndx + 3][2] = origin[2] + left[2] - up[2];

    tess.xyz[ndx + 4][0] = origin[0];
    tess.xyz[ndx + 4][1] = origin[1];
    tess.xyz[ndx + 4][2] = origin[2];


    // constant normal all the way around
    VectorSubtract(vec3_origin, backEnd.viewParms.orientation.axis[0], normal);

    tess.normal[ndx][0] = tess.normal[ndx + 1][0] = tess.normal[ndx + 2][0] = tess.normal[ndx + 3][0] = tess.normal[ndx + 4][0] = normal[0];
    tess.normal[ndx][1] = tess.normal[ndx + 1][1] = tess.normal[ndx + 2][1] = tess.normal[ndx + 3][1] = tess.normal[ndx + 4][1] = normal[1];
    tess.normal[ndx][2] = tess.normal[ndx + 1][2] = tess.normal[ndx + 2][2] = tess.normal[ndx + 3][2] = tess.normal[ndx + 4][2] = normal[2];

    // standard square texture coordinates
    tess.texCoords0[ndx][0] = tess.texCoords1[ndx][0] = s1;
    tess.texCoords0[ndx][1] = tess.texCoords1[ndx][1] = t1;

    tess.texCoords0[ndx + 1][0] = tess.texCoords1[ndx + 1][0] = s2;
    tess.texCoords0[ndx + 1][1] = tess.texCoords1[ndx + 1][1] = t1;

    tess.texCoords0[ndx + 2][0] = tess.texCoords1[ndx + 2][0] = s2;
    tess.texCoords0[ndx + 2][1] = tess.texCoords1[ndx + 2][1] = t2;

    tess.texCoords0[ndx + 3][0] = tess.texCoords1[ndx + 3][0] = s1;
    tess.texCoords0[ndx + 3][1] = tess.texCoords1[ndx + 3][1] = t2;

    tess.texCoords0[ndx + 4][0] = tess.texCoords1[ndx + 4][0] = (s1 + s2) / 2.0;
    tess.texCoords0[ndx + 4][1] = tess.texCoords1[ndx + 4][1] = (t1 + t2) / 2.0;

    // center uses full alpha
    *( unsigned int * ) &tess.vertexColors[ndx + 4] =
        *( unsigned int * )color;

    // fade around edges
    Com_Memcpy(lColor, color, sizeof(byte) * 4);
    lColor[3]                                                     = 0;
    *( unsigned int * ) &tess.vertexColors[ndx]                 =
        *( unsigned int * ) &tess.vertexColors[ndx + 1]         =
            *( unsigned int * ) &tess.vertexColors[ndx + 2]     =
                *( unsigned int * ) &tess.vertexColors[ndx + 3] =
                    *( unsigned int * )lColor;


    tess.numVertexes += 5;
    tess.numIndexes  += 12;
}
*/

/**
 * @brief RB_AddQuadStampExt
 * @param[in] origin
 * @param[in] left
 * @param[in] up
 * @param[in] color
 * @param[in] s1
 * @param[in] t1
 * @param[in] s2
 * @param[in] t2
 */
void RB_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2)
{
	vec3_t normal;
	int    ndx;

	RB_CHECKOVERFLOW(4, 6);

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[tess.numIndexes]     = ndx;
	tess.indexes[tess.numIndexes + 1] = ndx + 1;
	tess.indexes[tess.numIndexes + 2] = ndx + 3;

	tess.indexes[tess.numIndexes + 3] = ndx + 3;
	tess.indexes[tess.numIndexes + 4] = ndx + 1;
	tess.indexes[tess.numIndexes + 5] = ndx + 2;

	tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
	tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
	tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

	tess.xyz[ndx + 1][0] = origin[0] - left[0] + up[0];
	tess.xyz[ndx + 1][1] = origin[1] - left[1] + up[1];
	tess.xyz[ndx + 1][2] = origin[2] - left[2] + up[2];

	tess.xyz[ndx + 2][0] = origin[0] - left[0] - up[0];
	tess.xyz[ndx + 2][1] = origin[1] - left[1] - up[1];
	tess.xyz[ndx + 2][2] = origin[2] - left[2] - up[2];

	tess.xyz[ndx + 3][0] = origin[0] + left[0] - up[0];
	tess.xyz[ndx + 3][1] = origin[1] + left[1] - up[1];
	tess.xyz[ndx + 3][2] = origin[2] + left[2] - up[2];

	// constant normal all the way around
	VectorSubtract(vec3_origin, backEnd.viewParms.orientation.axis[0], normal);

	tess.normal[ndx][0] = tess.normal[ndx + 1][0] = tess.normal[ndx + 2][0] = tess.normal[ndx + 3][0] = normal[0];
	tess.normal[ndx][1] = tess.normal[ndx + 1][1] = tess.normal[ndx + 2][1] = tess.normal[ndx + 3][1] = normal[1];
	tess.normal[ndx][2] = tess.normal[ndx + 1][2] = tess.normal[ndx + 2][2] = tess.normal[ndx + 3][2] = normal[2];

	// standard square texture coordinates
	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s1;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t1;

	tess.texCoords[ndx + 1][0][0] = tess.texCoords[ndx + 1][1][0] = s2;
	tess.texCoords[ndx + 1][0][1] = tess.texCoords[ndx + 1][1][1] = t1;

	tess.texCoords[ndx + 2][0][0] = tess.texCoords[ndx + 2][1][0] = s2;
	tess.texCoords[ndx + 2][0][1] = tess.texCoords[ndx + 2][1][1] = t2;

	tess.texCoords[ndx + 3][0][0] = tess.texCoords[ndx + 3][1][0] = s1;
	tess.texCoords[ndx + 3][0][1] = tess.texCoords[ndx + 3][1][1] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	*( unsigned int * ) &tess.vertexColors[ndx]                 =
	    *( unsigned int * ) &tess.vertexColors[ndx + 1]         =
	        *( unsigned int * ) &tess.vertexColors[ndx + 2]     =
	            *( unsigned int * ) &tess.vertexColors[ndx + 3] =
	                *( unsigned int * )color;


	tess.numVertexes += 4;
	tess.numIndexes  += 6;
}

/**
 * @brief RB_AddQuadStamp
 * @param[in] origin
 * @param[in] left
 * @param[in] up
 * @param[in] color
 */
void RB_AddQuadStamp(vec3_t origin, vec3_t left, vec3_t up, byte *color)
{
	RB_AddQuadStampExt(origin, left, up, color, 0, 0, 1, 1);
}

/**
 * @brief RB_SurfaceSplash
 */
static void RB_SurfaceSplash(void)
{
	vec3_t left, up;
	float  radius = backEnd.currentEntity->e.radius;

	// calculate the xyz locations for the four corners

	VectorSet(left, -radius, 0, 0);
	VectorSet(up, 0, radius, 0);
	if (backEnd.viewParms.isMirror)
	{
		VectorSubtract(vec3_origin, left, left);
	}

	RB_AddQuadStamp(backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA);
}

/**
 * @brief RB_SurfaceSprite
 */
static void RB_SurfaceSprite(void)
{
	vec3_t left, up;
	float  radius = backEnd.currentEntity->e.radius;

	// calculate the xyz locations for the four corners

	if (backEnd.currentEntity->e.rotation == 0.f)
	{
		VectorScale(backEnd.viewParms.orientation.axis[1], radius, left);
		VectorScale(backEnd.viewParms.orientation.axis[2], radius, up);
	}
	else
	{
		float ang = M_PI * backEnd.currentEntity->e.rotation / 180;
		float s   = sin(ang);
		float c   = cos(ang);

		VectorScale(backEnd.viewParms.orientation.axis[1], c * radius, left);
		VectorMA(left, -s * radius, backEnd.viewParms.orientation.axis[2], left);

		VectorScale(backEnd.viewParms.orientation.axis[2], c * radius, up);
		VectorMA(up, s * radius, backEnd.viewParms.orientation.axis[1], up);
	}
	if (backEnd.viewParms.isMirror)
	{
		VectorSubtract(vec3_origin, left, left);
	}

	RB_AddQuadStamp(backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA);
}

/**
 * @brief RB_SurfacePolychain
 * @param[in] p
 */
void RB_SurfacePolychain(srfPoly_t *p)
{
	int i;
	int numv;

	RB_CHECKOVERFLOW(p->numVerts, 3 * (p->numVerts - 2));

	// fan triangles into the tess array
	numv = tess.numVertexes;
	for (i = 0; i < p->numVerts; i++)
	{
		VectorCopy(p->verts[i].xyz, tess.xyz[numv]);
		tess.texCoords[numv][0][0]       = p->verts[i].st[0];
		tess.texCoords[numv][0][1]       = p->verts[i].st[1];
		*(int *)&tess.vertexColors[numv] = *(int *)p->verts[i].modulate;

		numv++;
	}

	// generate fan indexes into the tess array
	for (i = 0; i < p->numVerts - 2; i++)
	{
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes                  += 3;
	}

	tess.numVertexes = numv;
}

/**
 * @brief RB_SurfaceTriangles
 * @param[in] srf
 */
void RB_SurfaceTriangles(srfTriangles_t *srf)
{
	int        i;
	drawVert_t *dv;
	float      *xyz, *normal, *texCoords;
	byte       *color;
	int        dlightBits;
	qboolean   needsNormal;

	// moved before overflow so dlights work properly
	RB_CHECKOVERFLOW(srf->numVerts, srf->numIndexes);

	dlightBits       = srf->dlightBits;
	tess.dlightBits |= dlightBits;

	for (i = 0 ; i < srf->numIndexes ; i += 3)
	{
		tess.indexes[tess.numIndexes + i + 0] = tess.numVertexes + srf->indexes[i + 0];
		tess.indexes[tess.numIndexes + i + 1] = tess.numVertexes + srf->indexes[i + 1];
		tess.indexes[tess.numIndexes + i + 2] = tess.numVertexes + srf->indexes[i + 2];
	}
	tess.numIndexes += srf->numIndexes;

	dv          = srf->verts;
	xyz         = tess.xyz[tess.numVertexes];
	normal      = tess.normal[tess.numVertexes];
	texCoords   = tess.texCoords[tess.numVertexes][0];
	color       = tess.vertexColors[tess.numVertexes];
	needsNormal = tess.shader->needsNormal;

	if (needsNormal)
	{
		for (i = 0 ; i < srf->numVerts ; i++, dv++, xyz += 4, normal += 4, texCoords += 4, color += 4)
		{
			xyz[0] = dv->xyz[0];
			xyz[1] = dv->xyz[1];
			xyz[2] = dv->xyz[2];

			normal[0] = dv->normal[0];
			normal[1] = dv->normal[1];
			normal[2] = dv->normal[2];

			texCoords[0] = dv->st[0];
			texCoords[1] = dv->st[1];

			texCoords[2] = dv->lightmap[0];
			texCoords[3] = dv->lightmap[1];

			*(int *)color = *(int *)dv->color;
		}
	}
	else
	{
		for (i = 0 ; i < srf->numVerts ; i++, dv++, xyz += 4, normal += 4, texCoords += 4, color += 4)
		{
			xyz[0] = dv->xyz[0];
			xyz[1] = dv->xyz[1];
			xyz[2] = dv->xyz[2];

			texCoords[0] = dv->st[0];
			texCoords[1] = dv->st[1];

			texCoords[2] = dv->lightmap[0];
			texCoords[3] = dv->lightmap[1];

			*(int *)color = *(int *)dv->color;
		}
	}

	tess.numVertexes += srf->numVerts;
}

/**
 * @brief RB_SurfaceFoliage
 * @param[in] srf
 */
void RB_SurfaceFoliage(srfFoliage_t *srf)
{
	int               o, i, a;
	int               numVerts = srf->numVerts, numIndexes = srf->numIndexes;   // basic setup
	vec4_t            distanceCull, distanceVector;
	float             alpha, z, dist, fovScale = backEnd.viewParms.fovX * (1.0f / 90.0f);
	vec3_t            local;
	vec_t             *xyz;
	int               srcColor, *color;
	int               dlightBits;
	foliageInstance_t *instance;

	// calculate distance vector
	VectorSubtract(backEnd.orientation.origin, backEnd.viewParms.orientation.origin, local);
	distanceVector[0] = -backEnd.orientation.modelMatrix[2];
	distanceVector[1] = -backEnd.orientation.modelMatrix[6];
	distanceVector[2] = -backEnd.orientation.modelMatrix[10];
	distanceVector[3] = DotProduct(local, backEnd.viewParms.orientation.axis[0]);

	// attempt distance cull
	Vector4Copy(tess.shader->distanceCull, distanceCull);

	if (distanceCull[1] > 0)
	{
		z     = fovScale * (DotProduct(srf->origin, distanceVector) + distanceVector[3] - srf->radius);
		alpha = (distanceCull[1] - z) * distanceCull[3];
		if (alpha < distanceCull[2])
		{
			return;
		}
	}

	// set dlight bits
	dlightBits       = srf->dlightBits;
	tess.dlightBits |= dlightBits;

	// iterate through origin list
	instance = srf->instances;
	for (o = 0; o < srf->numInstances; o++, instance++)
	{
		// fade alpha based on distance between inner and outer radii
		if (distanceCull[1] > 0.0f)
		{
			// calculate z distance
			z = fovScale * (DotProduct(instance->origin, distanceVector) + distanceVector[3]);
			if (z < -64.0f)      // epsilon so close-by foliage doesn't pop in and out
			{
				continue;
			}

			// check against frustum planes
			for (i = 0; i < 5; i++)
			{
				dist = DotProduct(instance->origin, backEnd.viewParms.frustum[i].normal) - backEnd.viewParms.frustum[i].dist;
				if (dist < -64)
				{
					break;
				}
			}
			if (i != 5)
			{
				continue;
			}

			// radix
			if (o & 1)
			{
				z *= 1.25;
				if (o & 2)
				{
					z *= 1.25;
				}
			}

			// calculate alpha
			alpha = (distanceCull[1] - z) * distanceCull[3];
			if (alpha < distanceCull[2])
			{
				continue;
			}

			// set color
			a        = alpha > 1.0f ? 255 : (int)(alpha * 255);
#ifdef Q3_BIG_ENDIAN // LBO 3/15/05. Byte-swap fix for Mac - alpha is in the LSB.
			srcColor = (*((int*) instance->color) & 0xFFFFFF00) | (a & 0xff);
#else
			srcColor = (*((int *) instance->color) & 0xFFFFFF) | (a << 24);
#endif
		}
		else
		{
			srcColor = *((int *) instance->color);
		}

		//Ren_Print("Color: %d %d %d %d\n", srf->colors[ o ][ 0 ], srf->colors[ o ][ 1 ], srf->colors[ o ][ 2 ], alpha );

		RB_CHECKOVERFLOW(numVerts, numIndexes);

		// set after overflow check so dlights work properly
		tess.dlightBits |= dlightBits;

		// copy indexes
		Com_Memcpy(&tess.indexes[tess.numIndexes], srf->indexes, numIndexes * sizeof(srf->indexes[0]));
		for (i = 0; i < numIndexes; i++)
		{
			tess.indexes[tess.numIndexes + i] += tess.numVertexes;
		}

		// copy xyz, normal and st
		xyz = tess.xyz[tess.numVertexes];
		Com_Memcpy(xyz, srf->xyz, numVerts * sizeof(srf->xyz[0]));
		if (tess.shader->needsNormal)
		{
			Com_Memcpy(&tess.normal[tess.numVertexes], srf->normal, numVerts * sizeof(srf->xyz[0]));
		}

		for (i = 0; i < numVerts; i++)
		{
			tess.texCoords[tess.numVertexes + i][0][0] = srf->texCoords[i][0];
			tess.texCoords[tess.numVertexes + i][0][1] = srf->texCoords[i][1];

			tess.texCoords[tess.numVertexes + i][1][0] = srf->lmTexCoords[i][0];
			tess.texCoords[tess.numVertexes + i][1][1] = srf->lmTexCoords[i][1];
		}

		// offset xyz
		for (i = 0; i < numVerts; i++, xyz += 4)
		{
			VectorAdd(xyz, instance->origin, xyz);
		}

		// copy color
		color = (int *) tess.vertexColors[tess.numVertexes];
		for (i = 0; i < numVerts; i++)
		{
			color[i] = srcColor;
		}

		// increment
		tess.numIndexes  += numIndexes;
		tess.numVertexes += numVerts;
	}

	// RB_DrawBounds( srf->bounds[ 0 ], srf->bounds[ 1 ] );
}

/**
 * @brief RB_SurfaceBeam
 */
void RB_SurfaceBeam(void)
{
#define NUM_BEAM_SEGS 6
	refEntity_t *e = &backEnd.currentEntity->e;
	int         i;
	vec3_t      perpvec;
	vec3_t      direction, normalized_direction;
	vec3_t      start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t      oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if (VectorNormalize(normalized_direction) == 0.f)
	{
		return;
	}

	PerpendicularVector(perpvec, normalized_direction);

	VectorScale(perpvec, 4, perpvec);

	for (i = 0; i < NUM_BEAM_SEGS ; i++)
	{
		RotatePointAroundVector(start_points[i], normalized_direction, perpvec, (360.0f / NUM_BEAM_SEGS) * i);
		VectorAdd(start_points[i], direction, end_points[i]);
	}

	GL_Bind(tr.whiteImage);

	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

	qglColor3f(1, 0, 0);

	// OpenGLES implementation
	GLboolean text  = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (glcol)
	{
		qglDisableClientState(GL_COLOR_ARRAY);
	}
	if (text)
	{
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	GLfloat vtx[NUM_BEAM_SEGS * 6 + 6];
	for (i = 0; i <= NUM_BEAM_SEGS; i++)
	{
		Com_Memcpy(vtx + i * 6, start_points[i % NUM_BEAM_SEGS], sizeof(GLfloat) * 3);
		Com_Memcpy(vtx + i * 6 + 3, end_points[i % NUM_BEAM_SEGS], sizeof(GLfloat) * 3);
	}
	qglVertexPointer(3, GL_FLOAT, 0, vtx);
	qglDrawArrays(GL_TRIANGLE_STRIP, 0, NUM_BEAM_SEGS * 2 + 2);
	if (glcol)
	{
		qglEnableClientState(GL_COLOR_ARRAY);
	}
	if (text)
	{
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
}

//================================================================================

/**
 * @brief DoRailCore
 * @param[in] start
 * @param[in] end
 * @param[in] up
 * @param[in] len
 * @param[in] spanWidth
 */
static void DoRailCore(const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth)
{
	float spanWidth2;
	int   vbase = tess.numVertexes;
	float t;       // = len / 256.0f;

	// configurable tile
	if (backEnd.currentEntity->e.radius > 0)
	{
		t = len / backEnd.currentEntity->e.radius;
	}
	else
	{
		t = len / 256.f;
	}

	spanWidth2 = -spanWidth;

	// FIXME: use quad stamp?
	VectorMA(start, spanWidth, up, tess.xyz[tess.numVertexes]);
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	VectorMA(start, spanWidth2, up, tess.xyz[tess.numVertexes]);
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	VectorMA(end, spanWidth, up, tess.xyz[tess.numVertexes]);

	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	VectorMA(end, spanWidth2, up, tess.xyz[tess.numVertexes]);
	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}

/**
 * @brief DoRailDiscs
 * @param[in] numSegs
 * @param[in] start
 * @param[in] dir
 * @param[in] right
 * @param[in] up
 */
static void DoRailDiscs(int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up)
{
	int    i, j;
	vec3_t pos[4];
	vec3_t v;
	int    spanWidth = r_railWidth->integer;
	float  c, s;
	float  scale;

	if (numSegs > 1)
	{
		numSegs--;
	}
	if (!numSegs)
	{
		return;
	}

	scale = 0.25f;

	for (i = 0; i < 4; i++)
	{
		c    = cos(DEG2RAD(45 + i * 90));
		s    = sin(DEG2RAD(45 + i * 90));
		v[0] = (right[0] * c + up[0] * s) * scale * spanWidth;
		v[1] = (right[1] * c + up[1] * s) * scale * spanWidth;
		v[2] = (right[2] * c + up[2] * s) * scale * spanWidth;
		VectorAdd(start, v, pos[i]);

		if (numSegs > 1)
		{
			// offset by 1 segment if we're doing a long distance shot
			VectorAdd(pos[i], dir, pos[i]);
		}
	}

	for (i = 0; i < numSegs; i++)
	{
		RB_CHECKOVERFLOW(4, 6);

		for (j = 0; j < 4; j++)
		{
			VectorCopy(pos[j], tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0][0] = (j < 2);
			tess.texCoords[tess.numVertexes][0][1] = (j && j != 3);
			tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
			tess.numVertexes++;

			VectorAdd(pos[j], dir, pos[j]);
		}

		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 0;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 2;
	}
}

/**
 * @brief RB_SurfaceRailRings
 */
void RB_SurfaceRailRings(void)
{
	refEntity_t *e = &backEnd.currentEntity->e;
	int         numSegs;
	int         len;
	vec3_t      vec;
	vec3_t      right, up;
	vec3_t      start, end;

	VectorCopy(e->oldorigin, start);
	VectorCopy(e->origin, end);

	// compute variables
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	MakeNormalVectors(vec, right, up);
	numSegs = (len) / r_railSegmentLength->value;
	if (numSegs <= 0)
	{
		numSegs = 1;
	}

	VectorScale(vec, r_railSegmentLength->value, vec);

	DoRailDiscs(numSegs, start, vec, right, up);
}

/**
 * @brief RB_SurfaceRailCore
 */
void RB_SurfaceRailCore(void)
{
	refEntity_t *e = &backEnd.currentEntity->e;
	int         len;
	vec3_t      right;
	vec3_t      vec;
	vec3_t      start, end;
	vec3_t      v1, v2;

	VectorCopy(e->oldorigin, start);
	VectorCopy(e->origin, end);

	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	// compute side vector
	VectorSubtract(start, backEnd.viewParms.orientation.origin, v1);
	VectorNormalize(v1);
	VectorSubtract(end, backEnd.viewParms.orientation.origin, v2);
	VectorNormalize(v2);
	CrossProduct(v1, v2, right);
	VectorNormalize(right);

	DoRailCore(start, end, right, len, e->frame > 0 ? e->frame : 1);
}

/**
 * @brief RB_SurfaceLightningBolt
 */
void RB_SurfaceLightningBolt(void)
{
	refEntity_t *e = &backEnd.currentEntity->e;
	int         len;
	vec3_t      right;
	vec3_t      vec;
	vec3_t      start, end;
	vec3_t      v1, v2;
	vec3_t      temp;
	int         i;

	VectorCopy(e->oldorigin, end);
	VectorCopy(e->origin, start);

	// compute variables
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	// compute side vector
	VectorSubtract(start, backEnd.viewParms.orientation.origin, v1);
	VectorNormalize(v1);
	VectorSubtract(end, backEnd.viewParms.orientation.origin, v2);
	VectorNormalize(v2);
	CrossProduct(v1, v2, right);
	VectorNormalize(right);

	for (i = 0 ; i < 4 ; i++)
	{
		DoRailCore(start, end, right, len, 8);
		RotatePointAroundVector(temp, vec, right, 45);
		VectorCopy(temp, right);
	}
}


/**
 * @brief LerpMeshVertexes
 * @param[in] surf
 * @param[in] backlerp
 */
static void LerpMeshVertexes(md3Surface_t *surf, float backlerp)
{
	short    *oldXyz, *newXyz, *oldNormals, *newNormals;
	float    *outXyz, *outNormal;
	float    newXyzScale;
	float    newNormalScale;
	int      vertNum;
	unsigned lat, lng;
	int      numVerts;

	outXyz    = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newXyz = ( short * )((byte *)surf + surf->ofsXyzNormals)
	         + (backEnd.currentEntity->e.frame * surf->numVerts * 4);
	newNormals = newXyz + 3;

	newXyzScale    = MD3_XYZ_SCALE * (1.0 - backlerp);
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if (backlerp == 0)
	{
		// just copy the vertexes
		for (vertNum = 0 ; vertNum < numVerts ; vertNum++,
		     newXyz += 4, newNormals += 4,
		     outXyz += 4, outNormal += 4)
		{

			outXyz[0] = newXyz[0] * newXyzScale;
			outXyz[1] = newXyz[1] * newXyzScale;
			outXyz[2] = newXyz[2] * newXyzScale;

			lat  = (newNormals[0] >> 8) & 0xff;
			lng  = (newNormals[0] & 0xff);
			lat *= (FUNCTABLE_SIZE / 256);
			lng *= (FUNCTABLE_SIZE / 256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
		}
	}
	else
	{
		float oldNormalScale, oldXyzScale;

		// interpolate and copy the vertex and normal
		oldXyz = ( short * )((byte *)surf + surf->ofsXyzNormals)
		         + (backEnd.currentEntity->e.oldframe * surf->numVerts * 4);
		oldNormals = oldXyz + 3;

		oldXyzScale    = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for (vertNum = 0 ; vertNum < numVerts ; vertNum++,
		     oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
		     outXyz += 4, outNormal += 4)
		{
			//vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// interpolate lat/long instead
#if 0
			lat                      = (newNormals[0] >> 8) & 0xff;
			lng                      = (newNormals[0] & 0xff);
			lat                     *= (FUNCTABLE_SIZE / 256);
			lng                     *= (FUNCTABLE_SIZE / 256);
			uncompressedNewNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedNewNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

			lat  = (oldNormals[0] >> 8) & 0xff;
			lng  = (oldNormals[0] & 0xff);
			lat *= (FUNCTABLE_SIZE / 256);
			lng *= (FUNCTABLE_SIZE / 256);

			uncompressedOldNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedOldNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;
#else
			lat = (int)((((oldNormals[0] >> 8) & 0xFF) * (FUNCTABLE_SIZE / 256) * newNormalScale) +
			            (((oldNormals[0] >> 8) & 0xFF) * (FUNCTABLE_SIZE / 256) * oldNormalScale));
			lng = (int)(((oldNormals[0] & 0xFF) * (FUNCTABLE_SIZE / 256) * newNormalScale) +
			            ((oldNormals[0] & 0xFF) * (FUNCTABLE_SIZE / 256) * oldNormalScale));

			outNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
#endif

			//VectorNormalize (outNormal);
		}

		// unecessary because of lat/lng lerping
		//VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes], numVerts);
	}
}


/**
 * @brief RB_SurfaceMesh
 * @param[in] surface
 */
void RB_SurfaceMesh(md3Surface_t *surface)
{
	int   j;
	float backlerp;
	int   *triangles;
	float *texCoords;
	int   indexes;
	int   Bob, Doug;
	int   numVerts;

	// check for REFLAG_HANDONLY
	if (backEnd.currentEntity->e.reFlags & REFLAG_ONLYHAND)
	{
		if (!strstr(surface->name, "hand"))
		{
			return;
		}
	}

	if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame)
	{
		backlerp = 0;
	}
	else
	{
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW(surface->numVerts, surface->numTriangles * 3);

	LerpMeshVertexes(surface, backlerp);

	triangles = ( int * )((byte *)surface + surface->ofsTriangles);
	indexes   = surface->numTriangles * 3;
	Bob       = tess.numIndexes;
	Doug      = tess.numVertexes;
	for (j = 0 ; j < indexes ; j++)
	{
		tess.indexes[Bob + j] = Doug + triangles[j];
	}
	tess.numIndexes += indexes;

	texCoords = ( float * )((byte *)surface + surface->ofsSt);

	numVerts = surface->numVerts;
	for (j = 0; j < numVerts; j++)
	{
		tess.texCoords[Doug + j][0][0] = texCoords[j * 2 + 0];
		tess.texCoords[Doug + j][0][1] = texCoords[j * 2 + 1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}

/**
 * @brief R_LatLongToNormal
 * @param[in] outNormal
 * @param[in] latLong
 */
void R_LatLongToNormal(vec3_t outNormal, short latLong)
{
	unsigned lat = (latLong >> 8) & 0xff;
	unsigned lng = (latLong & 0xff);

	lat *= (FUNCTABLE_SIZE / 256);
	lng *= (FUNCTABLE_SIZE / 256);

	// decode X as cos( lat ) * sin( long )
	// decode Y as sin( lat ) * sin( long )
	// decode Z as cos( long )

	outNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
	outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
	outNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
}

/**
 * @brief LerpCMeshVertexes
 * @param[in] surf
 * @param[in] backlerp
 */
static void LerpCMeshVertexes(mdcSurface_t *surf, float backlerp)
{
	vec3_t             oldOfsVec, newOfsVec;
	float              *outXyz, *outNormal;
	float              newXyzScale;
	float              newNormalScale;
	int                vertNum;
	unsigned           lat, lng;
	int                numVerts;
	int                newBase;
	short              *oldComp = NULL, *newComp = NULL;
	short              *oldXyz, *newXyz, *oldNormals, *newNormals;
	mdcXyzCompressed_t *oldXyzComp = NULL, *newXyzComp = NULL;
	qboolean           hasComp;

	outXyz    = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newBase = (int)*(( short * )((byte *)surf + surf->ofsFrameBaseFrames) + backEnd.currentEntity->e.frame);
	newXyz  = ( short * )((byte *)surf + surf->ofsXyzNormals)
	          + (newBase * surf->numVerts * 4);
	newNormals = newXyz + 3;

	hasComp = (surf->numCompFrames > 0);
	if (hasComp)
	{
		newComp = (( short * )((byte *)surf + surf->ofsFrameCompFrames) + backEnd.currentEntity->e.frame);
		if (*newComp >= 0)
		{
			newXyzComp = ( mdcXyzCompressed_t * )((byte *)surf + surf->ofsXyzCompressed)
			             + (*newComp * surf->numVerts);
		}
	}

	newXyzScale    = MD3_XYZ_SCALE * (1.0 - backlerp);
	newNormalScale = 1.0f - backlerp;

	numVerts = surf->numVerts;

	if (backlerp == 0.f)
	{
		// just copy the vertexes
		for (vertNum = 0 ; vertNum < numVerts ; vertNum++,
		     newXyz += 4, newNormals += 4,
		     outXyz += 4, outNormal += 4)
		{
			outXyz[0] = newXyz[0] * newXyzScale;
			outXyz[1] = newXyz[1] * newXyzScale;
			outXyz[2] = newXyz[2] * newXyzScale;

			// add the compressed ofsVec
			if (hasComp && *newComp >= 0)
			{
				R_MDC_DecodeXyzCompressed(newXyzComp->ofsVec, newOfsVec, outNormal);
				newXyzComp++;
				VectorAdd(outXyz, newOfsVec, outXyz);
			}
			else
			{
				lat  = (newNormals[0] >> 8) & 0xff;
				lng  = (newNormals[0] & 0xff);
				lat *= (FUNCTABLE_SIZE / 256);      // was 4 :sigh:
				lng *= (FUNCTABLE_SIZE / 256);

				outNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
				outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
				outNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

				// testing anorms table
				// VectorCopy( r_anormals[ (lng & (0xF << 4)) | ((lat >> 4) & 0xF) ], outNormal );
			}
		}
	}
	else
	{
		float oldXyzScale, oldNormalScale;
		// interpolate and copy the vertex and normal
		int oldBase = (int)*(( short * )((byte *)surf + surf->ofsFrameBaseFrames) + backEnd.currentEntity->e.oldframe);

		oldXyz = ( short * )((byte *)surf + surf->ofsXyzNormals)
		         + (oldBase * surf->numVerts * 4);
		oldNormals = oldXyz + 3;

		if (hasComp)
		{
			oldComp = (( short * )((byte *)surf + surf->ofsFrameCompFrames) + backEnd.currentEntity->e.oldframe);
			if (*oldComp >= 0)
			{
				oldXyzComp = ( mdcXyzCompressed_t * )((byte *)surf + surf->ofsXyzCompressed)
				             + (*oldComp * surf->numVerts);
			}
		}

		oldXyzScale    = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for (vertNum = 0 ; vertNum < numVerts ; vertNum++,
		     oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
		     outXyz += 4, outNormal += 4)
		{
			vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// add the compressed ofsVec
			if (hasComp && *newComp >= 0)
			{
				R_MDC_DecodeXyzCompressed(newXyzComp->ofsVec, newOfsVec, uncompressedNewNormal);
				newXyzComp++;
				VectorMA(outXyz, 1.0f - backlerp, newOfsVec, outXyz);
			}
			else
			{
				lat  = (newNormals[0] >> 8) & 0xff;
				lng  = (newNormals[0] & 0xff);
				lat *= (FUNCTABLE_SIZE / 256);      // was 4 :sigh:
				lng *= (FUNCTABLE_SIZE / 256);

				uncompressedNewNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
				uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
				uncompressedNewNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
			}

			if (hasComp && *oldComp >= 0)
			{
				R_MDC_DecodeXyzCompressed(oldXyzComp->ofsVec, oldOfsVec, uncompressedOldNormal);
				oldXyzComp++;
				VectorMA(outXyz, backlerp, oldOfsVec, outXyz);
			}
			else
			{
				lat  = (oldNormals[0] >> 8) & 0xff;
				lng  = (oldNormals[0] & 0xff);
				lat *= (FUNCTABLE_SIZE / 256);      // was 4 :sigh:
				lng *= (FUNCTABLE_SIZE / 256);

				uncompressedOldNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
				uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
				uncompressedOldNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
			}

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

			// wee bit faster (fixme: use lat/lng lerping)
			//% VectorNormalize (outNormal);
			VectorNormalizeFast(outNormal);
		}
	}
}

/**
 * @brief RB_SurfaceCMesh
 * @param[in] surface
 */
void RB_SurfaceCMesh(mdcSurface_t *surface)
{
	int   j;
	float backlerp;
	int   *triangles;
	float *texCoords;
	int   indexes;
	int   Bob, Doug;
	int   numVerts;

	// check for REFLAG_HANDONLY
	if (backEnd.currentEntity->e.reFlags & REFLAG_ONLYHAND)
	{
		if (!strstr(surface->name, "hand"))
		{
			return;
		}
	}

	if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame)
	{
		backlerp = 0;
	}
	else
	{
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW(surface->numVerts, surface->numTriangles * 3);

	LerpCMeshVertexes(surface, backlerp);

	triangles = ( int * )((byte *)surface + surface->ofsTriangles);
	indexes   = surface->numTriangles * 3;
	Bob       = tess.numIndexes;
	Doug      = tess.numVertexes;
	for (j = 0 ; j < indexes ; j++)
	{
		tess.indexes[Bob + j] = Doug + triangles[j];
	}
	tess.numIndexes += indexes;

	texCoords = ( float * )((byte *)surface + surface->ofsSt);

	numVerts = surface->numVerts;
	for (j = 0; j < numVerts; j++)
	{
		tess.texCoords[Doug + j][0][0] = texCoords[j * 2 + 0];
		tess.texCoords[Doug + j][0][1] = texCoords[j * 2 + 1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}

/**
 * @brief RB_SurfaceFace
 * @param[in] surf
 */
void RB_SurfaceFace(srfSurfaceFace_t *surf)
{
	int      i;
	unsigned *indices, *tessIndexes;
	float    *v;
	float    *normal;
	int      ndx;
	int      Bob;
	int      numPoints;
	int      dlightBits;

	RB_CHECKOVERFLOW(surf->numPoints, surf->numIndices);

	dlightBits       = surf->dlightBits;
	tess.dlightBits |= dlightBits;

	indices = ( unsigned * )((( char * ) surf) + surf->ofsIndices);

	Bob         = tess.numVertexes;
	tessIndexes = ( unsigned int * )( tess.indexes + tess.numIndexes);
	for (i = surf->numIndices - 1 ; i >= 0  ; i--)
	{
		tessIndexes[i] = indices[i] + Bob;
	}

	tess.numIndexes += surf->numIndices;

	numPoints = surf->numPoints;

	if (tess.shader->needsNormal)
	{
		normal = surf->plane.normal;
		for (i = 0, ndx = tess.numVertexes; i < numPoints; i++, ndx++)
		{
			VectorCopy(normal, tess.normal[ndx]);
		}
	}

	for (i = 0, v = surf->points[0], ndx = tess.numVertexes; i < numPoints; i++, v += VERTEXSIZE, ndx++)
	{
		VectorCopy(v, tess.xyz[ndx]);
		tess.texCoords[ndx][0][0]                   = v[3];
		tess.texCoords[ndx][0][1]                   = v[4];
		tess.texCoords[ndx][1][0]                   = v[5];
		tess.texCoords[ndx][1][1]                   = v[6];
		*( unsigned int * ) &tess.vertexColors[ndx] = *( unsigned int * ) &v[7];
	}

	tess.numVertexes += surf->numPoints;
}

/**
 * @brief LodErrorForVolume
 * @param[in] local
 * @param[in] radius
 * @return
 */
static float LodErrorForVolume(vec3_t local, float radius)
{
	vec3_t world;
	float  d;

	// never let it go lower than 1
	if (r_lodCurveError->value < 1)
	{
		return 1;
	}

	world[0] = local[0] * backEnd.orientation.axis[0][0] + local[1] * backEnd.orientation.axis[1][0] +
	           local[2] * backEnd.orientation.axis[2][0] + backEnd.orientation.origin[0];
	world[1] = local[0] * backEnd.orientation.axis[0][1] + local[1] * backEnd.orientation.axis[1][1] +
	           local[2] * backEnd.orientation.axis[2][1] + backEnd.orientation.origin[1];
	world[2] = local[0] * backEnd.orientation.axis[0][2] + local[1] * backEnd.orientation.axis[1][2] +
	           local[2] * backEnd.orientation.axis[2][2] + backEnd.orientation.origin[2];

	VectorSubtract(world, backEnd.viewParms.orientation.origin, world);
	d = DotProduct(world, backEnd.viewParms.orientation.axis[0]);

	if (d < 0)
	{
		d = -d;
	}
	d -= radius;
	if (d < 1)
	{
		d = 1;
	}

	return r_lodCurveError->value / d;
}

/**
 * @brief Just copy the grid of points and triangulate
 * @param[in] cv
 */
void RB_SurfaceGrid(srfGridMesh_t *cv)
{
	int           i, j;
	float         *xyz;
	float         *texCoords;
	float         *normal;
	unsigned char *color;
	drawVert_t    *dv;
	int           rows, irows, vrows;
	int           used;
	int           widthTable[MAX_GRID_SIZE];
	int           heightTable[MAX_GRID_SIZE];
	float         lodError;
	int           lodWidth = 1, lodHeight;
	int           numVertexes;
	int           dlightBits = cv->dlightBits;
	qboolean      needsNormal;

	tess.dlightBits |= dlightBits;

	// determine the allowable discrepance
	lodError = LodErrorForVolume(cv->lodOrigin, cv->lodRadius);

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[0] = 0;

	for (i = 1 ; i < cv->width - 1 ; i++)
	{
		if (cv->widthLodError[i] <= lodError)
		{
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = cv->width - 1;
	lodWidth++;

	heightTable[0] = 0;
	lodHeight      = 1;
	for (i = 1 ; i < cv->height - 1 ; i++)
	{
		if (cv->heightLodError[i] <= lodError)
		{
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = cv->height - 1;
	lodHeight++;

	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	used = 0;
	while (used < lodHeight - 1)
	{
		// see how many rows of both verts and indexes we can add without overflowing
		do
		{
			vrows = (SHADER_MAX_VERTEXES - tess.numVertexes) / lodWidth;
			irows = (SHADER_MAX_INDEXES - tess.numIndexes) / (lodWidth * 6);

			// if we don't have enough space for at least one strip, flush the buffer
			if (vrows < 2 || irows < 1)
			{
				RB_EndSurface();
				RB_BeginSurface(tess.shader, tess.fogNum);
				tess.dlightBits |= dlightBits;  // for proper dlighting
			}
			else
			{
				break;
			}
		}
		while (1);

		rows = irows;
		if (vrows < irows + 1)
		{
			rows = vrows - 1;
		}
		if (used + rows > lodHeight)
		{
			rows = lodHeight - used;
		}

		numVertexes = tess.numVertexes;

		xyz         = tess.xyz[numVertexes];
		normal      = tess.normal[numVertexes];
		texCoords   = tess.texCoords[numVertexes][0];
		color       = ( unsigned char * ) &tess.vertexColors[numVertexes];
		needsNormal = tess.shader->needsNormal;

		for (i = 0 ; i < rows ; i++)
		{
			for (j = 0 ; j < lodWidth ; j++)
			{
				dv = cv->verts + heightTable[used + i] * cv->width + widthTable[j];

				xyz[0]       = dv->xyz[0];
				xyz[1]       = dv->xyz[1];
				xyz[2]       = dv->xyz[2];
				texCoords[0] = dv->st[0];
				texCoords[1] = dv->st[1];
				texCoords[2] = dv->lightmap[0];
				texCoords[3] = dv->lightmap[1];
				if (needsNormal)
				{
					normal[0] = dv->normal[0];
					normal[1] = dv->normal[1];
					normal[2] = dv->normal[2];
				}
				*( unsigned int * ) color = *( unsigned int * ) dv->color;
				xyz                      += 4;
				normal                   += 4;
				texCoords                += 4;
				color                    += 4;
			}
		}

		// add the indexes
		{
			int numIndexes = tess.numIndexes;
			int v1, v2, v3, v4;
			int h = rows - 1;
			int w = lodWidth - 1;

			for (i = 0 ; i < h ; i++)
			{
				for (j = 0 ; j < w ; j++)
				{
					// vertex order to be reckognized as tristrips
					v1 = numVertexes + i * lodWidth + j + 1;
					v2 = v1 - 1;
					v3 = v2 + lodWidth;
					v4 = v3 + 1;

					tess.indexes[numIndexes]     = v2;
					tess.indexes[numIndexes + 1] = v3;
					tess.indexes[numIndexes + 2] = v1;

					tess.indexes[numIndexes + 3] = v1;
					tess.indexes[numIndexes + 4] = v3;
					tess.indexes[numIndexes + 5] = v4;
					numIndexes                  += 6;
				}
			}

			tess.numIndexes = numIndexes;
		}

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}

/*
===========================================================================
NULL MODEL
===========================================================================
*/

/**
 * @brief Draws x/y/z lines from the origin for orientation debugging
 */
void RB_SurfaceAxis(void)
{
	// OpenGLES implementation
	GL_Bind(tr.whiteImage);
	GL_State(GLS_DEFAULT);
	qglLineWidth(3);
	GLfloat col[] =
	{
		1, 0, 0, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		0, 0, 1, 1
	};
	GLfloat vtx[] =
	{
		0,  0,  0,
		16, 0,  0,
		0,  0,  0,
		0,  16, 0,
		0,  0,  0,
		0,  0,  16
	};
	GLboolean text  = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (text)
	{
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (!glcol)
	{
		qglEnableClientState(GL_COLOR_ARRAY);
	}
	qglColorPointer(4, GL_UNSIGNED_BYTE, 0, col);
	qglVertexPointer(3, GL_FLOAT, 0, vtx);
	qglDrawArrays(GL_LINES, 0, 6);
	if (text)
	{
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (!glcol)
	{
		qglDisableClientState(GL_COLOR_ARRAY);
	}
	qglLineWidth(1);
}

//===========================================================================

/**
 * @brief Entities that have a single procedurally generated surface
 * @param surfType - unused
 */
void RB_SurfaceEntity(surfaceType_t *surfType)
{
	switch (backEnd.currentEntity->e.reType)
	{
	case RT_SPLASH:
		RB_SurfaceSplash();
		break;
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;
	case RT_BEAM:
		RB_SurfaceBeam();
		break;
	case RT_RAIL_CORE:
		RB_SurfaceRailCore();
		break;
	case RT_RAIL_RINGS:
		RB_SurfaceRailRings();
		break;
	case RT_LIGHTNING:
		RB_SurfaceLightningBolt();
		break;
	default:
		RB_SurfaceAxis();
		break;
	}
}

/**
 * @brief RB_SurfaceBad
 * @param surfType - unused
 *
 * @todo better impl ?
 */
void RB_SurfaceBad(surfaceType_t *surfType)
{
	Ren_Print("Bad surface tesselated.\n");
}

#if 0

/**
 * @brief RB_SurfaceFlare
 * @param[in] surf
 */
void RB_SurfaceFlare(srfFlare_t *surf)
{
	vec3_t left, up;
	float  radius;
	byte   color[4];
	vec3_t dir;
	vec3_t origin;
	float  d;

	// calculate the xyz locations for the four corners
	radius = 30;
	VectorScale(backEnd.viewParms.orientation.axis[1], radius, left);
	VectorScale(backEnd.viewParms.orientation.axis[2], radius, up);
	if (backEnd.viewParms.isMirror)
	{
		VectorSubtract(vec3_origin, left, left);
	}

	color[0] = color[1] = color[2] = color[3] = 255;

	VectorMA(surf->origin, 3, surf->normal, origin);
	VectorSubtract(origin, backEnd.viewParms.orientation.origin, dir);
	VectorNormalize(dir);
	VectorMA(origin, r_ignore->value, dir, origin);

	d = -DotProduct(dir, surf->normal);
	if (d < 0)
	{
		return;
	}
#if 0
	color[0] *= d;
	color[1] *= d;
	color[2] *= d;
#endif

	RB_AddQuadStamp(origin, left, up, color);
}

#else
/**
 * @brief RB_SurfaceFlare
 * @param[in] surf
 */
void RB_SurfaceFlare(srfFlare_t *surf)
{
#if 0
	vec3_t left, up;
	byte   color[4];

	color[0] = surf->color[0] * 255;
	color[1] = surf->color[1] * 255;
	color[2] = surf->color[2] * 255;
	color[3] = 255;

	VectorClear(left);
	VectorClear(up);

	left[0] = r_ignore->value;

	up[1] = r_ignore->value;

	RB_AddQuadStampExt(surf->origin, left, up, color, 0, 0, 1, 1);
#endif
}
#endif

/**
 * @brief RB_SurfaceDisplayList
 * @param[in] surf
 */
void RB_SurfaceDisplayList(srfDisplayList_t *surf)
{
	// all apropriate state must be set in RB_BeginSurface
	// this isn't implemented yet...
}

/**
 * @brief RB_SurfacePolyBuffer
 * @param[in] surf
 */
void RB_SurfacePolyBuffer(srfPolyBuffer_t *surf)
{
	int i;
	int numv;

	RB_CHECKOVERFLOW(surf->pPolyBuffer->numVerts, surf->pPolyBuffer->numIndicies);

	numv = tess.numVertexes;
	for (i = 0; i < surf->pPolyBuffer->numVerts; i++)
	{
		VectorCopy(surf->pPolyBuffer->xyz[i], tess.xyz[numv]);
		tess.texCoords[numv][0][0]       = surf->pPolyBuffer->st[i][0];
		tess.texCoords[numv][0][1]       = surf->pPolyBuffer->st[i][1];
		*(int *)&tess.vertexColors[numv] = *(int *)surf->pPolyBuffer->color[i];

		numv++;
	}

	for (i = 0; i < surf->pPolyBuffer->numIndicies; i++)
	{
		tess.indexes[tess.numIndexes++] = tess.numVertexes + surf->pPolyBuffer->indicies[i];
	}

	tess.numVertexes = numv;
}

/**
 * @brief RB_SurfaceDecal
 * @param[in] srf
 */
void RB_SurfaceDecal(srfDecal_t *srf)
{
	int i;
	int numv;

	RB_CHECKOVERFLOW(srf->numVerts, 3 * (srf->numVerts - 2));

	// fan triangles into the tess array
	numv = tess.numVertexes;
	for (i = 0; i < srf->numVerts; i++)
	{
		VectorCopy(srf->verts[i].xyz, tess.xyz[numv]);
		tess.texCoords[numv][0][0]        = srf->verts[i].st[0];
		tess.texCoords[numv][0][1]        = srf->verts[i].st[1];
		*(int *) &tess.vertexColors[numv] = *(int *) srf->verts[i].modulate;
		numv++;
	}

	/* generate fan indexes into the tess array */
	for (i = 0; i < srf->numVerts - 2; i++)
	{
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes                  += 3;
	}

	tess.numVertexes = numv;
}

/**
 * @brief RB_SurfaceSkip
 * @param surf - dummy function for SF_SKIP
 */
void RB_SurfaceSkip(void *surf)
{
	return;
}

void(*rb_surfaceTable[SF_NUM_SURFACE_TYPES]) (void *) =
{
	(void (*)(void *))RB_SurfaceBad,               // SF_BAD,
	(void (*)(void *))RB_SurfaceSkip,              // SF_SKIP,
	(void (*)(void *))RB_SurfaceFace,              // SF_FACE,
	(void (*)(void *))RB_SurfaceGrid,              // SF_GRID,
	(void (*)(void *))RB_SurfaceTriangles,         // SF_TRIANGLES,
	(void (*)(void *))RB_SurfaceFoliage,           // SF_FOLIAGE,
	(void (*)(void *))RB_SurfacePolychain,         // SF_POLY,
	(void (*)(void *))RB_SurfaceMesh,              // SF_MD3,
	(void (*)(void *))RB_SurfaceCMesh,             // SF_MDC,
	(void (*)(void *))RB_SurfaceAnim,              // SF_MDS,
	(void (*)(void *))RB_MDM_SurfaceAnim,          // SF_MDM,
	(void (*)(void *))RB_SurfaceFlare,             // SF_FLARE,
	(void (*)(void *))RB_SurfaceEntity,            // SF_ENTITY
#ifndef __ANDROID__
	(void (*)(void *))RB_SurfaceDisplayList,       // SF_DISPLAY_LIST
#else
    NULL,                                          // NULL
#endif
	(void (*)(void *))RB_SurfacePolyBuffer,        // SF_POLYBUFFER
	(void (*)(void *))RB_SurfaceDecal,             // SF_DECAL
};
