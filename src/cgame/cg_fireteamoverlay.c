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

// HUD editor flags
// TODO: move these elsewhere (and probably rename)
#define FT_LATCHED_CLASS        (BIT(0))
#define FT_NO_HEADER            (BIT(1))
#define FT_COLORLESS_NAME       (BIT(2))
#define FT_STATUS_COLOR_NAME    (BIT(3))
#define FT_STATUS_COLOR_ROW     (BIT(4))
#define FT_SPAWN_POINT          (BIT(5))
#define FT_SPAWN_POINT_LOC      (BIT(6))
#define FT_SPAWN_POINT_MINOR    (BIT(7))

typedef struct fireteamOverlay_s
{
	float x;
	float y;
	float w;
	float h;

	float textScale;
	float textScaleMinorSpawn;

	float spacerInner; // spacing between elements on a row
	float spacerOuter; // spacer at the outer edges of a row

	float textHeight;
	float iconSize;
	float weaponIconSize;
	float powerupWidth;

	float textHeightOffset;
	float iconHeightOffset;
	float weaponIconHeightOffset;

	float classIconWidth;
	float healthWidth;

	float bestNameWidth; // includes powerup icon width if present
	float bestLocWidth;
	float bestWeaponIconWidthScale;
	float bestSpawnWidth;

	float maxLocWidth;
	float maxSpawnWidth;

	int currentWeapon;

	clientInfo_t *ci;
	fireteamData_t *ftData;

	char locStr[MAX_FIRETEAM_MEMBERS][MAX_LOC_LEN];
	char spawnPtStr[MAX_FIRETEAM_MEMBERS][MAX_LOC_LEN];
	char name[MAX_FIRETEAM_MEMBERS][MAX_NAME_LENGTH];

	vec4_t selectedMemberColor;
	vec4_t iconColor;
	vec4_t iconColorAlt;    // for weapon icon that isn't updated (teammate out of PVS)
	vec4_t textWhite;       // normal text
	vec4_t textYellow;      // for health drawing
	vec4_t textRed;         // for health drawing
	vec4_t textOrange;      // for connection issues
	vec4_t nameColor;
} fireteamOverlay_t;

static fireteamOverlay_t ftOverlay;

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

static void CG_FTOverlay_NoiseGen(fireteamOverlay_t *fto, hudComponent_t *comp)
{
	const char *info       = "Please join a real Fireteam!";
	float      titleHeight = 0;

	// draw the box
	fto->w = comp->location.w;
	fto->h = comp->location.h;
	CG_FillRect(fto->x, fto->y, fto->w, fto->h, comp->colorBackground);
	CG_DrawRect_FixedBorder(fto->x, fto->y, fto->w, fto->h, 1, comp->colorBorder);
	CG_FillRect(fto->x + 1, fto->y + 1, fto->w - 2, fto->h - 2, comp->colorSecondary);

	// draw a text info on it
	fto->textScale = CG_ComputeScale(comp);
	titleHeight    = CG_Text_Height_Ext(info, fto->textScale, 0, FONT_HEADER);
	CG_Text_Paint_Ext(fto->x + 4, comp->location.y + ((titleHeight + fto->h) * 0.5f), fto->textScale, fto->textScale,
	                  comp->colorMain, info, 0, 0, comp->styleText, FONT_TEXT);
}

static void CG_FTOverlay_SetColors(fireteamOverlay_t *fto, const float alpha)
{
	Vector4Set(fto->selectedMemberColor, 0.5f, 0.5f, 0.2f, 0.3f);
	Vector4Set(fto->iconColor, 1.0f, 1.0f, 1.0f, 1.0f);
	Vector4Set(fto->iconColorAlt, 1.0f, 1.0f, 1.0f, 0.5f);
	Vector4Set(fto->textWhite, 1.0f, 1.0f, 1.0f, 1.0f);
	Vector4Set(fto->textYellow, 1.0f, 1.0f, 0.0f, 1.0f);
	Vector4Set(fto->textRed, 1.0f, 0.0f, 0.0f, 1.0f);
	Vector4Set(fto->textOrange, 1.0f, 0.6f, 0.0f, 1.0f);
	Vector4Set(fto->nameColor, 1.0f, 1.0f, 1.0f, 1.0f);

	fto->selectedMemberColor[3] *= alpha;
	fto->iconColor[3]           *= alpha;
	fto->iconColorAlt[3]        *= alpha;
	fto->textWhite[3]           *= alpha;
	fto->textYellow[3]          *= alpha;
	fto->textRed[3]             *= alpha;
	fto->textOrange[3]          *= alpha;
	fto->nameColor[3]           *= alpha;
}

static void CG_FTOverlay_StorePlayerName(fireteamOverlay_t *fto, const int row, const hudComponent_t *comp)
{
	if (comp->style & FT_COLORLESS_NAME || (comp->style & FT_STATUS_COLOR_NAME && (fto->ci->health <= 0 || fto->ci->ping >= 999)))
	{
		char escapedName[MAX_NAME_LENGTH];

		// use NULL color here rather than 'fto->ci->cleanname' directly,
		// to make sure that a name with visible carets in it doesn't get colorized when drawing,
		// and works as expected with colorless names/status colored names
		Q_ColorizeString('*', fto->ci->cleanname, escapedName, sizeof(escapedName));
		Q_strncpyz(fto->name[row], escapedName, sizeof(fto->name[row]));
	}
	else
	{
		Q_strncpyz(fto->name[row], fto->ci->name, sizeof(fto->name[row]));
	}

	// truncate name if max chars is set
	if (cg_fireteamNameMaxChars.integer > 0)
	{
		const int nameMaxLen = Com_Clamp(0, MAX_NAME_LENGTH - 1, cg_fireteamNameMaxChars.integer);
		Q_TruncateStr(fto->name[row], nameMaxLen);
	}
}

static void CG_FTOverlay_StoreLocationString(fireteamOverlay_t *fto, const int row)
{
	const char *locStr;

	if (!(cg_locations.integer & LOC_FTEAM))
	{
		return;
	}

	locStr = CG_BuildLocationString(fto->ci->clientNum, fto->ci->location, LOC_FTEAM);

	// CG_BuildLocationString should always return a valid string!
	etl_assert(locStr != NULL);

	// store the location name so we don't need to rebuild it later for drawing
	Q_strncpyz(fto->locStr[row], locStr, sizeof(fto->locStr[row]));
}

static void CG_FTOverlay_StoreSpawnpointString(fireteamOverlay_t *fto, const int row, const hudComponent_t *comp)
{
	if (!(comp->style & FT_SPAWN_POINT_LOC))
	{
		return;
	}

	Q_strncpyz(fto->spawnPtStr[row],
	           Q_CleanStr(CG_GetLocationMsg(fto->ci->clientNum, cgs.majorSpawnpointEnt[fto->ci->spawnpt - 1].origin)),
	           sizeof(fto->spawnPtStr[row]));

}

#define HEALTH_WIDTH (CG_Text_Width_Ext_Float("999", fto->textScale, 0, FONT_TEXT))
#define CLASS_ICON_ARROW_WIDTH (CG_Text_Width_Ext_Float("->", fto->textScale, 0, FONT_TEXT))
// slightly wider than the icon size, to leave a bit of margin between the icon and name
#define POWERUP_WIDTH (fto->iconSize * 1.25f)
#define MAJOR_SPAWN_NUMBER_WIDTH_SCALE ((comp->style & FT_SPAWN_POINT_MINOR) ? 1.25f : 1.0f)

static float CG_FTOverlay_ClassIconWidth(const fireteamOverlay_t *fto, const hudComponent_t *comp)
{
	if (comp->style & FT_LATCHED_CLASS)
	{
		return (fto->iconSize * 2) + CLASS_ICON_ARROW_WIDTH;
	}

	return fto->iconSize;
}

static float CG_FTOverlay_NameWidth(fireteamOverlay_t *fto, const int row)
{
	float nameWidth = 0;
	int   limit     = 0;

	// names should be stored before calling this!
	etl_assert(fto->name[row][0] != '\0');

	if (cg_fireteamNameMaxChars.integer > 0)
	{
		limit = Com_Clamp(0, MAX_NAME_LENGTH, cg_fireteamNameMaxChars.integer);

		// if alignment is requested, keep a static width
		if (cg_fireteamNameAlign.integer)
		{
			// NOTE: use clean name for counting the padding here - we're counting visible padding!
			nameWidth = CG_Text_Width_Ext_Float(va("%*s", limit, &fto->ci->cleanname[row]), fto->textScale, limit, FONT_TEXT);
		}
		else
		{
			nameWidth = CG_Text_Width_Ext_Float(fto->name[row], fto->textScale, limit, FONT_TEXT);
		}
	}
	else
	{
		nameWidth = CG_Text_Width_Ext_Float(fto->name[row], fto->textScale, limit, FONT_TEXT);
	}

	// reserve more space if the player is holding an objective or is disguised,
	// as that gets placed in front of the player's name, shifting it to right
	if (fto->ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG) | (1 << PW_OPS_DISGUISED)))
	{
		nameWidth += POWERUP_WIDTH;
	}

	return nameWidth;
}

static float CG_FTOverlay_WeaponIconWidthScale(fireteamOverlay_t *fto)
{
	fto->currentWeapon = CG_FireTeamClientCurrentWeapon(fto->ci);

	// ensure we actually have a valid weapon (should always be the case)
	if (IS_VALID_WEAPON(fto->currentWeapon) &&
	    (cg_weapons[fto->currentWeapon].weaponIcon[0] || cg_weapons[fto->currentWeapon].weaponIcon[1]))
	{
		return cg_weapons[fto->currentWeapon].weaponIconScale;
	}

	return 1.0f;
}


// pretty useless, but it's clearer intention to call a function named this,
// rather than to just write out the value
static ID_INLINE float CG_FTOverlay_HealthWidth(const fireteamOverlay_t *fto)
{
	return HEALTH_WIDTH;
}

static float CG_FTOverlay_LocationWidth(const fireteamOverlay_t *fto, const int row)
{
	float locWidth = 0.0f;
	int   limit    = 0;

	if (!(cg_locations.integer & LOC_FTEAM))
	{
		return locWidth;
	}

	// location names should be stored before calling this!
	etl_assert(fto->locStr[row][0] != '\0');

	if (cg_locationMaxChars.integer > 0)
	{
		// we need to use clean string here since we're counting printf padding
		char cleanLocStr[MAX_LOC_LEN];
		Q_strncpyz(cleanLocStr, fto->locStr[row], sizeof(cleanLocStr));
		Q_CleanStr(cleanLocStr);

		limit    = Com_Clamp(0, MAX_LOC_LEN, cg_locationMaxChars.integer);
		locWidth = CG_Text_Width_Ext_Float(va("%*s", limit, cleanLocStr), fto->textScale, limit, FONT_TEXT);
	}
	else
	{
		locWidth = CG_Text_Width_Ext_Float(fto->locStr[row], fto->textScale, limit, FONT_TEXT);
	}

	return locWidth;
}

static float CG_FTOverlay_SpawnpointWidth(const fireteamOverlay_t *fto, const int row, const hudComponent_t *comp)
{
	float spawnWidth = 0;
	int   limit      = 0;

	if (!(comp->style & FT_SPAWN_POINT) && !(comp->style & FT_SPAWN_POINT_LOC))
	{
		return spawnWidth;
	}

	// text location takes precedence over numbers, if both are set
	if (comp->style & FT_SPAWN_POINT_LOC)
	{
		// spawnpoint names should be stored before calling this!
		etl_assert(fto->spawnPtStr[row][0] != '\0');

		if (cg_locationMaxChars.integer > 0)
		{
			limit = Com_Clamp(0, MAX_LOC_LEN, cg_locationMaxChars.integer);
			// spawnpoint strings are cleaned from color codes, we can use that to count padding
			spawnWidth = CG_Text_Width_Ext_Float(va("%*s", limit, fto->spawnPtStr[row]), fto->textScale, limit, FONT_TEXT);
		}
		else
		{
			spawnWidth = CG_Text_Width_Ext_Float(fto->spawnPtStr[row], fto->textScale, limit, FONT_TEXT);
		}
	}
	else if (comp->style & FT_SPAWN_POINT)
	{
		spawnWidth = CG_Text_Width_Ext_Float(va("%i", fto->ci->spawnpt), fto->textScale, 0, FONT_TEXT) * MAJOR_SPAWN_NUMBER_WIDTH_SCALE;

		if (comp->style & FT_SPAWN_POINT_MINOR && cg.hasMinorSpawnPoints && fto->ci->mspawnpt > 0)
		{
			spawnWidth += CG_Text_Width_Ext_Float(va("%i", fto->ci->mspawnpt), fto->textScaleMinorSpawn, 0, FONT_TEXT);
		}
	}

	return spawnWidth;
}

static void CG_FTOverlay_DrawHeader(fireteamOverlay_t *fto, hudComponent_t *comp)
{
	const char *header     = CG_TranslateString(va("%sFireteam", fto->ftData->priv ? "Private " : ""));
	float      titleHeight = 0;
	char       buf[64];

	CG_FillRect(fto->x + 1, fto->y + 1, fto->w - 2, fto->h - 1, comp->colorSecondary);

	Com_sprintf(buf, sizeof(buf), "%s: %s", header, cgs.clientinfo[cg.clientNum].team == TEAM_AXIS
	                                                                ? bg_fireteamNamesAxis[fto->ftData->ident]
	                                                                : bg_fireteamNamesAllies[fto->ftData->ident]);
	Q_strupr(buf);
	titleHeight = CG_Text_Height_Ext_Float(buf, fto->textScale, 0, FONT_HEADER);

	CG_Text_Paint_Ext(fto->x + fto->spacerOuter, comp->location.y + ((titleHeight + fto->h) * 0.5f), fto->textScale, fto->textScale,
	                  comp->colorMain, buf, 0, 0, comp->styleText, FONT_HEADER);

	fto->y += fto->h;
}

static void CG_FTOverlay_DrawClassIcon(fireteamOverlay_t *fto, hudComponent_t *comp)
{
	trap_R_SetColor(fto->iconColor);
	CG_DrawPic(fto->x, fto->y + fto->iconHeightOffset, fto->iconSize, fto->iconSize,
	           cgs.media.skillPics[SkillNumForClass(fto->ci->cls)]);

	fto->x += fto->iconSize;

	if (comp->style & FT_LATCHED_CLASS)
	{
		const float arrowWidth = CLASS_ICON_ARROW_WIDTH;

		if (fto->ci->cls != fto->ci->latchedcls)
		{
			CG_Text_Paint_Ext(fto->x, fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
			                  comp->colorMain, "^3->", 0, 0, comp->styleText, FONT_TEXT);

			fto->x += arrowWidth;

			CG_DrawPic(fto->x, fto->y + fto->iconHeightOffset, fto->iconSize, fto->iconSize,
			           cgs.media.skillPics[SkillNumForClass(fto->ci->latchedcls)]);

			fto->x += fto->iconSize;
		}
		else
		{
			fto->x += fto->iconSize + arrowWidth;
		}
	}

	fto->x += fto->spacerInner;
}

static void CG_FTOverlay_DrawPowerupIcon(fireteamOverlay_t *fto)
{
	if (fto->ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
	{
		trap_R_SetColor(fto->iconColor);
		CG_DrawPic(fto->x, fto->y + fto->iconHeightOffset, fto->iconSize, fto->iconSize, cgs.media.objectiveShader);

		fto->powerupWidth = POWERUP_WIDTH;
		fto->x           += POWERUP_WIDTH;
	}
	else if (fto->ci->powerups & (1 << PW_OPS_DISGUISED))
	{
		trap_R_SetColor(fto->iconColor);
		CG_DrawPic(fto->x, fto->y + fto->iconHeightOffset, fto->iconSize, fto->iconSize,
		           fto->ci->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);

		fto->powerupWidth = POWERUP_WIDTH;
		fto->x           += POWERUP_WIDTH;
	}
	else
	{
		fto->powerupWidth = 0;
	}
}

static void CG_FTOverlay_DrawName(fireteamOverlay_t *fto, const int row, const hudComponent_t *comp)
{
	// first draw the icon for any powerups, as that influences the position of the name
	CG_FTOverlay_DrawPowerupIcon(fto);

	// right aligned?
	if (cg_fireteamNameAlign.integer > 0)
	{
		CG_Text_Paint_RightAligned_Ext(fto->x + fto->bestNameWidth - fto->powerupWidth, fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
		                               fto->nameColor, fto->name[row], 0, 0, comp->styleText, FONT_TEXT);
	}
	else
	{
		CG_Text_Paint_Ext(fto->x, fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
		                  fto->nameColor, fto->name[row], 0, 0, comp->styleText, FONT_TEXT);
	}

	// fto->bestNameWidth already accounts for potential obj/disguise icon for max width calculations,
	// so subtract it here - otherwise we shift the position to the right too much
	fto->x += fto->bestNameWidth - fto->powerupWidth + fto->spacerInner;
}

static void CG_FTOverlay_DrawWeaponIcon(fireteamOverlay_t *fto)
{
	fto->currentWeapon = CG_FireTeamClientCurrentWeapon(fto->ci);

	if (IS_VALID_WEAPON(fto->currentWeapon))
	{
		const qhandle_t shader = cg_weapons[fto->currentWeapon].weaponIcon[0]
		    ? cg_weapons[fto->currentWeapon].weaponIcon[0]
		    : cg_weapons[fto->currentWeapon].weaponIcon[1];

		if (shader)
		{
			trap_R_SetColor((cg_entities[fto->ci->clientNum].currentValid || fto->ci->clientNum == cg.clientNum)
			       ? fto->iconColor
			       : fto->iconColorAlt);
			CG_DrawPic(fto->x, fto->y + fto->weaponIconHeightOffset,
			           cg_weapons[fto->currentWeapon].weaponIconScale * fto->weaponIconSize, fto->weaponIconSize, shader);
		}
	}

	fto->x += (fto->bestWeaponIconWidthScale * fto->weaponIconSize) + fto->spacerInner;
}

#define FT_HEALTH_NORMAL 80
#define FT_HEALTH_YELLOW 10

static void CG_FTOverlay_DrawHealth(fireteamOverlay_t *fto, const hudComponent_t *comp)
{
	const int  health      = fto->ci->health;
	const int  healthWidth = HEALTH_WIDTH;
	vec4_t     color;
	const char *text;

	if (health > FT_HEALTH_NORMAL)
	{
		Vector4Copy(comp->colorMain, color);
		text = va("%i", health);
	}
	else if (health >= FT_HEALTH_YELLOW)
	{
		Vector4Copy(fto->textYellow, color);
		text = va("%i", health);
	}
	else if (health > 0)
	{
		Vector4Copy(fto->textRed, color);
		text = va("%i", health);
	}
	// wounded
	else if (health == 0)
	{
		Vector4Copy(fto->textWhite, color);
		text = va("%s*%s%i",
		          ((cg.time % 500) > 250) ? "^7" : "^1",
		          ((cg.time % 500) > 250) ? "^1" : "^7",
		          health);
	}
	// limbo (-1 health)
	else
	{
		Vector4Copy(fto->textRed, color);
		text = "0";
	}

	CG_Text_Paint_RightAligned_Ext(fto->x + healthWidth, fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
	                               color, text, 0, 0, comp->styleText, FONT_TEXT);

	// always use static size, regardless of actual text being drawn
	fto->x += healthWidth + fto->spacerInner;
}

void CG_FTOverlay_DrawLocation(fireteamOverlay_t *fto, const int row, hudComponent_t *comp)
{
	int limit;

	if (!(cg_locations.integer & LOC_FTEAM))
	{
		return;
	}

	limit = fto->bestLocWidth > fto->maxLocWidth
	    ? CG_MaxCharsForWidth(fto->locStr[row], fto->textScale, FONT_TEXT, fto->maxLocWidth)
	    : 0;

	if (comp->alignText == ITEM_ALIGN_RIGHT)
	{
		CG_Text_Paint_RightAligned_Ext(fto->x + fto->maxLocWidth, fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
		                               comp->colorMain, fto->locStr[row], 0, limit, comp->styleText, FONT_TEXT);
	}
	else if (comp->alignText == ITEM_ALIGN_LEFT)
	{
		CG_Text_Paint_Ext(fto->x, fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
		                  comp->colorMain, fto->locStr[row], 0, limit, comp->styleText, FONT_TEXT);
	}
	else
	{
		CG_Text_Paint_Centred_Ext(fto->x + (fto->maxLocWidth * 0.5f), fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
		                          comp->colorMain, fto->locStr[row], 0, limit, comp->styleText, FONT_TEXT);
	}

	fto->x += fto->maxLocWidth + fto->spacerInner;
}

static void CG_FTOverlay_DrawSpawnpoint(fireteamOverlay_t *fto, const int row, const hudComponent_t *comp)
{
	const float timeDiff = cg.time - fto->ci->spawnChangedTime;
	vec4_t      spawnPtColor;

	if (!(comp->style & FT_SPAWN_POINT) && !(comp->style & FT_SPAWN_POINT_LOC))
	{
		return;
	}

	Vector4Copy(fto->textWhite, spawnPtColor);

	// lerp color for 1.5s after spawn is changed
	if (timeDiff < 1500)
	{
		const float t = timeDiff / 1500.0f;
		LerpColor(fto->textOrange, fto->textWhite, spawnPtColor, t);
	}

	if (comp->style & FT_SPAWN_POINT_LOC)
	{
		// 'fto->maxLocWidth' isn't set if 'cg_locations' does not enable fireteam locations
		const int limit = (fto->maxSpawnWidth == fto->maxLocWidth || fto->maxSpawnWidth < fto->bestSpawnWidth)
		    ? CG_MaxCharsForWidth(fto->spawnPtStr[row], fto->textScale, FONT_TEXT, fto->maxSpawnWidth)
		    : 0;

		if (comp->alignText == ITEM_ALIGN_RIGHT)
		{
			CG_Text_Paint_RightAligned_Ext(fto->x + fto->maxSpawnWidth, fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
			                               spawnPtColor, fto->spawnPtStr[row], 0, limit, comp->styleText, FONT_TEXT);
		}
		else if (comp->alignText == ITEM_ALIGN_LEFT)
		{
			CG_Text_Paint_Ext(fto->x, fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
			                  spawnPtColor, fto->spawnPtStr[row], 0, limit, comp->styleText, FONT_TEXT);
		}
		else
		{
			CG_Text_Paint_Centred_Ext(fto->x + (fto->maxSpawnWidth * 0.5f), fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
			                          spawnPtColor, fto->spawnPtStr[row], 0, limit, comp->styleText, FONT_TEXT);
		}

		fto->x += fto->maxSpawnWidth;
	}
	else
	{
		const char *spawnPtStr = va("%i", fto->ci->spawnpt);
		CG_Text_Paint_Ext(fto->x, fto->y + fto->textHeightOffset, fto->textScale, fto->textScale,
		                  spawnPtColor, spawnPtStr, 0, 0, comp->styleText, FONT_TEXT);

		fto->x += CG_Text_Width_Ext_Float(spawnPtStr, fto->textScale, 0, FONT_TEXT) * MAJOR_SPAWN_NUMBER_WIDTH_SCALE;

		if (comp->style & FT_SPAWN_POINT_MINOR && cg.hasMinorSpawnPoints && fto->ci->mspawnpt > 0)
		{
			const char *minorSpawnPtStr = va("%i", fto->ci->mspawnpt);
			CG_Text_Paint_Ext(fto->x, fto->y + fto->textHeightOffset, fto->textScaleMinorSpawn, fto->textScaleMinorSpawn,
			                  spawnPtColor, minorSpawnPtStr, 0, 0, comp->styleText, FONT_TEXT);

			fto->x += CG_Text_Width_Ext_Float(minorSpawnPtStr, fto->textScaleMinorSpawn, 0, FONT_TEXT);
		}
	}

	fto->x += fto->spacerInner;
}

static void CG_FTOverlay_ComputeLocLimits(fireteamOverlay_t *fto, float fixedElementsWidth, const hudComponent_t *comp)
{
	int remainingWidth;
	int maxWidth;

	// 'fto->w' should already be capped to comp max width at this point!
	etl_assert(fto->w == comp->location.w);

	maxWidth       = fto->w - (fto->spacerOuter * 2);
	remainingWidth = maxWidth - fixedElementsWidth;

	// if we're drawing locations, and using spawn point location strings,
	// divide the remaining space equally between them
	if (cg_locations.integer & LOC_FTEAM && comp->style & FT_SPAWN_POINT_LOC)
	{
		fto->maxLocWidth   = (remainingWidth - fto->spacerInner) / 2;
		fto->maxSpawnWidth = fto->maxLocWidth;
	}
	// locations are off, but spawn point is still drawn with a location string
	else if (comp->style & FT_SPAWN_POINT_LOC)
	{
		fto->maxSpawnWidth = remainingWidth;
	}
	// nothing else requires space, numbered spawn point display is reserved as fixed width
	else
	{
		fto->maxLocWidth   = remainingWidth;
		fto->maxSpawnWidth = fto->bestSpawnWidth;
	}
}

void CG_DrawFireTeamOverlay(hudComponent_t *comp)
{
	fireteamOverlay_t *fto                  = &ftOverlay;
	float             fixedElementsWidth    = 0;
	float             flexibleElementsWidth = 0;
	float             nameWidth, weaponIconWidthScale, locationWidth, spawnWidth;
	float             baseX;
	int               i;
	qboolean          drawHeader;

	Com_Memset(fto, 0, sizeof(fireteamOverlay_t));

	fto->x = comp->location.x;
	fto->y = comp->location.y;

	fto->ftData = CG_IsOnFireteam(cg.clientNum);

	// early exit
	if (cgs.clientinfo[cg.clientNum].shoutcaster || !fto->ftData)
	{
		// XXX : TODO : we currently don't generate any noise for the
		// fireteamoverlay, as it's pretty involved - so we do the minimum here,
		// which is to just draw a box filling the component size, which also
		// mentions this fact
		if (cg.generatingNoiseHud)
		{
			CG_FTOverlay_NoiseGen(fto, comp);
		}

		return;
	}

	fto->textScale           = CG_ComputeScale(comp);
	fto->textScaleMinorSpawn = fto->textScale / 1.5f;

	fto->h = comp->location.h / (MAX_FIRETEAM_MEMBERS + 1);

	fto->textHeight     = CG_Text_Height_Ext_Float("A", fto->textScale, 0, FONT_TEXT);
	fto->spacerOuter    = CG_Text_Width_Ext_Float("A", fto->textScale, 0, FONT_TEXT);
	fto->spacerInner    = fto->textHeight * 1.5f;
	fto->iconSize       = fto->textHeight * 2.0f;
	fto->weaponIconSize = fto->textHeight * 2.0f;

	fto->textHeightOffset       = (fto->h + fto->textHeight) * 0.5f;
	fto->iconHeightOffset       = (fto->h - fto->iconSize) * 0.5f;
	fto->weaponIconHeightOffset = (fto->h - (fto->textHeight * 2)) * 0.5f;

	// first, get the width for all the elements
	// the width does *NOT* contain the spacer required between each element,
	// as some of them are not necessarily drawn (location/spawnpoint)
	// we also store all the names, locations and spawn points (if using string display) here
	for (i = 0; i < MAX_FIRETEAM_MEMBERS; i++)
	{
		fto->ci = CG_SortedFireTeamPlayerForPosition(i);

		if (!fto->ci)
		{
			break;
		}

		CG_FTOverlay_StorePlayerName(fto, i, comp);
		CG_FTOverlay_StoreLocationString(fto, i);
		CG_FTOverlay_StoreSpawnpointString(fto, i, comp);

		nameWidth            = CG_FTOverlay_NameWidth(fto, i);
		weaponIconWidthScale = CG_FTOverlay_WeaponIconWidthScale(fto);
		locationWidth        = CG_FTOverlay_LocationWidth(fto, i);
		spawnWidth           = CG_FTOverlay_SpawnpointWidth(fto, i, comp);

		if (nameWidth > fto->bestNameWidth)
		{
			fto->bestNameWidth = nameWidth;
		}

		if (weaponIconWidthScale > fto->bestWeaponIconWidthScale)
		{
			fto->bestWeaponIconWidthScale = weaponIconWidthScale;
		}

		if (locationWidth > fto->bestLocWidth)
		{
			fto->bestLocWidth = locationWidth;
		}

		if (spawnWidth > fto->bestSpawnWidth)
		{
			fto->bestSpawnWidth = spawnWidth;
		}
	}

	// these are always static width and just depend on fireteam overlay settings,
	// no need to compute them inside a loop for each member
	fto->classIconWidth = CG_FTOverlay_ClassIconWidth(fto, comp);
	fto->healthWidth    = CG_FTOverlay_HealthWidth(fto);

	fixedElementsWidth = fto->classIconWidth + fto->spacerInner
	                     + fto->bestNameWidth + fto->spacerInner
	                     + (fto->weaponIconSize * fto->bestWeaponIconWidthScale) + fto->spacerInner
	                     + fto->healthWidth;

	// Figure out the remaining space. This is a bit complicated with locations
	// and spawnpoint being so complicated with their possible combinations,
	// but the general idea is this:
	//
	// * if spawn point is drawn with numbers, it's *always* fixed width
	// * if 'cg_locations' enables fireteam locations:
	//   * location string is "flexible" width and can be truncated
	//   * if spawn point is drawn with a location string, it's flexible width
	//     and will be truncated equally with location string to fit
	// * if 'cg_locations' is *NOT* enabled for fireteam:
	//   * if spawnpoint is drawn with a location string, it's flexible width
	//
	// There is one unaccounted issue with this, which is that the fireteam
	// requires a minimum width to display all the fixed width information.
	// This is not currently handled, and it's up to the user to make sure that
	// their fireteam comp is wide enough to have everything visible.
	// The game doesn't crash or anything if the comp is too narrow,
	// all the alignments simply break. This really isn't a high priority issue
	// though, as players will likely want to make the fireteam wide enough
	// anyway to display enough relevant information for any given element.

	// if locations are enabled, add the location string(s) to flexible elements
	if (cg_locations.integer & LOC_FTEAM || comp->style & FT_SPAWN_POINT_LOC)
	{
		fixedElementsWidth += fto->spacerInner;

		// both location string and spawn point string are flexible
		if (comp->style & FT_SPAWN_POINT_LOC)
		{
			// don't add an extra spacer if we're *NOT* drawing locations
			flexibleElementsWidth += (cg_locations.integer & LOC_FTEAM)
			    ? fto->bestLocWidth + fto->spacerInner + fto->bestSpawnWidth
			    : fto->bestSpawnWidth;
		}
		// only location string is flexible, spawnpoint is drawn with a number
		else if (comp->style & FT_SPAWN_POINT)
		{
			flexibleElementsWidth = fto->bestLocWidth;
			fixedElementsWidth   += fto->spacerInner + fto->bestSpawnWidth;
		}
		// spawnpoint drawing is off, only location is flexible
		else
		{
			flexibleElementsWidth += fto->bestLocWidth;
		}
	}
	// location string(s) are off, spawnpoint is drawn with a number
	else if (comp->style & FT_SPAWN_POINT)
	{
		fixedElementsWidth += fto->spacerInner + fto->bestSpawnWidth;
	}

	// if we're overflowing the comp width after adding location,
	// figure out how much space we have left for location(s)
	if (fixedElementsWidth + flexibleElementsWidth > (comp->location.w - (fto->spacerOuter * 2)))
	{
		fto->w = comp->location.w;
		CG_FTOverlay_ComputeLocLimits(fto, fixedElementsWidth, comp);
	}
	else
	{
		fto->w             = fixedElementsWidth + flexibleElementsWidth + (fto->spacerOuter * 2);
		fto->maxLocWidth   = fto->bestLocWidth;
		fto->maxSpawnWidth = fto->bestSpawnWidth;
	}

	CG_FTOverlay_SetColors(fto, comp->colorMain[3]);

	drawHeader = !(comp->style & FT_NO_HEADER);

	if (comp->showBackGround)
	{
		CG_FillRect(fto->x, fto->y, fto->w, fto->h * (i + (drawHeader ? 1 : 0)), comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(fto->x, fto->y, fto->w, fto->h * (i + (drawHeader ? 1 : 0)), 1, comp->colorBorder);
	}

	if (drawHeader)
	{
		CG_FTOverlay_DrawHeader(fto, comp);
	}

	baseX = fto->x;

	for (i = 0; i < MAX_FIRETEAM_MEMBERS; i++)
	{
		fto->x  = baseX;
		fto->ci = CG_SortedFireTeamPlayerForPosition(i);

		if (!fto->ci)
		{
			break;
		}

		if (comp->style & FT_STATUS_COLOR_NAME || comp->style & FT_STATUS_COLOR_ROW)
		{
			fireteamMemberStatusEnum_t status = CG_FireTeamMemberStatus(fto->ci);
			VectorCopy(*CG_FireTeamNameColor(status), fto->nameColor);

			if (comp->style & FT_STATUS_COLOR_ROW && status != NONE)
			{
				vec4_t rowColor;
				Vector4Copy(fto->nameColor, rowColor);
				rowColor[3] = comp->colorBackground[3];

				CG_FillRect(fto->x, fto->y, fto->w, fto->h, rowColor);
			}
		}
		else
		{
			Vector4Copy(fto->textWhite, fto->nameColor);
		}

		if (fto->ci->selected)
		{
			CG_FillRect(fto->x, fto->y, fto->w, fto->h, fto->selectedMemberColor);
		}

		fto->x += fto->spacerOuter;

		// All of the element draw functions adjust the x position such that
		// once they have finished executing, the position is shifted
		// to the right, to the correct position for the next element.
		// Any potential new elements should adhere to this,
		// to keep this functioning as expected.
		// This means that it's possible to reorder the elements in any way desired,
		// without messing up layout - the elements are simply drawn
		// left to right, in the order that their draw functions are called.

		CG_FTOverlay_DrawClassIcon(fto, comp);
		CG_FTOverlay_DrawName(fto, i, comp);
		CG_FTOverlay_DrawWeaponIcon(fto);
		CG_FTOverlay_DrawHealth(fto, comp);
		CG_FTOverlay_DrawLocation(fto, i, comp);
		CG_FTOverlay_DrawSpawnpoint(fto, i, comp);

		fto->y += fto->h;
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
