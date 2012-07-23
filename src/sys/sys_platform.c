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
 * @brief Contains platform-specific code divided into sections. This file will
 * get separated once premake supports different files per configuration
 */

#ifndef DEDICATED
#    ifdef BUNDLED_LIBS
#        include "SDL_video.h"
#    else
#        include <SDL/SDL_video.h>
#    endif

      // @todo SDL 2.0 window pointer from sdl_glimp.c
      extern SDL_Window* screen;
#endif
  
#ifdef _WIN32

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#include <windows.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <psapi.h>
  
// Used to determine where to store user-specific files
static char homePath[MAX_OSPATH] = { 0 };

#ifdef __WIN64__
void Sys_SnapVector(float *v)
{
	v[0] = rint(v[0]);
	v[1] = rint(v[1]);
	v[2] = rint(v[2]);
}
#endif

/*
================
Sys_DefaultHomePath
================
*/
char *Sys_DefaultHomePath(void)
{
	TCHAR   szPath[MAX_PATH];
	FARPROC qSHGetFolderPath;
	HMODULE shfolder = LoadLibrary("shfolder.dll");

	if (!*homePath)
	{
		if (shfolder == NULL)
		{
			Com_Printf("Unable to load SHFolder.dll\n");
			return NULL;
		}

		qSHGetFolderPath = GetProcAddress(shfolder, "SHGetFolderPathA");
		if (qSHGetFolderPath == NULL)
		{
			Com_Printf("Unable to find SHGetFolderPath in SHFolder.dll\n");
			FreeLibrary(shfolder);
			return NULL;
		}

		if (!SUCCEEDED(qSHGetFolderPath(NULL, CSIDL_APPDATA,
		                                NULL, 0, szPath)))
		{
			Com_Printf("Unable to detect CSIDL_APPDATA\n");
			FreeLibrary(shfolder);
			return NULL;
		}
		Q_strncpyz(homePath, szPath, sizeof(homePath));
		Q_strcat(homePath, sizeof(homePath), "\\WolfET");
		FreeLibrary(shfolder);
	}

	return homePath;
}

/*
================
Sys_TempPath
================
*/
const char *Sys_TempPath(void)
{
	static TCHAR path[MAX_PATH];
	DWORD        length;

	length = GetTempPath(sizeof(path), path);

	if (length > sizeof(path) || length == 0)
	{
		return Sys_DefaultHomePath();
	}
	else
	{
		return path;
	}
}

/*
================
Sys_Milliseconds
================
*/
int sys_timeBase;
int Sys_Milliseconds(void)
{
	int             sys_curtime;
	static qboolean initialized = qfalse;

	if (!initialized)
	{
		sys_timeBase = timeGetTime();
		initialized  = qtrue;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

// #ifndef __GNUC__ //see snapvectora.s
/*
================
Sys_SnapVector
================
*/
// void Sys_SnapVector( float *v )
// {
//  int i;
//  float f;
//
//  f = *v;
//  __asm   fld     f;
//  __asm   fistp   i;
//  *v = i;
//  v++;
//  f = *v;
//  __asm   fld     f;
//  __asm   fistp   i;
//  *v = i;
//  v++;
//  f = *v;
//  __asm   fld     f;
//  __asm   fistp   i;
//  *v = i;
// }
// #endif

/*
==================
Sys_SnapVector
==================
*/
void Sys_SnapVector(float *v)
{
	v[0] = rint(v[0]);
	v[1] = rint(v[1]);
	v[2] = rint(v[2]);
}
// #endif

/*
================
Sys_RandomBytes
================
*/
qboolean Sys_RandomBytes(byte *string, int len)
{
	HCRYPTPROV prov;

	if (!CryptAcquireContext(&prov, NULL, NULL,
	                         PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{

		return qfalse;
	}

	if (!CryptGenRandom(prov, len, (BYTE *)string))
	{
		CryptReleaseContext(prov, 0);
		return qfalse;
	}
	CryptReleaseContext(prov, 0);
	return qtrue;
}

/*
================
Sys_GetCurrentUser
================
*/
char *Sys_GetCurrentUser(void)
{
	static char   s_userName[1024];
	unsigned long size = sizeof(s_userName);

	if (!GetUserName(s_userName, &size))
	{
		strcpy(s_userName, "player");
	}

	if (!s_userName[0])
	{
		strcpy(s_userName, "player");
	}

	return s_userName;
}

/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData(void)
{
	char *data = NULL;
	char *cliptext;

	if (OpenClipboard(NULL) != 0)
	{
		HANDLE hClipboardData;

		if ((hClipboardData = GetClipboardData(CF_TEXT)) != 0)
		{
			if ((cliptext = GlobalLock(hClipboardData)) != 0)
			{
				data = Z_Malloc(GlobalSize(hClipboardData) + 1);
				Q_strncpyz(data, cliptext, GlobalSize(hClipboardData));
				GlobalUnlock(hClipboardData);

				strtok(data, "\n\r\b");
			}
		}
		CloseClipboard();
	}
	return data;
}

#define MEM_THRESHOLD 96 * 1024 * 1024

/*
==================
Sys_LowPhysicalMemory
==================
*/
qboolean Sys_LowPhysicalMemory(void)
{
	MEMORYSTATUS stat;
	GlobalMemoryStatus(&stat);
	return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/*
==============
Sys_Basename
==============
*/
const char *Sys_Basename(char *path)
{
	static char base[MAX_OSPATH] = { 0 };
	int         length;

	length = strlen(path) - 1;

	// Skip trailing slashes
	while (length > 0 && path[length] == '\\')
		length--;

	while (length > 0 && path[length - 1] != '\\')
		length--;

	Q_strncpyz(base, &path[length], sizeof(base));

	length = strlen(base) - 1;

	// Strip trailing slashes
	while (length > 0 && base[length] == '\\')
		base[length--] = '\0';

	return base;
}

/*
==============
Sys_Dirname
==============
*/
const char *Sys_Dirname(char *path)
{
	static char dir[MAX_OSPATH] = { 0 };
	int         length;

	Q_strncpyz(dir, path, sizeof(dir));
	length = strlen(dir) - 1;

	while (length > 0 && dir[length] != '\\')
		length--;

	dir[length] = '\0';

	return dir;
}

/*
==============
Sys_Mkdir
==============
*/
qboolean Sys_Mkdir(const char *path)
{
	if (!CreateDirectory(path, NULL))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd(void)
{
	static char cwd[MAX_OSPATH];

	_getcwd(cwd, sizeof(cwd) - 1);
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define MAX_FOUND_FILES 0x1000

/*
==============
Sys_ListFilteredFiles
==============
*/
void Sys_ListFilteredFiles(const char *basedir, char *subdirs, char *filter, char **list, int *numfiles)
{
	char               search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char               filename[MAX_OSPATH];
	intptr_t           findhandle;
	struct _finddata_t findinfo;

	if (*numfiles >= MAX_FOUND_FILES - 1)
	{
		return;
	}

	if (strlen(subdirs))
	{
		Com_sprintf(search, sizeof(search), "%s\\%s\\*", basedir, subdirs);
	}
	else
	{
		Com_sprintf(search, sizeof(search), "%s\\*", basedir);
	}

	findhandle = _findfirst(search, &findinfo);
	if (findhandle == -1)
	{
		return;
	}

	do
	{
		if (findinfo.attrib & _A_SUBDIR)
		{
			if (Q_stricmp(findinfo.name, ".") && Q_stricmp(findinfo.name, ".."))
			{
				if (strlen(subdirs))
				{
					Com_sprintf(newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else
				{
					Com_sprintf(newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
			}
		}
		if (*numfiles >= MAX_FOUND_FILES - 1)
		{
			break;
		}
		Com_sprintf(filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name);
		if (!Com_FilterPath(filter, filename, qfalse))
		{
			continue;
		}
		list[*numfiles] = CopyString(filename);
		(*numfiles)++;
	}
	while (_findnext(findhandle, &findinfo) != -1);

	_findclose(findhandle);
}

/*
==============
strgtr
==============
*/
static qboolean strgtr(const char *s0, const char *s1)
{
	int l0, l1, i;

	l0 = strlen(s0);
	l1 = strlen(s1);

	if (l1 < l0)
	{
		l0 = l1;
	}

	for (i = 0; i < l0; i++)
	{
		if (s1[i] > s0[i])
		{
			return qtrue;
		}
		if (s1[i] < s0[i])
		{
			return qfalse;
		}
	}
	return qfalse;
}

/*
==============
Sys_ListFiles
==============
*/
char **Sys_ListFiles(const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs)
{
	char               search[MAX_OSPATH];
	int                nfiles;
	char               **listCopy;
	char               *list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	intptr_t           findhandle;
	int                flag;
	int                i;

	if (filter)
	{

		nfiles = 0;
		Sys_ListFilteredFiles(directory, "", filter, list, &nfiles);

		list[nfiles] = 0;
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

	// passing a slash as extension will find directories
	if (extension[0] == '/' && extension[1] == 0)
	{
		extension = "";
		flag      = 0;
	}
	else
	{
		flag = _A_SUBDIR;
	}

	Com_sprintf(search, sizeof(search), "%s\\*%s", directory, extension);

	// search
	nfiles = 0;

	findhandle = _findfirst(search, &findinfo);
	if (findhandle == -1)
	{
		*numfiles = 0;
		return NULL;
	}

	do
	{
		if ((!wantsubs && flag ^ (findinfo.attrib & _A_SUBDIR)) || (wantsubs && findinfo.attrib & _A_SUBDIR))
		{
			if (nfiles == MAX_FOUND_FILES - 1)
			{
				break;
			}
			list[nfiles] = CopyString(findinfo.name);
			nfiles++;
		}
	}
	while (_findnext(findhandle, &findinfo) != -1);

	list[nfiles] = 0;

	_findclose(findhandle);

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

	do
	{
		flag = 0;
		for (i = 1; i < nfiles; i++)
		{
			if (strgtr(listCopy[i - 1], listCopy[i]))
			{
				char *temp = listCopy[i];
				listCopy[i]     = listCopy[i - 1];
				listCopy[i - 1] = temp;
				flag            = 1;
			}
		}
	}
	while (flag);

	return listCopy;
}

/*
==============
Sys_FreeFileList
==============
*/
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


/*
==============
Sys_Sleep

Block execution for msec or until input is received.
==============
*/
void Sys_Sleep(int msec)
{
	if (msec == 0)
	{
		return;
	}

#ifdef DEDICATED
	if (msec < 0)
	{
		WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), INFINITE);
	}
	else
	{
		WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), msec);
	}
#else
	// Client Sys_Sleep doesn't support waiting on stdin
	if (msec < 0)
	{
		return;
	}

	Sleep(msec);
#endif
}

/*
==============
Sys_ErrorDialog

Display an error message
==============
*/
void Sys_ErrorDialog(const char *error)
{
	if (Sys_Dialog(DT_YES_NO, va("%s. Copy console log to clipboard?", error),
	               "Error") == DR_YES)
	{
		HGLOBAL memoryHandle;
		char    *clipMemory;

		memoryHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, CON_LogSize() + 1);
		clipMemory   = (char *)GlobalLock(memoryHandle);

		if (clipMemory)
		{
			char         *p = clipMemory;
			char         buffer[1024];
			unsigned int size;

			while ((size = CON_LogRead(buffer, sizeof(buffer))) > 0)
			{
				Com_Memcpy(p, buffer, size);
				p += size;
			}

			*p = '\0';

			if (OpenClipboard(NULL) && EmptyClipboard())
			{
				SetClipboardData(CF_TEXT, memoryHandle);
			}

			GlobalUnlock(clipMemory);
			CloseClipboard();
		}
	}
}

/*
==============
Sys_Dialog

Display a win32 dialog box
==============
*/
dialogResult_t Sys_Dialog(dialogType_t type, const char *message, const char *title)
{
	UINT uType;

	switch (type)
	{
	default:
	case DT_INFO:
		uType = MB_ICONINFORMATION | MB_OK;
		break;
	case DT_WARNING:
		uType = MB_ICONWARNING | MB_OK;
		break;
	case DT_ERROR:
		uType = MB_ICONERROR | MB_OK;
		break;
	case DT_YES_NO:
		uType = MB_ICONQUESTION | MB_YESNO;
		break;
	case DT_OK_CANCEL:
		uType = MB_ICONWARNING | MB_OKCANCEL;
		break;
	}

	switch (MessageBox(NULL, message, title, uType))
	{
	default:
	case IDOK:
		return DR_OK;
	case IDCANCEL:
		return DR_CANCEL;
	case IDYES:
		return DR_YES;
	case IDNO:
		return DR_NO;
	}
}

#ifndef DEDICATED
static qboolean SDL_VIDEODRIVER_externallySet = qfalse;
#endif

/*
==============
Sys_GLimpSafeInit

Windows specific "safe" GL implementation initialisation
==============
*/
void Sys_GLimpSafeInit(void)
{
#ifndef DEDICATED
	if (!SDL_VIDEODRIVER_externallySet)
	{
		// Here, we want to let SDL decide what do to unless
		// explicitly requested otherwise
		_putenv("SDL_VIDEODRIVER=");
	}
#endif
}

/*
==============
Sys_GLimpInit

Windows specific GL implementation initialisation
==============
*/
void Sys_GLimpInit(void)
{
#ifndef DEDICATED
	if (!SDL_VIDEODRIVER_externallySet)
	{
		// It's a little bit weird having in_mouse control the
		// video driver, but from ioq3's point of view they're
		// virtually the same except for the mouse input anyway
		if (Cvar_VariableIntegerValue("in_mouse") == -1)
		{
			// Use the windib SDL backend, which is closest to
			// the behaviour of idq3 with in_mouse set to -1
			_putenv("SDL_VIDEODRIVER=windib");
		}
		else
		{
			// Use the DirectX SDL backend
			_putenv("SDL_VIDEODRIVER=directx");
		}
	}
#endif
}

/*
==============
Sys_PlatformInit

Windows specific initialisation
==============
*/
void Sys_PlatformInit(void)
{
#ifndef DEDICATED
	const char *SDL_VIDEODRIVER = getenv("SDL_VIDEODRIVER");

	if (SDL_VIDEODRIVER)
	{
		Com_Printf("SDL_VIDEODRIVER is externally set to \"%s\", "
		           "in_mouse -1 will have no effect\n", SDL_VIDEODRIVER);
		SDL_VIDEODRIVER_externallySet = qtrue;
	}
	else
	{
		SDL_VIDEODRIVER_externallySet = qfalse;
	}
#endif
}

/*
==============
Sys_SetEnv

set/unset environment variables (empty value removes it)
==============
*/
void Sys_SetEnv(const char *name, const char *value)
{
	_putenv(va("%s=%s", name, value));
}

/*
==============
Sys_PID
==============
*/
int Sys_PID(void)
{
	return GetCurrentProcessId();
}

/*
==============
Sys_PIDIsRunning
==============
*/
qboolean Sys_PIDIsRunning(int pid)
{
	DWORD processes[1024];
	DWORD numBytes, numProcesses;
	int   i;

	if (!EnumProcesses(processes, sizeof(processes), &numBytes))
	{
		return qfalse; // Assume it's not running

	}
	numProcesses = numBytes / sizeof(DWORD);

	// Search for the pid
	for (i = 0; i < numProcesses; i++)
	{
		if (processes[i] == pid)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
==================
Sys_StartProcess

NERVE - SMF
==================
*/
void Sys_StartProcess(char *exeName, qboolean doexit)
{
	TCHAR               szPathOrig[_MAX_PATH];
	STARTUPINFO         si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	GetCurrentDirectory(_MAX_PATH, szPathOrig);

	// JPW NERVE swiped from Sherman's SP code
	if (!CreateProcess(NULL, va("%s\\%s", szPathOrig, exeName), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		// couldn't start it, popup error box
		Com_Error(ERR_DROP, "Could not start process: '%s\\%s'\n", szPathOrig, exeName);
		return;
	}
	// jpw

	// TTimo: similar way of exiting as used in Sys_OpenURL below
	if (doexit)
	{
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
	}
}

/*
==================
Sys_OpenURL

NERVE - SMF
==================
*/
void Sys_OpenURL(const char *url, qboolean doexit)
{
#ifndef DEDICATED
	HWND wnd;

	static qboolean doexit_spamguard = qfalse;

	if (doexit_spamguard)
	{
		Com_DPrintf("Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url);
		return;
	}

	Com_Printf("Open URL: %s\n", url);

	if (!ShellExecute(NULL, "open", url, NULL, NULL, SW_RESTORE))
	{
		// couldn't start it, popup error box
		Com_Error(ERR_DROP, "Could not open url: '%s' ", url);
		return;
	}

	wnd = GetForegroundWindow();

	if (wnd)
	{
		ShowWindow(wnd, SW_MAXIMIZE);
	}

	if (doexit)
	{
		// show_bug.cgi?id=612
		doexit_spamguard = qtrue;
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
	}

	SDL_MinimizeWindow(screen);
#endif
}

#else

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
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <libgen.h>
#include <fcntl.h>
  
qboolean stdinIsATTY;

// Used to determine where to store user-specific files
static char homePath[MAX_OSPATH] = { 0 };

/*
==================
Sys_DefaultHomePath
==================
*/
char *Sys_DefaultHomePath(void)
{
	char *p;

	if (!*homePath)
	{
		if ((p = getenv("HOME")) != NULL)
		{
			Q_strncpyz(homePath, p, sizeof(homePath));
#ifdef MACOS_X
			Q_strcat(homePath, sizeof(homePath),
			         "/Library/Application Support/WolfensteinMP");
#else
			Q_strcat(homePath, sizeof(homePath), "/.etwolf");
#endif
		}
	}

	return homePath;
}

#ifndef MACOS_X
/*
================
Sys_TempPath
================
*/
const char *Sys_TempPath(void)
{
	const char *TMPDIR = getenv("TMPDIR");

	if (TMPDIR == NULL || TMPDIR[0] == '\0')
	{
		return "/tmp";
	}
	else
	{
		return TMPDIR;
	}
}
#endif

/*
==================
chmod OR on a file
==================
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

/*
================
Sys_Milliseconds
================
*/
/* base time in seconds, that's our origin
   timeval:tv_sec is an int:
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */
unsigned long sys_timeBase = 0;
/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
     0x7fffffff ms - ~24 days
   although timeval:tv_usec is an int, I'm not sure wether it is actually used as an unsigned int
     (which would affect the wrap period) */
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

// #if !id386
/*
==================
fastftol
==================
*/
long fastftol(float f)
{
	return (long)f;
}

/*
==================
Sys_SnapVector
==================
*/
void Sys_SnapVector(float *v)
{
	v[0] = rint(v[0]);
	v[1] = rint(v[1]);
	v[2] = rint(v[2]);
}
// #endif


/*
==================
Sys_RandomBytes
==================
*/
qboolean Sys_RandomBytes(byte *string, int len)
{
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
}

/*
==================
Sys_GetCurrentUser
==================
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

/*
==================
Sys_GetClipboardData
==================
*/
char *Sys_GetClipboardData(void)
{
	return NULL;
}

#define MEM_THRESHOLD 96 * 1024 * 1024

/*
==================
Sys_LowPhysicalMemory

TODO
==================
*/
qboolean Sys_LowPhysicalMemory(void)
{
	return qfalse;
}

/*
==================
Sys_Basename
==================
*/
const char *Sys_Basename(char *path)
{
	return basename(path);
}

/*
==================
Sys_Dirname
==================
*/
const char *Sys_Dirname(char *path)
{
	return dirname(path);
}

/*
==================
Sys_Mkdir
==================
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

/*
==================
Sys_Cwd
==================
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

/*
==================
Sys_ListFilteredFiles
==================
*/
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

/*
==================
Sys_ListFiles
==================
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

	int extLen;

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

/*
==================
Sys_FreeFileList
==================
*/
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

/*
==================
Sys_Sleep

Block execution for msec or until input is recieved.
==================
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

/*
==============
Sys_ErrorDialog

Display an error message
==============
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
	Sys_Dialog(DT_ERROR, va("%s. See \"%s\" for details.", error, ospath), "Error");
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

#ifndef MACOS_X
/*
==============
Sys_ZenityCommand
==============
*/
static int Sys_ZenityCommand(dialogType_t type, const char *message, const char *title)
{
	const char *options = "";
	char       command[1024];

	switch (type)
	{
	default:
	case DT_INFO:
		options = "--info";
		break;
	case DT_WARNING:
		options = "--warning";
		break;
	case DT_ERROR:
		options = "--error";
		break;
	case DT_YES_NO:
		options = "--question --ok-label=\"Yes\" --cancel-label=\"No\"";
		break;
	case DT_OK_CANCEL:
		options = "--question --ok-label=\"OK\" --cancel-label=\"Cancel\"";
		break;
	}

	Com_sprintf(command, sizeof(command), "zenity %s --text=\"%s\" --title=\"%s\"",
	            options, message, title);

	return system(command);
}

/*
==============
Sys_KdialogCommand
==============
*/
static int Sys_KdialogCommand(dialogType_t type, const char *message, const char *title)
{
	const char *options = "";
	char       command[1024];

	switch (type)
	{
	default:
	case DT_INFO:
		options = "--msgbox";
		break;
	case DT_WARNING:
		options = "--sorry";
		break;
	case DT_ERROR:
		options = "--error";
		break;
	case DT_YES_NO:
		options = "--warningyesno";
		break;
	case DT_OK_CANCEL:
		options = "--warningcontinuecancel";
		break;
	}

	Com_sprintf(command, sizeof(command), "kdialog %s \"%s\" --title \"%s\"",
	            options, message, title);

	return system(command);
}

/*
==============
Sys_XmessageCommand
==============
*/
static int Sys_XmessageCommand(dialogType_t type, const char *message, const char *title)
{
	const char *options = "";
	char       command[1024];

	switch (type)
	{
	default:
		options = "-buttons OK";
		break;
	case DT_YES_NO:
		options = "-buttons Yes:0,No:1";
		break;
	case DT_OK_CANCEL:
		options = "-buttons OK:0,Cancel:1";
		break;
	}

	Com_sprintf(command, sizeof(command), "xmessage -center %s \"%s\"",
	            options, message);

	return system(command);
}

/*
==============
Sys_Dialog

Display a *nix dialog box
==============
*/
dialogResult_t Sys_Dialog(dialogType_t type, const char *message, const char *title)
{
	typedef enum
	{
		NONE = 0,
		ZENITY,
		KDIALOG,
		XMESSAGE,
		NUM_DIALOG_PROGRAMS
	} dialogCommandType_t;
	typedef int (*dialogCommandBuilder_t)(dialogType_t, const char *, const char *);

	const char             *session                      = getenv("DESKTOP_SESSION");
	qboolean               tried[NUM_DIALOG_PROGRAMS]    = { qfalse };
	dialogCommandBuilder_t commands[NUM_DIALOG_PROGRAMS] = { NULL };
	dialogCommandType_t    preferredCommandType          = NONE;

	commands[ZENITY]   = &Sys_ZenityCommand;
	commands[KDIALOG]  = &Sys_KdialogCommand;
	commands[XMESSAGE] = &Sys_XmessageCommand;

	// This may not be the best way
	if (!Q_stricmp(session, "gnome"))
	{
		preferredCommandType = ZENITY;
	}
	else if (!Q_stricmp(session, "kde"))
	{
		preferredCommandType = KDIALOG;
	}

	while (1)
	{
		int i;
		int exitCode;

		for (i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++)
		{
			if (preferredCommandType != NONE && preferredCommandType != i)
			{
				continue;
			}

			if (!tried[i])
			{
				exitCode = commands[i](type, message, title);

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

		for (i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++)
		{
			if (!tried[i])
			{
				continue;
			}
		}

		break;
	}

	Com_DPrintf(S_COLOR_YELLOW "WARNING: failed to show a dialog\n");
	return DR_OK;
}
#endif

/*
==================
Sys_DoStartProcess
actually forks and starts a process

UGLY HACK:
  Sys_StartProcess works with a command line only
  if this command line is actually a binary with command line parameters,
  the only way to do this on unix is to use a system() call
  but system doesn't replace the current process, which leads to a situation like:
  wolf.x86--spawned_process.x86
  in the case of auto-update for instance, this will cause write access denied on wolf.x86:
  wolf-beta/2002-March/000079.html
  we hack the implementation here, if there are no space in the command line, assume it's a straight process and use execl
  otherwise, use system ..
  The clean solution would be Sys_StartProcess and Sys_StartProcess_Args..
==================
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
}

/*
==================
Sys_StartProcess
if !doexit, start the process asap
otherwise, push it for execution at exit
(i.e. let complete shutdown of the game and freeing of resources happen)
NOTE: might even want to add a small delay?
==================
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

/*
=================
Sys_OpenURL
=================
*/
void Sys_OpenURL(const char *url, qboolean doexit)
{
#ifndef DEDICATED
	char fn[MAX_OSPATH];
	char cmdline[MAX_CMD];

	static qboolean doexit_spamguard = qfalse;

	if (doexit_spamguard)
	{
		Com_DPrintf("Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url);
		return;
	}

	Com_Printf("Open URL: %s\n", url);

	Com_DPrintf("URL script: %s\n", fn);

	Com_sprintf(cmdline, MAX_CMD, "xdg-open '%s' &", url);
	Sys_StartProcess(cmdline, doexit);

	SDL_MinimizeWindow(screen);
#endif
}

/*
==============
Sys_GLimpSafeInit

Unix specific "safe" GL implementation initialisation
==============
*/
void Sys_GLimpSafeInit(void)
{
	// NOP
}

/*
==============
Sys_GLimpInit

Unix specific GL implementation initialisation
==============
*/
void Sys_GLimpInit(void)
{
	// NOP
}

/*
==============
Sys_PlatformInit

Unix specific initialisation
==============
*/
void Sys_PlatformInit(void)
{
	const char *term = getenv("TERM");

	signal(SIGHUP, Sys_SigHandler);
	signal(SIGQUIT, Sys_SigHandler);
	signal(SIGTRAP, Sys_SigHandler);
	signal(SIGIOT, Sys_SigHandler);
	signal(SIGBUS, Sys_SigHandler);

	stdinIsATTY = isatty(STDIN_FILENO) &&
	              !(term && (!strcmp(term, "raw") || !strcmp(term, "dumb")));
}

/*
==============
Sys_SetEnv

set/unset environment variables (empty value removes it)
==============
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

/*
==============
Sys_PID
==============
*/
int Sys_PID(void)
{
	return getpid();
}

/*
==============
Sys_PIDIsRunning
==============
*/
qboolean Sys_PIDIsRunning(int pid)
{
	return kill(pid, 0) == 0;
}
#endif
