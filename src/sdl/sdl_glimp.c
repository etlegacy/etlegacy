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
 * @file sdl_glimp.c
 */

#include "sdl_defs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../sys/sys_local.h"
#include "../client/client.h"
#include "sdl_icon.h"
#include "sdl_splash.h"

#ifdef __APPLE__
#define MACOS_X_GAMMA_RESET_FIX
#ifdef MACOS_X_GAMMA_RESET_FIX
static int gammaResetTime = 0;
#endif
#endif // __APPLE__

static int GLimp_CompareModes(const void *a, const void *b);

SDL_Window           *main_window   = NULL;
static SDL_Renderer  *main_renderer = NULL;
static SDL_GLContext SDL_glContext  = NULL;
static float         displayAspect  = 0.f;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable

// Window cvars
cvar_t *r_fullscreen = 0;
cvar_t *r_noBorder;
cvar_t *r_centerWindow;
cvar_t *r_customwidth;
cvar_t *r_customheight;
cvar_t *r_swapInterval;
cvar_t *r_mode;
cvar_t *r_customaspect;
cvar_t *r_displayRefresh;
cvar_t *r_windowLocation;

// Window surface cvars
cvar_t *r_stencilbits;  // number of desired stencil bits
cvar_t *r_depthbits;  // number of desired depth bits
cvar_t *r_colorbits;  // number of desired color bits, only relevant for fullscreen
cvar_t *r_ignorehwgamma;
cvar_t *r_ext_multisample;

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_OLD_GL,

	RSERR_UNKNOWN
} rserr_t;

typedef struct vidmode_s
{
	const char *description;
	int width, height;
	float pixelAspect;              // pixel width / height
} vidmode_t;

vidmode_t glimp_vidModes[] =        // keep in sync with LEGACY_RESOLUTIONS
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
	{ "Mode 11: 856x480 (16:9)",    856,  480,  1 },
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
static int s_numVidModes = ARRAY_LEN(glimp_vidModes);

/**
 * @brief GLimp_MainWindow
 * @return
 */
void *GLimp_MainWindow(void)
{
	return main_window;
}

/**
 * @brief Minimize the game so that user is back at the desktop
 */
void GLimp_Minimize(void)
{
	SDL_MinimizeWindow(main_window);
}

/**
 * @brief GLimp_GetModeInfo
 * @param[in,out] width
 * @param[in,out] height
 * @param[out] windowAspect
 * @param[in] mode
 * @return
 */
qboolean GLimp_GetModeInfo(int *width, int *height, float *windowAspect, int mode)
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
		vm = &glimp_vidModes[mode];

		*width      = vm->width;
		*height     = vm->height;
		pixelAspect = vm->pixelAspect;
	}

	*windowAspect = (float)*width / (*height * pixelAspect);

	return qtrue;
}

#define GLimp_ResolutionToFraction(resolution) GLimp_RatioToFraction((double) resolution.w / (double) resolution.h, 150)

/**
 * @brief Figures out the best possible fraction string for the given resolution ratio
 * @param ratio resolution ratio (resolution width / resolution height)
 * @param iterations how many iterations should it be allowed to run
 * @return fraction value formatted into a string (static stack)
 */
static char *GLimp_RatioToFraction(const double ratio, const int iterations)
{
	static char buff[64];
	int i;
	double bestDelta = DBL_MAX;
	unsigned int numerator = 1;
	unsigned int denominator = 1;
	unsigned int bestNumerator = 0;
	unsigned int bestDenominator = 0;

	for (i = 0; i < iterations; i++)
	{
		double delta = (double) numerator / (double) denominator - ratio;

		// Close enough for most resolutions
		if(fabs(delta) < 0.002)
		{
			break;
		}

		if (delta < 0)
		{
			numerator++;
		}
		else
		{
			denominator++;
		}

		double newDelta = fabs((double) numerator / (double) denominator - ratio);
		if (newDelta < bestDelta)
		{
			bestDelta = newDelta;
			bestNumerator = numerator;
			bestDenominator = denominator;
		}
	}

	sprintf(buff, "%u/%u", bestNumerator, bestDenominator);
	Com_DPrintf("%f -> %s\n", ratio, buff);
	return buff;
}

/**
* @brief Prints hardcoded screen resolutions
* @see r_availableModes for supported resolutions
*/
void GLimp_ModeList_f(void)
{
	int i, j, display, numModes = 0;
	SDL_Rect        modes[128];
	SDL_DisplayMode windowMode;

	Com_Printf("\n");
	Com_Printf((r_mode->integer == -2) ? "%s ^2(current)\n" : "%s\n",
	           "Mode -2: desktop resolution");
	Com_Printf((r_mode->integer == -1) ? "%s ^2(current)\n" : "%s\n",
	           "Mode -1: custom resolution");
	for (i = 0; i < s_numVidModes; i++)
	{
		Com_Printf((i == r_mode->integer) ? "%s ^2(current)\n" : "%s\n",
		           glimp_vidModes[i].description);
	}

	Com_Printf("\n" S_COLOR_GREEN "SDL detected modes:\n");

	display = SDL_GetWindowDisplayIndex(main_window);

	if (SDL_GetDesktopDisplayMode(display, &windowMode) < 0)
	{
		Com_Printf(S_COLOR_YELLOW "Couldn't get desktop display mode, no resolutions detected - %s\n", SDL_GetError());
		return;
	}

	for (i = 0; i < SDL_GetNumDisplayModes(display); i++)
	{
		SDL_DisplayMode mode;

		if (SDL_GetDisplayMode(display, i, &mode) < 0)
		{
			continue;
		}

		if (!mode.w || !mode.h)
		{
			Com_Printf("Display supports any resolution\n");
			return;
		}

		if (windowMode.format != mode.format)
		{
			continue;
		}

		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for (j = 0; j < numModes; j++)
		{
			if (mode.w == modes[j].w && mode.h == modes[j].h)
			{
				break;
			}
		}

		if (j != numModes)
		{
			continue;
		}

		modes[numModes].w = mode.w;
		modes[numModes].h = mode.h;
		numModes++;
	}

	if (numModes > 1)
	{
		qsort(modes, numModes, sizeof(SDL_Rect), GLimp_CompareModes);
	}

	for (i = 0; i < numModes; i++)
	{
		const char *newModeString = va("%ux%u ", modes[i].w, modes[i].h);
		Com_Printf("Mode XX: %s (%s)\n", newModeString, GLimp_ResolutionToFraction(modes[i]));
	}


	Com_Printf("\n");
}

/**
 * @brief GLimp_InitCvars
 */
static void GLimp_InitCvars(void)
{
	//r_sdlDriver = Cvar_Get("r_sdlDriver", "", CVAR_ROM);
	r_allowSoftwareGL = Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH);
	r_allowResize     = Cvar_Get("r_allowResize", "0", CVAR_ARCHIVE);

	// Window cvars
	r_fullscreen     = Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_noBorder       = Cvar_Get("r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_centerWindow   = Cvar_Get("r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_customwidth    = Cvar_Get("r_customwidth", "1280", CVAR_ARCHIVE | CVAR_LATCH);
	r_customheight   = Cvar_Get("r_customheight", "720", CVAR_ARCHIVE | CVAR_LATCH);
	r_swapInterval   = Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE);
	r_mode           = Cvar_Get("r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_customaspect   = Cvar_Get("r_customaspect", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_displayRefresh = Cvar_Get("r_displayRefresh", "0", CVAR_LATCH);
	Cvar_CheckRange(r_displayRefresh, 0, 240, qtrue);
	r_windowLocation = Cvar_Get("r_windowLocation", "0,-1,-1", CVAR_ARCHIVE | CVAR_PROTECTED);

	// Window render surface cvars
	r_stencilbits     = Cvar_Get("r_stencilbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_depthbits       = Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_colorbits       = Cvar_Get("r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_ignorehwgamma   = Cvar_Get("r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	r_ext_multisample = Cvar_Get("r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE);
	Cvar_CheckRange(r_ext_multisample, 0, 8, qtrue);

	// Old modes (these are used by the UI code)
	Cvar_Get("r_oldFullscreen", "", CVAR_ARCHIVE);
	Cvar_Get("r_oldMode", "", CVAR_ARCHIVE);

	Cmd_AddCommand("modelist", GLimp_ModeList_f, "Prints a list of available resolutions/modes.");
	Cmd_AddCommand("minimize", GLimp_Minimize, "Minimizes the game window.");
}

/**
 * @brief GLimp_Shutdown
 */
void GLimp_Shutdown(void)
{
	IN_Shutdown();

	if (main_renderer)
	{
		SDL_DestroyRenderer(main_renderer);
		main_renderer = NULL;
	}

	if (main_window)
	{
		int tmpX = SDL_WINDOWPOS_UNDEFINED, tmpY = SDL_WINDOWPOS_UNDEFINED;
		int displayIndex = SDL_GetWindowDisplayIndex(main_window);
		SDL_GetWindowPosition(main_window, &tmpX, &tmpY);
		Cvar_Set("r_windowLocation", va("%d,%d,%d", displayIndex, tmpX, tmpY));

		SDL_DestroyWindow(main_window);
		main_window = NULL;
	}

	Cmd_RemoveCommand("modelist");
	Cmd_RemoveCommand("minimize");

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

/**
 * @brief GLimp_CompareModes
 * @param[in] a
 * @param[in] b
 * @return
 */
static int GLimp_CompareModes(const void *a, const void *b)
{
	const float    ASPECT_EPSILON  = 0.001f;
	const SDL_Rect *modeA          = (const SDL_Rect *)a;
	const SDL_Rect *modeB          = (const SDL_Rect *)b;
	float          aspectA         = modeA->w / (float)modeA->h;
	float          aspectB         = modeB->w / (float)modeB->h;
	int            areaA           = modeA->w * modeA->h;
	int            areaB           = modeB->w * modeB->h;
	float          aspectDiffA     = Q_fabs(aspectA - displayAspect);
	float          aspectDiffB     = Q_fabs(aspectB - displayAspect);
	float          aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if (aspectDiffsDiff > ASPECT_EPSILON)
	{
		return 1;
	}
	else if (aspectDiffsDiff < -ASPECT_EPSILON)
	{
		return -1;
	}
	else
	{
		return areaA - areaB;
	}
}

/**
 * @brief GLimp_DetectAvailableModes
 */
static void GLimp_DetectAvailableModes(void)
{
	int             i, j;
	char            buf[MAX_STRING_CHARS] = { 0 };
	SDL_Rect        modes[128];
	int             numModes = 0;
	int             display  = 0;
	SDL_DisplayMode windowMode;

	if (!main_window)
	{
		if (!SDL_GetNumVideoDisplays())
		{
			Com_Error(ERR_VID_FATAL, "There is no available display to open a game screen - %s", SDL_GetError());
		}

		// Use the zero display index
		display = 0;
	}
	else
	{
		// Detect the used display
		display = SDL_GetWindowDisplayIndex(main_window);
	}

	// was SDL_GetWindowDisplayMode
	if (SDL_GetDesktopDisplayMode(display, &windowMode) < 0)
	{
		Com_Printf(S_COLOR_YELLOW "Couldn't get desktop display mode, no resolutions detected - %s\n", SDL_GetError());
		return;
	}

	for (i = 0; i < SDL_GetNumDisplayModes(display); i++)
	{
		SDL_DisplayMode mode;

		if (SDL_GetDisplayMode(display, i, &mode) < 0)
		{
			continue;
		}

		if (!mode.w || !mode.h)
		{
			Com_Printf("Display supports any resolution\n");
			return;
		}

		if (windowMode.format != mode.format)
		{
			continue;
		}

		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for (j = 0; j < numModes; j++)
		{
			if (mode.w == modes[j].w && mode.h == modes[j].h)
			{
				break;
			}
		}

		if (j != numModes)
		{
			continue;
		}

		modes[numModes].w = mode.w;
		modes[numModes].h = mode.h;
		numModes++;
	}

	if (numModes > 1)
	{
		qsort(modes, numModes, sizeof(SDL_Rect), GLimp_CompareModes);
	}

	for (i = 0; i < numModes; i++)
	{
		const char *newModeString = va("%ux%u ", modes[i].w, modes[i].h);

		if (strlen(newModeString) < (int)sizeof(buf) - strlen(buf))
		{
			Q_strcat(buf, sizeof(buf), newModeString);
		}
		else
		{
			Com_Printf(S_COLOR_YELLOW "Skipping mode %ux%u, buffer too small\n", modes[i].w, modes[i].h);
		}
	}

	if (*buf)
	{
		buf[strlen(buf) - 1] = 0;
		Com_Printf("Available modes [%i]: '%s'\n", numModes, buf);
		Cvar_Set("r_availableModes", buf);
	}
}

/**
 * @brief Setup the window location based on the previous sessions location and display
 * @param glConfig[in] current gl configuration
 * @param x[in,out] X location
 * @param y[in,out] Y location
 * @param fullscreen requested to run fullscreen
 */
static void GLimp_WindowLocation(glconfig_t *glConfig, int *x, int *y, const qboolean fullscreen)
{
	int displayIndex = 0, tmpX = SDL_WINDOWPOS_UNDEFINED, tmpY = SDL_WINDOWPOS_UNDEFINED;
	int numDisplays = SDL_GetNumVideoDisplays();

	if (!r_windowLocation->string || !r_windowLocation->string[0])
	{
		// Center window
		if (r_centerWindow->integer && !fullscreen)
		{
			*x = SDL_WINDOWPOS_CENTERED;
			*y = SDL_WINDOWPOS_CENTERED;
		}
		else
		{
			*x = SDL_WINDOWPOS_UNDEFINED;
			*y = SDL_WINDOWPOS_UNDEFINED;
		}
		return;
	}

	// We might be in headless mode, just ignore for now (xD)
	if (numDisplays < 0)
	{
		numDisplays = 1;
	}

	if (sscanf(r_windowLocation->string, "%d,%d,%d", &displayIndex, &tmpX, &tmpY) != 3)
	{
		return;
	}

	if (displayIndex < 0 || displayIndex >= numDisplays)
	{
		// Center window
		if (r_centerWindow->integer && !fullscreen)
		{
			*x = SDL_WINDOWPOS_CENTERED;
			*y = SDL_WINDOWPOS_CENTERED;
		}
		else
		{
			*x = SDL_WINDOWPOS_UNDEFINED;
			*y = SDL_WINDOWPOS_UNDEFINED;
		}
		return;
	}

	// Center window
	if (r_centerWindow->integer && !fullscreen)
	{
		*x = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
		*y = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
		return;
	}

	if (fullscreen || r_mode->integer == -2)
	{
		*x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex);
		*y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex);
		return;
	}

	// Default values so we skip out with safe values
	if (tmpX == -1 && tmpY == -1)
	{
		*x = SDL_WINDOWPOS_UNDEFINED;
		*y = SDL_WINDOWPOS_UNDEFINED;
		return;
	}

	SDL_Rect rect;
	SDL_GetDisplayBounds(displayIndex, &rect);

	// SDL resets the values to displays origins when switching between windowed and fullscreen, so just move it a bit
	if (tmpX == rect.x && tmpY == rect.y)
	{
		*x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex);
		*y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex);
		return;
	}

	// Make sure we have at least half of the game screen visible on the display its supposed to be in
	if ((tmpX + (glConfig->vidWidth / 2)) > rect.x && (tmpX + (glConfig->vidWidth / 2)) < (rect.x + rect.w)
	    && (tmpY + (glConfig->vidHeight / 2)) > rect.y && (tmpY + (glConfig->vidHeight / 2)) < (rect.y + rect.h))
	{
		*x = tmpX;
		*y = tmpY;

		return;
	}

	*x = SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex);
	*y = SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex);
}

/**
 * @brief GLimp_SetMode
 * @param[in,out] glConfig
 * @param[in] mode
 * @param[in] fullscreen
 * @param[in] noborder
 * @param[in] context
 * @return
 */
static int GLimp_SetMode(glconfig_t *glConfig, int mode, qboolean fullscreen, qboolean noborder, windowContext_t *context)
{
	int             perChannelColorBits;
	int             colorBits, depthBits, stencilBits;
	int             samples;
	int             i     = 0;
	SDL_Surface     *icon = NULL;
	SDL_DisplayMode desktopMode;
	int             display = 0;
	int             x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;

	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_GRABBED;

	Com_Printf("Initializing OpenGL display\n");

	if (r_allowResize->integer && !fullscreen)
	{
		flags |= SDL_WINDOW_RESIZABLE;
	}

	icon = SDL_CreateRGBSurfaceFrom(
		(void *)CLIENT_WINDOW_ICON.pixel_data,
		CLIENT_WINDOW_ICON.width,
		CLIENT_WINDOW_ICON.height,
		CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
		CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
		0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
		0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
		);

	// If a window exists, note its display index
	if (main_window != NULL)
	{
		display = SDL_GetWindowDisplayIndex(main_window);
	}

	if (SDL_GetDesktopDisplayMode(display, &desktopMode) == 0)
	{
		displayAspect = (double)desktopMode.w / (double)desktopMode.h;

		Com_Printf("Estimated display aspect: %.3f\n", displayAspect);
	}
	else
	{
		Com_Memset(&desktopMode, 0, sizeof(SDL_DisplayMode));

		Com_Printf("Cannot estimate display aspect, assuming 1.333\n");
	}

	Com_Printf("...setting mode %d: ", mode);

	if (mode == -2)
	{
		// use desktop video resolution
		if (desktopMode.h > 0)
		{
			glConfig->vidWidth  = desktopMode.w;
			glConfig->vidHeight = desktopMode.h;
		}
		else
		{
			glConfig->vidWidth  = 640;
			glConfig->vidHeight = 480;
			Com_Printf("Cannot determine display resolution, assuming 640x480\n");
		}

		glConfig->windowAspect = (float)glConfig->vidWidth / (float)glConfig->vidHeight;
	}
	else if (!GLimp_GetModeInfo(&glConfig->vidWidth, &glConfig->vidHeight, &glConfig->windowAspect, mode))
	{
		Com_Printf("invalid mode\n");
		return RSERR_INVALID_MODE;
	}
	Com_Printf("%dx%d\n", glConfig->vidWidth, glConfig->vidHeight);

	GLimp_WindowLocation(glConfig, &x, &y, fullscreen);

	// Destroy existing state if it exists
	if (SDL_glContext != NULL)
	{
		SDL_GL_DeleteContext(SDL_glContext);
		SDL_glContext = NULL;
	}

	if (main_window != NULL)
	{
		SDL_GetWindowPosition(main_window, &x, &y);
		Com_Printf(S_COLOR_YELLOW "Existing window at %dx%d before being destroyed\n", x, y);
		SDL_DestroyWindow(main_window);
		main_window = NULL;
	}

	if (fullscreen)
	{
		if (r_mode->integer == -2)
		{
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		else
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		glConfig->isFullscreen = qtrue;
	}
	else
	{
		if (noborder)
		{
			flags |= SDL_WINDOW_BORDERLESS;
		}

		glConfig->isFullscreen = qfalse;
	}

	colorBits = r_colorbits->integer;
	if ((!colorBits) || (colorBits >= 32))
	{
		colorBits = 24;
	}

	if (r_depthbits->value == 0.f)
	{
		depthBits = 24;
	}
	else
	{
		depthBits = r_depthbits->integer;
	}
	stencilBits = r_stencilbits->integer;
	samples     = r_ext_multisample->integer;

	for (i = 0; i < 16; i++)
	{
		int testColorBits, testDepthBits, testStencilBits;

		// 0 - default
		// 1 - minus colorBits
		// 2 - minus depthBits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
			case 2:
				if (colorBits == 24)
				{
					colorBits = 16;
				}
				break;
			case 1:
				if (depthBits == 32)
				{
					depthBits = 24;
				}
				else if (depthBits == 24)
				{
					depthBits = 16;
				}
				else if (depthBits == 16)
				{
					depthBits = 8;
				}
			case 3: // fall through
				if (stencilBits == 24)
				{
					stencilBits = 16;
				}
				else if (stencilBits == 16)
				{
					stencilBits = 8;
				}
			}
		}

		testColorBits   = colorBits;
		testDepthBits   = depthBits;
		testStencilBits = stencilBits;

		if ((i % 4) == 3) // reduce colorbits
		{
			if (testColorBits == 24)
			{
				testColorBits = 16;
			}
		}

		if ((i % 4) == 2) // reduce depthbits
		{
			if (testDepthBits == 24)
			{
				testDepthBits = 16;
			}
			else if (testDepthBits == 16)
			{
				testDepthBits = 8;
			}
		}

		if ((i % 4) == 1) // reduce stencilbits
		{
			if (testStencilBits == 24)
			{
				testStencilBits = 16;
			}
			else if (testStencilBits == 16)
			{
				testStencilBits = 8;
			}
			else
			{
				testStencilBits = 0;
			}
		}

		if (testColorBits == 24)
		{
			perChannelColorBits = 8;
		}
		else
		{
			perChannelColorBits = 4;
		}

#ifdef __sgi // Fix for SGIs grabbing too many bits of color
		if (perChannelColorBits == 4)
		{
			perChannelColorBits = 0; /* Use minimum size for 16-bit color */

		}
		// Need alpha or else SGIs choose 36+ bit RGB mode
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
#endif

#ifdef FEATURE_RENDERER_GLES
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, testDepthBits);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, testStencilBits);

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);

#ifdef __ANDROID__
		// Android complained about E/libEGL: called unimplemented OpenGL ES API
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);
#endif

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		// If not allowing software GL, demand accelerated
		if (!r_allowSoftwareGL->integer)
		{
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		}

		main_window = SDL_CreateWindow(CLIENT_WINDOW_TITLE, x, y, glConfig->vidWidth, glConfig->vidHeight, flags | SDL_WINDOW_SHOWN);

		if (!main_window)
		{
			Com_Printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
			continue;
		}

		if (fullscreen)
		{
			SDL_DisplayMode modefullScreen;

			switch (testColorBits)
			{
			case 16: modefullScreen.format = SDL_PIXELFORMAT_RGB565; break;
			case 24: modefullScreen.format = SDL_PIXELFORMAT_RGB24;  break;
			default: Com_Printf("SDL_SetWindowDisplayMode failed: testColorBits is %d, can't fullscreen\n", testColorBits); continue;
			}

			modefullScreen.w            = glConfig->vidWidth;
			modefullScreen.h            = glConfig->vidHeight;
			modefullScreen.refresh_rate = glConfig->displayFrequency = r_displayRefresh->integer;
			modefullScreen.driverdata   = NULL;

			if (SDL_SetWindowDisplayMode(main_window, &modefullScreen) < 0)
			{
				Com_Printf("SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError());
				continue;
			}
		}

		SDL_SetWindowIcon(main_window, icon);

#ifndef FEATURE_RENDERER_GLES
		if (context && context->versionMajor > 0)
		{
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, context->versionMajor);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, context->versionMinor);

			switch (context->context)
			{
			case GL_CONTEXT_COMP:
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
				break;
			case GL_CONTEXT_CORE:
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
				break;
			case GL_CONTEXT_EGL:
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
				break;
			case GL_CONTEXT_DEFAULT:
			default:
				break;
			}
		}
#endif

		if ((SDL_glContext = SDL_GL_CreateContext(main_window)) == NULL)
		{
			Com_Printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
			continue;
		}

		if (SDL_GL_MakeCurrent(main_window, SDL_glContext) < 0)
		{
			Com_Printf("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
		}

		if (SDL_GL_SetSwapInterval(r_swapInterval->integer) == -1)
		{
			Com_Printf("SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError());
		}

		glConfig->colorBits   = testColorBits;
		glConfig->depthBits   = testDepthBits;
		glConfig->stencilBits = testStencilBits;

		Com_Printf("Using %d color bits, %d depth, %d stencil display\n",
		           glConfig->colorBits, glConfig->depthBits, glConfig->stencilBits);
		break;
	}

	GLimp_DetectAvailableModes();

	if (!main_window) //|| !main_renderer)
	{
		Com_Printf("Couldn't get a visual\n");
		return RSERR_INVALID_MODE;
	}

	if (!re.InitOpenGLSubSystem())
	{
		Com_Printf("Too old OpenGL driver or hardware");
		return RSERR_OLD_GL;
	}

	SDL_FreeSurface(icon);

	return RSERR_OK;
}

/**
 * @brief GLimp_StartDriverAndSetMode
 * @param[in] glConfig
 * @param[in] mode
 * @param[in] fullscreen
 * @param[in] noborder
 * @param[in] context
 * @return
 */
static qboolean GLimp_StartDriverAndSetMode(glconfig_t *glConfig, int mode, qboolean fullscreen, qboolean noborder, windowContext_t *context)
{
	rserr_t err;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			Com_Printf("SDL_Init(SDL_INIT_VIDEO) FAILED (%s)\n", SDL_GetError());
			return qfalse;
		}

		Com_Printf("SDL initialized driver \"%s\"\n", SDL_GetCurrentVideoDriver());
	}

	if (fullscreen && Cvar_VariableIntegerValue("in_nograb"))
	{
		Com_Printf("Fullscreen not allowed with in_nograb 1\n");
		Cvar_Set("r_fullscreen", "0");
		r_fullscreen->modified = qfalse;
		fullscreen             = qfalse;
	}

	err = GLimp_SetMode(glConfig, mode, fullscreen, noborder, context);

	switch (err)
	{
	case RSERR_OK:
		return qtrue;
	case RSERR_INVALID_FULLSCREEN:
		Com_Printf("...WARNING: fullscreen unavailable in this mode\n");
		break;
	case RSERR_INVALID_MODE:
		Com_Printf("...WARNING: could not set the given mode (%d)\n", mode);
		break;
	case RSERR_OLD_GL:
		Com_Error(ERR_VID_FATAL, "Could not create OpenGL context");
	case RSERR_UNKNOWN: // fall through
	default:
		Com_Error(ERR_VID_FATAL, "Can't set mode - an unknown error occured");
	}

	return qfalse;
}

#define R_MODE_FALLBACK 4 // 800 * 600

/**
 * @brief GLimp_Splash
 * @param[in] glConfig
 * @return
 */
void GLimp_Splash(glconfig_t *glConfig)
{
	unsigned char splashData[SPLASH_DATA_SIZE]; // width * height * bytes_per_pixel
	SDL_Surface   *splashImage = NULL;

	// decode splash image
	SPLASH_IMAGE_RUN_LENGTH_DECODE(splashData,
	                               CLIENT_WINDOW_SPLASH.rle_pixel_data,
	                               CLIENT_WINDOW_SPLASH.width * CLIENT_WINDOW_SPLASH.height,
	                               CLIENT_WINDOW_SPLASH.bytes_per_pixel);

	// get splash image
	splashImage = SDL_CreateRGBSurfaceFrom(
		(void *)splashData,
		CLIENT_WINDOW_SPLASH.width,
		CLIENT_WINDOW_SPLASH.height,
		CLIENT_WINDOW_SPLASH.bytes_per_pixel * 8,
		CLIENT_WINDOW_SPLASH.bytes_per_pixel * CLIENT_WINDOW_SPLASH.width,
#ifdef Q3_LITTLE_ENDIAN
		0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
		0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
		);

	SDL_Rect dstRect;
	dstRect.x = glConfig->vidWidth / 2 - splashImage->w / 2;
	dstRect.y = glConfig->vidHeight / 2 - splashImage->h / 2;
	dstRect.w = splashImage->w;
	dstRect.h = splashImage->h;

	SDL_Surface *surface = SDL_GetWindowSurface(main_window);
	if(!surface)
	{
		// This happens on some platforms, most likely just the SDL build lacking renderers. Does not really matter tho.
		// the user just wont see our awesome splash screen, but the renderer should boot up just fine.
		// FIXME: maybe checkup on this later on if there's something we should change on the bundled sdl compile settings
		Com_DPrintf(S_COLOR_YELLOW "Could not get fetch SDL surface: %s\n", SDL_GetError() );
	}
	else if (SDL_BlitSurface(splashImage, NULL, surface, &dstRect) == 0) // apply image on surface
	{
		SDL_UpdateWindowSurface(main_window);
	}
	else
	{
		Com_Printf(S_COLOR_YELLOW "SDL_BlitSurface failed - %s\n", SDL_GetError());
	}

	SDL_FreeSurface(splashImage);
}

/**
 * @brief This routine is responsible for initializing the OS specific portions of OpenGL
 * @param[in,out] glConfig
 * @param[in] context
 */
void GLimp_Init(glconfig_t *glConfig, windowContext_t *context)
{
	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);

	Com_Printf("SDL build version %d.%d.%d - link version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch, linked.major, linked.minor, linked.patch);

	GLimp_InitCvars();

	if (Cvar_VariableIntegerValue("com_abnormalExit"))
	{
		Cvar_Set("r_mode", va("%d", R_MODE_FALLBACK));
		Cvar_Set("r_fullscreen", "0");
		Cvar_Set("r_centerWindow", "0");
		Cvar_Set("com_abnormalExit", "0");
	}

	Sys_GLimpInit();

	// Create the window and set up the context
	if (GLimp_StartDriverAndSetMode(glConfig, r_mode->integer, (qboolean) !!r_fullscreen->integer, (qboolean) !!r_noBorder->integer, context))
	{
		goto success;
	}

	// Try again, this time in a platform specific "safe mode"
	Sys_GLimpSafeInit();

	if (GLimp_StartDriverAndSetMode(glConfig, r_mode->integer, (qboolean) !!r_fullscreen->integer, qfalse, context))
	{
		goto success;
	}

	// Finally, try the default screen resolution
	if (r_mode->integer != R_MODE_FALLBACK)
	{
		Com_Printf("Setting r_mode %d failed, falling back on r_mode %d\n", r_mode->integer, R_MODE_FALLBACK);
		if (GLimp_StartDriverAndSetMode(glConfig, R_MODE_FALLBACK, qfalse, qfalse, context))
		{
			goto success;
		}
	}

	// Nothing worked, give up
	Com_Error(ERR_VID_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n");

success:
	// Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
	glConfig->deviceSupportsGamma = !r_ignorehwgamma->integer && SDL_SetWindowBrightness(main_window, 1.0f) >= 0;

	re.InitOpenGL();

	Cvar_Get("r_availableModes", "", CVAR_ROM);

#if defined(__APPLE__) && !defined(BUNDLED_SDL)
	// When running on system SDL2 on OSX the cocoa driver causes
	// the splash screen to stay on top of the rendering (at least when running from CLion)
	// FIXME: clear the splash? Does not seem to happen with bundled SDL2.
	const char *driver = SDL_GetCurrentVideoDriver();
	if(Q_stricmpn("cocoa", driver, 5))
	{
		GLimp_Splash(glConfig);
	}
#else
#ifndef __ANDROID__
	// Display splash screen
	GLimp_Splash(glConfig);
#endif
#endif

	// This depends on SDL_INIT_VIDEO, hence having it here
	IN_Init();
}

#ifdef MACOS_X_GAMMA_RESET_FIX
extern int CL_ScaledMilliseconds(void);
#endif

/**
 * @brief Responsible for doing a swapbuffers
 */
void GLimp_EndFrame(void)
{
	// don't flip if drawing to front buffer
	//FIXME: remove this nonesense
	if (Q_stricmp(Cvar_VariableString("r_drawBuffer"), "GL_FRONT") != 0)
	{
		SDL_GL_SwapWindow(main_window);
	}

	if (r_fullscreen->modified)
	{
		qboolean fullscreen;
		qboolean needToToggle;

		// Find out the current state
		fullscreen = !!(SDL_GetWindowFlags(main_window) & SDL_WINDOW_FULLSCREEN);

		if (r_fullscreen->integer && Cvar_VariableIntegerValue("in_nograb"))
		{
			Com_Printf("Fullscreen not allowed with in_nograb 1\n");
			Cvar_Set("r_fullscreen", "0");
			r_fullscreen->modified = qfalse;
		}

		// Is the state we want different from the current state?
		needToToggle = !!r_fullscreen->integer != fullscreen;

		if (needToToggle)
		{
			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if (!(SDL_SetWindowFullscreen(main_window, r_fullscreen->integer) >= 0)) // !sdlToggled
			{
				Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
			}

			IN_Restart();
		}

#ifdef MACOS_X_GAMMA_RESET_FIX
		// OS X 10.9 has a bug where toggling in or out of fullscreen mode
		// will cause the gamma to reset to the system default after an unknown
		// short delay. This little fix simply causes the gamma to be reset
		// again after a hopefully-long-enough-delay of 3 seconds.
		// Radar 15961845
		gammaResetTime = CL_ScaledMilliseconds() + 3000;
#endif
		r_fullscreen->modified = qfalse;
	}

#ifdef MACOS_X_GAMMA_RESET_FIX
	if ((gammaResetTime != 0) && (gammaResetTime < CL_ScaledMilliseconds()))
	{
		// Circuitous way of resetting the gamma to its current value.
		char old[6] = { 0 };
		Q_strncpyz(old, va("%i", Cvar_VariableIntegerValue("r_gamma")), 5);
		if (strlen(old))
		{
			Cvar_Set("r_gamma", "1");
			Cvar_Set("r_gamma", old);
		}

		gammaResetTime = 0;
	}
#endif
}

/**
 * @brief GLimp_SetGamma
 * @param[in] red
 * @param[in] green
 * @param[in] blue
 */
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256])
{
	Uint16 table[3][256];
	int    i, j;

	if (!cls.glconfig.deviceSupportsGamma || r_ignorehwgamma->integer > 0)
	{
		Com_Printf(S_COLOR_YELLOW "Device doesn't support gamma or r_ignorehwgamma is set.\n");
		return;
	}

	for (i = 0; i < 256; i++)
	{
		table[0][i] = ((( Uint16 ) red[i]) << 8) | red[i];
		table[1][i] = ((( Uint16 ) green[i]) << 8) | green[i];
		table[2][i] = ((( Uint16 ) blue[i]) << 8) | blue[i];
	}

#ifdef _WIN32

	// Win2K and newer put this odd restriction on gamma ramps...
	{
		OSVERSIONINFO vinfo;

		vinfo.dwOSVersionInfoSize = sizeof(vinfo);
		GetVersionEx(&vinfo);
		if (vinfo.dwMajorVersion >= 5 && vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			Com_DPrintf("performing gamma clamp.\n");
			for (j = 0 ; j < 3 ; j++)
			{
				for (i = 0 ; i < 128 ; i++)
				{
					if (table[j][i] > ((128 + i) << 8))
					{
						table[j][i] = (128 + i) << 8;
					}
				}

				if (table[j][127] > 254 << 8)
				{
					table[j][127] = 254 << 8;
				}
			}
		}
	}
#endif

	// enforce constantly increasing
	for (j = 0; j < 3; j++)
	{
		for (i = 1; i < 256; i++)
		{
			if (table[j][i] < table[j][i - 1])
			{
				table[j][i] = table[j][i - 1];
			}
		}
	}

	SDL_SetWindowGammaRamp(main_window, table[0], table[1], table[2]);
}
