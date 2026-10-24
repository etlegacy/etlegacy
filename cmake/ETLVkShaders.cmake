#-----------------------------------------------------------------
# Vulkan renderer shaders (src/renderer_vk/glsl)
#-----------------------------------------------------------------
# Shared by the renderer build (ETLBuildRenderer.cmake) and the standalone
# SPIR-V compile script (ETLCompileVkShaders.cmake, used by the CI). A new
# shader only needs adding to VK_SHADER_NAMES: the stage follows from the
# file name, and the renderer finds it by the name without ".glsl"
# (VK_CreateShaderModule("tr3d_lm.frag"), see ETLEmbedVkShaders.cmake).

set(VK_SHADER_NAMES
	"ui.vert.glsl"
	"ui.frag.glsl"
	"tr3d.vert.glsl"
	"tr3d.frag.glsl"
	"tr3d_lm.vert.glsl"
	"tr3d_lm.frag.glsl"
	"tr3d_static.vert.glsl"
	"tr3d_lm_static.vert.glsl"
	"tr3d_diffuse.vert.glsl"
	"tr3d_skin.vert.glsl"
	"tr3d_md3.vert.glsl"
	"tr3d_lm_static_dlight.vert.glsl"
	"tr3d_lm_dlight.frag.glsl"
	"tr3d_2stage.vert.glsl"
	"tr3d_2stage.frag.glsl"
	"ui_bindless.vert.glsl"
	"ui_bindless.frag.glsl"
	"gamma.vert.glsl"
	"gamma.frag.glsl"
	"world_cull.comp.glsl"
	"world_gather.comp.glsl"
	"tr3d_static_bl.vert.glsl"
	"tr3d_lm_static_bl.vert.glsl"
	"tr3d_bl.frag.glsl"
	"tr3d_lm_bl.frag.glsl"
	"tr3d_md3_bl.vert.glsl"
	"tr3d_skin_bl.vert.glsl"
	"tr3d_static_fog_bl.vert.glsl"
	"tr3d_foliage.vert.glsl"
	"tr3d_foliage_fog.vert.glsl"
)

# etl_vk_shader_stage(<file name> <stage var>)
# The shader stage (vertex/fragment/compute) of a .vert/.frag/.comp.glsl file.
function(etl_vk_shader_stage _NAME _STAGE_VAR)
	string(REGEX REPLACE "\\.glsl$" "" _base "${_NAME}")
	string(REGEX MATCH "[^.]+$" _ext "${_base}")
	if(_ext STREQUAL "vert")
		set(_stage "vertex")
	elseif(_ext STREQUAL "frag")
		set(_stage "fragment")
	elseif(_ext STREQUAL "comp")
		set(_stage "compute")
	else()
		message(FATAL_ERROR "Unknown Vulkan shader stage of ${_NAME} (expected .vert/.frag/.comp.glsl)")
	endif()
	set(${_STAGE_VAR} "${_stage}" PARENT_SCOPE)
endfunction()

# etl_vk_glsl_args(<compiler> <stage> <args var>)
# The stage arguments for glslc (shaderc) or glslangValidator (glslang).
function(etl_vk_glsl_args _COMPILER _STAGE _ARGS_VAR)
	get_filename_component(_compiler_name "${_COMPILER}" NAME_WE)
	if(_compiler_name STREQUAL "glslangValidator")
		if(_STAGE STREQUAL "vertex")
			set(_args -V -S vert)
		elseif(_STAGE STREQUAL "compute")
			set(_args -V -S comp)
		else()
			set(_args -V -S frag)
		endif()
	else()
		set(_args -fshader-stage=${_STAGE})
	endif()
	set(${_ARGS_VAR} ${_args} PARENT_SCOPE)
endfunction()
