#-----------------------------------------------------------------
# Build Renderer
#-----------------------------------------------------------------

if(NOT APPLE)
	set(R1_NAME renderer_opengl1_${ARCH})
	set(R2_NAME renderer_opengl2_${ARCH})
	set(R_ES_NAME renderer_opengles_${ARCH})
	set(R_VULKAN_NAME renderer_vulkan_${ARCH})
else()
	set(R1_NAME renderer_opengl1${LIB_SUFFIX})
	set(R2_NAME renderer_opengl2${LIB_SUFFIX})
	set(R_ES_NAME renderer_opengles${LIB_SUFFIX})
	set(R_VULKAN_NAME renderer_vulkan${LIB_SUFFIX})
endif()

set(RENDERERS_BUILDING)
if(FEATURE_RENDERER1)
	list(APPEND RENDERERS_BUILDING "opengl1")
endif()

if(FEATURE_RENDERER2)
	list(APPEND RENDERERS_BUILDING "opengl2")
endif()

if(FEATURE_RENDERER_GLES)
	list(APPEND RENDERERS_BUILDING "opengles")
endif()

if(FEATURE_RENDERER_VULKAN)
	list(APPEND RENDERERS_BUILDING "vulkan")
endif()

if(RENDERERS_BUILDING)
	message(STATUS "Building renderers: ${RENDERERS_BUILDING}")
	list(LENGTH RENDERERS_BUILDING RENDERERS_COUNT)
	if(RENDERERS_COUNT GREATER 1 AND NOT RENDERER_DYNAMIC)
		message(FATAL_ERROR "Only one renderer can be built at a time when building static libraries")
	elseif(RENDERERS_COUNT EQUAL 1 AND RENDERER_DYNAMIC)
		message(NOTICE "Only one renderer is being built, dynamic libraries is not necessary")
	endif()

	# Set the default renderer name for the client code
	list(GET RENDERERS_BUILDING 0 RENDERER_NAME)
	target_compile_definitions(client_libraries INTERFACE DEFAULT_RENDERER_NAME="${RENDERER_NAME}")
else()
	message(FATAL_ERROR "No renderers selected to build")
endif()

macro(configure_renderer renderer name)
	# install the dynamic lib only
	if(RENDERER_DYNAMIC)
		set_target_properties(${renderer}
			PROPERTIES
			OUTPUT_NAME "${name}"
			LIBRARY_OUTPUT_DIRECTORY ""
			LIBRARY_OUTPUT_DIRECTORY_DEBUG ""
			LIBRARY_OUTPUT_DIRECTORY_RELEASE ""
		)

		if(WIN32)
			set_target_properties(${renderer} PROPERTIES PREFIX "")
		endif(WIN32)

		if(WIN32)
			install(TARGETS ${renderer}
				LIBRARY DESTINATION "${INSTALL_DEFAULT_BINDIR}"
				ARCHIVE DESTINATION "${INSTALL_DEFAULT_BINDIR}"
			)
		else()
			install(TARGETS ${renderer}
				LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}"
				ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}"
			)
		endif()
	endif()

	if(NOT RENDERER_DYNAMIC)
		target_link_libraries(client_libraries INTERFACE ${renderer})
	endif()
endmacro()

if(RENDERER_DYNAMIC)
	message(STATUS "Will build dynamic renderer libraries")
	target_compile_definitions(client_libraries INTERFACE USE_RENDERER_DLOPEN)
	target_compile_definitions(renderer_libraries INTERFACE USE_RENDERER_DLOPEN)
	set(REND_LIBTYPE MODULE)
	list(APPEND RENDERER_COMMON ${RENDERER_COMMON_DYNAMIC})
else()
	set(REND_LIBTYPE STATIC)
endif()

if(FEATURE_RENDERER1)
	add_library(renderer1 ${REND_LIBTYPE} ${RENDERER1_FILES} ${RENDERER_COMMON} ${RENDERER_COMMON_OPENGL})

	target_link_libraries(renderer1 renderer_gl1_libraries renderer_libraries)
	target_include_directories(renderer1 PRIVATE src/renderer)

	if(NOT MSVC)
		target_link_libraries(renderer1 m)
	endif()

	configure_renderer(renderer1 ${R1_NAME})
endif()

if(FEATURE_RENDERER_GLES)
	add_library(renderer_es ${REND_LIBTYPE} ${RENDERERGLES_FILES} ${RENDERER_COMMON} ${RENDERER_COMMON_OPENGL})
	target_compile_definitions(renderer_es PRIVATE FEATURE_RENDERER_GLES)

	target_link_libraries(renderer_es renderer_gles_libraries renderer_libraries)
	target_include_directories(renderer_es PRIVATE src/rendererGLES)

	if(NOT MSVC)
		target_link_libraries(renderer_es m)
	endif()

	configure_renderer(renderer_es ${R_ES_NAME})
endif()

if(FEATURE_RENDERER2)
	if(MSVC)
		list(APPEND RENDERER2_FILES ${RENDERER2_SHADERS})
	endif()

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
		"src/renderer2/glsl/shdr/*.*"
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

	add_library(renderer2 ${REND_LIBTYPE} ${RENDERER2_FILES} ${RENDERER_COMMON} ${RENDERER_COMMON_OPENGL} ${RENDERER2_SHADERS})
	add_dependencies(renderer2 r2_shader_compile)
	target_link_libraries(renderer2 renderer_gl2_libraries renderer_libraries)
	target_compile_definitions(renderer2 PRIVATE FEATURE_RENDERER2)
	target_compile_definitions(renderer2 PUBLIC USE_REFENTITY_ANIMATIONSYSTEM=1)
	target_include_directories(renderer2 PRIVATE src/renderer2)

	if(NOT MSVC)
		target_link_libraries(renderer2 renderer_libraries m)
	endif()

	configure_renderer(renderer2 ${R2_NAME})
endif()

if(FEATURE_RENDERER_VULKAN)
	# Compile the GLSL UI shaders to SPIR-V, then embed each .spv binary into a
	# generated C header (static const unsigned char[]) so the renderer stays
	# self-contained and doesn't need runtime filesystem/pk3 access to load them.
	set(VK_SHADER_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/include/vk_shaders")
	file(MAKE_DIRECTORY "${VK_SHADER_GEN_DIR}")

	set(VK_SHADER_HEADERS "")
	set(VK_SHADER_NAMES "ui.vert.glsl" "ui.frag.glsl" "tr3d.vert.glsl" "tr3d.frag.glsl" "tr3d_lm.vert.glsl" "tr3d_lm.frag.glsl" "tr3d_static.vert.glsl" "tr3d_lm_static.vert.glsl" "tr3d_diffuse.vert.glsl" "tr3d_skin.vert.glsl" "tr3d_md3.vert.glsl" "tr3d_lm_static_dlight.vert.glsl" "tr3d_lm_dlight.frag.glsl" "tr3d_2stage.vert.glsl" "tr3d_2stage.frag.glsl" "ui_bindless.vert.glsl" "ui_bindless.frag.glsl" "gamma.vert.glsl" "gamma.frag.glsl")
	set(VK_SHADER_ARRAY_NAME_ui.vert.glsl "ui_vert_spv")
	set(VK_SHADER_ARRAY_NAME_ui.frag.glsl "ui_frag_spv")
	set(VK_SHADER_STAGE_ui.vert.glsl "vertex")
	set(VK_SHADER_STAGE_ui.frag.glsl "fragment")
	set(VK_SHADER_ARRAY_NAME_tr3d.vert.glsl "tr3d_vert_spv")
	set(VK_SHADER_ARRAY_NAME_tr3d.frag.glsl "tr3d_frag_spv")
	set(VK_SHADER_STAGE_tr3d.vert.glsl "vertex")
	set(VK_SHADER_STAGE_tr3d.frag.glsl "fragment")
	set(VK_SHADER_ARRAY_NAME_tr3d_lm.vert.glsl "tr3d_lm_vert_spv")
	set(VK_SHADER_ARRAY_NAME_tr3d_lm.frag.glsl "tr3d_lm_frag_spv")
	set(VK_SHADER_STAGE_tr3d_lm.vert.glsl "vertex")
	set(VK_SHADER_STAGE_tr3d_lm.frag.glsl "fragment")
	set(VK_SHADER_ARRAY_NAME_tr3d_static.vert.glsl "tr3d_static_vert_spv")
	set(VK_SHADER_STAGE_tr3d_static.vert.glsl "vertex")
	set(VK_SHADER_ARRAY_NAME_tr3d_diffuse.vert.glsl "tr3d_diffuse_vert_spv")
	set(VK_SHADER_STAGE_tr3d_diffuse.vert.glsl "vertex")
	set(VK_SHADER_ARRAY_NAME_tr3d_skin.vert.glsl "tr3d_skin_vert_spv")
	set(VK_SHADER_STAGE_tr3d_skin.vert.glsl "vertex")
	set(VK_SHADER_ARRAY_NAME_tr3d_md3.vert.glsl "tr3d_md3_vert_spv")
	set(VK_SHADER_STAGE_tr3d_md3.vert.glsl "vertex")
	set(VK_SHADER_ARRAY_NAME_tr3d_lm_static_dlight.vert.glsl "tr3d_lm_static_dlight_vert_spv")
	set(VK_SHADER_STAGE_tr3d_lm_static_dlight.vert.glsl "vertex")
	set(VK_SHADER_ARRAY_NAME_tr3d_lm_dlight.frag.glsl "tr3d_lm_dlight_frag_spv")
	set(VK_SHADER_STAGE_tr3d_lm_dlight.frag.glsl "fragment")
	set(VK_SHADER_ARRAY_NAME_tr3d_2stage.vert.glsl "tr3d_2stage_vert_spv")
	set(VK_SHADER_STAGE_tr3d_2stage.vert.glsl "vertex")
	set(VK_SHADER_ARRAY_NAME_tr3d_2stage.frag.glsl "tr3d_2stage_frag_spv")
	set(VK_SHADER_STAGE_tr3d_2stage.frag.glsl "fragment")
	set(VK_SHADER_ARRAY_NAME_ui_bindless.vert.glsl "ui_bindless_vert_spv")
	set(VK_SHADER_STAGE_ui_bindless.vert.glsl "vertex")
	set(VK_SHADER_ARRAY_NAME_ui_bindless.frag.glsl "ui_bindless_frag_spv")
	set(VK_SHADER_STAGE_ui_bindless.frag.glsl "fragment")
	set(VK_SHADER_ARRAY_NAME_tr3d_lm_static.vert.glsl "tr3d_lm_static_vert_spv")
	set(VK_SHADER_STAGE_tr3d_lm_static.vert.glsl "vertex")
	set(VK_SHADER_ARRAY_NAME_gamma.vert.glsl "gamma_vert_spv")
	set(VK_SHADER_ARRAY_NAME_gamma.frag.glsl "gamma_frag_spv")
	set(VK_SHADER_STAGE_gamma.vert.glsl "vertex")
	set(VK_SHADER_STAGE_gamma.frag.glsl "fragment")

	foreach(VK_SHADER_SRC_NAME ${VK_SHADER_NAMES})
		set(VK_SHADER_ARRAY_NAME "${VK_SHADER_ARRAY_NAME_${VK_SHADER_SRC_NAME}}")
		set(VK_SHADER_STAGE "${VK_SHADER_STAGE_${VK_SHADER_SRC_NAME}}")

		set(VK_SHADER_SRC "${CMAKE_SOURCE_DIR}/src/renderer_vk/glsl/${VK_SHADER_SRC_NAME}")
		set(VK_SHADER_SPV "${VK_SHADER_GEN_DIR}/${VK_SHADER_SRC_NAME}.spv")
		set(VK_SHADER_HDR "${VK_SHADER_GEN_DIR}/${VK_SHADER_SRC_NAME}.h")

		get_filename_component(VK_GLSL_COMPILER_NAME "${VK_GLSL_COMPILER}" NAME_WE)
		if(VK_GLSL_COMPILER_NAME STREQUAL "glslangValidator")
			if(VK_SHADER_STAGE STREQUAL "vertex")
				set(VK_GLSL_ARGS -V -S vert)
			else()
				set(VK_GLSL_ARGS -V -S frag)
			endif()
		else()
			set(VK_GLSL_ARGS -fshader-stage=${VK_SHADER_STAGE})
		endif()

		add_custom_command(
			OUTPUT ${VK_SHADER_SPV}
			COMMAND ${VK_GLSL_COMPILER} ${VK_GLSL_ARGS} ${VK_SHADER_SRC} -o ${VK_SHADER_SPV}
			DEPENDS ${VK_SHADER_SRC}
			COMMENT "Compiling Vulkan shader ${VK_SHADER_SRC_NAME}"
			VERBATIM
		)
		add_custom_command(
			OUTPUT ${VK_SHADER_HDR}
			COMMAND ${CMAKE_COMMAND} -DIN_FILE=${VK_SHADER_SPV} -DOUT_FILE=${VK_SHADER_HDR} -DARRAY_NAME=${VK_SHADER_ARRAY_NAME} -P ${CMAKE_SOURCE_DIR}/cmake/ETLBin2H.cmake
			DEPENDS ${VK_SHADER_SPV} ${CMAKE_SOURCE_DIR}/cmake/ETLBin2H.cmake
			COMMENT "Embedding ${VK_SHADER_SRC_NAME}.spv into ${VK_SHADER_SRC_NAME}.h"
			VERBATIM
		)

		list(APPEND VK_SHADER_HEADERS ${VK_SHADER_HDR})
	endforeach()

	add_custom_target(vk_shader_compile DEPENDS ${VK_SHADER_HEADERS})
	set_target_properties(vk_shader_compile PROPERTIES FOLDER Tools)

	add_library(renderer_vulkan ${REND_LIBTYPE} ${RENDERER_VULKAN_FILES} ${RENDERER_COMMON} ${RENDERER_COMMON_VULKAN} ${RENDERER_VULKAN_SHADERS})
	add_dependencies(renderer_vulkan vk_shader_compile)
	target_link_libraries(renderer_vulkan renderer_vulkan_libraries renderer_libraries)
	target_compile_definitions(renderer_vulkan PRIVATE FEATURE_RENDERER_VULKAN)
	target_compile_definitions(client_libraries INTERFACE FEATURE_RENDERER_VULKAN)
	target_include_directories(renderer_vulkan PRIVATE src/renderer_vk ${VK_SHADER_GEN_DIR})

	if(NOT MSVC)
		target_link_libraries(renderer_vulkan renderer_libraries m)
	endif()

	configure_renderer(renderer_vulkan ${R_VULKAN_NAME})
endif()
