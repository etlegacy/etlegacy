/*
 * Copyright (C) 2022 Gian 'myT' Schellenbaum.
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
 */
/**
 * @file bg_b64.h
 */

typedef struct
{
	unsigned char *stream;
	int bytes;
	int bit;
} bitStream_t;

// byte order: least significant byte first
// bit  order: least significant bit  first
void BS_Init(bitStream_t *bs, unsigned char *buffer, int bufferSize);
void BS_Write(bitStream_t *bs, int value, int bits);
int BS_Read(bitStream_t *bs, int bits);

// B64_Encode doesn't write padding chars
// B64_Encode null-terminates the output buffer
// B64_Decode will decode the full string when inBits <= 0
void B64_Encode(char *out, int maxOutChars, const unsigned char *in, int inBits);
void B64_Decode(unsigned char *out, int maxOutBytes, const char *in, int inBits);
void B64_DecodeBigEndian(unsigned char *out, int maxOutBytes, const char *in, int inBits);

char B64_Char(int offset);
int B64_Offset(char c);
