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
 * @file tvg_session.c
 * @brief Session data is the only data that stays persistant across level loads
 *        and tournament restarts.
 */

#include "tvg_local.h"
#include "../../etmain/ui/menudef.h"

#include "json.h"

/**
 * SESSION DATA
*/

/**
 * @brief Called on game shutdown
 * @param[in] client
 * @param[in] restart
 */
void TVG_WriteClientSessionData(gclient_t *client, qboolean restart)
{
	cJSON *root;
	char  fileName[MAX_QPATH] = { 0 };

	Com_sprintf(fileName, sizeof(fileName), "session/client%02i.dat", (int)(client - level.clients));
	Com_Printf("Writing session file %s\n", fileName);

	Q_JSONInit();

	root = cJSON_CreateObject();
	if (!root)
	{
		Com_Error(ERR_FATAL, "Could not allocate memory for session data\n");
	}

	cJSON_AddNumberToObject(root, "sessionTeam", client->sess.sessionTeam);
	cJSON_AddNumberToObject(root, "spectatorState", client->sess.spectatorState);
	cJSON_AddNumberToObject(root, "spectatorClient", client->sess.spectatorClient);
	cJSON_AddNumberToObject(root, "playerType", client->sess.playerType);
	cJSON_AddNumberToObject(root, "referee", client->sess.referee);

	cJSON_AddNumberToObject(root, "muted", client->sess.muted);
	//cJSON_AddNumberToObject(root, "ignoreClients1", client->sess.ignoreClients[0]);
	//cJSON_AddNumberToObject(root, "ignoreClients2", client->sess.ignoreClients[1]);
	cJSON_AddNumberToObject(root, "enterTime", client->pers.enterTime);

	cJSON_AddNumberToObject(root, "spec_team", client->sess.spec_team);
	cJSON_AddNumberToObject(root, "tvchat", client->sess.tvchat);

	if (!Q_FSWriteJSONTo(root, fileName))
	{
		Com_Error(ERR_FATAL, "Could not write session information\n");
	}
}

/**
 * @brief Called on a reconnect
 * @param[in] client
 */
void TVG_ReadSessionData(gclient_t *client)
{
	char  fileName[MAX_QPATH] = { 0 };
	cJSON *root               = NULL;

	Com_sprintf(fileName, sizeof(fileName), "session/client%02i.dat", (int)(client - level.clients));
	Com_Printf("Reading session file %s\n", fileName);

	root = Q_FSReadJsonFrom(fileName);

	client->sess.sessionTeam     = Q_ReadIntValueJson(root, "sessionTeam");
	client->sess.spectatorState  = Q_ReadIntValueJson(root, "spectatorState");
	client->sess.spectatorClient = Q_ReadIntValueJson(root, "spectatorClient");
	client->sess.playerType      = Q_ReadIntValueJson(root, "playerType");
	client->sess.referee         = Q_ReadIntValueJson(root, "referee");

	client->sess.muted = Q_ReadIntValueJson(root, "muted");
	//client->sess.ignoreClients[0]    = Q_ReadIntValueJson(root, "ignoreClients1");
	//client->sess.ignoreClients[1]    = Q_ReadIntValueJson(root, "ignoreClients2");
	client->pers.enterTime = Q_ReadIntValueJson(root, "enterTime");

	client->sess.spec_team = Q_ReadIntValueJson(root, "spec_team");
	client->sess.tvchat    = Q_ReadIntValueJson(root, "tvchat");

	cJSON_Delete(root);
}

/**
 * @brief Called on a first-time connect
 * @param[in] client
 * @param userinfo
 */
void TVG_InitSessionData(gclient_t *client, const char *userinfo)
{
	clientSession_t *sess = &client->sess;

	sess->sessionTeam    = TEAM_SPECTATOR;
	sess->spectatorState = SPECTATOR_FREE;
	sess->playerType     = 0;

	//Com_Memset(sess->ignoreClients, 0, sizeof(sess->ignoreClients));

	sess->muted = qfalse;

	// we set ref in TVG_ClientUserinfoChanged
	sess->referee   = RL_NONE; // (client->pers.localClient) ? RL_REFEREE : RL_NONE;
	sess->spec_team = 0;
	sess->tvchat    = qtrue;

	TVG_WriteClientSessionData(client, qfalse);
}

/**
 * @brief TVG_WriteSessionData
 * @param[in] restart
 */
void TVG_WriteSessionData(qboolean restart)
{
	int  i;
	char strServerInfo[MAX_INFO_STRING];

	trap_GetServerinfo(strServerInfo, sizeof(strServerInfo));
	trap_Cvar_Set("session", va("%i %i %s", tvg_gametype.integer,
	                            (teamInfo[TEAM_AXIS].spec_lock * TEAM_AXIS | teamInfo[TEAM_ALLIES].spec_lock * TEAM_ALLIES),
	                            Info_ValueForKey(strServerInfo, "mapname")));

	for (i = 0; i < level.numConnectedClients; i++)
	{
		// also take care of slow connecters and a short warmup
		if (level.clients[level.sortedClients[i]].pers.connected == CON_CONNECTED)
		{
			TVG_WriteClientSessionData(&level.clients[level.sortedClients[i]], restart);
		}
	}
}
