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
  * @file sys_win32_dpi.c
  */

#include "../qcommon/q_shared.h"
#include "../sys/sys_win32.h"

// Avoid needing to include win header that may not be available
typedef enum PROCESS_DPI_AWARENESS
{
	PROCESS_DPI_UNAWARE           = 0,
	PROCESS_SYSTEM_DPI_AWARE      = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

typedef LONG (WINAPI *RtlGetVersionPtr)(RTL_OSVERSIONINFOEXW *);
typedef BOOL (WINAPI *SetProcessDpiAwarenessContextPtr)(HANDLE);
typedef HRESULT (WINAPI *SetProcessDpiAwarenessPtr)(int);
typedef BOOL (WINAPI *SetProcessDPIAwarePtr)(void);

#define ETL_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)

static qboolean Sys_EnablePerMonitorV2(void);
static qboolean Sys_IsWin10(void);
static qboolean Sys_EnablePerMonitor(void);
static qboolean Sys_EnableDPIAware(void);
void            Sys_SetupDPIAwareness(void);
