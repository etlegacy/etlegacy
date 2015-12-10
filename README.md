Enemy Territory: Legacy [![Build Status](https://travis-ci.org/etlegacy/etlegacy.png?branch=master)](https://travis-ci.org/etlegacy/etlegacy) [![Analysis Status](https://scan.coverity.com/projects/1160/badge.svg)](https://scan.coverity.com/projects/1160)
==========

[![Join the chat at https://gitter.im/etlegacy/etlegacy](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/etlegacy/etlegacy?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

*A second breath of life for Wolfenstein: Enemy Territory*

* Website: [http://www.etlegacy.com](http://www.etlegacy.com)
* Downloads: [http://www.etlegacy.com/download](http://www.etlegacy.com/download)
* Wiki/FAQ: [http://dev.etlegacy.com/projects/etlegacy/wiki](http://dev.etlegacy.com/projects/etlegacy/wiki)
* Forums: [http://dev.etlegacy.com/projects/etlegacy/boards](http://dev.etlegacy.com/projects/etlegacy/boards)
* Development (bug reports and feature requests): [http://dev.etlegacy.com](http://dev.etlegacy.com)
* Repository: [https://github.com/etlegacy/etlegacy](https://github.com/etlegacy/etlegacy)
* Translation: [https://www.transifex.com/projects/p/etlegacy/](https://www.transifex.com/projects/p/etlegacy/)
* Contact: [\#etlegacy](http://webchat.freenode.net/?channels=#etlegacy) on irc.freenode.net and [etlegacy/etlegacy](https://gitter.im/etlegacy/etlegacy) on Gitter.im


INTRODUCTION
============

Enemy Territory: Legacy is based on the [raedwulf-et](https://bitbucket.org/tcmreastwood/raedwulf-et/)
project which in turn is based on the [GPL'd source code](https://github.com/id-Software/Enemy-Territory) of Wolfenstein: Enemy Territory.

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

### Game data

Wolfenstein: Enemy Territory is a free release, and can be downloaded from [Splash Damage](http://www.splashdamage.com/content/download-wolfenstein-enemy-territory).

This source release contains only the engine and mod code but not any game data,
which is still covered by the original EULA and must be obeyed as usual.

In order to run ET: Legacy you will need to copy the original assets files
(*pak0.pk3*, *pak1.pk3* and *pak2.pk3*) to the etmain folder.


### Compatibility with Enemy Territory 2.60b

ET: Legacy remains compatible with the ET 2.60b version as much as possible.

Please note that ET: Legacy is *not* compatible with PunkBuster enabled servers.
ET: Legacy clients also cannot connect to servers running the ETPro mod.


### Linux 64 bit

Please remember that 64 bit ET: Legacy clients can only connect to servers running
mods providing a 64 bit version. You will be able to play 32 bit-only mods only if
you compile ET: Legacy on a 32 bit system or crosscompile it for 32 bit architecture
on a 64 bit system.

At the moment, only the Legacy mod is available in 64 bit version, while all other
existing mods are available in 32 bit only version.

In case you are a running a 64 bit system, you probably might want to use the
**bundled libraries** which are located in a separate *etlegacy-libs* repository and
can be automatically downloaded using the `git submodule` command. See the next
section for more details.


DEPENDENCIES
============

* **CMake** (compile-time only)
* **OpenGL**
* **GLEW** version 1.10
* **SDL** version 2.0.3
* **ZLib** version 1.2.8
* **MiniZip**
* **libjpeg-turbo** version 1.3, or **libjpeg** version 8
* **libcurl** (optional, enabled by default)
* **Lua** version 5.3 (optional, enabled by default)
* **Ogg Vorbis** (optional, enabled by default)
* **Theora** (optional, enabled by default)
* **Freetype** version 2 (optional, enabled by default)
* **SQLite** version 3 (optional, enabled by default)
* **OpenAL** (optional)
* **Jansson** (optional)

To get the latest source code install [git](http://git-scm.com/) and
clone our repository hosted at [Github.com](https://github.com/etlegacy/etlegacy):

    $ git clone git://github.com/etlegacy/etlegacy.git

If the required dependencies are not installed on your system run:

    $ git submodule init
    $ git submodule update

This downloads the essential dependencies into the `libs/`directory. You can choose
whether to use bundled libraries instead of the system ones by changing the
`BUNDLED_LIBS` variable in the CMakeList.txt configuration file. You can then select
which bundled libraries to use by toggling the respective `BUNDLED_XXX` variable.


COMPILE AND INSTALL
===================

To install the binaries system-wide, you need to compile ET: Legacy with hardcoded
fs_basepath.

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

* option A: **easybuild**

In terminal, run:

    $ ./easybuild.sh

ET: Legacy will be installed in `~/etlegacy`.

* option B: **command line**

In terminal, run:

    $ mkdir build && cd build && cmake ..

To compile, run:

    $ make

If you wish to install ET: Legacy system-wide, run:

    # make install

Be sure to set the CMake variables (see above) beforehand.


**NOTES:**

  * Even if you have a 64 bit linux distribution which provides 32 bit versions of all
  the required libraries, you might also need the development libraries (-devel packages)
  installed on your system.

  * In order to compile the jpeg-turbo library properly you will need the **nasm** assembler.


### Crosscompiling on Linux with MinGW-w64

In terminal, run:

    $ mkdir build && cd build
    $ cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-cross-mingw-linux.cmake ..
    $ make

By default, MinGW name is set to *i686-w64-mingw32*. You may have to change it in
`cmake/Toolchain-cross-mingw-linux.cmake` depending on how it is called on your system.


### Windows

Install:

  1. [Visual Studio Community](http://www.visualstudio.com/)
  2. [CMake](http://www.cmake.org/) and make sure it is added to your system PATH

* option A: **easybuild**

    1. run easybuild.bat

ET: Legacy will be installed in `My Documents\ETLegacy-Build`.

* option B: **Visual Studio**

    1. create a `build` directory inside the directory which contains ET: Legacy sources
    2. open *Visual Studio Command Prompt* in the Start menu
    3. change directory with `cd` to the newly created build directory

In the command prompt, run:

    cmake -G "NMake Makefiles" -DBUNDLED_LIBS=YES .. && nmake

or

    cmake -G "Visual Studio 10" -DBUNDLED_LIBS=YES ..

and open the resulting project in Visual Studio.


**NOTES:**

  * If compilation of bundled libraries is aborted for any reason, you will probably need to clean the
  libs directory and start over. This can be done by executing `git clean -df && git reset --hard HEAD`
  inside `libs/` directory.

  * If the build fails during libcurl compilation because of missing *sed* utility, download it from
  [GnuWin](http://gnuwin32.sourceforge.net/packages/sed.htm) and place it into your system path or
  copy it into `MSVC/VC/bin`. It also comes with Git and can be placed into your system path
  automatically if you select that option during Git installation.

  * In order to compile the jpeg library properly there is a need for a file named 'win32.mak'.
  Unfortunately this file isn't shipped with Windows 8.0 and 8.1 SDK versions.
  Solution: Get the Windows SDK 7 and copy 'win32.mak' to `libs/jpeturbo/`.

  * Compiling renderer 2 on Windows requires GNU sed (gsed) to be installed. It can be downloaded
  from the [GnuWin](http://gnuwin32.sourceforge.net/packages/sed.htm) website.


### Mac OS X

* option A: **easybuild**

In Terminal, run:

    $ ./easybuild.sh

This will put an 'etlegacy' folder into your user folder.

* option B: **command line**

    1. install [Xcode](https://developer.apple.com/xcode/downloads/)
    2. install [Homebrew](http://brew.sh/)
    3. install [Homebrew Cask](http://caskroom.io/)

Then brew the following packages in the terminal app:

    $ brew cask install xquartz
    $ brew install --universal gnu-sed cmake glew sdl2 jpeg-turbo curl lua libogg libvorbis libtheora freetype

The --universal flag ensures both 32bit and 64bit libraries are installed. Although your system curl library supports both architectures, you also need to install its headers.

In terminal, run:

    $ mkdir build && cd build && cmake ..

To compile, run:

    $ make

If you wish to install ET: Legacy system-wide, run:

    # make install

Be sure to set the CMake variables (see above) beforehand.


**NOTES**:

  * In the legacy mod folder, the cgame_mac and ui_mac files are redundant since they are in the 
  etl_bin.pk3 and will be extracted at runtime, so you can delete those. The client is named etl.app
  (and can safely be renamed), while the dedicated server is just a command-line binary named "etlded".


LICENSE
=======

### Enemy Territory: Legacy

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

OpenWolf GPL Source Code
Copyright (C) 2011 Dusan Jocic

ET: Legacy
Copyright (C) 2012-2015 Jan Simek <mail@etlegacy.com>

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
