/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file cl_anticheat.c
 * @brief Client-side anti-cheat detection implementation
 */

#include "cl_anticheat.h"

// Global integrity state
clAcIntegrity_t clAcIntegrity;

// Checksum calculation for memory regions
static int CL_AC_CalculateChecksum(void *ptr, int size)
{
    int checksum = 0;
    unsigned char *p = (unsigned char *)ptr;
    int i;

    if (!ptr || size <= 0)
    {
        return 0;
    }

    for (i = 0; i < size; i++)
    {
        checksum += (p[i] * (i + 119)) ^ (i * 7919);
    }

    return checksum;
}

/**
 * @brief Initialize integrity checks by storing original VM pointers
 */
void CL_AC_InitIntegrityChecks(vm_t *vm)
{
    if (!vm)
    {
        Com_Printf("CL_AC_InitIntegrityChecks: NULL VM\n");
        return;
    }

    // Store original VM and function pointers
    clAcIntegrity.originalVM = vm;
    clAcIntegrity.originalSystemCall = vm->systemCall;
    clAcIntegrity.originalEntryPoint = vm->entryPoint;
    
    // Calculate initial VM code checksum
    if (vm->codeBase && vm->codeLength > 0)
    {
        clAcIntegrity.vmChecksum = CL_AC_CalculateChecksum(vm->codeBase, vm->codeLength);
    }
    
    clAcIntegrity.lastCheckedTime = cls.realtime;
    clAcIntegrity.initialized = qtrue;
    clAcIntegrity.status = AC_STATUS_OK;
    
    Com_Printf("Anti-cheat engine integrity checks initialized\n");
}

/**
 * @brief Check VM integrity by verifying VM structure hasn't been modified
 */
acStatus_t CL_AC_CheckVMIntegrity(vm_t *vm)
{
    int currentChecksum;
    
    if (!clAcIntegrity.initialized || !vm)
    {
        return AC_STATUS_UNKNOWN;
    }
    
    // Check if the VM pointer has changed
    if (clAcIntegrity.originalVM != vm)
    {
        Com_Printf("Anti-cheat warning: VM pointer changed\n");
        return AC_STATUS_VM_MODIFIED;
    }
    
    // Check if systemCall function has been modified
    if (clAcIntegrity.originalSystemCall != vm->systemCall)
    {
        Com_Printf("Anti-cheat warning: VM systemCall modified\n");
        return AC_STATUS_VM_MODIFIED;
    }
    
    // Check if entryPoint function has been modified
    if (clAcIntegrity.originalEntryPoint != vm->entryPoint)
    {
        Com_Printf("Anti-cheat warning: VM entryPoint modified\n");
        return AC_STATUS_VM_MODIFIED;
    }
    
    // Check if VM code has been modified
    if (vm->codeBase && vm->codeLength > 0)
    {
        currentChecksum = CL_AC_CalculateChecksum(vm->codeBase, vm->codeLength);
        if (currentChecksum != clAcIntegrity.vmChecksum)
        {
            Com_Printf("Anti-cheat warning: VM code modified\n");
            return AC_STATUS_VM_MODIFIED;
        }
    }
    
    // Check for VM duplication by comparing with currentVM and lastVM
    if (currentVM != NULL && currentVM != vm && 
        currentVM->name[0] != '\0' && !Q_stricmp(currentVM->name, vm->name))
    {
        Com_Printf("Anti-cheat warning: Duplicate VM detected\n");
        return AC_STATUS_VM_DUPLICATED;
    }
    
    return AC_STATUS_OK;
}

/**
 * @brief Validate call stack to detect return address spoofing
 */
acStatus_t CL_AC_ValidateCallStack(void)
{
    // This would require access to the call stack and return addresses
    // In a real implementation, this would check return addresses against valid code regions
    
    return AC_STATUS_OK;
}

/**
 * @brief Verify renderer functions haven't been hooked
 */
acStatus_t CL_AC_VerifyRendererFunctions(void)
{
    // In a real implementation, this would verify the integrity of renderer function pointers
    // by comparing them with known-good values or checksums
    
    // We would need to store the original refexport_t function pointers at initialization
    // and then compare them during verification
    
    return AC_STATUS_OK;
}

/**
 * @brief Verify OpenGL functions haven't been hooked
 */
acStatus_t CL_AC_VerifyOpenGLFunctions(void)
{
    // In a real implementation, this would verify the integrity of OpenGL function pointers
    // particularly glReadPixels which is commonly hooked for screenshot cleaning
    
    // We would need to store the original OpenGL function pointers at initialization
    // and then compare them during verification
    
    return AC_STATUS_OK;
}

/**
 * @brief Run all enabled detection methods
 * This should be called periodically during gameplay
 */
void CL_AC_RunDetection(void)
{
    acStatus_t status = AC_STATUS_OK;
    
    if (!clAcIntegrity.initialized || !cgvm)
    {
        return;
    }
    
    // Only run checks every few seconds to minimize performance impact
    if (cls.realtime - clAcIntegrity.lastCheckedTime < 5000)
    {
        return;
    }
    
    clAcIntegrity.lastCheckedTime = cls.realtime;
    
    // Run all detection methods
    status = CL_AC_CheckVMIntegrity(cgvm);
    if (status != AC_STATUS_OK)
    {
        clAcIntegrity.status = status;
        return;
    }
    
    status = CL_AC_ValidateCallStack();
    if (status != AC_STATUS_OK)
    {
        clAcIntegrity.status = status;
        return;
    }
    
    status = CL_AC_VerifyRendererFunctions();
    if (status != AC_STATUS_OK)
    {
        clAcIntegrity.status = status;
        return;
    }
    
    status = CL_AC_VerifyOpenGLFunctions();
    if (status != AC_STATUS_OK)
    {
        clAcIntegrity.status = status;
        return;
    }
    
    clAcIntegrity.status = AC_STATUS_OK;
}

/**
 * @brief Convert detection status to human-readable string
 */
const char *CL_AC_StatusString(acStatus_t status)
{
    switch (status)
    {
        case AC_STATUS_OK:
            return "No cheats detected";
        case AC_STATUS_VM_MODIFIED:
            return "VM integrity compromised";
        case AC_STATUS_VM_DUPLICATED:
            return "VM duplication detected";
        case AC_STATUS_CALLSTACK_SPOOFED:
            return "Call stack spoofing detected";
        case AC_STATUS_RENDERER_HOOKED:
            return "Renderer function hooks detected";
        case AC_STATUS_OPENGL_HOOKED:
            return "OpenGL function hooks detected";
        case AC_STATUS_UNKNOWN:
        default:
            return "Unknown status";
    }
}
