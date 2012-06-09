#------------------------------------------------------------------------
# ET:Legacy, http://www.etlegacy.com
# - Wolfenstein: Enemy Territory 2.60b compatible client/server
# - based on raedwulf-et: https://bitbucket.org/tcmreastwood/raedwulf-et
#------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.6)

project(ETLEGACY C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(ETLEGACY_VERSION_MAJOR  "2")
set(ETLEGACY_VERSION_MINOR  "7")
set(ETLEGACY_VERSION_PATCH  "0")
set(ETLEGACY_VERSION "${ETLEGACY_VERSION_MAJOR}.${ETLEGACY_VERSION_MINOR}${ETLEGACY_VERSION_PATCH}")

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
add_definitions(-DBOTLIB)

# Options
option(BUILD_SERVER	"Build the dedicated server executable"                                 OFF)
option(BUILD_CLIENT	"Build the client executable"                                           ON)
option(BUILD_MOD	"Build the mod libraries"                                               ON)

option(CROSS_COMPILE32  "Compile 32-bit version of ET:L (64bit is incompatible with 2.60b)"     ON)
option(BUNDLED_LIBS     "Use bundled libraries instead of the system ones."                     OFF)

# Optional features
option(SMP_SUPPORT 		"Enable SMP (multicore support) (client only)"			ON)
option(USE_CURL 		"Enable auto-download support using cURL (client only)" 	ON)
option(USE_CODEC_VORBIS 	"Enable OGG Vorbis support (client only)" 			OFF)
option(USE_OPENAL		"Enable OpenAL sound backend (client only)" 			OFF)
option(USE_FREETYPE		"Enable Freetype font library support (client only)" 		OFF)
option(GUIDMASTER_SUPPORT 	"Enable client registration with guidmaster (client only)" 	ON)
option(TRACKBASE_SUPPORT 	"Enable extended server statistics (server only)" 		ON)

#-----------------------------------------------------------------
# Platform-specific settings
#-----------------------------------------------------------------

if(UNIX AND CROSS_COMPILE32) # 32-bit build
    set(CMAKE_SYSTEM_PROCESSOR i386)
    set(CMAKE_SYSTEM_LIBRARY_PATH /lib32 /usr/lib32 /usr/local/lib32)
    set(FIND_LIBRARY_USE_LIB64_PATHS OFF)
    set(CMAKE_IGNORE_PATH /lib /usr/lib /usr/local/lib)
    set(CMAKE_C_FLAGS "-m32")
    set(CMAKE_EXE_LINKER_FLAGS "-m32")
    set(CMAKE_SHARED_LINKER_FLAGS "-m32")
    set(CMAKE_MODULE_LINKER_FLAGS "-m32")
endif(UNIX AND CROSS_COMPILE32)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Do not show CRT warnings
endif(MSVC)

if(UNIX)
        set(OS_LIBRARIES dl m)
elseif(WIN32)
        add_definitions(-DWINVER=0x501)
        set(OS_LIBRARIES ws2_32 psapi winmm)
endif()

# Get the system architecture
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i386")
        if(WIN32)
                set(ARCH "x86")
        else()
                set(ARCH "i386")
        endif()
elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
else()
        set(ARCH "none")
endif()

#-----------------------------------------------------------------
# Sources
#-----------------------------------------------------------------

FILE(GLOB COMMON_SRC
	"src/qcommon/*.c"
)

FILE(GLOB COMMON_SRC_REMOVE
	"src/qcommon/dl_main_curl.c"
	"src/qcommon/dl_main_stubs.c"
	"src/botlib/botlib_stub.c"
)
LIST(REMOVE_ITEM COMMON_SRC ${COMMON_SRC_REMOVE})

FILE(GLOB SERVER_SRC
	"src/server/*.c"
	"src/null/*.c"
	"src/botlib/be*.c"
	"src/botlib/l_*.c"
	"src/sys/sys_*.c"
	"src/sys/con_log.c"
	"src/sys/con_tty.c"
	"src/qcommon/dl_main_stubs.c"
)

FILE(GLOB CLIENT_SRC
	"src/server/*.c"
	"src/client/*.c"
	"src/botlib/be*.c"
	"src/botlib/l_*.c"
	"src/sys/sys_*.c"
	"src/sys/con_log.c"
	"src/sys/con_tty.c"
	"src/renderer/*.c"
	"src/sdl/*.c"
)

FILE(GLOB CGAME_SRC
	"src/cgame/*.c"
	"src/qcommon/q_math.c"
	"src/qcommon/q_shared.c"
	"src/ui/ui_shared.c"
	"src/game/bg_*.c"
)

FILE(GLOB QAGAME_SRC
	"src/game/*.c"
	"src/qcommon/q_math.c"
	"src/qcommon/q_shared.c"
)

FILE(GLOB UI_SRC
	"src/ui/*.c"
	"src/qcommon/q_math.c"
	"src/qcommon/q_shared.c"
	"src/game/bg_campaign.c"
	"src/game/bg_classes.c"
	"src/game/bg_misc.c"
)

#-----------------------------------------------------------------
# Client features
#-----------------------------------------------------------------
if(BUILD_CLIENT)
        Find_Package(OpenGL REQUIRED)

        if(NOT BUNDLED_LIBS)
                Find_Package(SDL32) # FindSDL doesn't detect 32bit lib when crosscompiling
                Find_Package(JPEG)
        endif(NOT BUNDLED_LIBS)
        
        if(USE_CURL)
                add_definitions(-DUSE_CURL)
                if(NOT BUNDLED_LIBS)
                        find_package(CURL)
                endif(NOT BUNDLED_LIBS)
                set(CLIENT_SRC ${CLIENT_SRC} "src/qcommon/dl_main_curl.c")
        else(USE_CURL)
                set(CLIENT_SRC ${CLIENT_SRC} "src/qcommon/dl_main_stubs.c")
        endif(USE_CURL)

        if(USE_FREETYPE)
                add_definitions(-DUSE_FREETYPE)
                find_package(Freetype)
        endif(USE_FREETYPE)

        if(USE_OPENAL)
                add_definitions(-DUSE_OPENAL)
                add_definitions(-DUSE_OPENAL_DLOPEN)
                find_package(OpenAL)
        endif(USE_OPENAL)

        if(USE_CODEC_VORBIS)
                add_definitions(-DUSE_CODEC_VORBIS)
                find_package(Vorbis)
        endif(USE_CODEC_VORBIS)

        if(SMP_SUPPORT)
                add_definitions(-DSMP)
        endif(SMP_SUPPORT)

        if(GUIDMASTER_SUPPORT)
                add_definitions(-DGUIDMASTER_SUPPORT)
        endif(GUIDMASTER_SUPPORT)

        # Use bundled libraries
        if(NOT SDL32_FOUND OR NOT JPEG_FOUND OR NOT CURL_FOUND AND USE_CURL OR BUNDLED_LIBS)
                if(EXISTS "${CMAKE_SOURCE_DIR}/libs/CMakeLists.txt")
                        message(STATUS "Using bundled libraries located at ${CMAKE_SOURCE_DIR}/libs")
                        add_subdirectory(libs) 
                else()
                        message(STATUS "=============================================================")
                        message(STATUS "Some of the required libraries were not found on your system!")
                        message(STATUS "=============================================================")
                        message(STATUS "Check the log and install the missing libraries.")
                        message(STATUS "You need the *multilib* package to crosscompile ET:L on a 64bit system.")
                        message(STATUS "Alternatively clone etlegacy repository and then run")
                        message(STATUS "        'git submodule init'" )
                        message(STATUS "        'git submodule update'")
                        message(STATUS "and enable BUNDLED_LIBS in CMake configuration.")
                        message(FATAL_ERROR "Build stopped because of missing libraries.")
                endif()
        endif(NOT SDL32_FOUND OR NOT JPEG_FOUND OR NOT CURL_FOUND AND USE_CURL OR BUNDLED_LIBS)
endif(BUILD_CLIENT)

#-----------------------------------------------------------------
# Server features
#-----------------------------------------------------------------
if(TRACKBASE_SUPPORT)
        add_definitions(-DTRACKBASE_SUPPORT)
endif(TRACKBASE_SUPPORT)

#-----------------------------------------------------------------
# Build
#-----------------------------------------------------------------
if(BUILD_CLIENT)
	add_executable(etl ${COMMON_SRC} ${CLIENT_SRC})
	include_directories(
                ${CURL_INCLUDE_DIR}
		${OPENGL_INCLUDE_DIR}
		${JPEG_INCLUDE_DIR}
                ${SDL32_INCLUDE_DIR}
                ${FREETYPE_INCLUDE_DIRS}
                ${OPENAL_INCLUDE_DIR}
                ${VORBIS_INCLUDE_DIR}
	)
	target_link_libraries(etl
		${OS_LIBRARIES}
                ${SDL32_LIBRARIES}
                ${OPENGL_LIBRARIES}
                ${CURL_LIBRARIES}
                ${JPEG_LIBRARIES}
                ${FREETYPE_LIBRARIES}
                ${OPENAL_LIBRARIES}
                ${VORBIS_FILE_LIBRARY} # This links only libvorbisfile
        )
        if(BUNDLED_LIBS)
                target_link_libraries(etl ${BUNDLED_LIBRARIES})
                add_dependencies(etl bundled_curl bundled_sdl bundled_jpeg)
        endif(BUNDLED_LIBS)
	set_target_properties(etl
                PROPERTIES COMPILE_DEFINITIONS "USE_ICON")
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	add_executable(etlded ${COMMON_SRC} ${SERVER_SRC})
	target_link_libraries(etlded
		${OS_LIBRARIES})
	set_target_properties(etlded
                PROPERTIES COMPILE_DEFINITIONS "DEDICATED")
endif(BUILD_SERVER)

if(BUILD_MOD)
        # cgame
	add_library(cgame.mp.${ARCH} SHARED ${CGAME_SRC})
	set_target_properties(cgame.mp.${ARCH}
		PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL"
			PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY "etmain")
        # qagame
	add_library(qagame.mp.${ARCH} SHARED ${QAGAME_SRC})
	set_target_properties(qagame.mp.${ARCH}
		PROPERTIES COMPILE_DEFINITIONS "GAMEDLL"
			PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY "etmain")
        # ui
	add_library(ui.mp.${ARCH} SHARED ${UI_SRC})
	set_target_properties(ui.mp.${ARCH}
		PROPERTIES
			PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY "etmain")
endif(BUILD_MOD)
