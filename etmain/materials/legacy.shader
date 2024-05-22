// legacy.shader

images/blackmask
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/2d/mapmask.tga
		depthwrite
		blendfunc blend
		rgbGen identity
		alphaGen vertex
	}
}

models/weapons2/shells/m_shell
{
	{
		map models/weapons2/shells/M_shell.jpg
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/shells/M_shell.jpg
		blendFunc GL_ZERO GL_ONE
		rgbGen lightingDiffuse
	}
}

icons/iconw_mortar
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_mortar.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_mortar_ax
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_mortar_ax.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_bazooka
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_bazooka.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_bazooka_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_bazooka_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_browning
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_browning.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_browning_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_browning_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_landmine_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_landmine_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_m1_garand_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_m1_garand_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_m1_garand_gren_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_m1_garand_gren_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_kar98_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_kar98_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_kar98_gren_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_kar98_gren_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_fg42_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_fg42_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_mg42_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_mg42_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_mortar_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_mortar_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_mortar_ax_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_mortar_ax_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_syringe_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_syringe_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_syringe2_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_syringe2_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_binoculars_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_binoculars_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_satchel_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_satchel_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_radio_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_radio_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

icons/iconw_silencer_1_select
{
	nomipmaps
	nopicmip
	{
		map icons/iconw_silencer_1_select.tga
		blendFunc blend
		rgbGen vertex
	}
}

gfx/hud/pm_constallied
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/hud/pm_constallied.tga
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/hud/pm_constaxis
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/hud/pm_constaxis.tga
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/pm_dynamite
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_dynamite.tga
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/pm_flagallied
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_flagallied.tga
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/pm_flagaxis
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_flagaxis.tga
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_alliedgren
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_alliedgren.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_axisgren
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_axisgren.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_bankdoor
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_bankdoor.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_bo_allied
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_bo_allied.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_bo_axis
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_bo_axis.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_churchill
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_churchill.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_constallied
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_constallied.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_constaxis
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_constaxis.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_dynamite
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_dynamite.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_flagallied
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_flagallied.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_flagaxis
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_flagaxis.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_fuel
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_fuel.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_goldbars
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_goldbars.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_guncontrols
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_guncontrols.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_healthammo
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_healthammo.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_jagdpanther
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_jagdpanther.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_oasiswall
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_oasiswall.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_oasis_pakgun
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_oasis_pakgun.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_radarbox
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_radarbox.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_radar_maindoor
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_radar_maindoor.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_radar_sidedoor
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_radar_sidedoor.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_satchel
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_satchel.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_truck
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_truck.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_tug
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_tug.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_frostbitedocs
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_frostbitedocs.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_frostbitemaindoor
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_frostbitemaindoor.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_frostbitenull
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_frostbitenull.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_frostbiteservicedoor
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_frostbiteservicedoor.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_frostbitestoragewall
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_frostbitestoragewall.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_frostbitetransmitter
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_frostbitetransmitter.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_adlnernest_drctl
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_adlnernest_drctl.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_adlnernest_trans
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_adlnernest_trans.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/braundorf/cm_controls
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/braundorf/cm_controls.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/braundorf/cm_fence
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/braundorf/cm_fence.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/braundorf/cm_maindoor
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/braundorf/cm_maindoor.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/braundorf/cm_sidedoor
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/braundorf/cm_sidedoor.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_bremgen
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/cm_bremgen.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/beer/cm_keycard
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/beer/cm_keycard.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/wb/cm_bellhammer
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/wb/cm_bellhammer.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/wb/cm_ghgate
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/wb/cm_ghgate.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/wb/cm_health
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/wb/cm_health.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/wb/cm_mcgate
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/wb/cm_mcgate.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/wb/cm_mggate
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/wb/cm_mggate.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/wb/cm_mmgate
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/wb/cm_mmgate.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/wb/cm_srwall
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/wb/cm_srwall.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/wb/cm_warbell
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/wb/cm_warbell.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/mort_hit
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/mort_hit.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/mort_target
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/mort_target.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/landmine_allied
{
	nopicmip
	nocompress
	nomipmaps
	{
		map sprites/landmine_allied.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/landmine_axis
{
	nopicmip
	nocompress
	nomipmaps
	{
		map sprites/landmine_axis.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_ammo_blue
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_ammo_blue.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_ammo_red
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_ammo_red.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_arrow_blue
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_arrow_blue.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_arrow_red
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_arrow_red.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_gun_blue
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_gun_blue.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_gun_red
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_gun_red.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_health_blue
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_health_blue.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_health_red
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_health_red.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_player_highlight
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_player_highlight.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_spy_blue
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_spy_blue.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_spy_red
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_spy_red.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_wrench_blue
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_wrench_blue.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

ui/assets/mp_wrench_red
{
	nopicmip
	nocompress
	nomipmaps
	{
		map ui/assets/mp_wrench_red.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

// KA-BAR
models/weapons2/knife_kbar/knife_yd
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/weapons2/knife_kbar/knife_yd_alpha.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map models/weapons2/knife_kbar/knife_yd.jpg
		blendFunc GL_ONE GL_ONE
		rgbGen lightingdiffuse
	}
}

// Mobile Browning .30 cal MG
models/weapons2/browning/browning
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/multiplayer/browning/browning.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/weapons2/browning/biped
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/weapons2/browning/biped.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/weapons2/browning/barrel
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
		map models/multiplayer/browning/barrel.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
		depthFunc equal
	}
}

// Axis Granatwefer 34
models/multiplayer/mortar/mortar_ax
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/multiplayer/mortar/mortar_ax.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/multiplayer/mortar/mortar_shell_ax
{
	{
		map models/multiplayer/mortar/mortar_shell_ax.jpg
		rgbGen lightingdiffuse
	}
}

models/multiplayer/mortar/mortar_sd
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/multiplayer/mortar/mortar_sd.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

// Helmet Ranks
models/players/temperate/common/rank1
{
	polygonoffset

	{
		map models/players/temperate/common/rank1.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank2
{
	polygonoffset
	{
		map models/players/temperate/common/rank2.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank3
{
	polygonoffset
	{
		map models/players/temperate/common/rank3.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank4
{
	polygonoffset
	{
		map models/players/temperate/common/rank4.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank5
{
	polygonoffset
	{
		map models/players/temperate/common/rank5.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank6
{
	polygonoffset
	{
		map models/players/temperate/common/rank6.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank7
{
	polygonoffset
	{
		map models/players/temperate/common/rank7.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank8
{
	polygonoffset
	{
		map models/players/temperate/common/rank8.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank9
{
	polygonoffset
	{
		map models/players/temperate/common/rank9.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank10
{
	polygonoffset
	{
		map models/players/temperate/common/rank10.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/rank11
{
	polygonoffset
	{
		map models/players/temperate/common/rank11.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank1
{
	polygonoffset
	{
		map models/players/temperate/common/xrank1.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank2
{
	polygonoffset
	{
		map models/players/temperate/common/xrank2.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank3
{
	polygonoffset
	{
		map models/players/temperate/common/xrank3.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank4
{
	polygonoffset
	{
		map models/players/temperate/common/xrank4.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank5
{
	polygonoffset
	{
		map models/players/temperate/common/xrank5.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank6
{
	polygonoffset
	{
		map models/players/temperate/common/xrank6.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank7
{
	polygonoffset
	{
		map models/players/temperate/common/xrank7.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank8
{
	polygonoffset
	{
		map models/players/temperate/common/xrank8.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank9
{
	polygonoffset
	{
		map models/players/temperate/common/xrank9.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank10
{
	polygonoffset
	{
		map models/players/temperate/common/xrank10.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/common/xrank11
{
	polygonoffset
	{
		map models/players/temperate/common/xrank11.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

//HUD Ranks//
gfx/hud/ranks/rank1
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank1.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank2
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank2.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank3
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank3.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank4
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank4.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank5
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank5.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank6
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank6.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank7
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank7.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank8
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank8.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank9
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank9.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank10
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank10.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/rank11
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank11.tga
		blendfunc blend
		rgbGen vertex
	}
}


gfx/hud/ranks/xrank1
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank1.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank2
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank2.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank3
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank3.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank4
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank4.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank5
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank5.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank6
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank6.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank7
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank7.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank8
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank8.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank9
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank9.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank10
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank10.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/xrank11
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/xrank11.tga
		blendfunc blend
		rgbGen vertex
	}
}

// disguised shader
gfx/2d/friendlycross
{
	nocompress
	nopicmip
	{
		map gfx/2d/friendlycross.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// enemy disguised shader
sprites/undercover
{
	nocompress
	nopicmip
	{
		map sprites/undercover.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// fireteam
sprites/fireteam
{
	nocompress
	nopicmip
	{
		map sprites/fireteam.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

// sharp crosshairs
gfx/2d/crosshairk
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshairk.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// nothing
gfx/2d/crosshairk_alt
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshaira_alt.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// slightly modified version of crosshair b so it centers
gfx/2d/crosshairl
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshairl.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// sharp center version of b_alt
gfx/2d/crosshairl_alt
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshairl_alt.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// copy of c
gfx/2d/crosshairm
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshairc.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// sharp version of b_alt
gfx/2d/crosshairm_alt
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshairp.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// copy of d
gfx/2d/crosshairn
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshaird.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// sharp version of d_alt
gfx/2d/crosshairn_alt
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshairp.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// copy of e
gfx/2d/crosshairo
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshaire.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

// sharp version of e_alt
gfx/2d/crosshairo_alt
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshairp.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

//sharp dot
gfx/2d/crosshairp
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshairp.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
// blurs even less, but artifacts at many sizes
//		alphaFunc GT0
		rgbGen vertex
	}
}

// nothing
gfx/2d/crosshairp_alt
{
	nocompress
	nopicmip
	{
		map gfx/2d/crosshairf_alt.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
