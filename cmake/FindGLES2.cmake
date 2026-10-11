#
# Try to find GLES library and include path.
# Once done this will define
#
# GLES2_FOUND
# GLES2_INCLUDE_PATH
# GLES2_LIBRARY
#

find_path(GLES2_INCLUDE_DIR GLES2/gl2.h)
find_library(GLES2_LIBRARY NAMES GLESv2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLES2 DEFAULT_MSG GLES2_LIBRARY GLES2_INCLUDE_DIR)
