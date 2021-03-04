#-----------------------------------------------------------------
# Setup Features
#-----------------------------------------------------------------

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

	if(NOT WIN32 AND NOT APPLE AND NOT ANDROID) # Dependency of GLEW and SDL_syswm.h
		find_package(X11 REQUIRED)
		include_directories(${X11_INCLUDE_DIR})
	endif()

	if(ARM)
		#check if we're running on Raspberry Pi
		MESSAGE("Looking for bcm_host.h")
		if(EXISTS "/opt/vc/include/bcm_host.h")
			MESSAGE("bcm_host.h found")
			set(BCMHOST found)
		else()
			MESSAGE("bcm_host.h not found")
		endif()

		if(DEFINED BCMHOST)
			set(COMMON_INCLUDE_DIRS
					"/opt/vc/include"
					"/opt/vc/include/interface/vcos/pthreads"
					"/opt/vc/include/interface/vmcs_host/linux"
					)
			include_directories(${COMMON_INCLUDE_DIRS})

			link_directories("/opt/vc/lib")

			LIST(APPEND CLIENT_LIBRARIES
					bcm_host
					pthread
					)
		endif()
	endif(ARM)

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

		# On 2.77 release the default usage of GLVND just caused issues as
		# libOpenGL was not installed on systems by default
		# cmake_policy(SET CMP0072 NEW) # use GLVND by default
		# Revert to using legacy libraries if available for now
		# FIXME: recheck before a new release
		if(CLIENT_GLVND)
			message(STATUS "Using GLVND instead of legacy GL library")
			set(OpenGL_GL_PREFERENCE GLVND)
		else()
			message(STATUS "Using legacy OpenGL instead of GLVND")
			set(OpenGL_GL_PREFERENCE LEGACY)
		endif ()
		find_package(OpenGL REQUIRED COMPONENTS OpenGL)
		list(APPEND RENDERER_LIBRARIES OpenGL::GL)
		include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
	else() # FEATURE_RENDERER_GLES
		find_package(GLES REQUIRED)
		list(APPEND RENDERER_LIBRARIES ${GLES_LIBRARY})
		include_directories(SYSTEM ${GLES_INCLUDE_DIR})
	endif()

	if(NOT BUNDLED_SDL)
		find_package(SDL2 2.0.8 REQUIRED)
		list(APPEND SDL_LIBRARIES ${SDL2_LIBRARY})
		include_directories(SYSTEM ${SDL2_INCLUDE_DIR})
	else() # BUNDLED_SDL
		if(MINGW AND WIN32)
			# We append the mingw32 library to the client list since SDL2Main requires it
			list(APPEND CLIENT_LIBRARIES mingw32)
		endif()
		list(APPEND SDL_LIBRARIES ${SDL32_BUNDLED_LIBRARIES})
		include_directories(SYSTEM ${SDL32_BUNDLED_INCLUDE_DIR})
		add_definitions(-DBUNDLED_SDL)
	endif()
	add_definitions(-DHAVE_SDL) # for tinygettext (always force SDL icons -> less dependancies)

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

	if(FEATURE_GETTEXT)
		add_definitions(-DFEATURE_GETTEXT)
		FILE(GLOB GETTEXT_SRC
			"src/qcommon/i18n_main.cpp"
			"src/qcommon/i18n_findlocale.c"
			"src/qcommon/i18n_findlocale.h"
			"src/tinygettext/tinygettext/dictionary.hpp"
			"src/tinygettext/tinygettext/dictionary_manager.hpp"
			"src/tinygettext/tinygettext/file_system.hpp"
			"src/tinygettext/tinygettext/iconv.hpp"
			"src/tinygettext/tinygettext/language.hpp"
			"src/tinygettext/tinygettext/log.hpp"
			"src/tinygettext/tinygettext/log_stream.hpp"
			"src/tinygettext/tinygettext/plural_forms.hpp"
			"src/tinygettext/tinygettext/po_parser.hpp"
			"src/tinygettext/tinygettext/tinygettext.hpp"
			"src/tinygettext/dictionary.cpp"
			"src/tinygettext/dictionary_manager.cpp"
			"src/tinygettext/iconv.cpp"
			"src/tinygettext/language.cpp"
			"src/tinygettext/log.cpp"
			"src/tinygettext/plural_forms.cpp"
			"src/tinygettext/po_parser.cpp"
			"src/tinygettext/tinygettext.cpp"
		)
		if(MSVC)
			list(APPEND GETTEXT_SRC "src/tinygettext/windows_file_system.cpp")
			list(APPEND GETTEXT_SRC "src/tinygettext/tinygettext/windows_file_system.hpp")
		else()
			list(APPEND GETTEXT_SRC "src/tinygettext/unix_file_system.cpp")
			list(APPEND GETTEXT_SRC "src/tinygettext/tinygettext/unix_file_system.hpp")
		endif()
		set(CLIENT_SRC ${CLIENT_SRC} ${GETTEXT_SRC})
	endif(FEATURE_GETTEXT)

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

	if(FEATURE_PNG)
		if(NOT BUNDLED_PNG)
			find_package(PNG REQUIRED)
			list(APPEND RENDERER_LIBRARIES ${LIBPNG_LIBRARIES})
			include_directories(SYSTEM ${LIBPNG_INCLUDE_DIRS})
		else()
			list(APPEND RENDERER_LIBRARIES ${BUNDLED_LIBPNG_LIBRARIES})
			include_directories(SYSTEM ${BUNDLED_LIBPNG_INCLUDE_DIR})
		endif()
		add_definitions(-DFEATURE_PNG)
	endif(FEATURE_PNG)

	if(FEATURE_OPENAL)
		if(NOT BUNDLED_OPENAL)
			find_package(OpenAL 1.14 REQUIRED)
			list(APPEND CLIENT_LIBRARIES ${OPENAL_LIBRARY})
			include_directories(SYSTEM ${OPENAL_INCLUDE_DIR})
			add_definitions(-DFEATURE_OPENAL_DLOPEN)
		else()
			list(APPEND CLIENT_LIBRARIES ${OPENAL_BUNDLED_LIBRARIES})
			include_directories(SYSTEM ${OPENAL_BUNDLED_INCLUDE_DIR})
			add_definitions(-DAL_LIBTYPE_STATIC)
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

	if(FEATURE_PAKISOLATION)
		add_definitions(-DFEATURE_PAKISOLATION)
	endif(FEATURE_PAKISOLATION)
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
	else()
		set(CLIENT_SRC ${CLIENT_SRC} "src/qcommon/dl_main_stubs.c")
		set(SERVER_SRC ${SERVER_SRC} "src/qcommon/dl_main_stubs.c")
	endif()

	if(FEATURE_SSL)
		if(NOT BUNDLED_WOLFSSL AND NOT BUNDLED_OPENSSL)
			if(NOT APPLE AND NOT WIN32)
				find_package(OpenSSL REQUIRED)
				list(APPEND CLIENT_LIBRARIES ${OPENSSL_LIBRARIES})
				list(APPEND SERVER_LIBRARIES ${OPENSSL_LIBRARIES})
				include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
				add_definitions(-DUSING_OPENSSL)
			else()
				# System SSL (Schannel on windows or Secure transport on mac)
				add_definitions(-DUSING_SCHANNEL)
			endif ()
		elseif(BUNDLED_OPENSSL)
			list(APPEND CLIENT_LIBRARIES ${OPENSSL_BUNDLED_LIBRARY})
			list(APPEND SERVER_LIBRARIES ${OPENSSL_BUNDLED_LIBRARY})
			include_directories(SYSTEM ${OPENSSL_BUNDLED_INCLUDE_DIR})
			add_definitions(-DUSING_OPENSSL)
		else() #BUNDLED_WOLFSSL
			list(APPEND CLIENT_LIBRARIES ${WOLFSSL_BUNDLED_LIBRARY})
			list(APPEND SERVER_LIBRARIES ${WOLFSSL_BUNDLED_LIBRARY})
			include_directories(SYSTEM ${WOLFSSL_BUNDLED_INCLUDE_DIR})
			add_definitions(-DUSING_WOLFSSL)
		endif()

		if(FEATURE_AUTH)
			add_definitions(-DLEGACY_AUTH)
		endif()
		add_definitions(-DFEATURE_SSL)
	else()
	endif()

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
			"src/db/db_sql_cmds.c"
		)
		set(CLIENT_SRC ${CLIENT_SRC} ${DBMS_SRC})
		set(SERVER_SRC ${SERVER_SRC} ${DBMS_SRC})
	endif(FEATURE_DBMS)

	if(FEATURE_AUTOUPDATE)
		add_definitions(-DFEATURE_AUTOUPDATE)
	endif(FEATURE_AUTOUPDATE)
endif()

if(BUILD_SERVER)
	# FIXME: this is actually DEDICATED only
	if(FEATURE_IRC_SERVER)
		add_definitions(-DFEATURE_IRC_SERVER)
		list(APPEND SERVER_SRC ${IRC_CLIENT_FILES})
	endif(FEATURE_IRC_SERVER)
endif(BUILD_SERVER)

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

	if(FEATURE_PRESTIGE)
		add_definitions(-DFEATURE_PRESTIGE)
	endif(FEATURE_PRESTIGE)

	if(FEATURE_LUA)
		if(NOT BUNDLED_LUA)
			find_package(Lua 5.4 REQUIRED)
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

	if(FEATURE_EDV)
		add_definitions(-DFEATURE_EDV)
	endif(FEATURE_EDV)
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
	find_package(MiniZip REQUIRED)
	list(APPEND CLIENT_LIBRARIES ${MINIZIP_LIBRARIES})
	list(APPEND SERVER_LIBRARIES ${MINIZIP_LIBRARIES})
	include_directories(SYSTEM ${MINIZIP_INCLUDE_DIRS})
else()
	add_definitions(-DBUNDLED_MINIZIP)
	list(APPEND CLIENT_LIBRARIES ${MINIZIP_BUNDLED_LIBRARIES})
	list(APPEND SERVER_LIBRARIES ${MINIZIP_BUNDLED_LIBRARIES})
	include_directories(SYSTEM ${MINIZIP_BUNDLED_INCLUDE_DIR})
endif()

if(FEATURE_ANTICHEAT)
	add_definitions(-DFEATURE_ANTICHEAT)
endif(FEATURE_ANTICHEAT)

if (FEATURE_RENDERER_GLES)
	add_definitions(-DFEATURE_RENDERER_GLES)
endif()
