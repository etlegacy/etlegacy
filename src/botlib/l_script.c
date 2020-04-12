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
 * @file l_script.c
 * @brief Lexicographical parser
 */

// include files for usage in the bot library
#include "../qcommon/q_shared.h"
#include "botlib.h"
#include "be_interface.h"
#include "l_script.h"
#include "l_memory.h"

#define PUNCTABLE

// longer punctuations first
punctuation_t default_punctuations[] =
{
	// binary operators
	{ ">>=", P_RSHIFT_ASSIGN,    NULL },
	{ "<<=", P_LSHIFT_ASSIGN,    NULL },
	//
	{ "...", P_PARMS,            NULL },
	// define merge operator
	{ "##",  P_PRECOMPMERGE,     NULL },
	// logic operators
	{ "&&",  P_LOGIC_AND,        NULL },
	{ "||",  P_LOGIC_OR,         NULL },
	{ ">=",  P_LOGIC_GEQ,        NULL },
	{ "<=",  P_LOGIC_LEQ,        NULL },
	{ "==",  P_LOGIC_EQ,         NULL },
	{ "!=",  P_LOGIC_UNEQ,       NULL },
	// arithmatic operators
	{ "*=",  P_MUL_ASSIGN,       NULL },
	{ "/=",  P_DIV_ASSIGN,       NULL },
	{ "%=",  P_MOD_ASSIGN,       NULL },
	{ "+=",  P_ADD_ASSIGN,       NULL },
	{ "-=",  P_SUB_ASSIGN,       NULL },
	{ "++",  P_INC,              NULL },
	{ "--",  P_DEC,              NULL },
	// binary operators
	{ "&=",  P_BIN_AND_ASSIGN,   NULL },
	{ "|=",  P_BIN_OR_ASSIGN,    NULL },
	{ "^=",  P_BIN_XOR_ASSIGN,   NULL },
	{ ">>",  P_RSHIFT,           NULL },
	{ "<<",  P_LSHIFT,           NULL },
	// reference operators
	{ "->",  P_POINTERREF,       NULL },
	// C++
	{ "::",  P_CPP1,             NULL },
	{ ".*",  P_CPP2,             NULL },
	// arithmatic operators
	{ "*",   P_MUL,              NULL },
	{ "/",   P_DIV,              NULL },
	{ "%",   P_MOD,              NULL },
	{ "+",   P_ADD,              NULL },
	{ "-",   P_SUB,              NULL },
	{ "=",   P_ASSIGN,           NULL },
	// binary operators
	{ "&",   P_BIN_AND,          NULL },
	{ "|",   P_BIN_OR,           NULL },
	{ "^",   P_BIN_XOR,          NULL },
	{ "~",   P_BIN_NOT,          NULL },
	// logic operators
	{ "!",   P_LOGIC_NOT,        NULL },
	{ ">",   P_LOGIC_GREATER,    NULL },
	{ "<",   P_LOGIC_LESS,       NULL },
	// reference operator
	{ ".",   P_REF,              NULL },
	// seperators
	{ ",",   P_COMMA,            NULL },
	{ ";",   P_SEMICOLON,        NULL },
	// label indication
	{ ":",   P_COLON,            NULL },
	//if statement
	{ "?",   P_QUESTIONMARK,     NULL },
	// embracements
	{ "(",   P_PARENTHESESOPEN,  NULL },
	{ ")",   P_PARENTHESESCLOSE, NULL },
	{ "{",   P_BRACEOPEN,        NULL },
	{ "}",   P_BRACECLOSE,       NULL },
	{ "[",   P_SQBRACKETOPEN,    NULL },
	{ "]",   P_SQBRACKETCLOSE,   NULL },

	{ "\\",  P_BACKSLASH,        NULL },
	// precompiler operator
	{ "#",   P_PRECOMP,          NULL },
#ifdef DOLLAR
	{ "$",   P_DOLLAR,           NULL },
#endif //DOLLAR
	{ NULL,  0,                  NULL }
};

char basefolder[MAX_QPATH];

/**
 * @brief PS_CreatePunctuationTable
 * @param[in,out] script
 * @param[in,out] punctuations
 */
void PS_CreatePunctuationTable(script_t *script, punctuation_t *punctuations)
{
	int           i;
	punctuation_t *p, *lastp, *newp;

	// get memory for the table
	if (!script->punctuationtable)
	{
		script->punctuationtable = (punctuation_t **)
		                           GetMemory(256 * sizeof(punctuation_t *));
	}
	Com_Memset(script->punctuationtable, 0, 256 * sizeof(punctuation_t *));
	// add the punctuations in the list to the punctuation table
	for (i = 0; punctuations[i].p; i++)
	{
		newp  = &punctuations[i];
		lastp = NULL;
		// sort the punctuations in this table entry on length (longer punctuations first)
		for (p = script->punctuationtable[(unsigned int) newp->p[0]]; p; p = p->next)
		{
			if (strlen(p->p) < strlen(newp->p))
			{
				newp->next = p;
				if (lastp)
				{
					lastp->next = newp;
				}
				else
				{
					script->punctuationtable[(unsigned int) newp->p[0]] = newp;
				}
				break;
			}
			lastp = p;
		}
		if (!p)
		{
			newp->next = NULL;
			if (lastp)
			{
				lastp->next = newp;
			}
			else
			{
				script->punctuationtable[(unsigned int) newp->p[0]] = newp;
			}
		}
	}
}

/**
 * @brief PunctuationFromNum
 * @param[in] script
 * @param[in] num
 * @return A pointer to the punctuation with the given number
 */
char *PunctuationFromNum(script_t *script, int num)
{
	int i;

	for (i = 0; script->punctuations[i].p; i++)
	{
		if (script->punctuations[i].n == num)
		{
			return script->punctuations[i].p;
		}
	}
	return "unknown punctuation";
}

/**
 * @brief Print a script error with filename and line number
 * @param[in] script
 * @param[in] str
 */
void QDECL ScriptError(script_t *script, const char *str, ...)
{
	char    text[1024];
	va_list ap;

	if (script->flags & SCFL_NOERRORS)
	{
		return;
	}

	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	botimport.Print(PRT_ERROR, "file %s, line %d: %s\n", script->filename, script->line, text);
}

/**
 * @brief Print a script warning with filename and line number
 * @param[in] script
 * @param[in] str
 */
void QDECL ScriptWarning(script_t *script, const char *str, ...)
{
	char    text[1024];
	va_list ap;

	if (script->flags & SCFL_NOWARNINGS)
	{
		return;
	}

	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	botimport.Print(PRT_WARNING, "file %s, line %d: %s\n", script->filename, script->line, text);
}

/**
 * @brief Set an array with punctuations, NULL restores default C/C++ set
 * @param[in,out] script
 * @param[in] p
 */
void SetScriptPunctuations(script_t *script, punctuation_t *p)
{
#ifdef PUNCTABLE
	if (p)
	{
		PS_CreatePunctuationTable(script, p);
	}
	else
	{
		PS_CreatePunctuationTable(script, default_punctuations);
	}
#endif //PUNCTABLE
	if (p)
	{
		script->punctuations = p;
	}
	else
	{
		script->punctuations = default_punctuations;
	}
}


/**
 * @brief Reads spaces, tabs, C-like comments etc.
 * When a newline character is found the scripts line counter is increased.
 *
 * @param[in,out] script
 * @return
 */
int PS_ReadWhiteSpace(script_t *script)
{
	while (1)
	{
		// skip white space
		while (*script->script_p <= ' ')
		{
			if (!*script->script_p)
			{
				return 0;
			}
			if (*script->script_p == '\n')
			{
				script->line++;
			}
			script->script_p++;
		}
		// skip comments
		if (*script->script_p == '/')
		{
			//comments //
			if (*(script->script_p + 1) == '/')
			{
				script->script_p++;
				do
				{
					script->script_p++;
					if (!*script->script_p)
					{
						return 0;
					}
				}
				while (*script->script_p != '\n');
				script->line++;
				script->script_p++;
				if (!*script->script_p)
				{
					return 0;
				}
				continue;
			}
			//comments /* */
			else if (*(script->script_p + 1) == '*')
			{
				script->script_p++;
				do
				{
					script->script_p++;
					if (!*script->script_p)
					{
						return 0;
					}
					if (*script->script_p == '\n')
					{
						script->line++;
					}
				}
				while (!(*script->script_p == '*' && *(script->script_p + 1) == '/'));
				script->script_p++;
				if (!*script->script_p)
				{
					return 0;
				}
				script->script_p++;
				if (!*script->script_p)
				{
					return 0;
				}
				continue;
			}
		}
		break;
	}
	return 1;
}

/**
 * @brief Reads an escape character.
 * @param[in,out] script Script to read from
 * @param[out] ch Place to store the read escape character
 * @return
 */
int PS_ReadEscapeCharacter(script_t *script, char *ch)
{
	int c, val, i;

	// step over the leading '\\'
	script->script_p++;
	// determine the escape character
	switch (*script->script_p)
	{
	case '\\': c = '\\'; break;
	case 'n': c  = '\n'; break;
	case 'r': c  = '\r'; break;
	case 't': c  = '\t'; break;
	case 'v': c  = '\v'; break;
	case 'b': c  = '\b'; break;
	case 'f': c  = '\f'; break;
	case 'a': c  = '\a'; break;
	case '\'': c = '\''; break;
	case '\"': c = '\"'; break;
	case '\?': c = '\?'; break;
	case 'x':
	{
		script->script_p++;
		for (i = 0, val = 0; ; i++, script->script_p++)
		{
			c = *script->script_p;
			if (c >= '0' && c <= '9')
			{
				c = c - '0';
			}
			else if (c >= 'A' && c <= 'Z')
			{
				c = c - 'A' + 10;
			}
			else if (c >= 'a' && c <= 'z')
			{
				c = c - 'a' + 10;
			}
			else
			{
				break;
			}
			val = (val << 4) + c;
		}
		script->script_p--;
		if (val > 0xFF)
		{
			ScriptWarning(script, "too large value in escape character");
			val = 0xFF;
		}
		c = val;
		break;
	}
	default:     // NOTE: decimal ASCII code, NOT octal
	{
		if (*script->script_p < '0' || *script->script_p > '9')
		{
			ScriptError(script, "unknown escape char");
		}
		for (i = 0, val = 0; ; i++, script->script_p++)
		{
			c = *script->script_p;
			if (c >= '0' && c <= '9')
			{
				c = c - '0';
			}
			else
			{
				break;
			}
			val = val * 10 + c;
		}
		script->script_p--;
		if (val > 0xFF)
		{
			ScriptWarning(script, "too large value in escape character");
			val = 0xFF;
		}
		c = val;
		break;
	}
	}

	// step over the escape character or the last digit of the number
	script->script_p++;
	// store the escape character
	*ch = (char)c;
	// succesfully read escape character
	return 1;
}


/**
 * @brief Reads C-like string.
 *
 * @details Escape characters are interpretted.
 * Quotes are included with the string.
 * Reads two strings with a white space between them as one string.
 *
 * @param[in,out] script Script to read from
 * @param[in,out] token	Buffer to store the string
 * @param[in] quote
 *
 * @return qtrue when a string was read succesfully
 */
int PS_ReadString(script_t *script, token_t *token, int quote)
{
	int  len, tmpline;
	char *tmpscript_p;

	if (quote == '\"')
	{
		token->type = TT_STRING;
	}
	else
	{
		token->type = TT_LITERAL;
	}

	len = 0;
	// leading quote
	token->string[len++] = *script->script_p++;

	while (1)
	{
		//minus 2 because trailing double quote and zero have to be appended
		if (len >= MAX_TOKEN - 2)
		{
			ScriptError(script, "string longer than MAX_TOKEN = %d", MAX_TOKEN);
			return 0;
		}
		// if there is an escape character and
		// if escape characters inside a string are allowed
		if (*script->script_p == '\\' && !(script->flags & SCFL_NOSTRINGESCAPECHARS))
		{
			if (!PS_ReadEscapeCharacter(script, &token->string[len]))
			{
				token->string[len] = 0;
				return 0;
			}
			len++;
		}
		// if a trailing quote
		else if (*script->script_p == quote)
		{
			// step over the double quote
			script->script_p++;
			// if white spaces in a string are not allowed
			if (script->flags & SCFL_NOSTRINGWHITESPACES)
			{
				break;
			}

			tmpscript_p = script->script_p;
			tmpline     = script->line;
			// read unusefull stuff between possible two following strings
			if (!PS_ReadWhiteSpace(script))
			{
				script->script_p = tmpscript_p;
				script->line     = tmpline;
				break;
			}
			// if there's no leading double qoute
			if (*script->script_p != quote)
			{
				script->script_p = tmpscript_p;
				script->line     = tmpline;
				break;
			}
			// step over the new leading double quote
			script->script_p++;
		}
		else
		{
			if (*script->script_p == '\0')
			{
				token->string[len] = 0;
				ScriptError(script, "missing trailing quote");
				return 0;
			}
			if (*script->script_p == '\n')
			{
				token->string[len] = 0;
				ScriptError(script, "newline inside string %s", token->string);
				return 0;
			}
			token->string[len++] = *script->script_p++;
		}
	}
	// trailing quote
	token->string[len++] = (char)quote;
	// end string with a zero
	token->string[len] = '\0';
	// the sub type is the length of the string
	token->subtype = len;
	return 1;
}

/**
 * @brief PS_ReadName
 * @param[in] script
 * @param[out] token
 * @return
 */
int PS_ReadName(script_t *script, token_t *token)
{
	int  len = 0;
	char c;

	token->type = TT_NAME;
	do
	{
		token->string[len++] = *script->script_p++;
		if (len >= MAX_TOKEN)
		{
			ScriptError(script, "name longer than MAX_TOKEN = %d", MAX_TOKEN);
			return 0;
		}
		c = *script->script_p;
	}
	while ((c >= 'a' && c <= 'z') ||
	       (c >= 'A' && c <= 'Z') ||
	       (c >= '0' && c <= '9') ||
	       c == '_');
	token->string[len] = '\0';
	// the sub type is the length of the name
	token->subtype = len;
	return 1;
}

/**
 * @brief NumberValue
 * @param[in,out] string
 * @param[in] subtype
 * @param[out] intvalue
 * @param[out] floatvalue
 */
void NumberValue(char *string, int subtype, int *intvalue,
                 float *floatvalue)
{
	*intvalue   = 0;
	*floatvalue = 0;
	// floating point number
	if (subtype & TT_FLOAT)
	{
		unsigned long int dotfound = 0;

		while (*string)
		{
			if (*string == '.')
			{
				if (dotfound)
				{
					return;
				}
				dotfound = 10;
				string++;
			}
			if (dotfound)
			{
				*floatvalue = *floatvalue + (float)(*string - '0') / (float)dotfound;
				dotfound   *= 10;
			}
			else
			{
				*floatvalue = *floatvalue * 10.0f + (float)(*string - '0');
			}
			string++;
		}
		*intvalue = (int)*floatvalue;
	}
	else if (subtype & TT_DECIMAL)
	{
		while (*string)
			*intvalue = *intvalue * 10 + (*string++ - '0');
		*floatvalue = (float)*intvalue;
	}
	else if (subtype & TT_HEX)
	{
		// step over the leading 0x or 0X
		string += 2;
		while (*string)
		{
			*intvalue <<= 4;
			if (*string >= 'a' && *string <= 'f')
			{
				*intvalue += *string - 'a' + 10;
			}
			else if (*string >= 'A' && *string <= 'F')
			{
				*intvalue += *string - 'A' + 10;
			}
			else
			{
				*intvalue += *string - '0';
			}
			string++;
		}
		*floatvalue = (float)*intvalue;
	}
	else if (subtype & TT_OCTAL)
	{
		// step over the first zero
		string += 1;
		while (*string)
			*intvalue = (*intvalue << 3) + (*string++ - '0');
		*floatvalue = (float)*intvalue;
	}
	else if (subtype & TT_BINARY)
	{
		// step over the leading 0b or 0B
		string += 2;
		while (*string)
			*intvalue = (*intvalue << 1) + (*string++ - '0');
		*floatvalue = (float)*intvalue;
	}
}

/**
 * @brief PS_ReadNumber
 * @param[in,out] script
 * @param[out] token
 * @return
 */
int PS_ReadNumber(script_t *script, token_t *token)
{
	int  len = 0, i;
	int  octal, dot;
	char c;
	//unsigned long int intvalue = 0;
	//long double floatvalue = 0;

	token->type = TT_NUMBER;
	// check for a hexadecimal number
	if (*script->script_p == '0' &&
	    (*(script->script_p + 1) == 'x' ||
	     *(script->script_p + 1) == 'X'))
	{
		token->string[len++] = *script->script_p++;
		token->string[len++] = *script->script_p++;
		c                    = *script->script_p;
		// hexadecimal
		while ((c >= '0' && c <= '9') ||
		       (c >= 'a' && c <= 'f') ||
		       (c >= 'A' && c <= 'F'))
		{
			token->string[len++] = *script->script_p++;
			if (len >= MAX_TOKEN)
			{
				ScriptError(script, "hexadecimal number longer than MAX_TOKEN = %d", MAX_TOKEN);
				return 0;
			}
			c = *script->script_p;
		}
		token->subtype |= TT_HEX;
	}
#ifdef BINARYNUMBERS
	// check for a binary number
	else if (*script->script_p == '0' &&
	         (*(script->script_p + 1) == 'b' ||
	          *(script->script_p + 1) == 'B'))
	{
		token->string[len++] = *script->script_p++;
		token->string[len++] = *script->script_p++;
		c                    = *script->script_p;
		// hexadecimal
		while (c == '0' || c == '1')
		{
			token->string[len++] = *script->script_p++;
			if (len >= MAX_TOKEN)
			{
				ScriptError(script, "binary number longer than MAX_TOKEN = %d", MAX_TOKEN);
				return 0;
			}
			c = *script->script_p;
		}
		token->subtype |= TT_BINARY;
	}
#endif //BINARYNUMBERS
	else // decimal or octal integer or floating point number
	{
		octal = qfalse;
		dot   = qfalse;
		if (*script->script_p == '0')
		{
			octal = qtrue;
		}
		while (1)
		{
			c = *script->script_p;
			if (c == '.')
			{
				dot = qtrue;
			}
			else if (c == '8' || c == '9')
			{
				octal = qfalse;
			}
			else if (c < '0' || c > '9')
			{
				break;
			}
			token->string[len++] = *script->script_p++;
			if (len >= MAX_TOKEN - 1)
			{
				ScriptError(script, "number longer than MAX_TOKEN = %d", MAX_TOKEN);
				return 0;
			}
		}
		if (octal)
		{
			token->subtype |= TT_OCTAL;
		}
		else
		{
			token->subtype |= TT_DECIMAL;
		}
		if (dot)
		{
			token->subtype |= TT_FLOAT;
		}
	}
	for (i = 0; i < 2; i++)
	{
		c = *script->script_p;
		// check for a LONG number
		if ((c == 'l' || c == 'L') &&
		    !(token->subtype & TT_LONG))
		{
			script->script_p++;
			token->subtype |= TT_LONG;
		}
		// check for an UNSIGNED number
		else if ((c == 'u' || c == 'U') &&
		         !(token->subtype & (TT_UNSIGNED | TT_FLOAT)))
		{
			script->script_p++;
			token->subtype |= TT_UNSIGNED;
		}
	}
	token->string[len] = '\0';
#ifdef NUMBERVALUE
	NumberValue(token->string, token->subtype, &token->intvalue, &token->floatvalue);
#endif //NUMBERVALUE
	if (!(token->subtype & TT_FLOAT))
	{
		token->subtype |= TT_INTEGER;
	}
	return 1;
}

/*
 * @brief PS_ReadLiteral
 * @param[in,out] script
 * @param[out] token
 * @return
 *
 * @note Unused
int PS_ReadLiteral(script_t *script, token_t *token)
{
    token->type = TT_LITERAL;
    // first quote
    token->string[0] = *script->script_p++;
    // check for end of file
    if (!*script->script_p)
    {
        ScriptError(script, "end of file before trailing \'");
        return 0;
    }
    // if it is an escape character
    if (*script->script_p == '\\')
    {
        if (!PS_ReadEscapeCharacter(script, &token->string[1]))
        {
            return 0;
        }
    }
    else
    {
        token->string[1] = *script->script_p++;
    }
    // check for trailing quote
    if (*script->script_p != '\'')
    {
        ScriptWarning(script, "too many characters in literal, ignored");
        while (*script->script_p &&
               *script->script_p != '\'' &&
               *script->script_p != '\n')
        {
            script->script_p++;
        }
        if (*script->script_p == '\'')
        {
            script->script_p++;
        }
    }
    // store the trailing quote
    token->string[2] = *script->script_p++;
    // store trailing zero to end the string
    token->string[3] = '\0';
    // the sub type is the integer literal value
    token->subtype = token->string[1];

    return 1;
}
*/

/**
 * @brief PS_ReadPunctuation
 * @param[in,out] script
 * @param[in,out] token
 * @return
 */
int PS_ReadPunctuation(script_t *script, token_t *token)
{
	size_t        len;
	char          *p;
	punctuation_t *punc;

#ifdef PUNCTABLE
	for (punc = script->punctuationtable[(unsigned int)*script->script_p]; punc; punc = punc->next)
	{
#else
	int i;

	for (i = 0; script->punctuations[i].p; i++)
	{
		punc = &script->punctuations[i];
#endif //PUNCTABLE
		p   = punc->p;
		len = strlen(p);
		// if the script contains at least as much characters as the punctuation
		if (script->script_p + len <= script->end_p)
		{
			// if the script contains the punctuation
			if (!strncmp(script->script_p, p, len))
			{
				Q_strncpyz(token->string, p, MAX_TOKEN);
				script->script_p += len;
				token->type       = TT_PUNCTUATION;
				// sub type is the number of the punctuation
				token->subtype = punc->n;
				return 1;
			}
		}
	}
	return 0;
}

/**
 * @brief PS_ReadPrimitive
 * @param[out] script
 * @param[in,out] token
 * @return
 */
int PS_ReadPrimitive(script_t *script, token_t *token)
{
	int len;

	len = 0;
	while (*script->script_p > ' ' && *script->script_p != ';')
	{
		if (len >= MAX_TOKEN - 1)
		{
			ScriptError(script, "primitive token longer than MAX_TOKEN = %d", MAX_TOKEN);
			return 0;
		}
		token->string[len++] = *script->script_p++;
	}
	token->string[len] = 0;
	// copy the token into the script structure
	Com_Memcpy(&script->token, token, sizeof(token_t));
	// primitive reading successfull
	return 1;
}

/**
 * @brief Read a token from the script
 * @param[in,out] script
 * @param[in,out] token
 * @return
 */
int PS_ReadToken(script_t *script, token_t *token)
{
	// if there is a token available (from UnreadToken)
	if (script->tokenavailable)
	{
		script->tokenavailable = 0;
		Com_Memcpy(token, &script->token, sizeof(token_t));
		return 1;
	}
	//save script pointer
	script->lastscript_p = script->script_p;
	// save line counter
	script->lastline = script->line;
	// clear the token stuff
	Com_Memset(token, 0, sizeof(token_t));
	// start of the white space
	script->whitespace_p = script->script_p;
	token->whitespace_p  = script->script_p;
	// read unusefull stuff
	if (!PS_ReadWhiteSpace(script))
	{
		return 0;
	}
	// end of the white space
	script->endwhitespace_p = script->script_p;
	token->endwhitespace_p  = script->script_p;
	// line the token is on
	token->line = script->line;
	// number of lines crossed before token
	token->linescrossed = script->line - script->lastline;
	// if there is a leading double quote
	if (*script->script_p == '\"')
	{
		if (!PS_ReadString(script, token, '\"'))
		{
			return 0;
		}
	}
	// if an literal
	else if (*script->script_p == '\'')
	{
		//if (!PS_ReadLiteral(script, token)) return 0;
		if (!PS_ReadString(script, token, '\''))
		{
			return 0;
		}
	}
	// if there is a number
	else if ((*script->script_p >= '0' && *script->script_p <= '9') ||
	         (*script->script_p == '.' &&
	          (*(script->script_p + 1) >= '0' && *(script->script_p + 1) <= '9')))
	{
		if (!PS_ReadNumber(script, token))
		{
			return 0;
		}
	}
	// if this is a primitive script
	else if (script->flags & SCFL_PRIMITIVE)
	{
		return PS_ReadPrimitive(script, token);
	}
	// if there is a name
	else if ((*script->script_p >= 'a' && *script->script_p <= 'z') ||
	         (*script->script_p >= 'A' && *script->script_p <= 'Z') ||
	         *script->script_p == '_')
	{
		if (!PS_ReadName(script, token))
		{
			return 0;
		}
	}
	// check for punctuations
	else if (!PS_ReadPunctuation(script, token))
	{
		ScriptError(script, "can't read token");
		return 0;
	}
	// copy the token into the script structure
	Com_Memcpy(&script->token, token, sizeof(token_t));
	// succesfully read a token
	return 1;
}

/*
 * @brief Expect a certain token
 * @param[in] script
 * @param[in] string
 * @return
 *
 * @note Unused
int PS_ExpectTokenString(script_t *script, const char *string)
{
    token_t token;

    if (!PS_ReadToken(script, &token))
    {
        ScriptError(script, "couldn't find expected %s", string);
        return 0;
    }

    if (strcmp(token.string, string))
    {
        ScriptError(script, "expected %s, found %s", string, token.string);
        return 0;
    }
    return 1;
}
*/

/*
 * @brief Expect a certain token type
 * @param[in] script
 * @param[in] type
 * @param[in] subtype
 * @param[in] token
 * @return
 *
 * @note Unused
int PS_ExpectTokenType(script_t *script, int type, int subtype, token_t *token)
{
    if (!PS_ReadToken(script, token))
    {
        ScriptError(script, "couldn't read expected token");
        return 0;
    }

    if (token->type != type)
    {
        char str[MAX_TOKEN];

        strcpy(str, "");
        if (type == TT_STRING)
        {
            strcpy(str, "string");
        }
        if (type == TT_LITERAL)
        {
            strcpy(str, "literal");
        }
        if (type == TT_NUMBER)
        {
            strcpy(str, "number");
        }
        if (type == TT_NAME)
        {
            strcpy(str, "name");
        }
        if (type == TT_PUNCTUATION)
        {
            strcpy(str, "punctuation");
        }
        ScriptError(script, "expected a %s, found %s", str, token->string);
        return 0;
    }
    if (token->type == TT_NUMBER)
    {
        if ((token->subtype & subtype) != subtype)
        {
            char str[MAX_TOKEN];

            strcpy(str, "");
            if (subtype & TT_DECIMAL)
            {
                strcpy(str, "decimal");
            }
            if (subtype & TT_HEX)
            {
                strcpy(str, "hex");
            }
            if (subtype & TT_OCTAL)
            {
                strcpy(str, "octal");
            }
            if (subtype & TT_BINARY)
            {
                strcpy(str, "binary");
            }
            if (subtype & TT_LONG)
            {
                strcat(str, " long");
            }
            if (subtype & TT_UNSIGNED)
            {
                strcat(str, " unsigned");
            }
            if (subtype & TT_FLOAT)
            {
                strcat(str, " float");
            }
            if (subtype & TT_INTEGER)
            {
                strcat(str, " integer");
            }
            ScriptError(script, "expected %s, found %s", str, token->string);
            return 0;
        }
    }
    else if (token->type == TT_PUNCTUATION)
    {
        if (subtype < 0)
        {
            ScriptError(script, "BUG: wrong punctuation subtype");
            return 0;
        }
        if (token->subtype != subtype)
        {
            ScriptError(script, "expected %s, found %s",
                        script->punctuations[subtype].p, token->string);
            return 0;
        }
    }
    return 1;
}
*/

/*
 * @brief Expect a token
 * @param[in] script
 * @param[in] token
 * @return
 *
 * @note Unused
int PS_ExpectAnyToken(script_t *script, token_t *token)
{
    if (!PS_ReadToken(script, token))
    {
        ScriptError(script, "couldn't read expected token");
        return 0;
    }
    else
    {
        return 1;
    }
}
*/

/*
 * @brief Check if token is available
 * @param[in,out] script
 * @param[in] string
 * @return true when the token is available
 *
 * @note Unused
int PS_CheckTokenString(script_t *script, char *string)
{
    token_t tok;

    if (!PS_ReadToken(script, &tok))
    {
        return 0;
    }
    // if the token is available
    if (!strcmp(tok.string, string))
    {
        return 1;
    }
    // token not available
    script->script_p = script->lastscript_p;
    return 0;
}
*/

/*
 * @brief Check token type
 * @param[in,out] script
 * @param[in] type
 * @param[in] subtype
 * @param[out] token
 * @return returns true and reads the token when a token with the given type is available
 *
 * @note Unused
int PS_CheckTokenType(script_t *script, int type, int subtype, token_t *token)
{
    token_t tok;

    if (!PS_ReadToken(script, &tok))
    {
        return 0;
    }
    // if the type matches
    if (tok.type == type &&
        (tok.subtype & subtype) == subtype)
    {
        Com_Memcpy(token, &tok, sizeof(token_t));
        return 1;
    }
    // token is not available
    script->script_p = script->lastscript_p;
    return 0;
}
*/

/*
 * @brief Skip tokens until the given token string is read
 * @param[in] script
 * @param[in] string
 * @return
 *
 * @note Unused
int PS_SkipUntilString(script_t *script, const char *string)
{
    token_t token;

    while (PS_ReadToken(script, &token))
    {
        if (!strcmp(token.string, string))
        {
            return 1;
        }
    }
    return 0;
}
*/

/*
 * @brief Unread the last token read from the script
 * @param[out] script
 *
 * @note Unused
void PS_UnreadLastToken(script_t *script)
{
    script->tokenavailable = 1;
}
*/

/*
 * @brief Unread the given token
 * @param[out] script
 * @param[in] token
 *
 * @note Unused
void PS_UnreadToken(script_t *script, token_t *token)
{
    Com_Memcpy(&script->token, token, sizeof(token_t));
    script->tokenavailable = 1;
}
*/

/*
 * @brief PS_NextWhiteSpaceChar
 * @param script
 * @return The next character of the read white space, returns NULL if none
 *
 * @note Unused
char PS_NextWhiteSpaceChar(script_t *script)
{
    if (script->whitespace_p != script->endwhitespace_p)
    {
        return *script->whitespace_p++;
    }
    else
    {
        return 0;
    }
}
*/

/**
 * @brief Remove any leading and trailing double quotes from the token
 * @param[in,out] string
 */
void StripDoubleQuotes(char *string)
{
	if (*string == '\"')
	{
		memmove(string, string + 1, strlen(string));
	}
	if (string[strlen(string) - 1] == '\"')
	{
		string[strlen(string) - 1] = '\0';
	}
}

/*
 * @brief Remove any leading and trailing single quotes from the token
 * @param[in,out] string
 *
 * @note Unused
void StripSingleQuotes(char *string)
{
    if (*string == '\'')
    {
        memmove(string, string + 1, strlen(string));
    }
    if (string[strlen(string) - 1] == '\'')
    {
        string[strlen(string) - 1] = '\0';
    }
}
*/

/*
 * @brief Read a possible signed floating point number
 * @param[in] script
 * @return
 *
 * @note Unused
long double ReadSignedFloat(script_t *script)
{
    token_t     token;
    long double sign = 1;

    PS_ExpectAnyToken(script, &token);
    if (!strcmp(token.string, "-"))
    {
        sign = -1;
        PS_ExpectTokenType(script, TT_NUMBER, 0, &token);
    }
    else if (token.type != TT_NUMBER)
    {
        ScriptError(script, "expected float value, found %s\n", token.string);
    }
    return sign * token.floatvalue;
}
*/

/*
 * @brief Read a possible signed integer
 * @param[in] script
 * @return
 *
 * @note Unused
signed long int ReadSignedInt(script_t *script)
{
    token_t         token;
    signed long int sign = 1;

    PS_ExpectAnyToken(script, &token);
    if (!strcmp(token.string, "-"))
    {
        sign = -1;
        PS_ExpectTokenType(script, TT_NUMBER, TT_INTEGER, &token);
    }
    else if (token.type != TT_NUMBER || token.subtype == TT_FLOAT)
    {
        ScriptError(script, "expected integer value, found %s\n", token.string);
    }
    return sign * token.intvalue;
}
*/

/*
 * @brief Set script flags
 * @param[out] script
 * @param[in] flags
 *
 * @note Unused
void SetScriptFlags(script_t *script, int flags)
{
    script->flags = flags;
}
*/

/*
 * @brief Get script flags
 * @param[in] script
 * @return Script flags
 *
 * @note Unused
int GetScriptFlags(script_t *script)
{
    return script->flags;
}
*/

/*
 * @brief ResetScript
 * @param[in,out] script
 *
 * @note Unused
void ResetScript(script_t *script)
{
    // pointer in script buffer
    script->script_p = script->buffer;
    // pointer in script buffer before reading token
    script->lastscript_p = script->buffer;
    // begin of white space
    script->whitespace_p = NULL;
    // end of white space
    script->endwhitespace_p = NULL;
    // set if there's a token available in script->token
    script->tokenavailable = 0;

    script->line     = 1;
    script->lastline = 1;
    // clear the saved token
    Com_Memset(&script->token, 0, sizeof(token_t));
}
*/

/**
 * @brief EndOfScript
 * @param[in,out] script
 * @return
 */
int EndOfScript(script_t *script)
{
	return script->script_p >= script->end_p;
}

/*
 * @brief NumLinesCrossed
 * @param[in,out] script
 * @return
 *
 * @note Unused
int NumLinesCrossed(script_t *script)
{
    return script->line - script->lastline;
}
*/

/*
 * @brief ScriptSkipTo
 * @param[in,out] script
 * @param[in] value
 * @return True if at the end of the script
 *
 * @note Unused
int ScriptSkipTo(script_t *script, const char *value)
{
    int  len;
    char firstchar;

    firstchar = *value;
    len       = strlen(value);
    do
    {
        if (!PS_ReadWhiteSpace(script))
        {
            return 0;
        }
        if (*script->script_p == firstchar)
        {
            if (!strncmp(script->script_p, value, len))
            {
                return 1;
            }
        }
        script->script_p++;
    }
    while (1);

    return 0; // never reached
}
*/

/**
 * @brief Load a script from the given file at the given offset with the given length
 * @param[in] filename
 * @return
 */
script_t *LoadScriptFile(const char *filename)
{
	fileHandle_t fp;
	char         pathname[MAX_QPATH];
	int          length;
	void         *buffer;
	script_t     *script;

	if (strlen(basefolder))
	{
		Com_sprintf(pathname, sizeof(pathname), "%s/%s", basefolder, filename);
	}
	else
	{
		Com_sprintf(pathname, sizeof(pathname), "%s", filename);
	}
	length = botimport.FS_FOpenFile(pathname, &fp, FS_READ);
	if (!fp)
	{
		//botimport.Print(PRT_WARNING, "LoadScriptFile: File %s not found!\n", pathname);
		return NULL;
	}

	buffer = GetClearedMemory(sizeof(script_t) + (unsigned int)length + 1);
	script = (script_t *) buffer;
	Com_Memset(script, 0, sizeof(script_t));
	Q_strncpyz(script->filename, filename, sizeof(script->filename));
	script->buffer         = (char *) buffer + sizeof(script_t);
	script->buffer[length] = 0;
	script->length         = length;
	// pointer in script buffer
	script->script_p = script->buffer;
	// pointer in script buffer before reading token
	script->lastscript_p = script->buffer;
	// pointer to end of script buffer
	script->end_p = &script->buffer[length];
	// set if there's a token available in script->token
	script->tokenavailable = 0;

	script->line     = 1;
	script->lastline = 1;

	SetScriptPunctuations(script, NULL);

	botimport.FS_Read(script->buffer, length, fp);
	botimport.FS_FCloseFile(fp);

	return script;
}

/**
 * @brief Load a script from the given memory with the given length
 * @param[in] ptr
 * @param[in] length
 * @param[in] name
 * @return
 */
script_t *LoadScriptMemory(const char *ptr, int length, const char *name)
{
	void     *buffer;
	script_t *script;

	buffer = GetClearedMemory(sizeof(script_t) + (unsigned int)length + 1);
	script = (script_t *) buffer;
	Com_Memset(script, 0, sizeof(script_t));
	Q_strncpyz(script->filename, name, sizeof(script->filename));
	script->buffer         = (char *) buffer + sizeof(script_t);
	script->buffer[length] = 0;
	script->length         = length;
	// pointer in script buffer
	script->script_p = script->buffer;
	// pointer in script buffer before reading token
	script->lastscript_p = script->buffer;
	// pointer to end of script buffer
	script->end_p = &script->buffer[length];
	// set if there's a token available in script->token
	script->tokenavailable = 0;

	script->line     = 1;
	script->lastline = 1;

	SetScriptPunctuations(script, NULL);

	Com_Memcpy(script->buffer, ptr, (unsigned int)length);

	return script;
}

/**
 * @brief Free a script
 * @param[in,out] script
 */
void FreeScript(script_t *script)
{
#ifdef PUNCTABLE
	if (script->punctuationtable)
	{
		FreeMemory(script->punctuationtable);
	}
#endif //PUNCTABLE
	FreeMemory(script);
}

/**
 * @brief Set the base folder to load files from
 * @param[in] path
 */
void PS_SetBaseFolder(const char *path)
{
	Com_sprintf(basefolder, sizeof(basefolder), "%s", path);
}
