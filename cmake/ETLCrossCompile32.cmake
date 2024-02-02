#------------------------------------------------------------------------
# ET: Legacy cross compilation NOTICE
# The CROSS_COMPILE32 is an old option which was created at a time
# when ET: Legacy only existed on x86 platforms (windows, linux and mac).
#
# MEANING that the option is old and has a caveat of only being tested on x86 platforms.
#
# If you to cross compile from a system with a different architecture then just use
# the CMAKE_TOOLCHAIN_FILE option and point it to a toolchain file for your target.
#
# YOU HAVE BEEN WARNED. (this notice was added 2024)
#------------------------------------------------------------------------
set(CROSS_COMPILE32_OLD_VALUES "ON;1;YES;TRUE;Y;T")

function(legacy_replace_or_append_flag CURRENT_FLAG WANTED_FLAG _SOURCE)
	if("${CURRENT_FLAG}" STREQUAL "")
		return()
	endif()
	set(MODIFIED 0)
	if (${_SOURCE} MATCHES "${CURRENT_FLAG}")
		string(REPLACE "${CURRENT_FLAG}" "${WANTED_FLAG}" ${_SOURCE} "${${_SOURCE}}")
		message(VERBOSE "Replacing ${CURRENT_FLAG} with ${WANTED_FLAG} in ${_SOURCE}")
		set(MODIFIED 1)
	endif()
	if (NOT ${_SOURCE} MATCHES "${WANTED_FLAG}")
		string(APPEND ${_SOURCE} " ${WANTED_FLAG}")
		message(VERBOSE "Appending ${WANTED_FLAG} to ${_SOURCE}")
		set(MODIFIED 1)
	endif()
	if(MODIFIED)
		string(STRIP "${${_SOURCE}}" ${_SOURCE})
		set(${_SOURCE} "${${_SOURCE}}" PARENT_SCOPE)
	endif()
endfunction()

function(force_rescan_library LIBRARY_NAME)
	set(${LIBRARY_NAME}_LIBRARY "${LIBRARY_NAME}_LIBRARY-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
endfunction()

# The CROSS_COMPILE32 can now contain the actual target processor name (so this is no longer locked to x86)
macro(legacy_check_cross_compile_flag)
	string(TOUPPER ${CROSS_COMPILE32} CROSS_COMPILE32_UPPER)
	# Possible old CROSS_COMPILE32 values that will cause a fallback value (old style)
	if (NOT CROSS_COMPILE32_UPPER IN_LIST CROSS_COMPILE32_OLD_VALUES)
		set(CMAKE_SYSTEM_PROCESSOR ${CROSS_COMPILE32})
	else()
		if (UNIX)
			set(CMAKE_SYSTEM_PROCESSOR i386)
		elseif(WIN32)
			#the new cmake on windows will otherwise use arch name of x64 which will fuck up our naming
			set(CMAKE_SYSTEM_PROCESSOR x86)
			# this is redundant but just to  be safe
			set(ENV{PLATFORM} win32)
		endif()
	endif()
	message(STATUS "Forcing processor as '${CMAKE_SYSTEM_PROCESSOR}' to cross compile 32bit")
endmacro()

if(CROSS_COMPILE32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		message(STATUS "Crosscompiling for 32 bits architecture")

		legacy_check_cross_compile_flag()

		set(CMAKE_SIZEOF_VOID_P 4)
		if (UNIX)
			set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
			legacy_replace_or_append_flag("-m64" "-m32" CMAKE_C_FLAGS)
			legacy_replace_or_append_flag("-m64" "-m32" CMAKE_CXX_FLAGS)
			legacy_replace_or_append_flag("-m64" "-m32" CMAKE_EXE_LINKER_FLAGS)
			legacy_replace_or_append_flag("-m64" "-m32" CMAKE_SHARED_LINKER_FLAGS)
			legacy_replace_or_append_flag("-m64" "-m32" CMAKE_MODULE_LINKER_FLAGS)
		elseif(NOT WIN32)
			message(WARNING "Cross compile 32bit is 'enabled' but the platform is unknown, this might not (most likely won't) work correctly")
		endif()
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		message(WARNING "Cross compilation for 32 bit is enabled on 32 bit toolset. This should not be required.")
		legacy_check_cross_compile_flag()
	else()
		message(FATAL_ERROR "System void pointer size is: ${CMAKE_SIZEOF_VOID_P} and cross compilation is requested. This will not work...")
	endif()
endif()

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
