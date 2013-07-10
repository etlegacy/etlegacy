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
 * @file sys_main.c
 */

#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __AROS__
#include <proto/dos.h>
#endif

#ifndef DEDICATED
#    ifdef BUNDLED_SDL
#        include "SDL.h"
#        include "SDL_cpuinfo.h"
#    else
#        include <SDL/SDL.h>
#        include <SDL/SDL_cpuinfo.h>
#    endif
#endif

#include "sys_local.h"
#include "sys_loadlib.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

static char binaryPath[MAX_OSPATH] = { 0 };
static char installPath[MAX_OSPATH] = { 0 };

#ifdef FEATURE_CURSES
static qboolean nocurses = qfalse;
void CON_Init_tty(void);
#endif

/*
=================
Sys_SetBinaryPath
=================
*/
void Sys_SetBinaryPath(const char *path)
{
#ifdef __AROS__
	if (!strcmp(path, "."))
	{
		NameFromLock(GetProgramDir(), binaryPath, sizeof(binaryPath));
	}
	else
#endif
	{
		Q_strncpyz(binaryPath, path, sizeof(binaryPath));
	}
}

/*
=================
Sys_BinaryPath
=================
*/
char *Sys_BinaryPath(void)
{
	return binaryPath;
}

/*
=================
Sys_SetDefaultInstallPath
=================
*/
void Sys_SetDefaultInstallPath(const char *path)
{
	Q_strncpyz(installPath, path, sizeof(installPath));
}

/*
=================
Sys_DefaultInstallPath
=================
*/
char *Sys_DefaultInstallPath(void)
{
	if (*installPath)
	{
		return installPath;
	}
	else
	{
		return Sys_Cwd();
	}
}

/**
 * @brief Restart the input subsystem
 */
void Sys_In_Restart_f(void)
{
	IN_Restart();
}

/**
 * @brief Handle new console input
 */
#if !defined (_WIN32)
char *Sys_ConsoleInput(void)
{
	return CON_Input();
}
#elif defined (__linux__)
char *Sys_ConsoleInput(void)
{
	return CON_Input();
}
#endif

/**
 * @brief Writes pid to profile or to the homepath root if running a server
 * @retval qtrue  if pid file successfully created
 * @retval qfalse if it wasn't possible to create a new pid file
 */
extern qboolean FS_FileInPathExists(const char *testpath);

qboolean Sys_WritePIDFile(void)
{
	fileHandle_t f;

	// First, check if the pid file is already there
	if (FS_FileInPathExists(com_pidfile->string))
	{
		// TODO: check if we are hijacking live pid file
		/*
		FS_FOpenFileRead(com_pidfile->string, &f, qtrue);

		if(Sys_PIDIsRunning(pid))
		{
		    Com_Printf("WARNING: another instance of ET:L is using this path!\n");
		    return qfalse;
		}
		*/
		FS_Delete(com_pidfile->string); // stale pid from previous run
	}

	f = FS_FOpenFileWrite(com_pidfile->string);
	if (f < 0)
	{
		return qfalse;
	}

	FS_Printf(f, "%d", com_pid->integer);

	FS_FCloseFile(f);

	// track profile changes
	Com_TrackProfile(com_pidfile->string);

	return qtrue;
}

/**
 * @brief Single exit point (regular exit or in case of error)
 */
static __attribute__ ((noreturn)) void Sys_Exit(int exitCode)
{
	CON_Shutdown();

#ifndef DEDICATED
	SDL_Quit();
#endif

	if (exitCode < 2)
	{
		// Normal exit
		if (FS_FileExists(com_pidfile->string))
		{
			// FIXME: delete even when outside of homepath
			remove(va("%s%c%s%c%s", Cvar_VariableString("fs_homepath"),
			          PATH_SEP, Cvar_VariableString("fs_game"),
			          PATH_SEP, com_pidfile->string));
		}
	}

	exit(exitCode);
}

/*
=================
Sys_Quit
=================
*/
void Sys_Quit(void)
{
#ifdef __AROS__
	NET_Shutdown();
#endif
	Sys_Exit(0);
#if defined (_WIN32)
	Sys_DestroyConsole();
#endif
}

/*
=================
Sys_GetProcessorFeatures
=================
*/
cpuFeatures_t Sys_GetProcessorFeatures(void)
{
	cpuFeatures_t features = 0;

#ifndef DEDICATED
	if (SDL_HasRDTSC())
	{
		features |= CF_RDTSC;
	}
	if (SDL_HasMMX())
	{
		features |= CF_MMX;
	}
	if (SDL_HasMMXExt())
	{
		features |= CF_MMX_EXT;
	}
	if (SDL_Has3DNow())
	{
		features |= CF_3DNOW;
	}
	if (SDL_Has3DNowExt())
	{
		features |= CF_3DNOW_EXT;
	}
	if (SDL_HasSSE())
	{
		features |= CF_SSE;
	}
	if (SDL_HasSSE2())
	{
		features |= CF_SSE2;
	}
	if (SDL_HasAltiVec())
	{
		features |= CF_ALTIVEC;
	}
#endif

	return features;
}

/*
=================
Sys_Init
=================
*/
#if defined (_WIN32)
extern void Sys_ClearViewlog_f(void);
#endif

void Sys_Init(void)
{
	Cmd_AddCommand("in_restart", Sys_In_Restart_f);
#if defined (_WIN32)
	Cmd_AddCommand("clearviewlog", Sys_ClearViewlog_f);
#endif

	Cvar_Set("arch", OS_STRING " " ARCH_STRING);
	Cvar_Set("username", Sys_GetCurrentUser());
}

/**
 * @brief Transform Q3 colour codes to ANSI escape sequences
 */
void Sys_AnsiColorPrint(const char *msg)
{
	static char buffer[MAXPRINTMSG];
	int         i, j, _found, length = 0;

	// colors hash from http://wolfwiki.anime.net/index.php/Color_Codes
	static int etAnsiHash[][6] =
	{
		// here should be black, but it's invisible in terminal
		// so you see dark grey here. like in 9 color
		{ 1,  30, 0,   '0', 'p',  'P'  },
		{ 1,  31, 0,   '1', 'q',  'Q'  },
		{ 1,  32, 0,   '2', 'r',  'R'  },
		{ 1,  33, 0,   '3', 's',  'S'  },
		{ 1,  34, 0,   '4', 't',  'T'  },
		{ 1,  36, 0,   '5', 'u',  'U'  },
		{ 1,  35, 0,   '6', 'v',  'V'  },
		{ 1,  37, 0,   '7', 'w',  'W'  },

		{ 38, 5,  208, '8', 'x',  'X'  },
		{ 1,  30, 0,   '9', 'y',  'Y'  },
		{ 0,  37, 0,   'z', 'Z',  ':'  }, // the same
		{ 0,  37, 0,   '[', '{',  ';'  }, // --------
		{ 0,  32, 0,   '<', '\\', '|'  },
		{ 0,  33, 0,   '=', ']',  '}'  },
		{ 0,  34, 0,   '>', '~',  0    },
		{ 0,  31, 0,   '?', '_',  0    },
		{ 38, 5,  94,  '@', '`',  0    },

		{ 38, 5,  214, 'a', 'A',  '!'  },
		{ 38, 5,  30,  'b', 'B',  '"'  },
		{ 38, 5,  90,  'c', 'C',  '#'  },
		{ 38, 5,  33,  'd', 'D',  '$'  },
		{ 38, 5,  93,  'e', 'E',  '%'  },
		{ 38, 5,  38,  'f', 'F',  '&'  },
		{ 38, 5,  194, 'g', 'G',  '\'' },
		{ 38, 5,  29,  'h', 'H',  '('  },

		{ 38, 5,  197, 'i', 'I',  ')'  },
		{ 38, 5,  124, 'j', 'J',  '*'  },
		{ 38, 5,  130, 'k', 'K',  '+'  },
		{ 38, 5,  179, 'l', 'L',  ','  },
		{ 38, 5,  143, 'm', 'M',  '-'  },
		{ 38, 5,  229, 'n', 'N',  '.'  },
		{ 38, 5,  228, 'o', 'O',  '/'  }
	};

	while (*msg)
	{
		if (Q_IsColorString(msg) || *msg == '\n' || *msg == '\\')
		{
			// First empty the buffer
			if (length > 0)
			{
				buffer[length] = '\0';
				fputs(buffer, stderr);
				length = 0;
			}

			if (*msg == '\n')
			{
				// Issue a reset and then the newline
				fputs("\033[0m\n", stderr);
				msg++;
			}
			else if (*msg == '\\')
			{
				fputs("\033[0m\\", stderr);
				msg++;
			}
			else if (*(msg + 1) == '7' && (*(msg + 2) == '"' || *(msg + 2) == '\'' || *(msg + 2) == '\0'))
			{
				fputs("\033[0m", stderr);
				msg += 2;
			}
			else
			{
				// Print the color code
				_found = 0;
				for (i = 0; i < 33; i++)
				{
					if (_found)
					{
						break;
					}

					for (j = 3; j < 6; j++)
					{
						if (etAnsiHash[i][j] == 0)
						{
							break;
						}

						if (*(msg + 1) == etAnsiHash[i][j])
						{
							if (etAnsiHash[i][2] == 0)
							{
								Com_sprintf(buffer, sizeof(buffer), "\033[%d;%dm",
								            etAnsiHash[i][0], etAnsiHash[i][1]);
								_found = 1;
								break;
							}
							else
							{
								Com_sprintf(buffer, sizeof(buffer), "\033[%d;%d;%dm",
								            etAnsiHash[i][0], etAnsiHash[i][1], etAnsiHash[i][2]);
								_found = 1;
								break;
							}
						}
					}
				}
				fputs(buffer, stderr);
				msg += 2;
			}
		}
		else
		{
			if (length >= MAXPRINTMSG - 1)
			{
				break;
			}

			buffer[length] = *msg;
			length++;
			msg++;
		}
	}

	// Empty anything still left in the buffer
	if (length > 0)
	{
		buffer[length] = '\0';
		fputs(buffer, stderr);
	}
}

/*
=================
Sys_Print
=================
*/
void Sys_Print(const char *msg)
{
#if defined (_WIN32)
	Conbuf_AppendText(msg);
#else
	CON_LogWrite(msg);
	CON_Print(msg);
#endif
}

/*
=================
Sys_Error
=================
*/
void Sys_Error(const char *error, ...)
{
	va_list argptr;
	char    string[1024];
#if defined (_WIN32)
	MSG msg;
#endif

	va_start(argptr, error);
	Q_vsnprintf(string, sizeof(string), error, argptr);
	va_end(argptr);

#if defined (_WIN32)
	Conbuf_AppendText(string);
	Conbuf_AppendText("\n");

	Sys_SetErrorText(string);
	Sys_ShowConsole(1, qtrue);

	timeEndPeriod(1);

	IN_Shutdown();

	// wait for the user to quit
	while (1)
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			Com_Quit_f();
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Sys_DestroyConsole();
#endif

	Sys_ErrorDialog(string);

	Sys_Exit(3);
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime(char *path)
{
	struct stat buf;

	if (stat(path, &buf) == -1)
	{
		return -1;
	}

	return buf.st_mtime;
}

/*
=================
Sys_UnloadDll
=================
*/
void Sys_UnloadDll(void *dllHandle)
{
	if (!dllHandle)
	{
		Com_Printf("Sys_UnloadDll(NULL)\n");
		return;
	}

	Sys_UnloadLibrary(dllHandle);
}

/*
=================
Sys_LoadDll

First try to load library name from system library path,
from executable path, then fs_basepath.
=================
*/

void *Sys_LoadDll(const char *name, qboolean useSystemLib)
{
	void *dllhandle;

	if (useSystemLib)
	{
		Com_Printf("Trying to load \"%s\"...\n", name);
	}

	if (!useSystemLib || !(dllhandle = Sys_LoadLibrary(name)))
	{
		const char *topDir;
		char       libPath[MAX_OSPATH];

		topDir = Sys_BinaryPath();

		if (!*topDir)
		{
			topDir = ".";
		}

		Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, topDir);
		Com_sprintf(libPath, sizeof(libPath), "%s%c%s", topDir, PATH_SEP, name);

		if (!(dllhandle = Sys_LoadLibrary(libPath)))
		{
			const char *basePath = Cvar_VariableString("fs_basepath");

			if (!basePath || !*basePath)
			{
				basePath = ".";
			}

			if (FS_FilenameCompare(topDir, basePath))
			{
				Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, basePath);
				Com_sprintf(libPath, sizeof(libPath), "%s%c%s", basePath, PATH_SEP, name);
				dllhandle = Sys_LoadLibrary(libPath);
			}

			if (!dllhandle)
			{
				Com_Printf("Loading \"%s\" failed\n", name);
			}
		}
	}

	return dllhandle;
}

/**
 * @brief Used by Sys_LoadGameDll to get handle on a mod library
 * @return Handle to a mod library
 */
static void *Sys_TryLibraryLoad(const char *base, const char *gamedir, const char *fname)
{
	void *libHandle;
	char *fn;

	fn = FS_BuildOSPath(base, gamedir, fname);

#ifdef __APPLE__
	if (FS_Unzip(fn, qtrue))
	{
		char buffer[MAX_OSPATH];
		Com_sprintf(buffer, sizeof(buffer), "%s.bundle/Contents/MacOS/%s", fname, fname);
		fn = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), gamedir, buffer);
	}
#endif // __APPLE__

	Com_Printf("Sys_LoadDll(%s)... ", fn);

	libHandle = Sys_LoadLibrary(fn);

	if (!libHandle)
	{
		Com_Printf("failed: \"%s\"\n", Sys_LibraryError());
		return NULL;
	}
	Com_Printf("succeeded\n");

	return libHandle;
}

/**
 * @brief Loads a mod library.
 * #1 look in fs_homepath
 * #2 look in fs_basepath
 * #3 try to revert to the default mod library
 */
void *Sys_LoadGameDll(const char *name,
                      intptr_t(**entryPoint) (int, ...),
                      intptr_t (*systemcalls)(intptr_t, ...))
{
	void *libHandle;
	void (*dllEntry)(intptr_t (*syscallptr)(intptr_t, ...));
	char fname[MAX_OSPATH];
	char *basepath;
	char *homepath;
	char *gamedir;

	assert(name);

	Com_sprintf(fname, sizeof(fname), Sys_GetDLLName("%s"), name);

	// TODO: use fs_searchpaths from files.c
	basepath = Cvar_VariableString("fs_basepath");
	homepath = Cvar_VariableString("fs_homepath");
	gamedir  = Cvar_VariableString("fs_game");

#ifndef DEDICATED
	// if the server is pure, extract the dlls from the mp_bin.pk3 so
	// that they can be referenced
	if (Cvar_VariableValue("sv_pure") && Q_stricmp(name, "qagame"))
	{
		FS_CL_ExtractFromPakFile(homepath, gamedir, fname);
	}
#endif

	libHandle = Sys_TryLibraryLoad(homepath, gamedir, fname);

	if (!libHandle && basepath)
	{
		libHandle = Sys_TryLibraryLoad(basepath, gamedir, fname);
	}

	// HACK: sometimes a library is loaded from the mod dir when it shouldn't. Why?
	if (!libHandle && strcmp(gamedir, DEFAULT_MODGAME))
	{
		Com_Printf("Sys_LoadDll: failed to load the mod library. Trying to revert to the default one.\n");
		libHandle = Sys_TryLibraryLoad(basepath, DEFAULT_MODGAME, fname);
	}

	if (!libHandle)
	{
		Com_Printf("Sys_LoadDll(%s) failed to load library\n", name);
		return NULL;
	}

	dllEntry    = (void (QDECL *)(intptr_t (QDECL *)(intptr_t, ...)))Sys_LoadFunction(libHandle, "dllEntry");
	*entryPoint = (intptr_t (QDECL *)(int, ...))Sys_LoadFunction(libHandle, "vmMain");

	if (!*entryPoint || !dllEntry)
	{
		Com_Printf("Sys_LoadDll(%s) failed to find vmMain function:\n\"%s\" !\n", name, Sys_LibraryError());
		Sys_UnloadLibrary(libHandle);

		return NULL;
	}

	Com_Printf("Sys_LoadDll(%s) found vmMain function at %p\n", name, *entryPoint);
	dllEntry(systemcalls);

	return libHandle;
}

/*
=================
Sys_ParseArgs
=================
*/
void Sys_ParseArgs(int argc, char **argv)
{
#ifdef FEATURE_CURSES
	int i;
#endif

	if (argc == 2)
	{
		if (!strcmp(argv[1], "--version") ||
		    !strcmp(argv[1], "-v"))
		{
#ifdef DEDICATED
			fprintf(stdout, Q3_VERSION " " CPUSTRING " dedicated server (%s)\n", __DATE__);
#else
			fprintf(stdout, "Client: " ET_VERSION "\n");
			fprintf(stdout, "Masked as: " FAKE_VERSION "\n");
#endif
			Sys_Exit(0);
		}
	}
#ifdef FEATURE_CURSES
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "+nocurses") ||
		    !strcmp(argv[i], "--nocurses"))
		{
			nocurses = qtrue;
			break;
		}
	}
#endif
}

#ifndef DEFAULT_BASEDIR
#       define DEFAULT_BASEDIR Sys_BinaryPath()
#endif

/*
=================
Sys_SigHandler
=================
*/
void Sys_SigHandler(int signal)
{
	static qboolean signalcaught = qfalse;

	if (signalcaught)
	{
		fprintf(stderr, "DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n",
		        signal);
	}
	else
	{
		signalcaught = qtrue;
#ifndef DEDICATED
		CL_Shutdown();
#endif
		SV_Shutdown(va("Received signal %d", signal));
	}

	if (signal == SIGTERM || signal == SIGINT)
	{
		Sys_Exit(1);
	}
	else
	{
		Sys_Exit(2);
	}
}

/*
=================
main
=================
*/
int main(int argc, char **argv)
{
	int  i;
	char commandLine[MAX_STRING_CHARS] = { 0 };

#ifndef DEDICATED
	// SDL version check

	// Compile time
#   if !SDL_VERSION_ATLEAST(MINSDL_MAJOR, MINSDL_MINOR, MINSDL_PATCH)
#       error A more recent version of SDL is required
#   endif

	// Run time
	const SDL_version *ver = SDL_Linked_Version();

#define MINSDL_VERSION \
	XSTRING(MINSDL_MAJOR) "." \
	XSTRING(MINSDL_MINOR) "." \
	XSTRING(MINSDL_PATCH)

	if (SDL_VERSIONNUM(ver->major, ver->minor, ver->patch) <
	    SDL_VERSIONNUM(MINSDL_MAJOR, MINSDL_MINOR, MINSDL_PATCH))
	{
		Sys_Dialog(DT_ERROR, va("SDL version " MINSDL_VERSION " or greater is required, "
		                                                      "but only version %d.%d.%d was found. You may be able to obtain a more recent copy "
		                                                      "from http://www.libsdl.org/.", ver->major, ver->minor, ver->patch), "SDL Library Too Old");

		Sys_Exit(1);
	}
#endif

	Sys_PlatformInit();

	// Set the initial time base
	Sys_Milliseconds();

	Sys_ParseArgs(argc, argv);
	Sys_SetBinaryPath(Sys_Dirname(argv[0]));
	Sys_SetDefaultInstallPath(DEFAULT_BASEDIR);

	// Concatenate the command line for passing to Com_Init
	for (i = 1; i < argc; i++)
	{
		const qboolean containsSpaces = (qboolean)(strchr(argv[i], ' ') != NULL);
		if (containsSpaces)
		{
			Q_strcat(commandLine, sizeof(commandLine), "\"");
		}

		Q_strcat(commandLine, sizeof(commandLine), argv[i]);

		if (containsSpaces)
		{
			Q_strcat(commandLine, sizeof(commandLine), "\"");
		}

		Q_strcat(commandLine, sizeof(commandLine), " ");
	}

	Com_Init(commandLine);
	NET_Init();

#ifdef FEATURE_CURSES
	if (nocurses)
	{
		CON_Init_tty();
	}
	else
	{
		CON_Init();
	}
#else
	CON_Init();
#endif

	signal(SIGILL, Sys_SigHandler);
	signal(SIGFPE, Sys_SigHandler);
	signal(SIGSEGV, Sys_SigHandler);
	signal(SIGTERM, Sys_SigHandler);
	signal(SIGINT, Sys_SigHandler);

	while (1)
	{
		IN_Frame();
		Com_Frame();
	}

	return 0;
}
