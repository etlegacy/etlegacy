/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 * @file cl_update.c
 */

#include "client.h"

#ifdef _WIN32
#define UPDATE_BINARY "updater.exe"
#define UPDATE_CMD "\"%s\" --install-dir \"%s\" --package-dir \"%s\" --script \"%s\" --wait \"%s\" --auto-close --execute \"etl.exe\" --execute-args \"%s\""
#else
#define UPDATE_BINARY "updater"
#define UPDATE_CMD "'%s' --install-dir '%s' --package-dir '%s' --script '%s' --wait '%s' --auto-close --execute 'etl' --execute-args '%s'"
#endif
#define UPDATE_PACKAGE "updater.zip"
#define UPDATE_CONFIG "updater.xml"
#define MIN_PACK_LEN 4

//#define UPDATE_SERVER_DLFILE UPDATE_SERVER_NAME "?sys=" CPUSTRING "&ver=" ETLEGACY_VERSION_SHORT "&file=%s"

autoupdate_t autoupdate;

void CL_CheckAutoUpdate(void)
{
	char info[MAX_INFO_STRING];

#ifndef FEATURE_AUTOUPDATE
	return;
#endif

	if (!com_autoupdate->integer)
	{
		Com_DPrintf("Updater is disabled by com_autoupdate 0.\n");
		return;
	}

	// Only check once per session
	if (autoupdate.updateChecked)
	{
		return;
	}

	// Resolve update server
	Com_Printf("Updater: resolving %s... ", UPDATE_SERVER_NAME);

	if (!NET_StringToAdr(va("%s:%i", UPDATE_SERVER_NAME, PORT_UPDATE), &autoupdate.autoupdateServer, NA_UNSPEC))
	{
		Com_Printf("couldn't resolve address\n");

		autoupdate.updateChecked = qtrue;
		return;
	}
	else
	{
		Com_Printf("resolved to %s\n", NET_AdrToString(autoupdate.autoupdateServer));
	}

	info[0] = 0;
	Info_SetValueForKey(info, "version", ETLEGACY_VERSION_SHORT);
	Info_SetValueForKey(info, "platform", CPUSTRING);
	Info_SetValueForKey(info, va("etl_bin_%s.pk3", ETLEGACY_VERSION_SHORT),
	                    Com_MD5File(va("legacy/etl_bin_%s.pk3", ETLEGACY_VERSION_SHORT), 0, NULL, 0));
	Info_SetValueForKey(info, va("pak3_%s.pk3", ETLEGACY_VERSION_SHORT),
	                    Com_MD5File(va("legacy/pak3_%s.pk3", ETLEGACY_VERSION_SHORT), 0, NULL, 0));

	NET_OutOfBandPrint(NS_CLIENT, autoupdate.autoupdateServer, "getUpdateInfo \"%s\"", info);

	autoupdate.updateChecked = qtrue;
}

void CL_GetAutoUpdate(void)
{
#ifdef FEATURE_AUTOUPDATE
	// Don't try and get an update if we haven't checked for one
	if (!autoupdate.updateChecked)
	{
		return;
	}

	// Make sure there's a valid update file to request
	if (strlen(com_updatefiles->string) < 5)
	{
		return;
	}

	Com_DPrintf("Connecting to auto-update server...\n");

	S_StopAllSounds();

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// toggle on all the download related cvars
	Cvar_Set("cl_allowDownload", "1");  // general flag
	Cvar_Set("cl_wwwDownload", "1");    // ftp/http support

	// clear any previous "server full" type messages
	clc.serverMessage[0] = 0;

	if (com_sv_running->integer)
	{
		// if running a local server, kill it
		SV_Shutdown("Server quit\n");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SV_Frame(0);

	CL_Disconnect(qtrue);
	Con_Close();

	Q_strncpyz(cls.servername, "ET:L Update Server", sizeof(cls.servername));

	if (autoupdate.autoupdateServer.type == NA_BAD)
	{
		Com_Printf("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		Cvar_Set("ui_connecting", "0");
		return;
	}

	// Copy auto-update server address to Server connect address
	memcpy(&clc.serverAddress, &autoupdate.autoupdateServer, sizeof(netadr_t));

	Com_DPrintf("%s resolved to %s\n", cls.servername,
	            NET_AdrToString(clc.serverAddress));

	cls.state = CA_DISCONNECTED;
	Cvar_Set("ui_connecting", "1");
	Cvar_Set("ui_dl_running", "1");

	cls.keyCatchers        = 0;
	clc.connectTime        = -99999; // CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set("cl_currentServerAddress", "ET:L Update Server");

	CL_CheckUpdateStarted();
#endif /* FEATURE_AUTOUPDATE */
}

static void CL_RunUpdateBinary(const char *updateBinary, const char *updateConfig)
{
	static char fn[MAX_OSPATH];

	Q_strncpyz(fn, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, updateBinary), MAX_OSPATH);

#ifndef _WIN32
	Sys_Chmod(fn, S_IXUSR);
#endif

	// will either exit with a successful process spawn, or will Com_Error ERR_DROP
	// so we need to clear the disconnected download data if needed
	if (cls.bWWWDlDisconnected)
	{
		cls.bWWWDlDisconnected = qfalse;
		CL_ClearStaticDownload();
	}

	Sys_StartProcess(va(UPDATE_CMD, fn, Cvar_VariableString("fs_basepath"), FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, NULL), FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, updateConfig), Cvar_VariableString("com_pid"), Com_GetCommandLine()), qtrue);

	// reinitialize the filesystem if the game directory or checksum has changed
	// - after Legacy mod update
	FS_ConditionalRestart(clc.checksumFeed);
}

static qboolean CL_UnpackUpdatePackage(const char *pack, const char *bin, const char *config)
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

qboolean CL_CheckUpdateDownloads(void)
{
#ifdef FEATURE_AUTOUPDATE
	// Auto-update
	if (autoupdate.updateStarted)
	{
		if (strlen(com_updatefiles->string) > MIN_PACK_LEN)
		{
			CL_InitDownloads();
			return qtrue;
		}

		if (CL_UnpackUpdatePackage(UPDATE_PACKAGE, UPDATE_BINARY, UPDATE_CONFIG))
		{
			CL_RunUpdateBinary(UPDATE_BINARY, UPDATE_CONFIG);
		}
		else
		{
			Cvar_Set("ui_connecting", "0");
			Cvar_Set("ui_dl_running", "0");
		}

		autoupdate.updateStarted = qfalse;

		CL_Disconnect(qtrue);

		// we can reset that now
		cls.bWWWDlDisconnected = qfalse;
		CL_ClearStaticDownload();

		return qtrue;
	}
#endif // FEATURE_AUTOUPDATE
	return qfalse;
}

qboolean CL_InitUpdateDownloads(void)
{
#ifdef FEATURE_AUTOUPDATE
	if (autoupdate.updateStarted && NET_CompareAdr(autoupdate.autoupdateServer, clc.serverAddress))
	{
		if (strlen(com_updatefiles->string) > MIN_PACK_LEN)
		{
			char *updateFile;
			char updateFilesRemaining[MAX_TOKEN_CHARS] = "";

			clc.bWWWDl             = qtrue;
			cls.bWWWDlDisconnected = qtrue;

			updateFile = strtok(com_updatefiles->string, ";");

			if (updateFile == NULL)
			{
				Com_Error(ERR_AUTOUPDATE, "Could not parse update string.");
			}
			else
			{
				// download format: @remotename@localname
				Q_strncpyz(clc.downloadList, va("@%s@%s", updateFile, updateFile), MAX_INFO_STRING);
				Q_strncpyz(cls.originalDownloadName, va("%s/%s", AUTOUPDATE_DIR, updateFile), sizeof(cls.originalDownloadName));

				if (!Q_stricmp(updateFile, UPDATE_PACKAGE))
				{
					Q_strncpyz(cls.downloadName, va("%s/updater/%s-%s-%s", UPDATE_SERVER_NAME, ETLEGACY_VERSION_SHORT, CPUSTRING, updateFile), sizeof(cls.downloadName));
				}
				else
				{
					Q_strncpyz(cls.downloadName, va("%s/packages/%s", UPDATE_SERVER_NAME, updateFile), sizeof(cls.downloadName));
				}

				Q_strncpyz(cls.downloadTempName, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, va("%s.tmp", updateFile)), sizeof(cls.downloadTempName));
				// TODO: add file size, so UI can show progress bar
				//Cvar_SetValue("cl_downloadSize", clc.downloadSize);

				if (!DL_BeginDownload(cls.downloadTempName, cls.downloadName))
				{
					Com_Error(ERR_AUTOUPDATE, "Could not download an update file: \"%s\"", cls.downloadName);
					clc.bWWWDlAborting = qtrue;
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

qboolean CL_UpdatePacketEvent(netadr_t from)
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

/*
===================
CL_UpdateInfoPacket
===================
*/
void CL_UpdateInfoPacket(netadr_t from)
{
	if (autoupdate.autoupdateServer.type == NA_BAD)
	{
		Com_DPrintf("CL_UpdateInfoPacket: Update server has bad address\n");
		return;
	}

	Com_DPrintf("Update server resolved to %s\n",
	            NET_AdrToString(autoupdate.autoupdateServer));

	if (!NET_CompareAdr(from, autoupdate.autoupdateServer))
	{
		// TODO: when the updater is server-side as well, write this message to the Attack log
		Com_DPrintf("CL_UpdateInfoPacket: Ignoring packet from %s, because the update server is located at %s\n",
		            NET_AdrToString(from), NET_AdrToString(autoupdate.autoupdateServer));
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
				CL_GetAutoUpdate();
				autoupdate.forceUpdate = qfalse;
				return;
			}
		}

#ifdef FEATURE_AUTOUPDATE
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_AUTOUPDATE);
#endif /* FEATURE_AUTOUPDATE */
	}
}

void CL_CheckUpdateStarted(void)
{
	// If we have completed a connection to the Auto-Update server...
	if (autoupdate.updateChecked && NET_CompareAdr(autoupdate.autoupdateServer, clc.serverAddress))
	{
		// Mark the client as being in the process of getting an update
		if (com_updateavailable->integer)
		{
			autoupdate.updateStarted = qtrue;
			CL_InitDownloads();
		}
	}
}

void CL_UpdateVarsClean(int flags)
{
	switch (flags)
	{
	case CLEAR_ALL:
		Cvar_Set("com_updatefiles", "");
		Cvar_Set("com_updatemessage", "");
		Cvar_Set("com_updatefiles", "");
	case CLEAR_FLAGS:
		autoupdate.updateChecked = qfalse;
		autoupdate.forceUpdate   = qfalse;
	default:
		autoupdate.updateStarted = qfalse;
		break;
	}
}

void CL_RunUpdate(void)
{
	if (!autoupdate.updateChecked)
	{
		autoupdate.forceUpdate = qtrue;
		CL_CheckAutoUpdate();
	}
	else
	{
		CL_GetAutoUpdate();
	}
}
