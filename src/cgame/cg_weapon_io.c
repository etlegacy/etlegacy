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
 * @file cg_weapon_io.c
 * @brief Handle weapon file loading
*/

#include "cg_local.h"

/**
 * @brief Read information for weapon animations (first/length/fps)
 * @param[in] filename
 * @param[out] wi
 * @return
 */
static qboolean CG_ParseWeaponConfig(const char *filename, weaponInfo_t *wi)
{
	char         *text_p, *prev;
	int          len;
	int          i;
	float        fps;
	char         *token;
	qboolean     newfmt = qfalse;
	char         text[20000];
	fileHandle_t f;

	// load the file
	len = CG_FOpenCompatFile(filename, &f, FS_READ);

	if (len <= 0)
	{
		CG_Printf("CG_ParseWeaponConfig: File not found: %s\n", filename);
		return qfalse;
	}

	if (len >= sizeof(text) - 1)
	{
		CG_Printf("CG_ParseWeaponConfig: File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;

	COM_BeginParseSession("CG_ParseWeaponConfig");

	// read optional parameters
	while (1)
	{
		prev  = text_p; // so we can unget
		token = COM_Parse(&text_p);

		if (!token[0])                         // get the variable
		{
			break;
		}

		//if ( !Q_stricmp( token, "whatever_variable" ) )
		//{
		//  token = COM_Parse( &text_p );   // get the value
		//  if (!token[0])
		//  {
		//    break;
		//  }
		//  continue;
		//}

		if (!Q_stricmp(token, "newfmt"))
		{
			newfmt = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if (token[0] >= '0' && token[0] <= '9')
		{
			text_p = prev;  // unget the token
			break;
		}

		Com_Printf("CG_ParseWeaponConfig: Unknown token in weapon cfg '%s' in %s\n", token, filename);
	}

	for (i = 0 ; i < MAX_WP_ANIMATIONS  ; i++)
	{
		token = COM_Parse(&text_p);     // first frame

		if (!token[0])
		{
			break;
		}

		wi->weapAnimations[i].firstFrame = Q_atoi(token);

		token = COM_Parse(&text_p);     // length

		if (!token[0])
		{
			break;
		}

		wi->weapAnimations[i].numFrames = Q_atoi(token);

		token = COM_Parse(&text_p);     // fps

		if (!token[0])
		{
			break;
		}

		fps = Q_atof(token);

		if (fps == 0.f)
		{
			fps = 1;
		}

		wi->weapAnimations[i].frameLerp   = (int)(1000 / fps);
		wi->weapAnimations[i].initialLerp = (int)(1000 / fps);

		token = COM_Parse(&text_p);     // looping frames

		if (!token[0])
		{
			break;
		}

		wi->weapAnimations[i].loopFrames = Q_atoi(token);

		if (wi->weapAnimations[i].loopFrames > wi->weapAnimations[i].numFrames)
		{
			wi->weapAnimations[i].loopFrames = wi->weapAnimations[i].numFrames;
		}
		else if (wi->weapAnimations[i].loopFrames < 0)
		{
			wi->weapAnimations[i].loopFrames = 0;
		}

		// store animation/draw bits in '.moveSpeed'

		wi->weapAnimations[i].moveSpeed = 0;

		if (newfmt)
		{
			token = COM_Parse(&text_p);     // barrel anim bits

			if (!token[0])
			{
				break;
			}

			wi->weapAnimations[i].moveSpeed = Q_atoi(token);

			token = COM_Parse(&text_p);     // animated weapon

			if (!token[0])
			{
				break;
			}

			if (atoi(token))
			{
				wi->weapAnimations[i].moveSpeed |= (1 << W_MAX_PARTS);      // set the bit one higher than can be set by the barrel bits

			}

			token = COM_Parse(&text_p);     // barrel hide bits (so objects can be flagged to not be drawn during all sequences (a reloading hand that comes in from off screen for that one animation for example)

			if (!token[0])
			{
				break;
			}

			wi->weapAnimations[i].moveSpeed |= ((atoi(token)) << 8);       // use 2nd byte for draw bits
		}
	}

	if (i != MAX_WP_ANIMATIONS)
	{
		CG_Printf("CG_ParseWeaponConfig: Error parsing weapon animation file: %s\n", filename);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseError
 * @param[in] handle
 * @param[in] format
 * @return
 */
static qboolean CG_RW_ParseError(int handle, const char *format, ...)
{
	int         line;
	char        filename[MAX_QPATH];
	va_list     argptr;
	static char string[4096];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	filename[0] = '\0';
	line        = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);

	trap_PC_FreeSource(handle);

	return qfalse;
}

/**
 * @brief CG_RW_ParseWeaponLinkPart
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @param[in] viewType
 * @return
 */
static qboolean CG_RW_ParseWeaponLinkPart(int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType)
{
	pc_token_t  token;
	char        filename[MAX_QPATH];
	int         part;
	partModel_t *partModel;

	if (!PC_Int_Parse(handle, &part))
	{
		return CG_RW_ParseError(handle, "expected part index");
	}

	if (part < 0 || part >= W_MAX_PARTS)
	{
		return CG_RW_ParseError(handle, "part index out of bounds");
	}

	partModel = &weaponInfo->partModels[viewType][part];

	Com_Memset(partModel, 0, sizeof(*partModel));

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "tag"))
		{
			if (!PC_String_ParseNoAlloc(handle, partModel->tagName, sizeof(partModel->tagName)))
			{
				return CG_RW_ParseError(handle, "expected tag name");
			}
		}
		else if (!Q_stricmp(token.string, "model"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected model filename");
			}

			partModel->model = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "skin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			partModel->skin[0] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "axisSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			partModel->skin[TEAM_AXIS] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "alliedSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			partModel->skin[TEAM_ALLIES] = trap_R_RegisterSkin(filename);
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseWeaponLink
 * @param[in] handle
 * @param[in] weaponInfo
 * @param[in] viewType
 * @return
 */
static qboolean CG_RW_ParseWeaponLink(int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "part"))
		{
			if (!CG_RW_ParseWeaponLinkPart(handle, weaponInfo, viewType))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseViewType
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @param[in] viewType
 * @return
 */
static qboolean CG_RW_ParseViewType(int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType)
{
	pc_token_t token;
	char       filename[MAX_QPATH];
	float       value;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "model"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected model filename");
			}

			weaponInfo->weaponModel[viewType].model = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "skin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->weaponModel[viewType].skin[0] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "axisSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->weaponModel[viewType].skin[TEAM_AXIS] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "alliedSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->weaponModel[viewType].skin[TEAM_ALLIES] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "flashModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected flashModel filename");
			}

			weaponInfo->flashModel[viewType] = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "flashScale"))
		{
			if (!PC_Float_Parse(handle, &value))
			{
				return CG_RW_ParseError(handle, "expected flashScale as float");
			}

			weaponInfo->flashScale[viewType] = value;
		}
		else if (!Q_stricmp(token.string, "weaponLink"))
		{
			if (!CG_RW_ParseWeaponLink(handle, weaponInfo, viewType))
			{
				return qfalse;
			}
		}
		else if (viewType == W_FP_MODEL && !Q_stricmp(token.string, "ejectBrassOffset"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->firstPersonEjectBrassOffset))
			{
				return CG_RW_ParseError(handle, "expected ejectBrassOffset as forward left up");
			}
		}
		else if (viewType == W_TP_MODEL && !Q_stricmp(token.string, "ejectBrassOffset"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->thirdPersonEjectBrassOffset))
			{
				return CG_RW_ParseError(handle, "expected ejectBrassOffset as forward left up");
			}
		}
		else if (!Q_stricmp(token.string, "dynFov90"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->dynFov90))
			{
				return CG_RW_ParseError(handle, "expected dynFov90 as up forward right");
			}
		}
		else if (!Q_stricmp(token.string, "dynFov120"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->dynFov120))
			{
				return CG_RW_ParseError(handle, "expected dynFov120 as up forward right");
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseModModel
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @return
 */
static qboolean CG_RW_ParseModModel(int handle, weaponInfo_t *weaponInfo)
{
	char filename[MAX_QPATH];
	int  mod;

	if (!PC_Int_Parse(handle, &mod))
	{
		return CG_RW_ParseError(handle, "expected mod index");
	}

	if (mod < 0 || mod >= 6)
	{
		return CG_RW_ParseError(handle, "mod index out of bounds");
	}

	if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
	{
		return CG_RW_ParseError(handle, "expected model filename");
	}

	// maybe it's a shader ...
	// check extensions of file name and register shader OR model
	if (!strstr(filename, ".md3") && !strstr(filename, ".mdc")) // FIXME: add more model formats?
	{   // we assume it's a shader
		weaponInfo->modModels[mod] = trap_R_RegisterShader(filename);
	}
	else
	{
		weaponInfo->modModels[mod] = trap_R_RegisterModel(filename);
	}

	// still no q_handle_t
	if (!weaponInfo->modModels[mod])
	{
		Com_Printf(S_COLOR_RED "ERROR: CG_RW_ParseModModel() no model or shader for %s registered.\n", filename);
		return qfalse; // this isn't vanilla behaviour
	}

	return qtrue;
}

/**
 * @var soundSurfaceTable
 */
const soundSurfaceTable_t soundSurfaceTable[W_MAX_SND_SURF] =
{
	{ 0,           "default" },
	{ 0,           "far",    },
	{ SURF_METAL,  "metal",  },
	{ SURF_WOOD,   "wood",   },
	{ SURF_GRASS,  "grass",  },
	{ SURF_GRAVEL, "gravel", },
	{ SURF_GLASS,  "glass",  },
	{ SURF_SNOW,   "snow",   },
	{ SURF_ROOF,   "roof",   },
	{ SURF_CARPET, "carpet"  },
	{ 0,           "water",  },
	{ 0,           "flesh"   }
};

/**
 * @brief CG_RW_ParseWeaponSound
 * @param[in] handle
 * @param[in,out] weaponSounds
 * @return
 */
static qboolean CG_RW_ParseWeaponSound(int handle, weaponSounds_t *weaponSounds)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return CG_RW_ParseError(handle, "expected sounds filenames or sounds number");
	}

	// get the number of files sound to register
	if (token.type == TT_NUMBER)
	{
		int  i;
		char filename[MAX_QPATH];

		if (token.intvalue > MAX_WEAPON_SOUNDS)
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: only up to 5 sounds supported per weapon sound\n");
		}

		if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
		{
			return CG_RW_ParseError(handle, "expected soundSurface filename");
		}

		for (i = 0; i < token.intvalue && i < MAX_WEAPON_SOUNDS; i++)
		{
			weaponSounds->sounds[i] = trap_S_RegisterSound(va("%s%i.wav", filename, i + 1), qfalse);
		}

		weaponSounds->count = i;
	}
	else    // assume only one file sound must be register
	{
		weaponSounds->count     = 1;
		weaponSounds->sounds[0] = trap_S_RegisterSound(token.string, qfalse);
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseSoundSurface
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @return
 */
static qboolean CG_RW_ParseSoundSurface(int handle, weaponSounds_t *weaponSound)
{
	pc_token_t     token;
	soundSurface_t soundSurface;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		for (soundSurface = 0; soundSurface < W_MAX_SND_SURF; soundSurface++)
		{
			if (!Q_stricmp(token.string, soundSurfaceTable[soundSurface].surfaceName))
			{
				break;
			}
		}

		if (soundSurface == W_MAX_SND_SURF)
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}

		if (!CG_RW_ParseWeaponSound(handle, &weaponSound[soundSurface]))
		{
			return qfalse;
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseImpactMark
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @return
 */
static qboolean CG_RW_ParseImpactMark(int handle, weaponInfo_t *weaponInfo)
{
	pc_token_t     token;
	char           filename[MAX_QPATH];
	soundSurface_t impactSurface;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	filename[0] = '\0';

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		for (impactSurface = 0; impactSurface < W_MAX_SND_SURF; impactSurface++)
		{
			if (!Q_stricmp(token.string, soundSurfaceTable[impactSurface].surfaceName))
			{
				if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
				{
					return CG_RW_ParseError(handle, "expected impactMark filename");
				}

				break;
			}
		}

		if (impactSurface == W_MAX_SND_SURF)
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}

		weaponInfo->impactMark[impactSurface] = trap_R_RegisterShaderNoMip(filename);
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseImpactMark
 * @param[in] handle
 * @param[out] impactParticle
 * @return
 */
static qboolean CG_RW_ParseParticleEffect(int handle, impactParticle_t *impactParticle)
{
	int                    index;
	char                   filename[MAX_QPATH];
	char                   surfaceType[8] = { 0 };
	pc_token_t             token;
	soundSurface_t         impactSurface;
	impactParticleEffect_t *weaponParticleEffect = NULL;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "particleEffectType"))
	{
		return CG_RW_ParseError(handle, "expected particleEffectType");
	}

	if (!PC_String_ParseNoAlloc(handle, surfaceType, sizeof(surfaceType)))
	{
		return CG_RW_ParseError(handle, "expected particleEffectType");
	}

	for (impactSurface = 0; impactSurface < W_MAX_SND_SURF; impactSurface++)
	{
		if (!Q_stricmp(surfaceType, soundSurfaceTable[impactSurface].surfaceName))
		{
			break;
		}
	}

	if (impactSurface == W_MAX_SND_SURF)
	{
		return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
	}

	for (index = 0; index < MAX_IMPACT_PARTICLE_EFFECT; index++)
	{
		if (!impactParticle->particleEffect[impactSurface][index].particleEffectUsed)
		{
			impactParticle->particleEffect[impactSurface][index].particleEffectUsed = qtrue;
			weaponParticleEffect                                                    = &impactParticle->particleEffect[impactSurface][index];
			break;
		}
	}

	if (index == MAX_IMPACT_PARTICLE_EFFECT /*|| !weaponParticleEffect*/)
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: only up to %i particle effect per surface\n", MAX_IMPACT_PARTICLE_EFFECT);
		return qfalse;
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "particleEffectShader"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected particleEffectShader filename");
			}

			weaponParticleEffect->particleEffectShader = trap_R_RegisterShader(filename);
		}
		else if (!Q_stricmp(token.string, "particleEffectSpeed"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectSpeed))
			{
				return CG_RW_ParseError(handle, "expected particleEffectSpeed");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectSpeedRand"))
		{
			if (!PC_Float_Parse(handle, &weaponParticleEffect->particleEffectSpeedRand))
			{
				return CG_RW_ParseError(handle, "expected particleEffectSpeedRand");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectDuration"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectDuration))
			{
				return CG_RW_ParseError(handle, "expected particleEffectDuration");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectCount"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectCount))
			{
				return CG_RW_ParseError(handle, "expected particleEffectCount");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectRandScale"))
		{
			if (!PC_Float_Parse(handle, &weaponParticleEffect->particleEffectRandScale))
			{
				return CG_RW_ParseError(handle, "expected particleEffectRandScale");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectWidth"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectWidth))
			{
				return CG_RW_ParseError(handle, "expected particleEffectWidth");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectHeight"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectHeight))
			{
				return CG_RW_ParseError(handle, "expected particleEffectHeight");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectAlpha"))
		{
			if (!PC_Float_Parse(handle, &weaponParticleEffect->particleEffectAlpha))
			{
				return CG_RW_ParseError(handle, "expected particleEffectAlpha");
			}
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseExtraEffect
 * @param[in] handle
 * @param[out] impactParticle
 * @return
 */
static qboolean CG_RW_ParseExtraEffect(int handle, impactParticle_t *impactParticle)
{
	int                 index;
	pc_token_t          token;
	impactExtraEffect_t *impactExtraEffect = NULL;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	for (index = 0; index < MAX_IMPACT_PARTICLE_EFFECT; index++)
	{
		if (!impactParticle->extraEffect[index].extraEffectUsed)
		{
			impactParticle->extraEffect[index].extraEffectUsed = qtrue;
			impactExtraEffect                                  = &impactParticle->extraEffect[index];
			break;
		}
	}

	if (index == MAX_IMPACT_PARTICLE_EFFECT /*|| !impactExtraEffect*/)
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: only up to %i particle effect per surface\n", MAX_IMPACT_PARTICLE_EFFECT);
		return qfalse;
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "extraEffectCount"))
		{
			if (!PC_Int_Parse(handle, &impactExtraEffect->extraEffectCount))
			{
				return CG_RW_ParseError(handle, "expected extraEffectCount");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectOriginRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectOriginRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectOriginRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectVelocityRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectVelocityRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectVelocityRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectVelocityScaling"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectVelocityScaling))
			{
				return CG_RW_ParseError(handle, "expected extraEffectVelocityScaling");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectShaderName"))
		{
			if (!PC_String_ParseNoAlloc(handle, impactExtraEffect->extraEffectShaderName, sizeof(impactExtraEffect->extraEffectShaderName)))
			{
				return CG_RW_ParseError(handle, "expected particleEffectRandScale");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectDuration"))
		{
			if (!PC_Int_Parse(handle, &impactExtraEffect->extraEffectDuration))
			{
				return CG_RW_ParseError(handle, "expected extraEffectDuration");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectDurationRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectDurationRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectDurationRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectSizeStart"))
		{
			if (!PC_Int_Parse(handle, &impactExtraEffect->extraEffectSizeStart))
			{
				return CG_RW_ParseError(handle, "expected extraEffectSizeStart");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectSizeStartRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectSizeStartRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectSizeStartRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectSizeEnd"))
		{
			if (!PC_Int_Parse(handle, &impactExtraEffect->extraEffectSizeEnd))
			{
				return CG_RW_ParseError(handle, "expected extraEffectSizeEnd");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectSizeEndRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectSizeEndRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectSizeEndRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectLightAnim"))
		{
			impactExtraEffect->extraEffectLightAnim = qtrue;
		}
	}

	return qtrue;
}

static impactParticleTable_t impactParticleTable[MAX_IMPACT_PARTICLE] = { { { 0 }, { 0 } } };

/**
 * @brief CG_ParseWeaponImpactParticle
 * @param[in] filename
 * @param[in] pImpactParticle
 * @return
 */
static qboolean CG_ParseWeaponImpactParticle(const char *filename, impactParticle_t **pImpactParticle)
{
	pc_token_t       token;
	int              handle;
	impactParticle_t *impactParticle = NULL;
	int              i;

	for (i = 0; i < MAX_IMPACT_PARTICLE; i++)
	{
		if (impactParticleTable[i].impactParticleName[0] == 0)
		{
			Q_strncpyz(impactParticleTable[i].impactParticleName, filename, MAX_QPATH);
			*pImpactParticle = impactParticle = &impactParticleTable[i].impactParticle;
			break;
		}
		else if (!Q_stricmp(filename, &impactParticleTable[i].impactParticleName[0]))
		{
			*pImpactParticle = &impactParticleTable[i].impactParticle;
			return qtrue;
		}
	}

	if (i == MAX_IMPACT_PARTICLE)
	{
		CG_Printf(S_COLOR_RED "WARNING: too much impact particle declared. Max is %i\n", MAX_IMPACT_PARTICLE);
		return qfalse;
	}

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "weaponImpactParticleDef"))
	{
		return CG_RW_ParseError(handle, "expected 'weaponImpactParticleDef'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "particleDirectionOffset"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->particleDirectionOffset))
			{
				return CG_RW_ParseError(handle, "expected particleDirectionOffset");
			}
		}
		else if (!Q_stricmp(token.string, "particleDirectionScaling"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->particleDirectionScaling))
			{
				return CG_RW_ParseError(handle, "expected particleDirectionScaling");
			}
		}
		else if (!Q_stricmp(token.string, "waterRippleRadius"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->waterRippleRadius))
			{
				return CG_RW_ParseError(handle, "expected waterRippleRadius");
			}
		}
		else if (!Q_stricmp(token.string, "waterRippleLifeTime"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->waterRippleLifeTime))
			{
				return CG_RW_ParseError(handle, "expected waterRippleLifeTime");
			}
		}
		else if (!Q_stricmp(token.string, "waterSplashDuration"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->waterSplashDuration))
			{
				return CG_RW_ParseError(handle, "expected waterSplashDuration");
			}
		}
		else if (!Q_stricmp(token.string, "waterSplashLight"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->waterSplashLight))
			{
				return CG_RW_ParseError(handle, "expected waterSplashLight");
			}
		}
		else if (!Q_stricmp(token.string, "waterSplashLightColor"))
		{
			if (!PC_Vec_Parse(handle, &impactParticle->waterSplashLightColor))
			{
				return CG_RW_ParseError(handle, "expected waterSplashLightColor");
			}
		}
		else if (!Q_stricmp(token.string, "waterSplashIsSprite"))
		{
			impactParticle->waterSplashIsSprite = qtrue;
		}
		else if (!Q_stricmp(token.string, "explosionShaderName"))
		{
			if (!PC_String_ParseNoAlloc(handle, impactParticle->explosionShaderName, sizeof(impactParticle->explosionShaderName)))
			{
				return CG_RW_ParseError(handle, "expected explosionShaderName");
			}
		}
		else if (!Q_stricmp(token.string, "explosionDuration"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->explosionDuration))
			{
				return CG_RW_ParseError(handle, "expected explosionDuration");
			}
		}
		else if (!Q_stricmp(token.string, "explosionSizeStart"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->explosionSizeStart))
			{
				return CG_RW_ParseError(handle, "expected explosionSizeStart");
			}
		}
		else if (!Q_stricmp(token.string, "explosionSizeStartRand"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->explosionSizeStartRand))
			{
				return CG_RW_ParseError(handle, "expected explosionSizeStartRand");
			}
		}
		else if (!Q_stricmp(token.string, "explosionSizeEnd"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->explosionSizeEnd))
			{
				return CG_RW_ParseError(handle, "expected explosionSizeEnd");
			}
		}
		else if (!Q_stricmp(token.string, "explosionSizeEndRand"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->explosionSizeEndRand))
			{
				return CG_RW_ParseError(handle, "expected explosionSizeEndRand");
			}
		}
		else if (!Q_stricmp(token.string, "explosionLightAnim"))
		{
			impactParticle->explosionLightAnim = qtrue;
		}
		else if (!Q_stricmp(token.string, "debrisSpeed"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->debrisSpeed))
			{
				return CG_RW_ParseError(handle, "expected debrisSpeed");
			}
		}
		else if (!Q_stricmp(token.string, "debrisSpeedRand"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->debrisSpeedRand))
			{
				return CG_RW_ParseError(handle, "expected debrisSpeedRand");
			}
		}
		else if (!Q_stricmp(token.string, "debrisDuration"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->debrisDuration))
			{
				return CG_RW_ParseError(handle, "expected debrisDuration");
			}
		}
		else if (!Q_stricmp(token.string, "debrisDurationRand"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->debrisDurationRand))
			{
				return CG_RW_ParseError(handle, "expected debrisDurationRand");
			}
		}
		else if (!Q_stricmp(token.string, "debrisCount"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->debrisCount))
			{
				return CG_RW_ParseError(handle, "expected debrisCount");
			}
		}
		else if (!Q_stricmp(token.string, "debrisCountExtra"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->debrisCountExtra))
			{
				return CG_RW_ParseError(handle, "expected debrisCountExtra");
			}
		}
		else if (!Q_stricmp(token.string, "debrisForBullet"))
		{
			impactParticle->debrisForBullet = qtrue;
		}
		else if (!Q_stricmp(token.string, "particleEffect"))
		{
			if (!CG_RW_ParseParticleEffect(handle, impactParticle))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "extraEffect"))
		{
			if (!CG_RW_ParseExtraEffect(handle, impactParticle))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

/**
 * @brief CG_RW_ParseClient
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @return
 */
static qboolean CG_RW_ParseClient(int handle, weaponInfo_t *weaponInfo)
{
	pc_token_t token;
	char       filename[MAX_QPATH];

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "standModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected standModel filename");
			}

			weaponInfo->standModel = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "droppedAnglesHack"))
		{
			weaponInfo->droppedAnglesHack = qtrue;
		}
		else if (!Q_stricmp(token.string, "pickupModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected pickupModel filename");
			}

			weaponInfo->weaponModel[W_PU_MODEL].model = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "pickupSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected pickupSound filename");
			}

			//weaponInfo->pickupSound = trap_S_RegisterSound( filename, qfalse );
		}
		else if (!Q_stricmp(token.string, "weaponConfig"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponConfig filename");
			}

			if (!CG_ParseWeaponConfig(filename, weaponInfo))
			{
				CG_Error("Couldn't register weapon (failed to parse %s)", filename);
			}
		}
		else if (!Q_stricmp(token.string, "handsModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected handsModel filename");
			}

			weaponInfo->handsModel = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "flashDlightColor"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->flashDlightColor))
			{
				return CG_RW_ParseError(handle, "expected flashDlightColor as r g b");
			}
		}
		else if (!Q_stricmp(token.string, "flashSound"))
		{
			if (!CG_RW_ParseWeaponSound(handle, &weaponInfo->flashSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "flashEchoSound"))
		{
			if (!CG_RW_ParseWeaponSound(handle, &weaponInfo->flashEchoSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "lastShotSound"))
		{
			if (!CG_RW_ParseWeaponSound(handle, &weaponInfo->lastShotSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "readySound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected readySound filename");
			}

			weaponInfo->readySound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "firingSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected firingSound filename");
			}

			weaponInfo->firingSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "overheatSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected overheatSound filename");
			}

			weaponInfo->overheatSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "reloadSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected reloadSound filename");
			}

			weaponInfo->reloadSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "reloadFastSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected reloadFastSound filename");
			}

			weaponInfo->reloadFastSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "spinupSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected spinupSound filename");
			}

			weaponInfo->spinupSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "spindownSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected spindownSound filename");
			}

			weaponInfo->spindownSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "switchSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected switchSound filename");
			}

			weaponInfo->switchSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "noAmmoSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected noAmmoSound filename");
			}

			weaponInfo->noAmmoSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "weaponIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponIcon filename");
			}

			weaponInfo->weaponIcon[0] = trap_R_RegisterShaderNoMip(filename);
		}
		else if (!Q_stricmp(token.string, "weaponIconScale"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->weaponIconScale))
			{
				return CG_RW_ParseError(handle, "expected weaponIconScale filename");
			}
		}
		else if (!Q_stricmp(token.string, "weaponSelectedIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponSelectedIcon filename");
			}

			weaponInfo->weaponIcon[1] = trap_R_RegisterShaderNoMip(filename);
		}
		else if (!Q_stricmp(token.string, "weaponSimpleIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponSimpleIcon shadername");
			}

			weaponInfo->weaponSimpleIcon = trap_R_RegisterShaderNoMip(filename);
		}
		else if (!Q_stricmp(token.string, "weaponSimpleIconScale"))
		{
			if (!PC_Point_Parse(handle, &weaponInfo->weaponSimpleIconScale))
			{
				return CG_RW_ParseError(handle, "expected weaponSimpleIconScale X Y");
			}
		}
		else if (!Q_stricmp(token.string, "weaponCardIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponCardIcon filename");
			}

			weaponInfo->weaponCardIcon = trap_R_RegisterShaderNoMip(filename);
		}
		else if (!Q_stricmp(token.string, "weaponCardScale"))
		{
			if (!PC_Point_Parse(handle, &weaponInfo->weaponCardScale))
			{
				return CG_RW_ParseError(handle, "expected weaponCardScale as width height");
			}
		}
		else if (!Q_stricmp(token.string, "weaponCardPointS"))
		{
			if (!PC_Point_Parse(handle, &weaponInfo->weaponCardPointS))
			{
				return CG_RW_ParseError(handle, "expected weaponCardScale as S0 S1");
			}
		}
		else if (!Q_stricmp(token.string, "weaponCardPointT"))
		{
			if (!PC_Point_Parse(handle, &weaponInfo->weaponCardPointT))
			{
				return CG_RW_ParseError(handle, "expected weaponCardScale as T0 T1");
			}
		}
		else if (!Q_stricmp(token.string, "missileModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected missileModel filename");
			}

			weaponInfo->missileModel = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "missileAlliedSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->missileAlliedSkin = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "missileAxisSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->missileAxisSkin = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "missileSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected missileSound filename");
			}

			weaponInfo->missileSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "missileFallSound"))
		{
			if (!CG_RW_ParseWeaponSound(handle, &weaponInfo->missileFallSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "missileBouncingSound"))
		{
			if (!CG_RW_ParseSoundSurface(handle, weaponInfo->missileBouncingSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "missileTrailFunc"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected missileTrailFunc");
			}

			if (!Q_stricmp(filename, "GrenadeTrail"))
			{
				weaponInfo->missileTrailFunc = CG_GrenadeTrail;
			}
			else if (!Q_stricmp(filename, "RocketTrail"))
			{
				weaponInfo->missileTrailFunc = CG_RocketTrail;
			}
			else if (!Q_stricmp(filename, "PyroSmokeTrail"))
			{
				weaponInfo->missileTrailFunc = CG_PyroSmokeTrail;
			}
			else if (!Q_stricmp(filename, "DynamiteTrail"))
			{
				weaponInfo->missileTrailFunc = CG_DynamiteTrail;
			}
		}
		else if (!Q_stricmp(token.string, "missileDlight"))
		{
			if (!PC_Float_Parse(handle, &weaponInfo->missileDlight))
			{
				return CG_RW_ParseError(handle, "expected missileDlight value");
			}
		}
		else if (!Q_stricmp(token.string, "missileDlightColor"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->missileDlightColor))
			{
				return CG_RW_ParseError(handle, "expected missileDlightColor as r g b");
			}
		}
		else if (!Q_stricmp(token.string, "ejectBrassFunc"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected ejectBrassFunc");
			}

			if (!Q_stricmp(filename, "MachineGunEjectBrass"))
			{
				weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
			}
			else if (!Q_stricmp(filename, "PanzerFaustEjectBrass"))
			{
				weaponInfo->ejectBrassFunc = CG_PanzerFaustEjectBrass;
			}
		}
		else if (!Q_stricmp(token.string, "fireRecoil"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->fireRecoil))
			{
				return CG_RW_ParseError(handle, "expected fireRecoil as pitch yaw roll");
			}
		}
		else if (!Q_stricmp(token.string, "adjustLean"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->adjustLean))
			{
				return CG_RW_ParseError(handle, "expected adjustLean as pitch yaw roll");
			}
		}
		else if (!Q_stricmp(token.string, "impactDurationCoeff"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->impactDurationCoeff))
			{
				return CG_RW_ParseError(handle, "expected impactDurationCoeff value");
			}
		}
		else if (!Q_stricmp(token.string, "impactMarkMaxRange"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->impactMarkMaxRange))
			{
				return CG_RW_ParseError(handle, "expected impactMarkMaxRange value");
			}
		}
		else if (!Q_stricmp(token.string, "impactSoundRange"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->impactSoundRange))
			{
				return CG_RW_ParseError(handle, "expected impactSoundRange value");
			}
		}
		else if (!Q_stricmp(token.string, "impactSoundVolume"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->impactSoundVolume))
			{
				return CG_RW_ParseError(handle, "expected impactSoundVolume value");
			}
		}
		else if (!Q_stricmp(token.string, "impactMarkRadius"))
		{
			if (!PC_Float_Parse(handle, &weaponInfo->impactMarkRadius))
			{
				return CG_RW_ParseError(handle, "expected impactMarkRadius value");
			}
		}
		else if (!Q_stricmp(token.string, "impactParticle"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected impactParticle filename");
			}

			if (!CG_ParseWeaponImpactParticle(filename, &weaponInfo->impactParticle))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "impactSound"))
		{
			if (!CG_RW_ParseSoundSurface(handle, weaponInfo->impactSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "impactMark"))
		{
			if (!CG_RW_ParseImpactMark(handle, weaponInfo))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "modModel"))
		{
			if (!CG_RW_ParseModModel(handle, weaponInfo))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "firstPerson"))
		{
			if (!CG_RW_ParseViewType(handle, weaponInfo, W_FP_MODEL))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "thirdPerson"))
		{
			if (!CG_RW_ParseViewType(handle, weaponInfo, W_TP_MODEL))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

/**
 * @brief CG_RegisterWeaponFromWeaponFile
 * @param[in] filename
 * @param[in] weaponInfo
 * @return
 */
static qboolean CG_RegisterWeaponFromWeaponFile(const char *filename, weaponInfo_t *weaponInfo)
{
	pc_token_t token;
	int        handle;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "weaponDef"))
	{
		return CG_RW_ParseError(handle, "expected 'weaponDef'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "client"))
		{
			if (!CG_RW_ParseClient(handle, weaponInfo))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

/**
 * @brief CG_RegisterWeapon
 * @param[in] weaponNum
 * @param[in] force
 */
void CG_RegisterWeapon(int weaponNum, qboolean force)
{
	weaponInfo_t *weaponInfo;

	if (!IS_VALID_WEAPON(weaponNum))
	{
		return;
	}

	weaponInfo = &cg_weapons[weaponNum];

	if (weaponInfo->registered && !force)
	{
		return;
	}

	Com_Memset(weaponInfo, 0, sizeof(*weaponInfo));
	weaponInfo->registered = qtrue;

	/*for( item = bg_itemlist + 1 ; item->classname ; item++ ) {
	    if( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
	        weaponInfo->item = item;
	        break;
	    }
	}

	if( !item->classname ) {
	    CG_Error( "Couldn't find weapon %i", weaponNum );
	}*/

	if (GetWeaponTableData(weaponNum)->weapFile)
	{
		if (!CG_RegisterWeaponFromWeaponFile(va("weapons/%s.weap", GetWeaponTableData(weaponNum)->weapFile), weaponInfo))
		{
			CG_Printf(S_COLOR_RED "WARNING: failed to register media for weapon %i from %s.weap\n", weaponNum, GetWeaponTableData(weaponNum)->weapFile);
		}
	}
	else
	{
		// no weapon file for theses weapons
		if (weaponNum == VERYBIGEXPLOSION || weaponNum == WP_DUMMY_MG42)
		{
			//CG_Printf(S_COLOR_YELLOW "WARNING: skipping weapon %i to register.\n", weaponNum);
			weaponInfo->weaponIconScale = 1;
		}
		else
		{
			CG_Printf(S_COLOR_RED "WARNING: trying to register weapon %i but there is no weapon file entry for it.\n", weaponNum);
		}
	}
}

/**
 * @brief Registers models and icons for items of bg_itemlist (except weapons which are registered from *.weap file data)
 *
 * @param[in] itemNum
 *
 * @note Actually IT_AMMO & IT_TEAM have no visuals
 */
void CG_RegisterItemVisuals(int itemNum)
{
	itemInfo_t *itemInfo = &BG_GetItem(itemNum)->itemInfo;
	gitem_t    *item;
	int        i;

	if (itemInfo->registered)
	{
		return;
	}

	item = BG_GetItem(itemNum);

	if (item->giType == IT_WEAPON)
	{
		return;
	}

	Com_Memset(itemInfo, 0, sizeof(*itemInfo));

	for (i = 0; i < MAX_ITEM_MODELS; i++)
	{
		// some items don't have world models see (bg_itemlist) - we don't have to register
		if (!item->world_model[i] || !item->world_model[i][0])
		{
			CG_DPrintf("CG_RegisterItemVisuals: NULL or empty world_model[%i] for item classname %s\n", i, item->classname);
			itemInfo->models[i] = 0;
		}
		else
		{
			itemInfo->models[i] = trap_R_RegisterModel(item->world_model[i]);
		}
	}

	// some items have no icon shader - we don't have to register
	if (item->icon && item->icon[0])
	{
		itemInfo->icons[0] = trap_R_RegisterShader(item->icon);
	}
	else
	{
		CG_DPrintf("CG_RegisterItemVisuals: NULL or empty item icon shader [%s] for classname %s\n", item->icon, item->classname);
		itemInfo->icons[0] = 0;
	}

	itemInfo->registered = qtrue;   // moved this down after the registerweapon()
}
