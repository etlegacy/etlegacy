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
 * @file cg_popupmessages.c
 */

#include "cg_local.h"

#define NUM_PM_STACK           3
#define NUM_PM_STACK_ITEMS     32
#define NUM_PM_STACK_ITEMS_BIG 3 // we shouldn't need many of these
#define NUM_PM_STACK_ITEMS_XP  32

typedef struct pmStackItem_s pmListItem_t;

struct pmStackItem_s
{
	int type;
	qboolean inuse;
	int time;
	char message[128];
	char message2[128];
	qhandle_t shader;
	qhandle_t weaponShader;
	int scaleShader;

	vec3_t color;
	int count;

	pmListItem_t *next;
	pmListItem_t *prev;
};

pmListItem_t cg_pmStack[NUM_PM_STACK][NUM_PM_STACK_ITEMS];
pmListItem_t *cg_pmOldList[NUM_PM_STACK];
pmListItem_t *cg_pmWaitingList[NUM_PM_STACK];

pmListItem_t *cg_pmWaitingListBig;
pmListItem_t cg_pmStackBig[NUM_PM_STACK_ITEMS_BIG];

pmListItem_t *cg_pmOldListXP;
pmListItem_t *cg_pmWaitingListXP;
pmListItem_t cg_pmStackXP[NUM_PM_STACK_ITEMS_XP];

const char *cg_skillRewards[SK_NUM_SKILLS][NUM_SKILL_LEVELS - 1] =
{
	{ "Binoculars for any Class",                 "Improved Physical Fitness",                 "Improved Health",                       "Trap Awareness"           }, // battle sense
	{ "Improved use of Explosive Ammunition",     "Improved Dexterity",                        "Improved Construction and Destruction", "a Flak Jacket"            }, // explosives & construction
	{ "Medic Ammo",                               "Improved First Aid Resources",              "Full Revive",                           "Adrenalin Self"           }, // first aid
	{ "Improved Ammo Resources",                  "Improved Signals",                          "Improved Air and Ground Support",       "Enemy Recognition"        }, // signals
	{ "Improved use of Light Weapon Ammunition",  "Faster Reload",                             "Improved Light Weapon Handling",        "Dual-Wield Pistols"       }, // light weapons
	{ "Improved Projectile Resources",            "Heavy Weapon Proficiency",                  "Improved Dexterity",                    "Improved Weapon Handling" }, // heavy weapons
	{ "Improved use of Scoped Weapon Ammunition", "Improved use of Sabotage and Misdirection", "Breath Control",                        "Assassin"                 } // scoped weapons & military intelligence
};

void CG_PMItemBigSound(pmListItem_t *item);

/**
 * @brief CG_InitPMGraphics
 */
void CG_InitPMGraphics(void)
{
	cgs.media.pmImages[PM_DYNAMITE]     = trap_R_RegisterShaderNoMip("gfx/limbo/pm_dynamite");
	cgs.media.pmImages[PM_CONSTRUCTION] = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_MINES]        = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_DEATH]        = trap_R_RegisterShaderNoMip("gfx/hud/pm_death");
	cgs.media.pmImages[PM_MESSAGE]      = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_OBJECTIVE]    = trap_R_RegisterShaderNoMip("sprites/objective");
	cgs.media.pmImages[PM_DESTRUCTION]  = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_TEAM]         = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_AMMOPICKUP]   = trap_R_RegisterShaderNoMip("gfx/limbo/filter_healthammo");
	cgs.media.pmImages[PM_HEALTHPICKUP] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_healthammo");
	cgs.media.pmImages[PM_WEAPONPICKUP] = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_CONNECT]      = trap_R_RegisterShaderNoMip("sprites/voiceChat");

	cgs.media.pmImageAlliesConstruct = trap_R_RegisterShaderNoMip("gfx/hud/pm_constallied");
	cgs.media.pmImageAxisConstruct   = trap_R_RegisterShaderNoMip("gfx/hud/pm_constaxis");
	cgs.media.pmImageAlliesMine      = trap_R_RegisterShaderNoMip("gfx/hud/pm_mineallied");
	cgs.media.pmImageAxisMine        = trap_R_RegisterShaderNoMip("gfx/hud/pm_mineaxis");
	cgs.media.pmImageAlliesFlag      = trap_R_RegisterShaderNoMip("gfx/limbo/pm_flagallied");
	cgs.media.pmImageAxisFlag        = trap_R_RegisterShaderNoMip("gfx/limbo/pm_flagaxis");
	cgs.media.pmImageSpecFlag        = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.hintKey                = trap_R_RegisterShaderNoMip("gfx/hud/keyboardkey_old");

	// extra obituaries
	cgs.media.pmImageSlime = trap_R_RegisterShaderNoMip("gfx/hud/pm_slime");
	cgs.media.pmImageLava  = trap_R_RegisterShaderNoMip("gfx/hud/pm_lava");
	cgs.media.pmImageCrush = trap_R_RegisterShaderNoMip("gfx/hud/pm_crush");
	cgs.media.pmImageShove = trap_R_RegisterShaderNoMip("gfx/hud/pm_shove");
	cgs.media.pmImageFall  = trap_R_RegisterShaderNoMip("gfx/hud/pm_falldown");
}

/**
 * @brief CG_InitPM
 */
void CG_InitPM(void)
{
	int i;

	for (i = 0; i < NUM_PM_STACK; ++i)
	{
		Com_Memset(&cg_pmStack[i], 0, sizeof(cg_pmStack[i]));
		cg_pmOldList[i]     = NULL;
		cg_pmWaitingList[i] = NULL;
	}

	Com_Memset(&cg_pmStackBig, 0, sizeof(cg_pmStackBig));
	cg_pmWaitingListBig = NULL;

	Com_Memset(&cg_pmStackXP, 0, sizeof(cg_pmStackXP));
	cg_pmOldListXP     = NULL;
	cg_pmWaitingListXP = NULL;
}


/*
* These have been replaced by cvars
* #define PM_FADETIME 2500
* #define PM_WAITTIME 2000
* #define PM_POPUP_TIME 1000
*/

#define PM_BIGPOPUP_TIME 2500

/**
 * @brief CG_AddToListFront
 * @param[in,out] list
 * @param[in,out] item
 */
void CG_AddToListFront(pmListItem_t **list, pmListItem_t *item)
{
	item->next = *list;
	*list      = item;
}

/**
 * @brief CG_UpdatePMList
 * @param[in] waitingList
 * @param[in] oldList
 * @param[in] time
 * @param[in] stayTime
 * @param[in] fadeTime
 */
void CG_UpdatePMList(pmListItem_t **waitingList, pmListItem_t **oldList, int time, int stayTime, int fadeTime)
{
	pmListItem_t *listItem = waitingList ? *waitingList : NULL;
	pmListItem_t *lastItem = NULL;

	if (listItem)
	{
		int t = listItem->time + time;

		if (cg.time > t)
		{
			if (listItem->next)
			{
				// there's another item waiting to come on, so move to old list
				*waitingList         = listItem->next;
				(*waitingList)->time = cg.time; // set time we popped up at

				if (oldList)
				{
					CG_AddToListFront(oldList, listItem);
				}
				else
				{
					// TODO: for now only rank/skill up use shorter PM List
					// so let abuse of it while it is alone
					CG_PMItemBigSound(*waitingList);

					listItem->inuse = qfalse;
					listItem->next  = NULL;
				}
			}
			else
			{
				if (cg.time > t + stayTime + fadeTime)
				{
					// we're gone completely
					*waitingList    = NULL;
					listItem->inuse = qfalse;
					listItem->next  = NULL;
				}
				//else
				//{ // just sit where we are, no pressure to do anything...
				//}
			}
		}
	}

	listItem = oldList ? *oldList : NULL;

	while (listItem)
	{
		int t = listItem->time + stayTime + fadeTime + time;

		if (cg.time > t)
		{
			// nuke this, and everything below it (though there shouldn't BE anything below us anyway)
			pmListItem_t *next;

			if (!lastItem)
			{
				// we're the top of the old list, so set to NULL
				*oldList = NULL;
			}
			else
			{
				lastItem->next = NULL;
			}

			do
			{
				next = listItem->next;

				listItem->next  = NULL;
				listItem->inuse = qfalse;
			}
			while ((listItem = next));

			break;
		}

		lastItem = listItem;
		listItem = listItem->next;
	}
}

/**
 * @brief CG_UpdatePMLists
 */
void CG_UpdatePMLists(void)
{
	int i;

	for (i = 0; i < NUM_PM_STACK; ++i)
	{
		CG_UpdatePMList(&cg_pmWaitingList[i], &cg_pmOldList[i], cg_popupTime.integer, cg_popupStayTime.integer, cg_popupFadeTime.integer);
	}

	CG_UpdatePMList(&cg_pmWaitingListXP, &cg_pmOldListXP, cg_popupXPGainTime.integer, cg_popupXPGainStayTime.integer, cg_popupXPGainFadeTime.integer);
	CG_UpdatePMList(&cg_pmWaitingListBig, NULL, PM_BIGPOPUP_TIME, cg_popupStayTime.integer, cg_popupFadeTime.integer);   // TODO: cvar popup BIG ?
}

/**
 * @brief CG_FindFreePMItem
 * @param[in] waitingList
 * @param[in] oldList
 * @param[in] PMListSize
 * @return
 */
pmListItem_t *CG_FindFreePMItem(pmListItem_t stack[], pmListItem_t **oldList, int PMListSize)
{
	int i = 0;

	for ( ; i < PMListSize; i++)
	{
		if (!stack[i].inuse)
		{
			return &stack[i];
		}
	}

	// no totally free items, so just grab the last item in the oldlist
	if (oldList && *oldList)
	{
		pmListItem_t *listItem = *oldList;
		pmListItem_t *lastItem = *oldList;

		while (listItem->next)
		{
			lastItem = listItem;
			listItem = listItem->next;
		}

		if (lastItem == *oldList)
		{
			*oldList = NULL;
		}
		else
		{
			lastItem->next = NULL;
		}

		listItem->inuse = qfalse;

		return listItem;
	}

	// there is no old list... PANIC!
	return NULL;
}

/**
 * @brief CG_CheckPMItemFilter
 * @param[in] type
 * @return
 */
qboolean CG_CheckPMItemFilter(popupMessageType_t type, int filter)
{
	switch (type)
	{
	case PM_CONNECT:
		return filter & POPUP_FILTER_CONNECT;
	case PM_TEAM:
		return filter & POPUP_FILTER_TEAMJOIN;
	case PM_MESSAGE:
	case PM_DYNAMITE:
	case PM_CONSTRUCTION:
	case PM_MINES:
	case PM_OBJECTIVE:
	case PM_DESTRUCTION:
		return filter & POPUP_FILTER_MISSION;
	case PM_AMMOPICKUP:
	case PM_HEALTHPICKUP:
	case PM_WEAPONPICKUP:
		return filter & POPUP_FILTER_PICKUP;
	case PM_DEATH:
		return filter & POPUP_FILTER_DEATH;
	default:
		break;
	}
	return qfalse;
}

/**
 * @brief CG_AddPMItemEx
 * @param type
 * @param message
 * @param message2
 * @param shader
 * @param weaponShader
 * @param scaleShader
 * @param color
 * @param stackNum
 */
void CG_AddPMItemEx(popupMessageType_t type, const char *message, const char *message2, qhandle_t shader, qhandle_t weaponShader, int scaleShader, vec3_t color, int stackNum)
{
	pmListItem_t   *listItem;
	char           *end;
	hudComponent_t *pmComp = (hudComponent_t *)((byte *)&CG_GetActiveHUD()->popupmessages + stackNum * sizeof(hudComponent_t));

	if (!message || !*message)
	{
		return;
	}

	if (type >= PM_NUM_TYPES)
	{
		CG_Printf("Invalid popup type: %d\n", type);
		return;
	}

	if (!pmComp->visible || CG_CheckPMItemFilter(type, pmComp->style))
	{
		return;
	}

	listItem = CG_FindFreePMItem(cg_pmStack[stackNum], &cg_pmOldList[stackNum], NUM_PM_STACK_ITEMS);

	if (!listItem)
	{
		return;
	}

	if (shader)
	{
		listItem->shader = shader;
	}
	else
	{
		listItem->shader = -1;
	}

	if (message2)
	{
		listItem->weaponShader = weaponShader;
		listItem->scaleShader  = scaleShader;
	}
	else
	{
		listItem->weaponShader = -1;
	}

	// colored obituaries
	VectorCopy(color != NULL ? color : colorWhite, listItem->color);

	listItem->inuse = qtrue;
	listItem->type  = type;
	Q_strncpyz(listItem->message, message, sizeof(cg_pmStack[stackNum][0].message));

	// print and THEN chop off the newline, as the console deals with newlines perfectly
	if (listItem->message[strlen(listItem->message) - 1] == '\n')
	{
		listItem->message[strlen(listItem->message) - 1] = 0;
	}

	// do not write obituary popups into console - we'll get double kill-messages otherwise
	if (type != PM_DEATH)
	{
		trap_Print(va("%s\n", listItem->message)); // FIXME: translate this (does it makes sense?)
	}

	// chop off the newline at the end if any
	while ((end = strchr(listItem->message, '\n')))
	{
		*end = '\0';
	}
	// don't eat popups for empty lines
	if (*listItem->message == '\0')
	{
		return;
	}

	if (message2)
	{
		Q_strncpyz(listItem->message2, message2, sizeof(cg_pmStack[stackNum][0].message2));

		if (listItem->message[strlen(listItem->message2) - 1] == '\n')
		{
			listItem->message[strlen(listItem->message2) - 1] = 0;
		}

		while ((end = strchr(listItem->message2, '\n')))
		{
			*end = '\0';
		}

		if (*listItem->message2 == '\0')
		{
			return;
		}
	}

	if (!cg_pmWaitingList[stackNum])
	{
		cg_pmWaitingList[stackNum] = listItem;
		listItem->time             = cg.time;
	}
	else
	{
		pmListItem_t *loop = cg_pmWaitingList[stackNum];

		while (loop->next)
		{
			loop = loop->next;
		}

		loop->next = listItem;
	}
}

/**
 * @brief CG_AddPMItem
 * @param[in] type
 * @param[in] message
 * @param[in] message2
 * @param[in] shader
 * @param[in] weaponShader
 * @param[in] scaleShader
 * @param[in] color
 */
void CG_AddPMItem(popupMessageType_t type, const char *message, const char *message2, qhandle_t shader, qhandle_t weaponShader, int scaleShader, vec3_t color)
{
	int i;

	for (i = 0; i < NUM_PM_STACK; ++i)
	{
		CG_AddPMItemEx(type, message, message2, shader, weaponShader, scaleShader, color, i);
	}
}

/**
 * @brief CG_PMItemBigSound
 * @param item
 */
void CG_PMItemBigSound(pmListItem_t *item)
{
	if (!cg.snap)
	{
		return;
	}

	// TODO: handle case missing ?
	switch (item->type)
	{
	case PM_RANK:
#ifdef FEATURE_PRESTIGE
	case PM_PRESTIGE:
#endif
		trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.sndRankUp);
		break;
	case PM_SKILL:
		trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.sndSkillUp);
		break;
	default:
		break;
	}
}

/**
 * @brief CG_AddPMItemBig
 * @param[in] type
 * @param[in] message
 * @param[in] shader
 */
void CG_AddPMItemBig(popupMessageBigType_t type, const char *message, qhandle_t shader)
{
	pmListItem_t *listItem;

	listItem = CG_FindFreePMItem(cg_pmStackBig, NULL, NUM_PM_STACK_ITEMS_BIG);

	if (!listItem)
	{
		return;
	}

	if (shader)
	{
		listItem->shader = shader;
	}
	else
	{
		listItem->shader = cgs.media.pmImages[type];
	}

	listItem->inuse = qtrue;
	listItem->type  = type;
	listItem->next  = NULL;
	Q_strncpyz(listItem->message, message, sizeof(cg_pmStackBig[0].message));

	if (!cg_pmWaitingListBig)
	{
		cg_pmWaitingListBig = listItem;
		listItem->time      = cg.time;

		CG_PMItemBigSound(listItem);
	}
	else
	{
		pmListItem_t *loop = cg_pmWaitingListBig;

		while (loop->next)
		{
			loop = loop->next;
		}

		loop->next = listItem;
	}
}

/**
 * @brief CG_AddPMItemXP
 * @param[in] type
 * @param[in] message
 * @param[in] message2
 * @param[in] shader
 */
void CG_AddPMItemXP(popupMessageXPGainType_t type, const char *message, const char *message2, qhandle_t shader)
{
	pmListItem_t *listItem = NULL;
	char         *end;
	qboolean     forceStackingXp;

	if (!message || !*message)
	{
		return;
	}

	if (type >= PM_XPGAIN_NUM_TYPES)
	{
		CG_Printf("Invalid XP gain popup type: %d\n", type);
		return;
	}

	// force stacking XP message values for certain XP gain
	forceStackingXp = !Q_stricmp(message2, "constructing") || !Q_stricmp(message2, "repairing");

	// force stacking XP only if we are repairing or constructing something
	if (!(CG_GetActiveHUD()->xpgain.style & POPUP_XPGAIN_NO_STACK)
	    || forceStackingXp)
	{
		if (cg_pmWaitingListXP)
		{
			listItem = cg_pmWaitingListXP;
		}
		else if (cg_pmOldListXP)
		{
			listItem = cg_pmOldListXP;
		}

		// reason are similar, use previous message
		if (listItem)
		{
			if (strstr(listItem->message2, message2))
			{
				// if the XP amount is different, stack it up (mainly kill assist)
				if (!(CG_GetActiveHUD()->xpgain.style & POPUP_XPGAIN_NO_XP_ADD_UP) || forceStackingXp || Q_stricmp(listItem->message, message))
				{
					Q_strncpyz(listItem->message, va("%f", Q_atof(listItem->message) + Q_atof(message)), sizeof(cg_pmStackXP[0].message));
				}

				// don't display multiplicator for repairing or constructing
				if (!forceStackingXp)
				{
					Q_strncpyz(listItem->message2, va("%s (x%d)", message2, ++listItem->count), sizeof(cg_pmStackXP[0].message2));
				}

				listItem->time = cg.time;

				return;
			}
		}
	}

	listItem = CG_FindFreePMItem(cg_pmStackXP, &cg_pmOldListXP, NUM_PM_STACK_ITEMS_XP);

	if (!listItem)
	{
		return;
	}

	if (shader)
	{
		listItem->shader = shader;
	}
	else
	{
		listItem->shader = -1;
	}

	listItem->inuse = qtrue;
	listItem->type  = type;
	Q_strncpyz(listItem->message, message, sizeof(cg_pmStackXP[0].message));

	// print and THEN chop off the newline, as the console deals with newlines perfectly
	if (listItem->message[strlen(listItem->message) - 1] == '\n')
	{
		listItem->message[strlen(listItem->message) - 1] = 0;
	}

	// chop off the newline at the end if any
	while ((end = strchr(listItem->message, '\n')))
	{
		*end = '\0';
	}

	// don't eat popups for empty lines
	if (*listItem->message == '\0')
	{
		return;
	}

	listItem->count = 1;

	if (message2 && !(CG_GetActiveHUD()->xpgain.style & POPUP_XPGAIN_NO_REASON))
	{
		Q_strncpyz(listItem->message2, message2, sizeof(cg_pmStackXP[0].message2));

		/*
		if (listItem->message[strlen(listItem->message2) - 1] == '\n')
		{
			listItem->message[strlen(listItem->message2) - 1] = 0;
		}

		while ((end = strchr(listItem->message2, '\n')))
		{
			*end = '\0';
		}

		if (*listItem->message2 == '\0')
		{
			return;
		}
		*/
	}

	if (!cg_pmWaitingListXP)
	{
		cg_pmWaitingListXP = listItem;
		listItem->time     = cg.time;
	}
	else
	{
		pmListItem_t *loop = cg_pmWaitingListXP;

		while (loop->next)
		{
			loop = loop->next;
		}

		loop->next = listItem;
	}
}

/**
 * @brief CG_DrawPMItems
 * @param[in] comp
 * @param[in] listItem
 * @param[in,out] y
 * @param[in] lineHeight
 * @param[in] size
 * @return
 */
static qboolean CG_DrawPMItems(hudComponent_t *comp, pmListItem_t *listItem, float *y, float lineHeight, float size, qboolean scrollDown,
                               int time, int stayTime, int fadeTime)
{
	float  t;
	float  w;
	vec4_t colorText;
	float  scale;
	float  x = (comp->alignText == ITEM_ALIGN_RIGHT) ? comp->location.x + comp->location.w : comp->location.x;
	char   buffer[256];
	int    lineNumber            = 1;
	char   weaponIconSpacing[32] = { 0 };

	if (!listItem)
	{
		return qfalse;
	}

	Vector4Copy(comp->colorMain, colorText);
	scale = CG_ComputeScale(comp /*lineHeight, comp->scale, &cgs.media.limboFont2*/);

	t = listItem->time + time + stayTime;
	if (cg.time > t)
	{
		colorText[3] *= 1 - ((cg.time - t) / (float)fadeTime);
	}

	Q_strncpyz(buffer, CG_TranslateString(listItem->message), sizeof(buffer));

	if (listItem->weaponShader > 0)
	{
		unsigned int i;

		// icon size + 2 extra space
		i = floor((size * abs(listItem->scaleShader)) / CG_Text_Width_Ext(" ", CG_ComputeScale(comp), 0, &cgs.media.limboFont2)) + 2;

		memset(weaponIconSpacing, ' ', sizeof(weaponIconSpacing));
		weaponIconSpacing[MIN(i, sizeof(weaponIconSpacing) - 1)] = '\0';

		Q_strcat(buffer, sizeof(buffer), weaponIconSpacing);
	}

	if (listItem->message2[0])
	{
		Q_strcat(buffer, sizeof(buffer), CG_TranslateString(listItem->message2));
	}

	w = comp->location.w - size * 2;
	CG_WordWrapString(buffer, CG_GetMaxCharsPerLine(buffer, scale, &cgs.media.limboFont2, w), buffer, sizeof(buffer), &lineNumber);

	// we reach the comp border, don't print the line
	if (scrollDown)
	{
		*y += lineHeight;

		if (*y + lineHeight * (lineNumber - 1) + lineHeight * 0.25f > comp->location.y + comp->location.h)
		{
			return qfalse;
		}
	}
	else
	{
		*y -= lineHeight * (lineNumber - 1);

		if (*y - (lineHeight + lineHeight * 0.25f) < comp->location.y)
		{
			return qfalse;
		}
	}

	if (listItem->shader > 0)
	{
		// colorize
		VectorCopy(listItem->color, colorText);
		trap_R_SetColor(colorText);

		if (comp->alignText == ITEM_ALIGN_RIGHT)
		{
			x -= size;
			CG_DrawPic(x, *y - size, size, size, listItem->shader);
		}
		else
		{
			CG_DrawPic(x, *y - size, size, size, listItem->shader);
			x += size;
		}

		// decolorize
		VectorCopy(colorWhite, colorText);
		trap_R_SetColor(NULL);
	}

	if (comp->alignText == ITEM_ALIGN_RIGHT)
	{
		w  = CG_Text_Line_Width_Ext_Float(buffer, scale, &cgs.media.limboFont2);
		x -= w;
	}

	CG_DrawMultilineText(x, *y - lineHeight * 0.25, w, scale, scale, colorText, buffer, lineHeight, 0, 0, comp->styleText, comp->alignText, &cgs.media.limboFont2);

	if (listItem->weaponShader > 0)
	{
		int  i;
		char *spacingStart;

		spacingStart = strstr(buffer, weaponIconSpacing);

		// keep in buffer text until icon insertion
		if (spacingStart && spacingStart != buffer)
		{
			etl_assert(spacingStart - buffer + 1 < sizeof(buffer) && spacingStart - buffer + 1 >= 0);
			buffer[spacingStart - buffer + 1] = '\0';

			// determine on which line the icon should appear
			for (i = 0; buffer[i]; ++i)
			{
				if (buffer[i] == '\n')
				{
					--lineNumber;
				}
			}
		}
		else
		{
			Q_strncpyz(buffer, CG_TranslateString(listItem->message), sizeof(buffer));
			Q_strcat(buffer, sizeof(buffer), " ");
		}

		// colorize
		VectorCopy(listItem->color, colorText);
		trap_R_SetColor(colorText);

		CG_DrawPic(x + CG_Text_Line_Width_Ext_Float(buffer, scale, &cgs.media.limboFont2), *y - size, size * listItem->scaleShader, size, listItem->weaponShader);

		// decolorize
		VectorCopy(colorWhite, colorText);
		trap_R_SetColor(NULL);
	}

	// next line
	*y += scrollDown ? lineHeight * (lineNumber - 1) + lineHeight * 0.25f : -(lineHeight + lineHeight * 0.25f);

	return qtrue;
}

/**
 * @brief CG_DrawPM
 * @param[in] comp
 */
void CG_DrawPM(hudComponent_t *comp)
{
	pmListItem_t *listItem;
	float        lineHeight;
	float        size;
	float        y;
	qboolean     isScapeAvailable;
	int          pmNum = comp - &CG_GetActiveHUD()->popupmessages;

	if (!cg_pmWaitingList[pmNum])
	{
		return;
	}

	size = lineHeight = CG_Text_Height_Ext("A", CG_ComputeScale(comp), 0, &cgs.media.limboFont2);

	size       *= 2.f;
	lineHeight *= 1.75f;

	y = comp->style & POPUP_SCROLL_DOWN ? comp->location.y : comp->location.y + comp->location.h;

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	isScapeAvailable = CG_DrawPMItems(comp, cg_pmWaitingList[pmNum], &y, lineHeight, size, comp->style & POPUP_SCROLL_DOWN,
	                                  cg_popupTime.integer, cg_popupStayTime.integer, cg_popupFadeTime.integer);

	for (listItem = cg_pmOldList[pmNum]; listItem && isScapeAvailable; listItem = listItem->next)
	{
		isScapeAvailable = CG_DrawPMItems(comp, listItem, &y, lineHeight, size, comp->style & POPUP_SCROLL_DOWN,
		                                  cg_popupTime.integer, cg_popupStayTime.integer, cg_popupFadeTime.integer);
	}
}

/**
 * @brief CG_DrawPMItemsBig
 * @param[in] comp
 */
void CG_DrawPMItemsBig(hudComponent_t *comp)
{
	vec4_t colorText, colorBackground, colorBorder;
	float  t, w, h, iconsSize, scale;

	if (!cg_pmWaitingListBig)
	{
		return;
	}

	Vector4Copy(comp->colorMain, colorText);
	Vector4Copy(comp->colorBackground, colorBackground);
	Vector4Copy(comp->colorBorder, colorBorder);
	t = cg_pmWaitingListBig->time + PM_BIGPOPUP_TIME + cg_popupStayTime.value;

	if (cg.time > t)
	{
		float fade = cg_popupFadeTime.integer ? 1 - ((cg.time - t) / cg_popupFadeTime.value) : 0;

		colorText[3]       *= fade;
		colorBackground[3] *= fade;
		colorBorder[3]     *= fade;
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, colorBorder);
	}

	h         = comp->location.h / 5.f;
	iconsSize = comp->location.h - h;

	scale = CG_ComputeScale(comp /*h, comp->scale, &cgs.media.limboFont2*/);

	trap_R_SetColor(colorText);
	CG_DrawPic(comp->location.x + comp->location.w - iconsSize, comp->location.y, iconsSize, iconsSize, cg_pmWaitingListBig->shader);
	trap_R_SetColor(NULL);

	//int lim = (comp->location.w - (x - comp->location.x)) / spacingWidth;
	w = CG_Text_Width_Ext(cg_pmWaitingListBig->message, scale, 0, &cgs.media.limboFont2);

	CG_Text_Paint_Ext(comp->location.x + (comp->location.w - w) - iconsSize, comp->location.y + iconsSize + h * 0.5, scale, scale, colorText, cg_pmWaitingListBig->message, 0, 0, comp->styleText, &cgs.media.limboFont2);
}

/**
 * @brief CG_DrawPMItems
 * @param[in] comp
 * @param[in] listItem
 * @param[in,out] y
 * @param[in] lineHeight
 * @param[in] size
 * @return
 */
static qboolean CG_DrawPMXPItems(hudComponent_t *comp, pmListItem_t *listItem, float *y, float lineHeight, float size, qboolean scrollDown,
                                 int time, int stayTime, int fadeTime)
{
	float  t;
	float  w;
	vec4_t colorText;
	vec4_t colorText2;
	float  scale;
	float  x = (comp->alignText == ITEM_ALIGN_RIGHT) ? comp->location.x + comp->location.w : comp->location.x;
	char   buffer[256];
	int    lineNumber = 1;
	float  XPGained;
	char   *XPGainNumber;
	float  remainder;

	if (!listItem)
	{
		return qfalse;
	}

	Vector4Copy(comp->colorMain, colorText);
	Vector4Copy(comp->colorSecondary, colorText2);
	scale = CG_ComputeScale(comp /*lineHeight, comp->scale, &cgs.media.limboFont2*/);

	XPGained  = Q_atof(listItem->message);
	remainder = fmodf(XPGained, 1);

	// there are digits after decimal point, draw decimal
	if (remainder)
	{
		XPGainNumber = va(" %2.1f XP ", XPGained);
	}
	else
	{
		XPGainNumber = va(" %2.0f XP ", XPGained);
	}

	// fadein
//	t = listItem->time + time + fadeTime;
//	if (cg.time < t)
//	{
//		colorText[3]  *= 1.0f - (t - cg.time) / (float)fadeTime;
//		colorText2[3] *= 1.0f - (t - cg.time) / (float)fadeTime;
//	}

	// fadeout
	t = listItem->time + time + stayTime;
	if (cg.time > t)
	{
		colorText[3]  *= 1 - ((cg.time - t) / (float)fadeTime);
		colorText2[3] *= 1 - ((cg.time - t) / (float)fadeTime);
	}

	Q_strncpyz(buffer, XPGainNumber, sizeof(buffer));

	if (listItem->message2[0])
	{
		Q_strcat(buffer, sizeof(buffer), CG_TranslateString(listItem->message2));
	}

	w = comp->location.w - size * 2;
	CG_WordWrapString(buffer, CG_GetMaxCharsPerLine(buffer, scale * 0.75, &cgs.media.limboFont2, w), buffer, sizeof(buffer), &lineNumber);

	// we reach the comp border, don't print the line
	if (scrollDown)
	{
		*y += lineHeight;

		if (*y + lineHeight * (lineNumber - 1) + lineHeight * 0.25f > comp->location.y + comp->location.h)
		{
			return qfalse;
		}
	}
	else
	{
		*y -= lineHeight * (lineNumber - 1);

		if (*y - (lineHeight + lineHeight * 0.25f) < comp->location.y)
		{
			return qfalse;
		}
	}

	if (listItem->shader > 0)
	{
		// colorize
		trap_R_SetColor(colorText);

		if (comp->alignText == ITEM_ALIGN_RIGHT)
		{
			x -= size;
			CG_DrawPic(x, *y - size, size, size, listItem->shader);
		}
		else
		{
			CG_DrawPic(x, *y - size, size, size, listItem->shader);
			x += size;
		}

		// decolorize
		trap_R_SetColor(NULL);
	}

	if (comp->alignText == ITEM_ALIGN_RIGHT)
	{
		w  = CG_Text_Line_Width_Ext_Float(XPGainNumber, scale, &cgs.media.limboFont1);
		w += CG_Text_Line_Width_Ext_Float(listItem->message2, scale * 0.75f, &cgs.media.limboFont2);
		x -= w;
	}

	// fix miss alignment due to shorter . width
	if (remainder)
	{
		if (comp->alignText == ITEM_ALIGN_RIGHT)
		{
			x += CG_Text_Line_Width_Ext_Float(".", scale, &cgs.media.limboFont1);
			x -= CG_Text_Line_Width_Ext_Float("1", scale, &cgs.media.limboFont1);
		}
		else
		{
			x -= CG_Text_Line_Width_Ext_Float(".", scale, &cgs.media.limboFont1);
			x += CG_Text_Line_Width_Ext_Float("1", scale, &cgs.media.limboFont1);
		}
	}

	CG_Text_Paint_Ext(x, *y - lineHeight * 0.25f, scale, scale, colorText, XPGainNumber, 0, 0, comp->styleText, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + (lineNumber > 1 ? 0 : CG_Text_Width_Ext(XPGainNumber, scale, 0, &cgs.media.limboFont1)),
	                  *y - lineHeight * 0.375f + (lineNumber == 1 ? 0 : lineHeight),
	                  scale * 0.75f, scale * 0.75f, colorText2, listItem->message2, 0, 0, comp->styleText, &cgs.media.limboFont2);

	// next line
	*y += scrollDown ? lineHeight * (lineNumber - 1) + lineHeight * 0.25f : -(lineHeight + lineHeight * 0.25f);

	return qtrue;
}

/**
 * @brief CG_DrawPMItemsXPGain
 * @param[in] comp
 */
void CG_DrawPMItemsXPGain(hudComponent_t *comp)
{
	pmListItem_t *listItem;
	float        lineHeight;
	float        size;
	float        y;
	qboolean     isScapeAvailable;

	if (!cg_pmWaitingListXP)
	{
		return;
	}

	size = lineHeight = CG_Text_Height_Ext("A", CG_ComputeScale(comp), 0, &cgs.media.limboFont2);

	size       *= 2.f;
	lineHeight *= 1.75f;

	y = comp->style & POPUP_XPGAIN_SCROLL_DOWN ? comp->location.y : comp->location.y + comp->location.h;

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	isScapeAvailable = CG_DrawPMXPItems(comp, cg_pmWaitingListXP, &y, lineHeight, size, comp->style & POPUP_XPGAIN_SCROLL_DOWN,
	                                    cg_popupXPGainTime.integer, cg_popupXPGainStayTime.integer, cg_popupXPGainFadeTime.integer);

	for (listItem = cg_pmOldListXP; listItem && isScapeAvailable; listItem = listItem->next)
	{
		isScapeAvailable = CG_DrawPMXPItems(comp, listItem, &y, lineHeight, size, comp->style & POPUP_XPGAIN_SCROLL_DOWN,
		                                    cg_popupXPGainTime.integer, cg_popupXPGainStayTime.integer, cg_popupXPGainFadeTime.integer);
	}
}

/**
 * @brief CG_GetPMItemText
 * @param[in] cent
 * @return
 */
const char *CG_GetPMItemText(centity_t *cent)
{
	switch (cent->currentState.effect1Time)
	{
	case PM_DYNAMITE:
		switch (cent->currentState.effect2Time)
		{
		case 0:
			return va(CG_TranslateString("Planted at %s."), CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		case 1:
			return va(CG_TranslateString("Defused at %s."), CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		}
		break;
	case PM_CONSTRUCTION:
		switch (cent->currentState.effect2Time)
		{
		case -1:
			return CG_ConfigString(CS_STRINGS + cent->currentState.effect3Time);
		case 0:
			return va(CG_TranslateString("%s has been constructed."), CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		}
		break;
	case PM_DESTRUCTION:
		switch (cent->currentState.effect2Time)
		{
		case 0:
			return va(CG_TranslateString("%s has been damaged."), CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		case 1:
			return va(CG_TranslateString("%s has been destroyed."), CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		}
		break;
	case PM_MINES:
		// Prevent spectators from being informed when a mine is spotted
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
		{
			return NULL;
		}
		if (cgs.clientinfo[cg.clientNum].team == cent->currentState.effect2Time)
		{
			return NULL;
		}

		if (cg_locations.integer & LOC_LANDMINES)
		{
			char *locStr = CG_BuildLocationString(-1, cent->currentState.origin, LOC_LANDMINES);

			if (!locStr || !*locStr)
			{
				return va("Spotted by %s", cgs.clientinfo[cent->currentState.effect3Time].name);
			}
			return va(CG_TranslateString("Spotted by %s^7 at %s"), cgs.clientinfo[cent->currentState.effect3Time].name, locStr);
		}
		else
		{
			return va(CG_TranslateString("Spotted by %s"), cgs.clientinfo[cent->currentState.effect3Time].name);
		}
	case PM_OBJECTIVE:
		switch (cent->currentState.density)
		{
		case 0:
			return va(CG_TranslateString("%s have stolen %s!"), cent->currentState.effect2Time == TEAM_ALLIES ? CG_TranslateString("Allies") : CG_TranslateString("Axis"), CG_ConfigString(CS_STRINGS + cent->currentState.effect3Time));
		case 1:
			return va(CG_TranslateString("%s have returned %s!"), cent->currentState.effect2Time == TEAM_ALLIES ? CG_TranslateString("Allies") : CG_TranslateString("Axis"), CG_ConfigString(CS_STRINGS + cent->currentState.effect3Time));
		}
		break;
	case PM_TEAM:
		switch (cent->currentState.density)
		{
		case 0:             // joined
		{
			const char *teamstr = NULL;

			switch (cent->currentState.effect2Time)
			{
			case TEAM_AXIS:
				teamstr = "Axis team";
				break;
			case TEAM_ALLIES:
				teamstr = "Allied team";
				break;
			default:
				teamstr = "Spectators";
				break;
			}

			return va(CG_TranslateString("%s^7 has joined the %s^7!"), cgs.clientinfo[cent->currentState.effect3Time].name, CG_TranslateString(teamstr));
		}
		case 1:
			return va(CG_TranslateString("%s^7 disconnected"), cgs.clientinfo[cent->currentState.effect3Time].name);
		}
		break;
	}

	return NULL;
}

static int lastSoundTime = 0;

/**
 * @brief CG_PlayPMItemSound
 * @param[in] cent
 */
void CG_PlayPMItemSound(centity_t *cent)
{
	switch (cent->currentState.effect1Time)
	{
	case PM_DYNAMITE:
		switch (cent->currentState.effect2Time)
		{
		case 0:
			if (cent->currentState.teamNum == TEAM_AXIS)
			{
				CG_SoundPlaySoundScript("axis_hq_dynamite_planted", NULL, -1, qtrue);
			}
			else
			{
				CG_SoundPlaySoundScript("allies_hq_dynamite_planted", NULL, -1, qtrue);
			}
			break;
		case 1:
			if (cent->currentState.teamNum == TEAM_AXIS)
			{
				CG_SoundPlaySoundScript("axis_hq_dynamite_defused", NULL, -1, qtrue);
			}
			else
			{
				CG_SoundPlaySoundScript("allies_hq_dynamite_defused", NULL, -1, qtrue);
			}
			break;
		}
		break;
	case PM_MINES:
		// Prevent spectators from being informed when a mine is spotted
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
		{
			break;
		}

		// don't spam landmine spotted sounds ... play once per 10 secs
		if (!lastSoundTime || cg.time > lastSoundTime)
		{
			if (cgs.clientinfo[cg.clientNum].team != cent->currentState.effect2Time)
			{
				// inverted teams
				if (cent->currentState.effect2Time == TEAM_AXIS)
				{
					CG_SoundPlaySoundScript("allies_hq_mines_spotted", NULL, -1, qtrue);
				}
				else
				{
					CG_SoundPlaySoundScript("axis_hq_mines_spotted", NULL, -1, qtrue);
				}
				lastSoundTime = cg.time + 10000; // 10 secs
			}
		}
		break;
	case PM_OBJECTIVE:
		switch (cent->currentState.density)
		{
		case 0:
			if (cent->currentState.effect2Time == TEAM_AXIS)
			{
				CG_SoundPlaySoundScript("axis_hq_objective_taken", NULL, -1, qtrue);
			}
			else
			{
				CG_SoundPlaySoundScript("allies_hq_objective_taken", NULL, -1, qtrue);
			}
			break;
		case 1:
			if (cent->currentState.effect2Time == TEAM_AXIS)
			{
				CG_SoundPlaySoundScript("axis_hq_objective_secure", NULL, -1, qtrue);
			}
			else
			{
				CG_SoundPlaySoundScript("allies_hq_objective_secure", NULL, -1, qtrue);
			}
			break;
		}
		break;
	default:
		break;
	}
}

/**
 * @brief CG_GetPMItemIcon
 * @param[in] cent
 * @return
 */
qhandle_t CG_GetPMItemIcon(centity_t *cent)
{
	switch (cent->currentState.effect1Time)
	{
	case PM_CONSTRUCTION:
		if (cent->currentState.density == TEAM_AXIS)
		{
			return cgs.media.pmImageAxisConstruct;
		}
		return cgs.media.pmImageAlliesConstruct;
	case PM_DESTRUCTION:
		if (cent->currentState.density == TEAM_AXIS)
		{
			return cgs.media.pmImageAxisConstruct;
		}
		return cgs.media.pmImageAlliesConstruct;
	case PM_MINES:
		if (cent->currentState.effect2Time == TEAM_AXIS)
		{
			return cgs.media.pmImageAlliesMine;
		}
		return cgs.media.pmImageAxisMine;
	case PM_TEAM:
		if (cent->currentState.effect2Time == TEAM_AXIS)
		{
			return cgs.media.pmImageAxisFlag;
		}
		else if (cent->currentState.effect2Time == TEAM_ALLIES)
		{
			return cgs.media.pmImageAlliesFlag;
		}
		return cgs.media.pmImageSpecFlag;
	default:
		return cgs.media.pmImages[cent->currentState.effect1Time];
	}
}
