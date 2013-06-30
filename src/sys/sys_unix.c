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
 *
 * @file sys_platform.c
 * @brief Contains unix-specific code for console..
 */

#ifndef DEDICATED
#    ifdef BUNDLED_SDL
#        include "SDL_video.h"
#    else
#        include <SDL/SDL_video.h>
#    endif
#endif

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#ifndef __AROS__
#include <sys/mman.h>
#endif
#include <sys/time.h>
#include <pwd.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/wait.h>

#ifdef __AROS__
#ifndef SIGIOT
#define SIGIOT SIGTERM
#endif
#include <datatypes/textclass.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <proto/openurl.h>

struct Library *OpenURLBase;
#endif

#ifndef __AROS__
qboolean stdinIsATTY;
#endif

// Used to determine where to store user-specific files
static char homePath[MAX_OSPATH] = { 0 };

/**
 * @brief Get current home path
 * @return path to ET: Legacy directory
 */
char *Sys_DefaultHomePath(void)
{
#ifndef __AROS__
	char *p;

	if (!*homePath)
	{
		if ((p = getenv("HOME")) != NULL)
		{
			Q_strncpyz(homePath, p, sizeof(homePath));
#ifdef __APPLE__
			Q_strcat(homePath, sizeof(homePath), "/Library/Application Support/etlegacy");
#else
			Q_strcat(homePath, sizeof(homePath), "/.etlwolf");
#endif
		}
	}
#endif

	return homePath;
}

/**
 * @brief Chmod a file
 * @param[in] file Filename
 * @param     mode Access Mode
 */
void Sys_Chmod(char *file, int mode)
{
	struct stat s_buf;
	int         perm;

	if (stat(file, &s_buf) != 0)
	{
		Com_Printf("stat('%s')  failed: errno %d\n", file, errno);
		return;
	}
	perm = s_buf.st_mode | mode;
	if (chmod(file, perm) != 0)
	{
		Com_Printf("chmod('%s', %d) failed: errno %d\n", file, perm, errno);
	}
	Com_DPrintf("chmod +%d '%s'\n", mode, file);
}

/**
 * @brief Base time in seconds
 *
 * That's our origin timeval:tv_sec is an int:\n
 * assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038\n
 * using unsigned long data type to work right with Sys_XTimeToSysTime
 */
unsigned long sys_timeBase = 0;

/**
 * @brief Current time in ms, using sys_timeBase as origin
 * @note sys_timeBase * 1000 + curtime -> ms since the Epoch, 0x7fffffff ms - ~24 days
 *
 * although timeval:tv_usec is an int, I'm not sure wether it is actually used as an unsigned int
 *   (which would affect the wrap period)
 */
int curtime;

int Sys_Milliseconds(void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);

	if (!sys_timeBase)
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec / 1000;
	}

	curtime = (tp.tv_sec - sys_timeBase) * 1000 + tp.tv_usec / 1000;

	return curtime;
}

/**
 * @param[in,out] v Vector
 */
void Sys_SnapVector(float *v)
{
	v[0] = rint(v[0]);
	v[1] = rint(v[1]);
	v[2] = rint(v[2]);
}

/**
 * @param[out] string Place to put random string
 * @param      len    How much random data we need?
 * @retval qfalse on failure
 * @retval qtrue  on success
 */
qboolean Sys_RandomBytes(byte *string, int len)
{
#ifndef __AROS__
	FILE *fp;

	fp = fopen("/dev/urandom", "r");
	if (!fp)
	{
		return qfalse;
	}

	if (!fread(string, sizeof(byte), len, fp))
	{
		fclose(fp);
		return qfalse;
	}

	fclose(fp);
	return qtrue;
#else
	return qfalse;
#endif
}

/**
 * @brief Get current user username
 * @return username
 */
char *Sys_GetCurrentUser(void)
{
#ifndef __AROS__
	struct passwd *p;

	if ((p = getpwuid(getuid())) != NULL)
	{
		return p->pw_name;
	}
#endif

	return "player";
}

char *Sys_GetClipboardData(void)
{
#ifdef __AROS__
	struct IFFHandle   *IFFHandle;
	struct ContextNode *cn;
	char               *data = NULL;

	if ((IFFHandle = AllocIFF()))
	{
		if ((IFFHandle->iff_Stream = (IPTR) OpenClipboard(0)))
		{
			InitIFFasClip(IFFHandle);
			if (!OpenIFF(IFFHandle, IFFF_READ))
			{
				if (!StopChunk(IFFHandle, ID_FTXT, ID_CHRS))
				{
					if (!ParseIFF(IFFHandle, IFFPARSE_SCAN))
					{
						cn = CurrentChunk(IFFHandle);
						if (cn && (cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS) && (cn->cn_Size > 0))
						{
							data = (char *) Z_Malloc(cn->cn_Size + 1);
							if (ReadChunkBytes(IFFHandle, data, cn->cn_Size))
							{
								data[cn->cn_Size] = '\0';
							}
							else
							{
								data[0] = '\0';
							}
						}
					}
				}
				CloseIFF(IFFHandle);
			}
			CloseClipboard((struct ClipboardHandle *) IFFHandle->iff_Stream);
		}
		FreeIFF(IFFHandle);
	}

	return data;
#else
	return NULL;
#endif
}

#define MEM_THRESHOLD 96 * 1024 * 1024

/**
 * @todo
 */
qboolean Sys_LowPhysicalMemory(void)
{
	return qfalse;
}

const char *Sys_Basename(char *path)
{
	return basename(path);
}

const char *Sys_Dirname(char *path)
{
	return dirname(path);
}

FILE *Sys_FOpen(const char *ospath, const char *mode)
{
	struct stat buf;

	// check if path exists and is a directory
	if (!stat(ospath, &buf) && S_ISDIR(buf.st_mode))
	{
		return NULL;
	}

	return fopen(ospath, mode);
}

/**
 * @brief Create directory
 * @param[in] path Path
 * @retval qfalse on failure
 * @retval qtrue  on success
 */
qboolean Sys_Mkdir(const char *path)
{
	int result = mkdir(path, 0750);

	if (result != 0)
	{
		return errno == EEXIST;
	}

	return qtrue;
}

/**
 * @brief Get current working directory
 * @return Path to current working directory
 */
char *Sys_Cwd(void)
{
	static char cwd[MAX_OSPATH];

	char *result = getcwd(cwd, sizeof(cwd) - 1);
	if (result != cwd)
	{
		return NULL;
	}

	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define MAX_FOUND_FILES 0x1000

void Sys_ListFilteredFiles(const char *basedir, char *subdirs, char *filter, char **list, int *numfiles)
{
	char          search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char          filename[MAX_OSPATH];
	DIR           *fdir;
	struct dirent *d;
	struct stat   st;

	if (*numfiles >= MAX_FOUND_FILES - 1)
	{
		return;
	}

	if (strlen(subdirs))
	{
		Com_sprintf(search, sizeof(search), "%s/%s", basedir, subdirs);
	}
	else
	{
		Com_sprintf(search, sizeof(search), "%s", basedir);
	}

	if ((fdir = opendir(search)) == NULL)
	{
		return;
	}

	while ((d = readdir(fdir)) != NULL)
	{
		Com_sprintf(filename, sizeof(filename), "%s/%s", search, d->d_name);
		if (stat(filename, &st) == -1)
		{
			continue;
		}

		if (st.st_mode & S_IFDIR)
		{
			if (Q_stricmp(d->d_name, ".") && Q_stricmp(d->d_name, ".."))
			{
				if (strlen(subdirs))
				{
					Com_sprintf(newsubdirs, sizeof(newsubdirs), "%s/%s", subdirs, d->d_name);
				}
				else
				{
					Com_sprintf(newsubdirs, sizeof(newsubdirs), "%s", d->d_name);
				}
				Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
			}
		}
		if (*numfiles >= MAX_FOUND_FILES - 1)
		{
			break;
		}
		Com_sprintf(filename, sizeof(filename), "%s/%s", subdirs, d->d_name);
		if (!Com_FilterPath(filter, filename, qfalse))
		{
			continue;
		}
		list[*numfiles] = CopyString(filename);
		(*numfiles)++;
	}

	closedir(fdir);
}

/**
 * @brief List files in directory by filter
 * @param[in]  directory What we want to list?
 * @param[in]  extension Limit files to specified extension
 * @param[in]  filter    Filter results
 * @param[out] numfiles  Number of listed files
 * @param      wantsubs  Do we want to list subdirectories too?
 * @return Array of strings with files
 */
char **Sys_ListFiles(const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs)
{
	struct dirent *d;
	DIR           *fdir;
	qboolean      dironly = wantsubs;
	char          search[MAX_OSPATH];
	int           nfiles;
	char          **listCopy;
	char          *list[MAX_FOUND_FILES];
	int           i;
	struct stat   st;
	int           extLen;

	if (filter)
	{

		nfiles = 0;
		Sys_ListFilteredFiles(directory, "", filter, list, &nfiles);

		list[nfiles] = NULL;
		*numfiles    = nfiles;

		if (!nfiles)
		{
			return NULL;
		}

		listCopy = Z_Malloc((nfiles + 1) * sizeof(*listCopy));
		for (i = 0 ; i < nfiles ; i++)
		{
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if (!extension)
	{
		extension = "";
	}

	if (extension[0] == '/' && extension[1] == 0)
	{
		extension = "";
		dironly   = qtrue;
	}

	extLen = strlen(extension);

	// search
	nfiles = 0;

	if ((fdir = opendir(directory)) == NULL)
	{
		*numfiles = 0;
		return NULL;
	}

	while ((d = readdir(fdir)) != NULL)
	{
		Com_sprintf(search, sizeof(search), "%s/%s", directory, d->d_name);
		if (stat(search, &st) == -1)
		{
			continue;
		}
		if ((dironly && !(st.st_mode & S_IFDIR)) ||
		    (!dironly && (st.st_mode & S_IFDIR)))
		{
			continue;
		}

		if (*extension)
		{
			if (strlen(d->d_name) < extLen ||
			    Q_stricmp(
			        d->d_name + strlen(d->d_name) - extLen,
			        extension))
			{
				continue; // didn't match
			}
		}

		if (nfiles == MAX_FOUND_FILES - 1)
		{
			break;
		}
		list[nfiles] = CopyString(d->d_name);
		nfiles++;
	}

	list[nfiles] = NULL;

	closedir(fdir);

	// return a copy of the list
	*numfiles = nfiles;

	if (!nfiles)
	{
		return NULL;
	}

	listCopy = Z_Malloc((nfiles + 1) * sizeof(*listCopy));
	for (i = 0 ; i < nfiles ; i++)
	{
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

void Sys_FreeFileList(char **list)
{
	int i;

	if (!list)
	{
		return;
	}

	for (i = 0 ; list[i] ; i++)
	{
		Z_Free(list[i]);
	}

	Z_Free(list);
}

/**
 * @brief Block execution for msec or until input is recieved.
 * @param msec How long we want to block execution?
 */
void Sys_Sleep(int msec)
{
	if (msec == 0)
	{
		return;
	}

#ifndef __AROS__
	if (stdinIsATTY)
	{
		fd_set fdset;

		FD_ZERO(&fdset);
		FD_SET(STDIN_FILENO, &fdset);
		if (msec < 0)
		{
			select(STDIN_FILENO + 1, &fdset, NULL, NULL, NULL);
		}
		else
		{
			struct timeval timeout;

			timeout.tv_sec  = msec / 1000;
			timeout.tv_usec = (msec % 1000) * 1000;
			select(STDIN_FILENO + 1, &fdset, NULL, NULL, &timeout);
		}
	}
	else
#endif
	{
		// With nothing to select() on, we can't wait indefinitely
		if (msec < 0)
		{
			msec = 10;
		}

		usleep(msec * 1000);
	}
}

/**
 * @brief Display an error message
 * @param[in] error Error String
 */
void Sys_ErrorDialog(const char *error)
{
	char         buffer[1024];
	unsigned int size;
	int          f         = -1;
	const char   *homepath = Cvar_VariableString("fs_homepath");
	const char   *gamedir  = Cvar_VariableString("fs_gamedir");
	const char   *fileName = "crashlog.txt";
	char         *ospath   = FS_BuildOSPath(homepath, gamedir, fileName);

	Sys_Print(va("%s\n", error));

#ifndef DEDICATED
	Sys_Dialog(DT_ERROR, va("%s\nSee \"%s\" for details.\n", error, ospath), "Error");
#endif

	// Make sure the write path for the crashlog exists...
	if (FS_CreatePath(ospath))
	{
		Com_Printf("ERROR: couldn't create path '%s' for crash log.\n", ospath);
		return;
	}

	// We might be crashing because we maxed out the Quake MAX_FILE_HANDLES,
	// which will come through here, so we don't want to recurse forever by
	// calling FS_FOpenFileWrite()...use the Unix system APIs instead.
	f = open(ospath, O_CREAT | O_TRUNC | O_WRONLY, 0640);
	if (f == -1)
	{
		Com_Printf("ERROR: couldn't open %s\n", fileName);
		return;
	}

	// We're crashing, so we don't care much if write() or close() fails.
	while ((size = CON_LogRead(buffer, sizeof(buffer))) > 0)
	{
		if (write(f, buffer, size) != size)
		{
			Com_Printf("ERROR: couldn't fully write to %s\n", fileName);
			break;
		}
	}

	close(f);
}

#ifdef __AROS__
/**
 * @brief Display an AROS dialog box
 * @param     type    Dialog Type
 * @param[in] message Message to show
 * @param[in] title   Message box title
 */
dialogResult_t Sys_Dialog(dialogType_t type, const char *message, const char *title)
{
	struct EasyStruct es;
	int               result;

	es.es_StructSize = sizeof(es);
	es.es_Flags      = 0;
	es.es_Title      = title;
	es.es_TextFormat = message;

	switch (type)
	{
	case DT_YES_NO:
		es.es_GadgetFormat = "Yes|No";
		break;
	case DT_OK_CANCEL:
		es.es_GadgetFormat = "OK|Cancel";
		break;
	default:
		es.es_GadgetFormat = "OK";
		break;
	}

	result = EasyRequest(0, &es, 0, 0);

	// the rightmost button is always 0, others are numbered left to right
	switch (type)
	{
	case DT_YES_NO:
		return result ? DR_YES : DR_NO;
	case DT_OK_CANCEL:
		return result ? DR_OK : DR_CANCEL;
	}

	return DR_OK;
}
#elif !defined (__APPLE__)
static char execBuffer[1024];
static char *execBufferPointer;
static char *execArgv[16];
static int  execArgc;

static void Sys_ClearExecBuffer(void)
{
	execBufferPointer = execBuffer;
	Com_Memset(execArgv, 0, sizeof(execArgv));
	execArgc = 0;
}

static void Sys_AppendToExecBuffer(const char *text)
{
	size_t size   = sizeof(execBuffer) - (execBufferPointer - execBuffer);
	int    length = strlen(text) + 1;

	if (length > size || execArgc >= ARRAY_LEN(execArgv))
	{
		return;
	}

	Q_strncpyz(execBufferPointer, text, size);
	execArgv[execArgc++] = execBufferPointer;

	execBufferPointer += length;
}

static int Sys_Exec(void)
{
	pid_t pid = fork();

	if (pid < 0)
	{
		return -1;
	}

	if (pid)
	{
		// Parent
		int exitCode;

		wait(&exitCode);

		return WEXITSTATUS(exitCode);
	}
	else
	{
		// Child
		execvp(execArgv[0], execArgv);

		// Failed to execute
		exit(-1);

		return -1;
	}
}

static void Sys_ZenityCommand(dialogType_t type, const char *message, const char *title)
{
	Sys_ClearExecBuffer();
	Sys_AppendToExecBuffer("zenity");

	switch (type)
	{
	default:
	case DT_INFO:
		Sys_AppendToExecBuffer("--info");
		break;
	case DT_WARNING:
		Sys_AppendToExecBuffer("--warning");
		break;
	case DT_ERROR:
		Sys_AppendToExecBuffer("--error");
		break;
	case DT_YES_NO:
		Sys_AppendToExecBuffer("--question");
		Sys_AppendToExecBuffer("--ok-label=Yes");
		Sys_AppendToExecBuffer("--cancel-label=No");
		break;

	case DT_OK_CANCEL:
		Sys_AppendToExecBuffer("--question");
		Sys_AppendToExecBuffer("--ok-label=OK");
		Sys_AppendToExecBuffer("--cancel-label=Cancel");
		break;
	}

	Sys_AppendToExecBuffer(va("--text=%s", message));
	Sys_AppendToExecBuffer(va("--title=%s", title));
}

static void Sys_KdialogCommand(dialogType_t type, const char *message, const char *title)
{
	Sys_ClearExecBuffer();
	Sys_AppendToExecBuffer("kdialog");

	switch (type)
	{
	default:
	case DT_INFO:
		Sys_AppendToExecBuffer("--msgbox");
		break;
	case DT_WARNING:
		Sys_AppendToExecBuffer("--sorry");
		break;
	case DT_ERROR:
		Sys_AppendToExecBuffer("--error");
		break;
	case DT_YES_NO:
		Sys_AppendToExecBuffer("--warningyesno");
		break;
	case DT_OK_CANCEL:
		Sys_AppendToExecBuffer("--warningcontinuecancel");
		break;
	}

	Sys_AppendToExecBuffer(message);
	Sys_AppendToExecBuffer(va("--title=%s", title));
}

static void Sys_XmessageCommand(dialogType_t type, const char *message, const char *title)
{
	Sys_ClearExecBuffer();
	Sys_AppendToExecBuffer("xmessage");
	Sys_AppendToExecBuffer("-buttons");

	switch (type)
	{
	default:           Sys_AppendToExecBuffer("OK:0"); break;
	case DT_YES_NO:    Sys_AppendToExecBuffer("Yes:0,No:1"); break;
	case DT_OK_CANCEL: Sys_AppendToExecBuffer("OK:0,Cancel:1"); break;
	}

	Sys_AppendToExecBuffer("-center");
	Sys_AppendToExecBuffer(message);
}

/**
 * @brief Display a *nix dialog box
 * @param     type    Dialog Type
 * @param[in] message Message to show
 * @param[in] title   Message box title
 */
dialogResult_t Sys_Dialog(dialogType_t type, const char *message, const char *title)
{
	typedef enum
	{
		NONE   = 0,
		ZENITY = 1,
		KDIALOG,
		XMESSAGE,
		NUM_DIALOG_PROGRAMS
	} dialogCommandType_t;
	typedef int (*dialogCommandBuilder_t)(dialogType_t, const char *, const char *);

	const char          *session = getenv("DESKTOP_SESSION");
	int                 i, exitCode;
	qboolean            tried[NUM_DIALOG_PROGRAMS] = { qfalse };
	dialogCommandType_t preferredCommandType       = NONE;

	// This may not be the best way
	if (!Q_stricmp(session, "gnome"))
	{
		preferredCommandType = ZENITY;
	}
	else if (!Q_stricmp(session, "kde") || !Q_stricmp(session, "kde-4"))
	{
		preferredCommandType = KDIALOG;
	}

	while (1)
	{
		for (i = ZENITY; i < NUM_DIALOG_PROGRAMS; i++)
		{
			if (preferredCommandType != NONE && preferredCommandType != i)
			{
				continue;
			}

			if (!tried[i])
			{
				switch (i)
				{
				case ZENITY:
					Sys_ZenityCommand(type, message, title);
					break;
				case KDIALOG:
					Sys_KdialogCommand(type, message, title);
					break;
				case XMESSAGE:
					Sys_XmessageCommand(type, message, title);
					break;
				}

				exitCode = Sys_Exec();

				if (exitCode >= 0)
				{
					switch (type)
					{
					case DT_YES_NO:
						return exitCode ? DR_NO : DR_YES;
					case DT_OK_CANCEL:
						return exitCode ? DR_CANCEL : DR_OK;
					default:
						return DR_OK;
					}
				}

				tried[i] = qtrue;

				// The preference failed, so start again in order
				if (preferredCommandType != NONE)
				{
					preferredCommandType = NONE;
					break;
				}
			}
		}

		// What is this for ??? ... looks like relic
		for (i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++)
		{
			if (!tried[i])
			{
				continue;
			}
		}

		break;
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: failed to show a system dialog\n");
	return DR_OK;
}
#endif

/**
 * @brief Actually forks and starts a process
 * @param[in] cmdline Execution string
 *
 * @note
 * UGLY HACK:\n
 * Sys_StartProcess works with a command line only\n
 * If this command line is actually a binary with command line parameters,
 * the only way to do this on unix is to use a system() call
 * but system doesn't replace the current process, which leads to a situation like:\n
 * wolf.x86--spawned_process.x86\n
 * in the case of auto-update for instance, this will cause write access denied on wolf.x86:\n
 * wolf-beta/2002-March/000079.html\n
 * we hack the implementation here, if there are no space in the command line, assume it's a straight process and use execl
 * otherwise, use system...\n
 * The clean solution would be Sys_StartProcess and Sys_StartProcess_Args..
 */
void Sys_DoStartProcess(char *cmdline)
{
#ifndef __AROS__
	switch (fork())
	{
	case -1:
		// main thread
		break;
	case 0:
		if (strchr(cmdline, ' '))
		{
			system(cmdline);
		}
		else
		{
			execl(cmdline, cmdline, NULL);
			printf("execl failed: %s\n", strerror(errno));
		}
		_exit(0);
		break;
	}
#endif
}

/**
 * @brief Starts process
 * @param[in] cmdline Execution string
 * @param     doexit  Quit from game after executing command
 * @note might even want to add a small delay?
 *
 * If !doexit then start the process ASAP, otherwise push it for execution at exit
 * (i.e. let complete shutdown of the game and freeing of resources happen)
 */
void Sys_StartProcess(char *cmdline, qboolean doexit)
{
	if (doexit)
	{
		Com_DPrintf("Sys_StartProcess %s (delaying to final exit)\n", cmdline);
		Sys_DoStartProcess(cmdline);
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
		return;
	}

	Com_DPrintf("Sys_StartProcess %s\n", cmdline);
	Sys_DoStartProcess(cmdline);
}

/**
 * @brief Open URL in system browser
 * @param[in] url    URL to open
 * @param     doexit Quit from game after opening URL
 */
void Sys_OpenURL(const char *url, qboolean doexit)
{
#ifndef DEDICATED
#ifndef __AROS__
	char fn[MAX_OSPATH];
	char cmdline[MAX_CMD];
#endif

	static qboolean doexit_spamguard = qfalse;

	if (doexit_spamguard)
	{
		Com_DPrintf("Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url);
		return;
	}

	Com_Printf("Open URL: %s\n", url);

#ifndef __AROS__
	Com_DPrintf("URL script: %s\n", fn);

#ifdef __APPLE__
	Com_sprintf(cmdline, MAX_CMD, "open '%s' &", url);
#else
	Com_sprintf(cmdline, MAX_CMD, "xdg-open '%s' &", url);
#endif

	Sys_StartProcess(cmdline, doexit);
#else
	if ((OpenURLBase = OpenLibrary("openurl.library", 1)))
	{
		URL_Open(url, TAG_DONE);
		CloseLibrary(OpenURLBase);
	}
#endif

	SDL_WM_IconifyWindow();
#endif
}

/**
 * @brief Unix specific "safe" GL implementation initialisation
 */
void Sys_GLimpSafeInit(void)
{
	// NOP
}

/**
 * @brief Unix specific GL implementation initialisation
 */
void Sys_GLimpInit(void)
{
	// NOP
}

/**
 * @brief Unix specific initialisation
 */
void Sys_PlatformInit(void)
{
	const char *term = getenv("TERM");

	signal(SIGHUP, Sys_SigHandler);
	signal(SIGQUIT, Sys_SigHandler);
	signal(SIGTRAP, Sys_SigHandler);
	signal(SIGIOT, Sys_SigHandler);
	signal(SIGBUS, Sys_SigHandler);

#ifndef __AROS__
	stdinIsATTY = isatty(STDIN_FILENO) &&
	              !(term && (!strcmp(term, "raw") || !strcmp(term, "dumb")));
#endif
}

/**
 * @brief Set/Unset environment variables
 * @param[in] name  Environment Variable
 * @param[in] value New value to set
 * @note Empty value removes variable
 */
void Sys_SetEnv(const char *name, const char *value)
{
	if (value && *value)
	{
		setenv(name, value, 1);
	}
	else
	{
		unsetenv(name);
	}
}

/**
 * @return PID of current process
*/
int Sys_PID(void)
{
	return getpid();
}

/**
 * @brief Check if specified PID is running
 * @param pid PID to check
 * @retval qfalse on failure
 * @retval qtrue  on success
 */
qboolean Sys_PIDIsRunning(int pid)
{
	return kill(pid, 0) == 0;
}
