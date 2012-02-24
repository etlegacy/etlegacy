/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <jsimek.cz@gmail.com>
 *
 * This file is part of ET: Legacy.
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
 * @file l_mem.h
 * @author Mr Elusive (MrElusive@demigod.demon.nl)
 */

//#define MEMDEBUG
#undef MEMDEBUG

#ifndef MEMDEBUG

void *GetClearedMemory( int size );
void *GetMemory( unsigned long size );

#else

#define GetMemory( size )             GetMemoryDebug( size, # size, __FILE__, __LINE__ );
#define GetClearedMemory( size )  GetClearedMemoryDebug( size, # size, __FILE__, __LINE__ );
//allocate a memory block of the given size
void *GetMemoryDebug( unsigned long size, char *label, char *file, int line );
//allocate a memory block of the given size and clear it
void *GetClearedMemoryDebug( unsigned long size, char *label, char *file, int line );
//
void PrintMemoryLabels( void );
#endif //MEMDEBUG

void FreeMemory( void *ptr );
int MemorySize( void *ptr );
void PrintMemorySize( unsigned long size );
int TotalAllocatedMemory( void );

