ET: Legacy
==========

*A second breath of life for Wolfenstein: Enemy Territory*

Website: [http://www.etlegacy.com](http://etlegacy.com)

IRC: \#etlegacy on irc.freenode.net

Repository: [https://github.com/etlegacy/etlegacy](https://github.com/etlegacy/etlegacy)

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
http://www.splashdamage.com/content/download-wolfenstein-enemy-territory

This source release contains only the engine and mod code and not any game data, 
the game data is still covered by the original EULA and must be obeyed as usual.

Compatibility with Enemy Territory 2.60b
----------------------------------------------------------------------------

Please remember that only if you compile ET:L on a 32bit system or crosscompile it
for 32bit architecture on a 64bit system will you be able to play on 2.60b servers.

In case you are a running a 64bit system, you will probably want to use the 
**bundled libraries** which are located in a separate *etlegacy-libs* repository and
can be automatically downloaded using the `git submodule` command. See the next section 
for more details.

NOTE: Even if you have a 64bit linux distribution which provides 32bit versions of all
the required libraries, cURL library also needs source code (package with -devel suffix) 
configured for the different architecture and it is rarely packaged on 64bit distributions.

Dependencies
-----------------------------------------------------------------------------

* **CMake** (compile-time only)
* **libSDL 1.2**
* **libjpeg 8**
* **libcurl** (optional, enabled by default)
* **OGG Vorbis File** (optional)
* **OpenAL** (optional)
* **Freetype** (optional)

To get the latest source code install [git](http://git-scm.com/) and
clone our repository hosted at Github.com:

    $ git clone git://github.com/etlegacy/etlegacy.git

If the required dependencies are not installed on your system run:

    $ git submodule init
    $ git submodule update

This downloads the essential dependencies (libjpeg, libSDL and libcurl) into the `libs/` 
directory. If one of those libraries is not installed on your system CMake will use the
ones located in the `libs/` directory automatically. Otherwise, you can control this 
by changing the `BUNDLED_LIBS` variable in the CMake script.

Compile and install
-----------------------------------------------------------------------------

### Linux

In terminal run:

    $ mkdir build && cd build && cmake ..
    $ make

To install the binaries system-wide, run as root:

    # make install

Alternatively you can run the game by specifying the full path to the `etl` binary in the `build` directory.

### Crosscompiling on linux with mingw32

In terminal run:

    $ mkdir build && cd build
    $ cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-cross-mingw32-linux.cmake ..
    $ make

Mingw32 name is set to *i686-pc-mingw32* by default. You may have to change that
in `cmake/Toolchain-cross-mingw32-linux.cmake` depending on how it is called on your system.

### Windows

* option A: **Visual Studio**

    1. download free Visual Studio C++ Express 2010
    2. when you install CMake, make sure it is added into your system PATH
    3. open `Visual Studio Command Prompt (2010)` (search for it in the Start menu)
    4. run `cmake -G "NMake Makefiles" -DBUNDLED_LIBS=YES .. && nmake` in the ET:L directory
       ... or `cmake -G "Visual Studio 10" ..` and open the resulting project in VS 2010

* option B: open the CMakeLists.txt file in [QT Creator](http://qt.nokia.com/products/developer-tools).

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
