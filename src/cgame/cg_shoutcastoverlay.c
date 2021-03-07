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
 * @file cg_shoutcastoverlay.c
 */

#include "cg_local.h"

#define MAX_AXIS     6
#define MAX_ALLIES   6

#define FONT_HEADER         &cgs.media.limboFont1
#define FONT_TEXT           &cgs.media.limboFont2

#define PLAYER_LIST_OVERLAY_BOX_WIDTH 170
#define PLAYER_LIST_OVERLAY_BOX_HEIGHT 28
#define PLAYER_LIST_OVERLAY_BORDER_DISTANCE_X 15
#define PLAYER_LIST_OVERLAY_BORDER_DISTANCE_Y (SCREEN_HEIGHT - 180)

#define PLAYER_STATUS_OVERLAY_NAMEBOX_WIDTH 150
#define PLAYER_STATUS_OVERLAY_NAMEBOX_HEIGHT 16
#define PLAYER_STATUS_OVERLAY_NAMEBOX_X (Ccg_WideX(SCREEN_WIDTH) - (Ccg_WideX(SCREEN_WIDTH) / 2) - (PLAYER_STATUS_OVERLAY_NAMEBOX_WIDTH / 2))
#define PLAYER_STATUS_OVERLAY_NAMEBOX_Y (SCREEN_HEIGHT - 75)

#define PLAYER_STATUS_OVERLAY_STATSBOX_WIDTH 224
#define PLAYER_STATUS_OVERLAY_STATSBOX_HEIGHT 20
#define PLAYER_STATUS_OVERLAY_STATSBOX_X (Ccg_WideX(SCREEN_WIDTH) - (Ccg_WideX(SCREEN_WIDTH) / 2) - (PLAYER_STATUS_OVERLAY_STATSBOX_WIDTH / 2))
#define PLAYER_STATUS_OVERLAY_STATSBOX_Y (PLAYER_STATUS_OVERLAY_NAMEBOX_Y + PLAYER_STATUS_OVERLAY_NAMEBOX_HEIGHT)

#define MINIMAP_WIDTH 150
#define MINIMAP_HEIGHT 150
#define MINIMAP_X (Ccg_WideX(SCREEN_WIDTH) - MINIMAP_WIDTH - PLAYER_LIST_OVERLAY_BORDER_DISTANCE_X)
#define MINIMAP_Y 15

#define GAMETIME_WIDTH 60
#define GAMETIME_HEIGHT 30
#define GAMETIME_X (Ccg_WideX(SCREEN_WIDTH) / 2) - (GAMETIME_WIDTH / 2)
#define GAMETIME_Y 12

#define TEAMNAMES_WIDTH 190
#define TEAMNAMES_HEIGHT (GAMETIME_HEIGHT)

#define POWERUPS_WIDTH 36
#define POWERUPS_HEIGHT 36
#define POWERUPS_X (Ccg_WideX(SCREEN_WIDTH) - POWERUPS_WIDTH - PLAYER_LIST_OVERLAY_BORDER_DISTANCE_X)
#define POWERUPS_Y (MINIMAP_Y + MINIMAP_HEIGHT + 5)


static vec4_t bg = { 0.0f, 0.0f, 0.0f, 0.7f };

static vec4_t colorAllies = { 0.121f, 0.447f, 0.811f, 0.45f };
static vec4_t colorAxis   = { 0.749f, 0.129f, 0.129f, 0.45f };

int players[12];

/**
* @brief CG_DrawMinimap
*/
void CG_DrawMinimap(void)
{
	CG_DrawAutoMapNew(MINIMAP_X, MINIMAP_Y, MINIMAP_WIDTH, MINIMAP_HEIGHT);
}

/**
* @brief CG_GetPlayerCurrentWeapon
* @param[in] player
*/
static int CG_GetPlayerCurrentWeapon(clientInfo_t *player)
{
	int curWeap;

	if (cg.predictedPlayerEntity.currentState.eFlags & EF_MOUNTEDTANK)
	{
		if (IS_MOUNTED_TANK_BROWNING(cg.snap->ps.clientNum))
		{
			curWeap = WP_MOBILE_BROWNING;
		}
		else
		{
			curWeap = WP_MOBILE_MG42;
		}
	}
	else if ((cg.predictedPlayerEntity.currentState.eFlags & EF_MG42_ACTIVE) || (cg.predictedPlayerEntity.currentState.eFlags & EF_AAGUN_ACTIVE))
	{
		curWeap = WP_MOBILE_MG42;
	}
	else
	{
		curWeap = cg_entities[player->clientNum].currentState.weapon;
	}
	return curWeap;
}

/**
* @brief CG_DrawShoutcastPlayerOverlayAxis
* @param[in] player
* @param[in] x
* @param[in] y
* @param[in] index
*/
static void CG_DrawShoutcastPlayerOverlayAxis(clientInfo_t *player, float x, float y, int index)
{
	int   curWeap, weapScale, textWidth, textHeight;
	float fraction;
	float topRowX    = x;
	float bottomRowX = x;
	char  *text;

	//Draw box
	CG_FillRect(x, y, PLAYER_LIST_OVERLAY_BOX_WIDTH, PLAYER_LIST_OVERLAY_BOX_HEIGHT, bg);
	CG_DrawRect_FixedBorder(x, y, PLAYER_LIST_OVERLAY_BOX_WIDTH, PLAYER_LIST_OVERLAY_BOX_HEIGHT / 2, 1, colorWhite);
	CG_DrawRect_FixedBorder(x, y, PLAYER_LIST_OVERLAY_BOX_WIDTH, PLAYER_LIST_OVERLAY_BOX_HEIGHT, 1, cg.snap->ps.clientNum == player->clientNum ? colorYellow : colorWhite);

	//Draw HP bar
	fraction = (float)player->health / (float)CG_GetPlayerMaxHealth(player->clientNum, player->cls, player->team);
	CG_FilledBar(topRowX + 0.5f, y + 0.5f, PLAYER_LIST_OVERLAY_BOX_WIDTH - 0.5f, PLAYER_LIST_OVERLAY_BOX_HEIGHT / 2 - 1, colorAxis, colorAxis, bg, fraction, BAR_BGSPACING_X0Y0);

	//Draw health
	if (player->health > 0)
	{
		text      = va("%i", player->health);
		textWidth = CG_Text_Width_Ext(text, 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(topRowX + 11 - (textWidth / 2), y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) + 3, 0.2f, 0.2f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
	}
	else if (player->health == 0)
	{
		CG_DrawPic(topRowX + 5, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) - 6, 12, 12, cgs.media.medicIcon);
	}
	else if (player->health < 0)
	{
		CG_DrawPic(topRowX + 5, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) - 6, 12, 12, cgs.media.scoreEliminatedShader);
	}

	//Draw name limit 20 chars, width 116
	textWidth  = CG_Text_Width_Ext(player->cleanname, 0.2f, 0, FONT_TEXT);
	textHeight = CG_Text_Height_Ext(player->cleanname, 0.2f, 0, FONT_TEXT);
	if (textWidth > 116)
	{
		textWidth = 116;
	}
	CG_Text_Paint_Ext(x + 26, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) + (textHeight / 2), 0.2f, 0.2f, colorWhite, player->cleanname, 0, 20, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//Draw follow bind
	text      = va("(F%i)", index + 1);
	textWidth = CG_Text_Width_Ext(text, 0.15f, 0, FONT_TEXT);
	CG_Text_Paint_Ext(x + PLAYER_LIST_OVERLAY_BOX_WIDTH - textWidth - 3, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) + 2.0f, 0.15f, 0.15f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//Draw class
	CG_DrawPic(bottomRowX + 4, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->cls)]);
	bottomRowX += 16;

	if (player->cls != player->latchedcls)
	{
		//Arrow latched class
		textWidth  = CG_Text_Width_Ext("->", 0.2f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("->", 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(bottomRowX, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) + (textHeight / 2) + 0.5f, 0.2f, 0.2f, colorYellow, "->", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		bottomRowX += textWidth;
		//Latched class
		CG_DrawPic(bottomRowX + 1, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->latchedcls)]);
	}

	//Draw powerups
	bottomRowX = x + PLAYER_LIST_OVERLAY_BOX_WIDTH;
	if (player->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
	{
		CG_DrawPic(bottomRowX - 14, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, cgs.media.objectiveShader);
		bottomRowX -= 14;
	}
	if (player->powerups & (1 << PW_OPS_DISGUISED))
	{
		CG_DrawPic(bottomRowX - 14, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, player->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		bottomRowX -= 14;
	}
	if (player->powerups & (1 << PW_INVULNERABLE))
	{
		CG_DrawPic(bottomRowX - 14, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, cgs.media.spawnInvincibleShader);
	}

	//Draw weapon icon
	curWeap    = CG_GetPlayerCurrentWeapon(player);
	weapScale  = cg_weapons[curWeap].weaponIconScale * 10;
	bottomRowX = x + PLAYER_LIST_OVERLAY_BOX_WIDTH - 63;

	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIconScale == 1)
	{
		bottomRowX += weapScale;
	}

	// note: WP_NONE is excluded
	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[0])     // do not try to draw nothing
	{
		CG_DrawPic(bottomRowX, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[0]);
	}
	else if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[1])
	{
		CG_DrawPic(bottomRowX, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[1]);
	}
}

/**
* @brief CG_DrawShoutcastPlayerOverlayAllies
* @param[in] player
* @param[in] x
* @param[in] y
* @param[in] index
*/
static void CG_DrawShoutcastPlayerOverlayAllies(clientInfo_t *player, float x, float y, int index)
{
	int   curWeap, weapScale, textWidth, textHeight;
	float fraction;
	float topRowX    = x;
	float bottomRowX = x + PLAYER_LIST_OVERLAY_BOX_WIDTH;
	char  *text;

	//Draw box
	CG_FillRect(x, y, PLAYER_LIST_OVERLAY_BOX_WIDTH, PLAYER_LIST_OVERLAY_BOX_HEIGHT, bg);
	CG_DrawRect_FixedBorder(x, y, PLAYER_LIST_OVERLAY_BOX_WIDTH, PLAYER_LIST_OVERLAY_BOX_HEIGHT / 2, 1, colorWhite);
	CG_DrawRect_FixedBorder(x, y, PLAYER_LIST_OVERLAY_BOX_WIDTH, PLAYER_LIST_OVERLAY_BOX_HEIGHT, 1, cg.snap->ps.clientNum == player->clientNum ? colorYellow : colorWhite);

	//Draw HP bar
	fraction = (float)player->health / (float)CG_GetPlayerMaxHealth(player->clientNum, player->cls, player->team);
	CG_FilledBar(topRowX + 0.5f, y + 0.5f, PLAYER_LIST_OVERLAY_BOX_WIDTH - 0.5f, PLAYER_LIST_OVERLAY_BOX_HEIGHT / 2 - 1, colorAllies, colorAllies, bg, fraction, BAR_BGSPACING_X0Y0 | BAR_LEFT);

	topRowX += PLAYER_LIST_OVERLAY_BOX_WIDTH;

	//Draw health
	if (player->health > 0)
	{
		text      = va("%i", player->health);
		textWidth = CG_Text_Width_Ext(text, 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(topRowX - textWidth + (textWidth / 2) - 11, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) + 3, 0.2f, 0.2f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
	}
	else if (player->health == 0)
	{
		CG_DrawPic(topRowX - 17, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) - 6, 12, 12, cgs.media.medicIcon);
	}
	else if (player->health < 0)
	{
		CG_DrawPic(topRowX - 17, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) - 6, 12, 12, cgs.media.scoreEliminatedShader);
	}

	//Draw name limit 20 chars, width 116
	textWidth  = CG_Text_Width_Ext(player->cleanname, 0.2f, 0, FONT_TEXT);
	textHeight = CG_Text_Height_Ext(player->cleanname, 0.2f, 0, FONT_TEXT);
	if (textWidth > 116)
	{
		textWidth = 116;
	}
	CG_Text_Paint_Ext(x + PLAYER_LIST_OVERLAY_BOX_WIDTH - textWidth - 26, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) + (textHeight / 2), 0.2f, 0.2f, colorWhite, player->cleanname, 0, 20, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//Draw follow bind
	text = va("(F%i)", index + 7);
	CG_Text_Paint_Ext(x + 2, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT / 4) + 2.0f, 0.15f, 0.15f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//Draw class
	CG_DrawPic(bottomRowX - 16, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->cls)]);
	bottomRowX -= 16;

	if (player->cls != player->latchedcls)
	{
		//Arrow latched class
		textWidth  = CG_Text_Width_Ext("<-", 0.2f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("<-", 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(bottomRowX - textWidth - 1, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) + (textHeight / 2) + 0.5f, 0.2f, 0.2f, colorYellow, "<-", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		bottomRowX = bottomRowX - textWidth - 1;
		//Latched class
		CG_DrawPic(bottomRowX - 12, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->latchedcls)]);
	}

	//Draw powerups
	bottomRowX = x;
	if (player->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
	{
		CG_DrawPic(bottomRowX + 2, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, cgs.media.objectiveShader);
		bottomRowX += 14;
	}
	if (player->powerups & (1 << PW_OPS_DISGUISED))
	{
		CG_DrawPic(bottomRowX + 2, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, player->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		bottomRowX += 14;
	}
	if (player->powerups & (1 << PW_INVULNERABLE))
	{
		CG_DrawPic(bottomRowX + 2, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, cgs.media.spawnInvincibleShader);
	}

	//Draw weapon icon
	curWeap   = CG_GetPlayerCurrentWeapon(player);
	weapScale = cg_weapons[curWeap].weaponIconScale * 10;

	// note: WP_NONE is excluded
	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[0])     // do not try to draw nothing
	{
		CG_DrawPic(x + 43, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 5, weapScale, 10, cg_weapons[curWeap].weaponIcon[0]);
	}
	else if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[1])
	{
		CG_DrawPic(x + 43, y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * 0.75f) - 5, weapScale, 10, cg_weapons[curWeap].weaponIcon[1]);
	}
}

/**
* @brief CG_DrawShoutcastOverlay
*/
void CG_DrawShoutcastPlayerList(void)
{
	clientInfo_t *ci;
	int          axis   = 0;
	int          allies = 0;
	int          i;

	if (cgs.topshots.show == SHOW_ON)
	{
		return;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ci = &cgs.clientinfo[i];

		if (!ci->infoValid)
		{
			continue;
		}

		if (ci->team == TEAM_SPECTATOR)
		{
			continue;
		}

		if (ci->team == TEAM_AXIS && axis < MAX_AXIS)
		{
			CG_DrawShoutcastPlayerOverlayAxis(ci, PLAYER_LIST_OVERLAY_BORDER_DISTANCE_X, PLAYER_LIST_OVERLAY_BORDER_DISTANCE_Y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * axis) + (1 * axis), axis);
			players[axis] = ci->clientNum;
			axis++;
		}

		if (ci->team == TEAM_ALLIES && allies < MAX_ALLIES)
		{
			CG_DrawShoutcastPlayerOverlayAllies(ci, Ccg_WideX(SCREEN_WIDTH) - PLAYER_LIST_OVERLAY_BOX_WIDTH - PLAYER_LIST_OVERLAY_BORDER_DISTANCE_X, PLAYER_LIST_OVERLAY_BORDER_DISTANCE_Y + (PLAYER_LIST_OVERLAY_BOX_HEIGHT * allies) + (1 * allies), allies);
			players[allies + 6] = ci->clientNum;
			allies++;
		}

		if (axis > MAX_AXIS && allies > TEAM_ALLIES)
		{
			break;
		}
	}
}

/**
* @brief CG_DrawShoutcastPlayerChargebar
* @param[in] x
* @param[in] y
* @param[in] width
* @param[in] height
* @param[in] flags
*/
static void CG_DrawShoutcastPlayerChargebar(float x, float y, int width, int height, int flags)
{
	float    barFrac, chargeTime;
	qboolean charge = qtrue;
	vec4_t   color;

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

	// display colored charge bar if charge bar isn't full enough
	if (GetWeaponTableData(cg.predictedPlayerState.weapon)->attributes & WEAPON_ATTRIBUT_CHARGE_TIME)
	{
		skillType_t skill = GetWeaponTableData(cg.predictedPlayerState.weapon)->skillBased;
		float       coeff = GetWeaponTableData(cg.predictedPlayerState.weapon)->chargeTimeCoeff[cgs.clientinfo[cg.clientNum].skill[skill]];

		if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * coeff)
		{
			charge = qfalse;
		}
	}
	else if ((cg.predictedPlayerState.eFlags & EF_ZOOMING || cg.predictedPlayerState.weapon == WP_BINOCULARS)
	         && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS)
	{
		skillType_t skill = GetWeaponTableData(WP_ARTY)->skillBased;
		float       coeff = GetWeaponTableData(WP_ARTY)->chargeTimeCoeff[cgs.clientinfo[cg.clientNum].skill[skill]];

		if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * coeff)
		{
			charge = qfalse;
		}
	}

	if (chargeTime < 0)
	{
		return;
	}

	barFrac = (cg.time - cg.snap->ps.classWeaponTime) / chargeTime;

	if (barFrac > 1.0f)
	{
		barFrac = 1.0f;
	}

	if (!charge)
	{
		color[0] = 1.0f;
		color[1] = color[2] = 0.1f;
		color[3] = 0.5f;
	}
	else
	{
		color[0] = color[1] = 1.0f;
		color[2] = barFrac;
		color[3] = 0.25f + barFrac * 0.5f;
	}

	CG_FilledBar(x, y, width, height, color, NULL, bg, barFrac, flags);
}

/**
* @brief CG_DrawShoutcasterStaminaBar
* @param[in] x
* @param[in] y
* @param[in] width
* @param[in] height
* @param[in] flags
*/
static void CG_DrawShoutcastPlayerStaminaBar(float x, float y, int width, int height, int flags)
{
	vec4_t colour = { 0.1f, 1.0f, 0.1f, 0.5f };
	vec_t  *color = colour;
	float  frac   = cg.snap->ps.stats[STAT_SPRINTTIME] / (float)SPRINTTIME;

	if (cg.snap->ps.powerups[PW_ADRENALINE])
	{
		if (cg.snap->ps.pm_flags & PMF_FOLLOW)
		{
			Vector4Average(colour, colorWhite, (float)sin(cg.time * .005), colour);
		}
		else
		{
			float msec = cg.snap->ps.powerups[PW_ADRENALINE] - cg.time;

			if (msec >= 0)
			{
				Vector4Average(colour, colorMdRed, (float)(.5 + sin(.2 * sqrt((double)msec) * M_TAU_F) * .5), colour);
			}
		}
	}
	else
	{
		color[0] = 1.0f - frac;
		color[1] = frac;
	}

	CG_FilledBar(x, y, width, height, color, NULL, bg, frac, flags);
}

/**
* @brief CG_RequestPlayerStats (CG_StatsDown_f)
* @param[in] clientNum
*/
static void CG_RequestPlayerStats(int clientNum)
{
	if (cgs.gamestats.requestTime < cg.time)
	{
		cgs.gamestats.requestTime = cg.time + 2000;
		trap_SendClientCommand(va("sgstats %d", clientNum));
	}
}

/**
* @brief CG_ParseStats
* @param[in] data
* @param[in] i
*/
static char *CG_ParseStats(char *data, int i)
{
	int  c;
	int  stop = 0;
	char *in = data, *out = "";

	while ((c = *in) != 0)
	{
		if (c == ':')
		{
			stop++;
		}
		if (c >= '0' && c <= '9' && stop == i)
		{
			do
			{
				out = va("%s%c", out, c);
				in++;
				c = *in;
			}
			while (c >= '0' && c <= '9');
			return out;
		}
		in++;
	}
	return out;
}

/**
* @brief CG_DrawShoutcastPlayerStatus
*/
void CG_DrawShoutcastPlayerStatus(void)
{
	gameStats_t   *gs     = &cgs.gamestats;
	clientInfo_t  *player = &cgs.clientinfo[cg.snap->ps.clientNum];
	playerState_t *ps     = &cg.snap->ps;
	rectDef_t     rect;
	float         nameBoxWidth = PLAYER_STATUS_OVERLAY_NAMEBOX_WIDTH;
	float         nameBoxHeight = PLAYER_STATUS_OVERLAY_NAMEBOX_HEIGHT;
	float         nameBoxX = PLAYER_STATUS_OVERLAY_NAMEBOX_X;
	float         nameBoxY = PLAYER_STATUS_OVERLAY_NAMEBOX_Y;
	float         statsBoxWidth = PLAYER_STATUS_OVERLAY_STATSBOX_WIDTH;
	float         statsBoxHeight = PLAYER_STATUS_OVERLAY_STATSBOX_HEIGHT;
	float         statsBoxX = PLAYER_STATUS_OVERLAY_STATSBOX_X;
	float         statsBoxY = PLAYER_STATUS_OVERLAY_STATSBOX_Y;
	float         textWidth, textWidth2, textHeight;
	char          *kills, *deaths, *selfkills, *dmgGiven, *dmgRcvd, *text;
	int           ammo, clip, akimbo, curWeap, weapScale, tmpX;

	if (cgs.topshots.show == SHOW_ON)
	{
		return;
	}

	//Draw name box
	CG_FillRect(nameBoxX, nameBoxY, nameBoxWidth, nameBoxHeight, bg);
	CG_DrawRect_FixedBorder(nameBoxX, nameBoxY, nameBoxWidth, nameBoxHeight, 1, colorWhite);

	//Draw team flag
	if (player->team == TEAM_ALLIES)
	{
		CG_DrawPic(nameBoxX + 4, nameBoxY + (nameBoxHeight / 2) - 4.5f, 14, 9, cgs.media.alliedFlag);
	}
	else
	{
		CG_DrawPic(nameBoxX + 4, nameBoxY + (nameBoxHeight / 2) - 4.5f, 14, 9, cgs.media.axisFlag);
	}

	//Draw name limit 20 chars, width 110
	textWidth  = CG_Text_Width_Ext(player->cleanname, 0.19f, 0, FONT_TEXT);
	textHeight = CG_Text_Height_Ext(player->cleanname, 0.19f, 0, FONT_TEXT);
	if (textWidth > 110)
	{
		textWidth = 110;
	}
	CG_Text_Paint_Ext(nameBoxX + (nameBoxWidth / 2) - (textWidth / 2), nameBoxY + (nameBoxHeight / 2) + (textHeight / 2), 0.19f, 0.19f, colorWhite, player->cleanname, 0, 20, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//Draw country flag
	CG_DrawFlag(nameBoxX + nameBoxWidth - 17, nameBoxY + (nameBoxHeight / 2) - 7, 1, player->clientNum);

	//Draw stats box
	CG_FillRect(statsBoxX, statsBoxY, statsBoxWidth, statsBoxHeight, bg);
	CG_DrawRect_FixedBorder(statsBoxX, statsBoxY, statsBoxWidth, statsBoxHeight, 1, colorWhite);

	//Draw powerups
	tmpX = statsBoxX + statsBoxWidth;
	if (ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG])
	{
		CG_DrawPic(tmpX + 3, statsBoxY, 20, 20, cgs.media.objectiveShader);
		tmpX += 23;
	}
	if (ps->powerups[PW_OPS_DISGUISED])
	{
		CG_DrawPic(tmpX + 3, statsBoxY, 20, 20, player->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		tmpX += 23;
	}
	if (ps->powerups[PW_INVULNERABLE])
	{
		CG_DrawPic(tmpX + 3, statsBoxY, 20, 20, cgs.media.spawnInvincibleShader);
	}

	CG_DrawShoutcastPlayerChargebar(statsBoxX, statsBoxY + statsBoxHeight, statsBoxWidth / 2, 2, BAR_BG | BAR_BGSPACING_X0Y0 | BAR_LEFT);
	CG_DrawShoutcastPlayerStaminaBar(statsBoxX + (statsBoxWidth / 2), statsBoxY + statsBoxHeight, statsBoxWidth / 2, 2, BAR_BG | BAR_BGSPACING_X0Y0);

	//Draw ammo count
	CG_PlayerAmmoValue(&ammo, &clip, &akimbo);

	if (ammo > 0 || clip > 0)
	{
		if (clip == -1)
		{
			text = va("%i", ammo);
		}
		else
		{
			text = va("%i/%i", ammo, clip);
		}

		textWidth = CG_Text_Width_Ext(text, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + statsBoxWidth - (textWidth / 2) - 16, statsBoxY + (statsBoxHeight / 2) + 2, 0.19f, 0.19f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
	}

	//Draw weapon icon
	curWeap   = CG_GetPlayerCurrentWeapon(player);
	weapScale = cg_weapons[curWeap].weaponIconScale * 10;
	tmpX      = statsBoxX + statsBoxWidth - 50;

	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIconScale == 1)
	{
		tmpX += weapScale;
	}

	// note: WP_NONE is excluded
	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[0])     // do not try to draw nothing
	{
		CG_DrawPic(tmpX, statsBoxY + (statsBoxHeight / 2) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[0]);
	}
	else if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[1])
	{
		CG_DrawPic(tmpX, statsBoxY + (statsBoxHeight / 2) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[1]);
	}

	//Draw hp
	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
		text       = va("%i", cg.snap->ps.stats[STAT_HEALTH]);
		textWidth  = CG_Text_Width_Ext(text, 0.19f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext(text, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX - (textWidth / 2) + 10, statsBoxY + (statsBoxHeight / 2) + (textHeight / 2), 0.19f, 0.19f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] <= 0 && (cg.snap->ps.pm_flags & PMF_LIMBO))
	{
		CG_DrawPic(statsBoxX + 5, statsBoxY + (statsBoxHeight / 2) - 6, 12, 12, cgs.media.scoreEliminatedShader);
	}
	else
	{
		CG_DrawPic(statsBoxX + 5, statsBoxY + (statsBoxHeight / 2) - 6, 12, 12, cgs.media.medicIcon);
	}

	statsBoxX += 18;

	//Draw class
	CG_DrawPic(statsBoxX + 1, statsBoxY + (statsBoxHeight / 2) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->cls)]);
	statsBoxX += 13;

	if (player->cls != player->latchedcls)
	{
		//Arrow latched class
		textWidth  = CG_Text_Width_Ext("->", 0.19f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("->", 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 1, statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 1, 0.19f, 0.19f, colorYellow, "->", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 1 + textWidth;
		//Latched class
		CG_DrawPic(statsBoxX + 1, statsBoxY + (statsBoxHeight / 2) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->latchedcls)]);
	}

	CG_RequestPlayerStats(ps->clientNum);

	if (gs->nClientID == player->clientNum && gs->fHasStats)
	{
		statsBoxX = PLAYER_STATUS_OVERLAY_STATSBOX_X + 55;

		dmgGiven  = va("%s", CG_ParseStats(gs->strExtra[0], 1));
		dmgRcvd   = va("%s", CG_ParseStats(gs->strExtra[1], 1));
		kills     = va("%s", CG_ParseStats(gs->strExtra[3], 1));
		deaths    = va("%s", CG_ParseStats(gs->strExtra[4], 1));
		selfkills = va("%s", CG_ParseStats(gs->strExtra[4], 2));

		//kills
		textWidth  = CG_Text_Width_Ext("K", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("K", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 6, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), 0.16f, 0.16f, colorMdGrey, "K", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(kills, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(kills, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 6 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, 0.19f, 0.19f, colorWhite, kills, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 6 + textWidth2;

		//Deaths
		textWidth  = CG_Text_Width_Ext("D", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("D", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 6, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), 0.16f, 0.16f, colorMdGrey, "D", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(deaths, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(deaths, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 6 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, 0.19f, 0.19f, colorWhite, deaths, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 6 + textWidth2;

		//Selfkills
		textWidth  = CG_Text_Width_Ext("SK", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("SK", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 4, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), 0.16f, 0.16f, colorMdGrey, "SK", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(selfkills, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(selfkills, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 4 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, 0.19f, 0.19f, colorWhite, selfkills, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 4 + textWidth2;

		//dmgGiven
		textWidth  = CG_Text_Width_Ext("DG", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("DG", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 15, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), 0.16f, 0.16f, colorMdGrey, "DG", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(dmgGiven, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(dmgGiven, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 15 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, 0.19f, 0.19f, colorMdGreen, dmgGiven, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 15 + textWidth2;

		//dmgRcvd
		textWidth  = CG_Text_Width_Ext("DR", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("DR", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 7, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), 0.16f, 0.16f, colorMdGrey, "DR", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(dmgRcvd, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(dmgRcvd, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 7 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, 0.19f, 0.19f, colorRed, dmgRcvd, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 5 + textWidth2;
	}

	//Draw cursor hint
	rect.x = (SCREEN_WIDTH / 2) - 24;
	rect.y = 260;
	rect.w = 48;
	rect.h = 48;

	CG_DrawCursorhint(&rect);

	//Draw stability bar
	rect.x = 50;
	rect.y = 208;
	rect.w = 10;
	rect.h = 64;

	CG_DrawWeapStability(&rect);
}

/**
* @brief CG_DrawShoutcastTeamNames
*/
static void CG_DrawShoutcastTeamNames()
{
	rectDef_t rect;
	int       textWidth;
	int       textHeight;
	char      *text;

	if (cg_shoutcastDrawTeamNames.integer)
	{
		//Draw axis label
		if (Q_PrintStrlen(cg_shoutcastTeamName1.string) > 0)
		{
			text = va("%s", cg_shoutcastTeamName1.string);
		}
		else
		{
			text = va("%s", "Axis");
		}

		rect.x = GAMETIME_X - TEAMNAMES_WIDTH;
		rect.y = GAMETIME_Y;
		rect.w = TEAMNAMES_WIDTH;
		rect.h = TEAMNAMES_HEIGHT;
		GradientBar_Paint(&rect, colorAxis);

		//Max width 174, limit 20 chars
		textWidth  = CG_Text_Width_Ext(text, 0.3f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext(text, 0.3f, 0, FONT_TEXT);

		if (textWidth > 174)
		{
			textWidth = 174;
		}

		CG_Text_Paint_Ext(rect.x + (rect.w / 2) - (textWidth / 2) + 1.35f, rect.y + (rect.h / 2) + (textHeight / 2) + 1.35f, 0.3f, 0.3f, colorBlack, text, 0, 20, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		CG_Text_Paint_Ext(rect.x + (rect.w / 2) - (textWidth / 2), rect.y + (rect.h / 2) + (textHeight / 2), 0.3f, 0.3f, colorWhite, text, 0, 20, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		//Draw allies label
		if (Q_PrintStrlen(cg_shoutcastTeamName2.string) > 0)
		{
			text = va("%s", cg_shoutcastTeamName2.string);
		}
		else
		{
			text = va("%s", "Allies");
		}

		rect.x = GAMETIME_X + GAMETIME_WIDTH;
		rect.y = GAMETIME_Y;
		rect.w = TEAMNAMES_WIDTH;
		rect.h = TEAMNAMES_HEIGHT;
		GradientBar_Paint(&rect, colorAllies);

		//Max width 174, limit 20 chars
		textWidth  = CG_Text_Width_Ext(text, 0.3f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext(text, 0.3f, 0, FONT_TEXT);

		if (textWidth > 174)
		{
			textWidth = 174;
		}

		CG_Text_Paint_Ext(rect.x + (rect.w / 2) - (textWidth / 2) + 1.35f, rect.y + (rect.h / 2) + (textHeight / 2) + 1.35f, 0.3f, 0.3f, colorBlack, text, 0, 20, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		CG_Text_Paint_Ext(rect.x + (rect.w / 2) - (textWidth / 2), rect.y + (rect.h / 2) + (textHeight / 2), 0.3f, 0.3f, colorWhite, text, 0, 20, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
	}
}

/**
* @brief CG_DrawTimerShoutcast
*/
void CG_DrawShoutcastTimer(void)
{
	if (cgs.gamestats.show == SHOW_ON)
	{
		return;
	}

	vec4_t color = { .6f, .6f, .6f, 1.f };
	char   *text, *rt, *rtAllies = "", *rtAxis = "", *round;
	int    tens;
	int    msec    = (cgs.timelimit * 60000.f) - (cg.time - cgs.levelStartTime); // 60.f * 1000.f
	int    seconds = msec / 1000;
	int    mins    = seconds / 60;
	int    w       = GAMETIME_WIDTH;
	int    h       = GAMETIME_HEIGHT;
	int    x       = GAMETIME_X;
	int    y       = GAMETIME_Y;
	int    textWidth;

	seconds -= mins * 60;
	tens     = seconds / 10;
	seconds -= tens * 10;

	if (cgs.gamestate != GS_PLAYING)
	{
		text     = va("^7%s", CG_TranslateString("WARMUP")); // don't draw reinforcement time in warmup mode // ^*
		color[3] = fabs(sin(cg.time * 0.002));
	}
	else if (msec < 0 && cgs.timelimit > 0.0f)
	{
		text     = "^70:00";
		color[3] = fabs(sin(cg.time * 0.002));
	}
	else
	{
		if (cgs.gametype != GT_WOLF_LMS && cgs.clientinfo[cg.clientNum].shoutcaster && cg_drawReinforcementTime.integer > 0)
		{
			int reinfTimeAx = CG_CalculateShoutcasterReinfTime(TEAM_AXIS);
			int reinfTimeAl = CG_CalculateShoutcasterReinfTime(TEAM_ALLIES);

			rtAllies = va("^$%i", reinfTimeAl);
			rtAxis   = va("^1%i", reinfTimeAx);
		}
		else if (cgs.gametype != GT_WOLF_LMS && (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW)) && cg_drawReinforcementTime.integer > 0)
		{
			int  reinfTime = CG_CalculateReinfTime(qfalse);
			char *c        = (cgs.clientinfo[cg.clientNum].shoutcaster ? (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS ? "^1" : "^$") : "^F");

			rt = va("%s%s%d", (reinfTime <= 2 && cgs.clientinfo[cg.clientNum].health == 0 &&
			                   !(cg.snap->ps.pm_flags & PMF_FOLLOW)) ? "^1" : c, ((cgs.timelimit <= 0.0f) ? "" : " "), reinfTime);
		}
		else
		{
			rt       = "";
			rtAllies = "";
			rtAxis   = "";
		}

		if (cgs.timelimit <= 0.0f)
		{
			text = "";
		}
		else
		{
			text = va("^7%2i:%i%i", mins, tens, seconds);
		}

		color[3] = 1.f;
	}

	textWidth = CG_Text_Width_Ext(text, 0.23f, 0, FONT_HEADER);

	//Draw box
	CG_FillRect(x, y, w, h, bg);
	CG_DrawRect_FixedBorder(x, y, w, h, 2, colorWhite);

	//Game time
	CG_Text_Paint_Ext(x + w / 2 - textWidth / 2, y + 13, 0.23f, 0.23f, color, text, 0, 0, 0, FONT_HEADER);

	//Axis reinf time
	CG_Text_Paint_Ext(x + 3, y + h - 5, 0.20f, 0.20f, color, rtAxis, 0, 0, 0, FONT_HEADER);

	//Allies reinf time
	textWidth = CG_Text_Width_Ext(rtAllies, 0.20f, 0, FONT_HEADER);
	CG_Text_Paint_Ext(x + w - textWidth - 3, y + h - 5, 0.20f, 0.20f, color, rtAllies, 0, 0, 0, FONT_HEADER);

	//Round number
	if (cgs.gametype == GT_WOLF_STOPWATCH)
	{
		round     = va("%i/2", cgs.currentRound + 1);
		textWidth = CG_Text_Width_Ext(round, 0.15f, 0, FONT_HEADER);
		CG_Text_Paint_Ext(x + w / 2 - textWidth / 2, y + h - 5.5f, 0.15f, 0.15f, colorWhite, round, 0, 0, 0, FONT_HEADER);
	}

	CG_DrawShoutcastTeamNames();
}

/**
* @brief CG_DrawShoutcastPowerups
*/
void CG_DrawShoutcastPowerups(void)
{
	if (cg.flagIndicator & (1 << PW_REDFLAG))
	{
		if (cg.redFlagCounter > 0)
		{
			CG_DrawPic(POWERUPS_X, POWERUPS_Y, POWERUPS_WIDTH, POWERUPS_HEIGHT, cgs.media.objectiveTeamShader);
		}
		else
		{
			CG_DrawPic(POWERUPS_X, POWERUPS_Y, POWERUPS_WIDTH, POWERUPS_HEIGHT, cgs.media.objectiveDroppedShader);
		}
	}
	else if (cg.flagIndicator & (1 << PW_BLUEFLAG))
	{
		if (cg.blueFlagCounter > 0)
		{
			CG_DrawPic(POWERUPS_X, POWERUPS_Y, POWERUPS_WIDTH, POWERUPS_HEIGHT, cgs.media.objectiveTeamShader);
		}
		else
		{
			CG_DrawPic(POWERUPS_X, POWERUPS_Y, POWERUPS_WIDTH, POWERUPS_HEIGHT, cgs.media.objectiveDroppedShader);
		}
	}
}

/**
* @brief CG_Shoutcast_KeyHandling
* @param[in] _key
* @param[in] down
*/
void CG_Shoutcast_KeyHandling(int key, qboolean down)
{
	if (down)
	{
		CG_ShoutcastCheckExecKey(key, qtrue);
	}
}

/**
* @brief CG_ShoutcastCheckExecKey
* @param[in] key
* @param[in] doaction
* @return
*/
qboolean CG_ShoutcastCheckExecKey(int key, qboolean doaction)
{
	if (key == K_ESCAPE)
	{
		return qtrue;
	}

	if ((key & K_CHAR_FLAG))
	{
		return qfalse;
	}

	key &= ~K_CHAR_FLAG;

	switch (key)
	{
	case K_F1:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[0]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F2:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[1]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F3:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[2]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F4:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[3]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F5:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[4]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F6:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[5]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F7:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[6]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F8:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[7]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F9:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[8]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F10:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[9]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F11:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[10]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	case K_F12:
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[11]));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return qtrue;
	}

	return qfalse;
}
