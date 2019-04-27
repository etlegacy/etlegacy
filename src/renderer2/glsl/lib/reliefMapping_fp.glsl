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
#endif


// the next two functions were taken from the iortcw project.  (thanks)
// There have been some minor adjustments though..

float SampleDepth(sampler2D normalMap, vec2 t) {
//	return 1.0 - texture2D(normalMap, t).a; //!!!DEBUG!!! our alpha data has some issue, using the displaceMap() function..
	return texture2D(normalMap, t).r; // i would not have tried using .r ..
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
