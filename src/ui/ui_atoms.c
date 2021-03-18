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
 * @file ui_atoms.c
 * @brief User interface building blocks and support functions.
 */

#include "ui_local.h"

/**
 * @brief Com_DPrintf
 * @param[in] fmt
 */
void QDECL Com_DPrintf(const char *fmt, ...)
{
	va_list argptr;
	char    msg[MAX_PRINT_MSG];
	float   developer;

	developer = trap_Cvar_VariableValue("developer");
	if (developer == 0.f)
	{
		return;
	}

	va_start(argptr, fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	Com_Printf("%s", msg);
}

/**
 * @brief Com_Error
 * @param code - unused
 * @param[in] error
 */
void QDECL Com_Error(int code, const char *error, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(va("%s", text));
}

/**
 * @brief Com_Printf
 * @param[in] msg
 */
void QDECL Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(va("%s", text));
}

/**
 * @brief UI_Argv
 * @param[in] arg
 * @return
 */
char *UI_Argv(int arg)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

/**
 * @brief UI_Cvar_VariableString
 * @param[in] varName
 * @return
 */
char *UI_Cvar_VariableString(const char *varName)
{
	static char buffer[2][MAX_STRING_CHARS];
	static int  toggle;

	toggle ^= 1;        // flip-flop to allow two returns without clash

	trap_Cvar_VariableStringBuffer(varName, buffer[toggle], sizeof(buffer[0]));

	return buffer[toggle];
}

/**
 * @brief UI_Cache_f
 */
static void UI_Cache_f(void)
{
	Display_CacheAll();
}

/**
 * @brief UI_ConsoleCommand
 * @param[in] realTime
 * @return
 */
qboolean UI_ConsoleCommand(int realTime)
{
	char            *cmd;
	uiClientState_t cstate;

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime  = realTime;

	cmd = UI_Argv(0);

	if (Q_stricmp(cmd, "ui_test") == 0)
	{
		UI_ShowPostGame();
	}
	else if (Q_stricmp(cmd, "ui_report") == 0)
	{
		UI_Report();
		return qtrue;
	}
	else if (Q_stricmp(cmd, "ui_load") == 0)
	{
		UI_Load();
		return qtrue;
	}
	else if (Q_stricmp(cmd, "ui_cache") == 0)
	{
		UI_Cache_f();
		return qtrue;
	}
	else if (Q_stricmp(cmd, "listfavs") == 0)
	{
		UI_ListFavourites_f();
		return qtrue;
	}
	else if (Q_stricmp(cmd, "removefavs") == 0)
	{
		UI_RemoveAllFavourites_f();
		return qtrue;
	}
	else if (Q_stricmp(cmd, "show_menu") == 0 && DC->getCVarValue("developer") != 0.f)
	{
		char *menu_name = UI_Argv(1);
		if (menu_name)
		{
			Menus_OpenByName(menu_name);
		}
	}

	trap_GetClientState(&cstate);
	if (cstate.connState == CA_DISCONNECTED)
	{
		if (Q_stricmp(cmd, "campaign") == 0)
		{
			UI_Campaign_f();
			return qtrue;
		}
		else if (Q_stricmp(cmd, "listcampaigns") == 0)
		{
			UI_ListCampaigns_f();
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Adjusted for resolution and screen aspect ratio
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 */
void UI_AdjustFrom640(float *x, float *y, float *w, float *h)
{
	// expect valid pointers
	*x *= uiInfo.uiDC.xscale;
	*y *= uiInfo.uiDC.yscale;
	*w *= uiInfo.uiDC.xscale;
	*h *= uiInfo.uiDC.yscale;

	// adjusting
	if (uiInfo.uiDC.glconfig.windowAspect > RATIO43)
	{
		*x *= RATIO43 / uiInfo.uiDC.glconfig.windowAspect;
		*w *= RATIO43 / uiInfo.uiDC.glconfig.windowAspect;
	}
}

/*
 * @brief UI_DrawNamedPic
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] picname
 *
 * @note Unused.
void UI_DrawNamedPic(float x, float y, float width, float height, const char *picname)
{
    qhandle_t hShader;

    hShader = trap_R_RegisterShaderNoMip(picname);
    UI_AdjustFrom640(&x, &y, &width, &height);
    trap_R_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}
*/

/**
 * @brief UI_DrawHandlePic
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] hShader
 */
void UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader)
{
	float s0;
	float s1;
	float t0;
	float t1;

	if (w < 0)       // flip about vertical
	{
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else
	{
		s0 = 0;
		s1 = 1;
	}

	if (h < 0)       // flip about horizontal
	{
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else
	{
		t0 = 0;
		t1 = 1;
	}

	UI_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

/*
 * @brief UI_DrawRotatedPic
 * Coordinates are 640*480 virtual values
 * @note Unused (cool stuff for ETL logo?!)
void UI_DrawRotatedPic(float x, float y, float width, float height, qhandle_t hShader, float angle)
{
    UI_AdjustFrom640(&x, &y, &width, &height);

    trap_R_DrawRotatedPic(x, y, width, height, 0, 0, 1, 1, hShader, angle);
}
*/

/**
 * Coordinates are 640*480 virtual values
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] color
 */
void UI_FillRect(float x, float y, float width, float height, const float *color)
{
	trap_R_SetColor(color);

	UI_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);

	trap_R_SetColor(NULL);
}

/**
 * @brief UI_DrawTopBottom
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 *
 * @note Unused
 */
void UI_DrawTopBottom(float x, float y, float w, float h)
{
	UI_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
	trap_R_DrawStretchPic(x, y + h - 1, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
}
