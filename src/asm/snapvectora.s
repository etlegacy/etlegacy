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
 * @file snapvectora.s
 */


//
// Sys_SnapVector NASM code (Andrew Henderson)
// See win32/win_shared.c for the Win32 equivalent
// This code is provided to ensure that the
//  rounding behavior (and, if necessary, the
//  precision) of DLL and QVM code are identical
//  e.g. for network-visible operations.
// See ftol.nasm for operations on a single float,
//  as used in compiled VM and DLL code that does
//  not use this system trap.
//

// 23/09/05 Ported to gas by intel2gas, best supporting actor Tim Angus
// <tim@ngus.net>

#include "qasm.h"

#if id386
.data

fpucw:  .long   0
cw037F: .long   0x037F

.text

// void Sys_SnapVector( float *v )
.globl C(Sys_SnapVector)
C(Sys_SnapVector):
        pushl   %eax
        pushl   %ebp
        movl    %esp,%ebp

        fnstcw  fpucw
        movl    12(%ebp),%eax
        fldcw   cw037F
        flds    (%eax)
        fistpl  (%eax)
        fildl   (%eax)
        fstps   (%eax)
        flds    4(%eax)
        fistpl  4(%eax)
        fildl   4(%eax)
        fstps   4(%eax)
        flds    8(%eax)
        fistpl  8(%eax)
        fildl   8(%eax)
        fstps   8(%eax)
        fldcw   fpucw

        popl %ebp
        popl %eax
        ret

// void Sys_SnapVectorCW( float *v, unsigned short int cw )
.globl C(Sys_SnapVectorCW)
C(Sys_SnapVectorCW):
        pushl   %eax
        pushl   %ebp
        movl    %esp,%ebp

        fnstcw  fpucw
        movl    12(%ebp),%eax
        fldcw   16(%ebp)
        flds    (%eax)
        fistpl  (%eax)
        fildl   (%eax)
        fstps   (%eax)
        flds    4(%eax)
        fistpl  4(%eax)
        fildl   4(%eax)
        fstps   4(%eax)
        flds    8(%eax)
        fistpl  8(%eax)
        fildl   8(%eax)
        fstps   8(%eax)
        fldcw   fpucw

        popl %ebp
        popl %eax
        ret
#endif
