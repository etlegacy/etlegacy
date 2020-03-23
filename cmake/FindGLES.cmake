#
# Try to find GLES library and include path.
# Once done this will define
#
# GLES_FOUND
# GLES_INCLUDE_PATH
# GLES_LIBRARY
#

if(EXISTS "/opt/vc/include/bcm_host.h")

	# Look for RPi 4. GLES binaries in different location.
	find_path(GLES_INCLUDE_DIR NAMES GLES/gl.h PATHS "/opt/vc/include/")
	file(READ "/proc/device-tree/model" CPUINFO)
	string(FIND "${CPUINFO}" "Raspberry Pi 4" result)

	if(${result} EQUAL -1)
		find_library(GLES_LIBRARY NAMES brcmGLESv2 PATHS "/opt/vc/lib/")
		MESSAGE("Raspberry Pi 2/3 found")
	else()
		# Use GLESv1_CM for RPi 4
		find_library(GLES_LIBRARY NAMES libGLESv1_CM.so.1 PATHS "/usr/lib/arm-linux-gnueabihf/")
		MESSAGE("Raspberry Pi 4 found")
	endif()
else()
	find_path(GLES_INCLUDE_DIR GLES/gl.h)
	find_library(GLES_LIBRARY NAMES GLESv1_CM)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLES DEFAULT_MSG GLES_LIBRARY GLES_INCLUDE_DIR)
