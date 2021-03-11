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
 * @file sys_win32.c
 * @brief Contains windows-specific code for console.
 */

#ifndef DEDICATED
#   include <SDL2/SDL_video.h>
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

#if defined(LEGACY_DUMP_MEMLEAKS)
#include <crtdbg.h>
#endif

// Used to determine where to store user-specific files
static char homePath[MAX_OSPATH] = { 0 };
//static jmp_buf sys_exitframe;
//static int     sys_retcode;
//static char    sys_exitstr[MAX_STRING_CHARS];

size_t Sys_WideCharArrayToString(wchar_t *array, char *buffer, size_t len)
{
	size_t length = WideCharToMultiByte(CP_UTF8, 0, array, -1, NULL, 0, NULL, NULL);
	if(length > len)
	{
		return -1;
	}

	WideCharToMultiByte(CP_UTF8, 0, array, -1, buffer, len, NULL, NULL);
	return strlen(buffer);
}

/**
 * Converts the input UTF-8 string to a windows UTF-16 wchar_t array
 * @param [in] string UTF-8 format data
 * @param [in,out] output wide char array for output data
 * @param [in] size of the output buffer
 * @return length of the string
 */
size_t Sys_StringToWideCharArray(const char* string, wchar_t *output, size_t len)
{
	size_t length = 0;

	length = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
	if(length > len)
	{
		return -1;
	}

	MultiByteToWideChar(CP_UTF8, 0, string, -1, output, length);
	return length;
}

void *Sys_LoadLibrary(const char *library)
{
	wchar_t libPath[MAX_OSPATH];
	Sys_StringToWideCharArray(library, libPath, MAX_OSPATH);
	return LoadLibraryW(libPath);
}

/**
 * @return homepath pointing to "My Documents\\ETLegacy"
 */
char *Sys_DefaultHomePath(void)
{
	wchar_t w_szPath[MAX_PATH];
	char    szPath[MAX_PATH];
	HRESULT found;

	if (!*homePath /*&& !com_homepath*/)
	{
		// FIXME: forcing SHGFP_TYPE_CURRENT because file creation fails
		//        when real CSIDL_PERSONAL is on a mapped drive
		// NOTE: SHGetFolderPath is marked as deprecated
		found = SHGetFolderPathW(NULL, CSIDL_PERSONAL,
		                        NULL, SHGFP_TYPE_CURRENT, w_szPath);

		if (found != S_OK)
		{
			Com_Printf("Unable to detect CSIDL_PERSONAL\n");
			return NULL;
		}

		Sys_WideCharArrayToString(w_szPath, szPath, MAX_PATH);

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
	static wchar_t   w_userName[MAX_PATH];
	static char   s_userName[MAX_PATH];
	DWORD size = MAX_PATH;

	if (GetUserNameW(w_userName, &size))
	{
		Sys_WideCharArrayToString(w_userName, s_userName, MAX_PATH);
	}
	else
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
	static char base[MAX_OSPATH] = { 0 };
	size_t      length;

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
	static char dir[MAX_OSPATH] = { 0 };
	size_t      length;

	Q_strncpyz(dir, path, sizeof(dir));
	length = strlen(dir) - 1;

	while (length > 0 && dir[length] != '\\')
		length--;

	dir[length] = '\0';

	return dir;
}

/**
 * @brief Creates an absolute path name for the specified relative path name
 * @param path relative path
 * @return absolute path (allocated string) or NULL if path does not exist
 */
char *Sys_RealPath(const char *path)
{
	wchar_t w_fullOsPath[_MAX_PATH], w_partialPath[_MAX_PATH];
	Sys_StringToWideCharArray(path, w_partialPath, _MAX_PATH);

	if (_wfullpath(w_fullOsPath, w_partialPath, _MAX_PATH) != NULL)
	{
		char *output = Com_Allocate(sizeof(char) * _MAX_PATH);
		if (!output)
		{
			Com_DPrintf("Failed to allocate buffer for realpath\n");
			return NULL;
		}
		output[0] = '\0';

		Sys_WideCharArrayToString(w_fullOsPath, output, _MAX_PATH);
		return output;
	}
	else
	{
		Com_DPrintf("Failed to fullpath a relative path: %s\n", path);
		return NULL;
	}
}

/**
 * @brief Sys_FOpen
 * @param[in] ospath
 * @param[in] mode
 * @return
 */
FILE *Sys_FOpen(const char *ospath, const char *mode)
{
	size_t length;
	wchar_t w_ospath[MAX_OSPATH];
	wchar_t w_mode[10];

	// Windows API ignores all trailing spaces and periods which can get around Quake 3 file system restrictions.
	length = strlen(ospath);
	if (length == 0 || ospath[length - 1] == ' ' || ospath[length - 1] == '.')
	{
		return NULL;
	}

	Sys_StringToWideCharArray(ospath, w_ospath, MAX_OSPATH);
	Sys_StringToWideCharArray(mode, w_mode, 10);

	return _wfopen(w_ospath, w_mode);
}

/**
 * @brief Sys_Mkdir
 * @param[in] path
 * @return
 */
qboolean Sys_Mkdir(const char *path)
{
	wchar_t w_path[MAX_OSPATH];
	Sys_StringToWideCharArray(path, w_path, MAX_OSPATH);

	if (!CreateDirectoryW(w_path, NULL))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			return qfalse;
		}
	}

	return qtrue;
}

int Sys_Remove(const char *path)
{
	wchar_t w_path[MAX_OSPATH];
	Sys_StringToWideCharArray(path, w_path, MAX_OSPATH);
	return _wremove(w_path);
}

int Sys_RemoveDir(const char *path)
{
	wchar_t w_path[MAX_OSPATH];
	Sys_StringToWideCharArray(path, w_path, MAX_OSPATH);
	return _wrmdir(w_path);
}

int Sys_Stat(const char *path, void *stat)
{
	wchar_t w_path[MAX_OSPATH];
	Sys_StringToWideCharArray(path, w_path, MAX_OSPATH);
	return _wstat(w_path, stat);
}

int Sys_Rename(const char *from, const char *to)
{
	wchar_t w_from[MAX_OSPATH], w_to[MAX_OSPATH];
	Sys_StringToWideCharArray(from, w_from, MAX_OSPATH);
	Sys_StringToWideCharArray(to, w_to, MAX_OSPATH);
	return _wrename(w_from, w_to);
}

/**
 * @brief Sys_Cwd
 * @return
 */
char *Sys_Cwd(void)
{
	static wchar_t w_cwd[MAX_OSPATH];
	static char cwd[MAX_OSPATH];

	if (_wgetcwd(w_cwd, MAX_OSPATH - 1) == NULL)
	{
		Com_Error(ERR_FATAL, "Could not get the working directory");
		return NULL;
	}
	w_cwd[MAX_OSPATH - 1] = 0;

	Sys_WideCharArrayToString(w_cwd, cwd, MAX_OSPATH);

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
	wchar_t            w_search[MAX_OSPATH];
	char               tmpFilename[MAX_OSPATH], filename[MAX_OSPATH];
	intptr_t           findhandle;
	struct _wfinddata_t findinfo;

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

	Sys_StringToWideCharArray(search, w_search, MAX_OSPATH);
	findhandle = _wfindfirst(w_search, &findinfo);
	if (findhandle == -1)
	{
		return;
	}

	do
	{
		Sys_WideCharArrayToString(findinfo.name, tmpFilename, MAX_OSPATH);
		if (findinfo.attrib & _A_SUBDIR)
		{
			if (Q_stricmp(tmpFilename, ".") && Q_stricmp(tmpFilename, ".."))
			{
				if (strlen(subdirs))
				{
					Com_sprintf(newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, tmpFilename);
				}
				else
				{
					Com_sprintf(newsubdirs, sizeof(newsubdirs), "%s", tmpFilename);
				}

				Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
			}
		}
		if (*numfiles >= MAX_FOUND_FILES - 1)
		{
			break;
		}
		Com_sprintf(filename, sizeof(filename), "%s\\%s", subdirs, tmpFilename);
		if (!Com_FilterPath(filter, filename, qfalse))
		{
			continue;
		}

		list[*numfiles] = CopyString(filename);
		(*numfiles)++;
	}
	while (_wfindnext(findhandle, &findinfo) != -1);

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
	wchar_t            w_search[MAX_OSPATH];
	char               tmpFilename[MAX_OSPATH];
	int                nfiles;
	char               **listCopy;
	char               *list[MAX_FOUND_FILES];
	struct _wfinddata_t findinfo;
	intptr_t           findhandle;
	int                flag;
	int                i;
	size_t             extLen;
	qboolean           invalid;

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

	Sys_StringToWideCharArray(search, w_search, MAX_OSPATH);
	findhandle = _wfindfirst(w_search, &findinfo);
	if (findhandle == -1)
	{
		*numfiles = 0;
		return NULL;
	}

	do
	{
		Sys_WideCharArrayToString(findinfo.name, tmpFilename, MAX_OSPATH);

		if ((!wantsubs && (flag ^ (findinfo.attrib & _A_SUBDIR))) || (wantsubs && (findinfo.attrib & _A_SUBDIR)))
		{
			if (*extension)
			{
				if (strlen(tmpFilename) < extLen || Q_stricmp(tmpFilename + strlen(tmpFilename) - extLen, extension))
				{
					continue; // didn't match
				}
			}

			// check for bad file names and don't add these
			invalid = qfalse;
			// note: this isn't done in Sys_ListFilteredFiles()

			for (i = 0; i < strlen(tmpFilename); i++)
			{
				if (tmpFilename[i] <= 31 || tmpFilename[i] >= 127)
				{
					Com_Printf(S_COLOR_RED "ERROR: invalid char in name of file '%s'.\n", tmpFilename);
					invalid = qtrue;
					break;
				}
			}

			if (invalid)
			{
				int error;

				error = remove(va("%s%c%s", directory, PATH_SEP, tmpFilename));

				if (error != 0)
				{
					Com_Printf(S_COLOR_RED "ERROR: cannot delete '%s'.\n", tmpFilename);
				}
#ifdef DEDICATED
				Sys_Error("Invalid character in file name '%s'. The file has been removed. Start the server again.", tmpFilename);
#else
				Cvar_Set("com_missingFiles", "");
				Com_Error(ERR_DROP, "Invalid file name detected and removed\nFile \"%s\" contains an invalid character for ET: Legacy file structure.\nSome admins take advantage of this to ensure their menu loads last.\nThe file has been removed.", tmpFilename);
#endif
			}

			if (nfiles == MAX_FOUND_FILES - 1)
			{
				break;
			}

			list[nfiles] = CopyString(tmpFilename);
			nfiles++;
		}
	}
	while (_wfindnext(findhandle, &findinfo) != -1);

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
			char   *p = clipMemory;
			char   buffer[1024];
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

/**
 * @brief Windows specific "safe" GL implementation initialisation
 */
void Sys_GLimpSafeInit(void)
{
	// NOP
}

/**
 * @brief Windows specific GL implementation initialisation
 */
void Sys_GLimpInit(void)
{
	// NOP
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
 * @brief Sys_OpenURL
 * @param[in] url
 * @param[in] doexit
 */
void Sys_OpenURL(const char *url, qboolean doexit)
{
#ifndef DEDICATED
	HWND wnd;
	wchar_t tmpUrl[MAX_PATH * 2];

	static qboolean doexit_spamguard = qfalse;

	if (doexit_spamguard)
	{
		Com_DPrintf("Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url);
		return;
	}

	Com_Printf("Open URL: %s\n", url);

	Sys_StringToWideCharArray(url, tmpUrl, MAX_PATH * 2);
	if (!ShellExecuteW(NULL, L"open", tmpUrl, NULL, NULL, SW_RESTORE))
	{
		// couldn't start it, popup error box
		Com_Error(ERR_DROP, "Could not open url: '%s' ", url);
		return;
	}

	wnd = GetForegroundWindow();

	if (wnd && IsIconic(wnd))
	{
		ShowWindow(wnd, SW_SHOW);
	}

	if (doexit)
	{
		// show_bug.cgi?id=612
		doexit_spamguard = qtrue;
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
	}
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

#if defined(LEGACY_DUMP_MEMLEAKS)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif

#ifndef DEDICATED
	Sys_SetProcessProperties();
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
}

void Sys_PlatformExit(int code)
{
#if defined(LEGACY_DUMP_MEMLEAKS)
	_CrtDumpMemoryLeaks();
#endif
	exit(code);
}

/**
 * @brief Check if filename should be allowed to be loaded as a DLL.
 * @param name
 */
qboolean Sys_DllExtension(const char *name)
{
	return COM_CompareExtension(name, DLL_EXT);
}
