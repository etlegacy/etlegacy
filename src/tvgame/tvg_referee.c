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
 * @file tvg_referee.c
 * @brief Referee handling
 */

#include "tvg_local.h"
#include "../../etmain/ui/menudef.h"

/**
 * @brief Parses for a referee command.
 * ref arg allows for the server console to utilize all referee commands (ent == NULL)
 * @param[in] ent
 * @param[in] cmd
 * @return
 */
qboolean TVG_refCommandCheck(gclient_t *client, const char *cmd)
{
	if (!Q_stricmp(cmd, "help"))
	{
		TVG_refHelp_cmd(client);
	}
	else if (!Q_stricmp(cmd, "warn"))
	{
		TVG_refWarning_cmd(client);
	}
	else if (!Q_stricmp(cmd, "mute"))
	{
		TVG_refMute_cmd(client, qtrue);
	}
	else if (!Q_stricmp(cmd, "unmute"))
	{
		TVG_refMute_cmd(client, qfalse);
	}
	else if (!Q_stricmp(cmd, "logout"))
	{
		TVG_refLogout_cmd(client);
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Lists ref commands.
 * @param[in] client
 */
void TVG_refHelp_cmd(gclient_t *client)
{
	// List commands only for enabled refs.
	if (client)
	{
		CP("print \"^3Referee commands:^7\n------------------------------------------\n\"");

		TVG_voteHelp(client, qfalse);

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
 * @param[in,out] client
 * @param dwCommand - unused
 * @param value - unused
 */
void TVG_ref_cmd(gclient_t *client, unsigned int dwCommand, int value)
{
	char arg[MAX_TOKEN_CHARS];

	// Roll through ref commands if already a ref
	if (client == NULL || client->sess.referee)
	{
		voteInfo_t votedata;

		trap_Argv(1, arg, sizeof(arg));

		Com_Memcpy(&level.voteInfo, &votedata, sizeof(voteInfo_t));

		if (TVG_refCommandCheck(client, arg))
		{
			return;
		}
		else
		{
			TVG_refHelp_cmd(client);
		}
		return;
	}

	if (client)
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

		client->sess.referee     = 1;
		client->sess.spec_invite = TEAM_AXIS | TEAM_ALLIES;
		AP(va("cp \"%s\n^3has become a referee\n\"", client->pers.netname));
		TVG_ClientUserinfoChanged(client - level.clients);
	}
}

/**
 * @brief G_refWarning_cmd
 * @param[in] client
 */
void TVG_refWarning_cmd(gclient_t *client)
{
	char cmd[MAX_TOKEN_CHARS];
	char reason[MAX_TOKEN_CHARS];
	int  kicknum;

	trap_Argv(2, cmd, sizeof(cmd));

	if (!*cmd)
	{
		TVG_refPrintf(client, "usage: ref warn <clientname> [reason].");
		return;
	}

	trap_Argv(3, reason, sizeof(reason));

	kicknum = TVG_ClientNumberFromString(client, cmd);

	if (kicknum != -1)
	{
		if (level.clients[kicknum].sess.referee == RL_NONE || ((!client || client->sess.referee == RL_RCON) && level.clients[kicknum].sess.referee <= RL_REFEREE))
		{
			trap_SendServerCommand(-1, va("cpm \"%s^7 was issued a ^1Warning^7 (%s)\n\"\n", level.clients[kicknum].pers.netname, *reason ? reason : "No Reason Supplied"));
		}
		else
		{
			TVG_refPrintf(client, "Insufficient rights to issue client a warning.");
		}
	}
}

/**
 * @brief (Un)Mutes a player
 * @param[in] ent
 * @param[in] mute
 */
void TVG_refMute_cmd(gclient_t *client, qboolean mute)
{
	int       pid;
	char      arg[MAX_TOKEN_CHARS];
	gclient_t *player;

	// Find the player to mute.
	trap_Argv(2, arg, sizeof(arg));
	if ((pid = TVG_ClientNumberFromString(client, arg)) == -1)
	{
		return;
	}

	player = level.clients + pid;

	// mute check so that players that are muted
	// before granted referee status, can be unmuted
	if (player->sess.referee != RL_NONE && mute)
	{
		TVG_refPrintf(client, "Cannot mute a referee.");
		return;
	}

	if (player->sess.muted == mute)
	{
		TVG_refPrintf(client, "\"%s^*\" %s", player->pers.netname, mute ? "is already muted!" : "is not muted!");
		return;
	}

	if (mute)
	{
		CPx(pid, "print \"^5You've been muted\n\"");
		player->sess.muted = qtrue;
		G_Printf("\"%s^*\" has been muted\n", player->pers.netname);
	}
	else
	{
		CPx(pid, "print \"^5You've been unmuted\n\"");
		player->sess.muted = qfalse;
		G_Printf("\"%s^*\" has been unmuted\n", player->pers.netname);
	}
	TVG_ClientUserinfoChanged(pid);
}

/**
 * @brief G_refLogout_cmd
 * @param[in] client
 */
void TVG_refLogout_cmd(gclient_t *client)
{
	if (client && client->sess.referee == RL_REFEREE)
	{
		client->sess.referee = RL_NONE;
		TVG_ClientUserinfoChanged(client - level.clients);
		CP("print \"You have been logged out\n\"");
	}
}

/**
 * @brief Client authentication
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_AuthRcon_f(gentity_t *ent, unsigned int dwCommand, int value)
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

	bannum = TVG_ClientNumberFromString(NULL, cmd);

	if (bannum != -1)
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

	cnum = TVG_ClientNumberFromString(NULL, cmd);

	if (cnum != -1)
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
			TVG_ClientUserinfoChanged(cnum);
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

	cnum = TVG_ClientNumberFromString(NULL, cmd);

	if (cnum != -1)
	{
		if (level.clients[cnum].sess.referee == RL_REFEREE)
		{
			level.clients[cnum].sess.referee = RL_NONE;
			G_Printf("%s is no longer a referee.\n", cmd);
			TVG_ClientUserinfoChanged(cnum);
		}
		else
		{
			G_Printf("User is not a referee.\n");
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

	cnum = TVG_ClientNumberFromString(NULL, cmd);

	if (cnum != -1)
	{
		if (level.clients[cnum].sess.referee != RL_RCON)
		{
			trap_SendServerCommand(cnum, va("cpm \"^3You have been muted\""));
			level.clients[cnum].sess.muted = qtrue;
			G_Printf("%s^* has been muted\n", cmd);
			TVG_ClientUserinfoChanged(cnum);
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

	cnum = TVG_ClientNumberFromString(NULL, cmd);

	if (cnum != -1)
	{
		if (level.clients[cnum].sess.muted)
		{
			trap_SendServerCommand(cnum, va("cpm \"^2You have been un-muted\""));
			level.clients[cnum].sess.muted = qfalse;
			G_Printf("%s has been un-muted\n", cmd);
			TVG_ClientUserinfoChanged(cnum);
		}
		else
		{
			G_Printf("User is not muted.\n");
		}
	}
}



/**
 * Utility
 */

/**
 * @brief TVG_refPrintf
 * @param[in] client
 * @param[in] fmt
 */
void TVG_refPrintf(gclient_t *client, const char *fmt, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	if (client == NULL)
	{
		trap_Printf(va("%s\n", text));
	}
	else
	{
		CP(va("print \"%s\n\"", text));
	}
}
