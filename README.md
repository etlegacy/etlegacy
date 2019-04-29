Enemy Territory: Legacy
==========

*This version aims for performance. It must run fast and smooth, or else nobody wants to play with it. Excel or die..*

INTRODUCTION
============

The goal of this branch is to make it work, and to make it run smoother while doing that..
Many calculational functions have been inlined, and rewritten in (at this moment, still unoptimized) SSE2/SSE3 intrinsics code.

The purpose of inlining functions, is to have less function-calls.
There are many calculational functions that are being called continuously. I prefer to have a bit larger code, for a game!, that runs faster. The penalty for inlining all those functions is minimal, in my opinion.

SSE2/SSE3 code is used for code that would benefit from doing 'the same thing' on each of their vector-components. SSE3 is able to do that same 'thing' on 4 values at the same time. ET uses a lot of vec3_t data types.. some were changed into vec4_t, so SSE can access that memory easier/faster,.. and/or to make better use of inlining.

There are some pieces of code that have their logical flow changed. While crawling the code, in search of optimizations, i encountered lots of locations where the best optimization was to change some fundamental logic.
A good example is that old 'personalModel' logic.. brrr


Anyway.. At current time, this branch only works on a Windows system.
The compiler settings should only make it compile with the new ETL_SSE stuff, if you compile on Windows using MSVS.


GENERAL NOTES
=============

### Solution Compiler Settings

All the seperate projects in the solution should have their compiler- & linker properties changed.
You need to tell MSVS to handle the SSE code.

Here are some screenshots of how to set the properties for each project.
You must change those settings for:
* cgame_mp_x86
* etl
* etlded
* qagame_mp_x86
* renderer_opengl2_x86
* ui_mp_x86

### Compiler
![Compiler Code Generation](https://github.com/etlegacy/etlegacy/tree/corec/compile_code_generation.jpg)
![Compiler Code Optimilization](https://github.com/etlegacy/etlegacy/tree/corec/compile_code_optimization.jpg)

### Linker
General
Optimization
![Linker General](https://github.com/etlegacy/etlegacy/tree/corec/link_general.jpg)
![Linker Optimilization](https://github.com/etlegacy/etlegacy/tree/corec/link_optimization.jpgg)


LICENSE
=======

### Enemy Territory: Legacy

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

OpenWolf GPL Source Code
Copyright (C) 2011 Dusan Jocic

XreaL GPL Source Code (renderer2)
Copyright (C) 2010-2011 Robert Beckebans

ET: Legacy
Copyright (C) 2012-2019 ET:Legacy Team <mail@etlegacy.com>

  ET: Legacy is free software: you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  ET: Legacy is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  ET: Legacy (see COPYING.txt). If not, see <http://www.gnu.org/licenses/>.

  ADDITIONAL TERMS:  The Wolfenstein: Enemy Territory GPL Source Code is also
  subject to certain additional terms. You should have received a copy of these
  additional terms immediately following the terms and conditions of the GNU GPL
  which accompanied the Wolf ET Source Code.  If not, please request a copy in
  writing from id Software at id Software LLC, c/o ZeniMax Media Inc., Suite 120,
  Rockville, Maryland 20850 USA.

  EXCLUDED CODE:  The code described below and contained in the Wolfenstein:
  Enemy Territory GPL Source Code release is not part of the Program covered by
  the GPL and is expressly excluded from its terms.  You are solely responsible
  for obtaining from the copyright holder a license for such code and complying
  with the applicable license terms.


### MD4 Message-Digest Algorithm

Copyright (C) 1991-1992, RSA Data Security, Inc. Created 1991. All rights reserved.

  License to copy and use this software is granted provided that it is identified
  as the "RSA Data Security, Inc. MD4 Message-Digest Algorithm" in all mater
  ial mentioning or referencing this software or this function.

  License is also granted to make and use derivative works provided that such work
  s are identified as "derived from the RSA Data Security, Inc. MD4 Message-Digest
  Algorithm" in all material mentioning or referencing the derived work.

  RSA Data Security, Inc. makes no representations concerning either the merchanta
  bility of this software or the suitability of this software for any particular p
  urpose. It is provided "as is" without express or implied warranty of any
  kind.


### MD5 Message-Digest Algorithm

The MD5 algorithm was developed by Ron Rivest. The public domain C language
implementation used in this program was written by Colin Plumb in 1993, no copyright
is claimed.

  This software is in the public domain. Permission to use, copy, modify, and
  distribute this software and its documentation for any purpose and without fee is
  hereby granted, without any conditions or restrictions. This software is provided
  "as is" without express or implied warranty.
