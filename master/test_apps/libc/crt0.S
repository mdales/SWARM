///////////////////////////////////////////////////////////////////////////////
// Copyright 2000 Michael Dales
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// name   libgcc.S
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   Implements the basic routines needed by a gcc program.
//
///////////////////////////////////////////////////////////////////////////////

#include <asm.h>


///////////////////////////////////////////////////////////////////////////////
// _start - This is where the program starts. We need to set the stack (which
//          will grow down, initialise libc, invoke main, and then exit the 
//          simulator. Note that the heap is set to be just below the stack.
//
//
.global _start			; _start:
CENTRY(start)
	
	mov   sp, #524288	@ Setup the stack...
	sub   sp, sp, #4	@ ...just below the heap
	sub   fp, sp, #4	@ And the frame pointer
	mov   r0, #524288	@ Offset of heap
        mov   r1, #8388608	@ Length of heap
	//bl    libc_mem_init	@ Initialise memory routines
	//bl    libc_stdio_init	@ Initialise stdio
	mov   r1, sp		@ Backup stack pointer	
	mrs   r0, cpsr		@ Get the CPSR
	bic   r0, r0, #0x1F	@ Remove current mode
	orr   r0, r0, #0x10	@ Go to user mode
	msr   cpsr_c, r0	@ Set the mode	
	mov   sp, r1		@ Restore the stack pointer
	swi   0x0080000E	@ get a pointer to the args in r0
	mov   r1, r0		@ move to argv (note address ahead)
	ldr   r0, [r1], #4	@ load argc, and set correct argv
	bl    main		@ Now in user land - so go do stuff
	swi   0x00800000	@ End it all now...
	
