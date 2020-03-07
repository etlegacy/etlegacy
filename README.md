**This is an experimental build for MacOSX Catalina.**

Please follow the original repository https://github.com/etlegacy/etlegacy to keep posted about the original project. Sooner or later they will provide a more reliable solution for this.

A big thank you to mukmuk etlegacy@discord for their help.


### Mac OS X

Install:

    1. [Xcode](https://developer.apple.com/xcode/downloads/)
    2. [Homebrew](http://brew.sh/)
    3. [Homebrew Cask](http://caskroom.io/)

Then brew the following packages in the terminal app:

    $ brew cask install xquartz
    $ brew install gnu-sed cmake glew sdl2 minizip jpeg-turbo curl lua libogg libvorbis theora freetype sqlite openal-soft nasm automake libtool
	
	To create the .dmg package you need install
	$ brew install GraphicsMagick nodejs librsvg
	$ npm install -g appdmg

* option A: **easybuild**

In Terminal, run:

    $ ./easybuild.sh -64
	
* This will put an 'etlegacy' folder into your user folder.

* It also generates an DMG package that you can use to install ETLegacy. This package already contains the Original ET dependencies. You can find the dmg installer in the **build** directory.

To see all the easybuild options:
    $ ./easybuild help

### Known Issues.

In my setup i'm experiencing visual problems when I start ETLegacy in the second time. A weird orange box appears, and we are not able to see the game menu. To solve that issue you must delete your etl_bin_v2.76.pk3 file and re-start the game.

```bash
rm /Users/$(whoami)/Library/Application\ Support/etlegacy/legacy/etl_bin_v2.76.pk3
```

Your game settings will kept untouched.



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
Copyright (C) 2012-2020 ET:Legacy Team <mail@etlegacy.com>

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
