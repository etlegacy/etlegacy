/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2023 ET:Legacy team <mail@etlegacy.com>
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
 * @file tr_splash.c
 * @brief handles Splash screen drawing while the client is staring up the game code
 */

#include "tr_local_proxy.h"
#include "tr_common.h"

image_t *splashImage = NULL;
qhandle_t splashHandle;

static void LoadSplashImage(const char *name, byte *data, unsigned int width, unsigned int height, uint8_t bytes)
{
	if (data)
	{
		splashImage = R_CreateImage(LEGACY_SPLASH_NAME, data, width, height, qfalse, qfalse, GL_CLAMP_TO_EDGE);
	}
	else
	{
		splashImage = R_FindImageFile( name, qfalse, qfalse, GL_CLAMP_TO_EDGE, qfalse);
	}

	if (!splashImage)
	{
		Ren_Print(S_COLOR_RED "Could not load splash image\n");
		return;
	}

	splashHandle = RE_RegisterShaderFromImage(LEGACY_SPLASH_NAME, LIGHTMAP_2D, splashImage, qfalse);

	GL_CheckErrors();
}

static void R_Splash_AdjustFrom640(float *x, float *y, float *w, float *h)
{
	float xscale;
	float yscale;

	// scale for screen sizes
	xscale = glConfig.vidWidth / 640.0f;
	yscale = glConfig.vidHeight / 480.0f;
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

void R_InitSplash(void)
{
	if (!ri.GLimp_SplashImage(&LoadSplashImage))
	{
		Ren_Print("Could not load splash image\n");
		return;
	}
}

void R_DrawSplash(void)
{
	float tmp, x, y , w, h;
	if (!splashImage)
	{
		return;
	}

	tmp = (float)splashImage->height / (float)splashImage->width;
	w = SCREEN_WIDTH_F * (glConfig.windowWidth > 1600 ? 0.2f : 0.4f);
	h = tmp * w;

	x = SCREEN_WIDTH_F / 2 - w / 2;
	y = SCREEN_HEIGHT_F / 2 - h / 2;

	R_Splash_AdjustFrom640(&x, &y, &w, &h);

	RE_BeginFrame();
	RE_SetColor(NULL);
	RE_StretchPic(x, y, w, h, 0, 0, 1, 1, splashHandle);
	RE_EndFrame(NULL, NULL);

	GL_CheckErrors();
}
