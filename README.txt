Wolfenstein: Enemy Territory GPL source release
===============================================

This file contains the following sections:

GENERAL NOTES
LICENSE

GENERAL NOTES
=============

Game data and patching:
-----------------------

Wolfenstein: Enemy Territory is a free release, and can be downloaded from
http://www.splashdamage.com/content/wolfenstein-enemy-territory-barracks

This source release does not contain any game data, the game data is still
covered by the original EULA and must be obeyed as usual.

Install the latest version of the game for your platform to get the game data.

Compiling on win32:
-------------------

A Visual C++ 2008 project is provided in src\wolf.sln.
The solution file is compatible with the Express release of Visual C++.

In order to test your binaries, backup and remove Main\mp_bin.pk3, then replace
WolfMP.exe, Main\qagame_mp_x86.dll, Main\cgame_mp_x86.dll, Main\ui_mp_x86.dll
by your compiled versions. When starting the server make sure to specify
'Pure Server: No' in the advanced settings page.

Compiling on GNU/Linux x86:
---------------------------

Get scons from http://scons.org/ if your favorite distribution doesn't
package it.

run scons from the src/ directory. see scons --help for build options

If any problems occur, consult the internet.

Other platforms, updated source code, security issues:
------------------------------------------------------

If you have obtained this source code several weeks after the time of release
(August 2010), it is likely that you can find modified and improved
versions of the engine in various open source projects across the internet.
Depending what is your interest with the source code, those may be a better
starting point.


LICENSE
=======

See COPYING.txt for the GNU GENERAL PUBLIC LICENSE

ADDITIONAL TERMS:  The Wolfenstein: Enemy Territory GPL Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU GPL which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

EXCLUDED CODE:  The code described below and contained in the Wolfenstein: Enemy Territory GPL Source Code release is not part of the Program covered by the GPL and is expressly excluded from its terms.  You are solely responsible for obtaining from the copyright holder a license for such code and complying with the applicable license terms.

IO on .zip files using portions of zlib
---------------------------------------------------------------------------
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

JPEG library
-----------------------------------------------------------------------------
src/jpeg-6
Copyright (C) 1991-1995, Thomas G. Lane

Permission is hereby granted to use, copy, modify, and distribute this
software (or portions thereof) for any purpose, without fee, subject to these
conditions:
(1) If any part of the source code for this software is distributed, then this
README file must be included, with this copyright and no-warranty notice
unaltered; and any additions, deletions, or changes to the original files
must be clearly indicated in accompanying documentation.
(2) If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the work of
the Independent JPEG Group".
(3) Permission for use of this software is granted only if the user accepts
full responsibility for any undesirable consequences; the authors accept
NO LIABILITY for damages of any kind.

These conditions apply to any software derived from or based on the IJG code,
not just to the unmodified library.  If you use our work, you ought to
acknowledge us.

NOTE: unfortunately the README that came with our copy of the library has
been lost, so the one from release 6b is included instead. There are a few
'glue type' modifications to the library to make it easier to use from
the engine, but otherwise the dependency can be easily cleaned up to a
better release of the library.

CURL library
-----------------------------------------------------------------------------
src/curl-7.12.2
COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 1996 - 2004, Daniel Stenberg, <daniel@haxx.se>.

All rights reserved.

Permission to use, copy, modify, and distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright
notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of a copyright holder shall not
be used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization of the copyright holder.

FT2 library
-----------------------------------------------------------------------------
src/ft2
The FT2 library is being used under the GPL v2 as indicated by its LICENSE.txt
