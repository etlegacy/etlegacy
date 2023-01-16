#-----------------------------------------------------------------
# Build Renderer
#-----------------------------------------------------------------

if(NOT APPLE)
	set(R1_NAME renderer_opengl1_${ARCH})
	set(R2_NAME renderer_opengl2_${ARCH})
else()
	set(R1_NAME renderer_opengl1${LIB_SUFFIX})
	set(R2_NAME renderer_opengl2${LIB_SUFFIX})
endif()

if(RENDERER_DYNAMIC)
	MESSAGE("Will build dynamic renderer libraries")
	target_compile_definitions(client_libraries INTERFACE USE_RENDERER_DLOPEN)
	target_compile_definitions(renderer_libraries INTERFACE USE_RENDERER_DLOPEN)
	set(REND_LIBTYPE MODULE)
	list(APPEND RENDERER_COMMON ${RENDERER_COMMON_DYNAMIC})
else()
	set(REND_LIBTYPE STATIC)
endif()

if(RENDERER_DYNAMIC OR NOT FEATURE_RENDERER2)

	if(FEATURE_RENDERER_GLES)
		add_library(renderer1 ${REND_LIBTYPE} ${RENDERERGLES_FILES} ${RENDERER_COMMON})
		target_compile_definitions(renderer1 PRIVATE FEATURE_RENDERER_GLES)
	else()
		add_library(renderer1 ${REND_LIBTYPE} ${RENDERER1_FILES} ${RENDERER_COMMON})
	endif()

	if(MSVC)
		target_link_libraries(renderer1 renderer_libraries)
	else()
		target_link_libraries(renderer1 renderer_libraries m)
	endif(MSVC)

	set_target_properties(renderer1
		PROPERTIES
		OUTPUT_NAME "${R1_NAME}"
	)

	# install the dynamic lib only
	if(RENDERER_DYNAMIC)
		set_target_properties(renderer1
			PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY ""
			LIBRARY_OUTPUT_DIRECTORY_DEBUG ""
			LIBRARY_OUTPUT_DIRECTORY_RELEASE ""
		)

		if(WIN32)
			set_target_properties(renderer1 PROPERTIES PREFIX "")
		endif(WIN32)

		if(WIN32)
			install(TARGETS renderer1
				LIBRARY DESTINATION "${INSTALL_DEFAULT_BINDIR}"
				ARCHIVE DESTINATION "${INSTALL_DEFAULT_BINDIR}"
			)
		else()
			install(TARGETS renderer1
				LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}"
				ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}"
			)
		endif()
	endif()

	if(NOT RENDERER_DYNAMIC)
		target_link_libraries(client_libraries INTERFACE renderer1)
	endif()
endif()

if(FEATURE_RENDERER2)
	if(MSVC)
		list(APPEND RENDERER2_FILES ${RENDERER2_SHADERS})
	endif(MSVC)

	if(MSVC OR XCODE)
		GET_FILENAME_COMPONENT(GLSL_FULLPATH ${GLSL_PATH} ABSOLUTE)
		FOREACH (SHAD ${RENDERER2_SHADERS})
			#GET_FILENAME_COMPONENT(SHAD_NAME ${SHAD} NAME_WE)
			GET_FILENAME_COMPONENT(SHAD_FOLDER ${SHAD} PATH)

			string(REPLACE "${GLSL_FULLPATH}/" "" SHAD_FOLDER "${SHAD_FOLDER}")
			string(REPLACE "${GLSL_FULLPATH}" "" SHAD_FOLDER "${SHAD_FOLDER}")

			string(REPLACE "/" "\\" SHAD_FOLDER_WIN "${SHAD_FOLDER}")
			source_group("Shaders\\${SHAD_FOLDER_WIN}" FILES ${SHAD})
		ENDFOREACH()
	endif()

	FILE(GLOB SHDR2H_SRC
		"src/tools/shdr/*.*"
	)
	add_executable(shdr2h ${SHDR2H_SRC})

	#This is where we generate the fallback shaders source file.
	add_custom_command(
		OUTPUT
			${CMAKE_CURRENT_BINARY_DIR}/include/__shaders.h # This is a fake output so we can force the process to run every time
			${CMAKE_CURRENT_BINARY_DIR}/include/shaders.h
		COMMAND shdr2h LEGACY ${CMAKE_SOURCE_DIR}/src/renderer2/glsl ${CMAKE_SOURCE_DIR}/src/renderer2/gldef/default.gldef ${CMAKE_CURRENT_BINARY_DIR}/include/shaders.h
		DEPENDS shdr2h
		COMMENT "Generating shaders include file include/shaders.h"
		VERBATIM
	)
	add_custom_target(r2_shader_compile ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include/__shaders.h)

	set_target_properties(shdr2h PROPERTIES FOLDER Tools)
	set_target_properties(r2_shader_compile PROPERTIES FOLDER Tools)

	add_library(renderer2 ${REND_LIBTYPE} ${RENDERER2_FILES} ${RENDERER_COMMON} ${RENDERER2_SHADERS})
	add_dependencies(renderer2 r2_shader_compile)

	if(MSVC)
		target_link_libraries(renderer2 renderer_libraries)
	else()
		target_link_libraries(renderer2 renderer_libraries m)
	endif(MSVC)

	set_target_properties(renderer2
		PROPERTIES
		OUTPUT_NAME "${R2_NAME}"
		COMPILE_DEFINITIONS "FEATURE_RENDERER2"
		LIBRARY_OUTPUT_DIRECTORY ""
		LIBRARY_OUTPUT_DIRECTORY_DEBUG ""
		LIBRARY_OUTPUT_DIRECTORY_RELEASE ""
	)

	if(WIN32)
		set_target_properties(renderer2 PROPERTIES PREFIX "")
	endif(WIN32)

	target_compile_definitions(renderer2 PUBLIC USE_REFENTITY_ANIMATIONSYSTEM=1)

	if(WIN32)
		install(TARGETS renderer2
			LIBRARY DESTINATION "${INSTALL_DEFAULT_BINDIR}"
			ARCHIVE DESTINATION "${INSTALL_DEFAULT_BINDIR}"
		)
	else()
		install(TARGETS renderer2
			LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}"
			ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		)
	endif()

	if(NOT RENDERER_DYNAMIC)
		target_link_libraries(client_libraries INTERFACE renderer2)
	endif()
endif(FEATURE_RENDERER2)
