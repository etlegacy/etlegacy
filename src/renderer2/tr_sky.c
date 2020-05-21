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
 * @file renderer2/tr_sky.c
 */

#include "tr_local.h"

#define SKY_SUBDIVISIONS        8
#define HALF_SKY_SUBDIVISIONS   (SKY_SUBDIVISIONS / 2)

static float s_cloudTexCoords[6][SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1][2];
static float s_cloudTexP[6][SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1];

/*
===================================================================================
POLYGON TO BOX SIDE PROJECTION
===================================================================================
*/

static vec3_t sky_clip[6] =
{
	{ 1,  1,  0 },
	{ 1,  -1, 0 },
	{ 0,  -1, 1 },
	{ 0,  1,  1 },
	{ 1,  0,  1 },
	{ -1, 0,  1 }
};

static float sky_mins[2][6], sky_maxs[2][6];
static float sky_min, sky_max;

/**
 * @brief AddSkyPolygon
 * @param[in] nump
 * @param[in] vecs
 */
static void AddSkyPolygon(int nump, vec3_t vecs)
{
	int    i, j;
	vec3_t v, av;
	float  s, t, dv;
	int    axis;
	float  *vp;

	// s = [0]/[2], t = [1]/[2]
	static int vec_to_st[6][3] =
	{
		{ -2, 3,  1  },
		{ 2,  3,  -1 },

		{ 1,  3,  2  },
		{ -1, 3,  -2 },

		{ -2, -1, 3  },
		{ -2, 1,  -3 }

		//  {-1,2,3},
		//  {1,2,-3}
	};

	// decide which face it maps to
	VectorCopy(vec3_origin, v);
	for (i = 0, vp = vecs; i < nump; i++, vp += 3)
	{
		VectorAdd(vp, v, v);
	}
	av[0] = Q_fabs(v[0]);
	av[1] = Q_fabs(v[1]);
	av[2] = Q_fabs(v[2]);
	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
		{
			axis = 1;
		}
		else
		{
			axis = 0;
		}
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
		{
			axis = 3;
		}
		else
		{
			axis = 2;
		}
	}
	else
	{
		if (v[2] < 0)
		{
			axis = 5;
		}
		else
		{
			axis = 4;
		}
	}

	// project new texture coords
	for (i = 0; i < nump; i++, vecs += 3)
	{
		j = vec_to_st[axis][2];
		if (j > 0)
		{
			dv = vecs[j - 1];
		}
		else
		{
			dv = -vecs[-j - 1];
		}
		if (dv < 0.001f)
		{
			continue;   // don't divide by zero
		}
		j = vec_to_st[axis][0];
		if (j < 0)
		{
			s = -vecs[-j - 1] / dv;
		}
		else
		{
			s = vecs[j - 1] / dv;
		}
		j = vec_to_st[axis][1];
		if (j < 0)
		{
			t = -vecs[-j - 1] / dv;
		}
		else
		{
			t = vecs[j - 1] / dv;
		}

		if (s < sky_mins[0][axis])
		{
			sky_mins[0][axis] = s;
		}
		if (t < sky_mins[1][axis])
		{
			sky_mins[1][axis] = t;
		}
		if (s > sky_maxs[0][axis])
		{
			sky_maxs[0][axis] = s;
		}
		if (t > sky_maxs[1][axis])
		{
			sky_maxs[1][axis] = t;
		}
	}
}

#define ON_EPSILON      0.1f    ///< point on plane side epsilon
#define MAX_CLIP_VERTS  64

/**
 * @brief ClipSkyPolygon
 * @param[in] nump
 * @param[in,out] vecs
 * @param[in] stage
 */
static void ClipSkyPolygon(int nump, vec3_t vecs, int stage)
{
	float    *norm;
	float    *v;
	qboolean front, back;
	float    d, e;
	float    dists[MAX_CLIP_VERTS];
	int      sides[MAX_CLIP_VERTS];
	vec3_t   newv[2][MAX_CLIP_VERTS];
	int      newc[2];
	int      i, j;

	if (nump > MAX_CLIP_VERTS - 2)
	{
		Ren_Drop("ClipSkyPolygon: MAX_CLIP_VERTS");
	}
	if (stage == 6)     // fully clipped, so draw it
	{
		AddSkyPolygon(nump, vecs);
		return;
	}

	front = back = qfalse;
	norm  = sky_clip[stage];
	for (i = 0, v = vecs; i < nump; i++, v += 3)
	{
		d = DotProduct(v, norm);
		if (d > ON_EPSILON)
		{
			front    = qtrue;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back     = qtrue;
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		dists[i] = d;
	}

	if (!front || !back)     // not clipped
	{
		ClipSkyPolygon(nump, vecs, stage + 1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy(vecs, (vecs + (i * 3)));
	newc[0] = newc[1] = 0;

	for (i = 0, v = vecs; i < nump; i++, v += 3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy(v, newv[0][newc[0]]);
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy(v, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy(v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy(v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
		{
			continue;
		}

		d = dists[i] / (dists[i] - dists[i + 1]);
		for (j = 0; j < 3; j++)
		{
			e                   = v[j] + d * (v[j + 3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon(newc[0], newv[0][0], stage + 1);
	ClipSkyPolygon(newc[1], newv[1][0], stage + 1);
}

/**
 * @brief ClearSkyBox
 */
static void ClearSkyBox(void)
{
	int i;

	for (i = 0; i < 6; i++)
	{
		sky_mins[0][i] = sky_mins[1][i] = 9999;
		sky_maxs[0][i] = sky_maxs[1][i] = -9999;
	}
}

/**
 * @brief Tess_ClipSkyPolygons
 */
void Tess_ClipSkyPolygons()
{
	vec3_t       p[5];          // need one extra point for clipping
	unsigned int i, j;

	ClearSkyBox();

	for (i = 0; i < tess.numIndexes; i += 3)
	{
		for (j = 0; j < 3; j++)
		{
			VectorSubtract(tess.xyz[tess.indexes[i + j]], backEnd.viewParms.orientation.origin, p[j]);
		}

		ClipSkyPolygon(3, p[0], 0);
	}
}

/*
===================================================================================
CLOUD VERTEX GENERATION
===================================================================================
*/

/**
 * @brief MakeSkyVec
 * @param[in] s range from -1 to 1
 * @param[in] t range from -1 to 1
 * @param[in] axis
 * @param[out] outSt
 * @param[out] outXYZ
 */
static void MakeSkyVec(float s, float t, int axis, vec4_t outSt, vec4_t outXYZ)
{
	// 1 = s, 2 = t, 3 = 2048
	static int st_to_vec[6][3] =
	{
		{ 3,  -1, 2 },
		{ -3, 1,  2 },

		{ 1,  3,  2 },
		{ -1, -3, 2 },

		{ -2, -1, 3 }, // 0 degrees yaw, look straight up
		{ 2,  -1, -3}   // look straight down
	};
	vec3_t b;
	int    j, k;
	float  boxSize;

	// merged from r1
	if (tr.glfogsettings[FOG_SKY].registered)         // trying this...
	{
		boxSize = tr.glfogsettings[FOG_SKY].end;       // trying this...
	}
	else
	{
		boxSize = backEnd.viewParms.zFar / 1.75f;    // div sqrt(3)
	}

	// make sure the sky is not near clipped
	if (boxSize < r_zNear->value * 2.0f)
	{
		boxSize = r_zNear->value * 2.0f;
	}
	// vanilla end

	b[0]    = s * boxSize;
	b[1]    = t * boxSize;
	b[2]    = boxSize;

	for (j = 0; j < 3; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
		{
			outXYZ[j] = -b[-k - 1];
		}
		else
		{
			outXYZ[j] = b[k - 1];
		}
	}
	outXYZ[3] = 1;

	// avoid bilerp seam
	s = (s + 1) * 0.5f;
	t = (t + 1) * 0.5f;
	if (s < sky_min)
	{
		s = sky_min;
	}
	else if (s > sky_max)
	{
		s = sky_max;
	}

	if (t < sky_min)
	{
		t = sky_min;
	}
	else if (t > sky_max)
	{
		t = sky_max;
	}

	t = 1.0f - t;


	if (outSt)
	{
		outSt[0] = s;
		outSt[1] = t;
		outSt[2] = 0;
		outSt[3] = 1;
	}
}

//static int    sky_texorder[6] = { 0, 2, 1, 3, 4, 5 };
static vec4_t s_skyPoints[SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1];
static float  s_skyTexCoords[SKY_SUBDIVISIONS + 1][SKY_SUBDIVISIONS + 1][4];

/**
 * @brief DrawSkySide
 * @param[in] image
 * @param[in] mins
 * @param[in] maxs
 */
static void DrawSkySide(struct image_s *image, const int mins[2], const int maxs[2]) // uses WT_EDGE_CLAMP
{
    int             s, t;
	int vertexStart = tess.numVertexes;
	int tHeight     = maxs[1] - mins[1] + 1;
	int sWidth      = maxs[0] - mins[0] + 1;

	GL_SelectTexture(0);
	GL_Bind(image);
	GL_Cull(CT_TWO_SIDED);

	for (t = mins[1] + HALF_SKY_SUBDIVISIONS; t <= maxs[1] + HALF_SKY_SUBDIVISIONS; t++)
	{
		for (s = mins[0] + HALF_SKY_SUBDIVISIONS; s <= maxs[0] + HALF_SKY_SUBDIVISIONS; s++)
		{
			VectorAdd(s_skyPoints[t][s], backEnd.viewParms.orientation.origin, tess.xyz[tess.numVertexes]);
			tess.xyz[tess.numVertexes][3] = 1;

			tess.texCoords[tess.numVertexes][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][1] = s_skyTexCoords[t][s][1];
			tess.texCoords[tess.numVertexes][2] = 0;
			tess.texCoords[tess.numVertexes][3] = 1;

			tess.numVertexes++;

			if (tess.numVertexes >= SHADER_MAX_VERTEXES)
			{
				Ren_Drop("SHADER_MAX_VERTEXES hit in FillCloudySkySide()\n");
			}
		}
	}

	for (t = 0; t < tHeight - 1; t++)
	{
		for (s = 0; s < sWidth - 1; s++)
		{
			tess.indexes[tess.numIndexes] = vertexStart + s + t * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + (t + 1) * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * (sWidth);
			tess.numIndexes++;

			tess.indexes[tess.numIndexes] = vertexStart + s + (t + 1) * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + 1 + (t + 1) * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * (sWidth);
			tess.numIndexes++;
		}
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD;
}

/**
 * @brief DrawSkySideInner
 * @param[in] image
 * @param[in] mins
 * @param[in] maxs
 */
static void DrawSkySideInner(struct image_s *image, const int mins[2], const int maxs[2])  // uses WT_REPEAT
{
    int s, t;
	int vertexStart = tess.numVertexes;
	int tHeight     = maxs[1] - mins[1] + 1;
	int sWidth      = maxs[0] - mins[0] + 1;

	GL_SelectTexture(0);
	GL_Bind(image);

	for (t = mins[1] + HALF_SKY_SUBDIVISIONS; t <= maxs[1] + HALF_SKY_SUBDIVISIONS; t++)
	{
		for (s = mins[0] + HALF_SKY_SUBDIVISIONS; s <= maxs[0] + HALF_SKY_SUBDIVISIONS; s++)
		{
			VectorAdd(s_skyPoints[t][s], backEnd.viewParms.orientation.origin, tess.xyz[tess.numVertexes]);
			tess.xyz[tess.numVertexes][3] = 1;

			tess.texCoords[tess.numVertexes][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][1] = s_skyTexCoords[t][s][1];
			tess.texCoords[tess.numVertexes][2] = s_skyTexCoords[t][s][2];
			tess.texCoords[tess.numVertexes][3] = s_skyTexCoords[t][s][3];

			tess.numVertexes++;

			if (tess.numVertexes >= SHADER_MAX_VERTEXES)
			{
				Ren_Drop("SHADER_MAX_VERTEXES hit in FillCloudySkySide()\n");
			}
		}
	}

	for (t = 0; t < tHeight - 1; t++)
	{
		for (s = 0; s < sWidth - 1; s++)
		{
			tess.indexes[tess.numIndexes] = vertexStart + s + t * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + (t + 1) * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * (sWidth);
			tess.numIndexes++;

			tess.indexes[tess.numIndexes] = vertexStart + s + (t + 1) * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + 1 + (t + 1) * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * (sWidth);
			tess.numIndexes++;
		}
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD;
}

/**
 * @brief FillCloudySkySide
 * @param[in] mins
 * @param[in] maxs
 * @param[in] addIndexes
 */
static void FillCloudySkySide(const int mins[2], const int maxs[2], qboolean addIndexes)
{
	int s, t;
	int vertexStart = tess.numVertexes;
	int tHeight     = maxs[1] - mins[1] + 1;
	int sWidth      = maxs[0] - mins[0] + 1;

	for (t = mins[1] + HALF_SKY_SUBDIVISIONS; t <= maxs[1] + HALF_SKY_SUBDIVISIONS; t++)
	{
		for (s = mins[0] + HALF_SKY_SUBDIVISIONS; s <= maxs[0] + HALF_SKY_SUBDIVISIONS; s++)
		{
			VectorAdd(s_skyPoints[t][s], backEnd.viewParms.orientation.origin, tess.xyz[tess.numVertexes]);
			tess.xyz[tess.numVertexes][3] = 1;

			tess.texCoords[tess.numVertexes][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][1] = s_skyTexCoords[t][s][1];
			tess.texCoords[tess.numVertexes][2] = 0;
			tess.texCoords[tess.numVertexes][3] = 1;

			tess.numVertexes++;

			if (tess.numVertexes >= SHADER_MAX_VERTEXES)
			{
				Ren_Drop("SHADER_MAX_VERTEXES hit in FillCloudySkySide()\n");
			}
		}
	}

	for (t = 0; t < tHeight - 1; t++)
	{
		for (s = 0; s < sWidth - 1; s++)
		{
			tess.indexes[tess.numIndexes] = vertexStart + s + t * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + (t + 1) * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * (sWidth);
			tess.numIndexes++;

			tess.indexes[tess.numIndexes] = vertexStart + s + (t + 1) * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + 1 + (t + 1) * (sWidth);
			tess.numIndexes++;
			tess.indexes[tess.numIndexes] = vertexStart + s + 1 + t * (sWidth);
			tess.numIndexes++;
		}
	}

	tess.attribsSet |= ATTR_POSITION | ATTR_TEXCOORD;
}

/**
 * @brief DrawSkyBox
 * @param shader
 */
static void DrawSkyBox(shader_t *shader, qboolean outerbox)
{
	int i;
	int sky_mins_subd[2], sky_maxs_subd[2];
	int s, t;

	sky_min = 0;
	sky_max = 1;

	Com_Memset(s_skyTexCoords, 0, sizeof(s_skyTexCoords));

	// set up for drawing
	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;

	GL_State(GLS_DEFAULT);

	for (i = 0; i < 6; i++)
	{
		sky_mins[0][i] = floor(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floor(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceil(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceil(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;

		if ((sky_mins[0][i] >= sky_maxs[0][i]) || (sky_mins[1][i] >= sky_maxs[1][i]))
		{
			continue;
		}

		sky_mins_subd[0] = sky_mins[0][i] * HALF_SKY_SUBDIVISIONS;
		sky_mins_subd[1] = sky_mins[1][i] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[0] = sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS;
		sky_maxs_subd[1] = sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS;

		if (sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS)
		{
			sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		}
		else if (sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS)
		{
			sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
		}
		if (sky_mins_subd[1] < -HALF_SKY_SUBDIVISIONS)
		{
			sky_mins_subd[1] = -HALF_SKY_SUBDIVISIONS;
		}
		else if (sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS)
		{
			sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;
		}

		if (sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS)
		{
			sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		}
		else if (sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS)
		{
			sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
		}
		if (sky_maxs_subd[1] < -HALF_SKY_SUBDIVISIONS)
		{
			sky_maxs_subd[1] = -HALF_SKY_SUBDIVISIONS;
		}
		else if (sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS)
		{
			sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;
		}

		// iterate through the subdivisions
		for (t = sky_mins_subd[1] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1] + HALF_SKY_SUBDIVISIONS; t++)
		{
			for (s = sky_mins_subd[0] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0] + HALF_SKY_SUBDIVISIONS; s++)
			{
				MakeSkyVec((s - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS,
				           (t - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS,
				           i, 
						   s_skyTexCoords[t][s], 
						   s_skyPoints[t][s]);
			}
		}

		if (outerbox)
		{
			DrawSkySide(shader->sky.outerbox, sky_mins_subd, sky_maxs_subd);
		}
		else
		{
			DrawSkySideInner(shader->sky.innerbox, sky_mins_subd, sky_maxs_subd);
		}
	}

	Tess_UpdateVBOs(tess.attribsSet);
	Tess_DrawElements();
}

/**
 * @brief FillCloudBox
 * @param shader - unused
 * @param[in] stage
 */
static void FillCloudBox(const shader_t *shader, int stage)
{
	int i;

	for (i = 0; i < 6; i++)
	{
		int   sky_mins_subd[2], sky_maxs_subd[2];
		int   s, t;
		float MIN_T;

		if (1)     // FIXME? shader->sky.fullClouds )
		{
			MIN_T = -HALF_SKY_SUBDIVISIONS;

			// still don't want to draw the bottom, even if fullClouds
			if (i == 5)
			{
				continue;
			}
		}
		else
		{
			switch (i)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				MIN_T = -1;
				break;
			case 5:
				// don't draw clouds beneath you
				continue;
			case 4:     // top
			default:
				MIN_T = -HALF_SKY_SUBDIVISIONS;
				break;
			}
		}

		sky_mins[0][i] = floor(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_mins[1][i] = floor(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[0][i] = ceil(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
		sky_maxs[1][i] = ceil(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;

		if ((sky_mins[0][i] >= sky_maxs[0][i]) || (sky_mins[1][i] >= sky_maxs[1][i]))
		{
			continue;
		}

		sky_mins_subd[0] = Q_ftol(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_mins_subd[1] = Q_ftol(sky_mins[1][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[0] = Q_ftol(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS);
		sky_maxs_subd[1] = Q_ftol(sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS);

		if (sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS)
		{
			sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
		}
		else if (sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS)
		{
			sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
		}
		if (sky_mins_subd[1] < MIN_T)
		{
			sky_mins_subd[1] = MIN_T;
		}
		else if (sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS)
		{
			sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;
		}

		if (sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS)
		{
			sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
		}
		else if (sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS)
		{
			sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
		}
		if (sky_maxs_subd[1] < MIN_T)
		{
			sky_maxs_subd[1] = MIN_T;
		}
		else if (sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS)
		{
			sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;
		}

		// iterate through the subdivisions
		for (t = sky_mins_subd[1] + HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1] + HALF_SKY_SUBDIVISIONS; t++)
		{
			for (s = sky_mins_subd[0] + HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0] + HALF_SKY_SUBDIVISIONS; s++)
			{
				MakeSkyVec((s - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS,
				           (t - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS,
				           i,
				           NULL,
				           s_skyPoints[t][s]);

				s_skyTexCoords[t][s][0] = s_cloudTexCoords[i][t][s][0];
				s_skyTexCoords[t][s][1] = s_cloudTexCoords[i][t][s][1];
			}
		}

		FillCloudySkySide(sky_mins_subd, sky_maxs_subd, stage == 0 ? qtrue : qfalse); // clouds
	}
}

/**
 * @brief BuildCloudData
 */
static void BuildCloudData()
{
#ifdef ETLEGACY_DEBUG
	shader_t *shader = tess.surfaceShader;

	etl_assert(shader->isSky);
#endif

	sky_min = 1.0 / 256.0;     // FIXME: not correct?
	sky_max = 255.0 / 256.0;

	// set up for drawing
	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;

	if (tess.surfaceShader->sky.cloudHeight != 0.f)
	{
		// FIXME: ok, this is really wierd. it's iterating through shader stages here,
		// which is unecessary for a multi-stage sky shader, as far as i can tell
		// nuking this
#if 0
		int i;

		for (i = 0; i < MAX_SHADER_STAGES; i++)
		{
			if (!tess.surfaceStages[i])
			{
				break;
			}

			FillCloudBox(tess.surfaceShader, i);
		}
#else
		FillCloudBox(tess.surfaceShader, 0);
#endif
	}

	Tess_UpdateVBOs(tess.attribsSet);
}

/**
 * @brief Called when a sky shader is parsed
 * @param[in] heightCloud
 */
void R_InitSkyTexCoords(float heightCloud)
{
	int    i, s, t;
	float  radiusWorld = 4096;
	float  p;
	float  sRad, tRad;
	vec4_t skyVec;
	vec3_t v;

	// init zfar so MakeSkyVec works even though
	// a world hasn't been bounded
	backEnd.viewParms.zFar = 1024;

	for (i = 0; i < 6; i++)
	{
		for (t = 0; t <= SKY_SUBDIVISIONS; t++)
		{
			for (s = 0; s <= SKY_SUBDIVISIONS; s++)
			{
				// compute vector from view origin to sky side integral point
				MakeSkyVec((s - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS,
				           (t - HALF_SKY_SUBDIVISIONS) / (float)HALF_SKY_SUBDIVISIONS,
				           i,
				           NULL,
				           skyVec);

				// compute parametric value 'p' that intersects with cloud layer
				p = (1.0f / (2 * DotProduct(skyVec, skyVec))) *
				    (-2 * skyVec[2] * radiusWorld +
				     2 * sqrt(Square(skyVec[2]) * Square(radiusWorld) +
				              2 * Square(skyVec[0]) * radiusWorld * heightCloud +
				              Square(skyVec[0]) * Square(heightCloud) +
				              2 * Square(skyVec[1]) * radiusWorld * heightCloud +
				              Square(skyVec[1]) * Square(heightCloud) +
				              2 * Square(skyVec[2]) * radiusWorld * heightCloud +
				              Square(skyVec[2]) * Square(heightCloud)));

				s_cloudTexP[i][t][s] = p;

				// compute intersection point based on p
				VectorScale(skyVec, p, v);
				v[2] += radiusWorld;

				// compute vector from world origin to intersection point 'v'
				VectorNormalize(v);

				sRad = Q_acos(v[0]);
				tRad = Q_acos(v[1]);

				s_cloudTexCoords[i][t][s][0] = sRad;
				s_cloudTexCoords[i][t][s][1] = tRad;
			}
		}
	}
}

//======================================================================================

/**
 * @brief RB_DrawSun
 */
void RB_DrawSun(void)
{
	float  size;
	float  dist;
	vec3_t origin, vec1, vec2;
	mat4_t transformMatrix;
	mat4_t modelViewMatrix;

	if (!tr.sunShader)
	{
		return;
	}

	if (!backEnd.skyRenderedThisView)
	{
		return;
	}
	if (!r_drawSun->integer)
	{
		return;
	}

	GL_PushMatrix();

	mat4_reset_translate_vec3(transformMatrix, backEnd.viewParms.orientation.origin);
	mat4_mult(backEnd.viewParms.world.viewMatrix, transformMatrix, modelViewMatrix);

	GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);
	GL_LoadModelViewMatrix(modelViewMatrix);

	dist = backEnd.viewParms.zFar / 1.75f; // div sqrt(3)
	// shrunk the size of the sun
	size = dist * 0.2f;

	VectorScale(tr.sunDirection, dist, origin);
	PerpendicularVector(vec1, tr.sunDirection);
	CrossProduct(tr.sunDirection, vec1, vec2);

	VectorScale(vec1, size, vec1);
	VectorScale(vec2, size, vec2);

	// farthest depth range
	glDepthRange(1.0, 1.0);
	Tess_Begin(Tess_StageIteratorGeneric, NULL, tr.sunShader, NULL, tess.skipTangentSpaces, qfalse, -1, tess.fogNum);
	Tess_AddQuadStamp(origin, vec1, vec2, colorWhite);
	Tess_End();

	// back to normal depth range
	glDepthRange(0.0, 1.0);

	GL_PopMatrix();
}

/**
 * @brief All of the visible sky triangles are in tess
 * Other things could be stuck in here, like birds in the sky, etc
 */
void Tess_StageIteratorSky(void)
{
	Ren_LogComment("--- Tess_StageIteratorSky( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
	               tess.numVertexes, tess.numIndexes / 3);

	if (r_fastSky->integer)
	{
		return;
	}

	// when portal sky exists, only render skybox for the portal sky scene
	if (tr.world && tr.world->hasSkyboxPortal && !(backEnd.refdef.rdflags & RDF_SKYBOXPORTAL))
	{
		return;
	}

	// does the current fog require fastsky?
	if (backEnd.viewParms.glFog.registered)
	{
		if (!backEnd.viewParms.glFog.drawsky)
		{
			return;
		}
	}
	else if (tr.glfogNum > FOG_NONE)
	{
		if (!tr.glfogsettings[FOG_CURRENT].drawsky)
		{
			return;
		}
	}

	backEnd.refdef.rdflags |= RDF_DRAWINGSKY;

	// go through all the polygons and project them onto
	// the sky box to see which blocks on each side need
	// to be drawn
	Tess_ClipSkyPolygons();

	// r_showSky will let all the sky blocks be drawn in
	// front of everything to allow developers to see how
	// much sky is getting sucked in
	if (r_showSky->integer)
	{
		glDepthRange(0.0, 0.0);
	}
	else
	{
		glDepthRange(1.0, 1.0);
	}

	// draw the outer skybox
	if (tess.surfaceShader->sky.outerbox && tess.surfaceShader->sky.outerbox != tr.blackCubeImage)
	{
		GL_Cull(CT_FRONT_SIDED);
		R_BindVBO(tess.vbo);
		R_BindIBO(tess.ibo);

		SetMacrosAndSelectProgram(trProg.gl_skyboxShader, USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal);

		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);   // in world space
		SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		// u_PortalPlane
		if (backEnd.viewParms.isPortal)
		{
			// clipping plane in world space
			clipPortalPlane();
		}

		GLSL_SetRequiredVertexPointers(trProg.gl_skyboxShader);

		DrawSkyBox(tess.surfaceShader, qtrue);
	}

	// generate the vertexes for all the clouds, which will be drawn
	// by the generic shader routine
	BuildCloudData();

	Tess_StageIteratorGeneric();

	// draw the inner skybox
	if (tess.surfaceShader->sky.innerbox && tess.surfaceShader->sky.innerbox != tr.blackCubeImage)
	{
		R_BindVBO(tess.vbo);
		R_BindIBO(tess.ibo);

		SetMacrosAndSelectProgram(trProg.gl_skyboxShader, USE_PORTAL_CLIPPING, backEnd.viewParms.isPortal);

		SetUniformVec3(UNIFORM_VIEWORIGIN, backEnd.viewParms.orientation.origin);   // in world space
		SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
		SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

		// u_PortalPlane
		if (backEnd.viewParms.isPortal)
		{
			// clipping plane in world space
			clipPortalPlane();
		}

		GLSL_SetRequiredVertexPointers(trProg.gl_skyboxShader);

		DrawSkyBox(tess.surfaceShader, qfalse);
	}
	
	// back to normal depth range
	glDepthRange(0.0, 1.0);

	// note that sky was drawn so we will draw a sun later
	backEnd.skyRenderedThisView = qtrue;
	backEnd.refdef.rdflags &= ~RDF_DRAWINGSKY;
}
