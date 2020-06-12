/* reliefMapping_vp.glsl - Relief mapping helper functions */
// The heights are encoded in the alpha channel of an image.


// https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
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
	float numLayers = mix(maxLayers, minLayers, viewAngle);
//	float numLayers = (maxLayers - minLayers) * viewAngle + minLayers;
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
