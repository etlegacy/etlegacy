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
 * @file renderer/tr_shader.c
 * @brief Parsing and definition of shaders
 */
#include "tr_local.h"

//#define SH_LOADTIMING

static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shaderStage_t stages[MAX_SHADER_STAGES];
static shader_t      shader;
static texModInfo_t  texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

// these are here because they are only referenced while parsing a shader
static char       implicitMap[MAX_QPATH];
static unsigned   implicitStateBits;
static cullType_t implicitCullType;

#define FILE_HASH_SIZE      4096
static shader_t *hashTable[FILE_HASH_SIZE];
#define generateHashValue(fname) Q_GenerateHashValue(fname, FILE_HASH_SIZE, qfalse, qtrue)

/**
 * @struct shaderStringPointer_s
 * @typedef shaderStringPointer_t
 *
 * @brief Table containing string indexes for each shader found in the scripts,
 * referenced by their checksum values.
 */
typedef struct shaderStringPointer_s
{
	char *pStr;
	struct shaderStringPointer_s *next;
} shaderStringPointer_t;

shaderStringPointer_t shaderChecksumLookup[FILE_HASH_SIZE];

/**
 * @brief R_RemapShader
 * @param[in] shaderName
 * @param[in] newShaderName
 * @param[in] timeOffset
 */
void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset)
{
	char      strippedName[MAX_QPATH];
	int       hash;
	shader_t  *sh, *sh2;
	qhandle_t h;

	sh = R_FindShaderByName(shaderName);
	if (sh == NULL || sh == tr.defaultShader)
	{
		h  = RE_RegisterShaderLightMap(shaderName, 0);
		sh = R_GetShaderByHandle(h);
	}
	if (sh == NULL || sh == tr.defaultShader)
	{
		Ren_Warning("WARNING: R_RemapShader: shader %s not found\n", shaderName);
		return;
	}

	sh2 = R_FindShaderByName(newShaderName);
	if (sh2 == NULL || sh2 == tr.defaultShader)
	{
		h   = RE_RegisterShaderLightMap(newShaderName, 0);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == NULL || sh2 == tr.defaultShader)
	{
		Ren_Warning("WARNING: R_RemapShader: new shader %s not found\n", newShaderName);
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension(shaderName, strippedName, sizeof(strippedName));
	hash = generateHashValue(strippedName);
	for (sh = hashTable[hash]; sh; sh = sh->next)
	{
		if (Q_stricmp(sh->name, strippedName) == 0)
		{
			if (sh != sh2)
			{
				sh->remappedShader = sh2;
			}
			else
			{
				sh->remappedShader = NULL;
			}
		}
	}
	if (timeOffset)
	{
		sh2->timeOffset = atof(timeOffset);
	}
}

/**
 * @brief ParseVector
 * @param[in,out] text
 * @param[in] count
 * @param[out] v
 * @return
 */
static qboolean ParseVector(char **text, int count, float *v)
{
	char *token;
	int  i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, "("))
	{
		Ren_Warning("WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return qfalse;
	}

	for (i = 0 ; i < count ; i++)
	{
		token = COM_ParseExt(text, qfalse);
		if (!token[0])
		{
			Ren_Warning("WARNING: missing vector element in shader '%s'\n", shader.name);
			return qfalse;
		}
		v[i] = (float)atof(token);
	}

	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, ")"))
	{
		Ren_Warning("WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief NameToAFunc
 * @param[in] funcname
 */
static unsigned NameToAFunc(const char *funcname)
{
	if (!Q_stricmp(funcname, "GT0"))
	{
		return GLS_ATEST_GT_0;
	}
	else if (!Q_stricmp(funcname, "LT128"))
	{
		return GLS_ATEST_LT_80;
	}
	else if (!Q_stricmp(funcname, "GE128"))
	{
		return GLS_ATEST_GE_80;
	}

	Ren_Warning("WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name);
	return 0;
}

/**
 * @brief NameToSrcBlendMode
 * @param[in] name
 * @return
 */
static int NameToSrcBlendMode(const char *name)
{
	if (!Q_stricmp(name, "GL_ONE"))
	{
		return GLS_SRCBLEND_ONE;
	}
	else if (!Q_stricmp(name, "GL_ZERO"))
	{
		return GLS_SRCBLEND_ZERO;
	}
	else if (!Q_stricmp(name, "GL_DST_COLOR"))
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_COLOR"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if (!Q_stricmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_DST_ALPHA"))
	{
		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_SRC_ALPHA_SATURATE"))
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	Ren_Warning("WARNING: unknown src blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_SRCBLEND_ONE;
}


/**
 * @brief NameToDstBlendMode
 * @param[in] name
 * @return
 */
static int NameToDstBlendMode(const char *name)
{
	if (!Q_stricmp(name, "GL_ONE"))
	{
		return GLS_DSTBLEND_ONE;
	}
	else if (!Q_stricmp(name, "GL_ZERO"))
	{
		return GLS_DSTBLEND_ZERO;
	}
	else if (!Q_stricmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_DST_ALPHA"))
	{
		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!Q_stricmp(name, "GL_SRC_COLOR"))
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_COLOR"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	Ren_Warning("WARNING: unknown dst blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_DSTBLEND_ONE;
}

/**
 * @brief NameToGenFunc
 * @param[in] funcname
 * @return
 */
static genFunc_t NameToGenFunc(const char *funcname)
{
	if (!Q_stricmp(funcname, "sin"))
	{
		return GF_SIN;
	}
	else if (!Q_stricmp(funcname, "square"))
	{
		return GF_SQUARE;
	}
	else if (!Q_stricmp(funcname, "triangle"))
	{
		return GF_TRIANGLE;
	}
	else if (!Q_stricmp(funcname, "sawtooth"))
	{
		return GF_SAWTOOTH;
	}
	else if (!Q_stricmp(funcname, "inversesawtooth"))
	{
		return GF_INVERSE_SAWTOOTH;
	}
	else if (!Q_stricmp(funcname, "noise"))
	{
		return GF_NOISE;
	}

	Ren_Warning("WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name);
	return GF_SIN;
}

/**
 * @brief ParseWaveForm
 * @param[in,out] text
 * @param[out] wave
 */
static void ParseWaveForm(char **text, waveForm_t *wave)
{
	char *token;

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->func = NameToGenFunc(token);

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->base = atof(token);

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->amplitude = atof(token);

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->phase = atof(token);

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->frequency = atof(token);
}

/**
 * @brief ParseTexMod
 * @param[in,out] _text
 * @param[in,out] stage
 */
static void ParseTexMod(char *_text, shaderStage_t *stage)
{
	const char   *token;
	char         **text = &_text;
	texModInfo_t *tmi;

	if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS)
	{
		Ren_Drop("ParseTexMod ERROR: too many tcMod stages in shader '%s'", shader.name);
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = COM_ParseExt(text, qfalse);

	// swap
	if (!Q_stricmp(token, "swap"))       // swap S/T coords (rotate 90d)
	{
		tmi->type = TMOD_SWAP;
	}
	// turb
	// added 'else' so it wouldn't claim 'swap' was unknown.
	else if (!Q_stricmp(token, "turb"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod turb parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.base = atof(token);
		token          = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.amplitude = atof(token);
		token               = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.phase = atof(token);
		token           = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_TURBULENT;
	}
	// scale
	else if (!Q_stricmp(token, "scale"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scale[0] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scale[1] = atof(token);
		tmi->type     = TMOD_SCALE;
	}
	// scroll
	else if (!Q_stricmp(token, "scroll"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing 1st scale scroll parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scroll[0] = atof(token);
		token          = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing 2nd scale scroll parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scroll[1] = atof(token);
		tmi->type      = TMOD_SCROLL;
	}
	// stretch
	else if (!Q_stricmp(token, "stretch"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.func = NameToGenFunc(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.base = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.amplitude = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.phase = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_STRETCH;
	}
	// transform
	else if (!Q_stricmp(token, "transform"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[0][0] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[0][1] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[1][0] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[1][1] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[1] = atof(token);

		tmi->type = TMOD_TRANSFORM;
	}
	// rotate
	else if (!Q_stricmp(token, "rotate"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->rotateSpeed = atof(token);
		tmi->type        = TMOD_ROTATE;
	}
	// entityTranslate
	else if (!Q_stricmp(token, "entityTranslate"))
	{
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		Ren_Warning("WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name);
	}
}

/**
 * @brief ParseStage
 * @param[in,out] stage
 * @param[in,out] text
 * @return
 */
static qboolean ParseStage(shaderStage_t *stage, char **text)
{
	char     *token;
	int      depthMaskBits     = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;

	while (1)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			Ren_Warning("WARNING: no matching '}' found\n");
			return qfalse;
		}

		if (token[0] == '}')
		{
			break;
		}

		// check special case for map16/map32/mapcomp/mapnocomp (compression enabled)
		if (!Q_stricmp(token, "map16"))          // only use this texture if 16 bit color depth
		{
			if (glConfig.colorBits <= 16)
			{
				token = "map";   // use this map
			}
			else
			{
				(void) COM_ParseExt(text, qfalse);     // ignore the map
				continue;
			}
		}
		else if (!Q_stricmp(token, "map32"))            // only use this texture if 16 bit color depth
		{
			if (glConfig.colorBits > 16)
			{
				token = "map";   // use this map
			}
			else
			{
				(void) COM_ParseExt(text, qfalse);     // ignore the map
				continue;
			}
		}
		else if (!Q_stricmp(token, "mapcomp"))            // only use this texture if compression is enabled
		{
			if (glConfig.textureCompression && r_extCompressedTextures->integer)
			{
				token = "map";   // use this map
			}
			else
			{
				(void) COM_ParseExt(text, qfalse);     // ignore the map
				continue;
			}
		}
		else if (!Q_stricmp(token, "mapnocomp"))            // only use this texture if compression is not available or disabled
		{
			if (!glConfig.textureCompression)
			{
				token = "map";   // use this map
			}
			else
			{
				(void) COM_ParseExt(text, qfalse);     // ignore the map
				continue;
			}
		}
		else if (!Q_stricmp(token, "animmapcomp"))            // only use this texture if compression is enabled
		{
			if (glConfig.textureCompression && r_extCompressedTextures->integer)
			{
				token = "animmap";   // use this map
			}
			else
			{
				while (token[0])
					(void) COM_ParseExt(text, qfalse);     // ignore the map
				continue;
			}
		}
		else if (!Q_stricmp(token, "animmapnocomp"))            // only use this texture if compression is not available or disabled
		{
			if (!glConfig.textureCompression)
			{
				token = "animmap";   // use this map
			}
			else
			{
				while (token[0])
				{
					(void) COM_ParseExt(text, qfalse);     // ignore the map
				}
				continue;
			}
		}

		// map <name>
		if (!Q_stricmp(token, "map"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			// fixes startup error and allows polygon shadows to work again
			if (!Q_stricmp(token, "$whiteimage") || !Q_stricmp(token, "*white"))
			{
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
			else if (!Q_stricmp(token, "$dlight"))
			{
				stage->bundle[0].image[0] = tr.dlightImage;
				continue;
			}
			else if (!Q_stricmp(token, "$lightmap"))
			{
				stage->bundle[0].isLightmap = qtrue;
				if (shader.lightmapIndex < 0)
				{
					stage->bundle[0].image[0] = tr.whiteImage;
				}
				else
				{
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				stage->bundle[0].image[0] = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT, qfalse);
				if (!stage->bundle[0].image[0])
				{
					Ren_Warning("WARNING: R_FindImageFile could not find 'map' image '%s' in shader '%s'\n", token, shader.name);
					return qfalse;
				}
			}
		}
		// clampmap <name>
		else if (!Q_stricmp(token, "clampmap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			stage->bundle[0].image[0] = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_CLAMP_TO_EDGE, qfalse);
			if (!stage->bundle[0].image[0])
			{
				Ren_Warning("WARNING: R_FindImageFile could not find 'clampmap' image '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}
		// lightmap <name>
		else if (!Q_stricmp(token, "lightmap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'lightmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			// fixes startup error and allows polygon shadows to work again
			if (!Q_stricmp(token, "$whiteimage") || !Q_stricmp(token, "*white"))
			{
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
			else if (!Q_stricmp(token, "$dlight"))
			{
				stage->bundle[0].image[0] = tr.dlightImage;
				continue;
			}
			else if (!Q_stricmp(token, "$lightmap"))
			{
				stage->bundle[0].isLightmap = qtrue;
				if (shader.lightmapIndex < 0)
				{
					stage->bundle[0].image[0] = tr.whiteImage;
				}
				else
				{
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				stage->bundle[0].image[0] = R_FindImageFile(token, qfalse, qfalse, GL_CLAMP_TO_EDGE, qtrue);
				if (!stage->bundle[0].image[0])
				{
					Ren_Warning("WARNING: R_FindImageFile could not find 'lighmap' image '%s' in shader '%s'\n", token, shader.name);
					return qfalse;
				}
				stage->bundle[0].isLightmap = qtrue;
			}
		}
		// animMap <frequency> <image1> .... <imageN>
		else if (!Q_stricmp(token, "animMap"))
		{
			int totalImages = 0;

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'animMmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof(token);

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while (1)
			{
				int num;

				token = COM_ParseExt(text, qfalse);
				if (!token[0])
				{
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if (num < MAX_IMAGE_ANIMATIONS)
				{
					stage->bundle[0].image[num] = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT, qfalse);
					if (!stage->bundle[0].image[num])
					{
						Ren_Warning("WARNING: R_FindImageFile could not find 'animMap' image '%s' in shader '%s'\n", token, shader.name);
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
				totalImages++;
			}

			if (totalImages > MAX_IMAGE_ANIMATIONS)
			{
				ri.Printf(PRINT_WARNING, "WARNING: ignoring excess images for 'animMap' (found %d, max is %d) in shader '%s'\n",
				          totalImages, MAX_IMAGE_ANIMATIONS, shader.name);
			}
		}
		else if (!Q_stricmp(token, "videoMap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'videoMmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri.CIN_PlayCinematic(token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[0].videoMapHandle != -1)
			{
				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].image[0]   = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
			else
			{
				ri.Printf(PRINT_WARNING, "WARNING: could not load '%s' for 'videoMap' keyword in shader '%s'\n", token, shader.name);
			}
		}
		// alphafunc <func>
		else if (!Q_stricmp(token, "alphaFunc"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			atestBits = NameToAFunc(token);
		}
		// depthFunc <func>
		else if (!Q_stricmp(token, "depthfunc"))
		{
			token = COM_ParseExt(text, qfalse);

			if (!token[0])
			{
				Ren_Warning("WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "lequal"))
			{
				depthFuncBits = 0;
			}
			else if (!Q_stricmp(token, "equal"))
			{
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else
			{
				Ren_Warning("WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		// detail
		else if (!Q_stricmp(token, "detail"))
		{
			stage->isDetail = qtrue;
		}
		// fog
		else if (!Q_stricmp(token, "fog"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parm for fog in shader '%s'\n", shader.name);
				continue;
			}
			if (!Q_stricmp(token, "on"))
			{
				stage->isFogged = qtrue;
			}
			else
			{
				stage->isFogged = qfalse;
			}
		}
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		else if (!Q_stricmp(token, "blendfunc"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
				continue;
			}
			// check for "simple" blends first
			if (!Q_stricmp(token, "add"))
			{
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			}
			else if (!Q_stricmp(token, "filter"))
			{
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			}
			else if (!Q_stricmp(token, "blend"))
			{
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			}
			else
			{
				// complex double blends
				blendSrcBits = NameToSrcBlendMode(token);

				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
				{
					Ren_Warning("WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
					continue;
				}
				blendDstBits = NameToDstBlendMode(token);
			}

			// clear depth mask for blended surfaces
			if (!depthMaskExplicit)
			{
				depthMaskBits = 0;
			}
		}
		// rgbGen
		else if (!Q_stricmp(token, "rgbGen"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "wave"))
			{
				ParseWaveForm(text, &stage->rgbWave);
				stage->rgbGen = CGEN_WAVEFORM;
			}
			else if (!Q_stricmp(token, "const"))
			{
				vec3_t color = { 0, 0, 0 };

				ParseVector(text, 3, color);
				stage->constantColor[0] = (byte)(255 * color[0]);
				stage->constantColor[1] = (byte)(255 * color[1]);
				stage->constantColor[2] = (byte)(255 * color[2]);

				stage->rgbGen = CGEN_CONST;
			}
			else if (!Q_stricmp(token, "identity"))
			{
				stage->rgbGen = CGEN_IDENTITY;
			}
			else if (!Q_stricmp(token, "identityLighting"))
			{
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if (!Q_stricmp(token, "entity"))
			{
				stage->rgbGen = CGEN_ENTITY;
			}
			else if (!Q_stricmp(token, "oneMinusEntity"))
			{
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if (!Q_stricmp(token, "vertex"))
			{
				stage->rgbGen = CGEN_VERTEX;
				if (stage->alphaGen == 0)
				{
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if (!Q_stricmp(token, "exactVertex"))
			{
				stage->rgbGen = CGEN_EXACT_VERTEX;
			}
			else if (!Q_stricmp(token, "lightingDiffuse"))
			{
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			}
			else if (!Q_stricmp(token, "oneMinusVertex"))
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				Ren_Warning("WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		// alphaGen
		else if (!Q_stricmp(token, "alphaGen"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "wave"))
			{
				ParseWaveForm(text, &stage->alphaWave);
				stage->alphaGen = AGEN_WAVEFORM;
			}
			else if (!Q_stricmp(token, "const"))
			{
				token                   = COM_ParseExt(text, qfalse);
				stage->constantColor[3] = (byte)(255 * atof(token));
				stage->alphaGen         = AGEN_CONST;
			}
			else if (!Q_stricmp(token, "identity"))
			{
				stage->alphaGen = AGEN_IDENTITY;
			}
			else if (!Q_stricmp(token, "entity"))
			{
				stage->alphaGen = AGEN_ENTITY;
			}
			else if (!Q_stricmp(token, "oneMinusEntity"))
			{
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if (!Q_stricmp(token, "normalzfade"))
			{
				stage->alphaGen = AGEN_NORMALZFADE;
				token           = COM_ParseExt(text, qfalse);
				if (token[0])
				{
					stage->constantColor[3] = (byte)(255 * atof(token));
				}
				else
				{
					stage->constantColor[3] = 255;
				}

				token = COM_ParseExt(text, qfalse);
				if (token[0])
				{
					stage->zFadeBounds[0] = atof(token);      // lower range
					token                 = COM_ParseExt(text, qfalse);
					stage->zFadeBounds[1] = atof(token);      // upper range
				}
				else
				{
					stage->zFadeBounds[0] = -1.0;   // lower range
					stage->zFadeBounds[1] = 1.0;    // upper range
				}

			}
			else if (!Q_stricmp(token, "vertex"))
			{
				stage->alphaGen = AGEN_VERTEX;
			}
			else if (!Q_stricmp(token, "lightingSpecular"))
			{
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			}
			else if (!Q_stricmp(token, "oneMinusVertex"))
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else if (!Q_stricmp(token, "portal"))
			{
				stage->alphaGen = AGEN_PORTAL;
				token           = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
				{
					shader.portalRange = 256;
					Ren_Warning("WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name);
				}
				else
				{
					shader.portalRange = atof(token);
				}
			}
			else
			{
				Ren_Warning("WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		// tcGen <function>
		else if (!Q_stricmp(token, "texgen") || !Q_stricmp(token, "tcGen"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing texgen parm in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "environment"))
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			}
			else if (!Q_stricmp(token, "firerisenv"))
			{
				stage->bundle[0].tcGen = TCGEN_FIRERISEENV_MAPPED;
			}
			else if (!Q_stricmp(token, "lightmap"))
			{
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			else if (!Q_stricmp(token, "texture") || !Q_stricmp(token, "base"))
			{
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
			else if (!Q_stricmp(token, "vector"))
			{
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[0]);
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[1]);

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else
			{
				Ren_Warning("WARNING: unknown texgen parm in shader '%s'\n", shader.name);
			}
		}
		// tcMod <type> <...>
		else if (!Q_stricmp(token, "tcMod"))
		{
			char buffer[1024] = "";

			while (1)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
				{
					break;
				}
				Q_strcat(buffer, sizeof(buffer), token);
				Q_strcat(buffer, sizeof(buffer), " ");
			}

			ParseTexMod(buffer, stage);

			continue;
		}
		// depthmask
		else if (!Q_stricmp(token, "depthwrite"))
		{
			depthMaskBits     = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		}
		else
		{
			Ren_Warning("WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name);
			return qfalse;
		}
	}

	// if cgen isn't explicitly specified, use either identity or identitylighting
	if (stage->rgbGen == CGEN_BAD)
	{
		if (blendSrcBits == 0 ||
		    blendSrcBits == GLS_SRCBLEND_ONE ||
		    blendSrcBits == GLS_SRCBLEND_SRC_ALPHA)
		{
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		else
		{
			stage->rgbGen = CGEN_IDENTITY;
		}
	}

	// if shader stage references a lightmap, but no lightmap is present
	// (vertex-approximated surfaces), then set cgen to vertex
	if (stage->bundle[0].isLightmap && shader.lightmapIndex < 0 &&
	    stage->bundle[0].image[0] == tr.whiteImage)
	{
		stage->rgbGen = CGEN_EXACT_VERTEX;
	}

	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	if ((blendSrcBits == GLS_SRCBLEND_ONE) &&
	    (blendDstBits == GLS_DSTBLEND_ZERO))
	{
		blendDstBits  = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if (stage->alphaGen == AGEN_IDENTITY)
	{
		if (stage->rgbGen == CGEN_IDENTITY
		    || stage->rgbGen == CGEN_LIGHTING_DIFFUSE)
		{
			stage->alphaGen = AGEN_SKIP;
		}
	}

	// compute state bits
	stage->stateBits = depthMaskBits |
	                   blendSrcBits | blendDstBits |
	                   atestBits |
	                   depthFuncBits;

	return qtrue;
}

/**
 * @brief ParseDeform
 * @param[in,out] text
 *
 * @note deformVertexes wave \<spread\> \<waveform\> \<base\> \<amplitude\> \<phase\> \<frequency\>
 * deformVertexes normal \<frequency\> \<amplitude\>
 * deformVertexes move \<vector\> \<waveform\> \<base\> \<amplitude\> \<phase\> \<frequency\>
 * deformVertexes bulge \<bulgeWidth\> \<bulgeHeight\> \<bulgeSpeed\>
 * deformVertexes projectionShadow
 * deformVertexes autoSprite
 * deformVertexes autoSprite2
 * deformVertexes text[0-7]
 */
static void ParseDeform(char **text)
{
	char          *token;
	deformStage_t *ds;

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing deform parm in shader '%s'\n", shader.name);
		return;
	}

	if (shader.numDeforms == MAX_SHADER_DEFORMS)
	{
		Ren_Warning("WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name);
		return;
	}

	ds = &shader.deforms[shader.numDeforms];
	shader.numDeforms++;

	if (!Q_stricmp(token, "projectionShadow"))
	{
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if (!Q_stricmp(token, "autosprite"))
	{
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if (!Q_stricmp(token, "autosprite2"))
	{
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if (!Q_stricmpn(token, "text", 4))
	{
		int n;

		n = token[4] - '0';
		if (n < 0 || n > 7)
		{
			n = 0;
		}
		ds->deformation = DEFORM_TEXT0 + n;
		return;
	}

	if (!Q_stricmp(token, "bulge"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeWidth = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeHeight = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeSpeed = atof(token);

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if (!Q_stricmp(token, "wave"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}

		if (atof(token) != 0.)
		{
			ds->deformationSpread = 1.0 / atof(token);
		}
		else
		{
			ds->deformationSpread = 100.0;
			Ren_Warning("WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if (!Q_stricmp(token, "normal"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.amplitude = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Ren_Warning("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.frequency = atof(token);

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if (!Q_stricmp(token, "move"))
	{
		int i;

		for (i = 0 ; i < 3 ; i++)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
				return;
			}
			ds->moveVector[i] = atof(token);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_MOVE;
		return;
	}

	Ren_Warning("WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name);
}

/**
 * @brief ParseSkyParms
 * @param[in,out] text
 *
 * @note skyParms \<outerbox\> \<cloudheight\> \<innerbox\>
 */
static void ParseSkyParms(char **text)
{
	char        *token;
	static char *suf[6] = { "rt", "bk", "lf", "ft", "up", "dn" };
	char        pathname[MAX_QPATH];
	int         i;

	// outerbox
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	if (strcmp(token, "-"))
	{
		for (i = 0 ; i < 6 ; i++)
		{
			Com_sprintf(pathname, sizeof(pathname), "%s_%s.tga", token, suf[i]);

			shader.sky.outerbox[i] = R_FindImageFile(( char * ) pathname, qtrue, qtrue, GL_CLAMP_TO_EDGE, qfalse);
			if (!shader.sky.outerbox[i])
			{
				Ren_Warning("WARNING: could not find image '%s' for outer skybox in shader '%s'\n", pathname, shader.name);
				shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	// cloudheight
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	shader.sky.cloudHeight = atof(token);
	if (shader.sky.cloudHeight == 0.f)
	{
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords(shader.sky.cloudHeight);

	// innerbox
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	if (strcmp(token, "-"))
	{
		for (i = 0 ; i < 6 ; i++)
		{
			Com_sprintf(pathname, sizeof(pathname), "%s_%s.tga", token, suf[i]);

			shader.sky.innerbox[i] = R_FindImageFile(( char * ) pathname, qtrue, qtrue, GL_REPEAT, qfalse);
			if (!shader.sky.innerbox[i])
			{
				Ren_Warning("WARNING: could not find image '%s' for inner skybox in shader '%s'\n", pathname, shader.name);
				shader.sky.innerbox[i] = tr.defaultImage;
			}
		}
	}

	shader.isSky = qtrue;
}

/**
 * @brief ParseSort
 * @param[in,out] text
 */
void ParseSort(char **text)
{
	char *token;

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Ren_Warning("WARNING: missing sort parameter in shader '%s'\n", shader.name);
		return;
	}

	if (!Q_stricmp(token, "portal"))
	{
		shader.sort = SS_PORTAL;
	}
	else if (!Q_stricmp(token, "sky"))
	{
		shader.sort = SS_ENVIRONMENT;
	}
	else if (!Q_stricmp(token, "opaque"))
	{
		shader.sort = SS_OPAQUE;
	}
	else if (!Q_stricmp(token, "decal"))
	{
		shader.sort = SS_DECAL;
	}
	else if (!Q_stricmp(token, "seeThrough"))
	{
		shader.sort = SS_SEE_THROUGH;
	}
	else if (!Q_stricmp(token, "banner"))
	{
		shader.sort = SS_BANNER;
	}
	else if (!Q_stricmp(token, "additive"))
	{
		shader.sort = SS_BLEND1;
	}
	else if (!Q_stricmp(token, "nearest"))
	{
		shader.sort = SS_NEAREST;
	}
	else if (!Q_stricmp(token, "underwater"))
	{
		shader.sort = SS_UNDERWATER;
	}
	else
	{
		shader.sort = atof(token);
	}
}

/**
 * @struct infoParm_s
 * @brief
 *
 * @note This table is also present in q3map
 *
 * @note clearSolid is never used
 */
typedef struct
{
	char *name;
	int clearSolid, surfaceFlags, contents; // clearSolid is never used
} infoParm_t;

infoParm_t infoParms[] =
{
	// server relevant contents

	{ "clipmissile",       1, 0,                 CONTENTS_MISSILECLIP      }, // impact only specific weapons (rl, gl)

	{ "water",             1, 0,                 CONTENTS_WATER            },
	{ "slag",              1, 0,                 CONTENTS_SLIME            }, // uses the CONTENTS_SLIME flag, but the shader reference is changed to 'slag'
	// to idendify that this doesn't work the same as 'slime' did.
	// (slime hurts instantly, slag doesn't)
	//  {"slime",       1,  0,  CONTENTS_SLIME },       // mildly damaging
	{ "lava",              1, 0,                 CONTENTS_LAVA             }, // very damaging
	{ "playerclip",        1, 0,                 CONTENTS_PLAYERCLIP       },
	{ "monsterclip",       1, 0,                 CONTENTS_MONSTERCLIP      },
	{ "nodrop",            1, 0,                 CONTENTS_NODROP           }, // don't drop items or leave bodies (death fog, lava, etc)
	{ "nonsolid",          1, SURF_NONSOLID,     0                         }, // clears the solid flag

	// utility relevant attributes
	{ "origin",            1, 0,                 CONTENTS_ORIGIN           }, // center of rotating brushes
	{ "trans",             0, 0,                 CONTENTS_TRANSLUCENT      }, // don't eat contained surfaces
	{ "detail",            0, 0,                 CONTENTS_DETAIL           }, // don't include in structural bsp
	{ "structural",        0, 0,                 CONTENTS_STRUCTURAL       }, // force into structural bsp even if trnas
	{ "areaportal",        1, 0,                 CONTENTS_AREAPORTAL       }, // divides areas
	{ "clusterportal",     1, 0,                 CONTENTS_CLUSTERPORTAL    }, // for bots
	{ "donotenter",        1, 0,                 CONTENTS_DONOTENTER       }, // for bots

	// Rafael - nopass
	{ "donotenterlarge",   1, 0,                 CONTENTS_DONOTENTER_LARGE }, // for larger bots

	{ "fog",               1, 0,                 CONTENTS_FOG              }, // carves surfaces entering
	{ "sky",               0, SURF_SKY,          0                         }, // emit light from an environment map
	{ "lightfilter",       0, SURF_LIGHTFILTER,  0                         }, // filter light going through it
	{ "alphashadow",       0, SURF_ALPHASHADOW,  0                         }, // test light on a per-pixel basis
	{ "hint",              0, SURF_HINT,         0                         }, // use as a primary splitter

	// server attributes
	{ "slick",             0, SURF_SLICK,        0                         },
	{ "noimpact",          0, SURF_NOIMPACT,     0                         }, // don't make impact explosions or marks
	{ "nomarks",           0, SURF_NOMARKS,      0                         }, // don't make impact marks, but still explode
	{ "ladder",            0, SURF_LADDER,       0                         },
	{ "nodamage",          0, SURF_NODAMAGE,     0                         },

	{ "monsterslick",      0, SURF_MONSTERSLICK, 0                         }, // surf only slick for monsters

	{ "glass",             0, SURF_GLASS,        0                         },
	{ "splash",            0, SURF_SPLASH,       0                         },

	// steps
	{ "metal",             0, SURF_METAL,        0                         },
	{ "metalsteps",        0, SURF_METAL,        0                         }, // retain bw compatibility with Q3A metal shaders...
	{ "nosteps",           0, SURF_NOSTEPS,      0                         },
	{ "woodsteps",         0, SURF_WOOD,         0                         },
	{ "grasssteps",        0, SURF_GRASS,        0                         },
	{ "gravelsteps",       0, SURF_GRAVEL,       0                         },
	{ "carpetsteps",       0, SURF_CARPET,       0                         },
	{ "snowsteps",         0, SURF_SNOW,         0                         },
	{ "roofsteps",         0, SURF_ROOF,         0                         }, // tile roof

	{ "rubble",            0, SURF_RUBBLE,       0                         },

	// drawsurf attributes
	{ "nodraw",            0, SURF_NODRAW,       0                         }, // don't generate a drawsurface (or a lightmap)
	{ "pointlight",        0, SURF_POINTLIGHT,   0                         }, // sample lighting at vertexes
	{ "nolightmap",        0, SURF_NOLIGHTMAP,   0                         }, // don't generate a lightmap
	{ "nodlight",          0, SURF_NODLIGHT,     0                         }, // don't ever add dynamic lights

	// these surface parms are unused in ETL but kept for mods
	{ "monsterslicknorth", 0, SURF_MONSLICK_N,   0                         },
	{ "monsterslickeast",  0, SURF_MONSLICK_E,   0                         },
	{ "monsterslicksouth", 0, SURF_MONSLICK_S,   0                         },
	{ "monsterslickwest",  0, SURF_MONSLICK_W,   0                         }
};

/**
 * @brief ParseSurfaceParm
 * @param[in,out] text
 *
 * @note surfaceparm \<name\>
 */
static void ParseSurfaceParm(char **text)
{
	char     *token;
	int      numInfoParms = ARRAY_LEN(infoParms); // sizeof(infoParms) / sizeof(infoParms[0]);
	int      i;
	qboolean unknownParm = qtrue;

	token = COM_ParseExt(text, qfalse);
	for (i = 0 ; i < numInfoParms ; i++)
	{
		if (!Q_stricmp(token, infoParms[i].name))
		{
			Ren_Developer("ParseSurfaceParm - adding '%s' surface parm properties to shader '%s'\n", token, shader.name);
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
#if 0
			if (infoParms[i].clearSolid)
			{
				si->contents &= ~CONTENTS_SOLID;
			}
#endif
			unknownParm = qfalse;
			break;
		}
	}

	if (unknownParm == qtrue)
	{
		if (Q_stricmp(token, "landmine"))  // ET exception - don't print warning for landmine
		{
			Ren_Developer("WARNING: ParseSurfaceParm - unknown or custom surface parm '%s' in shader '%s' - inspect your shader definitions if this parm is unknown\n", token, shader.name);
		}
	}
}

/**
 * @brief The current text pointer is at the explicit text definition of the
 * shader. Parse it into the global shader variable.
 * Later functions will optimize it.
 *
 * @param[in,out] text
 * @return
 */
static qboolean ParseShader(char **text)
{
	char *token;
	int  s = 0;

	tr.allowCompress = qtrue;   // allow compression by default

	token = COM_ParseExt(text, qtrue);
	if (token[0] != '{')
	{
		Ren_Warning("WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name);
		return qfalse;
	}

	while (1)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			Ren_Warning("WARNING: no concluding '}' in shader %s\n", shader.name);
			return qfalse;
		}

		// end of shader definition
		if (token[0] == '}')
		{
			tr.allowCompress = qtrue;   // allow compression by default
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
		// sun parms
		else if (!Q_stricmp(token, "q3map_sun") || !Q_stricmp(token, "q3map_sunExt"))
		{
			float a, b;

			token          = COM_ParseExt(text, qfalse);
			tr.sunLight[0] = atof(token);
			token          = COM_ParseExt(text, qfalse);
			tr.sunLight[1] = atof(token);
			token          = COM_ParseExt(text, qfalse);
			tr.sunLight[2] = atof(token);

			VectorNormalize(tr.sunLight);

			token = COM_ParseExt(text, qfalse);
			a     = atof(token);
			VectorScale(tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt(text, qfalse);
			a     = atof(token);
			a     = a / 180 * M_PI;

			token = COM_ParseExt(text, qfalse);
			b     = atof(token);
			b     = b / 180 * M_PI;

			tr.sunDirection[0] = cos(a) * cos(b);
			tr.sunDirection[1] = sin(a) * cos(b);
			tr.sunDirection[2] = sin(b);

			SkipRestOfLine(text); // skip q3map_sunExt additional parms
		}
		else if (!Q_stricmp(token, "deformVertexes"))
		{
			ParseDeform(text);
			continue;
		}
		else if (!Q_stricmp(token, "tesssize"))
		{
			SkipRestOfLine(text);
			continue;
		}
		else if (!Q_stricmp(token, "clampTime"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0])
			{
				shader.clampTime = atof(token);
			}
		}
		// skip stuff that only the q3map needs
		else if (!Q_stricmpn(token, "q3map", 5))
		{
			SkipRestOfLine(text);
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if (!Q_stricmp(token, "surfaceParm"))
		{
			ParseSurfaceParm(text);
			continue;
		}
		// no mip maps
		else if ((!Q_stricmp(token, "nomipmaps")) || (!Q_stricmp(token, "nomipmap")))
		{
			shader.noMipMaps = qtrue;
			shader.noPicMip  = qtrue;
			continue;
		}
		// no picmip adjustment
		else if (!Q_stricmp(token, "nopicmip"))
		{
			shader.noPicMip = qtrue;
			continue;
		}
		// polygonOffset
		else if (!Q_stricmp(token, "polygonOffset"))
		{
			shader.polygonOffset = qtrue;
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

			shader.fogParms.colorInt = ColorBytes4(shader.fogParms.color[0] * tr.identityLight,
			                                       shader.fogParms.color[1] * tr.identityLight,
			                                       shader.fogParms.color[2] * tr.identityLight, 1.0);

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: 'fogParms' incomplete - missing opacity value in shader '%s' set to 1\n", shader.name); // tcScale is 1.0f
				shader.fogParms.depthForOpaque = 1;
			}
			else
			{
				shader.fogParms.depthForOpaque = atof(token);
				shader.fogParms.depthForOpaque = shader.fogParms.depthForOpaque < 1 ? 1 : shader.fogParms.depthForOpaque;
			}
			shader.fogParms.tcScale = 1.0f / shader.fogParms.depthForOpaque;
			
			// skip any old gradient directions
			SkipRestOfLine(text);
			continue;
		}
		// portal
		else if (!Q_stricmp(token, "portal"))
		{
			shader.sort = SS_PORTAL;
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

			R_SetFog(FOG_SKY, 0, 5, fogColor[0], fogColor[1], fogColor[2], atof(token));
			continue;
		}
		else if (!Q_stricmp(token, "sunshader"))
		{
			size_t tokenLen;

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Ren_Warning("WARNING: missing shader name for 'sunshader'\n");
				continue;
			}
			tokenLen         = strlen(token) + 1;
			tr.sunShaderName = ri.Hunk_Alloc(sizeof(char) * tokenLen, h_low);
			Q_strncpyz(tr.sunShaderName, token, tokenLen);
		}
		else if (!Q_stricmp(token, "lightgridmulamb"))       // ambient multiplier for lightgrid
		{
			token = COM_ParseExt(text, qfalse);
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
		else if (!Q_stricmp(token, "lightgridmuldir"))                // directional multiplier for lightgrid
		{
			token = COM_ParseExt(text, qfalse);
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
			// to change at some point, but I'm not sure how to track fog parameters
			// on a "per-water volume" basis yet.

			if (fogvar == 0.f)           // '0' specifies "use the map values for everything except the fog color
			{   // TODO
			}
			else if (fogvar > 1)            // distance "linear" fog
			{
				R_SetFog(FOG_WATER, 0, fogvar, watercolor[0], watercolor[1], watercolor[2], 1.1);
			}
			else                          // density "exp" fog
			{
				R_SetFog(FOG_WATER, 0, 5, watercolor[0], watercolor[1], watercolor[2], fogvar);
			}

			continue;
		}
		// fogvars
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
			if (fogDensity > 1)      // linear
			{
				fogFar = fogDensity;
			}
			else
			{
				fogFar = 5;
			}

			R_SetFog(FOG_MAP, 0, fogFar, fogColor[0], fogColor[1], fogColor[2], fogDensity);
			R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 50, 0, 0, 0, 0);

			continue;
		}
		// allow disable fog for some shaders
		else if (!Q_stricmp(token, "nofog"))
		{
			shader.noFog = qtrue;
			continue;
		}
		// allow each shader to permit compression if available
		else if (!Q_stricmp(token, "allowcompress"))
		{
			tr.allowCompress = qtrue;
			continue;
		}
		else if (!Q_stricmp(token, "nocompress"))
		{
			tr.allowCompress = -1;
			continue;
		}
		// light <value> determines flaring in q3map, not needed here
		else if (!Q_stricmp(token, "light"))
		{
			(void) COM_ParseExt(text, qfalse);
			continue;
		}
		// cull <face>
		else if (!Q_stricmp(token, "cull"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Ren_Warning("WARNING: missing cull parms in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "none") || !Q_stricmp(token, "twosided") || !Q_stricmp(token, "disable"))
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
		// sort
		else if (!Q_stricmp(token, "sort"))
		{
			ParseSort(text);
			continue;
		}
		// implicit default mapping to eliminate redundant/incorrect explicit shader stages
		else if (!Q_stricmpn(token, "implicit", 8))
		{
			// set implicit mapping state
			if (!Q_stricmp(token, "implicitBlend"))
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				implicitCullType  = CT_TWO_SIDED;
			}
			else if (!Q_stricmp(token, "implicitMask"))
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_80;
				implicitCullType  = CT_TWO_SIDED;
			}
			else      // "implicitMap"
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
		// unknown directive
		else
		{
			Ren_Warning("WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name);
			return qfalse;
		}
	}

	// ignore shaders that don't have any stages, unless it is a sky or fog
	// or have implicit mapping
	if (s == 0 && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG) && implicitMap[0] == '\0')
	{
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;

	return qtrue;
}

/*
========================================================================================
SHADER OPTIMIZATION AND FOGGING
========================================================================================
*/

/**
 * @brief See if we can use on of the simple fastpath stage functions,
 * otherwise set to the generic stage function
 */
static void ComputeStageIteratorFunc(void)
{
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	// see if this should go into the sky path
	if (shader.isSky)
	{
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		return;
	}

	if (r_ignoreFastPath->integer)
	{
		return;
	}

	// stages with tcMods can't be fast-pathed
	if (stages[0].bundle[0].numTexMods != 0 ||
	    stages[0].bundle[1].numTexMods != 0)
	{
		return;
	}

	// see if this can go into the vertex lit fast path
	if (shader.numUnfoggedPasses == 1)
	{
		if (stages[0].rgbGen == CGEN_LIGHTING_DIFFUSE)
		{
			if (stages[0].alphaGen == AGEN_IDENTITY)
			{
				if (stages[0].bundle[0].tcGen == TCGEN_TEXTURE)
				{
					if (!shader.polygonOffset)
					{
						if (!shader.multitextureEnv)
						{
							if (!shader.numDeforms)
							{
								shader.optimalStageIteratorFunc = RB_StageIteratorVertexLitTexture;
								return;
							}
						}
					}
				}
			}
		}
	}

	// see if this can go into an optimized LM, multitextured path
	if (shader.numUnfoggedPasses == 1)
	{
		if ((stages[0].rgbGen == CGEN_IDENTITY) && (stages[0].alphaGen == AGEN_IDENTITY))
		{
			if (stages[0].bundle[0].tcGen == TCGEN_TEXTURE &&
			    stages[0].bundle[1].tcGen == TCGEN_LIGHTMAP)
			{
				if (!shader.polygonOffset)
				{
					if (!shader.numDeforms)
					{
						if (shader.multitextureEnv)
						{
							shader.optimalStageIteratorFunc = RB_StageIteratorLightmappedMultitexture;
							return;
						}
					}
				}
			}
		}
	}
}

typedef struct
{
	int blendA;
	int blendB;

	int multitextureEnv;
	int multitextureBlend;
} collapse_t;

static collapse_t collapse[] =
{
	{ 0,                                          GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, 0 },

	{ 0,                                          GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, 0 },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ 0,                                          GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, 0 },

	{ GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,        GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE },
#if 0
	{ 0,                                          GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_SRCBLEND_SRC_ALPHA,
	  GL_DECAL, 0 },
#endif
	{ -1,                                         -1,                                                       -1,-1 }
};

/**
 * @brief Attempt to combine two stages into a single multitexture stage
 * @return
 *
 * @todo FIXME: I think modulated add + modulated add collapses incorrectly
 */
static qboolean CollapseMultitexture(void)
{
	int             abits, bbits;
	int             i;
	textureBundle_t tmpBundle;

	if (!glActiveTextureARB)
	{
		return qfalse;
	}

	// make sure both stages are active
	if (!stages[0].active || !stages[1].active)
	{
		return qfalse;
	}

	abits = stages[0].stateBits;
	bbits = stages[1].stateBits;

	// make sure that both stages have identical state other than blend modes
	if ((abits & ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE)) !=
	    (bbits & ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE)))
	{
		return qfalse;
	}

	abits &= (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
	bbits &= (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);

	// search for a valid multitexture blend function
	for (i = 0; collapse[i].blendA != -1 ; i++)
	{
		if (abits == collapse[i].blendA
		    && bbits == collapse[i].blendB)
		{
			break;
		}
	}

	// nothing found
	if (collapse[i].blendA == -1)
	{
		return qfalse;
	}

	// GL_ADD is a separate extension
	if (collapse[i].multitextureEnv == GL_ADD && !glConfig.textureEnvAddAvailable)
	{
		return qfalse;
	}

	// make sure waveforms have identical parameters
	if ((stages[0].rgbGen != stages[1].rgbGen) ||
	    (stages[0].alphaGen != stages[1].alphaGen))
	{
		return qfalse;
	}

	// an add collapse can only have identity colors
	if (collapse[i].multitextureEnv == GL_ADD && stages[0].rgbGen != CGEN_IDENTITY)
	{
		return qfalse;
	}

	if (stages[0].rgbGen == CGEN_WAVEFORM)
	{
		if (memcmp(&stages[0].rgbWave,
		           &stages[1].rgbWave,
		           sizeof(stages[0].rgbWave)))
		{
			return qfalse;
		}
	}

	if (stages[0].alphaGen == AGEN_WAVEFORM)
	{
		if (memcmp(&stages[0].alphaWave,
		           &stages[1].alphaWave,
		           sizeof(stages[0].alphaWave)))
		{
			return qfalse;
		}
	}

	// make sure that lightmaps are in bundle 1 for 3dfx
	if (stages[0].bundle[0].isLightmap)
	{
		tmpBundle           = stages[0].bundle[0];
		stages[0].bundle[0] = stages[1].bundle[0];
		stages[0].bundle[1] = tmpBundle;
	}
	else
	{
		stages[0].bundle[1] = stages[1].bundle[0];
	}

	// set the new blend state bits
	shader.multitextureEnv = collapse[i].multitextureEnv;
	stages[0].stateBits   &= ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
	stages[0].stateBits   |= collapse[i].multitextureBlend;

	// move down subsequent shaders
	memmove(&stages[1], &stages[2], sizeof(stages[0]) * (MAX_SHADER_STAGES - 2));
	Com_Memset(&stages[MAX_SHADER_STAGES - 1], 0, sizeof(stages[0]));

	return qtrue;
}

/**
 * @brief FixRenderCommandList
 *
 * @todo This is a nasty issue. Shaders can be registered after drawsurfaces are generated
 * but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
 * to be rendered with bad shaders. To fix this, need to go through all render commands and fix
 * sortedIndex.
 *
 * @param newShader
 */
static void FixRenderCommandList(int newShader)
{
	renderCommandList_t *cmdList = &backEndData->commands;

	if (cmdList)
	{
		const void *curCmd = cmdList->cmds;

		while (1)
		{
			curCmd = PADP(curCmd, sizeof(void *));

			switch (*(const int *)curCmd)
			{
			case RC_SET_COLOR:
			{
				const setColorCommand_t *sc_cmd = (const setColorCommand_t *)curCmd;

				curCmd = (const void *)(sc_cmd + 1);
				break;
			}
			case RC_STRETCH_PIC:
			case RC_ROTATED_PIC:
			case RC_STRETCH_PIC_GRADIENT:
			{
				const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)curCmd;

				curCmd = (const void *)(sp_cmd + 1);
				break;
			}
			case RC_2DPOLYS:
			{
				const poly2dCommand_t *sp_cmd = (const poly2dCommand_t *)curCmd;
				curCmd = (const void *)(sp_cmd + 1);
				break;
			}
			case RC_DRAW_SURFS:
			{
				drawSurf_t               *drawSurf;
				shader_t                 *shader;
				int                      i, fogNum, frontFace, entityNum, dlightMap, sortedIndex;
				const drawSurfsCommand_t *ds_cmd = (const drawSurfsCommand_t *)curCmd;

				for (i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++)
				{
					R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &frontFace, &dlightMap);
					sortedIndex = ((drawSurf->sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1));
					if (sortedIndex >= newShader)
					{
						sortedIndex++;
						drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT)
						                 | (entityNum << QSORT_ENTITYNUM_SHIFT) | (fogNum << QSORT_FOGNUM_SHIFT) | (frontFace << QSORT_FRONTFACE_SHIFT) | dlightMap;

					}
				}
				curCmd = (const void *)(ds_cmd + 1);
				break;
			}
			case RC_DRAW_BUFFER:
			{
				const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)curCmd;

				curCmd = (const void *)(db_cmd + 1);
				break;
			}
			case RC_SWAP_BUFFERS:
			{
				const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)curCmd;

				curCmd = (const void *)(sb_cmd + 1);
				break;
			}
			case RC_END_OF_LIST:
			default:
				return;
			}
		}
	}
}

/**
 * @brief Positions the most recently created shader in the tr.sortedShaders[]
 * array so that the shader->sort key is sorted reletive to the other
 * shaders.
 *
 * @note Sets shader->sortedIndex
 */
static void SortNewShader(void)
{
	int      i;
	shader_t *newShader = tr.shaders[tr.numShaders - 1];
	float    sort       = newShader->sort;

	for (i = tr.numShaders - 2 ; i >= 0 ; i--)
	{
		if (tr.sortedShaders[i]->sort <= sort)
		{
			break;
		}
		tr.sortedShaders[i + 1] = tr.sortedShaders[i];
		tr.sortedShaders[i + 1]->sortedIndex++;
	}

	// fix rendercommandlist
	FixRenderCommandList(i + 1);

	newShader->sortedIndex  = i + 1;
	tr.sortedShaders[i + 1] = newShader;
}

/**
 * @brief GeneratePermanentShader
 * @return
 */
static shader_t *GeneratePermanentShader(void)
{
	shader_t *newShader;
	int      i, b;
	int      size, hash;

	if (tr.numShaders == MAX_SHADERS)
	{
		Ren_Warning("WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	// caching system
	newShader = R_CacheShaderAlloc(shader.lightmapIndex < 0 ? va("%s lm: %i", shader.name, shader.lightmapIndex) : NULL, sizeof(shader_t));

	*newShader = shader;

	if (shader.sort <= SS_SEE_THROUGH)      // was SS_DECAL, this allows grates to be fogged
	{
		newShader->fogPass = FP_EQUAL;
	}
	else if (shader.contentFlags & CONTENTS_FOG)
	{
		newShader->fogPass = FP_LE;
	}

	tr.shaders[tr.numShaders] = newShader;
	newShader->index          = tr.numShaders;

	tr.sortedShaders[tr.numShaders] = newShader;
	newShader->sortedIndex          = tr.numShaders;

	tr.numShaders++;

	for (i = 0 ; i < newShader->numUnfoggedPasses ; i++)
	{
		if (!stages[i].active)
		{
			newShader->stages[i] = NULL;    // make sure it's null
			break;
		}
		// caching system
		newShader->stages[i] = R_CacheShaderAlloc(NULL, sizeof(stages[i]));

		*newShader->stages[i] = stages[i];

		for (b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++)
		{
			if (!newShader->stages[i]->bundle[b].numTexMods)
			{
				// make sure unalloc'd texMods aren't pointing to some random point in memory
				newShader->stages[i]->bundle[b].texMods = NULL;
				continue;
			}
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof(texModInfo_t);
			// caching system
			newShader->stages[i]->bundle[b].texMods = R_CacheShaderAlloc(NULL, size);

			Com_Memcpy(newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size);
		}
	}

	SortNewShader();

	hash            = generateHashValue(newShader->name);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

#if 0 // To shut up compiler warnings until we use this function.
/**
 * @brief If vertex lighting is enabled, only render a single pass, trying to guess
 * which is the correct one to best aproximate what it is supposed to look like.
 * @note Unused in ET:L, but can be enabled with patches from the ioquake3 project.
 */
static void VertexLightingCollapse(void)
{
	int           stage;
	shaderStage_t *bestStage;
	int           bestImageRank;
	int           rank;

	// if we aren't opaque, just use the first pass
	if (shader.sort == SS_OPAQUE)
	{
		// pick the best texture for the single pass
		bestStage     = &stages[0];
		bestImageRank = -999999;

		for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = &stages[stage];

			if (!pStage->active)
			{
				break;
			}
			rank = 0;

			if (pStage->bundle[0].isLightmap)
			{
				rank -= 100;
			}
			if (pStage->bundle[0].tcGen != TCGEN_TEXTURE)
			{
				rank -= 5;
			}
			if (pStage->bundle[0].numTexMods)
			{
				rank -= 5;
			}
			if (pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING)
			{
				rank -= 3;
			}

			if (rank > bestImageRank)
			{
				bestImageRank = rank;
				bestStage     = pStage;
			}
		}

		stages[0].bundle[0]  = bestStage->bundle[0];
		stages[0].stateBits &= ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if (shader.lightmapIndex == LIGHTMAP_NONE)
		{
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		}
		else
		{
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;
	}
	else
	{
		// don't use a lightmap (tesla coils)
		if (stages[0].bundle[0].isLightmap)
		{
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if (stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY)
		{
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ((stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH)
		    && (stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH))
		{
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ((stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH)
		    && (stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH))
		{
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for (stage = 1; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t *pStage = &stages[stage];

		if (!pStage->active)
		{
			break;
		}

		Com_Memset(pStage, 0, sizeof(*pStage));
	}
}
#endif // 0

/**
 * @brief Sets a shader's stages to one of several defaults
 * @param image
 */
static void SetImplicitShaderStages(image_t *image)
{
	// set implicit cull type
	if (implicitCullType && !shader.cullType)
	{
		shader.cullType = implicitCullType;
	}

	// set shader stages
	switch (shader.lightmapIndex)
	{
	// dynamic colors at vertexes
	case LIGHTMAP_NONE:
		stages[0].bundle[0].image[0] = image;
		stages[0].active             = qtrue;
		stages[0].rgbGen             = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits          = implicitStateBits;
		break;
	// gui elements (note state bits are overridden)
	case LIGHTMAP_2D:
		stages[0].bundle[0].image[0] = image;
		stages[0].active             = qtrue;
		stages[0].rgbGen             = CGEN_VERTEX;
		stages[0].alphaGen           = AGEN_SKIP;
		stages[0].stateBits          = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		break;
	// fullbright is disabled per atvi request
	case LIGHTMAP_WHITEIMAGE:
	// explicit colors at vertexes
	case LIGHTMAP_BY_VERTEX:
		stages[0].bundle[0].image[0] = image;
		stages[0].active             = qtrue;
		stages[0].rgbGen             = CGEN_EXACT_VERTEX;
		stages[0].alphaGen           = AGEN_SKIP;
		stages[0].stateBits          = implicitStateBits;
		break;
	// use lightmap pass
	default:
		// masked or blended implicit shaders need texture first
		if (implicitStateBits & (GLS_ATEST_BITS | GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
		{
			stages[0].bundle[0].image[0] = image;
			stages[0].active             = qtrue;
			stages[0].rgbGen             = CGEN_IDENTITY;
			stages[0].stateBits          = implicitStateBits;

			stages[1].bundle[0].image[0]   = tr.lightmaps[shader.lightmapIndex];
			stages[1].bundle[0].isLightmap = qtrue;
			stages[1].active               = qtrue;
			stages[1].rgbGen               = CGEN_IDENTITY;
			stages[1].stateBits            = GLS_DEFAULT | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_EQUAL;
		}
		// otherwise do standard lightmap + texture
		else
		{
			stages[0].bundle[0].image[0]   = tr.lightmaps[shader.lightmapIndex];
			stages[0].bundle[0].isLightmap = qtrue;
			stages[0].active               = qtrue;
			stages[0].rgbGen               = CGEN_IDENTITY;
			stages[0].stateBits            = GLS_DEFAULT;

			stages[1].bundle[0].image[0] = image;
			stages[1].active             = qtrue;
			stages[1].rgbGen             = CGEN_IDENTITY;
			stages[1].stateBits          = GLS_DEFAULT | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		}
		break;
	}
}

/**
 * @brief Inits a shader
 * @param name
 * @param lighmapIndex
 */
static void InitShader(const char *name, int lightmapIndex)
{
	int i;

	// clear the global shader
	Com_Memset(&shader, 0, sizeof(shader));
	Com_Memset(&stages, 0, sizeof(stages));

	Q_strncpyz(shader.name, name, sizeof(shader.name));
	shader.lightmapIndex = lightmapIndex;

	for (i = 0 ; i < MAX_SHADER_STAGES ; i++)
	{
		stages[i].bundle[0].texMods = texMods[i];
	}
}

/**
 * @brief Returns a freshly allocated shader with all the needed info
 * from the current global working shader
 * @return
 */
static shader_t *FinishShader(void)
{
	int      stage, i;
	qboolean hasLightmapStage = qfalse;

	// set sky stuff appropriate
	if (shader.isSky)
	{
		shader.sort = SS_ENVIRONMENT;
	}

	// set polygon offset
	if (shader.polygonOffset && shader.sort == 0.f)
	{
		shader.sort = SS_DECAL;
	}

	// set appropriate stage information
	for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t *pStage = &stages[stage];

		if (!pStage->active)
		{
			break;
		}

		// check for a missing texture
		if (!pStage->bundle[0].image[0])
		{
			Ren_Warning("Shader %s has a stage with no image\n", shader.name);
			pStage->active = qfalse;
			continue;
		}

		// ditch this stage if it's detail and detail textures are disabled
		if (pStage->isDetail && !r_detailTextures->integer)
		{
			if (stage < (MAX_SHADER_STAGES - 1))
			{
				memmove(pStage, pStage + 1, sizeof(*pStage) * (MAX_SHADER_STAGES - stage - 1));
				// kill the last stage, since it's now a duplicate
				for (i = MAX_SHADER_STAGES - 1; i > stage; i--)
				{
					if (stages[i].active)
					{
						Com_Memset(&stages[i], 0, sizeof(*pStage));
						break;
					}
				}
				stage--;    // the next stage is now the current stage, so check it again
			}
			else
			{
				Com_Memset(pStage, 0, sizeof(*pStage));
			}
			continue;
		}

		// default texture coordinate generation
		if (pStage->bundle[0].isLightmap)
		{
			if (pStage->bundle[0].tcGen == TCGEN_BAD)
			{
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		}
		else
		{
			if (pStage->bundle[0].tcGen == TCGEN_BAD)
			{
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}

		// determine sort order and fog color adjustment
		if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) &&
		    (stages[0].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)))
		{
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if (((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE)) ||
			    ((blendSrcBits == GLS_SRCBLEND_ZERO) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR)))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ((blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			}
			// zero + blended alpha, one + blended alpha
			else if (blendSrcBits == GLS_SRCBLEND_SRC_ALPHA || blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			else
			{
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if (shader.sort == 0.f)
			{
				// see through item, like a grill or grate
				if (pStage->stateBits & GLS_DEPTHMASK_TRUE)
				{
					shader.sort = SS_SEE_THROUGH;
				}
				else
				{
					shader.sort = SS_BLEND0;
				}
			}
		}
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if (shader.sort == 0.f)
	{
		shader.sort = SS_OPAQUE;
	}

	// look for multitexture potential
	if (stage > 1 && CollapseMultitexture())
	{
		stage--;
	}

	if (shader.lightmapIndex >= 0 && !hasLightmapStage)
	{
		Ren_Developer("WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name);
		shader.lightmapIndex = LIGHTMAP_NONE;
	}

	// compute number of passes
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if (stage == 0)
	{
		shader.sort = SS_FOG;
	}

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	// default back to no compression for next shader
	if (r_extCompressedTextures->integer == 2)
	{
		tr.allowCompress = qfalse;
	}

	return GeneratePermanentShader();
}

//========================================================================================

typedef struct dynamicshader dynamicshader_t;

/**
 * @struct dynamicshader
 * @brief Dynamic shader list
 */
struct dynamicshader
{
	char *shadertext;
	dynamicshader_t *next;
};
static dynamicshader_t *dshader = NULL;

/**
 * @brief load a new dynamic shader
 * if shadertext is NULL, looks for matching shadername and removes it
 *
 * @param[in] shadername
 * @param[in] shadertext
 *
 * @return qtrue if request was successful, qfalse if the gods were angered
 */
qboolean RE_LoadDynamicShader(const char *shadername, const char *shadertext)
{
	const char      *func_err = "WARNING: RE_LoadDynamicShader";
	dynamicshader_t *dptr, *lastdptr;
	char            *q, *token;

	if (!shadername && shadertext)
	{
		Ren_Warning("RE_LoadDynamicShader: %s called with NULL shadername and non-NULL shadertext:\n%s\n", func_err, shadertext);
		return qfalse;
	}

	if (shadername && strlen(shadername) >= MAX_QPATH)
	{
		Ren_Warning("RE_LoadDynamicShader: %s shadername %s exceeds MAX_QPATH\n", func_err, shadername);
		return qfalse;
	}

	// empty the whole list
	if (!shadername && !shadertext)
	{
		dptr = dshader;
		while (dptr)
		{
			lastdptr = dptr->next;
			ri.Free(dptr->shadertext);
			ri.Free(dptr);
			dptr = lastdptr;
		}
		dshader = NULL;
		return qtrue;
	}

	// walk list for existing shader to delete, or end of the list
	dptr     = dshader;
	lastdptr = NULL;
	while (dptr)
	{
		q = dptr->shadertext;

		token = COM_ParseExt(&q, qtrue);

		if ((token[0] != 0) && !Q_stricmp(token, shadername))
		{
			// request to nuke this dynamic shader
			if (!shadertext)
			{
				if (!lastdptr)
				{
					dshader = NULL;
				}
				else
				{
					lastdptr->next = dptr->next;
				}
				ri.Free(dptr->shadertext);
				ri.Free(dptr);
				return qtrue;
			}
			Ren_Warning("RE_LoadDynamicShader: %s shader %s already exists!\n", func_err, shadername);
			return qfalse;
		}
		lastdptr = dptr;
		dptr     = dptr->next;
	}

	// cant add a new one with empty shadertext
	if (!shadertext || !strlen(shadertext))
	{
		Ren_Warning("RE_LoadDynamicShader: %s new shader %s has NULL shadertext!\n", func_err, shadername);
		return qfalse;
	}

	// create a new shader
	dptr = (dynamicshader_t *)ri.Z_Malloc(sizeof(*dptr));
	if (!dptr)
	{
		Ren_Fatal("Couldn't allocate struct for dynamic shader %s", shadername);
	}
	if (lastdptr)
	{
		lastdptr->next = dptr;
	}
	dptr->shadertext = ri.Z_Malloc(strlen(shadertext) + 1);
	if (!dptr->shadertext)
	{
		Ren_Fatal("Couldn't allocate buffer for dynamic shader %s", shadername);
	}
	Q_strncpyz(dptr->shadertext, shadertext, strlen(shadertext) + 1);
	dptr->next = NULL;
	if (!dshader)
	{
		dshader = dptr;
	}

	//ri.Printf( PRINT_ALL, "Loaded dynamic shader [%s] with shadertext [%s]\n", shadername, shadertext );

	return qtrue;
}

/**
 * @brief Scans the combined text description of all the shader files for
 * the given shader name.
 *
 * @param[in] shadername
 *
 * @return If found, it will return a valid shader. Otherwise return NULL if not found
 */
static char *FindShaderInShaderText(const char *shadername)
{
	char *p = s_shaderText;
	char *token;
#ifdef SH_LOADTIMING
	static int total = 0;

	int start = ri.Milliseconds();
#endif // SH_LOADTIMING

	if (!p)
	{
		Ren_Warning("FindShaderInShaderText WARNING: no shadertext found for shader '%s'\n", shadername);
		return NULL;
	}

	// if we have any dynamic shaders loaded, check them first
	if (dshader)
	{
		dynamicshader_t *dptr = dshader;
		char            *q;
		int             i = 0;

		while (dptr)
		{
			if (!dptr->shadertext || !strlen(dptr->shadertext))
			{
				Ren_Warning("WARNING: dynamic shader %s(%d) has no shadertext\n", shadername, i);
			}
			else
			{
				q = dptr->shadertext;

				token = COM_ParseExt(&q, qtrue);

				if ((token[0] != 0) && !Q_stricmp(token, shadername))
				{
#ifdef SH_LOADTIMING
					total += ri.Milliseconds() - start;
					Ren_Print("Shader lookup dshader '%s': %i, total: %i\n", shadername, ri.Milliseconds() - start, total);
#endif // SH_LOADTIMING
					//ri.Printf( PRINT_ALL, "Found dynamic shader [%s] with shadertext [%s]\n", shadername, dptr->shadertext );
					return q;
				}
			}
			i++;
			dptr = dptr->next;
		}
	}

	// optimized shader loading
	if (r_cacheShaders->integer)
	{
		/*if (strstr( shadername, "/" ) && !strstr( shadername, "." ))*/
		{
			unsigned short int    checksum;
			shaderStringPointer_t *pShaderString;

			checksum = generateHashValue(shadername);

			// if it's known, skip straight to it's position
			pShaderString = &shaderChecksumLookup[checksum];
			while (pShaderString && pShaderString->pStr)
			{
				p = pShaderString->pStr;

				token = COM_ParseExt(&p, qtrue);

				if ((token[0] != 0) && !Q_stricmp(token, shadername))
				{
#ifdef SH_LOADTIMING
					total += ri.Milliseconds() - start;
					Ren_Print("Shader lookup cached '%s': %i, total: %i\n", shadername, ri.Milliseconds() - start, total);
#endif // SH_LOADTIMING
					return p;
				}

				pShaderString = pShaderString->next;
			}
			
			// it's not even in our list, so it mustn't exist
#ifdef SH_LOADTIMING
			total += ri.Milliseconds() - start;
			Ren_Print("Shader lookup cached failed '%s': %i, total: %i\n", shadername, ri.Milliseconds() - start, total);
#endif // SH_LOADTIMING
			return NULL;
		}
	}

	// look for label
	// note that this could get confused if a shader name is used inside
	// another shader definition
	while (1)
	{
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0)
		{
			break;
		}

		if (!Q_stricmp(token, shadername))
		{
#ifdef SH_LOADTIMING
			total += ri.Milliseconds() - start;
			Ren_Print("Shader lookup label '%s': %i, total: %i\n", shadername, ri.Milliseconds() - start, total);
#endif // SH_LOADTIMING
			return p;
		}

		SkipBracedSection(&p);
	}

#ifdef SH_LOADTIMING
	total += ri.Milliseconds() - start;
	Ren_Print("Shader lookup '%s' failed: %i, total: %i\n", shadername, ri.Milliseconds() - start, total);
#endif // SH_LOADTIMING
	return NULL;
}

/**
 * @brief Will always return a valid shader, but it might be the
 * default shader if the real one can't be found.
 * @param[in] name
 * @return
 */
shader_t *R_FindShaderByName(const char *name)
{
	char     strippedName[MAX_QPATH];
	int      hash;
	shader_t *sh;

	if ((name == NULL) || (name[0] == 0))
	{
		Ren_Warning("R_FindShaderByName WARNING: Name is empty - returning default shader\n");
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));
	COM_FixPath(strippedName);

	hash = generateHashValue(strippedName);

	// see if the shader is already loaded
	for (sh = hashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, strippedName) == 0)
		{
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}

#define EXTERNAL_LIGHTMAP   "lm_%04d.tga"    // THIS MUST BE IN SYNC WITH Q3MAP2

/**
 * @brief Given a (potentially erroneous) lightmap index, attempts to load
 * an external lightmap image and/or sets the index to a valid number
 * @param[in,out] lightmapIndex
 */
void R_FindLightmap(int *lightmapIndex)
{
	image_t *image;
	char    fileName[MAX_QPATH];

	// don't fool with bogus lightmap indexes
	if (*lightmapIndex < 0)
	{
		return;
	}

	// does this lightmap already exist?
	if (*lightmapIndex < tr.numLightmaps && tr.lightmaps[*lightmapIndex] != NULL)
	{
		return;
	}

	// bail if no world dir
	if (tr.worldDir == NULL)
	{
		*lightmapIndex = LIGHTMAP_BY_VERTEX;
		return;
	}

	// sync up render thread, because we're going to have to load an image
	R_IssuePendingRenderCommands();

	// attempt to load an external lightmap
	sprintf(fileName, "%s/" EXTERNAL_LIGHTMAP, tr.worldDir, *lightmapIndex);
	image = R_FindImageFile(fileName, qfalse, qfalse, GL_CLAMP_TO_EDGE, qtrue);
	if (image == NULL)
	{
		*lightmapIndex = LIGHTMAP_BY_VERTEX;
		return;
	}

	// add it to the lightmap list
	if (*lightmapIndex >= tr.numLightmaps)
	{
		tr.numLightmaps = *lightmapIndex + 1;
	}
	tr.lightmaps[*lightmapIndex] = image;
}

/**
 * @brief Will always return a valid shader, but it might be the
 * default shader if the real one can't be found.
 *
 * In the interest of not requiring an explicit shader text entry to
 * be defined for every single image used in the game, three default
 * shader behaviors can be auto-created for any image:
 *
 * If lightmapIndex == LIGHTMAP_NONE, then the image will have
 * dynamic diffuse lighting applied to it, as apropriate for most
 * entity skin surfaces.
 *
 * If lightmapIndex == LIGHTMAP_2D, then the image will be used
 * for 2D rendering unless an explicit shader is found
 *
 * If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
 * the vertex rgba modulate values, as apropriate for misc_model
 * pre-lit surfaces.
 *
 * Other lightmapIndex values will have a lightmap stage created
 * and src*dest blending applied with the texture, as apropriate for
 * most world construction surfaces.
 *
 * @param[in] name
 * @param[in] lightmapIndex
 * @param[in] mipRawImage
 * @return
 */
shader_t *R_FindShader(const char *name, int lightmapIndex, qboolean mipRawImage)
{
	char     strippedName[MAX_QPATH];
	char     fileName[MAX_QPATH];
	int      hash;
	char     *shaderText;
	image_t  *image;
	shader_t *sh;

	if (name[0] == 0)
	{
		return tr.defaultShader;
	}

	// validate lightmap index
	R_FindLightmap(&lightmapIndex);

	COM_StripExtension(name, strippedName, sizeof(strippedName));
	COM_FixPath(strippedName);

	hash = generateHashValue(strippedName);

	// see if the shader is already loaded
#if 1
	for (sh = hashTable[hash]; sh; sh = sh->next)
	{
		// index by name

		// the original way was correct
		if (sh->lightmapIndex == lightmapIndex &&
		    !Q_stricmp(sh->name, strippedName))
		{
			// match found
			return sh;
		}

		// modified this so we don't keep trying to load an invalid lightmap shader
#if 0
		if (((sh->lightmapIndex == lightmapIndex) || (sh->lightmapIndex < 0 && lightmapIndex >= 0)) &&
		    !Q_stricmp(sh->name, strippedName))
		{
			// match found
			return sh;
		}
#endif
	}
#else
	for (sh = hashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ((sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
		    !Q_stricmp(sh->name, strippedName))
		{
			// match found
			return sh;
		}
	}
#endif

	// check the cache
	// assignment used as truth value
	// - don't cache shaders using lightmaps
	if (lightmapIndex < 0)
	{
		sh = R_FindCachedShader(strippedName, lightmapIndex, hash);
		if (sh != NULL)
		{
			return sh;
		}
	}

	InitShader(strippedName, lightmapIndex);

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1    = qtrue;
	shader.needsST2    = qtrue;
	shader.needsColor  = qtrue;

	// default to no implicit mappings
	implicitMap[0]    = '\0';
	implicitStateBits = GLS_DEFAULT;
	implicitCullType  = CT_FRONT_SIDED;

	// attempt to define shader from an explicit parameter file
	shaderText = FindShaderInShaderText(strippedName);
	if (shaderText)
	{
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if (r_printShaders->integer)
		{
			Ren_Print("*SHADER* %s\n", name);
		}

		if (!ParseShader(&shaderText))
		{
			// had errors, so use default shader
			shader.defaultShader = qtrue;
			sh                   = FinishShader();
			return sh;
		}

		// allow implicit mappings
		if (implicitMap[0] == '\0')
		{
			sh = FinishShader();
			return sh;
		}
	}

	// allow implicit mapping ('-' = use shader name)
	if (implicitMap[0] == '\0' || implicitMap[0] == '-')
	{
		Q_strncpyz(fileName, name, sizeof(fileName));
	}
	else
	{
		Q_strncpyz(fileName, implicitMap, sizeof(fileName));
	}
	COM_DefaultExtension(fileName, sizeof(fileName), ".tga");

	// implicit shaders were breaking nopicmip/nomipmaps
	if (!mipRawImage)
	{
		shader.noMipMaps = qtrue;
		shader.noPicMip  = qtrue;
	}

	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	image = R_FindImageFile(fileName, !shader.noMipMaps, !shader.noPicMip, mipRawImage ? GL_REPEAT : GL_CLAMP_TO_EDGE, qfalse);
	if (!image)
	{
		Ren_Developer("WARNING: Couldn't find image for shader %s\n", name);
		shader.defaultShader = qtrue;
		return FinishShader();
	}

	// set default stages (removing redundant code)
	SetImplicitShaderStages(image);

	return FinishShader();
}

/**
 * @brief RE_RegisterShaderFromImage
 * @param[in] name
 * @param[in] lightmapIndex
 * @param[in] image
 * @param mipRawImage - unused
 * @return
 */
qhandle_t RE_RegisterShaderFromImage(const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage)
{
	int      hash;
	shader_t *sh;

	hash = generateHashValue(name);

	// see if the shader is already loaded
	for (sh = hashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ((sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
		    // index by name
		    !Q_stricmp(sh->name, name))
		{
			// match found
			return sh->index;
		}
	}

	// clear the global shader
	InitShader(name, lightmapIndex);

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1    = qtrue;
	shader.needsST2    = qtrue;
	shader.needsColor  = qtrue;

	// set default stages (removing redundant code)
	SetImplicitShaderStages(image);

	sh = FinishShader();
	return sh->index;
}

/**
 * @brief This is the exported shader entry point for the rest of the system
 * It will always return an index that will be valid.
 *
 * This should really only be used for explicit shaders, because there is no
 * way to ask for different implicit lighting modes (vertex, lightmap, etc)
 *
 * @param[in] name
 * @param[in] lightmapIndex
 * @return
 */
qhandle_t RE_RegisterShaderLightMap(const char *name, int lightmapIndex)
{
	shader_t *sh;

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Print("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, lightmapIndex, qtrue);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShaderLightMap again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		Ren_Warning("RE_RegisterShaderLightMap WARNING: shader '%s' not found - using default shader\n", name);
		return 0;
	}

	return sh->index;
}

/**
 * @brief This is the exported shader entry point for the rest of the system
 * It will always return an index that will be valid.
 *
 * This should really only be used for explicit shaders, because there is no
 * way to ask for different implicit lighting modes (vertex, lightmap, etc)
 *
 * @param[in] name
 * @return
 */
qhandle_t RE_RegisterShader(const char *name)
{
	shader_t *sh;

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Warning("RE_RegisterShader WARNING: shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, LIGHTMAP_2D, qtrue);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		Ren_Warning("RE_RegisterShader WARNING: shader '%s' not found - using default shader\n", name);
		return 0;
	}

	return sh->index;
}

/**
 * @brief RE_RegisterShaderNoMip
 * @param name
 * @return
 *
 * @note For menu graphics that should never be picmiped
 */
qhandle_t RE_RegisterShaderNoMip(const char *name)
{
	shader_t *sh;

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Warning("RE_RegisterShaderNoMip WARNING: shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, LIGHTMAP_2D, qfalse);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		Ren_Developer("RE_RegisterShaderNoMip WARNING: shader '%s' not found - using default shader\n", name);
		return 0;
	}

	return sh->index;
}

/**
 * @brief When a handle is passed in by another module, this range checks
 * it and returns a valid (possibly default) shader_t to be used internally.
 * @param[in] hShader
 * @return
 */
shader_t *R_GetShaderByHandle(qhandle_t hShader)
{
	if (hShader < 0)
	{
		Ren_Developer("R_GetShaderByHandle: out of range hShader '%d'\n", hShader); // FIXME name
		return tr.defaultShader;
	}
	if (hShader >= tr.numShaders)
	{
		Ren_Developer("R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/**
 * @brief Dump information on all valid shaders to the console
 * A second parameter will cause it to print in sorted order
 */
void R_ShaderList_f(void)
{
	int      i;
	int      count = 0;
	shader_t *shader;

	Ren_Print("-----------------------\n");

	for (i = 0 ; i < tr.numShaders ; i++)
	{
		if (ri.Cmd_Argc() > 1)
		{
			shader = tr.sortedShaders[i];
		}
		else
		{
			shader = tr.shaders[i];
		}

		Ren_Print("%i ", shader->numUnfoggedPasses);

		if (shader->lightmapIndex >= 0)
		{
			Ren_Print("L ");
		}
		else
		{
			Ren_Print("  ");
		}

		if (shader->multitextureEnv == GL_ADD)
		{
			Ren_Print("MT(a) ");
		}
		else if (shader->multitextureEnv == GL_MODULATE)
		{
			Ren_Print("MT(m) ");
		}
		else if (shader->multitextureEnv == GL_DECAL)
		{
			Ren_Print("MT(d) ");
		}
		else
		{
			Ren_Print("      ");
		}

		if (shader->explicitlyDefined)
		{
			Ren_Print("E ");
		}
		else
		{
			Ren_Print("  ");
		}

		if (shader->optimalStageIteratorFunc == RB_StageIteratorGeneric)
		{
			Ren_Print("gen ");
		}
		else if (shader->optimalStageIteratorFunc == RB_StageIteratorSky)
		{
			Ren_Print("sky ");
		}
		else if (shader->optimalStageIteratorFunc == RB_StageIteratorLightmappedMultitexture)
		{
			Ren_Print("lmmt");
		}
		else if (shader->optimalStageIteratorFunc == RB_StageIteratorVertexLitTexture)
		{
			Ren_Print("vlt ");
		}
		else
		{
			Ren_Print("    ");
		}

		if (shader->defaultShader)
		{
			Ren_Print(": %s (DEFAULTED)\n", shader->name);
		}
		else
		{
			Ren_Print(": %s\n", shader->name);
		}
		count++;
	}
	Ren_Print("%i total shaders\n", count);
	Ren_Print("------------------\n");
}

// optimized shader loading

#define MAX_SHADER_STRING_POINTERS  100000
shaderStringPointer_t shaderStringPointerList[MAX_SHADER_STRING_POINTERS];

/**
 * @brief BuildShaderChecksumLookup
 */
static void BuildShaderChecksumLookup(void)
{
	char               *p = s_shaderText, *pOld;
	char               *token;
	unsigned short int checksum;
	int                numShaderStringPointers = 0;

	// initialize the checksums
	Com_Memset(shaderChecksumLookup, 0, sizeof(shaderChecksumLookup));

	if (!p)
	{
		return;
	}

	// loop for all labels
	while (1)
	{
		pOld = p;

		token = COM_ParseExt(&p, qtrue);
		if (!*token)
		{
			break;
		}

		// get it's checksum
		checksum = generateHashValue(token);

		//Ren_Print("Shader Found: %s\n", token );

		// if it's not currently used
		if (!shaderChecksumLookup[checksum].pStr)
		{
			shaderChecksumLookup[checksum].pStr = pOld;
		}
		else
		{
			// create a new list item
			shaderStringPointer_t *newStrPtr;

			if (numShaderStringPointers >= MAX_SHADER_STRING_POINTERS)
			{
				Ren_Drop("MAX_SHADER_STRING_POINTERS exceeded, too many shaders");
			}

			newStrPtr                           = &shaderStringPointerList[numShaderStringPointers++]; //ri.Hunk_Alloc( sizeof( shaderStringPointer_t ), h_low );
			newStrPtr->pStr                     = pOld;
			newStrPtr->next                     = shaderChecksumLookup[checksum].next;
			shaderChecksumLookup[checksum].next = newStrPtr;
		}

		// skip the actual shader section
		SkipBracedSection(&p);
	}
}

#define MAX_SHADER_FILES    4096
/**
 * @brief Finds and loads all .shader files, combining them into
 * a single large text block that can be scanned for shader names
 */
static void ScanAndLoadShaderFiles(void)
{
	char filename[MAX_QPATH];
	char **shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	int  buffersize[MAX_SHADER_FILES];
	char *p = NULL;
	int  numShaders;
	int  i;
	long sum = 0;

	Com_Memset(buffers, 0, sizeof(buffers));
	Com_Memset(buffersize, 0, sizeof(buffersize));

	// scan for shader files
	shaderFiles = ri.FS_ListFiles("scripts", ".shader", &numShaders);

	if (!shaderFiles || !numShaders)
	{
		Ren_Warning("ScanAndLoadShaderFiles WARNING: no shader files found\n");
		return;
	}

	if (numShaders > MAX_SHADER_FILES)
	{
		numShaders = MAX_SHADER_FILES;
		Ren_Warning("ScanAndLoadShaderFiles WARNING: MAX_SHADER_FILES reached\n");
	}

	// load and parse shader files
	for (i = 0; i < numShaders; i++)
	{
		Com_sprintf(filename, sizeof(filename), "scripts/%s", shaderFiles[i]);
		Ren_Developer("...loading '%s'\n", filename);
		buffersize[i] = ri.FS_ReadFile(filename, (void **)&buffers[i]);
		sum          += buffersize[i];
		if (!buffers[i])
		{
			Ren_Drop("ScanAndLoadShaderFiles: Couldn't load %s", filename);
		}
	}

	// build single large buffer
	s_shaderText = ri.Hunk_Alloc(sum + numShaders * 2, h_low);

	// optimised to not use strcat/strlen which can be VERY slow for the large strings we're using here
	p = s_shaderText;
	// free in reverse order, so the temp files are all dumped
	for (i = numShaders - 1; i >= 0 ; i--)
	{
		strcpy(p++, "\n");
		strcpy(p, buffers[i]);
		ri.FS_FreeFile(buffers[i]);
		buffers[i] = p;
		p         += buffersize[i];
	}

	// unixify all shaders
	COM_FixPath(s_shaderText);

	// free up memory
	ri.FS_FreeFileList(shaderFiles);

	// optimized shader loading (18ms on a P3-500 for sfm1.bsp)
	if (r_cacheShaders->integer)
	{
		BuildShaderChecksumLookup();
	}
}

/**
 * @brief CreateInternalShaders
 */
static void CreateInternalShaders(void)
{
	tr.numShaders = 0;

	// init the default shader
	InitShader("<default>", LIGHTMAP_NONE);

	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active             = qtrue;
	stages[0].stateBits          = GLS_DEFAULT;
	tr.defaultShader             = FinishShader();

	// shadow shader is just a marker
	Q_strncpyz(shader.name, "<stencil shadow>", sizeof(shader.name));
	shader.sort     = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();
}

static void CreateExternalShaders(void)
{
	tr.projectionShadowShader = R_FindShader("projectionShadow", LIGHTMAP_NONE, qtrue);
	tr.flareShader            = R_FindShader("flareShader", LIGHTMAP_NONE, qtrue);
	tr.sunflareShader[0]      = R_FindShader("sunflare1", LIGHTMAP_NONE, qtrue);
	tr.dlightShader           = R_FindShader("dlightshader", LIGHTMAP_NONE, qtrue);
}

//=============================================================================
// shader caching
static int      numBackupShaders = 0;
static shader_t *backupShaders[MAX_SHADERS];
static shader_t *backupHashTable[FILE_HASH_SIZE];

/**
 * @brief R_CacheShaderAllocExt
 * @param name - unused
 * @param[in] size
 * @param file - unused
 * @param line - unused
 * @return
 */
void *R_CacheShaderAllocExt(const char *name, int size, const char *file, int line)
{
	if (r_cache->integer && r_cacheShaders->integer)
	{
		void *ptr = ri.Z_Malloc(size);
		return ptr;
	}
	else
	{
		return ri.Hunk_Alloc(size, h_low);
	}
}

/**
 * @brief R_CacheShaderFreeExt
 * @param name - unused
 * @param[in] ptr
 * @param file - unused
 * @param line - unused
 */
void R_CacheShaderFreeExt(const char *name, void *ptr, const char *file, int line)
{
	if (r_cache->integer && r_cacheShaders->integer)
	{
		ri.Free(ptr);
	}
}

qboolean purgeallshaders = qfalse;

/**
 * @brief R_PurgeShaders
 * @param count - unused
 */
void R_PurgeShaders(int count)
{
	if (!numBackupShaders)
	{
		return;
	}

	purgeallshaders = qtrue;
	R_PurgeLightmapShaders();
	purgeallshaders = qfalse;
}

/**
 * @brief R_ShaderCanBeCached
 * @param[in] sh
 * @return
 */
qboolean R_ShaderCanBeCached(shader_t *sh)
{
	int i, j, b;

	if (purgeallshaders)
	{
		return qfalse;
	}

	if (sh->isSky)
	{
		return qfalse;
	}

	for (i = 0; i < sh->numUnfoggedPasses; i++)
	{
		if (sh->stages[i] && sh->stages[i]->active)
		{
			for (b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++)
			{
				// swapped order of for() comparisons so that
				// image[16] (out of bounds) isn't dereferenced
				//for (j=0; sh->stages[i]->bundle[b].image[j] && j < MAX_IMAGE_ANIMATIONS; j++) {
				for (j = 0; j < MAX_IMAGE_ANIMATIONS && sh->stages[i]->bundle[b].image[j]; j++)
				{
					if (sh->stages[i]->bundle[b].image[j]->imgName[0] == '*')
					{
						return qfalse;
					}
				}
			}
		}
	}
	return qtrue;
}

/**
 * @brief R_PurgeLightmapShaders
 */
void R_PurgeLightmapShaders(void)
{
	int      j, b, i = 0;
	shader_t *sh, *shPrev, *next;

	for (i = 0; i < sizeof(backupHashTable) / sizeof(backupHashTable[0]); i++)
	{
		sh = backupHashTable[i];

		shPrev = NULL;
		next   = NULL;

		while (sh)
		{
			if (sh->lightmapIndex >= 0 || !R_ShaderCanBeCached(sh))
			{
				next = sh->next;

				if (!shPrev)
				{
					backupHashTable[i] = sh->next;
				}
				else
				{
					shPrev->next = sh->next;
				}

				backupShaders[sh->index] = NULL;    // make sure we don't try and free it

				numBackupShaders--;

				for (j = 0 ; j < sh->numUnfoggedPasses ; j++)
				{
					if (!sh->stages[j])
					{
						break;
					}
					for (b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++)
					{
						if (sh->stages[j]->bundle[b].texMods)
						{
							R_CacheShaderFree(NULL, sh->stages[j]->bundle[b].texMods);
						}
					}
					R_CacheShaderFree(NULL, sh->stages[j]);
				}
				R_CacheShaderFree(sh->lightmapIndex < 0 ? va("%s lm: %i", sh->name, sh->lightmapIndex) : NULL, sh);

				sh = next;

				continue;
			}

			shPrev = sh;
			sh     = sh->next;
		}
	}
}

/**
 * @brief R_BackupShaders
 */
void R_BackupShaders(void)
{
	//int i;

	if (!r_cache->integer)
	{
		return;
	}
	if (!r_cacheShaders->integer)
	{
		return;
	}

	// copy each model in memory across to the backupModels
	Com_Memcpy(backupShaders, tr.shaders, sizeof(backupShaders));
	// now backup the hashTable
	Com_Memcpy(backupHashTable, hashTable, sizeof(hashTable));

	numBackupShaders = tr.numShaders;

	// ditch all lightmapped shaders
	R_PurgeLightmapShaders();

	//Ren_Print("Backing up %i images\n", numBackupShaders );

	//for( i = 0; i < tr.numShaders; i++ ) {
	//    if( backupShaders[ i ] ) {
	//        Ren_Print("Shader: %s: lm %i\n", backupShaders[ i ]->name, backupShaders[i]->lightmapIndex );
	//    }
	//}

	//Ren_Print("=======================================\n" );
}

/**
 * @brief Make sure all images that belong to this shader remain valid
 * @param[in] sh
 * @return
 */
static qboolean R_RegisterShaderImages(shader_t *sh)
{
	int i, j, b;

	if (sh->isSky)
	{
		return qfalse;
	}

	for (i = 0; i < sh->numUnfoggedPasses; i++)
	{
		if (sh->stages[i] && sh->stages[i]->active)
		{
			for (b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++)
			{
				for (j = 0; j < MAX_IMAGE_ANIMATIONS && sh->stages[i]->bundle[b].image[j]; j++)
				{
					if (!R_TouchImage(sh->stages[i]->bundle[b].image[j]))
					{
						return qfalse;
					}
				}
			}
		}
	}
	return qtrue;
}

/**
 * @brief Look for the given shader in the list of backupShaders
 * @param[in] name
 * @param[in] lightmapIndex
 * @param[in] hash
 * @return
 */
shader_t *R_FindCachedShader(const char *name, int lightmapIndex, int hash)
{
	shader_t *sh, *shPrev;

	if (!r_cacheShaders->integer)
	{
		return NULL;
	}

	if (!numBackupShaders)
	{
		return NULL;
	}

	if (!name)
	{
		return NULL;
	}

	sh     = backupHashTable[hash];
	shPrev = NULL;
	while (sh)
	{
		if (sh->lightmapIndex == lightmapIndex && !Q_stricmp(sh->name, name))
		{
			if (tr.numShaders == MAX_SHADERS)
			{
				Ren_Warning("WARNING: R_FindCachedShader - MAX_SHADERS hit\n");
				return NULL;
			}

			// make sure the images stay valid
			if (!R_RegisterShaderImages(sh))
			{
				return NULL;
			}

			// this is the one, so move this shader into the current list

			if (!shPrev)
			{
				backupHashTable[hash] = sh->next;
			}
			else
			{
				shPrev->next = sh->next;
			}

			sh->next        = hashTable[hash];
			hashTable[hash] = sh;

			backupShaders[sh->index] = NULL;    // make sure we don't try and free it

			// set the index up, and add it to the current list
			tr.shaders[tr.numShaders] = sh;
			sh->index                 = tr.numShaders;

			tr.sortedShaders[tr.numShaders] = sh;
			sh->sortedIndex                 = tr.numShaders;

			tr.numShaders++;

			numBackupShaders--;

			sh->remappedShader = NULL;  // remove any remaps

			SortNewShader();    // make sure it renders in the right order

			//Ren_Print("Removing %s from the cache: lm: %i\n", sh->name, sh->lightmapIndex );

			return sh;
		}

		shPrev = sh;
		sh     = sh->next;
	}

	return NULL;
}

/**
 * @brief R_LoadCacheShaders
 */
void R_LoadCacheShaders(void)
{
	int  len;
	char *buf;
	char *token, *pString;
	char name[MAX_QPATH];

	if (!r_cacheShaders->integer)
	{
		return;
	}

	// don't load the cache list in between level loads, only on startup, or after a vid_restart
	if (numBackupShaders > 0)
	{
		return;
	}

	len = ri.FS_ReadFile("shader.cache", NULL);

	if (len <= 0)
	{
		return;
	}

	buf = (char *)ri.Hunk_AllocateTempMemory(len);
	ri.FS_ReadFile("shader.cache", (void **)&buf);
	pString = buf;

	while ((token = COM_ParseExt(&pString, qtrue)) && token[0])
	{
		Q_strncpyz(name, token, sizeof(name));
		RE_RegisterModel(name);
	}

	ri.Hunk_FreeTempMemory(buf);
}

//=============================================================================

/**
 * @brief R_InitShaders
 */
void R_InitShaders(void)
{
	glfogNum = FOG_NONE;

	Ren_Print("Initializing Shaders\n");

	Com_Memset(hashTable, 0, sizeof(hashTable));
	CreateInternalShaders();
	ScanAndLoadShaderFiles();
	CreateExternalShaders();
	R_LoadCacheShaders();
}
