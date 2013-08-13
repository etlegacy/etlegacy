/*
===========================================================================
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2012 Dusan Jocic <dusanjocic@msn.com>

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

/* refraction_C_vp.glsl */

attribute vec4		attr_Position;
attribute vec3		attr_Normal;

uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

varying vec3		var_Position;
varying vec3		var_Normal;

void	main()
{
	vec4 vertex;
	vec3 normal;
	vec4 position;
	
#if defined(USE_VERTEX_SKINNING)

	VertexSkinning_P_N(attr_Position, attr_Normal, position, normal);

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * vertex;
	
	// transform position into world space
	var_Position = mat3(u_ModelMatrix) * position.xyz;

	// transform normal into world space
	var_Normal = (u_ModelMatrix * vec4(normal, 0.0)).xyz;
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform position into world space
	var_Position = mat3(u_ModelMatrix) * attr_Position.xyz;

	// transform normal into world space
	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
}

