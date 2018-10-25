/* reflection_CB_fp.glsl */

uniform samplerCube u_ColorMap;
uniform sampler2D   u_NormalMap;
uniform vec3        u_ViewOrigin;
uniform mat4        u_ModelMatrix;
uniform vec4        u_PortalPlane;

varying vec3 var_Position;
varying vec2 var_TexNormal;
varying vec4 var_Tangent;
varying vec4 var_Binormal;
varying vec4 var_Normal;

void main()
{
#if defined(USE_PORTAL_CLIPPING)
	{
		float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
		if (dist < 0.0)
		{
			discard;
			return;
		}
	}
#endif
	
	// compute incident ray in world space
	vec3 V = normalize(var_Position - u_ViewOrigin);

#if defined(USE_NORMAL_MAPPING)
	// compute normal in tangent space from normalmap
	vec3 N = normalize(2.0 * (texture2D(u_NormalMap, var_TexNormal.st).xyz - 0.5));

#if defined(r_NormalScale)
	if (r_NormalScale != 1.0) N.z *= r_NormalScale;
#endif

	// invert tangent space for twosided surfaces
	mat3 tangentToWorldMatrix;
#if defined(TWOSIDED)
	if (!gl_FrontFacing)
	{
		tangentToWorldMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	}
	else
#endif
	tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

	// transform normal into world space
	N = normalize(tangentToWorldMatrix * N);

#else

	vec3 N = normalize(var_Normal.xyz);
#endif

	// compute reflection ray
	vec3 R = reflect(V, N);

	gl_FragColor = textureCube(u_ColorMap, R).rgba;
	// gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
