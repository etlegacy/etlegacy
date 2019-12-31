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
 * @file cl_scrn.c
 * @brief Master for refresh, status bar, console, chat, notify, etc
 */

#include "client.h"
#include "../qcommon/q_unicode.h"

qboolean scr_initialized;           // ready to draw

cvar_t *cl_timegraph;
cvar_t *cl_graphheight;
cvar_t *cl_graphscale;
cvar_t *cl_graphshift;

/**
 * @brief Adjusted for resolution and screen aspect ratio
 * @param[in,out] x
 * @param[in,out] y
 * @param[in,out] w
 * @param[in,out] h
 */
void SCR_AdjustFrom640(float *x, float *y, float *w, float *h)
{
	float xscale;
	float yscale;

	// scale for screen sizes
	xscale = cls.glconfig.vidWidth / 640.0f;
	yscale = cls.glconfig.vidHeight / 480.0f;
	if (x)
	{
		*x *= xscale;
	}
	if (y)
	{
		*y *= yscale;
	}
	if (w)
	{
		*w *= xscale;
	}
	if (h)
	{
		*h *= yscale;
	}
}

/**
 * @brief Coordinates are 640*480 virtual values
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] color
 */
void SCR_FillRect(float x, float y, float width, float height, const float *color)
{
	re.SetColor(color);

	SCR_AdjustFrom640(&x, &y, &width, &height);
	re.DrawStretchPic(x, y, width, height, 0, 0, 0, 0, cls.whiteShader);

	re.SetColor(NULL);
}

/**
 * @brief Coordinates are 640*480 virtual values
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] hShader
 */
void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader)
{
	SCR_AdjustFrom640(&x, &y, &width, &height);
	re.DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}
/**
 * @brief SRC_DrawSingleChar
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] ch
 */
static void SRC_DrawSingleChar(int x, int y, int w, int h, int ch)
{
	int   row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	re.DrawStretchPic(x, y, w, h, fcol, frow,
	                  fcol + size, frow + size,
	                  cls.charSetShader);
}

/**
 * @brief Chars are drawn at 640*480 virtual or native screen size
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] ch
 * @param[in] nativeResolution
 */
void SCR_DrawChar(int x, int y, float w, float h, int ch, qboolean nativeResolution)
{
	if (ch == ' ')
	{
		return;
	}

	if (y < -h)
	{
		return;
	}

	if (nativeResolution)
	{
		SRC_DrawSingleChar(x, y, (int)w, (int)h, ch);
	}
	else
	{
		float ax, ay, aw, ah;

		ax = x;
		ay = y;
		aw = w;
		ah = h;
		SCR_AdjustFrom640(&ax, &ay, &aw, &ah);
		SRC_DrawSingleChar((int)ax, (int)ay, (int)aw, (int)ah, ch);
	}
}

/**
 * @brief Draws a multi-colored string with a drop shadow, optionally forcing
 * to a fixed color.
 *
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] string
 * @param[in] setColor
 * @param[in] forceColor
 * @param[in] noColorEscape
 * @param[in] dropShadow
 * @param[in] nativeResolution
 */
void SCR_DrawStringExt(int x, int y, float w, float h, const char *string, float *setColor, qboolean forceColor, qboolean noColorEscape, qboolean dropShadow, qboolean nativeResolution)
{
	vec4_t     color;
	const char *s;
	int        xx;

	if (dropShadow)
	{
		// draw the drop shadow
		Vector4Copy(colorBlack, color);
		color[3] = setColor[3];
		re.SetColor(color);
		s  = string;
		xx = x;
		while (*s)
		{
			if (!noColorEscape && Q_IsColorString(s))
			{
				s += 2;
				continue;
			}
			SCR_DrawChar(xx + 2, y + 2, w, h, Q_UTF8_CodePoint(s), nativeResolution);
			xx += w;
			s  += Q_UTF8_Width(s);
		}
	}

	// draw the colored text
	s  = string;
	xx = x;
	re.SetColor(setColor);
	while (*s)
	{
		if (Q_IsColorString(s))
		{
			if (!forceColor)
			{
				if (*(s + 1) == COLOR_NULL)
				{
					Com_Memcpy(color, setColor, sizeof(color));
				}
				else
				{
					Com_Memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof(color));
					color[3] = setColor[3];
				}
				color[3] = setColor[3];
				re.SetColor(color);
			}
			if (!noColorEscape)
			{
				s += 2;
				continue;
			}
		}
		SCR_DrawChar(xx, y, w, h, Q_UTF8_CodePoint(s), nativeResolution);
		xx += w;
		s  += Q_UTF8_Width(s);
	}
	re.SetColor(NULL);
}

/**
 * @brief SCR_DrawDemoRecording
 */
void SCR_DrawDemoRecording(void)
{
	if (!clc.demorecording)
	{
		return;
	}

	Cvar_Set("cl_demooffset", va("%d", FS_FTell(clc.demofile)));
}

static int   current;
static float values[1024];

/**
 * @brief SCR_DebugGraph
 * @param value
 */
void SCR_DebugGraph(float value)
{
	values[current] = value;
	current         = (current + 1) % ARRAY_LEN(values);
}

/**
 * @brief SCR_DrawDebugGraph
 */
void SCR_DrawDebugGraph(void)
{
	int   a, x, y, w, i, h;
	float v;

	// draw the graph
	w = cls.glconfig.vidWidth;
	x = 0;
	y = cls.glconfig.vidHeight;
	re.SetColor(g_color_table[0]);
	re.DrawStretchPic(x, y - cl_graphheight->integer,
	                  w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader);
	re.SetColor(NULL);

	for (a = 0 ; a < w ; a++)
	{
		i = (ARRAY_LEN(values) + current - 1 - (a % ARRAY_LEN(values))) % ARRAY_LEN(values);
		v = values[i];
		v = v * cl_graphscale->integer + cl_graphshift->integer;

		if (v < 0)
		{
			v += cl_graphheight->integer * (1 + (int)(-v / cl_graphheight->integer));
		}
		h = (int)v % cl_graphheight->integer;
		re.DrawStretchPic(x + w - 1 - a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader);
	}
}

/**
 * @brief SCR_Init
 */
void SCR_Init(void)
{
	cl_timegraph   = Cvar_Get("timegraph", "0", 0);
	cl_graphheight = Cvar_Get("graphheight", "32", 0);
	cl_graphscale  = Cvar_Get("graphscale", "1", 0);
	cl_graphshift  = Cvar_Get("graphshift", "0", 0);

	scr_initialized = qtrue;
}

/**
 * @brief SCR_DrawScreenField
 */
void SCR_DrawScreenField(void)
{
	re.BeginFrame();

	if (!uivm)
	{
		Com_DPrintf("draw screen without UI loaded\n");
		return;
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	if (!VM_Call(uivm, UI_IS_FULLSCREEN))
	{
		switch (cls.state)
		{
		default:
			Com_Error(ERR_FATAL, "SCR_DrawScreenField: bad cls.state");
		case CA_CINEMATIC:
			SCR_DrawCinematic();
			break;
		case CA_DISCONNECTED:
			// force menu up
			S_StopAllSounds();
			VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			VM_Call(uivm, UI_REFRESH, cls.realtime);
			VM_Call(uivm, UI_DRAW_CONNECT_SCREEN, qfalse);
			break;
		// if the cgame is valid, fall through to there
		//if (!cls.cgameStarted || !com_sv_running->integer) {
		// connecting clients will only show the connection dialog
		//VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qfalse );
		//break;
		//}
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			CL_CGameRendering();

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			//if (!com_sv_running->value || Cvar_VariableIntegerValue("sv_cheats")) // don't draw useless text if not in dev mode
			VM_Call(uivm, UI_REFRESH, cls.realtime);
			VM_Call(uivm, UI_DRAW_CONNECT_SCREEN, qtrue);
			break;
		case CA_ACTIVE:
			CL_CGameRendering();
			SCR_DrawDemoRecording();
			break;
		}
	}

	// the menu draws next
	if ((cls.keyCatchers & KEYCATCH_UI) && uivm)
	{
		VM_Call(uivm, UI_REFRESH, cls.realtime);
	}

	// console draws next
	Con_DrawConsole();

	// debug graph can be drawn on top of anything
	if (cl_timegraph->integer || cl_debugMove->integer)
	{
		SCR_DrawDebugGraph();
	}
}

/**
 * @brief This is called every frame, and can also be called explicitly to flush
 * text to the screen.
 */
void SCR_UpdateScreen(void)
{
	static int recursive = 0;

	if (!scr_initialized)
	{
		return;             // not initialized yet
	}

	if (++recursive >= 2)
	{
		recursive = 0;
		// Gordon: i'm breaking this again, because we've removed most of our cases but still have one which will not fix easily
		return;
		//Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
	}
	recursive = 1;

	SCR_DrawScreenField();

	if (com_speeds->integer)
	{
		re.EndFrame(&time_frontend, &time_backend);
	}
	else
	{
		re.EndFrame(NULL, NULL);
	}

	recursive = 0;
}
