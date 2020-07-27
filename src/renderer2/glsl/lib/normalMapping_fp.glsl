/* normalMapping_fp.glsl - Normalmapping helper functions */

// Compute reflections
// This function returns the color that's reflected.
// Note that the cubeProbe's textures of the cubemap are drawn on the inside of a cubeProbe.
// The reflection-vector is positioned at the center of a cubeProbe, and points to a pixel on the inside of the cubemap
vec3 computeReflections(vec3 viewDir, vec3 normal, samplerCube envmap0, samplerCube envmap1, float interpolate, float intensity)
{
	vec3 R = reflect(viewDir, normal); // the reflection vector
	vec4 envColor0 = textureCube(envmap0, R).rgba;
	vec4 envColor1 = textureCube(envmap1, R).rgba;
	return mix(envColor0, envColor1, interpolate).rgb * intensity;
}


// Compute the specular lighting
vec3 computeSpecular2(float dotNL, vec3 viewDir, vec3 normal, vec3 lightDir, vec3 lightColor, float exponent, float scale)
{
	if (dotNL < 0.0) return vec3(0.0);
	vec3 H = normalize(lightDir + viewDir); // the half-vector
	float dotNH = max(0.0, dot(normal, H));
	float intensity = pow(dotNH, exponent);
	return (intensity * scale * lightColor); // float * float * vec3  (should be better than: f*v*f)
}
vec3 computeSpecular(vec3 viewDir, vec3 normal, vec3 lightDir, vec3 lightColor, float exponent, float scale)
{
	float dotNL = dot(normal, lightDir);
	if (dotNL < 0.0) return vec3(0.0);
	vec3 H = normalize(lightDir + viewDir); // the half-vector
	float dotNH = max(0.0, dot(normal, H));
	float intensity = pow(dotNH, exponent);
	return (intensity * scale * lightColor); // float * float * vec3  (should be better than: f*v*f)
}


// compute the diffuse light term
// https://en.wikipedia.org/wiki/Lambert%27s_cosine_law
// Diffuse lighting looks a bit like shadowing an object.
// There is no real shadow being cast, but it renders surfaces darker when they are less facing the light.
// Changes in diffuse lighting are very visible: If the diffuse light value is 1.0, the surface will be rendered black (if it's not facing the light).
// If we don't want pitch black surfaces, we must take care that this value is always < 1.0.
// A value of 0.0 will effectively disable Lambertian.. the dark is gone.
// Keep the value in a range 0.0 to 1.0.
// Note: Before entities were always Half-Lambert. That is using a value of 0.5 for diffuse lighting.
//
// If you have a bumpmapped world, any bumps will become more visible the higher the diffuse lighting value.
// But as diffuse values go up, also the surfaces that are facing away from the light are getting darker.
// It's about finding the right balance for your taste. You don't want it too dark, but you want to see the bumps well.
//
// Diffuse lighting is applied to most objects: all world-brushes, entities, players, water..
float computeDiffuseLighting(float dotNL, float amount)
{
	float lambert = (1.0 - amount) + (dotNL * amount);
	//return lambert*lambert; // square the result: this also makes the result always >0. (The old halfLambert was doing this)
	return abs(lambert); // don't square, but abs instead. (the most useful values are so low already. squaring them, lowers them even more)
}
float computeDiffuseLighting2(vec3 normal, vec3 lightDir)
{
	return dot(normal, lightDir);
}
