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
 * @brief Contains windows-specific code for console.
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
#include "sys_win32.h"

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
#include <shlobj.h> // for SHGetFolderPath
#include <psapi.h>
#include <setjmp.h>

// Used to determine where to store user-specific files
static char    homePath[MAX_OSPATH] = { 0 };
static jmp_buf sys_exitframe;
static int     sys_retcode;
static char    sys_exitstr[MAX_STRING_CHARS];

#ifdef __WIN64__
void Sys_SnapVector(float *v)
{
	v[0] = rint(v[0]);
	v[1] = rint(v[1]);
	v[2] = rint(v[2]);
}
#endif

/*
 * @return homepath pointing to "My Documents\WolfETL"
 */
char *Sys_DefaultHomePath(void)
{
	TCHAR   szPath[MAX_PATH];
	HRESULT found;

	if (!*homePath /*&& !com_homepath*/)
	{
		// FIXME: forcing SHGFP_TYPE_DEFAULT because file creation fails
		//        when real CSIDL_PERSONAL is on a mapped drive
		// NOTE: SHGetFolderPath is marked as deprecated
		found = SHGetFolderPath(NULL, CSIDL_PERSONAL,
		                        NULL, SHGFP_TYPE_DEFAULT, szPath);

		if (found != S_OK)
		{
			Com_Printf("Unable to detect CSIDL_PERSONAL\n");
			return NULL;
		}
		Q_strncpyz(homePath, szPath, sizeof(homePath));
		Q_strcat(homePath, sizeof(homePath), "\\WolfETL");
	}

	return homePath;
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
Sys_FOpen
==============
*/
FILE *Sys_FOpen(const char *ospath, const char *mode)
{
	return fopen(ospath, mode);
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
		Com_Error(ERR_DROP, "Could not start process: '%s\\%s'", szPathOrig, exeName);
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

	SDL_WM_IconifyWindow();
#endif
}

/*
==================
WinMain
==================
*/
#ifdef _WIN32
WinVars_t   g_wv;
static char sys_cmdline[MAX_STRING_CHARS];
int         totalMsec, countMsec;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char cwd[MAX_OSPATH];
	int  startTime, endTime;

	// should never get a previous instance in Win32
	if (hPrevInstance)
	{
		return 0;
	}

#ifdef EXCEPTION_HANDLER
	WinSetExceptionVersion(Q3_VERSION);
#endif

	g_wv.hInstance = hInstance;
	Q_strncpyz(sys_cmdline, lpCmdLine, sizeof(sys_cmdline));

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// get the initial time base
	Sys_Milliseconds();

	Com_Init(sys_cmdline);
	NET_Init();

	_getcwd(cwd, sizeof(cwd));
	Com_Printf("Working directory: %s\n", cwd);

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
#if defined (_WIN32) && !defined (_DEBUG)
	if (!com_dedicated->integer && !com_viewlog->integer)
	{
		Sys_ShowConsole(0, qfalse);
	}
#endif

	//Jacker: We should not set the focus here as the focus gets lost to the console window
	//SetFocus(g_wv.hWnd);

	// main game loop
	while (1)
	{
		// if not running as a game client, sleep a bit
		if (g_wv.isMinimized || (com_dedicated && com_dedicated->integer))
		{
			Sleep(5);
		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
		//_controlfp( _PC_24, _MCW_PC );
		//_controlfp( -1, _MCW_EM  );	// no exceptions, even if some crappy
		// syscall turns them back on!

		startTime = Sys_Milliseconds();

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		//Com_FrameExt();
		Com_Frame();

		endTime    = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		countMsec++;
	}

	// never gets here
}
#endif

/*
==============
Sys_IsNumLockDown
==============
*/
qboolean Sys_IsNumLockDown(void)
{
	SHORT state = GetKeyState(VK_NUMLOCK);

	if (state & 0x01)
	{
		return qtrue;
	}

	return qfalse;
}
