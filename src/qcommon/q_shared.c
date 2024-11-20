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
 * @file q_shared.c
 * @brief Stateless support routines that are included in each code dll
 */

#include "q_shared.h"

/**
 * @brief Com_PowerOf2
 * @param[in] x
 * @return
 */
qboolean Com_PowerOf2(int x)
{
	int bitsSet;
	bitsSet = x & (x - 1); /* bitwise trick to check if x is a power of two */

	return (qboolean)(bitsSet == 0);
}

/**
 * @brief Com_NextPowerOf2
 * @param x value to get next power of 2
 * @return next power of 2 value
 */
long Com_NextPowerOf2(long x)
{
	long i = 1;

	while (i < x)
	{
		i <<= 1;
	}

	return i;
	// Could also be made with this but dunno if it's faster
	// return pow(2, ceil(log(x) / log(2)));
}

/**
 * @brief Com_PreviousPowerOf2
 * @param x value to get previous power of 2
 * @return previous power of 2 value
 */
long Com_PreviousPowerOf2(long x)
{
	return Com_NextPowerOf2(x) >> 1;
}

/**
 * @brief Com_ClosestPowerOf2
 * @param x value to get closest power of 2
 * @return closest power of 2 value
 */
long Com_ClosestPowerOf2(long x)
{
	long next = Com_NextPowerOf2(x);
	long prev = Com_PreviousPowerOf2(x);

	if (next - x < x - prev)
	{
		return next;
	}
	else
	{
		return prev;
	}
}

/**
 * @brief COM_FixPath
 * @param[in,out] pathname
 */
void COM_FixPath(char *pathname)
{
	while (*pathname)
	{
		if (*pathname == '\\')
		{
			*pathname = '/';
		}
		pathname++;
	}
}

/**
 * @brief COM_SkipPath
 * @param[in,out] pathname
 * @return
 */
char *COM_SkipPath(char *pathname)
{
	char *last = pathname;

	while (*pathname)
	{
		if (*pathname == '/')
		{
			last = pathname + 1;
		}
		pathname++;
	}
	return last;
}

/**
 * @brief COM_GetExtension
 * @param[in] name
 * @return
 */
const char *COM_GetExtension(const char *name)
{
	const char *dot = strrchr(name, '.'), *slash;

	if (dot && (!(slash = strrchr(name, '/')) || slash < dot))
	{
		return dot + 1;
	}
	else
	{
		return "";
	}
}

/**
 * @brief COM_StripExtension
 * @param[in] in
 * @param[out] out
 * @param[in] destsize
 */
void COM_StripExtension(const char *in, char *out, int destsize)
{
	const char *dot = strrchr(in, '.'), *slash;

	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
	{
		destsize = (destsize < (dot - in + 1) ? destsize : (dot - in + 1));
	}

	if (in == out && destsize > 1)
	{
		out[destsize - 1] = '\0';
	}
	else
	{
		Q_strncpyz(out, in, destsize);
	}
}

/**
 * @brief String compare the end of the strings and return qtrue if strings match
 * @param[in] in
 * @param[in] ext
 * @return
 */
qboolean COM_CompareExtension(const char *in, const char *ext)
{
	size_t inlen = strlen(in), extlen = strlen(ext);

	if (extlen <= inlen)
	{
		in += inlen - extlen;

		if (!Q_stricmp(in, ext))
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief COM_StripFilename
 * @param[in] in
 * @param[out] out
 */
void COM_StripFilename(const char *in, char *out)
{
	char *end;

	Q_strncpyz(out, in, strlen(in) + 1);
	end  = COM_SkipPath(out);
	*end = 0;
}

/**
 * @brief if path doesn't have an extension, then append the specified one (which should include the .)
 * @param[in,out] path
 * @param[in] maxSize
 * @param[in] extension
 */
void COM_DefaultExtension(char *path, size_t maxSize, const char *extension)
{
	const char *dot = strrchr(path, '.'), *slash;

	if (dot && (!(slash = strrchr(path, '/')) || slash < dot))
	{
		return;
	}
	else
	{
		Q_strcat(path, maxSize, extension);
	}
}

/**
 * @brief Com_HashKey
 * @param[in,out] string
 * @param[in] maxlen
 * @return
 */
int Com_HashKey(char *string, int maxlen)
{
	int hash = 0, i;

	for (i = 0; i < maxlen && string[i] != '\0'; i++)
	{
		hash += string[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}

//============================================================================

/**
 * @brief Allows bit-wise checks on arrays with more than one item (> 32 bits)
 * @param[in] array
 * @param[in] bitNum
 * @return
 */
qboolean COM_BitCheck(const int array[], unsigned int bitNum)
{
	unsigned int i = bitNum / 32;
	unsigned int bitmask;
	bitNum  = bitNum % 32;
	bitmask = 1u << bitNum;
	return ((array[i] & bitmask) != 0);
}

/**
 * @brief Allows bit-wise SETS on arrays with more than one item (> 32 bits)
 * @param[in] array
 * @param[out] bitNum
 */
void COM_BitSet(int array[], unsigned int bitNum)
{
	unsigned int i = bitNum / 32;
	unsigned int bitmask;
	bitNum  = bitNum % 32;
	bitmask = 1u << bitNum;

	array[i] |= bitmask;
}

/**
 * @brief Allows bit-wise CLEAR on arrays with more than one item (> 32 bits)
 * @param[out] array
 * @param[in] bitNum
 */
void COM_BitClear(int array[], unsigned int bitNum)
{
	unsigned int i = bitNum / 32;
	unsigned int bitmask;
	bitNum  = bitNum % 32;
	bitmask = ~(1u << bitNum);

	array[i] &= bitmask;
}
//============================================================================

/**
 * @brief ShortSwap
 * @param[in] l
 * @return
 */
short ShortSwap(short l)
{
	unsigned short tmp = l;
	unsigned short b1  = (tmp & 0x00ff) << 8;
	unsigned short b2  = (tmp & 0xff00) >> 8;

	return (b1 | b2);
}

/**
 * @brief ShortNoSwap
 * @param[in] l
 * @return
 *
 * @note Unused
 */
short ShortNoSwap(short l)
{
	return l;
}

/**
 * @brief LongSwap
 * @param[in] l
 * @return
 */
int LongSwap(int l)
{
	/* is compiled to bswap on gcc/clang/msvc */
	unsigned int tmp = l;
	unsigned int b1  = (tmp & 0x000000ff) << 24;
	unsigned int b2  = (tmp & 0x0000ff00) <<  8;
	unsigned int b3  = (tmp & 0x00ff0000) >>  8;
	unsigned int b4  = (tmp & 0xff000000) >> 24;

	return (b1 | b2 | b3 | b4);
}

/**
 * @brief LongNoSwap
 * @param[in] l
 * @return
 *
 * @note Unused
 */
int LongNoSwap(int l)
{
	return l;
}

/**
 * @brief FloatSwap
 * @param[in] f
 * @return
 */
float FloatSwap(const float *f)
{
	floatint_t out;

	out.f  = *f;
	out.ui = LongSwap(out.ui);

	return out.f;
}

/**
 * @brief FloatNoSwap
 * @param[in] f
 * @return
 *
 * @note Unused
 */
float FloatNoSwap(float f)
{
	return f;
}

/*
============================================================================
PARSING
============================================================================
*/

/**
 * @var punctuation
 * @brief Multiple character punctuation tokens
 */
const char *punctuation[] =
{
	"+=", "-=", "*=", "/=", "&=", "|=", "++", "--",
	"&&", "||", "<=", ">=", "==", "!=",
	NULL
};

static struct
{
	char com_token[MAX_TOKEN_CHARS];
	char com_parsename[MAX_TOKEN_CHARS];
	int com_lines;

	int backup_lines;
	char *backup_text;
} com_parser;

/**
 * @brief COM_BeginParseSession
 * @param[in] name
 */
void COM_BeginParseSession(const char *name)
{
	com_parser.com_lines = 0;
	Com_sprintf(com_parser.com_parsename, sizeof(com_parser.com_parsename), "%s", name);
}

/**
 * @brief COM_BackupParseSession
 * @param[in] data_p
 */
void COM_BackupParseSession(char **data_p)
{
	com_parser.backup_lines = com_parser.com_lines;
	com_parser.backup_text  = *data_p;
}

/**
 * @brief COM_RestoreParseSession
 * @param[out] data_p
 */
void COM_RestoreParseSession(char **data_p)
{
	com_parser.com_lines = com_parser.backup_lines;
	*data_p              = com_parser.backup_text;
}

/**
 * @brief COM_SetCurrentParseLine
 * @param[in] line
 */
void COM_SetCurrentParseLine(int line)
{
	com_parser.com_lines = line;
}

/**
 * @brief COM_GetCurrentParseLine
 * @return
 */
int COM_GetCurrentParseLine(void)
{
	return com_parser.com_lines;
}

/**
 * @brief COM_Parse
 * @param[in] data_p
 * @return
 */
char *COM_Parse(char **data_p)
{
	return COM_ParseExt(data_p, qtrue);
}

/**
 * @brief COM_ParseError
 * @param[in] format
 */
void COM_ParseError(const char *format, ...)
{
	va_list     argptr;
	static char string[4096];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	Com_Printf("ERROR COM_ParseError: %s, line %d: %s\n", com_parser.com_parsename, com_parser.com_lines, string);
}

/*
 * @brief COM_ParseWarning
 * @param[in] format
 * @note Unused.
void COM_ParseWarning(const char *format, ...)
{
    va_list     argptr;
    static char string[4096];

    va_start(argptr, format);
    Q_vsnprintf(string, sizeof(string), format, argptr);
    va_end(argptr);

    Com_Printf("WARNING COM_ParseWarning: %s, line %d: %s\n", com_parsename, com_lines, string);
}
*/

/**
 * @brief Parse a token out of a string
 * Will never return NULL, just empty strings
 *
 * If "allowLineBreaks" is qtrue then an empty
 * string will be returned if the next token is
 * a newline.
 *
 * @param[in,out] data
 * @param[out] hasNewLines
 * @return
 */
static char *SkipWhitespace(char *data, qboolean *hasNewLines)
{
	int c;

	while ((c = *data) <= ' ')
	{
		if (!c)
		{
			return NULL;
		}
		if (c == '\n')
		{
			com_parser.com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

/**
 * @brief COM_Compress
 * @param[in,out] data_p
 * @return
 */
int COM_Compress(char *data_p)
{
	char *in, *out;

	in = out = data_p;
	if (in)
	{
		int      c;
		qboolean newline = qfalse, whitespace = qfalse;

		while ((c = *in) != 0)
		{
			// skip double slash comments
			if (c == '/' && in[1] == '/')
			{
				while (*in && *in != '\n')
				{
					in++;
				}
				// skip /* */ comments
			}
			else if (c == '/' && in[1] == '*')
			{
				while (*in && (*in != '*' || in[1] != '/'))
					in++;
				if (*in)
				{
					in += 2;
				}
				// record when we hit a newline
			}
			else if (c == '\n' || c == '\r')
			{
				newline = qtrue;
				in++;
				// record when we hit whitespace
			}
			else if (c == ' ' || c == '\t')
			{
				whitespace = qtrue;
				in++;
				// an actual token
			}
			else
			{
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline)
				{
					*out++     = '\n';
					newline    = qfalse;
					whitespace = qfalse;
				}
				if (whitespace)
				{
					*out++     = ' ';
					whitespace = qfalse;
				}

				// copy quoted strings unmolested
				if (c == '"')
				{
					*out++ = c;
					in++;
					while (1)
					{
						c = *in;
						if (c && c != '"')
						{
							*out++ = c;
							in++;
						}
						else
						{
							break;
						}
					}
					if (c == '"')
					{
						*out++ = c;
						in++;
					}
				}
				else
				{
					*out = c;
					out++;
					in++;
				}
			}
		}

		*out = 0;
	}
	return out - data_p;
}

/**
 * @brief COM_ParseExt
 * @param[in,out] data_p
 * @param[in] allowLineBreaks
 * @return
 */
char *COM_ParseExt(char **data_p, qboolean allowLineBreaks)
{
	int      c = 0, len = 0;
	qboolean hasNewLines = qfalse;
	char     *data       = *data_p;

	com_parser.com_token[0] = 0;

	// make sure incoming data is valid
	if (!data)
	{
		*data_p = NULL;
		return com_parser.com_token;
	}

	// backup the session data so we can unget easily
	COM_BackupParseSession(data_p);

	while (1)
	{
		// skip whitespace
		data = SkipWhitespace(data, &hasNewLines);
		if (!data)
		{
			*data_p = NULL;
			return com_parser.com_token;
		}
		if (hasNewLines && !allowLineBreaks)
		{
			*data_p = data;
			return com_parser.com_token;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			data += 2;
			while (*data && *data != '\n')
			{
				data++;
			}
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			data += 2;
			while (*data && (*data != '*' || data[1] != '/'))
			{
				data++;
			}
			if (*data)
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\\' && *(data) == '\"')
			{
				// string-in-string
				if (len < MAX_TOKEN_CHARS - 1)
				{
					com_parser.com_token[len] = '\"';
					len++;
				}
				data++;

				while (1)
				{
					c = *data++;

					if (!c)
					{
						com_parser.com_token[len] = 0;
						*data_p                   = ( char * ) data;
						break;
					}
					if ((c == '\\' && *(data) == '\"'))
					{
						if (len < MAX_TOKEN_CHARS - 1)
						{
							com_parser.com_token[len] = '\"';
							len++;
						}
						data++;
						c = *data++;
						break;
					}
					if (len < MAX_TOKEN_CHARS - 1)
					{
						com_parser.com_token[len] = c;
						len++;
					}
				}
			}
			if (c == '\"' || !c)
			{
				com_parser.com_token[len] = 0;
				*data_p                   = ( char * ) data;
				return com_parser.com_token;
			}
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_parser.com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			com_parser.com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if (c == '\n')
		{
			com_parser.com_lines++;
		}
	}
	while (c > 32);

	com_parser.com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_parser.com_token;
}

/**
 * @brief COM_Parse2
 * @param[in] data_p
 * @return
 */
char *COM_Parse2(char **data_p)
{
	return COM_ParseExt2(data_p, qtrue);
}

// *INDENT-OFF*
/**
 * @brief COM_ParseExt2
 * @param[in,out] data_p
 * @param[in] allowLineBreaks
 * @return
 */
char *COM_ParseExt2(char **data_p, qboolean allowLineBreaks)
{
	int        c           = 0, len;
	qboolean   hasNewLines = qfalse;
	char       *data;
	const char **punc;

	if (!data_p)
	{
		Com_Error(ERR_FATAL, "COM_ParseExt2: NULL data_p");
	}

	data         = *data_p;
	len          = 0;
	com_parser.com_token[0] = 0;

	// make sure incoming data is valid
	if (!data)
	{
		*data_p = NULL;
		return com_parser.com_token;
	}

	// backup the session data so we can unget easily
	COM_BackupParseSession(data_p);

	// skip whitespace
	while (1)
	{
		data = SkipWhitespace(data, &hasNewLines);
		if (!data)
		{
			*data_p = NULL;
			return com_parser.com_token;
		}
		if (hasNewLines && !allowLineBreaks)
		{
			*data_p = data;
			return com_parser.com_token;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			data += 2;
			while (*data && *data != '\n')
			{
				data++;
			}
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			data += 2;
			while (*data && (*data != '*' || data[1] != '/'))
			{
				data++;
			}
			if (*data)
			{
				data += 2;
			}
		}
		else
		{
			// a real token to parse
			break;
		}
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;

			if ((c == '\\') && (*data == '\"'))
			{
				// allow quoted strings to use \" to indicate the " character
				data++;
			}
			else if (c == '\"' || !c)
			{
				com_parser.com_token[len] = 0;
				*data_p        = (char *)data;
				return com_parser.com_token;
			}
			else if (*data == '\n')
			{
				com_parser.com_lines++;
			}

			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_parser.com_token[len] = c;
				len++;
			}
		}
	}

	// check for a number
	// is this parsing of negative numbers going to cause expression problems
	if ((c >= '0' && c <= '9') ||
	    (c == '-' && data[1] >= '0' && data[1] <= '9') ||
	    (c == '.' && data[1] >= '0' && data[1] <= '9') ||
	    (c == '-' && data[1] == '.' && data[2] >= '0' && data[2] <= '9'))
	{
		do
		{
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_parser.com_token[len] = c;
				len++;
			}
			data++;

			c = *data;
		}
		while ((c >= '0' && c <= '9') || c == '.');

		// parse the exponent
		if (c == 'e' || c == 'E')
		{
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_parser.com_token[len] = c;
				len++;
			}
			data++;
			c = *data;

			if (c == '-' || c == '+')
			{
				if (len < MAX_TOKEN_CHARS - 1)
				{
					com_parser.com_token[len] = c;
					len++;
				}
				data++;
				c = *data;
			}

			do
			{
				if (len < MAX_TOKEN_CHARS - 1)
				{
					com_parser.com_token[len] = c;
					len++;
				}
				data++;

				c = *data;
			}
			while (c >= '0' && c <= '9');
		}

		if (len == MAX_TOKEN_CHARS)
		{
			len = 0;
		}
		com_parser.com_token[len] = 0;

		*data_p = (char *)data;
		return com_parser.com_token;
	}

	// check for a regular word
	// we still allow forward and back slashes in name tokens for pathnames
	// and also colons for drive letters
	if ((c >= 'a' && c <= 'z') ||
	    (c >= 'A' && c <= 'Z') ||
	    (c == '_') ||
	    (c == '/') ||
	    (c == '\\') ||
	    (c == '$') || (c == '*')) // for bad shader strings
	{
		do
		{
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_parser.com_token[len] = c;
				len++;
			}
			data++;

			c = *data;
		}
		while
		((c >= 'a' && c <= 'z') ||
		 (c >= 'A' && c <= 'Z') ||
		 (c == '_') ||
		 (c == '-') ||
		 (c >= '0' && c <= '9') ||
		 (c == '/') ||
		 (c == '\\') ||
		 (c == ':') ||
		 (c == '.') ||
		 (c == '$') ||
		 (c == '*') ||
		 (c == '@'));

		if (len == MAX_TOKEN_CHARS)
		{
			len = 0;
		}
		com_parser.com_token[len] = 0;

		*data_p = (char *)data;
		return com_parser.com_token;
	}

	// check for multi-character punctuation token
	for (punc = punctuation; *punc; punc++)
	{
		size_t j, l;

		l = strlen(*punc);
		for (j = 0; j < l; j++)
		{
			if (data[j] != (*punc)[j])
			{
				break;
			}
		}
		if (j == l)
		{
			// a valid multi-character punctuation
			Com_Memcpy(com_parser.com_token, *punc, l);
			com_parser.com_token[l] = 0;
			data        += l;
			*data_p      = (char *)data;
			return com_parser.com_token;
		}
	}

	// single character punctuation
	com_parser.com_token[0] = *data;
	com_parser.com_token[1] = 0;
	data++;
	*data_p = (char *)data;

	return com_parser.com_token;
}
// *INDENT-ON*

/**
 * @brief COM_MatchToken
 * @param[in] buf_p
 * @param[in] match
 */
void COM_MatchToken(char **buf_p, char *match)
{
	char *token;

	token = COM_Parse(buf_p);
	if (strcmp(token, match))
	{
		Com_Error(ERR_DROP, "COM_MatchToken: %s != %s", token, match);
	}
}

/**
 * @brief SkipBracedSection_Depth
 * @param program
 * @param depth
 *
 * @note Unused
 */
void SkipBracedSection_Depth(char **program, int depth)
{
	char *token;

	do
	{
		token = COM_ParseExt(program, qtrue);
		if (token[1] == 0)
		{
			if (token[0] == '{')
			{
				depth++;
			}
			else if (token[0] == '}')
			{
				depth--;
			}
		}
	}
	while (depth && *program);
}

/**
 * @brief The next token should be an open brace.
 * Skips until a matching close brace is found.
 * Internal brace depths are properly skipped.
 *
 * @param[in] program
 */
void SkipBracedSection(char **program)
{
	char *token;
	int  depth = 0;

	do
	{
		token = COM_ParseExt(program, qtrue);
		if (token[1] == 0)
		{
			if (token[0] == '{')
			{
				depth++;
			}
			else if (token[0] == '}')
			{
				depth--;
			}
		}
	}
	while (depth && *program);
}

/**
 * @brief SkipRestOfLine
 * @param[in,out] data
 */
void SkipRestOfLine(char **data)
{
	char *p = *data;
	int  c;

	while ((c = *p) != '\0')
	{
		p++;
		if (c == '\n')
		{
			com_parser.com_lines++;
			break;
		}
	}

	*data = p;
}

/**
 * @brief Parse value and key from the string
 * @param[in,out] buf_p
 * @param[out] key
 * @param[out] value
 * @param[in] separator
 * @return true if key&value is valid
 *
 * @note Unused
 *
qboolean ParseKeyValue(char **buf_p, char *key, char *value, char separator)
{
	char   *token;
	size_t len;
	byte   found = qfalse;

	token = COM_Parse(buf_p);
	len   = strlen(token);

	if (!len)
	{
		return qfalse;
	}

	if (separator && token[len - 1] == separator)
	{
		token[len - 1] = '\0';
		found          = qtrue;
	}

	Q_strcpy(key, token);

	if (separator && !found)
	{
		token = COM_ParseExt(buf_p, qfalse);

		if (!token || token[0] != separator || token[1] != 0)
		{
			COM_ParseError("Expected to find %c but found %s", separator, token);
			key[0] = 0;
			return qfalse;
		}
	}

	token = COM_ParseExt(buf_p, qfalse);
	Q_strcpy(value, token);

	return qtrue;
}
*/

/**
 * @brief Parse1DMatrix
 * @param[in] buf_p
 * @param[in] x
 * @param[out] m
 */
void Parse1DMatrix(char **buf_p, int x, float *m)
{
	char *token;
	int  i;

	COM_MatchToken(buf_p, "(");

	for (i = 0 ; i < x ; i++)
	{
		token = COM_Parse(buf_p);
		m[i]  = Q_atof(token);
	}

	COM_MatchToken(buf_p, ")");
}

/**
 * @brief Parse2DMatrix
 * @param[in] buf_p
 * @param[in] y
 * @param[in] x
 * @param[out] m
 */
void Parse2DMatrix(char **buf_p, int y, int x, float *m)
{
	int i;

	COM_MatchToken(buf_p, "(");

	for (i = 0 ; i < y ; i++)
	{
		Parse1DMatrix(buf_p, x, m + i * x);
	}

	COM_MatchToken(buf_p, ")");
}

/**
 * @brief Parse3DMatrix
 * @param[in] buf_p
 * @param[in] z
 * @param[in] y
 * @param[in] x
 * @param[out] m
 *
 * @note Unused
 */
void Parse3DMatrix(char **buf_p, int z, int y, int x, float *m)
{
	int i;

	COM_MatchToken(buf_p, "(");

	for (i = 0 ; i < z ; i++)
	{
		Parse2DMatrix(buf_p, y, x, m + i * x * y);
	}

	COM_MatchToken(buf_p, ")");
}

/**
 * @brief Com_ParseInfos
 * @param[in] buf
 * @param[in] max
 * @param[out] infos
 * @return
 *
 * @note Unused
 */
int Com_ParseInfos(char *buf, int max, char infos[][MAX_INFO_STRING])
{
	const char *token;
	int        count = 0;
	char       key[MAX_TOKEN_CHARS];

	while (1)
	{
		token = COM_Parse(&buf);
		if (!token[0])
		{
			break;
		}
		if (strcmp(token, "{"))
		{
			Com_Printf("Com_ParseInfos: Missing { in info file\n");
			break;
		}

		if (count == max)
		{
			Com_Printf("Com_ParseInfos: Max infos exceeded\n");
			break;
		}

		infos[count][0] = 0;
		while (1)
		{
			token = COM_Parse(&buf);
			if (!token[0])
			{
				Com_Printf("Com_ParseInfos: Unexpected end of info file\n");
				break;
			}
			if (!strcmp(token, "}"))
			{
				break;
			}
			Q_strncpyz(key, token, sizeof(key));

			token = COM_ParseExt(&buf, qfalse);
			if (!token[0])
			{
				token = "<NULL>";
			}
			Info_SetValueForKey(infos[count], key, token);
		}
		count++;
	}

	return count;
}

/**
 * @brief Com_HexStrToInt
 * @param[in] str
 * @return
 */
int Com_HexStrToInt(const char *str)
{
	if (!str || !str[0])
	{
		return -1;
	}

	// check for hex code
	if (str[0] == '0' && str[1] == 'x')
	{
		unsigned int i;
		int          n = 0;

		for (i = 2; i < strlen(str); i++)
		{
			char digit;

			n *= 16;

			digit = tolower(str[i]);

			if (digit >= '0' && digit <= '9')
			{
				digit -= '0';
			}
			else if (digit >= 'a' && digit <= 'f')
			{
				digit = digit - 'a' + 10;
			}
			else
			{
				return -1;
			}

			n += digit;
		}

		return n;
	}

	return -1;
}

/*
============================================================================
                    LIBRARY REPLACEMENT FUNCTIONS
============================================================================
*/

/**
 * @brief Q_isprint
 * @param[in] c
 * @return
 */
int Q_isprint(int c)
{
	if (c >= 0x20 && c <= 0x7E)
	{
		return (1);
	}
	return (0);
}

/**
 * @brief Q_islower
 * @param[in] c
 * @return
 */
int Q_islower(int c)
{
	if (c >= 'a' && c <= 'z')
	{
		return (1);
	}
	return (0);
}

/**
 * @brief Q_isupper
 * @param[in] c
 * @return
 */
int Q_isupper(int c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return (1);
	}
	return (0);
}

/**
 * @brief Q_isalpha
 * @param[in] c
 * @return
 */
int Q_isalpha(int c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
	{
		return (1);
	}
	return (0);
}

/**
 * @brief Q_isnumeric
 * @param[in] c
 * @return
 */
int Q_isnumeric(int c)
{
	if (c >= '0' && c <= '9')
	{
		return (1);
	}
	return (0);
}

/**
 * @brief Q_isalphanumeric
 * @param[in] c
 * @return
 */
int Q_isalphanumeric(int c)
{
	if (Q_isalpha(c) ||
	    Q_isnumeric(c))
	{
		return(1);
	}
	return (0);
}

/**
 * @brief Q_isanumber
 * @param[in] s
 * @return
 */
qboolean Q_isanumber(const char *s)
{
	char              *p;
	double UNUSED_VAR d;

	if (*s == '\0')
	{
		return qfalse;
	}

	// FIXME: Why d was marked unused but was used ?
	d = strtod(s, &p);

	return *p == '\0';
}

/**
 * @brief Q_isintegral
 * @param[in] f
 * @return
 */
qboolean Q_isintegral(float f)
{
	return (int)f == f; // NOLINT(cppcoreguidelines-narrowing-conversions)
}

/**
 * @brief Q_isforfilename
 * @param[in] c
 * @return
 */
int Q_isforfilename(int c)
{
	if ((Q_isalphanumeric(c) || c == '_') && c != ' ')         // space not allowed in filename
	{
		return(1);
	}
	return (0);
}

/**
 * get rid of 0x80+ and '%' chars, because old clients don't like them
 * @param string buffer to check
 * @param len length of the buffer
 */
void Q_SafeNetString(char *string, size_t len, qboolean strip)
{
	int i = 0;
	for (; i < len; ++i)
	{
		if (!string[i])
		{
			break;
		}
		if ((strip && (byte)string[i] > 127) || string[i] == '%')
		{
			string[i] = '.';
		}
	}
}

#ifdef _MSC_VER

/**
 * @brief Special wrapper function for Microsoft's broken _vsnprintf() function.
 * MinGW comes with its own snprintf() which is not broken.
 * @param[in,out] str
 * @param[in] size
 * @param[in] format
 * @param[in] ap
 * @return
 */
int Q_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	int retval;

	retval = _vsnprintf(str, size, format, args);

	if (retval < 0 || retval == size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.

		str[size - 1] = '\0';
		return size;
	}

	return retval;
}
#endif

/**
 * @brief Safe strncpy that ensures a trailing zero
 * @param[out] dest
 * @param[in] src
 * @param[in] destsize
 */
void Q_strncpyz(char *dest, const char *src, size_t destsize)
{
	etl_assert(dest && src && destsize > 0 && dest != src);
	if (!dest)
	{
		Com_Error(ERR_FATAL, "Q_strncpyz: NULL dest");
	}
	if (!src)
	{
		Com_Error(ERR_FATAL, "Q_strncpyz: NULL src");
	}
	if (destsize < 1)
	{
		Com_Error(ERR_FATAL, "Q_strncpyz: destsize < 1");
	}

	strncpy(dest, src, destsize - 1);
	dest[destsize - 1] = 0;
}

/**
 * @brief Compare strings without case sensitivity up to n characters
 * @param[in] s1
 * @param[in] s2
 * @param[in] n
 * @return
 */
int Q_stricmpn(const char *s1, const char *s2, size_t n)
{
	int c1, c2;

	if (s1 == NULL)
	{
		if (s2 == NULL)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else if (s2 == NULL)
	{
		return 1;
	}

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
		{
			return 0;       // strings are equal until end point
		}

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
			{
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z')
			{
				c2 -= ('a' - 'A');
			}
			if (c1 != c2)
			{
				return c1 < c2 ? -1 : 1;
			}
		}
	}
	while (c1);

	return 0;       // strings are equal
}

/**
 * @brief Compare strings up to n characters
 * @param[in] s1
 * @param[in] s2
 * @param[in] n
 * @return
 */
int Q_strncmp(const char *s1, const char *s2, size_t n)
{
	int c1, c2;

	if (s1 == NULL)
	{
		if (s2 == NULL)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else if (s2 == NULL)
	{
		return 1;
	}

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
		{
			return 0;       // strings are equal until end point
		}

		if (c1 != c2)
		{
			return c1 < c2 ? -1 : 1;
		}
	}
	while (c1);

	return 0;       // strings are equal
}

/**
 * @brief Compare whole strings without case sensitivity
 * @param[in] s1
 * @param[in] s2
 * @return
 */
int Q_stricmp(const char *s1, const char *s2)
{
	return Q_stricmpn(s1, s2, 99999);
}

/**
 * @brief Q_strlwr
 * @param[in,out] s1
 * @return
 */
char *Q_strlwr(char *s1)
{
	char *s;

	for (s = s1; *s; ++s)
	{
		if (('A' <= *s) && (*s <= 'Z'))
		{
			*s -= 'A' - 'a';
		}
	}

	return s1;
}

/**
 * @brief Q_strupr
 * @param[in,out] s1
 * @return
 */
char *Q_strupr(char *s1)
{
	char *cp;

	for (cp = s1 ; *cp ; ++cp)
	{
		if (('a' <= *cp) && (*cp <= 'z'))
		{
			*cp += 'A' - 'a';
		}
	}

	return s1;
}

/**
 * @brief Q_strcat
 * @param[out] dest
 * @param[in] size
 * @param[in] src
 *
 * @note Never goes past bounds or leaves without a terminating 0
 */
void Q_strcat(char *dest, size_t size, const char *src)
{
	size_t l1;
	etl_assert(dest && src && size > 0 && dest != src);

	l1 = strlen(dest);
	if (l1 >= size)
	{
		Com_Error(ERR_FATAL, "Q_strcat: already overflowed");
	}
	Q_strncpyz(dest + l1, src, size - l1);
}

/**
 * @brief Find the first occurrence of find in s.
 * @param[in] s
 * @param[in] find
 * @return
 */
const char *Q_stristr(const char *s, const char *find)
{
	char c;

	if ((c = *find++) != 0)
	{
		char   sc;
		size_t len;

		if (c >= 'a' && c <= 'z')
		{
			c -= ('a' - 'A');
		}
		len = strlen(find);
		do
		{
			do
			{
				if ((sc = *s++) == 0)
				{
					return NULL;
				}
				if (sc >= 'a' && sc <= 'z')
				{
					sc -= ('a' - 'A');
				}
			}
			while (sc != c);
		}
		while (Q_stricmpn(s, find, len) != 0);
		s--;
	}
	return s;
}

/**
 * @brief Q_PrintStrlen
 * @param[in] string
 * @return
 */
int Q_PrintStrlen(const char *string)
{
	int        len;
	const char *p;

	if (!string)
	{
		return 0;
	}

	len = 0;
	p   = string;
	while (*p)
	{
		if (Q_IsColorString(p))
		{
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}

/**
 * @brief Q_TruncateStr
 * @param[in] string
 * @param[in] limit
 * @return
 */
char *Q_TruncateStr(char *string, int limit)
{
	char *p;
	char *s;
	int  i, len;

	if (!string)
	{
		return 0;
	}

	len = Q_PrintStrlen(string);
	if (len <= limit)
	{
		return string;
	}

	p = string;
	s = string;
	for (i = 0; i < limit; i++)
	{
		if (Q_IsColorString(p))
		{
			limit += 2;
			p     += 2;
			i++;
			continue;
		}
		p++;
	}

	limit++; // for null byte
	s[limit] = '\0';

	return s;
}

/**
 * @brief Remove all leading and trailing whitespace and special characters from string.
 * @param[in,out] string
 * @return
 */
char *Q_TrimStr(char *string)
{
	char   *s     = string;
	char   *start = string;
	size_t len    = 0;

	while (*s && (*s <= 0x20 || *s >= 0x7F || (Q_IsColorString(s) && *(s + 2) == 0x20)))
	{
		if (Q_IsColorString(s) && *(s + 2) == 0x20)
		{
			s++;
		}
		s++;
	}
	if (*s)
	{
		char *p = s;
		while (*p)
		{
			p++;
		}
		while (*p <= 0x20 || *p >= 0x7F)
		{
			--p;
		}

		if (*p)
		{
			p[1] = '\0';
		}
		len = (size_t) (p - s + 1);
	}

	return (s == start) ? s : memmove(start, s, len + 1);
}

/**
 * @brief Remove special characters and color sequences from string.
 * @param[in,out] string
 * @return
 */
char *Q_CleanStr(char *string)
{
	char *d = string;
	char *s = string;
	int  c;

	while ((c = *s) != 0)
	{
		if (Q_IsColorString(s))
		{
			s++;
		}
		else if (c >= 0x20 && c <= 0x7E)
		{
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/**
 * @brief Takes a plain "un-colored" string, and then colorizes it so the string is displayed in the given color.
 * If given a string such as "Bob" and asked to colorize to '1' (red)', the output would be "^1Bob". If given
 * "John^^7Candy" the output is "^1John^^1^^17Candy"  -- Note that when drawn, this would literally show
 * the text "John^^7Candy" in red.
 *
 * If the desired result is to see "John^Candy" in red, then create a clean un-colored string before calling this.
 *
 * REQUIREMENTS:
 *	- Callers must pass in a buffer that is *at least* 3 bytes long.
 *  - inStr and outStr cannot overlap
 *
 * @param[in] colorCode
 * @param[in] inStr
 * @param[out] outStr
 * @param[in] outBufferLen
 */
void Q_ColorizeString(char colorCode, const char *inStr, char *outStr, size_t outBufferLen)
{
	size_t inLen     = strlen(inStr);
	size_t outOffset = 0;
	size_t inOffset  = 0;

	if (outBufferLen < 3 || inStr == outStr)
	{
		etl_assert(qfalse);
		Com_Error(ERR_DROP, "Q_ColorizeString: invalid input data");
	}

	outStr[outOffset++] = Q_COLOR_ESCAPE;
	outStr[outOffset++] = colorCode;

	// Ok the buffer is way too small
	if (outOffset + 1 >= outBufferLen)
	{
		outStr[outOffset] = 0;
		return;
	}

	// There needs to be one extra char available in the output buffer for the terminating zero
	while (inOffset < inLen && outOffset + 1 < outBufferLen)
	{
		char c = inStr[inOffset];

		if (c == Q_COLOR_ESCAPE)
		{
			// chars plus possible terminator char
			if (outOffset + 4 < outBufferLen)
			{
				outStr[outOffset++] = c;
				outStr[outOffset++] = Q_COLOR_ESCAPE;
				outStr[outOffset++] = colorCode;
			}
			else
			{
				break;
			}
		}
		else
		{
			outStr[outOffset++] = c;
		}

		inOffset++;
	}

	outStr[outOffset] = 0;
}

int Q_StringEndsWith(const char *str, const char *suffix)
{
	size_t len_str, len_suffix;
	if (!str || !suffix)
	{
		return 0;
	}
	len_str    = strlen(str);
	len_suffix = strlen(suffix);
	if (len_suffix > len_str)
	{
		return 0;
	}
	return strncmp(str + len_str - len_suffix, suffix, len_suffix) == 0;
}

// Colors table (Only used locally)
const struct
{
	const char *colorName;
	vec4_t *color;
} OSP_Colortable[] =
{
	{ "white",    &colorWhite    },
	{ "red",      &colorRed      },
	{ "green",    &colorGreen    },
	{ "blue",     &colorBlue     },
	{ "yellow",   &colorYellow   },
	{ "magenta",  &colorMagenta  },
	{ "cyan",     &colorCyan     },
	{ "orange",   &colorOrange   },
	{ "mdred",    &colorMdRed    },
	{ "mdgreen",  &colorMdGreen  },
	{ "dkgreen",  &colorDkGreen  },
	{ "mdcyan",   &colorMdCyan   },
	{ "mdyellow", &colorMdYellow },
	{ "mdorange", &colorMdOrange },
	{ "mdblue",   &colorMdBlue   },
	{ "ltgrey",   &colorLtGrey   },
	{ "mdgrey",   &colorMdGrey   },
	{ "dkgrey",   &colorDkGrey   },
	{ "black",    &colorBlack    },
	{ NULL,       NULL           }
};
const int OSP_ColorTableSize = ARRAY_LEN(OSP_Colortable) - 1;

const char *Q_GetColorString(unsigned int offset)
{
	if (offset >= OSP_ColorTableSize)
	{
		return NULL;
	}

	return OSP_Colortable[offset].colorName;
}

/**
 * @brief Q_ParseColor supports hex (rrggbb<aa>), float (0.0 - 1.0), int (0 - 255) and color name formats
 * @param[in] colString string that contains the color info
 * @param[out] outColor output vector to set
 * @return count of tokens used for the color
 */
int Q_ParseColor(const char *colString, float *outColor)
{
	vec4_t     temp  = { 0, 0, 0, 1 };
	const char *s    = colString;
	int        count = 0;

	if (!colString || !colString[0] || !outColor)
	{
		return qfalse;
	}

	outColor[3] = 1.0f;

	// if there is a hex prefix
	if (*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X'))
	{
		s += 2;
	}
	// web color style prefix
	else if (*s == '#')
	{
		s += 1;
	}

	// parse rrggbb
	if (Q_IsHexColorString(s))
	{
		outColor[0] = ((float)(gethex(*(s)) * 16.f + gethex(*(s + 1)))) / 255.00f;
		outColor[1] = ((float)(gethex(*(s + 2)) * 16.f + gethex(*(s + 3)))) / 255.00f;
		outColor[2] = ((float)(gethex(*(s + 4)) * 16.f + gethex(*(s + 5)))) / 255.00f;

		if (Q_HexColorStringHasAlpha(s))
		{
			outColor[3] = ((float)(gethex(*(s + 6)) * 16 + gethex(*(s + 7)))) / 255.00f;
		}
		return 1;
	}
	else if ((count = Q_sscanf(s, "%f %f %f %f", &temp[0], &temp[1], &temp[2], &temp[3])) >= 3)
	{
		if (vec4_isIntegral(temp) && (temp[0] > 1 || temp[1] > 1 || temp[2] > 1 || temp[3] > 1))
		{
			vec4_scale(temp, 1.0f / 255.00f, temp);
			// restore the alpha value since it might have been squashed via the integral scaling
			if (count == 3)
			{
				temp[3] = outColor[3];
			}
		}
		ClampColor(temp);
		vec4_copy(temp, outColor);
		return count;
	}
	else
	{
		int i = 0;

		while (OSP_Colortable[i].colorName != NULL)
		{
			if (Q_stricmp(s, OSP_Colortable[i].colorName) == 0)
			{
				outColor[0] = (*OSP_Colortable[i].color)[0];
				outColor[1] = (*OSP_Colortable[i].color)[1];
				outColor[2] = (*OSP_Colortable[i].color)[2];
				return 1;
			}
			i++;
		}
	}

	return 0;
}

/**
 * @brief Strips whitespaces and bad characters
 * @param[in] c
 * @return
 */
qboolean Q_isBadDirChar(char c)
{
	char badchars[] = { ';', ':', '&', '(', ')', '|', '<', '>', '*', '?', '[', ']', '~', '+', '@', '!', '\\', '/', ' ', '\'', '\"', '\0' };
	int  i;

	for (i = 0; badchars[i] != '\0'; i++)
	{
		if (c == badchars[i])
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Q_CleanDirName
 * @param[in,out] dirname
 * @return
 */
char *Q_CleanDirName(char *dirname)
{
	char *d = dirname;
	char *s = dirname;

	// clear trailing .'s
	while (*s == '.')
	{
		s++;
	}

	while (*s != '\0')
	{
		if (!Q_isBadDirChar(*s))
		{
			*d++ = *s;
		}
		s++;
	}
	*d = '\0';

	return dirname;
}

/**
 * @brief Q_CountChar
 * @param[in] string
 * @param[in] tocount
 * @return
 */
int Q_CountChar(const char *string, char tocount)
{
	int count;

	for (count = 0; *string; string++)
	{
		if (*string == tocount)
		{
			count++;
		}
	}

	return count;
}

/**
 * @brief Q_GenerateHashValue
 * @param[in] fname
 * @param[in] size
 * @param[in] fullPath
 * @param[in] ignoreCase
 * @return
 */
long Q_GenerateHashValue(const char *fname, int size, qboolean fullPath, qboolean ignoreCase)
{
	int  i    = 0;
	long hash = 0;
	char letter;

	if (!fname)
	{
		Com_Error(ERR_DROP, "Q_GenerateHashValue: null name");
		return 0; // FIXME: never executed
	}

	while (fname[i] != '\0')
	{
		if (ignoreCase)
		{
			letter = (char)tolower(fname[i]);
		}
		else
		{
			letter = fname[i];
		}

		if (!fullPath)
		{
			if (letter == '.')
			{
				break; // don't include extension
			}
		}

		if (letter == '\\')
		{
			letter = '/'; // damn path names
		}

		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash  = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size - 1);
	return hash;
}

/**
 * @brief Com_sprintf
 * @param dest
 * @param size
 * @param fmt
 * @return
 */
int QDECL Com_sprintf(char *dest, unsigned int size, const char *fmt, ...)
{
	int     len;
	va_list argptr;

	va_start(argptr, fmt);
	len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end(argptr);

	if (len >= size)
	{
		Com_Printf(S_COLOR_RED "ERROR: " S_COLOR_GREEN "Com_sprintf output length %u too short, require %d bytes.\n", size, len + 1);
	}

	return len;
}

/*
 * @brief Does a varargs printf into a temp buffer, so I don't need to have
 * varargs versions of all text functions.
 * @param[in] format
 * @return
 *
 * @note Unused
char *QDECL va(const char *format, ...)
{
    va_list     argptr;
    static char string[2][32000]; // in case va is called by nested functions
    static int  index = 0;
    char        *buf;

    buf = string[index & 1];
    index++;

    va_start(argptr, format);
    Q_vsnprintf(buf, sizeof(*string), format, argptr);
    va_end(argptr);

    return buf;
}
*/

/**
 * @brief Does a varargs printf into a temp buffer, so I don't need to have
 * varargs versions of all text functions.
 *
 * @param[in] format
 *
 * @return
 *
 * @todo FIXME: make this buffer size safe someday
 *
 * @todo - modified this into a circular list, to further prevent stepping on
 * previous strings
 */
char *QDECL va(const char *format, ...)
{
	va_list       argptr;
	static char   temp_buffer[MAX_VA_STRING];
	static char   string[MAX_VA_STRING];  // in case va is called by nested functions
	static size_t index = 0;
	char          *buf;
	size_t        len;

	va_start(argptr, format);
	Q_vsnprintf(temp_buffer, MAX_VA_STRING, format, argptr);
	va_end(argptr);

	if ((len = strlen(temp_buffer)) >= MAX_VA_STRING)
	{
		Com_Error(ERR_DROP, "Attempted to overrun string in call to va()");
	}

	if (len + index >= MAX_VA_STRING - 1)
	{
		index = 0;
	}

	buf = &string[index];
	Com_Memcpy(buf, temp_buffer, len + 1);

	index += len + 1;

	return buf;
}

/**
 * @brief Assumes buffer is atleast TRUNCATE_LENGTH big
 * @param[out] buffer
 * @param[in] s
 */
void Com_TruncateLongString(char *buffer, const char *s)
{
	size_t length = strlen(s);

	if (length <= TRUNCATE_LENGTH)
	{
		Q_strncpyz(buffer, s, TRUNCATE_LENGTH);
	}
	else
	{
		Q_strncpyz(buffer, s, (TRUNCATE_LENGTH / 2) - 3);
		Q_strcat(buffer, TRUNCATE_LENGTH, " ... ");
		Q_strcat(buffer, TRUNCATE_LENGTH, s + length - (TRUNCATE_LENGTH / 2) + 3);
	}
}

/**
 * @brief This is just a convenience function
 * for making temporary vectors for function calls
 * @param[in] x
 * @param[in] y
 * @param[in] z
 * @return
 *
 * @see This is straight out of g_utils.c around line 210
 */
float *tv(float x, float y, float z)
{
	static int    index;
	static vec3_t vecs[8];
	float         *v;

	// use an array so that multiple temp vectors won't collide for a while
	v     = vecs[index];
	index = (index + 1) & 7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}

/*
=====================================================================
  INFO STRINGS
=====================================================================
*/

/**
 * @brief Searches the string for the given key and returns
 * the associated value, or an empty string.
 * @param[in] s
 * @param[in] key
 * @return
 */
char *Info_ValueForKey(const char *s, const char *key)
{
	char        pkey[BIG_INFO_KEY];
	static char value[2][BIG_INFO_VALUE];   // use two buffers so compares
	// work without stomping on each other
	static int valueindex = 0;
	char       *o;

	if (!s || !key)
	{
		return "";
	}

	if (strlen(s) >= BIG_INFO_STRING) // BIG_INFO_VALUE
	{
		Com_Error(ERR_DROP, "Info_ValueForKey: oversize infostring [%s] [%s]", s, key);
	}

	if (strlen(key) >= BIG_INFO_KEY)
	{
		Com_Error(ERR_DROP, "Info_ValueForKey: oversize key [%s] [%s]", s, key);
	}

	valueindex ^= 1;
	if (*s == '\\')
	{
		s++;
	}
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				return "";
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp(key, pkey))
		{
			return value[valueindex];
		}

		if (!*s)
		{
			break;
		}
		s++;
	}

	return "";
}

/**
 * @brief Used to iterate through all the key/value pairs in an info string
 * @param[in] head
 * @param[out] key
 * @param[out] value
 * @return qfalse if we discover the infostring is invalid
 */
qboolean Info_NextPair(const char **head, char *key, char *value)
{
	char       *o;
	const char *s = *head;

	if (*s == '\\')
	{
		s++;
	}
	key[0]   = 0;
	value[0] = 0;

	o = key;
	while (*s != '\\')
	{
		if (!*s)
		{
			key[0] = 0;
			*head  = s;
			return qtrue;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	// If they send us an empty key...where there is a slash after it then we know
	// the client has been messing around with the userinfo string...
	if (key[0] == 0)
	{
		return qfalse;
	}

	o = value;
	while (*s != '\\' && *s)
	{
		*o++ = *s++;
	}
	*o = 0;

	*head = s;

	return qtrue;
}

/**
 * @brief Info_RemoveKey
 * @param[in,out] s
 * @param[in] key
 */
qboolean Info_RemoveKey(char *s, const char *key)
{
	char *start;
	char pkey[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	char *o;

	if (strlen(s) >= MAX_INFO_STRING)
	{
		Com_Error(ERR_DROP, "Info_RemoveKey: oversize infostring [%s] [%s]", s, key);
	}

	if (strchr(key, '\\'))
	{
		return qfalse;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
		{
			s++;
		}
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				return qfalse;
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				return qfalse;
			}
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp(key, pkey))
		{
			memmove(start, s, strlen(s) + 1);     // TODO: remove this part ... why ?
			return qtrue;
		}

		if (!*s)
		{
			return qfalse;
		}
	}
}

/**
 * @brief Info_RemoveKey_Big
 * @param[in,out] s
 * @param[in] key
 */
void Info_RemoveKey_Big(char *s, const char *key)
{
	char *start;
	char pkey[BIG_INFO_KEY];
	char value[BIG_INFO_VALUE];
	char *o;

	if (strlen(s) >= BIG_INFO_STRING)
	{
		Com_Error(ERR_DROP, "Info_RemoveKey_Big: oversize infostring [%s] [%s]", s, key);
	}

	if (strchr(key, '\\'))
	{
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
		{
			s++;
		}
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				return;
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				return;
			}
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp(key, pkey))
		{
			memmove(start, s, strlen(s) + 1); // remove this part
			return;
		}

		if (!*s)
		{
			return;
		}
	}
}

/**
 * @brief Some characters are illegal in info strings because they
 * can mess up the server's parsing
 * @param[in] s
 * @return
 */
qboolean Info_Validate(const char *s)
{
	if (strchr(s, '\"'))
	{
		return qfalse;
	}
	if (strchr(s, ';'))
	{
		return qfalse;
	}
	return qtrue;
}

/**
 * @brief Changes or adds a key/value pair
 * @param[in,out] s
 * @param[in] key
 * @param[in] value
 */
void Info_SetValueForKey(char *s, const char *key, const char *value)
{
	char newi[MAX_INFO_STRING];

	if (!value || !strlen(value))
	{
		return;
	}

	if (strlen(s) >= MAX_INFO_STRING)
	{
		Com_Error(ERR_DROP, "Info_SetValueForKey: oversize infostring [%s] [%s] [%s]", s, key, value);
	}

	if (strchr(key, '\\') || strchr(value, '\\'))
	{
		Com_Printf("Info_SetValueForKey: Can't use keys or values with a \\\n");
		return;
	}

	if (strchr(key, ';') || strchr(value, ';'))
	{
		Com_Printf("Info_SetValueForKey: Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr(key, '\"') || strchr(value, '\"'))
	{
		Com_Printf("Info_SetValueForKey: Can't use keys or values with a \"\n");
		return;
	}

	Info_RemoveKey(s, key);

	Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= MAX_INFO_STRING)
	{
		Com_Printf("Info_SetValueForKey: Info string length exceeded\n");
		return;
	}

	Q_strcat(s, MAX_INFO_STRING, newi);
}

/**
 * @brief Changes or adds a key/value pair
 * @param[in,out] s
 * @param[in] key
 * @param[in] value
 */
void Info_SetValueForKey_Big(char *s, const char *key, const char *value)
{
	char newi[BIG_INFO_STRING];

	if (!value || !strlen(value))
	{
		return;
	}

	if (strlen(s) >= BIG_INFO_STRING)
	{
		Com_Error(ERR_DROP, "Info_SetValueForKey_Big: oversize infostring [%s] [%s] [%s]", s, key, value);
	}

	if (strchr(key, '\\') || strchr(value, '\\'))
	{
		Com_Printf("Info_SetValueForKey_Big: Can't use keys or values with a \\\n");
		return;
	}

	if (strchr(key, ';') || strchr(value, ';'))
	{
		Com_Printf("Info_SetValueForKey_Big: Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr(key, '\"') || strchr(value, '\"'))
	{
		Com_Printf("Info_SetValueForKey_Big: Can't use keys or values with a \"\n");
		return;
	}

	Info_RemoveKey_Big(s, key);

	Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > BIG_INFO_STRING)
	{
		Com_Printf("Info_SetValueForKey_Big: BIG Info string length exceeded\n");
		return;
	}

	Q_strcat(s, BIG_INFO_STRING, newi);
}

/**
 * @brief Q_StrReplace
 * @param[in,out] haystack
 * @param[in] needle
 * @param[in] newVal
 * @return
 */
char *Q_StrReplace(char *haystack, const char *needle, const char *newVal)
{
	static char final[MAX_STRING_CHARS] = { "" };
	char        dest[MAX_STRING_CHARS]  = { "" };
	char        new[MAX_STRING_CHARS]   = { "" };
	char        *destp;
	size_t      needle_len = 0;
	size_t      new_len    = 0;

	if (!haystack || !*haystack)
	{
		return final;
	}
	if (!needle || !*needle)
	{
		Q_strncpyz(final, haystack, sizeof(final));
		return final;
	}
	if (*newVal)
	{
		Q_strncpyz(new, newVal, sizeof(new));
	}

	dest[0]    = '\0';
	needle_len = strlen(needle);
	new_len    = strlen(new);
	destp      = &dest[0];
	while (*haystack)
	{
		if (!Q_stricmpn(haystack, needle, needle_len))
		{
			Q_strcat(dest, sizeof(dest), new);
			haystack += needle_len;
			destp    += new_len;
			continue;
		}
		if (MAX_STRING_CHARS > (strlen(dest) + 1))
		{
			*destp   = *haystack;
			*++destp = '\0';
		}
		haystack++;
	}
	// don't work with final return value in case haystack was pointing at it.
	Q_strncpyz(final, dest, sizeof(final));
	return final;
}

//====================================================================

/**
 * @brief Com_CharIsOneOfCharset
 * @param[in] c
 * @param[in] set
 * @return
 */
static qboolean Com_CharIsOneOfCharset(char c, const char *set)
{
	unsigned int i;

	for (i = 0; i < strlen(set); i++)
	{
		if (set[i] == c)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Com_SkipCharset
 * @param[in] s
 * @param[in] sep
 * @return
 */
char *Com_SkipCharset(char *s, char *sep)
{
	char *p = s;

	while (p)
	{
		if (Com_CharIsOneOfCharset(*p, sep))
		{
			p++;
		}
		else
		{
			break;
		}
	}

	return p;
}

/**
 * @brief Com_SkipTokens
 * @param[in] s
 * @param[in] numTokens
 * @param[in] sep
 * @return
 */
char *Com_SkipTokens(char *s, int numTokens, const char *sep)
{
	int  sepCount = 0;
	char *p       = s;

	while (sepCount < numTokens)
	{
		if (Com_CharIsOneOfCharset(*p++, sep))
		{
			sepCount++;
			while (Com_CharIsOneOfCharset(*p, sep))
				p++;
		}
		else if (*p == '\0')
		{
			break;
		}
	}

	if (sepCount == numTokens)
	{
		return p;
	}
	else
	{
		return s;
	}
}

#if defined(_MSC_VER) && (_MSC_VER < 1800)
/**
 * @brief rint
 * @param[in] v
 * @return
 */
float rint(float v)
{
	if (v >= 0.5f)
	{
		return ceilf(v);
	}
	else
	{
		return floorf(v);
	}
}
#endif

/**
 * @brief Q_LinearSearch
 * @param[in] key
 * @param[in] ptr
 * @param[in] count
 * @param[in] size
 * @param[in] cmp
 * @return
 *
 * @note Unused
 */
void *Q_LinearSearch(const void *key, const void *ptr, size_t count, size_t size, cmpFunc_t cmp)
{
	size_t i;

	for (i = 0; i < count; i++)
	{
		if (cmp(key, ptr) == 0)
		{
			return (void *)ptr;
		}

		ptr = (const char *)ptr + size;
	}
	return NULL;
}

/**
 * @brief Q_GetIPLength
 * @param ip
 * @return The length of the IP address if it has a port, or INT_MAX otherwise
 *
 * @todo FIXME: IPv6
 */
int GetIPLength(char const *ip)
{
	char *start = strchr(ip, ':');

	return (start == NULL ? INT_MAX : start - ip);
}

/**
 * @brief CompareIPNoPort
 * @param[in] ip1
 * @param[in] ip2
 * @return
 */
qboolean CompareIPNoPort(char const *ip1, char const *ip2)
{
	int checkLength = MIN(GetIPLength(ip1), GetIPLength(ip2));

	// Don't compare the port - just the IP
	if (checkLength < INT_MAX && !Q_strncmp(ip1, ip2, checkLength))
	{
		return qtrue;
	}
	else if (checkLength == INT_MAX && !strcmp(ip1, ip2))
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/**
 * @brief Com_AnyOf returns first valid pointer from the set
 * @param[in] ptr list of pointers
 * @param[in] n pointer count
 * @return first non-null pointer
 */
void *Com_AnyOf(void **ptr, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		if (ptr[i])
		{
			return ptr[i];
		}
	}
	return NULL;
}

/**
 * @brief Round a float value with n decimal
 * @param[in] value The value to round
 * @param[in] decimalCount The number of decimal to keep
 * @return The rounded values
 */
float Com_RoundFloatWithNDecimal(float value, unsigned int decimalCount)
{
	float        v;
	unsigned int n;

	// compute the number of decimal to keep
	n = pow(10.f, decimalCount);

	// rouding on n digits
	v = roundf(value * n) / n;

	// in case the value is between (-0.5) / n and 0, the rounding will compute -0
	// we don't want to display -0, so let replace it by a pure 0
	return v == -0.f ? 0.f : v;
}

/**
 * @brief Shortens a values by thousand, keeping wanted decimal values and sleepding at some
 * @details (i.e) decimalCount = 2 splitAtIntCount = 3 : "4560000" to "4.56M"
 * decimalCount = 1 splitAtIntCount = 4 : "7890" to "7890"
 * decimalCount = 1 splitAtIntCount = 4 : "12340" to "12.3k"
 * @param[in] value The number to shorten
 * @param[in] decimalCount The number of decimal to be displayed
 * @param[in] splitAtIntCount number of integer we want to start to split the value
 * @return Down scaled value string with suffixed unit
 */
char *Com_ScaleNumberPerThousand(float value, unsigned int decimalCount, unsigned int splitAtIntCount)
{
	static const char *units[] = { "", "k", "M", "G", "T" };
	unsigned int      i        = 0;

	while (value > pow(10.f, splitAtIntCount) && i < ARRAY_LEN(units))
	{
		value /= 1000;
		++i;
	}

	return va("%g%s", Com_RoundFloatWithNDecimal(value, decimalCount), units[i]);
}

/**
 * @brief Convert a string to an integer
 * @details Convert a string to an integer, with the same behavior that the engine converts
 * cvars to their integer representation:
 *   - Integer is obtained from concatenating all the integers in the string,
 *   regardless of the other characters present in the string ("-" is the exception,
 *   of there is a "-" before the first integer, the number is turned into a negative)
 *   - If there are no integers in the string, return 0
 * @param[in] src String to convert to an integer
 * @return Result of converted string to an integer
 */
int ExtractInt(const char *src)
{
	unsigned int i;
	unsigned int srclen = strlen(src) + 1;
	int          destIx = 0;
	char         *tmp   = Com_Allocate(srclen);
	int          result = 0;

	// Go through all the characters in the source string
	for (i = 0; i < srclen; i++)
	{
		// Pick out negative sign before first integer, or integers only
		if (((src[i] == '-') && (destIx == 0)) || Q_isnumeric(src[i]))
		{
			tmp[destIx++] = src[i];
		}
	}

	// put string terminator in temp var
	tmp[destIx] = 0;

	// convert temp var to integer
	if (tmp[0] != 0)
	{
		int sign = 1;

		result = sign * Q_atoi(tmp);
	}

	Com_Dealloc(tmp);

	return result;
}

int32_t Q_FloatToInt(float f)
{
	floatint_t fi;

	fi.f = f;
	return fi.i;
}

float Q_IntToFloat(int32_t i)
{
	floatint_t fi;

	fi.i = i;
	return fi.f;
}

/**
 * @brief Q_ParseInt Parse string to int and check if the string was numeric
 * @param[in] src String to parse
 * @param[in,out] out Parsed int
 * @return qtrue if the src string is numeric
 */
qboolean Q_ParseInt(const char *src, int *out)
{
	char *end;

	*out = (int)strtol(src, &end, 10);
	return !*end;
}
