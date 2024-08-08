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

	// when HUD editor is enabled, adjust virtual grid to not fill the entire screen,
	// so HUD elements scale correctly to decreased viewport
	// mouse movement is handled separately in CG_MouseEvent
	if (cg.editingHud && !cg.fullScreenHudEditor)
	{
		*x *= 0.78f;
		*y *= 0.78f;
		*w *= 0.78f;
		*h *= 0.78f;
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
	return (Ccg_Is43Screen()) ? x : x * cgs.adr43;  // * (aspectratio / (4/3))
}

/**
 * @brief Ccg_WideXReverse
 * @param[in] x
 * @return
 */
float Ccg_WideXReverse(float x)
{
	return (Ccg_Is43Screen()) ? x : x / cgs.adr43; // * (aspectratio / (4/3))
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

/**
 * @brief CG_SetChargebarIconColor
 * Sets correct charge bar icon for fieldops to indicate air support status
 */
void CG_SetChargebarIconColor(void)
{
	if (cg.snap->ps.ammo[WP_ARTY] & NO_AIRSTRIKE && cg.snap->ps.ammo[WP_ARTY] & NO_ARTILLERY)
	{
		trap_R_SetColor(colorRed);
	}
	else if (cg.snap->ps.ammo[WP_ARTY] & NO_AIRSTRIKE)
	{
		trap_R_SetColor(colorOrange);
	}
	else if (cg.snap->ps.ammo[WP_ARTY] & NO_ARTILLERY)
	{
		trap_R_SetColor(colorYellow);
	}
}

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
void CG_FilledBar(float x, float y, float w, float h, float *startColor, float *endColor,
                  const float *bgColor, const float *bdColor, float frac, float needleFrac, int flags, qhandle_t icon)
{
	vec4_t backgroundcolor = { 1, 1, 1, 0.25f }, colorAtPos;  // colorAtPos is the lerped color if necessary
	float  x2 = x, x3 = x, y2 = y, y3 = y, w2 = w, h2 = h;
	float  iconW, iconH;

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
		if (endColor)
		{
			Vector4Average(startColor, endColor, frac, colorAtPos);
		}
		else
		{
			Vector4Scale(startColor, frac, colorAtPos);
		}
	}

	if (flags & BAR_DECOR)
	{
		if (flags & BAR_VERT)
		{
			y += (h * 0.1f);
			h *= 0.84f;
		}
		else
		{
			x += (w * 0.1f);
			w *= 0.84f;
		}
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
	else if (((flags & BAR_BORDER) || (flags & BAR_BORDER_SMALL)) && bdColor)
	{
		int indent = (flags & BAR_BORDER_SMALL) ? 1 : BAR_BORDERSIZE;

		CG_DrawRect_FixedBorder(x, y, w, h, indent, bdColor);
		x += indent;
		y += indent;
		w -= (2 * indent);
		h -= (2 * indent);
	}

	x3 = x;
	y3 = y;

	// backgroundcolor is reused for the needle from here, inverted in case we
	// draw it on the background itself
	if ((!(flags & BAR_BG) && endColor) || ((flags & BAR_BG) && !endColor))
	{
		backgroundcolor[0] = 255 - backgroundcolor[0];
		backgroundcolor[1] = 255 - backgroundcolor[1];
		backgroundcolor[2] = 255 - backgroundcolor[2];
	}

	// adjust for horiz/vertical and draw the fractional box
	if (flags & BAR_VERT)
	{
		iconW = w2;
		iconH = iconW;

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

		if (needleFrac > 0.f && flags & BAR_NEEDLE)
		{
			CG_FillRect(x3, y3 + (h * (1 - needleFrac)) + 0.0, w, 1.0, backgroundcolor);
		}

		if (flags & BAR_DECOR)
		{
			CG_DrawPic(x2, y2, w2, h2, cgs.media.hudSprintBar);
		}

		if (flags & BAR_ICON && icon > -1)
		{
			float offset = 4.0f;
			if (icon == cgs.media.hudPowerIcon)
			{
				iconW *= .5f;
				x3     = x2;
				x2    += iconW * .5f;

				if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS)
				{
					CG_SetChargebarIconColor();
				}
			}

			if (flags & BAR_LEFT)
			{
				CG_DrawPic(x2, y2 + h2 + offset, iconW, iconH, icon);
			}
			else
			{
				CG_DrawPic(x2, y2 - w2 - offset, iconW, iconH, icon);
			}
		}
	}
	else
	{
		iconH = h2;
		iconW = iconH;

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

		if (needleFrac > 0.f && flags & BAR_NEEDLE)
		{
			CG_FillRect(x3 + (w * (1 - needleFrac)) - 0.0, y3, 1.0, h, backgroundcolor);
		}

		if (flags & BAR_DECOR)
		{
			CG_DrawPic(x2, y2, w2, h2, cgs.media.hudSprintBarHorizontal);
		}

		if (flags & BAR_ICON && icon > -1)
		{
			float offset = 4.0f;
			if (icon == cgs.media.hudPowerIcon)
			{
				iconW *= .5f;

				if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS)
				{
					CG_SetChargebarIconColor();
				}
			}

			if (flags & BAR_LEFT)
			{
				CG_DrawPic(x2 + w2 + offset, y2, iconW, iconH, icon);
			}
			else
			{
				CG_DrawPic(x2 - iconW - offset, y2, iconW, iconH, icon);
			}
		}
	}
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
void CG_DrawRect_FixedBorder(float x, float y, float width, float height, float border, const float *color)
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

/**
 * @brief CG_FadeColor_Ext fade colors with support for variable starting alpha
 * @param[in] startMsec
 * @param[in] totalMsec
 * @param[in] alpha
 * @param[out] color
 * @return
 */
float *CG_FadeColor_Ext(int startMsec, int totalMsec, float alpha)
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
		color[3] = (totalMsec - t) * alpha / FADE_TIME;
	}
	else
	{
		color[3] = alpha;
	}
	color[0] = color[1] = color[2] = alpha;

	return color;
}

/**
 * @brief CG_LerpColorWithAttack, interpolates colors
 * @param[in] from
 * @param[in] to
 * @param[in] startMsec
 * @oaram[in] totalMsec
 * @param[in] attackMsec (attack delay)
 * @return
 */
float *CG_LerpColorWithAttack(vec4_t from, vec4_t to, int startMsec, int totalMsec, int attackMsec)
{
	static vec4_t color;
	int           t;

	if (startMsec == 0)
	{
		return from;
	}

	t = cg.time - startMsec;

	if (t >= totalMsec)
	{
		return to;
	}

	// compute transition
	if (t > attackMsec)
	{
		float progress = (float)(t - attackMsec) / (float)(totalMsec - attackMsec);
		color[0] = from[0] + (to[0] - from[0]) * progress;
		color[1] = from[1] + (to[1] - from[1]) * progress;
		color[2] = from[2] + (to[2] - from[2]) * progress;
		color[3] = from[3] + (to[3] - from[3]) * progress;
	}
	else
	{
		return from;
	}

	return color;
}


static vec4_t red       = { 1.0f, 0.2f, 0.2f, 1.0f };
static vec4_t blue      = { 0.2f, 0.2f, 1.0f, 1.0f };
static vec4_t other     = { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t spectator = { 0.7f, 0.7f, 0.7f, 1.0f };

/**
 * @brief CG_TeamColor
 * @param[in] team
 * @return
 *
 * @note Unused
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
 * @brief Set the color depending of the current health.
 * Color follow the below logic and lerp the color under 100 HP
 *  - >= 100   : default color (set outside the function)
 *  - 66 - 100 : Default color to Yellow
 *  - 33 - 66  : Yellow To Red
 *  - < 33     : Red
 *  - 0        : Black
 * @param[in] health
 * @param[in,out] hcolor
 */
void CG_ColorForHealth(int health, vec4_t hcolor)
{
	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if (health <= 0)
	{
		VectorClear(hcolor);    // black
		hcolor[3] = 1.f;
		return;
	}

	// enough health, keep default value
	if (health >= 100)
	{
		return;
	}

	// set the color based on health
	if (health <= 66)
	{
		hcolor[0] = 1.f;
		hcolor[1] = 1.f;
		hcolor[2] = 0;
	}
	else
	{
		hcolor[0] += (1 - hcolor[0]) * (1 - (health - 66.f) / 33.f);
		hcolor[1] += (1 - hcolor[1]) * (1 - (health - 66.f) / 33.f);
		hcolor[2] *= (health - 66.f) / 33.f;
	}

	if (health > 66)
	{
		return;
	}

	if (health <= 33)
	{
		hcolor[1] = 0;
	}
	else
	{
		hcolor[1] = (health - 33.f) / 33.f;
	}
}

/**
 * @brief CG_TranslateString
 * @param[in] string
 * @return
 */
const char *CG_TranslateString(const char *string)
{
	static char buffer[TRANSLATION_BUFFERS][MAX_PRINT_MSG];
	static int  buffOffset = 0;
	char        *buf;

	// some code expects this to return a copy always, even
	// if none is needed for translation, so always supply another buffer
	buf = buffer[buffOffset++ % TRANSLATION_BUFFERS];

	trap_TranslateString(string, buf);

	return buf;
}

/**
 * @brief Float a sprite over the player's head
 * added height parameter
 * @param point
 * @param[out] x
 * @param[out] y
 * @return
 */
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t point, float *x, float *y)
{
	vec3_t trans;
	float  xc, yc;
	float  px, py;
	float  z;

	px = (float)tan(DEG2RAD((double)cg.refdef.fov_x) / 2);
	py = (float)tan(DEG2RAD((double)cg.refdef.fov_y) / 2);

	VectorSubtract(point, cg.refdef.vieworg, trans);

	xc = 640.0f / 2.0f;
	yc = 480.0f / 2.0f;

	z = DotProduct(trans, cg.refdef.viewaxis[0]);
	if (z < 0.1f)
	{
		return qfalse;
	}
	px *= z;
	py *= z;
	if (px == 0.f || py == 0.f)
	{
		return qfalse;
	}

	*x = xc - (DotProduct(trans, cg.refdef.viewaxis[1]) * xc) / px;
	*y = yc - (DotProduct(trans, cg.refdef.viewaxis[2]) * yc) / py;
	*x = Ccg_WideX(*x);

	return qtrue;
}

/**
 * @brief CG_AddOnScreenText
 * @param[in] text
 * @param[in] origin
 */
void CG_AddOnScreenText(const char *text, vec3_t origin, qboolean fade)
{
	float x, y;

	if (cg.specStringCount >= MAX_FLOATING_STRINGS)
	{
		return;
	}

	if (CG_WorldCoordToScreenCoordFloat(origin, &x, &y))
	{
		float scale, w, h;
		float dist  = VectorDistance(origin, cg.refdef_current->vieworg);
		float dist2 = (dist * dist) / (3600.0f);

		if (dist2 > 2.0f)
		{
			dist2 = 2.0f;
		}

		scale = 2.37f - dist2 - dist / 6000.0f;
		if (scale < 0.05f)
		{
			scale = 0.05f;
		}

		w = CG_Text_Width_Ext(text, scale, 0, &cgs.media.limboFont2);
		h = CG_Text_Height_Ext(text, scale, 0, &cgs.media.limboFont2);

		x -= w / 2;
		y -= h / 2;

		// save it
		cg.specOnScreenLabels[cg.specStringCount].x      = x;
		cg.specOnScreenLabels[cg.specStringCount].y      = y;
		cg.specOnScreenLabels[cg.specStringCount].scale  = scale;
		cg.specOnScreenLabels[cg.specStringCount].text   = text;
		cg.specOnScreenLabels[cg.specStringCount].noFade = !fade;
		VectorCopy(origin, cg.specOnScreenLabels[cg.specStringCount].origin);
		cg.specOnScreenLabels[cg.specStringCount].visible = qtrue;

		// count
		cg.specStringCount++;
	}
	else
	{
		Com_Memset(&cg.specOnScreenLabels[cg.specStringCount], 0, sizeof(cg.specOnScreenLabels[cg.specStringCount]));
	}
}

/**
* @brief CG_AddOnBar
* @param[in] fraction
* @param[in] colorStart
* @param[in] colorEnd
* @param[in] colorBack
* @param[in] origin
*/
void CG_AddOnScreenBar(float fraction, vec4_t colorStart, vec4_t colorEnd, vec4_t colorBack, vec3_t origin)
{
	float x, y, alpha;

	if (cg.specBarCount >= MAX_FLOATING_BARS)
	{
		return;
	}

	if (CG_WorldCoordToScreenCoordFloat(origin, &x, &y))
	{
		float scale, w = 75, h = 7;
		float dist  = VectorDistance(origin, cg.refdef_current->vieworg);
		float dist2 = (dist * dist) / (3600.0f);

		if (dist > 2500)
		{
			return;
		}

		if (dist2 > 2.0f)
		{
			dist2 = 2.0f;
		}

		scale = 2.4f - dist2 - dist / 6000.0f;
		if (scale < 0.05f)
		{
			scale = 0.05f;
		}

		w *= scale;
		h *= scale;
		if (h < 5)
		{
			h = 5;
		}
		if (h > 7)
		{
			h = 7;
		}
		if (w < 10)
		{
			w = 10;
		}
		if (w > 40)
		{
			w = 40;
		}

		x -= w / 2;
		y -= h / 2;

		alpha = colorBack[3] * scale * 2.5f;
		if (alpha > 1.0f)
		{
			colorBack[3] = 1.0f;
		}
		else if (alpha < 0)
		{
			colorBack[3] = 0.0f;
		}
		else
		{
			colorBack[3] = alpha;
		}

		// save it
		cg.specOnScreenBar[cg.specBarCount].x        = x;
		cg.specOnScreenBar[cg.specBarCount].y        = y;
		cg.specOnScreenBar[cg.specBarCount].w        = w;
		cg.specOnScreenBar[cg.specBarCount].h        = h;
		cg.specOnScreenBar[cg.specBarCount].fraction = fraction;
		cg.specOnScreenBar[cg.specBarCount].visible  = qtrue;
		VectorCopy(origin, cg.specOnScreenBar[cg.specBarCount].origin);
		Vector4Copy(colorStart, cg.specOnScreenBar[cg.specBarCount].colorStart);
		Vector4Copy(colorEnd, cg.specOnScreenBar[cg.specBarCount].colorEnd);
		Vector4Copy(colorBack, cg.specOnScreenBar[cg.specBarCount].colorBack);

		// count
		cg.specBarCount++;
	}
	else
	{
		Com_Memset(&cg.specOnScreenBar[cg.specBarCount], 0, sizeof(cg.specOnScreenBar[cg.specBarCount]));
	}
}

/**
 * @brief CG_GetMaxCharsPerLine returns maximum number of character that fit into given width
 * @param[in]  str
 * @param[in]  textScale
 * @param[in]  font
 * @param[in]  width
 * @param[out] maxLineChars
 */
int CG_GetMaxCharsPerLine(const char *str, float textScale, fontHelper_t *font, float width)
{
	int maxLineChars = 0;
	int limit        = 0;

	while (str != NULL)
	{
		if (CG_Text_Width_Ext_Float(str, textScale, 0, font) < width)
		{
			maxLineChars = Q_PrintStrlen(str);
			break;
		}

		limit++;
		maxLineChars++;

		if (CG_Text_Width_Ext_Float(str, textScale, limit, font) > width)
		{
			break;
		}
	}

	return maxLineChars;
}

/**
 * @brief CG_WordWrapString breaks string onto lines respecting the maxLineChars
 * @param[in]  input
 * @param[in]  maxLineChars
 * @param[out] output
 * @param[in]  maxOutputSize
 * @param[out] numLineOutput
 */
char *CG_WordWrapString(const char *input, int maxLineChars, char *output, int maxOutputSize, int *numLineOutput)
{
	int i = 0, o = 0, l, k;
	int lineSplit = 0;
	int lineWidth = 0;
	int numLine   = 1;

	if (!input || !output || maxOutputSize <= 0)
	{
		return NULL;
	}

	while (input[i] && (o + 1) < maxOutputSize)
	{
		// split line
		if (lineWidth > maxLineChars)
		{
			// line might end on certain line break characters
			if (input[i] == ' ')
			{
				while (input[i] && input[i] == ' ')
					i++;                                 // eat trailing spaces
			}
			else if (input[i] == '\\' && input[i + 1] == 'n')
			{
				i += 2; // eat linebreak marker
			}
			else if (input[i] == '\n')
			{
				i++; // eat linebreak
			}
			else if (lineSplit)
			{
				++numLine;
				output[lineSplit] = '\n';
				lineWidth         = Q_UTF8_PrintStrlenExt(output + lineSplit + 1, o - (lineSplit + 1)); // get line length
				lineSplit         = 0;
				continue;
			}
			++numLine;
			output[o++] = '\n';
			lineWidth   = 0;
			lineSplit   = 0;
			continue;
		}

		// linebreak marker
		if (input[i] == '\\' && input[i + 1] == 'n')
		{
			++numLine;
			i          += 2; // eat linebreak marker
			output[o++] = '\n';
			lineSplit   = 0;
			lineWidth   = 0;
			continue;
		}
		else if (Q_IsColorString((input + i)))
		{
			lineWidth -= 2;
		}
		else if (input[i] == ' ')
		{
			lineSplit = o; // record last encountered whitespace
		}
		else if (input[i] == '\n')
		{
			lineSplit = 0;
			lineWidth = -1;
		}
		else if ((unsigned char)input[i] > 0x7F)
		{
			l = Q_UTF8_Width(input + i);

			for (k = 0; k < l; k++)
			{
				output[o++] = input[i++]; // copy surrogates
			}

			lineWidth++;
			continue;
		}

		output[o++] = input[i++];
		lineWidth++;
	}

	if (numLineOutput)
	{
		*numLineOutput = numLine;
	}

	output[o] = 0;

	return output;
}

/**
 * @brief CG_ComputeLinePosX
 * @param[in] x
 * @param[in] scalex
 * @param[in] text
 * @param[in] align
 * @param[in] font
 */
static float CG_ComputeLinePosX(float x, float w, float scalex, const char *text, int align, fontHelper_t *font)
{
	float lineW = CG_Text_Line_Width_Ext_Float(text, scalex, font);

	switch (align)
	{
	case ITEM_ALIGN_CENTER2:
		return x + (w * 0.5f);
	case ITEM_ALIGN_CENTER:
		return x + ((w * 0.5f) - (lineW * .5f));
	case ITEM_ALIGN_RIGHT:
		return x + w - lineW;
	default:
		return x;
	}
}

/**
 * @brief CG_DrawMultilineText
 * @param[in] x
 * @param[in] y
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] color
 * @param[in] text
 * @param[in] lineHeight
 * @param[in] adjust
 * @param[in] limit
 * @param[in] style
 * @param[in] align
 * @param[in] font
 */
void CG_DrawMultilineText(float x, float y, float w, float scalex, float scaley, vec4_t color, const char *text, float lineHeight, float adjust, int limit, int style, int align, fontHelper_t *font)
{
	vec4_t      newColor;
	glyphInfo_t *glyph;
	const char  *s = text;
	float       yadj;
	int         count = 0;
	float       lineX = x, lineY = y;
	float       fontSizeX = scalex;
	float       fontSizeY = scaley;
	float       newAlpha;

	if (!text)
	{
		return;
	}

	fontSizeX *= Q_UTF8_GlyphScale(font);
	fontSizeY *= Q_UTF8_GlyphScale(font);

	if (limit <= 0)
	{
		limit = MAX_QINT;
	}

	if (align > ITEM_ALIGN_LEFT)
	{
		lineX = CG_ComputeLinePosX(x, w, scalex, text, align, font);
	}

	Vector4Copy(color, newColor);

	if (style == ITEM_TEXTSTYLE_BLINK || style == ITEM_TEXTSTYLE_PULSE)
	{
		newAlpha    = Q_fabs(sin(cg.time / (style == ITEM_TEXTSTYLE_BLINK ? BLINK_DIVISOR : PULSE_DIVISOR)));
		newColor[3] = newAlpha;
	}

	trap_R_SetColor(newColor);

	while (s && *s && count < limit)
	{
		if (*s == '\n')
		{
			s++;
			lineY += lineHeight;

			if (align > ITEM_ALIGN_LEFT)
			{
				lineX = CG_ComputeLinePosX(x, w, scalex, s, align, font);
			}
			else
			{
				lineX = x;
			}
			continue;
		}

		glyph = Q_UTF8_GetGlyph(font, s);

		if (Q_IsColorString(s))
		{
			if (*(s + 1) == COLOR_NULL)
			{
				Vector4Copy(color, newColor);
			}
			else
			{
				Com_Memcpy(newColor, g_color_table[ColorIndex(*(s + 1))], sizeof(newColor));
				newColor[3] = color[3];
			}

			if (style == ITEM_TEXTSTYLE_BLINK || style == ITEM_TEXTSTYLE_PULSE)
			{
				newColor[3] = newAlpha;
			}

			trap_R_SetColor(newColor);
			s += 2;
			continue;
		}

		yadj = fontSizeY * glyph->top;

		if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE || style == ITEM_TEXTSTYLE_OUTLINESHADOWED)
		{
			const float ofs = style == ITEM_TEXTSTYLE_SHADOWED ? TEXTSTYLE_SHADOWED_OFFSET : TEXTSTYLE_SHADOWEDMORE_OFFSET;
			colorBlack[3] = newColor[3];
			trap_R_SetColor(colorBlack);
			CG_Text_PaintChar_Ext(lineX + (glyph->pitch * fontSizeX) + ofs * fontSizeX, lineY - yadj + ofs * fontSizeY, glyph->imageWidth, glyph->imageHeight, fontSizeX, fontSizeY, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
			colorBlack[3] = 1.0;
			trap_R_SetColor(newColor);
		}

		CG_Text_PaintChar_Ext(lineX + (glyph->pitch * fontSizeX), lineY - yadj, glyph->imageWidth, glyph->imageHeight, fontSizeX, fontSizeY, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);

		if (style == ITEM_TEXTSTYLE_OUTLINED || style == ITEM_TEXTSTYLE_OUTLINESHADOWED)
		{
			CG_Text_PaintChar_Ext(lineX + (glyph->pitch * fontSizeX) - TEXTSTYLE_OUTLINED_OFFSET * fontSizeX, lineY - yadj - TEXTSTYLE_OUTLINED_OFFSET * fontSizeY, glyph->imageWidth, glyph->imageHeight, fontSizeX, fontSizeY, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
		}

		lineX += (glyph->xSkip * fontSizeX) + adjust;
		s     += Q_UTF8_Width(s);
		count++;
	}

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_DropdownMainBox
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] borderColour
 * @param[in] text
 * @param[in] focus
 * @param[in] font
 */
void CG_DropdownMainBox(float x, float y, float w, float h, float scalex, float scaley, vec4_t borderColour,
                        const char *text, qboolean focus, vec4_t fontColour, int style, fontHelper_t *font)
{
	rectDef_t rect = { x, y, w, h };
	vec4_t    colour;
	float     textboxW;
	int       offsetX, offsetY;

	// dropdown arrow box is square so we can get the width reduction from that
	textboxW = rect.w - rect.h;
	rect.x  += textboxW;
	rect.w   = rect.h;

	if (focus)
	{
		VectorCopy(colorYellow, colour);
		colour[3] = 0.3f;
	}
	else
	{
		VectorCopy(colorWhite, colour);
		colour[3] = 0.3f;
	}

	CG_FillRect(x, y, textboxW, h, colour);
	CG_DrawRect_FixedBorder(x, y, textboxW, h, 1.0f, borderColour);

	if (focus)
	{
		VectorCopy(colorYellow, colour);
		colour[3] = 0.3f;
	}
	else if (BG_PanelButtons_GetFocusButton() == NULL && BG_CursorInRect(&((rectDef_t) { x, y, w, h })))
	{
		VectorCopy(colorWhite, colour);
		colour[3] = 0.5f;
	}
	else
	{
		VectorCopy(colorWhite, colour);
		colour[3] = 0.3f;
	}

	CG_FillRect(rect.x, rect.y, rect.w, rect.h, colour);
	CG_DrawRect_FixedBorder(rect.x, rect.y, rect.w, rect.h, 1.0f, borderColour);

	offsetX = CG_Text_Width_Ext("V", scalex, 0, font);
	offsetY = (CG_Text_Height_Ext("V", scaley, 0, font) + rect.h) * 0.5f;

	//VectorCopy(font->colour, colour);
	CG_Text_Paint_Ext(rect.x + (rect.w - offsetX) / 2.0f, y + offsetY, scalex, scaley, colour, "V", 0, 0, 0, font);

	offsetX = CG_Text_Width_Ext(text, scalex, 0, font);
	offsetY = (CG_Text_Height_Ext(text, scalex, 0, font) + rect.h) * 0.5f;

	CG_Text_Paint_Ext(x + (textboxW - offsetX) / 2.0f, y + offsetY, scalex, scaley, fontColour, text, 0, 0, style, font);
}

/**
 * @brief CG_DropdownBox
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] borderColour
 * @param[in] text
 * @param[in] focus
 * @param[in] fontColour
 * @param[in] style
 * @param[in] font
 * @return Next y coordinate for positioning next box
 */
float CG_DropdownBox(float x, float y, float w, float h, float scalex, float scaley, vec4_t borderColour,
                     const char *text, qboolean focus, vec4_t fontColour, int style, fontHelper_t *font)
{
	rectDef_t rect = { x, y, w, h };
	vec4_t    colour;
	float     textboxW;
	int       offsetX, offsetY;

	// dropdown arrow box is square so we can get the width reduction from that
	textboxW = rect.w - rect.h;

	rect.y += h;

	if (BG_CursorInRect(&rect))
	{
		VectorScale(colorYellow, 0.3f, colour);
		colour[3] = 1.0f;
	}
	else
	{
		VectorScale(colorWhite, 0.3f, colour);
		colour[3] = 1.0f;
	}

	CG_FillRect(rect.x, rect.y, rect.w, rect.h, colour);

	offsetX = CG_Text_Width_Ext(text, scalex, 0, font);
	offsetY = (CG_Text_Height_Ext(text, scaley, 0, font) + rect.h) * 0.5f;

	CG_Text_Paint_Ext(rect.x + (textboxW - offsetX) / 2.0f, rect.y + offsetY, scalex, scaley, fontColour, text, 0, 0, style, font);

	return rect.y;
}

/**
 * @brief CG_ShowHelp_On
 * @param[in,out] status
 */
void CG_ShowHelp_On(int *status)
{
	int milli = trap_Milliseconds();

	if (*status == SHOW_SHUTDOWN && milli < cg.fadeTime)
	{
		cg.fadeTime = 2 * milli + STATS_FADE_TIME - cg.fadeTime;
	}
	else if (*status != SHOW_ON)
	{
		cg.fadeTime = milli + STATS_FADE_TIME;
	}

	*status = SHOW_ON;
}

/**
 * @brief CG_ShowHelp_Off
 * @param[in,out] status
 */
void CG_ShowHelp_Off(int *status)
{
	if (*status != SHOW_OFF)
	{
		int milli = trap_Milliseconds();

		if (milli < cg.fadeTime)
		{
			cg.fadeTime = 2 * milli + STATS_FADE_TIME - cg.fadeTime;
		}
		else
		{
			cg.fadeTime = milli + STATS_FADE_TIME;
		}

		*status = SHOW_SHUTDOWN;
	}
}

/**
 * @brief CG_getBindKeyName
 * @param[in] cmd
 * @param[out] buf
 * @param[in] len
 * @return
 */
static char *CG_getBindKeyName(const char *cmd, char *buf, size_t len)
{
	int j;

	for (j = 0; j < 256; j++)
	{
		trap_Key_GetBindingBuf(j, buf, len);
		if (*buf == 0)
		{
			continue;
		}

		if (!Q_stricmp(buf, cmd))
		{
			trap_Key_KeynumToStringBuf(j, buf, MAX_STRING_TOKENS);
			Q_strupr(buf);
			return(buf);
		}
	}

	Q_strncpyz(buf, va("(%s)", cmd), len);
	return(buf);
}

/**
 * @brief CG_DrawHelpWindow
 * @param[in] x
 * @param[in] y
 * @param[in] title
 * @param[in] help
 * @param[in] cmdNumber
 * @param[in] bgColor
 * @param[in] borderColor
 * @param[in] bgColorTitle
 * @param[in] borderColorTitle
 * @param[in] fontHeader
 * @param[in] fontText
 */
void CG_DrawHelpWindow(float x, float y, int *status, const char *title, const helpType_t *help, unsigned int cmdNumber,
                       const vec4_t backgroundColor, const vec4_t borderColor, const vec4_t backgroundColorTitle, const vec4_t borderColorTitle,
                       panel_button_text_t *fontHeader, panel_button_text_t *fontText)
{
	float        diff;
	unsigned int i;
	int          len, maxlen = 0;
	int          w, h;
	char         format[MAX_STRING_TOKENS], buf[MAX_STRING_TOKENS];
	char         *lines[32];
	int          tSpacing = 9;      // Should derive from CG_Text_Height_Ext
	vec4_t       bgColor         ;
	vec4_t       bgColorTitle    ;
	vec4_t       bdColor     ;
	vec4_t       bdColorTitle;
	vec4_t       hdrColor;
	vec4_t       tColor;

	Vector4Copy(backgroundColor, bgColor);
	Vector4Copy(borderColor, bgColorTitle);
	Vector4Copy(backgroundColorTitle, bdColor);
	Vector4Copy(borderColorTitle, bdColorTitle);
	Vector4Copy(fontHeader->colour, hdrColor);
	Vector4Copy(fontText->colour, tColor);

	diff = cg.fadeTime - trap_Milliseconds();

	// FIXME: Should compute all this stuff beforehand
	// Compute required width
	for (i = 0; i < cmdNumber; i++)
	{
		if (help[i].cmd != NULL)
		{
			len = (int)strlen(CG_getBindKeyName(help[i].cmd, buf, sizeof(buf)));
			if (len > maxlen)
			{
				maxlen = len;
			}
		}
	}

	Q_strncpyz(format, va("^7%%%ds ^3%%s", maxlen), sizeof(format));
	for (i = 0, maxlen = 0; i < cmdNumber; i++)
	{
		if (help[i].cmd != NULL)
		{
			lines[i] = va(format, CG_getBindKeyName(help[i].cmd, buf, sizeof(buf)), help[i].info);
			len      = CG_Text_Width_Ext(lines[i], fontText->scalex, 0, fontText->font);
			if (len > maxlen)
			{
				maxlen = len;
			}
		}
		else
		{
			lines[i] = NULL;
		}
	}

	w = maxlen + 8;
	h = 2 + tSpacing + 2 +                                  // Header
	    2 + 1 +
	    tSpacing * (cmdNumber) +
	    2;

	// Fade-in effects
	if (diff > 0.0f)
	{
		float scale = (diff / STATS_FADE_TIME);

		if (*status == SHOW_ON)
		{
			scale = 1.0f - scale;
		}

		bgColor[3]      *= scale;
		bgColorTitle[3] *= scale;
		bdColor[3]      *= scale;
		bdColorTitle[3] *= scale;
		hdrColor[3]     *= scale;
		tColor[3]       *= scale;

		x -= w * (1.0f - scale);
	}
	else if (*status == SHOW_SHUTDOWN)
	{
		*status = SHOW_OFF;
		return;
	}

	CG_FillRect(x, y, w, h, bgColor);
	CG_DrawRect(x, y, w, h, 1, bdColor);

	y += 1;

	// Header
	CG_FillRect(x + 1, y, w - 2, tSpacing + 4, bgColorTitle);
	CG_DrawRect(x + 1, y, w - 2, tSpacing + 4, 1, bdColorTitle);

	x += 4;
	y += tSpacing;

	CG_Text_Paint_Ext(x, y, fontHeader->scalex, fontHeader->scaley, hdrColor, title, 0.0f, 0, fontHeader->style, fontHeader->font);

	y += 3;

	// Control info
	for (i = 0; i < cmdNumber; i++)
	{
		y += tSpacing;
		if (lines[i] != NULL)
		{
			CG_Text_Paint_Ext(x, y, fontText->scalex, fontText->scaley, tColor, lines[i], 0.0f, 0, fontText->style, fontText->font);
		}
	}
}

/**
 * @brief CG_ComputeScale
 * @param[in] comp
 * @return
 */
float CG_ComputeScale(hudComponent_t *comp /*, float height, float scale, fontHelper_t *font*/)
{
	return comp->hardScale * (comp->scale / 100.f);
	//return (height / (Q_UTF8_GlyphScale(font) * Q_UTF8_GetGlyph(font, "A")->height)) * (scale / 100.f);
}

void CG_DrawCursor(float x, float y)
{
	if (!cgDC.cursorVisible)
	{
		return;
	}

	CG_DrawPic(x, y, CURSOR_SIZE, CURSOR_SIZE, cgs.media.cursorIcon);
}
