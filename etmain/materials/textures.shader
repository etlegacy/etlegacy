// textures.shader

textures/region
{
	surfaceparm nolightmap
}

textures/test_autosprite
{
	cull none
	deformVertexes autoSprite
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm trans
	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
	}
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}
	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6
	}
}

textures/test_autosprite2
{
	cull none
	deformVertexes autoSprite2
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm trans
	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
	}
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}
	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6
	}
}
