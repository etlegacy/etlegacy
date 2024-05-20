/* liquid_vp.glsl */
#if defined(USE_DEFORM_VERTEXES)
#include "lib/deformVertexes"
#endif // USE_DEFORM_VERTEXES

attribute vec4 attr_Position;
attribute vec4 attr_Color;
attribute vec4 attr_TexCoord0;
attribute vec3 attr_Normal;
#if defined(USE_NORMAL_MAPPING)
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
#endif // USE_NORMAL_MAPPING
#if defined(USE_LIGHT_MAPPING)
attribute vec4 attr_TexCoord1;     // the lightmap texture coordinates
#endif // USE_LIGHT_MAPPING

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

// There is rgbGen and alphaGen.
// It returns a vec4 (r,g,b,a) and this is send to the shaders in the u_ColorModulate vec4.
// The values are all 0.0 for 'rgb', except when "rgbGen vertex" or "rgbGen oneminusvertex" are used.
// The value is 0.0 for 'a', except when "alphaGen vertex" or "alphaGen oneminusvertex" are used.
// If "rgbGen vertex" is used, values for 'rgb' are 1,1,1.
// If "rgbGen oneminusvertex" is used, values for 'rgb' are -1,-1,-1.
// If "alphaGen vertex" is used, the value for 'a' is 1.
// If "alphaGen oneminusvertex" is used, the value for 'a' is -1.
uniform vec4 u_ColorModulate;

uniform vec4 u_Color;
#if defined(USE_WATER) || defined(USE_DIFFUSE)
uniform mat4 u_DiffuseTextureMatrix;
#endif // USE_DIFFUSE
#if defined(USE_NORMAL_MAPPING)
uniform vec3 u_ViewOrigin;
uniform vec3 u_LightDir;
	#if defined(USE_PARALLAX_MAPPING)
uniform float u_DepthScale;
	#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
uniform vec4 u_PortalPlane;
#endif // USE_PORTAL_CLIPPING
#if defined(USE_DEFORM_VERTEXES)
uniform float u_Time;
#endif // USE_DEFORM_VERTEXES

varying vec4 var_Color;
varying vec3 var_Position;
varying vec3 var_Normal;
#if defined(USE_LIGHT_MAPPING)
varying vec2 var_TexLight;              // lightmap texture coordinates
varying vec4 var_LightmapColor;
#endif // USE_LIGHT_MAPPING
#if defined(USE_WATER) || defined(USE_DIFFUSE)
varying vec2 var_TexDiffuse;            // possibly moving coords
#endif
#if defined(USE_DIFFUSE)
varying float var_alphaGen;
#endif // USE_DIFFUSE
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying mat3 var_worldMatrix;
varying vec2 var_TexNormal;             // these coords are never moving
varying vec3 var_LightDirT;             // light direction in tangentspace
varying vec3 var_ViewDirT;              // view direction in tangentspace
	#if defined(USE_PARALLAX_MAPPING)
varying float var_distanceToCam;            //
	#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide;     // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
	vec4 position = attr_Position;

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position, attr_Normal, attr_TexCoord0.st, u_Time);
#endif // USE_DEFORM_VERTEXES

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 1.0)).xyz;

	var_Color = attr_Color * u_ColorModulate + u_Color;

#if defined(USE_LIGHT_MAPPING)
	// get lightmap texture coordinates
	var_TexLight      = attr_TexCoord1.st;
	var_LightmapColor = attr_Color * u_ColorModulate + u_Color;
#endif // USE_LIGHT_MAPPING

#if defined(USE_WATER) || defined(USE_DIFFUSE)
	// tcmod transformed texcoords
	var_TexDiffuse = (u_DiffuseTextureMatrix * attr_TexCoord0).st;
#endif

#if defined(USE_DIFFUSE)
	// the alpha value is the one set by alphaGen const <value>
//	var_alphaGen = u_Color.a;
	var_alphaGen = var_Color.a;
//var_alphaGen = attr_Color.a;
#endif // USE_DIFFUSE


#if defined(USE_NORMAL_MAPPING)
	// normalmap texcoords are not transformed
	var_TexNormal = attr_TexCoord0.st;

	// from tangentspace to worldspace
	var_worldMatrix = mat3(attr_Tangent, attr_Binormal, attr_Normal); // u_ModelMatrix
	// from worldspace to tangentspace
	var_tangentMatrix = transpose(var_worldMatrix); // for an ortho rotation matrix, the inverse is simply the transpose..


	// from vertex to light
	vec3 lightDirW = normalize(u_LightDir);
	var_LightDirT = var_tangentMatrix * lightDirW;

	// from vertex to camera
	vec3 viewDirW = var_Position - u_ViewOrigin; // !! do not normalize
	var_ViewDirT = var_tangentMatrix * viewDirW;


#if defined(USE_PARALLAX_MAPPING)
	var_distanceToCam = length(viewDirW);
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING


#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
