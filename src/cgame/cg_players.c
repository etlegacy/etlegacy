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
 * @file cg_players.c
 * @brief Handle the media and animation for player entities
 */

#include "cg_local.h"

#define SWING_RIGHT 1.f
#define SWING_LEFT  2.f

extern const char *cg_skillRewards[SK_NUM_SKILLS][NUM_SKILL_LEVELS - 1];

/**
 * @brief CG_EntOnFire
 * @param[in] cent
 * @return
 */
qboolean CG_EntOnFire(centity_t *cent)
{
	// don't display on respawn
	if (cent->currentState.powerups & (1 << PW_INVULNERABLE))
	{
		return qfalse;
	}

	if (cent->currentState.number == cg.snap->ps.clientNum && cent->currentState.eType != ET_CORPSE)
	{
		// the player is always starting out on fire, which is easily seen in cinematics
		//      so make sure onFireStart is not 0
		return  (cg.snap->ps.onFireStart
		         && (cg.snap->ps.onFireStart < cg.time)
		         && ((cg.snap->ps.onFireStart + 2000) > cg.time));
	}
	else
	{
		return  ((cent->currentState.onFireStart < cg.time) &&
		         (cent->currentState.onFireEnd > cg.time));
	}
}

/**
 * @brief CG_IsCrouchingAnim
 * @param[in] animModelInfo
 * @param[in] animNum
 * @return
 */
qboolean CG_IsCrouchingAnim(animModelInfo_t *animModelInfo, int animNum)
{
	animation_t *anim;

	// FIXME: make compatible with new scripting
	animNum &= ~ANIM_TOGGLEBIT;
	anim     = BG_GetAnimationForIndex(animModelInfo, animNum);

	if (anim->movetype & ((1 << ANIM_MT_IDLECR) | (1 << ANIM_MT_WALKCR) | (1 << ANIM_MT_WALKCRBK)))
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_CustomSound
 * @param clientNum - unused
 * @param[in] soundName
 * @return
 */
sfxHandle_t CG_CustomSound(int clientNum, const char *soundName)
{
	if (soundName[0] != '*')
	{
		return trap_S_RegisterSound(soundName, qfalse);
	}

	return 0;
}

/*
=============================================================================
CLIENT INFO
=============================================================================
*/

/**
 * @brief Load it now, taking the disk hits.
 * This will usually be deferred to a safe time
 * @param clientNum
 */
static void CG_LoadClientInfo(int clientNum)
{
	int i;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	for (i = 0 ; i < MAX_GENTITIES ; i++)
	{
		if (cg_entities[i].currentState.clientNum == clientNum && cg_entities[i].currentState.eType == ET_PLAYER)
		{
			CG_ResetPlayerEntity(&cg_entities[i]);
		}
	}
}

/**
 * @brief CG_ParseTeamXPs
 * @param[in] n
 */
void CG_ParseTeamXPs(int n)
{
	int        i, j;
	char       *cs = (char *)CG_ConfigString(CS_AXIS_MAPS_XP + n);
	const char *token;

	for (i = 0; i < MAX_MAPS_PER_CAMPAIGN; i++)
	{
		for (j = 0; j < SK_NUM_SKILLS; j++)
		{
			token = COM_ParseExt(&cs, qfalse);

			if (!token[0])
			{
				return;
			}

			if (n == 0)
			{
				cgs.tdbAxisMapsXP[j][i] = Q_atoi(token);
			}
			else
			{
				cgs.tdbAlliedMapsXP[j][i] = Q_atoi(token);
			}
		}
	}
}

void CG_LimboPanel_SendSetupMsg(qboolean forceteam);

/**
 * @brief CG_NewClientInfo
 * @param[in] clientNum
 */
void CG_NewClientInfo(int clientNum)
{
	clientInfo_t *ci = &cgs.clientinfo[clientNum];
	clientInfo_t newInfo;
	const char   *configstring;
	const char   *v;

	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	if (!*configstring)
	{
		Com_Memset(ci, 0, sizeof(*ci));
		return;     // player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	Com_Memset(&newInfo, 0, sizeof(newInfo));

	// grabbing some older stuff, if it's a new client, tinfo will update within one second anyway, otherwise you get the health thing flashing red
	// NOTE: why are we bothering to do all this setting up of a new clientInfo_t anyway? it was all for deffered clients iirc, which we dont have
	VectorCopy(ci->location, newInfo.location);
	newInfo.health       = ci->health;
	newInfo.fireteamData = ci->fireteamData;
	newInfo.clientNum    = clientNum;
	newInfo.selected     = ci->selected;

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz(newInfo.name, v, sizeof(newInfo.name));
	Q_strncpyz(newInfo.cleanname, v, sizeof(newInfo.cleanname));
	Q_CleanStr(newInfo.cleanname);

	// bot skill
	v                = Info_ValueForKey(configstring, "skill");
	newInfo.botSkill = Q_atoi(v);

	// team
	v            = Info_ValueForKey(configstring, "t");
	newInfo.team = (team_t)(Q_atoi(v));

	// class
	v           = Info_ValueForKey(configstring, "c");
	newInfo.cls = Q_atoi(v);

	// latched class
	v                  = Info_ValueForKey(configstring, "lc");
	newInfo.latchedcls = Q_atoi(v);

	// rank
	v            = Info_ValueForKey(configstring, "r");
	newInfo.rank = Q_atoi(v);

#ifdef FEATURE_PRESTIGE
	// prestige
	v                = Info_ValueForKey(configstring, "p");
	newInfo.prestige = Q_atoi(v);
#endif

	// fireteam
	v                = Info_ValueForKey(configstring, "f");
	newInfo.fireteam = Q_atoi(v);

	v = Info_ValueForKey(configstring, "m");
	if (*v)
	{
		int  i;
		char buf[2];

		buf[1] = '\0';
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			buf[0]            = *v;
			newInfo.medals[i] = Q_atoi(buf);
			v++;
		}
	}

	v = Info_ValueForKey(configstring, "ch");
	if (*v)
	{
		newInfo.character = cgs.gameCharacters[atoi(v)];
	}

	v = Info_ValueForKey(configstring, "s");
	if (*v)
	{
		int i;

		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			char skill[2];

			skill[0] = v[i];
			skill[1] = '\0';

			newInfo.skill[i] = Q_atoi(skill);
		}
	}

	// disguise clientNum
	v                         = Info_ValueForKey(configstring, "dn");
	newInfo.disguiseClientNum = Q_atoi(v);

	// weapon and latchedweapon ( FIXME: make these more secure )
	v              = Info_ValueForKey(configstring, "w");
	newInfo.weapon = Q_atoi(v);

	v                     = Info_ValueForKey(configstring, "lw");
	newInfo.latchedweapon = Q_atoi(v);

	v                       = Info_ValueForKey(configstring, "sw");
	newInfo.secondaryweapon = Q_atoi(v);

	v                              = Info_ValueForKey(configstring, "lsw");
	newInfo.latchedsecondaryweapon = Q_atoi(v);

	v                 = Info_ValueForKey(configstring, "ref");
	newInfo.refStatus = Q_atoi(v);

	v                   = Info_ValueForKey(configstring, "sc");
	newInfo.shoutcaster = Q_atoi(v);

	// Detect rank/skill changes client side.
	// Make sure we have some valid clientinfo, otherwise people are thrown
	// into spectator on map starts.
	if (clientNum == cg.clientNum && cgs.clientinfo[cg.clientNum].team > 0)
	{
		int i;

		if (newInfo.team != cgs.clientinfo[cg.clientNum].team)
		{
			if (cgs.autoFireteamCreateEndTime != cg.time + 20000)
			{
				cgs.autoFireteamCreateEndTime = 0;
			}
			if (cgs.autoFireteamJoinEndTime != cg.time + 20000)
			{
				cgs.autoFireteamJoinEndTime = 0;
			}
		}

#ifdef FEATURE_EDV
		if (newInfo.rank > cgs.clientinfo[cg.clientNum].rank && !cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#else
		if (newInfo.rank > cgs.clientinfo[cg.clientNum].rank)
#endif
		{
			CG_SoundPlaySoundScript(GetRankTableData(cgs.clientinfo[cg.clientNum].team, newInfo.rank)->soundNames, NULL, -1, qtrue);

			if (!(cg_popupBigFilter.integer & POPUP_BIG_FILTER_RANK))
			{
				CG_AddPMItemBig(PM_RANK, va(CG_TranslateString("Promoted to rank %s!"), GetRankTableData(cgs.clientinfo[cg.clientNum].team, newInfo.rank)->names), rankicons[newInfo.rank][cgs.clientinfo[cg.clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
			}
		}

		// Make sure primary class and primary weapons are correct for
		// subsequent calls to CG_LimboPanel_SendSetupMsg
		CG_LimboPanel_Setup();

		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			if (newInfo.skill[i] > cgs.clientinfo[cg.clientNum].skill[i])
			{
				// NOTE: slick hack so that funcs we call use the new value now
				cgs.clientinfo[cg.clientNum].skill[i] = newInfo.skill[i];

				if (newInfo.skill[i] == (NUM_SKILL_LEVELS - 1) && (i == SK_HEAVY_WEAPONS || i == SK_LIGHT_WEAPONS))
				{
					bg_playerclass_t *classinfo;
					classinfo = BG_GetPlayerClassInfo(cgs.clientinfo[cg.clientNum].team, cgs.clientinfo[cg.clientNum].cls);

					// Only select new weapon if using the first weapon (pistol 0)
					if (cgs.ccSelectedSecondaryWeapon == classinfo->classSecondaryWeapons[0].weapon)
					{
						CG_LimboPanel_SetDefaultWeapon(SECONDARY_SLOT);
						CG_LimboPanel_SendSetupMsg(qfalse);
					}
				}

#ifdef FEATURE_EDV
				if (!cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#endif
				{
					if (!(cg_popupBigFilter.integer & POPUP_BIG_FILTER_SKILL))
					{
						CG_AddPMItemBig(PM_SKILL, va(CG_TranslateString("Increased %s skill to level %i!"), CG_TranslateString(GetSkillTableData(i)->skillNames), newInfo.skill[i]), cgs.media.skillPics[i]);
					}

					CG_PriorityCenterPrint(va(CG_TranslateString("You have been rewarded with %s"), CG_TranslateString(cg_skillRewards[i][newInfo.skill[i] - 1])), 400, cg_fontScaleCP.value, 99999);
				}

#ifdef FEATURE_PRESTIGE
				if (cgs.prestige && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS && cgs.gametype != GT_WOLF_CAMPAIGN)
				{
					int j;
					int skillMax = 0, cnt = 0;

					// check skill max level
					for (j = NUM_SKILL_LEVELS - 1; j >= 0; j--)
					{
						if (GetSkillTableData(i)->skillLevels[j] >= 0)
						{
							skillMax = j;
							break;
						}
					}

					if (newInfo.skill[i] == skillMax)
					{
						// count the number of maxed out skills
						for (j = 0; j < SK_NUM_SKILLS; j++)
						{
							int k;
							skillMax = 0;

							// check skill max level
							for (k = NUM_SKILL_LEVELS - 1; k >= 0; k--)
							{
								if (GetSkillTableData(j)->skillLevels[k] >= 0)
								{
									skillMax = k;
									break;
								}
							}

							if (cgs.clientinfo[cg.clientNum].skill[j] >= skillMax)
							{
								cnt++;
							}
						}

						if (!(cg_popupBigFilter.integer & POPUP_BIG_FILTER_PRESTIGE))
						{
							if (cnt < SK_NUM_SKILLS)
							{
								CG_AddPMItemBig(PM_PRESTIGE, va(CG_TranslateString("Prestige point progression: %i/7"), cnt), cgs.media.prestigePics[1]);
							}
							else
							{
								CG_AddPMItemBig(PM_PRESTIGE, CG_TranslateString("Prestige point ready to be collected!"), cgs.media.prestigePics[2]);
							}
						}
					}
				}
#endif
			}
		}

		if (newInfo.team != cgs.clientinfo[cg.clientNum].team)
		{
			// clear these
			Com_Memset(cg.artilleryRequestPos, 0, sizeof(cg.artilleryRequestPos));
			Com_Memset(cg.artilleryRequestTime, 0, sizeof(cg.artilleryRequestTime));
		}

		trap_Cvar_Set("authLevel", va("%i", newInfo.refStatus));

		if (newInfo.refStatus != ci->refStatus)
		{
			if (newInfo.refStatus <= RL_NONE)
			{
				const char *info = CG_ConfigString(CS_SERVERINFO);

				trap_Cvar_Set("cg_ui_voteFlags", Info_ValueForKey(info, "voteFlags"));
				CG_Printf("[cgnotify]^3*** You have been stripped of your referee status! ***\n");
			}
			else
			{
				trap_Cvar_Set("cg_ui_voteFlags", "0");
				CG_Printf("[cgnotify]^2*** You have been authorized \"%s\" status ***\n", ((newInfo.refStatus == RL_RCON) ? "rcon" : "referee"));
				CG_Printf("Type: ^3ref^7 (by itself) for a list of referee commands.\n");
			}
		}

		if (newInfo.shoutcaster != ci->shoutcaster)
		{
			if (newInfo.shoutcaster <= 0)
			{
				CG_Printf("[cgnotify]^3*** You have been stripped of your shoutcaster status! ***\n");
			}
			else
			{
				CG_Printf("[cgnotify]^2*** You have been authorized \"shoutcaster\" status ***\n");
			}
		}
	}

	// passing the clientNum since that's all we need, and we
	// can't calculate it properly from the clientinfo
	CG_LoadClientInfo(clientNum);

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci               = newInfo;

	// make sure we have a character set
	if (!ci->character)
	{
		ci->character = BG_GetCharacter(ci->team, ci->cls);
	}

	// need to resort the fireteam list, in case ranks etc have changed
	CG_SortClientFireteam();
}

/*
=============================================================================
PLAYER ANIMATION
=============================================================================
*/

/**
 * @brief CG_PlayerClassForClientinfo
 * @param[in] ci
 * @param[in] cent
 * @return
 */
bg_playerclass_t *CG_PlayerClassForClientinfo(clientInfo_t *ci, centity_t *cent)
{
	int team, cls;

	if (cent && cent->currentState.eType == ET_CORPSE)
	{
		return BG_GetPlayerClassInfo(cent->currentState.modelindex, cent->currentState.modelindex2);
	}

	if (cent && (cent->currentState.powerups & (1 << PW_OPS_DISGUISED)))
	{
		team = ci->team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;

		// fixed incorrect class determination (was & 6, should be & 7)
		cls = (cent->currentState.powerups >> PW_OPS_CLASS_1) & 7;

		return BG_GetPlayerClassInfo(team, cls);
	}

	return BG_GetPlayerClassInfo(ci->team, ci->cls);
}

/**
 * @brief CG_SetLerpFrameAnimation
 * @param cent
 * @param ci
 * @param lf
 * @param newAnimation
 * @note May include ANIM_TOGGLEBIT
 */
static void CG_SetLerpFrameAnimation(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation)
{
	animation_t    *anim;
	bg_character_t *character;

	character = CG_CharacterForClientinfo(ci, cent);

	if (!character)
	{
		CG_Printf("Warning: CG_SetLerpFrameAnimation w/o character.\n");
		return;
	}

	lf->animationNumber = newAnimation;
	newAnimation       &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= character->animModelInfo->numAnimations)
	{
		CG_Error("CG_SetLerpFrameAnimation: Bad animation number: %i\n", newAnimation);
	}

	anim = character->animModelInfo->animations[newAnimation];

	lf->animation     = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if (cg_debugAnim.integer == 1)
	{
		CG_Printf("Anim: %i, %s\n", newAnimation, character->animModelInfo->animations[newAnimation]->name);
	}
}

/**
 * @brief Sets cg.snap, cg.oldFrame, and cg.backlerp
 * cg.time should be between oldFrameTime and frameTime after exit
 * @param[in] cent
 * @param[in] ci
 * @param[in,out] lf
 * @param[in] newAnimation
 * @param[in] speedScale
 */
void CG_RunLerpFrame(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale)
{
	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0)
	{
		lf->oldFrame = lf->frame = 0;
		lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if (ci && (newAnimation != lf->animationNumber || !lf->animation))
	{
		CG_SetLerpFrameAnimation(cent, ci, lf, newAnimation);
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime)
	{
		animation_t *anim;
		int         f;

		lf->oldFrame      = lf->frame;
		lf->oldFrameTime  = lf->frameTime;
		lf->oldFrameModel = lf->frameModel;

		// get the next frame based on the animation
		anim = lf->animation;
		if (!anim || !anim->frameLerp)
		{
			CG_Printf("Warning: CG_RunLerpFrame w/o animation.\n");
			return;     // shouldn't happen
		}
		if (cg.time < lf->animationTime)
		{
			lf->frameTime = lf->animationTime;      // initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f  = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale;        // adjust for haste, etc
		if (f >= anim->numFrames)
		{
			f -= anim->numFrames;
			if (anim->loopFrames)
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame      = anim->firstFrame + f;
		lf->frameModel = anim->mdxFile;

		if (cg.time > lf->frameTime)
		{
			lf->frameTime = cg.time;
			if (cg_debugAnim.integer)
			{
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
	}

	if (lf->frameTime > cg.time + 200)
	{
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time)
	{
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime)
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0f - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

/**
 * @brief CG_ClearLerpFrame
 * @param[in] cent
 * @param[in] ci
 * @param[in,out] lf
 * @param animationNumber
 */
static void CG_ClearLerpFrame(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int animationNumber)
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation(cent, ci, lf, animationNumber);
	if (lf->animation)
	{
		lf->oldFrame      = lf->frame = lf->animation->firstFrame;
		lf->oldFrameModel = lf->frameModel = lf->animation->mdxFile;
	}
}

/**
 * @brief CG_SetLerpFrameAnimationRate
 * @param[in] cent
 * @param[in] ci
 * @param[in,out] lf
 * @param[in] newAnimation
 * @note May include ANIM_TOGGLEBIT
 */
void CG_SetLerpFrameAnimationRate(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation)
{
	animation_t    *anim, *oldanim;
	int            oldAnimNum; // oldAnimTime
	qboolean       firstAnim = qfalse;
	bg_character_t *character;

	character = CG_CharacterForClientinfo(ci, cent);

	if (!character)
	{
		CG_Printf("Warning: CG_SetLerpFrameAnimationRate w/o character.\n");
		return;
	}

	// oldAnimTime = lf->animationTime;
	oldanim    = lf->animation;
	oldAnimNum = lf->animationNumber;

	if (!lf->animation)
	{
		firstAnim = qtrue;
	}

	lf->animationNumber = newAnimation;
	newAnimation       &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= character->animModelInfo->numAnimations)
	{
		CG_Error("CG_SetLerpFrameAnimationRate: Bad animation number: %i\n", newAnimation);
	}

	anim = character->animModelInfo->animations[newAnimation];

	lf->animation     = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if (!(anim->flags & ANIMFL_FIRINGANIM) || (lf != &cent->pe.torso))
	{
		int transitionMin = -1;

		if ((lf == &cent->pe.legs) && (CG_IsCrouchingAnim(character->animModelInfo, newAnimation) != CG_IsCrouchingAnim(character->animModelInfo, oldAnimNum)))
		{
			if (anim->moveSpeed || (anim->movetype & ((1 << ANIM_MT_TURNLEFT) | (1 << ANIM_MT_TURNRIGHT))))             // if unknown movetype, go there faster
			{
				transitionMin = lf->frameTime + 200;    // slowly raise/drop
			}
			else
			{
				transitionMin = lf->frameTime + 350;    // slowly raise/drop
			}
		}
		else if (anim->moveSpeed)
		{
			transitionMin = lf->frameTime + 120;    // always do some lerping (?)
		}
		else     // not moving, so take your time
		{
			transitionMin = lf->frameTime + 170;    // always do some lerping (?)

		}
		if (oldanim && oldanim->animBlend)     //transitionMin < lf->frameTime + oldanim->animBlend) {
		{
			transitionMin     = lf->frameTime + oldanim->animBlend;
			lf->animationTime = transitionMin;
		}
		else
		{
			// slow down transitions according to speed
			if (anim->moveSpeed && lf->animSpeedScale < 1.0f)
			{
				lf->animationTime += anim->initialLerp;
			}

			if (lf->animationTime < transitionMin)
			{
				lf->animationTime = transitionMin;
			}
		}
	}

	// if first anim, go immediately
	if (firstAnim)
	{
		lf->frameTime     = cg.time - 1;
		lf->animationTime = cg.time - 1;
		lf->frame         = anim->firstFrame;
		lf->frameModel    = anim->mdxFile;
	}

	if (cg_debugAnim.integer == 1) // extra debug info
	{
		CG_Printf("Anim: %i, %s\n", newAnimation, character->animModelInfo->animations[newAnimation]->name);
	}
}

#define ANIM_SCALEMAX_LOW   1.1f
#define ANIM_SCALEMAX_HIGH  1.6f

#define ANIM_SPEEDMAX_LOW   100
#define ANIM_SPEEDMAX_HIGH  20

/**
 * @brief Sets cg.snap, cg.oldFrame, and cg.backlerp
 * cg.time should be between oldFrameTime and frameTime after exit
 * @param[in] ci
 * @param[in,out] lf
 * @param[in] newAnimation
 * @param[in] cent
 * @param[in] recursion
 */
void CG_RunLerpFrameRate(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, centity_t *cent, int recursion)
{
	animation_t *anim, *oldAnim;
	animation_t *otherAnim = NULL;
	qboolean    isLadderAnim;

	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0)
	{
		lf->oldFrame = lf->frame = 0;
		lf->backlerp = 0.f;
		return;
	}

	isLadderAnim = lf->animation && (lf->animation->flags & ANIMFL_LADDERANIM);
	oldAnim      = lf->animation;

	// see if the animation sequence is switching
	if (newAnimation != lf->animationNumber || !lf->animation)
	{
		CG_SetLerpFrameAnimationRate(cent, ci, lf, newAnimation);

		// make sure the animation is valid from CG_SetLerpFrameAnimationRate due to character absent
		if (!lf->animation)
		{
			CG_Printf("Warning: CG_RunLerpFrameRate w/o animation.\n");
			return;     // shouldn't happen
		}
	}

	// make sure the animation speed is updated when possible
	anim = lf->animation;

	// force last frame for corpses
	if (cent->currentState.eType == ET_CORPSE)
	{
		lf->oldFrame      = lf->frame = anim->firstFrame + anim->numFrames - 1;
		lf->oldFrameModel = lf->frameModel = anim->mdxFile;
		lf->backlerp      = 0;
		return;
	}

	if (anim->moveSpeed && lf->oldFrameSnapshotTime)
	{
		// calculate the speed at which we moved over the last frame
		if (cg.latestSnapshotTime != lf->oldFrameSnapshotTime && cg.nextSnap)
		{
			float moveSpeed;

			if (cent->currentState.number == cg.snap->ps.clientNum)
			{
				if (isLadderAnim)     // only use Z axis for speed
				{
					lf->oldFramePos[0] = cent->lerpOrigin[0];
					lf->oldFramePos[1] = cent->lerpOrigin[1];
				}
				else        // only use x/y axis
				{
					lf->oldFramePos[2] = cent->lerpOrigin[2];
				}
				moveSpeed = Distance(cent->lerpOrigin, lf->oldFramePos) / ((float)(cg.time - lf->oldFrameTime) / 1000.0f);
			}
			else
			{
				if (isLadderAnim)     // only use Z axis for speed
				{
					lf->oldFramePos[0] = cent->currentState.pos.trBase[0];
					lf->oldFramePos[1] = cent->currentState.pos.trBase[1];
				}
				moveSpeed = Distance(cent->lerpOrigin, lf->oldFramePos) / ((float)(cg.time - lf->oldFrameTime) / 1000.0f);
			}

			// convert it to a factor of this animation's movespeed
			lf->animSpeedScale       = moveSpeed / (float)anim->moveSpeed;
			lf->oldFrameSnapshotTime = cg.latestSnapshotTime;
		}
	}
	else
	{
		// move at normal speed
		lf->animSpeedScale       = 1.0f;
		lf->oldFrameSnapshotTime = cg.latestSnapshotTime;
	}
	// adjust with manual setting (pain anims)
	lf->animSpeedScale *= cent->pe.animSpeed;

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime)
	{
		int f;

		lf->oldFrame      = lf->frame;
		lf->oldFrameTime  = lf->frameTime;
		lf->oldFrameModel = lf->frameModel;
		VectorCopy(cent->lerpOrigin, lf->oldFramePos);

		// restrict the speed range
		if (lf->animSpeedScale < 0.25f)       // if it's too slow, then a really slow spped, combined with a sudden take-off, can leave them playing a really slow frame while they a moving really fast
		{
			if (lf->animSpeedScale < 0.01f && isLadderAnim)
			{
				lf->animSpeedScale = 0.0f;
			}
			else
			{
				lf->animSpeedScale = 0.25f;
			}
		}
		else if (lf->animSpeedScale > ANIM_SCALEMAX_LOW)
		{
			if (!(anim->flags & ANIMFL_LADDERANIM))
			{
				// allow slower anims to speed up more than faster anims
				if (anim->moveSpeed > ANIM_SPEEDMAX_LOW)
				{
					lf->animSpeedScale = ANIM_SCALEMAX_LOW;
				}
				else if (anim->moveSpeed < ANIM_SPEEDMAX_HIGH)
				{
					if (lf->animSpeedScale > ANIM_SCALEMAX_HIGH)
					{
						lf->animSpeedScale = ANIM_SCALEMAX_HIGH;
					}
				}
				else
				{
					lf->animSpeedScale = ANIM_SCALEMAX_HIGH - (ANIM_SCALEMAX_HIGH - ANIM_SCALEMAX_LOW) * (float)(anim->moveSpeed - ANIM_SPEEDMAX_HIGH) / (float)(ANIM_SPEEDMAX_LOW - ANIM_SPEEDMAX_HIGH);
				}
			}
			else if (lf->animSpeedScale > 4)
			{
				lf->animSpeedScale = 4.0f;
			}

		}

		if (lf == &cent->pe.legs)
		{
			otherAnim = cent->pe.torso.animation;
		}
		else if (lf == &cent->pe.torso)
		{
			otherAnim = cent->pe.legs.animation;
		}

		// get the next frame based on the animation
		if (lf->animSpeedScale == 0.f)
		{
			// stopped on the ladder, so stay on the same frame
			f              = lf->frame - anim->firstFrame;
			lf->frameTime += anim->frameLerp;       // don't wait too long before starting to move again
		}
		else if (lf->oldAnimationNumber != lf->animationNumber &&
		         (!anim->moveSpeed || lf->oldFrame < anim->firstFrame || lf->oldFrame >= anim->firstFrame + anim->numFrames))         // Ridah, added this so walking frames don't always get reset to 0, which can happen in the middle of a walking anim, which looks wierd
		{
			lf->frameTime = lf->animationTime;    // initial lerp
			if (oldAnim && anim->moveSpeed)       // keep locomotions going continuously
			{
				f = (lf->frame - oldAnim->firstFrame) + 1;
				while (f < 0)
				{
					f += anim->numFrames;
				}
			}
			else
			{
				f = 0;
			}
		}
		else if ((lf == &cent->pe.legs) && otherAnim && !(anim->flags & ANIMFL_FIRINGANIM) && ((lf->animationNumber & ~ANIM_TOGGLEBIT) == (cent->pe.torso.animationNumber & ~ANIM_TOGGLEBIT)) && (!anim->moveSpeed))
		{
			// legs should synch with torso
			f = cent->pe.torso.frame - otherAnim->firstFrame;
			if (f >= anim->numFrames || f < 0)
			{
				f = 0;  // wait at the start for the legs to catch up (assuming they are still in an old anim)
			}
			lf->frameTime = cent->pe.torso.frameTime;
		}
		else if ((lf == &cent->pe.torso) && otherAnim && !(anim->flags & ANIMFL_FIRINGANIM) && ((lf->animationNumber & ~ANIM_TOGGLEBIT) == (cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT)) && (otherAnim->moveSpeed))
		{
			// torso needs to sync with legs
			f = cent->pe.legs.frame - otherAnim->firstFrame;
			if (f >= anim->numFrames || f < 0)
			{
				f = 0;  // wait at the start for the legs to catch up (assuming they are still in an old anim)
			}
			lf->frameTime = cent->pe.legs.frameTime;
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + (int)((float)anim->frameLerp * (1.0f / lf->animSpeedScale));
			if (lf->frameTime < cg.time)
			{
				lf->frameTime = cg.time;
			}

			// check for skipping frames (eg. death anims play in slo-mo if low framerate)
			if (anim->flags & ANIMFL_REVERSED)
			{
				if (cg.time > lf->frameTime && !anim->moveSpeed)
				{
					f = (anim->numFrames - 1) - ((lf->frame - anim->firstFrame) - (1 + (cg.time - lf->frameTime) / anim->frameLerp));
				}
				else
				{
					f = (anim->numFrames - 1) - ((lf->frame - anim->firstFrame) - 1);
				}
			}
			else
			{
				if (cg.time > lf->frameTime && !anim->moveSpeed)
				{
					f = (lf->frame - anim->firstFrame) + 1 + (cg.time - lf->frameTime) / anim->frameLerp;
				}
				else
				{
					f = (lf->frame - anim->firstFrame) + 1;
				}
			}

			if (f < 0)
			{
				f = 0;
			}
		}
		//f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		if (f >= anim->numFrames)
		{
			f -= anim->numFrames;
			if (anim->loopFrames)
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if (anim->flags & ANIMFL_REVERSED)
		{
			lf->frame      = anim->firstFrame + anim->numFrames - 1 - f;
			lf->frameModel = anim->mdxFile;
		}
		else
		{
			lf->frame      = anim->firstFrame + f;
			lf->frameModel = anim->mdxFile;
		}

		if (cg.time > lf->frameTime)
		{

			// run the frame again until we move ahead of the current time, fixes walking speeds for zombie
			if (/*!anim->moveSpeed ||*/ recursion > 4)
			{
				lf->frameTime = cg.time;
			}
			else
			{
				CG_RunLerpFrameRate(ci, lf, newAnimation, cent, recursion + 1);
			}

			if (cg_debugAnim.integer > 3)
			{
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
		lf->oldAnimationNumber = lf->animationNumber;
	}

	// BIG hack, occaisionaly (VERY occaisionally), the frametime gets totally wacked
	if (lf->frameTime > cg.time + 5000)
	{
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time)
	{
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime)
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0f - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

/**
 * @brief CG_ClearLerpFrameRate
 * @param[in] cent
 * @param[in] ci
 * @param[in,out] lf
 * @param[in] animationNumber
 */
void CG_ClearLerpFrameRate(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int animationNumber)
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimationRate(cent, ci, lf, animationNumber);
	if (lf->animation)
	{
		lf->oldFrame      = lf->frame = lf->animation->firstFrame;
		lf->oldFrameModel = lf->frameModel = lf->animation->mdxFile;
	}
}

/**
 * @brief Similar to CG_RunLerpFrameRate, simplifed, and sets remaining animation, so animation won't be replayed
 * @param[in] cent
 * @param[in,out] lf
 * @param[in] newAnimation
 */
void CG_SetLerpFrameAnimationRateCorpse(centity_t *cent, lerpFrame_t *lf, int newAnimation)
{
	animation_t *anim;
	int         rest;

	bg_character_t *character;

	if (cent->currentState.onFireStart >= 0)
	{
		character = cgs.gameCharacters[cent->currentState.onFireStart];
	}
	else
	{
		if (cent->currentState.modelindex < 4)
		{
			character = BG_GetCharacter(cent->currentState.modelindex, cent->currentState.modelindex2);
		}
		else
		{
			character = BG_GetCharacter(cent->currentState.modelindex - 4, cent->currentState.modelindex2);
		}
	}

	if (!character)
	{
		CG_Printf("Warning: CG_SetLerpFrameAnimationRateCorpse w/o character.\n");
		return;
	}

	lf->animationNumber = newAnimation;
	newAnimation       &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= character->animModelInfo->numAnimations)
	{
		CG_Error("CG_SetLerpFrameAnimationRate: Bad animation number: %i", newAnimation);
	}

	anim = character->animModelInfo->animations[newAnimation];
	rest = cent->currentState.effect1Time - cg.time;    // duratiom of remaining animation

	// make sure it is not out of anim
	if (rest < 0)
	{
		rest = 0;
	}

	if (rest > anim->duration)
	{
		rest = anim->duration;
	}

	lf->animation     = anim;
	lf->frame         = anim->firstFrame + ((anim->duration - rest) / anim->frameLerp);
	lf->frameTime     = cg.time - 1;
	lf->animationTime = cg.time + rest - anim->duration;
	lf->frameModel    = anim->mdxFile;
	if (cg_debugAnim.integer)
	{
		CG_Printf("Anim: %i, %s\n", newAnimation, character->animModelInfo->animations[newAnimation]->name);
	}
}

/**
 * @brief Simplifed code for corpses
 * @param[in] ci - unused
 * @param[in,out] lf
 * @param[in] newAnimation
 * @param[in] cent
 * @param[in] recursion - unused
 */
void CG_RunLerpFrameRateCorpse(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, centity_t *cent, int recursion)
{
	animation_t *anim;

	// see if the animation sequence is switching
	if (newAnimation != lf->animationNumber || !lf->animation)
	{
		CG_SetLerpFrameAnimationRateCorpse(cent, lf, newAnimation);

		// make sure the animation is valid from CG_SetLerpFrameAnimationRateCorpse due to character absent
		if (!lf->animation)
		{
			CG_Printf("Warning: CG_RunLerpFrameRateCorpse w/o animation.\n");
			return;     // shouldn't happen
		}
	}

	// make sure the animation speed is updated when possible
	anim = lf->animation;

	// animation time gone
	if (cent->currentState.effect1Time < cg.time)
	{
		lf->oldFrame      = lf->frame = anim->firstFrame + anim->numFrames - 1;
		lf->oldFrameModel = lf->frameModel = anim->mdxFile;
		lf->backlerp      = 0;
		return;
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime)
	{
		int f;

		lf->oldFrame      = lf->frame;
		lf->oldFrameTime  = lf->frameTime;
		lf->oldFrameModel = lf->frameModel;

		// get the next frame based on the animation
		anim = lf->animation;
		if (!anim || !anim->frameLerp)
		{
			CG_Printf("Warning: CG_RunLerpFrameRateCorpse w/o animation.\n");
			return;     // shouldn't happen
		}
		if (cg.time < lf->animationTime)
		{
			lf->frameTime = lf->animationTime;      // initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		if (f >= anim->numFrames)
		{
			f -= anim->numFrames;
			if (anim->loopFrames)
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame      = anim->firstFrame + f;
		lf->frameModel = anim->mdxFile;
		if (cg.time > lf->frameTime)
		{
			lf->frameTime = cg.time;

			if (cg_debugAnim.integer)
			{
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
	}

	if (lf->frameTime > cg.time + 200)
	{
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time)
	{
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime)
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0f - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

/**
 * @brief CG_PlayerAnimation
 * @param[in] cent
 * @param[out] body
 */
static void CG_PlayerAnimation(centity_t *cent, refEntity_t *body)
{
	int            clientNum = cent->currentState.clientNum;
	clientInfo_t   *ci       = &cgs.clientinfo[clientNum];
	int            animIndex;
	bg_character_t *character;

	character = CG_CharacterForClientinfo(ci, cent);

	if (!character)
	{
		CG_Printf("Warning: CG_PlayerAnimation w/o character.\n");
		return;
	}

	if (cg_noPlayerAnims.integer)
	{
		body->frame      = body->oldframe = body->torsoFrame = body->oldTorsoFrame = 0;
		body->frameModel = body->oldframeModel = body->torsoFrameModel = body->oldTorsoFrameModel = character->animModelInfo->animations[0]->mdxFile;
		return;
	}

	// default to whatever the legs are currently doing
	animIndex = cent->currentState.legsAnim;

	// do the shuffle turn frames locally
	if (!(cent->currentState.eFlags & EF_DEAD) && cent->pe.legs.yawing)
	{
		//CG_Printf("turn: %i\n", cg.time );
		int tempIndex;

		tempIndex = BG_GetAnimScriptAnimation(clientNum, character->animModelInfo, cent->currentState.aiState, (cent->pe.legs.yawing == SWING_RIGHT ? ANIM_MT_TURNRIGHT : ANIM_MT_TURNLEFT));

		if (tempIndex > -1)
		{
			animIndex = tempIndex;
		}
	}
	// run the animation
	if (cent->currentState.eType == ET_CORPSE)
	{
		CG_RunLerpFrameRateCorpse(ci, &cent->pe.legs, animIndex, cent, 0);
	}
	else
	{
		CG_RunLerpFrameRate(ci, &cent->pe.legs, animIndex, cent, 0);
	}

	body->oldframe      = cent->pe.legs.oldFrame;
	body->frame         = cent->pe.legs.frame;
	body->backlerp      = cent->pe.legs.backlerp;
	body->frameModel    = cent->pe.legs.frameModel;
	body->oldframeModel = cent->pe.legs.oldFrameModel;

	if (cent->currentState.eType == ET_CORPSE)
	{
		CG_RunLerpFrameRateCorpse(ci, &cent->pe.torso, cent->currentState.torsoAnim, cent, 0);
	}
	else
	{
		CG_RunLerpFrameRate(ci, &cent->pe.torso, cent->currentState.torsoAnim, cent, 0);
	}

	body->oldTorsoFrame      = cent->pe.torso.oldFrame;
	body->torsoFrame         = cent->pe.torso.frame;
	body->torsoBacklerp      = cent->pe.torso.backlerp;
	body->torsoFrameModel    = cent->pe.torso.frameModel;
	body->oldTorsoFrameModel = cent->pe.torso.oldFrameModel;
}

/*
=============================================================================
PLAYER ANGLES
=============================================================================
*/

/**
 * @brief CG_SwingAngles
 * @param[in] destination
 * @param[in] swingTolerance
 * @param[in] clampTolerance
 * @param[in] speed
 * @param[in,out] angle
 * @param[in,out] swinging
 */
static void CG_SwingAngles(float destination, float swingTolerance, float clampTolerance,
                           float speed, float *angle, qboolean *swinging)
{
	float swing;
	float move;
	float scale;

	if (!*swinging)
	{
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if (swing > swingTolerance || swing < -swingTolerance)
		{
			*swinging = qtrue;
		}
	}

	if (!*swinging)
	{
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing  = AngleSubtract(destination, *angle);
	scale  = Q_fabs(swing);
	scale *= 0.05;
	if (scale < 0.5f)
	{
		scale = 0.5f;
	}

	// swing towards the destination angle
	if (swing >= 0)
	{
		move = cg.frametime * scale * speed;
		if (move >= swing)
		{
			move      = swing;
			*swinging = qfalse;
		}
		else
		{
			*swinging = SWING_LEFT;     // left
		}
		*angle = AngleMod(*angle + move);
	}
	else /*if (swing < 0)*/
	{
		move = cg.frametime * scale * -speed;
		if (move <= swing)
		{
			move      = swing;
			*swinging = qfalse;
		}
		else
		{
			*swinging = SWING_RIGHT;    // right
		}
		*angle = AngleMod(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if (swing > clampTolerance)
	{
		*angle = AngleMod(destination - (clampTolerance - 1));
	}
	else if (swing < -clampTolerance)
	{
		*angle = AngleMod(destination + (clampTolerance - 1));
	}
}

/**
 * @brief CG_AddPainTwitch
 * @param[in,out] cent
 * @param[in] torsoAngles
 * @note direction unused ?
 */
static void CG_AddPainTwitch(centity_t *cent, vec3_t torsoAngles)
{
	int   t;
	float f;
	int   duration;
	//float direction;

	if (cent->pe.animSpeed == 0.f)
	{
		// we need to inititialize this stuff
		cent->pe.painAnimLegs  = -1;
		cent->pe.painAnimTorso = -1;
		cent->pe.animSpeed     = 1.0f;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		cent->pe.painAnimLegs  = -1;
		cent->pe.painAnimTorso = -1;
		cent->pe.animSpeed     = 1.0f;
		return;
	}

	if (cent->pe.painDuration)
	{
		duration = cent->pe.painDuration;
	}
	else
	{
		duration = PAIN_TWITCH_TIME;
	}

	/*
	direction = (float)duration * 0.085f;
	if (direction > 30)
	{
	    direction = 30;
	}
	if (direction < 10)
	{
	    direction = 10;
	}
	direction *= (float)(cent->pe.painDirection * 2) - 1;
	*/

	t = cg.time - cent->pe.painTime;
	if (t >= duration)
	{
		return;
	}

	f = 1.0f - (float)t / duration;
	if (cent->pe.painDirection)
	{
		torsoAngles[ROLL] += 20 * f;
	}
	else
	{
		torsoAngles[ROLL] -= 20 * f;
	}
}

#define LEAN_MAX    28.0f
#define LEAN_TIME_TO    200.0f  // time to get to/from full lean
#define LEAN_TIME_FR    300.0f  // time to get to/from full lean

/**
 * @brief CG_PredictLean
 * @param[in,out] cent
 * @param[in] torsoAngles
 * @param[in] headAngles
 * @param[in] viewHeight
 */
void CG_PredictLean(centity_t *cent, vec3_t torsoAngles, vec3_t headAngles, int viewHeight)
{
	int   leaning = 0;          // -1 left, 1 right
	float leanofs = 0;
	int   time;

	if (cent->currentState.constantLight & STAT_LEAN_LEFT)
	{
		leaning -= 1;
	}
	if (cent->currentState.constantLight & STAT_LEAN_RIGHT)
	{
		leaning += 1;
	}

	// note not really needed, just for better prediction
	if (BG_PlayerMounted(cent->currentState.eFlags))
	{
		leaning = 0;    // leaning not allowed on mg42
	}

	if (cent->currentState.eFlags & EF_FIRING)
	{
		leaning = 0;    // not allowed to lean while firing
	}
	if (cent->currentState.eFlags & EF_DEAD)
	{
		leaning = 0;    // not allowed to lean while dead
	}
	if ((cent->currentState.eFlags & EF_PRONE) || CHECKBITWISE(GetWeaponTableData(cent->currentState.weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
	{
		leaning = 0;    // not allowed to lean while prone
	}

	leanofs = cent->pe.leanDirection;

	if (leaning != cent->pe.leanDir)
	{
		cent->pe.leanTime = cg.time;
		cent->pe.leanDir  = leaning;
	}

	time = cg.time - cent->pe.leanTime;

	if (time < 1)
	{
		time = 1;
	}
	else if (time > 200)
	{
		time = 200;
	}

	cent->pe.leanTime = cg.time;

	if (!leaning)    // go back to center position
	{
		if (leanofs > 0)            // right
		{
			leanofs -= (((float)time / (float)LEAN_TIME_FR) * LEAN_MAX);
			if (leanofs < 0)
			{
				leanofs = 0;
			}
		}
		else if (leanofs < 0)       // left
		{
			leanofs += (((float)time / (float)LEAN_TIME_FR) * LEAN_MAX);
			if (leanofs > 0)
			{
				leanofs = 0;
			}
		}
	}

	if (leaning)
	{
		if (leaning > 0)   // right
		{
			if (leanofs < LEAN_MAX)
			{
				leanofs += (((float)time / (float)LEAN_TIME_TO) * LEAN_MAX);
			}

			if (leanofs > LEAN_MAX)
			{
				leanofs = LEAN_MAX;
			}
		}
		else        // left
		{
			if (leanofs > -LEAN_MAX)
			{
				leanofs -= (((float)time / (float)LEAN_TIME_TO) * LEAN_MAX);
			}

			if (leanofs < -LEAN_MAX)
			{
				leanofs = -LEAN_MAX;
			}
		}
	}

	cent->pe.leanDirection = leanofs;

	if (leaning)
	{
		vec3_t  start, end, tmins, tmaxs, right, viewangles;
		trace_t trace;

		VectorCopy(cent->lerpOrigin, start);
		start[2] += viewHeight;

		VectorCopy(cent->lerpAngles, viewangles);
		viewangles[ROLL] += leanofs / 2.0f;
		AngleVectors(viewangles, NULL, right, NULL);
		VectorMA(start, leanofs, right, end);

		VectorSet(tmins, -8, -8, -7);   // ATVI Wolfenstein Misc #472, bumped from -4 to cover gun clipping issue
		VectorSet(tmaxs, 8, 8, 4);

		CG_Trace(&trace, start, tmins, tmaxs, end, cent->currentState.clientNum, MASK_PLAYERSOLID);

		cent->pe.leanDirection *= trace.fraction;
	}

	if (torsoAngles)
	{
		torsoAngles[ROLL] += cent->pe.leanDirection * 1.25f;
	}
	if (headAngles)
	{
		headAngles[ROLL] += cent->pe.leanDirection; //* 0.75 ;
	}
}

/**
 * @brief Handles seperate torso motion
 *
 * legs pivot based on direction of movement
 *
 * head always looks exactly at cent->lerpAngles
 *
 * if motion < 20 degrees, show in head only
 * if < 45 degrees, also show in torso
 * @param[in,out] cent
 * @param[in] legs
 * @param[in] torso
 * @param[in] head
 */
static void CG_PlayerAngles(centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3])
{
	vec3_t         legsAngles, torsoAngles, headAngles;
	float          dest;
	vec3_t         velocity;
	float          speed;
	int            legsSet; // torsoSet;
	clientInfo_t   *ci        = &cgs.clientinfo[cent->currentState.clientNum];
	bg_character_t *character = CG_CharacterForClientinfo(ci, cent);
	centity_t      *cgsnap    = &cg_entities[cg.snap->ps.clientNum];

	if (!character)
	{
		return;
	}

	legsSet = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	//torsoSet = cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT;

	VectorCopy(cent->lerpAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);

	if (cent->currentState.eType == ET_CORPSE)
	{
		headAngles[0] = headAngles[2] = 0;
	}

	VectorClear(legsAngles);
	VectorClear(torsoAngles);

	// --------- yaw -------------

	// allow yaw to drift a bit, unless these conditions don't allow them
	if (!(BG_GetConditionBitFlag(cent->currentState.clientNum, ANIM_COND_MOVETYPE, ANIM_MT_IDLE) ||
	      BG_GetConditionBitFlag(cent->currentState.clientNum, ANIM_COND_MOVETYPE, ANIM_MT_IDLECR)) /*
	    ||   (BG_GetConditionValue( cent->currentState.clientNum, ANIM_COND_MOVETYPE, qfalse ) & ((1<<ANIM_MT_STRAFELEFT) | (1<<ANIM_MT_STRAFERIGHT)) )*/)
	{

		// always point all in the same direction
		cent->pe.torso.yawing   = qtrue; // always center
		cent->pe.torso.pitching = qtrue; // always center
		cent->pe.legs.yawing    = qtrue; // always center

		// if firing, make sure torso and head are always aligned
	}
	else if (BG_GetConditionValue(cent->currentState.clientNum, ANIM_COND_FIRING, qtrue))
	{
		cent->pe.torso.yawing   = qtrue; // always center
		cent->pe.torso.pitching = qtrue; // always center
	}

	// adjust legs for movement dir
	if ((cent->currentState.eFlags & EF_DEAD) || (cent->currentState.eFlags & EF_MOUNTEDTANK))
	{
		// don't let dead bodies twitch
		legsAngles[YAW]  = headAngles[YAW];
		torsoAngles[YAW] = headAngles[YAW];
	}
	else
	{
		float clampTolerance;

		legsAngles[YAW] = headAngles[YAW] + cent->currentState.angles2[YAW];

		if (!(cent->currentState.eFlags & EF_FIRING))
		{
			torsoAngles[YAW] = headAngles[YAW] + 0.35f * cent->currentState.angles2[YAW];
			clampTolerance   = 90;
		}
		else        // must be firing
		{
			torsoAngles[YAW] = headAngles[YAW]; // always face firing direction
			//if (Q_fabs(cent->currentState.angles2[YAW]) > 30)
			//  legsAngles[YAW] = headAngles[YAW];
			clampTolerance = 60;
		}

		// torso
		CG_SwingAngles(torsoAngles[YAW], 25, clampTolerance, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing);

		// if the legs are yawing (facing heading direction), allow them to rotate a bit, so we don't keep calling
		// the legs_turn animation while an AI is firing, and therefore his angles will be randomizing according to their accuracy

		clampTolerance = 150;

		if (BG_GetConditionBitFlag(ci->clientNum, ANIM_COND_MOVETYPE, ANIM_MT_IDLE))
		{
			cent->pe.legs.yawing = qfalse; // set it if they really need to swing
			CG_SwingAngles(legsAngles[YAW], 20, clampTolerance, 0.5f * cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);
		}
		else if (strstr(BG_GetAnimString(character->animModelInfo, legsSet), "strafe"))
		{
			// FIXME: what is this strstr hack??
			//if    ( BG_GetConditionValue( ci->clientNum, ANIM_COND_MOVETYPE, qfalse ) & ((1<<ANIM_MT_STRAFERIGHT)|(1<<ANIM_MT_STRAFELEFT)) )
			cent->pe.legs.yawing = qfalse; // set it if they really need to swing
			legsAngles[YAW]      = headAngles[YAW];
			CG_SwingAngles(legsAngles[YAW], 0, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);
		}
		else if (cent->pe.legs.yawing)
		{
			CG_SwingAngles(legsAngles[YAW], 0, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);
		}
		else
		{
			CG_SwingAngles(legsAngles[YAW], 40, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);
		}

		torsoAngles[YAW] = cent->pe.torso.yawAngle;
		legsAngles[YAW]  = cent->pe.legs.yawAngle;
	}

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if (headAngles[PITCH] > 180)
	{
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	}
	else
	{
		dest = headAngles[PITCH] * 0.75f;
	}
	//CG_SwingAngles( dest, 15, 30, 0.1, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	//torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

	if (cent->currentState.eFlags & EF_PRONE)
	{
		torsoAngles[PITCH] = legsAngles[PITCH] - 3;
	}
	else
	{
		CG_SwingAngles(dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching);
		torsoAngles[PITCH] = cent->pe.torso.pitchAngle;
	}

	// --------- roll -------------

	// lean towards the direction of travel
	VectorCopy(cent->currentState.pos.trDelta, velocity);
	speed = VectorNormalize(velocity);
	if (speed != 0.f)
	{
		vec3_t axis[3];
		float  side;

		speed *= 0.05;

		AnglesToAxis(legsAngles, axis);
		side              = speed * DotProduct(velocity, axis[1]);
		legsAngles[ROLL] -= side;

		side               = speed * DotProduct(velocity, axis[0]);
		legsAngles[PITCH] += side;
	}

	CG_PredictLean(cent, torsoAngles, headAngles, cg.snap->ps.clientNum == cent->currentState.clientNum ? cg.snap->ps.viewheight :  (int)cent->pe.headRefEnt.origin[2]);

	// pain twitch
	CG_AddPainTwitch(cent, torsoAngles);

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
	AnglesToAxis(legsAngles, legs);
	AnglesToAxis(torsoAngles, torso);
	AnglesToAxis(headAngles, head);

	if (cgsnap == cent && (cg.snap->ps.pm_flags & PMF_LADDER))
	{
		Com_Memcpy(torso, legs, sizeof(*torso));
	}
}

/**
 * @brief CG_BreathPuffs
 * @param[in] cent
 * @param[in] head
 */
static void CG_BreathPuffs(centity_t *cent, refEntity_t *head)
{
	clientInfo_t *ci = &cgs.clientinfo[cent->currentState.number];
	vec3_t       up, forward;
	int          contents;
	vec3_t       mang, morg, maxis[3];

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson)
	{
		return;
	}

	if (!(cent->currentState.eFlags & EF_DEAD))
	{
		return;
	}

	// see bg_pmove.c pml.groundTrace.surfaceFlags
	if (!(cent->currentState.eFlags & EF_BREATH))
	{
		return;
	}

	contents = CG_PointContents(head->origin, 0);
	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		return;
	}
	if (ci->breathPuffTime > cg.time)
	{
		return;
	}

	CG_GetOriginForTag(cent, head, "tag_mouth", 0, morg, maxis);
	AxisToAngles(maxis, mang);
	AngleVectors(mang, forward, NULL, up);

	// push the origin out a tad so it's not right in the guys face (tad==4)
	VectorMA(morg, 4, forward, morg);

	forward[0] = up[0] * 8 + forward[0] * 5;
	forward[1] = up[1] * 8 + forward[1] * 5;
	forward[2] = up[2] * 8 + forward[2] * 5;

	CG_SmokePuff(morg, forward, 4, 1, 1, 1, 0.5f, 2000, cg.time, cg.time + 400, 0, cgs.media.shotgunSmokePuffShader);

	ci->breathPuffTime = (int)(cg.time + 3000 + random() * 1000);
}

/**
 * @brief Float a sprite over the player's head added height parameter
 * @param[in] cent
 * @param[in] shader
 * @param[in] height
 * @param[in] off
 */
static void CG_PlayerFloatSprite(centity_t *cent, qhandle_t shader, int height, int off, vec4_t color)
{
	int         rf;
	vec3_t      right;
	refEntity_t ent;
	int         hPos[] = { 0, -13, 13,
		                   0,         -13, 13,
		                   0,         -13, 13 };
	int         vPos[] = { 0, 0,  0,
		                   13,        13, 13,
		                   26,        26, 26 };

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson)
	{
		rf = RF_THIRD_PERSON;       // only show in mirrors
	}
	else
	{
		rf = 0;
	}

	Com_Memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += height;

	AngleVectors(cg.refdefViewAngles, NULL, right, NULL);

	if (off > 8)   // sprite limit is 9, note: current code has 8 sprites
	{
		return;
	}

	// move it!
	ent.origin[2] += vPos[off];
	VectorMA(ent.origin, hPos[off], right, ent.origin) ;

	// Account for ducking
	// FIXME: adjust origin for others
	if (cent->currentState.clientNum == cg.snap->ps.clientNum)
	{
		if (cent->currentState.eFlags & EF_CROUCHING)
		{
			ent.origin[2] -= 18;
		}
		else if (cent->currentState.eFlags & EF_PRONE)
		{
			ent.origin[2] -= 45;
		}
	}
	else
	{
		if (cent->currentState.animMovetype)
		{
			ent.origin[2] -= 18;
		}
	}

	ent.reType       = RT_SPRITE;
	ent.customShader = shader;
	ent.radius       = 6.66f;
	ent.renderfx     = rf;

	if (color == NULL)
	{
		color = colorWhite;
	}

	ent.shaderRGBA[0] = 255 * color[0];
	ent.shaderRGBA[1] = 255 * color[1];
	ent.shaderRGBA[2] = 255 * color[2];
	ent.shaderRGBA[3] = 255 * color[3];
	trap_R_AddRefEntityToScene(&ent);
}

/**
 * @brief CG_PlayerFloatText
 * @param[in] cent
 * @param[in] text
 * @param[in] height
 */
static void CG_PlayerFloatText(centity_t *cent, const char *text, int height)
{
	vec3_t origin;

	VectorCopy(cent->lerpOrigin, origin);

	origin[2] += height;

	// adjust label height
	if (cent->currentState.eFlags & EF_CROUCHING ||
	    cent->currentState.eFlags & EF_PRONE || cent->currentState.eFlags & EF_PRONE_MOVING)
	{
		origin[2] -= 18;
	}

	CG_AddOnScreenText(text, origin);
}

/**
* @brief CG_PlayerBar
* @param[in] cent
* @param[in] fraction
* @param[in] colorStart
* @param[in] colorEnd
* @param[in] colorBack
* @param[in] height
*/
static void CG_PlayerFloatBar(centity_t *cent, float fraction, vec4_t colorStart, vec4_t colorEnd, vec4_t colorBack, int height)
{
	vec3_t origin;

	VectorCopy(cent->lerpOrigin, origin);

	origin[2] += height;

	// adjust bar height
	if (cent->currentState.eFlags & EF_CROUCHING ||
	    cent->currentState.eFlags & EF_PRONE || cent->currentState.eFlags & EF_PRONE_MOVING)
	{
		origin[2] -= 18;
	}

	CG_AddOnScreenBar(fraction, colorStart, colorEnd, colorBack, origin);
}

/**
 * @brief Float sprites over the player's head
 * @param[in] cent
 */
static void CG_PlayerSprites(centity_t *cent)
{
	int          numIcons = 0;
	int          height   = 56;
	clientInfo_t *ci      = &cgs.clientinfo[cent->currentState.clientNum];
	qboolean     sameTeam = (cg.snap->ps.persistant[PERS_TEAM] == ci->team);
	int          spacing  = 8;
	char         *name;

	if ((cent->currentState.powerups & (1 << PW_REDFLAG)) || (cent->currentState.powerups & (1 << PW_BLUEFLAG)))
	{
		// check if we see the enemy head, otherwise don't display the objectif icon
		// when hiding behind decor
		if (!sameTeam && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
		{
			trace_t trace;
			vec3_t  end;

			VectorMA(cent->pe.headRefEnt.origin, 6.0f, cent->pe.headRefEnt.axis[2], end);

			CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, end, cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM);

			// don't draw player icons if we can't see their head
			if (trace.fraction == 1.f || trace.entityNum == cent->currentState.number)
			{
				CG_PlayerFloatSprite(cent, cgs.media.objectiveShader, height, numIcons++, NULL);
			}
		}
		else
		{
			CG_PlayerFloatSprite(cent, cgs.media.objectiveShader, height, numIcons++, NULL);
		}
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		height = 8;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || ((cg.snap->ps.pm_flags & PMF_FOLLOW) && cgs.clientinfo[cg.clientNum].shoutcaster))
	{
		if (cg_shoutcastDrawHealth.integer > 0 && cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			if (cg_shoutcastDrawHealth.integer == 1 && cg_drawSpectatorNames.integer == 0)
			{
				CG_PlayerFloatText(cent, va("%s%ihp", ci->team == TEAM_AXIS ? "^1" : "^2", ci->health), height + spacing);
			}
			else if (cg_shoutcastDrawHealth.integer == 2)
			{
				vec4_t bgcolor     = { 0.0f, 0.0f, 0.0f, 1.0f };
				vec4_t healthColor = { 0.0f, 1.0f, 0.0f, 1.0f };

				float fraction = (float)ci->health / (float)CG_GetPlayerMaxHealth(cent->currentState.clientNum, ci->cls, ci->team);

				if (ci->team == TEAM_AXIS)
				{
					Vector4Set(healthColor, 1.0f, 0.0f, 0.0f, 1.0f);
				}

				CG_PlayerFloatBar(cent, fraction, healthColor, healthColor, bgcolor, height + spacing + 8);
				spacing += 14;
			}
		}

		if (cg_drawSpectatorNames.integer > 0)
		{
			name = cg_drawSpectatorNames.integer == 1 ? ci->cleanname : ci->name;

			if (cg_shoutcastDrawHealth.integer == 1 && cgs.clientinfo[cg.clientNum].shoutcaster)
			{
				name = va("%s %s%ihp", name, ci->team == TEAM_AXIS ? "^1" : "^2", ci->health);
			}

			CG_PlayerFloatText(cent, name, height + spacing);
		}

		// show some useful icons to spectators
		if (cent->currentState.eFlags & EF_CONNECTION)
		{
			CG_PlayerFloatSprite(cent, cgs.media.disconnectIcon, height, numIcons++, NULL);
			return;
		}
		if (cent->currentState.eFlags & EF_DEAD &&
		    ((cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC && cg.snap->ps.stats[STAT_HEALTH] > 0 && sameTeam) ||
		     (!(cg.snap->ps.pm_flags & PMF_FOLLOW) && cgs.clientinfo[cg.clientNum].shoutcaster) ||
		     ((cg.snap->ps.pm_flags & PMF_FOLLOW) && cgs.clientinfo[cg.snap->ps.clientNum].shoutcaster)))
		{
			CG_PlayerFloatSprite(cent, cgs.media.medicReviveShader, height, numIcons++, NULL);
			return;
		}
		if (cent->currentState.powerups & (1 << PW_INVULNERABLE))
		{
			CG_PlayerFloatSprite(cent, cgs.media.spawnInvincibleShader, height, numIcons++, NULL);
		}
		if (cent->currentState.powerups & (1 << PW_OPS_DISGUISED) && cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			CG_PlayerFloatSprite(cent, cgs.media.disguisedShader, height, numIcons++, NULL);
		}
		return;
	}

	if (cg.demoPlayback && cg_drawSpectatorNames.integer > 0)
	{
		CG_PlayerFloatText(cent, cg_drawSpectatorNames.integer == 1 ? ci->cleanname : ci->name, height + 8);
	}

	if (cent->currentState.powerups & (1 << PW_INVULNERABLE))
	{
		CG_PlayerFloatSprite(cent, cgs.media.spawnInvincibleShader, height, numIcons++, NULL);
	}

	// If this client is a medic, draw a 'revive' icon over
	// dead players that are not in limbo yet.
	if ((cent->currentState.eFlags & EF_DEAD)
	    && cent->currentState.number == cent->currentState.clientNum
	    && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_MEDIC
	    && cg.snap->ps.stats[STAT_HEALTH] > 0
	    && sameTeam)
	{
		CG_PlayerFloatSprite(cent, cgs.media.medicReviveShader, height, numIcons++, NULL);
	}

	if (cent->currentState.eFlags & EF_CONNECTION)
	{
		CG_PlayerFloatSprite(cent, cgs.media.disconnectIcon, height, numIcons++, NULL);
	}

	// show voice chat signal so players know who's talking
	if (cent->voiceChatSpriteTime > cg.time)
	{
		if (sameTeam)
		{
			CG_PlayerFloatSprite(cent, cent->voiceChatSprite, height, numIcons++, NULL);
		}
	}

	// only show talk icon to team-mates
	if ((cent->currentState.eFlags & EF_TALK) && sameTeam)
	{
		CG_PlayerFloatSprite(cent, cgs.media.balloonShader, height, numIcons++, NULL);
	}

	// draw disguised icon over disguised teammates and the fireteam icon of uniform source for enemies
	if (cent->currentState.powerups & (1 << PW_OPS_DISGUISED))
	{
		if (sameTeam)
		{
			CG_PlayerFloatSprite(cent, cgs.media.friendShader, height, numIcons++, NULL);
		}
		else // !sameTeam
		{
			if (cgs.clientinfo[cent->currentState.number].disguiseClientNum > -1
			    && CG_IsOnFireteam(cgs.clientinfo[cent->currentState.number].disguiseClientNum)
			    && CG_IsOnSameFireteam(cgs.clientinfo[cent->currentState.number].disguiseClientNum, cg.clientNum)
			    && cg.fireTeams->membersNumber > 1)                         // don't display FT icon with only 1 member in FT
			{
				CG_PlayerFloatSprite(cent, cgs.media.fireteamIcon, height, numIcons++,
				                     cgs.clientinfo[cgs.clientinfo[cent->currentState.number].disguiseClientNum].selected ? colorRed : colorGreen);
			}

			// shoutcasters see undercover enemies
			if (cgs.clientinfo[cent->currentState.number].disguiseClientNum > -1 && cgs.clientinfo[cg.clientNum].shoutcaster)
			{
				CG_PlayerFloatSprite(cent, cgs.media.disguisedShader, height, numIcons++, NULL);
			}
		}
	}

	if (CG_IsOnFireteam(cent->currentState.number) && CG_IsOnSameFireteam(cent->currentState.number, cg.clientNum))
	{
		CG_PlayerFloatSprite(cent, cgs.media.fireteamIcon, height, numIcons++,
		                     cgs.clientinfo[cent->currentState.number].selected ? colorRed : colorGreen);
	}
}

#define SHADOW_DISTANCE     64
#define ZOFS    6.0f
#define SHADOW_MIN_DIST 250.0f
#define SHADOW_MAX_DIST 512.0f

typedef struct
{
	char *tagname;
	float size;
	//float maxdist;  // unused, related code is disabled
	//float maxalpha; // unused, related code is disabled
	qhandle_t shader;
} shadowPart_t;

/**
 * @brief Returns the Z component of the surface being shadowed
 * @param[in] cent
 * @param[in,out] shadowPlane
 * @return
 * @note TODO: Should it return a full plane instead of a Z?
 */
static qboolean CG_PlayerShadow(centity_t *cent, float *shadowPlane)
{
	vec3_t       end;
	trace_t      trace;
	float        dist, distFade;
	vec3_t       origin, axis[3];
	vec4_t       projection    = { 0, 0, -1, 64 };
	shadowPart_t shadowParts[] =
	{
		//{ "tag_footleft",  10, 4,  1.0, 0 },
		//{ "tag_footright", 10, 4,  1.0, 0 },
		//{ "tag_torso",     18, 96, 0.8, 0 },
		{ "tag_footleft",  10, 0 },
		{ "tag_footright", 10, 0 },
		{ "tag_torso",     18, 0 },
		{ NULL,            0,  0 }
	};

	shadowParts[0].shader = cgs.media.shadowFootShader;     // pulled out of initliization
	shadowParts[1].shader = cgs.media.shadowFootShader;
	shadowParts[2].shader = cgs.media.shadowTorsoShader;

	*shadowPlane = 0;

	if (cg_shadows.integer == 0)
	{
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy(cent->lerpOrigin, end);
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace(&trace, cent->lerpOrigin, end, NULL, NULL, 0, MASK_PLAYERSOLID);

	// no shadow if too high
	//if ( trace.fraction == 1.0 || trace.fraction == 0.0f ) {
	//  return qfalse;
	//}

	*shadowPlane = trace.endpos[2] + 1;

	if (cg_shadows.integer != 1)        // no mark for stencil or projection shadows
	{
		return qtrue;
	}

	// no shadows when dead
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return qfalse;
	}

	// fade the shadow out with height
	//alpha = 1.0 - trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	dist     = VectorDistance(cent->lerpOrigin, cg.refdef_current->vieworg); //% cg.snap->ps.origin );
	distFade = 1.0f;
	if (!(cent->currentState.eFlags & EF_ZOOMING) && (dist > SHADOW_MIN_DIST))
	{
		if (dist > SHADOW_MAX_DIST)
		{
			if (dist > SHADOW_MAX_DIST * 2)
			{
				return qfalse;
			}
			else     // fade out
			{
				distFade = 1.0f - ((dist - SHADOW_MAX_DIST) / SHADOW_MAX_DIST);
			}

			if (distFade > 1.0f)
			{
				distFade = 1.0f;
			}
			else if (distFade < 0.0f)
			{
				distFade = 0.0f;
			}
		}

		// set origin
		VectorCopy(cent->lerpOrigin, origin);

		// project it onto the shadow plane
		if (origin[2] < *shadowPlane)
		{
			origin[2] = *shadowPlane;
		}

		// add a bit of height so foot shadows don't clip into sloped geometry as much
		origin[2] += 18.0f;

		//alpha *= distFade;

		// decal remix
		//CG_ImpactMark( cgs.media.shadowTorsoShader, trace.endpos, trace.plane.normal,
		//  0, alpha,alpha,alpha,1, qfalse, 16, qtrue, -1 );
		CG_ImpactMark(cgs.media.shadowTorsoShader, origin, projection, 18.0f,
		              cent->lerpAngles[YAW], distFade, distFade, distFade, distFade, -1);
		return qtrue;
	}

	if (dist < SHADOW_MAX_DIST)       // show more detail
	{
		vec3_t angles;
		int    tagIndex, subIndex;

		// now add shadows for the various body parts

		for (tagIndex = 0; shadowParts[tagIndex].tagname; tagIndex++)
		{
			// grab each tag with this name
			for (subIndex = 0; (subIndex = CG_GetOriginForTag(cent, &cent->pe.bodyRefEnt, shadowParts[tagIndex].tagname, subIndex, origin, axis)) >= 0; subIndex++)
			{
				// project it onto the shadow plane
				if (origin[2] < *shadowPlane)
				{
					origin[2] = *shadowPlane;
				}

				// add a bit of height so foot shadows don't clip into sloped geometry as much
				origin[2] += 5.0f;

#if 0
				alpha = 1.0 - ((origin[2] - (*shadowPlane + ZOFS)) / shadowParts[tagIndex].maxdist);
				if (alpha < 0)
				{
					continue;
				}
				if (alpha > shadowParts[tagIndex].maxalpha)
				{
					alpha = shadowParts[tagIndex].maxalpha;
				}
				alpha    *= (1.0 - distFade);
				origin[2] = *shadowPlane;
#endif

				AxisToAngles(axis, angles);

				// decal remix
				//CG_ImpactMark( shadowParts[tagIndex].shader, origin, trace.plane.normal,
				//  angles[YAW]/*cent->pe.legs.yawAngle*/, alpha,alpha,alpha,1, qfalse, shadowParts[tagIndex].size, qtrue, -1 );

				//CG_ImpactMark( shadowParts[ tagIndex ].shader, origin, up,
				//  cent->lerpAngles[ YAW ], 1.0f, 1.0f, 1.0f, 1.0f, qfalse, shadowParts[ tagIndex ].size, qtrue, -1 );
				CG_ImpactMark(shadowParts[tagIndex].shader, origin, projection, shadowParts[tagIndex].size,
				              angles[YAW], distFade, distFade, distFade, distFade, -1);
			}
		}
	}

	return qtrue;
}

/**
 * @brief Draw a mark at the water surface
 * @param[in] cent
 */
static void CG_PlayerSplash(centity_t *cent)
{
	vec3_t     start, end;
	trace_t    trace;
	int        contents;
	polyVert_t verts[4];

	if (!cg_shadows.integer)
	{
		return;
	}

	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CG_PointContents(end, 0);
	if (!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)))
	{
		return;
	}

	VectorCopy(cent->lerpOrigin, start);
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents(start, 0);
	if (contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA));

	if (trace.fraction == 1.0f)
	{
		return;
	}

	// create a mark polygon
	VectorCopy(trace.endpos, verts[0].xyz);
	verts[0].xyz[0]     -= 32;
	verts[0].xyz[1]     -= 32;
	verts[0].st[0]       = 0;
	verts[0].st[1]       = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[1].xyz);
	verts[1].xyz[0]     -= 32;
	verts[1].xyz[1]     += 32;
	verts[1].st[0]       = 0;
	verts[1].st[1]       = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[2].xyz);
	verts[2].xyz[0]     += 32;
	verts[2].xyz[1]     += 32;
	verts[2].st[0]       = 1;
	verts[2].st[1]       = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[3].xyz);
	verts[3].xyz[0]     += 32;
	verts[3].xyz[1]     -= 32;
	verts[3].st[0]       = 1;
	verts[3].st[1]       = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.wakeMarkShader, 4, verts);
}

//==========================================================================

/**
 * @brief Adds a piece with modifications or duplications for powerups
 * Also called by CG_Missile for quad rockets, but nobody can tell...
 * @param[in,out] ent
 * @param[in] powerups - unused
 * @param[in] team - unused
 * @param[in] es
 * @param[in] fireRiseDir
 */
void CG_AddRefEntityWithPowerups(refEntity_t *ent, int powerups, int team, entityState_t *es, const vec3_t fireRiseDir)
{
	refEntity_t backupRefEnt; //, parentEnt;
	qboolean    onFire = qfalse;
	float       alpha  = 0.0;

	ent->entityNum = es->number;

	/*  if (cent->pe.forceLOD) {
	        ent->reFlags |= REFLAG_FORCE_LOD;
	    }*/

	backupRefEnt = *ent;

	if (CG_EntOnFire(&cg_entities[es->number]))
	{
		ent->reFlags |= REFLAG_FORCE_LOD;
	}

	trap_R_AddRefEntityToScene(ent);

	// FIXME: onFire is always true !
	if (!onFire && CG_EntOnFire(&cg_entities[es->number]))
	{
		float fireStart, fireEnd;

		onFire = qtrue;
		// set the alpha
		if (ent->entityNum == cg.snap->ps.clientNum)
		{
			fireStart = cg.snap->ps.onFireStart;
			fireEnd   = cg.snap->ps.onFireStart + 1500;
		}
		else
		{
			fireStart = es->onFireStart;
			fireEnd   = es->onFireEnd;
		}

		alpha = (cg.time - fireStart) / 1500.0f;
		if (alpha > 1.0f)
		{
			alpha = (fireEnd - cg.time) / 1500.0f;
			if (alpha > 1.0f)
			{
				alpha = 1.0f;
			}
		}
	}

	if (onFire)
	{
		if (alpha < 0.0f)
		{
			alpha = 0.0f;
		}
		ent->shaderRGBA[3] = ( unsigned char )(255.0f * alpha);
		VectorCopy(fireRiseDir, ent->fireRiseDir);
		if (VectorCompare(ent->fireRiseDir, vec3_origin))
		{
			VectorSet(ent->fireRiseDir, 0, 0, 1);
		}
		ent->customShader = cgs.media.onFireShader;
		trap_R_AddRefEntityToScene(ent);

		ent->customShader = cgs.media.onFireShader2;
		trap_R_AddRefEntityToScene(ent);
	}

	*ent = backupRefEnt;
}

/**
 * @brief Predict, or calculate condition for this entity, if it is not the local client
 * @param[in] character
 * @param[in] cent
 */
void CG_AnimPlayerConditions(bg_character_t *character, centity_t *cent)
{
	entityState_t *es;
	int           legsAnim;

	if (!character)
	{
		return;
	}
	if (cg.snap && cg.snap->ps.clientNum == cent->currentState.number && !cg.renderingThirdPerson)
	{
		return;
	}

	es = &cent->currentState;

	// WEAPON
	if (es->eFlags & EF_ZOOMING)
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_WEAPON, WP_BINOCULARS, qtrue);
	}
	else
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_WEAPON, es->weapon, qtrue);
	}

	// MOUNTED
	if ((es->eFlags & EF_MG42_ACTIVE) || (es->eFlags & EF_MOUNTEDTANK))
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_MOUNTED, MOUNTED_MG42, qtrue);
	}
	else if (es->eFlags & EF_AAGUN_ACTIVE)
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_MOUNTED, MOUNTED_AAGUN, qtrue);
	}
	else
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_MOUNTED, MOUNTED_UNUSED, qtrue);
	}

	// UNDERHAND
	BG_UpdateConditionValue(es->clientNum, ANIM_COND_UNDERHAND, cent->lerpAngles[0] > 0, qtrue);

	if (es->eFlags & EF_CROUCHING)
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_CROUCHING, qtrue, qtrue);
	}
	else
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_CROUCHING, qfalse, qtrue);
	}

	if (es->eFlags & EF_FIRING)
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_FIRING, qtrue, qtrue);
	}
	else
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_FIRING, qfalse, qtrue);
	}

	// reverse engineer the legs anim -> movetype (if possible)
	legsAnim = es->legsAnim & ~ANIM_TOGGLEBIT;
	if (character->animModelInfo->animations[legsAnim]->movetype)
	{
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_MOVETYPE, character->animModelInfo->animations[legsAnim]->movetype, qfalse);
	}

	BG_UpdateConditionValue(es->clientNum, ANIM_COND_IMPACT_POINT, IMPACTPOINT_UNUSED, qtrue);
	BG_UpdateConditionValue(es->clientNum, ANIM_COND_STUNNED, 0, qtrue);
	BG_UpdateConditionValue(es->clientNum, ANIM_COND_SUICIDE, 0, qtrue);
}

/**
 * @brief CG_Player
 * @param[in,out] cent
 */
void CG_Player(centity_t *cent)
{
	clientInfo_t   *ci;
	refEntity_t    body;
	refEntity_t    head;
	refEntity_t    acc;
	vec3_t         playerOrigin = { 0 }, lightorigin = { 0 };
	int            clientNum, i;
	int            renderfx = 0, rank, team;
	qboolean       shadow      = qfalse; // gjd added to make sure it was initialized;
	float          shadowPlane = 0;
	qboolean       usingBinocs = qfalse;
	bg_character_t *character;
	float          hilightIntensity = 0.f;

	// if set to invisible, skip
	if (cent->currentState.eFlags & EF_NODRAW)
	{
		return;
	}

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		CG_Error("Bad clientNum on player entity\n");
	}
	ci = &cgs.clientinfo[clientNum];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if (!ci->infoValid)
	{
		return;
	}

	character = CG_CharacterForClientinfo(ci, cent);

	if (cent->currentState.eFlags & EF_MOUNTEDTANK)
	{
		VectorCopy(cg_entities[cg_entities[cent->currentState.clientNum].tagParent].mountedMG42Player.origin, playerOrigin);
	}
	else if ((cent->currentState.eFlags & EF_MG42_ACTIVE) || (cent->currentState.eFlags & EF_AAGUN_ACTIVE)) // see if we're attached to a gun
	{
		centity_t *mg42;
		int       num;

		// find the mg42 we're attached to
		for (num = 0 ; num < cg.snap->numEntities ; num++)
		{
			mg42 = &cg_entities[cg.snap->entities[num].number];
			if ((mg42->currentState.eType == ET_MG42_BARREL || mg42->currentState.eType == ET_AAGUN) &&
			    mg42->currentState.otherEntityNum == cent->currentState.number)
			{
				// found it, clamp behind gun
				vec3_t forward, right, up;

				//AngleVectors (mg42->s.apos.trBase, forward, right, up);
				AngleVectors(cent->lerpAngles, forward, right, up);

				if (cent->currentState.eFlags & EF_AAGUN_ACTIVE)
				{
					VectorMA(mg42->currentState.pos.trBase, -40, forward, playerOrigin);
				}
				else
				{
					VectorMA(mg42->currentState.pos.trBase, -36, forward, playerOrigin);
				}

				playerOrigin[2] = cent->lerpOrigin[2];
				break;
			}
		}

		if (num == cg.snap->numEntities)
		{
			VectorCopy(cent->lerpOrigin, playerOrigin);
		}
	}
	else
	{
		VectorCopy(cent->lerpOrigin, playerOrigin);
	}

	Com_Memset(&body, 0, sizeof(body));
	Com_Memset(&head, 0, sizeof(head));
	Com_Memset(&acc, 0, sizeof(acc));

	// get the rotation information
	CG_PlayerAngles(cent, body.axis, body.torsoAxis, head.axis);

	// copy the torso rotation to the accessories
	AxisCopy(body.torsoAxis, acc.axis);

	// calculate client-side conditions
	CG_AnimPlayerConditions(character, cent);

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation(cent, &body);

	// forcibly set binoc animation
	if (cent->currentState.eFlags & EF_ZOOMING)
	{
		usingBinocs = qtrue;
	}

	// add the any sprites hovering above the player
	// rain - corpses don't get icons (fireteam check ran out of bounds)
	if (cent->currentState.eType != ET_CORPSE)
	{
		CG_PlayerSprites(cent);
	}

	shadow = CG_PlayerShadow(cent, &shadowPlane);

	// add a water splash if partially in and out of water
	CG_PlayerSplash(cent);

	// get the player model information
	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson)
	{
		renderfx = RF_THIRD_PERSON;         // only draw in mirrors
	}

	// FIXME: buggy
	if (cg_shadows.integer == 3 && shadow)
	{
		renderfx |= RF_SHADOW_PLANE;
	}

	renderfx |= RF_LIGHTING_ORIGIN;         // use the same origin for all

	VectorCopy(playerOrigin, lightorigin);
	lightorigin[2] += 31;

	{
		vec3_t dist;
		vec_t  distSquared;

		VectorSubtract(lightorigin, cg.refdef_current->vieworg, dist);
		distSquared = VectorLengthSquared(dist);
		if (distSquared > Square(384.f))
		{
			renderfx |= RF_MINLIGHT;

			distSquared -= Square(384.f);

			if (distSquared > Square(768.f))
			{
				hilightIntensity = 1.f;
			}
			else
			{
				hilightIntensity = 1.f * (distSquared / Square(768.f));
			}

			//CG_Printf( "%f\n", hilightIntensity );
		}
	}

	body.hilightIntensity = hilightIntensity;
	head.hilightIntensity = hilightIntensity;
	acc.hilightIntensity  = hilightIntensity;

	// set renderfx for all parts
	acc.renderfx  = renderfx;
	body.renderfx = renderfx;
	head.renderfx = renderfx;

	// add the body
	if (cent->currentState.eType == ET_CORPSE && cent->currentState.time2 == 1)
	{
		body.hModel     = character->undressedCorpseModel;
		body.customSkin = character->undressedCorpseSkin;
	}
	else
	{
		body.customSkin = character->skin;
		body.hModel     = character->mesh;
	}

	VectorCopy(playerOrigin, body.origin);
	VectorCopy(lightorigin, body.lightingOrigin);
	body.shadowPlane = shadowPlane;
	VectorCopy(body.origin, body.oldorigin);    // don't positionally lerp at all

	cent->pe.bodyRefEnt = body;

	// if the model failed, allow the default nullmodel to be displayed
	// whoever wrote that comment sucks
	if (!body.hModel)
	{
		return;
	}

	// only need to set this once...
	VectorCopy(lightorigin, acc.lightingOrigin);

	CG_AddRefEntityWithPowerups(&body, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir);

#if 0
	// debug
	{
		int    y;
		vec3_t oldOrigin;

		VectorCopy(body.origin, oldOrigin);
		body.origin[0] -= 20;
		//body.origin[ 0 ] -= 20 * 36;
		for (y = 0; y < 40; y++)
		{
			body.origin[0] += 1;
			//body.origin[ 0 ] += 36;
			//body.origin[ 2 ] = BG_GetGroundHeightAtPoint( body.origin ) + (oldOrigin[2] - BG_GetGroundHeightAtPoint( oldOrigin ));
			body.frame    += (y & 1) ? 1 : -1;
			body.oldframe += (y & 1) ? -1 : 1;
			CG_AddRefEntityWithPowerups(&body, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir);
		}
		VectorCopy(oldOrigin, body.origin);
	}
#endif

	// DEBUG
	if (cg_debugPlayerHitboxes.integer && cent->currentState.eType != ET_CORPSE &&
	    cent->currentState.number == cg.snap->ps.clientNum)
	{
		// position marker
		if (cg_debugPlayerHitboxes.integer & 4)
		{
			int    x, zd, zu;
			vec3_t bmins, bmaxs;

			x  = (cent->currentState.solid & 255);
			zd = ((cent->currentState.solid >> 8) & 255);
			zu = ((cent->currentState.solid >> 16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			VectorAdd(bmins, cent->lerpOrigin, bmins);
			VectorAdd(bmaxs, cent->lerpOrigin, bmaxs);
			// red
			CG_RailTrail(tv(1.0f, 0.0f, 0.0f), bmins, bmaxs, 1, cent->currentState.number | HITBOXBIT_CLIENT);
		}

		// head axis
		if (cg_debugPlayerHitboxes.integer & 2)
		{
			orientation_t tag;
			int           idx;
			vec3_t        start;
			vec3_t        ends[3];
			vec3_t        axis[3];

			trap_R_LerpTag(&tag, &body, "tag_head", 0);

			VectorCopy(body.origin, start);

			for (idx = 0; idx < 3; idx++)
			{
				VectorMA(start, tag.origin[idx], body.axis[idx], start);
			}

			MatrixMultiply(tag.axis, body.axis, axis);

			for (idx = 0; idx < 3; idx++)
			{
				VectorMA(start, 32.0f, axis[idx], ends[idx]);
				// red
				CG_RailTrail2(tv(1.0f, 0.0f, 0.0f), start, ends[idx], -1, -1);
			}
		}

		// hitbox
		if (cg_debugPlayerHitboxes.integer & 1)
		{
			vec3_t mins, maxs, org, forward;

			VectorCopy(cg.predictedPlayerState.mins, mins);
			VectorCopy(cg.predictedPlayerState.maxs, maxs);

			if (cg.predictedPlayerState.eFlags & EF_PRONE)
			{
				maxs[2] = maxs[2] - (cg.predictedPlayerState.standViewHeight - PRONE_BODYHEIGHT + 8);
			}
			else if (cg.predictedPlayerState.pm_flags & PMF_DUCKED
			         && cg.predictedPlayerState.velocity[0] == 0.f && cg.predictedPlayerState.velocity[1] == 0.f)
			{
				maxs[2] = cg.predictedPlayerState.crouchMaxZ + DEFAULT_BODYHEIGHT_DELTA - CROUCH_IDLE_BODYHEIGHT_DELTA;
			}
			else if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
			{
				maxs[2] = cg.predictedPlayerState.crouchMaxZ;
			}
			else if (cg.predictedPlayerState.eFlags & EF_DEAD)
			{
				maxs[2] = cg.predictedPlayerState.deadViewHeight + DEAD_BODYHEIGHT_DELTA;
			}
			else
			{
				maxs[2] = cg.predictedPlayerState.standViewHeight + DEFAULT_BODYHEIGHT_DELTA;
			}

			VectorAdd(cent->lerpOrigin, mins, mins);
			VectorAdd(cent->lerpOrigin, maxs, maxs);
			// red
			CG_RailTrail(tv(1.0f, 0.0f, 0.0f), mins, maxs, 1, cent->currentState.number | HITBOXBIT_CLIENT);

			// head and legs
			if (cg.predictedPlayerState.eFlags & (EF_PRONE | EF_DEAD))
			{
				// legs
				VectorCopy(playerlegsProneMins, mins);
				VectorCopy(playerlegsProneMaxs, maxs);

				AngleVectors(cent->lerpAngles, forward, NULL, NULL);
				forward[2] = 0;
				VectorNormalizeFast(forward);

				if (cg.predictedPlayerState.eFlags & EF_PRONE)
				{
					org[0] = cent->lerpOrigin[0] + forward[0] * -24;
					org[1] = cent->lerpOrigin[1] + forward[1] * -24;
					org[2] = cent->lerpOrigin[2] + cg.pmext.proneLegsOffset;
				}
				else // EF_DEAD
				{
					org[0] = cent->lerpOrigin[0] + forward[0] * 32;
					org[1] = cent->lerpOrigin[1] + forward[1] * 32;
					org[2] = cent->lerpOrigin[2] - cg.pmext.proneLegsOffset;
				}

				VectorAdd(org, mins, mins);
				VectorAdd(org, maxs, maxs);
				// red
				CG_RailTrail(tv(1.0f, 0.0f, 0.0f), mins, maxs, 1, cent->currentState.number | HITBOXBIT_CLIENT | HITBOXBIT_LEGS);

				// head
				VectorSet(mins, -6, -6, -22);
				VectorSet(maxs, 6, 6, -10);

				if (cg.predictedPlayerState.eFlags & EF_PRONE)
				{
					org[0] = cent->lerpOrigin[0] + forward[0] * 24;
					org[1] = cent->lerpOrigin[1] + forward[1] * 24;
					org[2] = cent->lerpOrigin[2] + 8;
				}
				else // EF_DEAD
				{
					org[0] = cent->lerpOrigin[0] + forward[0] * -32;
					org[1] = cent->lerpOrigin[1] + forward[1] * -32;
					org[2] = cent->lerpOrigin[2] - 4;
				}

				VectorAdd(org, mins, mins);
				VectorAdd(org, maxs, maxs);
				// red
				CG_RailTrail(tv(1.0f, 0.0f, 0.0f), mins, maxs, 1, cent->currentState.number | HITBOXBIT_CLIENT | HITBOXBIT_HEAD);
			}
			else
			{
				org[0] = cent->lerpOrigin[0];
				org[1] = cent->lerpOrigin[1];
				org[2] = maxs[2] + 6;

				// head
				VectorSet(mins, -6, -6, -6);
				VectorSet(maxs, 6, 6, 6);

				VectorAdd(org, mins, mins);
				VectorAdd(org, maxs, maxs);

				// red
				CG_RailTrail(tv(1.0f, 0.0f, 0.0f), mins, maxs, 1, cent->currentState.number | HITBOXBIT_CLIENT | HITBOXBIT_HEAD);
			}
		}
	} // END DEBUG

	// add the head
	if (!(head.hModel = character->hudhead))
	{
		return;
	}
	head.customSkin = character->hudheadskin;

	VectorCopy(lightorigin, head.lightingOrigin);

	CG_PositionRotatedEntityOnTag(&head, &body, "tag_head");

	head.shadowPlane = shadowPlane;

	if (cent->currentState.eFlags & EF_FIRING)
	{
		cent->pe.lastFiredWeaponTime = 0;
		cent->pe.weaponFireTime     += cg.frametime;
	}
	else
	{
		if (cent->pe.weaponFireTime > 500 /*&& cent->pe.weaponFireTime*/)
		{
			cent->pe.lastFiredWeaponTime = cg.time;
		}

		cent->pe.weaponFireTime = 0;
	}

	if (cent->currentState.eType != ET_CORPSE && !(cent->currentState.eFlags & EF_DEAD))
	{
		hudHeadAnimNumber_t anim;

		if (cent->pe.weaponFireTime > 500)
		{
			anim = HD_ATTACK;
		}
		else if (cg.time - cent->pe.lastFiredWeaponTime < 500)
		{
			anim = HD_ATTACK_END;
		}
		else
		{
			anim = HD_IDLE1;
		}

		CG_HudHeadAnimation(character, &cent->pe.head, &head.oldframe, &head.frame, &head.backlerp, anim);
	}
	else
	{
		head.frame    = 0;
		head.oldframe = 0;
		head.backlerp = 0.f;
	}

	// set blinking flag
	CG_AddRefEntityWithPowerups(&head, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir);

	cent->pe.headRefEnt = head;

	// set the shadowplane for accessories
	acc.shadowPlane = shadowPlane;

	CG_BreathPuffs(cent, &head);

	// add the gun / barrel / flash
	if (!(cent->currentState.eFlags & EF_DEAD) /*&& !usingBinocs*/)
	{
		if ((cent->currentState.eFlags & EF_TALK)
		    && (cgs.clientinfo[cent->currentState.clientNum].weaponState == WSTATE_IDLE)
		    && !(cent->currentState.eFlags & (EF_FIRING | EF_MOUNTEDTANK | EF_ZOOMING))
		    && !(cent->pe.torso.animation->flags & (ANIMFL_LADDERANIM | ANIMFL_FIRINGANIM | ANIMFL_RELOADINGANIM))
		    && !(GetWeaponTableData(cent->currentState.weapon)->type & (WEAPON_TYPE_SET | WEAPON_TYPE_SCOPED)))
		{
			int contents;

			contents = CG_PointContents(head.origin, 0);

			// don't attach radio on hand when player is underwater
			if (!(contents & CONTENTS_WATER))
			{
				acc.hModel = cg_weapons[WP_SATCHEL_DET].weaponModel[W_TP_MODEL].model;
				CG_PositionEntityOnTag(&acc, &body, "tag_weapon", 0, NULL);
				CG_AddRefEntityWithPowerups(&acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir);
			}
			else
			{
				CG_AddPlayerWeapon(&body, NULL, cent);
			}
		}
		else
		{
			CG_AddPlayerWeapon(&body, NULL, cent);
		}

		CG_AddSoundWeapon(cent);
	}

	// add binoculars (if it's not the player)
	if (usingBinocs)
	{
		acc.hModel = cgs.media.thirdPersonBinocModel;
		CG_PositionEntityOnTag(&acc, &body, "tag_weapon", 0, NULL);
		CG_AddRefEntityWithPowerups(&acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir);
	}

	// adjust rank for disguised player
	if (cent->currentState.powerups & (1 << PW_OPS_DISGUISED))
	{
		rank = cgs.clientinfo[cgs.clientinfo[cent->currentState.number].disguiseClientNum].rank;
		team = ci->team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
	}
	else
	{
		rank = ci->rank;
		team = ci->team;
	}

	// add accessories
	for (i = ACC_BELT_LEFT; i < ACC_MAX; i++)
	{
		if (!(character->accModels[i]))
		{
			continue;
		}
		acc.hModel     = character->accModels[i];
		acc.customSkin = character->accSkins[i];

		// looted corpses dont have any accsserories, evil looters :E
		if (!(cent->currentState.eType == ET_CORPSE && cent->currentState.time2 == 1))
		{
			switch (i)
			{
			case ACC_BELT_LEFT:
				CG_PositionEntityOnTag(&acc, &body, "tag_bright", 0, NULL);
				break;
			case ACC_BELT_RIGHT:
				CG_PositionEntityOnTag(&acc, &body, "tag_bleft", 0, NULL);
				break;
			case ACC_BELT:
				CG_PositionEntityOnTag(&acc, &body, "tag_ubelt", 0, NULL);
				break;
			case ACC_BACK:
				CG_PositionEntityOnTag(&acc, &body, "tag_back", 0, NULL);
				break;
			case ACC_HAT:               // hat
			case ACC_RANK:
				if (cent->currentState.eFlags & EF_HEADSHOT)
				{
					continue;
				}
				if (i == ACC_RANK)
				{
					if (rank <= 0)
					{
						continue;
					}
					acc.customShader = rankicons[rank][team == TEAM_AXIS ? 1 : 0][1].shader;
				}
				CG_PositionEntityOnTag(&acc, &head, "tag_mouth", 0, NULL);
				break;
			case ACC_MOUTH2:            // hat2
			case ACC_MOUTH3:            // hat3
				CG_PositionEntityOnTag(&acc, &head, "tag_mouth", 0, NULL);
				break;
			// weapon and weapon2
			// these are used by characters who have permanent weapons attached to their character in the skin
			case ACC_WEAPON:        // weap
				CG_PositionEntityOnTag(&acc, &body, "tag_weapon", 0, NULL);
				break;
			case ACC_WEAPON2:       // weap2
				CG_PositionEntityOnTag(&acc, &body, "tag_weapon2", 0, NULL);
				break;
			default:
				continue;
			}

			CG_AddRefEntityWithPowerups(&acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir);
		}
	}
}

//=====================================================================

/**
 * @brief A player just came into view or teleported, so reset all animation info
 * @param[in,out] cent
 */
void CG_ResetPlayerEntity(centity_t *cent)
{
	if (!(cent->currentState.eFlags & EF_DEAD))
	{
		CG_ClearLerpFrameRate(cent, &cgs.clientinfo[cent->currentState.clientNum], &cent->pe.legs, cent->currentState.legsAnim);
		CG_ClearLerpFrame(cent, &cgs.clientinfo[cent->currentState.clientNum], &cent->pe.torso, cent->currentState.torsoAnim);

		Com_Memset(&cent->pe.legs, 0, sizeof(cent->pe.legs));
		cent->pe.legs.yawAngle   = cent->rawAngles[YAW];
		cent->pe.legs.yawing     = qfalse;
		cent->pe.legs.pitchAngle = 0;
		cent->pe.legs.pitching   = qfalse;

		Com_Memset(&cent->pe.torso, 0, sizeof(cent->pe.torso));
		cent->pe.torso.yawAngle   = cent->rawAngles[YAW];
		cent->pe.torso.yawing     = qfalse;
		cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
		cent->pe.torso.pitching   = qfalse;
	}

	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin, qfalse, cent->currentState.effect2Time);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles, qtrue, cent->currentState.effect2Time);

	VectorCopy(cent->lerpOrigin, cent->rawOrigin);
	VectorCopy(cent->lerpAngles, cent->rawAngles);

	if (cg_debugPosition.integer)
	{
		CG_Printf("%i ResetPlayerEntity yaw=%f\n", cent->currentState.number, (double)cent->pe.torso.yawAngle);
	}

	cent->pe.painAnimLegs  = -1;
	cent->pe.painAnimTorso = -1;
	cent->pe.animSpeed     = 1.0;
}

/**
 * @brief CG_GetBleedOrigin
 * @param[in] head_origin
 * @param[in] body_origin
 * @param[in] fleshEntityNum
 */
void CG_GetBleedOrigin(vec3_t head_origin, vec3_t body_origin, int fleshEntityNum)
{
	clientInfo_t   *ci = &cgs.clientinfo[fleshEntityNum];
	refEntity_t    body;
	refEntity_t    head;
	centity_t      *cent, backupCent;
	bg_character_t *character;

	if (!ci->infoValid)
	{
		return;
	}

	character = CG_CharacterForClientinfo(ci, NULL);

	cent       = &cg_entities[fleshEntityNum];
	backupCent = *cent;

	Com_Memset(&body, 0, sizeof(body));
	Com_Memset(&head, 0, sizeof(head));

	CG_PlayerAngles(cent, body.axis, body.torsoAxis, head.axis);
	CG_PlayerAnimation(cent, &body);

	body.hModel = character->mesh;
	if (!body.hModel)
	{
		return;
	}

	head.hModel = character->hudhead;
	if (!head.hModel)
	{
		CG_Printf("Warning: CG_GetBleedOrigin w/o model.\n");
		return;
	}

	VectorCopy(cent->lerpOrigin, body.origin);
	VectorCopy(body.origin, body.oldorigin);

	// Ridah, restore the cent so we don't interfere with animation timings
	*cent = backupCent;

	CG_PositionRotatedEntityOnTag(&head, &body, "tag_head");

	VectorCopy(head.origin, head_origin);
	VectorCopy(body.origin, body_origin);
}

/**
 * @brief CG_GetTag
 * @param[in] clientNum
 * @param[in] tagname
 * @param[in,out] orientation
 * @return
 */
qboolean CG_GetTag(int clientNum, const char *tagname, orientation_t *orientation)
{
	clientInfo_t *ci = &cgs.clientinfo[clientNum];
	centity_t    *cent;
	refEntity_t  *refent;
	vec3_t       tempAxis[3];
	vec3_t       org;
	int          i;

	if (cg.snap && clientNum == cg.snap->ps.clientNum && cg.renderingThirdPerson)
	{
		cent = &cg.predictedPlayerEntity;
	}
	else
	{
		cent = &cg_entities[ci->clientNum];
		if (!cent->currentValid)
		{
			return qfalse;      // not currently in PVS
		}
	}

	refent = &cent->pe.bodyRefEnt;

	if (trap_R_LerpTag(orientation, refent, tagname, 0) < 0)
	{
		return qfalse;
	}

	VectorCopy(refent->origin, org);

	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(org, orientation->origin[i], refent->axis[i], org);
	}

	VectorCopy(org, orientation->origin);

	// rotate with entity
	MatrixMultiply(refent->axis, orientation->axis, tempAxis);
	Com_Memcpy(orientation->axis, tempAxis, sizeof(vec3_t) * 3);

	return qtrue;
}

/**
 * @brief CG_GetWeaponTag
 * @param[in] clientNum
 * @param[in] tagname
 * @param[in,out] orientation
 * @return
 */
qboolean CG_GetWeaponTag(int clientNum, const char *tagname, orientation_t *orientation)
{
	clientInfo_t *ci = &cgs.clientinfo[clientNum];
	centity_t    *cent;
	refEntity_t  *refent;
	vec3_t       tempAxis[3];
	vec3_t       org;
	int          i;

	if (cg.snap && clientNum == cg.snap->ps.clientNum && cg.renderingThirdPerson)
	{
		cent = &cg.predictedPlayerEntity;
	}
	else
	{
		cent = &cg_entities[ci->clientNum];
		if (!cent->currentValid)
		{
			return qfalse;      // not currently in PVS
		}
	}

	if (cent->pe.gunRefEntFrame < cg.clientFrame - 1)
	{
		return qfalse;
	}

	refent = &cent->pe.gunRefEnt;

	if (trap_R_LerpTag(orientation, refent, tagname, 0) < 0)
	{
		return qfalse;
	}

	VectorCopy(refent->origin, org);

	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(org, orientation->origin[i], refent->axis[i], org);
	}

	VectorCopy(org, orientation->origin);

	// rotate with entity
	MatrixMultiply(refent->axis, orientation->axis, tempAxis);
	Com_Memcpy(orientation->axis, tempAxis, sizeof(vec3_t) * 3);

	return qtrue;
}

/**
 * @brief CG_SetHudHeadLerpFrameAnimation
 * @param[in] ch
 * @param[in,out] lf
 * @param[in] newAnimation
 */
void CG_SetHudHeadLerpFrameAnimation(bg_character_t *ch, lerpFrame_t *lf, int newAnimation)
{
	animation_t *anim;

	lf->animationNumber = newAnimation;
	newAnimation       &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= MAX_HD_ANIMATIONS)
	{
		CG_Error("Bad animation number (CG_SetHudHeadLerpFrameAnimation): %i\n", newAnimation);
	}

	anim = &ch->hudheadanimations[newAnimation];

	lf->animation     = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
}

/**
 * @brief CG_ClearHudHeadLerpFrame
 * @param[in] ch
 * @param[in,out] lf
 * @param animationNumber
 */
void CG_ClearHudHeadLerpFrame(bg_character_t *ch, lerpFrame_t *lf, int animationNumber)
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetHudHeadLerpFrameAnimation(ch, lf, animationNumber);
	lf->oldFrame      = lf->frame = lf->animation->firstFrame;
	lf->oldFrameModel = lf->frameModel = lf->animation->mdxFile;
}

/**
 * @brief CG_RunHudHeadLerpFrame
 * @param[in] ch
 * @param[in,out] lf
 * @param[in] newAnimation
 * @param[in] speedScale
 */
void CG_RunHudHeadLerpFrame(bg_character_t *ch, lerpFrame_t *lf, int newAnimation, float speedScale)
{
	// see if the animation sequence is switching
	if (!lf->animation)
	{
		CG_ClearHudHeadLerpFrame(ch, lf, newAnimation);
	}
	else if (newAnimation != lf->animationNumber)
	{
		CG_SetHudHeadLerpFrameAnimation(ch, lf, newAnimation);
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime)
	{
		int         f;
		animation_t *anim;

		lf->oldFrame      = lf->frame;
		lf->oldFrameTime  = lf->frameTime;
		lf->oldFrameModel = lf->frameModel;

		// get the next frame based on the animation
		anim = lf->animation;
		if (!anim || !anim->frameLerp)
		{
			CG_Printf("Warning: CG_RunHudHeadLerpFrame w/o animation.\n");
			return;     // shouldn't happen
		}
		if (cg.time < lf->animationTime)
		{
			lf->frameTime = lf->animationTime;      // initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f  = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale;        // adjust for haste, etc
		if (f >= anim->numFrames)
		{
			f -= anim->numFrames;
			if (anim->loopFrames)
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame      = anim->firstFrame + f;
		lf->frameModel = anim->mdxFile;
		if (cg.time > lf->frameTime)
		{
			lf->frameTime = cg.time;
		}
	}

	if (lf->frameTime > cg.time + 200)
	{
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time)
	{
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime)
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0f - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

/**
 * @brief CG_HudHeadAnimation
 * @param[in] ch
 * @param[in] lf
 * @param[out] oldframe
 * @param[out] frame
 * @param[out] backlerp
 * @param[in] animation
 */
void CG_HudHeadAnimation(bg_character_t *ch, lerpFrame_t *lf, int *oldframe, int *frame, float *backlerp, hudHeadAnimNumber_t animation)
{
	CG_RunHudHeadLerpFrame(ch, lf, (int)animation, 1.f);

	*oldframe = lf->oldFrame;
	*frame    = lf->frame;
	*backlerp = lf->backlerp;
}

/**
* @brief Get player max health
* @param[in] clientNum
* @param[in] class
*/
int CG_GetPlayerMaxHealth(int clientNum, int class, int team)
{
	int i;
	int maxHealth = 100;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!cgs.clientinfo[i].infoValid)
		{
			continue;
		}

		if (cgs.clientinfo[i].team != team)
		{
			continue;
		}

		if (cgs.clientinfo[i].cls != PC_MEDIC)
		{
			continue;
		}

		maxHealth += 10;

		if (maxHealth >= 125)
		{
			maxHealth = 125;
			break;
		}
	}

	if (cgs.clientinfo[clientNum].skill[SK_BATTLE_SENSE] >= 3)
	{
		maxHealth += 15;
	}

	if (class == PC_MEDIC)
	{
		maxHealth *= 1.12f;
	}

	return maxHealth;
}
