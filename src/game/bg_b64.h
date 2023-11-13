/*
===========================================================================
Copyright (C) 2022 Gian 'myT' Schellenbaum. All rights reserved.

This file is part of Challenge ProMode Arena (CPMA).
===========================================================================
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

char B64_Char(int offset);
int B64_Offset(char c);
