# - Find libjpeg-turbo
# Find the native libjpeg-turbo includes and library
# This module defines
#  JPEG_INCLUDE_DIR, where to find the libjpeg-turbo include directories.
#  JPEG_LIBRARIES, the libraries needed to use libjpeg-turbo.
#  JPEGTURBO_FOUND, If false, do not try to use libjpeg-turbo.
# also defined, but not for general use are
#  JPEG_LIBRARY, where to find the libjpeg-turbo library.

find_path(JPEG_INCLUDE_DIR turbojpeg.h)

set(JPEG_NAMES ${JPEG_NAMES} jpeg)
find_library(JPEG_LIBRARY NAMES ${JPEG_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set JPEGTURBO_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JPEGTURBO DEFAULT_MSG JPEG_LIBRARY JPEG_INCLUDE_DIR)

if(JPEGTURBO_FOUND)
  set(JPEG_LIBRARIES ${JPEG_LIBRARY})
endif()

mark_as_advanced(JPEG_LIBRARY JPEG_INCLUDE_DIR )
