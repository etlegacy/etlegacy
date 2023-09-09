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
void R_LoadSVG(imageData_t *data, byte **pic, int *width, int *height, byte alphaByte)
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
	tmp_data = Com_Allocate(data->size);
	Com_Memcpy(tmp_data, data->buffer.v, data->size);
	if (!tmp_data)
	{
		Ren_Drop("Could not allocate memory for the svg image.\n");
		return;
	}

	image = nsvgParse((char *)tmp_data, "px", dpi);
	Com_Dealloc(tmp_data);

	if (image == NULL)
	{
		ri.FS_FreeFile(data->buffer.v);
		Ren_Drop("Could not parse svg.\n");
		return;
	}

	rast = nsvgCreateRasterizer();
	if (rast == NULL)
	{
		nsvgDelete(image);
		Ren_Drop("Could not init svg rasterizer.\n");
		return;
	}

	scale = (float)glConfig.vidHeight / SCREEN_HEIGHT_F;
	if (scale < 0.f)
	{
		scale = 1.f;
	}

	columns = (int)(image->width * scale);
	rows    = (int)(image->height * scale);

	numPixels = columns * rows * 4;

	img_data = R_GetImageBuffer(numPixels, BUFFER_IMAGE, data->name);

	if (!img_data)
	{
		nsvgDeleteRasterizer(rast);
		nsvgDelete(image);
		Ren_Drop("Could not allocate memory for the svg image.\n");
		return;
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
}
