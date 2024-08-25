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
 * @file g_cmds_ext.c
 * @brief Extended command set handling
 */

#include "g_local.h"
#include "../../etmain/ui/menudef.h"

int iWeap = WS_MAX;

const char *lock_status[2] = { "unlock", "lock" };

/**
 * @struct cmd_usage_flag_e
 * @typedef cmd_usage_flag_t
 */
typedef enum cmdUsageFlag_e
{
	CMD_USAGE_ANY_TIME          = BIT(0),
	CMD_USAGE_INTERMISSION_ONLY = BIT(1),
	CMD_USAGE_NO_INTERMISSION   = BIT(2)
} cmdUsageFlag_t;

/**
 * @struct cmd_reference_t
 * @brief
 *
 * @note Update info:
 * 1. Add line to aCommandInfo w/appropriate info
 * 2. Add implementation for specific command (see an existing command for an example)
 */
typedef struct
{
	char *pszCommandName;
	cmdUsageFlag_t flag;
	int value;
	qboolean floodProtected;
	void (*pCommand)(gentity_t *ent, unsigned int dwCommand, int value);
	const char *pszHelpInfo;
} cmd_reference_t;

// VC optimizes for dup strings :)
static const cmd_reference_t aCommandInfo[] =
{
	// keep "say" command on top for optimisation purpose, they are the most used command
	{ "say",            CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_say_cmd,                           " <msg>:^7 Sends a chat message"                                                             },
	{ "say_team",       CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_say_team_cmd,                      " <msg>:^7 Sends a team chat message"                                                        },
	{ "say_buddy",      CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_say_buddy_cmd,                     " <msg>:^7 Sends a buddy chat message"                                                       },
	{ "say_teamnl",     CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_say_teamnl_cmd,                    " <msg>:^7 Sends a team chat message without location info"                                  },
	{ "vsay",           CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_vsay_cmd,                          " <msg>:^7 Sends a voice chat message"                                                       },
	{ "vsay_team",      CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_vsay_team_cmd,                     " <msg>:^7 Sends a voice team chat message"                                                  },
	{ "vsay_buddy",     CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_vsay_buddy_cmd,                    " <msg>:^7 Sends a voice buddy chat message"                                                 },

	{ "?",              CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_commands_cmd,                      ":^7 Gives a list of commands"                                                               },
	// copy of ?
	{ "commands",       CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_commands_cmd,                      ":^7 Gives a list of commands"                                                               },
	{ "help",           CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_commands_cmd,                      ":^7 Gives a list of commands"                                                               },

	{ "+stats",         CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 HUD overlay showing current weapon stats info"                                          },
	{ "+topshots",      CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 HUD overlay showing current top accuracies of all players"                              },
	{ "+objectives",    CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 HUD overlay showing current objectives info"                                            },
	{ "autorecord",     CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 Creates a demo with a consistent naming scheme"                                         },
	{ "autoscreenshot", CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 Creates a screenshot with a consistent naming scheme"                                   },
	{ "bottomshots",    CMD_USAGE_ANY_TIME,          qfalse,      qfalse, G_weaponRankings_cmd,                ":^7 Shows WORST player for each weapon. Add ^3<weapon_ID>^7 to show all stats for a weapon" },
	{ "callvote",       CMD_USAGE_NO_INTERMISSION,   qfalse,      qtrue,  (void (*)(gentity_t *,               unsigned int, int)) Cmd_CallVote_f, " <params>:^7 Calls a vote"                              },
	{ "currenttime",    CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 Displays current local time"                                                            },
	{ "dropobj",        CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_DropObjective_f,                 ":^7 Drop carried objective"                                                                 },
	{ "fireteam",       CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_FireTeam_MP_f,                   " <create|disband|leave|apply|invite|warn|kick|propose|privacy|admin>:^7 Manage fireteam"    },
	{ "follow",         CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_Follow_f,                        " <player_ID|allies|axis>:^7 Spectates a particular player or team"                          },
	{ "follownext",     CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_FollowNext_f,                    ":^7 Follow next player in list"                                                             },
	{ "followprev",     CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_FollowPrevious_f,                ":^7 Follow previous player in list"                                                         },
	{ "forcetapout",    CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_ForceTapout_f,                   ":^7 Force player into limbo"                                                                },
	{ "give",           CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_Give_f,                          " <all|skill|medal|health|weapons|ammo|allammo|keys>:^7 Gives something"                     },
	{ "god",            CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_God_f,                           ":^7 God Mode"                                                                               },
	{ "ignore",         CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_Ignore_f,                        " <clientname>:^7 Ignore a player from chat"                                                 },
#ifdef FEATURE_PRESTIGE
	{ "imcollectpr",    CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, Cmd_IntermissionCollectPrestige_f,   ""                                                                                           },
#endif
	{ "immaplist",      CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, G_IntermissionMapList,               ""                                                                                           },
	{ "immaphistory",   CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, G_IntermissionMapHistory,            ""                                                                                           },
	{ "impkd",          CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, Cmd_IntermissionPlayerKillsDeaths_f, ""                                                                                           },
#ifdef FEATURE_PRESTIGE
	{ "impr",           CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, Cmd_IntermissionPrestige_f,          ""                                                                                           },
#endif
	{ "impt",           CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, Cmd_IntermissionPlayerTime_f,        ""                                                                                           },
	{ "imready",        CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, Cmd_IntermissionReady_f,             ""                                                                                           },
#ifdef FEATURE_RATING
	{ "imsr",           CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, Cmd_IntermissionSkillRating_f,       ""                                                                                           },
#endif
	{ "imvotetally",    CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, G_IntermissionVoteTally_cmd,         ""                                                                                           },
	{ "imwa",           CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, Cmd_IntermissionWeaponAccuracies_f,  ""                                                                                           },
	{ "imws",           CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, Cmd_IntermissionWeaponStats_f,       ""                                                                                           },
//  { "invite",         CMD_USAGE_ANY_TIME,  qtrue,        NULL,                                " <player_ID>:^7 Invites a player to join a team" },
	{ "kill",           CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_Kill_f,                          ":^7 Suicide"                                                                                },
	{ "lock",           CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_lock_cmd,                          ":^7 Locks a player's team to prevent others from joining"                                   },
	{ "mapvote",        CMD_USAGE_INTERMISSION_ONLY, qtrue,       qfalse, G_IntermissionMapVote,               ""                                                                                           },
#ifdef FEATURE_MULTIVIEW
	{ "mvadd",          CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, G_smvAdd_cmd,                        " <player_ID>:^7 Adds a player to multi-screen view"                                         },
	{ "mvallies",       CMD_USAGE_NO_INTERMISSION,   TEAM_ALLIES, qfalse, G_smvAddTeam_cmd,                    ":^7 Views entire allies/axis team"                                                          },
	{ "mvaxis",         CMD_USAGE_NO_INTERMISSION,   TEAM_AXIS,   qfalse, G_smvAddTeam_cmd,                    ":^7 Views entire allies/axis team"                                                          },
	{ "mvall",          CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, G_smvAddAllTeam_cmd,                 ":^7 Views all entire teams"                                                                 },
	{ "mvnone",         CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, G_smvDisable_cmd,                    ":^7 Disables multiview mode and goes back to spectator mode"                                },
	{ "mvdel",          CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, G_smvDel_cmd,                        " [player_ID]:^7 Removes current selected or specific player from multi-screen view"         },
#endif
	{ "noclip",         CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_Noclip_f,                        ":^7 No clip"                                                                                },
	{ "nofatigue",      CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_Nofatigue_f,                     ":^7 Infinite endurance"                                                                     },
	{ "nostamina",      CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_Nostamina_f,                     ":^7 Infinite stamina / charge power"                                                        },
	{ "notarget",       CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_Notarget_f,                      ":^7 ???"                                                                                    },
	{ "notready",       CMD_USAGE_ANY_TIME,          qfalse,      qtrue,  G_ready_cmd,                         ":^7 Sets your status to ^5not ready^7 to start a match"                                     },
	{ "obj",            CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_SelectedObjective_f,             " <val>:^7 Selected Objective"                                                               },
	{ "pause",          CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, G_pause_cmd,                         ":^7 Allows a team to pause a match"                                                         },
	{ "players",        CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_players_cmd,                       ":^7 Lists all active players and their IDs/information"                                     },
	{ "rconAuth",       CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_AuthRcon_f,                      ":^7 Client authentication"                                                                  },
	{ "ready",          CMD_USAGE_NO_INTERMISSION,   qtrue,       qtrue,  G_ready_cmd,                         ":^7 Sets your status to ^5ready^7 to start a match"                                         },
	{ "readyteam",      CMD_USAGE_NO_INTERMISSION,   qtrue,       qtrue,  G_teamready_cmd,                     ":^7 Sets an entire team's status to ^5ready^7 to start a match"                             },
	{ "ref",            CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_ref_cmd,                           " <password>:^7 Become a referee (admin access)"                                             },
//  { "remove",         CMD_USAGE_ANY_TIME,  qtrue,        NULL,                                " <player_ID>:^7 Removes a player from the team" },
	{ "rs",             CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_ResetSetup_f,                    ""                                                                                           },
	{ "sclogin",        CMD_USAGE_ANY_TIME,          qfalse,      qtrue,  G_sclogin_cmd,                       " <password>:^7 Become a shoutcaster"                                                        },
	{ "sclogout",       CMD_USAGE_ANY_TIME,          qfalse,      qtrue,  G_sclogout_cmd,                      ":^7 Removes shoutcaster status"                                                             },
	{ "score",          CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_Score_f,                         ":^7 Request current scoreboard information"                                                 },
	{ "scores",         CMD_USAGE_ANY_TIME,          qtrue,       qfalse, G_scores_cmd,                        ":^7 Displays current match stat info"                                                       },
	{ "setviewpos",     CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_SetViewpos_f,                    " x y z pitch yaw roll useViewHeight(0/1):^7 Set the current player position and view angle" },
	{ "setspawnpt",     CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_SetSpawnPoint_f,                 " [majorSpawn] [minorSpawn]:^7 Select a spawn point"                                         },
	{ "sgstats",        CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_sgStats_f,                       ""                                                                                           },

	{ "showstats",      CMD_USAGE_ANY_TIME,          qtrue,       qfalse, G_PrintAccuracyLog,                  ":^7 Shows weapon accuracy stats"                                                            },
	{ "specinvite",     CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_specinvite_cmd,                    ":^7 Invites a player to spectate a speclock'ed team"                                        },
	{ "specuninvite",   CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_specuninvite_cmd,                  ":^7 Uninvites a spectator of a speclock'ed team"                                            },
	{ "speclock",       CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_speclock_cmd,                      ":^7 Locks a player's team from spectators"                                                  },
//  { "speconly",       CMD_USAGE_ANY_TIME,  qtrue,        NULL,                                ":^7 Toggles option to stay as a spectator in 1v1" },
	{ "specunlock",     CMD_USAGE_ANY_TIME,          qfalse,      qtrue,  G_speclock_cmd,                      ":^7 Unlocks a player's team from spectators"                                                },
	{ "statsall",       CMD_USAGE_ANY_TIME,          qfalse,      qtrue,  G_statsall_cmd,                      ":^7 Shows weapon accuracy stats for all players"                                            },
	{ "statsdump",      CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 Shows player stats + match info saved locally to a file"                                },
	{ "stoprecord",     CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 Stops a demo recording currently in progress"                                           },
	{ "stshots",        CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_WeaponStatsLeaders_f,            ""                                                                                           },
	{ "team",           CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  Cmd_Team_f,                          " <b|r|s|none>:^7 Joins a team (b = allies, r = axis, s = spectator)"                        },
	{ "class",          CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_Class_f,                         " <s|m|e|f|c> <weapon1> <weapon2>: ^7Select your class and weapons"                          },
	{ "timein",         CMD_USAGE_NO_INTERMISSION,   qfalse,      qfalse, G_pause_cmd,                         ":^7 Unpauses a match (if initiated by the issuing team)"                                    },
	{ "timeout",        CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, G_pause_cmd,                         ":^7 Allows a team to pause a match"                                                         },
	{ "topshots",       CMD_USAGE_ANY_TIME,          qtrue,       qfalse, G_weaponRankings_cmd,                ":^7 Shows BEST player for each weapon. Add ^3<weapon_ID>^7 to show all stats for a weapon"  },
	{ "unignore",       CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_UnIgnore_f,                      " <clientname>:^7 Unignore a player from chat"                                               },
	{ "unlock",         CMD_USAGE_ANY_TIME,          qfalse,      qtrue,  G_lock_cmd,                          ":^7 Unlocks a player's team, allowing others to join"                                       },
	{ "unpause",        CMD_USAGE_NO_INTERMISSION,   qfalse,      qfalse, G_pause_cmd,                         ":^7 Unpauses a match (if initiated by the issuing team)"                                    },
	{ "unready",        CMD_USAGE_NO_INTERMISSION,   qfalse,      qtrue,  G_ready_cmd,                         ":^7 Sets your status to ^5not ready^7 to start a match"                                     },
	{ "vote",           CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_Vote_f,                          " <n|0|y|1>:^7 Cast the vote (n|0 = no, y|1 = yes)"                                          },
	{ "weaponstats",    CMD_USAGE_ANY_TIME,          qfalse,      qfalse, G_weaponStats_cmd,                   " [player_ID]:^7 Shows weapon accuracy stats for a player"                                   },
	{ "where",          CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_Where_f,                         ":^7 Show the current XYZ player position"                                                   },
	{ "ws",             CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_WeaponStat_f,                    ":^7 Shows weapon stats"                                                                     },
	{ "wstats",         CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_wStats_f,                        ""                                                                                           },
	{ NULL,             CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ""                                                                                           }
};

/**
 * @brief G_ClientIsFlooding
 * @param[in] ent
 * @return
 */
qboolean G_ClientIsFlooding(gentity_t *ent)
{
	gclient_t *client = ent->client;

	if (!client || !G_ServerIsFloodProtected())
	{
		return qfalse;
	}

	// even if the command might be blocked, increase the next time commands are reduced
	// this makes sure that commands won't start reducing if we keep flooding
	client->sess.nextCommandDecreaseTime = level.time + 1000;

	// not enough time has passed since last command
	if (level.time < client->sess.nextReliableTime)
	{
		return qtrue;
	}

	// check for +1 since we only increase the counter afterwards
	if (client->sess.numReliableCommands + 1 > g_floodLimit.integer)
	{
		client->sess.nextReliableTime = level.time + g_floodWait.integer;
		return qtrue;
	}

	client->sess.numReliableCommands++;
	// if we're within limits, ignore g_floodWait
	client->sess.nextReliableTime = level.time;

	return qfalse;
}

/**
 * @brief G_commandCheck
 * @param[in] ent
 * @param[in] cmd
 * @param[in] fDoAnytime
 * @return
 */
qboolean G_commandCheck(gentity_t *ent, const char *cmd)
{
	unsigned int i;

	for (i = 0; aCommandInfo[i].pszCommandName; i++)
	{
		if (aCommandInfo[i].pCommand && 0 == Q_stricmp(cmd, aCommandInfo[i].pszCommandName))
		{
			// check for flood protected commands
			if (aCommandInfo[i].floodProtected && G_ClientIsFlooding(ent))
			{
				CPx(ent->s.clientNum, va("print \"^1Flood protection: ^7command ^3%s ^7ignored.\n\"", cmd));
				return qfalse;
			}
			// ignore some commands when at intermission
			if (level.intermissiontime && (aCommandInfo[i].flag & CMD_USAGE_NO_INTERMISSION))
			{
				CPx(ent->s.clientNum, va("print \"^3%s^7 not allowed during intermission.\n\"", cmd));
				return qfalse;
			}

			// ignore some commands when not at intermission
			if (!level.intermissiontime && (aCommandInfo[i].flag & CMD_USAGE_INTERMISSION_ONLY))
			{
				CPx(ent->s.clientNum, va("print \"^3%s^7 not allowed outside intermission.\n\"", cmd));
				return qfalse;
			}

			aCommandInfo[i].pCommand(ent, i, aCommandInfo[i].value);

			return qtrue;
		}
	}

	trap_SendServerCommand(ent->s.clientNum, va("print \"[lon]unknown cmd[lof] %s\n\"", cmd));

	return qfalse;
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
	if (!ent)
	{
		return qfalse;
	}

	if (pszCommand && dwCommand < ARRAY_LEN(aCommandInfo))
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
 * @param ent
 * @param dwCommand
 * @param value - unused
 */
void G_commands_cmd(gentity_t *ent, unsigned int dwCommand, int value)
{
	unsigned int i, rows, num_cmds;

	if (trap_Argc() > 1)
	{
		char arg[MAX_TOKEN_CHARS];
		trap_Argv(1, arg, sizeof(arg));

		for (i = 0; aCommandInfo[i].pszCommandName; i++)
		{
			if (aCommandInfo[i].pCommand && !Q_stricmp(arg, aCommandInfo[i].pszCommandName))
			{
				G_commandHelp(ent, arg, i);
				return;
			}
		}
	}

	num_cmds = ARRAY_LEN(aCommandInfo) - 1;

	rows = num_cmds / HELP_COLUMNS;
	if (num_cmds % HELP_COLUMNS)
	{
		rows++;
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

	CP(va("print \"\nType: ^3\\%s command_name^7 for more information\n\"", aCommandInfo[dwCommand].pszCommandName));
}

/**
 * @brief Locks/unlocks a player's team.
 * @param[in] ent
 * @param[in] dwCommand
 * @param[in] fLock
 */
void G_lock_cmd(gentity_t *ent, unsigned int dwCommand, int fLock)
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
void G_pause_cmd(gentity_t *ent, unsigned int dwCommand, int fPause)
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
				AP(va("print \"^3Match is ^1PAUSED^3!\n^7[by %s ^7for %s^7: - %d Timeouts Remaining]\n\"", ent->client->pers.netname, aTeams[tteam], teamInfo[tteam].timeouts));
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
void G_players_cmd(gentity_t *ent, unsigned int dwCommand, int fDump)
{
	int       i, idnum, max_rate, cnt = 0;
	int       user_rate, user_snaps;
	gclient_t *cl;
	gentity_t *cl_ent;
	char      guid[MAX_GUID_LENGTH + 1], n2[MAX_NETNAME], rate[32], version[64];
	char      *s, *tc, *ready, *ref, *spec, *ign, *muted, *special, userinfo[MAX_INFO_STRING], *user_version;

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

		Q_strncpyz(guid, cl->pers.cl_guid, sizeof(guid));
		Q_CleanStr(guid);

		Q_strncpyz(n2, cl->pers.netname, sizeof(n2));
		Q_CleanStr(n2);

		n2[26] = 0;

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
			Q_strcat(guid, sizeof(guid), "*");
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
				ready = ent ? "^5--------^7 :" : "-------- :";
			}
			else if (cl->pers.ready || (g_entities[idnum].r.svFlags & SVF_BOT))
			{
				ready = ent ? "^3(READY)^7  :" : "(READY)  :";
			}
			else
			{
				ready = ent ? "^7NOTREADY^7 :" : "NOTREADY :";
			}
		}
		else
		{
			ready = "";
		}

		if (cl->sess.referee && !(cl_ent->r.svFlags & SVF_BOT))
		{
			ref = "REF ";
		}
		else
		{
			ref = "";
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
void G_ready_cmd(gentity_t *ent, unsigned int dwCommand, int fDump)
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
 * @brief G_say_f
 * @param[in] ent
 * @param[in] dwCommand - unused
 * @param[in] value - unused
 */
void G_say_cmd(gentity_t *ent, unsigned int dwCommand, int value)
{
	G_Say_f(ent, SAY_ALL);
}

/**
 * @brief G_say_team_cmd
 * @param[in] ent
 * @param[in] dwCommand - unused
 * @param[in] value - unused
 */
void G_say_team_cmd(gentity_t *ent, unsigned int dwCommand, int value)
{
	G_Say_f(ent, SAY_TEAM);
}

/**
 * @brief G_say_buddy_cmd
 * @param[in] ent
 * @param[in] dwCommand - unused
 * @param[in] value - unused
 */
void G_say_buddy_cmd(gentity_t *ent, unsigned int dwCommand, int value)
{
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_FREE)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Can't buddy chat as spectator\n\"");
		return;
	}

	G_Say_f(ent, SAY_BUDDY);
}

/**
 * @brief Team chat w/no location info
 * @param[in] ent
 * @param dwCommand - unused
 * @param value - unused
 */
void G_say_teamnl_cmd(gentity_t *ent, unsigned int dwCommand, int value)
{
	G_Say_f(ent, SAY_TEAMNL);
}

/**
 * @brief G_vsay_f
 * @param[in] ent
 * @param[in] dwCommand - unused
 * @param[in] value - unused
 */
void G_vsay_cmd(gentity_t *ent, unsigned int dwCommand, int value)
{
	G_Voice_f(ent, SAY_ALL, qfalse, qfalse);
}

/**
 * @brief G_vsay_team_f
 * @param[in] ent
 * @param[in] dwCommand - unused
 * @param[in] value - unused
 */
void G_vsay_team_cmd(gentity_t *ent, unsigned int dwCommand, int value)
{
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_FREE)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Can't team chat as spectator\n\"");
		return;
	}

	G_Voice_f(ent, SAY_TEAM, qfalse, qfalse);
}

/**
 * @brief G_vsay_buddy_f
 * @param[in] ent
 * @param[in] dwCommand - unused
 * @param[in] value - unused
 */
void G_vsay_buddy_cmd(gentity_t *ent, unsigned int dwCommand, int value)
{
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_FREE)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Can't buddy chat as spectator\n\"");
		return;
	}

	G_Voice_f(ent, SAY_BUDDY, qfalse, qfalse);
}

/**
 * @brief Request for shoutcaster status
 * @param[in] ent
 * @param dwCommand - unused
 * @param value - unused
 */
void G_sclogin_cmd(gentity_t *ent, unsigned int dwCommand, int value)
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
 * @param value - unused
 */

void G_sclogout_cmd(gentity_t *ent, unsigned int dwCommand, int value)
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
	int       cnum;
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

	cnum = G_ClientNumberFromString(NULL, name);

	if (cnum == -1)
	{
		return;
	}

	ent = &g_entities[cnum];

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
	int       cnum;
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

	cnum = G_ClientNumberFromString(NULL, name);

	if (cnum == -1)
	{
		return;
	}

	ent = &g_entities[cnum];

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
 * @param value - unused
 */
void G_scores_cmd(gentity_t *ent, unsigned int dwCommand, int value)
{
	G_printMatchInfo(ent);
}

/**
 * @brief Sends an invitation to a player to spectate a team.
 * @param[in] ent
 * @param[in] dwCommand
 * @param fLock - unused
 */
void G_specinvite_cmd(gentity_t *ent, unsigned int dwCommand, int fLock)
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
		if ((pid = G_ClientNumberFromString(ent, arg)) == -1)
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
void G_specuninvite_cmd(gentity_t *ent, unsigned int dwCommand, int fLock)
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
		if ((pid = G_ClientNumberFromString(ent, arg)) == -1)
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
void G_speclock_cmd(gentity_t *ent, unsigned int dwCommand, int fLock)
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
void G_weaponStats_cmd(gentity_t *ent, unsigned int dwCommand, int fDump)
{
	G_statsPrint(ent, 0);
}

/**
 * @brief Shows all players' stats to the requesting client.
 * @param ent
 * @param dwCommand - unused
 * @param fDump - unused
 */
void G_statsall_cmd(gentity_t *ent, unsigned int dwCommand, int fDump)
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
void G_teamready_cmd(gentity_t *ent, unsigned int dwCommand, int fDump)
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

			G_MakeReady(&g_entities[level.sortedClients[i]]);
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
void G_weaponRankings_cmd(gentity_t *ent, unsigned int dwCommand, int state)
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

/**
 * @brief G_IntermissionVoteTally_cmd
 * @param[in] ent
 * @param dwCommand - unused
 * @param state - unused
 */
void G_IntermissionVoteTally_cmd(gentity_t *ent, unsigned int dwCommand, int state)
{
	G_IntermissionVoteTally(ent);
}
