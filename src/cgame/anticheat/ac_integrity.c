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
 * @file ac_integrity.c
 * @brief Anti-cheat integrity verification implementation
 */

#include "ac_integrity.h"

// Global integrity state
acIntegrity_t acIntegrity;

// Checksum calculation for memory regions
static int AC_CalculateChecksum(void *ptr, int size)
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
 * @brief Initialize integrity checks by storing original function pointers
 */
void AC_InitIntegrityChecks(void)
{
    // Store original function pointers for later verification
    acIntegrity.vmMainPtr = (void *)cg_vmMain;
    acIntegrity.systemCallPtr = (void *)syscall;
    
    // Calculate initial VM code checksum (simplified - would need actual VM memory access)
    acIntegrity.vmChecksum = AC_CalculateChecksum(cg_vmMain, 1024); // Simplified
    
    acIntegrity.lastCheckedTime = cg.time;
    acIntegrity.initialized = qtrue;
    acIntegrity.status = AC_STATUS_OK;
    
    CG_Printf("^2Anti-cheat integrity checks initialized\n");
}

/**
 * @brief Check VM integrity by verifying function pointers haven't been modified
 */
acStatus_t AC_CheckVMIntegrity(void)
{
    int currentChecksum;
    
    if (!acIntegrity.initialized)
    {
        return AC_STATUS_UNKNOWN;
    }
    
    // Verify vmMain hasn't been modified
    if (acIntegrity.vmMainPtr != (void *)cg_vmMain)
    {
        CG_Printf("^1Anti-cheat warning: VM entry point modified\n");
        return AC_STATUS_VM_MODIFIED;
    }
    
    // Verify systemCall hasn't been modified
    if (acIntegrity.systemCallPtr != (void *)syscall)
    {
        CG_Printf("^1Anti-cheat warning: System call function modified\n");
        return AC_STATUS_VM_MODIFIED;
    }
    
    // Verify VM code hasn't been modified (simplified)
    currentChecksum = AC_CalculateChecksum(cg_vmMain, 1024); // Simplified
    if (currentChecksum != acIntegrity.vmChecksum)
    {
        CG_Printf("^1Anti-cheat warning: VM code modified\n");
        return AC_STATUS_VM_MODIFIED;
    }
    
    // In a real implementation, we would also check for duplicate VM structures
    // which would require access to the VM internals from the engine
    
    return AC_STATUS_OK;
}

/**
 * @brief Validate call stack to detect return address spoofing
 * Note: This is a simplified implementation as full call stack validation
 * would require engine-level modifications
 */
acStatus_t AC_ValidateCallStack(void)
{
    // In a real implementation, this would check the return addresses on the stack
    // to ensure they point to valid code regions within the game
    
    // Simplified implementation - in reality this would need to be implemented
    // at the engine level in CL_CgameSystemCalls() and VM_CallFunc()
    
    return AC_STATUS_OK;
}

/**
 * @brief Verify renderer functions haven't been hooked
 * Note: This is a simplified implementation as full renderer verification
 * would require engine-level modifications
 */
acStatus_t AC_VerifyRendererFunctions(void)
{
    // In a real implementation, this would verify the integrity of renderer function pointers
    // by comparing them with known-good values or checksums
    
    // Simplified implementation - in reality this would need to be implemented
    // at the engine level with access to the refexport_t structure
    
    return AC_STATUS_OK;
}

/**
 * @brief Verify OpenGL functions haven't been hooked
 * Note: This is a simplified implementation as full OpenGL verification
 * would require engine-level modifications
 */
acStatus_t AC_VerifyOpenGLFunctions(void)
{
    // In a real implementation, this would verify the integrity of OpenGL function pointers
    // particularly glReadPixels which is commonly hooked for screenshot cleaning
    
    // Simplified implementation - in reality this would need to be implemented
    // at the engine level with access to the OpenGL function table
    
    return AC_STATUS_OK;
}

/**
 * @brief Run all enabled detection methods
 * This should be called periodically during gameplay
 */
void AC_RunDetection(void)
{
    acStatus_t status = AC_STATUS_OK;
    
    if (!acIntegrity.initialized)
    {
        AC_InitIntegrityChecks();
        return;
    }
    
    // Only run checks every few seconds to minimize performance impact
    if (cg.time - acIntegrity.lastCheckedTime < 5000)
    {
        return;
    }
    
    acIntegrity.lastCheckedTime = cg.time;
    
    // Run all detection methods
    status = AC_CheckVMIntegrity();
    if (status != AC_STATUS_OK)
    {
        acIntegrity.status = status;
        return;
    }
    
    status = AC_ValidateCallStack();
    if (status != AC_STATUS_OK)
    {
        acIntegrity.status = status;
        return;
    }
    
    status = AC_VerifyRendererFunctions();
    if (status != AC_STATUS_OK)
    {
        acIntegrity.status = status;
        return;
    }
    
    status = AC_VerifyOpenGLFunctions();
    if (status != AC_STATUS_OK)
    {
        acIntegrity.status = status;
        return;
    }
    
    acIntegrity.status = AC_STATUS_OK;
}

/**
 * @brief Convert detection status to human-readable string
 */
const char *AC_StatusString(acStatus_t status)
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
