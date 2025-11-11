# FindLuaJIT.cmake
# Sets:
#   LUAJIT_FOUND
#   LUAJIT_LIBRARIES
#   LUAJIT_INCLUDE_DIRS
#   LUAJIT_VERSION_STRING (best-effort)

include(FindPackageHandleStandardArgs)
find_package(PkgConfig QUIET)

# Try pkg-config first (names vary by distro)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_LUAJIT QUIET luajit luajit-5.1 luajit-2.1)
endif()

# Includes (we require luajit.h to ensure this is *LuaJIT*, not PUC Lua)
find_path(LUAJIT_INCLUDE_DIR
    NAMES luajit.h
    HINTS
        ${PC_LUAJIT_INCLUDE_DIRS}
    PATH_SUFFIXES
        luajit-2.1
        include
)

# Library names vary by OS/distro
find_library(LUAJIT_LIBRARY
    NAMES
        luajit-5.1
        luajit-2.1
        luajit
    HINTS
        ${PC_LUAJIT_LIBRARY_DIRS}
)

# Optional version string extraction
if (EXISTS "${LUAJIT_INCLUDE_DIR}/luajit.h")
    file(STRINGS "${LUAJIT_INCLUDE_DIR}/luajit.h" _lj_ver_line
         REGEX "^#define[ \t]+LUAJIT_VERSION[ \t]+\"[^\"]+\"")
    string(REGEX REPLACE ".*LUAJIT_VERSION[ \t]+\"([^\"]+)\".*" "\\1" LUAJIT_VERSION_STRING "${_lj_ver_line}")
endif()

set(LUAJIT_INCLUDE_DIRS "${LUAJIT_INCLUDE_DIR}")
set(LUAJIT_LIBRARIES    "${LUAJIT_LIBRARY}")

find_package_handle_standard_args(LuaJIT
    REQUIRED_VARS LUAJIT_INCLUDE_DIR LUAJIT_LIBRARY
    VERSION_VAR   LUAJIT_VERSION_STRING)

mark_as_advanced(LUAJIT_INCLUDE_DIR LUAJIT_LIBRARY)
