if(WIN32)
	FIND_PATH( GLEW_INCLUDE_PATH GL/glew.h
		$ENV{PROGRAMFILES}/GLEW/include
		${PROJECT_SOURCE_DIR}/src/nvgl/glew/include
		DOC "The directory where GL/glew.h resides"
	)
	FIND_LIBRARY( GLEW_LIBRARY
		NAMES glew GLEW glew32 glew32s
		PATHS
		$ENV{PROGRAMFILES}/GLEW/lib
		${PROJECT_SOURCE_DIR}/src/nvgl/glew/bin
		${PROJECT_SOURCE_DIR}/src/nvgl/glew/lib
		DOC "The GLEW library"
	)
else(WIN32)
	FIND_PATH( GLEW_INCLUDE_PATH GL/glew.h
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		DOC "The directory where GL/glew.h resides"
	)
	FIND_LIBRARY( GLEW_LIBRARY
		NAMES GLEW glew
		PATHS
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		DOC "The GLEW library"
	)
endif(WIN32)

if(GLEW_INCLUDE_PATH)
	set( GLEW_FOUND 1 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
else(GLEW_INCLUDE_PATH)
	set( GLEW_FOUND 0 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
endif(GLEW_INCLUDE_PATH)

MARK_AS_ADVANCED( GLEW_FOUND GLEW_LIBRARY GLEW_INCLUDE_PATH )