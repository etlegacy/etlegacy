#-----------------------------------------------------------------
# Platform
#-----------------------------------------------------------------

# Used to store real system processor when we overwrite CMAKE_SYSTEM_PROCESSOR for cross-compile builds
set(ETLEGACY_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

message(STATUS "System: ${CMAKE_SYSTEM} (${ETLEGACY_SYSTEM_PROCESSOR})")
message(STATUS "Lib arch: ${CMAKE_LIBRARY_ARCHITECTURE}")

if(UNIX AND CROSS_COMPILE32 AND NOT ARM) # 32-bit build
	set(CMAKE_SYSTEM_PROCESSOR i386)
	message(STATUS "Forcing ${CMAKE_SYSTEM_PROCESSOR} to cross compile 32bit")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m32")
elseif(WIN32 AND CROSS_COMPILE32)
	set(CMAKE_SYSTEM_PROCESSOR x86) #the new cmake on windows will otherwise use arch name of x64 which will fuck up our naming
	set(ENV{PLATFORM} win32) #this is redundant but just to  be safe
elseif(ARM AND CROSS_COMPILE32)
	message(STATUS "Cross compiling not supported for ARM!")
endif()

# FIXME: move this down to UNIX section?
if(ARM)
	if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv6l")
		message(STATUS "ARMV6 build options set.")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -pipe -mfloat-abi=hard -mfpu=vfp -march=armv6zk -O2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pipe -mfloat-abi=hard -mfpu=vfp -mtune=arm1176jzf-s -march=armv6zk -O2")
	elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7l")
		message(STATUS "ARMV7 build options set.")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -pipe -mfloat-abi=hard -mfpu=neon -march=armv7-a -O2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pipe -mfloat-abi=hard -mfpu=neon -march=armv7-a -O2")
	else()
		message(STATUS "Unknown ARM processor detected !!!")
	endif()
endif(ARM)

if(APPLE)
	# The ioapi requires this since OSX already uses 64 fileapi (there is no fseek64 etc)
	add_definitions(-DUSE_FILE32API)
endif(APPLE)

if(UNIX)
	# optimization/debug flags
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ffast-math")
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__extern_always_inline=inline")
	endif()
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")

	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

	if(CMAKE_SYSTEM MATCHES "OpenBSD*")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.obsd.")
	elseif(CMAKE_SYSTEM MATCHES "FreeBSD")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.fbsd.")
	elseif(CMAKE_SYSTEM MATCHES "NetBSD")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.nbsd.")
	elseif(APPLE)
		set(OS_LIBRARIES dl m)
		set(CMAKE_EXE_LINKER_FLAGS "-lobjc -framework Cocoa -framework IOKit -framework CoreFoundation")
		set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem") # These flags will cause error with older Xcode
		set(CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem")

		# Must specify a target, otherwise it will require the OS version used at compile time.
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
		execute_process(COMMAND xcrun -show-sdk-path OUTPUT_VARIABLE XCODE_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
		set(CMAKE_OSX_SYSROOT "${XCODE_SDK_PATH}")
		set(CMAKE_CXX_FLAGS "-isysroot ${CMAKE_OSX_SYSROOT} ${CMAKE_CXX_FLAGS}")

		if(BUILD_CLIENT)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Quartz -framework AudioToolbox -framework AudioUnit -framework Carbon -framework CoreAudio -framework ForceFeedback -liconv")
		endif()
		set(LIB_SUFFIX "_mac")
		set(CMAKE_SHARED_MODULE_SUFFIX "")
	else()
		set(OS_LIBRARIES ${CMAKE_DL_LIBS} m rt pthread)
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
	add_definitions(-DWINVER=0x601)

	if(WIN64)
		add_definitions(-DC_ONLY)
	endif(WIN64)

	set(OS_LIBRARIES wsock32 ws2_32 psapi winmm)

	if(BUNDLED_OPENSSL)
		list(APPEND OS_LIBRARIES Crypt32)
	endif()
	if(BUNDLED_SDL)
		# Libraries for Win32 native and MinGW required by static SDL2 build
		list(APPEND OS_LIBRARIES user32 gdi32 winmm imm32 ole32 oleaut32 version uuid hid setupapi)
	endif()
	set(LIB_SUFFIX "_mp_")
	if(MSVC)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /EHsc /O2")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /EHa /W3")

		set(CompilerFlags
			CMAKE_CXX_FLAGS
			CMAKE_CXX_FLAGS_DEBUG
			CMAKE_CXX_FLAGS_RELEASE
			CMAKE_C_FLAGS
			CMAKE_C_FLAGS_DEBUG
			CMAKE_C_FLAGS_RELEASE
		)

		foreach(CompilerFlag ${CompilerFlags})
			string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
		endforeach()

		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT.lib /NODEFAULTLIB:MSVCRTD.lib")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT.lib /NODEFAULTLIB:MSVCRTD.lib")
		add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Do not show CRT warnings
	endif(MSVC)
	if(MINGW AND NOT DEBUG_BUILD)
		set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -static-libgcc")
		set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -static-libgcc -static-libstdc++")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s")
		set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc -liconv -s")
		set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++ -liconv -s")
		add_definitions(-D_WIN32_IE=0x0501)
	endif(MINGW AND NOT DEBUG_BUILD)
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
		if(WIN32)
			set(ARCH "x64")
		else()
			set(ARCH "x86_64")
		endif()
	elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv6l")
		message(STATUS "Detected ARMV6 target processor")
		set(ARCH "arm")
	elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7l")
		message(STATUS "Detected ARMV7 target processor")
		set(ARCH "arm")
		#add_definitions(-DX265_ARCH_ARM=1 -DHAVE_ARMV7=1)
	else()
		set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
		message(STATUS "Warning: processor architecture not recognised (${CMAKE_SYSTEM_PROCESSOR})")
	endif()
endif(NOT APPLE)
