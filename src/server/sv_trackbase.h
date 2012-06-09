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
 * @file sv_trackbase.h
 * @brief Sends game statistics to Trackbase
 */
#ifdef TRACKBASE_SUPPORT

#ifndef _TRACKBASE_H
#define _TRACKBASE_H

#define TRACKBASE_ADDR "et-tracker.trackbase.net:4444"

#include "server.h"
//#include "../qcommon/q_shared.h"

void TB_Init();
void TB_ServerStart();
void TB_ServerStop();
void TB_ClientConnect(client_t *cl);
void TB_ClientDisconnect(client_t *cl);
void TB_ClientName(client_t *cl);
void TB_Map(char *mapname);
void TB_MapRestart();
void TB_MapEnd();
// void TB_TeamSwitch(client_t *cl); // unused
void TB_Frame(int msec);

void TB_catchBotConnect(int clientNum);
qboolean TB_catchServerCommand(int clientNum, char *msg);

void TB_requestWeaponStats();

#endif // _TRACKBASE_H

#endif // TRACKBASE_SUPPORT
