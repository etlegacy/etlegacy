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
 * @file cg_draw.c
 * @brief Draw all of the graphical elements during active (after loading)
 *        gameplay
 */

#include "cg_local.h"

char *Binding_FromName(const char *cvar);
void Controls_GetConfig(void);
void CG_DrawOverlays(void);
int activeFont;

extern vec4_t HUD_Border;

///////////////////////
////// new hud stuff
///////////////////////

/**
 * @brief CG_Text_SetActiveFont
 * @param[in] font
 */
void CG_Text_SetActiveFont(int font)
{
	activeFont = font;
}

/**
 * @brief CG_Text_Width_Ext
 * @param[in] text
 * @param[in] scale
 * @param[in] limit
 * @param[in] font
 * @return
 */
int CG_Text_Width_Ext(const char *text, float scale, int limit, fontHelper_t *font)
{
	return (int) CG_Text_Width_Ext_Float(text, scale, limit, font);
}

/**
 * @brief CG_Text_Width_Ext_Float
 * @param[in] text
 * @param[in] scale
 * @param[in] limit
 * @param[in] font
 * @return
 */
float CG_Text_Width_Ext_Float(const char *text, float scale, int limit, fontHelper_t *font)
{
	const char  *s = text;
	glyphInfo_t *glyph;
	int         count = 0;
	int         len   = 0;
	float       out   = 0;

	if (!text)
	{
		return 0;
	}

	len = Q_UTF8_Strlen(text);

	if (limit > 0 && len > limit)
	{
		len = limit;
	}

	while (s && *s && count < len)
	{
		if (Q_IsColorString(s))
		{
			s += 2;
			continue;
		}
		else
		{
			glyph = Q_UTF8_GetGlyph(font, s);
			out  += glyph->xSkip;
			s    += Q_UTF8_Width(s);
			count++;
		}
	}

	return out * scale * Q_UTF8_GlyphScale(font);
}

/**
 * @brief Calculates the point count for the string buffer until line end ('\n' or '\0')
 * @param[in] text string buffer to count
 * @param[in] scale font scale
 * @param[in] font font to use
 * @return string buffer point width
 */
float CG_Text_Line_Width_Ext_Float(const char *text, float scale, fontHelper_t *font)
{
	const char  *s = text;
	glyphInfo_t *glyph;
	float       out = 0;

	if (!text)
	{
		return 0;
	}

	while (s && *s)
	{
		if (Q_IsColorString(s))
		{
			s += 2;
			continue;
		}
		else if (*s == '\n')
		{
			break;
		}
		else
		{
			glyph = Q_UTF8_GetGlyph(font, s);
			out  += glyph->xSkip;
			s    += Q_UTF8_Width(s);
		}
	}

	return out * scale * Q_UTF8_GlyphScale(font);
}

/**
 * @brief CG_Text_Width
 * @param[in] text
 * @param[in] scale
 * @param[in] limit
 * @return
 */
int CG_Text_Width(const char *text, float scale, int limit)
{
	fontHelper_t *font = &cgDC.Assets.fonts[activeFont];

	return CG_Text_Width_Ext(text, scale, limit, font);
}

/**
 * @brief CG_Text_Height_Ext
 * @param[in] text
 * @param[in] scale
 * @param[in] limit
 * @param[in] font
 * @return
 */
int CG_Text_Height_Ext(const char *text, float scale, int limit, fontHelper_t *font)
{
	float max = 0;

	if (text)
	{
		const char  *s = text;
		glyphInfo_t *glyph;
		int         count = 0;
		int         len   = Q_UTF8_Strlen(text);

		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		while (s && *s && count < len)
		{
			if (Q_IsColorString(s))
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = Q_UTF8_GetGlyph(font, s);
				if (max < glyph->height)
				{
					max = glyph->height;
				}
				s += Q_UTF8_Width(s);
				count++;
			}
		}
	}
	return max * scale * Q_UTF8_GlyphScale(font);
}

/**
 * @brief CG_Text_Height
 * @param[in] text
 * @param[in] scale
 * @param[in] limit
 * @return
 */
int CG_Text_Height(const char *text, float scale, int limit)
{
	fontHelper_t *font = &cgDC.Assets.fonts[activeFont];

	return CG_Text_Height_Ext(text, scale, limit, font);
}

/**
 * @brief CG_Text_PaintChar_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] s
 * @param[in] t
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 */
void CG_Text_PaintChar_Ext(float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2, qhandle_t hShader)
{
	w *= scalex;
	h *= scaley;
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

/**
 * @brief CG_Text_PaintChar
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] scale
 * @param[in] s
 * @param[in] t
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 */
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader)
{
	float w = width * scale;
	float h = height * scale;

	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

/**
 * @brief CG_Text_Paint_Centred_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 * @param[in] style
 * @param[in] font
 */
void CG_Text_Paint_Centred_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font)
{
	x -= CG_Text_Width_Ext(text, scalex, limit, font) * 0.5f;

	CG_Text_Paint_Ext(x, y, scalex, scaley, color, text, adjust, limit, style, font);
}

/**
 * @brief CG_Text_Paint_RightAligned_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 * @param[in] style
 * @param[in] font
 */
void CG_Text_Paint_RightAligned_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font)
{
	x -= CG_Text_Width_Ext(text, scalex, limit, font);

	CG_Text_Paint_Ext(x, y, scalex, scaley, color, text, adjust, limit, style, font);
}

/**
 * @brief CG_Text_Paint_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 * @param[in] style
 * @param[in] font
 */
void CG_Text_Paint_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font)
{
	if (text)
	{
		vec4_t      newColor;
		glyphInfo_t *glyph;
		const char  *s = text;
		float       yadj;
		int         len, count = 0;
		float       newAlpha;

		scalex *= Q_UTF8_GlyphScale(font);
		scaley *= Q_UTF8_GlyphScale(font);

		len = Q_UTF8_Strlen(text);
		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		Vector4Copy(color, newColor);

		if (style == ITEM_TEXTSTYLE_BLINK || style == ITEM_TEXTSTYLE_PULSE)
		{
			newAlpha    = Q_fabs(sin(cg.time / (style == ITEM_TEXTSTYLE_BLINK ? BLINK_DIVISOR : PULSE_DIVISOR)));
			newColor[3] = newAlpha;
		}

		trap_R_SetColor(newColor);

		while (s && *s && count < len)
		{
			glyph = Q_UTF8_GetGlyph(font, s);

			if (Q_IsColorString(s))
			{
				if (*(s + 1) == COLOR_NULL)
				{
					Com_Memcpy(newColor, color, sizeof(newColor));
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
			else
			{
				yadj = scaley * glyph->top;

				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE || style == ITEM_TEXTSTYLE_OUTLINESHADOWED)
				{
					const float ofs = style == ITEM_TEXTSTYLE_SHADOWED ? TEXTSTYLE_SHADOWED_OFFSET : TEXTSTYLE_SHADOWEDMORE_OFFSET;
					colorBlack[3] = newColor[3];
					trap_R_SetColor(colorBlack);
					CG_Text_PaintChar_Ext(x + (glyph->pitch * scalex) + ofs * scalex, y - yadj + ofs * scaley, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
					colorBlack[3] = 1.0f;
					trap_R_SetColor(newColor);
				}

				CG_Text_PaintChar_Ext(x + (glyph->pitch * scalex), y - yadj, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);

				if (style == ITEM_TEXTSTYLE_OUTLINED || style == ITEM_TEXTSTYLE_OUTLINESHADOWED)
				{
					CG_Text_PaintChar_Ext(x + (glyph->pitch * scalex) - TEXTSTYLE_OUTLINED_OFFSET * scalex, y - yadj - TEXTSTYLE_OUTLINED_OFFSET * scaley, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
				}
				x += (glyph->xSkip * scalex) + adjust;
				s += Q_UTF8_Width(s);
				count++;
			}
		}
		trap_R_SetColor(NULL);
	}
}

/**
 * @brief CG_Text_Paint
 * @param[in] x
 * @param[in] y
 * @param[in] scale
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 * @param[in] style
 */
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style)
{
	fontHelper_t *font = &cgDC.Assets.fonts[activeFont];

	CG_Text_Paint_Ext(x, y, scale, scale, color, text, adjust, limit, style, font);
}

/**
 * @brief CG_Text_PaintWithCursor_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] scale
 * @param[in] color
 * @param[in] text
 * @param[in] cursorPos
 * @param[in] cursor
 * @param[in] limit
 * @param[in] style
 */
void CG_Text_PaintWithCursor_Ext(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, const char *cursor, int limit, int style, fontHelper_t *font)
{
	vec4_t      newColor = { 0, 0, 0, 0 };
	glyphInfo_t *glyph, *glyph2;
	float       useScale = scale * Q_UTF8_GlyphScale(font);

	if (text)
	{
		float      yadj;
		int        len   = Q_UTF8_Strlen(text);
		int        count = 0;
		const char *s    = text;

		trap_R_SetColor(color);
		Com_Memcpy(&newColor[0], &color[0], sizeof(vec4_t));

		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		glyph2 = Q_UTF8_GetGlyph(font, cursor);
		while (s && *s && count < len)
		{
			glyph = Q_UTF8_GetGlyph(font, s);
			yadj  = useScale * glyph->top;

			if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE)
			{
				const float ofs = style == ITEM_TEXTSTYLE_SHADOWED ? TEXTSTYLE_SHADOWED_OFFSET : TEXTSTYLE_SHADOWEDMORE_OFFSET;

				colorBlack[3] = newColor[3];
				trap_R_SetColor(colorBlack);
				CG_Text_PaintChar(x + (glyph->pitch * useScale) + ofs * useScale, y - yadj + ofs * useScale,
				                  glyph->imageWidth,
				                  glyph->imageHeight,
				                  useScale,
				                  glyph->s,
				                  glyph->t,
				                  glyph->s2,
				                  glyph->t2,
				                  glyph->glyph);
				colorBlack[3] = 1.0;
				trap_R_SetColor(newColor);
			}
			CG_Text_PaintChar(x + (glyph->pitch * useScale), y - yadj,
			                  glyph->imageWidth,
			                  glyph->imageHeight,
			                  useScale,
			                  glyph->s,
			                  glyph->t,
			                  glyph->s2,
			                  glyph->t2,
			                  glyph->glyph);

			// CG_DrawPic(x, y - yadj, scale * uiDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * uiDC.Assets.textFont.glyphs[text[i]].imageHeight, uiDC.Assets.textFont.glyphs[text[i]].glyph);
			yadj = useScale * glyph2->top;
			if (count == cursorPos && !((cgDC.realTime / BLINK_DIVISOR) & 1))
			{
				CG_Text_PaintChar(x + (glyph->pitch * useScale), y - yadj,
				                  glyph2->imageWidth,
				                  glyph2->imageHeight,
				                  useScale,
				                  glyph2->s,
				                  glyph2->t,
				                  glyph2->s2,
				                  glyph2->t2,
				                  glyph2->glyph);
			}

			x += (glyph->xSkip * useScale);
			s += Q_UTF8_Width(s);
			count++;

		}
		// need to paint cursor at end of text
		if (cursorPos == len && !((cgDC.realTime / BLINK_DIVISOR) & 1))
		{
			yadj = useScale * glyph2->top;
			CG_Text_PaintChar(x + (glyph2->pitch * useScale), y - yadj,
			                  glyph2->imageWidth,
			                  glyph2->imageHeight,
			                  useScale,
			                  glyph2->s,
			                  glyph2->t,
			                  glyph2->s2,
			                  glyph2->t2,
			                  glyph2->glyph);
		}

		trap_R_SetColor(NULL);
	}
}

/**
 * @brief CG_Text_PaintWithCursor
 * @param[in] x
 * @param[in] y
 * @param[in] scale
 * @param[in] color
 * @param[in] text
 * @param[in] cursorPos
 * @param[in] cursor
 * @param[in] limit
 * @param[in] style
 */
void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, const char *cursor, int limit, int style)
{
	fontHelper_t *font = &cgDC.Assets.fonts[activeFont];

	CG_Text_PaintWithCursor_Ext(x, y, scale, color, text, cursorPos, cursor, limit, style, font);
}

/**
 * @brief CG_DrawTeamBackground
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] alpha
 * @param[in] team
 *
 * @note Unused
 */
void CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team)
{
	vec4_t hcolor;

	if (team == TEAM_AXIS)
	{
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
		hcolor[3] = alpha;
	}
	else if (team == TEAM_ALLIES)
	{
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
		hcolor[3] = alpha;
	}
	else
	{
		return;
	}
	trap_R_SetColor(hcolor);
	CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(NULL);
}

/**
 * @brief Text_Paint_LimitX
 * @param[in,out] maxX
 * @param[in] x
 * @param[in] y
 * @param[in] scale
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 */
static void Text_Paint_LimitX(float *maxX, float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, fontHelper_t *font)
{
	if (text)
	{
		vec4_t     newColor = { 0, 0, 0, 0 };
		const char *s       = text;
		float      max      = *maxX;
		float      useScale = scale * Q_UTF8_GlyphScale(font);
		int        len      = Q_UTF8_Strlen(text);
		int        count    = 0;

		trap_R_SetColor(color);

		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		while (s && *s && count < len)
		{
			glyphInfo_t *glyph = Q_UTF8_GetGlyph(font, s);
			if (Q_IsColorString(s))
			{
				if (*(s + 1) == COLOR_NULL)
				{
					Com_Memcpy(&newColor[0], &color[0], sizeof(vec4_t));
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
			else
			{
				float yadj = useScale * glyph->top;

				if (CG_Text_Width_Ext(s, useScale, 1, font) + x > max)
				{
					*maxX = 0;
					break;
				}
				CG_Text_PaintChar(x + (glyph->pitch * useScale), y - yadj,
				                  glyph->imageWidth,
				                  glyph->imageHeight,
				                  useScale,
				                  glyph->s,
				                  glyph->t,
				                  glyph->s2,
				                  glyph->t2,
				                  glyph->glyph);
				x    += (glyph->xSkip * useScale) + adjust;
				*maxX = x;
				count++;
				s += Q_UTF8_Width(s);
			}
		}
		trap_R_SetColor(NULL);
	}
}

/**
 * @brief Text_Paint_LimitY
 * @param[in,out] maxX
 * @param[in] x
 * @param[in] y
 * @param[in] scale
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 */
static void Text_Paint_LimitY(float *maxY, float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, fontHelper_t *font)
{
	if (text)
	{
		vec4_t     newColor = { 0, 0, 0, 0 };
		const char *s       = text;
		float      max      = *maxY;
		float      useScale = scale * Q_UTF8_GlyphScale(font);
		int        len      = Q_UTF8_Strlen(text);
		int        count    = 0;
		float      newX     = x;
		int        height   = CG_Text_Height_Ext(text, useScale, 0, font);

		trap_R_SetColor(color);

		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		while (s && *s && count < len)
		{
			glyphInfo_t *glyph = Q_UTF8_GetGlyph(font, s);
			if (Q_IsColorString(s))
			{
				if (*(s + 1) == COLOR_NULL)
				{
					Com_Memcpy(&newColor[0], &color[0], sizeof(vec4_t));
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
			else
			{
				float yadj = useScale * glyph->top;

				if (CG_Text_Height_Ext(s, useScale, 1, font) + y > max)
				{
					*maxY = 0;
					break;
				}

				CG_Text_PaintChar(newX + (glyph->pitch * useScale), y - yadj,
				                  glyph->imageWidth,
				                  glyph->imageHeight,
				                  useScale,
				                  glyph->s,
				                  glyph->t,
				                  glyph->s2,
				                  glyph->t2,
				                  glyph->glyph);

				newX += (glyph->xSkip * useScale) + adjust;

				count++;
				s += Q_UTF8_Width(s);

				if (*s == '\n')
				{
					y    += height + 5;
					*maxY = y;
					newX  = x;
				}
			}
		}
		trap_R_SetColor(NULL);
	}
}

/**
 * @brief UI_DrawHorizontalScrollingString
 * @param[in] rect
 * @param[in] color
 * @param[in] scale
 * @param[in] scrollingSpeed
 * @param[in,out] scroll
 */
void CG_DrawHorizontalScrollingString(rectDef_t *rect, vec4_t color, float scale, int scrollingRefresh, int step, scrollText_t *scroll, fontHelper_t *font)
{
	if (scroll->length)
	{
		float pos       = rect->x;
		float thickness = rect->w;
		float maxPos    = 1;

		if (!scroll->init || scroll->offset > scroll->length)
		{
			scroll->init      = qtrue;
			scroll->offset    = 0;
			scroll->paintPos  = pos + 1;
			scroll->paintPos2 = -1;
			scroll->time      = 0;
		}

		if (cgDC.realTime > scroll->time)
		{
			scroll->time = cgDC.realTime + scrollingRefresh;
			if (scroll->paintPos <= pos + step)
			{
				if (scroll->offset < scroll->length)
				{
					scroll->paintPos += CG_Text_Width(&scroll->text[scroll->offset], scale, 1) - 1;

					scroll->offset = scroll->offset + 1;
				}
				else
				{
					scroll->offset = 0;
					if (scroll->paintPos2 >= 0)
					{
						scroll->paintPos = scroll->paintPos2;
					}
					else
					{
						scroll->paintPos = pos + thickness - step;
					}
					scroll->paintPos2 = -1;
				}
			}
			else
			{
				//serverStatus.motdPaintX--;
				scroll->paintPos -= step;
				if (scroll->paintPos2 >= 0)
				{
					//serverStatus.motdPaintX2--;
					scroll->paintPos2 -= step;
				}
			}
		}

		maxPos = pos + thickness - step;

		Text_Paint_LimitX(&maxPos, scroll->paintPos, rect->y, scale, color, &scroll->text[scroll->offset], 0, 0, font);

		if (scroll->paintPos2 >= 0)
		{
			float max2 = pos + thickness - step;

			Text_Paint_LimitX(&max2, scroll->paintPos2, rect->y, scale, color, scroll->text, 0, scroll->offset, font);
		}

		if (scroll->offset && maxPos > 0)
		{
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if (scroll->paintPos2 == -1)
			{
				scroll->paintPos2 = pos + thickness - step;
			}
		}
		else
		{
			scroll->paintPos2 = -1;
		}
	}
}

/**
 * @brief UI_DrawHorizontalScrollingString
 * @param[in] rect
 * @param[in] color
 * @param[in] scale
 * @param[in] scrollingSpeed
 * @param[in,out] scroll
 */
void CG_DrawVerticalScrollingString(rectDef_t *rect, vec4_t color, float scale, int scrollingRefresh, int step, scrollText_t *scroll, fontHelper_t *font)
{
	if (scroll->length)
	{
		float pos       = rect->y;
		float thickness = rect->h;
		float maxPos    = 1;

		if (!scroll->init || scroll->offset > scroll->length)
		{
			scroll->init      = qtrue;
			scroll->offset    = 0;
			scroll->paintPos  = pos + rect->h;
			scroll->paintPos2 = -1;
			scroll->time      = 0;
		}

		if (cgDC.realTime > scroll->time)
		{
			scroll->time = cgDC.realTime + scrollingRefresh;
			if (scroll->paintPos <= pos + step)
			{
				if (scroll->offset + 1 < scroll->length)
				{
					char *tmp;

					tmp = strchr(&scroll->text[scroll->offset + 1], '\n');

					if (!tmp)
					{
						tmp = strchr(&scroll->text[scroll->offset + 1], '\0');
					}

					scroll->offset = tmp - scroll->text;

					scroll->paintPos += CG_Text_Height_Ext(scroll->text, scale, 1, font) + step;
				}
				else
				{
					scroll->offset = 0;
					if (scroll->paintPos2 >= 0)
					{
						scroll->paintPos = scroll->paintPos2;
					}
					else
					{
						scroll->paintPos = pos + rect->h;
					}
					scroll->paintPos2 = -1;
				}
			}
			else
			{
				scroll->paintPos -= step;
				if (scroll->paintPos2 >= 0)
				{
					scroll->paintPos2 -= step;
				}
			}
		}

		maxPos = pos + thickness - step;

		Text_Paint_LimitY(&maxPos, rect->x, scroll->paintPos, scale, color, &scroll->text[scroll->offset], 0, 0, font);

		if (scroll->paintPos2 >= 0)
		{
			float max2 = pos + thickness - step;

			Text_Paint_LimitY(&max2, rect->x, scroll->paintPos2, scale, color, scroll->text, 0, scroll->offset, font);
		}

		if (scroll->offset && maxPos > 0)
		{
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if (scroll->paintPos2 == -1)
			{
				scroll->paintPos2 = pos + rect->h;
			}
		}
		else
		{
			scroll->paintPos2 = -1;
		}
	}
}

/**
 * @brief CG_DrawLine
 * @param[in] start
 * @param[in] end
 * @param[in] float
 * @param[in] color
 * @param[in] shader
 */
void CG_DrawLine(const vec3_t start, const vec3_t end, float width, const vec4_t color, qhandle_t shader)
{
	polyBuffer_t *pb;
	int          vert;
	byte         bcolor[4];

	vec3_t dir, diff, up;

	pb = CG_PB_FindFreePolyBuffer(shader, 4, 6);
	if (!pb)
	{
		return;
	}

	bcolor[0] = (byte)(color[0] * 255.f);
	bcolor[1] = (byte)(color[1] * 255.f);
	bcolor[2] = (byte)(color[2] * 255.f);
	bcolor[3] = (byte)(color[3] * 255.f);

	vert = pb->numVerts;

	VectorSubtract(start, end, dir);
	VectorNormalizeFast(dir);

	// start points
	VectorSubtract(start, cg.refdef_current->vieworg, diff);
	CrossProduct(dir, diff, up);
	VectorNormalizeFast(up);
	VectorScale(up, width * .5f, up);

	VectorAdd(start, up, pb->xyz[vert + 0]);
	Vector2Set(pb->st[vert + 0], 0.0, 0.0);
	Com_Memcpy(pb->color[vert + 0], bcolor, sizeof(*pb->color));

	VectorSubtract(start, up, pb->xyz[vert + 1]);
	Vector2Set(pb->st[vert + 1], 0.0, 1.0);
	Com_Memcpy(pb->color[vert + 1], bcolor, sizeof(*pb->color));

	// end points
	VectorSubtract(end, cg.refdef_current->vieworg, diff);
	CrossProduct(dir, diff, up);
	VectorNormalizeFast(up);
	VectorScale(up, width * .5f, up);

	VectorAdd(end, up, pb->xyz[vert + 2]);
	Vector2Set(pb->st[vert + 2], 1.0, 0.0);
	Com_Memcpy(pb->color[vert + 2], bcolor, sizeof(*pb->color));

	VectorSubtract(end, up, pb->xyz[vert + 3]);
	Vector2Set(pb->st[vert + 3], 1.0, 1.0);
	Com_Memcpy(pb->color[vert + 3], bcolor, sizeof(*pb->color));

	pb->numVerts = vert + 4;

	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 2);
	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 0);
	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 1);

	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 1);
	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 3);
	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 2);
}

/*
===========================================================================================
  LOWER RIGHT CORNER
===========================================================================================
*/

/**
 * @brief CG_DrawTeamInfo
 */
void CG_DrawTeamInfo(hudComponent_t *comp)
{
	int chatHeight = TEAMCHAT_HEIGHT;

	// no need to adjust chat height for intermission here - CG_DrawTeamInfo is called from CG_Draw2D
	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
	{
		chatHeight = cg_teamChatHeight.integer;
	}

	if (chatHeight <= 0)
	{
		return; // disabled
	}

	if (cgs.teamLastChatPos != cgs.teamChatPos)
	{
		vec4_t       hcolor;
		int          i, j;
		float        alphapercent;
		float        lineHeight = comp->location.h / chatHeight;
		float        icon_width;
		float        icon_height;
		float        flagOffsetX = 0;
		qhandle_t    flag        = 0;
		fontHelper_t *font       = &cgs.media.limboFont2;
		int          maxLineLength;
		int          chatPosX = comp->location.x;
		int          chatPosY = comp->location.y + comp->location.h;
		float        scale;

		scale = CG_ComputeScale(comp /*lineHeight, comp->scale, font*/);

		icon_width    = 12.f * scale * 5;
		icon_height   = 8.f * scale * 5;
		maxLineLength = (comp->location.w - (!comp->style ? (16.f * scale * 5.f) : 0)) / ((float)Q_UTF8_GetGlyph(font, "A")->xSkip * scale * Q_UTF8_GlyphScale(font));

		if (comp->showBackGround)
		{
			CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
		}

		if (comp->showBorder)
		{
			CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
		}

		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer)
		{
			cgs.teamLastChatPos++;
		}

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--)
		{
			int chatWidthCur = 0;
			int chatWidthMax = 0;

			alphapercent = 1.0f - (cg.time - cgs.teamChatMsgTimes[i % chatHeight]) / (float)(cg_teamChatTime.integer);
			alphapercent = Com_Clamp(0.0f, 1.0f, alphapercent);

			// chatter team instead
			if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_AXIS)
			{
				hcolor[0] = 1;
				hcolor[1] = 0;
				hcolor[2] = 0;
			}
			else if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_ALLIES)
			{
				hcolor[0] = 0;
				hcolor[1] = 0;
				hcolor[2] = 1;
			}
			else
			{
				hcolor[0] = 0;
				hcolor[1] = 1;
				hcolor[2] = 0;
			}

			hcolor[3] = comp->colorBackground[3] * alphapercent;

			trap_R_SetColor(hcolor);

			if (!(comp->style & 1))
			{
				flagOffsetX = 16.f * scale * 5;
				if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_AXIS)
				{
					flag = cgs.media.axisFlag;
				}
				else if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_ALLIES)
				{
					flag = cgs.media.alliedFlag;
				}
				else
				{
					flag = 0;
				}
			}

			// get the longest chat message on screen, use that for the width of chat background
			for (j = 0; j < TEAMCHAT_HEIGHT; j++)
			{
				chatWidthCur = CG_Text_Width_Ext(cgs.teamChatMsgs[j % chatHeight], scale, maxLineLength, &cgs.media.limboFont2);

				if (chatWidthCur > chatWidthMax)
				{
					chatWidthMax = chatWidthCur;
				}
			}

			CG_DrawPic(chatPosX, chatPosY - (cgs.teamChatPos - i) * lineHeight, chatWidthMax + flagOffsetX + 2, lineHeight, cgs.media.teamStatusBar); // +2 width for some padding at the end

			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = comp->colorMain[3] * alphapercent;
			trap_R_SetColor(hcolor);

			// chat icons
			if (flag)
			{
				CG_DrawPic(chatPosX, chatPosY - (cgs.teamChatPos - i - 1) * lineHeight - icon_height, icon_width, icon_height, flag);
			}
			CG_Text_Paint_Ext(chatPosX + flagOffsetX, chatPosY - (cgs.teamChatPos - i - 1) * lineHeight - 1, scale, scale, hcolor, cgs.teamChatMsgs[i % chatHeight], 0, 0, comp->styleText, &cgs.media.limboFont2);
		}
		trap_R_SetColor(NULL);
	}
}

/**
 * @brief CG_PickupItemText
 * @param[in] item
 * @return
 */
const char *CG_PickupItemText(int itemNum)
{
	gitem_t *item;

	item = BG_GetItem(itemNum);

	if (item->giType == IT_HEALTH)
	{
		return va(CG_TranslateString("a %s"), item->pickup_name);
	}

	if (item->giType == IT_TEAM)
	{
		return CG_TranslateString("an Objective");
	}

	// FIXME: this is just related to english
	if (item->pickup_name[0] == 'a' || item->pickup_name[0] == 'A')
	{
		return va(CG_TranslateString("an %s"), item->pickup_name);
	}

	return va(CG_TranslateString("a %s"), item->pickup_name);
}

/*
===============================================================================
CENTER PRINTING
===============================================================================
*/

#define CP_LINEWIDTH (int)(Ccg_WideX(56))

/**
 * @brief Called for important messages that should stay in the center of the screen
 * for a few moments
 * @param[in] str
 * @param[in] y
 * @param[in] fontScale
 */
void CG_CenterPrint(const char *str)
{
	CG_PriorityCenterPrint(str, 0);
	cg.centerPrintTime = cg.time;  // overwrite
}

/**
 * @brief Called for important messages that should stay in the center of the screen
 * for a few moments
 * @param[in] str
 * @param[in] y
 * @param[in] fontScale
 * @param[in] priority
 */
void CG_PriorityCenterPrint(const char *str, int priority)
{
	int   maxLineChars;
	float scale, w;

	// don't draw if this print message is less important
	if (cg.centerPrintTime && priority < cg.centerPrintPriority)
	{
		return;
	}

	scale = CG_ComputeScale(&CG_GetActiveHUD()->centerprint /*CG_GetActiveHUD()->centerprint.location.h, CG_GetActiveHUD()->centerprint.scale, &cgs.media.limboFont2*/);
	w     = CG_GetActiveHUD()->centerprint.location.w;

	maxLineChars = CG_GetMaxCharsPerLine(str, scale, &cgs.media.limboFont2, w);

	CG_WordWrapString(CG_TranslateString(str), maxLineChars, cg.centerPrint, sizeof(cg.centerPrint), NULL);
	cg.centerPrintPriority = priority;
	cg.centerPrintTime     = cg.time + 2000;
}

/**
 * @brief CG_DrawCenterString
 */
void CG_DrawCenterString(hudComponent_t *comp)
{
	float  *color;
	vec4_t textColor;

	if (!cg.centerPrintTime)
	{
		return;
	}

	Vector4Copy(comp->colorMain, textColor);
	color = CG_FadeColor_Ext(cg.centerPrintTime, (int)(1000 * cg_centertime.value), textColor[3]);
	if (!color)
	{
		cg.centerPrintTime     = 0;
		cg.centerPrintPriority = 0;
		return;
	}
	textColor[3] = color[3];

	CG_DrawCompMultilineText(comp, cg.centerPrint, textColor, comp->alignText, comp->styleText, &cgs.media.limboFont2);
}

/*
================================================================================
CROSSHAIRS
================================================================================
*/

/**
 * @brief CG_DrawWeapReticle
 */
static void CG_DrawScopedReticle(void)
{
	int weapon;
	int x = (cgs.glconfig.vidWidth / 2);
	int y = (cgs.glconfig.vidHeight / 2);

	// So that we will draw reticle
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback)
	{
		weapon = cg.snap->ps.weapon;
	}
	else
	{
		weapon = cg.weaponSelect;
	}

	if (!(GetWeaponTableData(weapon)->type & WEAPON_TYPE_SCOPED))
	{
		return;
	}

	// TODO : add support for ultra-widescreen / remove 16:9 assumption

	// sides
	CG_FillRect(0, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);
	CG_FillRect(560 + cgs.wideXoffset, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);

	// center
	if (cgs.media.reticleShaderSimple)
	{
		CG_DrawPic(80 + cgs.wideXoffset, 0, SCREEN_HEIGHT, SCREEN_HEIGHT, cgs.media.reticleShaderSimple);
	}

	if (weapon == WP_FG42_SCOPE)
	{
		// hairs
		trap_R_SetColor(colorBlack);

		// inside left
		trap_R_DrawStretchPic((x - 1) * 0.80,
		                      y - 1,
		                      (x - 1) * 0.20, 2, 0, 0, 0, 1, cgs.media.whiteShader);
		// outside left
		trap_R_DrawStretchPic(0,
		                      y - 2,
		                      (x - 1) * 0.80, 4, 0, 0, 0, 1, cgs.media.whiteShader);
		// inside right
		trap_R_DrawStretchPic(x,
		                      y - 1,
		                      (x - 1) * 0.80, 2, 0, 0, 0, 1, cgs.media.whiteShader);
		// outside right
		trap_R_DrawStretchPic(cgs.glconfig.vidWidth - (x * 0.80),
		                      y - 2,
		                      (x - 1) * 0.80, 4, 0, 0, 0, 1, cgs.media.whiteShader);
		// inside top
		trap_R_DrawStretchPic(x - 2,
		                      0,
		                      4, (y - 1) * 0.65, 0, 0, 0, 1, cgs.media.whiteShader);
		// outside top
		trap_R_DrawStretchPic(x - 1,
		                      ((y - 1) * 0.65),
		                      2, (y - 1) * 0.35, 0, 0, 0, 1, cgs.media.whiteShader);
		// inside bottom
		trap_R_DrawStretchPic(x - 1,
		                      y + 1,
		                      2, (y - 1) * 0.35, 0, 0, 0, 1, cgs.media.whiteShader);
		// outside bottom
		trap_R_DrawStretchPic(x - 2,
		                      y + 1 + ((y - 1) * 0.35),
		                      4, (y - 1) * 0.65, 0, 0, 0, 1, cgs.media.whiteShader);
		// center
		trap_R_DrawStretchPic(x - 1,
		                      y - 1,
		                      2, 2, 0, 0, 0, 1, cgs.media.whiteShader);

		trap_R_SetColor(NULL);
	}
	else if (weapon == WP_GARAND_SCOPE || weapon == WP_K43_SCOPE)
	{
		// hairs
		trap_R_SetColor(colorBlack);

		// left
		trap_R_DrawStretchPic(0,
		                      y - 2,
		                      (x * 0.85), 4, 0, 0, 0, 1, cgs.media.whiteShader);
		// right
		trap_R_DrawStretchPic(cgs.glconfig.vidWidth - (x * 0.85),
		                      y - 2,
		                      (x * 0.85), 4, 0, 0, 0, 1, cgs.media.whiteShader);
		// inside bottom
		trap_R_DrawStretchPic(x - 1,
		                      y + 1,
		                      2, (y - 1) * 0.25, 0, 0, 0, 1, cgs.media.whiteShader);
		// outside bottom
		trap_R_DrawStretchPic(x - 2,
		                      y + 1 + ((y - 1) * 0.25),
		                      4, (y - 1) * 0.75, 0, 0, 0, 1, cgs.media.whiteShader);

		trap_R_SetColor(NULL);
	}
}

/**
 * @brief CG_DrawMortarReticle
 */
static void CG_DrawMortarReticle(void)
{
	vec4_t   color             = { 1.f, 1.f, 1.f, .5f };
	vec4_t   color_back        = { 0.f, 0.f, 0.f, .25f };
	vec4_t   color_extends     = { .77f, .73f, .1f, 1.f };
	vec4_t   color_lastfire    = { .77f, .1f, .1f, 1.f };
	vec4_t   color_firerequest = { 1.f, 1.f, 1.f, 1.f };
	float    offset, localOffset;
	int      i, min, majorOffset, val, printval, fadeTime, requestFadeTime;
	char     *s;
	float    angle, angleMin, angleMax;
	qboolean hasRightTarget, hasLeftTarget;

	// Background
	CG_FillRect(136 + cgs.wideXoffset, 236, 154, 38, color_back);
	CG_FillRect(290 + cgs.wideXoffset, 160, 60, 208, color_back);
	CG_FillRect(350 + cgs.wideXoffset, 236, 154, 38, color_back);

	// Horizontal bar

	// bottom
	CG_FillRect(140 + cgs.wideXoffset, 264, 150, 1, color);    // left
	CG_FillRect(350 + cgs.wideXoffset, 264, 150, 1, color);    // right

	// 10 units - 5 degrees
	// total of 360 units
	// nothing displayed between 150 and 210 units
	// 360 / 10 = 36 bits, means 36 * 5 = 180 degrees
	// that means left is cg.predictedPlayerState.viewangles[YAW] - .5f * 180
	angle = 360 - AngleNormalize360(cg.predictedPlayerState.viewangles[YAW] - 90.f);

	offset      = (5.f / 65536) * ((int)(angle * (65536 / 5.f)) & 65535);
	min         = (int)(AngleNormalize360(angle - .5f * 180) / 15.f) * 15;
	majorOffset = (int)(floor((int)floor(AngleNormalize360(angle - .5f * 180)) % 15) / 5);

	for (val = i = 0; i < 36; i++)
	{
		localOffset = i * 10.f + (offset * 2.f);

		if (localOffset >= 150 && localOffset <= 210)
		{
			if (i % 3 == majorOffset)
			{
				val++;
			}
			continue;
		}

		if (i % 3 == majorOffset)
		{
			printval = min - val * 15 + 180;

			// rain - old tertiary abuse was nasty and had undefined result
			if (printval < 0)
			{
				printval += 360;
			}
			else if (printval >= 360)
			{
				printval -= 360;
			}

			s = va("%i", printval);
			CG_Text_Paint_Ext(500 - localOffset  + cgs.wideXoffset - .5f * CG_Text_Width_Ext(s, .15f, 0, &cgs.media.limboFont1), 244, .15f, .15f, color, s, 0, 0, 0, &cgs.media.limboFont1);
			CG_FillRect(500 - localOffset + cgs.wideXoffset, 248, 1, 16, color);
			val++;
		}
		else
		{
			CG_FillRect(500 - localOffset + cgs.wideXoffset, 256, 1, 8, color);
		}
	}

	// the extremes
	// 30 degrees plus a 15 degree border
	angleMin = AngleNormalize360(360 - (cg.pmext.mountedWeaponAngles[YAW] - 90.f) - (30.f + 15.f));
	angleMax = AngleNormalize360(360 - (cg.pmext.mountedWeaponAngles[YAW] - 90.f) + (30.f + 15.f));

	// right
	localOffset = (AngleNormalize360(angle - angleMin) / 5.f) * 10.f;
	CG_FillRect(320 - localOffset + cgs.wideXoffset, 252, 2, 18, color_extends);

	// left
	localOffset = (AngleNormalize360(angleMax - angle) / 5.f) * 10.f;
	CG_FillRect(320 + localOffset + cgs.wideXoffset, 252, 2, 18, color_extends);

	// last fire pos
	fadeTime = 0;
	if (CHECKBITWISE(GetWeaponTableData(cg.lastFiredWeapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET) && cg.mortarImpactTime >= -1)
	{
		fadeTime = cg.time - (cg.predictedPlayerEntity.firedTime + 5000);

		if (fadeTime < 3000)
		{
			float lastfireAngle;

			if (fadeTime > 0)
			{
				color_lastfire[3] = 1.f - (fadeTime / 3000.f);
			}

			lastfireAngle = AngleNormalize360(360 - (cg.mortarFireAngles[YAW] - 90.f));

			localOffset = ((AngleSubtract(angle, lastfireAngle)) / 5.f) * 10.f;
			CG_FillRect(320 - localOffset + cgs.wideXoffset, 252, 2, 18, color_lastfire);
		}
	}

	// mortar attack requests
	hasRightTarget = hasLeftTarget = qfalse;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		requestFadeTime = cg.time - (cg.artilleryRequestTime[i] + 25000);

		if (requestFadeTime < 5000)
		{
			vec3_t dir;
			float  yaw;
			float  attackRequestAngle;

			VectorSubtract(cg.artilleryRequestPos[i], cg.predictedPlayerEntity.lerpOrigin, dir);

			// ripped this out of vectoangles
			if (dir[1] == 0.f && dir[0] == 0.f)
			{
				yaw = 0;
			}
			else
			{
				if (dir[0] != 0.f)
				{
					yaw = (atan2(dir[1], dir[0]) * 180 / M_PI);
				}
				else if (dir[1] > 0)
				{
					yaw = 90;
				}
				else
				{
					yaw = 270;
				}
				if (yaw < 0)
				{
					yaw += 360;
				}
			}

			if (requestFadeTime > 0)
			{
				color_firerequest[3] = 1.f - (requestFadeTime / 5000.f);
			}

			attackRequestAngle = AngleNormalize360(360 - (yaw - 90.f));

			yaw = AngleSubtract(attackRequestAngle, angleMin);

			if (yaw < 0)
			{
				if (!hasLeftTarget)
				{
					trap_R_SetColor(color_firerequest);
					CG_DrawPic(136 + 2 + cgs.wideXoffset, 236 + 38 - 10 + 1, 8, 8, cgs.media.mortarTargetArrow);
					trap_R_SetColor(NULL);

					hasLeftTarget = qtrue;
				}
			}
			else if (yaw > 90)
			{
				if (!hasRightTarget)
				{
					trap_R_SetColor(color_firerequest);
					CG_DrawPic(350 + 154 - 10 + cgs.wideXoffset, 236 + 38 - 10 + 1, -8, 8, cgs.media.mortarTargetArrow);
					trap_R_SetColor(NULL);

					hasRightTarget = qtrue;
				}
			}
			else
			{
				localOffset = ((AngleSubtract(angle, attackRequestAngle)) / 5.f) * 10.f;

				trap_R_SetColor(color_firerequest);
				CG_DrawPic(320 - localOffset - 8 + cgs.wideXoffset, 264 - 8, 16, 16, cgs.media.mortarTarget);
				trap_R_SetColor(NULL);
			}
		}
	}

	// Vertical bar

	// sides
	CG_FillRect(295 + cgs.wideXoffset, 164, 1, 200, color);    // left
	CG_FillRect(345 + cgs.wideXoffset, 164, 1, 200, color);    // right

	// 10 units - 2.5 degrees
	// total of 200 units
	// 200 / 10 = 20 bits, means 20 * 2.5 = 50 degrees
	// that means left is cg.predictedPlayerState.viewangles[PITCH] - .5f * 50
	angle = AngleNormalize180(360 - (cg.predictedPlayerState.viewangles[PITCH] - 60));

	offset      = (2.5f / 65536) * ((int)(angle * (65536 / 2.5f)) & 65535);
	min         = floor((angle + .5f * 50) / 10.f) * 10;
	majorOffset = (int)(floor((int)((angle + .5f * 50) * 10.f) % 100) / 25);

	for (val = i = 0; i < 20; i++)
	{
		localOffset = i * 10.f + (offset * 4.f);

		if (i % 4 == majorOffset)
		{
			printval = min - val * 10;

			// old tertiary abuse was nasty and had undefined result
			if (printval <= -180)
			{
				printval += 360;
			}
			else if (printval >= 180)
			{
				printval -= 180;
			}

			s = va("%i", printval);
			CG_Text_Paint_Ext(320  + cgs.wideXoffset - .5f * CG_Text_Width_Ext(s, .15f, 0, &cgs.media.limboFont1), 164 + localOffset + .5f * CG_Text_Height_Ext(s, .15f, 0, &cgs.media.limboFont1), .15f, .15f, color, s, 0, 0, 0, &cgs.media.limboFont1);
			CG_FillRect(295 + 1 + cgs.wideXoffset, 164 + localOffset, 12, 1, color);
			CG_FillRect(345 - 12 + cgs.wideXoffset, 164 + localOffset, 12, 1, color);
			val++;
		}
		else
		{
			CG_FillRect(295 + 1 + cgs.wideXoffset, 164 + localOffset, 8, 1, color);
			CG_FillRect(345 - 8 + cgs.wideXoffset, 164 + localOffset, 8, 1, color);
		}
	}

	// the extremes
	// 30 degrees up
	// 20 degrees down
	angleMin = AngleNormalize180(360 - (cg.pmext.mountedWeaponAngles[PITCH] - 60)) - 20.f;
	angleMax = AngleNormalize180(360 - (cg.pmext.mountedWeaponAngles[PITCH] - 60)) + 30.f;

	// top
	localOffset = angleMax - angle;
	if (localOffset < 0)
	{
		localOffset = 0;
	}
	localOffset = (AngleNormalize360(localOffset) / 2.5f) * 10.f;
	if (localOffset < 100)
	{
		CG_FillRect(295 - 2 + cgs.wideXoffset, 264 - localOffset, 6, 2, color_extends);
		CG_FillRect(345 - 4 + 1 + cgs.wideXoffset, 264 - localOffset, 6, 2, color_extends);
	}

	// bottom
	localOffset = angle - angleMin;
	if (localOffset < 0)
	{
		localOffset = 0;
	}
	localOffset = (AngleNormalize360(localOffset) / 2.5f) * 10.f;
	if (localOffset < 100)
	{
		CG_FillRect(295 - 2 + cgs.wideXoffset, 264 + localOffset, 6, 2, color_extends);
		CG_FillRect(345 - 4 + 1 + cgs.wideXoffset, 264 + localOffset, 6, 2, color_extends);
	}

	// last fire pos
	if (CHECKBITWISE(GetWeaponTableData(cg.lastFiredWeapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET) && cg.mortarImpactTime >= -1)
	{
		if (fadeTime < 3000)
		{
			float lastfireAngle = AngleNormalize180(360 - (cg.mortarFireAngles[PITCH] - 60));

			if (lastfireAngle > angle)
			{
				localOffset = lastfireAngle - angle;
				if (localOffset < 0)
				{
					localOffset = 0;
				}
				localOffset = (AngleNormalize360(localOffset) / 2.5f) * 10.f;
				if (localOffset < 100)
				{
					CG_FillRect(295 - 2 + cgs.wideXoffset, 264 - localOffset, 6, 2, color_lastfire);
					CG_FillRect(345 - 4 + 1 + cgs.wideXoffset, 264 - localOffset, 6, 2, color_lastfire);
				}
			}
			else
			{
				localOffset = angle - lastfireAngle;
				if (localOffset < 0)
				{
					localOffset = 0;
				}
				localOffset = (AngleNormalize360(localOffset) / 2.5f) * 10.f;
				if (localOffset < 100)
				{
					CG_FillRect(295 - 2 + cgs.wideXoffset, 264 + localOffset, 6, 2, color_lastfire);
					CG_FillRect(345 - 4 + 1 + cgs.wideXoffset, 264 + localOffset, 6, 2, color_lastfire);
				}
			}
		}
	}
}

/**
 * @brief CG_DrawBinocReticle
 */
static void CG_DrawBinocReticle(void)
{
	if (cgs.media.binocShaderSimple)
	{
		CG_DrawPic(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, cgs.media.binocShaderSimple);
	}

	CG_FillRect(146 + cgs.wideXoffset, 239, 348, 1, colorBlack);

	CG_FillRect(188 + cgs.wideXoffset, 234, 1, 13, colorBlack);     // ll
	CG_FillRect(234 + cgs.wideXoffset, 226, 1, 29, colorBlack);     // l
	CG_FillRect(274 + cgs.wideXoffset, 234, 1, 13, colorBlack);     // lr
	CG_FillRect(320 + cgs.wideXoffset, 213, 1, 55, colorBlack);     // center
	CG_FillRect(360 + cgs.wideXoffset, 234, 1, 13, colorBlack);     // rl
	CG_FillRect(406 + cgs.wideXoffset, 226, 1, 29, colorBlack);     // r
	CG_FillRect(452 + cgs.wideXoffset, 234, 1, 13, colorBlack);     // rr
}

/**
 * @brief CG_DrawNoShootIcon
 */
static void CG_DrawNoShootIcon(hudComponent_t *comp)
{
	float x, y, w, h;

	if ((cg.predictedPlayerState.eFlags & EF_PRONE) && (GetWeaponTableData(cg.snap->ps.weapon)->type & WEAPON_TYPE_PANZER))
	{
		trap_R_SetColor(colorRed);
	}
	else if (cg.crosshairClientNoShoot)
	{
		float *color = CG_FadeColor(cg.crosshairEntTime, cg_drawCrosshairFade.integer);

		if (!color)
		{
			trap_R_SetColor(NULL);
			return;
		}

		trap_R_SetColor(color);
	}
	else
	{
		return;
	}

	x = comp->location.x;
	y = comp->location.y;
	w = comp->location.w;
	h = comp->location.h;
	CG_AdjustFrom640(&x, &y, &w, &h);

	trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, cgs.media.friendShader);
	trap_R_SetColor(NULL);
}

/**
 * @brief CG_DrawCrosshair
 */
void CG_DrawCrosshair(hudComponent_t *comp)
{
	float     w, h;
	qhandle_t hShader;
	float     f;
	float     x, y;
	int       weapnum;

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0 && !(cg.snap->ps.pm_flags & PMF_FOLLOW) && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.renderingThirdPerson)
	{
		return;
	}

	// using binoculars
	if (cg.zoomedBinoc)
	{
		CG_DrawBinocReticle();
		return;
	}

	// show reticle in limbo and spectator
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback)
	{
		weapnum = cg.snap->ps.weapon;
	}
	else
	{
		weapnum = cg.weaponSelect;
	}

	// no weapon, no crosshair
	if (weapnum == WP_NONE && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		return;
	}

	// special reticle for scoped weapon
	if (cg.zoomed)
	{
		if (!BG_PlayerMounted(cg.snap->ps.eFlags))
		{
			if (
#ifdef FEATURE_MULTIVIEW
				cg.mvTotalClients < 1 ||
#endif
				cg.snap->ps.stats[STAT_HEALTH] > 0)
			{
				CG_DrawScopedReticle();
			}

			return;
		}
	}

	if (cg.snap->ps.eFlags & EF_PRONE_MOVING)
	{
		return;
	}

	// FIXME: spectators/chasing?
	if (CHECKBITWISE(GetWeaponTableData(cg.predictedPlayerState.weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET) && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
	{
		CG_DrawMortarReticle();
		return;
	}

	if (cg_drawCrosshair.integer < 0)     // moved down, so it doesn't keep the scoped weapons from drawing reticles
	{
		return;
	}

	// no crosshair while leaning
	if (cg.snap->ps.leanf != 0.f)
	{
		return;
	}

	if (cg_customCrosshair.integer > CUSTOMCROSSHAIR_NONE && cg_customCrosshair.integer < CUSTOMCROSSHAIR_MAX)
	{
		CG_DrawCustomCrosshair();
		CG_DrawNoShootIcon(comp);
		return;
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	// crosshair size represents aim spread
	f = (float)(!(comp->style & CROSSHAIR_PULSE) ? 0 : cg.snap->ps.aimSpreadScale / 255.0);
	w = comp->location.w * (1 + f * 2.0f);
	h = comp->location.h * (1 + f * 2.0f);
	x = comp->location.x + (comp->location.w - w) * .5f;
	y = comp->location.y + (comp->location.h - h) * .5f;

	CG_AdjustFrom640(&x, &y, &w, &h);

	// set color based on health
	if (comp->style & CROSSHAIR_DYNAMIC_COLOR)
	{
		vec4_t hcolor;

		Vector4Copy(comp->colorMain, hcolor);
		CG_ColorForHealth(cg.snap->ps.stats[STAT_HEALTH], hcolor);
		hcolor[3] = comp->colorMain[3];
		trap_R_SetColor(hcolor);
	}
	else
	{
		trap_R_SetColor(comp->colorMain);
	}

	hShader = cgs.media.crosshairShader[cg_drawCrosshair.integer % NUM_CROSSHAIRS];

	trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, hShader);

	if (cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS])
	{
		// crosshair size represents aim spread
		f = (float)(!(comp->style & CROSSHAIR_PULSE_ALT) ? 0 : cg.snap->ps.aimSpreadScale / 255.0);
		w = comp->location.w * (1 + f * 2.0f);
		h = comp->location.h * (1 + f * 2.0f);
		x = comp->location.x + (comp->location.w - w) * .5f;
		y = comp->location.y + (comp->location.h - h) * .5f;

		CG_AdjustFrom640(&x, &y, &w, &h);

		// set color based on health
		if (comp->style & CROSSHAIR_DYNAMIC_COLOR_ALT)
		{
			vec4_t hcolor;

			Vector4Copy(comp->colorMain, hcolor);
			CG_ColorForHealth(cg.snap->ps.stats[STAT_HEALTH], hcolor);
			hcolor[3] = comp->colorSecondary[3];
			trap_R_SetColor(hcolor);
		}
		else
		{
			trap_R_SetColor(comp->colorSecondary);
		}

		trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);
	}
	trap_R_SetColor(NULL);

	CG_DrawNoShootIcon(comp);
}

/**
 * @brief Scan for any entities we're currently aiming at
 */
static void CG_ScanForCrosshairEntity()
{
	trace_t   trace;
	vec3_t    end;
	centity_t *cent = NULL;

	// no crosshair, no scan
	if (cg_drawCrosshair.integer < 0)
	{
		return;
	}

	if (cg.renderingThirdPerson)
	{
		// track the medic we are locked on
		if (cg.snap->ps.viewlocked == VIEWLOCK_MEDIC)
		{
			cg.crosshairEntNum             = cg.snap->ps.viewlocked_entNum;
			cg.crosshairEntTime            = cg.time;
			cg.identifyClientRequest       = cg.snap->ps.viewlocked_entNum;
			cg.crosshairNotLookingAtClient = qfalse;
		}

		return;
	}

	if (cg.generatingNoiseHud)
	{
		// aim on self
		cg.crosshairEntNum             = cg.snap->ps.clientNum;
		cg.crosshairEntTime            = cg.time;
		cg.identifyClientRequest       = cg.crosshairEntNum;
		cg.crosshairNotLookingAtClient = qfalse;

		return;
	}

	VectorMA(cg.refdef.vieworg, MAX_TRACE, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, end, cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM);

	if (trace.entityNum == ENTITYNUM_WORLD)
	{
		int i;
		int closestDist = 256;

		// find the closest entity from impact point
		for (i = 0; i < cg.crosshairEntsToScanCount; ++i)
		{
			// pos.trBase is not the middle of the dyna, but due to different
			// orientations relative to the map axis, this cannot be corrected in a
			// simple way unfortunately
			int dist = DistanceSquared(cg.crosshairEntsToScan[i]->currentState.pos.trBase, trace.endpos);

			if (closestDist > dist)
			{
				closestDist = dist;
				cent        = cg.crosshairEntsToScan[i];
			}
		}
	}
	else
	{
		cent = &cg_entities[trace.entityNum];
	}

	if (!cent)
	{
		return;
	}

	if (cent->currentState.number >= MAX_CLIENTS)
	{
		if (cent->currentState.eFlags & EF_TAGCONNECT)
		{
			cent = &cg_entities[cent->tagParent];

			if (!cent)
			{
				return;
			}
		}

		// is a tank with a healthbar
		// this might have some side-effects, but none right now as the script_mover is the only one that sets effect1Time
		if ((cent->currentState.eType == ET_MOVER && cent->currentState.effect1Time) ||
		    cent->currentState.eType == ET_CONSTRUCTIBLE_MARKER)
		{
			// update the fade timer
			cg.crosshairEntNum       = cent->currentState.number;
			cg.crosshairEntTime      = cg.time;
			cg.identifyClientRequest = cg.crosshairEntNum;

			// We're not looking at a client
			cg.crosshairNotLookingAtClient = qtrue;
			cg.crosshairClientNoShoot      = qfalse;
		}

		// is a dynamite or landmine and server allow displaying owner name (g_misc cvar)
		if ((cent->currentState.weapon == WP_DYNAMITE || cent->currentState.weapon == WP_LANDMINE || cent->currentState.weapon == WP_SATCHEL)
		    && cgs.clientinfo[cg.snap->ps.clientNum].team == cent->currentState.teamNum
		    && cent->currentState.otherEntityNum < MAX_CLIENTS)
		{
			// update the fade timer
			cg.crosshairEntNum       = cent->currentState.number;
			cg.crosshairEntTime      = cg.time;
			cg.identifyClientRequest = cg.crosshairEntNum;

			// We're not looking at a client
			cg.crosshairNotLookingAtClient = qtrue;
			cg.crosshairClientNoShoot      = qfalse;
		}

		return;
	}

	if (!cent->currentValid)
	{
		return;
	}

	cg.crosshairNotLookingAtClient = qfalse;
	cg.crosshairClientNoShoot      = qfalse;

	// update the fade timer
	cg.crosshairEntNum  = trace.entityNum;
	cg.crosshairEntTime = cg.time;
	if (cg.crosshairEntNum != cg.snap->ps.identifyClient)
	{
		cg.identifyClientRequest = cg.crosshairEntNum;
	}

	if (cent->currentState.powerups & (1 << PW_OPS_DISGUISED))
	{
		cg.crosshairClientNoShoot = cgs.clientinfo[cg.crosshairEntNum].team == cgs.clientinfo[cg.clientNum].team;
	}
}

/**
 * @brief CG_CheckForCursorHints
 * @note Concept in progress...
 */
static void CG_CheckForCursorHints()
{
	trace_t   trace;
	vec3_t    start, end;
	centity_t *tracent;
	float     dist;

	if (cg.renderingThirdPerson)
	{
		return;
	}

	if (cg.generatingNoiseHud)
	{
		// simulate cursor hint
		cg.cursorHintTime  = cg.time;
		cg.cursorHintFade  = cg_drawHintFade.integer;
		cg.cursorHintIcon  = HINT_BREAKABLE;
		cg.cursorHintValue = 128.f;
		return;
	}

	if (cg.snap->ps.serverCursorHint)      // server is dictating a cursor hint, use it.
	{
		cg.cursorHintTime  = cg.time;
		cg.cursorHintFade  = cg_drawHintFade.integer;   // fade out time
		cg.cursorHintIcon  = cg.snap->ps.serverCursorHint;
		cg.cursorHintValue = cg.snap->ps.serverCursorHintVal;
		return;
	}

	// From here on it's client-side cursor hints.  So if the server isn't sending that info (as an option)
	// then it falls into here and you can get basic cursorhint info if you want, but not the detailed info
	// the server sends.

	// the trace
	VectorCopy(cg.refdef_current->vieworg, start);
	VectorMA(start, CH_DIST, cg.refdef_current->viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, MASK_PLAYERSOLID);

	dist = trace.fraction * CH_DIST;

	if (trace.fraction == 1.f)
	{
		// might be water
		if ((CG_PointContents(trace.endpos, -1) & CONTENTS_WATER) && !(CG_PointContents(cg.refdef.vieworg, -1) & CONTENTS_WATER))
		{
			if (dist <= CH_WATER_DIST)
			{
				cg.cursorHintIcon  = HINT_WATER;
				cg.cursorHintTime  = cg.time;
				cg.cursorHintFade  = cg_drawHintFade.integer;
				cg.cursorHintValue = 0;
			}
		}
		return;
	}

	tracent = &cg_entities[trace.entityNum];

	// invisible entities don't show hints
	if (trace.entityNum >= MAX_CLIENTS &&
	    (tracent->currentState.powerups == STATE_INVISIBLE || tracent->currentState.powerups == STATE_UNDERCONSTRUCTION))
	{
		return;
	}

	// world
	if (trace.entityNum == ENTITYNUM_WORLD)
	{
		if ((trace.surfaceFlags & SURF_LADDER) && !(cg.snap->ps.pm_flags & PMF_LADDER))
		{
			if (dist <= CH_LADDER_DIST)
			{
				cg.cursorHintIcon  = HINT_LADDER;
				cg.cursorHintTime  = cg.time;
				cg.cursorHintFade  = cg_drawHintFade.integer;
				cg.cursorHintValue = 0;
			}
		}
	}
	else if (trace.entityNum < MAX_CLIENTS)       // people
	{   // knife
		if (GetWeaponTableData(cg.snap->ps.weapon)->type & WEAPON_TYPE_MELEE)
		{
			if (dist <= CH_KNIFE_DIST)
			{
				vec3_t pforward, eforward, attacker, target;

				VectorCopy(cg.snap->ps.viewangles, attacker);
				VectorCopy(tracent->lerpAngles, target);

				attacker[PITCH] = target[PITCH] = 0;

				AngleVectors(attacker, pforward, NULL, NULL);
				AngleVectors(target, eforward, NULL, NULL);

				if (DotProduct(eforward, pforward) > 0.6f)           // from behind(-ish)
				{
					cg.cursorHintIcon  = HINT_KNIFE;
					cg.cursorHintTime  = cg.time;
					cg.cursorHintFade  = 100;
					cg.cursorHintValue = 0;
				}
			}
		}
	}
}

/**
 * @brief Draw the crosshair health bar
 * @param[in] position
 * @param[in] health
 * @param[in] maxHealth
 */
void CG_DrawCrosshairHealthBar(hudComponent_t *comp)
{
	float  *fadeColor;
	vec4_t bgcolor, bdcolor;
	vec4_t c, c2;
	int    health, maxHealth;
	int    clientNum, class;
	float  x = comp->location.x, w = comp->location.w;
	int    style;

	// world-entity or no-entity
	if (cg.crosshairEntNum >= ENTITYNUM_WORLD)
	{
		return;
	}

	// don't draw crosshair names in shoutcast mode
	// shoutcasters can see tank and truck health
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR && cgs.clientinfo[cg.clientNum].shoutcaster &&
	    cg_entities[cg.crosshairEntNum].currentState.eType != ET_MOVER)
	{
		return;
	}

	// draw the name of the player being looked at
	fadeColor = CG_FadeColor(cg.crosshairEntTime, cg_drawCrosshairFade.integer);

	if (!fadeColor)
	{
		return;
	}

	if (cg.crosshairNotLookingAtClient)
	{
		if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			return;
		}

		if (cg_entities[cg.crosshairEntNum].currentState.eType != ET_MOVER || !cg_entities[cg.crosshairEntNum].currentState.effect1Time)
		{
			return;
		}

		health    = cg_entities[cg.crosshairEntNum].currentState.dl_intensity;
		maxHealth = 255;
	}
	else
	{
		// crosshair for disguised enemy
		if (cgs.clientinfo[cg.crosshairEntNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team)
		{
			if (!(cg_entities[cg.crosshairEntNum].currentState.powerups & (1 << PW_OPS_DISGUISED)) ||
			    cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
			{
				return;
			}

			if (BG_IsSkillAvailable(cgs.clientinfo[cg.snap->ps.clientNum].skill, SK_SIGNALS, SK_FIELDOPS_ENEMY_RECOGNITION) && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS)
			{
				return;
			}

			clientNum = cgs.clientinfo[cg.crosshairEntNum].disguiseClientNum;
			class     = (cg_entities[cg.crosshairEntNum].currentState.powerups >> PW_OPS_CLASS_1) & 7;
		}
		// we only want to see players on our team
		else if (!(cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR && cgs.clientinfo[cg.crosshairEntNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team))
		{
			clientNum = cg.crosshairEntNum;
			class     = cgs.clientinfo[cg.crosshairEntNum].cls;
		}
		else
		{
			return;
		}

		if (comp->style & CROSSHAIR_BAR_CLASS)
		{
			CG_DrawPic(x, comp->location.y, comp->location.h, comp->location.h, cgs.media.skillPics[SkillNumForClass(class)]);
			x += comp->location.h;
			w -= comp->location.h;
		}

#ifdef FEATURE_PRESTIGE
		if (cgs.prestige && cgs.clientinfo[clientNum].prestige > 0 && (comp->style & CROSSHAIR_BAR_PRESTIGE))
		{
			char  *s = va("%d", cgs.clientinfo[clientNum].prestige);
			float h;

			w -= CG_Text_Width_Ext_Float(s, comp->scale, 0, &cgs.media.limboFont2);
			h  = CG_Text_Height_Ext(s, comp->scale, 0, &cgs.media.limboFont2);

			CG_Text_Paint_Ext(comp->location.x + w, comp->location.y + (comp->location.h - h) * 0.5f, comp->scale, comp->scale, fadeColor, s, 0, 0, 0, &cgs.media.limboFont2);

			w -= comp->location.h;

			CG_DrawPic(x + w, comp->location.y, comp->location.h, comp->location.h, cgs.media.prestigePics[0]);
		}
#endif

		if (cgs.clientinfo[clientNum].rank > 0 && (comp->style & CROSSHAIR_BAR_RANK))
		{
			w -= comp->location.h;
			CG_DrawPic(x + w, comp->location.y, comp->location.h, comp->location.h, rankicons[cgs.clientinfo[clientNum].rank][cgs.clientinfo[clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
		}

		// set the health
		if (cg.crosshairEntNum == cg.snap->ps.identifyClient)
		{
			health = cg.snap->ps.identifyClientHealth;

			// identifyClientHealth is sent as unsigned char and negative numbers are not transmitted - see SpectatorThink()
			// this results in player health values behind max health
			// adjust dead player health bogus values for the health bar
			if (health > 156)
			{
				health = 0;
			}
		}
		else
		{
			// only team mate health is transmit in clientinfo, this cause a slight refresh delay for disguised ennemy
			// until they are identified through crosshair check on game side, which is connection depend
			health = cgs.clientinfo[cg.crosshairEntNum].health;
		}

		maxHealth = CG_GetPlayerMaxHealth(cg.crosshairEntNum, cgs.clientinfo[cg.crosshairEntNum].cls, cgs.clientinfo[cg.crosshairEntNum].team);
	}

	// remove unecessary style for bar customization
	style = (comp->style >> 3);

	if (style & (BAR_ICON << 1))
	{
		Vector4Copy(comp->colorMain, c);
		CG_ColorForHealth(health, c);
		c[3] = comp->colorMain[3] * fadeColor[3];

		style &= ~BAR_LERP_COLOR;
	}
	else
	{
		Vector4Copy(comp->colorMain, c);
		c[3] *= fadeColor[3];
		Vector4Copy(comp->colorSecondary, c2);
		c2[3] *= fadeColor[3];
	}

	Vector4Copy(comp->colorBackground, bgcolor);
	bgcolor[3] *= fadeColor[3];
	Vector4Copy(comp->colorBorder, bdcolor);
	bdcolor[3] *= fadeColor[3];

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, bgcolor);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, bdcolor);
	}

	CG_FilledBar(x, comp->location.y, w, comp->location.h, (style & BAR_LERP_COLOR) ? c2 : c, (style & BAR_LERP_COLOR) ? c : NULL, bgcolor, bdcolor,
	             Com_Clamp(0, 1.f, health / (float)maxHealth), 0.f, style, -1);

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_GetCrosshairNameString
 * @param[in] comp
 * @param[in] clientNum
 * @return a colorized or single color name string for crosshair info
 */
static const char *CG_GetCrosshairNameString(hudComponent_t *comp, int clientNum)
{
	char colorized[MAX_NAME_LENGTH + 2] = { 0 };

	// ensure the client is valid
	if (!cgs.clientinfo[clientNum].infoValid)
	{
		return va("unknown");
	}

	if (comp->style & 1)
	{
		// Draw them with full colors
		return cgs.clientinfo[clientNum].name;
	}

	// Draw them with a single color
	Q_ColorizeString('*', cgs.clientinfo[clientNum].cleanname, colorized, MAX_NAME_LENGTH + 2);

	return va("%s", colorized);
}

/**
 * @brief CG_DrawCrosshairNames
 * @param[in] comp
 */
void CG_DrawCrosshairNames(hudComponent_t *comp)
{
	float      *color;
	vec4_t     textColor;
	const char *s        = NULL;
	int        clientNum = -1;

	if (cg_drawCrosshair.integer < 0)
	{
		return;
	}

	// world-entity or no-entity
	if (cg.crosshairEntNum >= ENTITYNUM_WORLD)
	{
		return;
	}

	Vector4Copy(comp->colorMain, textColor);
	color = CG_FadeColor_Ext(cg.crosshairEntTime, cg_drawCrosshairFade.integer, textColor[3]);

	if (!color)
	{
		return;
	}

	// don't draw crosshair names in shoutcast mode
	// shoutcasters can see tank and truck health
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR && cgs.clientinfo[cg.clientNum].shoutcaster &&
	    cg_entities[cg.crosshairEntNum].currentState.eType != ET_MOVER)
	{
		return;
	}

	if (cg.crosshairNotLookingAtClient)
	{
		if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			entityState_t *es = &cg_entities[cg.crosshairEntNum].currentState;

			switch (es->eType)
			{
			case ET_MOVER:
				if (es->effect1Time)
				{
					s = Info_ValueForKey(CG_ConfigString(CS_SCRIPT_MOVER_NAMES), va("%i", cg.crosshairEntNum));
				}
				break;
			case ET_CONSTRUCTIBLE_MARKER:
				s = Info_ValueForKey(CG_ConfigString(CS_CONSTRUCTION_NAMES), va("%i", cg.crosshairEntNum));
				break;
			case ET_MISSILE:
				if (comp->style & 2 && VectorDistance(cg.refdef_current->vieworg, es->origin) < 512)
				{
					const char *weaponText;

					switch (es->weapon)
					{
					case WP_DYNAMITE:
						weaponText = "Dynamite";
						break;
					case WP_LANDMINE:
						weaponText = "Landmine";
						break;
					case WP_SATCHEL:
						weaponText = "Satchel Charge";
						break;
					default:
						weaponText = "Unknown weapon";
						break;
					}

					s = va(CG_TranslateString("%s^*\'s %s"), CG_GetCrosshairNameString(comp, es->otherEntityNum), weaponText);
				}
				break;
			default:
				break;
			}

			if (s && *s)
			{
				textColor[3] = color[3];
				CG_DrawCompText(comp, s, textColor, comp->styleText, &cgs.media.limboFont2);
			}
		}
		return;
	}

	// crosshair for disguised enemy
	if (cgs.clientinfo[cg.crosshairEntNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team)
	{
		if (!(cg_entities[cg.crosshairEntNum].currentState.powerups & (1 << PW_OPS_DISGUISED)) ||
		    cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
		{
			return;
		}

		if (BG_IsSkillAvailable(cgs.clientinfo[cg.snap->ps.clientNum].skill, SK_SIGNALS, SK_FIELDOPS_ENEMY_RECOGNITION) && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS)
		{
			color = CG_FadeColor_Ext(cg.crosshairEntTime, cg_drawCrosshairFade.integer, textColor[3]);

			if (!color)
			{
				return;
			}

			textColor[3] = color[3];

			s = CG_TranslateString("Disguised Enemy!");
			CG_DrawCompText(comp, s, textColor, comp->styleText, &cgs.media.limboFont2);
			return;
		}

		clientNum = cgs.clientinfo[cg.crosshairEntNum].disguiseClientNum;
	}
	// we only want to see players on our team
	else if (!(cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR && cgs.clientinfo[cg.crosshairEntNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team))
	{
		clientNum = cg.crosshairEntNum;
	}

	// draw the name of the player being looked at
	if (clientNum != -1)
	{
		textColor[3] = color[3];

		s = CG_GetCrosshairNameString(comp, clientNum);
		CG_DrawCompText(comp, s, textColor, comp->styleText, &cgs.media.limboFont2);
	}
}

/**
 * @brief
 * Draw a sprite for where playerstate looks showing if artillery can be called
 * in at the spot or not.
 */
void CG_DrawDebugArtillery(centity_t *cent)
{
	trace_t tr;
	vec3_t  viewOrigin, viewTarget, skyTarget;
	vec3_t  forward = { 0 };

	AngleVectors(cg.predictedPlayerState.viewangles, forward, NULL, NULL);

	VectorCopy(cg.predictedPlayerState.origin, viewOrigin);
	viewOrigin[2] += cg.predictedPlayerState.viewheight;

	VectorMA(viewOrigin, MAX_TRACE, forward, viewTarget);

	CG_Trace(&tr, viewOrigin, NULL, NULL, viewTarget, cent->currentState.number, MASK_SHOT);
	if (tr.surfaceFlags & SURF_NOIMPACT)
	{
		return;
	}

	VectorCopy(tr.endpos, viewTarget);
	VectorCopy(tr.endpos, skyTarget);

	skyTarget[2] = BG_GetSkyHeightAtPoint(viewTarget);

	CG_Trace(&tr, tr.endpos, NULL, NULL, skyTarget, cent->currentState.number, MASK_SHOT);
	if (tr.fraction < 1.0f && !(tr.surfaceFlags & SURF_NOIMPACT)) // was not SURF_SKY
	{
		CG_DrawSprite(tr.endpos, 6.66f, cgs.media.escortShader, (byte[]) { 255, 0, 0, 255 });
		CG_DrawSprite(viewTarget, 6.66f, cgs.media.escortShader, (byte[]) { 255, 0, 0, 255 });
	}
	else
	{
		CG_DrawSprite(viewTarget, 6.66f, cgs.media.escortShader, NULL);
	}
}

//==============================================================================

/**
 * @brief CG_DrawSpectator
 */
void CG_DrawSpectator(hudComponent_t *comp)
{
	const char *s;

#ifdef FEATURE_EDV
	if (cgs.demoCamera.renderingWeaponCam)
	{
		s = CG_TranslateString("WEAPONCAM");
	}
	else if (cgs.demoCamera.renderingFreeCam)
	{
		s = CG_TranslateString("FREECAM");
	}
	else
#endif
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.generatingNoiseHud)
	{
		s = CG_TranslateString(va("%s", "SPECTATOR"));
	}
	else
	{
		return;
	}

	CG_DrawCompText(comp, s, comp->colorMain, comp->styleText, &cgs.media.limboFont2);
}

/**
 * @brief CG_GetBindingKeyForVote
 * @param[in] str1
 * @param[in] str2
 */
static void CG_GetBindingKeyForVote(char *str1, char *str2)
{
	Q_strncpyz(str1, Binding_FromName("vote yes"), 32);
	Q_strncpyz(str2, Binding_FromName("vote no"), 32);
}

/**
 * @brief CG_DrawVote
 */
void CG_DrawVote(hudComponent_t *comp)
{
	const char *str = NULL;
	char       str1[32], str2[32];

	if (cgs.complaintEndTime > cg.time && !cg.demoPlayback && (comp->style & 1) && cgs.complaintClient >= 0)
	{
		CG_GetBindingKeyForVote(str1, str2);

		str = va("%s\n%s",
		         va(CG_TranslateString("File complaint against ^7%s^* for %s?"),
		            cgs.clientinfo[cgs.complaintClient].name,
		            (cgs.complaintClient == CG_LastAttacker()) ? "team-killing" : "team-bleeding"),
		         va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2));

		CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
		return;
	}

	if (cgs.applicationEndTime > cg.time && cgs.applicationClient >= 0)
	{
		CG_GetBindingKeyForVote(str1, str2);

		str = va("%s\n%s",
		         va(CG_TranslateString("Accept ^7%s^*'s application to join your fireteam?"), cgs.clientinfo[cgs.applicationClient].name),
		         va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2));

		CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
		return;
	}

	if (cgs.propositionEndTime > cg.time && cgs.propositionClient >= 0)
	{
		CG_GetBindingKeyForVote(str1, str2);

		str = va("%s\n%s",
		         va(CG_TranslateString("Accept ^7%s^*'s proposition to invite ^7%s^* to join your fireteam?"), cgs.clientinfo[cgs.propositionClient2].name, cgs.clientinfo[cgs.propositionClient].name),
		         va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2));

		CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
		return;
	}

	if (cgs.invitationEndTime > cg.time && cgs.invitationClient >= 0)
	{
		CG_GetBindingKeyForVote(str1, str2);

		str = va("%s\n%s",
		         va(CG_TranslateString("Accept ^7%s^*'s invitation to join their fireteam?"), cgs.clientinfo[cgs.invitationClient].name),
		         va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2));

		CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);

		return;
	}

	if (cgs.autoFireteamEndTime > cg.time && cgs.autoFireteamNum == -1)
	{
		CG_GetBindingKeyForVote(str1, str2);

		str = va("%s\n%s",
		         CG_TranslateString("Make Fireteam private?"),
		         va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2));

		CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);

		return;
	}

	if (cgs.autoFireteamCreateEndTime > cg.time && cgs.autoFireteamCreateNum == -1)
	{
		CG_GetBindingKeyForVote(str1, str2);

		str = va("%s\n%s",
		         CG_TranslateString("Create a Fireteam?"),
		         va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2));

		CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
		return;
	}

	if (cgs.autoFireteamJoinEndTime > cg.time && cgs.autoFireteamJoinNum == -1)
	{
		CG_GetBindingKeyForVote(str1, str2);

		str = va("%s\n%s",
		         CG_TranslateString("Join a Fireteam?"),
		         va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2));

		CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
		return;
	}

	if (cgs.voteTime)
	{
		int sec;

		CG_GetBindingKeyForVote(str1, str2);

		// play a talk beep whenever it is modified
		if (cgs.voteModified)
		{
			cgs.voteModified = qfalse;
		}

		sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
		if (sec < 0)
		{
			// expired votetime
			cgs.voteTime                  = 0;
			cgs.complaintEndTime          = 0;
			cgs.applicationEndTime        = 0;
			cgs.propositionEndTime        = 0;
			cgs.invitationEndTime         = 0;
			cgs.autoFireteamEndTime       = 0;
			cgs.autoFireteamCreateEndTime = 0;
			cgs.autoFireteamJoinEndTime   = 0;
			//sec = 0;
			return;
		}

		if (!Q_stricmpn(cgs.voteString, "kick", 4))
		{
			if (strlen(cgs.voteString) > 5)
			{
				int  nameindex;
				char buffer[128];

				Q_strncpyz(buffer, cgs.voteString + 5, sizeof(buffer));
				Q_CleanStr(buffer);

				for (nameindex = 0; nameindex < MAX_CLIENTS; nameindex++)
				{
					if (!cgs.clientinfo[nameindex].infoValid)
					{
						continue;
					}

					if (!Q_stricmp(cgs.clientinfo[nameindex].cleanname, buffer))
					{
						if (cgs.clientinfo[nameindex].team != TEAM_SPECTATOR && cgs.clientinfo[nameindex].team != cgs.clientinfo[cg.clientNum].team)
						{
							return;
						}
					}
				}
			}
		}

		if (!(cg.snap->ps.eFlags & EF_VOTED))
		{
			if (cgs.clientinfo[cg.clientNum].team != TEAM_AXIS && cgs.clientinfo[cg.clientNum].team != TEAM_ALLIES)
			{
				str = va("%s\n^*%s\n%s", va(CG_TranslateString("VOTE(%i): %s"), sec, cgs.voteString),
				         va(CG_TranslateString("YES:%i, NO:%i"), cgs.voteYes, cgs.voteNo),
				         va(CG_TranslateString("Can't vote as %s"), cgs.clientinfo[cg.clientNum].shoutcaster ? "Shoutcaster" : "Spectator"));

				CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
			}
			else
			{
				str = va("%s\n^*%s",
				         va(CG_TranslateString("VOTE(%i): %s"), sec, cgs.voteString),
				         va(CG_TranslateString("YES(%s):%i, NO(%s):%i"), str1, cgs.voteYes, str2, cgs.voteNo));

				CG_DrawCompMultilineText(comp, str,
				                         comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
			}
		}
		else
		{
			str = va("%s\n^*%s",
			         va(CG_TranslateString("YOU VOTED ON: %s"), cgs.voteString),
			         va(CG_TranslateString("Y:%i, N:%i"), cgs.voteYes, cgs.voteNo));

			CG_DrawCompMultilineText(comp, str,
			                         comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
		}

		return;
	}

	if (cgs.complaintEndTime > cg.time && !cg.demoPlayback && (comp->style & 1) && cgs.complaintClient < 0)
	{
		switch (cgs.complaintClient)
		{
		case -1: str = CG_TranslateString("Your complaint has been filed"); break;
		case -2: str = CG_TranslateString("Complaint dismissed"); break;
		case -3: str = CG_TranslateString("Server Host cannot be complained against"); break;
		case -4: str = CG_TranslateString(va("You were %s by the Server Host", (cgs.complaintClient == CG_LastAttacker()) ? "team-killed" : "team-bled")); break;
		case -5: str = CG_TranslateString(va("You were %s by a bot.", (cgs.complaintClient == CG_LastAttacker()) ? "team-killed" : "team-bled")); break;
		default: break;

		}

		if (str)
		{
			CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
			return;
		}
	}

	if (cgs.applicationEndTime > cg.time && cgs.applicationClient < 0)
	{
		switch (cgs.applicationClient)
		{
		case -1: str = CG_TranslateString("Your application has been submitted"); break;
		case -2: str = CG_TranslateString("Your application failed"); break;
		case -3: str = CG_TranslateString("Your application has been approved"); break;
		case -4: str = CG_TranslateString("Your application reply has been sent"); break;
		default: break;
		}

		if (str)
		{
			CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
			return;
		}
	}

	if (cgs.propositionEndTime > cg.time && cgs.propositionClient < 0)
	{
		switch (cgs.propositionClient)
		{
		case -1: str = CG_TranslateString("Your proposition has been submitted"); break;
		case -2: str = CG_TranslateString("Your proposition was rejected"); break;
		case -3: str = CG_TranslateString("Your proposition was accepted"); break;
		case -4: str = CG_TranslateString("Your proposition reply has been sent"); break;
		default: break;
		}

		if (str)
		{
			CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
			return;
		}
	}

	if (cgs.invitationEndTime > cg.time && cgs.invitationClient < 0)
	{
		switch (cgs.invitationClient)
		{
		case -1: str = CG_TranslateString("Your invitation has been submitted"); break;
		case -2: str = CG_TranslateString("Your invitation was rejected"); break;
		case -3: str = CG_TranslateString("Your invitation was accepted"); break;
		case -4: str = CG_TranslateString("Your invitation reply has been sent"); break;
		default: break;
		}

		if (str)
		{
			CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
			return;
		}

		if (cgs.invitationClient < 0)
		{
			return;
		}
	}

	if ((cgs.autoFireteamEndTime > cg.time && cgs.autoFireteamNum == -2) || (cgs.autoFireteamCreateEndTime > cg.time && cgs.autoFireteamCreateNum == -2) || (cgs.autoFireteamJoinEndTime > cg.time && cgs.autoFireteamJoinNum == -2))
	{
		CG_DrawCompMultilineText(comp, CG_TranslateString("Response Sent"), comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
		return;
	}
}

/**
 * @brief CG_DrawIntermission
 */
static void CG_DrawIntermission(void)
{
	// End-of-level autoactions
	if (!cg.demoPlayback)
	{
		static int doScreenshot = 0, doDemostop = 0;

		if (!cg.latchAutoActions)
		{
			cg.latchAutoActions = qtrue;

			if (cg_autoAction.integer & AA_SCREENSHOT)
			{
				doScreenshot = cg.time + 1000;
			}

			if (cg_autoAction.integer & AA_STATSDUMP)
			{
				CG_dumpStats_f();
			}

			if ((cg_autoAction.integer & AA_DEMORECORD) &&
			    ((cgs.gametype == GT_WOLF_STOPWATCH && cgs.currentRound == 0) ||
			     cgs.gametype != GT_WOLF_STOPWATCH))
			{
				doDemostop = cg.time + 5000;    // stats should show up within 5 seconds
			}
		}

		if (doScreenshot > 0 && doScreenshot < cg.time)
		{
			CG_autoScreenShot_f();
			doScreenshot = 0;
		}

		if (doDemostop > 0 && doDemostop < cg.time)
		{
			trap_SendConsoleCommand("stoprecord\n");
			doDemostop = 0;
		}
	}

	// Intermission view
	CG_Debriefing_Draw();
}

/**
 * @brief CG_DrawSpectatorMessage
 */
void CG_DrawSpectatorMessage(hudComponent_t *comp)
{
	const char *str, *strMV = NULL;
	char       strKey[32];
	static int lastconfigGet = 0;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

#ifdef FEATURE_EDV
	if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
	{
		return;
	}
#endif

	if (!((cg.snap->ps.pm_flags & PMF_LIMBO) || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) && !cg.generatingNoiseHud)
	{
		return;
	}

	if (cg.time - lastconfigGet > 1000)
	{
		Controls_GetConfig();

		lastconfigGet = cg.time;
	}

	Q_strncpyz(strKey, Binding_FromName("openlimbomenu"), 32);

#ifdef FEATURE_MULTIVIEW
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR && cgs.mvAllowed)
	{
		strMV = va("\n%s",
		           va(CG_TranslateString("Press %s to %s multiview mode"),
		              Binding_FromName("mvactivate"), ((cg.mvTotalClients > 0) ? CG_TranslateString("disable") : CG_TranslateString("activate"))));
	}
#endif

	// we are looking at a client
	if (!cg.crosshairNotLookingAtClient && cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		str = va("%s\n%s\n%s"
#ifdef FEATURE_MULTIVIEW
		         "%s"
#endif
		         ,
		         va(CG_TranslateString("Press %s to open Limbo Menu"), !Q_stricmp(strKey, "(openlimbomenu)") ? "ESCAPE" : strKey),
		         va(CG_TranslateString("Press %s to follow %s"), Binding_FromName("+attack"), cgs.clientinfo[cg.crosshairEntNum].name),
		         va(CG_TranslateString("^*Press %s to follow previous player"), Binding_FromName("weapalt"))
#ifdef FEATURE_MULTIVIEW
		         , strMV ? strMV : ""
#endif
		         );
	}
	else
	{
		str = va("%s\n%s\n%s"
#ifdef FEATURE_MULTIVIEW
		         "%s"
#endif
		         ,
		         va(CG_TranslateString("Press %s to open Limbo Menu"), !Q_stricmp(strKey, "(openlimbomenu)") ? "ESCAPE" : strKey),
		         va(CG_TranslateString("Press %s to follow next player"), Binding_FromName("+attack")),
		         va(CG_TranslateString("Press %s to follow previous player"), Binding_FromName("weapalt"))
#ifdef FEATURE_MULTIVIEW
		         , strMV ? strMV : ""
#endif
		         );
	}

	CG_DrawCompMultilineText(comp, str, comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
}

/**
 * @brief CG_ReinfTimeEx
 * @param[in] team
 * @return
 */
int CG_CalculateReinfTime(team_t team)
{
	int dwDeployTime;

	dwDeployTime = (team == TEAM_AXIS) ? cg_redlimbotime.integer : cg_bluelimbotime.integer;
	return (int)(1 + (dwDeployTime - ((cgs.aReinfOffset[team] + cg.time - cgs.levelStartTime) % dwDeployTime)) * 0.001f);
}

/**
 * @brief CG_CalculateReinfTime
 * @param menu
 * @return
 */
int CG_GetReinfTime(qboolean menu)
{
	team_t team;

	if (menu)
	{
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
		{
			team = cgs.ccSelectedTeam == 0 ? TEAM_AXIS : TEAM_ALLIES;
		}
		else
		{
			team = cgs.clientinfo[cg.clientNum].team;
		}
	}
	else
	{
		team = cgs.clientinfo[cg.snap->ps.clientNum].team;
	}

	return CG_CalculateReinfTime(team);
}

/**
 * @brief CG_DrawLimboMessage
 */
void CG_DrawLimboMessage(hudComponent_t *comp)
{
	const char *str1, *str2 = NULL;

#ifdef FEATURE_EDV
	if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
	{
		return;
	}
#endif

	if (cg.snap->ps.stats[STAT_HEALTH] > 0 && !cg.generatingNoiseHud)
	{
		return;
	}

	if (((cg.snap->ps.pm_flags & PMF_LIMBO) || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) && !cg.generatingNoiseHud)
	{
		return;
	}

	if (!(comp->style & 1))
	{
		if (cgs.gametype == GT_WOLF_LMS)
		{
			CG_DrawCompMultilineText(comp, CG_TranslateString("You are wounded and waiting for a medic."), comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
			return;
		}

		str2 = va("\n%s\n%s",
		          CG_TranslateString("You are wounded and waiting for a medic."),
		          va(CG_TranslateString("Press %s to go into reinforcement queue."), Binding_FromName("+moveup")));
	}
	else if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] == 0)
	{
		str1 = CG_TranslateString("No more reinforcements this round.");
	}
	else
	{
		int reinfTime = CG_CalculateReinfTime(cgs.clientinfo[cg.clientNum].team);

		// coloured respawn counter when deployment is close to bring attention to next respawn
		if (reinfTime > 2)
		{
			str1 = va(CG_TranslateString("Deploying in ^3%d ^*seconds"), reinfTime);
		}
		else if (reinfTime > 1)
		{
			str1 = va(CG_TranslateString("Deploying in %s%d ^*seconds"), cgs.clientinfo[cg.clientNum].health == 0 ? "^1" : "^3", reinfTime);
		}
		else
		{
			str1 = va(CG_TranslateString("Deploying in %s%d ^*second"), cgs.clientinfo[cg.clientNum].health == 0 ? "^1" : "^3", reinfTime);
		}
	}

	CG_DrawCompMultilineText(comp, va("%s%s", str1, str2 ? str2 : ""), comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
}

/**
 * @brief CG_DrawFollow
 * @return
 */
void CG_DrawFollow(hudComponent_t *comp)
{
	float y = comp->location.y;
	float lineHeight;
	float charHeight;
	float heightTextOffset;
	float heightIconsOffset;
	float scale;
	float iconsSize;

#ifdef FEATURE_MULTIVIEW
	// MV following info for mainview
	if (CG_ViewingDraw())
	{
		return;
	}
#endif

#ifdef FEATURE_EDV
	if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
	{
		return;
	}
#endif

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (!(cg.snap->ps.pm_flags & PMF_FOLLOW) && !cg.generatingNoiseHud)
	{
		return;
	}

	scale = CG_ComputeScale(comp);

	lineHeight        = comp->location.h * 0.5f;
	charHeight        = CG_Text_Height_Ext("A", scale, 0, &cgs.media.limboFont2);
	iconsSize         = charHeight * 2.5f;
	heightTextOffset  = (lineHeight + charHeight) * 0.5f;
	heightIconsOffset = (lineHeight - iconsSize) * 0.5f;

	// Spectators view teamflags
	if (cg.snap->ps.clientNum != cg.clientNum && cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_ALLIES)
		{
			CG_DrawPic(comp->location.x + 1, y, lineHeight * 1.5f, lineHeight, cgs.media.alliedFlag);
		}
		else
		{
			CG_DrawPic(comp->location.x + 1, y, lineHeight * 1.5f, lineHeight, cgs.media.axisFlag);
		}

		CG_DrawRect_FixedBorder(comp->location.x, y - 1, lineHeight * 1.5f + 2, lineHeight + 2, 1, HUD_Border);

		y += lineHeight;
	}

	// if in limbo, show different follow message
	if (cg.snap->ps.pm_flags & PMF_LIMBO)
	{
		char deploytime[128] = { 0 };

		if (comp->showBackGround)
		{
			CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
		}

		if (comp->showBorder)
		{
			CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
		}

		if (cgs.gametype != GT_WOLF_LMS && !(comp->style & FOLLOW_NO_COUNTDOWN))
		{
			if (cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] == 0)
			{
				if (cg.snap->ps.persistant[PERS_RESPAWNS_PENALTY] >= 0)
				{
					int deployTime   = ((cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS) ? cg_redlimbotime.integer : cg_bluelimbotime.integer) / 1000;
					int reinfDepTime = CG_CalculateReinfTime(cgs.clientinfo[cg.snap->ps.clientNum].team) + cg.snap->ps.persistant[PERS_RESPAWNS_PENALTY] * deployTime;

					if (reinfDepTime > 1)
					{
						Com_sprintf(deploytime, sizeof(deploytime), CG_TranslateString("Bonus Life! Deploying in ^3%d ^*seconds"), reinfDepTime);
					}
					else
					{
						Com_sprintf(deploytime, sizeof(deploytime), CG_TranslateString("Bonus Life! Deploying in ^3%d ^*second"), reinfDepTime);
					}
				}
				else
				{
					Com_sprintf(deploytime, sizeof(deploytime), "%s", CG_TranslateString("No more deployments this round"));
				}
			}
			else
			{
				int reinfTime = CG_CalculateReinfTime(cgs.clientinfo[cg.snap->ps.clientNum].team);

				// coloured respawn counter when deployment is close to bring attention to next respawn
				if (reinfTime > 1)
				{
					Com_sprintf(deploytime, sizeof(deploytime), CG_TranslateString("Deploying in ^3%d ^*seconds"), reinfTime);
				}
				else
				{
					Com_sprintf(deploytime, sizeof(deploytime), CG_TranslateString("Deploying in ^3%d ^*second"), reinfTime);
				}
			}

			CG_Text_Paint_Ext(comp->location.x, y + heightTextOffset, scale, scale, comp->colorMain, deploytime, 0, 0, comp->styleText, &cgs.media.limboFont2);
			y += lineHeight;
		}

		// Don't display if you're following yourself
		if (cg.snap->ps.clientNum != cg.clientNum)
		{
			const char *follow    = CG_TranslateString("Following");
			char       *w         = cgs.clientinfo[cg.snap->ps.clientNum].name;
			int        charWidth  = CG_Text_Width_Ext("A", scale, 0, &cgs.media.limboFont2);
			int        startClass = CG_Text_Width_Ext(va("(%s", follow), scale, 0, &cgs.media.limboFont2) + charWidth;
			int        startRank  = CG_Text_Width_Ext(w, scale, 0, &cgs.media.limboFont2) + lineHeight + 2 + 2 * charWidth;
			int        endRank;

			CG_DrawPic(comp->location.x + startClass, y + heightIconsOffset, iconsSize, iconsSize, cgs.media.skillPics[SkillNumForClass(cgs.clientinfo[cg.snap->ps.clientNum].cls)]);

			if (cgs.clientinfo[cg.snap->ps.clientNum].rank > 0)
			{
				CG_DrawPic(comp->location.x + startClass + startRank, y + heightIconsOffset, iconsSize, iconsSize, rankicons[cgs.clientinfo[cg.snap->ps.clientNum].rank][cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
				endRank = lineHeight + 2;
			}
			else
			{
				endRank = -charWidth;
			}

			CG_Text_Paint_Ext(comp->location.x, y + heightTextOffset, scale, scale, comp->colorMain, va("(%s", follow), 0, 0, comp->styleText, &cgs.media.limboFont2);
			CG_Text_Paint_Ext(comp->location.x + startClass + lineHeight + 2 + charWidth, y + heightTextOffset, scale, scale, colorWhite, w, 0, 0, comp->styleText, &cgs.media.limboFont2);
			CG_Text_Paint_Ext(comp->location.x + startClass + startRank + endRank, y + heightTextOffset, scale, scale, colorWhite, ")", 0, 0, comp->styleText, &cgs.media.limboFont2);
		}
	}
	else
	{
		const char *follow    = CG_TranslateString("Following");
		char       *w         = cgs.clientinfo[cg.snap->ps.clientNum].name;
		int        charWidth  = CG_Text_Width_Ext("A", scale, 0, &cgs.media.limboFont2);
		int        startClass = CG_Text_Width_Ext(follow, scale, 0, &cgs.media.limboFont2) + charWidth;

		CG_DrawPic(comp->location.x + startClass, y + heightIconsOffset, iconsSize, iconsSize, cgs.media.skillPics[SkillNumForClass(cgs.clientinfo[cg.snap->ps.clientNum].cls)]);

		if (cgs.clientinfo[cg.snap->ps.clientNum].rank > 0)
		{
			int startRank;

			startRank = CG_Text_Width_Ext(w, scale, 0, &cgs.media.limboFont2) + iconsSize + charWidth;
			CG_DrawPic(comp->location.x + startClass + startRank, y + heightIconsOffset, iconsSize, iconsSize, rankicons[cgs.clientinfo[cg.snap->ps.clientNum].rank][cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
		}

		CG_Text_Paint_Ext(comp->location.x, y + heightTextOffset, scale, scale, comp->colorMain, follow, 0, 0, comp->styleText, &cgs.media.limboFont2);
		CG_Text_Paint_Ext(comp->location.x + startClass + iconsSize + charWidth, y + heightTextOffset, scale, scale, colorWhite, w, 0, 0, comp->styleText, &cgs.media.limboFont2);
	}
}

/**
 * @brief CG_DrawWarmupTitle
 * @param[in] comp
 */
void CG_DrawWarmupTitle(hudComponent_t *comp)
{
	const char *s;

	if (cg.serverRespawning)
	{
		// print message informing player the server is restarting with a new map
		s = va("%s", CG_TranslateString("^3Server Restarting"));
	}
	else if (!cg.warmup || cg.generatingNoiseHud)
	{
		if (!(cgs.gamestate == GS_WARMUP && !cg.warmup) && cgs.gamestate != GS_WAITING_FOR_PLAYERS && !cg.generatingNoiseHud)
		{
			return;
		}

		if (cgs.minclients > 0)
		{
			s = va(CG_TranslateString("^3WARMUP:^* Waiting on ^2%i^* %s"), cgs.minclients, cgs.minclients == 1 ? CG_TranslateString("player") : CG_TranslateString("players"));
		}
		else
		{
			s = va("%s", CG_TranslateString("^3WARMUP:^* All players ready!"));
		}
	}
	else
	{
		int sec = (cg.warmup - cg.time) / 1000;

		if (sec <= 0)
		{
			s = CG_TranslateString("^3WARMUP:^* Match begins now!"); // " Timelimit at start: ^3%i min", cgs.timelimit
		}
		else
		{
			s = va("%s %s%i", CG_TranslateString("^3WARMUP:^* Match begins in"), sec  < 4 ? "^1" : "^2", sec);
		}
	}

	CG_DrawCompText(comp, s, comp->colorMain, comp->styleText, &cgs.media.limboFont2);
}

/**
 * @brief CG_DrawWarmupText
 * @param[in] comp
 */
void CG_DrawWarmupText(hudComponent_t *comp)
{
	const char *s = NULL, *s1 = NULL, *s2 = NULL;

	if (!cg.warmup || cg.generatingNoiseHud)
	{
		if ((!(cgs.gamestate == GS_WARMUP && !cg.warmup) && cgs.gamestate != GS_WAITING_FOR_PLAYERS) && !cg.generatingNoiseHud)
		{
			return;
		}

		if (CG_ConfigString(CS_CONFIGNAME)[0])
		{
			s = va(CG_TranslateString("Config: ^7%s^*"), CG_ConfigString(CS_CONFIGNAME));
		}

		if (!cg.demoPlayback && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
		    (!(cg.snap->ps.pm_flags & PMF_FOLLOW) || (cg.snap->ps.pm_flags & PMF_LIMBO)))
		{
			char str1[32];

			if (cg.snap->ps.eFlags & EF_READY)
			{
				s1 = CG_TranslateString("^2Ready");

				Q_strncpyz(str1, Binding_FromName("notready"), 32);
				if (!Q_stricmp(str1, "(?" "?" "?)"))
				{
					s2 = CG_TranslateString("^*Type ^3\\notready^* in the console to unready");
				}
				else
				{
					s2 = va(CG_TranslateString("^*Press ^3%s^* to unready"), str1);
					s2 = CG_TranslateString(s2);
				}
			}
			else
			{
				Q_strncpyz(str1, Binding_FromName("ready"), 32);
				if (!Q_stricmp(str1, "(?" "?" "?)"))
				{
					s2 = CG_TranslateString("^*Type ^3\\ready^* in the console to start");
				}
				else
				{
					s2 = va(CG_TranslateString("^*Press ^3%s^* to start"), str1);
					s2 = CG_TranslateString(s2);
				}
			}
		}

		CG_DrawCompMultilineText(comp, va("%s\n%s\n%s\n", s ? s : " ", s1 ? s1 : " ", s2 ? s2 : " "),
		                         comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
	}
	else if (cgs.gametype == GT_WOLF_STOPWATCH)
	{
		const char *cs;
		int        defender;

		s = va("%s %i", CG_TranslateString("STOPWATCH ROUND"), cgs.currentRound + 1);

		cs       = CG_ConfigString(CS_MULTI_INFO);
		defender = Q_atoi(Info_ValueForKey(cs, "d")); // defender

		if (!defender)
		{
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_AXIS)
			{
				if (cgs.currentRound == 1)
				{
					s1 = "You have been switched to the Axis team";
					s2 = "Keep the Allies from beating the clock!";
				}
				else
				{
					s1 = "You are on the Axis team";
				}
			}
			else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES)
			{
				if (cgs.currentRound == 1)
				{
					s1 = "You have been switched to the Allied team";
					s2 = "Try to beat the clock!";
				}
				else
				{
					s1 = "You are on the Allied team";
				}
			}
		}
		else
		{
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_AXIS)
			{
				if (cgs.currentRound == 1)
				{
					s1 = "You have been switched to the Axis team";
					s2 = "Try to beat the clock!";
				}
				else
				{
					s1 = "You are on the Axis team";
				}
			}
			else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES)
			{
				if (cgs.currentRound == 1)
				{
					s1 = "You have been switched to the Allied team";
					s2 = "Keep the Axis from beating the clock!";
				}
				else
				{
					s1 = "You are on the Allied team";
				}
			}
		}

		CG_DrawCompMultilineText(comp, va("%s\n%s\n%s\n", s ? s : " ", s1 ? CG_TranslateString(s1) : " ", s2 ? CG_TranslateString(s2) : " "),
		                         comp->colorMain, comp->alignText, comp->styleText, &cgs.media.limboFont2);
	}
}

//==================================================================================

/**
 * @brief CG_DrawFlashFade
 */
static void CG_DrawFlashFade(void)
{
	static int lastTime;
	qboolean   fBlackout = (int_ui_blackout.integer > 0);

	if (cgs.fadeStartTime + cgs.fadeDuration < cg.time)
	{
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	}
	else if (cgs.fadeAlphaCurrent != cgs.fadeAlpha)
	{
		int time;
		int elapsed = (time = trap_Milliseconds()) - lastTime;    // we need to use trap_Milliseconds() here since the cg.time gets modified upon reloading

		lastTime = time;
		if (elapsed < 500 && elapsed > 0)
		{
			if (cgs.fadeAlphaCurrent > cgs.fadeAlpha)
			{
				cgs.fadeAlphaCurrent -= ((float)elapsed / (float)cgs.fadeDuration);
				if (cgs.fadeAlphaCurrent < cgs.fadeAlpha)
				{
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
				}
			}
			else
			{
				cgs.fadeAlphaCurrent += ((float)elapsed / (float)cgs.fadeDuration);
				if (cgs.fadeAlphaCurrent > cgs.fadeAlpha)
				{
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
				}
			}
		}
	}

	// have to inform the ui that we need to remain blacked out (or not)
	if (int_ui_blackout.integer == 0)
	{
		if (
#ifdef FEATURE_MULTIVIEW
			cg.mvTotalClients < 1 &&
#endif
			cg.snap->ps.powerups[PW_BLACKOUT] > 0)
		{
			trap_Cvar_Set("ui_blackout", va("%d", cg.snap->ps.powerups[PW_BLACKOUT]));
		}
	}
	else if (cg.snap->ps.powerups[PW_BLACKOUT] == 0
#ifdef FEATURE_MULTIVIEW
	         || cg.mvTotalClients > 0
#endif
	         )
	{
		trap_Cvar_Set("ui_blackout", "0");
	}

	// now draw the fade
	if (cgs.fadeAlphaCurrent > 0.0f || fBlackout)
	{
		vec4_t col;

		VectorClear(col);
		col[3] = (fBlackout) ? 1.0f : cgs.fadeAlphaCurrent;
		CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, col);

		// bail out if we're a speclocked spectator with cg_draw2d = 0
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR && !cg_draw2D.integer)
		{
			return;
		}

		// Show who is speclocked
		if (fBlackout)
		{
			int   i, nOffset = 90;
			char  *teams[TEAM_NUM_TEAMS] = { "??", "AXIS", "ALLIES", "???" }; // FIXME: translate team names? team name
			float color[4]               = { 1, 1, 0, 1 };

			for (i = TEAM_AXIS; i <= TEAM_ALLIES; i++)
			{
				if (cg.snap->ps.powerups[PW_BLACKOUT] & i)
				{
					int  w, x;
					char *s1;

					s1 = va(CG_TranslateString("The %s team is speclocked!"), teams[i]);
					w  = CG_Text_Width_Ext(s1, cg_fontScaleSP.value, 0, &cgs.media.limboFont2);
					x  = Ccg_WideX(320) - w / 2;

					CG_Text_Paint_Ext(x, nOffset, cg_fontScaleSP.value, cg_fontScaleSP.value, color, s1, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
					nOffset += 12;
				}
			}
		}
	}
}

/**
 * @brief Hide the snap transition from regular view to/from zoomed
 * @todo FIXME: TODO: use cg_fade?
 */
static void CG_DrawFlashZoomTransition(void)
{
	float frac;
	float fadeTime = 400.f;

	if (!cg.snap)
	{
		return;
	}

	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		// don't draw when on mg_42
		// keep the timer fresh so when you remove yourself from the mg42, it'll fade
		cg.zoomTime = cg.time;
		return;
	}

	if (cg.renderingThirdPerson)
	{
		return;
	}

	frac = cg.time - cg.zoomTime;

	if (frac < fadeTime)
	{
		vec4_t color;

		frac = frac / fadeTime;
		Vector4Set(color, 0, 0, 0, 1.0f - frac);
		CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, color);
	}
}

/**
 * @brief CG_DrawFlashDamage
 */
static void CG_DrawFlashDamage(void)
{
	if (!cg.snap)
	{
		return;
	}

	if (cg_bloodFlash.value <= 0.f || cg_bloodFlashTime.value <= 0.f)
	{
		return;
	}

	if (cg.v_dmg_time > cg.time)
	{
		vec4_t col = { 0.2f, 0.f, 0.f, 0.f };
		float  width;

		width = Ccg_WideX(SCREEN_WIDTH);

		col[3] = Q_fabs((cg.v_dmg_time - cg.time) / cg_bloodFlashTime.value) * Com_Clamp(0.f, 1.f, cg_bloodFlash.value);

		if (cg.v_dmg_angle == -1.f) // all borders
		{
			// horrible duplicate from differentes borders below
			GradientRound_Paint(width * 0.125f, -SCREEN_HEIGHT * 0.125f, width * 0.75f, SCREEN_HEIGHT * 0.25f, col);
			GradientRound_Paint(width - width * 0.33f, -SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
			GradientRound_Paint(width - width * 0.125f, 0, width * 0.25f, SCREEN_HEIGHT, col);
			GradientRound_Paint(width - width * 0.33f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
			GradientRound_Paint(width * 0.125f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.125f, width * 0.75f, SCREEN_HEIGHT * 0.25f, col);
			GradientRound_Paint(-width * 0.33f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
			GradientRound_Paint(-width * 0.125f, 0, width * 0.25f, SCREEN_HEIGHT, col);
			GradientRound_Paint(-width * 0.33f, -SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
		else if (cg.v_dmg_angle < 30 || cg.v_dmg_angle >= 330)  // top
		{
			GradientRound_Paint(width * 0.125f, -SCREEN_HEIGHT * 0.125f, width * 0.75f, SCREEN_HEIGHT * 0.25f, col);
		}
		else if (/*cg.v_dmg_angle >= 30 &&*/ cg.v_dmg_angle < 60)   // top right corner
		{
			GradientRound_Paint(width - width * 0.33f, -SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
		else if (cg.v_dmg_angle >= 60 && cg.v_dmg_angle < 120)  // right
		{
			GradientRound_Paint(width - width * 0.125f, 0, width * 0.25f, SCREEN_HEIGHT, col);
		}
		else if (cg.v_dmg_angle >= 120 && cg.v_dmg_angle < 150) // bottom right corner
		{
			GradientRound_Paint(width - width * 0.33f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
		else if (cg.v_dmg_angle >= 150 && cg.v_dmg_angle < 210) // bottom
		{
			GradientRound_Paint(width * 0.125f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.125f, width * 0.75f, SCREEN_HEIGHT * 0.25f, col);
		}
		else if (cg.v_dmg_angle >= 210 && cg.v_dmg_angle < 240) // bottom left corner
		{
			GradientRound_Paint(-width * 0.33f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
		else if (cg.v_dmg_angle >= 240 && cg.v_dmg_angle < 300) // left
		{
			GradientRound_Paint(-width * 0.125f, 0, width * 0.25f, SCREEN_HEIGHT, col);
		}
		else //if (cg.v_dmg_angle >= 300 && cg.v_dmg_angle < 330)   // top left corner
		{
			GradientRound_Paint(-width * 0.33f, -SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
	}
}

/**
 * @brief CG_DrawFlashFire
 */
static void CG_DrawFlashFire(void)
{
	float alpha;

	if (!cg.snap)
	{
		return;
	}

	if (cg.renderingThirdPerson)
	{
		return;
	}

	if (!cg.snap->ps.onFireStart)
	{
		cg.v_noFireTime = cg.time;
		return;
	}

	alpha = (float)((FIRE_FLASH_TIME - 1000) - (cg.time - cg.snap->ps.onFireStart)) / (FIRE_FLASH_TIME - 1000);
	if (alpha > 0)
	{
		vec4_t col = { 1, 1, 1, 1 };
		float  max, f;

		if (alpha >= 1.0f)
		{
			alpha = 1.0f;
		}

		// fade in?
		f = (float)(cg.time - cg.v_noFireTime) / FIRE_FLASH_FADEIN_TIME;
		if (f >= 0.0f && f < 1.0f)
		{
			alpha = f;
		}

		max = (float)(0.5 + 0.5 * sin((double)((cg.time / 10) % 1000) / 1000.0));
		if (alpha > max)
		{
			alpha = max;
		}
		col[0] = alpha;
		col[1] = alpha;
		col[2] = alpha;
		col[3] = alpha;
		trap_R_SetColor(col);
		CG_DrawPic(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, cgs.media.viewFlashFire[(cg.time / 50) % 16]);
		trap_R_SetColor(NULL);

		trap_S_AddLoopingSound(cg.snap->ps.origin, vec3_origin, cgs.media.flameSound, (int)(255.0f * alpha), 0);
	}
	else
	{
		cg.v_noFireTime = cg.time;
	}
}

/**
 * @brief Screen flash stuff drawn first (on top of world, behind HUD)
 */
static void CG_DrawFlashBlendBehindHUD(void)
{
	CG_DrawFlashZoomTransition();
	CG_DrawFlashFade();

	// no flash blends if in limbo or spectator, and in the limbo menu
	if (((cg.snap->ps.pm_flags & PMF_LIMBO) || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) && cg.showGameView)
	{
		return;
	}

	CG_DrawFlashFire();
	CG_DrawFlashDamage();
}

/**
 * @brief CG_ObjectivePrint
 * @param[in] str
 * @param[in] fontScale
 */
void CG_ObjectivePrint(const char *str)
{
	int   maxLineChars;
	float scale, w;

	scale = CG_ComputeScale(&CG_GetActiveHUD()->objectivetext /*CG_GetActiveHUD()->objectivetext.location.h, CG_GetActiveHUD()->objectivetext.scale, &cgs.media.limboFont2*/);
	w     = CG_GetActiveHUD()->objectivetext.location.w;

	maxLineChars = CG_GetMaxCharsPerLine(str, scale, &cgs.media.limboFont2, w);
	CG_WordWrapString(CG_TranslateString(str), maxLineChars, cg.oidPrint, sizeof(cg.oidPrint), NULL);
	cg.oidPrintTime = cg.time;
}

/**
 * @brief CG_DrawObjectiveInfo
 */
void CG_DrawObjectiveInfo(hudComponent_t *comp)
{
	float  *color;
	vec4_t textColor;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (!cg.oidPrintTime)
	{
		return;
	}

	Vector4Copy(comp->colorMain, textColor);
	color = CG_FadeColor_Ext(cg.oidPrintTime, 250, textColor[3]);

	if (!color)
	{
		cg.oidPrintTime = 0;
		return;
	}

	textColor[3] = color[3];

	CG_DrawCompMultilineText(comp, cg.oidPrint, textColor, comp->alignText, comp->styleText, &cgs.media.limboFont2);

}

//==================================================================================

/**
 * @brief CG_DrawTimedMenus
 */
static void CG_DrawTimedMenus(void)
{
	if (cg.voiceTime)
	{
		if (cg.time - cg.voiceTime > 2500)
		{
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

/**
 * @brief CG_Fade
 * @param r - unused
 * @param g - unused
 * @param b - unused
 * @param[in] a
 * @param[in] time
 * @param[in] duration
 * @todo cleanup ?
 */
void CG_Fade(int r, int g, int b, int a, int time, int duration)
{
	// incorporate this into the current fade scheme

	cgs.fadeAlpha     = (float)a / 255.0f;
	cgs.fadeStartTime = time;
	cgs.fadeDuration  = duration;

	if (cgs.fadeStartTime + cgs.fadeDuration <= cg.time)
	{
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	}
	return; // FIXME: early return?

	/*
	if (time <= 0)      // do instantly
	{
	    cg.fadeRate = 1;
	    cg.fadeTime = cg.time - 1;  // set cg.fadeTime behind cg.time so it will start out 'done'
	}
	else
	{
	    cg.fadeRate = 1.0f / time;
	    cg.fadeTime = cg.time + time;
	}

	cg.fadeColor2[0] = ( float )r / 255.0f;
	cg.fadeColor2[1] = ( float )g / 255.0f;
	cg.fadeColor2[2] = ( float )b / 255.0f;
	cg.fadeColor2[3] = ( float )a / 255.0f;
	*/
}

/**
 * @brief CG_ScreenFade
 */
static void CG_ScreenFade(void)
{
	int msec;

	if (cg.fadeRate == 0.f)
	{
		return;
	}

	msec = cg.fadeTime - cg.time;
	if (msec <= 0)
	{
		cg.fadeColor1[0] = cg.fadeColor2[0];
		cg.fadeColor1[1] = cg.fadeColor2[1];
		cg.fadeColor1[2] = cg.fadeColor2[2];
		cg.fadeColor1[3] = cg.fadeColor2[3];

		if (cg.fadeColor1[3] == 0.f)
		{
			cg.fadeRate = 0;
			return;
		}

		CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, cg.fadeColor1);

	}
	else
	{
		vec4_t color;
		float  t    = ( float )msec * cg.fadeRate;
		float  invt = 1.0f - t;
		int    i;

		for (i = 0; i < 4; i++)
		{
			color[i] = cg.fadeColor1[i] * t + cg.fadeColor2[i] * invt;
		}

		if (color[3] != 0.f)
		{
			CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, color);
		}
	}
}

/**
 * @brief CG_DrawOnScreenLabels
 */
void CG_DrawOnScreenLabels(void)
{
	static vec3_t mins  = { -1, -1, -1 };
	static vec3_t maxs  = { 1, 1, 1 };
	vec4_t        white = { 1.0f, 1.0f, 1.0f, 1.0f };
	int           i;
	specLabel_t   *specLabel;
	trace_t       tr;
	int           FadeOut = 0;
	int           FadeIn  = 0;

	for (i = 0; i < cg.specStringCount; ++i)
	{
		specLabel = &cg.specOnScreenLabels[i];

		// Visible checks if information is actually valid
		if (!specLabel || !specLabel->visible)
		{
			continue;
		}

		CG_Trace(&tr, cg.refdef.vieworg, mins, maxs, specLabel->origin, -1, CONTENTS_SOLID);

		if (tr.fraction < 1.0f)
		{
			specLabel->lastInvisibleTime = cg.time;
		}
		else
		{
			specLabel->lastVisibleTime = cg.time;
		}

		FadeOut = cg.time - specLabel->lastVisibleTime;
		FadeIn  = cg.time - specLabel->lastInvisibleTime;

		if (FadeIn)
		{
			white[3] = (FadeIn > 500) ? 1.0f : FadeIn / 500.0f;
			if (white[3] < specLabel->alpha)
			{
				white[3] = specLabel->alpha;
			}
		}
		if (FadeOut)
		{
			white[3] = (FadeOut > 500) ? 0.0f : 1.0f - FadeOut / 500.0f;
			if (white[3] > specLabel->alpha)
			{
				white[3] = specLabel->alpha;
			}
		}
		if (white[3] > 1.0f)
		{
			white[3] = 1.0f;
		}

		specLabel->alpha = white[3];
		if (specLabel->alpha <= 0.0f)
		{
			continue;                           // no alpha = nothing to draw..
		}

		// Force full alpha anyway since we do not want to fade in/out
		if (specLabel->noFade)
		{
			specLabel->alpha = white[3] = 1.0f;
		}

		CG_Text_Paint_Ext(specLabel->x, specLabel->y, specLabel->scale, specLabel->scale, white, specLabel->text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		// expect update next frame again
		specLabel->visible = qfalse;
	}

	cg.specStringCount = 0;
}

/**
* @brief CG_DrawOnScreenBars
*/
void CG_DrawOnScreenBars(void)
{
	static vec3_t mins  = { -1, -1, -1 };
	static vec3_t maxs  = { 1, 1, 1 };
	vec4_t        white = { 1.0f, 1.0f, 1.0f, 1.0f };
	int           i;
	specBar_t     *specBar;
	trace_t       tr;
	int           FadeOut = 0;
	int           FadeIn  = 0;

	for (i = 0; i < cg.specBarCount; ++i)
	{
		specBar = &cg.specOnScreenBar[i];

		// Visible checks if information is actually valid
		if (!specBar || !specBar->visible)
		{
			continue;
		}

		CG_Trace(&tr, cg.refdef.vieworg, mins, maxs, specBar->origin, -1, CONTENTS_SOLID);

		if (tr.fraction < 1.0f)
		{
			specBar->lastInvisibleTime = cg.time;
		}
		else
		{
			specBar->lastVisibleTime = cg.time;
		}

		FadeOut = cg.time - specBar->lastVisibleTime;
		FadeIn  = cg.time - specBar->lastInvisibleTime;

		if (FadeIn)
		{
			white[3] = (FadeIn > 500) ? 1.0f : FadeIn / 500.0f;
			if (white[3] < specBar->alpha)
			{
				white[3] = specBar->alpha;
			}
		}
		if (FadeOut)
		{
			white[3] = (FadeOut > 500) ? 0.0f : 1.0f - FadeOut / 500.0f;
			if (white[3] > specBar->alpha)
			{
				white[3] = specBar->alpha;
			}
		}
		if (white[3] > 1.0f)
		{
			white[3] = 1.0f;
		}

		specBar->alpha = white[3];
		if (specBar->alpha <= 0.0f)
		{
			continue;                           // no alpha = nothing to draw..
		}

		CG_FilledBar(specBar->x, specBar->y, specBar->w, specBar->h, specBar->colorStart, specBar->colorEnd,
		             specBar->colorBack, specBar->colorBack, specBar->fraction, 0.f, BAR_BG, -1);

		// expect update next frame again
		specBar->visible = qfalse;
	}

	cg.specBarCount = 0;
}

/**
 * @brief CG_DrawBannerPrint
 */
void CG_DrawBannerPrint(hudComponent_t *comp)
{
	float  *color;
	vec4_t textColor;

	if ((cg_bannerTime.integer <= 0) || !cg.bannerPrintTime)
	{
		return;
	}

	color = CG_FadeColor(cg.bannerPrintTime, cg_bannerTime.integer);

	if (!color)
	{
		cg.bannerPrintTime = 0;
		return;
	}

	VectorCopy(comp->colorMain, textColor);
	textColor[3] = color[3];

	CG_DrawCompMultilineText(comp, cg.bannerPrint, textColor, comp->alignText, comp->styleText, &cgs.media.limboFont2);
}

#define MAX_DISTANCE 2000.f

/**
 * @brief CG_DrawEnvironmentalAwareness
 */
static void CG_DrawEnvironmentalAwareness()
{
	snapshot_t *snap;
	int        i;

	if (!cg_drawEnvAwareness.integer)
	{
		return;
	}

	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport)
	{
		snap = cg.nextSnap;
	}
	else
	{
		snap = cg.snap;
	}

	if (snap->ps.pm_flags & PMF_LIMBO
#ifdef FEATURE_MULTIVIEW
	    || cg.mvTotalClients > 0
#endif
	    )
	{
		return;
	}

	if (snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	for (i = 0; i < snap->numEntities; ++i)
	{
		centity_t *cent = &cg_entities[snap->entities[i].number];
		//char      description[MAX_QPATH] = { 0 };
		qhandle_t icon;
		vec3_t    dir;
		float     len;

		// skip self
		if (cent->currentState.eType == ET_PLAYER && cent->currentState.clientNum == cg.clientNum)
		{
			continue;
		}

		VectorSubtract(cent->lerpOrigin, cg.refdef.vieworg, dir);

		len = VectorLength(dir);

		// check range before drawing anything
		if (len < CH_DIST || len > MAX_DISTANCE)
		{
			continue;
		}

		icon = CG_GetCompassIcon(&snap->entities[i], qfalse, qfalse, cg_drawEnvAwareness.integer & 4, cg_drawEnvAwareness.integer & 2, cg_drawEnvAwareness.integer & 1, qfalse, NULL /*description*/);

		if (icon)
		{
			float x, y;
			float xc, yc;
			float px, py;
			float z;
			char  *distance;
			float baseSize;

			px = (float)tan(DEG2RAD((double)cg.refdef.fov_x) * 0.5);
			py = (float)tan(DEG2RAD((double)cg.refdef.fov_y) * 0.5);

			xc = Ccg_WideX(SCREEN_WIDTH) * 0.5f;
			yc = SCREEN_HEIGHT * 0.5f;

			z = DotProduct(dir, cg.refdef.viewaxis[0]);

			px *= Q_fabs(z);
			py *= MAX(0.1f, z);

			x = xc - (DotProduct(dir, cg.refdef.viewaxis[1]) * xc) / px;
			y = yc - (DotProduct(dir, cg.refdef.viewaxis[2]) * yc) / py;

			// don't let the icons going outside the screen
			x = Com_Clamp(xc - xc * cg_drawEnvAwarenessScale.value,
			              MIN(xc + xc * cg_drawEnvAwarenessScale.value,
			                  Ccg_WideX(SCREEN_WIDTH) - cg_drawEnvAwarenessIconSize.integer),
			              x);

			y = Com_Clamp(yc - yc * cg_drawEnvAwarenessScale.value,
			              MIN(yc + yc * cg_drawEnvAwarenessScale.value,
			                  SCREEN_HEIGHT - (cg_drawEnvAwarenessIconSize.integer + 12)),
			              y);

			switch (cg_drawUnit.integer)
			{
			case 1:
				distance = va("%.0fm", len * UNIT_TO_METER);
				break;
			case 2:
				distance = va("%.0fft", len * UNIT_TO_FEET);
				break;
			case 0:
			default:
				distance = va("%.0f", len);
				break;
			}

			baseSize = cg_drawEnvAwarenessIconSize.integer * (1 - Com_Clamp(0, .75f, len / MAX_DISTANCE));

			//CG_Text_Paint_Centred_Ext(x + baseSize * 0.5f, y - baseSize + 8, 0.12f, 0.12f, colorWhite, description, 0, 0, 0, &cgs.media.limboFont2);
			CG_DrawPic(x, y, baseSize, baseSize, icon);
			CG_Text_Paint_Centred_Ext(x + baseSize * 0.5f, y + baseSize + 8, 0.12f, 0.12f, colorWhite, distance, 0, 0, 0, &cgs.media.limboFont2);
		}
	}
}

/**
 * @brief CG_Draw2D
 */
static void CG_Draw2D(void)
{
	CG_ScreenFade();

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		CG_DrawIntermission();
		return;
	}

	if (cgs.dbShowing)
	{
		CG_Debriefing_Shutdown();
	}

	if (cg.editingSpeakers)
	{
		CG_SpeakerEditorDraw();
		return;
	}

	if (cg.editingCameras)
	{
		CG_CameraEditorDraw();
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].shoutcaster
	    || cg.demoPlayback || cgs.sv_cheats || cg_drawSpawnpoints.integer)
	{
		CG_DrawOnScreenLabels();
		CG_DrawOnScreenBars();
	}

	// no longer cheat protected, we draw crosshair/reticle in non demoplayback
	if (cg_draw2D.integer == 0)
	{
		if (cg.demoPlayback)
		{
			return;
		}
		CG_DrawCrosshair(&CG_GetActiveHUD()->crosshair);
		CG_DrawFlashFade();
		return;
	}

#ifdef FEATURE_EDV
	if (!cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#endif
	{
		CG_DrawFlashBlendBehindHUD();
		CG_DrawEnvironmentalAwareness();

		if (cg_drawStatus.integer)
		{
			Menu_PaintAll();
			CG_DrawTimedMenus();
		}
	}

	// don't draw center string if scoreboard is up
	if (!CG_DrawScoreboard())
	{
		CG_SetHud();

#ifdef FEATURE_EDV
		if (!cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#endif
		{
			// scan the known entities to see if the crosshair is sighted on one
			CG_ScanForCrosshairEntity();
			CG_CheckForCursorHints();

			CG_DrawActiveHud();
		}
	}
	else if (cgs.eventHandling != CGAME_EVENT_NONE)
	{
		// draw cursor
		trap_R_SetColor(NULL);
		CG_DrawCursor(cgDC.cursorx - 14, cgDC.cursory - 14);
	}

	if (cg.showFireteamMenu)
	{
		CG_Fireteams_Draw();
	}

	if (cg.showSpawnpointsMenu)
	{
		CG_Spawnpoints_Draw();
	}

	// Info overlays
	CG_DrawOverlays();

#ifdef FEATURE_EDV
	if (!cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#endif
	{
		// window updates
		CG_windowDraw();
	}
}

/**
 * @brief CG_StartShakeCamera
 * @param[in] p
 */
void CG_StartShakeCamera(float p)
{
	cg.cameraShakeScale = p;

	cg.cameraShakeLength = 1000 * (p * p);
	cg.cameraShakeTime   = cg.time + cg.cameraShakeLength;
	cg.cameraShakePhase  = crandom() * M_PI; // start chain in random dir
}

/**
 * @brief CG_ShakeCamera
 */
void CG_ShakeCamera(void)
{
	static vec3_t mins = { -16.0f, -16.0f, -16.0f };
	static vec3_t maxs = { 16.0f, 16.0f, 16.0f };

	if (cg.time > cg.cameraShakeTime)
	{
		cg.cameraShakeScale = 0; // all pending explosions resolved, so reset shakescale
		return;
	}

	{
		double  x    = (cg.cameraShakeTime - cg.time) / cg.cameraShakeLength;
		float   valx = sin(M_PI * 8 * 13.0 + cg.cameraShakePhase) * x * 6 * cg.cameraShakeScale;
		float   valy = sin(M_PI * 17 * x + cg.cameraShakePhase) * x * 6 * cg.cameraShakeScale;
		float   valz = cos(M_PI * 7 * x + cg.cameraShakePhase) * x * 6 * cg.cameraShakeScale;
		vec3_t  vec;
		trace_t tr;

		VectorAdd(cg.refdef.vieworg, tv(valx, valy, valz), vec);
		CG_Trace(&tr, cg.refdef.vieworg, mins, maxs, vec, cg.predictedPlayerState.clientNum, MASK_SOLID);
		if (!(tr.allsolid || tr.startsolid))
		{
			VectorCopy(tr.endpos, cg.refdef.vieworg);
		}
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
	}
}

/**
 * @brief CG_DrawMiscGamemodels
 */
void CG_DrawMiscGamemodels(void)
{
	int         i, j;
	refEntity_t ent;

	Com_Memset(&ent, 0, sizeof(ent));

	ent.reType            = RT_MODEL;
	ent.nonNormalizedAxes = qtrue;

	// static gamemodels don't project shadows
	ent.renderfx = RF_NOSHADOW;

	for (i = 0; i < cg.numMiscGameModels; i++)
	{
		if (cgs.miscGameModels[i].radius != 0.f)
		{
			if (CG_CullPointAndRadius(cgs.miscGameModels[i].org, cgs.miscGameModels[i].radius))
			{
				continue;
			}
		}

		if (!trap_R_inPVS(cg.refdef_current->vieworg, cgs.miscGameModels[i].org))
		{
			continue;
		}

		VectorCopy(cgs.miscGameModels[i].org, ent.origin);
		VectorCopy(cgs.miscGameModels[i].org, ent.oldorigin);
		VectorCopy(cgs.miscGameModels[i].org, ent.lightingOrigin);

		for (j = 0; j < 3; j++)
		{
			VectorCopy(cgs.miscGameModels[i].axes[j], ent.axis[j]);
		}
		ent.hModel = cgs.miscGameModels[i].model;

		trap_R_AddRefEntityToScene(&ent);
	}
}

/**
 * @brief CG_AddLineToScene
 * @param[in] start
 * @param[in] end
 * @param[in] colour
 */
void CG_AddLineToScene(const vec3_t start, const vec3_t end, const vec4_t colour)
{
	refEntity_t re;

	Com_Memset(&re, 0, sizeof(re));
	re.reType       = RT_RAIL_CORE;
	re.customShader = cgs.media.railCoreShader;
	VectorCopy(start, re.origin);
	VectorCopy(end, re.oldorigin);
	re.shaderRGBA[0] = (byte)(colour[0] * 0xff);
	re.shaderRGBA[1] = (byte)(colour[1] * 0xff);
	re.shaderRGBA[2] = (byte)(colour[2] * 0xff);
	re.shaderRGBA[3] = (byte)(colour[3] * 0xff);

	trap_R_AddRefEntityToScene(&re);
}

void CG_DrawRotateGizmo(const vec3_t origin, float radius, int numSegments, int activeAxis)
{
	int    i, j;
	vec3_t vec;
	vec3_t prevOrigin;
	vec4_t colour;

	for (j = 0; j < 3; j++)
	{
		vec3_clear(prevOrigin);
		VectorClear(colour);
		colour[3] = 1.f;
		if (activeAxis >= 0)
		{
			if (activeAxis == j)
			{
				colour[j] = 1.f;
			}
			else
			{
				colour[j] = .3f;
			}
		}
		else
		{
			colour[j] = 1.f;
		}

		for (i = 0; i <= numSegments; i++)
		{
			float theta = 2.0f * M_PI * i / numSegments;
			float x     = radius * cosf(theta);
			float y     = radius * sinf(theta);

			switch (j)
			{
			default:
			case 0:
				vec3_set(vec, 0, x, y);
				break;
			case 1:
				vec3_set(vec, y, 0, x);
				break;
			case 2:
				vec3_set(vec, x, y, 0);
				break;
			}

			vec3_add(origin, vec, vec);

			if (i > 0)
			{
				CG_AddLineToScene(prevOrigin, vec, colour);
			}

			vec3_copy(vec, prevOrigin);
		}
	}
}

void CG_DrawMoveGizmo(const vec3_t origin, float radius, int activeAxis)
{
	int         j;
	vec3_t      vec;
	vec4_t      colour;
	refEntity_t re;

	for (j = 0; j < 3; j++)
	{
		VectorClear(colour);
		colour[3] = 1.f;
		if (activeAxis >= 0)
		{
			if (activeAxis == j)
			{
				colour[j] = 1.f;
			}
			else
			{
				colour[j] = .3f;
			}
		}
		else
		{
			colour[j] = 1.f;
		}
		VectorClear(vec);
		vec[j] = 1.f;
		VectorMA(origin, radius, vec, vec);
		CG_AddLineToScene(origin, vec, colour);

		Com_Memset(&re, 0, sizeof(re));
		re.reType = RT_SPRITE;
		VectorCopy(vec, re.origin);
		VectorCopy(vec, re.oldorigin);
		re.radius        = 3;
		re.customShader  = cgs.media.waterBubbleShader;
		re.shaderRGBA[0] = (byte)(colour[0] * 0xff);
		re.shaderRGBA[1] = (byte)(colour[1] * 0xff);
		re.shaderRGBA[2] = (byte)(colour[2] * 0xff);
		re.shaderRGBA[3] = (byte)(colour[3] * 0xff);
		trap_R_AddRefEntityToScene(&re);
	}
}

/**
 * @brief Draw a sprite at a target location.
 */
void CG_DrawSprite(const vec3_t origin, float radius, qhandle_t shader, byte color[4])
{
	refEntity_t ent;
	Com_Memset(&ent, 0, sizeof(ent));

	ent.reType = RT_SPRITE;
	if (shader != 0)
	{
		ent.customShader = shader;
	}
	else
	{
		ent.customShader = cgs.media.escortShader;
	}

	ent.radius    = radius;
	ent.renderfx  = 0;
	ent.origin[0] = origin[0];
	ent.origin[1] = origin[1];
	ent.origin[2] = origin[2];

	if (color != NULL && !(color[0] == 0 && color[1] == 0 && color[2] == 0 && color[3] == 0))
	{
		ent.shaderRGBA[0] = color[0];
		ent.shaderRGBA[1] = color[1];
		ent.shaderRGBA[2] = color[2];
		ent.shaderRGBA[3] = color[3];
	}
	else
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
	}

	trap_R_AddRefEntityToScene(&ent);
}

/**
 * @brief CG_Coronas
 */
void CG_Coronas(void)
{
	if (cg_coronas.integer == 0)
	{
		return;
	}

	{
		int      i;
		trace_t  tr;
		float    dist;
		vec3_t   dir;
		float    fov;
		qboolean visible, behind, toofar;

		for (i = 0 ; i < cg.numCoronas ; ++i)
		{
			if (!trap_R_inPVS(cg.refdef_current->vieworg, cgs.corona[i].org))
			{
				continue;
			}

			behind = qfalse; // 'init'
			toofar = qfalse; // 'init'

			VectorSubtract(cg.refdef_current->vieworg, cgs.corona[i].org, dir);
			dist = VectorNormalize2(dir, dir);

			if (dist > cg_coronafardist.integer)
			{
				toofar = qtrue;
			}
			// dot = DotProduct(dir, cg.refdef_current->viewaxis[0]);
			fov = cosf((cg.refdef_current->fov_x / 2) * (float)(M_PI / 180));
			if (DotProduct(dir, cg.refdef_current->viewaxis[0]) >= -fov)
			{
				behind = qtrue;
			}

			if (cg_coronas.integer == 2)
			{   // if set to '2' trace everything
				behind = qfalse;
				toofar = qfalse;
			}

			if (!behind && !toofar)
			{
				CG_Trace(&tr, cg.refdef_current->vieworg, NULL, NULL, cgs.corona[i].org, -1, MASK_SOLID | CONTENTS_BODY);

				visible = qfalse; // 'init'
				if (tr.fraction == 1.f)
				{
					visible = qtrue;
				}
				trap_R_AddCoronaToScene(cgs.corona[i].org, cgs.corona[i].color[0], cgs.corona[i].color[1], cgs.corona[i].color[2], cgs.corona[i].scale, i, visible);
			}
		}
	}
}

/**
 * @brief Generate pseudo-random data to feed HUD elements
 */
static void CG_NoiseGenerator()
{
	if (!cg.generatingNoiseHud)
	{
		return;
	}

	trap_Cvar_Set("cl_noprint", "1");

	// banner
	CG_AddToBannerPrint("Iaculatores coniunctis: incesserit servitium castrensi post velut et deinde virgae.");

	// center print
	CG_CenterPrint("Insulari sufficiente postulatus aut nullo delatus hostiles iniecto aut suos.");

	// objevive print
	CG_ObjectivePrint("You are near an amazing place and everyone envy you.");

	// chat
	CG_AddToTeamChat("Lorem ipsum dolor sit amet, consectetuer adipiscing elit.", cg.snap->ps.clientNum);
	CG_AddToTeamChat("Maecenas porttitor congue massa.", cg.snap->ps.clientNum);
	CG_AddToTeamChat("Fusce posuere, magna sed pulvinar ultricies, purus lectus malesuada libero, sit amet commodo magna eros quis urna.", cg.snap->ps.clientNum);
	CG_AddToTeamChat("Nunc viverra imperdiet enim", cg.snap->ps.clientNum);

	// big popup messages kill
	CG_AddPMItemBig(PM_DEBUG, va(CG_TranslateString("Promoted to rank %s!"), GetRankTableData(cgs.clientinfo[cg.clientNum].team, cgs.clientinfo[cg.clientNum].rank)->names), rankicons[cgs.clientinfo[cg.clientNum].rank][cgs.clientinfo[cg.clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);

	// small popup message
	{
		static int noiseTime = 0;
		static int noiseTick = 0;

		noiseTime += cg.frametime;
		while (noiseTime > 100)
		{
			noiseTick++;
			if (noiseTick == 1)
			{
				CG_AddPMItem(PM_MESSAGE, "Neque atque quid praeter sed.", " ", cgs.media.pmImages[PM_MESSAGE], 0, 0, colorWhite);
			}
			else if (noiseTick == 2)
			{
				CG_AddPMItem(PM_DEATH, "Sed ut tum ad senem senex de senectute", " ", cgs.media.pmImages[PM_DEATH], 0, 0, colorWhite);
			}
			else if (noiseTick == 3)
			{
				CG_AddPMItem(PM_OBJECTIVE, "Nunc vero inanes flatus quorundam vile esse", " ", cgs.media.pmImages[PM_TEAM], 0, 0, colorWhite);
			}
			else if (noiseTick > 4)
			{
				noiseTick = 0;
			}
			noiseTime -= 100;
		}
	}

	// objective indicator simulation
	cg.flagIndicator |= (1 << PW_NUM_POWERUPS);

	// vote
	cgs.voteTime = cg.time - VOTE_TIME + 1;
	Q_strncpyz(cgs.voteString, "Do you want cast a vote ?", sizeof(cgs.voteString));

	// missile camera
	cg.latestMissile = &cg_entities[cg.snap->ps.clientNum];

	trap_Cvar_Set("cl_noprint", "0");
}

/**
 * @brief Perform all drawing needed to completely fill the screen
 */
void CG_DrawActive()
{
	// optionally draw the info screen instead
	if (!cg.snap)
	{
		CG_DrawInformation(qfalse);
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	cg.refdef_current->glfog.registered = qfalse;    // make sure it doesn't use fog from another scene

	CG_ShakeCamera();
	CG_PB_RenderPolyBuffers();
	CG_DrawMiscGamemodels();
	CG_Coronas();

	if (!(cg.limboEndCinematicTime > cg.time && cg.showGameView))
	{
		trap_R_RenderScene(cg.refdef_current);
	}

	if (!cg.showGameView)
	{
		// draw status bar and other floating elements
		CG_Draw2D();
	}
	else
	{
		CG_LimboPanel_Draw();
	}

	if (cg.editingHud)
	{
		CG_NoiseGenerator();
		CG_DrawHudEditor();
	}
}

void CG_DrawMissileCamera(hudComponent_t *comp)
{
	float     x, y, w, h;
	refdef_t  refdef;
	vec3_t    delta, angles;
	centity_t *cent;

	if (!cg.latestMissile || cgs.matchPaused)
	{
		return;
	}

	// Save out the old render info so we don't kill the LOD system here
	trap_R_SaveViewParms();

	cent = cg.latestMissile;

	memset(&refdef, 0, sizeof(refdef_t));
	memcpy(refdef.areamask, cg.snap->areamask, sizeof(refdef.areamask));

	x = comp->location.x;
	y = comp->location.y;
	w = comp->location.w;
	h = comp->location.h;

	CG_AdjustFrom640(&x, &y, &w, &h);
	memset(&refdef, 0, sizeof(refdef));
	AxisClear(refdef.viewaxis);

	refdef.fov_x  = cg.refdef_current->fov_x;
	refdef.fov_y  = cg.refdef_current->fov_y;
	refdef.x      = x;
	refdef.y      = y;
	refdef.width  = w;
	refdef.height = h;
	refdef.time   = cg.time;

	VectorCopy(cent->lerpOrigin, refdef.vieworg);

	BG_EvaluateTrajectoryDelta(&cent->currentState.pos, cg.time, delta, qtrue, 0);
	vectoangles(delta, angles);
	AnglesToAxis(angles, refdef.viewaxis);

	cg.refdef_current = &refdef;

	trap_R_ClearScene();

	CG_SetupFrustum();
	CG_DrawSkyBoxPortal(qfalse);

	if (!cg.hyperspace)
	{
		CG_AddPacketEntities();
		CG_AddMarks();
		CG_AddParticles();
		CG_AddLocalEntities(qfalse);
		CG_AddSmokeSprites();
		CG_AddAtmosphericEffects();
		CG_AddFlameChunks();
		CG_AddTrails();        // this must come last, so the trails dropped this frame get drawn
		CG_PB_RenderPolyBuffers();
		CG_DrawMiscGamemodels();
		CG_Coronas();
	}

	refdef.time = cg.time;
	trap_SetClientLerpOrigin(refdef.vieworg[0], refdef.vieworg[1], refdef.vieworg[2]);

	trap_R_RenderScene(&refdef);

	cg.refdef_current = &cg.refdef;

	// grain shader
	//CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.tv_grain);

	// Reset the view parameters
	trap_R_RestoreViewParms();
}
