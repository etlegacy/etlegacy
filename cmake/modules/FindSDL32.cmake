# - Find SDL32
#
# This module was created because the one which comes
# with CMake doesn't find 32bit SDL when crosscompiling
#
# This module defines
#  SDL32_INCLUDE_DIR, where to find SDL32lib.h, etc.
#  SDL32_LIBRARIES, the libraries needed to use SDL32.
#  SDL32_FOUND, If false, do not try to use SDL32.
# also defined, but not for general use are
#  SDL32_LIBRARY, where to find the SDL32 library.

FIND_PATH(SDL32_INCLUDE_DIR SDL/SDL.h)

SET(SDL32_NAMES ${SDL32_NAMES} libSDL sdl SDL)
FIND_LIBRARY(SDL32_LIBRARY NAMES ${SDL32_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set SDL32_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL32 DEFAULT_MSG SDL32_LIBRARY SDL32_INCLUDE_DIR)

IF(SDL32_FOUND)
  SET(SDL32_LIBRARIES ${SDL32_LIBRARY})
ENDIF(SDL32_FOUND)

MARK_AS_ADVANCED(SDL32_LIBRARY SDL32_INCLUDE_DIR )
