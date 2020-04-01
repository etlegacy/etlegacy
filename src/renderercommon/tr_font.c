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
 * @file tr_font.c
 * @brief Handles ingame fonts and can also generate new font files
 *
 * The fonts are pre-rendered using [FreeType], then the glyph data are saved
 * and then hand touched so the font bitmaps scale a bit better in GL.
 *
 * In the UI Scripting code, a scale of 1.0 is equal to a 48 point font. In Team
 * Arena, we use three or four scales, most of them exactly equaling the specific
 * rendered size. We rendered three sizes in Team Arena, 12, 16, and 20.
 *
 * How to generate new font data files
 * ===================================
 *
 * 1. Compile ET: Legacy with Freetype support by enabling FEATURE_FREETYPE in CMake
 * 2. Delete the fontname_x_xx.tga files and fontname_xx.dat files from the fonts
 *    path.
 * 3. In a ui script, specify a font, smallFont, and bigFont keyword with font
 *    name and point size.
 *    The new TrueType fonts must exist in the etmain directory at this point.
 * 4. Run the game with +set r_saveFontData 1.
 *    NOTE: you must specify r_saveFontData before the game is started.
 *    Setting it ingame is pointless as it will get reset to 0 when you restart
 *    the game.
 * 5. Exit the game and there will be three dat files and at least three tga files.
 *    The tga's are in 256x256 pages so if it takes three images to render
 *    a 24 point font. You will end up with fontname_0_24.tga through fontname_2_24.tga
 * 6. In future runs of the game, the system looks for these images and data
 *    files when a specific point sized font is rendered and loads them for use.
 *    Because of the original beta nature of the FreeType code you will probably
 *    want to hand touch the font bitmaps.
 *
 * [FreeType]:	http://www.freetype.org/	"FreeType"
 */

#ifdef FEATURE_RENDERER2
#include "../renderer2/tr_local.h"
#else
#include "../renderer/tr_local.h"
#endif

#include "../qcommon/qcommon.h"

#ifdef FEATURE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ERRORS_H
#include FT_SYSTEM_H
#include FT_IMAGE_H
#include FT_OUTLINE_H
#include FT_LCD_FILTER_H

#define FONT_SIZE 256
#define DPI 72

#define _FLOOR(x)  ((x) & - 64)
#define _CEIL(x)   (((x) + 63) & - 64)
#define _TRUNC(x)  ((x) >> 6)

FT_Library ftLibrary = NULL;

const char *supportedFormats[] = { "ttf", "otf" };
const int  formatCount         = ARRAY_LEN(supportedFormats);

#endif

#define MAX_FONTS 16
static int              registeredFontCount = 0;
static fontInfo_extra_t registeredFont[MAX_FONTS];

#ifdef FEATURE_FREETYPE
/**
 * @brief R_GetGlyphInfo
 * @param[in] glyph
 * @param[out] left
 * @param[out] right
 * @param[out] width
 * @param[out] top
 * @param[out] bottom
 * @param[out] height
 * @param[out] pitch
 */
void R_GetGlyphInfo(FT_GlyphSlot glyph, int *left, int *right, int *width, int *top, int *bottom, int *height, int *pitch)
{
	*left  = _FLOOR(glyph->metrics.horiBearingX - 1);
	*right = _CEIL(glyph->metrics.horiBearingX + glyph->metrics.width + 1);
	*width = _TRUNC(*right - *left);

	*top    = _CEIL(glyph->metrics.horiBearingY + 1);
	*bottom = _FLOOR(glyph->metrics.horiBearingY - glyph->metrics.height - 1);
	*height = _TRUNC(*top - *bottom);
	*pitch  = (*width + 3) & - 4;
}

/**
 * @brief R_RenderGlyph
 * @param[in] glyph
 * @param[out] glyphOut
 * @return
 */
FT_Bitmap *R_RenderGlyph(FT_GlyphSlot glyph, glyphInfo_t *glyphOut)
{
	FT_Bitmap *bit2;
	int       left, right, width, top, bottom, height, pitch, size;

	R_GetGlyphInfo(glyph, &left, &right, &width, &top, &bottom, &height, &pitch);

	if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
	{
		Ren_Print("Non-outline fonts are not supported\n");
		return NULL;
	}

	size = pitch * height;

	bit2 = (FT_Bitmap *)ri.Z_Malloc(sizeof(FT_Bitmap));

	bit2->width      = width;
	bit2->rows       = height;
	bit2->pitch      = pitch;
	bit2->pixel_mode = FT_PIXEL_MODE_GRAY;
	bit2->buffer     = (unsigned char *)ri.Z_Malloc(size);
	bit2->num_grays  = 256;

	Com_Memset(bit2->buffer, 0, size);

	FT_Outline_Translate(&glyph->outline, -left, -bottom);
	FT_Outline_Get_Bitmap(ftLibrary, &glyph->outline, bit2);

	glyphOut->height = height;
	glyphOut->pitch  = pitch;
	glyphOut->top    = _TRUNC(glyph->metrics.horiBearingY) + 1;
	glyphOut->bottom = bottom;
	glyphOut->xSkip  = _TRUNC(glyph->metrics.horiAdvance) + 1;
	return bit2;
}

/**
 * @brief RE_ConstructGlyphInfo
 * @param[in] imageSize
 * @param[in] imageOut
 * @param[in,out] xOut
 * @param[in,out] yOut
 * @param[in,out] maxHeight
 * @param[in] face
 * @param[in] codepoint
 * @param[in] calcHeight
 * @return
 */
static glyphInfo_t *RE_ConstructGlyphInfo(int imageSize, unsigned char *imageOut, int *xOut, int *yOut, int *maxHeight, FT_Face face, unsigned long codepoint, qboolean calcHeight)
{
	static glyphInfo_t glyph;
	unsigned char      *src, *dst;
	FT_Bitmap          *bitmap = NULL;
	FT_Int32           flags   = FT_LOAD_DEFAULT;

	Com_Memset(&glyph, 0, sizeof(glyphInfo_t));
	// make sure everything is here
	if (face != NULL)
	{
		FT_UInt  index = FT_Get_Char_Index(face, codepoint);
		FT_Error rc;
		float    scaled_width, scaled_height;
		int      i;

		if (index == 0)
		{
			return &glyph; // nothing to render
		}

		flags |= FT_LOAD_FORCE_AUTOHINT;

		// Test filtering
		FT_Library_SetLcdFilter(ftLibrary, FT_LCD_FILTER_LIGHT);
		flags |= FT_LOAD_TARGET_LCD;

		rc = FT_Load_Glyph(face, index, flags);

		if (rc)
		{
			Ren_Warning("RE_ConstructGlyphInfo: cannot load a glyph into the glyph slot of a face object.\n");
			return &glyph;         // nothing to render
		}

		bitmap = R_RenderGlyph(face->glyph, &glyph);
		if (!bitmap)
		{
			return &glyph;
		}

		if (glyph.height > *maxHeight)
		{
			*maxHeight = glyph.height;
		}

		if (calcHeight)
		{
			ri.Free(bitmap->buffer);
			ri.Free(bitmap);
			return &glyph;
		}

		scaled_width  = glyph.pitch;
		scaled_height = glyph.height;

		// we need to make sure we fit
		if (*xOut + scaled_width + 1 >= imageSize - 1)
		{
			*xOut  = 0;
			*yOut += *maxHeight + 1;
		}

		if (*yOut + *maxHeight + 1 >= imageSize - 1)
		{
			*yOut = -1;
			*xOut = -1;
			ri.Free(bitmap->buffer);
			ri.Free(bitmap);
			return &glyph;
		}

		src = bitmap->buffer;
		dst = imageOut + (*yOut * imageSize) + *xOut;

		if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO)
		{
			for (i = 0; i < glyph.height; i++)
			{
				int           j;
				unsigned char *_src = src;
				unsigned char *_dst = dst;
				unsigned char mask  = 0x80;
				unsigned char val   = *_src;

				for (j = 0; j < glyph.pitch; j++)
				{
					if (mask == 0x80)
					{
						val = *_src++;
					}
					if (val & mask)
					{
						*_dst = 0xff;
					}
					mask >>= 1;

					if (mask == 0)
					{
						mask = 0x80;
					}
					_dst++;
				}

				src += glyph.pitch;
				dst += imageSize;
			}
		}
		else
		{
			for (i = 0; i < glyph.height; i++)
			{
				Com_Memcpy(dst, src, glyph.pitch);
				src += glyph.pitch;
				dst += imageSize;
			}
		}

		// we now have an 8 bit per pixel grey scale bitmap
		// that is width wide and pf->ftSize->metrics.y_ppem tall

		glyph.imageHeight = scaled_height;
		glyph.imageWidth  = scaled_width;
		glyph.s           = (float)*xOut / imageSize;
		glyph.t           = (float)*yOut / imageSize;
		glyph.s2          = glyph.s + (float)scaled_width / imageSize;
		glyph.t2          = glyph.t + (float)scaled_height / imageSize;

		// ET uses pitch as a horizontal BearingX so we need to change this at this point for the font to be usable in game
		// Super stupid btw
		glyph.pitch = _TRUNC(face->glyph->metrics.horiBearingX);

		*xOut += scaled_width + 1;
	}

	if (bitmap && bitmap->buffer)
	{
		ri.Free(bitmap->buffer);
	}

	if (bitmap)
	{
		ri.Free(bitmap);
	}

	return &glyph;
}
#endif

static int  fdOffset;
static byte *fdFile;

/**
 * @brief readInt
 * @return
 */
int readInt(void)
{
	int i = ((unsigned int)fdFile[fdOffset] | ((unsigned int)fdFile[fdOffset + 1] << 8) | ((unsigned int)fdFile[fdOffset + 2] << 16) | ((unsigned int)fdFile[fdOffset + 3] << 24));

	fdOffset += 4;
	return i;
}

typedef union
{
	byte fred[4];
	float ffred;
} poor;

/**
 * @brief readFloat
 * @return
 */
float readFloat(void)
{
	poor me;
#if defined Q3_BIG_ENDIAN
	me.fred[0] = fdFile[fdOffset + 3];
	me.fred[1] = fdFile[fdOffset + 2];
	me.fred[2] = fdFile[fdOffset + 1];
	me.fred[3] = fdFile[fdOffset + 0];
#elif defined Q3_LITTLE_ENDIAN
	me.fred[0] = fdFile[fdOffset + 0];
	me.fred[1] = fdFile[fdOffset + 1];
	me.fred[2] = fdFile[fdOffset + 2];
	me.fred[3] = fdFile[fdOffset + 3];
#endif
	fdOffset += 4;
	return me.ffred;
}

#define GLYPH_OLD_FORMAT 20548 //!< sizeof(fontInfo_t) with 256 chars that 2.60b used

/**
 * @brief Loads ASCII texture font compatible with ET version 2.60b
 * @param[in] datName
 * @param[out] font
 * @return
 */
qboolean R_LoadPreRenderedFont(const char *datName, fontInfo_t *font)
{
	int len, i;

	// Check if this font isn't loaded already
	for (i = 0; i < registeredFontCount; i++)
	{
		if (Q_stricmp(datName, registeredFont[i].datName) == 0)
		{
			Com_Memcpy(font, &registeredFont[i], GLYPH_OLD_FORMAT);
			return qtrue;
		}
	}

	len = ri.FS_ReadFile(datName, NULL);
	if (len == GLYPH_OLD_FORMAT)
	{
		unsigned char *faceData;

		ri.FS_ReadFile(datName, (void **)&faceData);
		fdOffset = 0;
		fdFile   = faceData;
		for (i = 0; i < GLYPHS_ASCII_PER_FONT; i++)
		{
			font->glyphs[i].height      = readInt();
			font->glyphs[i].top         = readInt();
			font->glyphs[i].bottom      = readInt();
			font->glyphs[i].pitch       = readInt();
			font->glyphs[i].xSkip       = readInt();
			font->glyphs[i].imageWidth  = readInt();
			font->glyphs[i].imageHeight = readInt();
			font->glyphs[i].s           = readFloat();
			font->glyphs[i].t           = readFloat();
			font->glyphs[i].s2          = readFloat();
			font->glyphs[i].t2          = readFloat();
			font->glyphs[i].glyph       = readInt();
			Q_strncpyz(font->glyphs[i].shaderName, (const char *)&fdFile[fdOffset], sizeof(font->glyphs[i].shaderName));
			fdOffset += sizeof(font->glyphs[i].shaderName);
		}
		font->glyphScale = readFloat();

		Com_Memcpy(font->datName, datName, sizeof(font->datName));

		for (i = GLYPH_START; i <= GLYPH_ASCII_END; i++) // limit to 256 glyphs
		{
			font->glyphs[i].glyph = RE_RegisterShaderNoMip(font->glyphs[i].shaderName);
		}
		Com_Memcpy(&registeredFont[registeredFontCount++], font, GLYPH_OLD_FORMAT);
		ri.FS_FreeFile(faceData);
		return qtrue;
	}
	else if (len <= 0)
	{
		Ren_Warning("R_LoadPreRenderedFont: font file '%s' was not found.\n", datName);
	}
	else
	{
		Ren_Warning("R_LoadPreRenderedFont: font file '%s' is in an incompatible format.\n", datName);
	}

	return qfalse;
}

#ifdef FEATURE_FREETYPE
/**
 * @brief Loads a unicode TrueType or OpenType font
 * @param[in] fontName
 * @param[in] pointSize
 * @param[out] font
 * @param[in] extended
 * @return
 */
static qboolean R_LoadScalableFont(const char *fontName, int pointSize, fontInfo_t *font, qboolean extended)
{
	FT_Face       face;
	int           j, k, xOut, yOut, lastStart, imageNumber, len = 0;
	int           scaledSize, newSize, maxHeight, left;
	unsigned char *out, *imageBuff;
	glyphInfo_t   *glyph = NULL;
	image_t       *image;
	qhandle_t     h;
	float         max;
	float         glyphScale;
	unsigned char *faceData;
	unsigned long i = 0;
	char          name[1024];
	qboolean      formatFound = qfalse;
	int           imageSize;
	float         dpi;

	if (ftLibrary == NULL)
	{
		Ren_Warning("R_LoadScalableFont: FreeType not initialized.\n");
		return qfalse;
	}

	while (i < formatCount)
	{
		Com_sprintf(name, sizeof(name), "fonts/%s.%s", fontName, supportedFormats[i]);
		if (ri.FS_FOpenFileRead(name, NULL, qfalse) > 0)
		{
			formatFound = qtrue;
			break;
		}
		i++;
	}

	if (!formatFound)
	{
		Ren_Developer("R_LoadScalableFont: Unable to find any supported font files by the name of %s\n", fontName);
		return qfalse;
	}

	// Check if this font isn't loaded already
	for (i = 0; i < registeredFontCount; i++)
	{
		if (Q_stricmp(va("fonts/%s_%i.dat", fontName, pointSize), registeredFont[i].datName) == 0)
		{
			if (extended)
			{
				Com_Memcpy(font, &registeredFont[i], sizeof(fontInfo_extra_t));
			}
			else
			{
				Com_Memcpy(font, &registeredFont[i], sizeof(fontInfo_t));
			}
			return qtrue;
		}
	}

	len = ri.FS_ReadFile(name, (void **)&faceData);
	if (len <= 0)
	{
		Ren_Warning("R_LoadScalableFont: Unable to read font file '%s'\n", name);
		return qfalse;
	}

	// allocate on the stack first in case we fail
	if (FT_New_Memory_Face(ftLibrary, faceData, len, 0, &face))
	{
		ri.FS_FreeFile(faceData);
		Ren_Warning("R_LoadScalableFont: FreeType, unable to allocate new face.\n");
		return qfalse;
	}


	// scale dpi based on screen height
	dpi = (float)DPI * (glConfig.vidHeight / (float)SCREEN_HEIGHT);

	if (FT_Set_Char_Size(face, pointSize << 6, pointSize << 6, dpi, dpi))
	{
		ri.FS_FreeFile(faceData);
		Ren_Warning("R_LoadScalableFont: FreeType, unable to set face char size.\n");
		return qfalse;
	}

	//*font = &registeredFonts[registeredFontCount++];

	// scale image size based on screen height, use the next higher power of two
	for (imageSize = FONT_SIZE; imageSize < (float)FONT_SIZE * (glConfig.vidHeight / (float)SCREEN_HEIGHT); imageSize <<= 1)
		;

	// do not exceed maxTextureSize
	if (imageSize > glConfig.maxTextureSize)
	{
		imageSize = glConfig.maxTextureSize;
	}

	// make a 256x256 image buffer, once it is full, register it, clean it and keep going
	// until all glyphs are rendered

	out = (unsigned char *)ri.Z_Malloc(imageSize * imageSize);
	if (out == NULL)
	{
		ri.FS_FreeFile(faceData);
		Ren_Warning("R_LoadScalableFont: ri.Z_Malloc failure during output image creation.\n");
		return qfalse;
	}
	Com_Memset(out, 0, imageSize * imageSize);

	maxHeight = 0;

	for (i = GLYPH_START; i <= GLYPH_END(extended); i++)
	{
		// FIXME: RE_ConstructGlyphInfo might return NULL and we won't notice that
		RE_ConstructGlyphInfo(imageSize, out, &xOut, &yOut, &maxHeight, face, i, qtrue);
	}

	xOut        = 0;
	yOut        = 0;
	i           = GLYPH_START;
	lastStart   = i;
	imageNumber = 0;

	while (i <= GLYPHS_PER_FONT(extended))
	{
		if (i == GLYPHS_PER_FONT(extended))
		{
			// upload/save current image buffer
			xOut = yOut = -1;
		}
		else
		{
			glyph = RE_ConstructGlyphInfo(imageSize, out, &xOut, &yOut, &maxHeight, face, i, qfalse);
		}

		// FIXME: glyph might be NULL for various reasons
		if (!glyph)
		{
			//ri.FS_FreeFile(faceData);
			Ren_Warning("R_LoadScalableFont: glyph is NULL!\n");
		}

		if (xOut == -1 || yOut == -1)
		{
			// ran out of room
			// we need to create an image from the bitmap, set all the handles in the glyphs to this point

			scaledSize = imageSize * imageSize;
			newSize    = scaledSize * 4;
			imageBuff  = (unsigned char *)ri.Z_Malloc(newSize);
			left       = 0;
			max        = 0;
			for (k = 0; k < (scaledSize) ; k++)
			{
				if (max < out[k])
				{
					max = out[k];
				}
			}

			if (max > 0)
			{
				max = 255 / max;
			}

			for (k = 0; k < (scaledSize) ; k++)
			{
				imageBuff[left++] = 255;
				imageBuff[left++] = 255;
				imageBuff[left++] = 255;

				imageBuff[left++] = ((float)out[k] * max);
			}

			Com_sprintf(name, sizeof(name), "fonts/%s_%i_%i.tga", fontName, imageNumber++, pointSize);
			//if (r_saveFontData->integer)
			//{
			//	RE_SaveTGA(name, imageBuff, imageSize, imageSize, qtrue);
			//}

			//Com_sprintf (name, sizeof(name), "fonts/fontImage_%i_%i", imageNumber++, pointSize);
#ifdef FEATURE_RENDERER2
			image = R_CreateImage(name, imageBuff, imageSize, imageSize, IF_NOPICMIP, FT_LINEAR, WT_EDGE_CLAMP);
			h     = RE_RegisterShaderFromImage(name, image, qfalse);
#else
			image = R_CreateImage(name, imageBuff, imageSize, imageSize, qfalse, qfalse, GL_CLAMP_TO_EDGE);
			h     = RE_RegisterShaderFromImage(name, LIGHTMAP_2D, image, qfalse);
#endif
			for (j = lastStart; j < i; j++)
			{
				// split ascii and unicode chars into different containers for old mod compatibility
				if (j < GLYPHS_ASCII_PER_FONT)
				{
					font->glyphs[j].glyph = h;
					Q_strncpyz(font->glyphs[j].shaderName, name, sizeof(font->glyphs[j].shaderName));
				}
				else if (extended)
				{
					((fontInfo_extra_t *)font)->glyphsUTF8[j].glyph = h;
					Q_strncpyz(((fontInfo_extra_t *)font)->glyphsUTF8[j].shaderName, name, sizeof(((fontInfo_extra_t *)font)->glyphsUTF8[j].shaderName));
				}
			}
			lastStart = i;
			Com_Memset(out, 0, imageSize * imageSize);
			xOut = 0;
			yOut = 0;
			ri.Free(imageBuff);
			if (i == GLYPHS_PER_FONT(extended))
			{
				i++;
			}
		}
		else
		{
			// split ascii and unicode chars into different containers for old mod compatibility
			if (i < GLYPHS_ASCII_PER_FONT)
			{
				Com_Memcpy(&font->glyphs[i], glyph, sizeof(glyphInfo_t));
			}
			else if (extended)
			{
				Com_Memcpy(&((fontInfo_extra_t *)font)->glyphsUTF8[i], glyph, sizeof(glyphInfo_t));
			}
			i++;
		}
	}

	// change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )
	glyphScale = 72.0f / dpi;

	// we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
	glyphScale *= 48.0f / pointSize;

	registeredFont[registeredFontCount].glyphScale = glyphScale;
	font->glyphScale                               = glyphScale;

	Com_Memcpy(&font->datName, va("fonts/%s_%i.dat", fontName, pointSize), sizeof(font->datName));

	if (extended)
	{
		Com_Memcpy(&registeredFont[registeredFontCount++], (fontInfo_extra_t *)font, sizeof(fontInfo_extra_t));
	}
	else
	{
		Com_Memcpy(&registeredFont[registeredFontCount++], font, sizeof(fontInfo_t));
	}

	//if (r_saveFontData->integer)
	//{
	//	ri.FS_WriteFile(va("fonts/%s_%i.dat", fontName, pointSize), font, sizeof(fontInfo_t));
	//}

	ri.Free(out);

	ri.FS_FreeFile(faceData);
	return qtrue;
}
#endif

/**
 * @brief R_GetFont
 * @param[in] fontName
 * @param[in] pointSize
 * @param[out] font
 * @param[in] extended
 * @return
 */
static qboolean R_GetFont(const char *fontName, int pointSize, fontInfo_t *font, qboolean extended)
{
	char datName[MAX_QPATH];

	Com_sprintf(datName, sizeof(datName), "fonts/%s_%i.dat", fontName, pointSize);

	if (registeredFontCount >= MAX_FONTS)
	{
		Ren_Warning("R_GetFont: Too many fonts registered already.\n");
		return qfalse;
	}

#ifdef FEATURE_FREETYPE
	if (R_LoadScalableFont(fontName, pointSize, font, extended))
	{
		return qtrue;
	}
#endif

	if (R_LoadPreRenderedFont(datName, font))
	{
		return qtrue;
	}

	Ren_Warning("R_GetFont: can't load font '%s' '%s' point size %i.\n", fontName, datName, pointSize);

	return qfalse;
}

/**
 * @brief RE_RegisterFont
 * @param[in] fontName
 * @param[in] pointSize
 * @param[out] output
 * @param[in] extended
 */
void RE_RegisterFont(const char *fontName, int pointSize, void *output, qboolean extended)
{
	fontInfo_t *font = (fontInfo_t *)output;

	if (!fontName)
	{
		Ren_Print("RE_RegisterFont: called with empty name\n");
		return;
	}

	if (pointSize <= 0)
	{
		pointSize = 12;
	}

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	if (!R_GetFont(fontName, pointSize, font, extended))
	{
		Ren_Print("RE_RegisterFont: failed to register font with name '%s' point size %i\n", fontName, pointSize);
	}
}

/**
 * @brief R_InitFreeType
 */
void R_InitFreeType(void)
{
#ifdef FEATURE_FREETYPE
	if (FT_Init_FreeType(&ftLibrary))
	{
		Ren_Warning("R_InitFreeType: Unable to initialize FreeType.\n");
	}
#endif
	registeredFontCount = 0;
}

/**
 * @brief R_DoneFreeType
 */
void R_DoneFreeType(void)
{
#ifdef FEATURE_FREETYPE
	if (ftLibrary)
	{
		FT_Done_FreeType(ftLibrary);
		ftLibrary = NULL;
	}
#endif
	registeredFontCount = 0;
}
