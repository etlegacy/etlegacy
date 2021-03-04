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
 * @file sys_unix.c
 * @brief Contains unix-specific code for console.
 */

#ifndef DEDICATED
#include "../sdl/sdl_defs.h"
#endif

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"
#ifndef DEDICATED
#include "../renderercommon/tr_common.h"
#endif

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/wait.h>

qboolean stdinIsATTY;

// Used to determine where to store user-specific files
static char homePath[MAX_OSPATH] = { 0 };

/**
 * @brief Get current home path
 * @return path to ET: Legacy directory
 */
char *Sys_DefaultHomePath(void)
{
	char *p;

	if (!*homePath)
	{
		if ((p = getenv("HOME")) != NULL)
		{
			Q_strncpyz(homePath, p, sizeof(homePath));
#ifdef __APPLE__
			Q_strncpyz(homePath, OSX_ApplicationSupportPath(), sizeof(homePath));
			Q_strcat(homePath, sizeof(homePath), "/etlegacy");
#else
			Q_strcat(homePath, sizeof(homePath), "/.etlegacy");
#endif
		}

#ifdef __ANDROID__
		if(SDL_AndroidGetExternalStorageState())
		{
			Q_strncpyz(homePath, SDL_AndroidGetExternalStoragePath(), sizeof(homePath));
			Q_strcat(homePath, sizeof(homePath), "/etlegacy");
		}
		else
		{
			Q_strncpyz(homePath, SDL_AndroidGetInternalStoragePath(), sizeof(homePath));
			Q_strcat(homePath, sizeof(homePath), "/etlegacy");
		}
#endif
	}

	return homePath;
}

/**
 * @brief Safe function to Chmod a file and preventing TOCTOU attacks
 * @param[in] file Filename
 * @param[in] mode Access Mode
 */
void Sys_Chmod(const char *file, int mode)
{
	struct stat stat_infoBefore;
	struct stat fstat_infoAfter;
	int         perm;
	int         fd;

	// Get the state of the file
	if (stat(file, &stat_infoBefore) == -1)
	{
		Com_Printf("Sys_Chmod: first stat('%s')  failed: errno %d\n", file, errno);
		return;
	}

	perm = stat_infoBefore.st_mode | mode;

	// Try to get the file descriptor
	if ((fd = open(file, O_RDONLY, S_IXUSR)) == -1)
	{
		Com_Printf("Sys_Chmod: open('%s', %d, %d) failed: errno %d\n", file, O_RDONLY, S_IXUSR, errno);
		return;
	}

	Com_DPrintf("chmod +%d '%s'\n", mode, file);

	// Get the state of the file after opening function
	if (stat(file, &fstat_infoAfter) == -1)
	{
		Com_Printf("Sys_Chmod: second stat('%s')  failed: errno %d\n", file, errno);
		close(fd);
		return;
	}

	// Compare the state before and after opening
	if (stat_infoBefore.st_mode != fstat_infoAfter.st_mode ||
	    stat_infoBefore.st_ino != fstat_infoAfter.st_ino)
	{
		Com_Printf("Sys_Chmod: stat before and after open are different. The file ('%s') may differ (TOCTOU Attacks ?)\n", file);
		close(fd);
		return;
	}

	// Chmod the file by using the file descriptor
	if (fchmod(fd, perm) != 0)
	{
		Com_Printf("Sys_Chmod: fchmod('%s', %d) failed: errno %d\n", file, perm, errno);
	}

	close(fd);
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
	FILE *fp;

	fp = fopen("/dev/urandom", "r");
	if (!fp)
	{
		return qfalse;
	}

	setvbuf(fp, NULL, _IONBF, 0); // don't buffer reads from /dev/urandom

	if (fread(string, sizeof(byte), len, fp) != len)
	{
		fclose(fp);
		return qfalse;
	}

	fclose(fp);
	return qtrue;
}

/**
 * @brief Get current user username
 * @return username
 */
char *Sys_GetCurrentUser(void)
{
	struct passwd *p;

	if ((p = getpwuid(getuid())) == NULL)
	{
		return "player";
	}

	return p->pw_name;
}

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

/**
 * @brief Safe function to open a file and preventing TOCTOU attacks
 * @param[in] ospath The file path to open
 * @param[in] mode The mode used to open the file ('rb','wb','ab')
 * @return
 */
FILE *Sys_FOpen(const char *ospath, const char *mode)
{
	struct stat stat_infoBefore;
	struct stat stat_infoAfter;
	FILE        *fp;
	int         fd;
	int         oflag = 0;

	// Retrive the oflag for open depending of mode parameter
	if (*mode == 'w')
	{
		oflag |= O_WRONLY | O_CREAT | O_TRUNC;
	}
	else if (*mode == 'r')
	{
		oflag |= O_RDONLY;
	}
	else if (*mode == 'a')
	{
		oflag |= O_WRONLY | O_CREAT | O_APPEND;
	}
	else
	{
		Com_Printf("Sys_FOpen: invalid mode parameter to open file ('%s')", mode);
		return NULL;
	}

	// Handle + flag mode
	if (strlen(mode) > 1 && strchr(&mode[1], '+'))
	{
		oflag &= ~(O_WRONLY | O_RDONLY);
		oflag |= O_RDWR;
	}

	// Check the state (if path exists)
	if (stat(ospath, &stat_infoBefore) == -1)
	{
		// Check the error in case the the file doesn't exist
		if (errno != ENOENT)
		{
			Com_Printf("Sys_FOpen: first stat('%s')  failed: errno %d\n", ospath, errno);
			return NULL;
		}
		else if (*mode == 'r')
		{
			return NULL;
		}
	}
	else if (S_ISDIR(stat_infoBefore.st_mode))
	{
		return NULL;
	}

	// Try to open the file and get the file descriptor
	if ((fd = open(ospath, oflag, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) == -1)
	{
		Com_Printf("Sys_FOpen: open('%s', %d) failed: errno %d\n", ospath, oflag, errno);
		return NULL;
	}

	// Get the state of the current handle file only if the file wasn't created
	if (*mode == 'r' && fstat(fd, &stat_infoAfter) == -1)
	{
		Com_Printf("Sys_FOpen: second stat('%s')  failed: errno %d\n", ospath, errno);
		close(fd);
		return NULL;
	}

	// Compare the state before and after opening only if the file wasn't created
	if (*mode == 'r' && (stat_infoBefore.st_mode != stat_infoAfter.st_mode ||
	                     stat_infoBefore.st_ino != stat_infoAfter.st_ino))
	{
		Com_Printf("Sys_FOpen: stat before and after chmod are different. The file ('%s') may differ (TOCTOU Attacks ?)\n", ospath);
		close(fd);
		return NULL;
	}

	// Try to open the file with the file descriptor
	if (!(fp = fdopen(fd, mode)))
	{
		Com_Printf("Sys_FOpen: fdopen('%s', %s) failed: errno %d - %s\n", ospath, mode, errno, strerror(errno));
		close(fd);
		unlink(ospath);
		return NULL;
	}

	return fp;
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

void Sys_ListFilteredFiles(const char *basedir, const char *subdirs, const char *filter, char **list, int *numfiles)
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
char **Sys_ListFiles(const char *directory, const char *extension, const char *filter, int *numfiles, qboolean wantsubs)
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
	qboolean      invalid;

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
		if ((dironly && !(st.st_mode & S_IFDIR)) || (!dironly && (st.st_mode & S_IFDIR)))
		{
			continue;
		}

		if (*extension)
		{
			if (strlen(d->d_name) < extLen || Q_stricmp(d->d_name + strlen(d->d_name) - extLen, extension))
			{
				continue; // didn't match
			}
		}

		// check for bad file names
		invalid = qfalse;
		// note: this isn't done in Sys_ListFilteredFiles()

		for (i = 0; i < strlen(d->d_name); i++)
		{
			if (d->d_name[i] <= 31 || d->d_name[i] >= 127)
			{
				Com_Printf(S_COLOR_RED "ERROR: invalid char in name of file '%s'.\n", d->d_name);
				invalid = qtrue;
				break;
			}
		}

		if (invalid)
		{
			int error;

			error = remove(va("%s%c%s", directory, PATH_SEP, d->d_name));

			if (error != 0)
			{
				Com_Printf(S_COLOR_RED "ERROR: cannot delete '%s'.\n", d->d_name);
			}

#ifdef DEDICATED
			Sys_Error("Invalid character in file name '%s'. The file has been removed. Start the server again.", d->d_name);
#else
			Cvar_Set("com_missingFiles", "");
			Com_Error(ERR_DROP, "Invalid file name detected and removed\nFile \"%s\" contains an invalid character for ET: Legacy file structure.\nSome admins take advantage of this to ensure their menu loads last.\nThe file has been removed.", d->d_name);
#endif
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
 * @brief Displays an error message and writes the error into crashlog.txt
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
	char         *dirpath  = FS_BuildOSPath(homepath, gamedir, "");
	char         *ospath   = FS_BuildOSPath(homepath, gamedir, fileName);

	Sys_Print(va("%s\n", error));

#ifndef DEDICATED
	Sys_Dialog(DT_ERROR, va("%s\nSee \"%s\" for details.\n", error, ospath), "Error");
#endif

	// Make sure the write path for the crashlog exists...
	// check homepath
	if (!Sys_Mkdir(homepath))
	{
		Com_Printf("ERROR: couldn't create path '%s' to write file '%s'.\n", homepath, ospath);
		return;
	}
	// check gamedir (inside homepath)
	if (!Sys_Mkdir(dirpath))
	{
		Com_Printf("ERROR: couldn't create path '%s' to write file '%s'.\n", dirpath, ospath);
		return;
	}

	// We might be crashing because we maxed out the Quake MAX_FILE_HANDLES,
	// which will come through here, so we don't want to recurse forever by
	// calling FS_FOpenFileWrite()...use the Unix system APIs instead.
	f = open(ospath, O_CREAT | O_TRUNC | O_WRONLY, 0640);
	if (f == -1)
	{
		Com_Printf("ERROR: couldn't open '%s'\n", fileName);
		return;
	}

	// We're crashing, so we don't care much if write() or close() fails.
	while ((size = CON_LogRead(buffer, sizeof(buffer))) > 0)
	{
		if (write(f, buffer, size) != size)
		{
			Com_Printf("ERROR: couldn't fully write to '%s'\n", fileName);
			break;
		}
	}

	close(f);
}

#if !defined (__APPLE__)
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
	default:           Sys_AppendToExecBuffer("OK:0");
		break;
	case DT_YES_NO:    Sys_AppendToExecBuffer("Yes:0,No:1");
		break;
	case DT_OK_CANCEL: Sys_AppendToExecBuffer("OK:0,Cancel:1");
		break;
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

	const char          *session = getenv("DESKTOP_SESSION");
	int                 i, exitCode;
	qboolean            tried[NUM_DIALOG_PROGRAMS] = { qfalse };
	dialogCommandType_t preferredCommandType       = NONE;

	// first check the obsolete DESKTOP_SESSION
	if (!Q_stricmp(session, "default") || !Q_stricmp(session, ""))
	{
		// try the standard
		session = getenv("XDG_CURRENT_DESKTOP");
	}

	if (!Q_stricmpn(session, "gnome", 5)
	    || !Q_stricmpn(session, "unity", 5)
	    || !Q_stricmpn(session, "xfce", 4))
	{
		preferredCommandType = ZENITY;
	}
	else if (!Q_stricmpn(session, "kde", 3))
	{
		preferredCommandType = KDIALOG;
	}

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
				i                    = NONE;
			}
		}
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
	switch (fork())
	{
	case -1:
		// main thread
		break;
	case 0:
		if (strchr(cmdline, ' '))
		{
			int ret = system(cmdline);

			// we assume there is a valid command
			switch (ret)
			{
			case -1:
				printf("execl failed: child process could not be created, or its status could not be retrieved\n");
				break;
			default:
				break;
			}
		}
		else
		{
			execl(cmdline, cmdline, NULL);
			printf("execl failed: %s\n", strerror(errno));
		}

		_exit(0);
		break;
	}
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
	char cmdline[MAX_CMD];

	static qboolean doexit_spamguard = qfalse;

	if (doexit_spamguard)
	{
		Com_DPrintf("Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url);
		return;
	}

	Com_Printf("Open URL: %s\n", url);

#ifdef __APPLE__
	Com_sprintf(cmdline, MAX_CMD, "open '%s' &", url);
#else
	Com_sprintf(cmdline, MAX_CMD, "xdg-open '%s' &", url);
#endif

	Sys_StartProcess(cmdline, doexit);
#endif // not DEDICATED
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

// don't set signal handlers for anything that will generate coredump (in DEBUG builds)
#if !defined(ETLEGACY_DEBUG)
	signal(SIGTRAP, Sys_SigHandler);
	signal(SIGBUS, Sys_SigHandler);
#endif
	signal(SIGHUP, Sys_SigHandler);
	signal(SIGABRT, Sys_SigHandler);
	signal(SIGQUIT, Sys_SigHandler);

	stdinIsATTY = isatty(STDIN_FILENO) &&
	              !(term && (!strcmp(term, "raw") || !strcmp(term, "dumb")));
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
qboolean Sys_PIDIsRunning(unsigned int pid)
{
	return kill(pid, 0) == 0;
}

/**
 * @brief Check if filename should be allowed to be loaded as a DLL.
 * @param name
 */
qboolean Sys_DllExtension(const char *name)
{
	const char *p;
	char       c = 0;

	if (COM_CompareExtension(name, DLL_EXT))
	{
		return qtrue;
	}

	// Check for format of filename.so.1.2.3
	p = strstr(name, DLL_EXT ".");

	if (p)
	{
		p += strlen(DLL_EXT);

		// Check if .so is only followed for periods and numbers.
		while (*p)
		{
			c = *p;

			if (!isdigit(c) && c != '.')
			{
				return qfalse;
			}

			p++;
		}

		// Don't allow filename to end in a period. file.so., file.so.0., etc
		if (c != '.')
		{
			return qtrue;
		}
	}

	return qfalse;
}
