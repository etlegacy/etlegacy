// legacy_r2.shader - shader definitions for ET: Legacy renderer2

lights/defaultDynamicLight
{
	{
		stage attenuationMapZ
		map lights/squarelight1a.tga
		edgeClamp
	}
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/round.tga
		colored
		zeroClamp
	}
}

lights/defaultPointLight
{
	// this will also be the falloff for any
	// point light shaders that don't specify one
	lightFalloffImage	lights/squarelight1a.tga
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/squarelight1.tga
		colored
		zeroClamp
	}
}

lights/defaultProjectedLight
{
	// by default, stay bright almost all the way to the end
	//lightFalloffImage	_noFalloff

	//lightFalloffImage	lights/skyline1
	lightFalloffImage	lights/squarelight1b.tga
	//lightFalloffImage	makeintensity(lights/flashoff)

	{
		stage attenuationMapXY
		forceHighQuality
		map lights/squarelight1.tga
		colored
		zeroClamp
	}
}
