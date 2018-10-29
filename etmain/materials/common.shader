// common.shader

textures/common/ai_nopass
{
	qer_trans 0.30
	surfaceparm ai_nopass
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/ai_nopasslarge
{
	qer_trans 0.30
	surfaceparm ai_nopasslarge
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/ai_nosight
{
	qer_trans 0.3
	surfaceparm ai_nosight
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/ai_nosight_shoot_through
{
	qer_trans 0.3
	surfaceparm ai_nosight
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm playerclip
	surfaceparm trans
}

textures/common/antiportal
{
	qer_nocarve
	qer_trans 0.30
	surfaceparm antiportal
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm structural
	surfaceparm trans
}

textures/common/areaportal
{
	surfaceparm areaportal
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm structural
	surfaceparm trans
}

textures/common/caulk
{
	surfaceparm nodraw
	surfaceparm nomarks
}

textures/common/caulkterrain
{
	qer_editorimage textures/common/caulk.tga
	surfaceparm nodraw
	surfaceparm nomarks
}

textures/common/clip
{
	qer_trans 0.3
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm playerclip
	surfaceparm trans
}

textures/common/clip_metal
{
	qer_trans 0.3
	surfaceparm clipmissile
	surfaceparm metalsteps
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm playerclip
	surfaceparm trans
}

textures/common/clipfull
{
	qer_trans 0.3
	surfaceparm nodraw
	surfaceparm playerclip
	surfaceparm trans
}

textures/common/clipmissile
{
	qer_trans 0.3
	surfaceparm clipmissile
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm playerclip
	surfaceparm trans
}

textures/common/clipmonster
{
	qer_trans 0.3
	surfaceparm monsterclip
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/clipmonster2
{
	qer_editorimage textures/common/clipmonster.tga
	surfaceparm monsterclip
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/clipshot
{
	qer_trans 0.3
	surfaceparm clipshot
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/clipweap
{
	qer_trans 0.3
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm trans
}

textures/common/clipweap_metal
{
	qer_editorimage textures/common/clipweapmetal.tga
	qer_trans 0.3
	surfaceparm metalsteps
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm trans
}

// Don't ask...
textures/common/clipweapmetal
{
    qer_editorimage "textures/common/clip_metal.tga"
    qer_trans 0.3
    surfaceparm nodraw
    surfaceparm nomarks
    surfaceparm trans
    surfaceparm metalsteps
}

textures/common/clipweap_wood
{
	qer_editorimage textures/common/clipweapwood.tga
	qer_trans 0.3
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm woodsteps
}

textures/common/clusterportal
{
	qer_trans 0.3
	surfaceparm clusterportal
	surfaceparm detail
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/cushion
{
	qer_nocarve
	qer_trans 0.5
	surfaceparm nodamage
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm trans
}

textures/common/hint
{
	qer_nocarve
	qer_trans 0.3
	surfaceparm hint
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm structural
	surfaceparm trans
}

textures/common/ladder
{
	qer_trans 0.3
	surfaceparm ladder
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm playerclip
	surfaceparm trans
}

textures/common/leakoil
{
	qer_trans 0.5
//	surfaceparm markparticles
	surfaceparm nodraw
	surfaceparm trans
}

textures/common/leaksteam
{
	qer_trans 0.5
//	surfaceparm markparticles
	surfaceparm nodraw
	surfaceparm trans
}

textures/common/leakwater
{
	qer_trans 0.5
//	surfaceparm markparticles
	surfaceparm nodraw
	surfaceparm trans
}

textures/common/lightgrid
{
	qer_trans 0.5
	surfaceparm detail
	surfaceparm lightgrid
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/lmterrain_sand_marks
{
	qer_editorimage textures/common/terrain_sand.tga
	q3map_shadeangle 45
	q3map_terrain
	surfaceparm gravelsteps
	surfaceparm nodraw
}

textures/common/lmterrain_snow_marks
{
	qer_editorimage textures/common/terrain_snow.tga
	q3map_shadeangle 90
	q3map_terrain
	surfaceparm nodraw
	surfaceparm snowsteps
}

textures/common/nodraw
{
	qer_trans 0.3
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/nodrawnonsolid
{
	surfaceparm nodraw
	surfaceparm nonsolid
}

textures/common/nodrawwater
{
	qer_trans .5
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm water
}

textures/common/nodrop
{
	qer_nocarve
	qer_trans 0.5
	cull disable
	surfaceparm nodraw
	surfaceparm nodrop
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
}

textures/common/origin
{
	qer_nocarve
	qer_trans 0.3
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm origin
	surfaceparm trans
}

textures/common/skip
{
	qer_nocarve
	qer_trans 0.30
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm skip
	surfaceparm structural
	surfaceparm trans
}

textures/common/slick
{
	qer_trans 0.3
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm slick
	surfaceparm trans
}

textures/common/terrain
{
	q3map_terrain
	surfaceparm grasssteps
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm nomarks
}

textures/common/terrain_snow
{
	q3map_terrain
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm nomarks
//	surfaceparm snow
	surfaceparm snowsteps
}

textures/common/terrain_snow_wils
{
	q3map_terrain
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm snowsteps
}

textures/common/terrain2
{
	q3map_terrain
	surfaceparm grasssteps
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm nomarks
}

textures/common/trigger
{
	qer_nocarve
	qer_trans 0.5
	surfaceparm nodraw
	surfaceparm trans
}

// exactly like caulk, just a different image so you can tell the difference
textures/common/hullcaulk
{
	surfaceparm nodraw
	surfaceparm nomarks
}

// hint with normal priority.
textures/common/subtlehint
{
	qer_nocarve
	qer_trans 0.3
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm structural
	surfaceparm trans
}

// exactly like skip, but it filters with hint
textures/common/hintskip
{
	qer_editorimage textures/common/skip
	qer_nocarve
	qer_trans 0.30
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm skip
	surfaceparm structural
	surfaceparm trans
}
