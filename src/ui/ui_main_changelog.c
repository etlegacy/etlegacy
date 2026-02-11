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
 * @file ui_main_changelog.c
 */

#include "ui_local.h"

// Changelog browser limits kept static to avoid runtime allocations in the UI VM.
#define MAX_CHANGELOG_FILES 256
#define MAX_CHANGELOG_LINES 2048
#define MAX_CHANGELOG_TEXT  131072

typedef struct
{
	char name[MAX_QPATH];
} changelogFile_t;

static changelogFile_t uiChangelogFiles[MAX_CHANGELOG_FILES];
static int             uiChangelogFileCount;
static int             uiChangelogCurrentFile;
static char            uiChangelogText[MAX_CHANGELOG_TEXT];
static const char      *uiChangelogLines[MAX_CHANGELOG_LINES];
static int             uiChangelogLineCount;

/**
 * @brief Check if a filename ends with the expected changelog extension.
 */
static qboolean UI_IsChangelogFileName(const char *fileName)
{
	const char *dot;

	if (!fileName || !fileName[0])
	{
		return qfalse;
	}

	dot = strrchr(fileName, '.');
	if (!dot)
	{
		return qfalse;
	}

	return (Q_stricmp(dot, ".changelog") == 0) ? qtrue : qfalse;
}

/**
 * @brief Sort changelog filenames in reverse alphabetical order.
 */
static int QDECL UI_SortChangelogFiles(const void *a, const void *b)
{
	const changelogFile_t *ca = (const changelogFile_t *)a;
	const changelogFile_t *cb = (const changelogFile_t *)b;

	// Reverse order so higher alphabetical entries are shown first.
	return Q_stricmp(cb->name, ca->name);
}

/**
 * @brief Keep NEXT/PREVIOUS enable state in sync with current changelog index.
 */
static void UI_UpdateChangelogNavigationState(void)
{
	trap_Cvar_SetValue("ui_changelog_has_prev", (uiChangelogCurrentFile > 0) ? 1 : 0);
	trap_Cvar_SetValue("ui_changelog_has_next", (uiChangelogCurrentFile + 1 < uiChangelogFileCount) ? 1 : 0);
}

/**
 * @brief Split loaded changelog text into fixed line pointers for a list feeder.
 */
static void UI_ParseChangelogLines(void)
{
	char *lineStart;
	int  i;

	uiChangelogLineCount = 0;

	if (!uiChangelogText[0])
	{
		uiChangelogLines[uiChangelogLineCount++] = "";
		return;
	}

	lineStart                                = uiChangelogText;
	uiChangelogLines[uiChangelogLineCount++] = lineStart;

	for (i = 0; uiChangelogText[i] != '\0' && uiChangelogLineCount < MAX_CHANGELOG_LINES; i++)
	{
		if (uiChangelogText[i] == '\r')
		{
			uiChangelogText[i] = '\0';
			if (uiChangelogText[i + 1] == '\n')
			{
				i++;
			}

			if (uiChangelogText[i + 1] != '\0')
			{
				lineStart                                = &uiChangelogText[i + 1];
				uiChangelogLines[uiChangelogLineCount++] = lineStart;
			}
		}
		else if (uiChangelogText[i] == '\n')
		{
			uiChangelogText[i] = '\0';
			if (uiChangelogText[i + 1] != '\0')
			{
				lineStart                                = &uiChangelogText[i + 1];
				uiChangelogLines[uiChangelogLineCount++] = lineStart;
			}
		}
	}
}

/**
 * @brief Load and parse the currently selected changelog file.
 */
static void UI_LoadCurrentChangelog(void)
{
	fileHandle_t f;
	int          len;
	int          i;
	menuDef_t    *menu;
	const char   *fileName;
	char         displayName[MAX_QPATH];
	char         title[MAX_QPATH + 32];

	if (uiChangelogFileCount <= 0)
	{
		trap_Cvar_Set("ui_changelog_title", "CHANGELOG - No changelog files");
		Q_strncpyz(uiChangelogText, "No files found in etmain:/changelogs/", sizeof(uiChangelogText));
		uiChangelogLines[0]    = uiChangelogText;
		uiChangelogLineCount   = 1;
		uiChangelogCurrentFile = 0;
		UI_UpdateChangelogNavigationState();
		menu = Menus_FindByName("changelog_popup");
		Menu_SetFeederSelection(menu, FEEDER_CHANGELOG, 0, NULL);
		return;
	}

	if (uiChangelogCurrentFile < 0)
	{
		uiChangelogCurrentFile = 0;
	}
	else if (uiChangelogCurrentFile >= uiChangelogFileCount)
	{
		uiChangelogCurrentFile = uiChangelogFileCount - 1;
	}

	fileName = uiChangelogFiles[uiChangelogCurrentFile].name;
	Q_strncpyz(displayName, fileName, sizeof(displayName));
	for (i = 0; displayName[i] != '\0'; i++)
	{
		if (displayName[i] == '_')
		{
			displayName[i] = ' ';
		}
	}
	Com_sprintf(title, sizeof(title), "CHANGELOG - %s", displayName);
	trap_Cvar_Set("ui_changelog_title", title);

	len = trap_FS_FOpenFile(va("changelogs/%s", fileName), &f, FS_READ);
	if (len >= 0 && f)
	{
		if (len >= (int)sizeof(uiChangelogText))
		{
			// Keep one byte for the string terminator.
			len = (int)sizeof(uiChangelogText) - 1;
		}

		trap_FS_Read(uiChangelogText, len, f);
		uiChangelogText[len] = '\0';
		trap_FS_FCloseFile(f);
	}
	else
	{
		if (f)
		{
			trap_FS_FCloseFile(f);
		}

		Q_strncpyz(uiChangelogText, "Unable to load selected changelog file.", sizeof(uiChangelogText));
	}

	UI_ParseChangelogLines();
	UI_UpdateChangelogNavigationState();

	menu = Menus_FindByName("changelog_popup");
	Menu_SetFeederSelection(menu, FEEDER_CHANGELOG, 0, NULL);
}

/**
 * @brief Build an alphabetically sorted list of readable changelog files.
 */
void UI_ChangelogInit(void)
{
	char         fileList[8192];
	char         *filePtr;
	size_t       fileLen;
	int          count;
	int          i;
	fileHandle_t f;
	int          len;

	uiChangelogFileCount   = 0;
	uiChangelogCurrentFile = 0;
	uiChangelogText[0]     = '\0';
	uiChangelogLineCount   = 0;

	count   = trap_FS_GetFileList("changelogs", "", fileList, sizeof(fileList));
	filePtr = fileList;

	for (i = 0; i < count && uiChangelogFileCount < MAX_CHANGELOG_FILES; i++)
	{
		fileLen = strlen(filePtr) + 1;

		if (UI_IsChangelogFileName(filePtr))
		{
			// Only keep entries that can be opened as files from the VFS.
			len = trap_FS_FOpenFile(va("changelogs/%s", filePtr), &f, FS_READ);
			if (len >= 0 && f)
			{
				trap_FS_FCloseFile(f);
				Q_strncpyz(uiChangelogFiles[uiChangelogFileCount].name, filePtr, sizeof(uiChangelogFiles[uiChangelogFileCount].name));
				uiChangelogFileCount++;
			}
			else if (f)
			{
				trap_FS_FCloseFile(f);
			}
		}

		filePtr += fileLen;
	}

	if (uiChangelogFileCount > 1)
	{
		qsort(uiChangelogFiles, uiChangelogFileCount, sizeof(uiChangelogFiles[0]), UI_SortChangelogFiles);
	}

	UI_LoadCurrentChangelog();
}

/**
 * @brief Move to the "next" changelog entry in the swapped naming scheme.
 */
void UI_ChangelogNext(void)
{
	if (uiChangelogCurrentFile > 0)
	{
		uiChangelogCurrentFile--;
		UI_LoadCurrentChangelog();
	}
}

/**
 * @brief Move to the "previous" changelog entry in the swapped naming scheme.
 */
void UI_ChangelogPrevious(void)
{
	if (uiChangelogCurrentFile + 1 < uiChangelogFileCount)
	{
		uiChangelogCurrentFile++;
		UI_LoadCurrentChangelog();
	}
}

/**
 * @brief Provide list feeder count for changelog lines.
 */
int UI_ChangelogFeederCount(void)
{
	return uiChangelogLineCount;
}

/**
 * @brief Provide list feeder text for changelog content.
 */
const char *UI_ChangelogFeederItemText(int index, int column)
{
	if (index >= 0 && index < uiChangelogLineCount)
	{
		if (column == 0)
		{
			return uiChangelogLines[index];
		}
		return "";
	}

	return NULL;
}

/**
 * @brief Changelog feeder is read-only and ignores selection changes.
 */
void UI_ChangelogFeederSelection(int index)
{
	(void)index;
}
