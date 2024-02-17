/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file renderer/tr_shadows.c
 */

#include "tr_local.h"

/*
  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]
*/

typedef struct
{
	int i2;
	int facing;
} edgeDef_t;

#define MAX_EDGE_DEFS   32

static edgeDef_t edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static int       numEdgeDefs[SHADER_MAX_VERTEXES];
static int       facing[SHADER_MAX_INDEXES / 3];

/**
 * @brief R_AddEdgeDef
 * @param[in] i1
 * @param[in] i2
 * @param[in] facing
 */
void R_AddEdgeDef(int i1, int i2, int facing)
{
	int c = numEdgeDefs[i1];

	if (c == MAX_EDGE_DEFS)
	{
		return;     // overflow
	}
	edgeDefs[i1][c].i2     = i2;
	edgeDefs[i1][c].facing = facing;

	numEdgeDefs[i1]++;
}

/**
 * @brief R_RenderShadowEdges
 */
void R_RenderShadowEdges(void)
{
	int i;
	int c, c2;
	int j, k;
	int i2;
	// int c_edges = 0, c_rejected = 0;  // TODO: remove ?
	int hit[2];

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges

	for (i = 0 ; i < tess.numVertexes ; i++)
	{
		c = numEdgeDefs[i];
		for (j = 0 ; j < c ; j++)
		{
			if (!edgeDefs[i][j].facing)
			{
				continue;
			}

			hit[0] = 0;
			hit[1] = 0;

			i2 = edgeDefs[i][j].i2;
			c2 = numEdgeDefs[i2];
			for (k = 0 ; k < c2 ; k++)
			{
				if (edgeDefs[i2][k].i2 == i)
				{
					hit[edgeDefs[i2][k].facing]++;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if (hit[1] == 0)
			{
				glBegin(GL_TRIANGLE_STRIP);
				glVertex3fv(tess.xyz[i]);
				glVertex3fv(tess.xyz[i + tess.numVertexes]);
				glVertex3fv(tess.xyz[i2]);
				glVertex3fv(tess.xyz[i2 + tess.numVertexes]);
				glEnd();
				// c_edges++;
			}
			/*
			else
			{
			    c_rejected++;
			}
			*/
		}
	}
}

/**
 * @brief RB_ShadowTessEnd
 *
 * @note triangleFromEdge[ v1 ][ v2 ]
 *
 * set triangle from edge( v1, v2, tri )
 * if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
 * }
 */
void RB_ShadowTessEnd(void)
{
	int    i;
	int    numTris;
	vec3_t lightDir;

	// we can only do this if we have enough space in the vertex buffers
	if (tess.numVertexes >= SHADER_MAX_VERTEXES / 2)
	{
		return;
	}

	if (glConfig.stencilBits < 4)
	{
		return;
	}

	VectorCopy(backEnd.currentEntity->lightDir, lightDir);

	// project vertexes away from light direction
	for (i = 0 ; i < tess.numVertexes ; i++)
	{
		VectorMA(tess.xyz[i], -512, lightDir, tess.xyz[i + tess.numVertexes]);
	}

	// decide which triangles face the light
	Com_Memset(numEdgeDefs, 0, 4 * tess.numVertexes);

	numTris = tess.numIndexes / 3;

	{
		int    i1, i2, i3;
		vec3_t d1, d2, normal;
		float  *v1, *v2, *v3;
		float  d;

		for (i = 0 ; i < numTris ; i++)
		{
			i1 = tess.indexes[i * 3 + 0];
			i2 = tess.indexes[i * 3 + 1];
			i3 = tess.indexes[i * 3 + 2];

			v1 = tess.xyz[i1];
			v2 = tess.xyz[i2];
			v3 = tess.xyz[i3];

			VectorSubtract(v2, v1, d1);
			VectorSubtract(v3, v1, d2);
			CrossProduct(d1, d2, normal);

			d = DotProduct(normal, lightDir);
			if (d > 0)
			{
				facing[i] = 1;
			}
			else
			{
				facing[i] = 0;
			}

			// create the edges
			R_AddEdgeDef(i1, i2, facing[i]);
			R_AddEdgeDef(i2, i3, facing[i]);
			R_AddEdgeDef(i3, i1, facing[i]);
		}
	}


	// draw the silhouette edges

	GL_Bind(tr.whiteImage);
	glEnable(GL_CULL_FACE);
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);
	glColor3f(0.2f, 0.2f, 0.2f);

	// don't write to the color buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 255);

	// mirrors have the culling order reversed
	if (backEnd.viewParms.isMirror)
	{
		glCullFace(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

		R_RenderShadowEdges();

		glCullFace(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

		R_RenderShadowEdges();
	}
	else
	{
		glCullFace(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

		R_RenderShadowEdges();

		glCullFace(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

		R_RenderShadowEdges();
	}

	// reenable writing to the color buffer
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

/**
 * @brief Darken everything that is is a shadow volume.
 *
 * @details We have to delay this until everything has been shadowed,
 * because otherwise shadows from different body parts would
 * overlap and double darken.
 */
void RB_ShadowFinish(void)
{
	if (r_shadows->integer != 2)
	{
		return;
	}
	if (glConfig.stencilBits < 4)
	{
		return;
	}
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0, 255);

	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CULL_FACE);

	GL_Bind(tr.whiteImage);

	glLoadIdentity();

	glColor3f(0.6f, 0.6f, 0.6f);
	GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);

	glBegin(GL_QUADS);
	glVertex3f(-100, 100, -10);
	glVertex3f(100, 100, -10);
	glVertex3f(100, -100, -10);
	glVertex3f(-100, -100, -10);
	glEnd();

	glColor4f(1, 1, 1, 1);
	glDisable(GL_STENCIL_TEST);
}

/**
 * @brief RB_ProjectionShadowDeform
 */
void RB_ProjectionShadowDeform(void)
{
	float  *xyz = (float *)tess.xyz;
	int    i;
	float  h;
	vec3_t ground;
	vec3_t light;
	float  groundDist;
	float  d;
	vec3_t lightDir;

	ground[0] = backEnd.orientation.axis[0][2];
	ground[1] = backEnd.orientation.axis[1][2];
	ground[2] = backEnd.orientation.axis[2][2];

	groundDist = backEnd.orientation.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy(backEnd.currentEntity->lightDir, lightDir);
	d = DotProduct(lightDir, ground);
	// don't let the shadows get too long or go negative
	if (d < 0.5f)
	{
		VectorMA(lightDir, (0.5f - d), ground, lightDir);
		d = DotProduct(lightDir, ground);
	}
	d = 1.0f / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	for (i = 0; i < tess.numVertexes; i++, xyz += 4)
	{
		h = DotProduct(xyz, ground) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}
