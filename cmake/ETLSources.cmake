#-----------------------------------------------------------------
# Sources
#-----------------------------------------------------------------

FILE(GLOB COMMON_SRC
	"src/qcommon/*.c"
	"src/qcommon/*.h"
	"src/qcommon/crypto/sha-1/sha1.h"
	"src/qcommon/crypto/sha-1/sha1.c"
)

FILE(GLOB COMMON_SRC_REMOVE
	"src/qcommon/dl_main_curl.c"
	"src/qcommon/dl_main_stubs.c"
	"src/qcommon/i18n_*"
	"src/qcommon/json.c"
	"src/qcommon/json_stubs.c"
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
	"src/qcommon/update.c"
	"src/qcommon/download.c"
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
	"src/sdl/*.c"
	"src/sdl/*.h"
	"src/qcommon/update.c"
	"src/qcommon/download.c"
)

# These files are shared with the CGAME from the UI library
FILE(GLOB UI_SHARED
	"src/ui/ui_shared.c"
	"src/ui/ui_parse.c"
	"src/ui/ui_script.c"
	"src/ui/ui_menu.c"
	"src/ui/ui_menuitem.c"
)

FILE(GLOB CGAME_SRC
	"src/cgame/*.c"
	"src/cgame/*.h"
	"src/qcommon/q_math.c"
	"src/qcommon/q_shared.c"
	"src/qcommon/q_unicode.c"
	"src/game/bg_*.c"
)

LIST(APPEND CGAME_SRC ${UI_SHARED})

FILE(GLOB QAGAME_SRC
	"src/game/*.c"
	"src/game/*.h"
	"src/qcommon/crypto/sha-1/sha1.c"
	"src/qcommon/q_math.c"
	"src/qcommon/q_shared.c"
)

FILE(GLOB UI_SRC
	"src/ui/*.c"
	"src/ui/*.h"
	"src/qcommon/q_math.c"
	"src/qcommon/q_shared.c"
	"src/qcommon/q_unicode.c"
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
	"src/sys/sys_local.h"
	"src/qcommon/q_shared.h"
	"src/qcommon/puff.h"
)

FILE(GLOB RENDERER_COMMON_DYNAMIC
	"src/qcommon/q_shared.c"
	"src/qcommon/q_math.c"
	"src/qcommon/puff.c"
	"src/qcommon/md4.c"
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
	"src/renderer2/*.h"
)

SET(GLSL_PATH "src/renderer2/glsl")

FILE(GLOB RENDERER2_SHADERS
	"${GLSL_PATH}/*.glsl"
	"${GLSL_PATH}/*/*.glsl"
)

FILE(GLOB RENDERER2_SHADERDEFS
	"src/renderer2/gldef/*.gldef"
)

FILE(GLOB IRC_CLIENT_FILES
	"src/irc/htable.c"
	"src/irc/htable.h"
	"src/irc/irc_client.c"
)
