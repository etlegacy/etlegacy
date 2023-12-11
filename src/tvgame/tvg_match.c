/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2023 ET:Legacy team <mail@etlegacy.com>
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
 * @file tvg_match.c
 * @brief Match handling
 */

#include "tvg_local.h"
#include "../../etmain/ui/menudef.h"
#include "json.h"

/**
 * @brief Plays specified sound globally.
 * @param[in] sound
 */
void G_globalSound(const char *sound)
{
	gentity_t *te;

	te = G_TempEntityNotLinked(EV_GLOBAL_SOUND);

	te->s.eventParm = G_SoundIndex(sound);
	te->r.svFlags  |= SVF_BROADCAST;
}

/**
 * @brief Dumps end-of-match info
 * @param dwDumpType
 */
void G_matchInfoDump(unsigned int dwDumpType)
{
	
}

/**
 * @brief Sends a player's stats to the requesting client.
 * @param[in] client
 * @param[in] nType
 */
void TVG_statsPrint(gclient_t *client, int nType, int cooldown)
{
	int  pid;
	char arg[MAX_TOKEN_CHARS];
	const char *cmd = (nType == 0) ? "weaponstats" : ((nType == 1) ? "wstats" : "sgstats");

	// If requesting stats for self, it's easy
	if (trap_Argc() < 2)
	{
		if (client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			pid = client->sess.spectatorClient;
		}
		else
		{
			return;
		}
	}
	else
	{
		// find the player to poll stats
		trap_Argv(1, arg, sizeof(arg));
		if ((pid = TVG_MasterClientNumberFromString(client, arg)) == -1)
		{
			return;
		}
	}

	client->wantsInfoStats[nType].requested          = qtrue;
	client->wantsInfoStats[nType].requestedClientNum = pid;

	// request new stats
	if (level.cmds.lastInfoStatsUpdate + cooldown <= level.time)
	{
		level.cmds.infoStats[nType].valid[pid] = qfalse;
		level.cmds.lastInfoStatsUpdate         = level.time;

		trap_SendServerCommand(-2, va("%s %d\n", cmd, pid));
	}
}
