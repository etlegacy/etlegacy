/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2017 ET:Legacy team <mail@etlegacy.com>
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
 * @file cg_drawtools.c
 * @brief Helper functions called by cg_draw, cg_scoreboard, cg_info, etc
 */

#include "cg_local.h"

/**
 * @brief Adjusted for resolution and screen aspect ratio
 * @param[out] x
 * @param[out] y
 * @param[out] w
 * @param[out] h
 */
void CG_AdjustFrom640(float *x, float *y, float *w, float *h)
{
	// scale for screen sizes
	*x *= cgs.screenXScale;
	*y *= cgs.screenYScale;
	*w *= cgs.screenXScale;
	*h *= cgs.screenYScale;
	// adjust x-coordinate and width
	if (!Ccg_Is43Screen())
	{
		*x *= cgs.r43da;    // * ((4/3) / aspectratio);
		*w *= cgs.r43da;    // * ((4/3) / aspectratio);
	}
}

/**
 * @brief Ccg_Is43Screen
 * @return
 */
qboolean Ccg_Is43Screen(void)
{
	if (cgs.glconfig.windowAspect <= RATIO43)
	{
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief Ccg_WideX
 * @param[in] x
 * @return
 */
float Ccg_WideX(float x)
{
	return (Ccg_Is43Screen()) ? x : x *cgs.adr43; // * (aspectratio / (4/3))
}

/**
 * @brief Ccg_WideXoffset
 * @return
 */
float Ccg_WideXoffset(void)
{
	return (Ccg_Is43Screen()) ? 0.0f : ((640.0f * cgs.adr43) - 640.0f) * 0.5f;
}

/**
 * @brief CG_FillRect
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in,out] color
 * @note Coordinates are 640*480 virtual values
 */
void CG_FillRect(float x, float y, float width, float height, const float *color)
{
	trap_R_SetColor(color);

	CG_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 1, cgs.media.whiteShader);

	trap_R_SetColor(NULL);
}

/*
 * @brief CG_FillRectGradient
 * @param x
 * @param y
 * @param width
 * @param height
 * @param color
 * @param gradcolor
 * @param gradientType
 * @note Unused
void CG_FillRectGradient(float x, float y, float width, float height, const float *color, const float *gradcolor, int gradientType)
{
    trap_R_SetColor(color);

    CG_AdjustFrom640(&x, &y, &width, &height);
    trap_R_DrawStretchPicGradient(x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader, gradcolor, gradientType);

    trap_R_SetColor(NULL);
}
*/

#define BAR_BORDERSIZE 2

/**
 * @brief CG_FilledBar
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in,out] startColor
 * @param[in,out] endColor
 * @param[in,out] bgColor
 * @param[in] frac
 * @param[in] flags
 */
void CG_FilledBar(float x, float y, float w, float h, float *startColor, float *endColor, const float *bgColor, float frac, int flags)
{
	vec4_t backgroundcolor = { 1, 1, 1, 0.25f }, colorAtPos;  // colorAtPos is the lerped color if necessary

	if (frac > 1)
	{
		frac = 1.f;
	}
	if (frac < 0)
	{
		frac = 0;
	}

	if ((flags & BAR_BG) && bgColor)       // BAR_BG set, and color specified, use specified bg color
	{
		Vector4Copy(bgColor, backgroundcolor);
	}

	if (flags & BAR_LERP_COLOR)
	{
		Vector4Average(startColor, endColor, frac, colorAtPos);
	}

	// background
	if ((flags & BAR_BG))
	{
		int indent = BAR_BORDERSIZE;

		// draw background at full size and shrink the remaining box to fit inside with a border.  (alternate border may be specified by a BAR_BGSPACING_xx)
		CG_FillRect(x,
		            y,
		            w,
		            h,
		            backgroundcolor);

		if (flags & BAR_BGSPACING_X0Y0)              // fill the whole box (no border)
		{
		}
		else if (flags & BAR_BGSPACING_X0Y5)         // spacing created for weapon heat
		{
			indent *= 3;
			y      += indent;
			h      -= (2 * indent);

		}
		else                                    // default spacing of 2 units on each side
		{
			x += indent;
			y += indent;
			w -= (2 * indent);
			h -= (2 * indent);
		}
	}
	else if ((flags & BAR_BORDER) || (flags & BAR_BORDER_SMALL))
	{
		int indent = (flags & BAR_BORDER_SMALL) ? 1 : BAR_BORDERSIZE;

		CG_DrawRect_FixedBorder(x, y, w, h, indent, bgColor);
		x += indent;
		y += indent;
		w -= indent;
		h -= (2 * indent);
	}

	// adjust for horiz/vertical and draw the fractional box
	if (flags & BAR_VERT)
	{
		if (flags & BAR_LEFT)        // TODO: remember to swap colors on the ends here
		{
			y += (h * (1 - frac));
		}
		else if (flags & BAR_CENTER)
		{
			y += (h * (1 - frac) / 2);
		}

		if (flags & BAR_LERP_COLOR)
		{
			CG_FillRect(x, y, w, h * frac, colorAtPos);
		}
		else
		{
			CG_FillRect(x, y, w, h * frac, startColor);
		}
	}
	else
	{
		if (flags & BAR_LEFT)        // TODO: remember to swap colors on the ends here
		{
			x += (w * (1 - frac));
		}
		else if (flags & BAR_CENTER)
		{
			x += (w * (1 - frac) / 2);
		}

		if (flags & BAR_LERP_COLOR)
		{
			CG_FillRect(x, y, w * frac, h, colorAtPos);
		}
		else
		{
			CG_FillRect(x, y, w * frac, h, startColor);
		}
	}
}

/**
 * @brief Generic routine for pretty much all status indicators that show a fractional
 * value to the palyer by virtue of how full a drawn box is.
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] percent
 */
void CG_HorizontalPercentBar(float x, float y, float width, float height, float percent)
{
	vec4_t bgcolor = { 0.5f, 0.5f, 0.5f, 0.3f },
	       color   = { 1.0f, 1.0f, 1.0f, 0.3f };
	CG_FilledBar(x, y, width, height, color, NULL, bgcolor, percent, BAR_BG | BAR_NOHUDALPHA);
}

/**
 * @brief CG_DrawSides
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] size
 * @note Coordinates are 640*480 virtual values
 */
void CG_DrawSides(float x, float y, float w, float h, float size)
{
	CG_AdjustFrom640(&x, &y, &w, &h);
	size *= cgs.screenXScale;
	trap_R_DrawStretchPic(x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
	trap_R_DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
}

/**
 * @brief CG_DrawTopBottom
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] size
 * @note Coordinates are 640*480 virtual values
 */
void CG_DrawTopBottom(float x, float y, float w, float h, float size)
{
	CG_AdjustFrom640(&x, &y, &w, &h);
	size *= cgs.screenYScale;
	trap_R_DrawStretchPic(x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
	trap_R_DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
}

/**
 * @brief CG_DrawSides_NoScale
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] size
 * @note Coordinates are 640*480 virtual values
 */
void CG_DrawSides_NoScale(float x, float y, float w, float h, float size)
{
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
	trap_R_DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
}

/**
 * @brief CG_DrawTopBottom_NoScale
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] size
 * @note Coordinates are 640*480 virtual values
 */
void CG_DrawTopBottom_NoScale(float x, float y, float w, float h, float size)
{
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
	trap_R_DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
}

/**
 * @brief CG_DrawBottom_NoScale
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] size
 * @note Coordinates are 640*480 virtual values
 */
void CG_DrawBottom_NoScale(float x, float y, float w, float h, float size)
{
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
}

/**
 * @brief CG_DrawRect
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] size
 * @param[in,out] color
 * @note Coordinates are 640*480 virtual values
 */
void CG_DrawRect(float x, float y, float width, float height, float size, const float *color)
{
	trap_R_SetColor(color);

	CG_DrawTopBottom(x, y, width, height, size);
	CG_DrawSides(x, y, width, height, size);

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_DrawRect_FixedBorder
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] border
 * @param[in,out] color
 */
void CG_DrawRect_FixedBorder(float x, float y, float width, float height, int border, const float *color)
{
	trap_R_SetColor(color);

	CG_DrawTopBottom_NoScale(x, y, width, height, border);
	CG_DrawSides_NoScale(x, y, width, height, border);

	trap_R_SetColor(NULL);
}

/**
 * @brief Allows passing of st co-ords
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] s0
 * @param[in] t0
 * @param[in] s1
 * @param[in] t1
 * @param[in] hShader
 * @note Coordinates are 640*480 virtual values
 */
void CG_DrawPicST(float x, float y, float width, float height, float s0, float t0, float s1, float t1, qhandle_t hShader)
{
	CG_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, s0, t0, s1, t1, hShader);
}

/**
 * @brief CG_DrawPic
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] hShader
 * @note Coordinates are 640*480 virtual values
 */
void CG_DrawPic(float x, float y, float width, float height, qhandle_t hShader)
{
	float s0;
	float s1;
	float t0;
	float t1;

	if (width < 0)       // flip about vertical
	{
		width = -width;
		s0    = 1;
		s1    = 0;
	}
	else
	{
		s0 = 0;
		s1 = 1;
	}

	if (height < 0)      // flip about horizontal
	{
		height = -height;
		t0     = 1;
		t1     = 0;
	}
	else
	{
		t0 = 0;
		t1 = 1;
	}

	CG_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, s0, t0, s1, t1, hShader);
}

/**
 * @brief CG_DrawRotatedPic
 * @details Clear around a sized down screen
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] hShader
 * @param[in] angle
 */
void CG_DrawRotatedPic(float x, float y, float width, float height, qhandle_t hShader, float angle)
{
	CG_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawRotatedPic(x, y, width, height, 0, 0, 1, 1, hShader, angle);
}

/**
 * @brief This repeats a 64*64 tile graphic to fill the screen around a sized down
 * refresh window.
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] hShader
 */
static void CG_TileClearBox(int x, int y, int w, int h, qhandle_t hShader)
{
	float s1 = x / 64.0f;
	float t1 = y / 64.0f;
	float s2 = (x + w) / 64.0f;
	float t2 = (y + h) / 64.0f;

	trap_R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader);
}

/**
 * @brief CG_TileClear
 * @details Clear around a sized down screen
 */
void CG_TileClear(void)
{
	int top, bottom, left, right;
	int w = cgs.glconfig.vidWidth;
	int h = cgs.glconfig.vidHeight;

	if (cg.refdef.x == 0 && cg.refdef.y == 0 &&
	    cg.refdef.width == w && cg.refdef.height == h)
	{
		return;     // full screen rendering
	}

	top    = cg.refdef.y;
	bottom = top + cg.refdef.height - 1;
	left   = cg.refdef.x;
	right  = left + cg.refdef.width - 1;

	// clear above view screen
	CG_TileClearBox(0, 0, w, top, cgs.media.backTileShader);

	// clear below view screen
	CG_TileClearBox(0, bottom, w, h - bottom, cgs.media.backTileShader);

	// clear left of view screen
	CG_TileClearBox(0, top, left, bottom - top + 1, cgs.media.backTileShader);

	// clear right of view screen
	CG_TileClearBox(right, top, w - right, bottom - top + 1, cgs.media.backTileShader);
}

/**
 * @brief CG_FadeColor
 * @param[in] startMsec
 * @param[in] totalMsec
 * @return
 */
float *CG_FadeColor(int startMsec, int totalMsec)
{
	static vec4_t color;
	int           t;

	if (startMsec == 0)
	{
		return NULL;
	}

	t = cg.time - startMsec;

	if (t >= totalMsec)
	{
		return NULL;
	}

	// fade out
	if (totalMsec - t < FADE_TIME)
	{
		color[3] = (totalMsec - t) * 1.0f / FADE_TIME;
	}
	else
	{
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1.f;

	return color;
}

static vec4_t red = { 1.0f, 0.2f, 0.2f, 1.0f };
static vec4_t blue = { 0.2f, 0.2f, 1.0f, 1.0f };
static vec4_t other = { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t spectator = { 0.7f, 0.7f, 0.7f, 1.0f };

/**
 * @brief CG_TeamColor
 * @param[in] team
 * @return
 */
float *CG_TeamColor(int team)
{
	switch (team)
	{
	case TEAM_AXIS:
		return red;
	case TEAM_ALLIES:
		return blue;
	case TEAM_SPECTATOR:
		return spectator;
	default:
		return other;
	}
}

/**
 * @brief CG_GetColorForHealth
 * @param[in] health
 * @param[in] hcolor
 */
void CG_GetColorForHealth(int health, vec4_t hcolor)
{
	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if (health <= 0)
	{
		VectorClear(hcolor);    // black
		hcolor[3] = 1;
		return;
	}

	// set the color based on health
	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if (health >= 100)
	{
		hcolor[2] = 1.0;
	}
	else if (health < 66)
	{
		hcolor[2] = 0;
	}
	else
	{
		hcolor[2] = (health - 66.f) / 33.0f;
	}

	if (health > 60)
	{
		hcolor[1] = 1.0;
	}
	else if (health < 30)
	{
		hcolor[1] = 0;
	}
	else
	{
		hcolor[1] = (health - 30.f) / 30.0f;
	}
}

/**
 * @brief CG_ColorForHealth
 * @param[in] hcolor
 */
void CG_ColorForHealth(vec4_t hcolor)
{
	int health;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	health = cg.snap->ps.stats[STAT_HEALTH];
	if (health <= 0)
	{
		VectorClear(hcolor);    // black
		hcolor[3] = 1;
		return;
	}

	// set the color based on health
	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if (health >= 100)
	{
		hcolor[2] = 1.0;
	}
	else if (health < 66)
	{
		hcolor[2] = 0;
	}
	else
	{
		hcolor[2] = (health - 66.f) / 33.0f;
	}

	if (health > 60)
	{
		hcolor[1] = 1.0;
	}
	else if (health < 30)
	{
		hcolor[1] = 0;
	}
	else
	{
		hcolor[1] = (health - 30.f) / 30.0f;
	}
}

/**
 * @brief CG_TranslateString
 * @param[in] string
 * @return
 */
const char *CG_TranslateString(const char *string)
{
	static char staticbuf[2][MAX_VA_STRING];
	static int  bufcount = 0;
	char        *buf;

	// some code expects this to return a copy always, even
	// if none is needed for translation, so always supply another buffer
	buf = staticbuf[bufcount++ % 2];

	trap_TranslateString(string, buf);

	return buf;
}

/*
 * @note Unused.
 *
static int propMap[128][3] =
{
    { 0,   0,   -1               }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },
    { 0,   0,   -1               }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },

    { 0,   0,   -1               }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },
    { 0,   0,   -1               }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },

    { 0,   0,   PROP_SPACE_WIDTH }, // SPACE
    { 11,  122, 7                }, // !
    { 154, 181, 14               }, // "
    { 55,  122, 17               }, // #
    { 79,  122, 18               }, // $
    { 101, 122, 23               }, // %
    { 153, 122, 18               }, // &
    { 9,   93,  7                }, // '
    { 207, 122, 8                }, // (
    { 230, 122, 9                }, // )
    { 177, 122, 18               }, // *
    { 30,  152, 18               }, // +
    { 85,  181, 7                }, // ,
    { 34,  93,  11               }, // -
    { 110, 181, 6                }, // .
    { 130, 152, 14               }, // /

    { 22,  64,  17               }, // 0
    { 41,  64,  12               }, // 1
    { 58,  64,  17               }, // 2
    { 78,  64,  18               }, // 3
    { 98,  64,  19               }, // 4
    { 120, 64,  18               }, // 5
    { 141, 64,  18               }, // 6
    { 204, 64,  16               }, // 7
    { 162, 64,  17               }, // 8
    { 182, 64,  18               }, // 9
    { 59,  181, 7                }, // :
    { 35,  181, 7                }, // ;
    { 203, 152, 14               }, // <
    { 56,  93,  14               }, // =
    { 228, 152, 14               }, // >
    { 177, 181, 18               }, // ?

    { 28,  122, 22               }, // @
    { 5,   4,   18               }, // A
    { 27,  4,   18               }, // B
    { 48,  4,   18               }, // C
    { 69,  4,   17               }, // D
    { 90,  4,   13               }, // E
    { 106, 4,   13               }, // F
    { 121, 4,   18               }, // G
    { 143, 4,   17               }, // H
    { 164, 4,   8                }, // I
    { 175, 4,   16               }, // J
    { 195, 4,   18               }, // K
    { 216, 4,   12               }, // L
    { 230, 4,   23               }, // M
    { 6,   34,  18               }, // N
    { 27,  34,  18               }, // O

    { 48,  34,  18               }, // P
    { 68,  34,  18               }, // Q
    { 90,  34,  17               }, // R
    { 110, 34,  18               }, // S
    { 130, 34,  14               }, // T
    { 146, 34,  18               }, // U
    { 166, 34,  19               }, // V
    { 185, 34,  29               }, // W
    { 215, 34,  18               }, // X
    { 234, 34,  18               }, // Y
    { 5,   64,  14               }, // Z
    { 60,  152, 7                }, // [
    { 106, 151, 13               }, // '\'
    { 83,  152, 7                }, // ]
    { 128, 122, 17               }, // ^
    { 4,   152, 21               }, // _

    { 134, 181, 5                }, // '
    { 5,   4,   18               }, // A
    { 27,  4,   18               }, // B
    { 48,  4,   18               }, // C
    { 69,  4,   17               }, // D
    { 90,  4,   13               }, // E
    { 106, 4,   13               }, // F
    { 121, 4,   18               }, // G
    { 143, 4,   17               }, // H
    { 164, 4,   8                }, // I
    { 175, 4,   16               }, // J
    { 195, 4,   18               }, // K
    { 216, 4,   12               }, // L
    { 230, 4,   23               }, // M
    { 6,   34,  18               }, // N
    { 27,  34,  18               }, // O

    { 48,  34,  18               }, // P
    { 68,  34,  18               }, // Q
    { 90,  34,  17               }, // R
    { 110, 34,  18               }, // S
    { 130, 34,  14               }, // T
    { 146, 34,  18               }, // U
    { 166, 34,  19               }, // V
    { 185, 34,  29               }, // W
    { 215, 34,  18               }, // X
    { 234, 34,  18               }, // Y
    { 5,   64,  14               }, // Z
    { 153, 152, 13               }, // {
    { 11,  181, 5                }, // |
    { 180, 152, 13               }, // }
    { 79,  93,  17               }, // ~
    { 0,   0,   -1               } // DEL
};
*/

/*
 * @note Unused.
 *
static int propMapB[26][3] =
{
    { 11,  12,  33 },
    { 49,  12,  31 },
    { 85,  12,  31 },
    { 120, 12,  30 },
    { 156, 12,  21 },
    { 183, 12,  21 },
    { 207, 12,  32 },

    { 13,  55,  30 },
    { 49,  55,  13 },
    { 66,  55,  29 },
    { 101, 55,  31 },
    { 135, 55,  21 },
    { 158, 55,  40 },
    { 204, 55,  32 },

    { 12,  97,  31 },
    { 48,  97,  31 },
    { 82,  97,  30 },
    { 118, 97,  30 },
    { 153, 97,  30 },
    { 185, 97,  25 },
    { 213, 97,  30 },

    { 11,  139, 32 },
    { 42,  139, 51 },
    { 93,  139, 32 },
    { 126, 139, 31 },
    { 158, 139, 25 },
};

#define PROPB_GAP_WIDTH     4
#define PROPB_SPACE_WIDTH   12
#define PROPB_HEIGHT        36
*/

/*
 * @brief UI_DrawBannerString2
 * @note Unused.
 *
static void UI_DrawBannerString2(int x, int y, const char *str, vec4_t color)
{
    const char    *s = str;
    unsigned char ch;
    float         ax = x * cgs.screenXScale + cgs.screenXBias;
    float         ay = y * cgs.screenYScale;
    float         aw;
    float         ah;
    float         frow;
    float         fcol;
    float         fwidth;
    float         fheight;

    // draw the colored text
    trap_R_SetColor(color);

    while (*s)
    {
        ch = *s & 127;
        if (ch == ' ')
        {
            ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH) * cgs.screenXScale;
        }
        else if (ch >= 'A' && ch <= 'Z')
        {
            ch     -= 'A';
            fcol    = (float)propMapB[ch][0] / 256.0f;
            frow    = (float)propMapB[ch][1] / 256.0f;
            fwidth  = (float)propMapB[ch][2] / 256.0f;
            fheight = (float)PROPB_HEIGHT / 256.0f;
            aw      = (float)propMapB[ch][2] * cgs.screenXScale;
            ah      = (float)PROPB_HEIGHT * cgs.screenYScale;
            trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, cgs.media.charsetPropB);
            ax += (aw + (float)PROPB_GAP_WIDTH * cgs.screenXScale);
        }
        s++;
    }

    trap_R_SetColor(NULL);
}
*/

/*
 * @brief UI_DrawBannerString
 * @note Unused.
 *
void UI_DrawBannerString(int x, int y, const char *str, int style, vec4_t color)
{
    const char *s = str;
    int        ch;
    int        width = 0;
    vec4_t     drawcolor;

    // find the width of the drawn text
    while (*s)
    {
        ch = *s;
        if (ch == ' ')
        {
            width += PROPB_SPACE_WIDTH;
        }
        else if (ch >= 'A' && ch <= 'Z')
        {
            width += propMapB[ch - 'A'][2] + PROPB_GAP_WIDTH;
        }
        s++;
    }
    width -= PROPB_GAP_WIDTH;

    switch (style & UI_FORMATMASK)
    {
    case UI_CENTER:
        x -= width / 2;
        break;

    case UI_RIGHT:
        x -= width;
        break;

    case UI_LEFT:
    default:
        break;
    }

    if (style & UI_DROPSHADOW)
    {
        drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
        drawcolor[3] = color[3];
        UI_DrawBannerString2(x + 2, y + 2, str, drawcolor);
    }

    UI_DrawBannerString2(x, y, str, color);
}
*/

/*
 * @brief UI_ProportionalStringWidth
 * @note Unused.
 *
int UI_ProportionalStringWidth(const char *str)
{
    const char *s = str;
    int        ch;
    int        charWidth;
    int        width = 0;

    while (*s)
    {
        ch        = *s & 127;
        charWidth = propMap[ch][2];
        if (charWidth != -1)
        {
            width += charWidth;
            width += PROP_GAP_WIDTH;
        }
        s++;
    }

    width -= PROP_GAP_WIDTH;
    return width;
}
*/

/*
 * @brief UI_DrawProportionalString2
 * @note Unused.
 *
static void UI_DrawProportionalString2(int x, int y, const char *str, vec4_t color, float sizeScale, qhandle_t charset)
{
    const char    *s = str;
    unsigned char ch;
    float         ax = x * cgs.screenXScale + cgs.screenXBias;
    float         ay = y * cgs.screenYScale;
    float         aw;
    float         ah;
    float         frow;
    float         fcol;
    float         fwidth;
    float         fheight;

    // draw the colored text
    trap_R_SetColor(color);

    while (*s)
    {
        ch = *s & 127;
        if (ch == ' ')
        {
            aw = (float)PROP_SPACE_WIDTH * cgs.screenXScale * sizeScale;
        }
        else if (propMap[ch][2] != -1)
        {
            fcol    = (float)propMap[ch][0] / 256.0f;
            frow    = (float)propMap[ch][1] / 256.0f;
            fwidth  = (float)propMap[ch][2] / 256.0f;
            fheight = (float)PROP_HEIGHT / 256.0f;
            aw      = (float)propMap[ch][2] * cgs.screenXScale * sizeScale;
            ah      = (float)PROP_HEIGHT * cgs.screenYScale * sizeScale;
            trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, charset);
        }
        else
        {
            aw = 0;
        }

        ax += (aw + (float)PROP_GAP_WIDTH * cgs.screenXScale * sizeScale);
        s++;
    }

    trap_R_SetColor(NULL);
}
*/

/*
 * @brief UI_ProportionalSizeScale
 * @note Unused.
 *
float UI_ProportionalSizeScale(int style)
{
    if (style & UI_SMALLFONT)
    {
        return 0.75;
    }
    if (style & UI_EXSMALLFONT)
    {
        return 0.4;
    }

    return 1.0f;
}
*/

/*
 * @brief UI_DrawProportionalString
 * @note Unused.
 *
void UI_DrawProportionalString(int x, int y, const char *str, int style, vec4_t color)
{
    vec4_t drawcolor;
    int    width;
    float  sizeScale = UI_ProportionalSizeScale(style);

    switch (style & UI_FORMATMASK)
    {
    case UI_CENTER:
        width = UI_ProportionalStringWidth(str) * sizeScale;
        x    -= width / 2;
        break;

    case UI_RIGHT:
        width = UI_ProportionalStringWidth(str) * sizeScale;
        x    -= width;
        break;

    case UI_LEFT:
    default:
        break;
    }

    if (style & UI_DROPSHADOW)
    {
        drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
        drawcolor[3] = color[3];
        UI_DrawProportionalString2(x + 2, y + 2, str, drawcolor, sizeScale, cgs.media.charsetProp);
    }

    if (style & UI_INVERSE)
    {
        drawcolor[0] = color[0] * 0.8;
        drawcolor[1] = color[1] * 0.8;
        drawcolor[2] = color[2] * 0.8;
        drawcolor[3] = color[3];
        UI_DrawProportionalString2(x, y, str, drawcolor, sizeScale, cgs.media.charsetProp);
        return;
    }

    if (style & UI_PULSE)
    {
        //drawcolor[0] = color[0] * 0.8;
        //drawcolor[1] = color[1] * 0.8;
        //drawcolor[2] = color[2] * 0.8;
        drawcolor[3] = color[3];
        UI_DrawProportionalString2(x, y, str, color, sizeScale, cgs.media.charsetProp);

        drawcolor[0] = color[0];
        drawcolor[1] = color[1];
        drawcolor[2] = color[2];
        drawcolor[3] = 0.5 + 0.5 * sin(cg.time / PULSE_DIVISOR);
        UI_DrawProportionalString2(x, y, str, drawcolor, sizeScale, cgs.media.charsetPropGlow);
        return;
    }

    UI_DrawProportionalString2(x, y, str, color, sizeScale, cgs.media.charsetProp);
}
*/
