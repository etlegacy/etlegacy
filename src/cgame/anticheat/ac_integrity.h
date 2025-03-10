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
 * @file ac_integrity.h
 * @brief Anti-cheat integrity verification functions
 */

#ifndef INCLUDE_AC_INTEGRITY_H
#define INCLUDE_AC_INTEGRITY_H

#include "../cg_local.h"

// Detection flags
#define AC_DETECT_VM_INTEGRITY      (1 << 0)
#define AC_DETECT_CALLSTACK         (1 << 1)
#define AC_DETECT_RENDERER          (1 << 2)
#define AC_DETECT_OPENGL            (1 << 3)
#define AC_DETECT_ALL               (AC_DETECT_VM_INTEGRITY | AC_DETECT_CALLSTACK | AC_DETECT_RENDERER | AC_DETECT_OPENGL)

// Detection status
typedef enum {
    AC_STATUS_OK,
    AC_STATUS_VM_MODIFIED,
    AC_STATUS_VM_DUPLICATED,
    AC_STATUS_CALLSTACK_SPOOFED,
    AC_STATUS_RENDERER_HOOKED,
    AC_STATUS_OPENGL_HOOKED,
    AC_STATUS_UNKNOWN
} acStatus_t;

// VM integrity data
typedef struct {
    void *vmMainPtr;                // Original vmMain pointer
    void *systemCallPtr;            // Original systemCall pointer
    int vmChecksum;                 // Checksum of VM memory
    int lastCheckedTime;            // Last time integrity was checked
    qboolean initialized;           // Whether integrity checking is initialized
    acStatus_t status;              // Current detection status
} acIntegrity_t;

// Function prototypes
void AC_InitIntegrityChecks(void);
acStatus_t AC_CheckVMIntegrity(void);
acStatus_t AC_ValidateCallStack(void);
acStatus_t AC_VerifyRendererFunctions(void);
acStatus_t AC_VerifyOpenGLFunctions(void);
void AC_RunDetection(void);
const char *AC_StatusString(acStatus_t status);

extern acIntegrity_t acIntegrity;

#endif // INCLUDE_AC_INTEGRITY_H
