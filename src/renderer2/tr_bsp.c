/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
 * Copyright (C) 2009 Peter McNeill <n27@bigpond.net.au>
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
 * @file renderer2/tr_bsp.c
 * @brief Loads and prepares a map file for scene rendering.
 *
 * A single entry point: void RE_LoadWorldMap( const char *name );
 */

#include "tr_local.h"

static world_t    s_worldData;
static int        s_lightCount;
static growList_t s_interactions;
static byte       *fileBase;

static int c_vboWorldSurfaces;
static int c_vboLightSurfaces;
static int c_vboShadowSurfaces;

//===============================================================================

/**
 * @brief HSVtoRGB
 * @param[in] h
 * @param[in] s
 * @param[in] v
 * @param[out] rgb
 */
static void HSVtoRGB(float h, float s, float v, float rgb[3])
{
	int   i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor(h);
	f = h - i;

	p = v * (1 - s);
	q = v * (1 - s * f);
	t = v * (1 - s * (1 - f));

	switch (i)
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

/**
 * @brief R_ColorShiftLightingBytes
 * @param[in] in
 * @param[out] out
 */
static void R_ColorShiftLightingBytes(byte in[4], byte out[4])
{
	// shift the color data based on overbright range
	int shift = r_mapOverBrightBits->integer - tr.overbrightBits;
	// shift the data based on overbright range
	int r = in[0] << shift;
	int g = in[1] << shift;
	int b = in[2] << shift;

	// normalize by color instead of saturating to white
	if ((r | g | b) > 255)
	{
		int max = r > g ? r : g;

		max = max > b ? max : b;
		r   = r * 255 / max;
		g   = g * 255 / max;
		b   = b * 255 / max;
	}

	out[0] = (byte)r;
	out[1] = (byte)g;
	out[2] = (byte)b;
	out[3] = in[3];
}

/**
 * @brief R_ColorShiftLightingFloats
 * @param[in] in
 * @param[out] out
 */
static void R_ColorShiftLightingFloats(const vec4_t in, vec4_t out)
{
	int shift, r, g, b;

	// shift the color data based on overbright range
	shift = tr.mapOverBrightBits - tr.overbrightBits;

	// shift the data based on overbright range
	r = ((byte)(in[0] * 255)) << shift;
	g = ((byte)(in[1] * 255)) << shift;
	b = ((byte)(in[2] * 255)) << shift;

	// normalize by color instead of saturating to white
	if ((r | g | b) > 255)
	{
		int max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r   = r * 255 / max;
		g   = g * 255 / max;
		b   = b * 255 / max;
	}

	out[0] = r * (1.0f / 255.0f);
	out[1] = g * (1.0f / 255.0f);
	out[2] = b * (1.0f / 255.0f);
	out[3] = in[3];
}

/**
 * @brief R_ProcessLightmap
 * @param[in] pic
 * @param[in] in_padding
 * @param[in] width
 * @param[in] height
 * @param[out] pic_out
 * @return maxIntensity
 */
float R_ProcessLightmap(byte *pic, int in_padding, int width, int height, byte *pic_out)
{
	int   j;
	float maxIntensity = 0;

	if (r_showLightMaps->integer > 1)     // color code by intensity as development tool (FIXME: check range)
	{
		//double sumIntensity = 0;
		float r, g, b, intensity;
		float out[3] = { 0, 0, 0 };

		for (j = 0; j < width * height; j++)
		{
			r = pic[j * in_padding + 0];
			g = pic[j * in_padding + 1];
			b = pic[j * in_padding + 2];

			intensity = 0.33f * r + 0.685f * g + 0.063f * b;

			if (intensity > 255)
			{
				intensity = 1.0f;
			}
			else
			{
				intensity /= 255.0f;
			}

			if (intensity > maxIntensity)
			{
				maxIntensity = intensity;
			}

			HSVtoRGB(intensity, 1.00, 0.50, out);

			if (r_showLightMaps->integer == 3)
			{
				// Arnout: artists wanted the colours to be inversed
				pic_out[j * 4 + 0] = (byte)(out[2] * 255);
				pic_out[j * 4 + 1] = (byte)(out[1] * 255);
				pic_out[j * 4 + 2] = (byte)(out[0] * 255);
			}
			else
			{
				pic_out[j * 4 + 0] = (byte)(out[0] * 255);
				pic_out[j * 4 + 1] = (byte)(out[1] * 255);
				pic_out[j * 4 + 2] = (byte)(out[2] * 255);
			}
			pic_out[j * 4 + 3] = 255;

			//sumIntensity += intensity;
		}
	}
	else
	{
		for (j = 0; j < width * height; j++)
		{
			R_ColorShiftLightingBytes(&pic[j * in_padding], &pic_out[j * 4]);
			pic_out[j * 4 + 3] = 255;
		}
	}

	return maxIntensity;
}

/**
 * @brief LightmapNameCompare
 * @param[in] a
 * @param[in] b
 * @return
 */
static int QDECL LightmapNameCompare(const void *a, const void *b)
{
	int  c1, c2;
	char *s1 = *(char **)a;
	char *s2 = *(char **)b;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (c1 >= 'a' && c1 <= 'z')
		{
			c1 -= ('a' - 'A');
		}
		if (c2 >= 'a' && c2 <= 'z')
		{
			c2 -= ('a' - 'A');
		}

		if (c1 == '\\' || c1 == ':')
		{
			c1 = '/';
		}
		if (c2 == '\\' || c2 == ':')
		{
			c2 = '/';
		}

		if (c1 < c2)
		{
			// strings not equal
			return -1;
		}
		if (c1 > c2)
		{
			return 1;
		}
	}
	while (c1);

	// strings are equal
	return 0;
}

/*
 * @brief Standard conversion from rgbe to float pixels
 * @param[out] red
 * @param[out] green
 * @param[out] blue
 * @param[in] rgbe
 *
 * @note Ward uses ldexp(col+0.5,exp-(128+8)). However we wanted pixels
 * in the range [0,1] to map back into the range [0,1].
 *
 * @note Unused
static ID_INLINE void rgbe2float(float *red, float *green, float *blue, unsigned char rgbe[4])
{
    float e;
    float f;

    if (rgbe[3])    // nonzero pixel
    {
        f = ldexp(1.0, rgbe[3] - (int)(128 + 8));   // FIXME: never read
        //f = ldexp(1.0, rgbe[3] - 128) / 10.0;
        e = (rgbe[3] - 128) / 4.0f;

        // RB: exp2 not defined by MSVC
        //f = exp2(e);
        f = pow(2, e);

        //decoded = rgbe.rgb * exp2(fExp);
        *red   = (rgbe[0] / 255.0f) * f;
        *green = (rgbe[1] / 255.0f) * f;
        *blue  = (rgbe[2] / 255.0f) * f;
    }
    else
    {
        *red = *green = *blue = 0.0;
    }
}
*/

/**
 * @brief LoadRGBEToFloats
 * @param[in] name
 * @param[out] pic
 * @param[out] width
 * @param[out] height
 * @param[in] doGamma
 * @param[in] toneMap
 * @param[in] compensate
 */
void LoadRGBEToFloats(const char *name, float **pic, int *width, int *height, qboolean doGamma, qboolean toneMap,
                      qboolean compensate)
{
	int      i, j;
	byte     *buf_p;
	byte     *buffer;
	float    *floatbuf;
	char     *token;
	int      w, h, c;
	qboolean formatFound;
	//unsigned char   rgbe[4];
	//float           red;
	//float           green;
	//float           blue;
	//float           max;
	//float           inv, dif;
	float exposure = 1.6f;
	//float           exposureGain = 1.0;
	const vec3_t LUMINANCE_VECTOR = { 0.2125f, 0.7154f, 0.0721f };
	float        luminance;
	float        avgLuminance;
	float        maxLuminance;
	float        scaledLuminance;
	float        finalLuminance;
	double       sum;
	float        gamma;

	union
	{
		byte b[4];
		float f;
	} sample;
	vec4_t sampleVector;

	*pic = NULL;

	// load the file
	ri.FS_ReadFile(name, (void **)&buffer);
	if (!buffer)
	{
		Ren_Drop("LoadRGBE: '%s' not found\n", name);
	}

	buf_p = buffer;

	formatFound = qfalse;
	w           = h = 0;
	while (qtrue)
	{
		token = COM_ParseExt2((char **)&buf_p, qtrue);
		if (!token[0])
		{
			break;
		}

		if (!Q_stricmp(token, "FORMAT"))
		{
			//Ren_Print("LoadRGBE: FORMAT found\n");

			token = COM_ParseExt2((char **)&buf_p, qfalse);
			if (!Q_stricmp(token, "="))
			{
				token = COM_ParseExt2((char **)&buf_p, qfalse);
				if (!Q_stricmp(token, "32"))
				{
					token = COM_ParseExt2((char **)&buf_p, qfalse);
					if (!Q_stricmp(token, "-"))
					{
						token = COM_ParseExt2((char **)&buf_p, qfalse);
						if (!Q_stricmp(token, "bit_rle_rgbe"))
						{
							formatFound = qtrue;
						}
						else
						{
							Ren_Print("LoadRGBE: Expected 'bit_rle_rgbe' found instead '%s'\n", token);
						}
					}
					else
					{
						Ren_Print("LoadRGBE: Expected '-' found instead '%s'\n", token);
					}
				}
				else
				{
					Ren_Print("LoadRGBE: Expected '32' found instead '%s'\n", token);
				}
			}
			else
			{
				Ren_Print("LoadRGBE: Expected '=' found instead '%s'\n", token);
			}
		}

		if (!Q_stricmp(token, "-"))
		{
			token = COM_ParseExt2((char **)&buf_p, qfalse);
			if (!Q_stricmp(token, "Y"))
			{
				token = COM_ParseExt2((char **)&buf_p, qfalse);
				w     = Q_atoi(token);

				token = COM_ParseExt2((char **)&buf_p, qfalse);
				if (!Q_stricmp(token, "+"))
				{
					token = COM_ParseExt2((char **)&buf_p, qfalse);
					if (!Q_stricmp(token, "X"))
					{
						token = COM_ParseExt2((char **)&buf_p, qfalse);
						h     = Q_atoi(token);
						break;
					}
					else
					{
						Ren_Print("LoadRGBE: Expected 'X' found instead '%s'\n", token);
					}
				}
				else
				{
					Ren_Print("LoadRGBE: Expected '+' found instead '%s'\n", token);
				}
			}
			else
			{
				Ren_Print("LoadRGBE: Expected 'Y' found instead '%s'\n", token);
			}
		}
	}

	// go to the first byte
	while ((c = *buf_p++) != 0)
	{
		if (c == '\n')
		{
			//buf_p++;
			break;
		}
	}

	if (width)
	{
		*width = w;
	}
	if (height)
	{
		*height = h;
	}

	if (!formatFound)
	{
		ri.FS_FreeFile(buffer);
		Ren_Drop("LoadRGBE: %s has no format\n", name);
	}

	if (!w || !h)
	{
		ri.FS_FreeFile(buffer);
		Ren_Drop("LoadRGBE: %s has an invalid image size\n", name);
	}

	*pic     = (float *)ri.Hunk_AllocateTempMemory(w * h * 3 * sizeof(float));
	floatbuf = *pic;
	for (i = 0; i < (w * h); i++)
	{
#if 0
		rgbe[0] = *buf_p++;
		rgbe[1] = *buf_p++;
		rgbe[2] = *buf_p++;
		rgbe[3] = *buf_p++;

		rgbe2float(&red, &green, &blue, rgbe);

		*floatbuf++ = red;
		*floatbuf++ = green;
		*floatbuf++ = blue;
#else
		for (j = 0; j < 3; j++)
		{
			sample.b[0] = *buf_p++;
			sample.b[1] = *buf_p++;
			sample.b[2] = *buf_p++;
			sample.b[3] = *buf_p++;

			*floatbuf++ = sample.f / 255.0f;    // FIXME XMap2's output is 255 times too high
		}
#endif
	}

	// LOADING DONE
	if (doGamma)
	{
		floatbuf = *pic;
		gamma    = 1.0f / r_hdrLightmapGamma->value;
		for (i = 0; i < (w * h); i++)
		{
			for (j = 0; j < 3; j++)
			{
				//*floatbuf = pow(*floatbuf / 255.0f, gamma) * 255.0f;
				*floatbuf = pow(*floatbuf, gamma);
				floatbuf++;
			}
		}
	}

	if (toneMap)
	{
		// calculate the average and maximum luminance
		sum          = 0.0;
		maxLuminance = 0.0f;
		floatbuf     = *pic;
		for (i = 0; i < (w * h); i++)
		{
			for (j = 0; j < 3; j++)
			{
				sampleVector[j] = *floatbuf++;
			}

			luminance = DotProduct(sampleVector, LUMINANCE_VECTOR) + 0.0001f;
			if (luminance > maxLuminance)
			{
				maxLuminance = luminance;
			}

			sum += log(luminance);
		}
		sum         /= (w * h);
		avgLuminance = exp(sum);

		// post process buffer with tone mapping
		floatbuf = *pic;
		for (i = 0; i < (w * h); i++)
		{
			for (j = 0; j < 3; j++)
			{
				sampleVector[j] = *floatbuf++;
			}

			if (r_hdrLightmapExposure->value <= 0)
			{
				exposure = (r_hdrKey->value / avgLuminance);
			}
			else
			{
				exposure = r_hdrLightmapExposure->value;
			}

			scaledLuminance = exposure * DotProduct(sampleVector, LUMINANCE_VECTOR);
#if 0
			finalLuminance = scaledLuminance / (scaledLuminance + 1.0);
#elif 0
			finalLuminance = (scaledLuminance * (scaledLuminance / maxLuminance + 1.0)) / (scaledLuminance + 1.0);
#elif 0
			finalLuminance =
				(scaledLuminance * ((scaledLuminance / (maxLuminance * maxLuminance)) + 1.0)) / (scaledLuminance + 1.0);
#else
			// exponential tone mapping
			finalLuminance = 1.0 - exp(-scaledLuminance);
#endif

			//VectorScale(sampleVector, scaledLuminance * (scaledLuminance / maxLuminance + 1.0) / (scaledLuminance + 1.0), sampleVector);
			//VectorScale(sampleVector, scaledLuminance / (scaledLuminance + 1.0), sampleVector);

			VectorScale(sampleVector, finalLuminance, sampleVector);

			floatbuf -= 3;
			for (j = 0; j < 3; j++)
			{
				*floatbuf++ = sampleVector[j];
			}
		}
	}

	if (compensate)
	{
		floatbuf = *pic;
		for (i = 0; i < (w * h); i++)
		{
			for (j = 0; j < 3; j++)
			{
				*floatbuf = *floatbuf / r_hdrLightmapCompensate->value;
				floatbuf++;
			}
		}
	}

	ri.FS_FreeFile(buffer);
}
/**
 * @brief LoadRGBEToHalfs
 * @param name      - unused
 * @param halfImage - unused
 * @param width     - unused
 * @param height    - unused
 *
 * @note not implemented
 */
void LoadRGBEToHalfs(const char *name, unsigned short **halfImage, int *width, int *height)
{
	ri.Error(ERR_VID_FATAL, "LoadRGBEToHalfs in tr_bsp.cpp is not done yet");
}

/**
 * @brief LoadRGBEToBytes
 * @param[in] name
 * @param[out] ldrImage
 * @param[out] width
 * @param[out] height
 */
static void LoadRGBEToBytes(const char *name, byte **ldrImage, int *width, int *height)
{
	int    i, j;
	int    w, h;
	float  *hdrImage;
	float  *floatbuf;
	byte   *pixbuf;
	vec3_t sample;
	float  max;

#if 0
	w = h = 0;
	LoadRGBEToFloats(name, &hdrImage, &w, &h, qtrue, qtrue, qtrue);

	*width  = w;
	*height = h;

	*ldrImage = ri.Malloc(w * h * 4);
	pixbuf    = *ldrImage;

	floatbuf = hdrImage;
	for (i = 0; i < (w * h); i++)
	{
		for (j = 0; j < 3; j++)
		{
			sample[j] = *floatbuf++;
		}

		NormalizeColor(sample, sample);

		*pixbuf++ = (byte) (sample[0] * 255);
		*pixbuf++ = (byte) (sample[1] * 255);
		*pixbuf++ = (byte) (sample[2] * 255);
		*pixbuf++ = (byte) 255;
	}
#else
	w = h = 0;
	LoadRGBEToFloats(name, &hdrImage, &w, &h, qfalse, qfalse, qfalse);

	*width  = w;
	*height = h;

	*ldrImage = (byte *)ri.Z_Malloc(w * h * 4);
	pixbuf    = *ldrImage;

	floatbuf = hdrImage;
	for (i = 0; i < (w * h); i++)
	{
		for (j = 0; j < 3; j++)
		{
			sample[j] = *floatbuf++ *255.0f;
		}

		// clamp with color normalization
		max = sample[0];
		if (sample[1] > max)
		{
			max = sample[1];
		}
		if (sample[2] > max)
		{
			max = sample[2];
		}
		if (max > 255.0f)
		{
			VectorScale(sample, (255.0f / max), sample);
		}

		*pixbuf++ = (byte) sample[0];
		*pixbuf++ = (byte) sample[1];
		*pixbuf++ = (byte) sample[2];
		*pixbuf++ = (byte) 255;
	}
#endif

	ri.Hunk_FreeTempMemory(hdrImage);
}

#define LIGHTMAP_SIZE   128
/**
 * @brief R_LoadLightmapsInternal
 * @param[in] l
 * @param bspName - unused
 */
static void R_LoadLightmapsInternal(lump_t *l, const char *bspName)
{
	unsigned int len = l->filelen;
	image_t      *image;
	int          i, j;
	static byte  data[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4], *buf, *buf_p;

	buf = fileBase + l->fileofs;

	// we are about to upload textures
	R_IssuePendingRenderCommands();

	// create all the lightmaps
	tr.numLightmaps = len / (LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3);

	Ren_Developer("...loading %i lightmaps\n", tr.numLightmaps);

	for (i = 0; i < tr.numLightmaps; i++)
	{
		// expand the 24 bit on-disk to 32 bit
		buf_p = buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

		/*if (tr.worldDeluxeMapping)
		{
			if (i % 2 == 0)
			{
				for (j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++)
				{
					R_ColorShiftLightingBytes(&buf_p[j * 3], &data[j * 4]);
					data[j * 4 + 3] = 255;
				}
				image = R_CreateImage(va("_lightmap%d", i), data, LIGHTMAP_SIZE, LIGHTMAP_SIZE, IF_LIGHTMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_EDGE_CLAMP);
				Com_AddToGrowList(&tr.lightmaps, image);
			}
			else
			{
				for (j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++)
				{
					data[j * 4 + 0] = buf_p[j * 3 + 0];
					data[j * 4 + 1] = buf_p[j * 3 + 1];
					data[j * 4 + 2] = buf_p[j * 3 + 2];
					data[j * 4 + 3] = 255;
				}
				image = R_CreateImage(va("_lightmap%d", i), data, LIGHTMAP_SIZE, LIGHTMAP_SIZE, IF_NORMALMAP, FT_DEFAULT, WT_EDGE_CLAMP);
				Com_AddToGrowList(&tr.deluxemaps, image);
			}
		}
		else
		{*/
			for (j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++)
			{
				R_ColorShiftLightingBytes(&buf_p[j * 3], &data[j * 4]);
				data[j * 4 + 3] = 255;
			}
			image = R_CreateImage(va("_lightmap%d", i), data, LIGHTMAP_SIZE, LIGHTMAP_SIZE, IF_LIGHTMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_EDGE_CLAMP);
			Com_AddToGrowList(&tr.lightmaps, image);
		//}
	}
}

/**
 * @brief R_LoadLightmapsExternal
 * @param l - unused
 * @param[in] bspName
 */
static void R_LoadLightmapsExternal(lump_t *l, const char *bspName)
{
	image_t *image;
	int     i;
	int     numLightmaps;
	char    mapName[MAX_QPATH];
	char    **lightmapFiles;

	Q_strncpyz(mapName, bspName, sizeof(mapName));
	COM_StripExtension(mapName, mapName, sizeof(mapName));

	if (tr.worldHDR_RGBE)
	{
		// we are about to upload textures
		R_IssuePendingRenderCommands();

		// load HDR lightmaps
		lightmapFiles = ri.FS_ListFiles(mapName, ".hdr", &numLightmaps);

		if (!lightmapFiles || !numLightmaps)
		{
			Ren_Warning("WARNING: no lightmap files found for map %s\n", mapName);
			return;
		}

		qsort(lightmapFiles, numLightmaps, sizeof(char *), LightmapNameCompare);

		Ren_Developer("...loading %i HDR lightmaps\n", numLightmaps);

		if (r_hdrRendering->integer && r_hdrLightmap->integer && glConfig2.framebufferObjectAvailable &&
			glConfig2.framebufferBlitAvailable && glConfig2.textureFloatAvailable && glConfig2.textureHalfFloatAvailable)
		{
			int            width, height;
			unsigned short *hdrImage = NULL;

			for (i = 0; i < numLightmaps; i++)
			{
				Ren_Developer("...loading external lightmap as RGB 16 bit half HDR '%s/%s'\n", mapName, lightmapFiles[i]);

				width = height = 0;
				//LoadRGBEToFloats(va("%s/%s", mapName, lightmapFiles[i]), &hdrImage, &width, &height, qtrue, qfalse, qtrue);
				LoadRGBEToHalfs(va("%s/%s", mapName, lightmapFiles[i]), &hdrImage, &width, &height);

				//Ren_Print("...converted '%s/%s' to HALF format\n", mapName, lightmapFiles[i]);

				image = R_AllocImage(va("%s/%s", mapName, lightmapFiles[i]), qtrue);
				if (!image)
				{
					Com_Dealloc(hdrImage);
					break;
				}

				//Q_strncpyz(image->name, );
				image->type = GL_TEXTURE_2D;

				image->width = width;
				image->height = height;

				image->bits = IF_NOPICMIP | IF_RGBA16F;
				image->filterType = FT_NEAREST;
				image->wrapType = WT_EDGE_CLAMP;

				GL_Bind(image);

				image->internalFormat = GL_RGBA16F_ARB;
				image->uploadWidth = width;
				image->uploadHeight = height;

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, width, height, 0, GL_RGB, GL_HALF_FLOAT_ARB, hdrImage);

				if (glConfig2.generateMipmapAvailable)
				{
					//glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);    // make sure its nice
					glTexParameteri(image->type, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
					glTexParameteri(image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);   // default to trilinear
				}


				glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTexParameterf(image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glBindTexture(image->type, 0);

				GL_CheckErrors();

				Com_Dealloc(hdrImage);

				Com_AddToGrowList(&tr.lightmaps, image);
			}
		}
		else
		{
			int  width, height;
			byte *ldrImage;

			for (i = 0; i < numLightmaps; i++)
			{
				Ren_Developer("...loading external lightmap as RGB8 LDR '%s/%s'\n", mapName, lightmapFiles[i]);

				width = height = 0;
				LoadRGBEToBytes(va("%s/%s", mapName, lightmapFiles[i]), &ldrImage, &width, &height);

				image = R_CreateImage(va("%s/%s", mapName, lightmapFiles[i]), (byte *)ldrImage, width, height,
					IF_NOPICMIP | IF_LIGHTMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_EDGE_CLAMP);

				Com_AddToGrowList(&tr.lightmaps, image);

				ri.Free(ldrImage);
			}
		}
	}

		/*if (tr.worldDeluxeMapping)
		{
			// load deluxemaps
			lightmapFiles = ri.FS_ListFiles(mapName, ".png", &numLightmaps);

			if (!lightmapFiles || !numLightmaps)
			{
				lightmapFiles = ri.FS_ListFiles(mapName, ".tga", &numLightmaps);

				if (!lightmapFiles || !numLightmaps)
				{
					Ren_Warning("WARNING: no lightmap files found\n");
					return;
				}
			}

			qsort(lightmapFiles, numLightmaps, sizeof(char *), LightmapNameCompare);

			Ren_Developer("...loading %i deluxemaps\n", numLightmaps);

			for (i = 0; i < numLightmaps; i++)
			{
				Ren_Developer("...loading external lightmap '%s/%s'\n", mapName, lightmapFiles[i]);

				image = R_FindImageFile(va("%s/%s", mapName, lightmapFiles[i]), IF_NORMALMAP | IF_NOCOMPRESSION | IF_NOPICMIP, FT_DEFAULT, WT_EDGE_CLAMP, NULL);
				Com_AddToGrowList(&tr.deluxemaps, image);
			}
		}
	}*/

	//{
		/*lightmapFiles = ri.FS_ListFiles(mapName, ".png", &numLightmaps);

		if (!lightmapFiles || !numLightmaps)
		{*/
			lightmapFiles = ri.FS_ListFiles(mapName, ".tga", &numLightmaps);

			if (!lightmapFiles || !numLightmaps)
			{
				Ren_Warning("WARNING: no lightmap files found for %s\n", mapName);
				return;
			}
		//}

		qsort(lightmapFiles, numLightmaps, sizeof(char *), LightmapNameCompare);

		Ren_Developer("...loading %i lightmaps\n", numLightmaps);

		// we are about to upload textures
		R_IssuePendingRenderCommands();

		for (i = 0; i < numLightmaps; i++)
		{
			Ren_Developer("...loading external lightmap '%s/%s'\n", mapName, lightmapFiles[i]);

			/*if (tr.worldDeluxeMapping)
			{
				if (i % 2 == 0)
				{
					Ren_Developer("Loading lightmap\n");
					image = R_FindImageFile(va("%s/%s", mapName, lightmapFiles[i]), IF_LIGHTMAP | IF_NOCOMPRESSION | IF_NOPICMIP, FT_LINEAR, WT_EDGE_CLAMP, NULL);
					Com_AddToGrowList(&tr.lightmaps, image);
				}
				else
				{
					Ren_Developer("Loading deluxemap\n");
					image = R_FindImageFile(va("%s/%s", mapName, lightmapFiles[i]), IF_NORMALMAP | IF_NOCOMPRESSION | IF_NOPICMIP, FT_LINEAR, WT_EDGE_CLAMP, NULL);
					Com_AddToGrowList(&tr.deluxemaps, image);
				}
			}
			else
			{*/
				Ren_Developer("Loading lightmap\n");
				image = R_FindImageFile(va("%s/%s", mapName, lightmapFiles[i]), IF_LIGHTMAP | IF_NOCOMPRESSION | IF_NOPICMIP, FT_LINEAR, WT_EDGE_CLAMP, NULL);
				Com_AddToGrowList(&tr.lightmaps, image);
		}


}

/**
 * @brief R_LoadLightmaps
 * @param[in] l
 * @param[in] bspName
 */
static void R_LoadLightmaps(lump_t *l, const char *bspName)
{
	tr.fatLightmapSize = 0;

	if (!l->filelen)
	{
		R_LoadLightmapsExternal(l, bspName);
	}
	else
	{
		R_LoadLightmapsInternal(l, bspName);
	}
}

/**
 * @brief This is called by the clipmodel subsystem so we can share the 1.8 megs of
 * space in big maps...
 * @param vis
 */
void RE_SetWorldVisData(const byte *vis)
{
	tr.externalVisData = vis;
}

/**
 * @brief R_LoadVisibility
 * @param[in] l
 */
static void R_LoadVisibility(lump_t *l)
{
	unsigned int len;
	byte         *buf;

	Ren_Developer("...loading visibility\n");

	len               = PAD(s_worldData.numClusters, 64);
	s_worldData.novis = (byte *)ri.Hunk_Alloc(len, h_low);
	Com_Memset(s_worldData.novis, 0xff, len);

	len = l->filelen;
	if (!len)
	{
		return;
	}
	buf = fileBase + l->fileofs;

	s_worldData.numClusters  = LittleLong(((int *)buf)[0]);
	s_worldData.clusterBytes = LittleLong(((int *)buf)[1]);

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if (tr.externalVisData)
	{
		s_worldData.vis = tr.externalVisData;
	}
	else
	{
		byte *dest;

		dest = (byte *)ri.Hunk_Alloc(len - 8, h_low);
		Com_Memcpy(dest, buf + 8, len - 8);
		s_worldData.vis = dest;
	}
}

//===============================================================================

/**
 * @brief ShaderForShaderNum
 * @param[in] shaderNum
 * @return
 */
static shader_t *ShaderForShaderNum(int shaderNum)
{
	shader_t  *shader;
	dshader_t *dsh;

	shaderNum = LittleLong(shaderNum) + 0; // silence the warning
	if (shaderNum < 0 || shaderNum >= s_worldData.numShaders)
	{
		Ren_Drop("ShaderForShaderNum: bad num %i", shaderNum);
	}
	dsh = &s_worldData.shaders[shaderNum];

	//Ren_Print("ShaderForShaderNum: '%s'\n", dsh->shader);

	shader = R_FindShader(dsh->shader, SHADER_3D_STATIC, qtrue);

	// if the shader had errors, just use default shader
	if (shader->defaultShader)
	{
		//Ren_Print("failed\n");
		return tr.defaultShader;
	}

	//Ren_Print("success\n");
	return shader;
}

/**
 * @brief Creates a bounding sphere from a bounding box
 * @param mins
 * @param maxs
 * @param origin
 * @param radius
 */
static void SphereFromBounds(vec3_t mins, vec3_t maxs, vec3_t origin, float *radius)
{
	vec3_t temp;

	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5f, origin);
	VectorSubtract(maxs, origin, temp);
	*radius = VectorLength(temp);
}

/**
 * @brief Handles final surface classification
 * @param ds
 * @param gen
 * @param pt
 */
static void FinishGenericSurface(dsurface_t *ds, srfGeneric_t *gen, vec3_t pt)
{
	// set bounding sphere
	SphereFromBounds(gen->bounds[0], gen->bounds[1], gen->origin, &gen->radius);

	// take the plane normal from the lightmap vector and classify it
	gen->plane.normal[0] = LittleFloat(ds->lightmapVecs[2][0]);
	gen->plane.normal[1] = LittleFloat(ds->lightmapVecs[2][1]);
	gen->plane.normal[2] = LittleFloat(ds->lightmapVecs[2][2]);
	gen->plane.dist      = DotProduct(pt, gen->plane.normal);
	SetPlaneSignbits(&gen->plane);
	gen->plane.type = PlaneTypeForNormal(gen->plane.normal);
}

/**
 * @brief ParseFace
 * @param[in] ds
 * @param[out] verts
 * @param[out] surf
 * @param[out] indexes
 */
static void ParseFace(dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf, int *indexes)
{
	int              i, j;
	srfSurfaceFace_t *cv;
	srfTriangle_t    *tri;
	int              numVerts, numTriangles;

	// get lightmap
	if (r_vertexLighting->integer || !r_precomputedLighting->integer)
	{
		surf->lightmapNum = LIGHTMAP_NONE;
	}
	else
	{
		surf->lightmapNum = LittleLong(ds->lightmapNum);
	}

	/*if (tr.worldDeluxeMapping && surf->lightmapNum >= 2)
	{
		surf->lightmapNum /= 2;
	}*/

	/*
	if(surf->lightmapNum >= tr.lightmaps.currentElements)
	{
	    Ren_Drop( "Bad lightmap number %i in face surface", surf->lightmapNum);
	}
	*/

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum(ds->shaderNum);
	if (r_singleShader->integer && !surf->shader->isSky)
	{
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong(ds->numVerts);
	/*
	   if(numVerts > MAX_FACE_POINTS)
	   {
	   Ren_Warning( "WARNING: MAX_FACE_POINTS exceeded: %i\n", numVerts);
	   numVerts = MAX_FACE_POINTS;
	   surf->shader = tr.defaultShader;
	   }
	 */
	numTriangles = LittleLong(ds->numIndexes) / 3;

	cv              = (srfSurfaceFace_t *)ri.Hunk_Alloc(sizeof(*cv), h_low);
	cv->surfaceType = SF_FACE;

	cv->numTriangles = numTriangles;
	cv->triangles    = (srfTriangle_t *)ri.Hunk_Alloc(numTriangles * sizeof(cv->triangles[0]), h_low);

	cv->numVerts = numVerts;
	cv->verts    = (srfVert_t *)ri.Hunk_Alloc(numVerts * sizeof(cv->verts[0]), h_low);

	surf->data = (surfaceType_t *) cv;

	// copy vertexes
	ClearBounds(cv->bounds[0], cv->bounds[1]);
	verts += LittleLong(ds->firstVert);
	for (i = 0; i < numVerts; i++)
	{
		for (j = 0; j < 3; j++)
		{
			cv->verts[i].xyz[j]    = LittleFloat(verts[i].xyz[j]);
			cv->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}
		AddPointToBounds(cv->verts[i].xyz, cv->bounds[0], cv->bounds[1]);
		for (j = 0; j < 2; j++)
		{
			cv->verts[i].st[j]       = LittleFloat(verts[i].st[j]);
			cv->verts[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}

		for (j = 0; j < 4; j++)
		{
			cv->verts[i].lightColor[j] = verts[i].color[j] * (1.0f / 255.0f);
		}
		R_ColorShiftLightingFloats(cv->verts[i].lightColor, cv->verts[i].lightColor);
	}

	// copy triangles
	indexes += LittleLong(ds->firstIndex);
	for (i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
	{
		for (j = 0; j < 3; j++)
		{
			tri->indexes[j] = LittleLong(indexes[i * 3 + j]);

			if (tri->indexes[j] < 0 || tri->indexes[j] >= numVerts)
			{
				Ren_Drop("Bad index in face surface");
			}
		}
	}

	//R_CalcSurfaceTrianglePlanes(numTriangles, cv->triangles, cv->verts);

	surf->data = (surfaceType_t *) cv;

	// calc tangent spaces
#if 0 // example: oasis wall
	{
		float       *v;
		const float *v0, *v1, *v2;
		const float *t0, *t1, *t2;
		vec3_t      tangent;
		vec3_t      binormal;
		vec3_t      normal;

		for (i = 0; i < numVerts; i++)
		{
			VectorClear(cv->verts[i].tangent);
			VectorClear(cv->verts[i].binormal);
			VectorClear(cv->verts[i].normal);
		}

		for (i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
		{
			v0 = cv->verts[tri->indexes[0]].xyz;
			v1 = cv->verts[tri->indexes[1]].xyz;
			v2 = cv->verts[tri->indexes[2]].xyz;

			t0 = cv->verts[tri->indexes[0]].st;
			t1 = cv->verts[tri->indexes[1]].st;
			t2 = cv->verts[tri->indexes[2]].st;

			R_CalcTangentSpace(tangent, binormal, normal, v0, v1, v2, t0, t1, t2);

			for (j = 0; j < 3; j++)
			{
				v = cv->verts[tri->indexes[j]].tangent;
				VectorAdd(v, tangent, v);
				v = cv->verts[tri->indexes[j]].binormal;
				VectorAdd(v, binormal, v);
				v = cv->verts[tri->indexes[j]].normal;
				VectorAdd(v, normal, v);
			}
		}

		for (i = 0; i < numVerts; i++)
		{
			VectorNormalize(cv->verts[i].tangent);
			VectorNormalize(cv->verts[i].binormal);
			VectorNormalize(cv->verts[i].normal);
		}
	}
#else
	{
		srfVert_t *dv[3];

		for (i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
		{
			dv[0] = &cv->verts[tri->indexes[0]];
			dv[1] = &cv->verts[tri->indexes[1]];
			dv[2] = &cv->verts[tri->indexes[2]];

			R_CalcTangentVectors(dv);
		}
	}
#endif

	// finish surface
	FinishGenericSurface(ds, (srfGeneric_t *) cv, cv->verts[0].xyz);
}

/**
 * @brief ParseMesh
 * @param[in] ds
 * @param[out] verts
 * @param[out] surf
 */
static void ParseMesh(dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf)
{
	srfGridMesh_t        *grid;
	int                  i, j;
	int                  width, height, numPoints;
	static srfVert_t     points[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
	vec3_t               bounds[2];
	vec3_t               tmpVec;
	static surfaceType_t skipData = SF_SKIP;

	// get lightmap
	if (r_vertexLighting->integer || !r_precomputedLighting->integer)
	{
		surf->lightmapNum = LIGHTMAP_NONE;
	}
	else
	{
		surf->lightmapNum = LittleLong(ds->lightmapNum);
	}

	/*if (tr.worldDeluxeMapping && surf->lightmapNum >= 2)
	{
		surf->lightmapNum /= 2;
	}*/

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum(ds->shaderNum);
	if (r_singleShader->integer && !surf->shader->isSky)
	{
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if (s_worldData.shaders[LittleLong(ds->shaderNum)].surfaceFlags & SURF_NODRAW)
	{
		surf->data = &skipData;
		return;
	}

	width  = LittleLong(ds->patchWidth);
	height = LittleLong(ds->patchHeight);

	if (width < 0 || width > MAX_PATCH_SIZE || height < 0 || height > MAX_PATCH_SIZE)
	{
		Ren_Drop("ParseMesh: bad size");
	}

	verts    += LittleLong(ds->firstVert);
	numPoints = width * height;
	for (i = 0; i < numPoints; i++)
	{
		for (j = 0; j < 3; j++)
		{
			points[i].xyz[j]    = LittleFloat(verts[i].xyz[j]);
			points[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}

		for (j = 0; j < 2; j++)
		{
			points[i].st[j]       = LittleFloat(verts[i].st[j]);
			points[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}

		for (j = 0; j < 4; j++)
		{
			points[i].lightColor[j] = verts[i].color[j] * (1.0f / 255.0f);
		}
		R_ColorShiftLightingFloats(points[i].lightColor, points[i].lightColor);
	}

	// pre-tesseleate
	grid       = R_SubdividePatchToGrid(width, height, points);
	surf->data = (surfaceType_t *) grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for (i = 0; i < 3; i++)
	{
		bounds[0][i] = LittleFloat(ds->lightmapVecs[0][i]);
		bounds[1][i] = LittleFloat(ds->lightmapVecs[1][i]);
	}
	VectorAdd(bounds[0], bounds[1], bounds[1]);
	VectorScale(bounds[1], 0.5f, grid->lodOrigin);
	VectorSubtract(bounds[0], grid->lodOrigin, tmpVec);
	grid->lodRadius = VectorLength(tmpVec);

	// finish surface
	FinishGenericSurface(ds, (srfGeneric_t *) grid, grid->verts[0].xyz);
}

/**
 * @brief ParseTriSurf
 * @param[in] ds
 * @param[out] verts
 * @param[out] surf
 * @param[out] indexes
 */
static void ParseTriSurf(dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf, int *indexes)
{
	srfTriangles_t       *cv;
	srfTriangle_t        *tri;
	int                  i, j;
	int                  numVerts, numTriangles;
	static surfaceType_t skipData = SF_SKIP;

	// get lightmap
	if (r_vertexLighting->integer || !r_precomputedLighting->integer)
	{
		surf->lightmapNum = LIGHTMAP_NONE;
	}
	else
	{
		surf->lightmapNum = LittleLong(ds->lightmapNum);
	}

	/*if (tr.worldDeluxeMapping && surf->lightmapNum >= 2)
	{
		surf->lightmapNum /= 2;
	}*/

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader
	surf->shader = ShaderForShaderNum(ds->shaderNum);
	if (r_singleShader->integer && !surf->shader->isSky)
	{
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if (s_worldData.shaders[LittleLong(ds->shaderNum)].surfaceFlags & SURF_NODRAW)
	{
		surf->data = &skipData;
		return;
	}

	numVerts     = LittleLong(ds->numVerts);
	numTriangles = LittleLong(ds->numIndexes) / 3;

	cv              = (srfTriangles_t *)ri.Hunk_Alloc(sizeof(*cv), h_low);
	cv->surfaceType = SF_TRIANGLES;

	cv->numTriangles = numTriangles;
	cv->triangles    = (srfTriangle_t *)ri.Hunk_Alloc(numTriangles * sizeof(cv->triangles[0]), h_low);

	cv->numVerts = numVerts;
	cv->verts    = (srfVert_t *)ri.Hunk_Alloc(numVerts * sizeof(cv->verts[0]), h_low);

	surf->data = (surfaceType_t *) cv;

	// copy vertexes
	verts += LittleLong(ds->firstVert);
	for (i = 0; i < numVerts; i++)
	{
		for (j = 0; j < 3; j++)
		{
			cv->verts[i].xyz[j]    = LittleFloat(verts[i].xyz[j]);
			cv->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}

		for (j = 0; j < 2; j++)
		{
			cv->verts[i].st[j]       = LittleFloat(verts[i].st[j]);
			cv->verts[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}

		for (j = 0; j < 4; j++)
		{
			cv->verts[i].lightColor[j] = verts[i].color[j] * (1.0f / 255.0f);
		}
		R_ColorShiftLightingFloats(cv->verts[i].lightColor, cv->verts[i].lightColor);
	}

	// copy triangles
	indexes += LittleLong(ds->firstIndex);
	for (i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
	{
		for (j = 0; j < 3; j++)
		{
			tri->indexes[j] = LittleLong(indexes[i * 3 + j]);

			if (tri->indexes[j] < 0 || tri->indexes[j] >= numVerts)
			{
				Ren_Drop("Bad index in face surface");
			}
		}
	}

	// calc bounding box
	// HACK: don't loop only through the vertices because they can contain bad data with .lwo models ...
	ClearBounds(cv->bounds[0], cv->bounds[1]);
	for (i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
	{
		AddPointToBounds(cv->verts[tri->indexes[0]].xyz, cv->bounds[0], cv->bounds[1]);
		AddPointToBounds(cv->verts[tri->indexes[1]].xyz, cv->bounds[0], cv->bounds[1]);
		AddPointToBounds(cv->verts[tri->indexes[2]].xyz, cv->bounds[0], cv->bounds[1]);
	}

	//R_CalcSurfaceTrianglePlanes(numTriangles, cv->triangles, cv->verts);

	// calc tangent spaces
#if 0
	{
		float       *v;
		const float *v0, *v1, *v2;
		const float *t0, *t1, *t2;
		vec3_t      tangent;
		vec3_t      binormal;
		vec3_t      normal;

		for (i = 0; i < numVerts; i++)
		{
			VectorClear(cv->verts[i].tangent);
			VectorClear(cv->verts[i].binormal);
			VectorClear(cv->verts[i].normal);
		}

		for (i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
		{
			v0 = cv->verts[tri->indexes[0]].xyz;
			v1 = cv->verts[tri->indexes[1]].xyz;
			v2 = cv->verts[tri->indexes[2]].xyz;

			t0 = cv->verts[tri->indexes[0]].st;
			t1 = cv->verts[tri->indexes[1]].st;
			t2 = cv->verts[tri->indexes[2]].st;

#if 1
			R_CalcTangentSpace(tangent, binormal, normal, v0, v1, v2, t0, t1, t2);
#else
			R_CalcNormalForTriangle(normal, v0, v1, v2);
			R_CalcTangentsForTriangle2(tangent, binormal, v0, v1, v2, t0, t1, t2);
#endif

			for (j = 0; j < 3; j++)
			{
				v = cv->verts[tri->indexes[j]].tangent;
				VectorAdd(v, tangent, v);
				v = cv->verts[tri->indexes[j]].binormal;
				VectorAdd(v, binormal, v);
				v = cv->verts[tri->indexes[j]].normal;
				VectorAdd(v, normal, v);
			}
		}

		for (i = 0; i < numVerts; i++)
		{
			float dot;

			//VectorNormalize(cv->verts[i].tangent);
			VectorNormalize(cv->verts[i].binormal);
			VectorNormalize(cv->verts[i].normal);

			// Gram-Schmidt orthogonalize
			dot = DotProduct(cv->verts[i].normal, cv->verts[i].tangent);
			VectorMA(cv->verts[i].tangent, -dot, cv->verts[i].normal, cv->verts[i].tangent);
			VectorNormalize(cv->verts[i].tangent);

			//dot = DotProduct(cv->verts[i].normal, cv->verts[i].tangent);
			//VectorMA(cv->verts[i].tangent, -dot, cv->verts[i].normal, cv->verts[i].tangent);
			//VectorNormalize(cv->verts[i].tangent);
		}
	}
#else
	{
		srfVert_t *dv[3];

		for (i = 0, tri = cv->triangles; i < numTriangles; i++, tri++)
		{
			dv[0] = &cv->verts[tri->indexes[0]];
			dv[1] = &cv->verts[tri->indexes[1]];
			dv[2] = &cv->verts[tri->indexes[2]];

			R_CalcTangentVectors(dv);
		}
	}
#endif

	// do another extra smoothing for normals to avoid flat shading
	if (r_smoothNormals->integer & FLAGS_SMOOTH_TRISURF)
	{
		for (i = 0; i < numVerts; i++)
		{
			for (j = 0; j < numVerts; j++)
			{
				if (i == j)
				{
					continue;
				}

				if (R_CompareVert(&cv->verts[i], &cv->verts[j], qfalse))
				{
					VectorAdd(cv->verts[i].normal, cv->verts[j].normal, cv->verts[i].normal);
				}
			}

			VectorNormalize(cv->verts[i].normal);
		}
	}

	// finish surface
	FinishGenericSurface(ds, (srfGeneric_t *) cv, cv->verts[0].xyz);
}

/**
 * @brief Parses a foliage drawsurface
 * @param[in] ds
 * @param[out] verts
 * @param[out] surf
 * @param[out] indexes
 */
static void ParseFoliage(dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf, int *indexes)
{
	srfFoliage_t  *foliage;
	srfTriangle_t *tri;
	int           i, j, numVerts, numInstances, numTriangles;
	vec3_t        bounds[2], boundsTranslated[2];
	float         foliageHeightScale;

	foliageHeightScale = Com_Clamp(0.1f, 2.0f, r_drawFoliage->value);

	surf->fogIndex = LittleLong(ds->fogNum) + 1;
	surf->shader   = ShaderForShaderNum(ds->shaderNum);
	if (r_singleShader->integer && !surf->shader->isSky)
	{
		surf->shader = tr.defaultShader;
	}
	surf->lightmapNum = LIGHTMAP_BY_VERTEX;

	// foliage surfaces have their actual vert count in patchHeight
	// and the instance count in patchWidth
	// the instances are just additional drawverts

	// get counts
	numVerts     = LittleLong(ds->patchHeight);
	numInstances = LittleLong(ds->patchWidth);
	numTriangles = LittleLong(ds->numIndexes) / 3;

	// get memory
	foliage = (srfFoliage_t *)ri.Hunk_Alloc(sizeof(*foliage), h_low);
	// set up surface
	foliage->surfaceType = SF_FOLIAGE;
	// allocate space for tris
	foliage->numTriangles = numTriangles;
	foliage->triangles    = (srfTriangle_t *)ri.Hunk_Alloc(numTriangles * sizeof(foliage->triangles[0]), h_low);
	// allocate space for verts
	foliage->numVerts = numVerts;
	foliage->verts    = (srfVert_t *)ri.Hunk_Alloc(numVerts * sizeof(foliage->verts[0]), h_low);
	// allocate space for instances
	foliage->numInstances = numInstances;
	foliage->instances    = (foliageInstance_t *)ri.Hunk_Alloc(numInstances * sizeof(foliage->instances[0]), h_low);

	surf->data = (surfaceType_t *)foliage;

	// copy vertexes
	ClearBounds(bounds[0], bounds[1]);
	verts += LittleLong(ds->firstVert);
	for (i = 0; i < numVerts; i++)
	{
		for (j = 0; j < 3; j++)
		{
			foliage->verts[i].xyz[j]    = LittleFloat(verts[i].xyz[j]);
			foliage->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}
		foliage->verts[i].xyz[2] *= foliageHeightScale;

		// finish
		AddPointToBounds(foliage->verts[i].xyz, bounds[0], bounds[1]);

		// copy texture coordinates
		for (j = 0; j < 2; j++)
		{
			foliage->verts[i].st[j]       = LittleFloat(verts[i].st[j]);
			foliage->verts[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}
	}

	// copy indexes
	indexes += LittleLong(ds->firstIndex);
	for (i = 0, tri = foliage->triangles; i < numTriangles; i++, tri++)
	{
		for (j = 0; j < 3; j++)
		{
			tri->indexes[j] = LittleLong(indexes[i * 3 + j]);
			if (tri->indexes[j] < 0 || tri->indexes[j] >= numVerts)
			{
				Ren_Drop("Bad index in triangle surface");
			}
		}
	}

	// copy instances
	ClearBounds(foliage->bounds[0], foliage->bounds[1]);
	verts += numVerts;
	for (i = 0; i < numInstances; i++)
	{
		// copy origin
		for (j = 0; j < 3; j++)
		{
			foliage->instances[i].origin[j] = LittleFloat(verts[i].xyz[j]);
		}
		VectorAdd(bounds[0], foliage->instances[i].origin, boundsTranslated[0]);
		VectorAdd(bounds[1], foliage->instances[i].origin, boundsTranslated[1]);
		AddPointToBounds(boundsTranslated[0], foliage->bounds[0], foliage->bounds[1]);
		AddPointToBounds(boundsTranslated[1], foliage->bounds[0], foliage->bounds[1]);

		// copy color
		R_ColorShiftLightingBytes(verts[i].color, foliage->instances[i].color);
	}

	//R_CalcSurfaceTrianglePlanes(numTriangles, foliage->triangles, foliage->verts);

	// finish surface
	FinishGenericSurface(ds, (srfGeneric_t *)foliage, foliage->verts[0].xyz);
}

/**
 * @brief ParseFlare
 * @param[in] ds
 * @param verts - unused
 * @param[out] surf
 * @param indexes - unused
 */
static void ParseFlare(dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf, int *indexes)
{
	srfFlare_t *flare;
	int        i;

	// set lightmap
	surf->lightmapNum = LIGHTMAP_BY_VERTEX;

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader
	surf->shader = ShaderForShaderNum(ds->shaderNum);
	if (r_singleShader->integer && !surf->shader->isSky)
	{
		surf->shader = tr.defaultShader;
	}

	flare              = (srfFlare_t *)ri.Hunk_Alloc(sizeof(*flare), h_low);
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t *) flare;

	for (i = 0; i < 3; i++)
	{
		flare->origin[i] = LittleFloat(ds->lightmapOrigin[i]);
		flare->color[i]  = LittleFloat(ds->lightmapVecs[0][i]);
		flare->normal[i] = LittleFloat(ds->lightmapVecs[2][i]);
	}
}

/**
 * @brief R_MergedWidthPoints
 * @param[in] grid
 * @param[in] offset
 * @return true if there are grid points merged on a width edge
 */
int R_MergedWidthPoints(srfGridMesh_t *grid, int offset)
{
	int i, j;

	for (i = 1; i < grid->width - 1; i++)
	{
		for (j = i + 1; j < grid->width - 1; j++)
		{
			if (Q_fabs(grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0]) > .1f)
			{
				continue;
			}
			if (Q_fabs(grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1]) > .1f)
			{
				continue;
			}
			if (Q_fabs(grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2]) > .1f)
			{
				continue;
			}
			return qtrue;
		}
	}
	return qfalse;
}

/**
 * @brief R_MergedHeightPoints
 * @param[in] grid
 * @param[in] offset
 * @return true if there are grid points merged on a height edge
 */
int R_MergedHeightPoints(srfGridMesh_t *grid, int offset)
{
	int i, j;

	for (i = 1; i < grid->height - 1; i++)
	{
		for (j = i + 1; j < grid->height - 1; j++)
		{
			if (Q_fabs(grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0]) > .1f)
			{
				continue;
			}
			if (Q_fabs(grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1]) > .1f)
			{
				continue;
			}
			if (Q_fabs(grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2]) > .1f)
			{
				continue;
			}
			return qtrue;
		}
	}
	return qfalse;
}

/**
 * @brief R_FixSharedVertexLodError_r
 * @param[in] start
 * @param[in] grid1
 *
 * @note never sync LoD through grid edges with merged points!
 *
 * @todo FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
 */
static void R_FixSharedVertexLodError_r(int start, srfGridMesh_t *grid1)
{
	int           j, k, l, m, n, offset1, offset2, touch;
	srfGridMesh_t *grid2;

	for (j = start; j < s_worldData.numSurfaces; j++)
	{
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if (grid2->surfaceType != SF_GRID)
		{
			continue;
		}
		// if the LOD errors are already fixed for this patch
		if (grid2->lodFixed == 2)
		{
			continue;
		}
		// grids in the same LOD group should have the exact same lod radius
		if (grid1->lodRadius != grid2->lodRadius)
		{
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if (grid1->lodOrigin[0] != grid2->lodOrigin[0])
		{
			continue;
		}
		if (grid1->lodOrigin[1] != grid2->lodOrigin[1])
		{
			continue;
		}
		if (grid1->lodOrigin[2] != grid2->lodOrigin[2])
		{
			continue;
		}

		touch = qfalse;
		for (n = 0; n < 2; n++)
		{
			if (n)
			{
				offset1 = (grid1->height - 1) * grid1->width;
			}
			else
			{
				offset1 = 0;
			}
			if (R_MergedWidthPoints(grid1, offset1))
			{
				continue;
			}
			for (k = 1; k < grid1->width - 1; k++)
			{
				for (m = 0; m < 2; m++)
				{
					if (m)
					{
						offset2 = (grid2->height - 1) * grid2->width;
					}
					else
					{
						offset2 = 0;
					}
					if (R_MergedWidthPoints(grid2, offset2))
					{
						continue;
					}
					for (l = 1; l < grid2->width - 1; l++)
					{
						if (Q_fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1f)
						{
							continue;
						}
						if (Q_fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1f)
						{
							continue;
						}
						if (Q_fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1f)
						{
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->widthLodError[k];
						touch                   = qtrue;
					}
				}
				for (m = 0; m < 2; m++)
				{
					if (m)
					{
						offset2 = grid2->width - 1;
					}
					else
					{
						offset2 = 0;
					}
					if (R_MergedHeightPoints(grid2, offset2))
					{
						continue;
					}
					for (l = 1; l < grid2->height - 1; l++)
					{
						if (Q_fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1f)
						{
							continue;
						}
						if (Q_fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1f)
						{
							continue;
						}
						if (Q_fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1f)
						{
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->widthLodError[k];
						touch                    = qtrue;
					}
				}
			}
		}
		for (n = 0; n < 2; n++)
		{
			if (n)
			{
				offset1 = grid1->width - 1;
			}
			else
			{
				offset1 = 0;
			}
			if (R_MergedHeightPoints(grid1, offset1))
			{
				continue;
			}
			for (k = 1; k < grid1->height - 1; k++)
			{
				for (m = 0; m < 2; m++)
				{
					if (m)
					{
						offset2 = (grid2->height - 1) * grid2->width;
					}
					else
					{
						offset2 = 0;
					}
					if (R_MergedWidthPoints(grid2, offset2))
					{
						continue;
					}
					for (l = 1; l < grid2->width - 1; l++)
					{
						if (Q_fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1f)
						{
							continue;
						}
						if (Q_fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1f)
						{
							continue;
						}
						if (Q_fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1f)
						{
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->heightLodError[k];
						touch                   = qtrue;
					}
				}
				for (m = 0; m < 2; m++)
				{
					if (m)
					{
						offset2 = grid2->width - 1;
					}
					else
					{
						offset2 = 0;
					}
					if (R_MergedHeightPoints(grid2, offset2))
					{
						continue;
					}
					for (l = 1; l < grid2->height - 1; l++)
					{
						if (Q_fabs
						        (grid1->verts[grid1->width * k + offset1].xyz[0] -
						        grid2->verts[grid2->width * l + offset2].xyz[0]) > .1f)
						{
							continue;
						}
						if (Q_fabs
						        (grid1->verts[grid1->width * k + offset1].xyz[1] -
						        grid2->verts[grid2->width * l + offset2].xyz[1]) > .1f)
						{
							continue;
						}
						if (Q_fabs
						        (grid1->verts[grid1->width * k + offset1].xyz[2] -
						        grid2->verts[grid2->width * l + offset2].xyz[2]) > .1f)
						{
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->heightLodError[k];
						touch                    = qtrue;
					}
				}
			}
		}
		if (touch)
		{
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r(start, grid2);
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

/**
 * @brief This function assumes that all patches in one group are nicely stitched together for the highest LoD.
 * If this is not the case this function will still do its job but won't fix the highest LoD cracks.
 */
static void R_FixSharedVertexLodError(void)
{
	int           i;
	srfGridMesh_t *grid1;

	for (i = 0; i < s_worldData.numSurfaces; i++)
	{
		grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if (grid1->surfaceType != SF_GRID)
		{
			continue;
		}

		if (grid1->lodFixed)
		{
			continue;
		}

		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r(i + 1, grid1);
	}
}

/**
 * @brief R_StitchPatches
 * @param[in] grid1num
 * @param[in] grid2num
 * @return
 */
static int R_StitchPatches(int grid1num, int grid2num)
{
	float         *v1, *v2;
	int           k, l, m, n, offset1, offset2, row, column;
	srfGridMesh_t *grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	srfGridMesh_t *grid2 = (srfGridMesh_t *) s_worldData.surfaces[grid2num].data;

	for (n = 0; n < 2; n++)
	{

		if (n)
		{
			offset1 = (grid1->height - 1) * grid1->width;
		}
		else
		{
			offset1 = 0;
		}
		if (R_MergedWidthPoints(grid1, offset1))
		{
			continue;
		}
		for (k = 0; k < grid1->width - 2; k += 2)
		{
			for (m = 0; m < 2; m++)
			{
				if (grid2->width >= MAX_GRID_SIZE)
				{
					break;
				}
				if (m)
				{
					offset2 = (grid2->height - 1) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}
				for (l = 0; l < grid2->width - 1; l++)
				{
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) < .01f && Q_fabs(v1[1] - v2[1]) < .01f && Q_fabs(v1[2] - v2[2]) < .01f)
					{
						continue;
					}

					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m)
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}
					grid2                               = R_GridInsertColumn(grid2, l + 1, row, grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1]);
					grid2->lodStitched                  = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *)grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++)
			{
				if (grid2->height >= MAX_GRID_SIZE)
				{
					break;
				}
				if (m)
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}
				for (l = 0; l < grid2->height - 1; l++)
				{
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) < .01f && Q_fabs(v1[1] - v2[1]) < .01f && Q_fabs(v1[2] - v2[2]) < .01f)
					{
						continue;
					}

					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m)
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}
					grid2                               = R_GridInsertRow(grid2, l + 1, column, grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1]);
					grid2->lodStitched                  = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *)grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++)
	{
		if (n)
		{
			offset1 = grid1->width - 1;
		}
		else
		{
			offset1 = 0;
		}
		if (R_MergedHeightPoints(grid1, offset1))
		{
			continue;
		}
		for (k = 0; k < grid1->height - 2; k += 2)
		{
			for (m = 0; m < 2; m++)
			{
				if (grid2->width >= MAX_GRID_SIZE)
				{
					break;
				}
				if (m)
				{
					offset2 = (grid2->height - 1) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}
				for (l = 0; l < grid2->width - 1; l++)
				{
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) < .01f && Q_fabs(v1[1] - v2[1]) < .01f && Q_fabs(v1[2] - v2[2]) < .01f)
					{
						continue;
					}

					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m)
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}
					grid2 = R_GridInsertColumn(grid2, l + 1, row,
					                           grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k + 1]);
					grid2->lodStitched                  = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *)grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++)
			{
				if (grid2->height >= MAX_GRID_SIZE)
				{
					break;
				}
				if (m)
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}
				for (l = 0; l < grid2->height - 1; l++)
				{
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) < .01f && Q_fabs(v1[1] - v2[1]) < .01f && Q_fabs(v1[2] - v2[2]) < .01f)
					{
						continue;
					}

					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m)
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}
					grid2 = R_GridInsertRow(grid2, l + 1, column,
					                        grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k + 1]);
					grid2->lodStitched                  = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *)grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++)
	{
		if (n)
		{
			offset1 = (grid1->height - 1) * grid1->width;
		}
		else
		{
			offset1 = 0;
		}
		if (R_MergedWidthPoints(grid1, offset1))
		{
			continue;
		}
		for (k = grid1->width - 1; k > 1; k -= 2)
		{
			for (m = 0; m < 2; m++)
			{
				if (!grid2 || grid2->width >= MAX_GRID_SIZE)
				{
					break;
				}
				if (m)
				{
					offset2 = (grid2->height - 1) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}
				for (l = 0; l < grid2->width - 1; l++)
				{
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) < .01f && Q_fabs(v1[1] - v2[1]) < .01f && Q_fabs(v1[2] - v2[2]) < .01f)
					{
						continue;
					}

					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m)
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}
					grid2                               = R_GridInsertColumn(grid2, l + 1, row, grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1]);
					grid2->lodStitched                  = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *)grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++)
			{
				if (!grid2 || grid2->height >= MAX_GRID_SIZE)
				{
					break;
				}
				if (m)
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}
				for (l = 0; l < grid2->height - 1; l++)
				{
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) < .01f && Q_fabs(v1[1] - v2[1]) < .01f && Q_fabs(v1[2] - v2[2]) < .01f)
					{
						continue;
					}

					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m)
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}
					grid2 = R_GridInsertRow(grid2, l + 1, column, grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1]);
					if (!grid2)
					{
						break;
					}
					grid2->lodStitched                  = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *)grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++)
	{
		if (n)
		{
			offset1 = grid1->width - 1;
		}
		else
		{
			offset1 = 0;
		}
		if (R_MergedHeightPoints(grid1, offset1))
		{
			continue;
		}
		for (k = grid1->height - 1; k > 1; k -= 2)
		{
			for (m = 0; m < 2; m++)
			{
				if (!grid2 || grid2->width >= MAX_GRID_SIZE)
				{
					break;
				}
				if (m)
				{
					offset2 = (grid2->height - 1) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}
				for (l = 0; l < grid2->width - 1; l++)
				{
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) < .01f && Q_fabs(v1[1] - v2[1]) < .01f && Q_fabs(v1[2] - v2[2]) < .01f)
					{
						continue;
					}

					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m)
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}
					grid2 = R_GridInsertColumn(grid2, l + 1, row,
					                           grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k + 1]);
					grid2->lodStitched                  = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *)grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++)
			{
				if (!grid2 || grid2->height >= MAX_GRID_SIZE)
				{
					break;
				}
				if (m)
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}
				for (l = 0; l < grid2->height - 1; l++)
				{
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[1] - v2[1]) > .1f)
					{
						continue;
					}
					if (Q_fabs(v1[2] - v2[2]) > .1f)
					{
						continue;
					}

					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (Q_fabs(v1[0] - v2[0]) < .01f && Q_fabs(v1[1] - v2[1]) < .01f && Q_fabs(v1[2] - v2[2]) < .01f)
					{
						continue;
					}

					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m)
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}
					grid2 = R_GridInsertRow(grid2, l + 1, column,
					                        grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k + 1]);
					grid2->lodStitched                  = qfalse;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *)grid2;
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/**
 * @brief This function will try to stitch patches in the same LoD group together for the highest LoD.
 *
 * Only single missing vertice cracks will be fixed.
 *
 * Vertices will be joined at the patch side a crack is first found, at the other side
 * of the patch (on the same row or column) the vertices will not be joined and cracks
 * might still appear at that side.
 *
 * @param[in] grid1num
 * @return
 */
static int R_TryStitchingPatch(int grid1num)
{
	int           j, numstitches = 0;
	srfGridMesh_t *grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	srfGridMesh_t *grid2;

	for (j = 0; j < s_worldData.numSurfaces; j++)
	{
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if (grid2->surfaceType != SF_GRID)
		{
			continue;
		}
		// grids in the same LOD group should have the exact same lod radius
		if (grid1->lodRadius != grid2->lodRadius)
		{
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if (grid1->lodOrigin[0] != grid2->lodOrigin[0])
		{
			continue;
		}
		if (grid1->lodOrigin[1] != grid2->lodOrigin[1])
		{
			continue;
		}
		if (grid1->lodOrigin[2] != grid2->lodOrigin[2])
		{
			continue;
		}
		//
		while (R_StitchPatches(grid1num, j))
		{
			numstitches++;
		}
	}
	return numstitches;
}

/**
 * @brief R_StitchAllPatches
 */
static void R_StitchAllPatches(void)
{
	int           i, numstitches = 0;
	srfGridMesh_t *grid1;
	qboolean      stitched;

	Ren_Developer("...stitching LoD cracks\n");

	do
	{
		stitched = qfalse;
		for (i = 0; i < s_worldData.numSurfaces; i++)
		{
			grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
			// if this surface is not a grid
			if (grid1->surfaceType != SF_GRID)
			{
				continue;
			}

			if (grid1->lodStitched)
			{
				continue;
			}

			grid1->lodStitched = qtrue;
			stitched           = qtrue;

			numstitches += R_TryStitchingPatch(i);
		}
	}
	while (stitched);
	Ren_Developer("stitched %d LoD cracks\n", numstitches);
}

/**
 * @brief R_MovePatchSurfacesToHunk
 */
static void R_MovePatchSurfacesToHunk(void)
{
	int           i;
	unsigned int  size;
	srfGridMesh_t *grid, *hunkgrid;

	for (i = 0; i < s_worldData.numSurfaces; i++)
	{
		grid = (srfGridMesh_t *) s_worldData.surfaces[i].data;

		// if this surface is not a grid
		if (grid->surfaceType != SF_GRID)
		{
			continue;
		}

		size     = sizeof(*grid);
		hunkgrid = (srfGridMesh_t *)ri.Hunk_Alloc(size, h_low);
		Com_Memcpy(hunkgrid, grid, size);

		hunkgrid->widthLodError = (float *)ri.Hunk_Alloc(grid->width * 4, h_low);
		Com_Memcpy(hunkgrid->widthLodError, grid->widthLodError, grid->width * 4);

		hunkgrid->heightLodError = (float *)ri.Hunk_Alloc(grid->height * 4, h_low);
		Com_Memcpy(hunkgrid->heightLodError, grid->heightLodError, grid->height * 4);

		hunkgrid->numTriangles = grid->numTriangles;
		hunkgrid->triangles    = (srfTriangle_t *)ri.Hunk_Alloc(grid->numTriangles * sizeof(srfTriangle_t), h_low);
		Com_Memcpy(hunkgrid->triangles, grid->triangles, grid->numTriangles * sizeof(srfTriangle_t));

		hunkgrid->numVerts = grid->numVerts;
		hunkgrid->verts    = (srfVert_t *)ri.Hunk_Alloc(grid->numVerts * sizeof(srfVert_t), h_low);
		Com_Memcpy(hunkgrid->verts, grid->verts, grid->numVerts * sizeof(srfVert_t));

		R_FreeSurfaceGridMesh(grid);

		s_worldData.surfaces[i].data = (surfaceType_t *)hunkgrid;
	}
}

/**
 * @brief Compare function for qsort()
 * @param[in] a
 * @param[in] b
 * @return
 */
int BSPSurfaceCompare(const void *a, const void *b)
{
	bspSurface_t *aa = *(bspSurface_t **) a;
	bspSurface_t *bb = *(bspSurface_t **) b;

	// shader first
	if (aa->shader < bb->shader)
	{
		return -1;
	}
	else if (aa->shader > bb->shader)
	{
		return 1;
	}

	// by lightmap
	if (aa->lightmapNum < bb->lightmapNum)
	{
		return -1;
	}
	else if (aa->lightmapNum > bb->lightmapNum)
	{
		return 1;
	}

	return 0;
}

/**
 * @brief CopyVert
 * @param[in] in
 * @param[out] out
 */
static void CopyVert(const srfVert_t *in, srfVert_t *out)
{
	int j;

	for (j = 0; j < 3; j++)
	{
		out->xyz[j]            = in->xyz[j];
		out->tangent[j]        = in->tangent[j];
		out->binormal[j]       = in->binormal[j];
		out->normal[j]         = in->normal[j];
		out->lightDirection[j] = in->lightDirection[j];
	}

	for (j = 0; j < 2; j++)
	{
		out->st[j]       = in->st[j];
		out->lightmap[j] = in->lightmap[j];
	}

	for (j = 0; j < 4; j++)
	{
		out->paintColor[j] = in->paintColor[j];
		out->lightColor[j] = in->lightColor[j];
	}
}

#define EQUAL_EPSILON   0.001

/**
 * @brief R_CreateClusters
 */
static void R_CreateClusters()
{
	int          i, j;
	bspNode_t    *node;
	bspSurface_t *surface;
#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
	bspNode_t    *parent;
	bspSurface_t **mark;
	int          numClusters;
	bspCluster_t *cluster;
	growList_t   clusterSurfaces;
	const byte   *vis;
	int          c;
	int          surfaceNum;
	vec3_t       mins, maxs;

	Ren_Print("...creating BSP clusters\n");

	if (s_worldData.vis)
	{
		// go through the leaves and count clusters
		numClusters = 0;
		for (i = 0, node = s_worldData.nodes; i < s_worldData.numnodes; i++, node++)
		{
			if (node->cluster >= numClusters)
			{
				numClusters = node->cluster;
			}
		}
		numClusters++;

		s_worldData.numClusters = numClusters;
		s_worldData.clusters    = ri.Hunk_Alloc((numClusters + 1) * sizeof(*s_worldData.clusters), h_low); // + supercluster

		// reset surfaces' viewCount
		for (i = 0, surface = s_worldData.surfaces; i < s_worldData.numSurfaces; i++, surface++)
		{
			surface->viewCount = -1;
		}

		for (j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++)
		{
			node->visCounts[0] = -1;
		}

		for (i = 0; i < numClusters; i++)
		{
			cluster = &s_worldData.clusters[i];

			// mark leaves in cluster
			vis = s_worldData.vis + i * s_worldData.clusterBytes;

			for (j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++)
			{
				if (node->cluster < 0 || node->cluster >= numClusters)
				{
					continue;
				}

				// check general pvs
				if (!(vis[node->cluster >> 3] & (1 << (node->cluster & 7))))
				{
					continue;
				}

				parent = node;
				do
				{
					if (parent->visCounts[0] == i)
					{
						break;
					}
					parent->visCounts[0] = i;
					parent               = parent->parent;
				}
				while (parent);
			}


			// add cluster surfaces
			Com_InitGrowList(&clusterSurfaces, 100); //Com_InitGrowList(&clusterSurfaces, 10000);

			ClearBounds(mins, maxs);
			for (j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++)
			{
				if (node->contents == CONTENTS_NODE)
				{
					continue;
				}

				if (node->visCounts[0] != i)
				{
					continue;
				}

				BoundsAdd(mins, maxs, node->mins, node->maxs);

				mark = node->markSurfaces;
				c    = node->numMarkSurfaces;
				while (c--)
				{
					// the surface may have already been added if it
					// spans multiple leafs
					surface = *mark;

					surfaceNum = surface - s_worldData.surfaces;

					if ((surface->viewCount != i) && (surfaceNum < s_worldData.numWorldSurfaces))
					{
						surface->viewCount = i;
						Com_AddToGrowList(&clusterSurfaces, surface);
					}

					mark++;
				}
			}

			cluster->origin[0] = (mins[0] + maxs[0]) / 2;
			cluster->origin[1] = (mins[1] + maxs[1]) / 2;
			cluster->origin[2] = (mins[2] + maxs[2]) / 2;

			//Ren_Print("cluster %i origin at (%i %i %i)\n", i, (int)cluster->origin[0], (int)cluster->origin[1], (int)cluster->origin[2]);

			// move cluster surfaces list to hunk
			cluster->numMarkSurfaces = clusterSurfaces.currentElements;
			cluster->markSurfaces    = ri.Hunk_Alloc(cluster->numMarkSurfaces * sizeof(*cluster->markSurfaces), h_low);

			for (j = 0; j < cluster->numMarkSurfaces; j++)
			{
				cluster->markSurfaces[j] = (bspSurface_t *) Com_GrowListElement(&clusterSurfaces, j);
			}

			Com_DestroyGrowList(&clusterSurfaces);

			//Ren_Print("cluster %i contains %i bsp surfaces\n", i, cluster->numMarkSurfaces);
		}
	}
	else
	{
		numClusters = 0;

		s_worldData.numClusters = numClusters;
		s_worldData.clusters    = ri.Hunk_Alloc((numClusters + 1) * sizeof(*s_worldData.clusters), h_low); // + supercluster
	}

	// create a super cluster that will be always used when no view cluster can be found
	Com_InitGrowList(&clusterSurfaces, 100); //Com_InitGrowList(&clusterSurfaces, 10000);

	for (i = 0, surface = s_worldData.surfaces; i < s_worldData.numWorldSurfaces; i++, surface++)
	{
		Com_AddToGrowList(&clusterSurfaces, surface);
	}

	cluster                  = &s_worldData.clusters[numClusters];
	cluster->numMarkSurfaces = clusterSurfaces.currentElements;
	cluster->markSurfaces    = ri.Hunk_Alloc(cluster->numMarkSurfaces * sizeof(*cluster->markSurfaces), h_low);

	for (j = 0; j < cluster->numMarkSurfaces; j++)
	{
		cluster->markSurfaces[j] = (bspSurface_t *) Com_GrowListElement(&clusterSurfaces, j);
	}

	Com_DestroyGrowList(&clusterSurfaces);

	for (i = 0; i < MAX_VISCOUNTS; i++)
	{
		Com_InitGrowList(&s_worldData.clusterVBOSurfaces[i], 100);
	}

	//Ren_Print("noVis cluster contains %i bsp surfaces\n", cluster->numMarkSurfaces);

	Ren_Print("%i world clusters created\n", numClusters + 1);
#endif // #if defined(USE_BSP_CLUSTERSURFACE_MERGING)

	// reset surfaces' viewCount
	for (i = 0, surface = s_worldData.surfaces; i < s_worldData.numSurfaces; i++, surface++)
	{
		surface->viewCount = -1;
	}

	for (j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++)
	{
		node->visCounts[0] = -1;
	}
}

/**
 * @brief R_CreateWorldVBO
 */
static void R_CreateWorldVBO()
{
	int       s, i, j, k;
	int       numVerts = 0;
	srfVert_t *verts;
	//srfVert_t      *optimizedVerts;
	int           numTriangles = 0;
	srfTriangle_t *triangles;
	//int             numSurfaces;
	bspSurface_t *surface;
	//trRefLight_t   *light;
#ifdef ETLEGACY_DEBUG
	int startTime, endTime;

	startTime = ri.Milliseconds();
#endif

	for (s = 0, surface = &s_worldData.surfaces[0]; s < s_worldData.numWorldSurfaces; s++, surface++)
	{
		if (*surface->data == SF_FACE)
		{
			srfSurfaceFace_t *face = (srfSurfaceFace_t *) surface->data;

			if (face->numVerts)
			{
				numVerts += face->numVerts;
			}

			if (face->numTriangles)
			{
				numTriangles += face->numTriangles;
			}
		}
		else if (*surface->data == SF_GRID)
		{
			srfGridMesh_t *grid = (srfGridMesh_t *) surface->data;

			if (grid->numVerts)
			{
				numVerts += grid->numVerts;
			}

			if (grid->numTriangles)
			{
				numTriangles += grid->numTriangles;
			}
		}
		else if (*surface->data == SF_TRIANGLES)
		{
			srfTriangles_t *tri = (srfTriangles_t *) surface->data;

			if (tri->numVerts)
			{
				numVerts += tri->numVerts;
			}

			if (tri->numTriangles)
			{
				numTriangles += tri->numTriangles;
			}
		}
		else if (*surface->data == SF_FOLIAGE)
		{
			srfFoliage_t *foliage = (srfFoliage_t *)surface->data;

			if (foliage->numVerts)
			{
				numVerts += foliage->numVerts * foliage->numInstances;
			}

			if (foliage->numTriangles)
			{
				numTriangles += foliage->numTriangles * foliage->numInstances;
			}
		}
	}

	if (!numVerts || !numTriangles)
	{
		return;
	}

	Ren_Developer("...calculating world VBO ( %i verts %i tris )\n", numVerts, numTriangles);

	// create arrays

	s_worldData.numVerts = numVerts;
	s_worldData.verts    = verts = (srfVert_t *)ri.Hunk_Alloc(numVerts * sizeof(srfVert_t), h_low);
	//optimizedVerts = ri.Hunk_AllocateTempMemory(numVerts * sizeof(srfVert_t));

	s_worldData.numTriangles = numTriangles;
	s_worldData.triangles    = triangles = (srfTriangle_t *)ri.Hunk_Alloc(numTriangles * sizeof(srfTriangle_t), h_low);

	// set up triangle indices
	numVerts     = 0;
	numTriangles = 0;
	for (s = 0, surface = &s_worldData.surfaces[0]; s < s_worldData.numWorldSurfaces; s++, surface++)
	{
		if (*surface->data == SF_FACE)
		{
			srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

			srf->firstTriangle = numTriangles;

			if (srf->numTriangles)
			{
				srfTriangle_t *tri;

				for (i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
				{
					for (j = 0; j < 3; j++)
					{
						triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if (srf->numVerts)
			{
				numVerts += srf->numVerts;
			}
		}
		else if (*surface->data == SF_GRID)
		{
			srfGridMesh_t *srf = (srfGridMesh_t *) surface->data;

			srf->firstTriangle = numTriangles;

			if (srf->numTriangles)
			{
				srfTriangle_t *tri;

				for (i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
				{
					for (j = 0; j < 3; j++)
					{
						triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if (srf->numVerts)
			{
				numVerts += srf->numVerts;
			}
		}
		else if (*surface->data == SF_TRIANGLES)
		{
			srfTriangles_t *srf = (srfTriangles_t *) surface->data;

			srf->firstTriangle = numTriangles;

			if (srf->numTriangles)
			{
				srfTriangle_t *tri;

				for (i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
				{
					for (j = 0; j < 3; j++)
					{
						triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if (srf->numVerts)
			{
				numVerts += srf->numVerts;
			}
		}
		else if (*surface->data == SF_FOLIAGE)
		{
			srfFoliage_t *srf = (srfFoliage_t *)surface->data;

			srf->firstTriangle = numTriangles;

			if (srf->numTriangles)
			{
				srfTriangle_t *tri;

				for (i = 0; i < srf->numInstances; i++)
				{
					for (j = 0, tri = srf->triangles; j < srf->numTriangles; j++, tri++)
					{
						for (k = 0; k < 3; k++)
						{
							triangles[numTriangles + j].indexes[k] = numVerts + tri->indexes[k];
						}
					}
					numTriangles += srf->numTriangles;
					numVerts     += srf->numVerts;
				}
			}
		}
	}

	// build vertices
	numVerts = 0;
	for (s = 0, surface = &s_worldData.surfaces[0]; s < s_worldData.numWorldSurfaces; s++, surface++)
	{
		if (*surface->data == SF_FACE)
		{
			srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

			//srf->firstVert = numVerts;

			if (srf->numVerts)
			{
				for (i = 0; i < srf->numVerts; i++)
				{
					CopyVert(&srf->verts[i], &verts[numVerts + i]);
				}

				numVerts += srf->numVerts;
			}
		}
		else if (*surface->data == SF_GRID)
		{
			srfGridMesh_t *srf = (srfGridMesh_t *) surface->data;

			//srf->firstVert = numVerts;

			if (srf->numVerts)
			{
				for (i = 0; i < srf->numVerts; i++)
				{
					CopyVert(&srf->verts[i], &verts[numVerts + i]);
				}

				numVerts += srf->numVerts;
			}
		}
		else if (*surface->data == SF_TRIANGLES)
		{
			srfTriangles_t *srf = (srfTriangles_t *) surface->data;

			//srf->firstVert = numVerts;

			if (srf->numVerts)
			{
				for (i = 0; i < srf->numVerts; i++)
				{
					CopyVert(&srf->verts[i], &verts[numVerts + i]);
				}

				numVerts += srf->numVerts;
			}
		}
		else if (*surface->data == SF_FOLIAGE)
		{
			srfFoliage_t *srf = (srfFoliage_t *)surface->data;

			if (srf->numVerts)
			{
				srfVert_t         vert;
				foliageInstance_t *finstance;

				for (i = 0; i < srf->numInstances; i++)
				{
					finstance = &srf->instances[i];
					for (j = 0; j < srf->numVerts; j++)
					{
						Com_Memcpy(&vert, &srf->verts[j], sizeof(vert));
						VectorAdd(vert.xyz, finstance->origin, vert.xyz);

						vert.lightColor[0] = finstance->color[0] / 255.f;
						vert.lightColor[1] = finstance->color[1] / 255.f;
						vert.lightColor[2] = finstance->color[2] / 255.f;
						vert.lightColor[3] = finstance->color[3] / 255.f;

						CopyVert(&vert, &verts[numVerts + j]);
					}

					numVerts += srf->numVerts;
				}
			}
		}
	}

#if 0
	// assign the two nearest cubeProbes
	if (tr.cubeProbes.currentElements > 0)
	{
		srfGeneric_t *srf;
		cubemapProbe_t *cubeProbe1, *cubeProbe2;
		float distance1, distance2;
		for (s = 0, surface = &s_worldData.surfaces[0]; s < s_worldData.numWorldSurfaces; s++, surface++)
		{
			if (*surface->data == SF_FACE) srf = (srfGeneric_t *)surface->data;
			else if (*surface->data == SF_GRID) srf = (srfGeneric_t *)surface->data;
			else if (*surface->data == SF_TRIANGLES) srf = (srfGeneric_t *)surface->data;
			else if (*surface->data == SF_FOLIAGE) srf = (srfGeneric_t *)surface->data;
			else continue;

			// find the two nearest cubeProbes
			R_FindTwoNearestCubeMaps(srf->origin, &cubeProbe1, &cubeProbe2, &distance1, &distance2);
			srf->cubemap1 = cubeProbe1->cubemap;
			srf->cubemap2 = cubeProbe2->cubemap;
		}
	}
#endif

	s_worldData.vbo = R_CreateVBO2(va("staticBspModel0_VBO %i", 0), numVerts, verts,
	                               ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BINORMAL |
	                               ATTR_NORMAL | ATTR_COLOR
	                               , VBO_USAGE_STATIC);

	s_worldData.ibo = R_CreateIBO2(va("staticBspModel0_IBO %i", 0), numTriangles, triangles, VBO_USAGE_STATIC);

#ifdef ETLEGACY_DEBUG
	endTime = ri.Milliseconds();
	Ren_Developer("world VBO calculation time = %5.2f seconds\n", (endTime - startTime) / 1000.0);
#endif

	// point triangle surfaces to world VBO
	for (s = 0, surface = &s_worldData.surfaces[0]; s < s_worldData.numWorldSurfaces; s++, surface++)
	{
		if (*surface->data == SF_FACE)
		{
			srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

			//if(r_vboFaces->integer && srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
				//srf->ibo = R_CreateIBO2(va("staticBspModel0_planarSurface_IBO %i", k), srf->numTriangles, triangles + srf->firstTriangle, VBO_USAGE_STATIC);
			}
		}
		else if (*surface->data == SF_GRID)
		{
			srfGridMesh_t *srf = (srfGridMesh_t *) surface->data;

			//if(r_vboCurves->integer && srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
				//srf->ibo = R_CreateIBO2(va("staticBspModel0_curveSurface_IBO %i", k), srf->numTriangles, triangles + srf->firstTriangle, VBO_USAGE_STATIC);
			}
		}
		else if (*surface->data == SF_TRIANGLES)
		{
			srfTriangles_t *srf = (srfTriangles_t *) surface->data;

			//if(r_vboTriangles->integer && srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
				//srf->ibo = R_CreateIBO2(va("staticBspModel0_triangleSurface_IBO %i", k), srf->numTriangles, triangles + srf->firstTriangle, VBO_USAGE_STATIC);
			}
		}
		else if (*surface->data == SF_FOLIAGE)
		{
			srfFoliage_t *srf = (srfFoliage_t *) surface->data;

			//if(r_vboFoliage->integer && srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
			}
		}
	}

	// FIXME move this to somewhere else?
#if CALC_REDUNDANT_SHADOWVERTS

	startTime = ri.Milliseconds();

	s_worldData.redundantVertsCalculationNeeded = 0;
	for (i = 0; i < s_worldData.numLights; i++)
	{
		light = &s_worldData.lights[i];

		if ((r_precomputedLighting->integer || r_vertexLighting->integer) && !light->noRadiosity)
		{
			continue;
		}

		s_worldData.redundantVertsCalculationNeeded++;
	}

	if (s_worldData.redundantVertsCalculationNeeded)
	{
		Ren_Print("...calculating redundant world vertices ( %i verts )\n", numVerts);

		s_worldData.redundantLightVerts = ri.Hunk_Alloc(numVerts * sizeof(int), h_low);
		BuildRedundantIndices(numVerts, verts, s_worldData.redundantLightVerts, CompareLightVert);

		s_worldData.redundantShadowVerts = ri.Hunk_Alloc(numVerts * sizeof(int), h_low);
		BuildRedundantIndices(numVerts, verts, s_worldData.redundantShadowVerts, CompareShadowVert);

		s_worldData.redundantShadowAlphaTestVerts = ri.Hunk_Alloc(numVerts * sizeof(int), h_low);
		BuildRedundantIndices(numVerts, verts, s_worldData.redundantShadowAlphaTestVerts, CompareShadowVertAlphaTest);
	}

	endTime = ri.Milliseconds();
	Ren_Print("redundant world vertices calculation time = %5.2f seconds\n", (endTime - startTime) / 1000.0);
#endif

	//ri.Hunk_FreeTempMemory(triangles);
	//ri.Hunk_FreeTempMemory(optimizedVerts);
	//ri.Hunk_FreeTempMemory(verts);
}

/**
 * @brief R_CreateSubModelVBOs
 */
static void R_CreateSubModelVBOs()
{
	int           i, j, k, l, m;
	int           numVerts;
	srfVert_t     *verts;
	srfVert_t     *optimizedVerts;
	int           numTriangles;
	srfTriangle_t *triangles;
	shader_t      *shader, *oldShader;
	int           lightmapNum, oldLightmapNum;
	int           numSurfaces;
	bspSurface_t  *surface, *surface2;
	bspSurface_t  **surfacesSorted;
	bspModel_t    *model;
	growList_t    vboSurfaces;
	srfVBOMesh_t  *vboSurf;

	for (m = 1, model = s_worldData.models; m < s_worldData.numModels; m++, model++)
	{
		// count number of static area surfaces
		numSurfaces = 0;
		for (k = 0; k < model->numSurfaces; k++)
		{
			surface = model->firstSurface + k;
			shader  = surface->shader;

			if (shader->isSky)
			{
				continue;
			}

			if (shader->isPortal)
			{
				continue;
			}

			if (ShaderRequiresCPUDeforms(shader))
			{
				continue;
			}

			numSurfaces++;
		}

		if (!numSurfaces)
		{
			continue;
		}

		// build interaction caches list
		surfacesSorted = (bspSurface_t **)ri.Hunk_AllocateTempMemory(numSurfaces * sizeof(surfacesSorted[0]));

		numSurfaces = 0;
		for (k = 0; k < model->numSurfaces; k++)
		{
			surface = model->firstSurface + k;
			shader  = surface->shader;

			if (shader->isSky)
			{
				continue;
			}

			if (shader->isPortal)
			{
				continue;
			}

			if (ShaderRequiresCPUDeforms(shader))
			{
				continue;
			}

			surfacesSorted[numSurfaces] = surface;
			numSurfaces++;
		}

		Com_InitGrowList(&vboSurfaces, 1024);

		// sort surfaces by shader
		qsort(surfacesSorted, numSurfaces, sizeof(*surfacesSorted), BSPSurfaceCompare);

		// create a VBO for each shader
		oldShader      = NULL;
		oldLightmapNum = LIGHTMAP_NONE;

		for (k = 0; k < numSurfaces; k++)
		{
			surface     = surfacesSorted[k];
			shader      = surface->shader;
			lightmapNum = surface->lightmapNum;

			if (shader != oldShader || (r_precomputedLighting->integer ? lightmapNum != oldLightmapNum : qfalse))
			{
				oldShader      = shader;
				oldLightmapNum = lightmapNum;

				// count vertices and indices
				numVerts     = 0;
				numTriangles = 0;

				for (l = k; l < numSurfaces; l++)
				{
					surface2 = surfacesSorted[l];

					if (surface2->shader != shader)
					{
						continue;
					}

					if (*surface2->data == SF_FACE)
					{
						srfSurfaceFace_t *face = (srfSurfaceFace_t *) surface2->data;

						if (face->numVerts)
						{
							numVerts += face->numVerts;
						}

						if (face->numTriangles)
						{
							numTriangles += face->numTriangles;
						}
					}
					else if (*surface2->data == SF_GRID)
					{
						srfGridMesh_t *grid = (srfGridMesh_t *) surface2->data;

						if (grid->numVerts)
						{
							numVerts += grid->numVerts;
						}

						if (grid->numTriangles)
						{
							numTriangles += grid->numTriangles;
						}
					}
					else if (*surface2->data == SF_TRIANGLES)
					{
						srfTriangles_t *tri = (srfTriangles_t *) surface2->data;

						if (tri->numVerts)
						{
							numVerts += tri->numVerts;
						}

						if (tri->numTriangles)
						{
							numTriangles += tri->numTriangles;
						}
					}
					else if (*surface2->data == SF_FOLIAGE)
					{
						srfFoliage_t *fol = (srfFoliage_t *) surface2->data;

						if (fol->numVerts)
						{
							numVerts += fol->numVerts;
						}

						if (fol->numTriangles)
						{
							numTriangles += fol->numTriangles;
						}
					}
				}

				if (!numVerts || !numTriangles)
				{
					continue;
				}

				Ren_Developer("...calculating entity mesh VBOs ( %s, %i verts %i tris )\n", shader->name, numVerts,
				              numTriangles);

				// create surface
				vboSurf = (srfVBOMesh_t *)ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
				Com_AddToGrowList(&vboSurfaces, vboSurf);

				vboSurf->surfaceType = SF_VBO_MESH;
				vboSurf->numIndexes  = numTriangles * 3;
				vboSurf->numVerts    = numVerts;

				vboSurf->shader      = shader;
				vboSurf->lightmapNum = lightmapNum;

				// create arrays
				verts          = (srfVert_t *)ri.Hunk_AllocateTempMemory(numVerts * sizeof(srfVert_t));
				optimizedVerts = (srfVert_t *)ri.Hunk_AllocateTempMemory(numVerts * sizeof(srfVert_t));
				numVerts       = 0;

				triangles    = (srfTriangle_t *)ri.Hunk_AllocateTempMemory(numTriangles * sizeof(srfTriangle_t));
				numTriangles = 0;

				ClearBounds(vboSurf->bounds[0], vboSurf->bounds[1]);

				// build triangle indices
				for (l = k; l < numSurfaces; l++)
				{
					surface2 = surfacesSorted[l];

					if (surface2->shader != shader)
					{
						continue;
					}

					// set up triangle indices
					if (*surface2->data == SF_FACE)
					{
						srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface2->data;

						if (srf->numTriangles)
						{
							srfTriangle_t *tri;

							for (i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
							{
								for (j = 0; j < 3; j++)
								{
									triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
								}
							}

							numTriangles += srf->numTriangles;
						}

						if (srf->numVerts)
						{
							numVerts += srf->numVerts;
						}
					}
					else if (*surface2->data == SF_GRID)
					{
						srfGridMesh_t *srf = (srfGridMesh_t *) surface2->data;

						if (srf->numTriangles)
						{
							srfTriangle_t *tri;

							for (i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
							{
								for (j = 0; j < 3; j++)
								{
									triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
								}
							}

							numTriangles += srf->numTriangles;
						}

						if (srf->numVerts)
						{
							numVerts += srf->numVerts;
						}
					}
					else if (*surface2->data == SF_TRIANGLES)
					{
						srfTriangles_t *srf = (srfTriangles_t *) surface2->data;

						if (srf->numTriangles)
						{
							srfTriangle_t *tri;

							for (i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
							{
								for (j = 0; j < 3; j++)
								{
									triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
								}
							}

							numTriangles += srf->numTriangles;
						}

						if (srf->numVerts)
						{
							numVerts += srf->numVerts;
						}
					}
					else if (*surface2->data == SF_FOLIAGE)
					{
						srfFoliage_t *srf = (srfFoliage_t *) surface2->data;

						if (srf->numTriangles)
						{
							srfTriangle_t *tri;

							for (i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
							{
								for (j = 0; j < 3; j++)
								{
									triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
								}
							}

							numTriangles += srf->numTriangles;
						}

						if (srf->numVerts)
						{
							numVerts += srf->numVerts;
						}
					}
				}

				// build vertices
				numVerts = 0;
				for (l = k; l < numSurfaces; l++)
				{
					surface2 = surfacesSorted[l];

					if (surface2->shader != shader)
					{
						continue;
					}

					if (*surface2->data == SF_FACE)
					{
						srfSurfaceFace_t *cv = (srfSurfaceFace_t *) surface2->data;

						if (cv->numVerts)
						{
							for (i = 0; i < cv->numVerts; i++)
							{
								CopyVert(&cv->verts[i], &verts[numVerts + i]);

								AddPointToBounds(cv->verts[i].xyz, vboSurf->bounds[0], vboSurf->bounds[1]);
							}

							numVerts += cv->numVerts;
						}
					}
					else if (*surface2->data == SF_GRID)
					{
						srfGridMesh_t *cv = (srfGridMesh_t *) surface2->data;

						if (cv->numVerts)
						{
							for (i = 0; i < cv->numVerts; i++)
							{
								CopyVert(&cv->verts[i], &verts[numVerts + i]);

								AddPointToBounds(cv->verts[i].xyz, vboSurf->bounds[0], vboSurf->bounds[1]);
							}

							numVerts += cv->numVerts;
						}
					}
					else if (*surface2->data == SF_TRIANGLES)
					{
						srfTriangles_t *cv = (srfTriangles_t *) surface2->data;

						if (cv->numVerts)
						{
							for (i = 0; i < cv->numVerts; i++)
							{
								CopyVert(&cv->verts[i], &verts[numVerts + i]);

								AddPointToBounds(cv->verts[i].xyz, vboSurf->bounds[0], vboSurf->bounds[1]);
							}

							numVerts += cv->numVerts;
						}
					}
					else if (*surface2->data == SF_FOLIAGE)
					{
						srfFoliage_t *cv = (srfFoliage_t *) surface2->data;

						if (cv->numVerts)
						{
							for (i = 0; i < cv->numVerts; i++)
							{
								CopyVert(&cv->verts[i], &verts[numVerts + i]);

								AddPointToBounds(cv->verts[i].xyz, vboSurf->bounds[0], vboSurf->bounds[1]);
							}

							numVerts += cv->numVerts;
						}
					}
				}

				vboSurf->vbo =
					R_CreateVBO2(va("staticBspModel%i_VBO %i", m, vboSurfaces.currentElements), numVerts, verts,
					             ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL
					             | ATTR_COLOR
					             , VBO_USAGE_STATIC);

				vboSurf->ibo =
					R_CreateIBO2(va("staticBspModel%i_IBO %i", m, vboSurfaces.currentElements), numTriangles, triangles,
					             VBO_USAGE_STATIC);

				ri.Hunk_FreeTempMemory(triangles);
				ri.Hunk_FreeTempMemory(optimizedVerts);
				ri.Hunk_FreeTempMemory(verts);
			}
		}

		ri.Hunk_FreeTempMemory(surfacesSorted);

		// move VBO surfaces list to hunk
		model->numVBOSurfaces = vboSurfaces.currentElements;
		model->vboSurfaces    = (srfVBOMesh_t **)ri.Hunk_Alloc(model->numVBOSurfaces * sizeof(*model->vboSurfaces), h_low);

		for (i = 0; i < model->numVBOSurfaces; i++)
		{
			model->vboSurfaces[i] = (srfVBOMesh_t *) Com_GrowListElement(&vboSurfaces, i);
		}

		Com_DestroyGrowList(&vboSurfaces);

		Ren_Developer("%i VBO surfaces created for BSP submodel %i\n", model->numVBOSurfaces, m);
	}
}

/**
 * @brief R_LoadSurfaces
 * @param[in] surfs
 * @param[in] verts
 * @param[in] indexLump
 */
static void R_LoadSurfaces(lump_t *surfs, lump_t *verts, lump_t *indexLump)
{
	dsurface_t   *in = (dsurface_t *)(fileBase + surfs->fileofs);
	bspSurface_t *out;
	drawVert_t   *dv;
	int          *indexes;
	int          count;
	int          numFaces = 0, numMeshes = 0, numTriSurfs = 0, numFlares = 0, numFoliages = 0;
	int          i;

	Ren_Print("...loading surfaces\n");

	if (surfs->filelen % sizeof(*in))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	count = surfs->filelen / sizeof(*in);

	dv = (drawVert_t *)(fileBase + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}

	indexes = (int *)(fileBase + indexLump->fileofs);
	if (indexLump->filelen % sizeof(*indexes))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}

	out = (bspSurface_t *)ri.Hunk_Alloc(count * sizeof(*out), h_low);

	s_worldData.surfaces    = out;
	s_worldData.numSurfaces = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		switch (LittleLong(in->surfaceType))
		{
		case MST_PATCH:
			ParseMesh(in, dv, out);
			numMeshes++;
			break;
		case MST_TRIANGLE_SOUP:
			ParseTriSurf(in, dv, out, indexes);
			numTriSurfs++;
			break;
		case MST_PLANAR:
			ParseFace(in, dv, out, indexes);
			numFaces++;
			break;
		case MST_FLARE:
			ParseFlare(in, dv, out, indexes);
			numFlares++;
			break;
		case MST_FOLIAGE:
			ParseFoliage(in, dv, out, indexes);
			numFoliages++;
			break;
		default:
			Ren_Drop("Bad surfaceType");
		}
	}

	Ren_Print("...loaded %d faces, %i meshes, %i trisurfs, %i flares, %i foliages\n", numFaces, numMeshes, numTriSurfs,
	          numFlares, numFoliages);

	if (r_stitchCurves->integer)
	{
		R_StitchAllPatches();
	}

	R_FixSharedVertexLodError();

	if (r_stitchCurves->integer)
	{
		R_MovePatchSurfacesToHunk();
	}
}

/**
 * @brief R_LoadSubmodels
 * @param[in] l
 */
static void R_LoadSubmodels(lump_t *l)
{
	dmodel_t   *in = (dmodel_t *)(fileBase + l->fileofs);
	bspModel_t *out;
	int        i, j, count;

	Ren_Print("...loading submodels\n");

	if (l->filelen % sizeof(*in))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	count = l->filelen / sizeof(*in);

	s_worldData.numModels = count;
	s_worldData.models    = out = (bspModel_t *)ri.Hunk_Alloc(count * sizeof(*out), h_low);

	for (i = 0; i < count; i++, in++, out++)
	{
		model_t *model;

		model = R_AllocModel();

		etl_assert(model != NULL);  // this should never happen
		if (model == NULL)
		{
			Ren_Drop("R_LoadSubmodels: R_AllocModel() failed");
		}

		model->type = MOD_BSP;
		model->bsp  = out;
		Com_sprintf(model->name, sizeof(model->name), "*%d", i);

		for (j = 0; j < 3; j++)
		{
			out->bounds[0][j] = LittleFloat(in->mins[j]);
			out->bounds[1][j] = LittleFloat(in->maxs[j]);
		}

		out->firstSurface = s_worldData.surfaces + LittleLong(in->firstSurface);
		out->numSurfaces  = LittleLong(in->numSurfaces);

		if (i == 0)
		{
			// add this for limiting VBO surface creation
			s_worldData.numWorldSurfaces = out->numSurfaces;
		}

		// for attaching fog brushes to models
		out->firstBrush = LittleLong(in->firstBrush);
		out->numBrushes = LittleLong(in->numBrushes);

		// allocate decal memory
		j           = (i == 0 ? MAX_WORLD_DECALS : MAX_ENTITY_DECALS);
		out->decals = (decal_t *)ri.Hunk_Alloc(j * sizeof(*out->decals), h_low);
		Com_Memset(out->decals, 0, j * sizeof(*out->decals));
	}
}

//==================================================================

/**
 * @brief R_SetParent
 * @param[in,out] node
 * @param[in] parent
 */
static void R_SetParent(bspNode_t *node, bspNode_t *parent)
{
	node->parent = parent;

	if (node->contents != CONTENTS_NODE)
	{
		// add node surfaces to bounds
		if (node->numMarkSurfaces > 0)
		{
			int          c;
			bspSurface_t **mark;
			srfGeneric_t *gen;
			qboolean     mergedSurfBounds;

			// add node surfaces to bounds
			mark = node->markSurfaces;
			c    = node->numMarkSurfaces;
			ClearBounds(node->surfMins, node->surfMaxs);
			mergedSurfBounds = qfalse;
			while (c--)
			{
				gen = (srfGeneric_t *) (**mark).data;
				if (gen->surfaceType != SF_FACE &&
				    gen->surfaceType != SF_GRID &&
				    gen->surfaceType != SF_TRIANGLES &&
				    gen->surfaceType != SF_FOLIAGE)
				{
					mark++;
					continue;
				}
				AddPointToBounds(gen->bounds[0], node->surfMins, node->surfMaxs);
				AddPointToBounds(gen->bounds[1], node->surfMins, node->surfMaxs);
				mark++;
				mergedSurfBounds = qtrue;
			}

			if (!mergedSurfBounds)
			{
				VectorCopy(node->mins, node->surfMins);
				VectorCopy(node->maxs, node->surfMaxs);
			}
		}

		return;
	}

	R_SetParent(node->children[0], node);
	R_SetParent(node->children[1], node);

	// surface bounds
	BoundsAdd(node->surfMins, node->surfMaxs, node->children[0]->surfMins, node->children[0]->surfMaxs);
	BoundsAdd(node->surfMins, node->surfMaxs, node->children[1]->surfMins, node->children[1]->surfMaxs);
}

/**
 * @brief R_LoadNodesAndLeafs
 * @param[in] nodeLump
 * @param[in] leafLump
 */
static void R_LoadNodesAndLeafs(lump_t *nodeLump, lump_t *leafLump)
{
	int           i, j, p;
	dnode_t       *in = (dnode_t *)(fileBase + nodeLump->fileofs);
	dleaf_t       *inLeaf;
	bspNode_t     *out;
	int           numNodes, numLeafs;
	srfVert_t     *verts = NULL;
	srfTriangle_t *triangles = NULL;
	IBO_t         *volumeIBO = NULL;
	vec3_t        mins, maxs;
	//vec3_t      offset = {0.01, 0.01, 0.01};

	Ren_Print("...loading nodes and leaves\n");

	if (nodeLump->filelen % sizeof(dnode_t) || leafLump->filelen % sizeof(dleaf_t))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	numNodes = nodeLump->filelen / sizeof(dnode_t);
	numLeafs = leafLump->filelen / sizeof(dleaf_t);

	out = (bspNode_t *)ri.Hunk_Alloc((numNodes + numLeafs) * sizeof(*out), h_low);

	s_worldData.nodes            = out;
	s_worldData.numnodes         = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// skybox optimization
	s_worldData.numSkyNodes = 0;
	s_worldData.skyNodes    = (bspNode_t **)ri.Hunk_Alloc(WORLD_MAX_SKY_NODES * sizeof(*s_worldData.skyNodes), h_low);

	// load nodes
	for (i = 0; i < numNodes; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleLong(in->mins[j]);
			out->maxs[j] = LittleLong(in->maxs[j]);
		}

		// surface bounds
		VectorCopy(out->mins, out->surfMins);
		VectorCopy(out->maxs, out->surfMaxs);

		p          = LittleLong(in->planeNum);
		out->plane = s_worldData.planes + p;

		out->contents = CONTENTS_NODE;  // differentiate from leafs

		for (j = 0; j < 2; j++)
		{
			p = LittleLong(in->children[j]);
			if (p >= 0)
			{
				out->children[j] = s_worldData.nodes + p;
			}
			else
			{
				out->children[j] = s_worldData.nodes + numNodes + (-1 - p);
			}
		}
	}

	// load leafs
	inLeaf = (dleaf_t *)(fileBase + leafLump->fileofs);
	for (i = 0; i < numLeafs; i++, inLeaf++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleLong(inLeaf->mins[j]);
			out->maxs[j] = LittleLong(inLeaf->maxs[j]);
		}

		// surface bounds
		ClearBounds(out->surfMins, out->surfMaxs);

		out->cluster = LittleLong(inLeaf->cluster);
		out->area    = LittleLong(inLeaf->area);

		if (out->cluster >= s_worldData.numClusters)
		{
			s_worldData.numClusters = out->cluster + 1;
		}

		out->markSurfaces    = s_worldData.markSurfaces + LittleLong(inLeaf->firstLeafSurface);
		out->numMarkSurfaces = LittleLong(inLeaf->numLeafSurfaces);
	}

	// chain decendants and compute surface bounds
	R_SetParent(s_worldData.nodes, NULL);

	// calculate occlusion query volumes
	for (j = 0, out = &s_worldData.nodes[0]; j < s_worldData.numnodes; j++, out++)
	{
		//if(out->contents != -1 && !out->numMarkSurfaces)
		//	Ren_Drop( "leaf %i is empty", j);

		Com_Memset(out->lastVisited, -1, sizeof(out->lastVisited));
		Com_Memset(out->visible, qfalse, sizeof(out->visible));
		//out->occlusionQuerySamples[0] = 1;

		InitLink(&out->visChain, out);
		InitLink(&out->occlusionQuery, out);
		InitLink(&out->occlusionQuery2, out);
		//QueueInit(&node->multiQuery);

		glGenQueriesARB(MAX_VIEWS, out->occlusionQueryObjects);

		tess.multiDrawPrimitives = 0;
		tess.numIndexes          = 0;
		tess.numVertexes         = 0;

		VectorCopy(out->mins, mins);
		VectorCopy(out->maxs, maxs);

		for (i = 0; i < 3; i++)
		{
			out->origin[i] = (mins[i] + maxs[i]) * 0.5f;
		}

#if 0
		// HACK: make the AABB a little bit smaller to avoid z-fighting for the occlusion queries
		VectorAdd(mins, offset, mins);
		VectorSubtract(maxs, offset, maxs);
#endif
		Tess_AddCube(vec3_origin, mins, maxs, colorWhite);

		if (j == 0)
		{
			verts     = (srfVert_t *)ri.Hunk_AllocateTempMemory(tess.numVertexes * sizeof(srfVert_t));
			triangles = (srfTriangle_t *)ri.Hunk_AllocateTempMemory((tess.numIndexes / 3) * sizeof(srfTriangle_t));
		}

		for (i = 0; i < tess.numVertexes; i++)
		{
			VectorCopy(tess.xyz[i], verts[i].xyz);
		}

		for (i = 0; i < (tess.numIndexes / 3); i++)
		{
			triangles[i].indexes[0] = tess.indexes[i * 3 + 0];
			triangles[i].indexes[1] = tess.indexes[i * 3 + 1];
			triangles[i].indexes[2] = tess.indexes[i * 3 + 2];
		}

		out->volumeVBO = R_CreateVBO2(va("staticBspNode_VBO %i", j), tess.numVertexes, verts, ATTR_POSITION, VBO_USAGE_STATIC);

		if (j == 0)
		{
			out->volumeIBO = volumeIBO = R_CreateIBO2(va("staticBspNode_IBO %i", j), tess.numIndexes / 3, triangles, VBO_USAGE_STATIC);
		}
		else
		{
			out->volumeIBO = volumeIBO;
		}

		out->volumeVerts   = tess.numVertexes;
		out->volumeIndexes = tess.numIndexes;
	}

	if (triangles)
	{
		ri.Hunk_FreeTempMemory(triangles);
	}
	if (verts)
	{
		ri.Hunk_FreeTempMemory(verts);
	}

	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;
}

//=============================================================================

/**
 * @brief R_LoadShaders
 * @param[in] l
 */
static void R_LoadShaders(lump_t *l)
{
	int       i, count;
	dshader_t *in = (dshader_t *)(fileBase + l->fileofs);
	dshader_t *out;

	Ren_Print("...loading shaders\n");

	if (l->filelen % sizeof(*in))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	count = l->filelen / sizeof(*in);
	out   = (dshader_t *)ri.Hunk_Alloc(count * sizeof(*out), h_low);

	s_worldData.shaders    = out;
	s_worldData.numShaders = count;

	Com_Memcpy(out, in, count * sizeof(*out));

	for (i = 0; i < count; i++)
	{
		Ren_Developer("shader: '%s'\n", out[i].shader);

		out[i].surfaceFlags = LittleLong(out[i].surfaceFlags);
		out[i].contentFlags = LittleLong(out[i].contentFlags);
	}
}

/**
 * @brief R_LoadMarksurfaces
 * @param[in] l
 */
static void R_LoadMarksurfaces(lump_t *l)
{
	int          i, j, count;
	int          *in = (int *)(fileBase + l->fileofs);
	bspSurface_t **out;

	Ren_Print("...loading mark surfaces\n");

	if (l->filelen % sizeof(*in))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	count = l->filelen / sizeof(*in);
	out   = (bspSurface_t **)ri.Hunk_Alloc(count * sizeof(*out), h_low);

	s_worldData.markSurfaces    = out;
	s_worldData.numMarkSurfaces = count;

	for (i = 0; i < count; i++)
	{
		j      = LittleLong(in[i]);
		out[i] = s_worldData.surfaces + j;
	}
}

/**
 * @brief R_LoadPlanes
 * @param[in] l
 */
static void R_LoadPlanes(lump_t *l)
{
	int      i, j;
	cplane_t *out;
	dplane_t *in = (dplane_t *)(fileBase + l->fileofs);
	int      count;
	int      bits;

	Ren_Print("...loading planes\n");

	if (l->filelen % sizeof(*in))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	count = l->filelen / sizeof(*in);
	out   = (cplane_t *)ri.Hunk_Alloc(count * 2 * sizeof(*out), h_low);

	s_worldData.planes    = out;
	s_worldData.numplanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
			{
				bits |= 1 << j;
			}
		}

		out->dist     = LittleFloat(in->dist);
		out->type     = PlaneTypeForNormal(out->normal);
		out->signbits = bits;
	}
}

/**
 * @brief R_LoadFogs
 * @param[in] l
 * @param[in] brushesLump
 * @param[in] sidesLump
 */
static void R_LoadFogs(lump_t *l, lump_t *brushesLump, lump_t *sidesLump)
{
	int          i, j;
	fog_t        *out;
	dfog_t       *fogs = (dfog_t *)(fileBase + l->fileofs);
	dbrush_t     *brushes, *brush;
	dbrushside_t *sides;
	int          count, brushesCount, sidesCount;
	int          sideNum;
	int          planeNum;
	shader_t     *shader;
	float        d;
	int          firstSide;

	Ren_Print("...loading fogs\n");

	if (l->filelen % sizeof(*fogs))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	count = l->filelen / sizeof(*fogs);

	// create fog strucutres for them
	s_worldData.numFogs = count + 1;
	s_worldData.fogs    = (fog_t *)ri.Hunk_Alloc(s_worldData.numFogs * sizeof(*out), h_low);
	out                 = s_worldData.fogs + 1;

	// reset global fog
	s_worldData.globalFog = -1;

	if (!count)
	{
		Ren_Print("no fog volumes loaded\n");
		return;
	}

	brushes = (dbrush_t *)(fileBase + brushesLump->fileofs);
	if (brushesLump->filelen % sizeof(*brushes))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	brushesCount = brushesLump->filelen / sizeof(*brushes);

	sides = (dbrushside_t *)(fileBase + sidesLump->fileofs);
	if (sidesLump->filelen % sizeof(*sides))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	sidesCount = sidesLump->filelen / sizeof(*sides);

	for (i = 0; i < count; i++, fogs++)
	{
		out->originalBrushNumber = LittleLong(fogs->brushNum);

		// global fog has a brush number of -1, and no visible side
		if (out->originalBrushNumber == -1)
		{
			VectorSet(out->bounds[0], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);
			VectorSet(out->bounds[1], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
			firstSide = 0;
		}
		else
		{
			if ((unsigned)out->originalBrushNumber >= brushesCount)
			{
				Ren_Drop("fog brushNumber out of range");
			}
			brush = brushes + out->originalBrushNumber;

			firstSide = LittleLong(brush->firstSide);

			if ((unsigned)firstSide > sidesCount - 6)
			{
				Ren_Drop("fog brush sideNumber out of range");
			}

			// find which bsp submodel the fog volume belongs to
			for (j = 0; j < s_worldData.numModels; j++)
			{
				if (out->originalBrushNumber >= s_worldData.models[j].firstBrush &&
				    out->originalBrushNumber < (s_worldData.models[j].firstBrush + s_worldData.models[j].numBrushes))
				{
					out->modelNum = j;
					break;
				}
			}

			// brushes are always sorted with the axial sides first
			sideNum           = firstSide + 0;
			planeNum          = LittleLong(sides[sideNum].planeNum);
			out->bounds[0][0] = -s_worldData.planes[planeNum].dist;

			sideNum           = firstSide + 1;
			planeNum          = LittleLong(sides[sideNum].planeNum);
			out->bounds[1][0] = s_worldData.planes[planeNum].dist;

			sideNum           = firstSide + 2;
			planeNum          = LittleLong(sides[sideNum].planeNum);
			out->bounds[0][1] = -s_worldData.planes[planeNum].dist;

			sideNum           = firstSide + 3;
			planeNum          = LittleLong(sides[sideNum].planeNum);
			out->bounds[1][1] = s_worldData.planes[planeNum].dist;

			sideNum           = firstSide + 4;
			planeNum          = LittleLong(sides[sideNum].planeNum);
			out->bounds[0][2] = -s_worldData.planes[planeNum].dist;

			sideNum           = firstSide + 5;
			planeNum          = LittleLong(sides[sideNum].planeNum);
			out->bounds[1][2] = s_worldData.planes[planeNum].dist;
		}

		// get information from the shader for fog parameters
		shader = R_FindShader(fogs->shader, SHADER_3D_DYNAMIC, qtrue);

		out->fogParms = shader->fogParms;

		out->color[0] = shader->fogParms.color[0] * tr.identityLight;
		out->color[1] = shader->fogParms.color[1] * tr.identityLight;
		out->color[2] = shader->fogParms.color[2] * tr.identityLight;
		out->color[3] = 1;

		d            = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / (d * 8);

		// global fog sets clearcolor/zfar
		if (out->originalBrushNumber == -1)
		{
			s_worldData.globalFog = i + 1;
			VectorCopy(shader->fogParms.color, s_worldData.globalOriginalFog);
			s_worldData.globalOriginalFog[3] = shader->fogParms.depthForOpaque;
		}

		// set the gradient vector
		sideNum = LittleLong(fogs->visibleSide);

		// made this check a little more strenuous (was sideNum == -1)
		if (sideNum < 0 || sideNum >= sidesCount)
		{
			out->hasSurface = qfalse;
		}
		else
		{
			out->hasSurface = qtrue;
			planeNum        = LittleLong(sides[firstSide + sideNum].planeNum);
			VectorSubtract(vec3_origin, s_worldData.planes[planeNum].normal, out->surface);
			out->surface[3] = -s_worldData.planes[planeNum].dist;
		}

		out++;
	}

	Ren_Developer("%i fog volumes loaded\n", s_worldData.numFogs);
}

/**
 * @brief R_LoadLightGrid
 * @param[in] l
 */
void R_LoadLightGrid(lump_t *l)
{
	int            i, j, k;
	vec3_t         maxs;
	world_t        *w = &s_worldData;
	float          *wMins, *wMaxs;
	dgridPoint_t   *in;
	bspGridPoint_t *gridPoint;
	float          lat, lng;
	int            gridStep[3];
	int            pos[3];
	float          posFloat[3];
	byte           tmpAmbient[4], tmpDirected[4];

	Ren_Print("...loading light grid\n");

	w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];

	wMins = w->models[0].bounds[0];
	wMaxs = w->models[0].bounds[1];

	for (i = 0; i < 3; i++)
	{
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil(wMins[i] / w->lightGridSize[i]);
		maxs[i]               = w->lightGridSize[i] * floor(wMaxs[i] / w->lightGridSize[i]);
		w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i]) / w->lightGridSize[i] + 1;
	}

	w->numLightGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	Ren_Developer("grid size (%i %i %i)\n", (int)w->lightGridSize[0], (int)w->lightGridSize[1],
	              (int)w->lightGridSize[2]);
	Ren_Developer("grid bounds (%i %i %i)\n", (int)w->lightGridBounds[0], (int)w->lightGridBounds[1],
	              (int)w->lightGridBounds[2]);

	if (l->filelen != w->numLightGridPoints * sizeof(dgridPoint_t))
	{
		Ren_Warning("WARNING: light grid mismatch\n");
		w->lightGridData = NULL;
		return;
	}

	in = (dgridPoint_t *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Ren_Drop("LoadMap: funny lump size in %s", s_worldData.name);
	}
	gridPoint = (bspGridPoint_t *)ri.Hunk_Alloc(w->numLightGridPoints * sizeof(*gridPoint), h_low);

	w->lightGridData = gridPoint;
	//Com_Memcpy(w->lightGridData, (void *)(fileBase + l->fileofs), l->filelen);

	for (i = 0; i < w->numLightGridPoints; i++, in++, gridPoint++)
	{
		tmpAmbient[0] = in->ambient[0];
		tmpAmbient[1] = in->ambient[1];
		tmpAmbient[2] = in->ambient[2];
		tmpAmbient[3] = 255;

		tmpDirected[0] = in->directed[0];
		tmpDirected[1] = in->directed[1];
		tmpDirected[2] = in->directed[2];
		tmpDirected[3] = 255;

		R_ColorShiftLightingBytes(tmpAmbient, tmpAmbient);
		R_ColorShiftLightingBytes(tmpDirected, tmpDirected);

		for (j = 0; j < 3; j++)
		{
			gridPoint->ambientColor[j]  = tmpAmbient[j] / 255.0f;
			gridPoint->directedColor[j] = tmpDirected[j] / 255.0f;
		}

		gridPoint->ambientColor[3]  = 1.0f;
		gridPoint->directedColor[3] = 1.0f;

		// standard spherical coordinates to cartesian coordinates conversion

		// decode X as cos( lat ) * sin( long )
		// decode Y as sin( lat ) * sin( long )
		// decode Z as cos( long )

		// Lat = 0 at (1,0,0) to 360 (-1,0,0), encoded in 8-bit sine table format
		// Lng = 0 at (0,0,1) to 180 (0,0,-1), encoded in 8-bit sine table format <--- 180°?!

		if (in->latLong[0] == 0x80 && in->latLong[1] == 0x00)
		{
			 gridPoint->direction[0] = 0.0f;
			 gridPoint->direction[1] = 0.0f;
			 gridPoint->direction[2] = -1.0f;
		}
		else
		{
			lat = DEG2RAD(in->latLong[1] * (360.0 / 256.0)); // calculate with 256 instead of 255 to hit exact PI
			lng = DEG2RAD(in->latLong[0] * (360.0 / 256.0));

			gridPoint->direction[0] = cos(lat) * sin(lng);
			gridPoint->direction[1] = sin(lat) * sin(lng);
			gridPoint->direction[2] = cos(lng);
		}

		//Ren_Print(" %i %i %f %f\n" , in->latLong[1], in->latLong[0], lat , lng);

		// debug print to see if the XBSP format is correct
		//Ren_Print("%9d Amb: (%03.1f %03.1f %03.1f) Dir: (%03.1f %03.1f %03.1f)\n",
		//  i, gridPoint->ambientColor[0], gridPoint->ambientColor[1], gridPoint->ambientColor[2], gridPoint->direction[0], gridPoint->direction[1], gridPoint->direction[2]);
	}

	// calculate grid point positions
	gridStep[0] = 1;
	gridStep[1] = w->lightGridBounds[0];
	gridStep[2] = w->lightGridBounds[0] * w->lightGridBounds[1];

	for (i = 0; i < w->lightGridBounds[0]; i += 1)
	{
		for (j = 0; j < w->lightGridBounds[1]; j += 1)
		{
			for (k = 0; k < w->lightGridBounds[2]; k += 1)
			{
				pos[0] = i;
				pos[1] = j;
				pos[2] = k;

				posFloat[0] = i * w->lightGridSize[0];
				posFloat[1] = j * w->lightGridSize[1];
				posFloat[2] = k * w->lightGridSize[2];

				gridPoint = w->lightGridData + pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];

				VectorAdd(posFloat, w->lightGridOrigin, gridPoint->origin);
			}
		}
	}

	Ren_Developer("%i light grid points created\n", w->numLightGridPoints);
}

/**
 * @brief Parses all ents again and sets origin from given targetname null_info ent
 */
qboolean setProjTargetOrigin(char *lightDefs, char *targetname, trRefLight_t *light)
{
	char     *p = lightDefs, *token;
	char     keyname[MAX_TOKEN_CHARS];
	char     value[MAX_TOKEN_CHARS];
	qboolean isInfoNull, isRequestedTarget;
	char     *origin;

	if (!targetname || !targetname[0])
	{
		Ren_Warning("setProjTargetOrigin WARNING: no target set!\n");
		return qfalse;
	}

	//Ren_Print("searching for targetname %s\n", targetname);

	// count lights
	while (1)
	{
		// parse {
		token = COM_ParseExt2(&p, qtrue);

		if (!*token)
		{
			// end of entities string
			break;
		}

		if (*token != '{')
		{
			Ren_Warning("setProjTargetOrigin WARNING: expected { found '%s'\n", token);
			break;
		}

		isInfoNull        = qfalse;
		isRequestedTarget = qfalse;
		origin            = NULL;

		// parse epairs
		while (1)
		{
			// parse key
			token = COM_ParseExt2(&p, qtrue);

			if (*token == '}')
			{
				break;
			}

			if (!*token)
			{
				Ren_Warning("setProjTargetOrigin WARNING: EOF without closing bracket\n");
				break;
			}

			Q_strncpyz(keyname, token, sizeof(keyname));

			// parse value
			token = COM_ParseExt2(&p, qfalse);

			if (!*token)
			{
				Ren_Warning("setProjTargetOrigin WARNING: missing value for key '%s'\n", keyname);
				continue;
			}

			Q_strncpyz(value, token, sizeof(value));

			// check if this entity is a light
			// light related ents are
			// light, lightJunior, dlight, corona, info_null (spotlights)
			// see also misc_light_surface, misc_spotlight
			//if (!Q_stricmp(keyname, "classname") && (!Q_stricmp(value, "light") || !Q_stricmp(value, "lightJunior") || !Q_stricmp(value, "dlight")))
			if (!Q_stricmp(keyname, "classname") && (!Q_stricmp(value, "info_null")))
			{
				isInfoNull = qtrue;
			}

			// note: per definition lightJunior never has a target
			if (!Q_stricmp(keyname, "targetname") && !Q_stricmp(value, targetname))
			{
				isRequestedTarget = qtrue;
			}

			if (!Q_stricmp(keyname, "origin"))
			{
				origin =  ri.Z_Malloc(strlen(value) + 1);
				strcpy(origin, value);
			}
		}

		if (*token != '}')
		{
			Ren_Warning("setProjTargetOrigin WARNING: expected } found '%s'\n", token);

			if (origin)
			{
				ri.Free(origin);
			}

			break;
		}

		if (isInfoNull && isRequestedTarget)
		{
			if (origin)
			{
				sscanf(value, "%f %f %f", &light->l.projTarget[0], &light->l.projTarget[1], &light->l.projTarget[2]);
				ri.Free(origin);
			}
			else
			{
				Ren_Warning("setProjTargetOrigin WARNING: requested target '%s' has no origin\n", targetname);
			}

			return qtrue; // we are done
		}

		if (origin)
		{
			ri.Free(origin);
		}
	}

	return qfalse;
}

// debug
#define RADIUS_MULTIPLICATOR 1


/**
 * @brief Resets a static light to default values
 * @param[in]
 */
void resetRefLight(trRefLight_t *light)
{
	QuatClear(light->l.rotation); // reset rotation because it may be set to the rotation of other entities
	VectorClear(light->l.center);

	light->l.color[0] = 1;
	light->l.color[1] = 1;
	light->l.color[2] = 1;

	light->l.scale = r_lightScale->value;

	light->l.radius[0] = 64 * RADIUS_MULTIPLICATOR; // default radius 64
	light->l.radius[1] = 64 * RADIUS_MULTIPLICATOR;
	light->l.radius[2] = 64 * RADIUS_MULTIPLICATOR;

	VectorClear(light->l.projTarget);
	VectorClear(light->l.projRight);
	VectorClear(light->l.projUp);
	VectorClear(light->l.projStart);
	VectorClear(light->l.projEnd);

	light->l.inverseShadows = qfalse;

	light->isStatic    = qtrue;
	light->noRadiosity = qfalse;
	light->additive    = qtrue;

	light->shadowLOD = 0;

	light->l.rlType  = RL_OMNI;
}

/**
 * @brief This is extracting classname light and lightJunior entities from bsp into s_worldData.lights
 *
 * @param[in] lightDefs
 *
 * FIXME: check parser for missing keys
 * FIXME: Inspect lightJunior (is there a need to assign these to models?)
 * FIXME: spotlights targets
 * FIXME: sun spotlight
 *
 * FIXME: this might be optimized by more preparsing (f.e. store info_null ents for spotlights at the first time
 *        of parsing (we do a full parse of ent def string twice in R_LoadLights and for each target in setProjTarget ...)
 *
 * classname light:
 *
 * fade
 * light
 * radius
 * target
 * targetname
 * _anglescale
 * - For scaling angle attenuation. Use a small value (< 1.0) to lessen the angle attenuation, and a high value (> 1.0) for sharper, more faceted lighting.
 * _color
 * _deviance
 * - Radius within which additional samples will be placed.
 * _filter;_filterradius
 * - "_filterradius" "32" -- will filter lightmaps created by this light by 32 world units
 * _samples
 * - Makes Q3map2 replace the light with several smaller lights for smoother illumination. Values of 4 or so will be adequate.(where "#" is distance in world units for point/spot lights and degrees for suns)
 * _sun
 * angle?!
 *
 * {
 * "target" "t514"
 * "radius" "128"
 * "angle" "0"
 * "_color" "1.000000 0.690196 0.384314"
 * "light" "100"
 * "origin" "-2340 3234 1426"
 * "fade" ".9"
 * "classname" "light"
 * }
 *
 *
 * classname lightJunior:
 *
 * fade
 * light
 * radius
 *
 */
void R_LoadLights(char *lightDefs)
{
	//char *s;
	char         *p = lightDefs, *token;
	char         keyname[MAX_TOKEN_CHARS];
	char         value[MAX_TOKEN_CHARS];
	qboolean     isLight, hasInfoNull;
	int          numEntities       = 1; // parsed worldspawn so far
	int          numLights         = 0;
	int          numOmniLights     = 0;
	int          numProjLights     = 0;
	int          numParallelLights = 0;
	trRefLight_t *light;
	int          i           = 0;
	int          numInfoNull = 0;
	char         *target;

	// count lights
	while (1)
	{
		// parse {
		token = COM_ParseExt2(&p, qtrue);

		if (!*token)
		{
			// end of entities string
			break;
		}

		if (*token != '{')
		{
			Ren_Warning("R_LoadLights WARNING: expected { found '%s'\n", token);
			break;
		}

		// new entity
		isLight     = qfalse;
		hasInfoNull = qfalse;

		// parse epairs
		while (1)
		{
			// parse key
			token = COM_ParseExt2(&p, qtrue);

			if (*token == '}')
			{
				break;
			}

			if (!*token)
			{
				Ren_Warning("R_LoadLights WARNING: EOF without closing bracket\n");
				break;
			}

			Q_strncpyz(keyname, token, sizeof(keyname));

			// parse value
			token = COM_ParseExt2(&p, qfalse);

			if (!*token)
			{
				Ren_Warning("R_LoadLights WARNING: missing value for key '%s'\n", keyname);
				continue;
			}

			Q_strncpyz(value, token, sizeof(value));

			// check if this entity is a light
			// light related ents are
			// light, lightJunior, dlight, corona, info_null (spotlights)
			// see also misc_light_surface, misc_spotlight
			//if (!Q_stricmp(keyname, "classname") && (!Q_stricmp(value, "light") || !Q_stricmp(value, "lightJunior") || !Q_stricmp(value, "dlight")))
			if (!Q_stricmp(keyname, "classname") && (!Q_stricmp(value, "light") || !Q_stricmp(value, "lightJunior")))
			{
				isLight = qtrue;
			}

			// note: per definition lightJunior never has a target
			if (!Q_stricmp(keyname, "target")  || !Q_stricmp(keyname, "targetname"))
			{
				hasInfoNull = qtrue;
			}
		}

		if (*token != '}')
		{
			Ren_Warning("R_LoadLights WARNING: expected } found '%s'\n", token);
			break;
		}

		if (isLight)
		{
			numLights++;
		}

		if (isLight && hasInfoNull)
		{
			numInfoNull++;
		}

		numEntities++;
	}

	Ren_Developer("%i total entities counted\n", numEntities);
	Ren_Print("%i total lights and %i light targets counted\n", numLights, numInfoNull);

	s_worldData.numLights = numLights;

	if (!numLights)
	{
		s_worldData.lights = NULL;
		return;
	}

	// FIXME add 1 dummy light so we don't trash the hunk memory system ...
	s_worldData.lights = (trRefLight_t *)ri.Hunk_Alloc((s_worldData.numLights + 1) * sizeof(trRefLight_t), h_low);

	// basic light setup
	for (i = 0, light = s_worldData.lights; i < s_worldData.numLights; i++, light++)
	{
		resetRefLight(light);
	}

	// parse lights
	p           = lightDefs;
	numEntities = 1; // why 1? - worldspawn?
	light       = &s_worldData.lights[0];

	while (1)
	{
		// parse {
		token = COM_ParseExt2(&p, qtrue);

		if (!*token)
		{
			// end of entities string
			break;
		}

		if (*token != '{')
		{
			Ren_Warning("R_LoadLights WARNING: expected { found '%s'\n", token);
			break;
		}

		// new entity
		isLight = qfalse;
		target  = NULL;

		// parse epairs
		while (1)
		{
			// parse key
			token = COM_ParseExt2(&p, qtrue);

			if (*token == '}')
			{
				break;
			}

			if (!*token)
			{
				Ren_Warning("R_LoadLights WARNING: EOF without closing bracket\n");
				break;
			}

			Q_strncpyz(keyname, token, sizeof(keyname));

			// parse value
			token = COM_ParseExt2(&p, qfalse);

			if (!*token)
			{
				Ren_Warning("R_LoadLights WARNING: missing value for key '%s'\n", keyname);
				continue;
			}

			Q_strncpyz(value, token, sizeof(value));

			// check if this entity is a light
			//if (!Q_stricmp(keyname, "classname") && (!Q_stricmp(value, "light") || !Q_stricmp(value, "lightJunior") || !Q_stricmp(value, "dlight")))
			// Lights pointed at a target will be spotlights.(Some options will create a Q3map2 light shader for your map"
			if (!Q_stricmp(keyname, "classname") && !Q_stricmp(value, "light"))
			{
				isLight = qtrue;
			}
			// Light that only affects dynamic game models, but does not contribute to lightmaps.
			// Lights pointed at a target will be spotlights. (Some options will create a Q3map2 light shader for your map.)
			if (!Q_stricmp(keyname, "classname") && !Q_stricmp(value, "lightJunior"))
			{
				isLight = qtrue;
			}
			// check for origin
			else if (!Q_stricmp(keyname, "origin") || !Q_stricmp(keyname, "light_origin")) // ETL (origin)
			{
				sscanf(value, "%f %f %f", &light->l.origin[0], &light->l.origin[1], &light->l.origin[2]);
			}
			// check for origin
			else if (!Q_stricmp(keyname, "spawnflags")) // ETL (spawnflags)
			{
				// FIXME: light: ANGLE, NONLINEAR, Q3MAP_NON-DYNAMIC
				// FIXME: lightJunior: ANGLE, NEGATIVE_POINT, NEGATIVE_SPOT, NONLINEAR
				int spawnflags = Q_atoi(value);
			}
			else if (!Q_stricmp(keyname, "angle")) // ETL (angle)
			{
				 // FIXME
				int angle = Q_atoi(value);
			}
			// Falloff/radius adjustment value. Multiply the run of the slope by "fade" (1.0f default only valid for "Linear" lights wolf)
			else if (!Q_stricmp(keyname, "fade")) // ETL (fade)
			{
				 // FIXME
				float fade = Q_atoi(value);
			}
			// check for center
			else if (!Q_stricmp(keyname, "light_center"))
			{
				sscanf(value, "%f %f %f", &light->l.center[0], &light->l.center[1], &light->l.center[2]);
			}
			// check for color - weighted RGB value of light color ('k' key)(default white - 1.0 1.0 1.0)
			else if (!Q_stricmp(keyname, "_color")) // ETL
			{
				sscanf(value, "%f %f %f", &light->l.color[0], &light->l.color[1], &light->l.color[2]);
			}
			// check for radius - overrides the default 64 unit radius of a spotlight at the target point
			else if (!Q_stricmp(keyname, "light_radius") || !Q_stricmp(keyname, "radius")) // ETL (radius)
			{
				sscanf(value, "%f %f %f", &light->l.radius[0], &light->l.radius[1], &light->l.radius[2]);

				//VectorScale();
				light->l.radius[0] *= RADIUS_MULTIPLICATOR;
				light->l.radius[1] *= RADIUS_MULTIPLICATOR;
				light->l.radius[2] *= RADIUS_MULTIPLICATOR;
			}
			// check for light_target
			else if (!Q_stricmp(keyname, "light_target"))
			{
				sscanf(value, "%f %f %f", &light->l.projTarget[0], &light->l.projTarget[1], &light->l.projTarget[2]);

				light->l.rlType = RL_PROJ;
			}
			// lights pointed at a target will be spotlights
			else if (!Q_stricmp(keyname, "target")) // ETL
			{
				target = ri.Z_Malloc(strlen(value) + 1);
				strcpy(target, value);

				//light->l.rlType = RL_PROJ;
			}
			else if (!Q_stricmp(keyname, "targetname")) // ETL
			{
				// see target, this is just duplicate in bsp
			}
			// set this key to 1 on a spotlight to make an infinite sun light
			else if (!Q_stricmp(keyname, "_sun")) // ETL
			{
				// FIXME: inspect (has to be set on spotlight ..., target?!)
				// is this info exported? no sun/moon on radar/goldrush ...
				//light->l.rlType = RL_DIRECTIONAL;
			}
			// check for light_right
			else if (!Q_stricmp(keyname, "light_right"))
			{
				sscanf(value, "%f %f %f", &light->l.projRight[0], &light->l.projRight[1], &light->l.projRight[2]);

				light->l.rlType = RL_PROJ;
			}
			// check for light_up
			else if (!Q_stricmp(keyname, "light_up"))
			{
				sscanf(value, "%f %f %f", &light->l.projUp[0], &light->l.projUp[1], &light->l.projUp[2]);

				light->l.rlType = RL_PROJ;
			}
			// check for light_start
			else if (!Q_stricmp(keyname, "light_start"))
			{
				sscanf(value, "%f %f %f", &light->l.projStart[0], &light->l.projStart[1], &light->l.projStart[2]);

				light->l.rlType = RL_PROJ;
			}
			// check for light_end
			else if (!Q_stricmp(keyname, "light_end"))
			{
				sscanf(value, "%f %f %f", &light->l.projEnd[0], &light->l.projEnd[1], &light->l.projEnd[2]);

				light->l.rlType = RL_PROJ;
			}
			// Overrides the default 300 intensity
			// fixed: xreal did set radius ...
			else if (!Q_stricmp(keyname, "light") || !Q_stricmp(keyname, "_light")) // ETL (light)
			{
				// 300 is 100% = default intensity
				light->l.scale = atof(value) / 300.f * r_lightScale->value;

				if (light->l.scale < 0)
				{
					light->l.scale = r_lightScale->value;
				}

				if (!r_hdrRendering->integer || !glConfig2.textureFloatAvailable || !glConfig2.framebufferObjectAvailable || !glConfig2.framebufferBlitAvailable)
				{
					if (light->l.scale > r_lightScale->value)
					{
						light->l.scale = r_lightScale->value;
					}
				}
			}
			// check for scale
			else if (!Q_stricmp(keyname, "light_scale"))
			{
				light->l.scale = atof(value);

				if (!r_hdrRendering->integer || !glConfig2.textureFloatAvailable || !glConfig2.framebufferObjectAvailable || !glConfig2.framebufferBlitAvailable)
				{
					if (light->l.scale > r_lightScale->value)
					{
						light->l.scale = r_lightScale->value;
					}
				}
			}
			// check for light shader
			else if (!Q_stricmp(keyname, "texture"))
			{
				light->l.attenuationShader = RE_RegisterShaderLightAttenuation(value);
			}
			// check for rotation
			else if (!Q_stricmp(keyname, "rotation") || !Q_stricmp(keyname, "light_rotation"))
			{
				mat4_t rotation;

				sscanf(value, "%f %f %f %f %f %f %f %f %f", &rotation[0], &rotation[1], &rotation[2],
				       &rotation[4], &rotation[5], &rotation[6], &rotation[8], &rotation[9], &rotation[10]);

				quat_from_mat4(light->l.rotation, rotation);
			}
			// check if this light does not cast any shadows
			else if (!Q_stricmp(keyname, "noshadows") && !Q_stricmp(value, "1"))
			{
				light->l.noShadows = qtrue;
			}
			// check if this light does not contribute to the global lightmapping
			else if (!Q_stricmp(keyname, "noradiosity") && !Q_stricmp(value, "1"))
			{
				light->noRadiosity = qtrue;
			}
			// check if this light is a parallel sun light
			else if (!Q_stricmp(keyname, "parallel") && !Q_stricmp(value, "1"))
			{
				light->l.rlType = RL_DIRECTIONAL;
			}
		}

		if (*token != '}')
		{
			Ren_Warning("R_LoadLights WARNING: expected } found '%s'\n", token);

			if (target)
			{
				ri.Free(target);
			}

			break;
		}

		if (!isLight)
		{
			resetRefLight(light);
		}
		else
		{
			if ((numOmniLights + numProjLights + numParallelLights) < s_worldData.numLights)
			{
				switch (light->l.rlType)
				{
				case RL_OMNI:
					if (target) // might have a target
					{
						if (!setProjTargetOrigin(lightDefs, target, light))
						{
							// goldrush throws this because of missing info_null 't713'
							Ren_Warning("R_LoadLights WARNING: no projection target found for %s\n", target);
						}
						else
						{
							Ren_Developer("R_LoadLights target for %s\n", target);
						}
					}
					numOmniLights++;
					break;
				case RL_PROJ: // must have a target
					if (!setProjTargetOrigin(lightDefs, target, light))
					{
						// goldrush throws this because of missing info_null 't713'
						Ren_Warning("R_LoadLights WARNING: no projection target found for %s\n", target);
					}
					numProjLights++;
					break;
				case RL_DIRECTIONAL:
					numParallelLights++;
					break;
				default:
					break;
				}

				light++;
			}
		}

		if (target)
		{
			ri.Free(target);
		}

		numEntities++;
	}

	if ((numOmniLights + numProjLights + numParallelLights) != s_worldData.numLights)
	{
		Ren_Drop("counted %i lights and parsed %i lights", s_worldData.numLights, (numOmniLights + numProjLights + numParallelLights));
	}

	Ren_Developer("%i total entities parsed\n", numEntities);
	Ren_Developer("%i total lights parsed\n", numOmniLights + numProjLights + numParallelLights);
	Ren_Print("%i omni-directional lights parsed\n", numOmniLights);
	Ren_Print("%i projective lights parsed\n", numProjLights);
	Ren_Print("%i directional lights parsed\n", numParallelLights);
}

/**
 * @brief R_LoadEntities
 * @param[in] l
 */
void R_LoadEntities(lump_t *l)
{
	char    *p, *token, *s;
	char    keyname[MAX_TOKEN_CHARS];
	char    value[MAX_TOKEN_CHARS];
	world_t *w = &s_worldData;

	Ren_Print("...loading entities\n");

	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	// store for reference by the cgame
	w->entityString = (char *)ri.Hunk_Alloc(l->filelen + 1, h_low);
	//strcpy(w->entityString, (char *)(fileBase + l->fileofs));
	Q_strncpyz(w->entityString, (char *)(fileBase + l->fileofs), l->filelen + 1);
	w->entityParsePoint = w->entityString;

#if 1
	p = w->entityString;
#else
	p = (char *)(fileBase + l->fileofs);
#endif

	// only parse the world spawn
	while (1)
	{
		// parse key
		token = COM_ParseExt2(&p, qtrue);

		if (!*token)
		{
			Ren_Warning("R_LoadEntities WARNING: unexpected end of entities string while parsing worldspawn\n");
			break;
		}

		if (*token == '{')
		{
			continue;
		}

		if (*token == '}')
		{
			break;
		}

		Q_strncpyz(keyname, token, sizeof(keyname));

		// parse value
		token = COM_ParseExt2(&p, qfalse);

		if (!*token)
		{
			continue;
		}

		Q_strncpyz(value, token, sizeof(value));

		// check for remapping of shaders for vertex lighting
		s = "vertexremapshader";
		if (!Q_strncmp(keyname, s, strlen(s)))
		{
			s = strchr(value, ';');
			if (!s)
			{
				Ren_Warning("WARNING: no semi colon in vertexshaderremap '%s'\n", value);
				break;
			}
			*s++ = 0;

			//if (r_vertexLight->integer)
			//{
			//R_RemapShader(value, s, "0");
			//}
			continue;
		}

		// check for remapping of shaders
		s = "remapshader";
		if (!Q_strncmp(keyname, s, strlen(s)))
		{
			s = strchr(value, ';');
			if (!s)
			{
				Ren_Warning("WARNING: no semi colon in shaderremap '%s'\n", value);
				break;
			}
			*s++ = 0;
			R_RemapShader(value, s, "0");
			continue;
		}

		// check for a different grid size
		if (!Q_stricmp(keyname, "gridsize"))
		{
			sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2]);
			continue;
		}
		// check for ambient color
		else if (!Q_stricmp(keyname, "_color") || !Q_stricmp(keyname, "color"))
		{


				sscanf(value, "%f %f %f", &tr.worldEntity.ambientLight[0], &tr.worldEntity.ambientLight[1],
				       &tr.worldEntity.ambientLight[2]);

		}
		// check for ambient scale constant
		else if (!Q_stricmp(keyname, "ambientColor") || !Q_stricmp(keyname, "ambient") || !Q_stricmp(keyname, "_ambient"))
		{
			// FIXME: bypass r_ambientScale in R_SetupEntityLighting so map maker can set ambient scale for entities?
			// ? = atof(value);
		}
		// check for fog color
		else if (!Q_stricmp(keyname, "fogColor"))
		{
			sscanf(value, "%f %f %f", &tr.fogColor[0], &tr.fogColor[1], &tr.fogColor[2]);
		}
		// check for fog density
		else if (!Q_stricmp(keyname, "fogDensity"))
		{
			tr.fogDensity = atof(value);
		}

		// check for deluxe mapping support
		/*if (!Q_stricmp(keyname, "deluxeMapping") && !Q_stricmp(value, "1"))
		{
			Ren_Developer("map features directional light mapping\n");
			tr.worldDeluxeMapping = qtrue;
			continue;
		}*/
		// check for mapOverBrightBits override
		else if (!Q_stricmp(keyname, "mapOverBrightBits"))
		{
			tr.mapOverBrightBits = Q_bound(0, atof(value), 3);
		}

		// check for deluxe mapping provided by NetRadiant's q3map2
		/*if (!Q_stricmp(keyname, "_q3map2_cmdline"))
		{
			s = strstr(value, "-deluxe");
			if (s)
			{
				Ren_Developer("map features directional light mapping\n");
				tr.worldDeluxeMapping = qtrue;
			}
			continue;
		}*/


		// check for HDR light mapping support
		if (!Q_stricmp(keyname, "hdrRGBE") && !Q_stricmp(value, "1"))
		{
			Ren_Developer("map features HDR light mapping\n");
			tr.worldHDR_RGBE = qtrue;
			continue;
		}

		if (!Q_stricmp(keyname, "classname") && Q_stricmp(value, "worldspawn"))
		{
			Ren_Warning("WARNING: unexpected worldspawn found '%s'\n", value);
			continue;
		}
	}

	//Ren_Print("-----------\n%s\n----------\n", p);

	R_LoadLights(p);
}

/**
 * @brief R_GetEntityToken
 * @param[out] buffer
 * @param[in] size
 * @return
 */
qboolean R_GetEntityToken(char *buffer, size_t size)
{
	const char *s;

	s = COM_Parse2(&s_worldData.entityParsePoint);
	Q_strncpyz(buffer, s, size);
	if (!s_worldData.entityParsePoint || !s[0])
	{
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	}
	else
	{
		return qtrue;
	}
}

/**
 * @brief R_PrecacheInteraction
 * @param[in,out] light
 * @param[in] surface
 */
static void R_PrecacheInteraction(trRefLight_t *light, bspSurface_t *surface)
{
	interactionCache_t *iaCache;

	iaCache = (interactionCache_t *)ri.Hunk_Alloc(sizeof(*iaCache), h_low);
	Com_AddToGrowList(&s_interactions, iaCache);

	// connect to interaction grid
	if (!light->firstInteractionCache)
	{
		light->firstInteractionCache = iaCache;
	}

	if (light->lastInteractionCache)
	{
		light->lastInteractionCache->next = iaCache;
	}

	light->lastInteractionCache = iaCache;

	iaCache->next    = NULL;
	iaCache->surface = surface;

	iaCache->redundant = qfalse;
}

/*
 * @brief R_BuildShadowVolume
 * @param[in] numTriangles
 * @param[in] triangles
 * @param[in] numVerts
 * @param[out] indexes
 * @return
 *
 * @note Unused
static int R_BuildShadowVolume(int numTriangles, const srfTriangle_t * triangles, int numVerts, int indexes[SHADER_MAX_INDEXES])
{
    int             i;
    int             numIndexes;
    const srfTriangle_t *tri;

    // calculate zfail shadow volume
    numIndexes = 0;

    // set up indices for silhouette edges
    for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
    {
        if(!sh.facing[i])
        {
            continue;
        }

        if(tri->neighbors[0] < 0 || !sh.facing[tri->neighbors[0]])
        {
            indexes[numIndexes + 0] = tri->indexes[1];
            indexes[numIndexes + 1] = tri->indexes[0];
            indexes[numIndexes + 2] = tri->indexes[0] + numVerts;

            indexes[numIndexes + 3] = tri->indexes[1];
            indexes[numIndexes + 4] = tri->indexes[0] + numVerts;
            indexes[numIndexes + 5] = tri->indexes[1] + numVerts;

            numIndexes += 6;
        }

        if(tri->neighbors[1] < 0 || !sh.facing[tri->neighbors[1]])
        {
            indexes[numIndexes + 0] = tri->indexes[2];
            indexes[numIndexes + 1] = tri->indexes[1];
            indexes[numIndexes + 2] = tri->indexes[1] + numVerts;

            indexes[numIndexes + 3] = tri->indexes[2];
            indexes[numIndexes + 4] = tri->indexes[1] + numVerts;
            indexes[numIndexes + 5] = tri->indexes[2] + numVerts;

            numIndexes += 6;
        }

        if(tri->neighbors[2] < 0 || !sh.facing[tri->neighbors[2]])
        {
            indexes[numIndexes + 0] = tri->indexes[0];
            indexes[numIndexes + 1] = tri->indexes[2];
            indexes[numIndexes + 2] = tri->indexes[2] + numVerts;

            indexes[numIndexes + 3] = tri->indexes[0];
            indexes[numIndexes + 4] = tri->indexes[2] + numVerts;
            indexes[numIndexes + 5] = tri->indexes[0] + numVerts;

            numIndexes += 6;
        }
    }

    // set up indices for light and dark caps
    for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
    {
        if(!sh.facing[i])
        {
            continue;
        }

        // light cap
        indexes[numIndexes + 0] = tri->indexes[0];
        indexes[numIndexes + 1] = tri->indexes[1];
        indexes[numIndexes + 2] = tri->indexes[2];

        // dark cap
        indexes[numIndexes + 3] = tri->indexes[2] + numVerts;
        indexes[numIndexes + 4] = tri->indexes[1] + numVerts;
        indexes[numIndexes + 5] = tri->indexes[0] + numVerts;

        numIndexes += 6;
    }

    return numIndexes;
}
*/

/*
 * @brief R_BuildShadowPlanes
 * @param[in] numTriangles
 * @param[in] triangles
 * @param[in] numVerts
 * @param[in] verts
 * @param[out] shadowPlanes
 * @param[in] light
 * @return
 *
 * @note Unused
static int R_BuildShadowPlanes(int numTriangles, const srfTriangle_t * triangles, int numVerts, srfVert_t * verts,
                               cplane_t shadowPlanes[SHADER_MAX_TRIANGLES], trRefLight_t * light)
{
    int             i;
    int             numShadowPlanes;
    const srfTriangle_t *tri;
    vec3_t          pos[3];

//  vec3_t          lightDir;
    vec4_t          plane;

    if(r_noShadowFrustums->integer)
    {
        return 0;
    }

    // calculate shadow frustum
    numShadowPlanes = 0;

    // set up indices for silhouette edges
    for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
    {
        if(!sh.facing[i])
        {
            continue;
        }

        if(tri->neighbors[0] < 0 || !sh.facing[tri->neighbors[0]])
        {
            //indexes[numIndexes + 0] = tri->indexes[1];
            //indexes[numIndexes + 1] = tri->indexes[0];
            //indexes[numIndexes + 2] = tri->indexes[0] + numVerts;

            VectorCopy(verts[tri->indexes[1]].xyz, pos[0]);
            VectorCopy(verts[tri->indexes[0]].xyz, pos[1]);
            VectorCopy(light->origin, pos[2]);

            // extrude the infinite one
            //VectorSubtract(verts[tri->indexes[0]].xyz, light->origin, lightDir);
            //VectorAdd(verts[tri->indexes[0]].xyz, lightDir, pos[2]);
            //VectorNormalize(lightDir);
            //VectorMA(verts[tri->indexes[0]].xyz, 9999, lightDir, pos[2]);

            if(PlaneFromPoints(plane, pos[0], pos[1], pos[2]))
            {
                shadowPlanes[numShadowPlanes].normal[0] = plane[0];
                shadowPlanes[numShadowPlanes].normal[1] = plane[1];
                shadowPlanes[numShadowPlanes].normal[2] = plane[2];
                shadowPlanes[numShadowPlanes].dist = plane[3];

                numShadowPlanes++;
            }
            else
            {
                return 0;
            }
        }

        if(tri->neighbors[1] < 0 || !sh.facing[tri->neighbors[1]])
        {
            //indexes[numIndexes + 0] = tri->indexes[2];
            //indexes[numIndexes + 1] = tri->indexes[1];
            //indexes[numIndexes + 2] = tri->indexes[1] + numVerts;

            VectorCopy(verts[tri->indexes[2]].xyz, pos[0]);
            VectorCopy(verts[tri->indexes[1]].xyz, pos[1]);
            VectorCopy(light->origin, pos[2]);

            // extrude the infinite one
            //VectorSubtract(verts[tri->indexes[1]].xyz, light->origin, lightDir);
            //VectorNormalize(lightDir);
            //VectorMA(verts[tri->indexes[1]].xyz, 9999, lightDir, pos[2]);

            if(PlaneFromPoints(plane, pos[0], pos[1], pos[2]))
            {
                shadowPlanes[numShadowPlanes].normal[0] = plane[0];
                shadowPlanes[numShadowPlanes].normal[1] = plane[1];
                shadowPlanes[numShadowPlanes].normal[2] = plane[2];
                shadowPlanes[numShadowPlanes].dist = plane[3];

                numShadowPlanes++;
            }
            else
            {
                return 0;
            }
        }

        if(tri->neighbors[2] < 0 || !sh.facing[tri->neighbors[2]])
        {
            //indexes[numIndexes + 0] = tri->indexes[0];
            //indexes[numIndexes + 1] = tri->indexes[2];
            //indexes[numIndexes + 2] = tri->indexes[2] + numVerts;

            VectorCopy(verts[tri->indexes[0]].xyz, pos[0]);
            VectorCopy(verts[tri->indexes[2]].xyz, pos[1]);
            VectorCopy(light->origin, pos[2]);

            // extrude the infinite one
            //VectorSubtract(verts[tri->indexes[2]].xyz, light->origin, lightDir);
            //VectorNormalize(lightDir);
            //VectorMA(verts[tri->indexes[2]].xyz, 9999, lightDir, pos[2]);

            if(PlaneFromPoints(plane, pos[0], pos[1], pos[2]))
            {
                shadowPlanes[numShadowPlanes].normal[0] = plane[0];
                shadowPlanes[numShadowPlanes].normal[1] = plane[1];
                shadowPlanes[numShadowPlanes].normal[2] = plane[2];
                shadowPlanes[numShadowPlanes].dist = plane[3];

                numShadowPlanes++;
            }
            else
            {
                return 0;
            }
        }
    }

    // set up indices for light and dark caps
    for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
    {
        if(!sh.facing[i])
        {
            continue;
        }

        // light cap
        //indexes[numIndexes + 0] = tri->indexes[0];
        //indexes[numIndexes + 1] = tri->indexes[1];
        //indexes[numIndexes + 2] = tri->indexes[2];

        VectorCopy(verts[tri->indexes[0]].xyz, pos[0]);
        VectorCopy(verts[tri->indexes[1]].xyz, pos[1]);
        VectorCopy(verts[tri->indexes[2]].xyz, pos[2]);

        if(PlaneFromPoints(plane, pos[0], pos[1], pos[2]))
        {
            shadowPlanes[numShadowPlanes].normal[0] = plane[0];
            shadowPlanes[numShadowPlanes].normal[1] = plane[1];
            shadowPlanes[numShadowPlanes].normal[2] = plane[2];
            shadowPlanes[numShadowPlanes].dist = plane[3];

            numShadowPlanes++;
        }
        else
        {
            return 0;
        }
    }


    for(i = 0; i < numShadowPlanes; i++)
    {
        //vec_t           length, ilength;

        shadowPlanes[i].type = PLANE_NON_AXIAL;


        SetPlaneSignbits(&shadowPlanes[i]);
    }

    return numShadowPlanes;
}
*/

/**
 * @brief R_PrecacheGenericSurfInteraction
 * @param[in] surfT
 * @param shader - unused
 * @param[in] light
 * @return
 */
static qboolean R_PrecacheGenericSurfInteraction(srfGeneric_t *surf, shader_t *shader, trRefLight_t *light)
{
	// check if bounds intersect
	if (!BoundsIntersect(light->worldBounds[0], light->worldBounds[1], surf->bounds[0], surf->bounds[1]))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief R_PrecacheInteractionSurface
 * @param[in,out] surf
 * @param[in] light
 */
static void R_PrecacheInteractionSurface(bspSurface_t *surf, trRefLight_t *light)
{
	qboolean intersects;

	if (surf->lightCount == s_lightCount)
	{
		return;                 // already checked this surface
	}
	surf->lightCount = s_lightCount;

	// skip all surfaces that don't matter for lighting only pass
	if (surf->shader->isSky || (!surf->shader->interactLight && surf->shader->noShadows))
	{
		return;
	}

	switch (*surf->data)
	{
	case SF_FACE:
	case SF_GRID:
	case SF_TRIANGLES:
		intersects = R_PrecacheGenericSurfInteraction((srfGeneric_t *) surf->data, surf->shader, light);
		break;
	default:
		intersects = qfalse;
		break;
	}

	if (intersects)
	{
		R_PrecacheInteraction(light, surf);
	}
}

/**
 * @brief R_RecursivePrecacheInteractionNode
 * @param[in,out] node
 * @param[in] light
 */
static void R_RecursivePrecacheInteractionNode(bspNode_t *node, trRefLight_t *light)
{
	int r;

	do
	{
		// light already hit node
		if (node->lightCount == s_lightCount)
		{
			return;
		}

		node->lightCount = s_lightCount;

		if (node->contents != CONTENTS_NODE)
		{
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative
		r = BoxOnPlaneSide(light->worldBounds[0], light->worldBounds[1], node->plane);

		switch (r)
		{
		case 1:
			node = node->children[0];
			break;
		case 2:
			node = node->children[1];
			break;
		case 3:
		default:
			// recurse down the children, front side first
			R_RecursivePrecacheInteractionNode(node->children[0], light);

			// tail recurse
			node = node->children[1];
			break;
		}
	}
	while (1);
	{
		// leaf node, so add mark surfaces
		int          c;
		bspSurface_t *surf, **mark;

		// add the individual surfaces
		mark = node->markSurfaces;
		c    = node->numMarkSurfaces;

		while (c--)
		{
			// the surface may have already been added if it
			// spans multiple leafs
			surf = *mark;
			R_PrecacheInteractionSurface(surf, light);
			mark++;
		}
	}
}

/**
 * @brief R_RecursiveAddInteractionNode
 * @param[in,out] node
 * @param[in] light
 */
static void R_RecursiveAddInteractionNode(bspNode_t *node, trRefLight_t *light)
{
	int r;

	do
	{
		// light already hit node
		if (node->lightCount == s_lightCount)
		{
			return;
		}

		node->lightCount = s_lightCount;

		if (node->contents != CONTENTS_NODE)
		{
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative
		r = BoxOnPlaneSide(light->worldBounds[0], light->worldBounds[1], node->plane);

		switch (r)
		{
		case 1:
			node = node->children[0];
			break;
		case 2:
			node = node->children[1];
			break;
		case 3:
		default:
			// recurse down the children, front side first
			R_RecursiveAddInteractionNode(node->children[0], light);

			// tail recurse
			node = node->children[1];
			break;
		}
	}
	while (1);

	{
		//leaf node
		vec3_t worldBounds[2];

		VectorCopy(node->mins, worldBounds[0]);
		VectorCopy(node->maxs, worldBounds[1]);

		if (node->numMarkSurfaces > 0 && R_CullLightWorldBounds(light, worldBounds) != CULL_OUT)
		{
			link_t *l;

			l = (link_t *)ri.Hunk_Alloc(sizeof(*l), h_low);
			InitLink(l, node);

			InsertLink(l, &light->leafs);

			light->leafs.numElements++;
		}
	}
}

/**
 * @brief R_ShadowFrustumCullWorldBounds
 * @param[in] numShadowPlanes
 * @param[in] shadowPlanes
 * @param[in] worldBounds
 * @return CULL_IN, CULL_CLIP, or CULL_OUT
 *
 * @note Unused
 */
int R_ShadowFrustumCullWorldBounds(int numShadowPlanes, cplane_t *shadowPlanes, vec3_t worldBounds[2])
{
	int      i;
	cplane_t *plane;
	qboolean anyClip;
	int      r;

	if (!numShadowPlanes)
	{
		return CULL_CLIP;
	}

	// check against frustum planes
	anyClip = qfalse;
	for (i = 0; i < numShadowPlanes; i++)
	{
		plane = &shadowPlanes[i];
		r     = BoxOnPlaneSide(worldBounds[0], worldBounds[1], plane);

		if (r == 2)
		{
			// completely outside frustum
			return CULL_OUT;
		}
		if (r == 3)
		{
			anyClip = qtrue;
		}
	}

	if (!anyClip)
	{
		// completely inside frustum
		return CULL_IN;
	}

	// partially clipped
	return CULL_CLIP;
}

/**
 * @brief R_CreateInteractionVBO
 * @param[in,out] light
 * @return
 */
static interactionVBO_t *R_CreateInteractionVBO(trRefLight_t *light)
{
	interactionVBO_t *iaVBO;

	iaVBO = (interactionVBO_t *)ri.Hunk_Alloc(sizeof(*iaVBO), h_low);

	// connect to interaction grid
	if (!light->firstInteractionVBO)
	{
		light->firstInteractionVBO = iaVBO;
	}

	if (light->lastInteractionVBO)
	{
		light->lastInteractionVBO->next = iaVBO;
	}

	light->lastInteractionVBO = iaVBO;
	iaVBO->next               = NULL;

	return iaVBO;
}

/**
 * @brief Compare function for qsort()
 * @param[in] a
 * @param[in] b
 * @return
 */
static int InteractionCacheCompare(const void *a, const void *b)
{
	interactionCache_t *aa = *(interactionCache_t **) a;
	interactionCache_t *bb = *(interactionCache_t **) b;

	// shader first
	if (aa->surface->shader < bb->surface->shader)
	{
		return -1;
	}
	else if (aa->surface->shader > bb->surface->shader)
	{
		return 1;
	}

	// then alphaTest
	if (aa->surface->shader->alphaTest < bb->surface->shader->alphaTest)
	{
		return -1;
	}
	else if (aa->surface->shader->alphaTest > bb->surface->shader->alphaTest)
	{
		return 1;
	}

	return 0;
}

/**
 * @brief UpdateLightTriangles
 * @param[in] verts
 * @param[in] numTriangles
 * @param[in] triangles
 * @param[in] surfaceShader
 * @param[in] light
 * @return
 */
static int UpdateLightTriangles(const srfVert_t *verts, int numTriangles, srfTriangle_t *triangles, shader_t *surfaceShader,
                                trRefLight_t *light)
{
	int           i;
	srfTriangle_t *tri;
	int           numFacing = 0;

#if 1
	vec3_t lightDirection;

	vec3_t pos[3];
	vec4_t triPlane;
	float  d;
#endif

	for (i = 0, tri = triangles; i < numTriangles; i++, tri++)
	{
#if 1
		VectorCopy(verts[tri->indexes[0]].xyz, pos[0]);
		VectorCopy(verts[tri->indexes[1]].xyz, pos[1]);
		VectorCopy(verts[tri->indexes[2]].xyz, pos[2]);

		if (PlaneFromPoints(triPlane, pos[0], pos[1], pos[2]))
		{
			if (light->l.rlType == RL_DIRECTIONAL)
			{
				// light direction is from surface to light
#if 1
				VectorCopy(tr.sunDirection, lightDirection); // use sun light
#else
				VectorCopy(light->direction, lightDirection); // use global light
#endif

				d = DotProduct(triPlane, lightDirection);

				if (surfaceShader->cullType == CT_TWO_SIDED || (d > 0 && surfaceShader->cullType != CT_BACK_SIDED))
				{
					tri->facingLight = qtrue; // surfaceShader->cullType == CT_TWO_SIDED || ( d > 0 && surfaceShader->cullType != CT_BACK_SIDED );
				}
				else
				{
					tri->facingLight = qfalse;
				}
			}
			else
			{
				// check if light origin is behind triangle
				d = DotProduct(triPlane, light->origin) - triPlane[3];

				if (surfaceShader->cullType == CT_TWO_SIDED || (d > 0 && surfaceShader->cullType != CT_BACK_SIDED))
				{
					tri->facingLight = qtrue;
				}
				else
				{
					tri->facingLight = qfalse;
				}
			}
		}
		else
		{
			tri->facingLight = qtrue;   // FIXME ?
		}

		if (R_CullLightTriangle(light, pos) == CULL_OUT)
		{
			tri->facingLight = qfalse;
		}

		if (tri->facingLight)
		{
			numFacing++;
		}
#else
		tri->facingLight = qtrue;
		numFacing++;
#endif
	}

	return numFacing;
}

/**
 * @brief R_CreateVBOLightMeshes
 * @param[in] light
 */
static void R_CreateVBOLightMeshes(trRefLight_t *light)
{
#if 1
	int                i, j, k, l;
	int                numVerts;
	int                numTriangles, numLitTriangles;
	srfTriangle_t      *triangles;
	srfTriangle_t      *tri;
	interactionVBO_t   *iaVBO;
	interactionCache_t *iaCache, *iaCache2;
	interactionCache_t **iaCachesSorted;
	int                numCaches;
	shader_t           *shader, *oldShader;
	bspSurface_t       *surface;
	srfVBOMesh_t       *vboSurf;
	vec3_t             bounds[2];

	if (!r_vboLighting->integer)
	{
		return;
	}

	if (!light->firstInteractionCache)
	{
		// this light has no interactions precached
		return;
	}

	// count number of interaction caches
	numCaches = 0;
	for (iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
	{
		if (iaCache->redundant)
		{
			continue;
		}

		surface = iaCache->surface;

		if (!surface->shader->interactLight)
		{
			continue;
		}

		if (surface->shader->isPortal)
		{
			continue;
		}

		if (ShaderRequiresCPUDeforms(surface->shader))
		{
			continue;
		}

		numCaches++;
	}

	// build interaction caches list
	iaCachesSorted = (interactionCache_t **)ri.Hunk_AllocateTempMemory(numCaches * sizeof(iaCachesSorted[0]));

	numCaches = 0;
	for (iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
	{
		if (iaCache->redundant)
		{
			continue;
		}

		surface = iaCache->surface;

		if (!surface->shader->interactLight)
		{
			continue;
		}

		if (surface->shader->isPortal)
		{
			continue;
		}

		if (ShaderRequiresCPUDeforms(surface->shader))
		{
			continue;
		}

		iaCachesSorted[numCaches] = iaCache;
		numCaches++;
	}

	// sort interaction caches by shader
	qsort(iaCachesSorted, numCaches, sizeof(*iaCachesSorted), InteractionCacheCompare);

	// create a VBO for each shader
	shader = oldShader = NULL;

	for (k = 0; k < numCaches; k++)
	{
		iaCache                = iaCachesSorted[k];
		iaCache->mergedIntoVBO = qtrue;
		shader                 = iaCache->surface->shader;

		if (shader != oldShader)
		{
			oldShader = shader;

			// count vertices and indices
			numVerts     = 0;
			numTriangles = 0;

			ClearBounds(bounds[0], bounds[1]);
			for (l = k; l < numCaches; l++)
			{
				iaCache2 = iaCachesSorted[l];
				surface  = iaCache2->surface;

				if (surface->shader != shader)
				{
					continue;
				}

				if (*surface->data == SF_FACE)
				{
					srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

					if (srf->numTriangles)
					{
						numLitTriangles = UpdateLightTriangles(s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light);
						if (numLitTriangles)
						{
							BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
						}

						numTriangles += numLitTriangles;
					}

					if (srf->numVerts)
					{
						numVerts += srf->numVerts;
					}
				}
				else if (*surface->data == SF_GRID)
				{
					srfGridMesh_t *srf = (srfGridMesh_t *) surface->data;

					if (srf->numTriangles)
					{
						numLitTriangles = UpdateLightTriangles(s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light);
						if (numLitTriangles)
						{
							BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
						}

						numTriangles += numLitTriangles;
					}

					if (srf->numVerts)
					{
						numVerts += srf->numVerts;
					}
				}
				else if (*surface->data == SF_TRIANGLES)
				{
					srfTriangles_t *srf = (srfTriangles_t *) surface->data;

					if (srf->numTriangles)
					{
						numLitTriangles = UpdateLightTriangles(s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light);
						if (numLitTriangles)
						{
							BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
						}

						numTriangles += numLitTriangles;
					}

					if (srf->numVerts)
					{
						numVerts += srf->numVerts;
					}
				}
			}

			if (!numVerts || !numTriangles)
			{
				continue;
			}

			//Ren_Print("...calculating light mesh VBOs ( %s, %i verts %i tris )\n", shader->name, vertexesNum, indexesNum / 3);

			// create surface
			vboSurf              = (srfVBOMesh_t *)ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
			vboSurf->surfaceType = SF_VBO_MESH;
			vboSurf->numIndexes  = numTriangles * 3;
			vboSurf->numVerts    = numVerts;
			vboSurf->lightmapNum = LIGHTMAP_NONE;

			VectorCopy(bounds[0], vboSurf->bounds[0]);
			VectorCopy(bounds[1], vboSurf->bounds[1]);

			// create arrays
			triangles    = (srfTriangle_t *)ri.Hunk_AllocateTempMemory(numTriangles * sizeof(srfTriangle_t));
			numTriangles = 0;

			// build triangle indices
			for (l = k; l < numCaches; l++)
			{
				iaCache2 = iaCachesSorted[l];
				surface  = iaCache2->surface;

				if (surface->shader != shader)
				{
					continue;
				}

				if (*surface->data == SF_FACE)
				{
					srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

					for (i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
					{
						if (tri->facingLight)
						{
							for (j = 0; j < 3; j++)
							{
								triangles[numTriangles].indexes[j] = tri->indexes[j];
							}

							numTriangles++;
						}
					}
				}
				else if (*surface->data == SF_GRID)
				{
					srfGridMesh_t *srf = (srfGridMesh_t *) surface->data;

					for (i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
					{
						if (tri->facingLight)
						{
							for (j = 0; j < 3; j++)
							{
								triangles[numTriangles].indexes[j] = tri->indexes[j];
							}

							numTriangles++;
						}
					}
				}
				else if (*surface->data == SF_TRIANGLES)
				{
					srfTriangles_t *srf = (srfTriangles_t *) surface->data;

					for (i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
					{
						if (tri->facingLight)
						{
							for (j = 0; j < 3; j++)
							{
								triangles[numTriangles].indexes[j] = tri->indexes[j];
							}

							numTriangles++;
						}
					}
				}
			}

#if CALC_REDUNDANT_SHADOWVERTS
			OptimizeTrianglesLite(s_worldData.redundantLightVerts, numTriangles, triangles);
			if (c_redundantVertexes)
			{
				Ren_Developer("...removed %i redundant vertices from staticLightMesh %i ( %s, %i verts %i tris )\n",
				              c_redundantVertexes, c_vboLightSurfaces, shader->name, numVerts, numTriangles);
			}
#endif
			vboSurf->vbo = s_worldData.vbo;
			vboSurf->ibo =
				R_CreateIBO2(va("staticLightMesh_IBO %i", c_vboLightSurfaces), numTriangles, triangles, VBO_USAGE_STATIC);

			ri.Hunk_FreeTempMemory(triangles);

			// add everything needed to the light
			iaVBO               = R_CreateInteractionVBO(light);
			iaVBO->shader       = (struct shader_s *)shader;
			iaVBO->vboLightMesh = (struct srfVBOMesh_s *)vboSurf;

			c_vboLightSurfaces++;
		}
	}

	ri.Hunk_FreeTempMemory(iaCachesSorted);
#endif
}

/**
 * @brief R_CreateVBOShadowMeshes
 * @param[in] light
 */
static void R_CreateVBOShadowMeshes(trRefLight_t *light)
{
#if 1
	int                i, j, k, l;
	int                numVerts;
	int                numTriangles, numLitTriangles;
	srfTriangle_t      *triangles;
	srfTriangle_t      *tri;
	interactionVBO_t   *iaVBO;
	interactionCache_t *iaCache, *iaCache2;
	interactionCache_t **iaCachesSorted;
	int                numCaches;
	shader_t           *shader, *oldShader;
	qboolean           alphaTest, oldAlphaTest;
	bspSurface_t       *surface;
	srfVBOMesh_t       *vboSurf;
	vec3_t             bounds[2];

	if (!r_vboShadows->integer)
	{
		return;
	}

	if (r_shadows->integer < SHADOWING_ESM16)
	{
		return;
	}

	if (!light->firstInteractionCache)
	{
		// this light has no interactions precached
		return;
	}

	if (light->l.noShadows)
	{
		return;
	}

	switch (light->l.rlType)
	{
	case RL_OMNI:
		return;
	case RL_DIRECTIONAL:
	case RL_PROJ:
		break;
	default:
		return;
	}

	// count number of interaction caches
	numCaches = 0;
	for (iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
	{
		if (iaCache->redundant)
		{
			continue;
		}

		surface = iaCache->surface;

		if (!surface->shader->interactLight)
		{
			continue;
		}

		if (surface->shader->isSky)
		{
			continue;
		}

		if (surface->shader->noShadows)
		{
			continue;
		}

		if (surface->shader->sort > SS_OPAQUE)
		{
			continue;
		}

		if (surface->shader->isPortal)
		{
			continue;
		}

		if (ShaderRequiresCPUDeforms(surface->shader))
		{
			continue;
		}

		numCaches++;
	}

	// build interaction caches list
	iaCachesSorted = (interactionCache_t **)ri.Hunk_AllocateTempMemory(numCaches * sizeof(iaCachesSorted[0]));

	numCaches = 0;
	for (iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
	{
		if (iaCache->redundant)
		{
			continue;
		}

		surface = iaCache->surface;

		if (!surface->shader->interactLight)
		{
			continue;
		}

		if (surface->shader->isSky)
		{
			continue;
		}

		if (surface->shader->noShadows)
		{
			continue;
		}

		if (surface->shader->sort > SS_OPAQUE)
		{
			continue;
		}

		if (surface->shader->isPortal)
		{
			continue;
		}

		if (ShaderRequiresCPUDeforms(surface->shader))
		{
			continue;
		}

		iaCachesSorted[numCaches] = iaCache;
		numCaches++;
	}

	// sort interaction caches by shader
	qsort(iaCachesSorted, numCaches, sizeof(*iaCachesSorted), InteractionCacheCompare);

	// create a VBO for each shader
	shader       = oldShader = NULL;
	oldAlphaTest = alphaTest = -1;

	for (k = 0; k < numCaches; k++)
	{
		iaCache                = iaCachesSorted[k];
		iaCache->mergedIntoVBO = qtrue;

		shader    = iaCache->surface->shader;
		alphaTest = shader->alphaTest;

		iaCache->mergedIntoVBO = qtrue;

		if (alphaTest ? shader != oldShader : alphaTest != oldAlphaTest)
		{
			oldShader    = shader;
			oldAlphaTest = alphaTest;

			// count vertices and indices
			numVerts     = 0;
			numTriangles = 0;

			ClearBounds(bounds[0], bounds[1]);
			for (l = k; l < numCaches; l++)
			{
				iaCache2 = iaCachesSorted[l];
				surface  = iaCache2->surface;

#if 0
				if (surface->shader != shader)
				{
					break;
				}
#else
				if (alphaTest)
				{
					if (surface->shader != shader)
					{
						break;
					}
				}
				else
				{
					if (surface->shader->alphaTest != alphaTest)
					{
						break;
					}
				}
#endif

				if (*surface->data == SF_FACE)
				{
					srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

					if (srf->numTriangles)
					{
						numLitTriangles = UpdateLightTriangles(s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light);
						if (numLitTriangles)
						{
							BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
						}

						numTriangles += numLitTriangles;
					}

					if (srf->numVerts)
					{
						numVerts += srf->numVerts;
					}
				}
				else if (*surface->data == SF_GRID)
				{
					srfGridMesh_t *srf = (srfGridMesh_t *) surface->data;

					if (srf->numTriangles)
					{
						numLitTriangles = UpdateLightTriangles(s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light);
						if (numLitTriangles)
						{
							BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
						}

						numTriangles += numLitTriangles;
					}

					if (srf->numVerts)
					{
						numVerts += srf->numVerts;
					}
				}
				else if (*surface->data == SF_TRIANGLES)
				{
					srfTriangles_t *srf = (srfTriangles_t *) surface->data;

					if (srf->numTriangles)
					{
						numLitTriangles = UpdateLightTriangles(s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light);
						if (numLitTriangles)
						{
							BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
						}

						numTriangles += numLitTriangles;
					}

					if (srf->numVerts)
					{
						numVerts += srf->numVerts;
					}
				}
			}

			if (!numVerts || !numTriangles)
			{
				continue;
			}

			//Ren_Print("...calculating light mesh VBOs ( %s, %i verts %i tris )\n", shader->name, vertexesNum, indexesNum / 3);

			// create surface
			vboSurf              = (srfVBOMesh_t *)ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
			vboSurf->surfaceType = SF_VBO_MESH;
			vboSurf->numIndexes  = numTriangles * 3;
			vboSurf->numVerts    = numVerts;
			vboSurf->lightmapNum = LIGHTMAP_NONE;

			VectorCopy(bounds[0], vboSurf->bounds[0]);
			VectorCopy(bounds[1], vboSurf->bounds[1]);

			// create arrays
			triangles    = (srfTriangle_t *)ri.Hunk_AllocateTempMemory(numTriangles * sizeof(srfTriangle_t));
			numTriangles = 0;

			// build triangle indices
			for (l = k; l < numCaches; l++)
			{
				iaCache2 = iaCachesSorted[l];
				surface  = iaCache2->surface;

#if 0
				if (surface->shader != shader)
				{
					break;
				}
#else
				if (alphaTest)
				{
					if (surface->shader != shader)
					{
						break;
					}
				}
				else
				{
					if (surface->shader->alphaTest != alphaTest)
					{
						break;
					}
				}
#endif

				if (*surface->data == SF_FACE)
				{
					srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

					for (i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
					{
						if (tri->facingLight)
						{
							for (j = 0; j < 3; j++)
							{
								triangles[numTriangles].indexes[j] = tri->indexes[j];
							}

							numTriangles++;
						}
					}
				}
				else if (*surface->data == SF_GRID)
				{
					srfGridMesh_t *srf = (srfGridMesh_t *) surface->data;

					for (i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
					{
						if (tri->facingLight)
						{
							for (j = 0; j < 3; j++)
							{
								triangles[numTriangles].indexes[j] = tri->indexes[j];
							}

							numTriangles++;
						}
					}
				}
				else if (*surface->data == SF_TRIANGLES)
				{
					srfTriangles_t *srf = (srfTriangles_t *) surface->data;

					for (i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
					{
						if (tri->facingLight)
						{
							for (j = 0; j < 3; j++)
							{
								triangles[numTriangles].indexes[j] = tri->indexes[j];
							}

							numTriangles++;
						}
					}
				}
			}

#if CALC_REDUNDANT_SHADOWVERTS
			OptimizeTrianglesLite(s_worldData.redundantShadowVerts, numTriangles, triangles);
			if (c_redundantVertexes)
			{
				Ren_Developer("...removed %i redundant vertices from staticLightMesh %i ( %s, %i verts %i tris )\n",
				              c_redundantVertexes, c_vboLightSurfaces, shader->name, numVerts, numTriangles);
			}
#endif
			vboSurf->vbo = s_worldData.vbo;
			vboSurf->ibo = R_CreateIBO2(va("staticShadowMesh_IBO %i", c_vboLightSurfaces), numTriangles, triangles, VBO_USAGE_STATIC);

			ri.Hunk_FreeTempMemory(triangles);

			// add everything needed to the light
			iaVBO                = R_CreateInteractionVBO(light);
			iaVBO->shader        = (struct shader_s *)shader;
			iaVBO->vboShadowMesh = (struct srfVBOMesh_s *)vboSurf;

			c_vboShadowSurfaces++;
		}
	}

	ri.Hunk_FreeTempMemory(iaCachesSorted);
#endif
}

/**
 * @brief R_CreateVBOShadowCubeMeshes
 * @param[in] light
 */
static void R_CreateVBOShadowCubeMeshes(trRefLight_t *light)
{
	int                i, j, k, l;
	int                numVerts;
	int                numTriangles;
	srfTriangle_t      *triangles;
	srfTriangle_t      *tri;
	interactionVBO_t   *iaVBO;
	interactionCache_t *iaCache, *iaCache2;
	interactionCache_t **iaCachesSorted;
	int                numCaches;
	shader_t           *shader, *oldShader;
	qboolean           alphaTest, oldAlphaTest;
	int                cubeSide;
	bspSurface_t       *surface;
	srfVBOMesh_t       *vboSurf;

	if (!r_vboShadows->integer)
	{
		return;
	}

	if (r_shadows->integer < SHADOWING_ESM16)
	{
		return;
	}

	if (!light->firstInteractionCache)
	{
		// this light has no interactions precached
		return;
	}

	if (light->l.noShadows)
	{
		return;
	}

	if (light->l.rlType != RL_OMNI)
	{
		return;
	}

	// count number of interaction caches
	numCaches = 0;
	for (iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
	{
		if (iaCache->redundant)
		{
			continue;
		}

		surface = iaCache->surface;

		if (!surface->shader->interactLight)
		{
			continue;
		}

		if (surface->shader->isSky)
		{
			continue;
		}

		if (surface->shader->noShadows)
		{
			continue;
		}

		if (surface->shader->sort > SS_OPAQUE)
		{
			continue;
		}

		if (surface->shader->isPortal)
		{
			continue;
		}

		if (ShaderRequiresCPUDeforms(surface->shader))
		{
			continue;
		}

		numCaches++;
	}

	// build interaction caches list
	iaCachesSorted = (interactionCache_t **)ri.Hunk_AllocateTempMemory(numCaches * sizeof(iaCachesSorted[0]));

	numCaches = 0;
	for (iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
	{
		if (iaCache->redundant)
		{
			continue;
		}

		surface = iaCache->surface;

		if (!surface->shader->interactLight)
		{
			continue;
		}

		if (surface->shader->isSky)
		{
			continue;
		}

		if (surface->shader->noShadows)
		{
			continue;
		}

		if (surface->shader->sort > SS_OPAQUE)
		{
			continue;
		}

		if (surface->shader->isPortal)
		{
			continue;
		}

		if (ShaderRequiresCPUDeforms(surface->shader))
		{
			continue;
		}

		iaCachesSorted[numCaches] = iaCache;
		numCaches++;
	}

	// sort interaction caches by shader
	qsort(iaCachesSorted, numCaches, sizeof(*iaCachesSorted), InteractionCacheCompare);

	// create a VBO for each shader
	for (cubeSide = 0; cubeSide < 6; cubeSide++)
	{
		shader       = oldShader = NULL;
		oldAlphaTest = alphaTest = -1;

		for (k = 0; k < numCaches; k++)
		{
			iaCache   = iaCachesSorted[k];
			shader    = iaCache->surface->shader;
			alphaTest = shader->alphaTest;

			iaCache->mergedIntoVBO = qtrue;

			//if(!(iaCache->cubeSideBits & (1 << cubeSide)))
			//  continue;

			//if(shader != oldShader)
			if (alphaTest ? shader != oldShader : alphaTest != oldAlphaTest)
			{
				oldShader    = shader;
				oldAlphaTest = alphaTest;

				// count vertices and indices
				numVerts     = 0;
				numTriangles = 0;

				for (l = k; l < numCaches; l++)
				{
					iaCache2 = iaCachesSorted[l];

					surface = iaCache2->surface;

#if 0
					if (surface->shader != shader)
					{
						break;
					}
#else
					if (alphaTest)
					{
						if (surface->shader != shader)
						{
							break;
						}
					}
					else
					{
						if (surface->shader->alphaTest != alphaTest)
						{
							break;
						}
					}
#endif

					if (!(iaCache2->cubeSideBits & (1 << cubeSide)))
					{
						continue;
					}

					if (*surface->data == SF_FACE)
					{
						srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

						if (srf->numTriangles)
						{
							numTriangles +=
								UpdateLightTriangles(s_worldData.verts, srf->numTriangles,
								                     s_worldData.triangles + srf->firstTriangle, surface->shader, light);
						}

						if (srf->numVerts)
						{
							numVerts += srf->numVerts;
						}
					}
					else if (*surface->data == SF_GRID)
					{
						srfGridMesh_t *srf = (srfGridMesh_t *) surface->data;

						if (srf->numTriangles)
						{
							numTriangles +=
								UpdateLightTriangles(s_worldData.verts, srf->numTriangles,
								                     s_worldData.triangles + srf->firstTriangle, surface->shader, light);
						}

						if (srf->numVerts)
						{
							numVerts += srf->numVerts;
						}
					}
					else if (*surface->data == SF_TRIANGLES)
					{
						srfTriangles_t *srf = (srfTriangles_t *) surface->data;

						if (srf->numTriangles)
						{
							numTriangles +=
								UpdateLightTriangles(s_worldData.verts, srf->numTriangles,
								                     s_worldData.triangles + srf->firstTriangle, surface->shader, light);
						}

						if (srf->numVerts)
						{
							numVerts += srf->numVerts;
						}
					}
				}

				if (!numVerts || !numTriangles)
				{
					continue;
				}

				//Ren_Print("...calculating light mesh VBOs ( %s, %i verts %i tris )\n", shader->name, vertexesNum, indexesNum / 3);

				// create surface
				vboSurf              = (srfVBOMesh_t *)ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
				vboSurf->surfaceType = SF_VBO_MESH;
				vboSurf->numIndexes  = numTriangles * 3;
				vboSurf->numVerts    = numVerts;
				vboSurf->lightmapNum = LIGHTMAP_NONE;
				ZeroBounds(vboSurf->bounds[0], vboSurf->bounds[1]);

				// create arrays
				triangles    = (srfTriangle_t *)ri.Hunk_AllocateTempMemory(numTriangles * sizeof(srfTriangle_t));
				numTriangles = 0;

				// build triangle indices
				for (l = k; l < numCaches; l++)
				{
					iaCache2 = iaCachesSorted[l];
					surface  = iaCache2->surface;

#if 0
					if (surface->shader != shader)
					{
						break;
					}
#else
					if (alphaTest)
					{
						if (surface->shader != shader)
						{
							break;
						}
					}
					else
					{
						if (surface->shader->alphaTest != alphaTest)
						{
							break;
						}
					}
#endif

					if (!(iaCache2->cubeSideBits & (1 << cubeSide)))
					{
						continue;
					}

					if (*surface->data == SF_FACE)
					{
						srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface->data;

						for (i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
						{
							if (tri->facingLight)
							{
								for (j = 0; j < 3; j++)
								{
									triangles[numTriangles].indexes[j] = tri->indexes[j];
								}

								numTriangles++;
							}
						}
					}
					else if (*surface->data == SF_GRID)
					{
						srfGridMesh_t *srf = (srfGridMesh_t *) surface->data;

						for (i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
						{
							if (tri->facingLight)
							{
								for (j = 0; j < 3; j++)
								{
									triangles[numTriangles].indexes[j] = tri->indexes[j];
								}

								numTriangles++;
							}
						}
					}
					else if (*surface->data == SF_TRIANGLES)
					{
						srfTriangles_t *srf = (srfTriangles_t *) surface->data;

						for (i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
						{
							if (tri->facingLight)
							{
								for (j = 0; j < 3; j++)
								{
									triangles[numTriangles].indexes[j] = tri->indexes[j];
								}

								numTriangles++;
							}
						}
					}
				}

#if CALC_REDUNDANT_SHADOWVERTS
				if (alphaTest)
				{
					//OptimizeTriangles(s_worldData.numVerts, s_worldData.verts, numTriangles, triangles, CompareShadowVertAlphaTest);
					OptimizeTrianglesLite(s_worldData.redundantShadowAlphaTestVerts, numTriangles, triangles);
				}
				else
				{
					//OptimizeTriangles(s_worldData.numVerts, s_worldData.verts, numTriangles, triangles, CompareShadowVert);
					OptimizeTrianglesLite(s_worldData.redundantShadowVerts, numTriangles, triangles);
				}

				if (c_redundantVertexes)
				{
					Ren_Developer(
						"...removed %i redundant vertices from staticShadowPyramidMesh %i ( %s, %i verts %i tris )\n",
						c_redundantVertexes, c_vboShadowSurfaces, shader->name, numVerts, numTriangles);
				}
#endif
				vboSurf->vbo = s_worldData.vbo;
				vboSurf->ibo = R_CreateIBO2(va("staticShadowPyramidMesh_IBO %i", c_vboShadowSurfaces), numTriangles, triangles,
				                            VBO_USAGE_STATIC);

				ri.Hunk_FreeTempMemory(triangles);

				// add everything needed to the light
				iaVBO                = R_CreateInteractionVBO(light);
				iaVBO->cubeSideBits  = (1 << cubeSide);
				iaVBO->shader        = (struct shader_s *)shader;
				iaVBO->vboShadowMesh = (struct srfVBOMesh_s *)vboSurf;

				c_vboShadowSurfaces++;
			}
		}
	}

	ri.Hunk_FreeTempMemory(iaCachesSorted);
}

/**
 * @brief R_CalcInteractionCubeSideBits
 * @param[in] light
 */
static void R_CalcInteractionCubeSideBits(trRefLight_t *light)
{
	interactionCache_t *iaCache;
	bspSurface_t       *surface;
	vec3_t             localBounds[2];

	if (r_shadows->integer <= SHADOWING_BLOB)
	{
		return;
	}

	if (!light->firstInteractionCache)
	{
		// this light has no interactions precached
		return;
	}

	if (light->l.noShadows)
	{
		// actually noShadows lights are quite bad concerning this optimization
		return;
	}

	if (light->l.rlType != RL_OMNI)
	{
		return;
	}

	/*
	   if(glConfig.vertexBufferObjectAvailable && r_vboLighting->integer)
	   {
	   srfVBOLightMesh_t *srf;

	   for(iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
	   {
	   if(iaCache->redundant)
	   continue;

	   if(!iaCache->vboLightMesh)
	   continue;

	   srf = iaCache->vboLightMesh;

	   VectorCopy(srf->bounds[0], localBounds[0]);
	   VectorCopy(srf->bounds[1], localBounds[1]);

	   light->shadowLOD = 0;    // important for R_CalcLightCubeSideBits
	   iaCache->cubeSideBits = R_CalcLightCubeSideBits(light, localBounds);
	   }
	   }
	   else
	 */
	{
		for (iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
		{
			surface = iaCache->surface;

			if (*surface->data == SF_FACE || *surface->data == SF_GRID || *surface->data == SF_TRIANGLES)
			{
				srfGeneric_t *gen;

				gen = (srfGeneric_t *) surface->data;

				VectorCopy(gen->bounds[0], localBounds[0]);
				VectorCopy(gen->bounds[1], localBounds[1]);
			}
			else
			{
				iaCache->cubeSideBits = CUBESIDE_CLIPALL;
				continue;
			}

			light->shadowLOD      = 0; // important for R_CalcLightCubeSideBits
			iaCache->cubeSideBits = R_CalcLightCubeSideBits(light, localBounds);
		}
	}
}

/**
 * @brief R_PrecacheInteractions
 */
void R_PrecacheInteractions()
{
	int          i;
	trRefLight_t *light;
	bspSurface_t *surface;
#ifdef ETLEGACY_DEBUG
	int startTime, endTime;

	startTime = ri.Milliseconds();
#endif

	// reset surfaces' viewCount
	s_lightCount = 0;
	for (i = 0, surface = s_worldData.surfaces; i < s_worldData.numSurfaces; i++, surface++)
	{
		surface->lightCount = -1;
	}

	Com_InitGrowList(&s_interactions, 100); //Com_InitGrowList(&s_interactions, 10000);

	c_vboWorldSurfaces  = 0;
	c_vboLightSurfaces  = 0;
	c_vboShadowSurfaces = 0;

	Ren_Developer("...precaching %i lights\n", s_worldData.numLights);

	for (i = 0; i < s_worldData.numLights; i++)
	{
		light = &s_worldData.lights[i];

		if (!r_staticLight->integer || ((r_precomputedLighting->integer || r_vertexLighting->integer) && light->noRadiosity))
		{
			continue;
		}

#if 0
		Ren_Print("light %i: origin(%i %i %i) radius(%i %i %i) color(%f %f %f)\n",
		          i,
		          (int)light->l.origin[0], (int)light->l.origin[1], (int)light->l.origin[2],
		          (int)light->l.radius[0], (int)light->l.radius[1], (int)light->l.radius[2],
		          light->l.color[0], light->l.color[1], light->l.color[2]);
#endif

		// set up light transform matrix
		MatrixSetupTransformFromQuat(light->transformMatrix, light->l.rotation, light->l.origin);

		// set up light origin for lighting and shadowing
		R_SetupLightOrigin(light);

		// set up model to light view matrix
		R_SetupLightView(light);

		// set up projection
		R_SetupLightProjection(light);

		// calc local bounds for culling
		R_SetupLightLocalBounds(light);

		// setup world bounds for intersection tests
		R_SetupLightWorldBounds(light);

		// setup frustum planes for intersection tests
		R_SetupLightFrustum(light);

		// setup interactions
		light->firstInteractionCache = NULL;
		light->lastInteractionCache  = NULL;

		light->firstInteractionVBO = NULL;
		light->lastInteractionVBO  = NULL;

		// perform culling and add all the potentially visible surfaces
		s_lightCount++;
		R_RecursivePrecacheInteractionNode(s_worldData.nodes, light);

		// count number of leafs that touch this light
		s_lightCount++;
		QueueInit(&light->leafs);
		R_RecursiveAddInteractionNode(s_worldData.nodes, light);
		//Ren_Print("light %i touched %i leaves\n", i, QueueSize(&light->leafs));

		// create a static VBO surface for each light geometry batch
		R_CreateVBOLightMeshes(light);

		// create a static VBO surface for each shadow geometry batch
		R_CreateVBOShadowMeshes(light);

		// calculate pyramid bits for each interaction in omni-directional lights
		R_CalcInteractionCubeSideBits(light);

		// create a static VBO surface for each light geometry batch inside a cubemap pyramid
		R_CreateVBOShadowCubeMeshes(light);
	}

	// move interactions grow list to hunk
	s_worldData.numInteractions = s_interactions.currentElements;
	s_worldData.interactions    = (interactionCache_t **)ri.Hunk_Alloc(s_worldData.numInteractions * sizeof(*s_worldData.interactions), h_low);

	for (i = 0; i < s_worldData.numInteractions; i++)
	{
		s_worldData.interactions[i] = (interactionCache_t *) Com_GrowListElement(&s_interactions, i);
	}

	Com_DestroyGrowList(&s_interactions);

#ifdef ETLEGACY_DEBUG
	Ren_Developer("%i interactions precached\n", s_worldData.numInteractions);

	if (r_shadows->integer >= SHADOWING_ESM16)
	{
		// only interesting for omni-directional shadow mapping
		Ren_Developer("%i omni pyramid tests\n", tr.pc.c_pyramidTests);
		Ren_Developer("%i omni pyramid surfaces visible\n", tr.pc.c_pyramid_cull_ent_in);
		Ren_Developer("%i omni pyramid surfaces clipped\n", tr.pc.c_pyramid_cull_ent_clip);
		Ren_Developer("%i omni pyramid surfaces culled\n", tr.pc.c_pyramid_cull_ent_out);
	}

	endTime = ri.Milliseconds();

	Ren_Developer("lights precaching time = %5.2f seconds\n", (endTime - startTime) / 1000.0);
#endif
}

#define HASHTABLE_SIZE 7919 // 32749 // 2039    /* prime, use % */
#define HASH_USE_EPSILON

#ifdef HASH_USE_EPSILON
#define HASH_XYZ_EPSILON                    0.01f
#define HASH_XYZ_EPSILONSPACE_MULTIPLIER    1.f / HASH_XYZ_EPSILON
#endif

/**
 * @brief VertexCoordGenerateHash
 * @param[in] xyz
 * @return
 */
unsigned int VertexCoordGenerateHash(const vec3_t xyz)
{
	unsigned int hash = 0;

#ifndef HASH_USE_EPSILON
	hash += ~(*((unsigned int *)&xyz[0]) << 15);
	hash ^= (*((unsigned int *)&xyz[0]) >> 10);
	hash += (*((unsigned int *)&xyz[1]) << 3);
	hash ^= (*((unsigned int *)&xyz[1]) >> 6);
	hash += ~(*((unsigned int *)&xyz[2]) << 11);
	hash ^= (*((unsigned int *)&xyz[2]) >> 16);
#else
	vec3_t xyz_epsilonspace;

	VectorScale(xyz, HASH_XYZ_EPSILONSPACE_MULTIPLIER, xyz_epsilonspace);
	xyz_epsilonspace[0] = (float)floor(xyz_epsilonspace[0]);
	xyz_epsilonspace[1] = (float)floor(xyz_epsilonspace[1]);
	xyz_epsilonspace[2] = (float)floor(xyz_epsilonspace[2]);

	hash += ~(*((unsigned int *)&xyz_epsilonspace[0]) << 15);
	hash ^= (*((unsigned int *)&xyz_epsilonspace[0]) >> 10);
	hash += (*((unsigned int *)&xyz_epsilonspace[1]) << 3);
	hash ^= (*((unsigned int *)&xyz_epsilonspace[1]) >> 6);
	hash += ~(*((unsigned int *)&xyz_epsilonspace[2]) << 11);
	hash ^= (*((unsigned int *)&xyz_epsilonspace[2]) >> 16);
#endif

	hash = hash % (HASHTABLE_SIZE);
	return hash;
}

/**
 * @brief NewVertexHashTable
 * @return
 */
vertexHash_t **NewVertexHashTable(void)
{
	vertexHash_t **hashTable = (vertexHash_t **) ri.Hunk_Alloc(HASHTABLE_SIZE * sizeof(vertexHash_t *), h_low);

	Com_Memset(hashTable, 0, HASHTABLE_SIZE * sizeof(vertexHash_t *));

	return hashTable;
}

/**
 * @brief FreeVertexHashTable
 * @param[in] hashTable
 */
void FreeVertexHashTable(vertexHash_t **hashTable)
{
	int i;

	if (hashTable == NULL)
	{
		return;
	}

	for (i = 0; i < HASHTABLE_SIZE; i++)
	{
		Com_Dealloc(hashTable[i]);
	}

	//Com_Dealloc(hashTable); // TODO: check this..
}

/**
 * @brief FindVertexInHashTable
 * @param[in] hashTable
 * @param[in] xyz
 * @param[in] distance
 * @return
 */
vertexHash_t *FindVertexInHashTable(vertexHash_t **hashTable, const vec3_t xyz, float distance)
{
	unsigned int hash;
	vertexHash_t *vertexHash;

	if (hashTable == NULL || xyz == NULL)
	{
		return NULL;
	}

	hash = VertexCoordGenerateHash(xyz);

	for (vertexHash = hashTable[hash]; vertexHash; vertexHash = vertexHash->next)
	{
#ifndef HASH_USE_EPSILON
		if ((vertexHash->vcd.xyz[0] != xyz[0] || vertexHash->vcd.xyz[1] != xyz[1] ||
		     vertexHash->vcd.xyz[2] != xyz[2]))
		{
			continue;
		}

#elif 1
		if (Distance(xyz, vertexHash->xyz) > distance)
		{
			continue;
		}

#else
		if ((Q_fabs(xyz[0] - vertexHash->vcd.xyz[0])) > HASH_XYZ_EPSILON ||
		    (Q_fabs(xyz[1] - vertexHash->vcd.xyz[1])) > HASH_XYZ_EPSILON ||
		    (Q_fabs(xyz[2] - vertexHash->vcd.xyz[2])) > HASH_XYZ_EPSILON)
		{
			continue;
		}
#endif
		return vertexHash;
	}

	return NULL;
}

/**
 * @brief AddVertexToHashTable
 * @param[in,out] hashTable
 * @param[in] xyz
 * @param[in] data
 * @return
 */
vertexHash_t *AddVertexToHashTable(vertexHash_t **hashTable, vec3_t xyz, void *data)
{
	unsigned int hash;
	vertexHash_t *vertexHash;

	if (hashTable == NULL || xyz == NULL)
	{
		return NULL;
	}

	vertexHash = (vertexHash_t *)ri.Hunk_Alloc(sizeof(vertexHash_t), h_low);

	if (!vertexHash)
	{
		return NULL;
	}

	hash = VertexCoordGenerateHash(xyz);

	VectorCopy(xyz, vertexHash->xyz);
	vertexHash->data = data;

	// link into table
	vertexHash->next = hashTable[hash];
	hashTable[hash]  = vertexHash;

	return vertexHash;
}

/**
 * @brief GL_BindNearestCubeMap
 * @param[in] xyz
 */
void GL_BindNearestCubeMap(const vec3_t xyz)
{
#if 1
	int            j;
	float          distance, maxDistance;
	cubemapProbe_t *cubeProbe;

	Ren_LogComment("--- GL_BindNearestCubeMap ---\n");

	maxDistance      = 9999999.0f;
	tr.autoCubeImage = tr.blackCubeImage;
	for (j = 0; j < tr.cubeProbes.currentElements; j++)
	{
		cubeProbe = Com_GrowListElement(&tr.cubeProbes, j);

		distance = Distance(cubeProbe->origin, xyz);
		if (distance < maxDistance)
		{
			tr.autoCubeImage = cubeProbe->cubemap;
			maxDistance      = distance;
		}
	}
#else // cubeProbe hash values
	float          distance, maxDistance;
	cubemapProbe_t *cubeProbe;
	unsigned int   hash;
	vertexHash_t   *vertexHash;

	tr.autoCubeImage = tr.whiteCubeImage;
	if (!r_reflectionMapping->integer)
	{
		return;
	}

	if (tr.cubeHashTable == NULL || xyz == NULL)
	{
		return;
	}

	maxDistance = 9999999.0f;

	hash = VertexCoordGenerateHash(xyz);

	for (vertexHash = tr.cubeHashTable[hash]; vertexHash; vertexHash = vertexHash->next)
	{
		cubeProbe = (cubemapProbe_t *)vertexHash->data;

		distance = Distance(cubeProbe->origin, xyz);
		if (distance < maxDistance)
		{
			tr.autoCubeImage = cubeProbe->cubemap;
			maxDistance      = distance;
		}
	}
#endif

	GL_Bind(tr.autoCubeImage);
}

/**
 * @brief R_FindTwoNearestCubeMaps
 * @param[in] position
 * @param[out] cubeProbe1 // nearest
 * @param[out] cubeProbe2 // 2nd nearest
 */
void R_FindTwoNearestCubeMaps(const vec3_t position, cubemapProbe_t **cubeProbe1, cubemapProbe_t **cubeProbe2, float *distance1, float *distance2)
{
	int            j;
	float          distance;
	cubemapProbe_t *cubeProbe;
#if 0
	unsigned int   hash;
	vertexHash_t   *vertexHash;
#endif
	Ren_LogComment("--- R_FindTwoNearestCubeMaps ---\n");

	*cubeProbe1 = NULL;
	*cubeProbe2 = NULL;
#if 0 // cubeProbe hash values
	if (tr.cubeHashTable == NULL || position == NULL)
#else
	if (tr.cubeProbes.currentElements == 0 || position == NULL)
#endif
	{
		return;
	}
#if 0
	hash        = VertexCoordGenerateHash(position);
#endif
	*distance1 = *distance2 = 9999999.0f;

#if 1
	for (j = 0; j < tr.cubeProbes.currentElements; j++)
	{
		cubeProbe = Com_GrowListElement(&tr.cubeProbes, j);
#else // cubeProbe hash values
	for (j = 0, vertexHash = tr.cubeHashTable[hash]; vertexHash; vertexHash = vertexHash->next, j++)
	{
		cubeProbe = (cubemapProbe_t *)vertexHash->data;
#endif

		distance = Distance(cubeProbe->origin, position);
		if (distance < *distance1)
		{
			*cubeProbe2 = *cubeProbe1;
			*distance2 = *distance1;

			*cubeProbe1 = cubeProbe;
			*distance1 = distance;
		}
		else if (distance < *distance2) // && distance > *distance1)
		{
			*cubeProbe2 = cubeProbe;
			*distance2 = distance;
		}
	}

	//Ren_Print("iterated through %i cubeprobes\n", j);
}

/**
 * @brief This is saving our cube probes to special uncompressed pixeldata tga rgb format
 *
 * 	Optimize: - Remove the alpha channel (25% less data)? or use pixel intensity in the alpha channel?
 *
 * @param[in] filename
 * @param[in] data
 * @param[in] width in pixels
 * @param[in] height in pixels
 */
void R_SaveCubeProbes(const char *filename, byte *pixeldata, int width, int height)
{
	byte *buffer, *src, *dst;
	int  i;
	int  pixeldataBytes = width * height * 4;
	int  fileBytes      = 18 + pixeldataBytes;

	buffer = (byte *)ri.Z_Malloc(fileBytes);
	Com_Memset(buffer, 0, 18);
	buffer[2]  = 2;     // Uncompressed, RGB images
	//buffer[8] = 0 & 255; // X Origin: X coordinate of the lower left corner of the image
	//buffer[9] = 0 >> 8;
	//buffer[10] = (height-1) & 255; // X Origin: X coordinate of the lower left corner of the image
	//buffer[11] = (height-1) >> 8;
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 32;	// Number of bits per pixel
	//buffer[17] = 8;	// number of attribute bits associated with each pixel (alpha uses 8 bits)

	// copy pixel data
	src = pixeldata;
	dst = buffer + 18; // we never write the image identification field, so our dest image only needs to skip the fileheader-length
	for (i = 0 ; i < pixeldataBytes; i += 4, src += 4, dst += 4)
	{
		dst[0] = src[0]; // r
		dst[1] = src[1]; // g
		dst[2] = src[2]; // b
		//dst[3] = src[3]; // a
		dst[3] = 255; // alpha = opaque
	}

	ri.FS_WriteFile(filename, buffer, fileBytes); // this will close the file as well
	ri.Free(buffer);
}

/**
 * @brief This is loading a single cube (6 probes) from our file into cubeTemp.
 *
 * @param[in] number of cube probe
 * @param[in] total cube probes
 * @param[in, out] cubeTemp
 */
qboolean R_LoadCubeProbe(int cubeProbeNum, int totalCubeProbes, byte *cubeTemp[6])
{
	static byte *buffer = NULL; // pointer to the file buffer (including the 18 byte long header)
	byte *pixeldata     = NULL; // the pointer to the actual pixel colors

	int i;
	int totalPos = cubeProbeNum * 6;
	int fileNum = totalPos / REF_CUBEMAPS_PER_FILE;    // divide by howmany images fit in one bigger image on file
	int insidePos = totalPos % REF_CUBEMAPS_PER_FILE;  // the Nth image inside the big texture
	int sidesFree = REF_CUBEMAPS_PER_FILE - insidePos; // current number of images that still can be fit into the current big texture
	int cubeSidesInFile1 = (sidesFree >= 6) ? 6 : sidesFree;
	int cubeSidesInFile2 = 6 - cubeSidesInFile1;
	char *filename;
	int bytesRead;
	static int lastFileNum = -1; // initialize with a value that fileNum will never have

	if (fileNum != lastFileNum)
	{
		lastFileNum = fileNum;
		filename = va("cm/%s/cm_%04d.tga", s_worldData.baseName, fileNum);
		bytesRead = ri.FS_ReadFile(filename, (void **)&buffer); // this lso closes he file after reading the full file into the buffer
		if (bytesRead <= 0)
		{
			//Ren_Print("loadCubeProbes: %s not found", filename);
			ri.FS_FreeFile(buffer);
			buffer = NULL;
			return qfalse;
		}
	}
	pixeldata = buffer + 18; // +buffer[0]; // skip header + skip possibly given image identifier field (usually 0 length)
	for (i = 0; i < cubeSidesInFile1; i++)
	{
		// copy this cube map into buffer
		R_SubImageCpy(pixeldata,
			((insidePos + i) % REF_CUBEMAP_STORE_SIDE) * REF_CUBEMAP_SIZE, ((insidePos + i) / REF_CUBEMAP_STORE_SIDE) * REF_CUBEMAP_SIZE,
			REF_CUBEMAP_STORE_SIZE, REF_CUBEMAP_STORE_SIZE,
			cubeTemp[i],
			REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE,
			4, qfalse);
	}
	if (cubeProbeNum == totalCubeProbes)
	{
		ri.FS_FreeFile(buffer);
		buffer = NULL;
	}

	if (cubeSidesInFile2 > 0)
	{
		if (buffer)
		{
			ri.FS_FreeFile(buffer);
			buffer = NULL;
		}

		lastFileNum = fileNum + 1;
		filename = va("cm/%s/cm_%04d.tga", s_worldData.baseName, fileNum + 1);

		if (ri.FS_FOpenFileRead(filename, NULL, qfalse) <= 0)
		{
			return qfalse;
		}

		bytesRead = ri.FS_ReadFile(filename, (void **)&buffer);

		if (bytesRead <= 0)
		{
			//Ren_Print("loadCubeProbes: %s not found", filename);
			ri.FS_FreeFile(buffer);
			buffer = NULL;
			return qfalse;
		}
		pixeldata = buffer + 18; // skip header
		for (i = 0; i < cubeSidesInFile2; i++)
		{
			// copy this cube map into buffer
			R_SubImageCpy(pixeldata,
							(i % REF_CUBEMAP_STORE_SIDE) * REF_CUBEMAP_SIZE, (i / REF_CUBEMAP_STORE_SIDE) * REF_CUBEMAP_SIZE,
							REF_CUBEMAP_STORE_SIZE, REF_CUBEMAP_STORE_SIZE,
							tr.cubeTemp[cubeSidesInFile1 + i],
							REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE,
							4, qfalse);
		}
		if (cubeProbeNum == totalCubeProbes)
		{
			ri.FS_FreeFile(buffer);
			buffer = NULL;
		}
	}

	return qtrue; // result is true if all the sides of the cube could be loaded,
}

#define CUBES_MINIMUM_DISTANCE 200.0f

/**
 * @brief R_BuildCubeMaps
 */
void R_BuildCubeMaps(void)
{
	int            i, j; // k;
	int            ii, jj;
	refdef_t       rf;
	cubemapProbe_t *cubeProbe;
	//int            x, y, xy; // encode the pixel intensity into the alpha channel
	//byte           *dest;    // encode the pixel intensity into the alpha channel
	//byte   r, g, b, best;    // encode the pixel intensity into the alpha channel

	byte		*pixeldata	= NULL;
	char		*fileName	= NULL;
	int			fileCount	= 0; // the cm_ file numbering
	int			sideX		= 0; // this cube side image is the Nth image from the left (in the bigger texture)
	int			sideY		= 0; // this cube side image is the Nth image from the top (in the bigger texture)
	qboolean	dirtyBuf	= qfalse; // true if there is something in the fileBuf, and the fileBuf has not been written to disk yet.
	qboolean	createCM	= qfalse;

#if 1
	// the "progressbar" is 50 characters long
	// there are n cubeProbes    (n == tr.cubeProbes.currentElements)
	float ticsPerProbe; // 50 / tr.cubeProbes.currentElements;
	int tics = 0; // the current number of tics that have been written
#else
	size_t tics         = 0;
	size_t nextTicCount = 0;
	size_t ticsNeeded;
#endif

#ifdef ETLEGACY_DEBUG
	int endTime, startTime = ri.Milliseconds();
#endif

	if (!r_reflectionMapping->integer)
	{
		return;
	}

	Com_Memset(&rf, 0, sizeof(refdef_t));

	for (i = 0; i < 6; i++)
	{
		tr.cubeTemp[i] = (byte *)ri.Z_Malloc(REF_CUBEMAP_SIZE * REF_CUBEMAP_SIZE * 4);
	}

	// let's see if we have to create cm images again
	fileName = va("cm/%s/cm_0000.tga", s_worldData.baseName);
	if (!ri.FS_FileExists(fileName))
	{
		//pixeldata = ri.Z_Malloc(REF_CUBEMAP_STORE_SIZE * REF_CUBEMAP_STORE_SIZE * 4);
		createCM = qtrue;
		//Ren_Developer("Cubemaps not found!\n");
	}
	//else
	//{
	//	Ren_Developer("Cubemaps found!\n");
	//}

	// the cubeProbes list
	Com_InitGrowList(&tr.cubeProbes, 5000);
#if 0 // cubeProbe hash values
	tr.cubeHashTable = NewVertexHashTable();
#endif

	// calculate origins for our probes
#if 0 // cubeProbe hash values
#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
	if (tr.world->vis)
	{
		bspCluster_t *cluster;

		for (i = 0; i < tr.world->numClusters; i++)
		{
			cluster = &tr.world->clusters[i];

			// check to see if this is a shit location
			if (ri.CM_PointContents(cluster->origin, 0) == CONTENTS_SOLID)
			{
				continue;
			}

			if (FindVertexInHashTable(tr.cubeHashTable, cluster->origin, CUBES_MINIMUM_DISTANCE) == NULL)
			{
				cubeProbe = ri.Hunk_Alloc(sizeof(*cubeProbe), h_high);
				Com_AddToGrowList(&tr.cubeProbes, cubeProbe);

				VectorCopy(cluster->origin, cubeProbe->origin);

				AddVertexToHashTable(tr.cubeHashTable, cubeProbe->origin, cubeProbe);

				//gridPoint = tr.world->lightGridData + pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];

				// TODO connect cubeProbe with gridPoint
			}
		}
	}
#endif
#endif

	if (qtrue) // cubes based on nodes
	{
		bspNode_t *node;
		qboolean  isCubeOK;
		int       p;

		Ren_Print("...trying to allocate %d cubemaps from world nodes\n", tr.world->numnodes);

		// FIXME: this doesn't create cubes on important locations
		//        f.e. oasis (about 2600 cubes in total) water pump near allies spawn
		for (i = 0; i < tr.world->numnodes; i++)
		{
			node = &tr.world->nodes[i];

			// check to see if this is a shit location
			if (node->contents == CONTENTS_NODE)
			{
				continue;
			}

			if (node->area == -1)
			{
				// location is in the void
				continue;
			}

			// We don't want the cubeProbes to be too close to eachother
	#if 0 // cubeProbe hash values
			if (FindVertexInHashTable(tr.cubeHashTable, node->origin, CUBES_MINIMUM_DISTANCE) == NULL)
			{
	#else
				isCubeOK = qtrue;

			for (p = 0; p < tr.cubeProbes.currentElements; p++)
			{
				cubeProbe = (cubemapProbe_t *)Com_GrowListElement(&tr.cubeProbes, p);

				if (Distance(node->origin, cubeProbe->origin) > CUBES_MINIMUM_DISTANCE)
				{
				continue;
				}

				isCubeOK = qfalse;
				break;
			}

			if (isCubeOK)
			{
	#endif
				// create a new cubeProbe
				cubeProbe = (cubemapProbe_t *)ri.Hunk_Alloc(sizeof(*cubeProbe), h_high);
				Com_AddToGrowList(&tr.cubeProbes, cubeProbe);
				VectorCopy(node->origin, cubeProbe->origin);
	#if 0 // cubeProbe hash values
				AddVertexToHashTable(tr.cubeHashTable, cubeProbe->origin, cubeProbe);
	#endif
			}
		}
	}
	else // cubes based on lightgrid
	{
		int            numGridPoints, k;
		//bspGridPoint_t *gridPoint;
		//int            gridStep[3];
		float          posFloat[3];

		//gridStep[0] = 1;
		//gridStep[1] = tr.world->lightGridBounds[0];
		//gridStep[2] = tr.world->lightGridBounds[0] * tr.world->lightGridBounds[1];

		numGridPoints = tr.world->lightGridBounds[0] * tr.world->lightGridBounds[1] * tr.world->lightGridBounds[2];

		Ren_Print("...trying to allocate %d cubemaps", numGridPoints);
		Ren_Print(" with gridsize (%i %i %i)", (int)tr.world->lightGridSize[0], (int)tr.world->lightGridSize[1], (int)tr.world->lightGridSize[2]);
		Ren_Print(" and gridbounds (%i %i %i)\n", (int)tr.world->lightGridBounds[0], (int)tr.world->lightGridBounds[1], (int)tr.world->lightGridBounds[2]);

		// FIXME: don't use every grid position
		//        this is creating about 60000 cubes on oasis with about 500MB data per map!
		for (i = 0; i < tr.world->lightGridBounds[0]; i += 1)
		{
			for (j = 0; j < tr.world->lightGridBounds[1]; j += 1)
			{
				for (k = 0; k < tr.world->lightGridBounds[2]; k += 1)
				{
					posFloat[0] = i * tr.world->lightGridSize[0];
					posFloat[1] = j * tr.world->lightGridSize[1];
					posFloat[2] = k * tr.world->lightGridSize[2];

					VectorAdd(posFloat, tr.world->lightGridOrigin, posFloat);

					// check to see if this is a shit location
					if (ri.CM_PointContents(posFloat, 0) == CONTENTS_SOLID)
					{
						continue;
					}

					if (FindVertexInHashTable(tr.cubeHashTable, posFloat, CUBES_MINIMUM_DISTANCE) == NULL)
					{
						cubeProbe = ri.Hunk_Alloc(sizeof(*cubeProbe), h_high);
						Com_AddToGrowList(&tr.cubeProbes, cubeProbe);

						VectorCopy(posFloat, cubeProbe->origin);

						AddVertexToHashTable(tr.cubeHashTable, posFloat, cubeProbe);

						//gridPoint = tr.world->lightGridData + pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];

						// TODO connect cubeProbe with gridPoint
					}
				}
			}
		}
	}


	if (tr.cubeProbes.currentElements == 0)
	{
#if 1
		// if we could not even create one cubeProbe, we're done.. (don't fake one)
		return;
#else
		// fake one
		cubeProbe = (cubemapProbe_t *)ri.Hunk_Alloc(sizeof(*cubeProbe), h_low);
		Com_AddToGrowList(&tr.cubeProbes, cubeProbe);

		VectorClear(cubeProbe->origin);
#endif
	}

#if 1
	ticsPerProbe = 50.f / (float)tr.cubeProbes.currentElements; // currentElements is != 0 for sure
#endif
	Ren_Print("...creating %d cubemaps\n", tr.cubeProbes.currentElements);
	ri.Cvar_Set("viewlog", "1");
	Ren_Print("0%%  10   20   30   40   50   60   70   80   90   100%%\n");
	Ren_Print("|----|----|----|----|----|----|----|----|----|----|\n ");
	for (j = 0; j < tr.cubeProbes.currentElements; j++)
	{
		cubeProbe = (cubemapProbe_t *)Com_GrowListElement(&tr.cubeProbes, j);

		//Ren_Print("rendering cubemap at (%i %i %i)\n", (int)cubeProbe->origin[0], (int)cubeProbe->origin[1],
		//		  (int)cubeProbe->origin[2]);
#if 1
		// we are at probe j
		int currentTics = (int)(j * ticsPerProbe); // explicit typecast from float to int. This will floor() the result (which is what we want)

		if (currentTics != tics)
		{
			tics = currentTics;
			Ren_Print("*");
			Ren_UpdateScreen();
		}
#else
		if ((j + 1) >= nextTicCount)
		{
			ticsNeeded = (size_t)(((double)(j + 1) / tr.cubeProbes.currentElements) * 50.0);

			do
			{
				Ren_Print("*");
				// FIXME: updating screen doesn't work properly, R_BuildCubeMaps during map load causes eye cancer (on widescreen only)
				Ren_UpdateScreen();
			}
			while (++tics < ticsNeeded);

			nextTicCount = (size_t)((tics / 50.0f) * tr.cubeProbes.currentElements); // loading screen doesn't do 20fps?!
			if ((j + 1) == tr.cubeProbes.currentElements)
			{
				if (tics < 51)
				{
					Ren_Print("*");
				}
				Ren_Print("\n");
			}
		}
#endif
		// Load the cubemap from file if possible, else render a new cubemap
		if (!createCM)
		{
			// try to load the cubemap from file
			// when loading failed, set a flag to render a new cubemap
			createCM = !R_LoadCubeProbe(j, tr.cubeProbes.currentElements, tr.cubeTemp);
		}

		if (createCM)
		{
			if (!pixeldata)
			{
                                pixeldata = ri.Z_Malloc(REF_CUBEMAP_STORE_SIZE * REF_CUBEMAP_STORE_SIZE * 4);
			}

			// render the cubemap
			VectorCopy(cubeProbe->origin, rf.vieworg);

			AxisClear(rf.viewaxis);

			rf.fov_x  = 90;
			rf.fov_y  = 90;
			rf.x      = 0;
			rf.y      = 0;
			rf.width  = REF_CUBEMAP_SIZE;
			rf.height = REF_CUBEMAP_SIZE;
			rf.time   = 0;

			rf.rdflags = RDF_NOCUBEMAP | RDF_NOBLOOM;

			for (i = 0; i < 6; i++)
			{
				switch (i)
				{
				case 0:
				{
					// X-
					rf.viewaxis[0][0] = -1;
					rf.viewaxis[0][1] = 0;
					rf.viewaxis[0][2] = 0;

					rf.viewaxis[1][0] = 0;
					rf.viewaxis[1][1] = 0;
					rf.viewaxis[1][2] = 1;

					rf.viewaxis[2][0] = 0;
					rf.viewaxis[2][1] = 1;
					rf.viewaxis[2][2] = 0;
					break;
				}
				case 1:
				{
					// X+
					rf.viewaxis[0][0] = 1;
					rf.viewaxis[0][1] = 0;
					rf.viewaxis[0][2] = 0;

					rf.viewaxis[1][0] = 0;
					rf.viewaxis[1][1] = 0;
					rf.viewaxis[1][2] = -1;

					rf.viewaxis[2][0] = 0;
					rf.viewaxis[2][1] = 1;
					rf.viewaxis[2][2] = 0;

					break;
				}
				case 2:
				{
					// Y+
					rf.viewaxis[0][0] = 0;
					rf.viewaxis[0][1] = -1;
					rf.viewaxis[0][2] = 0;

					rf.viewaxis[1][0] = 1;
					rf.viewaxis[1][1] = 0;
					rf.viewaxis[1][2] = 0;

					rf.viewaxis[2][0] = 0;
					rf.viewaxis[2][1] = 0;
					rf.viewaxis[2][2] = 1;
					break;
				}
				case 3:
				{
					// Y-
					rf.viewaxis[0][0] = 0;
					rf.viewaxis[0][1] = 1;
					rf.viewaxis[0][2] = 0;

					rf.viewaxis[1][0] = 1;
					rf.viewaxis[1][1] = 0;
					rf.viewaxis[1][2] = 0;

					rf.viewaxis[2][0] = 0;
					rf.viewaxis[2][1] = 0;
					rf.viewaxis[2][2] = -1;
					break;
				}
				case 4:
				{
					// Z up
					rf.viewaxis[0][0] = 0;
					rf.viewaxis[0][1] = 0;
					rf.viewaxis[0][2] = 1;

					rf.viewaxis[1][0] = 1;
					rf.viewaxis[1][1] = 0;
					rf.viewaxis[1][2] = 0;

					rf.viewaxis[2][0] = 0;
					rf.viewaxis[2][1] = 1;
					rf.viewaxis[2][2] = 0;

					break;
				}
				case 5:
				{
					// Z down
					rf.viewaxis[0][0] = 0;
					rf.viewaxis[0][1] = 0;
					rf.viewaxis[0][2] = -1;

					rf.viewaxis[1][0] = -1;
					rf.viewaxis[1][1] = 0;
					rf.viewaxis[1][2] = 0;

					rf.viewaxis[2][0] = 0;
					rf.viewaxis[2][1] = 1;
					rf.viewaxis[2][2] = 0;
					break;
				}
				}

				tr.refdef.pixelTarget = tr.cubeTemp[i];
				tr.refdef.pixelTargetWidth  = REF_CUBEMAP_SIZE;
				tr.refdef.pixelTargetHeight = REF_CUBEMAP_SIZE;
				Com_Memset(tr.cubeTemp[i], 255, REF_CUBEMAP_SIZE * REF_CUBEMAP_SIZE * 4);

				RE_BeginFrame();
				RE_RenderScene(&rf);
				RE_EndFrame(&ii, &jj);

#if 0 // encode the pixel intensity into the alpha channel, saves work in the shader
				//if (qtrue)
				{
					dest = tr.cubeTemp[i];
					for (y = 0; y < REF_CUBEMAP_SIZE; y++)
					{
						for (x = 0; x < REF_CUBEMAP_SIZE; x++)
						{
							xy = ((y * REF_CUBEMAP_SIZE) + x) * 4;

							r = dest[xy + 0];
							g = dest[xy + 1];
							b = dest[xy + 2];

							if ((r > g) && (r > b))
							{
								best = r;
							}
							else if ((g > r) && (g > b))
							{
								best = g;
							}
							else
							{
								best = b;
							}

							dest[xy + 3] = best;
						}
					}
				}
#endif

				// collate cubemaps into one large image and write it out

				// Copy this cube map into buffer
				R_SubImageCpy(pixeldata,
								sideX * REF_CUBEMAP_SIZE, sideY * REF_CUBEMAP_SIZE,
								REF_CUBEMAP_STORE_SIZE, REF_CUBEMAP_STORE_SIZE,
								tr.cubeTemp[i],
								REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE,
								4, qtrue);

				dirtyBuf = qtrue;
				// Increment counters, and write file if it's full
				sideX++;
				if (sideX >= REF_CUBEMAP_STORE_SIDE)
				{
					sideX = 0;
					sideY++;
					if (sideY >= REF_CUBEMAP_STORE_SIDE)
					{
						sideY = 0;
						// File is full, write it
						fileName = va("cm/%s/cm_%04d.tga", s_worldData.baseName, fileCount);
						// provide a pointer to the pixeldata (not to the start of the header)
						R_SaveCubeProbes(fileName, pixeldata, REF_CUBEMAP_STORE_SIZE, REF_CUBEMAP_STORE_SIZE);

						fileCount++;
						dirtyBuf = qfalse;
					}
				}
			}
		}

		// build the cubemap
		cubeProbe->cubemap = R_AllocImage(va("_autoCube%d", j), qfalse);
		if (!cubeProbe->cubemap)
		{
			Ren_Print("R_BuildCubeMaps: Aborted - can't allocate image.\n");
			return;
		}

		cubeProbe->cubemap->type = GL_TEXTURE_CUBE_MAP_ARB;

		cubeProbe->cubemap->width  = REF_CUBEMAP_SIZE;
		cubeProbe->cubemap->height = REF_CUBEMAP_SIZE;

		cubeProbe->cubemap->bits       = IF_NOPICMIP;
		cubeProbe->cubemap->filterType = FT_LINEAR;
		cubeProbe->cubemap->wrapType   = WT_EDGE_CLAMP;

		GL_Bind(cubeProbe->cubemap);

		R_UploadImage((const byte **)tr.cubeTemp, 6, cubeProbe->cubemap);

		glBindTexture(cubeProbe->cubemap->type, 0);
	}
	Ren_Print("\n");

	if (createCM)
	{
		// write buffer if theres any still unwritten
		if (dirtyBuf)
		{
			fileName = va("cm/%s/cm_%04d.tga", s_worldData.baseName, fileCount);
			//Ren_Print("writing %s\n", fileName);
			R_SaveCubeProbes(fileName, pixeldata, REF_CUBEMAP_STORE_SIZE, REF_CUBEMAP_STORE_SIZE);
		}

		Ren_Print("Wrote %d cubemaps in %d files.\n", j, fileCount + 1);
	}
	else
	{
		Ren_Print("Read %d cubemaps from files.\n", tr.cubeProbes.currentElements -1);
	}

	if (pixeldata)
	{
		ri.Free(pixeldata);
	}

	// turn pixel targets off
	tr.refdef.pixelTarget = NULL;

	// assign the surfs a cubemap
	// (that is done in R_CreateWorldVBO())
#if 0
	for (i = 0; i < tr.world->numnodes; i++)
	{
		bspSurface_t **mark; //	msurface_t **mark;
		bspSurface_t *surf;  // msurface_t *surf;

		if (tr.world->nodes[i].contents != CONTENTS_SOLID)
		{
			mark = tr.world->nodes[i].markSurfaces; // .firstmarksurface;
			j    = tr.world->nodes[i].numMarkSurfaces; // .nummarksurfaces;
			while (j--)
			{
				int dist = 9999999;
				int best = 0;

				surf = *mark;
				mark++;
				sv = (void *)surf->data;
				if (sv->surfaceType != SF_STATIC)
				{
					continue;   //
				}
				if (sv->numIndices == 0 || sv->numVerts == 0)
				{
					continue;
				}
				if (sv->cubemap != NULL)
				{
					continue;
				}
//R_FindTwoNearestCubeMaps
				for (x = 0; x < tr.cubeProbes.currentElements; x++)
				{
					vec3_t pos;

					pos[0] = tr.cubeProbes[x].origin[0] - sv->origin[0];
					pos[1] = tr.cubeProbes[x].origin[1] - sv->origin[1];
					pos[2] = tr.cubeProbes[x].origin[2] - sv->origin[2];

					distance = VectorLength(pos);
					if (distance < dist)
					{
						dist = distance;
						best = x;
					}
				}
				sv->cubemap = tr.cubeProbes[best].cubemap;
			}
		}
	}
#endif

#ifdef ETLEGACY_DEBUG
	endTime = ri.Milliseconds();
	Ren_Developer("cubemap probes pre-rendering time of %i cubes = %5.2f seconds\n", tr.cubeProbes.currentElements,
	              (endTime - startTime) / 1000.0);
#endif
}

/**
 * @brief Called directly from cgame
 * @param[in] name
 */
void RE_LoadWorldMap(const char *name)
{
	int       i;
	dheader_t *header;
	byte      *buffer;
	byte      *startMarker;

	if (tr.worldMapLoaded)
	{
		Ren_Drop("ERROR: attempted to redundantly load world map\n");
	}

	Ren_Print("----- RE_LoadWorldMap( %s ) -----\n", name);

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45f;
	tr.sunDirection[1] = 0.3f;
	tr.sunDirection[2] = 0.9f;

	VectorNormalize(tr.sunDirection);

	// invalidate fogs (likely to be re-initialized to new values by the current map)
	// TODO: this is sort of silly.  I'm going to do a general cleanup on fog stuff
	//          now that I can see how it's been used.  (functionality can narrow since
	//          it's not used as much as it's designed for.)

	RE_SetFog(FOG_SKY, 0, 0, 0, 0, 0, 0);
	RE_SetFog(FOG_PORTALVIEW, 0, 0, 0, 0, 0, 0);
	RE_SetFog(FOG_HUD, 0, 0, 0, 0, 0, 0);
	RE_SetFog(FOG_MAP, 0, 0, 0, 0, 0, 0);
	RE_SetFog(FOG_CURRENT, 0, 0, 0, 0, 0, 0);
	RE_SetFog(FOG_TARGET, 0, 0, 0, 0, 0, 0);
	RE_SetFog(FOG_WATER, 0, 0, 0, 0, 0, 0);
	RE_SetFog(FOG_SERVER, 0, 0, 0, 0, 0, 0);

	tr.glfogNum = (glfogType_t)0;

	VectorCopy(colorMdGrey, tr.fogColor);
	tr.fogDensity = 0;

	// set default ambient color
	tr.worldEntity.ambientLight[0] = r_ambientScale->value;
	tr.worldEntity.ambientLight[1] = r_ambientScale->value;
	tr.worldEntity.ambientLight[2] = r_ambientScale->value;

	tr.worldMapLoaded = qtrue;

	// load it
	ri.FS_ReadFile(name, (void **)&buffer);
	if (!buffer)
	{
		Ren_Drop("RE_LoadWorldMap: %s not found", name);
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	// tr.worldDeluxeMapping will be set by R_LoadEntities()
	//tr.worldDeluxeMapping = qfalse;
	tr.worldHDR_RGBE      = qfalse;

	Com_Memset(&s_worldData, 0, sizeof(s_worldData));
	Q_strncpyz(s_worldData.name, name, sizeof(s_worldData.name));

	Q_strncpyz(s_worldData.baseName, COM_SkipPath(s_worldData.name), sizeof(s_worldData.name));
	COM_StripExtension(s_worldData.baseName, s_worldData.baseName, sizeof(s_worldData.baseName));

	startMarker = (byte *)ri.Hunk_Alloc(0, h_low);

	header   = (dheader_t *) buffer;
	fileBase = (byte *) header;

	i = LittleLong(header->version);
	if (i != BSP_VERSION && i != BSP_VERSION_Q3)
	{
		Ren_Drop("RE_LoadWorldMap: %s has wrong version number (%i should be %i for ET or %i for Q3)",
		         name, i, BSP_VERSION, BSP_VERSION_Q3);
	}

	// swap all the lumps
	for (i = 0; i < sizeof(dheader_t) / 4; i++)
	{
		((int *)header)[i] = LittleLong(((int *)header)[i]);
	}

	// load into heap
	Ren_UpdateScreen();
	R_LoadEntities(&header->lumps[LUMP_ENTITIES]);

	Ren_UpdateScreen();
	R_LoadShaders(&header->lumps[LUMP_SHADERS]);

	Ren_UpdateScreen();
	R_LoadLightmaps(&header->lumps[LUMP_LIGHTMAPS], name);

	Ren_UpdateScreen();
	R_LoadPlanes(&header->lumps[LUMP_PLANES]);

	Ren_UpdateScreen();
	R_LoadSurfaces(&header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES]);

	Ren_UpdateScreen();
	R_LoadMarksurfaces(&header->lumps[LUMP_LEAFSURFACES]);

	Ren_UpdateScreen();
	R_LoadNodesAndLeafs(&header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS]);

	Ren_UpdateScreen();
	R_LoadSubmodels(&header->lumps[LUMP_MODELS]);

	// moved fog lump loading here, so fogs can be tagged with a model num
	Ren_UpdateScreen();
	R_LoadFogs(&header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES]);

	Ren_UpdateScreen();
	R_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);

	Ren_UpdateScreen();
	R_LoadLightGrid(&header->lumps[LUMP_LIGHTGRID]);

	// create static VBOS from the world
	R_CreateWorldVBO();
	R_CreateClusters();
	R_CreateSubModelVBOs();
/*
	{
		// sun light is already init see resetRefLight() in R_LoadLights
		trRefLight_t *light = &s_worldData.lights[0];

		light->shader   = tr.sunShader;
		light->l.rlType = RL_DIRECTIONAL;

		light->l.origin[0] = 1;
		light->l.origin[1] = 1;
		light->l.origin[2] = 2000;

		light->l.projTarget[0] = 1;
		light->l.projTarget[1] = 1;
		light->l.projTarget[2] = 1;

		light->l.color[0] = tr.sunLight[0];
		light->l.color[1] = tr.sunLight[1];
		light->l.color[2] = tr.sunLight[2];

		light->l.scale = r_lightScale->value * 0.2;

		light->direction[0] = tr.sunDirection[0];
		light->direction[1] = tr.sunDirection[1];
		light->direction[2] = tr.sunDirection[2];

		light->l.radius[0] = 8182;
		light->l.radius[1] = 8182;
		light->l.radius[2] = 8182;

		light->l.inverseShadows = qfalse; // must be false
		light->isStatic         = qfalse; // must be false to render alpha-masked surfaces
	}
*/

	// we precache interactions between lights and surfaces
	// to reduce the polygon count
	R_PrecacheInteractions();

	s_worldData.dataSize = (byte *) ri.Hunk_Alloc(0, h_low) - startMarker;

	//Ren_Print("total world data size: %d.%02d MB\n", s_worldData.dataSize / (1024 * 1024),
	//(s_worldData.dataSize % (1024 * 1024)) * 100 / (1024 * 1024));

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	tr.world->hasSkyboxPortal = qfalse;

	// reset fog to world fog (if present)
	RE_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 20, 0, 0, 0, 0);

	// make sure the VBO glState entries are save
	R_BindNullVBO();
	R_BindNullIBO();

	// set the sun shader if there is one
	if (tr.sunShaderName)
	{
		tr.sunShader = R_FindShader(tr.sunShaderName, SHADER_3D_STATIC, qfalse);
	}
	else
	{
		tr.sunShader = 0;   // clear sunshader so it's not there if the level doesn't specify it
	}

	// build cubemaps after the necessary vbo stuff is done
	// FIXME: causes missing vbo error on radar (maps with portal sky or foliage )
	// devmap oasis; set developer 1; set r_showcubeprobs 1
	//
	R_BuildCubeMaps();

	// never move this to RE_BeginFrame because we need it to set it here for the first frame
	// but we need the information across 2 frames
	ClearLink(&tr.traversalStack);
	ClearLink(&tr.occlusionQueryQueue);
	ClearLink(&tr.occlusionQueryList);

	ri.FS_FreeFile(buffer);
}
