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
 * @file irc_client.h
 * @brief
 *
 * @note This is FEATURE_IRC_CLIENT and FEATURE_IRC_SERVER only
 */

// Hash table interface
#ifndef IRC_CLIENT_H
#define IRC_CLIENT_H

/* IRC control cvars */
extern cvar_t *irc_mode;
extern cvar_t *irc_server;
extern cvar_t *irc_channel;
extern cvar_t *irc_port;
extern cvar_t *irc_nickname;
extern cvar_t *irc_kick_rejoin;
extern cvar_t *irc_reconnect_delay;

#define IRCM_AUTO_CONNECT           1
#define IRCM_AUTO_OVERRIDE_NICKNAME 2
#define IRCM_MUTE_CHANNEL           4
#define IRCM_CHANNEL_TO_CHAT        8 ///< dedicated only

void IRC_Init(void);
void IRC_Connect(void);
void IRC_InitiateShutdown(void);
void IRC_WaitShutdown(void);
void IRC_Say(void);
qboolean IRC_IsConnected(void);
qboolean IRC_IsRunning(void);

#endif // IRC_CLIENT_H
