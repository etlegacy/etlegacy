#------------------------------------------------------------------------
# ET: Legacy, http://www.etlegacy.com
# - Wolfenstein: Enemy Territory 2.60b compatible client/server
# - based on raedwulf-et: https://bitbucket.org/tcmreastwood/raedwulf-et
#
# Please use TABs to indent! (x4)
#------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.10.3 FATAL_ERROR)

project(ETLEGACY C CXX)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
    cmake_policy(SET CMP0121 NEW)
endif()
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.22")
	cmake_policy(SET CMP0127 NEW)
endif()
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
	cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(CMakeDependentOption)

include(cmake/ETLBanner.cmake)
print_header()

# We need to add the macros & Functions before any other includes
include(cmake/ETLFunctions.cmake)

#-----------------------------------------------------------------
# General
#-----------------------------------------------------------------

set(MODNAME "legacy" CACHE STRING "Default mod name")
set(MODURL "www.etlegacy.com" CACHE STRING "Default mod URL")

#-----------------------------------------------------------------
# Options
#-----------------------------------------------------------------

option(BUILD_SERVER				"Build the dedicated server executable"							ON)
option(BUILD_CLIENT				"Build the client executable"									ON)
option(BUILD_MOD				"Build the mod libraries"										ON)

option(BUNDLED_LIBS				"Use all available bundled libraries instead of system libs"	ON)
option(BUNDLED_LIBS_DEFAULT		"Default value for bundled libs"								ON)

cmake_dependent_option(BUILD_CLIENT_MOD			"Build the client side mod libraries"							ON "BUILD_MOD" OFF)
cmake_dependent_option(BUILD_SERVER_MOD			"Build the server side mod libraries"							ON "BUILD_MOD" OFF)
cmake_dependent_option(BUILD_MOD_PK3			"Pack the mod libraries and game scripts into mod pk3"			ON "BUILD_CLIENT_MOD" OFF)

# for enabling memory debugging with release builds
cmake_dependent_option(ZONE_DEBUG				"Enable zone debug mode"										OFF "BUILD_CLIENT OR BUILD_SERVER" OFF)
cmake_dependent_option(HUNK_DEBUG				"Enable hunk debug mode"										OFF "BUILD_CLIENT OR BUILD_SERVER" OFF)

# no support for 32 bit binaries on OpenBSD/amd64 or Apple OSX anymore
cmake_dependent_option(CROSS_COMPILE32 "Compile 32-bit x86 version of ET:L (64bit is incompatible with 2.60b)" ON "NOT APPLE; NOT ANDROID;NOT CMAKE_SYSTEM MATCHES OpenBSD*;CMAKE_SYSTEM_PROCESSOR MATCHES x86|X86|amd64|AMD64" OFF)

# Optional features
option(FEATURE_CURL				"Enable auto-download support using cURL (client)"		ON)
option(FEATURE_SSL				"Build SSL for curl and authentication"					ON)
option(FEATURE_OGG_VORBIS		"Enable OGG Vorbis support (client)"					ON)
option(FEATURE_THEORA			"Enable Theora video support (client)"					ON)
option(FEATURE_OPENAL			"Enable OpenAL sound backend (client)"					ON)
option(FEATURE_FREETYPE			"Enable Freetype font library support (client)"			ON)
option(FEATURE_PNG				"Enable PNG screenshot support (client)"				ON)
option(FEATURE_TRACKER			"Enable extended server statistics (server)"			ON)
option(FEATURE_MULTIVIEW		"Enable multiview support (mod)"						ON)
option(FEATURE_EDV				"Enable extended demo viewer support (mod)"				ON)
option(FEATURE_ANTICHEAT		"Enable server side anti-wallhack code (server)"		ON)
option(FEATURE_AUTOUPDATE		"Enable updater which downloads latest ET:L files"		ON)
option(FEATURE_RENDERER1		"Build and use the vanilla renderer (OpenGL 1/2)"		ON)
option(FEATURE_RENDERER2		"Build and use the extended renderer (OpenGL 3)"		OFF)
option(FEATURE_RENDERER_GLES	"Build and use OpenGL ES renderer"						OFF)
option(FEATURE_RENDERER_VULKAN	"Build and use Vulkan renderer"							OFF)
option(FEATURE_IPV6				"Enable IPv6 networking code"							ON)
option(FEATURE_IRC_CLIENT		"Enable IRC client client side"							ON)
option(FEATURE_IRC_SERVER		"Enable IRC client server side"							ON)
option(RENDERER_DYNAMIC			"Build renderer into a dynamic library"					ON)
option(FEATURE_GETTEXT			"Enable localization using tinygettext"					ON)
option(FEATURE_SERVERMDX		"Enable MDX support on server side (mod)"				ON)
option(FEATURE_DBMS				"Enable dbms support (sqlite3)"							ON)
option(FEATURE_PAKISOLATION		"Enable custom pak isolation"							ON)
option(FORCE_DEBUG				"Force enable legacy debug macros"						OFF)
option(ENABLE_ASAN				"Enable AddressSanitizer on clang"						OFF)
option(ENABLE_MULTI_BUILD		"Enable binary names to include arch"					ON)
# TODO: implement this field on the libs as well
option(FORCE_STATIC_VCRT		"Force static MSVC runtime linking"						ON)
option(ENABLE_SSE				"Enable Streaming SIMD Extensions"						OFF)
option(FEATURE_AUTH				"Enable central authentication support"					ON)

cmake_dependent_option(FEATURE_WINDOWS_CONSOLE	"Build win32 executables with Windows console"	ON "WIN32" OFF)

cmake_dependent_option(FEATURE_LUA		"Enable Lua support (mod)"						ON "BUILD_SERVER_MOD" OFF)
cmake_dependent_option(FEATURE_LUASQL	"Enable LuaSQL sqlite3 backend (mod)"			ON "FEATURE_LUA" OFF)
cmake_dependent_option(FEATURE_RATING	"Enable skill rating support (mod)"				ON "FEATURE_DBMS" OFF)
cmake_dependent_option(FEATURE_PRESTIGE	"Enable prestige support (mod)"					ON "FEATURE_DBMS" OFF)
cmake_dependent_option(FEATURE_OMNIBOT	"Enable Omni-bot support (mod)"					ON "( NOT MINGW AND WIN32 ) OR FORCE_OMNIBOT OR CMAKE_SYSTEM_NAME MATCHES Linux" OFF)

option(INSTALL_EXTRA			"Install extra add-ons (omni-bot, geoip, wolfadmin)."	ON)

cmake_dependent_option(INSTALL_OMNIBOT		"Install Omni-bot"							ON "FEATURE_OMNIBOT AND INSTALL_EXTRA" OFF)
cmake_dependent_option(INSTALL_GEOIP		"Install GeoLite geolocation database"		ON "INSTALL_EXTRA" OFF)
cmake_dependent_option(INSTALL_WOLFADMIN	"Install WolfAdmin enhancement suite"		ON "INSTALL_EXTRA" OFF)

option(CLIENT_GLVND				"Link against GLVND OpenGL libraries"					OFF)

set(ET_FS_BASEPATH "" CACHE STRING "Copy required genuine ET files from ET_FS_BASEPATH")
cmake_dependent_option(ET_KEY	"Copy existing etkey file from ET_FS_BASEPATH)"			ON "ET_FS_BASEPATH" OFF)

if(BUILD_CLIENT OR BUILD_SERVER)
	set(BUILD_ENGINE ON)
endif()

# Will just remove the whole option if the feature is disabled
LEG_BUNDLED_LIB(SDL SDL "BUILD_CLIENT")
LEG_BUNDLED_LIB(ZLIB Zlib "BUILD_ENGINE")
LEG_BUNDLED_LIB(MINIZIP Minizip "BUILD_ENGINE")
LEG_BUNDLED_LIB(JPEG JPEG "BUILD_CLIENT")
LEG_BUNDLED_LIB(PNG libpng "BUILD_CLIENT AND FEATURE_PNG")
LEG_BUNDLED_LIB(CURL Curl "BUILD_ENGINE AND FEATURE_CURL" "NOT APPLE")
LEG_BUNDLED_LIB(OPENSSL OpenSSL "BUILD_ENGINE AND FEATURE_SSL" "NOT WIN32 AND NOT APPLE")
LEG_BUNDLED_LIB(WOLFSSL WolfSSL "BUILD_ENGINE AND FEATURE_SSL" "NOT BUNDLED_OPENSSL AND NOT WIN32 AND NOT APPLE")
LEG_BUNDLED_LIB(LUA Lua "FEATURE_LUA AND BUILD_MOD")
LEG_BUNDLED_LIB(OGG_VORBIS OGG "BUILD_CLIENT AND FEATURE_OGG_VORBIS")
LEG_BUNDLED_LIB(THEORA Theora "BUILD_CLIENT AND FEATURE_THEORA")
LEG_BUNDLED_LIB(OPENAL OpenAL "BUILD_CLIENT AND FEATURE_OPENAL" "NOT APPLE")
LEG_BUNDLED_LIB(OPENAL_INCLUDE OpenAL-OSX "BUILD_CLIENT AND FEATURE_OPENAL" "APPLE")
LEG_BUNDLED_LIB(GLEW Glew "BUILD_CLIENT AND ( FEATURE_RENDERER1 OR FEATURE_RENDERER2 )")
LEG_BUNDLED_LIB(FREETYPE Freetype2 "BUILD_CLIENT AND FEATURE_FREETYPE")
LEG_BUNDLED_LIB(SQLITE3 SQlite3 "BUILD_CLIENT OR BUILD_MOD")
LEG_BUNDLED_LIB(CJSON cJSON "BUILD_ENGINE OR BUILD_MOD")

#-----------------------------------------------------------------
# Setup
#-----------------------------------------------------------------

if (BUNDLED_OPENSSL AND BUNDLED_WOLFSSL)
	message(FATAL_ERROR "Cannot enable both OpenSSL and WolfSSL at the same time.")
endif()

# Group the VisualStudio projects
set(CMAKE_TARGETS_FOLDER CMake)
set(BUNDLED_TARGETS_FOLDER Bundled)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ${CMAKE_TARGETS_FOLDER})

# Set language levels
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)  # engine is intentionally C99 while mods remain C90

# Find and setup programs the build needs
include(cmake/ETLPrograms.cmake)

# Setup compiler flags for x86 builds
include(cmake/ETLCrossCompile32.cmake)

# Setup some basic helper variables
include(cmake/ETLCommon.cmake)

# Dummy targets for library binding
include(cmake/ETLTargets.cmake)

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

	#Since CMake 3.6 you are able to set the Visual Studio startup project
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.6")
        set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "etl")
    endif()
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	include(cmake/ETLBuildServer.cmake)
endif(BUILD_SERVER)

if(BUILD_MOD)
	include(cmake/ETLBuildMod.cmake)
endif(BUILD_MOD)

#-----------------------------------------------------------------
# Post build
#-----------------------------------------------------------------

# Install third-party add-ons
if(BUILD_MOD AND INSTALL_EXTRA)
	if(INSTALL_OMNIBOT)
		include(cmake/ETLInstallOmniBot.cmake)
	endif(INSTALL_OMNIBOT)
	if(INSTALL_GEOIP)
		include(cmake/ETLInstallGeoIP.cmake)
	endif(INSTALL_GEOIP)
	if(INSTALL_WOLFADMIN)
		include(cmake/ETLInstallWolfAdmin.cmake)
	endif(INSTALL_WOLFADMIN)
endif(BUILD_MOD AND INSTALL_EXTRA)

# Generate install and uninstall packs
include(cmake/ETLInstall.cmake)

# Packaging support
include(cmake/ETLCPack.cmake)

set_target_properties(uninstall PROPERTIES FOLDER ${CMAKE_TARGETS_FOLDER})
