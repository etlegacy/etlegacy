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
 */
/**
 * @file tr_common.c
 */

#include "tr_common.h"

/*
==================
R_GetModeInfo
==================
*/
typedef struct vidmode_s
{
	const char *description;
	int width, height;
	float pixelAspect;              // pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",           320,  240,  1 },
	{ "Mode  1: 400x300",           400,  300,  1 },
	{ "Mode  2: 512x384",           512,  384,  1 },
	{ "Mode  3: 640x480",           640,  480,  1 },
	{ "Mode  4: 800x600",           800,  600,  1 },
	{ "Mode  5: 960x720",           960,  720,  1 },
	{ "Mode  6: 1024x768",          1024, 768,  1 },
	{ "Mode  7: 1152x864",          1152, 864,  1 },
	{ "Mode  8: 1280x1024",         1280, 1024, 1 },
	{ "Mode  9: 1600x1200",         1600, 1200, 1 },
	{ "Mode 10: 2048x1536",         2048, 1536, 1 },
	{ "Mode 11: 856x480 (wide)",    856,  480,  1 },
	{ "Mode 12: 1366x768 (16:9)",   1366, 768,  1 },
	{ "Mode 13: 1440x900 (16:10)",  1440, 900,  1 },
	{ "Mode 14: 1680x1050 (16:10)", 1680, 1050, 1 },
	{ "Mode 15: 1600x1200",         1600, 1200, 1 },
	{ "Mode 16: 1920x1080 (16:9)",  1920, 1080, 1 },
	{ "Mode 17: 1920x1200 (16:10)", 1920, 1200, 1 },
	{ "Mode 18: 2560x1440 (16:9)",  2560, 1440, 1 },
	{ "Mode 19: 2560x1600 (16:10)", 2560, 1600, 1 },
	{ "Mode 20: 3840x2160 (16:9)",  3840, 2160, 1 },
};
static int s_numVidModes = ARRAY_LEN(r_vidModes);

qboolean R_GetModeInfo(int *width, int *height, float *windowAspect, int mode)
{
	vidmode_t *vm;
	float     pixelAspect;

	if (mode < -1)
	{
		return qfalse;
	}
	if (mode >= s_numVidModes)
	{
		return qfalse;
	}

	if (mode == -1)
	{
		*width      = r_customwidth->integer;
		*height     = r_customheight->integer;
		pixelAspect = r_customaspect->value;
	}
	else
	{
		vm = &r_vidModes[mode];

		*width      = vm->width;
		*height     = vm->height;
		pixelAspect = vm->pixelAspect;
	}

	*windowAspect = (float)*width / (*height * pixelAspect);

	return qtrue;
}

/**
* @brief Prints hardcoded screen resolutions
* @see r_availableModes for supported resolutions
*/
void R_ModeList_f(void)
{
	int i;

	Ren_Print("\n");
	Ren_Print((r_mode->integer == -2) ? "%s ^2(current)\n" : "%s\n",
	          "Mode -2: desktop resolution");
	Ren_Print((r_mode->integer == -1) ? "%s ^2(current)\n" : "%s\n",
	          "Mode -1: custom resolution");
	for (i = 0; i < s_numVidModes; i++)
	{
		Ren_Print((i == r_mode->integer) ? "%s ^2(current)\n" : "%s\n",
		          r_vidModes[i].description);
	}
	Ren_Print("\n");
}

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
