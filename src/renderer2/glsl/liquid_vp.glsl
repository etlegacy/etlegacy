/* liquid_vp.glsl */

attribute vec4 attr_Position;
attribute vec4 attr_Color;
attribute vec3 attr_Normal;
#if defined(USE_NORMAL_MAPPING)
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
attribute vec4 attr_TexCoord0;
#endif // USE_NORMAL_MAPPING

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform mat4 u_NormalTextureMatrix;
uniform vec3 u_ViewOrigin;
#if defined(USE_NORMAL_MAPPING)
uniform vec3 u_LightDir;
#if defined(USE_PARALLAX_MAPPING)
uniform float u_DepthScale;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
uniform vec4  u_PortalPlane;
#endif // USE_PORTAL_CLIPPING

varying vec4 var_LightColor;
varying vec3 var_Position;
varying vec3 var_ViewOrigin; // position - vieworigin
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying vec2 var_TexNormal; // these coords are never moving
#if defined(USE_WATER)
varying vec2 var_TexNormal2; // these coords might be moving (tcMod)
#endif // USE_WATER
varying vec3 var_LightDirection;
varying vec3 var_ViewOrigin2; // vieworigin in worldspace
#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S; // size and start position of search in texture space
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform position into world space
	var_Position = (u_ModelMatrix * attr_Position).xyz;

	// the vieworigin
	var_ViewOrigin = normalize(var_Position - u_ViewOrigin);

	// transform normal into world space
	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;

#if defined(USE_NORMAL_MAPPING)
	vec3 tangent = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	vec3 binormal = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;

	// in a vertex-shader there exists no gl_FrontFacing
	var_tangentMatrix = mat3(-tangent, -binormal, -var_Normal.xyz);

	// transform normalmap texcoords
	var_TexNormal.xy = attr_TexCoord0.st;
#if defined(USE_WATER)
	var_TexNormal2.xy = (u_NormalTextureMatrix * attr_TexCoord0).st;
#endif // USE_WATER

	var_LightColor = attr_Color;
	var_LightDirection = -normalize(u_LightDir);
	// the view origin in worldspace
	var_ViewOrigin2 = normalize(var_tangentMatrix * var_ViewOrigin);
#if defined(USE_PARALLAX_MAPPING)
	var_S = var_ViewOrigin2.xy * -u_DepthScale / var_ViewOrigin2.z;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
