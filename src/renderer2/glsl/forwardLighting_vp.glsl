/* forwardLighting_vp.glsl */
#if defined(USE_VERTEX_SKINNING)
#include "lib/vertexSkinning"
#endif // USE_VERTEX_SKINNING
#if defined(USE_VERTEX_ANIMATION)
#include "lib/vertexAnimation"
#endif // USE_VERTEX_ANIMATION
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
#if defined(USE_VERTEX_ANIMATION)
attribute vec4 attr_Position2;
attribute vec3 attr_Normal2;
#if defined(USE_NORMAL_MAPPING)
attribute vec3 attr_Tangent2;
attribute vec3 attr_Binormal2;
#endif // USE_NORMAL_MAPPING
#endif // USE_VERTEX_ANIMATION

uniform mat4 u_DiffuseTextureMatrix;
uniform mat4 u_LightAttenuationMatrix;
uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec4 u_ColorModulate;
uniform vec4 u_Color;
#if defined(USE_NORMAL_MAPPING)
uniform mat4 u_NormalTextureMatrix;
uniform mat4 u_SpecularTextureMatrix;
uniform vec3 u_ViewOrigin;
#endif // USE_NORMAL_MAPPING
#if defined(USE_DEFORM_VERTEXES)
uniform float u_Time;
#endif // USE_DEFORM_VERTEXES
#if defined(USE_VERTEX_ANIMATION)
uniform float u_VertexInterpolation;
#endif // USE_VERTEX_ANIMATION
#if defined(USE_PARALLAX_MAPPING)
uniform float u_DepthScale;
#endif // USE_PARALLAX_MAPPING
#if defined(USE_PORTAL_CLIPPING)
uniform vec4  u_PortalPlane;
#endif // USE_PORTAL_CLIPPING

varying vec3 var_Position;
varying vec4 var_TexDiffuse;
varying vec4 var_TexNormal;
varying vec4 var_TexAttenuation;
varying vec4 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying vec2 var_TexSpecular;
varying vec3 var_ViewOrigin; // vieworigin - position    !
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
	vec4 position;
	vec3 normal;
#if defined(USE_NORMAL_MAPPING)
	vec3 tangent;
	vec3 binormal;
#endif // USE_NORMAL_MAPPING

#if defined(USE_VERTEX_SKINNING)
	#if defined(USE_NORMAL_MAPPING)
	VertexSkinning_P_TBN(attr_Position, attr_Tangent, attr_Binormal, attr_Normal,
	                     position, tangent, binormal, normal);
	#else
	VertexSkinning_P_N(attr_Position, attr_Normal,
	                   position, normal);
	#endif
#elif defined(USE_VERTEX_ANIMATION)
	#if defined(USE_NORMAL_MAPPING)
	VertexAnimation_P_TBN(attr_Position, attr_Position2,
	                      attr_Tangent, attr_Tangent2,
	                      attr_Binormal, attr_Binormal2,
	                      attr_Normal, attr_Normal2,
	                      u_VertexInterpolation,
	                      position, tangent, binormal, normal);
	#else
	VertexAnimation_P_N(attr_Position, attr_Position2,
	                    attr_Normal, attr_Normal2,
	                    u_VertexInterpolation,
	                    position, normal);
	#endif
#else
	position = attr_Position;
	#if defined(USE_NORMAL_MAPPING)
	tangent  = attr_Tangent;
	binormal = attr_Binormal;
	#endif
	normal = attr_Normal;
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position,
	                           normal,
	                           attr_TexCoord0.st,
	                           u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	// calc light xy,z attenuation in light space
	var_TexAttenuation = u_LightAttenuationMatrix * position;

	// transform diffusemap texcoords
	var_TexDiffuse.xy = (u_DiffuseTextureMatrix * attr_TexCoord0).st;

	var_Normal.xyz = mat3(u_ModelMatrix) * normal;

#if defined(USE_NORMAL_MAPPING)
	tangent = (u_ModelMatrix * vec4(tangent, 0.0)).xyz;
	binormal = (u_ModelMatrix * vec4(binormal, 0.0)).xyz;

	// in a vertex-shader there exists no gl_FrontFacing
	var_tangentMatrix = mat3(tangent, binormal, var_Normal.xyz);
	// transpose(inverse()) ? on a rotationmatrix?       for a rotationmatrix, the transpose is equal to the inverse.
	//var_tangentMatrix = transpose(inverse(mat3(tangent, binormal, var_Normal.xyz)));

	// transform normalmap texcoords
	var_TexNormal.xy = (u_NormalTextureMatrix * attr_TexCoord0).st;

	// transform specularmap texture coords
	var_TexSpecular = (u_SpecularTextureMatrix * attr_TexCoord0).st;

	var_ViewOrigin = normalize(u_ViewOrigin - var_Position.xyz);
	var_ViewOrigin2 = normalize(var_tangentMatrix * var_ViewOrigin);

#if defined(USE_PARALLAX_MAPPING)
	var_S = var_ViewOrigin2.xy * -u_DepthScale / var_ViewOrigin2.z;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

	// assign color
	vec4 color = attr_Color * u_ColorModulate + u_Color;

	var_TexDiffuse.p = color.r;
	var_TexNormal.pq = color.gb;

#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
