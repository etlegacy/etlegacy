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
 * value to the player by virtue of how full a drawn box is.
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] percent
 *
 * @note Unused
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
 * @brief CG_GetColorForHealth
 * @param[in] health
 * @param[in] hcolor
 *
 * @note Unused
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
	else if (health <= 66)
	{
		hcolor[2] = 0;
	}
	else
	{
		hcolor[2] = (health - 66.f) / 33.0f;
	}

	if (health > 66)
	{
		hcolor[1] = 1.0;
	}
	else if (health <= 33)
	{
		hcolor[1] = 0;
	}
	else
	{
		hcolor[1] = (health - 33.f) / 33.0f;
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
void CG_AddOnScreenText(const char *text, vec3_t origin)
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
		cg.specOnScreenLabels[cg.specStringCount].x     = x;
		cg.specOnScreenLabels[cg.specStringCount].y     = y;
		cg.specOnScreenLabels[cg.specStringCount].scale = scale;
		cg.specOnScreenLabels[cg.specStringCount].text  = text;
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
 * @brief CG_WordWrapString breaks string onto lines respecting the maxLineChars
 * @param[in]  input
 * @param[in]  maxLineChars
 * @param[out] output
 * @param[in]  maxOutputSize
 */
char *CG_WordWrapString(const char *input, int maxLineChars, char *output, int maxOutputSize)
{
	int i = 0, o = 0, l, k;
	int lineSplit = 0;
	int lineWidth = 0;

	if (!input || !output || maxOutputSize <= 0)
	{
		return NULL;
	}

	while (input[i] && (o + 1) < maxOutputSize)
	{
		// split line
		if (lineWidth >= maxLineChars)
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
				output[lineSplit] = '\n';
				lineWidth         = Q_UTF8_PrintStrlenExt(output + lineSplit + 1, o - (lineSplit + 1)); // get line length
				lineSplit         = 0;
				continue;
			}
			output[o++] = '\n';
			lineWidth   = 0;
			lineSplit   = 0;
			continue;
		}

		// linebreak marker
		if (input[i] == '\\' && input[i + 1] == 'n')
		{
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
static int CG_ComputeLinePosX(float x, float scalex, const char *text, int align, fontHelper_t *font)
{
	int        lineW, lineL = 0;
	const char *ln = strchr(text, '\n');

	if (ln)
	{
		lineL = Q_UTF8_PrintStrlenExt(text, ln - text);
	}

	lineW = CG_Text_Width_Ext(text, scalex, lineL, font);

	if (align == ITEM_ALIGN_CENTER)
	{
		return x - lineW / 2;
	}
	else if (align == ITEM_ALIGN_RIGHT)
	{
		return x - lineW;
	}

	return x;
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
void CG_DrawMultilineText(float x, float y, float scalex, float scaley, vec4_t color, const char *text, int lineHeight, float adjust, int limit, int style, int align, fontHelper_t *font)
{
	vec4_t      newColor;
	glyphInfo_t *glyph;
	const char  *s = text;
	float       yadj;
	int         count = 0, ofs;
	int         lineX = x, lineY = y;
	float       fontSizeX = scalex;
	float       fontSizeY = scaley;

	if (!text)
	{
		return;
	}

	fontSizeX *= Q_UTF8_GlyphScale(font);
	fontSizeY *= Q_UTF8_GlyphScale(font);

	trap_R_SetColor(color);
	Vector4Copy(color, newColor);

	if (limit <= 0)
	{
		limit = MAX_QINT;
	}

	if (align > ITEM_ALIGN_LEFT)
	{
		lineX = CG_ComputeLinePosX(x, scalex, text, align, font);
	}

	while (s && *s && count < limit)
	{
		if (*s == '\n')
		{
			s++;
			lineY += lineHeight;

			if (align > ITEM_ALIGN_LEFT)
			{
				lineX = CG_ComputeLinePosX(x, scalex, s, align, font);
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
			trap_R_SetColor(newColor);
			s += 2;
			continue;
		}

		yadj = fontSizeY * glyph->top;

		if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE)
		{
			ofs           = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
			colorBlack[3] = newColor[3];
			trap_R_SetColor(colorBlack);
			CG_Text_PaintChar_Ext(lineX + (glyph->pitch * fontSizeX) + ofs, lineY - yadj + ofs, glyph->imageWidth, glyph->imageHeight, fontSizeX, fontSizeY, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
			colorBlack[3] = 1.0;
			trap_R_SetColor(newColor);
		}

		CG_Text_PaintChar_Ext(lineX + (glyph->pitch * fontSizeX), lineY - yadj, glyph->imageWidth, glyph->imageHeight, fontSizeX, fontSizeY, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
		lineX += (glyph->xSkip * fontSizeX) + adjust;
		s     += Q_UTF8_Width(s);
		count++;
	}

	trap_R_SetColor(NULL);
}
