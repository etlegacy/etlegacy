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
 * @file cg_fireteamoverlay.c
 */

#include "cg_local.h"

static int sortedFireTeamClients[MAX_CLIENTS];

#define FONT_HEADER         &cgs.media.limboFont1
#define FONT_TEXT           &cgs.media.limboFont2

/**
 * @brief CG_SortFireTeam
 * @param[in] a
 * @param[in] b
 * @return
 */
int QDECL CG_SortFireTeam(const void *a, const void *b)
{
	clientInfo_t *ca, *cb;
	int          cna, cnb;

	cna = *(const int *)a;
	cnb = *(const int *)b;

	ca = &cgs.clientinfo[cna];
	cb = &cgs.clientinfo[cnb];

	// not on our team, so shove back
	if (!CG_IsOnSameFireteam(cnb, cg.clientNum))
	{
		return -1;
	}
	if (!CG_IsOnSameFireteam(cna, cg.clientNum))
	{
		return 1;
	}

	// leader comes first
	if (CG_IsFireTeamLeader(cna))
	{
		return -1;
	}
	if (CG_IsFireTeamLeader(cnb))
	{
		return 1;
	}

	// then higher ranks
	if (ca->rank > cb->rank)
	{
		return -1;
	}
	if (cb->rank > ca->rank)
	{
		return 1;
	}

	// then score
	//if ( ca->score > cb->score ) {
	//  return -1;
	//}
	//if ( cb->score > ca->score ) {
	//  return 1;
	//}                                                                                                                       // not atm

	return 0;
}

/**
 * @brief Sorts client's fireteam by leader then rank
 */
void CG_SortClientFireteam()
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		sortedFireTeamClients[i] = i;
	}

	qsort(sortedFireTeamClients, MAX_CLIENTS, sizeof(sortedFireTeamClients[0]), CG_SortFireTeam);

	// debug
	//for(i = 0; i < MAX_CLIENTS; i++) {
	//	CG_Printf( "%i ", sortedFireTeamClients[i] );
	//}
	//C/G_Printf( "\n" );
}

/**
 * @brief Parses fireteam servercommand
 */
void CG_ParseFireteams()
{
	int        i, j;
	char       *s;
	const char *p;
	int        clnts[2];

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		cgs.clientinfo[i].fireteamData = NULL;
	}

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		char hexbuffer[11] = "0x00000000";

		p = CG_ConfigString(CS_FIRETEAMS + i);

		j = Q_atoi(Info_ValueForKey(p, "id"));
		if (j == -1)
		{
			cg.fireTeams[i].inuse = qfalse;
			continue;
		}
		else
		{
			cg.fireTeams[i].inuse = qtrue;
			cg.fireTeams[i].ident = j;
		}

		s                      = Info_ValueForKey(p, "l");
		cg.fireTeams[i].leader = Q_atoi(s);

		s                    = Info_ValueForKey(p, "p");
		cg.fireTeams[i].priv = (qboolean) !!Q_atoi(s);

		s = Info_ValueForKey(p, "c");
		Q_strncpyz(hexbuffer + 2, s, 9);
		Q_sscanf(hexbuffer, "%x", &clnts[1]);
		Q_strncpyz(hexbuffer + 2, s + 8, 9);
		Q_sscanf(hexbuffer, "%x", &clnts[0]);

		cg.fireTeams[i].membersNumber = 0;

		for (j = 0; j < cgs.maxclients; j++)
		{
			if (COM_BitCheck(clnts, j))
			{
				cg.fireTeams[i].membersNumber++;
				cg.fireTeams[i].joinOrder[j]   = qtrue;
				cgs.clientinfo[j].fireteamData = &cg.fireTeams[i];
				//CG_Printf("%s\n", cgs.clientinfo[j].name);
			}
			else
			{
				cg.fireTeams[i].joinOrder[j] = qfalse;
			}
		}
	}

	CG_SortClientFireteam();
}

/**
 * @brief Fireteam that the specified client is a part of
 * @param[in] clientNum
 * @return
 */
fireteamData_t *CG_IsOnFireteam(int clientNum)
{
	if (cgs.clientinfo[clientNum].team == TEAM_SPECTATOR)
	{
		return NULL;
	}
	return cgs.clientinfo[clientNum].fireteamData;
}

/**
 * @brief Fireteam that both specified clients are on, if they both are on the same team
 * @param[in] clientNum
 * @param[in] clientNum2
 * @return
 */
fireteamData_t *CG_IsOnSameFireteam(int clientNum, int clientNum2)
{
	if (CG_IsOnFireteam(clientNum) == CG_IsOnFireteam(clientNum2))
	{
		return CG_IsOnFireteam(clientNum);
	}

	return NULL;
}

/**
 * @brief Fireteam that specified client is leader of, or NULL if none
 * @param[in] clientNum
 * @return
 */
fireteamData_t *CG_IsFireTeamLeader(int clientNum)
{
	fireteamData_t *f;

	if (!(f = CG_IsOnFireteam(clientNum)))
	{
		return NULL;
	}

	if (f->leader != clientNum)
	{
		return NULL;
	}

	return f ;
}

/*
 * @brief Client, not on a fireteam, not sorted, but on your team
 * @param[in] pos
 * @param[in] max
 * @return Client information
 *
 * @note Unused
 *
clientInfo_t *CG_ClientInfoForPosition(int pos, int max)
{
    int i, cnt = 0;

    for (i = 0; i < MAX_CLIENTS && cnt < max; i++)
    {
        if (cg.clientNum != i && cgs.clientinfo[i].infoValid && !CG_IsOnFireteam(i) && cgs.clientinfo[cg.clientNum].team == cgs.clientinfo[i].team)
        {
            if (cnt == pos)
            {
                return &cgs.clientinfo[i];
            }
            cnt++;
        }
    }

    return NULL;
}
*/

/*
 * @brief Fireteam, that's on your same team
 * @param[in] pos
 * @param[in] max
 * @return
 *
 * @note Unused
 *
fireteamData_t *CG_FireTeamForPosition(int pos, int max)
{
    int i, cnt = 0;

    for (i = 0; i < MAX_FIRETEAMS && cnt < max; i++)
    {
        if (cg.fireTeams[i].inuse && cgs.clientinfo[cg.fireTeams[i].leader].team == cgs.clientinfo[cg.clientNum].team)
        {
            if (cnt == pos)
            {
                return &cg.fireTeams[i];
            }
            cnt++;
        }
    }

    return NULL;
}
*/

/*
 * @brief Client, not sorted by rank, on CLIENT'S fireteam
 * @param[in] pos
 * @param[in] max
 * @return
 *
 * @note Unused
 *
clientInfo_t *CG_FireTeamPlayerForPosition(int pos, int max)
{
    int            i, cnt = 0;
    fireteamData_t *f = CG_IsOnFireteam(cg.clientNum);

    if (!f)
    {
        return NULL;
    }

    for (i = 0; i < MAX_CLIENTS && cnt < max; i++)
    {
        if (cgs.clientinfo[i].infoValid && cgs.clientinfo[cg.clientNum].team == cgs.clientinfo[i].team)
        {
            if (!(f == CG_IsOnFireteam(i)))
            {
                continue;
            }

            if (cnt == pos)
            {
                return &cgs.clientinfo[i];
            }
            cnt++;
        }
    }

    return NULL;
}
*/

/**
 * @brief Client, sorted by rank, on CLIENT'S fireteam
 * @param[in] pos
 * @return
 */
clientInfo_t *CG_SortedFireTeamPlayerForPosition(int pos)
{
	int            i;
	int            cnt = 0;
	fireteamData_t *f  = CG_IsOnFireteam(cg.clientNum);

	if (!f)
	{
		return NULL;
	}

	for (i = 0; i < cgs.maxclients && cnt < MAX_FIRETEAM_MEMBERS; i++)
	{
		if (!(f == CG_IsOnFireteam(sortedFireTeamClients[i])))
		{
			return NULL;
		}

		if (cnt == pos)
		{
			return &cgs.clientinfo[sortedFireTeamClients[i]];
		}
		cnt++;
	}

	return NULL;
}

// Main Functions

typedef enum
{
	TIMEOUT,
	WOUNDED,
	DEAD,
	NONE
} fireteamMemberStatusEnum_t;

static fireteamMemberStatusEnum_t CG_FireTeamMemberStatus(clientInfo_t *ci);
static vec4_t * CG_FireTeamNameColor(fireteamMemberStatusEnum_t status);

static ID_INLINE int CG_FireTeamClientCurrentWeapon(clientInfo_t *ci)
{
	if (cg_entities[ci->clientNum].currentState.eFlags & EF_MOUNTEDTANK)
	{
		return IS_MOUNTED_TANK_BROWNING(ci->clientNum) ? WP_MOBILE_BROWNING : WP_MOBILE_MG42;
	}
	else if ((cg_entities[ci->clientNum].currentState.eFlags & EF_MG42_ACTIVE) || (cg_entities[ci->clientNum].currentState.eFlags & EF_AAGUN_ACTIVE))
	{
		return WP_MOBILE_MG42;
	}
	else
	{
		return cg_entities[ci->clientNum].currentState.weapon;
	}
}

/**
 * @brief Draw FireTeam overlay
 * @param[in] rect
 */
void CG_DrawFireTeamOverlay(hudComponent_t *comp)
{
	float          x = comp->location.x;
	float          y = comp->location.y;
	float          locwidth, namewidth, weapIconWidthScale;
	int            i, puwidth, lineX;
	int            bestNameWidth          = -1;
	int            bestLocWidth           = -1;
	int            bestWeapIconWidthScale = -1;
	char           buffer[64];
	float          w, computedWidth, h;
	float          heighTitle, heightText, iconsSize, weaponIconSize, heightTextOffset, heightIconsOffset, heightWeaponIconOffset;
	clientInfo_t   *ci = NULL;
	fireteamData_t *f  = NULL;
	char           *locStr[MAX_FIRETEAM_MEMBERS];
	int            curWeap;
	char           name[MAX_FIRETEAM_MEMBERS][MAX_NAME_LENGTH];
	int            nameMaxLen;
	float          scale;
	float          spacing;

	// colors and fonts for overlays
	vec4_t FT_select                = { 0.5f, 0.5f, 0.2f, 0.3f }; // selected member
	vec4_t iconColor                = { 1.0f, 1.0f, 1.0f, 1.0f }; // icon "color", used for alpha adjustments
	vec4_t iconColorSemitransparent = { 1.0f, 1.0f, 1.0f, 0.5f };
	vec4_t textWhite                = { 1.0f, 1.0f, 1.0f, 1.0f }; // regular text
	vec4_t textYellow               = { 1.0f, 1.0f, 0.0f, 1.0f }; // yellow text for health drawing
	vec4_t textRed                  = { 1.0f, 0.0f, 0.0f, 1.0f }; // red text for health drawing
	vec4_t textOrange               = { 1.0f, 0.6f, 0.0f, 1.0f }; // orange text for ping issues
	vec4_t nameColor                = { 1.0f, 1.0f, 1.0f, 1.0f };

	// early exits
	if (
		// we are a shoutcaster
		cgs.clientinfo[cg.clientNum].shoutcaster
		// or we are not on a fireteam
		|| !CG_IsOnFireteam(cg.clientNum)
		// or assign fireteam data, and early out if not on one
		|| !(f = CG_IsOnFireteam(cg.clientNum))
		)
	{
		// XXX : TODO : we currently don't generate any noise for the
		// fireteamoverlay, as it's pretty involved - so we do the minimum here,
		// which is to just draw a box filling the component size, which also
		// mentions this fact
		if (cg.generatingNoiseHud)
		{
			const char *info = "Please join a real Fireteam!";
			int        i     = 0;

			// draw the box
			w = comp->location.w;
			h = comp->location.h;
			CG_FillRect(x, y, w, h * (i + 1), comp->colorBackground);
			CG_DrawRect_FixedBorder(x, y, w, h * (i + 1), 1, comp->colorBorder);
			CG_FillRect(x + 1, y + 1, w - 2, h - 1, comp->colorSecondary);

			// draw a text info on it
			scale      = CG_ComputeScale(comp /* h, comp->scale, FONT_TEXT*/);
			heighTitle = CG_Text_Height_Ext(info, scale, 0, FONT_HEADER);
			CG_Text_Paint_Ext(x + 4, comp->location.y + ((heighTitle + h) * 0.5), scale, scale, comp->colorMain, info, 0, 0, 0, FONT_TEXT);
		}

		return;
	}

	Com_Memset(locStr, 0, sizeof(char *) * MAX_FIRETEAM_MEMBERS);
	for (i = 0; i < MAX_FIRETEAM_MEMBERS; i++)
	{
		Com_Memset(name[i], 0, sizeof(char) * (MAX_NAME_LENGTH));
	}

	h = comp->location.h / (MAX_FIRETEAM_MEMBERS + 1);

	scale   = CG_ComputeScale(comp /* h, comp->scale, FONT_TEXT*/);
	spacing = CG_Text_Width_Ext_Float("_", scale, 0, FONT_TEXT);

	// First get name and location width, also store location names
	for (i = 0; i < MAX_FIRETEAM_MEMBERS; i++)
	{
		ci = CG_SortedFireTeamPlayerForPosition(i);

		// Make sure it's valid
		if (!ci)
		{
			break;
		}

		if (cg_locations.integer & LOC_FTEAM)
		{
			locStr[i] = CG_BuildLocationString(ci->clientNum, ci->location, LOC_FTEAM);

			if (!locStr[i][1] || !*locStr[i])
			{
				locStr[i] = 0;
			}

			// cap max location length?
			if (cg_locationMaxChars.integer)
			{
				locwidth  = spacing;
				locwidth *= Com_Clamp(0, 128, cg_locationMaxChars.integer);     // 128 is max location length
			}
			else
			{
				locwidth = CG_Text_Width_Ext(locStr[i], scale, 0, FONT_TEXT);
			}
		}
		else
		{
			locwidth = 0;
		}

		if (comp->style & BIT(2) || (comp->style & BIT(3) && (ci->health <= 0 || ci->ping >= 999)))
		{
			Q_strncpyz(name[i], ci->cleanname, sizeof(name[i]));
		}
		else
		{
			Q_strncpyz(name[i], ci->name, sizeof(name[i]));
		}


		// truncate name if max chars is set
		if (cg_fireteamNameMaxChars.integer)
		{
			nameMaxLen = Com_Clamp(0, MAX_NAME_LENGTH - 1, cg_fireteamNameMaxChars.integer);
			Q_strncpyz(name[i], Q_TruncateStr(name[i], nameMaxLen), sizeof(name[i]));

			// if alignment is requested, keep a static width
			if (cg_fireteamNameAlign.integer)
			{
				namewidth  = spacing;
				namewidth *= Com_Clamp(0, MAX_NAME_LENGTH, cg_fireteamNameMaxChars.integer);
			}
			else
			{
				namewidth = CG_Text_Width_Ext(name[i], scale, 0, FONT_TEXT);
			}
		}
		else
		{
			namewidth = CG_Text_Width_Ext(name[i], scale, 0, FONT_TEXT);
		}

		if (ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG) | (1 << PW_OPS_DISGUISED)))
		{
			namewidth += 14;
		}

		// find the widest weapons icons
		curWeap = CG_FireTeamClientCurrentWeapon(ci);

		if (IS_VALID_WEAPON(curWeap) && (cg_weapons[curWeap].weaponIcon[0] || cg_weapons[curWeap].weaponIcon[1]))     // do not try to draw nothing
		{
			weapIconWidthScale = cg_weapons[curWeap].weaponIconScale;
		}
		else
		{
			weapIconWidthScale = 1;
		}

		if (namewidth > bestNameWidth)
		{
			bestNameWidth = namewidth;
		}

		if (locwidth > bestLocWidth)
		{
			bestLocWidth = locwidth;
		}

		if (weapIconWidthScale > bestWeapIconWidthScale)
		{
			bestWeapIconWidthScale = weapIconWidthScale;
		}
	}

	heightText             = CG_Text_Height_Ext("A", scale, 0, FONT_TEXT);
	iconsSize              = heightText * 2.5f;
	weaponIconSize         = heightText * 2;
	heightTextOffset       = (h + heightText) * 0.5f;
	heightIconsOffset      = (h - iconsSize) * 0.5f;
	heightWeaponIconOffset = (h - heightText * 2) * 0.5f;

	// NOTE: terrible way to do it ... but work
	computedWidth = spacing
	                + iconsSize                                         // class icons
	                + spacing * 2 + iconsSize + spacing                 // latched class
	                + iconsSize + spacing                               // objective icon
	                + spacing * 2 + bestNameWidth                       // player name
	                + bestWeapIconWidthScale * weaponIconSize + spacing // weapon icons
	                + spacing * 3 + spacing                             // health points
	                + bestLocWidth;                                     // location name

	// keep the best fit for the location text
	w = MIN(computedWidth, comp->location.w);

	// fireteam alpha adjustments

	// set background alpha first
	FT_select[3]  *= comp->colorMain[3];
	iconColor[3]  *= comp->colorMain[3];
	textWhite[3]  *= comp->colorMain[3];
	textYellow[3] *= comp->colorMain[3];
	textRed[3]    *= comp->colorMain[3];
	textOrange[3] *= comp->colorMain[3];
	nameColor[3]  *= comp->colorMain[3];

	if (comp->showBackGround)
	{
		CG_FillRect(x, y, w, h * (i + 1), comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(x, y, w, h * (i + 1), 1, comp->colorBorder);
	}


	if (!(comp->style & BIT(1)))
	{
		CG_FillRect(x + 1, y + 1, w - 2, h - 1, comp->colorSecondary);

		if (f->priv)
		{
			Com_sprintf(buffer, 64, CG_TranslateString("Private Fireteam: %s"), cgs.clientinfo[cg.clientNum].team == TEAM_AXIS ? bg_fireteamNamesAxis[f->ident] : bg_fireteamNamesAllies[f->ident]);
		}
		else
		{
			Com_sprintf(buffer, 64, CG_TranslateString("Fireteam: %s"), cgs.clientinfo[cg.clientNum].team == TEAM_AXIS ? bg_fireteamNamesAxis[f->ident] : bg_fireteamNamesAllies[f->ident]);
		}

		Q_strupr(buffer);
		heighTitle = CG_Text_Height_Ext(buffer, scale, 0, FONT_HEADER);
		CG_Text_Paint_Ext(x + 4, comp->location.y + ((heighTitle + h) * 0.5), scale, scale, comp->colorMain, buffer, 0, 0, 0, FONT_HEADER);
	}

	lineX = (int)x;

	for (i = 0; i < MAX_FIRETEAM_MEMBERS; i++)
	{
		x = lineX;

		if (i != 0 || !(comp->style & BIT(1)))
		{
			y += h;
		}
		// grab a pointer to the current player
		ci = CG_SortedFireTeamPlayerForPosition(i);

		// make sure it's valid
		if (!ci)
		{
			break;
		}

		if (comp->style & BIT(3) || comp->style & BIT(4))
		{
			fireteamMemberStatusEnum_t status = CG_FireTeamMemberStatus(ci);
			vec3_copy(*CG_FireTeamNameColor(status), nameColor);
			if (comp->style & BIT(4) && status != NONE)
			{
				vec4_t rowColor;
				vec4_copy(nameColor, rowColor);
				rowColor[3] = comp->colorBackground[3];
				CG_FillRect(x, y, w, h, rowColor);
			}
		}
		else
		{
			vec4_copy(textWhite, nameColor);
		}

		if (ci->selected)
		{
			// highlight selected players
			CG_FillRect(x, y, w, h, FT_select);
		}

		x += spacing;

		// draw class icon in fireteam overlay
		trap_R_SetColor(iconColor);
		CG_DrawPic(x, y + heightIconsOffset, iconsSize, iconsSize, cgs.media.skillPics[SkillNumForClass(ci->cls)]);
		x += iconsSize;

		if (comp->style & 1)
		{
			if (ci->cls != ci->latchedcls)
			{
				// draw the yellow arrow
				CG_Text_Paint_Ext(x, y + heightTextOffset, scale, scale, comp->colorMain, "^3->", 0, 0, comp->styleText, FONT_TEXT);
				x += spacing * 2;
				// draw latched class icon in fireteam overlay
				trap_R_SetColor(iconColor);
				CG_DrawPic(x, y + heightIconsOffset, iconsSize, iconsSize, cgs.media.skillPics[SkillNumForClass(ci->latchedcls)]);
				x += iconsSize;
			}
			else
			{
				x += spacing * 2 + iconsSize;
			}
		}

		x += spacing;

		// draw the mute-icon in the fireteam overlay..
		//if ( ci->muted ) {
		//	CG_DrawPic( x, y, heightTextOffset, heightTextOffset, cgs.media.muteIcon );
		//	x += 14;
		//} else if

		// draw objective icon (if they are carrying one) in fireteam overlay
		if (ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
		{
			trap_R_SetColor(iconColor);
			CG_DrawPic(x, y + heightIconsOffset, iconsSize, iconsSize, cgs.media.objectiveShader);
			x      += iconsSize;
			puwidth = iconsSize;
		}
		// or else draw the disguised icon in fireteam overlay
		else if (ci->powerups & (1 << PW_OPS_DISGUISED))
		{
			trap_R_SetColor(iconColor);
			CG_DrawPic(x, y + heightIconsOffset, iconsSize, iconsSize, ci->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
			x      += iconsSize;
			puwidth = iconsSize;
		}
		// otherwise draw rank icon in fireteam overlay
		else
		{
			//if (ci->rank > 0) CG_DrawPic( x, y, heightTextOffset, heightTextOffset, rankicons[ ci->rank ][  ci->team == TEAM_AXIS ? 1 : 0 ][0].shader );
			//x += 14;
			puwidth = 0;
		}

		x += spacing;

		// draw the player's name
		// right align?
		if (cg_fireteamNameAlign.integer > 0)
		{
			CG_Text_Paint_RightAligned_Ext(x + bestNameWidth - puwidth, y + heightTextOffset, scale, scale, nameColor, name[i], 0, 0, comp->styleText, FONT_TEXT);
		}
		else
		{
			CG_Text_Paint_Ext(x, y + heightTextOffset, scale, scale, nameColor, name[i], 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		}

		// add space
		x += spacing * 2 + bestNameWidth - puwidth;

		// draw the player's weapon icon
		curWeap = CG_FireTeamClientCurrentWeapon(ci);

		// note: WP_NONE is excluded
		if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[0])     // do not try to draw nothing
		{
			trap_R_SetColor((cg_entities[ci->clientNum].currentValid || ci->clientNum == cg.clientNum) ? iconColor : iconColorSemitransparent);  // semitransparent white for weapon that is not currently being updated
			CG_DrawPic(x, y + heightWeaponIconOffset, cg_weapons[curWeap].weaponIconScale * weaponIconSize, weaponIconSize, cg_weapons[curWeap].weaponIcon[0]);
		}
		else if (IS_VALID_WEAPON(curWeap) && cg_weapons[curWeap].weaponIcon[1])
		{
			trap_R_SetColor(iconColor);
			CG_DrawPic(x, y + heightWeaponIconOffset, cg_weapons[curWeap].weaponIconScale * weaponIconSize, weaponIconSize, cg_weapons[curWeap].weaponIcon[1]);
		}

		x += bestWeapIconWidthScale * heightText * 2 + spacing;

		// draw the player's health
		if (ci->health >= 100)
		{
			CG_Text_Paint_Ext(x, y + heightTextOffset, scale, scale, comp->colorMain, va("%i", ci->health), 0, 0, comp->styleText, FONT_TEXT);
			x += spacing * 3;
		}
		else if (ci->health >= 10)
		{
			x += spacing;
			CG_Text_Paint_Ext(x, y + heightTextOffset, scale, scale, ci->health > 80 ? comp->colorMain : textYellow, va("%i", ci->health), 0, 0, comp->styleText, FONT_TEXT);
			x += spacing * 2;
		}
		else if (ci->health > 0)
		{
			x += spacing * 2;
			CG_Text_Paint_Ext(x, y + heightTextOffset, scale, scale, textYellow, va("%i", ci->health), 0, 0, comp->styleText, FONT_TEXT);
			x += spacing;
		}
		else if (ci->health == 0)
		{
			x += spacing;
			CG_Text_Paint_Ext(x, y + heightTextOffset, scale, scale, ((cg.time % 500) > 250)  ? textWhite : textRed, "*", 0, 0, comp->styleText, FONT_TEXT);
			x += spacing;
			CG_Text_Paint_Ext(x, y + heightTextOffset, scale, scale, ((cg.time % 500) > 250)  ? textRed : textWhite, "0", 0, 0, comp->styleText, FONT_TEXT);
			x += spacing;
		}
		else
		{
			x += spacing * 2;
			CG_Text_Paint_Ext(x, y + heightTextOffset, scale, scale, textRed, "0", 0, 0, comp->styleText, FONT_TEXT);
			x += spacing;
		}

		// set hard limit on width
		x += spacing;
		if (cg_locations.integer & LOC_FTEAM)
		{
			float widthLocationLeft = w - (x - comp->location.x) - spacing;
			int   lim               = widthLocationLeft / CG_Text_Width_Ext_Float("A", scale, 0, FONT_TEXT);

			if (lim > 0)
			{
				if (comp->alignText == ITEM_ALIGN_RIGHT) // right align
				{
					CG_Text_Paint_RightAligned_Ext(x + widthLocationLeft, y + heightTextOffset, scale, scale, comp->colorMain, locStr[i], 0, lim, comp->styleText, FONT_TEXT);
				}
				else if (comp->alignText == ITEM_ALIGN_LEFT)
				{
					CG_Text_Paint_Ext(x, y + heightTextOffset, scale, scale, comp->colorMain, locStr[i], 0, lim, comp->styleText, FONT_TEXT);
				}
				else    // center
				{
					CG_Text_Paint_Centred_Ext(x + widthLocationLeft * 0.5, y + heightTextOffset, scale, scale, comp->colorMain, locStr[i], 0, lim, comp->styleText, FONT_TEXT);
				}
			}
		}
	}

	trap_R_SetColor(NULL);
}

static fireteamMemberStatusEnum_t CG_FireTeamMemberStatus(clientInfo_t *ci)
{
	if (ci->ping >= 999)
	{
		return TIMEOUT;
	}
	else if (ci->health == 0)
	{
		return WOUNDED;
	}
	else if (ci->health < 0)
	{
		return DEAD;
	}
	else
	{
		return NONE;
	}
}

static vec4_t * CG_FireTeamNameColor(fireteamMemberStatusEnum_t status)
{
	switch (status)
	{
	case TIMEOUT:
		return &colorOrange;
	case WOUNDED:
		return &colorYellow;
	case DEAD:
		return &colorRed;
	case NONE:
	default:
		return &colorWhite;
	}
}

/*
 * @brief CG_FireteamGetBoxNeedsButtons
 * @return
 * @note Unused
 *
qboolean CG_FireteamGetBoxNeedsButtons(void)
{
    if (cgs.applicationEndTime > cg.time)
    {
        if (cgs.applicationClient < 0)
        {
            return qfalse;
        }
        return qtrue;
    }

    if (cgs.invitationEndTime > cg.time)
    {
        if (cgs.invitationClient < 0)
        {
            return qfalse;
        }
        return qtrue;
    }

    if (cgs.propositionEndTime > cg.time)
    {
        if (cgs.propositionClient < 0)
        {
            return qfalse;
        }
        return qtrue;
    }

    return qfalse;
}
*/

/*
 * @brief CG_FireteamGetBoxText
 * @return
 * @note Unused
 *
const char *CG_FireteamGetBoxText(void)
{
    if (cgs.applicationEndTime > cg.time)
    {
        if (cgs.applicationClient == -1)
        {
            return "Sent";
        }

        if (cgs.applicationClient == -2)
        {
            return "Failed";
        }

        if (cgs.applicationClient == -3)
        {
            return "Accepted";
        }

        if (cgs.applicationClient == -4)
        {
            return "Sent";
        }

        if (cgs.applicationClient < 0)
        {
            return NULL;
        }

        return va("Accept application from %s?", cgs.clientinfo[cgs.applicationClient].name);
    }

    if (cgs.invitationEndTime > cg.time)
    {
        if (cgs.invitationClient == -1)
        {
            return "Sent";
        }

        if (cgs.invitationClient == -2)
        {
            return "Failed";
        }

        if (cgs.invitationClient == -3)
        {
            return "Accepted";
        }

        if (cgs.invitationClient == -4)
        {
            return "Sent";
        }

        if (cgs.invitationClient < 0)
        {
            return NULL;
        }

        return va("Accept invitiation from %s?", cgs.clientinfo[cgs.invitationClient].name);
    }

    if (cgs.propositionEndTime > cg.time)
    {
        if (cgs.propositionClient == -1)
        {
            return "Sent";
        }

        if (cgs.propositionClient == -2)
        {
            return "Failed";
        }

        if (cgs.propositionClient == -3)
        {
            return "Accepted";
        }

        if (cgs.propositionClient == -4)
        {
            return "Sent";
        }

        if (cgs.propositionClient < 0)
        {
            return NULL;
        }

        return va("Accept %s's proposition to invite %s to join your fireteam?", cgs.clientinfo[cgs.propositionClient2].name, cgs.clientinfo[cgs.propositionClient].name);
    }

    return NULL;
}
*/

/**
 * @brief CG_FireteamHasClass
 * @param[in] classnum
 * @param[in] selectedonly
 * @return
 */
qboolean CG_FireteamHasClass(int classnum, qboolean selectedonly)
{
	fireteamData_t *ft;
	int            i;

	if (!(ft = CG_IsOnFireteam(cg.clientNum)))
	{
		return qfalse;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		/*      if( i == cgs.clientinfo ) {
		            continue;
		        }*/

		if (!cgs.clientinfo[i].infoValid)
		{
			continue;
		}

		if (ft != CG_IsOnFireteam(i))
		{
			continue;
		}

		if (cgs.clientinfo[i].cls != classnum)
		{
			continue;
		}

		if (selectedonly && !cgs.clientinfo[i].selected)
		{
			continue;
		}

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_BuildSelectedFirteamString
 * @return
 */
const char *CG_BuildSelectedFirteamString(void)
{
	char         buffer[256];
	clientInfo_t *ci;
	int          cnt = 0;
	int          i;

	*buffer = '\0';
	for (i = 0; i < MAX_FIRETEAM_MEMBERS; i++)
	{
		ci = CG_SortedFireTeamPlayerForPosition(i);
		if (!ci)
		{
			break;
		}

		if (!ci->selected)
		{
			continue;
		}

		cnt++;

		Q_strcat(buffer, sizeof(buffer), va("%i ", ci->clientNum));
	}

	if (cnt == 0)
	{
		return "0";
	}

	if (!cgs.clientinfo[cg.clientNum].selected)
	{
		Q_strcat(buffer, sizeof(buffer), va("%i ", cg.clientNum));
		cnt++;
	}

	return va("%i %s", cnt, buffer);
}
