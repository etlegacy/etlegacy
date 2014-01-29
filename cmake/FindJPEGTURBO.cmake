# - Find libjpeg-turbo
# Find the native libjpeg-turbo includes and library
# This module defines
#  JPEGTURBO_INCLUDE_DIR, where to find the libjpeg-turbo include directories.
#  JPEGTURBO_LIBRARIES, the libraries needed to use libjpeg-turbo.
#  JPEGTURBO_FOUND, If false, do not try to use libjpeg-turbo.
# also defined, but not for general use are
#  JPEGTURBO_LIBRARY, where to find the libjpeg-turbo library.

find_path(JPEGTURBO_INCLUDE_DIR turbojpeg.h)

set(JPEGTURBO_NAMES ${JPEGTURBO_NAMES} turbojpeg)
find_library(JPEGTURBO_LIBRARY NAMES ${JPEGTURBO_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set JPEGTURBO_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JPEGTURBO DEFAULT_MSG JPEGTURBO_LIBRARY JPEGTURBO_INCLUDE_DIR)

if(JPEGTURBO_FOUND)
  set(JPEGTURBO_LIBRARIES ${JPEGTURBO_LIBRARY})
endif()

mark_as_advanced(JPEGTURBO_LIBRARY JPEGTURBO_INCLUDE_DIR )
