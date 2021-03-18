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
 * @file cl_ui.c
 */

#include "client.h"

#ifdef FEATURE_DBMS
#include "../db/db_sql.h" // FIXME; only for db_init (this is a com cvar ...)
#endif

#include "../botlib/botlib.h"

extern botlib_export_t *botlib_export;

vm_t *uivm;

/**
 * @brief GetClientState
 * @param[out] state
 */
static void GetClientState(uiClientState_t *state)
{
	state->connectPacketCount = clc.connectPacketCount;
	state->connState          = cls.state;
	Q_strncpyz(state->servername, cls.servername, sizeof(state->servername));
	Q_strncpyz(state->updateInfoString, cls.updateInfoString, sizeof(state->updateInfoString));
	Q_strncpyz(state->messageString, clc.serverMessage, sizeof(state->messageString));
	state->clientNum = cl.snap.ps.clientNum;
}

/**
 * @brief LAN_LoadCachedServers
 */
void LAN_LoadCachedServers(void)
{
	int32_t      size;
	fileHandle_t fileIn;
	char         filename[MAX_QPATH];

	cls.numglobalservers         = 0;
	cls.numfavoriteservers       = 0;
	cls.numGlobalServerAddresses = 0;

	Com_Memset(&cls.globalServers, 0, sizeof(cls.globalServers));
	Com_Memset(&cls.favoriteServers, 0, sizeof(cls.favoriteServers));
	Com_Memset(&cls.numGlobalServerAddresses, 0, sizeof(cls.numGlobalServerAddresses));

	if (cl_profile->string[0])
	{
#ifdef FEATURE_DBMS
		if (db_mode->integer > 0)
		{
			DB_LoadFavorites(cl_profile->string);

			return; // don't load favcache.dat
		}
#endif
		Com_sprintf(filename, sizeof(filename), "profiles/%s/favcache.dat", cl_profile->string);
	}
	else
	{
		Q_strncpyz(filename, "favcache.dat", sizeof(filename));
	}

	// moved to mod/profiles dir
	if (FS_FOpenFileRead(filename, &fileIn, qtrue) > 0)
	{
		FS_Read(&cls.numfavoriteservers, sizeof(int32_t), fileIn);
		FS_Read(&size, sizeof(int32_t), fileIn);
		if (size == sizeof(cls.favoriteServers))
		{
			FS_Read(&cls.favoriteServers, sizeof(cls.favoriteServers), fileIn);
		}
		else
		{
			cls.numfavoriteservers = 0;
		}
		FS_FCloseFile(fileIn);

		Com_Printf("Total favorite servers restored: %i\n", cls.numfavoriteservers);
	}
	else
	{
		Com_Printf("Warning: can't read '%s' - no favorite servers restored.\n", filename);
	}
}

/**
 * @brief Saves favorite servers into .dat file format
 */
void LAN_SaveServersToFile(void)
{
	int32_t      size;
	fileHandle_t fileOut;
	char         filename[MAX_QPATH];

	// moved to mod/profiles dir
	if (cl_profile->string[0])
	{
		Com_sprintf(filename, sizeof(filename), "profiles/%s/favcache.dat", cl_profile->string);
	}
	else
	{
		Q_strncpyz(filename, "favcache.dat", sizeof(filename));
	}

	if ((fileOut = FS_FOpenFileWrite(filename)) <= 0)
	{
		Com_Printf("Saving favorites failed: Can't open file to write\n");
		return;
	}

	(void) FS_Write(&cls.numfavoriteservers, sizeof(int32_t), fileOut);

	size = sizeof(cls.favoriteServers);

	(void) FS_Write(&size, sizeof(int32_t), fileOut);
	(void) FS_Write(&cls.favoriteServers, sizeof(cls.favoriteServers), fileOut);
	FS_FCloseFile(fileOut);

	Com_Printf("Total favorite servers saved: %i\n", cls.numfavoriteservers);
}

/**
 * @brief LAN_ResetPings
 * @param[in] source
 */
static void LAN_ResetPings(int source)
{
	int          count    = 0, i;
	serverInfo_t *servers = NULL;

	switch (source)
	{
	case AS_LOCAL:
		servers = &cls.localServers[0];
		count   = MAX_OTHER_SERVERS;
		break;
	case AS_GLOBAL:
		servers = &cls.globalServers[0];
		count   = MAX_GLOBAL_SERVERS;
		break;
	case AS_FAVORITES:
		servers = &cls.favoriteServers[0];
		count   = MAX_FAVOURITE_SERVERS;
		break;
	}

	if (servers)
	{
		for (i = 0; i < count; i++)
		{
			servers[i].ping = -1;
		}
	}
}

/**
 * @brief Adds servers to the internal data structure
 * @note  Never ever add a localhost!
 *
 * @param[in] source
 * @param[in] name
 * @param[in] address
 * @return 1 on success
 */
int LAN_AddServer(int source, const char *name, const char *address)
{
	int          max, *count = 0;
	netadr_t     adr;
	serverInfo_t *servers = NULL;

	if (NET_IsLocalAddressString(address))
	{
		Com_Printf("LAN_AddServer Warning: Can't add localhost\n");
		return -1;
	}

	switch (source)
	{
	case AS_LOCAL:
		max     = MAX_OTHER_SERVERS;
		count   = &cls.numlocalservers;
		servers = &cls.localServers[0];
		break;
	case AS_GLOBAL:
		max     = MAX_GLOBAL_SERVERS;
		count   = &cls.numglobalservers;
		servers = &cls.globalServers[0];
		break;
	case AS_FAVORITES:
		max     = MAX_FAVOURITE_SERVERS;
		count   = &cls.numfavoriteservers;
		servers = &cls.favoriteServers[0];
#ifdef FEATURE_DBMS
		if (db_mode->integer > 0 && cl_profile->string[0])
		{
			DB_InsertFavorite(cl_profile->string, source, name, address, "");
		}
#endif
		break;
	default:
		Com_Printf("LAN_AddServer Warning: Invalid source\n");
		return -1;
	}

	if (servers && *count < max)
	{
		int i;

		NET_StringToAdr(address, &adr, NA_UNSPEC);
		for (i = 0; i < *count; i++)
		{
			if (NET_CompareAdr(servers[i].adr, adr))
			{
				break;
			}
		}
		if (i >= *count)
		{
			servers[*count].adr = adr;
			Q_strncpyz(servers[*count].hostName, name, sizeof(servers[*count].hostName));
			servers[*count].visible = qtrue;
			(*count)++;

#ifdef FEATURE_DBMS
			if (source == AS_FAVORITES && db_mode->integer == 0)
#else
			if (source == AS_FAVORITES)
#endif
			{
				LAN_SaveServersToFile();
			}

			return 1;
		}
		return 0;
	}
	return -1;
}

/**
 * @brief Removes server(s) from the 'cache'
 * @param[in] source Positive values AS_LOCAL, AS_GLOBAL, AS_FAVORITES
 *               or negative values AS_LOCAL_ALL, AS_GLOBAL_ALL, AS_FAVORITES_ALL to remove all
 * @param[in] addr   Server address - in case of negative source param addr is not required
 *
 */
static void LAN_RemoveServer(int source, const char *addr)
{
	int          *count   = 0;
	serverInfo_t *servers = NULL;

	switch (source)
	{
	case AS_LOCAL:
	case AS_LOCAL_ALL:
		count   = &cls.numlocalservers;
		servers = &cls.localServers[0];
		break;
	case AS_GLOBAL:
	case AS_GLOBAL_ALL:
		count   = &cls.numglobalservers;
		servers = &cls.globalServers[0];
		break;
	case AS_FAVORITES:
	case AS_FAVORITES_ALL:
		count   = &cls.numfavoriteservers;
		servers = &cls.favoriteServers[0];
		break;
	}

	if (servers && count && *count > 0)
	{
		if (source >= AS_LOCAL) // single server removal
		{
			netadr_t comp;
			int      i, j;

			NET_StringToAdr(addr, &comp, NA_UNSPEC);
			for (i = 0; i < *count; i++)
			{
				if (NET_CompareAdr(comp, servers[i].adr))
				{
					j = i;

					while (j < *count - 1)
					{
						Com_Memcpy(&servers[j], &servers[j + 1], sizeof(servers[j]));
						j++;
					}
					(*count)--;
					break;
				}
			}

			if (source == AS_FAVORITES)
			{
#ifdef FEATURE_DBMS
				if (db_mode->integer > 0 && cl_profile->string[0])
				{
					DB_DeleteFavorite(cl_profile->string, addr);
				}
				else
#endif
				{
					LAN_SaveServersToFile();
				}

			}
		}
		else // remove all
		{
			switch (source)
			{
			case AS_LOCAL_ALL:
				Com_Printf("Removing %i local servers\n", cls.numlocalservers);
				cls.numlocalservers = 0;
				Com_Memset(&cls.localServers, 0, sizeof(cls.localServers));
				break;
			case AS_GLOBAL_ALL:
				Com_Printf("Removing %i global servers\n", cls.numglobalservers);
				cls.numglobalservers = 0;
				Com_Memset(&cls.globalServers, 0, sizeof(cls.globalServers));
				break;
			case AS_FAVORITES_ALL:
				Com_Printf("Removing %i favourite servers\n", cls.numfavoriteservers);
				cls.numfavoriteservers = 0;
				Com_Memset(&cls.favoriteServers, 0, sizeof(cls.favoriteServers));

#ifdef FEATURE_DBMS
				if (db_mode->integer > 0)
				{
					DB_DeleteFavorite(cl_profile->string, "*");
				}
				else
#endif
				{
					LAN_SaveServersToFile();
				}

				break;
			default:
				Com_Printf("LAN_RemoveServer: Invalid source\n");
				break;
			}
		}
	}
	else
	{
		Com_Printf("No server found - nothing to remove\n");
	}
}

/**
 * @brief LAN_GetServerCount
 * @param[in] source
 * @return
 */
static int LAN_GetServerCount(int source)
{
	switch (source)
	{
	case AS_LOCAL:
		return cls.numlocalservers;
	case AS_GLOBAL:
		return cls.numglobalservers;
	case AS_FAVORITES:
		return cls.numfavoriteservers;
	default:
		Com_Printf("LAN_GetServerCount Warning: Invalid source\n");
		break;
	}
	return 0;
}

/**
 * @brief LAN_GetServerAddressString
 * @param[in] source
 * @param[in] n
 * @param[out] buf
 * @param[in] buflen
 */
static void LAN_GetServerAddressString(int source, int n, char *buf, size_t buflen)
{
#ifdef _WIN64
	if (buflen > 1024)
	{
		buflen = (size_t)1024;
	}
#endif // WIN64

	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS)
		{
			Q_strncpyz(buf, NET_AdrToString(cls.localServers[n].adr), buflen);
			return;
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS)
		{
			Q_strncpyz(buf, NET_AdrToString(cls.globalServers[n].adr), buflen);
			return;
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_FAVOURITE_SERVERS)
		{
			Q_strncpyz(buf, NET_AdrToString(cls.favoriteServers[n].adr), buflen);
			return;
		}
		break;
	default:
		Com_Printf("LAN_GetServerAddressString Warning: Invalid source\n");
		break;
	}
	buf[0] = '\0';
}

/**
 * @brief LAN_GetServerInfo
 * @param[in] source
 * @param[in] n
 * @param[in] buf
 * @param[in] buflen
 */
static void LAN_GetServerInfo(int source, int n, char *buf, size_t buflen)
{
	char         info[MAX_STRING_CHARS];
	serverInfo_t *server = NULL;
	info[0] = '\0';

	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS)
		{
			server = &cls.localServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS)
		{
			server = &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_FAVOURITE_SERVERS)
		{
			server = &cls.favoriteServers[n];
		}
		break;
	default:
		Com_Printf("LAN_GetServerInfo Warning: Invalid source\n");
		return;
	}
	if (server && buf)
	{
		buf[0] = '\0';
		Info_SetValueForKey(info, "version", server->version);
		Info_SetValueForKey(info, "hostname", server->hostName);
		Info_SetValueForKey(info, "serverload", va("%i", server->load));
		Info_SetValueForKey(info, "mapname", server->mapName);
		Info_SetValueForKey(info, "clients", va("%i", server->clients));
		Info_SetValueForKey(info, "humans", va("%i", server->humans));
		Info_SetValueForKey(info, "sv_maxclients", va("%i", server->maxClients));
		Info_SetValueForKey(info, "sv_privateclients", va("%i", server->privateClients));
		Info_SetValueForKey(info, "ping", va("%i", server->ping));
		Info_SetValueForKey(info, "minping", va("%i", server->minPing));
		Info_SetValueForKey(info, "maxping", va("%i", server->maxPing));
		Info_SetValueForKey(info, "game", server->game);
		Info_SetValueForKey(info, "gametype", va("%i", server->gameType));
		Info_SetValueForKey(info, "nettype", va("%i", server->netType));
		Info_SetValueForKey(info, "addr", NET_AdrToString(server->adr));
		Info_SetValueForKey(info, "friendlyFire", va("%i", server->friendlyFire));
		Info_SetValueForKey(info, "maxlives", va("%i", server->maxlives));
		Info_SetValueForKey(info, "needpass", va("%i", server->needpass));
		Info_SetValueForKey(info, "punkbuster", va("%i", server->punkbuster));
		Info_SetValueForKey(info, "gamename", server->gameName);
		Info_SetValueForKey(info, "g_antilag", va("%i", server->antilag));
		Info_SetValueForKey(info, "weaprestrict", va("%i", server->weaprestrict));
		Info_SetValueForKey(info, "balancedteams", va("%i", server->balancedteams));
#ifdef _WIN64
		if (buflen > 1024)
		{
			buflen = (size_t)1024;
		}
#endif // WIN64
		Q_strncpyz(buf, info, buflen);
	}
	else
	{
		if (buf)
		{
			buf[0] = '\0';
		}
	}
}

/**
 * @brief LAN_GetServerPing
 * @param[in] source
 * @param[in] n
 * @return
 */
static int LAN_GetServerPing(int source, int n)
{
	serverInfo_t *server = NULL;

	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS)
		{
			server = &cls.localServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS)
		{
			server = &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_FAVOURITE_SERVERS)
		{
			server = &cls.favoriteServers[n];
		}
		break;
	default:
		Com_Printf("LAN_GetServerPing Warning: Invalid source\n");
		break;
	}
	if (server)
	{
		return server->ping;
	}
	return -1;
}

/**
 * @brief LAN_GetServerPtr
 * @param[in] source
 * @param[in] n
 * @return
 */
static serverInfo_t *LAN_GetServerPtr(int source, int n)
{
	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS)
		{
			return &cls.localServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS)
		{
			return &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_FAVOURITE_SERVERS)
		{
			return &cls.favoriteServers[n];
		}
		break;
	default:
		Com_Printf("LAN_GetServerPtr Warning: Invalid source\n");
		break;
	}
	return NULL;
}

/**
 * @brief LAN_CompareServers
 * @param[in] source
 * @param[in] sortKey
 * @param[in] sortDir
 * @param[in] s1
 * @param[in] s2
 * @return
 */
static int LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2)
{
	int          res;
	serverInfo_t *server1, *server2;
	char         name1[MAX_SERVER_NAME_LENGTH], name2[MAX_SERVER_NAME_LENGTH];

	server1 = LAN_GetServerPtr(source, s1);
	server2 = LAN_GetServerPtr(source, s2);
	if (!server1 || !server2)
	{
		return 0;
	}

	res = 0;
	switch (sortKey)
	{
	case SORT_HOST:
		Q_strncpyz(name1, server1->hostName, sizeof(name1));
		Q_TrimStr(Q_CleanStr(name1));
		Q_strncpyz(name2, server2->hostName, sizeof(name2));
		Q_TrimStr(Q_CleanStr(name2));
		res = Q_stricmp(name1, name2);
		break;

	case SORT_MAP:
		res = Q_stricmp(server1->mapName, server2->mapName);
		break;
	case SORT_CLIENTS:
		if (server1->humans > server2->humans)
		{
			res = -1;
		}
		else if (server1->humans < server2->humans)
		{
			res = 1;
		}
		else if (server1->clients > server2->clients)
		{
			res = -1;
		}
		else if (server1->clients < server2->clients)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	case SORT_GAME:
		if (server1->gameType < server2->gameType)
		{
			res = -1;
		}
		else if (server1->gameType > server2->gameType)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	case SORT_PING:
		if (server1->ping < server2->ping)
		{
			res = -1;
		}
		else if (server1->ping > server2->ping)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	}

	if (sortDir)
	{
		if (res < 0)
		{
			return 1;
		}
		if (res > 0)
		{
			return -1;
		}
		return 0;
	}
	return res;
}

/**
 * @brief LAN_GetPingQueueCount
 * @return
 */
static int LAN_GetPingQueueCount(void)
{
	return (CL_GetPingQueueCount());
}

/**
 * @brief LAN_ClearPing
 * @param[in] n
 */
static void LAN_ClearPing(int n)
{
	CL_ClearPing(n);
}

/**
 * @brief LAN_GetPing
 * @param[in] n
 * @param[out] buf
 * @param[in] buflen
 * @param[out] pingtime
 */
static void LAN_GetPing(int n, char *buf, size_t buflen, int *pingtime)
{
	CL_GetPing(n, buf, buflen, pingtime);
}

/**
 * @brief LAN_GetPingInfo
 * @param[in] n
 * @param[out] buf
 * @param[in] buflen
 */
static void LAN_GetPingInfo(int n, char *buf, size_t buflen)
{
	CL_GetPingInfo(n, buf, buflen);
}

/**
 * @brief LAN_MarkServerVisible
 * @param[in] source
 * @param[in] n
 * @param[in] visible
 */
static void LAN_MarkServerVisible(int source, int n, qboolean visible)
{
	if (n == -1)
	{
		int          count;
		serverInfo_t *server = NULL;

		switch (source)
		{
		case AS_LOCAL:
			count   = MAX_OTHER_SERVERS;
			server = &cls.localServers[0];
			break;
		case AS_GLOBAL:
			count  = MAX_GLOBAL_SERVERS;
			server = &cls.globalServers[0];
			break;
		case AS_FAVORITES:
			count   = MAX_FAVOURITE_SERVERS;
			server = &cls.favoriteServers[0];
			break;
		default:
			return;
		}
		if (server)
		{
			for (n = 0; n < count; n++)
			{
				server[n].visible = visible;
			}
		}
	}
	else
	{
		switch (source)
		{
		case AS_LOCAL:
			if (n >= 0 && n < MAX_OTHER_SERVERS)
			{
				cls.localServers[n].visible = visible;
			}
			break;
		case AS_GLOBAL:
			if (n >= 0 && n < MAX_GLOBAL_SERVERS)
			{
				cls.globalServers[n].visible = visible;
			}
			break;
		case AS_FAVORITES:
			if (n >= 0 && n < MAX_FAVOURITE_SERVERS)
			{
				cls.favoriteServers[n].visible = visible;
			}
			break;
		}
	}
}

/**
 * @brief LAN_ServerIsVisible
 * @param[in] source
 * @param[in] n
 * @return
 */
static int LAN_ServerIsVisible(int source, int n)
{
	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS)
		{
			return cls.localServers[n].visible;
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS)
		{
			return cls.globalServers[n].visible;
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_FAVOURITE_SERVERS)
		{
			return cls.favoriteServers[n].visible;
		}
		break;
	}
	return qfalse;
}

/**
 * @brief LAN_UpdateVisiblePings
 * @param[in] source
 * @return
 */
qboolean LAN_UpdateVisiblePings(int source)
{
	return CL_UpdateVisiblePings_f(source);
}

/**
 * @brief LAN_GetServerStatus
 * @param[in] serverAddress
 * @param[out] serverStatus
 * @param[in] maxLen
 * @return
 */
int LAN_GetServerStatus(const char *serverAddress, char *serverStatus, size_t maxLen)
{
	return CL_ServerStatus(serverAddress, serverStatus, maxLen);
}

/**
 * @brief LAN_ServerIsInFavoriteList
 * @param[in] source
 * @param[in] n
 * @return
 */
qboolean LAN_ServerIsInFavoriteList(int source, int n)
{
	int          i;
	serverInfo_t *server = NULL;

	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS)
		{
			server = &cls.localServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS)
		{
			server = &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_FAVOURITE_SERVERS)
		{
			return qtrue;
		}
		break;
	}

	if (!server)
	{
		return qfalse;
	}

	for (i = 0; i < cls.numfavoriteservers; i++)
	{
		if (NET_CompareAdr(cls.favoriteServers[i].adr, server->adr))
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief CL_GetGlconfig
 * @param[out] config
 */
static void CL_GetGlconfig(glconfig_t *config)
{
	*config = cls.glconfig;
}

/**
 * @brief GetClipboardData
 * @param[out] buf
 * @param[in] buflen
 */
static void GetClipboardData(char *buf, unsigned int buflen)
{
	char *cbd;

	cbd = IN_GetClipboardData();

	if (!cbd)
	{
		*buf = 0;
		return;
	}

	Q_strncpyz(buf, cbd, buflen);

	Z_Free(cbd);
}

/**
 * @brief Key_KeynumToStringBuf
 * @param[in] keynum
 * @param[out] buf
 * @param[in] buflen
 */
void Key_KeynumToStringBuf(int keynum, char *buf, size_t buflen)
{
	Q_strncpyz(buf, CL_TranslateStringBuf(Key_KeynumToString(keynum)), buflen);
}

/**
 * @brief Key_GetBindingBuf
 * @param[in] keynum
 * @param[out] buf
 * @param[in] buflen
 */
void Key_GetBindingBuf(int keynum, char *buf, size_t buflen)
{
	char *value;

	value = Key_GetBinding(keynum);
	if (value)
	{
		Q_strncpyz(buf, value, buflen);
	}
	else
	{
		*buf = 0;
	}
}

/**
 * @brief Key_GetCatcher
 * @return
 */
int Key_GetCatcher(void)
{
	return cls.keyCatchers;
}

/**
 * @brief Key_SetCatcher
 * @param[in] catcher
 */
void Key_SetCatcher(int catcher)
{
	// console overrides everything
	if (cls.keyCatchers & KEYCATCH_CONSOLE)
	{
		cls.keyCatchers = catcher | KEYCATCH_CONSOLE;
	}
	else
	{
		cls.keyCatchers = catcher;
	}
}

/**
 * @brief GetConfigString
 * @param[in] index
 * @param[out] buf
 * @param[in] size
 * @return
 */
static int GetConfigString(int index, char *buf, size_t size)
{
	int offset;

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		Com_Printf("Config string [%i] index out of range\n", index);
		return qfalse;
	}

	offset = cl.gameState.stringOffsets[index];
	if (!offset)
	{
		if (size)
		{
			buf[0] = 0;
		}

		// this happens when ui calls for player configstrings which might be empty
		//Com_Printf("Config string [%i] zero offset\n", index);
		return qfalse;
	}

	Q_strncpyz(buf, cl.gameState.stringData + offset, size);

	return qtrue;
}

/**
 * @brief FloatAsInt
 * @param[in] f
 * @return
 */
static int FloatAsInt(float f)
{
	floatint_t fi;

	fi.f = f;
	return fi.i;
}

/**
 * @brief Get engine value
 * @param[out] value buffer
 * @param[in] valueSize buffer size
 * @param[in] key to query
 * @return true if value for key is found
 */
static qboolean UI_GetValue(char *value, int valueSize, const char *key)
{
	return qfalse;
}

/**
 * @brief The ui module is making a system call
 * @param[in] args
 * @return
 */
intptr_t CL_UISystemCalls(intptr_t *args)
{
	switch (args[0])
	{
	case UI_ERROR:
		Com_Error(ERR_DROP, "%s", (char *)VMA(1));
	case UI_PRINT:
		Com_Printf("%s", (char *)VMA(1));
		return 0;
	case UI_MILLISECONDS:
		return Sys_Milliseconds();
	case UI_CVAR_REGISTER:
		Cvar_Register(VMA(1), VMA(2), VMA(3), args[4]);
		return 0;
	case UI_CVAR_UPDATE:
		Cvar_Update(VMA(1));
		return 0;
	case UI_CVAR_SET:
		Cvar_SetSafe(VMA(1), VMA(2));
		return 0;
	case UI_CVAR_VARIABLEVALUE:
		return FloatAsInt(Cvar_VariableValue(VMA(1)));
	case UI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
		return 0;
	case UI_CVAR_LATCHEDVARIABLESTRINGBUFFER:
		Cvar_LatchedVariableStringBuffer(VMA(1), VMA(2), args[3]);
		return 0;
	case UI_CVAR_SETVALUE:
		Cvar_SetValueSafe(VMA(1), VMF(2));
		return 0;
	case UI_CVAR_RESET:
		Cvar_Reset(VMA(1));
		return 0;
	case UI_CVAR_CREATE:
		Cvar_Register(NULL, VMA(1), VMA(2), args[3]);
		return 0;
	case UI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer(args[1], VMA(2), args[3]);
		return 0;
	case UI_ARGC:
		return Cmd_Argc();
	case UI_ARGV:
		Cmd_ArgvBuffer(args[1], VMA(2), args[3]);
		return 0;
	case UI_CMD_EXECUTETEXT:
		if (args[1] == EXEC_NOW
		    && (!strncmp(VMA(2), "snd_restart", 11)
		        || !strncmp(VMA(2), "vid_restart", 11)
		        || !strncmp(VMA(2), "quit", 5)))
		{
			Com_Printf(S_COLOR_YELLOW "turning EXEC_NOW '%.11s' into EXEC_INSERT\n", (const char *)VMA(2));
			args[1] = EXEC_INSERT;
		}
		Cbuf_ExecuteText(args[1], VMA(2));
		return 0;
	case UI_ADDCOMMAND:
		Cmd_AddCommand(VMA(1));
		return 0;
	case UI_FS_FOPENFILE:
		return FS_FOpenFileByMode(VMA(1), VMA(2), (fsMode_t)args[3]);
	case UI_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;
	case UI_FS_WRITE:
		FS_Write(VMA(1), args[2], args[3]);
		return 0;
	case UI_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;
	case UI_FS_DELETEFILE:
		return FS_Delete(VMA(1));
	case UI_FS_GETFILELIST:
		return FS_GetFileList(VMA(1), VMA(2), VMA(3), args[4]);
	case UI_R_REGISTERMODEL:
		return re.RegisterModel(VMA(1));
	case UI_R_REGISTERSKIN:
		return re.RegisterSkin(VMA(1));
	case UI_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip(VMA(1));
	case UI_R_CLEARSCENE:
		re.ClearScene();
		return 0;
	case UI_R_ADDREFENTITYTOSCENE:
		re.AddRefEntityToScene(VMA(1));
		return 0;
	case UI_R_ADDPOLYTOSCENE:
		re.AddPolyToScene(args[1], args[2], VMA(3));
		return 0;
	case UI_R_ADDPOLYSTOSCENE:
		re.AddPolysToScene(args[1], args[2], VMA(3), args[4]);
		return 0;
	case UI_R_ADDLIGHTTOSCENE:
		// new dlight code
		re.AddLightToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), args[7], args[8]);
		return 0;
	case UI_R_ADDCORONATOSCENE:
		re.AddCoronaToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], (qboolean)args[7]);
		return 0;
	case UI_R_RENDERSCENE:
		re.RenderScene(VMA(1));
		return 0;
	case UI_R_SETCOLOR:
		re.SetColor(VMA(1));
		return 0;
	case UI_R_DRAW2DPOLYS:
		re.Add2dPolys(VMA(1), args[2], args[3]);
		return 0;
	case UI_R_DRAWSTRETCHPIC:
		re.DrawStretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;
	case UI_R_DRAWROTATEDPIC:
		re.DrawRotatedPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMF(10));
		return 0;
	case UI_R_MODELBOUNDS:
		re.ModelBounds(args[1], VMA(2), VMA(3));
		return 0;
	case UI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;
	case UI_CM_LERPTAG:
		return re.LerpTag(VMA(1), VMA(2), VMA(3), args[4]);
	case UI_S_REGISTERSOUND:
		return S_RegisterSound(VMA(1), (qboolean)args[2]);
	case UI_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], args[3]);
		return 0;
	case UI_S_FADESTREAMINGSOUND:
		S_FadeStreamingSound(VMF(1), args[2], args[3]);
		return 0;
	case UI_S_FADEALLSOUNDS:
		S_FadeAllSounds(VMF(1), args[2], (qboolean)args[3]);
		return 0;
	case UI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], VMA(2), args[3]);
		return 0;
	case UI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], VMA(2), args[3]);
		return 0;
	case UI_KEY_SETBINDING:
		Key_SetBinding(args[1], VMA(2));
		return 0;
	case UI_KEY_BINDINGTOKEYS:
		Key_GetBindingByString(VMA(1), VMA(2), VMA(3));
		return 0;
	case UI_KEY_ISDOWN:
		return Key_IsDown(args[1]);
	case UI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();
	case UI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode((qboolean)args[1]);
		return 0;
	case UI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;
	case UI_KEY_GETCATCHER:
		return Key_GetCatcher();
	case UI_KEY_SETCATCHER:
		// Don't allow the ui module to close the console
		Key_SetCatcher(args[1] | (Key_GetCatcher() & KEYCATCH_CONSOLE));
		return 0;
	case UI_GETCLIPBOARDDATA:
		GetClipboardData(VMA(1), args[2]);
		return 0;
	case UI_GETCLIENTSTATE:
		GetClientState(VMA(1));
		return 0;
	case UI_GETGLCONFIG:
		CL_GetGlconfig(VMA(1));
		return 0;
	case UI_GETCONFIGSTRING:
		return GetConfigString(args[1], VMA(2), args[3]);
	case UI_LAN_LOADCACHEDSERVERS:
		LAN_LoadCachedServers();
		return 0;
	case UI_LAN_SAVECACHEDSERVERS:
		//LAN_SaveServersToFile(); // now done on add/remove fav server so we no longer save LAN favs on shutdown & restart
		return 0;
	case UI_LAN_ADDSERVER:
		return LAN_AddServer(args[1], VMA(2), VMA(3));
	case UI_LAN_REMOVESERVER:
		LAN_RemoveServer(args[1], VMA(2));
		return 0;
	case UI_LAN_GETPINGQUEUECOUNT:
		return LAN_GetPingQueueCount();
	case UI_LAN_CLEARPING:
		LAN_ClearPing(args[1]);
		return 0;
	case UI_LAN_GETPING:
		LAN_GetPing(args[1], VMA(2), args[3], VMA(4));
		return 0;
	case UI_LAN_GETPINGINFO:
		LAN_GetPingInfo(args[1], VMA(2), args[3]);
		return 0;
	case UI_LAN_GETSERVERCOUNT:
		return LAN_GetServerCount(args[1]);
	case UI_LAN_GETSERVERADDRESSSTRING:
		LAN_GetServerAddressString(args[1], args[2], VMA(3), args[4]);
		return 0;
	case UI_LAN_GETSERVERINFO:
		LAN_GetServerInfo(args[1], args[2], VMA(3), args[4]);
		return 0;
	case UI_LAN_GETSERVERPING:
		return LAN_GetServerPing(args[1], args[2]);
	case UI_LAN_MARKSERVERVISIBLE:
		LAN_MarkServerVisible(args[1], args[2], (qboolean)args[3]);
		return 0;
	case UI_LAN_SERVERISVISIBLE:
		return LAN_ServerIsVisible(args[1], args[2]);
	case UI_LAN_UPDATEVISIBLEPINGS:
		return LAN_UpdateVisiblePings(args[1]);
	case UI_LAN_RESETPINGS:
		LAN_ResetPings(args[1]);
		return 0;
	case UI_LAN_SERVERSTATUS:
		return LAN_GetServerStatus(VMA(1), VMA(2), args[3]);
	case UI_LAN_SERVERISINFAVORITELIST:
		return LAN_ServerIsInFavoriteList(args[1], args[2]);
	case UI_LAN_COMPARESERVERS:
		return LAN_CompareServers(args[1], args[2], args[3], args[4], args[5]);
	case UI_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();
	case UI_R_REGISTERFONT:
		re.RegisterFont(VMA(1), args[2], VMA(3), (args[4] == qtrue));
		return 0;
	case UI_MEMSET:
		return (intptr_t)Com_Memset(VMA(1), args[2], args[3]);
	case UI_MEMCPY:
		return (intptr_t)Com_Memcpy(VMA(1), VMA(2), args[3]);
	case UI_STRNCPY:
		return (intptr_t)strncpy(VMA(1), VMA(2), args[3]);
	case UI_SIN:
		return FloatAsInt(sin(VMF(1)));
	case UI_COS:
		return FloatAsInt(cos(VMF(1)));
	case UI_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));
	case UI_SQRT:
		return FloatAsInt(sqrt(VMF(1)));
	case UI_FLOOR:
		return FloatAsInt(floor(VMF(1)));
	case UI_CEIL:
		return FloatAsInt(ceil(VMF(1)));
	case UI_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine(VMA(1));
	case UI_PC_REMOVE_ALL_GLOBAL_DEFINES:
		botlib_export->PC_RemoveAllGlobalDefines();
		return 0;
	case UI_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle(VMA(1));
	case UI_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle(args[1]);
	case UI_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
	case UI_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine(args[1], VMA(2), VMA(3));
	case UI_PC_UNREAD_TOKEN:
		botlib_export->PC_UnreadLastTokenHandle(args[1]);
		return 0;
	case UI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;
	case UI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack(VMA(1), VMA(2), args[3]); // added fadeup time
		return 0;
	case UI_REAL_TIME:
		return Com_RealTime(VMA(1));
	case UI_CIN_PLAYCINEMATIC:
		Com_DPrintf("UI_CIN_PlayCinematic\n");
		return CIN_PlayCinematic(VMA(1), args[2], args[3], args[4], args[5], args[6]);
	case UI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);
	case UI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);
	case UI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;
	case UI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;
	case UI_R_REMAP_SHADER:
		re.RemapShader(VMA(1), VMA(2), VMA(3));
		return 0;
	case UI_CL_GETLIMBOSTRING:
		return CL_GetLimboString(args[1], VMA(2));
	case UI_CL_TRANSLATE_STRING:
		CL_TranslateStringMod(VMA(1), VMA(2));
		return 0;
	case UI_CHECKAUTOUPDATE:
		CL_RequestMasterData(qfalse);
		return 0;
	case UI_GET_AUTOUPDATE:
		Com_GetAutoUpdate();
		return 0;
	case UI_OPENURL:
		CL_OpenURL((const char *)VMA(1));
		return 0;
	case UI_GETHUNKDATA:
		Com_GetHunkInfo(VMA(1), VMA(2));
		return 0;
	// obsolete
	case UI_SET_PBCLSTATUS:
	case UI_SET_PBSVSTATUS:
		return 0;
	case UI_TRAP_GETVALUE:
		return UI_GetValue(VMA(1), args[2], VMA(3));
	default:
		Com_Error(ERR_DROP, "Bad UI system trap: %ld", (long int) args[0]);
	}

	return 0; // never reached, Com_Error quits ...
}

/**
 * @brief CL_ShutdownUI
 */
void CL_ShutdownUI(void)
{
	cls.keyCatchers &= ~KEYCATCH_UI;
	cls.uiStarted    = qfalse;
	if (!uivm)
	{
		return;
	}
	VM_Call(uivm, UI_SHUTDOWN);
	VM_Free(uivm);
	uivm = NULL;
}

/**
 * @brief CL_InitUI
 */
void CL_InitUI(void)
{
	int v;

	uivm = VM_Create("ui", qtrue, CL_UISystemCalls, VMI_NATIVE);
	if (!uivm)
	{
		Com_Error(ERR_FATAL, "VM_Create on UI failed");
	}

	// sanity check
	v = VM_Call(uivm, UI_GETAPIVERSION);
	if (v != UI_API_VERSION)
	{
		Com_Error(ERR_FATAL, "User Interface is version %d, expected %d", v, UI_API_VERSION);
		//cls.uiStarted = qfalse; // FIXME: Never executed !
	}

	// init for this gamestate
	VM_Call(uivm, UI_INIT, (cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE), qtrue, ETLEGACY_VERSION_INT);
}

qboolean UI_checkKeyExec(int key)
{
	if (uivm)
	{
		return (qboolean)(VM_Call(uivm, UI_CHECKEXECKEY, key));
	}
	else
	{
		return qfalse;
	}
}

/**
 * @brief See if the current console command is claimed by the ui
 * @return
 */
qboolean UI_GameCommand(void)
{
	if (!uivm)
	{
		return qfalse;
	}

	return (qboolean)(VM_Call(uivm, UI_CONSOLE_COMMAND, cls.realtime));
}
