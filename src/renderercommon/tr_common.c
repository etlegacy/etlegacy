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
* @file tr_common.c
*/

#include "tr_common.h"

#ifdef USE_RENDERER_DLOPEN
void QDECL Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	Ren_Print("%s", text);
}

void QDECL Com_DPrintf(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	Ren_Developer("%s", text);
}

void QDECL Com_Error(int level, const char *error, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Error(level, "%s", text);
}
#else
void make_osx_happy(void)
{
    //This is here just so that osx:s compiler is happy and does not whine about not having anything to link
}
#endif
