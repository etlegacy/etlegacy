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

#ifndef INCLUDE_UI_CVARS_H
#define INCLUDE_UI_CVARS_H

#include "../qcommon/q_shared.h"

extern vmCvar_t ui_brassTime;
extern vmCvar_t ui_drawCrosshair;
extern vmCvar_t ui_drawCrosshairPickups;
extern vmCvar_t ui_drawSpectatorNames;
extern vmCvar_t ui_marks;

extern vmCvar_t ui_autoactivate;

extern vmCvar_t ui_selectedPlayer;
extern vmCvar_t ui_selectedPlayerName;
extern vmCvar_t ui_netSource;
extern vmCvar_t ui_menuFiles;
extern vmCvar_t ui_gameType;
extern vmCvar_t ui_netGameType;
extern vmCvar_t ui_joinGameType;
extern vmCvar_t ui_dedicated;

// multiplayer cvars
extern vmCvar_t ui_serverFilterType;
extern vmCvar_t ui_currentNetMap;
extern vmCvar_t ui_currentMap;
extern vmCvar_t ui_mapIndex;
extern vmCvar_t ui_browserShowEmptyOrFull;
extern vmCvar_t ui_browserShowPasswordProtected;
extern vmCvar_t ui_browserShowFriendlyFire;
extern vmCvar_t ui_browserShowMaxlives;
extern vmCvar_t ui_browserShowAntilag;
extern vmCvar_t ui_browserShowWeaponsRestricted;
extern vmCvar_t ui_browserShowTeamBalanced;
extern vmCvar_t ui_browserShowHumans;

extern vmCvar_t ui_browserModFilter;
extern vmCvar_t ui_browserMapFilter;
extern vmCvar_t ui_browserServerNameFilterCheckBox;

extern vmCvar_t ui_disableOssFilter;

extern vmCvar_t ui_serverStatusTimeOut;

extern vmCvar_t g_gameType;

extern vmCvar_t cl_profile;
extern vmCvar_t cl_defaultProfile;
extern vmCvar_t ui_profile;
extern vmCvar_t ui_currentNetCampaign;
extern vmCvar_t ui_currentCampaign;
extern vmCvar_t ui_campaignIndex;
extern vmCvar_t ui_currentCampaignCompleted;
extern vmCvar_t ui_blackout;
extern vmCvar_t ui_cg_crosshairColor;
extern vmCvar_t ui_cg_crosshairColorAlt;
extern vmCvar_t ui_cg_crosshairAlpha;
extern vmCvar_t ui_cg_crosshairAlphaAlt;
extern vmCvar_t ui_cg_crosshairSize;
extern vmCvar_t ui_cg_crosshairScaleX;
extern vmCvar_t ui_cg_crosshairScaleY;
extern vmCvar_t ui_cg_crosshairSVG;

extern vmCvar_t cl_bypassMouseInput;

extern vmCvar_t ui_serverBrowserSettings;

extern vmCvar_t ui_cg_shoutcastTeamNameRed;
extern vmCvar_t ui_cg_shoutcastTeamNameBlue;
extern vmCvar_t ui_cg_shoutcastDrawHealth;
extern vmCvar_t ui_cg_shoutcastGrenadeTrail;

extern vmCvar_t ui_customFont1;
extern vmCvar_t ui_customFont2;

#endif  // #ifndef INCLUDE_UI_CVARS_H
