# - Find libjpeg-turbo
# Find the native libjpeg-turbo includes and library
# This module defines
#  JPEG_INCLUDE_DIR, where to find the libjpeg-turbo include directories.
#  JPEG_LIBRARIES, the libraries needed to use libjpeg-turbo.
#  JPEGTURBO_FOUND, If false, do not try to use libjpeg-turbo.
# also defined, but not for general use are
#  JPEG_LIBRARY, where to find the libjpeg-turbo library.

find_path(JPEG_INCLUDE_DIR turbojpeg.h
	/usr/include
	/usr/local/include
	/sw/include
	/opt/local/include
	DOC "The directory where turbojpeg.h resides"
)

find_library(JPEG_LIBRARY
	NAMES ${JPEG_NAMES} jpeg
	PATHS
	/usr/lib64
	/usr/lib
	/usr/local/lib64
	/usr/local/lib
	/sw/lib
	/opt/local/lib
	DOC "The JPEG library"
)

# Determine libjpeg-turbo version
if(JPEG_INCLUDE_DIR AND EXISTS "${JPEG_INCLUDE_DIR}/jconfig.h")
	file(STRINGS "${JPEG_INCLUDE_DIR}/jconfig.h" jpegturbo_version_str REGEX "^#define LIBJPEG_TURBO_VERSION[ ]+[0-9].[0-9].[0-9]")
	string(REGEX REPLACE "^#define LIBJPEG_TURBO_VERSION[ ]+([^\"]*).*" "\\1" JPEGTURBO_VERSION_STRING "${jpegturbo_version_str}")
	unset(jpegturbo_version_str)
endif()

# handle the QUIETLY and REQUIRED arguments and set JPEGTURBO_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JPEGTURBO
	REQUIRED_VARS JPEG_LIBRARY JPEG_INCLUDE_DIR
	VERSION_VAR JPEGTURBO_VERSION_STRING)

if(JPEGTURBO_FOUND)
	set(JPEG_LIBRARIES ${JPEG_LIBRARY})
endif()
