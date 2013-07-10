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
 * @file sys_local.h
 */

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

// Require a minimum version of SDL
#define MINSDL_MAJOR 1
#define MINSDL_MINOR 2
#define MINSDL_PATCH 10

// Input subsystem
void IN_Init(void);
void IN_Frame(void);
void IN_Shutdown(void);
void IN_Restart(void);

// Console
void CON_Shutdown(void);
void CON_Init(void);
char *CON_Input(void);
void CON_Print(const char *message);

unsigned int CON_LogSize(void);
unsigned int CON_LogWrite(const char *in);
unsigned int CON_LogRead(char *out, unsigned int outSize);

void Sys_GLimpSafeInit(void);
void Sys_GLimpInit(void);
void Sys_PlatformInit(void);
void Sys_SigHandler(int signal) __attribute__ ((noreturn));
void Sys_ErrorDialog(const char *error);
void Sys_AnsiColorPrint(const char *msg);

void *Sys_LoadDll(const char *name, qboolean useSystemLib);
void *Sys_LoadGameDll(const char *name, intptr_t(**entryPoint) (int, ...), intptr_t (*systemcalls)(intptr_t, ...));
void Sys_UnloadDll(void *dllHandle);

#if defined(_WIN32)
void Conbuf_AppendText(const char *msg);
void Sys_DestroyConsole(void);
#endif
