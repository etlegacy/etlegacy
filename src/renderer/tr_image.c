/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
 * @file tr_image.c
 */

#include "tr_local.h"

static byte          s_intensitytable[256];
static unsigned char s_gammatable[256];

int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int gl_filter_max = GL_LINEAR;

#define FILE_HASH_SIZE      4096
static image_t *hashTable[FILE_HASH_SIZE];

// in order to prevent zone fragmentation, all images will
// be read into this buffer. In order to keep things as fast as possible,
// we'll give it a starting value, which will account for the majority of
// images, but allow it to grow if the buffer isn't big enough
#define R_IMAGE_BUFFER_SIZE     (512 * 512 * 4)       // 512 x 512 x 32bit

int  imageBufferSize[BUFFER_MAX_TYPES] = { 0, 0, 0 };
void *imageBufferPtr[BUFFER_MAX_TYPES] = { NULL, NULL, NULL };

void *R_GetImageBuffer(int size, bufferMemType_t bufferType)
{
	if (imageBufferSize[bufferType] < R_IMAGE_BUFFER_SIZE && size <= imageBufferSize[bufferType])
	{
		imageBufferSize[bufferType] = R_IMAGE_BUFFER_SIZE;
		imageBufferPtr[bufferType]  = malloc(imageBufferSize[bufferType]);
//DAJ TEST		imageBufferPtr[bufferType] = Z_Malloc( imageBufferSize[bufferType] );
	}
	if (size > imageBufferSize[bufferType])       // it needs to grow
	{
		if (imageBufferPtr[bufferType])
		{
			free(imageBufferPtr[bufferType]);
		}

		imageBufferSize[bufferType] = size;
		imageBufferPtr[bufferType]  = malloc(imageBufferSize[bufferType]);
	}
	if (!imageBufferPtr[bufferType])
	{
		ri.Error(ERR_DROP, "R_GetImageBuffer: unable to allocate buffer\n");
	}

	return imageBufferPtr[bufferType];
}

void R_FreeImageBuffer(void)
{
	int bufferType;

	for (bufferType = 0; bufferType < BUFFER_MAX_TYPES; bufferType++)
	{
		if (!imageBufferPtr[bufferType])
		{
			return;
		}
		free(imageBufferPtr[bufferType]);

		imageBufferSize[bufferType] = 0;
		imageBufferPtr[bufferType]  = NULL;
	}
}

/*
** R_GammaCorrect
*/
void R_GammaCorrect(byte *buffer, int bufSize)
{
	int i;

	for (i = 0; i < bufSize; i++)
	{
		buffer[i] = s_gammatable[buffer[i]];
	}
}

typedef struct
{
	char *name;
	int minimize, maximize;
} textureMode_t;

textureMode_t modes[] =
{
	{ "GL_NEAREST",                GL_NEAREST,                GL_NEAREST },
	{ "GL_LINEAR",                 GL_LINEAR,                 GL_LINEAR  },
	{ "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_NEAREST",  GL_LINEAR_MIPMAP_NEAREST,  GL_LINEAR  },
	{ "GL_NEAREST_MIPMAP_LINEAR",  GL_NEAREST_MIPMAP_LINEAR,  GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_LINEAR",   GL_LINEAR_MIPMAP_LINEAR,   GL_LINEAR  }
};

/*
================
return a hash value for the filename
================
*/
static long generateHashValue(const char *fname)
{
	int  i    = 0;
	long hash = 0;
	char letter;

	while (fname[i] != '\0')
	{
		letter = tolower(fname[i]);
		if (letter == '.')
		{
			break;                              // don't include extension
		}
		if (letter == '\\')
		{
			letter = '/';                       // damn path names
		}
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash &= (FILE_HASH_SIZE - 1);
	return hash;
}

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode(const char *string)
{
	int     i;
	image_t *glt;

	for (i = 0 ; i < 6 ; i++)
	{
		if (!Q_stricmp(modes[i].name, string))
		{
			break;
		}
	}

	if (i == 6)
	{
		ri.Printf(PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i = 0 ; i < tr.numImages ; i++)
	{
		glt = tr.images[i];
		if (glt->mipmap)
		{
			GL_Bind(glt);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages(void)
{
	int total = 0;
	int i;

	for (i = 0; i < tr.numImages; i++)
	{
		if (tr.images[i]->frameUsed == tr.frameCount)
		{
			total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f(void)
{
	int        i;
	image_t    *image;
	int        texels;
	const char *yesno[] =
	{
		"no ", "yes"
	};

	ri.Printf(PRINT_ALL, "\n      -w-- -h-- -mm- -TMU- -if-- wrap --name-------\n");
	texels = 0;

	for (i = 0 ; i < tr.numImages ; i++)
	{
		image = tr.images[i];

		texels += image->uploadWidth * image->uploadHeight;
		ri.Printf(PRINT_ALL, "%4i: %4i %4i  %s   %d   ",
		          i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap], image->TMU);
		switch (image->internalFormat)
		{
		case 1:
			ri.Printf(PRINT_ALL, "I    ");
			break;
		case 2:
			ri.Printf(PRINT_ALL, "IA   ");
			break;
		case 3:
			ri.Printf(PRINT_ALL, "RGB  ");
			break;
		case 4:
			ri.Printf(PRINT_ALL, "RGBA ");
			break;
		case GL_RGBA8:
			ri.Printf(PRINT_ALL, "RGBA8");
			break;
		case GL_RGB8:
			ri.Printf(PRINT_ALL, "RGB8");
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			ri.Printf(PRINT_ALL, "DXT3 ");
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			ri.Printf(PRINT_ALL, "DXT5 ");
			break;
		case GL_RGB4_S3TC:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			ri.Printf(PRINT_ALL, "S3TC ");
			break;
		case GL_RGBA4:
			ri.Printf(PRINT_ALL, "RGBA4");
			break;
		case GL_RGB5:
			ri.Printf(PRINT_ALL, "RGB5 ");
			break;
		default:
			ri.Printf(PRINT_ALL, "???? ");
		}

		switch (image->wrapClampMode)
		{
		case GL_REPEAT:
			ri.Printf(PRINT_ALL, "rept ");
			break;
		case GL_CLAMP_TO_EDGE:
			ri.Printf(PRINT_ALL, "clmp ");
			break;
		default:
			ri.Printf(PRINT_ALL, "%4i ", image->wrapClampMode);
			break;
		}

		ri.Printf(PRINT_ALL, " %s\n", image->imgName);
	}
	ri.Printf(PRINT_ALL, " ---------\n");
	ri.Printf(PRINT_ALL, " %i total texels (not including mipmaps)\n", texels);
	ri.Printf(PRINT_ALL, " %i total images\n\n", tr.numImages);
}

//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function
before or after.
================
*/
static void ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out,
                            int outwidth, int outheight)
{
	int      i, j;
	unsigned *inrow, *inrow2;
	unsigned frac, fracstep;
	unsigned p1[2048], p2[2048];
	byte     *pix1, *pix2, *pix3, *pix4;

	if (outwidth > 2048)
	{
		ri.Error(ERR_DROP, "ResampleTexture: max width");
	}

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for (i = 0 ; i < outwidth ; i++)
	{
		p1[i] = 4 * (frac >> 16);
		frac += fracstep;
	}
	frac = 3 * (fracstep >> 2);
	for (i = 0 ; i < outwidth ; i++)
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	for (i = 0 ; i < outheight ; i++, out += outwidth)
	{
		inrow  = in + inwidth * (int)((i + 0.25) * inheight / outheight);
		inrow2 = in + inwidth * (int)((i + 0.75) * inheight / outheight);
		frac   = fracstep >> 1;
		for (j = 0 ; j < outwidth ; j++)
		{
			pix1                   = (byte *)inrow + p1[j];
			pix2                   = (byte *)inrow + p2[j];
			pix3                   = (byte *)inrow2 + p1[j];
			pix4                   = (byte *)inrow2 + p2[j];
			((byte *)(out + j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			((byte *)(out + j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			((byte *)(out + j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			((byte *)(out + j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
		}
	}
}

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture(unsigned *in, int inwidth, int inheight, qboolean only_gamma)
{
	int  i, c;
	byte *p;

	if (only_gamma)
	{
		if (!glConfig.deviceSupportsGamma)
		{
			p = (byte *)in;

			c = inwidth * inheight;
			for (i = 0 ; i < c ; i++, p += 4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		p = (byte *)in;

		c = inwidth * inheight;

		if (glConfig.deviceSupportsGamma)
		{
			for (i = 0 ; i < c ; i++, p += 4)
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		}
		else
		{
			for (i = 0 ; i < c ; i++, p += 4)
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}

/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap2(unsigned *in, int inWidth, int inHeight)
{
	int      i, j, k;
	byte     *outpix;
	int      inWidthMask, inHeightMask;
	int      total;
	int      outWidth, outHeight;
	unsigned *temp;

	outWidth  = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp      = ri.Hunk_AllocateTempMemory(outWidth * outHeight * 4);

	inWidthMask  = inWidth - 1;
	inHeightMask = inHeight - 1;

	for (i = 0 ; i < outHeight ; i++)
	{
		for (j = 0 ; j < outWidth ; j++)
		{
			outpix = (byte *) (temp + i * outWidth + j);
			for (k = 0 ; k < 4 ; k++)
			{
				total =
				    1 * ((byte *)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
				    2 * ((byte *)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
				    2 * ((byte *)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
				    1 * ((byte *)&in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +

				    2 * ((byte *)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
				    4 * ((byte *)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
				    4 * ((byte *)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
				    2 * ((byte *)&in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +

				    2 * ((byte *)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
				    4 * ((byte *)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
				    4 * ((byte *)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
				    2 * ((byte *)&in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +

				    1 * ((byte *)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
				    2 * ((byte *)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
				    2 * ((byte *)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
				    1 * ((byte *)&in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k];
				outpix[k] = total / 36;
			}
		}
	}

	Com_Memcpy(in, temp, outWidth * outHeight * 4);
	ri.Hunk_FreeTempMemory(temp);
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap(byte *in, int width, int height)
{
	int  i, j;
	byte *out;
	int  row;

	if (!r_simpleMipMaps->integer)
	{
		R_MipMap2((unsigned *)in, width, height);
		return;
	}

	if (width == 1 && height == 1)
	{
		return;
	}

	row      = width * 4;
	out      = in;
	width  >>= 1;
	height >>= 1;

	if (width == 0 || height == 0)
	{
		width += height;    // get largest
		for (i = 0 ; i < width ; i++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4]) >> 1;
			out[1] = (in[1] + in[5]) >> 1;
			out[2] = (in[2] + in[6]) >> 1;
			out[3] = (in[3] + in[7]) >> 1;
		}
		return;
	}

	for (i = 0 ; i < height ; i++, in += row)
	{
		for (j = 0 ; j < width ; j++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4] + in[row + 0] + in[row + 4]) >> 2;
			out[1] = (in[1] + in[5] + in[row + 1] + in[row + 5]) >> 2;
			out[2] = (in[2] + in[6] + in[row + 2] + in[row + 6]) >> 2;
			out[3] = (in[3] + in[7] + in[row + 3] + in[row + 7]) >> 2;
		}
	}
}

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture(byte *data, int pixelCount, byte blend[4])
{
	int i;
	int inverseAlpha;
	int premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0]   = blend[0] * blend[3];
	premult[1]   = blend[1] * blend[3];
	premult[2]   = blend[2] * blend[3];

	for (i = 0 ; i < pixelCount ; i++, data += 4)
	{
		data[0] = (data[0] * inverseAlpha + premult[0]) >> 9;
		data[1] = (data[1] * inverseAlpha + premult[1]) >> 9;
		data[2] = (data[2] * inverseAlpha + premult[2]) >> 9;
	}
}

byte mipBlendColors[16][4] =
{
	{ 0,   0,   0,   0   },
	{ 255, 0,   0,   128 },
	{ 0,   255, 0,   128 },
	{ 0,   0,   255, 128 },
	{ 255, 0,   0,   128 },
	{ 0,   255, 0,   128 },
	{ 0,   0,   255, 128 },
	{ 255, 0,   0,   128 },
	{ 0,   255, 0,   128 },
	{ 0,   0,   255, 128 },
	{ 255, 0,   0,   128 },
	{ 0,   255, 0,   128 },
	{ 0,   0,   255, 128 },
	{ 255, 0,   0,   128 },
	{ 0,   255, 0,   128 },
	{ 0,   0,   255, 128 },
};

/*
===============
Upload32
===============
*/
static void Upload32(unsigned *data,
                     int width, int height,
                     qboolean mipmap,
                     qboolean picmip,
                     qboolean lightMap,
                     int *format,
                     int *pUploadWidth, int *pUploadHeight,
                     qboolean noCompress)
{
	int      samples;
	unsigned *scaledBuffer    = NULL;
	unsigned *resampledBuffer = NULL;
	int      scaled_width, scaled_height;
	int      c;
	byte     *scan;
	GLenum   internalFormat = GL_RGB;

	// convert to exact power of 2 sizes
	for (scaled_width = 1 ; scaled_width < width ; scaled_width <<= 1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height <<= 1)
		;
	if (r_roundImagesDown->integer && scaled_width > width)
	{
		scaled_width >>= 1;
	}
	if (r_roundImagesDown->integer && scaled_height > height)
	{
		scaled_height >>= 1;
	}

	if (scaled_width != width || scaled_height != height)
	{
		resampledBuffer = R_GetImageBuffer(scaled_width * scaled_height * 4, BUFFER_RESAMPLED);
		ResampleTexture(data, width, height, resampledBuffer, scaled_width, scaled_height);
		data   = resampledBuffer;
		width  = scaled_width;
		height = scaled_height;
	}

	// perform optional picmip operation
	if (picmip)
	{
		scaled_width  >>= r_picmip->integer;
		scaled_height >>= r_picmip->integer;
	}

	// clamp to minimum size
	if (scaled_width < 1)
	{
		scaled_width = 1;
	}
	if (scaled_height < 1)
	{
		scaled_height = 1;
	}

	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	while (scaled_width > glConfig.maxTextureSize
	       || scaled_height > glConfig.maxTextureSize)
	{
		scaled_width  >>= 1;
		scaled_height >>= 1;
	}

	//scaledBuffer = ri.Hunk_AllocateTempMemory( sizeof( unsigned ) * scaled_width * scaled_height );
	scaledBuffer = R_GetImageBuffer(sizeof(unsigned) * scaled_width * scaled_height, BUFFER_SCALED);

	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	c       = width * height;
	scan    = ((byte *)data);
	samples = 3;

	if (lightMap)
	{
		if (r_greyscale->integer)
		{
			internalFormat = GL_LUMINANCE;
		}
		else
		{
			internalFormat = GL_RGB;
		}
	}
	else
	{
		float rMax = 0, gMax = 0, bMax = 0;
		int   i;

		for (i = 0; i < c; i++)
		{
			if (scan[i * 4 + 0] > rMax)
			{
				rMax = scan[i * 4 + 0];
			}
			if (scan[i * 4 + 1] > gMax)
			{
				gMax = scan[i * 4 + 1];
			}
			if (scan[i * 4 + 2] > bMax)
			{
				bMax = scan[i * 4 + 2];
			}
			if (scan[i * 4 + 3] != 255)
			{
				samples = 4;
				break;
			}
		}
		// select proper internal format
		if (samples == 3)
		{
			if (r_greyscale->integer)
			{
				if (r_texturebits->integer == 16)
				{
					internalFormat = GL_LUMINANCE8;
				}
				else if (r_texturebits->integer == 32)
				{
					internalFormat = GL_LUMINANCE16;
				}
				else
				{
					internalFormat = GL_LUMINANCE;
				}
			}
			else
			{
				if (!noCompress && glConfig.textureCompression == TC_S3TC_ARB)
				{
					internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				}
				else if (!noCompress && glConfig.textureCompression == TC_S3TC)
				{
					internalFormat = GL_RGB4_S3TC;
				}
				else if (r_texturebits->integer == 16)
				{
					internalFormat = GL_RGB5;
				}
				else if (r_texturebits->integer == 32)
				{
					internalFormat = GL_RGB8;
				}
				else
				{
					internalFormat = GL_RGB;
				}
			}
		}
		else if (samples == 4)
		{
			if (r_greyscale->integer)
			{
				if (r_texturebits->integer == 16)
				{
					internalFormat = GL_LUMINANCE8_ALPHA8;
				}
				else if (r_texturebits->integer == 32)
				{
					internalFormat = GL_LUMINANCE16_ALPHA16;
				}
				else
				{
					internalFormat = GL_LUMINANCE_ALPHA;
				}
			}
			else
			{
				if (!noCompress && glConfig.textureCompression == TC_S3TC_ARB)
				{
					internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				}
				else if (r_texturebits->integer == 16)
				{
					internalFormat = GL_RGBA4;
				}
				else if (r_texturebits->integer == 32)
				{
					internalFormat = GL_RGBA8;
				}
				else
				{
					internalFormat = GL_RGBA;
				}
			}
		}
	}

	// copy or resample data as appropriate for first MIP level
	if ((scaled_width == width) &&
	    (scaled_height == height))
	{
		if (!mipmap)
		{
			qglTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			*pUploadWidth  = scaled_width;
			*pUploadHeight = scaled_height;
			*format        = internalFormat;

			goto done;
		}
		Com_Memcpy(scaledBuffer, data, width * height * 4);
	}
	else
	{
		// use the normal mip-mapping function to go down from here
		while (width > scaled_width || height > scaled_height)
		{
			R_MipMap((byte *)data, width, height);
			width  >>= 1;
			height >>= 1;
			if (width < 1)
			{
				width = 1;
			}
			if (height < 1)
			{
				height = 1;
			}
		}
		Com_Memcpy(scaledBuffer, data, width * height * 4);
	}

	R_LightScaleTexture(scaledBuffer, scaled_width, scaled_height, !mipmap);

	*pUploadWidth  = scaled_width;
	*pUploadHeight = scaled_height;
	*format        = internalFormat;

	qglTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);

	if (mipmap)
	{
		int miplevel = 0;

		while (scaled_width > 1 || scaled_height > 1)
		{
			R_MipMap((byte *)scaledBuffer, scaled_width, scaled_height);
			scaled_width  >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
			{
				scaled_width = 1;
			}
			if (scaled_height < 1)
			{
				scaled_height = 1;
			}
			miplevel++;

			if (r_colorMipLevels->integer)
			{
				R_BlendOverTexture((byte *)scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel]);
			}

			qglTexImage2D(GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
		}
	}
done:

	if (mipmap)
	{
		if (textureFilterAnisotropic)
		{
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
			                 (GLint)Com_Clamp(1, maxAnisotropy, r_ext_max_anisotropy->integer));
		}

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		if (textureFilterAnisotropic)
		{
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
		}

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	GL_CheckErrors();
}

/*
================
R_CreateImage

This is the only way any image_t are created
================
*/
image_t *R_CreateImage(const char *name, const byte *pic, int width, int height,
                       qboolean mipmap, qboolean allowPicmip, int glWrapClampMode)
{
	image_t  *image;
	qboolean isLightmap = qfalse;
	long     hash;
	qboolean noCompress = qfalse;

	if (strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "R_CreateImage: \"%s\" is too long\n", name);
	}
	if (!strncmp(name, "*lightmap", 9))
	{
		isLightmap = qtrue;
		noCompress = qtrue;
	}
	if (!noCompress && strstr(name, "skies"))
	{
		noCompress = qtrue;
	}
	if (!noCompress && strstr(name, "weapons"))          // don't compress view weapon skins
	{
		noCompress = qtrue;
	}
	// if the shader hasn't specifically asked for it, don't allow compression
	if (r_ext_compressed_textures->integer == 2 && (tr.allowCompress != qtrue))
	{
		noCompress = qtrue;
	}
	else if (r_ext_compressed_textures->integer == 1 && (tr.allowCompress < 0))
	{
		noCompress = qtrue;
	}
	// don't compress textures smaller or equal to 128x128 pixels
	else if ((width * height) <= (128 * 128))
	{
		noCompress = qtrue;
	}

	if (tr.numImages == MAX_DRAWIMAGES)
	{
		ri.Error(ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n");
	}

	image = tr.images[tr.numImages] = R_CacheImageAlloc(sizeof(image_t));

	// ok, let's try the recommended way
	qglGenTextures(1, &image->texnum);

	tr.numImages++;

	image->mipmap      = mipmap;
	image->allowPicmip = allowPicmip;

	strcpy(image->imgName, name);

	image->width         = width;
	image->height        = height;
	image->wrapClampMode = glWrapClampMode;

	// lightmaps are always allocated on TMU 1
	if (qglActiveTextureARB && isLightmap)
	{
		image->TMU = 1;
	}
	else
	{
		image->TMU = 0;
	}

	if (qglActiveTextureARB)
	{
		GL_SelectTexture(image->TMU);
	}

	GL_Bind(image);

	Upload32((unsigned *)pic, image->width, image->height,
	         image->mipmap,
	         allowPicmip,
	         isLightmap,
	         &image->internalFormat,
	         &image->uploadWidth,
	         &image->uploadHeight,
	         noCompress);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode);

	qglBindTexture(GL_TEXTURE_2D, 0);

	if (image->TMU == 1)
	{
		GL_SelectTexture(0);
	}

	hash            = generateHashValue(name);
	image->next     = hashTable[hash];
	hashTable[hash] = image;

	image->hash = hash;

	return image;
}

//===================================================================

typedef struct
{
	char *ext;
	void (*ImageLoader)(const char *, unsigned char **, int *, int *, byte);
} imageExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple images of different formats available
static imageExtToLoaderMap_t imageLoaders[] =
{
	{ "tga",  R_LoadTGA },
	{ "jpg",  R_LoadJPG },
	{ "jpeg", R_LoadJPG },
	{ "png",  R_LoadPNG },
	{ "pcx",  R_LoadPCX },
	{ "bmp",  R_LoadBMP }
};

static int numImageLoaders = sizeof(imageLoaders) / sizeof(imageLoaders[0]);

/**
 * @brief Loads any of the supported image types into a cannonical 32 bit format.
 */
void R_LoadImage(const char *name, byte **pic, int *width, int *height)
{
	int  i;
	char localName[MAX_QPATH];
	char *altName;

	*pic    = NULL;
	*width  = 0;
	*height = 0;

	Q_strncpyz(localName, name, MAX_QPATH);

	COM_StripExtension(name, localName, MAX_QPATH);

	// Try and find a suitable match using all the image formats supported
	// Searching is done in this order: tga, jp(e)g, png, pcx, bmp
	for (i = 0; i < numImageLoaders; i++)
	{
		altName = va("%s.%s", localName, imageLoaders[i].ext);

		// Check if file exists
		if (ri.FS_FOpenFileRead(altName, NULL, qfalse))
		{
			// Load
			imageLoaders[i].ImageLoader(altName, pic, width, height, 0xFF);
		}

		if (*pic)
		{
			break;
		}
	}

	if (*pic == NULL)
	{
		// Loader failed, most likely because the file isn't there
		ri.Printf(PRINT_DEVELOPER, "WARNING: %s not present in any supported image format\n", localName);
	}
}

/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t *R_FindImageFile(const char *name, qboolean mipmap, qboolean allowPicmip, int glWrapClampMode, qboolean lightmap)
{
	image_t  *image;
	int      width, height;
	byte     *pic;
	long     hash;
	qboolean allowCompress = qfalse;

	if (!name)
	{
		return NULL;
	}

	hash = generateHashValue(name);

	// caching
	if (r_cacheGathering->integer)
	{
		ri.Cmd_ExecuteText(EXEC_NOW, va("cache_usedfile image %s %i %i %i\n", name, mipmap, allowPicmip, glWrapClampMode));
	}

	// see if the image is already loaded
	for (image = hashTable[hash]; image; image = image->next)
	{
		if (!strcmp(name, image->imgName))
		{
			// the white image can be used with any set of parms, but other mismatches are errors
			if (strcmp(name, "*white"))
			{
				if (image->mipmap != mipmap)
				{
					ri.Printf(PRINT_DEVELOPER, "WARNING: reused image %s with mixed mipmap parm\n", name);
				}
				if (image->allowPicmip != allowPicmip)
				{
					ri.Printf(PRINT_DEVELOPER, "WARNING: reused image %s with mixed allowPicmip parm\n", name);
				}
				if (image->wrapClampMode != glWrapClampMode)
				{
					ri.Printf(PRINT_ALL, "WARNING: reused image %s with mixed glWrapClampMode parm\n", name);
				}
			}
			return image;
		}
	}

	// check the cache
	// - assignment used as truth value
	// - don't do this for lightmaps
	if (!lightmap)
	{
		image = R_FindCachedImage(name, hash);
		if (image != NULL)
		{
			return image;
		}
	}

	// load the pic from disk
	R_LoadImage(name, &pic, &width, &height);
	if (pic == NULL)
	{
		return NULL;
	}

	// apply lightmap colouring
	if (lightmap)
	{
		R_ProcessLightmap(pic, 4, width, height, pic);

		// no texture compression
		if (lightmap)
		{
			allowCompress = tr.allowCompress;
		}
		tr.allowCompress = -1;
	}

	if (((width - 1) & width) || ((height - 1) & height))
	{
		Com_Printf("^1Image not power of 2 scaled: %s\n", name);
		return NULL;
	}

	image = R_CreateImage(( char * ) name, pic, width, height, mipmap, allowPicmip, glWrapClampMode);

	// no texture compression
	if (lightmap)
	{
		tr.allowCompress = allowCompress;
	}

	return image;
}

/*
================
R_CreateDlightImage
================
*/
#define DLIGHT_SIZE 16
static void R_CreateDlightImage(void)
{
	float d;
	int   x, y, b;
	byte  data[DLIGHT_SIZE][DLIGHT_SIZE][4];

	// make a centered inverse-square falloff blob for dynamic lighting
	for (x = 0 ; x < DLIGHT_SIZE ; x++)
	{
		for (y = 0 ; y < DLIGHT_SIZE ; y++)
		{
			d = (DLIGHT_SIZE / 2 - 0.5f - x) * (DLIGHT_SIZE / 2 - 0.5f - x) +
			    (DLIGHT_SIZE / 2 - 0.5f - y) * (DLIGHT_SIZE / 2 - 0.5f - y);
			b = 4000 / d;
			if (b > 255)
			{
				b = 255;
			}
			else if (b < 75)
			{
				b = 0;
			}
			data[y][x][0]         =
			    data[y][x][1]     =
			        data[y][x][2] = b;
			data[y][x][3]         = 255;
		}
	}
	tr.dlightImage = R_CreateImage("*dlight", (byte *)data, DLIGHT_SIZE, DLIGHT_SIZE, qfalse, qfalse, GL_CLAMP_TO_EDGE);
}

/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable(void)
{
	int   i;
	float d;
	float exp = 0.5f;

	for (i = 0 ; i < FOG_TABLE_SIZE ; i++)
	{
		d = pow((float)i / (FOG_TABLE_SIZE - 1), exp);

		tr.fogTable[i] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float R_FogFactor(float s, float t)
{
	float d;

	s -= 1.0 / 512;
	if (s < 0)
	{
		return 0;
	}
	if (t < 1.0 / 32)
	{
		return 0;
	}
	if (t < 31.0 / 32)
	{
		s *= (t - 1.0f / 32.0f) / (30.0f / 32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if (s > 1.0)
	{
		s = 1.0;
	}

	d = tr.fogTable[(int)(s * (FOG_TABLE_SIZE - 1))];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define FOG_S       16
#define FOG_T       16  //  used to be 32

static void R_CreateFogImage(void)
{
	int   x, y, alpha;
	byte  *data;
	float borderColor[4];

	// allocate table for image
	data = ri.Hunk_AllocateTempMemory(FOG_S * FOG_T * 4);

	// new, linear fog texture generating algo for GL_CLAMP_TO_EDGE (OpenGL 1.2+)

	// S is distance, T is depth
	for (x = 0 ; x < FOG_S ; x++)
	{
		for (y = 0 ; y < FOG_T ; y++)
		{
			alpha = 270 * ((float) x / FOG_S) * ((float) y / FOG_T);        // need slop room for fp round to 0
			if (alpha < 0)
			{
				alpha = 0;
			}
			else if (alpha > 255)
			{
				alpha = 255;
			}

			// ensure edge/corner cases are fully transparent (at 0,0) or fully opaque (at 1,N where N is 0-1.0)
			if (x == 0)
			{
				alpha = 0;
			}
			else if (x == (FOG_S - 1))
			{
				alpha = 255;
			}

			data[(y * FOG_S + x) * 4 + 0]         =
			    data[(y * FOG_S + x) * 4 + 1]     =
			        data[(y * FOG_S + x) * 4 + 2] = 255;
			data[(y * FOG_S + x) * 4 + 3]         = alpha; //%	255*d;
		}
	}

	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", (byte *)data, FOG_S, FOG_T, qfalse, qfalse, GL_CLAMP_TO_EDGE);
	ri.Hunk_FreeTempMemory(data);

	// FIXME: the following lines are unecessary for new GL_CLAMP_TO_EDGE fog (?)
	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
}

/*
==================
R_CreateDefaultImage
==================
*/
#define DEFAULT_SIZE    16
static void R_CreateDefaultImage(void)
{
	int  x, y;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset(data, 0, sizeof(data));
	for (x = 0 ; x < DEFAULT_SIZE ; x++)
	{
		for (y = 0 ; y < 2; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 128;
			data[y][x][2] = 0;
			data[y][x][3] = 255;

			data[x][y][0] = 255;
			data[x][y][1] = 128;
			data[x][y][2] = 0;
			data[x][y][3] = 255;

			data[DEFAULT_SIZE - 1 - y][x][0] = 255;
			data[DEFAULT_SIZE - 1 - y][x][1] = 128;
			data[DEFAULT_SIZE - 1 - y][x][2] = 0;
			data[DEFAULT_SIZE - 1 - y][x][3] = 255;

			data[x][DEFAULT_SIZE - 1 - y][0] = 255;
			data[x][DEFAULT_SIZE - 1 - y][1] = 128;
			data[x][DEFAULT_SIZE - 1 - y][2] = 0;
			data[x][DEFAULT_SIZE - 1 - y][3] = 255;
		}
	}
	tr.defaultImage = R_CreateImage("*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qfalse, GL_REPEAT);
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages(void)
{
	int  x, y;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset(data, 255, sizeof(data));
	tr.whiteImage = R_CreateImage("*white", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT);

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x = 0 ; x < DEFAULT_SIZE ; x++)
	{
		for (y = 0 ; y < DEFAULT_SIZE ; y++)
		{
			data[y][x][0]         =
			    data[y][x][1]     =
			        data[y][x][2] = tr.identityLightByte;
			data[y][x][3]         = 255;
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT);

	for (x = 0; x < 32; x++)
	{
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage("*scratch", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qtrue, GL_CLAMP_TO_EDGE);
	}

	R_CreateDlightImage();
	R_CreateFogImage();
}

/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings(void)
{
	int   i, j;
	float g;
	int   inf;
	int   shift;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if (!glConfig.deviceSupportsGamma)
	{
		tr.overbrightBits = 0;      // need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if (!glConfig.isFullscreen)
	{
		tr.overbrightBits = 0;
	}

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if (glConfig.colorBits > 16)
	{
		if (tr.overbrightBits > 2)
		{
			tr.overbrightBits = 2;
		}
	}
	else
	{
		if (tr.overbrightBits > 1)
		{
			tr.overbrightBits = 1;
		}
	}
	if (tr.overbrightBits < 0)
	{
		tr.overbrightBits = 0;
	}

	tr.identityLight     = 1.0f / (1 << tr.overbrightBits);
	tr.identityLightByte = 255 * tr.identityLight;


	if (r_intensity->value <= 1)
	{
		ri.Cvar_Set("r_intensity", "1");
	}

	if (r_gamma->value < 0.5f)
	{
		ri.Cvar_Set("r_gamma", "0.5");
	}
	else if (r_gamma->value > 3.0f)
	{
		ri.Cvar_Set("r_gamma", "3.0");
	}

	g = r_gamma->value;

	shift = tr.overbrightBits;

	for (i = 0; i < 256; i++)
	{
		if (g == 1)
		{
			inf = i;
		}
		else
		{
			inf = 255 * pow(i / 255.0f, 1.0f / g) + 0.5f;
		}
		inf <<= shift;
		if (inf < 0)
		{
			inf = 0;
		}
		if (inf > 255)
		{
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	for (i = 0 ; i < 256 ; i++)
	{
		j = i * r_intensity->value;
		if (j > 255)
		{
			j = 255;
		}
		s_intensitytable[i] = j;
	}

	if (glConfig.deviceSupportsGamma)
	{
		GLimp_SetGamma(s_gammatable, s_gammatable, s_gammatable);
	}
}

/*
===============
R_InitImages
===============
*/
void R_InitImages(void)
{
	Com_Memset(hashTable, 0, sizeof(hashTable));
	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();

	// load the cache media, if they were loaded previously, they'll be restored from the backupImages
	R_LoadCacheImages();
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures(void)
{
	int i;

	for (i = 0; i < tr.numImages ; i++)
	{
		qglDeleteTextures(1, &tr.images[i]->texnum);
	}
	Com_Memset(tr.images, 0, sizeof(tr.images));

	tr.numImages = 0;

	Com_Memset(glState.currenttextures, 0, sizeof(glState.currenttextures));
	if (qglActiveTextureARB)
	{
		GL_SelectTexture(1);
		qglBindTexture(GL_TEXTURE_2D, 0);
		GL_SelectTexture(0);
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
}

/*
============================================================================
SKINS
============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse(char **data_p)
{
	int         c = 0, len = 0;
	char        *data;
	static char com_token[MAX_TOKEN_CHARS];

	data         = *data_p;
	com_token[0] = 0;

	// make sure incoming data is valid
	if (!data)
	{
		*data_p = NULL;
		return com_token;
	}

	while (1)
	{
		// skip whitespace
		while ((c = *data) <= ' ')
		{
			if (!c)
			{
				break;
			}
			data++;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			while (*data && (*data != '*' || data[1] != '/'))
			{
				data++;
			}
			if (*data)
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if (c == 0)
	{
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p        = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	}
	while (c > 32 && c != ',');

	if (len == MAX_TOKEN_CHARS)
	{
		//Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

/*
==============
RE_GetSkinModel
==============
*/
qboolean RE_GetSkinModel(qhandle_t skinid, const char *type, char *name)
{
	int    i;
	int    hash;
	skin_t *skin;

	skin = tr.skins[skinid];
	hash = Com_HashKey((char *)type, strlen(type));

	for (i = 0; i < skin->numModels; i++)
	{
		if (hash != skin->models[i]->hash)
		{
			continue;
		}
		if (!Q_stricmp(skin->models[i]->type, type))
		{
			// (SA) whoops, should've been this way
			Q_strncpyz(name, skin->models[i]->model, sizeof(skin->models[i]->model));
			return qtrue;
		}
	}
	return qfalse;
}

/*
==============
RE_GetShaderFromModel
    return a shader index for a given model's surface
    'withlightmap' set to '0' will create a new shader that is a copy of the one found
    on the model, without the lighmap stage, if the shader has a lightmap stage

    NOTE: only works for bmodels right now.  Could modify for other models (md3's etc.)
==============
*/
qhandle_t RE_GetShaderFromModel(qhandle_t modelid, int surfnum, int withlightmap)
{
	model_t *model;

	if (surfnum < 0)
	{
		surfnum = 0;
	}

	model = R_GetModelByHandle(modelid);    // (SA) should be correct now

	if (model)
	{
		bmodel_t *bmodel = model->model.bmodel;

		if (bmodel && bmodel->firstSurface)
		{
			msurface_t *surf;
			shader_t   *shd;

			if (surfnum >= bmodel->numSurfaces)     // if it's out of range, return the first surface
			{
				surfnum = 0;
			}

			surf = bmodel->firstSurface + surfnum;
			// check for null shader (can happen on func_explosive's with botclips attached)
			if (!surf->shader)
			{
				return 0;
			}

			if (surf->shader->lightmapIndex > LIGHTMAP_NONE)
			{
				image_t  *image;
				long     hash;
				qboolean mip = qtrue;   // mip generation on by default

				// get mipmap info for original texture
				hash = generateHashValue(surf->shader->name);
				for (image = hashTable[hash]; image; image = image->next)
				{
					if (!strcmp(surf->shader->name, image->imgName))
					{
						mip = image->mipmap;
						break;
					}
				}
				shd                    = R_FindShader(surf->shader->name, LIGHTMAP_NONE, mip);
				shd->stages[0]->rgbGen = CGEN_LIGHTING_DIFFUSE;
			}
			else
			{
				shd = surf->shader;
			}

			return shd->index;
		}
	}

	return 0;
}

/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin(const char *name)
{
	qhandle_t     hSkin;
	skin_t        *skin;
	skinModel_t   *model;
	skinSurface_t *surf;
	union
	{
		char *c;
		void *v;
	} text;
	char *text_p;
	char *token;
	char surfName[MAX_QPATH];

	if (!name || !name[0])
	{
		Com_Printf("Empty name passed to RE_RegisterSkin\n");
		return 0;
	}

	if (strlen(name) >= MAX_QPATH)
	{
		Com_Printf("Skin name exceeds MAX_QPATH\n");
		return 0;
	}

	// see if the skin is already loaded
	for (hSkin = 1; hSkin < tr.numSkins ; hSkin++)
	{
		skin = tr.skins[hSkin];
		if (!Q_stricmp(skin->name, name))
		{
			if (skin->numSurfaces == 0)
			{
				return 0;       // default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if (tr.numSkins == MAX_SKINS)
	{
		ri.Printf(PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name);
		return 0;
	}
	tr.numSkins++;
	skin            = ri.Hunk_Alloc(sizeof(skin_t), h_low);
	tr.skins[hSkin] = skin;
	Q_strncpyz(skin->name, name, sizeof(skin->name));
	skin->numSurfaces = 0;

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	// If not a .skin file, load as a single shader
	// HACK: ET evilly has filenames slightly longer than MAX_QPATH
	// this check breaks the loading of such skins
	/*
	if ( strcmp( name + strlen( name ) - 5, ".skin" ) ) {
	    skin->numSurfaces = 1;
	    skin->surfaces[0] = ri.Hunk_Alloc( sizeof(skin->surfaces[0]), h_low );
	    skin->surfaces[0]->shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
	    return hSkin;
	}
	*/

	// load and parse the skin file
	text.v = NULL;
	if (ri.FS_FOpenFileRead(name, NULL, qfalse))
	{
		ri.FS_ReadFile(name, &text.v);
	}

	if (!text.c)
	{
		return 0;
	}

	text_p = text.c;
	while (text_p && *text_p)
	{
		// get surface name
		token = CommaParse(&text_p);
		Q_strncpyz(surfName, token, sizeof(surfName));

		if (!token[0])
		{
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr(surfName);

		if (*text_p == ',')
		{
			text_p++;
		}

		if (strstr(token, "tag_"))
		{
			continue;
		}

		if (strstr(token, "md3_"))
		{
			// this is specifying a model
			model = skin->models[skin->numModels] = ri.Hunk_Alloc(sizeof(*skin->models[0]), h_low);
			Q_strncpyz(model->type, token, sizeof(model->type));
			model->hash = Com_HashKey(model->type, sizeof(model->type));

			// get the model name
			token = CommaParse(&text_p);

			Q_strncpyz(model->model, token, sizeof(model->model));

			skin->numModels++;
			continue;
		}

		// parse the shader name
		token = CommaParse(&text_p);

		surf = skin->surfaces[skin->numSurfaces] = ri.Hunk_Alloc(sizeof(*skin->surfaces[0]), h_low);
		Q_strncpyz(surf->name, surfName, sizeof(surf->name));
		surf->hash   = Com_HashKey(surf->name, sizeof(surf->name));
		surf->shader = R_FindShader(token, LIGHTMAP_NONE, qtrue);
		skin->numSurfaces++;
	}

	ri.FS_FreeFile(text.v);

	// never let a skin have 0 shaders
	if (skin->numSurfaces == 0)
	{
		return 0;       // use default skin
	}

	return hSkin;
}

/*
===============
R_InitSkins
===============
*/
void R_InitSkins(void)
{
	skin_t *skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = ri.Hunk_Alloc(sizeof(skin_t), h_low);
	Q_strncpyz(skin->name, "<default skin>", sizeof(skin->name));
	skin->numSurfaces         = 1;
	skin->surfaces[0]         = ri.Hunk_Alloc(sizeof(*skin->surfaces), h_low);
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t *R_GetSkinByHandle(qhandle_t hSkin)
{
	if (hSkin < 1 || hSkin >= tr.numSkins)
	{
		return tr.skins[0];
	}
	return tr.skins[hSkin];
}

/*
===============
R_SkinList_f
===============
*/
void R_SkinList_f(void)
{
	int    i, j;
	skin_t *skin;

	ri.Printf(PRINT_ALL, "------------------\n");

	for (i = 0 ; i < tr.numSkins ; i++)
	{
		skin = tr.skins[i];

		ri.Printf(PRINT_ALL, "%3i:%s\n", i, skin->name);
		for (j = 0 ; j < skin->numSurfaces ; j++)
		{
			ri.Printf(PRINT_ALL, "       %s = %s\n",
			          skin->surfaces[j]->name, skin->surfaces[j]->shader->name);
		}
	}
	ri.Printf(PRINT_ALL, "------------------\n");
}

//==========================================================================================
// caching system

static int     numBackupImages = 0;
static image_t *backupHashTable[FILE_HASH_SIZE];

/*
===============
R_CacheImageAlloc

  this will only get called to allocate the image_t structures, not that actual image pixels
===============
*/
void *R_CacheImageAlloc(int size)
{
	if (r_cache->integer && r_cacheShaders->integer)
	{
		void *buf = malloc(size);    // ri.Z_Malloc causes load times about twice as long?
		if (!buf)
		{
			ri.Error(ERR_DROP, "R_CacheImageAlloc: unable to allocate buffer\n ");
		}
		return buf;
	}
	else
	{
		return ri.Hunk_Alloc(size, h_low);
	}
}

/*
===============
R_CacheImageFree
===============
*/
void R_CacheImageFree(void *ptr)
{
	if (r_cache->integer && r_cacheShaders->integer)
	{
		free(ptr);
	}
}

/*
===============
R_TouchImage

  remove this image from the backupHashTable and make sure it doesn't get overwritten
===============
*/
qboolean R_TouchImage(image_t *inImage)
{
	image_t *bImage, *bImagePrev;
	int     hash;
	//char    *name;

	if (inImage == tr.dlightImage ||
	    inImage == tr.whiteImage ||
	    inImage == tr.defaultImage ||
	    inImage->imgName[0] == '*')     // can't use lightmaps since they might have the same name, but different maps will have different actual lightmap pixels
	{
		return qfalse;
	}

	hash = inImage->hash;
	//name = inImage->imgName;

	bImage     = backupHashTable[hash];
	bImagePrev = NULL;
	while (bImage)
	{
		if (bImage == inImage)
		{
			// add it to the current images
			if (tr.numImages == MAX_DRAWIMAGES)
			{
				ri.Error(ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n");
			}

			tr.images[tr.numImages] = bImage;

			// remove it from the backupHashTable
			if (bImagePrev)
			{
				bImagePrev->next = bImage->next;
			}
			else
			{
				backupHashTable[hash] = bImage->next;
			}

			// add it to the hashTable
			bImage->next    = hashTable[hash];
			hashTable[hash] = bImage;

			// get the new texture
			tr.numImages++;

			return qtrue;
		}

		bImagePrev = bImage;
		bImage     = bImage->next;
	}

	return qtrue;
}

/*
===============
R_PurgeImage
===============
*/
void R_PurgeImage(image_t *image)
{
	qglDeleteTextures(1, &image->texnum);

	R_CacheImageFree(image);

	Com_Memset(glState.currenttextures, 0, sizeof(glState.currenttextures));
	if (qglActiveTextureARB)
	{
		GL_SelectTexture(1);
		qglBindTexture(GL_TEXTURE_2D, 0);
		GL_SelectTexture(0);
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
}

/*
===============
R_PurgeBackupImages

  Can specify the number of Images to purge this call (used for background purging)
===============
*/
void R_PurgeBackupImages(int purgeCount)
{
	int        i, cnt;
	static int lastPurged = 0;
	image_t    *image;

	if (!numBackupImages)
	{
		// nothing to purge
		lastPurged = 0;
		return;
	}

	R_IssuePendingRenderCommands();

	cnt = 0;
	for (i = lastPurged; i < FILE_HASH_SIZE; )
	{
		lastPurged = i;
		// assignment used as truth value
		if ((image = backupHashTable[i]))
		{
			// kill it
			backupHashTable[i] = image->next;
			R_PurgeImage(image);
			cnt++;

			if (cnt >= purgeCount)
			{
				return;
			}
		}
		else
		{
			i++;    // no images in this slot, so move to the next one
		}
	}

	// all done
	numBackupImages = 0;
	lastPurged      = 0;
}

/*
===============
R_BackupImages
===============
*/
void R_BackupImages(void)
{
	if (!r_cache->integer)
	{
		return;
	}
	if (!r_cacheShaders->integer)
	{
		return;
	}

	// backup the hashTable
	Com_Memcpy(backupHashTable, hashTable, sizeof(backupHashTable));

	// pretend we have cleared the list
	numBackupImages = tr.numImages;
	tr.numImages    = 0;

	Com_Memset(glState.currenttextures, 0, sizeof(glState.currenttextures));
	if (qglActiveTextureARB)
	{
		GL_SelectTexture(1);
		qglBindTexture(GL_TEXTURE_2D, 0);
		GL_SelectTexture(0);
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
}

/*
=============
R_FindCachedImage
=============
*/
image_t *R_FindCachedImage(const char *name, int hash)
{
	image_t *bImage;

	if (!r_cacheShaders->integer)
	{
		return NULL;
	}

	if (!numBackupImages)
	{
		return NULL;
	}

	bImage = backupHashTable[hash];

	while (bImage)
	{

		if (!Q_stricmp(name, bImage->imgName))
		{
			// add it to the current images
			if (tr.numImages == MAX_DRAWIMAGES)
			{
				ri.Error(ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n");
			}

			R_TouchImage(bImage);
			return bImage;
		}

		bImage = bImage->next;
	}

	return NULL;
}

/*
R_GetTextureId
*/
int R_GetTextureId(const char *name)
{
	int i;

	//	ri.Printf( PRINT_ALL, "R_GetTextureId [%s].\n", name );
	for (i = 0 ; i < tr.numImages ; i++)
	{
		if (!strcmp(name, tr.images[i]->imgName))
		{
			//ri.Printf( PRINT_ALL, "Found textureid %d\n", i );
			return i;
		}
	}

	//ri.Printf( PRINT_ALL, "Image not found.\n" );
	return -1;
}

/*
===============
R_LoadCacheImages
===============
*/
void R_LoadCacheImages(void)
{
	int  len;
	char *buf;
	char *token, *pString;
	char name[MAX_QPATH];
	int  parms[4], i;

	if (numBackupImages)
	{
		return;
	}

	len = ri.FS_ReadFile("image.cache", NULL);

	if (len <= 0)
	{
		return;
	}

	buf = (char *)ri.Hunk_AllocateTempMemory(len);
	ri.FS_ReadFile("image.cache", (void **)&buf);
	pString = buf;

	while ((token = COM_ParseExt(&pString, qtrue)) && token[0])
	{
		Q_strncpyz(name, token, sizeof(name));
		for (i = 0; i < 4; i++)
		{
			token    = COM_ParseExt(&pString, qfalse);
			parms[i] = atoi(token);
		}
		R_FindImageFile(name, parms[0], parms[1], parms[2], parms[3]);
	}

	ri.Hunk_FreeTempMemory(buf);
}
