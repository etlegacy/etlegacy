/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file tr_common.c
 */

#include "tr_common.h"
#include "tr_local_proxy.h"

cvar_t *r_ext_multisample;

/**
  * @var imageLoaders
  * @brief Note that the ordering indicates the order of preference used
  * when there are multiple images of different formats available
  */
imageExtToLoaderMap_t imageLoaders[] =
{
	{ "png",  R_LoadPNG },
	{ "tga",  R_LoadTGA },
	{ "jpg",  R_LoadJPG },
	{ "jpeg", R_LoadJPG },
	{ "pcx",  R_LoadPCX },
	{ "bmp",  R_LoadBMP }
};

int numImageLoaders = sizeof(imageLoaders) / sizeof(imageLoaders[0]);

/**
 * @brief Workaround for ri.Printf's 1024 characters buffer limit.
 * @param[in] string
 */
void R_PrintLongString(const char *string)
{
	char       buffer[1024];
	const char *p   = string;
	int        size = (int)strlen(string);

	while (size > 0)
	{
		Q_strncpyz(buffer, p, sizeof(buffer));
		Ren_Print("%s", buffer);
		p    += 1023;
		size -= 1023;
	}
}

static char *R_SkipSimpleWhitespace(char *data)
{
	while (*data && *data <= ' ')
	{
		data++;
	}

	return data;
}

static void R_ParseMaxPicMipToken(const char *token, int *maxPicMip, const char *shaderName)
{
	int        value;
	const char *name = (shaderName && shaderName[0]) ? shaderName : "<unknown>";

	if (!token || !token[0])
	{
		Ren_Warning("WARNING: missing value for 'maxpicmip' in shader '%s'\n", name);
		return;
	}

	value = Q_atoi(token);
	if (value < 0)
	{
		*maxPicMip = -1;
	}
	else
	{
		*maxPicMip = Com_Clamp(0, 3, value);
	}
}

qboolean R_ParseEtlDirective(char **text, int *maxPicMip, const char *shaderName, qboolean useParseExt2)
{
	char   *data;
	char   *lineStart;
	char   lineBuffer[MAX_STRING_CHARS];
	char   *linePtr;
	char   *token;
	size_t lineLen;

	if (!text || !*text || !maxPicMip)
	{
		return qfalse;
	}

	data = R_SkipSimpleWhitespace(*text);
	if (data[0] != '/' || data[1] != '/' || data[2] != '/')
	{
		return qfalse;
	}

	lineStart = R_SkipSimpleWhitespace(data + 3);
	lineLen   = 0;
	while (lineStart[lineLen] && lineStart[lineLen] != '\n')
	{
		lineLen++;
	}
	if (lineLen >= sizeof(lineBuffer))
	{
		lineLen = sizeof(lineBuffer) - 1;
	}
	Com_Memcpy(lineBuffer, lineStart, lineLen);
	lineBuffer[lineLen] = '\0';

	linePtr = lineBuffer;
	if (useParseExt2)
	{
		token = COM_ParseExt2(&linePtr, qfalse);
	}
	else
	{
		token = COM_ParseExt(&linePtr, qfalse);
	}

	if (token[0] && !Q_stricmp(token, "maxpicmip"))
	{
		if (useParseExt2)
		{
			token = COM_ParseExt2(&linePtr, qfalse);
		}
		else
		{
			token = COM_ParseExt(&linePtr, qfalse);
		}
		R_ParseMaxPicMipToken(token, maxPicMip, shaderName);
	}

	while (*data && *data != '\n')
	{
		data++;
	}
	*text = data;

	return qtrue;
}

void R_RegisterCommon(void)
{
	r_ext_multisample = ri.Cvar_Get("r_ext_multisample", "0", CVAR_ARCHIVE_ND | CVAR_LATCH | CVAR_UNSAFE);
	ri.Cvar_CheckRange(r_ext_multisample, 0, 16, qtrue);
}

#ifdef USE_RENDERER_DLOPEN
/**
 * @brief Com_Printf
 * @param[in] msg
 */
void QDECL Com_Printf(const char *fmt, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	Ren_Print("%s", text);
}

/**
 * @brief Com_DPrintf
 * @param[in] msg
 */
void QDECL Com_DPrintf(const char *fmt, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	Ren_Developer("%s", text);
}

/**
 * @brief Com_Error
 * @param[in] code
 * @param[in] error
 */
void QDECL Com_Error(int code, const char *error, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Error(code, "%s", text);
}
#endif
