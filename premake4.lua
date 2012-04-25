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
			"Optimize", -- OptimizeSpeed sigsegvs 64bit build
			"EnableSSE",
			"StaticRuntime"
		}
	
	configuration "Debug"
		defines     "_DEBUG"
		flags
		{
			"Symbols",
			"StaticRuntime",
		}
	--
	-- Options Configurations
	--
	newoption {
		trigger	=	"static",
		description = 	"Do not link dependencies dynamically",
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

		"src/renderer/**.c", "src/renderer/**.h",
	}
	defines
	{
		"BOTLIB",
		"USE_ICON",
	}
	excludes
	{
		"src/botlib/botlib_stub.c",

		-- Premake will support configuration-dependent files in the next version.
		-- Force cURL until then.
		"src/qcommon/dl_main_stubs.c",
	}

	--
	-- Options Configurations
	--
	newoption {
		trigger	=	"with-freetype",
		description = 	"Use freetype font library",
	}

	newoption {
		trigger =   "with-openal",
		description =   "Use OpenAL sound library",
	}

	newoption {
		trigger =	"netlib",
		value = 	"LIBRARY",
		description = "Choose a library that will be used for downloading files",
		allowed = {
			{ "curl",	"cURL (default)" },
		}
	}

	if not _OPTIONS["netlib"] then
		_OPTIONS["netlib"] = "curl"
	end
	
	configuration "with-freetype"
		if not _OPTIONS["static"] then
			buildoptions 	{ "`pkg-config --cflags freetype2`" }
			linkoptions	{ "`pkg-config --libs freetype2`" }
		end
		defines      	{ "USE_FREETYPE" }

	configuration "with-openal"
		if not _OPTIONS["static"] then
			buildoptions	{ "`pkg-config --cflags openal`" }
			linkoptions	{ "`pkg-config --libs openal`" }
		end
		defines      
		{
			"USE_OPENAL",
			"USE_OPENAL_DLOPEN",
		}

	--
	-- netlib options
	--
	configuration "curl"
		if not _OPTIONS["static"] then
			buildoptions    { "`pkg-config --cflags libcurl`" }
			linkoptions     { "`pkg-config --libs libcurl`" }
		end
		defines         { "USE_CURL" }
	
	-- 
	-- Windows build options
	-- 
	configuration {  "mingw"  }
		-- for the love of god, don't forget to link this first
		links { "mingw32" }

	configuration {  "vs* or mingw"  }
		targetextension ".exe"
		flags       { "WinMain" }
		links
		{ 
			-- NOTE TO SELF:
			-- Think twice before changing the order of the
			-- following libraries !!!
			"ws2_32",
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
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
		
		
	configuration { "vs* or mingw", "x32" }
		targetdir "build/win-x32"
		
	configuration { "vs* or mingw", "x64" }
		targetdir "build/win-x64"
		
	-- 		
	-- Linux build options		
	-- 		
	configuration { "linux", "not mingw", "gmake" }
		buildoptions { "`pkg-config --cflags gl`" }
		linkoptions { "`pkg-config --libs gl`" }
		if not _OPTIONS["static"] then
			buildoptions { "`pkg-config --cflags sdl`" }
			linkoptions { "`pkg-config --libs sdl`" }
		end
	
	configuration { "linux", "not mingw", "x32" }
		targetdir "build/linux-i386"
		
	configuration { "linux", "not mingw", "x64" }
		targetdir "build/linux-x86_64"
	
	configuration { "linux", "not mingw" }
		targetname  "etl"
		buildoptions { "-pthread" }
		links
		{
			"dl",
			"m",
		}
		
		if not _OPTIONS["static"] then
			links { "jpeg" }
		end

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
		"src/botlib/botlib_stub.c",
		"src/qcommon/dl_main_curl.c",
	}

	-- 
	-- Windows build options
	-- 
	configuration {  "mingw"  }
		-- for the love of god, don't forget to link this first
		links { "mingw32" }

	configuration { "vs* or mingw" }
		targetextension ".exe"
		flags       { "WinMain" }
		links
		{
			-- NOTE TO SELF:
			-- Think twice before changing the order of the
			-- following libraries !!!
			"ws2_32",
			"winmm",
			"wsock32",
 			"iphlpapi",
			
			"SDLmain",
			"SDL",
			
			"psapi",
		}
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
		
	configuration { "vs* or mingw", "x32" }
		targetdir 	"build/win-x32"
		
	configuration { "vs* or mingw", "x64" }
		targetdir 	"build/win-x64"

	--
	-- Linux build options	
	--
	configuration { "linux", "not mingw", "x32" }
		targetdir 	"build/linux-i386"
		
	configuration { "linux", "not mingw", "x64" }
		targetdir 	"build/linux-x86_64"
	
	configuration { "linux", "not mingw" }
		targetname  "etlded"
		buildoptions { "-pthread" }
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
		
		"src/cgame/**.c", "src/cgame/**.h",
		
		"src/game/bg_**.c", "src/game/bg_**.h",
		
		"src/ui/ui_shared.c", "src/ui/ui_shared.h"
	}
	includedirs
	{
		"src/qcommon",
		"src/game",
	}
	defines { "CGAMEDLL" }
		
	-- 
	-- Project Configurations
	-- 
	configuration {"vs* or mingw", "x32"}
		targetdir "build/win-x32/etmain"
		targetname  "cgame_mp_x86"
		targetextension ".dll"
		targetprefix ""
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}

	configuration {"vs* or mingw", "x64"}
		targetdir "build/win-x64/etmain"
		targetname  "cgame_mp_x86_64"
		targetextension ".dll"
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
		
		"src/game/**.c", "src/game/**.h",
		
		"src/botai/**.c", "src/botai/**.h",
	}
	includedirs
	{
		"src/qcommon",
	}
	defines
	{ 
		"GAMEDLL",
	}
	
	-- 
	-- Project Configurations
	-- 
    configuration {"vs* or mingw", "x32"}
		targetdir "build/win-x32/etmain"
		targetname  "qagame_mp_x86"
		targetextension ".dll"
		targetprefix ""
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}

	configuration {"vs* or mingw", "x64"}
		targetdir "build/win-x64/etmain"
		targetname  "qagame_mp_x86_64"
		targetextension ".dll"
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
		
		"src/ui/**.c", "src/ui/**.h",
		
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
	
	-- 
	-- Project Configurations
	-- 
    configuration {"vs* or mingw", "x32"}
		targetdir "build/win-x32/etmain"
		targetname  "ui_mp_x86"
		targetextension ".dll"
		targetprefix ""
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}

	configuration {"vs* or mingw", "x64"}
		targetdir "build/win-x64/etmain"
		targetname  "ui_mp_x86_64"
		targetextension ".dll"
		targetprefix ""
	
	configuration { "linux", "not mingw", "x32" }
		targetdir "build/linux-i386/etmain"
		targetname  "ui.mp.i386"
		targetprefix ""
	
	configuration { "linux", "not mingw", "x64" }
		targetdir "build/linux-x86_64/etmain"
		targetname  "ui.mp.x86_64"
		targetprefix ""
	