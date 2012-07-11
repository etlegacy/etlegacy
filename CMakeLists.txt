#------------------------------------------------------------------------
# ET:Legacy, http://www.etlegacy.com
# - Wolfenstein: Enemy Territory 2.60b compatible client/server
# - based on raedwulf-et: https://bitbucket.org/tcmreastwood/raedwulf-et
#------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.6)

project(ETLEGACY C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(Build)

set(ETLEGACY_VERSION_MAJOR  "2")
set(ETLEGACY_VERSION_MINOR  "7")
set(ETLEGACY_VERSION_PATCH  "0")
set(ETLEGACY_VERSION "${ETLEGACY_VERSION_MAJOR}.${ETLEGACY_VERSION_MINOR}${ETLEGACY_VERSION_PATCH}")

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
add_definitions(-DBOTLIB)

# Options
option(BUILD_SERVER     "Build the dedicated server executable"                                 ON)
option(BUILD_CLIENT     "Build the client executable"                                           ON)
option(BUILD_MOD        "Build the mod libraries"                                               ON)
option(BUILD_MOD_PK3    "Pack the mod libraries into mp_bin.pk3"                                OFF)

option(CROSS_COMPILE32  "Compile 32-bit version of ET:L (64bit is incompatible with 2.60b)"     ON)
option(BUNDLED_LIBS     "Use bundled libraries instead of the system ones."                     OFF)

# Optional features
option(SMP_SUPPORT              "Enable SMP (multicore support) (client)"                       ON)
option(USE_CURL                 "Enable auto-download support using cURL (client)"              ON)
option(USE_CODEC_VORBIS         "Enable OGG Vorbis support (client)"                            OFF)
option(USE_OPENAL               "Enable OpenAL sound backend (client) [BROKEN]"                 OFF)
option(USE_FREETYPE             "Enable Freetype font library support (client)"                 OFF)
option(TRACKBASE_SUPPORT        "Enable extended server statistics (server)"                    ON)

#-----------------------------------------------------------------
# Platform-specific settings
#-----------------------------------------------------------------

if(UNIX AND CROSS_COMPILE32) # 32-bit build
        set(CMAKE_SYSTEM_PROCESSOR i386)
        set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
        set(CMAKE_C_FLAGS "-m32")
        set(CMAKE_EXE_LINKER_FLAGS "-m32")
        set(CMAKE_SHARED_LINKER_FLAGS "-m32")
        set(CMAKE_MODULE_LINKER_FLAGS "-m32")
endif(UNIX AND CROSS_COMPILE32)

if(UNIX)
        set(OS_LIBRARIES dl m)
        set(LIB_SUFFIX ".mp.")
        if(NOT APPLE) # Linux and *BSD
                set(ETLEGACY_INSTALL_DIRECTORY "games/etlegacy") # FIXME: needs to be unset for CPack packages. How???
        else() # Mac OS X
                set(ETLEGACY_INSTALL_DIRECTORY "etlegacy") # TODO: correct this
        endif()
elseif(WIN32)
        add_definitions(-DWINVER=0x501)
        set(OS_LIBRARIES ws2_32 psapi winmm)
        set(LIB_SUFFIX "_mp_")
        set(ETLEGACY_INSTALL_DIRECTORY "etlegacy")
        if(MSVC)
                add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Do not show CRT warnings
        endif(MSVC)
endif()

# Get the system architecture
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

# If we change architecture we need to force rescan of libraries
if(NOT OLD_CROSS_COMPILE32 STREQUAL CROSS_COMPILE32)
        force_rescan_library(SDL32)
        force_rescan_library(CURL)
        force_rescan_library(JPEG)
        # TODO: recheck optional libs
        set(OLD_CROSS_COMPILE32 ${CROSS_COMPILE32} CACHE INTERNAL "Previous value for CROSS_COMPILE32")
endif(NOT OLD_CROSS_COMPILE32 STREQUAL CROSS_COMPILE32)

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
                        if(MINGW)
                                add_definitions(-DCURL_STATICLIB)
                        endif(MINGW)
                else(NOT BUNDLED_LIBS)
                        add_definitions(-DCURL_STATICLIB)
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

        # Use bundled libraries
        if(NOT SDL32_FOUND OR NOT JPEG_FOUND OR NOT CURL_FOUND AND USE_CURL OR BUNDLED_LIBS AND NOT MINGW)
                if(EXISTS "${CMAKE_SOURCE_DIR}/libs/CMakeLists.txt")
                        message(STATUS "Using bundled libraries located at ${CMAKE_SOURCE_DIR}/libs")
                        set(BUNDLED_LIBS ON)
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
        endif(NOT SDL32_FOUND OR NOT JPEG_FOUND OR NOT CURL_FOUND AND USE_CURL OR BUNDLED_LIBS AND NOT MINGW)
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
                ${OPENGL_INCLUDE_DIR}
                ${FREETYPE_INCLUDE_DIRS}
                ${OPENAL_INCLUDE_DIR}
                ${VORBIS_INCLUDE_DIR}
        )
        if(BUNDLED_LIBS)
                add_definitions(-DBUNDLED_LIBS)
                target_link_libraries(etl ${BUNDLED_LIBRARIES})
                include_directories(${BUNDLED_INCLUDE_DIRS})
                add_dependencies(etl bundled_curl bundled_sdl bundled_jpeg)
        else() # Using system libraries
                target_link_libraries(etl 
                        ${SDL32_LIBRARIES}
                        ${CURL_LIBRARIES}
                        ${JPEG_LIBRARIES}                
                )
                include_directories(
                        ${SDL32_INCLUDE_DIR}
                        ${CURL_INCLUDE_DIR}
                        ${JPEG_INCLUDE_DIR}                        
                )
        endif()
        target_link_libraries(etl
                ${OS_LIBRARIES} # Has to go after cURL and SDL
                ${OPENGL_LIBRARIES}
                ${FREETYPE_LIBRARIES}
                ${OPENAL_LIBRARIES}
                ${VORBIS_FILE_LIBRARY} # This links only libvorbisfile
        )
        set_target_properties(etl
                PROPERTIES COMPILE_DEFINITIONS "USE_ICON")
        install(TARGETS etl
                RUNTIME DESTINATION ${ETLEGACY_INSTALL_DIRECTORY})
endif(BUILD_CLIENT)

if(BUILD_SERVER)
        add_executable(etlded ${COMMON_SRC} ${SERVER_SRC})
        target_link_libraries(etlded ${OS_LIBRARIES})
        set_target_properties(etlded
                PROPERTIES COMPILE_DEFINITIONS "DEDICATED")
        install(TARGETS etlded
                RUNTIME DESTINATION ${ETLEGACY_INSTALL_DIRECTORY})
endif(BUILD_SERVER)

if(BUILD_MOD)
        # cgame
        add_library(cgame${LIB_SUFFIX}${ARCH} SHARED ${CGAME_SRC})
        set_target_properties(cgame${LIB_SUFFIX}${ARCH}
                PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL"
                        PREFIX ""
                        LIBRARY_OUTPUT_DIRECTORY "etmain")
        # qagame
        add_library(qagame${LIB_SUFFIX}${ARCH} SHARED ${QAGAME_SRC})
        set_target_properties(qagame${LIB_SUFFIX}${ARCH}
                PROPERTIES COMPILE_DEFINITIONS "GAMEDLL"
                        PREFIX ""
                        LIBRARY_OUTPUT_DIRECTORY "etmain")
        # ui
        add_library(ui${LIB_SUFFIX}${ARCH} SHARED ${UI_SRC})
        set_target_properties(ui${LIB_SUFFIX}${ARCH}
                PROPERTIES
                        PREFIX ""
                        LIBRARY_OUTPUT_DIRECTORY "etmain")
        install(TARGETS cgame${LIB_SUFFIX}${ARCH} qagame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
                LIBRARY DESTINATION "${ETLEGACY_INSTALL_DIRECTORY}/etmain"
                ARCHIVE DESTINATION "${ETLEGACY_INSTALL_DIRECTORY}/etmain"
        )

        # mp_bin.pk3
        if(BUILD_MOD_PK3)
                add_custom_target(mod_pk3 ALL DEPENDS etmain/mp_bin.pk3)
                add_custom_command(
                        OUTPUT etmain/mp_bin.pk3
                        COMMAND zip mp_bin.pk3 cgame${LIB_SUFFIX}${ARCH}* ui${LIB_SUFFIX}${ARCH}*
                        DEPENDS cgame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
                        WORKING_DIRECTORY etmain/
                )

                install(FILES ${CMAKE_CURRENT_BINARY_DIR}/etmain/mp_bin.pk3
                        DESTINATION ${ETLEGACY_INSTALL_DIRECTORY}/etmain)
        endif()
endif(BUILD_MOD)

#-----------------------------------------------------------------
# Installer/Package generation
#-----------------------------------------------------------------

# Uninstall target
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Uninstall.cmake"
        IMMEDIATE @ONLY
)
add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Uninstall.cmake
)

# Linux
if("${CMAKE_SYSTEM}" MATCHES "Linux")
        # Create symlinks (works only with 'make install') and add them as uninstall targets
        if(BUILD_CLIENT)
                install(CODE "
                        execute_process(
                                COMMAND \${CMAKE_COMMAND} -E remove \${CMAKE_INSTALL_PREFIX}/bin/etl
                                COMMAND \${CMAKE_COMMAND} -E create_symlink \"\${CMAKE_INSTALL_PREFIX}/games/etlegacy/etl\" \${CMAKE_INSTALL_PREFIX}/bin/etl
                        )
                        list(APPEND CMAKE_INSTALL_MANIFEST_FILES \${CMAKE_INSTALL_PREFIX}/bin/etl)
                ")
        endif(BUILD_CLIENT)
        if(BUILD_SERVER)
                install(CODE "
                        execute_process(
                                COMMAND \${CMAKE_COMMAND} -E remove \${CMAKE_INSTALL_PREFIX}/bin/etlded
                                COMMAND \${CMAKE_COMMAND} -E create_symlink \${CMAKE_INSTALL_PREFIX}/games/etlegacy/etlded \${CMAKE_INSTALL_PREFIX}/bin/etlded
                        )
                        list(APPEND CMAKE_INSTALL_MANIFEST_FILES \${CMAKE_INSTALL_PREFIX}/bin/etlded)
                ")
        endif(BUILD_SERVER) 
endif()

# CPack general configuration
set(CPACK_PACKAGE_NAME                  "etlegacy")
set(CPACK_PACKAGE_CONTACT               "mail@etlegacy.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "ET: Legacy is an online FPS game compatible with Wolfenstein: Enemy Territory 2.60b.")
set(CPACK_PACKAGE_DESCRIPTION           ${CPACK_PACKAGE_DESCRIPTION_SUMMARY}) # TODO: expand
set(CPACK_PACKAGE_DESCRIPTION_FILE      "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE         "${CMAKE_SOURCE_DIR}/COPYING.txt")
set(CPACK_PACKAGE_VERSION_MAJOR         ${ETLEGACY_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR         ${ETLEGACY_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH         ${ETLEGACY_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION               ${ETLEGACY_VERSION})
set(CPACK_PACKAGE_FILE_NAME             "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${ARCH}")
set(CPACK_PROJECT_CONFIG_FILE           "${CMAKE_CURRENT_BINARY_DIR}/CPackOptions.cmake")

# CPack generator-specific configuration
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackOptions.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CPackOptions.cmake"
        IMMEDIATE @ONLY
)

include(CPack) # Has to be included after the package configuration!