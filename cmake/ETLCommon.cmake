#-----------------------------------------------------------------
# Common
#-----------------------------------------------------------------

IF(NOT CMAKE_BUILD_TYPE)
	#SET(CMAKE_BUILD_TYPE "Debug")
	SET(CMAKE_BUILD_TYPE "Release")
	MESSAGE("No CMAKE_BUILD_TYPE specified, defaulting to ${CMAKE_BUILD_TYPE}")
ENDIF()

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

# set ETLEGACY_DEBUG definition for debug build type
# and set up properties to check if the build is visual studio or nmake on windows
string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype_upper)
string(TOUPPER "${CMAKE_GENERATOR}" buildgen_upper)
if(WIN32 AND buildgen_upper MATCHES "NMAKE MAKEFILES")
	SET(NMAKE_BUILD 1)
else()
	SET(NMAKE_BUILD 0)
endif()

if(buildtype_upper STREQUAL DEBUG)
	SET(DEBUG_BUILD 1)
else()
	SET(DEBUG_BUILD 0)
endif()

if(WIN32 AND buildgen_upper STREQUAL "NINJA")
	SET(NINJA_BUILD 1)
else()
	SET(NINJA_BUILD 0)
endif()

# Since Clang is now a thing with Visual Studio
if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND MSVC)
    set(MSVC_CLANG 1)
endif()

if(MSVC AND NOT NMAKE_BUILD AND NOT NINJA_BUILD)
	SET(VSTUDIO 1)
else()
	SET(VSTUDIO 0)
endif()

# FIXME: remove this crap
if(NMAKE_BUILD OR MSVC)
	set(VS_BUILD 1)
else()
	set(VS_BUILD 0)
endif()

if(APPLE AND CMAKE_GENERATOR STREQUAL Xcode)
	SET(XCODE 1)
else()
	SET(XCODE 0)
endif()

if(DEBUG_BUILD OR FORCE_DEBUG)
	add_definitions(-DETLEGACY_DEBUG=1)
	if(WIN32)
		add_definitions(-DLEGACY_DUMP_MEMLEAKS=1)
	endif()
else()
	if(MSVC)
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /DETLEGACY_DEBUG=1 /DLEGACY_DUMP_MEMLEAKS=1")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DETLEGACY_DEBUG=1 /DLEGACY_DUMP_MEMLEAKS=1")
	else()
		if(WIN32)
			set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DETLEGACY_DEBUG=1 /DLEGACY_DUMP_MEMLEAKS=1")
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DETLEGACY_DEBUG=1 /DLEGACY_DUMP_MEMLEAKS=1")
		else()
			set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DETLEGACY_DEBUG=1")
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DETLEGACY_DEBUG=1")
		endif()
	endif()
endif()

# Figure out what build is it (cool eh?)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "64 bits target architecture detected")
	SET(ETL_64BITS 1)
	if(WIN32)
		SET(ETL_WIN64 1)
	endif()
elseif(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	# NOTE: this should never happen, but just in case for an invalid toolchain...
	message(FATAL_ERROR "Unknown target architecture detected. Pointer size: ${CMAKE_SIZEOF_VOID_P}")
else()
	message(STATUS "32 bits target architecture detected")
	SET(ETL_32BITS 1)
endif()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" system_name_lower)

if(system_name_lower MATCHES "(i386)|(i686)|(x86)|(amd64)")
	message(STATUS "x86 architecture detected")
	set(ETL_X86 1)
elseif(system_name_lower MATCHES "(arm)|(aarch64)")
	message(STATUS "ARM architecture detected")
	set(ETL_ARM 1)
else()
	message(WARNING "Unknown architecture detected: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Installation options
# If we are in windows clean these so the packaging is cleaner
# these need to be set before any other processing happens!
if(WIN32)
	set(INSTALL_DEFAULT_BASEDIR ".")
	set(INSTALL_DEFAULT_BINDIR ".")
	set(INSTALL_DEFAULT_SHAREDIR ".")
	set(INSTALL_DEFAULT_MODDIR ".")
else()
	set(INSTALL_DEFAULT_BASEDIR ""					CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")
	set(INSTALL_DEFAULT_BINDIR "bin"				CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")
	set(INSTALL_DEFAULT_SHAREDIR "share"			CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")
	set(INSTALL_DEFAULT_MODDIR "lib/etlegacy"		CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")
endif()

if(INSTALL_DEFAULT_BASEDIR)
	# On OS X the base dir is the .app's (or etlded binary's) parent path, and is set in the etl code itself
	# so we do NOT want to define DEFAULT_BASEDIR at build time.
	if(NOT APPLE)
		add_definitions(-DDEFAULT_BASEDIR=\"${INSTALL_DEFAULT_BASEDIR}\")
	endif()
endif()

if (ENABLE_SSE)
	if (APPLE AND CMAKE_OSX_ARCHITECTURES)
		list(LENGTH CMAKE_OSX_ARCHITECTURES OSX_ARCH_COUNT)
	endif()

	if (CMAKE_CROSSCOMPILING OR OSX_ARCH_COUNT GREATER "1")
		message(VERBOSE "We are crosscompiling, so we skip the SSE test")
		add_definitions(-DETL_ENABLE_SSE=1)
	else()
		include(CheckCSourceCompiles)
		check_c_source_compiles("
		#include <immintrin.h>
		int main()
		{
			__m128 tmp;
			float result = 0.f;
			tmp = _mm_set_ss(12.f);
			tmp = _mm_rsqrt_ss(tmp);
			result = _mm_cvtss_f32(tmp);
			return 0;
		}" ETL_ENABLE_SSE)

		if (ETL_ENABLE_SSE)
			message(STATUS "x86 intrinsics available")
			add_definitions(-DETL_ENABLE_SSE=1)
		else()
			message(WARNING "No x86 intrinsics available while trying to enable it")
		endif()
	endif()
endif()
