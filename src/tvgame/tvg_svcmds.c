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
 * @file tvg_svcmds.c
 * @brief Holds commands that can be executed by the server console,
 *        but not remote clients
 */

#include "tvg_local.h"

#ifdef FEATURE_LUA
#include "tvg_lua.h"
#endif

/*
==============================================================================
PACKET FILTERING

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and you can use '*' to match any value
so you can specify an entire class C network with "addip 192.246.40.*"

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.

TTimo NOTE: GUID functions are copied over from the model of IP banning,
used to enforce max lives independently from server reconnect and team changes (Xian)

TTimo NOTE: for persistence, bans are stored in tvg_banIPs cvar MAX_CVAR_VALUE_STRING
The size of the cvar string buffer is limiting the banning to around 20 masks
this could be improved by putting some tvg_banIPs2 tvg_banIps3 etc. maybe
still, you should rely on PB for banning instead
==============================================================================
*/

typedef struct ipGUID_s
{
	char compare[33];
} ipGUID_t;

#define MAX_IPFILTERS   1024

typedef struct ipFilterList_s
{
	ipFilter_t ipFilters[MAX_IPFILTERS];
	int numIPFilters;
	char cvarIPList[32];
} ipFilterList_t;

static ipFilterList_t ipFilters;

/**
 * @brief StringToFilter
 * @param[in,out] s
 * @param[out] f
 * @return
 */
static qboolean StringToFilter(const char *s, ipFilter_t *f)
{
	char num[128];
	int  i, j;
	byte b[4];
	byte m[4];

	for (i = 0 ; i < 4 ; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}

	for (i = 0 ; i < 4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			if (*s == '*')     // 'match any'
			{   // b[i] and m[i] to 0
				s++;
				if (!*s)
				{
					break;
				}
				s++;
				continue;
			}
			G_Printf("Bad filter address: %s\n", s);
			return qfalse;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i]   = Q_atoi(num);
		m[i]   = 255;

		if (!*s)
		{
			break;
		}
		s++;
	}

	f->mask    = *(unsigned *)m;
	f->compare = *(unsigned *)b;

	return qtrue;
}

/**
 * @brief UpdateIPBans
 * @param[in] ipFilterList
 */
static void UpdateIPBans(ipFilterList_t *ipFilterList)
{
	byte b[4] = { 0 };
	byte m[4] = { 0 };
	int  i, j;
	char iplist_final[MAX_CVAR_VALUE_STRING];
	char ip[64];

	*iplist_final = 0;
	for (i = 0 ; i < ipFilterList->numIPFilters ; i++)
	{
		if (ipFilterList->ipFilters[i].compare == 0xffffffff)
		{
			continue;
		}

		*(unsigned *)b = ipFilterList->ipFilters[i].compare;
		*(unsigned *)m = ipFilterList->ipFilters[i].mask;
		*ip            = 0;
		for (j = 0; j < 4 ; j++)
		{
			if (m[j] != 255)
			{
				Q_strcat(ip, sizeof(ip), "*");
			}
			else
			{
				Q_strcat(ip, sizeof(ip), va("%i", b[j]));
			}
			Q_strcat(ip, sizeof(ip), (j < 3) ? "." : " ");
		}
		if (strlen(iplist_final) + strlen(ip) < MAX_CVAR_VALUE_STRING)
		{
			Q_strcat(iplist_final, sizeof(iplist_final), ip);
		}
		else
		{
			Com_Printf("%s overflowed at MAX_CVAR_VALUE_STRING\n", ipFilterList->cvarIPList);
			break;
		}
	}

	trap_Cvar_Set(ipFilterList->cvarIPList, iplist_final);
}

/**
 * @brief G_FilterPacket
 * @param[in] ipFilterList
 * @param[in] from
 * @return
 */
qboolean G_FilterPacket(ipFilterList_t *ipFilterList, char *from)
{
	int      i = 0;
	unsigned in;
	byte     m[4] = { 0 };
	char     *p   = from;

	while (*p && i < 4)
	{
		m[i] = 0;
		while (*p >= '0' && *p <= '9')
		{
			m[i] = m[i] * 10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
		{
			break;
		}
		i++;
		p++;
	}

	in = *(unsigned *)m;

	for (i = 0; i < ipFilterList->numIPFilters; i++)
	{
		if ((in & ipFilterList->ipFilters[i].mask) == ipFilterList->ipFilters[i].compare)
		{
			return tvg_filterBan.integer != 0;
		}
	}

	return tvg_filterBan.integer == 0;
}

/**
 * @brief G_FilterIPBanPacket
 * @param[in] from
 * @return
 */
qboolean G_FilterIPBanPacket(char *from)
{
	return(G_FilterPacket(&ipFilters, from));
}

/**
 * @brief AddIP
 * @param[in] ipFilterList
 * @param[in] str
 */
void AddIP(ipFilterList_t *ipFilterList, const char *str)
{
	int i;

	for (i = 0; i < ipFilterList->numIPFilters; i++)
	{
		if (ipFilterList->ipFilters[i].compare == 0xffffffff)
		{
			break;      // free spot
		}
	}

	if (i == ipFilterList->numIPFilters)
	{
		if (ipFilterList->numIPFilters == MAX_IPFILTERS)
		{
			G_Printf("IP filter list is full\n");
			return;
		}
		ipFilterList->numIPFilters++;
	}

	if (!StringToFilter(str, &ipFilterList->ipFilters[i]))
	{
		ipFilterList->ipFilters[i].compare = 0xffffffffu;
	}

	UpdateIPBans(ipFilterList);
}

/**
 * @brief AddIPBan
 * @param[in] str
 */
void AddIPBan(const char *str)
{
	AddIP(&ipFilters, str);
}

/**
 * @brief G_ProcessIPBans
 */
void TVG_ProcessIPBans(void)
{
	char *s, *t;
	char str[MAX_CVAR_VALUE_STRING];

	ipFilters.numIPFilters = 0;
	Q_strncpyz(ipFilters.cvarIPList, "tvg_banIPs", sizeof(ipFilters.cvarIPList));

	Q_strncpyz(str, tvg_banIPs.string, sizeof(str));

	for (t = s = tvg_banIPs.string; *t; /* */)
	{
		s = strchr(s, ' ');
		if (!s)
		{
			break;
		}
		while (*s == ' ')
			*s++ = 0;
		if (*t)
		{
			AddIP(&ipFilters, t);
		}
		t = s;
	}
}

/**
 * @brief Svcmd_AddIP_f
 */
void Svcmd_AddIP_f(void)
{
	char str[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2)
	{
		G_Printf("Usage:  addip <ip-mask>\n");
		return;
	}

	trap_Argv(1, str, sizeof(str));

	AddIP(&ipFilters, str);
}

/**
 * @brief Svcmd_RemoveIP_f
 */
void Svcmd_RemoveIP_f(void)
{
	ipFilter_t f;
	int        i;
	char       str[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2)
	{
		G_Printf("Usage:  removeip <ip-mask>\n");
		return;
	}

	trap_Argv(1, str, sizeof(str));

	if (!StringToFilter(str, &f))
	{
		return;
	}

	for (i = 0 ; i < ipFilters.numIPFilters ; i++)
	{
		if (ipFilters.ipFilters[i].mask == f.mask   &&
		    ipFilters.ipFilters[i].compare == f.compare)
		{
			ipFilters.ipFilters[i].compare = 0xffffffffu;
			G_Printf("Removed\n");

			UpdateIPBans(&ipFilters);
			return;
		}
	}

	G_Printf("Didn't find %s\n", str);
}

/**
 * @brief Svcmd_ListIp_f
 */
void Svcmd_ListIp_f(void)
{
	trap_SendConsoleCommand(EXEC_INSERT, "tvg_banIPs\n");
}

/**
 * @var names of enum entityType_t for Svcmd_EntityList_f
 */
const char *enttypenames[] =
{
	"ET_GENERAL",
	"ET_PLAYER",
	"ET_ITEM",
	"ET_MISSILE",
	"ET_MOVER",
	"ET_BEAM",
	"ET_PORTAL",
	"ET_SPEAKER",
	"unused ent type",              // ET_PUSH_TRIGGER
	"ET_TELEPORT_TRIGGER",
	"ET_INVISIBLE",
	"unused ent type",              // ET_CONCUSSIVE_TRIGGER
	"ET_OID_TRIGGER",
	"ET_EXPLOSIVE_INDICATOR",

	"ET_EXPLOSIVE",
	"unused ent type",              // ET_EF_SPOTLIGHT
	"ET_ALARMBOX",
	"ET_CORONA",
	"ET_TRAP",

	"ET_GAMEMODEL",
	"unused ent type",              // ET_FOOTLOCKER

	"ET_FLAMEBARREL",
	"unused ent type",              // ET_FP_PARTS

	"unused ent type",              // ET_FIRE_COLUMN
	"unused ent type",              // ET_FIRE_COLUMN_SMOKE
	"ET_RAMJET",

	"ET_FLAMETHROWER_CHUNK",

	"unused ent type",              // ET_EXPLO_PART

	"ET_PROP",

	"unused ent type",              // ET_AI_EFFECT

	"ET_CAMERA",
	"unused ent type",              // ET_MOVERSCALED

	"ET_CONSTRUCTIBLE_INDICATOR",
	"ET_CONSTRUCTIBLE",
	"ET_CONSTRUCTIBLE_MARKER",
	"unused ent type",              // ET_BOMB
	"unused ent type",              // ET_WAYPOINT
	"ET_BEAM_2",
	"ET_TANK_INDICATOR",
	"ET_TANK_INDICATOR_DEAD",

	"unused ent type",              // ET_BOTGOAL_INDICATOR
	"ET_CORPSE",
	"ET_SMOKER",

	"ET_TEMPHEAD",
	"ET_MG42_BARREL",
	"ET_TEMPLEGS",
	"ET_TRIGGER_MULTIPLE",
	"ET_TRIGGER_FLAGONLY",
	"ET_TRIGGER_FLAGONLY_MULTIPLE",
	"ET_GAMEMANAGER",
	"ET_AAGUN",
	"ET_CABINET_H",
	"ET_CABINET_A",
	"ET_HEALER",
	"ET_SUPPLIER",

	"unused ent type",             // ET_LANDMINE_HINT
	"unused ent type",             // ET_ATTRACTOR_HINT
	"unused ent type",             // ET_SNIPER_HINT
	"unused ent type",             // ET_LANDMINESPOT_HINT

	"ET_COMMANDMAP_MARKER",

	"ET_WOLF_OBJECTIVE",

	"ET_AIRSTRIKE_PLANE",

	"ET_EVENTS"
};

/**
 * @brief prints a list of used - or when any param is added for all entities with following info
 *        - entnum (color red -> neverFree)
 *        - entity type OR event OR freed
 *        - classname
 *        - neverFree
 */
void Svcmd_EntityList_f(void)
{
	int       e, entsFree = 0;
	gentity_t *check = g_entities;
	char      line[128];

	G_Printf("^7 No.: ^3Type^7/^2Event^7/(freed)          ^7Classname                 ^1Target                        ^2Targetname                    ^2TNH\n");

	for (e = 0; e < MAX_GENTITIES ; e++, check++)
	{
		if (!check->inuse)
		{
			if (trap_Argc() > 1)
			{
				G_Printf("^2%4i:^7 %s %s\n", e, check->classname, check->targetname);
			}
			entsFree++;
			continue;
		}

		Com_Memset(line, 0, sizeof(line));

		// print the ents which are in use
		//Q_strcat(line, sizeof(line), va("^7%4i: ", e));

		Com_sprintf(line, 128, "^7%4i: ", e);

		if (check->s.eType <= ET_EVENTS) // print events
		{
			Q_strcat(line, sizeof(line), va("^3%-27s^7", enttypenames[check->s.eType]));
		}

		if (check->classname)
		{
			G_Printf("%s %-25s ^1%-29s ^2%-29s^7 %i\n", line, check->classname, check->target, check->targetname, check->targetnamehash);
		}
		else
		{
			G_Printf("%s *unknown classname* %s\n", line, check->targetname);
		}
	}
	G_Printf("^2%4i: num_entities - %4i: entities not in use\n", level.num_entities, entsFree);
}

/**
 * @brief ClientForString
 * @param[in] s
 * @return
 *
 * @note If a player is called '3' and there are only 2 players
 * on the server (clientnum 0 and 1)
 * this function will say 'client 3 is not connected'
 * solution: first check for usernames, if none is found, check for slotnumbers
 */
gclient_t *ClientForString(const char *s)
{
	gclient_t *cl;
	int       i;

	// check for a name match
	for (i = 0 ; i < level.maxclients ; i++)
	{
		cl = &level.clients[i];
		if (cl->pers.connected == CON_DISCONNECTED)
		{
			continue;
		}
		if (!Q_stricmp(cl->pers.netname, s))
		{
			return cl;
		}
	}

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		int idnum = Q_atoi(s);

		if (idnum < 0 || idnum >= level.maxclients)
		{
			Com_Printf("Bad client slot: %i\n", idnum);
			return NULL;
		}

		cl = &level.clients[idnum];
		if (cl->pers.connected == CON_DISCONNECTED)
		{
			G_Printf("Client %i is not connected\n", idnum);
			return NULL;
		}
		return cl;
	}

	G_Printf("User %s is not on the server\n", s);

	return NULL;
}

/**
 * @brief G_Is_SV_Running
 * @return
 */
static qboolean G_Is_SV_Running(void)
{
	char cvar[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("sv_running", cvar, sizeof(cvar));
	return (qboolean)(atoi(cvar));
}

/**
 * @brief G_GetPlayerByNum
 * @param[in] clientNum
 * @return
 */
gclient_t *G_GetPlayerByNum(int clientNum)
{
	gclient_t *cl;

	// make sure server is running
	if (!G_Is_SV_Running())
	{
		return NULL;
	}

	if (trap_Argc() < 2)
	{
		G_Printf("No player specified\n");
		return NULL;
	}

	if (clientNum < 0 || clientNum >= level.maxclients)
	{
		Com_Printf("Bad client slot: %i\n", clientNum);
		return NULL;
	}

	cl = &level.clients[clientNum];

	if (cl->pers.connected == CON_DISCONNECTED)
	{
		G_Printf("Client %i is not connected\n", clientNum);
		return NULL;
	}

	return cl;
}

/**
 * @brief G_GetPlayerByName
 * @param[in] name
 * @return
 */
gclient_t *G_GetPlayerByName(const char *name)
{
	int       i;
	gclient_t *cl;
	char      cleanName[64];

	// make sure server is running
	if (!G_Is_SV_Running())
	{
		return NULL;
	}

	if (trap_Argc() < 2)
	{
		G_Printf("No player specified\n");
		return NULL;
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = &level.clients[level.sortedClients[i]];

		if (!Q_stricmp(cl->pers.netname, name))
		{
			return cl;
		}

		Q_strncpyz(cleanName, cl->pers.netname, sizeof(cleanName));
		Q_CleanStr(cleanName);
		if (!Q_stricmp(cleanName, name))
		{
			return cl;
		}
	}

	G_Printf("Player %s is not on the server\n", name);

	return NULL;
}

///> change into qfalse if you want to use the qagame banning system
///> which makes it possible to unban IP addresses
#define USE_ENGINE_BANLIST qtrue

/**
 * @brief kick a user off of the server
 */
static void Svcmd_Kick_f(void)
{
	gclient_t *cl;
	int       timeout;
	char      sTimeout[MAX_TOKEN_CHARS];
	char      name[MAX_NAME_LENGTH];
	int       cnum;

	// make sure server is running
	if (!G_Is_SV_Running())
	{
		G_Printf("Server is not running.\n");
		return;
	}

	if (trap_Argc() < 2 || trap_Argc() > 3)
	{
		G_Printf("Usage: kick <player name> [timeout]\n");
		return;
	}

	trap_Argv(1, name, sizeof(name));

	cnum = TVG_ClientNumberFromString(NULL, name);

	cl = (cnum == -1 ? NULL : &level.clients[cnum]);

	if (trap_Argc() == 3)
	{
		trap_Argv(2, sTimeout, sizeof(sTimeout));
		timeout = Q_atoi(sTimeout);
	}
	else
	{
		timeout = 300;
	}

	if (!cl)
	{
		if (!Q_stricmp(name, "all"))
		{
			int i;

			for (i = 0, cl = level.clients; i < level.numConnectedClients; i++, cl++)
			{
				// dont kick localclients ...
				if (cl->pers.localClient)
				{
					continue;
				}

				if (timeout != -1)
				{
					// use engine banning system, mods may choose to use their own banlist
					if (USE_ENGINE_BANLIST)
					{

						// kick but dont ban bots, they arent that lame
						if ((g_entities[cl - level.clients].r.svFlags & SVF_BOT))
						{
							timeout = 0;
						}

						trap_DropClient(cl - level.clients, "player kicked", timeout);
					}
					else
					{
						trap_DropClient(cl - level.clients, "player kicked", 0);

						// kick but dont ban bots, they arent that lame
						if (!(g_entities[cl - level.clients].r.svFlags & SVF_BOT))
						{
							char *ip;
							char userinfo[MAX_INFO_STRING];

							trap_GetUserinfo(cl - level.clients, userinfo, sizeof(userinfo));
							ip = Info_ValueForKey(userinfo, "ip");
							AddIPBan(ip);
						}
					}
				}
				else
				{
					trap_DropClient(cl - level.clients, "player kicked", 0);
				}
			}
		}

		return;
	}
	else
	{
		// dont kick localclients ...
		if (cl->pers.localClient)
		{
			G_Printf("Cannot kick host player\n");
			return;
		}

		if (timeout != -1)
		{
			// use engine banning system, mods may choose to use their own banlist
			if (USE_ENGINE_BANLIST)
			{
				// kick but dont ban bots, they arent that lame
				if ((g_entities[cl - level.clients].r.svFlags & SVF_BOT))
				{
					timeout = 0;
				}
				trap_DropClient(cl - level.clients, "player kicked", timeout);
			}
			else
			{
				trap_DropClient(cl - level.clients, "player kicked", 0);

				// kick but dont ban bots, they arent that lame
				if (!(g_entities[cl - level.clients].r.svFlags & SVF_BOT))
				{
					char *ip;
					char userinfo[MAX_INFO_STRING];

					trap_GetUserinfo(cl - level.clients, userinfo, sizeof(userinfo));
					ip = Info_ValueForKey(userinfo, "ip");
					AddIPBan(ip);
				}
			}
		}
		else
		{
			trap_DropClient(cl - level.clients, "player kicked", 0);
		}
	}
}

/**
 * @brief Svcmd_CP_f
 */
void Svcmd_CP_f(void)
{
	trap_SendServerCommand(-1, va("cp \"%s\"", Q_AddCR(ConcatArgs(1))));
}

/**
 * @brief G_UpdateSvCvars
 */
void G_UpdateSvCvars(void)
{
	char cs[MAX_INFO_STRING];
	int  i;

	cs[0] = '\0';

	for (i = 0; i < level.svCvarsCount; i++)
	{
		if (level.svCvars[i].Val2[0] == 0) // don't send a space char when not set
		{
			Info_SetValueForKey(cs, va("V%i", i),
			                    va("%i %s %s", level.svCvars[i].mode, level.svCvars[i].cvarName, level.svCvars[i].Val1));
		}
		else
		{
			Info_SetValueForKey(cs, va("V%i", i),
			                    va("%i %s %s %s", level.svCvars[i].mode, level.svCvars[i].cvarName, level.svCvars[i].Val1, level.svCvars[i].Val2));
		}
	}

	Info_SetValueForKey(cs, "N", va("%i", level.svCvarsCount));

	// FIXME: print a warning when this configstring has nearly reached MAX_INFO_STRING size and don't set it if greater
	trap_SetConfigstring(CS_SVCVAR, cs);
}

/**
 * @brief CC_cvarempty
 */
void CC_cvarempty(void)
{
	Com_Memset(level.svCvars, 0, sizeof(level.svCvars));
	level.svCvarsCount = 0;
	G_UpdateSvCvars();
}

/**
 * @brief Forces client cvar to a specific value
 */
void CC_svcvar(void)
{
	char cvarName[MAX_CVAR_VALUE_STRING];
	char mode[16];
	char cvarValue1[MAX_CVAR_VALUE_STRING];
	char cvarValue2[MAX_CVAR_VALUE_STRING];
	int  i;
	int  index = level.svCvarsCount;
	char *p;

	if (trap_Argc() <= 3)
	{
		G_Printf("usage: sv_cvar <cvar name> <mode> <value1> <value2>\nexamples: sv_cvar cg_hitsounds EQ 1\n          sv_cvar cl_maxpackets IN 60 125\n");
		return;
	}
	trap_Argv(1, cvarName, sizeof(cvarName));
	trap_Argv(2, mode, sizeof(mode));
	trap_Argv(3, cvarValue1, sizeof(cvarValue1));

	for (p = cvarName; *p != '\0'; ++p)
	{
		*p = tolower(*p);
	}

	if (trap_Argc() == 5)
	{
		trap_Argv(4, cvarValue2, sizeof(cvarValue2));
	}
	else
	{
		cvarValue2[0] = '\0';
	}

	// is this cvar already in the array?.. (maybe they have a double entry)
	for (i = 0; i < level.svCvarsCount; i++)
	{
		if (!Q_stricmp(cvarName, level.svCvars[i].cvarName))
		{
			index = i;
		}
	}

	if (index >= MAX_SVCVARS)
	{
		G_Printf("sv_cvar: MAX_SVCVARS hit\n");
		return;
	}

	if (!Q_stricmp(mode, "EQ") || !Q_stricmp(mode, "EQUAL"))
	{
		level.svCvars[index].mode = SVC_EQUAL;
	}
	else if (!Q_stricmp(mode, "G") || !Q_stricmp(mode, "GREATER"))
	{
		level.svCvars[index].mode = SVC_GREATER;
	}
	else if (!Q_stricmp(mode, "GE") || !Q_stricmp(mode, "GREATEREQUAL"))
	{
		level.svCvars[index].mode = SVC_GREATEREQUAL;
	}
	else if (!Q_stricmp(mode, "L") || !Q_stricmp(mode, "LOWER"))
	{
		level.svCvars[index].mode = SVC_LOWER;
	}
	else if (!Q_stricmp(mode, "LE") || !Q_stricmp(mode, "LOWEREQUAL"))
	{
		level.svCvars[index].mode = SVC_LOWEREQUAL;
	}
	else if (!Q_stricmp(mode, "IN") || !Q_stricmp(mode, "INSIDE"))
	{
		level.svCvars[index].mode = SVC_INSIDE;
	}
	else if (!Q_stricmp(mode, "OUT") || !Q_stricmp(mode, "OUTSIDE"))
	{
		level.svCvars[index].mode = SVC_OUTSIDE;
	}
	else if (!Q_stricmp(mode, "INC") || !Q_stricmp(mode, "INCLUDE"))
	{
		level.svCvars[index].mode = SVC_INCLUDE;
	}
	else if (!Q_stricmp(mode, "EXC") || !Q_stricmp(mode, "EXCLUDE"))
	{
		level.svCvars[index].mode = SVC_EXCLUDE;
	}
	else if (!Q_stricmp(mode, "WB") || !Q_stricmp(mode, "WITHBITS"))
	{
		level.svCvars[index].mode = SVC_WITHBITS;
	}
	else if (!Q_stricmp(mode, "WOB") || !Q_stricmp(mode, "WITHOUTBITS"))
	{
		level.svCvars[index].mode = SVC_WITHOUTBITS;
	}
	else
	{
		G_Printf("sv_cvar: invalid mode\n");
		return;
	}

	if (trap_Argc() == 5)
	{
		Q_strncpyz(level.svCvars[index].Val2, cvarValue2, sizeof(level.svCvars[0].Val2));
	}
	else
	{
		Q_strncpyz(level.svCvars[index].Val2, "", sizeof(level.svCvars[0].Val2));
	}

	Q_strncpyz(level.svCvars[index].cvarName, cvarName, sizeof(level.svCvars[0].cvarName));
	Q_strncpyz(level.svCvars[index].Val1, cvarValue1, sizeof(level.svCvars[0].Val1));

	// cvar wasn't yet in the array?
	if (index >= level.svCvarsCount)
	{
		level.svCvarsCount++;
	}

	G_UpdateSvCvars();
}

char *ConcatArgs(int start);

/**
 * @brief Svcmd_CSInfo_f
 */
void Svcmd_CSInfo_f(void)
{
	int      i = 0, j;
	char     cs[BIG_INFO_STRING];
	char     cspart[MAX_TOKEN_CHARS];
	char     valuestr[MAX_TOKEN_CHARS];
	int      value       = -1;
	int      size        = 0;
	int      total       = 0;
	char     *str        = NULL;
	qboolean arg1        = (trap_Argc() > 1) ? qtrue : qfalse;
	qboolean arg1numeric = qtrue;

	valuestr[0] = 0;
	if (arg1)
	{
		trap_Argv(1, valuestr, sizeof(valuestr));
		for (i = 0; i < strlen(valuestr); i++)
		{
			if (valuestr[i] < '0' || valuestr[i] > '9')
			{
				arg1numeric = qfalse;
				break;
			}
		}
		if (arg1numeric)
		{
			value = Q_atoi(valuestr);
			if (value >= MAX_CONFIGSTRINGS)
			{
				value = -1;
			}
		}
	}
	else
	{
		G_Printf("Help:\n'csinfo <CS No.>' will print the content of given string\n'csinfo *' will print all strings & content.\n\n");
	}

	G_Printf("CS   Length   Type\n--------------------------------------------\n");
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		trap_GetConfigstring(i, cs, sizeof(cs));
		size   = (int) strlen(cs);
		total += size;
		if (size == 0)
		{
			continue;
		}

		switch (i)
		{
		case CS_SERVERINFO:
			str = "CS_SERVERINFO";
			break;
		case CS_SYSTEMINFO:
			str = "CS_SYSTEMINFO";
			break;
		case CS_MUSIC:
			str = "CS_MUSIC";
			break;
		case CS_MESSAGE:
			str = "CS_MESSAGE";
			break;
		case CS_MOTD:
			str = "CS_MOTD";
			break;
		case CS_MUSIC_QUEUE:
			str = "CS_MUSIC_QUEUE";
			break;
		case CS_WARMUP:
			str = "CS_WARMUP";
			break;
		case CS_VOTE_STRING:
			str = "CS_VOTE_STRING";
			break;
		case CS_VOTE_YES:
			str = "CS_VOTE_YES";
			break;
		case CS_VOTE_NO:
			str = "CS_VOTE_NO";
			break;
		case CS_GAME_VERSION:
			str = "CS_GAME_VERSION";
			break;
		case CS_LEVEL_START_TIME:
			str = "CS_LEVEL_START_TIME";
			break;
		case CS_INTERMISSION:
			str = "CS_INTERMISSION";
			break;
		case CS_MULTI_INFO:
			str = "CS_MULTI_INFO";
			break;
		case CS_MULTI_MAPWINNER:
			str = "CS_MULTI_MAPWINNER";
			break;
		case CS_MULTI_OBJECTIVE:
			str = "CS_MULTI_OBJECTIVE";
			break;
		case CS_SCREENFADE:
			str = "CS_SCREENFADE";
			break;
		case CS_FOGVARS:
			str = "CS_FOGVARS";
			break;
		case CS_SKYBOXORG:
			str = "CS_SKYBOXORG";
			break;
		case CS_TARGETEFFECT:
			str = "CS_TARGETEFFECT";
			break;
		case CS_WOLFINFO:
			str = "CS_WOLFINFO";
			break;
		case CS_FIRSTBLOOD:
			str = "CS_FIRSTBLOOD";
			break;
		case CS_ROUNDSCORES1:
			str = "CS_ROUNDSCORES1";
			break;
		case CS_ROUNDSCORES2:
			str = "CS_ROUNDSCORES2";
			break;
		case CS_MAIN_AXIS_OBJECTIVE:
			str = "CS_MAIN_AXIS_OBJECTIVE";
			break;
		case  CS_MAIN_ALLIES_OBJECTIVE:
			str = "CS_MAIN_ALLIES_OBJECTIVE";
			break;
		case CS_SCRIPT_MOVER_NAMES:
			str = "CS_SCRIPT_MOVER_NAMES";
			break;
		case CS_CONSTRUCTION_NAMES:
			str = "CS_CONSTRUCTION_NAMES";
			break;
		case CS_VERSIONINFO:
			str = "CS_VERSIONINFO";
			break;
		case CS_REINFSEEDS:
			str = "CS_REINFSEEDS";
			break;
		case CS_SERVERTOGGLES:
			str = "CS_SERVERTOGGLES";
			break;
		case CS_GLOBALFOGVARS:
			str = "CS_GLOBALFOGVARS";
			break;
		case CS_AXIS_MAPS_XP:
			str = "CS_AXIS_MAPS_XP";
			break;
		case CS_ALLIED_MAPS_XP:
			str = "CS_ALLIED_MAPS_XP";
			break;
		case CS_INTERMISSION_START_TIME:
			str = "CS_INTERMISSION_START_TIME";
			break;
		case CS_ENDGAME_STATS:
			str = "CS_ENDGAME_STATS";
			break;
		case CS_CHARGETIMES:
			str = "CS_CHARGETIMES";
			break;
		case CS_FILTERCAMS:
			str = "CS_FILTERCAMS";
			break;
		case CS_MODINFO:
			str = "CS_MODINFO";
			break;
		case CS_TEAMRESTRICTIONS:
			str = "CS_TEAMRESTRICTIONS";
			break;
		case CS_SVCVAR:
			str = "CS_SVCVAR";
			break;
		case CS_CONFIGNAME:
			str = "CS_CONFIGNAME";
			break;
		case CS_UPGRADERANGE:
			str = "CS_UPGRADERANGE";
			break;
		case CS_SHADERSTATE:
			str = "CS_SHADERSTATE";
			break;
		default:
			if (i >= CS_MODELS && i < CS_MODELS + MAX_MODELS)
			{
				str = "CS_MODELS";
				break;
			}
			else if (i >= CS_SOUNDS && i < CS_SOUNDS + MAX_SOUNDS)
			{
				str = "CS_SOUNDS";
				break;
			}
			else if (i >= CS_SHADERS && i < CS_SHADERS + MAX_CS_SHADERS)
			{
				str = "CS_SHADERS";
				break;
			}
			else if (i >= CS_SKINS && i < CS_SKINS + MAX_CS_SKINS)
			{
				str = "CS_SKINS";
				break;
			}
			else if (i >= CS_CHARACTERS && i < CS_CHARACTERS + MAX_CHARACTERS)
			{
				str = "CS_CHARACTERS";
				break;
			}
			else if (i >= CS_PLAYERS && i < CS_PLAYERS + MAX_CLIENTS)
			{
				str = "CS_PLAYERS";
				break;
			}
			else if (i >= CS_MULTI_SPAWNTARGETS && i < CS_MULTI_SPAWNTARGETS + MAX_MULTI_SPAWNTARGETS)
			{
				str = "CS_MULTI_SPAWNTARGETS";
				break;
			}
			else if (i >= CS_OID_TRIGGERS && i < CS_OID_TRIGGERS + MAX_OID_TRIGGERS)
			{
				str = "CS_OID_TRIGGERS";
				break;
			}
			else if (i >= CS_OID_DATA && i < CS_OID_DATA + MAX_OID_TRIGGERS)
			{
				str = "CS_OID_DATA";
				break;
			}
			else if (i >= CS_DLIGHTS && i < CS_DLIGHTS + MAX_DLIGHT_CONFIGSTRINGS)
			{
				str = "CS_DLIGHTS";
				break;
			}
			else if (i >= CS_SPLINES && i < CS_SPLINES + MAX_SPLINE_CONFIGSTRINGS)
			{
				str = "CS_SERVERINFO";
				break;
			}
			else if (i >= CS_TAGCONNECTS && i < CS_TAGCONNECTS + MAX_TAGCONNECTS)
			{
				str = "CS_TAGCONNECTS";
				break;
			}
			else if (i >= CS_FIRETEAMS && i < CS_FIRETEAMS + MAX_FIRETEAMS)
			{
				str = "CS_FIRETEAMS";
				break;
			}
			else if (i >= CS_CUSTMOTD && i < CS_CUSTMOTD + MAX_MOTDLINES)
			{
				str = "CS_CUSTMOTD";
				break;
			}
			else if (i >= CS_STRINGS && i < CS_STRINGS + MAX_CSSTRINGS)
			{
				str = "CS_STRINGS";
				break;
			}
			else
			{
				str = "";
			}
			break;
		}

		if (arg1)
		{
			if (valuestr[0] == '*')
			{
				G_Printf("%-4i %-8i %-22s %s\n", i, size, str, cs); // note: this might not print the full content see CS 1 or 2
			}
			else if ((arg1numeric && value == i) || (!arg1numeric && !Q_stricmp(valuestr, str)))
			{
				G_Printf("%-4i %-8i %s\n", i, size, str);
				// value 239 is taken from SBP()
				for (j = 0; j <= (int)(size / (239 - 1)); j++)
				{
					Q_strncpyz(cspart, (char *)&cs[j * (239 - 1)], 239);
					G_Printf("%s", cspart);
				}
				G_Printf("\n");
			}
		}
		else
		{
			G_Printf("%-4i %-8i %s\n", i, size, str);
		}
	}
	G_Printf("--------------------------------------------\nTotal CONFIGSTRING Length: %i\n", total);
}

/**
 * @brief Svcmd_Ref_f
 */
void Svcmd_Ref_f(void)
{
	if (level.fLocalHost)
	{
		return;
	}

	TVG_ref_cmd(NULL, NULL);
}

/**
 * @brief Svcmd_Say_f
 */
qboolean Svcmd_Say_f(void)
{
	if (tvg_dedicated.integer)
	{
		trap_SendServerCommand(-1, va("cpm \"server: %s\n\"", Q_AddCR(ConcatArgs(1))));
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief Svcmd_Chat_f
 */
qboolean Svcmd_Chat_f(void)
{
	if (tvg_dedicated.integer)
	{
		// added for rcon/Lua chat
		trap_SendServerCommand(-1, va("chat \"console: %s\"", Q_AddCR(ConcatArgs(1))));
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief Svcmd_Qsay_f
 */
void Svcmd_Qsay_f(void)
{
	trap_SendServerCommand(-1, va("chat \"%s\"", Q_AddCR(ConcatArgs(1))));
}

extern void Svcmd_GameMem_f(void);

/**
 * @var consoleCommandTable
 * @brief Store common console command
 */
static consoleCommandTable_t consoleCommandTable[] =
{
	{ "entitylist",    Svcmd_EntityList_f },
	{ "csinfo",        Svcmd_CSInfo_f     },
	{ "game_memory",   Svcmd_GameMem_f    },
	{ "addip",         Svcmd_AddIP_f      },
	{ "removeip",      Svcmd_RemoveIP_f   },
	{ "listip",        Svcmd_ListIp_f     },

	{ "makeReferee",   TVG_MakeReferee    },
	{ "removeReferee", TVG_RemoveReferee  },
	{ "mute",          TVG_MuteClient     },
	{ "unmute",        TVG_UnMuteClient   },
	{ "ban",           TVG_PlayerBan      },
	{ "kick",          Svcmd_Kick_f       },                            // moved from engine
	{ "clientkick",    Svcmd_Kick_f       },                            // both similar to keep compatibility

	{ "cp",            Svcmd_CP_f         },
	{ "sv_cvarempty",  CC_cvarempty       },
	{ "sv_cvar",       CC_svcvar          },
	{ "playsound",     TVG_PlaySound_Cmd  },
	{ "playsound_env", TVG_PlaySound_Cmd  },

	{ "ref",           Svcmd_Ref_f        },                            // console also gets ref commands
	{ "qsay",          Svcmd_Qsay_f       },
};

/**
 * @brief TVG_ConsoleCommand
 * @return
 */
qboolean TVG_ConsoleCommand(void)
{
	char         cmd[MAX_TOKEN_CHARS];
	unsigned int i;

	trap_Argv(0, cmd, sizeof(cmd));

#ifdef FEATURE_LUA
	if (!Q_stricmp(cmd, "lua_status"))
	{
		TVG_LuaStatus(NULL);
		return qtrue;
	}
	else if (!Q_stricmp(cmd, "lua_restart"))
	{
		TVG_LuaRestart();
		return qtrue;
	}
	else if (Q_stricmp(cmd, "lua_api") == 0)
	{
		TVG_LuaStackDump();
		return qtrue;
	}
	// *LUA* API callbacks
	else if (TVG_LuaHook_ConsoleCommand(cmd))
	{
		return qtrue;
	}
	else
#endif
	// special cases for chat
	if (Q_stricmp(cmd, "say") == 0)
	{
		return Svcmd_Say_f();
	}
	else if (Q_stricmp(cmd, "chat") == 0)
	{
		return Svcmd_Chat_f();
	}

	for (i = 0; i < sizeof(consoleCommandTable) / sizeof(consoleCommandTable_t); i++)
	{
		if (!Q_stricmp(cmd, consoleCommandTable[i].name))
		{
			consoleCommandTable[i].cmd();
			return qtrue;
		}
	}

	return qfalse;
}
