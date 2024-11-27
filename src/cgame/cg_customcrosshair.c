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
 * @file cg_crosshair.c
 * @brief Handling of new "vectorized" custom crosshair.
 */

#include "cg_local.h"

static float x, y;

static const float *cached_color = NULL;

static float *fgColor;
static float *bgColor;

static inline void CG_CustomCrosshairSetColor(const float *color)
{
	if (cached_color == NULL || cached_color != color)
	{
		trap_R_SetColor(color);
		cached_color = color;
	}
}

static inline void CG_CustomCrosshairDrawRect(float x, float y, float width, float height, float outlineWidth, qboolean rounded, uint8_t filled)
{
	if (outlineWidth > 0.0)
	{
		int offsetX = 0;
		if (rounded)
		{
			offsetX = outlineWidth;
		}

		CG_CustomCrosshairSetColor(bgColor);

		trap_R_DrawStretchPic(x + offsetX, // top
		                      y - outlineWidth,
		                      width + outlineWidth + outlineWidth - offsetX - offsetX, outlineWidth, 0, 0, 0, 1, cgs.media.whiteShader);

		trap_R_DrawStretchPic(x + offsetX, // bottom
		                      y + height,
		                      width + outlineWidth + outlineWidth - offsetX - offsetX, outlineWidth, 0, 0, 0, 1, cgs.media.whiteShader);

		trap_R_DrawStretchPic(x, // left
		                      y,
		                      outlineWidth, height, 0, 0, 0, 1, cgs.media.whiteShader);

		trap_R_DrawStretchPic(x + width + outlineWidth, // right
		                      y,
		                      outlineWidth, height, 0, 0, 0, 1, cgs.media.whiteShader);
	}

	if (filled)
	{
		CG_CustomCrosshairSetColor(fgColor);
		trap_R_DrawStretchPic(x + outlineWidth,
		                      y,
		                      width, height, 0, 0, 0, 1, cgs.media.whiteShader);
	}
}

static float CG_CustomCrosshairCalcSpread()
{
	float spreadDistance = cg_customCrosshairCrossSpreadDistance.value;
	if (spreadDistance)
	{
		if (cg.predictedPlayerState.groundEntityNum == ENTITYNUM_NONE)
		{
			return spreadDistance * cg_customCrosshairCrossSpreadOTGCoef.value;
		}
		else
		{
			return ((float)cg.snap->ps.aimSpreadScale / 255.0) * spreadDistance;
		}
	}

	return 0.0;
}

void CG_DrawCustomCrosshair()
{
	static float   innerWidth, innerWidthOffset, outlineWidth, crossLength;
	static float   crossGap, crossSpread;
	static uint8_t outlineRounded;

	x = (float)cgs.glconfig.vidWidth / 2 + cg_crosshairX.value;
	y = (float)cgs.glconfig.vidHeight / 2 + cg_crosshairY.value;

	crossSpread = CG_CustomCrosshairCalcSpread();
	crossGap    = cg_customCrosshairCrossGap.value + crossSpread;

	// draw dot and/or small cross
	if (cg_customCrosshair.integer == CUSTOMCROSSHAIR_DOT_WITH_SMALLCROSS
	    || cg_customCrosshair.integer == CUSTOMCROSSHAIR_DOT
	    || cg_customCrosshair.integer == CUSTOMCROSSHAIR_SMALLCROSS)
	{
		// draw dot
		if (cg_customCrosshair.integer == CUSTOMCROSSHAIR_DOT_WITH_SMALLCROSS || cg_customCrosshair.integer == CUSTOMCROSSHAIR_DOT)
		{
			innerWidth       = cg_customCrosshairDotWidth.value;
			innerWidthOffset = innerWidth / 2;
			outlineWidth     = cg_customCrosshairDotOutlineWidth.value;
			crossLength      = cg_customCrosshairCrossLength.value;
			outlineRounded   = cg_customCrosshairDotOutlineRounded.integer;
			fgColor          = cgs.customCrosshairDotColor;
			bgColor          = cgs.customCrosshairDotOutlineColor;

			// TODO : make rounded look proper > 1.0 outlineWidth as well
			if (outlineWidth > 1.0)
			{
				outlineRounded = qfalse;
			}

			CG_CustomCrosshairDrawRect(
				x - innerWidthOffset - outlineWidth,
				y - innerWidthOffset,
				innerWidth, innerWidth, outlineWidth, cg_customCrosshairDotOutlineRounded.integer, qtrue);
		}

		// draw small cross
		if (cg_customCrosshair.integer == CUSTOMCROSSHAIR_DOT_WITH_SMALLCROSS || cg_customCrosshair.integer == CUSTOMCROSSHAIR_SMALLCROSS)
		{
			innerWidth       = cg_customCrosshairCrossWidth.value;
			innerWidthOffset = innerWidth / 2;
			outlineWidth     = cg_customCrosshairCrossOutlineWidth.value;
			outlineRounded   = cg_customCrosshairCrossOutlineRounded.integer;
			crossLength      = cg_customCrosshairCrossLength.value;
			fgColor          = cgs.customCrosshairCrossColor;
			bgColor          = cgs.customCrosshairCrossOutlineColor;

			// TODO : make rounded look proper > 1.0 outlineWidth as well
			if (outlineWidth > 1.0)
			{
				outlineRounded = qfalse;
			}

			// top outline
			CG_CustomCrosshairDrawRect(
				x - innerWidthOffset - outlineWidth,
				y - innerWidthOffset - (outlineWidth * 2) - crossLength - crossGap,
				innerWidth, crossLength, outlineWidth, outlineRounded, qtrue);

			// bottom outline
			CG_CustomCrosshairDrawRect(
				x - innerWidthOffset - outlineWidth,
				y - innerWidthOffset + innerWidth + (outlineWidth * 2) + crossGap,
				innerWidth, crossLength, outlineWidth, outlineRounded, qtrue);

			// left outline
			CG_CustomCrosshairDrawRect(
				x - innerWidthOffset - (outlineWidth * 3) - crossLength - crossGap,
				y - innerWidthOffset,
				crossLength, innerWidth, outlineWidth, outlineRounded, qtrue);

			// right outline
			CG_CustomCrosshairDrawRect(
				x - innerWidthOffset + innerWidth + (outlineWidth * 1) + crossGap,
				y - innerWidthOffset,
				crossLength, innerWidth, outlineWidth, outlineRounded, qtrue);
		}
	}
	// draw full cross
	else if (cg_customCrosshair.integer == CUSTOMCROSSHAIR_FULLCROSS)
	{
		innerWidth       = cg_customCrosshairCrossWidth.value;
		innerWidthOffset = innerWidth / 2;
		outlineRounded   = cg_customCrosshairCrossOutlineRounded.integer;
		outlineWidth     = cg_customCrosshairCrossOutlineWidth.value;
		crossLength      = cg_customCrosshairCrossLength.value;
		fgColor          = cgs.customCrosshairCrossColor;
		bgColor          = cgs.customCrosshairCrossOutlineColor;

		// TODO : make rounded look proper > 1.0 outlineWidth as well
		if (outlineWidth > 1.0)
		{
			outlineRounded = qfalse;
		}

		if (crossSpread != 0.0)
		{
			crossLength += crossSpread;
		}

		CG_CustomCrosshairSetColor(fgColor);

		// vertical filling
		trap_R_DrawStretchPic(x - innerWidthOffset,
		                      y - innerWidthOffset - crossLength,
		                      innerWidth, crossLength, 0, 0, 0, 1, cgs.media.whiteShader);

		trap_R_DrawStretchPic(x - innerWidthOffset,
		                      y + innerWidthOffset,
		                      innerWidth, crossLength, 0, 0, 0, 1, cgs.media.whiteShader);

		// horizontal filling
		trap_R_DrawStretchPic(x - innerWidthOffset - crossLength,
		                      y - innerWidthOffset,
		                      crossLength, innerWidth, 0, 0, 0, 1, cgs.media.whiteShader);

		trap_R_DrawStretchPic(x - innerWidthOffset + innerWidth,
		                      y - innerWidthOffset,
		                      crossLength, innerWidth, 0, 0, 0, 1, cgs.media.whiteShader);

		CG_CustomCrosshairSetColor(cgs.customCrosshairDotColor);

		// center filling
		trap_R_DrawStretchPic(x - innerWidthOffset,
		                      y - innerWidthOffset,
		                      innerWidth, innerWidth, 0, 0, 0, 1, cgs.media.whiteShader);

		if (outlineWidth > 0.0)
		{
			int roundedOffset = 0.0;
			if (cg_customCrosshairCrossOutlineRounded.integer)
			{
				roundedOffset = outlineWidth;
			}

			CG_CustomCrosshairSetColor(bgColor);

			// top outline
			trap_R_DrawStretchPic(x - innerWidthOffset - crossLength,
			                      y - innerWidthOffset - outlineWidth,
			                      crossLength, outlineWidth, 0, 0, 0, 1, cgs.media.whiteShader);

			trap_R_DrawStretchPic(x - innerWidthOffset,
			                      y - innerWidthOffset - crossLength - outlineWidth,
			                      innerWidth, outlineWidth, 0, 0, 0, 1, cgs.media.whiteShader);

			trap_R_DrawStretchPic(x + outlineWidth,
			                      y - innerWidthOffset - outlineWidth,
			                      crossLength, outlineWidth, 0, 0, 0, 1, cgs.media.whiteShader);

			// bottom outline
			trap_R_DrawStretchPic(x - innerWidthOffset - crossLength,
			                      y - innerWidthOffset + innerWidth,
			                      crossLength, outlineWidth, 0, 0, 0, 1, cgs.media.whiteShader);

			trap_R_DrawStretchPic(x - innerWidthOffset,
			                      y - innerWidthOffset + innerWidth + crossLength,
			                      innerWidth, outlineWidth, 0, 0, 0, 1, cgs.media.whiteShader);

			trap_R_DrawStretchPic(x + outlineWidth,
			                      y - innerWidthOffset + innerWidth,
			                      crossLength, outlineWidth, 0, 0, 0, 1, cgs.media.whiteShader);

			// sides-top outline
			trap_R_DrawStretchPic(x - innerWidthOffset - outlineWidth,
			                      y - innerWidthOffset - crossLength - outlineWidth + roundedOffset,
			                      outlineWidth, crossLength - roundedOffset, 0, 0, 0, 1, cgs.media.whiteShader);

			trap_R_DrawStretchPic(x + outlineWidth,
			                      y - innerWidthOffset - crossLength - outlineWidth + roundedOffset,
			                      outlineWidth, crossLength - roundedOffset, 0, 0, 0, 1, cgs.media.whiteShader);

			// sides-center outline
			trap_R_DrawStretchPic(x - innerWidthOffset - crossLength - outlineWidth,
			                      y - innerWidthOffset - outlineWidth + roundedOffset,
			                      outlineWidth, innerWidth + (outlineWidth - roundedOffset) * 2, 0, 0, 0, 1, cgs.media.whiteShader);

			trap_R_DrawStretchPic(x - innerWidthOffset + innerWidth + crossLength,
			                      y - innerWidthOffset - outlineWidth + roundedOffset,
			                      outlineWidth, innerWidth + (outlineWidth - roundedOffset) * 2, 0, 0, 0, 1, cgs.media.whiteShader);

			// sides-bottom outline
			trap_R_DrawStretchPic(x - innerWidthOffset - outlineWidth,
			                      y - innerWidthOffset + innerWidth + outlineWidth,
			                      outlineWidth, crossLength - roundedOffset, 0, 0, 0, 1, cgs.media.whiteShader);

			trap_R_DrawStretchPic(x + outlineWidth,
			                      y - innerWidthOffset + innerWidth + outlineWidth,
			                      outlineWidth, crossLength - roundedOffset, 0, 0, 0, 1, cgs.media.whiteShader);
		}
	}

	CG_CustomCrosshairSetColor(NULL);
}
