# Compiles every Vulkan renderer shader to SPIR-V (<OUT_DIR>/<name>.spv)
# without configuring the project. SPIR-V doesn't depend on the target
# platform, so the CI compiles the shaders once in a job of its own and the
# build jobs, whose build machines have no GLSL compiler, use the result
# through VK_SHADER_PREBUILT_DIR.
#
# Expected to be invoked in script mode from the source directory:
#   cmake -DVK_GLSL_COMPILER=glslc -DOUT_DIR=vk-shaders -P cmake/ETLCompileVkShaders.cmake

if(NOT VK_GLSL_COMPILER OR NOT OUT_DIR)
	message(FATAL_ERROR "ETLCompileVkShaders.cmake requires VK_GLSL_COMPILER and OUT_DIR to be set")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/ETLVkShaders.cmake")

get_filename_component(VK_SHADER_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/../src/renderer_vk/glsl" ABSOLUTE)
file(MAKE_DIRECTORY "${OUT_DIR}")

foreach(VK_SHADER_SRC_NAME ${VK_SHADER_NAMES})
	etl_vk_shader_stage(${VK_SHADER_SRC_NAME} VK_SHADER_STAGE)
	etl_vk_glsl_args("${VK_GLSL_COMPILER}" ${VK_SHADER_STAGE} VK_GLSL_ARGS)

	message(STATUS "Compiling Vulkan shader ${VK_SHADER_SRC_NAME}")
	execute_process(
		COMMAND "${VK_GLSL_COMPILER}" ${VK_GLSL_ARGS} "${VK_SHADER_SRC_DIR}/${VK_SHADER_SRC_NAME}" -o "${OUT_DIR}/${VK_SHADER_SRC_NAME}.spv"
		RESULT_VARIABLE VK_GLSL_RESULT
	)
	if(NOT VK_GLSL_RESULT EQUAL 0)
		message(FATAL_ERROR "Compiling ${VK_SHADER_SRC_NAME} failed (${VK_GLSL_RESULT})")
	endif()
endforeach()
