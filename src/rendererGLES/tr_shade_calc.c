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
 * @file rendererGLES/tr_shade_calc.c
 */

#include "tr_local.h"

#define WAVEVALUE(table, base, amplitude, phase, freq)  ((base) + table[(int64_t)((((phase) + tess.shaderTime * (freq)) *FUNCTABLE_SIZE)) & FUNCTABLE_MASK] * (amplitude))

/**
 * @brief TableForFunc
 * @param[in] func
 * @return
 */
static float *TableForFunc(genFunc_t func)
{
	switch (func)
	{
	case GF_SIN:
		return tr.sinTable;
	case GF_TRIANGLE:
		return tr.triangleTable;
	case GF_SQUARE:
		return tr.squareTable;
	case GF_SAWTOOTH:
		return tr.sawToothTable;
	case GF_INVERSE_SAWTOOTH:
		return tr.inverseSawToothTable;
	case GF_NONE:
	default:
		break;
	}

	Ren_Drop("TableForFunc called with invalid function '%d' in shader '%s'\n", func, tess.shader->name);
	return NULL;
}

/**
 * @brief Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
 * @param[in] wf
 * @return
 */
static float EvalWaveForm(const waveForm_t *wf)
{
	float *table;

	table = TableForFunc(wf->func);

	return WAVEVALUE(table, wf->base, wf->amplitude, wf->phase, wf->frequency);
}

/**
 * @brief EvalWaveFormClamped
 * @param[in] wf
 * @return
 */
static float EvalWaveFormClamped(const waveForm_t *wf)
{
	float glow = EvalWaveForm(wf);

	if (glow < 0)
	{
		return 0;
	}

	if (glow > 1)
	{
		return 1;
	}

	return glow;
}

/**
 * @brief RB_CalcStretchTexCoords
 * @param[in] wf
 * @param[out] texCoords
 */
void RB_CalcStretchTexCoords(const waveForm_t *wf, float *texCoords)
{
	float        p;
	texModInfo_t tmi;

	p = 1.0f / EvalWaveForm(wf);

	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords(&tmi, texCoords);
}

/*
====================================================================
DEFORMATIONS
====================================================================
*/

/**
 * @brief RB_CalcDeformVertexes
 * @param[in,out] ds
 */
void RB_CalcDeformVertexes(deformStage_t *ds)
{
	int    i;
	vec3_t offset;
	float  scale;
	float  *xyz    = ( float * ) tess.xyz;
	float  *normal = ( float * ) tess.normal;
	float  *table;

	if (ds->deformationWave.frequency < 0)
	{
		float    off;
		float    dot;
		qboolean inverse = qfalse;
		vec3_t   worldUp;

		if (VectorCompare(backEnd.currentEntity->e.fireRiseDir, vec3_origin))
		{
			VectorSet(backEnd.currentEntity->e.fireRiseDir, 0, 0, 1);
		}

		// get the world up vector in local coordinates
		if (backEnd.currentEntity->e.hModel)      // world surfaces dont have an axis
		{
			VectorRotate(backEnd.currentEntity->e.fireRiseDir, backEnd.currentEntity->e.axis, worldUp);
		}
		else
		{
			VectorCopy(backEnd.currentEntity->e.fireRiseDir, worldUp);
		}
		// don't go so far if sideways, since they must be moving
		VectorScale(worldUp, 0.4f + 0.6f * Q_fabs(backEnd.currentEntity->e.fireRiseDir[2]), worldUp);

		ds->deformationWave.frequency *= -1;
		if (ds->deformationWave.frequency > 999)      // hack for negative Z deformation (ack)
		{
			inverse                        = qtrue;
			ds->deformationWave.frequency -= 999;
		}

		table = TableForFunc(ds->deformationWave.func);

		for (i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
		{
			off = (xyz[0] + xyz[1] + xyz[2]) * ds->deformationSpread;

			scale = WAVEVALUE(table, ds->deformationWave.base,
			                  ds->deformationWave.amplitude,
			                  ds->deformationWave.phase + off,
			                  ds->deformationWave.frequency);

			dot = DotProduct(worldUp, normal);

			if (dot * scale > 0)
			{
				if (inverse)
				{
					scale *= -1;
				}
				VectorMA(xyz, dot * scale, worldUp, xyz);
			}
		}

		if (inverse)
		{
			ds->deformationWave.frequency += 999;
		}
		ds->deformationWave.frequency *= -1;
	}
	else if (ds->deformationWave.frequency == 0.)
	{
		scale = EvalWaveForm(&ds->deformationWave);

		for (i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
		{
			VectorScale(normal, scale, offset);

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
	else
	{
		float off;

		table = TableForFunc(ds->deformationWave.func);

		for (i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
		{
			off = (xyz[0] + xyz[1] + xyz[2]) * ds->deformationSpread;

			scale = WAVEVALUE(table, ds->deformationWave.base,
			                  ds->deformationWave.amplitude,
			                  ds->deformationWave.phase + off,
			                  ds->deformationWave.frequency);

			VectorScale(normal, scale, offset);

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
}

/**
 * @brief Wiggle the normals for wavy environment mapping
 * @param[in] ds
 */
void RB_CalcDeformNormals(deformStage_t *ds)
{
	int    i;
	double scale;
	float  *xyz    = ( float * ) tess.xyz;
	float  *normal = ( float * ) tess.normal;

	for (i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
	{
		scale = 0.98;
		scale = R_NoiseGet4f(xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
		                     tess.shaderTime * ds->deformationWave.frequency);
		normal[0] += ds->deformationWave.amplitude * scale;

		scale = 0.98;
		scale = R_NoiseGet4f(100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
		                     tess.shaderTime * ds->deformationWave.frequency);
		normal[1] += ds->deformationWave.amplitude * scale;

		scale = 0.98;
		scale = R_NoiseGet4f(200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
		                     tess.shaderTime * ds->deformationWave.frequency);
		normal[2] += ds->deformationWave.amplitude * scale;

		VectorNormalizeFast(normal);
	}
}

/**
 * @brief RB_CalcBulgeVertexes
 * @param[in] ds
 */
void RB_CalcBulgeVertexes(deformStage_t *ds)
{
	int         i;
	const float *st     = ( const float * ) tess.texCoords[0];
	float       *xyz    = ( float * ) tess.xyz;
	float       *normal = ( float * ) tess.normal;
	double      now     = backEnd.refdef.time * 0.001 * ds->bulgeSpeed;
	int64_t     off;
	float       scale;

	for (i = 0; i < tess.numVertexes; i++, xyz += 4, st += 4, normal += 4)
	{
		off   = (FUNCTABLE_SIZE / (M_PI * 2)) * (st[0] * ds->bulgeWidth + now);
		scale = tr.sinTable[off & FUNCTABLE_MASK] * ds->bulgeHeight;

		xyz[0] += normal[0] * scale;
		xyz[1] += normal[1] * scale;
		xyz[2] += normal[2] * scale;
	}
}

/**
 * @brief A deformation that can move an entire surface along a wave path
 * @param[in] ds
 */
void RB_CalcMoveVertexes(deformStage_t *ds)
{
	int    i;
	float  *xyz;
	float  *table;
	float  scale;
	vec3_t offset;

	table = TableForFunc(ds->deformationWave.func);

	scale = WAVEVALUE(table, ds->deformationWave.base,
	                  ds->deformationWave.amplitude,
	                  ds->deformationWave.phase,
	                  ds->deformationWave.frequency);

	VectorScale(ds->moveVector, scale, offset);

	xyz = ( float * ) tess.xyz;
	for (i = 0; i < tess.numVertexes; i++, xyz += 4)
	{
		VectorAdd(xyz, offset, xyz);
	}
}

/**
 * @brief Change a polygon into a bunch of text polygons
 * @param[in] text
 */
void DeformText(const char *text)
{
	int    i;
	vec3_t origin, width, height = { 0, 0, -1 };
	int    len;
	int    ch;
	byte   color[4];
	float  bottom, top;
	vec3_t mid;
	int    row, col;
	float  frow, fcol, size;

	CrossProduct(tess.normal[0], height, width);

	// find the midpoint of the box
	VectorClear(mid);
	bottom = 999999;
	top    = -999999;
	for (i = 0 ; i < 4 ; i++)
	{
		VectorAdd(tess.xyz[i], mid, mid);
		if (tess.xyz[i][2] < bottom)
		{
			bottom = tess.xyz[i][2];
		}
		if (tess.xyz[i][2] > top)
		{
			top = tess.xyz[i][2];
		}
	}
	VectorScale(mid, 0.25f, origin);

	// determine the individual character size
	height[0] = 0;
	height[1] = 0;
	height[2] = (top - bottom) * 0.5f;

	VectorScale(width, height[2] * -0.75f, width);

	// determine the starting position
	len = strlen(text);
	VectorMA(origin, (len - 1), width, origin);

	// clear the shader indexes
	tess.numIndexes  = 0;
	tess.numVertexes = 0;

	color[0] = color[1] = color[2] = color[3] = 255;

	// draw each character
	for (i = 0 ; i < len ; i++)
	{
		ch  = text[i];
		ch &= 255;

		if (ch != ' ')
		{
			row = ch >> 4;
			col = ch & 15;

			frow = row * 0.0625f;
			fcol = col * 0.0625f;
			size = 0.0625f;

			RB_AddQuadStampExt(origin, width, height, color, fcol, frow, fcol + size, frow + size);
		}
		VectorMA(origin, -2, width, origin);
	}
}

/**
 * @brief GlobalVectorToLocal
 * @param[in] in
 * @param[out] out
 */
void GlobalVectorToLocal(const vec3_t in, vec3_t out)
{
	out[0] = DotProduct(in, backEnd.orientation.axis[0]);
	out[1] = DotProduct(in, backEnd.orientation.axis[1]);
	out[2] = DotProduct(in, backEnd.orientation.axis[2]);
}

/**
 * @brief Assuming all the triangles for this shader are independant
 * quads, rebuild them as forward facing sprites
 */
static void AutospriteDeform(void)
{
	int    i;
	int    oldVerts;
	float  *xyz;
	vec3_t mid, delta;
	float  radius;
	vec3_t left, up;
	vec3_t leftDir, upDir;

	if (tess.numVertexes & 3)
	{
		Ren_Developer("Autosprite shader %s had odd vertex count\n", tess.shader->name);
	}
	if (tess.numIndexes != (tess.numVertexes >> 2) * 6)
	{
		Ren_Developer("Autosprite shader %s had odd index count\n", tess.shader->name);
	}

	oldVerts         = tess.numVertexes;
	tess.numVertexes = 0;
	tess.numIndexes  = 0;

	if (backEnd.currentEntity != &tr.worldEntity)
	{
		GlobalVectorToLocal(backEnd.viewParms.orientation.axis[1], leftDir);
		GlobalVectorToLocal(backEnd.viewParms.orientation.axis[2], upDir);
	}
	else
	{
		VectorCopy(backEnd.viewParms.orientation.axis[1], leftDir);
		VectorCopy(backEnd.viewParms.orientation.axis[2], upDir);
	}

	for (i = 0 ; i < oldVerts ; i += 4)
	{
		// find the midpoint
		xyz = tess.xyz[i];

		mid[0] = 0.25f * (xyz[0] + xyz[4] + xyz[8] + xyz[12]);
		mid[1] = 0.25f * (xyz[1] + xyz[5] + xyz[9] + xyz[13]);
		mid[2] = 0.25f * (xyz[2] + xyz[6] + xyz[10] + xyz[14]);

		VectorSubtract(xyz, mid, delta);
		radius = VectorLength(delta) * 0.707f;          // / sqrt(2)

		VectorScale(leftDir, radius, left);
		VectorScale(upDir, radius, up);

		if (backEnd.viewParms.isMirror)
		{
			VectorSubtract(vec3_origin, left, left);
		}

		// compensate for scale in the axes if necessary
		if (backEnd.currentEntity->e.nonNormalizedAxes)
		{
			float axisLength;

			axisLength = VectorLength(backEnd.currentEntity->e.axis[0]);
			if (axisLength == 0.f)
			{
				axisLength = 0;
			}
			else
			{
				axisLength = 1.0f / axisLength;
			}
			VectorScale(left, axisLength, left);
			VectorScale(up, axisLength, up);
		}

		RB_AddQuadStamp(mid, left, up, tess.vertexColors[i]);
	}
}

int edgeVerts[6][2] =
{
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

/**
 * @brief Autosprite2 will pivot a rectangular quad along the center of its long axis
 */
static void Autosprite2Deform(void)
{
	int    i, j, k;
	int    indexes;
	int    nums[2];
	float  *xyz;
	float  lengths[2];
	float  l;
	vec3_t forward;
	vec3_t temp;
	vec3_t mid[2];
	vec3_t major, minor;
	float  *v1, *v2;

	if (tess.numVertexes & 3)
	{
		Ren_Developer("Autosprite2 shader %s had odd vertex count\n", tess.shader->name);
	}
	if (tess.numIndexes != (tess.numVertexes >> 2) * 6)
	{
		Ren_Developer("Autosprite2 shader %s had odd index count\n", tess.shader->name);
	}

	if (backEnd.currentEntity != &tr.worldEntity)
	{
		GlobalVectorToLocal(backEnd.viewParms.orientation.axis[0], forward);
	}
	else
	{
		VectorCopy(backEnd.viewParms.orientation.axis[0], forward);
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	for (i = 0, indexes = 0 ; i < tess.numVertexes ; i += 4, indexes += 6)
	{
		// find the midpoint
		xyz = tess.xyz[i];

		// identify the two shortest edges
		nums[0]    = nums[1] = 0;
		lengths[0] = lengths[1] = 999999;

		for (j = 0 ; j < 6 ; j++)
		{
			v1 = xyz + 4 * edgeVerts[j][0];
			v2 = xyz + 4 * edgeVerts[j][1];

			VectorSubtract(v1, v2, temp);

			l = DotProduct(temp, temp);
			if (l < lengths[0])
			{
				nums[1]    = nums[0];
				lengths[1] = lengths[0];
				nums[0]    = j;
				lengths[0] = l;
			}
			else if (l < lengths[1])
			{
				nums[1]    = j;
				lengths[1] = l;
			}
		}

		for (j = 0 ; j < 2 ; j++)
		{
			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			mid[j][0] = 0.5f * (v1[0] + v2[0]);
			mid[j][1] = 0.5f * (v1[1] + v2[1]);
			mid[j][2] = 0.5f * (v1[2] + v2[2]);
		}

		// find the vector of the major axis
		VectorSubtract(mid[1], mid[0], major);

		// cross this with the view direction to get minor axis
		CrossProduct(major, forward, minor);
		VectorNormalize(minor);

		// re-project the points
		for (j = 0 ; j < 2 ; j++)
		{
			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			l = 0.5 * sqrt(lengths[j]);

			// we need to see which direction this edge
			// is used to determine direction of projection
			for (k = 0 ; k < 5 ; k++)
			{
				if (tess.indexes[indexes + k] == i + edgeVerts[nums[j]][0]
				    && tess.indexes[indexes + k + 1] == i + edgeVerts[nums[j]][1])
				{
					break;
				}
			}

			if (k == 5)
			{
				VectorMA(mid[j], l, minor, v1);
				VectorMA(mid[j], -l, minor, v2);
			}
			else
			{
				VectorMA(mid[j], -l, minor, v1);
				VectorMA(mid[j], l, minor, v2);
			}
		}
	}
}

/**
 * @brief RB_DeformTessGeometry
 */
void RB_DeformTessGeometry(void)
{
	int           i;
	deformStage_t *ds;

	for (i = 0 ; i < tess.shader->numDeforms ; i++)
	{
		ds = &tess.shader->deforms[i];

		switch (ds->deformation)
		{
		case DEFORM_NONE:
			break;
		case DEFORM_NORMALS:
			RB_CalcDeformNormals(ds);
			break;
		case DEFORM_WAVE:
			RB_CalcDeformVertexes(ds);
			break;
		case DEFORM_BULGE:
			RB_CalcBulgeVertexes(ds);
			break;
		case DEFORM_MOVE:
			RB_CalcMoveVertexes(ds);
			break;
		case DEFORM_PROJECTION_SHADOW:
			RB_ProjectionShadowDeform();
			break;
		case DEFORM_AUTOSPRITE:
			AutospriteDeform();
			break;
		case DEFORM_AUTOSPRITE2:
			Autosprite2Deform();
			break;
		case DEFORM_TEXT0:
		case DEFORM_TEXT1:
		case DEFORM_TEXT2:
		case DEFORM_TEXT3:
		case DEFORM_TEXT4:
		case DEFORM_TEXT5:
		case DEFORM_TEXT6:
		case DEFORM_TEXT7:
			DeformText(backEnd.refdef.text[ds->deformation - DEFORM_TEXT0]);
			break;
		}
	}
}

/*
====================================================================
COLORS
====================================================================
*/

/**
 * @brief RB_CalcColorFromEntity
 * @param[in,out] colors
 */
void RB_CalcColorFromEntity(unsigned char *colors)
{
	int i;
	int *pColors = ( int * ) colors;
	int c;

	if (!backEnd.currentEntity)
	{
		return;
	}

	c = *( int * ) backEnd.currentEntity->e.shaderRGBA;

	for (i = 0; i < tess.numVertexes; i++, pColors++)
	{
		*pColors = c;
	}
}

/**
 * @brief RB_CalcColorFromOneMinusEntity
 * @param[in,out] colors
 */
void RB_CalcColorFromOneMinusEntity(unsigned char *colors)
{
	int           i;
	int           *pColors = ( int * ) colors;
	unsigned char invModulate[4];
	int           c;

	if (!backEnd.currentEntity)
	{
		return;
	}

	invModulate[0] = 255 - backEnd.currentEntity->e.shaderRGBA[0];
	invModulate[1] = 255 - backEnd.currentEntity->e.shaderRGBA[1];
	invModulate[2] = 255 - backEnd.currentEntity->e.shaderRGBA[2];
	invModulate[3] = 255 - backEnd.currentEntity->e.shaderRGBA[3];  // this trashes alpha, but the AGEN block fixes it

	c = *( int * ) invModulate;

	for (i = 0; i < tess.numVertexes; i++, pColors++)
	{
		*pColors = c;
	}
}

/**
 * @brief RB_CalcAlphaFromEntity
 * @param[in,out] colors
 */
void RB_CalcAlphaFromEntity(unsigned char *colors)
{
	int i;

	if (!backEnd.currentEntity)
	{
		return;
	}

	colors += 3;

	for (i = 0; i < tess.numVertexes; i++, colors += 4)
	{
		*colors = backEnd.currentEntity->e.shaderRGBA[3];
	}
}

/**
 * @brief RB_CalcAlphaFromOneMinusEntity
 * @param[in,out] colors
 */
void RB_CalcAlphaFromOneMinusEntity(unsigned char *colors)
{
	int i;

	if (!backEnd.currentEntity)
	{
		return;
	}

	colors += 3;

	for (i = 0; i < tess.numVertexes; i++, colors += 4)
	{
		*colors = 0xff - backEnd.currentEntity->e.shaderRGBA[3];
	}
}

/**
 * @brief RB_CalcWaveColor
 * @param[in] wf
 * @param[in,out] colors
 */
void RB_CalcWaveColor(const waveForm_t *wf, unsigned char *colors)
{
	int   i;
	int   v;
	float glow;
	int   *iColors = ( int * ) colors;
	byte  color[4];

	if (wf->func == GF_NOISE)
	{
		glow = wf->base + R_NoiseGet4f(0, 0, 0, (tess.shaderTime + wf->phase) * wf->frequency) * wf->amplitude;
	}
	else
	{
		glow = EvalWaveForm(wf) * tr.identityLight;
	}

	if (glow < 0)
	{
		glow = 0;
	}
	else if (glow > 1)
	{
		glow = 1;
	}

	v        = (int)(255 * glow);
	color[0] = color[1] = color[2] = v;
	color[3] = 255;
	v        = *(int *)color;

	for (i = 0; i < tess.numVertexes; i++, iColors++)
	{
		*iColors = v;
	}
}

/**
 * @brief RB_CalcWaveAlpha
 * @param[in] wf
 * @param[in,out] colors
 */
void RB_CalcWaveAlpha(const waveForm_t *wf, unsigned char *colors)
{
	int   i;
	int   v;
	float glow;

	// added alphaGen noise support
	if (wf->func == GF_NOISE)
	{
		glow = wf->base + R_NoiseGet4f(0, 0, 0, (tess.shaderTime + wf->phase) * wf->frequency) * wf->amplitude;
	}
	else
	{
		glow = EvalWaveFormClamped(wf);
	}

	v = 255 * glow;

	for (i = 0; i < tess.numVertexes; i++, colors += 4)
	{
		colors[3] = v;
	}
}

/**
 * @brief RB_CalcModulateAlphasByFog
 * @param[in,out] colors
 */
void RB_CalcModulateAlphasByFog(unsigned char *colors)
{
	int   i;
	float f, texCoords[SHADER_MAX_VERTEXES][2];

	// no world, no fogging
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords(texCoords[0]);

	for (i = 0; i < tess.numVertexes; i++, colors += 4)
	{
		if (texCoords[i][0] <= 0.0f || texCoords[i][1] <= 0.0f)
		{
			continue;
		}
		else
		{
			f = 1.0f - (texCoords[i][0] * texCoords[i][1]);
		}
		if (f <= 0.0f)
		{
			colors[3] = 0;
		}
		else
		{
			colors[3] *= f;
		}
	}
}

/**
 * @brief RB_CalcModulateColorsByFog
 * @param[in,out] colors
 */
void RB_CalcModulateColorsByFog(unsigned char *colors)
{
	int   i;
	float f, texCoords[SHADER_MAX_VERTEXES][2];

	// no world, no fogging
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords(texCoords[0]);

	for (i = 0; i < tess.numVertexes; i++, colors += 4)
	{
		if (texCoords[i][0] <= 0.0f || texCoords[i][1] <= 0.0f)
		{
			continue;
		}
		else
		{
			f = 1.0f - (texCoords[i][0] * texCoords[i][1]);
		}
		if (f <= 0.0f)
		{
			colors[0] = 0;
			colors[1] = 0;
			colors[2] = 0;
		}
		else
		{
			colors[0] *= f;
			colors[1] *= f;
			colors[2] *= f;
		}
	}
}

/**
 * @brief RB_CalcModulateRGBAsByFog
 * @param[in,out] colors
 */
void RB_CalcModulateRGBAsByFog(unsigned char *colors)
{
	int   i;
	float f, texCoords[SHADER_MAX_VERTEXES][2];

	// no world, no fogging
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords(texCoords[0]);

	for (i = 0; i < tess.numVertexes; i++, colors += 4)
	{
		if (texCoords[i][0] <= 0.0f || texCoords[i][1] <= 0.0f)
		{
			continue;
		}
		else
		{
			f = 1.0f - (texCoords[i][0] * texCoords[i][1]);
		}
		if (f <= 0.0f)
		{
			colors[0] = 0;
			colors[1] = 0;
			colors[2] = 0;
			colors[3] = 0;
		}
		else
		{
			colors[0] *= f;
			colors[1] *= f;
			colors[2] *= f;
			colors[3] *= f;
		}
	}
}

/*
====================================================================
TEX COORDS
====================================================================
*/

/**
 * @brief To do the clipped fog plane really correctly, we should use
 * projected textures, but I don't trust the drivers and it
 * doesn't fit our shader data.
 *
 * @param[in,out] texCoords
 */
void RB_CalcFogTexCoords(float *texCoords)
{
	int      i;
	float    *v;
	fog_t    *fog = tr.world->fogs + tess.fogNum;    // get fog stuff
	vec3_t   local, viewOrigin;
	vec4_t   fogSurface, fogDistanceVector, fogDepthVector = { 0, 0, 0, 0 };
	bmodel_t *bmodel = tr.world->bmodels + fog->modelNum; // get fog stuff

	// all fogging distance is based on world Z units
	VectorSubtract(backEnd.orientation.origin, backEnd.viewParms.orientation.origin, local);
	//VectorSubtract( local, bmodel->origin, local );
	fogDistanceVector[0] = -backEnd.orientation.modelMatrix[2];
	fogDistanceVector[1] = -backEnd.orientation.modelMatrix[6];
	fogDistanceVector[2] = -backEnd.orientation.modelMatrix[10];
	fogDistanceVector[3] = DotProduct(local, backEnd.viewParms.orientation.axis[0]);

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[0] *= fog->shader->fogParms.tcScale * 1.0f;
	fogDistanceVector[1] *= fog->shader->fogParms.tcScale * 1.0f;
	fogDistanceVector[2] *= fog->shader->fogParms.tcScale * 1.0f;
	fogDistanceVector[3] *= fog->shader->fogParms.tcScale * 1.0f;

	// offset view origin by fog brush origin (fixme: really necessary?)
	//VectorSubtract( backEnd.orientation.viewOrigin, bmodel->origin, viewOrigin );
	VectorCopy(backEnd.orientation.viewOrigin, viewOrigin);

	// offset fog surface
	VectorCopy(fog->surface, fogSurface);
	fogSurface[3] = fog->surface[3] + DotProduct(fogSurface, bmodel->orientation.origin);

	// general fog case
	if (fog->originalBrushNumber >= 0)
	{
		float    s, t;
		float    eyeT;
		qboolean eyeInside;

		// rotate the gradient vector for this orientation
		if (fog->hasSurface)
		{
			fogDepthVector[0] = fogSurface[0] * backEnd.orientation.axis[0][0] +
			                    fogSurface[1] * backEnd.orientation.axis[0][1] + fogSurface[2] * backEnd.orientation.axis[0][2];
			fogDepthVector[1] = fogSurface[0] * backEnd.orientation.axis[1][0] +
			                    fogSurface[1] * backEnd.orientation.axis[1][1] + fogSurface[2] * backEnd.orientation.axis[1][2];
			fogDepthVector[2] = fogSurface[0] * backEnd.orientation.axis[2][0] +
			                    fogSurface[1] * backEnd.orientation.axis[2][1] + fogSurface[2] * backEnd.orientation.axis[2][2];
			fogDepthVector[3] = -fogSurface[3] + DotProduct(backEnd.orientation.origin, fogSurface);

			// scale the fog vectors based on the fog's thickness
			fogDepthVector[0] *= fog->shader->fogParms.tcScale * 1.0f;
			fogDepthVector[1] *= fog->shader->fogParms.tcScale * 1.0f;
			fogDepthVector[2] *= fog->shader->fogParms.tcScale * 1.0f;
			fogDepthVector[3] *= fog->shader->fogParms.tcScale * 1.0f;

			eyeT = DotProduct(viewOrigin, fogDepthVector) + fogDepthVector[3];
		}
		else
		{
			eyeT = 1;   // non-surface fog always has eye inside
		}
		// see if the viewpoint is outside
		eyeInside = eyeT < 0 ? qfalse : qtrue;

		// calculate density for each point
		for (i = 0, v = tess.xyz[0] ; i < tess.numVertexes; i++, v += 4)
		{
			// calculate the length in fog
			s = DotProduct(v, fogDistanceVector) + fogDistanceVector[3];
			t = DotProduct(v, fogDepthVector) + fogDepthVector[3];

			if (eyeInside)
			{
				t += eyeT;
			}

			//t *= fog->shader->fogParms.tcScale;

			texCoords[0] = s;
			texCoords[1] = t;
			texCoords   += 2;
		}
	}
	// optimized for level-wide fogging
	else
	{
		// calculate density for each point
		for (i = 0, v = tess.xyz[0]; i < tess.numVertexes; i++, v += 4)
		{
			// calculate the length in fog (t is always 0 if eye is in fog)
			texCoords[0] = DotProduct(v, fogDistanceVector) + fogDistanceVector[3];
			texCoords[1] = 1.0;
			texCoords   += 2;
		}
	}
}

/**
 * @brief RB_CalcEnvironmentTexCoords
 * @param[in,out] texCoords
 */
void RB_CalcEnvironmentTexCoords(float *texCoords)
{
	vec3_t viewOrigin, ia1, ia2, viewer, reflected;
	float  d2, sAdjust, tAdjust;
	float  *v      = tess.xyz[0]; // setup
	float  *normal = tess.normal[0]; // setup
	int    i;

	VectorCopy(backEnd.orientation.viewOrigin, viewOrigin);

	// origin of entity affects its environment map (every 256 units)
	// this is similar to racing game hacks where the env map seems to move
	// as the car passes through the world
	sAdjust = VectorLength(backEnd.orientation.origin) * 0.00390625;
	//%	 sAdjust = backEnd.orientation.origin[ 0 ] * 0.00390625;
	sAdjust = 0.5 -  (sAdjust - floor(sAdjust));

	tAdjust = backEnd.orientation.origin[2] * 0.00390625;
	tAdjust = 0.5 - (tAdjust - floor(tAdjust));

	// the final reflection vector must be converted into world-space again
	// we just assume here that all transformations are rotations, so the inverse
	// of the transform matrix (the 3x3 part) is just the transpose
	// additionally, we don't need all 3 rows, so we just calculate 2
	// and we also scale by 0.5 to eliminate two per-vertex multiplies
	ia1[0] = backEnd.orientation.axis[0][1] * 0.5f;
	ia1[1] = backEnd.orientation.axis[1][1] * 0.5f;
	ia1[2] = backEnd.orientation.axis[2][1] * 0.5f;
	ia2[0] = backEnd.orientation.axis[0][2] * 0.5f;
	ia2[1] = backEnd.orientation.axis[1][2] * 0.5f;
	ia2[2] = backEnd.orientation.axis[2][2] * 0.5f;

	// walk verts
	for (i = 0; i < tess.numVertexes; i++, v += 4, normal += 4, texCoords += 2)
	{
		VectorSubtract(viewOrigin, v, viewer);
		VectorNormalizeFast(viewer);

		d2 = 2.0f * DotProduct(normal, viewer);

		reflected[0] = normal[0] * d2 - viewer[0];
		reflected[1] = normal[1] * d2 - viewer[1];
		reflected[2] = normal[2] * d2 - viewer[2];

		texCoords[0] = sAdjust + DotProduct(reflected, ia1);
		texCoords[1] = tAdjust - DotProduct(reflected, ia2);
	}
}

/**
 * @brief RB_CalcFireRiseEnvTexCoords
 * @param[in,out] st
 */
void RB_CalcFireRiseEnvTexCoords(float *st)
{
	vec3_t viewer, reflected;
	int    i;
	float  *v      = tess.xyz[0];
	float  *normal = tess.normal[0];
	float  d;

	VectorNegate(backEnd.currentEntity->e.fireRiseDir, viewer);

	for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2)
	{
		VectorNormalizeFast(viewer);

		d = DotProduct(normal, viewer);

		reflected[0] = normal[0] * 2 * d - viewer[0];
		reflected[1] = normal[1] * 2 * d - viewer[1];
		reflected[2] = normal[2] * 2 * d - viewer[2];

		st[0] = 0.5f + reflected[1] * 0.5f;
		st[1] = 0.5f - reflected[2] * 0.5f;
	}
}

/**
 * @brief RB_CalcSwapTexCoords
 * @param[in,out] texCoords
 */
void RB_CalcSwapTexCoords(float *texCoords)
{
	int   i;
	float s, t;

	for (i = 0; i < tess.numVertexes; i++, texCoords += 2)
	{
		s = texCoords[0];
		t = texCoords[1];

		texCoords[0] = t;
		texCoords[1] = 1.0f - s;    // err, flaming effect needs this
	}
}

/**
 * @brief RB_CalcTurbulentTexCoords
 * @param[in] wf
 * @param[in,out] texCoords
 */
void RB_CalcTurbulentTexCoords(const waveForm_t *wf, float *texCoords)
{
	int    i;
	double now = (wf->phase + tess.shaderTime * wf->frequency);
	float  s, t;

	for (i = 0; i < tess.numVertexes; i++, texCoords += 2)
	{
		s = texCoords[0];
		t = texCoords[1];

		texCoords[0] = s + tr.sinTable[(( int ) (((tess.xyz[i][0] + tess.xyz[i][2]) * 1.0 / 128 * 0.125 + now) * FUNCTABLE_SIZE)) & (FUNCTABLE_MASK)] * wf->amplitude;
		texCoords[1] = t + tr.sinTable[(( int ) ((tess.xyz[i][1] * 1.0 / 128 * 0.125 + now) * FUNCTABLE_SIZE)) & (FUNCTABLE_MASK)] * wf->amplitude;
	}
}

/**
 * @brief RB_CalcScaleTexCoords
 * @param[in] scale
 * @param[in,out] texCoords
 */
void RB_CalcScaleTexCoords(const float scale[2], float *texCoords)
{
	int i;

	for (i = 0; i < tess.numVertexes; i++, texCoords += 2)
	{
		texCoords[0] *= scale[0];
		texCoords[1] *= scale[1];
	}
}

/**
 * @brief RB_CalcScrollTexCoords
 * @param[in] scrollSpeed
 * @param[in,out] texCoords
 */
void RB_CalcScrollTexCoords(const float scrollSpeed[2], float *texCoords)
{
	int    i;
	double timeScale       = tess.shaderTime;
	double adjustedScrollS = (double)scrollSpeed[0] * timeScale;
	double adjustedScrollT = (double)scrollSpeed[1] * timeScale;

	// clamp so coordinates don't continuously get larger, causing problems
	// with hardware limits
	adjustedScrollS = adjustedScrollS - floor(adjustedScrollS);
	adjustedScrollT = adjustedScrollT - floor(adjustedScrollT);

	for (i = 0; i < tess.numVertexes; i++, texCoords += 2)
	{
		texCoords[0] += adjustedScrollS;
		texCoords[1] += adjustedScrollT;
	}
}

/**
 * @brief RB_CalcTransformTexCoords
 * @param[in] tmi
 * @param[in,out] texCoords
 */
void RB_CalcTransformTexCoords(const texModInfo_t *tmi, float *texCoords)
{
	int   i;
	float s, t;

	for (i = 0; i < tess.numVertexes; i++, texCoords += 2)
	{
		s = texCoords[0];
		t = texCoords[1];

		texCoords[0] = s * tmi->matrix[0][0] + t * tmi->matrix[1][0] + tmi->translate[0];
		texCoords[1] = s * tmi->matrix[0][1] + t * tmi->matrix[1][1] + tmi->translate[1];
	}
}

/**
 * @brief RB_CalcRotateTexCoords
 * @param[in] degsPerSecond
 * @param[in,out] texCoords
 */
void RB_CalcRotateTexCoords(float degsPerSecond, float *texCoords)
{
	texModInfo_t tmi;
	double       timeScale = tess.shaderTime;
	double       degs      = -degsPerSecond * timeScale;
	int64_t      index     = degs * (FUNCTABLE_SIZE / 360.0);
	float        sinValue  = tr.sinTable[index & FUNCTABLE_MASK];
	float        cosValue  = tr.sinTable[(index + FUNCTABLE_SIZE / 4) & FUNCTABLE_MASK];

	tmi.matrix[0][0] = cosValue;
	tmi.matrix[1][0] = -sinValue;
	tmi.translate[0] = 0.5f - 0.5f * cosValue + 0.5f * sinValue;

	tmi.matrix[0][1] = sinValue;
	tmi.matrix[1][1] = cosValue;
	tmi.translate[1] = 0.5f - 0.5f * sinValue - 0.5f * cosValue;

	RB_CalcTransformTexCoords(&tmi, texCoords);
}

vec3_t lightOrigin = { -960, 1980, 96 };        // FIXME: track dynamically

/**
 * @brief Calculates specular coefficient and places it in the alpha channel
 * @param[in,out] alphas
 */
void RB_CalcSpecularAlpha(unsigned char *alphas)
{
	vec3_t viewer, reflected, lightDir;
	float  *v      = tess.xyz[0];
	float  *normal = tess.normal[0];
	float  l, d, ilength;
	int    b;
	int    numVertexes;
	int    i;

	alphas += 3;

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4, alphas += 4)
	{
		VectorSubtract(lightOrigin, v, lightDir);
		//ilength = Q_rsqrt( DotProduct( lightDir, lightDir ) );
		VectorNormalizeFast(lightDir);

		// calculate the specular color
		d = DotProduct(normal, lightDir);
		//d *= ilength;

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected[0] = normal[0] * 2 * d - lightDir[0];
		reflected[1] = normal[1] * 2 * d - lightDir[1];
		reflected[2] = normal[2] * 2 * d - lightDir[2];

		VectorSubtract(backEnd.orientation.viewOrigin, v, viewer);
		ilength = Q_rsqrt(DotProduct(viewer, viewer));
		l       = DotProduct(reflected, viewer);
		l      *= ilength;

		if (l < 0)
		{
			b = 0;
		}
		else
		{
			l = l * l;
			l = l * l;
			b = l * 255;
			if (b > 255)
			{
				b = 255;
			}
		}

		*alphas = b;
	}
}

/**
 * @brief The basic vertex lighting calc
 * @details Faster, table-based version of this function
 * saves about 1-2ms per frame on my machine with 64 x 1000 triangle models in scene
 *
 * @param[in,out] colors
 */
void RB_CalcDiffuseColor(unsigned char *colors)
{
	int           i, dp, *colorsInt = (int *) colors;
	float         *normal = tess.normal[0];
	trRefEntity_t *ent    = backEnd.currentEntity;
	vec3_t        lightDir;
	int           numVertexes = tess.numVertexes;

	VectorCopy(ent->lightDir, lightDir);

	for (i = 0; i < numVertexes; i++, normal += 4, colorsInt++)
	{
		dp = (int)(ENTITY_LIGHT_STEPS * DotProduct(normal, lightDir));

		// enable this for twosided lighting
		//if( tess.shader->cullType == CT_TWO_SIDED )
		//	dp = Q_fabs( dp );

		if (dp <= 0)
		{
			*colorsInt = ent->entityLightInt[0];
		}
		else if (dp >= ENTITY_LIGHT_STEPS)
		{
			*colorsInt = ent->entityLightInt[ENTITY_LIGHT_STEPS - 1];
		}
		else
		{
			*colorsInt = ent->entityLightInt[dp];
		}
	}
}
