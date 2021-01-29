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

#define PLAYER_OVERLAY_BORDER_DISTANCE_X 15
#define PLAYER_OVERLAY_BORDER_DISTANCE_Y 170
#define PLAYER_OVERLAY_BOX_WIDTH 170
#define PLAYER_OVERLAY_BOX_HEIGHT 28

#define PLAYER_STATUS_OVERLAY_NAMEBOX_WIDTH 150
#define PLAYER_STATUS_OVERLAY_NAMEBOX_HEIGHT 16

#define PLAYER_STATUS_OVERLAY_NAMEBOX_X (Ccg_WideX(SCREEN_WIDTH) - (Ccg_WideX(SCREEN_WIDTH) / 2) - (PLAYER_STATUS_OVERLAY_NAMEBOX_WIDTH / 2))
#define PLAYER_STATUS_OVERLAY_NAMEBOX_Y (SCREEN_HEIGHT - 75)

#define PLAYER_STATUS_OVERLAY_STATSBOX_WIDTH 220
#define PLAYER_STATUS_OVERLAY_STATSBOX_HEIGHT 20

#define PLAYER_STATUS_OVERLAY_STATSBOX_X (Ccg_WideX(SCREEN_WIDTH) - (Ccg_WideX(SCREEN_WIDTH) / 2) - (PLAYER_STATUS_OVERLAY_STATSBOX_WIDTH / 2))
#define PLAYER_STATUS_OVERLAY_STATSBOX_Y (PLAYER_STATUS_OVERLAY_NAMEBOX_Y + PLAYER_STATUS_OVERLAY_NAMEBOX_HEIGHT)

static vec4_t border      = { 1.0f, 1.0f, 1.0f, 0.7f };
static vec4_t border2     = { 1.0f, 1.0f, 1.0f, 0.5f };
static vec4_t bg          = { 0.0f, 0.0f, 0.0f, 0.7f };
static vec4_t colorAllies = { 0.0f, 0.36f, 0.52f, 0.3f };
static vec4_t colorAxis   = { 0.77f, 0.0f, 0.2f, 0.3f };
static vec4_t colorGreen  = { 0.0f, 1.0f, 0.0f, 1.0f };


/**
* @brief CG_DrawMinimap
*/
void CG_DrawMinimap()
{
	float      x = 20, y = 15, w = 150, h = 150;
	snapshot_t *snap;
	float      angle;
	int        i;

	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport)
	{
		snap = cg.nextSnap;
	}
	else
	{
		snap = cg.snap;
	}

	if (snap->ps.pm_flags & PMF_LIMBO /*|| snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR*/
#ifdef FEATURE_MULTIVIEW
	    || cg.mvTotalClients > 0
#endif
	    )
	{
		CG_DrawExpandedAutoMap();
		return;
	}

	/*if (!cg_altHud.integer)
	{
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
	}*/

	/*if ((snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster) || !cg_drawCompass.integer)
	{
	return;
	}*/

	CG_DrawNewAutoMap(x, y, w, h);


}

/**
* @brief CG_DrawShoutcastOverlay
*/
void CG_DrawShoutcastOverlay()
{
	clientInfo_t *ci;
	int          axis   = 0;
	int          allies = 0;
	int          i;

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

		if (ci->team == TEAM_ALLIES && allies < MAX_ALLIES)
		{
			CG_DrawShoutcastPlayerOverlayAllies(ci, PLAYER_OVERLAY_BORDER_DISTANCE_X, PLAYER_OVERLAY_BORDER_DISTANCE_Y + (PLAYER_OVERLAY_BOX_HEIGHT * allies) + (1 * allies));
			allies++;
		}
		if (ci->team == TEAM_AXIS && axis < MAX_AXIS)
		{
			CG_DrawShoutcastPlayerOverlayAxis(ci, Ccg_WideX(SCREEN_WIDTH) - PLAYER_OVERLAY_BOX_WIDTH - PLAYER_OVERLAY_BORDER_DISTANCE_X, PLAYER_OVERLAY_BORDER_DISTANCE_Y + (PLAYER_OVERLAY_BOX_HEIGHT * axis) + (1 * axis));
			axis++;
		}
		if (allies > MAX_ALLIES && axis > MAX_AXIS)
		{
			break;
		}
	}

	//CG_DrawMinimap();
	//CG_EventHandling(CGAME_EVENT_SHOUTCAST, qfalse);
}

/**
* @brief CG_DrawShoutcastPlayerOverlayAllies
* @param[in] player
* @param[in] x
* @param[in] y
*/
void CG_DrawShoutcastPlayerOverlayAllies(clientInfo_t *player, float x, float y)
{
	int   curWeap;
	float fraction;
	int   textWidth, textHeight;
	char  *text;
	float topRowX    = x;
	float bottomRowX = x;
	int   weapScale;

	CG_FillRect(x, y, PLAYER_OVERLAY_BOX_WIDTH, PLAYER_OVERLAY_BOX_HEIGHT, bg);
	CG_DrawRect_FixedBorder(x, y, PLAYER_OVERLAY_BOX_WIDTH, PLAYER_OVERLAY_BOX_HEIGHT, 1, border);
	CG_DrawRect_FixedBorder(x, y, PLAYER_OVERLAY_BOX_WIDTH, PLAYER_OVERLAY_BOX_HEIGHT / 2, 1, border);

	//HP bar
	fraction = (float)player->health / (float)CG_GetPlayerMaxHealth(player->clientNum, player->cls, player->team);
	CG_FilledBar(topRowX + 0.5f, y + 0.5f, PLAYER_OVERLAY_BOX_WIDTH - 0.5f, PLAYER_OVERLAY_BOX_HEIGHT / 2 - 1, colorAllies, colorAllies, bg, fraction, BAR_BGSPACING_X0Y0);

	//Health, height sometimes is 5 instead of 6 so hardcoded it to 6 / 2 = 3
	if (player->health > 0)
	{
		text      = va("%i", player->health);
		textWidth = CG_Text_Width_Ext(text, 0.2f, 0, FONT_TEXT);
		//textHeight = CG_Text_Height_Ext(text, 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(topRowX + 11 - (textWidth / 2), y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) + 3, 0.2f, 0.2f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		//topRowX += 11;
	}
	else if (player->health == 0)
	{
		CG_DrawPic(topRowX + 5, y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) - 6, 12, 12, cgs.media.medicIcon);
		//topRowX += 17;
	}
	else if (player->health < 0)
	{
		CG_DrawPic(topRowX + 5, y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) - 6, 12, 12, cgs.media.scoreEliminatedShader);
		//topRowX += 17;
	}

	//Name, cut too long names?
	//textWidth = CG_Text_Width_Ext(player->cleanname, 0.2f, 0, FONT_TEXT);
	textHeight = CG_Text_Height_Ext(player->cleanname, 0.2f, 0, FONT_TEXT);
	CG_Text_Paint_Ext(x + 26, y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) + (textHeight / 2), 0.2f, 0.2f, colorWhite, player->cleanname, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//Follow bind
	//textWidth = CG_Text_Width_Ext("(F12)", 0.15f, 0, FONT_TEXT);
	//textHeight = CG_Text_Height_Ext("(F12)", 0.15f, 0, FONT_TEXT);
	//CG_Text_Paint_Ext(x + PLAYER_OVERLAY_BOX_WIDTH - textWidth - 3, y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) + 2.0f, 0.15f, 0.15f, colorWhite, "(F12)", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//Class
	CG_DrawPic(bottomRowX + 4, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->cls)]);
	bottomRowX += 16;

	if (player->cls != player->latchedcls)
	{
		//Arrow latched class
		textWidth  = CG_Text_Width_Ext("->", 0.2f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("->", 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(bottomRowX, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) + (textHeight / 2) + 0.5f, 0.2f, 0.2f, colorYellow, "->", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		bottomRowX += textWidth;
		//Latched class
		CG_DrawPic(bottomRowX + 1, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->latchedcls)]);
	}

	bottomRowX = x + PLAYER_OVERLAY_BOX_WIDTH;

	//PowerUps
	if (player->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
	{
		CG_DrawPic(bottomRowX - 14, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, cgs.media.objectiveShader);
		bottomRowX -= 14;
	}
	else if (player->powerups & (1 << PW_OPS_DISGUISED))
	{
		CG_DrawPic(bottomRowX - 14, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, player->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		bottomRowX -= 14;
	}
	else if (player->powerups & (1 << PW_INVULNERABLE))
	{
		CG_DrawPic(bottomRowX - 14, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, cgs.media.spawnInvincibleShader);
		bottomRowX -= 14;
	}

	curWeap   = CG_GetPlayerCurrentWeapon(player);
	weapScale = cg_weapons[curWeap].weaponIconScale * 10;
	// note: WP_NONE is excluded
	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[0])     // do not try to draw nothing
	{
		CG_DrawPic(x + PLAYER_OVERLAY_BOX_WIDTH - weapScale - (weapScale / 2) - 32, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[0]);
	}
	else if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[1])
	{
		CG_DrawPic(x + PLAYER_OVERLAY_BOX_WIDTH - weapScale - (weapScale / 2) - 32, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[1]);
	}

	//Small health bar (Should be charge bar ideally)
	//CG_DrawRect(x + 4, y + 15.5f, 4, 10, 0.4f, border);
	//CG_FilledBar(x + 4.5f, y + 16, 3, 9, colorGreen, colorGreen, bg, fraction, BAR_BGSPACING_X0Y0 | BAR_VERT | BAR_LEFT);
}

/**
* @brief CG_DrawShoutcastPlayerOverlayAxis
* @param[in] player
* @param[in] x
* @param[in] y
*/
void CG_DrawShoutcastPlayerOverlayAxis(clientInfo_t *player, float x, float y)
{
	int   curWeap;
	float fraction;
	int   textWidth, textHeight;
	char  *text;
	float topRowX    = x;
	float bottomRowX = x + PLAYER_OVERLAY_BOX_WIDTH;
	int   weapScale;

	CG_FillRect(x, y, PLAYER_OVERLAY_BOX_WIDTH, PLAYER_OVERLAY_BOX_HEIGHT, bg);
	CG_DrawRect_FixedBorder(x, y, PLAYER_OVERLAY_BOX_WIDTH, PLAYER_OVERLAY_BOX_HEIGHT, 1, border);
	CG_DrawRect_FixedBorder(x, y, PLAYER_OVERLAY_BOX_WIDTH, PLAYER_OVERLAY_BOX_HEIGHT / 2, 1, border);

	//HP bar
	fraction = (float)player->health / (float)CG_GetPlayerMaxHealth(player->clientNum, player->cls, player->team);
	CG_FilledBar(topRowX + 0.5f, y + 0.5f, PLAYER_OVERLAY_BOX_WIDTH - 0.5f, PLAYER_OVERLAY_BOX_HEIGHT / 2 - 1, colorAxis, colorAxis, bg, fraction, BAR_BGSPACING_X0Y0 | BAR_LEFT);

	topRowX += PLAYER_OVERLAY_BOX_WIDTH;

	//Health, height sometimes is 5 instead of 6 so hardcoded it to 6 / 2 = 3
	if (player->health > 0)
	{
		text      = va("%i", player->health);
		textWidth = CG_Text_Width_Ext(text, 0.2f, 0, FONT_TEXT);
		//textHeight = CG_Text_Height_Ext(text, 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(topRowX - textWidth + (textWidth / 2) - 11, y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) + 3, 0.2f, 0.2f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		//topRowX = topRowX - textWidth - 8;
	}
	else if (player->health == 0)
	{
		CG_DrawPic(topRowX - 17, y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) - 6, 12, 12, cgs.media.medicIcon);
		//topRowX = topRowX - 20;
	}
	else if (player->health < 0)
	{
		CG_DrawPic(topRowX - 17, y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) - 6, 12, 12, cgs.media.scoreEliminatedShader);
		//topRowX = topRowX - 20;
	}

	//Name, cut too long names?
	textWidth  = CG_Text_Width_Ext(player->cleanname, 0.2f, 0, FONT_TEXT);
	textHeight = CG_Text_Height_Ext(player->cleanname, 0.2f, 0, FONT_TEXT);
	CG_Text_Paint_Ext(x + PLAYER_OVERLAY_BOX_WIDTH - textWidth - 26, y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) + (textHeight / 2), 0.2f, 0.2f, colorWhite, player->cleanname, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//Follow bind
	//textWidth = CG_Text_Width_Ext("(F11)", 0.15f, 0, FONT_TEXT);
	//textHeight = CG_Text_Height_Ext("(F11)", 0.15f, 0, FONT_TEXT);
	//CG_Text_Paint_Ext(x + 2, y + (PLAYER_OVERLAY_BOX_HEIGHT / 4) + 2.0f, 0.15f, 0.15f, colorWhite, "(F11)", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//trap_SendClientCommand(va("follow %d", 1));

	//Class
	CG_DrawPic(bottomRowX - 16, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->cls)]);
	bottomRowX -= 16;

	if (player->cls != player->latchedcls)
	{
		//Arrow latched class
		textWidth  = CG_Text_Width_Ext("<-", 0.2f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("<-", 0.2f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(bottomRowX - textWidth - 1, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) + (textHeight / 2) + 0.5f, 0.2f, 0.2f, colorYellow, "<-", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		bottomRowX = bottomRowX - textWidth - 1;
		//Latched class
		CG_DrawPic(bottomRowX - 12, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6, 12, 12, cgs.media.skillPics[SkillNumForClass(player->latchedcls)]);
	}

	bottomRowX = x;

	//PowerUps
	if (player->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
	{
		CG_DrawPic(bottomRowX + 2, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, cgs.media.objectiveShader);
		bottomRowX += 14;
	}
	else if (player->powerups & (1 << PW_OPS_DISGUISED))
	{
		CG_DrawPic(bottomRowX + 2, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, player->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		bottomRowX += 14;
	}
	else if (player->powerups & (1 << PW_INVULNERABLE))
	{
		CG_DrawPic(bottomRowX + 2, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 6.5f, 12, 12, cgs.media.spawnInvincibleShader);
		bottomRowX += 14;
	}

	curWeap   = CG_GetPlayerCurrentWeapon(player);
	weapScale = cg_weapons[curWeap].weaponIconScale * 10;

	// note: WP_NONE is excluded
	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[0])     // do not try to draw nothing
	{
		CG_DrawPic(x + 32, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 5, weapScale, 10, cg_weapons[curWeap].weaponIcon[0]);
	}
	else if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[1])
	{
		CG_DrawPic(x + 32, y + (PLAYER_OVERLAY_BOX_HEIGHT * 0.75f) - 5, weapScale, 10, cg_weapons[curWeap].weaponIcon[1]);
	}

	//Small health bar
	//CG_DrawRect(x - 8, y + 15.5f, 4, 10, 0.4f, border);
	//CG_FilledBar(x - 7.5f, y + 16, 3, 9, colorGreen, colorGreen, bg, fraction, BAR_BGSPACING_X0Y0 | BAR_VERT | BAR_LEFT);
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
* @brief CG_DrawShoutcastPlayerStatus
*/
void CG_DrawShoutcastPlayerStatus()
{
	gameStats_t   *gs = &cgs.gamestats;
	clientInfo_t  *player = &cgs.clientinfo[cg.snap->ps.clientNum];
	playerState_t *ps = &cg.snap->ps;
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

	//Draw name box
	CG_FillRect(nameBoxX, nameBoxY, nameBoxWidth, nameBoxHeight, bg);
	CG_DrawRect_FixedBorder(nameBoxX, nameBoxY, nameBoxWidth, nameBoxHeight, 1, border2);

	//Draw team flag
	if (player->team == TEAM_ALLIES)
	{
		CG_DrawPic(nameBoxX + 4, nameBoxY + (nameBoxHeight / 2) - 5, 14, 10, cgs.media.alliedFlag);
	}
	else
	{
		CG_DrawPic(nameBoxX + 4, nameBoxY + (nameBoxHeight / 2) - 5, 14, 10, cgs.media.axisFlag);
	}

	//Draw name
	textWidth  = CG_Text_Width_Ext(player->name, 0.19f, 0, FONT_TEXT);
	textHeight = CG_Text_Height_Ext(player->name, 0.19f, 0, FONT_TEXT);
	CG_Text_Paint_Ext(nameBoxX + (nameBoxWidth / 2) - (textWidth / 2), nameBoxY + (nameBoxHeight / 2) + (textHeight / 2), 0.19f, 0.19f, colorWhite, player->name, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

	//Draw country flag
	CG_DrawFlag(nameBoxX + nameBoxWidth - 17, nameBoxY + (nameBoxHeight / 2) - 7, 1, player->clientNum);

	//Draw stats box
	CG_FillRect(statsBoxX, statsBoxY, statsBoxWidth, statsBoxHeight, bg);
	CG_DrawRect_FixedBorder(statsBoxX, statsBoxY, statsBoxWidth, statsBoxHeight, 1, border2);

	//Draw powerups
	tmpX = statsBoxX + statsBoxWidth;
	if (ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG])
	{
		CG_DrawPic(tmpX + 3, statsBoxY, 20, 20, cgs.media.objectiveShader);
		tmpX = statsBoxX + statsBoxWidth + 23;
	}
	else if (ps->powerups[PW_OPS_DISGUISED])
	{
		CG_DrawPic(tmpX + 3, statsBoxY, 20, 20, player->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		tmpX = statsBoxX + statsBoxWidth + 23;
	}
	else if (ps->powerups[PW_INVULNERABLE])//(player->powerups & (1 << PW_INVULNERABLE))
	{
		CG_DrawPic(tmpX + 3, statsBoxY, 20, 20, cgs.media.spawnInvincibleShader);
	}

	//Draw small charge bar
	//CG_DrawRect(statsBoxX + statsBoxWidth - 8, statsBoxY + (statsBoxHeight / 2) - 5, 4, 10, 0.35f, border);
	CG_DrawRect_FixedBorder(statsBoxX + statsBoxWidth - 8, statsBoxY + (statsBoxHeight / 2) - 5, 4, 10, 1, border2);
	CG_DrawShoutcastPlayerChargebar(statsBoxX + statsBoxWidth - 7.7f, statsBoxY + (statsBoxHeight / 2) - 4.5f);

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

		textWidth  = CG_Text_Width_Ext(text, 0.19f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext(text, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + statsBoxWidth - textWidth - 10, statsBoxY + (statsBoxHeight / 2) + 2, 0.19f, 0.19f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
	}

	//Draw weapon icon
	curWeap   = CG_GetPlayerCurrentWeapon(player);
	weapScale = cg_weapons[curWeap].weaponIconScale * 10;

	// note: WP_NONE is excluded
	if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[0])     // do not try to draw nothing
	{
		CG_DrawPic(statsBoxX + statsBoxWidth - weapScale - (weapScale / 2) - 30, statsBoxY + (statsBoxHeight / 2) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[0]);
	}
	else if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[1])
	{
		CG_DrawPic(statsBoxX + statsBoxWidth - weapScale - (weapScale / 2) - 30, statsBoxY + (statsBoxHeight / 2) - 5, -weapScale, 10, cg_weapons[curWeap].weaponIcon[1]);
	}

	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
		//Draw hp
		text       = va("%i", cg.snap->ps.stats[STAT_HEALTH]);
		textWidth  = CG_Text_Width_Ext(text, 0.19f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext(text, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 4, statsBoxY + (statsBoxHeight / 2) + (textHeight / 2), 0.19f, 0.19f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 4 + textWidth;
	}
	else
	{
		CG_DrawPic(statsBoxX + 4, statsBoxY + (statsBoxHeight / 2) - 6, 12, 12, cgs.media.medicIcon);
		statsBoxX += 16;
	}

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
		statsBoxX += 8;
	}
	else
	{
		statsBoxX += 20;
	}

	CG_RequestPlayerStats(ps->clientNum);

	if (gs->nClientID == player->clientNum && gs->fHasStats)
	{
		//TODOryzyk
		//dmgGiven = CG_ParseStats(gs->strExtra[0], 1); ???
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
		CG_Text_Paint_Ext(statsBoxX + 4, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), 0.16f, 0.16f, colorMdGrey, "D", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(deaths, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(deaths, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 4 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, 0.19f, 0.19f, colorWhite, deaths, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 4 + textWidth2;

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
		CG_Text_Paint_Ext(statsBoxX + 20, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), 0.16f, 0.16f, colorMdGrey, "DG", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(dmgGiven, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(dmgGiven, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 20 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, 0.19f, 0.19f, colorDkGreen, dmgGiven, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 20 + textWidth2;

		//dmgRcvd
		textWidth  = CG_Text_Width_Ext("DR", 0.16f, 0, FONT_TEXT);
		textHeight = CG_Text_Height_Ext("DR", 0.16f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 5, statsBoxY + (statsBoxHeight / 2) - (textHeight / 2), 0.16f, 0.16f, colorMdGrey, "DR", 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);

		textHeight = CG_Text_Height_Ext(dmgRcvd, 0.19f, 0, FONT_TEXT);
		textWidth2 = CG_Text_Width_Ext(dmgRcvd, 0.19f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(statsBoxX + 5 + (textWidth / 2) - (textWidth2 / 2), statsBoxY + (statsBoxHeight / 2) + (textHeight / 2) + 4, 0.19f, 0.19f, colorMdRed, dmgRcvd, 0, 0, ITEM_TEXTSTYLE_NORMAL, FONT_TEXT);
		statsBoxX += 5 + textWidth2;
	}
}

/**
* @brief CG_GetPlayerCurrentWeapon
* @param[in] player
*/
int CG_GetPlayerCurrentWeapon(clientInfo_t *player)
{
	int curWeap;
	// draw the player's weapon icon
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
* @brief CG_ParseStats
* @param[in] data
* @param[in] i
*/
char *CG_ParseStats(char *data, int i)
{
	int  c;
	int  stop = 0;
	int  len = 0;
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
				if (len < 6)
				{
					out = va("%s%c", out, c);
					len++;
				}

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
* @brief CG_DrawShoutcastPlayerChargebar
* @param[in] x
* @param[in] y
*/
void CG_DrawShoutcastPlayerChargebar(float x, float y)
{
	float    barFrac, chargeTime;
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
		color[0] = 1;
		color[1] = 0;
		color[2] = 0;
		color[3] = 1;
	}
	else
	{
		color[0] = 1 - barFrac;
		color[1] = barFrac;
		color[2] = 0;
		color[3] = 1;
	}

	CG_FilledBar(x, y, 3, 9, color, color, bg, barFrac, BAR_BGSPACING_X0Y0 | BAR_VERT | BAR_LEFT);
}

//TODOryzyk
//Some code might be redundant
/**
* @brief CG_DrawTimerShoutcast
*/
void CG_DrawTimerShoutcast()
{
	vec4_t color = { .6f, .6f, .6f, 1.f };
	char   *text, *rt, *rtAllies = "", *rtAxis = "", *round;
	int    tens;
	int    secondsThen;
	int    msec    = (cgs.timelimit * 60000.f) - (cg.time - cgs.levelStartTime); // 60.f * 1000.f
	int    seconds = msec / 1000;
	int    mins    = seconds / 60;
	int    w       = 60;
	int    h       = 30;
	int    x       = (Ccg_WideX(SCREEN_WIDTH) / 2) - (w / 2);
	int    y       = 12;
	int    textWidth;
	int    textHeight;

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
	CG_DrawRect_FixedBorder(x, y, w, h, 1, border);

	//Game time
	CG_Text_Paint_Ext(x + w / 2 - textWidth / 2, y + 13, 0.23f, 0.23f, color, text, 0, 0, 0, FONT_HEADER);

	//Allies reinf time
	CG_Text_Paint_Ext(x + 3, y + h - 5, 0.20f, 0.20f, color, rtAllies, 0, 0, 0, FONT_HEADER);

	//Axis reinf time
	textWidth = CG_Text_Width_Ext(rtAxis, 0.20f, 0, FONT_HEADER);
	if (textWidth == 10)
	{
		textWidth = 0;
	}
	CG_Text_Paint_Ext(x + w - textWidth - 3, y + h - 5, 0.20f, 0.20f, color, rtAxis, 0, 0, 0, FONT_HEADER);

	//Round number
	if (cgs.gametype == GT_WOLF_STOPWATCH)
	{
		round     = va("^7%i/2", cgs.currentRound + 1);
		textWidth = CG_Text_Width_Ext(round, 0.15f, 0, FONT_HEADER);
		CG_Text_Paint_Ext(x + w / 2 - textWidth / 2, y + h - 5.5f, 0.15f, 0.15f, color, round, 0, 0, 0, FONT_HEADER);
	}

	//TODOryzyk
	//get it out

	if (cg_shoutcastDrawTeamNames.integer)
	{
		//Draw allies label
		if (Q_PrintStrlen(cg_shoutcastTeamName1.string) > 0)
		{
			text = va("%s", cg_shoutcastTeamName1.string);
		}
		else
		{
			text = va("%s", "Allies");
		}

		CG_FilledBar(x - 200, y, 200, h, colorAllies, colorAllies, bg, 1.0f, BAR_BGSPACING_X0Y0);
		textWidth  = CG_Text_Width_Ext(text, 0.3f, 0, FONT_HEADER);
		textHeight = CG_Text_Height_Ext(text, 0.3f, 0, FONT_HEADER);
		CG_Text_Paint_Ext(x - 200 + (200 / 2) - (textWidth / 2), y + (h / 2) + (textHeight / 2), 0.3f, 0.3f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_HEADER);

		//Draw axis label
		if (Q_PrintStrlen(cg_shoutcastTeamName2.string) > 0)
		{
			text = va("%s", cg_shoutcastTeamName2.string);
		}
		else
		{
			text = va("%s", "Axis");
		}

		CG_FilledBar(x + w, y, 200, h, colorAxis, colorAxis, bg, 1.0f, BAR_BGSPACING_X0Y0);
		textWidth  = CG_Text_Width_Ext(text, 0.3f, 0, FONT_HEADER);
		textHeight = CG_Text_Height_Ext(text, 0.3f, 0, FONT_HEADER);
		CG_Text_Paint_Ext(x + w + (200 / 2) - (textWidth / 2), y + (h / 2) + (textHeight / 2), 0.3f, 0.3f, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_HEADER);
	}
}

extern void CG_toggleShoutcasterHelp_f(void);

//if (cg_specHelp.integer > 0 && !cg.demoPlayback)
//{
//	CG_ShowHelp_On(&cg.spechelpWindow);
//	CG_EventHandling(CGAME_EVENT_MULTIVIEW, qfalse);
//}

/**
* @brief CG_sc_KeyHandling
* @param[in] _key
* @param[in] down
*/
void CG_sc_KeyHandling(int key, qboolean down)
{
	int milli = trap_Milliseconds();
	// Avoid active console keypress issues
	if (!down && !cgs.fKeyPressed[key])
	{
		return;
	}

	cgs.fKeyPressed[key] = down;

	switch (key)
	{
	// Help info
	case K_ESCAPE:
		CG_EventHandling(CGAME_EVENT_GAMEVIEW, qfalse);
		break;
	case K_BACKSPACE:
		if (!down)
		{
			// Dushan - fixed comiler warning
			CG_toggleSpecHelp_f();
		}
		return;
	case K_F1:
		if (!down)
		{
			trap_SendClientCommand(va("follow %d", 1));
			//CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return;
	default:
		CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		CG_RunBinding(key, down);
		break;
	}
}
