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
 * @file sv_main.c
 */

#include "server.h"

serverStatic_t svs;             // persistant server info
server_t       sv;              // local server
vm_t           *gvm = NULL;     // game virtual machine

cvar_t *sv_fps = NULL;          // time rate for running non-clients
cvar_t *sv_timeout;             // seconds without any message
cvar_t *sv_zombietime;          // seconds to sink messages after disconnect
cvar_t *sv_rconPassword;        // password for remote server commands
cvar_t *sv_privatePassword;     // password for the privateClient slots
cvar_t *sv_hidden;
cvar_t *sv_allowDownload;
cvar_t *sv_maxclients;
cvar_t *sv_democlients;         // number of slots reserved for playing a demo

cvar_t *sv_privateClients;      // number of clients reserved for password
cvar_t *sv_hostname;
cvar_t *sv_reconnectlimit;      // minimum seconds between connect messages
cvar_t *sv_tempbanmessage;

cvar_t *sv_padPackets;          // add nop bytes to messages
cvar_t *sv_killserver;          // menu system can set to 1 to shut server down
cvar_t *sv_mapname;
cvar_t *sv_mapChecksum;
cvar_t *sv_serverid;
cvar_t *sv_minRate;
cvar_t *sv_maxRate;
cvar_t *sv_minPing;
cvar_t *sv_maxPing;
cvar_t *sv_gametype;
cvar_t *sv_pure;
cvar_t *sv_floodProtect;
cvar_t *sv_lanForceRate;        // dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
cvar_t *sv_onlyVisibleClients;
cvar_t *sv_friendlyFire;
cvar_t *sv_maxlives;
cvar_t *sv_needpass;

cvar_t *sv_dl_timeout;          // seconds without any message when cl->state != CS_ACTIVE

cvar_t *sv_showAverageBPS;      // net debugging

cvar_t *sv_wwwDownload;         // server does a www dl redirect
cvar_t *sv_wwwBaseURL;          // base URL for redirect
// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
cvar_t *sv_wwwDlDisconnected;
cvar_t *sv_wwwFallbackURL; // URL to send to if an http/ftp fails or is refused client side

cvar_t *sv_cheats;
cvar_t *sv_packetloss;
cvar_t *sv_packetdelay;

cvar_t *sv_fullmsg;

cvar_t *sv_dlRate;

// do we communicate with others ?
cvar_t *sv_advert;      // 0 - no big brothers
                        // 1 - communicate with master server
                        // 2 - communicate with tracker

// server attack protection
cvar_t *sv_protect;     // 0 - unprotected
                        // 1 - ioquake3 method (default)
                        // 2 - OpenWolf method
                        // 4 - prints attack info to console (when ioquake3 or OPenWolf method is set)
cvar_t *sv_protectLog;  // name of log file

#ifdef FEATURE_ANTICHEAT
cvar_t *sv_wh_active;
cvar_t *sv_wh_bbox_horz;
cvar_t *sv_wh_bbox_vert;
cvar_t *sv_wh_check_fov;
#endif

cvar_t *sv_demopath;
cvar_t *sv_demoState;
cvar_t *sv_autoDemo;
cvar_t *sv_freezeDemo;  // to freeze server-side demos
cvar_t *sv_demoTolerant;

cvar_t *sv_ipMaxClients;

cvar_t *sv_guidCheck;

static void SVC_Status(netadr_t from, qboolean force);

/*
=============================================================================
EVENT MESSAGES
=============================================================================
*/

/**
 * @brief Converts newlines to "\\n" so a line prints nicer
 * @param in
 * @return
 */
static char *SV_ExpandNewlines(char *in)
{
	static char string[1024];
	size_t      l = 0;

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

/**
 * @brief The given command will be transmitted to the client, and is guaranteed
 * to not have future snapshot_t executed before it is executed
 *
 * @param[in,out] client
 * @param[in] cmd
 */
void SV_AddServerCommand(client_t *client, const char *cmd)
{
	int index;

	client->reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SV_DropClient()
	// doesn't cause a recursive drop client
	if (client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1)
	{
		int i;

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

/**
 * @brief Sends a reliable command string to be interpreted by the client game
 * module: "cp", "print", "chat", etc
 *
 * A NULL client will broadcast to all clients
 *
 * @param[in] cl
 * @param[in] fmt
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
		SV_WriteAttackLog("Warning: q3infoboom/q3msgboom exploit attack.\n"); // FIXME: add client slot
		return;
	}

	if (cl != NULL)
	{
		// FIXME: enable this and sort unwanted single client commands in SV_CheckServerCommand
		//if (sv.demoState == DS_RECORDING)
		//{
		//	SV_DemoWriteServerCommand((char *)message);
		//}
		SV_AddServerCommand(cl, (char *)message);
		return;
	}

	// hack to echo broadcast prints to console
	if (com_dedicated->integer && !strncmp((char *)message, "print", 5))
	{
		Com_Printf("broadcast: %s\n", SV_ExpandNewlines((char *)message));
	}

	// save broadcasts to demo
	// note: in the case a command is only issued to a specific client, it is NOT recorded (see above when cl != NULL).
	// If you want to record them, just place this code above, but be warned that it may be dangerous (such as "disconnect" command)
	// because server commands will be replayed to every connected clients!
	//
	// NOTE: SV_CheckServerCommand in SV_CheckServerCommand should sort unwanted commands ...
	if (sv.demoState == DS_RECORDING)
	{
		SV_DemoWriteServerCommand((char *)message);
	}

	// send the data to all relevant clients
	for (j = 0, client = svs.clients; j < sv_maxclients->integer ; j++, client++)
	{
		if (client->state < CS_PRIMED)
		{
			continue;
		}
		// don't need to send messages to AI
		if (client->gentity && (client->gentity->r.svFlags & SVF_BOT))
		{
			continue;
		}

		SV_AddServerCommand(client, (char *)message);
	}
}

/*
==============================================================================
MASTER SERVER FUNCTIONS
==============================================================================
*/

#define HEARTBEAT_MSEC  300 * 1000
#define HEARTBEAT_GAME  "EnemyTerritory-1"
#define HEARTBEAT_DEAD  "ETFlatline-1"

/**
 * @brief Send a message to the masters every few minutes to let it know we are
 * alive, and log information.
 *
 * We will also have a heartbeat sent when a server changes from empty to
 * non-empty, and full to non-full, but not on every player enter or exit.
 *
 * @param[in] msg
 */
void SV_MasterHeartbeat(const char *msg)
{
	static netadr_t adr[MAX_MASTER_SERVERS][2]; // [2] for v4 and v6 address for the same address string.
	int             i;
	int             res;
	int             netenabled;
	char            *master;

	if (!(sv_advert->integer & SVA_MASTER) || sv_hidden->integer)
	{
		return;
	}

	netenabled = Cvar_VariableIntegerValue("net_enabled");

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if (!com_dedicated || com_dedicated->integer != 2 || !(netenabled & (
#ifdef FEATURE_IPV6
	                                                           NET_ENABLEV6 |
#endif
	                                                           NET_ENABLEV4)))
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
		master = Cvar_VariableString(va("sv_master%i", i + 1));
		if (master[0] == '\0')
		{
			continue;
		}

		// see if we haven't already resolved the name
		if (netenabled & NET_ENABLEV4)
		{
			if (adr[i][0].type == NA_BAD)
			{
				Com_Printf("Resolving %s (IPv4)\n", master);
				res = NET_StringToAdr(master, &adr[i][0], NA_IP);

				if (res == 2)
				{
					// if no port was specified, use the default master port
					adr[i][0].port = BigShort(PORT_MASTER);
				}

				if (res)
				{
					Com_Printf("%s resolved to %s\n", master, NET_AdrToString(adr[i][0]));
				}
				else
				{
					Com_Printf("%s has no IPv4 address\n", master);
				}
			}
		}

#ifdef FEATURE_IPV6
		if (netenabled & NET_ENABLEV6)
		{
			if (adr[i][1].type == NA_BAD)
			{
				Com_Printf("Resolving %s (IPv6)\n", master);
				res = NET_StringToAdr(master, &adr[i][1], NA_IP6);

				if (res == 2)
				{
					// if no port was specified, use the default master port
					adr[i][1].port = BigShort(PORT_MASTER);
				}

				if (res)
				{
					Com_Printf("%s resolved to %s\n", master, NET_AdrToString(adr[i][1]));
				}
				else
				{
					Com_Printf("%s has no IPv6 address\n", master);
				}
			}
		}
#endif

		if ((((netenabled & NET_ENABLEV4) && adr[i][0].type == NA_BAD) || !(netenabled & NET_ENABLEV4))
		    && (((netenabled & NET_ENABLEV6) && adr[i][1].type == NA_BAD) || !(netenabled & NET_ENABLEV6)))
		{
			// if the address failed to resolve, clear it
			// so we don't take repeated dns hits
			Com_Printf("Couldn't resolve address: %s\n", master);
			Cvar_Set(va("sv_master%i", i + 1), "");
			continue;
		}

		Com_Printf("Sending heartbeat to %s\n", master);

		// this command should be changed if the server info / status format
		// ever incompatably changes

		if ((netenabled & NET_ENABLEV4) && adr[i][0].type != NA_BAD)
		{
			NET_OutOfBandPrint(NS_SERVER, adr[i][0], "heartbeat %s\n", msg);
		}

#ifdef FEATURE_IPV6
		if (netenabled & NET_ENABLEV6 && adr[i][1].type != NA_BAD)
		{
			NET_OutOfBandPrint(NS_SERVER, adr[i][1], "heartbeat %s\n", msg);
		}
#endif
	}
}

/**
 * @brief Sends gameCompleteStatus messages to all master servers
 */
void SV_MasterGameCompleteStatus()
{
	static netadr_t adr[MAX_MASTER_SERVERS][2]; // [2] for v4 and v6 address for the same address string.
	int             i;
	int             res;
	int             netenabled;
	char            *master;

	if (!(sv_advert->integer & SVA_MASTER))
	{
		return;
	}

	netenabled = Cvar_VariableIntegerValue("net_enabled");

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if (!com_dedicated || com_dedicated->integer != 2 || !(netenabled & (
#ifdef FEATURE_IPV6
	                                                           NET_ENABLEV6 |
#endif
	                                                           NET_ENABLEV4)))
	{
		return;     // only dedicated servers send heartbeats

	}

	// send to group masters
	for (i = 0; i < MAX_MASTER_SERVERS; i++)
	{
		master = Cvar_VariableString(va("sv_master%i", i + 1));
		if (master[0] == '\0')
		{
			continue;
		}

		// see if we haven't already resolved the name
		if (netenabled & NET_ENABLEV4)
		{
			if (adr[i][0].type == NA_BAD)
			{
				Com_Printf("Resolving %s (IPv4)\n", master);
				res = NET_StringToAdr(master, &adr[i][0], NA_IP);

				if (res == 2)
				{
					// if no port was specified, use the default master port
					adr[i][0].port = BigShort(PORT_MASTER);
				}

				if (res)
				{
					Com_Printf("%s resolved to %s\n", master, NET_AdrToString(adr[i][0]));
				}
				else
				{
					Com_Printf("%s has no IPv4 address\n", master);
				}
			}
		}

#ifdef FEATURE_IPV6
		if (netenabled & NET_ENABLEV6)
		{
			if (adr[i][1].type == NA_BAD)
			{
				Com_Printf("Resolving %s (IPv6)\n", master);
				res = NET_StringToAdr(master, &adr[i][1], NA_IP6);

				if (res == 2)
				{
					// if no port was specified, use the default master port
					adr[i][1].port = BigShort(PORT_MASTER);
				}

				if (res)
				{
					Com_Printf("%s resolved to %s\n", master, NET_AdrToString(adr[i][1]));
				}
				else
				{
					Com_Printf("%s has no IPv6 address\n", master);
				}
			}
		}
#endif

		if ((((netenabled & NET_ENABLEV4) && adr[i][0].type == NA_BAD) || !(netenabled & NET_ENABLEV4))
		    && (((netenabled & NET_ENABLEV6) && adr[i][1].type == NA_BAD) || !(netenabled & NET_ENABLEV6)))
		{
			// if the address failed to resolve, clear it
			// so we don't take repeated dns hits
			Com_Printf("Couldn't resolve address: %s\n", master);
			Cvar_Set(va("sv_master%i", i + 1), "");
			continue;
		}

		Com_Printf("Sending gameCompleteStatus to %s\n", master);

		// this command should be changed if the server info / status format
		// ever incompatably changes

		if ((netenabled & NET_ENABLEV4) && adr[i][0].type != NA_BAD)
		{
			SVC_Status(adr[i][0], qtrue);
		}

#ifdef FEATURE_IPV6
		if (netenabled & NET_ENABLEV6 && adr[i][1].type != NA_BAD)
		{
			SVC_Status(adr[i][1], qtrue);
		}
#endif
	}
}

/**
 * @brief Informs all masters that this server is going down
 */
void SV_MasterShutdown(void)
{
	svs.nextHeartbeatTime = -9999; // send a hearbeat right now
	SV_MasterHeartbeat(HEARTBEAT_DEAD);

	// when the master tries to poll the server, it won't respond, so
	// it will be removed from the list
}

/*
==============================================================================
CONNECTIONLESS COMMANDS
==============================================================================
*/

static leakyBucket_t buckets[MAX_BUCKETS];
static leakyBucket_t *bucketHashes[MAX_HASHES];
leakyBucket_t        outboundLeakyBucket;

/**
 * @brief SVC_HashForAddress
 * @param[in] address
 * @return
 */
static long SVC_HashForAddress(netadr_t address)
{
	byte         *ip  = NULL;
	size_t       size = 0;
	unsigned int i;
	long         hash = 0;

	switch (address.type)
	{
	case NA_IP:  ip = address.ip;  size = 4;
		break;
	case NA_IP6: ip = address.ip6; size = 16;
		break;
	default:
		break;
	}

	if (!ip)
	{
		Com_Printf("SVC_HashForAddress: Invalid IP - hash value is 0\n");
		return 0;
	}

	for (i = 0; i < size; i++)
	{
		hash += (long)((ip[i]) * (i + 119));
	}

	hash  = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (MAX_HASHES - 1);

	return hash;
}

/**
 * @brief Find or allocate a bucket for an address
 * @param[in] address
 * @param[in] burst
 * @param[in] period
 * @return
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

	// make sure we will never use time 0
	now = now ? now : 1;

	for (i = 0; i < MAX_BUCKETS; i++)
	{
		int interval;

		bucket   = &buckets[i];
		interval = now - bucket->lastTime;

		// Reclaim expired buckets
		if (bucket->lastTime > 0 && (unsigned) interval > (burst * period))
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
			case NA_IP:  Com_Memcpy(bucket->ipv._4, address.ip, 4);
				break;
			case NA_IP6: Com_Memcpy(bucket->ipv._6, address.ip6, 16);
				break;
			default:
				break;
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
	// Write the info to the attack log since this is relevant information as the system is malfunctioning
	SV_WriteAttackLogD(va("SVC_BucketForAddress: Could not allocate a bucket for client from %s\n", NET_AdrToString(address)));

	return NULL;
}

/**
 * @brief SVC_RateLimit
 * @param[in,out] bucket
 * @param[in] burst
 * @param[in] period
 * @return
 *
 * @note Don't call if sv_protect 1 (SVP_IOQ3) flag is not set!
 */
qboolean SVC_RateLimit(leakyBucket_t *bucket, int burst, int period)
{
	if (bucket != NULL)
	{
		int now              = Sys_Milliseconds();
		int interval         = now - bucket->lastTime;
		int expired          = interval / period;
		int expiredRemainder = interval % period;

		if (expired > bucket->burst || interval < 0)
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
		else
		{
			SV_WriteAttackLogD(va("SVC_RateLimit: burst limit exceeded for bucket: %i limit: %i\n", bucket->burst, burst));
		}
	}

	return qtrue;
}

/**
 * @brief Rate limit for a particular address
 * @param from
 * @param burst
 * @param period
 * @return
 *
 * @note  Don't call if sv_protect 1 (SVP_IOQ3) flag is not set!
 */
qboolean SVC_RateLimitAddress(netadr_t from, int burst, int period)
{
	leakyBucket_t *bucket = SVC_BucketForAddress(from, burst, period);

	return SVC_RateLimit(bucket, burst, period);
}

/**
 * @brief Send serverinfo cvars, etc to master servers when game complete or
 * by request of getstatus calls.
 *
 * Useful for tracking global player stats.
 *
 * @param[in] from
 * @param[in] force toggle rate limit checks
 */
static void SVC_Status(netadr_t from, qboolean force)
{
	char          player[1024];
	char          status[MAX_MSGLEN];
	int           i;
	client_t      *cl;
	playerState_t *ps;
	unsigned int  statusLength;
	unsigned int  playerLength;
	char          infostring[MAX_INFO_STRING];

	if (!force && (sv_protect->integer & SVP_IOQ3))
	{
		// Prevent using getstatus as an amplifier
		if (SVC_RateLimitAddress(from, 10, 1000))
		{
			SV_WriteAttackLog(va("SVC_Status: rate limit from %s exceeded, dropping request\n",
			                     NET_AdrToString(from)));
			return;
		}

		// Allow getstatus to be DoSed relatively easily, but prevent
		// excess outbound bandwidth usage when being flooded inbound
		if (SVC_RateLimit(&outboundLeakyBucket, 10, 100))
		{
			SV_WriteAttackLog("SVC_Status: rate limit exceeded, dropping request\n");
			return;
		}
	}

	// A maximum challenge length of 128 should be more than plenty.
	if (strlen(Cmd_Argv(1)) > 128)
	{
		SV_WriteAttackLog(va("SVC_Status: challenge length exceeded from %s, dropping request\n", NET_AdrToString(from)));
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

/**
 * @brief Responds with a short info message that should be enough to determine
 * if a user is interested in a server to do a full status
 *
 * @param[in] from
 */
void SVC_Info(netadr_t from)
{
	int  i, clients = 0, humans = 0;
	char *gamedir;
	char infostring[MAX_INFO_STRING];
	char *antilag;
	char *weaprestrict;
	char *balancedteams;

	if (sv_protect->integer & SVP_IOQ3)
	{
		// Prevent using getinfo as an amplifier
		if (SVC_RateLimitAddress(from, 10, 1000))
		{
			SV_WriteAttackLog(va("SVC_Info: rate limit from %s exceeded, dropping request\n",
			                     NET_AdrToString(from)));
			return;
		}

		// Allow getinfo to be DoSed relatively easily, but prevent
		// excess outbound bandwidth usage when being flooded inbound
		if (SVC_RateLimit(&outboundLeakyBucket, 10, 100))
		{
			SV_WriteAttackLog("SVC_Info: rate limit exceeded, dropping request\n");
			return;
		}
	}

	// Check whether Cmd_Argv(1) has a sane length. This was not done in the original Quake3 version which led
	// to the Infostring bug discovered by Luigi Auriemma. See http://aluigi.altervista.org/ for the advisory.
	// A maximum challenge length of 128 should be more than plenty.
	if (strlen(Cmd_Argv(1)) > 128)
	{
		SV_WriteAttackLog(va("SVC_Info: challenge length from %s exceeded, dropping request\n", NET_AdrToString(from)));
		return;
	}

	// count private clients too
	for (i = 0 ; i < sv_maxclients->integer ; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			clients++;
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

	Info_SetValueForKey(infostring, "version", ET_VERSION);
	Info_SetValueForKey(infostring, "protocol", va("%i", PROTOCOL_VERSION));
	Info_SetValueForKey(infostring, "hostname", sv_hostname->string);
	Info_SetValueForKey(infostring, "serverload", va("%i", svs.serverLoad));
	Info_SetValueForKey(infostring, "mapname", sv_mapname->string);
	Info_SetValueForKey(infostring, "clients", va("%i", clients));
	Info_SetValueForKey(infostring, "humans", va("%i", humans));
	Info_SetValueForKey(infostring, "sv_maxclients", va("%i", sv_maxclients->integer - sv_privateClients->integer - sv_democlients->integer));
	Info_SetValueForKey(infostring, "sv_privateclients", va("%i", sv_privateClients->integer));
	Info_SetValueForKey(infostring, "gametype", va("%i", sv_gametype->integer));
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

	Info_SetValueForKey(infostring, "friendlyFire", va("%i", sv_friendlyFire->integer));
	Info_SetValueForKey(infostring, "maxlives", va("%i", sv_maxlives->integer ? 1 : 0));
	Info_SetValueForKey(infostring, "needpass", va("%i", sv_needpass->integer ? 1 : 0));
	Info_SetValueForKey(infostring, "gamename", GAMENAME_STRING);

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

/**
 * @brief SV_FlushRedirect
 * @param[in] outputbuf
 */
static void SV_FlushRedirect(char *outputbuf)
{
	NET_OutOfBandPrint(NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf);
}

/**
 * @brief DRDoS stands for "Distributed Reflected Denial of Service".
 * See here: http://www.lemuria.org/security/application-drdos.html
 *
 * If the address isn't NA_IP, it's automatically denied.
 *
 * @return qfalse if we're good.
 * otherwise qtrue means we need to block.
 *
 * @note Don't call this if sv_protect 2 flag is not set!
 */
qboolean SV_CheckDRDoS(netadr_t from)
{
	int        i;
	int        globalCount;
	int        specificCount;
	int        timeNow;
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

	timeNow   = svs.time;
	exactFrom = from;

	// Time has wrapped
	if (lastGlobalLogTime > timeNow || lastSpecificLogTime > timeNow)
	{
		lastGlobalLogTime   = 0;
		lastSpecificLogTime = 0;

		// just setting time to 1 (cannot be 0 as then globalCount would not be counted)
		for (i = 0; i < MAX_INFO_RECEIPTS; i++)
		{
			if (svs.infoReceipts[i].time)
			{
				svs.infoReceipts[i].time = 1; // hack it so we count globalCount correctly
			}
		}
	}

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
		if (receipt->time + 2000 > timeNow)
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
		if (lastGlobalLogTime + 1000 <= timeNow)  // Limit one log every second.
		{
			SV_WriteAttackLog("Detected flood of getinfo/getstatus connectionless packets\n");
			lastGlobalLogTime = timeNow;
		}

		return qtrue;
	}
	if (specificCount >= 3)   // Already sent 3 to this IP in last 2 seconds.
	{
		if (lastSpecificLogTime + 1000 <= timeNow)   // Limit one log every second.
		{
			SV_WriteAttackLog(va("Possible DRDoS attack to address %s, ignoring getinfo/getstatus connectionless packet\n",
			                     NET_AdrToString(exactFrom)));
			lastSpecificLogTime = timeNow;
		}

		return qtrue;
	}

	receipt       = &svs.infoReceipts[oldest];
	receipt->adr  = from;
	receipt->time = timeNow;
	return qfalse;
}

/**
 * @brief An rcon packet arrived from the network.
 *
 * Shift down the remaining args. Redirect all printfs.
 *
 * @param[in] from
 * @param msg - unused
 */
static void SVC_RemoteCommand(netadr_t from, msg_t *msg)
{
	qboolean valid;
	char     remaining[1024];
	// if we send an OOB print message this size, 1.31 clients die in a Com_Printf buffer overflow
	// the buffer overflow will be fixed in > 1.31 clients
	// but we want a server side fix
	// we must NEVER send an OOB message that will be > 1.31 MAX_PRINT_MSG (4096)
#define SV_OUTPUTBUF_LENGTH (256 - 16)
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	char *cmd_aux;

	// Prevent using rcon as an amplifier and make dictionary attacks impractical
	if ((sv_protect->integer & SVP_IOQ3) && SVC_RateLimitAddress(from, 10, 1000))
	{
		SV_WriteAttackLog(va("Bad rcon - rate limit from %s exceeded, dropping request\n",
		                     NET_AdrToString(from)));
		return;
	}

	if (!strlen(sv_rconPassword->string) ||
	    strcmp(Cmd_Argv(1), sv_rconPassword->string))
	{
		static leakyBucket_t bucket;

		// Make DoS via rcon impractical
		if ((sv_protect->integer & SVP_IOQ3) && SVC_RateLimit(&bucket, 10, 1000))
		{
			SV_WriteAttackLog("Bad rcon - rate limit exceeded, dropping request\n");
			return;
		}

		valid = qfalse;
	}
	else
	{
		valid = qtrue;

		Com_Printf("Rcon from %s: %s\n", NET_AdrToString(from), Cmd_Argv(2));
		SV_WriteAttackLog(va("Rcon from %s: %s\n", NET_AdrToString(from), Cmd_Argv(2)));
	}

	// start redirecting all print outputs to the packet
	svs.redirectAddress = from;
	/* FIXME: our rcon redirection could be improved. Big rcon commands such as status
	          lead to sending out of band packets on every single call to Com_Printf
	          which leads to client overflows (also a Q3 issue)
	*/
	Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!strlen(sv_rconPassword->string))
	{
		Com_Printf("No rconpassword set on the server\n");
	}
	else if (!valid)
	{
		Com_Printf("Bad rconpassword\n");
		SV_WriteAttackLog(va("Bad rconpassword from %s\n", NET_AdrToString(from)));
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

/**
 * @brief A connectionless packet has four leading 0xff characters to distinguish it
 * from a game channel.
 * Clients that are in the game can still send connectionless packets.
 *
 * @param[in] from
 * @param[in] msg
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

	if (com_developer->integer)
	{
		Com_Printf("SV packet %s : %s\n", NET_AdrToString(from), c);
	}

	if (!Q_stricmp(c, "getstatus"))
	{
	    if(sv_hidden->integer)
        {
	        return;
        }

		if ((sv_protect->integer & SVP_OWOLF) && SV_CheckDRDoS(from))
		{
			return;
		}

		SVC_Status(from, qfalse);
	}
	else if (!Q_stricmp(c, "getinfo"))
	{
        if(sv_hidden->integer)
        {
            return;
        }

		if ((sv_protect->integer & SVP_OWOLF) && SV_CheckDRDoS(from))
		{
			return;
		}
		SVC_Info(from);
	}
	else if (!Q_stricmp(c, "getchallenge"))
	{
		if ((sv_protect->integer & SVP_OWOLF) && SV_CheckDRDoS(from))
		{
			return;
		}

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
	// Update server response message
	else if (!Q_stricmp(c, "updateResponse"))
	{
		Com_UpdateInfoPacket(from);
	}
	else
	{
		SV_WriteAttackLog(va("bad connectionless packet from %s:\n%s\n" // changed from Com_DPrintf to print in attack log
		                     , NET_AdrToString(from), s));              // this was never reported to admins before so they might be confused
	}                                                                   // note: if protect log isn't set we do Com_Printf
}

/**
 * @brief SV_PacketEvent
 * @param[in] from
 * @param[in] msg
 */
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
}

/**
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

			// let the game dll know about the ping - we are using mod based bots
			ps       = SV_GameClientNum(i);
			ps->ping = cl->ping;
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

/**
 * @brief If a packet has not been received from a client for timeout->integer
 * seconds, drop the connection.
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
	client_t *cl;
	int      i;
	int      droppoint    = svs.time - 1000 * sv_timeout->integer;    // default 60 - used in game and while vid_restart
	int      zombiepoint  = svs.time - 1000 * sv_zombietime->integer; // default 2
	int      droppoint_dl = svs.time - 1000 * sv_dl_timeout->integer; // default 300

	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->lastPacketTime > svs.time)
		{
			cl->lastPacketTime = svs.time;
		}

		// time is wonked
		if (cl->lastValidGamestate > svs.time || cl->lastValidGamestate == 0)
		{
			cl->lastValidGamestate = svs.time;
		}

		if (cl->state == CS_ZOMBIE && cl->lastPacketTime < zombiepoint)
		{
			// using the client id cause the cl->name is empty at this point
			Com_DPrintf("Going from CS_ZOMBIE to CS_FREE for client %d\n", i);
			cl->state = CS_FREE;    // can now be reused

			continue;
		}

		if (cl->state >= CS_CONNECTED)
		{
			if (*cl->downloadName) // download in progress
			{
				// this should deal with all download types (http & netchannel)
				// but we may add a check for lastPacketTime additionally in case of netchannel DL before we drop
				if (cl->downloadAckTime < droppoint_dl)
				{
					// wait several frames so a debugger session doesn't
					// cause a timeout
					if (++cl->timeoutCount > 5)
					{
						SV_DropClient(cl, va("download timed out %i\n", cl->state));
						cl->state = CS_FREE;    // don't bother with zombie state
					}
				}
				else
				{
					cl->timeoutCount = 0;
				}
			}
			else
			{
				if (cl->lastPacketTime < droppoint)
				{
					// wait several frames so a debugger session doesn't
					// cause a timeout
					if (++cl->timeoutCount > 5)
					{
						SV_DropClient(cl, va("game timed out %i\n", cl->state));
						cl->state = CS_FREE;    // don't bother with zombie state
					}
				}
				else
				{
					cl->timeoutCount = 0;
				}
			}
		}
		else
		{
			cl->timeoutCount = 0;
		}
	}
}

/**
 * @brief SV_CheckPaused
 * @return
 */
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

/**
 * @brief Return time in millseconds until processing of the next server frame.
 */
int SV_FrameMsec()
{
	if (sv_fps)
	{
		int frameMsec = (int)(1000.0f / sv_fps->value);

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

#ifdef DEDICATED
extern void Sys_Sleep(int msec);
#endif

#define CPU_USAGE_WARNING  70
#define FRAME_TIME_WARNING 30

/**
 * @brief Player movement occurs as a result of packet events, which happen
 * before SV_Frame is called
 *
 * @param[in] msec
 */
void SV_Frame(int msec)
{
	int        frameMsec;
	int        startTime;
	char       mapname[MAX_QPATH];
	int        frameStartTime = 0;
	static int start, end;

	start           = Sys_Milliseconds();
	svs.stats.idle += ( double )(start - end) / 1000;

	// the menu kills the server with this cvar
	if (sv_killserver->integer)
	{
		SV_Shutdown("Server was killed.\n");
		Cvar_Set("sv_killserver", "0");
		return;
	}

#ifdef DEDICATED
	if (svs.download.bWWWDlDisconnected)
	{
		Com_WWWDownload();
		if (!com_sv_running->integer)
		{
			return;
		}
	}
#endif
	// Running as a server, but no map loaded
	if (!com_sv_running->integer)
	{
#ifdef DEDICATED
		// Block until something interesting happens
		Sys_Sleep(-1);
#endif
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

	// if time is about to hit the 32nd bit, kick all clients
	// and clear sv.time, rather
	// than checking for negative time wraparound everywhere.
	// 2giga-milliseconds = 23 days, so it won't be too often
	if (svs.time > 0x70000000)
	{
		Q_strncpyz(mapname, sv_mapname->string, MAX_QPATH);
		SV_Shutdown("Restarting server due to time wrapping");
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

		// play/record demo frame (if enabled)
		if (sv.demoState == DS_RECORDING) // Record the frame
		{
			SV_DemoWriteFrame();
		}
		else if (sv.demoState == DS_WAITINGPLAYBACK || sv_demoState->integer == DS_WAITINGPLAYBACK) // Launch again the playback of the demo (because we needed a restart in order to set some cvars such as sv_maxclients or fs_game)
		{
			SV_DemoRestartPlayback();
		}
		else if (sv.demoState == DS_PLAYBACK) // Play the next demo frame
		{
			Com_DPrintf("Playing back demo frame\n");
			SV_DemoReadFrame();
		}
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
		int frameEndTime = Sys_Milliseconds();

		svs.totalFrameTime += (frameEndTime - frameStartTime);

		// we may send warnings (similar to watchdog) to the game in case the frametime is unacceptable
		//Com_Printf("FRAMETIME frame: %i total %i\n", frameEndTime - frameStartTime, svs.totalFrameTime);

		svs.currentFrameIndex++;

		//if( svs.currentFrameIndex % 50 == 0 )
		//	Com_Printf( "currentFrameIndex: %i\n", svs.currentFrameIndex );

		if (svs.currentFrameIndex == SERVER_PERFORMANCECOUNTER_FRAMES)
		{
			int averageFrameTime = svs.totalFrameTime / SERVER_PERFORMANCECOUNTER_FRAMES;

			svs.sampleTimes[svs.currentSampleIndex % SERVER_PERFORMANCECOUNTER_SAMPLES] = averageFrameTime;
			svs.currentSampleIndex++;

			if (svs.currentSampleIndex > SERVER_PERFORMANCECOUNTER_SAMPLES)
			{
				int totalTime = 0, i;

				for (i = 0; i < SERVER_PERFORMANCECOUNTER_SAMPLES; i++)
				{
					totalTime += svs.sampleTimes[i];
				}

				if (!totalTime)
				{
					totalTime = 1;
				}

				averageFrameTime = totalTime / SERVER_PERFORMANCECOUNTER_SAMPLES;

				svs.serverLoad = (int)((averageFrameTime / (float)frameMsec) * 100);
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

	// collect timing statistics
	// - the above 2.60 performance thingy is just inaccurate (30 seconds 'stats')
	//   to give good warning messages and is only done for dedicated
	end               = Sys_Milliseconds();
	svs.stats.active += (( double )(end - start)) / 1000;

	if (++svs.stats.count == STATFRAMES) // 5 seconds
	{
		svs.stats.latched_active = svs.stats.active;
		svs.stats.latched_idle   = svs.stats.idle;
		svs.stats.active         = 0;
		svs.stats.idle           = 0;
		svs.stats.count          = 0;

		svs.stats.cpu = svs.stats.latched_active + svs.stats.latched_idle;

		if (svs.stats.cpu != 0.f)
		{
			svs.stats.cpu = 100 * svs.stats.latched_active / svs.stats.cpu;
		}

		svs.stats.avg = 1000 * svs.stats.latched_active / STATFRAMES;

		// FIXME: add mail, IRC, player info etc for both warnings
		// TODO: inspect/adjust these values and/or add cvars
		if (svs.stats.cpu > CPU_USAGE_WARNING)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Server CPU has reached a critical usage of %i%%\n", (int) svs.stats.cpu);
		}

		if (svs.stats.avg > FRAME_TIME_WARNING)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Average frame time has reached a critical value of %ims\n", (int) svs.stats.avg);
		}
	}
}

/**
 * @brief SV_LoadTag
 * @param[in] mod_name
 * @return
 */
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

	(void) FS_ReadFile(mod_name, (void **)&buffer);

	if (!buffer)
	{
		return qfalse;
	}

	pinmodel = (tagHeader_t *)buffer;

	version = LittleLong(pinmodel->version);
	if (version != TAG_VERSION)
	{
		FS_FreeFile(buffer);
		Com_Printf(S_COLOR_YELLOW "WARNING: SV_LoadTag: %s has wrong version (%i should be %i)\n", mod_name, version, TAG_VERSION);
		return 0;
	}

	if (sv.num_tagheaders >= MAX_TAG_FILES)
	{
		FS_FreeFile(buffer);
		Com_Error(ERR_DROP, "MAX_TAG_FILES reached");
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
		FS_FreeFile(buffer);
		Com_Error(ERR_DROP, "MAX_SERVER_TAGS reached");
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

/**
 * @brief Send download messages and queued packets in the time that we're idle,
 * i.e. not computing a server frame or sending client snapshots.
 *
 * @return The time in msec until we expect to be called next
 */
int SV_SendQueuedPackets()
{
	int        numBlocks;
	int        dlStart, deltaT, delayT;
	static int dlNextRound = 0;
	int        timeVal     = INT_MAX;

	// Send out fragmented packets now that we're idle
	delayT = SV_SendQueuedMessages();
	if (delayT >= 0)
	{
		timeVal = delayT;
	}

	if (sv_dlRate->integer)
	{
		// Rate limiting. This is very imprecise for high
		// download rates due to millisecond timedelta resolution
		dlStart = Sys_Milliseconds();
		deltaT  = dlNextRound - dlStart;

		if (deltaT > 0)
		{
			if (deltaT < timeVal)
			{
				timeVal = deltaT + 1;
			}
		}
		else
		{
			numBlocks = SV_SendDownloadMessages();

			if (numBlocks)
			{
				// There are active downloads
				deltaT = Sys_Milliseconds() - dlStart;

				delayT  = 1000 * numBlocks * MAX_DOWNLOAD_BLKSIZE;
				delayT /= sv_dlRate->integer * 1024;

				if (delayT <= deltaT + 1)
				{
					// Sending the last round of download messages
					// took too long for given rate, don't wait for
					// next round, but always enforce a 1ms delay
					// between DL message rounds so we don't hog
					// all of the bandwidth. This will result in an
					// effective maximum rate of 1MB/s per user, but the
					// low download window size limits this anyways.
					if (timeVal > 2)
					{
						timeVal = 2;
					}

					dlNextRound = dlStart + deltaT + 1;
				}
				else
				{
					dlNextRound = dlStart + delayT;
					delayT     -= deltaT;

					if (delayT < timeVal)
					{
						timeVal = delayT;
					}
				}
			}
		}
	}
	else
	{
		if (SV_SendDownloadMessages())
		{
			timeVal = 0;
		}
	}

	return timeVal;
}
