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
 * @file tr_image_pcx.c
 * @brief PCX files are used for 8 bit images
 */

#include "tr_common.h"

typedef struct
{
	char manufacturer;
	char version;
	char encoding;
	char bits_per_pixel;
	unsigned short xmin, ymin, xmax, ymax;
	unsigned short hres, vres;
	unsigned char palette[48];
	char reserved;
	char color_planes;
	unsigned short bytes_per_line;
	unsigned short palette_type;
	unsigned short hscreensize, vscreensize;
	char filler[54];
	unsigned char data[];
} pcx_t;

/**
 * @brief R_LoadPCX
 * @param[in] filename
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param alphaByte - unused
 */
void R_LoadPCX(const char *filename, byte **pic, int *width, int *height, byte alphaByte)
{
	union
	{
		byte *b;
		void *v;
	} raw;
	byte           *end;
	pcx_t          *pcx;
	int            len;
	unsigned char  dataByte = 0, runLength = 0;
	byte           *out, *pix;
	unsigned short w, h;
	byte           *pic8;
	byte           *palette;
	unsigned int   i;
	unsigned       size = 0;

	if (width)
	{
		*width = 0;
	}
	if (height)
	{
		*height = 0;
	}
	*pic = NULL;

	// load the file
	len = ri.FS_ReadFile(filename, &raw.v);
	if (!raw.b || len <= 0)
	{
		return;
	}

	if ((unsigned)len < sizeof(pcx_t))
	{
		Ren_Print("PCX truncated: %s\n", filename);
		ri.FS_FreeFile(raw.v);
		return;
	}

	// parse the PCX file
	pcx = (pcx_t *)raw.b;
	end = raw.b + len;

	w    = LittleShort(pcx->xmax) + 1;
	h    = LittleShort(pcx->ymax) + 1;
	size = w * h;

	if (pcx->manufacturer != 0x0a
	    || pcx->version != 5
	    || pcx->encoding != 1
	    || pcx->color_planes != 1
	    || pcx->bits_per_pixel != 8
	    || w >= 1024
	    || h >= 1024)
	{
		Ren_Print("Bad or unsupported pcx file %s (%dx%d@%d)\n", filename, w, h, pcx->bits_per_pixel);
		return;
	}

	pix = pic8 = R_GetImageBuffer(size, BUFFER_IMAGE, filename);

	raw.b = pcx->data;
	// FIXME: should use bytes_per_line but original q3 didn't do that either
	while (pix < pic8 + size)
	{
		if (runLength > 0)
		{
			*pix++ = dataByte;
			--runLength;
			continue;
		}

		if (raw.b + 1 > end)
		{
			break;
		}
		dataByte = *raw.b++;

		if ((dataByte & 0xC0) == 0xC0)
		{
			if (raw.b + 1 > end)
			{
				break;
			}
			runLength = dataByte & 0x3F;
			dataByte  = *raw.b++;
		}
		else
		{
			runLength = 1;
		}
	}

	if (pix < pic8 + size)
	{
		Ren_Print("PCX file truncated: %s\n", filename);
		ri.FS_FreeFile(pcx);
		ri.Free(pic8);
	}

	if (raw.b - (byte *)pcx >= end - (byte *)769 || end[-769] != 0x0c)
	{
		Ren_Print("PCX missing palette: %s\n", filename);
		ri.FS_FreeFile(pcx);
		ri.Free(pic8);
		return;
	}

	palette = end - 768;

	pix = out = ri.Z_Malloc(4 * size);
	for (i = 0 ; i < size ; i++)
	{
		unsigned char p = pic8[i];
		pix[0] = palette[p * 3];
		pix[1] = palette[p * 3 + 1];
		pix[2] = palette[p * 3 + 2];
		pix[3] = 255;
		pix   += 4;
	}

	if (width)
	{
		*width = w;
	}
	if (height)
	{
		*height = h;
	}

	*pic = out;

	ri.FS_FreeFile(pcx);
	ri.Free(pic8);
}
