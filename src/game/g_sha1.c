/**
 * @file g_sha1.c
 * @brief Used for Lua and map configs
 * @copyright This code is taken from ETPub. All credits go to their team especially quad and pheno!
 * http://etpub.org
 */
#include "g_local.h"

#include "../qcommon/crypto/sha-1/sha1.h"

/**
 * @brief G_SHA1
 * @param[in] string
 * @return
 */
char *G_SHA1(const char *string)
{
	SHA1Context sha;

	SHA1Reset(&sha);
	SHA1Input(&sha, (const unsigned char *)string, strlen(string));

	if (!SHA1Result(&sha))
	{
		G_Error("sha1: could not compute message digest");
		return "";
	}
	else
	{
		return va("%08X%08X%08X%08X%08X",
		          sha.Message_Digest[0],
		          sha.Message_Digest[1],
		          sha.Message_Digest[2],
		          sha.Message_Digest[3],
		          sha.Message_Digest[4]);
	}
}
