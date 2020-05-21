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
 * @file l_memory.c
 * @brief Memory allocation
 */

#include "../qcommon/q_shared.h"
#include "botlib.h"
#include "be_interface.h"

#ifdef ETLEGACY_DEBUG
//  #define MEMDEBUG
#define MEMORYMANEGER
#endif

#define MEM_ID      0x12345678l
#define HUNK_ID     0x87654321l

int allocatedmemory;
int totalmemorysize;
int numblocks;

#ifdef MEMORYMANEGER

typedef struct memoryblock_s
{
	unsigned long int id;
	void *ptr;
	int size;
#ifdef MEMDEBUG
	char *label;
	char *file;
	int line;
#endif //MEMDEBUG
	struct memoryblock_s *prev, *next;
} memoryblock_t;

memoryblock_t *memory;

/**
 * @brief LinkMemoryBlock
 * @param[in,out] block
 */
void LinkMemoryBlock(memoryblock_t *block)
{
	block->prev = NULL;
	block->next = memory;
	if (memory)
	{
		memory->prev = block;
	}
	memory = block;
}

/**
 * @brief UnlinkMemoryBlock
 * @param[in,out] block
 */
void UnlinkMemoryBlock(memoryblock_t *block)
{
	if (block->prev)
	{
		block->prev->next = block->next;
	}
	else
	{
		memory = block->next;
	}
	if (block->next)
	{
		block->next->prev = block->prev;
	}
}

#ifdef MEMDEBUG
void *GetMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
/**
 * @brief Allocate a memory block of the given size
 * @param[in] size
 * @return
 */
void *GetMemory(unsigned long size)
#endif //MEMDEBUG
{
	void          *ptr;
	memoryblock_t *block;

	ptr         = botimport.GetMemory(size + sizeof(memoryblock_t));
	block       = (memoryblock_t *) ptr;
	block->id   = MEM_ID;
	block->ptr  = (char *) ptr + sizeof(memoryblock_t);
	block->size = size + sizeof(memoryblock_t);
#ifdef MEMDEBUG
	block->label = label;
	block->file  = file;
	block->line  = line;
#endif //MEMDEBUG
	LinkMemoryBlock(block);
	allocatedmemory += block->size;
	totalmemorysize += block->size + sizeof(memoryblock_t);
	numblocks++;
	return block->ptr;
}

#ifdef MEMDEBUG
void *GetClearedMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
/**
 * @brief Allocate a memory block of the given size and clear it
 * @param[in] size
 * @return
 */
void *GetClearedMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
#ifdef MEMDEBUG
	ptr = GetMemoryDebug(size, label, file, line);
#else
	ptr = GetMemory(size);
#endif //MEMDEBUG
	Com_Memset(ptr, 0, size);
	return ptr;
}

#ifdef MEMDEBUG
void *GetHunkMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
/**
 * @brief Allocate a memory block of the given size
 * @param[in] size
 * @return
 */
void *GetHunkMemory(unsigned long size)
#endif //MEMDEBUG
{
	void          *ptr;
	memoryblock_t *block;

	ptr         = botimport.HunkAlloc(size + sizeof(memoryblock_t));
	block       = (memoryblock_t *) ptr;
	block->id   = HUNK_ID;
	block->ptr  = (char *) ptr + sizeof(memoryblock_t);
	block->size = size + sizeof(memoryblock_t);
#ifdef MEMDEBUG
	block->label = label;
	block->file  = file;
	block->line  = line;
#endif //MEMDEBUG
	LinkMemoryBlock(block);
	allocatedmemory += block->size;
	totalmemorysize += block->size + sizeof(memoryblock_t);
	numblocks++;
	return block->ptr;
}

#ifdef MEMDEBUG
void *GetClearedHunkMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
/**
 * @brief Allocate a memory block of the given size and clear it
 * @param[in] size
 * @return
 */
void *GetClearedHunkMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
#ifdef MEMDEBUG
	ptr = GetHunkMemoryDebug(size, label, file, line);
#else
	ptr = GetHunkMemory(size);
#endif //MEMDEBUG
	Com_Memset(ptr, 0, size);
	return ptr;
}

/**
 * @brief BlockFromPointer
 * @param[in] ptr
 * @param[in] str
 * @return
 */
memoryblock_t *BlockFromPointer(void *ptr, char *str)
{
	memoryblock_t *block;

	if (!ptr)
	{
#ifdef MEMDEBUG
		//char *crash = (char *) NULL;
		//crash[0] = 1;
		botimport.Print(PRT_FATAL, "%s: NULL pointer\n", str);
#endif // MEMDEBUG
		return NULL;
	}
	block = ( memoryblock_t * )((char *) ptr - sizeof(memoryblock_t));
	if (block->id != MEM_ID && block->id != HUNK_ID)
	{
		botimport.Print(PRT_FATAL, "%s: invalid memory block\n", str);
		return NULL;
	}
	if (block->ptr != ptr)
	{
		botimport.Print(PRT_FATAL, "%s: memory block pointer invalid\n", str);
		return NULL;
	}
	return block;
}

/**
 * @brief Free the given memory block
 * @param[in] ptr
 */
void FreeMemory(void *ptr)
{
	memoryblock_t *block;

	block = BlockFromPointer(ptr, "FreeMemory");
	if (!block)
	{
		return;
	}
	UnlinkMemoryBlock(block);
	allocatedmemory -= block->size;
	totalmemorysize -= block->size + sizeof(memoryblock_t);
	numblocks--;
	//
	if (block->id == MEM_ID)
	{
		botimport.FreeMemory(block);
	}
}

/**
 * @brief Get the size of the memory block in bytes
 * @param[in] ptr
 * @return The size of the memory block in bytes
 */
int MemoryByteSize(void *ptr)
{
	memoryblock_t *block;

	block = BlockFromPointer(ptr, "MemoryByteSize");
	if (!block)
	{
		return 0;
	}
	return block->size;
}

/**
 * @brief Prints the total used memory size
 */
void PrintUsedMemorySize(void)
{
	botimport.Print(PRT_MESSAGE, "total allocated memory: %d KB\n", allocatedmemory >> 10);
	botimport.Print(PRT_MESSAGE, "total botlib memory: %d KB\n", totalmemorysize >> 10);
	botimport.Print(PRT_MESSAGE, "total memory blocks: %d\n", numblocks);
}

/**
 * @brief Print all memory blocks with label
 */
void PrintMemoryLabels(void)
{
	memoryblock_t *block;
	int           i;

	PrintUsedMemorySize();
	i = 0;
	Com_Printf("\r\n");
	for (block = memory; block; block = block->next)
	{
#ifdef MEMDEBUG
		if (block->id == HUNK_ID)
		{
			Com_Printf("%6d, hunk %p, %8d: %24s line %6d: %s\r\n", i, block->ptr, block->size, block->file, block->line, block->label);
		}
		else
		{
			Com_Printf("%6d,      %p, %8d: %24s line %6d: %s\r\n", i, block->ptr, block->size, block->file, block->line, block->label);
		}
#endif //MEMDEBUG
		i++;
	}
}

/**
 * @brief Free all allocated memory
 */
void DumpMemory(void)
{
	memoryblock_t *block;

	for (block = memory; block; block = memory)
	{
		FreeMemory(block->ptr);
	}
	totalmemorysize = 0;
	allocatedmemory = 0;
}

#else

#ifdef MEMDEBUG
void *GetMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
/**
 * @brief Allocate a memory block of the given size
 * @param size
 * @return
 */
void *GetMemory(unsigned long size)
#endif //MEMDEBUG
{
	void              *ptr;
	unsigned long int *memid;

	ptr = botimport.GetMemory(size + sizeof(unsigned long int));
	if (!ptr)
	{
		return NULL;
	}
	memid  = (unsigned long int *) ptr;
	*memid = MEM_ID;
	return (char *) ptr + sizeof(unsigned long int);
}

#ifdef MEMDEBUG
void *GetClearedMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
/**
 * @brief Allocate a memory block of the given size and clear it
 * @param[in] size
 * @return
 */
void *GetClearedMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
#ifdef MEMDEBUG
	ptr = GetMemoryDebug(size, label, file, line);
#else
	ptr = GetMemory(size);
#endif //MEMDEBUG
	Com_Memset(ptr, 0, size);
	return ptr;
}

#ifdef MEMDEBUG
void *GetHunkMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
/**
 * @brief Allocate a memory block of the given size
 * @param[in] size
 * @return
 */
void *GetHunkMemory(unsigned long size)
#endif //MEMDEBUG
{
	void              *ptr;
	unsigned long int *memid;

	ptr = botimport.HunkAlloc(size + sizeof(unsigned long int));
	if (!ptr)
	{
		return NULL;
	}
	memid  = (unsigned long int *) ptr;
	*memid = HUNK_ID;
	return (char *)ptr + sizeof(unsigned long int);
}

#ifdef MEMDEBUG
void *GetClearedHunkMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
/**
 * @brief Allocate a memory block of the given size and clear it
 * @param[in] size
 * @return
 */
void *GetClearedHunkMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
#ifdef MEMDEBUG
	ptr = GetHunkMemoryDebug(size, label, file, line);
#else
	ptr = GetHunkMemory(size);
#endif //MEMDEBUG
	Com_Memset(ptr, 0, size);
	return ptr;
}

/**
 * @brief FreeMemory
 * @param[in] ptr
 */
void FreeMemory(void *ptr)
{
	unsigned long int *memid;

	memid = (unsigned long int *) ((char *) ptr - sizeof(unsigned long int));

	if (*memid == MEM_ID)
	{
		botimport.FreeMemory(memid);
	}
}

/**
 * @brief PrintUsedMemorySize
 * @note Empty
 */
void PrintUsedMemorySize(void)
{
}

/**
 * @brief PrintMemoryLabels
 * @note Empty
 */
void PrintMemoryLabels(void)
{
}

#endif
