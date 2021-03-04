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
 * @file rendererGLES/tr_image.c
 */

#include "tr_local.h"

static byte          s_intensitytable[256];
static unsigned char s_gammatable[256];

int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int gl_filter_max = GL_LINEAR;

#define FILE_HASH_SIZE      4096
static image_t *hashTable[FILE_HASH_SIZE];
#define generateHashValue(fname) Q_GenerateHashValue(fname, FILE_HASH_SIZE, qfalse, qtrue);

// in order to prevent zone fragmentation, all images will
// be read into this buffer. In order to keep things as fast as possible,
// we'll give it a starting value, which will account for the majority of
// images, but allow it to grow if the buffer isn't big enough
// note: this image cache is bypassing the common q3 memory! -> the game isn't using hunk and zone memory only !!!
#define R_IMAGE_BUFFER_SIZE     (512 * 512 * 4)

int  imageBufferSize[BUFFER_MAX_TYPES] = { 0, 0, 0 };
void *imageBufferPtr[BUFFER_MAX_TYPES] = { NULL, NULL, NULL };

/**
 * @brief R_GetImageBuffer
 * @param[in] size
 * @param[in] bufferType
 * @param[in] filename
 * @return
 */
void *R_GetImageBuffer(int size, bufferMemType_t bufferType, const char *filename)
{
	if (imageBufferSize[bufferType] < R_IMAGE_BUFFER_SIZE && size <= imageBufferSize[bufferType])
	{
		imageBufferSize[bufferType] = R_IMAGE_BUFFER_SIZE;
		imageBufferPtr[bufferType]  = Com_Allocate(imageBufferSize[bufferType]);
	}
	if (size > imageBufferSize[bufferType])       // it needs to grow
	{
		if (imageBufferPtr[bufferType])
		{
			Com_Dealloc(imageBufferPtr[bufferType]);
		}

		imageBufferSize[bufferType] = size;
		imageBufferPtr[bufferType]  = Com_Allocate(imageBufferSize[bufferType]);
	}

	if (!imageBufferPtr[bufferType])
	{
		Ren_Drop("R_GetImageBuffer: unable to allocate buffer for image %s with size: %i\n", filename, size);
	}

	return imageBufferPtr[bufferType];
}

/**
 * @brief R_FreeImageBuffer
 */
void R_FreeImageBuffer(void)
{
	int bufferType;

	for (bufferType = 0; bufferType < BUFFER_MAX_TYPES; bufferType++)
	{
		if (!imageBufferPtr[bufferType])
		{
			continue;
		}
		Com_Dealloc(imageBufferPtr[bufferType]);

		imageBufferSize[bufferType] = 0;
		imageBufferPtr[bufferType]  = NULL;
	}
}

/**
 * @brief R_GammaCorrect
 * @param[in,out] buffer
 * @param[in] bufSize
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
	const char *name;
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

/**
 * @brief GL_TextureMode
 * @param[in] string
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
		Ren_Print("bad filter name\n");
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

/**
 * @brief R_SumOfUsedImages
 * @return
 *
 * @todo FIXME: add this to R_ImageList_f output?
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

/**
 * @brief R_ImageList_f
 */
void R_ImageList_f(void)
{
	int        i;
	image_t    *image;
	int        texels   = 0;
	const char *yesno[] =
	{
		"no ", "yes"
	};

	Ren_Print("\n      -w-- -h-- -mm- -TMU- -if-- wrap --name-------\n");

	for (i = 0 ; i < tr.numImages ; i++)
	{
		image = tr.images[i];

		texels += image->uploadWidth * image->uploadHeight;
		Ren_Print("%4i: %4i %4i  %s   %d   ",
		          i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap], image->TMU);
		switch (image->internalFormat)
		{
		case 1:
			Ren_Print("I    ");
			break;
		case 2:
			Ren_Print("IA   ");
			break;
		case 3:
			Ren_Print("RGB  ");
			break;
		case 4:
			Ren_Print("RGBA ");
			break;
		case GL_RGB8_OES:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			Ren_Print("S3TC ");
			break;
		case GL_RGBA4:
			Ren_Print("RGBA4");
			break;
		case GL_RGB5:
			Ren_Print("RGB5 ");
			break;
		default:
			Ren_Print("???? ");
			break;
		}

		switch (image->wrapClampMode)
		{
		case GL_REPEAT:
			Ren_Print("rept ");
			break;
		case GL_CLAMP_TO_EDGE:
			Ren_Print("clmp ");
			break;
		default:
			Ren_Print("%4i ", image->wrapClampMode);
			break;
		}

		Ren_Print(" %s\n", image->imgName);
	}
	Ren_Print(" ---------\n");
	Ren_Print(" %i total texels (not including mipmaps)\n", texels);
	Ren_Print(" %i total images\n\n", tr.numImages);
}

//=======================================================================

/**
 * @brief Used to resample images in a more general than quartering fashion.
 *
 * This will only be filtered properly if the resampled size
 * is greater than half the original size.
 *
 * If a larger shrinking is needed, use the mipmap function
 * before or after.
 *
 * @param[in] in
 * @param[in] inwidth
 * @param[in] inheight
 * @param[out] out
 * @param[in] outwidth
 * @param[in] outheight
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
		Ren_Drop("ResampleTexture: max width");
	}

	if (outwidth < 1)
	{
		outwidth = 1;
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
		//frac   = fracstep >> 1;   // FIXME: never read
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

/**
 * @brief Scale up the pixel values in a texture to increase the
 * lighting range
 * @param[in] in
 * @param[in] inwidth
 * @param[in] inheight
 * @param[in] only_gamma
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

/**
 * @brief Operates in place, quartering the size of the texture
 * Proper linear filter
 * @param in
 * @param inWidth
 * @param inHeight
 */
static void R_MipMap2(unsigned *in, int inWidth, int inHeight)
{
	int      i, j, k;
	byte     *outpix;
	int      inWidthMask  = inWidth - 1;
	int      inHeightMask = inHeight - 1;
	int      total;
	int      outWidth  = inWidth >> 1;
	int      outHeight = inHeight >> 1;
	unsigned *temp;

	temp = ri.Hunk_AllocateTempMemory(outWidth * outHeight * 4);

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

/**
 * @brief Operates in place, quartering the size of the texture
 * @param[in] in
 * @param[in] width
 * @param[in] height
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

/**
 * @brief Apply a color blend over a set of pixels
 * @param data
 * @param pixelCount
 * @param blend
 */
static void R_BlendOverTexture(byte *data, int pixelCount, byte blend[4])
{
	int i;
	int inverseAlpha = 255 - blend[3];
	int premult[3]   = { blend[0] * blend[3], blend[1] * blend[3], blend[2] * blend[3] };

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
// helper function for GLES format conversions
byte *gles_convertRGB(byte *data, int width, int height)
{
	byte *temp = (byte *) ri.Z_Malloc(width * height * 3);
	byte *src  = data;
	byte *dst  = temp;
	int  i, j;

	for (i = 0; i < width * height; i++)
	{
		for (j = 0; j < 3; j++)
			*(dst++) = *(src++);
		src++;
	}

	return temp;
}
byte *gles_convertRGBA4(byte *data, int width, int height)
{
	byte *temp = (byte *) ri.Z_Malloc(width * height * 2);
	int  i;

	unsigned int   *input  = ( unsigned int *)(data);
	unsigned short *output = (unsigned short *)(temp);
	for (i = 0; i < width * height; i++)
	{
		unsigned int pixel = *(input++);
		// Unpack the source data as 8 bit values
		unsigned int r = pixel & 0xff;
		unsigned int g = (pixel >> 8) & 0xff;
		unsigned int b = (pixel >> 16) & 0xff;
		unsigned int a = (pixel >> 24) & 0xff;
		// Convert to 4 bit vales
		r         >>= 4; g >>= 4; b >>= 4; a >>= 4;
		*(output++) = r << 12 | g << 8 | b << 4 | a;
	}
	return temp;
}
byte *gles_convertRGB5(byte *data, int width, int height)
{
	byte *temp = (byte *) ri.Z_Malloc(width * height * 2);
	byte *src  = data;
	byte *dst  = temp;
	byte r, g, b;
	int  i;

	unsigned int   *input  = ( unsigned int *)(data);
	unsigned short *output = (unsigned short *)(temp);
	for (i = 0; i < width * height; i++)
	{
		unsigned int pixel = *(input++);
		// Unpack the source data as 8 bit values
		unsigned int r = pixel & 0xff;
		unsigned int g = (pixel >> 8) & 0xff;
		unsigned int b = (pixel >> 16) & 0xff;
		// Convert to 4 bit vales
		r         >>= 3; g >>= 2; b >>= 3;
		*(output++) = r << 11 | g << 5 | b;
	}
	return temp;
}
byte *gles_convertLuminance(byte *data, int width, int height)
{
	byte *temp = (byte *) ri.Z_Malloc(width * height);
	byte *src  = data;
	byte *dst  = temp;
	byte r, g, b;
	int  i;

	unsigned int *input  = ( unsigned int *)(data);
	byte         *output = (byte *)(temp);
	for (i = 0; i < width * height; i++)
	{
		unsigned int pixel = input[i];
		// Unpack the source data as 8 bit values
		unsigned int r = pixel & 0xff;
		output[i] = r;
	}
	return temp;
}
byte *gles_convertLuminanceAlpha(byte *data, int width, int height)
{
	byte *temp = (byte *) ri.Z_Malloc(width * height * 2);
	byte *src  = data;
	byte *dst  = temp;
	byte r, g, b;
	int  i;

	unsigned int   *input  = ( unsigned int *)(data);
	unsigned short *output = (unsigned short *)(temp);
	for (i = 0; i < width * height; i++)
	{
		unsigned int pixel = input[i];
		// Unpack the source data as 8 bit values
		unsigned int r = pixel & 0xff;
		unsigned int a = (pixel >> 24) & 0xff;
		output[i] = r | a << 8;
	}
	return temp;
}

/**
 * @brief Upload32
 * @param[in,out] data
 * @param[in] width
 * @param[in] height
 * @param[in] mipmap
 * @param[in] picmip
 * @param[in] lightMap
 * @param[out] format
 * @param[out] pUploadWidth
 * @param[out] pUploadHeight
 * @param[in] noCompress
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
		resampledBuffer = ri.Hunk_AllocateTempMemory(sizeof(unsigned) * scaled_width * scaled_height * 4);
		ResampleTexture(data, width, height, resampledBuffer, scaled_width, scaled_height);
		data   = resampledBuffer;
		width  = scaled_width;
		height = scaled_height;
	}

	// perform optional picmip operation
	if (picmip)
	{
		scaled_width  >>= r_picMip->integer;
		scaled_height >>= r_picMip->integer;
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

	scaledBuffer = ri.Hunk_AllocateTempMemory(sizeof(unsigned) * scaled_width * scaled_height);

	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	c       = width * height;
	scan    = ((byte *)data);
	samples = 3;

	if (lightMap)
	{
		if (r_greyScale->integer)
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
			if (r_greyScale->integer)
			{
				internalFormat = GL_LUMINANCE;
			}
			else if (r_textureBits->integer == 16)
			{
				internalFormat = GL_RGB5;
			}
			else
			{
				internalFormat = GL_RGB;
			}
		}
		else if (samples == 4)
		{
			if (r_greyScale->integer)
			{
				internalFormat = GL_LUMINANCE_ALPHA;
			}
			else if (r_textureBits->integer == 16)
			{
				internalFormat = GL_RGBA4;
			}
			else
			{
				internalFormat = GL_RGBA;
			}
		}
	}

	//*pformat = GL_RGBA;
	if ((scaled_width == width) &&
	    (scaled_height == height))
	{
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

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, (mipmap) ? GL_TRUE : GL_FALSE);

	// and now, convert if needed and upload
	// GLES doesn't do conversion itself, so we have to handle that
	byte *temp;
	switch (internalFormat)
	{
	case GL_RGB5:
		temp = gles_convertRGB5((byte *)scaledBuffer, scaled_width, scaled_height);
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, scaled_width, scaled_height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, temp);
		ri.Free(temp);
		break;
	case GL_RGBA4:
		temp = gles_convertRGBA4((byte *)scaledBuffer, scaled_width, scaled_height);
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, temp);
		ri.Free(temp);
		break;
	case GL_RGB:
		temp = gles_convertRGB((byte *)scaledBuffer, scaled_width, scaled_height);
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, scaled_width, scaled_height, 0, GL_RGB, GL_UNSIGNED_BYTE, temp);
		ri.Free(temp);
		break;
	case GL_LUMINANCE:
        temp = gles_convertLuminance((byte*)scaledBuffer, width, height);
        qglTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE, scaled_width, scaled_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, temp);
        ri.Free(temp);
        break;
     case GL_LUMINANCE_ALPHA:
        temp = gles_convertLuminanceAlpha((byte*)scaledBuffer, width, height);
        qglTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, scaled_width, scaled_height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, temp);
        ri.Free(temp);
        break;
	default:
		internalFormat = GL_RGBA;
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
		break;
	}

	*pUploadWidth  = scaled_width;
	*pUploadHeight = scaled_height;
	*format        = internalFormat;

	//	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	//	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	if (mipmap)
	{
		if (textureFilterAnisotropic)
		{
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
			                 (GLint)Com_Clamp(1, maxAnisotropy, r_extMaxAnisotropy->integer));
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

	if (scaledBuffer != 0)
	{
		ri.Hunk_FreeTempMemory(scaledBuffer);
	}
	if (resampledBuffer != 0)
	{
		ri.Hunk_FreeTempMemory(resampledBuffer);
	}
}

/**
 * @brief This is the only way any image_t are created
 * @param[in] name
 * @param[in] pic
 * @param[in] width
 * @param[in] height
 * @param[in] mipmap
 * @param[in] allowPicmip
 * @param[in] wrapClampMode
 * @return
 */
image_t *R_CreateImage(const char *name, const byte *pic, int width, int height,
                       qboolean mipmap, qboolean allowPicmip, int wrapClampMode)
{
	image_t  *image;
	qboolean isLightmap = qfalse;
	long     hash;
	qboolean noCompress = qfalse;

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Drop("R_CreateImage: \"%s\" is too long\n", name);
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
	if (r_extCompressedTextures->integer == 2 && (tr.allowCompress != qtrue))
	{
		noCompress = qtrue;
	}
	else if (r_extCompressedTextures->integer == 1 && (tr.allowCompress < 0))
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
		Ren_Drop("R_CreateImage: MAX_DRAWIMAGES hit\n");
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
	image->wrapClampMode = wrapClampMode;

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

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapClampMode);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapClampMode);

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

/**
 * @brief Loads any of the supported image types into a cannonical 32 bit format.
 *
 * @param[in] name
 * @param[out] pic
 * @param[out] width
 * @param[out] height
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
		if (ri.FS_FOpenFileRead(altName, NULL, qfalse) > 0)
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
		Ren_Developer("WARNING: %s not present in any supported image format\n", localName);
	}
}

/**
 * @brief Finds or loads the given image.
 *
 * @param[in] name
 * @param[in] mipmap
 * @param[in] allowPicmip
 * @param[in] glWrapClampMode
 * @param[in] lightmap
 *
 * @return NULL if it fails, not a default image.
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
					Ren_Developer("WARNING: reused image %s with mixed mipmap parm\n", name);
				}
				if (image->allowPicmip != allowPicmip)
				{
					Ren_Developer("WARNING: reused image %s with mixed allowPicmip parm\n", name);
				}
				if (image->wrapClampMode != glWrapClampMode)
				{
					Ren_Developer("WARNING: reused image %s with mixed glWrapClampMode parm\n", name);
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
		Ren_Developer("WARNING: Image '%s' not found. Note: This might be false positive for shaders w/o image.\n", name);
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
		Ren_Developer("WARNING: Image not power of 2 scaled: %s (%i:%i)\n", name, width, height);
		return NULL;
	}

	image = R_CreateImage(name, pic, width, height, mipmap, allowPicmip, glWrapClampMode);

	// no texture compression
	if (lightmap)
	{
		tr.allowCompress = allowCompress;
	}

	return image;
}

#define DLIGHT_SIZE 16

/**
 * @brief R_CreateDlightImage
 */
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

#define FOG_S       16
#define FOG_T       16  //  used to be 32

/**
 * @brief R_CreateFogImage
 */
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
}

#define DEFAULT_SIZE    16

/**
 * @brief R_CreateDefaultImage
 */
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

/**
 * @brief R_CreateBuiltinImages
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

/**
 * @brief R_SetColorMappings
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
			inf = 255 * pow(i / 255.0, 1.0 / g) + 0.5;
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
		ri.GLimp_SetGamma(s_gammatable, s_gammatable, s_gammatable);
	}
}

/**
 * @brief R_InitImages
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

/**
 * @brief R_DeleteTextures
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

/**
 * @brief This is unfortunate, but the skin files aren't
 * compatable with our normal parsing rules.
 * @param[out] data_p
 * @return
 */
static char *CommaParse(char **data_p)
{
	int         c     = 0, len = 0;
	char        *data = *data_p;
	static char com_token[MAX_TOKEN_CHARS];

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
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
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
		//Ren_Print("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

/**
 * @brief RE_GetSkinModel
 * @param[in] skinid
 * @param[in] type
 * @param[in] name
 * @return
 */
qboolean RE_GetSkinModel(qhandle_t skinid, const char *type, char *name)
{
	int    i;
	int    hash;
	skin_t *skin = tr.skins[skinid];

	hash = Com_HashKey((char *)type, strlen(type));

	for (i = 0; i < skin->numModels; i++)
	{
		if (hash != skin->models[i]->hash)
		{
			continue;
		}
		if (!Q_stricmp(skin->models[i]->type, type))
		{
			// whoops, should've been this way
			Q_strncpyz(name, skin->models[i]->model, sizeof(skin->models[i]->model));
			return qtrue;
		}
	}
	return qfalse;
}

/**
 * @brief RE_GetShaderFromModel
 * @param[in] modelid
 * @param[in] surfnum
 * @param withlightmap set to '0' will create a new shader that is a copy of the one found
 * on the model, without the lighmap stage, if the shader has a lightmap stage
 *
 * @return A shader index for a given model's surface
 *
 * @note Only works for bmodels right now.  Could modify for other models (md3's etc.)
 * @todo FIXME:
 * @todo withlightmap is unused
 */
qhandle_t RE_GetShaderFromModel(qhandle_t modelid, int surfnum, int withlightmap)
{
	model_t *model;

	if (surfnum < 0)
	{
		surfnum = 0;
	}

	model = R_GetModelByHandle(modelid);    // should be correct now

	if (model)
	{
		bmodel_t *bmodel = model->model.bmodel;

		if (bmodel && bmodel->firstSurface)
		{
			msurface_t *surf;
			shader_t   *shd;

			if (bmodel->numSurfaces == 0)
			{
				return 0;
			}

			if (surfnum >= bmodel->numSurfaces)     // if it's out of range, return the first surface
			{
				surfnum = 0;
			}

			surf = bmodel->firstSurface + surfnum;
			// check for null shader (can happen on func_explosive's with botclips attached)
			if (!surf->shader) // || surf->shader->defaultShader ?
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

/**
 * @brief RE_RegisterSkin
 * @param[in] name
 * @return
 */
qhandle_t RE_RegisterSkin(const char *name)
{
    skinSurface_t parseSurfaces[MAX_SKIN_SURFACES];
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
	int  totalSurfaces = 0;

	if (!name || !name[0])
	{
		Ren_Warning("RE_RegisterSkin WARNING: empty name passed to RE_RegisterSkin\n");
		return 0;
	}

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Warning("RE_RegisterSkin WARNING: skin name exceeds MAX_QPATH in RE_RegisterSkin\n");
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
		Ren_Warning("WARNING: RE_RegisterSkin '%s' - MAX_SKINS hit\n", name);
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
	// WARNING: HACK: ET evilly has filenames slightly longer than MAX_QPATH
	// this check breaks the loading of such skins
	/*
	if ( strcmp( name + strlen( name ) - 5, ".skin" ) ) {
	    skin->numSurfaces = 1;
	    skin->surfaces = ri.Hunk_Alloc( sizeof( skinSurface_t ), h_low );
	    skin->surfaces[0].shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
	    return hSkin;
	}
	*/

	// load and parse the skin file
	text.v = NULL;
	if (ri.FS_FOpenFileRead(name, NULL, qfalse) > 0)
	{
		ri.FS_ReadFile(name, &text.v);
	}

	if (!text.c)
	{
		Ren_Developer("WARNING: RE_RegisterSkin '%s' - empty skin or file not in path\n", name);
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
			if (skin->numModels >= MAX_PART_MODELS)
			{
				Ren_Warning("WARNING: Ignoring models in '%s', the max is %d!\n", name, MAX_PART_MODELS);
				break;
			}

			// this is specifying a model
			model = skin->models[skin->numModels] = ri.Hunk_Alloc(sizeof(skinModel_t), h_low);
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

		if (skin->numSurfaces < MAX_SKIN_SURFACES)
		{
			surf = &parseSurfaces[skin->numSurfaces];
			Q_strncpyz(surf->name, surfName, sizeof(surf->name));
            surf->hash   = Com_HashKey(surf->name, sizeof(surf->name));
			surf->shader = R_FindShader(token, LIGHTMAP_NONE, qtrue);
			skin->numSurfaces++;
		}

		totalSurfaces++;
	}

	ri.FS_FreeFile(text.v);

	if (totalSurfaces > MAX_SKIN_SURFACES)
	{
		ri.Printf(PRINT_WARNING, "WARNING: Ignoring excess surfaces (found %d, max is %d) in skin '%s'!\n",
		          totalSurfaces, MAX_SKIN_SURFACES, name);
	}

	// never let a skin have 0 shaders
	if (skin->numSurfaces == 0)
	{
		return 0;       // use default skin
	}

    // copy surfaces to skin
    skin->surfaces = ri.Hunk_Alloc(skin->numSurfaces * sizeof( skinSurface_t ), h_low );
    Com_Memcpy( skin->surfaces, parseSurfaces, skin->numSurfaces * sizeof( skinSurface_t ) );

	return hSkin;
}

/**
 * @brief R_InitSkins
 */
void R_InitSkins(void)
{
	skin_t *skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = ri.Hunk_Alloc(sizeof(skin_t), h_low);
	Q_strncpyz(skin->name, "<default skin>", sizeof(skin->name));
	skin->numSurfaces         = 1;
	skin->surfaces         = ri.Hunk_Alloc(sizeof(skinSurface_t), h_low);
	skin->surfaces[0].shader = tr.defaultShader;
}

/**
 * @brief R_GetSkinByHandle
 * @param[in] hSkin
 * @return
 */
skin_t *R_GetSkinByHandle(qhandle_t hSkin)
{
	if (hSkin < 1 || hSkin >= tr.numSkins)
	{
		return tr.skins[0];
	}
	return tr.skins[hSkin];
}

/**
 * @brief R_SkinList_f
 */
void R_SkinList_f(void)
{
	int    i, j;
	skin_t *skin;

	Ren_Print("------------------\n");

	for (i = 0 ; i < tr.numSkins ; i++)
	{
		skin = tr.skins[i];

		Ren_Print("%3i:%s (%d surfaces)\n", i, skin->name, skin->numSurfaces);
		for (j = 0 ; j < skin->numSurfaces ; j++)
		{
			Ren_Print("       %s = %s\n", skin->surfaces[j].name, skin->surfaces[j].shader->name);
		}
	}
	Ren_Print("------------------\n");
}

//==========================================================================================
// caching system

static int     numBackupImages = 0;
static image_t *backupHashTable[FILE_HASH_SIZE];

/**
 * @brief This will only get called to allocate the image_t structures, not that actual image pixels
 * @param[in] size
 * @return
 */
void *R_CacheImageAlloc(int size)
{
	if (r_cache->integer && r_cacheShaders->integer)
	{
		void *buf = Com_Allocate(size);    // ri.Z_Malloc causes load times about twice as long?

		if (!buf)
		{
			Ren_Drop("R_CacheImageAlloc: unable to allocate buffer\n ");
		}
		return buf;
	}
	else
	{
		return ri.Hunk_Alloc(size, h_low);
	}
}

/**
 * @brief R_CacheImageFreeAll
 *
 * @note Unused
 */
void R_CacheImageFreeAll()
{
	if (r_cache->integer && r_cacheShaders->integer)
	{
		int i = 0;

		for (i = 0; i < FILE_HASH_SIZE; i++)
		{
			if (backupHashTable[i])
			{
				R_CacheImageFree(backupHashTable[i]);
			}
		}
	}
}

/**
 * @brief R_CacheImageFree
 * @param[in] ptr
 */
void R_CacheImageFree(void *ptr)
{
	if (r_cache->integer && r_cacheShaders->integer)
	{
		Com_Dealloc(ptr);
	}
}

/**
 * @brief Remove this image from the backupHashTable and make sure it doesn't get overwritten
 * @param inImage
 * @return
 */
qboolean R_TouchImage(image_t *inImage)
{
	image_t *bImage, *bImagePrev;
	int     hash;

	if (inImage == tr.dlightImage ||
	    inImage == tr.whiteImage ||
	    inImage == tr.defaultImage ||
	    inImage->imgName[0] == '*')     // can't use lightmaps since they might have the same name, but different maps will have different actual lightmap pixels
	{
		return qfalse;
	}

	hash = inImage->hash;

	bImage     = backupHashTable[hash];
	bImagePrev = NULL;
	while (bImage)
	{
		if (bImage == inImage)
		{
			// add it to the current images
			if (tr.numImages == MAX_DRAWIMAGES)
			{
				Ren_Drop("R_CreateImage: MAX_DRAWIMAGES hit\n");
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

/**
 * @brief R_PurgeImage
 * @param[in] image
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

/**
 * @brief Can specify the number of Images to purge this call (used for background purging)
 * @param[in] purgeCount
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

/**
 * @brief R_BackupImages
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

/**
 * @brief R_FindCachedImage
 * @param[in] name
 * @param[in] hash
 * @return
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
				Ren_Drop("R_CreateImage: MAX_DRAWIMAGES hit\n");
			}

			R_TouchImage(bImage);
			return bImage;
		}

		bImage = bImage->next;
	}

	return NULL;
}

/**
 * @brief R_GetTextureId
 * @param[in] name
 * @return
 */
int R_GetTextureId(const char *name)
{
	int i;

	//ri.Printf( PRINT_ALL, "R_GetTextureId [%s].\n", name );
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

/**
 * @brief R_LoadCacheImages
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
			parms[i] = Q_atoi(token);
		}
		R_FindImageFile(name, parms[0], parms[1], parms[2], parms[3]);
	}

	ri.Hunk_FreeTempMemory(buf);
}
