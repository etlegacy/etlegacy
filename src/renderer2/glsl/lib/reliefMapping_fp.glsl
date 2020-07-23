/* reliefMapping_vp.glsl - Relief mapping helper functions */
//
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
// arguments:
//   - displaceMap is a normalmap, with in the alpha channel the heightmap data
//   - textCoords the incoming texture coordinates to parallax
//   - viewDir is the viewdir in tangent space
//   - depthscale is the uniform u_DepthScale
//   - distanceToCam in world units
// output:
//   argument: parallaxHeight
//   function result: the new texture coordinates
//
vec2 parallax(sampler2D displaceMap, vec2 texCoords, vec3 viewDir, float depthscale, float distanceToCam, out float parallaxHeight) {
	// fade out the effect over distance
	const float fadeOutDistance = PARALLAX_FADE_START; // start fading at this distance. (distance from the camera)
	const float maxDistance = PARALLAX_FADE_END;       // at this distance, the effect is faded out
	if (distanceToCam > maxDistance) return texCoords; // early out if (point on) surface is too far away..
	float distanceRatio = 1.0;
	if (distanceToCam > fadeOutDistance) distanceRatio = 1.0 - ((distanceToCam - fadeOutDistance) / (maxDistance - fadeOutDistance));

	// steep parallax mapping
	const int minLayers = 4;
	const int maxLayers = 32;
	float viewAngle = max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0);
	float numLayers = (maxLayers - minLayers) * viewAngle + minLayers;
	float layerDepth = 1.0 / numLayers;
	vec2 p = viewDir.xy / viewDir.z * depthscale * distanceRatio;
	vec2 deltaTexCoords = p * layerDepth;
	float curDepth = 0.0;
	vec2 curTexCoords = texCoords;
	float curDepthValue = 1.0 - texture2D(displaceMap, curTexCoords).a; // we could also invert the heightmap in the alpha channel of the image
	vec2 prevTexCoords = curTexCoords;
	float prevDepthValue = curDepthValue;
	// a while-loop in glsl is not so easy to compile. My old while loop went woot..
	// Using variables in the while-condition causes an endless loop.
	// a for-loop in glsl, using constants, works well..
	for (int L=minLayers; L<maxLayers; L++) {
		if (L-minLayers >= numLayers) break;  // and do the loop-condition checks inside the loop
		if (curDepth >= curDepthValue) break;
		prevTexCoords = curTexCoords;
		prevDepthValue = curDepthValue;
		curTexCoords -= deltaTexCoords;
		curDepthValue = 1.0 - texture2D(displaceMap, curTexCoords).a; // we use a heightmap, where we should use a "depthmap"
		curDepth += layerDepth;
	}

	// parallax occlusion mapping
	float afterDepth  = curDepthValue - curDepth;
	float beforeDepth = prevDepthValue - curDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);
	float weight_1 = 1.0 - weight;
	vec2 finalTexCoords = prevTexCoords * weight + curTexCoords * weight_1;
	parallaxHeight = curDepth + beforeDepth * weight + afterDepth * weight_1;
	return finalTexCoords;
}


// arguments:
//   - displaceMap is a normalmap, with in the alpha channel the heightmap data
//   - textCoords the incoming texture coordinates to parallax
//   - lightDir in tangent space
//   - depthscale is the uniform u_DepthScale
//   - parallaxHeight as calculated by the parallax() function
// The function result is a value that represents the amount of light there is (not how much shadow):
// For example: 1.0 = only light, no shadow.  And 0.0 = no light, just shadow
// So the more shadow you get, the more the function result approaches 0.
// If the value returned is 0, then the final pixel will be black (that is very! dark).
//
float parallaxSelfShadow(sampler2D displaceMap, vec2 texCoords, vec3 lightDir, float depthscale, float distanceToCam, float parallaxHeight)
{
	float lightAngle = dot(vec3(0.0, 0.0, 1.0), lightDir);
	// if the surface normal points away from the light, do not apply any self-shadow
	if (lightAngle <= 0) {
		return 1.0; // don't add shadow. Keep it as it is..
	}

	// fade out the effect over distance
	const float fadeOutDistance = PARALLAX_FADE_START; // start fading at this distance. (distance from the camera)
	const float maxDistance = PARALLAX_FADE_END;       // at this distance, the effect is faded out
	if (distanceToCam > maxDistance) return 1.0;       // early out if (point on) surface is too far away..
	float distanceRatio = 1.0;
	if (distanceToCam > fadeOutDistance) distanceRatio = 1.0 - ((distanceToCam - fadeOutDistance) / (maxDistance - fadeOutDistance));

	const int minLayers = 16;
	const int maxLayers = 32;
	float numLayers = (maxLayers - minLayers) * abs(lightAngle) + minLayers;
	float numLayers1 = 1.0 / numLayers;
	float layerHeight = parallaxHeight * numLayers1;
	vec2 deltaTexCoords = lightDir.xy / lightDir.z * numLayers1 * depthscale;
	float curHeight = parallaxHeight - layerHeight;
	vec2 curTexCoords = texCoords + deltaTexCoords;
	float curHeightValue = 1.0 - texture2D(displaceMap, curTexCoords).a;
	float shadow = 0.0;
	int numSamples = 0;
	float stepHeight = 1.0 - numLayers1;
	for (int L=minLayers; L<maxLayers; L++) {
		if (L-minLayers >= numLayers) break;
		if (curHeight <= 0) break;
		if (curHeightValue < curHeight) {
			numSamples += 1;
			float newShadow = (curHeight - curHeightValue) * stepHeight;
			shadow = max(shadow, newShadow);
		}
		stepHeight -= numLayers1;
		curHeight -= layerHeight;
		curTexCoords += deltaTexCoords;
		curHeightValue = 1.0 - texture2D(displaceMap, curTexCoords).a;
	}

	// if we traced down to the bottom, and didn't hit a bump, we are in full light
	if (numSamples == 0) {
		return 1.0;
	}

	shadow = 1.0 - shadow * distanceRatio; // fade out the amount of shadow over distance..
	return shadow;
}


// All in one: POM + self shadows
// The function result is a vec3:   result.xy = parallax final texture coordinates
//                                  result.z  = shadow factor
//
vec3 parallaxAndShadow(sampler2D displaceMap, vec2 texCoords, vec3 viewDir, vec3 lightDir, float depthscale, float distanceToCam, float shadowFactor) {
	// fade out the effect over distance
	const float fadeOutDistance = PARALLAX_FADE_START;            // start fading at this distance. (distance from the camera)
	const float maxDistance = PARALLAX_FADE_END;                  // at this distance, the effect is faded out
	if (distanceToCam > maxDistance) return vec3(texCoords, 1.0); // early out if (point on) surface is too far away..
	float distanceRatio = 1.0;
	if (distanceToCam > fadeOutDistance) distanceRatio = 1.0 - ((distanceToCam - fadeOutDistance) / (maxDistance - fadeOutDistance));

	// steep parallax mapping
	const int minLayers = 4;
	const int maxLayers = 32;
	float viewAngle = max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0);
	float numLayers = (maxLayers - minLayers) * viewAngle + minLayers;
	float layerDepth = 1.0 / numLayers;
	vec2 p = viewDir.xy / viewDir.z * depthscale * distanceRatio;
	vec2 deltaTexCoords = p * layerDepth;
	float curDepth = 0.0;
	vec2 curTexCoords = texCoords;
	float curDepthValue = 1.0 - texture2D(displaceMap, curTexCoords).a; // we could also invert the heightmap in the alpha channel of the image
	vec2 prevTexCoords = curTexCoords;
	float prevDepthValue = curDepthValue;
	// a while-loop in glsl is not so easy to compile. My old while loop went woot..
	// Using variables in the while-condition causes an endless loop.
	// a for-loop in glsl, using constants, works well..
	for (int L=minLayers; L<maxLayers; L++) {
		if (L-minLayers >= numLayers) break;  // and do the loop-condition checks inside the loop
		if (curDepth >= curDepthValue) break;
		prevTexCoords = curTexCoords;
		prevDepthValue = curDepthValue;
		curTexCoords -= deltaTexCoords;
		curDepthValue = 1.0 - texture2D(displaceMap, curTexCoords).a; // we use a heightmap, where we should use a "depthmap"
		curDepth += layerDepth;
	}

	// parallax occlusion mapping
	float afterDepth  = curDepthValue - curDepth;
	float beforeDepth = prevDepthValue - curDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);
	float weight_1 = 1.0 - weight;
	vec2 finalTexCoords = prevTexCoords * weight + curTexCoords * weight_1;

	// self shadowing
	shadowFactor = clamp(shadowFactor, 0.0, 1.0);
	if (shadowFactor == 0.0) {
		return vec3(finalTexCoords, 1.0);
	} else
	{
		float parallaxHeight = curDepth + beforeDepth * weight + afterDepth * weight_1;
		float lightAngle = dot(vec3(0.0, 0.0, 1.0), lightDir);
		// if the surface normal points away from the light, do not apply any self-shadow
		if (lightAngle <= 0) {
			return vec3(finalTexCoords, 1.0); // don't add shadow. Keep it as it is..
		}

		const int minLayers = 16;
		const int maxLayers = 32;
		float numLayers = (maxLayers - minLayers) * abs(lightAngle) + minLayers;
		float numLayers1 = 1.0 / numLayers;
		float layerHeight = parallaxHeight * numLayers1;
		vec2 deltaTexCoords = lightDir.xy / lightDir.z * numLayers1 * depthscale;
		float curHeight = parallaxHeight - layerHeight;
		vec2 curTexCoords = finalTexCoords + deltaTexCoords;
		float curHeightValue = 1.0 - texture2D(displaceMap, curTexCoords).a;
		float shadow = 0.0;
		int numSamples = 0;
		float stepHeight = 1.0 - numLayers1;
		for (int L=minLayers; L<maxLayers; L++) {
			if (L-minLayers >= numLayers) break;
			if (curHeight <= 0) break;
			if (curHeightValue < curHeight) {
				numSamples += 1;
				float newShadow = (curHeight - curHeightValue) * stepHeight;
				shadow = max(shadow, newShadow);
			}
			stepHeight -= numLayers1;
			curHeight -= layerHeight;
			curTexCoords += deltaTexCoords;
			curHeightValue = 1.0 - texture2D(displaceMap, curTexCoords).a;
		}

		// if we traced down to the bottom, and didn't hit a bump, we are in full light
		if (numSamples == 0) {
			return vec3(finalTexCoords, 1.0);
		}
		shadow = 1.0 - shadow * distanceRatio * shadowFactor; // fade out the amount of shadow over distance, and by cvar factor
		return vec3(finalTexCoords, shadow);
	}
}
