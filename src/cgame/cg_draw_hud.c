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
 * @file cg_game_hud.c
 * @brief draws the players hud
 *
 */

#include "cg_local.h"

typedef struct hudStructure_s
{
	rectDef_t compas;

	rectDef_t staminabar;
	rectDef_t healthbar;
	rectDef_t weaponchargebar;

	rectDef_t healthtext;
	rectDef_t xptext;

	rectDef_t statsdisplay;
	qboolean statssimple;

	rectDef_t weaponicon;
	rectDef_t weaponammo;
} hudStucture_t;

static void CG_DrawPlayerStatusHead(void)
{
	hudHeadAnimNumber_t anim           = cg.idleAnim;
	rectDef_t           headRect       = { 44, 480 - 92, 62, 80 };
	bg_character_t      *character     = CG_CharacterForPlayerstate(&cg.snap->ps);
	bg_character_t      *headcharacter = BG_GetCharacter(cgs.clientinfo[cg.snap->ps.clientNum].team, cgs.clientinfo[cg.snap->ps.clientNum].cls);
	qhandle_t           painshader     = 0;

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

	CG_DrawPlayerHead(&headRect, character, headcharacter, 180, 0, cg.snap->ps.eFlags & EF_HEADSHOT ? qfalse : qtrue, anim, painshader, cgs.clientinfo[cg.snap->ps.clientNum].rank, qfalse);
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
	case WP_MORTAR_SET:
	case WP_PANZERFAUST:
		skipammo = qtrue;
		break;

	default:
		break;
	}

	if (cg.snap->ps.eFlags & EF_MG42_ACTIVE || cg.snap->ps.eFlags & EF_MOUNTEDTANK)
	{
		return WP_MOBILE_MG42;
	}

	// total ammo in clips
	*clips = cg.snap->ps.ammo[BG_FindAmmoForWeapon(weap)];

	// current clip
	*ammo = ps->ammoclip[BG_FindClipForWeapon(weap)];

	if (BG_IsAkimboWeapon(weap))
	{
		*akimboammo = ps->ammoclip[BG_FindClipForWeapon(BG_AkimboSidearm(weap))];
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
	else if (weap == WP_MORTAR || weap == WP_MORTAR_SET || weap == WP_PANZERFAUST)
	{
		*ammo += *clips;
	}

	if (skipammo)
	{
		*clips = -1;
	}

	return weap;
}

static void CG_DrawPlayerHealthBar(rectDef_t *rect)
{
	vec4_t bgcolour = { 1.f, 1.f, 1.f, 0.3f };
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

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, colour, NULL, bgcolour, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x, rect->y + rect->h + 4, rect->w, rect->w, cgs.media.hudHealthIcon);
}

static void CG_DrawStaminaBar(rectDef_t *rect)
{
	vec4_t bgcolour  = { 1.f, 1.f, 1.f, 0.3f };
	vec4_t colour    = { 0.1f, 1.0f, 0.1f, 0.5f };
	vec4_t colourlow = { 1.0f, 0.1f, 0.1f, 0.5f };
	vec_t  *color    = colour;
	int    flags     = 1 | 4 | 16 | 64;
	float  frac      = cg.pmext.sprintTime / (float)SPRINTTIME;

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
				Vector4Average(colour, colorWhite, .5f + sin(.2f * sqrt(msec) * 2 * M_PI) * .5f, colour);
			}
		}
	}
	else
	{
		if (frac < 0.25)
		{
			color = colourlow;
		}
	}

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, color, NULL, bgcolour, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x, rect->y + rect->h + 4, rect->w, rect->w, cgs.media.hudSprintIcon);
}

static void CG_DrawWeapRecharge(rectDef_t *rect)
{
	float    barFrac, chargeTime;
	int      flags;
	qboolean fade    = qfalse;
	vec4_t   bgcolor = { 1.0f, 1.0f, 1.0f, 0.25f };
	vec4_t   color;

	flags = 1 | 4 | 16;

	// Draw power bar
	if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_ENGINEER)
	{
		chargeTime = cg.engineerChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	}
	else if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC)
	{
		chargeTime = cg.medicChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	}
	else if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS)
	{
		chargeTime = cg.ltChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	}
	else if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS)
	{
		chargeTime = cg.covertopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	}
	else
	{
		chargeTime = cg.soldierChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	}

	barFrac = (float)(cg.time - cg.snap->ps.classWeaponTime) / chargeTime;
	if (barFrac > 1.0)
	{
		barFrac = 1.0;
	}

	color[0] = 1.0f;
	color[1] = color[2] = barFrac;
	color[3] = 0.25 + barFrac * 0.5;

	if (fade)
	{
		bgcolor[3] *= 0.4f;
		color[3]   *= 0.4;
	}

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, color, NULL, bgcolor, barFrac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x + (rect->w * 0.25f) - 1, rect->y + rect->h + 4, (rect->w * 0.5f) + 2, rect->w + 2, cgs.media.hudPowerIcon);
}

static void CG_DrawGunIcon(rectDef_t location)
{
	rectDef_t rect = location;
	// Draw weapon icon and overheat bar
	CG_DrawWeapHeat(&rect, HUD_HORIZONTAL);
	if (
#if FEATURE_MULTIVIEW
	    cg.mvTotalClients < 1 &&
#endif
	    cg_drawWeaponIconFlash.integer == 0)
	{
		CG_DrawPlayerWeaponIcon(&rect, qtrue, ITEM_ALIGN_RIGHT, &colorWhite);
	}
	else if (cg_drawWeaponIconFlash.integer == 2)  // ETPro style
	{
		int ws =
#if FEATURE_MULTIVIEW
		    (cg.mvTotalClients > 0) ? cgs.clientinfo[cg.snap->ps.clientNum].weaponState :
#endif
		    BG_simpleWeaponState(cg.snap->ps.weaponstate);

		CG_DrawPlayerWeaponIcon(&rect, (ws != WSTATE_IDLE), ITEM_ALIGN_RIGHT, ((ws == WSTATE_SWITCH || ws == WSTATE_RELOAD) ? &colorYellow : (ws == WSTATE_FIRE) ? &colorRed : &colorWhite));
	}
	else
	{
		int ws =
#if FEATURE_MULTIVIEW
		    (cg.mvTotalClients > 0) ? cgs.clientinfo[cg.snap->ps.clientNum].weaponState :
#endif
		    BG_simpleWeaponState(cg.snap->ps.weaponstate);
		CG_DrawPlayerWeaponIcon(&rect, (ws != WSTATE_IDLE), ITEM_ALIGN_RIGHT, ((ws == WSTATE_SWITCH) ? &colorWhite : (ws == WSTATE_FIRE) ? &colorRed : &colorYellow));
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

static void CG_DrawPlayerStatus(void)
{
	rectDef_t rect = { (Ccg_WideX(640) - 82), (480 - 56), 60, 32 };

	CG_DrawGunIcon(rect);

	CG_DrawAmmoCount(Ccg_WideX(640) - 22, 480 - 1 * (16 + 2) + 12 - 4);

	rect.x = 24;
	rect.y = 480 - 92;
	rect.w = 12;
	rect.h = 72;
	CG_DrawPlayerHealthBar(&rect);

	rect.x = 4;
	rect.y = 480 - 92;
	rect.w = 12;
	rect.h = 72;
	CG_DrawStaminaBar(&rect);

	rect.x = Ccg_WideX(640) - 16;
	rect.y = 480 - 92;
	rect.w = 12;
	rect.h = 72;
	CG_DrawWeapRecharge(&rect);
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

skillType_t CG_ClassSkillForPosition(clientInfo_t *ci, int pos)
{
	switch (pos)
	{
	case 0:
		return BG_ClassSkillForClass(ci->cls);
	case 1:
		return SK_BATTLE_SENSE;
	case 2:
		return SK_LIGHT_WEAPONS;
	}

	return SK_BATTLE_SENSE;
}

static void CG_DrawPlayerHealth(float x, float y)
{
	const char *str;
	float      w;
	str = va("%i", cg.snap->ps.stats[STAT_HEALTH]);
	w   = CG_Text_Width_Ext(str, 0.25f, 0, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x - w, y, 0.25f, 0.25f, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + 2, y, 0.2f, 0.2f, colorWhite, "HP", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

static void CG_DrawSkills(float x, float y)
{
	playerState_t *ps;
	clientInfo_t  *ci;
	skillType_t   skill;
	int           i;
	float         temp;

	ps = &cg.snap->ps;
	ci = &cgs.clientinfo[ps->clientNum];

	for (i = 0; i < 3; i++)
	{
		skill = CG_ClassSkillForPosition(ci, i);
		if (!cg_altHud.integer)
		{
			CG_DrawSkillBar(i * SKILL_BAR_X_SCALE + SKILL_BAR_X, 480 - (5 * SKILL_BAR_Y_SCALE) + SKILL_BAR_Y, SKILL_BAR_WIDTH, 4 * SKILL_ICON_SIZE, ci->skill[skill]);
			CG_DrawPic(i * SKILL_ICON_X_SCALE + SKILL_ICON_X, 480 + SKILL_ICON_Y, SKILL_ICON_SIZE, SKILL_ICON_SIZE, cgs.media.skillPics[skill]);
		}
		else
		{
			temp = 480 - 90 + (i * SKILL_ICON_SIZE * 1.7f);
			CG_DrawPic(SKILL_ICON_X, temp, SKILL_ICON_SIZE, SKILL_ICON_SIZE, cgs.media.skillPics[skill]);
			CG_Text_Paint_Ext(SKILL_ICON_X + 2, temp + 24, 0.25f, 0.25f, colorWhite, va("%i", ci->skill[skill]), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
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
	playerState_t *ps;

	ps = &cg.snap->ps;
	// draw treasure icon if we have the flag
	// rain - #274 - use the playerstate instead of the clientinfo
	if (ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG])
	{
		trap_R_SetColor(NULL);
		CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveShader);
	}
	else if (ps->powerups[PW_OPS_DISGUISED])       // Disguised?
	{
		CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
	}
}

static void CG_DrawPlayerStats(void)
{
	rectDef_t rect = { Ccg_WideX(640) - 40, 480 - 140, 36, 36 };

	CG_DrawPlayerHealth(SKILLS_X - 28, 480 - 4);

	if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	CG_DrawSkills(0, 0);

	CG_DrawXP(SKILLS_X + 28, 480 - 4);

	CG_DrawPowerUps(rect);
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

void CG_DrawLivesLeft(void)
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

	if (snap->ps.pm_flags & PMF_LIMBO || snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
#if FEATURE_MULTIVIEW
	    || cg.mvTotalClients > 0
#endif
	    )
	{
		return;
	}

	diff = basew * 0.25f;
	CG_DrawAutoMap(basex + (diff / 2), basey + (diff / 2), basew - diff, baseh - diff);

	if (cgs.autoMapExpanded)
	{
		if (cg.time - cgs.autoMapExpandTime < 100.f)
		{
			basey -= ((cg.time - cgs.autoMapExpandTime) / 100.f) * 128.f;
		}
		else
		{
			//basey -= 128.f;
			return;
		}
	}
	else
	{
		if (cg.time - cgs.autoMapExpandTime <= 150.f)
		{
			//basey -= 128.f;
			return;
		}
		else if ((cg.time - cgs.autoMapExpandTime > 150.f) && (cg.time - cgs.autoMapExpandTime < 250.f))
		{

			basey = (basey - 128.f) + ((cg.time - cgs.autoMapExpandTime - 150.f) / 100.f) * 128.f;
		}
	}

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
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		centity_t *cent = &cg_entities[i];

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

	//if( !(cgs.ccFilter & CC_FILTER_REQUESTS) ) {
	// draw revive medic icons
	if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_MEDIC)
	{
		entityState_t *ent;

		for (i = 0; i < snap->numEntities; i++)
		{
			ent = &snap->entities[i];

			if (ent->eType != ET_PLAYER)
			{
				continue;
			}

			if ((ent->eFlags & EF_DEAD) && ent->number == ent->clientNum)
			{
				if (!cgs.clientinfo[ent->clientNum].infoValid || cg.predictedPlayerState.persistant[PERS_TEAM] != cgs.clientinfo[ent->clientNum].team)
				{
					continue;
				}

				CG_DrawCompassIcon(basex, basey, basew, baseh, cg.predictedPlayerState.origin, ent->pos.trBase, cgs.media.medicReviveShader);
			}
		}
	}
	//}

	//if( !(cgs.ccFilter & CC_FILTER_BUDDIES) ) {
	for (i = 0; i < snap->numEntities; i++)
	{
		entityState_t *ent = &snap->entities[i];

		if (ent->eType != ET_PLAYER)
		{
			continue;
		}

		if (ent->eFlags & EF_DEAD)
		{
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

		CG_DrawCompassIcon(basex, basey, basew, baseh, cg.predictedPlayerState.origin, ent->pos.trBase, cgs.media.buddyShader);
	}
	//}
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
	x = 640 - w;
	y = (480 - 5 * (12 + 2) + 6 - 4) - 6 - h;     // don't ask

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

void CG_DrawActiveHud(void)
{
	rectDef_t rect;

	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
		if (!cg_altHud.integer)
		{
			CG_DrawPlayerStatusHead();
		}
		CG_DrawPlayerStatus();
		CG_DrawPlayerStats();
	}

	CG_DrawLivesLeft();

	// Cursor hint
	rect.w = rect.h = 48;
	rect.x = .5f * SCREEN_WIDTH - .5f * rect.w;
	rect.y = 260;
	CG_DrawCursorhint(&rect);

	// Stability bar
	rect.x = 50;
	rect.y = 208;
	rect.w = 10;
	rect.h = 64;
	CG_DrawWeapStability(&rect);

	// Stats Debugging
	CG_DrawStatsDebug();
}

void CG_DrawGlobalHud(void)
{
	rectDef_t rect;
	/*
	const hudStucture_t hud0 = {
	    {(Ccg_WideX(640) - 100 - 20 - 16),20 - 16,100 + 32,100 + 32}, //Compas location
	    {4,480 - 92,12,72}, //Stamina bar location
	    {24,480 - 92,12,72}, //Health bar location
	    {Ccg_WideX(640) - 16,480 - 92,12,72}, //Weapon charge bar location
	    {0,0,0,0}, //Health text location
	    {0,0,0,0}, //XP text location
	    {0,0,0,0}, //Status display
	    qfalse, //Status display simple
	    {0,0,0,0}, //Weapon icon location
	    {0,0,0,0} //Weapon ammo location
	};
	const hudStucture_t hud1 = {
	{504,4,132,132}, //Compas location
	{4,388,12,72}, //Stamina bar location
	{24,388,12,72}, //Health bar location
	{624,388,12,72}, //Weapon charge bar location
	{0,0,0,0}, //Health text location
	{0,0,0,0}, //XP text location
	{0,0,0,0}, //Status display
	qfalse, //Status display simple
	{0,0,0,0}, //Weapon icon location
	{0,0,0,0} //Weapon ammo location
	};
	*/

	rect.x = 4;
	rect.y = 360;
	rect.w = 72;
	rect.h = 72;

	CG_DrawPMItems(rect);
	CG_DrawPMItemsBig();

	if (cg_drawCompass.integer)
	{
		if (cg_altHud.integer)
		{
			//rect = { 44, 480 - 92, 62, 80 };
			rect.x = 44;
			rect.y = 480 - 75;
			rect.w = 72;
			rect.h = 72;
		}
		else
		{
			rect.x = (Ccg_WideX(640) - 100 - 20 - 16);
			rect.y = 20 - 16;
			rect.w = 100 + 32;
			rect.h = 100 + 32;
		}
		CG_DrawNewCompass(rect);
	}
}