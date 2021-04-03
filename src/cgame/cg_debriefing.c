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
 * @file cg_debriefing.c
 */

#include "cg_local.h"

team_t CG_Debriefing_FindWinningTeam(void);
team_t CG_Debriefing_FindOveralWinningTeam(void);
team_t CG_Debriefing_FindWinningTeamForPos(int pos);

int QDECL CG_SortPlayersByXP(const void *a, const void *b);

static panel_button_text_t debriefPlayerHeadingSmallerFont =
{
	0.2f,                  0.2f,
	{ 0.6f,                0.6f,0.6f,    1.f },
	0,                     0,
	&cgs.media.limboFont2,
};

#define DB_RANK_X      213 + 4
#define DB_NAME_X      DB_RANK_X + 28
#define DB_TIME_X      DB_NAME_X + 150
#define DB_KILLS_X     DB_TIME_X + 30
#define DB_DEATHS_X    DB_KILLS_X + 24
#define DB_GIBS_X      DB_DEATHS_X + 24
#define DB_SELFKILLS_X DB_GIBS_X + 24
#define DB_TEAMKILLS_X DB_SELFKILLS_X + 24
#define DB_TEAMGIBS_X  DB_TEAMKILLS_X + 24
#define DB_XP_X        DB_TEAMGIBS_X + 24
#define DH_HEADING_Y   60

static panel_button_t debriefPlayerWeaponStatsHeader =
{
	NULL,
	"Weapon Stats",
	{ 18,                      248,0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&debriefPlayerHeadingSmallerFont,// font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefPlayerWeaponStatsNameHeader =
{
	NULL,
	"Name",
	{ 18,                      262,0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&debriefPlayerHeadingSmallerFont,// font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefPlayerWeaponStatsShotsHeader =
{
	NULL,
	"Shots",
	{ 78,                      262,0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&debriefPlayerHeadingSmallerFont,// font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefPlayerWeaponStatsHitsHeader =
{
	NULL,
	"Hits",
	{ 118,                     262, 0, 0 },
	{ 0,                       0,   0, 0, 0, 0, 0, 0},
	&debriefPlayerHeadingSmallerFont,// font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefPlayerWeaponStatsKillsHeader =
{
	NULL,
	"Kills",
	{ 148,                     262,0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&debriefPlayerHeadingSmallerFont,// font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefPlayerWeaponStatsList =
{
	NULL,
	NULL,
	{ 18,                               262,  168, 80 },
	{ 0,                                0,    0,   0, 0, 0, 0, 0},
	&debriefPlayerHeadingSmallerFont,   // font
	NULL,                               // keyDown
	NULL,                               // keyUp
	CG_DebriefingPlayerWeaponStats_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerWeaponStatsListScroll =
{
	NULL,
	NULL,
	{ 18 + 168,                  256,        16, 96 },
	{ 1,                         0,          0,  0, 0, 0, 0, 0},
	NULL,                        // font
	CG_Debriefing_Scrollbar_KeyDown,// keyDown
	CG_Debriefing_Scrollbar_KeyUp,// keyUp
	CG_Debriefing_Scrollbar_Draw,
	NULL,
	0
};

static panel_button_t debriefTitleWindow =
{
	NULL,
	NULL,
	{ 10,                           4,  620, 22 },
	{ 0,                            0,  0,   0, 0, 0, 0, 0},
	NULL,                           // font
	NULL,                           // keyDown
	NULL,                           // keyUp
	CG_Debriefing_MissionTitle_Draw,
	NULL,
	0
};

static panel_button_t debriefMissionTitleWindow =
{
	NULL,
	NULL,
	{ 10,                        30,  198, 326 },
	{ 0,                         0,   0,   0, 0, 0, 0, 0},
	NULL,                        // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_PanelButtonsRender_Window,
	NULL,
	0
};

static panel_button_t debriefMissionImage =
{
	NULL,
	NULL,
	{ 16,                      46,  186, 161 },
	{ 0,                       0,   0,   0, 0, 0, 0, 0},
	NULL,                      // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	CG_Debriefing_Mission_Draw,
	NULL,
	0
};

static panel_button_t debriefMissionMaps =
{
	NULL,
	NULL,
	{ 12,                   210,  194, 60 },
	{ 0,                    0,    0,   0, 0, 0, 0, 0},
	NULL,                   // font
	CG_Debriefing_Maps_KeyDown,// keyDown
	NULL,                   // keyUp
	CG_Debriefing_Maps_Draw,
	NULL,
	0
};

static panel_button_t debriefMissionAwardsWindow =
{
	NULL,
	"ROLL OF HONOR",
	{ 213,                       30, 417, 240 },
	{ ITEM_ALIGN_CENTER,         0,  0,   0, 0, 0, 0, 0},
	NULL,                        // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_PanelButtonsRender_Window,
	NULL,
	0
};

static panel_button_t debriefMissionAwardsList =
{
	NULL,
	NULL,
	{ 215,                    44,   413, 220 },
	{ ITEM_ALIGN_CENTER,      0,    0,   0, 0, 0, 0, 0},
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	CG_Debriefing_Awards_Draw,
	NULL,
	0
};

static panel_button_t debriefMissionAwardsListScroll =
{
	NULL,
	NULL,
	{ SCREEN_WIDTH - 10 - 8 - 16,DH_HEADING_Y,                          16, 206 },
	{ 4,                         0,                                     0,  0, 0, 0, 0, 0},
	NULL,                        /* font		*/
	CG_Debriefing_Scrollbar_KeyDown,/* keyDown	*/
	CG_Debriefing_Scrollbar_KeyUp,/* keyUp	*/
	CG_Debriefing_Scrollbar_Draw,
	NULL,
	0
};

static panel_button_t debriefMissionStatsWindow =
{
	NULL,
	"MISSION STATS",
	{ 213,                       280 - 6,417, 70 + 12 },
	{ 0,                         0,  0,   0, 0, 0, 0, 0},
	NULL,                        // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_PanelButtonsRender_Window,
	NULL,
	0
};

static panel_button_t debriefMissionStatsHeaders =
{
	NULL,
	NULL,
	{ 219,                              298,   405, 16 },
	{ 0,                                0,     0,   0, 0, 0, 0, 0},
	NULL,                               // font
	NULL,                               // keyDown
	NULL,                               // keyUp
	CG_Debriefing_TeamSkillHeaders_Draw,
	NULL,
	0
};

static panel_button_t debriefMissionStatsWinner =
{
	NULL,
	NULL,
	{ 219,                         314,   405, 16 },
	{ 0,                           0,     0,   0, 0, 0, 0, 0},
	NULL,                          // font
	NULL,                          // keyDown
	NULL,                          // keyUp
	CG_Debriefing_TeamSkillXP_Draw,
	NULL,
	0
};

static panel_button_t debriefMissionStatsLoser =
{
	NULL,
	NULL,
	{ 219,                         330,   405, 16 },
	{ 1,                           0,     0,   0, 0, 0, 0, 0},
	NULL,                          // font
	NULL,                          // keyDown
	NULL,                          // keyUp
	CG_Debriefing_TeamSkillXP_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerListWindow =
{
	NULL,
	"PLAYERS",
	{ 213,                       30, 417, 326 },
	{ 0,                         0,  0,   0, 0, 0, 0, 0},
	NULL,                        // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_PanelButtonsRender_Window,
	NULL,
	0
};

static panel_button_text_t debriefPlayerListFont =
{
	0.2f,                  0.2f,
	{ 0.6f,                0.6f,0.6f,    1.f },
	0,                     0,
	&cgs.media.limboFont2,
};

static panel_button_t debriefHeadingRank =
{
	NULL,
	"Rank",
	{ DB_RANK_X,               DH_HEADING_Y,       0, 0 },
	{ 0,                       0,                  0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefHeadingName =
{
	NULL,
	"Name",
	{ DB_NAME_X,               DH_HEADING_Y,       0, 0 },
	{ 0,                       0,                  0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefHeadingTime =
{
	NULL,
	"Time",
	{ DB_TIME_X,               DH_HEADING_Y,       0, 0 },
	{ 0,                       0,                  0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefHeadingKills =
{
	NULL,
	"Kll",
	{ DB_KILLS_X,              DH_HEADING_Y,         0, 0 },
	{ 0,                       0,                    0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefHeadingDeaths =
{
	NULL,
	"Dth",
	{ DB_DEATHS_X,             DH_HEADING_Y,          0, 0 },
	{ 0,                       0,                     0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefHeadingGibs =
{
	NULL,
	"Gib",
	{ DB_GIBS_X,               DH_HEADING_Y,        0, 0 },
	{ 0,                       0,                   0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefHeadingSelfKills =
{
	NULL,
	"SK",
	{ DB_SELFKILLS_X,          DH_HEADING_Y,              0, 0 },
	{ 0,                       0,                         0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefHeadingTeamKills =
{
	NULL,
	"TK",
	{ DB_TEAMKILLS_X,          DH_HEADING_Y,              0, 0 },
	{ 0,                       0,                         0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefHeadingTeamGibs =
{
	NULL,
	"TG",
	{ DB_TEAMGIBS_X,           DH_HEADING_Y,             0, 0 },
	{ 0,                       0,                        0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t debriefHeadingXP =
{
	NULL,
	"XP",
	{ DB_XP_X,                 DH_HEADING_Y,       0, 0 },
	{ 0,                       0,                  0, 0, 0, 0, 0, 0},
	&debriefPlayerListFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	CG_DebriefingXPHeader_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerList =
{
	NULL,
	NULL,
	{ DB_RANK_X,                 DH_HEADING_Y,         SCREEN_WIDTH - 10 - 8 - 16 - DB_RANK_X - 16, 292 },
	{ 0,                         0,                    0,                                           0, 0, 0, 0, 0},
	&debriefPlayerListFont,      // font
	CG_DebriefingPlayerList_KeyDown,// keyDown
	NULL,                        // keyUp
	CG_DebriefingPlayerList_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerListScroll =
{
	NULL,
	NULL,
	{ SCREEN_WIDTH - 10 - 8 - 16,DH_HEADING_Y,                          16, 292 },
	{ 0,                         0,                                     0,  0, 0, 0, 0, 0},
	NULL,                        // font
	CG_Debriefing_Scrollbar_KeyDown,// keyDown
	CG_Debriefing_Scrollbar_KeyUp,// keyUp
	CG_Debriefing_Scrollbar_Draw,
	NULL,
	0
};

static panel_button_text_t debriefPlayerInfoFont =
{
	0.2f,                  0.2f,
	{ 0.6f,                0.6f,0.6f,    1.f },
	0,                     0,
	&cgs.media.limboFont2,
};

static panel_button_t debriefPlayerInfoWindow =
{
	NULL,
	"PLAYER STATS",
	{ 10,                        30, 198, 326 },
	{ 0,                         0,  0,   0, 0, 0, 0, 0},
	NULL,                        // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_PanelButtonsRender_Window,
	NULL,
	0
};

static panel_button_t debriefPlayerInfoName =
{
	NULL,
	NULL,
	{ 14,                         56,  0, 0 },
	{ 0,                          0,   0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,       // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_Debriefing_PlayerName_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerInfoRank =
{
	NULL,
	NULL,
	{ 66,                         70,  0, 0 },
	{ 0,                          0,   0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,       // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_Debriefing_PlayerRank_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerInfoMedals =
{
	NULL,
	NULL,
	{ 66,                           84,  0, 0 },
	{ 0,                            0,   0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,         // font
	NULL,                           // keyDown
	NULL,                           // keyUp
	CG_Debriefing_PlayerMedals_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerInfoTime =
{
	NULL,
	NULL,
	{ 66,                         98,  0, 0 },
	{ 0,                          0,   0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,       // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_Debriefing_PlayerTime_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerInfoXP =
{
	NULL,
	NULL,
	{ 66,                       112,  0, 0 },
	{ 0,                        0,    0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,     // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_Debriefing_PlayerXP_Draw,
	NULL,
	0
};

#ifdef FEATURE_RATING
static panel_button_t debriefPlayerInfoSR =
{
	NULL,
	NULL,
	{ 134,                      112,   0, 0 },
	{ 0,                        0,     0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,     // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_Debriefing_PlayerSR_Draw,
	NULL,
	0
};
#endif

static panel_button_t debriefPlayerInfoACC =
{
	NULL,
	NULL,
	{ 66,                        126,  0, 0 },
	{ 0,                         0,    0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,      // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_Debriefing_PlayerACC_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerInfoHS =
{
	NULL,
	NULL,
	{ 134,                      126,   0, 0 },
	{ 0,                        0,     0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,     // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_Debriefing_PlayerHS_Draw,
	NULL,
	0
};

#ifdef FEATURE_PRESTIGE
static panel_button_t debriefPlayerInfoPrestige =
{
	NULL,
	NULL,
	{ 170,                            142,   0, 0 },
	{ 0,                              0,     0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,           // font
	NULL,                             // keyDown
	NULL,                             // keyUp
	CG_Debriefing_PlayerPrestige_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerPrestigeButton =
{
	NULL,
	"^3COLLECT POINT",
	{ 110,                            216,88, 16 },
	{ 0,                              0,  0,  0, 0, 0, 0, 0},
	NULL,                             // font
	CG_Debriefing_PrestigeButton_KeyDown,// keyDown
	NULL,                             // keyUp
	CG_Debriefing_PrestigeButton_Draw,
	NULL,
	0
};

static panel_button_t debriefPlayerPrestigeNote =
{
	NULL,
	NULL,
	{ 110,                            162,   0, 0 },
	{ 0,                              0,     0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,           // font
	NULL,                             // keyDown
	NULL,                             // keyUp
	CG_Debriefing_PlayerPrestige_Note,
	NULL,
	0
};
#endif

static panel_button_t debriefPlayerInfoHitRegions =
{
	NULL,
	NULL,
	{ 146,                              170,   0, 0 },
	{ 0,                                0,     0, 0, 0, 0, 0, 0},
	&debriefPlayerInfoFont,             // font
	NULL,                               // keyDown
	NULL,                               // keyUp
	CG_Debriefing_PlayerHitRegions_Draw,
	NULL,
	0
};

#define PLAYERHEADER_SKILLS(number)           \
	static panel_button_t debriefPlayerInfoSkills ## number = {      \
		NULL,                                       \
		NULL,                                       \
		{ 24,                            136 + (number * 14), 12, 12 }, \
		{ number,                        0,                   0,  0, 0, 0, 0, 0},  \
		&debriefPlayerInfoFont,          /* font     */  \
		NULL,                            /* keyDown  */  \
		NULL,                            /* keyUp    */  \
		CG_Debriefing_PlayerSkills_Draw,            \
		NULL,                                       \
		0,                                       \
	}

PLAYERHEADER_SKILLS(0);
PLAYERHEADER_SKILLS(1);
PLAYERHEADER_SKILLS(2);
PLAYERHEADER_SKILLS(3);
PLAYERHEADER_SKILLS(4);
PLAYERHEADER_SKILLS(5);
PLAYERHEADER_SKILLS(6);

static panel_button_t *debriefPanelButtons[] =
{
	&debriefTitleWindow,
	&debriefPlayerListWindow,       &debriefPlayerList,                   &debriefPlayerListScroll,
	&debriefHeadingRank,            &debriefHeadingName,
	&debriefHeadingTime,            &debriefHeadingXP,                    &debriefHeadingKills,                &debriefHeadingDeaths,                &debriefHeadingGibs,                  &debriefHeadingSelfKills, &debriefHeadingTeamKills, &debriefHeadingTeamGibs,
	&debriefPlayerInfoWindow,       &debriefPlayerInfoName,               &debriefPlayerInfoRank,              &debriefPlayerInfoMedals,             &debriefPlayerInfoTime,               &debriefPlayerInfoXP,
#ifdef FEATURE_RATING
	&debriefPlayerInfoSR,
#endif
	&debriefPlayerInfoACC,          &debriefPlayerInfoHS,
	&debriefPlayerInfoSkills0,
	&debriefPlayerInfoSkills1,
	&debriefPlayerInfoSkills2,
	&debriefPlayerInfoSkills3,
	&debriefPlayerInfoSkills4,
	&debriefPlayerInfoSkills5,
	&debriefPlayerInfoSkills6,
#ifdef FEATURE_PRESTIGE
	&debriefPlayerInfoPrestige,
	&debriefPlayerPrestigeButton,
	&debriefPlayerPrestigeNote,
#endif
	&debriefPlayerInfoHitRegions,
	&debriefPlayerWeaponStatsHeader,&debriefPlayerWeaponStatsNameHeader,  &debriefPlayerWeaponStatsShotsHeader,&debriefPlayerWeaponStatsHitsHeader,  &debriefPlayerWeaponStatsKillsHeader,
	&debriefPlayerWeaponStatsList,  &debriefPlayerWeaponStatsListScroll,

	NULL
};

static panel_button_text_t teamDebriefTitleSmall =
{
	0.24f,                 0.24f,
	{ 1.f,                 1.f,              1.f,0.8f },
	0,                     ITEM_ALIGN_CENTER,
	&cgs.media.limboFont2,
};

static panel_button_text_t teamDebriefTitle =
{
	0.28f,                 0.28f,
	{ 1.f,                 1.f,  1.f,0.8f },
	0,                     0,
	&cgs.media.limboFont2,
};

#define TDB_SKILL_TITLES_XP(number, title, x)             \
	static panel_button_t teamDebriefSkillXPText_ ## number = {          \
		NULL,                                                   \
		title,                                                  \
		{ 100 + (number * 65),      304 - (x * 12),                 20, 200 },      \
		{ 0,                        0,                              0,  0, 0, 0, 0, 0},                             \
		&teamDebriefTitleSmall,     /* font     */              \
		NULL,                       /* keyDown  */                      \
		NULL,                       /* keyUp    */                  \
		BG_PanelButtonsRender_Text,                             \
		NULL,                                                   \
		0,                                                   \
	}

TDB_SKILL_TITLES_XP(0, "Battle Sense", 0);
TDB_SKILL_TITLES_XP(1, "Engineer", 1);
TDB_SKILL_TITLES_XP(2, "Medic", 0);
TDB_SKILL_TITLES_XP(3, "Field Ops", 1);
TDB_SKILL_TITLES_XP(4, "Light Weapons", 0);
TDB_SKILL_TITLES_XP(5, "Soldier", 1);
TDB_SKILL_TITLES_XP(6, "Covert Ops", 0);
TDB_SKILL_TITLES_XP(7, "Total", 1);

#define TDB_SKILL_AXIS_XP(number)                         \
	static panel_button_t teamDebriefSkillXPText0_ ## number = {         \
		NULL,                                                   \
		NULL,                                                   \
		{ 110 + (number * 65),             320,                  470, 200 },                \
		{ 0,                               number,               0,   0, 0, 0, 0, 0},                        \
		&teamDebriefTitle,                 /* font     */                  \
		NULL,                              /* keyDown  */                      \
		NULL,                              /* keyUp    */                  \
		CG_TeamDebriefingTeamSkillXP_Draw,                      \
		NULL,                                                   \
		0,                                                   \
	}

#define TDB_SKILL_ALLIES_XP(number)                       \
	static panel_button_t teamDebriefSkillXPText1_ ## number = {         \
		NULL,                                                   \
		NULL,                                                   \
		{ 110 + (number * 65),             340,                  470, 200 },                \
		{ 1,                               number,               0,   0, 0, 0, 0, 0},                        \
		&teamDebriefTitle,                 /* font     */                  \
		NULL,                              /* keyDown  */                      \
		NULL,                              /* keyUp    */                  \
		CG_TeamDebriefingTeamSkillXP_Draw,                      \
		NULL,                                                   \
		0,                                                   \
	}

TDB_SKILL_AXIS_XP(0);
TDB_SKILL_AXIS_XP(1);
TDB_SKILL_AXIS_XP(2);
TDB_SKILL_AXIS_XP(3);
TDB_SKILL_AXIS_XP(4);
TDB_SKILL_AXIS_XP(5);
TDB_SKILL_AXIS_XP(6);
TDB_SKILL_AXIS_XP(7);

TDB_SKILL_ALLIES_XP(0);
TDB_SKILL_ALLIES_XP(1);
TDB_SKILL_ALLIES_XP(2);
TDB_SKILL_ALLIES_XP(3);
TDB_SKILL_ALLIES_XP(4);
TDB_SKILL_ALLIES_XP(5);
TDB_SKILL_ALLIES_XP(6);
TDB_SKILL_ALLIES_XP(7);

static panel_button_t *teamDebriefPanelButtons[] =
{
	&debriefTitleWindow,        &debriefMissionTitleWindow, &debriefMissionAwardsWindow, &debriefMissionImage,        &debriefMissionMaps, &debriefMissionAwardsList, &debriefMissionAwardsListScroll,
	&debriefMissionStatsWindow, &debriefMissionStatsWinner, &debriefMissionStatsLoser,   &debriefMissionStatsHeaders,
	NULL
};

static panel_button_text_t chatPanelButtonFont =
{
	0.20f,                 0.20f,
	{ 1.f,                 1.f,              1.f,0.8f },
	0,                     ITEM_ALIGN_CENTER,
	&cgs.media.limboFont2,
};

static panel_button_t chatPanelWindow =
{
	NULL,
	"CHAT",
	{ 10,                        SCREEN_HEIGHT - 122,620, 112 },
	{ 0,                         0,                  0,   0, 0, 0, 0, 0},
	NULL,                        // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_PanelButtonsRender_Window,
	NULL,
	0
};

static panel_button_t chatPanelText =
{
	NULL,
	NULL,
	{ 14,                      SCREEN_HEIGHT - 33,  SCREEN_WIDTH - 28, TEAMCHAT_HEIGHT },
	{ 0,                       0,                   0,                 0, 0, 0, 0, 0   },
	NULL,                      // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	CG_Debriefing_ChatBox_Draw,
	NULL,
	0
};

static panel_button_t chatPanelNextButton =
{
	NULL,
	"MORE",
	{ SCREEN_WIDTH - 10 - 60 - 4, SCREEN_HEIGHT - 30,                        60, 16 },
	{ 0,                          0,                                         0,  0, 0, 0, 0, 0},
	NULL,                         // font
	CG_Debriefing_NextButton_KeyDown,// keyDown
	NULL,                         // keyUp
	CG_Debriefing_NextButton_Draw,
	NULL,
	0
};

static panel_button_t chatPanelVoteButton =
{
	NULL,
	"MAP VOTE",
	{ SCREEN_WIDTH - 10 - 60 - 4 - 60 - 4,SCREEN_HEIGHT - 30,                             60, 16 },
	{ 0,                          0,                                              0,  0, 0, 0, 0, 0},
	NULL,                         // font
	CG_Debriefing_VoteButton_KeyDown,// keyDown
	NULL,                         // keyUp
	CG_Debriefing_VoteButton_Draw,
	NULL,
	0
};

static panel_button_t chatPanelVoteNowButton =
{
	NULL,
	"^3VOTE NOW",
	{ SCREEN_WIDTH - 10 - 60 - 4 - 60 - 4,SCREEN_HEIGHT - 30,                           60, 16 },
	{ 0,                             0,                                            0,  0, 0, 0, 0, 0},
	NULL,                            // font
	CG_Debriefing_VoteButton_KeyDown,// keyDown
	NULL,                            // keyUp
	CG_Debriefing_VoteNowButton_Draw,
	NULL,
	0
};

static panel_button_t chatPanelQCButton =
{
	NULL,
	"QUICK CHAT",
	{ SCREEN_WIDTH - 10 - 60 - 4 - 60 - 4 - 80 - 4,SCREEN_HEIGHT - 30,                                    80, 16 },
	{ 0,                         0,                                                     0,  0, 0, 0, 0, 0},
	NULL,                        // font
	CG_Debriefing_QCButton_KeyDown,// keyDown
	NULL,                        // keyUp
	CG_PanelButtonsRender_Button,
	NULL,
	0
};

static panel_button_t chatPanelReadyButton =
{
	NULL,
	"READY",
	{ SCREEN_WIDTH - 10 - 60 - 4 - 60 - 4 - 80 - 4 - 80 - 4,SCREEN_HEIGHT - 30,                                                  80, 16 },
	{ 0,                           0,                                                                   0,  0, 0, 0, 0, 0},
	NULL,                          // font
	CG_Debriefing_ReadyButton_KeyDown,// keyDown
	NULL,                          // keyUp
	CG_Debriefing_ReadyButton_Draw,
	NULL,
	0
};

static panel_button_t chatTypeButton =
{
	NULL,
	NULL,
	{ 10 + 4,                     SCREEN_HEIGHT - 30,      80, 16 },
	{ 0,                          0,                       0,  0, 0, 0, 0, 0},
	NULL,                         // font
	CG_Debriefing_ChatButton_KeyDown,// keyDown
	NULL,                         // keyUp
	CG_Debriefing_ChatButton_Draw,
	NULL,
	0
};

static panel_button_t charPanelEditSurround =
{
	NULL,
	NULL,
	{ 10 + 4 + 80 + 4,           SCREEN_HEIGHT - 30,               232, 16 },
	{ 0,                         0,                                0,   0, 0, 0, 0, 0},
	NULL,                        // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_PanelButtonsRender_Button,
	NULL,
	0
};

static panel_button_t charPanelEdit =
{
	NULL,
	"chattext",
	{ 10 + 4 + 80 + 4 + 8,       SCREEN_HEIGHT - 34,             216, 16 },
	{ 0,                         0,                              0,   0, 0, 0, 0, 0},
	&chatPanelButtonFont,        // font
	NULL,                        /*BG_PanelButton_EditClick,*/ // keyDown
	NULL,                        // keyUp
	CG_Debriefing_ChatEdit_Draw,
	CG_Debriefing_ChatEditFinish,
	0
};

// MAPVOTE
#define DB_MAPNAME_X    15
#define DB_MAPVOTE_X    (DB_MAPNAME_X + 200)
#define DB_MAPVOTE_Y    (56 + 10)
#define DB_MAPVOTE_X2   (620 - 192 - 96 - 20)
#define DB_MAPVOTE_Y2   (326 + 30 - 192 - 16)

/**
 * @brief CG_MapVoteList_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_MapVoteList_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		int pos = ((cgs.cursorY - DB_MAPVOTE_Y - 2) / 12) + cgs.dbMapVoteListOffset;

		if (pos < 0 || pos >= cgs.dbNumMaps)
		{
			return qfalse;
		}
		if (pos != cgs.dbSelectedMap)
		{
			cgs.dbSelectedMap     = pos;
			cgs.dbSelectedMapTime = cg.time;
		}
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief CG_MapVote_VoteButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_MapVote_VoteButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (!cg.snap)
		{
			return qfalse;
		}

		if (cgs.dbMapMultiVote)
		{
			return qfalse;
		}

		if (cgs.dbSelectedMap != -1)
		{
			cgs.dbMapVotedFor[0] = cgs.dbSelectedMap;
			return qtrue;
		}
	}

	return qfalse;
}

vec4_t clrTxtBck = { 0.6f, 0.6f, 0.6f, 1.0f };

/**
 * @brief CG_MapVote_MultiVoteButton_Draw
 * @param[in] button
 */
void CG_MapVote_MultiVoteButton_Draw(panel_button_t *button)
{
	if (!cg.snap)
	{
		return;
	}

	if (!cgs.dbMapMultiVote)
	{
		return;
	}

	if (cgs.dbMapVotedFor[button->data[7] - 1] != -1)
	{
		CG_Text_Paint_Ext(button->rect.x + button->rect.w + 10,
		                  button->rect.y + (3 * (button->rect.h / 4)),
		                  .20f, .20f, clrTxtBck,
		                  cgs.dbMapDispName[cgs.dbMapVotedFor[button->data[7] - 1]],
		                  0, 0, 0, &cgs.media.limboFont2);
	}

	if (!(cg.snap->ps.eFlags & EF_VOTED))
	{
		const char *str;

		if (cgs.dbMapVotedFor[button->data[7] - 1] != -1)
		{
			str = va("^3%d: ^7RE-VOTE", 4 - button->data[7]);
		}
		else
		{
			str = va("^3%d: ^7VOTE", 4 - button->data[7]);
		}

		CG_PanelButtonsRender_Button_Ext(&button->rect, str);
	}
}

/**
 * @brief CG_MapVoteList_Draw
 * @param[in] button
 */
void CG_MapVoteList_Draw(panel_button_t *button)
{
	int           i;
	float         y      = button->rect.y + 12;
	float         y2     = DB_MAPVOTE_Y;
	qhandle_t     pic    = 0;
	static vec4_t acolor = { 1.0f, 1.0f, 1.0f, 1.0f };
	int           diff;

	// display map number if server is configured
	// to reset XP after certain number of maps
	if (cgs.mapVoteMapY > 0)
	{
		const char* s;
		float w;

		s = va("Map %d of %d", cgs.mapVoteMapX + 1, cgs.mapVoteMapY);
		w = CG_Text_Width_Ext(s, button->font->scalex, 0, button->font->font);
		CG_Text_Paint_Ext(DB_MAPNAME_X + 10 + cgs.wideXoffset + DB_MAPVOTE_X * 0.5f - w * 0.5f, y2,
			              button->font->scalex, button->font->scaley, button->font->colour,
		                  s, 0, 0, 0, button->font->font);
	}

	y2 += 15;

	for (i = 0; i + cgs.dbMapVoteListOffset < cgs.dbNumMaps && i < 16; i++)
	{
		if (strlen(cgs.dbMaps[i + cgs.dbMapVoteListOffset]) < 1)
		{
			break;
		}

		if (cgs.dbSelectedMap == i + cgs.dbMapVoteListOffset)
		{
			fileHandle_t f;
			vec4_t       clr = { 1.f, 1.f, 1.f, 0.3f };

			CG_FillRect(button->rect.x, y - 10, 245, 12, clr);

			// display the photograph image only..
			// ..and make it fade in nicely.
			diff      = cg.time - cgs.dbSelectedMapTime;
			acolor[3] = (diff > 1000) ? 1.0f : (float)diff / 1000.f;
			trap_R_SetColor(acolor);
			// First check if the corresponding map is downloaded to prevent warning about missing levelshot
			if (trap_FS_FOpenFile(va("maps/%s.bsp", cgs.dbMaps[i + cgs.dbMapVoteListOffset]), &f, FS_READ) > 0)
			{
				pic = trap_R_RegisterShaderNoMip(va("levelshots/%s.tga", cgs.dbMaps[i + cgs.dbMapVoteListOffset]));
				trap_FS_FCloseFile(f);
			}
			if (pic)
			{
				CG_DrawPic(DB_MAPVOTE_X2 + 24 + cgs.wideXoffset, DB_MAPVOTE_Y2 + 2, 250, 177.0f / 233.0f * 250, pic);
			}
			trap_R_SetColor(NULL);
			// display the photograph image + the layout image..

			CG_Text_Paint_Ext(DB_MAPVOTE_X2 + cgs.wideXoffset, y2, button->font->scalex,
			                  button->font->scaley, button->font->colour,
			                  va(CG_TranslateString("Last Played             : %s"),
			                     (cgs.dbMapLastPlayed[i + cgs.dbMapVoteListOffset] == -1 ? CG_TranslateString("Never") : va(CG_TranslateString("%d maps ago"),
			                                                                                                                cgs.dbMapLastPlayed[i + cgs.dbMapVoteListOffset]))),
			                  0, 0, 0, button->font->font);
			y2 += 15;
			CG_Text_Paint_Ext(DB_MAPVOTE_X2 + cgs.wideXoffset, y2, button->font->scalex,
			                  button->font->scaley, button->font->colour,
			                  va(CG_TranslateString("Total Accumulated Votes : %d"), cgs.dbMapTotalVotes[i + cgs.dbMapVoteListOffset]),
			                  0, 0, 0, button->font->font);
		}

		CG_Text_Paint_Ext(DB_MAPNAME_X + 12 + cgs.wideXoffset, y, button->font->scalex,
		                  button->font->scaley, button->font->colour,
		                  cgs.dbMapDispName[i + cgs.dbMapVoteListOffset],
		                  0, 30, 0, button->font->font);

		if (cg.snap->ps.eFlags & EF_VOTED)
		{
			int j = 0;

			vec4_t *colour = &button->font->colour;

			// add gradient color to identify three most voted maps
			for (j = 0; j < 3; j++)
			{
				if (cgs.dbSortedVotedMapsByTotal[j].totalVotes <= 0)
				{
					continue;
				}

				if (cgs.dbSortedVotedMapsByTotal[j].mapID == (i + cgs.dbMapVoteListOffset))
				{
					if (j == 0)
					{
						colour = &colorGreen;
					}
					else if (j == 1)
					{
						colour = &colorMdGreen;
					}
					else if (j == 2)
					{
						colour = &colorDkGreen;
					}
				}
			}

			CG_Text_Paint_Ext(DB_MAPVOTE_X + cgs.wideXoffset, y, button->font->scalex,
			                  button->font->scaley, *colour,
			                  va("%3d%% (%d)", cgs.dbMapVotesSum > 0 ? 100 * cgs.dbMapVotes[i + cgs.dbMapVoteListOffset] / cgs.dbMapVotesSum : 0,
			                     cgs.dbMapVotes[i + cgs.dbMapVoteListOffset]),
			                  0, 0, 0, button->font->font);
		}

		y += 12;
	}
}

/**
 * @brief CG_MapVote_VoteSend_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_MapVote_VoteSend_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (!cg.snap)
		{
			return qfalse;
		}

		if (!cgs.dbMapMultiVote)
		{
			if (cgs.dbMapVotedFor[0] != -1)
			{
				trap_SendClientCommand(va("mapvote %d", cgs.dbMapID[cgs.dbMapVotedFor[0]]));
				return qtrue;
			}
		}
		else
		{
			int      i;
			int      votedMapID[3];
			qboolean mapVoted = qfalse;

			for (i = 0; i < 3; i++)
			{
				if (cgs.dbMapVotedFor[i] != -1)
				{
					votedMapID[i] = cgs.dbMapID[cgs.dbMapVotedFor[i]];
					mapVoted      = qtrue;
				}
				else
				{
					votedMapID[i] = -1;
				}
			}

			if (mapVoted)
			{
				trap_SendClientCommand(va("mapvote %d %d %d", votedMapID[0], votedMapID[1], votedMapID[2]));

				return qtrue;
			}
		}
	}

	return qfalse;
}

/**
 * @brief CG_MapVote_VoteSend_Draw
 * @param[in] button
 */
void CG_MapVote_VoteSend_Draw(panel_button_t *button)
{
	if (!cg.snap)
	{
		return;
	}

	if (!(cg.snap->ps.eFlags & EF_VOTED))
	{
		if (!cgs.dbMapMultiVote)
		{
			if (cgs.dbMapVotedFor[0] == -1)
			{
				return;
			}
		}
		else
		{
			int i;

			for (i = 0; i < 3; i++)
			{
				if (cgs.dbMapVotedFor[i] != -1)
				{
					break;
				}

				if (i == 2)
				{
					return;
				}
			}
		}

		CG_PanelButtonsRender_Button_Ext(&button->rect, button->text);
	}
	else
	{
		CG_Text_Paint_Ext(button->rect.x + ((button->rect.w / 2) - (5 * 8)),
		                  button->rect.y + (3 * (button->rect.h / 4)),
		                  .20f, .20f, clrTxtBck,
		                  "^2VOTE DONE!",
		                  0, 0, 0, &cgs.media.limboFont2);
	}
}

/**
 * @brief CG_MapVote_VoteButton_Draw
 * @param[in] button
 */
void CG_MapVote_VoteButton_Draw(panel_button_t *button)
{
	if (!cg.snap)
	{
		return;
	}

	if (cgs.dbMapMultiVote)
	{
		return;
	}

	if (cgs.dbMapVotedFor[0] != -1)
	{
		CG_Text_Paint_Ext(button->rect.x + button->rect.w + 10,
		                  button->rect.y + (3 * (button->rect.h / 4)),
		                  .20f, .20f, clrTxtBck,
		                  cgs.dbMapDispName[cgs.dbMapVotedFor[0]],
		                  0, 0, 0, &cgs.media.limboFont2);
	}

	if (!(cg.snap->ps.eFlags & EF_VOTED))
	{
		CG_PanelButtonsRender_Button_Ext(&button->rect, (cgs.dbMapVotedFor[0] != -1) ? "^7RE-VOTE" : "^7VOTE");
	}
}

/**
 * @brief CG_MapVote_MultiVoteButton_KeyDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_MapVote_MultiVoteButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (!cg.snap)
		{
			return qfalse;
		}

		if (!cgs.dbMapMultiVote)
		{
			return qfalse;
		}

		if (cgs.dbSelectedMap != -1)
		{
			int i;
			int arrIdx = button->data[7] - 1;

			for (i = 0; i < 3; i++)
			{
				if (arrIdx == i)
				{
					continue;
				}
				if (cgs.dbMapVotedFor[i] == cgs.dbSelectedMap)
				{
					CG_Printf("^3Can't vote for the same map twice\n");
					return qfalse;
				}
			}

			cgs.dbMapVotedFor[arrIdx] = cgs.dbSelectedMap;
			return qtrue;
		}
	}

	return qfalse;
}

static panel_button_t mapVoteWindow =
{
	NULL,
	"MAP VOTE",
	{ 10,                        30, 620, 326 },
	{ 0,                         0,  0,   0, 0, 0, 0, 0},
	NULL,                        // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_PanelButtonsRender_Window,
	NULL,
	0
};

static panel_button_text_t mapVoteFont =
{
	0.2f,                  0.2f,
	{ 0.6f,                0.6f,0.6f,    1.f },
	0,                     0,
	&cgs.media.limboFont2,
};

static panel_button_t mapVoteHeadingName =
{
	NULL,
	"Name",
	{ DB_MAPNAME_X + 10,       DB_MAPVOTE_Y,               0, 0 },
	{ 0,                       0,                          0, 0, 0, 0, 0, 0},
	&mapVoteFont,              // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t mapVoteHeadingVotes =
{
	NULL,
	"Votes",
	{ DB_MAPVOTE_X,            DB_MAPVOTE_Y,         0, 0 },
	{ 0,                       0,                    0, 0, 0, 0, 0, 0},
	&mapVoteFont,              // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t mapVoteNamesList =
{
	NULL,
	NULL,
	{ DB_MAPNAME_X + 10,DB_MAPVOTE_Y,                 250, 16 * 12 },
	{ 0,                0,                            0,   0, 0, 0, 0, 0},
	&mapVoteFont,       // font
	CG_MapVoteList_KeyDown,// keyDown
	NULL,               // keyUp
	CG_MapVoteList_Draw,
	NULL,
	0
};

static panel_button_t mapVoteNamesListScroll =
{
	NULL,
	NULL,
	{ DB_MAPVOTE_X + 10 + 48,    DB_MAPVOTE_Y + 2,                      16, 16 * 12 },
	{ 3,                         0,                                     0,  0, 0, 0, 0, 0},
	NULL,                        // font
	CG_Debriefing_Scrollbar_KeyDown,// keyDown
	CG_Debriefing_Scrollbar_KeyUp,// keyUp
	CG_Debriefing_Scrollbar_Draw,
	NULL,
	0
};

static panel_button_t mapVoteButton =
{
	NULL,
	"^3VOTE",
	{ DB_MAPNAME_X + 10,       296 - 10 + 2,             64, 16 },
	{ 0,                       0,                        0,  0, 0, 0, 0, 0},
	NULL,                      // font
	CG_MapVote_VoteButton_KeyDown,// keyDown
	NULL,                      // keyUp
	CG_MapVote_VoteButton_Draw,
	NULL,
	0
};

static panel_button_t mapVoteButton1 =
{
	NULL,
	"^3VOTE #1",
	{ DB_MAPNAME_X + 10,            296 - 10 + 2,          64, 16 },
	{ 0,                            0,                     0,  0, 0, 0, 0, 3},
	NULL,                           // font
	CG_MapVote_MultiVoteButton_KeyDown,// keyDown
	NULL,                           // keyUp
	CG_MapVote_MultiVoteButton_Draw,
	NULL,
	0
};

static panel_button_t mapVoteButton2 =
{
	NULL,
	"^3VOTE #2",
	{ DB_MAPNAME_X + 10,            296 + 10 + 2,          64, 16 },
	{ 0,                            0,                     0,  0, 0, 0, 0, 2},
	NULL,                           // font
	CG_MapVote_MultiVoteButton_KeyDown,// keyDown
	NULL,                           // keyUp
	CG_MapVote_MultiVoteButton_Draw,
	NULL,
	0
};

static panel_button_t mapVoteButton3 =
{
	NULL,
	"^3VOTE #3",
	{ DB_MAPNAME_X + 10,            296 + 30 + 2,          64, 16 },
	{ 0,                            0,                     0,  0, 0, 0, 0, 1},
	NULL,                           // font
	CG_MapVote_MultiVoteButton_KeyDown,// keyDown
	NULL,                           // keyUp
	CG_MapVote_MultiVoteButton_Draw,
	NULL,
	0
};

static panel_button_t mapVoteSend =
{
	NULL,
	"^3SEND VOTE",
	{ DB_MAPNAME_X + 10,     296 - 30 + 2,        266, 16 },
	{ 0,                     0,                   0,   0, 0, 0, 0, 0},
	NULL,                    // font
	CG_MapVote_VoteSend_KeyDown,// keyDown
	NULL,                    // keyUp
	CG_MapVote_VoteSend_Draw,
	NULL,
	0
};

static panel_button_t mapVoteBorder1 =
{
	NULL,
	NULL,
	{ DB_MAPVOTE_X2 - 10,     DB_MAPVOTE_Y - 12,                  620 - DB_MAPVOTE_X2 + 10, DB_MAPVOTE_Y2 - DB_MAPVOTE_Y - 4 },
	{ 1,                      255,                                255,                      255, 40, 1, 0, 0                 },
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

static panel_button_t mapVoteBorder2 =
{
	NULL,
	NULL,
	{ DB_MAPVOTE_X2 - 10,     DB_MAPVOTE_Y2 - 10 + 2,                  620 - DB_MAPVOTE_X2 + 10, 370 - DB_MAPVOTE_Y2 - 2 - 10 },
	{ 1,                      255,                                     255,                      255, 40, 1, 0, 0             },
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

static panel_button_t mapVoteBorder3 =
{
	NULL,
	NULL,
	{ 20,                     DB_MAPVOTE_Y - 12,  DB_MAPVOTE_X2 - 40 + 2, 370 - DB_MAPVOTE_Y - 10 + 2 },
	{ 1,                      255,                255,                    255, 40, 1, 0, 0            },
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};
// MAPVOTE END

/**
 * @brief CG_Debriefing_ChatEdit_Draw
 * @param[in] button
 */
void CG_Debriefing_ChatEdit_Draw(panel_button_t *button)
{
	int    offset = -1;
	char   buffer[MAX_EDITFIELD + 1];
	vec4_t *color;

	trap_Cvar_VariableStringBuffer(button->text, buffer, sizeof(buffer));

	do
	{
		offset++;
		if (buffer[offset] == '\0')
		{
			break;
		}
	}
	while (CG_Text_Width_Ext(buffer + offset, button->font->scalex, 0, button->font->font) > button->rect.w);

	switch (cgs.dbChatMode)
	{
	case 0:
		color = &colorWhite;
		break;
	case 1:
		color = &colorCyan;
		break;
	case 2:
		color = &colorYellow;
		break;
	default:
		color = &button->font->colour;
		break;
	}

	CG_Text_PaintWithCursor_Ext(button->rect.x, button->rect.y + button->rect.h, button->font->scalex, *color, buffer + (button->data[2] <= offset ? button->data[2] : offset), button->data[2] <= offset ? 0 : button->data[2] - offset, trap_Key_GetOverstrikeMode() ? "_" : "|", offset ? Q_UTF8_Strlen(buffer + offset) : 0, button->font->style, button->font->font);
}

/**
 * @brief CG_Debriefing_ChatBox_Draw
 * @param[in] button
 */
void CG_Debriefing_ChatBox_Draw(panel_button_t *button)
{
	if (cgs.teamLastChatPos != cgs.teamChatPos)
	{
		vec4_t hcolor;
		float  lineHeight = 9.f;
		int    i;
		int    chatWidth  = button->rect.w;
		int    chatHeight = button->rect.h;

		for (i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++)
		{
			CG_Text_Width_Ext(cgs.teamChatMsgs[i % chatHeight], 0.2f, 0, &cgs.media.limboFont2);
		}

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--)
		{
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_AXIS)
			{
				hcolor[0] = 1;
				hcolor[1] = 0;
				hcolor[2] = 0;
			}
			else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES)
			{
				hcolor[0] = 0;
				hcolor[1] = 0;
				hcolor[2] = 1;
			}
			else
			{
				hcolor[0] = 0;
				hcolor[1] = 1;
				hcolor[2] = 0;
			}

			hcolor[3] = 0.33f;

			trap_R_SetColor(hcolor);
			CG_DrawPic(button->rect.x, button->rect.y - (cgs.teamChatPos - i) * lineHeight, chatWidth, lineHeight, cgs.media.teamStatusBar);

			trap_R_SetColor(NULL);

			if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_AXIS)
			{
				CG_DrawPic(button->rect.x, button->rect.y - (cgs.teamChatPos - i - 1) * lineHeight - 8, 12, 10, cgs.media.axisFlag);
			}
			else if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_ALLIES)
			{
				CG_DrawPic(button->rect.x, button->rect.y - (cgs.teamChatPos - i - 1) * lineHeight - 8, 12, 10, cgs.media.alliedFlag);
			}

			CG_Text_Paint_Ext(button->rect.x + 12, button->rect.y - (cgs.teamChatPos - i - 1) * lineHeight, 0.2f, 0.2f, colorWhite, cgs.teamChatMsgs[i % chatHeight], 0, 0, 0, &cgs.media.limboFont2);
		}
	}
}

static panel_button_t *chatPanelButtons[] =
{
	&chatPanelWindow,       &chatPanelText,
	&chatPanelNextButton,   &chatPanelVoteButton,&chatPanelVoteNowButton,  &chatPanelQCButton, &chatTypeButton, &chatPanelReadyButton,
	&charPanelEditSurround, &charPanelEdit,
	NULL
};

static panel_button_t *mapVoteButtons[] =
{
	&debriefTitleWindow,     &mapVoteWindow,    &mapVoteHeadingName, &mapVoteHeadingVotes,
	&mapVoteBorder1,         &mapVoteBorder2,   &mapVoteBorder3,
	&mapVoteNamesListScroll, &mapVoteNamesList, &mapVoteButton,
	&mapVoteButton1,         &mapVoteButton2,   &mapVoteButton3,     &mapVoteSend,
	NULL
};

/**
 * @brief CG_ChatPanel_Setup
 */
void CG_ChatPanel_Setup(void)
{
	BG_PanelButtonsSetup(chatPanelButtons);
	BG_PanelButtonsSetup(teamDebriefPanelButtons);
	BG_PanelButtonsSetup(debriefPanelButtons);
	// MAPVOTE TODO: only active for gametype 6 ?
	BG_PanelButtonsSetup(mapVoteButtons);

	// convert to possible ws coordinates..
	C_PanelButtonsSetup(chatPanelButtons, cgs.wideXoffset);
	C_PanelButtonsSetup(teamDebriefPanelButtons, cgs.wideXoffset);
	C_PanelButtonsSetup(debriefPanelButtons, cgs.wideXoffset);
	C_PanelButtonsSetup(mapVoteButtons, cgs.wideXoffset);
	// there is an exception: the same debriefTitleWindow is used in multiple panel_button_t
	// By now the debriefTitleWindow has been adjusted too much, so we correct for the difference..
	debriefTitleWindow.rect.x -= 2 * cgs.wideXoffset;
}

/**
 * @brief CG_Debriefing_Startup
 */
void CG_Debriefing_Startup(void)
{
	const char *s, *buf;

	cgs.dbShowing                   = qtrue;
	cgs.dbAccuraciesReceived        = qfalse;
	cgs.dbWeaponStatsReceived       = qfalse;
	cgs.dbPlayerKillsDeathsReceived = qfalse;
	cgs.dbPlayerTimeReceived        = qfalse;
#ifdef FEATURE_RATING
	cgs.dbSkillRatingReceived = qfalse;
#endif
#ifdef FEATURE_PRESTIGE
	cgs.dbPrestigeReceived = qfalse;
#endif
	cgs.dbLastScoreReceived = qfalse;

	cgs.dbLastRequestTime = 0;
	cgs.dbSelectedClient  = cg.clientNum;

	// mapvote
	cgs.dbSelectedMap       = -1;
	cgs.dbMapListReceived   = qfalse;
	cgs.dbVoteTallyReceived = qfalse;
	cgs.dbMapVotedFor[0]    = -1;
	cgs.dbMapVotedFor[1]    = -1;
	cgs.dbMapVotedFor[2]    = -1;

	cgs.dbAwardsParsed = qfalse;

	// display results first
	cgs.dbMode = 0;

	s   = CG_ConfigString(CS_MULTI_MAPWINNER);
	buf = Info_ValueForKey(s, "w");

	trap_Cvar_Set("chattext", "");

	if (Q_atoi(buf) == -1)
	{
	}
	else if (Q_atoi(buf))
	{
		trap_S_StartLocalSound(trap_S_RegisterSound("sound/music/allies_win.wav", qfalse), CHAN_LOCAL_SOUND);
	}
	else
	{
		trap_S_StartLocalSound(trap_S_RegisterSound("sound/music/axis_win.wav", qfalse), CHAN_LOCAL_SOUND);
	}
}

/**
 * @brief CG_Debriefing_Shutdown
 */
void CG_Debriefing_Shutdown(void)
{
	cgs.dbShowing = qfalse;
}

/**
 * @brief CG_Debriefing_InfoRequests
 *
 * @note debriefing server commands are parsed in CG_ServerCommand() - CG_Debriefing_ServerCommand() has been removed
 */
void CG_Debriefing_InfoRequests(void)
{
	if (cgs.dbLastRequestTime && (cg.time - cgs.dbLastRequestTime) < 1000)
	{
		return;
	}
	cgs.dbLastRequestTime = cg.time;

	if (!cgs.dbMapListReceived && cgs.gametype == GT_WOLF_MAPVOTE)
	{
		trap_SendClientCommand("immaplist");
		return;
	}

	if (!cgs.dbVoteTallyReceived && cgs.gametype == GT_WOLF_MAPVOTE)
	{
		trap_SendClientCommand("imvotetally");
		return;
	}

	if (!cgs.dbPlayerTimeReceived)
	{
		trap_SendClientCommand("impt");
		return;
	}

#ifdef FEATURE_RATING
	if (!cgs.dbSkillRatingReceived && cgs.skillRating)
	{
		trap_SendClientCommand("imsr");
		return;
	}
#endif

#ifdef FEATURE_PRESTIGE
	if (!cgs.dbPrestigeReceived && cgs.prestige)
	{
		trap_SendClientCommand("impr");
		return;
	}
#endif

	if (!cgs.dbPlayerKillsDeathsReceived)
	{
		trap_SendClientCommand("impkd");
		return;
	}

	if (!cgs.dbAccuraciesReceived)
	{
		trap_SendClientCommand("imwa");
		return;
	}

	if (!cgs.dbWeaponStatsReceived)
	{
		trap_SendClientCommand(va("imws %i", cgs.dbSelectedClient));
		return;
	}

	// if nothing else is pending, ask for scores
	if (!cgs.dbLastScoreReceived)
	{
		trap_SendClientCommand("score");
	}
}

/**
 * @brief CG_Debriefing_Draw
 * @return
 */
qboolean CG_Debriefing_Draw(void)
{
	int i;

	if (!cgs.dbShowing)
	{
		CG_Debriefing_Startup();
	}

	CG_Debriefing_InfoRequests();

	if (trap_Key_GetCatcher() & KEYCATCH_UI)
	{
		return qtrue;
	}

	if (!trap_Key_GetCatcher())
	{
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}

	// build award list asap to avoid empty name if awarded players disconnect early
	if (!cgs.dbAwardsParsed)
	{
		CG_Debriefing_ParseAwards();
	}

	switch (cgs.dbMode)
	{
	case 0: // player list
		CG_DrawScoreboard();
		BG_PanelButtonsRender(chatPanelButtons);
		CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);
		break;
	case 1: // awards
		BG_PanelButtonsRender(teamDebriefPanelButtons);
		BG_PanelButtonsRender(chatPanelButtons);
		CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);
		break;
	case 2: // campaign
		for (i = 0 ; i < cgs.maxclients; i++)
		{
			cgs.dbSortedClients[i] = i;
		}

		qsort(cgs.dbSortedClients, cgs.maxclients, sizeof(int), CG_SortPlayersByXP);

		BG_PanelButtonsRender(debriefPanelButtons);
		BG_PanelButtonsRender(chatPanelButtons);
		CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);
		break;
	case 3: // mapvote
		BG_PanelButtonsRender(mapVoteButtons);
		BG_PanelButtonsRender(chatPanelButtons);
		CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);
		break;
	}

	return qtrue;
}

/**
 * @brief CG_DebriefingPlayerList_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_DebriefingPlayerList_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		int pos = ((cgs.cursorY - DH_HEADING_Y) / 12) + cgs.dbPlayerListOffset;

		if (pos < 0 || pos >= cgs.maxclients)
		{
			return qfalse;
		}

		pos = cgs.dbSortedClients[pos];

		if (!cgs.clientinfo[pos].infoValid)
		{
			return qfalse;
		}

		CG_Debrieing_SetSelectedClient(pos);

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_Debriefing_GetNextWeaponStat
 * @param[in] pos
 * @return
 */
int CG_Debriefing_GetNextWeaponStat(int pos)
{
	int i;

	for (i = pos + 1; i < WS_MAX; i++)
	{
		if (cgs.dbWeaponStats[i].numShots)
		{
			return i;
		}
	}

	return -1;
}

/**
 * @brief CG_DebriefingPlayerWeaponStats_Draw
 * @param[in] button
 */
void CG_DebriefingPlayerWeaponStats_Draw(panel_button_t *button)
{
	int   i;
	float y = button->rect.y + 12;
	int   pos;

	if (!cgs.dbWeaponStatsReceived)
	{
		return;
	}

	pos = CG_Debriefing_GetNextWeaponStat(-1);
	for (i = cgs.dbWeaponListOffset; i > 0 && pos != -1; i--)
	{
		pos = CG_Debriefing_GetNextWeaponStat(pos);
	}

	for (i = 0; i < 7 && pos != -1; i++, pos = CG_Debriefing_GetNextWeaponStat(pos))
	{
		CG_Text_Paint_Ext(button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, aWeaponInfo[pos].pszName, 0, 0, 0, button->font->font);
		CG_Text_Paint_Ext(button->rect.x + 62, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", cgs.dbWeaponStats[pos].numShots), 0, 0, 0, button->font->font);
		CG_Text_Paint_Ext(button->rect.x + 102, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", cgs.dbWeaponStats[pos].numHits), 0, 0, 0, button->font->font);
		// syringe doesn't have kill stats
		if (pos != WS_SYRINGE)
		{
			CG_Text_Paint_Ext(button->rect.x + 132, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", cgs.dbWeaponStats[pos].numKills), 0, 0, 0, button->font->font);
		}

		y += 12;
	}
}

/**
 * @brief CG_Debriefing_TimeToString
 * @param[in] msec
 * @return
 */
const char *CG_Debriefing_TimeToString(float msec)
{
	int mins, seconds, tens;

	seconds  = msec / 1000;
	mins     = seconds / 60;
	seconds -= mins * 60;
	tens     = seconds / 10;
	seconds -= tens * 10;

	return va("%i:%i%i", mins, tens, seconds);
}

/**
 * @brief CG_DebriefingTitle_Draw
 * @param[in] button
 *
 * @note Unused
 */
void CG_DebriefingTitle_Draw(panel_button_t *button)
{
	const char *s, *buf;
	float      x, w;

	if (cg_gameType.integer == GT_WOLF_STOPWATCH)
	{
		int defender, winner;

		s        = CG_ConfigString(CS_MULTI_INFO);
		defender = Q_atoi(Info_ValueForKey(s, "d")); // defender

		s      = CG_ConfigString(CS_MULTI_MAPWINNER);
		winner = Q_atoi(Info_ValueForKey(s, "w"));

		if (cgs.currentRound)
		{
			// first round
			s = va(CG_TranslateString("CLOCK IS NOW SET TO %s!"), CG_Debriefing_TimeToString(cgs.nextTimeLimit * 60000.f)); // 60.f * 1000.f
		}
		else
		{
			// second round
			if (!defender)
			{
				if (winner != defender)
				{
					s = "ALLIES SUCCESSFULLY BEAT THE CLOCK!";
				}
				else
				{
					s = "ALLIES COULDN'T BEAT THE CLOCK!";
				}
			}
			else
			{
				if (winner != defender)
				{
					s = "AXIS SUCCESSFULLY BEAT THE CLOCK!";
				}
				else
				{
					s = "AXIS COULDN'T BEAT THE CLOCK!";
				}
			}
		}
	}
	else
	{
		s   = CG_ConfigString(CS_MULTI_MAPWINNER);
		buf = Info_ValueForKey(s, "w");

		if (atoi(buf) == -1)
		{
			// neutral
			s = "It's a TIE!";
		}
		else if (atoi(buf))
		{
			// allies
			s = "Allies Win!";
		}
		else
		{
			// axis
			s = "Axis Win!";
		}
	}

	//w = CG_Text_Width_Ext(s, button->font->scalex, 0, button->font->font);
	x = button->rect.x + 4; // + ((button->rect.w - w) * 0.5f);

	CG_Text_Paint_Ext(x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, s, 0, 0, 0, button->font->font);

	s = va("%i seconds to next map", MAX(60 - (cg.time - cgs.intermissionStartTime) / 1000, 0));
	w = CG_Text_Width_Ext(s, button->font->scalex, 0, button->font->font);
	x = button->rect.x + button->rect.w - w - 4;

	CG_Text_Paint_Ext(x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, s, 0, 0, 0, button->font->font);
}

/**
 * @brief CG_DebriefingXPHeader_Draw
 * @param[in] button
 */
void CG_DebriefingXPHeader_Draw(panel_button_t *button)
{
	if (cgs.gametype == GT_WOLF_LMS)
	{
		BG_PanelButtonsRender_TextExt(button, "Score");
	}
	else
	{
		BG_PanelButtonsRender_TextExt(button, "XP");
	}
}

static vec4_t clrSelectedClient = { 1.f, 1.f, 1.f, 0.3f };

/**
 * @brief CG_DebriefingPlayerList_Draw
 * @param[in] button
 */
void CG_DebriefingPlayerList_Draw(panel_button_t *button)
{
	int          i, j;
	float        y      = button->rect.y + 12;
	score_t      *score = NULL;
	clientInfo_t *ci;

	for (i = 0; i + cgs.dbPlayerListOffset < cgs.maxclients && i < 24; i++)
	{
		ci = &cgs.clientinfo[cgs.dbSortedClients[i + cgs.dbPlayerListOffset]];

		if (!ci->infoValid)
		{
			break;
		}

		for (j = 0; j < cgs.maxclients; j++)
		{
			if (cg.scores[j].client == cgs.dbSortedClients[i + cgs.dbPlayerListOffset])
			{
				score = &cg.scores[j];
				break;
			}
		}
		if (j == cgs.maxclients)
		{
			continue;
		}

		if (cgs.dbSelectedClient == cgs.dbSortedClients[i + cgs.dbPlayerListOffset])
		{
			CG_FillRect(button->rect.x, y - 10, SCREEN_WIDTH - 10 - 8 - 16 - button->rect.x + cgs.wideXoffset, 12, clrSelectedClient);
		}

		CG_Text_Paint_Ext(DB_RANK_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, CG_Debriefing_RankNameForClientInfo(ci), 0, 0, 0, button->font->font);

		CG_Text_Paint_Ext(DB_NAME_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, colorWhite, ci->name, 0, 28, 0, button->font->font);

		CG_Text_Paint_Ext(DB_TIME_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", score ? score->time : 0), 0, 0, 0, button->font->font);

		CG_Text_Paint_Ext(DB_XP_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", ci->score), 0, 0, 0, button->font->font);

		if (cgs.dbPlayerKillsDeathsReceived)
		{
			CG_Text_Paint_Ext(DB_KILLS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", ci->kills), 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_DEATHS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", ci->deaths), 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_GIBS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", ci->gibs), 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_SELFKILLS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", ci->selfKills), 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_TEAMKILLS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", ci->teamKills), 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_TEAMGIBS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", ci->teamGibs), 0, 0, 0, button->font->font);
		}
		else
		{
			CG_Text_Paint_Ext(DB_KILLS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, "-", 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_DEATHS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, "-", 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_GIBS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, "-", 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_SELFKILLS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, "-", 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_TEAMKILLS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, "-", 0, 0, 0, button->font->font);
			CG_Text_Paint_Ext(DB_TEAMGIBS_X + cgs.wideXoffset, y, button->font->scalex, button->font->scaley, button->font->colour, "-", 0, 0, 0, button->font->font);
		}

		y += 12;
	}
}

/**
 * @brief CG_SortPlayersByXP
 * @param[in] a
 * @param[in] b
 * @return
 */
int QDECL CG_SortPlayersByXP(const void *a, const void *b)
{
	const int ca = *(const int *)a;
	const int cb = *(const int *)b;

	if (!cgs.clientinfo[cb].infoValid)
	{
		return -1;
	}
	if (!cgs.clientinfo[ca].infoValid)
	{
		return 1;
	}

	if (cgs.clientinfo[cb].score > cgs.clientinfo[ca].score)
	{
		return 1;
	}
	if (cgs.clientinfo[ca].score > cgs.clientinfo[cb].score)
	{
		return -1;
	}

	return 0;
}

/**
 * @brief CG_Debriefing_FullRankNameForClientInfo
 * @param[in] ci
 * @return
 */
const char *CG_Debriefing_FullRankNameForClientInfo(clientInfo_t *ci)
{
	if (ci->team != TEAM_AXIS && ci->team != TEAM_ALLIES)
	{
		return (ci->shoutcaster) ? "Shoutcaster" : "Spectator";
	}

	return GetRankTableData(ci->team, ci->rank)->names;
}

/**
 * @brief CG_Debriefing_RankNameForClientInfo
 * @param[in] ci
 * @return
 */
const char *CG_Debriefing_RankNameForClientInfo(clientInfo_t *ci)
{
	if (ci->team != TEAM_AXIS && ci->team != TEAM_ALLIES)
	{
		return (ci->shoutcaster) ? "Shc" : "Spc";
	}

	return GetRankTableData(ci->team, ci->rank)->miniNames;
}

/**
 * @brief CG_Debriefing_ParseWeaponAccuracies
 */
void CG_Debriefing_ParseWeaponAccuracies(void)
{
	int i;

	for (i = 0; i < cgs.maxclients; i++)
	{
		cgs.clientinfo[i].totalWeapAcc   = (float)atof(CG_Argv(i * 2 + 1));
		cgs.clientinfo[i].totalWeapHSpct = (float)atof(CG_Argv(i * 2 + 2));
	}
	cgs.dbAccuraciesReceived = qtrue;
}

/**
 * @brief CG_Debriefing_ParsePlayerTime
 */
void CG_Debriefing_ParsePlayerTime(void)
{
	int i;

	for (i = 0; i < cgs.maxclients; i++)
	{
		cgs.clientinfo[i].timeAxis   = Q_atoi(CG_Argv(i * 3 + 1));
		cgs.clientinfo[i].timeAllies = Q_atoi(CG_Argv(i * 3 + 2));
		cgs.clientinfo[i].timePlayed = Q_atoi(CG_Argv(i * 3 + 3));
	}
	cgs.dbPlayerTimeReceived = qtrue;
}

#ifdef FEATURE_RATING
/**
 * @brief CG_Debriefing_ParseSkillRating
 */
void CG_Debriefing_ParseSkillRating(void)
{
	int i;

	for (i = 0; i < cgs.maxclients; i++)
	{
		cgs.clientinfo[i].rating      = atof(CG_Argv(i * 2 + 1));
		cgs.clientinfo[i].deltaRating = atof(CG_Argv(i * 2 + 2));
	}
	cgs.dbSkillRatingReceived = qtrue;
}
#endif

#ifdef FEATURE_PRESTIGE
/**
 * @brief CG_Debriefing_ParsePrestige
 */
void CG_Debriefing_ParsePrestige(void)
{
	int i;

	for (i = 0; i < cgs.maxclients; i++)
	{
		cgs.clientinfo[i].prestige = Q_atoi(CG_Argv(i + 1));
	}
	cgs.dbPrestigeReceived = qtrue;
}
#endif

/**
 * @brief CG_Debriefing_ParsePlayerKillsDeaths
 */
void CG_Debriefing_ParsePlayerKillsDeaths(void)
{
	int i;

	for (i = 0; i < cgs.maxclients; i++)
	{
		cgs.clientinfo[i].kills     = Q_atoi(CG_Argv((i * 6) + 1));
		cgs.clientinfo[i].deaths    = Q_atoi(CG_Argv((i * 6) + 2));
		cgs.clientinfo[i].gibs      = Q_atoi(CG_Argv((i * 6) + 3));
		cgs.clientinfo[i].selfKills = Q_atoi(CG_Argv((i * 6) + 4));
		cgs.clientinfo[i].teamKills = Q_atoi(CG_Argv((i * 6) + 5));
		cgs.clientinfo[i].teamGibs  = Q_atoi(CG_Argv((i * 6) + 6));
	}
	cgs.dbPlayerKillsDeathsReceived = qtrue;
}

/**
 * @brief CG_Debriefing_ParseWeaponStats
 */
void CG_Debriefing_ParseWeaponStats(void)
{
	int i;

	cgs.dbHitRegions[HR_HEAD] = Q_atoi(CG_Argv(1));
	cgs.dbHitRegions[HR_ARMS] = Q_atoi(CG_Argv(2));
	cgs.dbHitRegions[HR_BODY] = Q_atoi(CG_Argv(3));
	cgs.dbHitRegions[HR_LEGS] = Q_atoi(CG_Argv(4));

	for (i = 0; i < WS_MAX; i++)
	{
		cgs.dbWeaponStats[i].numShots = Q_atoi(CG_Argv((i * 3) + 5));
		cgs.dbWeaponStats[i].numHits  = Q_atoi(CG_Argv((i * 3) + 6));
		cgs.dbWeaponStats[i].numKills = Q_atoi(CG_Argv((i * 3) + 7));
	}

	cgs.dbWeaponStatsReceived = qtrue;
}

/**
 * @brief CG_Debriefing_ParseAwards
 */
void CG_Debriefing_ParseAwards(void)
{
	int        i   = 0;
	char       *cs = (char *)CG_ConfigString(CS_ENDGAME_STATS);
	const char *token;
	char       *s;
	size_t     size, len;
	int        clientNum;
	float      value;
	char       buffer[sizeof(cgs.dbAwardNamesBuffer)];

	Q_strncpyz(buffer, cs, sizeof(cgs.dbAwardNamesBuffer));
	cs = buffer;

	s    = cgs.dbAwardNamesBuffer;
	size = sizeof(cgs.dbAwardNamesBuffer);

	for (i = 0; i < NUM_ENDGAME_AWARDS; i++)
	{
		// clientNum
		token     = COM_Parse(&cs);
		clientNum = Q_atoi(token);

		if (clientNum >= 0 && clientNum < MAX_CLIENTS)
		{
			Q_strncpyz(s, va("%s", cgs.clientinfo[clientNum].name), size);
		}
		else
		{
			Q_strncpyz(s, "", size);
		}

		// value
		token = COM_Parse(&cs);
		value = atof(token);

		if (value > 0)
		{
			Q_strcat(s, size, (value == (int)(value)) ? va("^7 (%i)", (int)(value)) : va("^7 (%.2f)", value));
		}

		// award
		cgs.dbAwardNames[i] = s;

		len   = strlen(s);
		size -= len;
		s    += len + 1;

		// team
		token               = COM_Parse(&cs);
		cgs.dbAwardTeams[i] = (team_t)atoi(token);
	}

	cgs.dbAwardsParsed = qtrue;
}

/**
 * @brief CG_Debriefing_ScrollGetMax
 * @param[in] button
 * @return
 */
int CG_Debriefing_ScrollGetMax(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 0:     // player list
		return 24;
	case 1:     // weapon stats
		return 7;
	case 2:     // campaign
		return 7;
	case 3:     // mapvote
		return 17;
	case 4:     // awards
		return NUMSHOW_ENDGAME_AWARDS;
	}
	return 0;
}

/**
 * @brief CG_Debriefing_ScrollGetCount
 * @param[in] button
 * @return
 */
int CG_Debriefing_ScrollGetCount(panel_button_t *button)
{
	int i, cnt = 0;

	switch (button->data[0])
	{
	case 0:    // player list
		for (i = 0; i < cgs.maxclients; i++)
		{
			if (!cgs.clientinfo[cgs.dbSortedClients[i]].infoValid)
			{
				return i;
			}
		}
		return cgs.maxclients;
	case 1:    // weapon stats
	{
		if (!cgs.dbWeaponStatsReceived)
		{
			return 0;
		}
		for (i = 0; i < WS_MAX; i++)
		{
			if (cgs.dbWeaponStats[i].numShots)
			{
				cnt++;
			}
		}
		return cnt;
	}
	case 2:    // campaign
		if (cgs.campaignInfoLoaded)
		{
			return cgs.campaignData.mapCount;
		}
		return 0;
	case 3:    // mapvote
		return cgs.dbNumMaps;
	case 4:    // awards
		if (!cgs.dbAwardsParsed)
		{
			CG_Debriefing_ParseAwards();
		}
		for (i = 0; i < NUM_ENDGAME_AWARDS; i++)
		{
			if (cgs.dbAwardTeams[i] != TEAM_FREE)
			{
				cnt++;
			}
		}
		return cnt;
	}
	return 0;
}

/**
 * @brief CG_Debriefing_ScrollGetOffset
 * @param[in] button
 * @return
 */
int CG_Debriefing_ScrollGetOffset(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 0:    // player list
		return cgs.dbPlayerListOffset;
	case 1:    // weapon stats
		return cgs.dbWeaponListOffset;
	case 2:    // campaign
		return cgs.tdbMapListOffset;
	case 3:    // mapvote
		return cgs.dbMapVoteListOffset;
	case 4:    // awards
		return cgs.dbAwardsListOffset;
	}
	return 0;
}

/**
 * @brief CG_Debriefing_ScrollSetOffset
 * @param[in] button
 * @param[in] ofs
 */
void CG_Debriefing_ScrollSetOffset(panel_button_t *button, int ofs)
{
	switch (button->data[0])
	{
	case 0:
		cgs.dbPlayerListOffset = ofs;
		return;
	case 1:
		cgs.dbWeaponListOffset = ofs;
		return;
	case 2:
		cgs.tdbMapListOffset = ofs;
		return;
	case 3:
		cgs.dbMapVoteListOffset = ofs;
		return;
	case 4:
		cgs.dbAwardsListOffset = ofs;
		return;
	}
}

/**
 * @brief CG_Debriefing_ScrollGetBarRect
 * @param[in] button
 * @param[in,out] r
 */
void CG_Debriefing_ScrollGetBarRect(panel_button_t *button, rectDef_t *r)
{
	int max    = CG_Debriefing_ScrollGetMax(button);
	int cnt    = CG_Debriefing_ScrollGetCount(button);
	int offset = CG_Debriefing_ScrollGetOffset(button);

	if (cnt > max)
	{
		float h = button->rect.h;

		r->h = h * (max / (float)cnt);
		r->y = button->rect.y + (offset / (float)(cnt - max)) * (h - r->h);
	}
	else
	{
		r->h = button->rect.h;
		r->y = button->rect.y;
	}

	r->x = button->rect.x;
	r->w = button->rect.w;
}

/**
 * @brief CG_Debriefing_ScrollCheckOffset
 * @param[in] button
 */
void CG_Debriefing_ScrollCheckOffset(panel_button_t *button)
{
	int max    = CG_Debriefing_ScrollGetMax(button);
	int cnt    = CG_Debriefing_ScrollGetCount(button);
	int offset = CG_Debriefing_ScrollGetOffset(button);
	int maxofs = MAX(0, cnt - max);

	if (offset > maxofs)
	{
		CG_Debriefing_ScrollSetOffset(button, maxofs);
	}
	else if (offset < 0)
	{
		CG_Debriefing_ScrollSetOffset(button, 0);
	}
}

/**
 * @brief CG_Debriefing_MouseEvent
 * @param[in] x
 * @param[in] y
 */
void CG_Debriefing_MouseEvent(int x, int y)
{
	panel_button_t *button;

	switch (cgs.dbMode)
	{
	case 1: // awards
	case 2: // campaign
	case 3: // mapvote
		button = BG_PanelButtons_GetFocusButton();
		if (button && button->onDraw == CG_Debriefing_Scrollbar_Draw)
		{
			rectDef_t r;
			int       count, cnt;

			cnt = CG_Debriefing_ScrollGetCount(button);
			CG_Debriefing_ScrollGetBarRect(button, &r);

			button->data[1] += y;

			count = (cnt * button->data[1] * 0.5f) / r.h;
			if (count)
			{
				int ofs = CG_Debriefing_ScrollGetOffset(button);

				CG_Debriefing_ScrollSetOffset(button, ofs + count);
				CG_Debriefing_ScrollCheckOffset(button);
				ofs = CG_Debriefing_ScrollGetOffset(button) - ofs;

				if (ofs == count)
				{
					button->data[1] -= ofs * (r.h / (float)cnt);
				}
			}

			CG_Debriefing_ScrollGetBarRect(button, &r);
			cgs.cursorY = r.y + button->data[2];

			return;
		}
		break;
	default:
		break;
	}

	cgs.cursorX += x;
	if (cgs.cursorX < 0)
	{
		cgs.cursorX = 0;
	}
	else if (cgs.cursorX > SCREEN_WIDTH)
	{
		cgs.cursorX = SCREEN_WIDTH;
	}

	cgs.cursorY += y;
	if (cgs.cursorY < 0)
	{
		cgs.cursorY = 0;
	}
	else if (cgs.cursorY > SCREEN_HEIGHT)
	{
		cgs.cursorY = SCREEN_HEIGHT;
	}
}

/**
 * @brief CG_Debriefing_Scrollbar_Draw
 * @param[in] button
 */
void CG_Debriefing_Scrollbar_Draw(panel_button_t *button)
{
	vec4_t    clr1 = { .16f, .2f, .17f, .8f };
	vec4_t    clr2 = { 0.f, 0.f, 0.f, .6f };
	rectDef_t r;

	CG_Debriefing_ScrollCheckOffset(button);

	CG_FillRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, clr2);
	CG_DrawRect_FixedBorder(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1, colorMdGrey);

	CG_Debriefing_ScrollGetBarRect(button, &r);

	CG_FillRect(r.x, r.y, r.w, r.h, clr1);
	CG_DrawRect_FixedBorder(r.x, r.y, r.w, r.h, 1, colorMdGrey);
}

/**
 * @brief CG_Debriefing_Scrollbar_KeyDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_Debriefing_Scrollbar_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		rectDef_t r;
		CG_Debriefing_ScrollGetBarRect(button, &r);
		if (BG_CursorInRect(&r))
		{
			BG_PanelButtons_SetFocusButton(button);
			button->data[1] = 0;
			button->data[2] = cgs.cursorY - r.y;
		}
	}

	return qfalse;
}

/**
 * @brief CG_Debriefing_Scrollbar_KeyUp
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_Debriefing_Scrollbar_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (BG_PanelButtons_GetFocusButton() == button)
		{
			BG_PanelButtons_SetFocusButton(NULL);
		}
	}
	return qfalse;
}

/**
 * @brief CG_Debriefing_KeyEvent
 * @param[in] key
 * @param[in] down
 */
void CG_Debriefing_KeyEvent(int key, qboolean down)
{
	switch (cgs.dbMode)
	{
	case 0:  // players list
		break;
	case 1:  // awards
		if (BG_PanelButtonsKeyEvent(key, down, teamDebriefPanelButtons))
		{
			return;
		}
		break;
	case 2:  // campaign
		if (BG_PanelButtonsKeyEvent(key, down, debriefPanelButtons))
		{
			return;
		}
		break;
	case 3: // mapvote
		if (BG_PanelButtonsKeyEvent(key, down, mapVoteButtons))
		{
			return;
		}
		break;
	}

	if (BG_PanelButtonsKeyEvent(key, down, chatPanelButtons))
	{
		return;
	}

	if (!BG_PanelButtons_GetFocusButton() && down && key != K_MOUSE1)
	{
		BG_PanelButtons_SetFocusButton(&charPanelEdit);
		BG_PanelButton_EditClick(&charPanelEdit, key);
		BG_PanelButtons_SetFocusButton(NULL);
	}
}

/**
 * @brief CG_Debriefing_PlayerSkills_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerSkills_Draw(panel_button_t *button)
{
	clientInfo_t *ci;
	int          i;
	float        x;

	ci = CG_Debriefing_GetSelectedClientInfo();

	x = button->rect.x;
	CG_DrawPic(x, button->rect.y, button->rect.w, button->rect.h, cgs.media.skillPics[button->data[0]]);

	x += button->rect.w + 8;
	for (i = ci->skill[button->data[0]]; i > 0; i--)
	{
		CG_DrawPicST(x, button->rect.y, button->rect.w, button->rect.h, 0, 0, 1.f, 0.5f, cgs.media.limboStar_roll);

		x += button->rect.w + 2;
	}

	{
		vec4_t clr = { 1.f, 1.f, 1.f, 0.2f };

		trap_R_SetColor(clr);
		for (i = ci->skill[button->data[0]]; i < 4; i++)
		{
			CG_DrawPicST(x, button->rect.y, button->rect.w, button->rect.h, 0, 0, 1.f, 0.5f, cgs.media.limboStar_roll);

			x += button->rect.w + 2;
		}
		trap_R_SetColor(NULL);
	}
}

static qhandle_t img;
static qhandle_t imgH;
static qhandle_t imgA;
static qhandle_t imgB;
static qhandle_t imgL;

/**
 * @brief
 */
void CG_Debriefing_PlayerHitRegions_Draw(panel_button_t *button)
{
	int    totalHits = cgs.dbHitRegions[HR_HEAD] + cgs.dbHitRegions[HR_ARMS] + cgs.dbHitRegions[HR_BODY] + cgs.dbHitRegions[HR_LEGS];
	float  hitsHead  = (totalHits && cgs.dbHitRegions[HR_HEAD]) ? (cgs.dbHitRegions[HR_HEAD] / (float)totalHits) : 0.0f;
	float  hitsArms  = (totalHits && cgs.dbHitRegions[HR_ARMS]) ? (cgs.dbHitRegions[HR_ARMS] / (float)totalHits) : 0.0f;
	float  hitsBody  = (totalHits && cgs.dbHitRegions[HR_BODY]) ? (cgs.dbHitRegions[HR_BODY] / (float)totalHits) : 0.0f;
	float  hitsLegs  = (totalHits && cgs.dbHitRegions[HR_LEGS]) ? (cgs.dbHitRegions[HR_LEGS] / (float)totalHits) : 0.0f;
	float  alphaH    = hitsHead > 0.f ? (hitsHead * 0.8f) + 0.2f : 0.0f;
	float  alphaA    = hitsArms > 0.f ? (hitsArms * 0.8f) + 0.2f : 0.0f;
	float  alphaB    = hitsBody > 0.f ? (hitsBody * 0.8f) + 0.2f : 0.0f;
	float  alphaL    = hitsLegs > 0.f ? (hitsLegs * 0.8f) + 0.2f : 0.0f;
	float  w;
	vec4_t colorH, colorA, colorB, colorL;

#ifdef FEATURE_PRESTIGE
	int i, j, cnt = 0, skillMax;

	// hide if we need to display prestige collection note
	if (cgs.prestige && cgs.dbSelectedClient == cg.clientNum &&
	    cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS && cgs.gametype != GT_WOLF_CAMPAIGN)
	{
		// count the number of maxed out skills
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			skillMax = 0;

			// check skill max level
			for (j = NUM_SKILL_LEVELS - 1; j >= 0; j--)
			{
				if (GetSkillTableData(i)->skillLevels[j] >= 0)
				{
					skillMax = j;
					break;
				}
			}

			if (cgs.clientinfo[cg.clientNum].skill[i] >= skillMax)
			{
				cnt++;
			}
		}

		if (cnt >= SK_NUM_SKILLS)
		{
			return;
		}
	}
#endif

	w = CG_Text_Width_Ext("Head: ", button->font->scalex, 0, button->font->font);

	// register hitregions only once (when required)
	if (img == 0)
	{
		img = trap_R_RegisterShaderNoMip("gfx/misc/hitregions.tga");
	}
	if (imgH == 0)
	{
		imgH = trap_R_RegisterShaderNoMip("gfx/misc/hitregion_head.tga");
	}
	if (imgA == 0)
	{
		imgA = trap_R_RegisterShaderNoMip("gfx/misc/hitregion_arms.tga");
	}
	if (imgB == 0)
	{
		imgB = trap_R_RegisterShaderNoMip("gfx/misc/hitregion_body.tga");
	}
	if (imgL == 0)
	{
		imgL = trap_R_RegisterShaderNoMip("gfx/misc/hitregion_legs.tga");
	}

	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y + 8, button->font->scalex, button->font->scaley, button->font->colour, "Region Hits:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y + 2 * 12, button->font->scalex, button->font->scaley, button->font->colour, "Head:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + 2 * 12, button->font->scalex, button->font->scaley, button->font->colour, va("%2.0f%%", hitsHead * 100), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

	w = CG_Text_Width_Ext("Arms: ", button->font->scalex, 0, button->font->font);
	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y + 3 * 12, button->font->scalex, button->font->scaley, button->font->colour, "Arms:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + 3 * 12, button->font->scalex, button->font->scaley, button->font->colour, va("%2.0f%%", hitsArms * 100), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

	w = CG_Text_Width_Ext("Body: ", button->font->scalex, 0, button->font->font);
	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y + 4 * 12, button->font->scalex, button->font->scaley, button->font->colour, "Body:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + 4 * 12, button->font->scalex, button->font->scaley, button->font->colour, va("%2.0f%%", hitsBody * 100), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

	w = CG_Text_Width_Ext("Legs: ", button->font->scalex, 0, button->font->font);
	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y + 5 * 12, button->font->scalex, button->font->scaley, button->font->colour, "Legs:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + 5 * 12, button->font->scalex, button->font->scaley, button->font->colour, va("%2.0f%%", hitsLegs * 100), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

	// draw the image of a puppet, indicating red++ colors for higher hit-ratios per region
	CG_DrawPic(button->rect.x + 4, button->rect.y + 12, 54, 54, img);

	if (alphaH)
	{
		Vector4Set(colorH, 1.0f, 0.f, 0.f, alphaH);
		trap_R_SetColor(colorH);
		CG_DrawPic(button->rect.x + 4, button->rect.y + 12, 54, 54, imgH);
		trap_R_SetColor(NULL);
	}

	if (alphaA)
	{
		Vector4Set(colorA, 1.0f, 0.f, 0.f, alphaA);
		trap_R_SetColor(colorA);
		CG_DrawPic(button->rect.x + 4, button->rect.y + 12, 54, 54, imgA);
		trap_R_SetColor(NULL);
	}

	if (alphaB)
	{
		Vector4Set(colorB, 1.0f, 0.f, 0.f, alphaB);
		trap_R_SetColor(colorB);
		CG_DrawPic(button->rect.x + 4, button->rect.y + 12, 54, 54, imgB);
		trap_R_SetColor(NULL);
	}

	if (alphaL)
	{
		Vector4Set(colorL, 1.0f, 0.f, 0.f, alphaL);
		trap_R_SetColor(colorL);
		CG_DrawPic(button->rect.x + 4, button->rect.y + 12, 54, 54, imgL);
		trap_R_SetColor(NULL);
	}
}

#ifdef FEATURE_PRESTIGE
/**
 * @brief CG_Debriefing_PlayerPrestige_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerPrestige_Draw(panel_button_t *button)
{
	clientInfo_t *ci;
	float        w;

	if (!cgs.prestige || cgs.gametype == GT_WOLF_STOPWATCH || cgs.gametype == GT_WOLF_LMS || cgs.gametype == GT_WOLF_CAMPAIGN)
	{
		return;
	}

	ci = CG_Debriefing_GetSelectedClientInfo();
	w  = CG_Text_Width_Ext("Prestige: ", button->font->scalex, 0, button->font->font);

	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString("Prestige:"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, va("^2%i", ci->prestige), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
}

/**
 * @brief CG_Debriefing_PlayerPrestige_Note
 * @param[in] button
 */
void CG_Debriefing_PlayerPrestige_Note(panel_button_t *button)
{
	int i, j, cnt = 0, skillMax, h;

	if (!cgs.prestige || cgs.gametype == GT_WOLF_STOPWATCH || cgs.gametype == GT_WOLF_LMS || cgs.gametype == GT_WOLF_CAMPAIGN)
	{
		return;
	}

	if (cgs.dbSelectedClient != cg.clientNum)
	{
		return;
	}

	// count the number of maxed out skills
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		skillMax = 0;

		// check skill max level
		for (j = NUM_SKILL_LEVELS - 1; j >= 0; j--)
		{
			if (GetSkillTableData(i)->skillLevels[j] >= 0)
			{
				skillMax = j;
				break;
			}
		}

		if (cgs.clientinfo[cg.clientNum].skill[i] >= skillMax)
		{
			cnt++;
		}
	}

	if (cnt < SK_NUM_SKILLS)
	{
		return;
	}

	h = CG_Text_Height_Ext("A", button->font->scalex, 0, button->font->font);

	CG_DrawMultilineText(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour,
	                     CG_TranslateString("You may now collect\na prestige point.\n\nCollection resets\nskill levels."),
	                     2 * h, 0, 0, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, button->font->font);
}
#endif

/**
 * @brief CG_Debriefing_PlayerACC_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerACC_Draw(panel_button_t *button)
{
	clientInfo_t *ci;
	float        w;

	ci = CG_Debriefing_GetSelectedClientInfo();
	w  = CG_Text_Width_Ext("ACC: ", button->font->scalex, 0, button->font->font);

	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString("ACC:"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, va("%.2f%%", ci->totalWeapAcc), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
}

/**
 * @brief CG_Debriefing_PlayerHS_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerHS_Draw(panel_button_t *button)
{
	clientInfo_t *ci;
	float        w;

	ci = CG_Debriefing_GetSelectedClientInfo();
	w  = CG_Text_Width_Ext("HS: ", button->font->scalex, 0, button->font->font);

	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString("HS:"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, va("%.2f%%", ci->totalWeapHSpct), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
}

/**
 * @brief CG_Debriefing_PlayerXP_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerXP_Draw(panel_button_t *button)
{
	clientInfo_t *ci;
	float        w;

	ci = CG_Debriefing_GetSelectedClientInfo();
	w  = CG_Text_Width_Ext("XP: ", button->font->scalex, 0, button->font->font);

	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, "XP:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", ci->score), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
}

#ifdef FEATURE_RATING
/**
 * @brief CG_Debriefing_PlayerSR_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerSR_Draw(panel_button_t *button)
{
	if (cgs.skillRating && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
	{
		clientInfo_t *ci;
		float        w;

		ci = CG_Debriefing_GetSelectedClientInfo();
		w  = CG_Text_Width_Ext("SR: ", button->font->scalex, 0, button->font->font);

		CG_Text_Paint_Ext(button->rect.x - w, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString("SR:"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

		CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, va("%.2f ^5%+.2f^9", ci->rating, ci->deltaRating), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	}
}
#endif

/**
 * @brief CG_Debriefing_PlayerTime_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerTime_Draw(panel_button_t *button)
{
	clientInfo_t *ci;
	score_t      *score = NULL;
	int          i;
	float        w;

	ci = CG_Debriefing_GetSelectedClientInfo();

	for (i = 0; i < cgs.maxclients; i++)
	{
		if (cg.scores[i].client == cgs.dbSelectedClient)
		{
			score = &cg.scores[i];
			break;
		}
	}
	if (!score)
	{
		return;
	}

	w = CG_Text_Width_Ext("Time: ", button->font->scalex, 0, button->font->font);
	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString("Time:"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, va("%i^9/^1%i^9/^$%i^9  %.0f%% played", score->time, ci->timeAxis / 60000, ci->timeAllies / 60000, (ci->timeAxis + ci->timeAllies) > 0 ? 100.f * ci->timePlayed / (ci->timeAxis + ci->timeAllies) : 0), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
}

/**
 * @brief CG_Debriefing_PlayerMedals_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerMedals_Draw(panel_button_t *button)
{
	clientInfo_t *ci;
	float        w, x;
	int          i;

	ci = CG_Debriefing_GetSelectedClientInfo();
	w  = CG_Text_Width_Ext("Medals: ", button->font->scalex, 0, button->font->font);

	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString("Medals:"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

	x = button->rect.x;
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		if (ci->medals[i])
		{
			CG_DrawPic(x, button->rect.y - 10, 16, 16, cgs.media.medals[i]);

			x += 16 + 2;
		}
	}
}

/**
 * @brief CG_Debriefing_PlayerRank_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerRank_Draw(panel_button_t *button)
{
	clientInfo_t *ci;
	float        w;

	ci = CG_Debriefing_GetSelectedClientInfo();
	w  = CG_Text_Width_Ext("Rank: ", button->font->scalex, 0, button->font->font);

	CG_Text_Paint_Ext(button->rect.x - w, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString("Rank:"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);

	if (ci->rank > 0 && ci->team != TEAM_SPECTATOR)
	{
		CG_DrawPic(button->rect.x, button->rect.y - 12, 16, 16, rankicons[ci->rank][ci->team == TEAM_AXIS ? 1 : 0][0].shader);
		CG_Text_Paint_Ext(button->rect.x + 18, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_Debriefing_FullRankNameForClientInfo(ci), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	}
	else
	{
		CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_Debriefing_FullRankNameForClientInfo(ci), 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
	}
}

/**
 * @brief CG_Debriefing_PlayerName_Draw
 * @param[in] button
 */
void CG_Debriefing_PlayerName_Draw(panel_button_t *button)
{
	clientInfo_t *ci;

	ci = CG_Debriefing_GetSelectedClientInfo();

	switch (ci->team)
	{
	case TEAM_AXIS:
		CG_DrawPic(button->rect.x, button->rect.y - 9, 18, 12, cgs.media.axisFlag);
		break;
	case TEAM_ALLIES:
		CG_DrawPic(button->rect.x, button->rect.y - 9, 18, 12, cgs.media.alliedFlag);
		break;
	case TEAM_SPECTATOR: // fall through
	default:
		CG_DrawPic(button->rect.x, button->rect.y - 9, 18, 12, cgs.media.limboTeamButtonBack_on);
		CG_DrawPic(button->rect.x, button->rect.y - 9, 18, 12, cgs.media.limboTeamButtonSpec);
		break;
	}

	if (ci->team == TEAM_AXIS || ci->team == TEAM_ALLIES)
	{
		CG_DrawPic(button->rect.x, button->rect.y - 9, 18, 12, ci->team == TEAM_AXIS ? cgs.media.axisFlag : cgs.media.alliedFlag);
	}
	CG_Text_Paint_Ext(button->rect.x + 22, button->rect.y, button->font->scalex, button->font->scaley, colorWhite, ci->name, 0, 0, ITEM_TEXTSTYLE_SHADOWED, button->font->font);
}

/**
 * @brief CG_Debriefing_GetSelectedClientInfo
 * @return
 */
clientInfo_t *CG_Debriefing_GetSelectedClientInfo(void)
{
	clientInfo_t *ci;

	if (cgs.dbSelectedClient < 0 || cgs.dbSelectedClient >= cgs.maxclients)
	{
		CG_Debrieing_SetSelectedClient(cg.clientNum);
	}

	ci = &cgs.clientinfo[cgs.dbSelectedClient];
	if (!ci->infoValid)
	{
		CG_Debrieing_SetSelectedClient(cg.clientNum);
		ci = &cgs.clientinfo[cgs.dbSelectedClient];
	}

	return ci;
}

/**
 * @brief CG_Debrieing_SetSelectedClient
 * @param[in] clientNum
 */
void CG_Debrieing_SetSelectedClient(int clientNum)
{
	if (clientNum < 0 || clientNum >= cgs.maxclients)
	{
		return;
	}

	if (clientNum != cgs.dbSelectedClient)
	{
		cgs.dbSelectedClient      = clientNum;
		cgs.dbWeaponStatsReceived = qfalse;
	}
}

/**
 * @brief CG_Debriefing_ChatButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_Debriefing_ChatButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		cgs.dbChatMode = (cgs.dbChatMode + 1) % 3;

		if (cgs.dbChatMode > 0)
		{
			if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
			{
				cgs.dbChatMode = 0;
			}

			if (cgs.dbChatMode > 1)
			{
				if (!CG_IsOnFireteam(cg.clientNum))
				{
					cgs.dbChatMode = 0;
				}
			}
		}

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_Debriefing_ReadyButton_Draw
 * @param[in] button
 */
void CG_Debriefing_ReadyButton_Draw(panel_button_t *button)
{
	int timeleft = MAX(60 - (cg.time - cgs.intermissionStartTime) / 1000, 0);
	button->text = va("READY (%i:%02i)", timeleft / 60, timeleft % 60);

	if (!cg.snap)
	{
		return;
	}

	// keep on showing the timer when already pressed, or while being
	// on game type map voting
	if (cg.snap->ps.eFlags & EF_READY || cgs.gametype == GT_WOLF_MAPVOTE)
	{
		button->text = va("(%i:%02i)", timeleft / 60, timeleft % 60);
	}

	CG_PanelButtonsRender_Button(button);
}

/**
 * @brief CG_Debriefing_ChatButton_Draw
 * @param[in] button
 */
void CG_Debriefing_ChatButton_Draw(panel_button_t *button)
{
	const char *str;

	switch (cgs.dbChatMode)
	{
	case 1:
		str = (CG_TranslateString("^5TO TEAM"));
		break;
	case 2:
		str = (CG_TranslateString("^3TO FIRETEAM"));
		break;
	default:
		str = (CG_TranslateString("^2TO GLOBAL"));
		break;
	}

	CG_PanelButtonsRender_Button_Ext(&button->rect, str);
}

void CG_QuickMessage_f(void);

/**
 * @brief CG_Debriefing_ReadyButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_Debriefing_ReadyButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (!cg.snap)
		{
			return qfalse;
		}

		if (cg.snap->ps.eFlags & EF_READY)
		{
			return qfalse;
		}

		// disable the ready button so players don't interrupt intermission
		if (cgs.gametype == GT_WOLF_MAPVOTE)
		{
			return qfalse;
		}

		trap_SendClientCommand("imready");

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_Debriefing_QCButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_Debriefing_QCButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		CG_QuickMessage_f();
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief CG_Debriefing_VoteButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_Debriefing_VoteButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		cgs.dbMode = 3;

		// failsafe
		if (cgs.gametype != GT_WOLF_MAPVOTE && cgs.dbMode == 3)
		{
			cgs.dbMode = 0;
		}

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_Debriefing_NextButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_Debriefing_NextButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		cgs.dbMode = (cgs.dbMode + 1) % 4;

		if (cgs.gametype != GT_WOLF_MAPVOTE && cgs.dbMode == 3)
		{
			cgs.dbMode = 0;
		}

		return qtrue;
	}

	return qfalse;
}

#ifdef FEATURE_PRESTIGE
/**
 * @brief CG_Debriefing_PrestigeButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_Debriefing_PrestigeButton_KeyDown(panel_button_t *button, int key)
{
	int i, j, skillMax, cnt = 0;

	if (key == K_MOUSE1)
	{
		if (!cg.snap)
		{
			return qfalse;
		}

		// count the number of maxed out skills
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			skillMax = 0;

			// check skill max level
			for (j = NUM_SKILL_LEVELS - 1; j >= 0; j--)
			{
				if (GetSkillTableData(i)->skillLevels[j] >= 0)
				{
					skillMax = j;
					break;
				}
			}

			if (cgs.clientinfo[cg.clientNum].skill[i] >= skillMax)
			{
				cnt++;
			}
		}

		if (cnt < SK_NUM_SKILLS)
		{
			return qfalse;
		}

		trap_SendClientCommand("imcollectpr");

		// refresh data
		cgs.dbPrestigeReceived = qfalse;

		// refresh value display immediately to hide delayed effect
		cgs.clientinfo[cg.clientNum].prestige += 1;

		// reset skills client side only to keep current XPs value display
		// server sync will happen at next map
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			cgs.clientinfo[cg.clientNum].skill[i] = 0;
		}

		return qtrue;
	}

	return qfalse;
}
#endif

/**
 * @brief CG_Debriefing_VoteButton_Draw
 * @param[in] button
 */
void CG_Debriefing_VoteButton_Draw(panel_button_t *button)
{
	if (cgs.gametype != GT_WOLF_MAPVOTE)
	{
		return;
	}

	if (!(cg.snap->ps.eFlags & EF_VOTED))
	{
		return;
	}

	CG_PanelButtonsRender_Button(button);
}

/**
 * @brief CG_Debriefing_VoteNowButton_Draw
 * @param[in] button
 */
void CG_Debriefing_VoteNowButton_Draw(panel_button_t *button)
{
	if (cgs.gametype != GT_WOLF_MAPVOTE)
	{
		return;
	}

	if (cg.snap->ps.eFlags & EF_VOTED)
	{
		return;
	}

	CG_PanelButtonsRender_Button(button);
}

/**
 * @brief CG_Debriefing_NextButton_Draw
 * @param[in] button
 */
void CG_Debriefing_NextButton_Draw(panel_button_t *button)
{
	CG_PanelButtonsRender_Button(button);
}

#ifdef FEATURE_PRESTIGE
/**
 * @brief CG_Debriefing_PrestigeButton_Draw
 * @param[in] button
 */
void CG_Debriefing_PrestigeButton_Draw(panel_button_t *button)
{
	int i, j, cnt = 0, skillMax;

	if (cgs.gametype == GT_WOLF_CAMPAIGN || cgs.gametype == GT_WOLF_STOPWATCH || cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (!cgs.prestige)
	{
		return;
	}

	if (cgs.dbSelectedClient != cg.clientNum)
	{
		return;
	}

	// count the number of maxed out skills
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		skillMax = 0;

		// check skill max level
		for (j = NUM_SKILL_LEVELS - 1; j >= 0; j--)
		{
			if (GetSkillTableData(i)->skillLevels[j] >= 0)
			{
				skillMax = j;
				break;
			}
		}

		if (cgs.clientinfo[cg.clientNum].skill[i] >= skillMax)
		{
			cnt++;
		}
	}

	if (cnt < SK_NUM_SKILLS)
	{
		return;
	}

	CG_PanelButtonsRender_Button(button);
}
#endif

/**
 * @brief CG_Debriefing_ChatEditFinish
 * @param[in] button
 */
void CG_Debriefing_ChatEditFinish(panel_button_t *button)
{
	char buffer[MAX_EDITFIELD];
	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	Q_EscapeUnicodeInPlace(buffer, MAX_EDITFIELD);

	switch (cgs.dbChatMode)
	{
	case 0:
		trap_SendClientCommand(va("say %s", buffer));
		break;
	case 1:
		trap_SendClientCommand(va("say_team %s", buffer));
		break;
	case 2:
		trap_SendClientCommand(va("say_buddy %s", buffer));
		break;
	}

	button->data[2] = 0;

	trap_Cvar_Set(button->text, "");
}

/**
 * @brief CG_Debriefing_CalcCampaignProgress
 * @return
 *
 * @note Unused
 */
float CG_Debriefing_CalcCampaignProgress(void)
{
	int i;

	if (!cgs.campaignInfoLoaded)
	{
		return 0;
	}

	for (i = 0; i < cgs.campaignData.mapCount; i++)
	{
		if (!Q_stricmp(cgs.campaignData.mapnames[i], cgs.rawmapname))
		{
			return (i + 1) / (float)cgs.campaignData.mapCount;
		}
	}

	return 0;
}

/**
 * @brief CG_TeamDebriefing_CalcXP
 * @param[in] team
 * @param[in] mapindex
 * @param[in] skillindex
 * @return
 */
int CG_TeamDebriefing_CalcXP(team_t team, int mapindex, int skillindex)
{
	int j, cnt = 0;

	if (cg_gameType.integer == GT_WOLF_CAMPAIGN)
	{
		int i;

		for (i = 0; i < cgs.campaignData.mapCount; i++)
		{
			if (mapindex != -1 && i != mapindex)
			{
				continue;
			}

			for (j = 0; j < SK_NUM_SKILLS; j++)
			{
				if (skillindex != -1 && j != skillindex)
				{
					continue;
				}

				cnt += team == TEAM_AXIS ? cgs.tdbAxisMapsXP[j][i] : cgs.tdbAlliedMapsXP[j][i];
			}
		}
	}
	else if (cg_gameType.integer == GT_WOLF || cg_gameType.integer == GT_WOLF_STOPWATCH || cg_gameType.integer == GT_WOLF_MAPVOTE)
	{
		for (j = 0; j < SK_NUM_SKILLS; j++)
		{
			if (skillindex != -1 && j != skillindex)
			{
				continue;
			}

			cnt += team == TEAM_AXIS ? cgs.tdbAxisMapsXP[j][0] : cgs.tdbAlliedMapsXP[j][0];
		}
	}

	return cnt;
}

/**
 * @brief CG_TeamDebriefingTeamSkillXP_Draw
 * @param[in] button
 */
void CG_TeamDebriefingTeamSkillXP_Draw(panel_button_t *button)
{
	team_t team = button->data[0] == 0 ? TEAM_AXIS : TEAM_ALLIES;
	int    xp;

	if (button->data[1] == SK_NUM_SKILLS)
	{
		xp = CG_TeamDebriefing_CalcXP(team, cgs.tdbSelectedMap - 1, -1);
	}
	else
	{
		xp = CG_TeamDebriefing_CalcXP(team, cgs.tdbSelectedMap - 1, button->data[1]);
	}

	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, va("%i", xp), 0, 0, 0, button->font->font);
}

/**
 * @brief CG_PanelButtonsRender_Button_Ext
 * @param[in] r
 * @param[in] text
 */
void CG_PanelButtonsRender_Button_Ext(rectDef_t *r, const char *text)
{
	vec4_t clrBdr = { 0.1f, 0.1f, 0.1f, 0.5f };
	vec4_t clrBck = { 0.3f, 0.3f, 0.3f, 0.4f };

	vec4_t clrBck_hi = { 0.5f, 0.5f, 0.5f, 0.4f };
	vec4_t clrTxt_hi = { 0.9f, 0.9f, 0.9f, 1.f };

	qboolean hilight = BG_CursorInRect(r);

	CG_FillRect(r->x, r->y, r->w, r->h, hilight ? clrBck_hi : clrBck);
	CG_DrawRect_FixedBorder(r->x, r->y, r->w, r->h, 1, clrBdr);

	if (text)
	{
		float w;

		w = CG_Text_Width_Ext(text, 0.2f, 0, &cgs.media.limboFont2);

		CG_Text_Paint_Ext(r->x + ((r->w + 2) - w) * 0.5f, r->y + 11, 0.19f, 0.19f, hilight ? clrTxt_hi : clrTxtBck, text, 0, 0, 0, &cgs.media.limboFont2);
	}
}

/**
 * @brief CG_PanelButtonsRender_Button
 * @param[in] button
 */
void CG_PanelButtonsRender_Button(panel_button_t *button)
{
	CG_PanelButtonsRender_Button_Ext(&button->rect, button->text);
}

/**
 * @brief CG_PanelButtonsRender_Window_Ext
 * @param[in] r
 * @param[in] text
 * @param[in] align
 * @param[in] innerheight
 * @param[in] fontscale
 * @param[in] yofs
 */
void CG_PanelButtonsRender_Window_Ext(rectDef_t *r, const char *text, int align, int innerheight, float fontscale, int yofs)
{
	vec4_t clrBdr      = { 0.5f, 0.5f, 0.5f, 0.5f };
	vec4_t clrTitleBck = { 0.16f, 0.2f, 0.17f, 0.8f };
	vec4_t clrBck      = { 0.0f, 0.0f, 0.0f, 0.8f };

	CG_FillRect(r->x, r->y, r->w, r->h, clrBck);
	CG_DrawRect_FixedBorder(r->x, r->y, r->w, r->h, 1, clrBdr);

	CG_FillRect(r->x + 2, r->y + 2, r->w - 4, innerheight, clrTitleBck);

	if (text)
	{
		float x;

		if (align == ITEM_ALIGN_CENTER)
		{
			float w = CG_Text_Width_Ext(text, fontscale, 0, &cgs.media.limboFont1);
			x = r->x + (r->w - w) * 0.5f;
		}
		else if (align == ITEM_ALIGN_RIGHT)
		{
			float w = CG_Text_Width_Ext(text, fontscale, 0, &cgs.media.limboFont1);
			x = r->x + r->w - w;
		}
		else
		{
			x = r->x + 5;
		}

		CG_Text_Paint_Ext(x, r->y + yofs, fontscale, fontscale, clrTxtBck, text, 0, 0, 0, &cgs.media.limboFont1);
	}
}

/**
 * @brief CG_PanelButtonsRender_Window
 * @param[in] button
 */
void CG_PanelButtonsRender_Window(panel_button_t *button)
{
	CG_PanelButtonsRender_Window_Ext(&button->rect, button->text, button->data[0], 12, 0.19f, 11);
}

/**
 * @brief CG_Debriefing_WinStringForTeam
 * @param[in] team
 * @return
 */
const char *CG_Debriefing_WinStringForTeam(team_t team)
{
	switch (team)
	{
	case TEAM_ALLIES:
		return (CG_TranslateString("ALLIES WIN!"));
	case TEAM_AXIS:
		return (CG_TranslateString("AXIS WIN!"));
	default:
		return (CG_TranslateString("IT'S A TIE!"));
	}
}

/**
 * @brief CG_Debriefing_MissionTitle_Draw
 * @param[in] button
 */
void CG_Debriefing_MissionTitle_Draw(panel_button_t *button)
{
	const char *s;
	float      x, w;
	int        secs;

	if (cg_gameType.integer == GT_WOLF_STOPWATCH)
	{
		int defender, winner;

		s        = CG_ConfigString(CS_MULTI_INFO);
		defender = Q_atoi(Info_ValueForKey(s, "d")); // defender

		s      = CG_ConfigString(CS_MULTI_MAPWINNER);
		winner = Q_atoi(Info_ValueForKey(s, "w"));

		if (cgs.currentRound)
		{
			// first round
			s = va(CG_TranslateString("CLOCK IS NOW SET TO %s!"), CG_Debriefing_TimeToString(cgs.nextTimeLimit * 60000.f)); // 60.f * 1000.f
		}
		else
		{
			// second round
			if (!defender)
			{
				if (winner != defender)
				{
					s = CG_TranslateString("ALLIES SUCCESSFULLY BEAT THE CLOCK!");
				}
				else
				{
					s = CG_TranslateString("ALLIES COULDN'T BEAT THE CLOCK!");
				}
			}
			else
			{
				if (winner != defender)
				{
					s = CG_TranslateString("AXIS SUCCESSFULLY BEAT THE CLOCK!");
				}
				else
				{
					s = CG_TranslateString("AXIS COULDN'T BEAT THE CLOCK!");
				}
			}
		}
		CG_PanelButtonsRender_Window_Ext(&button->rect, s, 0, 18, 0.25f, 16);
	}
	else if (cg_gameType.integer == GT_WOLF_CAMPAIGN)
	{
		CG_PanelButtonsRender_Window_Ext(&button->rect, CG_Debriefing_WinStringForTeam(CG_Debriefing_FindWinningTeamForMap()), 0, 18, 0.25f, 16);

		s = va(CG_TranslateString("CAMPAIGN STATUS: %s"), CG_Debriefing_WinStringForTeam(CG_Debriefing_FindOveralWinningTeam()));
		w = CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1);
		CG_Text_Paint_Ext(button->rect.x + (button->rect.w - w) * 0.5f, button->rect.y + 16, 0.25f, 0.25f, clrTxtBck, s, 0, 0, 0, &cgs.media.limboFont1);
	}
	else if (cg_gameType.integer == GT_WOLF_MAPVOTE)
	{
		CG_PanelButtonsRender_Window_Ext(&button->rect, CG_Debriefing_WinStringForTeam(CG_Debriefing_FindWinningTeamForMap()), 0, 18, 0.25f, 16);

		if (cgs.dbMapVotedFor[0] != -1 || cgs.dbMapVotedFor[1] != -1  || cgs.dbMapVotedFor[2] != -1)
		{
			s = va("^2%s", CG_TranslateString("VOTED"));
		}
		else
		{
			s = va("^3%s", CG_TranslateString("VOTE NOW"));
		}
		w = CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1);
		CG_Text_Paint_Ext(button->rect.x + (button->rect.w - w) * 0.5f, button->rect.y + 16, 0.25f, 0.25f, clrTxtBck, s, 0, 0, 0, &cgs.media.limboFont1);
	}
	else
	{
		CG_PanelButtonsRender_Window_Ext(&button->rect, CG_Debriefing_WinStringForTeam(CG_Debriefing_FindOveralWinningTeam()), 0, 18, 0.25f, 16);
	}

	secs = MAX(60 - (cg.time - cgs.intermissionStartTime) / 1000, 0);

	s = va("%s%i ^9%s", secs < 4 ? "^1" : "^2", secs, secs > 1 ? CG_TranslateString("SECS TO NEXT MAP") : CG_TranslateString("SEC TO NEXT MAP"));

	w = CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1);
	x = button->rect.x + button->rect.w - w - 4;

	CG_Text_Paint_Ext(x, button->rect.y + 16, 0.25f, 0.25f, clrTxtBck, s, 0, 0, 0, &cgs.media.limboFont1);
}

const char *awardNames[NUM_ENDGAME_AWARDS] =
{
#ifdef FEATURE_PRESTIGE
	"Most Prestigious Player",
#endif
	"Highest Ranking Officer",
#ifdef FEATURE_RATING
	"Highest Skill Rating",
#endif
	"Highest Experience Points",
	"Most Highly Decorated",
	"Highest Fragger",
	"Highest Battle Sense",
	"Best Engineer",
	"Best Medic",
	"Best Field Ops",
	"Highest Light Weapons",
	"Best Soldier",
	"Best Covert Ops",
	"Highest Accuracy",
	"Highest Headshots Percentage",
	"Best Survivor",
	"Most Damage Given",
	"Most Gibs",
	"Most Selfkills",
	"Most Deaths",
	"I Ain't Got No Friends Award",
	"Welcome Newbie! Award",
};

/**
 * @brief CG_Debriefing_Awards_Draw
 * @param[in] button
 */
void CG_Debriefing_Awards_Draw(panel_button_t *button)
{
	int   i, j;
	float y = button->rect.y + 1;

	// fallback - should be done already
	if (!cgs.dbAwardsParsed)
	{
		CG_Debriefing_ParseAwards();
	}

	for (i = 0, j = 0; i < NUM_ENDGAME_AWARDS && j < NUMSHOW_ENDGAME_AWARDS; ++i)
	{
		if (i + cgs.dbAwardsListOffset >= NUM_ENDGAME_AWARDS)
		{
			break;
		}

		if (cgs.dbAwardTeams[i + cgs.dbAwardsListOffset] == TEAM_FREE)
		{
			continue;
		}

		switch (cgs.dbAwardTeams[i + cgs.dbAwardsListOffset])
		{
		case TEAM_AXIS:
			CG_DrawPic(button->rect.x + 6, y + 2, 18, 12, cgs.media.axisFlag);
			break;
		case TEAM_ALLIES:
			CG_DrawPic(button->rect.x + 6, y + 2, 18, 12, cgs.media.alliedFlag);
			break;
		case TEAM_SPECTATOR: // fall through
		default:
			CG_DrawPic(button->rect.x + 6, y + 2, 18, 12, cgs.media.limboTeamButtonBack_on);
			CG_DrawPic(button->rect.x + 6, y + 2, 18, 12, cgs.media.limboTeamButtonSpec); // TEAM_FREE shouldn't occur
			break;
		}

		CG_Text_Paint_Ext(button->rect.x + 28, y + 11, 0.19f, 0.19f, clrTxtBck, CG_TranslateString(awardNames[i + cgs.dbAwardsListOffset]), 0, 0, 0, &cgs.media.limboFont2);
		CG_Text_Paint_Ext(button->rect.x + 28 + 180, y + 11, 0.19f, 0.19f, clrTxtBck, va("^7%s", cgs.dbAwardNames[i + cgs.dbAwardsListOffset]), 0, 0, 0, &cgs.media.limboFont2);
		y += 16;

		++j;
	}
}

/**
 * @brief CG_Debriefing_Maps_Draw
 * @param[in] button
 */
void CG_Debriefing_Maps_Draw(panel_button_t *button)
{
	vec4_t clrBck = { 0.3f, 0.3f, 0.3f, 0.4f };

	if (cg_gameType.integer == GT_WOLF_CAMPAIGN)
	{
		float      y, w;
		int        i;
		const char *str;

		if (!cgs.campaignInfoLoaded)
		{
			return;
		}

		if (cgs.tdbSelectedMap == 0)
		{
			CG_FillRect(button->rect.x + 2, button->rect.y + 2, button->rect.w - 4, 12, clrBck);
		}
		CG_Text_Paint_Ext(button->rect.x + 4, button->rect.y + 11, 0.19f, 0.19f, clrTxtBck, va(CG_TranslateString("Campaign: %s"), cgs.campaignData.campaignName), 0, 0, 0, &cgs.media.limboFont2);

		y = button->rect.y + 14;
		for (i = 0; i < cgs.campaignData.mapCount; i++)
		{
			if (cgs.tdbSelectedMap == i + 1)
			{
				CG_FillRect(button->rect.x + 2, y + 2, button->rect.w - 4, 12, clrBck);
			}

			CG_Text_Paint_Ext(button->rect.x + 8, y + 11, 0.19f, 0.19f, clrTxtBck, va("%s", cgs.campaignData.arenas[i].longname), 0, 0, 0, &cgs.media.limboFont2);

			if (i <= cgs.currentCampaignMap)
			{
				str = CG_Debriefing_WinStringForTeam(CG_Debriefing_FindWinningTeamForPos(i + 1));

				w = CG_Text_Width_Ext(str, 0.2f, 0, &cgs.media.limboFont2);
				CG_Text_Paint_Ext(button->rect.x + button->rect.w - w - 8, y + 11, 0.19f, 0.19f, clrTxtBck, str, 0, 0, 0, &cgs.media.limboFont2);
			}

			y += 13;
		}
	}
}

/**
 * @brief CG_Debriefing_Mission_Draw
 * @param[in] button
 */
void CG_Debriefing_Mission_Draw(panel_button_t *button)
{
	static qhandle_t pinAxis = 0, pinAllied = 0, pinNeutral = 0;

	if (!pinAxis)
	{
		pinAxis = trap_R_RegisterShaderNoMip("gfx/loading/pin_axis");
	}

	if (!pinAllied)
	{
		pinAllied = trap_R_RegisterShaderNoMip("gfx/loading/pin_allied");
	}

	if (!pinNeutral)
	{
		pinNeutral = trap_R_RegisterShaderNoMip("gfx/loading/pin_neutral");
	}

	if (cg_gameType.integer == GT_WOLF_CAMPAIGN)
	{
		if (!cgs.campaignInfoLoaded)
		{
			return;
		}

		if (cgs.campaignData.mapTC[0][0] != 0.f && cgs.campaignData.mapTC[1][0] != 0.f)
		{
			float  x, y, w;
			vec4_t colourFadedBlack = { 0.f, 0.f, 0.f, 0.4f };
			int    i;

			CG_DrawPicST(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.campaignData.mapTC[0][0] / 1024.f, cgs.campaignData.mapTC[0][1] / 1024.f, cgs.campaignData.mapTC[1][0] / 1024.f, cgs.campaignData.mapTC[1][1] / 1024.f, trap_R_RegisterShaderNoMip("gfx/loading/camp_map"));

			for (i = cgs.campaignData.mapCount - 1; i >= 0; i--)
			{
				x = button->rect.x + ((cgs.campaignData.arenas[i].mappos[0] - cgs.campaignData.mapTC[0][0]) / 650.f * button->rect.w);
				y = button->rect.y + ((cgs.campaignData.arenas[i].mappos[1] - cgs.campaignData.mapTC[0][1]) / 650.f * button->rect.h);

				w = CG_Text_Width_Ext(cgs.campaignData.arenas[i].longname, 0.2f, 0, &cgs.media.limboFont2);

				// Pin half width is 12
				// Pin left margin is 3
				// Pin right margin is 0
				// Text margin is 2
				if (x + 14 + w > button->rect.x + button->rect.w)
				{
					// x - pinhwidth (12) - pin left margin (3) - w - text margin (2) => x - w - 17
					CG_FillRect(x - w - 17 + 1, y - 6 + 1, 17 + w, 12, colourFadedBlack);
					CG_FillRect(x - w - 17, y - 6, 17 + w, 12, colorBlack);
				}
				else
				{
					// Width = pinhwidth (12) + pin right margin (0) + w + text margin (2) = 14 + w
					CG_FillRect(x + 1, y - 6 + 1, 14 + w, 12, colourFadedBlack);
					CG_FillRect(x, y - 6, 14 + w, 12, colorBlack);
				}

				switch (CG_Debriefing_FindWinningTeamForPos(i + 1))
				{
				case TEAM_AXIS:
					CG_DrawPic(x - 12, y - 12, 24, 24, pinAxis);
					break;
				case TEAM_ALLIES:
					CG_DrawPic(x - 12, y - 12, 24, 24, pinAllied);
					break;
				default:
					CG_DrawPic(x - 12, y - 12, 24, 24, pinNeutral);
					break;
				}

				if (x + 14 + w > button->rect.x + button->rect.w)
				{
					CG_Text_Paint_Ext(x - w - 15, y + 3, 0.2f, 0.2f, colorWhite, cgs.campaignData.arenas[i].longname, 0, 0, 0, &cgs.media.limboFont2);
				}
				else
				{
					CG_Text_Paint_Ext(x + 12, y + 3, 0.2f, 0.2f, colorWhite, cgs.campaignData.arenas[i].longname, 0, 0, 0, &cgs.media.limboFont2);
				}
			}

			if (cgs.tdbSelectedMap)
			{
				float x = button->rect.x + ((cgs.campaignData.arenas[cgs.tdbSelectedMap - 1].mappos[0] - cgs.campaignData.mapTC[0][0]) / 650.f * button->rect.w);
				float y = button->rect.y + ((cgs.campaignData.arenas[cgs.tdbSelectedMap - 1].mappos[1] - cgs.campaignData.mapTC[0][1]) / 650.f * button->rect.h);

				switch (CG_Debriefing_FindWinningTeamForPos(cgs.tdbSelectedMap))
				{
				case TEAM_AXIS:
					CG_DrawPic(x - 12, y - 12, 24, 24, pinAxis);
					break;
				case TEAM_ALLIES:
					CG_DrawPic(x - 12, y - 12, 24, 24, pinAllied);
					break;
				default:
					CG_DrawPic(x - 12, y - 12, 24, 24, pinNeutral);
					break;
				}
			}

		}
		else
		{
			CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, trap_R_RegisterShaderNoMip("menu/art/unknownmap"));
		}
		return;
	}

	if (!cgs.arenaInfoLoaded)
	{
		return;
	}

	if (cgs.arenaData.mappos[0] != 0.f && cgs.arenaData.mappos[1] != 0.f)
	{
		float            x, y, w;
		vec2_t           tl, br;
		vec4_t           colourFadedBlack = { 0.f, 0.f, 0.f, 0.4f };
		static qhandle_t campMap          = 0;

		tl[0] = cgs.arenaData.mappos[0] - .5f * 650.f;
		if (tl[0] < 0)
		{
			tl[0] = 0;
		}
		br[0] = tl[0] + 650.f;
		if (br[0] > 1024.f)
		{
			br[0] = 1024.f;
			tl[0] = br[0] - 650.f;
		}

		tl[1] = cgs.arenaData.mappos[1] - .5f * 650.f;
		if (tl[1] < 0)
		{
			tl[1] = 0;
		}
		br[1] = tl[1] + 650.f;
		if (br[1] > 1024.f)
		{
			br[1] = 1024.f;
			tl[1] = br[1] - 650.f;
		}

		if (!campMap)
		{
			campMap = trap_R_RegisterShaderNoMip("gfx/loading/camp_map");
		}

		CG_DrawPicST(button->rect.x, button->rect.y, button->rect.w, button->rect.h, tl[0] / 1024.f, tl[1] / 1024.f, br[0] / 1024.f, br[1] / 1024.f, campMap);

		x = button->rect.x + ((cgs.arenaData.mappos[0] - tl[0]) / 650.f * button->rect.w);
		y = button->rect.y + ((cgs.arenaData.mappos[1] - tl[1]) / 650.f * button->rect.h);

		w = CG_Text_Width_Ext(cgs.arenaData.longname, 0.2f, 0, &cgs.media.limboFont2);

		// Pin half width is 12
		// Pin left margin is 3
		// Pin right margin is 0
		// Text margin is 2
		if (x + 14 + w > button->rect.x + button->rect.w)
		{
			// x - pinhwidth (12) - pin left margin (3) - w - text margin (2) => x - w - 17
			CG_FillRect(x - w - 17 + 1, y - 6 + 1, 17 + w, 12, colourFadedBlack);
			CG_FillRect(x - w - 17, y - 6, 17 + w, 12, colorBlack);
		}
		else
		{
			// Width = pinhwidth (12) + pin right margin (0) + w + text margin (2) = 14 + w
			CG_FillRect(x + 1, y - 6 + 1, 14 + w, 12, colourFadedBlack);
			CG_FillRect(x, y - 6, 14 + w, 12, colorBlack);
		}

		switch (CG_Debriefing_FindWinningTeam())
		{
		case TEAM_AXIS:
			CG_DrawPic(x - 12, y - 12, 24, 24, pinAxis);
			break;
		case TEAM_ALLIES:
			CG_DrawPic(x - 12, y - 12, 24, 24, pinAllied);
			break;
		default:
			CG_DrawPic(x - 12, y - 12, 24, 24, pinNeutral);
			break;
		}

		if (x + 14 + w > button->rect.x + button->rect.w)
		{
			CG_Text_Paint_Ext(x - w - 15, y + 3, 0.2f, 0.2f, colorWhite, cgs.arenaData.longname, 0, 0, 0, &cgs.media.limboFont2);
		}
		else
		{
			CG_Text_Paint_Ext(x + 12, y + 3, 0.2f, 0.2f, colorWhite, cgs.arenaData.longname, 0, 0, 0, &cgs.media.limboFont2);
		}
	}
	else
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, trap_R_RegisterShaderNoMip("menu/art/unknownmap"));
	}
}

/**
 * @brief CG_Debriefing_FindWinningTeamForMap
 * @return
 */
team_t CG_Debriefing_FindWinningTeamForMap(void)
{
	const char *s   = CG_ConfigString(CS_MULTI_MAPWINNER);
	const char *buf = Info_ValueForKey(s, "w");

	if (atoi(buf) == -1)
	{
	}
	else if (atoi(buf))
	{
		return TEAM_ALLIES;
	}
	else
	{
		return TEAM_AXIS;
	}

	return TEAM_FREE;
}

/**
 * @brief CG_Debriefing_FindWinningTeamForPos
 * @param pos
 * @return
 */
team_t CG_Debriefing_FindWinningTeamForPos(int pos)
{
	if (cg_gameType.integer == GT_WOLF_CAMPAIGN)
	{
		if (pos == 0)
		{
//          if( cgs.campaignData.mapCount == cgs.currentCampaignMap ) {
			int i;
			int axiswins = 0, alliedwins = 0;

			for (i = 0; i < cgs.campaignData.mapCount; i++)
			{
				if (cg.teamWonRounds[1] & (1 << i))
				{
					axiswins++;
				}
				else if (cg.teamWonRounds[0]  & (1 << i))
				{
					alliedwins++;
				}
			}

			if (axiswins > alliedwins)
			{
				return TEAM_AXIS;
			}
			else if (alliedwins > axiswins)
			{
				return TEAM_ALLIES;
			}
			/*          } else {
			                const char* s = CG_ConfigString( CS_MULTI_MAPWINNER );
			                const char* buf = Info_ValueForKey( s, "w" );

			                if( Q_atoi( buf ) == -1 ) {
			                } else if( Q_atoi( buf ) ) {
			                    return TEAM_ALLIES;
			                } else {
			                    return TEAM_AXIS;
			                }
			            }*/
		}
		else
		{
			if (cg.teamWonRounds[1] & (1 << (pos - 1)))
			{
				return TEAM_AXIS;
			}
			else if (cg.teamWonRounds[0]  & (1 << (pos - 1)))
			{
				return TEAM_ALLIES;
			}
		}
	}
	else if (cg_gameType.integer == GT_WOLF || cg_gameType.integer == GT_WOLF_LMS || cg_gameType.integer == GT_WOLF_MAPVOTE)
	{
		const char *s   = CG_ConfigString(CS_MULTI_MAPWINNER);
		const char *buf = Info_ValueForKey(s, "w");

		if (atoi(buf) == -1)
		{
		}
		else if (atoi(buf))
		{
			return TEAM_ALLIES;
		}
		else
		{
			return TEAM_AXIS;
		}
	}
	else if (cg_gameType.integer == GT_WOLF_STOPWATCH)
	{
		int        defender, winner;
		const char *s;

		s        = CG_ConfigString(CS_MULTI_INFO);
		defender = Q_atoi(Info_ValueForKey(s, "d")); // defender

		s      = CG_ConfigString(CS_MULTI_MAPWINNER);
		winner = Q_atoi(Info_ValueForKey(s, "w"));

		if (!cgs.currentRound)
		{
			// second round
			if (!defender)
			{
				if (winner != defender)
				{
					return TEAM_ALLIES;
				}
				else
				{
					return TEAM_AXIS;
				}
			}
			else
			{
				if (winner != defender)
				{
					return TEAM_AXIS;
				}
				else
				{
					return TEAM_ALLIES;
				}
			}
		}
	}

	return TEAM_FREE;
}

/**
 * @brief CG_Debriefing_FindOveralWinningTeam
 * @return
 */
team_t CG_Debriefing_FindOveralWinningTeam(void)
{
	return CG_Debriefing_FindWinningTeamForPos(0);
}

/**
 * @brief CG_Debriefing_FindWinningTeam
 * @return
 */
team_t CG_Debriefing_FindWinningTeam(void)
{
	if (cg_gameType.integer == GT_WOLF_CAMPAIGN)
	{
		return CG_Debriefing_FindWinningTeamForPos(cgs.tdbSelectedMap);
	}

	return CG_Debriefing_FindOveralWinningTeam();
}

/**
 * @brief CG_Debriefing_Maps_KeyDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_Debriefing_Maps_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (cg_gameType.integer == GT_WOLF_CAMPAIGN)
		{
			int pos = ((cgs.cursorY - button->rect.y) / 14) + cgs.tdbMapListOffset;

			if (pos < 0 || pos > cgs.currentCampaignMap + 1)
			{
				return qfalse;
			}

			cgs.tdbSelectedMap = pos;
		}

		return qtrue;
	}

	return qfalse;
}

const int skillPositions[SK_NUM_SKILLS + 1] =
{
	0,
	55,
	110,
	150,
	200,
	250,
	290,
	330,
};

/**
 * @brief CG_Debriefing_TeamSkillHeaders_Draw
 * @param[in] button
 */
void CG_Debriefing_TeamSkillHeaders_Draw(panel_button_t *button)
{
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		return;
	}

	{
		int i;

		for (i = 0; i <= SK_NUM_SKILLS; i++)
		{
			if (i == SK_NUM_SKILLS)
			{
				const char *str = "Total";
				float      w    = CG_Text_Width_Ext(str, 0.175f, 0, &cgs.media.limboFont2);

				CG_Text_Paint_Ext(button->rect.x + 55 + skillPositions[i] - (w * 0.5f), button->rect.y + 5, 0.2f, 0.2f, clrTxtBck, str, 0, 0, 0, &cgs.media.limboFont2);
			}
			else
			{
				CG_DrawPic(button->rect.x + 50 + skillPositions[i], button->rect.y - 8, 20, 20, cgs.media.skillPics[i]);
			}
		}
	}
}

/**
 * @brief CG_Debriefing_TeamSkillXP_Draw
 * @param[in] button
 */
void CG_Debriefing_TeamSkillXP_Draw(panel_button_t *button)
{
	team_t winner = CG_Debriefing_FindOveralWinningTeam();
	team_t team;
	float  scale;
	int    xp, i;

	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (button->data[0])
	{
		team = winner == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
	}
	else
	{
		team = winner == TEAM_AXIS ? TEAM_AXIS : TEAM_ALLIES;
	}

	if (team == winner)
	{
		scale = 0.225f;
	}
	else
	{
		scale = 0.175f;
	}

	switch (team)
	{
	case TEAM_AXIS:
		CG_Text_Paint_Ext(button->rect.x, button->rect.y + 11, scale, scale, clrTxtBck, "Axis", 0, 0, 0, &cgs.media.limboFont2);
		break;
	default:
		CG_Text_Paint_Ext(button->rect.x, button->rect.y + 11, scale, scale, clrTxtBck, "Allies", 0, 0, 0, &cgs.media.limboFont2);
		break;
	}

	for (i = 0; i <= SK_NUM_SKILLS; i++)
	{
		float      w;
		const char *str;

		if (i == SK_NUM_SKILLS)
		{
			xp = CG_TeamDebriefing_CalcXP(team, cgs.tdbSelectedMap - 1, -1);
		}
		else
		{
			xp = CG_TeamDebriefing_CalcXP(team, cgs.tdbSelectedMap - 1, i);
		}

		str = va("%i", xp);

		w = CG_Text_Width_Ext(str, scale, 0, &cgs.media.limboFont2);

		CG_Text_Paint_Ext(button->rect.x + 60 + skillPositions[i] - (w * 0.5f), button->rect.y + 11, scale, scale, clrTxtBck, str, 0, 0, 0, &cgs.media.limboFont2);
	}
}

/**
 * @brief CG_parseMapVoteListInfo
 */
void CG_parseMapVoteListInfo()
{
	int i;

	cgs.dbNumMaps = (trap_Argc() - 2) / 4;

	if (atoi(CG_Argv(1)))
	{
		cgs.dbMapMultiVote = qtrue;
	}

	for (i = 0; i < cgs.dbNumMaps; i++)
	{
		Q_strncpyz(cgs.dbMaps[i], CG_Argv((i * 4) + 2),
		           sizeof(cgs.dbMaps[0]));
		cgs.dbMapVotes[i]      = 0;
		cgs.dbMapID[i]         = Q_atoi(CG_Argv((i * 4) + 3));
		cgs.dbMapLastPlayed[i] = Q_atoi(CG_Argv((i * 4) + 4));
		cgs.dbMapTotalVotes[i] = Q_atoi(CG_Argv((i * 4) + 5));
		if (CG_FindArenaInfo(va("scripts/%s.arena", cgs.dbMaps[i]),
		                     cgs.dbMaps[i], &cgs.arenaData))
		{
			Q_strncpyz(cgs.dbMapDispName[i],
			           cgs.arenaData.longname,
			           sizeof(cgs.dbMaps[0]));
		}
		else
		{
			Q_strncpyz(cgs.dbMapDispName[i],
			           cgs.dbMaps[i],
			           sizeof(cgs.dbMaps[0]));
		}
	}

	CG_LocateArena();

	cgs.dbMapListReceived = qtrue;

	return;
}

/**
 * @brief CG_parseMapVoteTally
 */
void CG_parseMapVoteTally()
{
	int i, j, numMaps;

	cgs.dbMapVotesSum = 0;
	Com_Memset(cgs.dbSortedVotedMapsByTotal, -1, sizeof(cgs.dbSortedVotedMapsByTotal));

	numMaps = (trap_Argc() - 1);
	for (i = 0; i < numMaps; i++)
	{
		cgs.dbMapVotes[i]  = Q_atoi(CG_Argv(i + 1));
		cgs.dbMapVotesSum += cgs.dbMapVotes[i];

		// sort voted maps by total votes accumulated
		for (j = 0; j < MAX_VOTE_MAPS; j++)
		{
			if (cgs.dbSortedVotedMapsByTotal[j].totalVotes < cgs.dbMapVotes[i])
			{
				if (j != MAX_VOTE_MAPS - 1 && cgs.dbSortedVotedMapsByTotal[j].totalVotes != -1)
				{
					memmove(&cgs.dbSortedVotedMapsByTotal[j + 1], &cgs.dbSortedVotedMapsByTotal[j], sizeof(sortedVotedMapByTotal_s) * (MAX_VOTE_MAPS - j - 1));
				}

				cgs.dbSortedVotedMapsByTotal[j].mapID      = i;
				cgs.dbSortedVotedMapsByTotal[j].totalVotes = cgs.dbMapVotes[i];
				break;
			}
		}
	}

	cgs.dbVoteTallyReceived = qtrue;

	return;
}
