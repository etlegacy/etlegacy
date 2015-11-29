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
 */
/**
 * @file cg_draw_hud.c
 * @brief Draws the player's hud
 *
 */

#include "cg_local.h"

typedef enum
{
	FLAGS_MOVE_TIMERS   = BIT(0),
	FLAGS_REMOVE_RANKS  = BIT(1),
	FLAGS_MOVE_POPUPS   = BIT(2),
	FLAGS_POPUPS_SHADOW = BIT(3)
} althud_flags;

typedef enum
{
	STYLE_NORMAL,
	STYLE_SIMPLE
} componentStyle;

typedef struct hudComponent_s
{
	rectDef_t location;
	int visible;
	int style;
} hudComponent_t;

typedef struct hudStructure_s
{
	int hudnumber;
	hudComponent_t compas;
	hudComponent_t staminabar;
	hudComponent_t breathbar;
	hudComponent_t healthbar;
	hudComponent_t weaponchargebar;
	hudComponent_t healthtext;
	hudComponent_t xptext;
	hudComponent_t statsdisplay;
	hudComponent_t weaponicon;
	hudComponent_t weaponammo;
	hudComponent_t fireteam;
	hudComponent_t popupmessages;
	hudComponent_t powerups;
	hudComponent_t hudhead;

	hudComponent_t cursorhint;
	hudComponent_t weaponstability;
	hudComponent_t livesleft;

	hudComponent_t roundtimer;
	hudComponent_t reinforcment;
	hudComponent_t spawntimer;
	hudComponent_t localtime;
} hudStucture_t;

#define SKILL_ICON_SIZE     14

#define SKILLS_X 112
#define SKILLS_Y 20

#define SKILL_BAR_OFFSET    (2 * SKILL_BAR_X_INDENT)
#define SKILL_BAR_X_INDENT  0
#define SKILL_BAR_Y_INDENT  6

#define SKILL_BAR_WIDTH     (SKILL_ICON_SIZE - SKILL_BAR_OFFSET)
#define SKILL_BAR_X         (SKILL_BAR_OFFSET + SKILL_BAR_X_INDENT + SKILLS_X)
#define SKILL_BAR_X_SCALE   (SKILL_ICON_SIZE + 2)
#define SKILL_ICON_X        (SKILL_BAR_OFFSET + SKILLS_X)
#define SKILL_ICON_X_SCALE  (SKILL_ICON_SIZE + 2)
#define SKILL_BAR_Y         (SKILL_BAR_Y_INDENT - SKILL_BAR_OFFSET - SKILLS_Y)
#define SKILL_BAR_Y_SCALE   (SKILL_ICON_SIZE + 2)
#define SKILL_ICON_Y        (-(SKILL_ICON_SIZE + 2) - SKILL_BAR_OFFSET - SKILLS_Y)

#define MAXHUDS 128

int           hudCount = 0;
hudStucture_t hudlist[MAXHUDS];

hudStucture_t *activehud;
hudStucture_t hud0;

/* unused
rectDef_t CG_getRect(float x, float y, float w, float h)
{
    rectDef_t rect = { x, y, w, h };

    return rect;
}
*/

hudComponent_t CG_getComponent(float x, float y, float w, float h, qboolean visible, componentStyle style)
{
	hudComponent_t comp = { { x, y, w, h }, visible, style };

	return comp;
}

void CG_setDefaultHudValues(hudStucture_t *hud)
{
	// the Default hud
	hud->hudnumber       = 0;
	hud->compas          = CG_getComponent((Ccg_WideX(SCREEN_WIDTH) - 100 - 20 - 16), 16, 100 + 32, 100 + 32, qtrue, STYLE_NORMAL);
	hud->staminabar      = CG_getComponent(4, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL);
	hud->breathbar       = CG_getComponent(4, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL);
	hud->healthbar       = CG_getComponent(24, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL);
	hud->weaponchargebar = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 16, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL);
	hud->healthtext      = CG_getComponent(SKILLS_X - 28, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL);
	hud->xptext          = CG_getComponent(SKILLS_X + 28, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL);
	hud->statsdisplay    = CG_getComponent(SKILL_ICON_X, 0, 0, 0, qtrue, STYLE_NORMAL);
	hud->weaponicon      = CG_getComponent((Ccg_WideX(SCREEN_WIDTH) - 82), (SCREEN_HEIGHT - 56), 60, 32, qtrue, STYLE_NORMAL);
	hud->weaponammo      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 22, SCREEN_HEIGHT - 1 * (16 + 2) + 12 - 4, 0, 0, qtrue, STYLE_NORMAL);
	hud->fireteam        = CG_getComponent(10, 10, 100, 100, qtrue, STYLE_NORMAL);
	hud->popupmessages   = CG_getComponent(4, 360, 72, 72, qtrue, STYLE_NORMAL);
	hud->powerups        = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 40, SCREEN_HEIGHT - 140, 36, 36, qtrue, STYLE_NORMAL);
	hud->hudhead         = CG_getComponent(44, SCREEN_HEIGHT - 92, 62, 80, qtrue, STYLE_NORMAL);
	hud->cursorhint      = CG_getComponent(.5f * SCREEN_WIDTH - .5f * 48, 260, 48, 48, qtrue, STYLE_NORMAL); // FIXME: widescreen ?
	hud->weaponstability = CG_getComponent(50, 208, 10, 64, qtrue, STYLE_NORMAL);
	hud->livesleft       = CG_getComponent(0, 0, 0, 0, qtrue, STYLE_NORMAL);

	hud->reinforcment = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 70, SCREEN_HEIGHT - 70, 0, 0, qtrue, STYLE_NORMAL);
	hud->roundtimer   = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 55, SCREEN_HEIGHT - 70, 0, 0, qtrue, STYLE_NORMAL);
	hud->spawntimer   = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 70, SCREEN_HEIGHT - 60, 0, 0, qtrue, STYLE_NORMAL);
	hud->localtime    = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 55, SCREEN_HEIGHT - 60, 0, 0, qtrue, STYLE_NORMAL);
}

/* unused
static hudStucture_t *CG_getNextFreeHud()
{
    hudStucture_t *temp;

    if (hudCount < MAXHUDS)
    {
        temp = &hudlist[hudCount];
        hudCount++;
        memset(temp, 0, sizeof(hudStucture_t));
        CG_setDefaultHudValues(temp);
        return temp;
    }
    else
    {
        return NULL;
    }
}
*/

static hudStucture_t *CG_getHudByNumber(int number)
{
	int           i;
	hudStucture_t *hud;

	if (number < 0 || number >= MAXHUDS)
	{
		Com_Printf("getHudByNumber invalid HUD!\n");
		return NULL;
	}

	for (i = 0; i < hudCount; i++)
	{
		hud = &hudlist[i];

		if (hud->hudnumber == number)
		{
			return hud;
		}
	}

	return NULL;
}

static qboolean CG_isHudNumberAvailable(int number)
{
	hudStucture_t *hud = CG_getHudByNumber(number);

	if (!hud)
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

static void CG_addHudToList(hudStucture_t hud)
{
	hudlist[hudCount] = hud;
	hudCount++;
}

//  HUD SCRIPT FUNCTIONS BELLOW

static qboolean CG_HUD_ParseError(int handle, char *format, ...)
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

static qboolean CG_RectParse(int handle, rectDef_t *r)
{
	float x = 0;

	if (PC_Float_Parse(handle, &x))
	{
		r->x = Ccg_WideX(x);
		if (PC_Float_Parse(handle, &r->y))
		{
			if (PC_Float_Parse(handle, &r->w))
			{
				if (PC_Float_Parse(handle, &r->h))
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

static qboolean CG_ParseHudComponent(int handle, hudComponent_t *comp)
{
	CG_RectParse(handle, &comp->location); //PC_Rect_Parse

	if (!PC_Int_Parse(handle, &comp->style))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->visible))
	{
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_ParseHUD(int handle)
{
	pc_token_t    token;
	hudStucture_t temphud;

	CG_setDefaultHudValues(&temphud);

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_HUD_ParseError(handle, "expected '{'");
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

		if (!Q_stricmp(token.string, "hudnumber"))
		{
			if (!PC_Int_Parse(handle, &temphud.hudnumber))
			{
				return CG_HUD_ParseError(handle, "expected hud number");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "compas"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.compas))
			{
				return CG_HUD_ParseError(handle, "expected compas");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "staminabar"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.staminabar))
			{
				return CG_HUD_ParseError(handle, "expected staminabar");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "breathbar"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.breathbar))
			{
				return CG_HUD_ParseError(handle, "expected breathbar");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "healthbar"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.healthbar))
			{
				return CG_HUD_ParseError(handle, "expected healthbar");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "weaponchangebar"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.weaponchargebar))
			{
				return CG_HUD_ParseError(handle, "expected weaponchangebar");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "healthtext"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.healthtext))
			{
				return CG_HUD_ParseError(handle, "expected healthtext");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "xptext"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.xptext))
			{
				return CG_HUD_ParseError(handle, "expected xptext");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "statsdisplay"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.statsdisplay))
			{
				return CG_HUD_ParseError(handle, "expected statsdisplay");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "weaponicon"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.weaponicon))
			{
				return CG_HUD_ParseError(handle, "expected weaponicon");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "weaponammo"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.weaponammo))
			{
				return CG_HUD_ParseError(handle, "expected weaponammo");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "fireteam"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.fireteam))
			{
				return CG_HUD_ParseError(handle, "expected fireteam");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "popupmessages"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.popupmessages))
			{
				return CG_HUD_ParseError(handle, "expected popupmessages");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "powerups"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.powerups))
			{
				return CG_HUD_ParseError(handle, "expected powerups");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "hudhead"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.hudhead))
			{
				return CG_HUD_ParseError(handle, "expected hudhead");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "cursorhints"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.cursorhint))
			{
				return CG_HUD_ParseError(handle, "expected cursorhints");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "weaponstability"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.weaponstability))
			{
				return CG_HUD_ParseError(handle, "expected weaponstability");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "livesleft"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.livesleft))
			{
				return CG_HUD_ParseError(handle, "expected livesleft");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "roundtimer"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.roundtimer))
			{
				return CG_HUD_ParseError(handle, "expected livesleft");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "reinforcment"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.reinforcment))
			{
				return CG_HUD_ParseError(handle, "expected livesleft");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "spawntimer"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.spawntimer))
			{
				return CG_HUD_ParseError(handle, "expected livesleft");
			}
			continue;
		}

		if (!Q_stricmp(token.string, "localtime"))
		{
			if (!CG_ParseHudComponent(handle, &temphud.localtime))
			{
				return CG_HUD_ParseError(handle, "expected livesleft");
			}
			continue;
		}

		return CG_HUD_ParseError(handle, "unexpected token: %s", token.string);
	}

	if (CG_isHudNumberAvailable(temphud.hudnumber))
	{
		Com_Printf("...properties for hud %i have been read.\n", temphud.hudnumber);
		CG_addHudToList(temphud);
	}
	else
	{
		Com_Printf("^1Hud with number: %i already exists!\n", temphud.hudnumber);
	}

	return qtrue;
}

static qboolean CG_ReadHudFile(const char *filename)
{
	pc_token_t token;
	int        handle;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "hudDef"))
	{
		return CG_HUD_ParseError(handle, "expected 'hudDef'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_HUD_ParseError(handle, "expected '{'");
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

		if (!Q_stricmp(token.string, "hud"))
		{
			if (!CG_ParseHUD(handle))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_HUD_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

void CG_ReadHudScripts(void)
{
	if (!CG_ReadHudFile("ui/huds.hud"))
	{
		Com_Printf("^1ERROR while reading hud file\n");
	}

	Com_Printf("...hud count: %i\n", hudCount);
}

// HUD DRAWING FUNCTIONS BELLOW

vec4_t HUD_Background = { 0.16f, 0.2f, 0.17f, 0.8f };
vec4_t HUD_Border = { 0.5f, 0.5f, 0.5f, 0.5f };
vec4_t HUD_Text = { 0.6f, 0.6f, 0.6f, 1.0f };

static void CG_DrawPicShadowed(float x, float y, float w, float h, qhandle_t icon)
{
	trap_R_SetColor(colorBlack);
	CG_DrawPic(x + 2, y + 2, w, h, icon);
	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, w, h, icon);
}

static void CG_DrawPlayerStatusHead(hudComponent_t comp)
{
	hudHeadAnimNumber_t anim           = cg.idleAnim;
	bg_character_t      *character     = CG_CharacterForPlayerstate(&cg.snap->ps);
	bg_character_t      *headcharacter = BG_GetCharacter(cgs.clientinfo[cg.snap->ps.clientNum].team, cgs.clientinfo[cg.snap->ps.clientNum].cls);
	qhandle_t           painshader     = 0;
	rectDef_t           *headRect      = &comp.location;

	if (!comp.visible)
	{
		return;
	}

	if (cg.weaponFireTime > 500)
	{
		anim = HD_ATTACK;
	}
	else if (cg.time - cg.lastFiredWeaponTime < 500)
	{
		anim = HD_ATTACK_END;
	}
	else if (cg.time - cg.painTime < (character->hudheadanimations[HD_PAIN].numFrames * character->hudheadanimations[HD_PAIN].frameLerp))
	{
		anim = HD_PAIN;
	}
	else if (cg.time > cg.nextIdleTime)
	{
		cg.nextIdleTime = cg.time + 7000 + rand() % 1000;
		if (cg.snap->ps.stats[STAT_HEALTH] < 40)
		{
			cg.idleAnim = (rand() % (HD_DAMAGED_IDLE3 - HD_DAMAGED_IDLE2 + 1)) + HD_DAMAGED_IDLE2;
		}
		else
		{
			cg.idleAnim = (rand() % (HD_IDLE8 - HD_IDLE2 + 1)) + HD_IDLE2;
		}

		cg.lastIdleTimeEnd = cg.time + character->hudheadanimations[cg.idleAnim].numFrames * character->hudheadanimations[cg.idleAnim].frameLerp;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] < 5)
	{
		painshader = cgs.media.hudDamagedStates[3];
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] < 20)
	{
		painshader = cgs.media.hudDamagedStates[2];
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] < 40)
	{
		painshader = cgs.media.hudDamagedStates[1];
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] < 60)
	{
		painshader = cgs.media.hudDamagedStates[0];
	}

	if (cg.time > cg.lastIdleTimeEnd)
	{
		if (cg.snap->ps.stats[STAT_HEALTH] < 40)
		{
			cg.idleAnim = HD_DAMAGED_IDLE1;
		}
		else
		{
			cg.idleAnim = HD_IDLE1;
		}
	}

	CG_DrawPlayerHead(headRect, character, headcharacter, 180, 0, cg.snap->ps.eFlags & EF_HEADSHOT ? qfalse : qtrue, anim, painshader, cgs.clientinfo[cg.snap->ps.clientNum].rank, qfalse, cgs.clientinfo[cg.snap->ps.clientNum].team);
}

static int CG_PlayerAmmoValue(int *ammo, int *clips, int *akimboammo)
{
	centity_t     *cent;
	playerState_t *ps;
	int           weap;
	qboolean      skipammo = qfalse;

	*ammo = *clips = *akimboammo = -1;

	if (cg.snap->ps.clientNum == cg.clientNum)
	{
		cent = &cg.predictedPlayerEntity;
	}
	else
	{
		cent = &cg_entities[cg.snap->ps.clientNum];
	}
	ps = &cg.snap->ps;

	weap = cent->currentState.weapon;

	if (!weap)
	{
		return weap;
	}

	switch (weap)          // some weapons don't draw ammo count text
	{
	case WP_AMMO:
	case WP_MEDKIT:
	case WP_KNIFE:
	case WP_KNIFE_KABAR:
	case WP_PLIERS:
	case WP_SMOKE_MARKER:
	case WP_DYNAMITE:
	case WP_SATCHEL:
	case WP_SATCHEL_DET:
	case WP_SMOKE_BOMB:
	case WP_BINOCULARS:
		return weap;

	case WP_LANDMINE:
	case WP_MEDIC_SYRINGE:
	case WP_MEDIC_ADRENALINE:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_FLAMETHROWER:
	case WP_MORTAR:
	case WP_MORTAR2:
	case WP_MORTAR_SET:
	case WP_MORTAR2_SET:
	case WP_PANZERFAUST:
	case WP_BAZOOKA:
		skipammo = qtrue;
		break;

	default:
		break;
	}

	if ((cg.snap->ps.eFlags & EF_MG42_ACTIVE) || (cg.snap->ps.eFlags & EF_MOUNTEDTANK) || (cg.snap->ps.eFlags & EF_AAGUN_ACTIVE))
	{
		if (cg_entities[cg_entities[cg_entities[cg.snap->ps.clientNum].tagParent].tankparent].currentState.density & 8)
		{
			return WP_MOBILE_BROWNING;
		}
		else
		{
			return WP_MOBILE_MG42;
		}
	}

	// total ammo in clips
	*clips = cg.snap->ps.ammo[BG_FindAmmoForWeapon(weap)];

	// current clip
	*ammo = ps->ammoclip[BG_FindClipForWeapon(weap)];

	if (IS_AKIMBO_WEAPON(weap))
	{
		*akimboammo = ps->ammoclip[BG_FindClipForWeapon(weaponTable[weap].akimboSideram)];
	}
	else
	{
		*akimboammo = -1;
	}

	if (weap == WP_LANDMINE)
	{
		if (!cgs.gameManager)
		{
			*ammo = 0;
		}
		else
		{
			if (cgs.clientinfo[ps->clientNum].team == TEAM_AXIS)
			{
				*ammo = cgs.gameManager->currentState.otherEntityNum;
			}
			else
			{
				*ammo = cgs.gameManager->currentState.otherEntityNum2;
			}
		}
	}
	else if (IS_MORTAR_WEAPON(weap) || IS_PANZER_WEAPON(weap))
	{
		*ammo += *clips;
	}

	if (skipammo)
	{
		*clips = -1;
	}

	return weap;
}

vec4_t bgcolor = { 1.f, 1.f, 1.f, .3f };    // bars backgound

static void CG_DrawPlayerHealthBar(rectDef_t *rect)
{
	vec4_t colour;
	int    flags = 1 | 4 | 16 | 64;
	float  frac;

	CG_ColorForHealth(colour);
	colour[3] = 0.5f;

	if (cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_MEDIC)
	{
		frac = cg.snap->ps.stats[STAT_HEALTH] / ((float) cg.snap->ps.stats[STAT_MAX_HEALTH] * 1.12f);
	}
	else
	{
		frac = cg.snap->ps.stats[STAT_HEALTH] / (float) cg.snap->ps.stats[STAT_MAX_HEALTH];
	}

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, colour, NULL, bgcolor, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x, rect->y + rect->h + 4, rect->w, rect->w, cgs.media.hudHealthIcon);
}

static void CG_DrawStaminaBar(rectDef_t *rect)
{
	vec4_t colour = { 0.1f, 1.0f, 0.1f, 0.5f };
	vec_t  *color = colour;
	int    flags  = 1 | 4 | 16 | 64;
	float  frac   = cg.snap->ps.stats[STAT_SPRINTTIME] / (float)SPRINTTIME;

	if (cg.snap->ps.powerups[PW_ADRENALINE])
	{
		if (cg.snap->ps.pm_flags & PMF_FOLLOW)
		{
			Vector4Average(colour, colorWhite, sin(cg.time * .005f), colour);
		}
		else
		{
			float msec = cg.snap->ps.powerups[PW_ADRENALINE] - cg.time;

			if (msec < 0)
			{
				msec = 0;
			}
			else
			{
				Vector4Average(colour, colorMdRed, .5f + sin(.2f * sqrt(msec) * 2 * M_PI) * .5f, colour);
			}
		}
	}
	else
	{
		color[0] = 1.0f - frac;
		color[1] = frac;
	}

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, color, NULL, bgcolor, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x, rect->y + rect->h + 4, rect->w, rect->w, cgs.media.hudSprintIcon);
}

// draw the breath bar
static void CG_DrawBreathBar(rectDef_t *rect)
{
	static vec4_t colour = { 0.1f, 0.1f, 1.0f, 0.5f };
	vec_t         *color = colour;
	int           flags  = 1 | 4 | 16 | 64;
	float         frac   = cg.snap->ps.stats[STAT_AIRLEFT] / (float)HOLDBREATHTIME;

	color[0] = 1.0f - frac;
	color[2] = frac;

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, color, NULL, bgcolor, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x, rect->y + rect->h + 4, rect->w, rect->w, cgs.media.waterHintShader);
}

// draw weapon recharge bar
static void CG_DrawWeapRecharge(rectDef_t *rect)
{
	float    barFrac, chargeTime;
	int      flags  = 1 | 4 | 16;
	qboolean charge = qtrue;
	vec4_t   color;

	// Draw power bar
	switch (cg.snap->ps.stats[STAT_PLAYER_CLASS])
	{
	case PC_ENGINEER:
		chargeTime = cg.engineerChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_MEDIC:
		chargeTime = cg.medicChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_FIELDOPS:
		chargeTime = cg.fieldopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_COVERTOPS:
		chargeTime = cg.covertopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	default:
		chargeTime = cg.soldierChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	}

	// display colored charbar if charge bar isn't full enough
	// FIXME: put chargeTime factor in weapon table?
	switch (cg.predictedPlayerState.weapon)
	{
	case WP_PANZERFAUST:
	case WP_BAZOOKA:
		if (cgs.clientinfo[cg.clientNum].skill[SK_HEAVY_WEAPONS] >= 1)
		{
			if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.66f)
			{
				charge = qfalse;
			}
		}
		else if (cg.time - cg.snap->ps.classWeaponTime < chargeTime)
		{
			charge = qfalse;
		}
		break;
	case WP_GPG40:
	case WP_M7:
		if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.5f)
		{
			charge = qfalse;
		}
		break;
	case WP_MORTAR_SET:
	case WP_MORTAR2_SET:
		if (cgs.clientinfo[cg.clientNum].skill[SK_HEAVY_WEAPONS] >= 1)
		{
			if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.33f)
			{
				charge = qfalse;
			}
		}
		else if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.5f)
		{
			charge = qfalse;
		}
		break;
	case WP_SMOKE_BOMB:
	case WP_SATCHEL:
		if (cgs.clientinfo[cg.clientNum].skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 2)
		{
			if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.66f)
			{
				charge = qfalse;
			}
		}
		else if (cg.time - cg.snap->ps.classWeaponTime < chargeTime)
		{
			charge = qfalse;
		}
		break;
	case WP_LANDMINE:
		if (cgs.clientinfo[cg.clientNum].skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 2)
		{
			if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.33f)
			{
				charge = qfalse;
			}
		}
		else if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.5f)
		{
			charge = qfalse;
		}
		break;
	case WP_DYNAMITE:
		if (cgs.clientinfo[cg.clientNum].skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 3)
		{
			if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.66f)
			{
				charge = qfalse;
			}
		}
		else if (cg.time - cg.snap->ps.classWeaponTime < chargeTime)
		{
			charge = qfalse;
		}
		break;
	case WP_AMMO:
		if (cgs.clientinfo[cg.clientNum].skill[SK_SIGNALS] >= 1)
		{
			if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.15f)
			{
				charge = qfalse;
			}
		}
		else if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.25f)
		{
			charge = qfalse;
		}
		break;
	case WP_MEDKIT:
		if (cgs.clientinfo[cg.clientNum].skill[SK_SIGNALS] >= 2)
		{
			if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.15f)
			{
				charge = qfalse;
			}
		}
		else if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.25f)
		{
			charge = qfalse;
		}
		break;
	case WP_SMOKE_MARKER:
	case WP_BINOCULARS:
		if (cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS) // only fieldops binocs
		{
			if (cgs.clientinfo[cg.clientNum].skill[SK_FIRST_AID] >= 2)
			{
				if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * 0.66f)
				{
					charge = qfalse;
				}
			}
			else if (cg.time - cg.snap->ps.classWeaponTime < chargeTime)
			{
				charge = qfalse;
			}
		}
		break;
	case WP_MEDIC_ADRENALINE:
		if (cg.time - cg.snap->ps.classWeaponTime < chargeTime)
		{
			charge = qfalse;
		}
		break;
	default:
		break;
	}

	barFrac = (cg.time - cg.snap->ps.classWeaponTime) / chargeTime; // FIXME: potential DIV 0 when charge times are set to 0!

	if (barFrac > 1.0)
	{
		barFrac = 1.0;
	}

	if (!charge)
	{
		color[0] = 1.0f;
		color[1] = color[2] = 0.1f;
		color[3] = 0.5f;
	}
	else
	{
		color[0] = 1.0f;
		color[1] = color[2] = barFrac;
		color[3] = 0.25 + barFrac * 0.5;
	}

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, color, NULL, bgcolor, barFrac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);

	if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS)
	{
		if ((cg.predictedPlayerState.weapon == WP_SMOKE_MARKER && (cg.snap->ps.ammo[WP_ARTY] & NO_AIRSTRIKE)))
		{
			trap_R_SetColor(colorRed);
		}
		else if ((cg.predictedPlayerState.weapon != WP_SMOKE_MARKER && (cg.snap->ps.ammo[WP_ARTY] & NO_ARTILLERY)))
		{
			trap_R_SetColor(colorRed);
		}
		CG_DrawPic(rect->x + (rect->w * 0.25f) - 1, rect->y + rect->h + 4, (rect->w * 0.5f) + 2, rect->w + 2, cgs.media.hudPowerIcon);
		trap_R_SetColor(NULL);
	}
	else
	{
		CG_DrawPic(rect->x + (rect->w * 0.25f) - 1, rect->y + rect->h + 4, (rect->w * 0.5f) + 2, rect->w + 2, cgs.media.hudPowerIcon);
	}
}

static void CG_DrawGunIcon(rectDef_t location)
{
	rectDef_t rect = location;

	// Draw weapon icon and overheat bar
	CG_DrawWeapHeat(&rect, HUD_HORIZONTAL);

	// drawn the common white icon, usage of mounted weapons don't change cg.snap->ps.weapon for real
	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		CG_DrawPlayerWeaponIcon(&rect, qtrue, ITEM_ALIGN_RIGHT, &colorWhite);
		return;
	}

	if (
#if FEATURE_MULTIVIEW
	    cg.mvTotalClients < 1 &&
#endif
	    cg_drawWeaponIconFlash.integer == 0)
	{
		CG_DrawPlayerWeaponIcon(&rect, qtrue, ITEM_ALIGN_RIGHT, &colorWhite);
	}
	else
	{
		int ws =
#if FEATURE_MULTIVIEW
		    (cg.mvTotalClients > 0) ? cgs.clientinfo[cg.snap->ps.clientNum].weaponState :
#endif
		    BG_simpleWeaponState(cg.snap->ps.weaponstate);

		CG_DrawPlayerWeaponIcon(&rect, (ws != WSTATE_IDLE), ITEM_ALIGN_RIGHT, ((ws == WSTATE_SWITCH || ws == WSTATE_RELOAD) ? &colorYellow : (ws == WSTATE_FIRE) ? &colorRed : &colorWhite));
	}
}

static void CG_DrawAmmoCount(float x, float y)
{
	int  value, value2, value3;
	char buffer[32];

	// Draw ammo
	CG_PlayerAmmoValue(&value, &value2, &value3);
	if (value3 >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i|%i/%i", value3, value, value2);
		CG_Text_Paint_Ext(x - CG_Text_Width_Ext(buffer, .25f, 0, &cgs.media.limboFont1), y, .25f, .25f, colorWhite, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	else if (value2 >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i/%i", value, value2);
		CG_Text_Paint_Ext(x - CG_Text_Width_Ext(buffer, .25f, 0, &cgs.media.limboFont1), y, .25f, .25f, colorWhite, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	else if (value >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i", value);
		CG_Text_Paint_Ext(x - CG_Text_Width_Ext(buffer, .25f, 0, &cgs.media.limboFont1), y, .25f, .25f, colorWhite, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
}

static void CG_DrawSkillBar(float x, float y, float w, float h, int skill)
{
	int    i;
	float  blockheight = (h - 4) / (float)(NUM_SKILL_LEVELS - 1);
	float  draw_y      = y + h - blockheight;
	vec4_t colour;
	float  x1, y1, w1, h1;

	for (i = 0; i < NUM_SKILL_LEVELS - 1; i++)
	{
		if (i >= skill)
		{
			Vector4Set(colour, 1.f, 1.f, 1.f, .15f);
		}
		else
		{
			Vector4Set(colour, 0.f, 0.f, 0.f, .4f);
		}

		CG_FillRect(x, draw_y, w, blockheight, colour);

		if (i < skill)
		{
			x1 = x;
			y1 = draw_y;
			w1 = w;
			h1 = blockheight;
			CG_AdjustFrom640(&x1, &y1, &w1, &h1);

			trap_R_DrawStretchPic(x1, y1, w1, h1, 0, 0, 1.f, 0.5f, cgs.media.limboStar_roll);
		}

		CG_DrawRect_FixedBorder(x, draw_y, w, blockheight, 1, colorBlack);
		draw_y -= (blockheight + 1);
	}
}

extern pmove_t *pm;

skillType_t CG_ClassSkillForPosition(clientInfo_t *ci, int pos)
{
	switch (pos)
	{
	case 0:
		return BG_ClassSkillForClass(ci->cls);
	case 1:
		return SK_BATTLE_SENSE;
	case 2:
		// draw soldier level if using a heavy weapon instead of light weapons icon
		if ((pm && (pm->ps->persistant[PERS_HWEAPON_USE] || pm->ps->eFlags & EF_MOUNTEDTANK || IS_HEAVY_WEAPON(pm->ps->weapon))) && ci->cls != PC_SOLDIER)
		{
			return SK_SOLDIER;
		}
		return SK_LIGHT_WEAPONS;
	}

	return SK_BATTLE_SENSE;
}

static void CG_DrawPlayerHealth(float x, float y)
{
	const char *str = va("%i", cg.snap->ps.stats[STAT_HEALTH]);
	float      w    = CG_Text_Width_Ext(str, 0.25f, 0, &cgs.media.limboFont1);

	CG_Text_Paint_Ext(x - w, y, 0.25f, 0.25f, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + 2, y, 0.2f, 0.2f, colorWhite, "HP", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

static void CG_DrawSkills(hudComponent_t comp)
{
	playerState_t *ps = &cg.snap->ps;
	clientInfo_t  *ci = &cgs.clientinfo[ps->clientNum];
	skillType_t   skill;
	int           i;
	float         temp;

	for (i = 0; i < 3; i++)
	{
		skill = CG_ClassSkillForPosition(ci, i);
		if (comp.style == STYLE_NORMAL)
		{
			CG_DrawSkillBar(i * SKILL_BAR_X_SCALE + SKILL_BAR_X, SCREEN_HEIGHT - (5 * SKILL_BAR_Y_SCALE) + SKILL_BAR_Y, SKILL_BAR_WIDTH, 4 * SKILL_ICON_SIZE, ci->skill[skill]);
			CG_DrawPic(i * SKILL_ICON_X_SCALE + SKILL_ICON_X, SCREEN_HEIGHT + SKILL_ICON_Y, SKILL_ICON_SIZE, SKILL_ICON_SIZE, cgs.media.skillPics[skill]);
		}
		else
		{
			temp = comp.location.y + (i * SKILL_ICON_SIZE * 1.7f);
			//CG_DrawPic
			CG_DrawPicShadowed(comp.location.x, temp, SKILL_ICON_SIZE, SKILL_ICON_SIZE, cgs.media.skillPics[skill]);
			CG_Text_Paint_Ext(comp.location.x + 3, temp + 24, 0.25f, 0.25f, colorWhite, va("%i", ci->skill[skill]), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
	}
}

static void CG_DrawXP(float x, float y)
{
	const char *str;
	float      w;
	vec_t      *clr;

	if (cg.time - cg.xpChangeTime < 1000)
	{
		clr = colorYellow;
	}
	else
	{
		clr = colorWhite;
	}

	str = va("%i", (32768 * cg.snap->ps.stats[STAT_XP_OVERFLOW]) + cg.snap->ps.stats[STAT_XP]);
	w   = CG_Text_Width_Ext(str, 0.25f, 0, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x - w, y, 0.25f, 0.25f, clr, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + 2, y, 0.2f, 0.2f, clr, "XP", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

static void CG_DrawPowerUps(rectDef_t rect)
{
	playerState_t *ps = &cg.snap->ps;

	// draw treasure icon if we have the flag
	if (ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG])
	{
		trap_R_SetColor(NULL);
		CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveShader);
	}
	else if (ps->powerups[PW_OPS_DISGUISED])       // Disguised?
	{
		CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		// show the class to the client
		CG_DrawPic(rect.x + 9, rect.y + 9, 18, 18, cgs.media.skillPics[BG_ClassSkillForClass((cg_entities[ps->clientNum].currentState.powerups >> PW_OPS_CLASS_1) & 7)]);
	}
}

int CG_DrawField(int x, int y, int width, int value, int charWidth, int charHeight, qboolean dodrawpic, qboolean leftAlign)
{
	char num[16], *ptr;
	int  l;
	int  frame;
	int  startx;

	if (width < 1)
	{
		return 0;
	}

	// draw number string
	if (width > 5)
	{
		width = 5;
	}

	switch (width)
	{
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf(num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
	{
		l = width;
	}

	if (!leftAlign)
	{
		x -= 2 + charWidth * (l);
	}

	startx = x;

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
		{
			frame = STAT_MINUS;
		}
		else
		{
			frame = *ptr - '0';
		}

		if (dodrawpic)
		{
			CG_DrawPic(x, y, charWidth, charHeight, cgs.media.numberShaders[frame]);
		}
		x += charWidth;
		ptr++;
		l--;
	}

	return startx;
}

void CG_DrawLivesLeft(hudComponent_t comp)
{
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] < 0)
	{
		return;
	}

	CG_DrawPic(4, 360, 48, 24, cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.hudAlliedHelmet : cgs.media.hudAxisHelmet);

	CG_DrawField(44, 360, 3, cg.snap->ps.persistant[PERS_RESPAWNS_LEFT], 14, 20, qtrue, qtrue);
}

static char statsDebugStrings[6][512];
static int  statsDebugTime[6];
static int  statsDebugTextWidth[6];
static int  statsDebugPos;

void CG_InitStatsDebug(void)
{
	memset(&statsDebugStrings, 0, sizeof(statsDebugStrings));
	memset(&statsDebugTime, 0, sizeof(statsDebugTime));
	statsDebugPos = -1;
}

void CG_StatsDebugAddText(const char *text)
{
	if (cg_debugSkills.integer)
	{
		statsDebugPos++;

		if (statsDebugPos >= 6)
		{
			statsDebugPos = 0;
		}

		Q_strncpyz(statsDebugStrings[statsDebugPos], text, 512);
		statsDebugTime[statsDebugPos]      = cg.time;
		statsDebugTextWidth[statsDebugPos] = CG_Text_Width_Ext(text, .15f, 0, &cgs.media.limboFont2);

		CG_Printf("%s\n", text);
	}
}

/*
=================
CG_DrawCompassIcon
=================
*/
void CG_DrawCompassIcon(float x, float y, float w, float h, vec3_t origin, vec3_t dest, qhandle_t shader)
{
	float  angle, pi2 = M_PI * 2;
	vec3_t v1, angles;
	float  len;

	VectorCopy(dest, v1);
	VectorSubtract(origin, v1, v1);
	len = VectorLength(v1);
	VectorNormalize(v1);
	vectoangles(v1, angles);

	if (v1[0] == 0 && v1[1] == 0 && v1[2] == 0)
	{
		return;
	}

	angles[YAW] = AngleSubtract(cg.predictedPlayerState.viewangles[YAW], angles[YAW]);

	angle = ((angles[YAW] + 180.f) / 360.f - (0.50 / 2.f)) * pi2;

	w /= 2;
	h /= 2;

	x += w;
	y += h;

	{
		w = sqrt((w * w) + (h * h)) / 3.f * 2.f * 0.9f;
	}

	x = x + (cos(angle) * w);
	y = y + (sin(angle) * w);

	len = 1 - MIN(1.f, len / 2000.f);

	CG_DrawPic(x - (14 * len + 4) / 2, y - (14 * len + 4) / 2, 14 * len + 8, 14 * len + 8, shader);
}

static void CG_CompasMoveLocationCalc(float *locationvalue, qboolean directionplus, qboolean animationout)
{
	if (animationout)
	{
		if (directionplus)
		{
			*locationvalue += ((cg.time - cgs.autoMapExpandTime) / 100.f) * 128.f;
		}
		else
		{
			*locationvalue -= ((cg.time - cgs.autoMapExpandTime) / 100.f) * 128.f;
		}
	}
	else
	{
		if (!directionplus)
		{
			*locationvalue += (((cg.time - cgs.autoMapExpandTime - 150.f) / 100.f) * 128.f) - 128.f;
		}
		else
		{
			*locationvalue -= (((cg.time - cgs.autoMapExpandTime - 150.f) / 100.f) * 128.f) - 128.f;
		}
	}
}

static void CG_CompasMoveLocation(float *basex, float *basey, qboolean animationout)
{
	float x    = *basex;
	float y    = *basey;
	float cent = activehud->compas.location.w / 2;
	x += cent;
	y += cent;

	if (x < Ccg_WideX(320))
	{
		if (y < 240)
		{
			if (x < y)
			{
				//move left
				CG_CompasMoveLocationCalc(basex, qfalse, animationout);
			}
			else
			{
				//move up
				CG_CompasMoveLocationCalc(basey, qfalse, animationout);
			}
		}
		else
		{
			if (x < (SCREEN_HEIGHT - y))
			{
				//move left
				CG_CompasMoveLocationCalc(basex, qfalse, animationout);
			}
			else
			{
				//move down
				CG_CompasMoveLocationCalc(basey, qtrue, animationout);
			}
		}
	}
	else
	{
		if (y < 240)
		{
			if ((Ccg_WideX(SCREEN_WIDTH) - x) < y)
			{
				//move right
				CG_CompasMoveLocationCalc(basex, qtrue, animationout);
			}
			else
			{
				//move up
				CG_CompasMoveLocationCalc(basey, qfalse, animationout);
			}
		}
		else
		{
			if ((Ccg_WideX(SCREEN_WIDTH) - x) < (SCREEN_HEIGHT - y))
			{
				//move right
				CG_CompasMoveLocationCalc(basex, qtrue, animationout);
			}
			else
			{
				//move down
				CG_CompasMoveLocationCalc(basey, qtrue, animationout);
			}
		}
	}
}

/*
=================
CG_DrawNewCompass
=================
*/
static void CG_DrawNewCompass(rectDef_t location)
{
	float        basex = location.x, basey = location.y - 16, basew = location.w, baseh = location.h;
	snapshot_t   *snap;
	float        angle;
	int          i;
	static float lastangle  = 0;
	static float anglespeed = 0;
	float        diff;

	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport)
	{
		snap = cg.nextSnap;
	}
	else
	{
		snap = cg.snap;
	}

	if (snap->ps.pm_flags & PMF_LIMBO /*|| snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR*/
#if FEATURE_MULTIVIEW
	    || cg.mvTotalClients > 0
#endif
	    )
	{
		CG_DrawExpandedAutoMap();
		return;
	}

	diff = basew * 0.25f;

	if (cgs.autoMapExpanded)
	{
		if (cg.time - cgs.autoMapExpandTime < 100.f)
		{
			CG_CompasMoveLocation(&basex, &basey, qtrue);
		}
		else
		{
			CG_DrawExpandedAutoMap();
			return;
		}
	}
	else
	{
		if (cg.time - cgs.autoMapExpandTime <= 150.f)
		{
			CG_DrawExpandedAutoMap();
			return;
		}
		else if ((cg.time - cgs.autoMapExpandTime > 150.f) && (cg.time - cgs.autoMapExpandTime < 250.f))
		{
			CG_CompasMoveLocation(&basex, &basey, qfalse);
		}
	}

	if (snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || !cg_drawCompass.integer)
	{
		return;
	}

	CG_DrawAutoMap(basex + (diff / 2), basey + (diff / 2), basew - diff, baseh - diff);
	CG_DrawPic(basex + 4, basey + 4, basew - 8, baseh - 8, cgs.media.compassShader);

	angle       = (cg.predictedPlayerState.viewangles[YAW] + 180.f) / 360.f - (0.125f);
	diff        = AngleSubtract(angle * 360, lastangle * 360) / 360.f;
	anglespeed /= 1.08f;
	anglespeed += diff * 0.01f;
	if (Q_fabs(anglespeed) < 0.00001f)
	{
		anglespeed = 0;
	}
	lastangle += anglespeed;
	CG_DrawRotatedPic(basex + 4, basey + 4, basew - 8, baseh - 8, cgs.media.compass2Shader, lastangle);

	//if( !(cgs.ccFilter & CC_FILTER_REQUESTS) ) {
	// draw voice chats
	{
		centity_t *cent;

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			cent = &cg_entities[i];

			if (cg.predictedPlayerState.clientNum == i || !cgs.clientinfo[i].infoValid || cg.predictedPlayerState.persistant[PERS_TEAM] != cgs.clientinfo[i].team)
			{
				continue;
			}

			// also draw revive icons if cent is dead and player is a medic
			if (cent->voiceChatSpriteTime < cg.time)
			{
				continue;
			}

			if (cgs.clientinfo[i].health <= 0)
			{
				// reset
				cent->voiceChatSpriteTime = cg.time;
				continue;
			}

			CG_DrawCompassIcon(basex, basey, basew, baseh, cg.predictedPlayerState.origin, cent->lerpOrigin, cent->voiceChatSprite);
		}
	}
	//}

	/*if( !(cgs.ccFilter & CC_FILTER_DESTRUCTIONS) ) {
	    // draw explosives if an engineer
	    if ( cg.predictedPlayerState.stats[ STAT_PLAYER_CLASS ] == PC_ENGINEER ) {
	        for ( i = 0; i < snap->numEntities; i++ ) {
	            centity_t *cent = &cg_entities[ snap->entities[ i ].number ];

	            if ( cent->currentState.eType != ET_EXPLOSIVE_INDICATOR ) {
	                continue;
	            }

	            if ( cent->currentState.teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS )
	                continue;
	            else if ( cent->currentState.teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES )
	                continue;

	            CG_DrawCompassIcon( basex, basey, basew, baseh, cg.predictedPlayerState.origin, cent->lerpOrigin, cgs.media.compassDestroyShader );
	        }
	    }
	}*/

	{
		entityState_t *ent;

		for (i = 0; i < snap->numEntities; i++)
		{
			ent = &snap->entities[i];

			if (ent->eType != ET_PLAYER)
			{
				continue;
			}

			if (ent->eFlags & EF_DEAD)
			{
				if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_MEDIC && ent->number == ent->clientNum) // && !(cgs.ccFilter & CC_FILTER_REQUESTS)
				{
					if (!cgs.clientinfo[ent->clientNum].infoValid || cg.predictedPlayerState.persistant[PERS_TEAM] != cgs.clientinfo[ent->clientNum].team)
					{
						continue;
					}

					CG_DrawCompassIcon(basex, basey, basew, baseh, cg.predictedPlayerState.origin, ent->pos.trBase, cgs.media.medicReviveShader);
				}

				continue;
			}

			if (!cgs.clientinfo[ent->clientNum].infoValid || cg.predictedPlayerState.persistant[PERS_TEAM] != cgs.clientinfo[ent->clientNum].team)
			{
				continue;
			}

			if (!CG_IsOnSameFireteam(cg.clientNum, ent->clientNum))
			{
				continue;
			}

			CG_DrawCompassIcon(basex, basey, basew, baseh, cg.predictedPlayerState.origin, ent->pos.trBase, cgs.media.buddyShader); //if( !(cgs.ccFilter & CC_FILTER_BUDDIES) ) {
		}
	}
}

static void CG_DrawStatsDebug(void)
{
	int textWidth = 0;
	int i, x, y, w, h;

	if (!cg_debugSkills.integer)
	{
		return;
	}

	for (i = 0; i < 6; i++)
	{
		if (statsDebugTime[i] + 9000 > cg.time)
		{
			if (statsDebugTextWidth[i] > textWidth)
			{
				textWidth = statsDebugTextWidth[i];
			}
		}
	}

	w = textWidth + 6;
	h = 9;
	x = SCREEN_WIDTH - w;
	y = (SCREEN_HEIGHT - 5 * (12 + 2) + 6 - 4) - 6 - h;     // don't ask

	i = statsDebugPos;

	do
	{
		vec4_t colour;

		if (statsDebugTime[i] + 9000 <= cg.time)
		{
			break;
		}

		colour[0] = colour[1] = colour[2] = .5f;
		if (cg.time - statsDebugTime[i] > 5000)
		{
			colour[3] = .5f - .5f * ((cg.time - statsDebugTime[i] - 5000) / 4000.f);
		}
		else
		{
			colour[3] = .5f ;
		}
		CG_FillRect(x, y, w, h, colour);

		colour[0] = colour[1] = colour[2] = 1.f;
		if (cg.time - statsDebugTime[i] > 5000)
		{
			colour[3] = 1.f - ((cg.time - statsDebugTime[i] - 5000) / 4000.f);
		}
		else
		{
			colour[3] = 1.f ;
		}
		CG_Text_Paint_Ext(640.f - 3 - statsDebugTextWidth[i], y + h - 2, .15f, .15f, colour, statsDebugStrings[i], 0, 0, ITEM_TEXTSTYLE_NORMAL, &cgs.media.limboFont2);

		y -= h;

		i--;
		if (i < 0)
		{
			i = 6 - 1;
		}
	}
	while (i != statsDebugPos);
}

/*
===========================================================================================
  UPPER RIGHT CORNER
===========================================================================================
*/

#define UPPERRIGHT_X 634
#define UPPERRIGHT_W 50
/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot(float y)
{
	char *s = va("t:%i sn:%i cmd:%i", cg.snap->serverTime, cg.latestSnapshotNum, cgs.serverCommandSequence);
	int  w  = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);
	int  w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;
	int  x  = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;

	CG_FillRect(x, y, w2 + 5, 12 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 12 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);
	return y + 12 + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define MAX_FPS_FRAMES  500

static float CG_DrawFPS(float y)
{
	static int previousTimes[MAX_FPS_FRAMES];
	static int previous;
	static int index;
	static int oldSamples;
	char       *s;
	int        t         = trap_Milliseconds(); // don't use serverTime, because that will be drifting to correct for internet lag changes, timescales, timedemos, etc
	int        frameTime = t - previous;
	int        x, w, w2;
	int        samples = cg_drawFPS.integer;

	previous = t;

	if (samples < 4)
	{
		samples = 4;
	}
	if (samples > MAX_FPS_FRAMES)
	{
		samples = MAX_FPS_FRAMES;
	}
	if (samples != oldSamples)
	{
		index = 0;
	}

	oldSamples                     = samples;
	previousTimes[index % samples] = frameTime;
	index++;

	if (index > samples)
	{
		int i, fps;
		// average multiple frames together to smooth changes out a bit
		int total = 0;

		for (i = 0 ; i < samples ; ++i)
		{
			total += previousTimes[i];
		}

		total = total ? total : 1;

		fps = 1000 * samples / total;

		s = va("%i FPS", fps);
	}
	else
	{
		s = "estimating";
	}

	w  = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;

	x = (int)(Ccg_WideX(UPPERRIGHT_X)) - w2 - 2;
	CG_FillRect(x, y, w2 + 5, 12 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 12 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);

	return y + 12 + 4;
}

/*
=================
CG_DrawTimersAlt
=================
*/
static void CG_DrawTimersAlt(rectDef_t *respawn, rectDef_t *spawntimer, rectDef_t *localtime, rectDef_t *roundtimer)
{
	char    *s, *rt;
	qtime_t time;
	vec4_t  color = { 0.625f, 0.625f, 0.6f, 1.0f };
	int     tens;
	int     msec    = (cgs.timelimit * 60.f * 1000.f) - (cg.time - cgs.levelStartTime);
	int     seconds = msec / 1000;
	int     mins    = seconds / 60;

	seconds -= mins * 60;
	tens     = seconds / 10;
	seconds -= tens * 10;

	if (cgs.gamestate != GS_PLAYING)
	{
		s        = va("^7%s", CG_TranslateString("WARMUP")); // don't draw reinforcement time in warmup mode // ^*
		color[3] = fabs(sin(cg.time * 0.002));
	}
	else if (msec < 0 && cgs.timelimit > 0.0f)
	{
		s        = "^N0:00";
		color[3] = fabs(sin(cg.time * 0.002));
	}
	else
	{
		color[3] = 1.f;
		if (cgs.timelimit > 0.0f)
		{
			CG_Text_Paint_Ext(roundtimer->x, roundtimer->y, 0.19f, 0.19f, color, va("^7%i:%i%i", mins, tens, seconds), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}

		if (cgs.gametype != GT_WOLF_LMS && (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW)) && cg_drawReinforcementTime.integer > 0)
		{
			int reinfTime = CG_CalculateReinfTime(qfalse);

			rt = va("%s%d%s", (reinfTime <= 2 && cgs.clientinfo[cg.clientNum].health == 0) ? "^3" : "^F", reinfTime, ((cgs.timelimit <= 0.0f) ? "" : " "));
		}
		else
		{
			rt = "";
		}

		s = va("%s", rt);
	}
	CG_Text_Paint_Ext(respawn->x, respawn->y, 0.19f, 0.19f, color, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	// spawntimer
	if (cg_spawnTimer_set.integer != -1 && cg_spawnTimer_period.integer > 0 && cgs.gamestate == GS_PLAYING)
	{
		seconds = msec / 1000;
		s       = va("^1%d", cg_spawnTimer_period.integer + (seconds - cg_spawnTimer_set.integer) % cg_spawnTimer_period.integer);
		CG_Text_Paint_Ext(spawntimer->x, spawntimer->y, 0.19f, 0.19f, color, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	else if (cg_spawnTimer_set.integer != -1 && cg_spawnTimer_period.integer > 0 && cgs.gamestate != GS_PLAYING)
	{
		//We are not playing and the timer is set so reset/disable it
		trap_Cvar_Set("cg_spawnTimer_set", "-1");
	}
	// end spawntimer

	if (cg_drawTime.integer & LOCALTIME_ON)
	{
		qboolean pmtime = qfalse;

		// Fetch the local time
		trap_RealTime(&time);

		if (cg_drawTime.integer & LOCALTIME_SECOND)
		{
			if (cg_drawTime.integer & LOCALTIME_12HOUR)
			{
				if (time.tm_hour > 12)
				{
					pmtime = qtrue;
				}
				s = va("%i:%02i:%02i %s", (pmtime ? time.tm_hour - 12 : time.tm_hour), time.tm_min, time.tm_sec, (pmtime ? "PM" : "AM"));
			}
			else
			{
				s = va("%02i:%02i:%02i", time.tm_hour, time.tm_min, time.tm_sec);
			}
		}
		else
		{
			if (cg_drawTime.integer & LOCALTIME_12HOUR)
			{
				if (time.tm_hour > 12)
				{
					pmtime = qtrue;
				}
				s = va("%i:%02i %s", (pmtime ? time.tm_hour - 12 : time.tm_hour), time.tm_min, (pmtime ? "PM" : "AM"));
			}
			else
			{
				s = va("%02i:%02i", time.tm_hour, time.tm_min);
			}
		}
		CG_Text_Paint_Ext(localtime->x, localtime->y, 0.19f, 0.19f, color, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
}

/*
=================
CG_DrawTimerNormal
=================
*/
static float CG_DrawTimerNormal(float y)
{
	vec4_t color = { .6f, .6f, .6f, 1.f };
	char   *s, *rt;
	int    w, w2;
	int    tens;
	int    x;
	int    msec    = (cgs.timelimit * 60.f * 1000.f) - (cg.time - cgs.levelStartTime);
	int    seconds = msec / 1000;
	int    mins    = seconds / 60;

	seconds -= mins * 60;
	tens     = seconds / 10;
	seconds -= tens * 10;

	if (cgs.gamestate != GS_PLAYING)
	{
		s        = va("^7%s", CG_TranslateString("WARMUP")); // don't draw reinforcement time in warmup mode // ^*
		color[3] = fabs(sin(cg.time * 0.002));
	}
	else if (msec < 0 && cgs.timelimit > 0.0f)
	{
		s        = "^N0:00";
		color[3] = fabs(sin(cg.time * 0.002));
	}
	else
	{
		if (cgs.gametype != GT_WOLF_LMS && (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW)) && cg_drawReinforcementTime.integer > 0)
		{
			int reinfTime = CG_CalculateReinfTime(qfalse);

			rt = va("%s%d%s", (reinfTime <= 2 && cgs.clientinfo[cg.clientNum].health == 0) ? "^3" : "^F", reinfTime, ((cgs.timelimit <= 0.0f) ? "" : " "));
		}
		else
		{
			rt = "";
		}

		if (cgs.timelimit <= 0.0f)
		{
			s = va("%s", rt);
		}
		else
		{
			s = va("%s^7%i:%i%i", rt, mins, tens, seconds);  // ^*
		}

		color[3] = 1.f;
	}

	// spawntimer
	if (cg_spawnTimer_set.integer != -1 && cg_spawnTimer_period.integer > 0 && cgs.gamestate == GS_PLAYING)
	{
		seconds = msec / 1000;
		s       = va("^1%d %s", cg_spawnTimer_period.integer + (seconds - cg_spawnTimer_set.integer) % cg_spawnTimer_period.integer, s);
	}
	else if (cg_spawnTimer_set.integer != -1 && cg_spawnTimer_period.integer > 0 && cgs.gamestate != GS_PLAYING)
	{
		// we are not playing and the timer is set so reset/disable it
		trap_Cvar_Set("cg_spawnTimer_set", "-1");
	}

	w  = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;

	x = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;
	CG_FillRect(x, y, w2 + 5, 12 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 12 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, color, s, 0, 0, 0, &cgs.media.limboFont1);

	return y + 12 + 4;
}

static float CG_DrawLocalTime(float y)
{
	qtime_t  time;
	int      w, w2, x;
	char     *s;
	qboolean pmtime = qfalse;

	//Fetch the local time
	trap_RealTime(&time);

	if (cg_drawTime.integer & LOCALTIME_SECOND)
	{
		if (cg_drawTime.integer & LOCALTIME_12HOUR)
		{
			if (time.tm_hour > 12)
			{
				pmtime = qtrue;
			}
			s = va("%i:%02i:%02i %s", (pmtime ? time.tm_hour - 12 : time.tm_hour), time.tm_min, time.tm_sec, (pmtime ? "PM" : "AM"));
		}
		else
		{
			s = va("%02i:%02i:%02i", time.tm_hour, time.tm_min, time.tm_sec);
		}
	}
	else
	{
		if (cg_drawTime.integer & LOCALTIME_12HOUR)
		{
			if (time.tm_hour > 12)
			{
				pmtime = qtrue;
			}
			s = va("%i:%02i %s", (pmtime ? time.tm_hour - 12 : time.tm_hour), time.tm_min, (pmtime ? "PM" : "AM"));
		}
		else
		{
			s = va("%02i:%02i", time.tm_hour, time.tm_min);
		}
	}

	w  = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;

	x = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;
	CG_FillRect(x, y, w2 + 5, 12 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 12 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);

	return y + 12 + 4;
}

/*
===============================================================================
LAGOMETER
===============================================================================
*/

#define LAG_SAMPLES     128
#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

typedef struct
{
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/**
 * @brief Adds the current interpolate / extrapolate bar for this frame
 */
void CG_AddLagometerFrameInfo(void)
{
	lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = cg.time - cg.latestSnapshotTime;
	lagometer.frameCount++;
}

/**
 * @brief Log the ping time and number of dropped snapshots before it each time a snapshot is received
 */
void CG_AddLagometerSnapshotInfo(snapshot_t *snap)
{
	// dropped packet
	if (!snap)
	{
		lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = -1;
		lagometer.snapshotCount++;
		return;
	}

	if (cg.demoPlayback)
	{
		snap->ping = (snap->serverTime - snap->ps.commandTime) - 50;
	}

	// add this snapshot's info
	if (cg.demoPlayback)
	{
		static int lasttime = 0;

		// display snapshot time delta instead of ping
		lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->serverTime - lasttime;
		lasttime                                                               = snap->serverTime;
	}
	else
	{
		lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->ping;
	}
	lagometer.snapshotFlags[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/**
 * @brief Draw disconnect icon for long lag
 */
static float CG_DrawDisconnect(float y)
{
	int        cmdNum, w, w2, x;
	usercmd_t  cmd;
	const char *s;

	// use same dimension as timer
	w  = CG_Text_Width_Ext("xx:xx:xx", 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;
	x  = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;

	// dont draw if a demo and we're running at a different timescale
	if (cg.demoPlayback && cg_timescale.value != 1.0f)
	{
		return y + w2 + 13;
	}

	// don't draw if the server is respawning
	if (cg.serverRespawning)
	{
		return y + w2 + 13;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime
	    || cmd.serverTime > cg.time)        // special check for map_restart
	{
		return y + w2 + 13;
	}

	// also add text in center of screen
	s = CG_TranslateString("Connection Interrupted");
	w = CG_Text_Width_Ext(s, cg_fontScaleTP.value, 0, &cgs.media.limboFont2);
	CG_Text_Paint_Ext(Ccg_WideX(320) - w / 2, 100, cg_fontScaleTP.value, cg_fontScaleTP.value, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

	// blink the icon
	if ((cg.time >> 9) & 1)
	{
		return y + w2 + 13;
	}

	CG_DrawPic(x + 1, y + 1, w2 + 3, w2 + 3, cgs.media.disconnectIcon);
	return y + w2 + 13;
}

/**
 * @brief Draw ping
 */
static float CG_DrawPing(float y)
{
	int  curPing = cg.snap->ping;
	int  w, w2, x;
	char *s;

	s = va("Ping %d", curPing < 999 ? curPing : 999);
	w = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);

	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;

	x = (int)(Ccg_WideX(UPPERRIGHT_X)) - w2 - 2;
	CG_FillRect(x, y, w2 + 5, 12 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 12 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);

	return y + 12 + 4;
}

/**
 * @brief Draw the lagometer
 */
static float CG_DrawLagometer(float y)
{
	int   a, w, w2, x, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int   color;
	float vscale;

	// use same dimension as timer
	w  = CG_Text_Width_Ext("xx:xx:xx", 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;
	x  = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;

	// draw the graph
	trap_R_SetColor(NULL);
	CG_FillRect(x, y, w2 + 5, w2 + 5, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, w2 + 5, 1, HUD_Border);

	ax = x + 1;
	ay = y + 1;
	aw = w2 + 3;
	ah = w2 + 3;
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	color = -1;
	range = ah / 3;
	mid   = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for (a = 0 ; a < aw ; a++)
	{
		i  = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v  = lagometer.frameSamples[i];
		v *= vscale;
		if (v > 0)
		{
			if (color != 1)
			{
				color = 1;
				trap_R_SetColor(colorYellow);
			}
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 2)
			{
				color = 2;
				trap_R_SetColor(colorBlue);
			}
			v = -v;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range  = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for (a = 0 ; a < aw ; a++)
	{
		i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if (v > 0)
		{
			if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED)
			{
				if (color != 5)
				{
					color = 5;  // YELLOW for rate delay
					trap_R_SetColor(colorYellow);
				}
			}
			else
			{
				if (color != 3)
				{
					color = 3;
					trap_R_SetColor(colorGreen);
				}
			}
			v = v * vscale;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 4)
			{
				color = 4;      // RED for dropped snapshots
				trap_R_SetColor(colorRed);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(NULL);

	if (cg_nopredict.integer
#ifdef ALLOW_GSYNC
	    || cg_synchronousClients.integer
#endif // ALLOW_GSYNC
	    )
	{
		CG_Text_Paint_Ext(ax, ay, cg_fontScaleTP.value, cg_fontScaleTP.value, colorWhite, "snc", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}

	// don't draw if a demo and we're running at a different timescale, or if the server is respawning
	if (!cg.demoPlayback && !cg.serverRespawning)
	{
		CG_DrawDisconnect(y);
	}

	return y + w + 13;
}

/**
 * @brief Draw the player status
 */
static void CG_DrawPlayerStatus(void)
{
	if (activehud->weaponicon.visible)
	{
		CG_DrawGunIcon(activehud->weaponicon.location);
	}

	if (activehud->weaponammo.visible)
	{
		CG_DrawAmmoCount(activehud->weaponammo.location.x, activehud->weaponammo.location.y);
	}

	if (activehud->healthbar.visible)
	{
		CG_DrawPlayerHealthBar(&activehud->healthbar.location);
	}

	if (activehud->staminabar.visible)
	{
		qboolean underwater = qfalse;

		// check if we are underwater.
		// This check has changed to make it work for spectators following another player.
		// That's why ps.stats[STAT_AIRLEFT] has been added..
		//
		// While following high-pingers, You sometimes see the breathbar, even while they are not submerged..
		// So we check for underwater status differently when we are following others.
		// (It doesn't matter to do a more complex check for spectators.. they are not playing)
		if (cg.snap->ps.pm_flags & PMF_FOLLOW)
		{
			vec3_t origin;

			VectorCopy(cg.snap->ps.origin, origin);
			origin[2] += 36;
			underwater = (CG_PointContents(origin, cg.snap->ps.clientNum) & CONTENTS_WATER);
		}
		else
		{
			underwater = (cg.snap->ps.stats[STAT_AIRLEFT] < HOLDBREATHTIME) ? qtrue : qfalse;
		}

		if (underwater)
		{
			CG_DrawBreathBar(&activehud->breathbar.location);
		}
		else
		{
			CG_DrawStaminaBar(&activehud->staminabar.location);
		}
	}

	if (activehud->weaponchargebar.visible)
	{
		CG_DrawWeapRecharge(&activehud->weaponchargebar.location);
	}
}

/**
 * @brief Draw the player stats
 */
static void CG_DrawPlayerStats(void)
{
	if (activehud->healthtext.visible)
	{
		CG_DrawPlayerHealth(activehud->healthtext.location.x, activehud->healthtext.location.y);
	}

	if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (activehud->statsdisplay.visible)
	{
		CG_DrawSkills(activehud->statsdisplay);
	}

	if (activehud->xptext.visible)
	{
		CG_DrawXP(activehud->xptext.location.x, activehud->xptext.location.y);
	}

	if (activehud->powerups.visible)
	{
		CG_DrawPowerUps(activehud->powerups.location);
	}
}

void CG_Hud_Setup(void)
{
	hudStucture_t hud1;
	hudStucture_t hud2;

	// Hud0 aka the Default hud
	CG_setDefaultHudValues(&hud0);
	activehud = &hud0;
	CG_addHudToList(hud0);

	// Hud1
	hud1.hudnumber       = 1;
	hud1.compas          = CG_getComponent(44, SCREEN_HEIGHT - 75, 72, 72, qtrue, STYLE_NORMAL);
	hud1.staminabar      = CG_getComponent(4, 388, 12, 72, qtrue, STYLE_NORMAL);
	hud1.breathbar       = CG_getComponent(4, 388, 12, 72, qtrue, STYLE_NORMAL);
	hud1.healthbar       = CG_getComponent((Ccg_WideX(SCREEN_WIDTH) - 36), 388, 12, 72, qtrue, STYLE_NORMAL);
	hud1.weaponchargebar = CG_getComponent((Ccg_WideX(SCREEN_WIDTH) - 16), 388, 12, 72, qtrue, STYLE_NORMAL);
	hud1.healthtext      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 60, SCREEN_HEIGHT - 65, 0, 0, qtrue, STYLE_NORMAL);
	hud1.xptext          = CG_getComponent(28, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL);
	hud1.statsdisplay    = CG_getComponent(24, SCREEN_HEIGHT - 95, 0, 0, qtrue, STYLE_SIMPLE);
	hud1.weaponicon      = CG_getComponent((Ccg_WideX(SCREEN_WIDTH) - 82 - 20), (SCREEN_HEIGHT - 56), 60, 32, qtrue, STYLE_NORMAL);
	hud1.weaponammo      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 22 - 20, SCREEN_HEIGHT - 1 * (16 + 2) + 12 - 4, 0, 0, qtrue, STYLE_NORMAL);
	hud1.fireteam        = CG_getComponent(Ccg_WideX(SCREEN_WIDTH), 10, 100, 100, qtrue, STYLE_NORMAL);
	hud1.popupmessages   = CG_getComponent(4, 100, 72, 72, qtrue, STYLE_NORMAL);
	hud1.powerups        = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 40, SCREEN_HEIGHT - 140, 36, 36, qtrue, STYLE_NORMAL);
	hud1.hudhead         = CG_getComponent(44, SCREEN_HEIGHT - 92, 62, 80, qfalse, STYLE_NORMAL);
	hud1.cursorhint      = CG_getComponent(.5f * SCREEN_WIDTH - .5f * 48, 260, 48, 48, qtrue, STYLE_NORMAL);
	hud1.weaponstability = CG_getComponent(50, 208, 10, 64, qtrue, STYLE_NORMAL);
	hud1.livesleft       = CG_getComponent(0, 0, 0, 0, qtrue, STYLE_NORMAL);
	hud1.reinforcment    = CG_getComponent(55, SCREEN_HEIGHT - 12, 0, 0, qtrue, STYLE_NORMAL);
	hud1.roundtimer      = CG_getComponent(75, SCREEN_HEIGHT - 12, 0, 0, qtrue, STYLE_NORMAL);
	hud1.spawntimer      = CG_getComponent(55, SCREEN_HEIGHT - 2, 0, 0, qtrue, STYLE_NORMAL);
	hud1.localtime       = CG_getComponent(75, SCREEN_HEIGHT - 2, 0, 0, qtrue, STYLE_NORMAL);
	CG_addHudToList(hud1);

	// Hud2
	hud2.hudnumber       = 2;
	hud2.compas          = CG_getComponent(64, SCREEN_HEIGHT - 75, 72, 72, qtrue, STYLE_NORMAL);
	hud2.staminabar      = CG_getComponent(4, 388, 12, 72, qtrue, STYLE_NORMAL);
	hud2.breathbar       = CG_getComponent(4, 388, 12, 72, qtrue, STYLE_NORMAL);
	hud2.healthbar       = CG_getComponent(24, 388, 12, 72, qtrue, STYLE_NORMAL);
	hud2.weaponchargebar = CG_getComponent((Ccg_WideX(SCREEN_WIDTH) - 16), 388, 12, 72, qtrue, STYLE_NORMAL);
	hud2.healthtext      = CG_getComponent(65, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL);
	hud2.xptext          = CG_getComponent(120, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL);
	hud2.statsdisplay    = CG_getComponent(44, SCREEN_HEIGHT - 95, 0, 0, qtrue, STYLE_SIMPLE);
	hud2.weaponicon      = CG_getComponent((Ccg_WideX(SCREEN_WIDTH) - 82), (SCREEN_HEIGHT - 56), 60, 32, qtrue, STYLE_NORMAL);
	hud2.weaponammo      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 22, SCREEN_HEIGHT - 1 * (16 + 2) + 12 - 4, 0, 0, qtrue, STYLE_NORMAL);
	hud2.fireteam        = CG_getComponent(Ccg_WideX(SCREEN_WIDTH), 10, 100, 100, qtrue, STYLE_NORMAL);
	hud2.popupmessages   = CG_getComponent(4, 100, 72, 72, qtrue, STYLE_NORMAL);
	hud2.powerups        = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 40, SCREEN_HEIGHT - 140, 36, 36, qtrue, STYLE_NORMAL);
	hud2.hudhead         = CG_getComponent(44, SCREEN_HEIGHT - 92, 62, 80, qfalse, STYLE_NORMAL);
	hud2.cursorhint      = CG_getComponent(.5f * SCREEN_WIDTH - .5f * 48, 260, 48, 48, qtrue, STYLE_NORMAL);
	hud2.weaponstability = CG_getComponent(50, 208, 10, 64, qtrue, STYLE_NORMAL);
	hud2.livesleft       = CG_getComponent(0, 0, 0, 0, qtrue, STYLE_NORMAL);
	hud2.reinforcment    = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 70, SCREEN_HEIGHT - 70, 0, 0, qtrue, STYLE_NORMAL);
	hud2.roundtimer      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 55, SCREEN_HEIGHT - 70, 0, 0, qtrue, STYLE_NORMAL);
	hud2.spawntimer      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 70, SCREEN_HEIGHT - 60, 0, 0, qtrue, STYLE_NORMAL);
	hud2.localtime       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 55, SCREEN_HEIGHT - 60, 0, 0, qtrue, STYLE_NORMAL);
	CG_addHudToList(hud2);

	// Read the hud files
	CG_ReadHudScripts();
}

static void CG_PrintHudComponent(const char *name, hudComponent_t comp)
{
	Com_Printf("%s location: X %.f Y %.f W %.f H %.f visible: %i\n", name, comp.location.x, comp.location.y, comp.location.w, comp.location.h, comp.visible);
}

static void CG_PrintHud(hudStucture_t *hud)
{
	CG_PrintHudComponent("compas", hud->compas);
	CG_PrintHudComponent("staminabar", hud->staminabar);
	CG_PrintHudComponent("breathbar", hud->breathbar);
	CG_PrintHudComponent("healthbar", hud->healthbar);
	CG_PrintHudComponent("weaponchargebar", hud->weaponchargebar);
	CG_PrintHudComponent("healthtext", hud->healthtext);
	CG_PrintHudComponent("xptext", hud->xptext);
	CG_PrintHudComponent("statsdisplay", hud->statsdisplay);
	CG_PrintHudComponent("weaponicon", hud->weaponicon);
	CG_PrintHudComponent("weaponammo", hud->weaponammo);
	CG_PrintHudComponent("fireteam", hud->fireteam);
	CG_PrintHudComponent("popupmessages", hud->popupmessages);
	CG_PrintHudComponent("powerups", hud->powerups);
	CG_PrintHudComponent("hudhead", hud->hudhead);
	CG_PrintHudComponent("cursorhint", hud->cursorhint);
	CG_PrintHudComponent("weaponstability", hud->weaponstability);
	CG_PrintHudComponent("livesleft", hud->livesleft);
}

/*
=====================
CG_SetHud
=====================
*/
void CG_SetHud(void)
{
	if (cg_altHud.integer && activehud->hudnumber != cg_altHud.integer)
	{
		activehud = CG_getHudByNumber(cg_altHud.integer);
		if (!activehud)
		{
			Com_Printf("^1ERROR hud with number %i is not available, defaulting to 0\n", cg_altHud.integer);
			activehud = &hud0;
			trap_Cvar_Set("cg_altHud", "0");
			return;
		}

#ifdef LEGACY_DEBUG
		CG_PrintHud(activehud);
#endif

		Com_Printf("Setting hud to: %i\n", cg_altHud.integer);
	}
	else if (!cg_altHud.integer && activehud->hudnumber != hud0.hudnumber)
	{
		activehud = &hud0;
	}
}

/*
=====================
CG_DrawActiveHud
=====================
*/
void CG_DrawActiveHud(void)
{
	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
		if (activehud->hudhead.visible)
		{
			CG_DrawPlayerStatusHead(activehud->hudhead);
		}
		CG_DrawPlayerStatus();
		CG_DrawPlayerStats();
	}

	CG_DrawLivesLeft(activehud->livesleft);

	// Cursor hint
	if (activehud->cursorhint.visible)
	{
		CG_DrawCursorhint(&activehud->cursorhint.location);
	}

	// Stability bar
	if (activehud->weaponstability.visible)
	{
		CG_DrawWeapStability(&activehud->weaponstability.location);
	}

	// Stats Debugging
	CG_DrawStatsDebug();
}

/*
=====================
CG_DrawGlobalHud
=====================
*/
void CG_DrawGlobalHud(void)
{
	if (cg_altHudFlags.integer & FLAGS_MOVE_POPUPS)
	{
		CG_DrawPMItems(activehud->popupmessages.location, (cg_altHudFlags.integer & FLAGS_POPUPS_SHADOW ? ITEM_TEXTSTYLE_SHADOWED : 0));
	}
	else
	{
		CG_DrawPMItems(hud0.popupmessages.location, (cg_altHudFlags.integer & FLAGS_POPUPS_SHADOW ? ITEM_TEXTSTYLE_SHADOWED : 0));
	}

	if (!(cg_altHudFlags.integer & FLAGS_REMOVE_RANKS))
	{
		CG_DrawPMItemsBig();
	}

	CG_DrawNewCompass(activehud->compas.location);
}

/*
=====================
CG_DrawUpperRight
=====================
*/
void CG_DrawUpperRight(void)
{
	int y = 152; // 20 + 100 + 32;

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		if (cg_drawFireteamOverlay.integer && CG_IsOnFireteam(cg.clientNum))
		{
			if (cg_altHudFlags.integer & FLAGS_MOVE_POPUPS)
			{
				CG_DrawFireTeamOverlay(&activehud->fireteam.location);
			}
			else
			{
				CG_DrawFireTeamOverlay(&hud0.fireteam.location);
			}
		}

		if (!(cg.snap->ps.pm_flags & PMF_LIMBO) && (cgs.autoMapExpanded || (!cgs.autoMapExpanded && (cg.time - cgs.autoMapExpandTime < 250.f))))
		{
			return;
		}
	}

	if (cg_drawRoundTimer.integer)
	{
		if (cg_altHudFlags.integer & FLAGS_MOVE_TIMERS)
		{
			CG_DrawTimersAlt(&activehud->reinforcment.location, &activehud->spawntimer.location, &activehud->localtime.location, &activehud->roundtimer.location);
		}
		else
		{
			y = CG_DrawTimerNormal(y);
		}
	}

	if (cg_drawFPS.integer)
	{
		y = CG_DrawFPS(y);
	}

	if (cg_drawSnapshot.integer)
	{
		y = CG_DrawSnapshot(y);
	}

	if ((cg_drawTime.integer & LOCALTIME_ON) && !(cg_altHudFlags.integer & FLAGS_MOVE_TIMERS))
	{
		y = CG_DrawLocalTime(y);
	}

	if (cg_drawPing.integer)
	{
		y = CG_DrawPing(y);
	}

	if (cg_lagometer.integer && !cg.serverRespawning)
	{
		y = CG_DrawLagometer(y);
	}
}
