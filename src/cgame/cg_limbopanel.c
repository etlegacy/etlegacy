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
 * @file cg_limbopanel.c
 */

#include "cg_local.h"

#define SOUNDEVENT(sound) trap_S_StartLocalSound(sound, CHAN_LOCAL_SOUND)

#define SOUND_SELECT    SOUNDEVENT(cgs.media.sndLimboSelect)
#define SOUND_FILTER    SOUNDEVENT(cgs.media.sndLimboFilter)
//#define SOUND_CANCEL    SOUNDEVENT(cgs.media.sndLimboCancel)

void CG_DrawBorder(float x, float y, float w, float h, qboolean fill, qboolean drawMouseOver);

static team_t teamOrder[3] =
{
	TEAM_AXIS,
	TEAM_ALLIES,
	TEAM_SPECTATOR,
};

static panel_button_text_t nameEditFont =
{
	0.22f,                   0.24f,
	{ 1.f,                   1.f,  1.f,0.8f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

static panel_button_text_t classBarFont =
{
	0.22f,                 0.24f,
	{ 0.f,                 0.f,  0.f,0.8f },
	0,                     0,
	&cgs.media.limboFont2,
};

static panel_button_text_t titleLimboFont =
{
	0.24f,                 0.28f,
	{ 1.f,                 1.f,  1.f,0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};


//panel_button_text_t titleLimboFontBig =
//{
//	0.3f,                  0.3f,
//	{ 1.f,                 1.f, 1.f,  0.6f },
//	0,                     0,
//	&cgs.media.limboFont1,
//};

static panel_button_text_t titleLimboFontBigCenter =
{
	0.3f,                  0.3f,
	{ 1.f,                 1.f,              1.f,  0.6f },
	0,                     ITEM_ALIGN_CENTER,
	&cgs.media.limboFont1,
};

static panel_button_text_t spawnLimboFont =
{
	0.18f,                 0.22f,
	{ 1.f,                 1.f,  1.f,0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

static panel_button_text_t weaponButtonFont =
{
	0.33f,                 0.33f,
	{ 0.f,                 0.f,  0.f,0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

static panel_button_text_t weaponPanelNameFont =
{
	0.20f,                 0.24f,
	{ 1.0f,                1.0f, 1.0f,  0.4f },
	0,                     0,
	&cgs.media.limboFont1,
};

static panel_button_text_t weaponPanelFilterFont =
{
	0.17f,                    0.17f,
	{ 1.0f,                   1.0f, 1.0f,  0.6f },
	0,                        0,
	&cgs.media.limboFont1_lo,
};

static panel_button_text_t weaponPanelStatsFont =
{
	0.15f,                    0.17f,
	{ 1.0f,                   1.0f, 1.0f,  0.6f },
	0,                        0,
	&cgs.media.limboFont1_lo,
};

static panel_button_text_t weaponPanelStatsPercFont =
{
	0.2f,                  0.2f,
	{ 1.0f,                1.0f,1.0f,    0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

static panel_button_text_t objectivePanelTxt =
{
	0.2f,                  0.2f,
	{ 0.0f,                0.0f,0.0f,    0.5f },
	0,                     0,
	&cgs.media.limboFont2,
};


static panel_button_t rightLimboPannel =
{
	"gfx/limbo/limbo_back",
	NULL,
	{ 440,                    0,  200, SCREEN_HEIGHT },
	{ 0,                      0,  0,   0, 0, 0, 0, 0 },
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

#define MEDAL_PIC_GAP   ((MEDAL_PIC_SIZE - (MEDAL_PIC_WIDTH * MEDAL_PIC_COUNT)) / (MEDAL_PIC_COUNT + 1.f))
#define MEDAL_PIC_COUNT 7.f
#define MEDAL_PIC_WIDTH 22.f
#define MEDAL_PIC_X     450.f
#define MEDAL_PIC_SIZE  (630.f - MEDAL_PIC_X)
#define MEDAL_PIC(number)                \
	static panel_button_t medalPic ## number = {         \
		NULL,                                   \
		NULL,                                   \
		{ MEDAL_PIC_X + MEDAL_PIC_GAP + ((number) * (MEDAL_PIC_GAP + MEDAL_PIC_WIDTH)),119,                                                                           MEDAL_PIC_WIDTH, 26 }, \
		{ number,                  0,                                                                             0,               0, 0, 0, 0, 0},        \
		NULL,                      /* font       */              \
		NULL,                      /* keyDown    */                  \
		NULL,                      /* keyUp  */                  \
		CG_LimboPanel_RenderMedal,              \
		NULL,                                   \
		0,                                   \
	}

MEDAL_PIC(0);
MEDAL_PIC(1);
MEDAL_PIC(2);
MEDAL_PIC(3);
MEDAL_PIC(4);
MEDAL_PIC(5);
MEDAL_PIC(6);

#ifdef FEATURE_PRESTIGE
#define SKILL_PIC_GAP   ((SKILL_PIC_SIZE - (SKILL_PIC_WIDTH * SKILL_PIC_COUNT)) / (SKILL_PIC_COUNT + 1.f))
#define SKILL_PIC_COUNT 7.f
#define SKILL_PIC_WIDTH 22.f
#define SKILL_PIC_X     450.f
#define SKILL_PIC_SIZE  (630.f - SKILL_PIC_X)
#define SKILL_PIC(number)                \
	static panel_button_t skillPic ## number = {         \
		NULL,                                   \
		NULL,                                   \
		{ SKILL_PIC_X + SKILL_PIC_GAP + ((number) * (SKILL_PIC_GAP + SKILL_PIC_WIDTH)), 119, SKILL_PIC_WIDTH, 26 }, \
		{ number,                  0,             0,               0, 0, 0, 0, 0}, \
		NULL,                      /* font       */              \
		NULL,                      /* keyDown    */              \
		NULL,                      /* keyUp  */                  \
		CG_LimboPanel_RenderPrestige,        \
		NULL,                                \
		0,                                   \
	}

SKILL_PIC(0);
SKILL_PIC(1);
SKILL_PIC(2);
SKILL_PIC(3);
SKILL_PIC(4);
SKILL_PIC(5);
SKILL_PIC(6);
#endif

#define TEAM_COUNTER_GAP    ((TEAM_COUNTER_SIZE - (TEAM_COUNTER_WIDTH * TEAM_COUNTER_COUNT)) / (TEAM_COUNTER_COUNT + 1.f))
#define TEAM_COUNTER_COUNT  3.f
#define TEAM_COUNTER_WIDTH  20.f
#define TEAM_COUNTER_X      432.f
#define TEAM_COUNTER_SIZE   (660.f - TEAM_COUNTER_X)
#define TEAM_COUNTER_BUTTON_DIFF (-24.f)
#define TEAM_COUNTER_SPACING    4.f

#define TEAM_COUNTER(number)             \
	static panel_button_t teamCounter ## number = {      \
		NULL,                                   \
		NULL,                                   \
		{ TEAM_COUNTER_X + TEAM_COUNTER_GAP + ((number) * (TEAM_COUNTER_GAP + TEAM_COUNTER_WIDTH)),236,                                                                                       TEAM_COUNTER_WIDTH, 14 },  \
		{ 1,                         number,                                                                                    0,                  0, 0, 0, 0, 0},        \
		NULL,                        /* font       */              \
		NULL,                        /* keyDown    */                  \
		NULL,                        /* keyUp  */                  \
		CG_LimboPanel_RenderCounter,            \
		NULL,                                   \
		0,                                   \
	};                                          \
	static panel_button_t teamCounterLight ## number = { \
		NULL,                                   \
		NULL,                                   \
		{ TEAM_COUNTER_X + TEAM_COUNTER_GAP + ((number) * (TEAM_COUNTER_GAP + TEAM_COUNTER_WIDTH)) - 20,236,                                                                                            16, 16 }, \
		{ 1,                       number,                                                                                         0,  0, 0, 0, 0, 0},        \
		NULL,                      /* font       */              \
		NULL,                      /* keyDown    */                  \
		NULL,                      /* keyUp  */                  \
		CG_LimboPanel_RenderLight,              \
		NULL,                                   \
		0,                                   \
	};                                          \
	static panel_button_t teamButton ## number = {       \
		NULL,                                   \
		NULL,                                   \
		{ TEAM_COUNTER_X + TEAM_COUNTER_GAP + ((number) * (TEAM_COUNTER_GAP + TEAM_COUNTER_WIDTH) + (TEAM_COUNTER_BUTTON_DIFF / 2.f)) - 17 + TEAM_COUNTER_SPACING, \
		  188 + TEAM_COUNTER_SPACING, \
		  TEAM_COUNTER_WIDTH - TEAM_COUNTER_BUTTON_DIFF + 20 - 2 * TEAM_COUNTER_SPACING, \
		  44 - 2 * TEAM_COUNTER_SPACING },  \
		{ number,                                                                        0,0, 0, 0, 0, 0, 0 },        \
		NULL,                                                                            /* font       */              \
		CG_LimboPanel_TeamButton_KeyDown,                                                /* keyDown    */ \
		NULL,                                                                            /* keyUp  */                  \
		CG_LimboPanel_RenderTeamButton,         \
		NULL,                                   \
		0,                                   \
	}

TEAM_COUNTER(0);
TEAM_COUNTER(1);
TEAM_COUNTER(2);

#define CLASS_COUNTER_GAP   ((CLASS_COUNTER_SIZE - (CLASS_COUNTER_WIDTH * CLASS_COUNTER_COUNT)) / (CLASS_COUNTER_COUNT + 1.f))
#define CLASS_COUNTER_COUNT 5.f
#define CLASS_COUNTER_WIDTH 20.f
#define CLASS_COUNTER_X     435.f
#define CLASS_COUNTER_SIZE  (645.f - CLASS_COUNTER_X)
//#define CLASS_COUNTER_LIGHT_DIFF 4.f
#define CLASS_COUNTER_BUTTON_DIFF (-18.f)
#define CLASS_COUNTER(number)            \
	static panel_button_t classCounter ## number = {     \
		NULL,                                   \
		NULL,                                   \
		{ CLASS_COUNTER_X + CLASS_COUNTER_GAP + ((number) * (CLASS_COUNTER_GAP + CLASS_COUNTER_WIDTH)),302,                                                                                           CLASS_COUNTER_WIDTH, 14 }, \
		{ 0,                         number,                                                                                        0,                   0, 0, 0, 0, 0},        \
		NULL,                        /* font       */              \
		NULL,                        /* keyDown    */                  \
		NULL,                        /* keyUp  */                  \
		CG_LimboPanel_RenderCounter,            \
		NULL,                                   \
		0,                                      \
	};                                          \
	static panel_button_t classButton ## number = {      \
		NULL,                                   \
		NULL,                                   \
		{ CLASS_COUNTER_X + CLASS_COUNTER_GAP + ((number) * (CLASS_COUNTER_GAP + CLASS_COUNTER_WIDTH)) + (CLASS_COUNTER_BUTTON_DIFF / 2.f),266,                                                                                                                               CLASS_COUNTER_WIDTH - CLASS_COUNTER_BUTTON_DIFF, 34 },   \
		{ 0,                             number,                                                                                                                            0,                                               0, 0, 0, 0, 0},        \
		NULL,                            /* font       */              \
		CG_LimboPanel_ClassButton_KeyDown,/* keyDown   */  \
		NULL,                            /* keyUp  */                  \
		CG_LimboPanel_RenderClassButton,        \
		NULL,                                   \
		0,                                      \
	}

static panel_button_t classBar =
{
	"gfx/limbo/lightup_bar",
	NULL,
	{ 470,                    320,140, 20 },
	{ 0,                      0,  0,   0, 0, 0, 0, 0},
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

static panel_button_t classBarText =
{
	NULL,
	NULL,
	{ 460,                      334,   160, 16 },
	{ 0,                        0,     0,   0, 0, 0, 0, 0},
	&classBarFont,              // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_ClassBar_Draw,
	NULL,
	0
};

CLASS_COUNTER(0);
CLASS_COUNTER(1);
CLASS_COUNTER(2);
CLASS_COUNTER(3);
CLASS_COUNTER(4);

#define FILTER_BUTTON(number) \
	static panel_button_t filterButton ## number = { \
		NULL,                               \
		NULL,                               \
		{ 14,                      50 + ((number) * 30), 26, 26 },     \
		{ number,                  0,                    0,  0, 0, 0, 0, 0},    \
		NULL,                      /* font       */          \
		CG_LimboPanel_Filter_KeyDown,/* keyDown    */              \
		NULL,                      /* keyUp  */              \
		CG_LimboPanel_Filter_Draw,          \
		NULL,                               \
		0,                                  \
	}

FILTER_BUTTON(0);
FILTER_BUTTON(1);
FILTER_BUTTON(2);
FILTER_BUTTON(3);
FILTER_BUTTON(4);
FILTER_BUTTON(5);
FILTER_BUTTON(6);
FILTER_BUTTON(7);
FILTER_BUTTON(8);

static panel_button_t filterTitleText =
{
	NULL,
	"FILTERS",
	{ 8,                       36, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&weaponPanelFilterFont,    // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

#define LEFT_FRAME(shader, number, x, y, w, h) \
	static panel_button_t leftFrame0 ## number = {   \
		shader,                             \
		NULL,                               \
		{ x,                       y, w, h },                     \
		{ 0,                       0, 0, 0, 0, 0, 0, 0},         \
		NULL,                      /* font       */          \
		NULL,                      /* keyDown    */              \
		NULL,                      /* keyUp  */              \
		BG_PanelButtonsRender_Img,          \
		NULL,                               \
		0,                                  \
	}

#define LF_X1 64
#define LF_X2 416
#define LF_X3 440

#define LF_W1 (LF_X1 - 0)
#define LF_W2 (LF_X2 - LF_X1)
#define LF_W3 (LF_X3 - LF_X2)

#define LF_Y1 23
#define LF_Y2 375
#define LF_Y3 SCREEN_HEIGHT

#define LF_H1 (LF_Y1 - 0)
#define LF_H2 (LF_Y2 - LF_Y1)
#define LF_H3 (LF_Y3 - LF_Y2)

LEFT_FRAME("gfx/limbo/limbo_frame01", 1, 0, 0, LF_W1, LF_H1);
LEFT_FRAME("gfx/limbo/limbo_frame02", 2, LF_X1, 0, LF_W2, LF_H1);
LEFT_FRAME("gfx/limbo/limbo_frame03", 3, LF_X2, 0, LF_W3, LF_H1);

LEFT_FRAME("gfx/limbo/limbo_frame04", 4, LF_X2, LF_Y1, LF_W3, LF_H2);

LEFT_FRAME("gfx/limbo/limbo_frame05", 5, LF_X2, LF_Y2, LF_W3, LF_H3);
LEFT_FRAME("gfx/limbo/limbo_frame06", 6, LF_X1, LF_Y2, LF_W2, LF_H3);
LEFT_FRAME("gfx/limbo/limbo_frame07", 7, 0, LF_Y2, LF_W1, LF_H3);

LEFT_FRAME("gfx/limbo/limbo_frame08", 8, 0, LF_Y1, LF_W1, LF_H2);

static panel_button_t playerLimboHead =
{
	NULL,
	NULL,
	{ 456,                   30,   68, 84 },
	{ 0,                     0,    0,  0, 0, 0, 0, 0},
	NULL,                    // font
	NULL,                    // keyDown
	NULL,                    // keyUp
	CG_LimboPanel_RenderHead,
	NULL,
	0
};

static panel_button_t playerXPCounterText =
{
	NULL,
	"XP",
	{ 566,                        108,   60, 16 },
	{ 2,                          0,     0,  0, 0, 0, 0, 0},
	&spawnLimboFont,              // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_LimboPanelRenderText_NoLMS,
	NULL,
	0
};

static panel_button_t playerXPCounter =
{
	NULL,
	NULL,
	{ 584,                      96,   40, 16 },
	{ 2,                        0,    0,  0, 0, 0, 0, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};

static panel_button_t playerSkillCounter0 =
{
	NULL,
	NULL,
	{ 552,                      36,   60, 16 },
	{ 4,                        0,    0,  0, 0, 0, 4, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};

static panel_button_t playerSkillCounter1 =
{
	NULL,
	NULL,
	{ 552,                      56,   60, 16 },
	{ 4,                        1,    0,  0, 0, 0, 4, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};

static panel_button_t playerSkillCounter2 =
{
	NULL,
	NULL,
	{ 552,                      76,   60, 16 },
	{ 4,                        2,    0,  0, 0, 0, 4, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};

static panel_button_t playerSkillIcon0 =
{
	NULL,
	NULL,
	{ 532,                        36,   16, 16 },
	{ 0,                          0,    0,  0, 0, 0, 0, 0},
	NULL,                         // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_LimboPanel_RenderSkillIcon,
	NULL,
	0
};

static panel_button_t playerSkillIcon1 =
{
	NULL,
	NULL,
	{ 532,                        56,   16, 16 },
	{ 1,                          0,    0,  0, 0, 0, 0, 0},
	NULL,                         // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_LimboPanel_RenderSkillIcon,
	NULL,
	0
};

static panel_button_t playerSkillIcon2 =
{
	NULL,
	NULL,
	{ 532,                        76,   16, 16 },
	{ 2,                          0,    0,  0, 0, 0, 0, 0},
	NULL,                         // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_LimboPanel_RenderSkillIcon,
	NULL,
	0
};

#ifdef FEATURE_PRESTIGE
static panel_button_t playerPrestigeText =
{
	NULL,
	NULL,
	{ 576,                        16,   60, 16 },
	{ 0,                          0,     0,  0, 0, 0, 0, 0},
	&spawnLimboFont,              // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_LimboPanel_Prestige_Draw,
	NULL,
	0
};

static panel_button_t playerPrestigeIcon =
{
	NULL,
	NULL,
	{ 616,                        4,   16, 16 },
	{ 2,                          0,    0,  0, 0, 0, 0, 0},
	NULL,                         // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_LimboPanel_RenderPrestigeIcon,
	NULL,
	0
};
#endif

// =======================

static panel_button_t spawnPointText =
{
	NULL,
	"AUTOSPAWN",
	{ 150,                     392,0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&spawnLimboFont,           /* font        */
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t spawnPointButton =
{
	NULL,
	NULL,
	{ 132,                              381,   15, 15 },
	{ 0,                                0,     0,  0, 0, 0, 0, 0},
	NULL,                               // font
	CG_LimboPanel_SpawnPointButton_KeyDown,// keyDown
	NULL,                               // keyUp
	CG_LimboPanel_SpawnPointButton_Draw,
	NULL,
	0
};

static panel_button_t mapTimeCounter =
{
	NULL,
	NULL,
	{ 276,                      5,   20, 14 },
	{ 5,                        0,   0,  0, 0, 0, 0, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};

static panel_button_t mapTimeCounter2 =
{
	NULL,
	NULL,
	{ 252,                      5,   20, 14 },
	{ 5,                        1,   0,  0, 0, 0, 0, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};

static panel_button_t mapTimeCounterText =
{
	NULL,
	"MISSION TIME",
	{ 186,                     16, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&spawnLimboFont,           /* font        */
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

// =======================

static panel_button_t respawnCounter =
{
	NULL,
	NULL,
	{ 400,                      5,   20, 14 },
	{ 3,                        0,   0,  0, 0, 0, 0, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};

static panel_button_t respawnCounterText =
{
	NULL,
	"REINFORCEMENTS",
	{ 312,                     16, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&spawnLimboFont,           /* font        */
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

// =======================

static panel_button_t limboTitleText =
{
	NULL,
	"COMMAND MAP",
	{ 8,                       16, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&titleLimboFont,           /* font        */
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t playerSetupText =
{
	NULL,
	"PLAYER SETUP",
	{ 448,                     16, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&titleLimboFont,           /* font        */
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t skillsText =
{
	NULL,
	"SKILLS",
	{ 532,                             32, 0, 0 },
	{ 0,                               0,  0, 0, 0, 0, 0, 0},
	&weaponPanelStatsFont,             // font
	NULL,                              // keyDown
	NULL,                              // keyUp
	CG_LimboPanelRenderText_SkillsText,
	NULL,
	0
};

// =======================

static panel_button_t weaponPanel =
{
	NULL,
	NULL,
	{ 455,                    353,   140, 56 },
	{ 0,                      0,     0,   0, 0, 0, 0, 0},
	NULL,                     // font
	CG_LimboPanel_WeaponPanel_KeyDown,// keyDown
	CG_LimboPanel_WeaponPanel_KeyUp,// keyUp
	CG_LimboPanel_WeaponPanel,
	NULL,
	0
};

static panel_button_t weaponLight1 =
{
	NULL,
	NULL,
	{ 605,                     362,   20, 20 },
	{ SECONDARY_SLOT,          0,     0,  0, 0, 0, 0, 0},
	NULL,                      // font
	CG_LimboPanel_WeaponLights_KeyDown,// keyDown
	NULL,                      // keyUp
	CG_LimboPanel_WeaponLights,
	NULL,
	0
};

static panel_button_t weaponLight1Text =
{
	NULL,
	"1",
	{ 609,                     378,   0, 0 },
	{ 0,                       0,     0, 0, 0, 0, 0, 0},
	&weaponButtonFont,         // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t weaponLight2 =
{
	NULL,
	NULL,
	{ 605,                     386,   20, 20 },
	{ PRIMARY_SLOT,            0,     0,  0, 0, 0, 0, 0},
	NULL,                      // font
	CG_LimboPanel_WeaponLights_KeyDown,// keyDown
	NULL,                      // keyUp
	CG_LimboPanel_WeaponLights,
	NULL,
	0
};

static panel_button_t weaponLight2Text =
{
	NULL,
	"2",
	{ 609,                     402,   0, 0 },
	{ 0,                       0,     0, 0, 0, 0, 0, 0},
	&weaponButtonFont,         // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t weaponStatsShotsText =
{
	NULL,
	"SHOTS",
	{ 460,                     422,0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&weaponPanelStatsFont,     // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t weaponStatsShotsCounter =
{
	NULL,
	NULL,
	{ 460,                      426,   40, 14 },
	{ 6,                        0,     0,  0, 0, 0, 0, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};

static panel_button_t weaponStatsHitsText =
{
	NULL,
	"HITS",
	{ 516,                     422, 0, 0 },
	{ 0,                       0,   0, 0, 0, 0, 0, 0},
	&weaponPanelStatsFont,     // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t weaponStatsHitsCounter =
{
	NULL,
	NULL,
	{ 516,                      426,   40, 14 },
	{ 6,                        1,     0,  0, 0, 0, 0, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};


static panel_button_t weaponStatsAccText =
{
	NULL,
	"ACC",
	{ 570,                     422,  0, 0 },
	{ 0,                       0,    0, 0, 0, 0, 0, 0},
	&weaponPanelStatsFont,     // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t weaponStatsAccCounter =
{
	NULL,
	NULL,
	{ 570,                      426,   30, 14 },
	{ 6,                        2,     0,  0, 0, 0, 0, 0},
	NULL,                       // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LimboPanel_RenderCounter,
	NULL,
	0
};

static panel_button_t weaponStatsAccPercentage =
{
	NULL,
	"%",
	{ 600,                     436,   0, 0 },
	{ 0,                       0,     0, 0, 0, 0, 0, 0},
	&weaponPanelStatsPercFont, // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

// =======================

static panel_button_t commandmapPanel =
{
	NULL,
	NULL,
	{ CC_2D_X,                     CC_2D_Y,       CC_2D_W, CC_2D_H },
	{ 0,                           0,             0,       0, 0, 0, 0, 0},
	NULL,                          // font
	NULL,                          // keyDown
	NULL,                          // keyUp
	CG_LimboPanel_RenderCommandMap,
	NULL,
	0
};

// =======================

static panel_button_t objectivePanel =
{
	NULL,
	NULL,
	{ 8,                              398, 240, 74 },
	{ 0,                              0,   0,   0, 0, 0, 0, 0},
	NULL,                             // font
	NULL,                             // keyDown
	NULL,                             // keyUp
	CG_LimboPanel_RenderObjectiveBack,
	NULL,
	0
};

static panel_button_t objectivePanelText =
{
	NULL,
	NULL,
	{ 8,                              398, 240, 74 },
	{ 0,                              0,   0,   0, 0, 0, 0, 0},
	&objectivePanelTxt,               // font
	NULL,                             // keyDown
	NULL,                             // keyUp
	CG_LimboPanel_RenderObjectiveText,
	NULL,
	0
};

static panel_button_t objectivePanelTitle =
{
	NULL,
	"OBJECTIVES",
	{ 8,                       392,0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&titleLimboFont,           // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t objectivePanelButtonUp =
{
	"gfx/limbo/but_objective_up",
	NULL,
	{ 252,                       416,24, 24 },
	{ 0,                         0,  0,  0, 0, 0, 0, 1},
	NULL,                        // font
	CG_LimboPanel_ObjectiveText_KeyDown,// keyDown
	NULL,                        // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

static panel_button_t briefingButton =
{
	NULL,
	NULL,
	{ 252,                            388,   24, 24 },
	{ 0,                              0,     0,  0, 0, 0, 0, 0},
	NULL,                             // font
	CG_LimboPanel_BriefingButton_KeyDown,// keyDown
	NULL,                             // keyUp
	CG_LimboPanel_BriefingButton_Draw,
	NULL,
	0
};

static panel_button_t objectivePanelButtonDown =
{
	"gfx/limbo/but_objective_dn",
	NULL,
	{ 252,                       444,24, 24 },
	{ 0,                         0,  0,  0, 0, 0, 0, 0},
	NULL,                        // font
	CG_LimboPanel_ObjectiveText_KeyDown,// keyDown
	NULL,                        // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

// =======================

static panel_button_t okButtonText =
{
	NULL,
	"OK",
	{ 484,                     469,   100, 40 },
	{ 0,                       0,     0,   0, 0, 0, 0, 0},
	&titleLimboFont,           // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t okButton =
{
	NULL,
	NULL,
	{ 454 + 2,                454 + 2,       82 - 4, 18 - 4 },
	{ 0,                      0,             0,      0, 0, 0, 0, 0},
	NULL,                     // font
	CG_LimboPanel_OkButton_KeyDown,// keyDown
	NULL,                     // keyUp
	CG_LimboPanel_Border_Draw,
	NULL,
	0
};

static panel_button_t cancelButtonText =
{
	NULL,
	"CANCEL",
	{ 556,                     469,100, 40 },
	{ 0,                       0,  0,   0, 0, 0, 0, 0},
	&titleLimboFont,           // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t cancelButton =
{
	NULL,
	NULL,
	{ 543 + 2,                454 + 2,       82 - 4, 18 - 4 },
	{ 0,                      0,             0,      0, 0, 0, 0, 0},
	NULL,                     // font
	CG_LimboPanel_CancelButton_KeyDown,// keyDown
	NULL,                     // keyUp
	CG_LimboPanel_Border_Draw,
	NULL,
	0
};

// =======================

static panel_button_t nameEdit =
{
	NULL,
	"limboname",
	{ SCREEN_HEIGHT,             150,      120, 20 },
	{ 0,                         0,        0,   0, 0, 0, 0, 0},
	&nameEditFont,               // font
	BG_PanelButton_EditClick,    // keyDown
	NULL,                        // keyUp
	BG_PanelButton_RenderEdit,
	CG_LimboPanel_NameEditFinish,
	0
};

static panel_button_t plusButton =
{
	NULL,
	NULL,
	{ 18,                     325,  18, 12 },
	{ 12,                     0,    0,  0, 0, 0, 0, 0},
	NULL,                     // font
	CG_LimboPanel_PlusButton_KeyDown,// keyDown
	NULL,                     // keyUp
	CG_LimboPanel_Border_Draw,
	NULL,
	0
};

static panel_button_t plusButtonText =
{
	NULL,
	"+",
	{ 18,                      323,  18, 12 },
	{ 12,                      0,    0,  0, 0, 0, 0, 0},
	&titleLimboFontBigCenter,  // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t minusButton =
{
	NULL,
	NULL,
	{ 18,                     348,  18, 12 },
	{ 12,                     0,    0,  0, 0, 0, 0, 0},
	NULL,                     // font
	CG_LimboPanel_MinusButton_KeyDown,// keyDown
	NULL,                     // keyUp
	CG_LimboPanel_Border_Draw,
	NULL,
	0
};

static panel_button_t minusButtonText =
{
	NULL,
	"-",
	{ 18,                      346,  18, 12 },
	{ 12,                      0,    0,  0, 0, 0, 0, 0},
	&titleLimboFontBigCenter,  // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

static panel_button_t *limboPanelButtons[] =
{
	&rightLimboPannel,

	&classCounter0,           &classCounter1,             &classCounter2,         &classCounter3, &classCounter4,
//  &classCounterLight0,    &classCounterLight1,    &classCounterLight2,    &classCounterLight3,    &classCounterLight4,
	&classButton0,            &classButton1,              &classButton2,          &classButton3,  &classButton4,

	&classBar,                &classBarText,

	&leftFrame01,             &leftFrame02,               &leftFrame03,           &leftFrame04,
	&leftFrame05,             &leftFrame06,               &leftFrame07,           &leftFrame08,

	&filterButton0,           &filterButton1,             &filterButton2,         &filterButton3, &filterButton4,
	&filterButton5,           &filterButton6,             &filterButton7,         &filterButton8,
	&filterTitleText,

	&medalPic0,               &medalPic1,                 &medalPic2,             &medalPic3,     &medalPic4,    &medalPic5,&medalPic6,
#ifdef FEATURE_PRESTIGE
	&skillPic0,               &skillPic1,                 &skillPic2,             &skillPic3,     &skillPic4,    &skillPic5, &skillPic6,
#endif

	&teamCounter0,            &teamCounter1,              &teamCounter2,
	&teamCounterLight0,       &teamCounterLight1,         &teamCounterLight2,
	&teamButton0,             &teamButton1,               &teamButton2,

	&playerLimboHead,
#ifdef FEATURE_PRESTIGE
	&playerPrestigeText,      &playerPrestigeIcon,
#endif
	&playerXPCounter,         &playerXPCounterText,

	&respawnCounter,          &respawnCounterText,
	&mapTimeCounter,          &mapTimeCounter2,           &mapTimeCounterText,
	&spawnPointText,          &spawnPointButton,

	&playerSkillCounter0,     &playerSkillCounter1,       &playerSkillCounter2,
	&playerSkillIcon0,        &playerSkillIcon1,          &playerSkillIcon2,

	&objectivePanel,          &objectivePanelTitle,       &objectivePanelText,
	&objectivePanelButtonUp,  &objectivePanelButtonDown,

	&limboTitleText,
	&playerSetupText,
	&skillsText,

	&commandmapPanel,

	&okButton,                &okButtonText,
	&cancelButton,            &cancelButtonText,

	&nameEdit,

	&weaponLight1,            &weaponLight2,
	&weaponLight1Text,        &weaponLight2Text,
	&weaponPanel,
	&weaponStatsShotsText,    &weaponStatsHitsText,       &weaponStatsAccText,
	&weaponStatsShotsCounter, &weaponStatsHitsCounter,    &weaponStatsAccCounter,
	&weaponStatsAccPercentage,

	&briefingButton,

	&plusButton,              &plusButtonText,
	&minusButton,             &minusButtonText,

	NULL,
};

/**
 * @brief CG_LimboPanel_SpawnPointButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_SpawnPointButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		trap_SendClientCommand("setspawnpt 0");
		cgs.ccSelectedSpawnPoint = 0;

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_BriefingButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_BriefingButton_KeyDown(panel_button_t *button, int key)
{
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		return qfalse;
	}

	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		if (cg.limboEndCinematicTime > cg.time)
		{
			trap_S_StopStreamingSound(-1);
			cg.limboEndCinematicTime = 0;

			return qtrue;
		}

		cg.limboEndCinematicTime = cg.time + CG_SoundPlaySoundScript(va("news_%s", cgs.rawmapname), NULL, -1, qfalse);

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_SpawnPointButton_Draw
 * @param[in] button
 */
void CG_LimboPanel_SpawnPointButton_Draw(panel_button_t *button)
{
	if (CG_LimboPanel_GetSpawnPoint() == 0)
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, BG_CursorInRect(&button->rect) ? cgs.media.limboLight_on2 : cgs.media.limboLight_on);
	}
	else
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboLight_off);
	}
}

/**
 * @brief CG_LimboPanel_BriefingButton_Draw
 * @param[in] button
 */
void CG_LimboPanel_BriefingButton_Draw(panel_button_t *button)
{
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.limboEndCinematicTime > cg.time)
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, BG_CursorInRect(&button->rect) ? cgs.media.limboBriefingButtonStopOn : cgs.media.limboBriefingButtonStopOff);
	}
	else
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, BG_CursorInRect(&button->rect) ? cgs.media.limboBriefingButtonOn : cgs.media.limboBriefingButtonOff);
	}
}

/**
 * @brief CG_LimboPanel_NameEditFinish
 * @param button
 */
void CG_LimboPanel_NameEditFinish(panel_button_t *button)
{
	char buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);
	trap_Cvar_Set("name", buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
 * @brief CG_LimboPanel_CancelButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_CancelButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		//SOUND_CANCEL;
		SOUND_SELECT;

		if (cgs.limboLoadoutModified)
		{
			trap_SendClientCommand("rs");

			cgs.limboLoadoutSelected = qfalse;
		}

		CG_EventHandling(CGAME_EVENT_NONE, qfalse);

		return qtrue;
	}
	return qfalse;
}

/**
 * @brief CG_LimboPanel_PlusButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_PlusButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		cgs.ccZoomFactor /= 0.75f;

		if (cgs.ccZoomFactor > 1.f)
		{
			cgs.ccZoomFactor = 1.f;
		}

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_MinusButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_MinusButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		cgs.ccZoomFactor *= 0.75f;

		if (cgs.ccZoomFactor < (0.75f * 0.75f * 0.75f * 0.75f * 0.75f))
		{
			cgs.ccZoomFactor = (0.75f * 0.75f * 0.75f * 0.75f * 0.75f);
		}

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_SendSetupMsg
 * @param[in] forceteam
 */
void CG_LimboPanel_SendSetupMsg(qboolean forceteam)
{
	weapon_t   weap1, weap2;
	const char *str;
	team_t     team;

	if (forceteam)
	{
		team = CG_LimboPanel_GetTeam();
	}
	else
	{
		team = cgs.clientinfo[cg.clientNum].team;
	}

	if (team == TEAM_SPECTATOR)
	{
		if (forceteam)
		{
			if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
			{
				trap_SendClientCommand("team s 0 0 0");
			}
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return;
	}

	weap1 = cgs.ccSelectedPrimaryWeapon;
	weap2 = cgs.ccSelectedSecondaryWeapon;

	// TODO: handle all case ?
	switch (team)
	{
	case TEAM_AXIS:
		str = "r";
		break;
	case TEAM_ALLIES:
		str = "b";
		break;
	default:
		str = NULL;     // don't go spec
		break;
	}

	// if this happens, we're dazed and confused, abort
	if (!str)
	{
		return;
	}

	trap_SendClientCommand(va("team %s %i %i %i", str, CG_LimboPanel_GetClass(), weap1, weap2));

	if (forceteam)
	{
		CG_EventHandling(CGAME_EVENT_NONE, qfalse);
	}

	// print center message
	switch (CG_LimboPanel_GetTeam())
	{
	case TEAM_AXIS:
		str = "Axis";
		break;
	case TEAM_ALLIES:
		str = "Allied";
		break;
	default:     // default
		str = "unknown";
		break;
	}

	if (cgs.clientinfo[cg.clientNum].skill[SK_HEAVY_WEAPONS] >= 4 && cgs.clientinfo[cg.clientNum].cls == PC_SOLDIER && !Q_stricmp(GetWeaponTableData(weap1)->desc, GetWeaponTableData(weap2)->desc))
	{
		CG_PriorityCenterPrint(va(CG_TranslateString("You will spawn as an %s %s with a %s."), str, BG_ClassnameForNumber(CG_LimboPanel_GetClass()), GetWeaponTableData(weap1)->desc), 400, cg_fontScaleCP.value, -1);
	}
	else
	{
		if (GetWeaponTableData(weap2)->attributes & WEAPON_ATTRIBUT_AKIMBO)
		{
			CG_PriorityCenterPrint(va(CG_TranslateString("You will spawn as an %s %s with a %s and %s."), str, BG_ClassnameForNumber(CG_LimboPanel_GetClass()), GetWeaponTableData(weap1)->desc, GetWeaponTableData(weap2)->desc), 400, cg_fontScaleCP.value, -1);
		}
		else
		{
			CG_PriorityCenterPrint(va(CG_TranslateString("You will spawn as an %s %s with a %s and a %s."), str, BG_ClassnameForNumber(CG_LimboPanel_GetClass()), GetWeaponTableData(weap1)->desc, GetWeaponTableData(weap2)->desc), 400, cg_fontScaleCP.value, -1);
		}
	}

	cgs.limboLoadoutSelected = qtrue;
	cgs.limboLoadoutModified = qtrue;
}

/**
 * @brief CG_LimboPanel_OkButton_KeyDown
 * @param button - unused
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_OkButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		CG_LimboPanel_SendSetupMsg(qtrue);

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_TeamButton_KeyDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_TeamButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		if (cgs.ccSelectedTeam != button->data[0] && !CG_LimboPanel_TeamIsDisabled(teamOrder[button->data[0]]))
		{
			int oldmax  = CG_LimboPanel_GetMaxObjectives();
			int oldteam = cgs.ccSelectedTeam;

			cgs.ccSelectedTeam = button->data[0];

			if (cgs.ccSelectedObjective == oldmax)
			{
				cgs.ccSelectedObjective = CG_LimboPanel_GetMaxObjectives();
			}

			if (teamOrder[button->data[0]] != TEAM_SPECTATOR && CG_LimboPanel_ClassIsDisabled(teamOrder[button->data[0]], CG_LimboPanel_GetClass()))
			{
				cgs.ccSelectedClass = CG_LimboPanel_FindFreeClass(teamOrder[button->data[0]]);
			}

			// reset weapon to default when selecting spectator team
			if (teamOrder[oldteam] == TEAM_SPECTATOR || teamOrder[button->data[0]] == TEAM_SPECTATOR)
			{
				CG_LimboPanel_SetDefaultWeapon(PRIMARY_SLOT);
				CG_LimboPanel_SetDefaultWeapon(SECONDARY_SLOT);
			}
			else
			{
				weapon_t weap;

				weap = CG_LimboPanel_GetSelectedWeapon(PRIMARY_SLOT);

				// get equivalent primary weapon on team swap
				if (!weap)
				{
					CG_LimboPanel_SetDefaultWeapon(PRIMARY_SLOT);
				}
				else if (GetWeaponTableData(weap)->weapEquiv)
				{
					CG_LimboPanel_SetSelectedWeaponNum(PRIMARY_SLOT, GetWeaponTableData(weap)->weapEquiv);
				}

				weap = CG_LimboPanel_GetSelectedWeapon(SECONDARY_SLOT);

				// get equivalent secondary weapon on team swap
				if (!weap)
				{
					CG_LimboPanel_SetDefaultWeapon(SECONDARY_SLOT);
				}
				else if (GetWeaponTableData(weap)->weapEquiv)
				{
					CG_LimboPanel_SetSelectedWeaponNum(SECONDARY_SLOT, GetWeaponTableData(weap)->weapEquiv);
				}
			}

			CG_LimboPanel_RequestWeaponStats();

			cgs.limboLoadoutModified = qtrue;
		}

		return qtrue;
	}

	return qfalse;
}

static vec4_t clrRenderTeamButton2 = { 1.f, 1.f, 1.f, 0.4f };
static vec4_t clrRenderTeamButton4 = { 1.f, 0.f, 0.f, 0.75f };

/**
 * @brief CG_LimboPanel_RenderTeamButton
 * @param[in] button
 */
void CG_LimboPanel_RenderTeamButton(panel_button_t *button)
{
	rectDef_t lock;
	qhandle_t shader;
	qboolean  teamDisabled;

	teamDisabled = CG_LimboPanel_TeamIsDisabled(teamOrder[button->data[0]]);

	trap_R_SetColor(colorBlack);
	CG_DrawPic(button->rect.x + 1, button->rect.y + 1, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_off);

	if (teamDisabled)
	{
		trap_R_SetColor(clrRenderTeamButton4);
	}
	else
	{
		trap_R_SetColor(NULL);
	}

	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_off);

	if (CG_LimboPanel_GetTeam() == teamOrder[button->data[0]])
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_on);
	}
	else if (BG_CursorInRect(&button->rect))
	{
		if (!teamDisabled)
		{
			trap_R_SetColor(clrRenderTeamButton2);
		}
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_on);
		trap_R_SetColor(NULL);
	}

	switch (button->data[0])
	{
	case 0:
		shader = cgs.media.limboTeamButtonAxis;
		break;
	case 1:
		shader = cgs.media.limboTeamButtonAllies;
		break;
	case 2:
		shader = cgs.media.limboTeamButtonSpec;
		break;
	default:
		return;
	}

	trap_R_SetColor(NULL);
	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, shader);

	if (qfalse) // FIXME: There is no lock status transfer yet.
	{
		lock.w = lock.h = button->rect.h * 0.6f;

		lock.x = button->rect.x + (button->rect.w / 2) - (lock.w / 2);
		lock.y = button->rect.y + (button->rect.h / 2) - (lock.h / 2);

		CG_DrawPic(lock.x, lock.y, lock.w, lock.h, cgs.media.limboTeamLocked);
	}
}

/**
 * @brief CG_LimboPanel_ClassButton_KeyDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_ClassButton_KeyDown(panel_button_t *button, int key)
{
	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		return qfalse;
	}


	if (CG_LimboPanel_ClassIsDisabled(CG_LimboPanel_GetTeam(), button->data[1]))
	{
		return qfalse;
	}

	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		if (cgs.ccSelectedClass != button->data[1])
		{
			cgs.ccSelectedClass = button->data[1];

			CG_LimboPanel_SetDefaultWeapon(PRIMARY_SLOT);
			CG_LimboPanel_SetDefaultWeapon(SECONDARY_SLOT);

			CG_LimboPanel_RequestWeaponStats();

			CG_LimboPanel_SendSetupMsg(qfalse);
		}

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_ClassBar_Draw
 * @param[in] button
 */
void CG_LimboPanel_ClassBar_Draw(panel_button_t *button)
{
	const char *text = NULL;
	char       buffer[64];
	float      w;

	if (BG_CursorInRect(&medalPic0.rect))
	{
		text = GetSkillTableData(SK_BATTLE_SENSE)->skillNames;
	}
	else if (BG_CursorInRect(&medalPic1.rect))
	{
		text = GetSkillTableData(SK_EXPLOSIVES_AND_CONSTRUCTION)->skillNames;
	}
	else if (BG_CursorInRect(&medalPic2.rect))
	{
		text = GetSkillTableData(SK_FIRST_AID)->skillNames;
	}
	else if (BG_CursorInRect(&medalPic3.rect))
	{
		text = GetSkillTableData(SK_SIGNALS)->skillNames;
	}
	else if (BG_CursorInRect(&medalPic4.rect))
	{
		text = GetSkillTableData(SK_LIGHT_WEAPONS)->skillNames;
	}
	else if (BG_CursorInRect(&medalPic5.rect))
	{
		text = GetSkillTableData(SK_HEAVY_WEAPONS)->skillNames;
	}
	else if (BG_CursorInRect(&medalPic6.rect))
	{
		text = GetSkillTableData(SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS)->skillNames;
	}
	else if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		text = "JOIN A TEAM";
	}
	else if (BG_CursorInRect(&classButton0.rect))
	{
		text = BG_ClassnameForNumber(PC_SOLDIER);
	}
	else if (BG_CursorInRect(&classButton1.rect))
	{
		text = BG_ClassnameForNumber(PC_MEDIC);
	}
	else if (BG_CursorInRect(&classButton2.rect))
	{
		text = BG_ClassnameForNumber(PC_ENGINEER);
	}
	else if (BG_CursorInRect(&classButton3.rect))
	{
		text = BG_ClassnameForNumber(PC_FIELDOPS);
	}
	else if (BG_CursorInRect(&classButton4.rect))
	{
		text = BG_ClassnameForNumber(PC_COVERTOPS);
	}

	if (!text)
	{
		text = BG_ClassnameForNumber(CG_LimboPanel_GetClass());
	}

	Q_strncpyz(buffer, text, sizeof(buffer));
	Q_strupr(buffer);

	w = CG_Text_Width_Ext(buffer, button->font->scalex, 0, button->font->font);
	CG_Text_Paint_Ext(button->rect.x + (button->rect.w - w) * 0.5f, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString(buffer), 0, 0, button->font->style, button->font->font);
}

static vec4_t clrRenderClassButton  = { 1.f, 1.f, 1.f, 0.4f };
static vec4_t clrRenderClassButton2 = { 1.f, 1.f, 1.f, 0.75f };
static vec4_t clrRenderClassButton3 = { 1.f, 1.f, 1.f, 0.6f };
static vec4_t clrRenderClassButton4 = { 1.f, 0.f, 0.f, 0.5f };

/**
 * @brief CG_LimboPanel_RenderClassButton
 * @param[in] button
 */
void CG_LimboPanel_RenderClassButton(panel_button_t *button)
{
	int   i;
	float s0, t0, s1, t1;
	float x, y, w, h;

	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButton2Back_off);

	if (CG_LimboPanel_GetTeam() != TEAM_SPECTATOR)
	{
		if (button->data[1] == CG_LimboPanel_GetClass())
		{
			CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButton2Back_on);
		}
		else if (BG_CursorInRect(&button->rect))
		{
			trap_R_SetColor(clrRenderClassButton);
			CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButton2Back_on);
			trap_R_SetColor(NULL);
		}
	}

	for (i = 0; i < 4; i++)
	{
		if (cgs.clientinfo[cg.clientNum].skill[BG_ClassSkillForClass(button->data[1])] <= i)
		{
			break;
		}

		if (i == 0 || i == 1)
		{
			s0 = 0.5f;
			s1 = 1.0f;
		}
		else
		{
			s0 = 0.0f;
			s1 = 0.5f;
		}
		if (i == 1 || i == 2)
		{
			t0 = 0.5f;
			t1 = 1.0f;
		}
		else
		{
			t0 = 0.0f;
			t1 = 0.5f;
		}

		x = button->rect.x + button->rect.w * s0;
		y = button->rect.y + button->rect.h * t0;
		w = button->rect.w * 0.5f;
		h = button->rect.h * 0.5f;

		CG_AdjustFrom640(&x, &y, &w, &h);

		if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
		{
			trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, cgs.media.limboClassButton2Wedge_off);
		}
		else
		{
			if (button->data[1] == CG_LimboPanel_GetClass())
			{
				trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, cgs.media.limboClassButton2Wedge_on);
			}
			else if (BG_CursorInRect(&button->rect))
			{
				trap_R_SetColor(clrRenderClassButton3);
				trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, cgs.media.limboClassButton2Wedge_on);
				trap_R_SetColor(NULL);
			}
			else
			{
				trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, cgs.media.limboClassButton2Wedge_off);
			}
		}
	}


	if (CG_LimboPanel_GetTeam() != TEAM_SPECTATOR && button->data[1] == CG_LimboPanel_GetClass())
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButtons2[button->data[1]]);
	}
	else
	{
		if (CG_LimboPanel_GetTeam() != TEAM_SPECTATOR && CG_LimboPanel_ClassIsDisabled(CG_LimboPanel_GetTeam(), button->data[1]))
		{
			trap_R_SetColor(clrRenderClassButton4);
			CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButtons2[button->data[1]]);
		}
		else
		{
			trap_R_SetColor(clrRenderClassButton2);
			CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButtons2[button->data[1]]);
		}
		trap_R_SetColor(NULL);
	}
}

/**
 * @brief CG_LimboPanel_GetMaxObjectives
 * @return
 */
int CG_LimboPanel_GetMaxObjectives(void)
{
	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		return 0;
	}

	return Q_atoi(Info_ValueForKey(CG_ConfigString(CS_MULTI_INFO), "o")); // numobjectives
}

/**
 * @brief CG_LimboPanel_ObjectiveText_KeyDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_ObjectiveText_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		int max = CG_LimboPanel_GetMaxObjectives();

		SOUND_SELECT;

		if (button->data[7] == 0)
		{
			if (++cgs.ccSelectedObjective > max)
			{
				cgs.ccSelectedObjective = 0;
			}
		}
		else
		{
			if (--cgs.ccSelectedObjective < 0)
			{
				cgs.ccSelectedObjective = max;
			}
		}

		CG_LimboPanel_RequestObjective();

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_RenderObjectiveText(panel_button_t *button)
{
	const char *cs;
	char       *info, *s, *p;
	float      y;
	char       buffer[1024];
	int        status = 0;

	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		//cs = CG_ConfigString( CS_MULTI_MAPDESC );
		//Q_strncpyz( buffer, cs, sizeof(buffer) );

		Q_strncpyz(buffer, cg.objMapDescription_Neutral, sizeof(buffer));
	}
	else
	{
		if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
		{
			//cs = CG_ConfigString( CS_MULTI_MAPDESC3 );
			//Q_strncpyz( buffer, cs, sizeof(buffer) );

			Q_strncpyz(buffer, cg.objMapDescription_Neutral, sizeof(buffer));
		}
		else
		{
			if (cgs.ccSelectedObjective != CG_LimboPanel_GetMaxObjectives())
			{
				cs = CG_ConfigString(CS_MULTI_OBJECTIVE);

				if (CG_LimboPanel_GetTeam() == TEAM_AXIS)
				{
					//info = Info_ValueForKey( cs, "axis_desc" );
					info   = cg.objDescription_Axis[cgs.ccSelectedObjective];
					status = Q_atoi(Info_ValueForKey(cs, va("x%i", cgs.ccSelectedObjective + 1)));
				}
				else
				{
					//info = Info_ValueForKey( cs, "allied_desc" );
					info   = cg.objDescription_Allied[cgs.ccSelectedObjective];
					status = Q_atoi(Info_ValueForKey(cs, va("a%i", cgs.ccSelectedObjective + 1)));
				}

				if (!(info && *info))
				{
					info = "No Information Supplied";
				}

				Q_strncpyz(buffer, info, sizeof(buffer));
			}
			else
			{
				if (CG_LimboPanel_GetTeam() == TEAM_AXIS)
				{
					Q_strncpyz(buffer, cg.objMapDescription_Axis, sizeof(buffer));
				}
				else
				{
					Q_strncpyz(buffer, cg.objMapDescription_Allied, sizeof(buffer));
				}
			}
		}
	}

	while ((s = strchr(buffer, '*')))
	{
		*s = '\n';
	}

	CG_FitTextToWidth_Ext(buffer, button->font->scalex, button->rect.w - 16, sizeof(buffer), &cgs.media.limboFont2);

	y = button->rect.y + 12;

	s = p = buffer;
	while (*p)
	{
		if (*p == '\n')
		{
			*p++ = '\0';
			CG_Text_Paint_Ext(button->rect.x + 4, y, button->font->scalex, button->font->scaley, button->font->colour, s, 0, 0, 0, &cgs.media.limboFont2);
			y += 8;
			s  = p;
		}
		else
		{
			p++;
		}
	}

	if (cg_gameType.integer != GT_WOLF_LMS && CG_LimboPanel_GetTeam() != TEAM_SPECTATOR)
	{
		const char *ofTxt;
		float      w, x;

		if (cgs.ccSelectedObjective == CG_LimboPanel_GetMaxObjectives())
		{
			ofTxt = va(CG_TranslateString("1of%i"), CG_LimboPanel_GetMaxObjectives() + 1);
		}
		else
		{
			ofTxt = va(CG_TranslateString("%iof%i"), cgs.ccSelectedObjective + 2, CG_LimboPanel_GetMaxObjectives() + 1);
		}

		w = CG_Text_Width_Ext(ofTxt, 0.2f, 0, &cgs.media.limboFont2);

		x = button->rect.x + button->rect.w - w - 4;

		CG_Text_Paint_Ext(x, button->rect.y + button->rect.h - 2, 0.2f, 0.2f, colorBlack, ofTxt, 0, 0, 0, &cgs.media.limboFont2);
	}

	if (status == 1)
	{
		CG_DrawPic(button->rect.x + 87, button->rect.y + 8, button->rect.w - 174, button->rect.h - 8, cgs.media.ccStamps[0]);
	}
	else if (status == 2)
	{
		CG_DrawPic(button->rect.x + 87, button->rect.y + 8, button->rect.w - 174, button->rect.h - 8, cgs.media.ccStamps[1]);
	}
}

/**
 * @brief CG_LimboPanel_RenderObjectiveBack
 * @param[in] button
 */
void CG_LimboPanel_RenderObjectiveBack(panel_button_t *button)
{
	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboObjectiveBack[TEAM_SPECTATOR - TEAM_AXIS]);
}

/**
 * @brief CG_LimboPanel_RenderCommandMap
 * @param[in] button
 */
void CG_LimboPanel_RenderCommandMap(panel_button_t *button)
{
	CG_DrawMap(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.ccFilter, NULL, qtrue, 1.f, qtrue);
	CG_CommandMap_DrawHighlightText();
}

/**
 * @brief CG_LimboPanel_RenderLight_GetValue
 * @param[in] button
 * @return
 */
qboolean CG_LimboPanel_RenderLight_GetValue(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 0:
		return (CG_LimboPanel_GetClass() == button->data[1]) ? qtrue : qfalse;
	case 1:
		return (CG_LimboPanel_GetTeam() == teamOrder[button->data[1]]) ? qtrue : qfalse;
	default:
		break;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_RenderLight
 * @param[in] button
 * @todo Need cleanup
 */
void CG_LimboPanel_RenderLight(panel_button_t *button)
{
	if (CG_LimboPanel_RenderLight_GetValue(button))
	{
//      if( !button->data[2] || (button->data[2] - cg.time < 0) ) {
		button->data[3] = button->data[3] ^ 1;
//          if( button->data[3] ) {
//              button->data[2] = cg.time + rand() % 200;
//          } else {
//              button->data[2] = cg.time + rand() % 1000;
//          }
//      }

		CG_DrawPic(button->rect.x - 4, button->rect.y - 2, button->rect.w + 4, button->rect.h + 4, button->data[3] ? cgs.media.limboLight_on2 : cgs.media.limboLight_on);
	}
	else
	{
		CG_DrawPic(button->rect.x - 4, button->rect.y - 2, button->rect.w + 4, button->rect.h + 4, cgs.media.limboLight_off);
	}
}

/**
 * @brief CG_DrawPlayerHead
 * @param[in] rect
 * @param[in] character
 * @param[in] headcharacter
 * @param[in] yaw
 * @param[in] pitch
 * @param[in] drawHat
 * @param[in] animation
 * @param[in] painSkin
 * @param[in] rank
 * @param[in] spectator - unused
 * @param[in] team
 */
void CG_DrawPlayerHead(rectDef_t *rect, bg_character_t *character, bg_character_t *headcharacter, float yaw, float pitch, qboolean drawHat, hudHeadAnimNumber_t animation, qhandle_t painSkin, int rank, qboolean spectator, int team)
{
	float       len;
	vec3_t      origin;
	vec3_t      mins, maxs, angles;
	float       x, y, w, h;
	refdef_t    refdef;
	refEntity_t head, hat, mrank;

	if (!character)
	{
		return;
	}

	trap_R_SaveViewParms();

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;

	CG_AdjustFrom640(&x, &y, &w, &h);

	Com_Memset(&refdef, 0, sizeof(refdef));

	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear(refdef.viewaxis);

	refdef.fov_x = 8;
	refdef.fov_y = 10;

	refdef.x      = x;
	refdef.y      = y;
	refdef.width  = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();

	// offset the origin y and z to center the head
	trap_R_ModelBounds(character->hudhead, mins, maxs);

	origin[2] = -0.7f * (mins[2] + maxs[2]);
	origin[1] = 0.5f * (mins[1] + maxs[1]);

	// calculate distance so the head nearly fills the box
	// assume heads are taller than wide
	len       = 3.5f * (maxs[2] - mins[2]);
	origin[0] = len / tan(20 / 2);   // 0.268;  // len / tan( fov/2 )

	angles[PITCH] = pitch;
	angles[YAW]   = yaw;
	angles[ROLL]  = 0;

	Com_Memset(&head, 0, sizeof(head));
	AnglesToAxis(angles, head.axis);
	VectorCopy(origin, head.origin);
	head.hModel     = headcharacter->hudhead;
	head.customSkin = headcharacter->hudheadskin;
	head.renderfx   = RF_NOSHADOW | RF_FORCENOLOD;      // no stencil shadows

	// light the model with the current lightgrid
	//VectorCopy( cg.refdef.vieworg, head.lightingOrigin );
	if (!cg.showGameView)
	{
		head.renderfx |= /*RF_LIGHTING_ORIGIN |*/ RF_MINLIGHT;
	}

	CG_HudHeadAnimation(headcharacter, &cg.predictedPlayerEntity.pe.hudhead, &head.oldframe, &head.frame, &head.backlerp, animation);

	if (drawHat)
	{
		Com_Memset(&hat, 0, sizeof(hat));
		hat.hModel     = character->accModels[ACC_HAT];
		hat.customSkin = character->accSkins[ACC_HAT];
		hat.renderfx   = RF_NOSHADOW | RF_FORCENOLOD;   // no stencil shadows

		// light the model with the current lightgrid
		//VectorCopy( cg.refdef.vieworg, hat.lightingOrigin );
		if (!cg.showGameView)
		{
			hat.renderfx |= /*RF_LIGHTING_ORIGIN |*/ RF_MINLIGHT;
		}

		CG_PositionEntityOnTag(&hat, &head, "tag_mouth", 0, NULL);

		if (rank)
		{
			Com_Memset(&mrank, 0, sizeof(mrank));

			mrank.hModel       = character->accModels[ACC_RANK];
			mrank.customShader = rankicons[rank][team == TEAM_AXIS ? 1 : 0][1].shader;
			mrank.renderfx     = RF_NOSHADOW | RF_FORCENOLOD;   // no stencil shadows

			CG_PositionEntityOnTag(&mrank, &head, "tag_mouth", 0, NULL);
		}
	}

	head.shaderRGBA[0] = 255;
	head.shaderRGBA[1] = 255;
	head.shaderRGBA[2] = 255;
	head.shaderRGBA[3] = 255;

	hat.shaderRGBA[0] = 255;
	hat.shaderRGBA[1] = 255;
	hat.shaderRGBA[2] = 255;
	hat.shaderRGBA[3] = 255;

	mrank.shaderRGBA[0] = 255;
	mrank.shaderRGBA[1] = 255;
	mrank.shaderRGBA[2] = 255;
	mrank.shaderRGBA[3] = 255;

	trap_R_AddRefEntityToScene(&head);

	if (painSkin)
	{
		head.customShader = 0;
		head.customSkin   = painSkin;
		trap_R_AddRefEntityToScene(&head);
	}

	if (drawHat)
	{
		trap_R_AddRefEntityToScene(&hat);

		if (rank)
		{
			trap_R_AddRefEntityToScene(&mrank);
		}
	}
	trap_R_RenderScene(&refdef);

// render to texture api example
// draws the player head on one of the fueldump textures.
#ifdef TEST_API_RENDERTOTEXTURE
	{
		static int texid = 0;

		if (!texid)
		{
			texid = trap_R_GetTextureId("textures/stone/mxsnow3.tga");
		}
		trap_R_RenderToTexture(texid, 0, 0, 256, 256);
	}
#endif

	trap_R_RestoreViewParms();
}

/**
 * @brief CG_LimboPanel_RenderHead
 * @param[in] button
 */
void CG_LimboPanel_RenderHead(panel_button_t *button)
{
	vec4_t clrBackRenderHead = { 0.05f, 0.05f, 0.05f, 1.f };

	if (CG_LimboPanel_GetTeam() != TEAM_SPECTATOR)
	{
		CG_FillRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, clrBackRenderHead);
		CG_DrawPlayerHead(&button->rect, CG_LimboPanel_GetCharacter(), CG_LimboPanel_GetCharacter(), 180, 0, qtrue, HD_IDLE4, 0, cgs.clientinfo[cg.clientNum].rank, qfalse, CG_LimboPanel_GetTeam());
	}
	else
	{
		// TODO:
		//CG_FillRect( button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorBlack );
		//CG_DrawPlayerHead( &button->rect, BG_GetCharacter( TEAM_ALLIES, PC_SOLDIER ), BG_GetCharacter( TEAM_ALLIES, PC_SOLDIER ), 180, 0, qtrue, HD_IDLE4, 0, 0, qtrue );

		if (cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboShoutcaster);
		}
		else
		{
			CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboSpectator);
		}
	}

	VectorSet(clrBackRenderHead, .6f, .6f, .6f);
	trap_R_SetColor(clrBackRenderHead);

	// top / bottom
	CG_DrawPic(button->rect.x, button->rect.y - 2, button->rect.w, 2, cgs.media.limboWeaponCardSurroundH);
	CG_DrawPicST(button->rect.x, button->rect.y + button->rect.h, button->rect.w, 2, 0.f, 1.f, 1.f, 0.f, cgs.media.limboWeaponCardSurroundH);

	CG_DrawPic(button->rect.x - 2, button->rect.y, 2, button->rect.h, cgs.media.limboWeaponCardSurroundV);
	CG_DrawPicST(button->rect.x +  button->rect.w, button->rect.y, 2, button->rect.h, 1.f, 0.f, 0.f, 1.f, cgs.media.limboWeaponCardSurroundV);

	CG_DrawPicST(button->rect.x - 2, button->rect.y - 2, 2, 2, 0.f, 0.f, 1.f, 1.f, cgs.media.limboWeaponCardSurroundC);
	CG_DrawPicST(button->rect.x + button->rect.w, button->rect.y - 2, 2, 2, 1.f, 0.f, 0.f, 1.f, cgs.media.limboWeaponCardSurroundC);
	CG_DrawPicST(button->rect.x + button->rect.w, button->rect.y + button->rect.h, 2, 2, 1.f, 1.f, 0.f, 0.f, cgs.media.limboWeaponCardSurroundC);
	CG_DrawPicST(button->rect.x - 2, button->rect.y + button->rect.h, 2, 2, 0.f, 1.f, 1.f, 0.f, cgs.media.limboWeaponCardSurroundC);

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_LimboPanel_Filter_KeyDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_Filter_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_FILTER;

		cgs.ccFilter ^= (1 << button->data[0]);
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_Filter_Draw
 * @param[in] button
 */
void CG_LimboPanel_Filter_Draw(panel_button_t *button)
{
	if (cgs.ccFilter & (1 << button->data[0]))
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.ccFilterBackOff);
	}
	else
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.ccFilterBackOn);
	}

	CG_DrawPic(button->rect.x + 1, button->rect.y + 1, button->rect.w - 2, button->rect.h - 2, cgs.media.ccFilterPics[button->data[0]]);
}

/**
 * @brief CG_LimboPanel_RenderSkillIcon
 * @param[in] button
 */
void CG_LimboPanel_RenderSkillIcon(panel_button_t *button)
{
	qhandle_t shader;

	if (cg_gameType.integer == GT_WOLF_LMS /*|| CG_LimboPanel_GetTeam() == TEAM_SPECTATOR*/)
	{
		return;
	}

	switch (button->data[0])
	{
	case 0:
		shader = cgs.media.limboSkillsBS;
		break;
	case 1:
		shader = cgs.media.limboSkillsLW;
		break;
	case 2:
		shader = cgs.media.limboClassButtons[CG_LimboPanel_GetClass()];
		break;
	default:
		return;
	}

	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, shader);
}

#ifdef FEATURE_PRESTIGE
/**
 * @brief CG_LimboPanel_Prestige_Draw
 * @param[in] button
 */
void CG_LimboPanel_Prestige_Draw(panel_button_t *button)
{
	const char *text = NULL;
	float      w;

	if (cg_gameType.integer == GT_WOLF_STOPWATCH || cg_gameType.integer == GT_WOLF_LMS || cg_gameType.integer == GT_WOLF_CAMPAIGN)
	{
		return;
	}

	if (!cgs.prestige)
	{
		return;
	}

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	text = va("%3i", cgs.clientinfo[cg.clientNum].prestige);

	w = CG_Text_Width_Ext(text, button->font->scalex, 0, button->font->font);
	CG_Text_Paint_Ext(button->rect.x + (button->rect.w - w) * 0.5f, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString(text), 0, 0, button->font->style, button->font->font);
}

/**
 * @brief CG_LimboPanel_RenderPrestigeIcon
 * @param[in] button
 */
void CG_LimboPanel_RenderPrestigeIcon(panel_button_t *button)
{
	int i, j, skillMax, cnt = 0;

	if (cg_gameType.integer == GT_WOLF_STOPWATCH || cg_gameType.integer == GT_WOLF_LMS || cg_gameType.integer == GT_WOLF_CAMPAIGN)
	{
		return;
	}

	if (!cgs.prestige)
	{
		return;
	}

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
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
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.prestigePics[0]);
	}
	else
	{
		trap_R_SetColor(colorYellow);
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.prestigePics[2]);
		trap_R_SetColor(NULL);
	}
}
#endif

/**
 * @brief CG_LimboPanel_WeaponLights_KeyDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_WeaponLights_KeyDown(panel_button_t *button, int key)
{
	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		cgs.ccSelectedWeaponSlot = button->data[0];
		CG_LimboPanel_RequestWeaponStats();
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_WeaponLights
 * @param[in] button
 */
void CG_LimboPanel_WeaponLights(panel_button_t *button)
{
	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponNumber_off);
	}
	else
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, button->data[0] == cgs.ccSelectedWeaponSlot ? cgs.media.limboWeaponNumber_on : cgs.media.limboWeaponNumber_off);
	}
}

/**
 * @brief CG_LimboPanel_WeaponPanel_KeyDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_WeaponPanel_KeyDown(panel_button_t *button, int key)
{
	button->data[7] = 0;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		BG_PanelButtons_SetFocusButton(button);
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_WeaponPanel_KeyUp
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_LimboPanel_WeaponPanel_KeyUp(panel_button_t *button, int key)
{
	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	if (key == K_MOUSE1)
	{
		if (BG_PanelButtons_GetFocusButton() == button)
		{
			rectDef_t rect;
			int       cnt;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));
			rect.y -= rect.h;   // skip first (0) weap

			for (cnt = 1; button->data[cnt] != 0; cnt++, rect.y -= rect.h)
			{
				if (!BG_CursorInRect(&rect))
				{
					continue;
				}

				CG_LimboPanel_SetSelectedWeaponNum(cgs.ccSelectedWeaponSlot, (weapon_t)button->data[cnt]);

				if (!CG_LimboPanel_IsValidSelectedWeapon(SECONDARY_SLOT))
				{
					CG_LimboPanel_SetDefaultWeapon(SECONDARY_SLOT);
				}

				CG_LimboPanel_RequestWeaponStats();

				break;
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

static vec4_t clrDrawWeapon = { 1.f, 1.f, 1.f, 0.6f };

/**
 * @brief CG_LimboPanel_WeaponPanel_DrawWeapon
 * @param[in] rect
 * @param[in] weap
 * @param[in] highlight
 * @param[in] ofTxt
 * @param[in] disabled
 */
void CG_LimboPanel_WeaponPanel_DrawWeapon(rectDef_t *rect, weapon_t weap, qboolean highlight, const char *ofTxt, qboolean disabled)
{
	int    width;
	float  x;
	float  x2;
	float  y2;
	float  w;
	float  h;
	vec4_t clr;

	width = CG_Text_Width_Ext(ofTxt, 0.2f, 0, &cgs.media.limboFont2);
	x     = rect->x + rect->w - width - 4;

	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.limboWeaponCard);

	if (highlight && BG_CursorInRect(rect))
	{
		Vector4Copy(weaponPanelNameFont.colour, clr);
		clr[3] *= 1.5f;
		CG_Text_Paint_Ext(rect->x + 4, rect->y + 12, weaponPanelNameFont.scalex, weaponPanelNameFont.scaley, clr, GetWeaponTableData(weap)->desc, 0, 0, weaponPanelNameFont.style, weaponPanelNameFont.font);
	}
	else
	{
		CG_Text_Paint_Ext(rect->x + 4, rect->y + 12, weaponPanelNameFont.scalex, weaponPanelNameFont.scaley, weaponPanelNameFont.colour, GetWeaponTableData(weap)->desc, 0, 0, weaponPanelNameFont.style, weaponPanelNameFont.font);
	}

	x2 = rect->x;
	y2 = rect->y + (rect->h * 0.25f);
	w  = cg_weapons[weap].weaponCardScale[0] * rect->w;
	h  = cg_weapons[weap].weaponCardScale[1] * rect->h * 0.75f;

	trap_R_SetColor(NULL);

	CG_DrawPicST(x2, y2, w, h,
	             cg_weapons[weap].weaponCardPointS[0], cg_weapons[weap].weaponCardPointT[0],
	             cg_weapons[weap].weaponCardPointS[1], cg_weapons[weap].weaponCardPointT[1],
	             cg_weapons[weap].weaponCardIcon);

	if (disabled)
	{
		trap_R_SetColor(clrDrawWeapon);
		CG_DrawPic(x2, y2 + 4 + (h - 16) * 0.5f, w, 16, cgs.media.limboWeaponCardOOS);
		trap_R_SetColor(NULL);
	}

	CG_Text_Paint_Ext(x, rect->y + rect->h - 2, 0.2f, 0.2f, colorBlack, ofTxt, 0, 0, 0, &cgs.media.limboFont2);
}

static vec4_t clrBackBorder  = { 0.1f, 0.1f, 0.1f, 1.f };
static vec4_t clrBackBorder2 = { 0.2f, 0.2f, 0.2f, 1.f };

#define BRDRSIZE 4

/**
 * @brief CG_DrawBorder
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] fill
 * @param[in] drawMouseOver
 */
void CG_DrawBorder(float x, float y, float w, float h, qboolean fill, qboolean drawMouseOver)
{
	// top / bottom
	CG_DrawPic(x, y - BRDRSIZE, w, BRDRSIZE, cgs.media.limboWeaponCardSurroundH);
	CG_DrawPicST(x, y + h, w, BRDRSIZE, 0.f, 1.f, 1.f, 0.f, cgs.media.limboWeaponCardSurroundH);

	CG_DrawPic(x - BRDRSIZE, y, BRDRSIZE, h, cgs.media.limboWeaponCardSurroundV);
	CG_DrawPicST(x + w, y, BRDRSIZE, h, 1.f, 0.f, 0.f, 1.f, cgs.media.limboWeaponCardSurroundV);

	CG_DrawPicST(x - BRDRSIZE, y - BRDRSIZE, BRDRSIZE, BRDRSIZE, 0.f, 0.f, 1.f, 1.f, cgs.media.limboWeaponCardSurroundC);
	CG_DrawPicST(x + w, y - BRDRSIZE, BRDRSIZE, BRDRSIZE, 1.f, 0.f, 0.f, 1.f, cgs.media.limboWeaponCardSurroundC);
	CG_DrawPicST(x + w, y + h, BRDRSIZE, BRDRSIZE, 1.f, 1.f, 0.f, 0.f, cgs.media.limboWeaponCardSurroundC);
	CG_DrawPicST(x - BRDRSIZE, y + h, BRDRSIZE, BRDRSIZE, 0.f, 1.f, 1.f, 0.f, cgs.media.limboWeaponCardSurroundC);

	if (fill)
	{
		if (drawMouseOver)
		{
			rectDef_t rect;

			rect.x = x;
			rect.y = y;
			rect.w = w;
			rect.h = h;

			if (BG_CursorInRect(&rect))
			{
				CG_FillRect(x, y, w, h, clrBackBorder2);
			}
			else
			{
				CG_FillRect(x, y, w, h, clrBackBorder);
			}
		}
		else
		{
			CG_FillRect(x, y, w, h, clrBackBorder);
		}
	}
}

/**
 * @brief CG_LimboPanel_Border_Draw
 * @param[in] button
 */
void CG_LimboPanel_Border_Draw(panel_button_t *button)
{
	CG_DrawBorder(button->rect.x, button->rect.y, button->rect.w, button->rect.h, qtrue, qtrue);
}

static vec4_t clrWeaponPanel  = { 0.f, 0.f, 0.f, 0.4f };
static vec4_t clrWeaponPanel2 = { 1.f, 1.f, 1.f, 0.4f };

/**
 * @brief CG_LimboPanel_WeaponPanel
 * @param[in] button
 */
void CG_LimboPanel_WeaponPanel(panel_button_t *button)
{
	bg_playerclass_t *classInfo;
	weapon_t         weap;
	int              i, x, cnt = 0, totalCnt;
	rectDef_t        rect;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponCard);

		trap_R_SetColor(clrWeaponPanel);
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponBlendThingy);
		trap_R_SetColor(NULL);

		CG_Text_Paint_Ext(button->rect.x + 4, button->rect.y + 12, weaponPanelNameFont.scalex, weaponPanelNameFont.scaley, weaponPanelNameFont.colour, cgs.clientinfo[cg.clientNum].shoutcaster ? "SHOUTCASTER" : "SPECTATOR", 0, 0, weaponPanelNameFont.style, weaponPanelNameFont.font);

		return;
	}

	classInfo = CG_LimboPanel_GetPlayerClass();
	if (!classInfo)
	{
		return;
	}

	weap     = CG_LimboPanel_GetSelectedWeapon(cgs.ccSelectedWeaponSlot);
	totalCnt = CG_LimboPanel_WeaponCount(cgs.ccSelectedWeaponSlot);

	Com_Memcpy(&rect, &button->rect, sizeof(rect));
	rect.y -= rect.h;

	Com_Memset(button->data, 0, sizeof(button->data));

	for (i = 0, x = 1; i < MAX_WEAPS_PER_CLASS; i++)
	{
		weapon_t cycleWeap;

		if (cgs.ccSelectedWeaponSlot == PRIMARY_SLOT)
		{
			// is player had the minimum level required to use this weapon
			if (cgs.clientinfo[cg.clientNum].skill[classInfo->classPrimaryWeapons[i].skill] < classInfo->classPrimaryWeapons[i].minSkillLevel)
			{
				continue;
			}

			cycleWeap = classInfo->classPrimaryWeapons[i].weapon;
		}
		else
		{
			// is player had the minimum level required to use this weapon
			if (cgs.clientinfo[cg.clientNum].skill[classInfo->classSecondaryWeapons[i].skill] < classInfo->classSecondaryWeapons[i].minSkillLevel)
			{
				continue;
			}

			// if player handling a similar weapon in primary slot, don't show it
			if (classInfo->classSecondaryWeapons[i].weapon == cgs.ccSelectedPrimaryWeapon)
			{
				continue;
			}

			cycleWeap = classInfo->classSecondaryWeapons[i].weapon;
		}

		if (cycleWeap)
		{
			++cnt;

			if (cycleWeap == weap)
			{
				CG_LimboPanel_WeaponPanel_DrawWeapon(&button->rect, weap, totalCnt > 1 ? qtrue : qfalse, va(CG_TranslateString("%iof%i"), cnt, totalCnt), CG_LimboPanel_RealWeaponIsDisabled(weap));
				button->data[0] = weap;
			}
			else if (BG_PanelButtons_GetFocusButton() == button)
			{
				CG_LimboPanel_WeaponPanel_DrawWeapon(&rect, cycleWeap, qtrue, va(CG_TranslateString("%iof%i"), cnt, totalCnt), CG_LimboPanel_RealWeaponIsDisabled(cycleWeap));

				button->data[x] = cycleWeap;

				rect.y -= rect.h;
				x++;
			}
		}
	}

	// render in expanded mode ^
	if (BG_PanelButtons_GetFocusButton() == button && totalCnt > 1)
	{
		CG_DrawBorder(button->rect.x, button->rect.y - ((totalCnt - 1) * button->rect.h), button->rect.w, button->rect.h * totalCnt, qfalse, qfalse);
	}
	else    // render in normal mode
	{
		if (totalCnt <= 1 || !BG_CursorInRect(&button->rect))
		{
			trap_R_SetColor(clrWeaponPanel2);
		}
		CG_DrawPic(button->rect.x + button->rect.w - 20, button->rect.y + 4, 16, 12, cgs.media.limboWeaponCardArrow);


		trap_R_SetColor(clrWeaponPanel);
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponBlendThingy);
		trap_R_SetColor(NULL);
	}
}

/**
 * @brief CG_LimboPanel_RenderCounterNumber
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] number
 * @param[in] shaderBack
 * @param[in] shaderRoll
 * @param[in] numbuttons
 */
void CG_LimboPanel_RenderCounterNumber(float x, float y, float w, float h, float number, qhandle_t shaderBack, qhandle_t shaderRoll, int numbuttons)
{
	float numberS = (((numbuttons - 1) - number) + 0) * (1.f / numbuttons);
	float numberE = (((numbuttons - 1) - number) + 1) * (1.f / numbuttons);

	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, shaderBack);
	trap_R_DrawStretchPic(x, y, w, h, 0, numberS, 1, numberE, shaderRoll);
}

/**
 * @brief CG_LimboPanel_RenderCounter_ValueForButton
 * @param[in] button
 * @return
 */
int CG_LimboPanel_RenderCounter_ValueForButton(panel_button_t *button)
{
	int i, count = 0;

	switch (button->data[0])
	{
	case 0:     // class counts
		if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR || CG_LimboPanel_GetRealTeam() != CG_LimboPanel_GetTeam())
		{
			return 0;     // dont give class counts unless we are on that team (or spec)
		}
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (!cgs.clientinfo[i].infoValid)
			{
				continue;
			}
			if (cgs.clientinfo[i].team != CG_LimboPanel_GetTeam() || cgs.clientinfo[i].cls != button->data[1])
			{
				continue;
			}

			count++;
		}
		return count;
	case 1:     // team counts
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (!cgs.clientinfo[i].infoValid)
			{
				continue;
			}

			if (cgs.clientinfo[i].team != teamOrder[button->data[1]])
			{
				continue;
			}

			count++;
		}
		return count;
	case 2:     // xp
		return cg.xp;
	case 3:     // respawn time
		if (cgs.gamestate != GS_PLAYING)
		{
			if (cg.warmup)
			{
				return (cg.warmup - cg.time) / 1000;
			}

			return 0;
		}
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
		{
			return 0;
		}
		return (int)CG_CalculateReinfTime_Float(qtrue);
	case 4:     // skills
		switch (button->data[1])
		{
		case 0:
			count = cgs.clientinfo[cg.clientNum].skill[SK_BATTLE_SENSE];
			break;
		case 1:
			count = cgs.clientinfo[cg.clientNum].skill[SK_LIGHT_WEAPONS];
			break;
		case 2:
			count = cgs.clientinfo[cg.clientNum].skill[BG_ClassSkillForClass(CG_LimboPanel_GetClass())];
			break;
		default:
			break;
		}
		return (1 << count) - 1;
	case 5:     // clock
		if (cgs.gamestate != GS_PLAYING)
		{
			count = cgs.timelimit * 60;
			switch (button->data[1])
			{
			case 0:         // secs
				return count % 60;
			case 1:         // mins
				return count / 60;
			default:
				break;
			}
			return 0;
		}
		if (cgs.timelimit == 0.f)
		{
			return 0;
		}
		count = ((cgs.timelimit * 60000) - (cg.time - cgs.levelStartTime)) / 1000; // 60 * 1000
		switch (button->data[1])
		{
		case 0:         // secs
			return count % 60;
		case 1:         // mins
			return count / 60;
		default:
			break;
		}
		return 0;
	case 6:     // stats
		switch (button->data[1])
		{
		case 0:
			return cgs.ccWeaponShots;
		case 1:
			return cgs.ccWeaponHits;
		case 2:
			return cgs.ccWeaponShots != 0 ? 100 * cgs.ccWeaponHits / cgs.ccWeaponShots : 0;
		default:
			break;
		}
		return 0;
	default:
		break;
	}

	return 0;
}

/**
 * @brief CG_LimboPanel_RenderCounter_RollTimeForButton
 * @param[in] button
 * @return
 */
int CG_LimboPanel_RenderCounter_RollTimeForButton(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 0:     // class counts
	case 1:     // team counts
		return 100;

	case 4:     // skills
		return 1000;

	case 6:     // stats
	{
		float diff;

		diff = Q_fabs(button->data[3] - CG_LimboPanel_RenderCounter_ValueForButton(button));
		if (diff < 5)
		{
			return (int)(200.f / diff);
		}

		return 50;
	}
	case 5:     // clock
	case 3:     // respawn time
	case 2:     // xp
		return 50.f;
	default:
		break;
	}

	return 1000;
}

/**
 * @brief CG_LimboPanel_RenderCounter_MaxChangeForButton
 * @param[in] button
 * @return
 */
int CG_LimboPanel_RenderCounter_MaxChangeForButton(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 2:     // xp
	case 6:     // stats
		return 5;
	default:
		break;
	}

	return 1;
}

/**
 * @brief CG_LimboPanel_RenderCounter_NumRollers
 * @param[in] button
 * @return
 */
int CG_LimboPanel_RenderCounter_NumRollers(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 0:     // class counts
	case 1:     // team counts
	case 5:     // clock
	case 3:     // respawn time
		return 2;
	case 4:     // skills
		if (cg_gameType.integer == GT_WOLF_LMS /*|| CG_LimboPanel_GetTeam() == TEAM_SPECTATOR*/)
		{
			return 0;
		}
		return 4;
	case 6:     // stats
		switch (button->data[1])
		{
		case 0:
		case 1:
			return 4;
		case 2:
			return 3;
		default:
			break;
		}
		break;
	case 2:     // xp
		if (cg_gameType.integer == GT_WOLF_LMS)
		{
			return 0;
		}
		return 4;
	default:
		break;
	}

	return 0;
}

/**
 * @brief CG_LimboPanel_RenderCounter_CountsDown
 * @param[in] button
 * @return
 */
qboolean CG_LimboPanel_RenderCounter_CountsDown(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 4:     // skill
	case 2:     // xp
		return qfalse;
	default:
		break;
	}

	return qtrue;
}

/**
 * @brief CG_LimboPanel_RenderCounter_CountsUp
 * @param[in] button
 * @return
 */
qboolean CG_LimboPanel_RenderCounter_CountsUp(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 4:     // skill
	case 3:     // respawn time
	case 5:     // clock
		return qfalse;
	default:
		break;
	}

	return qtrue;
}

/**
 * @brief CG_LimboPanel_RenderCounter_StartSet
 * @param[in] button
 * @return
 */
qboolean CG_LimboPanel_RenderCounter_StartSet(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 3:     // respawn time
	case 5:     // clock
		return qtrue;
	default:
		break;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_RenderMedal
 * @param[in] button
 */
void CG_LimboPanel_RenderMedal(panel_button_t *button)
{
	if (cg_gameType.integer == GT_WOLF || cg_gameType.integer == GT_WOLF_MAPVOTE)
	{
		return;
	}

	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.medal_back);
	if (cgs.clientinfo[cg.clientNum].medals[button->data[0]])
	{
		CG_DrawPic(button->rect.x - 2, button->rect.y, button->rect.w + 4, button->rect.h, cgs.media.medals[button->data[0]]);
	}
}

#ifdef FEATURE_PRESTIGE
/**
 * @brief CG_LimboPanel_RenderPrestige
 * @param[in] button
 */
void CG_LimboPanel_RenderPrestige(panel_button_t *button)
{
	qhandle_t shader;
	vec4_t    color = { 1.0f, 1.0f, 1.0f, 0.4f };
	int       i, skillMax = 0;

	if (cgs.gametype == GT_WOLF_CAMPAIGN || cgs.gametype == GT_WOLF_STOPWATCH || cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (!cgs.prestige)
	{
		return;
	}

	switch (button->data[0])
	{
		case 0:
			shader = cgs.media.limboSkillsBS;
			break;
		case 1:
			shader = cgs.media.limboClassButtons[PC_ENGINEER];
			break;
		case 2:
			shader = cgs.media.limboClassButtons[PC_MEDIC];
			break;
		case 3:
			shader = cgs.media.limboClassButtons[PC_FIELDOPS];
			break;
		case 4:
			shader = cgs.media.limboSkillsLW;
			break;
		case 5:
			shader = cgs.media.limboClassButtons[PC_SOLDIER];
			break;
		case 6:
			shader = cgs.media.limboClassButtons[PC_COVERTOPS];
			break;
		default:
			return;
	}

	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboObjectiveBack[2]);

	// check skill max level
	for (i = NUM_SKILL_LEVELS - 1; i >= 0; i--)
	{
		if (GetSkillTableData(button->data[0])->skillLevels[i] >= 0)
		{
			skillMax = i;
			break;
		}
	}

	if (cgs.clientinfo[cg.clientNum].skill[button->data[0]] >= skillMax)
	{
		trap_R_SetColor(color);
		CG_DrawPic(button->rect.x + 2, button->rect.y + 4, button->rect.w - 4, button->rect.h - 8, shader);
		trap_R_SetColor(NULL);
		CG_DrawPic(button->rect.x + 2, button->rect.y + 4, button->rect.w - 4, button->rect.h - 8, cgs.media.ccStamps[0]);
	}
	else
	{
		color[3] = 0.5f;
		trap_R_SetColor(color);
		CG_DrawPic(button->rect.x + 2, button->rect.y + 4, button->rect.w - 4, button->rect.h - 8, shader);
		trap_R_SetColor(NULL);
	}
}
#endif

/**
 * @brief CG_LimboPanel_RenderCounter_IsReversed
 * @param[in] button
 * @return
 */
qboolean CG_LimboPanel_RenderCounter_IsReversed(panel_button_t *button)
{
	switch (button->data[0])
	{
	case 4:     // skill
		return qtrue;
	default:
		break;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_RenderCounter_GetShaders
 * @param[in] button
 * @param[in] shaderBack
 * @param[in] shaderRoll
 * @param[in] numimages
 */
void CG_LimboPanel_RenderCounter_GetShaders(panel_button_t *button, qhandle_t *shaderBack, qhandle_t *shaderRoll, int *numimages)
{
	switch (button->data[0])
	{
	case 4:     // skills
		*shaderBack = cgs.media.limboStar_back;
		*shaderRoll = cgs.media.limboStar_roll;
		*numimages  = 2;
		return;
	default:
		*shaderBack = cgs.media.limboNumber_back;
		*shaderRoll = cgs.media.limboNumber_roll;
		*numimages  = 10;
		return;
	}
}

/**
 * @brief CG_LimboPanelRenderText_NoLMS
 * @param[in] button
 */
void CG_LimboPanelRenderText_NoLMS(panel_button_t *button)
{
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		return;
	}

	BG_PanelButtonsRender_Text(button);
}

/**
 * @brief CG_LimboPanelRenderText_SkillsText
 * @param[in] button
 */
void CG_LimboPanelRenderText_SkillsText(panel_button_t *button)
{
	if (cg_gameType.integer == GT_WOLF_LMS /*|| CG_LimboPanel_GetTeam() == TEAM_SPECTATOR*/)
	{
		return;
	}

	BG_PanelButtonsRender_Text(button);
}

#define MAX_ROLLERS 8
#define COUNTER_ROLLTOTAL (cg.time - button->data[4])
/**
 * @brief CG_LimboPanel_RenderCounter
 * @param[in] button
 * @note This function is mental, i love it :)
 */
void CG_LimboPanel_RenderCounter(panel_button_t *button)
{
	float     x, w;
	float     count[MAX_ROLLERS];
	int       i, j;
	qhandle_t shaderBack;
	qhandle_t shaderRoll;
	int       numimages;
	float     counter_rolltime;
	int       num, value;

	counter_rolltime = CG_LimboPanel_RenderCounter_RollTimeForButton(button);
	num              = CG_LimboPanel_RenderCounter_NumRollers(button);
	value            = CG_LimboPanel_RenderCounter_ValueForButton(button);

	if (num > MAX_ROLLERS)
	{
		num = MAX_ROLLERS;
	}

	CG_LimboPanel_RenderCounter_GetShaders(button, &shaderBack, &shaderRoll, &numimages);

	if (COUNTER_ROLLTOTAL < counter_rolltime)
	{
		// we're rolling
		float frac = (COUNTER_ROLLTOTAL / counter_rolltime);
		int   valueOld, valueNew;

		for (i = 0, j = 1; i < num; i++, j *= numimages)
		{
			valueOld = (button->data[3] / j) % numimages;
			valueNew = (button->data[5] / j) % numimages;

			if (valueNew == valueOld)
			{
				count[i] = valueOld;
			}
			else if ((valueNew > valueOld) != (button->data[5] > button->data[3]))
			{
				// we're flipping around so....
				if (button->data[5] > button->data[3])
				{
					count[i] = valueOld + frac;
				}
				else
				{
					count[i] = valueOld - frac;
				}
			}
			else
			{
				// normal flip
				count[i] = valueOld + ((valueNew - valueOld) * frac);
			}
		}
	}
	else
	{
		if (button->data[3] != button->data[5])
		{
			button->data[3] = button->data[5];
		}
		else if (value != button->data[3])
		{
			int maxchange = abs(value - button->data[3]);

			if (maxchange > CG_LimboPanel_RenderCounter_MaxChangeForButton(button))
			{
				maxchange = CG_LimboPanel_RenderCounter_MaxChangeForButton(button);
			}

			if (value > button->data[3])
			{
				if (CG_LimboPanel_RenderCounter_CountsUp(button))
				{
					button->data[5] = button->data[3] + maxchange;
				}
				else
				{
					//button->data[3] =
					button->data[5] = value;
				}
			}
			else
			{
				if (CG_LimboPanel_RenderCounter_CountsDown(button))
				{
					button->data[5] = button->data[3] - maxchange;
				}
				else
				{
					//button->data[3] =
					button->data[5] = value;
				}
			}
			button->data[4] = cg.time;
		}

		for (i = 0, j = 1; i < num; i++, j *= numimages)
		{
			count[i] = (button->data[3] / j);
		}
	}

	x = button->rect.x;

	if (num > 1)
	{
		w = button->rect.w / (float)num;
	}
	else
	{
		w = button->rect.w;
	}

	if (CG_LimboPanel_RenderCounter_IsReversed(button))
	{
		for (i = 0; i < num; i++)
		{
			CG_LimboPanel_RenderCounterNumber(x, button->rect.y, w, button->rect.h, count[i], shaderBack, shaderRoll, numimages);

			x += w + button->data[6];
		}
	}
	else
	{
		for (i = num - 1; i >= 0; i--)
		{
			CG_LimboPanel_RenderCounterNumber(x, button->rect.y, w, button->rect.h, count[i], shaderBack, shaderRoll, numimages);

			x += w + button->data[6];
		}
	}

	if (button->data[0] == 0 || button->data[0] == 1)
	{
		CG_DrawPic(button->rect.x - 2, button->rect.y - 2, button->rect.w * 1.4f, button->rect.h + 7, cgs.media.limboCounterBorder);
	}
}

/**
 * @brief CG_LimboPanel_Setup
 */
void CG_LimboPanel_Setup(void)
{
	panel_button_t *button;
	panel_button_t **buttons = limboPanelButtons;
	clientInfo_t   *ci       = &cgs.clientinfo[cg.clientNum];
	int            i;
	char           buffer[256];

	cgs.limboLoadoutModified = qfalse;

	trap_Cvar_VariableStringBuffer("name", buffer, 256);
	trap_Cvar_Set("limboname", buffer);

	if (cgs.ccLayers)
	{
		cgs.ccSelectedLayer = CG_CurLayerForZ((int)cg.predictedPlayerEntity.lerpOrigin[2]);
	}

	for ( ; *buttons; buttons++)
	{
		button = (*buttons);

		if (button->onDraw == CG_LimboPanel_RenderCounter)
		{
			if (CG_LimboPanel_RenderCounter_StartSet(button))
			{
				button->data[3] = button->data[5] = CG_LimboPanel_RenderCounter_ValueForButton(button);
				button->data[4] = 0;
			}
		}
	}

	if (!cgs.limboLoadoutSelected)
	{
		// check selected team before selecting weapon to ensure it is properly set after next map / map restart
		// and to check if the selected weapon is still valid and linked to the correct team
		for (i = 0; i < 3; i++)
		{
			if (teamOrder[i] == ci->team)
			{
				cgs.ccSelectedTeam = i;
			}
		}

		if (ci->team != TEAM_SPECTATOR)
		{
			cgs.ccSelectedClass = ci->cls;
		}

		CG_LimboPanel_SetSelectedWeaponNum(PRIMARY_SLOT, (weapon_t)cgs.clientinfo[cg.clientNum].latchedweapon);

		if (!CG_LimboPanel_IsValidSelectedWeapon(PRIMARY_SLOT) || CG_LimboPanel_RealWeaponIsDisabled(cgs.ccSelectedPrimaryWeapon))
		{
			CG_LimboPanel_SetDefaultWeapon(PRIMARY_SLOT);
		}

		if (!CG_LimboPanel_IsValidSelectedWeapon(SECONDARY_SLOT))
		{
			CG_LimboPanel_SetDefaultWeapon(SECONDARY_SLOT);
		}
	}

	cgs.ccRequestedObjective = cgs.ccSelectedObjective = CG_LimboPanel_GetMaxObjectives();
	CG_LimboPanel_RequestObjective();

	cgs.ccSelectedObjective  = CG_LimboPanel_GetMaxObjectives();
	cgs.ccSelectedWeaponSlot = PRIMARY_SLOT;

	CG_LimboPanel_RequestWeaponStats();
}

/**
 * @brief CG_LimboPanel_Init
 */
void CG_LimboPanel_Init(void)
{
	BG_PanelButtonsSetup(limboPanelButtons);
	C_PanelButtonsSetup(limboPanelButtons, cgs.wideXoffset);    // convert to possible widescreen coordinates..
}

/**
 * @brief CG_LimboPanel_Draw
 * @return
 */
qboolean CG_LimboPanel_Draw(void)
{
	static panel_button_t *lastHighlight;
	panel_button_t        *hilight;

	hilight = BG_PanelButtonsGetHighlightButton(limboPanelButtons);
	if (hilight && hilight != lastHighlight)
	{
		lastHighlight = hilight;
	}

	if (cg.limboEndCinematicTime > cg.time)
	{
		CG_DrawPic(LIMBO_3D_X + 4 + cgs.wideXoffset, LIMBO_3D_Y - 8, LIMBO_3D_W - 8, LIMBO_3D_W - 8, cgs.media.limboRadioBroadcast);
	}

	BG_PanelButtonsRender(limboPanelButtons);

	trap_R_SetColor(NULL);
	CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);

	if (cgs.ccRequestedObjective != -1)
	{
		if (cg.time - cgs.ccLastObjectiveRequestTime > 1000)
		{
			if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
			{
				if (cgs.ccCurrentCamObjective != -1 || cgs.ccPortalEnt != -1)
				{
					CG_LimboPanel_RequestObjective();
				}
			}
			else
			{
				if ((cgs.ccRequestedObjective == cgs.ccSelectedObjective && (cgs.ccCurrentCamObjective != cgs.ccRequestedObjective || cgs.ccPortalEnt != -1)))
				{
					if (!(cgs.ccRequestedObjective == CG_LimboPanel_GetMaxObjectives() && cgs.ccCurrentCamObjective == -1 && cgs.ccPortalEnt == -1))
					{
						CG_LimboPanel_RequestObjective();
					}
				}
			}
		}
	}

	return qtrue;
}

/**
 * @brief CG_LimboPanel_KeyHandling
 * @param[in] key
 * @param[in] down
 */
void CG_LimboPanel_KeyHandling(int key, qboolean down)
{
	int b1, b2;

	if (BG_PanelButtonsKeyEvent(key, down, limboPanelButtons))
	{
		return;
	}

	if (down)
	{
		cgDC.getKeysForBinding("openlimbomenu", &b1, &b2);
		if ((b1 != -1 && b1 == key) || (b2 != -1 && b2 == key))
		{
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
			return;
		}
	}

	if (down && key)
	{
		if (CG_CommandCentreSpawnPointClick())
		{
			return;
		}
	}
}

// Utility funcs

/**
 * @brief CG_LimboPanel_GetSpawnPoint
 * @return
 */
int CG_LimboPanel_GetSpawnPoint(void)
{
	return cgs.ccSelectedSpawnPoint;
}

/**
 * @brief CG_LimboPanel_GetTeam
 * @return
 */
team_t CG_LimboPanel_GetTeam(void)
{
	return teamOrder[cgs.ccSelectedTeam];
}

/**
 * @brief CG_LimboPanel_GetRealTeam
 * @return
 */
team_t CG_LimboPanel_GetRealTeam(void)
{
	return cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR ? CG_LimboPanel_GetTeam() : cgs.clientinfo[cg.clientNum].team;
}

/**
 * @brief CG_LimboPanel_GetClass
 * @return
 */
int CG_LimboPanel_GetClass(void)
{
	return cgs.ccSelectedClass;
}

/**
 * @brief CG_LimboPanel_GetCharacter
 * @return
 */
bg_character_t *CG_LimboPanel_GetCharacter(void)
{
	return BG_GetCharacter(CG_LimboPanel_GetTeam(), CG_LimboPanel_GetClass());
}

/**
 * @brief CG_LimboPanel_GetPlayerClass
 * @return
 */
bg_playerclass_t *CG_LimboPanel_GetPlayerClass(void)
{
	return BG_GetPlayerClassInfo(CG_LimboPanel_GetTeam(), CG_LimboPanel_GetClass());
}

/**
 * @brief CG_LimboPanel_WeaponCount
 * @param[in] slotNumber primary or secondary slot weapon
 * @return
 */
int CG_LimboPanel_WeaponCount(int slotNumber)
{
	bg_playerclass_t *classInfo = CG_LimboPanel_GetPlayerClass();
	int              cnt = 0, i;

	if (slotNumber == PRIMARY_SLOT)
	{
		for (i = 0; i < MAX_WEAPS_PER_CLASS; i++)
		{
			// is player had the minimum level required to use this weapon
			if (cgs.clientinfo[cg.clientNum].skill[classInfo->classPrimaryWeapons[i].skill] < classInfo->classPrimaryWeapons[i].minSkillLevel)
			{
				continue;
			}

			if (!classInfo->classPrimaryWeapons[i].weapon)
			{
				break;
			}

			cnt++;
		}
	}
	else
	{
		for (i = 0; i < MAX_WEAPS_PER_CLASS; i++)
		{
			if (!classInfo->classSecondaryWeapons[i].weapon)
			{
				break;
			}

			// is player had the minimum level required to use this weapon
			if (cgs.clientinfo[cg.clientNum].skill[classInfo->classSecondaryWeapons[i].skill] < classInfo->classSecondaryWeapons[i].minSkillLevel)
			{
				continue;
			}

			// if player handling a similar weapon in primary slot, don't show it
			if (classInfo->classSecondaryWeapons[i].weapon == cgs.ccSelectedPrimaryWeapon)
			{
				continue;
			}

			cnt++;
		}
	}

	return cnt;
}

/**
 * @brief CG_LimboPanel_IsValidSelectedWeapon
 * @param[in] slot
 * @return
 */
qboolean CG_LimboPanel_IsValidSelectedWeapon(int slot)
{
	bg_playerclass_t *classInfo;
	int              i;
	weapon_t         weap;

	classInfo = CG_LimboPanel_GetPlayerClass();
	weap      = CG_LimboPanel_GetSelectedWeapon(slot);

	if (slot == PRIMARY_SLOT)
	{
		for (i = 0; i < MAX_WEAPS_PER_CLASS; i++)
		{
			if (!classInfo->classPrimaryWeapons[i].weapon)
			{
				break;
			}

			if (classInfo->classPrimaryWeapons[i].weapon == weap)
			{
				// is player had the minimum level required to use this weapon
				if (cgs.clientinfo[cg.clientNum].skill[classInfo->classPrimaryWeapons[i].skill] < classInfo->classPrimaryWeapons[i].minSkillLevel)
				{
					break;
				}

				return qtrue;
			}
		}
	}

	for (i = 0; i < MAX_WEAPS_PER_CLASS; i++)
	{
		if (!classInfo->classSecondaryWeapons[i].weapon)
		{
			break;
		}

		if (classInfo->classSecondaryWeapons[i].weapon == weap)
		{
			// is player had the minimum level required to use this weapon
			if (cgs.clientinfo[cg.clientNum].skill[classInfo->classSecondaryWeapons[i].skill] < classInfo->classSecondaryWeapons[i].minSkillLevel)
			{
				break;
			}

			// if player handling a similar weapon in primary slot, don't show it
			if (classInfo->classSecondaryWeapons[i].weapon == cgs.ccSelectedPrimaryWeapon)
			{
				break;
			}

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_SetSelectedDefaultWeapon
 * @param[in] slot
 */
void CG_LimboPanel_SetDefaultWeapon(int slot)
{
	bg_playerclass_t *classInfo;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		return;
	}

	classInfo = CG_LimboPanel_GetPlayerClass();
	if (!classInfo)
	{
		return;
	}

	if (slot == PRIMARY_SLOT)
	{
		cgs.ccSelectedPrimaryWeapon = classInfo->classPrimaryWeapons[0].weapon;
	}
	else
	{
		int i, lastValidWeaponPos = 0;

		for (i = 0; i < MAX_WEAPS_PER_CLASS; i++)
		{
			if (!classInfo->classSecondaryWeapons[i].weapon)
			{
				break;
			}

			// is player had the minimum level required to use this weapon
			if (cgs.clientinfo[cg.clientNum].skill[classInfo->classSecondaryWeapons[i].skill] < classInfo->classSecondaryWeapons[i].minSkillLevel)
			{
				continue;
			}

			// if player handling a similar weapon in primary slot, don't show it
			if (classInfo->classSecondaryWeapons[i].weapon == cgs.ccSelectedPrimaryWeapon)
			{
				continue;
			}

			lastValidWeaponPos = i;
		}

		cgs.ccSelectedSecondaryWeapon = classInfo->classSecondaryWeapons[lastValidWeaponPos].weapon;
	}
}

/**
 * @brief CG_LimboPanel_GetSelectedWeapon
 * @return
 */
weapon_t CG_LimboPanel_GetSelectedWeapon(int slot)
{
	if (slot == PRIMARY_SLOT)
	{
		if (CG_LimboPanel_RealWeaponIsDisabled(cgs.ccSelectedPrimaryWeapon))
		{
			CG_LimboPanel_SetDefaultWeapon(PRIMARY_SLOT);
		}

		return cgs.ccSelectedPrimaryWeapon;
	}

	return cgs.ccSelectedSecondaryWeapon;
}

/**
 * @brief CG_LimboPanel_RequestWeaponStats
 */
void CG_LimboPanel_RequestWeaponStats(void)
{
	extWeaponStats_t weapStat;
	weapon_t         weapon;

	weapon   = CG_LimboPanel_GetSelectedWeapon(cgs.ccSelectedWeaponSlot);
	weapStat = GetWeaponTableData(weapon)->indexWeaponStat;

	if (weapStat == WS_MAX)
	{
		// FIXME: Bleh?
		return;
	}

	trap_SendClientCommand(va("ws %i", weapStat));
}

/**
 * @brief CG_LimboPanel_RequestObjective
 */
void CG_LimboPanel_RequestObjective(void)
{
	int max;

	max = CG_LimboPanel_GetMaxObjectives();

	if (cgs.ccSelectedObjective != max && CG_LimboPanel_GetTeam() != TEAM_SPECTATOR)
	{
		trap_SendClientCommand(va("obj %i", cgs.ccSelectedObjective));
	}
	else
	{
		trap_SendClientCommand(va("obj %i", -1));
	}
	cgs.ccRequestedObjective       = cgs.ccSelectedObjective;
	cgs.ccLastObjectiveRequestTime = cg.time;
}

/**
 * @brief CG_LimboPanel_SetSelectedWeaponNum
 * @param[in] number
 */
void CG_LimboPanel_SetSelectedWeaponNum(int slot, weapon_t weapon)
{
	if (slot == PRIMARY_SLOT)
	{
		if (!CG_LimboPanel_RealWeaponIsDisabled(weapon))
		{
			cgs.ccSelectedPrimaryWeapon = weapon;
		}
	}
	else
	{
		cgs.ccSelectedSecondaryWeapon = weapon;
	}
}

/**
 * @brief CG_LimboPanel_TeamCount
 * @param[in] weap weapon_t or -1
 * @return
 */
int CG_LimboPanel_TeamCount(int weap)
{
	int i, cnt;

	if (weap == -1)     // we aint checking for a weapon, so always include ourselves
	{
		cnt = 1;
	}
	else     // we ARE checking for a weapon, so ignore ourselves
	{
		cnt = 0;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (i == cg.clientNum)
		{
			continue;
		}

		if (!cgs.clientinfo[i].infoValid)
		{
			continue;
		}

		if (cgs.clientinfo[i].team != CG_LimboPanel_GetTeam())
		{
			continue;
		}

		if (weap != -1)
		{
			if (cgs.clientinfo[i].weapon != weap && cgs.clientinfo[i].latchedweapon != weap)
			{
				continue;
			}
		}

		cnt++;
	}

	return cnt;
}

/**
 * @brief Convert a string to an integer
 * @details Convert a string to an integer, with the same behavior that the engine converts
 * cvars to their integer representation:
 *   - Integer is obtained from concatenating all the integers in the string,
 *   regardless of the other characters present in the string ("-" is the exception,
 *   of there is a "-" before the first integer, the number is turned into a negative)
 *   - If there are no integers in the string, return 0
 * @param[in] src String to convert to an integer
 * @return Result of converted string to an integer
 * @note Originates from q_shared.c
 */
int ExtractInt(const char *src)
{
	unsigned int i;
	unsigned int srclen = strlen(src) + 1;
	int          destIx = 0;
	char         *tmp   = Com_Allocate(srclen);
	int          result = 0;

	// Go through all the characters in the source string
	for (i = 0; i < srclen; i++)
	{
		// Pick out negative sign before first integer, or integers only
		if (((src[i] == '-') && (destIx == 0)) || Q_isnumeric(src[i]))
		{
			tmp[destIx++] = src[i];
		}
	}

	// put string terminator in temp var
	tmp[destIx] = 0;

	// convert temp var to integer
	if (tmp[0] != 0)
	{
		int sign = 1;

		result = sign * Q_atoi(tmp);
	}

	Com_Dealloc(tmp);

	return result;
}

/**
 * @brief CG_LimboPanel_MaxCount
 * @param[in] playerCount
 * @param[in] variableString
 * @return
 */
int CG_LimboPanel_MaxCount(int playerCount, const char *variableString)
{
	int maxCount;

	maxCount = ExtractInt(variableString);
	if (maxCount == -1)
	{
		return MAX_CLIENTS;
	}
	if (strstr(variableString, ".-"))
	{
		maxCount = floor(maxCount * playerCount * 0.01f);
	}
	else if (strstr(variableString, "."))
	{
		maxCount = ceil(maxCount * playerCount * 0.01f);
	}

	return maxCount;
}

/**
 * @brief Checks for heavy and rifle weapons
 * @param[in] weapon
 * @return
 * @note FIXME: this function needs some rework: count picked up opposite team weapons too
 *       see G_IsWeaponDisabled
 *       check: CG_LimboPanel_RealWeaponIsDisabled probably doesn't have to check for alt weapons
 *       they can't be selected
 */
qboolean CG_LimboPanel_RealWeaponIsDisabled(weapon_t weapon)
{
	int        count, wcount;
	const char *maxCount;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR)
	{
		return qtrue;
	}

	// never restrict normal weapons
	if (!(GetWeaponTableData(weapon)->skillBased == SK_HEAVY_WEAPONS || (GetWeaponTableData(GetWeaponTableData(weapon)->weapAlts)->type & WEAPON_TYPE_RIFLENADE)))
	{
		return qfalse;
	}

	count  = CG_LimboPanel_TeamCount(-1);
	wcount = CG_LimboPanel_TeamCount(weapon);

	// heavy weapon restriction
	if (GetWeaponTableData(weapon)->skillBased == SK_HEAVY_WEAPONS)
	{
		if (wcount >= ceil(count * cgs.weaponRestrictions))
		{
			return qtrue;
		}
	}

	if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_PANZER)
	{
		maxCount = cg.maxRockets;
	}
	else if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_MORTAR)
	{
		maxCount = cg.maxMortars;
	}
	else if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_MG)
	{
		maxCount = cg.maxMachineguns;
	}
	else if (GetWeaponTableData(GetWeaponTableData(weapon)->weapAlts)->type & WEAPON_TYPE_RIFLENADE)
	{
		maxCount = cg.maxRiflegrenades;
	}
	else if (weapon == WP_FLAMETHROWER)
	{
		maxCount = cg.maxFlamers;
	}
	else
	{
		return qfalse;
	}

	if (GetWeaponTableData(weapon)->weapAlts)
	{
		// add alt weapons
		wcount += CG_LimboPanel_TeamCount(GetWeaponTableData(weapon)->weapAlts);
	}

	if (wcount >= CG_LimboPanel_MaxCount(count, maxCount))
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_ClassCount
 * @param[in] checkTeam
 * @param[in] classIndex
 * @return
 */
int CG_LimboPanel_ClassCount(team_t checkTeam, int classIndex)
{
	int i, cnt = 0;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (i == cg.clientNum)
		{
			continue;
		}

		if (!cgs.clientinfo[i].infoValid)
		{
			continue;
		}

		if (cgs.clientinfo[i].team != checkTeam)
		{
			continue;
		}

		if (cgs.clientinfo[i].cls != classIndex && cgs.clientinfo[i].latchedcls != classIndex)
		{
			continue;
		}

		cnt++;
	}

	return cnt;
}

/**
 * @brief CG_LimboPanel_ClassIsDisabled
 * @param[in] selectedTeam
 * @param[in] classIndex
 * @return
 */
qboolean CG_LimboPanel_ClassIsDisabled(team_t selectedTeam, int classIndex)
{
	bg_playerclass_t *classinfo;
	team_t           playerTeam;
	int              classCount, playerCount;

	if (selectedTeam == TEAM_SPECTATOR)
	{
		return qtrue;
	}

	if (classIndex < PC_SOLDIER || classIndex > PC_COVERTOPS)
	{
		return qfalse;
	}

	playerTeam = CG_LimboPanel_GetRealTeam();
	classinfo  = CG_LimboPanel_GetPlayerClass();

	if (classinfo->classNum == classIndex && playerTeam == selectedTeam && cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
	{
		return qfalse;
	}

	classCount  = CG_LimboPanel_ClassCount(selectedTeam, classIndex);
	playerCount = CG_LimboPanel_TeamCount(-1);

	if (classCount >= CG_LimboPanel_MaxCount(playerCount, cg.maxPlayerClasses[classIndex]))
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_TeamIsFull
 * @param[in] checkTeam
 * @return
 */
qboolean CG_LimboPanel_TeamIsFull(team_t checkTeam)
{
	int i, cnt = 0;

	if (checkTeam == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (i == cg.clientNum)
		{
			continue;
		}

		if (!cgs.clientinfo[i].infoValid)
		{
			continue;
		}

		if (cgs.clientinfo[i].team != checkTeam)
		{
			continue;
		}

		cnt++;
	}

	if (cg.maxPlayers > 0 && cg.maxPlayers <= cnt && cgs.clientinfo[cg.clientNum].team != checkTeam)
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_LimboPanel_TeamIsDisabled
 * @param[in] checkTeam
 * @return
 */
qboolean CG_LimboPanel_TeamIsDisabled(team_t checkTeam)
{
	if (checkTeam == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return qtrue;
	}

	if (CG_LimboPanel_TeamIsFull(checkTeam))
	{
		return qtrue;
	}

	if (CG_LimboPanel_FindFreeClass(checkTeam) != -1)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_LimboPanel_FindFreeClass
 * @param[in] checkTeam
 * @return
 */
int CG_LimboPanel_FindFreeClass(team_t checkTeam)
{
	int i;

	for (i = PC_SOLDIER; i < NUM_PLAYER_CLASSES; i++)
	{
		if (!CG_LimboPanel_ClassIsDisabled(checkTeam, i))
		{
			return i;
		}
	}

	return -1;
}
