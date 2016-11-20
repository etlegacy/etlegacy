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
 * @file g_cmds_ext.c
 * @brief Extended command set handling
 */

#include "g_local.h"
#include "../../etmain/ui/menudef.h"

int iWeap = WS_MAX;

char *lock_status[2] = { "unlock", "lock" };

// Update info:
//  1. Add line to aCommandInfo w/appropriate info
//  2. Add implementation for specific command (see an existing command for an example)
//
typedef struct
{
	char *pszCommandName;
	qboolean fAnytime;
	qboolean fValue;
	void (*pCommand)(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
	const char *pszHelpInfo;
} cmd_reference_t;

// VC optimizes for dup strings :)
static const cmd_reference_t aCommandInfo[] =
{
	{ "+stats",         qtrue,  qtrue,  NULL,                  ":^7 HUD overlay showing current weapon stats info"                                          },
	{ "+topshots",      qtrue,  qtrue,  NULL,                  ":^7 HUD overlay showing current top accuracies of all players"                              },
	{ "+objectives",    qtrue,  qtrue,  NULL,                  ":^7 HUD overlay showing current objectives info"                                            },
	{ "?",              qtrue,  qtrue,  G_commands_cmd,        ":^7 Gives a list of commands"                                                               },
	{ "autorecord",     qtrue,  qtrue,  NULL,                  ":^7 Creates a demo with a consistent naming scheme"                                         },
	{ "autoscreenshot", qtrue,  qtrue,  NULL,                  ":^7 Creates a screenshot with a consistent naming scheme"                                   },
	{ "bottomshots",    qtrue,  qfalse, G_weaponRankings_cmd,  ":^7 Shows WORST player for each weapon. Add ^3<weapon_ID>^7 to show all stats for a weapon" },
	{ "callvote",       qtrue,  qfalse, (void (*)(gentity_t *, unsigned int, qboolean))Cmd_CallVote_f, " <params>:^7 Calls a vote"                          },
	{ "commands",       qtrue,  qtrue,  G_commands_cmd,        ":^7 Gives a list of commands"                                                               },
	{ "currenttime",    qtrue,  qtrue,  NULL,                  ":^7 Displays current local time"                                                            },
	{ "follow",         qfalse, qtrue,  Cmd_Follow_f,          " <player_ID|allies|axis>:^7 Spectates a particular player or team"                          },
//  { "invite",         qtrue,  qtrue,  NULL, " <player_ID>:^7 Invites a player to join a team" },
	{ "lock",           qtrue,  qtrue,  G_lock_cmd,            ":^7 Locks a player's team to prevent others from joining"                                   },
	{ "notready",       qtrue,  qfalse, G_ready_cmd,           ":^7 Sets your status to ^5not ready^7 to start a match"                                     },
	{ "pause",          qfalse, qtrue,  G_pause_cmd,           ":^7 Allows a team to pause a match"                                                         },
	{ "players",        qtrue,  qtrue,  G_players_cmd,         ":^7 Lists all active players and their IDs/information"                                     },
	{ "ready",          qtrue,  qtrue,  G_ready_cmd,           ":^7 Sets your status to ^5ready^7 to start a match"                                         },
	{ "readyteam",      qfalse, qtrue,  G_teamready_cmd,       ":^7 Sets an entire team's status to ^5ready^7 to start a match"                             },
	{ "ref",            qtrue,  qtrue,  G_ref_cmd,             " <password>:^7 Become a referee (admin access)"                                             },
//  { "remove",         qtrue,  qtrue,  NULL, " <player_ID>:^7 Removes a player from the team" },
	{ "say_teamnl",     qtrue,  qtrue,  G_say_teamnl_cmd,      "<msg>:^7 Sends a team chat without location info"                                           },
	{ "scores",         qtrue,  qtrue,  G_scores_cmd,          ":^7 Displays current match stat info"                                                       },
	{ "specinvite",     qtrue,  qtrue,  G_specinvite_cmd,      ":^7 Invites a player to spectate a speclock'ed team"                                        },
	{ "speclock",       qtrue,  qtrue,  G_speclock_cmd,        ":^7 Locks a player's team from spectators"                                                  },
//  { "speconly",       qtrue,  qtrue,  NULL, ":^7 Toggles option to stay as a spectator in 1v1" },
	{ "specunlock",     qtrue,  qfalse, G_speclock_cmd,        ":^7 Unlocks a player's team from spectators"                                                },
	{ "statsall",       qtrue,  qfalse, G_statsall_cmd,        ":^7 Shows weapon accuracy stats for all players"                                            },
	{ "statsdump",      qtrue,  qtrue,  NULL,                  ":^7 Shows player stats + match info saved locally to a file"                                },
	{ "stoprecord",     qtrue,  qtrue,  NULL,                  ":^7 Stops a demo recording currently in progress"                                           },
	{ "team",           qtrue,  qtrue,  Cmd_Team_f,            " <b|r|s|none>:^7 Joins a team (b = allies, r = axis, s = spectator)"                        },
	{ "timein",         qfalse, qfalse, G_pause_cmd,           ":^7 Unpauses a match (if initiated by the issuing team)"                                    },
	{ "timeout",        qfalse, qtrue,  G_pause_cmd,           ":^7 Allows a team to pause a match"                                                         },
	{ "topshots",       qtrue,  qtrue,  G_weaponRankings_cmd,  ":^7 Shows BEST player for each weapon. Add ^3<weapon_ID>^7 to show all stats for a weapon"  },
	{ "unlock",         qtrue,  qfalse, G_lock_cmd,            ":^7 Unlocks a player's team, allowing others to join"                                       },
	{ "unpause",        qfalse, qfalse, G_pause_cmd,           ":^7 Unpauses a match (if initiated by the issuing team)"                                    },
	{ "unready",        qtrue,  qfalse, G_ready_cmd,           ":^7 Sets your status to ^5not ready^7 to start a match"                                     },
	{ "weaponstats",    qtrue,  qfalse, G_weaponStats_cmd,     " [player_ID]:^7 Shows weapon accuracy stats for a player"                                   },
#ifdef FEATURE_MULTIVIEW
	{ "mvwadd",         qfalse, qtrue,  NULL,                  " <player_ID>:^7 Adds a player to multi-screen view"                                         },
	{ "mvallies",       qfalse, qtrue,  NULL,                  ": ^7 Views entire allies/axis team"                                                         },
	{ "mvaxis",         qfalse, qtrue,  NULL,                  ": ^7 Views entire allies/axis team"                                                         },
	{ "mvnone",         qfalse, qtrue,  NULL,                  ":^7 Disables multiview mode and goes back to spectator mode"                                },
	{ "mvdel",          qfalse, qtrue,  NULL,                  " [player_ID]:^7 Removes current selected or specific player from multi-screen view"         },
#endif
	{ 0,                qfalse, qtrue,  NULL,                  0                                                                                            }
};

// Commands
qboolean G_commandCheck(gentity_t *ent, char *cmd, qboolean fDoAnytime)
{
	unsigned int          i, cCommands = sizeof(aCommandInfo) / sizeof(aCommandInfo[0]);
	const cmd_reference_t *pCR;

	for (i = 0; i < cCommands; i++)
	{
		pCR = &aCommandInfo[i];
		if (NULL != pCR->pCommand && pCR->fAnytime == fDoAnytime && 0 == Q_stricmp(cmd, pCR->pszCommandName))
		{
			if (!G_commandHelp(ent, cmd, i))
			{
				pCR->pCommand(ent, i, pCR->fValue);
			}
			return qtrue;
		}
	}

#ifdef FEATURE_MULTIVIEW
	return(G_smvCommands(ent, cmd));
#else
	return qfalse;
#endif
}

// Prints specific command help info.
qboolean G_commandHelp(gentity_t *ent, char *pszCommand, unsigned int dwCommand)
{
	char arg[MAX_TOKEN_CHARS];

	if (!ent)
	{
		return qfalse;
	}
	trap_Argv(1, arg, sizeof(arg));
	if (!Q_stricmp(arg, "?"))
	{
		CP(va("print \"\n^3%s%s\n\n\"", pszCommand, aCommandInfo[dwCommand].pszHelpInfo));
		return qtrue;
	}

	return qfalse;
}

// Debounces cmd request as necessary.
qboolean G_cmdDebounce(gentity_t *ent, const char *pszCommandName)
{
	if (ent->client->pers.cmd_debounce > level.time)
	{
		CP(va("print \"Wait another %.1fs to issue ^3%s\n\"", 1.0 * (float)(ent->client->pers.cmd_debounce - level.time) / 1000.0,
		      pszCommandName));
		return qfalse;
	}

	ent->client->pers.cmd_debounce = level.time + CMD_DEBOUNCE;
	return qtrue ;
}

void G_noTeamControls(gentity_t *ent)
{
	CP("cpm \"Team commands not enabled on this server.\n\"");
}

////////////////////////////////////////////////////////////////////////////
/////
/////           Match Commands
/////
/////

// ************** COMMANDS / ?
//
// Lists server commands.
void G_commands_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	int i, rows, num_cmds = sizeof(aCommandInfo) / sizeof(aCommandInfo[0]) - 1;

	rows = num_cmds / HELP_COLUMNS;
	if (num_cmds % HELP_COLUMNS)
	{
		rows++;
	}
	if (rows < 0)
	{
		return;
	}

	CP("print \"^5\nAvailable Game Commands:\n------------------------\n\"");
	for (i = 0; i < rows; i++)
	{
		if (i + rows * 3 + 1 <= num_cmds)
		{
			CP(va("print \"^3%-17s%-17s%-17s%-17s\n\"", aCommandInfo[i].pszCommandName,
			      aCommandInfo[i + rows].pszCommandName,
			      aCommandInfo[i + rows * 2].pszCommandName,
			      aCommandInfo[i + rows * 3].pszCommandName));
		}
		else if (i + rows * 2 + 1 <= num_cmds)
		{
			CP(va("print \"^3%-17s%-17s%-17s\n\"", aCommandInfo[i].pszCommandName,
			      aCommandInfo[i + rows].pszCommandName,
			      aCommandInfo[i + rows * 2].pszCommandName));
		}
		else if (i + rows + 1 <= num_cmds)
		{
			CP(va("print \"^3%-17s%-17s\n\"", aCommandInfo[i].pszCommandName, aCommandInfo[i + rows].pszCommandName));
		}
		else
		{
			CP(va("print \"^3%-17s\n\"", aCommandInfo[i].pszCommandName));
		}
	}

	CP("print \"\nType: ^3\\command_name ?^7 for more information\n\"");
}

// Locks/unlocks a player's team.
void G_lock_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fLock)
{
	if (team_nocontrols.integer)
	{
		G_noTeamControls(ent);
		return;
	}
	if (!G_cmdDebounce(ent, aCommandInfo[dwCommand].pszCommandName))
	{
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES)
	{
		if (teamInfo[ent->client->sess.sessionTeam].team_lock == fLock)
		{
			CP(va("print \"^3Your team is already %sed!\n\"", lock_status[fLock]));
		}
		else
		{
			char *info;

			info = va("\"The %s team is now %sed!\n\"", aTeams[ent->client->sess.sessionTeam], lock_status[fLock]);

			teamInfo[ent->client->sess.sessionTeam].team_lock = fLock;
			AP(va("print %s", info));
			AP(va("cp %s", info));
		}
	}
	else
	{
		CP(va("print \"Spectators can't %s a team!\n\"", lock_status[fLock]));
	}
}

// Pause/unpause a match.
void G_pause_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fPause)
{
	char *status[2] = { "^5UN", "^1" };

	if (team_nocontrols.integer)
	{
		G_noTeamControls(ent);
		return;
	}

	if (g_gamestate.integer != GS_PLAYING)
	{
		// generic output for pause/unpause/timeouts ...
		CP("print \"Command not available - match isn't in progress!\n\"");
		return;
	}

	if ((PAUSE_UNPAUSING >= level.match_pause && !fPause) || (PAUSE_NONE != level.match_pause && fPause))
	{
		CP(va("print \"The match is already %sPAUSED^7!\n\"", status[fPause]));
		return;
	}

	// Alias for referees
	if (ent->client->sess.referee)
	{
		G_refPause_cmd(ent, fPause);
	}
	else
	{
		int tteam = ent->client->sess.sessionTeam;

		if (!G_cmdDebounce(ent, aCommandInfo[dwCommand].pszCommandName))
		{
			return;
		}

		// Trigger the auto-handling of pauses
		if (fPause)
		{
			if (0 == teamInfo[tteam].timeouts)
			{
				CP("cpm \"^3Your team has no more timeouts remaining!\n\"");
				return;
			}
			else
			{
				teamInfo[tteam].timeouts--;
				level.match_pause = tteam + 128;
				G_globalSoundEnum(GAMESOUND_MISC_REFEREE);
				G_spawnPrintf(DP_PAUSEINFO, level.time + 15000, NULL);
				AP(va("print \"^3Match is ^1PAUSED^3!\n^7[%s^7: - %d Timeouts Remaining]\n\"", aTeams[tteam], teamInfo[tteam].timeouts));
				AP(va("cp \"^3Match is ^1PAUSED^3! (%s^3)\n\"", aTeams[tteam]));
				level.server_settings |= CV_SVS_PAUSE;
				trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
			}
		}
		else if (tteam + 128 != level.match_pause)
		{
			CP("cpm \"^3Your team didn't call the timeout!\n\"");
			return;
		}
		else
		{
			AP("print \"^3Match is ^5UNPAUSED^3 ... resuming in 10 seconds!\n\"");
			level.match_pause = PAUSE_UNPAUSING;
			G_globalSound("sound/osp/prepare.wav");
			G_spawnPrintf(DP_UNPAUSING, level.time + 10, NULL);
		}
	}
}

// Show client info
void G_players_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	int       i, idnum, max_rate, cnt = 0;
	int       user_rate, user_snaps;
	gclient_t *cl;
	gentity_t *cl_ent;
	char      n2[MAX_NETNAME], ready[16], ref[8], rate[32];
	char      *s, *tc, *ign, *muted, userinfo[MAX_INFO_STRING];

	if (g_gamestate.integer == GS_PLAYING)
	{
		if (ent)
		{
			CP("print \"^7 ID : Player                    Nudge  Rate  MaxPkts  Snaps  Specials\n\"");
			CP("print \"^7----------------------------------------------------------------------\n\"");
		}
		else
		{
			G_Printf(" ID : Player                    Nudge  Rate  MaxPkts  Snaps  Specials\n");
			G_Printf("---------------------------------------------------------------------\n");
		}
	}
	else
	{
		if (ent)
		{
			CP("print \"^7Status   : ID : Player                    Nudge  Rate  MaxPkts  Snaps  Specials\n\"");
			CP("print \"^7-------------------------------------------------------------------------------\n\"");
		}
		else
		{
			G_Printf("Status   : ID : Player                    Nudge  Rate  MaxPkts  Snaps  Specials\n");
			G_Printf("-------------------------------------------------------------------------------\n");
		}
	}

	max_rate = trap_Cvar_VariableIntegerValue("sv_maxrate");

	for (i = 0; i < level.numConnectedClients; i++)
	{
		idnum  = level.sortedClients[i]; //level.sortedNames[i];
		cl     = &level.clients[idnum];
		cl_ent = g_entities + idnum;

		SanitizeString(cl->pers.netname, n2, qfalse);
		n2[26]   = 0;
		ref[0]   = 0;
		ready[0] = 0;

		// Rate info
		if (cl_ent->r.svFlags & SVF_BOT)
		{
			Q_strncpyz(rate, va("%s%s%s%s", "[BOT]", " -----", "       --", "     --"), sizeof(rate));
		}
		else if (cl->pers.connected == CON_CONNECTING)
		{
			Q_strncpyz(rate, va("%s", "^3>>> CONNECTING <<<^7"), sizeof(rate));
		}
		else
		{
			trap_GetUserinfo(idnum, userinfo, sizeof(userinfo));
			s          = Info_ValueForKey(userinfo, "rate");
			user_rate  = (max_rate > 0 && atoi(s) > max_rate) ? max_rate : atoi(s);
			s          = Info_ValueForKey(userinfo, "snaps");
			user_snaps = atoi(s);

			Q_strncpyz(rate, va("%5d%6d%9d%7d", cl->pers.clientTimeNudge, user_rate, cl->pers.clientMaxPackets, user_snaps), sizeof(rate));
		}

		if (g_gamestate.integer != GS_PLAYING)
		{
			if (cl->sess.sessionTeam == TEAM_SPECTATOR || cl->pers.connected == CON_CONNECTING)
			{
				strcpy(ready, ((ent) ? "^5--------^7 :" : "-------- :"));
			}
			else if (cl->pers.ready || (g_entities[idnum].r.svFlags & SVF_BOT))
			{
				strcpy(ready, ((ent) ? "^3(READY)^7  :" : "(READY)  :"));
			}
			else
			{
				strcpy(ready, ((ent) ? "^7NOTREADY^7 :" : "NOTREADY :"));
			}
		}

		if (cl->sess.referee && !(cl_ent->r.svFlags & SVF_BOT))
		{
			strcpy(ref, "REF ");
		}

		if (ent && COM_BitCheck(ent->client->sess.ignoreClients, idnum))
		{
			ign = "IGN ";
		}
		else
		{
			ign = "";
		}

		if (cl->sess.muted)
		{
			muted = "MUT ";
		}
		else
		{
			muted = "";
		}

		tc = (ent) ? "^7 " : " ";
		if (g_gametype.integer >= GT_WOLF)
		{
			if (cl->sess.sessionTeam == TEAM_AXIS)
			{
				tc = (ent) ? "^1X^7" : "X";
			}
			else if (cl->sess.sessionTeam == TEAM_ALLIES)
			{
				tc = (ent) ? "^4L^7" : "L";
			}
			else if (cl->sess.sessionTeam == TEAM_SPECTATOR)
			{
				tc = (ent) ? "^2S^7" : "S";
			}
			else // unknown
			{
				tc = (ent) ? "^2U^7" : "U";
			}
		}

		if (ent)
		{
			CP(va("print \"%s%s%2d:%s %-26s^7%s  ^3%s%s%s^7\n\"", ready, tc, idnum, ((ref[0]) ? "^3" : "^7"), n2, rate, ref, ign, muted));
		}
		else
		{
			G_Printf("%s%s%2d: %-26s%s  %s%s\n", ready, tc, idnum, n2, rate, ref, muted);
		}

		cnt++;
	}

	if (ent)
	{
		CP(va("print \"\n^3%2d^7 total players\n\n\"", cnt));
	}
	else
	{
		G_Printf("\n%2d total players\n\n", cnt);
	}

	// Team speclock info
	if (g_gametype.integer >= GT_WOLF)
	{
		for (i = TEAM_AXIS; i <= TEAM_ALLIES; i++)
		{
			if (teamInfo[i].spec_lock)
			{
				if (ent)
				{
					CP(va("print \"** %s team is speclocked.\n\"", aTeams[i]));
				}
				else
				{
					G_Printf("** %s team is speclocked.\n", aTeams[i]);
				}
			}
		}
	}
}

// Sets a player's "ready" status.
void G_ready_cmd(gentity_t *ent, unsigned int dwCommand, qboolean state)
{
	char *status[2] = { " NOT", "" };

	if (g_gamestate.integer == GS_PLAYING || g_gamestate.integer == GS_INTERMISSION)
	{
		CP("cpm \"Match is already in progress!\n\"");
		return;
	}

	if (!state && g_gamestate.integer == GS_WARMUP_COUNTDOWN)
	{
		CP("cpm \"Countdown started.... ^3notready^7 ignored!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cpm \"You must be in the game to be ^3ready^7!\n\"");
		return;
	}

	// Can't ready until enough players.
	if (level.numPlayingClients < match_minplayers.integer)
	{
		CP("cpm \"Not enough players to start match!\n\"");
		return;
	}

	if (!G_cmdDebounce(ent, aCommandInfo[dwCommand].pszCommandName))
	{
		return;
	}

	// Move them to correct ready state
	if (ent->client->pers.ready == state)
	{
		CP(va("print \"You are already%s ready!\n\"", status[state]));
	}
	else
	{
		ent->client->pers.ready = state;
		if (!level.intermissiontime)
		{
			if (state)
			{
				G_MakeReady(ent);
			}
			else
			{
				G_MakeUnready(ent);
			}

			AP(va("print \"%s^7 is%s ready!\n\"", ent->client->pers.netname, status[state]));
			AP(va("cp \"\n%s\n^3is%s ready!\n\"", ent->client->pers.netname, status[state]));
		}
	}

	G_readyMatchState();
}

// Team chat w/no location info
void G_say_teamnl_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	Cmd_Say_f(ent, SAY_TEAMNL, qfalse);
}

// Shows match stats to the requesting client.
void G_scores_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	G_printMatchInfo(ent);
}

// Sends an invitation to a player to spectate a team.
void G_specinvite_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fLock)
{
	gentity_t *player;
	char      arg[MAX_TOKEN_CHARS];

	if (team_nocontrols.integer)
	{
		G_noTeamControls(ent);
		return;
	}
	if (!G_cmdDebounce(ent, aCommandInfo[dwCommand].pszCommandName))
	{
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES)
	{
		int pid;

		if (!teamInfo[ent->client->sess.sessionTeam].spec_lock)
		{
			CP("cpm \"Your team isn't locked from spectators!\n\"");
			return;
		}

		// Find the player to invite.
		trap_Argv(1, arg, sizeof(arg));
		if ((pid = ClientNumberFromString(ent, arg)) == -1)
		{
			return;
		}

		player = g_entities + pid;

		// Can't invite self
		if (player->client == ent->client)
		{
			CP("cpm \"You can't specinvite yourself!\n\"");
			return;
		}

		// Can't invite an active player.
		if (player->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			CP("cpm \"You can't specinvite a non-spectator!\n\"");
			return;
		}

		player->client->sess.spec_invite |= ent->client->sess.sessionTeam;

		// Notify sender/recipient
		CP(va("print \"%s^7 has been sent a spectator invitation.\n\"", player->client->pers.netname));
		G_printFull(va("*** You've been invited to spectate the %s team!", aTeams[ent->client->sess.sessionTeam]), player);

	}
	else
	{
		CP("cpm \"Spectators can't specinvite players!\n\"");
	}
}

// Locks/unlocks a player's team from spectators.
void G_speclock_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fLock)
{
	if (team_nocontrols.integer)
	{
		G_noTeamControls(ent);
		return;
	}

	if (!G_cmdDebounce(ent, aCommandInfo[dwCommand].pszCommandName))
	{
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES)
	{
		if (teamInfo[ent->client->sess.sessionTeam].spec_lock == fLock)
		{
			CP(va("print \"\n^3Your team is already %sed from spectators!\n\n\"", lock_status[fLock]));
		}
		else
		{
			G_printFull(va("The %s team is now %sed from spectators", aTeams[ent->client->sess.sessionTeam], lock_status[fLock]), NULL);
			G_updateSpecLock(ent->client->sess.sessionTeam, fLock);
			if (fLock)
			{
				CP("cpm \"Use ^3specinvite^7 to invite people to spectate.\n\"");
			}
		}
	}
	else
	{
		CP(va("print \"Spectators can't %s a team from spectators!\n\"", lock_status[fLock]));
	}
}

// Shows a player's stats to the requesting client.
void G_weaponStats_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump)
{
	G_statsPrint(ent, 0);
}

// Shows all players' stats to the requesting client.
void G_statsall_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump)
{
	int       i;
	gentity_t *player;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		player = &g_entities[level.sortedClients[i]];
		if (player->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}
		CP(va("ws %s\n", G_createStats(player)));
	}
}

// Sets a player's team "ready" status.
void G_teamready_cmd(gentity_t *ent, unsigned int dwCommand, qboolean state)
{
	int       i;
	gclient_t *cl;

	if (g_gamestate.integer == GS_PLAYING || g_gamestate.integer == GS_INTERMISSION)
	{
		CP("cpm \"Match is already in progress!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cpm \"Spectators can't ready a team!\n\"");
		return;
	}

	// Can't ready until enough players.
	if (level.numPlayingClients < match_minplayers.integer)
	{
		CP("cpm \"Not enough players to start match!\n\"");
		return;
	}

	if (!G_cmdDebounce(ent, aCommandInfo[dwCommand].pszCommandName))
	{
		return;
	}

	// Move them to correct ready state
	for (i = 0; i < level.numPlayingClients; i++)
	{
		cl = level.clients + level.sortedClients[i];
		if (cl->sess.sessionTeam == ent->client->sess.sessionTeam)
		{
			cl->pers.ready = qtrue;

			G_MakeReady(ent);
		}
	}

	G_printFull(va("The %s team is ready!", aTeams[ent->client->sess.sessionTeam]), NULL);
	G_readyMatchState();
}

// These map to WS_* weapon indexes
const unsigned int cQualifyingShots[WS_MAX] =
{
	10,     // 0  WS_KNIFE
	10,     // 1  WS_KNIFE_KBAR
	15,     // 2  WS_LUGER
	15,     // 3  WS_COLT
	30,     // 4  WS_MP40
	30,     // 5  WS_THOMPSON
	30,     // 6  WS_STEN
	30,     // 7  WS_FG42
	3,      // 8  WS_PANZERFAUST
	3,      // 9  WS_BAZOOKA
	100,    // 10 WS_FLAMETHROWER
	5,      // 11 WS_GRENADE
	5,      // 12 WS_MORTAR
	5,      // 13 WS_MORTAR2
	5,      // 14 WS_DYNAMITE
	3,      // 15 WS_AIRSTRIKE
	3,      // 16 WS_ARTILLERY
	3,      // 17 WS_SATCHEL
	5,      // 18 WS_GRENADELAUNCHER
	10,     // 19 WS_LANDMINE
	100,    // 20 WS_MG42
	100,    // 21 WS_BROWNING
	30,     // 22 WS_CARBINE
	30,     // 23 WS_KAR98
	30,     // 24 WS_GARAND
	30,     // 25 WS_K43
};

// Gives back overall or specific weapon rankings
int QDECL SortStats(const void *a, const void *b)
{
	gclient_t *ca, *cb;
	float     accA, accB;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// then connecting clients
	if (ca->pers.connected == CON_CONNECTING)
	{
		return 1;
	}
	if (cb->pers.connected == CON_CONNECTING)
	{
		return -1;
	}

	if (ca->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return 1;
	}
	if (cb->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return -1;
	}

	if ((ca->sess.aWeaponStats[iWeap].atts) < cQualifyingShots[iWeap])
	{
		return 1;
	}
	if ((cb->sess.aWeaponStats[iWeap].atts) < cQualifyingShots[iWeap])
	{
		return -1;
	}

	accA = (float)(ca->sess.aWeaponStats[iWeap].hits * 100.0) / (float)(ca->sess.aWeaponStats[iWeap].atts);
	accB = (float)(cb->sess.aWeaponStats[iWeap].hits * 100.0) / (float)(cb->sess.aWeaponStats[iWeap].atts);

	// then sort by accuracy
	if (accA > accB)
	{
		return -1;
	}
	return 1;
}

// Shows the most accurate players for each weapon to the requesting client
void G_weaponStatsLeaders_cmd(gentity_t *ent, qboolean doTop, qboolean doWindow)
{
	int             i, iWeap, wBestAcc, cClients, cPlaces;
	unsigned        shots;
	int             aClients[MAX_CLIENTS];
	float           acc;
	char            z[MAX_STRING_CHARS];
	const gclient_t *cl;

	z[0] = 0;
	for (iWeap = WS_KNIFE; iWeap < WS_MAX; iWeap++)
	{
		wBestAcc = (doTop) ? 0 : 99999;
		cClients = 0;
		cPlaces  = 0;

		// suckfest - needs two passes, in case there are ties
		for (i = 0; i < level.numConnectedClients; i++)
		{
			cl = &level.clients[level.sortedClients[i]];

			if (cl->sess.sessionTeam == TEAM_SPECTATOR)
			{
				continue;
			}

			shots = cl->sess.aWeaponStats[iWeap].atts;
			if (shots >= cQualifyingShots[iWeap])
			{
				acc                  = (float)((cl->sess.aWeaponStats[iWeap].hits) * 100.0f) / (float)shots;
				aClients[cClients++] = level.sortedClients[i];
				if (((doTop) ? acc : (float)wBestAcc) > ((doTop) ? wBestAcc : acc))
				{
					wBestAcc = (int)acc;
					cPlaces++;
				}
			}
		}

		if (!doTop && cPlaces < 2)
		{
			continue;
		}

		for (i = 0; i < cClients; i++)
		{
			cl  = &level.clients[aClients[i]];
			acc = (float)(cl->sess.aWeaponStats[iWeap].hits * 100.0f) / (float)(cl->sess.aWeaponStats[iWeap].atts);

			if (((doTop) ? acc : (float)wBestAcc + 0.999f) >= ((doTop) ? wBestAcc : acc))
			{
				Q_strcat(z, sizeof(z), va(" %d %d %d %d %d %d", iWeap + 1, aClients[i],
				                          cl->sess.aWeaponStats[iWeap].hits,
				                          cl->sess.aWeaponStats[iWeap].atts,
				                          cl->sess.aWeaponStats[iWeap].kills,
				                          cl->sess.aWeaponStats[iWeap].deaths));
			}
		}
	}
	CP(va("%sbstats%s %s 0", ((doWindow) ? "w" : ""), ((doTop) ? "" : "b"), z));
}

// Shows best/worst accuracy for all weapons, or sorted
// accuracies for a single weapon.
void G_weaponRankings_cmd(gentity_t *ent, unsigned int dwCommand, qboolean state)
{
	gclient_t *cl;
	int       c = 0, i, wBestAcc;
	unsigned  shots;
	char      z[MAX_STRING_CHARS];

	if (trap_Argc() < 2)
	{
		G_weaponStatsLeaders_cmd(ent, state, qfalse);
		return;
	}

	wBestAcc = (state) ? 0 : 99999;

	// Find the weapon
	trap_Argv(1, z, sizeof(z));
	if ((iWeap = atoi(z)) == 0 || iWeap < WS_KNIFE || iWeap >= WS_MAX)
	{
		for (iWeap = WS_MAX - 1; iWeap >= WS_KNIFE; iWeap--)
		{
			if (!Q_stricmp(z, aWeaponInfo[iWeap].pszCode))
			{
				break;
			}
		}
	}

	if (iWeap < WS_KNIFE)
	{
		G_commandHelp(ent, (state) ? "topshots" : "bottomshots", dwCommand);

		Q_strncpyz(z, "^3Available weapon codes:^7\n", sizeof(z));
		for (i = WS_KNIFE; i < WS_MAX; i++)
		{
			Q_strcat(z, sizeof(z), va("  %s - %s\n", aWeaponInfo[i].pszCode, aWeaponInfo[i].pszName));
		}
		CP(va("print \"%s\"", z));
		return;
	}

	memcpy(&level.sortedStats, &level.sortedClients, sizeof(level.sortedStats));
	qsort(level.sortedStats, level.numConnectedClients, sizeof(level.sortedStats[0]), SortStats);

	z[0] = 0;
	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = &level.clients[level.sortedStats[i]];

		if (cl->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}

		shots = cl->sess.aWeaponStats[iWeap].atts;
		if (shots >= cQualifyingShots[iWeap])
		{
			float acc = (float)(cl->sess.aWeaponStats[iWeap].hits * 100.0f) / (float)shots;

			c++;
			wBestAcc = (((state) ? acc : wBestAcc) > ((state) ? wBestAcc : acc)) ? (int)acc : wBestAcc;
			Q_strcat(z, sizeof(z), va(" %d %d %d %d %d", level.sortedStats[i],
			                          cl->sess.aWeaponStats[iWeap].hits,
			                          shots,
			                          cl->sess.aWeaponStats[iWeap].kills,
			                          cl->sess.aWeaponStats[iWeap].deaths));
		}
	}

	CP(va("astats%s %d %d %d%s", ((state) ? "" : "b"), c, iWeap, wBestAcc, z));
}
