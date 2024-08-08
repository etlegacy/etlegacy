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
 * @file cg_shoutcastoverlay.c
 */

#include "cg_local.h"

#define MAX_PLAYERS     6

#define FONT_HEADER         &cgs.media.limboFont1
#define FONT_TEXT           &cgs.media.limboFont2

int players[12];

/**
* @brief CG_GetPlayerCurrentWeapon
* @param[in] player
*/
static int CG_GetPlayerCurrentWeapon(clientInfo_t *player)
{
	int curWeap;

	if (cg_entities[player->clientNum].currentState.eFlags & EF_MOUNTEDTANK)
	{
		if (IS_MOUNTED_TANK_BROWNING(player->clientNum))
		{
			curWeap = WP_MOBILE_BROWNING;
		}
		else
		{
			curWeap = WP_MOBILE_MG42;
		}
	}
	else if ((cg_entities[player->clientNum].currentState.eFlags & EF_MG42_ACTIVE) || (cg_entities[player->clientNum].currentState.eFlags & EF_AAGUN_ACTIVE))
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
static void CG_DrawShoutcastPlayerOverlayAxis(hudComponent_t *comp, clientInfo_t *player, float y, int index)
{
	int    curWeap, weapScale, textWidth, textHeight;
	float  fraction;
	float  statusWidth = comp->location.w / 5.f;
	float  topRowX     = comp->location.x;
	float  bottomRowX  = comp->location.x;
	float  height      = comp->location.h / MAX_PLAYERS;
	char   *text;
	char   name[MAX_NAME_LENGTH + 2] = { 0 };
	vec4_t hcolor, borderColor;

	if (player->health > 0)
	{
		Vector4Copy(colorLtGrey, borderColor);
	}
	else
	{
		Vector4Copy(comp->colorBorder, borderColor);
	}

	// draw box
	CG_FillRect(comp->location.x, y, comp->location.w, height, comp->colorBackground);
	CG_FillRect(comp->location.x, y, statusWidth, height, comp->colorSecondary);
	CG_DrawRect_FixedBorder(comp->location.x, y, statusWidth, height, 2, borderColor);
	CG_DrawRect_FixedBorder(comp->location.x + statusWidth - 0.75f, y, comp->location.w - statusWidth + 0.5f, height / 2, 2, borderColor);
	CG_DrawRect_FixedBorder(comp->location.x, y, comp->location.w, height, 2, cg.snap->ps.clientNum == player->clientNum ? colorYellow : borderColor);

	// draw HP bar
	fraction = (float)player->health / (float)CG_GetPlayerMaxHealth(player->clientNum, player->cls, player->team);
	CG_FilledBar(topRowX + statusWidth, y + 1, comp->location.w - statusWidth - 1, height / 2 - 1.75f, comp->colorSecondary, comp->colorSecondary,
	             comp->colorBackground, comp->colorBackground, fraction, 0.f, BAR_BGSPACING_X0Y0, -1);

	// draw health
	if (player->health > 0)
	{
		Vector4Copy(colorWhite, hcolor);
		CG_ColorForHealth(player->health, hcolor);

		text      = va("%i", player->health);
		textWidth = CG_Text_Width_Ext(text, 0.27f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(topRowX + (statusWidth / 2) - (textWidth / 2) - 0.5f, y + (height / 2) + 4, 0.27f, 0.27f, hcolor, text, 0, 0, comp->styleText, FONT_TEXT);
	}
	else if (player->health == 0)
	{
		CG_DrawPic(topRowX + (statusWidth / 2) - 10, y + (height / 2) - 10, 20, 20, cgs.media.medicIcon);
	}
	else if (player->health < 0)
	{
		CG_DrawPic(topRowX + (statusWidth / 2) - 10, y + (height / 2) - 10, 20, 20, cgs.media.scoreEliminatedShader);
	}

	// draw name limit 20 chars
	Q_ColorizeString(player->health < 0 ? '9' : '7', player->cleanname, name, MAX_NAME_LENGTH + 2);
	textHeight = CG_Text_Height_Ext(name, 0.16f, 0, FONT_TEXT);
	CG_Text_Paint_Ext(comp->location.x + statusWidth + 1, y + (height / 4) + (textHeight / 2), 0.16f, 0.16f, comp->colorMain, name, 0, 20, comp->styleText, FONT_TEXT);

	// draw follow bind
	if (player->health < 0)
	{
		Vector4Copy(colorMdGrey, hcolor);
	}
	else
	{
		Vector4Copy(colorWhite, hcolor);
	}

	text      = va("(F%i)", index + 1);
	textWidth = CG_Text_Width_Ext(text, 0.12f, 0, FONT_TEXT);
	CG_Text_Paint_Ext(comp->location.x + comp->location.w - textWidth - 2, y + (height / 4) + 2.0f, 0.12f, 0.12f, hcolor, text, 0, 0, comp->styleText, FONT_TEXT);

	// draw class
	CG_DrawPic(bottomRowX + statusWidth + 4, y + (height * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->cls)]);
	bottomRowX += statusWidth + 16;

	if (player->cls != player->latchedcls)
	{
		// arrow latched class
		textWidth  = CG_Text_Width_Ext("->", 0.2f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("->", 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(bottomRowX, y + (height * 0.75f) + (textHeight / 2) + 0.5f, 0.2f, 0.2f, colorYellow, "->", 0, 0, comp->styleText, FONT_TEXT);
		bottomRowX += textWidth;
		// latched class
		CG_DrawPic(bottomRowX + 1, y + (height * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->latchedcls)]);
	}

	// draw powerups
	bottomRowX = comp->location.x + comp->location.w;
	if (player->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
	{
		CG_DrawPic(bottomRowX - 14, y + (height * 0.75f) - 6.5f, 12, 12, cgs.media.objectiveShader);
		bottomRowX -= 14;
	}
	if (player->powerups & (1 << PW_OPS_DISGUISED))
	{
		CG_DrawPic(bottomRowX - 14, y + (height * 0.75f) - 6.5f, 12, 12, player->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		bottomRowX -= 14;
	}
	if (player->powerups & (1 << PW_INVULNERABLE))
	{
		CG_DrawPic(bottomRowX - 14, y + (height * 0.75f) - 6.5f, 12, 12, cgs.media.spawnInvincibleShader);
	}

	// draw weapon icon
	curWeap    = CG_GetPlayerCurrentWeapon(player);
	weapScale  = cg_weapons[curWeap].weaponIconScale * 10;
	bottomRowX = comp->location.x + comp->location.w - 63;

	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIconScale == 1)
	{
		bottomRowX += weapScale;
	}

	// note: WP_NONE is excluded
	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[0])     // do not try to draw nothing
	{
		CG_DrawPic(bottomRowX, y + (height * 0.75f) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[0]);
	}
	else if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[1])
	{
		CG_DrawPic(bottomRowX, y + (height * 0.75f) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[1]);
	}
}

/**
* @brief CG_DrawShoutcastPlayerOverlayAllies
* @param[in] player
* @param[in] x
* @param[in] y
* @param[in] index
*/
static void CG_DrawShoutcastPlayerOverlayAllies(hudComponent_t *comp, clientInfo_t *player, float y, int index)
{
	int    curWeap, weapScale, textWidth, textHeight;
	float  fraction;
	float  statusWidth = comp->location.w / 5.f;
	float  topRowX     = comp->location.x;
	float  bottomRowX  = comp->location.x + comp->location.w - statusWidth;
	float  height      = comp->location.h / MAX_PLAYERS;
	char   *text;
	char   name[MAX_NAME_LENGTH + 2] = { 0 };
	vec4_t hcolor, borderColor;

	if (player->health > 0)
	{
		Vector4Copy(colorLtGrey, borderColor);
	}
	else
	{
		Vector4Copy(comp->colorBorder, borderColor);
	}

	// draw box
	CG_FillRect(comp->location.x + 0.75f, y, comp->location.w - 1, height, comp->colorBackground);
	CG_FillRect(comp->location.x + comp->location.w - statusWidth, y, statusWidth, height, comp->colorSecondary);
	CG_DrawRect_FixedBorder(comp->location.x + comp->location.w - statusWidth, y, statusWidth, height, 2, borderColor);
	CG_DrawRect_FixedBorder(comp->location.x, y, comp->location.w - statusWidth + 0.75f, height / 2, 2, borderColor);
	CG_DrawRect_FixedBorder(comp->location.x, y, comp->location.w, height, 2, cg.snap->ps.clientNum == player->clientNum ? colorYellow : borderColor);

	// draw HP bar
	fraction = (float)player->health / (float)CG_GetPlayerMaxHealth(player->clientNum, player->cls, player->team);
	CG_FilledBar(topRowX + 1, y + 1, comp->location.w - statusWidth - 1, height / 2 - 1.5f, comp->colorSecondary, comp->colorSecondary,
	             comp->colorBackground, comp->colorBackground, fraction, 0.f, BAR_BGSPACING_X0Y0 | BAR_LEFT, -1);

	topRowX += comp->location.w;

	// draw health
	if (player->health > 0)
	{
		Vector4Copy(colorWhite, hcolor);
		CG_ColorForHealth(player->health, hcolor);

		text      = va("%i", player->health);
		textWidth = CG_Text_Width_Ext(text, 0.27f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(topRowX - (statusWidth / 2) - (textWidth / 2) - 0.5f, y + (height / 2) + 4, 0.27f, 0.27f, hcolor, text, 0, 0, comp->styleText, FONT_TEXT);
	}
	else if (player->health == 0)
	{
		CG_DrawPic(topRowX - (statusWidth / 2) - 10, y + (height / 2) - 10, 20, 20, cgs.media.medicIcon);
	}
	else if (player->health < 0)
	{
		CG_DrawPic(topRowX - (statusWidth / 2) - 10, y + (height / 2) - 10, 20, 20, cgs.media.scoreEliminatedShader);
	}

	// draw name limit 20 chars
	Q_ColorizeString(player->health < 0 ? '9' : '7', player->cleanname, name, MAX_NAME_LENGTH + 2);
	textWidth  = CG_Text_Width_Ext(name, 0.16f, 0, FONT_TEXT);
	textHeight = CG_Text_Height_Ext(name, 0.16f, 0, FONT_TEXT);
	if (textWidth > 116)
	{
		textWidth = 116;
	}
	CG_Text_Paint_Ext(comp->location.x + comp->location.w - textWidth - 30, y + (height / 4) + (textHeight / 2), 0.16f, 0.16f, comp->colorMain, name, 0, 20, comp->styleText, FONT_TEXT);

	// draw follow bind
	if (player->health < 0)
	{
		Vector4Copy(colorMdGrey, hcolor);
	}
	else
	{
		Vector4Copy(colorWhite, hcolor);
	}

	text = va("(F%i)", index + 7);
	CG_Text_Paint_Ext(comp->location.x + 1, y + (height / 4) + 2.0f, 0.12f, 0.12f, hcolor, text, 0, 0, comp->styleText, FONT_TEXT);

	// draw class
	CG_DrawPic(bottomRowX - 16, y + (height * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->cls)]);
	bottomRowX -= 16;

	if (player->cls != player->latchedcls)
	{
		// arrow latched class
		textWidth  = CG_Text_Width_Ext("<-", 0.2f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("<-", 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(bottomRowX - textWidth - 1, y + (height * 0.75f) + (textHeight / 2) + 0.5f, 0.2f, 0.2f, colorYellow, "<-", 0, 0, comp->styleText, FONT_TEXT);
		bottomRowX = bottomRowX - textWidth - 1;
		// latched class
		CG_DrawPic(bottomRowX - 12, y + (height * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->latchedcls)]);
	}

	// draw powerups
	bottomRowX = comp->location.x;
	if (player->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
	{
		CG_DrawPic(bottomRowX + 2, y + (height * 0.75f) - 6.5f, 12, 12, cgs.media.objectiveShader);
		bottomRowX += 14;
	}
	if (player->powerups & (1 << PW_OPS_DISGUISED))
	{
		CG_DrawPic(bottomRowX + 2, y + (height * 0.75f) - 6.5f, 12, 12, player->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		bottomRowX += 14;
	}
	if (player->powerups & (1 << PW_INVULNERABLE))
	{
		CG_DrawPic(bottomRowX + 2, y + (height * 0.75f) - 6.5f, 12, 12, cgs.media.spawnInvincibleShader);
	}

	// draw weapon icon
	curWeap   = CG_GetPlayerCurrentWeapon(player);
	weapScale = cg_weapons[curWeap].weaponIconScale * 10;

	// note: WP_NONE is excluded
	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[0])     // do not try to draw nothing
	{
		CG_DrawPic(comp->location.x + 43, y + (height * 0.75f) - 5, weapScale, 10, cg_weapons[curWeap].weaponIcon[0]);
	}
	else if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[1])
	{
		CG_DrawPic(comp->location.x + 43, y + (height * 0.75f) - 5, weapScale, 10, cg_weapons[curWeap].weaponIcon[1]);
	}
}


/**
 * @brief CG_DrawShoutcastPlayerList
 * @param[in] comp
 */
static void CG_DrawShoutcastPlayerList(hudComponent_t *comp, team_t team)
{
	int count;
	int i;

	for (i = 0, count = 0; i < MAX_CLIENTS && count < MAX_PLAYERS; ++i)
	{
		clientInfo_t *ci = &cgs.clientinfo[i];

		if (!ci->infoValid)
		{
			continue;
		}

		if (ci->team == team)
		{
			if (ci->team == TEAM_AXIS)
			{
				CG_DrawShoutcastPlayerOverlayAxis(comp, ci, comp->location.y + (comp->location.h / MAX_PLAYERS * count) + (1 * count), count);
				players[count] = ci->clientNum;
			}
			else
			{
				CG_DrawShoutcastPlayerOverlayAllies(comp, ci, comp->location.y + (comp->location.h / MAX_PLAYERS * count) + (1 * count), count);
				players[count + 6] = ci->clientNum;
			}

			++count;
		}
	}
}

/**
 * @brief CG_DrawShoutcastPlayerListAxis
 * @param[in] comp
 */
void CG_DrawShoutcastPlayerListAxis(hudComponent_t *comp)
{
	if (cgs.topshots.show == SHOW_ON)
	{
		return;
	}

	if (!cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	CG_DrawShoutcastPlayerList(comp, TEAM_AXIS);
}

/**
 * @brief CG_DrawShoutcastPlayerListAllies
 * @param[in] comp
 */
void CG_DrawShoutcastPlayerListAllies(hudComponent_t *comp)
{
	if (cgs.topshots.show == SHOW_ON)
	{
		return;
	}

	if (!cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	CG_DrawShoutcastPlayerList(comp, TEAM_ALLIES);
}

/**
 * @brief CG_DrawShoutcastPlayerChargebar
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] flags
 * @param[in] bgColor
 */
static void CG_DrawShoutcastPlayerChargebar(float x, float y, int width, int height, int flags, vec4_t bgColor)
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
		int index = BG_IsSkillAvailable(cgs.clientinfo[cg.clientNum].skill,
		                                GetWeaponTableData(cg.predictedPlayerState.weapon)->skillBased,
		                                GetWeaponTableData(cg.predictedPlayerState.weapon)->chargeTimeSkill);

		float coeff = GetWeaponTableData(cg.predictedPlayerState.weapon)->chargeTimeCoeff[index];

		if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * coeff)
		{
			charge = qfalse;
		}
	}
	else if ((cg.predictedPlayerState.eFlags & EF_ZOOMING || cg.predictedPlayerState.weapon == WP_BINOCULARS)
	         && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS)
	{
		int index = BG_IsSkillAvailable(cgs.clientinfo[cg.clientNum].skill,
		                                GetWeaponTableData(WP_ARTY)->skillBased,
		                                GetWeaponTableData(WP_ARTY)->chargeTimeSkill);

		float coeff = GetWeaponTableData(WP_ARTY)->chargeTimeCoeff[index];

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

	CG_FilledBar(x, y, width, height, color, NULL, bgColor, bgColor, barFrac, 0.f, flags, -1);
}

/**
 * @brief CG_DrawShoutcastPlayerStaminaBar
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] flags
 * @param[in] bgColor
 */
static void CG_DrawShoutcastPlayerStaminaBar(float x, float y, int width, int height, int flags, vec4_t bgColor)
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

	CG_FilledBar(x, y, width, height, color, NULL, bgColor, bgColor, frac, 0.f, flags, -1);
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
 * @param[in] comp
 */
void CG_DrawShoutcastPlayerStatus(hudComponent_t *comp)
{
	gameStats_t   *gs            = &cgs.gamestats;
	clientInfo_t  *player        = &cgs.clientinfo[cg.snap->ps.clientNum];
	playerState_t *ps            = &cg.snap->ps;
	float         nameBoxWidth   = comp->location.w / 1.5f;
	float         nameBoxHeight  = comp->location.h / 2.25f;
	float         nameBoxX       = comp->location.x + (comp->location.w - nameBoxWidth) * 0.5f;
	float         nameBoxY       = comp->location.y;
	float         statsBoxWidth  = comp->location.w;
	float         statsBoxHeight = comp->location.h - nameBoxHeight;
	float         statsBoxX      = comp->location.x;
	float         statsBoxY      = comp->location.y + nameBoxHeight;
	float         textWidth, textWidth2, textHeight;
	char          *kills, *deaths, *selfkills, *dmgGiven, *dmgRcvd, *text;
	int           ammo, clip, akimbo, curWeap, weapScale, tmpX;
	char          name[MAX_NAME_LENGTH + 2] = { 0 };
	float         scale;
	float         scale2;

	if (cgs.topshots.show == SHOW_ON)
	{
		return;
	}

	if (!cgs.clientinfo[cg.clientNum].shoutcaster && !cg.demoPlayback)
	{
		return;
	}

	if (!(cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		return;
	}

	scale  = CG_ComputeScale(comp);
	scale2 = scale * 0.842;

	// draw name box
	if (comp->showBackGround)
	{
		CG_FillRect(nameBoxX, nameBoxY, nameBoxWidth, nameBoxHeight, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(nameBoxX, nameBoxY, nameBoxWidth, nameBoxHeight, 2, comp->colorBorder);
	}

	// draw team flag
	if (player->team == TEAM_ALLIES)
	{
		CG_DrawPic(nameBoxX + 4, nameBoxY + (nameBoxHeight / 2) - 4.5f, 14, 9, cgs.media.alliedFlag);
	}
	else
	{
		CG_DrawPic(nameBoxX + 4, nameBoxY + (nameBoxHeight / 2) - 4.5f, 14, 9, cgs.media.axisFlag);
	}

	// draw name limit 20 chars, width 110
	Q_ColorizeString('7', player->cleanname, name, MAX_NAME_LENGTH + 2);
	textWidth  = CG_Text_Width_Ext(name, scale, 0, FONT_TEXT);
	textHeight = CG_Text_Height_Ext(name, scale, 0, FONT_TEXT);
	if (textWidth > 110)
	{
		textWidth = 110;
	}
	CG_Text_Paint_Ext(nameBoxX + (nameBoxWidth / 2) - (textWidth / 2), nameBoxY + (nameBoxHeight / 2) + (textHeight / 2), scale, scale, comp->colorMain, name, 0, 20, comp->styleText, FONT_TEXT);

	// draw country flag
	CG_DrawFlag(nameBoxX + nameBoxWidth - 17, nameBoxY + (nameBoxHeight / 2) - 7, 1, player->clientNum);

	// draw stats box
	if (comp->showBackGround)
	{
		CG_FillRect(statsBoxX, statsBoxY, statsBoxWidth, statsBoxHeight, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(statsBoxX, statsBoxY, statsBoxWidth, statsBoxHeight, 2, comp->colorBorder);
	}

	// draw powerups
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

	CG_DrawShoutcastPlayerChargebar(statsBoxX, statsBoxY + statsBoxHeight, statsBoxWidth / 2, 2, BAR_BG | BAR_BGSPACING_X0Y0 | BAR_LEFT, comp->colorBackground);
	CG_DrawShoutcastPlayerStaminaBar(statsBoxX + (statsBoxWidth / 2), statsBoxY + statsBoxHeight, statsBoxWidth / 2, 2, BAR_BG | BAR_BGSPACING_X0Y0, comp->colorBackground);

	// draw ammo count
	CG_PlayerAmmoValue(&ammo, &clip, &akimbo, NULL);

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

		textWidth = CG_Text_Width_Ext(text, scale, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + statsBoxWidth - (textWidth / 2) - 16, statsBoxY + (statsBoxHeight / 2) + 2, scale, scale, comp->colorMain, text, 0, 0, comp->styleText, FONT_TEXT);
	}

	// draw weapon icon
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

	// draw hp
	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
		vec4_t hcolor;

		Vector4Copy(colorWhite, hcolor);
		CG_ColorForHealth(cg.snap->ps.stats[STAT_HEALTH], hcolor);
		text       = va("%i", cg.snap->ps.stats[STAT_HEALTH]);
		textWidth  = CG_Text_Width_Ext(text, scale, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext(text, scale, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX - (textWidth / 2) + 10, statsBoxY + (statsBoxHeight / 2) + (textHeight / 2), scale, scale, hcolor, text, 0, 0, comp->styleText, FONT_TEXT);
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

	// draw class
	CG_DrawPic(statsBoxX + 1, statsBoxY + (statsBoxHeight / 2) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->cls)]);
	statsBoxX += 13;

	if (player->cls != player->latchedcls)
	{
		// arrow latched class
		textWidth  = CG_Text_Width_Ext("->", 0.19f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("->", 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 1, statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 1, scale, scale, colorYellow, "->", 0, 0, comp->styleText, FONT_TEXT);
		statsBoxX += 1 + textWidth;
		// latched class
		CG_DrawPic(statsBoxX + 1, statsBoxY + (statsBoxHeight / 2) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->latchedcls)]);
	}

	CG_RequestPlayerStats(ps->clientNum);

	if (gs->nClientID == player->clientNum && gs->fHasStats)
	{
		statsBoxX = statsBoxX + 55;

		dmgGiven  = va("%s", CG_ParseStats(gs->strExtra[0], 1));
		dmgRcvd   = va("%s", CG_ParseStats(gs->strExtra[1], 1));
		kills     = va("%s", CG_ParseStats(gs->strExtra[3], 1));
		deaths    = va("%s", CG_ParseStats(gs->strExtra[4], 1));
		selfkills = va("%s", CG_ParseStats(gs->strExtra[4], 2));

		// kills
		textWidth  = CG_Text_Width_Ext("K", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("K", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 6, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), scale2, scale2, comp->colorSecondary, "K", 0, 0, comp->styleText, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(kills, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(kills, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 6 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, scale, scale, colorWhite, kills, 0, 0, comp->styleText, FONT_TEXT);
		statsBoxX += 6 + textWidth2;

		// deaths
		textWidth  = CG_Text_Width_Ext("D", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("D", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 6, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), scale2, scale2, comp->colorSecondary, "D", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(deaths, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(deaths, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 6 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, scale, scale, colorWhite, deaths, 0, 0, comp->styleText, FONT_TEXT);
		statsBoxX += 6 + textWidth2;

		// selfkills
		textWidth  = CG_Text_Width_Ext("SK", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("SK", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 4, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), scale2, scale2, comp->colorSecondary, "SK", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(selfkills, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(selfkills, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 4 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, scale, scale, colorWhite, selfkills, 0, 0, comp->styleText, FONT_TEXT);
		statsBoxX += 4 + textWidth2;

		// dmgGiven
		textWidth  = CG_Text_Width_Ext("DG", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("DG", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 15, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), scale2, scale2, comp->colorSecondary, "DG", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(dmgGiven, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(dmgGiven, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 15 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, scale, scale, colorMdGreen, dmgGiven, 0, 0, comp->styleText, FONT_TEXT);
		statsBoxX += 15 + textWidth2;

		// dmgRcvd
		textWidth  = CG_Text_Width_Ext("DR", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("DR", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 7, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), scale2, scale2, comp->colorSecondary, "DR", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(dmgRcvd, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(dmgRcvd, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 7 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, scale, scale, colorRed, dmgRcvd, 0, 0, comp->styleText, FONT_TEXT);
		//statsBoxX += 5 + textWidth2;
	}
}

/**
 * @brief CG_DrawShoutcastTeamNames
 * @param[in] comp
 */
static void CG_DrawShoutcastTeamNames(hudComponent_t *comp, char *text)
{
	int   textWidth;
	int   textHeight;
	float scale;

	if (cgs.gamestats.show == SHOW_ON)
	{
		return;
	}

	if (comp->showBackGround)
	{
		GradientBar_Paint(&comp->location, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 2, comp->colorBorder);
	}

	scale = CG_ComputeScale(comp);

	// max width 174, limit 20 chars
	textWidth  = MIN(CG_Text_Width_Ext(text, scale, 0, FONT_TEXT), 174);
	textHeight = CG_Text_Height_Ext(text, scale, 0, FONT_TEXT);

	CG_Text_Paint_Ext(comp->location.x + (comp->location.w / 2) - (textWidth / 2) + 1.35f, comp->location.y + (comp->location.h / 2) + (textHeight / 2) + 1.35f, scale, scale, comp->colorSecondary, text, 0, 20, comp->styleText, FONT_TEXT);
	CG_Text_Paint_Ext(comp->location.x + (comp->location.w / 2) - (textWidth / 2), comp->location.y + (comp->location.h / 2) + (textHeight / 2), scale, scale, comp->colorMain, text, 0, 20, comp->styleText, FONT_TEXT);
}

/**
 * @brief CG_DrawShoutcastTeamNameAxis
 * @param[in] comp
 */
void CG_DrawShoutcastTeamNameAxis(hudComponent_t *comp)
{
	if (cgs.gamestats.show == SHOW_ON)
	{
		return;
	}

	if (!cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	// draw axis label
	CG_DrawShoutcastTeamNames(comp, Q_PrintStrlen(cg_shoutcastTeamNameRed.string) > 0
	                                    ? cg_shoutcastTeamNameRed.string : "Axis");
}

/**
 * @brief CG_DrawShoutcastTeamNameAllies
 * @param[in] comp
 */
void CG_DrawShoutcastTeamNameAllies(hudComponent_t *comp)
{
	if (cgs.gamestats.show == SHOW_ON)
	{
		return;
	}

	if (!cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	// draw allies label
	CG_DrawShoutcastTeamNames(comp, Q_PrintStrlen(cg_shoutcastTeamNameBlue.string) > 0
	                                    ? cg_shoutcastTeamNameBlue.string : "Allies");
}

/**
* @brief CG_ToggleShoutcasterMode
*        set event handling to CGAME_EVENT_SHOUTCAST so we can listen to keypresses
* @param[in] shoutcaster
*/
void CG_ToggleShoutcasterMode(int shoutcaster)
{
	if (shoutcaster)
	{
		CG_EventHandling(CGAME_EVENT_SHOUTCAST, qfalse);
	}
	else
	{
		CG_EventHandling(CGAME_EVENT_NONE, qfalse);
	}
}

/**
* @brief CG_ShoutcastCheckKeyCatcher
*
* @details track the moment when key catcher is changed away from KEYCATCH_UI
*          so we can set back event handling to CGAME_EVENT_SHOUTCAST
*          and key catcher to KEYCATCH_CGAME for shoutcaster follow keybinds
*
* @param[in] keycatcher
*/
void CG_ShoutcastCheckKeyCatcher(int keycatcher)
{
	// going out of ui menu
	if (cgs.clientinfo[cg.clientNum].shoutcaster && cgs.eventHandling == CGAME_EVENT_NONE &&
	    cg.snap->ps.pm_type != PM_INTERMISSION && !(keycatcher & KEYCATCH_UI) && (cg.lastKeyCatcher & KEYCATCH_UI))
	{
		CG_ToggleShoutcasterMode(1);
	}

	// going out of limbo menu, hud editor
	if (cgs.clientinfo[cg.clientNum].shoutcaster && cgs.eventHandling == CGAME_EVENT_NONE &&
	    cg.snap->ps.pm_type != PM_INTERMISSION && !(keycatcher & KEYCATCH_UI))
	{
		CG_ToggleShoutcasterMode(1);
	}

	// resolution changes don't automatically close the ui menus but show confirmation window after vid_restart
	// so need to turn off shoutcast event handling otherwise mouse cursor will not work
	if (cgs.clientinfo[cg.clientNum].shoutcaster && cgs.eventHandling == CGAME_EVENT_SHOUTCAST && (keycatcher & KEYCATCH_UI))
	{
		CG_ToggleShoutcasterMode(0);
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

	if (key >= K_F1 && key <= K_F12)
	{
		if (doaction)
		{
			trap_SendClientCommand(va("follow %d", players[key - K_F1]));
		}

		return qtrue;
	}

	return qfalse;
}
