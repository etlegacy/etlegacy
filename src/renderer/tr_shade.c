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
 * @file renderer/tr_shade.c
 * @brief This file deals with applying shaders to surface data in the tess struct.
 *
 * @note THIS ENTIRE FILE IS BACK END
 */

#include "tr_local.h"
#if idppc_altivec && !defined(__APPLE__)
#include <altivec.h>
#endif

/**
 * @brief R_ArrayElementDiscrete
 * @param[in] index
 */
static void GLAPIENTRY R_ArrayElementDiscrete(GLint index)
{
	glColor4ubv(tess.svars.colors[index]);
	if (glState.currenttmu)
	{
		glMultiTexCoord2fARB(0, tess.svars.texcoords[0][index][0], tess.svars.texcoords[0][index][1]);
		glMultiTexCoord2fARB(1, tess.svars.texcoords[1][index][0], tess.svars.texcoords[1][index][1]);
	}
	else
	{
		glTexCoord2fv(tess.svars.texcoords[0][index]);
	}
	glVertex3fv(tess.xyz[index]);
}

/**
 * @brief This is just because of the GLEW and Windows idiocy. Straight call to glArrayElement brakes the build on function type mismatch.
 * @param[in] index
 */
static void GLAPIENTRY R_ArrayElement(GLint index)
{
	glArrayElement(index);
}

static int c_vertexes;          // for seeing how long our average strips are
static int c_begins;

/**
 * @brief R_DrawStripElements
 * @param[in] numIndexes
 * @param[in] indexes
 */
static void R_DrawStripElements(int numIndexes, const glIndex_t *indexes, void(GLAPIENTRY * element)(GLint))
{
	int      i;
	int      last[3] = { -1, -1, -1 };
	qboolean even;

	c_begins++;

	if (numIndexes <= 0)
	{
		return;
	}

	glBegin(GL_TRIANGLE_STRIP);

	// prime the strip
	element(indexes[0]);
	element(indexes[1]);
	element(indexes[2]);
	c_vertexes += 3;

	last[0] = indexes[0];
	last[1] = indexes[1];
	last[2] = indexes[2];

	even = qfalse;

	for (i = 3; i < numIndexes; i += 3)
	{
		// odd numbered triangle in potential strip
		if (!even)
		{
			// check previous triangle to see if we're continuing a strip
			if ((indexes[i + 0] == last[2]) && (indexes[i + 1] == last[1]))
			{
				element(indexes[i + 2]);
				c_vertexes++;
				etl_assert(indexes[i + 2] < tess.numVertexes);
				even = qtrue;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				glEnd();

				glBegin(GL_TRIANGLE_STRIP);
				c_begins++;

				element(indexes[i + 0]);
				element(indexes[i + 1]);
				element(indexes[i + 2]);

				c_vertexes += 3;

				even = qfalse;
			}
		}
		else
		{
			// check previous triangle to see if we're continuing a strip
			if ((last[2] == indexes[i + 1]) && (last[0] == indexes[i + 0]))
			{
				element(indexes[i + 2]);
				c_vertexes++;

				even = qfalse;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				glEnd();

				glBegin(GL_TRIANGLE_STRIP);
				c_begins++;

				element(indexes[i + 0]);
				element(indexes[i + 1]);
				element(indexes[i + 2]);
				c_vertexes += 3;

				even = qfalse;
			}
		}

		// cache the last three vertices
		last[0] = indexes[i + 0];
		last[1] = indexes[i + 1];
		last[2] = indexes[i + 2];
	}

	glEnd();
}

/**
 * @brief Optionally performs our own glDrawElements that looks for strip conditions
 * instead of using the single glDrawElements call that may be inefficient
 * without compiled vertex arrays.
 *
 * @param[in] numIndexes
 * @param[in] indexes
 */
static void R_DrawElements(int numIndexes, const glIndex_t *indexes)
{
	glDrawElements(GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, indexes);
}

/*
=============================================================
SURFACE SHADERS
=============================================================
*/

shaderCommands_t tess;
static qboolean  setArraysOnce;

/**
 * @brief R_BindAnimatedImage
 * @param[in] bundle
 */
static void R_BindAnimatedImage(textureBundle_t *bundle)
{
	int64_t index;

	if (bundle->isVideoMap)
	{
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if (bundle->numImageAnimations <= 1)
	{
		if (bundle->isLightmap && (backEnd.refdef.rdflags & RDF_SNOOPERVIEW))
		{
			GL_Bind(tr.whiteImage);
		}
		else
		{
			GL_Bind(bundle->image[0]);
		}
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	//index   = (int)(tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
	index   = (int64_t)(tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
	index >>= FUNCTABLE_SIZE2;

	if (index < 0)
	{
		index = 0;  // may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	if (bundle->isLightmap && (backEnd.refdef.rdflags & RDF_SNOOPERVIEW))
	{
		GL_Bind(tr.whiteImage);
	}
	else
	{
		GL_Bind(bundle->image[index]);
	}
}

/**
 * @brief Draws triangle outlines for debugging
 * @param[in] input
 */
static void DrawTris(shaderCommands_t *input)
{
	char         *s        = r_trisColor->string;
	vec4_t       trisColor = { 1, 1, 1, 1 };
	unsigned int stateBits = 0;

	// exclude 2d from drawing tris
	if (backEnd.projection2D == qtrue)
	{
		return;
	}

	GL_Bind(tr.whiteImage);

	if (*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X'))
	{
		s += 2;
		if (Q_IsHexColorString(s))
		{
			trisColor[0] = ((float)(gethex(*(s)) * 16 + gethex(*(s + 1)))) / 255.00f;
			trisColor[1] = ((float)(gethex(*(s + 2)) * 16 + gethex(*(s + 3)))) / 255.00f;
			trisColor[2] = ((float)(gethex(*(s + 4)) * 16 + gethex(*(s + 5)))) / 255.00f;

			if (Q_HexColorStringHasAlpha(s))
			{
				trisColor[3] = ((float)(gethex(*(s + 6)) * 16 + gethex(*(s + 7)))) / 255.00f;
			}
		}
	}
	else
	{
		int  i;
		char *token;

		for (i = 0 ; i < 4 ; i++)
		{
			token = COM_Parse(&s);
			if (token[0])
			{
				trisColor[i] = (float)atof(token);
			}
			else
			{
				trisColor[i] = 1.f;
			}
		}

		if (trisColor[3] == 0.f)
		{
			trisColor[3] = 1.f;
		}
	}

	if (trisColor[3] < 1.f)
	{
		stateBits |= (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}

	glColor4fv(trisColor);

	if (r_showTris->integer == 2)
	{
		stateBits |= (GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);
		GL_State(stateBits);
		glDepthRange(0, 0);
	}
#ifdef CELSHADING_HACK
	else if (r_showTris->integer == 3)
	{
		stateBits |= (GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);
		GL_State(stateBits);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(4.0, 0.5);
		glLineWidth(5.0);
	}
#endif
	else
	{
		stateBits |= (GLS_POLYMODE_LINE);
		GL_State(stateBits);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, 16, input->xyz);   // padded for SIMD

	if (glLockArraysEXT)
	{
		glLockArraysEXT(0, input->numVertexes);
		Ren_LogComment("glLockArraysEXT\n");
	}

	R_DrawElements(input->numIndexes, input->indexes);

	if (glUnlockArraysEXT)
	{
		glUnlockArraysEXT();
		Ren_LogComment("glUnlockArraysEXT\n");
	}
	glDepthRange(0, 1);
	glDisable(GL_POLYGON_OFFSET_LINE);
}

/**
 * @brief Draws vertex normals for debugging
 * @param[in] input
 */
static void DrawNormals(shaderCommands_t *input)
{
	vec3_t temp;

	GL_Bind(tr.whiteImage);
	glColor3f(1, 1, 1);
	glDepthRange(0, 0);    // never occluded
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);

	// light direction
	if (r_showNormals->integer == 2)
	{
		trRefEntity_t *ent = backEnd.currentEntity;
		vec3_t        temp2;

		if (ent->e.renderfx & RF_LIGHTING_ORIGIN)
		{
			VectorSubtract(ent->e.lightingOrigin, backEnd.orientation.origin, temp2);
		}
		else
		{
			VectorClear(temp2);
		}
		temp[0] = DotProduct(temp2, backEnd.orientation.axis[0]);
		temp[1] = DotProduct(temp2, backEnd.orientation.axis[1]);
		temp[2] = DotProduct(temp2, backEnd.orientation.axis[2]);

		glColor3f(ent->ambientLight[0] / 255, ent->ambientLight[1] / 255, ent->ambientLight[2] / 255);
		glPointSize(5);
		glBegin(GL_POINTS);
		glVertex3fv(temp);
		glEnd();
		glPointSize(1);

		if (Q_fabs(VectorLengthSquared(ent->lightDir) - 1.0f) > 0.2f)
		{
			glColor3f(1, 0, 0);
		}
		else
		{
			glColor3f(ent->directedLight[0] / 255, ent->directedLight[1] / 255, ent->directedLight[2] / 255);
		}
		glLineWidth(3);
		glBegin(GL_LINES);
		glVertex3fv(temp);
		VectorMA(temp, 32, ent->lightDir, temp);
		glVertex3fv(temp);
		glEnd();
		glLineWidth(1);
	}
	// normals drawing
	else
	{
		int i;

		glBegin(GL_LINES);
		for (i = 0 ; i < input->numVertexes ; i++)
		{
			glVertex3fv(input->xyz[i]);
			VectorMA(input->xyz[i], r_normalLength->value, input->normal[i], temp);
			glVertex3fv(temp);
		}
		glEnd();
	}

	glDepthRange(0, 1);
}

/**
 * @brief We must set some things up before beginning any tesselation,
 * because a surface may be forced to perform a RB_End due
 * to overflow.
 *
 * @param[in] shader
 * @param[in] fogNum
 */
void RB_BeginSurface(shader_t *shader, int fogNum)
{
	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes               = 0;
	tess.numVertexes              = 0;
	tess.shader                   = state;
	tess.fogNum                   = fogNum;
	tess.dlightBits               = 0; // will be OR'd in by surface functions
	tess.xstages                  = state->stages;
	tess.numPasses                = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime != 0.0 && tess.shaderTime >= tess.shader->clampTime)
	{
		tess.shaderTime = tess.shader->clampTime;
	}
}

/**
 * @brief DrawMultitextured
 * @param[in] input
 * @param[in] stage
 *
 * @note output = t0 * t1 or t0 + t1
 *
 * t0 = most upstream according to spec
 * t1 = most downstream according to spec
 */
static void DrawMultitextured(shaderCommands_t *input, int stage)
{
	shaderStage_t *pStage = tess.xstages[stage];

	if (tess.shader->noFog && pStage->isFogged)
	{
		R_FogOn();
	}
	else if (tess.shader->noFog && !pStage->isFogged)
	{
		R_FogOff(); // turn it back off
	}
	else            // make sure it's on
	{
		R_FogOn();
	}

	GL_State(pStage->stateBits);

	// this is an ugly hack to work around a GeForce driver
	// bug with multitexture and clip planes
	if (backEnd.viewParms.isPortal)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// base
	GL_SelectTexture(0);
	glTexCoordPointer(2, GL_FLOAT, 0, input->svars.texcoords[0]);
	R_BindAnimatedImage(&pStage->bundle[0]);

	// lightmap/secondary pass
	GL_SelectTexture(1);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if (r_lightMap->integer)
	{
		GL_TexEnv(GL_REPLACE);
	}
	else
	{
		GL_TexEnv(tess.shader->multitextureEnv);
	}

	glTexCoordPointer(2, GL_FLOAT, 0, input->svars.texcoords[1]);

	R_BindAnimatedImage(&pStage->bundle[1]);

	R_DrawElements(input->numIndexes, input->indexes);

	// disable texturing on TEXTURE1, then select TEXTURE0
	//glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisable(GL_TEXTURE_2D);

	GL_SelectTexture(0);
}

/**
 * @brief Perform all dynamic lighting with a single rendering pass
 */
static void DynamicLightSinglePass(void)
{
	int      i, l, a, b, c, color, *intColors;
	vec3_t   origin;
	byte     *colors;
	unsigned hitIndexes[SHADER_MAX_INDEXES];
	int      numIndexes;
	float    radius, radiusInverseCubed;
	float    intensity, remainder, modulate;
	vec3_t   floatColor, dir;
	dlight_t *dl;

	// early out
	if (backEnd.refdef.num_dlights == 0)
	{
		return;
	}

	// clear colors
	Com_Memset(tess.svars.colors, 0, sizeof(tess.svars.colors));

	// walk light list
	for (l = 0; l < backEnd.refdef.num_dlights; l++)
	{
		// early out
		if (!(tess.dlightBits & (1 << l)))
		{
			continue;
		}

		// setup
		dl = &backEnd.refdef.dlights[l];
		VectorCopy(dl->transformed, origin);
		radius             = dl->radius;
		radiusInverseCubed = dl->radiusInverseCubed;
		intensity          = dl->intensity;
		floatColor[0]      = dl->color[0] * 255.0f;
		floatColor[1]      = dl->color[1] * 255.0f;
		floatColor[2]      = dl->color[2] * 255.0f;

		// directional lights have max intensity and washout remainder intensity
		if (dl->flags & REF_DIRECTED_DLIGHT)
		{
			remainder = intensity * 0.125f;
		}
		else
		{
			remainder = 0.0f;
		}

		// illuminate vertexes
		colors = tess.svars.colors[0];
		for (i = 0; i < tess.numVertexes; i++, colors += 4)
		{
			backEnd.pc.c_dlightVertexes++;

			// directional dlight, origin is a directional normal
			if (dl->flags & REF_DIRECTED_DLIGHT)
			{
				// twosided surfaces use absolute value of the calculated lighting
				modulate = intensity * DotProduct(dl->origin, tess.normal[i]);
				if (tess.shader->cullType == CT_TWO_SIDED)
				{
					modulate = Q_fabs(modulate);
				}
				modulate += remainder;
			}
			// ball dlight
			else
			{
				dir[0] = radius - Q_fabs(origin[0] - tess.xyz[i][0]);
				if (dir[0] <= 0.0f)
				{
					continue;
				}
				dir[1] = radius - Q_fabs(origin[1] - tess.xyz[i][1]);
				if (dir[1] <= 0.0f)
				{
					continue;
				}
				dir[2] = radius - Q_fabs(origin[2] - tess.xyz[i][2]);
				if (dir[2] <= 0.0f)
				{
					continue;
				}

				modulate = intensity * dir[0] * dir[1] * dir[2] * radiusInverseCubed;
			}

			// optimizations
			if (modulate < (1.0f / 128.0f))
			{
				continue;
			}
			else if (modulate > 1.0f)
			{
				modulate = 1.0f;
			}

			// add to color
			color     = colors[0] + (int)(floatColor[0] * modulate);
			colors[0] = color > 255 ? 255 : (byte)color;
			color     = colors[1] + (int)(floatColor[1] * modulate);
			colors[1] = color > 255 ? 255 : (byte)color;
			color     = colors[2] + (int)(floatColor[2] * modulate);
			colors[2] = color > 255 ? 255 : (byte)color;
		}
	}

	// build a list of triangles that need light
	intColors  = (int *) tess.svars.colors;
	numIndexes = 0;
	for (i = 0; i < tess.numIndexes; i += 3)
	{
		a = tess.indexes[i];
		b = tess.indexes[i + 1];
		c = tess.indexes[i + 2];
		if (!(intColors[a] | intColors[b] | intColors[c]))
		{
			continue;
		}
		hitIndexes[numIndexes++] = a;
		hitIndexes[numIndexes++] = b;
		hitIndexes[numIndexes++] = c;
	}

	if (numIndexes == 0)
	{
		return;
	}

	// debug code
	//for( i = 0; i < numIndexes; i++ )
	//    intColors[ hitIndexes[ i ] ] = 0x000000FF;

	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);

	// render the dynamic light pass
	R_FogOff();
	GL_Bind(tr.whiteImage);
	GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
	R_DrawElements(numIndexes, hitIndexes);
	backEnd.pc.c_totalIndexes  += numIndexes;
	backEnd.pc.c_dlightIndexes += numIndexes;
	R_FogOn();
}

#if idppc_altivec
/**
 * @brief Perform dynamic lighting with multiple rendering passes
 */
static void DynamicLightPass_altivec(void)
{
	int                  i, l, a, b, c, *intColors;
	vec_t                origin0, origin1, origin2;
	vec_t                dir0, dir1, dir2;
	byte                 *colors;
	unsigned             hitIndexes[SHADER_MAX_INDEXES];
	int                  numIndexes;
	float                radius, radiusInverseCubed;
	float                intensity, remainder, modulate;
	vec3_t               floatColor;
	dlight_t             *dl;
	float                colorMax = 255;
	vector float         floatColorVec0, floatColorVec1;
	vector float         modulateVec, colorVec, zero, colorMaxVec;
	vector short         colorShort;
	vector signed int    colorInt;
	vector unsigned char floatColorVecPerm, modulatePerm, colorChar;
	vector unsigned char vSel = VECCONST_UINT8(0x00, 0x00, 0x00, 0xff,
	                                           0x00, 0x00, 0x00, 0xff,
	                                           0x00, 0x00, 0x00, 0xff,
	                                           0x00, 0x00, 0x00, 0xff);

	// early out
	if (backEnd.refdef.num_dlights == 0)
	{
		return;
	}

	// There has to be a better way to do this so that floatColor
	// and/or modulate are already 16-byte aligned.
	floatColorVecPerm = vec_lvsl(0, (float *)floatColor);
	modulatePerm      = vec_lvsl(0, (float *)&modulate);
	modulatePerm      = (vector unsigned char)vec_splat((vector unsigned int)modulatePerm, 0);
	zero              = (vector float)vec_splat_s8(0);
	colorMaxVec       = vec_splat(vec_lde(0, &colorMax), 0);

	// walk light list
	for (l = 0; l < backEnd.refdef.num_dlights; l++)
	{
		// early out
		if (!(tess.dlightBits & (1 << l)))
		{
			continue;
		}

		// clear colors
		Com_Memset(tess.svars.colors, 0, sizeof(tess.svars.colors));

		// setup
		dl                 = &backEnd.refdef.dlights[l];
		origin0            = dl->transformed[0];
		origin1            = dl->transformed[1];
		origin2            = dl->transformed[2];
		radius             = dl->radius;
		radiusInverseCubed = dl->radiusInverseCubed;
		intensity          = dl->intensity;
		floatColor[0]      = dl->color[0] * 255.0f;
		floatColor[1]      = dl->color[1] * 255.0f;
		floatColor[2]      = dl->color[2] * 255.0f;

		floatColorVec0 = vec_ld(0, floatColor);
		floatColorVec1 = vec_ld(11, floatColor);
		floatColorVec0 = vec_perm(floatColorVec0, floatColorVec0, floatColorVecPerm);

		// directional lights have max intensity and washout remainder intensity
		if (dl->flags & REF_DIRECTED_DLIGHT)
		{
			remainder = intensity * 0.125;
		}
		else
		{
			remainder = 0.0f;
		}

		// illuminate vertexes
		colors = tess.svars.colors[0];
		for (i = 0; i < tess.numVertexes; i++, colors += 4)
		{
			backEnd.pc.c_dlightVertexes++;

			// directional dlight, origin is a directional normal
			if (dl->flags & REF_DIRECTED_DLIGHT)
			{
				// twosided surfaces use absolute value of the calculated lighting
				modulate = intensity * DotProduct(dl->origin, tess.normal[i]);
				if (tess.shader->cullType == CT_TWO_SIDED)
				{
					modulate = Q_fabs(modulate);
				}
				modulate += remainder;
			}
			// ball dlight
			else
			{
				dir0 = radius - Q_fabs(origin0 - tess.xyz[i][0]);
				if (dir0 <= 0.0f)
				{
					continue;
				}
				dir1 = radius - Q_fabs(origin1 - tess.xyz[i][1]);
				if (dir1 <= 0.0f)
				{
					continue;
				}
				dir2 = radius - Q_fabs(origin2 - tess.xyz[i][2]);
				if (dir2 <= 0.0f)
				{
					continue;
				}

				modulate = intensity * dir0 * dir1 * dir2 * radiusInverseCubed;
			}

			// optimizations
			if (modulate < (1.0f / 128.0f))
			{
				continue;
			}
			else if (modulate > 1.0f)
			{
				modulate = 1.0f;
			}

			modulateVec = vec_ld(0, (float *)&modulate);
			modulateVec = vec_perm(modulateVec, modulateVec, modulatePerm);
			colorVec    = vec_madd(floatColorVec0, modulateVec, zero);
			colorVec    = vec_max(colorVec, colorMaxVec); // clamp
			colorInt    = vec_cts(colorVec, 0); // RGBx
			colorShort  = vec_pack(colorInt, colorInt);     // RGBxRGBx
			colorChar   = vec_packsu(colorShort, colorShort); // RGBxRGBxRGBxRGBx
			colorChar   = vec_sel(colorChar, vSel, vSel);   // RGBARGBARGBARGBA replace alpha with 255
			vec_ste((vector unsigned int)colorChar, 0, (unsigned int *)colors);   // store color
		}

		// build a list of triangles that need light
		intColors  = (int *) tess.svars.colors;
		numIndexes = 0;
		for (i = 0; i < tess.numIndexes; i += 3)
		{
			a = tess.indexes[i];
			b = tess.indexes[i + 1];
			c = tess.indexes[i + 2];
			if (!(intColors[a] | intColors[b] | intColors[c]))
			{
				continue;
			}
			hitIndexes[numIndexes++] = a;
			hitIndexes[numIndexes++] = b;
			hitIndexes[numIndexes++] = c;
		}

		if (numIndexes == 0)
		{
			continue;
		}

		// debug code (fixme, there's a bug in this function!)
		//for( i = 0; i < numIndexes; i++ )
		//  intColors[ hitIndexes[ i ] ] = 0x000000FF;

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);

		R_FogOff();
		GL_Bind(tr.whiteImage);
		GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
		R_DrawElements(numIndexes, hitIndexes);
		backEnd.pc.c_totalIndexes  += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
		R_FogOn();
	}
}
#endif

/**
 * @brief DynamicLightPass_scalar
 */
static void DynamicLightPass_scalar(void)
{
	int      i, l, a, b, c, color, *intColors;
	vec3_t   origin;
	byte     *colors;
	unsigned hitIndexes[SHADER_MAX_INDEXES];
	int      numIndexes;
	float    radius, radiusInverseCubed;
	float    intensity, remainder, modulate;
	vec3_t   floatColor, dir;
	dlight_t *dl;

	// early out
	if (backEnd.refdef.num_dlights == 0)
	{
		return;
	}

	// walk light list
	for (l = 0; l < backEnd.refdef.num_dlights; l++)
	{
		// early out
		if (!(tess.dlightBits & (1 << l)))
		{
			continue;
		}

		// clear colors
		Com_Memset(tess.svars.colors, 0, sizeof(tess.svars.colors));

		// setup
		dl = &backEnd.refdef.dlights[l];
		VectorCopy(dl->transformed, origin);
		radius             = dl->radius;
		radiusInverseCubed = dl->radiusInverseCubed;
		intensity          = dl->intensity;
		floatColor[0]      = dl->color[0] * 255.0f;
		floatColor[1]      = dl->color[1] * 255.0f;
		floatColor[2]      = dl->color[2] * 255.0f;

		// directional lights have max intensity and washout remainder intensity
		if (dl->flags & REF_DIRECTED_DLIGHT)
		{
			remainder = intensity * 0.125f;
		}
		else
		{
			remainder = 0.0f;
		}

		// illuminate vertexes
		colors = tess.svars.colors[0];
		for (i = 0; i < tess.numVertexes; i++, colors += 4)
		{
			backEnd.pc.c_dlightVertexes++;

			// directional dlight, origin is a directional normal
			if (dl->flags & REF_DIRECTED_DLIGHT)
			{
				// twosided surfaces use absolute value of the calculated lighting
				modulate = intensity * DotProduct(dl->origin, tess.normal[i]);
				if (tess.shader->cullType == CT_TWO_SIDED)
				{
					modulate = Q_fabs(modulate);
				}
				modulate += remainder;
			}
			// ball dlight
			else
			{
				dir[0] = radius - Q_fabs(origin[0] - tess.xyz[i][0]);
				if (dir[0] <= 0.0f)
				{
					continue;
				}
				dir[1] = radius - Q_fabs(origin[1] - tess.xyz[i][1]);
				if (dir[1] <= 0.0f)
				{
					continue;
				}
				dir[2] = radius - Q_fabs(origin[2] - tess.xyz[i][2]);
				if (dir[2] <= 0.0f)
				{
					continue;
				}

				modulate = intensity * dir[0] * dir[1] * dir[2] * radiusInverseCubed;
			}

			// optimizations
			if (modulate < (1.0f / 128.0f))
			{
				continue;
			}
			else if (modulate > 1.0f)
			{
				modulate = 1.0f;
			}

			// set color
			color     = (int)(floatColor[0] * modulate);
			colors[0] = color > 255 ? 255 : (byte)color;
			color     = (int)(floatColor[1] * modulate);
			colors[1] = color > 255 ? 255 : (byte)color;
			color     = (int)(floatColor[2] * modulate);
			colors[2] = color > 255 ? 255 : (byte)color;
		}

		// build a list of triangles that need light
		intColors  = (int *) tess.svars.colors;
		numIndexes = 0;
		for (i = 0; i < tess.numIndexes; i += 3)
		{
			a = tess.indexes[i];
			b = tess.indexes[i + 1];
			c = tess.indexes[i + 2];
			if (!(intColors[a] | intColors[b] | intColors[c]))
			{
				continue;
			}
			hitIndexes[numIndexes++] = a;
			hitIndexes[numIndexes++] = b;
			hitIndexes[numIndexes++] = c;
		}

		if (numIndexes == 0)
		{
			continue;
		}

		// debug code (fixme, there's a bug in this function!)
		//for( i = 0; i < numIndexes; i++ )
		//  intColors[ hitIndexes[ i ] ] = 0x000000FF;

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);

		R_FogOff();
		GL_Bind(tr.whiteImage);
		GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
		R_DrawElements(numIndexes, hitIndexes);
		backEnd.pc.c_totalIndexes  += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
		R_FogOn();
	}
}

/**
 * @brief DynamicLightPass
 */
static void DynamicLightPass(void)
{
#if idppc_altivec
	if (com_altivec->integer)
	{
		// must be in a seperate function or G3 systems will crash.
		DynamicLightPass_altivec();
		return;
	}
#endif // idppc_altivec
	DynamicLightPass_scalar();
}

/**
 * @brief Blends a fog texture on top of everything else
 */
static void RB_FogPass(void)
{
	fog_t *fog;
	int   i;

	// no fog pass in snooper
	if ((tr.refdef.rdflags & RDF_SNOOPERVIEW) || tess.shader->noFog || !r_wolfFog->integer)
	{
		return;
	}

	// no world, no fogging
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texcoords[0]);

	fog = tr.world->fogs + tess.fogNum;

	for (i = 0; i < tess.numVertexes; i++)
	{
		*( int * )&tess.svars.colors[i] = fog->shader->fogParms.colorInt;
	}

	RB_CalcFogTexCoords(( float * ) tess.svars.texcoords[0]);

	GL_Bind(tr.fogImage);

	if (tess.shader->fogPass == FP_EQUAL)
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL);
	}
	else
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}

	R_DrawElements(tess.numIndexes, tess.indexes);
}

/**
 * @brief ComputeColors
 * @param[in] pStage
 */
static void ComputeColors(shaderStage_t *pStage)
{
	// rgbGen
	switch (pStage->rgbGen)
	{
	case CGEN_IDENTITY:
		Com_Memset(tess.svars.colors, 0xff, tess.numVertexes * 4);
		break;
	default:
	case CGEN_IDENTITY_LIGHTING:
		Com_Memset(tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4);
		break;
	case CGEN_LIGHTING_DIFFUSE:
		RB_CalcDiffuseColor(( unsigned char * ) tess.svars.colors);
		break;
	case CGEN_EXACT_VERTEX:
		Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
		break;
	case CGEN_CONST:
	{
		int i;

		for (i = 0; i < tess.numVertexes; i++)
		{
			*(int *)tess.svars.colors[i] = *(int *)pStage->constantColor;
		}
	}
	break;
	case CGEN_VERTEX:
		if (tr.identityLight == 1)
		{
			Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
		}
		else
		{
			int i;

			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
				tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
				tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
		break;
	case CGEN_ONE_MINUS_VERTEX:
	{
		int i;

		if (tr.identityLight == 1)
		{
			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
				tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
				tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
			}
		}
		else
		{
			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][0] = (255 - tess.vertexColors[i][0]) * tr.identityLight;
				tess.svars.colors[i][1] = (255 - tess.vertexColors[i][1]) * tr.identityLight;
				tess.svars.colors[i][2] = (255 - tess.vertexColors[i][2]) * tr.identityLight;
			}
		}
	}
	break;
	case CGEN_FOG:
	{
		int   i;
		fog_t *fog = tr.world->fogs + tess.fogNum;

		for (i = 0; i < tess.numVertexes; i++)
		{
			*( int * )&tess.svars.colors[i] = fog->shader->fogParms.colorInt;
		}
	}
	break;
	case CGEN_WAVEFORM:
		RB_CalcWaveColor(&pStage->rgbWave, ( unsigned char * ) tess.svars.colors);
		break;
	case CGEN_ENTITY:
		RB_CalcColorFromEntity(( unsigned char * ) tess.svars.colors);
		break;
	case CGEN_ONE_MINUS_ENTITY:
		RB_CalcColorFromOneMinusEntity(( unsigned char * ) tess.svars.colors);
		break;
	}

	// alphaGen
	switch (pStage->alphaGen)
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if (pStage->rgbGen != CGEN_IDENTITY)
		{
			if ((pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1) ||
			    pStage->rgbGen != CGEN_VERTEX)
			{
				int i;

				for (i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.colors[i][3] = 0xff;
				}
			}
		}
		break;
	case AGEN_CONST:
		if (pStage->rgbGen != CGEN_CONST)
		{
			int i;

			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha(&pStage->alphaWave, ( unsigned char * ) tess.svars.colors);
		break;
	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha(( unsigned char * ) tess.svars.colors);
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity(( unsigned char * ) tess.svars.colors);
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity(( unsigned char * ) tess.svars.colors);
		break;
	case AGEN_NORMALZFADE:
	{
		float    alpha, range, lowest, highest, dot;
		vec3_t   worldUp;
		int      i;
		qboolean zombieEffect = qfalse;

		if (VectorCompare(backEnd.currentEntity->e.fireRiseDir, vec3_origin))
		{
			VectorSet(backEnd.currentEntity->e.fireRiseDir, 0, 0, 1);
		}

		if (backEnd.currentEntity->e.hModel)        // world surfaces dont have an axis
		{
			VectorRotate(backEnd.currentEntity->e.fireRiseDir, backEnd.currentEntity->e.axis, worldUp);
		}
		else
		{
			VectorCopy(backEnd.currentEntity->e.fireRiseDir, worldUp);
		}

		lowest = pStage->zFadeBounds[0];
		if (lowest == -1000)        // use entity alpha
		{
			lowest       = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		highest = pStage->zFadeBounds[1];
		if (highest == -1000)       // use entity alpha
		{
			highest      = backEnd.currentEntity->e.shaderTime;
			zombieEffect = qtrue;
		}
		range = highest - lowest;
		for (i = 0; i < tess.numVertexes; i++)
		{
			dot = DotProduct(tess.normal[i], worldUp);

			// special handling for Zombie fade effect
			if (zombieEffect)
			{
				alpha  = (float)backEnd.currentEntity->e.shaderRGBA[3] * (dot + 1.0f) / 2.0f;
				alpha += (2.0f * (float)backEnd.currentEntity->e.shaderRGBA[3]) * (1.0f - (dot + 1.0f) / 2.0f);
				if (alpha > 255.0f)
				{
					alpha = 255.0f;
				}
				else if (alpha < 0.0f)
				{
					alpha = 0.0f;
				}
				tess.svars.colors[i][3] = (byte)(alpha);
				continue;
			}

			if (dot < highest)
			{
				if (dot > lowest)
				{
					if (dot < lowest + range / 2)
					{
						alpha = ((float)pStage->constantColor[3] * ((dot - lowest) / (range / 2)));
					}
					else
					{
						alpha = ((float)pStage->constantColor[3] * (1.0f - ((dot - lowest - range / 2) / (range / 2))));
					}
					if (alpha > 255.0f)
					{
						alpha = 255.0f;
					}
					else if (alpha < 0.0f)
					{
						alpha = 0.0f;
					}

					// finally, scale according to the entity's alpha
					if (backEnd.currentEntity->e.hModel)
					{
						alpha *= (float)backEnd.currentEntity->e.shaderRGBA[3] / 255.0f;
					}

					tess.svars.colors[i][3] = (byte)(alpha);
				}
				else
				{
					tess.svars.colors[i][3] = 0;
				}
			}
			else
			{
				tess.svars.colors[i][3] = 0;
			}
		}
	}
	break;
	case AGEN_VERTEX:
		if (pStage->rgbGen != CGEN_VERTEX)
		{
			int i;

			for (i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
		break;
	case AGEN_ONE_MINUS_VERTEX:
	{
		int i;

		for (i = 0; i < tess.numVertexes; i++)
		{
			tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
		}
	}
	break;
	case AGEN_PORTAL:
	{
		unsigned char alpha;
		float         len;
		vec3_t        v;
		int           i;

		for (i = 0; i < tess.numVertexes; i++)
		{
			VectorSubtract(tess.xyz[i], backEnd.viewParms.orientation.origin, v);
			len = VectorLength(v);

			len /= tess.shader->portalRange;

			if (len < 0)
			{
				alpha = 0;
			}
			else if (len > 1)
			{
				alpha = 0xff;
			}
			else
			{
				alpha = len * 0xff;
			}

			tess.svars.colors[i][3] = alpha;
		}
	}
	break;
	}

	// fog adjustment for colors to fade out as fog increases
	if (tess.fogNum && !tess.shader->noFog)
	{
		switch (pStage->adjustColorsForFog)
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog(( unsigned char * ) tess.svars.colors);
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog(( unsigned char * ) tess.svars.colors);
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog(( unsigned char * ) tess.svars.colors);
			break;
		case ACFF_NONE:
			break;
		}
	}
}

/**
 * @brief ComputeTexCoords
 * @param[in] pStage
 */
static void ComputeTexCoords(shaderStage_t *pStage)
{
	int i;
	int b;
	int tm;

	for (b = 0; b < NUM_TEXTURE_BUNDLES; b++)
	{
		// generate the texture coordinates
		switch (pStage->bundle[b].tcGen)
		{
		case TCGEN_IDENTITY:
			Com_Memset(tess.svars.texcoords[b], 0, sizeof(float) * 2 * tess.numVertexes);
			break;
		case TCGEN_TEXTURE:
			for (i = 0 ; i < tess.numVertexes ; i++)
			{
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
			}
			break;
		case TCGEN_LIGHTMAP:
			for (i = 0 ; i < tess.numVertexes ; i++)
			{
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
			}
			break;
		case TCGEN_VECTOR:
			for (i = 0 ; i < tess.numVertexes ; i++)
			{
				tess.svars.texcoords[b][i][0] = DotProduct(tess.xyz[i], pStage->bundle[b].tcGenVectors[0]);
				tess.svars.texcoords[b][i][1] = DotProduct(tess.xyz[i], pStage->bundle[b].tcGenVectors[1]);
			}
			break;
		case TCGEN_FOG:
			RB_CalcFogTexCoords(( float * ) tess.svars.texcoords[b]);
			break;
		case TCGEN_ENVIRONMENT_MAPPED:
			RB_CalcEnvironmentTexCoords(( float * ) tess.svars.texcoords[b]);
			break;
		case TCGEN_FIRERISEENV_MAPPED:
			RB_CalcFireRiseEnvTexCoords(( float * ) tess.svars.texcoords[b]);
			break;
		case TCGEN_BAD:
			return;
		}

		// alter texture coordinates
		for (tm = 0; tm < pStage->bundle[b].numTexMods ; tm++)
		{
			switch (pStage->bundle[b].texMods[tm].type)
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;        // break out of for loop
				break;

			case TMOD_SWAP:
				RB_CalcSwapTexCoords(( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords(&pStage->bundle[b].texMods[tm].wave,
				                          ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords(backEnd.currentEntity->e.shaderTexCoord,
				                       ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords(pStage->bundle[b].texMods[tm].scroll,
				                       ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords(pStage->bundle[b].texMods[tm].scale,
				                      ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_STRETCH:
				RB_CalcStretchTexCoords(&pStage->bundle[b].texMods[tm].wave,
				                        ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords(&pStage->bundle[b].texMods[tm],
				                          ( float * ) tess.svars.texcoords[b]);
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords(pStage->bundle[b].texMods[tm].rotateSpeed,
				                       ( float * ) tess.svars.texcoords[b]);
				break;

			default:
				Ren_Drop("ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle[b].texMods[tm].type, tess.shader->name);
			}
		}
	}
}

extern void R_Fog(glfog_t *curfog);

/**
 * @brief Set the fog parameters for this pass
 */
void SetIteratorFog(void)
{
	// changed for problem when you start the game with r_fastsky set to '1'
	//  if(r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		R_FogOff();
		return;
	}

	if (backEnd.refdef.rdflags & RDF_DRAWINGSKY)
	{
		if (glfogsettings[FOG_SKY].registered)
		{
			R_Fog(&glfogsettings[FOG_SKY]);
		}
		else
		{
			R_FogOff();
		}

		return;
	}

	if (skyboxportal && (backEnd.refdef.rdflags & RDF_SKYBOXPORTAL))
	{
		if (glfogsettings[FOG_PORTALVIEW].registered)
		{
			R_Fog(&glfogsettings[FOG_PORTALVIEW]);
		}
		else
		{
			R_FogOff();
		}
	}
	else
	{
		if (glfogNum > FOG_NONE)
		{
			R_Fog(&glfogsettings[FOG_CURRENT]);
		}
		else
		{
			R_FogOff();
		}
	}
}

/**
 * @brief RB_IterateStagesGeneric
 * @param[in] input
 */
static void RB_IterateStagesGeneric(shaderCommands_t *input)
{
	shaderStage_t *pStage;
	int           stage;

	for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		pStage = tess.xstages[stage];

		if (!pStage)
		{
			break;
		}

		ComputeColors(pStage);
		ComputeTexCoords(pStage);

		if (!setArraysOnce)
		{
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, input->svars.colors);
		}

		// do multitexture
		if (pStage->bundle[1].image[0] != 0)
		{
			DrawMultitextured(input, stage);
		}
		else
		{
			int fadeStart;

			if (!setArraysOnce)
			{
				glTexCoordPointer(2, GL_FLOAT, 0, input->svars.texcoords[0]);
			}

			// set state
			R_BindAnimatedImage(&pStage->bundle[0]);

			// per stage fogging (detail textures)
			if (tess.shader->noFog && pStage->isFogged)
			{
				R_FogOn();
			}
			else if (tess.shader->noFog && !pStage->isFogged)
			{
				R_FogOff(); // turn it back off
			}
			else        // make sure it's on
			{
				R_FogOn();
			}

			// fading model stuff
			fadeStart = backEnd.currentEntity->e.fadeStartTime;

			if (fadeStart)
			{
				int fadeEnd = backEnd.currentEntity->e.fadeEndTime;

				if (fadeStart > tr.refdef.time)           // has not started to fade yet
				{
					GL_State(pStage->stateBits);
				}
				else
				{
					int          i;
					unsigned int tempState;
					float        alphaval;

					if (fadeEnd < tr.refdef.time)         // entity faded out completely
					{
						continue;
					}

					alphaval = (float)(fadeEnd - tr.refdef.time) / (float)(fadeEnd - fadeStart);

					tempState = pStage->stateBits;
					// remove the current blend, and don't write to Z buffer
					tempState &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_DEPTHMASK_TRUE);
					// set the blend to src_alpha, dst_one_minus_src_alpha
					tempState |= (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
					GL_State(tempState);
					GL_Cull(CT_FRONT_SIDED);
					// modulate the alpha component of each vertex in the render list
					for (i = 0; i < tess.numVertexes; i++)
					{
						tess.svars.colors[i][0] *= alphaval;
						tess.svars.colors[i][1] *= alphaval;
						tess.svars.colors[i][2] *= alphaval;
						tess.svars.colors[i][3] *= alphaval;
					}
				}
			}
			// lightmap stages should be GL_ONE GL_ZERO so they can be seen
			else if (r_lightMap->integer && (pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap))
			{
				unsigned int stateBits = (pStage->stateBits & ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) |
				                         (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);

				GL_State(stateBits);
			}
			else
			{
				GL_State(pStage->stateBits);
			}

			// draw
			R_DrawElements(input->numIndexes, input->indexes);
		}

		// allow skipping out to show just lightmaps during development
		if (r_lightMap->integer && (pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap))
		{
			break;
		}
	}
}

/**
 * @brief RB_StageIteratorGeneric
 */
void RB_StageIteratorGeneric(void)
{
	shaderCommands_t *input  = &tess;
	shader_t         *shader = input->shader;

	RB_DeformTessGeometry();

	Ren_LogComment("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name);

	// set GL fog
	SetIteratorFog();

	// set face culling appropriately
	GL_Cull(shader->cullType);

	// set polygon offset if necessary
	if (shader->polygonOffset)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// if there is only a single pass then we can enable color
	// and texture arrays before we compile, otherwise we need
	// to avoid compiling those arrays since they will change
	// during multipass rendering
	if (tess.numPasses > 1 || shader->multitextureEnv)
	{
		setArraysOnce = qfalse;
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else
	{
		setArraysOnce = qtrue;

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texcoords[0]);
	}

	// lock XYZ
	glVertexPointer(3, GL_FLOAT, 16, input->xyz);   // padded for SIMD
	if (glLockArraysEXT)
	{
		glLockArraysEXT(0, input->numVertexes);
		Ren_LogComment("glLockArraysEXT\n");
	}

	// enable color and texcoord arrays after the lock if necessary
	if (!setArraysOnce)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
	}

	// call shader function
	RB_IterateStagesGeneric(input);

	// now do any dynamic lighting needed
	//tess.dlightBits = 255;  // HACK!
	//if( tess.dlightBits && tess.shader->sort <= SS_OPAQUE &&
	if (tess.dlightBits && tess.shader->fogPass &&
	    !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY)))
	{
		if (r_dynamicLight->integer == 2)
		{
			DynamicLightPass();
		}
		else
		{
			DynamicLightSinglePass();
		}
	}

	// now do fog
	if (tess.fogNum && tess.shader->fogPass)
	{
		RB_FogPass();
	}

	// unlock arrays
	if (glUnlockArraysEXT)
	{
		glUnlockArraysEXT();
		Ren_LogComment("glUnlockArraysEXT\n");
	}

	// reset polygon offset
	if (shader->polygonOffset)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

/**
 * @brief RB_StageIteratorVertexLitTexture
 */
void RB_StageIteratorVertexLitTexture(void)
{
	shaderCommands_t *input  = &tess;
	shader_t         *shader = input->shader;

	// compute colors
	RB_CalcDiffuseColor(( unsigned char * ) tess.svars.colors);

	Ren_LogComment("--- RB_StageIteratorVertexLitTexturedUnfogged( %s ) ---\n", tess.shader->name);

	// set GL fog
	SetIteratorFog();

	// set face culling appropriately
	GL_Cull(shader->cullType);

	// set arrays and lock
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);
	glTexCoordPointer(2, GL_FLOAT, 16, tess.texCoords[0][0]);
	glVertexPointer(3, GL_FLOAT, 16, input->xyz);

	if (glLockArraysEXT)
	{
		glLockArraysEXT(0, input->numVertexes);
		Ren_LogComment("glLockArraysEXT\n");
	}

	// call special shade routine
	R_BindAnimatedImage(&tess.xstages[0]->bundle[0]);
	GL_State(tess.xstages[0]->stateBits);
	R_DrawElements(input->numIndexes, input->indexes);

	// now do any dynamic lighting needed
	//if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE )
	if (tess.dlightBits && tess.shader->fogPass &&
	    !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY)))
	{
		if (r_dynamicLight->integer == 2)
		{
			DynamicLightPass();
		}
		else
		{
			DynamicLightSinglePass();
		}
	}

	// now do fog
	if (tess.fogNum && tess.shader->fogPass)
	{
		RB_FogPass();
	}

	// unlock arrays
	if (glUnlockArraysEXT)
	{
		glUnlockArraysEXT();
		Ren_LogComment("glUnlockArraysEXT\n");
	}
}

//define    REPLACE_MODE

/**
 * @brief RB_StageIteratorLightmappedMultitexture
 */
void RB_StageIteratorLightmappedMultitexture(void)
{
	shaderCommands_t *input  = &tess;
	shader_t         *shader = input->shader;

	Ren_LogComment("--- RB_StageIteratorLightmappedMultitexture( %s ) ---\n", tess.shader->name);

	// set GL fog
	SetIteratorFog();

	// set face culling appropriately
	GL_Cull(shader->cullType);

	// set color, pointers, and lock
	GL_State(GLS_DEFAULT);
	glVertexPointer(3, GL_FLOAT, 16, input->xyz);

#ifdef REPLACE_MODE
	glDisableClientState(GL_COLOR_ARRAY);
	glColor3f(1, 1, 1);
	glShadeModel(GL_FLAT);
#else
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.constantColor255);
#endif

	// select base stage
	GL_SelectTexture(0);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	R_BindAnimatedImage(&tess.xstages[0]->bundle[0]);
	glTexCoordPointer(2, GL_FLOAT, 16, tess.texCoords[0][0]);

	// configure second stage
	GL_SelectTexture(1);
	glEnable(GL_TEXTURE_2D);
	if (r_lightMap->integer)
	{
		GL_TexEnv(GL_REPLACE);
	}
	else
	{
		GL_TexEnv(GL_MODULATE);
	}

	// modified for snooper
	if (tess.xstages[0]->bundle[1].isLightmap && (backEnd.refdef.rdflags & RDF_SNOOPERVIEW))
	{
		GL_Bind(tr.whiteImage);
	}
	else
	{
		R_BindAnimatedImage(&tess.xstages[0]->bundle[1]);
	}

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 16, tess.texCoords[0][1]);

	// lock arrays
	if (glLockArraysEXT)
	{
		glLockArraysEXT(0, input->numVertexes);
		Ren_LogComment("glLockArraysEXT\n");
	}

	R_DrawElements(input->numIndexes, input->indexes);

	// disable texturing on TEXTURE1, then select TEXTURE0
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(0);
#ifdef REPLACE_MODE
	GL_TexEnv(GL_MODULATE);
	glShadeModel(GL_SMOOTH);
#endif

	// now do any dynamic lighting needed
	//if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE )
	if (tess.dlightBits && tess.shader->fogPass &&
	    !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY)))
	{
		if (r_dynamicLight->integer == 2)
		{
			DynamicLightPass();
		}
		else
		{
			DynamicLightSinglePass();
		}
	}

	// now do fog
	if (tess.fogNum && tess.shader->fogPass)
	{
		RB_FogPass();
	}

	// unlock arrays
	if (glUnlockArraysEXT)
	{
		glUnlockArraysEXT();
		Ren_LogComment("glUnlockArraysEXT\n")
	}
}

/**
 * @brief RB_EndSurface
 */
void RB_EndSurface(void)
{
	shaderCommands_t *input = &tess;

	if (input->numIndexes == 0)
	{
		return;
	}

	if (input->indexes[SHADER_MAX_INDEXES - 1] != 0)
	{
		Ren_Drop("RB_EndSurface() - input->maxShaderIndicies(%i) hit", SHADER_MAX_INDEXES);
	}
	if (input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0.f)
	{
		Ren_Drop("RB_EndSurface() - input->maxShaderVerts(%i) hit", SHADER_MAX_VERTEXES);
	}

	if (tess.shader == tr.shadowShader)
	{
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if (r_debugSort->integer && r_debugSort->integer < tess.shader->sort)
	{
		return;
	}

	// update performance counters
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes     += tess.numVertexes;
	backEnd.pc.c_indexes      += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	// call off to shader specific tess end function
	tess.currentStageIteratorFunc();

	// draw debugging stuff
	if (r_showTris->integer)
	{
		DrawTris(input);
	}
	if (r_showNormals->integer)
	{
		DrawNormals(input);
	}

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes  = 0;
	tess.numVertexes = 0;

	Ren_LogComment("----------\n");
}
