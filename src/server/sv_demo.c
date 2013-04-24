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
 * @file sv_demo.c
 */

#include "server.h"

#define SV_DEMO_DIR va("demos/server%s%s", sv_demopath->string[0] ? "/" : "", sv_demopath->string[0] ? sv_demopath->string : "")

void SV_StartDemoRecording(client_t *client, const char *filename, int forcetrack)
{

}

void SV_WriteDemoMessage(client_t *client, msg_t *sendbuffer, qboolean clienttoserver)
{

}

void SV_StopDemoRecording(client_t *client)
{

}

void SV_Demo_Stop(qboolean cancel)
{

}

void SV_Demo_Stop_f(void)
{
	SV_Demo_Stop(qfalse);
}

void SV_Demo_Cancel_f(void)
{
	SV_Demo_Stop(qtrue);
}

void SV_Demo_Start_f(void)
{

}

void SV_Demo_Purge_f(void)
{

}

void SV_DemoList_f(void)
{

}

qboolean SV_DemoRecording(void)
{
	return qfalse;
}

void SV_AddDemoCommands(void)
{
	Cmd_AddCommand("serverdemostop", SV_Demo_Stop_f);
	Cmd_AddCommand("serverdemocancel", SV_Demo_Cancel_f);
	Cmd_AddCommand("serverdemorecord", SV_Demo_Start_f);
	Cmd_AddCommand("serverdemopurge", SV_Demo_Purge_f);
	Cmd_AddCommand("serverdemolist", SV_DemoList_f);
}