/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 *
 * @file sys_loadlib.h
 */

#if defined (DEDICATED) || defined (__AROS__)
#   ifdef _WIN32
#       include <windows.h>
#       define Sys_LoadLibrary(f)   (void *)LoadLibrary(f)
#       define Sys_UnloadLibrary(h)     FreeLibrary((HMODULE)h)
#       define Sys_LoadFunction(h, fn)   (void *)GetProcAddress((HMODULE)h, fn)
#       define Sys_LibraryError()   "unknown"
#   else
#       include <dlfcn.h>
#       define Sys_LoadLibrary(f)   dlopen(f, RTLD_NOW)
#       define Sys_UnloadLibrary(h)     dlclose(h)
#       define Sys_LoadFunction(h, fn)   dlsym(h, fn)
#       define Sys_LibraryError()   dlerror()
#   endif
#else
#   ifdef BUNDLED_SDL
#       include "SDL.h"
#       include "SDL_loadso.h"
#   else
#       include <SDL/SDL.h>
#       include <SDL/SDL_loadso.h>
#   endif
#   define Sys_LoadLibrary(f)   SDL_LoadObject(f)
#   define Sys_UnloadLibrary(h)     SDL_UnloadObject(h)
#   define Sys_LoadFunction(h, fn)   SDL_LoadFunction(h, fn)
#   define Sys_LibraryError()   SDL_GetError()
#endif
