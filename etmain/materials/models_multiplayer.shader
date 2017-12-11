// models_multiplayer.shader

models/multiplayer/satchel/radio
{	
	diffusemap models/multiplayer/satchel/radio
	bumpmap models/multiplayer/satchel/radio_n
	specularmap models/multiplayer/satchel/radio_s
}

models/multiplayer/satchel/satchel_allied
{
	diffusemap models/multiplayer/satchel/satchel_allied
	bumpmap models/multiplayer/satchel/satchel_allied_n
    specularmap models/multiplayer/satchel/satchel_allied_s	
}

models/multiplayer/satchel/satchel_axis
{
	diffusemap models/multiplayer/satchel/satchel_axis
	bumpmap models/multiplayer/satchel/satchel_axis_n
	specularmap models/multiplayer/satchel/satchel_axis_s
}

//fixme later, need to figure out how the light really works
models/multiplayer/satchel/light_ref
{
	{
		map textures/effects/tinfx.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/multiplayer/satchel/lightoff.tga
		blendFunc blend
		rgbGen lightingdiffuse
	}
}

models/multiplayer/satchel/lightgreen_off
{
	{
		map models/multiplayer/satchel/lightoff.tga
		rgbgen lightingdiffuse
	}
}

models/multiplayer/satchel/lightgreen_off
{
	{
		map models/multiplayer/satchel/lightoff.tga
		rgbgen lightingdiffuse
	}
}

models/multiplayer/satchel/lightgreen_on
{
	{
		map models/multiplayer/satchel/lightoff.tga
		rgbgen lightingdiffuse
	}
	{
		map models/multiplayer/satchel/lightgreen.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave noise 0.2 0.8 0.5 10
	}
}
models/multiplayer/satchel/lightred_off
{
	{
		map models/multiplayer/satchel/lightoff.tga
		rgbgen lightingdiffuse
	}
}

models/multiplayer/satchel/lightred_on
{
	{
		map models/multiplayer/satchel/lightoff.tga
		rgbgen lightingdiffuse
	}
	{
		map models/multiplayer/satchel/lightred.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave noise 0.2 0.8 0.3 9
	}
}

models/multiplayer/supplies/healthbox
{
    diffusemap models/multiplayer/supplies/healthbox
	bumpmap models/multiplayer/supplies/healthbox_n
	specularmap models/multiplayer/supplies/healthbox_s
}

models/multiplayer/supplies/ammobox
{
	diffusemap models/multiplayer/supplies/ammobox
	bumpmap models/multiplayer/supplies/ammobox_n
	specularmap models/multiplayer/supplies/ammobox_s
}

models/multiplayer/supplies/ammobox_2
{
	diffusemap models/multiplayer/supplies/ammobox_2
	bumpmap models/multiplayer/supplies/ammobox_2_n
	specularmap models/multiplayer/supplies/ammobox_2_s
}

models/multiplayer/smokebomb/smoke_bomb
{
	diffusemap models/multiplayer/smokebomb/smoke_bomb
	bumpmap models/multiplayer/smokebomb/smoke_bomb_n
	specularmap models/multiplayer/smokebomb/smoke_bomb_s
}

models/multiplayer/smokegrenade/smoke_grenade
{
	diffusemap models/multiplayer/smokegrenade/smoke_grenade
	bumpmap models/multiplayer/smokegrenade/smoke_grenade_n
	specularmap models/multiplayer/smokegrenade/smoke_grenade_s
}

models/multiplayer/landmine/landmine
{
	diffusemap models/multiplayer/landmine/landmine
	bumpmap models/multiplayer/landmine/landmine_n
	specularmap models/multiplayer/landmine/landmine_s
}

models/multiplayer/pliers/pliers
{
    diffusemap models/multiplayer/pliers/pliers
	bumpmap models/multiplayer/pliers/pliers_n
	specularmap models/multiplayer/pliers/pliers_s
}

models/multiplayer/binocs/binoculars
{
	map models/multiplayer/binocs/binoculars
	bumpmap models/multiplayer/binocs/binoculars_n
	specularmap models/multiplayer/binocs/binoculars_s
}

models/multiplayer/mortar/mortar_sd
{
	diffusemap models/multiplayer/mortar/mortar_sd
	bumpmap models/multiplayer/mortar/mortar_sd_n
	specularmap models/multiplayer/mortar/mortar_sd_s
}

models/multiplayer/mg42/s_mg42
{
	diffusemap models/multiplayer/mg42/s_mg42
	bumpmap models/multiplayer/mg42/s_mg42_n
	specularmap models/multiplayer/mg42/s_mg42_s
}

models/multiplayer/browning/handle
{
    diffusemap models/multiplayer/browning/handle
	bumpmap models/multiplayer/browning/handle_n
	specularmap models/multiplayer/browning/handle_s
}

models/multiplayer/browning/browning
{
	diffusemap models/multiplayer/browning/browning
	bumpmap models/multiplayer/browning/browning_n
	specularmap models/multiplayer/browning/browning_s
}

models/multiplayer/browning/barrel
{
	cull none
	{
		map models/multiplayer/browning/barrel.tga
		rgbGen const ( 0 0 0 ) 
		alphaFunc GE128
		depthWrite
	}
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
		depthFunc equal
	}
	{
		diffusemap models/multiplayer/browning/barrel
		bumpmap models/multiplayer/browning/barrel_n
		specularmap models/multiplayer/browning/barrel_s
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
		depthFunc equal
	}
}

models/multiplayer/gold/gold
{
	{
		diffusemap models/multiplayer/gold/gold
		bumpmap models/multiplayer/gold/gold_n
		specularmap models/multiplayer/gold/gold_s
	}
	{
		map textures/effects/envmap_gold.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 2.25 1.25
		tcGen environment
		rgbGen lightingdiffuse
	}
}
//not in our source
//models/multiplayer/treasure/treasure
//{
//	cull disable
//	deformvertexes autosprite
//	sort nearest
//	surfaceparm trans
//	{
//		map models/multiplayer/treasure/treasure.tga
//		blendfunc gl_src_alpha gl_one_minus_src_alpha
//		tcmod stretch sin .8 .08 0 .8
//	}
//}

models/multiplayer/flagpole/american
{
	cull disable
	{
		diffusemap models/multiplayer/flagpole/american
		bumpmap models/multiplayer/flagpole/american_n
		specularmap models/multiplayer/flagpole/american_s
	}
}

models/multiplayer/flagpole/american_reinforce
{
	cull disable

	{
		diffusemap models/multiplayer/flagpole/american_reinforce
		bumpmap models/multiplayer/flagpole/american_reinforce_n
		specularmap models/multiplayer/flagpole/american_reinforce_s
	}
}

models/multiplayer/flagpole/disc
{
	diffusemap models/multiplayer/flagpole/disc
	bumpmap models/multiplayer/flagpole/disc_n
	specularmap models/multiplayer/flagpole/disc_s
	{
		clampmap models/multiplayer/flagpole/disc
		rgbGen lightingDiffuse
		tcMod rotate 30
		blenfunc blend
	}
}

models/multiplayer/flagpole/flagpole
{
	diffusemap models/multiplayer/flagpole/flag_clouds
	bumpmap models/multiplayer/flagpole/flag_clouds_n
	specularmap models/multiplayer/flagpole/flag_clouds_s	
}

models/multiplayer/flagpole/german
{
	cull disable
	{
		diffusemap models/multiplayer/flagpole/german
		bumpmap models/multiplayer/flagpole/german_n
		specularmap models/multiplayer/flagpole/german_s
	}
}

models/multiplayer/flagpole/german_reinforce
{
	cull disable
	{
		diffusemap models/multiplayer/flagpole/german_reinforce
		bumpmap models/multiplayer/flagpole/german_reinforce_n
		specularmap models/multiplayer/flagpole/german_reinforce_s
	}
}

// waypoint marker
//models/multiplayer/flagpole/waypoint
//{
//	cull disable
//	deformVertexes wave 194 sin 0 3 0 .4
//	{
//		map models/multiplayer/flagpole/waypoint.tga
//		rgbGen lightingDiffuse
//	}
//}

models/multiplayer/kar98/kar98silencer
{
	diffusemap models/multiplayer/m1_garand/m1garandsilencer_yd
	bumpmap models/multiplayer/m1_garand/m1garandsilencer_yd_n
	specularmap models/multiplayer/m1_garand/m1garandsilencer_yd_s
}

models/multiplayer/kar98/mauser3 
{ 
	diffusemap models/weapons2/mauser/mauser3_yd
	bumpmap models/weapons2/mauser/mauser3_yd_n
	specularmap models/weapons2/mauser/mauser3_yd_s      
}

models/multiplayer/mg42/bullet
{
	diffusemap models/multiplayer/mg42/bullet
	bumpmap models/multiplayer/mg42/bullet_n
	specularmap models/multiplayer/mg42/bullet_s
}

models/multiplayer/mine_marker/allied_marker
{
	cull disable
	nomipmaps
	nopicmip
	{
		diffusemap models/multiplayer/mine_marker/allied_marker
		bumpmap models/multiplayer/mine_marker/allied_marker_n
		specularmap models/multiplayer/mine_marker/allied_marker_s
		
	}
}

models/multiplayer/mine_marker/axis_marker
{
	cull disable
	nomipmaps
	nopicmip
	{
		diffusemap models/multiplayer/mine_marker/axis_marker
		bumpmap models/multiplayer/mine_marker/axis_marker_n
		specularmap models/multiplayer/mine_marker/axis_marker_s
	}
}

models/multiplayer/m1_garand/m1_garand
{
	diffusemap models/multiplayer/m1_garand/m1_garand_yd
	bumpmap models/multiplayer/m1_garand/m1_garand_yd_n
	specularmap models/multiplayer/m1_garand/m1_garand_yd_s
}

models/multiplayer/m1_garand/m1garandscope
{
	diffusemap models/multiplayer/m1_garand/m1garandscope_yd
	bumpmap models/multiplayer/m1_garand/m1garandscope_yd_n
	specularmap models/multiplayer/m1_garand/m1garandscope_yd_s
}

models/multiplayer/m1_garand/m1garandsilencer
{
	diffusemap models/multiplayer/m1_garand/m1garandsilencer_yd
	bumpmap models/multiplayer/m1_garand/m1garandsilencer_yd_n
	specularmap models/multiplayer/m1_garand/m1garandsilencer_yd_s
}

models/multiplayer/syringe/adrenaline
{
	cull disable
	{
		map models/multiplayer/syringe/fluid2.tga
		blendfunc blend
		rgbgen lightingdiffuse
		tcmod scale 4 6
		tcmod scroll 0 -.8
	}
	{
		map models/multiplayer/syringe/syringe.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen lightingdiffuse
	}
	{
		map models/multiplayer/syringe/syringe_reflections.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen lightingdiffuse
	}
}

models/multiplayer/syringe/plunger
{
	{
		map models/multiplayer/syringe/plunger.tga
		rgbgen lightingdiffuse
	}
}

models/multiplayer/syringe/syringe
{
	cull disable
	{
		map models/multiplayer/syringe/fluid.tga
		blendfunc blend
		rgbgen lightingdiffuse
		tcmod scale 4 6
		tcmod scroll 0 -.8
	}
	{
		map models/multiplayer/syringe/syringe.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen lightingdiffuse
	}
	{
		map models/multiplayer/syringe/syringe_reflections.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen lightingdiffuse
	}
}

models/multiplayer/syringe/100percent
{
	cull disable
	{
		map models/multiplayer/syringe/fluid3.tga
		blendfunc blend
		rgbgen lightingdiffuse
		tcmod scale 4 6
		tcmod scroll 0 -.8
	}
	{
		map models/multiplayer/syringe/syringe.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen lightingdiffuse
	}
	{
		map models/multiplayer/syringe/syringe_reflections.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen lightingdiffuse
	}
}
