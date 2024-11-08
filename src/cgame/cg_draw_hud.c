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
 * @file cg_draw_hud.c
 * @brief Draws the player's hud
 *
 */

#include "cg_local.h"

hudData_t hudData;
hudComponent_t *showOnlyHudComponent = NULL;

static lagometer_t lagometer;

/**
* @var hudComponentFields
* @brief for accessing hudStucture_t's fields in a loop
*/
const hudComponentFields_t hudComponentFields[] =
{
	{ HUDF(crosshair),          CG_DrawCrosshair,                 0.19f,  { "Pulse",         "Pulse Alt",    "Dynamic Color",  "Dynamic Color Alt" } },         // FIXME: outside cg_draw_hud
	{ HUDF(compass),            CG_DrawNewCompass,                0.19f,  { "Square",        "Draw Item",    "Draw Sec Obj",   "Draw Prim Obj", "Decor", "Direction", "Cardinal Pts", "Always Draw"} },
	{ HUDF(staminabar),         CG_DrawStaminaBar,                0.19f,  { "Left",          "Center",       "Vertical",       "No Alpha", "Bar Bckgrnd", "X0 Y5", "X0 Y0", "Lerp Color", "Bar Border", "Border Tiny", "Decor", "Icon"} },
	{ HUDF(breathbar),          CG_DrawBreathBar,                 0.19f,  { "Left",          "Center",       "Vertical",       "No Alpha", "Bar Bckgrnd", "X0 Y5", "X0 Y0", "Lerp Color", "Bar Border", "Border Tiny", "Decor", "Icon"} },
	{ HUDF(healthbar),          CG_DrawPlayerHealthBar,           0.19f,  { "Left",          "Center",       "Vertical",       "No Alpha", "Bar Bckgrnd", "X0 Y5", "X0 Y0", "Lerp Color", "Bar Border", "Border Tiny", "Decor", "Icon", "Needle", "Dynamic Color"} },
	{ HUDF(weaponchargebar),    CG_DrawWeapRecharge,              0.19f,  { "Left",          "Center",       "Vertical",       "No Alpha", "Bar Bckgrnd", "X0 Y5", "X0 Y0", "Lerp Color", "Bar Border", "Border Tiny", "Decor", "Icon", "Needle"} },
	{ HUDF(healthtext),         CG_DrawPlayerHealth,              0.25f,  { "Dynamic Color", "Draw Suffix" } },
	{ HUDF(xptext),             CG_DrawXP,                        0.25f,  { "Draw Suffix" } },
	{ HUDF(ranktext),           CG_DrawRank,                      0.20f,  { 0 } },
	{ HUDF(statsdisplay),       CG_DrawSkills,                    0.25f,  { "Column" } },
	{ HUDF(weaponicon),         CG_DrawGunIcon,                   0.19f,  { "Icon Flash",    "Dynamic Heat Color" } },
	{ HUDF(weaponammo),         CG_DrawAmmoCount,                 0.25f,  { "Dynamic Color" } },
	{ HUDF(fireteam),           CG_DrawFireTeamOverlay,           0.20f,  { "Latched Class", "No Header",    "Colorless Name", "Status Color Name", "Status Color Row"} },// FIXME: outside cg_draw_hud
	{ HUDF(popupmessages),      CG_DrawPM,                        0.22f,  { "No Connect",    "No TeamJoin",  "No Mission",     "No Pickup", "No Death", "Weapon Icon", "Alt Weap Icons", "Swap V<->K", "Force Colors", "Scroll Down"} }, // FIXME: outside cg_draw_hud
	{ HUDF(popupmessages2),     CG_DrawPM,                        0.22f,  { "No Connect",    "No TeamJoin",  "No Mission",     "No Pickup", "No Death", "Weapon Icon", "Alt Weap Icons", "Swap V<->K", "Force Colors", "Scroll Down"} }, // FIXME: outside cg_draw_hud
	{ HUDF(popupmessages3),     CG_DrawPM,                        0.22f,  { "No Connect",    "No TeamJoin",  "No Mission",     "No Pickup", "No Death", "Weapon Icon", "Alt Weap Icons", "Swap V<->K", "Force Colors", "Scroll Down"} }, // FIXME: outside cg_draw_hud
	{ HUDF(powerups),           CG_DrawPowerUps,                  0.19f,  { 0 } },
	{ HUDF(objectives),         CG_DrawObjectiveStatus,           0.19f,  { 0 } },
	{ HUDF(hudhead),            CG_DrawPlayerStatusHead,          0.19f,  { 0 } },
	{ HUDF(cursorhints),        CG_DrawCursorhint,                0.19f,  { "Size Pulse",    "Strobe Pulse", "Alpha Pulse" } },// FIXME: outside cg_draw_hud
	{ HUDF(cursorhintsbar),     CG_DrawCursorHintBar,             0.19f,  { "Left",          "Center",       "Vertical",       "No Alpha", "Bar Bckgrnd", "X0 Y5", "X0 Y0", "Lerp Color", "Bar Border", "Border Tiny", "Decor", "Icon"} },  // FIXME: outside cg_draw_hud
	{ HUDF(cursorhintstext),    CG_DrawCursorHintText,            0.19f,  { "Draw Suffix" } },// FIXME: outside cg_draw_hud
	{ HUDF(weaponstability),    CG_DrawWeapStability,             0.19f,  { "Always",        "Left",         "Center",         "Vertical", "No Alpha", "Bar Bckgrnd", "X0 Y5", "X0 Y0", "Lerp Color", "Bar Border", "Border Tiny", "Decor", "Icon"} }, // FIXME: outside cg_draw_hud
	{ HUDF(livesleft),          CG_DrawLivesLeft,                 0.19f,  { 0 } },
	{ HUDF(roundtimer),         CG_DrawRoundTimer,                0.19f,  { "Simple" } },
	{ HUDF(reinforcement),      CG_DrawRespawnTimer,              0.19f,  { 0 } },
	{ HUDF(spawntimer),         CG_DrawSpawnTimer,                0.19f,  { 0 } },
	{ HUDF(localtime),          CG_DrawLocalTime,                 0.19f,  { "Second",        "12 Hours" } },
	{ HUDF(votetext),           CG_DrawVote,                      0.22f,  { "Complaint" } }, // FIXME: outside cg_draw_hud
	{ HUDF(spectatortext),      CG_DrawSpectatorMessage,          0.22f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(limbotext),          CG_DrawLimboMessage,              0.22f,  { "No Wounded Msg" } },// FIXME: outside cg_draw_hud
	{ HUDF(followtext),         CG_DrawFollow,                    0.22f,  { "No Countdown" } },// FIXME: outside cg_draw_hud
	{ HUDF(demotext),           CG_DrawDemoMessage,               0.22f,  { "Details" } },
	{ HUDF(missilecamera),      CG_DrawMissileCamera,             0.22f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(sprinttext),         CG_DrawPlayerSprint,              0.25f,  { "Draw Suffix" } },
	{ HUDF(breathtext),         CG_DrawPlayerBreath,              0.25f,  { "Draw Suffix" } },
	{ HUDF(weaponchargetext),   CG_DrawWeaponCharge,              0.25f,  { "Draw Suffix" } },
	{ HUDF(fps),                CG_DrawFPS,                       0.19f,  { 0 } },
	{ HUDF(snapshot),           CG_DrawSnapshot,                  0.19f,  { 0 } },
	{ HUDF(ping),               CG_DrawPing,                      0.19f,  { 0 } },
	{ HUDF(speed),              CG_DrawSpeed,                     0.19f,  { "Max Speed" } },
	{ HUDF(lagometer),          CG_DrawLagometer,                 0.19f,  { 0 } },
	{ HUDF(disconnect),         CG_DrawDisconnect,                0.35f,  { "No Text" } },
	{ HUDF(chat),               CG_DrawTeamInfo,                  0.20f,  { "No Team Flag" } },// FIXME: outside cg_draw_hud
	{ HUDF(spectatorstatus),    CG_DrawSpectator,                 0.35f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(pmitemsbig),         CG_DrawPMItemsBig,                0.22f,  { "No Skill",      "No Rank",      "No Prestige" } },// FIXME: outside cg_draw_hud
	{ HUDF(warmuptitle),        CG_DrawWarmupTitle,               0.35f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(warmuptext),         CG_DrawWarmupText,                0.22f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(objectivetext),      CG_DrawObjectiveInfo,             0.22f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(centerprint),        CG_DrawCenterString,              0.22f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(banner),             CG_DrawBannerPrint,               0.23f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(crosshairtext),      CG_DrawCrosshairNames,            0.25f,  { "Full Color",    "Explosive Owner" } },// FIXME: outside cg_draw_hud
	{ HUDF(crosshairbar),       CG_DrawCrosshairHealthBar,        0.25f,  { "Class",         "Rank",         "Prestige",       "Left", "Center", "Vertical", "No Alpha", "Bar Bckgrnd", "X0 Y5", "X0 Y0", "Lerp Color", "Bar Border", "Border Tiny", "Decor", "Icon", "Dynamic Color"} }, // FIXME: outside cg_draw_hud
	{ HUDF(stats),              CG_DrawShoutcastPlayerStatus,     0.19f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(xpgain),             CG_DrawPMItemsXPGain,             0.22f,  { "Scroll Down",   "No Reason",    "No Stack", "No XP Add up" } },   // FIXME: outside cg_draw_hud
	{ HUDF(scPlayerListAxis),   CG_DrawShoutcastPlayerListAxis,   0.16f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(scPlayerListAllies), CG_DrawShoutcastPlayerListAllies, 0.16f,  { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(scTeamNamesAxis),    CG_DrawShoutcastTeamNameAxis,     0.3f,   { 0 } },           // FIXME: outside cg_draw_hud
	{ HUDF(scTeamNamesAllies),  CG_DrawShoutcastTeamNameAllies,   0.3f,   { 0 } },           // FIXME: outside cg_draw_hud
	{ NULL,                     0,                                qfalse, NULL, 0.00f,{ 0 } },
};

/**
 * @brief CG_getActiveHUD Returns reference to an active hud structure.
 * @return
 */
hudStucture_t *CG_GetActiveHUD()
{
	return hudData.active;
}

static int compIndex = 0;

/**
 * @brief CG_getComponent
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] visible
 * @param[in] style
 * @return
 */
static ID_INLINE hudComponent_t CG_getComponent(float x, float y, float w, float h, qboolean visible, int style,
                                                float scale, const vec4_t colorMain, const vec4_t colorSecondary,
                                                int showBackground, const vec4_t colorBackground,
                                                int showBorder, const vec4_t colorBorder,
                                                int styleText, int alignText, int autoAdjust, float hardScale, void (*draw)(hudComponent_t *comp))
{
	hudComponent_t tmp;
	Com_Memset(&tmp, 0, sizeof(hudComponent_t));

	rect_set(tmp.internalLocation, x, y, w, h);

	tmp.visible = visible;
	tmp.style   = style;
	tmp.scale   = scale;

	vec4_copy(colorMain, tmp.colorMain);
	vec4_copy(colorSecondary, tmp.colorSecondary);

	tmp.showBackGround = showBackground;
	vec4_copy(colorBackground, tmp.colorBackground);

	tmp.showBorder = showBorder;
	vec4_copy(colorBorder, tmp.colorBorder);

	tmp.styleText  = styleText;
	tmp.alignText  = alignText;
	tmp.autoAdjust = autoAdjust;
	tmp.offset     = compIndex++;
	tmp.hardScale  = hardScale;
	tmp.parsed     = qfalse;
	tmp.draw       = draw;

	return tmp;
}

vec4_t HUD_Background    = { 0.16f, 0.2f, 0.17f, 0.5f };
vec4_t HUD_BackgroundAlt = { 0.0f, 0.0f, 0.0f, 0.3f };
vec4_t HUD_Border        = { 0.5f, 0.5f, 0.5f, 0.5f };
vec4_t HUD_Text          = { 0.6f, 0.6f, 0.6f, 1.0f };

/**
 * @brief CG_setDefaultHudValues
 * @param[out] hud
 */
void CG_setDefaultHudValues(hudStucture_t *hud)
{
	compIndex = 0;
	// the Default hud

	hud->hudnumber          = 0;
	hud->name[0]            = '\0';
	hud->crosshair          = CG_getComponent(SCREEN_WIDTH * .5f - 24, SCREEN_HEIGHT * .5 - 24, 48, 48, qtrue, CROSSHAIR_PULSE, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawCrosshair);
	hud->compass            = CG_getComponent(SCREEN_WIDTH - 136, 0, 132, 132, qtrue, COMPASS_ITEM | COMPASS_SECONDARY_OBJECTIVES | COMPASS_PRIMARY_OBJECTIVES | COMPASS_DECOR | COMPASS_CARDINAL_POINTS, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawNewCompass);
	hud->staminabar         = CG_getComponent(4, SCREEN_HEIGHT - 92, 12, 72, qtrue, BAR_LEFT | BAR_VERT | BAR_BG | BAR_BGSPACING_X0Y0 | BAR_LERP_COLOR | BAR_DECOR | BAR_ICON, 100.f, (vec4_t) { 0, 1.0f, 0.1f, 0.5f }, (vec4_t) { 1.0f, 0, 0.1f, 0.5f }, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawStaminaBar);
	hud->breathbar          = CG_getComponent(4, SCREEN_HEIGHT - 92, 12, 72, qtrue, BAR_LEFT | BAR_VERT | BAR_BG | BAR_BGSPACING_X0Y0 | BAR_LERP_COLOR | BAR_DECOR | BAR_ICON, 100.f, (vec4_t) { 0, 0.1f, 1.0f, 0.5f }, (vec4_t) { 1.0f, 0.1f, 0, 0.5f }, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawBreathBar);
	hud->healthbar          = CG_getComponent(24, SCREEN_HEIGHT - 92, 12, 72, qtrue, BAR_LEFT | BAR_VERT | BAR_BG | BAR_BGSPACING_X0Y0 | BAR_DECOR | BAR_ICON | BAR_NEEDLE | (BAR_NEEDLE << 1), 100.f, (vec4_t) { 1.f, 1.f, 1.f, 0.75f }, (vec4_t) { 1.f, 0, 0, 0.25f }, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawPlayerHealthBar);
	hud->weaponchargebar    = CG_getComponent(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 92, 12, 72, qtrue, BAR_LEFT | BAR_VERT | BAR_BG | BAR_LERP_COLOR | BAR_DECOR | BAR_ICON | BAR_NEEDLE, 100.f, (vec4_t) { 1.0, 1.0f, 1.0f, 0.75f }, (vec4_t) { 1.0, 1.0f, 0.1f, 0.25f }, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawWeapRecharge);
	hud->healthtext         = CG_getComponent(47, 465, 57, 14, qtrue, 2, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.25f, CG_DrawPlayerHealth);
	hud->xptext             = CG_getComponent(108, 465, 57, 14, qtrue, 1, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.25f, CG_DrawXP);
	hud->ranktext           = CG_getComponent(167, 465, 57, 14, qfalse, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.20f, CG_DrawRank);    // disable
	hud->statsdisplay       = CG_getComponent(116, 394, 42, 70, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.25f, CG_DrawSkills);
	hud->weaponicon         = CG_getComponent(SCREEN_WIDTH - 88, SCREEN_HEIGHT - 52, 60, 32, qtrue, 1 | 2, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawGunIcon);
	hud->weaponammo         = CG_getComponent(SCREEN_WIDTH - 82, 458, 57, 14, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_RIGHT, qfalse, 0.25f, CG_DrawAmmoCount);
	hud->fireteam           = CG_getComponent(10, 10, 350, 100, qtrue, 1, 100.f, colorWhite, HUD_Background, qtrue, HUD_BackgroundAlt, qtrue, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.20f, CG_DrawFireTeamOverlay);
	hud->popupmessages      = CG_getComponent(4, 245, 422, 96, qtrue, 64, 89.7f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawPM);
	hud->popupmessages2     = CG_getComponent(4, 245, 422, 96, qfalse, 64, 89.7f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawPM);
	hud->popupmessages3     = CG_getComponent(4, 245, 422, 96, qfalse, 64, 89.7f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawPM);
	hud->powerups           = CG_getComponent(SCREEN_WIDTH  - 40, SCREEN_HEIGHT - 136, 36, 36, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawPowerUps);
	hud->objectives         = CG_getComponent(4, SCREEN_HEIGHT - 136, 36, 36, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawObjectiveStatus);
	hud->hudhead            = CG_getComponent(44, SCREEN_HEIGHT - 96, 62, 80, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawPlayerStatusHead);
	hud->cursorhints        = CG_getComponent(SCREEN_WIDTH * .5f - 24, 260, 48, 48, qtrue, 1, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawCursorhint);
	hud->cursorhintsbar     = CG_getComponent(SCREEN_WIDTH * .5f - 24, 308, 48, 8, qtrue, BAR_BORDER_SMALL | BAR_LERP_COLOR, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawCursorHintBar);
	hud->cursorhintstext    = CG_getComponent(SCREEN_WIDTH * .5f + 24, 294, 57, 14, qfalse, 1, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.25f, CG_DrawCursorHintText);
	hud->weaponstability    = CG_getComponent(50, 208, 10, 64, qtrue, (BAR_CENTER | BAR_VERT | BAR_LERP_COLOR) << 1, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawWeapStability);
	hud->livesleft          = CG_getComponent(4, 360, 48, 24, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawLivesLeft);
	hud->roundtimer         = CG_getComponent(SCREEN_WIDTH - 60, 152, 57, 14, qtrue, 0, 100.f, colorWhite, colorWhite, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawRoundTimer);
	hud->reinforcement      = CG_getComponent(SCREEN_WIDTH - 60, SCREEN_HEIGHT - 70, 57, 14, qfalse, 0, 100.f, colorLtBlue, colorWhite, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawRespawnTimer);
	hud->spawntimer         = CG_getComponent(SCREEN_WIDTH - 60, SCREEN_HEIGHT - 60, 57, 14, qfalse, 0, 100.f, colorRed, colorWhite, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawSpawnTimer);
	hud->localtime          = CG_getComponent(SCREEN_WIDTH - 60, 168, 57, 14, qfalse, 0, 100.f, HUD_Text, HUD_Text, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawLocalTime);
	hud->votetext           = CG_getComponent(4, 202, 278, 28, qtrue, 1, 100.f, colorYellow, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawVote);
	hud->spectatortext      = CG_getComponent(4, 160, 278, 38, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawSpectatorMessage);
	hud->limbotext          = CG_getComponent(4, 124, 278, 38, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawLimboMessage);
	hud->followtext         = CG_getComponent(4, 124, 278, 24, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawFollow);
	hud->demotext           = CG_getComponent(10, 0, 57, 10, qtrue, 0, 100.f, (vec4_t){ 1, 0, 0, 0.5 }, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawDemoMessage);
	hud->missilecamera      = CG_getComponent(4, 120, 160, 120, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawMissileCamera);
	hud->sprinttext         = CG_getComponent(20, SCREEN_HEIGHT - 96, 57, 14, qfalse, 1, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.25f, CG_DrawPlayerSprint);
	hud->breathtext         = CG_getComponent(20, SCREEN_HEIGHT - 96, 57, 14, qfalse, 1, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.25f, CG_DrawPlayerBreath);
	hud->weaponchargetext   = CG_getComponent(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 96, 57, 14, qfalse, 1, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.25f, CG_DrawWeaponCharge);
	hud->fps                = CG_getComponent(SCREEN_WIDTH - 60, 184, 57, 14, qfalse, 0, 100.f, HUD_Text, HUD_Text, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawFPS);
	hud->snapshot           = CG_getComponent(SCREEN_WIDTH - 60, 305, 57, 38, qfalse, 0, 100.f, HUD_Text, HUD_Text, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER2, qfalse, 0.19f, CG_DrawSnapshot);
	hud->ping               = CG_getComponent(SCREEN_WIDTH - 60, 200, 57, 14, qfalse, 0, 100.f, HUD_Text, HUD_Text, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawPing);
	hud->speed              = CG_getComponent(SCREEN_WIDTH - 60, 275, 57, 14, qfalse, 0, 100.f, HUD_Text, HUD_Text, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawSpeed);
	hud->lagometer          = CG_getComponent(SCREEN_WIDTH - 60, 216, 57, 57, qfalse, 0, 100.f, HUD_Text, HUD_Text, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawLagometer);
	hud->disconnect         = CG_getComponent(SCREEN_WIDTH - 60, 216, 57, 57, qtrue, 0, 100.f, colorWhite, colorWhite, qtrue, HUD_Background, qtrue, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.35f, CG_DrawDisconnect);
	hud->chat               = CG_getComponent(165, 406, 364, 72, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.20f, CG_DrawTeamInfo);
	hud->spectatorstatus    = CG_getComponent(SCREEN_WIDTH * .5f - 70, 421, 140, 24, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.35f, CG_DrawSpectator);
	hud->pmitemsbig         = CG_getComponent(347, 292, 290, 57, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawPMItemsBig);
	hud->warmuptitle        = CG_getComponent(SCREEN_WIDTH * .5f - 211, 120, 422, 24, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.35f, CG_DrawWarmupTitle);
	hud->warmuptext         = CG_getComponent(SCREEN_WIDTH * .5f - 211, 310, 422, 39, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.22f, CG_DrawWarmupText);
	hud->objectivetext      = CG_getComponent(SCREEN_WIDTH * .5f - 211, 351, 422, 24, qtrue, 0, 100.f, colorWhite, colorWhite, qtrue, (vec4_t) { 0, 0.5f, 0.5f, 0.25f }, qtrue, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qtrue, 0.22f, CG_DrawObjectiveInfo);
	hud->centerprint        = CG_getComponent(SCREEN_WIDTH * .5f - 211, 378, 422, 24, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.22f, CG_DrawCenterString);
	hud->banner             = CG_getComponent(SCREEN_WIDTH * .5f - 211, 20, 422, 24, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.23f, CG_DrawBannerPrint);
	hud->crosshairtext      = CG_getComponent(SCREEN_WIDTH * .5f - 150, 182, 300, 16, qtrue, 0, 100.f, colorWhite, colorWhite, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.25f, CG_DrawCrosshairNames);
	hud->crosshairbar       = CG_getComponent(SCREEN_WIDTH * .5f - 55, 199, 110, 10, qtrue, CROSSHAIR_BAR_CLASS | CROSSHAIR_BAR_RANK | ((BAR_BG | BAR_LERP_COLOR) << 3), 100.f, (vec4_t) { 1.f, 1.f, 1.f, 0.75f }, (vec4_t) { 1.f, 0, 0, 0.25f }, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, qfalse, 0.25f, CG_DrawCrosshairHealthBar);
	hud->stats              = CG_getComponent(SCREEN_WIDTH * .5f - 112, SCREEN_HEIGHT - 75, 224, 36, qfalse, 0, 100.f, colorWhite, colorMdGrey, qtrue, (vec4_t) { 0.0f, 0.0f, 0.0f, 0.7f }, qtrue, colorLtGrey, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_CENTER, qfalse, 0.19f, CG_DrawShoutcastPlayerStatus);
	hud->xpgain             = CG_getComponent(SCREEN_WIDTH * .625f, 235, 178, 53, qtrue, 1, 100.f, colorWhite, colorLtGrey, qfalse, HUD_Background, qfalse, HUD_Border, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_LEFT, qfalse, 0.22f, CG_DrawPMItemsXPGain);
	hud->scPlayerListAxis   = CG_getComponent(15, SCREEN_HEIGHT - 180, 142, 168, qfalse, 0, 100.f, colorWhite, (vec4_t) { 0.749f, 0.129f, 0.129f, 0.45f }, qtrue, (vec4_t) { 0.0f, 0.0f, 0.0f, 0.7f }, qfalse, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_LEFT, qfalse, 0.16f, CG_DrawShoutcastPlayerListAxis);
	hud->scPlayerListAllies = CG_getComponent(SCREEN_WIDTH - 142 - 15, SCREEN_HEIGHT - 180, 142, 168, qfalse, 0, 100.f, colorWhite, (vec4_t) { 0.121f, 0.447f, 0.811f, 0.45f }, qtrue, (vec4_t) { 0.0f, 0.0f, 0.0f, 0.7f }, qfalse, (vec4_t) { 0.0f, 0.0f, 0.0f, 0.7f }, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_LEFT, qfalse, 0.16f, CG_DrawShoutcastPlayerListAllies);
	hud->scTeamNamesAxis    = CG_getComponent(SCREEN_WIDTH * .5f - 190 - 30, 12, 190, 30, qfalse, 01, 100.f, colorWhite, colorBlack, qtrue, (vec4_t) { 0.749f, 0.129f, 0.129f, 0.45f }, qfalse, (vec4_t) { 0.0f, 0.0f, 0.0f, 0.7f }, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_LEFT, qfalse, 0.3f, CG_DrawShoutcastTeamNameAxis);
	hud->scTeamNamesAllies  = CG_getComponent(SCREEN_WIDTH * .5f + 30, 12, 190, 30, qfalse, 0, 100.f, colorWhite, colorBlack, qtrue, (vec4_t) { 0.121f, 0.447f, 0.811f, 0.45f }, qfalse, HUD_Border, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_LEFT, qfalse, 0.3f, CG_DrawShoutcastTeamNameAllies);
}

/**
 * @brief CG_GetHudByIndex
 * @param[in] index
 * @return found hud
 */
hudStucture_t *CG_GetHudByNumber(int number)
{
	int i;

	for (i = 0; i < hudData.count; i++)
	{
		hudStucture_t *hud = hudData.list[i];

		if (hud->hudnumber == number)
		{
			return hud;
		}
	}

	return NULL;
}

/**
 * @brief CG_GetHudByName
 * @param[in] name
 * @return found hud
 */
hudStucture_t *CG_GetHudByName(const char *name)
{
	int i;

	for (i = 0; i < hudData.count; i++)
	{
		hudStucture_t *hud = hudData.list[i];

		if (!Q_stricmp(hud->name, name))
		{
			return hud;
		}
	}

	return NULL;
}

/**
 * @brief CG_UpdateParentHUD
 * @param[in] oldParent
 * @param[in] newParent
 * @param[in] newParentNum
 */
void CG_UpdateParentHUD(const char *oldParent, const char *newParent, int newParentNum)
{
	int i;
	// ensure to update parent child as well
	for (i = 0; i < hudData.count; i++)
	{
		hudStucture_t *hud = hudData.list[i];

		if (!Q_stricmp(hud->parent, oldParent))
		{
			// use grand-parent value
			Q_strncpyz(hud->parent, newParent, sizeof(hud->parent));
			hud->parentNumber = newParentNum;
		}
	}
}

// HUD DRAWING FUNCTIONS BELLOW

/**
 * @brief CG_DrawPicShadowed
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] icon
 */
static void CG_DrawPicShadowed(float x, float y, float w, float h, qhandle_t icon)
{
	float ofsX;
	float ofsY;

	trap_R_SetColor(colorBlack);
	ofsX = (w * 1.07f) - w;
	ofsY = (h * 1.07f) - h;
	CG_DrawPic(x + ofsX, y + ofsY, w, h, icon);
	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, w, h, icon);
}

/**
 * @brief CG_DrawCompText
 * @param[in] comp
 * @param[in] str
 * @param[in] color
 * @param[in] fontStyle
 * @param[in] font
 */
void CG_DrawCompText(hudComponent_t *comp, const char *str, vec4_t color, int fontStyle, fontHelper_t *font)
{
	float x = comp->location.x, y = comp->location.y;
	float w, w2, h, h2, scale, paddingW, paddingH;

	if (!str)
	{
		return;
	}

	scale = CG_ComputeScale(comp /*comp->location.h , comp->scale, font*/);

	w  = CG_Text_Width_Ext(str, scale, 0, font);
	h  = CG_Text_Height_Ext(str, scale, 0, font);
	w2 = MIN(comp->location.w, w);
	h2 = comp->autoAdjust ? MIN(comp->location.h, h) : MAX(comp->location.h, h);

	paddingW = Com_Clamp(0, CG_Text_Width_Ext("A", scale, 0, font) * .5f, (comp->location.w - w) * .5f);
	paddingH = Com_Clamp(0, CG_Text_Height_Ext("A", scale, 0, font) * .5f, (comp->location.h - h) * .5f);

	if (comp->autoAdjust)
	{
		h2 = MIN(comp->location.h, h + paddingH * 2.f);
		y += ((comp->location.h - h2) * .5f);
	}

	switch (comp->alignText)
	{
	case ITEM_ALIGN_RIGHT:
		x += (comp->location.w - w2);
		if (comp->autoAdjust)
		{
			x  -= paddingW * 2.f;
			w2 += paddingW * 2.f;
		}
		break;
	case ITEM_ALIGN_CENTER:
	case ITEM_ALIGN_CENTER2:
		x += ((comp->location.w - w2) * .5f);
		if (comp->autoAdjust)
		{
			x  -= paddingW;
			w2 += paddingW * 2.f;
		}
		break;
	case ITEM_ALIGN_LEFT:
		if (comp->autoAdjust)
		{
			w2 += paddingW * 2.f;
		}
		break;
	default: break;
	}

	if (comp->showBackGround)
	{
		if (comp->autoAdjust)
		{
			CG_FillRect(x, y, w2, h2, comp->colorBackground);
		}
		else
		{
			CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
		}
	}

	if (comp->showBorder)
	{
		if (comp->autoAdjust)
		{
			CG_DrawRect_FixedBorder(x, y, w2, h2, 1, comp->colorBorder);
		}
		else
		{
			CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
		}
	}

	switch (comp->alignText)
	{
	case ITEM_ALIGN_RIGHT:   x += (comp->autoAdjust ? paddingW : -paddingW); break;
	case ITEM_ALIGN_CENTER:
	case ITEM_ALIGN_CENTER2: x += (comp->autoAdjust ? paddingW : 0); break;
	case ITEM_ALIGN_LEFT:    x += paddingW; break;
	default: break;
	}

	CG_Text_Paint_Ext(x, y + ((h2 + h) * .5f), scale, scale, color, str, 0, 0, fontStyle, font);
}

/**
 * @brief CG_DrawCompMultilineText
 * @param[in] comp
 * @param[in] str
 * @param[in] lineLength
 * @param[in] lineNumber
 * @param[in] color
 * @param[in] fontStyle
 * @param[in] font
 */
void CG_DrawCompMultilineText(hudComponent_t *comp, const char *str, vec4_t color, int align, int fontStyle, fontHelper_t *font)
{
	unsigned int lineNumber = 0;
	float        x = comp->location.x, y = comp->location.y;
	float        w = 0, w2, h = 0, h2, scale, paddingW, paddingH;
	char         *ptr;
	char         temp[1024] = { 0 };

	if (!str)
	{
		return;
	}

	Q_strncpyz(temp, str, 1024);

	// count line number and max char length
	ptr = strtok(temp, "\n");
	do
	{
		lineNumber++;
		w   = MAX(CG_Text_Width_Ext_Float(ptr, 1.f, 0, font), w);
		h  += CG_Text_Height_Ext(ptr, 1.f, 0, font);
		ptr = strtok(NULL, "\n");
	}
	while (ptr != NULL);

	scale = CG_ComputeScale(comp /*comp->location.h / lineNumber, comp->scale, font*/);

	// compute max width and height
	w *= scale;
	h *= scale;
	w2 = MIN(comp->location.w, w);
	h2 = comp->autoAdjust ? MIN(comp->location.h, h) : MAX(comp->location.h, h);

	paddingW = Com_Clamp(0, CG_Text_Width_Ext_Float("A", scale, 0, font) * .5f, (comp->location.w - w) * .5f);
	paddingH = Com_Clamp(0, CG_Text_Height_Ext("A", scale, 0, font) * .75f, (comp->location.h - h) * .75f);

	if (comp->autoAdjust)
	{
		h2 = MIN(h2 + paddingH * (lineNumber + 1), comp->location.h);
		y += ((comp->location.h - h2) * .5f);
	}

	switch (align)
	{
	case ITEM_ALIGN_RIGHT:
		x += (comp->location.w - w2);
		if (comp->autoAdjust)
		{
			x  -= paddingW * 2.f;
			w2 += paddingW * 2.f;
		}
		break;
	case ITEM_ALIGN_CENTER:
		x += ((comp->location.w - w2) * .5f);
		if (comp->autoAdjust)
		{
			x  -= paddingW;
			w2 += paddingW * 2.f;
		}
		break;
	case ITEM_ALIGN_CENTER2:
		x    += ((comp->location.w - w2) * .5f);
		align = ITEM_ALIGN_LEFT;
		if (comp->autoAdjust)
		{
			x  -= paddingW;
			w2 += paddingW * 2.f;
		}
		break;
	case ITEM_ALIGN_LEFT:
		if (comp->autoAdjust)
		{
			w2 += paddingW * 2.f;
		}
		break;
	default: break;
	}

	if (comp->showBackGround)
	{
		if (comp->autoAdjust)
		{
			CG_FillRect(x, y, w2, h2, comp->colorBackground);
		}
		else
		{
			CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
		}
	}

	if (comp->showBorder)
	{
		if (comp->autoAdjust)
		{
			CG_DrawRect_FixedBorder(x, y, w2, h2, 1, comp->colorBorder);
		}
		else
		{
			CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
		}
	}

	CG_DrawMultilineText(comp->location.x + paddingW, y + ((h2 + h) * .5f) / lineNumber, comp->location.w - (paddingW * 2), scale, scale, color, str,
	                     h2 / lineNumber, 0, 0, fontStyle, align, font);
}

/**
 * @brief CG_DrawPlayerStatusHead
 * @param[in] comp
 */
void CG_DrawPlayerStatusHead(hudComponent_t *comp)
{
	hudHeadAnimNumber_t anim           = cg.idleAnim;
	bg_character_t      *character     = CG_CharacterForPlayerstate(&cg.snap->ps);
	bg_character_t      *headcharacter = BG_GetCharacter(cgs.clientinfo[cg.snap->ps.clientNum].team, cgs.clientinfo[cg.snap->ps.clientNum].cls);
	qhandle_t           painshader     = 0;
	rectDef_t           *headRect      = &comp->location;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
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
			cg.idleAnim = (hudHeadAnimNumber_t)((rand() % (HD_DAMAGED_IDLE3 - HD_DAMAGED_IDLE2 + 1)) + HD_DAMAGED_IDLE2);
		}
		else
		{
			cg.idleAnim = (hudHeadAnimNumber_t)((rand() % (HD_IDLE8 - HD_IDLE2 + 1)) + HD_IDLE2);
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

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	CG_DrawPlayerHead(headRect, character, headcharacter, 180, 0, (cg.snap->ps.eFlags & EF_HEADSHOT) ? qfalse : qtrue, anim, painshader, cgs.clientinfo[cg.snap->ps.clientNum].rank, qfalse, cgs.clientinfo[cg.snap->ps.clientNum].team);
}

/**

 */

/**
 * @brief Get the current ammo and/or clip count of the holded weapon (if using ammo).
 * @param[out] ammo - the number of ammo left (in the current clip if using clip)
 * @param[out] clips - the total ammount of ammo in all clips (if using clip)
 * @param[out] akimboammo - the number of ammo left in the second pistol of akimbo (if using akimbo)
 * @param[out] colorAmmo
 */
void CG_PlayerAmmoValue(int *ammo, int *clips, int *akimboammo, vec4_t **colorAmmo /*, vec4_t **colorClip*/)
{
	centity_t     *cent;
	playerState_t *ps;
	weapon_t      weap;

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

	weap = (weapon_t)cent->currentState.weapon;

	if (!IS_VALID_WEAPON(weap))
	{
		return;
	}

	// some weapons don't draw ammo count
	if (!GetWeaponTableData(weap)->useAmmo)
	{
		return;
	}

	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		return;
	}

	// total ammo in clips, grenade launcher is not a clip weapon but show the clip anyway
	if (GetWeaponTableData(weap)->useClip || (weap == WP_M7 || weap == WP_GPG40))
	{
		// current reserve
		*clips = cg.snap->ps.ammo[GetWeaponTableData(weap)->ammoIndex];

		// current clip
		*ammo = ps->ammoclip[GetWeaponTableData(weap)->clipIndex];

		// akimbo ammo clip
		if (GetWeaponTableData(weap)->attributes & WEAPON_ATTRIBUT_AKIMBO)
		{
			*akimboammo = ps->ammoclip[GetWeaponTableData(GetWeaponTableData(weap)->akimboSideArm)->clipIndex];
		}

		if (colorAmmo)
		{
			float maxClip   = GetWeaponTableData(weap)->maxClip * (*akimboammo != -1 ? 2 : 1);
			float totalAmmo = *ammo + (*akimboammo != -1 ? *akimboammo : 0);
			float ammoLeft  = maxClip ? totalAmmo * 100 / maxClip : 0;
			float alpha     = (**colorAmmo)[3];

			if (ammoLeft <= 30.f)
			{
				*colorAmmo = &colorRed;
			}
			else if (ammoLeft <= 40.f)
			{
				*colorAmmo = &colorOrange;
			}
			else if (ammoLeft <= 50.f)
			{
				*colorAmmo = &colorYellow;
			}

			(**colorAmmo)[3] = alpha;
		}

		/*
		if (colorClip)
		{
			float maxAmmo  = BG_MaxAmmoForWeapon(weap, cgs.clientinfo[cent->currentState.clientNum].skill, cgs.clientinfo[cent->currentState.clientNum].cls);
			float ammoLeft = maxAmmo ? GetWeaponTableData(weap)->maxClip * (*akimboammo != -1 ? 2 : 1) * 100 / maxAmmo : 0;
			float alpha    = (**colorClip)[3];

			if (ammoLeft <= 30.f)
			{
				*colorClip = &colorRed;
			}
			else if (ammoLeft <= 40.f)
			{
				*colorClip = &colorOrange;
			}
			else if (ammoLeft <= 50.f)
			{
				*colorClip = &colorYellow;
			}

			(**colorClip)[3] = alpha;
		}
		*/
	}
	else
	{
		float maxAmmo = 0;

		if (weap == WP_LANDMINE)
		{
			if (!cgs.gameManager)
			{
				*ammo = 0;
			}
			else
			{
				maxAmmo = ExtractInt(cg.maxLandmines);

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
		else
		{
			// some weapons don't draw ammo clip count text
			*ammo = ps->ammoclip[GetWeaponTableData(weap)->clipIndex] + cg.snap->ps.ammo[GetWeaponTableData(weap)->ammoIndex];

			maxAmmo = BG_MaxAmmoForWeapon(weap, cgs.clientinfo[cent->currentState.clientNum].skill, cgs.clientinfo[cent->currentState.clientNum].cls);
		}

		if (colorAmmo)
		{
			float totalAmmo = *ammo + (*akimboammo != -1 ? *akimboammo : 0);
			float ammoLeft  = maxAmmo ? totalAmmo * 100 / maxAmmo : 0;
			float alpha     = (**colorAmmo)[3];

			if (ammoLeft <= 30.f)
			{
				*colorAmmo = &colorRed;
			}
			else if (ammoLeft <= 40.f)
			{
				*colorAmmo = &colorOrange;
			}
			else if (ammoLeft <= 50.f)
			{
				*colorAmmo = &colorYellow;
			}

			(**colorAmmo)[3] = alpha;
		}
	}
}

/**
 * @brief Check if we are underwater
 * @details This check has changed to make it work for spectators following another player.
 * That's why ps.stats[STAT_AIRLEFT] has been added..
 *
 * While following high-pingers, You sometimes see the breathbar, even while they are not submerged..
 * So we check for underwater status differently when we are following others.
 * (It doesn't matter to do a more complex check for spectators.. they are not playing)
 * @return
 */
static qboolean CG_CheckPlayerUnderwater()
{
	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		vec3_t origin;

		VectorCopy(cg.snap->ps.origin, origin);
		origin[2] += 36;
		return (qboolean)(CG_PointContents(origin, cg.snap->ps.clientNum) & CONTENTS_WATER);
	}

	return cg.snap->ps.stats[STAT_AIRLEFT] < HOLDBREATHTIME;
}

vec4_t bgcolor = { 1.f, 1.f, 1.f, .3f };    // bars backgound

/**
 * @brief CG_DrawPlayerHealthBar
 * @param[in] rect
 */
void CG_DrawPlayerHealthBar(hudComponent_t *comp)
{
	vec4_t color;
	int    style = comp->style;
	float  frac  = 0.f;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	// show extra bonus HP from medics in team
	if (comp->style & (BAR_NEEDLE))
	{
		// compute max health with medics team bonus only
		float maxHealth = cg.snap->ps.stats[STAT_MAX_HEALTH];

		// remove medic class health bonus
		if (cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_MEDIC)
		{
			maxHealth /= 1.12f;
		}

		// remove battle sense skill health bonus
		if (BG_IsSkillAvailable(cgs.clientinfo[cg.clientNum].skill, SK_BATTLE_SENSE, SK_BATTLE_SENSE_HEALTH))
		{
			maxHealth -= 15;
		}

		// no team medics health bonus, the needle is not displayed
		if (maxHealth - DEFAULT_HEALTH > 0)
		{
			frac = (cg.snap->ps.stats[STAT_MAX_HEALTH] - (maxHealth - DEFAULT_HEALTH)) / cg.snap->ps.stats[STAT_MAX_HEALTH];
		}
	}

	if (comp->style & (BAR_NEEDLE << 1))
	{
		Vector4Copy(comp->colorMain, color);
		CG_ColorForHealth(cg.snap->ps.stats[STAT_HEALTH], color);
		color[3] = comp->colorMain[3];

		style &= ~BAR_LERP_COLOR;
	}
	else
	{
		Vector4Copy(comp->colorMain, color);
	}

	CG_FilledBar(comp->location.x, comp->location.y, comp->location.w, comp->location.h,
	             (style & BAR_LERP_COLOR) ? comp->colorSecondary : color, (style & BAR_LERP_COLOR) ? color : NULL,
	             comp->colorBackground, comp->colorBorder, cg.snap->ps.stats[STAT_HEALTH] / (float) cg.snap->ps.stats[STAT_MAX_HEALTH], frac,
	             style, cgs.media.hudHealthIcon);

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_DrawStaminaBar
 * @param[in] rect
 */
void CG_DrawStaminaBar(hudComponent_t *comp)
{
	vec4_t color;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (CG_CheckPlayerUnderwater())
	{
		return;
	}

	Vector4Copy(comp->colorMain, color);

	if (cg.snap->ps.powerups[PW_ADRENALINE])
	{
		if (cg.snap->ps.pm_flags & PMF_FOLLOW)
		{
			Vector4Average(color, colorWhite, (float)sin(cg.time * .005), color);
		}
		else
		{
			float msec = cg.snap->ps.powerups[PW_ADRENALINE] - cg.time;

			if (msec >= 0)
			{
				Vector4Average(color, colorMdRed, (float)(.5 + sin(.2 * sqrt((double)msec) * M_TAU_F) * .5), color);
			}
		}
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	CG_FilledBar(comp->location.x, comp->location.y, comp->location.w, comp->location.h,
	             (comp->style & BAR_LERP_COLOR) ? comp->colorSecondary : color, (comp->style & BAR_LERP_COLOR) ? color : NULL,
	             comp->colorBackground, comp->colorBorder, cg.snap->ps.stats[STAT_SPRINTTIME] / SPRINTTIME, 0.f, comp->style, cgs.media.hudSprintIcon);

	trap_R_SetColor(NULL);
}

/**
 * @brief Draw the breath bar
 * @param[in] rect
 */
void CG_DrawBreathBar(hudComponent_t *comp)
{
	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (!CG_CheckPlayerUnderwater())
	{
		return;
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	CG_FilledBar(comp->location.x, comp->location.y, comp->location.w, comp->location.h,
	             (comp->style & BAR_LERP_COLOR) ? comp->colorSecondary : comp->colorMain, (comp->style & BAR_LERP_COLOR) ? comp->colorMain : NULL,
	             comp->colorBackground, comp->colorBorder, cg.snap->ps.stats[STAT_AIRLEFT] / HOLDBREATHTIME, 0.f, comp->style, cgs.media.waterHintShader);

	trap_R_SetColor(NULL);
}

/**
 * @param[in]  weapon
 * @param[in]  chargeTime
 * @param[out] needleFrac
 * @param[out] charge
 */
static void CG_CalcPowerState(const int weapon, const float chargeTime, float *needleFrac, qboolean *charge)
{

	int index = BG_IsSkillAvailable(cgs.clientinfo[cg.clientNum].skill,
	                                GetWeaponTableData(weapon)->skillBased,
	                                GetWeaponTableData(weapon)->chargeTimeSkill);

	float coeff = GetWeaponTableData(weapon)->chargeTimeCoeff[index];
	*needleFrac = coeff;

	if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * coeff)
	{
		*charge = qfalse;
	}
}

/**
 * @brief Draw weapon recharge bar
 * @param rect
 */
void CG_DrawWeapRecharge(hudComponent_t *comp)
{
	float    barFrac, needleFrac = 0.f, chargeTime;
	qboolean charge = qtrue;
	vec4_t   color;
	int      style = comp->style;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

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

	// Calculate power state
	if (GetWeaponTableData(cg.predictedPlayerState.weapon)->attributes & WEAPON_ATTRIBUT_CHARGE_TIME)
	{
		CG_CalcPowerState(cg.predictedPlayerState.weapon, chargeTime, &needleFrac, &charge);
	}
	else if ((cg.predictedPlayerState.eFlags & EF_ZOOMING || cg.predictedPlayerState.weapon == WP_BINOCULARS)
	         && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS)
	{
		CG_CalcPowerState(WP_ARTY, chargeTime, &needleFrac, &charge);
	}
	else
	{
		switch (cgs.clientinfo[cg.snap->ps.clientNum].cls)
		{
		case PC_SOLDIER:
		{
			if (COM_BitCheck(cg.snap->ps.weapons, WP_PANZERFAUST))
			{
				CG_CalcPowerState(WP_PANZERFAUST, chargeTime, &needleFrac, &charge);
			}
			else if (COM_BitCheck(cg.snap->ps.weapons, WP_BAZOOKA))
			{
				CG_CalcPowerState(WP_BAZOOKA, chargeTime, &needleFrac, &charge);
			}
			else if (COM_BitCheck(cg.snap->ps.weapons, WP_MORTAR2))
			{
				CG_CalcPowerState(WP_MORTAR_SET, chargeTime, &needleFrac, &charge);
			}
			else if (COM_BitCheck(cg.snap->ps.weapons, WP_MORTAR))
			{
				CG_CalcPowerState(WP_MORTAR2_SET, chargeTime, &needleFrac, &charge);
			}
		}
		break;
		case PC_MEDIC:
			CG_CalcPowerState(WP_MEDKIT, chargeTime, &needleFrac, &charge);
			break;
		case PC_FIELDOPS:
			CG_CalcPowerState(WP_ARTY, chargeTime, &needleFrac, &charge);
			break;
		case PC_COVERTOPS:
			CG_CalcPowerState(WP_SATCHEL, chargeTime, &needleFrac, &charge);
			break;
		case PC_ENGINEER:
			switch (cg.predictedPlayerState.weapon)
			{
			case WP_CARBINE:
			case WP_KAR98:
				CG_CalcPowerState(WP_GPG40, chargeTime, &needleFrac, &charge);
				break;
			case WP_COLT:
			case WP_LUGER:
			case WP_AKIMBO_COLT:
			case WP_AKIMBO_LUGER:
				if (COM_BitCheck(cg.snap->ps.weapons, WP_GPG40) || COM_BitCheck(cg.snap->ps.weapons, WP_M7))
				{
					CG_CalcPowerState(WP_GPG40, chargeTime, &needleFrac, &charge);
				}
				break;
			}
		}
	}

	// display colored charge bar if charge bar isn't full enough
	barFrac = (cg.time - cg.snap->ps.classWeaponTime) / chargeTime; // FIXME: potential DIV 0 when charge times are set to 0!

	if (barFrac > 1.0f)
	{
		barFrac = 1.0f;
	}

	if (!charge)
	{
		color[0] = 1.0f;
		color[1] = color[2] = 0.1f;
		color[3] = 0.5f;

		style &= ~BAR_LERP_COLOR;
	}
	else
	{
		Vector4Copy(comp->colorMain, color);
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	CG_FilledBar(comp->location.x, comp->location.y, comp->location.w, comp->location.h,
	             (style & BAR_LERP_COLOR) ? comp->colorSecondary : color, (style & BAR_LERP_COLOR) ? color : NULL,
	             comp->colorBackground, comp->colorBorder, barFrac, needleFrac, style, cgs.media.hudPowerIcon);

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_DrawGunIcon
 * @param[in] location
 */
void CG_DrawGunIcon(hudComponent_t *comp)
{
	vec4_t color;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	// Draw weapon icon and overheat bar
	CG_DrawWeapHeat(&comp->location, HUD_HORIZONTAL, (comp->style & 2));

	// weapon flash color
	if (comp->style & 1)
	{
		int ws =
#ifdef FEATURE_MULTIVIEW
			(cg.mvTotalClients > 0) ? cgs.clientinfo[cg.snap->ps.clientNum].weaponState :
#endif
			BG_simpleWeaponState(cg.snap->ps.weaponstate);

		if (ws == WSTATE_SWITCH || ws == WSTATE_RELOAD)
		{
			VectorCopy(colorYellow, color);
		}
		else if (ws == WSTATE_FIRE)
		{
			VectorCopy(colorRed, color);
		}
		else
		{
			VectorCopy(comp->colorMain, color);
		}

		// keep custom alpha value
		color[3] = comp->colorMain[3];
	}
	else
	{
		Vector4Copy(comp->colorMain, color);
	}

	CG_DrawPlayerWeaponIcon(&comp->location, comp->alignText, &color);
}

/**
 * @brief CG_DrawAmmoCount
 * @param[in] x
 * @param[in] y
 */
void CG_DrawAmmoCount(hudComponent_t *comp)
{
	int    value, value2, value3;
	char   buffer[16] = { 0 };
	vec4_t *color     = &comp->colorMain;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// Draw ammo
	CG_PlayerAmmoValue(&value, &value2, &value3, (comp->style & 1) ? &color : NULL);

	// .25f
	if (value3 >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i:%i/%i", value3, value, value2);
	}
	else if (value2 >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i/%i", value, value2);
	}
	else if (value >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i", value);
	}

	CG_DrawCompText(comp, buffer, *color, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawSkillBar
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] skillLvl
 */
static void CG_DrawSkillBar(float x, float y, float w, float h, int skillLvl, skillType_t skill)
{
	int    i;
	float  blockheight = (h - 4) / (float)(NUM_SKILL_LEVELS - 1);
	float  draw_y      = y + h - blockheight;
	vec4_t colour;
	float  x1, y1, w1, h1;

	for (i = 1; i < NUM_SKILL_LEVELS; i++)
	{

		if (GetSkillTableData(skill)->skillLevels[i] < 0)
		{
			Vector4Set(colour, 1.f, 0.f, 0.f, .15f);
		}
		else if (skillLvl >= i)
		{
			Vector4Set(colour, 0.f, 0.f, 0.f, .4f);
		}
		else
		{
			Vector4Set(colour, 1.f, 1.f, 1.f, .15f);
		}

		CG_FillRect(x, draw_y, w, blockheight, colour);

		// draw the star only if the skill is reach and available
		if (skillLvl >= i && GetSkillTableData(skill)->skillLevels[i] >= 0)
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

/**
 * @brief CG_ClassSkillForPosition
 * @param[in] ci
 * @param[in] pos
 * @return
 */
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
		if ((BG_PlayerMounted(cg.snap->ps.eFlags) || GetWeaponTableData(cg.snap->ps.weapon)->skillBased == SK_HEAVY_WEAPONS) && ci->cls != PC_SOLDIER)
		{
			return SK_HEAVY_WEAPONS;
		}
		return SK_LIGHT_WEAPONS;
	default:
		break;
	}

	return SK_BATTLE_SENSE;
}

/**
 * @brief CG_DrawPlayerHealth
 * @param[in] x
 * @param[in] y
 */
void CG_DrawPlayerHealth(hudComponent_t *comp)
{
	const char *str;
	vec4_t     color;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// dynamic color
	if (comp->style & 1)
	{
		Vector4Copy(comp->colorMain, color);
		CG_ColorForHealth(cg.snap->ps.stats[STAT_HEALTH], color);
		color[3] = comp->colorMain[3];
	}
	else
	{
		Vector4Copy(comp->colorMain, color);
	}

	str = va("%i%s", cg.snap->ps.stats[STAT_HEALTH], comp->style & 2 ? " HP" : "");

	CG_DrawCompText(comp, str, color, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawPlayerSprint
 * @param[in] x
 * @param[in] y
 */
void CG_DrawPlayerSprint(hudComponent_t *comp)
{
	const char *str;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (CG_CheckPlayerUnderwater())
	{
		return;
	}

	if (cg.snap->ps.powerups[PW_ADRENALINE])
	{
		str = va("%d%s", (cg.snap->ps.powerups[PW_ADRENALINE] - cg.time) / 1000, (comp->style & 1) ? " s" : "");
	}
	else
	{
		str = va("%.0f%s", (cg.snap->ps.stats[STAT_SPRINTTIME] / SPRINTTIME) * 100, (comp->style & 1) ? " %" : "");
	}

	CG_DrawCompText(comp, str, comp->colorMain, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawPlayerBreath
 * @param[in] x
 * @param[in] y
 */
void CG_DrawPlayerBreath(hudComponent_t *comp)
{
	const char *str;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (!CG_CheckPlayerUnderwater())
	{
		return;
	}

	str = va("%.0f%s", (cg.snap->ps.stats[STAT_AIRLEFT] / HOLDBREATHTIME) * 100, (comp->style & 1) ? " %" : "");

	CG_DrawCompText(comp, str, comp->colorMain, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawWeaponCharge
 * @param[in] x
 * @param[in] y
 */
void CG_DrawWeaponCharge(hudComponent_t *comp)
{
	const char *str;
	float      chargeTime;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

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

	str = va("%.0f%s", MIN(((cg.time - cg.snap->ps.classWeaponTime) / chargeTime) * 100, 100), (comp->style & 1) ? " %" : "");

	CG_DrawCompText(comp, str, comp->colorMain, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawSkills
 * @param[in] comp
 */
void CG_DrawSkills(hudComponent_t *comp)
{
	playerState_t *ps = &cg.snap->ps;
	clientInfo_t  *ci = &cgs.clientinfo[ps->clientNum];
	int           i;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	for (i = 0; i < 3; i++)
	{
		skillType_t skill = CG_ClassSkillForPosition(ci, i);
		if (!comp->style)
		{
			int w = (comp->location.w - 3) / 3;

			CG_DrawSkillBar(comp->location.x + i + i * w, comp->location.y, w, comp->location.h - w, ci->skill[skill], skill);
			CG_DrawPic(comp->location.x + i + i * w, comp->location.y + comp->location.h - w, w, w, cgs.media.skillPics[skill]);
		}
		else
		{
			int   j        = 1;
			int   skillLvl = 0;
			float tempY;
			float scale;

			// the display is divided into 3 "boxes", each containing an icon + text
			// icon takes up 60% of the box height (results in roughly square icon at default size)
			float iconH = (comp->location.h / 3) * 0.6f;
			float textH = (comp->location.h / 3) * 0.4f;

			for (; j < NUM_SKILL_LEVELS; ++j)
			{
				if (BG_IsSkillAvailable(ci->skill, skill, j))
				{
					skillLvl++;
				}
			}

			tempY = comp->location.y + (i * (iconH + textH));
			scale = CG_ComputeScale(comp /*comp->location.h / 6.f, comp->scale, &cgs.media.limboFont2*/);

			//CG_DrawPic
			CG_DrawPicShadowed(comp->location.x, tempY, comp->location.w, iconH, cgs.media.skillPics[skill]);

			// text is drawn from bottom left, so skip to the very bottom of the current "box"
			tempY += iconH + textH;

			CG_Text_Paint_Centred_Ext(comp->location.x + (comp->location.w * 0.5f), tempY, scale, scale, comp->colorMain, va("%i", skillLvl), 0, 0, comp->styleText, &cgs.media.limboFont1);
		}
	}
}

/**
 * @brief CG_DrawXP
 * @param[in] x
 * @param[in] y
 */
void CG_DrawXP(hudComponent_t *comp)
{
	const char *str;
	vec_t      *clr;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (cg.time - cg.xpChangeTime < 1000)
	{
		clr = colorYellow;
	}
	else
	{
		clr = comp->colorMain;
	}

	str = va("%s%s", Com_ScaleNumberPerThousand((float) cg.snap->ps.stats[STAT_XP], 2, 4), (comp->style & 1) ? " XP" : "");

	CG_DrawCompText(comp, str, clr, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawRank
 * @param[in] x
 * @param[in] y
 */
void CG_DrawRank(hudComponent_t *comp)
{
	const char    *str;
	playerState_t *ps = &cg.snap->ps;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	str = va("%s", GetRankTableData(cgs.clientinfo[ps->clientNum].team, cgs.clientinfo[ps->clientNum].rank)->miniNames);

	CG_DrawCompText(comp, str, comp->colorMain, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawPowerUps
 * @param[in] rect
 */
void CG_DrawPowerUps(hudComponent_t *comp)
{
	playerState_t *ps = &cg.snap->ps;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	// draw treasure icon if we have the flag
	if (ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG] || cg.generatingNoiseHud)
	{
		trap_R_SetColor(NULL);
		CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveShader);
	}
	else if (ps->powerups[PW_OPS_DISGUISED])       // Disguised?
	{
		CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		// show the class to the client
		CG_DrawPic(comp->location.x + 9, comp->location.y + 9, 18, 18, cgs.media.skillPics[BG_ClassSkillForClass((cg_entities[ps->clientNum].currentState.powerups >> PW_OPS_CLASS_1) & 7)]);
	}
	else if (ps->powerups[PW_ADRENALINE] > 0)       // adrenaline
	{
		vec4_t color = { 1.0, 0.0, 0.0, 1.0 };
		color[3] *= 0.5 + 0.5 * sin(cg.time / 150.0);
		trap_R_SetColor(color);
		CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.hudAdrenaline);
		trap_R_SetColor(NULL);
	}
	else if (ps->powerups[PW_INVULNERABLE] && !(ps->pm_flags & PMF_LIMBO))       // spawn shield
	{
		CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.spawnInvincibleShader);
	}
}

/**
 * @brief CG_DrawObjectiveStatus
 * @param[in] rect
 */
void CG_DrawObjectiveStatus(hudComponent_t *comp)
{
	playerState_t *ps                  = &cg.snap->ps;
	float         flagIconWidth        = comp->location.w * 0.333f;
	float         flagIconHeight       = comp->location.h * 0.222f;
	float         flagIconHeightOffset = comp->location.h * 0.777f;
	float         scale;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	scale = CG_ComputeScale(comp);

	// draw objective status icon
	if ((cg.flagIndicator & (1 << PW_REDFLAG) || cg.flagIndicator & (1 << PW_BLUEFLAG) || cg.flagIndicator & (1 << PW_NUM_POWERUPS)) && (!cgs.clientinfo[cg.clientNum].shoutcaster || (cg.snap->ps.pm_flags & PMF_FOLLOW)))
	{
		// draw objective info icon (if teammates or enemies are carrying one)
		vec4_t color = { 1.f, 1.f, 1.f, 1.f };
		color[3] *= 0.67 + 0.33 * sin(cg.time / 200.0);
		trap_R_SetColor(color);

		if ((cg.flagIndicator & (1 << PW_REDFLAG) && cg.flagIndicator & (1 << PW_BLUEFLAG)) || cg.flagIndicator & (1 << PW_NUM_POWERUPS))
		{
			if (cg.redFlagCounter > 0 && cg.blueFlagCounter > 0)
			{
				// both own and enemy flags stolen
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveBothTEShader);
			}
			else if (cg.redFlagCounter > 0 && !cg.blueFlagCounter)
			{
				// own flag stolen and enemy flag dropped
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.objectiveBothTDShader : cgs.media.objectiveBothDEShader);
			}
			else if (!cg.redFlagCounter && cg.blueFlagCounter > 0)
			{
				// own flag dropped and enemy flag stolen
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.objectiveBothTDShader : cgs.media.objectiveBothDEShader);
			}
			else
			{
				// both own and enemy flags dropped
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
			trap_R_SetColor(NULL);

			// display team flag
			color[3] = 1.f;
			trap_R_SetColor(color);
			CG_DrawPic(comp->location.x, comp->location.y + flagIconHeightOffset, flagIconWidth, flagIconHeight, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.axisFlag : cgs.media.alliedFlag);
			CG_DrawPic(comp->location.x + comp->location.w - flagIconWidth, comp->location.y + flagIconHeightOffset, flagIconWidth, flagIconHeight, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.alliedFlag : cgs.media.axisFlag);

			// clear debug flag
			cg.flagIndicator &= ~(1 << PW_NUM_POWERUPS);
		}
		else if (cg.flagIndicator & (1 << PW_REDFLAG))
		{
			if (cg.redFlagCounter > 0)
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.objectiveTeamShader : cgs.media.objectiveEnemyShader);
			}
			else
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
			trap_R_SetColor(NULL);

			// display team flag
			color[3] = 1.f;
			trap_R_SetColor(color);
			CG_DrawPic(comp->location.x + (ps->persistant[PERS_TEAM] == TEAM_AXIS ? comp->location.w - flagIconWidth : 0), comp->location.y + flagIconHeightOffset, flagIconWidth, flagIconHeight, cgs.media.alliedFlag);
		}
		else if (cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.blueFlagCounter > 0)
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.objectiveTeamShader : cgs.media.objectiveEnemyShader);
			}
			else
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
			trap_R_SetColor(NULL);

			// display team flag
			color[3] = 1.f;
			trap_R_SetColor(color);
			CG_DrawPic(comp->location.x + (ps->persistant[PERS_TEAM] == TEAM_ALLIES ? comp->location.w - flagIconWidth : 0), comp->location.y + flagIconHeightOffset, flagIconWidth, flagIconHeight, cgs.media.axisFlag);
		}

		// display active flag counter
		if (cg.redFlagCounter > 1)
		{
			CG_Text_Paint_Ext(comp->location.x + (ps->persistant[PERS_TEAM] == TEAM_ALLIES ? flagIconWidth * 0.5f : comp->location.w - flagIconWidth * 0.5f), comp->location.y + comp->location.h, scale, scale, comp->colorMain, va("%i", cg.redFlagCounter), 0, 0, comp->styleText, &cgs.media.limboFont1);
		}
		if (cg.blueFlagCounter > 1)
		{
			CG_Text_Paint_Ext(comp->location.x + (ps->persistant[PERS_TEAM] == TEAM_AXIS ? flagIconWidth * 0.5f : comp->location.w - flagIconWidth * 0.5f), comp->location.y + comp->location.h, scale, scale, comp->colorMain, va("%i", cg.blueFlagCounter), 0, 0, comp->styleText, &cgs.media.limboFont1);
		}

		trap_R_SetColor(NULL);
	}
	else if (cgs.clientinfo[cg.clientNum].shoutcaster && !(cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		// simplified version for shoutcaster when not following players
		vec4_t color = { 1.f, 1.f, 1.f, 1.f };
		color[3] *= 0.67 + 0.33 * sin(cg.time / 200.0);
		trap_R_SetColor(color);

		if (cg.flagIndicator & (1 << PW_REDFLAG) && cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.redFlagCounter > 0 && cg.blueFlagCounter > 0)
			{
				// both team stole an enemy flags
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveBothTEShader);
			}
			else if ((cg.redFlagCounter > 0 && !cg.blueFlagCounter) || (!cg.redFlagCounter && cg.blueFlagCounter > 0))
			{
				// one flag stolen and the other flag dropped
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveTeamShader);
			}
			else
			{
				// both team dropped flags
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
		}
		else if (cg.flagIndicator & (1 << PW_REDFLAG))
		{
			if (cg.redFlagCounter > 0)
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveTeamShader);
			}
			else
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
		}
		else if (cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.blueFlagCounter > 0)
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveTeamShader);
			}
			else
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
		}
		trap_R_SetColor(NULL);

		// display team flag
		color[3] = 1.f;
		trap_R_SetColor(color);

		if (cg.flagIndicator & (1 << PW_REDFLAG))
		{
			CG_DrawPic(comp->location.x, comp->location.y + flagIconHeightOffset, flagIconWidth, flagIconHeight, cgs.media.alliedFlag);
		}

		if (cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			CG_DrawPic(comp->location.x + comp->location.w - flagIconWidth, comp->location.y + flagIconHeightOffset, flagIconWidth, flagIconHeight, cgs.media.axisFlag);
		}

		// display active flag counter
		if (cg.redFlagCounter > 1)
		{
			CG_Text_Paint_Ext(comp->location.x + flagIconWidth * 0.5f, comp->location.y + comp->location.h, scale, scale, comp->colorMain, va("%i", cg.redFlagCounter), 0, 0, comp->styleText, &cgs.media.limboFont1);
		}
		if (cg.blueFlagCounter > 1)
		{
			CG_Text_Paint_Ext(comp->location.x + comp->location.w - flagIconWidth * 0.5f, comp->location.y + comp->location.h, scale, scale, comp->colorMain, va("%i", cg.blueFlagCounter), 0, 0, comp->styleText, &cgs.media.limboFont1);
		}

		trap_R_SetColor(NULL);
	}
}

static int lastDemoScoreTime = 0;

/**
 * @brief CG_DrawDemoMessage
 */
void CG_DrawDemoMessage(hudComponent_t *comp)
{
	char status[1024];
	char demostatus[128];
	char wavestatus[128];

	if (!cl_demorecording.integer && !cl_waverecording.integer && !cg.demoPlayback && !cg.generatingNoiseHud)
	{
		return;
	}

	// poll for score
	if ((!lastDemoScoreTime || cg.time > lastDemoScoreTime) && !cg.demoPlayback)
	{
		trap_SendClientCommand("score");
		lastDemoScoreTime = cg.time + 5000; // 5 secs
	}

	if (comp->style & 1)
	{
		if (cl_demorecording.integer)
		{
			Com_sprintf(demostatus, sizeof(demostatus), __(" demo %s: %ik "), cl_demofilename.string, cl_demooffset.integer / 1024);
		}
		else
		{
			Q_strncpyz(demostatus, "", sizeof(demostatus));
		}

		if (cl_waverecording.integer)
		{
			Com_sprintf(wavestatus, sizeof(demostatus), __(" audio %s: %ik "), cl_wavefilename.string, cl_waveoffset.integer / 1024);
		}
		else
		{
			Q_strncpyz(wavestatus, "", sizeof(wavestatus));
		}
	}
	else
	{
		Q_strncpyz(demostatus, "", sizeof(demostatus));
		Q_strncpyz(wavestatus, "", sizeof(wavestatus));
	}

	Com_sprintf(status, sizeof(status), "%s%s%s", cg.demoPlayback ? __("REPLAY") : __("RECORD"), demostatus, wavestatus);

	CG_DrawCompText(comp, status, comp->colorMain, comp->styleText, &cgs.media.limboFont2);
}

/**
 * @brief CG_DrawField
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] value
 * @param[in] charWidth
 * @param[in] charHeight
 * @param[in] dodrawpic
 * @param[in] leftAlign
 * @return
 */
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
	l = (int)strlen(num);
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

/**
 * @brief CG_DrawLivesLeft
 * @param[in] comp
 */
void CG_DrawLivesLeft(hudComponent_t *comp)
{
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] < 0)
	{
		return;
	}

	CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.hudAlliedHelmet : cgs.media.hudAxisHelmet);

	CG_DrawField(comp->location.w - 4, comp->location.y, 3, cg.snap->ps.persistant[PERS_RESPAWNS_LEFT], 14, 20, qtrue, qtrue);
}

static char statsDebugStrings[6][512];
static int  statsDebugTime[6];
static int  statsDebugTextWidth[6];
static int  statsDebugPos;

/**
 * @brief CG_InitStatsDebug
 */
void CG_InitStatsDebug(void)
{
	Com_Memset(&statsDebugStrings, 0, sizeof(statsDebugStrings));
	Com_Memset(&statsDebugTime, 0, sizeof(statsDebugTime));
	statsDebugPos = -1;
}

/**
 * @brief CG_StatsDebugAddText
 * @param[in] text
 */
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

/**
 * @brief CG_GetCompassIcon
 * @param[in] ent
 * @param[in] drawAllVoicesChat get all icons voices chat, otherwise only request relevant icons voices chat (need medic/ammo ...)
 * @param[in] drawFireTeam draw fireteam members position
 * @param[in] drawPrimaryObj draw primary objective position
 * @param[in] drawSecondaryObj draw secondary objective position
 * @param[in] drawItemObj draw item objective position
 * @param[in] drawDynamic draw dynamic elements position (player revive, command map marker)
 * @return A valid compass icon handle otherwise 0
 */
qhandle_t CG_GetCompassIcon(entityState_t *ent, qboolean drawAllVoicesChat, qboolean drawFireTeam, qboolean drawPrimaryObj, qboolean drawSecondaryObj, qboolean drawItemObj, qboolean drawDynamic, char *name)
{
	centity_t *cent = &cg_entities[ent->number];

	if (!cent->currentValid)
	{
		return 0;
	}

	switch (ent->eType)
	{
	case ET_PLAYER:
	{
		qboolean sameTeam = cg.predictedPlayerState.persistant[PERS_TEAM] == cgs.clientinfo[ent->clientNum].team;

		if (!cgs.clientinfo[ent->clientNum].infoValid)
		{
			return 0;
		}

		if (sameTeam && cgs.clientinfo[ent->clientNum].powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
		{
			return cgs.media.objectiveShader;
		}

		if (ent->eFlags & EF_DEAD)
		{
			if (drawDynamic &&
			    ((cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_MEDIC &&
			      cg.predictedPlayerState.stats[STAT_HEALTH] > 0 && ent->number == ent->clientNum && sameTeam) ||
			     (!(cg.snap->ps.pm_flags & PMF_FOLLOW) && cgs.clientinfo[cg.clientNum].shoutcaster)))
			{
				return cgs.media.medicReviveShader;
			}

			return 0;
		}

		if (sameTeam && cent->voiceChatSpriteTime > cg.time &&
		    (drawAllVoicesChat ||
		     (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_MEDIC && cent->voiceChatSprite == cgs.media.medicIcon) ||
		     (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS && cent->voiceChatSprite == cgs.media.ammoIcon)))
		{
			// FIXME: not the best place to reset it
			if (cgs.clientinfo[ent->clientNum].health <= 0)
			{
				// reset
				cent->voiceChatSpriteTime = cg.time;
				return 0;
			}

			return cent->voiceChatSprite;
		}

		if (drawFireTeam && (CG_IsOnSameFireteam(cg.clientNum, ent->clientNum) || cgs.clientinfo[cg.clientNum].shoutcaster))
		{
			// draw overlapping no-shoot icon if disguised and in same team
			if (ent->powerups & (1 << PW_OPS_DISGUISED) && cg.predictedPlayerState.persistant[PERS_TEAM] == cgs.clientinfo[ent->clientNum].team)
			{
				return cgs.clientinfo[ent->clientNum].selected ? cgs.media.friendShader : 0;
			}
			return cgs.clientinfo[ent->clientNum].selected ? cgs.media.buddyShader : 0;
		}
		break;
	}
	case ET_ITEM:
	{
		gitem_t *item;

		item = BG_GetItem(ent->modelindex);

		if (drawItemObj && !cg.flagIndicator && item && item->giType == IT_TEAM)
		{
			if ((item->giPowerUp == PW_BLUEFLAG && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
			    || (item->giPowerUp == PW_REDFLAG && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES))
			{
				return cgs.media.objectiveBlueShader;
			}

			return cgs.media.objectiveRedShader;
		}
		break;
	}
	case ET_EXPLOSIVE_INDICATOR:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(
				CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// draw explosives if an engineer
			if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_ENGINEER ||
			    (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS && ent->effect1Time == 1))
			{
				if (ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return 0;
				}

				if (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES)
				{
					return 0;
				}

				return cgs.media.destroyShader;
			}
		}
		break;
	}
	case ET_CONSTRUCTIBLE_INDICATOR:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// draw construction if an engineer
			if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_ENGINEER)
			{
				if (ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_AXIS)
				{
					return 0;
				}

				if (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_ALLIES)
				{
					return 0;
				}

				return cgs.media.constructShader;
			}
		}
		break;
	}
	case ET_TANK_INDICATOR:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// FIXME: show only when relevant
			if ((ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
			    || (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES))
			{
				return cgs.media.escortShader;
			}

			return cgs.media.destroyShader;
		}
		break;
	}
	case ET_TANK_INDICATOR_DEAD:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// FIXME: show only when relevant
			// draw repair if an engineer
			if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_ENGINEER && (
					(ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
					|| (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES)))
			{
				return cgs.media.constructShader;
			}
		}
		break;
	}
	case ET_TRAP:
	{
		if (drawSecondaryObj)
		{
			if (ent->frame == 0)
			{
				return cgs.media.regroupShader;
			}

			if (ent->frame == 4)
			{
				return cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.regroupShader : cgs.media.defendShader;
			}

			if (ent->frame == 3)
			{
				return cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.regroupShader : cgs.media.defendShader;
			}
		}
		break;
	}
	// FIXME: ET_COMMANDMAP_MARKER, ET_HEALER, ET_SUPPLIER
	//case ET_MG42_BARREL:
	//{
	//    return cgs.media.mg42HintShader;
	//}
	//case ET_CABINET_H:
	//{
	//	return cgs.media.healthHintShader;
	//}
	//caseET_CABINET_A:
	//{
	//	return cgs.media.ammoHintShader;
	//}
	default:
		break;
	}

	return 0;
}

/**
 * @brief CG_CompasMoveLocationCalc
 * @param[out] locationvalue
 * @param[in] directionplus
 * @param[in] animationout
 */
static void CG_CompasMoveLocationCalc(float *locationvalue, qboolean directionplus, qboolean animationout)
{
	float frac = cg_commandMapTime.value / 250.f;

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
			*locationvalue += (((cg.time - cgs.autoMapExpandTime - 150.f * frac) / 100.f) * 128.f) - 128.f * frac;
		}
		else
		{
			*locationvalue -= (((cg.time - cgs.autoMapExpandTime - 150.f * frac) / 100.f) * 128.f) - 128.f * frac;
		}
	}
}

/**
 * @brief CG_CompasMoveLocation
 * @param[in] basex
 * @param[in] basey
 * @param[in] basew
 * @param[in] animationout
 */
static void CG_CompasMoveLocation(float *basex, float *basey, float basew, qboolean animationout)
{
	float x    = *basex;
	float y    = *basey;
	float cent = basew / 2;
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

/**
 * @brief CG_DrawNewCompass
 * @param location
 */
void CG_DrawNewCompass(hudComponent_t *comp)
{
	float      basex           = comp->location.x;
	float      basey           = comp->location.y;
	float      basew           = comp->location.w;
	float      baseh           = comp->location.h;
	float      expandedMapFrac = cg_commandMapTime.value / 250.f;
	snapshot_t *snap;

	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport)
	{
		snap = cg.nextSnap;
	}
	else
	{
		snap = cg.snap;
	}

	if (((snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || snap->ps.stats[STAT_HEALTH] <= 0) && !cgs.clientinfo[cg.clientNum].shoutcaster)
#ifdef FEATURE_MULTIVIEW
	    || cg.mvTotalClients > 0
#endif
	    )
	{
		CG_DrawExpandedAutoMap();
		return;
	}

	if (cgs.autoMapExpanded)
	{
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR
		    && cg.time - cgs.autoMapExpandTime < 100.f * expandedMapFrac)
		{
			if (!(comp->style & COMPASS_ALWAYS_DRAW))
			{
				CG_CompasMoveLocation(&basex, &basey, basew, qtrue);
			}
		}
		else
		{
			CG_DrawExpandedAutoMap();

			if (!(comp->style & COMPASS_ALWAYS_DRAW))
			{
				return;
			}
		}
	}
	else
	{
		if (cg.time - cgs.autoMapExpandTime <= 150.f * expandedMapFrac
		    || (cg.time - cgs.autoMapExpandTime <= 250.f * expandedMapFrac
		        && cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR))
		{
			CG_DrawExpandedAutoMap();

			if (!(comp->style & COMPASS_ALWAYS_DRAW))
			{
				return;
			}
		}
		else if ((cg.time - cgs.autoMapExpandTime > 150.f * expandedMapFrac) && (cg.time - cgs.autoMapExpandTime < cg_commandMapTime.value))
		{
			if (!(comp->style & COMPASS_ALWAYS_DRAW))
			{
				CG_CompasMoveLocation(&basex, &basey, basew, qfalse);
			}
		}
	}

	if (comp->showBackGround)
	{
		CG_FillRect(basex, basey, basew, baseh, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(basex, basey, basew, baseh, 1, comp->colorBorder);
	}

	CG_DrawAutoMap(basex, basey, basew, baseh, comp->style);
}
/**
 * @brief CG_DrawStatsDebug
 */
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
#define UPPERRIGHT_W 52

/**
 * @brief CG_DrawSnapshot
 * @param[in] comp
 * @return
 */
void CG_DrawSnapshot(hudComponent_t *comp)
{
	CG_DrawCompMultilineText(comp, va("t:%i\nsn:%i\ncmd:%i", cg.snap->serverTime, cg.latestSnapshotNum, cgs.serverCommandSequence),
	                         comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawSpeed
 * @param[in] comp
 * @return
 */
void CG_DrawSpeed(hudComponent_t *comp)
{
	static vec_t highestSpeed, speed;
	static int   lasttime;
	char         *s, *s2 = NULL;
	int          thistime;

	if (resetmaxspeed)
	{
		highestSpeed  = 0;
		resetmaxspeed = qfalse;
	}

	thistime = trap_Milliseconds();

	if (thistime > lasttime + 100)
	{
		speed = VectorLength(cg.predictedPlayerState.velocity);

		if (speed > highestSpeed)
		{
			highestSpeed = speed;
		}

		lasttime = thistime;
	}

	switch (cg_drawUnit.integer)
	{
	case 0:
		// Units per second
		s  = va("%.1f UPS", speed);
		s2 = va("%.1f MAX", highestSpeed);
		break;
	case 1:
		// Kilometers per hour
		s  = va("%.1f KPH", (speed / SPEED_US_TO_KPH));
		s2 = va("%.1f MAX", (highestSpeed / SPEED_US_TO_KPH));
		break;
	case 2:
		// Miles per hour
		s  = va("%.1f MPH", (speed / SPEED_US_TO_MPH));
		s2 = va("%.1f MAX", (highestSpeed / SPEED_US_TO_MPH));
		break;
	default:
		s  = "";
		s2 = "";
		break;
	}

	if (comp->style & 1)
	{
		CG_DrawCompMultilineText(comp, va("%s\n%s", s, s2), comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont1);
	}
	else
	{
		CG_DrawCompText(comp, s, comp->colorMain, comp->styleText, &cgs.media.limboFont1);
	}
}

#define MAX_FPS_FRAMES  500

/**
 * @brief CG_DrawFPS
 * @param[in] comp
 * @return
 */
void CG_DrawFPS(hudComponent_t *comp)
{
	static int previousTimes[MAX_FPS_FRAMES];
	static int previous;
	static int index;
	static int oldSamples;
	const char *s;
	int        t;
	int        frameTime;
	int        samples = cg_drawFPS.integer;

	t = trap_Milliseconds(); // don't use serverTime, because that will be drifting to correct for internet lag changes, timescales, timedemos, etc

	frameTime = t - previous;
	previous  = t;

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

	CG_DrawCompText(comp, s, comp->colorMain, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_SpawnTimerText red colored spawn time text in reinforcement time HUD element.
 * @return red colored text or NULL when its not supposed to be rendered
*/
char *CG_SpawnTimerText()
{
	int msec = (cgs.timelimit * 60000.f) - (cg.time - cgs.levelStartTime);
	int seconds;
	int secondsThen;

	if (cg_spawnTimer_set.integer != -1 && cgs.gamestate == GS_PLAYING && !cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			int period = cg_spawnTimer_period.integer > 0 ? cg_spawnTimer_period.integer : (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS ? cg_bluelimbotime.integer / 1000 : cg_redlimbotime.integer / 1000);
			if (period > 0) // prevent division by 0 for weird cases like limbtotime < 1000
			{
				seconds     = msec / 1000;
				secondsThen = ((cgs.timelimit * 60000.f) - cg_spawnTimer_set.integer) / 1000;
				return va("%i", period + (seconds - secondsThen) % period);
			}
		}
	}
	else if (cg_spawnTimer_set.integer != -1 && cg_spawnTimer_period.integer > 0 && cgs.gamestate != GS_PLAYING)
	{
		// We are not playing and the timer is set so reset/disable it
		// this happens for example when custom period is set by timerSet and map is restarted or changed
		trap_Cvar_Set("cg_spawnTimer_set", "-1");
	}
	return NULL;
}

/**
 * @brief CG_SpawnTimersText
 * @param[out] respawn
 * @param[out] spawntimer
 * @return
 */
static qboolean CG_SpawnTimersText(char **s, char **rt)
{
	if (cgs.gamestate != GS_PLAYING)
	{
		int limbotimeOwn, limbotimeEnemy;
		if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
		{
			limbotimeOwn   = cg_redlimbotime.integer;
			limbotimeEnemy = cg_bluelimbotime.integer;
		}
		else
		{
			limbotimeOwn   = cg_bluelimbotime.integer;
			limbotimeEnemy = cg_redlimbotime.integer;
		}

		*rt = va("%2.0i", limbotimeEnemy / 1000);
		*s  = (cgs.gametype == GT_WOLF_LMS && !cgs.clientinfo[cg.clientNum].shoutcaster) ? va("%s", CG_TranslateString("WARMUP")) : va("%2.0i", limbotimeOwn / 1000);

		// if hud editor is up, return qfalse since we want to see text style changes
		return !cg.generatingNoiseHud;
	}
	else if (cgs.gametype != GT_WOLF_LMS)
	{
		if (cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			*s  = va("%2.0i", CG_CalculateReinfTime(TEAM_ALLIES));
			*rt = va("%2.0i", CG_CalculateReinfTime(TEAM_AXIS));
		}
		else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			*s  = va("%2.0i", CG_GetReinfTime(qfalse));
			*rt = CG_SpawnTimerText();
		}
	}
	return qfalse;
}

/**
 * @brief CG_RoundTimerText
 * @return
 */
static char *CG_RoundTimerText()
{
	qtime_t qt;
	char    *seconds;
	char    *minutes;

	if (cgs.gamestate != GS_PLAYING)
	{
		return "WARMUP";
	}

	if (CG_RoundTime(&qt) < 0 && cgs.timelimit > 0.0f)
	{
		return "0:00"; // round ended
	}

	seconds = qt.tm_sec > 9 ? va("%i", qt.tm_sec) : va("0%i", qt.tm_sec);
	minutes = qt.tm_min > 9 ? va("%i", qt.tm_min) : va("0%i", qt.tm_min);

	return va("%s:%s", minutes, seconds);
}

/**
 * @brief CG_LocalTimeText
 * @param[in] style
 * @return
 */
static char *CG_LocalTimeText(int style)
{
	qtime_t  time;
	char     *s;
	qboolean pmtime = qfalse;

	//Fetch the local time
	trap_RealTime(&time);

	if (style & LOCALTIME_SECOND)
	{
		if (style & LOCALTIME_12HOUR)
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
		if (style & LOCALTIME_12HOUR)
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
	return s;
}

/**
 * @brief CG_DrawRespawnTimer
 * @param[in] comp
 */
void CG_DrawRespawnTimer(hudComponent_t *comp)
{
	char     *s = NULL, *rt = NULL;
	qboolean blink;

	if (cg_paused.integer)
	{
		return;
	}

	blink = CG_SpawnTimersText(&s, &rt);

	if (s)
	{
		CG_DrawCompText(comp, s, comp->colorMain, blink ? ITEM_TEXTSTYLE_BLINK : comp->styleText, &cgs.media.limboFont1);
	}
}

/**
 * @brief CG_DrawSpawnTimer
 * @param[in] comp
 */
void CG_DrawSpawnTimer(hudComponent_t *comp)
{
	char     *s = NULL, *rt = NULL;
	qboolean blink;

	if (cg_paused.integer)
	{
		return;
	}

	// note: pass reinforcement timer in as 's' to get the ENEMY reinforcement time
	// FIXME: this should be refactored, this makes no sense... what even is 's'? and 'rt'?
	//  spawntimer/reinforcement timer? but the function doesn't treat them as such...
	blink = CG_SpawnTimersText(&s, &rt);

	if (s)
	{
		CG_DrawCompText(comp, rt, comp->colorMain, blink ? ITEM_TEXTSTYLE_BLINK : comp->styleText, &cgs.media.limboFont1);
	}
}

/**
 * @brief CG_DrawRoundTimer
 * @param[in] comp
 */
void CG_DrawRoundTimer(hudComponent_t *comp)
{
	char     *s = NULL, *rt = NULL, *mt;
	qboolean blink;

	if (cg_paused.integer)
	{
		return;
	}

	blink = CG_SpawnTimersText(&s, &rt);

	mt = va("%s%s", "^*", CG_RoundTimerText());

	if (comp->style & 1)
	{
		s = mt;
	}
	else
	{
		if (s)
		{
			s = va("^$%s%s%s", s, " ", mt);
		}
		else
		{
			s = mt;
		}

		if (rt)
		{
			s = va("^1%s%s%s", rt, " ", s);
		}
	}

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		// round number
		if (cgs.gametype == GT_WOLF_STOPWATCH)
		{
			s = va("%s\n%i/2", s, cgs.currentRound + 1);
		}
		else
		{
			// extra empty line so we keep a nice looking round time
			s = va("%s\n ", s);
		}

		CG_DrawCompMultilineText(comp, s, comp->colorMain, blink ? ITEM_TEXTSTYLE_BLINK : comp->alignText, comp->styleText, &cgs.media.limboFont1);
	}
	else
	{
		CG_DrawCompText(comp, s, comp->colorMain, blink ? ITEM_TEXTSTYLE_BLINK : comp->styleText, &cgs.media.limboFont1);
	}
}

/**
 * @brief CG_DrawLocalTime
 * @param[in] comp
 */
void CG_DrawLocalTime(hudComponent_t *comp)
{
	char *s;

	s = CG_LocalTimeText(comp->style);

	CG_DrawCompText(comp, s, comp->colorMain, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief Adds the current interpolate / extrapolate bar for this frame
 */
void CG_AddLagometerFrameInfo(void)
{
	lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = cg.time - cg.latestSnapshotTime;
	lagometer.frameCount++;
}

/**
 * @brief Log the ping time, server framerate and number of dropped snapshots
 * before it each time a snapshot is received.
 * @param[in] snap
 */
void CG_AddLagometerSnapshotInfo(snapshot_t *snap)
{
	unsigned int index = lagometer.snapshotCount & (LAG_SAMPLES - 1);
	int          oldest;

	// dropped packet
	if (!snap)
	{
		lagometer.snapshotSamples[index] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	if (cg.demoPlayback)
	{
		static int lasttime = 0;

		snap->ping = (snap->serverTime - snap->ps.commandTime) - (1000 / cgs.sv_fps);

		// display snapshot time delta instead of ping
		lagometer.snapshotSamples[index] = snap->serverTime - lasttime;
		lasttime                         = snap->serverTime;
	}
	else
	{
		lagometer.snapshotSamples[index] = MAX(snap->ping - snap->ps.stats[STAT_ANTIWARP_DELAY], 0);
	}
	lagometer.snapshotAntiwarp[index] = snap->ping;  // TODO: check this for demoPlayback
	lagometer.snapshotFlags[index]    = snap->snapFlags;
	lagometer.snapshotCount++;

	// compute server framerate
	index = cgs.sampledStat.count;

	if (cgs.sampledStat.count < LAG_SAMPLES)
	{
		cgs.sampledStat.count++;
	}
	else
	{
		index -= 1;
	}

	cgs.sampledStat.samples[index].elapsed = snap->serverTime - cgs.sampledStat.lastSampleTime;
	cgs.sampledStat.samples[index].time    = snap->serverTime;

	if (cgs.sampledStat.samples[index].elapsed < 0)
	{
		cgs.sampledStat.samples[index].elapsed = 0;
	}

	cgs.sampledStat.lastSampleTime = snap->serverTime;

	cgs.sampledStat.samplesTotalElpased += cgs.sampledStat.samples[index].elapsed;

	oldest = snap->serverTime - PERIOD_SAMPLES;
	for (index = 0; index < cgs.sampledStat.count; index++)
	{
		if (cgs.sampledStat.samples[index].time > oldest)
		{
			break;
		}

		cgs.sampledStat.samplesTotalElpased -= cgs.sampledStat.samples[index].elapsed;
	}

	if (index)
	{
		memmove(cgs.sampledStat.samples, cgs.sampledStat.samples + index, sizeof(sample_t) * (cgs.sampledStat.count - index));
		cgs.sampledStat.count -= index;
	}

	cgs.sampledStat.avg = cgs.sampledStat.samplesTotalElpased > 0
	                      ? (int) (cgs.sampledStat.count / (cgs.sampledStat.samplesTotalElpased / 1000.0f) + 0.5f)
	                      : 0;
}

/**
 * @brief Draw disconnect icon for long lag
 * @param[in] y
 * @return
 */
void CG_DrawDisconnect(hudComponent_t *comp)
{
	int        cmdNum;
	float      w, w2;
	usercmd_t  cmd;
	const char *s;
	float      scale;

	// dont draw if a demo and we're running at a different timescale
	if (cg.demoPlayback && cg_timescale.value != 1.0f)
	{
		return;
	}

	// don't draw if the server is respawning
	if (cg.serverRespawning)
	{
		return;
	}

	// don't draw if intermission is about to start
	if (cg.intermissionStarted)
	{
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - cg.cmdBackup + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime
	    || cmd.serverTime > cg.time)        // special check for map_restart
	{
		return;
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	scale = CG_ComputeScale(comp /*comp->location.h / 5.f, comp->scale, &cgs.media.limboFont2*/);

	// also add text in center of screen
	if (!(comp->style & BIT(0)))
	{
		s = CG_TranslateString("Connection Interrupted");
		w = CG_Text_Width_Ext(s, scale, 0, &cgs.media.limboFont2);
		CG_Text_Paint_Ext(Ccg_WideX(320) - w / 2, 100, scale, scale, comp->colorMain, s, 0, 0, comp->styleText, &cgs.media.limboFont2);
	}

	// blink the icon
	if ((cg.time >> 9) & 1)
	{
		return;
	}

	// use same dimension as timer
	w  = CG_Text_Width_Ext("xx:xx:xx", 0.19f, 0, &cgs.media.limboFont1);
	w2 = (comp->location.w > w) ? comp->location.w : w;

	CG_DrawPic(comp->location.x, comp->location.y, w2 + 3, w2 + 3, cgs.media.disconnectIcon);
}

/**
 * @brief CG_DrawPing
 * @param[in] y
 * @return
 */
void CG_DrawPing(hudComponent_t *comp)
{
	char *s;

	s = va("Ping %d", cg.snap->ping < 999 ? cg.snap->ping : 999);

	CG_DrawCompText(comp, s, comp->colorMain, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawLagometer
 * @param[in] y
 * @return
 */
void CG_DrawLagometer(hudComponent_t *comp)
{
	int   a, i;
	float v, w, w2;
	float ax, ay, aw, ah, mid, range;
	int   color;
	float vscale;
	float scale;

	scale = CG_ComputeScale(comp /*comp->location.h / 5.f, comp->scale, &cgs.media.limboFont2*/);

	// use same dimension as timer
	w  = CG_Text_Width_Ext("xx:xx:xx", scale, 0, &cgs.media.limboFont1);
	w2 = (comp->location.w > w) ? comp->location.w : w;

	// draw the graph
	trap_R_SetColor(NULL);
	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, w2, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, w2, comp->location.h, 1, comp->colorBorder);
	}

	ax = comp->location.x;
	ay = comp->location.y;
	aw = w2;
	ah = w2;
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
			// antiwarp indicator
			if (lagometer.snapshotAntiwarp[i] > 0)
			{
				w = lagometer.snapshotAntiwarp[i] * vscale;

				if (color != 6)
				{
					color = 6;
					trap_R_SetColor((vec4_t) { 0, 0.5, 0, 0.5f });
				}

				if (w > range)
				{
					w = range;
				}
				trap_R_DrawStretchPic(ax + aw - a, ay + ah - w, 1, w, 0, 0, 0, 0, cgs.media.whiteShader);
			}

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
		CG_Text_Paint_Ext(ax, ay, scale * 1.75, scale * 1.75, colorWhite, "snc", 0, 0, comp->styleText, &cgs.media.limboFont2);
	}

	// don't draw if a demo and we're running at a different timescale
	if (!cg.demoPlayback)
	{
		CG_DrawDisconnect(&hudData.active->disconnect);
	}

	// add snapshots/s in top-right corner of meter
	{
		char   *result;
		vec4_t *clr;

		if (cgs.sampledStat.avg < cgs.sv_fps * 0.5f)
		{
			clr = &colorRed;
		}
		else if (cgs.sampledStat.avg < cgs.sv_fps * 0.75f)
		{
			clr = &colorYellow;
		}
		else
		{
			clr = &comp->colorMain;
		}

		// FIXME: see warmup blinky blinky
		//if (cgs.gamestate != GS_PLAYING)
		//{
		//	color[3] = fabs(sin(cg.time * 0.002));
		//}

		// FIXME: we might do different views x/Y or in %
		//result = va("%i/%i", cgs.sampledStat.avg, cgs.sv_fps);
		result = va("%i", cgs.sampledStat.avg);

		w  = CG_Text_Width_Ext(result, scale, 0, &cgs.media.limboFont1);
		w2 = (comp->location.w > w) ? comp->location.w : w;

		CG_Text_Paint_Ext(comp->location.x + ((w2 - w) / 2), comp->location.y + comp->location.h / 5.f, scale, scale, *clr, result, 0, 0, comp->styleText, &cgs.media.limboFont1);
	}
}

/**
 * @brief CG_Hud_Setup
 */
void CG_Hud_Setup(void)
{
	unsigned int  i;
	hudStucture_t *hud0 = NULL, *tmp;

	Com_Memset(&hudData, 0, sizeof(hudData_t));

	hud0 = CG_GetFreeHud();

	// Hud0 aka the Default hud
	CG_setDefaultHudValues(hud0);

	Q_strncpyz(hud0->name, DEFAULTHUD, sizeof(hud0->name));

	// generate the default hud anchors
	CG_GenerateHudAnchors(hud0);

	CG_RegisterHud(hud0);

	hudData.active = hud0;

	// Read the hud files
	CG_ReadHudsFromFile();

	// set the user hud
	CG_SetHud();

	// compute all huds positions
	for (i = 0; i < hudData.count; i++)
	{
		tmp = hudData.list[i];
		if (tmp)
		{
			CG_ComputeComponentPositions(tmp);
		}
	}
}

#ifdef ETLEGACY_DEBUG

/**
 * @brief CG_PrintHudComponent
 * @param[in] name
 * @param[in] comp
 */
static void CG_PrintHudComponent(const char *name, hudComponent_t *comp)
{
	Com_Printf("%s location: X %.f Y %.f W %.f H %.f "
	           "style %i visible: "
	           "%i scale : %f "
	           "color %.f %.f %.f %.f\n",
	           name, comp->location.x, comp->location.y, comp->location.w, comp->location.h,
	           comp->style, comp->visible,
	           comp->scale,
	           comp->colorMain[0], comp->colorMain[1], comp->colorMain[2], comp->colorMain[3]);
}

/**
 * @brief CG_PrintHud
 * @param[in] hud
 */
static void CG_PrintHud(hudStucture_t *hud)
{
	int i;

	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (!hudComponentFields[i].isAlias)
		{
			CG_PrintHudComponent(hudComponentFields[i].name, (hudComponent_t *)((char * )hud + hudComponentFields[i].offset));
		}
	}
}
#endif

static ID_INLINE void CG_HUD_LoadByName(const char *name)
{
	char tmp[MAX_QPATH] = { 0 };
	int  len;

	if (!name || !*name)
	{
		return;
	}

	len = strlen(name);

	if (MAX_QPATH <= len)
	{
		return;
	}

	Q_strncpyz(tmp, name, MAX_QPATH);

	if (len <= 4 || strcmp(tmp + len - 4, ".dat") != 0)
	{
		Q_strcat(tmp, MAX_QPATH, ".dat");
	}

	hudData.active = CG_ReadSingleHudJsonFile(tmp);

	if (hudData.active)
	{
		CG_ComputeComponentPositions(hudData.active);
	}
}

/**
 * @brief CG_SetHud
 */
void CG_SetHud(void)
{
	static int modCount  = -1;
	static int shoutcast = -1;

	vmCvar_t hudCvar = cgs.clientinfo[cg.clientNum].shoutcaster ? cg_shoutcasterHud : cg_altHud;

	if (hudCvar.modificationCount == modCount
	    && hudData.active
	    && hudData.active->active
	    && cgs.clientinfo[cg.clientNum].shoutcaster == shoutcast)
	{
		return;
	}

	hudData.active = CG_GetHudByName(hudCvar.string);

	if (!hudData.active)
	{
		if (!hudData.active)
		{
			CG_HUD_LoadByName(hudCvar.string);
		}

		if (!hudData.active)
		{
			if (Q_isanumber(hudCvar.string))
			{
				hudData.active = CG_GetHudByNumber(hudCvar.integer);
			}
		}
	}

	modCount  = hudCvar.modificationCount;
	shoutcast = cgs.clientinfo[cg.clientNum].shoutcaster;

	if (!hudData.active)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING hud %s is not available, defaulting to 0\n", hudCvar.string);
		hudData.active = CG_GetHudByNumber(0);
		trap_Cvar_Set(shoutcast ? "cg_shoutcasterHud" : "cg_altHud", "0");
		return;
	}

#ifdef ETLEGACY_DEBUG
	CG_PrintHud(hudData.active);
#endif

	if (hudData.active->name[0])
	{
		Com_Printf("Setting hud to: '%s'\n", hudData.active->name);
	}
	else
	{
		Com_Printf("Setting hud to index: %i\n", hudData.active->hudnumber);
	}
}

static void CG_ComputeRectBasedOnPoint(rectDef_t *loc, anchorPoint_t point)
{
	switch (point)
	{
	case TOP_LEFT:
		break;
	case TOP_MIDDLE:
		loc->x += (loc->w / 2);
		break;
	case TOP_RIGHT:
		loc->x += loc->w;
		break;
	case MIDDLE_RIGHT:
		loc->x += loc->w;
		loc->y += (loc->h / 2);
		break;
	case BOTTOM_RIGHT:
		loc->x += loc->w;
		loc->y += loc->h;
		break;
	case BOTTOM_MIDDLE:
		loc->x += (loc->w / 2);
		loc->y += loc->h;
		break;
	case BOTTOM_LEFT:
		loc->y += loc->h;
		break;
	case MIDDLE_LEFT:
		loc->y += (loc->h / 2);
		break;
	case CENTER:
		loc->x += (loc->w / 2);
		loc->y += (loc->h / 2);
		break;
	}
}

void CG_CalculateComponentLocation(hudComponent_t *comp, int depth, rectDef_t *out)
{
	rectDef_t parentLoc, tmpCompLoc;

	// force quit this insanity
	if (depth > 10 || depth < 0)
	{
		Com_Printf(S_COLOR_RED "Hud component recursive dependency is too deep, para-shooting out of this mess!\n");
		return;
	}

	rect_copy(comp->internalLocation, tmpCompLoc);

	if (comp->anchorPoint)
	{
		rectDef_t tmpLoc;
		rect_copy(comp->internalLocation, tmpLoc);
		tmpLoc.x = tmpLoc.y = 0;
		CG_ComputeRectBasedOnPoint(&tmpLoc, comp->anchorPoint);

		tmpCompLoc.x -= tmpLoc.x;
		tmpCompLoc.y -= tmpLoc.y;
	}

	// are we depending on a component?
	if (comp->parentAnchor.parent)
	{
		CG_CalculateComponentLocation(comp->parentAnchor.parent, depth + 1, &parentLoc);
	}
	else
	{
		// our parent is the screen, so fill it in
		parentLoc.x = parentLoc.y = 0;
		parentLoc.w = SCREEN_WIDTH_F;
		parentLoc.h = SCREEN_HEIGHT_F;
	}

	// figure out the parent components anchor location
	CG_ComputeRectBasedOnPoint(&parentLoc, comp->parentAnchor.point);

	// final location
	tmpCompLoc.x += parentLoc.x;
	tmpCompLoc.y += parentLoc.y;

	etl_assert(out);
	rect_copy(tmpCompLoc, *out);
}

typedef struct
{
	anchorPoint_t self;
	anchor_t parent;
} anchorPoints_t;

static anchorPoints_t CG_ClosestAnchors(rectDef_t *self, rectDef_t *parent, float *outLen)
{
	rectDef_t      tmp, tmp2;
	int            i, x;
	float          length = FLT_MAX, tmpLen = 0;
	vec2_t         tmpVec, tmpVec2;
	anchorPoints_t ret = { TOP_LEFT, { NULL, TOP_LEFT } };

	for (i = 0; i <= CENTER; i++)
	{
		rect_copy(*self, tmp);
		CG_ComputeRectBasedOnPoint(&tmp, i);

		for (x = 0; x <= CENTER; x++)
		{
			rect_copy(*parent, tmp2);
			CG_ComputeRectBasedOnPoint(&tmp2, x);

			vec2_set(tmpVec, tmp.x, tmp.y);
			vec2_set(tmpVec2, tmp2.x, tmp2.y);
			vec2_sub(tmpVec, tmpVec2, tmpVec);
			tmpLen = vec2_length(tmpVec);

			if (tmpLen < length)
			{
				ret.self         = i;
				ret.parent.point = x;
				length           = tmpLen;
			}
		}
	}

	if (outLen)
	{
		*outLen = length;
	}

	return ret;
}

static ID_INLINE qboolean CG_CheckComponentParentDepth(hudComponent_t *target, hudComponent_t *possibleParent)
{
	unsigned int   i;
	hudComponent_t *tmp = NULL;

	for (i = 0; i < 10; i++)
	{
		tmp = possibleParent->parentAnchor.parent;

		if (tmp == possibleParent || tmp == target)
		{
			return qfalse;
		}
		else if (!tmp)
		{
			return qtrue;
		}
	}

	return qfalse;
}

static anchorPoints_t CG_FindClosestAnchors(hudStucture_t *hud, hudComponent_t *current)
{
	unsigned int   i;
	hudComponent_t *comp;
	rectDef_t      tmpParentRect, currentRect;
	float          length = FLT_MAX, tmpLen = 0;

	anchorPoints_t out = { TOP_LEFT, { NULL, TOP_LEFT } };
	anchorPoints_t tmpAnchors;

	rect_clear(tmpParentRect);
	rect_clear(currentRect);

	CG_CalculateComponentLocation(current, 0, &currentRect);

	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (hudComponentFields[i].isAlias)
		{
			continue;
		}

		comp = (hudComponent_t *)((char * )hud + hudComponentFields[i].offset);

		// can't parent to self
		if (comp == current)
		{
			continue;
		}

		CG_CalculateComponentLocation(comp, 0, &tmpParentRect);

		tmpAnchors = CG_ClosestAnchors(&currentRect, &tmpParentRect, &tmpLen);
		if (tmpLen < length && CG_CheckComponentParentDepth(current, comp))
		{
			out               = tmpAnchors;
			out.parent.parent = comp;
			length            = tmpLen;
		}
	}

	rect_clear(tmpParentRect);
	tmpParentRect.w = SCREEN_WIDTH_F;
	tmpParentRect.h = SCREEN_HEIGHT_F;

	tmpAnchors = CG_ClosestAnchors(&currentRect, &tmpParentRect, &tmpLen);
	if (tmpLen < length)
	{
		out               = tmpAnchors;
		out.parent.parent = NULL;
		length            = tmpLen;
	}

	return out;
}

qboolean CG_ComputeComponentPosition(hudComponent_t *comp, int depth)
{
	rectDef_t parentLoc, tmpLoc;
	float     xAbs;

	// force quit this insanity
	if (depth > 10 || depth < 0)
	{
		Com_Printf(S_COLOR_RED "Hud component recursive dependency is too deep, para-shooting out of this mess!\n");
		return qfalse;
	}

	rect_copy(comp->internalLocation, comp->location);
	comp->location.x = comp->location.y = 0;

	// are we depending on a component?
	if (comp->parentAnchor.parent)
	{
		if (!comp->parentAnchor.parent->computed)
		{
			if (!CG_ComputeComponentPosition(comp->parentAnchor.parent, depth + 1))
			{
				return qfalse;
			}
		}

		Com_Memcpy(&parentLoc, &comp->parentAnchor.parent->location, sizeof(rectDef_t));
	}
	else
	{
		// our parent is the screen, so fill it in
		parentLoc.x = parentLoc.y = 0;
		parentLoc.w = Ccg_WideX(SCREEN_WIDTH_F);
		parentLoc.h = SCREEN_HEIGHT_F;
	}

	// figure out the parent components anchor location
	CG_ComputeRectBasedOnPoint(&parentLoc, comp->parentAnchor.point);

	if (comp->anchorPoint)
	{
		rect_copy(comp->internalLocation, tmpLoc);
		tmpLoc.x = tmpLoc.y = 0;
		CG_ComputeRectBasedOnPoint(&tmpLoc, comp->anchorPoint);

		comp->location.x -= tmpLoc.x;
		comp->location.y -= tmpLoc.y;
	}

	// final location
	xAbs = Q_fabs(comp->internalLocation.x);

	if (xAbs)
	{
		comp->location.x += Ccg_WideX(xAbs) * (CG_IsFloatNegative(comp->internalLocation.x) ? -1.f : 1.f) + parentLoc.x;
	}
	else
	{
		comp->location.x += parentLoc.x;
	}

	comp->location.y += comp->internalLocation.y + parentLoc.y;

	comp->computed = qtrue;

	return qtrue;
}

static void CG_GenerateComponentAnchors(hudStucture_t *hud, hudComponent_t *comp, qboolean checkComponents, int depth, rectDef_t *out)
{
	rectDef_t      parentLoc, parentLocBackup, tmpCompLoc;
	anchorPoints_t points;

	// force quit this insanity
	if (depth > 10 || depth < 0)
	{
		Com_Printf(S_COLOR_RED "Hud component recursive dependency is too deep, para-shooting out of this mess!\n");
		return;
	}

	rect_copy(comp->internalLocation, tmpCompLoc);

	if (comp->anchorPoint)
	{
		rectDef_t tmpLoc;
		rect_copy(comp->internalLocation, tmpLoc);
		tmpLoc.x = tmpLoc.y = 0;
		CG_ComputeRectBasedOnPoint(&tmpLoc, comp->anchorPoint);

		tmpCompLoc.x -= tmpLoc.x;
		tmpCompLoc.y -= tmpLoc.y;
	}

	// are we depending on a component?
	if (comp->parentAnchor.parent)
	{
		CG_GenerateComponentAnchors(hud, comp->parentAnchor.parent, checkComponents, depth + 1, &parentLoc);
	}
	else
	{
		// our parent is the screen, so fill it in
		parentLoc.x = parentLoc.y = 0;
		parentLoc.w = SCREEN_WIDTH_F;
		parentLoc.h = SCREEN_HEIGHT_F;
	}
	rect_copy(parentLoc, parentLocBackup);

	// figure out the parent components anchor location
	CG_ComputeRectBasedOnPoint(&parentLoc, comp->parentAnchor.point);

	// final location
	tmpCompLoc.x += parentLoc.x;
	tmpCompLoc.y += parentLoc.y;

	if (out)
	{
		rect_copy(tmpCompLoc, *out);
		return;
	}

	comp->location.x = tmpCompLoc.x;
	comp->location.y = tmpCompLoc.y;

	// restore the parent backup
	rect_copy(parentLocBackup, parentLoc);

	// At this point we know the components real location in the 4/3 screen space
	if (checkComponents)
	{
		points = CG_FindClosestAnchors(hud, comp);
	}
	else
	{
		// find the closest valid anchors for the current locations
		points = CG_ClosestAnchors(&tmpCompLoc, &parentLoc, NULL);
	}

	if (points.self)
	{
		CG_ComputeRectBasedOnPoint(&tmpCompLoc, points.self);
	}

	if (points.parent.parent)
	{
		CG_CalculateComponentLocation(points.parent.parent, 0, &parentLoc);
	}
	else
	{
		parentLoc.x = parentLoc.y = 0;
		parentLoc.w = SCREEN_WIDTH_F;
		parentLoc.h = SCREEN_HEIGHT_F;
	}
	CG_ComputeRectBasedOnPoint(&parentLoc, points.parent.point);

	tmpCompLoc.x = tmpCompLoc.x - parentLoc.x;
	tmpCompLoc.y = tmpCompLoc.y - parentLoc.y;

	comp->internalLocation.x  = tmpCompLoc.x;
	comp->internalLocation.y  = tmpCompLoc.y;
	comp->anchorPoint         = points.self;
	comp->parentAnchor.point  = points.parent.point;
	comp->parentAnchor.parent = points.parent.parent;
}

void CG_GenerateHudAnchors(hudStucture_t *hud)
{
	unsigned int   i;
	hudComponent_t *comp;

	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (hudComponentFields[i].isAlias)
		{
			continue;
		}

		comp = (hudComponent_t *)((char * )hud + hudComponentFields[i].offset);

		if (comp && !comp->computed)
		{
			CG_GenerateComponentAnchors(hud, comp, qfalse, 0, NULL);
		}
	}
}

void CG_ComputeComponentPositions(hudStucture_t *hud)
{
	unsigned int   i;
	hudComponent_t *comp;

	if (hud->computed)
	{
		return;
	}

	for (i = 0; i < HUD_COMPONENTS_NUM; i++)
	{
		comp = hud->components[i];

		if (comp && !comp->computed)
		{
			if (!CG_ComputeComponentPosition(comp, 0))
			{
				Com_Printf(S_COLOR_RED "Could not setup component\n");
			}
		}
	}

	hud->computed = qtrue;
}

static ID_INLINE qboolean CG_Hud_HasParent(hudComponent_t *comp, hudComponent_t *has)
{
	hudComponent_t *tmp = comp;

	if (!comp || !has)
	{
		return qfalse;
	}

	if (comp == has)
	{
		return qtrue;
	}

	while ((tmp = tmp->parentAnchor.parent))
	{
		if (tmp == has)
		{
			return qtrue;
		}

		if (tmp == tmp->parentAnchor.parent)
		{
			CG_Printf(S_COLOR_YELLOW "Circular component dependency!\n");
			tmp->parentAnchor.parent = NULL;
			return qfalse;
		}
	}

	return qfalse;
}

void CG_CalculateComponentInternals(hudStucture_t *hud, hudComponent_t *comp)
{
	unsigned int   i;
	hudComponent_t *tmp;
	rectDef_t      parentLoc, tmpLoc;
	anchorPoints_t points;

	// At this point we go back to a virtual 4/3 screen
	if (comp->parentAnchor.parent)
	{
		// FIXME: check if we actually need to do something else?
		// FIXME: how to disconnect a component from parent on the editor?
		rect_copy(comp->parentAnchor.parent->location, parentLoc);
	}
	else
	{
		parentLoc.x = parentLoc.y = 0;
		parentLoc.w = Ccg_WideX(SCREEN_WIDTH_F);
		parentLoc.h = SCREEN_HEIGHT_F;
	}

	rect_copy(comp->location, tmpLoc);

	// find the closest valid anchors for the current locations
	points = CG_ClosestAnchors(&tmpLoc, &parentLoc, NULL);

	if (points.parent.point != comp->parentAnchor.point)
	{
		CG_Printf(S_COLOR_CYAN "Switched parent anchor point: %i -> %i\n", comp->parentAnchor.point, points.parent.point);
	}

	if (points.self != comp->anchorPoint)
	{
		CG_Printf(S_COLOR_CYAN "Switched component anchor point: %i -> %i\n", comp->anchorPoint, points.self);
	}

	if (points.self)
	{
		CG_ComputeRectBasedOnPoint(&tmpLoc, points.self);
	}

	CG_ComputeRectBasedOnPoint(&parentLoc, points.parent.point);

	tmpLoc.x = tmpLoc.x - parentLoc.x;
	tmpLoc.y = tmpLoc.y - parentLoc.y;

	comp->internalLocation.x = Ccg_WideXReverse(Q_fabs(tmpLoc.x)) * (CG_IsFloatNegative(tmpLoc.x) ? -1.f : 1.f);
	comp->internalLocation.y = tmpLoc.y;
	comp->internalLocation.w = tmpLoc.w;
	comp->internalLocation.h = tmpLoc.h;
	comp->anchorPoint        = points.self;
	comp->parentAnchor.point = points.parent.point;

	comp->computed = qfalse;

	for (i = 0; i < HUD_COMPONENTS_NUM; i++)
	{
		tmp = hud->components[i];

		if (!tmp)
		{
			continue;
		}

		if (tmp == comp)
		{
			continue;
		}

		if (CG_Hud_HasParent(tmp, comp))
		{
			tmp->computed = qfalse;
		}
	}

	hud->computed = qfalse;
}

/**
 * @brief CG_DrawActiveHud
 */
void CG_DrawActiveHud(void)
{
	unsigned int   i;
	hudComponent_t *comp;

	CG_ComputeComponentPositions(hudData.active);

	for (i = 0; i < HUD_COMPONENTS_NUM; i++)
	{
		comp = hudData.active->components[i];

		if ((comp && comp->visible && comp->draw) && !(showOnlyHudComponent != NULL && showOnlyHudComponent != comp))
		{
			comp->draw(comp);
		}
	}

	// Stats Debugging
	CG_DrawStatsDebug();
}
