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
 * @file tvg_active.c
 */

#include "tvg_local.h"

/**
 * @brief G_SpectatorAttackFollow
 * @param client
 * @return true if a player was found to follow, otherwise false
 */
qboolean G_SpectatorAttackFollow(gclient_t *client)
{
	trace_t       tr;
	vec3_t        forward, right, up;
	vec3_t        start, end;
	vec3_t        mins, maxs;
	static vec3_t enlargeMins = { -64.0f, -64.0f, -48.0f };
	static vec3_t enlargeMaxs = { 64.0f, 64.0f, 0.0f };

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

	trap_Trace(&tr, start, mins, maxs, end, client - level.clients, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE);

	if (tr.entityNum < MAX_CLIENTS && level.ettvMasterClients[tr.entityNum].valid)
	{
		client->sess.spectatorState  = SPECTATOR_FOLLOW;
		client->sess.spectatorClient = tr.entityNum;
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief SpectatorThink
 * @param[in,out] ent
 * @param[in] ucmd
 */
void SpectatorThink(gclient_t *client, usercmd_t *ucmd)
{
	//gentity_t *crosshairEnt = &g_entities[ent->client->ps.identifyClient];

	//// sanity check - check .active in case the client sends us something completely bogus

	//if (crosshairEnt->inuse && crosshairEnt->client &&
	//    (ent->client->sess.sessionTeam == crosshairEnt->client->sess.sessionTeam ||
	//     crosshairEnt->client->ps.powerups[PW_OPS_DISGUISED]))
	//{

	//	// identifyClientHealth sent as unsigned char, so we
	//	// can't transmit negative numbers
	//	if (crosshairEnt->health >= 0)
	//	{
	//		ent->client->ps.identifyClientHealth = crosshairEnt->health;
	//	}
	//	else
	//	{
	//		ent->client->ps.identifyClientHealth = 0;
	//	}
	//}

	if (client->sess.spectatorState != SPECTATOR_FOLLOW)
	{
		pmove_t pm;

		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed   = 800; // was: 400 // faster than normal
		if (client->ps.sprintExertTime)
		{
			client->ps.speed *= 3;  // allow sprint in free-cam mode
		}
		// dead players are frozen too, in a timeout
		if ((client->ps.pm_flags & PMF_LIMBO) && level.match_pause != PAUSE_NONE)
		{
			client->ps.pm_type = PM_FREEZE;
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

		Pmove(&pm);

		// Activate - made it a latched event (occurs on keydown only)
		//if (client->latched_buttons & BUTTON_ACTIVATE)
		//{
		//	Cmd_Activate_f(ent);
		//}

		//// save results of pmove
		//VectorCopy(client->ps.origin, ent->s.origin);

		//G_TouchTriggers(ent);
		//trap_UnlinkEntity(ent);
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
		/*if (G_SpectatorAttackFollow(client))
		{
			return;
		}
		else*/
		{
			// not clicked on a player?.. then follow next,
			// to prevent constant traces done by server.
			if (client->buttons & BUTTON_SPRINT)
			{
				Cmd_FollowCycle_f(client, 1, qtrue);
			}
			// no humans playing?.. then follow a bot
			if (client->sess.spectatorState != SPECTATOR_FOLLOW)
			{
				Cmd_FollowCycle_f(client, 1, qfalse);
			}
		}
		// attack + sprint button cycles through non-bot/human players
		// attack button cycles through all players
	}
	else if ((client->buttons & BUTTON_ATTACK) && !(client->oldbuttons & BUTTON_ATTACK) &&
	         !(client->buttons & BUTTON_ACTIVATE))
	{
		Cmd_FollowCycle_f(client, 1, (client->buttons & BUTTON_SPRINT));
	}
	else if (client->sess.sessionTeam == TEAM_SPECTATOR && client->sess.spectatorState == SPECTATOR_FOLLOW &&
	         (((client->buttons & BUTTON_ACTIVATE) && !(client->oldbuttons & BUTTON_ACTIVATE)) || ucmd->upmove > 0))
	{
		StopFollowing(client);
	}
}

/**
 * @brief
 * g_inactivity and g_spectatorinactivity :
 * Values have to be higher then 10 seconds, that is the time after the warn message is sent.
 * (if it's lower than that value, it will not work at all)
 *
 * @param[in,out] client Client
 *
 * @return Returns qfalse if the client is dropped
 */
qboolean ClientInactivityTimer(gclient_t *client)
{
	int      inactivity     = G_InactivityValue;
	int      inactivityspec = G_SpectatorInactivityValue;
	qboolean inTeam         = (client->sess.sessionTeam == TEAM_ALLIES || client->sess.sessionTeam == TEAM_AXIS) ? qtrue : qfalse;

	qboolean doDrop = (g_spectatorInactivity.integer && (g_maxclients.integer - level.numNonSpectatorClients <= 0)) ? qtrue : qfalse;

	// no countdown in warmup and intermission
	if (g_gamestate.integer != GS_PLAYING)
	{
		return qtrue;
	}

	// inactivity settings disabled?
	if (g_inactivity.integer == 0 && g_spectatorInactivity.integer == 0)
	{
		// Give everyone some time, so if the operator sets g_inactivity or g_spectatorinactivity during
		// gameplay, everyone isn't kicked or moved to spectators
		client->inactivityTime    = level.time + 60000;
		client->inactivityWarning = qfalse;
		return qtrue;
	}

	// the client is still active?
	if (client->pers.cmd.forwardmove || client->pers.cmd.rightmove || client->pers.cmd.upmove ||
	    (client->pers.cmd.wbuttons & (WBUTTON_LEANLEFT | WBUTTON_LEANRIGHT))  ||
	    (client->pers.cmd.buttons & BUTTON_ATTACK) ||
	    BG_PlayerMounted(client->ps.eFlags) ||
	    ((client->ps.eFlags & EF_PRONE) && (client->ps.weapon == WP_MOBILE_MG42_SET || client->ps.weapon == WP_MOBILE_BROWNING_SET)) ||
	    (client->ps.pm_flags & PMF_LIMBO) || (client->ps.pm_type == PM_DEAD) ||
	    client->sess.shoutcaster)
	{
		client->inactivityWarning = qfalse;

		if (inTeam)
		{
			client->inactivityTime = level.time + 1000 * inactivity;
		}
		else
		{
			client->inactivityTime = level.time + 1000 * inactivityspec;
		}
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
		if (g_inactivity.integer && (level.time > client->inactivityTime - inactivity) && inTeam)
		{
			client->inactivityWarning     = qtrue;
			client->inactivityTime        = level.time + 1000 * inactivity;
			client->inactivitySecondsLeft = inactivity;
		}
		// if a player will not be kicked from the server (because there are still free slots),
		// do not display messages for inactivity-drop/kick.
		else if (doDrop && g_spectatorInactivity.integer &&
		         (level.time > client->inactivityTime - inactivityspec) && !inTeam)
		{
			client->inactivityWarning     = qtrue;
			client->inactivityTime        = level.time + 1000 * inactivityspec;
			client->inactivitySecondsLeft = inactivityspec;
		}

		if (g_inactivity.integer && inTeam)
		{
			int secondsLeft = (client->inactivityTime + inactivity - level.time) / 1000;

			// countdown expired..
			if (secondsLeft <= 0)
			{
				CPx(client - level.clients, "cp \"^3Moved to spectator for inactivity\n\"");
			}
			// display a message at 30 seconds, and countdown at last 10 ..
			else if (secondsLeft <= 10 || secondsLeft == 30)
			{
				CPx(client - level.clients, va("cp \"^1%i ^3seconds until moving to spectator for inactivity\n\"", secondsLeft));
			}
		}
		else if (doDrop && g_spectatorInactivity.integer && !inTeam)
		{
			int secondsLeft = (client->inactivityTime + inactivityspec - level.time) / 1000;

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
	}
	else
	{
		if (inTeam && g_inactivity.integer)
		{
			client->inactivityWarning     = qfalse;
			client->inactivityTime        = level.time + 1000 * inactivityspec;
			client->inactivitySecondsLeft = inactivityspec;

			G_Printf("Moved to spectator for inactivity: %s\n", client->pers.netname);
		}
		else if (doDrop && g_spectatorInactivity.integer && !inTeam)
		{
			// slots occupied by bots should be considered "free",
			// because bots will disconnect if a human player connects..
			G_Printf("Spectator dropped for inactivity: %s\n", client->pers.netname);
			trap_DropClient(client - level.clients, "Dropped due to inactivity", 0);
			return qfalse;
		}
	}

	// do not kick by default
	return qtrue;
}

/**
 * @brief Actions that happen once a second
 * @param[in,out] ent  Entity
 * @param         msec Scheduler time
 */
void ClientTimerActions(gentity_t *ent, int msec)
{
	gclient_t *client = ent->client;

	client->timeResidual += msec;

	while (client->timeResidual >= 1000)
	{
		client->timeResidual -= 1000;

		// regenerate
		if (ent->health < client->ps.stats[STAT_MAX_HEALTH])
		{
			// medic only
			if (client->sess.playerType == PC_MEDIC)
			{
				if (ent->health > client->ps.stats[STAT_MAX_HEALTH] / 1.11)
				{
					ent->health += 2;

					if (ent->health > client->ps.stats[STAT_MAX_HEALTH])
					{
						ent->health = client->ps.stats[STAT_MAX_HEALTH];
					}
				}
				else
				{
					ent->health += 3;
					if (ent->health > client->ps.stats[STAT_MAX_HEALTH] / 1.1)
					{
						ent->health = client->ps.stats[STAT_MAX_HEALTH] / 1.1;
					}
				}
			}

		}
		else if (ent->health > client->ps.stats[STAT_MAX_HEALTH])               // count down health when over max
		{
			ent->health--;
		}
	}
}

/**
 * @brief ClientIntermissionThink
 * @param[in,out] client Client
 */
void ClientIntermissionThink(gclient_t *client)
{
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons    = client->pers.cmd.buttons;

	client->oldwbuttons = client->wbuttons;
	client->wbuttons    = client->pers.cmd.wbuttons;
}

/**
 * @brief This will be called once for each client frame, which will
 * usually be a couple times for each server frame on fast clients.
 *
 * If "g_synchronousClients 1" is set, this will be called exactly
 * once for each server frame, which makes for smooth demo recording.
 *
 * @param[in] client
 */
void TVClientThink_real(gclient_t *client)
{
	usercmd_t *ucmd;
	int msec, i;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED)
	{
		return;
	}

	client->ps.ammo[WP_ARTY] = 0;

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
	if (msec > 200)
	{
		msec = 200;
	}

	// pmove fix
	if (pmove_msec.integer < 8)
	{
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33)
	{
		trap_Cvar_Set("pmove_msec", "33");
	}

	// zinx etpro antiwarp
	client->pers.pmoveMsec = pmove_msec.integer;
	if (pmove_fixed.integer || client->pers.pmoveFixed)
	{
		ucmd->serverTime = ((ucmd->serverTime + client->pers.pmoveMsec - 1) /
		                    client->pers.pmoveMsec) * client->pers.pmoveMsec;
	}

	// check for exiting intermission
	//if (level.intermissiontime)
	//{
	//	ClientIntermissionThink(client);
	//	return;
	//}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	// moved here to allow for spec inactivity checks as well
	if (!ClientInactivityTimer(client))
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

	// spectators don't do much
	// In limbo use SpectatorThink
	if (client->sess.sessionTeam == TEAM_SPECTATOR || (client->ps.pm_flags & PMF_LIMBO))
	{
		SpectatorThink(client, ucmd);
	}
}

/**
 * @brief etpro antiwarp
 * @author zinx
 * @param[in,out] client
 * @param[in]     cmd User Command
 */
void TVClientThink_cmd(gclient_t *client, usercmd_t *cmd)
{
	client->pers.oldcmd = client->pers.cmd;
	client->pers.cmd    = *cmd;
	TVClientThink_real(client);
}

/**
 * @brief A new command has arrived from the client
 * @param[in] clientNum Client Number from 0 to MAX_CLIENTS
 */
void TVClientThink(int clientNum)
{
	gclient_t *client = level.clients + clientNum;
	usercmd_t newcmd;

	trap_GetUsercmd(clientNum, &newcmd);
	TVClientThink_cmd(client, &newcmd);
}

/**
 * @brief TVSpectatorClientEndFrame
 * @param[in,out] client
 */
void TVSpectatorClientEndFrame(gclient_t *client)
{
	if (level.intermission)
	{
		client->ps.pm_type = PM_INTERMISSION;
		VectorCopy(level.ettvMasterPs.origin, client->ps.origin);
		VectorCopy(level.ettvMasterPs.viewangles, client->ps.viewangles);
	}

	// if we are doing a chase cam or a remote view, grab the latest info
	if (client->sess.spectatorState == SPECTATOR_FOLLOW || (client->ps.pm_flags & PMF_LIMBO))
	{
		gclient_t *cl;	

		if (client->sess.spectatorClient >= 0)
		{
			cl = &level.clients[client->sess.spectatorClient];
			if (level.ettvMasterClients[client->sess.spectatorClient].valid)
			{
				playerState_t *ps = &level.ettvMasterClients[client->sess.spectatorClient].ps;
				int flags      = (ps->eFlags & ~(EF_VOTED | EF_READY)) | (client->ps.eFlags & (EF_VOTED | EF_READY));
				int ping       = client->ps.ping;
			    int savedScore = client->ps.persistant[PERS_SCORE];

				client->ps                        = *ps;
				client->ps.pm_flags              |= PMF_FOLLOW;
				client->ps.persistant[PERS_SCORE] = savedScore;
				client->ps.eFlags                 = flags;
				client->ps.ping                   = ping;

				return;
			}
		}

		client->sess.spectatorState = SPECTATOR_FREE;
		TVClientBegin(client - level.clients);
	}
}

/**
 * @brief Called at the end of each server frame for each connected client
 * A fast client will have multiple TVClientThink for each TVClientEndFrame,
 * while a slow client may have multiple TVClientEndFrame between TVClientThink.
 *
 * @param[in,out] ent Entity
 */
void TVClientEndFrame(gclient_t *client)
{
	if (g_gamestate.integer == GS_PLAYING && level.match_pause == PAUSE_NONE)
	{
		// count player time
		if (!(client->ps.persistant[PERS_RESPAWNS_LEFT] == 0 && (client->ps.pm_flags & PMF_LIMBO)))
		{
			if (client->sess.sessionTeam == TEAM_AXIS)
			{
				client->sess.time_axis += level.frameTime;
			}
			else if (client->sess.sessionTeam == TEAM_ALLIES)
			{
				client->sess.time_allies += level.frameTime;
			}
		}

		// don't count skulled player time
		if (!(client->sess.sessionTeam == TEAM_SPECTATOR || (client->ps.pm_flags & PMF_LIMBO) || client->ps.stats[STAT_HEALTH] <= 0))
		{
			// ensure time played is always smaller or equal than time spent in teams
			// work around for unreset data of slow connecters
			if (client->sess.time_played > (client->sess.time_axis + client->sess.time_allies))
			{
				client->sess.time_played = 0;
			}
			client->sess.time_played += level.frameTime;
		}
	}

	// used for informing of speclocked teams.
	// Zero out here and set only for certain specs
	client->ps.powerups[PW_BLACKOUT] = 0;

	// check for flood protection - if 1 second has passed between commands, reduce the flood limit counter
	if (level.time >= client->sess.nextCommandDecreaseTime && client->sess.numReliableCommands)
	{
		client->sess.numReliableCommands--;
		client->sess.nextCommandDecreaseTime = level.time + 1000;
	}

	if ((client->sess.sessionTeam == TEAM_SPECTATOR) || (client->ps.pm_flags & PMF_LIMBO))
	{
		TVSpectatorClientEndFrame(client);
	}
}
