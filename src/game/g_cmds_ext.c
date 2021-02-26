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
 * @file g_cmds_ext.c
 * @brief Extended command set handling
 */

#include "g_local.h"
#include "../../etmain/ui/menudef.h"

int iWeap = WS_MAX;

const char *lock_status[2] = { "unlock", "lock" };

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
	// copy of ?
	{ "help",           qtrue,  qtrue,  G_commands_cmd,        ":^7 Gives a list of commands"                                                               },
	{ "commands",       qtrue,  qtrue,  G_commands_cmd,        ":^7 Gives a list of commands"                                                               },

	{ "autorecord",     qtrue,  qtrue,  NULL,                  ":^7 Creates a demo with a consistent naming scheme"                                         },
	{ "autoscreenshot", qtrue,  qtrue,  NULL,                  ":^7 Creates a screenshot with a consistent naming scheme"                                   },
	{ "bottomshots",    qtrue,  qfalse, G_weaponRankings_cmd,  ":^7 Shows WORST player for each weapon. Add ^3<weapon_ID>^7 to show all stats for a weapon" },
	{ "callvote",       qtrue,  qfalse, (void (*)(gentity_t *, unsigned int, qboolean))Cmd_CallVote_f, " <params>:^7 Calls a vote"                          },
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
	{ "sclogin",        qtrue,  qfalse, G_sclogin_cmd,         " <password>:^7 Become a shoutcaster"                                                        },
	{ "sclogout",       qtrue,  qfalse, G_sclogout_cmd,        ":^7 Removes shoutcaster status"                                                             },
	{ "scores",         qtrue,  qtrue,  G_scores_cmd,          ":^7 Displays current match stat info"                                                       },
	{ "specinvite",     qtrue,  qtrue,  G_specinvite_cmd,      ":^7 Invites a player to spectate a speclock'ed team"                                        },
	{ "specuninvite",   qtrue,  qtrue,  G_specuninvite_cmd,    ":^7 Uninvites a spectator of a speclock'ed team"                                        },
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

/**
 * @brief G_commandCheck
 * @param[in] ent
 * @param[in] cmd
 * @param[in] fDoAnytime
 * @return
 */
qboolean G_commandCheck(gentity_t *ent, const char *cmd, qboolean fDoAnytime)
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

/**
 * @brief Prints specific command help info.
 * @param[in] ent
 * @param[in] pszCommand
 * @param[in] dwCommand
 * @return
 */
qboolean G_commandHelp(gentity_t *ent, const char *pszCommand, unsigned int dwCommand)
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

/**
 * @brief Debounces cmd request as necessary.
 * @param[in,out] ent
 * @param[in] pszCommand
 * @return
 */
qboolean G_cmdDebounce(gentity_t *ent, const char *pszCommand)
{
	if (ent->client->pers.cmd_debounce > level.time)
	{
		CP(va("print \"Wait another %.1fs to issue ^3%s\n\"", (double)(1.0f * (float)(ent->client->pers.cmd_debounce - level.time) / 1000.0f),
		      pszCommand));
		return qfalse;
	}

	ent->client->pers.cmd_debounce = level.time + CMD_DEBOUNCE;
	return qtrue ;
}

/**
 * @brief G_noTeamControls
 * @param ent - unused
 */
void G_noTeamControls(gentity_t *ent)
{
	CP("cpm \"Team commands not enabled on this server.\n\"");
}

////////////////////////////////////////////////////////////////////////////
/////
/////           Match Commands
/////
/////

/**
 * @brief Lists server commands.
 * @param ent - unused
 * @param dwCommand - unused
 * @param fValue - unused
 */
void G_commands_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	int i, rows, num_cmds = sizeof(aCommandInfo) / sizeof(aCommandInfo[0]);

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

/**
 * @brief Locks/unlocks a player's team.
 * @param[in] ent
 * @param[in] dwCommand
 * @param[in] fLock
 */
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

/**
 * @brief Pause/unpause a match.
 * @param[in] ent
 * @param[in] dwCommand
 * @param[in] fPause
 */
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

/**
 * @brief Show client info
 * @param[in] ent
 * @param dwCommand - unused
 * @param fDump - unused
 */
void G_players_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump)
{
	int       i, idnum, max_rate, cnt = 0;
	int       user_rate, user_snaps;
	gclient_t *cl;
	gentity_t *cl_ent;
	char      guid[MAX_GUID_LENGTH + 1], n2[MAX_NETNAME], ready[16], ref[8], rate[32], version[64];
	char      *s, *tc, *spec, *ign, *muted, *special, userinfo[MAX_INFO_STRING], *user_version;

	if (g_gamestate.integer == GS_PLAYING)
	{
		if (ent)
		{
			CP("print \"^7GUID       ID : Player                    Nudge  Rate  MaxPkts  Snaps  Specials\n\"");
			CP("print \"^7-------------------------------------------------------------------------------\n\"");
		}
		else
		{
			G_Printf("GUID       ID : Player                    Nudge  Rate  MaxPkts  Snaps  Specials\n");
			G_Printf("-------------------------------------------------------------------------------\n");
		}
	}
	else
	{
		if (ent)
		{
			CP("print \"^7GUID      Status   : ID : Player                    Nudge  Rate  MaxPkts  Snaps  Specials\n\"");
			CP("print \"^7-----------------------------------------------------------------------------------------\n\"");
		}
		else
		{
			G_Printf("GUID      Status   : ID : Player                    Nudge  Rate  MaxPkts  Snaps  Specials\n");
			G_Printf("-----------------------------------------------------------------------------------------\n");
		}
	}

	max_rate = trap_Cvar_VariableIntegerValue("sv_maxrate");

	for (i = 0; i < level.numConnectedClients; i++)
	{
		idnum  = level.sortedClients[i]; //level.sortedNames[i];
		cl     = &level.clients[idnum];
		cl_ent = g_entities + idnum;

		SanitizeString(cl->pers.cl_guid, guid, qfalse);
		SanitizeString(cl->pers.netname, n2, qfalse);
		n2[26]   = 0;
		ref[0]   = 0;
		ready[0] = 0;

		// GUID
		if (cl_ent->r.svFlags & SVF_BOT)
		{
			// omnibot requires 9 chars (OMNIBOT01)
			guid[9] = '\0';
		}
		else
		{
			// display only 8 char with * for humans
			guid[8] = '\0';
			strcat(guid, "*");
		}

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
			user_rate  = (max_rate > 0 && Q_atoi(s) > max_rate) ? max_rate : Q_atoi(s);
			s          = Info_ValueForKey(userinfo, "snaps");
			user_snaps = Q_atoi(s);

			Q_strncpyz(rate, va("%5d%6d%9d%7d", cl->pers.clientTimeNudge, user_rate, cl->pers.clientMaxPackets, user_snaps), sizeof(rate));
		}

		// Version info
		if (cl_ent->r.svFlags & SVF_BOT)
		{
			Q_strncpyz(version, va("%s", "--"), sizeof(version));
		}
		else
		{
			trap_GetUserinfo(idnum, userinfo, sizeof(userinfo));

			user_version = Info_ValueForKey(userinfo, "etVersion");

			// no engine version found, check cgame version as a fallback
			if (user_version[0] == 0)
			{
				user_version = Info_ValueForKey(userinfo, "cg_etVersion");
			}

			Q_strncpyz(version, user_version, sizeof(version));
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

		if (cl->sess.shoutcaster && !(cl_ent->r.svFlags & SVF_BOT))
		{
			spec = "SC ";
		}
		else if ((cl->sess.spec_invite & TEAM_AXIS) && (cl->sess.spec_invite & TEAM_ALLIES))
		{
			spec = "SB ";
		}
		else if (cl->sess.spec_invite & TEAM_AXIS)
		{
			spec = "SX ";
		}
		else if (cl->sess.spec_invite & TEAM_ALLIES)
		{
			spec = "SL ";
		}
		else
		{
			spec = "";
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

		if (cl->pers.connected == CON_CONNECTING)
		{
			special = va("%s", "                 ");
		}
		else
		{
			special = va("%s%s%s%s", ref, spec, ign, muted);
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
				tc = (ent) ? "^$L^7" : "L";
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
			CP(va("print \"%-9s %s%s%2d : %s%-26s^7%s  ^3%-8s^7  ^9%s^7\n\"", guid, ready, tc, idnum, ((ref[0]) ? "^3" : "^7"), n2, rate, special, version));
		}
		else
		{
			G_Printf("%-9s %s%s%2d : %-26s%s  %-8s  %s\n", guid, ready, tc, idnum, n2, rate, special, version);
		}

		cnt++;
	}

	if (ent)
	{
		CP(va("print \"\n^3%2d^7 total player%s\n\n\"", cnt, cnt > 1 ? "s" : ""));
	}
	else
	{
		G_Printf("\n%2d total player%s\n\n", cnt, cnt > 1 ? "s" : "");
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

/**
 * @brief Sets a player's "ready" status.
 * @param[in,out] ent
 * @param[in] dwCommand
 * @param[in] fDump
 */
void G_ready_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump)
{
	char *status[2] = { " NOT", "" };

	if (g_gamestate.integer == GS_PLAYING || g_gamestate.integer == GS_INTERMISSION)
	{
		CP("cpm \"Match is already in progress!\n\"");
		return;
	}

	if (!fDump && g_gamestate.integer == GS_WARMUP_COUNTDOWN)
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

	// Move them to correct ready fDump
	if (ent->client->pers.ready == fDump)
	{
		CP(va("print \"You are already%s ready!\n\"", status[fDump]));
	}
	else
	{
		ent->client->pers.ready = fDump;
		if (!level.intermissiontime)
		{
			if (fDump)
			{
				G_MakeReady(ent);
			}
			else
			{
				G_MakeUnready(ent);
			}

			AP(va("print \"%s^7 is%s ready!\n\"", ent->client->pers.netname, status[fDump]));
			AP(va("cp \"\n%s\n^3is%s ready!\n\"", ent->client->pers.netname, status[fDump]));
		}
	}

	G_readyMatchState();
}

/**
 * @brief Team chat w/no location info
 * @param[in] ent
 * @param dwCommand - unused
 * @param fValue - unused
 */
void G_say_teamnl_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	Cmd_Say_f(ent, SAY_TEAMNL, qfalse);
}

/**
 * @brief Request for shoutcaster status
 * @param[in] ent
 * @param dwCommand - unused
 * @param fValue - unused
 */
void G_sclogin_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	char cmd[MAX_TOKEN_CHARS], pwd[MAX_TOKEN_CHARS];

	if (!ent || !ent->client)
	{
		return;
	}

	trap_Argv(0, cmd, sizeof(cmd));

	if (!G_IsShoutcastStatusAvailable(ent))
	{
		CP("print \"Sorry, shoutcaster status disabled on this server.\n\"");
		return;
	}

	if (ent->client->sess.shoutcaster)
	{
		CP("print \"Sorry, you are already logged in as shoutcaster.\n\"");
		return;
	}

	if (trap_Argc() < 2)
	{
		CP(va("print \"Usage: %s [password]\n\"", cmd));
		return;
	}

	trap_Argv(1, pwd, sizeof(pwd));

	if (Q_stricmp(pwd, shoutcastPassword.string))
	{
		CP("print \"Invalid shoutcaster password!\n\"");
		return;
	}

	G_MakeShoutcaster(ent);
}

/**
 * @brief Removes shoutcaster status
 * @param[in] ent
 * @param dwCommand - unused
 * @param fValue - unused
 */

void G_sclogout_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	char cmd[MAX_TOKEN_CHARS];

	if (!ent || !ent->client)
	{
		return;
	}

	trap_Argv(0, cmd, sizeof(cmd));

	if (!G_IsShoutcastStatusAvailable(ent))
	{
		CP("print \"Sorry, shoutcaster status disabled on this server.\n\"");
		return;
	}

	if (!ent->client->sess.shoutcaster)
	{
		CP("print \"Sorry, you are not logged in as shoutcaster.\n\"");
		return;
	}

	G_RemoveShoutcaster(ent);
}

/**
 * @brief Set shoutcaster command.
 */
void G_makesc_cmd(void)
{
	char      cmd[MAX_TOKEN_CHARS], name[MAX_NAME_LENGTH];
	int       pcount, pids[MAX_CLIENTS];
	gentity_t *ent;

	trap_Argv(0, cmd, sizeof(cmd));

	if (trap_Argc() != 2)
	{
		G_Printf("Usage: %s <slot#|name>\n", cmd);
		return;
	}

	if (!G_IsShoutcastPasswordSet())
	{
		G_Printf("%s: Sorry, shoutcaster status disabled on this server.\n", cmd);
		return;
	}

	trap_Argv(1, name, sizeof(name));
	pcount = ClientNumbersFromString(name, pids);

	if (pcount > 1)
	{
		G_Printf("%s: More than one player matches. "
		         "Be more specific or use the slot number.\n", cmd);
		return;
	}
	else if (pcount < 1)
	{
		G_Printf("%s: No connected player found with that "
		         "name or slot number.\n", cmd);
		return;
	}

	ent = pids[0] + g_entities;

	if (!ent || !ent->client)
	{
		return;
	}

	// ignore bots
	if (ent->r.svFlags & SVF_BOT)
	{
		G_Printf("%s: Sorry, a bot can not be a shoutcaster.\n", cmd);
		return;
	}

	if (ent->client->sess.shoutcaster)
	{
		G_Printf("%s: Sorry, %s^7 is already a shoutcaster.\n", cmd, ent->client->pers.netname);
		return;
	}

	G_MakeShoutcaster(ent);
}

/**
 * @brief Remove shoutcaster command.
 */
void G_removesc_cmd(void)
{
	char      cmd[MAX_TOKEN_CHARS], name[MAX_NAME_LENGTH];
	int       pcount, pids[MAX_CLIENTS];
	gentity_t *ent;

	trap_Argv(0, cmd, sizeof(cmd));

	if (trap_Argc() != 2)
	{
		G_Printf("Usage: %s <slot#|name>\n", cmd);
		return;
	}

	if (!G_IsShoutcastPasswordSet())
	{
		G_Printf("%s: Sorry, shoutcaster status disabled on this server.\n", cmd);
		return;
	}

	trap_Argv(1, name, sizeof(name));
	pcount = ClientNumbersFromString(name, pids);

	if (pcount > 1)
	{
		G_Printf("%s: More than one player matches. Be more specific or use the slot number.\n", cmd);
		return;
	}
	else if (pcount < 1)
	{
		G_Printf("%s: No connected player found with that name or slot number.\n", cmd);
		return;
	}

	ent = pids[0] + g_entities;

	if (!ent || !ent->client)
	{
		return;
	}

	if (!ent->client->sess.shoutcaster)
	{
		G_Printf("%s: Sorry, %s^7 is not a shoutcaster.\n", cmd, ent->client->pers.netname);
		return;
	}

	G_RemoveShoutcaster(ent);
}

/**
 * @brief Shows match stats to the requesting client.
 * @param[in] ent
 * @param dwCommand - unused
 * @param fValue - unused
 */
void G_scores_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	G_printMatchInfo(ent);
}

/**
 * @brief Sends an invitation to a player to spectate a team.
 * @param[in] ent
 * @param[in] dwCommand
 * @param fLock - unused
 */
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

		// Find the player to invite
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

		// Can't invite an active player
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

/**
 * @brief Remove invitation of a player to spectate a team.
 * @param[in] ent
 * @param[in] dwCommand
 * @param fLock - unused
 */
void G_specuninvite_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fLock)
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

		// Find the player to invite
		trap_Argv(1, arg, sizeof(arg));
		if ((pid = ClientNumberFromString(ent, arg)) == -1)
		{
			return;
		}

		player = g_entities + pid;

		// Can't uninvite self
		if (player->client == ent->client)
		{
			CP("cpm \"You can't specuninvite yourself!\n\"");
			return;
		}

		// Can't uninvite an active player
		if (player->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			CP("cpm \"You can't specuninvite a non-spectator!\n\"");
			return;
		}

		// Can't uninvite referre
		if (player->client->sess.referee)
		{
			CP("cpm \"You can't specuninvite a referee!\n\"");
			return;
		}

		// Can't uninvite shoutcaster
		if (player->client->sess.shoutcaster)
		{
			CP("cpm \"You can't specuninvite a shoutcaster!\n\"");
			return;
		}

		if (player->client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			StopFollowing(player);
			player->client->sess.spec_team &= ~ent->client->sess.sessionTeam;
		}
		player->client->sess.spec_invite &= ~ent->client->sess.sessionTeam;

		// Notify sender/recipient
		CP(va("print \"%s^7 has been sent an uninvite spectator notification.\n\"", player->client->pers.netname));
		G_printFull(va("*** You've been uninvited to spectate the %s team!", aTeams[ent->client->sess.sessionTeam]), player);

	}
	else
	{
		// Referee can't specuninvite oneself
		if (ent->client->sess.referee)
		{
			CP("cpm \"Referee can't specuninvite oneself!\n\"");
			return;
		}

		// Shoutcaster can't specuninvite oneself
		if (ent->client->sess.shoutcaster)
		{
			CP("cpm \"Shoutcaster can't specuninvite oneself!\n\"");
			return;
		}

		// Spectators can uninvite themselves from current spectated team
		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			StopFollowing(ent);
			ent->client->sess.spec_team &= ~ent->client->sess.sessionTeam;
		}
		ent->client->sess.spec_invite &= ~ent->client->sess.sessionTeam;
		CP("cpm \"You have uninvited yourself!\n\"");
	}
}

/**
 * @brief Locks/unlocks a player's team from spectators.
 * @param[in] ent
 * @param[in] dwCommand
 * @param[in] fLock
 */
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

/**
 * @brief Shows a player's stats to the requesting client.
 * @param[in] ent
 * @param dwCommand - unused
 * @param fDump - unused
 */
void G_weaponStats_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump)
{
	G_statsPrint(ent, 0);
}

/**
 * @brief Shows all players' stats to the requesting client.
 * @param ent
 * @param dwCommand - unused
 * @param fDump - unused
 */
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

/**
 * @brief Sets a player's team "ready" status.
 * @param[in] ent
 * @param[in] dwCommand
 * @param fDump - unused
 */
void G_teamready_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump)
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

/**
  * @var These map to WS_* weapon indexes
  */
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
	30,     // 26 WS_MP34
	5,      // 27 WS_SYRINGE
};

/**
 * @brief Gives back overall or specific weapon rankings
 * @param[in] a
 * @param[in] b
 * @return
 */
int QDECL SortStats(const void *a, const void *b)
{
	gclient_t *ca, *cb;
	float     accA, accB;

	ca = &level.clients[*(const int *)a];
	cb = &level.clients[*(const int *)b];

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

/**
 * @brief Shows the most accurate players for each weapon to the requesting client
 * @param ent - unused
 * @param[in] doTop
 * @param[in] doWindow
 */
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
				Q_strcat(z, sizeof(z), va(" %d %d %d %d %d %d %d", iWeap + 1, aClients[i],
				                          cl->sess.aWeaponStats[iWeap].hits,
				                          cl->sess.aWeaponStats[iWeap].atts,
				                          cl->sess.aWeaponStats[iWeap].kills,
				                          cl->sess.aWeaponStats[iWeap].deaths,
				                          cl->sess.aWeaponStats[iWeap].headshots));
			}
		}
	}
	CP(va("%sbstats%s %s 0", ((doWindow) ? "w" : ""), ((doTop) ? "" : "b"), z));
}

/**
 * @brief Shows best/worst accuracy for all weapons, or sorted accuracies for a single weapon
 * @param[in] ent
 * @param[in] dwCommand
 * @param[in] state
 */
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
	if ((iWeap = Q_atoi(z)) == 0 || iWeap < WS_KNIFE || iWeap >= WS_MAX)
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

	Com_Memcpy(&level.sortedStats, &level.sortedClients, sizeof(level.sortedStats));
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
			Q_strcat(z, sizeof(z), va(" %d %d %d %d %d %d", level.sortedStats[i],
			                          cl->sess.aWeaponStats[iWeap].hits,
			                          shots,
			                          cl->sess.aWeaponStats[iWeap].kills,
			                          cl->sess.aWeaponStats[iWeap].deaths,
			                          cl->sess.aWeaponStats[iWeap].headshots));
		}
	}

	CP(va("astats%s %d %d %d%s", ((state) ? "" : "b"), c, iWeap, wBestAcc, z));
}
