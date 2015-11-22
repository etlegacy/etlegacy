# Copyright (C) 2007-2009 LuaDist.
# Created by Peter Kapec <kapecp@gmail.com>
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
#	Note:
#		Searching headers and libraries is very simple and is NOT as powerful as scripts
#		distributed with CMake, because LuaDist defines directories to search for.
#		Everyone is encouraged to contact the author with improvements. Maybe this file
#		becomes part of CMake distribution sometimes.

# - Find sqlite3
# Find the native SQLITE3 headers and libraries.
#
# SQLITE3_INCLUDE_DIRS - where to find sqlite3.h, etc.
# SQLITE3_LIBRARIES    - List of libraries when using sqlite.
# SQLITE3_FOUND	       - True if sqlite found.

# Look for the header file.
FIND_PATH(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
MARK_AS_ADVANCED(SQLITE3_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(SQLITE3_LIBRARY NAMES sqlite3)
MARK_AS_ADVANCED(SQLITE3_LIBRARY)

# Determine SQLite3 version
IF(SQLITE3_INCLUDE_DIR AND EXISTS "${SQLITE3_INCLUDE_DIR}/sqlite3.h")
	FILE(STRINGS "${SQLITE3_INCLUDE_DIR}/sqlite3.h" sqlite3_version_str REGEX "^#define SQLITE_VERSION[ ]+\".*\"")
	STRING(REGEX REPLACE "^#define SQLITE_VERSION[ ]+\"([^\"]*)\".*" "\\1" SQLITE3_VERSION_STRING "${sqlite3_version_str}")
	UNSET(sqlite3_version_str)
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set SQLITE3_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SQLite3 REQUIRED_VARS SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR VERSION_VAR SQLITE3_VERSION_STRING)

# Copy the results to the output variables.
IF(SQLITE3_FOUND)
	SET(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
	SET(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
ELSE(SQLITE3_FOUND)
	SET(SQLITE3_LIBRARIES)
	SET(SQLITE3_INCLUDE_DIRS)
ENDIF(SQLITE3_FOUND)
