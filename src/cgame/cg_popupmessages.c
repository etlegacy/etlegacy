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
 * @file cg_popupmessages.c
 */

#include "cg_local.h"

#define NUM_PM_STACK_ITEMS     32
#define NUM_PM_STACK_ITEMS_BIG 8 // we shouldn't need many of these

typedef struct pmStackItem_s pmListItem_t;
typedef struct pmStackItemBig_s pmListItemBig_t;

struct pmStackItem_s
{
	popupMessageType_t type;
	qboolean inuse;
	int time;
	char message[128];
	char message2[128];
	qhandle_t shader;
	qhandle_t weaponShader;
	int scaleShader;

	vec3_t color;

	pmListItem_t *next;
};

struct pmStackItemBig_s
{
	popupMessageBigType_t type;
	qboolean inuse;
	int time;
	char message[128];
	qhandle_t shader;

	pmListItemBig_t *next;
};

pmListItem_t    cg_pmStack[NUM_PM_STACK_ITEMS];
pmListItem_t    *cg_pmOldList;
pmListItem_t    *cg_pmWaitingList;
pmListItemBig_t *cg_pmWaitingListBig;

pmListItemBig_t cg_pmStackBig[NUM_PM_STACK_ITEMS_BIG];

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

void CG_PMItemBigSound(pmListItemBig_t *item);

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
}

/**
 * @brief CG_InitPM
 */
void CG_InitPM(void)
{
	Com_Memset(&cg_pmStack, 0, sizeof(cg_pmStack));
	Com_Memset(&cg_pmStackBig, 0, sizeof(cg_pmStackBig));

	cg_pmOldList        = NULL;
	cg_pmWaitingList    = NULL;
	cg_pmWaitingListBig = NULL;
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
 * @brief CG_UpdatePMLists
 */
void CG_UpdatePMLists(void)
{
	pmListItem_t    *listItem;
	pmListItem_t    *lastItem;
	pmListItemBig_t *listItemBig;

	if ((listItem = cg_pmWaitingList))
	{
		int t = listItem->time;

		if (cg.time > t)
		{
			if (listItem->next)
			{
				// there's another item waiting to come on, so move to old list
				cg_pmWaitingList       = listItem->next;
				cg_pmWaitingList->time = cg.time; // set time we popped up at

				CG_AddToListFront(&cg_pmOldList, listItem);
			}
			else
			{
				if (cg.time > t + cg_popupStayTime.integer + cg_popupFadeTime.integer)
				{
					// we're gone completely
					cg_pmWaitingList = NULL;
					listItem->inuse  = qfalse;
					listItem->next   = NULL;
				}
				//else
				//{ // just sit where we are, no pressure to do anything...
				//}
			}
		}
	}

	listItem = cg_pmOldList;
	lastItem = NULL;
	while (listItem)
	{
		int t = listItem->time + cg_popupStayTime.integer + cg_popupFadeTime.integer;

		if (cg.time > t)
		{
			// nuke this, and everything below it (though there shouldn't BE anything below us anyway)
			pmListItem_t *next;

			if (!lastItem)
			{
				// we're the top of the old list, so set to NULL
				cg_pmOldList = NULL;
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

	if ((listItemBig = cg_pmWaitingListBig))
	{
		int t = PM_BIGPOPUP_TIME + listItemBig->time;

		if (cg.time > t)
		{
			if (listItemBig->next)
			{
				// there's another item waiting to come on, so kill us and shove the next one to the front
				cg_pmWaitingListBig       = listItemBig->next;
				cg_pmWaitingListBig->time = cg.time; // set time we popped up at

				CG_PMItemBigSound(cg_pmWaitingListBig);

				listItemBig->inuse = qfalse;
				listItemBig->next  = NULL;
			}
			else
			{
				if (cg.time > t + cg_popupStayTime.integer + cg_popupFadeTime.integer)
				{
					// we're gone completely
					cg_pmWaitingListBig = NULL;
					listItemBig->inuse  = qfalse;
					listItemBig->next   = NULL;
				}
				else
				{
					// just sit where we are, no pressure to do anything...
				}
			}
		}
	}
}

/**
 * @brief CG_FindFreePMItemBig
 * @return
 */
pmListItemBig_t *CG_FindFreePMItemBig(void)
{
	int i = 0;

	for ( ; i < NUM_PM_STACK_ITEMS_BIG; i++)
	{
		if (!cg_pmStackBig[i].inuse)
		{
			return &cg_pmStackBig[i];
		}
	}

	return NULL;
}

/**
 * @brief CG_FindFreePMItem
 * @return
 */
pmListItem_t *CG_FindFreePMItem(void)
{
	pmListItem_t *listItem;
	pmListItem_t *lastItem;
	int          i = 0;

	for ( ; i < NUM_PM_STACK_ITEMS; i++)
	{
		if (!cg_pmStack[i].inuse)
		{
			return &cg_pmStack[i];
		}
	}

	// no totally free items, so just grab the last item in the oldlist
	if ((lastItem = listItem = cg_pmOldList))
	{
		while (listItem->next)
		{
			lastItem = listItem;
			listItem = listItem->next;
		}

		if (lastItem == cg_pmOldList)
		{
			cg_pmOldList = NULL;
		}
		else
		{
			lastItem->next = NULL;
		}

		listItem->inuse = qfalse;

		return listItem;
	}
	else
	{
		// there is no old list... PANIC!
		return NULL;
	}
}

/**
 * @brief CG_CheckPMItemFilter
 * @param[in] type
 * @return
 */
static qboolean CG_CheckPMItemFilter(popupMessageType_t type)
{
	switch (type)
	{
	case PM_CONNECT:
		if (cg_popupFilter.integer & POPUP_FILTER_CONNECT)
		{
			return qtrue;
		}
		break;
	case PM_TEAM:
		if (cg_popupFilter.integer & POPUP_FILTER_TEAMJOIN)
		{
			return qtrue;
		}
		break;
	case PM_MESSAGE:
	case PM_DYNAMITE:
	case PM_CONSTRUCTION:
	case PM_MINES:
	case PM_OBJECTIVE:
	case PM_DESTRUCTION:
		if (cg_popupFilter.integer & POPUP_FILTER_MISSION)
		{
			return qtrue;
		}
		break;
	case PM_AMMOPICKUP:
	case PM_HEALTHPICKUP:
	case PM_WEAPONPICKUP:
		if (cg_popupFilter.integer & POPUP_FILTER_PICKUP)
		{
			return qtrue;
		}
		break;
	case PM_DEATH:
		if (cg_popupFilter.integer & POPUP_FILTER_DEATH)
		{
			return qtrue;
		}
		break;
	default:
		break;
	}
	return qfalse;
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
void CG_AddPMItem(popupMessageType_t type, const char *message, const char *message2, qhandle_t shader, qhandle_t weaponShader, int scaleShader, vec4_t color)
{
	pmListItem_t *listItem;
	char         *end;

	if (!message || !*message)
	{
		return;
	}

	if (type >= PM_NUM_TYPES)
	{
		CG_Printf("Invalid popup type: %d\n", type);
		return;
	}

	if (CG_CheckPMItemFilter(type))
	{
		// log filtered pm to console. Death obituaries are logged in CG_Obituary().
		if (type != PM_DEATH)
		{
			trap_Print(va("%s\n", message));
		}
		return;
	}

	listItem = CG_FindFreePMItem();

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
	Q_strncpyz(listItem->message, message, sizeof(cg_pmStack[0].message));

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
		Q_strncpyz(listItem->message2, message2, sizeof(cg_pmStack[0].message2));

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

	if (!cg_pmWaitingList)
	{
		cg_pmWaitingList = listItem;
		listItem->time   = cg.time;
	}
	else
	{
		pmListItem_t *loop = cg_pmWaitingList;

		while (loop->next)
		{
			loop = loop->next;
		}

		loop->next = listItem;
	}
}

/**
 * @brief CG_PMItemBigSound
 * @param item
 */
void CG_PMItemBigSound(pmListItemBig_t *item)
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
	pmListItemBig_t *listItem;

	listItem = CG_FindFreePMItemBig();

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
		pmListItemBig_t *loop = cg_pmWaitingListBig;

		while (loop->next)
		{
			loop = loop->next;
		}

		loop->next = listItem;
	}
}

#define PM_ICON_SIZE_NORMAL 20
#define PM_ICON_SIZE_SMALL 12
#define ICON_Y_OFFSET(y)  y + 3

/**
 * @brief CG_DrawPMItems
 * @param[in] rect
 * @param[in] style
 */
void CG_DrawPMItems(rectDef_t rect, int style)
{
	vec4_t       color     = { 0.f, 0.f, 0.f, 1.f };
	vec4_t       colorText = { 1.f, 1.f, 1.f, 1.f };
	float        t;
	int          i, size, w, sizew;
	pmListItem_t *listItem = cg_pmOldList;
	float        y         = rect.y; //360;
	float        fontScale = cg_fontScaleSP.value;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		y = 110;
	}

	if (cg_drawSmallPopupIcons.integer)
	{
		size = PM_ICON_SIZE_SMALL;
		y   += 4;
	}
	else
	{
		size = PM_ICON_SIZE_NORMAL;
	}

	if (cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] >= 0)
	{
		y -= 20;
	}

	if (!cg_pmWaitingList)
	{
		return;
	}

	t = cg_pmWaitingList->time + cg_popupStayTime.integer;
	if (cg.time > t)
	{
		colorText[3] = color[3] = 1 - ((cg.time - t) / (float)cg_popupFadeTime.integer);
	}

	if (cg_pmWaitingList->shader > 0)
	{
		// colorize
		VectorCopy(cg_pmWaitingList->color, colorText);
		trap_R_SetColor(colorText);

		CG_DrawPic(4, ICON_Y_OFFSET(y), size, size, cg_pmWaitingList->shader);

		// decolorize
		VectorCopy(colorWhite, colorText);
		trap_R_SetColor(NULL);
	}
	else
	{
		size = -2;
	}

	CG_Text_Paint_Ext(size + 6, y + 12, fontScale, fontScale, colorText, cg_pmWaitingList->message, 0, 0, style, &cgs.media.limboFont2); // 4 + size + 2

	w     = CG_Text_Width_Ext(cg_pmWaitingList->message, fontScale, 0, &cgs.media.limboFont2);
	sizew = (cg_drawSmallPopupIcons.integer) ? PM_ICON_SIZE_SMALL : PM_ICON_SIZE_NORMAL;

	if (cg_pmWaitingList->weaponShader > 0)
	{
		// colorize
		VectorCopy(cg_pmWaitingList->color, colorText);
		trap_R_SetColor(colorText);

		CG_DrawPic(size + w + 12, ICON_Y_OFFSET(y), sizew * cg_pmWaitingList->scaleShader, sizew, cg_pmWaitingList->weaponShader); // 4 + size + 2 + w + 6

		// decolorize
		VectorCopy(colorWhite, colorText);
		trap_R_SetColor(NULL);
	}
	else
	{
		size  = (cg_drawSmallPopupIcons.integer) ? 2 : 8;
		sizew = 0;
	}

	if (cg_pmWaitingList->message2[0])
	{
		CG_Text_Paint_Ext(size + w + sizew * cg_pmWaitingList->scaleShader + 16, y + 12, fontScale, fontScale, colorText, cg_pmWaitingList->message2, 0, 0, style, &cgs.media.limboFont2); // 4 + size + 2 + w + 6 + sizew*... + 4
	}

	for (i = 0; i < 6 && listItem; i++, listItem = listItem->next)
	{
		size = (cg_drawSmallPopupIcons.integer) ? PM_ICON_SIZE_SMALL : PM_ICON_SIZE_NORMAL;

		y -= size + 2;

		t = listItem->time + cg_popupStayTime.integer;
		if (cg.time > t)
		{
			colorText[3] = color[3] = 1 - ((cg.time - t) / (float)cg_popupFadeTime.integer);
		}
		else
		{
			colorText[3] = color[3] = 1.f;
		}

		if (listItem->shader > 0)
		{
			// colorize
			VectorCopy(listItem->color, colorText);
			trap_R_SetColor(colorText);

			CG_DrawPic(4, ICON_Y_OFFSET(y), size, size, listItem->shader);

			// decolorize
			VectorCopy(colorWhite, colorText);
			trap_R_SetColor(NULL);
		}
		else
		{
			size = -2;
		}

		CG_Text_Paint_Ext(rect.x + size + 2, y + 12, fontScale, fontScale, colorText, listItem->message, 0, 0, style, &cgs.media.limboFont2);

		w     = CG_Text_Width_Ext(listItem->message, fontScale, 0, &cgs.media.limboFont2);
		sizew = (cg_drawSmallPopupIcons.integer) ? PM_ICON_SIZE_SMALL : PM_ICON_SIZE_NORMAL;

		if (listItem->weaponShader > 0)
		{
			// colorize
			VectorCopy(listItem->color, colorText);
			trap_R_SetColor(colorText);

			CG_DrawPic(size + w + 12, ICON_Y_OFFSET(y), sizew * listItem->scaleShader, sizew, listItem->weaponShader);

			// decolorize
			VectorCopy(colorWhite, colorText);
			trap_R_SetColor(NULL);
		}
		else
		{
			size  = (cg_drawSmallPopupIcons.integer) ? 2 : 8;
			sizew = 0;
		}

		if (listItem->message2[0])
		{
			CG_Text_Paint_Ext(size + w + sizew * listItem->scaleShader + 16, y + 12, fontScale, fontScale, colorText, listItem->message2, 0, 0, style, &cgs.media.limboFont2);
		}
	}
}

/**
 * @brief CG_DrawPMItemsBig
 * @param[in] style
 * @return
 */
void CG_DrawPMItemsBig(int style)
{
	vec4_t colorText = { 1.f, 1.f, 1.f, 1.f };
	float  t, w;
	float  fontScale = cg_fontScaleSP.value;

	if (!cg_pmWaitingListBig)
	{
		return;
	}

	t = cg_pmWaitingListBig->time + PM_BIGPOPUP_TIME + cg_popupStayTime.value;
	if (cg.time > t)
	{
		colorText[3] = cg_popupFadeTime.integer ? 1 - ((cg.time - t) / cg_popupFadeTime.value) : 0;
	}

	trap_R_SetColor(colorText);
	CG_DrawPic(Ccg_WideX(SCREEN_WIDTH) - 116, 270, 48, 48, cg_pmWaitingListBig->shader);
	trap_R_SetColor(NULL);

	w = CG_Text_Width_Ext(cg_pmWaitingListBig->message, fontScale, 0, &cgs.media.limboFont2);
	CG_Text_Paint_Ext(Ccg_WideX(SCREEN_WIDTH) - 64 - w, 326, fontScale, fontScale, colorText, cg_pmWaitingListBig->message, 0, 0, style, &cgs.media.limboFont2);
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
