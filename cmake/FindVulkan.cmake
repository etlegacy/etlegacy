# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindVulkan
----------

.. versionadded:: 3.7

Find Vulkan, which is a low-overhead, cross-platform 3D graphics
and computing API.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` targets if Vulkan has been found:

``Vulkan::Vulkan``
  The main Vulkan library.

``Vulkan::glslc``
  .. versionadded:: 3.19

  The GLSLC SPIR-V compiler, if it has been found.

``Vulkan::Headers``
  .. versionadded:: 3.21

  Provides just Vulkan headers include paths, if found.  No library is
  included in this target.  This can be useful for applications that
  load Vulkan library dynamically.

``Vulkan::glslangValidator``
  .. versionadded:: 3.21

  The glslangValidator tool, if found.  It is used to compile GLSL and
  HLSL shaders into SPIR-V.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables::

  VULKAN_FOUND          - "True" if Vulkan was found
  VULKAN_INCLUDE_DIRS   - include directories for Vulkan
  VULKAN_LIBRARIES      - link against this library to use Vulkan

The module will also define three cache variables::

  VULKAN_INCLUDE_DIR        - the Vulkan include directory
  VULKAN_LIBRARY            - the path to the Vulkan library
  VULKAN_GLSLC_EXECUTABLE   - the path to the GLSL SPIR-V compiler
  VULKAN_GLSLANG_VALIDATOR_EXECUTABLE - the path to the glslangValidator tool

Hints
^^^^^

.. versionadded:: 3.18

The ``VULKAN_SDK`` environment variable optionally specifies the
location of the Vulkan SDK root directory for the given
architecture. It is typically set by sourcing the toplevel
``setup-env.sh`` script of the Vulkan SDK directory into the shell
environment.

#]=======================================================================]

if(WIN32)
  find_path(VULKAN_INCLUDE_DIR
    NAMES vulkan/vulkan.h
    HINTS
      "$ENV{VULKAN_SDK}/Include"
    )

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(VULKAN_LIBRARY
      NAMES vulkan-1
      HINTS
        "$ENV{VULKAN_SDK}/Lib"
        "$ENV{VULKAN_SDK}/Bin"
      )
    find_program(VULKAN_GLSLC_EXECUTABLE
      NAMES glslc
      HINTS
        "$ENV{VULKAN_SDK}/Bin"
      )
    find_program(VULKAN_GLSLANG_VALIDATOR_EXECUTABLE
      NAMES glslangValidator
      HINTS
        "$ENV{VULKAN_SDK}/Bin"
      )
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    find_library(VULKAN_LIBRARY
      NAMES vulkan-1
      HINTS
        "$ENV{VULKAN_SDK}/Lib32"
        "$ENV{VULKAN_SDK}/Bin32"
      )
    find_program(VULKAN_GLSLC_EXECUTABLE
      NAMES glslc
      HINTS
        "$ENV{VULKAN_SDK}/Bin32"
      )
    find_program(VULKAN_GLSLANG_VALIDATOR_EXECUTABLE
      NAMES glslangValidator
      HINTS
        "$ENV{VULKAN_SDK}/Bin32"
      )
  endif()
else()
  find_path(VULKAN_INCLUDE_DIR
    NAMES vulkan/vulkan.h
    HINTS "$ENV{VULKAN_SDK}/include")
  find_library(VULKAN_LIBRARY
    NAMES vulkan
    HINTS "$ENV{VULKAN_SDK}/lib")
  find_program(VULKAN_GLSLC_EXECUTABLE
    NAMES glslc
    HINTS "$ENV{VULKAN_SDK}/bin")
  find_program(VULKAN_GLSLANG_VALIDATOR_EXECUTABLE
    NAMES glslangValidator
    HINTS "$ENV{VULKAN_SDK}/bin")
endif()

set(VULKAN_LIBRARIES ${VULKAN_LIBRARY})
set(VULKAN_INCLUDE_DIRS ${VULKAN_INCLUDE_DIR})

find_package_handle_standard_args(Vulkan
  DEFAULT_MSG
  VULKAN_LIBRARY VULKAN_INCLUDE_DIR)

mark_as_advanced(VULKAN_INCLUDE_DIR VULKAN_LIBRARY VULKAN_GLSLC_EXECUTABLE
                 VULKAN_GLSLANG_VALIDATOR_EXECUTABLE)

if(VULKAN_FOUND AND NOT TARGET Vulkan::Vulkan)
  add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
  set_target_properties(Vulkan::Vulkan PROPERTIES
    IMPORTED_LOCATION "${VULKAN_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_INCLUDE_DIRS}")
endif()

if(VULKAN_FOUND AND NOT TARGET Vulkan::Headers)
  add_library(Vulkan::Headers INTERFACE IMPORTED)
  set_target_properties(Vulkan::Headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_INCLUDE_DIRS}")
endif()

if(VULKAN_FOUND AND VULKAN_GLSLC_EXECUTABLE AND NOT TARGET Vulkan::glslc)
  add_executable(Vulkan::glslc IMPORTED)
  set_property(TARGET Vulkan::glslc PROPERTY IMPORTED_LOCATION "${VULKAN_GLSLC_EXECUTABLE}")
endif()

if(VULKAN_FOUND AND VULKAN_GLSLANG_VALIDATOR_EXECUTABLE AND NOT TARGET Vulkan::glslangValidator)
  add_executable(Vulkan::glslangValidator IMPORTED)
  set_property(TARGET Vulkan::glslangValidator PROPERTY IMPORTED_LOCATION "${VULKAN_GLSLANG_VALIDATOR_EXECUTABLE}")
endif()
