/* normalMapping_fp.glsl - helper functions */


//=== Compute reflections =========================================================================================================================

// This function returns the color that's reflected.
// Note that the cubeProbe's textures of the cubemap are drawn on the inside of a cubeProbe.
// The reflection-vector is positioned at the center of a cubeProbe, and points to a pixel on the inside of the cubemap.
vec3 computeReflections(vec3 viewDir, vec3 normal, samplerCube envmap0, samplerCube envmap1, float interpolate, float intensity)
{
	vec3 R = reflect(viewDir, normal); // the reflection vector
	vec4 envColor0 = textureCube(envmap0, R).rgba;
	vec4 envColor1 = textureCube(envmap1, R).rgba;
	return mix(envColor0, envColor1, interpolate).rgb * intensity;
}

// the cubeprobes are aligned to world axis.
// The reflect vector needs to be in worldspace because of the cubeprobes textures.
vec3 computeReflectionsW(vec3 viewDir, vec3 normal, mat3 tangentToworld, samplerCube envmap0, samplerCube envmap1, float interpolate, float intensity)
{
	vec3 R = tangentToworld * reflect(viewDir, normal); // the reflection vector in worldspace
	vec4 envColor0 = textureCube(envmap0, R).rgba;
	vec4 envColor1 = textureCube(envmap1, R).rgba;
	return mix(envColor0, envColor1, interpolate).rgb * intensity;
}




//=== Compute specular lighting ===================================================================================================================

// The light direction is from light to surface.
// The view direction is from camera to surface.
vec3 computeSpecular(vec3 viewDir, vec3 normal, vec3 lightDir, vec3 lightColor, float exponent, float scale)
{
#if 1
	vec3 reflectDir = reflect(lightDir, normal);
	float d = dot(viewDir, reflectDir);
	if (d <= 0) return vec3(0.0, 0.0, 0.0);
	float intensity = pow(d, exponent);
	return scale * intensity * lightColor;
#else
	float dotNL = dot(normal, lightDir);
	if (dotNL < 0.0) return vec3(0.0, 0.0, 0.0);
	vec3 H = normalize(lightDir + viewDir); // the half-vector
	float dotNH = max(0.0, dot(normal, H));
	float intensity = pow(dotNH, exponent);
	return (intensity * scale * lightColor); // float * float * vec3  (should be better than: f*v*f)
#endif
}




//=== Compute diffuse lighting ====================================================================================================================

// Diffuse lighting looks a bit like shadowing an object.
// There is no real shadow being cast, but it renders surfaces darker when they are less facing the light.

float computeDiffuseLighting(vec3 normal, vec3 lightDir, float amount)
{
	if (amount == 0) return 1.0;
	float dotNL = dot(normal, normalize(lightDir));
	if (dotNL <= 0) return 1.0; // in full light
	float lambert = 1.0 - (dotNL * amount);
	return lambert;
//	float lambert = (1.0 - amount) + (dotNL * amount);
//	return lambert*lambert; // square the result: The old halfLambert was doing this
}

// this version is avoiding darkening surfaces in shadow even more
float computeDiffuseLighting2(vec3 surfaceNormal, vec3 pixelNormal, vec3 lightDir, float amount)
{
	if (amount == 0) return 1.0;
	vec3 L = normalize(lightDir);
	float dotPNL = dot(pixelNormal, L);
	float dotSNL = dot(surfaceNormal, L);
	amount *= max(0.0, dotSNL);
	return (1.0 - amount) + (dotPNL * 0.5 + 0.5) * amount;
}
