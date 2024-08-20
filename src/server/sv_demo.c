/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2012-2024 Stephen Larroque <lrq3000@gmail.com>
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
 * @file sv_demo.c
 */

#include "server.h"

typedef struct gameCommands_s
{
	char commandToSave[MAX_QPATH];
	char commandToSend[MAX_QPATH];
	int commandToSaveLength;
	int commandToSendLength;

	char commands[MAX_CLIENTS][MAX_STRING_CHARS];
} gameCommands_t;

#define MAX_DEMO_GAMECOMMANDS 24
static gameCommands_t gameCommands[MAX_DEMO_GAMECOMMANDS];
int                   gameCommandsCount;

// static function decl
static void SV_DemoWriteClientConfigString(int clientNum, const char *cs_string);
static void SV_DemoStartPlayback(void);
static qboolean SV_DemoPlayNext(void);
static void SV_DemoStateChanged(void);

#define Q_IsColorStringGameCommand(p)      ((p) && *(p) == Q_COLOR_ESCAPE && *((p) + 1)) // ^[anychar]
//#define CEIL(VARIABLE) ((VARIABLE - (int)VARIABLE) == 0 ? (int)VARIABLE : (int)VARIABLE + 1) // UNUSED but can be useful

/***********************************************
 * VARIABLES
 ***********************************************/

// headers/markers for demo messages (events)
typedef enum
{
	demo_endDemo,              ///< end of demo (close the demo)
	demo_EOF,                  ///< end of file/flux (end of event, separator, notify the demo parser to iterate to the next event of the _same_ frame)
	demo_endFrame,             ///< player and gentity state marker (recorded each end of frame, hence the name) - at the same time marks the end of the demo frame

	demo_configString,         ///< config string setting event
	demo_clientConfigString,   ///< client config string setting event
	demo_clientCommand,        ///< client command event
	demo_serverCommand,        ///< server command event
	demo_serverConsoleCommand, ///< server console command
	demo_gameCommand,          ///< game command event
	demo_clientUserinfo,       ///< client userinfo event (client_t management)
	demo_entityState,          ///< entityState_t management
	demo_entityShared,         ///< entityShared_t management
	demo_playerState,          ///< players game state event (playerState_t management)

	//demo_clientUsercmd,    ///< players commands/movements packets (usercmd_t management)
} demo_ops_e;

/*** STATIC VARIABLES ***/
// We set them as static so that they are global only for this file, this limit a bit the side-effect

// Big fat buffer to store all our stuff
static byte buf[0x400000];

// save cvars and restore them after the demo
static int      savedMaxClients = -1;
char            savedCvarsInfo[BIG_INFO_STRING];
static qboolean restoreSavedCvars = qtrue;

// for restarting playback
static char savedPlaybackDemonameVal[MAX_OSPATH] = "";
static char *savedPlaybackDemoname               = savedPlaybackDemonameVal;

// arbitrary limit
#define MAX_DEMO_AUTOPLAY 10
char demoAutoPlay[MAX_DEMO_AUTOPLAY][MAX_OSPATH];

/**
 * @brief Restores all CVARs
 */
static void SV_DemoCvarsRestore()
{
	char       key[BIG_INFO_KEY];
	char       value[BIG_INFO_VALUE];
	const char *savedCvars = savedCvarsInfo;

	// restore initial cvars of the server that were modified by the demo playback
	// note: must do it before the map_restart! so that latched values such as sv_maxclients takes effect
	if (restoreSavedCvars)
	{
		Cvar_SetValue("sv_democlients", 0);

		while (savedCvars)
		{
			Info_NextPair(&savedCvars, key, value);

			if (!key[0])
			{
				break;
			}

			if (!Q_stricmp(key, "mapname"))
			{
				continue;
			}

			Cvar_SetSafe(key, value);
		}

		savedMaxClients = -1;
		Com_Memset(savedCvarsInfo, 0, BIG_INFO_STRING);

		// FIXME: game_restart is not implemented

		//if (strlen(savedFsGame))
		//{
		//	// After setting all the other vars, we switch back the mod if necessary (it must be done only AFTER all the other cvars are set, else cvars set after won't take effect!)
		//	Cbuf_AddText(va("game_restart %s\n", savedFsGame));
		//	Q_strncpyz(savedFsGame, "", MAX_QPATH);
		//}

		Com_Printf("Demo changed CVARs restored.\n");
	}
}

/***********************************************
* AUXILIARY FUNCTIONS: CHECKING, FILTERING AND CLEANING
* Functions used to either trim unnecessary or privacy concerned data, or to just check if the data should be written in the demo, relayed to a more specialized function or just dropped.
***********************************************/

/**
 * @brief Same as Q_CleanStr but also remove any ^s or special empty color created by the engine in a gamecommand.
 * @param[in] string
 * @return
 */
static char *SV_CleanStrCmd(char *string)
{
	char *d = string;
	char *s = string;
	int  c;

	while ((c = *s) != 0)
	{
		if (Q_IsColorStringGameCommand(s))
		{
			s++;
		}
		else if (c >= 0x20 && c <= 0x7E)
		{
			*d++ = c;
		}

		s++;
	}

	*d = '\0';

	return string;
}

/**
* @brief Store newest game commands during playback for reuse
*
* @param[in] cmd
* @return
*/
static qboolean SV_DemoStoreGameCommand(const char *cmd)
{
	int i;
	int index = Q_atoi(Cmd_ArgsFrom(1));

	for (i = 0; i < gameCommandsCount; i++)
	{
		if (!Q_strncmp(gameCommands[i].commandToSave, cmd, gameCommands[i].commandToSaveLength))
		{
			Q_strncpyz(gameCommands[i].commands[index], cmd, MAX_STRING_CHARS);
			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief Send newest game commands during playback
*
* @param[in] client
* @param[in] arg0
* @param[in] arg1
*/
static qboolean SV_DemoSendStoredCommands(client_t *client, const char *arg0, const char *arg1)
{
	int      i;
	int      index = Q_atoi(Cmd_ArgsFrom(1));
	qboolean found = qfalse;

	for (i = 0; i < gameCommandsCount; i++)
	{
		if (!Q_strncmp(gameCommands[i].commandToSend, arg0, gameCommands[i].commandToSendLength))
		{
			if (index >= 0 && index < MAX_CLIENTS && strlen(gameCommands[i].commands[index]))
			{
				SV_GameSendServerCommand(client - svs.clients, gameCommands[i].commands[index]);
			}

			found = qtrue;
		}
	}

	return found;
}

/**
 * @brief Check that the clientCommand is OK (if not we either continue with a more specialized function, or drop it altogether if there's a privacy concern)
 * @param[in] client
 * @param[in] cmd
 * @return
 */
static qboolean SV_CheckClientCommand(client_t *client, const char *cmd)
{
	// if that's a userinfo command, we directly handle that with a specialized function
	// (and we check that it contains at least 10 characters so that when we copy the string we don't end up copying a random address in memory)
	if (strlen(cmd) > 9 && !Q_strncmp(cmd, "userinfo", 8))
	{
		char userinfo[MAX_INFO_STRING];

		Com_Memset(userinfo, 0, MAX_INFO_STRING);

		Q_strncpyz(userinfo, cmd + 9, MAX_INFO_STRING); // trimming out the "userinfo " substring (because we only need the userinfo string)
		SV_DemoWriteClientUserinfo(client, (const char *)userinfo); // passing relay to the specialized function for this job

		return qfalse; // we return false if the check wasn't right (meaning that this function does not need to process anything)
	}
	else if (!Q_strncmp(cmd, "say_team", 8))
	{
		// Privacy check: if the command is say_team, we just drop it
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Check that the serverCommand is OK to save (or if we drop it because already handled somewhere else or it's unwanted to store)
 * @param[in] cmd
 * @return
 */
static qboolean SV_CheckServerCommand(const char *cmd)
{
	if (!Q_strncmp(cmd, "print", 5) || !Q_strncmp(cmd, "cp", 2) || !Q_strncmp(cmd, "disconnect", 10))
	{
		// If that's a print/cp command, it's already handled by gameCommand (here it's a redundancy).
		// FIXME? possibly some print/cp are not handled by gameCommand? From my tests it was ok,
		// but if someday a few prints are missing, try to delete this check...
		// For security, we also drop disconnect (even if it should not be recorded anyway in the first place),
		// because server commands will be sent to every connected clients, and so it will disconnect everyone.
		return qfalse; // we return false if the check wasn't right
	}

	return qtrue; // else, the check is OK and we continue to process with the original function
}

/**
 * @brief Check and store the last command and compare it with the current one, to avoid duplicates.
 * If onlyStore is true, it will only store the new cmd, without checking.
 *
 * @param[in] cmd
 * @param[in] onlyStore
 * @return
 */
qboolean SV_CheckLastCmd(const char *cmd, qboolean onlyStore)
{
	static char prevcmddata[MAX_STRING_CHARS];
	static char *prevcmd = prevcmddata;
	char        cleanedprevcmd[MAX_STRING_CHARS];
	char        cleanedcmd[MAX_STRING_CHARS];

	Com_Memset(cleanedprevcmd, 0, MAX_STRING_CHARS);
	Com_Memset(cleanedcmd, 0, MAX_STRING_CHARS);

	Q_strncpyz(cleanedprevcmd, SV_CleanStrCmd(va("%s", prevcmd)), MAX_STRING_CHARS);
	Q_strncpyz(cleanedcmd, SV_CleanStrCmd(va("%s", cmd)), MAX_STRING_CHARS);

	// if we only want to store, we skip any checking
	if (!onlyStore && strlen(prevcmd) > 0 && !Q_stricmp(cleanedprevcmd, cleanedcmd))
	{
		// check that the previous cmd was different from the current cmd.
		return qfalse; // drop this command, it's a repetition of the previous one
	}
	else
	{
		// memorize the current cmd for the next check (clean the cmd before, because sometimes the same string is issued by the engine with some empty colors?)
		Q_strncpyz(prevcmd, cmd, MAX_STRING_CHARS);
		return qtrue;
	}
}

/**
 * @brief Check that the gameCommand is OK and that we can save it (or if we drop it for privacy concerns and/or because it's handled somewhere else already)
 * @param[in] cmd
 * @return
 */
static qboolean SV_CheckGameCommand(const char *cmd)
{
	if (!SV_CheckLastCmd(cmd, qfalse))
	{
		// check that the previous cmd was different from the current cmd.
		// The engine may send the same command to every client by their cid
		// (one command per client) instead of just sending one command to all using NULL or -1.
		// drop this command, it's a repetition of the previous one
		return qfalse;
	}
	else
	{
		if (!Q_strncmp(cmd, "chat", 4) || !Q_strncmp(cmd, "tchat", 5))
		{
			return qfalse; // we return false if the check wasn't right
		}
		else if (!Q_strncmp(cmd, "cs", 2))
		{
			// if it's a configstring command, we handle that with the specialized function
			Cmd_TokenizeString(cmd);
			SV_DemoWriteConfigString(atoi(Cmd_Argv(1)), Cmd_Argv(2)); // relay to the specialized write configstring function
			return qfalse; // drop it with the processing of the game command
		}

		return qtrue; // else, the check is OK and we continue to process with the original function
	}
}

/**
 * @brief Check that the configstring is OK (or if we relay to ClientConfigString)
 * @param cs_index
 * @param cs_string
 * @return
 */
static qboolean SV_CheckConfigString(int cs_index, const char *cs_string)
{
	// if this is a player, we save the configstring as a clientconfigstring
	if (cs_index >= CS_PLAYERS && cs_index < CS_PLAYERS + sv_maxclients->integer)
	{
		SV_DemoWriteClientConfigString(cs_index - CS_PLAYERS, cs_string);
		// we return false if the check wasn't right (meaning that we stop the normal configstring processing since the clientconfigstring function will handle that)
		return qfalse;
	}
	else if (cs_index == CS_SYSTEMINFO)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Filters privacy keys from a client userinfo to later write in a demo file
 * @param[in,out] userinfo
 */
void SV_DemoFilterClientUserinfo(char *userinfo)
{
	Info_RemoveKey(userinfo, "cl_guid");
	Info_RemoveKey(userinfo, "n_guid"); // nitmod guid
	Info_RemoveKey(userinfo, "ip");
}

/**
 * @brief Attempts to clean invalid characters from a filename that may prevent the demo to be stored on the filesystem
 * @param[in] string
 * @return
 */
static char *SV_CleanFilename(char *string)
{
	char *d = string;
	char *s = string;
	int  c;

	while ((c = *s) != 0)
	{
		if (Q_IsColorString(s))
		{
			s++; // skip if it's a color
		}
		else if (c == 0x2D || c == 0x2E || c == 0x5F ||  // - . _
		         (c >= 0x30 && c <= 0x39) || // numbers
		         (c >= 0x41 && c <= 0x5A) || // uppercase letters
		         (c >= 0x61 && c <= 0x7A) // lowercase letters
		         )
		{
			*d++ = c; // keep if this character is not forbidden (contained inside the above whitelist)
		}

		s++; // go to next character
	}

	*d = '\0';

	return string;
}

/***********************************************
* DEMO WRITING FUNCTIONS
* Functions used to construct and write demo events
***********************************************/

/**
 * @brief Write a message/event to the demo file
 * @param[in,out] msg
 */
static void SV_DemoWriteMessage(msg_t *msg)
{
	int len;

	// write the entire message to the file, prefixed by the length
	// append EOF (end-of-file or rather end-of-flux) to the message
	// so that it will tell the demo parser when the demo will be read that the message ends here
	// and that it can proceed to the next message
	MSG_WriteByte(msg, demo_EOF);
	len = LittleLong(msg->cursize);
	(void) FS_Write(&len, 4, sv.demoFile);
	(void) FS_Write(msg->data, msg->cursize, sv.demoFile);
	MSG_Clear(msg);
}

/**
 * @brief Write a client command to the demo file
 * @param[in] client
 * @param[in] cmd
 */
static void SV_DemoWriteClientCommand(client_t *client, const char *cmd)
{
	msg_t msg;

	if (!SV_CheckClientCommand(client, cmd))
	{
		// check that this function really needs to store the command (or if another more specialized function should do it instead, eg: userinfo)
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientCommand);
	MSG_WriteByte(&msg, client - svs.clients);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/**
 * @brief SV_DemoClientCommandCapture
 * @param[in] client
 * @param[in] msg
 * @return
 */
qboolean SV_DemoClientCommandCapture(client_t *client, const char *msg)
{
	if (sv.demoState == DS_RECORDING)     // if demo is recording, we store this command and clientid
	{
		SV_DemoWriteClientCommand(client, msg);
	}
	else if (sv.demoState == DS_PLAYBACK &&
	         ((client - svs.clients) >= sv_democlients->integer) && ((client - svs.clients) < sv_maxclients->integer)) // preventing only real clients commands (not democlients commands replayed)
	{
		// if there is a demo playback, we prevent any real client from doing a team change, if so, we issue a chat messsage (except if the player join team spectator again)
		if ((!Q_stricmp(Cmd_Argv(0), "team") && Q_stricmp(Cmd_Argv(1), "s") && Q_stricmp(Cmd_Argv(1), "spectator")))
		{
			// issue a chat message only to the player trying to join a team
			SV_SendServerCommand(client, "chat \"^3Can't join a team when a demo is replaying!\"");
			// issue a chat message only to the player trying to join a team
			SV_SendServerCommand(client, "cp \"^3Can't join a team when a demo is replaying!\"");
			return qfalse;
		}

		if (SV_DemoSendStoredCommands(client, Cmd_Argv(0), Cmd_Argv(1)))
		{
			return qfalse;
		}
	}

	return qtrue;
}

/**
 * @brief Write a server command to the demo file
 * @param[in] cmd
 *
 * @note We record only server commands that are meant to be sent to everyone in the first place,
 * that's why we don't even bother to store the clientnum. For commands that were only intended to specifically one player,
 * we only record them if they were game commands (see below the specialized function),
 * else we do not because it may be unsafe (such as "disconnect" command)
 */
void SV_DemoWriteServerCommand(const char *cmd)
{
	msg_t msg;

	if (!SV_CheckServerCommand(cmd))
	{
		// check that this function really needs to store the command
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_serverCommand);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/**
* @brief Write a server console command to the demo file
* @param[in] exec_when
* @param[in] cmd
*
* Records map_restart commands so we can perform it at correct time
*
*/
void SV_DemoWriteServerConsoleCommand(int exec_when, const char *cmd)
{
	msg_t msg;

	if (Q_strncmp(cmd, "map_restart", 11))
	{
		return;
	}

	// make sure we aren't restarting twice in the same frame
	if (com_frameTime == sv.serverId)
	{
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_serverConsoleCommand);
	MSG_WriteLong(&msg, exec_when);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/**
 * @brief Write a game command to the demo file
 * @param[in] clientNum
 * @param[in] cmd
 *
 * @note Game commands are sent to server commands,
 * but we record them separately because game commands are safe to be replayed to everyone
 * (even if at recording time it was only intended to one player),
 * while server commands may be unsafe if it was dedicated only to one player (such as "disconnect")
 */
void SV_DemoWriteGameCommand(int clientNum, const char *cmd)
{
	msg_t msg;

	if (!SV_CheckGameCommand(cmd))
	{
		// check that this function really needs to store the command
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_gameCommand);
	MSG_WriteLong(&msg, clientNum);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/**
 * @brief Write a configstring to the demo file
 *
 * @details
 * cs_index = index of the configstring (see bg_public.h)
 * cs_string = content of the configstring to set
 *
 * @param[in] cs_index
 * @param[in] cs_string
 */
void SV_DemoWriteConfigString(int cs_index, const char *cs_string)
{
	msg_t msg;
	char  cindex[MAX_CONFIGSTRINGS];

	if (!SV_CheckConfigString(cs_index, cs_string))
	{
		// check that this function really needs to store the configstring (or if it's a client configstring we relay to the specialized function)
		return;
	} // else we save it below as a normal configstring (for capture scores CS_SCORES1/2, for CS_FLAGSTATUS, etc..)

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_configString);

	// convert index to a string since we don't have any other way to store values that are greater than a byte (and max_configstrings is 1024 currently)
	// FIXME: try to replace by a WriteLong instead of WriteString? Or WriteData (with length! and it uses WriteByte)
	Com_sprintf(cindex, sizeof(cindex), "%i", cs_index);

	MSG_WriteString(&msg, (const char *)cindex);
	MSG_WriteString(&msg, cs_string);
	SV_DemoWriteMessage(&msg);
}

/**
 * @brief Write a client configstring to the demo file
 *
 * @param[in] clientNum
 * @param[in] cs_string
 *
 * @note This is a bit redundant with SV_DemoWriteUserinfo, because clientCommand userinfo sets the new userinfo for a democlient,
 * and clients configstrings are always derived from userinfo string.
 * But this function is left for security purpose (so we make sure the configstring is set right).
 */
static void SV_DemoWriteClientConfigString(int clientNum, const char *cs_string)
{
	msg_t msg;

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientConfigString);
	MSG_WriteByte(&msg, clientNum);
	MSG_WriteString(&msg, cs_string);
	SV_DemoWriteMessage(&msg);
}

/**
 * @brief Write a client userinfo to the demo file (client_t fields can almost all be filled from the userinfo string)
 * @param[in] client
 * @param[in] userinfo
 *
 * @note In practice, clients userinfo should be loaded before their configstrings (because configstrings derive from userinfo in a real game)
 */
void SV_DemoWriteClientUserinfo(client_t *client, const char *userinfo)
{
	msg_t       msg;
	static char fuserinfodata[MAX_STRING_CHARS];
	static char *fuserinfo = fuserinfodata;

	// copy the client's userinfo to another storage var, so that we don't modify the client's userinfo
	// (else the filtering will clean its infos such as cl_guid and ip, and next map change the client will be disconnected because of a wrong guid/ip!)
	Q_strncpyz(fuserinfo, userinfo, sizeof(fuserinfodata));

	if (strlen(userinfo))
	{
		SV_DemoFilterClientUserinfo(fuserinfo);   // filters out privacy keys such as ip, cl_guid
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientUserinfo); // write the event marker
	MSG_WriteByte(&msg, client - svs.clients); // write the client number (client_t - svs.clients = client num int)
	MSG_WriteString(&msg, fuserinfo); // write the (filtered) userinfo string
	SV_DemoWriteMessage(&msg); // commit this demo event in the demo file
}

/*
 * @brief Write a client usercmd_t for the current packet (called from sv_client.c SV_UserMove) which contains the movements commands for the player
 *
 * @param[in] cl
 * @param[in] delta
 * @param[in] cmdCount
 * @param[in] cmds
 * @param[in] key
 *
 * @note Note: this is unnecessary to make players move, this is handled by entities management.
 * This is only used to add more data to the dama for data analysis AND avoid inactivity timer activation
 *
 * @note Enabling this feature will use a LOT more storage space, so enable it only if you will really use it.
 * Generally, you're probably better off leaving it disabled.
void SV_DemoWriteClientUsercmd( client_t *cl, qboolean delta, int cmdCount, usercmd_t *cmds, int key )
{
    msg_t msg;
    usercmd_t nullcmd;
    usercmd_t *cmd, *oldcmd;
    int i;

    //Com_Printf("DebugGBOWriteusercmd: %i\n", cl - svs.clients);

    MSG_Init(&msg, buf, sizeof(buf));
    MSG_WriteByte(&msg, demo_clientUsercmd);
    MSG_WriteByte(&msg, cl - svs.clients);
    if (delta)
    MSG_WriteByte(&msg, 1);
    else
    MSG_WriteByte(&msg, 0);
    MSG_WriteByte(&msg, cmdCount);

    Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
    oldcmd = &nullcmd;
    for ( i = 0 ; i < cmdCount ; i++ )
    {
        cmd = &cmds[i];
        MSG_WriteDeltaUsercmdKey( &msg, key, oldcmd, cmd );
        oldcmd = cmd;
    }

    SV_DemoWriteMessage(&msg);
}
*/

/**
 * @brief Write all active clients playerState (playerState_t)
 *
 * @note This is called at every game's endFrame.
 *
 * @note Contrary to the other DemoWrite functions, this one writes all entities at once in one message, instead of one entity/command per message.
 */
static void SV_DemoWriteAllPlayerState(void)
{
	msg_t         msg;
	playerState_t *player;
	int           i;

	MSG_Init(&msg, buf, sizeof(buf));

	// write clients playerState (playerState_t)
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state < CS_ACTIVE)
		{
			continue;
		}

		player = SV_GameClientNum(i);
		MSG_WriteByte(&msg, demo_playerState);
		MSG_WriteByte(&msg, i);
		MSG_WriteDeltaPlayerstate(&msg, &sv.demoPlayerStates[i], player);
		sv.demoPlayerStates[i] = *player;
	}

	// commit all these datas to the demo file
	SV_DemoWriteMessage(&msg);
}

/**
 * @brief Write all entities state (gentity_t->entityState_t)
 *
 * @note This is called at every game's endFrame.
 *
 * @note Contrary to the other DemoWrite functions, this one writes all entities at once in one message, instead of one entity/command per message.
 * This could be easily changed, but I'm not sure it would be beneficial for the CPU time and demo storage.
 */
static void SV_DemoWriteAllEntityState(void)
{
	msg_t          msg;
	sharedEntity_t *entity;
	int            i;

	MSG_Init(&msg, buf, sizeof(buf));

	// write entities (gentity_t->entityState_t or concretely sv.gentities[num].s, in gamecode level. instead of sv.)
	MSG_WriteByte(&msg, demo_entityState);
	for (i = 0; i < sv.num_entities; i++)
	{
		if (i >= sv_maxclients->integer && i < MAX_CLIENTS)
		{
			continue;
		}

		entity           = SV_GentityNum(i);
		entity->s.number = i;
		MSG_WriteDeltaEntity(&msg, &sv.demoEntities[i].s, &entity->s, qfalse);
		sv.demoEntities[i].s = entity->s;
	}

	// end marker/Condition to break: since we don't know prior how many entities we store,
	// when reading the demo we will use an empty entity to break from our while loop
	MSG_WriteBits(&msg, ENTITYNUM_NONE, GENTITYNUM_BITS);

	// commit all these datas to the demo file
	SV_DemoWriteMessage(&msg);
}

/**
 * @brief Write all entities (gentity_t->entityShared_t)
 *
 * @note This is called at every game's endFrame.
 *
 * @note Contrary to the other DemoWrite functions, this one writes all entities at once in one message, instead of one entity/command per message.
 */
static void SV_DemoWriteAllEntityShared(void)
{
	msg_t          msg;
	sharedEntity_t *entity;
	int            i;

	MSG_Init(&msg, buf, sizeof(buf));

	// write entities (gentity_t->entityShared_t or concretely sv.gentities[num].r, in gamecode level. instead of sv.)
	MSG_WriteByte(&msg, demo_entityShared);

	for (i = 0; i < sv.num_entities; i++)
	{
		if (i >= sv_maxclients->integer && i < MAX_CLIENTS)
		{
			continue;
		}

		entity = SV_GentityNum(i);
		MSG_WriteDeltaSharedEntity(&msg, &sv.demoEntities[i].r, &entity->r, qfalse, i);
		sv.demoEntities[i].r = entity->r;
	}

	// end marker/Condition to break: since we don't know prior how many entities we store,
	// when reading the demo we will use an empty entity to break from our while loop
	MSG_WriteBits(&msg, ENTITYNUM_NONE, GENTITYNUM_BITS);

	// commit all these datas to the demo file
	SV_DemoWriteMessage(&msg);
}

/**
 * @brief Record all the entities (gentities fields) and players (player_t fields) at the end of every frame (this is the only write function to be called in every frame for sure)
 *
 * @details Will be called once per server's frame
 * Called in the main server's loop SV_Frame() in sv_main.c
 * Note that this function could be called DemoWriteEndFrame,
 * because it writes once at the end of every frame (the other events are written whenever they happen using hooks)
 */
void SV_DemoWriteFrame(void)
{
	msg_t msg;

	// STEP1: write all entities states at the end of the frame

	// write entities (gentity_t->entityState_t or concretely sv.gentities[num].s, in gamecode level. instead of sv.)
	SV_DemoWriteAllEntityState();

	// write entities (gentity_t->entityShared_t or concretely sv.gentities[num].r, in gamecode level. instead of sv.)
	SV_DemoWriteAllEntityShared();

	// write clients playerState (playerState_t)
	SV_DemoWriteAllPlayerState();

	//-----------------------------------------------------

	// STEP2: write the endFrame marker and server time

	MSG_Init(&msg, buf, sizeof(buf));

	// write end of frame marker: this will commit every demo entity change (and it's done at the very end of every server frame to overwrite any change the gamecode/engine may have done)
	MSG_WriteByte(&msg, demo_endFrame);

	// write server time (will overwrite the server time every end of frame)
	MSG_WriteLong(&msg, sv.time);

	// commit data to the demo file
	SV_DemoWriteMessage(&msg);
}

/***********************************************
* DEMO MANAGEMENT FUNCTIONS
* Functions to start/stop the recording/playback of a demo file
***********************************************/

/**
* @brief SV_DemoAutoDemoRecordCheck
*
* @details sv_autoDemo 2 - Start recording only when there are active players
*          stop recording when there are no active players
*
*/
static qboolean SV_DemoAutoDemoRecordCheck(void)
{
	client_t *cl;
	int      i;
	qboolean activePlayers = qfalse;

	if (!sv.demoSupported)
	{
		Com_Printf("Demo recording is not supported for %s mod.\n", Cvar_VariableString("fs_game"));
		return qfalse;
	}

	if (svs.autoDemoTime + 10000 > svs.time)
	{
		return qfalse;
	}

	svs.autoDemoTime = svs.time;

	switch (sv_autoDemo->integer)
	{
	case 1:
		return sv.demoState == DS_NONE;
	case 2:

		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		{
			if (cl->state == CS_ACTIVE && cl->gentity && !(cl->gentity->r.svFlags & SVF_BOT))
			{
				activePlayers = qtrue;
				break;
			}
		}

		if (activePlayers && sv.demoState == DS_NONE)
		{
			return qtrue;
		}

		if (!activePlayers && sv.demoState == DS_RECORDING)
		{
			SV_DemoStopAll();
		}
	// fall through
	default:
		return qfalse;
	}
}

/**
 * @brief Generates a meaningful demo filename and automatically starts the demo recording.
 *
 * @details This function is used in conjunction with the variable sv_autoDemo 1
 *
 * @note Be careful, if the hostname contains bad characters, the demo may not be able to be saved at all!
 * There's a small filtering in place but a bad filename may pass through!
 *
 * @note this function is called at MapRestart and SpawnServer (called in Map func),
 * but in no way it's called right at the time it's set.
 */
void SV_DemoAutoDemoRecord(void)
{
	qtime_t now;
	char    demoname[MAX_OSPATH];

	Com_Memset(demoname, 0, MAX_OSPATH);

	if (!SV_DemoAutoDemoRecordCheck())
	{
		return;
	}

	Com_RealTime(&now);
	Q_strncpyz(demoname, va("%s_%04d-%02d-%02d-%02d-%02d-%02d_%s",
	                        SV_CleanFilename(va("%s", sv_hostname->string)),
	                        1900 + now.tm_year,
	                        1 + now.tm_mon,
	                        now.tm_mday,
	                        now.tm_hour,
	                        now.tm_min,
	                        now.tm_sec,
	                        SV_CleanFilename(Cvar_VariableString("mapname"))),
	           MAX_OSPATH);

	// print a message
	Com_Printf("DEMO: automatic recording server-side demo to: %s/svdemos/%s.%s%d\n", strlen(Cvar_VariableString("fs_game")) ? Cvar_VariableString("fs_game") : BASEGAME, demoname, SVDEMOEXT, PROTOCOL_VERSION);
	SV_SendServerCommand(NULL, "chat \"^3DEMO: automatic recording server-side demo to: %s.%s%d.\"", demoname, SVDEMOEXT, PROTOCOL_VERSION);
	// launch the demo recording
	Cbuf_AddText(va("demo_record %s\n", demoname));
}

/**
* @brief SV_DemoRestartPlayback Just issue again the demo_play command along with the demo filename
* (used when the system will automatically restart the server for special cvars values to be set up, eg: sv_maxclients, fs_game, g_gametype, etc..)
*/
void SV_DemoRestartPlayback(void)
{
	if (strlen(savedPlaybackDemoname))
	{
		// set the demoState from DS_WAITINGPLAYBACK to DS_NONE (this avoids the engine to repeatedly restart the playback until it gets done, here we just launch it once)
		// this fix messages bug and accelerate the loading of the demo when switching mods
		sv.demoState = DS_RESTART;
		Cvar_SetValue("sv_demoState", DS_RESTART);
		// restart the playback (reissue the demo_play command and the demo filename)
		Cbuf_AddText(va("%s\n", savedPlaybackDemoname));
	}
	return;
}

/**
* @brief SV_DemoPlayNext Plays next demo from the list
*/
static qboolean SV_DemoPlayNext(void)
{
	int i;

	for (i = 0; i < MAX_DEMO_AUTOPLAY; i++)
	{
		if (strlen(demoAutoPlay[i]))
		{
			restoreSavedCvars = qfalse;
			Com_sprintf(sv.demoName, sizeof(sv.demoName), "%s", demoAutoPlay[i]);
			Com_Memset(demoAutoPlay[i], 0, sizeof(demoAutoPlay[i]));

			if (com_sv_running->integer)
			{
				Cbuf_AddText("map_restart 0\n");
			}

			Cbuf_AddText(va("demo_play \"%s\"\n", sv.demoName));

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief SV_DemoStopPlayback Close the demo file and restart the map (can be used both when recording or when playing or at shutdown of the game)
 */
static void SV_DemoStopPlayback(const char *message)
{
	client_t *client;
	int      i;

	FS_FCloseFile(sv.demoFile);

	Com_Printf("%s (%s)\n", message, sv.demoName);

	if (sv.demoState == DS_ERROR)
	{
		sv.demoState = DS_NONE;
		Cvar_SetValue("sv_demoState", DS_NONE);
		Cvar_SetValue("sv_killserver", 1); // do we have to do this for our listen sever? - we do Com_Error after ...
		SV_DemoCvarsRestore();
		if (strlen(message))
		{
			Com_Error(ERR_DROP, "%s", message);
		}
		else
		{
			Com_Error(ERR_DROP, "An error happened while replaying the demo, please check the log for more info.");
		}
	}
	else if (sv.demoState == DS_PLAYBACK)
	{
		if (!SV_DemoPlayNext())
		{
			Cvar_SetValue("sv_killserver", 1);
			SV_DemoCvarsRestore();
		}
		else
		{
			// unload democlients
			for (i = 0; i < sv_democlients->integer; i++)
			{
				client = &svs.clients[i];
				if (client->demoClient)
				{
					SV_DropClient(client, "disconnected");   // same as SV_Disconnect_f(client);
					client->demoClient = qfalse;
				}
			}
		}

		sv.demoState = DS_NONE;
		Cvar_SetValue("sv_demoState", DS_NONE);
	}
}

/**
* @brief SV_DemoPlaybackError Call this to abort the demo play by error.
* @param [in]
*/
static void SV_DemoPlaybackError(const char *message)
{
	sv.demoState = DS_ERROR;
	SV_DemoStopPlayback(va("SV_DemoPlaybackError: %s", message));
}

/**
 * @brief SV_DemoStartPlayback Start the playback of a demo
 *
 * @details sv.demo* have already been set and the demo file opened, start reading gamestate info.
 *
 * This function will also check that everything is alright (such as gametype, map, sv_fps, sv_democlients, sv_maxclients, etc.),
 * if not it will try to fix it automatically
 *
 * @note For developers: this is basically a mirror of SV_DemoStartRecord() but the other way around (there it writes, here it reads)
 * - but only for the part about the headers, for the rest (eg: userinfo, configstrings, playerstates) it's directly managed by ReadFrame.
 */
static void SV_DemoStartPlayback(void)
{
	msg_t      msg;
	char       map[MAX_QPATH], fs[MAX_QPATH];
	char       key[BIG_INFO_STRING], value[BIG_INFO_STRING];
	char       hostname[MAX_NAME_LENGTH]; // unused
	const char *metadata;
	int        i, r, time = 400, fps = 20, clients = 0, gametype = 0;

	Com_Memset(map, 0, MAX_QPATH);
	Com_Memset(fs, 0, MAX_QPATH);
	Com_Memset(hostname, 0, MAX_NAME_LENGTH);

	MSG_Init(&msg, buf, sizeof(buf));

	// get the demo header
	r = FS_Read(&msg.cursize, 4, sv.demoFile);
	if (r != 4)
	{
		SV_DemoPlaybackError("DEMOERROR: SV_DemoReadFrame: demo is corrupted (not initialized correctly!)");
	}
	msg.cursize = LittleLong(msg.cursize);
	if (msg.cursize == -1)
	{
		SV_DemoPlaybackError("DEMOERROR: SV_DemoReadFrame: demo is corrupted (demo file is empty?)");
	}

	if (msg.cursize > msg.maxsize)
	{
		SV_DemoPlaybackError("DEMOERROR: SV_DemoReadFrame: demo message too long");
	}

	r = FS_Read(msg.data, msg.cursize, sv.demoFile);
	if (r != msg.cursize)
	{
		SV_DemoPlaybackError("DEMOERROR: Demo file was truncated.\n");
	}

	if (restoreSavedCvars)
	{
		Com_Memset(savedCvarsInfo, 0, BIG_INFO_STRING);
		Q_strncpyz(savedCvarsInfo, Cvar_InfoString_Big(CVAR_SERVERINFO | CVAR_WOLFINFO), BIG_INFO_STRING);
		Info_SetValueForKey(savedCvarsInfo, "sv_fps", va("%i", sv_fps->integer));
		Info_SetValueForKey(savedCvarsInfo, "sv_serverTimeReset", va("%i", sv_serverTimeReset->integer));
	}

	Cvar_SetValue("sv_serverTimeReset", 1);

	metadata = MSG_ReadString(&msg);

	// reading meta-data (mostly cvars)
	while (metadata)
	{
		Info_NextPair(&metadata, key, value);

		if (!key[0])
		{
			break;
		}

		if (!Q_stricmp(key, "time"))
		{
			// svs.time
			time = Q_atoi(value);
			if (time < 400)
			{
				SV_DemoPlaybackError(va("DEMOERROR: Demo time too small: %d.\n", time));
			}
		}
		else if (!Q_stricmp(key, "sv_fps"))
		{
			fps = Q_atoi(value);
			if (sv_fps->integer != fps && fps > 0)
			{
				Cvar_SetValue("sv_fps", fps);
			}
		}
		else if (!Q_stricmp(key, "sv_maxclients"))
		{
			// number of democlients (sv_maxclients at the time of the recording)
			clients = Q_atoi(value);

			if (sv_maxclients->integer < clients ||
			    savedMaxClients < 0 || // if there's no savedMaxClients (so this means we didin't change sv_maxclients yet, and we always need to do so since we need to add sv_democlients)
			    sv_maxclients->integer <= savedMaxClients)
			{ // or if maxclients is below or equal to the previous value of maxclients (normally it can only be equal, but if we switch the mod with game_restart,
				// it can get the default value of 8, which can be below, so we need to check that as well)

				// save the old values of sv_maxclients and sv_democlients to later restore them
				if (savedMaxClients < 0) // save only if it's the first value, the subsequent ones may be default values of the engine
				{
					savedMaxClients = sv_maxclients->integer;
				}

				// automatically adjusting sv_democlients and sv_maxclients
				Cvar_SetValue("sv_democlients", clients);
				Cvar_SetLatched("sv_maxclients", va("%i", (sv_maxclients->integer + clients > MAX_CLIENTS) ? MAX_CLIENTS : sv_maxclients->integer + clients));

				Com_Printf("DEMO: Not enough demo slots, automatically increasing sv_democlients to %d and sv_maxclients to %d.\n",
				           clients, (sv_maxclients->integer + clients > MAX_CLIENTS) ? MAX_CLIENTS : sv_maxclients->integer + clients);
			}
		}
		else if (!Q_stricmp(key, "g_gametype"))
		{
			gametype = Q_atoi(value);
		}
		else if (!Q_stricmp(key, "gamename"))
		{
			// fs_game
			Q_strncpyz(fs, value, MAX_QPATH);
			if (strlen(fs))
			{
				Com_Printf("DEMO: Warning: this demo was recorded for the following mod: %s\n", fs);
			}
		}
		else if (!Q_stricmp(key, "mapname"))
		{
			Q_strncpyz(map, value, MAX_QPATH);
			if (!FS_FOpenFileRead(va("maps/%s.bsp", map), NULL, qfalse))
			{
				SV_DemoPlaybackError(va("Map does not exist: %s.\n", map));
			}
		}
		else if (!Q_stricmp(key, "hostname"))
		{
			Q_strncpyz(hostname, value, MAX_NAME_LENGTH);

		}
		else
		{
			Cvar_SetSafe(key, value);
		}
	}

	if (sv_demoState->integer != DS_RESTART)
	{
		restoreSavedCvars = qfalse;
		SV_DemoStopPlayback("Demo waiting for playback.");

		// set the status WAITINGPLAYBACK meaning that as soon as the server will be restarted, the next SV_Frame() iteration must reactivate the demo playback
		// set the cvar too because when restarting the server, all sv.* vars will be destroyed (FIXME: SO WHATS THE POINT OF sv.demoState?????)
		sv.demoState = DS_WAITINGPLAYBACK;
		Cvar_SetValue("sv_demoState", DS_WAITINGPLAYBACK);

		// we need to copy the value because since we may spawn a new server (if the demo is played client-side OR if we change fs_game), we will lose all sv. vars
		// savedPlaybackDemoname is used to restart the playback
		Q_strncpyz(savedPlaybackDemoname, Cmd_Cmd(), MAX_OSPATH);

		// FIXME: game_restart is not implemented

		//if ((Q_stricmp(Cvar_VariableString("fs_game"), fs) && strlen(fs)) ||
		//    (!strlen(fs) && Q_stricmp(Cvar_VariableString("fs_game"), fs) && Q_stricmp(fs, BASEGAME)))
		//{
		//	// switch the mod
		//	Com_Printf("DEMO: Trying to switch automatically to the mod %s to replay the demo\n", strlen(fs) ? fs : BASEGAME);
		//	Cvar_SetValue("sv_democlients", 0);
		//	Cbuf_AddText(va("game_restart %s\n", fs));
		//}

		// change gametype and map (using devmap to enable cheats)
		Cbuf_AddText(va("g_gametype %i\ndevmap %s\n", gametype, map));

		return;
	}

	sv.time = time;

	// initialize our stuff
	Com_Memset(sv.demoEntities, 0, sizeof(sv.demoEntities));
	Com_Memset(sv.demoPlayerStates, 0, sizeof(sv.demoPlayerStates));
	Cvar_SetValue("sv_democlients", clients); // note: we need SV_Startup() to NOT use SV_ChangeMaxClients for this to work without crashing when changing fs_game

	// FIXME: omnibot - this bot stuff isn't tested well (but better than before)
	// disable bots and ensure they don't connect again
	if (Cvar_Get("omnibot_enable", "0", 0)->integer > 0) // FIXME: and/or check for bot player count, omnibot_enable is latched !!!
	{

		Cbuf_ExecuteText(EXEC_NOW, "bot maxbots 0; bot kickall\n");
		Cvar_SetValue("omnibot_enable", 0); // FIXME: restore value and bot players after demo play

		Com_Printf("Bye bye omnibots - you are not allowed to view this demo.\n");
	}

	// force all real clients already connected before the demo begins to be set to spectator team
	for (i = sv_democlients->integer; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			// only set as spectator a player that is at least connected (or primed or active)
			SV_ExecuteClientCommand(&svs.clients[i], "team spectator", qtrue, qfalse);
			Cbuf_ExecuteText(EXEC_NOW, va("forceteam %i spectator", i));
		}
	}

	Com_Printf("Playing server-side demo %s.\n", sv.demoName);
	SV_SendServerCommand(NULL, "chat \"^3DEMO: Server-side demo replay started!\"");   // send a message to player
	SV_SendServerCommand(NULL, "cp \"^3DEMO: Server-side demo replay started!\"");   // send a centerprint message to player

	sv.demoState = DS_PLAYBACK;
	Cvar_SetValue("sv_demoState", DS_PLAYBACK);

	// don't save values anymore: the next time we stop playback, we will restore previous values (unless we are in autoplay)
	restoreSavedCvars = qtrue;

	SV_DemoStateChanged();

	// reading the first frame, which should contain some initialization events (eg: initial confistrings/userinfo when demo recording started, initial entities states and placement, etc..)
	SV_DemoReadFrame();
}

/**
 * @brief Start the recording of a demo by saving some headers data (such as the number of clients, mapname, gametype, etc..)
 * @details sv.demo* have already been set and the demo file opened, start writing gamestate info
 */
static void SV_DemoStartRecord(void)
{
	msg_t msg;
	char  *info;
	int   i;

	// set democlients to 0 since it's only used for replaying demo
	Cvar_SetValue("sv_democlients", 0);

	MSG_Init(&msg, buf, sizeof(buf));

	info = Cvar_InfoString_Big(CVAR_SERVERINFO | CVAR_WOLFINFO);

	Info_SetValueForKey(info, "time", va("%i", sv.time));
	Info_SetValueForKey(info, "sv_fps", va("%i", sv_fps->integer));
	MSG_WriteString(&msg, info);

	// write all the above into the demo file
	SV_DemoWriteMessage(&msg);

	// write initial clients userinfo
	// note: should be before clients configstrings since clients configstrings are derived from userinfo
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		client_t *client = &svs.clients[i];

		if (client->state >= CS_CONNECTED)
		{
			if (client->userinfo[0] != '\0')
			{
				// if player is connected and the userinfo exists, we store it
				SV_DemoWriteClientUserinfo(client, (const char *)client->userinfo);
			}
		}
	}

	// write all configstrings (such as current capture score CS_SCORE1/2, etc...), including clients configstrings
	// note: system configstrings will be filtered and excluded (there's a check function for that)
	// and clients configstrings  will be automatically redirected to the specialized function (see the check function)
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		// if the configstring pointer exists in memory (because we will check all the possible indexes,
		// but we don't know if they really exist in memory and are used or not, so here we check for that)
		if (sv.configstrings[i])
		{
			SV_DemoWriteConfigString(i, sv.configstrings[i]);
		}
	}

	// write entities and players
	Com_Memset(sv.demoEntities, 0, sizeof(sv.demoEntities));
	Com_Memset(sv.demoPlayerStates, 0, sizeof(sv.demoPlayerStates));

	// end of frame
	SV_DemoWriteFrame();

	// anounce we are writing the demo
	Com_Printf("DEMO: Recording server-side demo %s.\n", sv.demoName);
	SV_SendServerCommand(NULL, "chat \"^3DEMO: Recording server-side demo %s.\"", sv.demoName);

	sv.demoState = DS_RECORDING;
	Cvar_SetValue("sv_demoState", DS_RECORDING);

	SV_DemoStateChanged();
}

/**
 * @brief Stop the recording of a demo
 * @details Write end of demo (demo_endDemo marker) and close the demo file
 */
static void SV_DemoStopRecord(void)
{
	msg_t msg;

	// end the demo
	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_endDemo);
	SV_DemoWriteMessage(&msg); // this also writes demo_EOF

	// close the file (else it won't be openable until the server is closed)
	FS_FCloseFile(sv.demoFile);
	// change recording state
	sv.demoState = DS_NONE;
	Cvar_SetValue("sv_demoState", DS_NONE);
	SV_DemoStateChanged();
	// announce
	Com_Printf("DEMO: Stopped recording server-side demo %s.\n", sv.demoName);
	SV_SendServerCommand(NULL, "chat \"^3DEMO: Stopped recording server-side demo %s.\"", sv.demoName);
}

/***********************************************
 * DEMO READING FUNCTIONS
 *
 * Functions to read demo events
 ***********************************************/

/**
 * @brief Replay a client command
 *
 * @details Client command management (generally automatic, such as tinfo for HUD team overlay status, team selection, etc.)
 * - except userinfo command that is managed by another event
 *
 * @param[in] msg
 */
static void SV_DemoReadClientCommand(msg_t *msg)
{
	char *cmd;
	int  num;

	num = MSG_ReadByte(msg);
	cmd = MSG_ReadString(msg);

	SV_ExecuteClientCommand(&svs.clients[num], cmd, qtrue, qfalse); // 3rd arg = clientOK, and it's necessarily true since we saved the command in the demo (else it wouldn't be saved)
}

/**
 * @brief Replay a server command
 *
 * @details Server command management - except print/cp (dropped at recording because already handled by gameCommand).
 *
 * @param[in] msg
 */
static void SV_DemoReadServerCommand(msg_t *msg)
{
	char *cmd;

	cmd = MSG_ReadString(msg);
	SV_SendServerCommand(NULL, "%s", cmd);
}

/**
* @brief SV_DemoReadServerConsoleCommand
*
* Replay a server console command (map_restart)
*
* @param[in] msg
*/
static void SV_DemoReadServerConsoleCommand(msg_t *msg)
{
	char *cmd;
	int  exec_when;

	exec_when = MSG_ReadLong(msg);
	cmd       = MSG_ReadString(msg);
	Cbuf_ExecuteText(exec_when, cmd);
}

/**
 * @brief Replay a game command
 *
 * @details Game command management, such as prints/centerprint (cp) scores command, except chat/tchat (handled by clientCommand)
 *
 * Basically the same as demo_serverCommand (because sv_GameSendServerCommand uses SV_SendServerCommand,
 * game commands are NOT safe to be replayed to everyone, server commands may be unsafe such as disconnect)
 *
 * @param[in] msg
 */
static void SV_DemoReadGameCommand(msg_t *msg)
{
	playerState_t *player;
	client_t      *client;
	char          *cmd;
	int           clientNum, i;
	qboolean      canSend = qfalse;

	clientNum = MSG_ReadLong(msg);
	cmd       = MSG_ReadString(msg);

	// during intermission allow sending all commands to everyone that were originally send to stat collecting bot
	// if it wasn't present then stats like '/scores' might not be shown at the end of game
	if (clientNum != -1 && Q_atoi(Info_ValueForKey(sv.configstrings[CS_WOLFINFO], "gamestate")) == GS_INTERMISSION)
	{
		client = &svs.clients[clientNum];

		if (client && client->userinfo[0] && Q_atoi(Info_ValueForKey(client->userinfo, "tv")) & 1)
		{
			canSend = qtrue;
		}
	}

	if (SV_CheckLastCmd(cmd, qfalse) && clientNum < sv_maxclients->integer)
	{
		// store commands that should be sent per request
		if (SV_DemoStoreGameCommand(cmd))
		{
			return;
		}

		// do not send game commands to democlients
		for (i = sv_democlients->integer, client = svs.clients + sv_democlients->integer; i < sv_maxclients->integer; i++, client++)
		{
			player = SV_GameClientNum(i);

			if (client->state == CS_ACTIVE && (clientNum == -1 || player->clientNum == clientNum || canSend))
			{
				SV_GameSendServerCommand(i, cmd);
			}
		}
	}
}

/**
 * @brief Read a configstring from a message and load it into memory
 * @param[in] msg
 */
static void SV_DemoReadConfigString(msg_t *msg)
{
	char       key[BIG_INFO_KEY];
	char       value[BIG_INFO_VALUE];
	const char *configstring;
	int        num;

	//num = MSG_ReadLong(msg, MAX_CONFIGSTRINGS); FIXME: doesn't work, dunno why, but it would be better than a string to store a long int!
	num          = Q_atoi(MSG_ReadString(msg));
	configstring = MSG_ReadString(msg);

	// we make sure to not overwrite real client configstrings (else when the demo starts, normal players will have no name, no model and no status!)
	// this cannot be done at recording time because we can't know how many sv_maxclients will be set at replaying time
	if (num < CS_PLAYERS + sv_democlients->integer || num >= CS_PLAYERS + sv_maxclients->integer)
	{
		SV_SetConfigstring(num, configstring);

		// set cvars based on configstrings
		while (configstring)
		{
			Info_NextPair(&configstring, key, value);

			if (!key[0])
			{
				break;
			}

			if (Cvar_Flags(key) & CVAR_NONEXISTENT)
			{
				continue;
			}

			if (!Q_stricmp(key, "g_nextTimeLimit") && Q_atof(value))
			{
				Cvar_SetSafe("timelimit", value);
			}
			else if (!Q_stricmp(key, "sv_maxclients"))
			{
				continue;
			}

			Cvar_SetSafe(key, value);
		}
	}
}

#define TEAM_COMMAND_LENGTH 13

/**
 * @brief Read a demo client configstring from a message, load it into memory and broadcast changes to gamecode and clients
 * @details This function also manages demo clientbegin at connections and teamchange
 * (which are normally totally handled by the gamecode, so we can't directly access nor store these events in the demo,
 * we must use clever ways to reproduce them at the right time)
 *
 * @param[in] msg
 */
static void SV_DemoReadClientConfigString(msg_t *msg)
{
	client_t *client;
	char     *configstring;
	int      num;

	num          = MSG_ReadByte(msg);
	configstring = MSG_ReadString(msg);

	/**** DEMOCLIENTS CONNECTION MANAGEMENT  ****/
	// this part manages when a client should begin or be dropped based on the configstrings. This is a workaround because begin and disconnect events are managed in the gamecode,
	// so we here use a clever way to know when these events happen (this is based on a careful reading of how work the mechanisms that manage players in a real game, so this should be OK in any case).
	// note: this part could also be used in userinfo instead of client configstrings (but since DropClient automatically sets userinfo to null, which is not the case the other way around, this way was preferred)
	if (Q_stricmp(sv.configstrings[CS_PLAYERS + num], configstring) && configstring && strlen(configstring))
	{
		// client begin or just changed team: previous configstring and new one are different, and the new one is not null
		int svdoldteam;
		int svdnewteam;

		client = &svs.clients[num];

		svdoldteam = strlen(Info_ValueForKey(sv.configstrings[CS_PLAYERS + num], "t")) ? Q_atoi(Info_ValueForKey(sv.configstrings[CS_PLAYERS + num], "t")) : -1; // affect the new team if detected, else if an empty string is returned, just set -1 (will allow us to detect that there's really no team change instead of having 0 which is TEAM_FREE)
		svdnewteam = strlen(Info_ValueForKey(configstring, "t")) ? Q_atoi(Info_ValueForKey(configstring, "t")) : -1;

		// set the client configstring (using a standard Q3 function)
		SV_SetConfigstring(CS_PLAYERS + num, configstring);

		// set the name (useful for internal functions such as status_f). Anyway userinfo will automatically set both name (server-side) and netname (gamecode-side).
		Q_strncpyz(client->name, Info_ValueForKey(configstring, "n"), MAX_NAME_LENGTH);

		// set the remoteAddress of this client to localhost (this will set "ip\localhost" in the userinfo,
		// which will in turn force the gamecode to set this client as a localClient, which avoids inactivity timers and some other stuffs to be triggered)
		if (strlen(configstring)) // we check that the client isn't being dropped
		{
			NET_StringToAdr("localhost", &client->netchan.remoteAddress, NA_LOOPBACK);
		}

		SV_ClientEnterWorld(client, NULL);

		// DEMOCLIENT INITIAL TEAM MANAGEMENT
		// note: needed only to set the initial team of the democlients, subsequent team changes are directly handled by their clientCommands
		// if there was no team for this player before or if the new team is different
		if (configstring && strlen(configstring) &&
		    (svdoldteam == -1 || (svdoldteam != svdnewteam && svdnewteam != -1 && client->gentity->s.teamNum == TEAM_FREE))
		    )
		{
			// if the client changed team, we manually issue a team change (workaround by using a clientCommand team)

			char *svdnewteamstr;

			// FIXME: selecting medic because of possible class and weapon restrictions.
			// use client->sess.playerType from configstring (key = c) for class, client->sess.playerWeapon (key = w) for weapon, and client->sess.playerWeapon2 (key = sw) for secondary weapon
			// but it's not really needed as players are correct class and weapons anyway and medic is the safest bet if there will be weapon/class restrictions because of config issues
			switch (svdnewteam)
			{
			case TEAM_ALLIES:
				svdnewteamstr = "allies 1 8 7";
				break;
			case TEAM_AXIS:
				svdnewteamstr = "axis 1 3 2";
				break;
			case TEAM_SPECTATOR:
				svdnewteamstr = "spectator";
				break;
			case TEAM_FREE:
			default:
				svdnewteamstr = "free";
				break;
			}

			SV_ExecuteClientCommand(client, va("team %s", svdnewteamstr), qtrue, qfalse);
		}
	}
	else if (Q_stricmp(sv.configstrings[CS_PLAYERS + num], configstring) && strlen(sv.configstrings[CS_PLAYERS + num]) && (!configstring || !strlen(configstring)))
	{
		// client disconnect: different configstrings and the new one is empty, so the client is not there anymore,
		// we drop him (also we check that the old config was not empty, else we would be disconnecting a player who is already dropped)
		client = &svs.clients[num];
		SV_DropClient(client, "disconnected");   // same as SV_Disconnect_f(client);
		SV_SetConfigstring(CS_PLAYERS + num, configstring); // empty the configstring
	}
	else
	{ // in any other case (should there be?), we simply set the client configstring (which should not produce any error)
		SV_SetConfigstring(CS_PLAYERS + num, configstring);
	}
}

/**
 * @brief Read a demo client userinfo string from a message, load it into memory, fills client_t fields by parsing the userinfo and broacast the change to the gamecode and clients
 * @param msg
 *
 * @note This function should be called before SV_DemoReadClientConfigString on demo playback start and every first client connection to the server
		 because userinfo is set right on connect to the server and configstring derives from it, client slots are reused so no need to worry about going over MAX_CLIENTS.
 */
static void SV_DemoReadClientUserinfo(msg_t *msg)
{
	sharedEntity_t *entity;
	client_t       *client;
	char           *userinfo;
	int            num;

	// get client
	num    = MSG_ReadByte(msg);
	client = &svs.clients[num];
	// get userinfo
	userinfo = MSG_ReadString(msg);

	client->demoClient = qtrue;

	entity = SV_GentityNum(num);

	// set the remoteAddress of this client to localhost (this will set "ip\localhost" in the userinfo,
	// which will in turn force the gamecode to set this client as a localClient, which avoids inactivity timers and some other stuffs to be triggered)
	if (strlen(userinfo)) // we check that the client isn't being dropped (in which case we shouldn't set an address)
	{
		NET_StringToAdr("localhost", &client->netchan.remoteAddress, NA_LOOPBACK);
		entity->r.svFlags |= SVF_BOT; //Set player as a "bot" so the empty IP and GUID will not cause a kick
	}

	// update the userinfo for both the server and gamecode
	// we need to prepend the userinfo command (or in fact any word) to tokenize the userinfo string to the second index because SV_UpdateUserinfo_f expects to fetch it with Argv(1)
	// will update the server userinfo, automatically fill client_t fields and then transmit to the gamecode and call ClientUserinfoChanged()
	// which will also update the gamecode's client_t from the new userinfo (eg: name [server-side] and netname [gamecode-side] will be both updated)
	Cmd_TokenizeString(va("userinfo \"%s\"", userinfo));
	SV_UpdateUserinfo_f(client);

	// differentiate between players and bots to properly replay gamestates
	if (strlen(userinfo) && strlen(Info_ValueForKey(userinfo, "cg_etVersion")))
	{
		entity->r.svFlags &= ~SVF_BOT;
	}
}

/**
 * @brief Read the usercmd_t for a democlient and restituate the movements
 *
 * @details This is NOT needed to make democlients move, this is handled by entities management,
 * but it should avoid inactivity timer to activate and can be used for demo analysis
 *
 * @param[in] msg
 *
 * @todo FIXME: should set ucmd->serverTime = client->ps.commandTime + 2 to avoid the dropping of the usercmds_t packets, see g_active.c
 */
/*
static void SV_DemoReadClientUsercmd( msg_t *msg )
{
    int num, deltaint;
    qboolean delta;

    Cmd_SaveCmdContext(); // Save the context (tokenized strings) so that the engine can continue its usual processing normally just after this function
    num = MSG_ReadByte(msg);
    deltaint = MSG_ReadByte(msg);
    if (deltaint)
        delta = qtrue;
    else
        delta = qfalse;
    //Com_Printf("DebugGBOReadusercmd: %i\n", num);
    SV_UserMove(&svs.clients[num], msg, delta);
    Cmd_RestoreCmdContext(); // Restore the context (tokenized strings)
}
*/

/**
 * @brief Read all democlients playerstate (playerState_t) from a message and store them in a demoPlayerStates array (it will be loaded in memory later when SV_DemoReadRefresh() is called)
 * @param[in] msg
 */
static void SV_DemoReadAllPlayerState(msg_t *msg)
{
	playerState_t *player;
	int           num;

	num = MSG_ReadByte(msg);

	if (num < 0 || num >= MAX_CLIENTS)
	{
		SV_DemoPlaybackError("SV_DemoReadAllPlayerState: invalid demo message!");
	}

	player = SV_GameClientNum(num);
	MSG_ReadDeltaPlayerstate(msg, &sv.demoPlayerStates[num], player);
	sv.demoPlayerStates[num] = *player;
}

/**
 * @brief Read all entities state (gentity_t or sharedEntity_t->entityState_t) from a message and store them in a demoEntities[num].s array
 *        (it will be loaded in memory later when SV_DemoReadRefresh() is called)
 * @param[in] msg
 */
static void SV_DemoReadAllEntityState(msg_t *msg)
{
	sharedEntity_t *entity;
	int            num;

	// for each state in this message (because we store a concatenation of all entities states in a single message, we could do otherwise, but that's the design)
	while (1)
	{
		// get entity num
		num = MSG_ReadBits(msg, GENTITYNUM_BITS);

		// reached the end of the message, stop here
		if (num == ENTITYNUM_NONE)
		{
			break;
		}

		// create a blank entity
		entity = SV_GentityNum(num);
		// interpolate the new entity state from previous state in sv.demoEntities
		MSG_ReadDeltaEntity(msg, &sv.demoEntities[num].s, &entity->s, num);

		// save new entity state (in sv.demoEntities, which in other words display the new state)
		sv.demoEntities[num].s = entity->s;
	}
}

/**
 * @brief Read all shared entities (gentity_t or sharedEntity_t->entityShared_t
 * @param[in] msg
 *
 * @note sharedEntity_t = gentity_t != entityShared_t which is a subfield of gentity_t) from a message and store them in a demoEntities[num].r array
 *       (it will be loaded in memory later when SV_DemoReadRefresh() is called)
 */
static void SV_DemoReadAllEntityShared(msg_t *msg)
{
	sharedEntity_t *entity;
	int            num;

	while (1) // loop until we read the whole message (which is storing several states)
	{
		// get entity num
		num = MSG_ReadBits(msg, GENTITYNUM_BITS);

		// reached end of message, stop here
		if (num == ENTITYNUM_NONE)
		{
			break;
		}

		// load an empty entity
		entity = SV_GentityNum(num);
		// interpolate the new entity state from previous state in sv.demoEntities
		MSG_ReadDeltaSharedEntity(msg, &sv.demoEntities[num].r, &entity->r, num);

		// link/unlink the entity
		if (entity->r.linked && (!sv.demoEntities[num].r.linked ||
		                         entity->r.linkcount != sv.demoEntities[num].r.linkcount))
		{
			SV_LinkEntity(entity);
		}
		else if (!entity->r.linked && sv.demoEntities[num].r.linked)
		{
			SV_UnlinkEntity(entity);
		}

		// save the new state in sv.demoEntities (ie, display current entity state)
		sv.demoEntities[num].r = entity->r;
		if (num > sv.num_entities)
		{
			sv.num_entities = num;
		}
	}
}

/**
 * @brief Load into memory all stored demo players states and entities
 *        (which effectively overwrites the one that were previously written by the game since SV_ReadFrame is called at the very end of every game's frame iteration).
 */
static void SV_DemoReadRefreshEntities(void)
{
	int i;

	for (i = 0; i < sv_democlients->integer; i++)
	{
		*SV_GameClientNum(i) = sv.demoPlayerStates[i]; // Overwrite player states
	}

	// overwrite anything the game may have changed
	for (i = 0; i < sv.num_entities; i++)
	{
		// FIXME? shouldn't MAX_CLIENTS be sv_maxclients->integer?
		if (i >= sv_democlients->integer && i < MAX_CLIENTS)
		{
			continue;
		}

		// overwrite entities
		*SV_GentityNum(i) = sv.demoEntities[i];
	}
}

/**
 * @brief Play a frame from the demo file
 *
 * @details This function will read one frame per call, and will process every events contained (switch to the next event when meeting the demo_EOF marker)
 * until it meets the end of the frame (demo_endDemo marker).
 *
 * Called in the main server's loop SV_Frame() in sv_main.c (it's called after any other event processing,
 * so that it overwrites anything the game may have loaded into memory, but before the entities are broadcasted to the clients)
 */
qboolean SV_DemoReadFrame(void)
{
	msg_t msg;
	int   cmd, r, time;

	static int memsvtime;
	static int currentframe = -1;

read_next_demo_frame: // used to read another whole demo frame

	// demo freezed? Just stop reading the demo frames
	if (Cvar_VariableIntegerValue("sv_freezeDemo"))
	{
		// reset server time to the same time as the previous frame, to avoid the time going backward when resuming the demo (which will disconnect every players)
		sv.time = memsvtime;
		return qfalse;
	}

	currentframe++;

	if (com_timescale->value < 1.0f && com_timescale->value > 0.0f)
	{
		// check timescale: if slowed timescale (below 1.0), then we check that we pass one frame on 1.0/com_timescale (eg: timescale = 0.5, 1.0/0.5=2, so we pass one frame on two)
		if (currentframe % (int)(1.0f / com_timescale->value) != 0)
		{ // if it's not yet the right time to read the frame, we just pass and wait for the next server frame to read this demo frame
			return qfalse;
		}
	}

	MSG_Init(&msg, buf, sizeof(buf));

	while (1)
	{
read_next_demo_event: // used to read next demo event

		MSG_BeginReading(&msg);

		// get a message
		r = FS_Read(&msg.cursize, 4, sv.demoFile);

		if (r != 4)
		{
			SV_DemoPlaybackError("DEMOERROR: SV_DemoReadFrame: Demo file is corrupted");
		}
		msg.cursize = LittleLong(msg.cursize); // get the size of the next demo message

		if (msg.cursize > msg.maxsize) // if the size is too big, we throw an error
		{
			SV_DemoPlaybackError("DEMOERROR: SV_DemoReadFrame: demo message too long\n");
		}

		// fetch the demo message (using the length we got) from the demo file sv.demoFile, and store it into msg.data
		// (will be accessed automatically by MSG_thing() functions), and store in r the length of the data returned (used to check that it's correct)
		r = FS_Read(msg.data, msg.cursize, sv.demoFile);

		// if the returned length of the read demo message is not the same as the length we expected
		// (the one that was stored just prior to the demo message),
		// we return an error because we miss the demo message, and the only reason is that the file is truncated, so there's nothing to read after
		if (r != msg.cursize)
		{
			SV_DemoPlaybackError("DEMOERROR: Demo file was truncated.");
		}

		// parse the message
		while (1)
		{
			cmd = MSG_ReadByte(&msg); // read the demo message marker

			switch (cmd) // switch to the right processing depending on the type of the marker
			{
			default:
				// error tolerance mode: if we encounter an unknown demo message, we just skip to the next (this may allow for retrocompatibility)
				if (sv_demoTolerant->integer)
				{
					MSG_Clear(&msg);
					goto read_next_demo_event;
				}
				else
				{
					// drop the demo
					SV_DemoPlaybackError(va("SV_DemoReadFrame: Illegible demo message [%i %i:%i]", cmd, msg.readcount, msg.cursize));
				}

			// end of a demo event (the loop will continue to real the next event)
			// fall through
			case demo_EOF:
				MSG_Clear(&msg);
				goto read_next_demo_event;

			// general configstrings setting (such as capture scores CS_SCORES1/2, etc.) - except clients configstrings
			case demo_configString:
				SV_DemoReadConfigString(&msg);
				break;

			// client configstrings setting and clients status management
			case demo_clientConfigString:
				SV_DemoReadClientConfigString(&msg);
				break;

			// client userinfo setting and client_t fields management
			case demo_clientUserinfo:
				SV_DemoReadClientUserinfo(&msg);
				break;

			// client command management (generally automatic, such as tinfo for HUD team overlay status, team selection, etc.) - except userinfo command that is managed by another event
			case demo_clientCommand:
				SV_DemoReadClientCommand(&msg);
				break;

			// server command management - except print/cp (already handled by gameCommand),
			case demo_serverCommand:
				SV_DemoReadServerCommand(&msg);
				break;

			// server console command management (map_restart)
			case demo_serverConsoleCommand:
				SV_DemoReadServerConsoleCommand(&msg);
				break;

			// game command management - such as prints/centerprint (cp) scores command - except chat/tchat (handled by clientCommand)
			// basically the same as demo_serverCommand (because sv_GameSendServerCommand uses SV_SendServerCommand,
			// but game commands are safe to be replayed to everyone, while server commands may be unsafe such as disconnect)
			case demo_gameCommand:
				SV_DemoReadGameCommand(&msg);
				break;

			// manage playerState_t (some more players game status management, see demo_endFrame)
			case demo_playerState:
				SV_DemoReadAllPlayerState(&msg);
				break;

			// manage gentity->entityState_t (some more gentities game status management, see demo_endFrame)
			case demo_entityState:
				SV_DemoReadAllEntityState(&msg);
				break;

			// gentity_t->entityShared_t management (see g_local.h for more infos)
			case demo_entityShared:
				SV_DemoReadAllEntityShared(&msg);
				break;
			/*
			case demo_clientUsercmd:
			    SV_DemoReadClientUsercmd(&msg);
			    break;
			*/
			case -1: // no more chars in msg FIXME: inspect!
				Com_DPrintf("SV_DemoReadFrame: no chars [%i %i:%i]\n", cmd, msg.readcount, msg.cursize);
				return qfalse;

			// end of the frame - players and entities game status update: we commit every demo entity to the server, update the server time,
			// then release the demo frame reading here to the next server (and demo) frame
			case demo_endFrame:

				// update entities
				SV_DemoReadRefreshEntities();     // load into memory the demo entities (overwriting any change the game may have done)
				// set the server time
				time = MSG_ReadLong(&msg);

				// iterate a few demo frames to catch to until we are above the server time,
				// note: this is needed after map_restart (SV_MapRestart_f) and to correctly replay gamestates (when the recording started at GS_WARMUP_COUNTDOWN and GS_PLAYING)
				if (time < sv.time)
				{
					// note: having a server time below the demo time is CRITICAL, else we may send to the clients a server time that is below the previous,
					// or in the case of legacy mod the time would be set to levelTime = levelTime + level.previousTime (G_RunFrame).
					int timetoreach = sv.time;

					sv.time = time;
					while (sv.time <= timetoreach)
					{
						SV_DemoReadFrame(); // run a few frames to settle things out
					}
				}
				else
				{
					sv.time   = time;  // refresh server in-game time (overwriting any change the game may have done)
					memsvtime = sv.time;     // keep memory of the last server time, in case we want to freeze the demo
				}

				// check for timescale: if timescale is faster (above 1.0), we read more frames at once (eg: timescale=2, we read 2 frames for one call of this function)
				if (com_timescale->value > 1.0f)
				{
					// check that we've read all the frames we needed
					if (currentframe % (int)(com_timescale->value) != 0)
					{
						goto read_next_demo_frame;     // if not true, we read another frame
					}
				}

				return qfalse;     // else we end the current demo frame

			// end of the demo file - just stop playback and restore saved cvars
			case demo_endDemo:
				SV_DemoStopPlayback("End of demo reached.");
				return qtrue;
			}
		}
	}
	return qfalse;
}

/**
 * @brief SV_DemoStopAll
 */
void SV_DemoStopAll(void)
{
	// stop any demos
	if (sv.demoState == DS_RECORDING)
	{
		SV_DemoStopRecord();
	}
	else if (sv.demoState == DS_PLAYBACK)
	{
		SV_DemoStopPlayback("Demo playback stopped.");
	}
}

/**
 * @brief SV_Demo_Record_f
 */
static void SV_Demo_Record_f(void)
{
	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() > 2)
	{
		Com_Printf("Usage: demo_record <demoname>\n");
		return;
	}

	if (!sv.demoSupported)
	{
		Com_Printf("Demo recording is not supported for %s mod.\n", Cvar_VariableString("fs_game"));
		return;
	}

	if (sv.demoState != DS_NONE)
	{
		Com_Printf("A demo is already being recorded/played. Use demo_stop and retry.\n");
		return;
	}

	if (sv_maxclients->integer >= MAX_CLIENTS)
	{
		Com_Printf("DEMO: ERROR: Too many client slots, reduce sv_maxclients and retry.\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		Com_sprintf(sv.demoName, sizeof(sv.demoName), "svdemos/%s.%s%d", Cmd_Argv(1), SVDEMOEXT, PROTOCOL_VERSION);
	}
	else
	{
		int number = 0;
		// scan for a free demo name
		while (1)
		{
			Com_sprintf(sv.demoName, sizeof(sv.demoName), "svdemos/%d.%s%d", number++, SVDEMOEXT, PROTOCOL_VERSION);
			if (!FS_FileExists(sv.demoName))
			{
				break;  // file doesn't exist
			}
		}
	}

	sv.demoFile = FS_FOpenFileWrite(sv.demoName);
	if (!sv.demoFile)
	{
		Com_Printf("DEMO: ERROR: Couldn't open %s for writing.\n", sv.demoName);
		return;
	}

	SV_DemoStartRecord();
}

/**
 * @brief SV_Demo_Play_f
 */
static void SV_Demo_Play_f(void)
{
	char *arg;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: demo_play <demoname>\n");
		return;
	}

	if (sv.demoState != DS_NONE && sv.demoState != DS_WAITINGPLAYBACK && sv.demoState != DS_RESTART)
	{
		Com_Printf("A demo is already being recorded/played. Use demo_stop and retry.\n");
		return;
	}

	// check for an extension .svdm_?? (?? is protocol)
	arg = Cmd_Argv(1);
	if (!strcmp(arg + strlen(arg) - 6, va(".%s%d", SVDEMOEXT, PROTOCOL_VERSION)))
	{
		Com_sprintf(sv.demoName, sizeof(sv.demoName), "svdemos/%s", arg);
	}
	else
	{
		Com_sprintf(sv.demoName, sizeof(sv.demoName), "svdemos/%s.%s%d", arg, SVDEMOEXT, PROTOCOL_VERSION);
	}

	FS_FOpenFileRead(sv.demoName, &sv.demoFile, qtrue);
	if (!sv.demoFile)
	{
		Com_Printf("ERROR: Couldn't open %s for reading.\n", sv.demoName);
		return;
	}

	SV_DemoStartPlayback();
}

/**
* @brief SV_DemoSort sorts alphabetically
*/
static int QDECL SV_DemoSort(const void *a, const void *b)
{
	const char *fileA = a;
	const char *fileB = b;

	if (strlen(fileA) && !strlen(fileB))
	{
		return -1;
	}

	if (!strlen(fileA) && strlen(fileB))
	{
		return 1;
	}

	return Q_stricmp(fileA, fileB);
}

/**
* @brief SV_Demo_AutoPlay_f plays demos from a folder
*/
static void SV_Demo_AutoPlay_f(void)
{
	char fileList[30000];
	char path[MAX_OSPATH];
	char *arg, *fileName;
	int  i, count;

	if (sv.demoState != DS_NONE && sv.demoState != DS_WAITINGPLAYBACK && sv.demoState != DS_RESTART)
	{
		Com_Printf("A demo is already being recorded/played. Use demo_stop and retry.\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		arg = Cmd_Argv(1);
		Com_sprintf(path, sizeof(path), "svdemos/%s", arg);
	}
	else
	{
		Com_sprintf(path, sizeof(path), "svdemos");
	}

	count = FS_GetFileList(path, va("%s%d", SVDEMOEXT, PROTOCOL_VERSION), fileList, sizeof(fileList));

	if (count > MAX_DEMO_AUTOPLAY)
	{
		Com_Printf("Too many demos in a folder (%i) try removing some (Limit: %i).\n", count, MAX_DEMO_AUTOPLAY);
		return;
	}

	if (count)
	{
		Com_Memset(demoAutoPlay, 0, sizeof(demoAutoPlay));
		fileName = fileList;

		if (Cmd_Argc() == 2)
		{
			arg = va("%s/", arg);
		}
		else
		{
			arg = "";
		}

		for (i = 0; i < count; i++)
		{
			Q_strncpyz(demoAutoPlay[i], va("%s%s", arg, fileName), MAX_OSPATH);

			fileName += strlen(fileName) + 1;
		}

		qsort(demoAutoPlay, MAX_DEMO_AUTOPLAY, sizeof(demoAutoPlay[0]), SV_DemoSort);

		SV_DemoPlayNext();
		// on first demo save cvars
		restoreSavedCvars = qtrue;
		return;
	}

	Com_Printf("No demos found in %s.\n", arg);
}

/**
 * @brief SV_Demo_Stop_f
 */
static void SV_Demo_Stop_f(void)
{
	if (sv.demoState == DS_NONE)
	{
		Com_Printf("No demo is currently being recorded or played.\n");
		return;
	}

	SV_DemoStopAll();
}

/**
 * @brief SV_CompleteDemoName
 * @param args - unused
 * @param[in] argNum
 */
static void SV_CompleteDemoName(char *args, int argNum)
{
	if (argNum == 2)
	{
		char demoExt[16];

		Com_sprintf(demoExt, sizeof(demoExt), ".%s%d", SVDEMOEXT, PROTOCOL_VERSION);
		Field_CompleteFilename("svdemos", demoExt, qtrue, qtrue);
	}
}

/**
* @brief SV_Demo_Fastforward_f
*
* @details Fast-forward demo recording but stop when gamestate changes so it's easy to skip some parts like warmup.
*
*/
static void SV_Demo_Fastforward_f(void)
{
	int timetoreach, gamestate;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: demo_ff <seconds>\n");
		return;
	}

	if (sv.demoState != DS_PLAYBACK)
	{
		Com_Printf("No demo is currently being played.\n");
		return;
	}

	timetoreach = Q_atoi(Cmd_Argv(1));

	if (timetoreach <= 0)
	{
		Com_Printf("Bad argument.\n");
		return;
	}

	timetoreach = sv.time + (timetoreach * 1000);
	gamestate   = Q_atoi(Info_ValueForKey(sv.configstrings[CS_WOLFINFO], "gamestate"));

	while (sv.time < timetoreach)
	{
		if (SV_DemoReadFrame() || gamestate != Q_atoi(Info_ValueForKey(sv.configstrings[CS_WOLFINFO], "gamestate")))
		{
			break;
		}
	}
}

/**
 * @brief SV_DemoInit
 */
void SV_DemoInit(void)
{
	Cmd_AddCommand("demo_record", SV_Demo_Record_f, "Starts demo recording.");
	Cmd_AddCommand("demo_play", SV_Demo_Play_f, "Plays a demo record.", SV_CompleteDemoName);
	Cmd_AddCommand("demo_autoplay", SV_Demo_AutoPlay_f, va("Plays demos from a folder. (Max %i)", MAX_DEMO_AUTOPLAY));
	Cmd_AddCommand("demo_stop", SV_Demo_Stop_f, "Stops a demo record.");
	Cmd_AddCommand("demo_ff", SV_Demo_Fastforward_f, "Fast-forwards a demo record.");
}

/**
* @brief SV_DemoSupport
*/
void SV_DemoSupport(const char *commands)
{
	char key[BIG_INFO_KEY];
	char value[BIG_INFO_VALUE];

	sv.demoSupported = qtrue;

	SV_DemoStateChanged();

	if (sv_demoState->integer == DS_WAITINGPLAYBACK)
	{
		if (!commands || !commands[0])
		{
			return;
		}

		Com_Memset(gameCommands, 0, MAX_DEMO_GAMECOMMANDS * sizeof(gameCommands_t));
		gameCommandsCount = 0;

		while (commands)
		{
			Info_NextPair(&commands, key, value);

			if (!key[0])
			{
				break;
			}

			if (gameCommandsCount >= MAX_DEMO_GAMECOMMANDS)
			{
				break;
			}

			Q_strncpyz(gameCommands[gameCommandsCount].commandToSave, key, MAX_QPATH);
			Q_strncpyz(gameCommands[gameCommandsCount].commandToSend, value, MAX_QPATH);
			gameCommands[gameCommandsCount].commandToSaveLength = strlen(key);
			gameCommands[gameCommandsCount].commandToSendLength = strlen(value);
			gameCommandsCount++;
		}
	}
}

static void SV_DemoStateChanged(void)
{
	VM_Call(gvm, GAME_DEMOSTATECHANGED, sv_demoState->integer, sv_democlients->integer);
}
