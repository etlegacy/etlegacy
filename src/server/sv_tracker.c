/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 */
/**
 * @file sv_tracker.h
 * @brief Sends game statistics to Tracker
 * @author Konrad "morsik" Mosoń
 */
#ifdef FEATURE_TRACKER

#include "sv_tracker.h"

long t;
int  waittime = 15; // seconds
char expect[16];
int  expectnum;

qboolean maprunning = qfalse;

int querycl = -1;

enum
{
	TR_BOT_NONE,
	TR_BOT_CONNECT
} catchBot;
qboolean catchBotNum = 0;

/**
 * @def MAX_TRACKERS
 * @brief Maximum number of tracker endpoints parsed from the sv_tracker cvar.
 *
 * @note The sv_tracker cvar value is bounded by MAX_CVAR_VALUE_STRING (256)
 *       chars by the cvar system itself, so fitting MAX_TRACKERS entries
 *       only works when the configured hostnames / IPs are reasonably short.
 *       Prefer short hostnames or raw IPs when configuring many endpoints.
 */
#define MAX_TRACKERS 8

netadr_t trackerAddrs[MAX_TRACKERS];
int      numTrackerAddrs = 0;
static cvar_t *sv_tracker_cvar = NULL;

char infostring[MAX_INFO_STRING];

char *Tracker_getGUID(client_t *cl);

/**
 * @brief Try to resolve a single address and add it to the tracker list
 * @param[in] addr_str Address string (already trimmed, non-empty)
 */
static void Tracker_AddAddress(const char *addr_str)
{
	if (numTrackerAddrs >= MAX_TRACKERS)
	{
		Com_Printf("Max trackers (%i) reached, ignoring: %s\n", MAX_TRACKERS, addr_str);
		return;
	}

	Com_Printf("Resolving %s\n", addr_str);
	if (!NET_StringToAdr(addr_str, &trackerAddrs[numTrackerAddrs], NA_IP))
	{
		Com_Printf("Couldn't resolve address: %s\n", addr_str);
		return;
	}

	Com_Printf("%s resolved to %i.%i.%i.%i:%i\n", addr_str,
	           trackerAddrs[numTrackerAddrs].ip[0],
	           trackerAddrs[numTrackerAddrs].ip[1],
	           trackerAddrs[numTrackerAddrs].ip[2],
	           trackerAddrs[numTrackerAddrs].ip[3],
	           BigShort(trackerAddrs[numTrackerAddrs].port));
	numTrackerAddrs++;
}

/**
 * @brief Parse a whitespace-separated list of addresses and resolve each entry
 * @param[in] list Raw list from sv_tracker cvar
 *
 * Uses COM_ParseExt() to stay consistent with the engine's standard tokenizer
 * used elsewhere for cvar lists. Tokens are separated by any whitespace
 * (space, tab). Example cvar value: "tracker1.example.com:4444 tracker2.example.com:4444".
 */
static void Tracker_ParseAddressList(const char *list)
{
	char buf[MAX_CVAR_VALUE_STRING];
	char *p;
	char *token;

	if (!list || !*list)
	{
		return;
	}

	Q_strncpyz(buf, list, sizeof(buf));
	p = buf;

	while (1)
	{
		token = COM_ParseExt(&p, qfalse);
		if (!token[0])
		{
			break;
		}

		Tracker_AddAddress(token);
	}
}

/**
 * @brief Sends data to Tracker
 * @param[in] format Formatted data to send
 */
void Tracker_Send(char *format, ...)
{
	va_list argptr;
	char    msg[MAX_MSGLEN];
	int i;

	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg), format, argptr);
	va_end(argptr);

	for (i = 0; i < numTrackerAddrs; i++)
	{
		NET_OutOfBandPrint(NS_SERVER, &trackerAddrs[i], "%s", msg);
	}
}

/**
 * @brief Initialize Tracker support
 */
void Tracker_Init(void)
{
	const char *tracker;

	if (!(sv_advert->integer & SVA_TRACKER))
	{
		Com_Printf("Tracker: Server communication disabled by sv_advert.\n");
		return;
	}

	sv_tracker_cvar = Cvar_Get("sv_tracker", "et-tracker.trackbase.net:4444", CVAR_PROTECTED);
	tracker         = sv_tracker_cvar->string;
	sv_tracker_cvar->modified = qfalse;
	t               = time(0);
	expectnum       = 0;
	numTrackerAddrs = 0;

	Tracker_ParseAddressList(tracker);

	Com_Printf("Tracker: Server communication enabled (%i endpoint(s)).\n", numTrackerAddrs);
}

/**
 * @brief Send info about server startup
 */
void Tracker_ServerStart(void)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	Tracker_Send("start");
}

/**
 * @brief Send info about server shutdown
 */
void Tracker_ServerStop(void)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	Tracker_Send("stop");
}

/**
 * @brief Send info about new client connected
 * @param[in] cl Client
 */
void Tracker_ClientConnect(client_t *cl)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	Tracker_Send("connect %i %s %s", (int)(cl - svs.clients), Tracker_getGUID(cl), cl->name);
}

/**
 * @brief Send info when client disconnects
 * @param[in] cl Client
 */
void Tracker_ClientDisconnect(client_t *cl)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	Tracker_Send("disconnect %i", (int)(cl - svs.clients));
}

/**
 * @brief Send info when player changes his name
 * @param[in] cl Client
 */
void Tracker_ClientName(client_t *cl)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	if (!*cl->name)
	{
		return;
	}

	Tracker_Send("name %i %s %s", (int)(cl - svs.clients), Tracker_getGUID(cl), Info_ValueForKey(cl->userinfo, "name"));
}

/**
 * @brief Send info when map has changed
 * @param[in] mapname Current Map
 */
void Tracker_Map(char *mapname)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	Tracker_Send("map %s", mapname);
	maprunning = qtrue;
}

/**
 * @brief Send info when map restarts
 *
 * Allows counting time from 0 again on TB
 */
void Tracker_MapRestart(void)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	Tracker_Send("maprestart");
	maprunning = qtrue;
}

/**
 * @brief Send info when map has finished
 *
 * Sometimes intermission is very long, so TB can show appropriate info to players
 */
void Tracker_MapEnd(void)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	Tracker_Send("mapend");
	Tracker_requestWeaponStats();
	maprunning = qfalse;
}

/**
 * @brief Creates client information for other functions
 * @param clientNum Client ID (from 0 to MAX_CLIENTS)
 * @note Just for internal use
 */
char *Tracker_createClientInfo(int clientNum)
{
	playerState_t *ps;
	int           playerClass;
	char          *modName;
	ps = SV_GameClientNum(clientNum);

	// PauluzzNL
	// The indexes are different between mods, most mods use the default etmain value of 5 for STAT_PLAYER_CLASS, legacy mod uses index 4.
	// may need to modify for more mods later
	modName = Info_ValueForKey(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE), "gamename");
	if (Q_strncmp(modName, "legacy", 6) == 0)
	{
		playerClass = ps->stats[STAT_PLAYER_CLASS];
	}
	else
	{
		playerClass = ps->stats[5];
	}
	return va("%i\\%i\\%c\\%i\\%s", svs.clients[clientNum].ping, ps->persistant[PERS_SCORE], Info_ValueForKey(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE), "P")[clientNum], playerClass, svs.clients[clientNum].name);
}

/**
 * @brief Request weapon stats data from mod
 */
void Tracker_requestWeaponStats(void)
{
	int      i;
	qboolean onlybots = qtrue;
	char     *P;

	if (!maprunning)
	{
		return;
	}

	Q_strncpyz(infostring, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE), sizeof(infostring));
	P = Info_ValueForKey(infostring, "P");

	Q_strncpyz(expect, "ws", sizeof(expect));
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
		Tracker_Send("wsc %i", expectnum);

		for (i = 0; i < sv_maxclients->value; i++)
		{
			if (svs.clients[i].state == CS_ACTIVE)
			{
				// send basic data is client is spectator
				if (P[i] == '3' || (svs.clients[i].netchan.remoteAddress.type == NA_BOT && onlybots))
				{
					Tracker_Send("ws %i 0 0 0\\%s", i, Tracker_createClientInfo(i));
				}
			}
		}

		if (querycl >= 0)
		{
			SV_ExecuteClientCommand(&svs.clients[querycl], "statsall", qtrue, qfalse);
		}
	}
}

/**
 * @brief Frame function
 */
void Tracker_Frame(int msec)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	if (sv_tracker_cvar && sv_tracker_cvar->modified)
	{
		Com_Printf("Tracker: sv_tracker changed, reinitializing...\n");
		Tracker_Init();
		return;
	}

	if (catchBot == TR_BOT_CONNECT)
	{
		Tracker_ClientConnect(&svs.clients[catchBotNum]);
		catchBot = TR_BOT_NONE;
	}

	if (!(time(0) - waittime > t))
	{
		return;
	}

	Tracker_Send("p"); // send ping to tb to show that server is still alive

	expectnum = 0; // reset before next statsall
	Tracker_requestWeaponStats();

	t = time(0);
}

/**
 * @brief Catches server command
 * @param     clientNum Client ID (from 0 to MAX_CLIENTS)
 * @param[in] msg       Message sends by backend
 */
qboolean Tracker_catchServerCommand(int clientNum, char *msg)
{
	int slot;

	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return qfalse;
	}

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
			expect[0] = 0;
			querycl   = -1;
		}

		slot = 0;
		sscanf(msg, "ws %i", &slot);
		Tracker_Send("%s\\%s", msg, Tracker_createClientInfo(slot));

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Catch bot connection
 */
void Tracker_catchBotConnect(int clientNum)
{
	if (!(sv_advert->integer & SVA_TRACKER))
	{
		return;
	}

	catchBot    = TR_BOT_CONNECT;
	catchBotNum = clientNum;
}

/**
 * @brief Get guid for stats
 * @param[in] cl Client
 *
 * We prefer to use original cl_guid, but some mods has their own guid values
 */
char * Tracker_getGUID(client_t *cl)
{
	char *cl_guid = Info_ValueForKey(cl->userinfo, "cl_guid");
	if (strcmp("", cl_guid) != 0 && strcmp("unknown", cl_guid) != 0)
	{
		return cl_guid;
	}
	else
	{
		if (*Info_ValueForKey(cl->userinfo, "n_guid"))
		{
			return Info_ValueForKey(cl->userinfo, "n_guid");
		}
		else if (*Info_ValueForKey(cl->userinfo, "sil_guid"))
		{
			return Info_ValueForKey(cl->userinfo, "sil_guid");
		}
		else
		{
			return "unknown";
		}
	}
}

#endif // FEATURE_TRACKER
