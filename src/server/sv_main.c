/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 * @file sv_main.c
 */

#include "server.h"

serverStatic_t svs;                 // persistant server info
server_t       sv;                  // local server
vm_t           *gvm = NULL;         // game virtual machine

cvar_t *sv_fps = NULL;          // time rate for running non-clients
cvar_t *sv_timeout;             // seconds without any message
cvar_t *sv_zombietime;          // seconds to sink messages after disconnect
cvar_t *sv_rconPassword;        // password for remote server commands
cvar_t *sv_privatePassword;     // password for the privateClient slots
cvar_t *sv_allowDownload;
cvar_t *sv_maxclients;

cvar_t *sv_privateClients;      // number of clients reserved for password
cvar_t *sv_hostname;
cvar_t *sv_master[MAX_MASTER_SERVERS];      // master server ip address
cvar_t *sv_reconnectlimit;      // minimum seconds between connect messages
cvar_t *sv_tempbanmessage;
cvar_t *sv_showloss;            // report when usercmds are lost
cvar_t *sv_padPackets;          // add nop bytes to messages
cvar_t *sv_killserver;          // menu system can set to 1 to shut server down
cvar_t *sv_mapname;
cvar_t *sv_mapChecksum;
cvar_t *sv_serverid;
cvar_t *sv_maxRate;
cvar_t *sv_minPing;
cvar_t *sv_maxPing;
cvar_t *sv_pure;
cvar_t *sv_floodProtect;
cvar_t *sv_allowAnonymous;
cvar_t *sv_lanForceRate;  // TTimo - dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
cvar_t *sv_onlyVisibleClients;  // DHM - Nerve
cvar_t *sv_friendlyFire;        // NERVE - SMF
cvar_t *sv_maxlives;            // NERVE - SMF
cvar_t *sv_needpass;

cvar_t *sv_dl_maxRate;

cvar_t *g_gameType;

cvar_t *sv_reloading;

cvar_t *sv_showAverageBPS;      // NERVE - SMF - net debugging

cvar_t *sv_wwwDownload;  // server does a www dl redirect
cvar_t *sv_wwwBaseURL;  // base URL for redirect
// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
cvar_t *sv_wwwDlDisconnected;
cvar_t *sv_wwwFallbackURL; // URL to send to if an http/ftp fails or is refused client side

cvar_t *sv_cheats;
cvar_t *sv_packetloss;
cvar_t *sv_packetdelay;

cvar_t *sv_fullmsg;

static void SVC_Status(netadr_t from);

#define LL(x) x = LittleLong(x)

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
 * @brief Converts newlines to "\n" so a line prints nicer
 */
static char *SV_ExpandNewlines(char *in)
{
	static char string[1024];
	int         l = 0;

	while (*in && l < sizeof(string) - 3)
	{
		if (*in == '\n')
		{
			string[l++] = '\\';
			string[l++] = 'n';
		}
		else
		{
			// HACK: strip out localization tokens before string command is displayed in syscon window
			if (!Q_strncmp(in, "[lon]", 5) || !Q_strncmp(in, "[lof]", 5))
			{
				in += 5;
				continue;
			}

			string[l++] = *in;
		}
		in++;
	}
	string[l] = 0;

	return string;
}

/*
 * @brief The given command will be transmitted to the client, and is guaranteed
 * to not have future snapshot_t executed before it is executed
 */
void SV_AddServerCommand(client_t *client, const char *cmd)
{
	int index, i;

	client->reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SV_DropClient()
	// doesn't cause a recursive drop client
	if (client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1)
	{
		Com_Printf("===== pending server commands =====\n");
		for (i = client->reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++)
		{
			Com_Printf("cmd %5d: %s\n", i, client->reliableCommands[i & (MAX_RELIABLE_COMMANDS - 1)]);
		}
		Com_Printf("cmd %5d: %s\n", i, cmd);
		SV_DropClient(client, "Server command overflow");
		return;
	}
	index = client->reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
	Q_strncpyz(client->reliableCommands[index], cmd, sizeof(client->reliableCommands[index]));
}


/*
 * @brief Sends a reliable command string to be interpreted by the client game
 * module: "cp", "print", "chat", etc
 *
 * A NULL client will broadcast to all clients
 */
void QDECL SV_SendServerCommand(client_t *cl, const char *fmt, ...)
{
	va_list  argptr;
	byte     message[MAX_MSGLEN];
	client_t *client;
	int      j;

	va_start(argptr, fmt);
	Q_vsnprintf((char *)message, sizeof(message), fmt, argptr);
	va_end(argptr);

	// do not forward server command messages that would be too big to clients
	// ( q3infoboom / q3msgboom stuff )
	// see http://aluigi.altervista.org/adv/q3msgboom-adv.txt
	if (strlen((char *)message) > 1022)
	{
		return;
	}

	if (cl != NULL)
	{
		SV_AddServerCommand(cl, (char *)message);
		return;
	}

	// hack to echo broadcast prints to console
	if (com_dedicated->integer && !strncmp((char *)message, "print", 5))
	{
		Com_Printf("broadcast: %s\n", SV_ExpandNewlines((char *)message));
	}

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < sv_maxclients->integer ; j++, client++)
	{
		if (client->state < CS_PRIMED)
		{
			continue;
		}
		// Ridah, don't need to send messages to AI
		if (client->gentity && client->gentity->r.svFlags & SVF_BOT)
		{
			continue;
		}
		// done.
		SV_AddServerCommand(client, (char *)message);
	}
}


/*
==============================================================================

MASTER SERVER FUNCTIONS

==============================================================================
*/

/*
 * @brief Send a message to the masters every few minutes to let it know we are
 * alive, and log information.
 *
 * We will also have a heartbeat sent when a server changes from empty to
 * non-empty, and full to non-full, but not on every player enter or exit.
 */
#define HEARTBEAT_MSEC  300 * 1000
#define HEARTBEAT_GAME  "EnemyTerritory-1"
#define HEARTBEAT_DEAD  "ETFlatline-1"

void SV_MasterHeartbeat(const char *message)
{
	static netadr_t adr[MAX_MASTER_SERVERS][2]; // [2] for v4 and v6 address for the same address string.
	int             i;
	int             res;
	int             netenabled;

	netenabled = Cvar_VariableIntegerValue("net_enabled");

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if (!com_dedicated || com_dedicated->integer != 2 || !(netenabled & (NET_ENABLEV4 | NET_ENABLEV6)))
	{
		return;     // only dedicated servers send heartbeats

	}
	// if not time yet, don't send anything
	if (svs.time < svs.nextHeartbeatTime)
	{
		return;
	}

	svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;

	// send to group masters
	for (i = 0; i < MAX_MASTER_SERVERS; i++)
	{
		if (!sv_master[i]->string[0])
		{
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if (sv_master[i]->modified || (adr[i][0].type == NA_BAD && adr[i][1].type == NA_BAD))
		{
			sv_master[i]->modified = qfalse;

			if (netenabled & NET_ENABLEV4)
			{
				Com_Printf("Resolving %s (IPv4)\n", sv_master[i]->string);
				res = NET_StringToAdr(sv_master[i]->string, &adr[i][0], NA_IP);

				if (res == 2)
				{
					// if no port was specified, use the default master port
					adr[i][0].port = BigShort(PORT_MASTER);
				}

				if (res)
				{
					Com_Printf("%s resolved to %s\n", sv_master[i]->string, NET_AdrToStringwPort(adr[i][0]));
				}
				else
				{
					Com_Printf("%s has no IPv4 address.\n", sv_master[i]->string);
				}
			}

			if (netenabled & NET_ENABLEV6)
			{
				Com_Printf("Resolving %s (IPv6)\n", sv_master[i]->string);
				res = NET_StringToAdr(sv_master[i]->string, &adr[i][1], NA_IP6);

				if (res == 2)
				{
					// if no port was specified, use the default master port
					adr[i][1].port = BigShort(PORT_MASTER);
				}

				if (res)
				{
					Com_Printf("%s resolved to %s\n", sv_master[i]->string, NET_AdrToStringwPort(adr[i][1]));
				}
				else
				{
					Com_Printf("%s has no IPv6 address.\n", sv_master[i]->string);
				}
			}

			if (adr[i][0].type == NA_BAD && adr[i][1].type == NA_BAD)
			{
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				Com_Printf("Couldn't resolve address: %s\n", sv_master[i]->string);
				Cvar_Set(sv_master[i]->name, "");
				sv_master[i]->modified = qfalse;
				continue;
			}
		}


		Com_Printf("Sending heartbeat to %s\n", sv_master[i]->string);

		// this command should be changed if the server info / status format
		// ever incompatably changes

		if (adr[i][0].type != NA_BAD)
		{
			NET_OutOfBandPrint(NS_SERVER, adr[i][0], "heartbeat %s\n", HEARTBEAT_GAME);
		}
		if (adr[i][1].type != NA_BAD)
		{
			NET_OutOfBandPrint(NS_SERVER, adr[i][1], "heartbeat %s\n", HEARTBEAT_GAME);
		}
	}
}

/*
 * @brief Sends gameCompleteStatus messages to all master servers
 */
void SV_MasterGameCompleteStatus()
{
	static netadr_t adr[MAX_MASTER_SERVERS];
	int             i;

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if (!com_dedicated || com_dedicated->integer != 2)
	{
		return;     // only dedicated servers send master game status
	}

	// send to group masters
	for (i = 0 ; i < MAX_MASTER_SERVERS ; i++)
	{
		if (!sv_master[i]->string[0])
		{
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if (sv_master[i]->modified)
		{
			sv_master[i]->modified = qfalse;

			Com_Printf("Resolving %s\n", sv_master[i]->string);
			if (!NET_StringToAdr(sv_master[i]->string, &adr[i], NA_IP))
			{
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				Com_Printf("Couldn't resolve address: %s\n", sv_master[i]->string);
				Cvar_Set(sv_master[i]->name, "");
				sv_master[i]->modified = qfalse;
				continue;
			}
			if (!strstr(":", sv_master[i]->string))
			{
				adr[i].port = BigShort(PORT_MASTER);
			}
			Com_Printf("%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string,
			           adr[i].ip[0], adr[i].ip[1], adr[i].ip[2], adr[i].ip[3],
			           BigShort(adr[i].port));
		}

		Com_Printf("Sending gameCompleteStatus to %s\n", sv_master[i]->string);
		// this command should be changed if the server info / status format
		// ever incompatably changes
		SVC_Status(adr[i]);
	}
}

/*
 * @brief Informs all masters that this server is going down
 */
void SV_MasterShutdown(void)
{
	// send a hearbeat right now
	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat(HEARTBEAT_DEAD);                 // NERVE - SMF - changed to flatline

	// send it again to minimize chance of drops
//	svs.nextHeartbeatTime = -9999;
//	SV_MasterHeartbeat( HEARTBEAT_DEAD );

	// when the master tries to poll the server, it won't respond, so
	// it will be removed from the list
}


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

typedef struct leakyBucket_s leakyBucket_t;
struct leakyBucket_s
{
	netadrtype_t type;

	union
	{
		byte _4[4];
		byte _6[16];
	} ipv;

	int lastTime;
	signed char burst;

	long hash;

	leakyBucket_t *prev, *next;
};

// This is deliberately quite large to make it more of an effort to DoS
#define MAX_BUCKETS         16384
#define MAX_HASHES          1024

static leakyBucket_t buckets[MAX_BUCKETS];
static leakyBucket_t *bucketHashes[MAX_HASHES];

static long SVC_HashForAddress(netadr_t address)
{
	byte   *ip  = NULL;
	size_t size = 0;
	int    i;
	long   hash = 0;

	switch (address.type)
	{
	case NA_IP:  ip = address.ip;  size = 4; break;
	case NA_IP6: ip = address.ip6; size = 16; break;
	default: break;
	}

	for (i = 0; i < size; i++)
	{
		hash += (long)(ip[i]) * (i + 119);
	}

	hash  = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (MAX_HASHES - 1);

	return hash;
}

/*
 * @brief Find or allocate a bucket for an address
 */
static leakyBucket_t *SVC_BucketForAddress(netadr_t address, int burst, int period)
{
	leakyBucket_t *bucket = NULL;
	int           i;
	long          hash = SVC_HashForAddress(address);
	int           now  = Sys_Milliseconds();

	for (bucket = bucketHashes[hash]; bucket; bucket = bucket->next)
	{
		switch (bucket->type)
		{
		case NA_IP:
			if (memcmp(bucket->ipv._4, address.ip, 4) == 0)
			{
				return bucket;
			}
			break;

		case NA_IP6:
			if (memcmp(bucket->ipv._6, address.ip6, 16) == 0)
			{
				return bucket;
			}
			break;

		default:
			break;
		}
	}

	for (i = 0; i < MAX_BUCKETS; i++)
	{
		int interval;

		bucket   = &buckets[i];
		interval = now - bucket->lastTime;

		// Reclaim expired buckets
		if (bucket->lastTime > 0 && (interval > (burst * period) ||
		                             interval < 0))
		{
			if (bucket->prev != NULL)
			{
				bucket->prev->next = bucket->next;
			}
			else
			{
				bucketHashes[bucket->hash] = bucket->next;
			}

			if (bucket->next != NULL)
			{
				bucket->next->prev = bucket->prev;
			}

			Com_Memset(bucket, 0, sizeof(leakyBucket_t));
		}

		if (bucket->type == NA_BAD)
		{
			bucket->type = address.type;
			switch (address.type)
			{
			case NA_IP:  Com_Memcpy(bucket->ipv._4, address.ip, 4);   break;
			case NA_IP6: Com_Memcpy(bucket->ipv._6, address.ip6, 16); break;
			default: break;
			}

			bucket->lastTime = now;
			bucket->burst    = 0;
			bucket->hash     = hash;

			// Add to the head of the relevant hash chain
			bucket->next = bucketHashes[hash];
			if (bucketHashes[hash] != NULL)
			{
				bucketHashes[hash]->prev = bucket;
			}

			bucket->prev       = NULL;
			bucketHashes[hash] = bucket;

			return bucket;
		}
	}

	// Couldn't allocate a bucket for this address
	return NULL;
}

static qboolean SVC_RateLimit(leakyBucket_t *bucket, int burst, int period)
{
	if (bucket != NULL)
	{
		int now              = Sys_Milliseconds();
		int interval         = now - bucket->lastTime;
		int expired          = interval / period;
		int expiredRemainder = interval % period;

		if (expired > bucket->burst)
		{
			bucket->burst    = 0;
			bucket->lastTime = now;
		}
		else
		{
			bucket->burst   -= expired;
			bucket->lastTime = now - expiredRemainder;
		}

		if (bucket->burst < burst)
		{
			bucket->burst++;

			return qfalse;
		}
	}

	return qtrue;
}

/*
 * @brief Rate limit for a particular address
 */
static qboolean SVC_RateLimitAddress(netadr_t from, int burst, int period)
{
	leakyBucket_t *bucket = SVC_BucketForAddress(from, burst, period);

	return SVC_RateLimit(bucket, burst, period);
}

/*
 * @brief Send serverinfo cvars, etc to master servers when game complete.
 * Useful for tracking global player stats.
 */
static void SVC_Status(netadr_t from)
{
	char                 player[1024];
	char                 status[MAX_MSGLEN];
	int                  i;
	client_t             *cl;
	playerState_t        *ps;
	int                  statusLength;
	int                  playerLength;
	char                 infostring[MAX_INFO_STRING];
	static leakyBucket_t bucket;

	// Prevent using getstatus as an amplifier
	if (SVC_RateLimitAddress(from, 10, 1000))
	{
		Com_DPrintf("SVC_Status: rate limit from %s exceeded, dropping request\n",
		            NET_AdrToString(from));
		return;
	}

	// Allow getstatus to be DoSed relatively easily, but prevent
	// excess outbound bandwidth usage when being flooded inbound
	if (SVC_RateLimit(&bucket, 10, 100))
	{
		Com_DPrintf("SVC_Status: rate limit exceeded, dropping request\n");
		return;
	}

	strcpy(infostring, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));

	// echo back the parameter to status. so master servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1));
	Info_SetValueForKey(infostring, "version", ET_VERSION);

	status[0]    = 0;
	statusLength = 0;

	for (i = 0 ; i < sv_maxclients->integer ; i++)
	{
		cl = &svs.clients[i];
		if (cl->state >= CS_CONNECTED)
		{
			ps = SV_GameClientNum(i);
			Com_sprintf(player, sizeof(player), "%i %i \"%s\"\n",
			            ps->persistant[PERS_SCORE], cl->ping, cl->name);
			playerLength = strlen(player);
			if (statusLength + playerLength >= sizeof(status))
			{
				break;      // can't hold any more
			}
			strcpy(status + statusLength, player);
			statusLength += playerLength;
		}
	}

	NET_OutOfBandPrint(NS_SERVER, from, "statusResponse\n%s\n%s", infostring, status);
}

/*
 * @brief Responds with a short info message that should be enough to determine
 * if a user is interested in a server to do a full status
 */
void SVC_Info(netadr_t from)
{
	int  i, count, humans;
	char *gamedir;
	char infostring[MAX_INFO_STRING];
	char *antilag;
	char *weaprestrict;
	char *balancedteams;

	/*
	 * Check whether Cmd_Argv(1) has a sane length. This was not done in the original Quake3 version which led
	 * to the Infostring bug discovered by Luigi Auriemma. See http://aluigi.altervista.org/ for the advisory.
	 */

	// A maximum challenge length of 128 should be more than plenty.
	if (strlen(Cmd_Argv(1)) > 128)
	{
		return;
	}

	// don't count privateclients
	count = humans = 0;
	for (i = sv_privateClients->integer ; i < sv_maxclients->integer ; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			count++;
			if (svs.clients[i].netchan.remoteAddress.type != NA_BOT)
			{
				humans++;
			}
		}
	}

	infostring[0] = 0;

	// echo back the parameter to status. so servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1));

	Info_SetValueForKey(infostring, "protocol", va("%i", PROTOCOL_VERSION));
	Info_SetValueForKey(infostring, "hostname", sv_hostname->string);
	Info_SetValueForKey(infostring, "serverload", va("%i", svs.serverLoad));
	Info_SetValueForKey(infostring, "mapname", sv_mapname->string);
	Info_SetValueForKey(infostring, "clients", va("%i", count));
	Info_SetValueForKey(infostring, "sv_maxclients", va("%i", sv_maxclients->integer - sv_privateClients->integer));
	Info_SetValueForKey(infostring, "gametype", Cvar_VariableString("g_gametype"));
	Info_SetValueForKey(infostring, "pure", va("%i", sv_pure->integer));

	if (sv_minPing->integer)
	{
		Info_SetValueForKey(infostring, "minPing", va("%i", sv_minPing->integer));
	}
	if (sv_maxPing->integer)
	{
		Info_SetValueForKey(infostring, "maxPing", va("%i", sv_maxPing->integer));
	}
	gamedir = Cvar_VariableString("fs_game");
	if (*gamedir)
	{
		Info_SetValueForKey(infostring, "game", gamedir);
	}
	Info_SetValueForKey(infostring, "sv_allowAnonymous", va("%i", sv_allowAnonymous->integer));

	Info_SetValueForKey(infostring, "friendlyFire", va("%i", sv_friendlyFire->integer));            // NERVE - SMF
	Info_SetValueForKey(infostring, "maxlives", va("%i", sv_maxlives->integer ? 1 : 0));            // NERVE - SMF
	Info_SetValueForKey(infostring, "needpass", va("%i", sv_needpass->integer ? 1 : 0));
	Info_SetValueForKey(infostring, "gamename", GAMENAME_STRING);                                 // Arnout: to be able to filter out Quake servers

	// TTimo
	antilag = Cvar_VariableString("g_antilag");
	if (antilag)
	{
		Info_SetValueForKey(infostring, "g_antilag", antilag);
	}

	weaprestrict = Cvar_VariableString("g_heavyWeaponRestriction");
	if (weaprestrict)
	{
		Info_SetValueForKey(infostring, "weaprestrict", weaprestrict);
	}

	balancedteams = Cvar_VariableString("g_balancedteams");
	if (balancedteams)
	{
		Info_SetValueForKey(infostring, "balancedteams", balancedteams);
	}

	NET_OutOfBandPrint(NS_SERVER, from, "infoResponse\n%s", infostring);
}

static void SV_FlushRedirect(char *outputbuf)
{
	NET_OutOfBandPrint(NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf);
}

/*
 * DRDoS stands for "Distributed Reflected Denial of Service".
 * See here: http://www.lemuria.org/security/application-drdos.html
 *
 * If the address isn't NA_IP, it's automatically denied.
 *
 * @retval qfalse if we're good.
 * @retval qtrue return value means we need to block.
 */
qboolean SV_CheckDRDoS(netadr_t from)
{
	int        i;
	int        globalCount;
	int        specificCount;
	receipt_t  *receipt;
	netadr_t   exactFrom;
	int        oldest;
	int        oldestTime;
	static int lastGlobalLogTime   = 0;
	static int lastSpecificLogTime = 0;

	// Usually the network is smart enough to not allow incoming UDP packets
	// with a source address being a spoofed LAN address.  Even if that's not
	// the case, sending packets to other hosts in the LAN is not a big deal.
	// NA_LOOPBACK qualifies as a LAN address.
	if (Sys_IsLANAddress(from))
	{
		return qfalse;
	}

	exactFrom = from;

	if (from.type == NA_IP)
	{
		from.ip[3] = 0; // xx.xx.xx.0
	}
	else
	{
		from.ip6[15] = 0;
	}

	// Count receipts in last 2 seconds.
	globalCount   = 0;
	specificCount = 0;
	receipt       = &svs.infoReceipts[0];
	oldest        = 0;
	oldestTime    = 0x7fffffff;
	for (i = 0; i < MAX_INFO_RECEIPTS; i++, receipt++)
	{
		if (receipt->time + 2000 > svs.time)
		{
			if (receipt->time)
			{
				// When the server starts, all receipt times are at zero.  Furthermore,
				// svs.time is close to zero.  We check that the receipt time is already
				// set so that during the first two seconds after server starts, queries
				// from the master servers don't get ignored.  As a consequence a potentially
				// unlimited number of getinfo+getstatus responses may be sent during the
				// first frame of a server's life.
				globalCount++;
			}
			if (NET_CompareBaseAdr(from, receipt->adr))
			{
				specificCount++;
			}
		}
		if (receipt->time < oldestTime)
		{
			oldestTime = receipt->time;
			oldest     = i;
		}
	}

	if (globalCount == MAX_INFO_RECEIPTS)   // All receipts happened in last 2 seconds.
	{
		if (lastGlobalLogTime + 1000 <= svs.time)  // Limit one log every second.
		{
			Com_Printf("Detected flood of getinfo/getstatus connectionless packets\n");
			lastGlobalLogTime = svs.time;
		}
		return qtrue;
	}
	if (specificCount >= 3)   // Already sent 3 to this IP in last 2 seconds.
	{
		if (lastSpecificLogTime + 1000 <= svs.time)   // Limit one log every second.
		{
			Com_Printf("Possible DRDoS attack to address %i.%i.%i.%i, ignoring getinfo/getstatus connectionless packet\n",
			           exactFrom.ip[0], exactFrom.ip[1], exactFrom.ip[2], exactFrom.ip[3]);
			lastSpecificLogTime = svs.time;
		}
		return qtrue;
	}

	receipt       = &svs.infoReceipts[oldest];
	receipt->adr  = from;
	receipt->time = svs.time;
	return qfalse;
}

/*
 * @brief An rcon packet arrived from the network.
 *
 * Shift down the remaining args. Redirect all printfs.
 */
static void SVC_RemoteCommand(netadr_t from, msg_t *msg)
{
	qboolean valid;
	char     remaining[1024];
	// if we send an OOB print message this size, 1.31 clients die in a Com_Printf buffer overflow
	// the buffer overflow will be fixed in > 1.31 clients
	// but we want a server side fix
	// we must NEVER send an OOB message that will be > 1.31 MAXPRINTMSG (4096)
#define SV_OUTPUTBUF_LENGTH (256 - 16)
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	char *cmd_aux;

	// Prevent using rcon as an amplifier and make dictionary attacks impractical
	if (SVC_RateLimitAddress(from, 10, 1000))
	{
		Com_DPrintf("SVC_RemoteCommand: rate limit from %s exceeded, dropping request\n",
		            NET_AdrToString(from));
		return;
	}

	if (!strlen(sv_rconPassword->string) ||
	    strcmp(Cmd_Argv(1), sv_rconPassword->string))
	{
		static leakyBucket_t bucket;

		// Make DoS via rcon impractical
		if (SVC_RateLimit(&bucket, 10, 1000))
		{
			Com_DPrintf("SVC_RemoteCommand: rate limit exceeded, dropping request\n");
			return;
		}

		valid = qfalse;
		Com_Printf("Bad rcon from %s: %s\n", NET_AdrToString(from), Cmd_Argv(2));
	}
	else
	{
		valid = qtrue;
		Com_Printf("Rcon from %s: %s\n", NET_AdrToString(from), Cmd_Argv(2));
	}

	// start redirecting all print outputs to the packet
	svs.redirectAddress = from;
	// FIXME TTimo our rcon redirection could be improved
	//   big rcon commands such as status lead to sending
	//   out of band packets on every single call to Com_Printf
	//   which leads to client overflows
	//   see show_bug.cgi?id=51
	//     (also a Q3 issue)
	Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!strlen(sv_rconPassword->string))
	{
		Com_Printf("No rconpassword set on the server.\n");
	}
	else if (!valid)
	{
		Com_Printf("Bad rconpassword.\n");
	}
	else
	{
		remaining[0] = 0;

		// get the command directly, "rcon <pass> <command>" to avoid quoting issues
		// extract the command by walking
		// since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
		cmd_aux  = Cmd_Cmd();
		cmd_aux += 4;
		while (cmd_aux[0] == ' ')
			cmd_aux++;
		while (cmd_aux[0] && cmd_aux[0] != ' ')   // password
			cmd_aux++;
		while (cmd_aux[0] == ' ')
			cmd_aux++;

		Q_strcat(remaining, sizeof(remaining), cmd_aux);

		Cmd_ExecuteString(remaining);

	}

	Com_EndRedirect();
}

/*
 * A connectionless packet has four leading 0xff characters to distinguish it
 * from a game channel.
 * Clients that are in the game can still send connectionless packets.
 */
static void SV_ConnectionlessPacket(netadr_t from, msg_t *msg)
{
	char *s;
	char *c;

	MSG_BeginReadingOOB(msg);
	MSG_ReadLong(msg);          // skip the -1 marker

	if (!Q_strncmp("connect", (char *)&msg->data[4], 7))
	{
		Huff_Decompress(msg, 12);
	}

	s = MSG_ReadStringLine(msg);

	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);
	Com_DPrintf("SV packet %s : %s\n", NET_AdrToString(from), c);

	if (!Q_stricmp(c, "getstatus"))
	{
		SVC_Status(from);
	}
	else if (!Q_stricmp(c, "getinfo"))
	{
		SVC_Info(from);
	}
	else if (!Q_stricmp(c, "getchallenge"))
	{
		SV_GetChallenge(from);
	}
	else if (!Q_stricmp(c, "connect"))
	{
		SV_DirectConnect(from);
	}
	else if (!Q_stricmp(c, "rcon"))
	{
		SVC_RemoteCommand(from, msg);
	}
	else if (!Q_stricmp(c, "disconnect"))
	{
		// if a client starts up a local server, we may see some spurious
		// server disconnect messages when their new server sees our final
		// sequenced messages to the old client
	}
	else
	{
		Com_DPrintf("bad connectionless packet from %s:\n%s\n"
		            , NET_AdrToString(from), s);
	}
}

void SV_PacketEvent(netadr_t from, msg_t *msg)
{
	int      i;
	client_t *cl;
	int      qport;

	// check for connectionless packet (0xffffffff) first
	if (msg->cursize >= 4 && *(int *)msg->data == -1)
	{
		SV_ConnectionlessPacket(from, msg);
		return;
	}

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	MSG_BeginReadingOOB(msg);
	MSG_ReadLong(msg);                  // sequence number
	qport = MSG_ReadShort(msg) & 0xffff;

	// find which client the message is from
	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (!NET_CompareBaseAdr(from, cl->netchan.remoteAddress))
		{
			continue;
		}
		// it is possible to have multiple clients from a single IP
		// address, so they are differentiated by the qport variable
		if (cl->netchan.qport != qport)
		{
			continue;
		}

		// the IP port can't be used to differentiate them, because
		// some address translating routers periodically change UDP
		// port assignments
		if (cl->netchan.remoteAddress.port != from.port)
		{
			Com_Printf("SV_PacketEvent: fixing up a translated port\n");
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if (SV_Netchan_Process(cl, msg))
		{
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if (cl->state != CS_ZOMBIE)
			{
				cl->lastPacketTime = svs.time;  // don't timeout
				SV_ExecuteClientMessage(cl, msg);
			}
		}
		return;
	}

	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	NET_OutOfBandPrint(NS_SERVER, from, "disconnect");
}


/*
 * @brief Updates the cl->ping variables
 */
static void SV_CalcPings(void)
{
	int           i, j;
	client_t      *cl;
	int           total, count;
	int           delta;
	playerState_t *ps;

	for (i = 0 ; i < sv_maxclients->integer ; i++)
	{
		cl = &svs.clients[i];

		if (cl->state != CS_ACTIVE)
		{
			cl->ping = 999;
			continue;
		}
		if (!cl->gentity)
		{
			cl->ping = 999;
			continue;
		}
		if (cl->gentity->r.svFlags & SVF_BOT)
		{
			cl->ping = 0;
			continue;
		}

		total = 0;
		count = 0;
		for (j = 0 ; j < PACKET_BACKUP ; j++)
		{
			if (cl->frames[j].messageAcked <= 0)
			{
				continue;
			}
			delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
			count++;
			total += delta;
		}
		if (!count)
		{
			cl->ping = 999;
		}
		else
		{
			cl->ping = total / count;
			if (cl->ping > 999)
			{
				cl->ping = 999;
			}
		}

		// let the game dll know about the ping
		ps       = SV_GameClientNum(i);
		ps->ping = cl->ping;
	}
}

/*
 * @brief If a packet has not been received from a client for timeout->integer
 * seconds, drop the conneciton.
 *
 * Server time is used instead of realtime to avoid dropping the local client
 * while debugging.
 *
 * When a client is normally dropped, the client_t goes into a zombie state
 * for a few seconds to make sure any final reliable message gets resent
 * if necessary.
 */
static void SV_CheckTimeouts(void)
{
	int      i;
	client_t *cl;
	int      droppoint;
	int      zombiepoint;

	droppoint   = svs.time - 1000 * sv_timeout->integer;
	zombiepoint = svs.time - 1000 * sv_zombietime->integer;

	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->lastPacketTime > svs.time)
		{
			cl->lastPacketTime = svs.time;
		}

		if (cl->state == CS_ZOMBIE && cl->lastPacketTime < zombiepoint)
		{
			// using the client id cause the cl->name is empty at this point
			Com_DPrintf("Going from CS_ZOMBIE to CS_FREE for client %d\n", i);
			cl->state = CS_FREE;    // can now be reused

			continue;
		}
		if (cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint)
		{
			// wait several frames so a debugger session doesn't
			// cause a timeout
			if (++cl->timeoutCount > 5)
			{
				SV_DropClient(cl, "timed out");
				cl->state = CS_FREE;    // don't bother with zombie state
			}
		}
		else
		{
			cl->timeoutCount = 0;
		}
	}
}

static qboolean SV_CheckPaused(void)
{
	int      count = 0;
	client_t *cl;
	int      i;

	if (!cl_paused->integer)
	{
		return qfalse;
	}

	// only pause if there is just a single client connected
	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		if (cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT)
		{
			count++;
		}
	}

	if (count > 1)
	{
		// don't pause
		if (sv_paused->integer)
		{
			Cvar_Set("sv_paused", "0");
		}
		return qfalse;
	}

	if (!sv_paused->integer)
	{
		Cvar_Set("sv_paused", "1");
	}
	return qtrue;
}

/*
 * @brief Return time in millseconds until processing of the next server frame.
 */
int SV_FrameMsec()
{
	if (sv_fps)
	{
		int frameMsec;

		frameMsec = 1000.0f / sv_fps->value;

		if (frameMsec < sv.timeResidual)
		{
			return 0;
		}
		else
		{
			return frameMsec - sv.timeResidual;
		}
	}
	else
	{
		return 1;
	}
}

/*
 * @brief Player movement occurs as a result of packet events, which happen
 * before SV_Frame is called
 */
void SV_Frame(int msec)
{
	int  frameMsec;
	int  startTime;
	char mapname[MAX_QPATH];
	int  frameStartTime = 0, frameEndTime;

	// the menu kills the server with this cvar
	if (sv_killserver->integer)
	{
		SV_Shutdown("Server was killed.\n");
		Cvar_Set("sv_killserver", "0");
		return;
	}

	if (!com_sv_running->integer)
	{
		return;
	}

	// allow pause if only the local client is connected
	if (SV_CheckPaused())
	{
		return;
	}

	if (com_dedicated->integer)
	{
		frameStartTime = Sys_Milliseconds();
	}

	// if it isn't time for the next frame, do nothing
	if (sv_fps->integer < 1)
	{
		Cvar_Set("sv_fps", "10");
	}
	frameMsec = 1000 / sv_fps->integer ;

	sv.timeResidual += msec;

	if (com_dedicated->integer && sv.timeResidual < frameMsec)
	{
		// NET_Sleep will give the OS time slices until either get a packet
		// or time enough for a server frame has gone by
		NET_Sleep(frameMsec - sv.timeResidual);
		return;
	}

	// if time is about to hit the 32nd bit, kick all clients
	// and clear sv.time, rather
	// than checking for negative time wraparound everywhere.
	// 2giga-milliseconds = 23 days, so it won't be too often
	if (svs.time > 0x70000000)
	{
		Q_strncpyz(mapname, sv_mapname->string, MAX_QPATH);
		SV_Shutdown("Restarting server due to time wrapping");
		// TTimo
		// show_bug.cgi?id=388
		// there won't be a map_restart if you have shut down the server
		// since it doesn't restart a non-running server
		// instead, re-run the current map
		Cbuf_AddText(va("map %s\n", mapname));
		return;
	}
	// this can happen considerably earlier when lots of clients play and the map doesn't change
	if (svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities)
	{
		Q_strncpyz(mapname, sv_mapname->string, MAX_QPATH);
		SV_Shutdown("Restarting server due to numSnapshotEntities wrapping");
		// TTimo see above
		Cbuf_AddText(va("map %s\n", mapname));
		return;
	}

	if (sv.restartTime && svs.time >= sv.restartTime)
	{
		sv.restartTime = 0;
		Cbuf_AddText("map_restart 0\n");
		return;
	}

	// update infostrings if anything has been changed
	if (cvar_modifiedFlags & CVAR_SERVERINFO)
	{
		SV_SetConfigstring(CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if (cvar_modifiedFlags & CVAR_SERVERINFO_NOUPDATE)
	{
		SV_SetConfigstringNoUpdate(CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO_NOUPDATE;
	}
	if (cvar_modifiedFlags & CVAR_SYSTEMINFO)
	{
		SV_SetConfigstring(CS_SYSTEMINFO, Cvar_InfoString_Big(CVAR_SYSTEMINFO));
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}
	if (cvar_modifiedFlags & CVAR_WOLFINFO)
	{
		SV_SetConfigstring(CS_WOLFINFO, Cvar_InfoString(CVAR_WOLFINFO));
		cvar_modifiedFlags &= ~CVAR_WOLFINFO;
	}

	if (com_speeds->integer)
	{
		startTime = Sys_Milliseconds();
	}
	else
	{
		startTime = 0;  // quite a compiler warning
	}

	// update ping based on the all received frames
	SV_CalcPings();

	// run the game simulation in chunks
	while (sv.timeResidual >= frameMsec)
	{
		sv.timeResidual -= frameMsec;
		svs.time        += frameMsec;

		// let everything in the world think and move
		VM_Call(gvm, GAME_RUN_FRAME, svs.time);
	}

	if (com_speeds->integer)
	{
		time_game = Sys_Milliseconds() - startTime;
	}

	// check timeouts
	SV_CheckTimeouts();

	// check user info buffer thingy
	SV_CheckClientUserinfoTimer();

	// send messages back to the clients
	SV_SendClientMessages();

	// send a heartbeat to the master if needed
	SV_MasterHeartbeat(HEARTBEAT_GAME);

	if (com_dedicated->integer)
	{
		frameEndTime = Sys_Milliseconds();

		svs.totalFrameTime += (frameEndTime - frameStartTime);
		svs.currentFrameIndex++;

		//if( svs.currentFrameIndex % 50 == 0 )
		//	Com_Printf( "currentFrameIndex: %i\n", svs.currentFrameIndex );

		if (svs.currentFrameIndex == SERVER_PERFORMANCECOUNTER_FRAMES)
		{
			int averageFrameTime;

			averageFrameTime = svs.totalFrameTime / SERVER_PERFORMANCECOUNTER_FRAMES;

			svs.sampleTimes[svs.currentSampleIndex % SERVER_PERFORMANCECOUNTER_SAMPLES] = averageFrameTime;
			svs.currentSampleIndex++;

			if (svs.currentSampleIndex > SERVER_PERFORMANCECOUNTER_SAMPLES)
			{
				int totalTime, i;

				totalTime = 0;
				for (i = 0; i < SERVER_PERFORMANCECOUNTER_SAMPLES; i++)
				{
					totalTime += svs.sampleTimes[i];
				}

				if (!totalTime)
				{
					totalTime = 1;
				}

				averageFrameTime = totalTime / SERVER_PERFORMANCECOUNTER_SAMPLES;

				svs.serverLoad = (averageFrameTime / (float)frameMsec) * 100;
			}

			//Com_Printf( "serverload: %i (%i/%i)\n", svs.serverLoad, averageFrameTime, frameMsec );

			svs.totalFrameTime    = 0;
			svs.currentFrameIndex = 0;
		}
	}
	else
	{
		svs.serverLoad = -1;
	}
}

int SV_LoadTag(const char *mod_name)
{
	unsigned char *buffer;
	tagHeader_t   *pinmodel;
	int           version;
	md3Tag_t      *tag;
	md3Tag_t      *readTag;
	int           i, j;

	for (i = 0; i < sv.num_tagheaders; i++)
	{
		if (!Q_stricmp(mod_name, sv.tagHeadersExt[i].filename))
		{
			return i + 1;
		}
	}

	FS_ReadFile(mod_name, (void **)&buffer);

	if (!buffer)
	{
		return qfalse;
	}

	pinmodel = (tagHeader_t *)buffer;

	version = LittleLong(pinmodel->version);
	if (version != TAG_VERSION)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: SV_LoadTag: %s has wrong version (%i should be %i)\n", mod_name, version, TAG_VERSION);
		return 0;
	}

	if (sv.num_tagheaders >= MAX_TAG_FILES)
	{
		Com_Error(ERR_DROP, "MAX_TAG_FILES reached\n");

		FS_FreeFile(buffer);
		return 0;
	}

	LL(pinmodel->ident);
	LL(pinmodel->numTags);
	LL(pinmodel->ofsEnd);
	LL(pinmodel->version);

	Q_strncpyz(sv.tagHeadersExt[sv.num_tagheaders].filename, mod_name, MAX_QPATH);
	sv.tagHeadersExt[sv.num_tagheaders].start = sv.num_tags;
	sv.tagHeadersExt[sv.num_tagheaders].count = pinmodel->numTags;

	if (sv.num_tags + pinmodel->numTags >= MAX_SERVER_TAGS)
	{
		Com_Error(ERR_DROP, "MAX_SERVER_TAGS reached\n");

		FS_FreeFile(buffer);
		return qfalse;
	}

	// swap all the tags
	tag          = &sv.tags[sv.num_tags];
	sv.num_tags += pinmodel->numTags;
	readTag      = ( md3Tag_t * )(buffer + sizeof(tagHeader_t));
	for (i = 0 ; i < pinmodel->numTags; i++, tag++, readTag++)
	{
		for (j = 0 ; j < 3 ; j++)
		{
			tag->origin[j]  = LittleFloat(readTag->origin[j]);
			tag->axis[0][j] = LittleFloat(readTag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(readTag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(readTag->axis[2][j]);
		}
		Q_strncpyz(tag->name, readTag->name, 64);
	}

	FS_FreeFile(buffer);
	return ++sv.num_tagheaders;
}
