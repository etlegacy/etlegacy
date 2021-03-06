ET: Legacy [![Travis Status](https://travis-ci.org/etlegacy/etlegacy.svg?branch=master)](https://travis-ci.org/etlegacy/etlegacy) [![AppVeyor status](https://ci.appveyor.com/api/projects/status/468s0285u3w4vfom/branch/master?svg=true)](https://ci.appveyor.com/project/rmarquis/etlegacy/branch/master) [![Analysis Status](https://scan.coverity.com/projects/1160/badge.svg)](https://scan.coverity.com/projects/1160) ![CodeQL](https://github.com/etlegacy/etlegacy/workflows/CodeQL/badge.svg) [![chat](https://img.shields.io/discord/260750790203932672.svg?logo=discord)](https://discord.gg/UBAZFys)
==========

*A second breath of life for Wolfenstein: Enemy Territory*

* Website: [https://www.etlegacy.com](https://www.etlegacy.com)
* Downloads: [https://www.etlegacy.com/download](https://www.etlegacy.com/download)
* Wiki/FAQ: [https://github.com/etlegacy/etlegacy/wiki](https://github.com/etlegacy/etlegacy/wiki)
* Development: [https://github.com/etlegacy/etlegacy](https://github.com/etlegacy/etlegacy)
* Assets Repository: [https://drive.google.com](https://drive.google.com/drive/folders/0Bw7Yu-pqzcSaLXEtVEVjZF82UEU)
* Documentation: [https://etlegacy.readthedocs.io/](https://etlegacy.readthedocs.io/)
* Lua API: [https://etlegacy-lua-docs.readthedocs.io](https://etlegacy-lua-docs.readthedocs.io)
* Translation: [https://www.transifex.com/etlegacy/etlegacy](https://www.transifex.com/etlegacy/etlegacy)
* Contact: [\#etlegacy](https://webchat.freenode.net/?channels=#etlegacy) on irc.freenode.net and [etlegacy/#etlegacy](https://discordapp.com/channels/260750790203932672/346956915814957067) on Discord.

## INTRODUCTION

ET: Legacy is an open source project based on the code of [Wolfenstein: Enemy Territory](https://www.splashdamage.com/games/wolfenstein-enemy-territory/) which [was released](https://www.splashdamage.com/news/wolfenstein-enemy-territory-goes-open-source/) in 2010 under the terms of the GPLv3.

There are two aspects to this project:

* An updated game engine, **ET: Legacy**, which aims to fix bugs and security exploits, remove old dependencies, add useful features and modernize its graphics while still remaining compatible with ET 2.60b and as many of its mods as possible.
* A new mod, **Legacy**, which aims to add many useful features and improvements while staying close to the original gameplay, as well as being lightweight and extensible through Lua scripts.

For more information consult our [wiki](https://github.com/etlegacy/etlegacy/wiki).

## CONTRIBUTING

See [CONTRIBUTING](CONTRIBUTING.md).

## SECURITY POLICY

See [SECURITY](SECURITY.md).

## GENERAL NOTES

### Game data

Wolfenstein: Enemy Territory is a free release, and can be downloaded from [Splash Damage](https://www.splashdamage.com/games/wolfenstein-enemy-territory/).

This source release contains only the engine and mod code but not any game data,
which is still covered by the original EULA and must be obeyed as usual.

In order to run ET: Legacy you will need to copy the original assets (*pak0.pk3*)
to the etmain folder.


### Compatibility with Enemy Territory 2.60b

ET: Legacy remains compatible with the ET 2.60b version as much as possible.

Please note that ET: Legacy is *not* compatible with PunkBuster enabled servers.
ET: Legacy clients also cannot connect to servers running the ETPro mod.


### Linux 64 bit

Please remember that 64 bit ET: Legacy clients can only connect to servers running
mods providing a 64 bit version. You will be able to play 32 bit-only mods only if
you compile ET: Legacy on a 32 bit system or cross-compile it for 32 bit architecture
on a 64 bit system.

At the moment, only the Legacy mod is available in 64 bit version, while all other
existing mods are available in 32 bit only version.

In case you are a running a 64 bit system, you probably might want to use the
**bundled libraries** which are located in a separate *etlegacy-libs* repository and
can be automatically downloaded using the `git submodule` command. See the next
section for more details.


## DEPENDENCIES

* **CMake** (compile-time only)
* **OpenGL**
* **GLEW**
* **SDL**
* **ZLib**
* **MiniZip**
* **libjpeg-turbo** or **libjpeg**
* **libcurl** (optional, enabled by default)
* **Lua** (optional, enabled by default)
* **Ogg Vorbis** (optional, enabled by default)
* **Theora** (optional, enabled by default)
* **Freetype** (optional, enabled by default)
* **libpng** (optional, enabled by default)
* **SQLite** (optional, enabled by default)
* **OpenAL** (optional, enabled by default)

Grab info about current lib versions from our [Libs Changelog](https://github.com/etlegacy/etlegacy/wiki/Libs-Changelog) wiki page.

To get the latest source code install [git](https://git-scm.com/) and
clone our repository hosted at [Github.com](https://github.com/etlegacy/etlegacy):

    $ git clone git://github.com/etlegacy/etlegacy.git

If the required dependencies are not installed on your system run:

    $ git submodule init
    $ git submodule update

This downloads the essential dependencies into the `libs/`directory. You can choose
whether to use bundled libraries instead of the system ones by changing the
`BUNDLED_LIBS` variable in the CMakeList.txt configuration file. You can then select
which bundled libraries to use by toggling the respective `BUNDLED_XXX` variable.


## COMPILE AND INSTALL

To install the binaries system-wide, you need to compile ET: Legacy with hardcoded
`fs_basepath`.

The following variables can be adjusted in CMake:

  * **`INSTALL_DEFAULT_BASEDIR`**: sets default `fs_basepath`, i.e. where etl and etlded
    executables look for data files. In most cases it is `CMAKE_INSTALL_PREFIX`+`INSTALL_DEFAULT_MODDIR`.
    Defaults to empty value, because we want `fs_basepath` to be the current working directory
    when not installing the game system-wide.

  * (optional) **`INSTALL_DEFAULT_BINDIR`**: Location for executables. Appended to `CMAKE_INSTALL_PREFIX`.
    Defaults to `bin`.

  * (optional) **`INSTALL_DEFAULT_MODDIR`**: Location for libraries and paks. Appended to
    `CMAKE_INSTALL_PREFIX`. Defaults to `share/etlegacy` and then `legacy` is appended to it.


### Linux

Install required dependencies.

* option A: **easybuild**

In terminal, run one of the following:

    $ ./easybuild.sh        # for compiling a 32 bit version or
    $ ./easybuild.sh -64    # for compiling a 64 bit version

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

By default, MinGW name is set to **`i686-w64-mingw32`**. You may have to change it in
`cmake/Toolchain-cross-mingw-linux.cmake` depending on how it is called on your system.


### Windows

Install:

  1. [Visual Studio Community](https://visualstudio.microsoft.com/) with the _Desktop Development with C++_ workload
  2. [CMake](https://cmake.org/) and make sure it is added to your system PATH

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

    cmake -G "Visual Studio 16" -DBUNDLED_LIBS=YES ..

and open the resulting project in Visual Studio.


**NOTES:**

  * If compilation of bundled libraries is aborted for any reason, you will probably need to clean the
  libs directory and start over. This can be done by executing `git clean -df && git reset --hard HEAD`
  inside `libs/` directory.

  * If the build fails during libcurl compilation because of missing *sed* utility, download it from
  [GnuWin](http://gnuwin32.sourceforge.net/packages/sed.htm) and place it into your system path or
  copy it into `MSVC/VC/bin`. It also comes with Git and can be placed into your system path
  automatically if you select that option during Git installation.


### Mac OS X

Install:

1. Xcode:
 * At least a recent Version of Xcode Command Line Tools (Terminal -> `xcode-select --install`)
 * or a complete Xcode IDE (through App Store or https://developer.apple.com/xcode/downloads/)
2. Homebrew (https://brew.sh/)

Then brew the following packages in the terminal app:

    $ brew cask install xquartz
    $ brew install gnu-sed cmake glew sdl2 minizip jpeg-turbo curl lua libogg libvorbis theora freetype libpng sqlite openal-soft autoconf nasm automake libtool

Depending on what brew version you're using (mostly older ones), you have to specify `brew install --universal` to get both 32bit and 64bit libs. If it throws an error, just use the command listed above. Although your system curl library supports both architectures, you also need to install its headers.

* option A: **easybuild**

There are many flags and options provided by easybuild.sh. The ET: Legacy version you can compile depends on the used Mac OS version.

If you're running **up to Mojave (10.14)**, use one the following flags in Terminal.app:

    $ ./easybuild.sh        # for compiling a 32 bit version or
    $ ./easybuild.sh -64    # for compiling a 64 bit version

This will put an 'etlegacy' folder with the selected arch into your user folder.

With Mac OS **Catalina (10.15) and above**, your only option is to compile and run a 64 bit client. Therefore you need to use the following flags:

    $ ./easybuild.sh -64 --osx=10.15    #watch out for the double dash at --osx !

Take a look into easybuild.sh for more information and further options/flags.

* option B: **command line**

In terminal, run:

    $ mkdir build && cd build && cmake ..

Look into easybuild.sh for all available CMake options.

To compile, run:

    $ make

If you wish to install ET: Legacy system-wide, run:

    # make install

Be sure to set the CMake variables (see above) beforehand.


**NOTES**:

  * In the legacy mod folder, the cgame_mac and ui_mac files are redundant since they are in the 
  mod .pk3 and will be extracted at runtime, so you can delete those. The client is named "ET Legacy.app"
  (and can safely be renamed), while the dedicated server is just a command-line binary named "etlded".


### Raspberry Pi

ET: Legacy supports both OpenGL and OpenGL ES on the Raspberry Pi.

**Tested devices**: 3B+, 4B.


**Required dependencies**

```
sudo apt-get install build-essential libfreeimage-dev libopenal-dev libpango1.0-dev libsndfile-dev libudev-dev \
libasound2-dev libjpeg8-dev libwebp-dev automake libgl1-mesa-glx libjpeg62-turbo libogg0 libopenal1 libvorbis0a \
libvorbisfile3 zlib1g libraspberrypi0 libraspberrypi-bin libraspberrypi-dev libx11-dev libglew-dev libegl1-mesa-dev \
nasm autoconf git cmake zip gcc g++ libtool libxrandr-dev x11proto-randr-dev
```


On the Pi 3B+, it is advised to add a slight overclock to the GPU to provide a better experience. Run `sudo nano /boot/config.txt` and
add the following to the config file:

```
core_freq=500
v3d_freq=500
```

You may be able to increase the overclock more than this, but increasing too far will be unstable and will likely crash
your Pi. If you experience crashes whilst having the overclock in place, decrease the values accordingly.


**Pi 3B+ install instructions**

Install using experimental OpenGL driver:

1.  Enable experimental OpenGL driver via the `raspi-config` command line application.
1.  Modify `easybuild.sh` and set the `FEATURE_RENDERER_GLES` flag to 0 under the `RPI` section.
1.  Increase GPU memory split to at least 256mb.
1.  Run `./easybuild.sh -RPI -j4` to build for Raspberry Pi.

Install using OpenGLES:

1.  Ensure the legacy OpenGL driver is enabled via the `raspi-config` command line application (it is by default).
1.  Modify `easybuild.sh` and set the `FEATURE_RENDERER_GLES` flag to 1 under the `RPI` section.
1.  Increase GPU memory split to at least 256mb.
1.  Run `./easybuild.sh -RPI -j4` to build for Raspberry Pi.
1.  Exit X11 to terminal in order to enable hardware accelerated rendering.


**Pi 4B install instructions**

The OpenGL driver used is the Fake KMS driver and currently both OpenGL and GLES are ran within an X11 session.
If you want to switch between OpenGL and GLES when installing ET: Legacy on the Pi 4, simply set the `FEATURE_RENDERER_GLES` flag to 0
or 1 under the `RPI` section within the `easybuild.sh` script.


## LICENSE

### ET: Legacy

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

OpenWolf GPL Source Code
Copyright (C) 2011 Dusan Jocic

XreaL GPL Source Code (renderer2)
Copyright (C) 2010-2011 Robert Beckebans

ET: Legacy
Copyright (C) 2012-2020 ET:Legacy Team <mail@etlegacy.com>

  ET: Legacy is free software: you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  ET: Legacy is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  ET: Legacy (see COPYING.txt). If not, see <https://www.gnu.org/licenses/>.

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
