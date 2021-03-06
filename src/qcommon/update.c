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
 * @file update.c
 */

#ifdef DEDICATED
#include "../server/server.h"
#define upd svs.download
#define gamebin "etlded"
#else
#include "../client/client.h"
#define upd cls.download
#define gamebin "etl"
#endif

#ifdef _WIN32
#define UPDATE_BINARY "updater.exe"
#define GAME_BINARY gamebin ".exe"
#define CMDP "\""
#else
#define UPDATE_BINARY "updater"
#define GAME_BINARY gamebin
#define CMDP "'"
#endif
#define UPDATE_PACKAGE "updater.zip"
#define UPDATE_CONFIG "updater.xml"
#define MIN_PACK_LEN 4

autoupdate_t autoupdate;

/**
 * @brief Com_CheckAutoUpdate
 */
void Com_CheckAutoUpdate(void)
{
#ifdef FEATURE_AUTOUPDATE
	int  res;
	char info[MAX_INFO_STRING];

	if (autoupdate.autoupdateServer.type == NA_BAD)
	{
		Com_Printf("Updater: resolving %s... ", com_updateServer->string);
		res = NET_StringToAdr(com_motdServer->string, &autoupdate.autoupdateServer, NA_UNSPEC);

		if (res == 2)
		{
			// if no port was specified, use the default update port
			autoupdate.autoupdateServer.port = BigShort(PORT_UPDATE);
		}

		if (res)
		{
			Com_Printf("resolved to %s\n", NET_AdrToString(autoupdate.autoupdateServer));
		}
		else
		{
			Com_Printf(S_COLOR_YELLOW "couldn't resolve address\n");
			autoupdate.updateChecked = qtrue;
			return;
		}
	}

	info[0] = 0;
	Info_SetValueForKey(info, "version", ETLEGACY_VERSION);
	Info_SetValueForKey(info, "platform", CPUSTRING);

#ifndef DEDICATED
	Info_SetValueForKey(info, "lang", Cvar_VariableString("cl_lang"));
#endif  // DEDICATED

	Info_SetValueForKey(info, va("%s_%s.pk3", MODNAME, ETLEGACY_VERSION),
	                    Com_MD5File(va("%s/%s_%s.pk3", MODNAME, MODNAME, ETLEGACY_VERSION), 0, NULL, 0));

	NET_OutOfBandPrint(
#ifdef DEDICATED
	    NS_SERVER
#else
	    NS_CLIENT
#endif  // DEDICATED
	    , autoupdate.autoupdateServer, "getUpdateInfo \"%s\"", info);

	autoupdate.updateChecked = qtrue;

#endif  // FEATURE_AUTOUPDATE
}

/**
 * @brief Com_GetAutoUpdate
 */
void Com_GetAutoUpdate(void)
{
#ifdef FEATURE_AUTOUPDATE
	// Don't try and get an update if we haven't checked for one
	if (!autoupdate.updateChecked)
	{
		return;
	}

	if (com_updateavailable->integer != 2)
	{
#ifndef DEDICATED
		Sys_OpenURL(PRODUCT_URL, qtrue);
#endif
		return;
	}

	// Make sure there's a valid update file to request
	if (strlen(com_updatefiles->string) < MIN_PACK_LEN)
	{
		return;
	}

	Com_DPrintf("Connecting to auto-update server...\n");

#ifndef DEDICATED
	S_StopAllSounds();

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// FIXME: this should be saved & restored after update
	// toggle on all the download related cvars
	Cvar_Set("cl_allowDownload", "1");  // general flag
	Cvar_Set("cl_wwwDownload", "1");    // ftp/http support

	// clear any previous "server full" type messages
	clc.serverMessage[0] = 0;
#endif

	if (com_sv_running->integer)
	{
		// if running a local server, kill it
		SV_Shutdown("Server quit\n");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SV_Frame(0);

#ifndef DEDICATED
	CL_Disconnect(qtrue);
	Con_Close();

	Q_strncpyz(cls.servername, "ET:Legacy Update Server", sizeof(cls.servername));
#endif

	if (autoupdate.autoupdateServer.type == NA_BAD)
	{
		Com_Printf("Bad server address\n");
#ifndef DEDICATED
		cls.state = CA_DISCONNECTED;
		Cvar_Set("ui_connecting", "0");
#endif
		return;
	}

#ifndef DEDICATED
	// Copy auto-update server address to Server connect address
	Com_Memcpy(&clc.serverAddress, &autoupdate.autoupdateServer, sizeof(netadr_t));

	Com_DPrintf("%s resolved to %s\n", cls.servername,
	            NET_AdrToString(clc.serverAddress));

	cls.state = CA_DISCONNECTED;
	Cvar_Set("ui_connecting", "1");
	Cvar_Set("ui_dl_running", "1");

	cls.keyCatchers        = 0;
	clc.connectTime        = -99999; // CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set("cl_currentServerAddress", "ET:Legacy Update Server");
#endif

	Com_CheckUpdateStarted();
#endif /* FEATURE_AUTOUPDATE */
}

#ifdef FEATURE_AUTOUPDATE
/**
 * @brief Com_RunUpdateBinary
 * @param[in] updateBinary
 * @param[in] updateConfig
 */
static void Com_RunUpdateBinary(const char *updateBinary, const char *updateConfig)
{
	static char fn[MAX_OSPATH];
	char        cmdBuffer[MAX_OSPATH];

	Q_strncpyz(fn, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, updateBinary), MAX_OSPATH);

#ifndef _WIN32
	Sys_Chmod(fn, S_IXUSR);
#endif

#ifndef DEDICATED
	// will either exit with a successful process spawn, or will Com_Error ERR_DROP
	// so we need to clear the disconnected download data if needed
	if (upd.bWWWDlDisconnected)
	{
		upd.bWWWDlDisconnected = qfalse;
		Com_ClearStaticDownload();
	}
#endif

	Com_sprintf(cmdBuffer, MAX_OSPATH, CMDP "%s" CMDP, fn);
	Com_sprintf(cmdBuffer, MAX_OSPATH, " --install-dir " CMDP "%s" CMDP, Cvar_VariableString("fs_basepath"));
	Com_sprintf(cmdBuffer, MAX_OSPATH, " --package-dir " CMDP "%s" CMDP, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, NULL));
	Com_sprintf(cmdBuffer, MAX_OSPATH, " --script " CMDP "%s" CMDP, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, updateConfig));
	Com_sprintf(cmdBuffer, MAX_OSPATH, " --wait " CMDP "%s" CMDP, Cvar_VariableString("com_pid"));
	Com_sprintf(cmdBuffer, MAX_OSPATH, " --auto-close");

#if defined(__APPLE__) && !defined(DEDICATED)
	Com_sprintf(cmdBuffer, MAX_OSPATH, " --execute " CMDP "ET Legacy.app" CMDP);
#else
	Com_sprintf(cmdBuffer, MAX_OSPATH, " --execute " CMDP GAME_BINARY CMDP);
#endif
	Com_sprintf(cmdBuffer, MAX_OSPATH, " --execute-args " CMDP "%s" CMDP, Com_GetCommandLine());

	Sys_StartProcess(cmdBuffer, qtrue);

	// reinitialize the filesystem if the game directory or checksum has changed
	// - after mod update
#ifdef DEDICATED
	FS_ConditionalRestart(sv.checksumFeed);
#else
	FS_ConditionalRestart(clc.checksumFeed);
#endif
}
#endif /* FEATURE_AUTOUPDATE */

#ifdef FEATURE_AUTOUPDATE
/**
 * @brief Com_UnpackUpdatePackage
 * @param[in] pack
 * @param[in] bin
 * @param[in] config
 * @return
 */
static qboolean Com_UnpackUpdatePackage(const char *pack, const char *bin, const char *config)
{
	char *fn1 = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, pack);
	char *fn2 = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, NULL);

	if (FS_UnzipTo(fn1, fn2, qtrue))
	{
		fn1 = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, bin);
		fn2 = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, config);

		if (FS_FileInPathExists(fn1) && FS_FileInPathExists(fn2))
		{
			return qtrue;
		}
		else
		{
			Com_Printf("The updater binary or config does not exist\n");
		}
	}
	else
	{
		Com_Printf("Failed to unpack the update package\n");
	}

	return qfalse;
}
#endif /* FEATURE_AUTOUPDATE */

#ifdef FEATURE_AUTOUPDATE
/**
 * @brief Com_CLeanUpdateFolder
 * @param[in] bin
 */
static void Com_CLeanUpdateFolder(const char *bin)
{
	//We just remove the old updater here, if it exists.
	//The update installer itself does other cleanups
	char *fn1 = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, bin);
	if (FS_FileInPathExists(fn1))
	{
		FS_Remove(fn1);
	}
}
#endif /* FEATURE_AUTOUPDATE */

/**
 * @brief Com_CheckUpdateDownloads
 * @return
 */
qboolean Com_CheckUpdateDownloads(void)
{
#ifdef FEATURE_AUTOUPDATE
	// Auto-update
	if (autoupdate.updateStarted)
	{
		if (strlen(com_updatefiles->string) > MIN_PACK_LEN)
		{
			Com_InitDownloads();
			return qtrue;
		}

		Com_CLeanUpdateFolder(UPDATE_BINARY);

		if (Com_UnpackUpdatePackage(UPDATE_PACKAGE, UPDATE_BINARY, UPDATE_CONFIG))
		{
			Com_RunUpdateBinary(UPDATE_BINARY, UPDATE_CONFIG);
		}
		else
		{
			Cvar_Set("ui_connecting", "0");
			Cvar_Set("ui_dl_running", "0");
		}

		autoupdate.updateStarted = qfalse;

		CL_Disconnect(qtrue);

		// we can reset that now
		upd.bWWWDlDisconnected = qfalse;
		Com_ClearStaticDownload();

		return qtrue;
	}
#endif // FEATURE_AUTOUPDATE
	return qfalse;
}

/**
 * @brief Com_InitUpdateDownloads
 * @return
 */
qboolean Com_InitUpdateDownloads(void)
{
#ifdef FEATURE_AUTOUPDATE
	if (autoupdate.updateStarted
#ifndef DEDICATED
	    && NET_CompareAdr(autoupdate.autoupdateServer, clc.serverAddress)
#endif
	    )
	{
		if (strlen(com_updatefiles->string) > MIN_PACK_LEN)
		{
			char *updateFile;
			char updateFilesRemaining[MAX_TOKEN_CHARS] = "";

			upd.bWWWDl             = qtrue;
			upd.bWWWDlDisconnected = qtrue;

			updateFile = strtok(com_updatefiles->string, ";");

			if (updateFile == NULL)
			{
				Com_Error(ERR_AUTOUPDATE, "Could not parse update string.");
			}
			else
			{
				// download format: @remotename@localname
				Q_strncpyz(upd.downloadList, va("@%s@%s", updateFile, updateFile), MAX_INFO_STRING);
				Q_strncpyz(upd.originalDownloadName, va("%s/%s", AUTOUPDATE_DIR, updateFile), sizeof(upd.originalDownloadName));

				if (!Q_stricmp(updateFile, UPDATE_PACKAGE))
				{
					Q_strncpyz(upd.downloadName, va("%s/updater/%s-%s-%s", com_updateServer->string, ETLEGACY_VERSION_SHORT, CPUSTRING, updateFile), sizeof(upd.downloadName));
				}
				else
				{
					Q_strncpyz(upd.downloadName, va("%s/packages/%s", com_updateServer->string, updateFile), sizeof(upd.downloadName));
				}

				Q_strncpyz(upd.downloadTempName, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, va("%s.tmp", updateFile)), sizeof(upd.downloadTempName));
				// TODO: add file size, so UI can show progress bar
				//Cvar_SetValue("cl_downloadSize", clc.downloadSize);

				if (!DL_BeginDownload(upd.downloadTempName, upd.downloadName))
				{
					Com_Error(ERR_AUTOUPDATE, "Could not download an update file: \"%s\"", upd.downloadName);
					upd.bWWWDlAborting = qtrue; //TODO: never executed
				}

				while (1)
				{
					updateFile = strtok(NULL, ";");

					if (updateFile == NULL)
					{
						break;
					}

					Q_strcat(updateFilesRemaining, sizeof(updateFilesRemaining), va("%s;", updateFile));
				}

				if (strlen(updateFilesRemaining) > MIN_PACK_LEN)
				{
					Cvar_Set("com_updatefiles", updateFilesRemaining);
				}
				else
				{
					Cvar_Set("com_updatefiles", "");
				}
			}
		}
		return qtrue;
	}
#endif // FEATURE_AUTOUPDATE

	return qfalse;
}

/**
 * @brief Com_UpdatePacketEvent
 * @param[in] from
 * @return
 */
qboolean Com_UpdatePacketEvent(netadr_t from)
{
#ifdef FEATURE_AUTOUPDATE
	static qboolean autoupdateRedirected = qfalse;
	// Update server doesn't understand netchan packets
	if (NET_CompareAdr(autoupdate.autoupdateServer, from))
	{
		if (autoupdate.updateStarted && !autoupdateRedirected)
		{
			autoupdateRedirected = qtrue;
			//CL_InitDownloads();
			return qtrue;
		}
	}
#endif /* FEATURE_AUTOUPDATE */

	return qfalse;
}

/**
 * @brief Com_UpdateInfoPacket
 * @param[in] from
 */
void Com_UpdateInfoPacket(netadr_t from)
{
	if (autoupdate.autoupdateServer.type == NA_BAD)
	{
		Com_DPrintf("Com_UpdateInfoPacket: Update server has bad address\n");
		return;
	}

	Com_DPrintf("Update server resolved to %s\n",
	            NET_AdrToString(autoupdate.autoupdateServer));

	if (!NET_CompareAdr(from, autoupdate.autoupdateServer))
	{
#ifdef DEDICATED
		SV_WriteAttackLog(va("bad update info packet from %s\n", NET_AdrToString(from)));
#else
		Com_DPrintf("Com_UpdateInfoPacket: Ignoring packet from %s, because the update server is located at %s\n",
		            NET_AdrToString(from), NET_AdrToString(autoupdate.autoupdateServer));
#endif
		return;
	}

	Cvar_Set("com_updateavailable", Cmd_Argv(1));
	Cvar_Set("com_updatefiles", "");

	if (com_updateavailable->integer)
	{
		Cvar_Set("com_updatemessage", Cmd_Argv(2));

		if (com_updateavailable->integer == 2)
		{
			Cvar_Set("com_updatefiles", Cmd_Argv(3));

			if (autoupdate.forceUpdate)
			{
				Com_GetAutoUpdate();
				autoupdate.forceUpdate = qfalse;
				return;
			}
		}

#ifdef FEATURE_AUTOUPDATE
#ifndef DEDICATED
		if (uivm)
		{
			uiMenuCommand_t currentMenu = (uiMenuCommand_t)(VM_Call(uivm, UI_GET_ACTIVE_MENU));
			if (currentMenu != UIMENU_WM_AUTOUPDATE)
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_AUTOUPDATE);
			}
		}
		else
#endif
		{
			Com_Printf("%s ^1RUN UPDATE COMMAND TO UPDATE\n", com_updatemessage->string);
		}
#endif /* FEATURE_AUTOUPDATE */
	}
}

/**
 * @brief Com_CheckUpdateStarted
 */
void Com_CheckUpdateStarted(void)
{
	// If we have completed a connection to the Auto-Update server...
	if (autoupdate.updateChecked
#ifndef DEDICATED
	    && NET_CompareAdr(autoupdate.autoupdateServer, clc.serverAddress)
#endif
	    )
	{
		// Mark the client as being in the process of getting an update
		if (com_updateavailable->integer)
		{
			autoupdate.updateStarted = qtrue;
			Com_InitDownloads();
		}
	}
}

/**
 * @brief Com_UpdateVarsClean
 * @param[in] flags
 */
void Com_UpdateVarsClean(int flags)
{
	switch (flags)
	{
	case CLEAR_ALL:
		Cvar_Set("com_updatefiles", "");
		Cvar_Set("com_updatemessage", "");
		Cvar_Set("com_updatefiles", "");
		autoupdate.masterDataChecked = 0;
	case CLEAR_FLAGS:
		autoupdate.updateChecked = qfalse;
		autoupdate.forceUpdate   = qfalse;
	default:
		autoupdate.updateStarted = qfalse;
		break;
	}
}

/**
 * @brief Com_Update_f
 */
void Com_Update_f(void)
{
#ifndef DEDICATED
	if (!autoupdate.updateChecked)
	{
		autoupdate.forceUpdate = qtrue;
		Com_CheckAutoUpdate();
	}
	else
	{
		Com_GetAutoUpdate();
	}
#else
	autoupdate.forceUpdate = qtrue;
	Com_CheckAutoUpdate();
#endif
}
