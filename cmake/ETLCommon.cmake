function(force_rescan_library LIBRARY_NAME)
        set(${LIBRARY_NAME}_LIBRARY "${LIBRARY_NAME}_LIBRARY-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
endfunction()

IF(NOT CMAKE_BUILD_TYPE)
    #SET(CMAKE_BUILD_TYPE "Debug")
    SET(CMAKE_BUILD_TYPE "Release")
    MESSAGE("No CMAKE_BUILD_TYPE specified, defaulting to ${CMAKE_BUILD_TYPE}")
ENDIF(NOT CMAKE_BUILD_TYPE)

# set LEGACY_DEBUG definition for debug build type
# and set up properties to check if the build is visual studio or nmake on windows
string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype_upper)
string(TOUPPER "${CMAKE_GENERATOR}" buildgen_upper)
if(WIN32 AND buildgen_upper MATCHES "NMAKE MAKEFILES")
	SET(NMAKE_BUILD ON CACHE BOOL "NMake build")
else(WIN32 AND buildgen_upper MATCHES "NMAKE MAKEFILES")
	SET(NMAKE_BUILD OFF CACHE BOOL "NMake build")
endif(WIN32 AND buildgen_upper MATCHES "NMAKE MAKEFILES")

if(buildtype_upper MATCHES DEBUG)
	SET(DEBUG_BUILD ON CACHE BOOL "Debug build")
else(buildtype_upper MATCHES DEBUG)
	SET(DEBUG_BUILD OFF CACHE BOOL "Debug build")
endif(buildtype_upper MATCHES DEBUG)

if(MSVC AND NOT NMAKE_BUILD)
	SET(VSTUDIO ON CACHE BOOL "Visual studio build")
else(MSVC AND NOT NMAKE_BUILD)
	SET(VSTUDIO OFF CACHE BOOL "Visual studio build")
endif(MSVC AND NOT NMAKE_BUILD)

if(APPLE AND CMAKE_GENERATOR STREQUAL Xcode)
	SET(XCODE ON CACHE BOOL "Xcode build")
else()
	SET(XCODE OFF CACHE BOOL "Xcode build")
endif()

if(DEBUG_BUILD AND FORCE_DEBUG)
	add_definitions(-DLEGACY_DEBUG=1)
else()
	if(MSVC)
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /DLEGACY_DEBUG=1")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DLEGACY_DEBUG=1")
	else()
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLEGACY_DEBUG=1")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLEGACY_DEBUG=1")
	endif()
endif()

# Figure out what build is it (cool eh?)
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT CROSS_COMPILE32)
	SET(64BITS ON CACHE BOOL "64 Bit build")
	if(WIN32)
		SET(WIN64 ON CACHE BOOL "Windows 64 build")
	endif(WIN32)
else(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT CROSS_COMPILE32)
	SET(32BITS ON CACHE BOOL "32 Bit build")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT CROSS_COMPILE32)

# Installation options
set(INSTALL_DEFAULT_BASEDIR	""			CACHE STRING "Should be CMAKE_INSTALL_PREFIX + INSTALL_DEFAULT_MODDIR")
set(INSTALL_DEFAULT_BINDIR	"bin"			CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")
set(INSTALL_DEFAULT_MODDIR	"lib/etlegacy"		CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")

if(INSTALL_DEFAULT_BASEDIR)
	# On OS X the base dir is the .app's (or etlded binary's) parent path, and is set in the etl code itself
	# so we do NOT want to define DEFAULT_BASEDIR at build time.
	if(NOT APPLE)
		add_definitions(-DDEFAULT_BASEDIR=\"${INSTALL_DEFAULT_BASEDIR}\")
	endif()
endif(INSTALL_DEFAULT_BASEDIR)
