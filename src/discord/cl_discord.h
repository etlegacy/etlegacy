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
 * If not, please request a copy in writing to id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file cl_discord.h
 * @brief Discord Rich Presence client wrapper
 *
 * @note This is FEATURE_DISCORD and _WIN32 only
 */

#ifndef CL_DISCORD_H
#define CL_DISCORD_H

#include "../qcommon/q_shared.h"

/**
 * @brief CL_DiscordInit
 * @note Initializes the Discord Rich Presence SDK. Called from CL_Init after CL_InitCvars.
 */
void CL_DiscordInit(void);

/**
 * @brief CL_DiscordFrame
 * @note Drains SDK callbacks + throttled presence update + state-transition/toggle polling. Called from CL_Frame every frame.
 */
void CL_DiscordFrame(void);

/**
 * @brief CL_DiscordShutdown
 * @note Tears down the Discord Rich Presence SDK. Called from CL_Shutdown before the client state is zeroed.
 */
void CL_DiscordShutdown(void);

#endif /* CL_DISCORD_H */
