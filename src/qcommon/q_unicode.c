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

	if (*s <= 0x7F)
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
	if (ch <= 0x007F)
	{
		return 1;
	}
	if (ch <= 0x07FF)
	{
		return 2;
	}
	if (ch <= 0xFFFF)
	{
		return 3;
	}
	if (ch <= 0x10FFFF)
	{
		return 4;
	}
	return 0;
}

qboolean Q_UTF8_ValidateSingle(const char *str)
{
	int    i = 0, utfBytes = 0;
	size_t len = strlen(str);
	byte   current = str[0];

	if (0x00 <= current && current <= 0x7F)
	{
		utfBytes = 0; // 0XXXXXXX
	}
	else if ((current & 0xE0) == 0xC0)
	{
		utfBytes = 1; // 110XXXXX
	}
	else if (current == 0xED && (len - 1) > 0 && ((byte) str[1] & 0xA0) == 0xA0)
	{
		return qfalse; //U+D800 to U+DFFF
	}
	else if ((current & 0xF0) == 0xE0)
	{
		utfBytes = 2; // 1110XXXX
	}
	else if ((current & 0xF8) == 0xF0)
	{
		utfBytes = 3; // 11110XXX
	}
	else
	{
		return qfalse;
	}

	if(utfBytes > len)
	{
		return qfalse;
	}

	for (; 0 < utfBytes && i < len; utfBytes--)
	{
		if ((++i == len) || (((byte)str[i] & 0xC0) != 0x80))
		{
			return qfalse;
		}
	}

	return qtrue;
}

qboolean Q_UTF8_Validate(const char *str)
{
	int    i, utfBytes = 0;
	byte   current;
	size_t len = strlen(str);
	for (i = 0; i < len; i++)
	{
		current = str[i];

		if (0x00 <= current && current <= 0x7F)
		{
			utfBytes = 0; // 0XXXXXXX
		}
		else if ((current & 0xE0) == 0xC0)
		{
			utfBytes = 1; // 110XXXXX
		}
		else if (current == 0xED && i < (len - 1) && ((byte) str[i + 1] & 0xA0) == 0xA0)
		{
			return qfalse; //U+D800 to U+DFFF
		}
		else if ((current & 0xF0) == 0xE0)
		{
			utfBytes = 2; // 1110XXXX
		}
		else if ((current & 0xF8) == 0xF0)
		{
			utfBytes = 3; // 11110XXX
		}
		else
		{
			return qfalse;
		}

		if(utfBytes > (len - i))
		{
			return qfalse;
		}

		for (; 0 < utfBytes && i < len; utfBytes--)
		{
			if ((++i == len) || (((byte)str[i] & 0xC0) != 0x80))
			{
				return qfalse;
			}
		}
	}

	return qtrue;
}

char *Q_Extended_To_UTF8(char *txt)
{
	static char tmpPrintBuffer[MAX_VA_STRING];

	if (!Q_UTF8_Validate(txt))
	{
		size_t i;
		char *tmpPointer = tmpPrintBuffer;
		size_t len = strlen(txt);
		for (i = 0; i < len;)
		{
			if (127 < (unsigned char) txt[i] && !Q_UTF8_ValidateSingle(&txt[i]))
			{
				char *buffer = Q_UTF8_Encode((unsigned char)txt[i]);
				while (*buffer)
				{
					tmpPointer[0] = buffer[0];
					buffer++;
					tmpPointer++;
				}
				i++;
			}
			else
			{
				int width = Q_UTF8_Width(&txt[i]);

				// Well width returned something that should not be possible, guard our ass against infinite loop.
				if(width <= 0)
				{
					i++;
					continue;
				}

				while (width--)
				{
					tmpPointer[0] = txt[i++];
					tmpPointer++;
				}
			}
		}
		tmpPointer[0] = '\0';
		return tmpPrintBuffer;
	}
	return txt;
}

/**
 * @brief Q_UTF8_Strlen
 * @param[in] str
 * @return
 */
size_t Q_UTF8_Strlen(const char *str)
{
	size_t l = 0;
	while (*str)
	{
		l++;

		str += Q_UTF8_Width(str);
	}

	return l;
}

size_t Q_UTF32_Strlen(const uint32_t *str, size_t len)
{
	int    i = 0;
	size_t l = 0;
	for (; i < len && str[i]; i++)
	{
		l += Q_UTF8_WidthCP(str[i]);
	}
	return l;
}

char *Q_UTF8_CharAt(char *str, size_t offset)
{
	int l = 0;

	while (*str)
	{
		if (offset == l)
		{
			return str;
		}

		l++;
		str += Q_UTF8_Width(str);
	}

	return NULL;
}

/**
 * @brief Q_UTF8_PrintStrlen2
 * @param[in] str
 * @param[in] length string length
 * @return
 */
int Q_UTF8_PrintStrlenExt(const char *str, int length)
{
	int        l      = 0;
	const char *start = str;

	while (*str && (str - start) < length)
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
 * @brief Q_UTF8_PrintStrlen
 * @param[in] str
 * @return
 */
int Q_UTF8_PrintStrlen(const char *str)
{
	return Q_UTF8_PrintStrlenExt(str, MAX_QINT);
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
 * @param[in,out] buffer
 * @param[in] dstOffset
 * @param[in] srcOffset
 * @param[in] size
 */
void Q_UTF8_Move(char *buffer, size_t dstOffset, size_t srcOffset, size_t size)
{
	size_t byteOffset1, byteOffset2, byteSize;

	if (!size)
	{
		return;
	}

	byteOffset1 = Q_UTF8_ByteOffset(buffer, dstOffset);
	byteOffset2 = Q_UTF8_ByteOffset(buffer, srcOffset);
	byteSize    = Q_UTF8_ByteOffset(&buffer[byteOffset2], size);

	if (!byteSize)
	{
		byteSize = 1;
	}

	if (dstOffset < srcOffset && (srcOffset + size) > Q_UTF8_Strlen(buffer))
	{
		byteSize++;
	}

	memmove(&buffer[byteOffset1], &buffer[byteOffset2], byteSize); // +1
	buffer[strlen(buffer) + 1] = '\0';
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

		*p &= ((unsigned long)~0x00) >> num;

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
uint32_t Q_UTF8_CodePoint(const char *str)
{
	int           i, j;
	int           n         = 0;
	int           size      = Q_UTF8_Width(str);
	uint32_t      codepoint = 0;
	unsigned char *p        = (unsigned char *) &codepoint;

	if (!str || !str[0])
	{
		return 0;
	}

	// Its an extended char
	if(!Q_UTF8_ValidateSingle(str))
	{
		return (unsigned char)str[0];
	}

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
		Com_Error( ERR_FATAL, "Q_UTF8_CodePoint: overflow caught" );
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
 * @param[out] outLen
 */
void Q_UTF8_ToUTF32(const char *string, uint32_t *charArray, size_t *outLen)
{
	int        i  = 0;
	const char *c = string;

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

	*outLen = i;
}


void Q_UTF32_ToUTF8(const uint32_t *charArray, size_t arraySize, char *string, size_t *outLen)
{
	int len, i, x, byteOffset = 0;

	for (i = 0; i < arraySize; i++)
	{
		len = Q_UTF8_WidthCP(charArray[i]);
		char *str = Q_UTF8_Encode(charArray[i]);

		for (x = 0; x < len; x++)
		{
			string[byteOffset + x] = str[x];
		}
		byteOffset += x;
	}

	string[byteOffset] = '\0';

	*outLen = byteOffset;
}

/**
 * Escapes a string replacing all non ascii characters with '\u{code-point}' escapes.
 * @param fromStr source string which to escape
 * @param toStr output buffer
 * @param maxSize output buffer size
 * @return generated string length
 */
size_t Q_EscapeUnicode(char *fromStr, char *toStr, const size_t maxSize)
{
	// \u{num}
	size_t width = 0;
	char   *str  = fromStr;
	int    l     = 0;

	while (*str)
	{
		if (l >= maxSize)
		{
			return l;
		}

		width = Q_UTF8_Width(str);

		if (width > 1)
		{
			toStr[l++] = '\\';
			toStr[l++] = 'u';
			toStr[l++] = '{';
			uint32_t cd = Q_UTF8_CodePoint(str);

			if (cd > 999999999)
			{
				return 0;
			}

			char buffer[10];
			sprintf(buffer, "%d", cd);
			size_t bufferLen = strlen(buffer);

			Q_strncpyz(&toStr[l], buffer, maxSize - l);
			l += bufferLen;

			toStr[l] = '}';
		}
		else
		{
			toStr[l] = str[0];
		}

		l++;
		str += width;
	}

	toStr[l] = '\0';
	return l;
}

/**
 * Unescapes a string replacing the escaped character with the real utf-8 bytes
 * @param fromStr escaped string
 * @param toStr output buffer
 * @param maxSize output buffer size
 * @return generated string length
 */
size_t Q_UnescapeUnicode(char *fromStr, char *toStr, const size_t maxSize)
{
	char *str = fromStr;
	int  l    = 0;

	while (*str)
	{
		if (l >= maxSize)
		{
			return l;
		}

		if (str[0] == '\\' && str[1] == 'u' && str[2] == '{')
		{
			char   tmpNumber[20] = { 0 };
			size_t numberOffset  = 0;
			str += 3;

			while (str && str[0] != '}')
			{
				tmpNumber[numberOffset++] = str[0];
				str++;
			}

			if (!numberOffset)
			{
				if (str)
				{
					str++;
					continue;
				}

				return 0;
			}

			if (!str)
			{
				return 0;
			}

			tmpNumber[numberOffset] = '\0';
			int number = Q_atoi(tmpNumber);

			// Ignore non printable keys
			if (number < 32)
			{
				str++;
				continue;
			}

			char *buffer = Q_UTF8_Encode(number);

			while (*buffer)
			{
				toStr[l++] = buffer[0];
				buffer++;
			}

			str++;
			continue;
		}

		toStr[l] = str[0];

		l++;
		str++;
	}

	toStr[l] = '\0';
	return l;
}

/**
 * Escapes the input string in-place
 * @param string value to escape
 * @param size size of the buffer
 * @return generated string length
 */
size_t Q_EscapeUnicodeInPlace(char *string, const size_t size)
{
	char   *tmpOutput = Com_Allocate(sizeof(char) * size);
	size_t len        = Q_EscapeUnicode(string, tmpOutput, size);
	Q_strncpyz(string, tmpOutput, size);
	Com_Dealloc(tmpOutput);
	return len;
}

/**
 * Unescapes the input string in-place
 * @param string value to unescape
 * @param size size of the buffer
 * @return generated string length
 */
size_t Q_UnescapeUnicodeInPlace(char *string, const size_t size)
{
	char   *tmpOutput = Com_Allocate(sizeof(char) * size);
	size_t len        = Q_UnescapeUnicode(string, tmpOutput, size);
	Q_strncpyz(string, tmpOutput, size);
	Com_Dealloc(tmpOutput);
	return len;
}
