/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2017 ET:Legacy team <mail@etlegacy.com>
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
 * @file sys_win32.c
 * @brief Contains windows-specific code for console.
 */

#ifndef DEDICATED
#    ifdef BUNDLED_SDL
#        include "SDL_video.h"
#    else
#        include <SDL2/SDL_video.h>
#    endif
#endif

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_resource.h"
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
//static jmp_buf sys_exitframe;
//static int     sys_retcode;
//static char    sys_exitstr[MAX_STRING_CHARS];

/**
 * @return homepath pointing to "My Documents\\ETLegacy"
 */
char *Sys_DefaultHomePath(void)
{
	TCHAR   szPath[MAX_PATH];
	HRESULT found;

	if (!*homePath /*&& !com_homepath*/)
	{
		// FIXME: forcing SHGFP_TYPE_CURRENT because file creation fails
		//        when real CSIDL_PERSONAL is on a mapped drive
		// NOTE: SHGetFolderPath is marked as deprecated
		found = SHGetFolderPath(NULL, CSIDL_PERSONAL,
		                        NULL, SHGFP_TYPE_CURRENT, szPath);

		if (found != S_OK)
		{
			Com_Printf("Unable to detect CSIDL_PERSONAL\n");
			return NULL;
		}

		Q_strncpyz(homePath, szPath, sizeof(homePath));
		Q_strcat(homePath, sizeof(homePath), "\\ETLegacy");
	}

	return homePath;
}

int sys_timeBase;

/**
 * @brief Sys_Milliseconds
 * @return
 */
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

/**
 * @brief Sys_SnapVector
 * @param[in,out] v
 */
void Sys_SnapVector(float *v)
{
	v[0] = rint(v[0]);
	v[1] = rint(v[1]);
	v[2] = rint(v[2]);
}

/**
 * @brief Sys_RandomBytes
 * @param[in] string
 * @param[in] len
 * @return
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

/**
 * @brief Sys_GetCurrentUser
 * @return
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

#define MEM_THRESHOLD 96 * 1024 * 1024

/**
 * @brief Sys_LowPhysicalMemory
 * @return
 */
qboolean Sys_LowPhysicalMemory(void)
{
	MEMORYSTATUS stat;
	GlobalMemoryStatus(&stat);
	return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/**
 * @brief Sys_Basename
 * @param path
 * @return
 */
const char *Sys_Basename(char *path)
{
	static char  base[MAX_OSPATH] = { 0 };
	size_t length;

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

/**
 * @brief Sys_Dirname
 * @param path
 * @return
 */
const char *Sys_Dirname(char *path)
{
	static char  dir[MAX_OSPATH] = { 0 };
	size_t length;

	Q_strncpyz(dir, path, sizeof(dir));
	length = strlen(dir) - 1;

	while (length > 0 && dir[length] != '\\')
		length--;

	dir[length] = '\0';

	return dir;
}

/**
 * @brief Sys_FOpen
 * @param[in] ospath
 * @param[in] mode
 * @return
 */
FILE *Sys_FOpen(const char *ospath, const char *mode)
{
	return fopen(ospath, mode);
}

/**
 * @brief Sys_Mkdir
 * @param[in] path
 * @return
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

/**
 * @brief Sys_Cwd
 * @return
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

/**
 * @brief Sys_ListFilteredFiles
 * @param[in] basedir
 * @param[in] subdirs
 * @param[in] filter
 * @param[out] list
 * @param[in,out] numfiles
 */
void Sys_ListFilteredFiles(const char *basedir, const char *subdirs, const char *filter, char **list, int *numfiles)
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

/**
 * @brief strgtr
 * @param[in] s0
 * @param[in] s1
 * @return
 */
static qboolean strgtr(const char *s0, const char *s1)
{
	size_t l0, l1, i;

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

/**
 * @brief Sys_ListFiles
 * @param[in] directory
 * @param[in] extension
 * @param[in] filter
 * @param[out] numfiles
 * @param[in] wantsubs
 * @return
 */
char **Sys_ListFiles(const char *directory, const char *extension, const char *filter, int *numfiles, qboolean wantsubs)
{
	char               search[MAX_OSPATH];
	int                nfiles;
	char               **listCopy;
	char               *list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	intptr_t           findhandle;
	int                flag;
	int                i;
	size_t       extLen;
#ifdef DEDICATED
	qboolean invalid;
#endif

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

	extLen = strlen(extension);

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
		if ((!wantsubs && (flag ^ (findinfo.attrib & _A_SUBDIR))) || (wantsubs && (findinfo.attrib & _A_SUBDIR)))
		{
			if (*extension)
			{
				if (strlen(findinfo.name) < extLen || Q_stricmp(findinfo.name + strlen(findinfo.name) - extLen, extension))
				{
					continue; // didn't match
				}
			}

#ifdef DEDICATED
			// check for bad file names and don't add these
			invalid = qfalse;
			// note: this isn't done in Sys_ListFilteredFiles()

			for (i = 0; i < strlen(findinfo.name); i++)
			{
				if (findinfo.name[i] <= 31 || findinfo.name[i] == 127)
				{
					Com_Printf("ERROR: invalid char in name of file '%s'.\n", findinfo.name);
					invalid = qtrue;
					break;
				}
			}

			if (invalid)
			{
				remove(va("%s%c%s", directory, PATH_SEP, findinfo.name));
				Sys_Error("Invalid character in filename '%s'. The file has been removed. Start the server again.", findinfo.name);

				continue; // never add invalid files
			}
#endif

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

/**
 * @brief Sys_FreeFileList
 * @param[in,out] list
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

/**
 * @brief Block execution for msec or until input is received.
 * @param msec
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

/**
 * @brief Display an error message
 * @param[in] error
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
			size_t size;

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

/**
 * @brief Display a win32 dialog box
 * @param[in] type
 * @param[in] message
 * @param[in] title
 * @return
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

/**
 * @brief Windows specific "safe" GL implementation initialisation
 */
void Sys_GLimpSafeInit(void)
{
#ifndef DEDICATED
	if (!SDL_VIDEODRIVER_externallySet)
	{
		// Here, we want to let SDL decide what do to unless
		// explicitly requested otherwise
		Sys_SetEnv("SDL_VIDEODRIVER", "");
	}
#endif
}

/**
 * @brief Windows specific GL implementation initialisation
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
			Sys_SetEnv("SDL_VIDEODRIVER", "windib");
		}
#if 0
		else
		{
			// Use the DirectX SDL backend
			Sys_SetEnv("SDL_VIDEODRIVER", "directx");
		}
#endif
	}
#endif
}

/**
 * @brief Set/unset environment variables (empty value removes it)
 * @param name
 * @param value
 */
void Sys_SetEnv(const char *name, const char *value)
{
	_putenv(va("%s=%s", name, value));
}

/**
 * @brief Sys_PID
 * @return
 */
int Sys_PID(void)
{
	return GetCurrentProcessId();
}

/**
 * @brief Sys_PIDIsRunning
 * @param[in] pid
 * @return
 */
qboolean Sys_PIDIsRunning(unsigned int pid)
{
	DWORD        processes[1024];
	DWORD        numBytes, numProcesses;
	unsigned int i;

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

/**
 * @brief Sys_StartProcess
 * @param[in] cmdline
 * @param[in] doexit
 */
void Sys_StartProcess(char *cmdline, qboolean doexit)
{
	//TCHAR               szPathOrig[_MAX_PATH];
	STARTUPINFO         si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	//GetCurrentDirectory(_MAX_PATH, szPathOrig);

	if (!CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		// couldn't start it, popup error box
		Com_Error(ERR_DROP, "Could not start process: '%s'", cmdline);
		return;
	}

	// similar way of exiting as used in Sys_OpenURL below
	if (doexit)
	{
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
	}
}

/**
 * @brief Sys_Splash
 * @param[in] show
 */
void Sys_Splash(qboolean show)
{
#ifndef DEDICATED
	if (show)
	{
		if (g_wv.hWndSplash)
		{
			return;
		}

		g_wv.hWndSplash = CreateDialog(g_wv.hInstance, MAKEINTRESOURCE(IDD_SPLASH), NULL, NULL);
	}
	else
	{
		if (!g_wv.hWndSplash)
		{
			return;
		}

		ShowWindow(g_wv.hWndSplash, SW_HIDE);
		DestroyWindow(g_wv.hWndSplash);
		g_wv.hWndSplash = NULL;
	}
#endif
}

/**
 * @brief Sys_OpenURL
 * @param[in] url
 * @param[in] doexit
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

	Cbuf_ExecuteText(EXEC_NOW, "minimize");
#endif
}

/**
 * @brief Sys_CreateConsoleWindow
 */
void Sys_CreateConsoleWindow(void)
{
#ifndef DEDICATED
	int                        hConHandle;
	long                       lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE                       *fp;

	static qboolean consoleIsOpen = qfalse;

	if (consoleIsOpen)
	{
		FreeConsole();
		consoleIsOpen = qtrue;
		return;
	}
	else
	{
		consoleIsOpen = qtrue;
	}

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo))
	{
		coninfo.dwSize.Y = 9999;
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
	}

	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)(GetStdHandle(STD_OUTPUT_HANDLE));
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp         = _fdopen(hConHandle, "w");
	*stdout    = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	// redirect unbuffered STDIN to the console
	lStdHandle = (long)(GetStdHandle(STD_INPUT_HANDLE));
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp         = _fdopen(hConHandle, "r");
	*stdin     = *fp;

	setvbuf(stdin, NULL, _IONBF, 0);

	// redirect unbuffered STDERR to the console
	lStdHandle = (long)(GetStdHandle(STD_ERROR_HANDLE));
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp         = _fdopen(hConHandle, "w");
	*stderr    = *fp;

	setvbuf(stderr, NULL, _IONBF, 0);

	//SetConsoleTitle(WINDOWNAME);

	Sys_SetUpConsoleAndSignals();
#endif
}

// TODO: This could be enabled in the future
//#define SET_PROCESS_AFFINITY 1

/**
 * @brief Sys_SetProcessProperties
 */
void Sys_SetProcessProperties(void)
{
#ifdef SET_PROCESS_AFFINITY
	DWORD_PTR processAffinityMask;
	DWORD_PTR systemAffinityMask;
	int       core, bit, currentCore;
	BOOL      success;
#endif
	HANDLE process = GetCurrentProcess();

	//Set Process priority
	SetPriorityClass(process, HIGH_PRIORITY_CLASS);

#ifdef SET_PROCESS_AFFINITY
	if (!GetProcessAffinityMask(process, &processAffinityMask, &systemAffinityMask))
	{
		return;
	}

	//set this to the core you want your process to run on
	// probably could catch this from a cvar, but do we want people fucking arround with it
	core = SET_PROCESS_AFFINITY;
	for (bit = 0, currentCore = 1; bit < 64; bit++)
	{
		if (BIT(bit) & processAffinityMask)
		{
			if (currentCore != core)
			{
				CLEARBIT(processAffinityMask, bit);
			}

			currentCore++;
		}
	}

	success = SetProcessAffinityMask(process, processAffinityMask);
	if (success)
	{
		//Yup great
		Com_DPrintf(S_COLOR_GREEN "Sys_SetProcessProperties succesfully set process affinity");
	}
	else
	{
		//Fuck!
		Com_DPrintf(S_COLOR_RED "Sys_SetProcessProperties failed to set process affinity");
	}
#endif
}

WinVars_t g_wv;

/**
 * @brief Windows specific initialization
 */
void Sys_PlatformInit(void)
{
	g_wv.hInstance = GetModuleHandle(NULL);

#ifdef EXCEPTION_HANDLER
	WinSetExceptionVersion(Q3_VERSION);
#endif

#ifndef DEDICATED
	Sys_SetProcessProperties();

	//show the splash screen
	Sys_Splash(qtrue);
#endif

#ifdef USE_WINDOWS_CONSOLE
	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();
#endif

#ifdef DEDICATED
	Sys_ShowConsoleWindow(1, qtrue);
#endif

	// no abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

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
