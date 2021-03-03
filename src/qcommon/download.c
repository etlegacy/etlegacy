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
 * @file download.c
 */

#ifdef DEDICATED
#include "../server/server.h"
#define dld svs.download
#define Com_AddReliableCommand(x)
#else
#include "../client/client.h"
#define dld cls.download
#define Com_AddReliableCommand(x) CL_AddReliableCommand(x)
#endif

#if defined(FEATURE_PAKISOLATION) && !defined(DEDICATED)
/**
 * @brief DL_ContainerizePath target destination path to container if needed
 */
const char* DL_ContainerizePath(const char *temp, const char *dest)
{
	char       hash[41] = { 0 };
	const char *pakname = FS_Basename(dest);
	const char *gamedir = FS_Dirpath(dest);

	FS_CalculateFileSHA1(temp, hash);

	// generate container for valid gamedirs only
	if (!Q_stricmp(gamedir, BASEGAME) || !Q_stricmp(gamedir, DEFAULT_MODGAME))
	{
		// whitelist check
		// FIXME: error out if not whitelisted
		if (FS_MatchFileInPak(temp, "*.dll") && !FS_IsWhitelisted(pakname, hash))
		{
			Com_DPrintf(S_COLOR_YELLOW "WARNING: Checksum verification - '%s' is not whitelisted\n", pakname);
		}

		return va("%s%c%s%c%s", gamedir, PATH_SEP, FS_CONTAINER, PATH_SEP, pakname);
	}

	return dest;
}
#endif

/**
 * @brief Com_ClearDownload
 */
void Com_ClearDownload(void)
{
	dld.download           = 0;
	dld.downloadNumber     = 0;
	dld.downloadBlock      = 0;
	dld.downloadCount      = 0;
	dld.downloadSize       = 0;
	dld.downloadFlags      = 0;
	dld.downloadList[0]    = '\0';
	dld.bWWWDl             = qfalse;
	dld.bWWWDlAborting     = qfalse;
	dld.redirectedList[0]  = '\0';
	dld.badChecksumList[0] = '\0';
}

/**
 * @brief Clear download information that we keep in cls (disconnected download support)
 */
void Com_ClearStaticDownload(void)
{
	etl_assert(!dld.bWWWDlDisconnected);    // reset before calling
	dld.noReconnect             = qfalse;
	dld.downloadRestart         = qfalse;
	dld.systemDownload          = qfalse;
	dld.downloadTempName[0]     = '\0';
	dld.downloadName[0]         = '\0';
	dld.originalDownloadName[0] = '\0';
}

/**
* @brief Called when all downloading has been completed
* Initiates update process after an update has been downloaded.
*/
static void Com_DownloadsComplete(void)
{
	// Make sure this is reset to false, what ever has been downloaded
	dld.systemDownload = qfalse;

	if (Com_CheckUpdateDownloads())
	{
		return;
	}

	// if we downloaded files we need to restart the file system
	if (dld.downloadRestart)
	{
		dld.downloadRestart = qfalse;

#ifdef DEDICATED
		FS_Restart(sv.checksumFeed);
#else
		Com_Printf("Client download complete - restarting ...\n");
		FS_Restart(clc.checksumFeed);    // We possibly downloaded a pak, restart the file system to load it

		if (!dld.bWWWDlDisconnected)
		{
			// inform the server so we get new gamestate info
			Com_AddReliableCommand("donedl");
		}
#endif
		// we can reset that now
		dld.bWWWDlDisconnected = qfalse;
		Com_ClearStaticDownload();

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}
#ifndef DEDICATED
	else
	{
		Com_Printf("Client download complete\n");
	}
#endif

	// I wonder if that happens - it should not but I suspect it could happen if a download fails in the middle or is aborted
	etl_assert(!dld.bWWWDlDisconnected);

#ifndef DEDICATED
	CL_DownloadsComplete();
#endif
}

/**
 * @brief Requests a file to download from the server. Stores it in the current
 * game directory.
 *
 * @param[in] localName
 * @param[in] remoteName
 */
void Com_BeginDownload(const char *localName, const char *remoteName)
{
	//Com_DPrintf("***** Com_BeginDownload *****\n"
	//          "Localname: %s\n"
	//          "Remotename: %s\n"
	//          "****************************\n", localName, remoteName);

	Com_Printf("Client downloading: %s\n", remoteName); // localName and remoteName are the same name

	Q_strncpyz(dld.downloadName, localName, sizeof(dld.downloadName));
	Com_sprintf(dld.downloadTempName, sizeof(dld.downloadTempName), "%s.tmp", localName);

	dld.downloadBlock = 0; // Starting new file
	dld.downloadCount = 0;

#ifndef DEDICATED
	// Set so UI gets access to it
	Cvar_Set("cl_downloadName", remoteName);
	Cvar_Set("cl_downloadSize", "0");
	Cvar_Set("cl_downloadCount", "0");
	Cvar_SetValue("cl_downloadTime", cls.realtime);

	Com_AddReliableCommand(va("download %s", remoteName));
#endif
}

static void checkDownloadName(char *filename)
{
	int i;

	for (i = 0; i < strlen(filename); i++)
	{
		if (filename[i] <= 31 || filename[i] >= 127)
		{
			Cvar_Set("com_missingFiles", "");
			Com_Error(ERR_DROP, "Disconnected from server.\n\nServer file name \"%s\" is containing an invalid character for the ET: Legacy file structure.\n\nDownloading file denied.", filename);
		}
	}
}

/**
 * @brief A download completed or failed
 */
void Com_NextDownload(void)
{
	char *s;
	char *remoteName, *localName;

	// We are looking to start a download here
	if (*dld.downloadList)
	{
		s = dld.downloadList;

		// format is:
		//  @remotename@localname@remotename@localname, etc.

		if (*s == '@')
		{
			s++;
		}
		remoteName = s;

		if ((s = strchr(s, '@')) == NULL)
		{
			Com_DownloadsComplete();
			return;
		}

		*s++      = 0;
		localName = s;
		if ((s = strchr(s, '@')) != NULL)
		{
			*s++ = 0;
		}
		else
		{
			s = localName + strlen(localName);    // point at the nul byte

		}

		checkDownloadName(remoteName);

		Com_BeginDownload(localName, remoteName);

		dld.downloadRestart = qtrue;

		// move over the rest
		memmove(dld.downloadList, s, strlen(s) + 1);

		return;
	}

	Com_DownloadsComplete();
}

/**
* @brief After receiving a valid game state, we validate the cgame and
* local zip files here and determine if we need to download them
*/
void Com_InitDownloads(void)
{
	char missingFiles[1024] = { '\0' };

	// init some of the www dl data
	dld.bWWWDl             = qfalse;
	dld.bWWWDlAborting     = qfalse;
	dld.bWWWDlDisconnected = qfalse;
	Com_ClearStaticDownload();

	if (!Com_InitUpdateDownloads())
	{
		// whatever auto download configuration, store missing files in a cvar, use later in the ui maybe
		if (FS_ComparePaks(missingFiles, sizeof(missingFiles), qfalse))
		{
			Cvar_Set("com_missingFiles", missingFiles);
		}
		else
		{
			Cvar_Set("com_missingFiles", "");
		}

		// reset the redirect checksum tracking
		dld.redirectedList[0] = '\0';

#ifdef DEDICATED
		Com_NextDownload();
#else
		if (cl_allowDownload->integer && FS_ComparePaks(dld.downloadList, sizeof(dld.downloadList), qtrue))
		{
			if (*dld.downloadList)
			{
				// if autodownloading is not enabled on the server
				cls.state = CA_CONNECTED;
				Com_NextDownload();
				return;
			}
		}
#endif
	}
	else
	{
		return;
	}

	Com_DownloadsComplete();
}

/**
 * @brief Com_WWWDownload
 */
void Com_WWWDownload(void)
{
	const char *to_ospath;
	dlStatus_t      ret;
	static qboolean bAbort = qfalse;

	if (dld.bWWWDlAborting)
	{
		if (!bAbort)
		{
			Com_DPrintf("Com_WWWDownload: WWWDlAborting\n");
			bAbort = qtrue;
		}
		return;
	}
	if (bAbort)
	{
		Com_DPrintf("Com_WWWDownload: WWWDlAborting done\n");
		bAbort = qfalse;
	}

	ret = DL_DownloadLoop();

	if (ret == DL_CONTINUE)
	{
		return;
	}
	else if (ret == DL_DONE)
	{
		// taken from CL_ParseDownload
		// we work with OS paths

		dld.download = 0;

		if (dld.systemDownload)
		{
			to_ospath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), dld.originalDownloadName, NULL);
		}
		else
		{
#if defined(FEATURE_PAKISOLATION) && !defined(DEDICATED)
			to_ospath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"),
									   DL_ContainerizePath(dld.downloadTempName, dld.originalDownloadName), NULL);
#else
			to_ospath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), dld.originalDownloadName, NULL);
#endif
		}

		//FIXME: This can get hit more than once for a file..
		if (FS_FileInPathExists(dld.downloadTempName))
		{
			if (Sys_Rename(dld.downloadTempName, to_ospath))
			{
				FS_CopyFile(dld.downloadTempName, to_ospath);

				if (Sys_Remove(dld.downloadTempName) != 0)
				{
					Com_Printf("WARNING: Com_WWWDownload - cannot remove file '%s'\n", dld.downloadTempName);
				}
			}
		}
		else
		{
			Com_DPrintf("Downloaded file does not exist (anymore?) '%s'\n", dld.downloadTempName);
		}

		*dld.downloadTempName = *dld.downloadName = 0;
		Cvar_Set("cl_downloadName", "");
		if (dld.bWWWDlDisconnected)
		{
			// for an auto-update in disconnected mode, we'll be spawning the setup in CL_DownloadsComplete
			if (!autoupdate.updateStarted && !dld.noReconnect)
			{
				// reconnect to the server, which might send us to a new disconnected download
				Cbuf_ExecuteText(EXEC_APPEND, "reconnect\n");
			}
		}
		else
		{
			Com_AddReliableCommand("wwwdl done");
			// tracking potential web redirects leading us to wrong checksum - only works in connected mode
			if (strlen(dld.redirectedList) + strlen(dld.originalDownloadName) + 1 >= sizeof(dld.redirectedList))
			{
				// just to be safe
				Com_Printf("ERROR: redirectedList overflow (%s)\n", dld.redirectedList);
			}
			else
			{
				strcat(dld.redirectedList, "@");
				strcat(dld.redirectedList, dld.originalDownloadName);
			}
		}
	}
	else
	{
		if (dld.bWWWDlDisconnected)
		{
			// in a connected download, we'd tell the server about failure and wait for a reply
			// but in this case we can't get anything from server
			// if we just reconnect it's likely we'll get the same disconnected download message, and error out again
			// this may happen for a regular dl or an auto update
			const char *error = va("Download failure while getting '%s'\n", dld.downloadName);    // get the msg before clearing structs

			dld.bWWWDlDisconnected = qfalse; // need clearing structs before ERR_DROP, or it goes into endless reload
			Com_ClearStaticDownload();
			Com_Error(ERR_DROP, "%s", error);
		}
		else
		{
			// see CL_ParseDownload, same abort strategy
			Com_Printf("Download failure while getting '%s'\n", dld.downloadName);
			Com_AddReliableCommand("wwwdl fail");
			dld.bWWWDlAborting = qtrue;
		}
		return;
	}

	dld.bWWWDl = qfalse;
	Com_NextDownload();
}

/**
 * @brief FS code calls this when doing FS_ComparePaks
 * we can detect files that we got from a www dl redirect with a wrong checksum
 * this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
 *
 * @param[in] pakname
 *
 * @return
 */
qboolean Com_WWWBadChecksum(const char *pakname)
{
	if (strstr(dld.redirectedList, va("@%s", pakname)))
	{
		Com_Printf("WARNING: file %s obtained through download redirect has wrong checksum\n", pakname);
		Com_Printf("         this likely means the server configuration is broken\n");
		if (strlen(dld.badChecksumList) + strlen(pakname) + 1 >= sizeof(dld.badChecksumList))
		{
			Com_Printf("ERROR: badChecksumList overflowed (%s)\n", dld.badChecksumList);
			return qfalse;
		}
		strcat(dld.badChecksumList, "@");
		strcat(dld.badChecksumList, pakname);
		Com_DPrintf("bad checksums: %s\n", dld.badChecksumList);
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief Com_SetupDownloadRaw
 * @param[in] remote
 * @param[in] filename
 */
static void Com_SetupDownloadRaw(const char *remote, const char *path, const char *filename, const char *tempName, qboolean systemDownload)
{
	dld.bWWWDl             = qtrue;
	dld.bWWWDlDisconnected = qtrue;
	dld.systemDownload     = systemDownload;

	// download format: @remotename@localname
	Q_strncpyz(dld.downloadList, va("@%s@%s", filename, filename), MAX_INFO_STRING);
	Q_strncpyz(dld.originalDownloadName, path[0] ? va("%s/%s", path, filename) : filename, sizeof(dld.originalDownloadName));
	Q_strncpyz(dld.downloadName, va("%s/%s", remote, filename), sizeof(dld.downloadName));
	Q_strncpyz(dld.downloadTempName, tempName, sizeof(dld.downloadTempName));

	if (!DL_BeginDownload(dld.downloadTempName, dld.downloadName))
	{
		dld.bWWWDlAborting = qtrue;
		Com_Error(ERR_DROP, "Could not download file: \"%s\"", dld.downloadName);
	}
}

/**
 * @brief Com_SetupDownload
 * @param[in] remote
 * @param[in] filename
 */
static void Com_SetupDownload(const char *remote, const char *filename)
{
	dld.bWWWDl             = qtrue;
	dld.bWWWDlDisconnected = qtrue;

	// download format: @remotename@localname
	Q_strncpyz(dld.downloadList, va("@%s@%s", filename, filename), MAX_INFO_STRING);
	Q_strncpyz(dld.originalDownloadName, va("%s/%s", Cvar_VariableString("fs_game"), filename), sizeof(dld.originalDownloadName));
	Q_strncpyz(dld.downloadName, va("%s/%s", remote, filename), sizeof(dld.downloadName));
	Q_strncpyz(dld.downloadTempName, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), Cvar_VariableString("fs_game"), va("%s" TMP_FILE_EXTENSION, filename)), sizeof(dld.downloadTempName));

	if (!DL_BeginDownload(dld.downloadTempName, dld.downloadName))
	{
		dld.bWWWDlAborting = qtrue;
		Com_Error(ERR_DROP, "Could not download file: \"%s\"", dld.downloadName);
	}
}

/**
 * @brief Com_Download_f
 */
void Com_Download_f(void)
{
#ifndef DEDICATED
	if (cls.state >= CA_LOADING)
	{
		Com_Printf("download: client already downloading or in game!");
		return;
	}
#endif

	dld.noReconnect = qtrue;

	if (Cmd_Argc() > 1)
	{
		char *name;

		name = Cmd_Argv(1);

		// Check if the fileName ends with the pk3 extension
		if (!COM_CompareExtension(name, ".pk3"))
		{
			Com_Printf("download: command is for pk3 files only!");
		}
		else
		{
			Com_SetupDownload(com_downloadURL->string, name);
		}
	}
	else
	{
		Com_Printf("Usage: download <filename1.pk3> <filename2.pk3> <filename3.pk3> ...");
	}
}

#if defined(FEATURE_SSL)
/**
 * @brief This method handles the downloading and updating of the ca-certificates file in <homepath>/cacert.pem
 */
void Com_CheckCaCertStatus(void)
{
	const char *ospath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), CA_CERT_FILE, NULL);
	qboolean downloadFile = qfalse;

	if (FS_SV_FileExists(CA_CERT_FILE, qfalse))
	{
		long age = FS_FileAge(ospath);

		if(age <= 0)
		{
			Com_DPrintf("CA file returned an age of: %li", age);
			return;
		}

		// Check if the file is older than 2 weeks
		if (age > (60 * 60 * 24 * 14))
		{
			downloadFile = qtrue;
		}
	}
	else
	{
		downloadFile = qtrue;
	}

	if (downloadFile)
	{
		Com_SetupDownloadRaw(MIRROR_SERVER_URL "/certificates", "", CA_CERT_FILE, CA_CERT_FILE TMP_FILE_EXTENSION, qtrue);
	}
}
#endif
