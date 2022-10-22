/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2022 ET:Legacy team <mail@etlegacy.com>
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
 * @file g_demo_legacy.c
 */

#include "g_local.h"

 /**
 * @brief G_DemoPlaybackInit
 * @param[in] demoplayback
 * @param[in] demoClientsNum
 */
void G_DemoPlaybackInit(qboolean demoPlayback, int demoClientsNum)
{
	level.demoPlayback   = demoPlayback;
	level.demoClientsNum = demoClientsNum;
}

/**
* @brief G_DemoRunFrame
*/
void G_DemoRunFrame(void)
{
	gentity_t  *client;
	static int oldGamestate = -1;
	int        i;

	trap_Cvar_Set("g_guidCheck", 0);
	trap_Cvar_Set("g_allowVote", 0);
	trap_Cvar_Set("vote_allow_map", 0);

	if (g_gamestate.integer == GS_INTERMISSION && oldGamestate != GS_INTERMISSION)
	{
		level.intermissiontime = level.time;

		FindIntermissionPoint();

		// move all clients to the intermission point
		for (i = level.demoClientsNum; i < level.maxclients; i++)
		{
			client = g_entities + i;
			if (!client->inuse)
			{
				continue;
			}
			MoveClientToIntermission(client, qfalse);
		}
	}

	if (oldGamestate == GS_INTERMISSION && g_gamestate.integer != GS_INTERMISSION)
	{
		level.intermissiontime = 0;
	}

	// let entities initialize
	if (level.framenum < 7)
	{
		for (i = 0; i < level.num_entities; i++)
		{
			g_entities[i].runthisframe = qfalse;
		}

		// go through all allocated objects
		for (i = 0; i < level.num_entities; i++)
		{
			G_RunEntity(&g_entities[i], level.frameTime);
		}
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		if (level.sortedClients[i] < level.demoClientsNum)
		{
			continue;
		}

		ClientEndFrame(&g_entities[level.sortedClients[i]]);
	}

	oldGamestate = g_gamestate.integer;
}
