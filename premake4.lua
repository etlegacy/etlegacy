-- 
-- ET: Legacy, an online game based on Wolfenstein: Enemy Territory
-- 
-- You need *premake* to compile this project. 
-- Get it from here: http://industriousone.com/premake
-- 
solution "etlegacy"

	configurations { "Release", "Debug", "mingw" }
	platforms { "x32", "x64" }
	--
	-- Release/Debug Configurations
	--
	configuration "Release"
		defines     "NDEBUG"
		flags      
		{
			"OptimizeSpeed",
			"EnableSSE",
			"StaticRuntime"
		}
	
	configuration "Debug"
		defines     "_DEBUG"
		flags
		{
			"Symbols",
			"StaticRuntime",
			--"NoRuntimeChecks"
		}

-- 
-- CLIENT BUILD
-- 
project "etlegacy"
	targetname  "etl"
	language    "C"
	kind        "WindowedApp"
	flags       { "ExtraWarnings" }
	files
	{
		"src/qcommon/**.c", "src/qcommon/**.h",
		
		"src/botlib/**.c", "src/botlib/**.h",
		
		"src/client/**.c", "src/client/**.h",
		"src/server/**.c", "src/server/**.h",
		
		"src/sdl/sdl_**.c",

		"src/sys/sys_platform.c",
		"src/sys/sys_main.c",
		"src/sys/con_log.c",
		"src/sys/con_tty.c",

		"src/renderer/**.c", "src/renderer/**.cpp", "src/renderer/**.h",		
	}
	defines
	{
		"BOTLIB",
		"USE_ICON",
	}
	excludes
	{
		"src/qcommon/dl_main_stubs.c",
		"src/botlib/botlib_stub.c",
	}
	
	--
	-- Options Configurations
	--
	configuration "with-freetype"
		buildoptions 	{ "`pkg-config --cflags freetype2`" }
		linkoptions	{ "`pkg-config --libs freetype2`" }
		defines      	{ "USE_FREETYPE" }

	configuration "with-openal"
		buildoptions	{ "`pkg-config --cflags openal`" }
		linkoptions	{ "`pkg-config --libs openal`" }
		defines      
		{
			"USE_OPENAL",
			"USE_OPENAL_DLOPEN",
		}
	
	-- 
	-- Windows build options
	-- 
	configuration {  "mingw"  }
		targetname  "etl"
		flags       { "WinMain" }
		libdirs
		{
			-- specify library directories with -L
		}
		links
		{ 
			-- NOTE TO SELF:
			-- Think twice before changing the order of the
			-- following libraries !!!
			"mingw32", -- for the love of god, don't forget to link this first
			"ws2_32",
			"dinput8",
			"winmm",
			"wsock32",
 			"iphlpapi",
			"opengl32",
			
			"curl",
			"jpeg",
			"SDLmain",
			"SDL",
			
			"psapi",
		}
		buildoptions
		{
			"`/usr/i686-pc-mingw32/usr/bin/pkg-config --cflags sdl`",
			"`/usr/i686-pc-mingw32/usr/bin/pkg-config --cflags libcurl`",
			"`/usr/i686-pc-mingw32/usr/bin/pkg-config --cflags freetype2`",
		}
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
		
		
	configuration { "mingw", "x32" }
		targetdir "build/win-x32"
		
	configuration { "mingw", "x64" }
		targetdir "build/win-x64"
		
	-- 		
	-- Linux build options		
	-- 		
	configuration { "linux", "not mingw", "gmake" }
		buildoptions
		{
			"`pkg-config --cflags x11`",
			"`pkg-config --cflags xext`",
			"`pkg-config --cflags xxf86dga`",
			"`pkg-config --cflags xxf86vm`",
			"`pkg-config --cflags sdl`",
			"`pkg-config --cflags libcurl`",
			"`pkg-config --cflags gl`",
		}
		linkoptions
		{
			"`pkg-config --libs x11`",
			"`pkg-config --libs xext`",
			"`pkg-config --libs xxf86dga`",
			"`pkg-config --libs xxf86vm`",
			"`pkg-config --libs sdl`",
			"`pkg-config --libs libcurl`",
			"`pkg-config --libs gl`",
		}
	
	configuration { "linux", "not mingw", "x32" }
		targetdir "build/linux-i386"
		
	configuration { "linux", "not mingw", "x64" }
		targetdir "build/linux-x86_64"
	
	configuration { "linux", "not mingw" }
		targetname  "etl"
		buildoptions
		{
			"-pthread"
		}
		links
		{
			"dl",
			"m",
			"jpeg",
		}

-- 
-- DEDICATED SERVER BUILD
-- 
project "etlegacy-dedicated"
	targetname  "etlded"
	language    "C"
	kind        "WindowedApp"
	flags       { "ExtraWarnings" }
	files
	{
		"src/qcommon/**.c", "src/qcommon/**.h",
		
		"src/botlib/**.c", "src/botlib/**.h",
		"src/server/**.c", "src/server/**.h",
		
		"src/null/null_client.c",
		"src/null/null_input.c",
		"src/null/null_snddma.c",
			
		"src/sys/sys_platform.c",
		"src/sys/sys_main.c",
		"src/sys/con_log.c",
		"src/sys/con_tty.c",
	}
	defines
	{ 
		"DEDICATED",
		"BOTLIB",
	}
	excludes
	{
		"src/qcommon/dl_main_stubs.c",
		"src/botlib/botlib_stub.c",
	}
	
	-- 
	-- Windows build options
	-- 
	configuration { "mingw" }
		flags       { "WinMain" }
		buildoptions
		{
			"`/usr/i686-pc-mingw32/usr/bin/pkg-config --cflags libcurl`",
		}
		links
		{
			-- NOTE TO SELF:
			-- Think twice before changing the order of the
			-- following libraries !!!
			"mingw32", -- for the love of god, don't forget to link this first
			"ws2_32",
			"winmm",
			"wsock32",
 			"iphlpapi",
			
			"curl",
			"SDLmain",
			"SDL",
			
			"psapi",
		}
		libdirs
		{
			"../mingw-deps/mingw-libs"
		}
		includedirs
		{
			-- specify include directories with -I
	-- 
		}
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
		
	configuration { "mingw", "x32" }
		targetdir 	"build/win-x32"
		
	configuration { "mingw", "x64" }
		targetdir 	"build/win-x64"

	--
	-- Linux build options	
	-- 	
	configuration { "linux", "not mingw", "gmake" }
		buildoptions
		{
			"`pkg-config --cflags libcurl`",
		}
		linkoptions
		{
			"`pkg-config --libs libcurl`",
		}
		
	configuration { "linux", "not mingw", "x32" }
		targetdir 	"build/linux-i386"
		
	configuration { "linux", "not mingw", "x64" }
		targetdir 	"build/linux-x86_64"
	
	configuration { "linux", "not mingw" }
		targetname  "etlded"
		buildoptions
		{
			"-pthread"
		}
		links
		{
			"dl",
			"m",
		}

-- 
-- CGAME lib
-- 			
project "etmain_cgame"
	targetname  "cgame"
	language    "C"
	kind        "SharedLib"
	flags       { "ExtraWarnings" }
	files
	{
		"src/qcommon/q_math.c",
		"src/qcommon/q_shared.c",
		"src/qcommon/q_shared.h",
		"src/cgame/cg_public.h",
		"src/renderer/tr_types.h",
		"src/ui/keycodes.h",
		"src/game/surfaceflags.h",
		
		"src/cgame/**.c", "src/cgame/**.cpp", "src/cgame/**.h",
		
		"src/game/bg_**.c", "src/game/bg_**.cpp", "src/game/bg_**.h",
		
		"src/ui/ui_shared.c", "src/ui/ui_shared.h"
	}
	includedirs
	{
		"src/qcommon",
		"src/game",
	}
	defines
	{ 
		"CGAMEDLL",
	}
		
	-- 
	-- Project Configurations
	-- 
	configuration {"mingw", "x32"}
		targetdir "build/win-x32/etmain"
		targetname  "cgame_mp_x86"
		targetsuffix ".dll"
		targetprefix ""
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}

	configuration {"mingw", "x64"}
		targetdir "build/win-x64/etmain"
		targetname  "cgame_mp_x86_64"
		targetsuffix ".dll"
		targetprefix ""
	
	configuration { "linux", "not mingw", "x32" }
		targetdir "build/linux-i386/etmain"
		targetname  "cgame.mp.i386"
		targetprefix ""
	
	configuration { "linux", "not mingw", "x64" }
		targetdir "build/linux-x86_64/etmain"
		targetname  "cgame.mp.x86_64"
		targetprefix ""
-- 
-- QAGAME lib
-- 		
project "etmain_game"
	targetname  "game"
	language    "C"
	kind        "SharedLib"
	flags       { "ExtraWarnings" }
	files
	{
		"src/qcommon/q_math.c",
		"src/qcommon/q_shared.c",
		"src/qcommon/q_shared.h",
		"src/game/g_public.h",
		"src/game/surfaceflags.h",
		
		"src/game/**.c", "src/game/**.cpp", "src/game/**.h",
		
		"src/botai/**.c", "src/botai/**.h",
	}
	includedirs
	{
		"src/qcommon",
	}
	defines
	{ 
		"GAMEDLL",
		"NO_BOT_SUPPORT"
	}
	
	-- 
	-- Project Configurations
	-- 
	configuration {"mingw", "x32"}
		targetdir "build/win-x32/etmain"
		targetname  "qagame_mp_x86"
		targetsuffix ".dll"
		targetprefix ""
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}

	configuration {"mingw", "x64"}
		targetdir "build/win-x64/etmain"
		targetname  "qagame_mp_x86_64"
		targetsuffix ".dll"
		targetprefix ""
	
	configuration { "linux", "not mingw", "x32" }
		targetdir "build/linux-i386/etmain"
		targetname  "qagame.mp.i386"
		targetprefix ""
	
	configuration { "linux", "not mingw", "x64" }
		targetdir "build/linux-x86_64/etmain"
		targetname  "qagame.mp.x86_64"
		targetprefix ""
-- 
-- UI lib
-- 
project "etmain_ui"
	targetname  "ui"
	language    "C"
	kind        "SharedLib"
	flags       { "ExtraWarnings" }
	files
	{
		"src/qcommon/q_math.c",
		"src/qcommon/q_shared.c",
		"src/qcommon/q_shared.h",
		"src/ui/ui_public.h",
		"src/renderer/tr_types.h",
		"src/ui/keycodes.h",
		"src/game/surfaceflags.h",
		
		"src/ui/**.c", "src/ui/**.cpp", "src/ui/**.h",
		
		"etmain/ui/menudef.h",
		"etmain/ui/menumacros.h",
		
		"src/game/bg_public.h",
		"src/game/bg_classes.h",
		"src/game/bg_campaign.c",
		"src/game/bg_classes.c",
		"src/game/bg_misc.c",
	}
	includedirs
	{
		"src/qcommon",
	}
	defines
	{ 
		--"CGAMEDLL",
	}
	
	-- 
	-- Project Configurations
	-- 
	configuration {"mingw", "x32"}
		targetdir "build/win-x32/etmain"
		targetname  "ui_mp_x86"
		targetsuffix ".dll"
		targetprefix ""
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}

	configuration {"mingw", "x64"}
		targetdir "build/win-x64/etmain"
		targetname  "ui_mp_x86_64"
		targetsuffix ".dll"
		targetprefix ""
	
	configuration { "linux", "not mingw", "x32" }
		targetdir "build/linux-i386/etmain"
		targetname  "ui.mp.i386"
		targetprefix ""
	
	configuration { "linux", "not mingw", "x64" }
		targetdir "build/linux-x86_64/etmain"
		targetname  "ui.mp.x86_64"
		targetprefix ""
	