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
 * @file tvg_cmds_ext.c
 * @brief Extended command set handling
 */

#include "tvg_local.h"
#include "../../etmain/ui/menudef.h"

int iWeap = WS_MAX;

const char *lock_status[2] = { "unlock", "lock" };

/**
 * @struct cmd_usage_flag_e
 * @typedef cmd_usage_flag_t
 */
typedef enum tvcmdUsageFlag_e
{
	CMD_USAGE_ANY_TIME          = BIT(0),
	CMD_USAGE_INTERMISSION_ONLY = BIT(1),
	CMD_USAGE_NO_INTERMISSION   = BIT(2),
	CMD_USAGE_AUTOUPDATE        = BIT(3)
} tvcmdUsageFlag_t;

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
	tvcmdUsageFlag_t flag;
	int value;
	int cooldown;
	int lastTime;
	qboolean floodProtected;
	void (*pCommand)(gclient_t *client, unsigned int dwCommand, int value);
	const char *pszHelpInfo;
} tvcmd_reference_t;

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
	{ "say",              CMD_USAGE_ANY_TIME,           0, NOCD, 0,      qtrue,  TVG_say_cmd,                    " <msg>:^7 Sends a chat message"                                                             },
	{ "say_team",         CMD_USAGE_ANY_TIME,           0, NOCD, 0,      qtrue,  TVG_say_cmd,                    " <msg>:^7 Sends a chat message"                                                        },
	{ "say_buddy",        CMD_USAGE_ANY_TIME,           0, NOCD, 0,      qtrue,  TVG_say_cmd,                    " <msg>:^7 Sends a buddy chat message"                                                       },
	{ "say_teamnl",       CMD_USAGE_ANY_TIME,           0, NOCD, 0,      qtrue,  TVG_say_cmd,                    " <msg>:^7 Sends a team chat message without location info"                                  },

	{ "tvchat",           CMD_USAGE_ANY_TIME,           0, NOCD, 0,      qtrue,  TVG_tvchat_cmd,                 ":^7 Turns tvchat on/off" },

	//{ "ignore",         CMD_USAGE_ANY_TIME,          qtrue,       qfalse, TVCmd_Ignore_f,                      " <clientname>:^7 Ignore a player from chat"                                                 },
	//{ "unignore",       CMD_USAGE_ANY_TIME,          qtrue,       qfalse, TVCmd_UnIgnore_f,                    " <clientname>:^7 Unignore a player from chat"                                               },

	//{ "?",              CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_commands_cmd,                      ":^7 Gives a list of commands"                                                               },
	// copy of ?
	//{ "commands",       CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_commands_cmd,                      ":^7 Gives a list of commands"                                                               },
	//{ "help",           CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_commands_cmd,                      ":^7 Gives a list of commands"                                                               },

	//{ "+stats",         CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 HUD overlay showing current weapon stats info"                                          },
	//{ "+topshots",      CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 HUD overlay showing current top accuracies of all players"                              },
	//{ "+objectives",    CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 HUD overlay showing current objectives info"                                            },

	{ "bottomshots",    CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE,          qfalse, MEDIUMCD, 0,     qfalse, TVG_weaponRankings_cmd,                ":^7 Shows WORST player for each weapon. Add ^3<weapon_ID>^7 to show all stats for a weapon" },
	//{ "callvote",       CMD_USAGE_NO_INTERMISSION,   qfalse,      qtrue,  (void (*)(gentity_t *,               unsigned int, int)) Cmd_CallVote_f, " <params>:^7 Calls a vote"                              },
	//{ "currenttime",    CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 Displays current local time"                                                            },

	{ "follow",         CMD_USAGE_NO_INTERMISSION,   0, NOCD, 0,       qfalse, TVG_Cmd_Follow_f,                        " <player_ID|allies|axis>:^7 Spectates a particular player or team"                          },
	{ "follownext",     CMD_USAGE_NO_INTERMISSION,   0, NOCD, 0,       qfalse, TVG_Cmd_FollowNext_f,                    ":^7 Follow next player in list"                                                             },
	{ "followprev",     CMD_USAGE_NO_INTERMISSION,    0, NOCD, 0,       qfalse, TVG_Cmd_FollowPrevious_f,                ":^7 Follow previous player in list"                                                         },

	{ "imvotetally",      CMD_USAGE_INTERMISSION_ONLY, qtrue, NOCD, 0,       qfalse, TVG_IntermissionVoteTally,         ""                                                                                            },
	{ "immaplist",        CMD_USAGE_INTERMISSION_ONLY, qtrue, NOCD, 0,       qfalse, TVG_IntermissionMapList,               ""                                                                                           },
	{ "immaphistory",     CMD_USAGE_INTERMISSION_ONLY, qtrue, NOCD, 0,       qfalse, TVG_IntermissionMapHistory,            ""                                                                                           },

	{ "impkd",            CMD_USAGE_INTERMISSION_ONLY, qtrue, NOCD, 0,       qfalse, TVCmd_IntermissionPlayerKillsDeaths_f, ""                                                                                           },
	{ "impr",             CMD_USAGE_INTERMISSION_ONLY, qtrue, NOCD, 0,       qfalse, TVCmd_IntermissionPrestige_f,          ""                                                                                           },
	{ "impt",             CMD_USAGE_INTERMISSION_ONLY, qtrue, NOCD, 0,       qfalse, TVCmd_IntermissionPlayerTime_f,        ""                                                                                           },
	{ "imsr",             CMD_USAGE_INTERMISSION_ONLY, qtrue, NOCD, 0,       qfalse, TVCmd_IntermissionSkillRating_f,       ""                                                                                           },
	{ "imwa",             CMD_USAGE_INTERMISSION_ONLY, qtrue, NOCD, 0,       qfalse, TVCmd_IntermissionWeaponAccuracies_f,  ""                                                                                           },
	//{ "imws",           CMD_USAGE_INTERMISSION_ONLY, NOCD, 0,       qfalse, TVCmd_IntermissionWeaponStats_f,       ""                                                                                           },

	//{ "noclip",         CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_Noclip_f,                        ":^7 No clip"                                                                                },
	
	//{ "obj",            CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_SelectedObjective_f,             " <val>:^7 Selected Objective"                                                               },
	
	{ "players",        CMD_USAGE_ANY_TIME,          0, SHORTCD, 0,      qtrue,  TVG_players_cmd,                       ":^7 Lists all active players and their IDs"                                     },
	{ "viewers",        CMD_USAGE_ANY_TIME,          0, SHORTCD, 0,      qtrue,  TVG_viewers_cmd,                       ":^7 Lists all viewers and their IDs" },
	//{ "rconAuth",       CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_AuthRcon_f,                      ":^7 Client authentication"                                                                  },
	
	//{ "ref",            CMD_USAGE_ANY_TIME,          qtrue,       qtrue,  G_ref_cmd,                           " <password>:^7 Become a referee (admin access)"                                             },

	{ "score",            CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, qtrue, SHORTCD,  0,      qfalse, TVG_Cmd_Score_f,                       ":^7 Request current scoreboard information"                                                 },
	{ "scores",           CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, qtrue, MEDIUMCD, 0,      qfalse, TVG_scores_cmd,                        ":^7 Displays current match stat info"                                                       },
	
	{ "sgstats",        CMD_USAGE_ANY_TIME,          VERYSHORTCD, NOCD, 0,      qtrue, TVCmd_sgStats_f,                       ""                                                                                           },
	{ "wstats",         CMD_USAGE_ANY_TIME,          VERYSHORTCD, NOCD, 0,      qtrue, TVCmd_wStats_f,                        "" },
	{ "weaponstats",    CMD_USAGE_ANY_TIME,          VERYSHORTCD, NOCD, 0,      qtrue, TVG_weaponStats_cmd,                   " [player_ID]:^7 Shows weapon accuracy stats for a player" },

	//{ "showstats",      CMD_USAGE_ANY_TIME,          qtrue,       qfalse, G_PrintAccuracyLog,                  ":^7 Shows weapon accuracy stats"                                                            },
	//{ "statsall",       CMD_USAGE_ANY_TIME,          qfalse,      qtrue,  G_statsall_cmd,                      ":^7 Shows weapon accuracy stats for all players"                                            },
	//{ "statsdump",      CMD_USAGE_ANY_TIME,          qtrue,       qfalse, NULL,                                ":^7 Shows player stats + match info saved locally to a file"                                },
	{ "stshots",        CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, MEDIUMCD,       VERYLONGCD, 0,       qfalse, TVCmd_WeaponStatsLeaders_f,            ""                                                                                           },
	{ "topshots",       CMD_USAGE_ANY_TIME | CMD_USAGE_AUTOUPDATE, qtrue,          MEDIUMCD, 0,       qfalse, TVG_weaponRankings_cmd,                ":^7 Shows BEST player for each weapon. Add ^3<weapon_ID>^7 to show all stats for a weapon" },

	//{ "ws",             CMD_USAGE_ANY_TIME,          qtrue,       qfalse, Cmd_WeaponStat_f,                    ":^7 Shows weapon stats"                                                                     },
	//{ "setviewpos",     CMD_USAGE_NO_INTERMISSION,   qtrue,       qfalse, Cmd_SetViewpos_f,                    " x y z pitch yaw roll useViewHeight(0/1):^7 Set the current player position and view angle" },
	{ NULL,             CMD_USAGE_ANY_TIME,          qtrue, NOCD, 0,      qfalse, NULL,                                ""                                                                                           }
};

/**
 * @brief TVG_ClientIsFlooding
 * @param[in] client
 * @return
 */
qboolean TVG_ClientIsFlooding(gclient_t *client)
{
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
 * @brief TVG_commandCheck
 * @param[in] client
 * @param[in] cmd
 * @param[in] fDoAnytime
 * @return
 */
qboolean TVG_commandCheck(gclient_t *client, const char *cmd)
{
	unsigned int i;

	for (i = 0; tvCommandInfo[i].pszCommandName; i++)
	{
		if (tvCommandInfo[i].pCommand && 0 == Q_stricmp(cmd, tvCommandInfo[i].pszCommandName))
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

			tvCommandInfo[i].pCommand(client, i, tvCommandInfo[i].value);

			return qtrue;
		}
	}

	trap_SendServerCommand(client - level.clients, va("print \"TVGAME: [lon]unknown cmd[lof] %s\n\"", cmd));

	return qfalse;
}

/**
* @brief TVG_SendCommands Sends clients commands to master server
*        one at a time so to not spam too much
* @return
*/
void TVG_SendCommands(void)
{
	unsigned int i;

	for (i = 0; tvCommandInfo[i].pszCommandName; i++)
	{
		// request intermission stats
		if (tvCommandInfo[i].flag & CMD_USAGE_INTERMISSION_ONLY)
		{
			if (!tvCommandInfo[i].lastTime && level.intermission)
			{
				trap_SendServerCommand(-2, tvCommandInfo[i].pszCommandName);
				tvCommandInfo[i].lastTime = level.time;
				return;
			}
		}

		// auto update some information
		if (tvCommandInfo[i].flag & CMD_USAGE_AUTOUPDATE)
		{
			if (tvCommandInfo[i].lastTime + tvCommandInfo[i].cooldown <= level.time)
			{
				trap_SendServerCommand(-2, tvCommandInfo[i].pszCommandName);
				tvCommandInfo[i].lastTime = level.time;
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
 * @param dwCommand - unused
 * @param value - unused
 */
void TVG_players_cmd(gclient_t *client, unsigned int dwCommand, int value)
{
	char cs[MAX_TOKEN_CHARS];
	char name[MAX_TOKEN_CHARS];
	int i, idnum, count = 0;
	
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
}

/**
* @brief Show viewers info
* @param[in] client
* @param dwCommand - unused
* @param value - unused
*/
void TVG_viewers_cmd(gclient_t *client, unsigned int dwCommand, int value)
{
	gclient_t *cl;
	char cs[MAX_TOKEN_CHARS];
	char name[MAX_TOKEN_CHARS];
	int i, idnum, count = 0;

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
}

/**
 * @brief G_say_f
 * @param[in] client
 * @param[in] dwCommand - unused
 * @param[in] value - unused
 */
void TVG_say_cmd(gclient_t *client, unsigned int dwCommand, int value)
{
	TVG_Say_f(client, SAY_ALL);
}

/**
* @brief TVG_tvchat_cmd
* @param[in,out] client
* @param dwCommand - unused
* @param value    - unused
*
* @note argv(0) tvchat
*/
void TVG_tvchat_cmd(gclient_t *client, unsigned int dwCommand, int value)
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
}

/**
 * @brief Sends match stats to the requesting client.
 * @param[in] client
 * @param dwCommand - unused
 * @param value - unused
 */
void TVG_scores_cmd(gclient_t *client, unsigned int dwCommand, int value)
{
	int i;
	for (i = 0; i < level.cmds.scoresEndIndex; i++)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.scores[i]);
	}
}

/**
 * @brief Shows a player's stats to the requesting client.
 * @param[in] client
 * @param dwCommand - unused
 * @param fDump - unused
 */
void TVG_weaponStats_cmd(gclient_t *client, unsigned int dwCommand, int value)
{
	TVG_statsPrint(client, 0, value);
}

/**
 * @brief Shows all players' stats to the requesting client.
 * @param ent
 * @param dwCommand - unused
 * @param fDump - unused
 */
void G_statsall_cmd(gentity_t *ent, unsigned int dwCommand, int fDump)
{
	//int       i;
	//gentity_t *player;

	//for (i = 0; i < level.numConnectedClients; i++)
	//{
	//	player = &g_entities[level.sortedClients[i]];
	//	if (player->client->sess.sessionTeam == TEAM_SPECTATOR)
	//	{
	//		continue;
	//	}
	//	CP(va("ws %s\n", G_createStats(player)));
	//}
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
 * @param[in] client
 * @param[in] doTop
 * @param[in] doWindow
 */
void TVG_weaponStatsLeaders_cmd(gclient_t *client, qboolean doTop, qboolean doWindow)
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
}

/**
 * @brief Shows best/worst accuracy for all weapons, or sorted accuracies for a single weapon
 * @param[in] client
 * @param[in] dwCommand
 * @param[in] state
 */
void TVG_weaponRankings_cmd(gclient_t *client, unsigned int dwCommand, int state)
{
	if (trap_Argc() < 2)
	{
		TVG_weaponStatsLeaders_cmd(client, state, qfalse);
		return;
	}

	if (state)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.astats);
	}
	else
	{
		trap_SendServerCommand(client - level.clients, level.cmds.astatsb);
	}
}
