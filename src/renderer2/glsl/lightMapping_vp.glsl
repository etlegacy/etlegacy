/* lightMapping_vp.glsl */
#if defined(USE_DEFORM_VERTEXES)
#include "lib/deformVertexes"
#endif // USE_DEFORM_VERTEXES

// vertex attributes
attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0; // the diffuse/colormap texture coordinates
attribute vec4 attr_TexCoord1; // the lightmap texture coordinates
attribute vec4 attr_Color;
attribute vec3 attr_Normal;
#if defined(USE_NORMAL_MAPPING)
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
#endif // USE_NORMAL_MAPPING

// uniform variables
uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec4 u_ColorModulate;
uniform vec4 u_Color;
#if defined(USE_DIFFUSE)
uniform mat4 u_DiffuseTextureMatrix;
#if defined(USE_NORMAL_MAPPING)
uniform vec3 u_ViewOrigin;
uniform vec3 u_LightDir;
#if defined(USE_PARALLAX_MAPPING)
uniform float u_DepthScale;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#endif // USE_DIFFUSE
#if defined(USE_PORTAL_CLIPPING)
uniform vec4 u_PortalPlane;
#endif // USE_PORTAL_CLIPPING
#if defined(USE_DEFORM_VERTEXES)
uniform float u_Time;
#endif // USE_DEFORM_VERTEXES

// varying variables
varying vec3 var_Position;
varying vec4 var_Color;
varying vec2 var_TexLight;
varying vec3 var_Normal;
#if defined(USE_DIFFUSE)
varying vec2 var_TexDiffuse;
#if defined(USE_NORMAL_MAPPING)
varying vec3 var_Tangent;
varying vec3 var_biNormal;
varying mat3 var_tangentMatrix;
varying vec3 var_LightDirection;
varying vec3 var_ViewOrigin;
#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S;
varying vec3 var_ViewOrigin2;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#endif // USE_DIFFUSE
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
	vec4 position = attr_Position;

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position, attr_Normal, attr_TexCoord0.st, u_Time);
#endif // USE_DEFORM_VERTEXES

	// transform vertex position into homogenous clip-space
#if 1
	gl_Position = u_ModelViewProjectionMatrix * position;
#else // 1
	gl_Position.xy = attr_TexCoord1 * 2.0 - 1.0;
	gl_Position.z  = 0.0;
	gl_Position.w  = 1.0;
#endif // 1

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	// assign color
//	var_Color = attr_Color * u_ColorModulate + u_Color;
	var_Color = vec4(vec3(attr_Color.rgb), u_ColorModulate.a * 0.5 + 0.5); // the alphaGen value goes in the alpha=channel

	// get lightmap texture coordinates
	var_TexLight = attr_TexCoord1.st;

	var_Normal.xyz = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;

#if defined(USE_DIFFUSE)
	// transform diffusemap texcoords
	var_TexDiffuse = (u_DiffuseTextureMatrix * attr_TexCoord0).st;
#endif // USE_DIFFUSE

#if defined(USE_NORMAL_MAPPING)
	// transform tangentspace axis
	vec3 tangent  = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	vec3 binormal = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;
	var_Tangent = tangent;
	var_biNormal = binormal;

	// in a vertex-shader there exists no gl_FrontFacing
	var_tangentMatrix = mat3(-tangent, -binormal, -var_Normal.xyz);

	var_LightDirection = -normalize(u_LightDir);

	// the vieworigin
	var_ViewOrigin = normalize(var_Position - u_ViewOrigin);

#if defined(USE_PARALLAX_MAPPING)
	vec3 viewOrigin2 = normalize(var_tangentMatrix * var_ViewOrigin);
//	vec3 viewOrigin2 = normalize(vec3(dot(var_ViewOrigin,-tangent), dot(var_ViewOrigin,-binormal), dot(var_ViewOrigin,-var_Normal.xyz)));
	var_S = viewOrigin2.xy * (-u_DepthScale / viewOrigin2.z);
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
