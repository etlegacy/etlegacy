/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
 * Copyright (C) 2012 Konrad Mosoń <mosonkonrad@gmail.com>
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
 *
 * @file sv_tracker.h
 * @brief Sends game statistics to Trackbase
 */
#ifdef FEATURE_TRACKER

#ifndef _TRACKER_H
#define _TRACKER_H

#define TRACKER_ADDR "et-tracker.trackbase.net:4444"

#include "server.h"
//#include "../qcommon/q_shared.h"

void Tracker_Init(void);
void Tracker_ServerStart(void);
void Tracker_ServerStop(void);
void Tracker_ClientConnect(client_t *cl);
void Tracker_ClientDisconnect(client_t *cl);
void Tracker_ClientName(client_t *cl);
void Tracker_Map(char *mapname);
void Tracker_MapRestart(void);
void Tracker_MapEnd(void);
// void Tracker_TeamSwitch(client_t *cl); // unused
void Tracker_Frame(int msec);

void Tracker_catchBotConnect(int clientNum);
qboolean Tracker_catchServerCommand(int clientNum, char *msg);

void Tracker_requestWeaponStats(void);

#endif // _TRACKER_H

#endif // FEATURE_TRACKER
