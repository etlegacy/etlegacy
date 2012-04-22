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

#include <SDL/SDL.h>
#include <SDL/SDL_cpuinfo.h>

#include "sys_local.h"
#include "sys_loadlib.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

static char binaryPath[MAX_OSPATH] = { 0 };
static char installPath[MAX_OSPATH] = { 0 };

/*
=================
Sys_SetBinaryPath
=================
*/
void Sys_SetBinaryPath(const char *path)
{
	Q_strncpyz(binaryPath, path, sizeof(binaryPath));
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

/*
=================
Sys_DefaultAppPath
=================
*/
char *Sys_DefaultAppPath(void)
{
	return Sys_BinaryPath();
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f(void)
{
	IN_Restart();
}

/*
=================
Sys_ConsoleInput

Handle new console input
=================
*/
char *Sys_ConsoleInput(void)
{
	return CON_Input();
}

#ifdef DEDICATED
#   define PID_FILENAME PRODUCT_NAME "_server.pid"
#else
#   define PID_FILENAME PRODUCT_NAME ".pid"
#endif

/*
=================
Sys_PIDFileName
=================
*/
static char *Sys_PIDFileName(void)
{
	return va("%s/%s", Sys_TempPath(), PID_FILENAME);
}

/*
=================
Sys_WritePIDFile

Return qtrue if there is an existing stale PID file
=================
*/
qboolean Sys_WritePIDFile(void)
{
	char     *pidFile = Sys_PIDFileName();
	FILE     *f;
	qboolean stale = qfalse;

	// First, check if the pid file is already there
	if ((f = fopen(pidFile, "r")) != NULL)
	{
		char pidBuffer[64] = { 0 };
		int  pid;

		fread(pidBuffer, sizeof(char), sizeof(pidBuffer) - 1, f);
		fclose(f);

		pid = atoi(pidBuffer);
		if (!Sys_PIDIsRunning(pid))
		{
			stale = qtrue;
		}
	}

	if ((f = fopen(pidFile, "w")) != NULL)
	{
		fprintf(f, "%d", Sys_PID());
		fclose(f);
	}
	else
	{
		Com_Printf(S_COLOR_YELLOW "Couldn't write %s.\n", pidFile);
	}

	return stale;
}

/*
=================
Sys_Exit

Single exit point (regular exit or in case of error)
=================
*/
static void Sys_Exit(int exitCode)
{
	CON_Shutdown();

#ifndef DEDICATED
	SDL_Quit();
#endif

	if (exitCode < 2)
	{
		// Normal exit
		remove(Sys_PIDFileName());
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
	Sys_Exit(0);
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
void Sys_Init(void)
{
	Cmd_AddCommand("in_restart", Sys_In_Restart_f);
	Cvar_Set("arch", OS_STRING " " ARCH_STRING);
	Cvar_Set("username", Sys_GetCurrentUser());
}

/*
=================
Sys_AnsiColorPrint

Transform Q3 colour codes to ANSI escape sequences
=================
*/
void Sys_AnsiColorPrint(const char *msg)
{
	static char buffer[MAXPRINTMSG];
	int         length      = 0;

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
				int i, j, _found = 0;
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
	CON_LogWrite(msg);
	CON_Print(msg);
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

	va_start(argptr, error);
	Q_vsnprintf(string, sizeof(string), error, argptr);
	va_end(argptr);

	CL_Shutdown();
	Sys_ErrorDialog(string);

	Sys_Exit(3);
}

/*
=================
Sys_Warn
=================
*/
void Sys_Warn(char *warning, ...)
{
	va_list argptr;
	char    string[1024];

	va_start(argptr, warning);
	Q_vsnprintf(string, sizeof(string), warning, argptr);
	va_end(argptr);

	CON_Print(va("Warning: %s", string));
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
Sys_TryLibraryLoad
=================
*/
static void *Sys_TryLibraryLoad(const char *base, const char *gamedir, const char *fname, char *fqpath)
{
	void *libHandle;
	char *fn;

	*fqpath = 0;

	fn = FS_BuildOSPath(base, gamedir, fname);
	Com_Printf("Sys_LoadDll(%s)... \n", fn);

	libHandle = Sys_LoadLibrary(fn);

	if (!libHandle)
	{
		Com_Printf("Sys_LoadDll(%s) failed:\n\"%s\"\n", fn, Sys_LibraryError());
		return NULL;
	}

	Com_Printf("Sys_LoadDll(%s): succeeded ...\n", fn);
	Q_strncpyz(fqpath, fn, MAX_QPATH) ;

	return libHandle;
}

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
#1 look in fs_homepath
#2 look in fs_basepath
=================
*/
void *Sys_LoadDll(const char *name, char *fqpath,
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

	Q_snprintf(fname, sizeof(fname), Sys_GetDLLName("%s"), name);

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

	libHandle = Sys_TryLibraryLoad(homepath, gamedir, fname, fqpath);

	if (!libHandle && basepath)
	{
		libHandle = Sys_TryLibraryLoad(basepath, gamedir, fname, fqpath);
	}

	if (!libHandle)
	{
		Com_Printf("Sys_LoadDll(%s) failed to load library\n", name);
		return NULL;
	}

	dllEntry    = Sys_LoadFunction(libHandle, "dllEntry");
	*entryPoint = Sys_LoadFunction(libHandle, "vmMain");

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
	if (argc == 2)
	{
		if (!strcmp(argv[1], "--version") ||
		    !strcmp(argv[1], "-v"))
		{
#ifdef DEDICATED
			fprintf(stdout, Q3_VERSION " dedicated server (%s)\n", __DATE__);
#else
			fprintf(stdout, "Client: " ET_VERSION "\n");
			fprintf(stdout, "Masked as: " FAKE_VERSION "\n");
#endif
			Sys_Exit(0);
		}
	}
}

#ifndef DEFAULT_BASEDIR
#   ifdef MACOS_X
#       define DEFAULT_BASEDIR Sys_StripAppBundle(Sys_BinaryPath())
#   else
#       define DEFAULT_BASEDIR Sys_BinaryPath()
#   endif
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
		const qboolean containsSpaces = strchr(argv[i], ' ') != NULL;
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

	CON_Init();

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
