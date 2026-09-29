#-----------------------------------------------------------------
# gl4es - desktop OpenGL 1.x/2.x over GLES2/WebGL
#
# For Emscripten client builds, gl4es translates the OpenGL1 renderer's
# desktop GL calls to WebGL/GLES2, replacing Emscripten's own
# LEGACY_GL_EMULATION. gl4es is fetched and built as a static library
# (libGL.a) via ExternalProject and exposed through the bundled_gl4es_int
# interface target.
#-----------------------------------------------------------------

include(ExternalProject)
include(CheckIncludeFile)

add_library(bundled_gl4es_int INTERFACE)

set(GL4ES_PREFIX "${LIBS_BINARY_DIR}/gl4es")
set(GL4ES_SOURCE_DIR "${GL4ES_PREFIX}/src/bundled_gl4es")
# gl4es forces its archive output to <source>/lib regardless of the build dir.
set(GL4ES_LIBRARY "${GL4ES_SOURCE_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}GL${CMAKE_STATIC_LIBRARY_SUFFIX}")

set(GL4ES_CMAKE_ARGS
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DSTATICLIB=ON
	-DNOX11=ON
	-DNOEGL=ON
	-DNO_GBM=ON
	-DNO_LOADER=ON
	# No GL context exists at program start for a static lib, so don't
	# auto-initialise via a constructor; the engine calls
	# initialize_gl4es() itself right after SDL_GL_CreateContext().
	-DNO_INIT_CONSTRUCTOR=ON
)

if(CMAKE_TOOLCHAIN_FILE)
	list(APPEND GL4ES_CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()

ExternalProject_Add(bundled_gl4es
	GIT_REPOSITORY https://github.com/ptitSeb/gl4es.git
	GIT_TAG 17f0894e19d1553e4176276c759915dab44c08e2
	PREFIX "${GL4ES_PREFIX}"
	BUILD_BYPRODUCTS "${GL4ES_LIBRARY}"
	CMAKE_ARGS ${GL4ES_CMAKE_ARGS}
	INSTALL_COMMAND ""
)

add_dependencies(bundled_gl4es_int bundled_gl4es)
target_link_libraries(bundled_gl4es_int INTERFACE "${GL4ES_LIBRARY}")
target_include_directories(bundled_gl4es_int INTERFACE "${GL4ES_SOURCE_DIR}/include")
target_compile_definitions(bundled_gl4es_int INTERFACE FEATURE_GL4ES)
