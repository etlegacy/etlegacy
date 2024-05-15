/*
my test materials are in X:\ETLegacy\etlegacy\build\legacy\materials
my test textures are in "My Documents"\ETLegacy\etmain\texturesC.pk3
my test models are in "My Documents"\ETLegacy\etmain\modpack_beta6.pk3
my test models are in "My Documents"\ETLegacy\legacy\modelsC.pk3
*/


/* reliefMapping_fp.glsl - All-In-One parallax mapping helper function */

// We do Parallax Occlusion Mapping (POM).
//
// The needed texturemap is a normalmap/bumpmap in the RGB of an image,
// the heights are encoded in the alpha channel of that image.
// You can make such a map yourself (for the best results), or you can make one at runtime.
// To do that, you need a bumpmap and a heightmap image (say, image_n & image_h).
// The heightmap is an RGB image with just black/white shades of grey. Black = low, white = high.
// In the material, when supplying the bumpmap, make it a displacemap with the following instruction:
//     bumpmap displaceMap(textures/folder/image_n.tga, textures/folder/image_h.tga)
// And do not to forget to also add a new line in the material header, to enable parallax mapping for the material:
//     parallax
//


// The values to adjust the fading zone.
// The zone that is used to fade out the parallax effect over distance.
// Parallax mapped surfaces change depth over distance from the viewer.
// That way we can slowly render the effect invisible, and we do not need to render
// any parallaxed surfaces that are too far away to notice the effect.
// We don't have to parallax the whole world, but just the surfaces nearby.
//
#define PARALLAX_FADE_START 200.0
#define PARALLAX_FADE_END   500.0


// https://learnopengl.com/Advanced-Lighting/Parallax-Mapping

//
// All in one: POM + self shadows
//
// The function result is a vec3:
///  result.xy = parallax final texture coordinates
//   result.z  = shadow factor  (1.0 means full light, no shadow.  0.0 means no light, maximum shadow)
//
// Function arguments:
//   "displaceMap" is an RGBA image.  RGB stores the normalmap, and the alpha channel holds the heightmap data.
//   "texCoords" are the current texture coordinates for which to parallax map.
//   "viewDir" & "lightDir" must be a normalized vectors, in tangent space. LightDir is from light to surface.
//   "depthscale" is the value of the cvar r_parallaxDepthScale.
//   "distanceToCam" is the distance from object to camera, in worldspace.
//   "shadowFactor" is used to set the amount of parallax-selfshadowing. A value 0 disables this feature. (cvar r_parallaxShadow)
//   "lightmapColor" is the color computed for the lightmap term.
//
vec3 parallaxAndShadow(sampler2D displaceMap, vec2 texCoords, vec3 viewDir, vec3 lightDir, float depthscale, float distanceToCam, float shadowFactor, vec3 lightmapColor) {
	// the function result
	vec3 result = vec3(texCoords, 1.0); // set shadow value to 1.0 (no shadow)
	// fade out the effect over distance
	if (distanceToCam > PARALLAX_FADE_END) return result;     // early out if (point on) surface is too far away..
	float distanceRatio = (distanceToCam > PARALLAX_FADE_START) ? 1.0 - ((distanceToCam - PARALLAX_FADE_START) / (PARALLAX_FADE_END - PARALLAX_FADE_START)) : 1.0;

	// in tangentspace, the normal of the surface is always the same
	const vec3 surfaceNormalT = vec3(0.0, 0.0, 1.0);

	// steep parallax mapping
	const int minLayers = 4;
	const int maxLayers = 64;

	// calculate the derivatives for use in the textureGrad or textureLod function
	vec2 dx = dFdx(texCoords);
	vec2 dy = dFdy(texCoords);
	// calculate the miplevel for use in the textureLod function.
	vec2 vTexCoordsPerSize = texCoords * textureSize(displaceMap, 0);
	vec2 dxSize = dFdx(vTexCoordsPerSize);
	vec2 dySize = dFdy(vTexCoordsPerSize);
	vec2 dTexCoords = dxSize * dxSize + dySize * dySize;
	float mipLevel = max(0.5 * log2(max(dTexCoords.x, dTexCoords.y)), 0);

	// less layers when viewed from straight above (in tangent space)
	float viewAngle = abs(dot(surfaceNormalT, viewDir));
	float numLayers = int((maxLayers - minLayers) * viewAngle + minLayers);
	float layerDepth = 1.0 / numLayers;
	vec2 deltaTexCoords = viewDir.xy / viewDir.z * layerDepth * depthscale * distanceRatio;
	float curDepth = 0.0;
	vec2 curTexCoords = texCoords;
	float curDepthValue = textureLod(displaceMap, curTexCoords, mipLevel).a;
//@	float curDepthValue = textureGrad(displaceMap, curTexCoords, dx, dy).a;
//@	float curDepthValue = texture2D(displaceMap, curTexCoords).a;
	vec2 prevTexCoords;
	float prevDepthValue;
	float afterDepth;
	float beforeDepth;
	float parallaxHeight;
	float weight;
	float weight_1;
	// a while-loop in glsl is not so easy to run. My old while loop went woot..
	// a for-loop in glsl works well.
	for (int L=0; L<numLayers; L++) {
		if (curDepth >= curDepthValue) break;
		prevTexCoords = curTexCoords;
		prevDepthValue = curDepthValue;
		curTexCoords -= deltaTexCoords;
		curDepthValue = textureLod(displaceMap, curTexCoords, mipLevel).a;
//@		curDepthValue = textureGrad(displaceMap, curTexCoords, dx, dy).a;
//@		curDepthValue =  texture2D(displaceMap, curTexCoords).a;
		curDepth += layerDepth;
	}

	// parallax occlusion mapping
	afterDepth  = curDepthValue - curDepth;
	beforeDepth = prevDepthValue - curDepth + layerDepth;
	weight = afterDepth / (afterDepth - beforeDepth);
	weight_1 = 1.0 - weight;
	result.xy = prevTexCoords * weight + curTexCoords * weight_1; // finalTexCoords

	// self shadowing
	shadowFactor = clamp(shadowFactor, 0.0, 1.0);
	// early out if self-shadowing is disabled
	if (shadowFactor == 0.0) {
		return result;
	} else {
		parallaxHeight = curDepth + beforeDepth * weight + afterDepth * weight_1;

		// there are no shadows when the surface is lit from the backside.
		float lightAngle = dot(surfaceNormalT, lightDir);
		if (lightAngle < 0) return result;

		const int minLayers = 4;
		const int maxLayers = 8;
		float shadow = 0.0;
		float numLayers = (maxLayers - minLayers) * (1.0 - lightAngle) + minLayers;
		float divNumLayers = 1.0 / numLayers;
		float layerHeight = parallaxHeight * divNumLayers;
		float curHeight = parallaxHeight - layerHeight;
		vec2 deltaTexCoords = -lightDir.xy / -lightDir.z * divNumLayers * depthscale;
		vec2 curTexCoords = result.xy + deltaTexCoords; // result.xy is the parallax vec2 result
		float curHeightValue = textureLod(displaceMap, curTexCoords, mipLevel).a;
//@		float curHeightValue = textureGrad(displaceMap, curTexCoords, dx, dy).a;
//@		float curHeightValue = texture2D(displaceMap, curTexCoords).a;
		float stepHeight = 1.0 - divNumLayers;
		for (int L=0; L<numLayers; L++) {
			if (curHeight <= 0) break;
			if (curHeightValue < curHeight) shadow = max(shadow, (curHeight - curHeightValue) * stepHeight);
			stepHeight -= divNumLayers;
			curHeight -= layerHeight;
			curTexCoords += deltaTexCoords;
			curHeightValue = textureLod(displaceMap, curTexCoords, mipLevel).a;
//@			curHeightValue = textureGrad(displaceMap, curTexCoords, dx, dy).a;
//@			curHeightValue = texture2D(displaceMap, curTexCoords).a;
		}

		// if there is no shadow, we are in full light
		if (shadow == 0.0) return result;

		// When the lightmap is dark (in shadow) there should be no/less POM self-shadowing..
		// Because a lightmap can contain color, it's not just a grayscale image, i average over the color channels.
		float avgLightmap = ((lightmapColor.r + lightmapColor.g + lightmapColor.b) * 0.333333333);
		shadow *= avgLightmap;

		// fade out the amount of shadow over distance, and by cvar factor
		result.z = 1.0 - shadow * distanceRatio * shadowFactor;
		return result;

	}
}
