/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * Daemon GPL Source Code
 * Copyright (C) 2012-2013 Unvanquished Developers
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
 * @file q_unicode.c
 * @brief Unicode & UTF-8 handling
 */

#include "q_shared.h"
#include "q_unicode.h"

/**
 * @brief Never returns more than 4
 * @param[in] str
 * @return
 */
int Q_UTF8_Width(const char *str)
{
	int                 ewidth;
	const unsigned char *s = (const unsigned char *)str;

	if (!str)
	{
		return 0;
	}

	if     (*s <= 0x7F)
	{
		ewidth = 0;
	}
	else if (0xC2 <= *s && *s <= 0xDF)
	{
		ewidth = 1;
	}
	else if (0xE0 <= *s && *s <= 0xEF)
	{
		ewidth = 2;
	}
	else if (0xF0 <= *s && *s <= 0xF4)
	{
		ewidth = 3;
	}
	else
	{
		ewidth = 0;
	}

	for ( ; *s && ewidth > 0; s++, ewidth--)
		;

	return s - (const unsigned char *)str + 1;
}

/**
 * @brief Q_UTF8_WidthCP
 * @param[in] ch
 * @return
 */
int Q_UTF8_WidthCP(int ch)
{
	if (ch <=   0x007F)
	{
		return 1;
	}
	if (ch <=   0x07FF)
	{
		return 2;
	}
	if (ch <=   0xFFFF)
	{
		return 3;
	}
	if (ch <= 0x10FFFF)
	{
		return 4;
	}
	return 0;
}

/**
 * @brief Q_UTF8_Strlen
 * @param[in] str
 * @return
 */
int Q_UTF8_Strlen(const char *str)
{
	int l = 0;

	while (*str)
	{
		l++;

		str += Q_UTF8_Width(str);
	}

	return l;
}

/**
 * @brief Q_UTF8_PrintStrlen
 * @param[in] str
 * @return
 */
int Q_UTF8_PrintStrlen(const char *str)
{
	int l = 0;

	while (*str)
	{
		if (Q_IsColorString(str))
		{
			str += 2;
			continue;
		}
		if (*str == Q_COLOR_ESCAPE && str[1] == Q_COLOR_ESCAPE)
		{
			++str;
		}

		l++;

		str += Q_UTF8_Width(str);
	}

	return l;
}

/**
 * @brief Q_UTF8_ByteOffset
 * @param[in] str
 * @param[in] offset
 * @return
 */
int Q_UTF8_ByteOffset(const char *str, int offset)
{
	int i = 0, l = 0, m = 0;

	if (offset <= 0)
	{
		return 0;
	}

	while (*str)
	{
		l++;

		m    = Q_UTF8_Width(str);
		i   += m;
		str += m;

		if (l == offset)
		{
			break;
		}
	}

	return i;
}

/**
 * @brief Q_UTF8_Insert
 * @param[in,out] dest
 * @param[in] size
 * @param[in] offset
 * @param[in] key
 * @param[in] overstrike
 */
void Q_UTF8_Insert(char *dest, int size, int offset, int key, qboolean overstrike)
{
	int  len = 0, i = 0, byteOffset = 0;
	char *str;

	str        = Q_UTF8_Encode(key);
	byteOffset = Q_UTF8_ByteOffset(dest, offset);
	len        = Q_UTF8_WidthCP(key);

	if (offset < size)
	{
		if (overstrike)
		{
			int moveReq = len - Q_UTF8_Width(&dest[byteOffset]);
			if (moveReq > 0)
			{
				memmove(&dest[byteOffset + moveReq], &dest[byteOffset], strlen(dest) + 1 - byteOffset);
			}
			else if (moveReq < 0)
			{
				memmove(&dest[byteOffset], &dest[byteOffset - moveReq], strlen(dest) + 1 - byteOffset);
			}
		}
		else
		{
			memmove(&dest[byteOffset + len], &dest[byteOffset], strlen(dest) + 1 - byteOffset);
		}
	}

	for (i = 0; i < len; i++)
	{
		dest[byteOffset + i] = str[i];
	}
}

/**
 * @brief Q_UTF8_Move
 * @param[in,out] data
 * @param[in] offset1
 * @param[in] offset2
 * @param[in] size
 */
void Q_UTF8_Move(char *data, size_t offset1, size_t offset2, size_t size)
{
	size_t byteOffset1 = 0, byteOffset2 = 0, byteSize = 0;

	if (!size)
	{
		return;
	}

	byteOffset1 = Q_UTF8_ByteOffset(data, offset1);
	byteOffset2 = Q_UTF8_ByteOffset(data, offset2);
	byteSize    = Q_UTF8_ByteOffset(&data[byteOffset2], size);

	if (!byteSize)
	{
		byteSize = 1;
	}

	if (offset1 < offset2 && (offset2 + size) > Q_UTF8_Strlen(data))
	{
		byteSize++;
	}

	memmove(&data[byteOffset1], &data[byteOffset2], byteSize); // +1
	data[strlen(data) + 1] = '\0';
}

/**
 * @brief Q_UTF8_ContByte
 * @param[in] c
 * @return
 */
qboolean Q_UTF8_ContByte(char c)
{
	return (unsigned char )0x80 <= (unsigned char)c && (unsigned char)c <= (unsigned char )0xBF;
}

/**
 * @brief getbit
 * @param[in,out] p
 * @param[in] pos
 * @return
 */
static qboolean getbit(const unsigned char *p, int pos)
{
	p   += pos / 8;
	pos %= 8;

	return (p && (*p & (1 << (7 - pos)))) != 0;
}

/**
 * @brief setbit
 * @param[in,out] p
 * @param[in] pos
 * @param[in] on
 */
static void setbit(unsigned char *p, int pos, qboolean on)
{
	p   += pos / 8;
	pos %= 8;

	if (on)
	{
		*p |= 1 << (7 - pos);
	}
	else
	{
		*p &= ~(1 << (7 - pos));
	}
}

/**
 * @brief shiftbitsright
 * @param[in,out] p
 * @param[in] num
 * @param[in] by
 */
static void shiftbitsright(unsigned char *p, unsigned long num, unsigned long by)
{
	int           step, off;
	unsigned char *e;

	if (by >= num)
	{
		for (; num > 8; p++, num -= 8)
		{
			*p = 0;
		}

		*p &= (~0x00) >> num;

		return;
	}

	step = by / 8;
	off  = by % 8;

	for (e = p + (num + 7) / 8 - 1; e > p + step; e--)
	{
		*e = (*(e - step) >> off) | (*(e - step - 1) << (8 - off));
	}

	*e = *(e - step) >> off;

	for (e = p; e < p + step; e++)
	{
		*e = 0;
	}
}

/**
 * @brief Calculates codepoint for a single char
 * @param str character to convert
 * @return codepoint of a character
 */
unsigned long Q_UTF8_CodePoint(const char *str)
{
	int           i, j;
	int           n         = 0;
	int           size      = Q_UTF8_Width(str);
	unsigned long codepoint = 0;
	unsigned char *p        = (unsigned char *) &codepoint;

	if (size > sizeof(codepoint))
	{
		size = sizeof(codepoint);
	}
	else if (size < 1)
	{
		size = 1;
	}

	for (i = (size > 1 ? size : 0); i < 8; i++)
	{
		setbit(p, n++, getbit((const unsigned char *)str, i));
	}

	for (i = 1; i < size; i++)
	{
		for (j = 2; j < 8; j++)
		{
			setbit(p, n++, getbit(((const unsigned char *)str) + i, j));
		}
	}

	/*
	if( n > 8 * sizeof(codepoint) )
	{
	      Com_Error( ERR_DROP, "Q_UTF8_CodePoint: overflow caught" );

	  return 0;
	}
	*/

	shiftbitsright(p, 8 * sizeof(codepoint), 8 * sizeof(codepoint) - n);

#ifndef Q3_BIG_ENDIAN
	for (i = 0; i < sizeof(codepoint) / 2; i++)
	{
		p[i]                         ^= p[sizeof(codepoint) - 1 - i];
		p[sizeof(codepoint) - 1 - i] ^= p[i];
		p[i]                         ^= p[sizeof(codepoint) - 1 - i];
	}
#endif

	return codepoint;
}

/**
 * @brief Q_UTF8_Encode
 * @param[in] codepoint
 * @return
 */
char *Q_UTF8_Encode(unsigned long codepoint)
{
	static char sbuf[2][5];
	static int  index = 0;
	char        *buf  = sbuf[index++ & 1];

	if     (codepoint <= 0x007F)
	{
		buf[0] = codepoint;
		buf[1] = 0;
	}
	else if (0x0080 <= codepoint && codepoint <= 0x07FF)
	{
		buf[0] = 0xC0 | ((codepoint & 0x07C0) >> 6);
		buf[1] = 0x80 | (codepoint & 0x003F);
		buf[2] = 0;
	}
	else if (0x0800 <= codepoint && codepoint <= 0xFFFF)
	{
		buf[0] = 0xE0 | ((codepoint & 0xF000) >> 12);
		buf[1] = 0x80 | ((codepoint & 0x0FC0) >> 6);
		buf[2] = 0x80 | (codepoint & 0x003F);
		buf[3] = 0;
	}
	else if (0x010000 <= codepoint && codepoint <= 0x10FFFF)
	{
		buf[0] = 0xF0 | ((codepoint & 0x1C0000) >> 18);
		buf[1] = 0x80 | ((codepoint & 0x03F000) >> 12);
		buf[2] = 0x80 | ((codepoint & 0x000FC0) >> 6);
		buf[3] = 0x80 | (codepoint & 0x00003F);
		buf[4] = 0;
	}
	else
	{
		buf[0] = 0;
	}

	return buf;
}

/**
 * @brief Stores a single UTF8 char inside an int
 * @param[in] s
 * @return
 */
int Q_UTF8_Store(const char *s)
{
	int           r   = 0;
	const uint8_t *us = ( const uint8_t * ) s;

	if (!us)
	{
		return 0;
	}

	if (!(us[0] & 0x80))       // 0xxxxxxx
	{
		r = us[0];
	}
	else if ((us[0] & 0xE0) == 0xC0)       // 110xxxxx
	{
		r  = us[0];
		r |= ( uint32_t ) us[1] << 8;
	}
	else if ((us[0] & 0xF0) == 0xE0)       // 1110xxxx
	{
		r  = us[0];
		r |= ( uint32_t ) us[1] << 8;
		r |= ( uint32_t ) us[2] << 16;
	}
	else if ((us[0] & 0xF8) == 0xF0)       // 11110xxx
	{
		r  = us[0];
		r |= ( uint32_t ) us[1] << 8;
		r |= ( uint32_t ) us[2] << 16;
		r |= ( uint32_t ) us[3] << 24;
	}

	return r;
}

/**

 */
/**
 * @brief Converts a single UTF8 char stored as an int into a byte array
 * @param[in] e
 * @return
 */
char *Q_UTF8_Unstore(int e)
{
	static unsigned char sbuf[2][5];
	static int           index = 0;
	unsigned char        *buf;

	index = (index + 1) & 1;
	buf   = sbuf[index];

	buf[0] = e & 0xFF;
	buf[1] = (e >> 8) & 0xFF;
	buf[2] = (e >> 16) & 0xFF;
	buf[3] = (e >> 24) & 0xFF;
	buf[4] = 0;

	return ( char * ) buf;
}

/**
 * @brief Q_UTF8_GetGlyphExtended
 * @param[in] fontdata
 * @param[in] codepoint
 * @return
 */
glyphInfo_t *Q_UTF8_GetGlyphExtended(void *fontdata, unsigned long codepoint)
{
	if (codepoint > GLYPH_UTF_END)
	{
		codepoint = INVALID_CHAR_OFFSET;
	}

	if (codepoint <= GLYPH_ASCII_END)
	{
		return &((fontInfo_extra_t *)fontdata)->glyphs[codepoint];
	}
	else
	{
		return &((fontInfo_extra_t *)fontdata)->glyphsUTF8[codepoint];
	}
}

/**
 * @brief Q_UTF8_GetGlyphVanilla
 * @param[in] fontdata
 * @param[in] codepoint
 * @return
 */
glyphInfo_t *Q_UTF8_GetGlyphVanilla(void *fontdata, unsigned long codepoint)
{
	if (codepoint > GLYPH_ASCII_END)
	{
		codepoint = INVALID_CHAR_OFFSET;
	}

	return &((fontInfo_t *)fontdata)->glyphs[codepoint];
}

/**
 * @brief Q_UTF8_RegisterFont
 * @param[in] fontName
 * @param[in] pointSize
 * @param[in,out] font
 * @param[in] extended
 * @param font_register
 */
void Q_UTF8_RegisterFont(const char *fontName, int pointSize, fontHelper_t *font, qboolean extended, void (*font_register)(const char *, int, void *))
{
	if (!font)
	{
		return;
	}

	Q_UTF8_FreeFont(font);

	if (extended)
	{
		font->fontData = Com_Allocate(sizeof(fontInfo_extra_t));
		font->GetGlyph = &Q_UTF8_GetGlyphExtended;
	}
	else
	{
		font->fontData = Com_Allocate(sizeof(fontInfo_t));
		font->GetGlyph = &Q_UTF8_GetGlyphVanilla;
	}

	font_register(fontName, pointSize, font->fontData);
}

/**
 * @brief Q_UTF8_FreeFont
 * @param[in,out] font
 */
void Q_UTF8_FreeFont(fontHelper_t *font)
{
	if (font)
	{
		if (font->fontData)
		{
			Com_Dealloc(font->fontData);
			font->fontData = NULL;
			font->GetGlyph = NULL;
		}
	}
}

/**
 * @brief Q_UTF8_ToUTF32
 * @param[in,out] string
 * @param[in] charArray
 * @param[out] outlen
 */
void Q_UTF8_ToUTF32(char *string, int *charArray, int *outlen)
{
	int  i  = 0;
	char *c = string;

	// Quick and dirty UTF-8 to UTF-32 conversion
	while (*c)
	{
		int utf32 = 0;

		if ((*c & 0x80) == 0)
		{
			utf32 = *c++;
		}
		else if ((*c & 0xE0) == 0xC0)    // 110x xxxx
		{
			utf32 |= (*c++ & 0x1F) << 6;
			utf32 |= (*c++ & 0x3F);
		}
		else if ((*c & 0xF0) == 0xE0)    // 1110 xxxx
		{
			utf32 |= (*c++ & 0x0F) << 12;
			utf32 |= (*c++ & 0x3F) << 6;
			utf32 |= (*c++ & 0x3F);
		}
		else if ((*c & 0xF8) == 0xF0)    // 1111 0xxx
		{
			utf32 |= (*c++ & 0x07) << 18;
			utf32 |= (*c++ & 0x3F) << 12;
			utf32 |= (*c++ & 0x3F) << 6;
			utf32 |= (*c++ & 0x3F);
		}
		else
		{
			//Unrecognized UTF-8 lead byte
			c++;
		}
		charArray[i++] = utf32;
	}

	*outlen = i;
}
