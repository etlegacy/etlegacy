/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
 * @file ui_gameinfo.c
 */

#include "ui_local.h"

static void UI_LoadArenasFromFile(char *filename)
{
	int        handle;
	pc_token_t token;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}

	if (!trap_PC_ReadToken(handle, &token))
	{
		trap_PC_FreeSource(handle);
		return;
	}

	if (*token.string != '{')
	{
		trap_PC_FreeSource(handle);
		return;
	}

	uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
	uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
	uiInfo.mapList[uiInfo.mapCount].typeBits  = 0;

	while (trap_PC_ReadToken(handle, &token))
	{
		if (*token.string == '}')
		{

			if (!uiInfo.mapList[uiInfo.mapCount].typeBits)
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF);
			}

			uiInfo.mapCount++;
			if (uiInfo.mapCount >= MAX_MAPS)
			{
				break;
			}

			if (!trap_PC_ReadToken(handle, &token))
			{
				// eof
				trap_PC_FreeSource(handle);
				return;
			}

			if (*token.string != '{')
			{
				trap_Print(va(S_COLOR_RED "unexpected token '%s' inside: %s\n", token.string, filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "map"))
		{
			if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mapLoadName))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "longname"))
		{
			if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mapName))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "briefing"))
		{
			if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].briefing))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "lmsbriefing"))
		{
			if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].lmsbriefing))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		/*
		else if (!Q_stricmp(token.string, "objectives"))
		{
		    if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].objectives))
		    {
		        trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
		        trap_PC_FreeSource(handle);
		        return;
		    }
		}
		*/
		else if (!Q_stricmp(token.string, "timelimit"))
		{
			if (!PC_Int_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].Timelimit))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "axisrespawntime"))
		{
			if (!PC_Int_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].AxisRespawnTime))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "alliedrespawntime"))
		{
			if (!PC_Int_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].AlliedRespawnTime))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "type"))
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
			else
			{
				if (strstr(token.string, "wolfsp"))
				{
					uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_SINGLE_PLAYER);
				}
				if (strstr(token.string, "wolflms"))
				{
					uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF_LMS);
				}
				if (strstr(token.string, "wolfmp"))
				{
					uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF);
				}
				if (strstr(token.string, "wolfsw"))
				{
					uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF_STOPWATCH);
				}
			}
		}
		else if (!Q_stricmp(token.string, "mapposition_x"))
		{
			if (!PC_Float_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mappos[0]))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "mapposition_y"))
		{
			if (!PC_Float_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mappos[1]))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
	}

	trap_PC_FreeSource(handle);
	return;
}

/**
 * @brief Sorting the map list
 */
int QDECL UI_SortArenas(const void *a, const void *b)
{
	mapInfo ca = *(mapInfo *)a;
	mapInfo cb = *(mapInfo *)b;
	char    cleanNameA[MAX_STRING_CHARS];
	char    cleanNameB[MAX_STRING_CHARS];

	Q_strncpyz(cleanNameA, ca.mapName, sizeof(cleanNameA));
	Q_strncpyz(cleanNameB, cb.mapName, sizeof(cleanNameB));
	Q_CleanStr(cleanNameA);
	Q_CleanStr(cleanNameB);

	return strcmp(cleanNameA, cleanNameB);
}

void UI_LoadArenas(void)
{
	int  numdirs;
	char filename[128];
	char dirlist[1024];
	char *dirptr;
	int  i;
	int  dirlen;

	uiInfo.mapCount = 0;

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 1024);
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadArenasFromFile(filename);
	}

	// sorting the maplist
	qsort(uiInfo.mapList, uiInfo.mapCount, sizeof(uiInfo.mapList[0]), UI_SortArenas);
}

mapInfo *UI_FindMapInfoByMapname(const char *name)
{
	int i;

	if (uiInfo.mapCount == 0)
	{
		UI_LoadArenas();
	}

	for (i = 0; i < uiInfo.mapCount; i++)
	{
		if (!Q_stricmp(uiInfo.mapList[i].mapLoadName, name))
		{
			return &uiInfo.mapList[i];
		}
	}

	return NULL;
}

static void UI_LoadCampaignsFromFile(const char *filename)
{
	int        handle, i;
	pc_token_t token;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}

	if (!trap_PC_ReadToken(handle, &token))
	{
		trap_PC_FreeSource(handle);
		return;
	}
	if (*token.string != '{')
	{
		trap_PC_FreeSource(handle);
		return;
	}

	while (trap_PC_ReadToken(handle, &token))
	{
		if (*token.string == '}')
		{
			if (uiInfo.campaignList[uiInfo.campaignCount].initial)
			{
				if (uiInfo.campaignList[uiInfo.campaignCount].typeBits & (1 << GT_SINGLE_PLAYER))
				{
					uiInfo.campaignList[uiInfo.campaignCount].unlocked = qtrue;
				}
				// Always unlock the initial SP campaign
			}

			uiInfo.campaignList[uiInfo.campaignCount].campaignCinematic = -1;
			uiInfo.campaignList[uiInfo.campaignCount].campaignShot      = -1;

			uiInfo.campaignCount++;

			if (!trap_PC_ReadToken(handle, &token))
			{
				// eof
				trap_PC_FreeSource(handle);
				return;
			}

			if (*token.string != '{')
			{
				//uiInfo.campaignList[uiInfo.campaignCount].order = -1;

				trap_Print(va(S_COLOR_RED "unexpected token '%s' inside: %s\n", token.string, filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "shortname"))
		{
			if (!PC_String_Parse(handle, &uiInfo.campaignList[uiInfo.campaignCount].campaignShortName))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "name"))
		{
			if (!PC_String_Parse(handle, &uiInfo.campaignList[uiInfo.campaignCount].campaignName))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "description"))
		{
			if (!PC_String_Parse(handle, &uiInfo.campaignList[uiInfo.campaignCount].campaignDescription))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "image"))
		{
			if (!PC_String_Parse(handle, &uiInfo.campaignList[uiInfo.campaignCount].campaignShotName))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
			else
			{
				uiInfo.campaignList[uiInfo.campaignCount].campaignShot = -1;
			}
		}
		else if (!Q_stricmp(token.string, "initial"))
		{
			uiInfo.campaignList[uiInfo.campaignCount].initial = qtrue;
		}
		else if (!Q_stricmp(token.string, "next"))
		{
			if (!PC_String_Parse(handle, &uiInfo.campaignList[uiInfo.campaignCount].nextCampaignShortName))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		}
		else if (!Q_stricmp(token.string, "type"))
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}

			if (strstr(token.string, "wolfsp"))
			{
				uiInfo.campaignList[uiInfo.campaignCount].typeBits |= (1 << GT_SINGLE_PLAYER);
			}
			if (strstr(token.string, "wolfmp"))
			{
				uiInfo.campaignList[uiInfo.campaignCount].typeBits |= (1 << GT_WOLF);
			}
			if (strstr(token.string, "wolfsw"))
			{
				uiInfo.campaignList[uiInfo.campaignCount].typeBits |= (1 << GT_WOLF_STOPWATCH);
			}
			if (strstr(token.string, "wolflms"))
			{
				uiInfo.campaignList[uiInfo.campaignCount].typeBits |= (1 << GT_WOLF_LMS);
			}
		}
		else if (!Q_stricmp(token.string, "maps"))
		{
			char *ptr, mapname[128], *mapnameptr;

			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}

			// find the mapInfo's that match our mapnames
			uiInfo.campaignList[uiInfo.campaignCount].mapCount = 0;

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
				for (i = 0; i < uiInfo.mapCount; i++)
				{
					if (!Q_stricmp(uiInfo.mapList[i].mapLoadName, mapname))
					{
						uiInfo.campaignList[uiInfo.campaignCount].mapInfos[uiInfo.campaignList[uiInfo.campaignCount].mapCount++] = &uiInfo.mapList[i];
						break;
					}
				}
			}
		}
		else if (!Q_stricmp(token.string, "maptc"))
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}

			uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][0] = token.floatvalue;

			if (!trap_PC_ReadToken(handle, &token))
			{
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}

			uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][1] = token.floatvalue;

			uiInfo.campaignList[uiInfo.campaignCount].mapTC[1][0] = 650 + uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][0];
			uiInfo.campaignList[uiInfo.campaignCount].mapTC[1][1] = 650 + uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][1];
		}
	}

	trap_PC_FreeSource(handle);
}

const char *UI_DescriptionForCampaign(void)
{
	int  i = 0, j = 0;
	char *mapname;
	char info[MAX_INFO_STRING];

	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	mapname = Info_ValueForKey(info, "mapname");

	for ( ; i < uiInfo.campaignCount; i++)
	{
		for ( ; j < uiInfo.campaignList[i].mapCount; j++)
		{
			if (!Q_stricmp(mapname, uiInfo.campaignList[i].mapInfos[j]->mapName))
			{
				return uiInfo.campaignList[i].campaignDescription;
			}
		}
	}

	return NULL;
}

const char *UI_NameForCampaign(void)
{
	int  i = 0, j = 0;
	char *mapname;
	char info[MAX_INFO_STRING];

	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	mapname = Info_ValueForKey(info, "mapname");

	for ( ; i < uiInfo.campaignCount; i++)
	{
		for ( ; j < uiInfo.campaignList[i].mapCount; j++)
		{
			if (!Q_stricmp(mapname, uiInfo.campaignList[i].mapInfos[j]->mapName))
			{
				return uiInfo.campaignList[i].campaignName;
			}
		}
	}

	return NULL;
}

int UI_FindCampaignInCampaignList(const char *shortName)
{
	int i;

	if (!shortName)
	{
		return(-1);
	}

	for (i = 0; i < uiInfo.campaignCount; i++)
	{
		if (!Q_stricmp(uiInfo.campaignList[i].campaignShortName, shortName))
		{
			return(i);
		}
	}

	return(-1);
}

/**
 * @brief Sorting the campaign list
 */
int QDECL UI_SortCampaigns(const void *a, const void *b)
{
	char cleanNameA[MAX_STRING_CHARS];
	char cleanNameB[MAX_STRING_CHARS];

	campaignInfo_t ca = *(campaignInfo_t *)a;
	campaignInfo_t cb = *(campaignInfo_t *)b;
	Q_strncpyz(cleanNameA, ca.campaignName, sizeof(cleanNameA));
	Q_strncpyz(cleanNameB, cb.campaignName, sizeof(cleanNameB));
	Q_CleanStr(cleanNameA);
	Q_CleanStr(cleanNameB);

	return strcmp(cleanNameA, cleanNameB);
}

void UI_LoadCampaigns(void)
{
	int  numdirs;
	char filename[128];
	char dirlist[1024];
	char *dirptr;
	int  i, j;
	int  dirlen;
	long hash;
	char *ch;

	uiInfo.campaignCount = 0;
	memset(&uiInfo.campaignList, 0, sizeof(uiInfo.campaignList));

	// get all campaigns from .campaign files
	numdirs = trap_FS_GetFileList("scripts", ".campaign", dirlist, 1024);
	dirptr  = dirlist;
	for (i = 0; i < numdirs && uiInfo.campaignCount < MAX_CAMPAIGNS; i++, dirptr += dirlen + 1)
	{
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadCampaignsFromFile(filename);
	}

	if (UI_OutOfMemory())
	{
		trap_Print(S_COLOR_YELLOW "WARNING: not anough memory in pool to load all campaigns\n");
	}

	// Sort the campaigns for single player

	// first, find the initial campaign
	for (i = 0; i < uiInfo.campaignCount; i++)
	{
		if (!(uiInfo.campaignList[i].typeBits & (1 << GT_SINGLE_PLAYER)))
		{
			continue;
		}

		if (uiInfo.campaignList[i].initial)
		{
			uiInfo.campaignList[i].order = 0;
			break;
		}
	}

	// now use the initial nextCampaignShortName to find the next one, etc etc for single player campaigns
	// rain - don't let i go above the maximum number of campaigns
	while (i < MAX_CAMPAIGNS)
	{
		j = UI_FindCampaignInCampaignList(uiInfo.campaignList[i].nextCampaignShortName);

		if (j == -1)
		{
			break;
		}

		uiInfo.campaignList[j].order = uiInfo.campaignList[i].order + 1;
		i                            = j;
	}

	// Load the campaign save
	BG_LoadCampaignSave(va("profiles/%s/campaign.dat", cl_profile.string), &uiInfo.campaignStatus, cl_profile.string);

	for (i = 0; i < uiInfo.campaignCount; i++)
	{
		// generate hash for campaign shortname
		for (hash = 0, ch = (char *)uiInfo.campaignList[i].campaignShortName; *ch != '\0'; ch++)
		{
			hash += (long)(tolower(*ch)) * ((ch - uiInfo.campaignList[i].campaignShortName) + 119);
		}

		// find the entry in the campaignsave
		for (j = 0; j < uiInfo.campaignStatus.header.numCampaigns; j++)
		{
			if (hash == uiInfo.campaignStatus.campaigns[j].shortnameHash)
			{
				uiInfo.campaignList[i].unlocked    = qtrue;
				uiInfo.campaignList[i].progress    = uiInfo.campaignStatus.campaigns[j].progress;
				uiInfo.campaignList[i].cpsCampaign = &uiInfo.campaignStatus.campaigns[j];
			}
		}
	}

	// Sorting the campaign list
	qsort(uiInfo.campaignList, uiInfo.campaignCount, sizeof(uiInfo.campaignList[0]), UI_SortCampaigns);
}
