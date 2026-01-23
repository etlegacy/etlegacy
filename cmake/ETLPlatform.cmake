#-----------------------------------------------------------------
# Platform
#-----------------------------------------------------------------

# Used to store real system processor when we overwrite CMAKE_SYSTEM_PROCESSOR for cross-compile builds
set(ETLEGACY_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# How many architectures are we buildin
set(ETL_ARCH_COUNT 1)

add_library(os_libraries INTERFACE)

# Enable specific C warnings
if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdeclaration-after-statement -Wunused-but-set-variable")
endif()

# Color diagnostics for build systems other than make
if(UNIX)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
	endif()
endif()

if(CROSS_COMPILE32 AND ETL_ARM)
	message(FATAL_ERROR "Cross compiling not supported for ARM!")
endif()

if(UNIX)
	# optimization/debug flags
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ffast-math")
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		# XXX : attach debug symbols to all builds for now
		# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
		# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb")
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
		target_link_libraries(os_libraries INTERFACE m pthread)
		set(LIB_SUFFIX ".mp.obsd.")
	elseif(CMAKE_SYSTEM MATCHES "FreeBSD")
		target_link_libraries(os_libraries INTERFACE m pthread)
		set(LIB_SUFFIX ".mp.fbsd.")
	elseif(CMAKE_SYSTEM MATCHES "NetBSD")
		target_link_libraries(os_libraries INTERFACE m pthread)
		set(LIB_SUFFIX ".mp.nbsd.")
    elseif(ANDROID)
		target_link_libraries(os_libraries INTERFACE android log OpenSLES)
		set(LIB_SUFFIX ".mp.android.")
	elseif(APPLE)
		# TODO: use find package with the MacOs frameworks instead of direct linker flags..
		target_link_libraries(os_libraries INTERFACE dl m objc)

		find_library(cocoa_libraries Cocoa REQUIRED)
		find_library(iokit_libraries IOKit REQUIRED)
		find_library(core_foundation_libraries CoreFoundation REQUIRED)

		target_link_libraries(os_libraries INTERFACE ${cocoa_libraries} ${iokit_libraries} ${core_foundation_libraries})

		if (DEFINED CMAKE_OSX_ARCHITECTURES)
			list(LENGTH CMAKE_OSX_ARCHITECTURES ETL_ARCH_COUNT)
		endif()

		# new curl builds need the System Configuration framework
		if (BUNDLED_CURL)
			find_library(system_configuration_library SystemConfiguration REQUIRED)
			target_link_libraries(os_libraries INTERFACE ${system_configuration_library})
		endif()

		if(BUNDLED_CURL AND FEATURE_SSL AND (NOT BUNDLED_OPENSSL AND NOT BUNDLED_WOLFSSL))
			find_library(security_library Security REQUIRED)
			target_link_libraries(os_libraries INTERFACE ${security_library})
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
			find_package(Iconv REQUIRED)
			find_library(audio_toolbox_library AudioToolbox REQUIRED)
			find_library(carbon_library Carbon REQUIRED)
			find_library(core_audio_library CoreAudio REQUIRED)
			find_library(core_video_library CoreVideo REQUIRED)
			find_library(force_feedback_library ForceFeedback REQUIRED)
			find_library(opengl_library OpenGL REQUIRED)

			target_link_libraries(client_libraries INTERFACE
					${audio_toolbox_library} ${carbon_library} ${core_audio_library}
					${core_video_library} ${force_feedback_library} ${opengl_library}
					Iconv::Iconv
			)
			# TODO: check if this breaks compatibility with pre macos 13.0 versions?
			if (BUNDLED_SDL)
				find_library(core_haptics_library CoreHaptics REQUIRED)
				find_library(game_controller_library GameController REQUIRED)
				target_link_libraries(client_libraries INTERFACE ${core_haptics_library} ${game_controller_library})
			endif()
		endif()
		set(LIB_SUFFIX "_mac")
		set(CMAKE_SHARED_MODULE_SUFFIX "")
	else()
		target_link_libraries(os_libraries INTERFACE  m ${CMAKE_DL_LIBS} rt pthread)
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
	target_compile_definitions(shared_libraries INTERFACE WINVER=0x601)

	if(ETL_WIN64)
		target_compile_definitions(shared_libraries INTERFACE C_ONLY)
	endif()

	target_link_libraries(os_libraries INTERFACE wsock32 ws2_32 psapi winmm)

	if(FEATURE_SSL)
		target_link_libraries(os_libraries INTERFACE Crypt32)
	endif()

	if(BUNDLED_SDL)
		# Libraries for Win32 native and MinGW required by static SDL2 build
		target_link_libraries(os_libraries INTERFACE user32 gdi32 imm32 ole32 oleaut32 version uuid hid setupapi)
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

		# is using cl.exe the __FILE__ macro will not contain the full path to the file by default.
		# enable the full path with /FC (we use our own ETL_FILENAME to cut the path to project relative path)
		target_compile_options(shared_libraries INTERFACE /FC)

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
	if(ETL_X86 AND ETL_32BITS)
		if(WIN32)
			set(ARCH "x86")
		else()
			set(ARCH "i386")
		endif()
		set(BIN_DESCRIBE "(x86 32-bit)")
	elseif(ETL_X86 AND ETL_64BITS)
		if(WIN32)
			set(ARCH "x64")
		else()
			set(ARCH "x86_64")
		endif()
		set(BIN_DESCRIBE "(x86 64-bit)")
	elseif(ETL_ARM AND ETL_64BITS)
		if(NOT ANDROID)
			set(ARCH "aarch64")
		else()
			set(ARCH "arm64-v8a")
		endif()
	else()
		set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
		message(STATUS "Warning: processor architecture not recognised (${CMAKE_SYSTEM_PROCESSOR})")
	endif()

	if (ENABLE_MULTI_BUILD)
		set(BIN_SUFFIX ".${ARCH}")
	endif()

	# default to a sane value
	if(NOT DEFINED BIN_DESCRIBE OR "${BIN_DESCRIBE}" STREQUAL "")
		set(BIN_DESCRIBE "(${ARCH})")
	endif()
endif()

if(UNIX AND ETL_64BITS)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# summary
message(STATUS "System: ${CMAKE_SYSTEM} (${ETLEGACY_SYSTEM_PROCESSOR})")
message(STATUS "Lib arch: ${CMAKE_LIBRARY_ARCHITECTURE}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "Install path:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Compiler flags:")
message(STATUS "- C             ${CMAKE_C_FLAGS}")
message(STATUS "- C++           ${CMAKE_CXX_FLAGS}")
message(STATUS "- PIC           ${CMAKE_POSITION_INDEPENDENT_CODE}")
message(STATUS "Linker flags:")
message(STATUS "- Executable    ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "- Module        ${CMAKE_MODULE_LINKER_FLAGS}")
message(STATUS "- Shared        ${CMAKE_SHARED_LINKER_FLAGS}")
