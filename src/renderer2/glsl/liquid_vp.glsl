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

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec4 u_ColorModulate;
#if defined(USE_DIFFUSE)
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
	uniform vec4  u_PortalPlane;
#endif // USE_PORTAL_CLIPPING
#if defined(USE_DEFORM_VERTEXES)
	uniform float u_Time;
#endif // USE_DEFORM_VERTEXES

//varying vec4 var_LightColor;
varying float var_alphaGen;
varying vec3 var_Position;
uniform vec4 u_Color;
#if defined(USE_DIFFUSE)
	varying vec2 var_TexDiffuse;        // possibly moving coords
#endif // USE_DIFFUSE
#if defined(USE_NORMAL_MAPPING)
	varying mat3 var_tangentMatrix;
	varying mat3 var_worldMatrix;
	varying vec2 var_TexNormal;         // these coords are never moving
	varying vec3 var_LightDirT;         // light direction in tangentspace
	varying vec3 var_ViewDirT;          // view direction in tangentspace
	#if defined(USE_PARALLAX_MAPPING)
		varying float var_distanceToCam;    //
	#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
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
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

//	var_LightColor = attr_Color; // * u_ColorModulate + u_Color;
//var_LightColor = vec4(1.0);

	// normalmap texcoords are not transformed
	var_TexNormal = attr_TexCoord0.st;

#if defined(USE_DIFFUSE)
	// tcmod transformed texcoords
	var_TexDiffuse = (u_DiffuseTextureMatrix * attr_TexCoord0).st;

	// the alpha value is the one set by alphaGen const <value>
//	var_alphaGen = u_ColorModulate.a; // * 0.5 + 0.5;
//!	var_alphaGen = u_Color.a;
#endif // USE_DIFFUSE
	var_alphaGen = u_Color.a;


#if defined(USE_NORMAL_MAPPING)
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
