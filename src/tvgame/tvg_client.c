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
 * @file tvg_client.c
 * @brief Client functions that don't happen every frame
 */

#include "tvg_local.h"
#include "../../etmain/ui/menudef.h"
#include "../game/g_strparse.h"

#ifdef FEATURE_LUA
#include "tvg_lua.h"
#endif

// new bounding box
vec3_t playerMins = { -18, -18, -24 };
vec3_t playerMaxs = { 18, 18, 48 };

/**
* @brief QUAKED info_player_deathmatch (1 0 1) (-18 -18 -24) (18 18 48)
* potential spawning position for deathmatch games.
* Targets will be fired when someone spawns in on them.
* "nobots" will prevent bots from using this spot.
* "nohumans" will prevent non-bots from using this spot.
*
* If the start position is targeting an entity, the players camera will start out facing that ent (like an info_notnull)
*
* @param[in,out] ent
*/
void SP_info_player_deathmatch(gentity_t *ent)
{
	int i;

	TVG_SpawnInt("nobots", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_BOTS;
	}
	TVG_SpawnInt("nohumans", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_HUMANS;
	}

	ent->enemy = TVG_PickTarget(ent->target);
	if (ent->enemy)
	{
		vec3_t dir;

		VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
		vectoangles(dir, ent->s.angles);
	}
}

/**
* @brief QUAKED info_player_checkpoint (1 0 0) (-16 -16 -24) (16 16 32) a b c d
* these are start points /after/ the level start
* the letter (a b c d) designates the checkpoint that needs to be complete in order to use this start position
*
* @param[in] ent
*/
void SP_info_player_checkpoint(gentity_t *ent)
{
	ent->classname = "info_player_checkpoint";
	SP_info_player_deathmatch(ent);
}

/**
* @brief QUAKED info_player_start (1 0 0) (-18 -18 -24) (18 18 48)
* equivelant to info_player_deathmatch
* @param ent
*/
void SP_info_player_start(gentity_t *ent)
{
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch(ent);
}

/**
 * @brief QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) AXIS ALLIED
 * The intermission will be viewed from this point.  Target an info_notnull for the view direction.
 */
void SP_info_player_intermission(gentity_t *ent)
{
}

/**
 * @brief SP_info_notnull
 */
void SP_info_notnull(gentity_t *self)
{
}

/*
=======================================================================
  SelectSpawnPoint
=======================================================================
*/

/**
* @brief Find the spot that we DON'T want to use
* @param[in] from
* @return
*/
gentity_t *SelectNearestDeathmatchSpawnPoint(vec3_t from)
{
	gentity_t *spot = NULL;
	float     dist, nearestDist = 999999;
	gentity_t *nearestSpot = NULL;

	while ((spot = TVG_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		dist = VectorDistance(spot->r.currentOrigin, from);
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

#define MAX_SPAWN_POINTS    128

/**
* @brief Go to a random point that doesn't telefrag
* @return
*/
gentity_t *SelectRandomDeathmatchSpawnPoint(void)
{
	gentity_t *spot = NULL;
	int       count = 0;
	int       selection;
	gentity_t *spots[MAX_SPAWN_POINTS];

	while ((spot = TVG_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		spots[count] = spot;
		count++;
	}

	if (!count)     // no spots that won't telefrag
	{
		return TVG_Find(NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[selection];
}

/**
* @brief Chooses a player start, deathmatch start, etc
*
* @param[in] avoidPoint
* @param[in] origin
* @param[in] angles
* @return
*/
gentity_t *SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles)
{
	gentity_t *nearestSpot;
	gentity_t *spot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint(avoidPoint);
	spot        = SelectRandomDeathmatchSpawnPoint();

	if (spot == nearestSpot)
	{
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint();
		if (spot == nearestSpot)
		{
			// last try
			spot = SelectRandomDeathmatchSpawnPoint();
		}
	}

	// find a player start spot
	if (!spot)
	{
		G_Error("Couldn't find a spawn point\n");
	}

	VectorCopy(spot->r.currentOrigin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);

	return spot;
}

/**
 * @brief TVG_SelectSpectatorSpawnPoint
 * @param[out] origin
 * @param[out] angles
 */
static void TVG_SelectSpectatorSpawnPoint(vec3_t origin, vec3_t angles)
{
	VectorCopy(level.intermission_origins[0], origin);
	VectorCopy(level.intermission_angles[0], angles);
}

//======================================================================

/**
 * @brief TVG_SetClientViewAngle
 * @param[in,out] ent
 * @param[in] angle
 */
void TVG_SetClientViewAngle(gclient_t *client, const vec3_t angle)
{
	int i;
	int cmdAngle;

	// set the delta angle
	for (i = 0 ; i < 3 ; i++)
	{
		cmdAngle                   = ANGLE2SHORT(angle[i]);
		client->ps.delta_angles[i] = cmdAngle - client->pers.cmd.angles[i];
	}
	VectorCopy(angle, client->ps.viewangles);
}

/**
 * @brief Count the number of players on a team
 *
 * @param[in] ignoreClientNum
 * @param[in] team
 *
 * @return Number of players on a team
 */
int TVG_TeamCount(int ignoreClientNum, team_t team)
{
	int i, ref, count = 0;

	for (i = 0; i < level.numValidMasterClients; i++)
	{
		if ((ref = level.validMasterClients[i]) == ignoreClientNum)
		{
			continue;
		}
		if (level.ettvMasterClients[ref].ps.teamNum == team)
		{
			count++;
		}
	}

	return count;
}

/**
 * @brief ClientCleanName
 * @param[in] in
 * @param[out] out
 * @param[in] outSize
 */
static void ClientCleanName(const char *in, char *out, size_t outSize)
{
	size_t len          = 0;
	int    colorlessLen = 0;
	char   ch;
	char   *p;
	int    spaces = 0;

	// save room for trailing null byte
	outSize--;
	p  = out;
	*p = 0;

	while (1)
	{
		ch = *in++;
		if (!ch)
		{
			break;
		}

		// don't allow leading spaces
		if (!*p && ch == ' ')
		{
			continue;
		}

		// check colors
		if (ch == Q_COLOR_ESCAPE)
		{
			// solo trailing carat is not a color prefix
			if (!*in)
			{
				break;
			}

			// make sure room in dest for both chars
			if (len > outSize - 2)
			{
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len   += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if (ch == ' ')
		{
			spaces++;
			if (spaces > 3)
			{
				continue;
			}
		}
		else
		{
			spaces = 0;
		}

		if (len > outSize - 1)
		{
			break;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if (*p == 0 || colorlessLen == 0)
	{
		Q_strncpyz(p, "UnnamedPlayer", outSize);
	}
}

/**
 * @brief GetParsedIP
 * @param[in] ipadd
 * @return
 *
 * @todo FIXME: IP6
 *
 * @note Courtesy of Dens
 */
const char *GetParsedIP(const char *ipadd)
{
	// code by Dan Pop, http://bytes.com/forum/thread212174.html
	unsigned      b1, b2, b3, b4, port = 0;
	unsigned char c;
	int           rc;
	static char   ipge[20];

	if (!Q_strncmp(ipadd, "localhost", 9))
	{
		return "localhost";
	}

	rc = Q_sscanf(ipadd, "%3u.%3u.%3u.%3u:%u%c", &b1, &b2, &b3, &b4, &port, &c);
	if (rc < 4 || rc > 5)
	{
		return NULL;
	}
	if ((b1 | b2 | b3 | b4) > 255 || port > 65535)
	{
		return NULL;
	}
	if (strspn(ipadd, "0123456789.:") < strlen(ipadd))
	{
		return NULL;
	}

	Com_sprintf(ipge, sizeof(ipge), "%u.%u.%u.%u", b1, b2, b3, b4);
	return ipge;
}

/**
 * @brief Based on userinfocheck.lua and combinedfixes.lua
 * @param clientNum - unused
 * @param[in] userinfo
 * @return
 *
 * @note FIXME: IP6
 */
char *CheckUserinfo(int clientNum, char *userinfo)
{
	char         *value;
	unsigned int length = strlen(userinfo);
	int          i, slashCount = 0, count = 0;

	if (length < 1)
	{
		return "Userinfo too short";
	}

	// 44 is a bit random now: MAX_INFO_STRING - 44 = 980. The LUA script
	// uses 980, but I don't know if there is a specific reason for that
	// number. Userinfo should never get this big anyway, unless someone is
	// trying to force the engine to truncate it, so that the real IP is lost
	if (length > MAX_INFO_STRING - 44)
	{
		return "Userinfo too long.";
	}

	// userinfo always has to have a leading slash
	if (userinfo[0] != '\\')
	{
		return "Missing leading slash in userinfo.";
	}
	// the engine always adds ip\ip:port at the end, so there will never
	// be a trailing slash
	if (userinfo[length - 1] == '\\')
	{
		return "Trailing slash in userinfo.";
	}

	for (i = 0; userinfo[i]; ++i)
	{
		if (userinfo[i] == '\\')
		{
			slashCount++;
		}
	}
	if (slashCount % 2 != 0)
	{
		return "Bad number of slashes in userinfo.";
	}

	// make sure there is only one ip, cl_guid, name and cl_punkbuster field
	if (length > 4)
	{
		for (i = 0; userinfo[i + 3]; ++i)
		{
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'i' &&
			    userinfo[i + 2] == 'p' && userinfo[i + 3] == '\\')
			{
				count++;
			}
		}
	}
	if (count == 0)
	{
		return "Missing IP in userinfo.";
	}
	else if (count > 1)
	{
		return "Too many IP fields in userinfo.";
	}
	else
	{
		if (GetParsedIP(Info_ValueForKey(userinfo, "ip")) == NULL)
		{
			return "Malformed IP in userinfo.";
		}
	}
	count = 0;

	if (length > 9)
	{
		for (i = 0; userinfo[i + 8]; ++i)
		{
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'c' &&
			    userinfo[i + 2] == 'l' && userinfo[i + 3] == '_' &&
			    userinfo[i + 4] == 'g' && userinfo[i + 5] == 'u' &&
			    userinfo[i + 6] == 'i' && userinfo[i + 7] == 'd' &&
			    userinfo[i + 8] == '\\')
			{
				count++;
			}
		}
	}
	if (count > 1)
	{
		return "Too many cl_guid fields in userinfo.";
	}
	count = 0;

	if (length > 6)
	{
		for (i = 0; userinfo[i + 5]; ++i)
		{
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'n' &&
			    userinfo[i + 2] == 'a' && userinfo[i + 3] == 'm' &&
			    userinfo[i + 4] == 'e' && userinfo[i + 5] == '\\')
			{
				count++;
			}
		}
	}
	if (count == 0)
	{
		// if an empty name is entered, the userinfo will not contain a /name key/value at all..
		// FIXME: replace ?
		return "Missing name field in userinfo.";
	}
	else if (count > 1)
	{
		return "Too many name fields in userinfo.";
	}
	count = 0;

	if (length > 15)
	{
		for (i = 0; userinfo[i + 14]; ++i)
		{
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'c' &&
			    userinfo[i + 2] == 'l' && userinfo[i + 3] == '_' &&
			    userinfo[i + 4] == 'p' && userinfo[i + 5] == 'u' &&
			    userinfo[i + 6] == 'n' && userinfo[i + 7] == 'k' &&
			    userinfo[i + 8] == 'b' && userinfo[i + 9] == 'u' &&
			    userinfo[i + 10] == 's' && userinfo[i + 11] == 't' &&
			    userinfo[i + 12] == 'e' && userinfo[i + 13] == 'r' &&
			    userinfo[i + 14] == '\\')
			{
				count++;
			}
		}
	}
	if (count > 1)
	{
		return "Too many cl_punkbuster fields in userinfo.";
	}

	value = Info_ValueForKey(userinfo, "rate");
	if (value == NULL || value[0] == '\0')
	{
		return "Wrong rate field in userinfo.";
	}

	return 0;
}

/**
 * @brief Called from TVG_ClientConnect when the player first connects and
 * directly by the server system when the player updates a userinfo variable.
 *
 * The game can override any of the settings and call trap_SetUserinfo
 * if desired.
 *
 * @param[in] clientNum
 */
void TVG_ClientUserinfoChanged(int clientNum)
{
	gclient_t  *client                    = level.clients + clientNum;
	const char *userinfo_ptr              = NULL;
	char       cs_key[MAX_STRING_CHARS]   = "";
	char       cs_value[MAX_STRING_CHARS] = "";
	char       cs_skill[MAX_STRING_CHARS] = "";
	char       *reason;
	char       cs_name[MAX_NETNAME] = "";
	char       oldname[MAX_NAME_LENGTH];
	char       userinfo[MAX_INFO_STRING];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	reason = CheckUserinfo(clientNum, userinfo);
	if (reason)
	{
		G_Printf("TVG_ClientUserinfoChanged: CheckUserinfo: client %d: %s\n", clientNum, reason);
		trap_DropClient(clientNum, va("^1%s", "Bad userinfo."), 0);
		return;
	}

	// Cache config string values used in ClientUserinfoChanged
	// Every time we call Info_ValueForKey we search through the string from the start....
	// let's grab the values we need in just one pass... key matching is fast because
	// we will use our minimal perfect hashing function.
	userinfo_ptr = userinfo;

	while (1)
	{
		g_StringToken_t token;
		// Get next key/value pair
		if (qfalse == Info_NextPair(&userinfo_ptr, cs_key, cs_value))
		{
			// This would only happen if the end user is trying to manually modify the user info string
			G_Printf("TVG_ClientUserinfoChanged: client %d hacking clientinfo, empty key found!\n", clientNum);
			trap_DropClient(clientNum, "Bad userinfo.", 0);
			return;
		}
		// Empty string for key and we exit...
		if (cs_key[0] == 0)
		{
			break;
		}

		token = G_GetTokenForString(cs_key);
		switch (token)
		{
		case TOK_ip:
		{
			if (CompareIPNoPort(client->pers.client_ip, cs_value) == qfalse)
			{
				// They're trying to hack their ip address....
				G_Printf("TVG_ClientUserinfoChanged: client %d hacking ip, old=%s, new=%s\n", clientNum, client->pers.client_ip, cs_value);
				trap_DropClient(clientNum, "Bad userinfo.", 0);
				return;
			}
			Q_strncpyz(client->pers.client_ip, cs_value, MAX_IP4_LENGTH);
			break;
		}
		case TOK_cg_uinfo:
			break;
		case TOK_name:
			// see also MAX_NAME_LENGTH
			if (strlen(cs_value) >= MAX_NETNAME)
			{
				// They're trying long names
				G_Printf("TVG_ClientUserinfoChanged: client %d kicked for long name in config string old=%s, new=%s\n", clientNum, client->pers.cl_guid, cs_value);
				trap_DropClient(clientNum, va("Name too long (>%d). Plase change your name.", MAX_NETNAME - 1), 0);
				return;
			}
			if (!tvg_extendedNames.integer)
			{
				unsigned int i;

				// Avoid ext. ASCII chars in the CS
				for (i = 0; i < strlen(cs_value); ++i)
				{
					// extended ASCII chars have values between -128 and 0 (signed char) and the ASCII code flags are 0-31
					if (cs_value[i] < 32)
					{
						G_Printf("TVG_ClientUserinfoChanged: client %d kicked for extended ASCII characters name in config string old=%s, new=%s\n", clientNum, client->pers.cl_guid, cs_value);
						trap_DropClient(clientNum, "Server does not allow extended ASCII characters. Please change your name.", 0);
						return;
					}
				}
			}
			Q_strncpyz(cs_name, cs_value, MAX_NETNAME);
			break;
		case TOK_cl_guid:
			if (strcmp(client->pers.cl_guid, cs_value))
			{
				// They're trying to hack their guid...
				G_Printf("TVG_ClientUserinfoChanged: client %d hacking cl_guid, old=%s, new=%s\n", clientNum, client->pers.cl_guid, cs_value);
				trap_DropClient(clientNum, "Bad userinfo.", 0);
				return;
			}
			Q_strncpyz(client->pers.cl_guid, cs_value, MAX_GUID_LENGTH + 1);
			break;
		case TOK_skill:
			Q_strncpyz(cs_skill, cs_value, MAX_STRING_CHARS);
			break;
		default:
			continue;
		}
	}

	// overwrite empty names with a default name, but do not kick those players ...
	// (player did delete the profile or profile can't be read)
	if (strlen(cs_name) == 0)
	{
		Q_strncpyz(cs_name, va("Target #%i", clientNum), 15);
		Info_SetValueForKey(userinfo, "name", cs_name);
		trap_SetUserinfo(clientNum, userinfo);
		//CP("cp \"You cannot assign an empty playername! Your name has been reset.\"");
		G_LogPrintf("TVG_ClientUserinfoChanged: %i User with empty name. (Changed to: \"Target #%i\")\n", clientNum, clientNum);
		G_DPrintf("TVG_ClientUserinfoChanged: %i User with empty name. (Changed to: \"Target #%i\")\n", clientNum, clientNum);
	}

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		Q_strncpyz(userinfo, "\\name\\badinfo", sizeof(userinfo));
		G_Printf("TVG_ClientUserinfoChanged: CheckUserinfo: client %d: Invalid userinfo\n", clientNum);
		trap_DropClient(clientNum, "Invalid userinfo", 300);
		return;
	}

#ifndef DEBUG_STATS
	if (g_developer.integer || *tvg_log.string || tvg_dedicated.integer)
#endif
	{
		G_Printf("Userinfo: %s\n", userinfo);
	}

	if (tvg_protect.integer & G_PROTECT_LOCALHOST_REF)
	{
	}
	else // no protection, check for local client and set ref (for LAN/listen server games)
	{
		if (!strcmp(client->pers.client_ip, "localhost"))
		{
			client->pers.localClient = qtrue;
			level.fLocalHost         = qtrue;
			client->sess.referee     = RL_REFEREE;
		}
	}

	client->pmext.bAutoReload = qfalse;
	client->pers.activateLean = qfalse;

	// set name
	Q_strncpyz(oldname, client->pers.netname, sizeof(oldname));
	ClientCleanName(cs_name, client->pers.netname, sizeof(client->pers.netname));

	if (client->pers.connected == CON_CONNECTED)
	{
		if (strcmp(oldname, client->pers.netname))
		{
			trap_SendServerCommand(-1, va("print \"[lof]" S_COLOR_WHITE "%s" S_COLOR_WHITE " [lon]renamed to[lof] %s\n\"", oldname,
			                              client->pers.netname));
		}
	}

	client->ps.stats[STAT_MAX_HEALTH] = DEFAULT_HEALTH;

	// To communicate it to cgame
	client->ps.stats[STAT_PLAYER_CLASS] = client->sess.playerType;
	// Gordon: Not needed any more as it's in clientinfo?

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	//s = va("n\\%s\\t\\%i\\c\\%i\\lc\\%i\\r\\%i\\m\\%s\\s\\%s\\dn\\%i\\w\\%i\\lw\\%i\\sw\\%i\\lsw\\%i\\mu\\%i\\ref\\%i\\sc\\%i\\u\\%u",
	//       client->pers.netname,
	//       client->sess.sessionTeam,
	//       client->sess.playerType,
	//       client->sess.rank,
	//       medalStr,
	//       skillStr,
	//       client->sess.muted ? 1 : 0,
	//       client->sess.referee
	//       );

#ifdef FEATURE_LUA
	// *LUA* API callbacks
	TVG_LuaHook_ClientUserinfoChanged(clientNum);
#endif

	//G_LogPrintf("TVG_ClientUserinfoChanged: %i %s\n", clientNum, s);
	//G_DPrintf("TVG_ClientUserinfoChanged: %i :: %s\n", clientNum, s);
}

/**
 * @brief Called when a player begins connecting to the server.
 * Called again for every map change or tournement restart.
 *
 * @details The session information will be valid after exit.
 *
 * Return NULL if the client should be allowed, otherwise return
 * a string with the reason for denial.
 *
 * Otherwise, the client will be sent the current gamestate
 * and will eventually get to TVG_ClientBegin.
 *
 * @param[in] clientNum
 * @param[in] firstTime will be qtrue the very first time a client connects to the server machine, but qfalse on map changes and tournement restarts.
 * @param[in] isBot
 *
 * @return NULL if the client should be allowed, otherwise return
 * a string with the reason for denial.
 */
char *TVG_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	gclient_t  *client = level.clients + clientNum;
	const char *userinfo_ptr;
	char       userinfo[MAX_INFO_STRING];
	char       cs_key[MAX_STRING_CHARS]      = "";
	char       cs_value[MAX_STRING_CHARS]    = "";
	char       cs_ip[MAX_STRING_CHARS]       = "";
	char       cs_password[MAX_STRING_CHARS] = "";
	char       cs_name[MAX_NETNAME + 1]      = "";
	char       cs_guid[MAX_GUID_LENGTH + 1]  = "";
	//char       cs_rate[MAX_STRING_CHARS]     = "";

#ifdef FEATURE_LUA
	char reason[MAX_STRING_CHARS] = "";
#endif

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	// grab the values we need in just one pass
	// Use our minimal perfect hash generated code to give us the
	// result token in optimal time
	userinfo_ptr = userinfo;
	while (1)
	{
		g_StringToken_t token;
		// Get next key/value pair
		Info_NextPair(&userinfo_ptr, cs_key, cs_value);
		// Empty string for key and we exit...
		if (cs_key[0] == 0)
		{
			break;
		}

		token = G_GetTokenForString(cs_key);
		switch (token)
		{
		case TOK_ip:
			Q_strncpyz(cs_ip, cs_value, MAX_STRING_CHARS);
			break;
		case TOK_name:
			Q_strncpyz(cs_name, cs_value, MAX_NETNAME + 1);
			break;
		case TOK_cl_guid:
			Q_strncpyz(cs_guid, cs_value, MAX_GUID_LENGTH + 1);
			break;
		case TOK_password:
			Q_strncpyz(cs_password, cs_value, MAX_STRING_CHARS);
			break;
		//case TOK_rate:
		//	Q_strncpyz(cs_rate, cs_value, MAX_STRING_CHARS);
		//	break;
		default:
			continue;
		}
	}

	// IP filtering
	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
	// check to see if they are on the banned IP list
	if (G_FilterIPBanPacket(cs_ip))
	{
		return "You are banned from this server.";
	}

	// don't permit empty name
	if (!strlen(cs_name))
	{
		return va("Bad name: Name is empty. Please change your name.");
	}

	// don't permit long names ... - see also MAX_NETNAME
	if (strlen(cs_name) >= MAX_NAME_LENGTH)
	{
		return va("Bad name: Name too long (>%d). Please change your name.", MAX_NAME_LENGTH - 1);
	}

	if (!tvg_extendedNames.integer)
	{
		unsigned int i;

		// Avoid ext. ASCII chars in the CS
		for (i = 0; i < strlen(cs_name); ++i)
		{
			// extended ASCII chars have values between -128 and 0 (signed char) and the ASCII code flags are 0-31
			if (cs_name[i] < 32)
			{
				return "Bad name: Extended ASCII characters. Please change your name.";
			}
		}
	}

	// we don't check password for bots and local client
	// NOTE: local client <-> "ip" "localhost"
	//   this means this client is not running in our current process
	if ((strcmp(cs_ip, "localhost") != 0))
	{
		// check for a password
		if (g_password.string[0] && Q_stricmp(g_password.string, "none") && strcmp(g_password.string, cs_password) != 0)
		{
			if (!sv_privatepassword.string[0] || strcmp(sv_privatepassword.string, cs_password))
			{
				return "Invalid password";
			}
		}
	}

	// porting q3f flag bug fix
	// If a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get lost in the ether
	// see ZOMBIE state
	if (client->pers.connected != CON_DISCONNECTED)
	{
		G_LogPrintf("Forcing disconnect on active client: %i\n", (int)(client - level.clients));
		// so lets just fix up anything that should happen on a disconnect
		TVG_ClientDisconnect(client - level.clients);
	}

	Com_Memset(client, 0, sizeof(*client));

	client->pers.connected   = CON_CONNECTING;
	client->pers.connectTime = level.time;

	// Set the client ip and guid
	Q_strncpyz(client->pers.client_ip, cs_ip, MAX_IP4_LENGTH);
	Q_strncpyz(client->pers.cl_guid, cs_guid, MAX_GUID_LENGTH + 1);

	if (firstTime)
	{
		// read or initialize the session data
		TVG_InitSessionData(client, userinfo);
		client->pers.enterTime            = level.time;
		client->ps.persistant[PERS_SCORE] = 0;
	}
	else
	{
		TVG_ReadSessionData(client);
	}

	client->pers.enterTime = level.time;

	if (firstTime)
	{
		// force into spectator
		client->sess.sessionTeam     = TEAM_SPECTATOR;
		client->sess.spectatorState  = SPECTATOR_FREE;
		client->sess.spectatorClient = 0;
	}

#ifdef FEATURE_LUA
	// LUA API callbacks (check with Lua scripts)
	if (TVG_LuaHook_ClientConnect(clientNum, firstTime, isBot, reason))
	{
		return va("You are excluded from this server. %s\n", reason);
	}
#endif

	// get and distribute relevent paramters
	G_LogPrintf("ClientConnect: %i\n", clientNum);

	TVG_ClientUserinfoChanged(clientNum);

	// don't do the "xxx connected" messages if they were caried over from previous level
	// disabled for bots - see join message ... make cvar ?
	if (firstTime)
	{
		trap_SendServerCommand(-1, va("cpm \"" S_COLOR_WHITE "%s" S_COLOR_WHITE " connected\n\"", client->pers.netname));
	}

	// count current clients and rank for scoreboard
	TVG_CalculateRanks();

	return NULL;
}

/**
 * @brief Called when a client has finished connecting, and is ready
 * to be placed into the level.  This will happen every level load,
 * and on transition between teams, but doesn't happen on respawns
 *
 * @param[in] clientNum
 */
void TVG_ClientBegin(int clientNum)
{
	gclient_t *client = level.clients + clientNum;
	int       flags;
	int       spawn_count, lives_left;
	int       stat_xp, score; // restore xp & score

#ifdef FEATURE_LUA
	// call LUA clientBegin only once when player connects
	qboolean firsttime = qfalse;
	if (client->pers.connected == CON_CONNECTING)
	{
		firsttime = qtrue;
	}
#endif

	client->pers.connected       = CON_CONNECTED;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	// Also save PERS_SPAWN_COUNT, so that CG_Respawn happens
	spawn_count = client->ps.persistant[PERS_SPAWN_COUNT];

	if (client->ps.persistant[PERS_RESPAWNS_LEFT] > 0)
	{
		lives_left = client->ps.persistant[PERS_RESPAWNS_LEFT] - 1;
	}
	else
	{
		lives_left = client->ps.persistant[PERS_RESPAWNS_LEFT];
	}
	flags = client->ps.eFlags;

	// restore xp & score
	stat_xp = client->ps.stats[STAT_XP];
	score   = client->ps.persistant[PERS_SCORE];

	Com_Memset(&client->ps, 0, sizeof(client->ps));

	client->ps.persistant[PERS_SCORE] = score;

	if (client->sess.spectatorState == SPECTATOR_FREE) // restore xp
	{
		client->ps.stats[STAT_XP] = stat_xp;
	}

	if (level.intermission)
	{
		client->ps.pm_type = PM_INTERMISSION;
	}

	client->ps.eFlags                         = flags;
	client->ps.persistant[PERS_SPAWN_COUNT]   = spawn_count;
	client->ps.persistant[PERS_RESPAWNS_LEFT] = lives_left;

	TVG_ClientSpawn(client);

	client->inactivityTime        = level.time + TVG_InactivityValue * 1000;
	client->inactivitySecondsLeft = TVG_InactivityValue;

	G_LogPrintf("TVG_ClientBegin: %i\n", clientNum);

	// count current clients and rank for scoreboard
	TVG_CalculateRanks();

#ifdef FEATURE_LUA
	// call LUA clientBegin only once
	if (firsttime == qtrue)
	{
		// LUA API callbacks
		TVG_LuaHook_ClientBegin(clientNum);
	}
#endif
}

/**
 * @brief Called every time a client is placed fresh in the world:
 * after the first TVG_ClientBegin, and after each respawn
 * Initializes all non-persistant parts of playerState
 *
 * @param[in,out] ent
 * @param[in] revived
 * @param[in] teamChange
 * @param[in] restoreHealth
 */
void TVG_ClientSpawn(gclient_t *client)
{
	int                clientnum = client - level.clients;
	vec3_t             spawn_origin, spawn_angles;
	int                i;
	clientPersistant_t savedPers;
	clientSession_t    savedSess;
	int                persistant[MAX_PERSISTANT];
	int                flags;
	int                savedPing;
	int                savedTeam;

	TVG_SelectSpectatorSpawnPoint(spawn_origin, spawn_angles);

	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	flags  = client->ps.eFlags & EF_TELEPORT_BIT;
	flags ^= EF_TELEPORT_BIT;

	flags |= (client->ps.eFlags & EF_VOTED);

	// clear everything but the persistant data
	savedPers = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	savedTeam = client->ps.teamNum;

	for (i = 0 ; i < MAX_PERSISTANT ; i++)
	{
		persistant[i] = client->ps.persistant[i];
	}

	{
		Com_Memset(client, 0, sizeof(*client));
	}

	client->pers       = savedPers;
	client->sess       = savedSess;
	client->ps.ping    = savedPing;
	client->ps.teamNum = savedTeam;

	client->ps.pm_type = level.ettvMasterPs.pm_type;

	for (i = 0 ; i < MAX_PERSISTANT ; i++)
	{
		client->ps.persistant[i] = persistant[i];
	}

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM]        = client->sess.sessionTeam;
	client->ps.persistant[PERS_HWEAPON_USE] = 0;

	// breathbar
	client->ps.stats[STAT_AIRLEFT] = HOLDBREATHTIME;

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = DEFAULT_HEALTH;
	client->ps.eFlags                 = flags;

	client->ps.classWeaponTime = -999999;

	// setup the bounding boxes and viewheights for prediction
	VectorCopy(playerMins, client->ps.mins);
	VectorCopy(playerMaxs, client->ps.maxs);

	client->ps.crouchViewHeight = CROUCH_VIEWHEIGHT;
	client->ps.standViewHeight  = DEFAULT_VIEWHEIGHT;
	client->ps.deadViewHeight   = DEAD_VIEWHEIGHT;

	client->ps.crouchMaxZ = client->ps.maxs[2] - (client->ps.standViewHeight - client->ps.crouchViewHeight);

	client->ps.runSpeedScale    = 0.8f;
	client->ps.sprintSpeedScale = 1.1f;
	client->ps.crouchSpeedScale = 0.25f;
	client->ps.weaponstate      = WEAPON_READY;

	if (level.mod & LEGACY)
	{
		client->ps.stats[STAT_SPRINTTIME] = SPRINTTIME;
	}

	client->pmext.sprintTime   = SPRINTTIME;
	client->ps.sprintExertTime = 0;
	client->ps.friction        = 1.0f;

	client->pmext.bAutoReload = qfalse;

	client->ps.clientNum = level.ettvMasterPs.clientNum;

	// start tracing legs and head again if they were in solid
	client->pmext.deadInSolid = qfalse;

	trap_GetUsercmd(clientnum, &client->pers.cmd);

	// client has (almost) no say in weapon selection when spawning
	client->pers.cmd.weapon = client->ps.weapon;

	client->ps.stats[STAT_MAX_HEALTH] = DEFAULT_HEALTH;
	client->ps.stats[STAT_HEALTH]     = client->ps.stats[STAT_MAX_HEALTH];

	VectorCopy(spawn_origin, client->ps.origin);

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	TVG_SetClientViewAngle(client, spawn_angles);

	client->inactivityTime        = level.time + TVG_InactivityValue * 1000;
	client->inactivityWarning     = qfalse;
	client->inactivitySecondsLeft = TVG_InactivityValue;
	client->latched_buttons       = 0;
	client->latched_wbuttons      = 0;

#ifdef FEATURE_LUA
	// *LUA* API callbacks
	TVG_LuaHook_ClientSpawn(clientnum);
#endif

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime      = level.time - 100;
	client->pers.cmd.serverTime = level.time;

	TVG_ClientThink(clientnum);

	// run the presend to set anything else
	TVG_ClientEndFrame(client);

	// set idle animation on weapon
	client->ps.weapAnim = ((client->ps.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | WEAP_IDLE1;
}

/**
 * @brief Called when a player drops from the server.
 * Will not be called between levels.
 *
 * @details This should NOT be called directly by any game logic,
 * call trap_DropClient(), which will call this and do
*  server system housekeeping.
 *
 * @param[in] clientNum
 */
void TVG_ClientDisconnect(int clientNum)
{
	gclient_t *client = level.clients + clientNum;

#ifdef FEATURE_LUA
	// LUA API callbacks
	TVG_LuaHook_ClientDisconnect(clientNum);
#endif

	TVG_RemoveFromAllIgnoreLists(clientNum);

	G_LogPrintf("TVG_ClientDisconnect: %i\n", clientNum);

	client->pers.connected            = CON_DISCONNECTED;
	client->ps.persistant[PERS_TEAM]  = TEAM_FREE;
	client->ps.persistant[PERS_SCORE] = 0;
	client->sess.sessionTeam          = TEAM_FREE;

	TVG_CalculateRanks();
}


/**
 * @brief In just the GAME DLL, we want to store the groundtrace surface stuff,
 * so we don't have to keep tracing.
 *
 * @param[in] clientNum
 * @param[in] surfaceFlags
 */
void ClientStoreSurfaceFlags(int clientNum, int surfaceFlags)
{

}

/**
* @brief TVG_RemoveFromAllIgnoreLists
* @param[in] clientNum
*/
void TVG_RemoveFromAllIgnoreLists(int clientNum)
{
	//int i;

	//for (i = 0; i < MAX_CLIENTS; i++)
	//{
	//	COM_BitClear(level.clients[i].sess.ignoreClients, clientNum);
	//}
}
