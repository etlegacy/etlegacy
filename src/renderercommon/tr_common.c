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
 * @file tr_common.c
 */

#include "tr_common.h"
#include "tr_local_proxy.h"

cvar_t *r_ext_multisample;

/**
  * @var imageLoaders
  * @brief Note that the ordering indicates the order of preference used
  * when there are multiple images of different formats available
  */
imageExtToLoaderMap_t imageLoaders[] =
{
	{ "svg",  R_LoadSVG },
	{ "png",  R_LoadPNG },
	{ "tga",  R_LoadTGA },
	{ "jpg",  R_LoadJPG },
	{ "jpeg", R_LoadJPG },
	{ "pcx",  R_LoadPCX },
	{ "bmp",  R_LoadBMP }
};

int numImageLoaders = sizeof(imageLoaders) / sizeof(imageLoaders[0]);

/**
 * @brief Workaround for ri.Printf's 1024 characters buffer limit.
 * @param[in] string
 */
void R_PrintLongString(const char *string)
{
	char       buffer[1024];
	const char *p   = string;
	int        size = (int)strlen(string);

	while (size > 0)
	{
		Q_strncpyz(buffer, p, sizeof(buffer));
		Ren_Print("%s", buffer);
		p    += 1023;
		size -= 1023;
	}
}

void R_RegisterCommon(void)
{
	r_ext_multisample = ri.Cvar_Get("r_ext_multisample", "0", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	ri.Cvar_CheckRange(r_ext_multisample, 0, 8, qtrue);
}

#ifdef USE_RENDERER_DLOPEN
/**
 * @brief Com_Printf
 * @param[in] msg
 */
void QDECL Com_Printf(const char *fmt, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	Ren_Print("%s", text);
}

/**
 * @brief Com_DPrintf
 * @param[in] msg
 */
void QDECL Com_DPrintf(const char *fmt, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	Ren_Developer("%s", text);
}

/**
 * @brief Com_Error
 * @param[in] code
 * @param[in] error
 */
void QDECL Com_Error(int code, const char *error, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Error(code, "%s", text);
}
#endif
