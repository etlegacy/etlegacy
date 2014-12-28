ET: Legacy [![Build Status](https://travis-ci.org/etlegacy/etlegacy.png?branch=master)](https://travis-ci.org/etlegacy/etlegacy) [![Analysis Status](https://scan.coverity.com/projects/1160/badge.svg)](https://scan.coverity.com/projects/1160)
==========

*A second breath of life for Wolfenstein: Enemy Territory*

* Website: [http://www.etlegacy.com](http://www.etlegacy.com)
* Release downloads: [http://dev.etlegacy.com/download](http://dev.etlegacy.com/download)
* Wiki/FAQ: [http://dev.etlegacy.com/projects/etlegacy/wiki](http://dev.etlegacy.com/projects/etlegacy/wiki)
* Forums: [http://dev.etlegacy.com/projects/etlegacy/boards](http://dev.etlegacy.com/projects/etlegacy/boards)
* Development (bug reports and feature requests): [http://dev.etlegacy.com](http://dev.etlegacy.com)
* Repository: [https://github.com/etlegacy/etlegacy](https://github.com/etlegacy/etlegacy)
* IRC: [\#etlegacy](http://webchat.freenode.net/?channels=#etlegacy) on irc.freenode.net

INTRODUCTION
============

ET: Legacy is based on the [raedwulf-et project](https://bitbucket.org/tcmreastwood/raedwulf-et/)
which in turn is based on the GPL'd source code of Wolfenstein: Enemy Territory.

The main goals of the project are fixing bugs, cleaning up the codebase and adding useful features
while remaining compatible with the ET 2.60b version.

The Legacy mod is the default mod shipped with ET: Legacy. It aims to add many useful features and
improvements, while staying close to the original gameplay, as well as being lightweight and fully
extensible through Lua scripts.

For more information consult our [changelog](http://dev.etlegacy.com/projects/etlegacy/wiki/Changelog).

ET: Legacy development is a collaborative effort done in an open, transparent and friendly manner.
Anyone is welcome to join our efforts!

GENERAL NOTES
=============

Game data
-----------------------------------------------------------------------------

Wolfenstein: Enemy Territory is a free release, and can be downloaded from [Splash Damage](http://www.splashdamage.com/content/download-wolfenstein-enemy-territory).

This source release contains only the engine and mod code but not any game data,
which is still covered by the original EULA and must be obeyed as usual.

In order to run ET: Legacy you wil need to copy the original assets files
(*pak0.pk3*, *pak1.pk3* and *pak2.pk3*) to the etmain folder.

Compatibility with Enemy Territory 2.60b
----------------------------------------------------------------------------

Please remember that 64 bit ET: Legacy clients can only connect to servers running mods
providing a 64 bit version. You will be able to play 32 bit-only mods only if you compile
ET: Legacy on a 32 bit system or crosscompile it for 32 bit architecture on a 64 bit system.

At the moment, note that only the Legacy mod is available in 64 bit version, while all
other existing mods are available in 32 bit only version.

In case you are a running a 64 bit system, you probably might want to use the
**bundled libraries** which are located in a separate *etlegacy-libs* repository and
can be automatically downloaded using the `git submodule` command. See the next section
for more details.

Dependencies
-----------------------------------------------------------------------------

* **CMake** (compile-time only)
* **OpenGL**
* **GLEW** version 1.10
* **SDL** version 2.0.3
* **libjpeg-turbo** version 1.3, or **libjpeg** version 8
* **libcurl** (optional, enabled by default)
* **Lua** version 5.2 (optional, enabled by default)
* **Ogg Vorbis** (optional, enabled by default)
* **Freetype** version 2 (optional)
* **OpenAL** (optional)
* **Jansson** (optional)

To get the latest source code install [git](http://git-scm.com/) and
clone our repository hosted at [Github.com](https://github.com/etlegacy/etlegacy):

    $ git clone git://github.com/etlegacy/etlegacy.git

If the required dependencies are not installed on your system run:

    $ git submodule init
    $ git submodule update

This downloads the essential dependencies into the `libs/`directory. You can choose whether
to use bundled libraries instead of the system ones by changing the `BUNDLED_LIBS` variable
in the CMakeList.txt configuration file. You can then select which bundled libraries to use
by toggling the respective `BUNDLED_XXX` variable.

Compile and install
-----------------------------------------------------------------------------

To install the binaries system-wide, you need to compile ET: Legacy with hardcoded fs_basepath.

The following variables can be adjusted in CMake:
  * **INSTALL_DEFAULT_BASEDIR**: sets default *fs_basepath*, i.e. where etl and etlded
    executables look for data files. In most cases it is CMAKE_INSTALL_PREFIX+INSTALL_DEFAULT_MODDIR.
    Defaults to empty value, because we want *fs_basepath* to be the current working directory
    when not installing the game system-wide.
  * (optional) **INSTALL_DEFAULT_BINDIR**: Location for executables. Appended to CMAKE_INSTALL_PREFIX.
    Defaults to "bin".
  * (optional) **INSTALL_DEFAULT_MODDIR**: Location for libraries and paks. Appended to
    CMAKE_INSTALL_PREFIX. Defaults to "share/etlegacy" and then "legacy" is appended to it.


### Linux

* option A: **command line**

In terminal, run:

    $ mkdir build && cd build && cmake-gui ..

To compile, run:

    $ make

If you wish to install ET: Legacy system-wide, run:

    # make install

Be sure to set the CMake variables (see above) beforehand.

* option B: **easybuild**

In terminal, run:

    $ ./easybuild.sh

To install, run:

    $ cd build
    $ make install

ET: Legacy will be installed in `~/etlegacy`. To change it, set the CMake variables (see above) in the
easybuild.sh file beforehand.

**NOTE:**

Even if you have a 64 bit linux distribution which provides 32 bit versions of all
the required libraries, you might also need the development libraries (-devel packages)
installed on your system.


### Crosscompiling on Linux with MinGW-w64

In terminal, run:

    $ mkdir build && cd build
    $ cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-cross-mingw-linux.cmake ..
    $ make

By default, MinGW name is set to *i686-w64-mingw32*. You may have to change it in
`cmake/Toolchain-cross-mingw-linux.cmake` depending on how it is called on your system.


### Windows

* option A: **Visual Studio**

    1. install the free [Visual Studio](http://www.visualstudio.com/) (C++ Express 2010 or newer)
    2. install [CMake](http://www.cmake.org/) and make sure it is added to your system PATH
    3. create a `build` directory inside the directory which contains ET: Legacy sources
    4. open *Visual Studio Command Prompt* in the Start menu
    5. change directory with `cd` to the newly created build directory

In the command prompt, run:

    cmake -G "NMake Makefiles" -DBUNDLED_LIBS=YES .. && nmake

or

    cmake -G "Visual Studio 10" -DBUNDLED_LIBS=YES ..

and open the resulting project in Visual Studio.

* option B: **QtCreator**

    1. install the free [QtCreator](http://www.qt.io/download-open-source/)
    2. install [CMake](http://www.cmake.org/) and make sure it is added to your system PATH
    3. open the CMakeLists.txt file in QtCreator.

* option C: **easybuild**

    1. install the free [Visual Studio](http://www.visualstudio.com/) (C++ Express 2010 or newer)
    2. install [CMake](http://www.cmake.org/) and make sure it is added to your system PATH
    3. run easybuild.bat

ET: Legacy will be installed in `My Documents\ETLegacy-Build`. To change it, set the CMake variables
(see above) in the easybuild.bat file beforehand.

**NOTES:**

If compilation of bundled libraries is aborted for any reason, you will probably need to clean the
libs directory and start over. This can be done by executing `git clean -df && git reset --hard HEAD`
inside `libs/` directory.

If the build fails during libcurl compilation because of missing *sed* utility, download it from
[GnuWin](http://gnuwin32.sourceforge.net/packages/sed.htm) and place it into your system path or
copy it into `MSVC/VC/bin`. It also comes with Git and can be placed into your system path
automatically if you select that option during Git installation.

In order to compile the jpeg library properly there is a need for a file named 'win32.mak'.
Unfortunately this file isn't shipped with Windows 8.0 and 8.1 SDK versions.
Solution: Get the Windows SDK 7 and copy 'win32.mak' to `libs/jpeturbo/`.


### Mac OS X

* option A: **command line**

TBD

* option B: **easybuild**

In Terminal, run:

    $ ./easybuild.sh
    $ cd build && make install

This will put an 'etlegacy' folder into your user folder.

**NOTE**:

In the legacy mod folder, the cgame_mac and ui_mac files are redundant since they are in the
etl_bin.pk3 and will be extracted at runtime, so you can delete those. The client is named etl.app
(and can safely be renamed), while the dedicated server is just a command-line binary named "etlded".



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
