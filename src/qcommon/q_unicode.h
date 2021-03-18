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
 * @file q_unicode.h
 * @brief Unicode & UTF-8 handling
 *
 * The whole unicode handling system was published at [cprogramming.com]
 * by Jeff Bezanson and adapted for the id Tech 3 engine by Unvanquished
 * developers.
 *
 * [cprogramming.com]: http://www.cprogramming.com/tutorial/unicode.html
 */

#ifndef INCLUDE_Q_UNICODE_H
#define INCLUDE_Q_UNICODE_H

// The version in which unicode was added
#define UNICODE_SUPPORT_VERSION 273

// the cross sign which is used in place of invalid chars
#define INVALID_CHAR_OFFSET 215

int Q_UTF8_Width(const char *str);
int Q_UTF8_WidthCP(int ch);
qboolean Q_UTF8_ValidateSingle(const char *str);
qboolean Q_UTF8_Validate(const char *str);
char *Q_Extended_To_UTF8(char *txt);
size_t Q_UTF8_Strlen(const char *str);
size_t Q_UTF32_Strlen(const uint32_t *str, size_t len);
char* Q_UTF8_CharAt(char *str, size_t offset);
int Q_UTF8_PrintStrlen(const char *str);
int Q_UTF8_PrintStrlenExt(const char *str, int length);
int Q_UTF8_ByteOffset(const char *str, int offset);
void Q_UTF8_Insert(char *dest, int size, int offset, int key, qboolean overstrike);
void Q_UTF8_Move(char *buffer, size_t dstOffset, size_t srcOffset, size_t size);
qboolean Q_UTF8_ContByte(char c);
uint32_t Q_UTF8_CodePoint(const char *str);
void Q_UTF8_RegisterFont(const char *fontName, int pointSize, fontHelper_t *font, qboolean extended, void (*font_register)(const char *, int, void *));
void Q_UTF8_FreeFont(fontHelper_t *font);
char *Q_UTF8_Encode(unsigned long codepoint);
int Q_UTF8_Store(const char *s);
char *Q_UTF8_Unstore(int e);
void Q_UTF8_ToUTF32(const char *string, uint32_t *charArray, size_t *outLen);
void Q_UTF32_ToUTF8(const uint32_t *charArray, size_t arraySize, char *string, size_t *outLen);

size_t Q_EscapeUnicode(char *fromStr, char *toStr, size_t maxSize);
size_t Q_UnescapeUnicode(char *fromStr, char *toStr, size_t maxSize);

size_t Q_EscapeUnicodeInPlace(char *string, size_t size);
size_t Q_UnescapeUnicodeInPlace(char *string, size_t size);

#endif // INCLUDE_Q_UNICODE_H
