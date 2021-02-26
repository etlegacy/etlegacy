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
 * @file cg_character.c
 * @brief Character loading
 */

#include "cg_local.h"

char bigTextBuffer[100000];

/**
 * @brief Parse gib models
 * @details Read a configuration file containing gib models for use with this character.
 * @param[in] modelPath
 * @param[out] character
 * @return
 */
static qboolean CG_ParseGibModels(const char *modelPath, bg_character_t *character)
{
	char         *text_p;
	int          len;
	int          i;
	char         *token;
	fileHandle_t f;

	Com_Memset(character->gibModels, 0, sizeof(character->gibModels));

	// load the file
	len = trap_FS_FOpenFile(va("%s.gibs", modelPath), &f, FS_READ);
	if (len <= 0)
	{
		CG_Printf("File %s.gibs not found\n", modelPath);
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	if (len >= (int)sizeof(bigTextBuffer) - 1)
	{
		CG_Printf("File %s.gibs too long\n", modelPath);
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	//CG_Printf("CG_ParseGibModel reading %s.gibs\n", modelPath);

	trap_FS_Read(bigTextBuffer, len, f);
	bigTextBuffer[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = bigTextBuffer;

	COM_BeginParseSession("CG_ParseGibModels");

	for (i = 0; i < MAX_GIB_MODELS; i++)
	{
		token = COM_Parse(&text_p);

		if (!token[0])
		{
			break;
		}

		// cache this model
		character->gibModels[i] = trap_R_RegisterModel(token);

		if (!character->gibModels[i])
		{
			CG_Printf("CG_ParseGibModels gibModel[%i] %s not registered from %s.gibs\n", i, token, modelPath);
		}
	}

	return qtrue;
}

/**

 */
/**
 * @brief Parse HUD head config
 * @details Parse HUD head config
 * @param[in] filename
 * @param[out] hha
 * @return
 */
static qboolean CG_ParseHudHeadConfig(const char *filename, animation_t *hha)
{
	char         *text_p;
	int          len;
	int          i;
	float        fps;
	char         *token;
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len <= 0)
	{
		return qfalse;
	}

	if (len >= (int)sizeof(bigTextBuffer) - 1)
	{
		CG_Printf("File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	trap_FS_Read(bigTextBuffer, len, f);
	bigTextBuffer[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = bigTextBuffer;

	COM_BeginParseSession("CG_ParseHudHeadConfig");

	for (i = 0 ; i < MAX_HD_ANIMATIONS ; i++)
	{
		token = COM_Parse(&text_p);     // first frame

		if (!token[0])
		{
			break;
		}

		hha[i].firstFrame = Q_atoi(token);

		token = COM_Parse(&text_p);     // length

		if (!token[0])
		{
			break;
		}

		hha[i].numFrames = Q_atoi(token);

		token = COM_Parse(&text_p);     // fps

		if (!token[0])
		{
			break;
		}

		fps = (float)atof(token);

		if (fps == 0.f)
		{
			fps = 1;
		}

		hha[i].frameLerp   = (int)(1000 / fps);
		hha[i].initialLerp = (int)(1000 / fps);

		token = COM_Parse(&text_p);     // looping frames

		if (!token[0])
		{
			break;
		}

		hha[i].loopFrames = Q_atoi(token);

		if (hha[i].loopFrames > hha[i].numFrames)
		{
			hha[i].loopFrames = hha[i].numFrames;
		}
		else if (hha[i].loopFrames < 0)
		{
			hha[i].loopFrames = 0;
		}
	}

	if (i != MAX_HD_ANIMATIONS)
	{
		CG_Printf("Error parsing hud head animation file: %s", filename);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Calculate move speeds
 * @details Calculate movement speeds.
 * @param[in] character
 */
static void CG_CalcMoveSpeeds(bg_character_t *character)
{
	const char    *tags[2]  = { "tag_footleft", "tag_footright" };
	vec3_t        oldPos[2] = { { 0, 0 } };
	refEntity_t   refent;
	animation_t   *anim;
	int           i, j, k;
	float         totalSpeed;
	int           numSpeed;
	int           low;
	orientation_t o[2];

	Com_Memset(&refent, 0, sizeof(refent));

	refent.hModel = character->mesh;

	for (i = 0; i < character->animModelInfo->numAnimations; i++)
	{
		anim = character->animModelInfo->animations[i];

		if (anim->moveSpeed >= 0)
		{
			continue;
		}

		totalSpeed = 0;
		numSpeed   = 0;

		// for each frame
		for (j = 0; j < anim->numFrames; j++)
		{
			refent.frame           = anim->firstFrame + j;
			refent.oldframe        = refent.frame;
			refent.torsoFrameModel = refent.oldTorsoFrameModel = refent.frameModel = refent.oldframeModel = anim->mdxFile;

			// for each foot
			for (k = 0; k < 2; k++)
			{
				if (trap_R_LerpTag(&o[k], &refent, tags[k], 0) < 0)
				{
					CG_Error("CG_CalcMoveSpeeds: unable to find tag %s, cannot calculate movespeed\n", tags[k]);
				}
			}

			// find the contact foot
			if (anim->flags & ANIMFL_LADDERANIM)
			{
				if (o[0].origin[0] > o[1].origin[0])
				{
					low = 0;
				}
				else
				{
					low = 1;
				}
				totalSpeed += Q_fabs(oldPos[low][2] - o[low].origin[2]);
			}
			else
			{
				if (o[0].origin[2] < o[1].origin[2])
				{
					low = 0;
				}
				else
				{
					low = 1;
				}
				totalSpeed += Q_fabs(oldPos[low][0] - o[low].origin[0]);
			}

			numSpeed++;

			// save the positions
			for (k = 0; k < 2; k++)
			{
				VectorCopy(o[k].origin, oldPos[k]);
			}
		}

		// record the speed
		anim->moveSpeed = round(((totalSpeed / numSpeed) * 1000.0f / anim->frameLerp));
	}
}

/**
 * @brief Parse animation files
 * @details Read in all the configuration and script files for this model.
 * @param[in] character
 * @param[in] animationGroup
 * @param[in] animationScript
 * @return
 */
static qboolean CG_ParseAnimationFiles(bg_character_t *character, const char *animationGroup, const char *animationScript)
{
	fileHandle_t f;
	int          len;

	// set the name of the animationGroup and animationScript in the animModelInfo structure
	Q_strncpyz(character->animModelInfo->animationGroup, animationGroup, sizeof(character->animModelInfo->animationGroup));
	Q_strncpyz(character->animModelInfo->animationScript, animationScript, sizeof(character->animModelInfo->animationScript));

	BG_R_RegisterAnimationGroup(animationGroup, character->animModelInfo);

	// calc movespeed values if required
	CG_CalcMoveSpeeds(character);

	// load the script file
	len = trap_FS_FOpenFile(animationScript, &f, FS_READ);

	if (len <= 0)
	{
		return qfalse;
	}

	if (len >= (int)sizeof(bigTextBuffer) - 1)
	{
		CG_Printf("File %s is too long\n", animationScript);
		trap_FS_FCloseFile(f);

		return qfalse;
	}

	trap_FS_Read(bigTextBuffer, len, f);
	bigTextBuffer[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	BG_AnimParseAnimScript(character->animModelInfo, &cgs.animScriptData, animationScript, bigTextBuffer);

	return qtrue;
}

/**
 * @brief Check for existing animation model information
 * @details If this player model has already been parsed, then use the existing information.
 *          Otherwise, set the modelInfo pointer to the first free slot.
 * @param[in] animationGroup
 * @param[in] animationScript
 * @param[in] animModelInfo
 * @return
 */
static qboolean CG_CheckForExistingAnimModelInfo(const char *animationGroup, const char *animationScript, animModelInfo_t **animModelInfo)
{
	int             i;
	animModelInfo_t *trav, *firstFree = NULL;

	for (i = 0, trav = cgs.animScriptData.modelInfo; i < MAX_ANIMSCRIPT_MODELS; i++, trav++)
	{
		if (*trav->animationGroup && *trav->animationScript)
		{
			if (!Q_stricmp(trav->animationGroup, animationGroup) && !Q_stricmp(trav->animationScript, animationScript))
			{
				// found a match, use this animModelInfo
				*animModelInfo = trav;
				return qtrue;
			}
		}
		else if (!firstFree)
		{
			firstFree = trav;
		}
	}

	if (!firstFree)
	{
		CG_Error("unable to find a free modelinfo slot, cannot continue\n");
	}
	else
	{
		*animModelInfo = firstFree;
		// clear the structure out ready for use
		Com_Memset(*animModelInfo, 0, sizeof(**animModelInfo));
	}

	// we need to parse the information from the script files
	return qfalse;
}

/**
 * @brief Register accessories
 * @details Register model accessories.
 * @param[in] modelName
 * @param[in,out] model
 * @param[in] skinname
 * @param[out] skin
 * @return
 */
static qboolean CG_RegisterAcc(const char *modelName, int *model, const char *skinname, qhandle_t *skin)
{
	char filename[MAX_QPATH];

	*model = trap_R_RegisterModel(modelName);

	if (!*model)
	{
		return qfalse;
	}

	COM_StripExtension(modelName, filename, sizeof(filename));
	Q_strcat(filename, sizeof(filename), va("_%s.skin", skinname));
	*skin = trap_R_RegisterSkin(filename);

	return qtrue;
}

typedef struct
{
	const char *type;
	accType_t index;
} acc_t;

static acc_t cg_accessories[] =
{
	{ "md3_beltr",   ACC_BELT_LEFT  },
	{ "md3_beltl",   ACC_BELT_RIGHT },
	{ "md3_belt",    ACC_BELT       },
	{ "md3_back",    ACC_BACK       },
	{ "md3_weapon",  ACC_WEAPON     },
	{ "md3_weapon2", ACC_WEAPON2    },
};

static int cg_numAccessories = sizeof(cg_accessories) / sizeof(cg_accessories[0]);

static acc_t cg_headAccessories[] =
{
	{ "md3_hat",  ACC_HAT    },
	{ "md3_rank", ACC_RANK   },
	{ "md3_hat2", ACC_MOUTH2 },
	{ "md3_hat3", ACC_MOUTH3 },
};

static int cg_numHeadAccessories = sizeof(cg_headAccessories) / sizeof(cg_headAccessories[0]);

/**
 * @brief Register character
 * @details Register character.
 * @param[in] characterFile
 * @param[in] character
 * @return
 */
qboolean CG_RegisterCharacter(const char *characterFile, bg_character_t *character)
{
	bg_characterDef_t characterDef;
	char              *filename;
	char              buf[MAX_QPATH];

	Com_Memset(&characterDef, 0, sizeof(characterDef));

	if (!BG_ParseCharacterFile(characterFile, &characterDef))
	{
		return qfalse;  // the parser will provide the error message
	}

	// register mesh
	if (!(character->mesh = trap_R_RegisterModel(characterDef.mesh)))
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: failed to register mesh '%s' referenced from '%s'\n", characterDef.mesh, characterFile);
	}

	// register skin
	COM_StripExtension(characterDef.mesh, buf, sizeof(buf));
	filename = va("%s_%s.skin", buf, characterDef.skin);

	if (!(character->skin = trap_R_RegisterSkin(filename)))
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: failed to register skin '%s' referenced from '%s'\n", filename, characterFile);
	}
	else
	{
		char accessoryname[MAX_QPATH];
		int  i;

		for (i = 0; i < cg_numAccessories; i++)
		{
			if (trap_R_GetSkinModel(character->skin, cg_accessories[i].type, accessoryname))
			{
				if (!CG_RegisterAcc(accessoryname, &character->accModels[cg_accessories[i].index], characterDef.skin, &character->accSkins[cg_accessories[i].index]))
				{
					CG_Printf(S_COLOR_YELLOW "WARNING: failed to register accessory '%s' referenced from '%s'->'%s'\n", accessoryname, characterFile, filename);
				}
			}
		}

		for (i = 0; i < cg_numHeadAccessories; i++)
		{
			if (trap_R_GetSkinModel(character->skin, cg_headAccessories[i].type, accessoryname))
			{
				if (!CG_RegisterAcc(accessoryname, &character->accModels[cg_headAccessories[i].index], characterDef.skin, &character->accSkins[cg_headAccessories[i].index]))
				{
					CG_Printf(S_COLOR_YELLOW "WARNING: failed to register hud accessory '%s' referenced from '%s'->'%s'\n", accessoryname, characterFile, filename);
				}
			}
		}
	}

	// gib models
	COM_StripExtension(characterDef.mesh, buf, sizeof(buf));
	CG_ParseGibModels(buf, character); // it is here so we can use modelpath from above

	// register undressed corpse media
	if (*characterDef.undressedCorpseModel)
	{
		// register undressed corpse model
		if (!(character->undressedCorpseModel = trap_R_RegisterModel(characterDef.undressedCorpseModel)))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to register undressed corpse model '%s' referenced from '%s'\n", characterDef.undressedCorpseModel, characterFile);
		}

		// register undressed corpse skin
		COM_StripExtension(characterDef.undressedCorpseModel, buf, sizeof(buf));
		filename = va("%s_%s.skin", buf, characterDef.undressedCorpseSkin);

		if (!(character->undressedCorpseSkin = trap_R_RegisterSkin(filename)))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to register undressed corpse skin '%s' referenced from '%s'\n", filename, characterFile);
		}
	}
	else
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: no undressed coprse model definition in '%s'\n", characterFile);
	}

	// register the head for the hud
	if (*characterDef.hudhead)
	{
		// register hud head model
		if (!(character->hudhead = trap_R_RegisterModel(characterDef.hudhead)))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to register hud head model '%s' referenced from '%s'\n", characterDef.hudhead, characterFile);
		}

		if (*characterDef.hudheadskin && !(character->hudheadskin = trap_R_RegisterSkin(characterDef.hudheadskin)))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to register hud head skin '%s' referenced from '%s'\n", characterDef.hudheadskin, characterFile);
		}

		if (!CG_ParseHudHeadConfig(characterDef.hudheadanims, character->hudheadanimations))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to register hud head animations '%s' referenced from '%s'\n", characterDef.hudheadanims, characterFile);
		}
	}
	else
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: no hud head character definition in '%s'\n", characterFile);
	}

	// parse animation files
	if (!CG_CheckForExistingAnimModelInfo(characterDef.animationGroup, characterDef.animationScript, &character->animModelInfo))
	{
		if (!CG_ParseAnimationFiles(character, characterDef.animationGroup, characterDef.animationScript))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to load animation files referenced from '%s'\n", characterFile);
			return qfalse;
		}
	}

	return qtrue;
}

/**
 * @brief Character for clientinfo
 * @details Define character for clientinfo.
 * @param[in] ci
 * @param[in] cent
 * @return
 */
bg_character_t *CG_CharacterForClientinfo(clientInfo_t *ci, centity_t *cent)
{
	if (cent && cent->currentState.eType == ET_CORPSE)
	{
		if (cent->currentState.onFireStart >= 0)
		{
			return cgs.gameCharacters[cent->currentState.onFireStart];
		}
		else
		{
			if (cent->currentState.modelindex < 4)
			{
				return BG_GetCharacter(cent->currentState.modelindex, cent->currentState.modelindex2);
			}
			else
			{
				return BG_GetCharacter(cent->currentState.modelindex - 4, cent->currentState.modelindex2);
			}
		}
	}

	if (cent && (cent->currentState.powerups & (1 << PW_OPS_DISGUISED)))
	{
		int team = ci->team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
		int cls  = (cent->currentState.powerups >> PW_OPS_CLASS_1) & 7;

		return BG_GetCharacter(team, cls);
	}

	if (ci->character)
	{
		return ci->character;
	}

	return BG_GetCharacter(ci->team, ci->cls);
}

/**
 * @brief Character for playerstate
 * @details Define character for playerstate.
 * @param[in] ps
 * @return
 */
bg_character_t *CG_CharacterForPlayerstate(playerState_t *ps)
{
	if (ps->powerups[PW_OPS_DISGUISED])
	{
		int team = cgs.clientinfo[ps->clientNum].team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
		int cls  = 0;

		if (ps->powerups[PW_OPS_CLASS_1])
		{
			cls |= 1;
		}
		if (ps->powerups[PW_OPS_CLASS_2])
		{
			cls |= 2;
		}
		if (ps->powerups[PW_OPS_CLASS_3])
		{
			cls |= 4;
		}

		return BG_GetCharacter(team, cls);
	}

	return BG_GetCharacter(cgs.clientinfo[ps->clientNum].team, cgs.clientinfo[ps->clientNum].cls);
}

/**
 * @brief Register player classes
 * @details Register player classes.
 */
void CG_RegisterPlayerClasses(void)
{
	bg_playerclass_t *classInfo;
	bg_character_t   *character;
	int              team, cls;

	for (team = TEAM_AXIS; team <= TEAM_ALLIES; team++)
	{
		for (cls = PC_SOLDIER; cls < NUM_PLAYER_CLASSES; cls++)
		{
			classInfo = BG_GetPlayerClassInfo(team, cls);
			character = BG_GetCharacter(team, cls);

			Q_strncpyz(character->characterFile, classInfo->characterFile, sizeof(character->characterFile));

			if (!CG_RegisterCharacter(character->characterFile, character))
			{
				CG_Error("ERROR: CG_RegisterPlayerClasses: failed to load character file '%s' for the %s %s\n", character->characterFile, (team == TEAM_AXIS ? "Axis" : "Allied"), BG_ClassnameForNumber(classInfo->classNum));
			}

			if (!(classInfo->icon = trap_R_RegisterShaderNoMip(classInfo->iconName)))
			{
				CG_Printf(S_COLOR_YELLOW "WARNING: failed to load class icon '%s' for the %s %s\n", classInfo->iconName, (team == TEAM_AXIS ? "Axis" : "Allied"), BG_ClassnameForNumber(classInfo->classNum));
			}

			if (!(classInfo->arrow = trap_R_RegisterShaderNoMip(classInfo->iconArrow)))
			{
				CG_Printf(S_COLOR_YELLOW "WARNING: failed to load icon arrow '%s' for the %s %s\n", classInfo->iconArrow, (team == TEAM_AXIS ? "Axis" : "Allied"), BG_ClassnameForNumber(classInfo->classNum));
			}
		}
	}
}
