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
 * @file null_client.c
 */

#include "../client/client.h"

cvar_t *cl_shownet;
// win32 dedicated

void CL_Shutdown(void)
{
}

void CL_Init(void)
{
	cl_shownet = Cvar_Get("cl_shownet", "0", CVAR_TEMP);
}

void CL_MouseEvent(int dx, int dy, int time)
{
}

void Key_WriteBindings(fileHandle_t f)
{
}

void CL_Frame(int msec)
{
}

void CL_PacketEvent(netadr_t from, msg_t *msg)
{
}

void CL_CharEvent(int key)
{
}

void CL_Disconnect(qboolean showMainMenu)
{
}

void CL_MapLoading(void)
{
}

qboolean CL_GameCommand(void)
{
	return qfalse;
}

void CL_KeyEvent(int key, qboolean down, unsigned time)
{
}

qboolean UI_GameCommand(void)
{
	return qfalse;
}

void CL_ForwardCommandToServer(const char *string)
{
}

void CL_ConsolePrint(char *txt)
{
}

void CL_JoystickEvent(int axis, int value, int time)
{
}

void CL_InitKeyCommands(void)
{
}

void CL_FlushMemory(void)
{
}

void CL_StartHunkUsers(void)
{
}

void CL_ShutdownAll(void)
{
}

// for win32 dedicated
void Key_ClearStates(void)
{
}
