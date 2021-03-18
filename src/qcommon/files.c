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
 * @file files.c
 * @brief Handle based filesystem for ET: Legacy
 *
 * @note This implementation is POSIX only see
 * https://www.securecoding.cert.org/confluence/display/c/FIO19-C.+Do+not+use+fseek%28%29+and+ftell%28%29+to+compute+the+size+of+a+regular+file
 */

#include "q_shared.h"
#include "qcommon.h"
#include "crypto/sha-1/sha1.h"

#ifdef BUNDLED_MINIZIP
#    include "unzip.h"
#elif defined(__APPLE__)
#    include <minizip/unzip.h>
#else
#    include <unzip.h>
#endif

/*
=============================================================================
ET: Legacy (QUAKE3) FILESYSTEM

FIXME: This docu needs an update - the following lines aren't fully valid for ET: Legacy

All of ET: Legacy's (Quake's) data access is through a hierarchical file system, but the contents of
the file system can be transparently merged from several sources.

A "qpath" is a reference to game file data.  MAX_ZPATH is 256 characters, which must include
a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
references outside the quake directory system.

The "base path" is the path to the directory holding all the game directories and usually
the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\etlegacy"
command line to allow code debugging in a different directory (make sure it contains an install)
Basepath cannot be modified at all after startup and is usually readonly.

The "home path" is the path used for all write access. On win32 systems we have had "base path"
== "home path", but this no longer the case for ET: Legacy - it requires "base path"
!= "home path" Any files that are created (demos, screenshots, etc) will be created relative
to the base path, so home path should usually be writable.
"home path" points to ~/.etlegacy or c:\<USER>\My Documets\ETLegacy by default.

The user can also install custom mods and content in "home path", so it should be searched
along with "home path" and "base path" for game content. Mods downloads from online server games
are installed into "home path". These downloads install client files only. A full mod server
and client install should be done in "base path". This ensures the install of the game or mods
are never overwritten by any downloads.

The "base game" is the directory under the paths where data comes from by default, and
can be any path if set.

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base game.
This is the basis for addons.

Clients automatically set the game directory after receiving a gamestate from a server,
so only servers need to worry about +set fs_game.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

To save disk space and speed loading, directory trees can be collapsed into zip files.
The files use a ".pk3" extension to prevent users from unzipping them accidentally, but
otherwise the are simply normal uncompressed zip files.  A game directory can have multiple
zip files of the form "pak0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk3 distributed as a patch to override all existing data.

Because we will have updated executables freely available online, there is no point to
trying to restrict demo / oem versions of the game with code changes.  Demo / oem versions
should be exactly the same executables as release versions, but with different data that
automatically restricts where game media can come from to prevent add-ons from working.

File search order: when FS_FOpenFileRead gets called it will go through the fs_searchpaths
structure and stop on the first successful hit. fs_searchpaths is built with successive
calls to FS_AddGameDirectory

Additionaly, we search in several subdirectories:
current game is the current mode
base game is a variable to allow mods based on other mods
(such as etmain + missionpack content combination in a mod for instance)
BASEGAME is the hardcoded base game ("etmain")

e.g. the qpath "sound/newstuff/test.wav" would be searched for in the following places:

home path + current game's zip files
home path + current game's directory
base path + current game's zip files
base path + current game's directory
cd path + current game's zip files
cd path + current game's directory

home path + base game's zip file
home path + base game's directory
base path + base game's zip file
base path + base game's directory
cd path + base game's zip file
cd path + base game's directory

home path + BASEGAME's zip file
home path + BASEGAME's directory
base path + BASEGAME's zip file
base path + BASEGAME's directory
cd path + BASEGAME's zip file
cd path + BASEGAME's directory

server download, to be written to home path + current game's directory


The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

Because the same files are loaded by both the clip model (CM_) and renderer (TR_)
subsystems, a simple single-file caching scheme is used.  The CM_ subsystems will
load the file with a request to cache.  Only one file will be kept cached at a time,
so any models that are going to be referenced by both subsystems should alternate
between the CM_ load function and the ref load function.

TODO: A qpath that starts with a leading slash will always refer to the base game, even if another
game is currently active.  This allows character models, skins, and sounds to be downloaded
to a common directory no matter which game is active.

How to prevent downloading zip files?
Pass pk3 file names in systeminfo, and download before FS_Restart()?

Aborting a download disconnects the client from the server.

How to mark files as downloadable?  Commercial add-ons won't be downloadable.

Non-commercial downloads will want to download the entire zip file.
the game would have to be reset to actually read the zip in

Auto-update information

Path separators

Casing

  separate server gamedir and client gamedir, so if the user starts
  a local game after having connected to a network game, it won't stick
  with the network game.

  allow menu options for game selection?

Read / write config to floppy option.

Different version coexistance?

When building a pak file, make sure a etconfig.cfg isn't present in it,
or configs will never get loaded from disk!

  todo:

  downloading (outside fs?)
  game directory passing and restarting
  explain MAX_OSPATH and MAX_QPATH
=============================================================================
*/

#define MAX_ZPATH           256
#define MAX_SEARCH_PATHS    4096
#define MAX_FILEHASH_SIZE   1024

/**
 * @struct fileInPack_s
 * @brief
 */
typedef struct fileInPack_s
{
	char *name;                         ///< name of the file
	unsigned long pos;                  ///< file info position in zip
	unsigned long len;                  ///< uncompress file size
	struct  fileInPack_s *next;         ///< next file in the hash
} fileInPack_t;

/**
 * @struct pack_s
 * @brief
 */
typedef struct
{
	char pakPathname[MAX_OSPATH];               ///< c:\\etlegacy\\etmain
	char pakFilename[MAX_OSPATH];               ///< c:\\etlegacy\\etmain\\pak0.pk3
	char pakBasename[MAX_OSPATH];               ///< pak0
	char pakGamename[MAX_OSPATH];               ///< etmain
	unzFile handle;                             ///< handle to zip file
	int checksum;                               ///< regular checksum
	int pure_checksum;                          ///< checksum for pure
	int numfiles;                               ///< number of files in pk3
	int referenced;                             ///< referenced file flags
	int hashSize;                               ///< hash table size (power of 2)
	fileInPack_t **hashTable;                   ///< hash table
	fileInPack_t *buildBuffer;                  ///< buffer with the filenames etc.
} pack_t;

/**
 * @struct directory_s
 * @brief
 */
typedef struct
{
	char path[MAX_OSPATH];          ///< c:\\etlegacy
	char fullpath[MAX_OSPATH];      ///< c:\\etlegacy\\etmain
	char gamedir[MAX_OSPATH];       ///< etmain
} directory_t;

/**
 * @struct searchpath_s
 * @brief
 */
typedef struct searchpath_s
{
	struct searchpath_s *next;

	pack_t *pack;           ///< only one of pack \/ dir will be non NULL
	directory_t *dir;
} searchpath_t;

/**
 * @var fs_gamedir
 * @brief This will be a single directory name with no separators
 */
char                fs_gamedir[MAX_OSPATH];
static cvar_t       *fs_debug;
static cvar_t       *fs_homepath;
static cvar_t       *fs_basepath;
static cvar_t       *fs_basegame;
static cvar_t       *fs_gamedirvar;
static searchpath_t *fs_searchpaths;
#if defined(FEATURE_PAKISOLATION) && !defined(DEDICATED)
/**
* @var fs_containerMount
* @brief This enables a containerized directory where all downloadable content is stored,
*        set on default mod server connect once the gamestate is received
*/
static cvar_t *fs_containerMount;
#endif

/**
 * @var fs_readCount
 * @brief Total bytes read
 */
static int fs_readCount;

/**
 * @var fs_loadCount
 * @brief Total files read
 */
static int fs_loadCount;

/**
 * @var fs_loadStack
 * @brief Total files in memory
 */
static int fs_loadStack;

/**
 * @var fs_packFiles
 * @brief Total number of files in packs
 */
static int fs_packFiles = 0;

static int fs_checksumFeed;

/**
 * @union qfile_gus
 * @typedef qfile_gut
 * @brief
 */
typedef union qfile_gus
{
	FILE *o;
	unzFile z;
} qfile_gut;

/**
 * @struct qfile_us
 * @typedef qfile_ut
 * @brief
 */
typedef struct qfile_us
{
	qfile_gut file;
	qboolean unique;
} qfile_ut;

/**
 * @struct fileHandleData_s
 * @brief
 */
typedef struct
{
	qfile_ut handleFiles;
	qboolean handleSync;
	int fileSize;
	int zipFilePos;
	int zipFileLen;
	qboolean zipFile;
	char name[MAX_ZPATH];
} fileHandleData_t;

static fileHandleData_t fsh[MAX_FILE_HANDLES];

/**
 * @var fs_reordered
 * @brief wether we did a reorder on the current search path when joining the server
 */
static qboolean fs_reordered;

// never load anything from pk3 files that are not present at the server when pure
// ex: when fs_numServerPaks != 0, FS_FOpenFileRead won't load anything outside of pk3 except .cfg .menu .game .dat
static int  fs_numServerPaks = 0;
static int  fs_serverPaks[MAX_SEARCH_PATHS];                    ///< checksums
static char *fs_serverPakNames[MAX_SEARCH_PATHS];               ///< pk3 names

// only used for autodownload, to make sure the client has at least
// all the pk3 files that are referenced at the server side
static int  fs_numServerReferencedPaks;
static int  fs_serverReferencedPaks[MAX_SEARCH_PATHS];              ///< checksums
static char *fs_serverReferencedPakNames[MAX_SEARCH_PATHS];         ///< pk3 names

// last valid game folder used
char lastValidBase[MAX_OSPATH];
char lastValidGame[MAX_OSPATH];

#ifdef FS_MISSING
FILE *missingFiles = NULL;
#endif

/* C99 defines __func__ */
#ifndef __func__
#   define __func__ "(unknown)"
#endif

/**
 * @brief FS_Initialized
 * @return
 */
qboolean FS_Initialized(void)
{
	return (fs_searchpaths != NULL);
}

/**
 * @brief FS_PakIsPure
 * @param[in] pack
 * @return
 */
qboolean FS_PakIsPure(pack_t *pack)
{
	if (fs_numServerPaks)
	{
		int i;

		for (i = 0 ; i < fs_numServerPaks ; i++)
		{
			// FIXME: also use hashed file names
			// NOTE TTimo: a pk3 with same checksum but different name would be validated too
			//   I don't see this as allowing for any exploit, it would only happen if the client does manips of it's file names 'not a bug'
			if (pack->checksum == fs_serverPaks[i])
			{
				return qtrue;       // on the approved list
			}
		}
		return qfalse;  // not on the pure server pak list
	}
	return qtrue;
}

/**
 * @brief return load stack
 */
int FS_LoadStack(void)
{
	return fs_loadStack;
}

/**
 * @brief return a hash value for the fileName
 * @param[in] fname
 * @param[in] hashSize
 * @return
 */
static long FS_HashFileName(const char *fname, int hashSize)
{
	int  i    = 0;
	long hash = 0;
	char letter;

	while (fname[i] != '\0')
	{
		letter = tolower(fname[i]);
		if (letter == '.')
		{
			break;                              // don't include extension
		}
		if (letter == '\\')
		{
			letter = '/';                       // damn path names
		}
		if (letter == PATH_SEP)
		{
			letter = '/';                       // damn path names

		}
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash  = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (hashSize - 1);

	return hash;
}

/**
 * @brief FS_HandleForFile
 * @return
 */
static fileHandle_t FS_HandleForFile(void)
{
	int i;

	for (i = 1 ; i < MAX_FILE_HANDLES ; i++)
	{
		if (fsh[i].handleFiles.file.o == NULL)
		{
			return i;
		}
	}
	Com_Error(ERR_DROP, "FS_HandleForFile: none free");

	return 0;
}

/**
 * @brief FS_FileForHandle
 * @param[in] f
 * @return
 */
static FILE *FS_FileForHandle(fileHandle_t f)
{
	if (f < 1 || f >= MAX_FILE_HANDLES)
	{
		Com_Error(ERR_DROP, "FS_FileForHandle: %d out of range", f);
	}
	if (fsh[f].zipFile == qtrue)
	{
		Com_Error(ERR_DROP, "FS_FileForHandle: can't get FILE on zip file");
	}
	if (!fsh[f].handleFiles.file.o)
	{
		Com_Error(ERR_DROP, "FS_FileForHandle: NULL");
	}

	return fsh[f].handleFiles.file.o;
}

/**
 * @brief FS_ForceFlush
 * @param[in] f
 */
void FS_ForceFlush(fileHandle_t f)
{
	FILE *file;

	file = FS_FileForHandle(f);
	setvbuf(file, NULL, _IONBF, 0);
}

/**
 * @brief FS_fplength
 * @param[in] h
 * @return
 */
long FS_fplength(FILE *h)
{
	int         fd, errno;
	struct stat stat_info;

	if ((fd = fileno(h)) == -1)
	{
		Com_Error(ERR_DROP, "FS_fplength: can't get file descriptor. failed: -1\n");
	}

	if ((errno = fstat(fd, &stat_info)) < 0)
	{
		Com_Error(ERR_DROP, "FS_fplength: can't get file stat. failed: errno %d\n", errno);
	}

	return stat_info.st_size;
}

/**
 * @brief If this is called on a non-unique FILE (from a pak file), it will return
 * the size of the pak file, not the expected size of the file.
 * @param[in] f
 * @return
 */
long FS_filelength(fileHandle_t f)
{
	FILE *h;

	h = FS_FileForHandle(f);

	if (h == NULL)
	{
		return -1;
	}
	else
	{
		return FS_fplength(h);
	}
}

/**
 * @brief Fix things up differently for win/unix/mac
 * @param[in] path
 */
static void FS_ReplaceSeparators(char *path)
{
	char     *s;
	qboolean lastCharWasSep = qfalse;

	for (s = path ; *s ; s++)
	{
		if (*s == '/' || *s == '\\')
		{
			if (!lastCharWasSep)
			{
				*s             = PATH_SEP;
				lastCharWasSep = qtrue;
			}
			else
			{
				memmove(s, s + 1, strlen(s));
			}
		}
		else
		{
			lastCharWasSep = qfalse;
		}
	}
}

/**
 * @brief Normalize slashes in a file path to be posix/unix-like forward slashes
 * @param path
 * @return path
 */
char *FS_NormalizePath(const char *path)
{
	static char dir[MAX_OSPATH] = { 0 };

	Q_strncpyz(dir, path, sizeof(dir));
	Q_strncpyz(dir, Q_StrReplace(dir, "\\", "/"), sizeof(dir));

	return dir;
}

/**
 * @brief Qpath may have either forward or backwards slashes
 * @param[in] base
 * @param[in] game
 * @param[in] qpath
 * @return
 */
char *FS_BuildOSPath(const char *base, const char *game, const char *qpath)
{
	char        temp[MAX_OSPATH];
	static char ospath[2][MAX_OSPATH];
	static int  toggle;

	toggle ^= 1;        // flip-flop to allow two returns without clash

	if (!game || !game[0])
	{
		game = fs_gamedir;
	}

	if (qpath)
	{
		Com_sprintf(temp, sizeof(temp), "/%s/%s", game, qpath);
	}
	else
	{
		Com_sprintf(temp, sizeof(temp), "/%s", game);
	}

	FS_ReplaceSeparators(temp);
	Com_sprintf(ospath[toggle], sizeof(ospath[0]), "%s%s", base, temp);

	return ospath[toggle];
}

/**
 * @brief Creates any directories needed to store the given fileName
 * @param[in] OSPath
 * @return
 */
qboolean FS_CreatePath(const char *OSPath)
{
	char *ofs;
	char path[MAX_OSPATH];

	// make absolutely sure that it can't back up the path
	if (strstr(OSPath, "..") || strstr(OSPath, "::"))
	{
		Com_Printf("WARNING: refusing to create relative path \"%s\"\n", OSPath);
		return qtrue;
	}

	Q_strncpyz(path, OSPath, sizeof(path));
	FS_ReplaceSeparators(path);

	// Skip creation of the root directory as it will always be there
	ofs = strchr(path, PATH_SEP);
	if (ofs != NULL)
	{
		ofs++;
	}

	for (; ofs != NULL && *ofs ; ofs++)
	{
		if (*ofs == PATH_SEP)
		{
			// create the directory
			*ofs = 0;
			if (!Sys_Mkdir(path))
			{
				Com_Error(ERR_FATAL, "FS_CreatePath: failed to create path \"%s\"",
				          path);
			}
			*ofs = PATH_SEP;
		}
	}

	return qfalse;
}

/**
 * @brief Copy a fully specified file from one place to another
 * @param fromOSPath
 * @param toOSPath
 */
void FS_CopyFile(const char *fromOSPath, const char *toOSPath)
{
	FILE *f;
	int  len;
	byte *buf;

	if ((!fromOSPath || fromOSPath[0] == '\0') || (!toOSPath || toOSPath[0] == '\0'))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: cannot copy files. Empty path passed to FS_CopyFile\n");
		return;
	}

	Com_Printf("Copying %s to %s\n", fromOSPath, toOSPath);

	if (strstr(fromOSPath, "journal.dat") || strstr(fromOSPath, "journaldata.dat"))
	{
		Com_Printf("Ignoring journal files\n");
		return;
	}

	f = Sys_FOpen(fromOSPath, "rb");
	if (!f)
	{
		Com_Printf(S_COLOR_YELLOW "FS_CopyFile WARNING: cannot open file '%s'\n", fromOSPath);
		return;
	}

	fseek(f, 0, SEEK_END);
	len = ftell(f); // on failure, -1L is returned, and errno is set to a system-specific positive value
	if (len == -1)
	{
		Com_Error(ERR_FATAL, "FS_CopyFile: unable to get current position in stream of file '%s'", fromOSPath);
	}
	fseek(f, 0, SEEK_SET);

	buf = Com_Allocate(len);
	if (!buf)
	{
		Com_Error(ERR_FATAL, "FS_CopyFile: unable to allocate buffer");
	}
	if (fread(buf, 1, len, f) != len)
	{
		Com_Error(ERR_FATAL, "FS_CopyFile: Short read");
	}
	fclose(f);

	if (FS_CreatePath(toOSPath))
	{
		Com_Dealloc(buf);
		return;
	}

	f = Sys_FOpen(toOSPath, "wb");
	if (!f)
	{
		Com_Dealloc(buf);
		return;
	}
	if (fwrite(buf, 1, len, f) != len)
	{
		Com_Error(ERR_FATAL, "FS_CopyFile: short write");
	}
	fclose(f);
	Com_Dealloc(buf);
}

/**
 * @brief ERR_FATAL if trying to manipulate a file with the platform library extension
 * @param[in] fileName
 * @param[in] function
 */
void FS_CheckFilenameIsNotExecutable(const char *fileName, const char *function)
{
	// Check if the fileName ends with the library extension
	if (COM_CompareExtension(fileName, DLL_EXT))
	{
		Com_Error(ERR_FATAL, "%s: Not allowed to manipulate '%s' due "
		                     "to %s extension", function, fileName, DLL_EXT);
	}
}

/**
 * @brief ERR_FATAL if trying to maniuplate a file with the platform library or pk3 extension
 * @param filename
 * @param function
 */
static void FS_CheckFilenameIsMutable(const char *filename, const char *function)
{
	// Check if the filename ends with the library or pk3 extension
	if (Sys_DllExtension(filename) || COM_CompareExtension(filename, ".pk3"))
	{
		Com_Error(ERR_FATAL, "%s: Denied to manipulate '%s' due "
		                     "to %s extension", function, filename, COM_GetExtension(filename));
	}
}

/**
 * @brief FS_Remove
 * @param[in] osPath
 *
 * @note Ensure file removals are save when initiated by user inputs - call FS_CheckFilenameIsMutable
 *       or FS_CheckFilenameIsNotExecutable before to avoid unwanted binary or pk3 removals
 */
void FS_Remove(const char *osPath)
{
	int ret;

	ret = Sys_Remove(osPath);

	if (ret != 0)
	{
		Com_Printf(S_COLOR_YELLOW "FS_Remove WARNING: cannot remove file '%s'\n", osPath);
	}
}

/**
 * @brief FS_HomeRemove
 * @param[in] homePath
 */
void FS_HomeRemove(const char *homePath)
{
	int ret;

	FS_CheckFilenameIsNotExecutable(homePath, __func__); // keep in mind we want to delete pk3s in homepath

	ret = Sys_Remove(FS_BuildOSPath(fs_homepath->string, fs_gamedir, homePath));

	if (ret != 0)
	{
		Com_Printf(S_COLOR_YELLOW "FS_HomeRemove WARNING: cannot remove file '%s'\n", FS_BuildOSPath(fs_homepath->string, fs_gamedir, homePath));
	}
}

/**
 * @brief Tests if path and file exists
 * @param[in] ospath full OS -path to file
 * @return qtrue if the file exists
 */
qboolean FS_FileInPathExists(const char *ospath)
{
	FILE *fileHandle;

	if (!ospath || !ospath[0])
	{
		return qfalse;
	}

	fileHandle = Sys_FOpen(ospath, "rb");

	if (fileHandle)
	{
		fclose(fileHandle);
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Tests if the file exists in the current gamedir of fs_homepath
 *
 * DOES NOT search the paths. This is to determine if opening a file to
 * write (which always goes into the current gamedir) will cause any overwrites.
 *
 * @param[in] file
 *
 * @note This goes with FS_FOpenFileWrite for opening the file afterwards
 */
qboolean FS_FileExists(const char *file)
{
	if (!file)
	{
		Com_Printf("ERROR FS_FileExists: null fileName\n");
		return qfalse;
	}

	if (file[0] == '\0')
	{
		Com_Printf("WARNING FS_FileExists: empty fileName\n");
		return qfalse;
	}

	return FS_FileInPathExists(FS_BuildOSPath(fs_homepath->string, fs_gamedir, file));
}

/**
 * @brief Tests if the file in fs_homepath exists
 * @param[in] file
 * @return
 */
qboolean FS_SV_FileExists(const char *file, qboolean checkBase)
{
	char *testpath;
	qboolean homeFound = qfalse;

	testpath                       = FS_BuildOSPath(fs_homepath->string, file, "");
	testpath[strlen(testpath) - 1] = '\0';

	homeFound = FS_FileInPathExists(testpath);

	if (!homeFound && checkBase)
	{
		testpath                       = FS_BuildOSPath(fs_basepath->string, file, "");
		testpath[strlen(testpath) - 1] = '\0';
		return FS_FileInPathExists(testpath);
	}

	return homeFound;
}

/**
 * @brief FS_SV_FOpenFileWrite
 * @param[in] fileName
 * @return
 */
fileHandle_t FS_SV_FOpenFileWrite(const char *fileName)
{
	char         *ospath;
	fileHandle_t f;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_SV_FOpenFileWrite: Filesystem call made without initialization");
	}

	ospath                     = FS_BuildOSPath(fs_homepath->string, fileName, "");
	ospath[strlen(ospath) - 1] = '\0';

	f              = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	if (fs_debug->integer)
	{
		Com_Printf("FS_SV_FOpenFileWrite: %s\n", ospath);
	}

	FS_CheckFilenameIsMutable(ospath, __func__);

	if (FS_CreatePath(ospath))
	{
		return 0;
	}

	Com_DPrintf("writing to: %s\n", ospath);
	fsh[f].handleFiles.file.o = Sys_FOpen(ospath, "wb");

	Q_strncpyz(fsh[f].name, fileName, sizeof(fsh[f].name));

	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o)
	{
		f = 0;
	}

	return f;
}

/**
 * @brief search for a file somewhere below the home path, base path or cd path
 * we search in that order, matching FS_SV_FOpenFileRead order
 *
 * @param[in] fileName
 * @param[out] fp
 */
long FS_SV_FOpenFileRead(const char *fileName, fileHandle_t *fp)
{
	char         *ospath;
	fileHandle_t f = 0;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_SV_FOpenFileRead: Filesystem call made without initialization");
	}

	f              = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz(fsh[f].name, fileName, sizeof(fsh[f].name));

	// search homepath
	ospath = FS_BuildOSPath(fs_homepath->string, fileName, "");
	// remove trailing slash
	ospath[strlen(ospath) - 1] = '\0';

	if (fs_debug->integer)
	{
		Com_Printf("FS_SV_FOpenFileRead (fs_homepath): %s\n", ospath);
	}

	fsh[f].handleFiles.file.o = Sys_FOpen(ospath, "rb");
	fsh[f].handleSync         = qfalse;
	if (!fsh[f].handleFiles.file.o)
	{
		// If fs_homepath == fs_basepath, don't bother (dedicated server)
		// FIXME: use FS_PathCmp() (slashes, low/big letters ... trailing slash)
		if (Q_stricmp(fs_homepath->string, fs_basepath->string))
		{
			// search basepath
			ospath                     = FS_BuildOSPath(fs_basepath->string, fileName, "");
			ospath[strlen(ospath) - 1] = '\0';

			if (fs_debug->integer)
			{
				Com_Printf("FS_SV_FOpenFileRead (fs_basepath): %s\n", ospath);
			}

			fsh[f].handleFiles.file.o = Sys_FOpen(ospath, "rb");
			fsh[f].handleSync         = qfalse;
		}
	}

	// File not found
	if (!fsh[f].handleFiles.file.o)
	{
		f = 0;
	}

	*fp = f;
	if (f)
	{
		return FS_filelength(f);
	}

	return -1;
}

/**
 * @brief FS_SV_Rename - used to rename downloaded files from .tmp to .pk3
 * @param[in] from
 * @param[in] to
 */
void FS_SV_Rename(const char *from, const char *to)
{
	char *from_ospath, *to_ospath;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_SV_Rename: Filesystem call made without initialization");
	}

	from_ospath                          = FS_BuildOSPath(fs_homepath->string, from, "");
	to_ospath                            = FS_BuildOSPath(fs_homepath->string, to, "");
	from_ospath[strlen(from_ospath) - 1] = '\0';
	to_ospath[strlen(to_ospath) - 1]     = '\0';

	if (fs_debug->integer)
	{
		Com_Printf("FS_SV_Rename: %s --> %s\n", from_ospath, to_ospath);
	}
	FS_CheckFilenameIsNotExecutable(to_ospath, __func__);

	if (Sys_Rename(from_ospath, to_ospath))
	{
		// Failed, try copying it and deleting the original
		FS_CopyFile(from_ospath, to_ospath);
		// no need to check removal, this is our tmp file
		FS_Remove(from_ospath);
	}
}

/**
 * @brief FS_Rename
 * @param[in] from
 * @param[in] to
 */
void FS_Rename(const char *from, const char *to)
{
	char *from_ospath, *to_ospath;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_Rename: Filesystem call made without initialization");
	}

	from_ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, from);
	to_ospath   = FS_BuildOSPath(fs_homepath->string, fs_gamedir, to);

	if (fs_debug->integer)
	{
		Com_Printf("FS_Rename: %s --> %s\n", from_ospath, to_ospath);
	}
	FS_CheckFilenameIsMutable(to_ospath, __func__);

	if (Sys_Rename(from_ospath, to_ospath))
	{
		// Failed, try copying it and deleting the original
		FS_CopyFile(from_ospath, to_ospath);
		FS_CheckFilenameIsMutable(from_ospath, __func__);
		FS_Remove(from_ospath);
	}
}

/**
 * @brief If the FILE pointer is an open pak file, leave it open.
 * @param[in] f
 *
 * @note For some reason, other dll's can't just cal fclose()
 * on files returned by FS_FOpenFile...
 *
 */
void FS_FCloseFile(fileHandle_t f)
{
	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_FCloseFile: Filesystem call made without initialization");
	}

	if (fsh[f].zipFile == qtrue)
	{
		(void) unzCloseCurrentFile(fsh[f].handleFiles.file.z);
		if (fsh[f].handleFiles.unique)
		{
			unzClose(fsh[f].handleFiles.file.z);
		}
		Com_Memset(&fsh[f], 0, sizeof(fsh[f]));
		return;
	}

	// we didn't find it as a pak, so close it as a unique file
	if (fsh[f].handleFiles.file.o)
	{
		fclose(fsh[f].handleFiles.file.o);
	}
	Com_Memset(&fsh[f], 0, sizeof(fsh[f]));
}

/**
 * @brief FS_FOpenFileWrite
 * @param[in] fileName
 * @return
 */
fileHandle_t FS_FOpenFileWrite(const char *fileName)
{
	char         *ospath;
	fileHandle_t f;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_FOpenFileWrite: Filesystem call made without initialization");
	}

	f              = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, fileName);

	if (fs_debug->integer)
	{
		Com_Printf("FS_FOpenFileWrite: %s\n", ospath);
	}

	FS_CheckFilenameIsMutable(ospath, __func__);

	if (FS_CreatePath(ospath))
	{
		return 0;
	}

	fsh[f].handleFiles.file.o = Sys_FOpen(ospath, "wb");

	Q_strncpyz(fsh[f].name, fileName, sizeof(fsh[f].name));

	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o)
	{
		f = 0;
	}
	return f;
}

/**
 * @brief FS_FOpenDLLWrite
 * @param[in] fileName
 * @return
 */
fileHandle_t FS_FOpenDLLWrite(const char *fileName)
{
	char         *ospath;
	fileHandle_t f;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_FOpenDLLWrite: Filesystem call made without initialization\n");
	}

	f              = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, fileName);

	if (fs_debug->integer)
	{
		Com_Printf("FS_FOpenDLLWrite: %s\n", ospath);
	}

	if (FS_CreatePath(ospath))
	{
		return 0;
	}

	fsh[f].handleFiles.file.o = Sys_FOpen(ospath, "wb");

	Q_strncpyz(fsh[f].name, fileName, sizeof(fsh[f].name));

	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o)
	{
		f = 0;
	}
	return f;
}

/**
 * @brief FS_FOpenFileAppend
 * @param[in] fileName
 * @return
 */
fileHandle_t FS_FOpenFileAppend(const char *fileName)
{
	char         *ospath;
	fileHandle_t f;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_FOpenFileAppend: Filesystem call made without initialization");
	}

	f              = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz(fsh[f].name, fileName, sizeof(fsh[f].name));

	ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, fileName);

	if (fs_debug->integer)
	{
		Com_Printf("FS_FOpenFileAppend: %s\n", ospath);
	}

	FS_CheckFilenameIsMutable(ospath, __func__);

	if (FS_CreatePath(ospath))
	{
		return 0;
	}

	fsh[f].handleFiles.file.o = Sys_FOpen(ospath, "ab");
	fsh[f].handleSync         = qfalse;
	if (!fsh[f].handleFiles.file.o)
	{
		f = 0;
	}
	return f;
}

/**
 * @brief Compare filenames
 * Ignores case and separator char distinctions
 *
 * @param[in] s1
 * @param[in] s2
 * @return
 */
qboolean FS_FilenameCompare(const char *s1, const char *s2)
{
	int c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (c1 >= 'a' && c1 <= 'z')
		{
			c1 -= ('a' - 'A');
		}
		if (c2 >= 'a' && c2 <= 'z')
		{
			c2 -= ('a' - 'A');
		}

		if (c1 == '\\' || c1 == ':')
		{
			c1 = '/';
		}
		if (c2 == '\\' || c2 == ':')
		{
			c2 = '/';
		}

		if (c1 != c2)
		{
			return qtrue;      // strings not equal
		}
	}
	while (c1);

	return qfalse;       // strings are equal
}

/**
 * @brief FS_IsExt
 * @param[in] fileName
 * @param[in] ext
 * @param[in] namelen
 * @return qtrue if ext matches file extension fileName
 */
qboolean FS_IsExt(const char *fileName, const char *ext, int namelen)
{
	int extlen;

	extlen = strlen(ext);

	if (extlen > namelen)
	{
		return qfalse;
	}

	fileName += namelen - extlen;

	return !Q_stricmp(fileName, ext);
}

/**
 * @brief FS_IsDemoExt
 * @param[in] fileName
 * @param namelen - unused
 * @return qtrue if fileName has a demo extension
 */
qboolean FS_IsDemoExt(const char *fileName, int namelen)
{
	char *ext_test;

	ext_test = strrchr(fileName, '.');
	if (ext_test && (!Q_stricmpn(ext_test + 1, DEMOEXT, ARRAY_LEN(DEMOEXT) - 1) || !Q_stricmpn(ext_test + 1, SVDEMOEXT, ARRAY_LEN(SVDEMOEXT) - 1)))
	{
		int index;
		int protocol = Q_atoi(ext_test + ARRAY_LEN(DEMOEXT));

		if (protocol == PROTOCOL_VERSION /*com_protocol->integer*/)
		{
			return qtrue;
		}

		for (index = 0; demo_protocols[index]; index++)
		{
			if (demo_protocols[index] == protocol)
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

extern qboolean com_fullyInitialized;

static int fs_filter_flag = 0;

#ifdef _WIN32
static voidpf ZCALLBACK FS_UnzOpenFopenFileFunc (voidpf opaque, const char* filename, int mode)
{
	voidpf file = NULL;
	const char* mode_fopen = NULL;

	if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
	{
		mode_fopen = "rb";
	}
	else if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
	{
		mode_fopen = "r+b";
	}
	else if (mode & ZLIB_FILEFUNC_MODE_CREATE)
	{
		mode_fopen = "wb";
	}

	if ((filename!=NULL) && (mode_fopen != NULL))
	{
		file = Sys_FOpen(filename, mode_fopen);
	}
	return file;
}

static unzFile FS_UnzOpen(const char *fileName)
{
	static zlib_filefunc_def funcs;
	static qboolean funcsInit = qfalse;

	if(!funcsInit)
	{
		fill_fopen_filefunc(&funcs);
		funcs.zopen_file = FS_UnzOpenFopenFileFunc;
		funcsInit = qtrue;
	}

	return unzOpen2(fileName, &funcs);
}
#else
#define FS_UnzOpen(x) unzOpen(x)
#endif

/**
 * @brief Opens a file to read from an absolute system directory, should not be used instead of the normal FS methods
 * @param fullFileName Full file OS path
 * @param file filehandle
 * @return filesize or -1 if the open fails
 */
long FS_FOpenFileReadFullDir(const char *fullFileName, fileHandle_t *file)
{
	FILE         *filep;

	*file                         = FS_HandleForFile();
	fsh[*file].handleFiles.unique = qtrue;

	filep   = Sys_FOpen(fullFileName, "rb");
	if (filep == NULL)
	{
		*file = 0;
		return -1;
	}

	Q_strncpyz(fsh[*file].name, fullFileName, sizeof(fsh[*file].name));
	fsh[*file].zipFile = qfalse;

	if (fs_debug->integer)
	{
		Com_Printf("FS_FOpenFileRead: %s\n", fullFileName);
	}

	fsh[*file].handleFiles.file.o = filep;
	return FS_fplength(filep);
}

/**
 * @brief Finds the file in the search path.
 * Used for streaming data out of either a separate file or a ZIP file.
 *
 * @param[in] fileName to be opened
 * @param[in] search
 * @param[out] file set to open FILE pointer
 * @param[in] uniqueFILE
 * @param[in] unpure
 * @returns filesize or qboolean indicating if file exists when FILE pointer param is NULL
 */
long FS_FOpenFileReadDir(const char *fileName, searchpath_t *search, fileHandle_t *file, qboolean uniqueFILE, qboolean unpure)
{
	long         hash;
	pack_t       *pak;
	fileInPack_t *pakFile;
	directory_t  *dir;
	char         *netpath;
	FILE         *filep;
	int          len;

	if (fileName == NULL)
	{
		Com_Error(ERR_FATAL, "FS_FOpenFileReadDir: NULL 'fileName' parameter passed");
	}

	// qpaths are not supposed to have a leading slash
	if (fileName[0] == '/' || fileName[0] == '\\')
	{
		fileName++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if (strstr(fileName, "..") || strstr(fileName, "::"))
	{
		if (file == NULL)
		{
			return qfalse;
		}

		*file = 0;
		return -1;
	}

	// make sure the etkey file is only readable by the etl.exe at initialization
	// any other time the key should only be accessed in memory using the provided functions
	if (com_fullyInitialized && strstr(fileName, "etkey"))
	{
		if (file == NULL)
		{
			return qfalse;
		}

		*file = 0;
		return -1;
	}

	if (file == NULL)
	{
		// just wants to see if file is there

		// is the element a pak file?
		if (search->pack)
		{
			hash = FS_HashFileName(fileName, search->pack->hashSize);

			if (search->pack->hashTable[hash])
			{
				// look through all the pak file elements
				pak     = search->pack;
				pakFile = pak->hashTable[hash];

				do
				{
					// case and separator insensitive comparisons
					if (!FS_FilenameCompare(pakFile->name, fileName))
					{
						// found it!
						if (pakFile->len)
						{
							return pakFile->len;
						}
						else
						{
							// It's not nice, but legacy code depends
							// on positive value if file exists no matter
							// what size
							return 1;
						}
					}

					pakFile = pakFile->next;
				}
				while (pakFile != NULL);
			}
		}
		else if (search->dir)
		{
			dir = search->dir;

			netpath = FS_BuildOSPath(dir->path, dir->gamedir, fileName);
			filep   = Sys_FOpen(netpath, "rb");

			if (filep)
			{
				len = FS_fplength(filep);
				fclose(filep);

				if (len)
				{
					return len;
				}
				else
				{
					return 1;
				}
			}
		}

		return 0;
	}

	*file                         = FS_HandleForFile();
	fsh[*file].handleFiles.unique = uniqueFILE;

	// is the element a pak file?
	if (search->pack)
	{
		hash = FS_HashFileName(fileName, search->pack->hashSize);

		if (search->pack->hashTable[hash])
		{
			//qboolean includeCampaignFiles = (Cvar_VariableIntegerValue("g_gametype") == 4 || com_dedicated == NULL|| com_dedicated->integer == 0);
			qboolean includeCampaignFiles = (Cvar_VariableIntegerValue("g_gametype") == 4);

			// disregard if it doesn't match one of the allowed pure pak files
			if (!unpure && !FS_PakIsPure(search->pack))
			{
				*file = 0;
				return -1;
			}

			// look through all the pak file elements
			pak     = search->pack;
			pakFile = pak->hashTable[hash];

			do
			{
				// case and separator insensitive comparisons
				if (!FS_FilenameCompare(pakFile->name, fileName))
				{
					// found it!

					// mark the pak as having been referenced and mark specifics on cgame and ui
					// shaders, txt, arena files  by themselves do not count as a reference as
					// these are loaded from all pk3s
					// from every pk3 file..
					len = strlen(fileName);

					if (!(pak->referenced & FS_GENERAL_REF))
					{
						// blacklist
						if (!FS_IsExt(fileName, ".shader", len) &&
						    !FS_IsExt(fileName, ".txt", len) &&
						    !FS_IsExt(fileName, ".cfg", len) &&
						    !FS_IsExt(fileName, ".config", len) &&
						    !FS_IsExt(fileName, ".bot", len) && // not used in ET for real
						    !FS_IsExt(fileName, ".arena", len) &&
						    !FS_IsExt(fileName, ".menu", len) &&
						    Q_stricmp(fileName, Sys_GetDLLName("qagame")) != 0 &&
						    !strstr(fileName, "levelshots") &&
						    !FS_IsExt(fileName, ".campaign", len) // don't referernce for gametype != 4 - see below
						    )
						{
							pak->referenced |= FS_GENERAL_REF;
						}

						// special whitelist - objective gametype still has to reference 'campaign' pk3s
						// FIXME: dedicated campaign servers require an additional map restart when switching gametype to 4 while server is running with other gametypes
						// this won't trigger for the first map because g_gametype is latched cvar and cvar modfifications are processed later on
						// ... but this is better than populating the CS with not needed references and forcing players to download
						// maps/pk3s containing campaign files in other gametypes - delete the print after fix
						if (FS_IsExt(fileName, ".campaign", len) && includeCampaignFiles)
						{
							pak->referenced |= FS_GENERAL_REF;
							Com_Printf("^3Campaign PK3 file %s is referenced in search path!\n", fileName);
						}
					}

					// for OS client/server interoperability, we expect binaries for .so and .dll to be in the same pk3
					// so that when we reference the DLL files on any platform, this covers everyone else

					// qagame dll
					if (!(pak->referenced & FS_QAGAME_REF) && !Q_stricmp(fileName, Sys_GetDLLName("qagame")))
					{
						pak->referenced |= FS_QAGAME_REF;
					}
					// cgame dll
					if (!(pak->referenced & FS_CGAME_REF) && !Q_stricmp(fileName, Sys_GetDLLName("cgame")))
					{
						pak->referenced |= FS_CGAME_REF;
					}
					// ui dll
					if (!(pak->referenced & FS_UI_REF) && !Q_stricmp(fileName, Sys_GetDLLName("ui")))
					{
						pak->referenced |= FS_UI_REF;
					}

					if (uniqueFILE)
					{
						// open a new file on the pakfile
						fsh[*file].handleFiles.file.z = FS_UnzOpen(pak->pakFilename);

						if (fsh[*file].handleFiles.file.z == NULL)
						{
							Com_Error(ERR_FATAL, "FS_FOpenFileReadDir: Couldn't open %s", pak->pakFilename);
						}
					}
					else
					{
						fsh[*file].handleFiles.file.z = pak->handle;
					}

					Q_strncpyz(fsh[*file].name, fileName, sizeof(fsh[*file].name));
					fsh[*file].zipFile = qtrue;

					// set the file position in the zip file (also sets the current file info)
					unzSetOffset(fsh[*file].handleFiles.file.z, pakFile->pos);

					// open the file in the zip
					unzOpenCurrentFile(fsh[*file].handleFiles.file.z);
					fsh[*file].zipFilePos = pakFile->pos;
					fsh[*file].zipFileLen = pakFile->len;

					if (fs_debug->integer)
					{
						Com_Printf("FS_FOpenFileRead: %s (found in '%s')\n",
						           fileName, pak->pakFilename);
					}

					return pakFile->len;
				}

				pakFile = pakFile->next;
			}
			while (pakFile != NULL);
		}
	}
	else if (search->dir)
	{
		// check a file in the directory tree

		// if we are running restricted, the only files we
		// will allow to come from the directory are .cfg files
		len = strlen(fileName);
		// FIXME TTimo I'm not sure about the fs_numServerPaks test
		// if you are using FS_ReadFile to find out if a file exists,
		//   this test can make the search fail although the file is in the directory
		// I had the problem on https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=8
		// turned out I used FS_FileExists instead
		if (!unpure && fs_numServerPaks)
		{
			if (!FS_IsExt(fileName, ".cfg", len) &&      // for config files
			    !FS_IsExt(fileName, ".menu", len) &&    // menu files
			    !FS_IsExt(fileName, ".game", len) &&    // menu files
			    !FS_IsExt(fileName, ".dat", len) &&     // for journal files
			    !FS_IsExt(fileName, ".bin", len) &&     // glsl shader binary
#ifdef ETLEGACY_DEBUG
			    !FS_IsExt(fileName, ".glsl", len) &&
#endif
			    !FS_IsDemoExt(fileName, len))           // demos
			{
				*file = 0;
				return -1;
			}
		}

		dir = search->dir;

		netpath = FS_BuildOSPath(dir->path, dir->gamedir, fileName);
		filep   = Sys_FOpen(netpath, "rb");

		if (filep == NULL)
		{
			*file = 0;
			return -1;
		}

		Q_strncpyz(fsh[*file].name, fileName, sizeof(fsh[*file].name));
		fsh[*file].zipFile = qfalse;

		if (fs_debug->integer)
		{
			Com_Printf("FS_FOpenFileRead: %s (found in '%s/%s')\n", fileName,
			           dir->path, dir->gamedir);
		}

		fsh[*file].handleFiles.file.o = filep;
		return FS_fplength(filep);
	}

	*file = 0;
	return -1;
}

#if !defined(DEDICATED)
#define ALLOW_RAW_FILE_ACCESS (com_sv_running && com_sv_running->integer)
#else
#define ALLOW_RAW_FILE_ACCESS qfalse
#endif

/**
 * @brief Finds the file in the search path.
 * Used for streaming data out of either a separate file or a ZIP file.
 *
 * @param[in] fileName
 * @param[in,out] file
 * @param[in] uniqueFILE
 *
 * @returns filesize and an open FILE pointer -  0 or -1 for invalid files
 */
long FS_FOpenFileRead(const char *fileName, fileHandle_t *file, qboolean uniqueFILE)
{
	searchpath_t *search;
	long         len;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_FOpenFileRead: Filesystem call made without initialization");
	}

	for (search = fs_searchpaths; search; search = search->next)
	{
		if (search->pack && (fs_filter_flag & FS_EXCLUDE_PK3))
		{
			continue;
		}
		if (search->dir && (fs_filter_flag & FS_EXCLUDE_DIR))
		{
			continue;
		}

		len = FS_FOpenFileReadDir(fileName, search, file, uniqueFILE, ALLOW_RAW_FILE_ACCESS);

		if (file == NULL)
		{
			if (len > 0)
			{
				return len;
			}
		}
		else
		{
			if (len >= 0 && *file)
			{
				return len;
			}
		}
	}

#ifdef FS_MISSING
	if (missingFiles)
	{
		fprintf(missingFiles, "%s\n", fileName);
	}
#endif

	if (file)
	{
		Com_DPrintf(S_COLOR_RED "ERROR: Can't find %s\n", fileName);

		*file = 0;
		return -1;
	}
	else
	{
		// When file is NULL, we're querying the existance of the file
		// If we've got here, it doesn't exist
		return 0;
	}
}

long FS_FOpenFileRead_Filtered(const char *qpath, fileHandle_t *file, qboolean uniqueFILE, int filter_flag)
{
	long ret;

	fs_filter_flag = filter_flag;
	ret            = FS_FOpenFileRead(qpath, file, uniqueFILE);
	fs_filter_flag = 0;

	return ret;
}

// relevant to client only
#if !defined(DEDICATED)

/**
 * @brief Extracts the latest file from a pak file.
 *
 * @details Compares packed file against extracted file. If no differences, does not copy.
 * This is necessary for exe/dlls which may or may not be locked.
 *
 * @param[in] base
 * @param[in] gamedir
 * @param[in] fileName
 *
 * @return the return value doesn't tell whether file was extracted or not, it just says whether it's ok to continue
 * (i.e. either the right file was extracted successfully, or it was already present)
 *
 * @note fullpath gives the full OS path to the dll that will potentially be loaded
 * It can be fs_homepath/<fs_game>/ or fs_basepath/<fs_game>/
 * The dll is extracted to fs_homepath if needed
 *
 * @note cvar_lastVersion is the optional name of a CVAR_ARCHIVE used to store the wolf version for the last extracted .so
 */
qboolean FS_CL_ExtractFromPakFile(const char *base, const char *gamedir, const char *fileName)
{
	int      srcLength;
	byte     *srcData;
	byte     *destData;
	qboolean needToCopy = qtrue;
	FILE     *destHandle;
	char     outputFile[MAX_OSPATH];

	Q_strncpyz(outputFile, FS_BuildOSPath(base, gamedir, fileName), MAX_OSPATH * sizeof(char));

	// read in compressed file (force it to exclude files from directories)
	fs_filter_flag = FS_EXCLUDE_DIR;
	srcLength      = FS_ReadFile(fileName, (void **)&srcData);
	fs_filter_flag = 0;

	// if its not in the pak, we bail
	if (srcLength <= 0)
	{
		Com_Printf("FS_CL_ExtractFromPakFile: failed to read '%s' from pak file\n", fileName);
		return qfalse;
	}

	// read in local file
	destHandle = Sys_FOpen(outputFile, "rb");

	// if we have a local file, we need to compare the two
	if (destHandle)
	{
		int destLength;

		fseek(destHandle, 0, SEEK_END);
		destLength = ftell(destHandle);
		fseek(destHandle, 0, SEEK_SET);

		if (destLength > 0)
		{
			destData = (byte *)Z_Malloc(destLength);

			if (fread(destData, destLength, 1, destHandle) != 1 && (!feof(destHandle) || ferror(destHandle)))
			{
				Com_Error(ERR_FATAL, "FS_CL_ExtractFromPakFile: Short read '%s'", fileName);
			}
			// compare files
			if (destLength == srcLength)
			{
				int i;

				for (i = 0; i < destLength; i++)
				{
					if (destData[i] != srcData[i])
					{
						break;
					}
				}

				if (i == destLength)
				{
					needToCopy = qfalse;
				}
			}

			Z_Free(destData);
		}

		fclose(destHandle);
	}

	// write file
	if (needToCopy)
	{
		fileHandle_t f;

		f = FS_FOpenDLLWrite(fileName);
		if (!f)
		{
			Com_Printf("FS_CL_ExtractFromPakFile: failed to open %s\n", fileName);
			return qfalse;
		}

		FS_Write(srcData, srcLength, f);

		FS_FCloseFile(f);
	}

	FS_FreeFile(srcData);
	return qtrue;
}
#endif

/**
 * @brief FS_AllowDeletion
 * @param[in] fileName
 * @return
 */
qboolean FS_AllowDeletion(const char *fileName)
{
	// for safety, only allow deletion from the save, profiles, demo directory and pid file
	if (Q_strncmp(fileName, "save/", 5) != 0 &&
	    Q_strncmp(fileName, "profiles/", 9) != 0 &&
	    Q_strncmp(fileName, "demos/", 6) != 0 &&
	    !strstr(fileName, ".pid")) // PID file
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief FS_DeleteDir
 * @param[in] dirname
 * @param[in] nonEmpty
 * @param[in] recursive
 * @return
 */
int FS_DeleteDir(const char *dirname, qboolean nonEmpty, qboolean recursive)
{
	char *ospath;
	char **pFiles = NULL;
	int  i, nFiles = 0;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_DeleteDir: Filesystem call made without initialization");
	}

	if (!dirname || dirname[0] == 0)
	{
		return 0;
	}

	if (!FS_AllowDeletion(dirname))
	{
		return 0;
	}

	if (recursive)
	{
		ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, dirname);
		pFiles = Sys_ListFiles(ospath, "/", NULL, &nFiles, qfalse);
		for (i = 0; i < nFiles; i++)
		{
			char temp[MAX_OSPATH];

			if (!Q_stricmp(pFiles[i], "..") || !Q_stricmp(pFiles[i], "."))
			{
				continue;
			}

			Com_sprintf(temp, sizeof(temp), "%s/%s", dirname, pFiles[i]);

			if (!FS_DeleteDir(temp, nonEmpty, recursive))
			{
				return 0;
			}
		}
		Sys_FreeFileList(pFiles);
	}

	if (nonEmpty)
	{
		ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, dirname);
		pFiles = Sys_ListFiles(ospath, NULL, NULL, &nFiles, qfalse);
		for (i = 0; i < nFiles; i++)
		{
			ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s/%s", dirname, pFiles[i]));

			if (Sys_Remove(ospath) == -1)        // failure
			{
				return 0;
			}
		}
		Sys_FreeFileList(pFiles);
	}

	ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, dirname);

	if (Sys_RemoveDir(ospath) == 0)
	{
		return 1;
	}

	return 0;
}

/**
 * @brief Test an file given OS path
 * @param ospath
 * @return -1 if not found
 *          1 if directory
 *          0 otherwise
 */
int FS_OSStatFile(const char *ospath)
{
	sys_stat_t stat_buf;

	if (Sys_Stat(ospath, &stat_buf) == -1)
	{
		return -1;
	}
	if (Sys_S_IsDir(stat_buf.st_mode))
	{
		return 1;
	}

	return 0;
}

/**
 * @brief Return the age of the file in seconds
 * @param ospath full OS path to the file to check
 * @return time in seconds and -1 if not possible
 */
long FS_FileAge(const char *ospath)
{
	sys_stat_t stat_buf;
	time_t now, creation;

	if (Sys_Stat(ospath, &stat_buf) == -1)
	{
		return -1;
	}

	time(&now);
	creation = stat_buf.st_ctime;
	if (creation <= 0)
	{
		return -1;
	}

	return (now - creation);
}

/**
 * @brief Removes file in the current fs_gamedir in homepath
 * @param[in] fileName
 * @return
 */
int FS_Delete(const char *fileName)
{
	char *ospath;
	int  stat;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_Delete: Filesystem call made without initialization");
	}

	if (!fileName || fileName[0] == 0)
	{
		return 0;
	}

	if (!FS_AllowDeletion(fileName))
	{
		return 0;
	}

	ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, fileName);

	FS_CheckFilenameIsNotExecutable(ospath, __func__);

	stat = FS_OSStatFile(ospath);
	if (stat == -1)
	{
		return 0;
	}

	if (stat == 1)
	{
		return FS_DeleteDir(fileName, qtrue, qtrue);
	}
	else
	{
		if (Sys_Remove(ospath) != -1)        // success
		{
			return 1;
		}
	}

	return 0;
}

/**
 * @brief FS_Read
 * @param[out] buffer
 * @param[in] len
 * @param[in] f
 * @return
 */
int FS_Read(void *buffer, int len, fileHandle_t f)
{
	byte *buf;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_Read: Filesystem call made without initialization");
	}

	if (!f)
	{
		return 0;
	}

	buf           = (byte *)buffer;
	fs_readCount += len;

	if (fsh[f].zipFile == qfalse)
	{
		int read, block;
		int remaining = len;
		int tries     = 0;

		while (remaining)
		{
			block = remaining;
			read  = fread(buf, 1, block, fsh[f].handleFiles.file.o);
			if (read == 0)
			{
				// we might have been trying to read from a CD, which
				// sometimes returns a 0 read on windows
				if (!tries)
				{
					tries = 1;
				}
				else
				{
					return len - remaining;   //Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
				}
			}

			if (read == -1)
			{
				Com_Error(ERR_FATAL, "FS_Read: -1 bytes read");
			}

			remaining -= read;
			buf       += read;
		}
		return len;
	}
	else
	{
		return unzReadCurrentFile(fsh[f].handleFiles.file.z, buffer, len);
	}
}

/**
 * @brief Properly handles partial writes
 * @param[in] buffer
 * @param[in] len
 * @param[in] h
 * @return
 */
int FS_Write(const void *buffer, int len, fileHandle_t h)
{
	int  block, remaining;
	int  written;
	byte *buf;
	int  tries;
	FILE *f;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_Write: Filesystem call made without initialization");
	}

	if (!h)
	{
		return 0;
	}

	f   = FS_FileForHandle(h);
	buf = (byte *)buffer;

	remaining = len;
	tries     = 0;
	while (remaining)
	{
		block   = remaining;
		written = fwrite(buf, 1, block, f);
		if (written == 0)
		{
			if (!tries)
			{
				tries = 1;
			}
			else
			{
				Com_Printf("FS_Write: 0 bytes written (%d attempted)\n", block);
				return 0;
			}
		}

		if (written < 0)
		{
			Com_Printf("FS_Write: %d bytes written (%d attempted)\n", written, block);
			return 0;
		}

		remaining -= written;
		buf       += written;
	}
	if (fsh[h].handleSync)
	{
		fflush(f);
	}
	return len;
}

/**
 * @brief FS_Printf
 * @param[in] h
 * @param[in] fmt
 */
void QDECL FS_Printf(fileHandle_t h, const char *fmt, ...)
{
	va_list argptr;
	char    msg[MAX_PRINT_MSG];

	va_start(argptr, fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	FS_Write(msg, strlen(msg), h);
}

#define PK3_SEEK_BUFFER_SIZE 65536

/**
 * @brief FS_Seek
 * @param[in] f
 * @param[in] offset
 * @param[in] origin
 * @return
 */
int FS_Seek(fileHandle_t f, long offset, int origin)
{
	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_Seek: Filesystem call made without initialization");
		return -1;
	}

	if (fsh[f].zipFile == qtrue)
	{
		// FIXME: this is really, really
		// crappy (but better than what was here before)
		byte buffer[PK3_SEEK_BUFFER_SIZE];
		int  remainder;
		int  currentPosition = FS_FTell(f);

		// change negative offsets into FS_SEEK_SET
		if (offset < 0)
		{
			switch (origin)
			{
			case FS_SEEK_END:
				remainder = fsh[f].zipFileLen + offset;
				break;

			case FS_SEEK_CUR:
				remainder = currentPosition + offset;
				break;

			case FS_SEEK_SET:
			default:
				remainder = 0;
				break;
			}

			if (remainder < 0)
			{
				remainder = 0;
			}

			origin = FS_SEEK_SET;
		}
		else
		{
			if (origin == FS_SEEK_END)
			{
				remainder = fsh[f].zipFileLen - currentPosition + offset;
			}
			else
			{
				remainder = offset;
			}
		}

		switch (origin)
		{
		case FS_SEEK_SET:
			if (remainder == currentPosition)
			{
				return offset;
			}
			unzSetOffset(fsh[f].handleFiles.file.z, fsh[f].zipFilePos);
			unzOpenCurrentFile(fsh[f].handleFiles.file.z);
		// fall through
		case FS_SEEK_END:
		case FS_SEEK_CUR:
			while (remainder > PK3_SEEK_BUFFER_SIZE)
			{
				FS_Read(buffer, PK3_SEEK_BUFFER_SIZE, f);
				remainder -= PK3_SEEK_BUFFER_SIZE;
			}
			FS_Read(buffer, remainder, f);
			return offset;
		default:
			Com_Error(ERR_FATAL, "Bad origin in FS_Seek");
			return -1;
		}
	}
	else
	{
		int  _origin;
		FILE *file;

		file = FS_FileForHandle(f);

		switch (origin)
		{
		case FS_SEEK_CUR:
			_origin = SEEK_CUR;
			break;
		case FS_SEEK_END:
			_origin = SEEK_END;
			break;
		case FS_SEEK_SET:
			_origin = SEEK_SET;
			break;
		default:
			_origin = SEEK_CUR;
			Com_Error(ERR_FATAL, "FS_Seek: Bad origin");
		}

		return fseek(file, offset, _origin);
	}
}

/**
======================================================================================
CONVENIENCE FUNCTIONS FOR ENTIRE FILES
======================================================================================
*/

/**
 * @brief FS_FileIsInPAK
 * @param[in] fileName
 * @param[out] pChecksum
 * @return
 */
int FS_FileIsInPAK(const char *fileName, int *pChecksum)
{
	searchpath_t *search;
	pack_t       *pak;
	fileInPack_t *pakFile;
	long         hash = 0;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_FileIsInPAK: Filesystem call made without initialization");
	}

	if (!fileName)
	{
		Com_Error(ERR_FATAL, "FS_FileIsInPAK: NULL 'fileName' parameter passed");
	}

	// qpaths are not supposed to have a leading slash
	if (fileName[0] == '/' || fileName[0] == '\\')
	{
		fileName++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if (strstr(fileName, "..") || strstr(fileName, "::"))
	{
		return -1;
	}

	// search through the path, one element at a time
	for (search = fs_searchpaths ; search ; search = search->next)
	{
		// is the element a pak file?
		if (search->pack)
		{
			hash = FS_HashFileName(fileName, search->pack->hashSize);
		}
		// is the element a pak file?
		if (search->pack && search->pack->hashTable[hash])
		{
			// disregard if it doesn't match one of the allowed pure pak files
			if (!FS_PakIsPure(search->pack))
			{
				continue;
			}

			// look through all the pak file elements
			pak     = search->pack;
			pakFile = pak->hashTable[hash];
			do
			{
				// case and separator insensitive comparisons
				if (!FS_FilenameCompare(pakFile->name, fileName))
				{
					if (pChecksum)
					{
						*pChecksum = pak->pure_checksum;
					}
					return 1;
				}
				pakFile = pakFile->next;
			}
			while (pakFile != NULL);
		}
	}
	return -1;
}

/**
 * @brief Open a file relative to the ET:L search path.
 * A null buffer will just return the file length without loading.
 * @param[in] qpath
 * @param[out] buffer
 * @return
 */
int FS_ReadFile(const char *qpath, void **buffer)
{
	fileHandle_t h;
	byte         *buf;
	qboolean     isConfig;
	int          len;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_ReadFile: Filesystem call made without initialization");
	}

	if (!qpath || !qpath[0])
	{
		Com_Error(ERR_FATAL, "FS_ReadFile: empty name");
	}

	buf = NULL; // quiet compiler warning

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if (strstr(qpath, ".cfg"))
	{
		isConfig = qtrue;
		if (com_journal && com_journal->integer == 2)
		{
			int r;

			Com_DPrintf("Loading %s from journal file.\n", qpath);
			r = FS_Read(&len, sizeof(len), com_journalDataFile);
			if (r != sizeof(len))
			{
				if (buffer != NULL)
				{
					*buffer = NULL;
				}
				return -1;
			}
			// if the file didn't exist when the journal was created
			if (!len)
			{
				if (buffer == NULL)
				{
					return 1;           // hack for old journal files
				}
				*buffer = NULL;
				return -1;
			}
			if (buffer == NULL)
			{
				return len;
			}

			buf     = Hunk_AllocateTempMemory(len + 1);
			*buffer = buf;

			r = FS_Read(buf, len, com_journalDataFile);
			if (r != len)
			{
				Com_Error(ERR_FATAL, "FS_ReadFile: Read from journalDataFile failed");
			}

			fs_loadCount++;
			fs_loadStack++;

			// guarantee that it will have a trailing 0 for string operations
			buf[len] = 0;

			return len;
		}
	}
	else
	{
		isConfig = qfalse;
	}

	// look for it in the filesystem or pack files
	len = FS_FOpenFileRead(qpath, &h, qfalse);
	if (h == 0)
	{
		if (buffer)
		{
			*buffer = NULL;
		}
		// if we are journalling and it is a config file, write a zero to the journal file
		if (isConfig && com_journal && com_journal->integer == 1)
		{
			Com_DPrintf("Writing zero for %s to journal file.\n", qpath);
			len = 0;
			FS_Write(&len, sizeof(len), com_journalDataFile);
			FS_Flush(com_journalDataFile);
		}
		return -1;
	}

	if (!buffer)
	{
		if (isConfig && com_journal && com_journal->integer == 1)
		{
			Com_DPrintf("Writing len for %s to journal file.\n", qpath);
			FS_Write(&len, sizeof(len), com_journalDataFile);
			FS_Flush(com_journalDataFile);
		}
		FS_FCloseFile(h);
		return len;
	}

	fs_loadCount++;
	fs_loadStack++;

	buf     = Hunk_AllocateTempMemory(len + 1);
	*buffer = buf;

	FS_Read(buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFile(h);

	// if we are journalling and it is a config file, write it to the journal file
	if (isConfig && com_journal && com_journal->integer == 1)
	{
		Com_DPrintf("Writing %s to journal file.\n", qpath);
		FS_Write(&len, sizeof(len), com_journalDataFile);
		FS_Write(buf, len, com_journalDataFile);
		FS_Flush(com_journalDataFile);
	}
	return len;
}

/**
 * @brief FS_FreeFile
 * @param[out] buffer
 */
void FS_FreeFile(void *buffer)
{
	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_FreeFile: Filesystem call made without initialization");
	}
	if (!buffer)
	{
		Com_Error(ERR_FATAL, "FS_FreeFile: NULL parameter");
	}
	fs_loadStack--;

	Hunk_FreeTempMemory(buffer);

	// if all of our temp files are free, clear all of our space
	if (fs_loadStack == 0)
	{
		Hunk_ClearTempMemory();
	}
}

/**
 * @brief Filename are reletive to the quake search path
 * @param[in] qpath
 * @param[out] buffer
 * @param[in] size
 */
void FS_WriteFile(const char *qpath, const void *buffer, int size)
{
	fileHandle_t f;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_WriteFile: Filesystem call made without initialization");
	}

	if (!qpath || !buffer)
	{
		Com_Error(ERR_FATAL, "FS_WriteFile: NULL parameter");
	}

	f = FS_FOpenFileWrite(qpath);
	if (!f)
	{
		Com_Printf("Failed to open %s\n", qpath);
		return;
	}

	FS_Write(buffer, size, f);

	FS_FCloseFile(f);
}

/**
==========================================================================
ZIP FILE LOADING
==========================================================================
*/

/**
 * @brief Creates a new pak_t in the search chain for the contents of a zip file.
 * @param[in] zipfile
 * @param[in] basename
 * @return
 */
static pack_t *FS_LoadZipFile(const char *zipfile, const char *basename)
{
	fileInPack_t    *buildBuffer;
	pack_t          *pack;
	unzFile         uf;
	int             err;
	unz_global_info gi;
	char            fileName_inzip[MAX_ZPATH];
	unz_file_info   file_info;
	unsigned int    i, len;
	long            hash;
	int             fs_numHeaderLongs = 0;
	int             *fs_headerLongs;
	char            *namePtr;

	uf  = FS_UnzOpen(zipfile);
	err = unzGetGlobalInfo(uf, &gi);

	if (err != UNZ_OK)
	{
		return NULL;
	}

	len = 0;
	unzGoToFirstFile(uf);
	for (i = 0; i < gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, fileName_inzip, sizeof(fileName_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK)
		{
			break;
		}
		len += strlen(fileName_inzip) + 1;
		unzGoToNextFile(uf);
	}

	buildBuffer                         = Z_Malloc((gi.number_entry * sizeof(fileInPack_t)) + len);
	namePtr                             = ((char *) buildBuffer) + gi.number_entry * sizeof(fileInPack_t);
	fs_headerLongs                      = Z_Malloc((gi.number_entry + 1) * sizeof(int));
	fs_headerLongs[fs_numHeaderLongs++] = LittleLong(fs_checksumFeed);

	// get the hash table size from the number of files in the zip
	// because lots of custom pk3 files have less than 32 or 64 files
	for (i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1)
	{
		if (i > gi.number_entry)
		{
			break;
		}
	}

	pack            = Z_Malloc(sizeof(pack_t) + i * sizeof(fileInPack_t *));
	pack->hashSize  = i;
	pack->hashTable = ( fileInPack_t ** )(((char *) pack) + sizeof(pack_t));
	for (i = 0; i < pack->hashSize; i++)
	{
		pack->hashTable[i] = NULL;
	}

	Q_strncpyz(pack->pakFilename, zipfile, sizeof(pack->pakFilename));
	Q_strncpyz(pack->pakBasename, basename, sizeof(pack->pakBasename));

	// strip .pk3 if needed
	if (strlen(pack->pakBasename) > 4 && !Q_stricmp(pack->pakBasename + strlen(pack->pakBasename) - 4, ".pk3"))
	{
		pack->pakBasename[strlen(pack->pakBasename) - 4] = 0;
	}

	pack->handle   = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile(uf);

	for (i = 0; i < gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, fileName_inzip, sizeof(fileName_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK)
		{
			break;
		}
		if (file_info.uncompressed_size > 0)
		{
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong(file_info.crc);
		}
		Q_strlwr(fileName_inzip);
		hash                = FS_HashFileName(fileName_inzip, pack->hashSize);
		buildBuffer[i].name = namePtr;
		strcpy(buildBuffer[i].name, fileName_inzip);
		namePtr += strlen(fileName_inzip) + 1;
		// store the file position in the zip
		buildBuffer[i].pos    = unzGetOffset(uf);
		buildBuffer[i].len    = file_info.uncompressed_size;
		buildBuffer[i].next   = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile(uf);
	}

	pack->checksum      = Com_BlockChecksum(&fs_headerLongs[1], sizeof(*fs_headerLongs) * (fs_numHeaderLongs - 1));
	pack->pure_checksum = Com_BlockChecksum(fs_headerLongs, sizeof(*fs_headerLongs) * fs_numHeaderLongs);
	pack->checksum      = LittleLong(pack->checksum);
	pack->pure_checksum = LittleLong(pack->pure_checksum);

	Z_Free(fs_headerLongs);

	pack->buildBuffer = buildBuffer;
	return pack;
}

/**
 * @brief Frees a pak structure and releases all associated resources
 * @param[in] thepak
 */
static void FS_FreePak(pack_t *thepak)
{
	unzClose(thepak->handle);
	Z_Free(thepak->buildBuffer);
	Z_Free(thepak);
}

/**
 * @brief Compares whether the given pak file matches a referenced checksum
 * @param[in] zipfile
 * @param[in] checksum
 * @return
 *
 * @note Unused
 */
qboolean FS_CompareZipChecksum(const char *zipfile, const int checksum)
{
	pack_t *thepak;
	int    index, zipChecksum;

	thepak = FS_LoadZipFile(zipfile, "");

	if (!thepak)
	{
		return qfalse;
	}

	zipChecksum = thepak->checksum;
	FS_FreePak(thepak);

	if(checksum)
	{
		return checksum == zipChecksum;
	}

	for (index = 0; index < fs_numServerReferencedPaks; index++)
	{
		if (zipChecksum == fs_serverReferencedPaks[index])
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief find a pack matching the checksum
 * @param checksum to find
 * @return the found pack or NULL
 */
static pack_t *FS_FindPack(const int checksum)
{
	searchpath_t *sp;
	for (sp = fs_searchpaths ; sp ; sp = sp->next)
	{
		if (sp->pack && sp->pack->checksum == checksum)
		{
			return sp->pack;
		}
	}

	return NULL;
}

/**
 * @brief does the search path contain a pack with checksum
 * @param checksum to find
 * @return qtrue if pack is found
 */
static inline qboolean FS_HasPack(const int checksum)
{
	return FS_FindPack(checksum) != NULL;
}

/**
=================================================================================
DIRECTORY SCANNING FUNCTIONS
=================================================================================
*/

/**
 * @brief FS_ReturnPath
 * @param[in] zname
 * @param[out] zpath
 * @param[out] depth
 * @return
 */
static int FS_ReturnPath(const char *zname, char *zpath, int *depth)
{
	int len = 0, at = 0, newdep = 0;

	zpath[0] = 0;

	while (zname[at] != 0)
	{
		if (zname[at] == '/' || zname[at] == '\\')
		{
			len = at;
			newdep++;
		}
		at++;
	}
	strcpy(zpath, zname);
	zpath[len] = 0;
	*depth     = newdep;

	return len;
}

/**
 * @brief FS_AddFileToList
 * @param[in] name
 * @param[out] list
 * @param[in] nfiles
 * @return
 */
static int FS_AddFileToList(const char *name, char *list[MAX_FOUND_FILES], int nfiles)
{
	int i;

	if (nfiles == MAX_FOUND_FILES - 1)
	{
		return nfiles;
	}
	for (i = 0 ; i < nfiles ; i++)
	{
		if (!Q_stricmp(name, list[i]))
		{
			return nfiles;      // already in list
		}
	}
	list[nfiles] = CopyString(name);
	nfiles++;

	return nfiles;
}

/**
 * @brief FS_ListFilteredFiles
 * @param[in] path
 * @param[in] extension
 * @param[in] filter
 * @param[out] numfiles
 * @param[in] allowNonPureFilesOnDisk
 * @return A uniqued list of files that match the given criteria
 * from all search paths
 */
char **FS_ListFilteredFiles(const char *path, const char *extension, const char *filter, int *numfiles, qboolean allowNonPureFilesOnDisk)
{
	int          nfiles;
	char         **listCopy;
	char         *list[MAX_FOUND_FILES];
	searchpath_t *search;
	int          i;
	int          pathLength;
	int          extensionLength;
	int          length, pathDepth, temp;
	pack_t       *pak;
	fileInPack_t *buildBuffer;
	char         zpath[MAX_ZPATH];
	char         *name;
	int          zpathLen, depth;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_ListFilteredFiles: Filesystem call made without initialization");
	}

	if (!path)
	{
		*numfiles = 0;
		return NULL;
	}
	if (!extension)
	{
		extension = "";
	}

	pathLength = strlen(path);
	if (path[pathLength - 1] == '\\' || path[pathLength - 1] == '/')
	{
		pathLength--;
	}
	extensionLength = strlen(extension);
	nfiles          = 0;
	FS_ReturnPath(path, zpath, &pathDepth);

	// search through the path, one element at a time, adding to list
	for (search = fs_searchpaths ; search ; search = search->next)
	{
		// is the element a pak file?
		if (search->pack)
		{
			// If we are pure, don't search for files on paks that
			// aren't on the pure list
			if (!FS_PakIsPure(search->pack))
			{
				continue;
			}

			// look through all the pak file elements
			pak         = search->pack;
			buildBuffer = pak->buildBuffer;
			for (i = 0; i < pak->numfiles; i++)
			{
				// check for directory match
				name = buildBuffer[i].name;

				if (filter)
				{
					// case insensitive
					if (!Com_FilterPath(filter, name, qfalse))
					{
						continue;
					}
					// unique the match
					nfiles = FS_AddFileToList(name, list, nfiles);
				}
				else
				{
					zpathLen = FS_ReturnPath(name, zpath, &depth);

					if ((depth - pathDepth) > 2 || pathLength > zpathLen || Q_stricmpn(name, path, pathLength))
					{
						continue;
					}

					// check for extension match
					length = strlen(name);
					if (length < extensionLength)
					{
						continue;
					}

					if (Q_stricmp(name + length - extensionLength, extension))
					{
						continue;
					}
					// unique the match

					temp = pathLength;
					if (pathLength)
					{
						temp++;     // include the '/'
					}
					nfiles = FS_AddFileToList(name + temp, list, nfiles);
				}
			}
		}
		else if (search->dir)     // scan for files in the filesystem
		{
			// don't scan directories for files if we are pure or restricted
			if (fs_numServerPaks && !allowNonPureFilesOnDisk)
			{
				continue;
			}
			else
			{
				char *netpath;
				int  numSysFiles;
				char **sysFiles;
				char *name;

				netpath  = FS_BuildOSPath(search->dir->path, search->dir->gamedir, path);
				sysFiles = Sys_ListFiles(netpath, extension, filter, &numSysFiles, qfalse);
				for (i = 0 ; i < numSysFiles ; i++)
				{
					// unique the match
					name   = sysFiles[i];
					nfiles = FS_AddFileToList(name, list, nfiles);
				}
				Sys_FreeFileList(sysFiles);
			}
		}
	}

	// return a copy of the list
	*numfiles = nfiles;

	if (!nfiles)
	{
		return NULL;
	}

	listCopy = Z_Malloc((nfiles + 1) * sizeof(*listCopy));
	for (i = 0 ; i < nfiles ; i++)
	{
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

/**
 * @brief FS_ListFiles
 * @param[in] path
 * @param[in] extension
 * @param[out] numfiles
 * @return
 */
char **FS_ListFiles(const char *path, const char *extension, int *numfiles)
{
	return FS_ListFilteredFiles(path, extension, NULL, numfiles, ALLOW_RAW_FILE_ACCESS);
}

/**
 * @brief FS_FreeFileList
 * @param[in,out] list
 */
void FS_FreeFileList(char **list)
{
	int i;

	if (!fs_searchpaths)
	{
		Com_Error(ERR_FATAL, "FS_FreeFileList: Filesystem call made without initialization");
	}

	if (!list)
	{
		return;
	}

	for (i = 0 ; list[i] ; i++)
	{
		Z_Free(list[i]);
	}

	Z_Free(list);
}

/**
 * @brief FS_GetFileList
 * @param[in] path
 * @param[in] extension
 * @param[out] listbuf
 * @param[in] bufsize
 * @return
 */
int FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	int  nFiles = 0, i, nTotal = 0, nLen;
	char **pFiles = NULL;

	*listbuf = 0;

	if (Q_stricmp(path, "$modlist") == 0)
	{
		return FS_GetModList(listbuf, bufsize);
	}

	pFiles = FS_ListFiles(path, extension, &nFiles);

	for (i = 0; i < nFiles; i++)
	{
		nLen = strlen(pFiles[i]) + 1;
		if (nTotal + nLen + 1 < bufsize)
		{
			strcpy(listbuf, pFiles[i]);
			listbuf += nLen;
			nTotal  += nLen;
		}
		else
		{
			nFiles = i;
			break;
		}
	}

	FS_FreeFileList(pFiles);

	return nFiles;
}

/**
 * @brief Sys_CountFileList
 * @param[in] list
 * @return
 *
 * @note mkv: Naive implementation. Concatenates three lists into a
 *      new list, and frees the old lists from the heap.
 * bk001129 - from cvs1.17 (mkv)
 *
 * @todo FIXME those two should move to common.c next to Sys_ListFiles
 */
static unsigned int Sys_CountFileList(char **list)
{
	unsigned int i = 0;

	if (list)
	{
		while (*list)
		{
			list++;
			i++;
		}
	}
	return i;
}

/**
 * @brief Sys_ConcatenateFileLists
 * @param[in,out] list0
 * @param[in,out] list1
 * @param[in,out] list2
 * @return
 *
 * @note mkv: Naive implementation. Concatenates three lists into a
 *      new list, and frees the old lists from the heap.
 * bk001129 - from cvs1.17 (mkv)
 *
 * @todo FIXME those two should move to common.c next to Sys_ListFiles
 */
static char **Sys_ConcatenateFileLists(char **list0, char **list1, char **list2)
{
	int  totalLength = 0;
	char **cat = NULL, **dst, **src;

	totalLength += Sys_CountFileList(list0);
	totalLength += Sys_CountFileList(list1);
	totalLength += Sys_CountFileList(list2);

	/* Create new list. */
	dst = cat = Z_Malloc((totalLength + 1) * sizeof(char *));

	/* Copy over lists. */
	if (list0)
	{
		for (src = list0; *src; src++, dst++)
			*dst = *src;
	}
	if (list1)
	{
		for (src = list1; *src; src++, dst++)
			*dst = *src;
	}
	if (list2)
	{
		for (src = list2; *src; src++, dst++)
			*dst = *src;
	}

	// Terminate the list
	*dst = NULL;

	// Free our old lists.
	// NOTE: not freeing their content, it's been merged in dst and still being used
	if (list0)
	{
		Z_Free(list0);
	}
	if (list1)
	{
		Z_Free(list1);
	}
	if (list2)
	{
		Z_Free(list2);
	}

	return cat;
}

/**
 * @brief A mod directory is a peer to etmain with a pk3 in it
 * The directories are searched in base path and home path
 * @param listbuf
 * @param bufsize
 * @return A list of mod directory names
 */
int FS_GetModList(char *listbuf, int bufsize)
{
	int          nMods = 0, i, j, nTotal = 0, nLen, nPaks, nPotential, nDescLen;
	char         **pPaks = NULL;
	char         *name, *path;
	char         descPath[MAX_OSPATH];
	fileHandle_t descHandle;
	int          dummy;
	char         **pFiles;
	char         **pFiles0;
	char         **pFiles1;
	char         **pFiles2 = NULL;
	qboolean     bDrop     = qfalse;

	*listbuf = 0;

	pFiles0 = Sys_ListFiles(fs_homepath->string, NULL, NULL, &dummy, qtrue);
	pFiles1 = Sys_ListFiles(fs_basepath->string, NULL, NULL, &dummy, qtrue);

	// we searched for mods in the three paths
	// it is likely that we have duplicate names now, which we will cleanup below
	pFiles     = Sys_ConcatenateFileLists(pFiles0, pFiles1, pFiles2);
	nPotential = Sys_CountFileList(pFiles);

	for (i = 0 ; i < nPotential ; i++)
	{
		name = pFiles[i];
		// NOTE: cleaner would involve more changes
		// ignore duplicate mod directories
		if (i != 0)
		{
			bDrop = qfalse;
			for (j = 0; j < i; j++)
			{
				if (Q_stricmp(pFiles[j], name) == 0)
				{
					// this one can be dropped
					bDrop = qtrue;
					break;
				}
			}
		}
		if (bDrop)
		{
			continue;
		}

		// we drop "etmain" "." and ".."
		if (Q_stricmp(name, BASEGAME) && Q_stricmpn(name, ".", 1))
		{
			// now we need to find some .pk3 files to validate the mod
			// NOTE: (actually I'm not sure why .. what if it's a mod under developement with no .pk3?)
			// we didn't keep the information when we merged the directory names, as to what OS Path it was found under
			//   so it could be in base path, cd path or home path
			//   we will try each three of them here (yes, it's a bit messy)
			// NOTE: what about dropping the current loaded mod as well?
			path  = FS_BuildOSPath(fs_basepath->string, name, "");
			nPaks = 0;
			pPaks = Sys_ListFiles(path, ".pk3", NULL, &nPaks, qfalse);
			Sys_FreeFileList(pPaks);   // we only use Sys_ListFiles to check wether .pk3 files are present

			/* try on home path */
			if (nPaks <= 0)
			{
				path  = FS_BuildOSPath(fs_homepath->string, name, "");
				nPaks = 0;
				pPaks = Sys_ListFiles(path, ".pk3", NULL, &nPaks, qfalse);
				Sys_FreeFileList(pPaks);
			}

			if (nPaks > 0)
			{
				nLen = strlen(name) + 1;
				// nLen is the length of the mod path
				// we need to see if there is a description available
				descPath[0] = '\0';

				Com_sprintf(descPath, sizeof(descPath), "%s%cdescription.txt", name, PATH_SEP);

				nDescLen = FS_SV_FOpenFileRead(descPath, &descHandle);
				if (nDescLen > 0)
				{
					FILE *file;

					file = FS_FileForHandle(descHandle);
					Com_Memset(descPath, 0, sizeof(descPath));

					nDescLen = fread(descPath, 1, 48, file);
					if (nDescLen >= 0)
					{
						descPath[nDescLen] = '\0';
					}
					FS_FCloseFile(descHandle);
				}
				else
				{
					strcpy(descPath, name);
				}

				if (descHandle)
				{
					FS_FCloseFile(descHandle);
				}

				nDescLen = strlen(descPath) + 1;

				if (nTotal + nLen + 1 + nDescLen + 1 < bufsize)
				{
					strcpy(listbuf, name);
					listbuf += nLen;
					strcpy(listbuf, descPath);
					listbuf += nDescLen;
					nTotal  += nLen + nDescLen;
					nMods++;
				}
				else
				{
					break;
				}
			}
		}
	}
	Sys_FreeFileList(pFiles);

	return nMods;
}

//============================================================================

/**
 * @brief FS_Dir_f
 */
void FS_Dir_f(void)
{
	char *path;
	char *extension;
	char **dirnames;
	int  ndirs;
	int  i;

	if (Cmd_Argc() < 2 || Cmd_Argc() > 3)
	{
		Com_Printf("usage: dir <directory> [extension]\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		path      = Cmd_Argv(1);
		extension = "";
	}
	else
	{
		path      = Cmd_Argv(1);
		extension = Cmd_Argv(2);
	}

	Com_Printf("Directory of %s %s\n", path, extension);
	Com_Printf("---------------\n");

	dirnames = FS_ListFiles(path, extension, &ndirs);

	for (i = 0; i < ndirs; i++)
	{
		Com_Printf("%s\n", dirnames[i]);
	}
	FS_FreeFileList(dirnames);
}

/**
 * @brief FS_ConvertPath
 * @param[in,out] s
 */
void FS_ConvertPath(char *s)
{
	while (*s)
	{
		if (*s == '\\' || *s == ':')
		{
			*s = '/';
		}
		s++;
	}
}

/**
 * @brief Ignore case and separator char distinctions
 *
 * @param[in] s1
 * @param[in] s2
 *
 * @return
 *
 * @note FS_PathCmp doesn't deal with trailing slashes! See FS_IsSamePath
 * /aa/bb - /aa/bb/ returns not equal
 */
int FS_PathCmp(const char *s1, const char *s2)
{
	int c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (c1 >= 'a' && c1 <= 'z')
		{
			c1 -= ('a' - 'A');
		}
		if (c2 >= 'a' && c2 <= 'z')
		{
			c2 -= ('a' - 'A');
		}

		if (c1 == '\\' || c1 == ':')
		{
			c1 = '/';
		}
		if (c2 == '\\' || c2 == ':')
		{
			c2 = '/';
		}

		if (c1 < c2)
		{
			return -1;      // strings not equal
		}
		if (c1 > c2)
		{
			return 1;
		}
	}
	while (c1);

	return 0;       // strings are equal
}

/**
 * @brief Similar to FS_PathCmp() but real path name comparison.
 *        It deals with trailing slashes and relative paths.
 *
 * @warning Do not use this with quake game paths - only real filesystem paths!
 *
 * @param[in] s1 path A
 * @param[in] s2 path B
 */
qboolean FS_IsSamePath(const char *s1, const char *s2)
{
	char *res1, *res2;

	res1 = Sys_RealPath(s1);
	res2 = Sys_RealPath(s2);

	// Sys_RealPath() returns NULL if there are issues with the file
	// so the function returns true (only) if there are no errors and paths are equal
	if (res1 && res2 && !Q_stricmp(res1, res2))
	{
		Com_Dealloc(res1);
		Com_Dealloc(res2);
		return qtrue;
	}

	Com_Dealloc(res1);
	Com_Dealloc(res2);
	return qfalse;
}

/**
 * @brief FS_SortFileList
 * @param[in,out] filelist
 * @param[in] numfiles
 */
void FS_SortFileList(char **filelist, int numfiles)
{
	int  i, j, k, numsortedfiles;
	char **sortedlist;

	sortedlist     = Z_Malloc((numfiles + 1) * sizeof(*sortedlist));
	sortedlist[0]  = NULL;
	numsortedfiles = 0;
	for (i = 0; i < numfiles; i++)
	{
		for (j = 0; j < numsortedfiles; j++)
		{
			if (FS_PathCmp(filelist[i], sortedlist[j]) < 0)
			{
				break;
			}
		}
		for (k = numsortedfiles; k > j; k--)
		{
			sortedlist[k] = sortedlist[k - 1];
		}
		sortedlist[j] = filelist[i];
		numsortedfiles++;
	}
	Com_Memcpy(filelist, sortedlist, numfiles * sizeof(*filelist));
	Z_Free(sortedlist);
}

/**
 * @brief FS_NewDir_f
 */
void FS_NewDir_f(void)
{
	char *filter;
	char **dirnames;
	int  ndirs;
	int  i;

	if (Cmd_Argc() < 2)
	{
		Com_Printf("usage: fdir <filter>\n");
		Com_Printf("example: fdir *q3dm*.bsp\n");
		return;
	}

	filter = Cmd_Argv(1);

	Com_Printf("---------------\n");

	dirnames = FS_ListFilteredFiles("", "", filter, &ndirs, qfalse);

	FS_SortFileList(dirnames, ndirs);

	for (i = 0; i < ndirs; i++)
	{
		FS_ConvertPath(dirnames[i]);
		Com_Printf("%s\n", dirnames[i]);
	}
	Com_Printf("%d files listed\n", ndirs);
	FS_FreeFileList(dirnames);
}

/**
 * @brief FS_Path_f
 */
void FS_Path_f(void)
{
	searchpath_t *s;
	int          i;

	Com_Printf("Current working directory:\n");
	Com_Printf("    %s\n", Sys_Cwd());
	Com_Printf("Current search path:\n");
	for (s = fs_searchpaths; s; s = s->next)
	{
		if (s->pack)
		{
			//Com_Printf( "%s %X (%i files)\n", s->pack->pakFilename, s->pack->checksum, s->pack->numfiles );
			Com_Printf("    %s (%i files)\n", s->pack->pakFilename, s->pack->numfiles);
			if (fs_numServerPaks)
			{
				if (!FS_PakIsPure(s->pack))
				{
					Com_Printf("        not on the pure list\n");
				}
				else
				{
					Com_Printf("        on the pure list\n");
				}
			}
		}
		else
		{
			Com_Printf("    %s%c%s\n", s->dir->path, PATH_SEP, s->dir->gamedir);
		}
	}

	Com_Printf("\n");
	for (i = 1 ; i < MAX_FILE_HANDLES ; i++)
	{
		if (fsh[i].handleFiles.file.o)
		{
			Com_Printf("handle %i: %s\n", i, fsh[i].name);
		}
	}
}

/**
 * @brief Simulates the 'touch' unix command
 */
void FS_TouchFile_f(void)
{
	fileHandle_t f;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: touchFile <file>\n");
		return;
	}

	FS_FOpenFileRead(Cmd_Argv(1), &f, qfalse);
	if (f)
	{
		FS_FCloseFile(f);
	}
}

/**
 * @brief FS_Which
 * @param[in] fileName
 * @param[in] searchPath
 * @return
 */
qboolean FS_Which(const char *fileName, void *searchPath)
{
	searchpath_t *search = (searchpath_t *)searchPath;

	if (FS_FOpenFileReadDir(fileName, search, NULL, qfalse, qfalse) > 0)
	{
		if (search->pack)
		{
			Com_Printf("File \"%s\" found in \"%s\"\n", fileName, search->pack->pakFilename);
			return qtrue;
		}
		else if (search->dir)
		{
			Com_Printf("File \"%s\" found at \"%s\"\n", fileName, search->dir->fullpath);
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief FS_Which_f
 */
void FS_Which_f(void)
{
	searchpath_t *search;
	char         *fileName = Cmd_Argv(1);

	if (!fileName[0])
	{
		Com_Printf("Usage: which <file>\n");
		return;
	}

	// qpaths are not supposed to have a leading slash
	if (fileName[0] == '/' || fileName[0] == '\\')
	{
		fileName++;
	}

	// just wants to see if file is there
	for (search = fs_searchpaths; search; search = search->next)
	{
		if (FS_Which(fileName, search))
		{
			return;
		}
	}

	Com_Printf("File not found: \"%s\"\n", fileName);
}

//===========================================================================

/**
 * @brief FS_AddPackToPath add a single package to the search path
 * @param osPath full os path to the pk3 file
 * @param gameName game name this package will be registered for
 */
static void FS_AddPackToPath(const char *osPath, const char *gameName)
{
	pack_t       *pak;
	searchpath_t *search;

	if ((pak = FS_LoadZipFile(osPath, "")) == NULL)
	{
		return;
	}

	Q_strncpyz(pak->pakPathname, FS_Dirpath(osPath), sizeof(pak->pakPathname));
	Q_strncpyz(pak->pakGamename, FS_NormalizePath(gameName), sizeof(pak->pakGamename));

	fs_packFiles += pak->numfiles;

	search         = Z_Malloc(sizeof(searchpath_t));
	search->pack   = pak;
	search->next   = fs_searchpaths;
	fs_searchpaths = search;
}

/**
 * @brief paksort
 * @param[in] a
 * @param[in] b
 * @return
 */
static int QDECL paksort(const void *a, const void *b)
{
	char *aa, *bb;

	aa = *(char **)a;
	bb = *(char **)b;

	return FS_PathCmp(aa, bb);
}

/**
 * @brief Sets fs_gamedir, adds the directory to the head of the path,
 * then loads the zip headers
 * @param[in] path
 * @param[in] dir
 */
void FS_AddGameDirectory(const char *path, const char *dir)
{
	searchpath_t *sp;
	searchpath_t *search;
	pack_t       *pak;
	char         curpath[MAX_OSPATH + 1], *pakfile;
	int          numfiles;
	char         **pakfiles;
	int          pakfilesi;
	char         **pakfilestmp;
	int          numdirs;
	char         **pakdirs;
	int          pakdirsi;
	char         **pakdirstmp;
	int          pakwhich;
	int          len;

	// Unique
	for (sp = fs_searchpaths ; sp ; sp = sp->next)
	{
		if (sp->dir && !Q_stricmp(sp->dir->path, path) && !Q_stricmp(sp->dir->gamedir, dir))
		{
			return;         // we've already got this one
		}
	}

	Q_strncpyz(fs_gamedir, dir, sizeof(fs_gamedir));

	// find all pak files in this directory
	Q_strncpyz(curpath, FS_BuildOSPath(path, dir, ""), sizeof(curpath));
	curpath[strlen(curpath) - 1] = '\0';    // strip the trailing slash

	// Get .pk3 files
	pakfiles = Sys_ListFiles(curpath, ".pk3", NULL, &numfiles, qfalse);

	qsort(pakfiles, numfiles, sizeof(char *), paksort);

	if (fs_numServerPaks)
	{
		numdirs = 0;
		pakdirs = NULL;
	}
	else
	{
		// Get top level directories (we'll filter them later since the Sys_ListFiles filtering is terrible)
		pakdirs = Sys_ListFiles(curpath, "/", NULL, &numdirs, qfalse);

		qsort(pakdirs, numdirs, sizeof(char *), paksort);
	}

	pakfilesi = 0;
	pakdirsi  = 0;

	while ((pakfilesi < numfiles) || (pakdirsi < numdirs))
	{
		// Check if a pakfile or pakdir comes next
		if (pakfilesi >= numfiles)
		{
			// We've used all the pakfiles, it must be a pakdir.
			pakwhich = 0;
		}
		else if (pakdirsi >= numdirs)
		{
			// We've used all the pakdirs, it must be a pakfile.
			pakwhich = 1;
		}
		else
		{
			// Could be either, compare to see which name comes first
			// Need tmp variables for appropriate indirection for paksort()
			pakfilestmp = &pakfiles[pakfilesi];
			pakdirstmp  = &pakdirs[pakdirsi];
			pakwhich    = (paksort(pakfilestmp, pakdirstmp) < 0);
		}
		if (pakwhich)
		{
			// The next .pk3 file is before the next .pk3dir
			pakfile = FS_BuildOSPath(path, dir, pakfiles[pakfilesi]);
			if ((pak = FS_LoadZipFile(pakfile, pakfiles[pakfilesi])) == 0)
			{
				// This isn't a .pk3! Next!
				pakfilesi++;
				continue;
			}

			Q_strncpyz(pak->pakPathname, curpath, sizeof(pak->pakPathname));
			// store the game name for downloading
			Q_strncpyz(pak->pakGamename, FS_NormalizePath(dir), sizeof(pak->pakGamename));

			fs_packFiles += pak->numfiles;

			search         = Z_Malloc(sizeof(searchpath_t));
			search->pack   = pak;
			search->next   = fs_searchpaths;
			fs_searchpaths = search;

			pakfilesi++;
		}
		else
		{
			// The next .pk3dir is before the next .pk3 file
			// But wait, this could be any directory, we're filtering to only ending with ".pk3dir" here.
			len = strlen(pakdirs[pakdirsi]);
			if (!FS_IsExt(pakdirs[pakdirsi], ".pk3dir", len))
			{
				// This isn't a .pk3dir! Next!
				pakdirsi++;
				continue;
			}

			pakfile = FS_BuildOSPath(path, dir, pakdirs[pakdirsi]);

			// add the directory to the search path
			search      = Z_Malloc(sizeof(searchpath_t));
			search->dir = Z_Malloc(sizeof(*search->dir));

			Q_strncpyz(search->dir->path, curpath, sizeof(search->dir->path));  // c:\etlegacy\etmain
			Q_strncpyz(search->dir->fullpath, pakfile, sizeof(search->dir->fullpath));  // c:\etlegacy\etmain\mypak.pk3dir
			Q_strncpyz(search->dir->gamedir, pakdirs[pakdirsi], sizeof(search->dir->gamedir)); // mypak.pk3dir

			search->next   = fs_searchpaths;
			fs_searchpaths = search;

			pakdirsi++;
		}
	}

	// done
	Sys_FreeFileList(pakfiles);
	Sys_FreeFileList(pakdirs);

	//
	// add the directory to the search path
	//
	search      = Z_Malloc(sizeof(searchpath_t));
	search->dir = Z_Malloc(sizeof(*search->dir));

	Q_strncpyz(search->dir->path, path, sizeof(search->dir->path));
	Q_strncpyz(search->dir->fullpath, curpath, sizeof(search->dir->fullpath));
	Q_strncpyz(search->dir->gamedir, dir, sizeof(search->dir->gamedir));

	search->next   = fs_searchpaths;
	fs_searchpaths = search;
}

/**
 * @brief FS_idPak
 * @param[in] pak
 * @param[in] base
 * @return
 */
qboolean FS_idPak(const char *pak, const char *base)
{
	int i;

	for (i = 0; i < NUM_ID_PAKS; i++)
	{
		if (!FS_FilenameCompare(pak, va("%s/pak%d", base, i)))
		{
			break;
		}
	}
	if (i < NUM_ID_PAKS)
	{
		return qtrue;
	}
	return qfalse;
}

/**
 * @struct officialpak_s
 */
typedef struct
{
	char pakname[MAX_QPATH];
	qboolean ok;
} officialpak_t;

/**
 * @brief FS_VerifyOfficialPaks
 * @return
 */
qboolean FS_VerifyOfficialPaks(void)
{
	int           i, j;
	searchpath_t  *sp;
	int           numOfficialPaksOnServer = 0;
	int           numOfficialPaksLocal    = 0;
	officialpak_t officialpaks[64];

	if (!fs_numServerPaks)
	{
		return qtrue;
	}

	for (i = 0; i < fs_numServerPaks; i++)
	{
		if (FS_idPak(fs_serverPakNames[i], BASEGAME))
		{
			Q_strncpyz(officialpaks[numOfficialPaksOnServer].pakname, fs_serverPakNames[i], sizeof(officialpaks[i].pakname));
			officialpaks[numOfficialPaksOnServer].ok = qfalse;
			numOfficialPaksOnServer++;
		}
	}

	for (i = 0; i < fs_numServerPaks; i++)
	{
		for (sp = fs_searchpaths ; sp ; sp = sp->next)
		{
			if (sp->pack && sp->pack->checksum == fs_serverPaks[i])
			{
				char packPath[MAX_QPATH];

				Com_sprintf(packPath, sizeof(packPath), "%s/%s", sp->pack->pakGamename, sp->pack->pakBasename);

				if (FS_idPak(packPath, BASEGAME))
				{
					for (j = 0; j < numOfficialPaksOnServer; j++)
					{
						if (!Q_stricmp(packPath, officialpaks[j].pakname))
						{
							officialpaks[j].ok = qtrue;
							numOfficialPaksLocal++;
						}
					}
				}
				break;
			}
		}
	}

	if (numOfficialPaksOnServer != numOfficialPaksLocal)
	{
		for (i = 0; i < numOfficialPaksOnServer; i++)
		{
			if (officialpaks[i].ok != qtrue)
			{
				Com_Printf("ERROR: Missing/corrupt official pak file %s\n", officialpaks[i].pakname);
			}
		}

		// assumed we have a valid client installation and the server has installed
		// genuine packs twice (f.e. in fs_homepath AND fs_basepath)
		// in this case numOfficialPaksLocal is greater than numOfficialPaksOnServer ...
		if (numOfficialPaksOnServer < numOfficialPaksLocal)
		{
			Com_Error(ERR_DROP, "ERROR: Connection aborted - corrupt/duplicate official pak file server installation.");
		}

		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Check whether the string contains stuff like "../" to prevent directory traversal bugs
 * @param[in] checkdir
 * @return qtrue if it does.
 */
qboolean FS_CheckDirTraversal(const char *checkdir)
{
	if (strstr(checkdir, "../") || strstr(checkdir, "..\\"))
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief FS_InvalidGameDir
 * @param
 * @return qtrue if path is a reference to current directory or directory traversal or a sub-directory
 */
qboolean FS_InvalidGameDir(const char *gamedir)
{
	if (!strcmp(gamedir, ".") || !strcmp(gamedir, "..")
	    || strchr(gamedir, '/') || strchr(gamedir, '\\'))
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief FS_ComparePaks
 *
 * @details
 * dlstring == qtrue
 * Returns a list of pak files that we should download from the server. They all get stored
 * in the current gamedir and an FS_Restart will be fired up after we download them all.
 *
 * The string is the format:
 *
 * \@remotename\@localname [repeat]
 *
 * static int      fs_numServerReferencedPaks;
 * static int      fs_serverReferencedPaks[MAX_SEARCH_PATHS];
 * static char     *fs_serverReferencedPakNames[MAX_SEARCH_PATHS];
 *
 * ----------------
 * dlstring == qfalse
 *
 * we are not interested in a download string format, we want something human-readable
 * (this is used for diagnostics while connecting to a pure server)
 * ================
 *
 * @param[out] neededpaks
 * @param[in] len
 * @param[in] dlstring
 * @return
 */
qboolean FS_ComparePaks(char *neededpaks, size_t len, qboolean dlstring)
{
	qboolean     havepak;
	char         *origpos = neededpaks;
	int          i;

	if (!fs_numServerReferencedPaks)
	{
		return qfalse; // Server didn't send any pack information along
	}

	*neededpaks = 0;

	for (i = 0 ; i < fs_numServerReferencedPaks ; i++)
	{
		// Ok, see if we have this pak file
		havepak = qfalse;

		// never auto download any of the id paks
		if (FS_idPak(fs_serverReferencedPakNames[i], BASEGAME))
		{
			continue;
		}

		// Make sure the server cannot make us write to non-quake3 directories.
		if (FS_CheckDirTraversal(fs_serverReferencedPakNames[i]))
		{
			Com_Printf("WARNING: Invalid download name %s\n", fs_serverReferencedPakNames[i]);
			continue;
		}

		havepak = FS_HasPack(fs_serverReferencedPaks[i]);

		if (!havepak && fs_serverReferencedPakNames[i] && *fs_serverReferencedPakNames[i])
		{
			// Don't got it
			if (dlstring)
			{
				// We need this to make sure we won't hit the end of the buffer or the server could
				// overwrite non-pk3 files on clients by writing so much crap into neededpaks that
				// Q_strcat cuts off the .pk3 extension.

				origpos += strlen(origpos);

				// Remote name
				Q_strcat(neededpaks, len, "@");
				Q_strcat(neededpaks, len, fs_serverReferencedPakNames[i]);
				Q_strcat(neededpaks, len, ".pk3");

				// Local name
				Q_strcat(neededpaks, len, "@");
				// Do we have one with the same name?
				if (FS_SV_FileExists(va("%s.pk3", fs_serverReferencedPakNames[i]), qfalse))
				{
					char st[MAX_ZPATH];
					// We already have one called this, we need to download it to another name
					// Make something up with the checksum in it
					Com_sprintf(st, sizeof(st), "%s.%08x.pk3", fs_serverReferencedPakNames[i], fs_serverReferencedPaks[i]);
					Q_strcat(neededpaks, len, st);
				}
				else
				{
					Q_strcat(neededpaks, len, fs_serverReferencedPakNames[i]);
					Q_strcat(neededpaks, len, ".pk3");
				}

				// Find out whether it might have overflowed the buffer and don't add this file to the
				// list if that is the case.
				if (strlen(origpos) + (origpos - neededpaks) >= len - 1)
				{
					*origpos = '\0';
					break;
				}
			}
			else
			{
				Q_strcat(neededpaks, len, fs_serverReferencedPakNames[i]);
				Q_strcat(neededpaks, len, ".pk3");
				// Do we have one with the same name?
				if (FS_SV_FileExists(va("%s.pk3", fs_serverReferencedPakNames[i]), qfalse))
				{
					Q_strcat(neededpaks, len, " (local file exists with wrong checksum)");
#ifndef DEDICATED
					// let the client subsystem track bad download redirects (dl file with wrong checksums)
					// this is a bit ugly but the only other solution would have been callback passing..
					if (Com_WWWBadChecksum(va("%s.pk3", fs_serverReferencedPakNames[i])))
					{
						// remove a potentially malicious download file
						// (this is also intended to avoid expansion of the pk3 into a file with different checksum .. messes up wwwdl chkfail)
						char *rmv = FS_BuildOSPath(fs_homepath->string, va("%s.pk3", fs_serverReferencedPakNames[i]), "");

						rmv[strlen(rmv) - 1] = '\0';
						// no need to check removal - this is a pk3
						FS_Remove(rmv);
					}
#endif
				}
				Q_strcat(neededpaks, len, "\n");
			}
		}
	}

	if (*neededpaks)
	{
		Com_Printf("Need paks: %s\n", neededpaks);
		return qtrue;
	}

	return qfalse; // We have them all
}

/**
 * @brief Frees all resources and closes all files
 * @param closemfp - unused
 */
void FS_Shutdown(qboolean closemfp)
{
	searchpath_t *p, *next;
	int          i;

	for (i = 0; i < MAX_FILE_HANDLES; i++)
	{
		if (fsh[i].fileSize)
		{
			FS_FCloseFile(i);
		}
	}

	// free everything
	for (p = fs_searchpaths ; p ; p = next)
	{
		next = p->next;

		if (p->pack)
		{
			FS_FreePak(p->pack);
		}
		if (p->dir)
		{
			Z_Free(p->dir);
		}
		Z_Free(p);
	}

	// any FS_ calls will now be an error until reinitialized
	fs_searchpaths = NULL;
	fs_checksumFeed = 0;

	Cmd_RemoveCommand("path");
	Cmd_RemoveCommand("dir");
	Cmd_RemoveCommand("fdir");
	Cmd_RemoveCommand("touchFile");
	Cmd_RemoveCommand("which");

#ifdef FS_MISSING
	if (closemfp)
	{
		fclose(missingFiles);
	}
#endif
}

/**
 * @brief FS_ReorderPurePaks
 *
 * @note The reordering that happens here is not reflected in the cvars (\\cvarlist *pak*)
 * this can lead to misleading situations
 */
static void FS_ReorderPurePaks(void)
{
	searchpath_t *s;
	int          i;
	searchpath_t **p_insert_index, // for linked list reordering
	             **p_previous; // when doing the scan

	fs_reordered = qfalse;

	// only relevant when connected to pure server
	if (!fs_numServerPaks)
	{
		return;
	}

	p_insert_index = &fs_searchpaths; // we insert in order at the beginning of the list
	for (i = 0 ; i < fs_numServerPaks ; i++)
	{
		p_previous = p_insert_index; // track the pointer-to-current-item
		for (s = *p_insert_index; s; s = s->next)     // the part of the list before p_insert_index has been sorted already
		{
			if (s->pack && fs_serverPaks[i] == s->pack->checksum)
			{
				fs_reordered = qtrue;
				// move this element to the insert list
				*p_previous     = s->next;
				s->next         = *p_insert_index;
				*p_insert_index = s;
				// increment insert list
				p_insert_index = &s->next;
				break; // iterate to next server pack
			}
			p_previous = &s->next;
		}
	}
}

/**
 * @brief FS_AddBothGameDirectories
 *
 * @param[in] subpath
 */
static void FS_AddBothGameDirectories(const char *subpath)
{
	if (fs_basepath->string[0])
	{
		// fs_homepath is used for all systems
		// NOTE: same filtering below for mods and basegame
		FS_AddGameDirectory(fs_basepath->string, subpath);

		if (fs_homepath->string[0] && !FS_IsSamePath(fs_homepath->string, fs_basepath->string))
		{
			FS_AddGameDirectory(fs_homepath->string, subpath);
#if defined(FEATURE_PAKISOLATION) && !defined(DEDICATED)
			/* only mount containers for certain directories and if in pure mode */
			if (fs_numServerPaks && (!Q_stricmp(subpath, BASEGAME) || (!Q_stricmp(subpath, DEFAULT_MODGAME) && fs_containerMount->integer)))
			{
				char contPath[MAX_OSPATH];
				Com_sprintf(contPath, sizeof(contPath), "%s%c%s", subpath, PATH_SEP, FS_CONTAINER);
				FS_AddGameDirectory(fs_homepath->string, contPath);
				Q_strncpyz(fs_gamedir, subpath, sizeof(fs_gamedir));
			}
			// We are in a non pure server, so just try to mount the minimal required packs
			else if(fs_numServerReferencedPaks && (!Q_stricmp(subpath, BASEGAME) || (!Q_stricmp(subpath, DEFAULT_MODGAME) && fs_containerMount->integer)))
			{
				int i = 0;
				for (i = 0 ; i < fs_numServerReferencedPaks ; i++)
				{
					// Do we have the pack already
					if (FS_HasPack(fs_serverReferencedPaks[i]))
					{
						continue;
					}

					if (!fs_serverReferencedPakNames[i] || !*fs_serverReferencedPakNames[i])
					{
						Com_DPrintf("Missing package name for checksum: %i\n", fs_serverReferencedPaks[i]);
						continue;
					}

					char *packName = strchr(fs_serverReferencedPakNames[i], '/');
					if (packName && (packName += 1))
					{
						char tmpPath[MAX_OSPATH] = { '\0' };
						char *path = FS_BuildOSPath(fs_homepath->string, subpath, va("%s%c%s.pk3", FS_CONTAINER, PATH_SEP, packName));
						Q_strcpy(tmpPath, path);

						if (FS_FileInPathExists(tmpPath) && FS_CompareZipChecksum(tmpPath, fs_serverReferencedPaks[i]))
						{
							Com_DPrintf("Found referenced pack in container: %s\n", fs_serverReferencedPakNames[i]);
							FS_AddPackToPath(tmpPath, BASEGAME);
						}
					}
				}
			}
#endif
		}
	}
}

/**
 * @brief FS_Startup
 * @param[in] gameName
 */
static void FS_Startup(const char *gameName)
{
	const char *homePath;

	Com_Printf("----- Initializing Filesystem --\n");

	fs_packFiles = 0;

	fs_debug    = Cvar_Get("fs_debug", "0", 0);
	fs_basepath = Cvar_Get("fs_basepath", Sys_DefaultInstallPath(), CVAR_INIT | CVAR_PROTECTED);
	fs_basegame = Cvar_Get("fs_basegame", "", CVAR_INIT | CVAR_PROTECTED);

	homePath = Sys_DefaultHomePath();

	if (!homePath || !homePath[0])
	{
#ifdef DEDICATED
		homePath = fs_basepath->string;
#else
		Com_Error(ERR_FATAL, "FS_Startup: Default home path is empty.");
#endif
	}

	fs_homepath = Cvar_Get("fs_homepath", homePath, CVAR_INIT);

	fs_gamedirvar = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);

#if defined(FEATURE_PAKISOLATION) && !defined(DEDICATED)
	fs_containerMount = Cvar_Get("fs_containerMount", "0", CVAR_INIT);
#endif

	if (FS_InvalidGameDir(gameName))
	{
		Com_Error(ERR_DROP, "Invalid com_basegame '%s'", gameName);
	}
	if (FS_InvalidGameDir(fs_basegame->string))
	{
		Com_Error(ERR_DROP, "Invalid fs_basegame '%s'", fs_basegame->string);
	}
	if (FS_InvalidGameDir(fs_gamedirvar->string))
	{
		Com_Error(ERR_DROP, "Invalid fs_game '%s'", fs_gamedirvar->string);
	}

	// add search path elements in reverse priority order
	FS_AddBothGameDirectories(gameName);

	// check for additional base game so mods can be based upon other mods
	if (fs_basegame->string[0] && !Q_stricmp(gameName, BASEGAME) && Q_stricmp(fs_basegame->string, gameName))
	{
		FS_AddBothGameDirectories(fs_basegame->string);
	}

	// check for additional game folder for mods
	if (fs_gamedirvar->string[0] && !Q_stricmp(gameName, BASEGAME) && Q_stricmp(fs_gamedirvar->string, gameName))
	{
		FS_AddBothGameDirectories(fs_gamedirvar->string);
	}

	// add our commands
	Cmd_AddCommand("path", FS_Path_f, "Prints current search path including files.");
	Cmd_AddCommand("dir", FS_Dir_f, "Prints a given directory.");
	Cmd_AddCommand("fdir", FS_NewDir_f, "Prints a filtered directory.");
	Cmd_AddCommand("touchFile", FS_TouchFile_f, "Simulates the 'touch' unix command.");
	Cmd_AddCommand("which", FS_Which_f, "Searches for a given file.");

	// reorder the pure pk3 files according to server order
	FS_ReorderPurePaks();

	// print the current search paths
	FS_Path_f();

	fs_gamedirvar->modified = qfalse; // We just loaded, it's not modified

#ifdef FS_MISSING
	if (missingFiles == NULL)
	{
		missingFiles = Sys_FOpen("\\missing.txt", "ab");
	}
#endif
	Com_Printf("%d files in pk3 files\n", fs_packFiles);

#ifndef DEDICATED
	// clients: don't start if base == home, so downloads won't overwrite original files! DO NOT CHANGE!
	if (FS_IsSamePath(fs_homepath->string, fs_basepath->string))
	{
		Com_Error(ERR_FATAL, "FS_Startup: fs_homepath and fs_basepath are equal - set different paths!");
	}

#ifdef FEATURE_GETTEXT
	// only translate default mod
	// - other mods don't support our unicode translation files
	// - mods have own strings to translate - we avoid language mixes
	if (!Q_stricmp(fs_gamedirvar->string, DEFAULT_MODGAME))
	{
		doTranslateMod = qtrue;
	}
	else
	{
		doTranslateMod = qfalse;
	}
#endif // FEATURE_GETTEXT

#endif // ifndef DEDICATED

	Com_Printf("--------------------------------\n");
}

/**
 * @brief FS_LoadedPakChecksums
 * @return A space separated string containing the checksums of all loaded pk3 files.
 * Servers with sv_pure set will get this string and pass it to clients.
 */
const char *FS_LoadedPakChecksums(void)
{
	static char  info[BIG_INFO_STRING];
	searchpath_t *search;
	size_t       len;

	info[0] = 0;

	for (search = fs_searchpaths ; search ; search = search->next)
	{
		// is the element a pak file?
		if (!search->pack)
		{
			continue;
		}

		Q_strcat(info, sizeof(info), va("%i ", search->pack->checksum));
	}

	// remove last space char
	len = strlen(info);
	if (len > 1) // foolproof
	{
		info[len - 1] = 0;
	}

	return info;
}

/**
 * @brief FS_LoadedPakNames
 * @return A space separated string containing the names of all loaded pk3 files.
 * Servers with sv_pure set will get this string and pass it to clients.
 */
const char *FS_LoadedPakNames(void)
{
	static char  info[BIG_INFO_STRING];
	searchpath_t *search;

	info[0] = 0;

	for (search = fs_searchpaths ; search ; search = search->next)
	{
		// is the element a pak file?
		if (!search->pack)
		{
			continue;
		}

		if (*info)
		{
			Q_strcat(info, sizeof(info), " ");
		}

		// full path ...
		Q_strcat(info, sizeof(info), search->pack->pakGamename);
		Q_strcat(info, sizeof(info), "/");
		Q_strcat(info, sizeof(info), search->pack->pakBasename);
	}

	return info;
}

/**
 * @brief FS_LoadedPakPureChecksums
 * @return A space separated string containing the pure checksums of all loaded pk3 files.
 * Servers with sv_pure use these checksums to compare with the checksums the clients send
 * back to the server.
 */
const char *FS_LoadedPakPureChecksums(void)
{
	static char  info[BIG_INFO_STRING];
	searchpath_t *search;
	size_t       len;

	info[0] = 0;

	for (search = fs_searchpaths ; search ; search = search->next)
	{
		// is the element a pak file?
		if (!search->pack)
		{
			continue;
		}

		Q_strcat(info, sizeof(info), va("%i ", search->pack->pure_checksum));
	}

	// remove last space char
	len = strlen(info);
	if (len > 1) // foolproof
	{
		info[len - 1] = 0;
	}

	// only comment out when you need a new pure checksums string
	//Com_DPrintf("FS_LoadPakPureChecksums: %s\n", info);

	return info;
}

/**
 * @brief FS_ReferencedPakChecksums
 * @return A space separated string containing the checksums of all referenced pk3 files.
 * The server will send this to the clients so they can check which files should be auto-downloaded.
 */
const char *FS_ReferencedPakChecksums(void)
{
	static char  info[BIG_INFO_STRING];
	searchpath_t *search;
	size_t       len;

	info[0] = 0;

	for (search = fs_searchpaths ; search ; search = search->next)
	{
		// is the element a pak file?
		if (search->pack)
		{
			if (search->pack->referenced || Q_stricmpn(search->pack->pakGamename, BASEGAME, strlen(BASEGAME)))
			{
				Q_strcat(info, sizeof(info), va("%i ", search->pack->checksum));
			}
		}
	}

	// remove last space char
	len = strlen(info);
	if (len > 1) // foolproof
	{
		info[len - 1] = 0;
	}

	return info;
}

/**
 * @brief FS_ReferencedPakNames
 * @return A space separated string containing the names of all referenced pk3 files.
 * The server will send this to the clients so they can check which files should be auto-downloaded.
 */
const char *FS_ReferencedPakNames(void)
{
	static char  info[BIG_INFO_STRING];
	searchpath_t *search;
	size_t       len;

	info[0] = 0;

	// we want to return ALL pk3's from the fs_game path
	// and referenced one's from etmain
	for (search = fs_searchpaths ; search ; search = search->next)
	{
		// is the element a pak file?
		if (search->pack)
		{
			if (search->pack->referenced || Q_stricmpn(search->pack->pakGamename, BASEGAME, strlen(BASEGAME)))
			{
				Q_strcat(info, sizeof(info), search->pack->pakGamename);
				Q_strcat(info, sizeof(info), "/");
				Q_strcat(info, sizeof(info), search->pack->pakBasename);
				Q_strcat(info, sizeof(info), " ");
			}
		}
	}

	// remove last space char
	len = strlen(info);
	if (len > 1) // foolproof
	{
		info[len - 1] = 0;
	}

	return info;
}

/**
 * @brief This function is only used by the client to build the string sent back to server,
 * it's useless in dedicated
 *
 *
 * @return A space separated string containing the pure checksums of all referenced pk3 files.
 * Servers with sv_pure set will get this string back from clients for pure validation
 *
 * @note The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."
 */
const char *FS_ReferencedPakPureChecksums(void)
{
	static char  info[BIG_INFO_STRING];
	searchpath_t *search;
	int          nFlags, numPaks = 0, checksum = fs_checksumFeed;
	size_t       len;

	info[0] = 0;

	for (nFlags = FS_CGAME_REF; nFlags; nFlags = nFlags >> 1)
	{
		if (nFlags & FS_GENERAL_REF)
		{
			// add a delimter between must haves and general refs
			//Q_strcat(info, sizeof(info), "@ ");
			info[strlen(info) + 1] = '\0';
			info[strlen(info) + 2] = '\0';
			info[strlen(info)]     = '@';
			info[strlen(info)]     = ' ';
		}
		for (search = fs_searchpaths ; search ; search = search->next)
		{
			// is the element a pak file and has it been referenced based on flag?
			if (search->pack && (search->pack->referenced & nFlags))
			{
				Q_strcat(info, sizeof(info), va("%i ", search->pack->pure_checksum));
				if (nFlags & (FS_CGAME_REF | FS_UI_REF))
				{
					break;
				}
				checksum ^= search->pack->pure_checksum;
				numPaks++;
			}
		}
	}
	// last checksum is the encoded number of referenced pk3s
	checksum ^= numPaks;
	Q_strcat(info, sizeof(info), va("%i ", checksum));

	// remove last space char
	len = strlen(info);
	if (len > 1) // foolproof
	{
		info[len - 1] = 0;
	}

	return info;
}

/**
 * @brief FS_ClearPakReferences
 * @param[in] flags
 */
void FS_ClearPakReferences(int flags)
{
	searchpath_t *search;

	if (!flags)
	{
		flags = -1;
	}
	for (search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file and has it been referenced?
		if (search->pack)
		{
			search->pack->referenced &= ~flags;
		}
	}
}

/**
 * @brief FS_ClearPureServerPacks
 */
void FS_ClearPureServerPacks(void)
{
	// Remove pure paks
	/* Old Code
	FS_PureServerSetLoadedPaks("", "");
	FS_PureServerSetReferencedPaks("", "");
	*/
	// This does the same thing as the above,
	// but does not cause the tokenizing of strings..
	fs_numServerPaks           = 0;
	fs_numServerReferencedPaks = 0;
	fs_checksumFeed            = 0;

	if (fs_reordered)
	{
		// force a restart to make sure the search order will be correct
		Com_DPrintf("FS search reorder is required\n");
		FS_Restart(fs_checksumFeed);
	}
}

/**
 * @brief If the string is empty, all data sources will be allowed.
 * If not empty, only pk3 files that match one of the space separated checksums
 * will be checked for files, with the exception of .cfg and .dat files.
 *
 * @param[in] pakSums
 * @param[in] pakNames
 */
void FS_PureServerSetLoadedPaks(const char *pakSums, const char *pakNames)
{
	int i, c, d;

	Cmd_TokenizeString(pakSums);

	c = Cmd_Argc();
	if (c > MAX_SEARCH_PATHS)
	{
		c = MAX_SEARCH_PATHS;
	}

	fs_numServerPaks = c;

	for (i = 0 ; i < c ; i++)
	{
		fs_serverPaks[i] = Q_atoi(Cmd_Argv(i));
	}

	if (fs_numServerPaks)
	{
		Com_Printf("Connected to a pure server.\n");
	}
	else
	{
#if defined(FEATURE_PAKISOLATION) && !defined(DEDICATED)
		// reset the value on each disconnect, before fs restarts
		Cvar_Set("fs_containerMount", "0");
#endif
		if (fs_reordered)
		{
			// force a restart to make sure the search order will be correct
			Com_DPrintf("FS search reorder is required\n");
			FS_Restart(fs_checksumFeed);
			return;
		}
	}

	for (i = 0 ; i < c ; i++)
	{
		if (fs_serverPakNames[i])
		{
			Z_Free(fs_serverPakNames[i]);
		}
		fs_serverPakNames[i] = NULL;
	}

	if (pakNames && *pakNames)
	{
		Cmd_TokenizeString(pakNames);

		d = Cmd_Argc();
		if (d > MAX_SEARCH_PATHS)
		{
			d = MAX_SEARCH_PATHS;
		}

		for (i = 0 ; i < d ; i++)
		{
			fs_serverPakNames[i] = CopyString(Cmd_Argv(i));
		}
	}
}

/**
 * @brief The checksums and names of the pk3 files referenced at the server are sent to
 * the client and stored here. The client will use these checksums to see if any
 * pk3 files need to be auto-downloaded.
 *
 * @param[in] pakSums
 * @param[in] pakNames
 */
void FS_PureServerSetReferencedPaks(const char *pakSums, const char *pakNames)
{
	int i, c, d = 0;

	Cmd_TokenizeString(pakSums);

	c = Cmd_Argc();
	if (c > MAX_SEARCH_PATHS)
	{
		c = MAX_SEARCH_PATHS;
	}

	fs_numServerReferencedPaks = c;

	for (i = 0 ; i < c ; i++)
	{
		fs_serverReferencedPaks[i] = Q_atoi(Cmd_Argv(i));
	}

	for (i = 0 ; i < ARRAY_LEN(fs_serverReferencedPakNames); i++)
	{
		if (fs_serverReferencedPakNames[i])
		{
			Z_Free(fs_serverReferencedPakNames[i]);
		}

		fs_serverReferencedPakNames[i] = NULL;
	}

	if (pakNames && *pakNames)
	{
		Cmd_TokenizeString(pakNames);

		d = Cmd_Argc();
		if (d > MAX_SEARCH_PATHS)
		{
			d = MAX_SEARCH_PATHS;
		}

		for (i = 0 ; i < d ; i++)
		{
			fs_serverReferencedPakNames[i] = CopyString(Cmd_Argv(i));
		}
	}
}

/**
 * @brief Prints file system info
 */
void FS_Fileinfo_f(void)
{
	Com_Printf("fs_gamedir %s\n", fs_gamedir);
	Com_Printf("fs_debug %s\n", fs_debug->string);

	Com_Printf("fs_homepath %s\n", fs_homepath->string);
	Com_Printf("fs_basepath %s\n", fs_basepath->string);
	Com_Printf("fs_basegame %s\n", fs_basegame->string);
	Com_Printf("fs_gamedirvar %s\n", fs_gamedirvar->string);

	Com_Printf("Total megs read %i\n", fs_readCount / (1024 * 1024)); // total at runtime, isn't reset
	Com_Printf("Total files read %i\n", fs_loadCount);                // total at runtime, isn't reset
	Com_Printf("Total files in memory %i\n", fs_loadStack);           // should be 0 in game
	Com_Printf("Total number of files in packs %i\n", fs_packFiles);
}

modHash modHashes;

/**
 * @brief FS_CalcModHashes
 */
void FS_CalcModHashes(void)
{
	char *tmp = Cvar_VariableString("fs_game");

	modHashes.defaultMod = Com_BlockChecksum(DEFAULT_MODGAME, strlen(DEFAULT_MODGAME));
	modHashes.currentMod = Com_BlockChecksum(tmp, strlen(tmp));
}

/**
 * @brief If we can't find pak0.pk3, assume that the paths are busted
 * and error out now, rather than getting an unreadable graphics screen.
 *
 * pak0.pk3 is the main requirement for a successful initialization,
 * additional missing paks can be checked and redownloaded after start up.
 *
 * @param[in] checksumFeed
 */
static void FS_CheckRequiredFiles(int checksumFeed)
{
	if (!FS_FOpenFileRead("pak0.pk3", NULL, qfalse))
	{
		// this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		// i.e. standalone game server
		if (checksumFeed && lastValidBase[0])
		{
			FS_PureServerSetLoadedPaks("", "");
			Cvar_Set("fs_basepath", lastValidBase);
			Cvar_Set("fs_game", lastValidGame);
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			FS_Restart(checksumFeed);
			Com_Error(ERR_DROP, "Invalid game folder");
		}

		Com_Error(ERR_FATAL, "FS_InitFilesystem: Original game data files not found.\n\nPlease copy pak0.pk3 from the 'etmain' path of your Wolfenstein: Enemy Territory installation to:\n\n\"%s%c%s\"\n\n",
		          Cvar_VariableString("fs_basepath"), PATH_SEP, BASEGAME);
	}
}

/**
 * @brief Called only at initial startup, not when the filesystem
 * is resetting due to a game change
 */
void FS_InitFilesystem(void)
{
	cvar_t *tmp_fs_game;

	// allow command line params to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	Com_StartupVariable("fs_basepath");
	Com_StartupVariable("fs_homepath");
	Com_StartupVariable("fs_game");

	// ET: Legacy start
	// if fs_game is not specified, set 'DEFAULT_MODGAME' mod as default fs_game
	// this 'optimization' grants us 2.60b compatibility w/o deeper changes and users
	// don't have to set fs_game param to run latest mod code
	tmp_fs_game = Cvar_Get("fs_game", "", 0);
	if (!strcmp(tmp_fs_game->string, ""))
	{
		Cvar_Set("fs_game", DEFAULT_MODGAME);
		tmp_fs_game         = Cvar_Get("fs_game", "", 0);
		tmp_fs_game->flags |= CVAR_USER_CREATED; // deal as startup var

		Com_Printf("INFO: fs_game now defaults to '%s' mod instead of 'etmain'\n", tmp_fs_game->string);
	}

	FS_CalcModHashes();

	// try to start up normally
	FS_Startup(BASEGAME);

	FS_CheckRequiredFiles(0);

	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));

	if (fs_debug->integer)
	{
		Cmd_AddCommand("fileinfo", FS_Fileinfo_f, "Prints file system info.");
	}
}

/**
 * @brief FS_Restart
 * @param[in] checksumFeed
 */
void FS_Restart(int checksumFeed)
{
	// free anything we currently have loaded
	FS_Shutdown(qfalse);

	// set the checksum feed
	fs_checksumFeed = checksumFeed;

	// clear pak references
	FS_ClearPakReferences(0);

	// try to start up normally
	FS_Startup(BASEGAME);

	FS_CheckRequiredFiles(checksumFeed);

	FS_CalcModHashes();

	// new check before safeMode
	if (Q_stricmp(fs_gamedirvar->string, lastValidGame))
	{
		// skip the etconfig.cfg if "safe" is on the command line
		if (!Com_SafeMode())
		{
			char *cl_profileStr = Cvar_VariableString("cl_profile");

			if (cl_profileStr[0])
			{
				// check existing pid file and make sure it's ok
				if (!Com_CheckProfile())
				{
#ifndef ETLEGACY_DEBUG
					Com_Printf(S_COLOR_YELLOW "WARNING: profile.pid found for profile '%s' - system settings will revert to defaults\n", cl_profileStr);
					// set crashed state
					Cbuf_AddText("set com_crashed 1\n");
#endif
				}

				// write a new one
				if (!Sys_WritePIDFile())
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: couldn't write %s\n", com_pidfile->string);
				}

				// exec the config
				Cbuf_AddText(va("exec profiles/%s/%s\n", cl_profileStr, CONFIG_NAME));
			}
			else
			{
				Cbuf_AddText(va("exec %s\n", CONFIG_NAME));
			}
		}
	}

	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));
}

/**
 * @brief Restart if necessary.
 * @param[in] checksumFeed
 * @return
 * @todo This doesn't catch all cases where an FS_Restart is necessary
 */
qboolean FS_ConditionalRestart(int checksumFeed)
{
	if (fs_gamedirvar->modified || checksumFeed != fs_checksumFeed)
	{
		FS_Restart(checksumFeed);

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Handle based file calls for virtual machines
 * @param[in] qpath
 * @param[in,out] f
 * @param[in] mode
 * @return
 */
int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	int      r;
	qboolean sync = qfalse;

	switch (mode)
	{
	case FS_READ: // returns 0 or -1 for invalid files
		r = FS_FOpenFileRead(qpath, f, qtrue);
		break;
	case FS_WRITE:
		*f = FS_FOpenFileWrite(qpath);
		r  = 0;
		if (*f == 0)
		{
			r = -1;
		}
		break;
	case FS_APPEND_SYNC:
		sync = qtrue;
	// fall through
	case FS_APPEND:
		*f = FS_FOpenFileAppend(qpath);
		r  = 0;
		if (*f == 0)
		{
			r = -1;
		}
		break;
	default:
		Com_Error(ERR_FATAL, "FS_FOpenFileByMode: bad mode [%i] of file '%s'", mode, qpath);
		return -1;
	}

	if (!f)
	{
		return r;
	}

	if (*f)
	{
		fsh[*f].fileSize = r;
	}
	fsh[*f].handleSync = sync;

	return r;
}

/**
 * @brief FS_FTell
 * @param[in] f
 * @return
 */
int FS_FTell(fileHandle_t f)
{
	int pos;

	if (fsh[f].zipFile == qtrue)
	{
		pos = unztell(fsh[f].handleFiles.file.z);

		if (pos == UNZ_PARAMERROR)
		{
			Com_Error(ERR_FATAL, "FS_FTell: Cannot get current position in zip\n");
		}
	}
	else
	{
		pos = ftell(fsh[f].handleFiles.file.o);

		if (pos == -1)
		{
			Com_Error(ERR_FATAL, "FS_FTell: Cannot get current position in file stream\n");
		}
	}

	return pos;
}

/**
 * @brief FS_Flush
 * @param[in] f
 */
void FS_Flush(fileHandle_t f)
{
	fflush(fsh[f].handleFiles.file.o);
}

/**
 * @brief FS_FilenameCompletion
 * @param[in] dir
 * @param[in] numext
 * @param[in] ext
 * @param[in] stripExt
 * @param callback
 * @param[in] allowNonPureFilesOnDisk
 */
void FS_FilenameCompletion(const char *dir, int numext, const char **ext,
                           qboolean stripExt, void (*callback)(const char *s), qboolean allowNonPureFilesOnDisk)
{
	char **filenames;
	int  nfiles;
	int  i, j;
	char fileName[MAX_STRING_CHARS];

	for (j = 0; j < numext; j++)
	{
		filenames = FS_ListFilteredFiles(dir, ext[j], NULL, &nfiles, allowNonPureFilesOnDisk);

		FS_SortFileList(filenames, nfiles);

		for (i = 0; i < nfiles; i++)
		{
			FS_ConvertPath(filenames[i]);
			Q_strncpyz(fileName, filenames[i], MAX_STRING_CHARS);

			if (stripExt)
			{
				COM_StripExtension(fileName, fileName, sizeof(fileName));
			}

			callback(fileName);
		}
		FS_FreeFileList(filenames);
	}
}

/**
 * @brief Compared requested pak against the names as we built them in FS_ReferencedPakNames
 * @param pak
 * @return
 *
 * @note CVE-2006-2082
 */
qboolean FS_VerifyPak(const char *pak)
{
	char         teststring[BIG_INFO_STRING];
	searchpath_t *search;

	for (search = fs_searchpaths ; search ; search = search->next)
	{
		if (search->pack)
		{
			Q_strncpyz(teststring, search->pack->pakGamename, sizeof(teststring));
			Q_strcat(teststring, sizeof(teststring), "/");
			Q_strcat(teststring, sizeof(teststring), search->pack->pakBasename);
			Q_strcat(teststring, sizeof(teststring), ".pk3");
			if (!Q_stricmp(teststring, pak))
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/**
 * @brief Extracts zipped file into the selected path
 * @param[in] fileName to extract
 * @param[in] outpath the path where to write the extracted files
 * @param[in] quiet whether to inform if unzipping fails
 * @return qtrue    if successfully extracted
 * @return qfalse   if extraction failed
 */
qboolean FS_UnzipTo(const char *fileName, const char *outpath, qboolean quiet)
{
	char            zipPath[MAX_OSPATH];
	unzFile         zipFile;
	unz_global_info zipInfo;
	int             err, i;
	void            *buf      = NULL;
	qboolean        isUnZipOK = qtrue;

	Com_sprintf(zipPath, sizeof(zipPath), "%s", fileName);
	zipFile = FS_UnzOpen(zipPath);

	if (!zipFile)
	{
		if (!quiet || fs_debug->integer)
		{
			Com_Printf(S_COLOR_RED "FS_UnzipTo ERROR: not a zip file (%s).\n", zipPath);
		}
		return qfalse;
	}

	err = unzGetGlobalInfo(zipFile, &zipInfo);

	if (err != UNZ_OK)
	{
		if (!quiet || fs_debug->integer)
		{
			Com_Printf(S_COLOR_RED "FS_UnzipTo: unable to unzip file (%s). ERROR %i\n", zipPath, err);
		}
		(void) unzClose(zipFile);
		return qfalse;
	}

	err = unzGoToFirstFile(zipFile);

	if (err != UNZ_OK)
	{
		if (!quiet || fs_debug->integer)
		{
			Com_Printf(S_COLOR_RED "FS_UnzipTo: unable to read first element of file (%s). ERROR %i\n", zipPath, err);
		}
		(void) unzClose(zipFile);
		return qfalse;
	}

	for (i = 0; i < zipInfo.number_entry; i++)
	{
		unz_file_info file_info;
		char          newFileName[MAX_OSPATH];
		char          newFilePath[MAX_OSPATH];
		FILE          *newFile;

		err = unzGetCurrentFileInfo(zipFile, &file_info, newFileName, sizeof(newFileName), NULL, 0, NULL, 0);

		if (err != UNZ_OK)
		{
			if (!quiet || fs_debug->integer)
			{
				Com_Printf(S_COLOR_RED "FS_UnzipTo: unable to read zip info element [%i] of file (%s). ERROR %i\n", i, zipPath, err);
			}
			(void) unzClose(zipFile);
			return qfalse;
		}

		Com_sprintf(newFilePath, sizeof(newFilePath), "%s%c%s", outpath, PATH_SEP, newFileName);

		if (newFilePath[strlen(newFilePath) - 1] == PATH_SEP)
		{
			if (!quiet || fs_debug->integer)
			{
				Com_Printf("FS_UnzipTo: Creating directory %s...\n", newFilePath);
			}

			if (FS_CreatePath(newFilePath))
			{
				Com_Printf(S_COLOR_RED "FS_UnzipTo ERROR: Unable to create directory (%s).\n", newFilePath);
				(void) unzClose(zipFile);
				return qfalse;
			}
		}
		else
		{
			if (!quiet || fs_debug->integer)
			{
				Com_Printf("FS_UnzipTo: Extracting %s...\n", newFilePath);
			}

			newFile = Sys_FOpen(newFilePath, "wb");
			if (!newFile)
			{
				Com_Printf(S_COLOR_YELLOW "FS_UnzipTo WARNING: Can't open file '%s'.\n", newFilePath);
				isUnZipOK = qfalse;
				break;
			}

			if (buf)
			{
				Com_Dealloc(buf);
			}

			buf = Com_Allocate(file_info.uncompressed_size);

			if (!buf)
			{
				Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't create buffer for output file.\n");
				fclose(newFile);
				isUnZipOK = qfalse;
				break;
			}

			err = unzOpenCurrentFile(zipFile);
			if (err)
			{
				Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't open current file. ERROR %i\n", err);
				fclose(newFile);
				isUnZipOK = qfalse;
				break;
			}

			err = unzReadCurrentFile(zipFile, buf, file_info.uncompressed_size);
			if ((err < 0) || (err != file_info.uncompressed_size))
			{
				Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't read current file. ERROR %i\n", err);
				(void) unzCloseCurrentFile(zipFile);
				fclose(newFile);
				isUnZipOK = qfalse;
				break;
			}

			err = fwrite(buf, file_info.uncompressed_size, 1, newFile);
			if (err != 1)
			{
				Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't write file.\n");
				(void) unzCloseCurrentFile(zipFile);
				fclose(newFile);
				isUnZipOK = qfalse;
				break;
			}

			fclose(newFile);

			(void) unzCloseCurrentFile(zipFile);
		}

		err = unzGoToNextFile(zipFile);
		if (err) // UNZ_OK and UNZ_EOF are both 0 - deal with errors and UNZ_END_OF_LIST_OF_FILE
		{
			switch (err)
			{
			case UNZ_END_OF_LIST_OF_FILE:     // if the actual file was the latest
				if (!quiet || fs_debug->integer)
				{
					Com_Printf(S_COLOR_YELLOW "FS_Unzip: End of files in '%s'.\n", fileName);
				}
				break;
			case UNZ_PARAMERROR:
				Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't go to next file in '%s' [file is NULL].\n", newFilePath);
				isUnZipOK = qfalse;
				break;
			case UNZ_BADZIPFILE:
				Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't go to next file in '%s' [bad zip file].\n", newFilePath);
				isUnZipOK = qfalse;
				break;
			case UNZ_INTERNALERROR:
				Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't go to next file in '%s' [internal error].\n", newFilePath);
				isUnZipOK = qfalse;
				break;
			case UNZ_CRCERROR:
				Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't go to next file in '%s' [crc error].\n", newFilePath);
				isUnZipOK = qfalse;
				break;
			default:
				Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't go to next file in '%s' [unknown error %i].\n", newFilePath, err);
				isUnZipOK = qfalse;
				break;
			}

			break; // unzip loop aborted!
		}
	}

	err = unzClose(zipFile);

	if (err != UNZ_OK)
	{
		Com_Printf(S_COLOR_YELLOW "FS_Unzip WARNING: Can't close zip file '%s' [unknown error %i].\n", fileName, err);
		isUnZipOK = qfalse;
	}

	if (buf)
	{
		Com_Dealloc(buf);
	}

	return isUnZipOK;
}

/**
 * @brief Extracts zipped file into the current gamedir
 * @param[in] fileName to extract
 * @param[in] quiet whether to inform if unzipping fails
 * @return qtrue    if successfully extracted
 * @return qfalse   if extraction failed
 */
qboolean FS_Unzip(const char *fileName, qboolean quiet)
{
	char newFilePath[MAX_OSPATH];

	Com_sprintf(newFilePath, sizeof(newFilePath), "%s%c%s", fs_homepath->string, PATH_SEP, fs_gamedir);
	return FS_UnzipTo(fileName, newFilePath, quiet);
}


/**
* @brief FS_Basename
* @param[in] path
* @return basename
*/
const char *FS_Basename(const char *path)
{
	static char base[MAX_OSPATH] = { 0 };
	int         length           = (int)strlen(path) - 1;

	// Skip trailing slashes
	while (length > 0 && (IsPathSep(path[length])))
		length--;

	while (length > 0 && !(IsPathSep(path[length - 1])))
		length--;

	Q_strncpyz(base, &path[length], sizeof(base));

	length = strlen(base) - 1;

	// Strip trailing slashes
	while (length > 0 && base[length] == '\\')
		base[length--] = '\0';

	return base;
}

/**
* @brief FS_Basename
* @param[in] path
* @return directory path
*/
const char *FS_Dirpath(const char *path)
{
	static char dirpath[MAX_OSPATH] = { 0 };
	int         index               = 0;
	int         lastSepPos          = 0;

	while (index < MAX_OSPATH && path[index])
	{
		if (IsPathSep(path[index]))
		{
			lastSepPos = index + 1;
		}
		index++;
	}

	Q_strncpyz(dirpath, path, lastSepPos);

	return dirpath;
}

/**
* @brief FS_MatchFileInPak
* @param[in] filepath to the pak file
* @param[in] match string
* @return qtrue if matched path is found
*/
qboolean FS_MatchFileInPak(const char *filepath, const char *match)
{
	int          i;
	pack_t       *pak;
	fileInPack_t *buildBuffer;
	qboolean     found = qfalse;

	pak = FS_LoadZipFile(filepath, "");

	if (!pak)
	{
		return qfalse;
	}

	buildBuffer = pak->buildBuffer;

	for (i = 0; i < pak->numfiles; i++)
	{
		if (Com_FilterPath(match, buildBuffer[i].name, qfalse))
		{
			found = qtrue;
			break;
		}
	}

	FS_FreePak(pak);

	return found;
}

/**
* @brief FS_CalculateFileSHA1
* @param[in] path to the file
* @param[out] hash sha1
* @return status
*/
int FS_CalculateFileSHA1(const char *path, char *hash)
{
	SHA1Context sha;
	FILE        *f;
	int         len;
	byte        *buf;
	const int   MAX_BUFFER_SIZE = 64 * 1024;

	Com_Memset(hash, 0, 40);
	SHA1Reset(&sha);

	f = Sys_FOpen(path, "rb");
	if (!f)
	{
		return 1;
	}

	fseek(f, 0, SEEK_END);
	len = ftell(f);

	if (len == -1)
	{
		fclose(f);
		return 1;
	}

	fseek(f, 0, SEEK_SET);

	buf = Com_Allocate(MAX_BUFFER_SIZE);
	if (!buf)
	{
		fclose(f);
		return 1;
	}

	while (1)
	{
		len = fread(buf, 1, MAX_BUFFER_SIZE, f);
		SHA1Input(&sha, buf, len);
		if (len != MAX_BUFFER_SIZE)
		{
			break;
		}
	}

	if (!SHA1Result(&sha))
	{
		fclose(f);
		Com_Dealloc(buf);
		return 1;
	}

	Com_Memcpy(hash, va(
				   "%08x%08x%08x%08x%08x",
				   sha.Message_Digest[0],
				   sha.Message_Digest[1],
				   sha.Message_Digest[2],
				   sha.Message_Digest[3],
				   sha.Message_Digest[4]
				   ), 40);

	fclose(f);
	Com_Dealloc(buf);

	return 0;
}

#if defined(FEATURE_PAKISOLATION) && !defined(DEDICATED)

typedef struct
{
	char name[MAX_OSPATH];
	char hash[41];
} pakMetaEntry_t;

#define WL_MAX_ENTRIES 16
#define WL_FILENAME    "checksums.txt"

pakMetaEntry_t pakMetaEntries[WL_MAX_ENTRIES];
pakMetaEntry_t *pakMetaEntryMap[WL_MAX_ENTRIES];

/**
* @brief FS_InitWhitelist loads the file containing whitelisted entries
*/
void FS_InitWhitelist()
{
	int            i = 0, p = 0, lc = 0, div, len, msec, fileLen, pakNameHash;
	FILE           *file;
	char           *fileListPath, *buf, *line;
	pakMetaEntry_t *pakEntry;

	msec = Sys_Milliseconds();

	Com_Memset(pakMetaEntries, 0, sizeof(pakMetaEntries));
	Com_Memset(pakMetaEntryMap, 0, sizeof(pakMetaEntryMap));

	fileListPath = va("%s%c%s", fs_homepath->string, PATH_SEP, WL_FILENAME);
	file         = Sys_FOpen(fileListPath, "rb");
	if (!file)
	{
		Com_DPrintf(S_COLOR_YELLOW "WARNING: " WL_FILENAME " was not found\n");
		return;
	}

	fseek(file, 0, SEEK_END);
	fileLen = ftell(file);
	if (fileLen == -1)
	{
		Com_DPrintf(S_COLOR_YELLOW "WARNING: unable to get current position in stream of file " WL_FILENAME "\n");
		fclose(file);
		return;
	}
	fseek(file, 0, SEEK_SET);

	buf = Com_Allocate(fileLen + 1);
	if (!buf)
	{
		Com_DPrintf(S_COLOR_YELLOW "WARNING: unable to allocate buffer for " WL_FILENAME " contents\n");
		fclose(file);
		return;
	}
	if (fread(buf, 1, fileLen, file) != fileLen)
	{
		Com_DPrintf(S_COLOR_YELLOW "WARNING: FS_InitWhitelist: short read");
		fclose(file);
		Com_Dealloc(buf);
		return;
	}

	fclose(file);

	buf[fileLen] = 0;
	line         = buf;

	// Each file hash entry consists of pk3 name and sha1 hash pair.
	// Each entry delimited by newline.
	while (i < WL_MAX_ENTRIES)
	{
		if (p >= fileLen)
		{
			break;
		}
		if (buf[p] == '\n' || !buf[p + 1])
		{
			lc++;
			len = &buf[p] - line;
			if (len == 0)
			{
				line = &buf[++p];
				continue;
			}
			for (div = len; div > 0; div--)
			{
				if (*(line + div) == ' ')
				{
					break;
				}
			}
			if (div == 0 || div == len || (len - div != 41))
			{
				Com_DPrintf(S_COLOR_YELLOW "WARNING: FS_InitWhitelist: Erroneous line %i found, ignoring the rest of file\n", lc);
				break;
			}
			pakEntry = &pakMetaEntries[i++];
			Com_Memcpy(pakEntry->name, line, (size_t)div);
			Com_Memcpy(pakEntry->hash, line + div + 1, 40);
			pakNameHash                  = FS_HashFileName(pakEntry->name, WL_MAX_ENTRIES);
			pakMetaEntryMap[pakNameHash] = pakEntry;

			line = &buf[p + 1];
		}
		else if (buf[p] == '/' && buf[p + 1] == '/')
		{
			while (++p < fileLen && (buf[p] && buf[p] != '\n')) /* skip line */
				;
			line = &buf[p + 1];
			lc++;
		}
		p++;
	}

	Com_Dealloc(buf);

	Com_DPrintf(S_COLOR_CYAN "INFO: %i entries imported from whitelist in %i ms\n", lc, (Sys_Milliseconds() - msec));
}

/**
* @brief FS_IsWhitelisted checks whether pak is contained in the whitelist
* @param[in] pakName the basename of the pak
* @param[out] hash the sha1 hash of the pak
*/
qboolean FS_IsWhitelisted(const char *pakName, const char *hash)
{
	int            i = 0;
	int            pakNameHash;
	pakMetaEntry_t *pakEntry;

	pakNameHash = FS_HashFileName(pakName, WL_MAX_ENTRIES);
	pakEntry    = pakMetaEntryMap[pakNameHash];

	if (!pakEntry)
	{
		// try manual search on hash miss
		for (i = 0; i < WL_MAX_ENTRIES; i++)
		{
			pakEntry = &pakMetaEntries[i];

			// list end, bail out
			if (!pakEntry->name[0] || !pakEntry->hash[0])
			{
				return qfalse;
			}

			// found it?
			if (!strcmp(pakEntry->name, pakName))
			{
				break;
			}
		}

		if (i == WL_MAX_ENTRIES)
		{
			return qfalse;
		}
	}

	// got match?
	if (!strcmp(pakEntry->hash, hash))
	{
		return qtrue;
	}
	return qfalse;
}
#endif
