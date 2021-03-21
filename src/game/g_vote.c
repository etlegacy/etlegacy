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
 * @file g_vote.c
 * @brief All callvote handling
 */

#include "g_local.h"
#include "../../etmain/ui/menudef.h" // For vote options

#define T_FFA   0x01
#define T_1V1   0x02
#define T_SP    0x04
#define T_TDM   0x08
#define T_CTF   0x10
#define T_WLF   0x20
#define T_WSW   0x40
#define T_WCP   0x80
#define T_WCH   0x100

static const char *ACTIVATED   = "ACTIVATED";
static const char *DEACTIVATED = "DEACTIVATED";
static const char *ENABLED     = "ENABLED";
static const char *DISABLED    = "DISABLED";

// Update info:
//  1. Add line to aVoteInfo w/appropriate info
//  2. Add implementation for attempt and success (see an existing command for an example)
typedef struct
{
	unsigned int dwGameTypes;
	const char *pszVoteName;
	int (*pVoteCommand)(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
	const char *pszVoteMessage;
	const char *pszVoteHelp;
} vote_reference_t;

/**
 * @var aVoteInfo
 * @brief VC optimizes for dup strings :)
 */
static const vote_reference_t aVoteInfo[] =
{
	{ 0x1ff, "gametype",               G_Gametype_v,               "Set Gametype to",            " <value>^7\n  Changes the current gametype"                          },
	{ 0x1ff, "kick",                   G_Kick_v,                   "KICK",                       " <player_id>^7\n  Attempts to kick player from server"               },
	{ 0x1ff, "mute",                   G_Mute_v,                   "MUTE",                       " <player_id>^7\n  Removes the chat capabilities of a player"         },
	{ 0x1ff, "unmute",                 G_UnMute_v,                 "UN-MUTE",                    " <player_id>^7\n  Restores the chat capabilities of a player"        },
	{ 0x1ff, "map",                    G_Map_v,                    "Change map to",              " <mapname>^7\n  Votes for a new map to be loaded"                    },
	{ 0x1ff, "campaign",               G_Campaign_v,               "Change campaign to",         " <campaign>^7\n  Votes for a new map to be loaded"                   },
	{ 0x1ff, "maprestart",             G_MapRestart_v,             "Map Restart",                " ^7\n  Restarts the current map in progress"                         },
	{ 0x1ff, "matchreset",             G_MatchReset_v,             "Match Reset",                " ^7\n  Resets the entire match"                                      },
	{ 0x1ff, "mutespecs",              G_Mutespecs_v,              "Mute Spectators",            " <0|1>^7\n  Mutes in-game spectator chat"                            },
	{ 0x1ff, "nextmap",                G_Nextmap_v,                "Load Next Map",              " ^7\n  Loads the next map or campaign in the map queue"              },
	{ 0x1ff, "referee",                G_Referee_v,                "Referee",                    " <player_id>^7\n  Elects a player to have admin abilities"           },
	{ 0x1ff, "shuffleteams",           G_ShuffleTeams_v,           "Shuffle Teams",              " ^7\n  Randomly place players on each team"                          },
	{ 0x1ff, "shuffleteams_norestart", G_ShuffleTeams_NoRestart_v, "Shuffle Teams (No Restart)", " ^7\n  Randomly place players on each team"                          },
	{ 0x1ff, "startmatch",             G_StartMatch_v,             "Start Match",                " ^7\n  Sets all players to \"ready\" status to start the match"      },
	{ 0x1ff, "swapteams",              G_SwapTeams_v,              "Swap Teams",                 " ^7\n  Switch the players on each team"                              },
	{ 0x1ff, "friendlyfire",           G_FriendlyFire_v,           "Friendly Fire",              " <0|1>^7\n  Toggles ability to hurt teammates"                       },
	{ 0x1ff, "timelimit",              G_Timelimit_v,              "Timelimit",                  " <value>^7\n  Changes the current timelimit"                         },
	{ 0x1ff, "unreferee",              G_Unreferee_v,              "UNReferee",                  " <player_id>^7\n  Elects a player to have admin abilities removed"   },
	{ 0x1ff, "warmupdamage",           G_Warmupfire_v,             "Warmup Damage",              " <0|1|2>^7\n  Specifies if players can inflict damage during warmup" },
	{ 0x1ff, "antilag",                G_AntiLag_v,                "Anti-Lag",                   " <0|1>^7\n  Toggles Anit-Lag on the server"                          },
	{ 0x1ff, "balancedteams",          G_BalancedTeams_v,          "Balanced Teams",             " <0|1>^7\n  Toggles team balance forcing"                            },
	{ 0x1ff, "surrender",              G_Surrender_v,              "Surrender",                  " ^7\n  Ends the match"                                               },
	{ 0x1ff, "restartcampaign",        G_RestartCampaign_v,        "Restart Campaign",           " ^7\n  Restarts the current Campaign"                                },
	{ 0x1ff, "nextcampaign",           G_NextCampaign_v,           "Next Campaign",              " ^7\n  Ends the current campaign and starts the next one"            },
	{ 0x1ff, "poll",                   G_Poll_v,                   "[poll]",                     " <text>^7\n  Poll majority opinion"                                  },
	{ 0x1ff, "config",                 G_Config_v,                 "Game config",                " <configname>^7\n  Loads up the server game config"                  },
	{ 0x1ff, "cointoss",               G_CoinToss_v,               "Coin toss",                  " ^7\n  Tosses a coin and displays result to all players"             },
	{ 0,     0,                        NULL,                       0,                            0                                                                     },
};

/**
 * @brief Checks for valid custom callvote requests from the client.
 * @param[in] ent
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_voteCmdCheck(gentity_t *ent, char *arg, char *arg2, qboolean fRefereeCmd)
{
	unsigned int i, cVoteCommands = sizeof(aVoteInfo) / sizeof(aVoteInfo[0]);

	for (i = 0; i < cVoteCommands; i++)
	{
		if (!Q_stricmp(arg, aVoteInfo[i].pszVoteName))
		{
			int hResult = aVoteInfo[i].pVoteCommand(ent, i, arg, arg2, fRefereeCmd);

			if (hResult == G_OK)
			{
				Com_sprintf(arg, VOTE_MAXSTRING, "%s", aVoteInfo[i].pszVoteMessage);
				level.voteInfo.vote_fn = aVoteInfo[i].pVoteCommand;
			}
			else
			{
				level.voteInfo.vote_fn = NULL;
			}

			return hResult;
		}
	}

	return G_NOTFOUND;
}

/**
 * @brief Voting help summary.
 * @param[in] ent
 * @param[in] fShowVote
 */
void G_voteHelp(gentity_t *ent, qboolean fShowVote)
{
	int i, rows = 0, num_cmds = sizeof(aVoteInfo) / sizeof(aVoteInfo[0]) - 1;     // Remove terminator;
	int vi[100];            // Just make it large static.

	if (fShowVote)
	{
		CP("print \"\nValid ^3callvote^7 commands are:\n^3----------------------------\n\"");
	}

	for (i = 0; i < num_cmds; i++)
	{
		if (aVoteInfo[i].dwGameTypes & (1 << g_gametype.integer))
		{
			vi[rows++] = i;
		}
	}

	num_cmds = rows;
	rows     = num_cmds / HELP_COLUMNS;

	if (num_cmds % HELP_COLUMNS)
	{
		rows++;
	}

	for (i = 0; i < rows; i++)
	{
		if (i + rows * 3 + 1 <= num_cmds)
		{
			G_refPrintf(ent, "^5%-25s%-25s%-25s%-25s", aVoteInfo[vi[i]].pszVoteName,
			            aVoteInfo[vi[i + rows]].pszVoteName,
			            aVoteInfo[vi[i + rows * 2]].pszVoteName,
			            aVoteInfo[vi[i + rows * 3]].pszVoteName);
		}
		else if (i + rows * 2 + 1 <= num_cmds)
		{
			G_refPrintf(ent, "^5%-25s%-25s%-25s", aVoteInfo[vi[i]].pszVoteName,
			            aVoteInfo[vi[i + rows]].pszVoteName,
			            aVoteInfo[vi[i + rows * 2]].pszVoteName);
		}
		else if (i + rows + 1 <= num_cmds)
		{
			G_refPrintf(ent, "^5%-25s%-25s", aVoteInfo[vi[i]].pszVoteName,
			            aVoteInfo[vi[i + rows]].pszVoteName);
		}
		else
		{
			G_refPrintf(ent, "^5%-25s", aVoteInfo[vi[i]].pszVoteName);
		}
	}

	if (fShowVote)
	{
		CP("print \"\nUsage: ^3\\callvote <command> <params>\n^7For current settings/help, use: ^3\\callvote <command> ?\n\"");
	}

	return;
}

/**
 * @brief Set disabled votes for client UI
 */
void G_voteFlags(void)
{
	int i, flags = 0;

	for (i = 0; i < numVotesAvailable; i++)
	{
		if (trap_Cvar_VariableIntegerValue(voteToggles[i].pszCvar) == 0)
		{
			flags |= voteToggles[i].flag;
		}
	}

	if (flags != voteFlags.integer)
	{
		trap_Cvar_Set("voteFlags", va("%d", flags));
	}
}

/**
 * @brief Prints specific callvote command help description.
 * @param[in] ent
 * @param[in] fRefereeCmd
 * @param[in] cmd
 * @return
 */
qboolean G_voteDescription(gentity_t *ent, qboolean fRefereeCmd, unsigned int cmd)
{
	char arg[MAX_TOKEN_CHARS];
	char *ref_cmd = (fRefereeCmd) ? "\\ref" : "\\callvote";

	if (!ent)
	{
		return qfalse;
	}

	trap_Argv(2, arg, sizeof(arg));
	if (!Q_stricmp(arg, "?") || trap_Argc() == 2)
	{
		trap_Argv(1, arg, sizeof(arg));
		G_refPrintf(ent, "\nUsage: ^3%s %s%s\n", ref_cmd, arg, aVoteInfo[cmd].pszVoteHelp);
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Localize disable message info.
 * @param[in] ent
 * @param[in] cmd
 */
void G_voteDisableMessage(gentity_t *ent, const char *cmd)
{
	G_refPrintf(ent, "Sorry, [lof]^3%s^7 [lon]voting has been disabled", cmd);
}

/**
 * @brief Player ID message stub.
 * @param[in] ent
 */
void G_playersMessage(gentity_t *ent)
{
	G_refPrintf(ent, "Use the ^3players^7 command to find a valid player ID.");
}

/**
 * @brief Localize current parameter setting.
 * @param[in] ent
 * @param[in] cmd
 * @param[in] setting
 */
void G_voteCurrentSetting(gentity_t *ent, const char *cmd, const char *setting)
{
	G_refPrintf(ent, "^2%s^7 is currently ^3%s\n", cmd, setting);
}

/**
 * @brief Vote toggling
 * @param[in] ent
 * @param arg - unused
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @param[in] curr_setting
 * @param[in] vote_allow
 * @param[in] vote_type
 * @return
 */
int G_voteProcessOnOff(gentity_t *ent, char *arg, char *arg2, qboolean fRefereeCmd, int curr_setting, int vote_allow, unsigned int vote_type)
{
	if (!vote_allow && ent && !ent->client->sess.referee)
	{
		G_voteDisableMessage(ent, aVoteInfo[vote_type].pszVoteName);
		G_voteCurrentSetting(ent, aVoteInfo[vote_type].pszVoteName, ((curr_setting) ? ENABLED : DISABLED));
		return G_INVALID;
	}
	if (G_voteDescription(ent, fRefereeCmd, vote_type))
	{
		G_voteCurrentSetting(ent, aVoteInfo[vote_type].pszVoteName, ((curr_setting) ? ENABLED : DISABLED));
		return G_INVALID;
	}

	if ((atoi(arg2) && curr_setting) || (!atoi(arg2) && !curr_setting))
	{
		G_refPrintf(ent, "^3%s^5 is already %s!", aVoteInfo[vote_type].pszVoteName, ((curr_setting) ? ENABLED : DISABLED));
		return G_INVALID;
	}

	Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);
	Com_sprintf(arg2, VOTE_MAXSTRING, "%s", (atoi(arg2)) ? ACTIVATED : DEACTIVATED);

	return G_OK;
}

/**
 * Several commands to help with cvar setting
 */

/**
 * @brief G_voteSetOnOff
 * @param[in] desc
 * @param[in] cvar
 */
void G_voteSetOnOff(const char *desc, const char *cvar)
{
	AP(va("cpm \"^3%s is: ^5%s\n\"", desc, (atoi(level.voteInfo.vote_value)) ? ENABLED : DISABLED));
	//trap_SendConsoleCommand(EXEC_APPEND, va("%s %s\n", cvar, level.voteInfo.vote_value));
	trap_Cvar_Set(cvar, level.voteInfo.vote_value);
}

/**
 * @brief G_voteSetValue
 * @param[in] desc
 * @param[in] cvar
 */
void G_voteSetValue(const char *desc, const char *cvar)
{
	AP(va("cpm \"^3%s set to: ^5%s\n\"", desc, level.voteInfo.vote_value));
	//trap_SendConsoleCommand(EXEC_APPEND, va("%s %s\n", cvar, level.voteInfo.vote_value));
	trap_Cvar_Set(cvar, level.voteInfo.vote_value);
}

/**
 * @brief G_voteSetVoteString
 * @param[in] desc
 */
void G_voteSetVoteString(const char *desc)
{
	AP(va("print \"^3%s set to: ^5%s\n\"", desc, level.voteInfo.vote_value));
	trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteInfo.voteString));
}

////////////////////////////////////////////////////////
// Actual vote command implementation
////////////////////////////////////////////////////////

// *** Load competition settings for current mode ***

/**
 * @brief G_GametypeList
 * @param[in] ent
 */
void G_GametypeList(gentity_t *ent)
{
	int i;

	G_refPrintf(ent, "\nAvailable gametypes:\n--------------------");

	for (i = GT_WOLF; i < GT_MAX_GAME_TYPE; i++)
	{
		if (i != GT_WOLF_CAMPAIGN)
		{
			G_refPrintf(ent, "  %d ^3(%s)", i, gameNames[i]);
		}
	}

	G_refPrintf(ent, "\n");
}

/**
 * @brief GameType
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in,out] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Gametype_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		int i = Q_atoi(arg2);

		if (!vote_allow_gametype.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			G_GametypeList(ent);
			G_voteCurrentSetting(ent, arg, va("%d (%s)", g_gametype.integer, gameNames[g_gametype.integer]));
			return G_INVALID;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			G_GametypeList(ent);
			G_voteCurrentSetting(ent, arg, va("%d (%s)", g_gametype.integer, gameNames[g_gametype.integer]));
			return G_INVALID;
		}

		if (i < GT_WOLF || i >= GT_MAX_GAME_TYPE || i == GT_WOLF_CAMPAIGN)
		{
			G_refPrintf(ent, "\n^3Invalid gametype: ^7%d", i);
			G_GametypeList(ent);
			return G_INVALID;
		}

		if (i == g_gametype.integer)
		{
			G_refPrintf(ent, "\n^3Gametype^5 is already set to %s!", gameNames[i]);
			return G_INVALID;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", gameNames[i]);

		// Vote action (vote has passed)
	}
	else
	{
		char s[MAX_STRING_CHARS];

		// Set gametype
		G_voteSetValue("Gametype", "g_gametype");

		trap_Cvar_VariableStringBuffer("mapname", s, sizeof(s));
		trap_SendConsoleCommand(EXEC_APPEND, va("map %s\n", s));
	}

	return G_OK;
}

/**
 * @brief Player Kick
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in,out] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Kick_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		int pid;

		if (!vote_allow_kick.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return G_INVALID;
		}
		else if ((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee)
		{
			G_refPrintf(ent, "Can't vote to kick referees!");
			return G_INVALID;
		}

		if (level.clients[pid].sess.shoutcaster)
		{
			G_refPrintf(ent, "Can't vote to kick shoutcasters!");
			return G_INVALID;
		}

		if (g_entities[pid].r.svFlags & SVF_BOT)
		{
			G_refPrintf(ent, "Can't vote to kick bots!");
			return G_INVALID;
		}

		if (!fRefereeCmd && ent)
		{
			if (level.clients[pid].sess.sessionTeam != TEAM_SPECTATOR && level.clients[pid].sess.sessionTeam != ent->client->sess.sessionTeam)
			{
				G_refPrintf(ent, "Can't vote to kick players on opposing team!");
				return G_INVALID;
			}
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		// Kick a player
		trap_SendConsoleCommand(EXEC_APPEND, va("clientkick %d\n", Q_atoi(level.voteInfo.vote_value)));
		AP(va("cp \"%s\n^3has been kicked!\n\"", level.clients[atoi(level.voteInfo.vote_value)].pers.netname));
	}

	return G_OK;
}

/**
 * @brief Player Mute
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in,out] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Mute_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	if (fRefereeCmd)
	{
		// handled elsewhere
		return G_NOTFOUND;
	}

	// Vote request (vote is being initiated)
	if (arg)
	{
		int pid;

		if (!vote_allow_muting.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return G_INVALID;
		}
		else if ((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee)
		{
			G_refPrintf(ent, "Can't vote to mute referees!");
			return G_INVALID;
		}

		if (g_entities[pid].r.svFlags & SVF_BOT)
		{
			G_refPrintf(ent, "Can't vote to mute bots!");
			return G_INVALID;
		}

		if (level.clients[pid].sess.muted)
		{
			G_refPrintf(ent, "Player is already muted!");
			return G_INVALID;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		int pid = Q_atoi(level.voteInfo.vote_value);

		// Mute a player
		if (level.clients[pid].sess.referee != RL_RCON)
		{
			trap_SendServerCommand(pid, va("cpm \"^3You have been muted\""));
			level.clients[pid].sess.muted = qtrue;
			AP(va("cp \"%s\n^3has been muted!\n\"", level.clients[pid].pers.netname));
			ClientUserinfoChanged(pid);
		}
		else
		{
			G_Printf("Cannot mute a referee.\n");
		}
	}

	return G_OK;
}

/**
 * @brief Player Un-Mute
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in,out] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_UnMute_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	if (fRefereeCmd)
	{
		// handled elsewhere
		return G_NOTFOUND;
	}

	// Vote request (vote is being initiated)
	if (arg)
	{
		int pid;

		if (!vote_allow_muting.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return G_INVALID;
		}
		else if ((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (!level.clients[pid].sess.muted)
		{
			G_refPrintf(ent, "Player is not muted!");
			return G_INVALID;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		int pid = Q_atoi(level.voteInfo.vote_value);

		// Mute a player
		if (level.clients[pid].sess.referee != RL_RCON)
		{
			trap_SendServerCommand(pid, va("cpm \"^3You have been un-muted\""));
			level.clients[pid].sess.muted = qfalse;
			AP(va("cp \"%s\n^3has been un-muted!\n\"", level.clients[pid].pers.netname));
			ClientUserinfoChanged(pid);
		}
		else
		{
			G_Printf("Cannot un-mute a referee.\n");
		}
	}

	return G_OK;
}

/**
 * @brief Map - simpleton: we dont verify map is allowed/exists
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Map_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		char serverinfo[MAX_INFO_STRING];

		trap_GetServerinfo(serverinfo, sizeof(serverinfo));

		if (!vote_allow_map.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			G_voteCurrentSetting(ent, arg, Info_ValueForKey(serverinfo, "mapname"));
			return G_INVALID;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			G_voteCurrentSetting(ent, arg, Info_ValueForKey(serverinfo, "mapname"));
			return G_INVALID;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);

		// Vote action (vote has passed)
	}
	else
	{
		char s[MAX_STRING_CHARS];

		if (g_gametype.integer == GT_WOLF_CAMPAIGN)
		{
			trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));
			trap_SendConsoleCommand(EXEC_APPEND, va("campaign %s%s\n", level.voteInfo.vote_value, ((*s) ? va("; set nextcampaign \"%s\"", s) : "")));
		}
		else
		{
			Svcmd_ResetMatch_f(qtrue, qfalse);
			trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
			trap_SendConsoleCommand(EXEC_APPEND, va("map %s%s\n", level.voteInfo.vote_value, ((*s) ? va("; set nextmap \"%s\"", s) : "")));
		}
	}

#ifdef FEATURE_DBMS
	// deinitialize db
	G_DB_DeInit();
#endif

	return G_OK;
}

/**
 * @brief Campaign - simpleton: we dont verify map is allowed/exists
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Campaign_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		char serverinfo[MAX_INFO_STRING];

		trap_GetServerinfo(serverinfo, sizeof(serverinfo));

		if (!vote_allow_map.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			if (g_gametype.integer == GT_WOLF_CAMPAIGN)
			{
				G_voteCurrentSetting(ent, arg, g_campaigns[level.currentCampaign].shortname);
			}
			return G_INVALID;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			if (g_gametype.integer == GT_WOLF_CAMPAIGN)
			{
				G_voteCurrentSetting(ent, arg, g_campaigns[level.currentCampaign].shortname);
			}
			return G_INVALID;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);

		// Vote action (vote has passed)
	}
	else
	{
		char s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));
		trap_SendConsoleCommand(EXEC_APPEND, va("campaign %s%s\n", level.voteInfo.vote_value, ((*s) ? va("; set nextcampaign \"%s\"", s) : "")));
	}

#ifdef FEATURE_DBMS
	// deinitialize db
	G_DB_DeInit();
#endif

	return G_OK;
}

/**
 * @brief Map Restart
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_MapRestart_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (!vote_allow_maprestart.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		else if (trap_Argc() != 2 && G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return G_INVALID;
		}
		// Vote action (vote has passed)
	}
	else
	{
		// Restart the map back to warmup
		Svcmd_ResetMatch_f(qfalse, qtrue);
		AP("cp \"^1*** Level Restarted! ***\n\"");
	}

	return G_OK;
}

/**
 * @brief Match Restart
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param arg2 - unused
 * @param[in] fRefereeCmd
 * @return
 */
int G_MatchReset_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (!vote_allow_matchreset.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		else if (trap_Argc() != 2 && G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return G_INVALID;
		}
		// Vote action (vote has passed)
	}
	else
	{
		// Restart the map back to warmup
		Svcmd_ResetMatch_f(qtrue, qtrue);
		AP("cp \"^1*** Match Reset! ***\n\"");
	}

	return G_OK;
}

/**
 * @brief Mute Spectators
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Mutespecs_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		return(G_voteProcessOnOff(ent, arg, arg2, fRefereeCmd,
		                          !!(match_mutespecs.integer),
		                          vote_allow_mutespecs.integer,
		                          dwVoteIndex));
		// Vote action (vote has passed)
	}
	else
	{
		// Mute/unmute spectators
		G_voteSetOnOff("Spectator Muting", "match_mutespecs");
	}

	return G_OK;
}

/**
 * @brief Nextmap
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param arg2 - unused
 * @param[in] fRefereeCmd
 * @return
 */
int G_Nextmap_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (trap_Argc() > 2)
		{
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg, aVoteInfo[dwVoteIndex].pszVoteHelp);
			return G_INVALID;
		}
		else if (!vote_allow_nextmap.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		else
		{
			char s[MAX_STRING_CHARS];

			if (g_gametype.integer == GT_WOLF_CAMPAIGN)
			{
				trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));
				if (!*s)
				{
					G_refPrintf(ent, "'nextcampaign' is not set.");
					return G_INVALID;
				}
			}
			else
			{
				trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
				if (!*s)
				{
					G_refPrintf(ent, "'nextmap' is not set.");
					return G_INVALID;
				}
			}
		}

		// Vote action (vote has passed)
	}
	else
	{
		if (g_gametype.integer == GT_WOLF_CAMPAIGN)
		{
			// Load in the nextcampaign
			trap_SendConsoleCommand(EXEC_APPEND, "vstr nextcampaign\n");
			AP("cp \"^3*** Loading nextcampaign! ***\n\"");
		}
		else if (g_gametype.integer == GT_WOLF_MAPVOTE)
		{
			if (g_gamestate.integer == GS_PLAYING     // don't do in intermission (check warmup/warmup-countdown
			    && (g_mapVoteFlags.integer & MAPVOTE_NEXTMAP_VOTEMAP))
			{
				// Don't do this. This is awkward, since it is not done at
				// !nextmap nor nextcampaignvotes. Besides we don't want to store
				// mapstats of an unfinished map or spend resources at generating
				// playerstats
				// G_LogExit( "Nextmap vote passed" );
				// - There is a flag for so let the users decide
				//   Some log parsers require G_LogExit
				AP("chat \"^3*** Nextmap vote passed - vote a new map! ***\"");
				G_LogExit("Nextmap vote passed");
			}
			else
			{
				AP("cp \"^3*** Loading nextmap! ***\n\"");
				// Load in the nextmap
				trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
			}
		}
		else
		{
			// Load in the nextmap
			trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
			AP("cp \"^3*** Loading nextmap! ***\n\"");
		}
	}

#ifdef FEATURE_DBMS
	// deinitialize db
	G_DB_DeInit();
#endif

	return G_OK;
}

/**
 * @brief Referee voting
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Referee_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		int pid;

		if (!vote_allow_referee.integer && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}

		if (!ent->client->sess.referee && level.numPlayingClients < 3)
		{
			G_refPrintf(ent, "Sorry, not enough clients in the game to vote for a referee");
			return G_INVALID;
		}

		if (ent->client->sess.referee && trap_Argc() == 2)
		{
			G_playersMessage(ent);
			return G_INVALID;
		}
		else if (trap_Argc() == 2)
		{
			pid = ent - g_entities;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return G_INVALID;
		}
		else if ((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee)
		{
			G_refPrintf(ent, "[lof]%s [lon]is already a referee!", level.clients[pid].pers.netname);
			return -1;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		// Voting in a new referee
		gclient_t *cl = &level.clients[atoi(level.voteInfo.vote_value)];

		if (cl->pers.connected == CON_DISCONNECTED)
		{
			AP("print \"Player left before becoming referee\n\"");
		}
		else
		{
			cl->sess.referee     = RL_REFEREE; // FIXME: Differentiate voted refs from passworded refs
			cl->sess.spec_invite = TEAM_AXIS | TEAM_ALLIES;
			AP(va("cp \"%s^7 is now a referee\n\"", cl->pers.netname));
			ClientUserinfoChanged(atoi(level.voteInfo.vote_value));
		}
	}
	return G_OK;
}

/**
 * @brief Shuffle teams
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param arg2 - unused
 * @param[in] fRefereeCmd
 * @return
 */
int G_ShuffleTeams_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (trap_Argc() > 2)
		{
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg, aVoteInfo[dwVoteIndex].pszVoteHelp);
			return G_INVALID;
		}
		else if (!vote_allow_shuffleteams.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		// Vote action (vote has passed)
	}
	else
	{
		// Shuffle the teams!
#ifdef FEATURE_RATING
		if (g_skillRating.integer)
		{
			Svcmd_ShuffleTeamsSR_f(qtrue);
		}
		else
#endif
		{
			Svcmd_ShuffleTeamsXP_f(qtrue);
		}
	}

	return G_OK;
}

/**
 * @brief Start Match
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_StartMatch_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (trap_Argc() > 2)
		{
			if (!Q_stricmp(arg2, "?"))
			{
				G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg, aVoteInfo[dwVoteIndex].pszVoteHelp);
				return G_INVALID;
			}
		}

		if (g_gamestate.integer == GS_PLAYING || g_gamestate.integer == GS_INTERMISSION)
		{
			G_refPrintf(ent, "^3Match is already in progress!");
			return G_INVALID;
		}

		if (g_gamestate.integer == GS_WARMUP_COUNTDOWN)
		{
			G_refPrintf(ent, "^3Countdown already started!");
			return G_INVALID;
		}

		if (level.numPlayingClients < match_minplayers.integer)
		{
			G_refPrintf(ent, "^3Not enough players to start match!");
			return G_INVALID;
		}

		// Vote action (vote has passed)
	}
	else
	{
		// Set everyone to "ready" status
		G_refAllReady_cmd(NULL);
	}

	return G_OK;
}

/**
 * @brief Swap teams
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param arg2 - unused
 * @param[in] fRefereeCmd
 * @return
 */
int G_SwapTeams_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (trap_Argc() > 2)
		{
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg, aVoteInfo[dwVoteIndex].pszVoteHelp);
			return G_INVALID;
		}
		else if (!vote_allow_swapteams.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}

		// Vote action (vote has passed)
	}
	else
	{
		// Swap the teams!
		Svcmd_SwapTeams_f();
	}

	return G_OK;
}

/**
 * @brief Team Damage
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_FriendlyFire_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		return(G_voteProcessOnOff(ent, arg, arg2, fRefereeCmd,
		                          !!(g_friendlyFire.integer),
		                          vote_allow_friendlyfire.integer,
		                          dwVoteIndex));
		// Vote action (vote has passed)
	}
	else
	{
		// Team damage (friendlyFire)
		G_voteSetOnOff("Friendly Fire", "g_friendlyFire");
	}

	return G_OK;
}

/**
 * @brief Anti-Lag
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_AntiLag_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		return(G_voteProcessOnOff(ent, arg, arg2, fRefereeCmd,
		                          !!(g_antilag.integer),
		                          vote_allow_antilag.integer,
		                          dwVoteIndex));
		// Vote action (vote has passed)
	}
	else
	{
		// Anti-Lag (g_antilag)
		G_voteSetOnOff("Anti-Lag", "g_antilag");
	}

	return G_OK;
}

/**
 * @brief Balanced Teams
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_BalancedTeams_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		return(G_voteProcessOnOff(ent, arg, arg2, fRefereeCmd,
		                          !!(g_balancedteams.integer),
		                          vote_allow_balancedteams.integer,
		                          dwVoteIndex));
		// Vote action (vote has passed)
	}
	else
	{
		// Balanced Teams (g_balancedteams)
		G_voteSetOnOff("Balanced Teams", "g_balancedteams");
		trap_Cvar_Set("g_teamForceBalance", level.voteInfo.vote_value);
		trap_Cvar_Set("g_lms_teamForceBalance", level.voteInfo.vote_value);
	}

	return G_OK;
}

/**
 * @brief G_Config_v
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Config_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (vote_allow_config.integer <= 0 && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		else if (trap_Argc() > 3)
		{
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg, aVoteInfo[dwVoteIndex].pszVoteHelp);
			G_PrintConfigs(ent);
			return G_INVALID;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			G_PrintConfigs(ent);
			return G_INVALID;
		}
		else if (arg2 == NULL || strlen(arg2) < 1)
		{
			G_PrintConfigs(ent);
			return G_INVALID;
		}

		if (!G_isValidConfig(ent, arg2))
		{
			return G_INVALID;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);
	}
	else // Vote action (vote has passed)
	{
		// Load in comp settings for current gametype
		if (G_configSet(level.voteInfo.vote_value))
		{
			AP("cpm \"Competition Settings Loaded!\n\"");
		}
	}

	return G_OK;
}

/**
 * @brief Timelimit
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Timelimit_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (!vote_allow_timelimit.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			G_voteCurrentSetting(ent, arg, g_timelimit.string);
			return G_INVALID;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			G_voteCurrentSetting(ent, arg, g_timelimit.string);
			return G_INVALID;
		}
		else if (atoi(arg2) < 0)
		{
			G_refPrintf(ent, "Sorry, can't specify a timelimit < 0!");
			return G_INVALID;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);

		// Vote action (vote has passed)
	}
	else
	{
		// Timelimit change
		G_voteSetVoteString("Timelimit");
	}

	return G_OK;
}

const char *warmupType[] = { "None", "Enemies Only", "Everyone" };

/**
 * @brief G_WarmupDamageTypeList
 * @param[in] ent
 */
void G_WarmupDamageTypeList(gentity_t *ent)
{
	int i;

	G_refPrintf(ent, "\nAvailable Warmup Damage types:\n------------------------------");
	for (i = 0; i < (sizeof(warmupType) / sizeof(char *)); i++)
	{
		G_refPrintf(ent, "  %d ^3(%s)", i, warmupType[i]);
	}
	G_refPrintf(ent, "\n");
}

/**
 * @brief Warmup Weapon Fire
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in,out] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Warmupfire_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		int i = Q_atoi(arg2), val = (match_warmupDamage.integer < 0) ? 0 :
		                          (match_warmupDamage.integer > 2) ? 2 :
		                          match_warmupDamage.integer;

		if (!vote_allow_warmupdamage.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			G_WarmupDamageTypeList(ent);
			G_voteCurrentSetting(ent, arg, va("%d (%s)", val, warmupType[val]));
			return G_INVALID;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			G_WarmupDamageTypeList(ent);
			G_voteCurrentSetting(ent, arg, va("%d (%s)", val, warmupType[val]));
			return G_INVALID;
		}

		if (i < 0 || i > 2)
		{
			G_refPrintf(ent, "\n^3Invalid Warmup Damage type: ^7%d", i);
			G_WarmupDamageTypeList(ent);
			return G_INVALID;
		}

		if (i == val)
		{
			G_refPrintf(ent, "\n^3Warmup Damage^5 is already set to %s!", warmupType[i]);
			return G_INVALID;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", warmupType[i]);

		// Vote action (vote has passed)
	}
	else
	{
		// Warmup damage setting
		AP(va("cpm \"^3Warmup Damage set to: ^5%s\n\"", warmupType[atoi(level.voteInfo.vote_value)]));
		trap_SendConsoleCommand(EXEC_APPEND, va("match_warmupDamage %s\n", level.voteInfo.vote_value));
	}

	return G_OK;
}

/**
 * @brief Un-Referee voting
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param[in,out] arg2
 * @param[in] fRefereeCmd
 * @return
 */
int G_Unreferee_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		int pid;

		if (!vote_allow_referee.integer && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}

		if (ent->client->sess.referee && trap_Argc() == 2)
		{
			G_playersMessage(ent);
			return G_INVALID;
		}
		else if (trap_Argc() == 2)
		{
			pid = ent - g_entities;
		}
		else if (G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return G_INVALID;
		}
		else if ((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee == RL_NONE)
		{
			G_refPrintf(ent, "[lof]%s [lon]^3isn't a referee!", level.clients[pid].pers.netname);
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee == RL_RCON)
		{
			G_refPrintf(ent, "[lof]%s's [lon]^3status cannot be removed", level.clients[pid].pers.netname);
			return G_INVALID;
		}

		if (level.clients[pid].pers.localClient)
		{
			G_refPrintf(ent, "[lof]%s [lon]^3is the Server Host", level.clients[pid].pers.netname);
			return G_INVALID;
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		// Stripping of referee status
		gclient_t *cl = &level.clients[atoi(level.voteInfo.vote_value)];

		cl->sess.referee = RL_NONE;
		// don't remove shoutcaster invitation
		if (!cl->sess.shoutcaster)
		{
			cl->sess.spec_invite = 0;
		}
		AP(va("cp \"%s^7\nis no longer a referee\n\"", cl->pers.netname));
		ClientUserinfoChanged(atoi(level.voteInfo.vote_value));
	}

	return G_OK;
}

//MAPVOTE

/**
 * @brief G_IntermissionMapVote
 * @param[in,out] ent
 */
void G_IntermissionMapVote(gentity_t *ent)
{
	char arg[MAX_TOKEN_CHARS];
	int  mapID;

	if (g_gametype.integer != GT_WOLF_MAPVOTE)
	{
		CP(va("print \"^3Map voting not enabled!\n\""));
		return;
	}

	if (g_gamestate.integer != GS_INTERMISSION)
	{
		CP(va("print \"^3Can't vote until intermission\n\""));
		return;
	}

	if (!level.intermissiontime)
	{
		CP(va("print \"^3You can only vote during intermission\n\""));
		return;
	}

	if (ent->client->ps.eFlags & EF_VOTED)
	{
		CP(va("print \"^3You have already cast your vote\n\""));
		return;         // vote already cast
	}

	// normal one-map vote
	if (trap_Argc() == 2)
	{
		trap_Argv(1, arg, sizeof(arg));
		mapID = Q_atoi(arg);

		if (mapID < 0 || mapID >= MAX_VOTE_MAPS)
		{
			CP(va("print \"^3Invalid vote\n\""));
			return;
		}

		ent->client->ps.eFlags |= EF_VOTED;
		level.mapvoteinfo[mapID].numVotes++;
		level.mapvoteinfo[mapID].totalVotes++;
		ent->client->sess.mapVotedFor[0] = mapID;
	}
	else if (trap_Argc() == 4)
	{
		int voteRank;

		for (voteRank = 1; voteRank <= 3; voteRank++)
		{
			trap_Argv(voteRank, arg, sizeof(arg));
			mapID = Q_atoi(arg);

			if (mapID < 0 || mapID >= MAX_VOTE_MAPS)
			{
				continue;
			}

			ent->client->ps.eFlags                     |= EF_VOTED;
			level.mapvoteinfo[mapID].numVotes          += voteRank;
			level.mapvoteinfo[mapID].totalVotes        += voteRank;
			ent->client->sess.mapVotedFor[voteRank - 1] = mapID;
		}

		// no vote cast, don't send Tally
		if (!(ent->client->ps.eFlags & EF_VOTED))
		{
			CP(va("print \"^3Invalid vote\n\""));
			return;
		}
	}
	else
	{
		return;
	}

	// Someone has voted. Send the votetally to all ...
	// Doing it now, so there is no need for players to keep polling for this.
	G_IntermissionVoteTally(NULL);
}

/**
 * @brief G_IntermissionMapList
 * @param[in] ent
 */
void G_IntermissionMapList(gentity_t *ent)
{
	int  i;
	char mapList[MAX_STRING_CHARS];
	int  maxMaps;

	if (g_gametype.integer != GT_WOLF_MAPVOTE || !level.intermissiontime)
	{
		return;
	}

	maxMaps = g_maxMapsVotedFor.integer;
	if (maxMaps > level.mapVoteNumMaps)
	{
		maxMaps = level.mapVoteNumMaps;
	}

	Q_strncpyz(mapList, va("immaplist %d ", (g_mapVoteFlags.integer & MAPVOTE_MULTI_VOTE)), MAX_STRING_CHARS);

	for (i = 0; i < maxMaps; i++)
	{
		Q_strcat(mapList, MAX_STRING_CHARS,
		         va("%s %d %d %d ",
		            level.mapvoteinfo[level.sortedMaps[i]].bspName,
		            level.sortedMaps[i],
		            level.mapvoteinfo[level.sortedMaps[i]].lastPlayed,
		            level.mapvoteinfo[level.sortedMaps[i]].totalVotes));
	}

	trap_SendServerCommand(ent - g_entities, mapList);
	return;
}

/**
 * @brief G_IntermissionVoteTally
 * @param[in] ent
 */
void G_IntermissionVoteTally(gentity_t *ent)
{
	int  i;
	char voteTally[MAX_STRING_CHARS];
	int  maxMaps;

	if (g_gametype.integer != GT_WOLF_MAPVOTE || !level.intermissiontime)
	{
		return;
	}

	maxMaps = g_maxMapsVotedFor.integer;
	if (maxMaps > level.mapVoteNumMaps)
	{
		maxMaps = level.mapVoteNumMaps;
	}

	Q_strncpyz(voteTally, "imvotetally ", MAX_STRING_CHARS);
	for (i = 0; i < maxMaps; i++)
	{
		Q_strcat(voteTally, MAX_STRING_CHARS, va("%d ", level.mapvoteinfo[level.sortedMaps[i]].numVotes));
	}

	// when argument "ent" == NULL, the votetally should be send to all players..
	if (ent)
	{
		trap_SendServerCommand(ent - g_entities, voteTally);
	}
	else
	{
		for (i = 0; i < level.numConnectedClients; ++i)
		{
			trap_SendServerCommand(level.sortedClients[i], voteTally);
		}
	}
	return;
}

// MAPVOTE END

/**
 * @brief Shuffle teams without restart
 * @param[in] ent
 * @param[in] dwVoteIndex
 * @param[in] arg
 * @param arg2 - unused
 * @param[in] fRefereeCmd
 * @return
 */
int G_ShuffleTeams_NoRestart_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (trap_Argc() > 2)
		{
			// CHRUKER: b047 - Removed unneeded linebreak
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg, aVoteInfo[dwVoteIndex].pszVoteHelp);
			return G_INVALID;
		}
		else if (!vote_allow_shuffleteams_norestart.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		// Vote action (vote has passed)
	}
	else
	{
		// Shuffle the teams!
#ifdef FEATURE_RATING
		if (g_skillRating.integer)
		{
			Svcmd_ShuffleTeamsSR_f(qfalse);
		}
		else
#endif
		{
			Svcmd_ShuffleTeamsXP_f(qfalse);
		}
	}

	return G_OK;
}

/**
 * @brief G_Surrender_v
 * @param[in] ent
 * @param dwVoteIndex - unused
 * @param[in] arg
 * @param[in] arg2
 * @param fRefereeCmd - unused
 * @return
 */
int G_Surrender_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (!vote_allow_surrender.integer)
		{
			return G_INVALID;
		}
		if (g_gamestate.integer != GS_PLAYING)
		{
			return G_INVALID;
		}
		Q_strncpyz(arg2,
		           (ent->client->sess.sessionTeam == TEAM_AXIS) ?
		           "[AXIS]" : "[ALLIES]",
		           VOTE_MAXSTRING);
	}
	// Vote action (vote has passed)
	else if (g_gamestate.integer == GS_PLAYING)
	{
		char cs[MAX_STRING_CHARS];

		trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
		Info_SetValueForKey(cs, "w",
		                    (level.voteInfo.voteTeam == TEAM_AXIS) ? "1" : "0");
		trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
		G_LogExit(va("%s Surrender\n",
		             (level.voteInfo.voteTeam == TEAM_AXIS) ?
		             "Axis" : "Allies"));
		AP(va("chat \"%s have surrendered!\"",
		      (level.voteInfo.voteTeam == TEAM_AXIS) ?
		      "^1AXIS^7" : "^$ALLIES^7"));
	}
	return G_OK;
}

/**
 * @brief G_NextCampaign_v
 * @param ent         - unused
 * @param dwVoteIndex - unused
 * @param[in] arg
 * @param arg2        - unused
 * @param fRefereeCmd - unused
 * @return
 */
int G_NextCampaign_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (!vote_allow_nextcampaign.integer)
		{
			return G_INVALID;
		}
		if (g_gametype.integer != GT_WOLF_CAMPAIGN)
		{
			return G_INVALID;
		}
	}
	// Vote action (vote has passed)
	else
	{
		char s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));
		if (*s)
		{
			trap_SendConsoleCommand(EXEC_APPEND, "vstr nextcampaign\n");
		}
	}

#ifdef FEATURE_DBMS
	// deinitialize db
	G_DB_DeInit();
#endif

	return G_OK;
}

/**
 * @brief G_RestartCampaign_v
 * @param ent         - unused
 * @param dwVoteIndex - unused
 * @param[in] arg
 * @param arg2        - unused
 * @param fRefereeCmd - unused
 * @return
 */
int G_RestartCampaign_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (!vote_allow_restartcampaign.integer)
		{
			return G_INVALID;
		}
		if (g_gametype.integer != GT_WOLF_CAMPAIGN)
		{
			return G_INVALID;
		}
	}
	// Vote action (vote has passed)
	else
	{
		char s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));
		trap_SendConsoleCommand(EXEC_APPEND,
		                        va("campaign %s%s\n",
		                           g_campaigns[level.currentCampaign].shortname,
		                           ((*s) ? va("; set nextcampaign \"%s\"", s) : "")));
	}

	return G_OK;
}

/**
 * @brief G_Poll_v
 * @param ent         - unused
 * @param dwVoteIndex - unused
 * @param[in] arg
 * @param[out] arg2
 * @param fRefereeCmd - unused
 * @return
 */
int G_Poll_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (!vote_allow_poll.integer)
		{
			return G_INVALID;
		}
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", ConcatArgs(2));
	}
	return G_OK;
}

/**
 * @brief G_CoinToss_v
 * @param ent         - unused
 * @param dwVoteIndex - unused
 * @param[in] arg
 * @param[out] arg2
 * @param fRefereeCmd - unused
 * @return
 */
int G_CoinToss_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if (arg)
	{
		if (!vote_allow_cointoss.integer && ent && !ent->client->sess.referee)
		{
			return G_INVALID;
		}
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", ConcatArgs(2));
	}
	// Vote action (vote has passed)
	else
	{
		int  roll    = rand() % 2; // 0 or 1, 50/50 chance
		char *result = roll == 0 ? "HEADS" : "TAILS";

		G_printFull(va("Result of the coin toss is ^3%s^7!", result), NULL);
	}

	return G_OK;
}
