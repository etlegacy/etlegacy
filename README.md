ET: Legacy
==========

*A second breath of life for Wolfenstein: Enemy Territory*

INTRODUCTION
============

ET: Legacy is based on the [raedwulf-et project](https://bitbucket.org/tcmreastwood/raedwulf-et/) 
which in turn is based on the GPL'd source code of Wolfenstein: Enemy Territory. 

Its main goal is to fix bugs and clean up the codebase while remaining 
(somewhat) compatible with the ET 2.60b version. 

GENERAL NOTES
=============

Game data and patching:
-----------------------------------------------------------------------------

Wolfenstein: Enemy Territory is a free release, and can be downloaded from
http://www.splashdamage.com/content/wolfenstein-enemy-territory-barracks

This source release does not contain any game data, the game data is still
covered by the original EULA and must be obeyed as usual.

Compiling
-----------------------------------------------------------------------------

Install mercurial and get the latest code from the repository hosted at 
sourceforge.net: _hg clone http://hg.code.sf.net/p/etlegacy/code etlegacy-code_

Then just run: _make_

LICENSE
=======

See COPYING.txt for the GNU GENERAL PUBLIC LICENSE

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

IO on .zip files using portions of zlib
-----------------------------------------------------------------------------
lines	file(s)
4301	src/qcommon/unzip.c
Copyright (C) 1998 Gilles Vollant
zlib is Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

MD4 Message-Digest Algorithm
-----------------------------------------------------------------------------
lines   file(s)
289     src/qcommon/md4.c
Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All rights reserved.

License to copy and use this software is granted provided that it is identified
as the <93>RSA Data Security, Inc. MD4 Message-Digest Algorithm<94> in all mater
ial mentioning or referencing this software or this function.
License is also granted to make and use derivative works provided that such work
s are identified as <93>derived from the RSA Data Security, Inc. MD4 Message-Dig
est Algorithm<94> in all material mentioning or referencing the derived work.
RSA Data Security, Inc. makes no representations concerning either the merchanta
bility of this software or the suitability of this software for any particular p
urpose. It is provided <93>as is<94> without express or implied warranty of any
kind.
