//  tinygettext - A gettext replacement that works directly on .po files
//  Copyright (C) 2006 Ingo Ruhnke <grumbel@gmx.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_TINYGETTEXT_ICONV_HPP
#define HEADER_TINYGETTEXT_ICONV_HPP

#include <string>

#ifdef BUNDLED_SDL // Using SDL iconv
#  include "SDL.h"

#  define tinygettext_ICONV_CONST const
#  define tinygettext_iconv_t     SDL_iconv_t
#  define tinygettext_iconv       SDL_iconv
#  define tinygettext_iconv_open  SDL_iconv_open
#  define tinygettext_iconv_close SDL_iconv_close 

#else // Using pure iconv

#  include <iconv.h>

#  ifdef __MORPHOS__
#    define HAVE_ICONV_CONST
#    define ICONV_CONST const
#  endif

// In FreeBSD, iconv is included in libc starting from version 10.0.
#  ifdef __FreeBSD__
#    include <sys/param.h>
#    if __FreeBSD_version >= 1000100
#      define HAVE_ICONV_CONST
#      define ICONV_CONST const
#    endif
#  endif

#  ifdef HAVE_ICONV_CONST
#    define tinygettext_ICONV_CONST ICONV_CONST
#  else
#    define tinygettext_ICONV_CONST 
#  endif

#  define tinygettext_iconv_t     iconv_t
#  define tinygettext_iconv       iconv
#  define tinygettext_iconv_open  iconv_open
#  define tinygettext_iconv_close iconv_close 

#endif

namespace tinygettext {

class IConv
{
private:
  std::string to_charset;
  std::string from_charset;
  tinygettext_iconv_t cd;

public:
  IConv();
  IConv(const std::string& fromcode, const std::string& tocode);
  ~IConv();

  void set_charsets(const std::string& fromcode, const std::string& tocode);
  std::string convert(const std::string& text);

private:
  IConv (const IConv&);
  IConv& operator= (const IConv&);
};

} // namespace tinygettext

#endif

/* EOF */
