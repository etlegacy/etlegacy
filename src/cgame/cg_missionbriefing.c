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
 * @file cg_missionbriefing.c
 */

#include "cg_local.h"

/**
 * @brief CG_DescriptionForCampaign
 * @return
 */
const char *CG_DescriptionForCampaign(void)
{
	return cgs.campaignInfoLoaded ? cgs.campaignData.campaignDescription : NULL;
}

/**
 * @brief CG_NameForCampaign
 * @return
 */
const char *CG_NameForCampaign(void)
{
	return cgs.campaignInfoLoaded ? cgs.campaignData.campaignName : NULL;
}

/**
 * @brief CG_FindCampaignInFile
 * @param[in] filename
 * @param[in] campaignShortName
 * @param[in] info
 * @return
 */
qboolean CG_FindCampaignInFile(const char *filename, const char *campaignShortName, cg_campaignInfo_t *info)
{
	int        handle;
	pc_token_t token;
	qboolean   campaignFound = qfalse;

	info->mapCount = 0;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token))
	{
		trap_PC_FreeSource(handle);
		return qfalse;
	}

	if (*token.string != '{')
	{
		trap_PC_FreeSource(handle);
		return qfalse;
	}

	while (trap_PC_ReadToken(handle, &token))
	{
		if (*token.string == '}')
		{
			if (campaignFound)
			{
				trap_PC_FreeSource(handle);
				return qtrue;
			}

			if (!trap_PC_ReadToken(handle, &token))
			{
				// eof
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			if (*token.string != '{')
			{
				trap_Print(va(S_COLOR_RED "unexpected token '%s' inside: %s\n", token.string, filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			info->mapCount = 0;
		}
		else if (!Q_stricmp(token.string, "shortname"))
		{
			if (!trap_PC_ReadToken(handle, &token))          // don't do a stringparse due to memory constraints
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			if (!Q_stricmp(token.string, campaignShortName))
			{
				campaignFound = qtrue;
			}
		}
		else if (!Q_stricmp(token.string, "next") ||
		         !Q_stricmp(token.string, "image"))
		{
			if (!trap_PC_ReadToken(handle, &token))          // don't do a stringparse due to memory constraints
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "description"))
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			Q_strncpyz(info->campaignDescription, token.string, sizeof(info->campaignDescription));
		}
		else if (!Q_stricmp(token.string, "name"))
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			Q_strncpyz(info->campaignName, token.string, sizeof(info->campaignName));
		}
		else if (!Q_stricmp(token.string, "maps"))
		{
			char *ptr, mapname[128], *mapnameptr;

			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			ptr = token.string;
			while (*ptr)
			{
				mapnameptr = mapname;
				while (*ptr && *ptr != ';')
				{
					*mapnameptr++ = *ptr++;
				}
				if (*ptr)
				{
					ptr++;
				}
				*mapnameptr = '\0';

				if (info->mapCount >= MAX_MAPS_PER_CAMPAIGN)
				{
					trap_Print(va(S_COLOR_RED "too many maps for a campaign inside: %s\n", filename));
					trap_PC_FreeSource(handle);
					break;
				}

				Q_strncpyz(info->mapnames[info->mapCount++], mapname, MAX_QPATH);
			}
		}
		else if (!Q_stricmp(token.string, "maptc"))
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			info->mapTC[0][0] = token.floatvalue;

			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			info->mapTC[0][1] = token.floatvalue;

			info->mapTC[1][0] = 650 + info->mapTC[0][0];
			info->mapTC[1][1] = 650 + info->mapTC[0][1];
		}
	}

	trap_PC_FreeSource(handle);
	return qfalse;
}

/**
 * @brief CG_FindArenaInfo
 * @param[in] filename
 * @param[in] mapname
 * @param[in] info
 * @return
 */
qboolean CG_FindArenaInfo(const char *filename, const char *mapname, arenaInfo_t *info)
{
	int        handle;
	pc_token_t token;
	const char *dummy;
	qboolean   found = qfalse;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token))
	{
		trap_PC_FreeSource(handle);
		return qfalse;
	}

	if (*token.string != '{')
	{
		trap_PC_FreeSource(handle);
		return qfalse;
	}

	while (trap_PC_ReadToken(handle, &token))
	{
		if (*token.string == '}')
		{
			if (found)
			{
				trap_PC_FreeSource(handle);
				return qtrue;
			}
			found = qfalse;

			if (!trap_PC_ReadToken(handle, &token))
			{
				// eof
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			if (*token.string != '{')
			{
				trap_Print(va(S_COLOR_RED "unexpected token '%s' inside: %s\n", token.string, filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "objectives") || !Q_stricmp(token.string, "description") || !Q_stricmp(token.string, "type"))
		{
			if (!PC_String_Parse(handle, &dummy))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "longname"))
		{
			if (!PC_String_Parse(handle, &dummy))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
			else
			{
				Q_strncpyz(info->longname, dummy, 128);
			}
		}
		else if (!Q_stricmp(token.string, "map"))
		{
			if (!PC_String_Parse(handle, &dummy))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
			else
			{
				if (!Q_stricmp(dummy, mapname))
				{
					found = qtrue;
				}
			}
		}
		else if (!Q_stricmp(token.string, "Timelimit") || !Q_stricmp(token.string, "AxisRespawnTime") || !Q_stricmp(token.string, "AlliedRespawnTime"))
		{
			if (!PC_Int_Parse(handle, (int *)&dummy))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "lmsbriefing"))
		{
			if (!PC_String_Parse(handle, &dummy))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
			else
			{
				Q_strncpyz(info->lmsdescription, dummy, sizeof(info->lmsdescription));
			}
		}
		else if (!Q_stricmp(token.string, "briefing"))
		{
			if (!PC_String_Parse(handle, &dummy))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
			else
			{
				Q_strncpyz(info->description, dummy, sizeof(info->description));
			}
		}
		else if (!Q_stricmp(token.string, "alliedwintext"))
		{
			if (!PC_String_Parse(handle, &dummy))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
			else
			{
				Q_strncpyz(info->alliedwintext, dummy, sizeof(info->description));
			}
		}
		else if (!Q_stricmp(token.string, "axiswintext"))
		{
			if (!PC_String_Parse(handle, &dummy))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
			else
			{
				Q_strncpyz(info->axiswintext, dummy, sizeof(info->description));
			}
		}
		else if (!Q_stricmp(token.string, "mapposition_x"))
		{
			vec_t x;

			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			x = token.floatvalue;

			info->mappos[0] = x;
		}
		else if (!Q_stricmp(token.string, "mapposition_y"))
		{
			vec_t y;

			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			y = token.floatvalue;

			info->mappos[1] = y;
		}
	}

	trap_PC_FreeSource(handle);
	return qfalse;
}

/**
 * @brief CG_LocateCampaign
 */
void CG_LocateCampaign(void)
{
	int          numdirs;
	char         filename[MAX_QPATH];
	char         dirlist[1024];
	char         *dirptr;
	int          i;
	unsigned int dirlen;
	qboolean     found = qfalse;

	// get all campaigns from .campaign files
	numdirs = trap_FS_GetFileList("scripts", ".campaign", dirlist, 1024);
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		dirlen = strlen(dirptr);
		Q_strncpyz(filename, "scripts/", MAX_QPATH);
		Q_strcat(filename, MAX_QPATH, dirptr);
		if (CG_FindCampaignInFile(filename, cgs.currentCampaign, &cgs.campaignData))
		{
			found = qtrue;
			break;
		}
	}

	if (!found)
	{
		return;
	}

	for (i = 0; i < cgs.campaignData.mapCount; i++)
	{
		Com_sprintf(filename, sizeof(filename), "scripts/%s.arena", cgs.campaignData.mapnames[i]);

		if (!CG_FindArenaInfo(filename, cgs.campaignData.mapnames[i], &cgs.campaignData.arenas[i]))
		{
			return;
		}
	}

	cgs.campaignInfoLoaded = qtrue;
}

/**
 * @brief CG_LocateArena
 */
void CG_LocateArena(void)
{
	char filename[MAX_QPATH];

	Com_sprintf(filename, sizeof(filename), "scripts/%s.arena", cgs.rawmapname);

	if (!CG_FindArenaInfo(filename, cgs.rawmapname, &cgs.arenaData))
	{
		return;
	}

	cgs.arenaInfoLoaded = qtrue;
}
