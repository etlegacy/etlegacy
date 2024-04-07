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
 * @file tvg_cmds_ext.c
 * @brief Extended command set handling
 */

#include "tvg_local.h"
#include "../../etmain/ui/menudef.h"

int iWeap = WS_MAX;

const char *lock_status[2] = { "unlock", "lock" };

#define NOCD        0
#define VERYSHORTCD 2500
#define SHORTCD     5000
#define MEDIUMCD    15000
#define LONGCD      30000
#define VERYLONGCD  60000

// VC optimizes for dup strings :)
static tvcmd_reference_t tvCommandInfo[] =
{
	// keep "say" command on top for optimisation purpose, they are the most used command
	{ "say",          CMD_USAGE_ANY_TIME,                        0,          NOCD,       0, qtrue,  0,          TVG_say_cmd,                             " <msg>:^7 Sends a chat message"                                                             },
	{ "say_team",     CMD_USAGE_ANY_TIME,                        0,          NOCD,       0, qtrue,  0,          TVG_say_cmd,                             " <msg>:^7 Sends a chat message"                                                             },
	{ "say_buddy",    CMD_USAGE_ANY_TIME,                        0,          NOCD,       0, qtrue,  0,          TVG_say_cmd,                             " <msg>:^7 Sends a buddy chat message"                                                       },
	{ "say_teamnl",   CMD_USAGE_ANY_TIME,                        0,          NOCD,       0, qtrue,  0,          TVG_say_cmd,                             " <msg>:^7 Sends a team chat message without location info"                                  },

	{ "tvchat",       CMD_USAGE_ANY_TIME,                        0,          NOCD,       0, qtrue,  0,          TVG_tvchat_cmd,                          ":^7 Turns tvchat on/off"                                                                    },
	//{ "ignore",         CMD_USAGE_ANY_TIME,          qtrue,       qfalse, TVCmd_Ignore_f,                      " <clientname>:^7 Ignore a player from chat"                                                 },
	//{ "unignore",       CMD_USAGE_ANY_TIME,          qtrue,       qfalse, TVCmd_UnIgnore_f,                    " <clientname>:^7 Unignore a player from chat"                                               },

	//{ "?",              CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_commands_cmd,                      ":^7 Gives a list of commands"                                                               },
	// copy of ?
	//{ "commands",       CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_commands_cmd,                      ":^7 Gives a list of commands"                                                               },
	//{ "help",           CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_commands_cmd,                      ":^7 Gives a list of commands"                                                               },

	{ "bottomshots",  CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, 0,          MEDIUMCD,   0, qfalse, ETJUMP_MOD, TVG_weaponRankings_cmd,                  ":^7 Shows WORST player for each weapon. Add ^3<weapon_ID>^7 to show all stats for a weapon" },
	{ "callvote",     CMD_USAGE_NO_INTERMISSION,                 0,          NOCD,       0, qtrue,  0,          TVG_Cmd_CallVote_f,                      " <params>:^7 Calls a vote"                                                                  },

	{ "follow",       CMD_USAGE_NO_INTERMISSION,                 0,          NOCD,       0, qfalse, 0,          TVG_Cmd_Follow_f,                        " <player_ID|allies|axis>:^7 Spectates a particular player or team"                          },
	{ "follownext",   CMD_USAGE_NO_INTERMISSION,                 0,          NOCD,       0, qfalse, 0,          TVG_Cmd_FollowNext_f,                    ":^7 Follow next player in list"                                                             },
	{ "followprev",   CMD_USAGE_NO_INTERMISSION,                 0,          NOCD,       0, qfalse, 0,          TVG_Cmd_FollowPrevious_f,                ":^7 Follow previous player in list"                                                         },

	{ "imvotetally",  CMD_USAGE_INTERMISSION_ONLY,               0,          NOCD,       0, qfalse, 0,          TVG_IntermissionVoteTally,               ""                                                                                           },
	{ "immaplist",    CMD_USAGE_INTERMISSION_ONLY,               0,          NOCD,       0, qfalse, 0,          TVG_IntermissionMapList,                 ""                                                                                           },
	{ "immaphistory", CMD_USAGE_INTERMISSION_ONLY,               0,          NOCD,       0, qfalse, 0,          TVG_IntermissionMapHistory,              ""                                                                                           },

	{ "impkd",        CMD_USAGE_INTERMISSION_ONLY,               0,          NOCD,       0, qfalse, 0,          TVG_Cmd_IntermissionPlayerKillsDeaths_f, ""                                                                                           },
	{ "impr",         CMD_USAGE_INTERMISSION_ONLY,               0,          NOCD,       0, qfalse, 0,          TVG_Cmd_IntermissionPrestige_f,          ""                                                                                           },
	{ "impt",         CMD_USAGE_INTERMISSION_ONLY,               0,          NOCD,       0, qfalse, 0,          TVG_Cmd_IntermissionPlayerTime_f,        ""                                                                                           },
	{ "imsr",         CMD_USAGE_INTERMISSION_ONLY,               0,          NOCD,       0, qfalse, 0,          TVG_Cmd_IntermissionSkillRating_f,       ""                                                                                           },
	{ "imwa",         CMD_USAGE_INTERMISSION_ONLY,               0,          NOCD,       0, qfalse, 0,          TVG_Cmd_IntermissionWeaponAccuracies_f,  ""                                                                                           },
	{ "imws",         CMD_USAGE_INTERMISSION_ONLY,               0,          NOCD,       0, qfalse, 0,          TVG_Cmd_IntermissionWeaponStats_f,       ""                                                                                           },

	{ "players",      CMD_USAGE_ANY_TIME,                        0,          SHORTCD,    0, qtrue,  0,          TVG_players_cmd,                         ":^7 Lists all active players and their IDs"                                                 },
	{ "viewers",      CMD_USAGE_ANY_TIME,                        0,          SHORTCD,    0, qtrue,  0,          TVG_viewers_cmd,                         ":^7 Lists all viewers and their IDs"                                                        },
	{ "rconAuth",     CMD_USAGE_ANY_TIME,                        0,          SHORTCD,    0, qfalse, 0,          TVG_Cmd_AuthRcon_f,                      ":^7 Client authentication"                                                                  },

	{ "ref",          CMD_USAGE_ANY_TIME,                        0,          SHORTCD,    0, qtrue,  0,          TVG_ref_cmd,                             " <password>:^7 Become a referee (admin access)"                                             },

	{ "score",        CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, qtrue,      SHORTCD,    0, qfalse, 0,          TVG_Cmd_Score_f,                         ":^7 Request current scoreboard information"                                                 },
	{ "scores",       CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, qtrue,      MEDIUMCD,   0, qfalse, ETJUMP_MOD, TVG_scores_cmd,                          ":^7 Displays current match stat info"                                                       },

	{ "sgstats",      CMD_USAGE_ANY_TIME,                        SHORTCD,    NOCD,       0, qtrue,  0,          TVG_Cmd_sgStats_f,                       ""                                                                                           },
	{ "wstats",       CMD_USAGE_ANY_TIME,                        SHORTCD,    NOCD,       0, qtrue,  0,          TVG_Cmd_wStats_f,                        ""                                                                                           },
	{ "weaponstats",  CMD_USAGE_ANY_TIME,                        SHORTCD,    NOCD,       0, qtrue,  0,          TVG_weaponStats_cmd,                     " [player_ID]:^7 Shows weapon accuracy stats for a player"                                   },

	{ "statsall",     CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, VERYLONGCD, NOCD,       0, qtrue,  ETJUMP_MOD, TVG_statsall_cmd,                        ":^7 Shows weapon accuracy stats for all players"                                            },
	{ "stshots",      CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, MEDIUMCD,   VERYLONGCD, 0, qfalse, ETJUMP_MOD, TVG_Cmd_WeaponStatsLeaders_f,            ""                                                                                           },
	{ "topshots",     CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, qtrue,      MEDIUMCD,   0, qfalse, ETJUMP_MOD, TVG_weaponRankings_cmd,                  ":^7 Shows BEST player for each weapon. Add ^3<weapon_ID>^7 to show all stats for a weapon"  },
	{ "ws",           CMD_USAGE_ANY_TIME,                        qtrue,      MEDIUMCD,   0, qtrue,  0,          TVG_Cmd_WeaponStat_f,                    ":^7 Shows weapon stats"                                                                     },

	{ "setviewpos",   CMD_USAGE_NO_INTERMISSION,                 qtrue,      MEDIUMCD,   0, qfalse, 0,          TVG_Cmd_SetViewpos_f,                    " x y z pitch yaw roll useViewHeight(0/1):^7 Set the current player position and view angle" },
	{ "noclip",       CMD_USAGE_NO_INTERMISSION,                 qtrue,      NOCD,       0, qfalse, 0,          TVG_Cmd_Noclip_f,                        ":^7 No clip"                                                                                },

	{ "obj",          CMD_USAGE_NO_INTERMISSION,                 qtrue,      NOCD,       0, qfalse, 0,          TVG_Cmd_SelectedObjective_f,             " <val>:^7 Selected Objective"                                                               },
	{ NULL,           CMD_USAGE_ANY_TIME,                        qtrue,      NOCD,       0, qfalse, 0,          NULL,                                    ""                                                                                           }
};

/**
 * @brief TVG_ClientIsFlooding
 * @param[in] client
 * @return
 */
qboolean TVG_ClientIsFlooding(gclient_t *client)
{
	if (!client || !TVG_ServerIsFloodProtected())
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
	if (client->sess.numReliableCommands + 1 > tvg_floodLimit.integer)
	{
		client->sess.nextReliableTime = level.time + tvg_floodWait.integer;
		return qtrue;
	}

	client->sess.numReliableCommands++;
	// if we're within limits, ignore g_floodWait
	client->sess.nextReliableTime = level.time;

	return qfalse;
}

/**
 * @brief TVG_commandCheck
 * @param[in] client
 * @param[in] cmd
 * @return
 */
qboolean TVG_commandCheck(gclient_t *client, const char *cmd)
{
	unsigned int i;

	for (i = 0; tvCommandInfo[i].pszCommandName; i++)
	{
		if (tvCommandInfo[i].pCommand && !Q_stricmp(cmd, tvCommandInfo[i].pszCommandName))
		{
			// check for flood protected commands
			if (tvCommandInfo[i].floodProtected && TVG_ClientIsFlooding(client))
			{
				CPx(client - level.clients, va("print \"^1Flood protection: ^7command ^3%s ^7ignored.\n\"", cmd));
				return qfalse;
			}
			// ignore some commands when at intermission
			if (level.intermission && (tvCommandInfo[i].flag & CMD_USAGE_NO_INTERMISSION))
			{
				CPx(client - level.clients, va("print \"^3%s^7 not allowed during intermission.\n\"", cmd));
				return qfalse;
			}

			// ignore some commands when not at intermission
			if (!level.intermission && (tvCommandInfo[i].flag & CMD_USAGE_INTERMISSION_ONLY))
			{
				CPx(client - level.clients, va("print \"^3%s^7 not allowed outside intermission.\n\"", cmd));
				return qfalse;
			}

			tvCommandInfo[i].pCommand(client, &tvCommandInfo[i]);

			return qtrue;
		}
	}

	trap_SendServerCommand(client - level.clients, va("print \"TVGAME: [lon]unknown cmd[lof] %s\n\"", cmd));

	return qfalse;
}

/**
* @brief TVG_SendCommands Sends clients commands to master server
*        preferably one at a time so to not spam too much
* @return
*/
void TVG_SendCommands(void)
{
	unsigned int i;

	for (i = 0; tvCommandInfo[i].pszCommandName; i++)
	{
		// request intermission stats
		if ((tvCommandInfo[i].flag & CMD_USAGE_INTERMISSION_ONLY) && level.intermission)
		{
			if (tvCommandInfo[i].pCommand(NULL, &tvCommandInfo[i]))
			{
				return;
			}
		}

		// auto update some information
		if ((tvCommandInfo[i].flag & CMD_USAGE_AUTOUPDATE) && !(level.mod & tvCommandInfo[i].mods))
		{
			if (tvCommandInfo[i].pCommand(NULL, &tvCommandInfo[i]))
			{
				return;
			}
		}
	}
}

/**
 * @brief Prints specific command help info.
 * @param[in] client
 * @param[in] pszCommand
 * @param[in] dwCommand
 * @return
 */
qboolean TVG_commandHelp(gclient_t *client, const char *pszCommand, unsigned int dwCommand)
{
	if (!client)
	{
		return qfalse;
	}

	if (pszCommand && dwCommand < ARRAY_LEN(tvCommandInfo))
	{
		CP(va("print \"\n^3%s%s\n\n\"", pszCommand, tvCommandInfo[dwCommand].pszHelpInfo));
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Debounces cmd request as necessary.
 * @param[in,out] client
 * @param[in] pszCommand
 * @return
 */
qboolean TVG_cmdDebounce(gclient_t *client, const char *pszCommand)
{
	if (client->pers.cmd_debounce > level.time)
	{
		CP(va("print \"Wait another %.1fs to issue ^3%s\n\"", (double)(1.0f * (float)(client->pers.cmd_debounce - level.time) / 1000.0f),
		      pszCommand));
		return qfalse;
	}

	client->pers.cmd_debounce = level.time + CMD_DEBOUNCE;
	return qtrue ;
}

////////////////////////////////////////////////////////////////////////////
/////
/////           Match Commands
/////
/////

/**
 * @brief Lists server commands.
 * @param client
 * @param dwCommand
 * @param value - unused
 */
void TVG_commands_cmd(gclient_t *client, unsigned int dwCommand, int value)
{
	unsigned int i, rows, num_cmds;

	if (trap_Argc() > 1)
	{
		char arg[MAX_TOKEN_CHARS];
		trap_Argv(1, arg, sizeof(arg));

		for (i = 0; tvCommandInfo[i].pszCommandName; i++)
		{
			if (tvCommandInfo[i].pCommand && !Q_stricmp(arg, tvCommandInfo[i].pszCommandName))
			{
				TVG_commandHelp(client, arg, i);
				return;
			}
		}
	}

	num_cmds = ARRAY_LEN(tvCommandInfo) - 1;

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
			CP(va("print \"^3%-17s%-17s%-17s%-17s\n\"", tvCommandInfo[i].pszCommandName,
			      tvCommandInfo[i + rows].pszCommandName,
			      tvCommandInfo[i + rows * 2].pszCommandName,
			      tvCommandInfo[i + rows * 3].pszCommandName));
		}
		else if (i + rows * 2 + 1 <= num_cmds)
		{
			CP(va("print \"^3%-17s%-17s%-17s\n\"", tvCommandInfo[i].pszCommandName,
			      tvCommandInfo[i + rows].pszCommandName,
			      tvCommandInfo[i + rows * 2].pszCommandName));
		}
		else if (i + rows + 1 <= num_cmds)
		{
			CP(va("print \"^3%-17s%-17s\n\"", tvCommandInfo[i].pszCommandName, tvCommandInfo[i + rows].pszCommandName));
		}
		else
		{
			CP(va("print \"^3%-17s\n\"", tvCommandInfo[i].pszCommandName));
		}
	}

	CP(va("print \"\nType: ^3\\%s command_name^7 for more information\n\"", tvCommandInfo[dwCommand].pszCommandName));
}

/**
 * @brief Show client info
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_players_cmd(gclient_t *client, tvcmd_reference_t *self)
{
	char cs[MAX_STRING_CHARS];
	char name[MAX_STRING_CHARS];
	int  i, idnum, count = 0;

	if (client)
	{
		CP("print \"^sID : Player                    \n\"");
		CP("print \"^1-------------------------------\n\"");
	}
	else
	{
		G_Printf("ID : Player                    \n");
		G_Printf("-------------------------------\n");
	}

	for (i = 0; i < level.numValidMasterClients; i++)
	{
		idnum = level.validMasterClients[i];

		trap_GetConfigstring(CS_PLAYERS + idnum, cs, sizeof(cs));

		Q_strncpyz(name, Info_ValueForKey(cs, "n"), sizeof(name));
		Q_CleanStr(name);
		name[26] = 0;
		count++;

		if (client)
		{
			CP(va("print \"%2d : %-26s\n\"", idnum, name));
		}
		else
		{
			G_Printf("%2d : %-26s\n", idnum, name);
		}
	}

	if (client)
	{
		CP(va("print \"\n^3%2d^7 total player%s\n\n\"", count, count > 1 ? "s" : ""));
	}
	else
	{
		G_Printf("\n%2d total player%s\n\n", count, count > 1 ? "s" : "");
	}

	return qtrue;
}

/**
* @brief Show viewers info
* @param[in] client
* @param[in] self
*/
qboolean TVG_viewers_cmd(gclient_t *client, tvcmd_reference_t *self)
{
	gclient_t *cl;
	char      name[MAX_STRING_CHARS];
	int       i, idnum, count = 0;

	if (client)
	{
		CP("print \"^sID : Spectator                    \n\"");
		CP("print \"^1----------------------------------\n\"");
	}
	else
	{
		G_Printf("ID : Spectator                    \n");
		G_Printf("----------------------------------\n");
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		idnum = level.sortedClients[i];
		cl    = &level.clients[idnum];

		Q_strncpyz(name, cl->pers.netname, sizeof(name));
		Q_CleanStr(name);
		name[26] = 0;
		count++;

		if (client)
		{
			CP(va("print \"%2d : %-26s\n\"", idnum, name));
		}
		else
		{
			G_Printf("%2d : %-26s\n", idnum, name);
		}
	}

	if (client)
	{
		CP(va("print \"\n^3%2d^7 total viewer%s\n\n\"", count, count > 1 ? "s" : ""));
	}
	else
	{
		G_Printf("\n%2d total viewer%s\n\n", count, count > 1 ? "s" : "");
	}

	return qtrue;
}

/**
 * @brief G_say_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_say_cmd(gclient_t *client, tvcmd_reference_t *self)
{
	TVG_Say_f(client, SAY_ALL);

	return qtrue;
}

/**
* @brief TVG_tvchat_cmd
* @param[in] client
* @param[in] self
*
* @note argv(0) tvchat
*/
qboolean TVG_tvchat_cmd(gclient_t *client, tvcmd_reference_t *self)
{
	char *msg;
	char *name;

	name = ConcatArgs(1);

	if (!Q_stricmp(name, "on") || Q_atoi(name))
	{
		client->sess.tvchat = qtrue;
	}
	else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0"))
	{
		client->sess.tvchat = qfalse;
	}
	else
	{
		client->sess.tvchat = !client->sess.tvchat;
	}

	if (client->sess.tvchat)
	{
		msg = "tvchat ON\n";
	}
	else
	{
		msg = "tvchat OFF\n";
	}

	trap_SendServerCommand(client - level.clients, va("print \"%s\"", msg));

	return qtrue;
}

/**
 * @brief Sends match stats to the requesting client.
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_scores_cmd(gclient_t *client, tvcmd_reference_t *self)
{
	return qfalse;

	//int i;

	//if (!client)
	//{
	//	if (TVG_CommandsAutoUpdate(self))
	//	{
	//		return qtrue;
	//	}
	//
	//	return qfalse;
	//}

	//for (i = 0; i < level.cmds.scoresEndIndex; i++)
	//{
	//	trap_SendServerCommand(client - level.clients, level.cmds.scores[i]);
	//}

	//return qtrue;
}

/**
 * @brief Shows a player's stats to the requesting client.
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_weaponStats_cmd(gclient_t *client, tvcmd_reference_t *self)
{
	TVG_statsPrint(client, 0, self->value);

	return qtrue;
}

/**
 * @brief Shows all players' stats to the requesting client.
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_statsall_cmd(gclient_t *client, tvcmd_reference_t *self)
{
	int i;

	if (!client)
	{
		// only request during intermission
		if (!level.intermission)
		{
			return qfalse;
		}

		if (self->lastUpdateTime)
		{
			return qfalse;
		}

		trap_SendServerCommand(-2, self->pszCommandName);
		self->lastUpdateTime = level.time;

		return qtrue;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (level.cmds.infoStats[INFO_WS].valid[i])
		{
			trap_SendServerCommand(client - level.clients, level.cmds.infoStats[INFO_WS].data[i]);
		}
	}

	return qtrue;
}

/**
 * @brief Shows the most accurate players for each weapon to the requesting client
 * @param[in] client
 * @param[in] doTop
 * @param[in] doWindow
 */
qboolean TVG_weaponStatsLeaders_cmd(gclient_t *client, qboolean doTop, qboolean doWindow)
{
	if (doWindow && doTop)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.wbstats);
	}
	else
	{
		if (doTop)
		{
			trap_SendServerCommand(client - level.clients, level.cmds.bstats);
		}
		else
		{
			trap_SendServerCommand(client - level.clients, level.cmds.bstatsb);
		}
	}

	return qtrue;
}

/**
 * @brief Shows best/worst accuracy for all weapons, or sorted accuracies for a single weapon
 * @param[in] client
 * @param[in] dwCommand
 * @param[in] state
 */
qboolean TVG_weaponRankings_cmd(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_CommandsAutoUpdate(self);
	}

	if (trap_Argc() < 2)
	{
		TVG_weaponStatsLeaders_cmd(client, self->value, qfalse);
		return qtrue;
	}

	if (self->value)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.astats);
	}
	else
	{
		trap_SendServerCommand(client - level.clients, level.cmds.astatsb);
	}

	return qtrue;
}
