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
 * @file ui_shared.c
 * @brief String allocation/managment
 */

#include "ui_shared.h"
#include "ui_local.h"    // For CS settings/retrieval

scrollInfo_t scrollInfo;

void      (*captureFunc)(void *p) = NULL;
void      *captureData = NULL;
itemDef_t *itemCapture = NULL;   // item that has the mouse captured ( if any )

displayContextDef_t *DC = NULL;

qboolean g_waitingForKey = qfalse;
qboolean g_editingField  = qfalse;

itemDef_t *g_bindItem = NULL;
itemDef_t *g_editItem = NULL;

menuDef_t Menus[MAX_MENUS];      // defined menus
int       menuCount = 0;         // how many

// a stack for modal menus only, stores the menus to come back to
// (an item can be NULL, goes back to main menu / no action required)
menuDef_t *modalMenuStack[MAX_MODAL_MENUS];
int       modalMenuCount = 0;

qboolean debugMode            = qfalse;
int      lastListBoxClickTime = 0;

#ifdef CGAME
#define MEM_POOL_SIZE  128 * 1024
#else
#define MEM_POOL_SIZE  2048 * 1024  // was 1536, 1024
#endif

static char     memoryPool[MEM_POOL_SIZE];
static int      allocPoint;
static qboolean outOfMemory;

/**
 * @brief Convert rectangle-coordinates for use with the current aspectratio.
 * @param[out] rect
 */
void Cui_WideRect(rectDef_t *rect)
{
	rect->x *= DC->xscale;
	rect->y *= DC->yscale;
	rect->w *= DC->xscale;
	rect->h *= DC->yscale;

	if (DC->glconfig.windowAspect > RATIO43 && DC->getCVarValue("r_mode") != 11.f)
	{
		rect->x *= RATIO43 / DC->glconfig.windowAspect;
		rect->w *= RATIO43 / DC->glconfig.windowAspect;
	}
}

/**
 * @brief Convert an x-coordinate for use with the current aspectratio.
 * (if the current aspectratio is 4:3, then leave the x-coordinate unchanged)
 * @param[in] x
 * @return
 */
float Cui_WideX(float x)
{
	return (DC->glconfig.windowAspect <= RATIO43) ? x : x *(DC->glconfig.windowAspect * RPRATIO43);  // aspectratio / (4/3)
}

/**
 * @brief The horizontal center of screen pixel-difference of a 4:3 ratio vs. the current aspectratio
 */
float Cui_WideXoffset(void)
{
	return (DC->glconfig.windowAspect <= RATIO43) ? 0.0f : ((640.0f * (DC->glconfig.windowAspect * RPRATIO43)) - 640.0f) * 0.5f;
}

/**
 * @brief UI_Alloc
 * @param[in] size
 * @return
 */
void *UI_Alloc(int size)
{
	char *p;

	if (allocPoint + size > MEM_POOL_SIZE)
	{
		outOfMemory = qtrue;
		if (DC->Print)
		{
			DC->Print(S_COLOR_RED "UI_Alloc: Failure. UI out of memory!\n");
		}

		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += (size + 15) & ~15;

	return p;
}

/**
 * @brief UI_InitMemory
 */
void UI_InitMemory(void)
{
	allocPoint  = 0;
	outOfMemory = qfalse;
}

/**
 * @brief UI_OutOfMemory
 * @return
 */
qboolean UI_OutOfMemory(void)
{
	return outOfMemory;
}

#define HASH_TABLE_SIZE 2048

/**
 * @brief Return a hash value for the string
 * @param[in] str
 * @return
 */
static long hashForString(const char *str)
{
	int  i    = 0;
	long hash = 0;
	char letter;

	while (str[i] != '\0')
	{
		letter = (char)tolower(str[i]);
		hash  += (long)(letter) * (i + 119);
		i++;
	}

	hash &= (HASH_TABLE_SIZE - 1);
	return hash;
}

typedef struct stringDef_s
{
	struct stringDef_s *next;
	const char *str;
} stringDef_t;

static int  strPoolIndex = 0;
static char strPool[STRING_POOL_SIZE];

static int         strHandleCount = 0;
static stringDef_t *strHandle[HASH_TABLE_SIZE];

/**
 * @brief String_Alloc
 * @param[in] p
 * @return
 */
const char *String_Alloc(const char *p)
{
	int               len;
	long              hash;
	stringDef_t       *str, *last;
	static const char *staticNULL = "";

	if (p == NULL)
	{
		return NULL;
	}

	if (*p == 0)
	{
		return staticNULL;
	}

	hash = hashForString(p);

	str = strHandle[hash];
	while (str)
	{
		if (strcmp(p, str->str) == 0)
		{
			return str->str;
		}

		str = str->next;
	}

	len = strlen(p);
	if (len + strPoolIndex + 1 < STRING_POOL_SIZE)
	{
		int ph = strPoolIndex;

		strcpy(&strPool[strPoolIndex], p);
		strPoolIndex += len + 1;

		str  = strHandle[hash];
		last = str;
		while (str && str->next)
		{
			str  = str->next;
			last = str;
		}

		str       = UI_Alloc(sizeof(stringDef_t));
		str->next = NULL;
		str->str  = &strPool[ph];
		if (last)
		{
			last->next = str;
		}
		else
		{
			strHandle[hash] = str;
		}

		return &strPool[ph];
	}

	return NULL;
}

/**
 * @brief String_Report
 */
void String_Report(void)
{
	float f;

	Com_Printf("Memory/String Pool Info\n");
	Com_Printf("----------------\n");
	f  = strPoolIndex;
	f /= STRING_POOL_SIZE;
	f *= 100;
	Com_Printf("String Pool is %.1f%% full, %i bytes out of %i used.\n", (double)f, strPoolIndex, STRING_POOL_SIZE);
	f  = allocPoint;
	f /= MEM_POOL_SIZE;
	f *= 100;
	Com_Printf("Memory Pool is %.1f%% full, %i bytes out of %i used.\n", (double)f, allocPoint, MEM_POOL_SIZE);
}

/**
 * @brief String_Init
 */
void String_Init(void)
{
	int i;

	for (i = 0; i < HASH_TABLE_SIZE; i++)
	{
		strHandle[i] = 0;
	}
	strHandleCount = 0;
	strPoolIndex   = 0;
	menuCount      = 0;
	modalMenuCount = 0;
	UI_InitMemory();
	Item_SetupKeywordHash();
	Menu_SetupKeywordHash();
	if (DC && DC->getBindingBuf)
	{
		Controls_GetConfig();
	}
}

/**
 * @brief Lerp and clamp each component of @p a and @p b into @p c by the fraction @p t
 * @param[in] a
 * @param[in] b
 * @param[in] c
 * @param[in] t
 */
void LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		c[i] = a[i] + t * (b[i] - a[i]);
		if (c[i] < 0)
		{
			c[i] = 0;
		}
		else if (c[i] > 1.0f)
		{
			c[i] = 1.0;
		}
	}
}

// display, window, menu, item code

/**
 * @brief Initializes the display with a structure to all the drawing routines
 * @param[in] dc
 */
void Init_Display(displayContextDef_t *dc)
{
	DC = dc;
}

// type and style painting

/**
 * @brief GradientBar_Paint
 * @param[in] rect
 * @param[in] color
 */
void GradientBar_Paint(rectDef_t *rect, vec4_t color)
{
	// gradient bar takes two paints
	DC->setColor(color);
	DC->drawHandlePic(rect->x, rect->y, rect->w, rect->h, DC->Assets.gradientBar);
	DC->setColor(NULL);
}

/**
 * @brief GradientRound_Paint
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] color
 */
void GradientRound_Paint(float x, float y, float w, float h, vec4_t color)
{
	// gradient takes two paints
	DC->setColor(color);
	DC->drawHandlePic(x, y, w, h, DC->Assets.gradientRound);
	DC->setColor(NULL);
}

/**
 * @brief Initializes a window structure ( windowDef_t ) with defaults
 * @param[in,out] w
 */
void Window_Init(Window *w)
{
	Com_Memset(w, 0, sizeof(windowDef_t));
	w->borderSize   = 1;
	w->foreColor[0] = w->foreColor[1] = w->foreColor[2] = w->foreColor[3] = 1.0;
	w->cinematic    = -1;
}

/**
 * @brief Fade
 * @param[in,out] flags
 * @param[in,out] f
 * @param[in] clamp
 * @param[in,out] nextTime
 * @param[in] offsetTime
 * @param[in] bFlags
 * @param[in] fadeAmount
 */
void Fade(int *flags, float *f, float clamp, int *nextTime, int offsetTime, qboolean bFlags, float fadeAmount)
{
	if (*flags & (WINDOW_FADINGOUT | WINDOW_FADINGIN))
	{
		if (DC->realTime > *nextTime)
		{
			*nextTime = DC->realTime + offsetTime;
			if (*flags & WINDOW_FADINGOUT)
			{
				*f -= fadeAmount;
				if (bFlags && *f <= 0.0f)
				{
					*flags &= ~(WINDOW_FADINGOUT | WINDOW_VISIBLE);
				}
			}
			else
			{
				*f += fadeAmount;
				if (*f >= clamp)
				{
					*f = clamp;
					if (bFlags)
					{
						*flags &= ~WINDOW_FADINGIN;
					}
				}
			}
		}
	}
}

/**
 * @brief Window_Paint
 * @param[in,out] w
 * @param[in] fadeAmount
 * @param[in] fadeClamp
 * @param[in] fadeCycle
 */
void Window_Paint(Window *w, float fadeAmount, float fadeClamp, float fadeCycle)
{
	vec4_t    color = { 0.0, 0.0, 0.0, 0.0 };
	rectDef_t fillRect;

	if (w == NULL)
	{
		return;
	}

	if (debugMode)
	{
		color[0] = color[1] = color[2] = color[3] = 1;
		DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, 1, color);
	}

	if (w->style == 0 && w->border == 0)
	{
		return;
	}

	fillRect = w->rect;

	// FIXME: do right thing for right border type
	if (w->border != 0)
	{
		fillRect.x += w->borderSize;
		fillRect.y += w->borderSize;
		fillRect.w -= 2 * w->borderSize;
		fillRect.h -= 2 * w->borderSize;
	}

	if (w->style == WINDOW_STYLE_FILLED)
	{
		// box, but possible a shader that needs filled
		if (w->background)
		{
			Fade(&w->flags, &w->backColor[3], fadeClamp, &w->nextTime, fadeCycle, qtrue, fadeAmount);
			DC->setColor(w->backColor);
			DC->drawHandlePic(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background);
			DC->setColor(NULL);
		}
		else
		{
			DC->fillRect(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->backColor);
		}
	}
	else if (w->style == WINDOW_STYLE_GRADIENT)
	{
		GradientBar_Paint(&fillRect, w->backColor);
		// gradient bar
	}
	else if (w->style == WINDOW_STYLE_SHADER)
	{
		if (w->flags & WINDOW_FORECOLORSET)
		{
			DC->setColor(w->foreColor);
		}

		DC->drawHandlePic(fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background);
		DC->setColor(NULL);
	}
	else if (w->style == WINDOW_STYLE_TEAMCOLOR)
	{
		if (DC->getTeamColor)
		{
			DC->getTeamColor(&color);
			DC->fillRect(fillRect.x, fillRect.y, fillRect.w, fillRect.h, color);
		}
	}
	else if (w->style == WINDOW_STYLE_CINEMATIC)
	{
		if (w->cinematic == -1)
		{
			w->cinematic = DC->playCinematic(w->cinematicName, fillRect.x, fillRect.y, fillRect.w, fillRect.h);
			if (w->cinematic == -1)
			{
				w->cinematic = -2;
			}
		}
		if (w->cinematic >= 0)
		{
			DC->runCinematicFrame(w->cinematic);
			DC->drawCinematic(w->cinematic, fillRect.x, fillRect.y, fillRect.w, fillRect.h);
		}
	}

	if (w->border == WINDOW_BORDER_FULL)
	{
		// WARNING: full HACK HACK HACK
		if (w->style == WINDOW_STYLE_TEAMCOLOR)
		{
			if (color[0] > 0)
			{
				// red
				color[0] = 1;
				color[1] = color[2] = .5;
			}
			else
			{
				color[2] = 1;
				color[0] = color[1] = .5;
			}

			color[3] = 1;
			DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, color);
		}
		else
		{
			DC->drawRect(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, w->borderColor);
		}
	}
	else if (w->border == WINDOW_BORDER_HORZ)
	{
		// top/bottom
		DC->setColor(w->borderColor);
		DC->drawTopBottom(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize);
		DC->setColor(NULL);
	}
	else if (w->border == WINDOW_BORDER_VERT)
	{
		// left right
		DC->setColor(w->borderColor);
		DC->drawSides(w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize);
		DC->setColor(NULL);
	}
	else if (w->border == WINDOW_BORDER_KCGRADIENT)
	{
		// this is just two gradient bars along each horz edge
		rectDef_t r = w->rect;

		r.h = w->borderSize;
		GradientBar_Paint(&r, w->borderColor);
		r.y = w->rect.y + w->rect.h - 1;
		GradientBar_Paint(&r, w->borderColor);
	}
}

/**
 * @brief IsVisible
 * @param[in] flags
 * @return
 */
qboolean IsVisible(int flags)
{
	return ((flags & WINDOW_VISIBLE) && !(flags & WINDOW_FADINGOUT));
}

/**
 * @brief FileExists
 * @param filename
 * @return
 */
qboolean FileExists(const char *filename)
{
	fileHandle_t f;

	if (trap_FS_FOpenFile(filename, &f, FS_READ) < 0)
	{
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	else
	{
		trap_FS_FCloseFile(f);
		return qtrue;
	}
}

/**
 * @brief AdjustFrom640
 * @param[out] x
 * @param[out] y
 * @param[out] w
 * @param[out] h
 */
void AdjustFrom640(float *x, float *y, float *w, float *h)
{
	*x *= DC->xscale;
	*y *= DC->yscale;
	*w *= DC->xscale;
	*h *= DC->yscale;

	// adjust screen
	if (DC->glconfig.windowAspect > RATIO43)
	{
		*x *= RATIO43 / DC->glconfig.windowAspect;
		*w *= RATIO43 / DC->glconfig.windowAspect;
	}
}

/**
 * @brief Rect_ContainsPoint
 * @param[in] rect
 * @param[in] x
 * @param[in] y
 * @return
 */
qboolean Rect_ContainsPoint(rectDef_t *rect, float x, float y)
{
	if (rect)
	{
		// correction for widescreen cursor coordinates..
		x = Cui_WideX(x);

		if (x > Cui_WideX(rect->x) && x < Cui_WideX(rect->x + rect->w) && y > rect->y && y < rect->y + rect->h)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Rect_ContainsPointN
 * @param[in] rect
 * @param[in] x
 * @param[in] y
 * @return
 */
qboolean Rect_ContainsPointN(rectDef_t *rect, float x, float y)
{
	if (rect)
	{
		if (x > rect->x && x < rect->x + rect->w + 200 && y > rect->y && y < rect->y + rect->h)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Window_CloseCinematic
 * @param[in,out] window
 */
void Window_CloseCinematic(windowDef_t *window)
{
	if (window->style == WINDOW_STYLE_CINEMATIC && window->cinematic >= 0)
	{
		DC->stopCinematic(window->cinematic);
		window->cinematic = -1;
	}
}

/**
 * @brief Display_CloseCinematics
 */
void Display_CloseCinematics(void)
{
	int i;

	for (i = 0; i < menuCount; i++)
	{
		Menu_CloseCinematics(&Menus[i]);
	}
}

/**
 * @brief Display_VisibleMenuCount
 * @return
 */
int Display_VisibleMenuCount(void)
{
	int i, count = 0;

	for (i = 0; i < menuCount; i++)
	{
		if (Menus[i].window.flags & (WINDOW_FORCED | WINDOW_VISIBLE))
		{
			count++;
		}
	}

	return count;
}

/**
 * @brief ToWindowCoords
 * @param[out] x
 * @param[out] y
 * @param[in] window
 */
void ToWindowCoords(float *x, float *y, windowDef_t *window)
{
	*x += window->rect.x;
	*y += window->rect.y;
}

/*
 * @brief Rect_ToWindowCoords
 * @param rect
 * @param window
 *
 * @note Unused
void Rect_ToWindowCoords(rectDef_t *rect, windowDef_t *window)
{
    ToWindowCoords(&rect->x, &rect->y, window);
}
*/

typedef struct
{
	char *command;
	int id; // unused
	int defaultbind1_right;
	int defaultbind2_right;
	int defaultbind1_left;
	int defaultbind2_left;
	int bind1;
	int bind2;
} bind_t;

/**
 * @var g_bindings
 * @brief
 * @note These MUST be all lowercase now
 */
static bind_t g_bindings[] =
{
	{ "+forward",         'w',             -1,  K_UPARROW,       -1,  -1, -1, -1 },
	{ "+back",            's',             -1,  K_DOWNARROW,     -1,  -1, -1, -1 },
	{ "+moveleft",        'a',             -1,  K_LEFTARROW,     -1,  -1, -1, -1 },
	{ "+moveright",       'd',             -1,  K_RIGHTARROW,    -1,  -1, -1, -1 },
	{ "+moveup",          K_SPACE,         -1,  K_KP_INS,        -1,  -1, -1, -1 },
	{ "+movedown",        'c',             -1,  K_LCTRL,         -1,  -1, -1, -1 },
	{ "+leanright",       'e',             -1,  K_PGDN,          -1,  -1, -1, -1 },
	{ "+leanleft",        'q',             -1,  K_DEL,           -1,  -1, -1, -1 },
	{ "+prone",           'x',             -1,  K_LSHIFT,        -1,  -1, -1, -1 },
	{ "+attack",          K_MOUSE1,        -1,  K_MOUSE1,        -1,  -1, -1, -1 },
	{ "weapalt",          K_MOUSE2,        -1,  K_MOUSE2,        -1,  -1, -1, -1 },
	{ "weapprev",         K_MWHEELDOWN,    -1,  K_MWHEELDOWN,    -1,  -1, -1, -1 },
	{ "weapnext",         K_MWHEELUP,      -1,  K_MWHEELUP,      -1,  -1, -1, -1 },
	{ "weaponbank 10",    '0',             -1,  '0',             -1,  -1, -1, -1 },
	{ "weaponbank 1",     '1',             -1,  '1',             -1,  -1, -1, -1 },
	{ "weaponbank 2",     '2',             -1,  '2',             -1,  -1, -1, -1 },
	{ "weaponbank 3",     '3',             -1,  '3',             -1,  -1, -1, -1 },
	{ "weaponbank 4",     '4',             -1,  '4',             -1,  -1, -1, -1 },
	{ "weaponbank 5",     '5',             -1,  '5',             -1,  -1, -1, -1 },
	{ "weaponbank 6",     '6',             -1,  '6',             -1,  -1, -1, -1 },
	{ "weaponbank 7",     '7',             -1,  '7',             -1,  -1, -1, -1 },
	{ "weaponbank 8",     '8',             -1,  '8',             -1,  -1, -1, -1 },
	{ "weaponbank 9",     '9',             -1,  '9',             -1,  -1, -1, -1 },
	{ "+sprint",          K_LSHIFT,        -1,  K_MOUSE3,        -1,  -1, -1, -1 },
	{ "+speed",           K_CAPSLOCK,      -1,  K_CAPSLOCK,      -1,  -1, -1, -1 },
	{ "+activate",        'f',             -1,  K_ENTER,         -1,  -1, -1, -1 },
	{ "+zoom",            'b',             -1,  'b',             -1,  -1, -1, -1 },
	{ "+mapexpand",       'g',             -1,  '#',             -1,  -1, -1, -1 },
	{ "dropobj",          'h',             -1,  'h',             -1,  -1, -1, -1 },
	{ "+reload",          'r',             -1,  K_END,           -1,  -1, -1, -1 },
	{ "kill",             'k',             -1,  'k',             -1,  -1, -1, -1 },
	{ "+scores",          K_TAB,           -1,  K_TAB,           -1,  -1, -1, -1 },
	{ "+stats",           K_LALT,          -1,  K_F9,            -1,  -1, -1, -1 },
	{ "+topshots",        K_LCTRL,         -1,  K_F10,           -1,  -1, -1, -1 },
	{ "+objectives",      'o',             -1,  'o',             -1,  -1, -1, -1 },
	{ "toggleconsole",    '`',             '~', '`',             '~', -1, -1, -1 },
	{ "togglemenu",       K_ESCAPE,        -1,  K_ESCAPE,        -1,  -1, -1, -1 },
	{ "openlimbomenu",    'l',             -1,  'l',             -1,  -1, -1, -1 },
#ifdef FEATURE_MULTIVIEW
	{ "mvactivate",       'm',             -1,  'm',             -1,  -1, -1, -1 },
	{ "spechelp",         K_BACKSPACE,     -1,  K_BACKSPACE,     -1,  -1, -1, -1 },
#endif
	{ "mapzoomout",       ',',             -1,  '[',             -1,  -1, -1, -1 },
	{ "mapzoomin",        '.',             -1,  ']',             -1,  -1, -1, -1 },
	{ "zoomin",           '=',             -1,  '-',             -1,  -1, -1, -1 },
	{ "zoomout",          '-',             -1,  '=',             -1,  -1, -1, -1 },
	{ "classmenu",        'p',             -1,  'p',             -1,  -1, -1, -1 },
	{ "teammenu",         'j',             -1,  'j',             -1,  -1, -1, -1 },
	{ "spawnmenu",        'i',             -1,  'i',             -1,  -1, -1, -1 },
	{ "messagemode",      't',             -1,  't',             -1,  -1, -1, -1 },
	{ "messagemode2",     'y',             -1,  'y',             -1,  -1, -1, -1 },
	{ "messagemode3",     'u',             -1,  'u',             -1,  -1, -1, -1 },
	{ "mp_quickmessage",  'v',             -1,  'v',             -1,  -1, -1, -1 },
	{ "mp_fireteammsg",   'z',             -1,  'c',             -1,  -1, -1, -1 },
	{ "vote yes",         K_F1,            -1,  K_F1,            -1,  -1, -1, -1 },
	{ "vote no",          K_F2,            -1,  K_F2,            -1,  -1, -1, -1 },
	{ "ready",            K_F3,            -1,  K_F3,            -1,  -1, -1, -1 },
	{ "notready",         K_F4,            -1,  K_F4,            -1,  -1, -1, -1 },
	{ "autoscreenshot",   K_F11,           -1,  K_F11,           -1,  -1, -1, -1 },
	{ "autoRecord",       K_F12,           -1,  K_F12,           -1,  -1, -1, -1 },
	{ "mp_fireteamadmin", K_KP_ENTER,      -1,  K_KP_ENTER,      -1,  -1, -1, -1 },
	{ "selectbuddy -1",   K_KP_PLUS,       -1,  K_KP_PLUS,       -1,  -1, -1, -1 },
	{ "selectbuddy 0",    K_KP_END,        -1,  K_KP_END,        -1,  -1, -1, -1 },
	{ "selectbuddy 1",    K_KP_DOWNARROW,  -1,  K_KP_DOWNARROW,  -1,  -1, -1, -1 },
	{ "selectbuddy 2",    K_KP_PGDN,       -1,  K_KP_PGDN,       -1,  -1, -1, -1 },
	{ "selectbuddy 3",    K_KP_LEFTARROW,  -1,  K_KP_LEFTARROW,  -1,  -1, -1, -1 },
	{ "selectbuddy 4",    K_KP_5,          -1,  K_KP_5,          -1,  -1, -1, -1 },
	{ "selectbuddy 5",    K_KP_RIGHTARROW, -1,  K_KP_RIGHTARROW, -1,  -1, -1, -1 },
	{ "selectbuddy 6",    K_KP_HOME,       -1,  K_KP_HOME,       -1,  -1, -1, -1 },
	{ "selectbuddy 7",    K_KP_UPARROW,    -1,  K_KP_UPARROW,    -1,  -1, -1, -1 },
	{ "selectbuddy -2",   K_KP_MINUS,      -1,  K_KP_MINUS,      -1,  -1, -1, -1 },
	{ "shoutcastmenu",    'n',             -1,  'n',             -1,  -1, -1, -1 },
};

static const int g_bindCount = sizeof(g_bindings) / sizeof(bind_t);

/**
 * @brief Controls_GetConfig
 */
void Controls_GetConfig(void)
{
	int i;

	// iterate each command, get its numeric binding
	for (i = 0; i < g_bindCount; i++)
	{
		DC->getKeysForBinding(g_bindings[i].command, &g_bindings[i].bind1, &g_bindings[i].bind2);
	}
}

/**
 * @brief Controls_SetConfig
 * @param[in] restart
 */
void Controls_SetConfig(qboolean restart)
{
	int i;

	// iterate each command, get its numeric binding
	for (i = 0; i < g_bindCount; i++)
	{
		if (g_bindings[i].bind1 != -1)
		{
			DC->setBinding(g_bindings[i].bind1, g_bindings[i].command);

			if (g_bindings[i].bind2 != -1)
			{
				DC->setBinding(g_bindings[i].bind2, g_bindings[i].command);
			}
		}
	}

	if (restart)
	{
		DC->executeText(EXEC_APPEND, "in_restart\n");
	}
}

/**
 * @brief Controls_SetDefaults
 * @param[in] lefthanded
 */
void Controls_SetDefaults(qboolean lefthanded)
{
	int i;

	// iterate each command, set its default binding
	for (i = 0; i < g_bindCount; i++)
	{
		g_bindings[i].bind1 = lefthanded ? g_bindings[i].defaultbind1_left : g_bindings[i].defaultbind1_right;
		g_bindings[i].bind2 = lefthanded ? g_bindings[i].defaultbind2_left : g_bindings[i].defaultbind2_right;
	}
}

/**
 * @brief Binding_IDFromName
 * @param[in] name
 * @return
 */
int Binding_IDFromName(const char *name)
{
	int i;

	for (i = 0; i < g_bindCount; i++)
	{
		if (!Q_stricmp(name, g_bindings[i].command))
		{
			return i;
		}
	}

	return -1;
}

/**
 * @brief Binding_Set
 * @param[in] id
 * @param[in] b1
 * @param[in] b2
 */
void Binding_Set(int id, int b1, int b2)
{
	if (id != -1)
	{
		if (b1 != -2)
		{
			int key1;
			int key2;

			// Set the current value of bind1 to bind2
			DC->getKeysForBinding(g_bindings[id].command, &key1, &key2);
			Binding_Set(id, -2, key1);

			Binding_Unset(id, 1);
			g_bindings[id].bind1 = b1;
		}

		if (b2 != -2)
		{
			Binding_Unset(id, 2);
			g_bindings[id].bind2 = b2;
		}
	}
}

/**
 * @brief Binding_Unset
 * @param[in] id
 * @param[in] index
 */
void Binding_Unset(int id, int index)
{
	int key1;
	int key2;

	// Find the keys binded to action
	DC->getKeysForBinding(g_bindings[id].command, &key1, &key2);

	// Unbind the key1 or key2 according to index
	switch (index)
	{
	case 1:
		DC->setBinding(key1, "");
		break;
	case 2:
		DC->setBinding(key2, "");
		break;
	default:
		break;
	}
}

/**
 * @brief Binding_Check
 * @param[in] id
 * @param[in] b1
 * @param[in] key
 * @return
 */
qboolean Binding_Check(int id, qboolean b1, int key)
{
	if (id != -1)
	{
		if (b1)
		{
			if (g_bindings[id].bind1 == key)
			{
				return qtrue;
			}
		}
		else
		{
			if (g_bindings[id].bind2 == key)
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/**
 * @brief Binding_Get
 * @param[in] id
 * @param[in] b1
 * @return
 */
int Binding_Get(int id, qboolean b1)
{
	if (id != -1)
	{
		if (b1)
		{
			return g_bindings[id].bind1;
		}
		else
		{
			return g_bindings[id].bind2;
		}
	}

	return -1;
}

/**
 * @brief Binding_Count
 * @return
 */
int Binding_Count(void)
{
	return g_bindCount;
}

char g_nameBind1[32];
char g_nameBind2[32];

/**
 * @brief Binding_FromName
 * @param[in] cvar
 * @return
 */
char *Binding_FromName(const char *cvar)
{
	int b1, b2;

	DC->getKeysForBinding(cvar, &b1, &b2);

	if (b1 != -1)
	{
		DC->keynumToStringBuf(b1, g_nameBind1, 32);
		Q_strupr(g_nameBind1);

		if (b2 != -1)
		{
			DC->keynumToStringBuf(b2, g_nameBind2, 32);
			Q_strupr(g_nameBind2);
			Q_strcat(g_nameBind1, 32, DC->translateString(" or "));
			Q_strcat(g_nameBind1, 32, g_nameBind2);
		}
	}
	else
	{
		Q_strncpyz(g_nameBind1, "(?" "?" "?)", 32);
	}

	return g_nameBind1;
}

/**
 * @brief Display_KeyBindPending
 * @return
 */
qboolean Display_KeyBindPending(void)
{
	return g_waitingForKey;
}

/**
 * @brief Display_GetContext
 * @return
 *
 * @note Unused
 */
displayContextDef_t *Display_GetContext()
{
	return DC;
}

/**
 * @brief Display_CaptureItem
 * @param[in] x
 * @param[in] y
 * @return
 */
void *Display_CaptureItem(int x, int y)
{
	int i;

	for (i = 0; i < menuCount; i++)
	{
		// turn off focus each item
		// menu->items[i].window.flags &= ~WINDOW_HASFOCUS;
		if (Rect_ContainsPoint(&Menus[i].window.rect, x, y))
		{
			return &Menus[i];
		}
	}

	return NULL;
}

/**
 * @brief Display_MouseMove
 * @param[in,out] p
 * @param[in] x
 * @param[in] y
 * @return
 */
qboolean Display_MouseMove(void *p, int x, int y)
{
	menuDef_t *menu = p;

	if (menu == NULL)
	{
		int i;

		menu = Menu_GetFocused();
		if (menu)
		{
			if (menu->window.flags & WINDOW_POPUP)
			{
				Menu_HandleMouseMove(menu, x, y);
				return qtrue;
			}
		}

		for (i = 0; i < menuCount; i++)
		{
			Menu_HandleMouseMove(&Menus[i], x, y);
		}
	}
	else
	{
		menu->window.rect.x += x;
		menu->window.rect.y += y;
		Menu_UpdatePosition(menu);
	}

	return qtrue;
}

/**
 * @brief Display_CursorType
 * @param[in] x
 * @param[in] y
 * @return
 *
 * @note Unused
 */
int Display_CursorType(int x, int y)
{
	rectDef_t r2;
	int       i;

	for (i = 0; i < menuCount; i++)
	{
		r2.x = Menus[i].window.rect.x - 3;
		r2.y = Menus[i].window.rect.y - 3;
		r2.w = r2.h = 7;
		if (Rect_ContainsPoint(&r2, x, y))
		{
			return CURSOR_SIZER;
		}
	}

	return CURSOR_ARROW;
}

/**
 * @brief Display_HandleKey
 * @param[in] key
 * @param[in] down
 * @param[in] x
 * @param[in] y
 *
 * @note Unused
 */
void Display_HandleKey(int key, qboolean down, int x, int y)
{
	menuDef_t *menu = Display_CaptureItem(x, y);

	if (menu == NULL)
	{
		menu = Menu_GetFocused();
	}

	if (menu)
	{
		Menu_HandleKey(menu, key, down);
	}
}

/**
 * @brief Window_CacheContents
 * @param[in] window
 */
static void Window_CacheContents(windowDef_t *window)
{
	if (window)
	{
		if (window->cinematicName)
		{
			int cin = DC->playCinematic(window->cinematicName, 0, 0, 0, 0);
			DC->stopCinematic(cin);
		}
	}
}

/**
 * @brief Item_CacheContents
 * @param[in] item
 */
static void Item_CacheContents(itemDef_t *item)
{
	if (item)
	{
		Window_CacheContents(&item->window);
	}
}

/**
 * @brief Menu_CacheContents
 * @param[in] menu
 */
static void Menu_CacheContents(menuDef_t *menu)
{
	if (menu)
	{
		int i;

		Window_CacheContents(&menu->window);

		for (i = 0; i < menu->itemCount; i++)
		{
			Item_CacheContents(menu->items[i]);
		}

		if (menu->soundName && *menu->soundName)
		{
			DC->registerSound(menu->soundName, qtrue);
		}
	}
}

/**
 * @brief Display_CacheAll
 */
void Display_CacheAll(void)
{
	int i;

	for (i = 0; i < menuCount; i++)
	{
		Menu_CacheContents(&Menus[i]);
	}
}

/*
 * @brief Translates string
 * @param[in] handle
 * @param[out] out
 * @return
 *
 * @note Unused
qboolean PC_String_Parse_Trans(int handle, const char **out)
{
    pc_token_t token;

    if (!trap_PC_ReadToken(handle, &token))
    {
        return qfalse;
    }

    *(out) = String_Alloc(DC->translateString(token.string));
    return qtrue;
}
*/

/**
 * @brief PC_Rect_Parse
 * @param[in] handle
 * @param[in] r
 * @return
 */
qboolean PC_Rect_Parse(int handle, rectDef_t *r)
{
	if (PC_Float_Parse(handle, &r->x))
	{
		if (PC_Float_Parse(handle, &r->y))
		{
			if (PC_Float_Parse(handle, &r->w))
			{
				if (PC_Float_Parse(handle, &r->h))
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

// Panel Handling
// ======================================================
panel_button_t *bg_focusButton;

/**
 * @brief BG_RectContainsPoint
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] px
 * @param[in] py
 * @return
 */
qboolean BG_RectContainsPoint(float x, float y, float w, float h, float px, float py)
{
	if (px > x && px < x + w && py > y && py < y + h)
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief BG_CursorInRect
 * @param[in] rect
 * @return
 */
qboolean BG_CursorInRect(rectDef_t *rect)
{
	return BG_RectContainsPoint(rect->x, rect->y, rect->w, rect->h, DC->cursorx, DC->cursory);
}

/**
 * @brief BG_PanelButton_RenderEdit
 * @param[in] button
 */
void BG_PanelButton_RenderEdit(panel_button_t *button)
{
	qboolean useCvar = button->data[0] ? qfalse : qtrue;
	int      offset  = -1;

	if (useCvar)
	{
		char buffer[MAX_EDITFIELD + 1];

		trap_Cvar_VariableStringBuffer(button->text, buffer, sizeof(buffer));

		do
		{
			offset++;
			if (buffer[offset] == '\0')
			{
				break;
			}
		}
		while (DC->textWidthExt(buffer + offset, button->font->scalex, 0, button->font->font) > button->rect.w);

		if (BG_PanelButtons_GetFocusButton() == button)
		{
			DC->drawTextWithCursorExt(button->rect.x, button->rect.y + button->rect.h, button->font->scalex, button->font->colour, buffer + (button->data[2] <= offset ? button->data[2] : offset), button->data[2] <= offset ? 0 : button->data[2] - offset, trap_Key_GetOverstrikeMode() ? "_" : "|", offset ? Q_UTF8_Strlen(buffer + offset) : 0, button->font->style, button->font->font);
		}
		else
		{
			DC->drawTextExt(button->rect.x, button->rect.y + button->rect.h, button->font->scalex, button->font->scaley, button->font->colour, buffer + offset, 0, 0, button->font->style, button->font->font);
		}
	}
	else
	{
		char *s = button->text;

		do
		{
			offset++;
			if (s[offset] == '\0')
			{
				break;
			}
		}
		while (DC->textWidthExt(s + offset, button->font->scalex, 0, button->font->font) > button->rect.w);

		if (BG_PanelButtons_GetFocusButton())
		{
			DC->drawTextWithCursorExt(button->rect.x, button->rect.y + button->rect.h, button->font->scalex, button->font->colour, s + (button->data[2] <= offset ? button->data[2] : offset), button->data[2] <= offset ? 0 : button->data[2] - offset, trap_Key_GetOverstrikeMode() ? "_" : "|", offset ? Q_UTF8_Strlen(s + offset) : 0, button->font->style, button->font->font);
		}
		else
		{
			DC->drawTextExt(button->rect.x, button->rect.y + button->rect.h, button->font->scalex, button->font->scaley, button->font->colour, s + offset, 0, 0, button->font->style, button->font->font);
		}
	}
}

/**
 * @brief BG_PanelButton_EditClick
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean BG_PanelButton_EditClick(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (!BG_CursorInRect(&button->rect) && BG_PanelButtons_GetFocusButton() == button)
		{
			BG_PanelButtons_SetFocusButton(NULL);
			if (button->onFinish)
			{
				button->onFinish(button);
			}

			return qfalse;
		}
		else
		{
			BG_PanelButtons_SetFocusButton(button);
			return qtrue;
		}
	}
	else if (BG_PanelButtons_GetFocusButton() != button)
	{
		return qfalse;
	}
	else
	{
		char     buffer[MAX_EDITFIELD];
		char     *s = NULL;
		size_t   len, stringLen, maxlen;
		qboolean useCvar = button->data[0] ? qfalse : qtrue;

		if (useCvar)
		{
			maxlen = sizeof(buffer);
			DC->getCVarString(button->text, buffer, sizeof(buffer));
			s = buffer;
		}
		else
		{
			maxlen = (size_t)button->data[0];
			s      = button->text;
		}

		len       = strlen(s);
		stringLen = Q_UTF8_Strlen(s);

		if (key & K_CHAR_FLAG)
		{
			key &= ~K_CHAR_FLAG;

			if (key == 'h' - 'a' + 1)          // ctrl-h is backspace
			{
				if (len && button->data[2])
				{
					int  offset    = Q_UTF8_ByteOffset(s, button->data[2]);
					char *prev     = Q_UTF8_CharAt(s, button->data[2] - 1);
					int  charWidth = Q_UTF8_Width(prev);
					memmove(s + offset - charWidth, s + offset, len - offset);

					button->data[2]--;

					offset    = Q_UTF8_ByteOffset(s, stringLen - 1);
					s[offset] = '\0';

					if (useCvar)
					{
						trap_Cvar_Set(button->text, buffer);
					}
				}

				return qtrue;
			}

			if (key < 32)
			{
				return qtrue;
			}

			if (button->data[1])
			{
				if (key < '0' || key > '9')
				{
					if (button->data[1] == 2)
					{
						return qtrue;
					}
					else if (!(len == 0 && key == '-'))
					{
						return qtrue;
					}
				}
			}

			if (len >= maxlen - 1)
			{
				return qtrue;
			}

			Q_UTF8_Insert(s, stringLen, button->data[2], key, trap_Key_GetOverstrikeMode());

			if (useCvar)
			{
				trap_Cvar_Set(button->text, buffer);
			}

			button->data[2]++;

			return qtrue;
		}
		else
		{
			if (key == K_DEL || key == K_KP_DEL)
			{
				if (button->data[2] < stringLen)
				{
					int  offset    = Q_UTF8_ByteOffset(s, button->data[2]);
					char *current  = Q_UTF8_CharAt(s, button->data[2]);
					int  charWidth = Q_UTF8_Width(current);

					memmove(s + offset,
					        s + offset + charWidth, len - offset);

					offset    = Q_UTF8_ByteOffset(s, stringLen - 1);
					s[offset] = '\0';

					if (useCvar)
					{
						trap_Cvar_Set(button->text, buffer);
					}
				}

				return qtrue;
			}
			else if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
			{
				if (button->data[2] < stringLen)
				{
					button->data[2]++;
				}

				return qtrue;
			}
			else if (key == K_LEFTARROW || key == K_KP_LEFTARROW)
			{
				if (button->data[2] > 0)
				{
					button->data[2]--;
				}

				return qtrue;
			}
			else if (key == K_HOME || key == K_KP_HOME)
			{
				button->data[2] = 0;
				return qtrue;
			}
			else if (key == K_END || key == K_KP_END)
			{
				button->data[2] = stringLen;
				return qtrue;
			}
			else if (key == K_INS || key == K_KP_INS)
			{
				trap_Key_SetOverstrikeMode(!trap_Key_GetOverstrikeMode());
				return qtrue;
			}
			else if (key == K_ENTER || key == K_KP_ENTER)
			{
				if (button->onFinish)
				{
					button->onFinish(button);
				}

				BG_PanelButtons_SetFocusButton(NULL);
				return qfalse;
			}
		}
	}

	return qtrue;
}

/**
 * @brief BG_PanelButtonsKeyEvent
 * @param[in] key
 * @param[in] down
 * @param[in] buttons
 * @return
 */
qboolean BG_PanelButtonsKeyEvent(int key, qboolean down, panel_button_t **buttons)
{
	panel_button_t *button;

	if (BG_PanelButtons_GetFocusButton())
	{
		for ( ; *buttons; buttons++)
		{
			button = (*buttons);

			if (button == BG_PanelButtons_GetFocusButton())
			{
				if (button->onKeyDown && down)
				{
					if (!button->onKeyDown(button, key))
					{
						if (BG_PanelButtons_GetFocusButton())
						{
							return qfalse;
						}
					}
					else
					{
						return qtrue;
					}
				}

				if (button->onKeyUp && !down)
				{
					if (!button->onKeyUp(button, key))
					{
						if (BG_PanelButtons_GetFocusButton())
						{
							return qfalse;
						}
					}
					else
					{
						return qtrue;
					}
				}
			}
		}
	}

	if (down)
	{
		for ( ; *buttons; buttons++)
		{
			button = (*buttons);

			if (button->onKeyDown)
			{
				if (BG_CursorInRect(&button->rect))
				{
					if (button->onKeyDown(button, key))
					{
						return qtrue;
					}
				}
			}
		}
	}
	else
	{
		for ( ; *buttons; buttons++)
		{
			button = (*buttons);

			if (button->onKeyUp && BG_CursorInRect(&button->rect))
			{
				if (button->onKeyUp(button, key))
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

/**
 * @brief BG_PanelButtonsSetup
 * @param[in,out] buttons
 */
void BG_PanelButtonsSetup(panel_button_t **buttons)
{
	panel_button_t *button;

	for ( ; *buttons; buttons++)
	{
		button = (*buttons);

		if (button->shaderNormal)
		{
			button->hShaderNormal = trap_R_RegisterShaderNoMip(button->shaderNormal);
		}
	}
}

/**
 * @brief BG_PanelButtonsGetHighlightButton
 * @param[in] buttons
 * @return
 */
panel_button_t *BG_PanelButtonsGetHighlightButton(panel_button_t **buttons)
{
	panel_button_t *button;

	for ( ; *buttons; buttons++)
	{
		button = (*buttons);

		if (button->onKeyDown && BG_CursorInRect(&button->rect))
		{
			return button;
		}
	}

	return NULL;
}

/**
 * @brief BG_PanelButtonsRender
 * @param[in] buttons
 */
void BG_PanelButtonsRender(panel_button_t **buttons)
{
	panel_button_t *button;

	for ( ; *buttons; buttons++)
	{
		button = (*buttons);

		if (button->onDraw)
		{
			button->onDraw(button);
		}
	}
}

/**
 * @brief BG_PanelButtonsRender_TextExt
 * @param[in] button
 * @param[in] text
 */
void BG_PanelButtonsRender_TextExt(panel_button_t *button, const char *text)
{
	float x = button->rect.x;

	if (!button->font)
	{
		return;
	}

	if (button->font->align == ITEM_ALIGN_CENTER)
	{
		x += ((button->rect.w - DC->textWidthExt(text, button->font->scalex, 0, button->font->font)) * 0.5f);
	}
	else if (button->font->align == ITEM_ALIGN_RIGHT)
	{
		x += button->rect.w - DC->textWidthExt(text, button->font->scalex, 0, button->font->font);
	}

	if (button->data[1])
	{
		vec4_t clrBdr = { 0.5f, 0.5f, 0.5f, 1.f };
		vec4_t clrBck = { 0.f, 0.f, 0.f, 0.8f };

		DC->fillRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, clrBck);
		DC->drawRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1, clrBdr);
	}

	DC->drawTextExt(x, button->rect.y + button->data[0], button->font->scalex, button->font->scaley, button->font->colour, text, 0, 0, button->font->style, button->font->font);
}

/**
 * @brief BG_PanelButtonsRender_Text
 * @param[in] button
 */
void BG_PanelButtonsRender_Text(panel_button_t *button)
{
	BG_PanelButtonsRender_TextExt(button, button->text);
}

/**
 * @brief BG_PanelButtonsRender_Img
 * @param[in] button
 */
void BG_PanelButtonsRender_Img(panel_button_t *button)
{
	vec4_t clr = { 1.f, 1.f, 1.f, 1.f };

	if (button->data[0])
	{
		clr[0] = button->data[1] / 255.f;
		clr[1] = button->data[2] / 255.f;
		clr[2] = button->data[3] / 255.f;
		clr[3] = button->data[4] / 255.f;

		trap_R_SetColor(clr);
	}

	if (button->data[5])
	{
		DC->drawRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1, clr);
	}
	else
	{
		DC->drawHandlePic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, button->hShaderNormal);
	}

	if (button->data[0])
	{
		trap_R_SetColor(NULL);
	}
}

/**
 * @brief BG_PanelButtons_GetFocusButton
 * @return
 */
panel_button_t *BG_PanelButtons_GetFocusButton(void)
{
	return bg_focusButton;
}

/**
 * @brief BG_PanelButtons_SetFocusButton
 * @param button
 */
void BG_PanelButtons_SetFocusButton(panel_button_t *button)
{
	bg_focusButton = button;
}

/**
 * @brief BG_FitTextToWidth_Ext
 * @param[in,out] instr
 * @param[in] scale
 * @param[in] w
 * @param[in] size
 * @param[in] font
 */
void BG_FitTextToWidth_Ext(char *instr, float scale, float w, size_t size, fontHelper_t *font)
{
	char buffer[1024];
	char *s, *p, *c, *ls = NULL;

	Q_strncpyz(buffer, instr, 1024);
	Com_Memset(instr, 0, size);

	c = s = instr;
	p = buffer;

	while (*p)
	{
		*c = *p++;

		if (*c == ' ')
		{
			ls = c;
		} // store last space, to try not to break mid word

		c++;

		if (*p == '\n')
		{
			s = c + 1;
		}
		else if (DC->textWidthExt(s, scale, 0, font) > w)
		{
			if (ls)
			{
				*ls = '\n';
				s   = ls + 1;
			}
			else
			{
				*c       = *(c - 1);
				*(c - 1) = '\n';
				s        = c++;
			}

			ls = NULL;
		}
	}

	if (c != buffer && (*(c - 1) != '\n'))
	{
		*c++ = '\n';
	}

	*c = '\0';
}

/**
 * @brief Adjusting panel coordinates so it is horizontally centered.
 * @param[in,out] buttons
 * @param[in] xoffset
 */
void C_PanelButtonsSetup(panel_button_t **buttons, float xoffset)
{
	panel_button_t *button;

	if (xoffset == 0.0f)
	{
		return;
	}

	for ( ; *buttons; buttons++)
	{
		button          = (*buttons);
		button->rect.x += xoffset;
	}
}
