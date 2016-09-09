/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2016 ET:Legacy team <mail@etlegacy.com>
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
 * @file g_svcmds.c
 * @brief Holds commands that can be executed by the server console,
 *        but not remote clients
 */

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

#ifdef FEATURE_LUA
#include "g_lua.h"
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

TTimo NOTE: for persistence, bans are stored in g_banIPs cvar MAX_CVAR_VALUE_STRING
The size of the cvar string buffer is limiting the banning to around 20 masks
this could be improved by putting some g_banIPs2 g_banIps3 etc. maybe
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
static ipFilterList_t ipMaxLivesFilters;

static ipGUID_t guidMaxLivesFilters[MAX_IPFILTERS];
static int      numMaxLivesFilters = 0;

qboolean StringToFilter(const char *s, ipFilter_t *f)
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
		b[i]   = atoi(num);
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

static void UpdateIPBans(ipFilterList_t *ipFilterList)
{
	byte b[4];
	byte m[4];
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

void PrintMaxLivesGUID(void)
{
	int i;

	for (i = 0 ; i < numMaxLivesFilters ; i++)
	{
		G_LogPrintf("%i. %s\n", i, guidMaxLivesFilters[i].compare);
	}
	G_LogPrintf("--- End of list\n");
}

qboolean G_FilterPacket(ipFilterList_t *ipFilterList, char *from)
{
	int      i = 0;
	unsigned in;
	byte     m[4];
	char     *p = from;

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
		i++, p++;
	}

	in = *(unsigned *)m;

	for (i = 0; i < ipFilterList->numIPFilters; i++)
	{
		if ((in & ipFilterList->ipFilters[i].mask) == ipFilterList->ipFilters[i].compare)
		{
			return g_filterBan.integer != 0;
		}
	}

	return g_filterBan.integer == 0;
}

qboolean G_FilterIPBanPacket(char *from)
{
	return(G_FilterPacket(&ipFilters, from));
}

qboolean G_FilterMaxLivesIPPacket(char *from)
{
	return(G_FilterPacket(&ipMaxLivesFilters, from));
}

/**
 * @brief Check to see if the user is trying to sneak back in with g_enforcemaxlives enabled
 */
qboolean G_FilterMaxLivesPacket(char *from)
{
	int i;

	for (i = 0; i < numMaxLivesFilters; i++)
	{
		if (!Q_stricmp(guidMaxLivesFilters[i].compare, from))
		{
			return 1;
		}
	}
	return 0;
}

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

void AddIPBan(const char *str)
{
	AddIP(&ipFilters, str);
}

void AddMaxLivesBan(const char *str)
{
	AddIP(&ipMaxLivesFilters, str);
}

/**
 * @brief with g_enforcemaxlives enabled, this adds a client GUID to a list
 * that prevents them from quitting and reconnecting
 */
void AddMaxLivesGUID(char *str)
{
	if (numMaxLivesFilters == MAX_IPFILTERS)
	{
		G_Printf("MaxLives GUID filter list is full\n");
		return;
	}
	Q_strncpyz(guidMaxLivesFilters[numMaxLivesFilters].compare, str, 33);
	numMaxLivesFilters++;
}

void G_ProcessIPBans(void)
{
	char *s, *t;
	char str[MAX_CVAR_VALUE_STRING];

	ipFilters.numIPFilters = 0;
	Q_strncpyz(ipFilters.cvarIPList, "g_banIPs", sizeof(ipFilters.cvarIPList));

	Q_strncpyz(str, g_banIPs.string, sizeof(str));

	for (t = s = g_banIPs.string; *t; /* */)
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
			G_Printf("Removed.\n");

			UpdateIPBans(&ipFilters);
			return;
		}
	}

	G_Printf("Didn't find %s.\n", str);
}

/**
 * @brief Clears out the entire list maxlives enforcement banlist
 */
void ClearMaxLivesBans()
{
	int i;

	for (i = 0; i < numMaxLivesFilters; i++)
	{
		guidMaxLivesFilters[i].compare[0] = '\0';
	}
	numMaxLivesFilters = 0;

	ipMaxLivesFilters.numIPFilters = 0;
	Q_strncpyz(ipMaxLivesFilters.cvarIPList, "g_maxlivesbanIPs", sizeof(ipMaxLivesFilters.cvarIPList));
}

// names of enume entityType_t for Svcmd_EntityList_f
char *enttypenames[] =
{
	"ET_GENERAL",
	"ET_PLAYER",
	"ET_ITEM",
	"ET_MISSILE",
	"ET_MOVER",
	"ET_BEAM",
	"ET_PORTAL",
	"ET_SPEAKER",
	"ET_PUSH_TRIGGER",
	"ET_TELEPORT_TRIGGER",
	"ET_INVISIBLE",
	"ET_CONCUSSIVE_TRIGGER",
	"ET_OID_TRIGGER",
	"ET_EXPLOSIVE_INDICATOR",

	"ET_EXPLOSIVE",
	"ET_EF_SPOTLIGHT",             // unused
	"ET_ALARMBOX",
	"ET_CORONA",
	"ET_TRAP",

	"ET_GAMEMODEL",
	"ET_FOOTLOCKER",

	"ET_FLAMEBARREL",
	"ET_FP_PARTS",

	"ET_FIRE_COLUMN",
	"ET_FIRE_COLUMN_SMOKE",
	"ET_RAMJET",

	"ET_FLAMETHROWER_CHUNK",

	"ET_EXPLO_PART",

	"ET_PROP",

	"ET_AI_EFFECT",

	"ET_CAMERA",
	"ET_MOVERSCALED",

	"ET_CONSTRUCTIBLE_INDICATOR",
	"ET_CONSTRUCTIBLE",
	"ET_CONSTRUCTIBLE_MARKER",
	"ET_BOMB",
	"ET_WAYPOINT",
	"ET_BEAM_2",
	"ET_TANK_INDICATOR",
	"ET_TANK_INDICATOR_DEAD",

	"ET_BOTGOAL_INDICATOR",
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

	"ET_LANDMINE_HINT",
	"ET_ATTRACTOR_HINT",
	"ET_SNIPER_HINT",
	"ET_LANDMINESPOT_HINT",

	"ET_COMMANDMAP_MARKER",

	"ET_WOLF_OBJECTIVE",

	"ET_EVENTS"
};

/**
 * @brief prints a list of used - or when any param is added for all entities with following info
 *        - entnum
 *        - entity type OR event
 *        - classname
 *        - neverFree
 */
void Svcmd_EntityList_f(void)
{
	int       e, entsFree = 0;
	gentity_t *check = g_entities;
	char      line[128];

	for (e = 0; e < MAX_GENTITIES ; e++, check++)
	{
		if (!check->inuse)
		{
			if (trap_Argc() > 1)
			{
				G_Printf("^2%4i: %s %s\n", e, check->classname, check->targetname);
			}
			entsFree++;
			continue;
		}

		memset(line, 0, sizeof(line));

		// print the ents which are in use
		//Q_strcat(line, sizeof(line), va("^7%4i: ", e));

		Com_sprintf(line, 128, "^7%4i: ", e);

		if (check->s.eType <= ET_EVENTS) // print events
		{
			Q_strcat(line, sizeof(line), va("^3%-27s^7", enttypenames[check->s.eType]));
		}
		else
		{
			Q_strcat(line, sizeof(line), va("^2%-27s^7", eventnames[check->s.eType - ET_EVENTS]));
		}

		if (check->classname)
		{
			G_Printf("%s %-25s ^1%s^7\n", line, check->classname, check->targetname);
		}
		else
		{
			G_Printf("%s *unknown classname* %s\n", line, check->targetname);
		}
	}
	G_Printf("^2%4i: num_entities - %4i: entities not in use\n", level.num_entities, entsFree);
}

// note: if a player is called '3' and there are only 2 players
// on the server (clientnum 0 and 1)
// this function will say 'client 3 is not connected'
// solution: first check for usernames, if none is found, check for slotnumbers
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
		int idnum = atoi(s);

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

static qboolean G_Is_SV_Running(void)
{
	char cvar[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("sv_running", cvar, sizeof(cvar));
	return (qboolean)atoi(cvar);
}

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
		G_Printf("No player specified.\n");
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

gclient_t *G_GetPlayerByName(char *name)
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
		G_Printf("No player specified.\n");
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

/**
 * <code>forceteam \<player\> \<team\></code>
 */
void Svcmd_ForceTeam_f(void)
{
	gclient_t *cl;
	char      str[MAX_TOKEN_CHARS];

	// find the player
	trap_Argv(1, str, sizeof(str));
	cl = ClientForString(str);
	if (!cl)
	{
		return;
	}

	// set the team
	trap_Argv(2, str, sizeof(str));
	SetTeam(&g_entities[cl - level.clients], str, qtrue, cl->sess.playerWeapon, cl->sess.playerWeapon2, qtrue);
}

/**
 * @brief starts match if in tournament mode
 */
void Svcmd_StartMatch_f(void)
{
	/*  if ( !g_noTeamSwitching.integer ) {
	        trap_SendServerCommand( -1, va("print \"g_noTeamSwitching not activated.\n\""));
	        return;
	    }
	*/

	G_refAllReady_cmd(NULL);

	/*
	    if ( level.numPlayingClients <= 1 ) {
	        trap_SendServerCommand( -1, va("print \"Not enough playing clients to start match.\n\""));
	        return;
	    }

	    if ( g_gamestate.integer == GS_PLAYING ) {
	        trap_SendServerCommand( -1, va("print \"Match is already in progress.\n\""));
	        return;
	    }

	    if ( g_gamestate.integer == GS_WARMUP ) {
	        trap_SendConsoleCommand( EXEC_APPEND, va( "map_restart 0 %i\n", GS_PLAYING ) );
	    }
	*/
}

/**
 * @brief multiuse now for both map restarts and total match resets
 */
void Svcmd_ResetMatch_f(qboolean fDoReset, qboolean fDoRestart)
{
	int i;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		g_entities[level.sortedClients[i]].client->pers.ready = 0;
	}

	if (fDoReset)
	{
		G_resetRoundState();
		G_resetModeState();
	}

	if (fDoRestart)
	{
		trap_SendConsoleCommand(EXEC_APPEND, va("map_restart 0 %i\n", ((g_gamestate.integer != GS_PLAYING) ? GS_RESET : GS_WARMUP)));
	}
}

/**
 * @brief swaps all clients to opposite team
 */
void Svcmd_SwapTeams_f(void)
{
	G_resetRoundState();

	if ((g_gamestate.integer == GS_INITIALIZE) ||
	    (g_gamestate.integer == GS_WARMUP) ||
	    (g_gamestate.integer == GS_RESET))
	{
		G_swapTeams();
		return;
	}

	G_resetModeState();

	trap_Cvar_Set("g_swapteams", "1");
	Svcmd_ResetMatch_f(qfalse, qtrue);
}

/**
 * @brief randomly places players on teams
 * @param restart
 */
void Svcmd_ShuffleTeams_f(qboolean restart)
{
	if (restart)
	{
		G_resetRoundState();
	}

	G_shuffleTeams();

	if ((g_gamestate.integer == GS_INITIALIZE) ||
	    (g_gamestate.integer == GS_WARMUP) ||
	    (g_gamestate.integer == GS_RESET))
	{
		return;
	}
	if (restart)
	{
		G_resetModeState();
		Svcmd_ResetMatch_f(qfalse, qtrue);
	}
}

void Svcmd_Campaign_f(void)
{
	char             str[MAX_TOKEN_CHARS];
	int              i;
	g_campaignInfo_t *campaign = NULL;

	// find the campaign
	trap_Argv(1, str, sizeof(str));

	for (i = 0; i < level.campaignCount; i++)
	{
		campaign = &g_campaigns[i];

		if (!Q_stricmp(campaign->shortname, str))
		{
			break;
		}
	}

	if (i == level.campaignCount || !(campaign->typeBits & (1 << GT_WOLF)))
	{
		G_Printf("Can't find campaign '%s'\n", str);
		return;
	}

	trap_Cvar_Set("g_currentCampaign", campaign->shortname);
	trap_Cvar_Set("g_currentCampaignMap", "0");

	level.newCampaign = qtrue;

	// we got a campaign, start it
	trap_Cvar_Set("g_gametype", va("%i", GT_WOLF_CAMPAIGN));

	trap_SendConsoleCommand(EXEC_APPEND, va("map %s\n", campaign->mapnames[0]));
}

void Svcmd_ListCampaigns_f(void)
{
	int i, mpCampaigns = 0;

	for (i = 0; i < level.campaignCount; i++)
	{
		if (g_campaigns[i].typeBits & (1 << GT_WOLF))
		{
			mpCampaigns++;
		}
	}

	if (mpCampaigns)
	{
		G_Printf("%i campaigns found:\n", mpCampaigns);
	}
	else
	{
		G_Printf("No campaigns found.\n");
		return;
	}

	for (i = 0; i < level.campaignCount; i++)
	{
		if (g_campaigns[i].typeBits & (1 << GT_WOLF))
		{
			G_Printf(" %s\n", g_campaigns[i].shortname);
		}
	}
}

// modified from maddoc sp func
extern void ReviveEntity(gentity_t *ent, gentity_t *traceEnt);
extern int FindClientByName(char *name);

void Svcmd_RevivePlayer(char *name)
{
	int       clientNum;
	gentity_t *player;

	if (!g_cheats.integer)
	{
		G_Printf("Cheats are not enabled on this server.\n");
		return;
	}

	clientNum = FindClientByName(name);
	if (clientNum < 0)
	{
		return;
	}
	player = &g_entities[clientNum];

	ReviveEntity(player, player);
}

/**
 * @brief Gib command - based on shrubbot
 */
static void Svcmd_Gib(void)
{
	int       pids[MAX_CLIENTS];
	char      name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	gentity_t *vic;
	qboolean  doAll = qfalse;

	// ignore in intermission
	if (level.intermissiontime)
	{
		G_Printf("Gib command not allowed during intermission.\n");
		return;
	}

	if (trap_Argc() < 2)
	{
		doAll = qtrue;
	}

	trap_Argv(1, name, sizeof(name));

	if (!Q_stricmp(name, "-1") || doAll)
	{
		int it, count = 0;

		for (it = 0; it < level.numConnectedClients; it++)
		{
			vic = g_entities + level.sortedClients[it];
			if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
			      vic->client->sess.sessionTeam == TEAM_ALLIES))
			{
				continue;
			}
			G_Damage(vic, NULL, NULL, NULL, NULL, 500, 0, MOD_UNKNOWN);
			count++;
		}

		if (count > 0)
		{
			CPx(-1, va("cp \"^3%d^7 players gibbed.\"", count));
		}
		else
		{
			G_Printf("There is no player to gib.\n");
		}
		return;
	}

	if (ClientNumbersFromString(name, pids) != 1)
	{
		G_MatchOnePlayer(pids, err, sizeof(err));
		G_Printf("Error - can't gib - %s.", err);
		return;
	}
	vic = &g_entities[pids[0]];

	if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
	      vic->client->sess.sessionTeam == TEAM_ALLIES))
	{
		G_Printf("Player must be on a team to be gibbed.\n");
		return;
	}

	G_Damage(vic, NULL, NULL, NULL, NULL, 500, 0, MOD_UNKNOWN);

	CPx(-1, va("cp \"^7%s^7 was gibbed.\"", vic->client->pers.netname));

	return;
}

/**
 * @brief kill command - kills players
 */
static void Svcmd_Die(void)
{
	int       pids[MAX_CLIENTS];
	char      name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	gentity_t *vic;
	qboolean  doAll = qfalse;

	// FIXME: usage
	//        add reason?

	// ignore in intermission
	if (level.intermissiontime)
	{
		G_Printf("Die command not allowed during intermission.\n");
		return;
	}

	if (trap_Argc() < 2)
	{
		doAll = qtrue;
	}

	trap_Argv(1, name, sizeof(name));

	if (!Q_stricmp(name, "-1") || doAll)
	{
		int it, count = 0;

		for (it = 0; it < level.numConnectedClients; it++)
		{
			vic = g_entities + level.sortedClients[it];
			if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
			      vic->client->sess.sessionTeam == TEAM_ALLIES))
			{
				continue;
			}
			G_Damage(vic, NULL, NULL, NULL, NULL, 140, 0, MOD_UNKNOWN);
			count++;
		}

		if (count > 0)
		{
			CPx(-1, va("cp \"^3%d^7 players died.\"", count));
		}
		else
		{
			G_Printf("There is no player to die.\n");
		}

		return;
	}

	if (ClientNumbersFromString(name, pids) != 1)
	{
		G_MatchOnePlayer(pids, err, sizeof(err));
		G_Printf("Error - can't execute die command - %s.\n", err);
		return;
	}
	vic = &g_entities[pids[0]];

	if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
	      vic->client->sess.sessionTeam == TEAM_ALLIES))
	{
		G_Printf("Player must be on a team to die.\n");
		return;
	}

	G_Damage(vic, NULL, NULL, NULL, NULL, 140, 0, MOD_UNKNOWN);

	CPx(-1, va("cp \"^7%s^7 died.\"", vic->client->pers.netname));

	return;
}

/**
 * @brief freeze command - freezes players
 */
static void Svcmd_Freeze(void)
{
	int       pids[MAX_CLIENTS];
	char      name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	gentity_t *vic;
	qboolean  doAll = qfalse;

	// FIXME: usage
	//        make all outputs nice - cp with icon?
	//        reason?

	// ignore in intermission
	if (level.intermissiontime)
	{
		G_Printf("Freeze command not allowed during intermission.\n");
		return;
	}

	if (trap_Argc() < 2)
	{
		doAll = qtrue;
	}

	trap_Argv(1, name, sizeof(name));

	if (!Q_stricmp(name, "-1") || doAll)
	{
		int it, count = 0;

		for (it = 0; it < level.numConnectedClients; it++)
		{
			vic = g_entities + level.sortedClients[it];
			if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
			      vic->client->sess.sessionTeam == TEAM_ALLIES))
			{
				continue;
			}

			// only freeze & count not frozen players :)
			if (vic->client->freezed == qtrue)
			{
				continue;
			}

			vic->client->freezed = qtrue;
			vic->takedamage      = qfalse;
			count++;
		}

		if (count > 0)
		{
			CPx(-1, va("cp \"^3%d^7 players are frozen.\"", count));
		}
		else
		{
			G_Printf("No players in team or they are already frozen.\n");
		}

		return;
	}

	if (ClientNumbersFromString(name, pids) != 1)
	{
		G_MatchOnePlayer(pids, err, sizeof(err));
		G_Printf("Error - can't freeze - %s.\n", err);
		return;
	}
	vic = &g_entities[pids[0]];

	if (!(vic->client->sess.sessionTeam == TEAM_AXIS || vic->client->sess.sessionTeam == TEAM_ALLIES))
	{
		G_Printf("Player must be on a team to be frozen.\n");
		return;
	}

	vic->client->freezed = qtrue;
	vic->takedamage      = qfalse;

	CPx(-1, va("cp \"^7%s^7 is frozen.\"", vic->client->pers.netname));

	return;
}

/**
 * @brief unfreeze command - unfreezes players
 */
static void Svcmd_Unfreeze(void)
{
	int       pids[MAX_CLIENTS];
	char      name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	gentity_t *vic;
	qboolean  doAll = qfalse;

	// FIXME: usage
	//        make all outputs nice - cp with icon?
	//        reason?

	if (trap_Argc() < 2)
	{
		doAll = qtrue;
	}

	trap_Argv(1, name, sizeof(name));

	if (!Q_stricmp(name, "-1") || doAll)
	{
		int it, count = 0;

		for (it = 0; it < level.numConnectedClients; it++)
		{
			vic = g_entities + level.sortedClients[it];
			if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
			      vic->client->sess.sessionTeam == TEAM_ALLIES))
			{
				continue;
			}

			// only unfreeze & count not frozen players :)
			if (vic->client->freezed == qfalse)
			{
				continue;
			}

			vic->client->freezed = qfalse;
			vic->takedamage      = qtrue;
			count++;
		}

		if (count > 0)
		{
			CPx(-1, va("cp \"^3%d^7 players are unfrozen.\"", count));
		}
		else
		{
			G_Printf("No players in team or they are already unfrozen.\n");
		}

		return;
	}

	if (ClientNumbersFromString(name, pids) != 1)
	{
		G_MatchOnePlayer(pids, err, sizeof(err));
		G_Printf("Error - can't unfreeze - %s.\n", err);
		return;
	}
	vic = &g_entities[pids[0]];

	if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
	      vic->client->sess.sessionTeam == TEAM_ALLIES))
	{
		G_Printf("Player must be on a team to be unfrozen.\n");
		return;
	}

	vic->client->freezed = qfalse;
	vic->takedamage      = qtrue;

	CPx(-1, va("cp \"^7%s^7 is unfrozen.\"", vic->client->pers.netname));

	return;
}

/**
 * @brief burn command - burns players
 */
static void Svcmd_Burn(void)
{
	int       pids[MAX_CLIENTS];
	char      name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	gentity_t *vic;
	qboolean  doAll = qfalse;

	// FIXME: usage
	//        make all outputs nice - cp with icon?
	//        reason?

	// ignore in intermission
	if (level.intermissiontime)
	{
		G_Printf("Burn command not allowed during intermission.\n");
		return;
	}

	if (trap_Argc() < 2)
	{
		doAll = qtrue;
	}

	trap_Argv(1, name, sizeof(name));

	if (!Q_stricmp(name, "-1") || doAll)
	{
		int it, count = 0;

		for (it = 0; it < level.numConnectedClients; it++)
		{
			vic = g_entities + level.sortedClients[it];
			if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
			      vic->client->sess.sessionTeam == TEAM_ALLIES))
			{
				continue;
			}

			vic->client->ps.eFlags |= EF_SMOKING;
			// FIXME: add mod param? mod_unknown instead of flamer
			G_BurnMeGood(vic, vic, NULL);
			count++;
		}

		if (count > 0)
		{
			CPx(-1, va("cp \"^3%d^7 players burned.\"", count));
		}
		else
		{
			G_Printf("No players in team or they are already burned.\n");
		}

		return;
	}

	if (ClientNumbersFromString(name, pids) != 1)
	{
		G_MatchOnePlayer(pids, err, sizeof(err));
		G_Printf("Error - can't burn - %s.\n", err);
		return;
	}
	vic = &g_entities[pids[0]];
	if (!(vic->client->sess.sessionTeam == TEAM_AXIS || vic->client->sess.sessionTeam == TEAM_ALLIES))
	{
		G_Printf("Player must be on a team to be burned.\n");
		return;
	}

	vic->client->ps.eFlags |= EF_SMOKINGBLACK;
	// FIXME: add mod param? mod_unknown instead of flamer
	G_BurnMeGood(vic, vic, NULL);

	CPx(-1, va("cp \"^7%s^7 is burned.\"", vic->client->pers.netname));
	return;
}

// FIXME/note:
// beside the pip command EV_SPARKS and EV_SPARKS_ELECTRIC
// aren't used for real ... we may delete these events
// EV_SPARKS_ELECTRIC doesn't seem to work anyway
static void G_Pip(gentity_t *vic)
{
	gentity_t *pip = G_TempEntity(vic->r.currentOrigin, EV_SPARKS);

	VectorCopy(vic->r.currentOrigin, pip->s.origin);
	VectorCopy(vic->r.currentAngles, pip->s.angles);
	pip->s.origin[2] -= 6;
	pip->s.density    = 5000;
	pip->s.frame      = 6000;
	pip->s.angles2[0] = 18;
	pip->s.angles2[1] = 18;
	pip->s.angles2[2] = .5;
}

/**
 * @brief pip command - pips players
 */
static void Svcmd_Pip(void)
{
	int       pids[MAX_CLIENTS];
	char      name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	gentity_t *vic;
	qboolean  doAll = qfalse;

	// FIXME: usage
	//        make all outputs nice - cp with icon?
	//        reason?

	// ignore in intermission
	if (level.intermissiontime)
	{
		G_Printf("Pip command not allowed during intermission.\n");
		return;
	}

	if (trap_Argc() < 2)
	{
		doAll = qtrue;
	}

	trap_Argv(1, name, sizeof(name));

	if (!Q_stricmp(name, "-1") || doAll)
	{
		int it, count = 0;

		for (it = 0; it < level.numConnectedClients; it++)
		{
			vic = g_entities + level.sortedClients[it];
			if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
			      vic->client->sess.sessionTeam == TEAM_ALLIES))
			{
				continue;
			}

			G_Pip(vic);
			count++;
		}

		if (count > 0)
		{
			CPx(-1, va("cp \"^3%d^7 players pipped.\"", count));
		}
		else
		{
			G_Printf("No players in team or they are already pipped.\n");
		}

		return;
	}

	if (ClientNumbersFromString(name, pids) != 1)
	{
		G_MatchOnePlayer(pids, err, sizeof(err));
		G_Printf("Error - can't pip - %s.\n", err);
		return;
	}
	vic = &g_entities[pids[0]];
	if (!(vic->client->sess.sessionTeam == TEAM_AXIS || vic->client->sess.sessionTeam == TEAM_ALLIES))
	{
		G_Printf("Player must be on a team to be pipped.\n");
		return;
	}

	G_Pip(vic);

	CPx(-1, va("cp \"^7%s^7 is pipped.\"", vic->client->pers.netname));

	return;
}

static void Svcmd_Fling(int flingType) // 0 = fling, 1 = throw, 2 = launch
{
	int       pids[MAX_CLIENTS];
	char      name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	char      fling[9], pastTense[9];
	gentity_t *vic;
	qboolean  doAll = qfalse;

	switch (flingType)
	{
	case 0:
		Q_strncpyz(fling, "fling", sizeof(fling));
		Q_strncpyz(pastTense, "flung", sizeof(pastTense));
		break;
	case 1:
		Q_strncpyz(fling, "throw", sizeof(fling));
		Q_strncpyz(pastTense, "thrown", sizeof(pastTense));
		break;
	case 2:
		Q_strncpyz(fling, "launch", sizeof(fling));
		Q_strncpyz(pastTense, "launched", sizeof(pastTense));
		break;
	default:
		return;
	}

	// ignore in intermission
	if (level.intermissiontime)
	{
		G_Printf("%s command not allowed during intermission.\n", fling);
		return;
	}

	if (trap_Argc() < 2)
	{
		doAll = qtrue;
	}

	trap_Argv(1, name, sizeof(name));

	if (!Q_stricmp(name, "-1") || doAll)
	{
		int i, count = 0;

		for (i = 0; i < level.numConnectedClients; i++)
		{
			vic = g_entities + level.sortedClients[i];

			if (!(vic->client->sess.sessionTeam == TEAM_AXIS ||
			      vic->client->sess.sessionTeam == TEAM_ALLIES))
			{
				continue;
			}

			count += G_FlingClient(vic, flingType);
		}

		if (count > 0)
		{
			CPx(-1, va("cp \"^3%d^7 players %s.\"", count, pastTense));
		}
		else
		{
			G_Printf("No players in team or they are already %s.\n", pastTense);
		}

		return;
	}

	if (ClientNumbersFromString(name, pids) != 1)
	{
		G_MatchOnePlayer(pids, err, sizeof(err));
		G_Printf("Error - can't %s - %s.\n", fling, err);
		return;
	}
	vic = &g_entities[pids[0]];
	if (!(vic->client->sess.sessionTeam == TEAM_AXIS || vic->client->sess.sessionTeam == TEAM_ALLIES))
	{
		G_Printf("Player must be on a team to be %s.\n", pastTense);
		return;
	}

	if (G_FlingClient(vic, flingType))
	{
		CPx(-1, va("cp \"^7%s^7 was %s.\"", vic->client->pers.netname, pastTense));
	}

	return;
}

// change into qfalse if you want to use the qagame banning system
// which makes it possible to unban IP addresses
#define USE_ENGINE_BANLIST qtrue

/**
 * @brief kick a user off of the server
 */
static void Svcmd_Kick_f(void)
{
	gclient_t *cl;
	int       timeout;
	char      sTimeout[MAX_TOKEN_CHARS];
	char      name[MAX_TOKEN_CHARS];

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

	if (trap_Argc() == 3)
	{
		trap_Argv(2, sTimeout, sizeof(sTimeout));
		timeout = atoi(sTimeout);
	}
	else
	{
		timeout = 300;
	}

	trap_Argv(1, name, sizeof(name));
	cl = G_GetPlayerByName(name);   //ClientForString( name );

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
					char *ip;
					char userinfo[MAX_INFO_STRING];

					trap_GetUserinfo(cl->ps.clientNum, userinfo, sizeof(userinfo));
					ip = Info_ValueForKey(userinfo, "ip");

					// use engine banning system, mods may choose to use their own banlist
					if (USE_ENGINE_BANLIST)
					{

						// kick but dont ban bots, they arent that lame
						if ((g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT))
						{
							timeout = 0;
						}

						trap_DropClient(cl->ps.clientNum, "player kicked", timeout);
					}
					else
					{
						trap_DropClient(cl->ps.clientNum, "player kicked", 0);

						// kick but dont ban bots, they arent that lame
						if (!(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT))
						{
							AddIPBan(ip);
						}
					}
				}
				else
				{
					trap_DropClient(cl->ps.clientNum, "player kicked", 0);
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
			char *ip;
			char userinfo[MAX_INFO_STRING];

			trap_GetUserinfo(cl->ps.clientNum, userinfo, sizeof(userinfo));
			ip = Info_ValueForKey(userinfo, "ip");

			// use engine banning system, mods may choose to use their own banlist
			if (USE_ENGINE_BANLIST)
			{
				// kick but dont ban bots, they arent that lame
				if ((g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT))
				{
					timeout = 0;
				}
				trap_DropClient(cl->ps.clientNum, "player kicked", timeout);
			}
			else
			{
				trap_DropClient(cl->ps.clientNum, "player kicked", 0);

				// kick but dont ban bots, they arent that lame
				if (!(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT))
				{
					AddIPBan(ip);
				}
			}
		}
		else
		{
			trap_DropClient(cl->ps.clientNum, "player kicked", 0);
		}
	}
}

/**
 * @brief kick a user off of the server
 */
static void Svcmd_KickNum_f(void)
{
	gclient_t *cl;
	int       timeout;
	char      *ip;
	char      userinfo[MAX_INFO_STRING];
	char      sTimeout[MAX_TOKEN_CHARS];
	char      name[MAX_TOKEN_CHARS];
	int       clientNum;

	// make sure server is running
	if (!G_Is_SV_Running())
	{
		G_Printf("Server is not running.\n");
		return;
	}

	if (trap_Argc() < 2 || trap_Argc() > 3)
	{
		G_Printf("Usage: kick <client number> [timeout]\n");
		return;
	}

	if (trap_Argc() == 3)
	{
		trap_Argv(2, sTimeout, sizeof(sTimeout));
		timeout = atoi(sTimeout);
	}
	else
	{
		timeout = 300;
	}

	trap_Argv(1, name, sizeof(name));
	clientNum = atoi(name);

	cl = G_GetPlayerByNum(clientNum);
	if (!cl)
	{
		return;
	}
	if (cl->pers.localClient)
	{
		G_Printf("Cannot kick host player\n");
		return;
	}

	trap_GetUserinfo(cl->ps.clientNum, userinfo, sizeof(userinfo));
	ip = Info_ValueForKey(userinfo, "ip");
	// use engine banning system, mods may choose to use their own banlist
	if (USE_ENGINE_BANLIST)
	{
		// kick but dont ban bots, they arent that lame
		if ((g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT))
		{
			timeout = 0;
		}
		trap_DropClient(cl->ps.clientNum, "player kicked", timeout);
	}
	else
	{
		trap_DropClient(cl->ps.clientNum, "player kicked", 0);

		// kick but dont ban bots, they arent that lame
		if (!(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT))
		{
			AddIPBan(ip);
		}
	}
}

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
		G_Printf("usage: sv_cvar <cvar name> <mode> <value1> <value2>\nexamples: sv_cvar cg_hitsounds EQ 1\n          sv_cvar cl_maxpackets IN 60 100\n");
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

void CC_loadconfig(void)
{
	char scriptName[MAX_QPATH];

	if (trap_Argc() != 2)
	{
		G_Printf("usage: loadConfig <config name>\n");
		return;
	}

	trap_Argv(1, scriptName, sizeof(scriptName));

	trap_SetConfigstring(CS_CONFIGNAME, "");
	memset(&level.config, 0, sizeof(config_t));
	G_configSet(scriptName);
}

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
			value = atoi(valuestr);
			if (value >= MAX_CONFIGSTRINGS)
			{
				value = -1;
			}
		}
	}
	else
	{
		G_Printf("Note: csinfo <CS No.> will print the content of given string\n\n");
	}

	G_Printf("CS   Length   Type\n--------------------------------------------\n");
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		trap_GetConfigstring(i, cs, sizeof(cs));
		size   = strlen(cs);
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
		case CS_LEGACYINFO:
			str = "CS_LEGACYINFO";
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
			if ((arg1numeric && value == i) || (!arg1numeric && !Q_stricmp(valuestr, str)))
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

qboolean ConsoleCommand(void)
{
	char cmd[MAX_TOKEN_CHARS];

	trap_Argv(0, cmd, sizeof(cmd));

#ifdef FEATURE_LUA
	if (!Q_stricmp(cmd, "lua_status"))
	{
		G_LuaStatus(NULL);
		return qtrue;
	}
	if (Q_stricmp(cmd, "lua_api") == 0)
	{
		G_LuaStackDump();
		return qtrue;
	}
	// *LUA* API callbacks
	if (G_LuaHook_ConsoleCommand(cmd))
	{
		return qtrue;
	}
#endif
	if (Q_stricmp(cmd, "entitylist") == 0)
	{
		Svcmd_EntityList_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "csinfo") == 0)
	{
		Svcmd_CSInfo_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "forceteam") == 0)
	{
		Svcmd_ForceTeam_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "game_memory") == 0)
	{
		Svcmd_GameMem_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "addip") == 0)
	{
		Svcmd_AddIP_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "removeip") == 0)
	{
		Svcmd_RemoveIP_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "listip") == 0)
	{
		trap_SendConsoleCommand(EXEC_INSERT, "g_banIPs\n");
		return qtrue;
	}
	if (Q_stricmp(cmd, "listmaxlivesip") == 0)
	{
		PrintMaxLivesGUID();
		return qtrue;
	}
	if (Q_stricmp(cmd, "start_match") == 0)
	{
		Svcmd_StartMatch_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "reset_match") == 0)
	{
		Svcmd_ResetMatch_f(qtrue, qtrue);
		return qtrue;
	}
	if (Q_stricmp(cmd, "swap_teams") == 0)
	{
		Svcmd_SwapTeams_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "shuffle_teams") == 0)
	{
		Svcmd_ShuffleTeams_f(qtrue);
		return qtrue;
	}
	if (Q_stricmp(cmd, "shuffle_teams_norestart") == 0)
	{
		Svcmd_ShuffleTeams_f(qfalse);
		return qtrue;
	}
	if (Q_stricmp(cmd, "makeReferee") == 0)
	{
		G_MakeReferee();
		return qtrue;
	}
	if (Q_stricmp(cmd, "removeReferee") == 0)
	{
		G_RemoveReferee();
		return qtrue;
	}
	if (Q_stricmp(cmd, "mute") == 0)
	{
		G_MuteClient();
		return qtrue;
	}
	if (Q_stricmp(cmd, "unmute") == 0)
	{
		G_UnMuteClient();
		return qtrue;
	}
	if (Q_stricmp(cmd, "ban") == 0)
	{
		G_PlayerBan();
		return qtrue;
	}
	if (Q_stricmp(cmd, "campaign") == 0)
	{
		Svcmd_Campaign_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "listcampaigns") == 0)
	{
		Svcmd_ListCampaigns_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "revive") == 0)
	{
		trap_Argv(1, cmd, sizeof(cmd));
		Svcmd_RevivePlayer(cmd);
		return qtrue;
	}
	// moved from engine
	if (!Q_stricmp(cmd, "kick"))
	{
		Svcmd_Kick_f();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "clientkick"))
	{
		Svcmd_KickNum_f();
		return qtrue;
	}
#ifdef FEATURE_OMNIBOT
	if (!Q_stricmp(cmd, "bot"))
	{
		Bot_Interface_ConsoleCommand();
		return qtrue;
	}
#endif
	if (!Q_stricmp(cmd, "cp"))
	{
		trap_SendServerCommand(-1, va("cp \"%s\"", Q_AddCR(ConcatArgs(1))));
		return qtrue;
	}
	if (!Q_stricmp(cmd, "reloadConfig"))
	{
		trap_SetConfigstring(CS_CONFIGNAME, "");
		memset(&level.config, 0, sizeof(config_t));
		G_configSet(g_customConfig.string);

		return qtrue;
	}
	if (!Q_stricmp(cmd, "loadConfig"))
	{
		CC_loadconfig();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "sv_cvarempty"))
	{
		memset(level.svCvars, 0, sizeof(level.svCvars));
		level.svCvarsCount = 0;
		G_UpdateSvCvars();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "sv_cvar"))
	{
		CC_svcvar();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "playsound") || !Q_stricmp(cmd, "playsound_env"))
	{
		G_PlaySound_Cmd();
		return qtrue;
	}
	//if (g_cheats.integer)
	//{
	if (!Q_stricmp(cmd, "gib"))
	{
		Svcmd_Gib();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "die"))
	{
		Svcmd_Die();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "freeze"))
	{
		Svcmd_Freeze();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "unfreeze"))
	{
		Svcmd_Unfreeze();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "burn"))
	{
		Svcmd_Burn();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "pip"))
	{
		Svcmd_Pip();
		return qtrue;
	}
	if (!Q_stricmp(cmd, "throw"))
	{
		Svcmd_Fling(1);
		return qtrue;
	}
	//}

	if (g_dedicated.integer)
	{
		// FIXME
		// this 'say' condition is never reached?!
		if (!Q_stricmp(cmd, "say"))
		{
			trap_SendServerCommand(-1, va("cpm \"server: %s\n\"", Q_AddCR(ConcatArgs(1))));
			return qtrue;
		}
		// added for rcon/Lua chat
		if (!Q_stricmp(cmd, "chat"))
		{
			trap_SendServerCommand(-1, va("chat \"console: %s\"", Q_AddCR(ConcatArgs(1))));
			return qtrue;
		}

		// console also gets ref commands
		if (!level.fLocalHost && Q_stricmp(cmd, "ref") == 0)
		{
			//G_refCommandCheck expects the next argument (warn, pause, lock,..)
			trap_Argv(1, cmd, sizeof(cmd));
			if (!G_refCommandCheck(NULL, cmd))
			{
				G_refHelp_cmd(NULL);
			}
			return qtrue;
		}

		// everything else will also be printed as a say command
		//trap_SendServerCommand( -1, va("cpm \"server: %s\n\"", ConcatArgs(0) ) );

		// prints to the console instead now
		return qfalse;
	}

	return qfalse;
}
