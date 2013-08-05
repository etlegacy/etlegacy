/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
 * @file cg_popupmessages.c
 */

#include "cg_local.h"

#define NUM_PM_STACK_ITEMS  32

#define NUM_PM_STACK_ITEMS_BIG 8 // we shouldn't need many of these

typedef struct pmStackItem_s pmListItem_t;
typedef struct pmStackItemBig_s pmListItemBig_t;

struct pmStackItem_s
{
	popupMessageType_t type;
	qboolean inuse;
	int time;
	char message[128];
	qhandle_t shader;

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
	{ "Binoculars",                               "Improved Physical Fitness",                 "Improved Health",                       "Trap Awareness"           }, // battle sense
	{ "Improved use of Explosive Ammunition",     "Improved Dexterity",                        "Improved Construction and Destruction", "a Flak Jacket"            }, // explosives & construction
	{ "Medic Ammo",                               "Improved Resources",                        "Full Revive",                           "Adrenalin Self"           }, // first aid
	{ "Improved Resources",                       "Improved Signals",                          "Improved Air and Ground Support",       "Enemy Recognition"        }, // signals
	{ "Improved use of Light Weapon Ammunition",  "Faster Reload",                             "Improved Light Weapon Handling",        "Dual-Wield Pistols"       }, // light weapons
	{ "Improved Projectile Resources",            "Heavy Weapon Proficiency",                  "Improved Dexterity",                    "Improved Weapon Handling" }, // heavy weapons
	{ "Improved use of Scoped Weapon Ammunition", "Improved use of Sabotage and Misdirection", "Breath Control",                        "Assassin"                 } // scoped weapons & military intelligence
};

void CG_PMItemBigSound(pmListItemBig_t *item);

void CG_InitPMGraphics(void)
{
	cgs.media.pmImages[PM_DYNAMITE]     = trap_R_RegisterShaderNoMip("gfx/limbo/cm_dynamite");
	cgs.media.pmImages[PM_CONSTRUCTION] = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_MINES]        = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_DEATH]        = trap_R_RegisterShaderNoMip("gfx/hud/pm_death");
	cgs.media.pmImages[PM_MESSAGE]      = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_OBJECTIVE]    = trap_R_RegisterShaderNoMip("sprites/objective");
	cgs.media.pmImages[PM_DESTRUCTION]  = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_TEAM]         = trap_R_RegisterShaderNoMip("sprites/voiceChat");

	cgs.media.pmImageAlliesConstruct = trap_R_RegisterShaderNoMip("gfx/hud/pm_constallied");
	cgs.media.pmImageAxisConstruct   = trap_R_RegisterShaderNoMip("gfx/hud/pm_constaxis");
	cgs.media.pmImageAlliesMine      = trap_R_RegisterShaderNoMip("gfx/hud/pm_mineallied");
	cgs.media.pmImageAxisMine        = trap_R_RegisterShaderNoMip("gfx/hud/pm_mineaxis");
	cgs.media.hintKey                = trap_R_RegisterShaderNoMip("gfx/hud/keyboardkey_old");
}

void CG_InitPM(void)
{
	memset(&cg_pmStack, 0, sizeof(cg_pmStack));
	memset(&cg_pmStackBig, 0, sizeof(cg_pmStackBig));

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

#define PM_FADETIME_BIG 1000
#define PM_WAITTIME_BIG 3500
#define PM_BIGPOPUP_TIME 2500

void CG_AddToListFront(pmListItem_t **list, pmListItem_t *item)
{
	item->next = *list;
	*list      = item;
}

void CG_UpdatePMLists(void)
{
	pmListItem_t    *listItem;
	pmListItem_t    *lastItem;
	pmListItemBig_t *listItem2;

	if ((listItem = cg_pmWaitingList))
	{
		int t = cg_popupTime.integer + listItem->time;

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
				else
				{
					// just sit where we are, no pressure to do anything...
				}
			}
		}
	}

	listItem = cg_pmOldList;
	lastItem = NULL;
	while (listItem)
	{
		int t = cg_popupTime.integer + listItem->time + cg_popupStayTime.integer + cg_popupFadeTime.integer;

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

	if ((listItem2 = cg_pmWaitingListBig))
	{
		int t = PM_BIGPOPUP_TIME + listItem2->time;

		if (cg.time > t)
		{
			if (listItem2->next)
			{
				// there's another item waiting to come on, so kill us and shove the next one to the front
				cg_pmWaitingListBig       = listItem2->next;
				cg_pmWaitingListBig->time = cg.time; // set time we popped up at

				CG_PMItemBigSound(cg_pmWaitingListBig);

				listItem2->inuse = qfalse;
				listItem2->next  = NULL;
			}
			else
			{
				if (cg.time > t + cg_popupStayTime.integer + cg_popupFadeTime.integer)
				{
					// we're gone completely
					cg_pmWaitingListBig = NULL;
					listItem2->inuse    = qfalse;
					listItem2->next     = NULL;
				}
				else
				{
					// just sit where we are, no pressure to do anything...
				}
			}
		}
	}
}

pmListItemBig_t *CG_FindFreePMItem2(void)
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

void CG_AddPMItem(popupMessageType_t type, const char *message, qhandle_t shader, vec3_t color)
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
		listItem->shader = cgs.media.pmImages[type];
	}

	listItem->inuse = qtrue;
	listItem->type  = type;
	Q_strncpyz(listItem->message, message, sizeof(cg_pmStack[0].message));

	// colored obituaries
	listItem->color[0] = listItem->color[1] = listItem->color[2] = 1.f;
	if (color != NULL)
	{
		VectorCopy(color, listItem->color);
	}

	// moved this: print and THEN chop off the newline, as the
	// console deals with newlines perfectly.  We do chop off the newline
	// at the end, if any, though.
	if (listItem->message[strlen(listItem->message) - 1] == '\n')
	{
		listItem->message[strlen(listItem->message) - 1] = 0;
	}

	trap_Print(va("%s\n", listItem->message));

	while ((end = strchr(listItem->message, '\n'))) // added parens
	{
		*end = '\0';
	}

	// don't eat popups for empty lines
	if (*listItem->message == '\0')
	{
		return;
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

void CG_PMItemBigSound(pmListItemBig_t *item)
{
	if (!cg.snap)
	{
		return;
	}

	switch (item->type)
	{
	case PM_RANK:
		trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.sndRankUp);
		break;
	case PM_SKILL:
		trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.sndSkillUp);
		break;
	default:
		break;
	}
}

void CG_AddPMItemBig(popupMessageBigType_t type, const char *message, qhandle_t shader)
{
	pmListItemBig_t *listItem = CG_FindFreePMItem2();

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
void CG_DrawPMItems(rectDef_t rect)
{
	vec4_t       colour     = { 0.f, 0.f, 0.f, 1.f };
	vec4_t       colourText = { 1.f, 1.f, 1.f, 1.f };
	float        t;
	int          i, j, size;
	pmListItem_t *listItem = cg_pmOldList;
	float        y         = rect.y; //360;

	if (cg_drawSmallPopupIcons.integer)
	{
		size = PM_ICON_SIZE_SMALL;

		y += 4;
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

	t = cg_pmWaitingList->time + cg_popupTime.integer + cg_popupStayTime.integer;
	if (cg.time > t)
	{
		colourText[3] = colour[3] = 1 - ((cg.time - t) / (float)cg_popupFadeTime.integer);
	}

	if (cg_pmWaitingList->shader > 0)
	{
		// colorize
		for (j = 0; j < 3; j++)
		{
			colourText[j] = cg_pmWaitingList->color[j];
		}
		trap_R_SetColor(colourText);
		// draw
		CG_DrawPic(4, y, size, size, cg_pmWaitingList->shader);
		// decolorize
		for (j = 0; j < 3; j++)
		{
			colourText[j] = 1.f;
		}
		trap_R_SetColor(NULL);
	}
	else
	{
		size = 0;
	}

	CG_Text_Paint_Ext(4 + size + 2, y + 12, 0.2f, 0.2f, colourText, cg_pmWaitingList->message, 0, 0, 0, &cgs.media.limboFont2);

	for (i = 0; i < 6 && listItem; i++, listItem = listItem->next)
	{
		y -= size + 2;

		t = listItem->time + cg_popupTime.integer + cg_popupStayTime.integer;
		if (cg.time > t)
		{
			colourText[3] = colour[3] = 1 - ((cg.time - t) / (float)cg_popupFadeTime.integer);
		}
		else
		{
			colourText[3] = colour[3] = 1.f;
		}

		if (listItem->shader > 0)
		{
			for (j = 0; j < 3; j++) // colorize
			{
				colourText[j] = listItem->color[j];
			}

			trap_R_SetColor(colourText);
			CG_DrawPic(4, y, size, size, listItem->shader);

			for (j = 0; j < 3; j++) // decolorize
			{
				colourText[j] = 1.f;
			}
			trap_R_SetColor(NULL);
		}
		else
		{
			size = 0;
		}

		CG_Text_Paint_Ext(rect.x + size + 2, y + 12, 0.2f, 0.2f, colourText, listItem->message, 0, 0, 0, &cgs.media.limboFont2);
	}
}

void CG_DrawPMItemsBig(void)
{
	vec4_t colour     = { 0.f, 0.f, 0.f, 1.f };
	vec4_t colourText = { 1.f, 1.f, 1.f, 1.f };
	float  t;
	float  y = 270;
	float  w;

	if (!cg_pmWaitingListBig)
	{
		return;
	}

	t = cg_pmWaitingListBig->time + PM_BIGPOPUP_TIME + PM_WAITTIME_BIG;
	if (cg.time > t)
	{
		colourText[3] = colour[3] = 1 - ((cg.time - t) / (float)PM_FADETIME_BIG);
	}

	trap_R_SetColor(colourText);
	CG_DrawPic(Ccg_WideX(SCREEN_WIDTH) - 56, y, 48, 48, cg_pmWaitingListBig->shader);
	trap_R_SetColor(NULL);


	w = CG_Text_Width_Ext(cg_pmWaitingListBig->message, 0.22f, 0, &cgs.media.limboFont2);
	CG_Text_Paint_Ext(Ccg_WideX(SCREEN_WIDTH) - 4 - w, y + 56, 0.22f, 0.24f, colourText, cg_pmWaitingListBig->message, 0, 0, 0, &cgs.media.limboFont2);
}

#define TXTCOLOR_OBJ "^O"

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
				return va("%sSpotted by ^7%s", TXTCOLOR_OBJ, cgs.clientinfo[cent->currentState.effect3Time].name);
			}
			return va(CG_TranslateString("%sSpotted by ^7%s%s at %s"), TXTCOLOR_OBJ, cgs.clientinfo[cent->currentState.effect3Time].name, TXTCOLOR_OBJ, locStr);
		}
		else
		{
			return va(CG_TranslateString("%sSpotted by ^7%s"), TXTCOLOR_OBJ, cgs.clientinfo[cent->currentState.effect3Time].name);
		}
		break;
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
		case 0:         // joined
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
	}

	return NULL;
}

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
	case PM_MINES:
		if (cent->currentState.effect2Time == TEAM_AXIS)
		{
			return cgs.media.pmImageAlliesMine;
		}
		return cgs.media.pmImageAxisMine;
	default:
		return cgs.media.pmImages[cent->currentState.effect1Time];
	}
}
