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
 * @file tvg_vote.c
 * @brief All callvote handling
 */

#include "tvg_local.h"
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
	int (*pVoteCommand)(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
	const char *pszVoteMessage;
	const char *pszVoteHelp;
} vote_reference_t;

/**
 * @var aVoteInfo
 * @brief VC optimizes for dup strings :)
 */
static const vote_reference_t aVoteInfo[] =
{
	{ 0x1ff, "kick",                   G_Kick_v,                   "KICK",                       " <player_id>^7\n  Attempts to kick player from server"               },
	{ 0x1ff, "mute",                   G_Mute_v,                   "MUTE",                       " <player_id>^7\n  Removes the chat capabilities of a player"         },
	{ 0x1ff, "unmute",                 G_UnMute_v,                 "UN-MUTE",                    " <player_id>^7\n  Restores the chat capabilities of a player"        },
	{ 0x1ff, "mutespecs",              G_Mutespecs_v,              "Mute Spectators",            " <0|1>^7\n  Mutes in-game spectator chat"                            },
	{ 0x1ff, "referee",                G_Referee_v,                "Referee",                    " <player_id>^7\n  Elects a player to have admin abilities"           },
	{ 0x1ff, "unreferee",              G_Unreferee_v,              "UNReferee",                  " <player_id>^7\n  Elects a player to have admin abilities removed"   },
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
 * @param[in] client
 * @param[in] fShowVote
 */
void TVG_voteHelp(gclient_t *client, qboolean fShowVote)
{
	int i, rows = 0, num_cmds = sizeof(aVoteInfo) / sizeof(aVoteInfo[0]) - 1;     // Remove terminator;
	int vi[100];            // Just make it large static.

	if (fShowVote)
	{
		CP("print \"\nValid ^3callvote^7 commands are:\n^3----------------------------\n\"");
	}

	for (i = 0; i < num_cmds; i++)
	{
		if (aVoteInfo[i].dwGameTypes & BIT(g_gametype.integer))
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
			TVG_refPrintf(client, "^5%-25s%-25s%-25s%-25s", aVoteInfo[vi[i]].pszVoteName,
			            aVoteInfo[vi[i + rows]].pszVoteName,
			            aVoteInfo[vi[i + rows * 2]].pszVoteName,
			            aVoteInfo[vi[i + rows * 3]].pszVoteName);
		}
		else if (i + rows * 2 + 1 <= num_cmds)
		{
			TVG_refPrintf(client, "^5%-25s%-25s%-25s", aVoteInfo[vi[i]].pszVoteName,
			            aVoteInfo[vi[i + rows]].pszVoteName,
			            aVoteInfo[vi[i + rows * 2]].pszVoteName);
		}
		else if (i + rows + 1 <= num_cmds)
		{
			TVG_refPrintf(client, "^5%-25s%-25s", aVoteInfo[vi[i]].pszVoteName,
			            aVoteInfo[vi[i + rows]].pszVoteName);
		}
		else
		{
			TVG_refPrintf(client, "^5%-25s", aVoteInfo[vi[i]].pszVoteName);
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
		TVG_refPrintf(ent, "\nUsage: ^3%s %s%s\n", ref_cmd, arg, aVoteInfo[cmd].pszVoteHelp);
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
	TVG_refPrintf(ent, "[lon]Sorry, [lof]^3%s^7 [lon]voting has been disabled", cmd);
}

/**
 * @brief Player ID message stub.
 * @param[in] ent
 */
void G_playersMessage(gentity_t *ent)
{
	TVG_refPrintf(ent, "Use the ^3players^7 command to find a valid player ID.");
}

/**
 * @brief Localize current parameter setting.
 * @param[in] ent
 * @param[in] cmd
 * @param[in] setting
 */
void G_voteCurrentSetting(gentity_t *ent, const char *cmd, const char *setting)
{
	TVG_refPrintf(ent, "^2%s^7 is currently ^3%s\n", cmd, setting);
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
		TVG_refPrintf(ent, "^3%s^5 is already %s!", aVoteInfo[vote_type].pszVoteName, ((curr_setting) ? ENABLED : DISABLED));
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
		else if ((pid = TVG_ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee)
		{
			TVG_refPrintf(ent, "Can't vote to kick referees!");
			return G_INVALID;
		}

		if (level.clients[pid].sess.shoutcaster)
		{
			TVG_refPrintf(ent, "Can't vote to kick shoutcasters!");
			return G_INVALID;
		}

		if (g_entities[pid].r.svFlags & SVF_BOT)
		{
			TVG_refPrintf(ent, "Can't vote to kick bots!");
			return G_INVALID;
		}

		if (!fRefereeCmd && ent)
		{
			if (level.clients[pid].sess.sessionTeam != TEAM_SPECTATOR && level.clients[pid].sess.sessionTeam != ent->client->sess.sessionTeam)
			{
				TVG_refPrintf(ent, "Can't vote to kick players on opposing team!");
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
		else if ((pid = TVG_ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee)
		{
			TVG_refPrintf(ent, "Can't vote to mute referees!");
			return G_INVALID;
		}

		if (g_entities[pid].r.svFlags & SVF_BOT)
		{
			TVG_refPrintf(ent, "Can't vote to mute bots!");
			return G_INVALID;
		}

		if (level.clients[pid].sess.muted)
		{
			TVG_refPrintf(ent, "Player is already muted!");
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
			TVClientUserinfoChanged(pid);
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
		else if ((pid = TVG_ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (!level.clients[pid].sess.muted)
		{
			TVG_refPrintf(ent, "Player is not muted!");
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
			TVClientUserinfoChanged(pid);
		}
		else
		{
			G_Printf("Cannot un-mute a referee.\n");
		}
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
			TVG_refPrintf(ent, "Sorry, not enough clients in the game to vote for a referee");
			return G_INVALID;
		}

		if (ent->client->sess.referee && trap_Argc() == 2)
		{
			//G_playersMessage(ent);
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
		else if ((pid = TVG_ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee)
		{
			TVG_refPrintf(ent, "[lof]%s [lon]is already a referee!", level.clients[pid].pers.netname);
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
			TVClientUserinfoChanged(atoi(level.voteInfo.vote_value));
		}
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
			//G_playersMessage(ent);
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
		else if ((pid = TVG_ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee == RL_NONE)
		{
			TVG_refPrintf(ent, "[lof]%s [lon]^3isn't a referee!", level.clients[pid].pers.netname);
			return G_INVALID;
		}

		if (level.clients[pid].sess.referee == RL_RCON)
		{
			TVG_refPrintf(ent, "[lof]%s's [lon]^3status cannot be removed", level.clients[pid].pers.netname);
			return G_INVALID;
		}

		if (level.clients[pid].pers.localClient)
		{
			TVG_refPrintf(ent, "[lof]%s [lon]^3is the Server Host", level.clients[pid].pers.netname);
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
		TVClientUserinfoChanged(atoi(level.voteInfo.vote_value));
	}

	return G_OK;
}

//MAPVOTE

/**
 * @brief G_IntermissionMapVote
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void G_IntermissionMapVote(gentity_t *ent, unsigned int dwCommand, int value)
{
	//char arg[MAX_TOKEN_CHARS];
	//int  mapID;

	//if (g_gametype.integer != GT_WOLF_MAPVOTE)
	//{
	//	CP(va("print \"^3Map voting not enabled!\n\""));
	//	return;
	//}

	//if (g_gamestate.integer != GS_INTERMISSION)
	//{
	//	CP(va("print \"^3Can't vote until intermission\n\""));
	//	return;
	//}

	//if (!level.intermissiontime)
	//{
	//	CP(va("print \"^3You can only vote during intermission\n\""));
	//	return;
	//}

	//if (ent->client->ps.eFlags & EF_VOTED)
	//{
	//	CP(va("print \"^3You have already cast your vote\n\""));
	//	return;         // vote already cast
	//}

	//// normal one-map vote
	//if (trap_Argc() == 2)
	//{
	//	trap_Argv(1, arg, sizeof(arg));
	//	mapID = Q_atoi(arg);

	//	if (mapID < 0 || mapID >= MAX_VOTE_MAPS)
	//	{
	//		CP(va("print \"^3Invalid vote\n\""));
	//		return;
	//	}

	//	ent->client->ps.eFlags |= EF_VOTED;
	//	level.mapvoteinfo[mapID].numVotes++;
	//	level.mapvoteinfo[mapID].totalVotes++;
	//	ent->client->sess.mapVotedFor[0] = mapID;
	//}
	//else if (trap_Argc() == 4)
	//{
	//	int voteRank;

	//	for (voteRank = 1; voteRank <= 3; voteRank++)
	//	{
	//		trap_Argv(voteRank, arg, sizeof(arg));
	//		mapID = Q_atoi(arg);

	//		if (mapID < 0 || mapID >= MAX_VOTE_MAPS)
	//		{
	//			continue;
	//		}

	//		ent->client->ps.eFlags                     |= EF_VOTED;
	//		level.mapvoteinfo[mapID].numVotes          += 1;
	//		level.mapvoteinfo[mapID].totalVotes        += 1;
	//		ent->client->sess.mapVotedFor[voteRank - 1] = mapID;
	//	}

	//	// no vote cast, don't send Tally
	//	if (!(ent->client->ps.eFlags & EF_VOTED))
	//	{
	//		CP(va("print \"^3Invalid vote\n\""));
	//		return;
	//	}
	//}
	//else
	//{
	//	return;
	//}

	//// Someone has voted. Send the votetally to all ...
	//// Doing it now, so there is no need for players to keep polling for this.
	//G_IntermissionVoteTally(NULL);
}

/**
 * @brief TVG_IntermissionMapList
 * @param[in] client
 * @param dwCommand - unused
 * @param value    - unused
 */
void TVG_IntermissionMapList(gclient_t *client, unsigned int dwCommand, int value)
{
	if (level.cmds.immaplistValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.immaplist);
	}
}

/**
 * @brief TVG_IntermissionMapHistory
 * @param[in] client
 * @param dwCommand - unused
 * @param value    - unused
 */
void TVG_IntermissionMapHistory(gclient_t *client, unsigned int dwCommand, int value)
{
	if (level.cmds.immaphistoryValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.immaphistory);
	}
}

/**
 * @brief TVG_IntermissionVoteTally
 * @param[in] client
 * @param dwCommand - unused
 * @param value    - unused
 */
void TVG_IntermissionVoteTally(gclient_t *client, unsigned int dwCommand, int value)
{
	if (level.cmds.imvotetallyValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.imvotetally);
	}
}
