#------------------------------------------------------------------------
# ET:Legacy, http://www.etlegacy.com
# - Wolfenstein: Enemy Territory 2.60b compatible client/server
# - based on raedwulf-et: https://bitbucket.org/tcmreastwood/raedwulf-et
#
# Please use TABs to indent! (x8)
#------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.3)

# FIXME: currently eglport.c and eglport.h are disabled by macro FEATURE_RENDERER_GLES for non GLES builds
#        -> adjust the FILE sources and remove the macro from above files

project(ETLEGACY C CXX)
set(ETLEGACY_VERSION "2.71rc3-dirty")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(Build)
include(GetGitRevisionDescription)
include(CMakeDependentOption)

# Used to store real system processor when we overwrite CMAKE_SYSTEM_PROCESSOR for cross-compile builds
set(ETLEGACY_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# Options
option(BUILD_SERVER		"Build the dedicated server executable"				ON)
option(BUILD_CLIENT		"Build the client executable"					ON)
option(BUILD_MOD		"Build the mod libraries"					ON)
option(PANDORA			"Set to ON if targeting an OpenPandora device"			OFF)

# Find GNU sed executable, BSD variant does not work with our script
if(UNIX AND NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
	find_program(SED_EXECUTABLE gsed)
else()
	find_program(SED_EXECUTABLE sed)
endif()

# Enable pk3 creation if zip executable is found. Download it on Windows
find_program(ZIP_EXECUTABLE zip PATHS ${CMAKE_BINARY_DIR})
if(NOT ZIP_EXECUTABLE AND WIN32)
	message(STATUS "Downloading zip.exe to " ${CMAKE_BINARY_DIR}/zip.exe)
	file(DOWNLOAD http://stahlworks.com/dev/zip.exe ${CMAKE_BINARY_DIR}/zip.exe SHOW_PROGRESS TIMEOUT 10)
	find_program(ZIP_EXECUTABLE zip PATHS ${CMAKE_BINARY_DIR})
endif()
find_program(UNZIP_EXECUTABLE unzip PATHS ${CMAKE_BINARY_DIR})
if(NOT UNZIP_EXECUTABLE AND WIN32)
	message(STATUS "Downloading unzip.exe to " ${CMAKE_BINARY_DIR}/unzip.exe)
	file(DOWNLOAD http://stahlworks.com/dev/unzip.exe ${CMAKE_BINARY_DIR}/unzip.exe SHOW_PROGRESS TIMEOUT 10)
	find_program(UNZIP_EXECUTABLE unzip PATHS ${CMAKE_BINARY_DIR})
endif()

cmake_dependent_option(BUILD_MOD_PK3		"Pack the mod libraries into etl_bin.pk3"		ON "ZIP_EXECUTABLE" OFF)
cmake_dependent_option(BUILD_PAK3_PK3		"Pack updated game scripts into pak3.pk3"		ON "ZIP_EXECUTABLE" OFF)

option(BUNDLED_LIBS				"Use all available bundled libraries instead of system libs."	OFF)
cmake_dependent_option(BUNDLED_SDL		"Use bundled SDL library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_CURL		"Use bundled CURL library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_JPEG		"Use bundled JPEG library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_LUA		"Use bundled LUA library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_OGG_VORBIS	"Use bundled OGG library instead of the system one."	ON "BUNDLED_LIBS" OFF)
cmake_dependent_option(BUNDLED_GLEW		"Use bundled GLEW library instead of the system one."	ON "BUNDLED_LIBS" OFF)

# no support for 32 bit binaries on OpenBSD/amd64
cmake_dependent_option(CROSS_COMPILE32 "Compile 32-bit version of ET:L (64bit is incompatible with 2.60b)" ON "NOT CMAKE_SYSTEM MATCHES OpenBSD*" OFF)

# Installation options
set(INSTALL_DEFAULT_BASEDIR	""			CACHE STRING "Should be CMAKE_INSTALL_PREFIX + INSTALL_DEFAULT_MODDIR")
set(INSTALL_DEFAULT_BINDIR	"bin"			CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")
set(INSTALL_DEFAULT_MODDIR	"share/etlegacy"	CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")

if(INSTALL_DEFAULT_BASEDIR)
	add_definitions(-DDEFAULT_BASEDIR=\"${INSTALL_DEFAULT_BASEDIR}\")
endif(INSTALL_DEFAULT_BASEDIR)

# Optional features
option(FEATURE_CURL		"Enable auto-download support using cURL (client)"			ON)
option(FEATURE_OGG_VORBIS	"Enable OGG Vorbis support (client)"					ON)
option(FEATURE_OPENAL		"Enable OpenAL sound backend (client)"					OFF)
option(FEATURE_FREETYPE		"Enable Freetype font library support (client)"				OFF)
option(FEATURE_TRACKER		"Enable extended server statistics (server)"				ON)
option(FEATURE_LUA		"Enable Lua support in game code"					ON)
option(FEATURE_SDL2		"Experimental support for SDL 2"					ON)
option(FEATURE_MULTIVIEW	"Compile the mod files with multiview support (broken)"			OFF)
option(FEATURE_ANTICHEAT	"Enable server side anti-wallhack code (experimental)"			ON)
option(FEATURE_CURSES		"Enable PDCurses console"						OFF)
option(FEATURE_AUTOUPDATE	"Enable updater which downloads latest ET:L files"			ON)
option(FEATURE_RENDERER2	"Build and use the new renderer"					OFF)
option(FEATURE_RENDERER_GLES	"Set to ON if you want OpenGL ES renderer"				OFF)
option(FEATURE_IPV6		"Enable IPv6 networking code"						OFF)
option(FEATURE_IRC_CLIENT	"Enable IRC client"							OFF)
option(RENDERER_DYNAMIC		"Build renderer into a dynamic library"					OFF)
option(FEATURE_WINDOWS_CONSOLE	"Build win32 executables with Windows console"				ON)
option(FEATURE_CROUCH		"Set to ON if you want Toggle Crouch"					OFF)

cmake_dependent_option(FEATURE_GETTEXT	"Enable localization using tinygettext"				ON "BUNDLED_LIBS" OFF)

cmake_dependent_option(FEATURE_OMNIBOT	"Enable Omni-bot in mod code (Windows, linux and OS X only)"	ON "WIN32 OR APPLE OR CMAKE_SYSTEM_NAME MATCHES Linux" OFF)
cmake_dependent_option(INSTALL_OMNIBOT	"Install Omni-bot"						ON "FEATURE_OMNIBOT" OFF)

set(ET_FS_BASEPATH		""	CACHE STRING "Copy required genuine ET files from ET_FS_BASEPATH")
cmake_dependent_option(ET_KEY	"Copy existing etkey file from ET_FS_BASEPATH)"				ON "ET_FS_BASEPATH" OFF)

IF(NOT CMAKE_BUILD_TYPE)
    #SET(CMAKE_BUILD_TYPE "Debug")
    SET(CMAKE_BUILD_TYPE "Release")
    MESSAGE("No CMAKE_BUILD_TYPE specified, defaulting to ${CMAKE_BUILD_TYPE}") 
ENDIF(NOT CMAKE_BUILD_TYPE)

#-----------------------------------------------------------------
# Platform-specific settings
#-----------------------------------------------------------------

message(STATUS "System: ${CMAKE_SYSTEM} (${ETLEGACY_SYSTEM_PROCESSOR})")

if(UNIX AND CROSS_COMPILE32) # 32-bit build
	if(PANDORA)
		set(CMAKE_SYSTEM_PROCESSOR arm)
		message(STATUS "Forcing ${CMAKE_SYSTEM_PROCESSOR} to cross compile 32bit")
	else()
		set(CMAKE_SYSTEM_PROCESSOR i386)
		message(STATUS "Forcing ${CMAKE_SYSTEM_PROCESSOR} to cross compile 32bit")
	endif()
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
	
	if(PANDORA)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -fsigned-char")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -fsigned-char")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	endif()
	
	if(PANDORA)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp")
	else()
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m32")
	endif()
elseif(WIN32 AND CROSS_COMPILE32)
	set(CMAKE_SYSTEM_PROCESSOR x86) #the new cmake on windows will otherwise use arch name of x64 which will fuck up our naming
	set(ENV{PLATFORM} win32) #this is redundant but just to  be safe
endif()

if(UNIX)
	# optimization/debug flags
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ffast-math")
	if(PANDORA)
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ftree-vectorize -Ofast")
	else()
		if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
			set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
		endif()
	endif()
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")

	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math")
	if(PANDORA)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ftree-vectorize  -Ofast")
	endif()
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

	if(CMAKE_SYSTEM MATCHES "OpenBSD*")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.obsd.")
	elseif(CMAKE_SYSTEM MATCHES "FreeBSD")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.fbsd.")
	elseif(APPLE)
		set(OS_LIBRARIES dl m)
		set(CMAKE_EXE_LINKER_FLAGS "-lobjc -framework Cocoa -framework IOKit -framework CoreFoundation")
		set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem") # These flags will cause error with older Xcode
		set(CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem")
		if(BUILD_CLIENT)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Quartz -framework AudioUnit -framework Carbon")
		endif()
		set(LIB_SUFFIX "_mac")
		set(CMAKE_SHARED_MODULE_SUFFIX "")
	else()
		set(OS_LIBRARIES dl m rt pthread)
		set(LIB_SUFFIX ".mp.")
	endif()
	
	if(NOT MSYS)
		include(CheckCCompilerFlag)
		check_c_compiler_flag("-fvisibility=hidden" SUPPORT_VISIBILITY)
		if(SUPPORT_VISIBILITY) # GCC 4+
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
		endif(SUPPORT_VISIBILITY)
	endif(NOT MSYS)
	
elseif(WIN32)
	add_definitions(-DWINVER=0x501)
	set(OS_LIBRARIES ws2_32 psapi winmm)
	set(LIB_SUFFIX "_mp_")
	if(MSVC)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /EHsc /Oi")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /EHa /W3")
		add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Do not show CRT warnings
	endif(MSVC)
	if(MINGW)
		add_definitions(-D_WIN32_IE=0x0501)
	endif(MINGW)
endif()

# Get the system architecture
if(NOT APPLE)
	if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i386" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86")
		if(WIN32)
			set(ARCH "x86")
		else()
			set(ARCH "i386")
		endif()
	elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
		set(ARCH "x86_64")
	else()
		set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
		message(STATUS "Warning: processor architecture not recognised (${CMAKE_SYSTEM_PROCESSOR})")
	endif()
endif(NOT APPLE)

if(PANDORA)
	add_definitions(-DPANDORA -DARM -DNEON -DAUTOAIM)
endif()
if (FEATURE_RENDERER_GLES)
	add_definitions(-DFEATURE_RENDERER_GLES)
endif()
if (FEATURE_CROUCH)
	add_definitions(-DFEATURE_CROUCH)
endif()

#-----------------------------------------------------------------
# Sources
#-----------------------------------------------------------------

FILE(GLOB COMMON_SRC
	"src/qcommon/*.c"
	"src/qcommon/*.h"
)

FILE(GLOB ZLIB_SRC
	"src/zlib/*.c"
	"src/zlib/*.h"
)

FILE(GLOB COMMON_SRC_REMOVE
	"src/qcommon/dl_main_curl.c"
	"src/qcommon/dl_main_stubs.c"
	"src/qcommon/i18n_*"
)

LIST(REMOVE_ITEM COMMON_SRC ${COMMON_SRC_REMOVE})

# Platform specific code for server and client
if(UNIX)
	if(APPLE)
		LIST(APPEND PLATFORM_SRC "src/sys/sys_osx.m")
		SET_SOURCE_FILES_PROPERTIES("src/sys/sys_osx.m" PROPERTIES LANGUAGE C)
	endif(APPLE)

	LIST(APPEND PLATFORM_SRC "src/sys/sys_unix.c")
	LIST(APPEND PLATFORM_SRC "src/sys/con_tty.c")
elseif(WIN32)
	LIST(APPEND PLATFORM_SRC "src/sys/sys_win32.c")
	LIST(APPEND PLATFORM_SRC "src/sys/sys_win32_con.c")
	LIST(APPEND PLATFORM_SRC "src/sys/con_win32.c")
	LIST(APPEND PLATFORM_SRC "src/sys/win_resource.rc")
endif()

FILE(GLOB SERVER_SRC
	"src/server/*.c"
	"src/server/*.h"
	"src/null/*.c"
	"src/null/*.h"
	"src/botlib/be*.c"
	"src/botlib/be*.h"
	"src/botlib/l_*.c"
	"src/botlib/l_*.h"
	"src/sys/sys_main.c"
	"src/sys/con_log.c"
	"src/qcommon/dl_main_stubs.c"
)

FILE(GLOB CLIENT_SRC
	"src/server/*.c"
	"src/server/*.h"
	"src/client/*.c"
	"src/client/*.h"
	"src/botlib/be*.c"
	"src/botlib/be*.h"
	"src/botlib/l_*.c"
	"src/botlib/l_*.h"
	"src/sys/sys_main.c"
	"src/sys/con_log.c"
#	"src/renderer/*.c"
#	"src/renderer/*.h"
	"src/sdl/*.c"
	"src/sdl/*.h"
)

FILE(GLOB CLIENT_SRC_REMOVE
	"src/sdl/sdl_glimp.c"
	"src/sdl/sdl_gamma.c"
	"src/sdl/sdl_icon.h"
)

LIST(REMOVE_ITEM CLIENT_SRC ${CLIENT_SRC_REMOVE})

FILE(GLOB CGAME_SRC
	"src/cgame/*.c"
	"src/cgame/*.h"
	"src/qcommon/q_math.c"
	"src/qcommon/q_shared.c"
	"src/ui/ui_shared.c"
	"src/game/bg_*.c"
)

FILE(GLOB QAGAME_SRC
	"src/game/*.c"
	"src/game/*.h"
	"src/game/sha-1/sha1.c"
	"src/qcommon/q_math.c"
	"src/qcommon/q_shared.c"
)

FILE(GLOB UI_SRC
	"src/ui/*.c"
	"src/ui/*.h"
	"src/qcommon/q_math.c"
	"src/qcommon/q_shared.c"
	"src/game/bg_classes.c"
	"src/game/bg_misc.c"
)

FILE(GLOB CLIENT_FILES
	"src/client/*.c"
)

FILE(GLOB SERVER_FILES
	"src/server/*.c"
)

FILE(GLOB SYSTEM_FILES
	"src/sys/sys_main.c"
	"src/sys/con_log.c"
)

FILE(GLOB SDL_FILES
	"src/sdl/*.c"
)

FILE(GLOB BOTLIB_FILES
	"src/botlib/be*.c"
	"src/botlib/l_*.c"
)

FILE(GLOB RENDERER_COMMON
	"src/renderercommon/*.c"
	"src/renderercommon/*.h"
	#Library build requires the following
	"src/sdl/sdl_glimp.c"
	"src/sdl/sdl_gamma.c"
	"src/sdl/sdl_icon.h"
	"src/sys/sys_local.h"
	"src/qcommon/q_shared.h"
	"src/qcommon/puff.h"
)

FILE(GLOB RENDERER_COMMON_DYNAMIC
	"src/qcommon/q_shared.c"
	"src/qcommon/q_math.c"
	"src/qcommon/puff.c"
)
	
FILE(GLOB RENDERER1_FILES
	"src/renderer/*.c"
	"src/renderer/*.h"
)

FILE(GLOB RENDERERGLES_FILES
	"src/rendererGLES/*.c"
	"src/rendererGLES/*.h"
	"src/sdl/eglport.c"
)

FILE(GLOB RENDERER2_FILES
	"src/renderer2/*.c"
	"src/renderer2/*.cpp"
	"src/renderer2/*.h"
)

FILE(GLOB RENDERER2_SHADERS
	"src/renderer2/glsl/*.glsl"
)

FILE(GLOB IRC_CLIENT_FILES
	"src/qcommon/htable.c"
	"src/qcommon/htable.h"
)

# Version generation
git_describe(GIT_DESCRIBE)
git_describe(GIT_DESCRIBE_TAG "--abbrev=0")
if(GIT_DESCRIBE)
	set(ETL_CMAKE_VERSION ${GIT_DESCRIBE})
	set(ETL_CMAKE_VERSION_SHORT ${GIT_DESCRIBE_TAG})
else() # Not using source from git repo
	set(ETL_CMAKE_VERSION ${ETLEGACY_VERSION})
	set(ETL_CMAKE_VERSION_SHORT ${ETLEGACY_VERSION})
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/git_version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/git_version.h" @ONLY)
list(APPEND COMMON_SRC "${CMAKE_CURRENT_BINARY_DIR}/include/git_version.h")

# If we change architecture we need to force rescan of libraries
if(NOT OLD_CROSS_COMPILE32 STREQUAL CROSS_COMPILE32)
	force_rescan_library(SDL32)
	force_rescan_library(CURL)
	force_rescan_library(JPEG)
	# TODO: recheck optional libs
	set(OLD_CROSS_COMPILE32 ${CROSS_COMPILE32} CACHE INTERNAL "Previous value for CROSS_COMPILE32")
	message(STATUS "Libraries rescanned")
endif(NOT OLD_CROSS_COMPILE32 STREQUAL CROSS_COMPILE32)

# Use bundled libraries
if(BUNDLED_LIBS OR BUNDLED_SDL OR BUNDLED_CURL AND FEATURE_CURL OR BUNDLED_JPEG OR BUNDLED_GLEW OR BUNDLED_LUA AND FEATURE_LUA OR BUNDLED_OGG_VORBIS AND FEATURE_OGG_VORBIS)
	if(EXISTS "${CMAKE_SOURCE_DIR}/libs/CMakeLists.txt")
		message(STATUS "Using bundled libraries located at ${CMAKE_SOURCE_DIR}/libs")
		add_subdirectory(libs)
	else()
		message(STATUS "======================================================")
		message(STATUS "Bundled libraries were not found on your system!")
		message(STATUS "======================================================")
		message(STATUS "You need the *multilib* package to crosscompile ET:L on a 64bit system.")
		message(STATUS "Alternatively clone etlegacy repository and then run")
		message(STATUS "        'git submodule init && git submodule update'")
		message(STATUS "and enable BUNDLED_* in CMake configuration.")
		message(FATAL_ERROR "Build stopped because of missing libraries.")
	endif()
endif(BUNDLED_LIBS OR BUNDLED_SDL OR BUNDLED_CURL AND FEATURE_CURL OR BUNDLED_JPEG OR BUNDLED_GLEW OR BUNDLED_LUA AND FEATURE_LUA OR BUNDLED_OGG_VORBIS AND FEATURE_OGG_VORBIS)

#-----------------------------------------------------------------
# Client features
#-----------------------------------------------------------------
if(BUILD_CLIENT)
	if(FEATURE_RENDERER_GLES)
		list(APPEND RENDERER_LIBRARIES -lEGL -lGLES_CM)
		include_directories(SYSTEM /mnt/utmp/codeblocks/usr/include/gles)
	else()
		find_package(OpenGL REQUIRED)
		list(APPEND RENDERER_LIBRARIES ${OPENGL_LIBRARIES})
		include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
	endif()

	if(NOT WIN32) # Dependency of GLEW and SDL_syswm.h
		find_package(X11 REQUIRED)
		include_directories(${X11_INCLUDE_DIR})
	endif(NOT WIN32)

	if(NOT FEATURE_RENDERER_GLES)
		if(NOT BUNDLED_GLEW)
			find_package(GLEW REQUIRED)
			list(APPEND RENDERER_LIBRARIES ${GLEW_LIBRARY})
			include_directories(SYSTEM ${GLEW_INCLUDE_PATH})
		else()
			list(APPEND RENDERER_LIBRARIES ${BUNDLED_GLEW_LIBRARIES})
			include_directories(SYSTEM ${BUNDLED_GLEW_INCLUDE_DIR})
			add_definitions(-DBUNDLED_GLEW)
			add_definitions(-DGLEW_STATIC)
		endif()
	endif()

	if(NOT BUNDLED_SDL)
		if(FEATURE_SDL2)
			find_package(SDL2 REQUIRED) # FindSDL doesn't detect 32bit lib when crosscompiling
			list(APPEND SDL_LIBRARIES ${SDL2_LIBRARY})
			include_directories(SYSTEM ${SDL2_INCLUDE_DIR})
		else(FEATURE_SDL2) # SDL 1.3
			find_package(SDL32 REQUIRED) # FindSDL doesn't detect 32bit lib when crosscompiling
			if(WIN32)
				list(APPEND SDL_LIBRARIES ${SDL32MAIN_LIBRARY})
			endif()
			list(APPEND SDL_LIBRARIES ${SDL32_LIBRARIES})
			include_directories(SYSTEM ${SDL32_INCLUDE_DIR})
		endif(FEATURE_SDL2)
	else() # BUNDLED_SDL
		list(APPEND SDL_LIBRARIES ${SDL32_BUNDLED_LIBRARIES})
		include_directories(SYSTEM ${SDL32_BUNDLED_INCLUDE_DIR})
		add_definitions(-DBUNDLED_SDL)
	endif()
	if(APPLE)
		add_library(INTERNAL_SDLMain ${CMAKE_SOURCE_DIR}/src/sys/SDLMain.m )
		list(APPEND RENDERER_LIBRARIES ${INTERNAL_SDLMain})
	endif(APPLE)

	if(NOT BUNDLED_JPEG)
		find_package(JPEG 8 REQUIRED)
		list(APPEND RENDERER_LIBRARIES ${JPEG_LIBRARIES})
		include_directories(SYSTEM ${JPEG_INCLUDE_DIR})

		# Check for libjpeg v8
		include(CheckFunctionExists)
		set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
		set(CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARY})
		# FIXME: function is checked, but HAVE_JPEG_MEM_SRC is empty. Why?
		check_function_exists("jpeg_mem_src" HAVE_JPEG_MEM_SRC)
	else()
		list(APPEND RENDERER_LIBRARIES ${JPEG_BUNDLED_LIBRARIES})
		include_directories(SYSTEM ${JPEG_BUNDLED_INCLUDE_DIR})
	endif()

	if(FEATURE_CURL)
		if(NOT BUNDLED_CURL)
			find_package(CURL REQUIRED)
			list(APPEND CLIENT_LIBRARIES ${CURL_LIBRARIES})
			include_directories(SYSTEM ${CURL_INCLUDE_DIR})
			if(MINGW)
				add_definitions(-DCURL_STATICLIB)
			endif(MINGW)
		else() # BUNDLED_CURL
			list(APPEND CLIENT_LIBRARIES ${CURL_BUNDLED_LIBRARY}) # NOTE: LIBRARY not LIBRARIES
			include_directories(SYSTEM ${CURL_BUNDLED_INCLUDE_DIR})
			add_definitions(-DCURL_STATICLIB)
		endif()
		set(CLIENT_SRC ${CLIENT_SRC} "src/qcommon/dl_main_curl.c")
	else(FEATURE_CURL)
		set(CLIENT_SRC ${CLIENT_SRC} "src/qcommon/dl_main_stubs.c")
	endif(FEATURE_CURL)

	if(FEATURE_GETTEXT)
		add_definitions(-DFEATURE_GETTEXT)
		FILE(GLOB GETTEXT_SRC
			"src/qcommon/i18n_main.cpp"
			"src/qcommon/i18n_findlocale.c"
			"src/qcommon/i18n_findlocale.h"
			"libs/tinygettext/dictionary_manager.hpp"
			"libs/tinygettext/file_system.hpp"
			"libs/tinygettext/iconv.cpp"
			"libs/tinygettext/plural_forms.hpp"
			"libs/tinygettext/tinygettext.cpp"
			"libs/tinygettext/tinygettext.hpp"
			"libs/tinygettext/dictionary.cpp"
			"libs/tinygettext/dictionary.hpp"
			"libs/tinygettext/dictionary_manager.cpp"
			"libs/tinygettext/iconv.hpp"
			"libs/tinygettext/language.cpp"
			"libs/tinygettext/language.hpp"
			"libs/tinygettext/log.cpp"
			"libs/tinygettext/log.hpp"
			"libs/tinygettext/log_stream.hpp"
			"libs/tinygettext/plural_forms.cpp"
			"libs/tinygettext/po_parser.cpp"
			"libs/tinygettext/po_parser.hpp"
		)
		set(CLIENT_SRC ${CLIENT_SRC} ${GETTEXT_SRC})
	endif(FEATURE_GETTEXT)

	if(FEATURE_AUTOUPDATE)
		add_definitions(-DFEATURE_AUTOUPDATE)
	endif(FEATURE_AUTOUPDATE)

	if(FEATURE_IPV6)
		add_definitions(-DFEATURE_IPV6)
	endif(FEATURE_IPV6)

	if(FEATURE_FREETYPE)
		find_package(Freetype REQUIRED)
		list(APPEND RENDERER_LIBRARIES ${FREETYPE_LIBRARIES})
		include_directories(SYSTEM ${FREETYPE_INCLUDE_DIRS})
		add_definitions(-DFEATURE_FREETYPE)
	endif(FEATURE_FREETYPE)

	if(FEATURE_OPENAL)
		find_package(OpenAL REQUIRED)
		list(APPEND CLIENT_LIBRARIES ${OPENAL_LIBRARIES})
		include_directories(SYSTEM ${OPENAL_INCLUDE_DIR})
		add_definitions(-DFEATURE_OPENAL)
		add_definitions(-DFEATURE_OPENAL_DLOPEN)
	endif(FEATURE_OPENAL)

	if(FEATURE_OGG_VORBIS)
		if(NOT BUNDLED_OGG_VORBIS)
			find_package(Vorbis REQUIRED)
			list(APPEND CLIENT_LIBRARIES ${VORBIS_FILE_LIBRARY} ${OGG_LIBRARY} ${VORBIS_LIBRARY})
			include_directories(SYSTEM ${VORBIS_INCLUDE_DIR})
		else() # BUNDLED_OGG_VORBIS
			list(APPEND CLIENT_LIBRARIES ${OGG_VORBIS_BUNDLED_LIBRARIES} ${OGG_BUNDLED_LIBRARIES})
			include_directories(SYSTEM ${OGG_VORBIS_BUNDLED_INCLUDE_DIR} ${OGG_BUNDLED_INCLUDE_DIR})
		endif()
		add_definitions(-DFEATURE_OGG_VORBIS)
	endif(FEATURE_OGG_VORBIS)
	
	if(FEATURE_IRC_CLIENT)
		add_definitions(-DFEATURE_IRC_CLIENT)
		list(APPEND CLIENT_SRC ${IRC_CLIENT_FILES})
	endif(FEATURE_IRC_CLIENT)
endif(BUILD_CLIENT)

#-----------------------------------------------------------------
# Mod features
#-----------------------------------------------------------------
if(BUILD_MOD)
	if(FEATURE_MULTIVIEW)
		add_definitions(-DFEATURE_MULTIVIEW)
	endif(FEATURE_MULTIVIEW)

	if(FEATURE_LUA)
		if(NOT BUNDLED_LUA)
			find_package(Lua 5.1 REQUIRED)
			list(APPEND MOD_LIBRARIES ${LUA_LIBRARIES})
			include_directories(SYSTEM ${LUA_INCLUDE_DIR})

			if(${LUA_VERSION_STRING} MATCHES "^5.2")
				add_definitions(-DLUA_5_2)
			elseif(NOT LUA_VERSION_STRING)
				message(WARNING "Unable to detect Lua version, assuming 5.1")
			endif()
		else() # BUNDLED_LUA
			list(APPEND MOD_LIBRARIES ${LUA_BUNDLED_LIBRARIES})
			include_directories(SYSTEM ${LUA_BUNDLED_INCLUDE_DIR})
			add_definitions(-DBUNDLED_LUA)
		endif()
		add_definitions(-DFEATURE_LUA)
	endif(FEATURE_LUA)

	if(FEATURE_OMNIBOT) # 32bit Windows, Linux and OS X only
		if("${ARCH}" STREQUAL "i386" OR "${ARCH}" STREQUAL "x86")
			LIST(APPEND QAGAME_SRC "src/game/g_etbot_interface.cpp")
			LIST(APPEND QAGAME_SRC "src/Omnibot/Common/BotLoadLibrary.cpp")
			add_definitions(-DFEATURE_OMNIBOT)
		else()
			message(STATUS "Warning: Omni-bot support is 32bit only - build skipped")
		endif()
	endif(FEATURE_OMNIBOT)
endif(BUILD_MOD)

#-----------------------------------------------------------------
# Server/Common features
#-----------------------------------------------------------------
if(FEATURE_TRACKER)
	add_definitions(-DFEATURE_TRACKER)
endif(FEATURE_TRACKER)

if(FEATURE_ANTICHEAT)
	add_definitions(-DFEATURE_ANTICHEAT)
endif(FEATURE_ANTICHEAT)

if(FEATURE_CURSES)
	find_package(Curses REQUIRED)
	set(CURSES_NEED_NCURSES 1) # Tells FindCurses that ncurses is required
	list(APPEND CLIENT_LIBRARIES ${CURSES_LIBRARIES})
	list(APPEND SERVER_LIBRARIES ${CURSES_LIBRARIES})
	include_directories(SYSTEM ${CURSES_INCLUDE_DIR})
	list(APPEND COMMON_SRC "src/sys/con_curses.c")
	add_definitions(-DFEATURE_CURSES)
endif(FEATURE_CURSES)

#-----------------------------------------------------------------
# Groups for Visual Studio (Windows) and win32 definitions
#-----------------------------------------------------------------
if(WIN32)
	if(MSVC)
		FILE(GLOB CLIENT_SRC_GROUP
			"src/client/cl_*.h"
			"src/client/cl_*.c"
		)

		FILE(GLOB CLIENT_SND_SRC_GROUP
			"src/client/qal.c"
			"src/client/qal.h"
			"src/client/snd_*.c"
			"src/client/snd_*.h"
		)

		FILE(GLOB TINY_GETTEXT_SRC_GROUP
			"libs/tinygettext/*.cpp"
			"libs/tinygettext/*.hpp"
		)

		source_group("Source Files\\Client" FILES ${CLIENT_SRC_GROUP})
		source_group("Source Files\\Sound" FILES ${CLIENT_SND_SRC_GROUP})
		source_group("Source Files\\Server" FILES ${SERVER_FILES})
		source_group("Source Files\\System" FILES ${SYSTEM_FILES})
		source_group("Source Files\\SDL" FILES ${SDL_FILES})
		source_group("Source Files\\Botlib" FILES ${BOTLIB_FILES})
		source_group("Source Files\\Common" FILES ${COMMON_SRC})
		source_group("Source Files\\Platform" FILES ${PLATFORM_SRC})
		source_group("Source Files\\zlib" FILES ${ZLIB_SRC})
		source_group("Source Files\\GetText" FILES ${TINY_GETTEXT_SRC_GROUP})
		source_group("Source Files\\Renderer" FILES ${RENDERER1_FILES})
		source_group("Source Files\\Renderer2" FILES ${RENDERER2_FILES})
		source_group("Source Files\\RendererCommon" FILES ${RENDERER_COMMON})
		source_group("Shaders" FILES ${RENDERER2_SHADERS})
	endif(MSVC)
	
	if(FEATURE_WINDOWS_CONSOLE)
		add_definitions( "-DUSE_WINDOWS_CONSOLE" )
	endif(FEATURE_WINDOWS_CONSOLE)
endif(WIN32)

#-----------------------------------------------------------------
# Build
#-----------------------------------------------------------------
include_directories(${PROJECT_BINARY_DIR}/include) # git_version.h

if(BUILD_CLIENT)
	#RENDERER BUILDS
	if(RENDERER_DYNAMIC)
		MESSAGE("Will build dynamic renderer libraries")
		add_definitions( "-DUSE_RENDERER_DLOPEN" )
		set(REND_LIBTYPE MODULE)
		list(APPEND RENDERER_COMMON ${RENDERER_COMMON_DYNAMIC}) 
	else(RENDERER_DYNAMIC)
		set(REND_LIBTYPE STATIC)
	endif(RENDERER_DYNAMIC)
	if(RENDERER_DYNAMIC OR NOT FEATURE_RENDERER2)
		if(FEATURE_RENDERER_GLES)
			add_library(renderer_opengl1_${ARCH} ${REND_LIBTYPE} ${RENDERERGLES_FILES} ${RENDERER_COMMON})
		else()
			add_library(renderer_opengl1_${ARCH} ${REND_LIBTYPE} ${RENDERER1_FILES} ${RENDERER_COMMON})
		endif()
		if(BUNDLED_SDL)
			add_dependencies(renderer_opengl1_${ARCH} bundled_sdl)
		endif(BUNDLED_SDL)
		if(NOT FEATURE_RENDERER_GLES)
			if(BUNDLED_GLEW)
				add_dependencies(renderer_opengl1_${ARCH} bundled_glew)
			endif(BUNDLED_GLEW)
		endif(NOT FEATURE_RENDERER_GLES)
		if(BUNDLED_JPEG)
			add_dependencies(renderer_opengl1_${ARCH} bundled_jpeg)
		endif(BUNDLED_JPEG)
		target_link_libraries(renderer_opengl1_${ARCH} ${RENDERER_LIBRARIES} ${SDL_LIBRARIES})

		# install the dynamic lib only
		if(RENDERER_DYNAMIC)
			install(TARGETS renderer_opengl1_${ARCH}
				LIBRARY DESTINATION "${INSTALL_DEFAULT_BINDIR}"
				ARCHIVE DESTINATION "${INSTALL_DEFAULT_BINDIR}"
			)
		endif(RENDERER_DYNAMIC)
		if(NOT RENDERER_DYNAMIC)
			list(APPEND CLIENT_LIBRARIES renderer_opengl1_${ARCH})
		endif(NOT RENDERER_DYNAMIC)
	endif(RENDERER_DYNAMIC OR NOT FEATURE_RENDERER2)
	if(FEATURE_RENDERER2)
		if(MSVC)
			list(APPEND RENDERER2_FILES ${RENDERER2_SHADERS})
		endif(MSVC)
		
		#This is where we generate the fallback shaders source file.
		if(SED_EXECUTABLE)
			SET(SHADER_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/glsl/tr_glslsources.c")
			FILE(READ "src/renderer2/glsl/copyright.txt" SHADER_SOURCES)
			SET(SHADER_SOURCES "${SHADER_SOURCES}\n/* This file was generated by CMake do not modify as it will get overwritten */\n\n#include \"${CMAKE_CURRENT_SOURCE_DIR}/src/renderer2/tr_local.h\"\n\n")
			FOREACH (SHAD ${RENDERER2_SHADERS})
				GET_FILENAME_COMPONENT(SHAD_NAME ${SHAD} NAME_WE)
				string(TOLOWER ${SHAD_NAME} SHAD_NAME_LOW)
				set(SHAD_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/glsl)
				execute_process(COMMAND ${SED_EXECUTABLE} -f ${CMAKE_CURRENT_SOURCE_DIR}/glsl2c.sed ${SHAD}
					WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
					OUTPUT_VARIABLE OUT_RES
					RESULT_VARIABLE RES_VAR
				)
				IF(SHADER_FUNCTION)
					SET(SHADER_FUNCTION "${SHADER_FUNCTION}\telse if(!Q_stricmp(name,\"${SHAD_NAME_LOW}\"))\n\t{\n\t\treturn fallbackShader_${SHAD_NAME_LOW};\n\t}\n")
				ELSE(SHADER_FUNCTION)
					SET(SHADER_FUNCTION "const char* GetFallbackShader(const char *name)\n{\n\tif(!Q_stricmp(name,\"${SHAD_NAME_LOW}\"))\n\t{\n\t\treturn fallbackShader_${SHAD_NAME_LOW};\n\t}\n")
				ENDIF(SHADER_FUNCTION)
				SET(SHADER_SOURCES "${SHADER_SOURCES}const char *fallbackShader_${SHAD_NAME_LOW} =\n${OUT_RES}\n")
			ENDFOREACH(SHAD)
			SET(SHADER_FUNCTION "${SHADER_FUNCTION}\treturn NULL;\n}")
			FILE(WRITE "${SHADER_OUTPUT_FILE}" "${SHADER_SOURCES}${SHADER_FUNCTION}")
			LIST(APPEND RENDERER2_FILES "${SHADER_OUTPUT_FILE}")
		else(SED_EXECUTABLE)
			MESSAGE(FATAL_ERROR "The fallbackshader source file was not created due to \"sed\" missing. The build would fail. :(")
		endif(SED_EXECUTABLE)
		
		# increased default hunkmegs value
		add_definitions(-DFEATURE_INC_HUNKMEGS)
		add_library(renderer_opengl2_${ARCH} ${REND_LIBTYPE} ${RENDERER2_FILES} ${RENDERER_COMMON})
		if(BUNDLED_SDL)
			add_dependencies(renderer_opengl2_${ARCH} bundled_sdl)
		endif(BUNDLED_SDL)
		if(BUNDLED_GLEW)
				add_dependencies(renderer_opengl2_${ARCH} bundled_glew)
		endif(BUNDLED_GLEW)
		if(BUNDLED_JPEG)
			add_dependencies(renderer_opengl2_${ARCH} bundled_jpeg)
		endif(BUNDLED_JPEG)
		target_link_libraries(renderer_opengl2_${ARCH} ${RENDERER_LIBRARIES} ${SDL_LIBRARIES})
		set_target_properties(renderer_opengl2_${ARCH}
			PROPERTIES COMPILE_DEFINITIONS "FEATURE_RENDERER2"
		)
		install(TARGETS renderer_opengl2_${ARCH}
			LIBRARY DESTINATION "${INSTALL_DEFAULT_BINDIR}"
			ARCHIVE DESTINATION "${INSTALL_DEFAULT_BINDIR}"
		)
		if(NOT RENDERER_DYNAMIC)
			list(APPEND CLIENT_LIBRARIES renderer_opengl2_${ARCH})
		endif()
	endif(FEATURE_RENDERER2)
	
	if(FEATURE_WINDOWS_CONSOLE AND WIN32)
		add_executable(etl WIN32 ${COMMON_SRC} ${ZLIB_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
	else(FEATURE_WINDOWS_CONSOLE AND WIN32)
		add_executable(etl ${COMMON_SRC} ${ZLIB_SRC} ${CLIENT_SRC} ${PLATFORM_SRC} ${PLATFORM_CLIENT_SRC})
	endif(FEATURE_WINDOWS_CONSOLE AND WIN32)
	
	if(BUNDLED_SDL)
		add_dependencies(etl bundled_sdl)
	endif(BUNDLED_SDL)
	if(BUNDLED_CURL)
		add_dependencies(etl bundled_curl)
	endif(BUNDLED_CURL)
	if(BUNDLED_OGG_VORBIS)
		add_dependencies(etl bundled_ogg bundled_ogg_vorbis bundled_ogg_vorbis_file)
	endif(BUNDLED_OGG_VORBIS)
	
	target_link_libraries(etl
		${OS_LIBRARIES} # Has to go after cURL and SDL
		${CLIENT_LIBRARIES}
		${SDL_LIBRARIES}
	)

	install(TARGETS etl
		RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}")
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	if(FEATURE_WINDOWS_CONSOLE AND WIN32)
		add_executable(etlded WIN32 ${COMMON_SRC} ${ZLIB_SRC} ${SERVER_SRC} ${PLATFORM_SRC} ${PLATFORM_SERVER_SRC})
	else(FEATURE_WINDOWS_CONSOLE AND WIN32)
		add_executable(etlded ${COMMON_SRC} ${ZLIB_SRC} ${SERVER_SRC} ${PLATFORM_SRC} ${PLATFORM_SERVER_SRC})
	endif(FEATURE_WINDOWS_CONSOLE AND WIN32)
	target_link_libraries(etlded ${OS_LIBRARIES} ${SERVER_LIBRARIES})
	set_target_properties(etlded PROPERTIES COMPILE_DEFINITIONS "DEDICATED")
	install(TARGETS etlded RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}")
endif(BUILD_SERVER)

if(BUILD_MOD)
	#
	# cgame
	#
	add_library(cgame${LIB_SUFFIX}${ARCH} MODULE ${CGAME_SRC})
	set_target_properties(cgame${LIB_SUFFIX}${ARCH}
		PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL"
		PREFIX ""
		LIBRARY_OUTPUT_DIRECTORY "legacy"
		OSX_ARCHITECTURES "i386;x86_64"
	)

	#
	# qagame
	#
	add_library(qagame${LIB_SUFFIX}${ARCH} MODULE ${QAGAME_SRC})
	if(FEATURE_LUA)
		if(BUNDLED_LUA)
			add_dependencies(qagame${LIB_SUFFIX}${ARCH} bundled_lua)
		endif(BUNDLED_LUA)
		target_link_libraries(qagame${LIB_SUFFIX}${ARCH} ${MOD_LIBRARIES})
	endif(FEATURE_LUA)

	set_target_properties(qagame${LIB_SUFFIX}${ARCH}
		PROPERTIES COMPILE_DEFINITIONS "GAMEDLL"
		PREFIX ""
		LIBRARY_OUTPUT_DIRECTORY "legacy"
	)

	#
	# ui
	#
	add_library(ui${LIB_SUFFIX}${ARCH} MODULE ${UI_SRC})
	set_target_properties(ui${LIB_SUFFIX}${ARCH}
		PROPERTIES
		PREFIX ""
		LIBRARY_OUTPUT_DIRECTORY "legacy"
		OSX_ARCHITECTURES "i386;x86_64"
	)

	# install bins of cgame, ui and qgame 
	install(TARGETS cgame${LIB_SUFFIX}${ARCH} qagame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
		LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
		ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
	)

	#
	# etl_bin.pk3
	#
	if(BUILD_MOD_PK3)
		add_custom_target(mod_pk3 ALL DEPENDS legacy/etl_bin_${ETL_CMAKE_VERSION_SHORT}.pk3)
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/legacy/etl_bin_${ETL_CMAKE_VERSION_SHORT}.pk3
			COMMAND ${ZIP_EXECUTABLE} etl_bin_${ETL_CMAKE_VERSION_SHORT}.pk3 cgame${LIB_SUFFIX}${ARCH}* ui${LIB_SUFFIX}${ARCH}*
			DEPENDS cgame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy/
		)

		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/legacy/etl_bin_${ETL_CMAKE_VERSION_SHORT}.pk3
			DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
		)
	endif()
endif(BUILD_MOD)

if(BUILD_PAK3_PK3)
	add_custom_target(
		pak3_pk3 ALL
		COMMAND ${ZIP_EXECUTABLE} -q -r ${CMAKE_CURRENT_BINARY_DIR}/legacy/pak3_${ETL_CMAKE_VERSION_SHORT}.pk3 *
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/etmain/
	)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/legacy/pak3_${ETL_CMAKE_VERSION_SHORT}.pk3
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
	)
endif(BUILD_PAK3_PK3)

if(INSTALL_OMNIBOT AND BUILD_MOD)
	message(STATUS "Installing Omni-Bot")
	# Note: used archive (16MB) doesn't contain incomplete nav- and other unwanted files
	if(UNIX)
		set(ETLEGACY_OMNIBOT_ARCHIVE "omnibot-linux-latest.tar.gz")
	elseif(WIN32 AND UNZIP_EXECUTABLE)
		set(ETLEGACY_OMNIBOT_ARCHIVE "omnibot-windows-latest.zip")
	endif()

	set(ETLEGACY_OMNIBOT_DL_URL "http://mirror.etlegacy.com/omnibot/${ETLEGACY_OMNIBOT_ARCHIVE}")

	#file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy")
	
	message(STATUS "Downloading Omni-Bot archive to ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}")

	if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}")
		file(DOWNLOAD
			${ETLEGACY_OMNIBOT_DL_URL}
			"${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}"
			SHOW_PROGRESS TIMEOUT 30
		)
	endif()

	message(STATUS "Extracting Omni-Bot to ${CMAKE_CURRENT_BINARY_DIR}/legacy/omni-bot")
	if(UNIX)
		execute_process(
			COMMAND tar -xf ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
		)
	elseif(WIN32)
		execute_process(
			COMMAND ${UNZIP_EXECUTABLE} -u ${CMAKE_CURRENT_BINARY_DIR}/legacy/${ETLEGACY_OMNIBOT_ARCHIVE}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy
		)
	endif()

	message(STATUS "Adding Omni-Bot to installer scripts")
	install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/legacy/omni-bot/"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy/omni-bot"
	)
	
	# ensure unique permissions and bot user path has write permission set
	if(UNIX)
		execute_process(
			COMMAND chmod -R 644 "${INSTALL_DEFAULT_MODDIR}/legacy/omni-bot/"
			WORKING_DIRECTORY "${INSTALL_DEFAULT_MODDIR}/legacy/omni-bot"
		)
	endif(UNIX)
	
endif(INSTALL_OMNIBOT AND BUILD_MOD)

#-----------------------------------------------------------------
# Installer/Package generation
#-----------------------------------------------------------------

# misc/etmain/ adds
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/misc/etmain/"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
)

# misc adds
if(INSTALL_OMNIBOT)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etl_bot.sh"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etlded_bot.sh"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
endif(INSTALL_OMNIBOT)

# other adds
if(UNIX)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etl.svg"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps"
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etlegacy.desktop"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications"
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etlegacy.xml"
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/mime/packages"
	)
else(UNIX)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etl.ico"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
	)
endif(UNIX)

# project adds
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}"
)

# copy required genuine files
if(ET_FS_BASEPATH AND INSTALL_DEFAULT_BASEDIR)
	message(STATUS "Installing genuine ET files")

	install(FILES "${ET_FS_BASEPATH}/etmain/mp_bin.pk3"
		DESTINATION "${INSTALL_DEFAULT_BASEDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	)

	install(FILES "${ET_FS_BASEPATH}/etmain/pak0.pk3"
		DESTINATION "${INSTALL_DEFAULT_BASEDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	)

	install(FILES "${ET_FS_BASEPATH}/etmain/pak1.pk3"
		DESTINATION "${INSTALL_DEFAULT_BASEDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	)

	install(FILES "${ET_FS_BASEPATH}/etmain/pak2.pk3"
		DESTINATION "${INSTALL_DEFAULT_BASEDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	)

	# personal data (owner only)
	install(FILES "${ET_FS_BASEPATH}/etmain/video/etintro.roq"
		DESTINATION "${INSTALL_DEFAULT_BASEDIR}/etmain/video"
		PERMISSIONS OWNER_WRITE OWNER_READ
	)
	
	if(ET_KEY)
		install(FILES "${ET_FS_BASEPATH}/etmain/etkey"
			DESTINATION "${INSTALL_DEFAULT_BASEDIR}/etmain"
			PERMISSIONS OWNER_WRITE OWNER_READ
		)
	endif(ET_KEY)
elseif(NOT ET_FS_BASEPATH AND INSTALL_DEFAULT_BASEDIR)
	message(STATUS "***********************************************************")
	message(STATUS "Genuine ET files are not copied - ET: Legacy won't start !!!")
	message(STATUS "In order to start the game copy")
	message(STATUS "mp_bin.pk3, pak0.pk3, pak1.pk3 and pak2.pk3")
	message(STATUS "to ${INSTALL_DEFAULT_BASEDIR}/etmain")
	message(STATUS "***********************************************************")
endif()

# Uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/Uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/Uninstall.cmake"
	IMMEDIATE @ONLY
)
add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Uninstall.cmake
)

# Packaging support
# TODO: move this to include(EtlegacyCPack)
# CPack general configuration
set(CPACK_PACKAGE_NAME				"etlegacy")
set(CPACK_BUNDLE_NAME				"etlegacy")
set(CPACK_PACKAGE_FILE_NAME			"etlegacy")
set(CPACK_BUNDLE_STARTUP_COMMAND		"etl")
set(CPACK_PACKAGE_ICON				"${CMAKE_SOURCE_DIR}/misc/etl.icns")
set(CPACK_BUNDLE_ICON				"${CMAKE_SOURCE_DIR}/misc/etl.icns")
set(CPACK_BUNDLE_PLIST				"${CMAKE_SOURCE_DIR}/misc/Info.plist")
set(CPACK_PACKAGE_CONTACT			"mail@etlegacy.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY		"ET: Legacy is an online FPS game compatible with Wolfenstein: Enemy Territory 2.60b.")
set(CPACK_PACKAGE_DESCRIPTION			${CPACK_PACKAGE_DESCRIPTION_SUMMARY}) # TODO: expand
set(CPACK_PACKAGE_DESCRIPTION_FILE		"${CMAKE_SOURCE_DIR}/README.md")
#set(CPACK_RESOURCE_FILE_LICENSE		"${CMAKE_SOURCE_DIR}/COPYING.txt") # FIXME: breaks bundle generator
set(CPACK_PACKAGE_VERSION_MAJOR			${ETLEGACY_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR			${ETLEGACY_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH			${ETLEGACY_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION			${ETLEGACY_VERSION})
set(CPACK_PACKAGE_FILE_NAME			"${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${ARCH}")
set(CPACK_PROJECT_CONFIG_FILE			"${CMAKE_CURRENT_BINARY_DIR}/CPackOptions.cmake")

# CPack generator-specific configuration
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackOptions.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/CPackOptions.cmake"
	IMMEDIATE @ONLY
)

include(CPack) # Has to be included after the package configuration!
