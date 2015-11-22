#------------------------------------------------------------------------
# ET: Legacy, http://www.etlegacy.com
# - Wolfenstein: Enemy Territory 2.60b compatible client/server
# - based on raedwulf-et: https://bitbucket.org/tcmreastwood/raedwulf-et
#
# Please use TABs to indent! (x8)
#------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.3)

project(ETLEGACY C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)
include(CMakeDependentOption)
find_program(ZIP_EXECUTABLE zip PATHS ${CMAKE_BINARY_DIR})

#-----------------------------------------------------------------
# Options
#-----------------------------------------------------------------

option(BUILD_SERVER		"Build the dedicated server executable"				ON)
option(BUILD_CLIENT		"Build the client executable"					ON)
option(BUILD_MOD		"Build the mod libraries"					ON)
option(RPI			"Build RPI version"						OFF)

cmake_dependent_option(BUILD_MOD_PK3		"Pack the mod libraries into etl_bin.pk3"		ON "ZIP_EXECUTABLE" OFF)
cmake_dependent_option(BUILD_PAK3_PK3		"Pack updated game scripts into pak3.pk3"		ON "ZIP_EXECUTABLE" OFF)

option(BUNDLED_LIBS				"Use all available bundled libraries instead of system libs."	ON)
cmake_dependent_option(BUNDLED_SDL		"Use bundled SDL library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_CURL		"Use bundled CURL library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_JPEG		"Use bundled JPEG library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_LUA		"Use bundled LUA library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_OGG_VORBIS	"Use bundled OGG library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_THEORA		"Use bundled Theora library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_GLEW		"Use bundled GLEW library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_FREETYPE		"Use bundled Freetype2 library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_JANSSON		"Use bundled Jansson library."				OFF "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_SQLITE3		"Use bundled SQLITE3 library instead of the system one."	ON "BUNDLED_LIBS" OFF)

# no support for 32 bit binaries on OpenBSD/amd64
cmake_dependent_option(CROSS_COMPILE32 "Compile 32-bit version of ET:L (64bit is incompatible with 2.60b)" ON "NOT CMAKE_SYSTEM MATCHES OpenBSD*" OFF)

# Optional features
option(FEATURE_CURL		"Enable auto-download support using cURL (client)"			ON)
option(FEATURE_OGG_VORBIS	"Enable OGG Vorbis support (client)"					ON)
option(FEATURE_THEORA		"Enable Theora video support (client)"					ON)
option(FEATURE_OPENAL		"Enable OpenAL sound backend (client)"					OFF)
option(FEATURE_FREETYPE		"Enable Freetype font library support (client)"				ON)
option(FEATURE_TRACKER		"Enable extended server statistics (server)"				OFF)
option(FEATURE_LUA		"Enable Lua support in game code"					ON)
option(FEATURE_MULTIVIEW	"Compile the mod files with multiview support (broken)"			OFF)
option(FEATURE_ANTICHEAT	"Enable server side anti-wallhack code (experimental)"			ON)
option(FEATURE_CURSES		"Enable PDCurses console"						OFF)
option(FEATURE_AUTOUPDATE	"Enable updater which downloads latest ET:L files"			ON)
option(FEATURE_RENDERER2	"Build and use the new renderer"					ON)
option(FEATURE_RENDERER_GLES	"Set to ON if you want OpenGL ES renderer"				OFF)
option(FEATURE_IPV6		"Enable IPv6 networking code"						OFF)
option(FEATURE_IRC_CLIENT	"Enable IRC client"							OFF)
option(RENDERER_DYNAMIC		"Build renderer into a dynamic library"					ON)
option(FEATURE_WINDOWS_CONSOLE	"Build win32 executables with Windows console"				ON)
option(FEATURE_GETTEXT		"Enable localization using tinygettext"					ON)
option(FEATURE_JANSSON		"Enable JSON support (client)"						OFF)
option(FEATURE_SERVERMDX	"Enable MDX support on server side"					ON)
option(FEATURE_LIVEAUTH		"Enable ET:Live authentification support"				ON)
option(FEATURE_DBMS		"Enable dbms support (sqlite3)"						OFF)
option(FORCE_DEBUG		"Force enable legacy debug macros"					OFF)

cmake_dependent_option(FEATURE_OMNIBOT	"Enable Omni-bot in mod code (Windows, linux and OS X only)"	ON "WIN32 OR APPLE OR CMAKE_SYSTEM_NAME MATCHES Linux" OFF)
cmake_dependent_option(INSTALL_OMNIBOT	"Install Omni-bot"						ON "FEATURE_OMNIBOT" OFF)

set(ET_FS_BASEPATH		""	CACHE STRING "Copy required genuine ET files from ET_FS_BASEPATH")
cmake_dependent_option(ET_KEY	"Copy existing etkey file from ET_FS_BASEPATH)"				ON "ET_FS_BASEPATH" OFF)

#-----------------------------------------------------------------
# Setup
#-----------------------------------------------------------------

# Find and setup programs the build needs
include(cmake/ETLPrograms.cmake)

# Setup some basic helper variables
include(cmake/ETLCommon.cmake)

# Platform specific compiler settings
include(cmake/ETLPlatform.cmake)

# Source globs
include(cmake/ETLSources.cmake)

# Git version header generation
include(cmake/ETLVersion.cmake)

# Use bundled libraries
if(BUNDLED_LIBS)
	include(cmake/ETLBundledLibs.cmake)
endif(BUNDLED_LIBS)

# Set up includes and libraries based on feature options
include(cmake/ETLSetupFeatures.cmake)

# Group the sources (only msvc and xcode now)
include(cmake/ETLSourceGroup.cmake)

#-----------------------------------------------------------------
# Build
#-----------------------------------------------------------------

if(BUILD_CLIENT)
	include(cmake/ETLBuildRenderer.cmake)
	include(cmake/ETLBuildClient.cmake)
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	include(cmake/ETLBuildServer.cmake)
endif(BUILD_SERVER)

if(BUILD_MOD)
	include(cmake/ETLBuildMod.cmake)
endif(BUILD_MOD)

if(BUILD_PAK3_PK3)
	include(cmake/ETLBuildPack.cmake)
endif(BUILD_PAK3_PK3)

#-----------------------------------------------------------------
# Post build
#-----------------------------------------------------------------

if(INSTALL_OMNIBOT AND BUILD_MOD)
	include(cmake/ETLInstallOmniBot.cmake)
endif(INSTALL_OMNIBOT AND BUILD_MOD)

# Generate install and uninstall packs
include(cmake/ETLInstall.cmake)

# Packaging support
include(cmake/ETLCPack.cmake)
