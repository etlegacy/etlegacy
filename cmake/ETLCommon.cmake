#-----------------------------------------------------------------
# Common
#-----------------------------------------------------------------

function(force_rescan_library LIBRARY_NAME)
	set(${LIBRARY_NAME}_LIBRARY "${LIBRARY_NAME}_LIBRARY-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
endfunction()

IF(NOT CMAKE_BUILD_TYPE)
	#SET(CMAKE_BUILD_TYPE "Debug")
	SET(CMAKE_BUILD_TYPE "Release")
	MESSAGE("No CMAKE_BUILD_TYPE specified, defaulting to ${CMAKE_BUILD_TYPE}")
ENDIF()

# set ETLEGACY_DEBUG definition for debug build type
# and set up properties to check if the build is visual studio or nmake on windows
string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype_upper)
string(TOUPPER "${CMAKE_GENERATOR}" buildgen_upper)
if(WIN32 AND buildgen_upper MATCHES "NMAKE MAKEFILES")
	SET(NMAKE_BUILD 1)
else()
	SET(NMAKE_BUILD 0)
endif()

if(buildtype_upper MATCHES DEBUG)
	SET(DEBUG_BUILD 1)
else()
	SET(DEBUG_BUILD 0)
endif()

if(WIN32 AND buildgen_upper MATCHES "NINJA")
	SET(NINJA_BUILD 1)
else()
	SET(NINJA_BUILD 0)
endif()

# message(FATAL_ERROR "Using buildgen: ${buildgen_upper} ${NINJA_BUILD}")

if(MSVC AND NOT NMAKE_BUILD AND NOT NINJA_BUILD)
	SET(VSTUDIO 1)
else()
	SET(VSTUDIO 0)
endif()

if(NMAKE_BUILD OR NINJA_BUILD)
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
else()
	if(MSVC)
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /DETLEGACY_DEBUG=1")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DETLEGACY_DEBUG=1")
	else()
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DETLEGACY_DEBUG=1")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DETLEGACY_DEBUG=1")
	endif()
endif()

# Figure out what build is it (cool eh?)
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT CROSS_COMPILE32)
	SET(ETL_64BITS 1)
	if(WIN32)
		SET(ETL_WIN64 1)
	endif()
else()
	SET(ETL_32BITS 1)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set(ETL_X86 1)
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    set(ETL_ARM 1)
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
