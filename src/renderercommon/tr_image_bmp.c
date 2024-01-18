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
 * @file tr_image_bmp.c
 */

#include "tr_common.h"

typedef struct
{
	char id[2];
	unsigned fileSize;
	unsigned reserved0;
	unsigned bitmapDataOffset;
	unsigned bitmapHeaderSize;
	unsigned width;
	unsigned height;
	unsigned short planes;
	unsigned short bitsPerPixel;
	unsigned compression;
	unsigned bitmapDataSize;
	unsigned hRes;
	unsigned vRes;
	unsigned colors;
	unsigned importantColors;
	unsigned char palette[256][4];
} BMPHeader_t;

/**
 * @brief R_LoadBMP
 * @param[in] name
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param alphaByte - unused
 */
qboolean R_LoadBMP(imageData_t *data, byte **pic, int *width, int *height, byte alphaByte)
{
	int         columns, rows;
	unsigned    numPixels;
	byte        *pixbuf;
	int         row, column;
	byte        *buf_p;
	byte        *end;
	BMPHeader_t bmpHeader;
	byte        *bmpRGBA;

	*pic = NULL;

	if (width)
	{
		*width = 0;
	}

	if (height)
	{
		*height = 0;
	}

	if (data->size < 54)
	{
		Ren_Warning("LoadBMP: header too short (%s)\n", data->name);
	}

	buf_p = data->buffer.b;
	end   = data->buffer.b + data->size;

	bmpHeader.id[0]            = *buf_p++;
	bmpHeader.id[1]            = *buf_p++;
	bmpHeader.fileSize         = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.reserved0        = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.bitmapDataOffset = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.bitmapHeaderSize = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.width            = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.height           = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.planes           = LittleShort(*( short * ) buf_p);
	buf_p                     += 2;
	bmpHeader.bitsPerPixel     = LittleShort(*( short * ) buf_p);
	buf_p                     += 2;
	bmpHeader.compression      = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.bitmapDataSize   = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.hRes             = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.vRes             = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.colors           = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;
	bmpHeader.importantColors  = LittleLong(*( int * ) buf_p);
	buf_p                     += 4;

	if (bmpHeader.bitsPerPixel == 8)
	{
		if (buf_p + sizeof(bmpHeader.palette) > end)
		{
			Ren_Warning("LoadBMP: header too short (%s)\n", data->name);
			return qfalse;
		}

		Com_Memcpy(bmpHeader.palette, buf_p, sizeof(bmpHeader.palette));
	}

	if (data->buffer.b + bmpHeader.bitmapDataOffset > end)
	{
		Ren_Warning("LoadBMP: invalid offset value in header (%s)\n", data->name);
		return qfalse;
	}

	buf_p = data->buffer.b + bmpHeader.bitmapDataOffset;

	if (bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M')
	{
		Ren_Warning("LoadBMP: only Windows-style BMP files supported (%s)\n", data->name);
		return qfalse;
	}
	if (bmpHeader.fileSize != data->size)
	{
		Ren_Warning("LoadBMP: header size does not match file size (%u vs. %u) (%s)\n", bmpHeader.fileSize, data->size, data->name);
		return qfalse;
	}
	if (bmpHeader.compression != 0)
	{
		Ren_Warning("LoadBMP: only uncompressed BMP files supported (%s)\n", data->name);
		return qfalse;
	}
	if (bmpHeader.bitsPerPixel < 8)
	{
		Ren_Warning("LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", data->name);
		return qfalse;
	}

	switch (bmpHeader.bitsPerPixel)
	{
	case 8:
	case 16:
	case 24:
	case 32:
		break;
	default:
		Ren_Warning("LoadBMP: illegal pixel_size '%hu' in file '%s'\n", bmpHeader.bitsPerPixel, data->name);
		return qfalse;
	}

	columns = bmpHeader.width;
	rows    = bmpHeader.height;
	if (rows < 0)
	{
		rows = -rows;
	}
	numPixels = columns * rows;

	if (columns <= 0 || !rows || numPixels > 0x1FFFFFFF // 4*1FFFFFFF == 0x7FFFFFFC < 0x7FFFFFFF
	    || ((numPixels * 4) / columns) / 4 != rows)
	{
		Ren_Warning("LoadBMP: %s has an invalid image size\n", data->name);
		return qfalse;
	}
	if (buf_p + numPixels * bmpHeader.bitsPerPixel / 8 > end)
	{
		Ren_Warning("LoadBMP: file truncated (%s)\n", data->name);
		return qfalse;
	}

	if (width)
	{
		*width = columns;
	}
	if (height)
	{
		*height = rows;
	}

	bmpRGBA = R_GetImageBuffer(numPixels * 4, BUFFER_IMAGE, data->name);
	*pic    = bmpRGBA;


	for (row = rows - 1; row >= 0; row--)
	{
		pixbuf = bmpRGBA + row * columns * 4;

		for (column = 0; column < columns; column++)
		{
			unsigned char  red, green, blue, alpha;
			int            palIndex;
			unsigned short shortPixel;

			switch (bmpHeader.bitsPerPixel)
			{
			case 8:
				palIndex  = *buf_p++;
				*pixbuf++ = bmpHeader.palette[palIndex][2];
				*pixbuf++ = bmpHeader.palette[palIndex][1];
				*pixbuf++ = bmpHeader.palette[palIndex][0];
				*pixbuf++ = 0xff;
				break;
			case 16:
				shortPixel = *( unsigned short * ) pixbuf;
				pixbuf    += 2;
				*pixbuf++  = (shortPixel & (31 << 10)) >> 7;
				*pixbuf++  = (shortPixel & (31 << 5)) >> 2;
				*pixbuf++  = (shortPixel & (31)) << 3;
				*pixbuf++  = 0xff;
				break;

			case 24:
				blue      = *buf_p++;
				green     = *buf_p++;
				red       = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 255;
				break;
			case 32:
				blue      = *buf_p++;
				green     = *buf_p++;
				red       = *buf_p++;
				alpha     = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				break;
			}
		}
	}

	return qtrue;
}
