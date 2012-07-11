# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR "x86")

# Choose an appropriate compiler prefix
# see http://www.mingw.org/
set(COMPILER_PREFIX "i686-pc-mingw32")

# which compilers to use for C and C++
find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
set(CMAKE_C_COMPILER ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
set(CMAKE_CXX_COMPILER ${COMPILER_PREFIX}-g++)

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH  /usr/${COMPILER_PREFIX})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)