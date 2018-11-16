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
 * @file tr_image_tga.c
 * @brief TGA files are used for 24/32 bit images
 */

#include "tr_common.h"

typedef struct _TargaHeader
{
	unsigned char id_length, colormap_type, image_type;
	unsigned short colormap_index, colormap_length;
	unsigned char colormap_size;
	unsigned short x_origin, y_origin, width, height;
	unsigned char pixel_size, attributes;
} TargaHeader;

/**
 * @brief R_LoadTGA
 * @param[in,out] name
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[in] alphaByte
 */
void R_LoadTGA(const char *name, byte **pic, int *width, int *height, byte alphaByte)
{
	unsigned     columns, rows, numPixels;
	byte         *pixbuf;
	unsigned int row, column;
	byte         *buf_p;
	byte         *end;
	union
	{
		byte *b;
		void *v;
	} buffer;
	TargaHeader targa_header;
	byte        *targa_rgba;
	int         length;

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

	if (length < 18)
	{
		ri.FS_FreeFile(buffer.v);
		Ren_Drop("LoadTGA: header too short (%s)\n", name);
	}

	buf_p = buffer.b;
	end   = buffer.b + length;

	targa_header.id_length     = buf_p[0];
	targa_header.colormap_type = buf_p[1];
	targa_header.image_type    = buf_p[2];

	Com_Memcpy(&targa_header.colormap_index, &buf_p[3], 2);
	Com_Memcpy(&targa_header.colormap_length, &buf_p[5], 2);
	targa_header.colormap_size = buf_p[7];
	Com_Memcpy(&targa_header.x_origin, &buf_p[8], 2);
	Com_Memcpy(&targa_header.y_origin, &buf_p[10], 2);
	Com_Memcpy(&targa_header.width, &buf_p[12], 2);
	Com_Memcpy(&targa_header.height, &buf_p[14], 2);
	targa_header.pixel_size = buf_p[16];
	targa_header.attributes = buf_p[17];

	targa_header.colormap_index  = LittleShort(targa_header.colormap_index);
	targa_header.colormap_length = LittleShort(targa_header.colormap_length);
	targa_header.x_origin        = LittleShort(targa_header.x_origin);
	targa_header.y_origin        = LittleShort(targa_header.y_origin);
	targa_header.width           = LittleShort(targa_header.width);
	targa_header.height          = LittleShort(targa_header.height);

	buf_p += 18;

	if (targa_header.image_type != 2
	    && targa_header.image_type != 10
	    && targa_header.image_type != 3)
	{
		ri.FS_FreeFile(buffer.v);
		Ren_Drop("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported(%s)\n", name);

	}

	if (targa_header.colormap_type != 0)
	{
		ri.FS_FreeFile(buffer.v);
		Ren_Drop("LoadTGA: colormaps not supported(%s)\n", name);
	}

	if ((targa_header.pixel_size != 32 && targa_header.pixel_size != 24) && targa_header.image_type != 3)
	{
		ri.FS_FreeFile(buffer.v);
		Ren_Drop("LoadTGA: Only 32 or 24 bit images supported (no colormaps)(%s)\n", name);
	}

	columns   = targa_header.width;
	rows      = targa_header.height;
	numPixels = columns * rows * 4;

	if (!columns || !rows || numPixels > 0x7FFFFFFF || numPixels / columns / 4 != rows)
	{
		ri.FS_FreeFile(buffer.v);
		Ren_Drop("LoadTGA: %s has an invalid image size\n", name);
	}

	targa_rgba = R_GetImageBuffer(numPixels, BUFFER_IMAGE, name);

	if (targa_header.id_length != 0)
	{
		if (buf_p + targa_header.id_length > end)
		{
			ri.Free(targa_rgba);
			ri.FS_FreeFile(buffer.v);
			Ren_Drop("LoadTGA: header too short (%s)\n", name);
		}

		buf_p += targa_header.id_length;  // skip TARGA image comment
	}

	if (targa_header.image_type == 2 || targa_header.image_type == 3)
	{
		if (buf_p + columns * rows * targa_header.pixel_size / 8 > end)
		{
			ri.Free(targa_rgba);
			ri.FS_FreeFile(buffer.v);
			Ren_Drop("LoadTGA: file truncated (%s)\n", name);
		}

		// Uncompressed RGB or gray scale image
		for (row = rows - 1; row != UINT_MAX; row--)
		{
			pixbuf = targa_rgba + row * columns * 4;
			for (column = 0; column < columns; column++)
			{
				unsigned char red, green, blue, alpha;
				switch (targa_header.pixel_size)
				{
				case 8:
					blue      = *buf_p++;
					green     = blue;
					red       = blue;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphaByte;
					break;
				case 24:
					blue      = *buf_p++;
					green     = *buf_p++;
					red       = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphaByte;
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
				default:
					ri.Free(targa_rgba);
					ri.FS_FreeFile(buffer.v);
					Ren_Drop("LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name);
				}
			}
		}
	}
	else if (targa_header.image_type == 10)   // Runlength encoded RGB images
	{
		unsigned char red, green, blue, alpha, packetHeader, packetSize, j;

		for (row = rows - 1; row != UINT_MAX; row--)
		{
			pixbuf = targa_rgba + row * columns * 4;
			for (column = 0; column < columns; )
			{
				if (buf_p + 1 > end)
				{
					Ren_Drop("LoadTGA: file truncated (%s)\n", name);
				}
				packetHeader = *buf_p++;
				packetSize   = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80)          // run-length packet
				{
					if (buf_p + targa_header.pixel_size / 8 > end)
					{
						Ren_Drop("LoadTGA: file truncated (%s)\n", name);
					}
					switch (targa_header.pixel_size)
					{
					case 24:
						blue  = *buf_p++;
						green = *buf_p++;
						red   = *buf_p++;
						alpha = alphaByte;
						break;
					case 32:
						blue  = *buf_p++;
						green = *buf_p++;
						red   = *buf_p++;
						alpha = *buf_p++;
						break;
					default:
						ri.Free(targa_rgba);
						ri.FS_FreeFile(buffer.v);
						Ren_Drop("LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name);
					}

					for (j = 0; j < packetSize; j++)
					{
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alpha;
						column++;
						if (column == columns)   // run spans across rows
						{
							column = 0;
							if (row > 0)
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
				else                              // non run-length packet
				{
					if (buf_p + targa_header.pixel_size / 8 * packetSize > end)
					{
						ri.Free(targa_rgba);
						ri.FS_FreeFile(buffer.v);
						Ren_Drop("LoadTGA: file truncated (%s)\n", name);
					}
					for (j = 0; j < packetSize; j++)
					{
						switch (targa_header.pixel_size)
						{
						case 24:
							blue      = *buf_p++;
							green     = *buf_p++;
							red       = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphaByte;
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
						default:
							ri.Free(targa_rgba);
							ri.FS_FreeFile(buffer.v);
							Ren_Drop("LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name);
						}
						column++;
						if (column == columns)   // pixel packet run spans across rows
						{
							column = 0;
							if (row > 0)
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
breakOut:   ;
		}
	}

#if 1
	// this is the chunk of code to ensure a behavior that meets TGA specs
	// bk0101024 - fix from Leonardo
	// bit 5 set => top-down
	if (targa_header.attributes & 0x20)
	{
		unsigned char *flip;
		unsigned char *src, *dst;

		//Ren_Warning( "WARNING: '%s' TGA file header declares top-down image, flipping\n", name);

		flip = (unsigned char *)ri.Hunk_AllocateTempMemory(columns * 4);
		for (row = 0; row < rows / 2; row++)
		{
			src = targa_rgba + row * 4 * columns;
			dst = targa_rgba + (rows - row - 1) * 4 * columns;

			Com_Memcpy(flip, src, columns * 4);
			Com_Memcpy(src, dst, columns * 4);
			Com_Memcpy(dst, flip, columns * 4);
		}
		ri.Hunk_FreeTempMemory(flip);
	}
#else
	// instead we just print a warning
	if (targa_header.attributes & 0x20)
	{
		Ren_Warning("WARNING: '%s' TGA file header declares top-down image, ignoring\n", name);
	}
#endif

	if (width)
	{
		*width = columns;
	}
	if (height)
	{
		*height = rows;
	}

	*pic = targa_rgba;

	ri.FS_FreeFile(buffer.v);
}

/**
 * @brief RE_SaveTGA
 * @param[in] filename
 * @param[in] data
 * @param[in] width
 * @param[in] height
 * @param[in] withAplha
 */
void RE_SaveTGA(const char *filename, byte *data, int width, int height, qboolean withAlpha)
{
	byte          *buffer;
	int           i, c;
	int           row;
	unsigned char *flip;
	unsigned char *src, *dst;

	buffer = (byte *)ri.Z_Malloc(width * height * 4 + 18);
	Com_Memset(buffer, 0, 18);
	buffer[2]  = 2;     // uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 32;    // pixel size

	// swap rgb to bgr
	c = 18 + width * height * 4;
	for (i = 18 ; i < c ; i += 4)
	{
		buffer[i]     = data[i - 18 + 2]; // blue
		buffer[i + 1] = data[i - 18 + 1]; // green
		buffer[i + 2] = data[i - 18 + 0]; // red

		if (withAlpha)
		{
			buffer[i + 3] = data[i - 18 + 3]; // alpha
		}
		else
		{
			buffer[i + 3] = 255; //data[i - 18 + 3]; // alpha
		}
	}

	// flip upside down
	flip = (unsigned char *)ri.Z_Malloc(width * 4);
	for (row = 0; row < height / 2; row++)
	{
		src = buffer + 18 + row * 4 * width;
		dst = buffer + 18 + (height - row - 1) * 4 * width;

		Com_Memcpy(flip, src, width * 4);
		Com_Memcpy(src, dst, width * 4);
		Com_Memcpy(dst, flip, width * 4);
	}

	ri.Free(flip);
	ri.FS_WriteFile(filename, buffer, c);
	ri.Free(buffer);
}
