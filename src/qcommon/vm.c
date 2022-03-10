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
 * @file vm.c
 * @brief Virtual machine intermix code and data symbol table
 *
 * A dll has one imported function: VM_SystemCall and one exported function:
 * Perform
 */

#include "vm_local.h"
#include "../sys/sys_local.h"

vm_t *currentVM = NULL;
vm_t *lastVM    = NULL;
int  vm_debugLevel;

#define MAX_VM      3
vm_t vmTable[MAX_VM];

const char *vmStrs[MAX_VM] =
{
	"GameVM",
	"CGameVM",
	"UIVM",
};

void VM_VmInfo_f(void);
void VM_VmProfile_f(void);

/**
 * @brief Converts a VM pointer to a C pointer and
 * checks to make sure that the range is acceptable
 * @param[in] p
 * @param length - unused
 * @return
 *
 * @note Unused
 */
void *VM_VM2C(vmptr_t p, int length)
{
	return (void *)p;
}

/**
 * @brief VM_Debug
 * @param[in] level
 */
void VM_Debug(int level)
{
	vm_debugLevel = level;
}

/**
 * @brief VM_Init
 */
void VM_Init(void)
{
	Cmd_AddCommand("vmprofile", VM_VmProfile_f, "Prints VM profile."); // FIXME: doesn't print anything with +set developer 1
	Cmd_AddCommand("vminfo", VM_VmInfo_f, "Prints info about registered VM.");

	Com_Memset(vmTable, 0, sizeof(vmTable));
}

/**
 * @brief Assumes a program counter value
 * @param[in,out] vm
 * @param[in] value
 * @return
 */
const char *VM_ValueToSymbol(vm_t *vm, int value)
{
	vmSymbol_t  *sym = vm->symbols;
	static char text[MAX_TOKEN_CHARS];

	if (!sym)
	{
		return "NO SYMBOLS";
	}

	// find the symbol
	while (sym->next && sym->next->symValue <= value)
	{
		sym = sym->next;
	}

	if (value == sym->symValue)
	{
		return sym->symName;
	}

	Com_sprintf(text, sizeof(text), "%s+%i", sym->symName, value - sym->symValue);

	return text;
}

#ifdef DEBUG_VM
/**
 * @brief For profiling, find the symbol behind this value
 * @param[in,out] vm
 * @param[in] value
 * @return
 */
vmSymbol_t *VM_ValueToFunctionSymbol(vm_t *vm, int value)
{
	vmSymbol_t        *sym = vm->symbols;
	static vmSymbol_t nullSym;

	if (!sym)
	{
		return &nullSym;
	}

	while (sym->next && sym->next->symValue <= value)
	{
		sym = sym->next;
	}

	return sym;
}
#endif

/**
 * @brief VM_SymbolToValue
 * @param[in,out] vm
 * @param[in] symbol
 * @return
 *
 * @note Unused
 */
int VM_SymbolToValue(vm_t *vm, const char *symbol)
{
	vmSymbol_t *sym;

	for (sym = vm->symbols; sym; sym = sym->next)
	{
		if (!strcmp(symbol, sym->symName))
		{
			return sym->symValue;
		}
	}
	return 0;
}

/**
 * @brief ParseHex
 * @param[in] text
 * @return
 *
 * @note Unused
 */
int ParseHex(const char *text)
{
	int value = 0;
	int c;

	while ((c = *text++) != 0)
	{
		if (c >= '0' && c <= '9')
		{
			value = value * 16 + c - '0';
			continue;
		}
		if (c >= 'a' && c <= 'f')
		{
			value = value * 16 + 10 + c - 'a';
			continue;
		}
		if (c >= 'A' && c <= 'F')
		{
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}

/**
 * @brief VM_LoadSymbols
 * @param[in,out] vm
 * @note Unused
 */
void VM_LoadSymbols(vm_t *vm)
{
	union
	{
		char *c;
		void *v;
	} mapfile;
	char       *text_p, *token;
	char       name[MAX_QPATH];
	char       symbols[MAX_QPATH];
	vmSymbol_t **prev, *sym;
	int        count;
	int        value;
	size_t     chars;
	int        segment;
	int        numInstructions;

	// don't load symbols if not developer
	if (!com_developer->integer)
	{
		return;
	}

	COM_StripExtension(vm->name, name, sizeof(name));
	Com_sprintf(symbols, sizeof(symbols), "vm/%s.map", name);
	FS_ReadFile(symbols, &mapfile.v);
	if (!mapfile.c)
	{
		Com_Printf("Couldn't load symbol file: %s\n", symbols);
		return;
	}

	numInstructions = vm->instructionPointersLength >> 2;

	// parse the symbols
	text_p = mapfile.c;
	prev   = &vm->symbols;
	count  = 0;

	while (1)
	{
		token = COM_Parse(&text_p);
		if (!token[0])
		{
			break;
		}
		segment = ParseHex(token);
		if (segment)
		{
			COM_Parse(&text_p);
			COM_Parse(&text_p);
			continue;       // only load code segment values
		}

		token = COM_Parse(&text_p);
		if (!token[0])
		{
			Com_Printf("WARNING: incomplete line at end of file\n");
			break;
		}
		value = ParseHex(token);

		token = COM_Parse(&text_p);
		if (!token[0])
		{
			Com_Printf("WARNING: incomplete line at end of file\n");
			break;
		}
		chars     = strlen(token);
		sym       = Hunk_Alloc(sizeof(*sym) + chars, h_high);
		*prev     = sym;
		prev      = &sym->next;
		sym->next = NULL;

		// convert value from an instruction number to a code offset
		if (value >= 0 && value < numInstructions)
		{
			value = vm->instructionPointers[value];
		}

		sym->symValue = value;
		Q_strncpyz(sym->symName, token, chars + 1);

		count++;
	}

	vm->numSymbols = count;
	Com_Printf("%i symbols parsed from %s\n", count, symbols);
	FS_FreeFile(mapfile.v);
}

/**
 * @brief Dlls will call this directly
 *
 * @details cg010206 The horror; the horror.
 *
 * The syscall mechanism relies on stack manipulation to get it's args.
 *  This is likely due to C's inability to pass "..." parameters to
 *  a function in one clean chunk. On PowerPC Linux, these parameters
 *  are not necessarily passed on the stack, so while (&arg[0] == arg)
 *  is true, (&arg[1] == 2nd function parameter) is not necessarily
 *  accurate, as arg's value might have been stored to the stack or
 *  other piece of scratch memory to give it a valid address, but the
 *  next parameter might still be sitting in a register.
 *
 * Quake's syscall system also assumes that the stack grows downward,
 *  and that any needed types can be squeezed, safely, into a signed int.
 *
 * This hack below copies all needed values for an argument to a
 *  array in memory, so that Quake can get the correct values. This can
 *  also be used on systems where the stack grows upwards, as the
 *  presumably standard and safe stdargs.h macros are used.
 *
 * As for having enough space in a signed int for your datatypes, well,
 *  it might be better to wait for DOOM 3 before you start porting.  :)
 *
 * The original code, while probably still inherently dangerous, seems
 *  to work well enough for the platforms it already works on. Rather
 *  than add the performance hit for those platforms, the original code
 *  is still in use there.
 *
 * For speed, we just grab 15 arguments, and don't worry about exactly
 *  how many the syscall actually needs; the extra is thrown away.
 *
 * @param[in] arg
 * @return
 */
intptr_t QDECL VM_DllSyscall(intptr_t arg, ...)
{
#if defined(__x86_64__) || defined (__llvm__) || defined(__ANDROID__) || ((defined __linux__) && (defined __powerpc__))
	// rcg010206 - see commentary above
	intptr_t args[VM_SYSCALL_ARGS] = { 0 };
	int      i;
	va_list  ap;
	size_t len = ARRAY_LEN(args);

	args[0] = arg;

	va_start(ap, arg);
	for (i = 1; i < len; i++)
	{
		args[i] = va_arg(ap, intptr_t);
		if (VM_CALL_END == (int) args[i])
		{
			args[i] = 0;
			break;
		}
	}
	va_end(ap);

	return currentVM->systemCall(args);
#else                           // original id code
	return currentVM->systemCall(&arg);
#endif
}

/**
 * @brief Reload the data, but leave everything else in place
 * This allows a server to do a map_restart without changing memory allocation
 * @param[in,out] vm
 * @return
 */
vm_t *VM_Restart(vm_t *vm)
{
	vmHeader_t *header;
	size_t     dataLength;
	int        i;
	char       filename[MAX_QPATH];

	// DLL's can't be restarted in place
	if (vm->dllHandle)
	{
		char     name[MAX_QPATH];
		intptr_t (*systemCall)(intptr_t *parms);
		qboolean extract = vm->extract;

		systemCall = vm->systemCall;
		Q_strncpyz(name, vm->name, sizeof(name));

		VM_Free(vm);

		vm = VM_Create(name, extract, systemCall, VMI_NATIVE);
		return vm;
	}

	// load the image
	Com_Printf("VM_Restart()\n");
	Com_sprintf(filename, sizeof(filename), "vm/%s.qvm", vm->name);
	Com_Printf("Loading vm file %s.\n", filename);
	FS_ReadFile(filename, (void **)&header);
	if (!header)
	{
		Com_Error(ERR_DROP, "VM_Restart: restart failed.");
	}

	// byte swap the header
	// FIXME: Division of result of sizeof() on pointer type. This seems suspect.
	for (i = 0; i < sizeof(*header) / 4; i++)
	{
		((int *)header)[i] = LittleLong(((int *)header)[i]);
	}

	// validate
	if (header->vmMagic != VM_MAGIC
	    || header->bssLength < 0 || header->dataLength < 0 || header->litLength < 0 || header->codeLength <= 0)
	{
		VM_Free(vm);
		Com_Error(ERR_FATAL, "VM_Restart: %s has bad header", filename);
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header->dataLength + header->litLength + header->bssLength;
	for (i = 0; dataLength > (1 << i); i++)
	{
	}
	dataLength = 1 << i;

	// clear the data
	Com_Memset(vm->dataBase, 0, dataLength);

	// copy the intialized data
	Com_Memcpy(vm->dataBase, (byte *) header + header->dataOffset, header->dataLength + header->litLength);

	// byte swap the longs
	for (i = 0; i < header->dataLength; i += 4)
	{
		*(int *)(vm->dataBase + i) = LittleLong(*(int *)(vm->dataBase + i));
	}

	// free the original file
	FS_FreeFile(header);

	return vm;
}

#define STACK_SIZE  0x20000

/**
 * @brief If image ends in .qvm it will be interpreted, otherwise it will attempt to
 * load as a system dll
 * @param[in] module
 * @param[in] extract
 * @param systemCalls
 * @param[in] interpret
 * @return
 */
vm_t *VM_Create(const char *module, qboolean extract, intptr_t (*systemCalls)(intptr_t *), vmInterpret_t interpret)
{
	vm_t *vm;
	int  i;

	if (!module || !module[0] || !systemCalls)
	{
		Com_Error(ERR_FATAL, "VM_Create: bad parms");
	}

	// see if we already have the VM
	for (i = 0; i < MAX_VM; i++)
	{
		if (!Q_stricmp(vmTable[i].name, module))
		{
			vm = &vmTable[i];
			return vm;
		}
	}

	// find a free vm
	for (i = 0; i < MAX_VM; i++)
	{
		if (!vmTable[i].name[0])
		{
			break;
		}
	}

	if (i == MAX_VM)
	{
		Com_Error(ERR_FATAL, "VM_Create: no free vm_t");
	}

	vm = &vmTable[i];

	Q_strncpyz(vm->name, module, sizeof(vm->name));
	vm->systemCall = systemCalls;
	vm->extract    = extract;

	if (interpret == VMI_NATIVE)
	{
		// try to load as a system dll
		vm->dllHandle = Sys_LoadGameDll(module, extract, &vm->entryPoint, VM_DllSyscall);

		// never try qvm
		if (vm->dllHandle)
		{
			return vm;
		}
		return NULL;
	}

#if 0
	// load the image
	Com_sprintf(filename, sizeof(filename), "vm/%s.qvm", vm->name);
	Com_Printf("Loading vm file %s.\n", filename);
	length = FS_ReadFile(filename, (void **)&header);
	if (!header)
	{
		Com_Printf("Failed.\n");
		VM_Free(vm);
		return NULL;
	}

	// byte swap the header
	for (i = 0; i < sizeof(*header) / 4; i++)
	{
		((int *)header)[i] = LittleLong(((int *)header)[i]);
	}

	// validate
	if (header->vmMagic != VM_MAGIC
	    || header->bssLength < 0 || header->dataLength < 0 || header->litLength < 0 || header->codeLength <= 0)
	{
		VM_Free(vm);
		Com_Error(ERR_FATAL, "%s has bad header", filename);
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header->dataLength + header->litLength + header->bssLength;
	for (i = 0; dataLength > (1 << i); i++)
	{
	}
	dataLength = 1 << i;

	// allocate zero filled space for initialized and uninitialized data
	vm->dataBase = Hunk_Alloc(dataLength, h_high);
	vm->dataMask = dataLength - 1;

	// copy the intialized data
	Com_Memcpy(vm->dataBase, (byte *) header + header->dataOffset, header->dataLength + header->litLength);

	// byte swap the longs
	for (i = 0; i < header->dataLength; i += 4)
	{
		*(int *)(vm->dataBase + i) = LittleLong(*(int *)(vm->dataBase + i));
	}

	// allocate space for the jump targets, which will be filled in by the compile/prep functions
	vm->instructionPointersLength = header->instructionCount * 4;
	vm->instructionPointers       = Hunk_Alloc(vm->instructionPointersLength, h_high);

	// copy or compile the instructions
	vm->codeLength = header->codeLength;

	if (interpret >= VMI_COMPILED)
	{
		vm->compiled = qtrue;
		VM_Compile(vm, header);
	}
	else
	{
		vm->compiled = qfalse;
		VM_PrepareInterpreter(vm, header);
	}

	// free the original file
	FS_FreeFile(header);

	// load the map file
	VM_LoadSymbols(vm);

	// the stack is implicitly at the end of the image
	vm->programStack = vm->dataMask + 1;
	vm->stackBottom  = vm->programStack - STACK_SIZE;

	Com_Printf("%s loaded in %d bytes on the hunk\n", module, remaining - Hunk_MemoryRemaining());

	return vm;
#else
	return NULL;
#endif
}

/**
 * @brief VM_Free
 * @param[in,out] vm
 */
void VM_Free(vm_t *vm)
{
	if (vm->dllHandle)
	{
		Sys_UnloadDll(vm->dllHandle);
		Com_Memset(vm, 0, sizeof(*vm));
	}
#if 0                           // now automatically freed by hunk
	if (vm->codeBase)
	{
		Z_Free(vm->codeBase);
	}
	if (vm->dataBase)
	{
		Z_Free(vm->dataBase);
	}
	if (vm->instructionPointers)
	{
		Z_Free(vm->instructionPointers);
	}
#endif
	Com_Memset(vm, 0, sizeof(*vm));

	currentVM = NULL;
	lastVM    = NULL;
}

/**
 * @brief VM_Clear
 */
void VM_Clear(void)
{
	int i;

	for (i = 0; i < MAX_VM; i++)
	{
		if (vmTable[i].dllHandle)
		{
			Sys_UnloadDll(vmTable[i].dllHandle);
		}
		Com_Memset(&vmTable[i], 0, sizeof(vm_t));
	}
	currentVM = NULL;
	lastVM    = NULL;
}

/**
 * @brief VM_ArgPtr
 * @param[in] intValue
 * @return
 */
void *VM_ArgPtr(intptr_t intValue)
{
	if (!intValue)
	{
		return NULL;
	}
	// bk001220 - currentVM is missing on reconnect
	if (currentVM == NULL)
	{
		return NULL;
	}

	if (currentVM->entryPoint)
	{
		return (void *)(currentVM->dataBase + intValue);
	}
	else
	{
		return (void *)(currentVM->dataBase + (intValue & currentVM->dataMask));
	}
}

/**
 * @brief VM_ExplicitArgPtr
 * @param[in] vm
 * @param[in] intValue
 * @return
 */
void *VM_ExplicitArgPtr(vm_t *vm, intptr_t intValue)
{
	if (!intValue)
	{
		return NULL;
	}

	// bk010124 - currentVM is missing on reconnect here as well?
	if (currentVM == NULL)
	{
		return NULL;
	}

	if (vm->entryPoint)
	{
		return (void *)(vm->dataBase + intValue);
	}
	else
	{
		return (void *)(vm->dataBase + (intValue & vm->dataMask));
	}
}

/**
 * @brief VM_CallFunc
 * @details Upon a system call, the stack will look like:
 *
 * sp+32   parm1
 * sp+28   parm0
 * sp+24   return value
 * sp+20   return address
 * sp+16   local1
 * sp+14   local0
 * sp+12   arg1
 * sp+8    arg0
 * sp+4    return stack
 * sp      return address
 *
 * An interpreted function will immediately execute an OP_ENTER instruction,
 * which will subtract space for locals from sp
 *
 * @param[in] vm
 * @param[in] callNum
 * @return
 */
intptr_t QDECL VM_CallFunc(vm_t *vm, int callNum, ...)
{
	vm_t     *oldVM;
	intptr_t r;

	if (!vm)
	{
		Com_Error(ERR_FATAL, "VM_Call: NULL vm");
	}

	oldVM     = currentVM;
	currentVM = vm;
	lastVM    = vm;

	if (vm_debugLevel)
	{
		Com_Printf("VM_Call( %i )\n", callNum);
	}

	// if we have a dll loaded, call it directly
	if (vm->entryPoint)
	{
		// rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
		intptr_t args[VM_SYSCALL_ARGS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		va_list  ap;
		size_t   i;

		va_start(ap, callNum);
		for (i = 0; i < ARRAY_LEN(args); i++)
		{
			// We add the end of args point since windows at least just returns random values if there are no args
			// this way we know that only valid values are sent to the vm
			args[i] = va_arg(ap, intptr_t);
			if (VM_CALL_END == (int) args[i])
			{
				args[i] = 0;
				break;
			}
		}
		va_end(ap);

		r = vm->entryPoint(callNum, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7],
		                   args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15]);
	}
#if 0
	else if (vm->compiled)
	{
		r = VM_CallCompiled(vm, &callnum);
	}
	else
	{
		r = VM_CallInterpreted(vm, &callnum);
	}
#else
	else
	{
		Com_Error(ERR_FATAL, "VM_Call: call without entrypoint");
	}
#endif

	if (oldVM != NULL)
	{
		currentVM = oldVM;
	}
	return r;
}

/**
 * @brief VM_ProfileSort
 * @param[in] a
 * @param[in] b
 * @return
 */
static int QDECL VM_ProfileSort(const void *a, const void *b)
{
	vmSymbol_t *sa, *sb;

	sa = *(vmSymbol_t **) a;
	sb = *(vmSymbol_t **) b;

	if (sa->profileCount < sb->profileCount)
	{
		return -1;
	}
	if (sa->profileCount > sb->profileCount)
	{
		return 1;
	}
	return 0;
}

/**
 * @brief VM_VmProfile_f
 */
void VM_VmProfile_f(void)
{
	vm_t       *vm;
	vmSymbol_t **sorted, *sym;
	int        i;
	double     total;

	if (!lastVM)
	{
		return;
	}

	vm = lastVM;

	if (!vm->numSymbols)
	{
		Com_Printf("VM symbols not available\n");
		return;
	}

	sorted    = Z_Malloc(vm->numSymbols * sizeof(*sorted));
	sorted[0] = vm->symbols;
	total     = sorted[0]->profileCount;
	for (i = 1; i < vm->numSymbols; i++)
	{
		sorted[i] = sorted[i - 1]->next;
		total    += sorted[i]->profileCount;
	}

	qsort(sorted, vm->numSymbols, sizeof(*sorted), VM_ProfileSort);

	for (i = 0; i < vm->numSymbols; i++)
	{
		int perc;

		sym = sorted[i];

		perc = 100 * (float)sym->profileCount / total;
		Com_Printf("%2i%% %9i %s\n", perc, sym->profileCount, sym->symName);
		sym->profileCount = 0;
	}

	Com_Printf("    %9.0f total\n", total);

	Z_Free(sorted);
}

/**
 * @brief VM_VmInfo_f
 */
void VM_VmInfo_f(void)
{
	vm_t *vm;
	int  i;

	Com_Printf("Registered virtual machines:\n");
	for (i = 0; i < MAX_VM; i++)
	{
		vm = &vmTable[i];
		if (!vm->name[0])
		{
			break;
		}
		Com_Printf("%s : ", vm->name);
		if (vm->dllHandle)
		{
			Com_Printf("native\n");
			continue;
		}
		if (vm->compiled)
		{
			Com_Printf("compiled on load\n");
		}
		else
		{
			Com_Printf("interpreted\n");
		}
		Com_Printf("    code length : %7i\n", vm->codeLength);
		Com_Printf("    table length: %7i\n", vm->instructionPointersLength);
		Com_Printf("    data length : %7i\n", vm->dataMask + 1);
	}
}

#ifdef DEBUG_VM
/**
 * @brief Insert calls to this while debugging the vm compiler
 * @param[in] args
 */
void VM_LogSyscalls(int *args)
{
	static int  callnum;
	static FILE *f;

	if (!f)
	{
		f = Sys_FOpen(FS_BuildOSPath(Cvar_VariableString("fs_homepath"), Cvar_VariableString("fs_game"), "syscalls.log"), "w");

		if (!f)
		{
			Com_Printf("Cannot open syscalls.log\n");
			return;
		}
	}
	callnum++;
	fprintf(f, "%i: %p (%i) = %i %i %i %i\n", callnum, (void *)(args - (int *)currentVM->dataBase),
	        args[0], args[1], args[2], args[3], args[4]);
}
#endif
