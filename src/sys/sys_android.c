/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
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
 */
/**
 * @file sys_android.c
 */

#ifdef __ANDROID__

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/

/* Start up the ET Legacy app */
void Java_org_etlegacy_app_SDLActivity_nativeInit()
{
    Android_JNI_SetupThread();

    SDL_SetMainReady();

    /* Run the application code! */
    int status;
    char *argv[2];
    argv[0] = SDL_strdup("ET Legacy");
    // send r_fullscreen 0 with argv[1] because on fullscreen can cause some issues see: https://github.com/rafal1137/android-project/commit/d960cc244b17d8cc0d084f9c8dad9c1af4b2ba72#diff-b9bd293cfb066fe80c10d3fcdd0fd6cbL439
    argv[1] = 0;
    status = SDL_main(1, argv);

}

#endif /* __ANDROID__ */
