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
 * @file cl_scrn.c
 * @brief master for refresh, status bar, console, chat, notify, etc
 */

#include "client.h"

qboolean scr_initialized;           // ready to draw

cvar_t *cl_timegraph;
cvar_t *cl_debuggraph;
cvar_t *cl_graphheight;
cvar_t *cl_graphscale;
cvar_t *cl_graphshift;

/**
 * @brief Adjusted for resolution and screen aspect ratio
 */
void SCR_AdjustFrom640(float *x, float *y, float *w, float *h)
{
	float xscale;
	float yscale;

#if 0
	// adjust for wide screens
	if (cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640)
	{
		*x += 0.5 * (cls.glconfig.vidWidth - (cls.glconfig.vidHeight * 640 / 480));
	}
#endif

	// scale for screen sizes
	xscale = cls.glconfig.vidWidth / 640.0;
	yscale = cls.glconfig.vidHeight / 480.0;
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
 */
void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader)
{
	SCR_AdjustFrom640(&x, &y, &width, &height);
	re.DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

/**
 * @brief Chars are drawn at 640*480 virtual screen size
 */
static void SCR_DrawChar(int x, int y, float size, int ch)
{
	int   row, col;
	float frow, fcol;
	float ax, ay, aw, ah;

	ch &= 255;

	if (ch == ' ')
	{
		return;
	}

	if (y < -size)
	{
		return;
	}

	ax = x;
	ay = y;
	aw = size;
	ah = size;
	SCR_AdjustFrom640(&ax, &ay, &aw, &ah);

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	re.DrawStretchPic(ax, ay, aw, ah,
	                  fcol, frow,
	                  fcol + size, frow + size,
	                  cls.charSetShader);
}

/**
 * @brief Small chars are drawn at native screen resolution
 */
void SCR_DrawSmallChar(int x, int y, int ch)
{
	int   row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if (ch == ' ')
	{
		return;
	}

	if (y < -SMALLCHAR_HEIGHT)
	{
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	re.DrawStretchPic(x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
	                  fcol, frow,
	                  fcol + size, frow + size,
	                  cls.charSetShader);
}

/*
==================
SCR_DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawStringExt(int x, int y, float size, const char *string, float *setColor, qboolean forceColor, qboolean noColorEscape)
{
	vec4_t     color;
	const char *s;
	int        xx;

	// draw the drop shadow
	color[0] = color[1] = color[2] = 0;
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
		SCR_DrawChar(xx + 2, y + 2, size, *s);
		xx += size;
		s++;
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
					memcpy(color, setColor, sizeof(color));
				}
				else
				{
					memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof(color));
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
		SCR_DrawChar(xx, y, size, *s);
		xx += size;
		s++;
	}
	re.SetColor(NULL);
}

void SCR_DrawBigString(int x, int y, const char *s, float alpha, qboolean noColorEscape)
{
	float color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, qfalse, noColorEscape);
}

void SCR_DrawBigStringColor(int x, int y, const char *s, vec4_t color, qboolean noColorEscape)
{
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, qtrue, noColorEscape);
}

/*
==================
SCR_DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawSmallStringExt(int x, int y, const char *string, float *setColor, qboolean forceColor, qboolean noColorEscape)
{
	vec4_t     color;
	const char *s;
	int        xx;

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
					memcpy(color, setColor, sizeof(color));
				}
				else
				{
					memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof(color));
					color[3] = setColor[3];
				}
				re.SetColor(color);
			}
			if (!noColorEscape)
			{
				s += 2;
				continue;
			}
		}
		SCR_DrawSmallChar(xx, y, *s);
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	re.SetColor(NULL);
}

/**
 * @brief Like strlen, but skips color escape codes
 */
static int SCR_Strlen(const char *str)
{
	const char *s    = str;
	int        count = 0;

	while (*s)
	{
		if (Q_IsColorString(s))
		{
			s += 2;
		}
		else
		{
			count++;
			s++;
		}
	}

	return count;
}

/**
 * @note Unused.
 */
int SCR_GetBigStringWidth(const char *str)
{
	return SCR_Strlen(str) * BIGCHAR_WIDTH;;
}

//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording(void)
{
	if (!clc.demorecording)
	{
		return;
	}

	Cvar_Set("cl_demooffset", va("%d", FS_FTell(clc.demofile)));
}

/*
===============================================================================
DEBUG GRAPH
===============================================================================
*/

static int   current;
static float values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph(float value)
{
	values[current] = value;
	current         = (current + 1) % ARRAY_LEN(values);
}

/*
==============
SCR_DrawDebugGraph
==============
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

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	cl_timegraph   = Cvar_Get("timegraph", "0", CVAR_CHEAT);
	cl_debuggraph  = Cvar_Get("debuggraph", "0", CVAR_CHEAT);
	cl_graphheight = Cvar_Get("graphheight", "32", CVAR_CHEAT);
	cl_graphscale  = Cvar_Get("graphscale", "1", CVAR_CHEAT);
	cl_graphshift  = Cvar_Get("graphshift", "0", CVAR_CHEAT);

	scr_initialized = qtrue;
}

//=======================================================

/**
 * @brief This will be called twice if rendering in stereo mode
 */
void SCR_DrawScreenField(stereoFrame_t stereoFrame)
{
	re.BeginFrame(stereoFrame);

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	/*  if ( cls.state != CA_ACTIVE ) {
	        if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 ) {
	            re.SetColor( g_color_table[0] );
	            re.DrawStretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
	            re.SetColor( NULL );
	        }
	    }*/

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
			break;
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
			CL_CGameRendering(stereoFrame);

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			//if (!com_sv_running->value || Cvar_VariableIntegerValue("sv_cheats")) // don't draw useless text if not in dev mode
			VM_Call(uivm, UI_REFRESH, cls.realtime);
			VM_Call(uivm, UI_DRAW_CONNECT_SCREEN, qtrue);
			break;
		case CA_ACTIVE:
			CL_CGameRendering(stereoFrame);
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
	if (cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer)
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

	// if running in stereo, we need to draw the frame twice
	if (cls.glconfig.stereoEnabled)
	{
		SCR_DrawScreenField(STEREO_LEFT);
		SCR_DrawScreenField(STEREO_RIGHT);
	}
	else
	{
		SCR_DrawScreenField(STEREO_CENTER);
	}

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
