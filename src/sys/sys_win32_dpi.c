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

#include "sys_win32_dpi.h"

/**
* @brief Sys_EnablePerMonitorV2
* @return
*/
static qboolean Sys_EnablePerMonitorV2(void)
{
	SetProcessDpiAwarenessContextPtr set_process_dpi_awareness_context_f = NULL;
	HMODULE                          u32dll                              = GetModuleHandle(TEXT("user32"));

	if (!u32dll)
	{
		return qfalse;
	}

	set_process_dpi_awareness_context_f = (SetProcessDpiAwarenessContextPtr)GetProcAddress(u32dll, "SetProcessDpiAwarenessContext");

	if (set_process_dpi_awareness_context_f)
	{
		return set_process_dpi_awareness_context_f(ETL_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE ? qtrue : qfalse;
	}
	return qfalse;
}

/**
* @brief Sys_IsWin10
* @return
*/
static qboolean Sys_IsWin10(void)
{
	RtlGetVersionPtr     rtl_get_version_f = NULL;
	HMODULE              ntdll             = GetModuleHandle(TEXT("ntdll"));
	RTL_OSVERSIONINFOEXW osver;

	if (!ntdll)
	{
		return qfalse; // will never happen

	}
	rtl_get_version_f = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");

	if (!rtl_get_version_f)
	{
		return qfalse; // will never happen

	}
	osver.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

	if (rtl_get_version_f(&osver) == 0)
	{
		if (osver.dwMajorVersion >= 10)
		{
			return qtrue;
		}
	}

	return qfalse;
}


/**
* @brief Sys_EnablePerMonitor
* @return
*/
static qboolean Sys_EnablePerMonitor(void)
{
	SetProcessDpiAwarenessPtr set_process_dpi_awareness_f = NULL;
	HMODULE                   u32dll                      = GetModuleHandle(TEXT("user32"));
	qboolean                  win10                       = qfalse;

	if (!u32dll)
	{
		return qfalse;
	}

	set_process_dpi_awareness_f = (SetProcessDpiAwarenessPtr)GetProcAddress(u32dll, "SetProcessDpiAwarenessInternal");

	win10 = Sys_IsWin10();

	if (set_process_dpi_awareness_f)
	{
		HRESULT hr = set_process_dpi_awareness_f(win10 ? PROCESS_PER_MONITOR_DPI_AWARE : PROCESS_SYSTEM_DPI_AWARE);

		if (SUCCEEDED(hr))
		{
			return qtrue;
		}
		else if (hr == E_ACCESSDENIED)
		{
			// This can happen when function is called more than once or there is a manifest override
			// Definitely should be logging this
		}
	}
	return qfalse;
}

/**
* @brief Sys_EnableDPIAware
* @return
*/
static qboolean Sys_EnableDPIAware(void)
{
	SetProcessDPIAwarePtr set_process_dpi_aware_f = NULL;
	HMODULE               u32dll                  = GetModuleHandle(TEXT("user32"));

	if (!u32dll)
	{
		return qfalse;
	}

	set_process_dpi_aware_f = (SetProcessDPIAwarePtr)GetProcAddress(u32dll, "SetProcessDPIAware");

	if (set_process_dpi_aware_f)
	{
		return set_process_dpi_aware_f() != FALSE ? qtrue : qfalse;
	}
	return qfalse;
}

/**
* @brief Sys_SetupDPIAwareness
* @return
*/
void Sys_SetupDPIAwareness(void)
{
	Com_DPrintf(S_COLOR_GREEN "DPI Awareness...\n");
	if (!Sys_EnablePerMonitorV2())
	{
		Com_DPrintf(S_COLOR_RED " ...per monitor v2: failed\n");
	}
	else
	{
		Com_DPrintf(S_COLOR_GREEN " ...per monitor v2: succeeded\n");
		return;
	}
	if (!Sys_EnablePerMonitor())
	{
		Com_DPrintf(S_COLOR_RED " ...per monitor: failed\n");
	}
	else
	{
		Com_DPrintf(S_COLOR_GREEN " ...per monitor: succeeded\n");
		return;
	}

	if (!Sys_EnableDPIAware())
	{
		Com_DPrintf(S_COLOR_RED " ...per process: failed\n");
	}
	else
	{
		Com_DPrintf(S_COLOR_GREEN " ...per process: succeeded\n");
		return;
	}
}
