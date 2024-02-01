#-------------------------------------------------------------------------
# Toolchain Cross Arm Linux (depends on the gcc-aarch64-linux-gnu package)
#-------------------------------------------------------------------------

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_SIZEOF_VOID_P 8)

set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH  /usr/${COMPILER_PREFIX})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
if(NOT CMAKE_FIND_ROOT_PATH_MODE_PROGRAM)
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_INCLUDE)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_PACKAGE)
	set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

if(LEGACY_ARM_ARCH)
	set(CMAKE_C_FLAGS "-march=${LEGACY_ARM_ARCH}")
	set(CMAKE_CXX_FLAGS "-march=${LEGACY_ARM_ARCH}")
else()
	set(CMAKE_C_FLAGS "-march=armv8-a")
	set(CMAKE_CXX_FLAGS "-march=armv8-a")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nodefaultlibs -fno-builtin -fno-stack-protector -nostdinc++ -mno-outline-atomics -lc")

# cache flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "c++ flags")
