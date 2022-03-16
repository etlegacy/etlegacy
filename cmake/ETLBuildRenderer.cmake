#-----------------------------------------------------------------
# Build Renderer
#-----------------------------------------------------------------

if(NOT APPLE)
	set(R1_NAME renderer_opengl1_${ARCH})
	set(R2_NAME renderer_opengl2_${ARCH})
	set(VK_NAME renderer_vulkan${ARCH})
else()
	set(R1_NAME renderer_opengl1${LIB_SUFFIX})
	set(R2_NAME renderer_opengl2${LIB_SUFFIX})
endif()

if(RENDERER_DYNAMIC)
	MESSAGE("Will build dynamic renderer libraries")
	add_definitions( "-DUSE_RENDERER_DLOPEN" )
	set(REND_LIBTYPE MODULE)
	list(APPEND RENDERER_COMMON ${RENDERER_COMMON_DYNAMIC})
else()
	set(REND_LIBTYPE STATIC)
endif()

if(RENDERER_DYNAMIC OR NOT FEATURE_RENDERER2)

	if(FEATURE_RENDERER_GLES)
		add_library(${R1_NAME} ${REND_LIBTYPE} ${RENDERERGLES_FILES} ${RENDERER_COMMON})
	else()
		add_library(${R1_NAME} ${REND_LIBTYPE} ${RENDERER1_FILES} ${RENDERER_COMMON})
	endif()

	if(NOT FEATURE_RENDERER_GLES)
		if(BUNDLED_GLEW)
			add_dependencies(${R1_NAME} bundled_glew)
		endif(BUNDLED_GLEW)
	endif()

	if(BUNDLED_JPEG)
		add_dependencies(${R1_NAME} bundled_jpeg)
	endif()

	if(BUNDLED_FREETYPE)
		add_dependencies(${R1_NAME} bundled_freetype)
	endif()

	if(MSVC)
		target_link_libraries(${R1_NAME} ${RENDERER_LIBRARIES})
	else()
		target_link_libraries(${R1_NAME} ${RENDERER_LIBRARIES} 'm')
	endif(MSVC)

	# install the dynamic lib only
	if(RENDERER_DYNAMIC)
		set_target_properties(${R1_NAME}
			PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY ""
			LIBRARY_OUTPUT_DIRECTORY_DEBUG ""
			LIBRARY_OUTPUT_DIRECTORY_RELEASE ""
		)

		if(WIN32)
			set_target_properties(${R1_NAME} PROPERTIES PREFIX "")
		endif(WIN32)

		if(WIN32)
			install(TARGETS ${R1_NAME}
				LIBRARY DESTINATION "${INSTALL_DEFAULT_BINDIR}"
				ARCHIVE DESTINATION "${INSTALL_DEFAULT_BINDIR}"
			)
		else()
			install(TARGETS ${R1_NAME}
				LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}"
				ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}"
			)
		endif()
	endif()

	if(NOT RENDERER_DYNAMIC)
		list(APPEND CLIENT_LIBRARIES ${R1_NAME})
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

	add_library(${R2_NAME} ${REND_LIBTYPE} ${RENDERER2_FILES} ${RENDERER_COMMON} ${RENDERER2_SHADERS})
	add_dependencies(${R2_NAME} r2_shader_compile)

	if(BUNDLED_GLEW)
		add_dependencies(${R2_NAME} bundled_glew)
	endif()

	if(BUNDLED_JPEG)
		add_dependencies(${R2_NAME} bundled_jpeg)
	endif()

	if(BUNDLED_FREETYPE)
		add_dependencies(${R2_NAME} bundled_freetype)
	endif()

	if(MSVC)
		target_link_libraries(${R2_NAME} ${RENDERER_LIBRARIES})
	else()
		target_link_libraries(${R2_NAME} ${RENDERER_LIBRARIES} 'm')
	endif(MSVC)

	set_target_properties(${R2_NAME}
		PROPERTIES COMPILE_DEFINITIONS "FEATURE_RENDERER2"
		LIBRARY_OUTPUT_DIRECTORY ""
		LIBRARY_OUTPUT_DIRECTORY_DEBUG ""
		LIBRARY_OUTPUT_DIRECTORY_RELEASE ""
	)

	if(WIN32)
		set_target_properties(${R2_NAME} PROPERTIES PREFIX "")
	endif(WIN32)

	target_compile_definitions(${R2_NAME} PUBLIC USE_REFENTITY_ANIMATIONSYSTEM=1)

	if(WIN32)
		install(TARGETS ${R2_NAME}
			LIBRARY DESTINATION "${INSTALL_DEFAULT_BINDIR}"
			ARCHIVE DESTINATION "${INSTALL_DEFAULT_BINDIR}"
		)
	else()
		install(TARGETS ${R2_NAME}
			LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}"
			ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		)
	endif()

	if(NOT RENDERER_DYNAMIC)
		list(APPEND CLIENT_LIBRARIES ${R2_NAME})
	endif()
endif(FEATURE_RENDERER2)

if(FEATURE_RENDERER_VULKAN)
	#Include slighly modified FindVulkan.cmake file
	include(cmake/FindVulkan.cmake)

	foreach(SHADERS IN LISTS ${RENDERER_VULKAN_SHADERS})
		GET_FILENAME_COMPONENT(SHAD_NAME ${SHADERS} NAME)
		add_custom_command(
			COMMAND
				${VULKAN_GLSLC_EXECUTABLE} ${SHAD_NAME} -o "${CMAKE_CURRENT_BINARY_DIR}/glsl/${SHAD_NAME}.spv"
				OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/glsl/${SHAD_NAME}.spv"
				DEPENDS ${VULKAN_GLSLC_EXECUTABLE}
				COMMENT "Compiling ${SHAD_NAME}"
				)
		list(APPEND SPV_SHADERS "${CMAKE_CURRENT_BINARY_DIR}/glsl/${SHAD_NAME}.spv")
	endforeach()

	add_custom_target(vulkan_shaders ALL DEPENDS ${SPV_SHADERS})

	add_library(${VK_NAME} ${REND_LIBTYPE} ${RENDERER_VULKAN_FILES} ${RENDERER_COMMON} ${RENDERER_VULKAN_SPIRV_SHADERS})
	add_dependencies(${VK_NAME} vulkan_shaders)

	if(BUNDLED_JPEG)
		add_dependencies(${VK_NAME} bundled_jpeg)
	endif()

	if(BUNDLED_FREETYPE)
		add_dependencies(${VK_NAME} bundled_freetype)
	endif()

	if(MSVC)
		target_link_libraries(${VK_NAME} ${RENDERER_LIBRARIES})
	else()
		target_link_libraries(${VK_NAME} ${RENDERER_LIBRARIES} 'm')
	endif(MSVC)

	set_target_properties(${VK_NAME}
		PROPERTIES COMPILE_DEFINITIONS "FEATURE_RENDERER_VULKAN"
		LIBRARY_OUTPUT_DIRECTORY ""
		LIBRARY_OUTPUT_DIRECTORY_DEBUG ""
		LIBRARY_OUTPUT_DIRECTORY_RELEASE ""
	)

	if(WIN32)
		set_target_properties(${VK_NAME} PROPERTIES PREFIX "")
	endif(WIN32)

	if(WIN32)
		install(TARGETS ${VK_NAME}
			LIBRARY DESTINATION "${INSTALL_DEFAULT_BINDIR}"
			ARCHIVE DESTINATION "${INSTALL_DEFAULT_BINDIR}"
		)
	else()
		install(TARGETS ${VK_NAME}
			LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}"
			ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}"
		)
	endif()

	if(NOT RENDERER_DYNAMIC)
		list(APPEND CLIENT_LIBRARIES ${VK_NAME})
	endif()
endif(FEATURE_RENDERER_VULKAN)
