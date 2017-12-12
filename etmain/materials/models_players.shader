// models_players.shader

models/players/temperate/allied/strap
{
	cull disable
	diffusemap models/players/temperate/allied/inside
	bumpmap models/players/temperate/allied/inside_n
	specularmap models/players/temperate/allied/inside_s

}

models/players/multi/acc/backpack/backpack_lieu
{
	cull disable // Why does this shader have cull disable??
	{
		diffusemap models/players/multi/acc/backpack/backpack_lieu
		bumpmap models/players/multi/acc/backpack/backpack_lieu_n
		specularmap models/players/multi/acc/backpack/backpack_lieu_s
		alphafunc ge128
		
	}
}

models/players/temperate/axis/ranks/major
{
	polygonoffset
	
	{
		map gfx/hud/ranks/major.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/axis/ranks/feldwebel
{
	polygonoffset
	
	{
		map gfx/hud/ranks/sergeant.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/axis/ranks/leutnant
{
	polygonoffset
	
	{
		map gfx/hud/ranks/lieutenant.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}


models/players/temperate/axis/ranks/gefreiter
{
	polygonoffset
	
	{
		map gfx/hud/ranks/corporal.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}



models/players/temperate/axis/ranks/oberst
{
	polygonoffset
	
	{
		map gfx/hud/ranks/colonel.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/axis/ranks/hauptmann
{
	polygonoffset	
	
	{
		map gfx/hud/ranks/captain.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}



models/players/temperate/axis/ranks/oberschutze
{
	polygonoffset	
	
	{
		map gfx/hud/ranks/1stclass.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}



models/players/temperate/allied/ranks/major
{
	polygonoffset
	
	{
		map gfx/hud/ranks/major.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/allied/ranks/sergeant
{
	qer_models/players/temperate/allied/ranks/sergeant.tga
	polygonoffset
	
	{
		map gfx/hud/ranks.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/players/temperate/allied/ranks/lieutenant
{
	polygonoffset
	
	{
		map gfx/hud/ranks/lieutenant.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}


models/players/temperate/allied/ranks/corporal
{
	polygonoffset
	
	{
		map gfx/hud/ranks/corporal.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}



models/players/temperate/allied/ranks/colonel
{
	polygonoffset
	
	{
		map gfx/hud/ranks/colonel.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}


models/players/temperate/allied/ranks/captain
{
	polygonoffset	
	
	{
		map gfx/hud/ranks/captain.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}



models/players/temperate/allied/ranks/1stclass
{
	polygonoffset	
	
	{
		map gfx/hud/ranks/1stclass.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}


//start axis players

models/players/temperate/axis/engineer/engineer_body
{
	cull none
	diffusemap models/players/temperate/axis/engineer/engineer_body
	bumpmap models/players/temperate/axis/engineer/engineer_body_n
	specularmap models/players/temperate/axis/engineer/engineer_body_s
}

models/players/temperate/axis/cvops/body_cvops
{
	cull none
	diffusemap models/players/temperate/axis/cvops/body_cvops
	bumpmap models/players/temperate/axis/cvops/body_cvops_n
	specularmap models/players/temperate/axis/cvops/body_cvops_s
}

models/players/temperate/axis/fieldops/body_fieldops
{
	cull none
	diffusemap models/players/temperate/axis/fieldops/body_fieldops
	bumpmap models/players/temperate/axis/fieldops/body_fieldops_n
	specularmap models/players/temperate/axis/fieldops/body_fieldops_s
}

models/players/temperate/axis/medic/axis_medic
{
	cull none
	diffusemap models/players/temperate/axis/medic/axis_medic
	bumpmap models/players/temperate/axis/medic/axis_medic_n
	specularmap models/players/temperate/axis/medic/axis_medic_s
}

models/players/temperate/axis/soldier/body_soldier
{
	cull none
	diffusemap models/players/temperate/axis/soldier/body_soldier
	bumpmap models/players/temperate/axis/soldier/body_soldier_n
	specularmap models/players/temperate/axis/soldier/body_soldier_s
}



//start allied
models/players/temperate/allied/soldier/body
{
	cull none
	diffusemap models/players/temperate/allied/soldier/body
	bumpmap models/players/temperate/allied/soldier/body_n
	specularmap models/players/temperate/allied/soldier/body_s
}

models/players/temperate/allied/medic/body
{
	cull none
	diffusemap models/players/temperate/allied/medic/body
	bumpmap models/players/temperate/allied/medic/body_n
	specularmap models/players/temperate/allied/medic/body_s
}

models/players/temperate/allied/engineer/body
{
	cull none
	diffusemap models/players/temperate/allied/engineer/body
	bumpmap models/players/temperate/allied/engineer/body_n
	specularmap models/players/temperate/allied/engineer/body_s
}
models/players/hud/allied_field
{
	cull none
	diffusemap models/players/hud/allied_field
	bumpmap models/players/hud/allied_field_n
	specularmap models/players/allied_field_s
}
models/players/temperate/allied/fieldops/body
{
	cull none
	diffusemap models/players/temperate/allied/fieldops/body
	bumpmap models/players/temperate/allied/fieldops/body_n
	specularmap models/players/temperate/allied/fieldops/body_s
}

//allied cvops head
models/players/hud/allied_cvops
{
    cull_none
	 diffusemap models/players/hud/allied_cvops
	 bumpmap models/players/hud/allied_cvops_n
	 specularmap models/players/hud/allied_cvops_s
}
//allied cvops body
models/players/temperate/allied/cvops/body
{
	cull none
	diffusemap models/players/temperate/allied/cvops/body
	bumpmap models/players/temperate/allied/cvops/body_n
	specularmap models/players/temperate/allied/cvops/body_s
}

//hud eyes

models/players/hud/eye01
{
   diffusemap models/players/hud/eye01
   bumpmap models/players/hud/eye01_n
   specularmap models/players/hud/eye01_s
}

models/players/hud/eye02
{
   diffusemap models/players/hud/eye02
   bumpmap models/players/hud/eye02_n
   specularmap models/players/hud/eye02_s
}

models/players/hud/eye03
{
   diffusemap models/players/hud/eye03
   bumpmap models/players/hud/eye03_n
   specularmap models/players/hud/eye03_s
}

//hud teeth

models/players/hud/teeth01
{
   diffusemap models/players/hud/teeth01
   bumpmap models/players/hud/teeth01_n
   specularmap models/players/hud/teeth01_s
}

