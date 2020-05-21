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
 * @file l_memory.h
 * @brief Memory allocation
 */

#ifndef INCLUDE_L_MEMORY_H
#define INCLUDE_L_MEMORY_H

#ifdef ETLEGACY_DEBUG
//  #define MEMDEBUG
#endif

#ifdef MEMDEBUG
#define GetMemory(size)             GetMemoryDebug(size, # size, __FILE__, __LINE__);
#define GetClearedMemory(size)      GetClearedMemoryDebug(size, # size, __FILE__, __LINE__);
// allocate a memory block of the given size
void *GetMemoryDebug(unsigned long size, char *label, char *file, int line);
// allocate a memory block of the given size and clear it
void *GetClearedMemoryDebug(unsigned long size, char *label, char *file, int line);
//
#define GetHunkMemory(size)         GetHunkMemoryDebug(size, # size, __FILE__, __LINE__);
#define GetClearedHunkMemory(size)  GetClearedHunkMemoryDebug(size, # size, __FILE__, __LINE__);
// allocate a memory block of the given size
void *GetHunkMemoryDebug(unsigned long size, char *label, char *file, int line);
// allocate a memory block of the given size and clear it
void *GetClearedHunkMemoryDebug(unsigned long size, char *label, char *file, int line);
#else
// allocate a memory block of the given size
void *GetMemory(unsigned long size);
// allocate a memory block of the given size and clear it
void *GetClearedMemory(unsigned long size);
//
// allocate a memory block of the given size
void *GetHunkMemory(unsigned long size);
// allocate a memory block of the given size and clear it
void *GetClearedHunkMemory(unsigned long size);
#endif

// free the given memory block
void FreeMemory(void *ptr);
// prints the total used memory size
void PrintUsedMemorySize(void);
// print all memory blocks with label
void PrintMemoryLabels(void);
// returns the size of the memory block in bytes
int MemoryByteSize(void *ptr);
// free all allocated memory
void DumpMemory(void);

#endif // #ifndef INCLUDE_L_MEMORY_H
