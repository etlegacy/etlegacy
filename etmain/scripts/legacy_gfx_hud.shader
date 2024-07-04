//========================================//
// legacy_gfx_hud.shader
//========================================//


//========================================//
// HUD Ranks
//========================================//
gfx/hud/ranks/arank2
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank2.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/arank3
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank3.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/arank4
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank4.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/arank5
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank5.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/arank6
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank6.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/arank7
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank7.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/arank8
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank8.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/arank9
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank9.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/arank10
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank10.tga
		blendfunc blend
		rgbGen vertex
	}
}

gfx/hud/ranks/arank11
{
	nomipmaps
	nopicmip
	{
		map gfx/hud/ranks/rank11.tga
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
//========================================//


//========================================//
// Minimap shaders
//========================================//
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
//========================================//

sprintbarhorizontal
{
	nocompress
	nomipmap
	nopicmip
	{
		clampmap ui/assets/hudsprinthorizontal.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
