#-----------------------------------------------------------------
# Platform
#-----------------------------------------------------------------

# Used to store real system processor when we overwrite CMAKE_SYSTEM_PROCESSOR for cross-compile builds
set(ETLEGACY_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

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

	if(ENABLE_ASAN)
		include (CheckCCompilerFlag)
		include (CheckCXXCompilerFlag)
		set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
		CHECK_C_COMPILER_FLAG("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS_C)
		CHECK_CXX_COMPILER_FLAG("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS_CXX)

		if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
			# Clang requires an external symbolizer program.
			FIND_PROGRAM(LLVM_SYMBOLIZER
					NAMES llvm-symbolizer
					llvm-symbolizer-3.8
					llvm-symbolizer-3.7
					llvm-symbolizer-3.6)

			if(NOT LLVM_SYMBOLIZER)
				message(WARNING "AddressSanitizer failed to locate an llvm-symbolizer program. Stack traces may lack symbols.")
			endif()
		endif()

		if(HAVE_FLAG_SANITIZE_ADDRESS_C AND HAVE_FLAG_SANITIZE_ADDRESS_CXX)
			message(STATUS "Enabling AddressSanitizer for this configuration")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
		else()
			message(FATAL_ERROR "AddressSanitizer enabled but compiler doesn't support it - cannot continue.")
		endif()
	endif()

	if(CMAKE_SYSTEM MATCHES "OpenBSD*")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.obsd.")
	elseif(CMAKE_SYSTEM MATCHES "FreeBSD")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.fbsd.")
	elseif(CMAKE_SYSTEM MATCHES "NetBSD")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.nbsd.")
	elseif(ANDROID)
		set(OS_LIBRARIES ifaddrs ogg vorbis android)
		set(LIB_SUFFIX ".mp.android.")
	elseif(APPLE)
		set(OS_LIBRARIES dl m)
		set(CMAKE_EXE_LINKER_FLAGS "-lobjc -framework Cocoa -framework IOKit -framework CoreFoundation")

		if(BUNDLED_CURL AND FEATURE_SSL AND (NOT BUNDLED_OPENSSL AND NOT BUNDLED_WOLFSSL))
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Security")
		endif()

		set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem") # These flags will cause error with older Xcode
		set(CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem")

		# Must specify a target, otherwise it will require the OS version used at compile time.
		if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
			set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14")
		endif()

		if(NOT DEFINED XCODE_SDK_PATH OR NOT XCODE_SDK_PATH)
			execute_process(COMMAND xcrun -show-sdk-path OUTPUT_VARIABLE XCODE_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
			message(STATUS "Using detected SDK from path: ${XCODE_SDK_PATH}")
		else()
			message(STATUS "Using custom SDK path: ${XCODE_SDK_PATH}")
		endif()

		execute_process(COMMAND xcrun --show-sdk-version OUTPUT_VARIABLE XCODE_SDK_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
		message(STATUS "Using SDK version: ${XCODE_SDK_VERSION}")

		# After version 10.14 it's no longer possible to build 32 bit applications with this script
		if( (CMAKE_OSX_DEPLOYMENT_TARGET GREATER "10.14" OR XCODE_SDK_VERSION GREATER_EQUAL "10.15") AND CROSS_COMPILE32)
			message(FATAL_ERROR "Can't build a 32bit build on this OSX SDK version")
		endif()

		set(CMAKE_OSX_SYSROOT "${XCODE_SDK_PATH}")
		set(CMAKE_CXX_FLAGS "-isysroot ${CMAKE_OSX_SYSROOT} ${CMAKE_CXX_FLAGS}")

		if(BUILD_CLIENT)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AudioToolbox -framework AudioUnit -framework Carbon -framework CoreAudio -framework CoreVideo -framework ForceFeedback -framework OpenGL -liconv")
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

	if(FEATURE_SSL)
		list(APPEND OS_LIBRARIES Crypt32)
	endif()

	if(BUNDLED_SDL)
		# Libraries for Win32 native and MinGW required by static SDL2 build
		list(APPEND OS_LIBRARIES user32 gdi32 winmm imm32 ole32 oleaut32 version uuid hid setupapi)
	endif()
	set(LIB_SUFFIX "_mp_")
	if(MSVC)

		message(STATUS "MSVC version: ${MSVC_VERSION}")

		if(ENABLE_ASAN)
			# Dont know the exact version but its somewhere there
			if(MSVC_VERSION LESS 1925)
				message(FATAL_ERROR "AddressSanitizer enabled but compiler doesn't support it - cannot continue.")
			endif()

			message(STATUS "Enabling AddressSanitizer for this configuration")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
		endif()

		if(FORCE_STATIC_VCRT)
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
		endif()

		# Should we always use this?
		# add_definitions(-DC_ONLY)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Do not show CRT warnings
	endif(MSVC)

	if(MINGW)

		# This is not yet supported, but most likely will happen in the future.
		if(ENABLE_ASAN)
			include (CheckCCompilerFlag)
			include (CheckCXXCompilerFlag)
			set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
			CHECK_C_COMPILER_FLAG("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS_C)
			CHECK_CXX_COMPILER_FLAG("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS_CXX)

			if(HAVE_FLAG_SANITIZE_ADDRESS_C AND HAVE_FLAG_SANITIZE_ADDRESS_CXX)
				message(STATUS "Enabling AddressSanitizer for this configuration")
				add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
			else()
				message(FATAL_ERROR "AddressSanitizer enabled but compiler doesn't support it - cannot continue.")
			endif()
		endif()

		if(NOT DEBUG_BUILD)
			set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -static-libgcc")
			set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -static-libgcc -static-libstdc++")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s")
			set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc -liconv -s")
			set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++ -liconv -s")
			add_definitions(-D_WIN32_IE=0x0501)
		endif()

	endif()
endif()

# Get the system architecture
# Not that this is NOT APPLE because in macos we bundle everything into a single file
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
		message(SEND_ERROR "armv6l not ready and should not be used!")
	elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7l")
		message(STATUS "Detected ARMV7 target processor")
		set(ARCH "arm")
		#add_definitions(-DX265_ARCH_ARM=1 -DHAVE_ARMV7=1)
	elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7-a")
		set(ARCH "armeabi-v7a")
	elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
		set(ARCH "arm64-v8a")
	else()
		set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
		message(STATUS "Warning: processor architecture not recognised (${CMAKE_SYSTEM_PROCESSOR})")
	endif()
endif()

# summary
message(STATUS "System: ${CMAKE_SYSTEM} (${ETLEGACY_SYSTEM_PROCESSOR})")
message(STATUS "Lib arch: ${CMAKE_LIBRARY_ARCHITECTURE}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "Install path:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Compiler flags:")
message(STATUS "- C             ${CMAKE_C_FLAGS}")
message(STATUS "- C++           ${CMAKE_CXX_FLAGS}")
message(STATUS "Linker flags:")
message(STATUS "- Executable    ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "- Module        ${CMAKE_MODULE_LINKER_FLAGS}")
message(STATUS "- Shared        ${CMAKE_SHARED_LINKER_FLAGS}")
