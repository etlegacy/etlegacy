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
 * @file l_precomp.c
 * @brief Pre-compiler
 */

// FIXME: PC_StringizeTokens ?!

#include "../qcommon/q_shared.h"
#include "botlib.h"
#include "be_interface.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"

//#define DEBUG_EVAL

#define MAX_DEFINEPARMS         128

#define DEFINEHASHING           1

// directive name with parse function
typedef struct directive_s
{
	char *name;
	int (*func)(source_t *source);
} directive_t;

#define DEFINEHASHSIZE      1024

int numtokens;

// list with global defines added to every source loaded
define_t *globaldefines;

/**
 * @brief Print a source error
 * @param[in] source
 * @param[in] str
 */
void QDECL SourceError(source_t *source, const char *str, ...)
{
	char    text[1024];
	va_list ap;

	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	botimport.Print(PRT_ERROR, "file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text);
}

/**
 * @brief Print a source warning
 * @param[in] source
 * @param[in] str
 */
void QDECL SourceWarning(source_t *source, const char *str, ...)
{
	char    text[1024];
	va_list ap;

	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	botimport.Print(PRT_WARNING, "file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text);
}

/**
 * @brief PC_PushIndent
 * @param[in] source
 * @param[in] type
 * @param[in] skip
 */
void PC_PushIndent(source_t *source, int type, int skip)
{
	indent_t *indent;

	indent              = (indent_t *) GetMemory(sizeof(indent_t));
	indent->type        = type;
	indent->script      = source->scriptstack;
	indent->skip        = (skip != 0);
	source->skip       += indent->skip;
	indent->next        = source->indentstack;
	source->indentstack = indent;
}

/**
 * @brief PC_PopIndent
 * @param[in] source
 * @param[in] type
 * @param[in] skip
 */
void PC_PopIndent(source_t *source, int *type, int *skip)
{
	indent_t *indent;

	*type = 0;
	*skip = 0;

	indent = source->indentstack;
	if (!indent)
	{
		return;
	}

	// must be an indent from the current script
	if (source->indentstack->script != source->scriptstack)
	{
		return;
	}

	*type               = indent->type;
	*skip               = indent->skip;
	source->indentstack = source->indentstack->next;
	source->skip       -= indent->skip;
	FreeMemory(indent);
}

/**
 * @brief PC_PushScript
 * @param[in,out] source
 * @param[in,out] script
 */
void PC_PushScript(source_t *source, script_t *script)
{
	script_t *s;

	for (s = source->scriptstack; s; s = s->next)
	{
		if (!Q_stricmp(s->filename, script->filename))
		{
			SourceError(source, "%s recursively included", script->filename);
			return;
		}
	}
	// push the script on the script stack
	script->next        = source->scriptstack;
	source->scriptstack = script;
}

/**
 * @brief PC_CopyToken
 * @param[in] token
 * @return
 */
token_t *PC_CopyToken(token_t *token)
{
	token_t *t;

	t = (token_t *) GetMemory(sizeof(token_t));
	if (!t)
	{
		Com_Error(ERR_FATAL, "out of token space");
		return NULL;
	}
	//freetokens = freetokens->next;
	Com_Memcpy(t, token, sizeof(token_t));
	t->next = NULL;
	numtokens++;
	return t;
}

/**
 * @brief PC_FreeToken
 * @param[in] token
 */
void PC_FreeToken(token_t *token)
{
	FreeMemory(token);
	numtokens--;
}

/**
 * @brief PC_ReadSourceToken
 * @param[in,out] source
 * @param[in] token
 * @return
 */
int PC_ReadSourceToken(source_t *source, token_t *token)
{
	token_t  *t;
	script_t *script;
	int      type, skip;

	// if there's no token already available
	while (!source->tokens)
	{
		//if there's a token to read from the script
		if (PS_ReadToken(source->scriptstack, token))
		{
			return qtrue;
		}
		// if at the end of the script
		if (EndOfScript(source->scriptstack))
		{
			// remove all indents of the script
			while (source->indentstack &&
			       source->indentstack->script == source->scriptstack)
			{
				SourceWarning(source, "missing #endif");
				PC_PopIndent(source, &type, &skip);
			}
		}
		// if this was the initial script
		if (!source->scriptstack->next)
		{
			return qfalse;
		}
		// remove the script and return to the last one
		script              = source->scriptstack;
		source->scriptstack = source->scriptstack->next;
		FreeScript(script);
	}
	// copy the already available token
	Com_Memcpy(token, source->tokens, sizeof(token_t));
	// free the read token
	t              = source->tokens;
	source->tokens = source->tokens->next;
	PC_FreeToken(t);
	return qtrue;
}

/**
 * @brief PC_UnreadSourceToken
 * @param[in,out] source
 * @param[in] token
 * @return
 */
int PC_UnreadSourceToken(source_t *source, token_t *token)
{
	token_t *t;

	t              = PC_CopyToken(token);
	t->next        = source->tokens;
	source->tokens = t;
	return qtrue;
}

define_t *PC_FindHashedDefine(define_t **definehash, const char *name);
int PC_ExpandDefineIntoSource(source_t *source, token_t *deftoken, define_t *define);

/**
 * @brief PC_ReadDefineParms
 * @param[in] source
 * @param[in] define
 * @param[in,out] parms
 * @param[in] maxparms
 * @return
 */
int PC_ReadDefineParms(source_t *source, define_t *define, token_t **parms, int maxparms)
{
	token_t  token, *t, *last;
	int      i, done, lastcomma, numparms, indent;
	define_t *newdefine;

	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "define %s missing parms", define->name);
		return qfalse;
	}

	if (define->numparms > maxparms)
	{
		SourceError(source, "define with more than %d parameters", maxparms);
		return qfalse;
	}

	for (i = 0; i < define->numparms; i++)
	{
		parms[i] = NULL;
	}
	// if no leading "("
	if (strcmp(token.string, "("))
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "define %s missing parms", define->name);
		return qfalse;
	}
	// read the define parameters
	for (done = 0, numparms = 0, indent = 1; !done; )
	{
		if (numparms >= maxparms)
		{
			SourceError(source, "define %s with too many parms", define->name);
			return qfalse;
		}
		if (numparms >= define->numparms)
		{
			SourceWarning(source, "define %s has too many parms", define->name);
			return qfalse;
		}
		parms[numparms] = NULL;
		lastcomma       = 1;
		last            = NULL;
		while (!done)
		{
			if (!PC_ReadSourceToken(source, &token))
			{
				SourceError(source, "define %s incomplete", define->name);
				return qfalse;
			}

			if (!strcmp(token.string, ","))
			{
				if (indent <= 1)
				{
					if (lastcomma)
					{
						SourceWarning(source, "too many comma's");
					}
					// lastcomma = 1; // FIXME: lastcomma is never read !
					break;
				}
			}
			lastcomma = 0;

			if (!strcmp(token.string, "("))
			{
				indent++;
			}
			else if (!strcmp(token.string, ")"))
			{
				if (--indent <= 0)
				{
					if (!parms[define->numparms - 1])
					{
						SourceWarning(source, "too few define parms");
					}
					done = 1;
					break;
				}
			}
			else if (token.type == TT_NAME)
			{
				newdefine = PC_FindHashedDefine(source->definehash, token.string);
				if (newdefine)
				{
					if (!PC_ExpandDefineIntoSource(source, &token, newdefine))
					{
						return qfalse;
					}
					continue;
				}
			}

			if (numparms < define->numparms)
			{
				t       = PC_CopyToken(&token);
				t->next = NULL;
				if (last)
				{
					last->next = t;
				}
				else
				{
					parms[numparms] = t;
				}
				last = t;
			}
		}
		numparms++;
	}
	return qtrue;
}

/**
 * @brief PC_StringizeTokens
 * @param[in] tokens
 * @param[in,out] token
 * @return
 */
int PC_StringizeTokens(token_t *tokens, token_t *token)
{
	token_t *t;

	token->type            = TT_STRING;
	token->whitespace_p    = NULL;
	token->endwhitespace_p = NULL;
	token->string[0]       = '\0';
	strcat(token->string, "\"");
	for (t = tokens; t; t = t->next)
	{
		strncat(token->string, t->string, MAX_TOKEN - strlen(token->string));
	}
	strncat(token->string, "\"", MAX_TOKEN - strlen(token->string));
	return qtrue;
}

/**
 * @brief PC_MergeTokens
 * @param[in,out] t1
 * @param[in] t2
 * @return
 */
int PC_MergeTokens(token_t *t1, token_t *t2)
{
	// merging of a name with a name or number
	if (t1->type == TT_NAME && (t2->type == TT_NAME || t2->type == TT_NUMBER))
	{
		strcat(t1->string, t2->string);
		return qtrue;
	}
	// merging of two strings
	if (t1->type == TT_STRING && t2->type == TT_STRING)
	{
		// remove trailing double quote
		t1->string[strlen(t1->string) - 1] = '\0';
		// concat without leading double quote
		Q_strcat(t1->string, sizeof(t1->string), &t2->string[1]);
		return qtrue;
	}
	// FIXME: merging of two number of the same sub type
	return qfalse;
}

/*
 * @brief PC_PrintDefine
 * @param[in] define
 *
 * @note Unused
void PC_PrintDefine(define_t *define)
{
    printf("define->name = %s\n", define->name);
    printf("define->flags = %d\n", define->flags);
    printf("define->builtin = %d\n", define->builtin);
    printf("define->numparms = %d\n", define->numparms);
    //token_t *parms;                 //define parameters
    //token_t *tokens;                    //macro tokens (possibly containing parm tokens)
    //struct define_s *next;          //next defined macro in a list
}
*/
#if DEFINEHASHING

/*
 * @brief PC_PrintDefineHashTable
 * @param definehash
 *
 * @note unused
void PC_PrintDefineHashTable(define_t **definehash)
{
    int      i;
    define_t *d;

    for (i = 0; i < DEFINEHASHSIZE; i++)
    {
        Com_Printf("%4d:", i);
        for (d = definehash[i]; d; d = d->hashnext)
        {
            Com_Printf(" %s", d->name);
        }
        Com_Printf("\n");
    }
}
*/

//char primes[16] = {1, 3, 5, 7, 11, 13, 17, 19, 23, 27, 29, 31, 37, 41, 43, 47};

/**
 * @brief PC_NameHash
 * @param[in] name
 * @return
 */
int PC_NameHash(const char *name)
{
	int hash, i;

	hash = 0;
	for (i = 0; name[i] != '\0'; i++)
	{
		hash += name[i] * (119 + i);
		//hash += (name[i] << 7) + i;
		//hash += (name[i] << (i&15));
	} //end while
	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (DEFINEHASHSIZE - 1);
	return hash;
}

/**
 * @brief PC_AddDefineToHash
 * @param[in,out] define
 * @param[in,out] definehash
 */
void PC_AddDefineToHash(define_t *define, define_t **definehash)
{
	int hash;

	hash             = PC_NameHash(define->name);
	define->hashnext = definehash[hash];
	definehash[hash] = define;
}

/**
 * @brief PC_FindHashedDefine
 * @param[in] definehash
 * @param[in] name
 * @return
 */
define_t *PC_FindHashedDefine(define_t **definehash, const char *name)
{
	define_t *d;
	int      hash;

	hash = PC_NameHash(name);
	for (d = definehash[hash]; d; d = d->hashnext)
	{
		if (!strcmp(d->name, name))
		{
			return d;
		}
	}
	return NULL;
}
#endif //DEFINEHASHING

/**
 * @brief PC_FindDefine
 * @param[in] defines
 * @param[in] name
 * @return
 */
define_t *PC_FindDefine(define_t *defines, const char *name)
{
	define_t *d;

	for (d = defines; d; d = d->next)
	{
		if (!strcmp(d->name, name))
		{
			return d;
		}
	}
	return NULL;
}

/**
 * @brief PC_FindDefineParm
 * @param define
 * @param name
 * @return The number of the parm otherwise if no parm found with the given name -1 is returned
 */
int PC_FindDefineParm(define_t *define, char *name)
{
	token_t *p;
	int     i;

	i = 0;
	for (p = define->parms; p; p = p->next)
	{
		if (!strcmp(p->string, name))
		{
			return i;
		}
		i++;
	}
	return -1;
}

/**
 * @brief PC_FreeDefine
 * @param[in,out] define
 */
void PC_FreeDefine(define_t *define)
{
	token_t *t, *next;

	// free the define parameters
	for (t = define->parms; t; t = next)
	{
		next = t->next;
		PC_FreeToken(t);
	}
	// free the define tokens
	for (t = define->tokens; t; t = next)
	{
		next = t->next;
		PC_FreeToken(t);
	}
	// free the define
	FreeMemory(define);
}

/*
 * @brief Add builtin defines
 * @param[in] source
 *
 * @note Unused. Keep it here for the time being.
void PC_AddBuiltinDefines(source_t *source)
{
    int      i;
    define_t *define;
    struct builtin
    {
        char *string;
        int builtin;
    } builtin[] =
    {
        { "__LINE__", BUILTIN_LINE },
        { "__FILE__", BUILTIN_FILE },
        { "__DATE__", BUILTIN_DATE },
        { "__TIME__", BUILTIN_TIME },
        //{   "__STDC__", BUILTIN_STDC },
        { NULL,       0            }
    };

    for (i = 0; builtin[i].string; i++)
    {
        define = (define_t *) GetMemory(sizeof(define_t) + strlen(builtin[i].string) + 1);
        Com_Memset(define, 0, sizeof(define_t));
        define->name = (char *) define + sizeof(define_t);
        strcpy(define->name, builtin[i].string);
        define->flags  |= DEFINE_FIXED;
        define->builtin = builtin[i].builtin;
        // add the define to the source
#if DEFINEHASHING
        PC_AddDefineToHash(define, source->definehash);
#else
        define->next    = source->defines;
        source->defines = define;
#endif //DEFINEHASHING
    }
}
*/

/**
 * @brief PC_ExpandBuiltinDefine
 * @param[in] source
 * @param[in] deftoken
 * @param[in] define
 * @param[out] firsttoken
 * @param[out] lasttoken
 * @return
 */
int PC_ExpandBuiltinDefine(source_t *source, token_t *deftoken, define_t *define,
                           token_t **firsttoken, token_t **lasttoken)
{
	token_t *token;
	time_t  t;
	char    *curtime;

	token = PC_CopyToken(deftoken);
	switch (define->builtin)
	{
	case BUILTIN_LINE:
	{
		sprintf(token->string, "%d", deftoken->line);
#ifdef NUMBERVALUE
		token->intvalue   = deftoken->line;
		token->floatvalue = deftoken->line;
#endif //NUMBERVALUE
		token->type    = TT_NUMBER;
		token->subtype = TT_DECIMAL | TT_INTEGER;
		*firsttoken    = token;
		*lasttoken     = token;
		break;
	}
	case BUILTIN_FILE:
	{
		strcpy(token->string, source->scriptstack->filename);
		token->type    = TT_NAME;
		token->subtype = strlen(token->string);
		*firsttoken    = token;
		*lasttoken     = token;
		break;
	}
	case BUILTIN_DATE:
	{
		t       = time(NULL);
		curtime = ctime(&t);
		strcpy(token->string, "\"");
		strncat(token->string, curtime + 4, 7);
		strncat(token->string + 7, curtime + 20, 4);
		strcat(token->string, "\"");
		//Com_Dealloc(curtime);
		token->type    = TT_NAME;
		token->subtype = strlen(token->string);
		*firsttoken    = token;
		*lasttoken     = token;
		break;
	}
	case BUILTIN_TIME:
	{
		t       = time(NULL);
		curtime = ctime(&t);
		strcpy(token->string, "\"");
		strncat(token->string, curtime + 11, 8);
		strcat(token->string, "\"");
		//Com_Dealloc(curtime);
		token->type    = TT_NAME;
		token->subtype = (int)strlen(token->string);
		*firsttoken    = token;
		*lasttoken     = token;
		break;
	}
	case BUILTIN_STDC:
	default:
	{
		*firsttoken = NULL;
		*lasttoken  = NULL;
		break;
	}
	}
	return qtrue;
}

/**
 * @brief PC_ExpandDefine
 * @param[in] source
 * @param[in] deftoken
 * @param[in] define
 * @param[out] firsttoken
 * @param[out] lasttoken
 * @return
 */
int PC_ExpandDefine(source_t *source, token_t *deftoken, define_t *define,
                    token_t **firsttoken, token_t **lasttoken)
{
	token_t *parms[MAX_DEFINEPARMS], *dt, *pt, *t;
	token_t *t1, *t2, *first, *last, *nextpt, token;
	int     parmnum, i;

	// if it is a builtin define
	if (define->builtin)
	{
		return PC_ExpandBuiltinDefine(source, deftoken, define, firsttoken, lasttoken);
	}
	// if the define has parameters
	if (define->numparms)
	{
		if (!PC_ReadDefineParms(source, define, parms, MAX_DEFINEPARMS))
		{
			return qfalse;
		}
#ifdef DEBUG_EVAL
		for (i = 0; i < define->numparms; i++)
		{
			Com_Printf("define parms %d:", i);
			for (pt = parms[i]; pt; pt = pt->next)
			{
				Com_Printf("%s", pt->string);
			}
		}
#endif //DEBUG_EVAL
	}
	// empty list at first
	first = NULL;
	last  = NULL;
	// create a list with tokens of the expanded define
	for (dt = define->tokens; dt; dt = dt->next)
	{
		parmnum = -1;
		// if the token is a name, it could be a define parameter
		if (dt->type == TT_NAME)
		{
			parmnum = PC_FindDefineParm(define, dt->string);
		}
		// if it is a define parameter
		if (parmnum >= 0)
		{
			for (pt = parms[parmnum]; pt; pt = pt->next)
			{
				t = PC_CopyToken(pt);
				// add the token to the list
				t->next = NULL;
				if (last)
				{
					last->next = t;
				}
				else
				{
					first = t;
				}
				last = t;
			}
		}
		else
		{
			// if stringizing operator
			if (dt->string[0] == '#' && dt->string[1] == '\0')
			{
				// the stringizing operator must be followed by a define parameter
				if (dt->next)
				{
					parmnum = PC_FindDefineParm(define, dt->next->string);
				}
				else
				{
					parmnum = -1;
				}

				if (parmnum >= 0)
				{
					// step over the stringizing operator
					dt = dt->next;
					// stringize the define parameter tokens
					if (!PC_StringizeTokens(parms[parmnum], &token))
					{
						SourceError(source, "can't stringize tokens");
						return qfalse;
					}
					t = PC_CopyToken(&token);
				}
				else
				{
					SourceWarning(source, "stringizing operator without define parameter");
					continue;
				}
			}
			else
			{
				t = PC_CopyToken(dt);
			}
			// add the token to the list
			t->next = NULL;
			if (last)
			{
				last->next = t;
			}
			else
			{
				first = t;
			}
			last = t;
		}
	}
	// check for the merging operator
	for (t = first; t; )
	{
		if (t->next)
		{
			// if the merging operator
			if (t->next->string[0] == '#' && t->next->string[1] == '#')
			{
				t1 = t;
				t2 = t->next->next;
				if (t2)
				{
					if (!PC_MergeTokens(t1, t2))
					{
						SourceError(source, "can't merge %s with %s", t1->string, t2->string);
						return qfalse;
					}
					PC_FreeToken(t1->next);
					t1->next = t2->next;
					if (t2 == last)
					{
						last = t1;
					}
					PC_FreeToken(t2);
					continue;
				}
			}
		}
		t = t->next;
	}
	// store the first and last token of the list
	*firsttoken = first;
	*lasttoken  = last;
	// free all the parameter tokens
	for (i = 0; i < define->numparms; i++)
	{
		for (pt = parms[i]; pt; pt = nextpt)
		{
			nextpt = pt->next;
			PC_FreeToken(pt);
		}
	}

	return qtrue;
}

/**
 * @brief PC_ExpandDefineIntoSource
 * @param[in,out] source
 * @param[in] deftoken
 * @param[in] define
 * @return
 */
int PC_ExpandDefineIntoSource(source_t *source, token_t *deftoken, define_t *define)
{
	token_t *firsttoken, *lasttoken;

	if (!PC_ExpandDefine(source, deftoken, define, &firsttoken, &lasttoken))
	{
		return qfalse;
	}

	if (firsttoken && lasttoken)
	{
		lasttoken->next = source->tokens;
		source->tokens  = firsttoken;
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief PC_ConvertPath
 * @param[in] path
 */
void PC_ConvertPath(char *path)
{
	char *ptr;

	// remove double path seperators
	for (ptr = path; *ptr; )
	{
		if ((*ptr == '\\' || *ptr == '/') &&
		    (*(ptr + 1) == '\\' || *(ptr + 1) == '/'))
		{
			memmove(ptr, ptr + 1, strlen(ptr));
		}
		else
		{
			ptr++;
		}
	}
	// set OS dependent path seperators
	for (ptr = path; *ptr; )
	{
		if (*ptr == '/' || *ptr == '\\')
		{
			*ptr = PATH_SEP;
		}
		ptr++;
	}
}

/**
 * @brief PC_Directive_include
 * @param[in] source
 * @return
 */
int PC_Directive_include(source_t *source)
{
	script_t *script;
	token_t  token;
	char     path[_MAX_PATH];

	if (source->skip > 0)
	{
		return qtrue;
	}

	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "#include without file name");
		return qfalse;
	}
	if (token.linescrossed > 0)
	{
		SourceError(source, "#include without file name");
		return qfalse;
	}
	if (token.type == TT_STRING)
	{
		StripDoubleQuotes(token.string);
		PC_ConvertPath(token.string);
		script = LoadScriptFile(token.string);
		if (!script)
		{
			Q_strncpyz(path, source->includepath, sizeof(path));
			Q_strcat(path, sizeof(path), token.string);
			script = LoadScriptFile(path);
		}
	}
	else if (token.type == TT_PUNCTUATION && *token.string == '<')
	{
		Q_strncpyz(path, source->includepath, sizeof(path));
		while (PC_ReadSourceToken(source, &token))
		{
			if (token.linescrossed > 0)
			{
				PC_UnreadSourceToken(source, &token);
				break;
			}
			if (token.type == TT_PUNCTUATION && *token.string == '>')
			{
				break;
			}
			Q_strcat(path, sizeof(path), token.string);
		}
		if (*token.string != '>')
		{
			SourceWarning(source, "#include missing trailing >");
		}
		if (!strlen(path))
		{
			SourceError(source, "#include without file name between < >");
			return qfalse;
		}
		PC_ConvertPath(path);
		script = LoadScriptFile(path);
	}
	else
	{
		SourceError(source, "#include without file name");
		return qfalse;
	}
	if (!script)
	{
		SourceError(source, "file %s not found", path);
		return qfalse;
	}
	PC_PushScript(source, script);
	return qtrue;
}

/**
 * @brief Reads a token from the current line, continues reading on the next
 * Line only if a backslash '\' is encountered.
 *
 * @param[in] source
 * @param[in] token
 * @return
 */
int PC_ReadLine(source_t *source, token_t *token)
{
	int crossline;

	crossline = 0;
	do
	{
		if (!PC_ReadSourceToken(source, token))
		{
			return qfalse;
		}

		if (token->linescrossed > crossline)
		{
			PC_UnreadSourceToken(source, token);
			return qfalse;
		}
		crossline = 1;
	}
	while (!strcmp(token->string, "\\"));
	return qtrue;
}

/**
 * @brief PC_WhiteSpaceBeforeToken
 * @param[in] token
 * @return True if there was a white space in front of the token
 */
int PC_WhiteSpaceBeforeToken(token_t *token)
{
	return token->endwhitespace_p - token->whitespace_p > 0;
}

/**
 * @brief PC_ClearTokenWhiteSpace
 * @param[out] token
 */
void PC_ClearTokenWhiteSpace(token_t *token)
{
	token->whitespace_p    = NULL;
	token->endwhitespace_p = NULL;
	token->linescrossed    = 0;
}

/**
 * @brief PC_Directive_undef
 * @param[in] source
 * @return
 */
int PC_Directive_undef(source_t *source)
{
	token_t  token;
	define_t *define, *lastdefine;
	int      hash;

	if (source->skip > 0)
	{
		return qtrue;
	}

	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "undef without name");
		return qfalse;
	}
	if (token.type != TT_NAME)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "expected name, found %s", token.string);
		return qfalse;
	}
#if DEFINEHASHING

	hash = PC_NameHash(token.string);
	for (lastdefine = NULL, define = source->definehash[hash]; define; define = define->hashnext)
	{
		if (!strcmp(define->name, token.string))
		{
			if (define->flags & DEFINE_FIXED)
			{
				SourceWarning(source, "can't undef %s", token.string);
			}
			else
			{
				if (lastdefine)
				{
					lastdefine->hashnext = define->hashnext;
				}
				else
				{
					source->definehash[hash] = define->hashnext;
				}
				PC_FreeDefine(define);
			}
			break;
		}
		lastdefine = define;
	}
#else //DEFINEHASHING
	for (lastdefine = NULL, define = source->defines; define; define = define->next)
	{
		if (!strcmp(define->name, token.string))
		{
			if (define->flags & DEFINE_FIXED)
			{
				SourceWarning(source, "can't undef %s", token.string);
			}
			else
			{
				if (lastdefine)
				{
					lastdefine->next = define->next;
				}
				else
				{
					source->defines = define->next;
				}
				PC_FreeDefine(define);
			}
			break;
		}
		lastdefine = define;
	}
#endif //DEFINEHASHING
	return qtrue;
}

/**
 * @brief PC_Directive_define
 * @param[in] source
 * @return
 */
int PC_Directive_define(source_t *source)
{
	token_t  token, *t, *last;
	define_t *define;

	if (source->skip > 0)
	{
		return qtrue;
	}

	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "#define without name");
		return qfalse;
	}
	if (token.type != TT_NAME)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "expected name after #define, found %s", token.string);
		return qfalse;
	}
	// check if the define already exists
#if DEFINEHASHING
	define = PC_FindHashedDefine(source->definehash, token.string);
#else
	define = PC_FindDefine(source->defines, token.string);
#endif //DEFINEHASHING
	if (define)
	{
		if (define->flags & DEFINE_FIXED)
		{
			SourceError(source, "can't redefine %s", token.string);
			return qfalse;
		}
		SourceWarning(source, "redefinition of %s", token.string);
		// unread the define name before executing the #undef directive
		PC_UnreadSourceToken(source, &token);
		if (!PC_Directive_undef(source))
		{
			return qfalse;
		}
	}
	// allocate define
	define = (define_t *) GetMemory(sizeof(define_t) + strlen(token.string) + 1);
	Com_Memset(define, 0, sizeof(define_t));
	define->name = (char *) define + sizeof(define_t);
	strcpy(define->name, token.string);
	// add the define to the source
#if DEFINEHASHING
	PC_AddDefineToHash(define, source->definehash);
#else //DEFINEHASHING
	define->next    = source->defines;
	source->defines = define;
#endif //DEFINEHASHING
	   // if nothing is defined, just return
	if (!PC_ReadLine(source, &token))
	{
		return qtrue;
	}
	// if it is a define with parameters
	if (!PC_WhiteSpaceBeforeToken(&token) && !strcmp(token.string, "("))
	{
		// read the define parameters
		last = NULL;
		if (!PC_CheckTokenString(source, ")"))
		{
			while (1)
			{
				if (!PC_ReadLine(source, &token))
				{
					SourceError(source, "expected define parameter");
					return qfalse;
				}
				// if it isn't a name
				if (token.type != TT_NAME)
				{
					SourceError(source, "invalid define parameter");
					return qfalse;
				}

				if (PC_FindDefineParm(define, token.string) >= 0)
				{
					SourceError(source, "two the same define parameters");
					return qfalse;
				}
				// add the define parm
				t = PC_CopyToken(&token);
				PC_ClearTokenWhiteSpace(t);
				t->next = NULL;
				if (last)
				{
					last->next = t;
				}
				else
				{
					define->parms = t;
				}
				last = t;
				define->numparms++;
				// read next token
				if (!PC_ReadLine(source, &token))
				{
					SourceError(source, "define parameters not terminated");
					return qfalse;
				}

				if (!strcmp(token.string, ")"))
				{
					break;
				}
				// then it must be a comma
				if (strcmp(token.string, ","))
				{
					SourceError(source, "define not terminated");
					return qfalse;
				}
			}
		}
		if (!PC_ReadLine(source, &token))
		{
			return qtrue;
		}
	}
	// read the defined stuff
	last = NULL;
	do
	{
		t = PC_CopyToken(&token);
		if (t->type == TT_NAME && !strcmp(t->string, define->name))
		{
			SourceError(source, "recursive define (removed recursion)");
			continue;
		}
		PC_ClearTokenWhiteSpace(t);
		t->next = NULL;
		if (last)
		{
			last->next = t;
		}
		else
		{
			define->tokens = t;
		}
		last = t;
	}
	while (PC_ReadLine(source, &token));
	//
	if (last)
	{
		// check for merge operators at the beginning or end
		if (!strcmp(define->tokens->string, "##") ||
		    !strcmp(last->string, "##"))
		{
			SourceError(source, "define with misplaced ##");
			return qfalse;
		}
	}
	return qtrue;
}

/**
 * @brief PC_DefineFromString
 * @param[in] string
 * @return
 */
define_t *PC_DefineFromString(const char *string)
{
	script_t *script;
	source_t src;
	token_t  *t;
	int      res, i;
	define_t *def;

	script = LoadScriptMemory(string, (int)strlen(string), "*extern");
	// create a new source
	Com_Memset(&src, 0, sizeof(source_t));
	strncpy(src.filename, "*extern", _MAX_PATH);
	src.scriptstack = script;
#if DEFINEHASHING
	src.definehash = GetClearedMemory(DEFINEHASHSIZE * sizeof(define_t *));
#endif //DEFINEHASHING
	   // create a define from the source
	res = PC_Directive_define(&src);
	// free any tokens if left
	for (t = src.tokens; t; t = src.tokens)
	{
		src.tokens = src.tokens->next;
		PC_FreeToken(t);
	}
#ifdef DEFINEHASHING
	def = NULL;
	for (i = 0; i < DEFINEHASHSIZE; i++)
	{
		if (src.definehash[i])
		{
			def = src.definehash[i];
			break;
		}
	}
#else
	def = src.defines;
#endif //DEFINEHASHING

#if DEFINEHASHING
	FreeMemory(src.definehash);
#endif //DEFINEHASHING

	FreeScript(script);
	// if the define was created succesfully
	if (res > 0)
	{
		return def;
	}
	// free the define if created
	if (src.defines)
	{
		PC_FreeDefine(def);
	}

	return NULL;
}

/*
 * @brief Add a define to the source
 * @param[in] source
 * @param[in] string
 * @return
 *
 * @note Unused
int PC_AddDefine(source_t *source, const char *string)
{
    define_t *define;

    define = PC_DefineFromString(string);
    if (!define)
    {
        return qfalse;
    }
#if DEFINEHASHING
    PC_AddDefineToHash(define, source->definehash);
#else //DEFINEHASHING
    define->next    = source->defines;
    source->defines = define;
#endif //DEFINEHASHING
    return qtrue;
}
*/

/**
 * @brief Add a globals define that will be added to all opened sources
 * @param[in] string
 * @return
 */
int PC_AddGlobalDefine(const char *string)
{
	define_t *define;

	define = PC_DefineFromString(string);
	if (!define)
	{
		return qfalse;
	}
	define->next  = globaldefines;
	globaldefines = define;
	return qtrue;
}

/*
 * @brief Remove the given global define
 * @param[in] name
 * @return
 *
 * @note Unused. Keep it here for the time being.
int PC_RemoveGlobalDefine(const char *name)
{
    define_t *define;

    define = PC_FindDefine(globaldefines, name);
    if (define)
    {
        PC_FreeDefine(define);
        return qtrue;
    }
    return qfalse;
}
*/

/**
 * @brief Remove all globals defines
 */
void PC_RemoveAllGlobalDefines(void)
{
	define_t *define;

	for (define = globaldefines; define; define = globaldefines)
	{
		globaldefines = globaldefines->next;
		PC_FreeDefine(define);
	}

	globaldefines = NULL;
}

/**
 * @brief PC_CopyDefine
 * @param source - unused
 * @param[in] define
 * @return
 */
define_t *PC_CopyDefine(source_t *source, define_t *define)
{
	(void)source;
	define_t *newdefine;
	token_t  *token, *newtoken, *lasttoken;
	newdefine = (define_t *) GetMemory(sizeof(define_t) + strlen(define->name) + 1);
	// copy the define name
	newdefine->name = (char *) newdefine + sizeof(define_t);
	strcpy(newdefine->name, define->name);
	newdefine->flags    = define->flags;
	newdefine->builtin  = define->builtin;
	newdefine->numparms = define->numparms;
	// the define is not linked
	newdefine->next     = NULL;
	newdefine->hashnext = NULL;
	// copy the define tokens
	newdefine->tokens = NULL;
	for (lasttoken = NULL, token = define->tokens; token; token = token->next)
	{
		newtoken       = PC_CopyToken(token);
		newtoken->next = NULL;
		if (lasttoken)
		{
			lasttoken->next = newtoken;
		}
		else
		{
			newdefine->tokens = newtoken;
		}
		lasttoken = newtoken;
	}
	// copy the define parameters
	newdefine->parms = NULL;
	for (lasttoken = NULL, token = define->parms; token; token = token->next)
	{
		newtoken       = PC_CopyToken(token);
		newtoken->next = NULL;
		if (lasttoken)
		{
			lasttoken->next = newtoken;
		}
		else
		{
			newdefine->parms = newtoken;
		}
		lasttoken = newtoken;
	}
	return newdefine;
}

/**
 * @brief PC_AddGlobalDefinesToSource
 * @param[in] source
 */
void PC_AddGlobalDefinesToSource(source_t *source)
{
	define_t *define, *newdefine;

	for (define = globaldefines; define; define = define->next)
	{
		newdefine = PC_CopyDefine(source, define);
#if DEFINEHASHING
		PC_AddDefineToHash(newdefine, source->definehash);
#else //DEFINEHASHING
		newdefine->next = source->defines;
		source->defines = newdefine;
#endif //DEFINEHASHING
	}
}

/**
 * @brief PC_Directive_if_def
 * @param[in] source
 * @param[in] type
 * @return
 */
int PC_Directive_if_def(source_t *source, int type)
{
	token_t  token;
	define_t *d;
	int      skip;

	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "#ifdef without name");
		return qfalse;
	}
	if (token.type != TT_NAME)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "expected name after #ifdef, found %s", token.string);
		return qfalse;
	}
#if DEFINEHASHING
	d = PC_FindHashedDefine(source->definehash, token.string);
#else
	d = PC_FindDefine(source->defines, token.string);
#endif //DEFINEHASHING
	skip = (type == INDENT_IFDEF) == (d == NULL);
	PC_PushIndent(source, type, skip);
	return qtrue;
}

/**
 * @brief PC_Directive_ifdef
 * @param[in] source
 * @return
 */
int PC_Directive_ifdef(source_t *source)
{
	return PC_Directive_if_def(source, INDENT_IFDEF);
}

/**
 * @brief PC_Directive_ifndef
 * @param[in] source
 * @return
 */
int PC_Directive_ifndef(source_t *source)
{
	return PC_Directive_if_def(source, INDENT_IFNDEF);
}

/**
 * @brief PC_Directive_else
 * @param[in] source
 * @return
 */
int PC_Directive_else(source_t *source)
{
	int type, skip;

	PC_PopIndent(source, &type, &skip);
	if (!type)
	{
		SourceError(source, "misplaced #else");
		return qfalse;
	}
	if (type == INDENT_ELSE)
	{
		SourceError(source, "#else after #else");
		return qfalse;
	}
	PC_PushIndent(source, INDENT_ELSE, !skip);
	return qtrue;
}

/**
 * @brief PC_Directive_endif
 * @param[in] source
 * @return
 */
int PC_Directive_endif(source_t *source)
{
	int type, skip;

	PC_PopIndent(source, &type, &skip);
	if (!type)
	{
		SourceError(source, "misplaced #endif");
		return qfalse;
	}
	return qtrue;
}

typedef struct operator_s
{
	int operator;
	int priority;
	int parentheses;
	struct operator_s *prev, *next;
} operator_t;

typedef struct value_s
{
	int intvalue;
	float floatvalue;
	int parentheses;
	struct value_s *prev, *next;
} value_t;

/**
 * @brief PC_OperatorPriority
 * @param[in] op
 * @return
 */
int PC_OperatorPriority(int op)
{
	switch (op)
	{
	case P_MUL:
		return 15;
	case P_DIV:
		return 15;
	case P_MOD:
		return 15;
	case P_ADD:
		return 14;
	case P_SUB:
		return 14;

	case P_LOGIC_AND:
		return 7;
	case P_LOGIC_OR:
		return 6;
	case P_LOGIC_GEQ:
		return 12;
	case P_LOGIC_LEQ:
		return 12;
	case P_LOGIC_EQ:
		return 11;
	case P_LOGIC_UNEQ:
		return 11;

	case P_LOGIC_NOT:
		return 16;
	case P_LOGIC_GREATER:
		return 12;
	case P_LOGIC_LESS:
		return 12;

	case P_RSHIFT:
		return 13;
	case P_LSHIFT:
		return 13;

	case P_BIN_AND:
		return 10;
	case P_BIN_OR:
		return 8;
	case P_BIN_XOR:
		return 9;
	case P_BIN_NOT:
		return 16;

	case P_COLON:
		return 5;
	case P_QUESTIONMARK:
		return 5;

	default:
		break;
	}
	return qfalse;
}

//#define AllocValue()          GetClearedMemory(sizeof(value_t));
//#define FreeValue(val)        FreeMemory(val)
//#define AllocOperator(op)     op = (operator_t *) GetClearedMemory(sizeof(operator_t));
//#define FreeOperator(op)      FreeMemory(op);

#define MAX_VALUES      64
#define MAX_OPERATORS   64
#define AllocValue(val)                                 \
	if (numvalues >= MAX_VALUES) {                      \
		SourceError(source, "out of value space\n");      \
		error = 1;                                      \
		break;                                          \
	}                                                   \
	else { \
		val = &value_heap[numvalues++]; }
#define FreeValue(val)

#define AllocOperator(op)                               \
	if (numoperators >= MAX_OPERATORS) {                \
		SourceError(source, "out of operator space\n");   \
		error = 1;                                      \
		break;                                          \
	}                                                   \
	else { \
		op = &operator_heap[numoperators++]; }
#define FreeOperator(op)

/**
 * @brief PC_EvaluateTokens
 * @param[in] source
 * @param[in] tokens
 * @param[out] intvalue
 * @param[out] floatvalue
 * @param[in] integer
 * @return
 */
int PC_EvaluateTokens(source_t *source, token_t *tokens, int *intvalue,
                      float *floatvalue, int integer)
{
	operator_t *o, *firstoperator, *lastoperator;
	value_t    *v, *firstvalue, *lastvalue, *v1, *v2;
	token_t    *t;
	int        brace               = 0;
	int        parentheses         = 0;
	int        error               = 0;
	int        lastwasvalue        = 0;
	int        negativevalue       = 0;
	int        questmarkintvalue   = 0;
	float      questmarkfloatvalue = 0;
	int        gotquestmarkvalue   = qfalse;

	operator_t operator_heap[MAX_OPERATORS];
	int        numoperators = 0;
	value_t    value_heap[MAX_VALUES];
	int        numvalues = 0;


	firstoperator = lastoperator = NULL;
	firstvalue    = lastvalue = NULL;
	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	for (t = tokens; t; t = t->next)
	{
		switch (t->type)
		{
		case TT_NAME:
		{
			if (lastwasvalue || negativevalue)
			{
				SourceError(source, "syntax error in #if/#elif");
				error = 1;
				break;
			}
			if (strcmp(t->string, "defined"))
			{
				SourceError(source, "undefined name %s in #if/#elif", t->string);
				error = 1;
				break;
			}
			t = t->next;
			if (!strcmp(t->string, "("))
			{
				brace = qtrue;
				t     = t->next;
			}
			if (!t || t->type != TT_NAME)
			{
				SourceError(source, "defined without name in #if/#elif");
				error = 1;
				break;
			}
			//v = (value_t *) GetClearedMemory(sizeof(value_t));
			AllocValue(v)
#if DEFINEHASHING
			if (PC_FindHashedDefine(source->definehash, t->string))
#else
			if (PC_FindDefine(source->defines, t->string))
#endif //DEFINEHASHING
			{
				v->intvalue   = 1;
				v->floatvalue = 1;
			}
			else
			{
				v->intvalue   = 0;
				v->floatvalue = 0;
			}
			v->parentheses = parentheses;
			v->next        = NULL;
			v->prev        = lastvalue;
			if (lastvalue)
			{
				lastvalue->next = v;
			}
			else
			{
				firstvalue = v;
			}
			lastvalue = v;
			if (brace)
			{
				t = t->next;
				if (!t || strcmp(t->string, ")"))
				{
					SourceError(source, "defined without ) in #if/#elif");
					error = 1;
					break;
				}
			}
			brace = qfalse;
			// defined() creates a value
			lastwasvalue = 1;
			break;
		}
		case TT_NUMBER:
		{
			if (lastwasvalue)
			{
				SourceError(source, "syntax error in #if/#elif");
				error = 1;
				break;
			}
			//v = (value_t *) GetClearedMemory(sizeof(value_t));
			AllocValue(v)
			if (negativevalue)
			{
				v->intvalue   = -t->intvalue;
				v->floatvalue = -t->floatvalue;
			}
			else
			{
				v->intvalue   = t->intvalue;
				v->floatvalue = t->floatvalue;
			}
			v->parentheses = parentheses;
			v->next        = NULL;
			v->prev        = lastvalue;
			if (lastvalue)
			{
				lastvalue->next = v;
			}
			else
			{
				firstvalue = v;
			}
			lastvalue = v;
			// last token was a value
			lastwasvalue = 1;

			negativevalue = 0;
			break;
		}
		case TT_PUNCTUATION:
		{
			if (negativevalue)
			{
				SourceError(source, "misplaced minus sign in #if/#elif");
				error = 1;
				break;
			}
			if (t->subtype == P_PARENTHESESOPEN)
			{
				parentheses++;
				break;
			}
			else if (t->subtype == P_PARENTHESESCLOSE)
			{
				parentheses--;
				if (parentheses < 0)
				{
					SourceError(source, "too many ) in #if/#elsif");
					error = 1;
				}
				break;
			}
			// check for invalid operators on floating point values
			if (!integer)
			{
				if (t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
				    t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
				    t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
				    t->subtype == P_BIN_XOR)
				{
					SourceError(source, "illigal operator %s on floating point operands\n", t->string);
					error = 1;
					break;
				}
			}
			switch (t->subtype)
			{
			case P_LOGIC_NOT:
			case P_BIN_NOT:
			{
				if (lastwasvalue)
				{
					SourceError(source, "! or ~ after value in #if/#elif");
					error = 1;
					break;
				}
				break;
			}
			case P_INC:
			case P_DEC:
			{
				SourceError(source, "++ or -- used in #if/#elif");
				break;
			}
			case P_SUB:
			{
				if (!lastwasvalue)
				{
					negativevalue = 1;
					break;
				}
			}

			case P_MUL:
			case P_DIV:
			case P_MOD:
			case P_ADD:

			case P_LOGIC_AND:
			case P_LOGIC_OR:
			case P_LOGIC_GEQ:
			case P_LOGIC_LEQ:
			case P_LOGIC_EQ:
			case P_LOGIC_UNEQ:

			case P_LOGIC_GREATER:
			case P_LOGIC_LESS:

			case P_RSHIFT:
			case P_LSHIFT:

			case P_BIN_AND:
			case P_BIN_OR:
			case P_BIN_XOR:

			case P_COLON:
			case P_QUESTIONMARK:
			{
				if (!lastwasvalue)
				{
					SourceError(source, "operator %s after operator in #if/#elif", t->string);
					error = 1;
					break;
				}
				break;
			}
			default:
			{
				SourceError(source, "invalid operator %s in #if/#elif", t->string);
				error = 1;
				break;
			}
			}
			if (!error && !negativevalue)
			{
				//o = (operator_t *) GetClearedMemory(sizeof(operator_t));
				AllocOperator(o)
				o->operator    = t->subtype;
				o->priority    = PC_OperatorPriority(t->subtype);
				o->parentheses = parentheses;
				o->next        = NULL;
				o->prev        = lastoperator;
				if (lastoperator)
				{
					lastoperator->next = o;
				}
				else
				{
					firstoperator = o;
				}
				lastoperator = o;
				lastwasvalue = 0;
			}
			break;
		}
		default:
		{
			SourceError(source, "unknown %s in #if/#elif", t->string);
			error = 1;
			break;
		}
		}
		if (error)
		{
			break;
		}
	}
	if (!error)
	{
		if (!lastwasvalue)
		{
			SourceError(source, "trailing operator in #if/#elif");
			error = 1;
		}
		else if (parentheses)
		{
			SourceError(source, "too many ( in #if/#elif");
			error = 1;
		}
	}

	gotquestmarkvalue   = qfalse;
	questmarkintvalue   = 0;
	questmarkfloatvalue = 0;
	// while there are operators
	while (!error && firstoperator)
	{
		v = firstvalue;
		for (o = firstoperator; o->next; o = o->next)
		{
			// if the current operator is nested deeper in parentheses
			// than the next operator
			if (o->parentheses > o->next->parentheses)
			{
				break;
			}
			// if the current and next operator are nested equally deep in parentheses
			if (o->parentheses == o->next->parentheses)
			{
				// if the priority of the current operator is equal or higher
				// than the priority of the next operator
				if (o->priority >= o->next->priority)
				{
					break;
				}
			}
			// if the arity of the operator isn't equal to 1
			if (o->operator != P_LOGIC_NOT
			    && o->operator != P_BIN_NOT)
			{
				v = v->next;
			}
			// if there's no value or no next value
			if (!v)
			{
				SourceError(source, "mising values in #if/#elif");
				error = 1;
				break;
			}
		}
		if (error)
		{
			break;
		}
		v1 = v;
		v2 = v->next;
#ifdef DEBUG_EVAL
		if (integer)
		{
			Com_Printf("operator %s, value1 = %d", PunctuationFromNum(source->scriptstack, o->operator), v1->intvalue);
			if (v2)
			{
				Com_Printf("value2 = %d", v2->intvalue);
			}
		}
		else
		{
			Com_Printf("operator %s, value1 = %f", PunctuationFromNum(source->scriptstack, o->operator), v1->floatvalue);
			if (v2)
			{
				Com_Printf("value2 = %f", v2->floatvalue);
			}
		}
#endif //DEBUG_EVAL
		switch (o->operator)
		{
		case P_LOGIC_NOT:
			v1->intvalue   = !v1->intvalue;
			v1->floatvalue = !v1->floatvalue;
			break;
		case P_BIN_NOT:
			v1->intvalue = ~v1->intvalue;
			break;
		case P_MUL:
			v1->intvalue   *= v2->intvalue;
			v1->floatvalue *= v2->floatvalue;
			break;
		case P_DIV:
			if (!v2->intvalue || !v2->floatvalue)
			{
				SourceError(source, "divide by zero in #if/#elif\n");
				error = 1;
				break;
			}
			v1->intvalue   /= v2->intvalue;
			v1->floatvalue /= v2->floatvalue;
			break;
		case P_MOD:
			if (!v2->intvalue)
			{
				SourceError(source, "divide by zero in #if/#elif\n");
				error = 1;
				break;
			}
			v1->intvalue %= v2->intvalue;
			break;
		case P_ADD:
			v1->intvalue   += v2->intvalue;
			v1->floatvalue += v2->floatvalue;
			break;
		case P_SUB:
			v1->intvalue   -= v2->intvalue;
			v1->floatvalue -= v2->floatvalue;
			break;
		case P_LOGIC_AND:
			v1->intvalue   = v1->intvalue && v2->intvalue;
			v1->floatvalue = v1->floatvalue && v2->floatvalue;
			break;
		case P_LOGIC_OR:
			v1->intvalue   = v1->intvalue || v2->intvalue;
			v1->floatvalue = v1->floatvalue || v2->floatvalue;
			break;
		case P_LOGIC_GEQ:
			v1->intvalue   = v1->intvalue >= v2->intvalue;
			v1->floatvalue = v1->floatvalue >= v2->floatvalue;
			break;
		case P_LOGIC_LEQ:
			v1->intvalue   = v1->intvalue <= v2->intvalue;
			v1->floatvalue = v1->floatvalue <= v2->floatvalue;
			break;
		case P_LOGIC_EQ:
			v1->intvalue   = v1->intvalue == v2->intvalue;
			v1->floatvalue = fabsf(v1->floatvalue - v2->floatvalue) < 0.0001f;
			break;
		case P_LOGIC_UNEQ:
			v1->intvalue   = v1->intvalue != v2->intvalue;
			v1->floatvalue = fabsf(v1->floatvalue - v2->floatvalue) > 0.0001f;
			break;
		case P_LOGIC_GREATER:
			v1->intvalue   = v1->intvalue > v2->intvalue;
			v1->floatvalue = v1->floatvalue > v2->floatvalue;
			break;
		case P_LOGIC_LESS:
			v1->intvalue   = v1->intvalue < v2->intvalue;
			v1->floatvalue = v1->floatvalue < v2->floatvalue;
			break;
		case P_RSHIFT:
			v1->intvalue >>= v2->intvalue;
			break;
		case P_LSHIFT:
			v1->intvalue <<= v2->intvalue;
			break;
		case P_BIN_AND:
			v1->intvalue &= v2->intvalue;
			break;
		case P_BIN_OR:
			v1->intvalue |= v2->intvalue;
			break;
		case P_BIN_XOR:
			v1->intvalue ^= v2->intvalue;
			break;
		case P_COLON:
		{
			if (!gotquestmarkvalue)
			{
				SourceError(source, ": without ? in #if/#elif");
				error = 1;
				break;
			}
			if (integer)
			{
				if (!questmarkintvalue)
				{
					v1->intvalue = v2->intvalue;
				}
			}
			else
			{
				if (questmarkfloatvalue == 0.0f)
				{
					v1->floatvalue = v2->floatvalue;
				}
			}
			gotquestmarkvalue = qfalse;
			break;
		}
		case P_QUESTIONMARK:
		{
			if (gotquestmarkvalue)
			{
				SourceError(source, "? after ? in #if/#elif");
				error = 1;
				break;
			}
			questmarkintvalue   = v1->intvalue;
			questmarkfloatvalue = v1->floatvalue;
			gotquestmarkvalue   = qtrue;
			break;
		}
		}

#ifdef DEBUG_EVAL
		if (integer)
		{
			Com_Printf("result value = %d", v1->intvalue);
		}
		else
		{
			Com_Printf("result value = %f", v1->floatvalue);
		}
#endif //DEBUG_EVAL
		if (error)
		{
			break;
		}
		// if not an operator with arity 1
		if (o->operator != P_LOGIC_NOT
		    && o->operator != P_BIN_NOT)
		{
			// remove the second value if not question mark operator
			if (o->operator != P_QUESTIONMARK)
			{
				v = v->next;
			}

			if (v->prev)
			{
				v->prev->next = v->next;
			}
			else
			{
				firstvalue = v->next;
			}
			if (v->next)
			{
				v->next->prev = v->prev;
			}
			else
			{
				lastvalue = v->prev;
			}
			//FreeMemory(v);
			FreeValue(v);
		}
		// remove the operator
		if (o->prev)
		{
			o->prev->next = o->next;
		}
		else
		{
			firstoperator = o->next;
		}
		if (o->next)
		{
			o->next->prev = o->prev;
		}
		else
		{
			lastoperator = o->prev;
		}
		//FreeMemory(o);
		FreeOperator(o);
	}
	if (firstvalue)
	{
		if (intvalue)
		{
			*intvalue = firstvalue->intvalue;
		}
		if (floatvalue)
		{
			*floatvalue = firstvalue->floatvalue;
		}
	}
	for (o = firstoperator; o; o = lastoperator)
	{
		lastoperator = o->next;
		//FreeMemory(o);
		FreeOperator(o);
	}
	for (v = firstvalue; v; v = lastvalue)
	{
		lastvalue = v->next;
		//FreeMemory(v);
		FreeValue(v);
	}
	if (!error)
	{
		return qtrue;
	}
	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	return qfalse;
}

/**
 * @brief PC_Evaluate
 * @param[in] source
 * @param[out] intvalue
 * @param[out] floatvalue
 * @param[in] integer
 * @return
 */
int PC_Evaluate(source_t *source, int *intvalue, float *floatvalue, int integer)
{
	token_t  token, *firsttoken, *lasttoken;
	token_t  *t, *nexttoken;
	define_t *define;
	int      defined = qfalse;

	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}

	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "no value after #if/#elif");
		return qfalse;
	}
	firsttoken = NULL;
	lasttoken  = NULL;
	do
	{
		// if the token is a name
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = qfalse;
				t       = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else if (!strcmp(token.string, "defined"))
			{
				defined = qtrue;
				t       = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else
			{
				// then it must be a define
#if DEFINEHASHING
				define = PC_FindHashedDefine(source->definehash, token.string);
#else
				define = PC_FindDefine(source->defines, token.string);
#endif //DEFINEHASHING
				if (!define)
				{
					SourceError(source, "can't evaluate %s, not defined", token.string);
					return qfalse;
				}
				if (!PC_ExpandDefineIntoSource(source, &token, define))
				{
					return qfalse;
				}
			}
		}
		// if the token is a number or a punctuation
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION)
		{
			t       = PC_CopyToken(&token);
			t->next = NULL;
			if (lasttoken)
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}
		else // can't evaluate the token
		{
			SourceError(source, "can't evaluate %s", token.string);
			return qfalse;
		}
	}
	while (PC_ReadLine(source, &token));

	if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer))
	{
		return qfalse;
	}

#ifdef DEBUG_EVAL
	Com_Printf("eval:");
#endif //DEBUG_EVAL
	for (t = firsttoken; t; t = nexttoken)
	{
#ifdef DEBUG_EVAL
		Com_Printf(" %s", t->string);
#endif //DEBUG_EVAL
		nexttoken = t->next;
		PC_FreeToken(t);
	}
#ifdef DEBUG_EVAL
	if (integer)
	{
		Com_Printf("eval result: %d", *intvalue);
	}
	else
	{
		Com_Printf("eval result: %f", *floatvalue);
	}
#endif //DEBUG_EVAL

	return qtrue;
}

/**
 * @brief PC_DollarEvaluate
 * @param[in] source
 * @param[out] intvalue
 * @param[out] floatvalue
 * @param[in] integer
 * @return
 */
int PC_DollarEvaluate(source_t *source, int *intvalue, float *floatvalue, int integer)
{
	int      indent, defined = qfalse;
	token_t  token, *firsttoken, *lasttoken;
	token_t  *t, *nexttoken;
	define_t *define;

	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}

	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "no leading ( after $evalint/$evalfloat");
		return qfalse;
	}
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "nothing to evaluate");
		return qfalse;
	}
	indent     = 1;
	firsttoken = NULL;
	lasttoken  = NULL;
	do
	{
		// if the token is a name
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = qfalse;
				t       = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else if (!strcmp(token.string, "defined"))
			{
				defined = qtrue;
				t       = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else
			{
				// then it must be a define
#if DEFINEHASHING
				define = PC_FindHashedDefine(source->definehash, token.string);
#else
				define = PC_FindDefine(source->defines, token.string);
#endif //DEFINEHASHING
				if (!define)
				{
					SourceError(source, "can't evaluate %s, not defined", token.string);
					return qfalse;
				}
				if (!PC_ExpandDefineIntoSource(source, &token, define))
				{
					return qfalse;
				}
			}
		}
		// if the token is a number or a punctuation
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION)
		{
			if (*token.string == '(')
			{
				indent++;
			}
			else if (*token.string == ')')
			{
				indent--;
			}
			if (indent <= 0)
			{
				break;
			}
			t       = PC_CopyToken(&token);
			t->next = NULL;
			if (lasttoken)
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}
		else // can't evaluate the token
		{
			SourceError(source, "can't evaluate %s", token.string);
			return qfalse;
		}
	}
	while (PC_ReadSourceToken(source, &token));

	if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer))
	{
		return qfalse;
	}

#ifdef DEBUG_EVAL
	Com_Printf("$eval:");
#endif //DEBUG_EVAL
	for (t = firsttoken; t; t = nexttoken)
	{
#ifdef DEBUG_EVAL
		Com_Printf(" %s", t->string);
#endif //DEBUG_EVAL
		nexttoken = t->next;
		PC_FreeToken(t);
	}
#ifdef DEBUG_EVAL
	if (integer)
	{
		Com_Printf("$eval result: %d", *intvalue);
	}
	else
	{
		Com_Printf("$eval result: %f", *floatvalue);
	}
#endif //DEBUG_EVAL

	return qtrue;
}

/**
 * @brief PC_Directive_elif
 * @param[in] source
 * @return
 */
int PC_Directive_elif(source_t *source)
{
	int value;
	int type, skip;

	PC_PopIndent(source, &type, &skip);
	if (!type || type == INDENT_ELSE)
	{
		SourceError(source, "misplaced #elif");
		return qfalse;
	}
	if (!PC_Evaluate(source, &value, NULL, qtrue))
	{
		return qfalse;
	}
	skip = (value == 0);
	PC_PushIndent(source, INDENT_ELIF, skip);
	return qtrue;
}

/**
 * @brief PC_Directive_if
 * @param[in] source
 * @return
 */
int PC_Directive_if(source_t *source)
{
	int value;
	int skip;

	if (!PC_Evaluate(source, &value, NULL, qtrue))
	{
		return qfalse;
	}
	skip = (value == 0);
	PC_PushIndent(source, INDENT_IF, skip);
	return qtrue;
}

/**
 * @brief PC_Directive_line
 * @param[in] source
 * @return
 */
int PC_Directive_line(source_t *source)
{
	SourceError(source, "#line directive not supported");
	return qfalse;
}

/**
 * @brief PC_Directive_error
 * @param[in] source
 * @return
 */
int PC_Directive_error(source_t *source)
{
	token_t token;

	strcpy(token.string, "");
	(void) PC_ReadSourceToken(source, &token);
	SourceError(source, "#error directive: %s", token.string);
	return qfalse;
}

/**
 * @brief PC_Directive_pragma
 * @param[in] source
 * @return
 */
int PC_Directive_pragma(source_t *source)
{
	token_t token;

	SourceWarning(source, "#pragma directive not supported");
	while (PC_ReadLine(source, &token))
		;
	return qtrue;
}

/**
 * @brief UnreadSignToken
 * @param[in] source
 */
void UnreadSignToken(source_t *source)
{
	token_t token;

	token.line            = source->scriptstack->line;
	token.whitespace_p    = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed    = 0;
	strcpy(token.string, "-");
	token.type    = TT_PUNCTUATION;
	token.subtype = P_SUB;
	PC_UnreadSourceToken(source, &token);
}

/**
 * @brief PC_Directive_eval
 * @param[in] source
 * @return
 */
int PC_Directive_eval(source_t *source)
{
	int     value;
	token_t token;

	if (!PC_Evaluate(source, &value, NULL, qtrue))
	{
		return qfalse;
	}

	token.line            = source->scriptstack->line;
	token.whitespace_p    = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed    = 0;
	sprintf(token.string, "%ld", labs(value));
	token.type    = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return qtrue;
}

/**
 * @brief PC_Directive_evalfloat
 * @param[in] source
 * @return
 */
int PC_Directive_evalfloat(source_t *source)
{
	float   value;
	token_t token;

	if (!PC_Evaluate(source, NULL, &value, qfalse))
	{
		return qfalse;
	}
	token.line            = source->scriptstack->line;
	token.whitespace_p    = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed    = 0;
	sprintf(token.string, "%1.2f", Q_fabs(value));
	token.type    = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return qtrue;
}

directive_t directives[15] =
{
	{ "if",        PC_Directive_if        },
	{ "ifdef",     PC_Directive_ifdef     },
	{ "ifndef",    PC_Directive_ifndef    },
	{ "elif",      PC_Directive_elif      },
	{ "else",      PC_Directive_else      },
	{ "endif",     PC_Directive_endif     },
	{ "include",   PC_Directive_include   },
	{ "define",    PC_Directive_define    },
	{ "undef",     PC_Directive_undef     },
	{ "line",      PC_Directive_line      },
	{ "error",     PC_Directive_error     },
	{ "pragma",    PC_Directive_pragma    },
	{ "eval",      PC_Directive_eval      },
	{ "evalfloat", PC_Directive_evalfloat },
	{ NULL,        NULL                   }
};

/**
 * @brief PC_ReadDirective
 * @param[in] source
 * @return
 */
int PC_ReadDirective(source_t *source)
{
	token_t token;

	// read the directive name
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "found # without name");
		return qfalse;
	}
	// directive name must be on the same line
	if (token.linescrossed > 0)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "found # at end of line");
		return qfalse;
	}
	// if if is a name
	if (token.type == TT_NAME)
	{
		int i;

		// find the precompiler directive
		for (i = 0; directives[i].name; i++)
		{
			if (!strcmp(directives[i].name, token.string))
			{
				return directives[i].func(source);
			}
		}
	}
	SourceError(source, "unknown precompiler directive %s", token.string);
	return qfalse;
}

/**
 * @brief PC_DollarDirective_evalint
 * @param[in] source
 * @return
 */
int PC_DollarDirective_evalint(source_t *source)
{
	int     value;
	token_t token;

	if (!PC_DollarEvaluate(source, &value, NULL, qtrue))
	{
		return qfalse;
	}

	token.line            = source->scriptstack->line;
	token.whitespace_p    = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed    = 0;
	sprintf(token.string, "%ld", labs(value));
	token.type    = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
#ifdef NUMBERVALUE
	token.intvalue   = value;
	token.floatvalue = value;
#endif //NUMBERVALUE
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return qtrue;
}

/**
 * @brief PC_DollarDirective_evalfloat
 * @param[in] source
 * @return
 */
int PC_DollarDirective_evalfloat(source_t *source)
{
	float   value;
	token_t token;

	if (!PC_DollarEvaluate(source, NULL, &value, qfalse))
	{
		return qfalse;
	}
	token.line            = source->scriptstack->line;
	token.whitespace_p    = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed    = 0;
	sprintf(token.string, "%1.2f", Q_fabs(value));
	token.type    = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
#ifdef NUMBERVALUE
	token.intvalue   = (int)value;
	token.floatvalue = value;
#endif //NUMBERVALUE
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return qtrue;
}

directive_t dollardirectives[20] =
{
	{ "evalint",   PC_DollarDirective_evalint   },
	{ "evalfloat", PC_DollarDirective_evalfloat },
	{ NULL,        NULL                         }
};

/**
 * @brief PC_ReadDollarDirective
 * @param[in] source
 * @return
 */
int PC_ReadDollarDirective(source_t *source)
{
	token_t token;

	// read the directive name
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "found $ without name");
		return qfalse;
	}
	// directive name must be on the same line
	if (token.linescrossed > 0)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "found $ at end of line");
		return qfalse;
	}
	// if if is a name
	if (token.type == TT_NAME)
	{
		int i;
		// find the precompiler directive
		for (i = 0; dollardirectives[i].name; i++)
		{
			if (!strcmp(dollardirectives[i].name, token.string))
			{
				return dollardirectives[i].func(source);
			}
		}
	}
	PC_UnreadSourceToken(source, &token);
	SourceError(source, "unknown precompiler directive %s", token.string);
	return qfalse;
}

/**
 * @brief Read a token from the source
 * @param[in,out] source
 * @param[in] token
 * @return
 */
int PC_ReadToken(source_t *source, token_t *token)
{
	define_t *define;

	while (1)
	{
		if (!PC_ReadSourceToken(source, token))
		{
			return qfalse;
		}
		// check for precompiler directives
		if (token->type == TT_PUNCTUATION && *token->string == '#')
		{
			// read the precompiler directive
			if (!PC_ReadDirective(source))
			{
				return qfalse;
			}
			continue;
		}
		if (token->type == TT_PUNCTUATION && *token->string == '$')
		{
			// read the precompiler directive
			if (!PC_ReadDollarDirective(source))
			{
				return qfalse;
			}
			continue;
		}
		// if skipping source because of conditional compilation
		if (source->skip)
		{
			continue;
		}
		// recursively concatenate strings that are behind each other
		if (token->type == TT_STRING)
		{
			token_t newtoken;
			if (PC_ReadToken(source, &newtoken))
			{
				if (newtoken.type == TT_STRING)
				{
					token->string[strlen(token->string) - 1] = '\0';
					if (strlen(token->string) + strlen(newtoken.string + 1) + 1 >= MAX_TOKEN)
					{
						SourceError(source, "string longer than MAX_TOKEN %d\n", MAX_TOKEN);
						return qfalse;
					}
					strcat(token->string, newtoken.string + 1);
				}
				else
				{
					PC_UnreadToken(source, &newtoken);
				}
			}
		}
		// if the token is a name
		if (token->type == TT_NAME)
		{
			// check if the name is a define macro
#if DEFINEHASHING
			define = PC_FindHashedDefine(source->definehash, token->string);
#else
			define = PC_FindDefine(source->defines, token->string);
#endif //DEFINEHASHING
			// if it is a define macro
			if (define)
			{
				// expand the defined macro
				if (!PC_ExpandDefineIntoSource(source, token, define))
				{
					return qfalse;
				}
				continue;
			}
		}
		// copy token for unreading
		Com_Memcpy(&source->token, token, sizeof(token_t));
		// found a token
		return qtrue;
	}
}

/*
 * @brief Expect a certain token
 * @param[in] source
 * @param[in] string
 * @return
 *
 * @note Unused. Keep it here for the time being.
int PC_ExpectTokenString(source_t *source, char *string)
{
    token_t token;

    if (!PC_ReadToken(source, &token))
    {
        SourceError(source, "couldn't find expected %s", string);
        return qfalse;
    }

    if (strcmp(token.string, string))
    {
        SourceError(source, "expected %s, found %s", string, token.string);
        return qfalse;
    }
    return qtrue;
}
*/

/*
 * @brief Expect a certain token type
 * @param[in] source
 * @param[in] type
 * @param[in] subtype
 * @param[in] token
 * @return
 *
 * @note Unused. Keep it here for the time being.
int PC_ExpectTokenType(source_t *source, int type, int subtype, token_t *token)
{
    if (!PC_ReadToken(source, token))
    {
        SourceError(source, "couldn't read expected token");
        return qfalse;
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
        SourceError(source, "expected a %s, found %s", str, token->string);
        return qfalse;
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
                strcat(str, "long");
            }
            if (subtype & TT_UNSIGNED)
            {
                strcat(str, "unsigned");
            }
            if (subtype & TT_FLOAT)
            {
                strcat(str, "float");
            }
            if (subtype & TT_INTEGER)
            {
                strcat(str, "integer");
            }
            SourceError(source, "expected %s, found %s", str, token->string);
            return qfalse;
        }
    }
    else if (token->type == TT_PUNCTUATION)
    {
        if (token->subtype != subtype)
        {
            SourceError(source, "found %s", token->string);
            return qfalse;
        }
    }
    return qtrue;
}
*/

/*
 * @brief Expect a token
 * @param[in] source
 * @param[in] token
 * @return
 *
 * @note Unused. Keep it here for the time being.
int PC_ExpectAnyToken(source_t *source, token_t *token)
{
    if (!PC_ReadToken(source, token))
    {
        SourceError(source, "couldn't read expected token");
        return qfalse;
    }
    else
    {
        return qtrue;
    }
}
*/

/**
 * @brief Ckeck if token is available
 * @param[in] source
 * @param[in] string
 * @return true when the token is available
 */
int PC_CheckTokenString(source_t *source, const char *string)
{
	token_t tok;

	if (!PC_ReadToken(source, &tok))
	{
		return qfalse;
	}
	// if the token is available
	if (!strcmp(tok.string, string))
	{
		return qtrue;
	}

	PC_UnreadSourceToken(source, &tok);
	return qfalse;
}

/*
 * @brief PC_CheckTokenType
 * @param[in] source
 * @param[in] type
 * @param[in] subtype
 * @param[out] token
 * @return Return true and reads the token when a token with the given type is available
 *
 * @note Unused. Keep it here for the time being.
int PC_CheckTokenType(source_t *source, int type, int subtype, token_t *token)
{
    token_t tok;

    if (!PC_ReadToken(source, &tok))
    {
        return qfalse;
    }
    // if the type matches
    if (tok.type == type &&
        (tok.subtype & subtype) == subtype)
    {
        Com_Memcpy(token, &tok, sizeof(token_t));
        return qtrue;
    }

    PC_UnreadSourceToken(source, &tok);
    return qfalse;
}
*/

/*
 * @brief Skip tokens until the given token string is read
 * @param[in] source
 * @param[in] string
 * @return
 *
 * @note Unused. Keep it here for the time being.
int PC_SkipUntilString(source_t *source, const char *string)
{
    token_t token;

    while (PC_ReadToken(source, &token))
    {
        if (!strcmp(token.string, string))
        {
            return qtrue;
        }
    }
    return qfalse;
}
*/

/*
 * @brief Inread the last token read from the script
 * @param[in] source
 *
 * @note Unused
void PC_UnreadLastToken(source_t *source)
{
    PC_UnreadSourceToken(source, &source->token);
}
*/

/**
 * @brief Unread the given token
 * @param[in] source
 * @param[in] token
 */
void PC_UnreadToken(source_t *source, token_t *token)
{
	PC_UnreadSourceToken(source, token);
}



/*
 * @brief Set the source include path
 * @param[in,out] source
 * @param[in] path
 *
 * @note Unused. Keep it here for the time being.
void PC_SetIncludePath(source_t *source, const char *path)
{
    size_t len;

    Q_strncpyz(source->includepath, path, _MAX_PATH - 1);

    len = strlen(source->includepath);
    // add trailing path seperator
    if (len > 0 && source->includepath[len - 1] != '\\' &&
        source->includepath[len - 1] != '/')
    {
        strcat(source->includepath, va("%c", PATH_SEP));
    }
}
*/


/*
 * @brief Set the punction set
 * @param[out] source
 * @param[in] p
 *
 * @note Unused. Keep it here for the time being.
void PC_SetPunctuations(source_t *source, punctuation_t *p)
{
    source->punctuations = p;
}
*/

/**
 * @brief Load a source file
 * @param[in] filename
 * @return
 */
source_t *LoadSourceFile(const char *filename)
{
	source_t *source;
	script_t *script;

	script = LoadScriptFile(filename);
	if (!script)
	{
		//botimport.Print(PRT_WARNING, "LoadSourceFile: File %s not found!\n", filename);
		return NULL;
	}

	script->next = NULL;

	source = (source_t *) GetMemory(sizeof(source_t));
	Com_Memset(source, 0, sizeof(source_t));

	Q_strncpyz(source->filename, filename, MAX_QPATH);

	source->scriptstack = script;
	source->tokens      = NULL;
	source->defines     = NULL;
	source->indentstack = NULL;
	source->skip        = 0;

#if DEFINEHASHING
	source->definehash = GetClearedMemory(DEFINEHASHSIZE * sizeof(define_t *));
#endif //DEFINEHASHING
	PC_AddGlobalDefinesToSource(source);
	return source;
}

/*
 * @brief Load a source from memory
 * @param[in] ptr
 * @param[in] length
 * @param[in] name
 * @return
 *
 * @note Unused. Keep it here for the time being.
source_t *LoadSourceMemory(char *ptr, int length, const char *name)
{
    source_t *source;
    script_t *script;

    script = LoadScriptMemory(ptr, length, name);
    if (!script)
    {
        return NULL;
    }
    script->next = NULL;

    source = (source_t *) GetMemory(sizeof(source_t));
    Com_Memset(source, 0, sizeof(source_t));

    Q_strncpyz(source->filename, name, _MAX_PATH);

    source->scriptstack = script;
    source->tokens      = NULL;
    source->defines     = NULL;
    source->indentstack = NULL;
    source->skip        = 0;

#if DEFINEHASHING
    source->definehash = GetClearedMemory(DEFINEHASHSIZE * sizeof(define_t *));
#endif //DEFINEHASHING
    PC_AddGlobalDefinesToSource(source);
    return source;
}
*/

/**
 * @brief Free the given source
 * @param[in,out] source
 */
void FreeSource(source_t *source)
{
	script_t *script;
	token_t  *token;
	define_t *define;
	indent_t *indent;
	int      i;

	//PC_PrintDefineHashTable(source->definehash);
	// free all the scripts
	while (source->scriptstack)
	{
		script              = source->scriptstack;
		source->scriptstack = source->scriptstack->next;
		FreeScript(script);
	}
	// free all the tokens
	while (source->tokens)
	{
		token          = source->tokens;
		source->tokens = source->tokens->next;
		PC_FreeToken(token);
	}
#if DEFINEHASHING
	for (i = 0; i < DEFINEHASHSIZE; i++)
	{
		while (source->definehash[i])
		{
			define                = source->definehash[i];
			source->definehash[i] = source->definehash[i]->hashnext;
			PC_FreeDefine(define);
		}
	}
#else //DEFINEHASHING
	  //free all defines
	while (source->defines)
	{
		define          = source->defines;
		source->defines = source->defines->next;
		PC_FreeDefine(define);
	}
#endif //DEFINEHASHING
	   // free all indents
	while (source->indentstack)
	{
		indent              = source->indentstack;
		source->indentstack = source->indentstack->next;
		FreeMemory(indent);
	}
#if DEFINEHASHING

	if (source->definehash)
	{
		FreeMemory(source->definehash);
	}
#endif //DEFINEHASHING
	   // free the source itself
	FreeMemory(source);
}

#define MAX_SOURCEFILES     64

source_t *sourceFiles[MAX_SOURCEFILES];

/**
 * @brief PC_LoadSourceHandle
 * @param[in] filename
 * @return
 */
int PC_LoadSourceHandle(const char *filename)
{
	source_t *source;
	int      i;

	for (i = 1; i < MAX_SOURCEFILES; i++)
	{
		if (!sourceFiles[i])
		{
			break;
		}
	} //end for
	if (i >= MAX_SOURCEFILES)
	{
		return 0;
	}
	PS_SetBaseFolder("");
	source = LoadSourceFile(filename);
	if (!source)
	{
		return 0;
	}
	sourceFiles[i] = source;
	return i;
}

/**
 * @brief PC_FreeSourceHandle
 * @param[in] handle
 * @return
 */
int PC_FreeSourceHandle(int handle)
{
	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return qfalse;
	}
	if (!sourceFiles[handle])
	{
		return qfalse;
	}

	FreeSource(sourceFiles[handle]);
	sourceFiles[handle] = NULL;
	return qtrue;
}

/**
 * @brief PC_ReadTokenHandle
 * @param[in] handle
 * @param[in] pc_token
 * @return
 */
int PC_ReadTokenHandle(int handle, pc_token_t *pc_token)
{
	token_t token;
	int     ret;

	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return 0;
	}
	if (!sourceFiles[handle])
	{
		return 0;
	}

	ret = PC_ReadToken(sourceFiles[handle], &token);
	strcpy(pc_token->string, token.string);
	pc_token->type         = token.type;
	pc_token->subtype      = token.subtype;
	pc_token->intvalue     = token.intvalue;
	pc_token->floatvalue   = token.floatvalue;
	pc_token->line         = token.line;
	pc_token->linescrossed = token.linescrossed;
	if (pc_token->type == TT_STRING)
	{
		StripDoubleQuotes(pc_token->string);
	}
	return ret;
}

/**
 * @brief PC_UnreadLastTokenHandle
 * @param[in] handle
 */
void PC_UnreadLastTokenHandle(int handle)
{
	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return;
	}
	if (!sourceFiles[handle])
	{
		return;
	}

	PC_UnreadSourceToken(sourceFiles[handle], &sourceFiles[handle]->token);
}

/**
 * @brief PC_SourceFileAndLine
 * @param[in] handle
 * @param[in] filename
 * @param[in] line
 * @return
 */
int PC_SourceFileAndLine(int handle, char *filename, int *line)
{
	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return qfalse;
	}
	if (!sourceFiles[handle])
	{
		return qfalse;
	}

	strcpy(filename, sourceFiles[handle]->filename);
	if (sourceFiles[handle]->scriptstack)
	{
		*line = sourceFiles[handle]->scriptstack->line;
	}
	else
	{
		*line = 0;
	}
	return qtrue;
}

/*
 * @brief Set the base folder to load files from
 * @param[in] path
 *
 * @note Unused. Keep it here for the time being.
void PC_SetBaseFolder(const char *path)
{
    PS_SetBaseFolder(path);
}
*/

/**
 * @brief PC_CheckOpenSourceHandles
 */
void PC_CheckOpenSourceHandles(void)
{
	int i;

	for (i = 1; i < MAX_SOURCEFILES; i++)
	{
		if (sourceFiles[i])
		{
			botimport.Print(PRT_ERROR, "file %s still open in precompiler\n", sourceFiles[i]->scriptstack->filename);
		}
	}
}
