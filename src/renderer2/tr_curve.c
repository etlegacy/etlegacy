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
 * @file renderer2/tr_curve.c
 *
 * @brief This file does all of the processing necessary to turn a raw grid of
 * points read from the map file into a srfGridMesh_t ready for rendering.
 *
 * The level of detail solution is direction independent, based only on
 * subdivided distance from the true curve.
 *
 * Only a single entry point:
 * R_SubdividePatchToGrid(int width, int height, srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE])
 */

#include "tr_local.h"
#include "../qcommon/q_shared.h"

#pragma warning(disable:4700)


/**
 * @brief LerpSurfaceVert
 * @param[in] a
 * @param[in] b
 * @param[out] out
 */
static void LerpSurfaceVert(srfVert_t *a, srfVert_t *b, srfVert_t *out)
{
#if 0
	out->xyz[0] = (a->xyz[0] + b->xyz[0]) * 0.5f;
	out->xyz[1] = (a->xyz[1] + b->xyz[1]) * 0.5f;
	out->xyz[2] = (a->xyz[2] + b->xyz[2]) * 0.5f;

	out->st[0] = (a->st[0] + b->st[0]) * 0.5f;
	out->st[1] = (a->st[1] + b->st[1]) * 0.5f;

	out->lightmap[0] = (a->lightmap[0] + b->lightmap[0]) * 0.5f;
	out->lightmap[1] = (a->lightmap[1] + b->lightmap[1]) * 0.5f;

	out->tangent[0] = (a->tangent[0] + b->tangent[0]) * 0.5f;
	out->tangent[1] = (a->tangent[1] + b->tangent[1]) * 0.5f;
	out->tangent[2] = (a->tangent[2] + b->tangent[2]) * 0.5f;

	out->binormal[0] = (a->binormal[0] + b->binormal[0]) * 0.5f;
	out->binormal[1] = (a->binormal[1] + b->binormal[1]) * 0.5f;
	out->binormal[2] = (a->binormal[2] + b->binormal[2]) * 0.5f;

	out->normal[0] = (a->normal[0] + b->normal[0]) * 0.5f;
	out->normal[1] = (a->normal[1] + b->normal[1]) * 0.5f;
	out->normal[2] = (a->normal[2] + b->normal[2]) * 0.5f;

	out->paintColor[0] = (a->paintColor[0] + b->paintColor[0]) * 0.5f;
	out->paintColor[1] = (a->paintColor[1] + b->paintColor[1]) * 0.5f;
	out->paintColor[2] = (a->paintColor[2] + b->paintColor[2]) * 0.5f;
	out->paintColor[3] = (a->paintColor[3] + b->paintColor[3]) * 0.5f;

	out->lightColor[0] = (a->lightColor[0] + b->lightColor[0]) * 0.5f;
	out->lightColor[1] = (a->lightColor[1] + b->lightColor[1]) * 0.5f;
	out->lightColor[2] = (a->lightColor[2] + b->lightColor[2]) * 0.5f;
	out->lightColor[3] = (a->lightColor[3] + b->lightColor[3]) * 0.5f;

	out->lightDirection[0] = (a->lightDirection[0] + b->lightDirection[0]) * 0.5f;
	out->lightDirection[1] = (a->lightDirection[1] + b->lightDirection[1]) * 0.5f;
	out->lightDirection[2] = (a->lightDirection[2] + b->lightDirection[2]) * 0.5f;
#else
	VectorAM(a->xyz, b->xyz, 0.5f, out->xyz);
	Vector2AM(a->st, b->st, 0.5f, out->st);
	Vector2AM(a->lightmap, b->lightmap, 0.5f, out->lightmap);
	VectorAM(a->tangent, b->tangent, 0.5f, out->tangent);
	VectorAM(a->binormal, b->binormal, 0.5f, out->binormal);
	VectorAM(a->normal, b->normal, 0.5f, out->normal);
	Vector4AM(a->paintColor, b->paintColor, 0.5f, out->paintColor);
	Vector4AM(a->lightColor, b->lightColor, 0.5f, out->lightColor);
	VectorAM(a->lightDirection, b->lightDirection, 0.5f, out->lightDirection);
#endif
}

/**
 * @brief Transpose
 * @param[in] width
 * @param[in] height
 * @param[in,out] ctrl
 */
static void Transpose(int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE])
{
	int       i, j;
	srfVert_t temp;

	if (width > height)
	{
		for (i = 0; i < height; i++)
		{
			for (j = i + 1; j < width; j++)
			{
				if (j < height)
				{
					// swap the value
					temp       = ctrl[j][i];
					ctrl[j][i] = ctrl[i][j];
					ctrl[i][j] = temp;
				}
				else
				{
					// just copy
					ctrl[j][i] = ctrl[i][j];
				}
			}
		}
	}
	else
	{
		for (i = 0; i < width; i++)
		{
			for (j = i + 1; j < height; j++)
			{
				if (j < width)
				{
					// swap the value
					temp       = ctrl[i][j];
					ctrl[i][j] = ctrl[j][i];
					ctrl[j][i] = temp;
				}
				else
				{
					// just copy
					ctrl[i][j] = ctrl[j][i];
				}
			}
		}
	}
}

/**
 * @brief Handles all the complicated wrapping and degenerate cases
 * @param[in] width
 * @param[in] height
 * @param[in,out] ctrl
 */
static void MakeMeshNormals(int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE])
{
	int        i, j, k, dist, k1_7, n;
	vec3_t     normal;
	vec3_t     sum;
	vec3_t     base;
	vec3_t     delta;
	int        x, y;
	srfVert_t  *dv;
	vec3_t     around[8], temp;
	qboolean   good[8];
	qboolean   wrapWidth, wrapHeight;
	//float      len;
	static int neighbors[8][2] =
	{
		{ 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 }, { -1, 0 }, { -1, 1 }
	};

	for (i = 0; i < height; i++)
	{
		VectorSubtract(ctrl[i][0].xyz, ctrl[i][width - 1].xyz, delta);
		//len = VectorLengthSquared(delta);
		//if (len > 1.0f)
		if (VectorLengthSquared(delta) > 1.0f)
		{
			break;
		}
	}
	wrapWidth = (qboolean)(i == height);

	for (i = 0; i < width; i++)
	{
		VectorSubtract(ctrl[0][i].xyz, ctrl[height - 1][i].xyz, delta);
		//len = VectorLengthSquared(delta);
		//if (len > 1.0f)
		if (VectorLengthSquared(delta) > 1.0f)
		{
			break;
		}
	}
	wrapHeight = (qboolean)(i == width);

	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			dv = &ctrl[j][i];
			VectorCopy(dv->xyz, base);
			for (k = 0; k < 8; k++)
			{
				VectorClear(around[k]);
				good[k] = qfalse;

				for (dist = 1; dist <= 3; dist++)
				{
					x = neighbors[k][0] * dist + i;
					y = neighbors[k][1] * dist + j;
					if (wrapWidth)
					{
						if (x < 0)
						{
							x = width - 1 + x;
						}
						else if (x >= width)
						{
							x = 1 + x - width;
						}
					}
					if (wrapHeight)
					{
						if (y < 0)
						{
							y = height - 1 + y;
						}
						else if (y >= height)
						{
							y = 1 + y - height;
						}
					}

					if (x < 0 || x >= width || y < 0 || y >= height)
					{
						break;  // edge of patch
					}
					VectorSubtract(ctrl[y][x].xyz, base, temp);
					if (VectorNormalize2(temp, temp) == 0.f)
					{
						continue;   // degenerate edge, get more dist
					}
					else
					{
						good[k] = qtrue;
						VectorCopy(temp, around[k]);
						break;  // good edge
					}
				}
			}

			VectorClear(sum);
			n = 0;
			for (k = 0; k < 8; k++)
			{
				k1_7 = (k + 1) & 7;
				if (!good[k] || !good[k1_7])
				{
					continue;   // didn't get two points
				}
				CrossProduct(around[k1_7], around[k], normal);
				// try not to normalize with a null vector
				if (normal[0]==0.f && normal[1] == 0.f && normal[2] == 0.f)
				{
					continue;
				}
				// this should never return a length 0, but we check anyway..
				if (VectorNormalize2(normal, normal) == 0.f)
				{
					continue;
				}
				VectorAdd(normal, sum, sum);
				n++;
			}
			//VectorNormalize2(sum, dv->normal);
			//VectorScale(sum, 1.0f / n, dv->normal); // we don't need to normalize again. we know exactly howmuch to scale, to get the average
			VectorScale(sum, rcp((float)n), dv->normal); // we don't need to normalize again. we know exactly howmuch to scale, to get the average
		}
	}
}

/**
 * @brief MakeMeshTangentVectors
 * @param[in] width
 * @param[in] height
 * @param[in] ctrl
 * @param[in] numTriangles
 * @param[in] triangles
 */
static void MakeMeshTangentVectors(int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], int numTriangles,
                                   srfTriangle_t triangles[SHADER_MAX_TRIANGLES])
{
	int              i, j, jwi, wh = width * height;
	srfVert_t        *dv[3];
	static srfVert_t ctrl2[MAX_GRID_SIZE * MAX_GRID_SIZE];
	srfTriangle_t    *tri;

	// FIXME: use more elegant way
	for (i = 0; i < width; i++)
	{
		for (j = 0, jwi = i; j < height; j++, jwi += width)
		{
			//dv[0]  = &ctrl2[j * width + i];
			//*dv[0] = ctrl[j][i];
			ctrl2[jwi] = ctrl[j][i];
		}
	}

	for (i = 0, tri = triangles; i < numTriangles; i++, tri++)
	{
		dv[0] = &ctrl2[tri->indexes[0]];
		dv[1] = &ctrl2[tri->indexes[1]];
		dv[2] = &ctrl2[tri->indexes[2]];

		R_CalcTangentVectors(dv);
	}

#if 0
	for (i = 0; i < (width * height); i++)
	{
		dv0 = &ctrl2[i];

		VectorNormalizeOnly(dv0->normal);
#if 0
		VectorNormalizeOnly(dv0->tangent);
		VectorNormalizeOnly(dv0->binormal);
#else
		//d = DotProduct(dv0->tangent, dv0->normal);
		Dot(dv0->tangent, dv0->normal, d);
		VectorMA(dv0->tangent, -d, dv0->normal, dv0->tangent);
		VectorNormalizeOnly(dv0->tangent);

		//d = DotProduct(dv0->binormal, dv0->normal);
		Dot(dv0->binormal, dv0->normal, d);
		VectorMA(dv0->binormal, -d, dv0->normal, dv0->binormal);
		VectorNormalizeOnly(dv0->binormal);
#endif
	}
#endif


	if (r_smoothNormals->integer & FLAGS_SMOOTH_MESH) // do another extra smoothing for normals to avoid flat shading
	{
		for (i = 0; i < wh; i++)
		{
			for (j = 0; j < wh; j++)
			{
				if (R_CompareVert(&ctrl2[i], &ctrl2[j], qfalse))
				{
					VectorAdd(ctrl2[i].normal, ctrl2[j].normal, ctrl2[i].normal);
				}
			}

			VectorNormalizeOnly(ctrl2[i].normal);
		}
	}

	for (i = 0; i < width; i++)
	{
		for (j = 0, jwi = i; j < height; j++, jwi += width)
		{
			//dv[0] = &ctrl2[j * width + i];
			dv[0] = &ctrl2[jwi];
			dv[1] = &ctrl[j][i];

			VectorCopy(dv[0]->tangent, dv[1]->tangent);
			VectorCopy(dv[0]->binormal, dv[1]->binormal);
		}
	}
}

/**
 * @brief MakeMeshTriangles
 * @param[in] width
 * @param[in] height
 * @param[in] ctrl
 * @param[out] triangles
 * @return
 */
static int MakeMeshTriangles(int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE],
                             srfTriangle_t triangles[SHADER_MAX_TRIANGLES])
{
	int              i, j;
	int              numTriangles = 0;
	static srfVert_t ctrl2[MAX_GRID_SIZE * MAX_GRID_SIZE];
	int              iwidth, iwj;
	int              v1, v2, v3, v4;

	for (i = 0, iwidth = 0; i < height; i++, iwidth += width)
	{
		for (j = 0, iwj = iwidth; j < width; j++, iwj++)
		{
			if (i < height-1 && j < width-1) {
				// vertex order to be reckognized as tristrips
				//v1 = i * width + j + 1;
				v1 = iwj + 1;
				v2 = v1 - 1;
				v3 = v2 + width;
				v4 = v3 + 1;

				triangles[numTriangles].indexes[0] = v2;
				triangles[numTriangles].indexes[1] = v3;
				triangles[numTriangles].indexes[2] = v1;
				numTriangles++;

				triangles[numTriangles].indexes[0] = v1;
				triangles[numTriangles].indexes[1] = v3;
				triangles[numTriangles].indexes[2] = v4;
				numTriangles++;
			}
			ctrl2[iwj] = ctrl[i][j];
		}
	}

	return numTriangles;
}

/*
 * @brief MakeTangentSpaces
 * @param[in] width
 * @param[in] height
 * @param[in] ctrl
 * @param[in] numTriangles
 * @param[out] triangles
 *
 * @note Unused
static void MakeTangentSpaces(int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], int numTriangles,
                              srfTriangle_t triangles[SHADER_MAX_TRIANGLES])
{
    int             i, j;
    float          *v;
    const float    *v0, *v1, *v2;
    const float    *t0, *t1, *t2;
    vec3_t          tangent;
    vec3_t          binormal;
    vec3_t          normal;
    vec_t           d;
    srfVert_t      *dv0, *dv1, *dv2;
    srfVert_t       ctrl2[MAX_GRID_SIZE * MAX_GRID_SIZE];
    srfTriangle_t  *tri;

    // FIXME: use more elegant way
    for(i = 0; i < width; i++)
    {
        for(j = 0; j < height; j++)
        {
            dv0 = &ctrl2[j * width + i];
            *dv0 = ctrl[j][i];
        }
    }

    for(i = 0; i < (width * height); i++)
    {
        dv0 = &ctrl2[i];

        VectorClear(dv0->tangent);
        VectorClear(dv0->binormal);
        VectorClear(dv0->normal);
    }

    for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
    {
        dv0 = &ctrl2[tri->indexes[0]];
        dv1 = &ctrl2[tri->indexes[1]];
        dv2 = &ctrl2[tri->indexes[2]];

        v0 = dv0->xyz;
        v1 = dv1->xyz;
        v2 = dv2->xyz;

        t0 = dv0->st;
        t1 = dv1->st;
        t2 = dv2->st;

        R_CalcTangentSpace2(tangent, binormal, normal, v0, v1, v2, t0, t1, t2);

        for(j = 0; j < 3; j++)
        {
            dv0 = &ctrl2[tri->indexes[j]];

            v = dv0->tangent;
            VectorAdd(v, tangent, v);

            v = dv0->binormal;
            VectorAdd(v, binormal, v);

            v = dv0->normal;
            VectorAdd(v, normal, v);
        }
    }

    for(i = 0; i < (width * height); i++)
    {
        dv0 = &ctrl2[i];

        VectorNormalizeOnly(dv0->normal);
#if 0
        VectorNormalizeOnly(dv0->tangent);
        VectorNormalizeOnly(dv0->binormal);
#else
        //d = DotProduct(dv0->tangent, dv0->normal);
		Dot(dv0->tangent, dv0->normal, d);
        VectorMA(dv0->tangent, -d, dv0->normal, dv0->tangent);
        VectorNormalizeOnly(dv0->tangent);

        //d = DotProduct(dv0->binormal, dv0->normal);
		Dot(dv0->binormal, dv0->normal, d);
        VectorMA(dv0->binormal, -d, dv0->normal, dv0->binormal);
        VectorNormalizeOnly(dv0->binormal);
#endif
    }

    if (r_smoothNormals->integer & FLAGS_SMOOTH_MESH) // do another extra smoothing for normals to avoid flat shading
    {
    	for(i = 0; i < (width * height); i++)
		{
			for(j = 0; j < (width * height); j++)
			{
				if(R_CompareVert(&ctrl2[i], &ctrl2[j], qfalse))
				{
					VectorAdd(ctrl2[i].normal, ctrl2[j].normal, ctrl2[i].normal);
				}
			}

			VectorNormalizeOnly(ctrl2[i].normal);
		}
    }

    for(i = 0; i < width; i++)
    {
        for(j = 0; j < height; j++)
        {
            dv0 = &ctrl2[j * width + i];
            dv1 = &ctrl[j][i];

            VectorCopy(dv0->tangent, dv1->tangent);
            VectorCopy(dv0->binormal, dv1->binormal);
            VectorCopy(dv0->normal, dv1->normal);
        }
    }
}
*/

/**
 * @brief InvertCtrl
 * @param[in] width
 * @param[in] height
 * @param[in,out] ctrl
 */
static void InvertCtrl(int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE])
{
	int       i, j, w1j, w2 = width / 2;
	srfVert_t temp;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < w2; j++)
		{
			w1j = width - 1 - j;
			temp = ctrl[i][j];
			ctrl[i][j] = ctrl[i][w1j];
			ctrl[i][w1j] = temp;
		}
	}
}

/**
 * @brief InvertErrorTable
 * @param[out] errorTable
 * @param[in] width
 * @param[in] height
 */
static void InvertErrorTable(float errorTable[2][MAX_GRID_SIZE], int width, int height)
{
	int   i, h1i;
	float copy[2][MAX_GRID_SIZE];

	Com_Memcpy(copy, errorTable, sizeof(copy));

	for (i = 0; i < width; i++)
	{
		errorTable[1][i] = copy[0][i];  //[width-1-i];
	}

	h1i = height - 1;
	for (i = 0; i < height; i++)
	{
		//errorTable[0][i] = copy[1][height - 1 - i];
		errorTable[0][i] = copy[1][h1i--];
	}
}

/**
 * @brief PutPointsOnCurve
 * @param[in] ctrl
 * @param[in] width
 * @param[in] height
 */
static void PutPointsOnCurve(srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], int width, int height)
{
	int       i, j;
	srfVert_t prev, next;

	for (i = 0; i < width; i++)
	{
		for (j = 1; j < height; j += 2)
		{
			LerpSurfaceVert(&ctrl[j][i], &ctrl[j + 1][i], &prev);
			LerpSurfaceVert(&ctrl[j][i], &ctrl[j - 1][i], &next);
			LerpSurfaceVert(&prev, &next, &ctrl[j][i]);
		}
	}

	for (j = 0; j < height; j++)
	{
		for (i = 1; i < width; i += 2)
		{
			LerpSurfaceVert(&ctrl[j][i], &ctrl[j][i + 1], &prev);
			LerpSurfaceVert(&ctrl[j][i], &ctrl[j][i - 1], &next);
			LerpSurfaceVert(&prev, &next, &ctrl[j][i]);
		}
	}
}

/**
 * @brief R_CreateSurfaceGridMesh
 * @param[in] width
 * @param[in] height
 * @param[in] ctrl
 * @param[out] errorTable
 * @param[in] numTriangles
 * @param[out] triangles
 * @return
 */
static srfGridMesh_t *R_CreateSurfaceGridMesh(int width, int height,
                                              srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE],
                                              float errorTable[2][MAX_GRID_SIZE],
                                              int numTriangles, srfTriangle_t triangles[SHADER_MAX_TRIANGLES])
{
	int           i, j, jwidth;
	unsigned int  size;
	srfVert_t     *vert;
	vec3_t        tmpVec;
	srfGridMesh_t *grid;

	// copy the results out to a grid
	size = sizeof(*grid);

	if (r_stitchCurves->integer)
	{
		grid = /*ri.Hunk_Alloc */ (srfGridMesh_t *)Com_Allocate(size);
		Com_Memset(grid, 0, size);

		grid->widthLodError = /*ri.Hunk_Alloc */ (float *)Com_Allocate(width * 4);
		Com_Memcpy(grid->widthLodError, errorTable[0], width * 4);

		grid->heightLodError = /*ri.Hunk_Alloc */ (float *)Com_Allocate(height * 4);
		Com_Memcpy(grid->heightLodError, errorTable[1], height * 4);

		grid->numTriangles = numTriangles;
		grid->triangles    = (srfTriangle_t *)Com_Allocate(grid->numTriangles * sizeof(srfTriangle_t));
		Com_Memcpy(grid->triangles, triangles, numTriangles * sizeof(srfTriangle_t));

		grid->numVerts = (width * height);
		grid->verts    = (srfVert_t *)Com_Allocate(grid->numVerts * sizeof(srfVert_t));
	}
	else
	{
		grid = (srfGridMesh_t *)ri.Hunk_Alloc(size, h_low);
		Com_Memset(grid, 0, size);

		grid->widthLodError = (float *)ri.Hunk_Alloc(width * 4, h_low);
		Com_Memcpy(grid->widthLodError, errorTable[0], width * 4);

		grid->heightLodError = (float *)ri.Hunk_Alloc(height * 4, h_low);
		Com_Memcpy(grid->heightLodError, errorTable[1], height * 4);

		grid->numTriangles = numTriangles;
		grid->triangles    = (srfTriangle_t *)ri.Hunk_Alloc(grid->numTriangles * sizeof(srfTriangle_t), h_low);
		Com_Memcpy(grid->triangles, triangles, numTriangles * sizeof(srfTriangle_t));

		grid->numVerts = (width * height);
		grid->verts    = (srfVert_t *)ri.Hunk_Alloc(grid->numVerts * sizeof(srfVert_t), h_low);
	}

	grid->width       = width;
	grid->height      = height;
	grid->surfaceType = SF_GRID;
	ClearBounds(grid->bounds[0], grid->bounds[1]);
	for (i = 0; i < width; i++)
	{
		for (j = 0, jwidth = i; j < height; j++, jwidth += width)
		{
			vert  = &grid->verts[jwidth]; // j * width + i
			*vert = ctrl[j][i];
			AddPointToBounds(vert->xyz, grid->bounds[0], grid->bounds[1]);
		}

	}

	// compute local origin and bounds
	VectorAdd(grid->bounds[0], grid->bounds[1], grid->origin);
	VectorScale(grid->origin, 0.5f, grid->origin);
	VectorSubtract(grid->bounds[0], grid->origin, tmpVec);
	grid->radius = VectorLength(tmpVec);

	VectorCopy(grid->origin, grid->lodOrigin);
	grid->lodRadius = grid->radius;

	return grid;
}

/**
 * @brief R_FreeSurfaceGridMesh
 * @param[in] grid
 */
void R_FreeSurfaceGridMesh(srfGridMesh_t *grid)
{
	Com_Dealloc(grid->widthLodError);
	Com_Dealloc(grid->heightLodError);
	Com_Dealloc(grid->triangles);
	Com_Dealloc(grid->verts);
	Com_Dealloc(grid);
}

/**
 * @brief R_SubdividePatchToGrid
 * @param[in] width
 * @param[in] height
 * @param[in] points
 * @return
 */
srfGridMesh_t *R_SubdividePatchToGrid(int width, int height, srfVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE])
{
	int                  i, j, k, jwidth, j1, j2, j3, j_1;
	srfVert_t            prev, next, mid;
	float                len, maxLen;
	int                  dir;
	int                  t;
	static srfVert_t     ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float                errorTable[2][MAX_GRID_SIZE];
	int                  numTriangles;
	static srfTriangle_t triangles[SHADER_MAX_TRIANGLES];

	for (i = 0; i < width; i++)
	{
		for (j = 0, jwidth = i; j < height; j++, jwidth += width)
		{
			ctrl[j][i] = points[jwidth]; // j * width + i
		}
	}

	for (dir = 0; dir < 2; dir++)
	{

		for (j = 0; j < MAX_GRID_SIZE; j++)
		{
			errorTable[dir][j] = 0;
		}

		// horizontal subdivisions
		for (j = 0; j + 2 < width; j += 2)
		{
			j1 = j + 1;
			j2 = j + 2;
			j3 = j + 3;
			// check subdivided midpoints against control points

			// FIXME: also check midpoints of adjacent patches against the control points
			// this would basically stitch all patches in the same LOD group together.

			maxLen = 0;
			for (i = 0; i < height; i++)
			{
				vec3_t midxyz;
				vec3_t midxyz2;
				vec3_t dir;
				vec3_t projected;
				float  d;

				// calculate the point on the curve
				//for (l = 0; l < 3; l++)
				//{
				//	midxyz[l] = (ctrl[i][j].xyz[l] + ctrl[i][j1].xyz[l] * 2 + ctrl[i][j2].xyz[l]) * 0.25f;
				//}
				VectorScale(ctrl[i][j1].xyz, 2.0f, midxyz);
				VectorAdd(midxyz, ctrl[i][j].xyz, midxyz);
				VectorAdd(midxyz, ctrl[i][j2].xyz, midxyz);
				VectorScale(midxyz, 0.25f, midxyz);

				// see how far off the line it is
				// using dist-from-line will not account for internal
				// texture warping, but it gives a lot less polygons than
				// dist-from-midpoint
				VectorSubtract(midxyz, ctrl[i][j].xyz, midxyz);
				VectorSubtract(ctrl[i][j2].xyz, ctrl[i][j].xyz, dir);
				VectorNormalizeOnly(dir);

				//d = DotProduct(midxyz, dir);
				Dot(midxyz, dir, d);
				VectorScale(dir, d, projected);
				VectorSubtract(midxyz, projected, midxyz2);
				len = VectorLengthSquared(midxyz2); // we will do the sqrt later
				if (len > maxLen)
				{
					maxLen = len;
				}
			}

			maxLen = sqrt(maxLen);

			// if all the points are on the lines, remove the entire columns
			if (maxLen < 0.1f)
			{
				errorTable[dir][j1] = 999.f;
				continue;
			}

			// see if we want to insert subdivided columns
			if (width + 2 > MAX_GRID_SIZE)
			{
				//errorTable[dir][j1] = 1.0f / maxLen;
				errorTable[dir][j1] = rcp(maxLen);
				continue;   // can't subdivide any more
			}

			if (maxLen <= r_subDivisions->value)
			{
				//errorTable[dir][j1] = 1.0f / maxLen;
				errorTable[dir][j1] = rcp(maxLen);
				continue;   // didn't need subdivision
			}

			//errorTable[dir][j2] = 1.0f / maxLen;
			errorTable[dir][j2] = rcp(maxLen);

			// insert two columns and replace the peak
			width += 2;
			for (i = 0; i < height; i++)
			{
				LerpSurfaceVert(&ctrl[i][j], &ctrl[i][j1], &prev);
				LerpSurfaceVert(&ctrl[i][j1], &ctrl[i][j2], &next);
				LerpSurfaceVert(&prev, &next, &mid);

				for (k = width - 1; k > j3; k--)
				{
					ctrl[i][k] = ctrl[i][k - 2];
				}
				ctrl[i][j1] = prev;
				ctrl[i][j2] = mid;
				ctrl[i][j3] = next;
			}

			// back up and recheck this set again, it may need more subdivision
			j -= 2;

		}

		Transpose(width, height, ctrl);
		t      = width;
		width  = height;
		height = t;
	}

	// put all the aproximating points on the curve
	PutPointsOnCurve(ctrl, width, height);

	// cull out any rows or columns that are colinear
	for (i = 1; i < width - 1; i++)
	{
		if (errorTable[0][i] != 999.f)
		{
			continue;
		}
		for (j = i + 1; j < width; j++)
		{
			j_1 = j - 1;
			for (k = 0; k < height; k++)
			{
				ctrl[k][j_1] = ctrl[k][j];
			}
			errorTable[0][j_1] = errorTable[0][j];
		}
		width--;
	}

	for (i = 1; i < height - 1; i++)
	{
		if (errorTable[1][i] != 999.f)
		{
			continue;
		}
		for (j = i + 1; j < height; j++)
		{
			j_1 = j - 1;
			for (k = 0; k < width; k++)
			{
				ctrl[j_1][k] = ctrl[j][k];
			}
			errorTable[1][j_1] = errorTable[1][j];
		}
		height--;
	}

#if 1
	// flip for longest tristrips as an optimization
	// the results should be visually identical with or
	// without this step
	if (height > width)
	{
		Transpose(width, height, ctrl);
		InvertErrorTable(errorTable, width, height);
		t      = width;
		width  = height;
		height = t;
		InvertCtrl(width, height, ctrl);
	}
#endif

	// calculate triangles
	numTriangles = MakeMeshTriangles(width, height, ctrl, triangles);

	// calculate normals
	MakeMeshNormals(width, height, ctrl);
	MakeMeshTangentVectors(width, height, ctrl, numTriangles, triangles);

	// calculate tangent spaces
	//MakeTangentSpaces(width, height, ctrl, numTriangles, triangles);

	return R_CreateSurfaceGridMesh(width, height, ctrl, errorTable, numTriangles, triangles);
}

/**
 * @brief R_GridInsertColumn
 * @param[in,out] grid
 * @param[in] column
 * @param[in] row
 * @param[in] point
 * @param[in] loderror
 * @return
 */
srfGridMesh_t *R_GridInsertColumn(srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror)
{
	int                  i, j, jgwi, jgwo;
	int                  width = grid->width + 1, height, oldwidth = 0;
	static srfVert_t     ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float                errorTable[2][MAX_GRID_SIZE];
	float                lodRadius;
	vec3_t               lodOrigin;
	int                  numTriangles;
	static srfTriangle_t triangles[SHADER_MAX_TRIANGLES];

	if (width > MAX_GRID_SIZE)
	{
		return NULL;
	}
	height = grid->height;
	for (i = 0; i < width; i++)
	{
		if (i == column)
		{
			//insert new column
			for (j = 0, jgwi = i; j < grid->height; j++, jgwi += grid->width)
			{
				//LerpSurfaceVert(&grid->verts[j * grid->width + i - 1], &grid->verts[j * grid->width + i], &ctrl[j][i]);
				LerpSurfaceVert(&grid->verts[jgwi - 1], &grid->verts[jgwi], &ctrl[j][i]);
				if (j == row)
				{
					VectorCopy(point, ctrl[j][i].xyz);
				}
			}
			errorTable[0][i] = loderror;
			continue;
		}
		errorTable[0][i] = grid->widthLodError[oldwidth];
		for (j = 0, jgwo = oldwidth; j < grid->height; j++, jgwo += grid->width)
		{
			//ctrl[j][i] = grid->verts[j * grid->width + oldwidth];
			ctrl[j][i] = grid->verts[jgwo];
		}
		oldwidth++;
	}
	for (j = 0; j < grid->height; j++)
	{
		errorTable[1][j] = grid->heightLodError[j];
	}

	// put all the aproximating points on the curve
	//PutPointsOnCurve( ctrl, width, height );

	// calculate triangles
	numTriangles = MakeMeshTriangles(width, height, ctrl, triangles);

	// calculate normals
	MakeMeshNormals(width, height, ctrl);
	MakeMeshTangentVectors(width, height, ctrl, numTriangles, triangles);

	// calculate tangent spaces
	//MakeTangentSpaces(width, height, ctrl, numTriangles, triangles);

	VectorCopy(grid->lodOrigin, lodOrigin);
	lodRadius = grid->lodRadius;
	// free the old grid
	R_FreeSurfaceGridMesh(grid);
	// create a new grid
	grid            = R_CreateSurfaceGridMesh(width, height, ctrl, errorTable, numTriangles, triangles);
	grid->lodRadius = lodRadius;
	VectorCopy(lodOrigin, grid->lodOrigin);
	return grid;
}

/**
 * @brief R_GridInsertRow
 * @param[in,out] grid
 * @param[in] row
 * @param[in] column
 * @param[in] point
 * @param[in] loderror
 * @return
 */
srfGridMesh_t *R_GridInsertRow(srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror)
{
	int                  i, j, i1gw, igw, ohgw;
	int                  width = grid->width, height = grid->height + 1, oldheight = 0;
	static srfVert_t     ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float                errorTable[2][MAX_GRID_SIZE];
	float                lodRadius;
	vec3_t               lodOrigin;
	int                  numTriangles;
	static srfTriangle_t triangles[SHADER_MAX_TRIANGLES];

	if (height > MAX_GRID_SIZE)
	{
		return NULL;
	}
	for (i = 0, igw = 0; i < height; i++, igw += grid->width)
	{
		if (i == row)
		{
			//insert new row
			i1gw = igw - grid->width; // (i - 1) * grid->width
			for (j = 0; j < grid->width; j++, igw++, i1gw++)
			{
				//LerpSurfaceVert(&grid->verts[(i - 1) * grid->width + j], &grid->verts[i * grid->width + j], &ctrl[i][j]);
				LerpSurfaceVert(&grid->verts[i1gw], &grid->verts[igw], &ctrl[i][j]);
				if (j == column)
				{
					VectorCopy(point, ctrl[i][j].xyz);
				}
			}
			errorTable[1][i] = loderror;
			continue;
		}
		errorTable[1][i] = grid->heightLodError[oldheight];
		ohgw = oldheight * grid->width;
		for (j = 0; j < grid->width; j++)
		{
			//ctrl[i][j] = grid->verts[oldheight * grid->width + j];
			ctrl[i][j] = grid->verts[ohgw + j];
		}
		oldheight++;
	}
	for (j = 0; j < grid->width; j++)
	{
		errorTable[0][j] = grid->widthLodError[j];
	}
	// put all the aproximating points on the curve
	//PutPointsOnCurve( ctrl, width, height );

	// calculate triangles
	numTriangles = MakeMeshTriangles(width, height, ctrl, triangles);

	// calculate normals
	MakeMeshNormals(width, height, ctrl);
	MakeMeshTangentVectors(width, height, ctrl, numTriangles, triangles);

	// calculate tangent spaces
	//MakeTangentSpaces(width, height, ctrl, numTriangles, triangles);

	VectorCopy(grid->lodOrigin, lodOrigin);
	lodRadius = grid->lodRadius;
	// free the old grid
	R_FreeSurfaceGridMesh(grid);
	// create a new grid
	grid            = R_CreateSurfaceGridMesh(width, height, ctrl, errorTable, numTriangles, triangles);
	grid->lodRadius = lodRadius;
	VectorCopy(lodOrigin, grid->lodOrigin);
	return grid;
}

#pragma warning(default:4700)
