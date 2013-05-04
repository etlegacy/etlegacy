/*
===========================================================================
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/* depthFill_vp.glsl */

attribute vec4		attr_Position;
attribute vec3		attr_Normal;
attribute vec4		attr_TexCoord0;
attribute vec4		attr_Color;

uniform mat4		u_ColorTextureMatrix;
uniform vec3        u_AmbientColor;
uniform mat4		u_ModelViewProjectionMatrix;

uniform int			u_DeformGen;
uniform vec4		u_DeformWave;	// [base amplitude phase freq]
uniform vec3		u_DeformBulge;	// [width height speed]
uniform float		u_DeformSpread;
uniform float		u_Time;

varying vec2		var_Tex;
varying vec4		var_Color;

void	main()
{
#if defined(USE_VERTEX_SKINNING)
	vec4 position = vec4(0.0);
	
	for(int i = 0; i < 4; i++)
	{
		int boneIndex = int(attr_BoneIndexes[i]);
		float boneWeight = attr_BoneWeights[i];
		mat4  boneMatrix = u_BoneMatrix[boneIndex];
		
		position += (boneMatrix * attr_Position) * boneWeight;
	}
	
	//position = DeformPosition(position, attr_Normal, attr_TexCoord0.st);
#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition(	u_DeformGen,
								u_DeformWave,	// [base amplitude phase freq]
								u_DeformBulge,	// [width height speed]
								u_DeformSpread,
								u_Time,
								position,
								attr_Normal,
								attr_TexCoord0.st);
#endif	

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;
#else

	vec4 position = DeformPosition(attr_Position, attr_Normal, attr_TexCoord0.st);

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;
#endif
	
	// transform texcoords
	var_Tex = (u_ColorTextureMatrix * attr_TexCoord0).st;
	
#if defined(r_precomputedLighting)
	// assign color
	var_Color = attr_Color;
#else
	var_Color = vec4(u_AmbientColor, 1.0);
#endif
}
