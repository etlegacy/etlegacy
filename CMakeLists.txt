#------------------------------------------------------------------------
# ET:Legacy, http://www.etlegacy.com
# - Wolfenstein: Enemy Territory 2.60b compatible client/server
# - based on raedwulf-et: https://bitbucket.org/tcmreastwood/raedwulf-et
#
# Please use TABs to indent.
#
# TODO/FIXME
# - make uninstall doesn't delete pathes ('etlegacy' & subfolders - test case: INSTALL_DEFAULT_ dirs are equal)
#------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.6)

project(ETLEGACY C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(Build)

set(ETLEGACY_VERSION_MAJOR  "2")
set(ETLEGACY_VERSION_MINOR  "7")
set(ETLEGACY_VERSION_PATCH  "0")
set(ETLEGACY_VERSION "${ETLEGACY_VERSION_MAJOR}.${ETLEGACY_VERSION_MINOR}${ETLEGACY_VERSION_PATCH}")
# Used to store real system processor when we overwrite CMAKE_SYSTEM_PROCESSOR for cross-compile builds
# See IS_64_BIT_BUILD_SYSTEM
set(ETLEGACY_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# this is hardly used .. we may delete that
add_definitions(-DBOTLIB)

# Options
option(BUILD_SERVER		"Build the dedicated server executable"				ON)
option(BUILD_CLIENT		"Build the client executable"						ON)
option(BUILD_MOD		"Build the mod libraries"							ON)
option(BUILD_MOD_PK3	"Pack the mod libraries into etl_bin.pk3"			OFF)
option(BUILD_PAK3_PK3	"Pack updated game scripts into pak3.pk3"			OFF)
option(BUNDLED_LIBS		"Use bundled libraries instead of the system ones"	OFF)

# no support for 32 bit binaries on OpenBSD/amd64
if (CMAKE_SYSTEM MATCHES "OpenBSD*")
	option(CROSS_COMPILE32 "Compile 32-bit version of ET:L (64bit is incompatible with 2.60b)" OFF)
else()
	option(CROSS_COMPILE32 "Compile 32-bit version of ET:L (64bit is incompatible with 2.60b)" ON)
endif()

# Installation options
set(INSTALL_DEFAULT_BASEDIR	""					CACHE STRING "Should be CMAKE_INSTALL_PREFIX + INSTALL_DEFAULT_MODDIR")
set(INSTALL_DEFAULT_BINDIR	"bin"				CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")
set(INSTALL_DEFAULT_MODDIR	"share/etlegacy"	CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")

if(INSTALL_DEFAULT_BASEDIR)
	add_definitions(-DDEFAULT_BASEDIR=\"${INSTALL_DEFAULT_BASEDIR}\")
endif(INSTALL_DEFAULT_BASEDIR)

# Optional features
if (CMAKE_SYSTEM MATCHES "Darwin")
	option(SMP_SUPPORT		"Enable SMP (multicore support) (client)"			OFF)
else()
	option(SMP_SUPPORT		"Enable SMP (multicore support) (client)"			ON)
endif()
option(USE_CURL				"Enable auto-download support using cURL (client)"	ON)
option(USE_CODEC_VORBIS		"Enable OGG Vorbis support (client)"				OFF)
option(USE_OPENAL			"Enable OpenAL sound backend (client) [BROKEN]"		OFF)
option(USE_FREETYPE			"Enable Freetype font library support (client)"		OFF)
option(TRACKBASE_SUPPORT	"Enable extended server statistics (server)"		ON)

if((WIN32 OR UNIX) AND NOT CMAKE_SYSTEM MATCHES "Darwin")
	option(BUILD_OMNIBOTS	"Enable omnibot build in mod code (Win & Linux server only!)"	OFF)
	option(INSTALL_OMNIBOTS	"Install Omni-Bot - LINUX only"									OFF)
endif()

option(BUILD_MV_SUPPORT		"Compile the mod files with multiview support (broken?!)"		OFF)

set(ET_FS_BASEPATH			""	CACHE STRING "Copy required genuine ET files from ET_FS_BASEPATH")
option(ET_KEY				"Copy etkey file (requires ET_FS_BASEPATH set)"					OFF)

#-----------------------------------------------------------------
# Platform-specific settings
#-----------------------------------------------------------------

if(UNIX AND CROSS_COMPILE32) # 32-bit build
	message(STATUS "System: ${ETLEGACY_SYSTEM_PROCESSOR}")

	set(CMAKE_SYSTEM_PROCESSOR i386)
	message(STATUS "Forcing ${CMAKE_SYSTEM_PROCESSOR} to cross compile 32bit")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
	set(CMAKE_C_FLAGS "-m32")
	set(CMAKE_EXE_LINKER_FLAGS "-m32")
	set(CMAKE_SHARED_LINKER_FLAGS "-m32")
	set(CMAKE_MODULE_LINKER_FLAGS "-m32")
endif(UNIX AND CROSS_COMPILE32)

if(UNIX)
	if(CMAKE_SYSTEM MATCHES "OpenBSD*")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.obsd.")
	elseif(CMAKE_SYSTEM MATCHES "Darwin")
		set(OS_LIBRARIES dl m)
		set(CMAKE_EXE_LINKER_FLAGS "-lobjc -framework Cocoa -framework IOKit -framework CoreFoundation")
		if(BUILD_CLIENT)
			# TODO: -lSDLmain belongs to FindSDL32.cmake
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Quartz -framework AudioUnit -framework Carbon -lSDLmain")
		endif()
		set(LIB_SUFFIX "_mac")
		set(CMAKE_SHARED_MODULE_SUFFIX "")
		add_definitions("-DMACOS_X")
	else()
		set(OS_LIBRARIES dl m)
		set(LIB_SUFFIX ".mp.")
	endif()
	if(NOT MSYS)
		include(CheckCCompilerFlag)
		check_c_compiler_flag("-fvisibility=hidden" SUPPORT_VISIBILITY)
		if(SUPPORT_VISIBILITY)
			message(STATUS "Compiling with -fvisibility=hidden")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
		endif(SUPPORT_VISIBILITY)
	endif()
elseif(WIN32)
	add_definitions(-DWINVER=0x501)
	set(OS_LIBRARIES ws2_32 psapi winmm)
	set(LIB_SUFFIX "_mp_")
	if(MSVC)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Do not show CRT warnings
	endif(MSVC)
endif()

# Get the system architecture
if(NOT CMAKE_SYSTEM MATCHES "Darwin")
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
endif(NOT CMAKE_SYSTEM MATCHES "Darwin")

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

# Platform specific code for server and client
if(UNIX)
	if(CMAKE_SYSTEM MATCHES "Darwin")
		LIST(APPEND PLATFORM_SRC "src/sys/sys_osx.m")
		SET_SOURCE_FILES_PROPERTIES("src/sys/sys_osx.m" PROPERTIES LANGUAGE C)
	endif(CMAKE_SYSTEM MATCHES "Darwin")

	LIST(APPEND PLATFORM_SRC "src/sys/sys_unix.c")
	LIST(APPEND PLATFORM_SRC "src/sys/con_tty.c")
elseif(WIN32)
	LIST(APPEND PLATFORM_SRC "src/sys/sys_win32.c")
	LIST(APPEND PLATFORM_SRC "src/sys/con_win32.c")
endif()

FILE(GLOB SERVER_SRC
	"src/server/*.c"
	"src/null/*.c"
	"src/botlib/be*.c"
	"src/botlib/l_*.c"
	"src/sys/sys_main.c"
	"src/sys/con_log.c"
	"src/qcommon/dl_main_stubs.c"
)

FILE(GLOB CLIENT_SRC
	"src/server/*.c"
	"src/client/*.c"
	"src/botlib/be*.c"
	"src/botlib/l_*.c"
	"src/sys/sys_main.c"
	"src/sys/con_log.c"
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
	message(STATUS "Libraries rescanned")
endif(NOT OLD_CROSS_COMPILE32 STREQUAL CROSS_COMPILE32)

#-----------------------------------------------------------------
# Client features
#-----------------------------------------------------------------
if(BUILD_CLIENT)
	Find_Package(OpenGL REQUIRED)

	if(NOT BUNDLED_LIBS)
		Find_Package(SDL32) # FindSDL doesn't detect 32bit lib when crosscompiling
		Find_Package(JPEG 8)

		# Check for libjpeg v8
		include(CheckFunctionExists)
		set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
		set(CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARY})
		# FIXME: function is checked, but HAVE_JPEG_MEM_SRC is empty. Why?
		check_function_exists("jpeg_mem_src" HAVE_JPEG_MEM_SRC)
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
	add_executable(etl ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SRC})
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
		RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}")
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	add_executable(etlded ${COMMON_SRC} ${SERVER_SRC} ${PLATFORM_SRC})
	target_link_libraries(etlded ${OS_LIBRARIES})
	set_target_properties(etlded
		PROPERTIES COMPILE_DEFINITIONS "DEDICATED")
	install(TARGETS etlded
		RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}")
endif(BUILD_SERVER)

if(BUILD_MOD)
	if(BUILD_MV_SUPPORT)
		add_definitions(-DMV_SUPPORT)
	endif(BUILD_MV_SUPPORT)

	#
	# cgame
	#
	add_library(cgame${LIB_SUFFIX}${ARCH} MODULE ${CGAME_SRC})
	set_target_properties(cgame${LIB_SUFFIX}${ARCH}
		PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL"
		PREFIX ""
		LIBRARY_OUTPUT_DIRECTORY "legacy"
	)

	#
	# qagame
	#
	if(BUILD_OMNIBOTS)
		# 32bit WIN & Linux only
		# note: 'Get the system architecture' above sets ARCH for cross compile
		if(("${ARCH}" STREQUAL "i386" OR "${ARCH}" STREQUAL "x86") AND NOT CMAKE_SYSTEM MATCHES "Darwin")
			add_definitions(-DOMNIBOTS)
			# TODO: move to Platform-specific settings?
			set(CMAKE_CXX_FLAGS "-m32")

			LIST(APPEND QAGAME_SRC "src/game/g_etbot_interface.cpp")
			LIST(APPEND QAGAME_SRC "src/Omnibot/Common/BotLoadLibrary.cpp")

			if("${ETLEGACY_SYSTEM_PROCESSOR}" STREQUAL "i686" OR "${ETLEGACY_SYSTEM_PROCESSOR}" STREQUAL "i386" OR "${ETLEGACY_SYSTEM_PROCESSOR}" STREQUAL "x86")
				#message(STATUS "omni-bot 32bit on 32bit")
			else()
				message(STATUS "omni-bot BotLoadLibrary includes cstdio")
				add_definitions(-DIS_64_BIT_BUILD_SYSTEM)
			endif()
		else()
			message(STATUS "Warning: omni-bot support is 32bit Win & Linux only - build skipped")
		endif()
	endif(BUILD_OMNIBOTS)
		    
	add_library(qagame${LIB_SUFFIX}${ARCH} MODULE ${QAGAME_SRC})
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
	)
                        
	# install qgame only
	install(TARGETS qagame${LIB_SUFFIX}${ARCH}
		LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
		ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
	)
                
	#install(TARGETS cgame${LIB_SUFFIX}${ARCH} qagame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
	#        LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
	#        ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain")
 
	#
	# etl_bin.pk3
	#
	if(BUILD_MOD_PK3)
		add_custom_target(mod_pk3 ALL DEPENDS legacy/etl_bin.pk3)
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/legacy/etl_bin.pk3
			COMMAND zip etl_bin.pk3 cgame${LIB_SUFFIX}${ARCH}* ui${LIB_SUFFIX}${ARCH}*
			DEPENDS cgame${LIB_SUFFIX}${ARCH} ui${LIB_SUFFIX}${ARCH}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/legacy/
		)

		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/legacy/etl_bin.pk3
			DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
		)
	endif()
endif(BUILD_MOD)

if(BUILD_PAK3_PK3)
	add_custom_target(pak3_pk3 ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/legacy/pak3.pk3)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/legacy/pak3.pk3
		COMMAND zip -r ${CMAKE_CURRENT_BINARY_DIR}/legacy/pak3.pk3 *
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/etmain/
	)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/legacy/pak3.pk3
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy"
	)
endif(BUILD_PAK3_PK3)

if(INSTALL_OMNIBOTS)
	if(UNIX AND NOT CMAKE_SYSTEM MATCHES "Darwin")
		message(STATUS "Installing Omni-Bot")

		# Note: used zip (16MB) doesn't contain windows, incomplete nav- and other unwanted files
		# FIXME: change to tar or rar ...
		set(ETLEGACY_OMNIBOT_L_DL_URL "http://mirror.etlegacy.com/omnibot/OmniBot_ETLegacy_Linux_r2497.zip")

		message(STATUS "Downloading Omni-Bot to ${CMAKE_CURRENT_BINARY_DIR}/omnibot.zip")
		file(DOWNLOAD ${ETLEGACY_OMNIBOT_L_DL_URL} "${CMAKE_CURRENT_BINARY_DIR}/omnibot.zip")

		message(STATUS "Unzipping Omni-Bot to ${CMAKE_CURRENT_BINARY_DIR}/omnibot")
		execute_process(
			COMMAND unzip -o -q ${CMAKE_CURRENT_BINARY_DIR}/omnibot.zip -domnibot
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)

		message(STATUS "Adding Omni-Bot to installer scripts")
			install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/omnibot/"
			DESTINATION "${INSTALL_DEFAULT_MODDIR}/legacy/omni-bot"
		)
	else()
		message(STATUS "Installing Omni-Bot is Linux only")
	endif(UNIX AND NOT CMAKE_SYSTEM MATCHES "Darwin")
endif(INSTALL_OMNIBOTS)

#-----------------------------------------------------------------
# Installer/Package generation
#-----------------------------------------------------------------

# install adds ...

# misc/etmain/ adds
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/misc/etmain/"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
)

# misc adds
if(INSTALL_OMNIBOTS)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etl_bot.sh"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE
	)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etlded_bot.sh"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE
	)
endif(INSTALL_OMNIBOTS)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/misc/etl.ico"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}"
)

# project adds
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt"
	DESTINATION "${INSTALL_DEFAULT_MODDIR}"
)

# copy required genuine files
if (ET_FS_BASEPATH)
	message(STATUS "Installing genuine ET files")

	install(FILES "${ET_FS_BASEPATH}/etmain/mp_bin.pk3"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ
	)

	install(FILES "${ET_FS_BASEPATH}/etmain/pak0.pk3"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ
	)

	install(FILES "${ET_FS_BASEPATH}/etmain/pak1.pk3"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ
	)

	install(FILES "${ET_FS_BASEPATH}/etmain/pak2.pk3"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
		PERMISSIONS OWNER_WRITE OWNER_READ
	)

	install(FILES "${ET_FS_BASEPATH}/etmain/video/etintro.roq"
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain/video"
		PERMISSIONS OWNER_WRITE OWNER_READ
	)

	if (ET_KEY)
		install(FILES "${ET_FS_BASEPATH}/etmain/etkey"
			DESTINATION "${INSTALL_DEFAULT_MODDIR}/etmain"
			PERMISSIONS OWNER_WRITE OWNER_READ
		)
	endif(ET_KEY)
else()
	message(STATUS "***********************************************************")
	message(STATUS "Genuine ET files are not copied - ET:Legacy won't start !!!")
	message(STATUS "In order to start the game copy")
	message(STATUS "mp_bin.pk3, pak0.pk3, pak1.pk3 and pak2.pk3")
	message(STATUS "to ${INSTALL_DEFAULT_MODDIR}/etmain")
	message(STATUS "***********************************************************")
endif(ET_FS_BASEPATH)

# Uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/Uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/Uninstall.cmake"
	IMMEDIATE @ONLY
)
add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/Uninstall.cmake
)

# CPack general configuration
set(CPACK_PACKAGE_NAME					"etlegacy")
set(CPACK_PACKAGE_CONTACT				"mail@etlegacy.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY	"ET: Legacy is an online FPS game compatible with Wolfenstein: Enemy Territory 2.60b.")
set(CPACK_PACKAGE_DESCRIPTION			${CPACK_PACKAGE_DESCRIPTION_SUMMARY}) # TODO: expand
set(CPACK_PACKAGE_DESCRIPTION_FILE		"${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE			"${CMAKE_SOURCE_DIR}/COPYING.txt")
set(CPACK_PACKAGE_VERSION_MAJOR			${ETLEGACY_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR			${ETLEGACY_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH			${ETLEGACY_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION				${ETLEGACY_VERSION})
set(CPACK_PACKAGE_FILE_NAME				"${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${ARCH}")
set(CPACK_PROJECT_CONFIG_FILE			"${CMAKE_CURRENT_BINARY_DIR}/CPackOptions.cmake")

# CPack generator-specific configuration
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackOptions.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/CPackOptions.cmake"
	IMMEDIATE @ONLY
)

include(CPack) # Has to be included after the package configuration!
