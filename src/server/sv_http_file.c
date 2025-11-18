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
 * @file sv_http_file.c
 * @brief HTTP file operations - validation, location, and serving
 */

#include "server.h"
#include "sv_http.h"

/**
 * @brief Validate requested path for security
 * @param[in] path Requested path from HTTP request
 * @return qtrue if path is valid and safe, qfalse otherwise
 *
 * @note This validation mirrors the security checks used in SV_WriteDownloadToClient
 *       to ensure HTTP file serving has the same security as the regular download system
 */
qboolean HTTP_ValidatePath(const char *path)
{
	const char *cleanPath;
	const char *ext;

	if (!path || !path[0])
	{
		Com_DPrintf("HTTP: Empty path rejected\n");
		return qfalse;
	}

	// Skip leading slash if present
	cleanPath = path;
	if (cleanPath[0] == '/')
	{
		cleanPath++;
	}

	// Empty after removing slash
	if (!cleanPath[0])
	{
		Com_DPrintf("HTTP: Empty path after normalization rejected\n");
		return qfalse;
	}

	// Use existing filesystem security check for path traversal
	if (FS_CheckDirTraversal(cleanPath))
	{
		Com_DPrintf("HTTP: Path traversal attempt rejected: %s\n", cleanPath);
		return qfalse;
	}

	// Only allow .pk3 files
	ext = strrchr(cleanPath, '.');
	if (!ext || Q_stricmp(ext, ".pk3") != 0)
	{
		Com_DPrintf("HTTP: Non-pk3 file rejected: %s\n", cleanPath);
		return qfalse;
	}

	// Check if downloads are allowed (same as SV_CheckDownloadAllowed)
	if (!sv_allowDownload->integer)
	{
		Com_DPrintf("HTTP: Downloads disabled on server: %s\n", cleanPath);
		return qfalse;
	}

	// Check if this is an official id pak (same check as SV_CheckDownloadAllowed)
	// Note: FS_idPak expects path WITHOUT .pk3 extension
	{
		char pathWithoutExt[MAX_QPATH];
		Q_strncpyz(pathWithoutExt, cleanPath, sizeof(pathWithoutExt));
		COM_StripExtension(pathWithoutExt, pathWithoutExt, sizeof(pathWithoutExt));

		if (FS_idPak(pathWithoutExt, BASEGAME))
		{
			Com_DPrintf("HTTP: Cannot download official id pak: %s\n", cleanPath);
			return qfalse;
		}
	}

	// CRITICAL: Verify the pak is in the loaded pak list (CVE-2006-2082 protection)
	// This ensures we only serve files that are already registered in the filesystem
	if (!FS_VerifyPak(cleanPath))
	{
		Com_DPrintf("HTTP: Pak not in loaded pak list (CVE-2006-2082): %s\n", cleanPath);
		return qfalse;
	}

	// Path is valid and safe
	Com_DPrintf("HTTP: Path validated: %s\n", cleanPath);
	return qtrue;
}

/**
 * @brief Open file for serving over HTTP using secure filesystem
 * @param[in] path Game-relative path to file (e.g., "etmain/pak0.pk3")
 * @param[out] fileHandle Pointer to store file handle
 * @param[out] fileSize Pointer to store file size
 * @return qtrue if file opened successfully, qfalse otherwise
 *
 * @note This should only be called after HTTP_ValidatePath has approved the path
 */
qboolean HTTP_OpenFileForServing(const char *path, fileHandle_t *fileHandle, int *fileSize)
{
	long       size;
	const char *cleanPath;

	if (!path || !path[0] || !fileHandle || !fileSize)
	{
		return qfalse;
	}

	// Skip leading slash if present
	cleanPath = path;
	if (cleanPath[0] == '/')
	{
		cleanPath++;
	}

	// Use secure server filesystem function that:
	// - Searches homepath and basepath automatically
	// - Returns file size
	// - Opens the file handle
	// - Uses proper security checks
	size = FS_SV_FOpenFileRead(cleanPath, fileHandle);

	if (size <= 0 || !*fileHandle)
	{
		Com_DPrintf("HTTP: Failed to open file: %s\n", cleanPath);
		return qfalse;
	}

	*fileSize = (int)size;
	Com_DPrintf("HTTP: File opened: %s (handle: %d, size: %d bytes)\n", cleanPath, *fileHandle, *fileSize);
	return qtrue;
}

/**
 * @brief Close open file
 * @param[in] fileHandle File handle to close
 */
void HTTP_CloseFile(fileHandle_t fileHandle)
{
	if (fileHandle)
	{
		FS_FCloseFile(fileHandle);
		Com_DPrintf("HTTP: File closed (handle: %d)\n", fileHandle);
	}
}

/**
 * @brief Read chunk of data from file
 * @param[in] fileHandle Open file handle
 * @param[out] buffer Buffer to read data into
 * @param[in] bufferSize Size of buffer
 * @return Number of bytes read, or -1 on error
 */
int HTTP_ReadFileChunk(fileHandle_t fileHandle, void *buffer, int bufferSize)
{
	int bytesRead;

	if (!fileHandle || !buffer || bufferSize <= 0)
	{
		return -1;
	}

	bytesRead = FS_Read(buffer, bufferSize, fileHandle);
	if (bytesRead < 0)
	{
		Com_DPrintf("HTTP: File read error\n");
		return -1;
	}

	Com_DPrintf("HTTP: Read %d bytes from file\n", bytesRead);
	return bytesRead;
}

/**
 * @brief Seek to position in file
 * @param[in] fileHandle Open file handle
 * @param[in] offset Offset to seek to
 * @param[in] whence Seek mode (FS_SEEK_SET, FS_SEEK_CUR, FS_SEEK_END)
 * @return qtrue on success, qfalse on error
 */
qboolean HTTP_SeekFile(fileHandle_t fileHandle, int offset, int whence)
{
	int result;

	if (!fileHandle)
	{
		return qfalse;
	}

	result = FS_Seek(fileHandle, offset, whence);
	if (result != 0)
	{
		Com_DPrintf("HTTP: File seek failed\n");
		return qfalse;
	}

	Com_DPrintf("HTTP: Seeked to offset %d\n", offset);
	return qtrue;
}
