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
 * @file g_character.c
 * @brief Character loading
 */

#include "g_local.h"
#ifdef FEATURE_SERVERMDX
#include "g_mdx.h"
#endif

#define MAX_TEXT_CHAR 100000
static char text[MAX_TEXT_CHAR];           // <- was causing callstacks >64k

#ifdef FEATURE_SERVERMDX

/**
 * @brief G_CalcMoveSpeeds
 * @param[in,out] character
 *
 * @see Adapted from BG_CalcMoveSpeeds
 */
static void G_CalcMoveSpeeds(bg_character_t *character)
{
	char          *tags[2]  = { "tag_footleft", "tag_footright" };
	vec3_t        oldPos[2] = { { 0, 0, 0 }, { 0, 0, 0 } };
	grefEntity_t  refent;
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
					G_Error("G_CalcMoveSpeeds: unable to find tag %s, cannot calculate movespeed", tags[k]);
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
		anim->moveSpeed = round((totalSpeed / numSpeed) * 1000.0 / anim->frameLerp);
	}
}
#endif

/**
 * @brief Read in all the configuration and script files for this model.
 * @param[in] character
 * @param[in] animationGroup
 * @param[in] animationScript
 * @return
 */
static qboolean G_ParseAnimationFiles(bg_character_t *character, const char *animationGroup, const char *animationScript)
{
	fileHandle_t f;
	int          len;

	// set the name of the animationGroup and animationScript in the animModelInfo structure
	Q_strncpyz(character->animModelInfo->animationGroup, animationGroup, sizeof(character->animModelInfo->animationGroup));
	Q_strncpyz(character->animModelInfo->animationScript, animationScript, sizeof(character->animModelInfo->animationScript));

	BG_R_RegisterAnimationGroup(animationGroup, character->animModelInfo);

#ifdef FEATURE_SERVERMDX
	// calc movespeed values if required
	G_CalcMoveSpeeds(character);
#endif

	// load the script file
	len = trap_FS_FOpenFile(animationScript, &f, FS_READ);
	if (len <= 0)
	{
		return qfalse;
	}
	if (len >= MAX_TEXT_CHAR - 1)
	{
		G_Printf("File %s is too long\n", animationScript);
		trap_FS_FCloseFile(f);

		return qfalse;
	}
	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	BG_AnimParseAnimScript(character->animModelInfo, &level.animScriptData, animationScript, text);

	return qtrue;
}

/**
 * @brief If this player model has already been parsed, then use the existing information.
 * Otherwise, set the modelInfo pointer to the first free slot.
 *
 * @param[in] animationGroup
 * @param[in] animationScript
 * @param[out] animModelInfo
 * @return qtrue if existing model found, qfalse otherwise
 */
static qboolean G_CheckForExistingAnimModelInfo(const char *animationGroup, const char *animationScript, animModelInfo_t **animModelInfo)
{
	int             i;
	animModelInfo_t *trav, *firstFree = NULL;

	for (i = 0, trav = level.animScriptData.modelInfo; i < MAX_ANIMSCRIPT_MODELS; i++, trav++)
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
		G_Error("unable to find a free modelinfo slot, cannot continue\n");
	}
	else
	{
		*animModelInfo = firstFree;
		// clear the structure out ready for use
		Com_Memset(*animModelInfo, 0, sizeof(animModelInfo_t));
	}
	// qfalse signifies that we need to parse the information from the script files
	return qfalse;
}

/**
 * @brief G_RegisterCharacter
 * @param[in] characterFile
 * @param[in] character
 * @return
 */
qboolean G_RegisterCharacter(const char *characterFile, bg_character_t *character)
{
	bg_characterDef_t characterDef;

	Com_Memset(&characterDef, 0, sizeof(characterDef));

	if (!BG_ParseCharacterFile(characterFile, &characterDef))
	{
		return qfalse;  // the parser will provide the error message
	}

#ifdef FEATURE_SERVERMDX
	// Register mesh
	if (!(character->mesh = trap_R_RegisterModel(characterDef.mesh)))
	{
		G_Printf(S_COLOR_YELLOW "WARNING: failed to register mesh '%s' referenced from '%s'\n", characterDef.mesh, characterFile);
	}
#endif

	// Parse Animation Files
	if (!G_CheckForExistingAnimModelInfo(characterDef.animationGroup, characterDef.animationScript, &character->animModelInfo))
	{
		if (!G_ParseAnimationFiles(character, characterDef.animationGroup, characterDef.animationScript))
		{
			G_Printf(S_COLOR_YELLOW "WARNING: failed to load animation files referenced from '%s'\n", characterFile);
			return qfalse;
		}
#ifdef FEATURE_SERVERMDX
		mdx_LoadHitsFile(characterDef.animationGroup, character->animModelInfo);
#endif
	}
	return qtrue;
}

/**
 * @brief G_RegisterPlayerClasses
 */
void G_RegisterPlayerClasses(void)
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

			if (!G_RegisterCharacter(character->characterFile, character))
			{
				G_Error("ERROR: G_RegisterPlayerClasses: failed to load character file '%s' for the %s %s\n", character->characterFile, (team == TEAM_AXIS ? "Axis" : "Allied"), BG_ClassnameForNumber(classInfo->classNum));
			}
		}
	}
}

/**
 * @brief G_UpdateCharacter
 * @param[in,out] client
 */
void G_UpdateCharacter(gclient_t *client)
{
	char           infostring[MAX_INFO_STRING];
	char           *s;
	int            characterIndex;
	bg_character_t *character;

	trap_GetUserinfo(client->ps.clientNum, infostring, sizeof(infostring));
	s = Info_ValueForKey(infostring, "ch");
	if (*s)
	{
		characterIndex = Q_atoi(s);
		if (characterIndex < 0 || characterIndex >= MAX_CHARACTERS)
		{
			goto set_default_character;
		}

		if (client->pers.characterIndex != characterIndex)
		{
			client->pers.characterIndex = characterIndex;
			trap_GetConfigstring(CS_CHARACTERS + characterIndex, infostring, MAX_INFO_STRING);

			if (!(client->pers.character = BG_FindCharacter(infostring)))
			{
				// not found - create it (this should never happen as we should have everything precached)
				client->pers.character = BG_FindFreeCharacter(infostring);

				if (!client->pers.character)
				{
					goto set_default_character;
				}

				Q_strncpyz(client->pers.character->characterFile, infostring, sizeof(client->pers.character->characterFile));

				if (!G_RegisterCharacter(infostring, client->pers.character))
				{
					G_Printf(S_COLOR_YELLOW "WARNING: G_UpdateCharacter: failed to load character file '%s' for %s\n", infostring,
					         client->pers.netname);

					goto set_default_character;
				}
			}
			// reset anims so client's dont freak out

			// this can only be done if the model really changed - otherwise, the animation may get
			// screwed up if we are in the middle of some animation and we come into this function;
			// plus, also reset the timer so we can properly start the next animation
			client->ps.legsAnim   = 0;
			client->ps.torsoAnim  = 0;
			client->ps.legsTimer  = 0;
			client->ps.torsoTimer = 0;
		}
		return;
	}

set_default_character:
	// set default character
	character = BG_GetCharacter(client->sess.sessionTeam, client->sess.playerType);
	if (client->pers.character != character)
	{
		client->pers.characterIndex = -1;
		client->pers.character      = character;

		client->ps.legsAnim   = 0;
		client->ps.torsoAnim  = 0;
		client->ps.legsTimer  = 0;
		client->ps.torsoTimer = 0;
	}
}
