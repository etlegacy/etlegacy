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
 * @file g_referee.c
 * @brief Referee handling
 */

#include "g_local.h"
#include "../../etmain/ui/menudef.h"

/**
 * @brief Parses for a referee command.
 * ref arg allows for the server console to utilize all referee commands (ent == NULL)
 * @param[in] ent
 * @param[in] cmd
 * @return
 */
qboolean G_refCommandCheck(gentity_t *ent, const char *cmd)
{
	if (!Q_stricmp(cmd, "allready"))
	{
		G_refAllReady_cmd(ent);
	}
	else if (!Q_stricmp(cmd, "lock"))
	{
		G_refLockTeams_cmd(ent, qtrue);
	}
	else if (!Q_stricmp(cmd, "help"))
	{
		G_refHelp_cmd(ent);
	}
	else if (!Q_stricmp(cmd, "pause"))
	{
		G_refPause_cmd(ent, qtrue);
	}
	else if (!Q_stricmp(cmd, "putallies"))
	{
		G_refPlayerPut_cmd(ent, TEAM_ALLIES);
	}
	else if (!Q_stricmp(cmd, "putaxis"))
	{
		G_refPlayerPut_cmd(ent, TEAM_AXIS);
	}
	else if (!Q_stricmp(cmd, "remove"))
	{
		G_refRemove_cmd(ent);
	}
	else if (!Q_stricmp(cmd, "speclock"))
	{
		G_refSpeclockTeams_cmd(ent, qtrue);
	}
	else if (!Q_stricmp(cmd, "specunlock"))
	{
		G_refSpeclockTeams_cmd(ent, qfalse);
	}
	else if (!Q_stricmp(cmd, "unlock"))
	{
		G_refLockTeams_cmd(ent, qfalse);
	}
	else if (!Q_stricmp(cmd, "unpause"))
	{
		G_refPause_cmd(ent, qfalse);
	}
	else if (!Q_stricmp(cmd, "warmup"))
	{
		G_refWarmup_cmd(ent);
	}
	else if (!Q_stricmp(cmd, "warn"))
	{
		G_refWarning_cmd(ent);
	}
	else if (!Q_stricmp(cmd, "mute"))
	{
		G_refMute_cmd(ent, qtrue);
	}
	else if (!Q_stricmp(cmd, "unmute"))
	{
		G_refMute_cmd(ent, qfalse);
	}
	else if (!Q_stricmp(cmd, "makeShoutcaster") || !Q_stricmp(cmd, "makesc"))
	{
		G_refMakeShoutcaster_cmd(ent);
	}
	else if (!Q_stricmp(cmd, "removeShoutcaster") || !Q_stricmp(cmd, "removesc"))
	{
		G_refRemoveShoutcaster_cmd(ent);
	}
	else if (!Q_stricmp(cmd, "logout"))
	{
		G_refLogout_cmd(ent);
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Lists ref commands.
 * @param[in] ent
 */
void G_refHelp_cmd(gentity_t *ent)
{
	// List commands only for enabled refs.
	if (ent)
	{
		CP("print \"^3Referee commands:^7\n------------------------------------------\n\"");

		G_voteHelp(ent, qfalse);

		CP("print \"^5allready putallies^7 <pid> ^5specunlock warn ^7<pid>\n\"");
		CP("print \"^5help putaxis^7 <pid> ^5unlock mute ^7<pid>\n\"");
		CP("print \"^5lock remove^7 <pid> ^5unpause unmute ^7<pid>\n\"");
		CP("print \"^5pause speclock logout warmup ^7[value]\n\"");
		CP("print \"^5makeshoutcaster^7 <pid> ^5removeshoutcaster^7 <pid>\n\"");

		CP("print \"Usage: ^3\\ref <cmd> [params]\n\n\"");

		// Help for the console
	}
	else
	{
		G_Printf("\nAdditional console commands:\n----------------------------------------------\n");
		G_Printf("allready putallies <pid> unpause\n");
		G_Printf("help putaxis <pid> warmup [value]\n");
		G_Printf("lock speclock warn <pid>\n");
		G_Printf("pause specunlock\n");
		G_Printf("remove <pid> unlock\n\n");

		G_Printf("Usage: <cmd> [params]\n\n");
	}
}

/**
 * @brief Request for ref status or lists ref commands.
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param fValue - unused
 */
void G_ref_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	char arg[MAX_TOKEN_CHARS];

	// Roll through ref commands if already a ref
	if (ent == NULL || ent->client->sess.referee)
	{
		voteInfo_t votedata;

		trap_Argv(1, arg, sizeof(arg));

		Com_Memcpy(&votedata, &level.voteInfo, sizeof(voteInfo_t));

		if (Cmd_CallVote_f(ent, 0, qtrue))
		{
			Com_Memcpy(&level.voteInfo, &votedata, sizeof(voteInfo_t));
			return;
		}
		else
		{
			Com_Memcpy(&level.voteInfo, &votedata, sizeof(voteInfo_t));

			if (G_refCommandCheck(ent, arg))
			{
				return;
			}
			else
			{
				G_refHelp_cmd(ent);
			}
		}
		return;
	}

	if (ent)
	{
		if (!Q_stricmp(refereePassword.string, "none") || !refereePassword.string[0])
		{
			CP("print \"Sorry, referee status disabled on this server.\n\"");
			return;
		}

		if (trap_Argc() < 2)
		{
			CP("print \"Usage: ref [password]\n\"");
			return;
		}

		trap_Argv(1, arg, sizeof(arg));

		if (Q_stricmp(arg, refereePassword.string))
		{
			CP("print \"Invalid referee password!\n\"");
			return;
		}

		ent->client->sess.referee     = 1;
		ent->client->sess.spec_invite = TEAM_AXIS | TEAM_ALLIES;
		AP(va("cp \"%s\n^3has become a referee\n\"", ent->client->pers.netname));
		ClientUserinfoChanged(ent - g_entities);
	}
}

/**
 * @brief Readies all players in the game.
 * @param[in] ent
 */
void G_refAllReady_cmd(gentity_t *ent)
{
	int       i;
	gclient_t *cl;

	if (g_gamestate.integer == GS_PLAYING)
	{
		// allow allready in intermission
		G_refPrintf(ent, "Match already in progress!");
		return;
	}

	// Ready them all and lock the teams
	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];
		if (cl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			cl->pers.ready = qtrue;
		}
	}

	// Can we start?
	level.ref_allready = qtrue;
	G_readyMatchState();
}

/**
 * @brief Changes team lock status
 * @param[in] ent
 * @param[in] fLock
 */
void G_refLockTeams_cmd(gentity_t *ent, qboolean fLock)
{
	char *status;

	teamInfo[TEAM_AXIS].team_lock   = (TeamCount(-1, TEAM_AXIS)) ? fLock : qfalse;
	teamInfo[TEAM_ALLIES].team_lock = (TeamCount(-1, TEAM_ALLIES)) ? fLock : qfalse;

	status = va("Referee has ^3%sLOCKED^7 teams", ((fLock) ? "" : "UN"));

	G_printFull(status, NULL);
	G_refPrintf(ent, "You have %sLOCKED teams", ((fLock) ? "" : "UN"));

	if (fLock)
	{
		level.server_settings |= CV_SVS_LOCKTEAMS;
	}
	else
	{
		level.server_settings &= ~CV_SVS_LOCKTEAMS;
	}
	trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
}

/**
 * @brief Pause/unpause a match.
 * @param[in] ent
 * @param[in] fPause
 */
void G_refPause_cmd(gentity_t *ent, qboolean fPause)
{
	char *status[2] = { "^5UN", "^1" };
	char *referee   = (ent) ? "Referee" : "ref";

	if ((PAUSE_UNPAUSING >= level.match_pause && !fPause) || (PAUSE_NONE != level.match_pause && fPause))
	{
		G_refPrintf(ent, "The match is already %sPAUSED!", status[fPause]);
		return;
	}

	if (ent && !G_cmdDebounce(ent, ((fPause) ? "pause" : "unpause")))
	{
		return;
	}

	if (g_gamestate.integer != GS_PLAYING)
	{
		// generic output for pause/unpause/timeouts ...
		G_refPrintf(ent, "Command not available - match isn't in progress!");
		return;
	}

	// Trigger the auto-handling of pauses
	if (fPause)
	{
		level.match_pause = 100 + ((ent) ? (1 + ent - g_entities) : 0);
		G_globalSoundEnum(GAMESOUND_MISC_REFEREE);
		G_spawnPrintf(DP_PAUSEINFO, level.time + 15000, NULL);
		AP(va("print \"^3%s ^1PAUSED^3 the match^3!\n", referee));
		AP(va("cp \"^3Match is ^1PAUSED^3! (^7%s^3)\n\"", referee));
		level.server_settings |= CV_SVS_PAUSE;
		trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
	}
	else
	{
		AP(va("print \"^3%s ^5UNPAUSES^3 the match ... resuming in 10 seconds!\n\"", referee));
		level.match_pause = PAUSE_UNPAUSING;
		G_globalSound("sound/osp/prepare.wav");
		G_spawnPrintf(DP_UNPAUSING, level.time + 10, NULL);
		return;
	}
}

/**
 * @brief Puts a player on a team.
 * @param[in] ent
 * @param[in] team_id
 */
void G_refPlayerPut_cmd(gentity_t *ent, team_t team_id)
{
	int       pid;
	char      arg[MAX_TOKEN_CHARS];
	gentity_t *player;

	// Works for teamplayish matches
	if (g_gametype.integer < GT_WOLF)
	{
		G_refPrintf(ent, "\"put[allies|axis]\" only for team-based games!");
		return;
	}

	// Find the player to place.
	trap_Argv(2, arg, sizeof(arg));
	if ((pid = ClientNumberFromString(ent, arg)) == -1)
	{
		return;
	}

	player = g_entities + pid;

	// Can only move to other teams.
	if (player->client->sess.sessionTeam == team_id)
	{
		G_refPrintf(ent, "\"%s\" is already on team %s!", player->client->pers.netname, aTeams[team_id]);
		return;
	}

	if (team_maxplayers.integer && TeamCount(-1, team_id) >= team_maxplayers.integer)
	{
		G_refPrintf(ent, "Sorry, the %s team is already full!", aTeams[team_id]);
		return;
	}

	player->client->pers.invite = team_id;
	player->client->pers.ready  = qfalse;

	if (team_id == TEAM_AXIS)
	{
		SetTeam(player, "red", qtrue, WP_NONE, WP_NONE, qfalse);
	}
	else
	{
		SetTeam(player, "blue", qtrue, WP_NONE, WP_NONE, qfalse);
	}

	if (g_gamestate.integer == GS_WARMUP || g_gamestate.integer == GS_WARMUP_COUNTDOWN)
	{
		G_readyMatchState();
	}
}

/**
 * @brief Removes a player from a team.
 * @param[in] ent
 */
void G_refRemove_cmd(gentity_t *ent)
{
	int       pid;
	char      arg[MAX_TOKEN_CHARS];
	gentity_t *player;

	// Works for teamplayish matches
	if (g_gametype.integer < GT_WOLF)
	{
		G_refPrintf(ent, "\"remove\" only for team-based games!");
		return;
	}

	// Find the player to remove.
	trap_Argv(2, arg, sizeof(arg));
	if ((pid = ClientNumberFromString(ent, arg)) == -1)
	{
		return;
	}

	player = g_entities + pid;

	// Can only remove active players.
	if (player->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		G_refPrintf(ent, "You can only remove people in the game!");
		return;
	}

	// Announce the removal
	AP(va("cp \"%s\n^7removed from team %s\n\"", player->client->pers.netname, aTeams[player->client->sess.sessionTeam]));
	CPx(pid, va("print \"^5You've been removed from the %s team\n\"", aTeams[player->client->sess.sessionTeam]));

	SetTeam(player, "s", qtrue, WP_NONE, WP_NONE, qfalse);

	if (g_gamestate.integer == GS_WARMUP || g_gamestate.integer == GS_WARMUP_COUNTDOWN)
	{
		G_readyMatchState();
	}
}

/**
 * @brief Changes team spectator lock status
 * @param ent - unused
 * @param[in] fLock
 */
void G_refSpeclockTeams_cmd(gentity_t *ent, qboolean fLock)
{
	char *status;

	// Ensure proper locking
	G_updateSpecLock(TEAM_AXIS, (TeamCount(-1, TEAM_AXIS)) ? fLock : qfalse);
	G_updateSpecLock(TEAM_ALLIES, (TeamCount(-1, TEAM_ALLIES)) ? fLock : qfalse);

	status = va("Referee has ^3SPECTATOR %sLOCKED^7 teams", ((fLock) ? "" : "UN"));

	G_printFull(status, NULL);

	if (fLock)
	{
		level.server_settings |= CV_SVS_LOCKSPECS;
	}
	else
	{
		level.server_settings &= ~CV_SVS_LOCKSPECS;
	}
	trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
}

/**
 * @brief G_refWarmup_cmd
 * @param[in] ent
 */
void G_refWarmup_cmd(gentity_t *ent)
{
	char cmd[MAX_TOKEN_CHARS];

	trap_Argv(2, cmd, sizeof(cmd));

	if (!*cmd || Q_atoi(cmd) < 0)
	{
		trap_Cvar_VariableStringBuffer("g_warmup", cmd, sizeof(cmd));
		G_refPrintf(ent, "Warmup Time: %d", Q_atoi(cmd));
		return;
	}

	trap_Cvar_Set("g_warmup", va("%d", Q_atoi(cmd)));
}

/**
 * @brief G_refWarning_cmd
 * @param[in] ent
 */
void G_refWarning_cmd(gentity_t *ent)
{
	char cmd[MAX_TOKEN_CHARS];
	char reason[MAX_TOKEN_CHARS];
	int  kicknum;

	trap_Argv(2, cmd, sizeof(cmd));

	if (!*cmd)
	{
		G_refPrintf(ent, "usage: ref warn <clientname> [reason].");
		return;
	}

	trap_Argv(3, reason, sizeof(reason));

	kicknum = G_refClientnumForName(ent, cmd);

	if (kicknum != MAX_CLIENTS)
	{
		if (level.clients[kicknum].sess.referee == RL_NONE || ((!ent || ent->client->sess.referee == RL_RCON) && level.clients[kicknum].sess.referee <= RL_REFEREE))
		{
			trap_SendServerCommand(-1, va("cpm \"%s^7 was issued a ^1Warning^7 (%s)\n\"\n", level.clients[kicknum].pers.netname, *reason ? reason : "No Reason Supplied"));
		}
		else
		{
			G_refPrintf(ent, "Insufficient rights to issue client a warning.");
		}
	}
}

/**
 * @brief (Un)Mutes a player
 * @param[in] ent
 * @param[in] mute
 */
void G_refMute_cmd(gentity_t *ent, qboolean mute)
{
	int       pid;
	char      arg[MAX_TOKEN_CHARS];
	gentity_t *player;

	// Find the player to mute.
	trap_Argv(2, arg, sizeof(arg));
	if ((pid = ClientNumberFromString(ent, arg)) == -1)
	{
		return;
	}

	player = g_entities + pid;

	// mute check so that players that are muted
	// before granted referee status, can be unmuted
	if (player->client->sess.referee != RL_NONE && mute)
	{
		G_refPrintf(ent, "Cannot mute a referee.");
		return;
	}

	if (player->client->sess.muted == mute)
	{
		G_refPrintf(ent, "\"%s^*\" %s", player->client->pers.netname, mute ? "is already muted!" : "is not muted!");
		return;
	}

	if (mute)
	{
		CPx(pid, "print \"^5You've been muted\n\"");
		player->client->sess.muted = qtrue;
		G_Printf("\"%s^*\" has been muted\n", player->client->pers.netname);
	}
	else
	{
		CPx(pid, "print \"^5You've been unmuted\n\"");
		player->client->sess.muted = qfalse;
		G_Printf("\"%s^*\" has been unmuted\n", player->client->pers.netname);
	}
	ClientUserinfoChanged(pid);
}

/**
 * @brief G_refLogout_cmd
 * @param[in] ent
 */
void G_refLogout_cmd(gentity_t *ent)
{
	if (ent && ent->client && ent->client->sess.referee == RL_REFEREE)
	{
		ent->client->sess.referee = RL_NONE;
		ClientUserinfoChanged(ent->s.clientNum);
		CP("print \"You have been logged out\n\"");
	}
}

/**
 * @brief Client authentication
 * @param[in,out] ent
 */
void Cmd_AuthRcon_f(gentity_t *ent)
{
	char buf[MAX_TOKEN_CHARS], cmd[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("rconPassword", buf, sizeof(buf));
	trap_Argv(1, cmd, sizeof(cmd));

	if (*buf && !strcmp(buf, cmd))
	{
		ent->client->sess.referee = RL_RCON;
	}
}

/**
 * Console admin commands
 */

/**
 * @brief G_PlayerBan
 */
void G_PlayerBan()
{
	char cmd[MAX_TOKEN_CHARS];
	int  bannum;

	trap_Argv(1, cmd, sizeof(cmd));

	if (!*cmd)
	{
		G_Printf("usage: ban <clientname>.");
		return;
	}

	bannum = G_refClientnumForName(NULL, cmd);

	if (bannum != MAX_CLIENTS)
	{
		//if( level.clients[bannum].sess.referee != RL_RCON ) {

		if (!(g_entities[bannum].r.svFlags & SVF_BOT))
		{
			const char *value;
			char       userinfo[MAX_INFO_STRING];

			trap_GetUserinfo(bannum, userinfo, sizeof(userinfo));
			value = Info_ValueForKey(userinfo, "ip");

			AddIPBan(value);
		}
		else
		{
			G_Printf("^3*** Can't ban a bot!\n");
		}
	}
}

/**
 * @brief G_MakeReferee
 */
void G_MakeReferee()
{
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if (!*cmd)
	{
		G_Printf("usage: MakeReferee <clientname>.");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if (cnum != MAX_CLIENTS)
	{
		if (level.clients[cnum].sess.referee == RL_NONE)
		{
			level.clients[cnum].sess.referee = RL_REFEREE;
			AP(va("cp \"%s\n^3has been made a referee\n\"", cmd));
			G_Printf("%s has been made a referee.\n", cmd);
			if (level.clients[cnum].sess.muted)
			{
				trap_SendServerCommand(cnum, va("cpm \"^2You have been un-muted\""));
				level.clients[cnum].sess.muted = qfalse;
			}
			ClientUserinfoChanged(cnum);
		}
		else
		{
			G_Printf("User is already authed.\n");
		}
	}
}

/**
 * @brief G_RemoveReferee
 */
void G_RemoveReferee()
{
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if (!*cmd)
	{
		G_Printf("usage: RemoveReferee <clientname>.");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if (cnum != MAX_CLIENTS)
	{
		if (level.clients[cnum].sess.referee == RL_REFEREE)
		{
			level.clients[cnum].sess.referee = RL_NONE;
			G_Printf("%s is no longer a referee.\n", cmd);
			ClientUserinfoChanged(cnum);
		}
		else
		{
			G_Printf("User is not a referee.\n");
		}
	}
}

/**
 * @brief G_MakeShoutcaster
 * @param[in] ent
 */
void G_MakeShoutcaster(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	// move the player to spectators
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		SetTeam(ent, "spectator", qtrue, -1, -1, qfalse);
	}

	ent->client->sess.shoutcaster = 1;
	ent->client->sess.spec_invite = TEAM_AXIS | TEAM_ALLIES;
	AP(va("cp \"%s\n^3has become a shoutcaster\n\"", ent->client->pers.netname));
	ClientUserinfoChanged(ent - g_entities);
}

/**
 * @brief G_RemoveShoutcaster
 * @param[in] ent
 */
void G_RemoveShoutcaster(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	ent->client->sess.shoutcaster = 0;

	if (!ent->client->sess.referee)    // don't remove referee's invitation
	{
		ent->client->sess.spec_invite = 0;

		// unfollow player if team is spec locked
		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			int spectatorClientTeam = level.clients[ent->client->sess.spectatorClient].sess.sessionTeam;

			if (spectatorClientTeam == TEAM_AXIS && teamInfo[TEAM_AXIS].spec_lock)
			{
				StopFollowing(ent);
			}
			else if (spectatorClientTeam == TEAM_ALLIES && teamInfo[TEAM_ALLIES].spec_lock)
			{
				StopFollowing(ent);
			}
		}
	}

	ClientUserinfoChanged(ent - g_entities);
}

/**
 * @brief G_RemoveAllShoutcasters
 */
void G_RemoveAllShoutcasters(void)
{
	int i;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];

		if (cl->sess.shoutcaster)
		{
			cl->sess.shoutcaster = 0;

			// don't remove referee's invitation
			if (!cl->sess.referee)
			{
				cl->sess.spec_invite = 0;
			}

			ClientUserinfoChanged(cl - level.clients);
		}
	}
}

/**
 * @brief G_MuteClient
 */
void G_MuteClient()
{
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if (!*cmd)
	{
		G_Printf("usage: Mute <clientname>.");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if (cnum != MAX_CLIENTS)
	{
		if (level.clients[cnum].sess.referee != RL_RCON)
		{
			trap_SendServerCommand(cnum, va("cpm \"^3You have been muted\""));
			level.clients[cnum].sess.muted = qtrue;
			G_Printf("%s^* has been muted\n", cmd);
			ClientUserinfoChanged(cnum);
		}
		else
		{
			G_Printf("Cannot mute a referee.\n");
		}
	}
}

/**
 * @brief G_UnMuteClient
 */
void G_UnMuteClient()
{
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if (!*cmd)
	{
		G_Printf("usage: Unmute <clientname>.\n");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if (cnum != MAX_CLIENTS)
	{
		if (level.clients[cnum].sess.muted)
		{
			trap_SendServerCommand(cnum, va("cpm \"^2You have been un-muted\""));
			level.clients[cnum].sess.muted = qfalse;
			G_Printf("%s has been un-muted\n", cmd);
			ClientUserinfoChanged(cnum);
		}
		else
		{
			G_Printf("User is not muted.\n");
		}
	}
}

/**
 * @brief G_IsShoutcastPasswordSet
 * @return
 */
qboolean G_IsShoutcastPasswordSet(void)
{
	if (Q_stricmp(shoutcastPassword.string, "none") && shoutcastPassword.string[0])
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief G_IsShoutcastStatusAvailable
 * @param[in] ent
 * @return
 */
qboolean G_IsShoutcastStatusAvailable(gentity_t *ent)
{
	if (ent->r.svFlags & SVF_BOT)
	{
		return qfalse;
	}

	// check for available password
	if (G_IsShoutcastPasswordSet())
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief G_refMakeShoutcaster_cmd
 * @param[in] ent
 */
void G_refMakeShoutcaster_cmd(gentity_t *ent)
{
	int       pid;
	char      name[MAX_NAME_LENGTH];
	gentity_t *player;

	if (trap_Argc() != 3)
	{
		G_refPrintf(ent, "Usage: \\ref makeShoutcaster <pid>");
		return;
	}

	if (!G_IsShoutcastPasswordSet())
	{
		G_refPrintf(ent, "Sorry, shoutcaster status disabled on this server.");
		return;
	}

	trap_Argv(2, name, sizeof(name));

	if ((pid = ClientNumberFromString(ent, name)) == -1)
	{
		return;
	}

	player = g_entities + pid;

	if (!player || !player->client)
	{
		return;
	}

	// ignore bots
	if (player->r.svFlags & SVF_BOT)
	{
		G_refPrintf(ent, "Sorry, a bot can not be a shoutcaster.");
		return;
	}

	if (player->client->sess.shoutcaster)
	{
		G_refPrintf(ent, "Sorry, %s^7 is already a shoutcaster.", player->client->pers.netname);
		return;
	}

	G_MakeShoutcaster(player);
}

/**
 * @brief G_refRemoveShoutcaster_cmd
 * @param[in] ent
 */
void G_refRemoveShoutcaster_cmd(gentity_t *ent)
{
	int       pid;
	char      name[MAX_NAME_LENGTH];
	gentity_t *player;

	if (trap_Argc() != 3)
	{
		G_refPrintf(ent, "Usage: \\ref removeShoutcaster <pid>");
		return;
	}

	if (!G_IsShoutcastPasswordSet())
	{
		G_refPrintf(ent, "Sorry, shoutcaster status disabled on this server.");
		return;
	}

	trap_Argv(2, name, sizeof(name));

	if ((pid = ClientNumberFromString(ent, name)) == -1)
	{
		return;
	}

	player = g_entities + pid;

	if (!player || !player->client)
	{
		return;
	}

	if (!player->client->sess.shoutcaster)
	{
		G_refPrintf(ent, "Sorry, %s^7 is not a shoutcaster.", player->client->pers.netname);
		return;
	}

	G_RemoveShoutcaster(player);
}

/**
 * Utility
 */

/**
 * @brief G_refClientnumForName
 * @param[in] ent
 * @param[in] name
 * @return
 */
int G_refClientnumForName(gentity_t *ent, const char *name)
{
	char cleanName[MAX_TOKEN_CHARS];
	int  i;

	if (!*name)
	{
		return MAX_CLIENTS;
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		Q_strncpyz(cleanName, level.clients[level.sortedClients[i]].pers.netname, sizeof(cleanName));
		Q_CleanStr(cleanName);
		if (!Q_stricmp(cleanName, name))
		{
			return(level.sortedClients[i]);
		}
	}

	G_refPrintf(ent, "Client not on server.");

	return MAX_CLIENTS;
}

/**
 * @brief G_refPrintf
 * @param[in] ent
 * @param[in] fmt
 */
void G_refPrintf(gentity_t *ent, const char *fmt, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	if (ent == NULL)
	{
		trap_Printf(va("%s\n", text));
	}
	else
	{
		CP(va("print \"%s\n\"", text));
	}
}
