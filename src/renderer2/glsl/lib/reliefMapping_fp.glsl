/* reliefMapping_vp.glsl - Relief mapping helper functions */
// The heights are encoded in the alpha channel of an image.

#if 0
// input:          V = the view direction to the camera in tangentspace
//           Vscaled = parallaxScale * V.xy / V.z
//                 T = current texture coordinates
//         normalMap = the .rgb is the normal, the .a is the height
// returns a vec3, where .xy is the st offset,  and .z is the parralaxHeight

//test.. remove this function..or make it work (again)
vec3 parallaxReliefMappingOffset(vec3 V, vec2 Vscaled, vec2 T, sampler2D normalMap)
{
	const float minLayers = 8.0;
	const float maxLayers = 16.0;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0,0,1),V)));
	float layerHeight = 1.0 / numLayers;
	vec2 offset = Vscaled / numLayers;
	float curLayerHeight = 0.0;
	vec2 curTexCoords = T;
	float texHeight = texture2D(normalMap, curTexCoords).a;
	while (texHeight > curLayerHeight) {
		curLayerHeight += layerHeight;
		curTexCoords -= offset;
		texHeight = texture2D(normalMap, curTexCoords).a;
	}
	vec2 deltaTexCoords = offset * 0.5;
	float deltaHeight = layerHeight * 0.5;
	curTexCoords += deltaTexCoords;
	curLayerHeight -= deltaHeight;
	const int numSearches = 6;
	for (int i=0; i < numSearches; i++) {
		deltaTexCoords *= 0.5;
		deltaHeight *= 0.5;
		texHeight = texture2D(normalMap, curTexCoords).a;
		if (texHeight > curLayerHeight) {
			curTexCoords -= deltaTexCoords;
			curLayerHeight += deltaHeight;
		} else {
			curTexCoords += deltaTexCoords;
			curLayerHeight -= deltaHeight;
		}
	}
	return vec3(deltaTexCoords, curLayerHeight); //curTexCoords;
}


// the next two functions were taken from the iortcw project. to see it in action.
// There have been some minor adjustments though..

float SampleDepth(sampler2D normalMap, vec2 t) {
	return texture2D(normalMap, t).a; // this work fine when using the material "bumpmap displaceMap(normalmap.tga, heightmapRGB.tga)"
}

vec2 RayIntersectDisplaceMap(vec2 t, vec2 ds, sampler2D normalMap) {
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;
	const float stepSize = 1.0 / float(linearSearchSteps); 

	// current depth position
	float depth = 0.0;

	// best match found (starts with last position 1.0)
	float bestDepth = 1.0;

	// search front to back for first point inside object
	for (int i = 0; i < linearSearchSteps - 1; ++i) {
		depth += stepSize;
		float d = SampleDepth(normalMap, ds * depth + t);
		if (depth >= d) {
			bestDepth = depth; // store best depth
			break;
		}
	}

	// current size of search window
	float size = stepSize * 0.5;
	depth = bestDepth - size;

	// recurse around first point (depth) for closest match
	for (int i = 0; i < binarySearchSteps; ++i) {
		float d = SampleDepth(normalMap, ds * depth + t);
		size *= 0.5;
		if (depth >= d) {
			bestDepth = depth;
			depth -= size;
		} else {
			depth += size;
		}
	}

	return ds * bestDepth;
}
#endif



// This is etlegacy:      https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
// arguments:
//   - displaceMap is a normalmap, with in the alpha channel the heightmap data
//   - textCoords the incoming texture coordinates to parallax
//   - viewDir is the viewdir in tangent space
//   - depthscale is the uniform u_DepthScale
//   - distanceToCam in world units
// output:
//   the new texture coordinates
//
// Note: i want to make the parallax mapped surfaces change depth over distance from the viewer.
//       That way we can slowly render the effect invisible, and we do not need to render
//       any parallaxed surfaces that are too far away to notice the effect.
//       This way we don't have to parallax the whole world, but just the surfaces nearby.
// Update: It works great..
vec2 parallax(sampler2D displaceMap, vec2 texCoords, vec3 viewDir, float depthscale, float distanceToCam) {
	const float fadeOutDistance = 200.0;
	const float maxDistance = 256.0;
	if (distanceToCam > maxDistance) return texCoords; // early out if (point on) surface is too far away..
	float distanceRatio = 1.0;
	if (distanceToCam > fadeOutDistance) distanceRatio = 1.0 - ((distanceToCam - fadeOutDistance) / (maxDistance - fadeOutDistance));
#if 1
	// version 0:
	float height = 1.0 - texture2D(displaceMap, texCoords).a; // depends on how you define height: It could as well be depth.. (depth = 1 - height)
	vec2 p = viewDir.xy / viewDir.z * height * depthscale * distanceRatio;
	return texCoords - p;
#else
#if 0
	// version 1:
	const float numLayers = 8;
	float layerDepth = 1.0 / numLayers;
	vec2 p = viewDir.xy / viewDir.z * depthscale * distanceRatio;
	vec2 deltatexCoords = p / numLayers;
	float curDepth = 0.0;
	vec2 curTexCoords = texCoords;
	float curDepthValue = texture2D(displaceMap, curTexCoords).a;
	while (curDepth < curDepthValue) {
		curTexCoords -= deltatexCoords;
		curDepthValue = texture2D(displaceMap, curTexCoords).a;
		curDepth += layerDepth;
	}
	return curTexCoords;
#else
#if 0
	// version 2:
	// tbh: i see a difference, but it doesn't look better than version 0..  maybe with a better heightmap.
	// and for some woot reason, this code gives an error in TestFlare  !   but how? and why?   readpixel done too quickly?
	float minLayers = 4;
	float maxLayers = 32;
	float viewAngle = max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0);
//	float numLayers = mix(maxLayers, minLayers, viewAngle); // this produces no error, but my system hangs..
	float numLayers = (maxLayers - minLayers) * viewAngle + minLayers;
	float layerDepth = 1.0 / numLayers;
	vec2 p = viewDir.xy / viewDir.z * depthscale * distanceRatio;
	vec2 deltatexCoords = p / numLayers;
	float curDepth = 0.0;
	vec2 curTexCoords = texCoords;
	float curDepthValue = texture2D(displaceMap, curTexCoords).a;
	while (curDepth < curDepthValue) {
		curTexCoords -= deltatexCoords;
		curDepthValue = texture2D(displaceMap, curTexCoords).a;
		curDepth += layerDepth;
	}
	return curTexCoords;
#else
	// version 3:
	float minLayers = 4;
	float maxLayers = 32;
	float viewAngle = max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0);
//	float numLayers = mix(maxLayers, minLayers, viewAngle); // this produces no error, but my system hangs..
	float numLayers = (maxLayers - minLayers) * viewAngle + minLayers;
	float layerDepth = 1.0 / numLayers;
	vec2 p = viewDir.xy / viewDir.z * depthscale * distanceRatio;
	vec2 deltaTexCoords = p / numLayers;
	float curDepth = 0.0;
	vec2 curTexCoords = texCoords;
	float curDepthValue = texture2D(displaceMap, curTexCoords).a;
	while (curDepth < curDepthValue) {
		curTexCoords -= deltaTexCoords;
		curDepthValue = texture2D(displaceMap, curTexCoords).a;
		curDepth += layerDepth;
	}
	vec2 prevTexCoords = curTexCoords + deltaTexCoords;
	float afterDepth  = curDepthValue - curDepth;
	float beforeDepth = texture(displaceMap, prevTexCoords).r - curDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + curTexCoords * (1.0 - weight);
	return finalTexCoords;
#endif
#endif
#endif
}
