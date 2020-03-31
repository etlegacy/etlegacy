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
 * @file vm_local.h
 */

#ifndef INCLUDE_VM_LOCAL_H
#define INCLUDE_VM_LOCAL_H

#include "q_shared.h"
#include "qcommon.h"

/**
 * @enum opcode_t
 */
typedef enum
{
	OP_UNDEF,

	OP_IGNORE,

	OP_BREAK,

	OP_ENTER,
	OP_LEAVE,
	OP_CALL,
	OP_PUSH,
	OP_POP,

	OP_CONST,
	OP_LOCAL,

	OP_JUMP,

	//-------------------

	OP_EQ,
	OP_NE,

	OP_LTI,
	OP_LEI,
	OP_GTI,
	OP_GEI,

	OP_LTU,
	OP_LEU,
	OP_GTU,
	OP_GEU,

	OP_EQF,
	OP_NEF,

	OP_LTF,
	OP_LEF,
	OP_GTF,
	OP_GEF,

	//-------------------

	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,              ///< *(stack[top-1]) = stack[top]
	OP_ARG,

	OP_BLOCK_COPY,

	//-------------------

	OP_SEX8,
	OP_SEX16,

	OP_NEGI,
	OP_ADD,
	OP_SUB,
	OP_DIVI,
	OP_DIVU,
	OP_MODI,
	OP_MODU,
	OP_MULI,
	OP_MULU,

	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_BCOM,

	OP_LSH,
	OP_RSHI,
	OP_RSHU,

	OP_NEGF,
	OP_ADDF,
	OP_SUBF,
	OP_DIVF,
	OP_MULF,

	OP_CVIF,
	OP_CVFI
} opcode_t;

typedef intptr_t vmptr_t;

/**
 * @struct vmSymbol_s
 * @typedef vmSymbol_t
 * @brief
 */
typedef struct vmSymbol_s
{
	struct vmSymbol_s *next;
	int symValue;
	int profileCount;
	char symName[1];        ///< variable sized
} vmSymbol_t;

#define VM_OFFSET_PROGRAM_STACK     0
#define VM_OFFSET_SYSTEM_CALL       4

#define VM_SYSCALL_ARGS 16

/**
 * @struct vm_s
 * @brief
 */
struct vm_s
{
	// DO NOT MOVE OR CHANGE THESE WITHOUT CHANGING THE VM_OFFSET_* DEFINES
	// USED BY THE ASM CODE
	int programStack;               ///< the vm may be recursively entered
	intptr_t (*systemCall)(intptr_t *parms);

	//------------------------------------

	char name[MAX_QPATH];

	char fqpath[MAX_QPATH + 1] ;

	// for dynamic linked modules
	void *dllHandle;
	intptr_t(QDECL * entryPoint)(int callNum, ...);

	// for interpreted modules
	qboolean currentlyInterpreting;

	qboolean compiled;
	byte *codeBase;
	int codeLength;

	int *instructionPointers;
	int instructionPointersLength;

	byte *dataBase;
	int dataMask;

	int stackBottom;                ///< if programStack < stackBottom, error

	int numSymbols;
	struct vmSymbol_s *symbols;

	int callLevel;                  ///< for debug indenting
	int breakFunction;              ///< increment breakCount on function entry to this
	int breakCount;
	qboolean extract;
};

extern vm_t *currentVM;
extern int  vm_debugLevel;

void VM_Compile(vm_t *vm, vmHeader_t *header);
int VM_CallCompiled(vm_t *vm, int *args);

void VM_PrepareInterpreter(vm_t *vm, vmHeader_t *header);
int VM_CallInterpreted(vm_t *vm, int *args);

vmSymbol_t *VM_ValueToFunctionSymbol(vm_t *vm, int value);
int VM_SymbolToValue(vm_t *vm, const char *symbol);
const char *VM_ValueToSymbol(vm_t *vm, int value);
#ifdef DEBUG_VM
void VM_LogSyscalls(int *args);
#endif

#endif // #ifndef INCLUDE_VM_LOCAL_H
