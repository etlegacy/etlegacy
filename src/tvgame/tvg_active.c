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
 * @file tvg_active.c
 */

#include "tvg_local.h"

/**
 * @brief TVG_SpectatorAttackFollow
 * @param client
 * @return true if a player was found to follow, otherwise false
 */
qboolean TVG_SpectatorAttackFollow(gclient_t *client)
{
	trace_t       tr;
	vec3_t        forward, right, up;
	vec3_t        start, end;
	vec3_t        mins, maxs;
	static vec3_t enlargeMins = { -12.0f, -12.0f, -12.0f };
	static vec3_t enlargeMaxs = { 12.0f, 12.0f, 0.0f };

	AngleVectors(client->ps.viewangles, forward, right, up);
	VectorCopy(client->ps.origin, start);
	VectorMA(start, MAX_TRACE, forward, end);

	// enlarge the hitboxes, so spectators can easily click on them..
	VectorCopy(client->ps.mins, mins);
	VectorCopy(client->ps.maxs, maxs);
	VectorAdd(mins, enlargeMins, mins);
	VectorAdd(maxs, enlargeMaxs, maxs);
	// also put the start-point a bit forward, so we don't start the trace in solid..
	VectorMA(start, 75.0f, forward, start);

	trap_Trace(&tr, start, mins, maxs, end, ENTITYNUM_NONE, CONTENTS_BODY);

	if (tr.entityNum < MAX_CLIENTS && level.ettvMasterClients[tr.entityNum].valid)
	{
		client->sess.spectatorState  = SPECTATOR_FOLLOW;
		client->sess.spectatorClient = tr.entityNum;
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief TVG_SpectatorThink
 * @param[in] client
 * @param[in] ucmd
 */
void TVG_SpectatorThink(gclient_t *client, usercmd_t *ucmd)
{
	if (client->ps.identifyClient >= 0 && client->ps.identifyClient < MAX_CLIENTS)
	{
		if (level.ettvMasterClients[client->ps.identifyClient].valid)
		{
			if (level.ettvMasterClients[client->ps.identifyClient].ps.stats[STAT_HEALTH] >= 0)
			{
				client->ps.identifyClientHealth = level.ettvMasterClients[client->ps.identifyClient].ps.stats[STAT_HEALTH];
			}
			else
			{
				client->ps.identifyClientHealth = 0;
			}
		}
	}

	if (client->sess.spectatorState != SPECTATOR_FOLLOW)
	{
		pmove_t pm;

		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed   = 800; // was: 400 // faster than normal
		if (client->ps.sprintExertTime)
		{
			client->ps.speed *= 3;  // allow sprint in free-cam mode
		}
		else if (client->noclip)
		{
			client->ps.pm_type = PM_NOCLIP;
		}

		// set up for pmove
		Com_Memset(&pm, 0, sizeof(pm));
		pm.ps            = &client->ps;
		pm.pmext         = &client->pmext;
		pm.character     = client->pers.character;
		pm.cmd           = *ucmd;
		pm.skill         = client->sess.skill;
		pm.tracemask     = MASK_PLAYERSOLID & ~CONTENTS_BODY; // spectators can fly through bodies
		pm.trace         = trap_TraceCapsuleNoEnts;
		pm.pointcontents = trap_PointContents;
		pm.activateLean  = client->pers.activateLean;

		TVG_Pmove(&pm);

		// Activate - made it a latched event (occurs on keydown only)
		//if (client->latched_buttons & BUTTON_ACTIVATE)
		//{
		//	Cmd_Activate_f(ent);
		//}
	}

	client->ps.classWeaponTime = 0;

	client->oldbuttons = client->buttons;
	client->buttons    = ucmd->buttons;

	client->oldwbuttons = client->wbuttons;
	client->wbuttons    = ucmd->wbuttons;

	// attack button + "firing" a players = spectate that "victim"
	if ((client->buttons & BUTTON_ATTACK) && !(client->oldbuttons & BUTTON_ATTACK) &&
	    client->sess.spectatorState != SPECTATOR_FOLLOW &&
	    client->sess.sessionTeam == TEAM_SPECTATOR)        // don't do it if on a team
	{
		if (TVG_SpectatorAttackFollow(client))
		{
			return;
		}
		else
		{
			// not clicked on a player?.. then follow next,
			// to prevent constant traces done by server.
			if (client->buttons & BUTTON_SPRINT)
			{
				TVG_Cmd_FollowCycle_f(client, 1, qtrue);
			}
			// no humans playing?.. then follow a bot
			if (client->sess.spectatorState != SPECTATOR_FOLLOW)
			{
				TVG_Cmd_FollowCycle_f(client, 1, qfalse);
			}
		}
		// attack + sprint button cycles through non-bot/human players
		// attack button cycles through all players
	}
	else if ((client->buttons & BUTTON_ATTACK) && !(client->oldbuttons & BUTTON_ATTACK) &&
	         !(client->buttons & BUTTON_ACTIVATE))
	{
		TVG_Cmd_FollowCycle_f(client, 1, (client->buttons & BUTTON_SPRINT));
	}
	else if (client->sess.sessionTeam == TEAM_SPECTATOR && client->sess.spectatorState == SPECTATOR_FOLLOW &&
	         (((client->buttons & BUTTON_ACTIVATE) && !(client->oldbuttons & BUTTON_ACTIVATE)) || ucmd->upmove > 0))
	{
		TVG_StopFollowing(client);
	}
}

/**
 * @brief TVG_ClientInactivityTimer
 * tvg_inactivity values have to be higher then 10 seconds, that is the time after the warn message is sent.
 * (if it's lower than that value, it will not work at all)
 *
 * @param[in,out] client
 *
 * @return qfalse if the client is dropped
 */
qboolean TVG_ClientInactivityTimer(gclient_t *client)
{
	int inactivity = TVG_InactivityValue;

	// no countdown in intermission
	if (tvg_gamestate.integer == GS_INTERMISSION)
	{
		return qtrue;
	}

	// inactivity settings disabled?
	if (tvg_inactivity.integer == 0)
	{
		// Give everyone some time, so if the operator sets tvg_inactivity
		// gameplay, everyone isn't kicked
		client->inactivityTime    = level.time + 60000;
		client->inactivityWarning = qfalse;
		return qtrue;
	}

	// the client is still active?
	if (client->pers.cmd.forwardmove || client->pers.cmd.rightmove || client->pers.cmd.upmove ||
	    (client->pers.cmd.wbuttons & (WBUTTON_LEANLEFT | WBUTTON_LEANRIGHT)) ||
	    (client->pers.cmd.buttons & BUTTON_ATTACK) ||
	    (client->ps.pm_flags & PMF_FOLLOW))
	{
		client->inactivityWarning = qfalse;
		client->inactivityTime    = level.time + 1000 * inactivity;
		return qtrue;
	}

	// no inactivity for localhost
	if (client->pers.localClient)
	{
		return qtrue;
	}

	// start countdown
	if (!client->inactivityWarning)
	{
		int secondsLeft;

		if (level.time > client->inactivityTime - inactivity)
		{
			client->inactivityWarning     = qtrue;
			client->inactivityTime        = level.time + 1000 * inactivity;
			client->inactivitySecondsLeft = inactivity;
		}

		secondsLeft = (client->inactivityTime + inactivity - level.time) / 1000;

		// countdown expired..
		if (secondsLeft <= 0)
		{
			CPx(client - level.clients, "cp \"^3Dropped for inactivity\n\"");
		}
		// display a message at 30 seconds, and countdown at last 10 ..
		else if (secondsLeft <= 10 || secondsLeft == 30)
		{
			CPx(client - level.clients, va("cp \"^1%i ^3seconds until inactivity drop\n\"", secondsLeft));
		}
	}
	else
	{
		G_Printf("Spectator dropped for inactivity: %s\n", client->pers.netname);
		trap_DropClient(client - level.clients, "Dropped due to inactivity", 0);
		return qfalse;
	}

	// do not kick by default
	return qtrue;
}

/**
 * @brief TVG_ClientIntermissionThink
 * @param[in,out] client Client
 */
void TVG_ClientIntermissionThink(gclient_t *client)
{
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons    = client->pers.cmd.buttons;

	client->oldwbuttons = client->wbuttons;
	client->wbuttons    = client->pers.cmd.wbuttons;
}

/**
 * @brief This will be called once for each client frame, which will
 * usually be a couple times for each server frame on fast clients.
 * @param[in] client
 */
void TVG_ClientThink_real(gclient_t *client)
{
	usercmd_t *ucmd;
	int       msec, i;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED)
	{
		return;
	}

	// mark the time, so the connection sprite can be removed
	ucmd = &client->pers.cmd;

	client->ps.identifyClient = ucmd->identClient;

	// sanity check the command time to prevent speedup cheating
	if (ucmd->serverTime > level.time + 200)
	{
		ucmd->serverTime = level.time + 200;
	}
	if (ucmd->serverTime < level.time - 1000)
	{
		ucmd->serverTime = level.time - 1000;
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if (msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW)
	{
		return;
	}

	TVG_SendMatchInfo(client);

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	// moved here to allow for spec inactivity checks as well
	if (!TVG_ClientInactivityTimer(client))
	{
		return;
	}

	for (i = 0; i < INFO_NUM; i++)
	{
		if (client->wantsInfoStats[i].requested && level.cmds.infoStats[i].valid[client->wantsInfoStats[i].requestedClientNum])
		{
			trap_SendServerCommand(client - level.clients, level.cmds.infoStats[i].data[client->wantsInfoStats[i].requestedClientNum]);
			client->wantsInfoStats[i].requested = qfalse;
		}
	}

	if (level.intermission)
	{
		TVG_ClientIntermissionThink(client);
		return;
	}

	if (client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		TVG_SpectatorThink(client, ucmd);
	}
}

/**
 * @brief TVG_ClientThink_cmd
 * @param[in] client
 * @param[in] cmd
 */
void TVG_ClientThink_cmd(gclient_t *client, usercmd_t *cmd)
{
	client->pers.oldcmd = client->pers.cmd;
	client->pers.cmd    = *cmd;
	TVG_ClientThink_real(client);
}

/**
 * @brief A new command has arrived from the client
 * @param[in] clientNum Client Number from 0 to g_maxclients
 */
void TVG_ClientThink(int clientNum)
{
	gclient_t *client = level.clients + clientNum;
	usercmd_t newcmd;

	trap_GetUsercmd(clientNum, &newcmd);
	TVG_ClientThink_cmd(client, &newcmd);
}

/**
 * @brief TVSpectatorClientEndFrame
 * @param[in,out] client
 */
void TVG_SpectatorClientEndFrame(gclient_t *client)
{
	if (level.intermission && client->ps.pm_type != PM_INTERMISSION)
	{
		// take out of follow mode if needed
		if (client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			TVG_StopFollowing(client);
		}

		client->ps.pm_type = PM_INTERMISSION;
		VectorCopy(level.intermission_origin, client->ps.origin);
		VectorCopy(level.intermission_angle, client->ps.viewangles);
	}

	// if we are doing a chase cam or a remote view, grab the latest info
	if (client->sess.spectatorState == SPECTATOR_FOLLOW)
	{
		if (client->sess.spectatorClient >= 0)
		{
			if (level.ettvMasterClients[client->sess.spectatorClient].valid)
			{
				playerState_t *ps        = &level.ettvMasterClients[client->sess.spectatorClient].ps;
				int           flags      = (ps->eFlags & ~(EF_VOTED | EF_READY)) | (client->ps.eFlags & (EF_VOTED | EF_READY));
				int           ping       = client->ps.ping;
				int           savedScore = client->ps.persistant[PERS_SCORE];

				client->ps                        = *ps;
				client->ps.pm_flags              |= PMF_FOLLOW;
				client->ps.persistant[PERS_SCORE] = savedScore;
				client->ps.eFlags                 = flags;
				client->ps.ping                   = ping;

				return;
			}
		}

		client->sess.spectatorState = SPECTATOR_FREE;
		TVG_ClientBegin(client - level.clients);
	}
}

/**
 * @brief Called at the end of each server frame for each connected client
 * A fast client will have multiple TVG_ClientThink for each TVG_ClientEndFrame,
 * while a slow client may have multiple TVG_ClientEndFrame between TVG_ClientThink.
 *
 * @param[in,out] ent Entity
 */
void TVG_ClientEndFrame(gclient_t *client)
{
	// check for flood protection - if 1 second has passed between commands, reduce the flood limit counter
	if (level.time >= client->sess.nextCommandDecreaseTime && client->sess.numReliableCommands)
	{
		client->sess.numReliableCommands--;
		client->sess.nextCommandDecreaseTime = level.time + 1000;
	}

	if (client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		TVG_SpectatorClientEndFrame(client);
	}
}
