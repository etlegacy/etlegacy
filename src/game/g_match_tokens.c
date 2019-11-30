/**
- * @file g_match_tokens.c
- * @brief ANSI-C code produced by gperf version 3.0.1
- *
- * <pre>
- * Command-line: gperf -t7C --language=ANSI-C input_tokens.gperf
- * Computed positions: -k'1-2'
- * </pre>
- */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
    && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
    && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
    && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
    && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
    && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
    && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
    && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
    && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
    && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
    && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
    && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
    && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
    && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
    && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
    && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
    && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
    && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
    && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
    && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
    && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
    && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
    && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "input_tokens.gperf"

#include <string.h>
#include "g_strparse.h"
#line 5 "input_tokens.gperf"
struct g_strtoken_t { char *name; g_StringToken_t index; };

#define TOTAL_KEYWORDS 49
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 20
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 87
/* maximum key range = 87, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash(register const char *str, register unsigned int len)
{
	static const unsigned char asso_values[] =
	{
		88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
		88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
		88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
		88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
		88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
		88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
		88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
		88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
		88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
		88, 88, 88, 88, 88, 5,  88, 0,  0,  5,
		25, 30, 25, 20, 36, 11, 88, 0,  10, 35,
		30, 88, 10, 88, 45, 15, 0,  50, 88, 50,
		40, 88, 88, 88, 88, 88, 88, 88
	};
	register int hval = len;

	switch (hval)
	{
	default:
		hval += asso_values[(unsigned char)str[1]];
	/*FALLTHROUGH*/
	case 1:
		hval += asso_values[(unsigned char)str[0]];
		break;
	}
	return hval;
}

#ifdef __GNUC__
__inline
#ifdef __GNUC_STDC_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
/**
 * @brief in_word_set
 * @param[in] str
 * @param[in] len
 * @return
 */
const struct g_strtoken_t *in_word_set(register const char *str, register unsigned int len)
{
	static const struct g_strtoken_t wordlist[] =
	{
		{ "" },
#line 25 "input_tokens.gperf"
		{ "t", 18},
		{ "" }, { "" }, { "" }, { "" },
#line 26 "input_tokens.gperf"
		{ "c", 19},
#line 44 "input_tokens.gperf"
		{ "bc", 37},
#line 45 "input_tokens.gperf"
		{ "bcs", 38},
		{ "" }, { "" },
#line 43 "input_tokens.gperf"
		{ "l", 36},
#line 46 "input_tokens.gperf"
		{ "bl", 39},
#line 47 "input_tokens.gperf"
		{ "bls", 40},
		{ "" }, { "" },
#line 30 "input_tokens.gperf"
		{ "s", 23},
#line 37 "input_tokens.gperf"
		{ "lc", 30},
#line 15 "input_tokens.gperf"
		{ "password", 8},
		{ "" },
#line 22 "input_tokens.gperf"
		{ "skill", 15},
#line 52 "input_tokens.gperf"
		{ "g", 45},
#line 14 "input_tokens.gperf"
		{ "cl_guid", 7},
#line 12 "input_tokens.gperf"
		{ "ip", 5},
		{ "" }, { "" },
#line 28 "input_tokens.gperf"
		{ "f", 21},
#line 41 "input_tokens.gperf"
		{ "bd", 34},
#line 55 "input_tokens.gperf"
		{ "lsw", 48},
#line 9 "input_tokens.gperf"
		{ "gamestate", 2},
#line 16 "input_tokens.gperf"
		{ "scriptName", 9},
#line 24 "input_tokens.gperf"
		{ "n", 17},
#line 33 "input_tokens.gperf"
		{ "dc", 26},
#line 18 "input_tokens.gperf"
		{ "cg_uinfo", 11},
#line 13 "input_tokens.gperf"
		{ "name", 6},
		{ "" },
#line 29 "input_tokens.gperf"
		{ "m", 22},
#line 53 "input_tokens.gperf"
		{ "gs", 46},
		{ "" },
#line 7 "input_tokens.gperf"
		{ "g_currentRound", 0},
#line 8 "input_tokens.gperf"
		{ "g_nextTimeLimit", 1},
		{ "" },
#line 10 "input_tokens.gperf"
		{ "g_currentCampaign", 3},
#line 21 "input_tokens.gperf"
		{ "ch", 14},
		{ "" },
#line 11 "input_tokens.gperf"
		{ "g_currentCampaignMap", 4},
#line 27 "input_tokens.gperf"
		{ "r", 20},
#line 48 "input_tokens.gperf"
		{ "br", 41},
#line 49 "input_tokens.gperf"
		{ "brs", 42},
#line 23 "input_tokens.gperf"
		{ "rate", 16},
		{ "" },
#line 35 "input_tokens.gperf"
		{ "w", 28},
#line 31 "input_tokens.gperf"
		{ "xp", 24},
#line 51 "input_tokens.gperf"
		{ "hs", 44},
		{ "" },
#line 20 "input_tokens.gperf"
		{ "pmove_msec", 13},
#line 19 "input_tokens.gperf"
		{ "pmove_fixed", 12},
#line 32 "input_tokens.gperf"
		{ "dn", 25},
		{ "" }, { "" }, { "" }, { "" },
#line 36 "input_tokens.gperf"
		{ "lw", 29},
#line 42 "input_tokens.gperf"
		{ "hd", 35},
		{ "" }, { "" }, { "" },
#line 38 "input_tokens.gperf"
		{ "sw", 31},
#line 50 "input_tokens.gperf"
		{ "he", 43},
		{ "" }, { "" }, { "" },
#line 34 "input_tokens.gperf"
		{ "dr", 27},
		{ "" }, { "" }, { "" }, { "" },
#line 40 "input_tokens.gperf"
		{ "rn", 33},
#line 39 "input_tokens.gperf"
		{ "ref", 32},
		{ "" }, { "" }, { "" },
#line 17 "input_tokens.gperf"
		{ "respawn", 10},
		{ "" }, { "" }, { "" }, { "" },
#line 54 "input_tokens.gperf"
		{ "mu", 47}
	};

	if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
	{
		register int key = hash(str, len);

		if (key <= MAX_HASH_VALUE && key >= 0)
		{
			register const char *s = wordlist[key].name;

			if (*str == *s && !strcmp(str + 1, s + 1))
			{
				return &wordlist[key];
			}
		}
	}
	return 0;
}
#line 56 "input_tokens.gperf"
