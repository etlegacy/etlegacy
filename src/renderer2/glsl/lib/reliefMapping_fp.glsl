/* reliefMapping_vp.glsl - Relief mapping helper functions */

#if defined(USE_PARALLAX_MAPPING)
float RayIntersectDisplaceMap(vec2 dp, vec2 ds, sampler2D normalMap)
{
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;

	float depthStep = 1.0 / float(linearSearchSteps);

	// current size of search window
	float size = depthStep;

	// current depth position
	float depth = 0.0;

	// best match found (starts with last position 1.0)
	float bestDepth = 1.0;

	// search front to back for first point inside object
	for (int i = 0; i < linearSearchSteps - 1; ++i)
	{
		depth += size;

		vec4 t = texture2D(normalMap, dp + ds * depth);

		if (bestDepth > 0.996)       // if no depth found yet
		{
			if (depth >= t.w)
			{
				bestDepth = depth;  // store best depth
			}
		}
	}

	depth = bestDepth;

	// recurse around first point (depth) for closest match
	for (int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		vec4 t = texture2D(normalMap, dp + ds * depth);

		if (depth >= t.w)
		#ifdef RM_DOUBLEDEPTH
			if (depth <= t.z)
		#endif
		{
			bestDepth = depth;
			depth    -= 2.0 * size;
		}

		depth += size;
	}

	return bestDepth;
}
#endif
