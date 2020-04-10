/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
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
 * @file renderer2/tr_image.c
 */

#include "tr_local.h"

static byte          s_intensitytable[256];
static unsigned char s_gammatable[256];

int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int gl_filter_max = GL_LINEAR;

#define IMAGE_FILE_HASH_SIZE 4096
image_t *r_imageHashTable[IMAGE_FILE_HASH_SIZE];
// In r2 we use full filename hashes!? well mkay
// TODO: check if this fullpath shit is really needed for anything
#define generateHashValue(fname) Q_GenerateHashValue(fname, IMAGE_FILE_HASH_SIZE, qtrue, qtrue)

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

/**
 * @struct textureMode_s
 * @brief
 */
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

/**
 * @brief GL_TextureMode
 * @param[in] string
 */
void GL_TextureMode(const char *string)
{
	int     i;
	image_t *image;

	for (i = 0; i < 6; i++)
	{
		if (!Q_stricmp(modes[i].name, string))
		{
			break;
		}
	}

	if (i == 6)
	{
		Ren_Print("GL_TextureMode: Bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// bound texture anisotropy
	if (glConfig2.textureAnisotropyAvailable)
	{
		if (r_extTextureFilterAnisotropic->value > glConfig2.maxTextureAnisotropy)
		{
			ri.Cvar_Set("r_ext_texture_filter_anisotropic", va("%f", glConfig2.maxTextureAnisotropy));
		}
		else if (r_extTextureFilterAnisotropic->value < 1.0f)
		{
			ri.Cvar_Set("r_ext_texture_filter_anisotropic", "1.0");
		}
	}

	// change all the existing mipmap texture objects
	for (i = 0; i < tr.images.currentElements; i++)
	{
		image = (image_t *)Com_GrowListElement(&tr.images, i);

		if (image->filterType == FT_DEFAULT)
		{
			GL_Bind(image);

			// set texture filter
			glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, gl_filter_max);

			// set texture anisotropy
			if (glConfig2.textureAnisotropyAvailable)
			{
				glTexParameterf(image->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_extTextureFilterAnisotropic->value);
			}
		}
	}
}

/**
 * @brief R_SumOfUsedImages
 * @return
 */
int R_SumOfUsedImages(void)
{
	int     total = 0;
	int     i;
	image_t *image;

	for (i = 0; i < tr.images.currentElements; i++)
	{
		image = (image_t *)Com_GrowListElement(&tr.images, i);

		if (image->frameUsed == tr.frameCount)
		{
			total += image->uploadWidth * image->uploadHeight;
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
	int        dataSize = 0;
	int        imageDataSize;
	const char *yesno[] =
	{
		"no ", "yes"
	};

	Ren_Print("\n      -w-- -h-- -mm- -type-   -if-- wrap --name-------\n");

	for (i = 0; i < tr.images.currentElements; i++)
	{
		image = (image_t *)Com_GrowListElement(&tr.images, i);

		Ren_Print("%4i: %4i %4i  %s   ",
		          i, image->uploadWidth, image->uploadHeight, yesno[image->filterType == FT_DEFAULT]);

		switch (image->type)
		{
		case GL_TEXTURE_2D:
			texels       += image->uploadWidth * image->uploadHeight;
			imageDataSize = image->uploadWidth * image->uploadHeight;

			Ren_Print("2D   ");
			break;
		case GL_TEXTURE_CUBE_MAP_ARB:
			texels       += image->uploadWidth * image->uploadHeight * 6;
			imageDataSize = image->uploadWidth * image->uploadHeight * 6;

			Ren_Print("CUBE ");
			break;
		default:
			Ren_Print("???? ");
			imageDataSize = image->uploadWidth * image->uploadHeight;
			break;
		}

		switch (image->internalFormat)
		{
		case GL_RGB8:
			Ren_Print("RGB8     ");
			imageDataSize *= 3;
			break;
		case GL_RGBA8:
			Ren_Print("RGBA8    ");
			imageDataSize *= 4;
			break;
		case GL_RGB16:
			Ren_Print("RGB      ");
			imageDataSize *= 6;
			break;
		case GL_RGB16F_ARB:
			Ren_Print("RGB16F   ");
			imageDataSize *= 6;
			break;
		case GL_RGB32F_ARB:
			Ren_Print("RGB32F   ");
			imageDataSize *= 12;
			break;
		case GL_RGBA16F_ARB:
			Ren_Print("RGBA16F  ");
			imageDataSize *= 8;
			break;
		case GL_RGBA32F_ARB:
			Ren_Print("RGBA32F  ");
			imageDataSize *= 16;
			break;
		case GL_ALPHA16F_ARB:
			Ren_Print("A16F     ");
			imageDataSize *= 2;
			break;
		case GL_ALPHA32F_ARB:
			Ren_Print("A32F     ");
			imageDataSize *= 4;
			break;
		case GL_LUMINANCE_ALPHA16F_ARB:
			Ren_Print("LA16F    ");
			imageDataSize *= 4;
			break;
		case GL_LUMINANCE_ALPHA32F_ARB:
			Ren_Print("LA32F    ");
			imageDataSize *= 8;
			break;
		case GL_COMPRESSED_RGBA_ARB:
			Ren_Print("ARB      ");
			imageDataSize *= 4;     // FIXME
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			Ren_Print("DXT1     ");
			imageDataSize *= 4 / 8;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			Ren_Print("DXT1a    ");
			imageDataSize *= 4 / 8;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			Ren_Print("DXT3     ");
			imageDataSize *= 4 / 4;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			Ren_Print("DXT5     ");
			imageDataSize *= 4 / 4;
			break;
		case GL_DEPTH_COMPONENT16_ARB:
			Ren_Print("D16      ");
			imageDataSize *= 2;
			break;
		case GL_DEPTH_COMPONENT24_ARB:
			Ren_Print("D24      ");
			imageDataSize *= 3;
			break;
		case GL_DEPTH_COMPONENT32_ARB:
			Ren_Print("D32      ");
			imageDataSize *= 4;
			break;
		default:
			Ren_Print("????     ");
			imageDataSize *= 4;
			break;
		}

		switch (image->wrapType)
		{
		case WT_REPEAT:
			Ren_Print("rept  ");
			break;
		case WT_CLAMP:
			Ren_Print("clmp  ");
			break;
		case WT_EDGE_CLAMP:
			Ren_Print("eclmp ");
			break;
		case WT_ZERO_CLAMP:
			Ren_Print("zclmp ");
			break;
		case WT_ALPHA_ZERO_CLAMP:
			Ren_Print("azclmp");
			break;
		default:
			Ren_Print("%4i  ", image->wrapType);
			break;
		}

		dataSize += imageDataSize;

		Ren_Print(" %s\n", image->name);
	}
	Ren_Print(" ---------\n");
	Ren_Print(" %i total texels (not including mipmaps)\n", texels);
	Ren_Print(" %d.%02d MB total image memory\n", dataSize / (1024 * 1024),
	          (dataSize % (1024 * 1024)) * 100 / (1024 * 1024));
	Ren_Print(" %i total images\n\n", tr.images.currentElements);
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
 * @param[in,out] in
 * @param[in] inwidth
 * @param[in] inheight
 * @param[out] out
 * @param[in] outwidth
 * @param[in] outheight
 * @param[in] normalMap
 */
static void R_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight, qboolean normalMap)
{
	int      x, y;
	unsigned *inrow, *inrow2;
	unsigned fracstep, frac;
	unsigned p1[2048], p2[2048];
	byte     *pix1, *pix2, *pix3, *pix4;
	float    inv127 = 1.0f / 127.0f;
	vec3_t   n, n2, n3, n4;

	// NOTE: limitation not needed anymore
	//if(outwidth > 2048)
	//  Ren_Drop( "ResampleTexture: max width");

	if (outwidth < 1)
	{
		outwidth = 1;
	}

	fracstep = inwidth * 0x10000 / outwidth;
	frac     = fracstep >> 2;

	for (x = 0; x < outwidth; x++)
	{
		p1[x] = 4 * (frac >> 16);
		frac += fracstep;
	}
	frac = 3 * (fracstep >> 2);
	for (x = 0; x < outwidth; x++)
	{
		p2[x] = 4 * (frac >> 16);
		frac += fracstep;
	}

	if (normalMap)
	{
		for (y = 0; y < outheight; y++, out += outwidth)
		{
			inrow  = in + inwidth * (int)((y + 0.25) * inheight / outheight);
			inrow2 = in + inwidth * (int)((y + 0.75) * inheight / outheight);

			//frac = fracstep >> 1;

			for (x = 0; x < outwidth; x++)
			{
				pix1 = (byte *) inrow + p1[x];
				pix2 = (byte *) inrow + p2[x];
				pix3 = (byte *) inrow2 + p1[x];
				pix4 = (byte *) inrow2 + p2[x];

				n[0] = (pix1[0] * inv127 - 1.0f);
				n[1] = (pix1[1] * inv127 - 1.0f);
				n[2] = (pix1[2] * inv127 - 1.0f);

				n2[0] = (pix2[0] * inv127 - 1.0f);
				n2[1] = (pix2[1] * inv127 - 1.0f);
				n2[2] = (pix2[2] * inv127 - 1.0f);

				n3[0] = (pix3[0] * inv127 - 1.0f);
				n3[1] = (pix3[1] * inv127 - 1.0f);
				n3[2] = (pix3[2] * inv127 - 1.0f);

				n4[0] = (pix4[0] * inv127 - 1.0f);
				n4[1] = (pix4[1] * inv127 - 1.0f);
				n4[2] = (pix4[2] * inv127 - 1.0f);

				VectorAdd(n, n2, n);
				VectorAdd(n, n3, n);
				VectorAdd(n, n4, n);

				if (VectorNormalize(n) == 0.f)
				{
					VectorSet(n, 0, 0, 1);
				}

				((byte *) (out + x))[0] = (byte) (128 + 127 * n[0]);
				((byte *) (out + x))[1] = (byte) (128 + 127 * n[1]);
				((byte *) (out + x))[2] = (byte) (128 + 127 * n[2]);
				((byte *) (out + x))[3] = (byte) (128 + 127 * 1.0);
			}
		}
	}
	else
	{
		for (y = 0; y < outheight; y++, out += outwidth)
		{
			inrow  = in + inwidth * (int)((y + 0.25) * inheight / outheight);
			inrow2 = in + inwidth * (int)((y + 0.75) * inheight / outheight);

			//frac = fracstep >> 1;

			for (x = 0; x < outwidth; x++)
			{
				pix1 = (byte *) inrow + p1[x];
				pix2 = (byte *) inrow + p2[x];
				pix3 = (byte *) inrow2 + p1[x];
				pix4 = (byte *) inrow2 + p2[x];

				((byte *) (out + x))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
				((byte *) (out + x))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
				((byte *) (out + x))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
				((byte *) (out + x))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
			}
		}
	}
}

/**
 * @brief Scale up the pixel values in a texture to increase the
 * lighting range
 * @param[in,out] in
 * @param[in] inwidth
 * @param[in] inheight
 * @param[in] onlyGamma
 */
void R_LightScaleTexture(unsigned *in, int inwidth, int inheight, qboolean onlyGamma)
{
	if (onlyGamma)
	{
		if (!glConfig.deviceSupportsGamma)
		{
			int  i, c = inwidth * inheight;
			byte *p = (byte *) in;

			for (i = 0; i < c; i++, p += 4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		int  i, c = inwidth * inheight;
		byte *p = (byte *) in;

		if (glConfig.deviceSupportsGamma)
		{
			// raynorpat: small optimization
			if (r_intensity->value != 1.0f)
			{
				for (i = 0; i < c; i++, p += 4)
				{
					p[0] = s_intensitytable[p[0]];
					p[1] = s_intensitytable[p[1]];
					p[2] = s_intensitytable[p[2]];
				}
			}
		}
		else
		{
			for (i = 0; i < c; i++, p += 4)
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
 * @param[in] in
 * @param[in] inWidth
 * @param[in] inHeight
 */
static void R_MipMap2(unsigned *in, int inWidth, int inHeight)
{
	int      i, j, k;
	byte     *outpix;
	int      inWidthMask, inHeightMask;
	int      total;
	int      outWidth  = inWidth >> 1;
	int      outHeight = inHeight >> 1;
	unsigned *temp;

	temp = (unsigned int *)ri.Hunk_AllocateTempMemory(outWidth * outHeight * 4);

	inWidthMask  = inWidth - 1;
	inHeightMask = inHeight - 1;

	for (i = 0; i < outHeight; i++)
	{
		for (j = 0; j < outWidth; j++)
		{
			outpix = (byte *) (temp + i * outWidth + j);
			for (k = 0; k < 4; k++)
			{
				total =
				    1 * ((byte *) &in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
				    2 * ((byte *) &in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
				    2 * ((byte *) &in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
				    1 * ((byte *) &in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +
				    2 * ((byte *) &in[((i * 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
				    4 * ((byte *) &in[((i * 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
				    4 * ((byte *) &in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
				    2 * ((byte *) &in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +
				    2 * ((byte *) &in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
				    4 * ((byte *) &in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
				    4 * ((byte *) &in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
				    2 * ((byte *) &in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +
				    1 * ((byte *) &in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
				    2 * ((byte *) &in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
				    2 * ((byte *) &in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
				    1 * ((byte *) &in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k];
				outpix[k] = total / 36;
			}
		}
	}

	Com_Memcpy(in, temp, outWidth * outHeight * 4);
	ri.Hunk_FreeTempMemory(temp);
}

/**
 * @brief Operates in place, quartering the size of the texture
 * @param[in,out] in
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
		width += height;        // get largest
		for (i = 0; i < width; i++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4]) >> 1;
			out[1] = (in[1] + in[5]) >> 1;
			out[2] = (in[2] + in[6]) >> 1;
			out[3] = (in[3] + in[7]) >> 1;
		}
		return;
	}

	for (i = 0; i < height; i++, in += row)
	{
		for (j = 0; j < width; j++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4] + in[row + 0] + in[row + 4]) >> 2;
			out[1] = (in[1] + in[5] + in[row + 1] + in[row + 5]) >> 2;
			out[2] = (in[2] + in[6] + in[row + 2] + in[row + 6]) >> 2;
			out[3] = (in[3] + in[7] + in[row + 3] + in[row + 7]) >> 2;
		}
	}
}

// *INDENT-OFF*
/**
 * @brief Operates in place, quartering the size of the texture
 * @param[in,out] in
 * @param[in] width
 * @param[in] height
 */
static void R_MipNormalMap(byte *in, int width, int height)
{
	int    i, j;
	byte   *out;
	vec4_t n;
	vec_t  length;
	float  inv255 = 1.0f / 255.0f;

	if (width == 1 && height == 1)
	{
		return;
	}

	out = in;
	width >>= 1;
	//width  <<= 2; // why is this *4 ??  mipmaps that get wider and wider?
	height >>= 1;

	for (i = 0; i < height; i++, in += width)
	{
		for (j = 0; j < width; j += 8, out += 4, in += 8)
		{
			n[0] = (in[0] * inv255 - 0.5f) * 2.0f +
			       (in[4] * inv255 - 0.5f) * 2.0f +
			       (in[width + 0] * inv255 - 0.5f) * 2.0f +
			       (in[width + 4] * inv255 - 0.5f) * 2.0f;

			n[1] = (in[1] * inv255 - 0.5f) * 2.0f +
			       (in[5] * inv255 - 0.5f) * 2.0f +
			       (in[width + 1] * inv255 - 0.5f) * 2.0f +
			       (in[width + 5] * inv255 - 0.5f) * 2.0f;

			n[2] = (in[2] * inv255 - 0.5f) * 2.0f +
			       (in[6] * inv255 - 0.5f) * 2.0f +
			       (in[width + 2] * inv255 - 0.5f) * 2.0f +
			       (in[width + 6] * inv255 - 0.5f) * 2.0f;

			n[3] = (inv255 * in[3]) +
			       (inv255 * in[7]) +
			       (inv255 * in[width + 3]) +
			       (inv255 * in[width + 7]);

			length = VectorLength(n);

			if (length != 0.f)
			{
				float length1 = 1.0 / length;
				n[0] *= length1;
				n[1] *= length1;
				n[2] *= length1;
			}
			else
			{
				VectorSet(n, 0.0, 0.0, 1.0);
			}

			out[0] = (byte) (128 + 127 * n[0]);
			out[1] = (byte) (128 + 127 * n[1]);
			out[2] = (byte) (128 + 127 * n[2]);
			out[3] = (byte) (n[3] * 255.0f / 4.0f);
			//out[3] = (in[3] + in[7] + in[width + 3] + in[width + 7]) >> 2;
		}
	}
}
// *INDENT-ON*

/**
 * @brief R_HeightMapToNormalMap
 * @param[in,out] in
 * @param[in] width
 * @param[in] height
 * @param[in] scale
 */
static void R_HeightMapToNormalMap(byte *in, int width, int height, float scale)
{
	int    x, y;
	float  r, g, b;
	float  c, cx, cy;
	float  dcx, dcy;
	float  inv255 = 1.0f / 255.0f;
	vec3_t n;
	byte   *out = in;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			// convert the pixel at x, y in the bump map to a normal (float)

			// expand [0,255] texel values to the [0,1] range
			r = in[4 * (y * width + x) + 0];
			g = in[4 * (y * width + x) + 1];
			b = in[4 * (y * width + x) + 2];

			c = (r + g + b) * inv255;

			// expand the texel to its right
			if (x == width - 1)
			{
				r = in[4 * (y * width + x) + 0];
				g = in[4 * (y * width + x) + 1];
				b = in[4 * (y * width + x) + 2];
			}
			else
			{
				r = in[4 * (y * width + ((x + 1) % width)) + 0];
				g = in[4 * (y * width + ((x + 1) % width)) + 1];
				b = in[4 * (y * width + ((x + 1) % width)) + 2];
			}

			cx = (r + g + b) * inv255;

			// expand the texel one up
			if (y == height - 1)
			{
				r = in[4 * (y * width + x) + 0];
				g = in[4 * (y * width + x) + 1];
				b = in[4 * (y * width + x) + 2];
			}
			else
			{
				r = in[4 * (((y + 1) % height) * width + x) + 0];
				g = in[4 * (((y + 1) % height) * width + x) + 1];
				b = in[4 * (((y + 1) % height) * width + x) + 2];
			}

			cy = (r + g + b) * inv255;

			dcx = scale * (c - cx);
			dcy = scale * (c - cy);

			// normalize the vector
			VectorSet(n, dcx, dcy, 1.0);    //scale);
			if (VectorNormalize(n) == 0.f)
			{
				VectorSet(n, 0, 0, 1);
			}

			// repack the normalized vector into an RGB unsigned byte
			// vector in the normal map image
			*out++ = (byte) (128 + 127 * n[0]);
			*out++ = (byte) (128 + 127 * n[1]);
			*out++ = (byte) (128 + 127 * n[2]);

			// put in no height as displacement map by default
			*out++ = (byte) 0;  //(Q_bound(0, c * 255.0 / 3.0, 255));
		}
	}
}

/**
 * @brief R_DisplaceMap
 * @param[in,out] in
 * @param[in] in2
 * @param[in] width
 * @param[in] height
 */
static void R_DisplaceMap(byte *in, byte *in2, int width, int height)
{
	int    x, y;
	vec3_t n;
	int    avg;
	float  inv255 = 1.0f / 255.0f;
	byte   *out   = in;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			n[0] = (in[4 * (y * width + x) + 0] * inv255 - 0.5f) * 2.0f;
			n[1] = (in[4 * (y * width + x) + 1] * inv255 - 0.5f) * 2.0f;
			n[2] = (in[4 * (y * width + x) + 2] * inv255 - 0.5f) * 2.0f;

			avg  = 0;
			avg += in2[4 * (y * width + x) + 0];
			avg += in2[4 * (y * width + x) + 1];
			avg += in2[4 * (y * width + x) + 2];
			avg /= 3;

			*out++ = (byte) (128 + 127 * n[0]);
			*out++ = (byte) (128 + 127 * n[1]);
			*out++ = (byte) (128 + 127 * n[2]);
			*out++ = (byte) (avg);
		}
	}
}

/**
 * @brief R_AddNormals
 * @param[in,out] in
 * @param[in] in2
 * @param[in] width
 * @param[in] height
 */
static void R_AddNormals(byte *in, byte *in2, int width, int height)
{
	int    x, y;
	vec3_t n;
	byte   a;
	vec3_t n2;
	byte   a2;
	float  inv255 = 1.0f / 255.0f;
	byte   *out   = in;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			n[0] = (in[4 * (y * width + x) + 0] * inv255 - 0.5f) * 2.0f;
			n[1] = (in[4 * (y * width + x) + 1] * inv255 - 0.5f) * 2.0f;
			n[2] = (in[4 * (y * width + x) + 2] * inv255 - 0.5f) * 2.0f;
			a    = in[4 * (y * width + x) + 3];

			n2[0] = (in2[4 * (y * width + x) + 0] * inv255 - 0.5f) * 2.0f;
			n2[1] = (in2[4 * (y * width + x) + 1] * inv255 - 0.5f) * 2.0f;
			n2[2] = (in2[4 * (y * width + x) + 2] * inv255 - 0.5f) * 2.0f;
			a2    = in2[4 * (y * width + x) + 3];

			VectorAdd(n, n2, n);

			if (VectorNormalize(n) == 0.f)
			{
				VectorSet(n, 0, 0, 1);
			}

			*out++ = (byte) (128 + 127 * n[0]);
			*out++ = (byte) (128 + 127 * n[1]);
			*out++ = (byte) (128 + 127 * n[2]);
			*out++ = (byte) (Q_bound(0, a + a2, 255));
		}
	}
}

/**
 * @brief R_InvertAlpha
 * @param[in,out] in
 * @param[in] width
 * @param[in] height
 */
static void R_InvertAlpha(byte *in, int width, int height)
{
	int  x, y;
	byte *out = in;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			out[4 * (y * width + x) + 3] = 255 - in[4 * (y * width + x) + 3];
		}
	}
}

/**
 * @brief R_InvertColor
 * @param[in,out] in
 * @param[in] width
 * @param[in] height
 */
static void R_InvertColor(byte *in, int width, int height)
{
	int  x, y;
	byte *out = in;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			out[4 * (y * width + x) + 0] = 255 - in[4 * (y * width + x) + 0];
			out[4 * (y * width + x) + 1] = 255 - in[4 * (y * width + x) + 1];
			out[4 * (y * width + x) + 2] = 255 - in[4 * (y * width + x) + 2];
		}
	}
}

/**
 * @brief R_MakeIntensity
 * @param[in,out] in
 * @param width
 * @param height
 */
static void R_MakeIntensity(byte *in, int width, int height)
{
	int  x, y;
	byte *out = in;
	byte red;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			red = out[4 * (y * width + x) + 0];

			out[4 * (y * width + x) + 1] = red;
			out[4 * (y * width + x) + 2] = red;
			out[4 * (y * width + x) + 3] = red;
		}
	}
}

/**
 * @brief R_MakeAlpha
 * @param[in,out] in
 * @param[in] width
 * @param[in] height
 */
static void R_MakeAlpha(byte *in, int width, int height)
{
	int  x, y;
	byte *out = in;
	int  avg;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			avg  = 0;
			avg += out[4 * (y * width + x) + 0];
			avg += out[4 * (y * width + x) + 1];
			avg += out[4 * (y * width + x) + 2];
			avg /= 3;

			out[4 * (y * width + x) + 0] = 255;
			out[4 * (y * width + x) + 1] = 255;
			out[4 * (y * width + x) + 2] = 255;
			out[4 * (y * width + x) + 3] = (byte) avg;
		}
	}
}

/**
 * @brief Apply a color blend over a set of pixels
 * @param[in,out] data
 * @param[in] pixelCount
 * @param[in] blend
 */
static void R_BlendOverTexture(byte *data, int pixelCount, byte blend[4])
{
	int i;
	int inverseAlpha = 255 - blend[3];
	int premult[3];

	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for (i = 0; i < pixelCount; i++, data += 4)
	{
		data[0] = (byte)((data[0] * inverseAlpha + premult[0]) >> 9);
		data[1] = (byte)((data[1] * inverseAlpha + premult[1]) >> 9);
		data[2] = (byte)((data[2] * inverseAlpha + premult[2]) >> 9);
	}
}

byte mipBlendColors[16][4] =
{
	{ 0,   0,   0,   0   }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
};

/**
 * @brief R_UploadImage
 * @param[in] dataArray
 * @param[in] numData
 * @param[in] image
 */
void R_UploadImage(const byte **dataArray, int numData, image_t *image)
{
	const byte *data         = dataArray[0];
	byte       *scaledBuffer = NULL;
	int        scaledWidth, scaledHeight;
	int        i, c;
	const byte *scan;
	GLenum     target;
	GLenum     format               = GL_RGBA;
	GLenum     internalFormat       = GL_RGB;
	float      rMax                 = 0, gMax = 0, bMax = 0;
	vec4_t     zeroClampBorder      = { 0, 0, 0, 1 };
	vec4_t     alphaZeroClampBorder = { 0, 0, 0, 0 };

	if (glConfig2.textureNPOTAvailable)
	{
		scaledWidth  = image->width;
		scaledHeight = image->height;
	}
	else
	{
		// convert to exact power of 2 sizes
		for (scaledWidth = 1; scaledWidth < image->width; scaledWidth <<= 1)
			;
		for (scaledHeight = 1; scaledHeight < image->height; scaledHeight <<= 1)
			;
	}

	if (r_roundImagesDown->integer && scaledWidth > image->width)
	{
		scaledWidth >>= 1;
	}
	if (r_roundImagesDown->integer && scaledHeight > image->height)
	{
		scaledHeight >>= 1;
	}

	// perform optional picmip operation
	if (!(image->bits & IF_NOPICMIP))
	{
		scaledWidth  >>= r_picMip->integer;
		scaledHeight >>= r_picMip->integer;
	}

	// clamp to minimum size
	if (scaledWidth < 1)
	{
		scaledWidth = 1;
	}
	if (scaledHeight < 1)
	{
		scaledHeight = 1;
	}

	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	if (image->type == GL_TEXTURE_CUBE_MAP_ARB)
	{
		while (scaledWidth > glConfig2.maxCubeMapTextureSize || scaledHeight > glConfig2.maxCubeMapTextureSize)
		{
			scaledWidth  >>= 1;
			scaledHeight >>= 1;
		}
	}
	else
	{
		while (scaledWidth > glConfig.maxTextureSize || scaledHeight > glConfig.maxTextureSize)
		{
			scaledWidth  >>= 1;
			scaledHeight >>= 1;
		}
	}

	scaledBuffer = (byte *)ri.Hunk_AllocateTempMemory(sizeof(byte) * scaledWidth * scaledHeight * 4);

	// set target
	switch (image->type)
	{
	case GL_TEXTURE_CUBE_MAP_ARB:
		target = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
		break;
	default:
		target = GL_TEXTURE_2D;
		break;
	}

	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	c    = image->width * image->height;
	scan = data;

	if (image->bits & (IF_DEPTH16 | IF_DEPTH24 | IF_DEPTH32))
	{
		format = GL_DEPTH_COMPONENT;

		if (image->bits & IF_DEPTH16)
		{
			internalFormat = GL_DEPTH_COMPONENT16_ARB;
		}
		else if (image->bits & IF_DEPTH24)
		{
			internalFormat = GL_DEPTH_COMPONENT24_ARB;
		}
		else if (image->bits & IF_DEPTH32)
		{
			internalFormat = GL_DEPTH_COMPONENT32_ARB;
		}
	}
	else if (image->bits & (IF_PACKED_DEPTH24_STENCIL8))
	{
		format         = GL_DEPTH_STENCIL_EXT;
		internalFormat = GL_DEPTH24_STENCIL8_EXT;
	}
	else if (glConfig2.textureFloatAvailable &&
	         (image->bits & (IF_RGBA16F | IF_RGBA32F | IF_RGBA16 | IF_LA16F | IF_LA32F | IF_ALPHA16F | IF_ALPHA32F)))
	{
		if (image->bits & IF_RGBA16F)
		{
			internalFormat = GL_RGBA16F_ARB;
		}
		else if (image->bits & IF_RGBA32F)
		{
			internalFormat = GL_RGBA32F_ARB;
		}
		else if (image->bits & IF_LA16F)
		{
			internalFormat = GL_LUMINANCE_ALPHA16F_ARB;
		}
		else if (image->bits & IF_LA32F)
		{
			internalFormat = GL_LUMINANCE_ALPHA32F_ARB;
		}
		else if (image->bits & IF_RGBA16)
		{
			internalFormat = GL_RGBA16;
		}
		else if (image->bits & IF_ALPHA16F)
		{
			internalFormat = GL_ALPHA16F_ARB;
		}
		else if (image->bits & IF_ALPHA32F)
		{
			internalFormat = GL_ALPHA32F_ARB;
		}
	}
	else if (image->bits & IF_RGBE)
	{
		internalFormat = GL_RGBA8;
	}
	else
	{
		int samples = 3;

		// normalmaps have the displacement maps in the alpha channel
		// samples 3 would cause an opaque alpha channel and odd displacements!
		if (image->bits & IF_NORMALMAP)
		{
			if (image->bits & (IF_DISPLACEMAP | IF_ALPHATEST))
			{
				samples = 4;
			}
			else
			{
				samples = 3;
			}
		}
		else if (image->bits & IF_LIGHTMAP)
		{
			samples = 3;
		}
		else
		{
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
		}

		// select proper internal format
		if (samples == 3)
		{
			if (glConfig.textureCompression == TC_S3TC && !(image->bits & IF_NOCOMPRESSION))
			{
				internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			}
			else
			{
				internalFormat = GL_RGB8;
			}
		}
		else if (samples == 4)
		{
			if (image->bits & IF_ALPHA)
			{
				internalFormat = GL_ALPHA8;
			}
			else
			{
				if (glConfig.textureCompression == TC_S3TC && !(image->bits & IF_NOCOMPRESSION))
				{
					if (image->bits & IF_DISPLACEMAP)
					{
						internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
					}
					else if (image->bits & IF_ALPHATEST)
					{
						internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
					}
					else
					{
						internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
					}
				}
				else
				{
					internalFormat = GL_RGBA8;
				}
			}
		}
	}

	for (i = 0; i < numData; i++)
	{
		data = dataArray[i];

		// copy or resample data as appropriate for first MIP level
		if ((scaledWidth == image->width) && (scaledHeight == image->height))
		{
			Com_Memcpy(scaledBuffer, data, scaledWidth * scaledHeight * 4);
		}
		else
		{
			R_ResampleTexture((unsigned *)data, image->width, image->height, (unsigned *)scaledBuffer, scaledWidth, scaledHeight, (image->bits & IF_NORMALMAP));
		}

		if (!(image->bits & (IF_NORMALMAP | IF_RGBA16F | IF_RGBA32F | IF_LA16F | IF_LA32F)))
		{
			R_LightScaleTexture((unsigned *)scaledBuffer, scaledWidth, scaledHeight, image->filterType == FT_DEFAULT);
		}

		image->uploadWidth    = scaledWidth;
		image->uploadHeight   = scaledHeight;
		image->internalFormat = internalFormat;

		switch (image->type)
		{
		case GL_TEXTURE_CUBE_MAP_ARB:
			glTexImage2D(target + i, 0, internalFormat, scaledWidth, scaledHeight, 0, format, GL_UNSIGNED_BYTE,
			             scaledBuffer);
			break;
		default:
			if (image->bits & IF_PACKED_DEPTH24_STENCIL8)
			{
				glTexImage2D(target, 0, internalFormat, scaledWidth, scaledHeight, 0, format, GL_UNSIGNED_INT_24_8_EXT, NULL);
			}
			else
			{
				glTexImage2D(target, 0, internalFormat, scaledWidth, scaledHeight, 0, format, GL_UNSIGNED_BYTE, scaledBuffer);
			}
			break;
		}

		if (image->filterType == FT_DEFAULT)
		{
			if (glConfig2.framebufferObjectAvailable)
			{
				if (image->type != GL_TEXTURE_CUBE_MAP_ARB) // FIXME: deal with frame buffer object? No mipmap for now ...
				{
					glGenerateMipmapEXT(image->type);
					glTexParameteri(image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);   // default to trilinear
				}
			}
			else if (glConfig2.generateMipmapAvailable)
			{
				// raynorpat: if hardware mipmap generation is available, use it
				//glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);	// make sure its nice
				glTexParameteri(image->type, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
				glTexParameteri(image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);   // default to trilinear
			}
		}

		if (!glConfig2.framebufferObjectAvailable && !glConfig2.generateMipmapAvailable)
		{
			if (image->filterType == FT_DEFAULT && !(image->bits & (IF_DEPTH16 | IF_DEPTH24 | IF_DEPTH32 | IF_PACKED_DEPTH24_STENCIL8)))
			{
				int mipLevel = 0;
				int mipWidth = scaledWidth, mipHeight = scaledHeight;

				while (mipWidth > 1 || mipHeight > 1)
				{
					if (image->bits & IF_NORMALMAP)
					{
						R_MipNormalMap(scaledBuffer, mipWidth, mipHeight);
					}
					else
					{
						R_MipMap(scaledBuffer, mipWidth, mipHeight);
					}

					mipWidth  >>= 1;
					mipHeight >>= 1;

					if (mipWidth < 1)
					{
						mipWidth = 1;
					}

					if (mipHeight < 1)
					{
						mipHeight = 1;
					}

					mipLevel++;

					if (r_colorMipLevels->integer && !(image->bits & IF_NORMALMAP))
					{
						R_BlendOverTexture(scaledBuffer, mipWidth * mipHeight, mipBlendColors[mipLevel]);
					}

					switch (image->type)
					{
					case GL_TEXTURE_CUBE_MAP_ARB:
						glTexImage2D(target + i, mipLevel, internalFormat, mipWidth, mipHeight, 0, format, GL_UNSIGNED_BYTE,
						             scaledBuffer);
						break;
					default:
						glTexImage2D(target, mipLevel, internalFormat, mipWidth, mipHeight, 0, format, GL_UNSIGNED_BYTE,
						             scaledBuffer);
						break;
					}
				}
			}
		}
	}

	GL_CheckErrors();

	// set filter type
	switch (image->filterType)
	{
	case FT_DEFAULT:
		// set texture anisotropy
		if (glConfig2.textureAnisotropyAvailable)
		{
			glTexParameterf(image->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_extTextureFilterAnisotropic->value);
		}

		glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		break;
	case FT_LINEAR:
		glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case FT_NEAREST:
		glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	default:
		Ren_Warning("WARNING: unknown filter type for image '%s'\n", image->name);
		glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	}

	GL_CheckErrors();

	// set wrap type
	switch (image->wrapType)
	{
	case WT_REPEAT:
		glTexParameterf(image->type, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(image->type, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;
	case WT_CLAMP:
		glTexParameterf(image->type, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(image->type, GL_TEXTURE_WRAP_T, GL_CLAMP);
		break;
	case WT_EDGE_CLAMP:
		glTexParameterf(image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		break;
	case WT_ZERO_CLAMP:
		glTexParameterf(image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameterf(image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(image->type, GL_TEXTURE_BORDER_COLOR, zeroClampBorder);
		break;
	case WT_ALPHA_ZERO_CLAMP:
		glTexParameterf(image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameterf(image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(image->type, GL_TEXTURE_BORDER_COLOR, alphaZeroClampBorder);
		break;
	default:
		Ren_Warning("WARNING: unknown wrap type for image '%s'\n", image->name);
		glTexParameterf(image->type, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(image->type, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;
	}

	GL_CheckErrors();

	if (scaledBuffer != 0)
	{
		ri.Hunk_FreeTempMemory(scaledBuffer);
	}
}

/**
 * @brief R_AllocImage
 * @param[in] name
 * @param[in] linkIntoHashTable
 * @return
 */
image_t *R_AllocImage(const char *name, qboolean linkIntoHashTable)
{
	image_t *image;
	long    hash;
	char    buffer[1024];

	//if(strlen(name) >= MAX_QPATH)
	if (strlen(name) >= 1024)
	{
		Ren_Drop("R_AllocImage: \"%s\" image name is too long\n", name);
		return NULL;
	}

	image = (image_t *)ri.Hunk_Alloc(sizeof(image_t), h_low);
	Com_Memset(image, 0, sizeof(image_t));

	glGenTextures(1, &image->texnum);

	Com_AddToGrowList(&tr.images, image);

	Q_strncpyz(image->name, name, sizeof(image->name));

	if (linkIntoHashTable)
	{
		Q_strncpyz(buffer, name, sizeof(buffer));
		hash                   = generateHashValue(buffer);
		image->next            = r_imageHashTable[hash];
		r_imageHashTable[hash] = image;
	}

	return image;
}

/**
 * @brief R_CreateImage
 * @param[in] name
 * @param[in] pic
 * @param[in] width
 * @param[in] height
 * @param[in] bits
 * @param[in] filterType
 * @param[in] wrapType
 * @return
 */
image_t *R_CreateImage(const char *name,
                       const byte *pic, int width, int height, int bits, filterType_t filterType, wrapType_t wrapType)
{
	image_t *image;

	image = R_AllocImage(name, qtrue);
	if (!image)
	{
		return NULL;
	}

	image->type = GL_TEXTURE_2D;

	image->width  = width;
	image->height = height;

	image->bits       = bits;
	image->filterType = filterType;
	image->wrapType   = wrapType;

	GL_Bind(image);

	R_UploadImage(&pic, 1, image);

	//GL_Unbind();
	glBindTexture(image->type, 0);

	return image;
}

/**
 * @brief R_CreateCubeImage
 * @param[in] name
 * @param[in] pic
 * @param[in] width
 * @param[in] height
 * @param[in] bits
 * @param[in] filterType
 * @param[in] wrapType
 * @return
 */
image_t *R_CreateCubeImage(const char *name,
                           const byte *pic[6],
                           int width, int height, int bits, filterType_t filterType, wrapType_t wrapType)
{
	image_t *image;

	image = R_AllocImage(name, qtrue);
	if (!image)
	{
		return NULL;
	}

	image->type = GL_TEXTURE_CUBE_MAP_ARB;

	image->width  = width;
	image->height = height;

	image->bits       = bits;
	image->filterType = filterType;
	image->wrapType   = wrapType;

	GL_Bind(image);

	R_UploadImage(pic, 6, image);

	glBindTexture(image->type, 0);

	return image;
}

static void R_LoadImage(char **buffer, byte **pic, int *width, int *height, int *bits, const char *materialName);
//image_t *R_LoadDDSImage(const char *name, int bits, filterType_t filterType, wrapType_t wrapType);

/**
 * @brief ParseHeightMap
 * @param[in,out] text
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[in,out] bits
 * @param[in] materialName
 * @return
 */
static qboolean ParseHeightMap(char **text, byte **pic, int *width, int *height, int *bits, const char *materialName)
{
	char  *token;
	float scale;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != '(')
	{
		Ren_Warning("WARNING: expecting '(', found '%s' for heightMap\n", token);
		return qfalse;
	}

	R_LoadImage(text, pic, width, height, bits, materialName);
	if (!*pic)
	{
		Ren_Warning("WARNING: failed loading of image for heightMap\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ',')
	{
		Ren_Warning("WARNING: no matching ',' found for heightMap\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	scale = atof(token);

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ')')
	{
		Ren_Warning("WARNING: expecting ')', found '%s' for heightMap\n", token);
		return qfalse;
	}

	R_HeightMapToNormalMap(*pic, *width, *height, scale);

	*bits &= ~IF_ALPHA;
	*bits |= IF_NORMALMAP;

	return qtrue;
}

/**
 * @brief ParseDisplaceMap
 * @param[in,out] text
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[out] bits
 * @param[in] materialName
 * @return
 */
static qboolean ParseDisplaceMap(char **text, byte **pic, int *width, int *height, int *bits, const char *materialName)
{
	char *token;
	byte *pic2;
	int  width2, height2;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != '(')
	{
		Ren_Warning("WARNING: expecting '(', found '%s' for displaceMap\n", token);
		return qfalse;
	}

	R_LoadImage(text, pic, width, height, bits, materialName);
	if (!*pic)
	{
		Ren_Warning("WARNING: failed loading of first image for displaceMap\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ',')
	{
		Ren_Warning("WARNING: no matching ',' found for displaceMap\n");
		return qfalse;
	}

	R_LoadImage(text, &pic2, &width2, &height2, bits, materialName);
	if (!pic2)
	{
		Ren_Warning("WARNING: failed loading of second image for displaceMap\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ')')
	{
		Ren_Warning("WARNING: expecting ')', found '%s' for displaceMap\n", token);
	}

	if (*width != width2 || *height != height2)
	{
		Ren_Warning("WARNING: images for displaceMap have different dimensions (%i x %i != %i x %i)\n",
		            *width, *height, width2, height2);
		Com_Dealloc(pic2);
		return qfalse;
	}

	R_DisplaceMap(*pic, pic2, *width, *height);

	Com_Dealloc(pic2);

	*bits &= ~IF_ALPHA;
	*bits |= IF_NORMALMAP;
	*bits |= IF_DISPLACEMAP;

	return qtrue;
}

/**
 * @brief ParseAddNormals
 * @param[in,out] text
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[out] bits
 * @param[in] materialName
 * @return
 */
static qboolean ParseAddNormals(char **text, byte **pic, int *width, int *height, int *bits, const char *materialName)
{
	char *token;
	byte *pic2;
	int  width2, height2;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != '(')
	{
		Ren_Warning("WARNING: expecting '(', found '%s' for addNormals\n", token);
		return qfalse;
	}

	R_LoadImage(text, pic, width, height, bits, materialName);
	if (!*pic)
	{
		Ren_Warning("WARNING: failed loading of first image for addNormals\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ',')
	{
		Ren_Warning("WARNING: no matching ',' found for addNormals\n");
		return qfalse;
	}

	R_LoadImage(text, &pic2, &width2, &height2, bits, materialName);
	if (!pic2)
	{
		Ren_Warning("WARNING: failed loading of second image for addNormals\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ')')
	{
		Ren_Warning("WARNING: expecting ')', found '%s' for addNormals\n", token);
	}

	if (*width != width2 || *height != height2)
	{
		Ren_Warning("WARNING: images for addNormals have different dimensions (%i x %i != %i x %i)\n",
		            *width, *height, width2, height2);

		Com_Dealloc(pic2);
		return qfalse;
	}

	R_AddNormals(*pic, pic2, *width, *height);

	Com_Dealloc(pic2);

	*bits &= ~IF_ALPHA;
	*bits |= IF_NORMALMAP;

	return qtrue;
}

/**
 * @brief ParseInvertAlpha
 * @param[in,out] text
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[out] bits
 * @param[in] materialName
 * @return
 */
static qboolean ParseInvertAlpha(char **text, byte **pic, int *width, int *height, int *bits, const char *materialName)
{
	char *token;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != '(')
	{
		Ren_Warning("WARNING: expecting '(', found '%s' for invertAlpha\n", token);
		return qfalse;
	}

	R_LoadImage(text, pic, width, height, bits, materialName);
	if (!*pic)
	{
		Ren_Warning("WARNING: failed loading of image for invertAlpha\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ')')
	{
		Ren_Warning("WARNING: expecting ')', found '%s' for invertAlpha\n", token);
		return qfalse;
	}

	R_InvertAlpha(*pic, *width, *height);

	return qtrue;
}

/**
 * @brief ParseInvertColor
 * @param[in,out] text
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[out] bits
 * @param[in] materialName
 * @return
 */
static qboolean ParseInvertColor(char **text, byte **pic, int *width, int *height, int *bits, const char *materialName)
{
	char *token;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != '(')
	{
		Ren_Warning("WARNING: expecting '(', found '%s' for invertColor\n", token);
		return qfalse;
	}

	R_LoadImage(text, pic, width, height, bits, materialName);
	if (!*pic)
	{
		Ren_Warning("WARNING: failed loading of image for invertColor\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ')')
	{
		Ren_Warning("WARNING: expecting ')', found '%s' for invertColor\n", token);
		return qfalse;
	}

	R_InvertColor(*pic, *width, *height);

	return qtrue;
}

/**
 * @brief ParseMakeIntensity
 * @param[in,out] text
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[out] bits
 * @param[in] materialName
 * @return
 */
static qboolean ParseMakeIntensity(char **text, byte **pic, int *width, int *height, int *bits, const char *materialName)
{
	char *token;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != '(')
	{
		Ren_Warning("WARNING: expecting '(', found '%s' for makeIntensity\n", token);
		return qfalse;
	}

	R_LoadImage(text, pic, width, height, bits, materialName);
	if (!*pic)
	{
		Ren_Warning("WARNING: failed loading of image for makeIntensity\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ')')
	{
		Ren_Warning("WARNING: expecting ')', found '%s' for makeIntensity\n", token);
		return qfalse;
	}

	R_MakeIntensity(*pic, *width, *height);

	*bits &= ~IF_ALPHA;
	*bits &= ~IF_NORMALMAP;

	return qtrue;
}

/**
 * @brief ParseMakeAlpha
 * @param[in,out] text
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[out] bits
 * @param[in] materialName
 * @return
 */
static qboolean ParseMakeAlpha(char **text, byte **pic, int *width, int *height, int *bits, const char *materialName)
{
	char *token;

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != '(')
	{
		Ren_Warning("WARNING: expecting '(', found '%s' for makeAlpha\n", token);
		return qfalse;
	}

	R_LoadImage(text, pic, width, height, bits, materialName);
	if (!*pic)
	{
		Ren_Warning("WARNING: failed loading of image for makeAlpha\n");
		return qfalse;
	}

	token = COM_ParseExt2(text, qfalse);
	if (token[0] != ')')
	{
		Ren_Warning("WARNING: expecting ')', found '%s' for makeAlpha\n", token);
		return qfalse;
	}

	R_MakeAlpha(*pic, *width, *height);

	//*bits |= IF_ALPHA;
	*bits &= IF_NORMALMAP;

	return qtrue;
}

/**
 * @brief This is a hack to get the common imageloaders working properly
 * @param[in] size
 * @param[in] bufferType
 * @param[in] filename
 * @return
 */
void *R_GetImageBuffer(int size, bufferMemType_t bufferType, const char *filename)
{
	void *buf = NULL;

	if (bufferType != BUFFER_IMAGE)
	{
		Ren_Fatal("R_GetImageBuffer in the new renderer is called improperly\n");
	}

	buf = Com_Allocate(size);

	if (!buf)
	{
		Ren_Drop("R_GetImageBuffer: unable to allocate buffer for image %s with size: %i\n", filename, size);
	}

	return buf;
}

/**
 * @brief Loads any of the supported image types into a canonical
 * 32 bit format.
 * @param[in,out] buffer
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[in,out] bits
 * @param[in] materialName
 */
static void R_LoadImage(char **buffer, byte **pic, int *width, int *height, int *bits, const char *materialName)
{
	char *token;

	*pic    = NULL;
	*width  = 0;
	*height = 0;

	// was COM_ParseExt2 which is too strict and drops valid/used chars  in image names ('+' in map et_carentan)
	token = COM_ParseExt(buffer, qfalse);
	if (!token[0])
	{
		Ren_Developer("WARNING: NULL parameter in R_LoadImage for material/shader '%s'\n", materialName);
		return;
	}

	//Ren_Print("R_LoadImage: token '%s'\n", token);

	// heightMap(<map>, <float>)  Turns a grayscale height map into a normal map. <float> varies the bumpiness
	if (!Q_stricmp(token, "heightMap"))
	{
		if (!ParseHeightMap(buffer, pic, width, height, bits, materialName))
		{
			if (materialName && materialName[0] != '\0')
			{
				Ren_Warning("WARNING: failed to parse heightMap(<map>, <float>) expression for shader '%s'\n", materialName);
			}
		}
	}
	// displaceMap(<map>, <map>)  Sets the alpha channel to an average of the second image's RGB channels.
	else if (!Q_stricmp(token, "displaceMap"))
	{
		if (!ParseDisplaceMap(buffer, pic, width, height, bits, materialName))
		{
			if (materialName && materialName[0] != '\0')
			{
				Ren_Warning("WARNING: failed to parse displaceMap(<map>, <map>) expression for shader '%s'\n", materialName);
			}
		}
	}
	// addNormals(<map>, <map>)  Adds two normal maps together. Result is normalized.
	else if (!Q_stricmp(token, "addNormals"))
	{
		if (!ParseAddNormals(buffer, pic, width, height, bits, materialName))
		{
			if (materialName && materialName[0] != '\0')
			{
				Ren_Warning("WARNING: failed to parse addNormals(<map>, <map>) expression for shader '%s'\n", materialName);
			}
		}
	}
	// smoothNormals(<map>)  Does a box filter on the normal map, and normalizes the result.
	else if (!Q_stricmp(token, "smoothNormals"))
	{
		Ren_Warning("WARNING: smoothNormals(<map>) keyword not supported\n");
	}
	// add(<map>, <map>)  Adds two images without normalizing the result
	else if (!Q_stricmp(token, "add"))
	{
		Ren_Warning("WARNING: add(<map>, <map>) keyword not supported\n");
	}
	// scale(<map>, <float> [,float] [,float] [,float])  Scales the RGBA by the specified factors. Defaults to 0.
	else if (!Q_stricmp(token, "scale"))
	{
		Ren_Warning("WARNING: scale(<map>, <float> [,float] [,float] [,float]) keyword not supported\n");
	}
	// invertAlpha(<map>)  Inverts the alpha channel (0 becomes 1, 1 becomes 0)
	else if (!Q_stricmp(token, "invertAlpha"))
	{
		if (!ParseInvertAlpha(buffer, pic, width, height, bits, materialName))
		{
			if (materialName && materialName[0] != '\0')
			{
				Ren_Warning("WARNING: failed to parse invertAlpha(<map>) expression for shader '%s'\n", materialName);
			}
		}
	}
	// invertColor(<map>)  Inverts the R, G, and B channels
	else if (!Q_stricmp(token, "invertColor"))
	{
		if (!ParseInvertColor(buffer, pic, width, height, bits, materialName))
		{
			if (materialName && materialName[0] != '\0')
			{
				Ren_Warning("WARNING: failed to parse invertColor(<map>) expression for shader '%s'\n", materialName);
			}
		}
	}
	// makeIntensity(<map>)  Copies the red channel to the G, B, and A channels
	else if (!Q_stricmp(token, "makeIntensity"))
	{
		if (!ParseMakeIntensity(buffer, pic, width, height, bits, materialName))
		{
			if (materialName && materialName[0] != '\0')
			{
				Ren_Warning("WARNING: failed to parse makeIntensity(<map>) expression for shader '%s'\n", materialName);
			}
		}
	}
	// makeAlpha(<map>)  Sets the alpha channel to an average of the RGB channels. Sets the RGB channels to white.
	else if (!Q_stricmp(token, "makeAlpha"))
	{
		if (!ParseMakeAlpha(buffer, pic, width, height, bits, materialName))
		{
			if (materialName && materialName[0] != '\0')
			{
				Ren_Warning("WARNING: failed to parse makeAlpha(<map>) expression for shader '%s'\n", materialName);
			}
		}
	}
	else
	{
		//qboolean   orgNameFailed = qfalse;
		int i;
		//const char *ext;
		char filename[MAX_QPATH];
		char *altName;
		byte alphaByte;

		// clear alpha of normalmaps for displacement mapping
		if (*bits & IF_NORMALMAP)
		{
			alphaByte = 0x00;
		}
		else
		{
			alphaByte = 0xFF;
		}
		/*
		Q_strncpyz(filename, token, sizeof(filename));

		ext = COM_GetExtension(filename);

		if (*ext)
		{
		    // look for the correct loader and use it
		    for (i = 0; i < numImageLoaders; i++)
		    {
		        if (!Q_stricmp(ext, imageLoaders[i].ext))
		        {
		            // load
		            imageLoaders[i].ImageLoader(filename, pic, width, height, alphaByte);
		            break;
		        }
		    }

		    // a loader was found
		    if (i < numImageLoaders)
		    {
		        if (!*pic)
		        {
		            // loader failed, most likely because the file isn't there;
		            // try again without the extension
		            orgNameFailed = qtrue;
		            COM_StripExtension(token, filename, MAX_QPATH);
		        }
		        else
		        {
		            // something loaded
		            return;
		        }
		    }
		}

		// try and find a suitable match using all the image formats supported
		for (i = 0; i < numImageLoaders; i++)
		{
		    char *altName = va("%s.%s", filename, imageLoaders[i].ext);

		    // load
		    imageLoaders[i].ImageLoader(altName, pic, width, height, alphaByte);

		    if (*pic)
		    {
		        if (orgNameFailed)
		        {
		            //Ren_Developer( "WARNING: %s not present, using %s instead\n", token, altName);
		        }
		        break;
		    }
		}
		*/
		Q_strncpyz(filename, token, MAX_QPATH);

		COM_StripExtension(token, filename, MAX_QPATH);

		// Try and find a suitable match using all the image formats supported
		// Searching is done in this order: tga, jp(e)g, png, pcx, bmp
		for (i = 0; i < numImageLoaders; i++)
		{
			altName = va("%s.%s", filename, imageLoaders[i].ext);

			// Check if file exists
			if (ri.FS_FOpenFileRead(altName, NULL, qfalse) > 0)
			{
				// Load
				imageLoaders[i].ImageLoader(altName, pic, width, height, alphaByte);
			}

			if (*pic)
			{
				break;
			}
		}

		if (!*pic)
		{
			// Loader failed, most likely because the file isn't there
			Ren_Developer("WARNING: %s not present in any supported image format\n", filename);
		}
	}
}

/**
 * @brief R_ImageCopyBack
 * @param[in] image
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 */
void R_ImageCopyBack(image_t *image, int x, int y, int width, int height)
{
	GL_Bind(image);

	if (glIsTexture(image->texnum) == GL_FALSE)
	{
		glReadBuffer(GL_BACK);

		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, width, height, 0);

		// these shouldn't be necessary if the image was initialized properly
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		//Fast path
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x, y, width, height);
	}
}

/**
 * @brief Finds or loads the given image.
 * @param[in] imageName
 * @param[in] bits
 * @param[in] filterType
 * @param[in] wrapType
 * @param[in] materialName
 * @return NULL if it fails, not a default image.
 */
image_t *R_FindImageFile(const char *imageName, int bits, filterType_t filterType, wrapType_t wrapType, const char *materialName)
{
	image_t *image = NULL;
	int     width  = 0, height = 0;
	byte    *pic   = NULL;
	long    hash;
	char    buffer[1024];
	//char          ddsName[1024];
	char          *buffer_p;
	unsigned long diff;

	if (!imageName)
	{
		return NULL;
	}

	Q_strncpyz(buffer, imageName, sizeof(buffer));
	hash = generateHashValue(buffer);

	//Ren_Print("R_FindImageFile: buffer '%s'\n", buffer);

	// see if the image is already loaded
	for (image = r_imageHashTable[hash]; image; image = image->next)
	{
		if (!Q_stricmpn(buffer, image->name, sizeof(image->name)))
		{
			// the white image can be used with any set of parms, but other mismatches are errors
			if (Q_stricmp(buffer, "_white"))
			{
				diff = bits ^ image->bits;

				/*
				   if(diff & IF_NOMIPMAPS)
				   {
				   Ren_Developer( "WARNING: reused image %s with mixed mipmap parm\n", name);
				   }
				 */

				if (diff & IF_NOPICMIP)
				{
					Ren_Developer("WARNING: reused image '%s' with mixed allowPicmip parm for shader '%s\n", imageName, materialName);
				}

				if (image->wrapType != wrapType)
				{
					Ren_Print("WARNING: reused image '%s' with mixed glWrapType parm for shader '%s'\n", imageName, materialName);
				}
			}
			return image;
		}
	}

#if 0
	if (glConfig.textureCompression == TC_S3TC && !(bits & IF_NOCOMPRESSION) && Q_stricmpn(imageName, "fonts", 5))
	{
		Q_strncpyz(ddsName, imageName, sizeof(ddsName));
		COM_StripExtension(ddsName, ddsName, sizeof(ddsName));
		Q_strcat(ddsName, sizeof(ddsName), ".dds");

		// try to load a customized .dds texture
		image = R_LoadDDSImage(ddsName, bits, filterType, wrapType);
		if (image != NULL)
		{
			Ren_Print("found custom .dds '%s'\n", ddsName);
			return image;
		}
	}
#endif

#if 0
	else if (r_tryCachedDDSImages->integer && !(bits & IF_NOCOMPRESSION) && Q_strncasecmp(name, "fonts", 5))
	{
		Q_strncpyz(ddsName, "dds/", sizeof(ddsName));
		Q_strcat(ddsName, sizeof(ddsName), name);
		COM_StripExtension(ddsName, ddsName, sizeof(ddsName));
		Q_strcat(ddsName, sizeof(ddsName), ".dds");

		// try to load a cached .dds texture from the XreaL/<mod>/dds/ folder
		image = R_LoadDDSImage(ddsName, bits, filterType, wrapType);
		if (image != NULL)
		{
			Ren_Print("found cached .dds '%s'\n", ddsName);
			return image;
		}
	}
#endif

	// load the pic from disk
	buffer_p = &buffer[0];
	R_LoadImage(&buffer_p, &pic, &width, &height, &bits, materialName);
	if (pic == NULL)
	{
		// FIXME !!!
		// this will occure in mods for default light shaders until we add our material pk3 to the common search path
		// or modders add the light shaders (and related images) to their mod.
		// update: set to dev print - FIXME: there are more cases for some models on vanilla maps! 
		Ren_Developer("WARNING R_FindImageFile: can't load material '%s'\n", materialName);
		return NULL;
	}

	if (((width - 1) & width) || ((height - 1) & height))
	{
		Ren_Warning("WARNING: Image not power of 2 scaled: %s   %i:%i\n", materialName, width, height);
		return NULL;
	}

	if (bits & IF_LIGHTMAP)
	{
		R_ProcessLightmap(pic, 4, width, height, pic);

		bits |= IF_NOCOMPRESSION;
	}

#if 0
	//if(r_tryCachedDDSImages->integer && !(bits & IF_NOCOMPRESSION) && Q_strncasecmp(name, "fonts", 5))
	{
		// try to cache a .dds texture to the XreaL/<mod>/dds/ folder
		SavePNG(ddsName, pic, width, height, 4, qtrue);
	}
#endif

	image = R_CreateImage((char *)buffer, pic, width, height, bits, filterType, wrapType);
	Com_Dealloc(pic);
	return image;
}

/**
 * @brief R_SwapPixel
 * @param[in,out] inout
 * @param[in] x
 * @param[in] y
 * @param[in] x2
 * @param[in] y2
 * @param[in] width
 * @param height - unused
 */
static ID_INLINE void R_SwapPixel(byte *inout, int x, int y, int x2, int y2, int width, int height)
{
	byte color[4];
	byte color2[4];

	color[0] = inout[4 * (y * width + x) + 0];
	color[1] = inout[4 * (y * width + x) + 1];
	color[2] = inout[4 * (y * width + x) + 2];
	color[3] = inout[4 * (y * width + x) + 3];

	color2[0] = inout[4 * (y2 * width + x2) + 0];
	color2[1] = inout[4 * (y2 * width + x2) + 1];
	color2[2] = inout[4 * (y2 * width + x2) + 2];
	color2[3] = inout[4 * (y2 * width + x2) + 3];

	inout[4 * (y * width + x) + 0] = color2[0];
	inout[4 * (y * width + x) + 1] = color2[1];
	inout[4 * (y * width + x) + 2] = color2[2];
	inout[4 * (y * width + x) + 3] = color2[3];

	inout[4 * (y2 * width + x2) + 0] = color[0];
	inout[4 * (y2 * width + x2) + 1] = color[1];
	inout[4 * (y2 * width + x2) + 2] = color[2];
	inout[4 * (y2 * width + x2) + 3] = color[3];
}

/**
 * @brief R_Flip
 * @param[in,out] in
 * @param[in] width
 * @param[in] height
 */
static void R_Flip(byte *in, int width, int height)
{
	int x, y;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width / 2; x++)
		{
			R_SwapPixel(in, x, y, (width - 1 - x), y, width, height);
		}
	}
}

/**
 * @brief R_Flop
 * @param[in,out] in
 * @param[in] width
 * @param[in] height
 */
static void R_Flop(byte *in, int width, int height)
{
	int x, y;

	for (y = 0; y < height / 2; y++)
	{
		for (x = 0; x < width; x++)
		{
			R_SwapPixel(in, x, y, x, (height - 1 - y), width, height);
		}
	}
}

/**
 * @brief R_Rotate
 * @param[in,out] in
 * @param[in] width
 * @param[in] height
 * @param[in] degrees
 */
static void R_Rotate(byte *in, int width, int height, int degrees)
{
	byte color[4];
	int  x, y, x2, y2;
	byte *out, *tmp;

	tmp = (byte *)Com_Allocate(width * height * 4);
	Com_Memset(tmp, 0, width * height * 4);

	// rotate into tmp buffer
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			color[0] = in[4 * (y * width + x) + 0];
			color[1] = in[4 * (y * width + x) + 1];
			color[2] = in[4 * (y * width + x) + 2];
			color[3] = in[4 * (y * width + x) + 3];

			if (degrees == 90)
			{
				x2 = y;
				y2 = (height - (1 + x));

				tmp[4 * (y2 * width + x2) + 0] = color[0];
				tmp[4 * (y2 * width + x2) + 1] = color[1];
				tmp[4 * (y2 * width + x2) + 2] = color[2];
				tmp[4 * (y2 * width + x2) + 3] = color[3];
			}
			else if (degrees == -90)
			{
				x2 = (width - (1 + y));
				y2 = x;

				tmp[4 * (y2 * width + x2) + 0] = color[0];
				tmp[4 * (y2 * width + x2) + 1] = color[1];
				tmp[4 * (y2 * width + x2) + 2] = color[2];
				tmp[4 * (y2 * width + x2) + 3] = color[3];
			}
			else
			{
				tmp[4 * (y * width + x) + 0] = color[0];
				tmp[4 * (y * width + x) + 1] = color[1];
				tmp[4 * (y * width + x) + 2] = color[2];
				tmp[4 * (y * width + x) + 3] = color[3];
			}

		}
	}

	// copy back to input
	out = in;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			out[4 * (y * width + x) + 0] = tmp[4 * (y * width + x) + 0];
			out[4 * (y * width + x) + 1] = tmp[4 * (y * width + x) + 1];
			out[4 * (y * width + x) + 2] = tmp[4 * (y * width + x) + 2];
			out[4 * (y * width + x) + 3] = tmp[4 * (y * width + x) + 3];
		}
	}

	Com_Dealloc(tmp);
}

/**
 * @brief Copies between a smaller image and a larger image.
 * Last flag controls is copy in or out of larger image.
 *
 * @param[in,out] dest
 * @param[in] destx
 * @param[in] desty
 * @param[in] destw
 * @param[in] desth
 * @param[in,out] src
 * @param[in] srcw
 * @param[in] srch
 * @param[in] bytes
 * @param[in] in
 *
 * @note e.g.
 * dest = Com_Allocate(4*4*channels);
 * [________]
 * [________]
 * [________]
 * [________]
 * src = Com_Allocate(2*2*channels);
 * [____]
 * [____]
 * R_SubImageCpy(dest, 0, 0, 4, 4, src, 2, 2, channels, qtrue);
 * [____]___]
 * [____]___]
 * [________]
 * [________]
 * ===============
*/
void R_SubImageCpy(byte *dest, size_t destx, size_t desty, size_t destw, size_t desth, byte *src, size_t srcw, size_t srch, size_t bytes, qboolean in)
{
	size_t s_rowBytes = srcw * bytes;
	size_t d_rowBytes = destw * bytes;
	byte   *d         = dest + ((destx * bytes) + (desty * d_rowBytes));
	byte   *d_max     = dest + (destw * desth * bytes) - s_rowBytes;
	byte   *s         = src;
	byte   *s_max     = src + (srcw * srch * bytes) - s_rowBytes;

	while ((s <= s_max) && (d <= d_max))
	{
		if (in)
		{
			Com_Memcpy(d, s, s_rowBytes);
		}
		else
		{
			Com_Memcpy(s, d, s_rowBytes);
		}
		d += d_rowBytes;
		s += s_rowBytes;
	}
}

/**
 * @brief Finds or loads the given image.
 * @param[in] imageName
 * @param[in] bits
 * @param[in] filterType
 * @param[in] wrapType
 * @param[in] materialName
 * @return NULL if it fails, not a default image.
 *
 * @note Fear the use of goto
 */
image_t *R_FindCubeImage(const char *imageName, int bits, filterType_t filterType, wrapType_t wrapType, const char *materialName)
{
	int         i;
	image_t     *image = NULL;
	int         width  = 0, height = 0;
	byte        *pic[6];
	long        hash;
	//static char *openglSuffices[6] = { "px", "nx", "py", "ny", "pz", "nz" };

	/*
	    convert $1_forward.tga -flip -rotate 90 $1_px.png
	    convert $1_back.tga -flip -rotate -90 $1_nx.png

	    convert $1_left.tga -flip $1_py.png
	    convert $1_right.tga -flop $1_ny.png

	    convert $1_up.tga -flip -rotate 90 $1_pz.png
	    convert $1_down.tga -flop -rotate -90 $1_nz.png
	 */
	//static char     *doom3Suffices[6] = { "forward", "back", "left", "right", "up", "down" };
	//static qboolean doom3FlipX[6]     = { qtrue, qtrue, qfalse, qtrue, qtrue, qfalse };
	//static qboolean doom3FlipY[6]     = { qfalse, qfalse, qtrue, qfalse, qfalse, qtrue };
	//static int      doom3Rot[6]       = { 90, -90, 0, 0, 90, -90 };

	/*
	    convert $1_rt.tga -flip -rotate 90 $1_px.tga
	    convert $1_lf.tga -flip -rotate -90 $1_nx.tga

	    convert $1_bk.tga -flip $1_py.tga
	    convert $1_ft.tga -flop $1_ny.tga

	    convert $1_up.tga -flip -rotate 90 $1_pz.tga
	    convert $1_dn.tga -flop -rotate -90 $1_nz.tga
	 */
	int             firstImageWidth;  // some ET sky textures are not of same dimension (suffix dn/down/bottom in most cases to safe space)
	int             firstImageHeight; // get dimensions from first image 'rt' and force this value for the cube map
	char			fistFilename_p[1024]; // FIXME
	static char     *quakeSuffices[6] = { "rt", "lf", "bk", "ft", "up", "dn" };
	static qboolean quakeFlipX[6]     = { qtrue, qtrue, qfalse, qtrue, qtrue, qfalse };
	static qboolean quakeFlipY[6]     = { qfalse, qfalse, qtrue, qfalse, qfalse, qtrue };
	static int      quakeRot[6]       = { 90, -90, 0, 0, 90, -90 };
	int             bitsIgnore        = IF_NONE;
	char            buffer[1024], filename[1024];
	//char ddsName[1024];
	char *filename_p;

	if (!imageName)
	{
		return NULL;
	}

	Q_strncpyz(buffer, imageName, sizeof(buffer));
	hash = generateHashValue(buffer);

	// see if the image is already loaded
	for (image = r_imageHashTable[hash]; image; image = image->next)
	{
		if (!Q_stricmp(buffer, image->name))
		{
			return image;
		}
	}

#if 0
	if (glConfig.textureCompression == TC_S3TC && !(bits & IF_NOCOMPRESSION) && Q_stricmpn(imageName, "fonts", 5))
	{
		Q_strncpyz(ddsName, imageName, sizeof(ddsName));
		COM_StripExtension(ddsName, ddsName, sizeof(ddsName));
		Q_strcat(ddsName, sizeof(ddsName), ".dds");

		// try to load a customized .dds texture
		image = R_LoadDDSImage(ddsName, bits, filterType, wrapType);
		if (image != NULL)
		{
			Ren_Print("found custom .dds '%s'\n", ddsName);
			return image;
		}
	}
#endif

#if 0
	else if (r_tryCachedDDSImages->integer && !(bits & IF_NOCOMPRESSION) && Q_strncasecmp(name, "fonts", 5))
	{
		Q_strncpyz(ddsName, "dds/", sizeof(ddsName));
		Q_strcat(ddsName, sizeof(ddsName), name);
		COM_StripExtension(ddsName, ddsName, sizeof(ddsName));
		Q_strcat(ddsName, sizeof(ddsName), ".dds");

		// try to load a cached .dds texture from the XreaL/<mod>/dds/ folder
		image = R_LoadDDSImage(ddsName, bits, filterType, wrapType);
		if (image != NULL)
		{
			Ren_Print("found cached .dds '%s'\n", ddsName);
			return image;
		}
	}
#endif

	for (i = 0; i < 6; i++)
	{
		pic[i] = NULL;
	}

//tryQuakeSuffices:
	for (i = 0; i < 6; i++)
	{
		Com_sprintf(filename, sizeof(filename), "%s_%s", buffer, quakeSuffices[i]);

		filename_p = &filename[0];
		R_LoadImage(&filename_p, &pic[i], &width, &height, &bitsIgnore, materialName);

		if (!pic[i] || width != height)
		{
			image = NULL;
			Ren_Warning("Warning R_FindCubeImage: Invalid image %s for cube %s\n", filename, imageName);
			goto skipCubeImage;
		}

		if (i == 0)
		{
			firstImageWidth  = width;
			firstImageHeight = height;
			Com_sprintf(fistFilename_p, sizeof(filename), "%s_%s", buffer, quakeSuffices[i]);
		}
		else
		{
			if (width != firstImageWidth || height != firstImageHeight)
			{
				Ren_Warning("Warning R_FindCubeImage: Image %s has different dimension [%i/%i] - [%i/%i] expected\n", filename, width, height, firstImageWidth, firstImageHeight);
				Com_Dealloc(pic[i]);
				// hack: load the first image instead
				// FIXME: better fix - scale the image to proper dimension
				filename_p = &fistFilename_p[0];
				R_LoadImage(&filename_p, &pic[i], &width, &height, &bitsIgnore, materialName);
			}
		}

		if (quakeFlipX[i])
		{
			R_Flip(pic[i], width, height);
		}

		if (quakeFlipY[i])
		{
			R_Flop(pic[i], width, height);
		}

		R_Rotate(pic[i], width, height, quakeRot[i]);
	}
/*
	goto createCubeImage;

tryOpenGLSuffices:
	for (i = 0; i < 6; i++)
	{
		Com_sprintf(filename, sizeof(filename), "%s_%s", buffer, openglSuffices[i]);

		filename_p = &filename[0];
		R_LoadImage(&filename_p, &pic[i], &width, &height, &bitsIgnore, materialName);

		if (!pic[i] || width != height)
		{
			image = NULL;
			goto tryDoom3Suffices;
		}
	}
	goto createCubeImage;


tryDoom3Suffices:
	for (i = 0; i < 6; i++)
	{
		Com_sprintf(filename, sizeof(filename), "%s_%s", buffer, doom3Suffices[i]);

		filename_p = &filename[0];
		R_LoadImage(&filename_p, &pic[i], &width, &height, &bitsIgnore, materialName);

		if (!pic[i] || width != height)
		{
			image = NULL;
			goto tryQuakeSuffices;
		}

		if (doom3FlipX[i])
		{
			R_Flip(pic[i], width, height);
		}

		if (doom3FlipY[i])
		{
			R_Flop(pic[i], width, height);
		}

		R_Rotate(pic[i], width, height, doom3Rot[i]);
	}
	//goto createCubeImage;

createCubeImage:
*/
	image = R_CreateCubeImage((char *)buffer, (const byte **)pic, width, height, bits, filterType, wrapType);

skipCubeImage:
	for (i = 0; i < 6; i++)
	{
		if (pic[i])
		{
			Com_Dealloc(pic[i]);
		}
	}
	return image;
}

/**
 * @brief R_InitFogTable
 */
void R_InitFogTable(void)
{
	int i;

	for (i = 0; i < FOG_TABLE_SIZE; i++)
	{
		tr.fogTable[i] = pow((double)i / (FOG_TABLE_SIZE - 1), DEFAULT_FOG_EXP_DENSITY);
	}
}

/**
 * @brief This is called for each texel of the fog texture on startup
 * and for each vertex of transparent shaders in fog dynamically
 * @param[in] s
 * @param[in] t
 * @return A 0.0 to 1.0 fog density value.
 */
float R_FogFactor(float s, float t)
{
	float d;

	s -= 1.0f / 512;
	if (s < 0)
	{
		return 0;
	}
	if (t < 1.0f / 32)
	{
		return 0;
	}
	if (t < 31.0f / 32)
	{
		s *= (t - 1.0f / 32.0f) / (30.0f / 32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if (s > 1.0f)
	{
		s = 1.0f;
	}

	d = tr.fogTable[(int)(s * (FOG_TABLE_SIZE - 1))];

	return d;
}

#define FOG_S   256
#define FOG_T   32

/**
 * @brief R_CreateFogImage
 */
static void R_CreateFogImage(void)
{
	int   x, y;
	byte  *data;
	float d;

	data = (byte *)ri.Hunk_AllocateTempMemory(FOG_S * FOG_T * 4);

	// S is distance, T is depth
	for (x = 0; x < FOG_S; x++)
	{
		for (y = 0; y < FOG_T; y++)
		{
			d = R_FogFactor((x + 0.5f) / FOG_S, (y + 0.5f) / FOG_T);

			data[(y * FOG_S + x) * 4 + 0] = data[(y * FOG_S + x) * 4 + 1] = data[(y * FOG_S + x) * 4 + 2] = 255;
			data[(y * FOG_S + x) * 4 + 3] = 255 * d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	//And Open GL3 does it for real, was WT_CLAMP
	tr.fogImage = R_CreateImage("_fog", (byte *) data, FOG_S, FOG_T, IF_NOPICMIP, FT_LINEAR, WT_EDGE_CLAMP);
	ri.Hunk_FreeTempMemory(data);
}

#define DEFAULT_SIZE    128

/**
 * @brief R_CreateDefaultImage
 */
static void R_CreateDefaultImage(void)
{
	int  x, y;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset(data, 0, sizeof(data));
	for (x = 0; x < DEFAULT_SIZE; x++)
	{
		for (y = 0; y < 2; y++)
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

	tr.defaultImage = R_CreateImage("_default", (byte *) data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NOPICMIP, FT_DEFAULT, WT_REPEAT);
}

/**
 * @brief R_CreateRandomNormalsImage
 *        unused  - see randomNormalsImage
static void R_CreateRandomNormalsImage(void)
{
	int  x, y;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset(data, 32, sizeof(data));
	for (x = 0; x < DEFAULT_SIZE; x++)
	{
		for (y = 0; y < DEFAULT_SIZE; y++)
		{
			vec3_t n;
			float  r, angle;

			r     = random();
			angle = M_TAU_F * r; // / 360.0;

			VectorSet(n, cos(angle), sin(angle), r);
			VectorNormalize(n);

			//VectorSet(n, crandom(), crandom(), crandom());

			data[y][x][0] = (byte) (128 + 127 * n[0]);
			data[y][x][1] = (byte) (128 + 127 * n[1]);
			data[y][x][2] = (byte) (128 + 127 * n[2]);
			data[y][x][3] = 255;
		}
	}
	tr.randomNormalsImage = R_CreateImage("_randomNormals", (byte *) data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NOPICMIP, FT_DEFAULT, WT_REPEAT);
}
*/

/**
 * @brief R_CreateNoFalloffImage
 */
static void R_CreateNoFalloffImage(void)
{
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// we use a solid white image instead of disabling texturing
	Com_Memset(data, 255, sizeof(data));
	tr.noFalloffImage = R_CreateImage("_noFalloff", (byte *) data, 8, 8, IF_NOPICMIP, FT_LINEAR, WT_EDGE_CLAMP);
}

#define ATTENUATION_XY_SIZE 128

/**
 * @brief R_CreateAttenuationXYImage
 */
static void R_CreateAttenuationXYImage(void)
{
	float d;
	int   x, y;
	byte  data[ATTENUATION_XY_SIZE][ATTENUATION_XY_SIZE][4];
	int   b;

	// make a centered inverse-square falloff blob for dynamic lighting
	for (x = 0; x < ATTENUATION_XY_SIZE; x++)
	{
		for (y = 0; y < ATTENUATION_XY_SIZE; y++)
		{
			d = (ATTENUATION_XY_SIZE / 2 - 0.5f - x) * (ATTENUATION_XY_SIZE / 2 - 0.5f - x) +
			    (ATTENUATION_XY_SIZE / 2 - 0.5f - y) * (ATTENUATION_XY_SIZE / 2 - 0.5f - y);
			b = 4000 / d;
			if (b > 255)
			{
				b = 255;
			}
			else if (b < 75)
			{
				b = 0;
			}
			data[y][x][0] = data[y][x][1] = data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}
	tr.attenuationXYImage =
	    R_CreateImage("_attenuationXY", (byte *) data, ATTENUATION_XY_SIZE, ATTENUATION_XY_SIZE, IF_NOPICMIP, FT_LINEAR,
	                  WT_EDGE_CLAMP);
}

/**
 * @brief R_CreateCubeRenderImage
 * @param[in] name
 * @param[in] color
 * @param[in] width
 * @param[in] height
 * @param[in] bits
 * @param[in] filterType
 * @param[in] wrapType
 * @return
 */
image_t *R_CreateCubeRenderImage(const char *name, int color, int width, int height, int bits, filterType_t filterType, wrapType_t wrapType)
{
	int     i;
	byte    *data[6];
	image_t *ret = NULL;

	for (i = 0; i < 6; i++)
	{
		data[i] = (byte *)ri.Hunk_AllocateTempMemory(width * height * 4);
		if (!data[i])
		{
			Ren_Fatal("R_CreateCubeRenderImage: failed to create cube render image for %s\n", name);
			return NULL;
		}

		Com_Memset(data[i], color, width * height * 4);
	}

	ret = R_CreateCubeImage(name, (const byte **)data, width, height, IF_NOPICMIP | bits, filterType, wrapType);

	for (i = 5; i >= 0; i--)
	{
		ri.Hunk_FreeTempMemory(data[i]);
	}

	return ret;
}

/**
 * @brief R_CreateRenderImageSize
 * @param[in] name
 * @param[in] width
 * @param[in] height
 * @param[in] bits
 * @param[in] filterType
 * @param[in] wrapType
 * @return
 */
image_t *R_CreateRenderImageSize(const char *name, int width, int height, int bits, filterType_t filterType, wrapType_t wrapType)
{
	byte    *data;
	image_t *ret;

	data = (byte *)ri.Hunk_AllocateTempMemory(width * height * 4);
	if (!data)
	{
		Ren_Fatal("R_CreateRenderImageSize: Cannot allocate render image: %s\n", name);
		return NULL;
	}

	ret = R_CreateImage(name, data, width, height, IF_NOPICMIP | bits, filterType, wrapType);

	ri.Hunk_FreeTempMemory(data);

	return ret;
}

/**
 * @brief R_CreateRenderImage
 * @param[in] name
 * @param[in] quat
 * @param[in] bits
 * @param[in] filterType
 * @param[in] wrapType
 * @return
 */
image_t *R_CreateRenderImage(const char *name, qboolean quat, int bits, filterType_t filterType, wrapType_t wrapType)
{
	int width, height;

	if (glConfig2.textureNPOTAvailable)
	{
		width  = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}
	else
	{
		width  = NearestPowerOfTwo(glConfig.vidWidth);
		height = NearestPowerOfTwo(glConfig.vidHeight);
	}

	if (quat)
	{
		width  *= 0.25f;
		height *= 0.25f;
	}

	return R_CreateRenderImageSize(name, width, height, bits, filterType, wrapType);
}

/**
 * @brief R_CreateContrastRenderFBOImage
 */
static void R_CreateContrastRenderFBOImage(void)
{
	if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
	{
		tr.contrastRenderFBOImage = R_CreateRenderImage("_contrastRenderFBO", qtrue, IF_NOCOMPRESSION | IF_RGBA16F, FT_LINEAR, WT_EDGE_CLAMP);
	}
	else
	{
		tr.contrastRenderFBOImage = R_CreateRenderImage("_contrastRenderFBO", qtrue, IF_NOCOMPRESSION, FT_LINEAR, WT_EDGE_CLAMP);
	}
}

/**
 * @brief R_CreateBloomRenderFBOImage
 */
static void R_CreateBloomRenderFBOImage(void)
{
	int i;
	for (i = 0; i < 2; i++)
	{
		if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
		{
			tr.bloomRenderFBOImage[i] = R_CreateRenderImage(va("_bloomRenderFBO%d", i), qtrue, IF_NOCOMPRESSION | IF_RGBA16F, FT_LINEAR, WT_EDGE_CLAMP);
		}
		else
		{
			tr.bloomRenderFBOImage[i] = R_CreateRenderImage(va("_bloomRenderFBO%d", i), qtrue, IF_NOCOMPRESSION, FT_LINEAR, WT_EDGE_CLAMP);
		}
	}
}

/**
 * @brief R_CreateCurrentRenderImage
 */
static void R_CreateCurrentRenderImage(void)
{
	tr.currentRenderImage = R_CreateRenderImage("_currentRender", qfalse, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);
}

/**
 * @brief R_CreateDepthRenderImage
 */
static void R_CreateDepthRenderImage(void)
{
	tr.depthRenderImage = R_CreateRenderImage("_depthRender", qfalse, IF_DEPTH24, FT_NEAREST, WT_EDGE_CLAMP);
}

/**
 * @brief R_CreatePortalRenderImage
 */
static void R_CreatePortalRenderImage(void)
{
	if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
	{
		tr.portalRenderImage = R_CreateRenderImage("_portalRender", qfalse, IF_RGBA16F, FT_NEAREST, WT_EDGE_CLAMP);
	}
	else
	{
		tr.portalRenderImage = R_CreateRenderImage("_portalRender", qfalse, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);
	}
}

/**
 * @brief R_CreateOcclusionRenderFBOImage
 */
static void R_CreateOcclusionRenderFBOImage(void)
{
	tr.occlusionRenderFBOImage = R_CreateRenderImage("_occlusionFBORender", qfalse, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);
}

/**
 * @brief R_CreateDepthToColorFBOImages
 */
static void R_CreateDepthToColorFBOImages(void)
{

	//FIXME which one should we use? Read up!
	/*
	tr.depthToColorBackFacesFBOImage  = R_CreateRenderImage("_depthToColorBackFacesFBORender", qfalse, IF_ALPHA32F, FT_NEAREST, WT_CLAMP);
	tr.depthToColorFrontFacesFBOImage = R_CreateRenderImage("_depthToColorFrontFacesFBORender", qfalse, IF_ALPHA32F, FT_NEAREST, WT_CLAMP);
	 */
	tr.depthToColorBackFacesFBOImage  = R_CreateRenderImage("_depthToColorBackFacesFBORender", qfalse, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);
	tr.depthToColorFrontFacesFBOImage = R_CreateRenderImage("_depthToColorFrontFacesFBORender", qfalse, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);
}

/**
 * @brief R_CreateDownScaleFBOImages
 *
 * @todo Clean up this mess some day ...
 */
static void R_CreateDownScaleFBOImages(void)
{
	byte           *data;
	int				width, height;


	if (glConfig2.textureNPOTAvailable)
	{
		width = glConfig.vidWidth * 0.25f;
		height = glConfig.vidHeight * 0.25f;
	}
	else
	{
		width = NearestPowerOfTwo(glConfig.vidWidth * 0.25f);
		height = NearestPowerOfTwo(glConfig.vidHeight * 0.25f);
	}

	data = ri.Hunk_AllocateTempMemory(width * height * 4);

	if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
	{
		tr.downScaleFBOImage_quarter = R_CreateRenderImageSize("_downScaleFBOImage_quarter", width, height, IF_RGBA16F, FT_NEAREST, WT_EDGE_CLAMP);
	}
	else
	{
		tr.downScaleFBOImage_quarter = R_CreateRenderImageSize("_downScaleFBOImage_quarter", width, height, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);
	}

	if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
	{
		tr.downScaleFBOImage_64x64 = R_CreateRenderImageSize("_downScaleFBOImage_64x64", 64, 64, IF_RGBA16F, FT_NEAREST, WT_EDGE_CLAMP);
	}
	else
	{
		tr.downScaleFBOImage_64x64 = R_CreateRenderImageSize("_downScaleFBOImage_64x64", 64, 64, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);
	}

	ri.Hunk_FreeTempMemory(data);

#if 0

	width = height = 16;
	data = ri.Hunk_AllocateTempMemory(width * height * 4);

	if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
	{
		tr.downScaleFBOImage_16x16 = R_CreateRenderImageSize("_downScaleFBOImage_16x16", 16, 16, IF_RGBA16F, FT_NEAREST, WT_CLAMP);
	}
	else
	{
		tr.downScaleFBOImage_16x16 = R_CreateRenderImageSize("_downScaleFBOImage_16x16", 16, 16, IF_NOCOMPRESSION, FT_NEAREST, WT_CLAMP);
	}

	if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
	{
		tr.downScaleFBOImage_4x4 = R_CreateRenderImageSize("_downScaleFBOImage_4x4", 4, 4, IF_RGBA16F, FT_NEAREST, WT_CLAMP);
	}
	else
	{
		tr.downScaleFBOImage_4x4 = R_CreateRenderImageSize("_downScaleFBOImage_4x4", 4, 4, IF_NOCOMPRESSION, FT_NEAREST, WT_CLAMP);
	}

	if (r_hdrRendering->integer && glConfig2.textureFloatAvailable)
	{
		tr.downScaleFBOImage_1x1 = R_CreateRenderImageSize("_downScaleFBOImage_1x1", 1, 1, IF_RGBA16F, FT_NEAREST, WT_CLAMP);
	}
	else
	{
		tr.downScaleFBOImage_1x1 = R_CreateRenderImageSize("_downScaleFBOImage_1x1", 1, 1, IF_NOCOMPRESSION, FT_NEAREST, WT_CLAMP);
	}

	ri.Hunk_FreeTempMemory(data);
#endif
}

/**
 * @brief R_CreateDeferredRenderFBOImages
 */
static void R_CreateDeferredRenderFBOImages(void)
{
	tr.deferredNormalFBOImage = R_CreateRenderImage("_deferredNormalFBO", qfalse, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);

	if (HDR_ENABLED())
	{
		tr.lightRenderFBOImage = R_CreateRenderImage("_lightRenderFBO", qfalse, IF_RGBA16F, FT_NEAREST, WT_EDGE_CLAMP);
	}
	else
	{
		tr.lightRenderFBOImage = R_CreateRenderImage("_lightRenderFBO", qfalse, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);
	}

	if (HDR_ENABLED())
	{
		tr.deferredRenderFBOImage = R_CreateRenderImage("_deferredRenderFBO", qfalse, IF_RGBA16F, FT_NEAREST, WT_EDGE_CLAMP);
	}
	else
	{
		tr.deferredRenderFBOImage = R_CreateRenderImage("_deferredRenderFBO", qfalse, IF_NOCOMPRESSION, FT_NEAREST, WT_EDGE_CLAMP);
	}
}

/**
 * @brief R_CreateShadowMapFBOImage
 */
static void R_CreateShadowMapFBOImage(void)
{
	int          i, size, bits;
	filterType_t filter = FT_NEAREST;

	if (!glConfig2.textureFloatAvailable || r_shadows->integer < SHADOWING_ESM16)
	{
		return;
	}

	if (r_shadowMapLinearFilter->integer)
	{
		filter = FT_LINEAR;
	}

	for (i = 0; i < MAX_SHADOWMAPS; i++)
	{
		size = shadowMapResolutions[i];

		// we can do the most expensive filtering types with OpenGL 3 hardware
		if (r_shadows->integer == SHADOWING_ESM32)
		{
			bits = IF_ALPHA32F;
		}
		else if (r_shadows->integer == SHADOWING_VSM32)
		{
			bits = IF_LA32F;
		}
		else if (r_shadows->integer == SHADOWING_EVSM32)
		{
			if (r_evsmPostProcess->integer)
			{
				bits = IF_ALPHA32F;
			}
			else
			{
				bits = IF_RGBA32F;
			}
		}
		else
		{
			bits = IF_RGBA16F;
		}

		tr.shadowMapFBOImage[i] = R_CreateRenderImageSize(va("_shadowMapFBO%d", i), size, size, bits, filter, WT_EDGE_CLAMP);
	}

	// sun shadow maps
	for (i = 0; i < MAX_SHADOWMAPS; i++)
	{
		size = sunShadowMapResolutions[i];

		// we can do the most expensive filtering types with OpenGL 3 hardware
		if (r_shadows->integer == SHADOWING_ESM32)
		{
			bits = IF_ALPHA32F;
		}
		else if (r_shadows->integer == SHADOWING_VSM32)
		{
			bits = IF_LA32F;
		}
		else if (r_shadows->integer == SHADOWING_EVSM32)
		{
			if (r_evsmPostProcess->integer)
			{
				bits = IF_DEPTH24;
			}
			else
			{
				bits = IF_RGBA32F;
			}
		}
		else
		{
			bits = IF_RGBA16F;
		}

		tr.sunShadowMapFBOImage[i] = R_CreateRenderImageSize(va("_sunShadowMapFBO%d", i), size, size, bits, filter, WT_EDGE_CLAMP);
	}
}

/**
 * @brief R_CreateShadowCubeFBOImage
 */
static void R_CreateShadowCubeFBOImage(void)
{
	int          j, size, bits;
	filterType_t filter = FT_NEAREST;

	if (!glConfig2.textureFloatAvailable || r_shadows->integer < SHADOWING_ESM16)
	{
		return;
	}

	if (r_shadowMapLinearFilter->integer)
	{
		filter = FT_LINEAR;
	}

	for (j = 0; j < 5; j++)
	{
		size = shadowMapResolutions[j];

		if (r_shadows->integer == SHADOWING_ESM32)
		{
			bits = IF_ALPHA32F;
		}
		else if (r_shadows->integer == SHADOWING_VSM32)
		{
			bits = IF_LA32F;
		}
		else if (r_shadows->integer == SHADOWING_EVSM32)
		{
			if (r_evsmPostProcess->integer)
			{
				bits = IF_ALPHA32F;
			}
			else
			{
				bits = IF_RGBA32F;
			}
		}
		else
		{
			bits = IF_RGBA16F;
		}

		tr.shadowCubeFBOImage[j] = R_CreateCubeRenderImage(va("_shadowCubeFBO%d", j), 0, size, size, bits, filter, WT_EDGE_CLAMP);
	}
}

/**
 * @brief R_CreateColorCubeImages
 */
static void R_CreateColorCubeImages(void)
{
	tr.blackCubeImage = R_CreateCubeRenderImage("_blackCube", 0, REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE, 0, FT_LINEAR, WT_EDGE_CLAMP);
	tr.autoCubeImage  = R_CreateCubeRenderImage("_autoCube", 0, REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE, 0, FT_LINEAR, WT_EDGE_CLAMP);
	tr.whiteCubeImage = R_CreateCubeRenderImage("_whiteCube", 0xFF, REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE, 0, FT_LINEAR, WT_EDGE_CLAMP);
}

/**
 * @brief R_CreateBuiltinImages
 */
void R_CreateBuiltinImages(void)
{
	int   x, y;
	byte  data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	byte  *out;
	float s, value;
	byte  intensity;

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset(data, 255, sizeof(data));
	tr.whiteImage = R_CreateImage("_white", (byte *) data, 8, 8, IF_NOPICMIP, FT_LINEAR, WT_REPEAT);
	//TODO: we need a fake one for this as a specular, specular shouldnt be zero
	//adjusted to 0.2 wich seems pretty fine
	// we use a solid black image instead of disabling texturing
	Com_Memset(data, 0, sizeof(data));
	tr.blackImage = R_CreateImage("_black", (byte *) data, 8, 8, IF_NOPICMIP, FT_LINEAR, WT_REPEAT);


	//this can be used later as a specular created image..
	/*for (x = 0; x < DEFAULT_SIZE; x++)
	{
		for (y = 0; y < DEFAULT_SIZE; y++)
		{
			data[y][x][0] = 123;
			data[y][x][1] = 118;
			data[y][x][2] = 117;
			data[y][x][3] = 255;
		}
	}
	
	tr.SpecImage = R_CreateImage("_Spec", (byte *) data, 8, 8, IF_NOPICMIP, FT_LINEAR, WT_REPEAT);*/
	// this is for depth rendering
	// red
	for (x = 0; x < DEFAULT_SIZE; x++)
	{
		for (y = 0; y < DEFAULT_SIZE; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 0;
			data[y][x][2] = 0;
			data[y][x][3] = 255;
		}
	}
	tr.redImage = R_CreateImage("_red", (byte *) data, 8, 8, IF_NOPICMIP, FT_LINEAR, WT_REPEAT);

	/* green
	for (x = 0; x < DEFAULT_SIZE; x++)
	{
		for (y = 0; y < DEFAULT_SIZE; y++)
		{
			data[y][x][0] = 0;
			data[y][x][1] = 255;
			data[y][x][2] = 0;
			data[y][x][3] = 255;
		}
	}
	tr.greenImage = R_CreateImage("_green", (byte *) data, 8, 8, IF_NOPICMIP, FT_LINEAR, WT_REPEAT);

	// blue
	for (x = 0; x < DEFAULT_SIZE; x++)
	{
		for (y = 0; y < DEFAULT_SIZE; y++)
		{
			data[y][x][0] = 0;
			data[y][x][1] = 0;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}
	tr.blueImage = R_CreateImage("_blue", (byte *) data, 8, 8, IF_NOPICMIP, FT_LINEAR, WT_REPEAT);
	*/
	// generate a default normalmap with a zero heightmap
	for (x = 0; x < DEFAULT_SIZE; x++)
	{
		for (y = 0; y < DEFAULT_SIZE; y++)
		{
			data[y][x][0] = 128;
			data[y][x][1] = 128;
			data[y][x][2] = 255;
			data[y][x][3] = 0;
		}
	}
	tr.flatImage = R_CreateImage("_flat", (byte *) data, 8, 8, IF_NOPICMIP | IF_NORMALMAP, FT_LINEAR, WT_REPEAT);

	for (x = 0; x < 32; x++)
	{
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage("_scratch", (byte *) data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NONE, FT_LINEAR, WT_EDGE_CLAMP);
	}

	out = &data[0][0][0];
	for (y = 0; y < DEFAULT_SIZE; y++)
	{
		for (x = 0; x < DEFAULT_SIZE; x++, out += 4)
		{
			s = (((float)x + 0.5f) * (2.0f / DEFAULT_SIZE) - 1.0f);

			s = Q_fabs(s) - (1.0f / DEFAULT_SIZE);

			value = 1.0f - (s * 2.0f) + (s * s);

			intensity = ClampByte(Q_ftol(value * 255.0f));

			out[0] = intensity;
			out[1] = intensity;
			out[2] = intensity;
			out[3] = intensity;
		}
	}

	tr.quadraticImage = R_CreateImage("_quadratic", (byte *) data, DEFAULT_SIZE, DEFAULT_SIZE, IF_NOPICMIP | IF_NOCOMPRESSION, FT_LINEAR, WT_EDGE_CLAMP);

	//R_CreateRandomNormalsImage();
	R_CreateFogImage();
	R_CreateNoFalloffImage();
	R_CreateAttenuationXYImage();
	R_CreateContrastRenderFBOImage();
	R_CreateBloomRenderFBOImage();
	R_CreateCurrentRenderImage();
	R_CreateDepthRenderImage();
	R_CreatePortalRenderImage();
	R_CreateOcclusionRenderFBOImage();
	R_CreateDepthToColorFBOImages();
	R_CreateDownScaleFBOImages();
	R_CreateDeferredRenderFBOImages();
	R_CreateShadowMapFBOImage();
	R_CreateShadowCubeFBOImage();
	R_CreateColorCubeImages();
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

	tr.mapOverBrightBits = r_mapOverBrightBits->integer;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;

	if (!glConfig.deviceSupportsGamma)
	{
		tr.overbrightBits = 0;  // need hardware gamma for overbright
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

	tr.identityLight = 1.0f / (1 << tr.overbrightBits);

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
			inf = 255 * pow(i / 255.0, 1.0f / g) + 0.5;
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

	for (i = 0; i < 256; i++)
	{
		j = i * r_intensity->value;

		if (j > 255)
		{
			j = 255;
		}

		s_intensitytable[i] = j;
	}

	//if (glConfig.deviceSupportsGamma)
	//{
	//   ri.GLimp_SetGamma(s_gammatable, s_gammatable, s_gammatable);
	//}
}

/**
 * @brief R_InitImages
 */
void R_InitImages(void)
{
	const char *charsetImage  = "gfx/2d/consolechars";
	const char *grainImage    = "gfx/2d/camera/grain.png";
	const char *vignetteImage = "gfx/2d/camera/vignette.png";

	Ren_Developer("------- R_InitImages -------\n");

	Com_Memset(r_imageHashTable, 0, sizeof(r_imageHashTable));
	Com_InitGrowList(&tr.images, 4096);
	Com_InitGrowList(&tr.lightmaps, 128);
	//Com_InitGrowList(&tr.deluxemaps, 128);

	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();

	tr.charsetImage = R_FindImageFile(charsetImage, IF_NOCOMPRESSION | IF_NOPICMIP, FT_DEFAULT, WT_EDGE_CLAMP, NULL);
	if (!tr.charsetImage)
	{
		Ren_Warning("R_InitImages: could not load '%s'\n", charsetImage);
	}

	tr.grainImage = R_FindImageFile(grainImage, IF_NOCOMPRESSION | IF_NOPICMIP, FT_DEFAULT, WT_REPEAT, NULL);
	if (!tr.grainImage)
	{
		Ren_Warning("R_InitImages: could not load '%s'\n", grainImage);
	}

	tr.vignetteImage = R_FindImageFile(vignetteImage, IF_NOCOMPRESSION | IF_NOPICMIP, FT_DEFAULT, WT_EDGE_CLAMP, NULL);
	if (!tr.vignetteImage)
	{
		Ren_Warning("R_InitImages: could not load '%s'\n", vignetteImage);
	}
}

/**
 * @brief R_ShutdownImages
 */
void R_ShutdownImages(void)
{
	int     i;
	image_t *image;

	Ren_Developer("------- R_ShutdownImages -------\n");

	for (i = 0; i < tr.images.currentElements; i++)
	{
		image = (image_t *)Com_GrowListElement(&tr.images, i);

		glDeleteTextures(1, &image->texnum);
	}

	Com_Memset(glState.currenttextures, 0, sizeof(glState.currenttextures));

	Com_DestroyGrowList(&tr.images);
	Com_DestroyGrowList(&tr.lightmaps);
	//Com_DestroyGrowList(&tr.deluxemaps);
	Com_DestroyGrowList(&tr.cubeProbes);
#if 0 // cubeProbe hash values
	// the cubeProbe hash values can also be freed (the images have just been removed)
	FreeVertexHashTable(tr.cubeHashTable);
#endif
}

/**
 * @brief RE_GetTextureId
 * @param name
 * @return
 */
int RE_GetTextureId(const char *name)
{
	int     i;
	image_t *image;

	Ren_Developer(S_COLOR_YELLOW "RE_GetTextureId [%s].\n", name);

	for (i = 0; i < tr.images.currentElements; i++)
	{
		image = (image_t *)Com_GrowListElement(&tr.images, i);

		if (!strcmp(name, image->name))
		{
			return i;
		}
	}

	return -1;
}
