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
 * @file null_client.c
 */

#include "../client/client.h"

cvar_t *cl_shownet;
// win32 dedicated

/**
 * @brief CL_Shutdown
 */
void CL_Shutdown(void)
{
}

/**
 * @brief CL_Init
 */
void CL_Init(void)
{
	cl_shownet = Cvar_Get("cl_shownet", "0", CVAR_TEMP);
}

/**
 * @brief CL_MouseEvent
 * @param dx   - unused
 * @param dy   - unused
 * @param time - unused
 */
void CL_MouseEvent(int dx, int dy, int time)
{
}

/**
 * @brief Key_WriteBindings
 * @param f - unused
 */
void Key_WriteBindings(fileHandle_t f)
{
}

/**
 * @brief CL_Frame
 * @param msec
 */
void CL_Frame(int msec)
{
}

/**
 * @brief CL_PacketEvent
 * @param from - unused
 * @param msg  - unused
 */
void CL_PacketEvent(netadr_t from, msg_t *msg)
{
}

/**
 * @brief CL_CharEvent
 * @param key - unused
 */
void CL_CharEvent(int key)
{
}

/**
 * @brief CL_Disconnect
 * @param showMainMenu - unused
 */
void CL_Disconnect(qboolean showMainMenu)
{
}

/**
 * @brief CL_MapLoading
 */
void CL_MapLoading(void)
{
}

/**
 * @brief CL_GameCommand
 * @return
 */
qboolean CL_GameCommand(void)
{
	return qfalse;
}

/**
 * @brief CL_KeyEvent
 * @param key  - unused
 * @param down - unused
 * @param time - unused
 */
void CL_KeyEvent(int key, qboolean down, unsigned time)
{
}

/**
 * @brief UI_GameCommand
 * @return
 */
qboolean UI_GameCommand(void)
{
	return qfalse;
}

/**
 * @brief CL_ForwardCommandToServer
 * @param string - unused
 */
void CL_ForwardCommandToServer(const char *string)
{
}

/**
 * @brief CL_ConsolePrint
 * @param txt - unused
 */
void CL_ConsolePrint(char *txt)
{
}

/**
 * @brief CL_JoystickEvent
 * @param axis  - unused
 * @param value - unused
 * @param time  - unused
 */
void CL_JoystickEvent(int axis, int value, int time)
{
}

/**
 * @brief CL_InitKeyCommands
 */
void CL_InitKeyCommands(void)
{
}

/**
 * @brief CL_ConnectedToServer
 * @return
 */
qboolean CL_ConnectedToServer(void)
{
	return qfalse;
}

/**
 * @brief CL_FlushMemory
 */
void CL_FlushMemory(void)
{
}

/**
 * @brief CL_StartHunkUsers
 */
void CL_StartHunkUsers(void)
{
}

/**
 * @brief CL_ShutdownAll
 */
void CL_ShutdownAll(void)
{
}

/**
 * @brief Key_ClearStates
 *
 * @note for win32 dedicated
 */
void Key_ClearStates(void)
{
}

/**
 * @brief CL_Snd_Shutdown
 */
void CL_Snd_Shutdown(void)
{
}
