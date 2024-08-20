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
 * @file tr_image_svg.c
 * @brief SVG LOADING
 */

#include "tr_local_proxy.h"

#include "tr_common.h"

#include "../qcommon/qcommon.h"

#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"

/**
 * @brief R_LoadSVG loads an svg image and converts it to a buffer
 * @param [in] name name of the image to load
 * @param [in,out] pic output image buffer
 * @param [in,out] width width of the image
 * @param [in,out] height height of the image
 * @param [in] alphaByte - unused
 */
qboolean R_LoadSVG(imageData_t *data, byte **pic, int *width, int *height, byte alphaByte)
{
	NSVGimage      *image = NULL;
	NSVGrasterizer *rast  = NULL;
	int            columns, rows, numPixels;
	float          dpi, scale;
	byte           *img_data, *tmp_data;

	*pic = NULL;

	if (width)
	{
		*width = 0;
	}
	if (height)
	{
		*height = 0;
	}

	// dpi = 96.f * ((float)glConfig.vidHeight / SCREEN_HEIGHT_F);
	dpi = 96.f;
	if (dpi < 96.f)
	{
		dpi = 96.f;
	}

	// the svg parser modifies the data, so we need to copy it.
	// FIXME: look into if there is an update at some point on this..
	tmp_data = ri.Hunk_AllocateTempMemory(data->size + 1);
	if (!tmp_data)
	{
		Ren_Warning("R_LoadSVG: Could not allocate memory for the svg image.\n");
		return qfalse;
	}

	Com_Memcpy(tmp_data, data->buffer.v, data->size);
	tmp_data[data->size] = 0;
	image                = nsvgParse((char *)tmp_data, "px", dpi);
	ri.Hunk_FreeTempMemory(tmp_data);

	if (image == NULL)
	{
		Ren_Warning("R_LoadSVG: Could not parse svg.\n");
		return qfalse;
	}

	rast = nsvgCreateRasterizer();
	if (rast == NULL)
	{
		nsvgDelete(image);
		Ren_Warning("R_LoadSVG: Could not init svg rasterizer.\n");
		return qfalse;
	}

	scale = MAX((float)glConfig.vidHeight / SCREEN_HEIGHT_F, (float)glConfig.vidWidth / SCREEN_WIDTH_F);
	if (scale < 0.f)
	{
		scale = 2.25f;  // safe 16:9 value from 1920 * 1080 res
	}

	columns = (int)(image->width * scale);
	rows    = (int)(image->height * scale);

#ifdef GL_ARB_texture_non_power_of_two
	if (!GLEW_ARB_texture_non_power_of_two && (!Com_PowerOf2(columns) || !Com_PowerOf2(rows)))
	{
		columns = (int)Com_ClosestPowerOf2(columns);
		scale   = (float)columns / image->width;
		rows    = (int)(image->height * scale);

		if (!Com_PowerOf2(rows))
		{
			scale   = 2.25f;    // safe 16:9 value from 1920 * 1080 res
			columns = (int)image->width * scale;
			rows    = (int)image->height * scale;
		}
	}
#endif

	// force higher svg resolution (0: x1, 1: x2, 2: x4)
	if (r_scalesvg->integer)
	{
		int value = r_scalesvg->integer;
		columns = columns << value;
		rows    = rows << value;
		scale   = (float)columns / image->width;
	}

	// cap to max texture size to avoid ResampleTexture errors, and keep memory usage sane
	if (columns > 2048 || rows > 2048)
	{
		float ratio = (float)image->width / image->height;

		if (columns > rows)
		{
			columns = 2048;
			rows    = (int)(2048 / ratio);
		}
		else
		{
			rows    = 2048;
			columns = (int)(2048 * ratio);
		}

		scale = (float)columns / image->width;
	}

	columns = MIN(columns, 2048);
	rows    = MIN(rows, 2048);

	numPixels = columns * rows * 4;

	img_data = R_GetImageBuffer(numPixels, BUFFER_IMAGE, data->name);

	if (!img_data)
	{
		nsvgDeleteRasterizer(rast);
		nsvgDelete(image);
		Ren_Warning("R_LoadSVG: Could not allocate memory for the svg image.\n");
		return qfalse;
	}

	nsvgRasterize(rast, image, 0, 0, scale, img_data, columns, rows, columns * 4);

	if (width)
	{
		*width = columns;
	}
	if (height)
	{
		*height = rows;
	}
	*pic = img_data;

	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);

	return qtrue;
}
