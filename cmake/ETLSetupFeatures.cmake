# If we change architecture we need to force rescan of libraries
if(NOT OLD_CROSS_COMPILE32 STREQUAL CROSS_COMPILE32)
	force_rescan_library(SDL32)
	force_rescan_library(CURL)
	force_rescan_library(JPEG)
	force_rescan_library(JPEGTURBO)
	# TODO: recheck optional libs
	set(OLD_CROSS_COMPILE32 ${CROSS_COMPILE32} CACHE INTERNAL "Previous value for CROSS_COMPILE32")
	message(STATUS "Libraries rescanned")
endif(NOT OLD_CROSS_COMPILE32 STREQUAL CROSS_COMPILE32)

#-----------------------------------------------------------------
# Client features
#-----------------------------------------------------------------
if(BUILD_CLIENT)
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
		
		find_package(OpenGL REQUIRED)
		list(APPEND RENDERER_LIBRARIES ${OPENGL_LIBRARIES})
		include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
	else() # FEATURE_RENDERER_GLES
    		list(APPEND RENDERER_LIBRARIES -lGLESv1_CM)
    		include_directories(SYSTEM /mnt/utmp/codeblocks/usr/include/gles)
	endif()

	if(NOT BUNDLED_SDL)
		find_package(SDL2 2.0.3 REQUIRED) # FindSDL doesn't detect 32bit lib when crosscompiling
		list(APPEND SDL_LIBRARIES ${SDL2_LIBRARY})
		include_directories(SYSTEM ${SDL2_INCLUDE_DIR})
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
		find_package(JPEGTURBO)
		if(JPEGTURBO_FOUND)
			list(APPEND RENDERER_LIBRARIES ${JPEG_LIBRARIES})
			include_directories(SYSTEM ${JPEG_INCLUDE_DIR})

			# Check for libjpeg-turbo v1.3
			include(CheckFunctionExists)
			set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
			set(CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARY})
			# FIXME: function is checked, but HAVE_JPEG_MEM_SRC is empty. Why?
			check_function_exists("jpeg_mem_src" HAVE_JPEG_MEM_SRC)
		else()
			find_package(JPEG 8 REQUIRED)
			list(APPEND RENDERER_LIBRARIES ${JPEG_LIBRARIES})
			include_directories(SYSTEM ${JPEG_INCLUDE_DIR})

			# Check for libjpeg v8
			include(CheckFunctionExists)
			set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
			set(CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARY})
			# FIXME: function is checked, but HAVE_JPEG_MEM_SRC is empty. Why?
			check_function_exists("jpeg_mem_src" HAVE_JPEG_MEM_SRC)
		endif()
	else()
		list(APPEND RENDERER_LIBRARIES ${JPEG_BUNDLED_LIBRARIES})
		include_directories(SYSTEM ${JPEG_BUNDLED_INCLUDE_DIR})
	endif()

	if(FEATURE_JANSSON)
		list(APPEND CLIENT_LIBRARIES ${BUNDLED_JANSSON_LIBRARY})
		include_directories(SYSTEM ${BUNDLED_JANSSON_INCLUDE_DIR})
		set(CLIENT_SRC ${CLIENT_SRC} "src/qcommon/json.c")
	else(FEATURE_JANSSON)
		set(CLIENT_SRC ${CLIENT_SRC} "src/qcommon/json_stubs.c")
	endif(FEATURE_JANSSON)

	if(FEATURE_GETTEXT)
		add_definitions(-DFEATURE_GETTEXT)
		FILE(GLOB GETTEXT_SRC
			"src/qcommon/i18n_main.cpp"
			"src/qcommon/i18n_findlocale.c"
			"src/qcommon/i18n_findlocale.h"
			"src/tinygettext/dictionary_manager.hpp"
			"src/tinygettext/file_system.hpp"
			"src/tinygettext/iconv.cpp"
			"src/tinygettext/plural_forms.hpp"
			"src/tinygettext/tinygettext.cpp"
			"src/tinygettext/tinygettext.hpp"
			"src/tinygettext/dictionary.cpp"
			"src/tinygettext/dictionary.hpp"
			"src/tinygettext/dictionary_manager.cpp"
			"src/tinygettext/iconv.hpp"
			"src/tinygettext/language.cpp"
			"src/tinygettext/language.hpp"
			"src/tinygettext/log.cpp"
			"src/tinygettext/log.hpp"
			"src/tinygettext/log_stream.hpp"
			"src/tinygettext/plural_forms.cpp"
			"src/tinygettext/po_parser.cpp"
			"src/tinygettext/po_parser.hpp"
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
		if(NOT BUNDLED_FREETYPE)
			find_package(Freetype REQUIRED)
			list(APPEND RENDERER_LIBRARIES ${FREETYPE_LIBRARIES})
			include_directories(SYSTEM ${FREETYPE_INCLUDE_DIRS})
		else()
			list(APPEND RENDERER_LIBRARIES ${BUNDLED_FREETYPE_LIBRARIES})
			include_directories(SYSTEM ${BUNDLED_FREETYPE_INCLUDE_DIR})
		endif()
		add_definitions(-DFEATURE_FREETYPE)
	endif(FEATURE_FREETYPE)

	if(FEATURE_OPENAL)
		if(NOT BUNDLED_OPENAL)
			find_package(OpenAL 1.14 REQUIRED)
			list(APPEND CLIENT_LIBRARIES ${OPENAL_LIBRARY})
			include_directories(SYSTEM ${OPENAL_INCLUDE_DIR})
			add_definitions(-DFEATURE_OPENAL_DLOPEN)
		else()
			list(APPEND CLIENT_LIBRARIES ${OPENAL_BUNDLED_LIBRARIES})
			include_directories(SYSTEM ${OPENAL_BUNDLED_INCLUDE_DIR})
		endif()
		add_definitions(-DFEATURE_OPENAL)
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

	if(FEATURE_THEORA)
		if(NOT BUNDLED_THEORA)
			find_package(Theora REQUIRED)
			list(APPEND CLIENT_LIBRARIES ${THEORA_LIBRARY})
			include_directories(SYSTEM ${THEORA_INCLUDE_DIR})
		else() # BUNDLED_THEORA
			list(APPEND CLIENT_LIBRARIES ${THEORA_BUNDLED_LIBRARIES})
			include_directories(SYSTEM ${THEORA_BUNDLED_INCLUDE_DIR})
		endif()
		add_definitions(-DFEATURE_THEORA)
	endif(FEATURE_THEORA)

	if(FEATURE_IRC_CLIENT)
		add_definitions(-DFEATURE_IRC_CLIENT)
		list(APPEND CLIENT_SRC ${IRC_CLIENT_FILES})
	endif(FEATURE_IRC_CLIENT)

	if(FEATURE_LIVEAUTH)
		add_definitions(-DFEATURE_LIVEAUTH)
	endif(FEATURE_LIVEAUTH)
endif(BUILD_CLIENT)

if(BUILD_CLIENT OR BUILD_SERVER)
	if(FEATURE_CURL)
		if(NOT BUNDLED_CURL)
			find_package(CURL REQUIRED)
			list(APPEND CLIENT_LIBRARIES ${CURL_LIBRARIES})
			list(APPEND SERVER_LIBRARIES ${CURL_LIBRARIES})
			include_directories(SYSTEM ${CURL_INCLUDE_DIR})
			if(MINGW)
				add_definitions(-DCURL_STATICLIB)
			endif(MINGW)
		else() # BUNDLED_CURL
			list(APPEND CLIENT_LIBRARIES ${CURL_BUNDLED_LIBRARY})
			list(APPEND SERVER_LIBRARIES ${CURL_BUNDLED_LIBRARY})
			include_directories(SYSTEM ${CURL_BUNDLED_INCLUDE_DIR})
			add_definitions(-DCURL_STATICLIB)
		endif()
		set(CLIENT_SRC ${CLIENT_SRC} "src/qcommon/dl_main_curl.c")
		set(SERVER_SRC ${SERVER_SRC} "src/qcommon/dl_main_curl.c")
	else(FEATURE_CURL)
		set(CLIENT_SRC ${CLIENT_SRC} "src/qcommon/dl_main_stubs.c")
		set(SERVER_SRC ${SERVER_SRC} "src/qcommon/dl_main_stubs.c")
	endif(FEATURE_CURL)

	if(FEATURE_DBMS)
		if(NOT BUNDLED_SQLITE3)
			find_package(SQLite3 REQUIRED)
			list(APPEND CLIENT_LIBRARIES ${SQLITE3_LIBRARY})
			list(APPEND SERVER_LIBRARIES ${SQLITE3_LIBRARY})
			include_directories(SYSTEM ${SQLITE3_INCLUDE_DIR})
		else() # BUNDLED_SQLITE3
			list(APPEND CLIENT_LIBRARIES ${SQLITE3_BUNDLED_LIBRARIES})
			list(APPEND SERVER_LIBRARIES ${SQLITE3_BUNDLED_LIBRARIES})
			include_directories(SYSTEM ${SQLITE3_BUNDLED_INCLUDE_DIR})
		endif()
		add_definitions(-DFEATURE_DBMS)
		FILE(GLOB DBMS_SRC
			"src/db/db_sql.h"
			"src/db/db_sqlite3.c"
			"src/db/db_sql_console.c"
		)
		set(CLIENT_SRC ${CLIENT_SRC} ${DBMS_SRC})
		set(SERVER_SRC ${SERVER_SRC} ${DBMS_SRC})
	endif(FEATURE_DBMS)

endif()

#-----------------------------------------------------------------
# Mod features
#-----------------------------------------------------------------
if(BUILD_MOD)
	if(FEATURE_MULTIVIEW)
		add_definitions(-DFEATURE_MULTIVIEW)
	endif(FEATURE_MULTIVIEW)

	if(FEATURE_RATING)
		add_definitions(-DFEATURE_RATING)
	endif(FEATURE_RATING)

	if(FEATURE_LUA)
		if(NOT BUNDLED_LUA)
			find_package(Lua 5.3 REQUIRED)
			list(APPEND MOD_LIBRARIES ${LUA_LIBRARIES})
			include_directories(SYSTEM ${LUA_INCLUDE_DIR})
		else() # BUNDLED_LUA
			list(APPEND MOD_LIBRARIES ${LUA_BUNDLED_LIBRARIES})
			include_directories(SYSTEM ${LUA_BUNDLED_INCLUDE_DIR})
			add_definitions(-DBUNDLED_LUA)
		endif()
		add_definitions(-DFEATURE_LUA)
	endif(FEATURE_LUA)

	if(FEATURE_OMNIBOT)
		LIST(APPEND QAGAME_SRC "src/game/g_etbot_interface.cpp")
		LIST(APPEND QAGAME_SRC "src/Omnibot/Common/BotLoadLibrary.cpp")
		add_definitions(-DFEATURE_OMNIBOT)
	endif(FEATURE_OMNIBOT)
endif(BUILD_MOD)

#-----------------------------------------------------------------
# Server/Common features
#-----------------------------------------------------------------
if(NOT BUNDLED_ZLIB)
	find_package(ZLIB 1.2.8 REQUIRED)
	list(APPEND CLIENT_LIBRARIES ${ZLIB_LIBRARIES})
	list(APPEND SERVER_LIBRARIES ${ZLIB_LIBRARIES})
	include_directories(SYSTEM ${ZLIB_INCLUDE_DIRS})
else()
	list(APPEND CLIENT_LIBRARIES ${ZLIB_BUNDLED_LIBRARIES})
	list(APPEND SERVER_LIBRARIES ${ZLIB_BUNDLED_LIBRARIES})
	include_directories(SYSTEM ${ZLIB_BUNDLED_INCLUDE_DIR})
endif()

if(NOT BUNDLED_MINIZIP)
	find_package(MiniZip)
	list(APPEND CLIENT_LIBRARIES ${MINIZIP_LIBRARIES})
	list(APPEND SERVER_LIBRARIES ${MINIZIP_LIBRARIES})
	include_directories(SYSTEM ${MINIZIP_INCLUDE_DIRS})
else()
	list(APPEND CLIENT_LIBRARIES ${MINIZIP_BUNDLED_LIBRARIES})
	list(APPEND SERVER_LIBRARIES ${MINIZIP_BUNDLED_LIBRARIES})
	include_directories(SYSTEM ${MINIZIP_BUNDLED_INCLUDE_DIR})
endif()

if(FEATURE_TRACKER)
	add_definitions(-DFEATURE_TRACKER)
endif(FEATURE_TRACKER)

if(FEATURE_ANTICHEAT)
	add_definitions(-DFEATURE_ANTICHEAT)
endif(FEATURE_ANTICHEAT)

if (FEATURE_RENDERER_GLES)
	add_definitions(-DFEATURE_RENDERER_GLES)
endif()

if(FEATURE_CURSES)
	find_package(Curses REQUIRED)
	set(CURSES_NEED_NCURSES 1) # Tells FindCurses that ncurses is required
	list(APPEND CLIENT_LIBRARIES ${CURSES_LIBRARIES})
	list(APPEND SERVER_LIBRARIES ${CURSES_LIBRARIES})
	include_directories(SYSTEM ${CURSES_INCLUDE_DIR})
	list(APPEND COMMON_SRC "src/sys/con_curses.c")
	add_definitions(-DFEATURE_CURSES)
endif(FEATURE_CURSES)


