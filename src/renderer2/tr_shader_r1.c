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
 * @file renderer2/tr_shader_r1.c
 * @brief Parsing and definition of shaders from r1 scripts folder
 */

#include "tr_shader.h"

static char **shaderTextHashTableR1[MAX_SHADERTEXT_HASH];
static char *s_shaderTextR1;

extern shaderTable_t *shaderTableHashTable[MAX_SHADERTABLE_HASH];

extern shader_t        shader;
extern dynamicShader_t *dshader;
extern shaderTable_t   table;
extern shaderStage_t   stages[MAX_SHADER_STAGES];
extern char            implicitMap[MAX_QPATH];
extern unsigned        implicitStateBits;
extern cullType_t      implicitCullType;

/**
 * @brief ParseVector
 * @param[in,out] text
 * @param[in] count
 * @param[out] v
 * @return qtrue for success else qfalse
 */
static qboolean ParseVector(char **text, int count, float *v)
{
	char *token;
	int  i;

	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, "("))
	{
		Ren_Warning("WARNING: missing parenthesis '(' in shader '%s' of token '%s'\n", shader.name, token);
		return qfalse;
	}

	for (i = 0; i < count; i++)
	{
		token = COM_ParseExt(text, qfalse);
		if (!token[0])
		{
			Ren_Warning("WARNING: missing vector element in shader '%s' - no token\n", shader.name);
			return qfalse;
		}
		v[i] = atof(token);
	}

	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, ")"))
	{
		Ren_Warning("WARNING: missing parenthesis ')' in shader '%s' of token '%s'\n", shader.name, token);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief The current text pointer is at the explicit text definition of the
 * shader. Parse it into the global shader variable.  Later functions
 * will optimize it.
 * @param[in,out] _text
 * @return
 */
qboolean ParseShaderR1(char *_text)
{
	char **text = &_text;
	char *token;
	int  s = 0;

	shader.explicitlyDefined = qtrue;

	token = COM_ParseExt2(text, qtrue);

	if (token[0] != '{')
	{
		Ren_Warning("WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name);
		return qfalse;
	}

	while (1)
	{
		token = COM_ParseExt2(text, qtrue);
		if (!token[0])
		{
			Ren_Warning("WARNING: no concluding '}' in shader %s\n", shader.name);
			return qfalse;
		}

		// end of shader definition
		if (token[0] == '}')
		{
			break;
		}
		// stage definition
		else if (token[0] == '{')
		{
			if (s >= MAX_SHADER_STAGES)
			{
				Ren_Warning("WARNING: too many stages in shader %s (max is %i)\n", shader.name, MAX_SHADER_STAGES);
				return qfalse;
			}

			if (!ParseStage(&stages[s], text))
			{
				Ren_Warning("WARNING: can't parse stages of shader %s @[%.50s ...]\n", shader.name, _text);
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if (!Q_stricmpn(token, "qer", 3))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip description
		else if (!Q_stricmp(token, "description"))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip renderbump
		else if (!Q_stricmp(token, "renderbump"))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip unsmoothedTangents
		else if (!Q_stricmp(token, "unsmoothedTangents"))
		{
			Ren_Warning("WARNING: unsmoothedTangents keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// skip guiSurf
		else if (!Q_stricmp(token, "guiSurf"))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip decalInfo
		else if (!Q_stricmp(token, "decalInfo"))
		{
			Ren_Warning("WARNING: decalInfo keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
			continue;
		}
		// skip Quake4's extra material types
		else if (!Q_stricmp(token, "materialType"))
		{
			Ren_Warning("WARNING: materialType keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
			continue;
		}
		// skip Prey's extra material types
		else if (!Q_stricmpn(token, "matter", 6))
		{
			//Ren_Warning( "WARNING: materialType keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
			continue;
		}
		// sun parms
		else if (!Q_stricmp(token, "xmap_sun") || !Q_stricmp(token, "q3map_sun") || !Q_stricmp(token, "q3map_sunExt"))
		{
			float a, b;

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			tr.sunLight[0] = atof(token);

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			tr.sunLight[1] = atof(token);


			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			tr.sunLight[2] = atof(token);

			VectorNormalize(tr.sunLight);

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			a = atof(token);
			VectorScale(tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			a = atof(token);
			a = a / 180 * M_PI;

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'xmap_sun' keyword in shader '%s'\n", shader.name);
				continue;
			}
			b = atof(token);
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos(a) * cos(b);
			tr.sunDirection[1] = sin(a) * cos(b);
			tr.sunDirection[2] = sin(b);

			SkipRestOfLine(text); // skip q3map_sunExt additional parms
			continue;
		}
		// noShadows
		else if (!Q_stricmp(token, "noShadows"))
		{
			shader.noShadows = qtrue;
			continue;
		}
		// noSelfShadow
		else if (!Q_stricmp(token, "noSelfShadow"))
		{
			Ren_Warning("WARNING: noSelfShadow keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// forceShadows
		else if (!Q_stricmp(token, "forceShadows"))
		{
			Ren_Warning("WARNING: forceShadows keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// forceOverlays
		else if (!Q_stricmp(token, "forceOverlays"))
		{
			Ren_Warning("WARNING: forceOverlays keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// noPortalFog
		else if (!Q_stricmp(token, "noPortalFog"))
		{
			Ren_Warning("WARNING: noPortalFog keyword not supported in shader '%s'\n", shader.name);
			continue;
		}
		// fogLight
		else if (!Q_stricmp(token, "fogLight"))
		{
			Ren_Warning("WARNING: fogLight keyword not supported in shader '%s'\n", shader.name);
			shader.fogLight = qtrue;
			continue;
		}
		// blendLight
		else if (!Q_stricmp(token, "blendLight"))
		{
			Ren_Warning("WARNING: blendLight keyword not supported in shader '%s'\n", shader.name);
			shader.blendLight = qtrue;
			continue;
		}
		// ambientLight
		else if (!Q_stricmp(token, "ambientLight"))
		{
			Ren_Warning("WARNING: ambientLight keyword not supported in shader '%s'\n", shader.name);
			shader.ambientLight = qtrue;
			continue;
		}
		// volumetricLight
		else if (!Q_stricmp(token, "volumetricLight"))
		{
			shader.volumetricLight = qtrue;
			continue;
		}
		// translucent
		else if (!Q_stricmp(token, "translucent"))
		{
			shader.translucent = qtrue;
			continue;
		}
		// forceOpaque
		else if (!Q_stricmp(token, "forceOpaque"))
		{
			shader.forceOpaque = qtrue;
			continue;
		}
		// forceSolid
		else if (!Q_stricmp(token, "forceSolid") || !Q_stricmp(token, "solid"))
		{
			continue;
		}
		else if (!Q_stricmp(token, "deformVertexes") || !Q_stricmp(token, "deform"))
		{
			ParseDeform(text);
			continue;
		}
		else if (!Q_stricmp(token, "tesssize"))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip noFragment
		else if (!Q_stricmp(token, "noFragment"))
		{
			continue;
		}
		else if (!Q_stricmp(token, "clampTime"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0])
			{
				shader.clampTime = atof(token);
			}
			else
			{
				Ren_Warning("WARNING: 'clampTime' incomplete - missing time value in shader '%s' - time not set.\n", shader.name);
			}
			continue;
		}
		// skip stuff that only the xmap needs
		else if (!Q_stricmpn(token, "xmap", 4) || !Q_stricmpn(token, "q3map", 5))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip stuff that only xmap or the server needs
		else if (!Q_stricmp(token, "surfaceParm"))
		{
			ParseSurfaceParm(text);
			continue;
		}
		// no mip maps
		else if (!Q_stricmp(token, "nomipmap") || !Q_stricmp(token, "nomipmaps"))
		{
			shader.filterType = FT_LINEAR;
			shader.noPicMip   = qtrue;
			continue;
		}
		// no picmip adjustment
		else if (!Q_stricmp(token, "nopicmip"))
		{
			shader.noPicMip = qtrue;
			continue;
		}
		// RF, allow each shader to permit compression if available
		else if (!Q_stricmp(token, "allowcompress"))
		{
			shader.uncompressed = qfalse;
			continue;
		}
		else if (!Q_stricmp(token, "nocompress"))
		{
			shader.uncompressed = qtrue;
			continue;
		}
		// polygonOffset
		else if (!Q_stricmp(token, "polygonOffset"))
		{
			shader.polygonOffset = qtrue;
			continue;
		}
		// parallax mapping
		else if (!Q_stricmp(token, "parallax"))
		{
			shader.parallax = qtrue;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if (!Q_stricmp(token, "entityMergable"))
		{
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if (!Q_stricmp(token, "fogParms"))
		{
			if (!ParseVector(text, 3, shader.fogParms.color))
			{
				return qfalse;
			}

			//shader.fogParms.colorInt = ColorBytes4(shader.fogParms.color[0] * tr.identityLight,
			//                                       shader.fogParms.color[1] * tr.identityLight,
			//                                       shader.fogParms.color[2] * tr.identityLight, 1.0);

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: 'fogParms' incomplete - missing opacity value in shader '%s' set to 1\n", shader.name);
				shader.fogParms.depthForOpaque = 1;
			}
			else
			{
				shader.fogParms.depthForOpaque = atof(token);
				shader.fogParms.depthForOpaque = shader.fogParms.depthForOpaque < 1 ? 1 : shader.fogParms.depthForOpaque;
			}
			//shader.fogParms.tcScale = 1.0f / shader.fogParms.depthForOpaque;

			shader.fogVolume = qtrue;
			shader.sort      = SS_FOG;

			// skip any old gradient directions
			SkipRestOfLine(text);
			continue;
		}
		// noFog
		else if (!Q_stricmp(token, "noFog"))
		{
			shader.noFog = qtrue;
			continue;
		}
		// portal
		else if (!Q_stricmp(token, "portal"))
		{
			shader.sort     = SS_PORTAL;
			shader.isPortal = qtrue;
			continue;
		}
		// portal or mirror
		else if (!Q_stricmp(token, "mirror"))
		{
			shader.sort     = SS_PORTAL;
			shader.isPortal = qtrue;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if (!Q_stricmp(token, "skyparms"))
		{
			ParseSkyParms(text);
			continue;
		}
		// This is fixed fog for the skybox/clouds determined solely by the shader
		// it will not change in a level and will not be necessary
		// to force clients to use a sky fog the server says to.
		// skyfogvars <(r,g,b)> <dist>
		else if (!Q_stricmp(token, "skyfogvars"))
		{
			vec3_t fogColor;

			if (!ParseVector(text, 3, fogColor))
			{
				return qfalse;
			}
			token = COM_ParseExt(text, qfalse);

			if (!token[0])
			{
				Ren_Warning("WARNING: missing density value for sky fog\n");
				continue;
			}

			if (atof(token) > 1)
			{
				Ren_Warning("WARNING: last value for skyfogvars is 'density' which needs to be 0.0-1.0\n");
				continue;
			}

			RE_SetFog(FOG_SKY, 0, 5, fogColor[0], fogColor[1], fogColor[2], atof(token));

			continue;
		}
		// ET waterfogvars
		else if (!Q_stricmp(token, "waterfogvars"))
		{
			vec3_t watercolor;
			float  fogvar;

			if (!ParseVector(text, 3, watercolor))
			{
				return qfalse;
			}
			token = COM_ParseExt(text, qfalse);

			if (!token[0])
			{
				Ren_Warning("WARNING: missing density/distance value for water fog\n");
				continue;
			}

			fogvar = atof(token);

			// right now allow one water color per map.  I'm sure this will need
			//          to change at some point, but I'm not sure how to track fog parameters
			//          on a "per-water volume" basis yet.
			if (fogvar == 0)
			{                   // '0' specifies "use the map values for everything except the fog color
				// TODO
			}
			else if (fogvar > 1)
			{                   // distance "linear" fog
				RE_SetFog(FOG_WATER, 0, fogvar, watercolor[0], watercolor[1], watercolor[2], 1.1);
			}
			else
			{                   // density "exp" fog
				RE_SetFog(FOG_WATER, 0, 5, watercolor[0], watercolor[1], watercolor[2], fogvar);
			}
			continue;
		}
		// ET fogvars
		else if (!Q_stricmp(token, "fogvars"))
		{
			vec3_t fogColor;
			float  fogDensity;
			int    fogFar;

			if (!ParseVector(text, 3, fogColor))
			{
				return qfalse;
			}

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing density value for the fog\n");
				continue;
			}

			// NOTE:   fogFar > 1 means the shader is setting the farclip, < 1 means setting
			//         density (so old maps or maps that just need softening fog don't have to care about farclip)

			fogDensity = atof(token);
			if (fogDensity > 1)
			{                   // linear
				fogFar = fogDensity;
			}
			else
			{
				fogFar = 5;
			}

			RE_SetFog(FOG_MAP, 0, fogFar, fogColor[0], fogColor[1], fogColor[2], fogDensity);
			RE_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 50, 0, 0, 0, 0);
			continue;
		}
		// ET sunshader <name>
		else if (!Q_stricmp(token, "sunshader"))
		{
			size_t tokenLen;

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing shader name for 'sunshader'\n");
				continue;
			}

			// Don't call tr.sunShader = R_FindShader(token, SHADER_3D_STATIC, qtrue);
			// because it breaks the computation of the current shader
			tokenLen         = strlen(token) + 1;
			tr.sunShaderName = (char *)ri.Hunk_Alloc(sizeof(char) * tokenLen, h_low);
			Q_strncpyz(tr.sunShaderName, token, tokenLen);
		}
		else if (!Q_stricmp(token, "lightgridmulamb"))
		{
			// ambient multiplier for lightgrid
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing value for 'lightgrid ambient multiplier'\n");
				continue;
			}
			if (atof(token) > 0)
			{
				tr.lightGridMulAmbient = atof(token);
			}
		}
		else if (!Q_stricmp(token, "lightgridmuldir"))
		{
			// directional multiplier for lightgrid
			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing value for 'lightgrid directional multiplier'\n");
				continue;
			}
			if (atof(token) > 0)
			{
				tr.lightGridMulDirected = atof(token);
			}
		}
		// light <value> determines flaring in xmap, not needed here
		else if (!Q_stricmp(token, "light"))
		{
			(void) COM_ParseExt2(text, qfalse);
			continue;
		}
		// cull <face>
		else if (!Q_stricmp(token, "cull"))
		{
			token = COM_ParseExt2(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing cull parms in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "none") || !Q_stricmp(token, "twoSided") || !Q_stricmp(token, "disable"))
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if (!Q_stricmp(token, "back") || !Q_stricmp(token, "backside") || !Q_stricmp(token, "backsided"))
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else if (!Q_stricmp(token, "front"))
			{
				// CT_FRONT_SIDED is set per default see R_FindShader - nothing to do just don't throw a warning
			}
			else
			{
				Ren_Warning("WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name);
			}
			continue;
		}
		// distancecull <opaque distance> <transparent distance> <alpha threshold>
		else if (!Q_stricmp(token, "distancecull"))
		{
			int i;

			for (i = 0; i < 3; i++)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
				{
					Ren_Warning("WARNING: missing distancecull parms in shader '%s'\n", shader.name);
				}
				else
				{
					shader.distanceCull[i] = atof(token);
				}
			}

			if (shader.distanceCull[1] - shader.distanceCull[0] > 0)
			{
				// distanceCull[ 3 ] is an optimization
				shader.distanceCull[3] = 1.0f / (shader.distanceCull[1] - shader.distanceCull[0]);
			}
			else
			{
				shader.distanceCull[0] = 0;
				shader.distanceCull[1] = 0;
				shader.distanceCull[2] = 0;
				shader.distanceCull[3] = 0;
			}
			continue;
		}
		// twoSided
		else if (!Q_stricmp(token, "twoSided"))
		{
			shader.cullType = CT_TWO_SIDED;
			continue;
		}
		// backSided
		else if (!Q_stricmp(token, "backSided"))
		{
			shader.cullType = CT_BACK_SIDED;
			continue;
		}
		// clamp
		else if (!Q_stricmp(token, "clamp"))
		{
			shader.wrapType = WT_CLAMP;
			continue;
		}
		// edgeClamp
		else if (!Q_stricmp(token, "edgeClamp"))
		{
			shader.wrapType = WT_EDGE_CLAMP;
			continue;
		}
		// zeroClamp
		else if (!Q_stricmp(token, "zeroclamp"))
		{
			shader.wrapType = WT_ZERO_CLAMP;
			continue;
		}
		// alphaZeroClamp
		else if (!Q_stricmp(token, "alphaZeroClamp"))
		{
			shader.wrapType = WT_ALPHA_ZERO_CLAMP;
			continue;
		}
		// sort
		else if (!Q_stricmp(token, "sort"))
		{
			ParseSort(text);
			continue;
		}
		// implicit default mapping to eliminate redundant/incorrect explicit shader stages
		else if (!Q_stricmpn(token, "implicit", 8))
		{
			//Ren_Warning( "WARNING: keyword '%s' not supported in shader '%s'\n", token, shader.name);
			//SkipRestOfLine(text);

			// set implicit mapping state
			if (!Q_stricmp(token, "implicitBlend"))
			{
				//implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_128;
				implicitCullType  = CT_TWO_SIDED;
			}
			else if (!Q_stricmp(token, "implicitMask"))
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_128;
				implicitCullType  = CT_TWO_SIDED;
			}
			else                // "implicitMap"
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE;
				implicitCullType  = CT_FRONT_SIDED;
			}

			// get image
			token = COM_ParseExt(text, qfalse);
			if (token[0] != '\0')
			{
				Q_strncpyz(implicitMap, token, sizeof(implicitMap));
			}
			else
			{
				implicitMap[0] = '-';
				implicitMap[1] = '\0';
			}

			continue;
		}
		// spectrum
		else if (!Q_stricmp(token, "spectrum"))
		{
			Ren_Warning("WARNING: spectrum keyword not supported in shader '%s'\n", shader.name);

			token = COM_ParseExt2(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parm for 'spectrum' keyword in shader '%s'\n", shader.name);
				continue;
			}
			shader.spectrum      = qtrue;
			shader.spectrumValue = Q_atoi(token);
			continue;
		}
		// diffuseMap <image>
		else if (!Q_stricmp(token, "diffuseMap"))
		{
			ParseDiffuseMap(&stages[s], text);
			s++;
			continue;
		}
		// normalMap <image>
		else if (!Q_stricmp(token, "normalMap") || !Q_stricmp(token, "bumpMap"))
		{
			ParseNormalMap(&stages[s], text);
			s++;
			continue;
		}
		// specularMap <image>
		else if (!Q_stricmp(token, "specularMap"))
		{
			ParseSpecularMap(&stages[s], text);
			s++;
			continue;
		}
		// glowMap <image>
		else if (!Q_stricmp(token, "glowMap"))
		{
			ParseGlowMap(&stages[s], text);
			s++;
			continue;
		}
		// reflectionMap <image>
		else if (!Q_stricmp(token, "reflectionMap"))
		{
			ParseReflectionMap(&stages[s], text);
			s++;
			continue;
		}
		// reflectionMapBlended <image>
		else if (!Q_stricmp(token, "reflectionMapBlended"))
		{
			ParseReflectionMapBlended(&stages[s], text);
			s++;
			continue;
		}
		// lightMap <image>
		else if (!Q_stricmp(token, "lightMap"))
		{
			Ren_Warning("WARNING: obsolete lightMap keyword not supported in shader '%s'\n", shader.name);
			SkipRestOfLine(text);
			continue;
		}
		// lightFalloffImage <image>
		else if (!Q_stricmp(token, "lightFalloffImage"))
		{
			ParseLightFalloffImage(&stages[s], text);
			s++;
			continue;
		}
		// Doom 3 DECAL_MACRO
		else if (!Q_stricmp(token, "DECAL_MACRO"))
		{
			shader.polygonOffset      = qtrue;
			shader.sort               = SS_DECAL;
			SurfaceParm("discrete");
			SurfaceParm("noShadows");
			continue;
		}
		// Prey DECAL_ALPHATEST_MACRO
		else if (!Q_stricmp(token, "DECAL_ALPHATEST_MACRO"))
		{
			// what's different?
			shader.polygonOffset      = qtrue;
			shader.sort               = SS_DECAL;
			SurfaceParm("discrete");
			SurfaceParm("noShadows");
			continue;
		}
		else if (SurfaceParm(token))
		{
			continue;
		}
		else
		{
			Ren_Warning("WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name);
			SkipRestOfLine(text);
			continue;
		}
	}

	// ignore shaders that don't have any stages, unless it is a sky or fog
	if (s == 0 && !shader.forceOpaque && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG) && implicitMap[0] == '\0')
	{
		return qfalse;
	}

	return qtrue;
}

//========================================================================================

/**
 * @brief Scans the combined text description of all the shader files for
 * the given shader name.
 * @param[in] shaderName
 * @return NULL if not found, otherwise it will return a valid shader
 */
char *FindShaderInShaderTextR1(const char *shaderName)
{
	char *token, *p;
	int  i, hash;

	// if we have any dynamic shaders loaded, check them first
	if (dshader)
	{
		dynamicShader_t *dptr = dshader;
		char            *q;
		i = 0;

		while (dptr)
		{
			if (!dptr->shadertext || !strlen(dptr->shadertext))
			{
				Ren_Warning("WARNING: dynamic shader %s(%d) has no shadertext\n", shaderName, i);
			}
			else
			{
				q = dptr->shadertext;

				token = COM_ParseExt(&q, qtrue);

				if ((token[0] != 0) && !Q_stricmp(token, shaderName))
				{
					//ri.Printf( PRINT_ALL, "Found dynamic shader [%s] with shadertext [%s]\n", shadername, dptr->shadertext );
					return q;
				}
			}
			i++;
			dptr = dptr->next;
		}
	}

	hash = generateHashValue(shaderName, MAX_SHADERTEXT_HASH);

	for (i = 0; shaderTextHashTableR1[hash][i]; i++)
	{
		p     = shaderTextHashTableR1[hash][i];
		token = COM_ParseExt(&p, qtrue);
		if (!Q_stricmp(token, shaderName))
		{
			//Ren_Print("found shader '%s' by hashing\n", shaderName);
			return p;
		}
	}

	return NULL;
}

/**
 * @brief Finds and loads all .shader files, combining them into
 * a single large text block that can be scanned for shader names
 */
int ScanAndLoadShaderFilesR1()
{
	char         **shaderFiles;
	char         *buffers[MAX_SHADER_FILES];
	char         *p;
	int          numShaderFiles, i;
	char         *oldp, *token, *textEnd;
	char         **hashMem;
	int          shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash;
	unsigned int size;
	char         filename[MAX_QPATH];
	long         sum = 0, summand;

	Com_Memset(buffers, 0, sizeof(buffers));
	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	// scan for shader files
	shaderFiles = ri.FS_ListFiles("scripts", ".shader", &numShaderFiles);

	if (!shaderFiles || !numShaderFiles)
	{
		Ren_Print("...no vanilla shader files found!\n");
		return 0;
	}

	Ren_Print("...scanning %i vanilla shader files\n", numShaderFiles);

	if (numShaderFiles >= MAX_SHADER_FILES)
	{
		Ren_Drop("MAX_SHADER_FILES limit is reached!");
	}

	// load and parse shader files
	for (i = 0; i < numShaderFiles; i++)
	{
		Com_sprintf(filename, sizeof(filename), "scripts/%s", shaderFiles[i]);
		COM_BeginParseSession(filename);

		Ren_Developer("...loading '%s'\n", filename);
		summand = ri.FS_ReadFile(filename, (void **)&buffers[i]);

		if (!buffers[i])
		{
			Ren_Drop("Couldn't load %s", filename); // in this case shader file is cought/listed but the file can't be read - drop!
		}

		p = buffers[i];
		while (1)
		{
			token = COM_ParseExt(&p, qtrue);

			if (!*token)
			{
				break;
			}

			// Step over the "table"/"guide" and the name
			if (!Q_stricmp(token, "table") || !Q_stricmp(token, "guide"))
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!*token)
				{
					break;
				}
			}

			oldp = p;

			token = COM_ParseExt2(&p, qtrue);
			if (token[0] != '{' && token[1] != '\0')
			{
				Ren_Warning("WARNING: Bad shader file %s has incorrect syntax near token '%s' line %i\n", filename, token, COM_GetCurrentParseLine());
				ri.FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}

			SkipBracedSection(&oldp);
			p = oldp;
		}

		if (buffers[i])
		{
			sum += summand;
		}
	}

	// build single large buffer
	s_shaderTextR1    = (char *)ri.Hunk_Alloc(sum + numShaderFiles * 2, h_low);
	s_shaderTextR1[0] = '\0';
	textEnd           = s_shaderTextR1;

	// free in reverse order, so the temp files are all dumped
	for (i = numShaderFiles - 1; i >= 0 ; i--)
	{
		if (!buffers[i])
		{
			continue;
		}

		strcat(textEnd, buffers[i]);
		strcat(textEnd, "\n");
		textEnd += strlen(textEnd);
		ri.FS_FreeFile(buffers[i]);
	}

	COM_Compress(s_shaderTextR1);

	// free up memory
	ri.FS_FreeFileList(shaderFiles);

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderTextR1;
	// look for shader names
	while (1)
	{
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0)
		{
			break;
		}

		// skip shader tables
		if (!Q_stricmp(token, "table"))
		{
			// skip table name
			(void) COM_ParseExt2(&p, qtrue);

			SkipBracedSection(&p);
		}
		// support shader templates
		else if (!Q_stricmp(token, "guide"))
		{
			// parse shader name
			token = COM_ParseExt2(&p, qtrue);
			//Ren_Print("...guided '%s'\n", token);

			hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTableSizes[hash]++;
			size++;

			// skip guide name
			token = COM_ParseExt2(&p, qtrue);

			// skip parameters
			token = COM_ParseExt2(&p, qtrue);
			if (Q_stricmp(token, "("))
			{
				Ren_Warning("expected ( found '%s'\n", token);
				break;
			}

			while (1)
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!token[0])
				{
					break;
				}

				if (!Q_stricmp(token, ")"))
				{
					break;
				}
			}

			if (Q_stricmp(token, ")"))
			{
				Ren_Warning("expected ( found '%s'\n", token);
				break;
			}
		}
		else
		{
			hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTableSizes[hash]++;
			size++;
			SkipBracedSection(&p);
		}
	}

	//Ren_Print("Shader hash table size %i\n", size);

	size += MAX_SHADERTEXT_HASH;

	hashMem = (char **)ri.Hunk_Alloc(size * sizeof(char *), h_low);

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++)
	{
		shaderTextHashTableR1[i] = hashMem;
		hashMem                 += shaderTextHashTableSizes[i] + 1;
	}

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderTextR1;

	// look for shader names
	while (1)
	{
		oldp  = p;
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0)
		{
			break;
		}

		// parse shader tables
		if (!Q_stricmp(token, "table"))
		{
			int           depth;
			float         values[FUNCTABLE_SIZE];
			int           numValues;
			shaderTable_t *tb;
			qboolean      alreadyCreated;

			Com_Memset(&values, 0, sizeof(values));
			Com_Memset(&table, 0, sizeof(table));

			token = COM_ParseExt2(&p, qtrue);

			Q_strncpyz(table.name, token, sizeof(table.name));

			// check if already created
			alreadyCreated = qfalse;
			hash           = generateHashValue(table.name, MAX_SHADERTABLE_HASH);
			for (tb = shaderTableHashTable[hash]; tb; tb = tb->next)
			{
				if (Q_stricmp(tb->name, table.name) == 0)
				{
					// match found
					alreadyCreated = qtrue;
					break;
				}
			}

			depth     = 0;
			numValues = 0;
			do
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!Q_stricmp(token, "snap"))
				{
					table.snap = qtrue;
				}
				else if (!Q_stricmp(token, "clamp"))
				{
					table.clamp = qtrue;
				}
				else if (token[0] == '{')
				{
					depth++;
				}
				else if (token[0] == '}')
				{
					depth--;
				}
				else if (token[0] == ',')
				{
					continue;
				}
				else
				{
					if (numValues == FUNCTABLE_SIZE)
					{
						Ren_Warning("WARNING: FUNCTABLE_SIZE hit\n");
						break;
					}
					values[numValues++] = atof(token);
				}
			}
			while (depth && p);

			if (!alreadyCreated)
			{
				Ren_Developer("...generating '%s'\n", table.name);
				GeneratePermanentShaderTable(values, numValues);
			}
		}
		// support shader templates
		else if (!Q_stricmp(token, "guide"))
		{
			// parse shader name
			oldp  = p;
			token = COM_ParseExt2(&p, qtrue);

			//Ren_Print("...guided '%s'\n", token);

			hash                                                          = generateHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTableR1[hash][shaderTextHashTableSizes[hash]++] = oldp;

			// skip guide name
			token = COM_ParseExt2(&p, qtrue);

			// skip parameters
			token = COM_ParseExt2(&p, qtrue);
			if (Q_stricmp(token, "("))
			{
				Ren_Warning("expected ( found '%s'\n", token);
				break;
			}

			while (1)
			{
				token = COM_ParseExt2(&p, qtrue);

				if (!token[0])
				{
					break;
				}

				if (!Q_stricmp(token, ")"))
				{
					break;
				}
			}

			if (Q_stricmp(token, ")"))
			{
				Ren_Warning("expected ( found '%s'\n", token);
				break;
			}
		}
		else
		{
			hash                                                          = generateHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTableR1[hash][shaderTextHashTableSizes[hash]++] = oldp;

			SkipBracedSection(&p);
		}
	}

	return numShaderFiles;
}
