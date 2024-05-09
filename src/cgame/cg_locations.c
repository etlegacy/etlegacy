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
 * @file cg_locations.c
 * @brief handle map locations also includes the editor for locations
 */

#include "cg_local.h"

#define CG_CurrentClient() cgs.clientinfo[cg.clientNum]
#define CG_CurrentClientLocation() CG_GetLocation(cg.clientNum, CG_CurrentClient().location)
#define CG_ResetCurrentClientLocation() cgs.clientLocation[cg.clientNum].lastLocation = 0

void CG_LocationsEditor(qboolean show)
{
	cg.editingLocations = show;
}

void CG_LocationsSave(const char *path)
{
	char         output[MAX_QPATH];
	int          i;
	location_t   *loc;
	char         *s;
	fileHandle_t file;
	qtime_t      time;

	if (!cg.editingLocations)
	{
		CG_Printf(S_COLOR_RED "Location editing is not enabled.\n");
		return;
	}

	if (path)
	{
		if (strlen(path) >= MAX_QPATH)
		{
			CG_Printf(S_COLOR_RED "ERROR CG_LocationsSave: given path too long '%s'\n", path);
			return;
		}
		Q_strncpyz(output, path, sizeof(output));
	}
	else
	{
		Com_sprintf(output, sizeof(output), "maps/%s_loc_local.dat", cgs.rawmapname);
	}

	CG_Printf("Number of locations to save: %i\n", cgs.numLocations);

	if (trap_FS_FOpenFile(output, &file, FS_WRITE) < 0)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_LocationsSave: failed to save locations to '%s'\n", output);
		return;
	}

	s = va("/////////////////////////////////////////////////////////\n// Map name: %s\n", cgs.rawmapname);
	trap_FS_Write(s, (int) strlen(s), file);

	s = va("// Num. locations: %i\n", cgs.numLocations);
	trap_FS_Write(s, (int) strlen(s), file);

	trap_RealTime(&time);
	s = va("// Created at: %i-%i-%i\n", time.tm_mday, time.tm_mon + 1, 1900 + time.tm_year);
	trap_FS_Write(s, (int) strlen(s), file);

	s = va("// Created by: %s\n/////////////////////////////////////////////////////////\n\n", CG_CurrentClient().cleanname);
	trap_FS_Write(s, (int) strlen(s), file);

	for (i = 0; i < cgs.numLocations; i++)
	{
		loc = &cgs.location[i];

		s = va("%i %i %i ", (int) loc->origin[0], (int) loc->origin[1], (int) loc->origin[2]);
		trap_FS_Write(s, (int) strlen(s), file);

		// if the message is the same, then the output should contain only a @ char, meaning the message is copied over
		if (i > 0 && !strcmp(cgs.location[i - 1].message, loc->message))
		{
			s = "@\n";
		}
		else
		{
			s = va("\"%s\"\n", loc->message);
		}

		trap_FS_Write(s, (int) strlen(s), file);
	}

	trap_FS_FCloseFile(file);

	CG_Printf("Locations saved to: '%s'\n", output);
}

void CG_LocationsAdd(const char *message)
{
	if (!cg.editingLocations)
	{
		CG_Printf(S_COLOR_RED "Location editing is not enabled.\n");
		return;
	}

	if (cgs.numLocations == MAX_C_LOCATIONS)
	{
		CG_Printf("^9Too many locations specified.\n");
		return;
	}

	location_t *loc = &cgs.location[cgs.numLocations];

	loc->index = cgs.numLocations;
	Q_strncpyz(loc->message, message, sizeof(loc->message));

	VectorCopy(cgs.clientinfo[cg.clientNum].location, loc->origin);

	// nudge the location just a bit up from the players location
	loc->origin[2] += 40;

	cgs.numLocations++;

	if (cgs.numLocations == MAX_C_LOCATIONS)
	{
		CG_Printf("^9Too many locations specified.\n");
	}

	CG_ResetCurrentClientLocation();
}

void CG_LocationsRenameCurrent(const char *message)
{
	location_t *loc = NULL;

	if (!cg.editingLocations)
	{
		CG_Printf(S_COLOR_RED "Location editing is not enabled.\n");
		return;
	}

	loc = CG_CurrentClientLocation();

	if (!loc)
	{
		CG_Printf("^9No valid location currently found.\n");
		return;
	}

	Q_strncpyz(loc->message, message, sizeof(loc->message));
}

void CG_LocationsRemoveCurrent(void)
{
	int        move;
	location_t *loc = NULL;

	if (!cg.editingLocations)
	{
		CG_Printf(S_COLOR_RED "Location editing is not enabled.\n");
		return;
	}

	loc = CG_CurrentClientLocation();

	if (!loc)
	{
		CG_Printf("^9No valid location currently found.\n");
		return;
	}

	move = (cgs.numLocations - 1) - loc->index;

	if (move)
	{
		int target = loc->index;
		memmove(&cgs.location[target], &cgs.location[target + 1], move * sizeof(location_t));
		cgs.numLocations--;

		// fix the indexes
		for (; target < cgs.numLocations; target++)
		{
			cgs.location[target].index--;
		}
	}
	else
	{
		// the location was the last one in the list, so just pop a number
		cgs.numLocations--;
	}

	// reset the current client location
	CG_ResetCurrentClientLocation();
}

void CG_LocationsMoveCurrent(void)
{
	location_t *loc = NULL;

	if (!cg.editingLocations)
	{
		CG_Printf(S_COLOR_RED "Location editing is not enabled.\n");
		return;
	}

	loc = CG_CurrentClientLocation();

	if (!loc)
	{
		CG_Printf("^9No valid location currently found.\n");
		return;
	}

	VectorCopy(cgs.clientinfo[cg.clientNum].location, loc->origin);

	// nudge the location just a bit up from the players location
	loc->origin[2] += 40;

	CG_ResetCurrentClientLocation();
}

void CG_LocationsDump(void)
{
	int        i;
	location_t *loc;

	if (!cg.editingLocations)
	{
		CG_Printf(S_COLOR_RED "Location editing is not enabled.\n");
		return;
	}

	if (!cgs.numLocations)
	{
		return;
	}

	CG_Printf("^7Number of locations: %i\n", cgs.numLocations);

	for (i = 0; i < cgs.numLocations; i++)
	{
		loc = &cgs.location[i];
		CG_Printf("^7Location msg: \"%s^7\" in x:%.1f y:%.1f z:%.1f\n", loc->message, loc->origin[0], loc->origin[1],
		          loc->origin[2]);
	}
}

void CG_LocationsReload(void)
{
	if (!cg.editingLocations)
	{
		CG_Printf(S_COLOR_RED "Location editing is not enabled.\n");
		return;
	}

	CG_ResetCurrentClientLocation();

	cgs.locationsLoaded = qfalse;
	cgs.numLocations    = 0;
	memset(cgs.location, 0, sizeof(location_t) * MAX_C_LOCATIONS);

	CG_LoadLocations();
}

/**
 * @brief CG_RenderLocations
 */
void CG_RenderLocations(void)
{
	refEntity_t re;
	location_t  *location;
	location_t  *current;
	int         i;

	if (cgs.numLocations < 1)
	{
		return;
	}

	current = CG_CurrentClientLocation();

	// draw the current location...
	if (current)
	{
		CPri(va("%s\n", current->message));
		// CG_Text_Paint_Ext(10, 10, cg.centerPrintFontScale, cg.centerPrintFontScale, colorWhite, current->message, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}

	for (i = 0; i < cgs.numLocations; ++i)
	{
		location = &cgs.location[i];

		if (VectorDistance(cg.refdef.vieworg, location->origin) > 3000)
		{
			continue;
		}

		if (!trap_R_inPVS(cg.refdef.vieworg, location->origin))
		{
			continue;
		}

		Com_Memset(&re, 0, sizeof(re));
		re.reType = RT_SPRITE;
		VectorCopy(location->origin, re.origin);
		VectorCopy(location->origin, re.oldorigin);
		re.radius       = 8;
		re.customShader = cgs.media.waterBubbleShader;

		if (current == location)
		{
			Vector4Set(re.shaderRGBA, 0, 255, 0, 255);
		}
		else
		{
			Vector4Set(re.shaderRGBA, 255, 0, 0, 255);
		}

		trap_R_AddRefEntityToScene(&re);

		if (location->message[0])
		{
			CG_AddOnScreenText(location->message, location->origin, qfalse);
		}
		else
		{
			CG_AddOnScreenText("No location message", location->origin, qfalse);
		}
	}
}

/**
 * @brief CG_GetLocation
 * @param[in] client
 * @param[in] origin
 * @return
 */
location_t *CG_GetLocation(int client, vec3_t origin)
{
	location_t *curLoc;
	location_t *bestLoc = NULL;
	float      bestdist = 200000000.f;
	float      len;
	int        i;

	if (ISVALIDCLIENTNUM(client) && cgs.clientLocation[client].lastLocation)
	{
		if (cgs.clientLocation[client].lastX == origin[0]
		    && cgs.clientLocation[client].lastY == origin[1]
		    && cgs.clientLocation[client].lastZ == origin[2])
		{
			return &cgs.location[cgs.clientLocation[client].lastLocation];
		}
	}

	for (i = 0; i < cgs.numLocations; ++i)
	{
		curLoc = &cgs.location[i];
		len    = VectorDistance(origin, curLoc->origin);

		if (len > bestdist || !trap_R_inPVS(origin, curLoc->origin))
		{
			//CG_Printf("^6PVS %i \n", trap_R_inPVS(origin, curLoc->origin));
			//CG_Printf("^6OR %i  %i   --  %i   %i\n", origin[0], origin[1], curLoc->origin[0], curLoc->origin[1]);
			continue;
		}

		bestdist = len;
		bestLoc  = curLoc;
	}

	// store new information
	if (ISVALIDCLIENTNUM(client) && bestLoc != NULL)
	{
		cgs.clientLocation[client].lastX        = origin[0];
		cgs.clientLocation[client].lastY        = origin[1];
		cgs.clientLocation[client].lastZ        = origin[2];
		cgs.clientLocation[client].lastLocation = bestLoc->index;
	}

	return bestLoc;
}

/**
 * @brief CG_GetLocationMsg
 * @param[in] clientNum
 * @param[in] origin
 * @return
 */
char *CG_GetLocationMsg(int clientNum, vec3_t origin)
{
	location_t *bestLoc = CG_GetLocation(clientNum, origin);

	// Fast way out, there are no locations for the map
	if (cgs.numLocations < 1)
	{
		return "Unknown";
	}

	if (bestLoc != NULL && strlen(bestLoc->message) > 1)
	{
		return va("%s", bestLoc->message);
	}

	return "Unknown";
}

/**
 * @brief CG_BuildLocationString
 * @param[in] clientNum
 * @param[in] origin
 * @param[in] flag
 * @return
 */
char *CG_BuildLocationString(int clientNum, vec3_t origin, int flag)
{
	char *locStr = NULL;
	int  locMaxLen;

	if (cg_locations.integer & flag)
	{
		qboolean locValid = qtrue;

		if (cg_locations.integer & LOC_SHOWDISTANCE)
		{
			if (clientNum == cg.clientNum)
			{
				locStr = va("^3     ");
				//CG_Printf("same client\n");
			}
			else
			{
				//locStr = va("^3%7.2f", VectorDistance(origin, cgs.clientinfo[cg.clientNum].location) / 40.f); // meter
				locStr = va("^3%7.2f",
				            VectorDistance(origin, cgs.clientinfo[cg.clientNum].location));        // game units
			}
		}
		else
		{
			locStr = va("^3%s", CG_GetLocationMsg(clientNum, origin));
			if (!(cg_locations.integer & LOC_KEEPUNKNOWN))
			{
				if (!Q_stricmp(locStr, "Unknown"))
				{
					locStr   = va("^3(%s)", BG_GetLocationString(origin[0], origin[1]));
					locValid = qfalse; // don't draw it twice..
				}
			}
			// truncate location string if max chars is set
			if (cg_locationMaxChars.integer)
			{
				locMaxLen = Com_Clamp(0, 128, cg_locationMaxChars.integer); // 128 is max location length
				locStr    = Q_TruncateStr(locStr, locMaxLen);
			}
		}

		if ((cg_locations.integer & LOC_SHOWCOORDS) && locValid)
		{
			Q_strcat(locStr, 64, va(" ^3(%s)", BG_GetLocationString(origin[0], origin[1])));   // append a location
		}
	}
	else
	{
		locStr = va("^3%s", BG_GetLocationString(origin[0], origin[1]));
	}

	return locStr;
}

#define MAX_BUFFER 32768

/**
 * @brief Report a location for the player. Uses placed nearby target_location entities
 */
void CG_LoadLocations(void)
{
	fileHandle_t f;                     // handle of file on disk
	int          fLen;         // length of the file
	char         fBuffer[MAX_BUFFER]; // buffer to read the file into
	char         message[128] = "\0"; // location description
	char         temp[128]    = "\0"; // temporary buffer
	int          x            = 0; // x-coord of the location
	int          y            = 0; // y-coord of the location
	int          z            = 0; // z-coord of the location
	int          p            = 0; // current location in the file buffer
	int          t            = 0; // current location in the temp buffer

	// _loc_local.dat files are for user "private" files that are not supposed to be in pk3 files
	fLen = trap_FS_FOpenFile(va("maps/%s_loc_local.dat", cgs.rawmapname), &f, FS_READ);

	if (fLen <= 0)
	{
		// _loc_override.dat files are for overriding the locations and these files can be inside a pk3 file
		fLen = trap_FS_FOpenFile(va("maps/%s_loc_override.dat", cgs.rawmapname), &f, FS_READ);
	}

	if (fLen <= 0)
	{
		// open the location .dat file that matches the map's name this filename is the default with which the map should ship
		fLen = trap_FS_FOpenFile(va("maps/%s_loc.dat", cgs.rawmapname), &f, FS_READ);
	}

	if (fLen <= 0)
	{
		CG_Printf(S_COLOR_BLUE "LoadLocations: ^3Warning: ^9No location data found for map ^2%s^9.\n", cgs.rawmapname);
		return;
	}

	if (fLen >= MAX_BUFFER)
	{
		trap_FS_FCloseFile(f);
		CG_Error("Location file is too big, make it smaller (max = %i bytes)\n", MAX_BUFFER);
	}

	trap_FS_Read(&fBuffer, fLen, f);                    // read the file into the buffer
	fBuffer[fLen] = '\0';                               // make sure it's null-terminated
	trap_FS_FCloseFile(f);                              // close the file, we're done with it

	CG_Printf(S_COLOR_BLUE "LoadLocations: ^9location data for map ^2%s ^9loaded\n", cgs.rawmapname);

	// start parsing!
	while (p < fLen)
	{
		// check for the beginning of a comment
		if (fBuffer[p++] == '/')
		{
			//check for single line comment
			if (fBuffer[p] == '/')
			{
				while (p < fLen && (fBuffer[p] != '\n' && fBuffer[p] != '\r'))
				{
					p++;
				}
			}
			// check for multiline comment
			else if (fBuffer[p] == '*')
			{
				p++;
				while (p < fLen && (fBuffer[p] != '*' && fBuffer[p + 1] != '/'))
				{
					p++;
				}
			}
		}

		// parse the next line
		while (p < fLen && (fBuffer[p] != '\n' && fBuffer[p] != '\r'))
		{
			// grab the x-coord
			while (p < fLen && fBuffer[p] != ' ')
			{
				temp[t++] = fBuffer[p++];
			}
			temp[t] = '\0';
			x       = Q_atoi(temp);
			t       = 0;
			Com_Memset(&temp, 0, sizeof(temp));

			if (p > fLen)
			{
				break;
			}

			p++;

			// grab the y-coord
			while (p < fLen && fBuffer[p] != ' ')
			{
				temp[t++] = fBuffer[p++];
			}
			temp[t] = '\0';
			y       = Q_atoi(temp);
			t       = 0;
			Com_Memset(&temp, 0, sizeof(temp));

			if (p > fLen)
			{
				break;
			}

			p++;

			// grab the z-coord
			while (p < fLen && fBuffer[p] != ' ')
			{
				temp[t++] = fBuffer[p++];
			}
			temp[t] = '\0';
			z       = Q_atoi(temp);
			t       = 0;

			Com_Memset(&temp, 0, sizeof(temp));
			if (p > fLen)
			{
				break;
			}

			p++;

			// grab the description
			while (p < fLen && fBuffer[p] != '\n' && fBuffer[p] != '\r')
			{
				// ignore quotation marks
				if (fBuffer[p] != '\"')
				{
					temp[t++] = fBuffer[p++];
				}
				else
				{
					p++;
				}
			}
			temp[t] = '\0';
			t       = 0;

			// if @, then keep the previous location name, otherwise, update message
			if (Q_stricmp(temp, "@"))
			{
				Q_strncpyz(message, temp, sizeof(message));
			}

			if (p > fLen)
			{
				break;
			}

			if ((x != 0 || y != 0 || z != 0) && strlen(message) > 0)
			{
				location_t *loc = &cgs.location[cgs.numLocations];

				loc->index = cgs.numLocations;
				Q_strncpyz(loc->message, message, sizeof(message));
				loc->origin[0] = x;
				loc->origin[1] = y;
				loc->origin[2] = z;
				cgs.numLocations++;

				if (cgs.numLocations == MAX_C_LOCATIONS)
				{
					CG_Printf("^9Too many locations specified.\n");
					break;
				}
			}
		}
	}
	// ok we are successful
	CG_Printf("^2%i ^9locations loaded.\n", cgs.numLocations);
	cgs.locationsLoaded = qtrue;
}
