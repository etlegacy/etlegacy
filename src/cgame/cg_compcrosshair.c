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
 * @brief Handling of new vectorized competitive crosshair.
 */

#include "cg_local.h"

static int x, y;

static const float *cached_color = NULL;

static float *fgColor;
static float *bgColor;

static inline void CG_CompCrosshairSetColor(const float *color)
{
	if (cached_color == NULL || cached_color != color)
	{
		trap_R_SetColor(color);
		cached_color = color;
	}
}

static inline void CG_CompCrosshairDrawRect(float x, float y, float width, float height, float outlineWidth, qboolean rounded, uint8_t filled)
{
	if (outlineWidth > 0.0)
	{
		int offsetX = 0;
		// int offsetY = 0;
		if (rounded)
		{
			offsetX = outlineWidth;
			// offsetY = outlineWidth;
		}

		CG_CompCrosshairSetColor(bgColor);

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
		CG_CompCrosshairSetColor(fgColor);
		trap_R_DrawStretchPic(x + outlineWidth,
		                      y,
		                      width, height, 0, 0, 0, 1, cgs.media.whiteShader);
	}
}

static float CG_CompCrosshairCalcSpread()
{
	float spreadDistance = cg_compCrosshairCrossSpreadDistance.value;
	if (spreadDistance)
	{
		if (cg.predictedPlayerState.groundEntityNum == ENTITYNUM_NONE)
		{
			return spreadDistance * cg_compCrosshairCrossSpreadOTGCoef.value;
		}
		else
		{
			return ((float)cg.snap->ps.aimSpreadScale / 255.0) * spreadDistance;
		}
	}

	return 0.0;
}

void CG_DrawCompCrosshair()
{
	static float   innerWidth, innerWidthOffset, outlineWidth, crossLength;
	static float   crossGap, crossSpread;
	static uint8_t outlineRounded;

	if (cg_compCrosshair.integer <= COMPCROSSHAIR_NONE || cg_compCrosshair.integer >= COMPCROSSHAIR_MAX)
	{
		return;
	}

	x = (cgs.glconfig.vidWidth / 2);
	y = (cgs.glconfig.vidHeight / 2);

	crossSpread = CG_CompCrosshairCalcSpread();
	crossGap    = cg_compCrosshairCrossGap.value + crossSpread;

	// draw dot and/or small cross
	if (cg_compCrosshair.integer == COMPCROSSHAIR_DOT_WITH_SMALLCROSS
	    || cg_compCrosshair.integer == COMPCROSSHAIR_DOT
	    || cg_compCrosshair.integer == COMPCROSSHAIR_SMALLCROSS)
	{
		// draw dot
		if (cg_compCrosshair.integer == COMPCROSSHAIR_DOT_WITH_SMALLCROSS || cg_compCrosshair.integer == COMPCROSSHAIR_DOT)
		{
			innerWidth       = cg_compCrosshairDotWidth.value;
			innerWidthOffset = innerWidth / 2;
			outlineWidth     = cg_compCrosshairDotOutlineWidth.value;
			crossLength      = cg_compCrosshairCrossLength.value;
			outlineRounded   = cg_compCrosshairDotOutlineRounded.integer;
			fgColor          = cgs.compCrosshairDotColor;
			bgColor          = cgs.compCrosshairDotOutlineColor;

			CG_CompCrosshairDrawRect(
				x - innerWidthOffset - outlineWidth,
				y - innerWidthOffset,
				innerWidth, innerWidth, outlineWidth, cg_compCrosshairDotOutlineRounded.integer, qtrue);
		}

		// draw small cross
		if (cg_compCrosshair.integer == COMPCROSSHAIR_DOT_WITH_SMALLCROSS || cg_compCrosshair.integer == COMPCROSSHAIR_SMALLCROSS)
		{
			innerWidth       = cg_compCrosshairCrossWidth.value;
			innerWidthOffset = innerWidth / 2;
			outlineWidth     = cg_compCrosshairCrossOutlineWidth.value;
			outlineRounded   = cg_compCrosshairCrossOutlineRounded.integer;
			crossLength      = cg_compCrosshairCrossLength.value;
			fgColor          = cgs.compCrosshairCrossColor;
			bgColor          = cgs.compCrosshairCrossOutlineColor;

			// top outline
			CG_CompCrosshairDrawRect(
				x - innerWidthOffset - outlineWidth,
				y - innerWidthOffset - (outlineWidth * 2) - crossLength - crossGap,
				innerWidth, crossLength, outlineWidth, outlineRounded, qtrue);

			// bottom outline
			CG_CompCrosshairDrawRect(
				x - innerWidthOffset - outlineWidth,
				y - innerWidthOffset + innerWidth + (outlineWidth * 2) + crossGap,
				innerWidth, crossLength, outlineWidth, outlineRounded, qtrue);

			// left outline
			CG_CompCrosshairDrawRect(
				x - innerWidthOffset - (outlineWidth * 3) - crossLength - crossGap,
				y - innerWidthOffset,
				crossLength, innerWidth, outlineWidth, outlineRounded, qtrue);

			// right outline
			CG_CompCrosshairDrawRect(
				x - innerWidthOffset + innerWidth + (outlineWidth * 1) + crossGap,
				y - innerWidthOffset,
				crossLength, innerWidth, outlineWidth, outlineRounded, qtrue);
		}
	}
	// draw full cross
	else if (cg_compCrosshair.integer == COMPCROSSHAIR_FULLCROSS)
	{
		innerWidth       = cg_compCrosshairCrossWidth.value;
		innerWidthOffset = innerWidth / 2;
		outlineRounded   = cg_compCrosshairCrossOutlineRounded.integer;
		outlineWidth     = cg_compCrosshairCrossOutlineWidth.value;
		crossLength      = cg_compCrosshairCrossLength.value;
		fgColor          = cgs.compCrosshairCrossColor;
		bgColor          = cgs.compCrosshairCrossOutlineColor;

		if (crossSpread != 0.0)
		{
			crossLength += crossSpread;
		}

		CG_CompCrosshairSetColor(fgColor);

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

		CG_CompCrosshairSetColor(cgs.compCrosshairDotColor);

		// center filling
		trap_R_DrawStretchPic(x - innerWidthOffset,
		                      y - innerWidthOffset,
		                      innerWidth, innerWidth, 0, 0, 0, 1, cgs.media.whiteShader);

		if (outlineWidth > 0.0)
		{
			int roundedOffset = 0.0;
			if (cg_compCrosshairCrossOutlineRounded.integer)
			{
				roundedOffset = outlineWidth;
			}

			CG_CompCrosshairSetColor(bgColor);

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

	CG_CompCrosshairSetColor(NULL);
}
