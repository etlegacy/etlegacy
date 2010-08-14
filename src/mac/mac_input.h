/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

enum {
	IK_Attack = 4,
	IK_AltAttack,
	IK_Jump,
	IK_Crouch,
	IK_Next,
	IK_Prev,
	IK_Joy7,
	IK_Joy8,
	IK_Use,
	IK_Zoom,
	IK_Joy11,
	IK_Joy12,

	IK_JoyX,
	IK_JoyY,
	IK_JoyZ,
	IK_JoyR,
	IK_JoyU,
	IK_JoyV,
	IK_JoyPovLeft,
	IK_JoyPovRight,
	IK_JoyPovUp,
	IK_JoyPovDown
};

void IN_UpDown( void );
void IN_UpUp( void );
void IN_DownDown( void );
void IN_DownUp( void );
void IN_LeftDown( void );
void IN_LeftUp( void );
void IN_RightDown( void );
void IN_RightUp( void );
void IN_ForwardDown( void );
void IN_ForwardUp( void );
void IN_BackUp( void );

void IN_Button0Up( void );
void IN_Button0Down( void );
void IN_Button1Up( void );
void IN_Button1Down( void );
void IN_Button1Up( void );
void IN_Button2Down( void );
void IN_Button2Up( void );
void IN_Button3Down( void );
void IN_Button3Up( void );
void IN_Button4Down( void );
void IN_Button4Up( void );
void IN_Button5Down( void );
void IN_Button5Up( void );
void IN_Button6Down( void );
void IN_Button6Up( void );
void IN_Button7Down( void );
void IN_Button7Up( void );
