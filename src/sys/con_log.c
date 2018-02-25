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
 * @file con_log.c
 */

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#define MAX_LOG 32768

static char         consoleLog[MAX_LOG];
static unsigned int writePos = 0;
static unsigned int readPos  = 0;

/**
 * @brief CON_LogSize
 * @return
 */
unsigned int CON_LogSize(void)
{
	if (readPos <= writePos)
	{
		return writePos - readPos;
	}
	else
	{
		return writePos + MAX_LOG - readPos;
	}
}

/**
 * @brief CON_LogFree
 * @return
 */
static unsigned int CON_LogFree(void)
{
	return MAX_LOG - CON_LogSize() - 1;
}

/**
 * @brief CON_LogWrite
 * @param[in] in
 * @return
 */
unsigned int CON_LogWrite(const char *in)
{
	unsigned int length = strlen(in);
	unsigned int firstChunk;
	unsigned int secondChunk;

	while (CON_LogFree() < length && CON_LogSize() > 0)
	{
		// Free enough space
		while (consoleLog[readPos] != '\n' && CON_LogSize() > 1)
			readPos = (readPos + 1) % MAX_LOG;

		// Skip past the '\n'
		readPos = (readPos + 1) % MAX_LOG;
	}

	if (CON_LogFree() < length)
	{
		return 0;
	}

	if (writePos + length > MAX_LOG)
	{
		firstChunk  = MAX_LOG - writePos;
		secondChunk = length - firstChunk;
	}
	else
	{
		firstChunk  = length;
		secondChunk = 0;
	}

	Com_Memcpy(consoleLog + writePos, in, firstChunk);
	Com_Memcpy(consoleLog, in + firstChunk, secondChunk);

	writePos = (writePos + length) % MAX_LOG;

	return length;
}

/**
 * @brief CON_LogRead
 * @param[out] out
 * @param[in] outSize
 * @return
 */
unsigned int CON_LogRead(char *out, unsigned int outSize)
{
	unsigned int firstChunk;
	unsigned int secondChunk;

	if (CON_LogSize() < outSize)
	{
		outSize = CON_LogSize();
	}

	if (readPos + outSize > MAX_LOG)
	{
		firstChunk  = MAX_LOG - readPos;
		secondChunk = outSize - firstChunk;
	}
	else
	{
		firstChunk  = outSize;
		secondChunk = 0;
	}

	Com_Memcpy(out, consoleLog + readPos, firstChunk);
	Com_Memcpy(out + firstChunk, consoleLog, secondChunk);

	readPos = (readPos + outSize) % MAX_LOG;

	return outSize;
}
