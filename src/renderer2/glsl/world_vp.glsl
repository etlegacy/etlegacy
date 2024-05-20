/* world_vp.glsl */
#if defined(USE_DEFORM_VERTEXES)
#include "lib/deformVertexes"
#endif // USE_DEFORM_VERTEXES

// vertex attributes
attribute vec4 attr_Position;
attribute vec4 attr_Color;
attribute vec3 attr_Normal;
attribute vec4 attr_TexCoord0; // the diffuse/colormap texture coordinates
#if defined(USE_LIGHT_MAPPING)
attribute vec4 attr_TexCoord1;     // the lightmap texture coordinates
#endif // USE_LIGHT_MAPPING
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
uniform vec3 u_LightColor;
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
varying vec3 var_Normal;
#if defined(USE_LIGHT_MAPPING)
varying vec2 var_TexLight;    //map
#endif // USE_LIGHT_MAPPING
#if defined(USE_DIFFUSE)
varying vec2 var_TexDiffuse;
	#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;                 // world to tangent space
varying mat3 var_worldMatrix;                   // tangent to world space
varying vec3 var_LightDirW;                     // in worldspace
varying vec3 var_LightDirT;                     // in tangentspace
varying vec3 var_ViewDirT;                      // in tangentspace
varying vec3 var_ViewDirWn;                     // in worldspace normalized
		#if defined(USE_PARALLAX_MAPPING)
varying float var_distanceToCam;                // distance in world space, from camera to object
		#endif // USE_PARALLAX_MAPPING
	#endif // USE_NORMAL_MAPPING
#endif // USE_DIFFUSE
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

	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 1.0)).xyz; // worldspace

	// assign color
	var_Color = attr_Color * u_ColorModulate + u_Color;


#if defined(USE_DIFFUSE)
	// transform diffusemap texcoords
	var_TexDiffuse = (u_DiffuseTextureMatrix * attr_TexCoord0).st;
#endif // USE_DIFFUSE


#if defined(USE_LIGHT_MAPPING)
	// get lightmap texture coordinates
	var_TexLight = attr_TexCoord1.st;
#endif // USE_LIGHT_MAPPING


#if defined(USE_NORMAL_MAPPING)
	// from tangentspace to worldspace
	var_worldMatrix = mat3(attr_Tangent, attr_Binormal, attr_Normal); // u_ModelMatrix
	// from worldspace to tangentspace
	var_tangentMatrix = transpose(var_worldMatrix); // for an ortho rotation matrix, the inverse is simply the transpose..


	// from vertex to light
	var_LightDirW = normalize(u_LightDir);
	var_LightDirT = var_tangentMatrix * var_LightDirW;

	// from vertex to camera
	vec3 viewDirW = var_Position - u_ViewOrigin; // !! do not normalize
	var_ViewDirWn = normalize(viewDirW);
	var_ViewDirT  = var_tangentMatrix * viewDirW;


#if defined(USE_PARALLAX_MAPPING)
	var_distanceToCam = length(viewDirW);
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING


#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
