/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file sys_local.h
 */

#ifndef INCLUDE_SYS_LOCAL_H
#define INCLUDE_SYS_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

// Input subsystem
void IN_Init(void);
void IN_Frame(void);
void IN_Shutdown(void);
void IN_Restart(void);

//SDL Window system
void *GLimp_MainWindow(void);
void GLimp_Minimize(void);
void GLimp_FlashWindow(int);

//Export to renderer
void GLimp_Init(glconfig_t *glConfig, const char *glConfigString);
void GLimp_Shutdown(void);
void GLimp_EndFrame(void);
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]);
qboolean GLimp_SplashImage(qboolean (*LoadSplashImage)(const char *name, byte *data, unsigned int size, unsigned int width, unsigned int height, uint8_t bytes));

// Console
void CON_Shutdown(void);
void CON_Init(void);
char *CON_Input(void);
void CON_Print(const char *msg);

unsigned int CON_LogSize(void);
unsigned int CON_LogWrite(const char *in);
unsigned int CON_LogRead(char *out, unsigned int outSize);

void Sys_GLimpSafeInit(void);
void Sys_GLimpInit(void);
void Sys_PlatformInit(void);

#ifdef _WIN32
void Sys_PlatformExit(int code) _attribute((noreturn));
#else
#define Sys_PlatformExit(x) exit(x);
#endif

void Sys_SigHandler(int signal);
void Sys_ErrorDialog(const char *error);
void Sys_AnsiColorPrint(const char *msg);

void *Sys_LoadDll(const char *name, qboolean useSystemLib);
// NOTE: arm64 mac has a different calling convention for fixed parameters vs. variadic parameters.
//       As the module entryPoints (vmMain) in jk2 use fixed arg0 to arg11 we can't use "..." around here or we end up with undefined behavior.
//       See: https://developer.apple.com/documentation/apple-silicon/addressing-architectural-differences-in-your-macos-code
void *Sys_LoadGameDll(const char *name, qboolean extract, VM_EntryPoint_t *entryPoint, intptr_t (*systemcalls)(intptr_t, ...));
void Sys_UnloadDll(void *dllHandle);
void Sys_ParseArgs(int argc, char **argv);
void Sys_BuildCommandLine(int argc, char **argv, char *buffer, size_t bufferSize);

#ifdef __ANDROID__
char *Sys_CdToExtStorage(void);
#endif

#ifdef USE_WINDOWS_CONSOLE
void Conbuf_AppendText(const char *msg);
void Sys_DestroyConsole(void);
#endif

void Sys_SetUpConsoleAndSignals(void);

#ifdef __APPLE__
const char *OSX_ApplicationSupportPath(void);
int OSX_NeedsQuarantineFix();
#endif

#endif // #ifndef INCLUDE_SYS_LOCAL_H
