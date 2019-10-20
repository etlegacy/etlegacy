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
 * @file tr_image_svg.c
 * @brief SVG LOADING
 */

#ifdef FEATURE_RENDERER2
#include "../renderer2/tr_local.h"
#else
#include "../renderer/tr_local.h"
#endif

#include "tr_common.h"

#include "../qcommon/qcommon.h"

#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"


void R_LoadSVG(const char *name, byte **pic, int *width, int *height, byte alphaByte)
{
	NSVGimage      *image = NULL;
	NSVGrasterizer *rast  = NULL;
	unsigned       columns, rows, numPixels;
	int            length;
	byte           *img_data;
	union
	{
		byte *b;
		void *v;
	} buffer;

	*pic = NULL;

	if (width)
	{
		*width = 0;
	}
	if (height)
	{
		*height = 0;
	}

	//
	// load the file
	//
	length = ri.FS_ReadFile(name, &buffer.v);
	if (!buffer.b || length <= 0)
	{
		return;
	}

	image = nsvgParse((char *)buffer.b, "px", 96.f * (glConfig.vidHeight / (float)SCREEN_HEIGHT));
	if (image == NULL)
	{
		ri.FS_FreeFile(buffer.v);
		Ren_Drop("Could not parse svg.\n");
		return;
	}

	rast = nsvgCreateRasterizer();
	if (rast == NULL)
	{
		nsvgDelete(image);
		ri.FS_FreeFile(buffer.v);
		Ren_Drop("Could not init svg rasterizer.\n");
		return;
	}

	columns = (int)image->width;
	rows    = (int)image->height;

	numPixels = columns * rows * 4;

	img_data = R_GetImageBuffer(numPixels, BUFFER_IMAGE, name);

	if (!img_data)
	{
		nsvgDeleteRasterizer(rast);
		nsvgDelete(image);
		ri.FS_FreeFile(buffer.v);
		Ren_Drop("Could not allocate memory for the svg image.\n");
		return;
	}

	nsvgRasterize(rast, image, 0, 0, 1, img_data, columns, rows, columns * 4);

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
	// ri.Free(img_data);
	ri.FS_FreeFile(buffer.v);
}
