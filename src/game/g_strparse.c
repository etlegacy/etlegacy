#include "g_strparse.h"
#include "g_local.h"

typedef struct
{
	char *name;
	g_StringToken_t g_index;
} g_strtoken_t;

extern const g_strtoken_t *in_word_set(const char *str, unsigned int len);

g_StringToken_t G_GetTokenForString(char const *str)
{
	// Use our minimal perfect hash generated code to give us the
	// result token in optimal time
	const g_strtoken_t *token = in_word_set(str, strlen(str));
	if (token == NULL)
	{
		return TOK_UNKNOWN;
	}
	return (g_StringToken_t)token->g_index;
}
