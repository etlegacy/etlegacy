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

#include "sv_trackbase.h"

long t;
int  waittime = 15; // seconds
char expect[16];
int  expectnum;

qboolean maprunning = qfalse;

int querycl = -1;

enum
{
	TB_BOT_NONE,
	TB_BOT_CONNECT
} catchBot;
qboolean catchBotNum = 0;

netadr_t addr;
#ifdef TRACKBASE_DEBUG
netadr_t local;
#endif

char infostring[MAX_INFO_STRING];

char *TB_getGUID(client_t *cl);

void TB_Send(char *format, ...)
{
	va_list argptr;
	char    msg[MAX_MSGLEN];

	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg), format, argptr);
	va_end(argptr);

	NET_OutOfBandPrint(NS_SERVER, addr, "%s", msg);
#ifdef TRACKBASE_DEBUG
	NET_OutOfBandPrint(NS_SERVER, local, "%s", msg);
#endif
}

void TB_Init()
{
	t         = time(0);
	expectnum = 0;

	NET_StringToAdr(TRACKBASE_ADDR, &addr, NA_IP);
#ifdef TRACKBASE_DEBUG
	NET_StringToAdr("127.0.0.1:6066", &local, NA_IP);
#endif
}

void TB_ServerStart()
{
	TB_Send("start");
}

void TB_ServerStop()
{
	TB_Send("stop");
}

void TB_ClientConnect(client_t *cl)
{
	TB_Send("connect %i %s %s", (int)(cl - svs.clients), TB_getGUID(cl), cl->name);
}

void TB_ClientDisconnect(client_t *cl)
{
	TB_Send("disconnect %i", (int)(cl - svs.clients));
}

void TB_ClientName(client_t *cl)
{
	if (!*cl->name)
	{
		return;
	}

	TB_Send("name %i %s %s", (int)(cl - svs.clients), TB_getGUID(cl), Info_ValueForKey(cl->userinfo, "name"));
}

void TB_ClientTeam(client_t *cl)
{
	playerState_t *ps;
	ps = SV_GameClientNum(cl - svs.clients);

	TB_Send("team %i %i %i %s", (int)(cl - svs.clients), Info_ValueForKey(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE), "P")[cl - svs.clients], ps->stats[STAT_PLAYER_CLASS], cl->name);
}

void TB_Map(char *mapname)
{
	TB_Send("map %s", mapname);
	maprunning = qtrue;
}

void TB_MapRestart()
{
	TB_Send("maprestart");
	maprunning = qtrue;
}

void TB_MapEnd()
{
	TB_Send("mapend");
	TB_requestWeaponStats();
	maprunning = qfalse;
}

void TB_TeamSwitch(client_t *cl)
{
	TB_Send("team %i", (int)(cl - svs.clients));
}

char *TB_makeClientInfo(int clientNum)
{
	playerState_t *ps;
	ps = SV_GameClientNum(clientNum);

	return va("%i\\%i\\%c\\%i\\%s", svs.clients[clientNum].ping, ps->persistant[PERS_SCORE], Info_ValueForKey(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE), "P")[clientNum], ps->stats[STAT_PLAYER_CLASS], svs.clients[clientNum].name);
}

void TB_requestWeaponStats()
{
	int      i;
	qboolean onlybots = qtrue;
	char     *P;

	if (!maprunning)
	{
		return;
	}

	strcpy(infostring, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));
	P = Info_ValueForKey(infostring, "P");

	strcpy(expect, "ws");
	for (i = 0; i < sv_maxclients->value; i++)
	{
		if (svs.clients[i].state == CS_ACTIVE)
		{
			if (svs.clients[i].netchan.remoteAddress.type != NA_BOT)
			{
				onlybots = qfalse;
				querycl  = i;
			}
			expectnum++;
		}
	}

	if (expectnum > 0)
	{
		TB_Send("wsc %i", expectnum);

		for (i = 0; i < sv_maxclients->value; i++)
		{
			if (svs.clients[i].state == CS_ACTIVE)
			{
				// send basic data is client is spectator
				if (P[i] == '3' || (svs.clients[i].netchan.remoteAddress.type == NA_BOT && onlybots))
				{
					TB_Send("ws %i 0 0 0\\%s", i, TB_makeClientInfo(i));
				}
			}
		}

		if (querycl >= 0)
		{
			SV_ExecuteClientCommand(&svs.clients[querycl], "statsall", qtrue, qfalse);
		}
	}
}

void TB_Frame(int msec)
{
	if (catchBot == TB_BOT_CONNECT)
	{
		TB_ClientConnect(&svs.clients[catchBotNum]);
		catchBot = TB_BOT_NONE;
	}

	if (!(time(0) - waittime > t))
	{
		return;
	}

	TB_Send("p"); // send ping to tb to show that server is still alive

	expectnum = 0; // reset before next statsall
	TB_requestWeaponStats();

	t = time(0);
}

qboolean TB_catchServerCommand(int clientNum, char *msg)
{
	int slot;

	if (clientNum != querycl)
	{
		return qfalse;
	}

	if (expectnum == 0)
	{
		return qfalse;
	}

	if (!(!strncmp(expect, msg, strlen(expect))))
	{
		return qfalse;
	}

	if (msg[strlen(msg) - 1] == '\n')
	{
		msg[strlen(msg) - 1] = '\0';
	}

	if (!Q_strncmp("ws", msg, 2))
	{
		expectnum--;
		if (expectnum == 0)
		{
			strcpy(expect, "");
			querycl = -1;
		}
		slot = 0;
		sscanf(msg, "ws %i", &slot);
		TB_Send("%s\\%s", msg, TB_makeClientInfo(slot));
		return qtrue;
	}

	return qfalse;
}

void TB_catchBotConnect(int clientNum)
{
	catchBot    = TB_BOT_CONNECT;
	catchBotNum = clientNum;
}

char *TB_getGUID(client_t *cl)
{
	if (*Info_ValueForKey(cl->userinfo, "cl_guid"))
	{
		return Info_ValueForKey(cl->userinfo, "cl_guid");
	}
	else if (*Info_ValueForKey(cl->userinfo, "n_guid"))
	{
		return Info_ValueForKey(cl->userinfo, "n_guid");
	}
	else
	{
		return "unknown";
	}
}

#endif // TRACKBASE_SUPPORT
