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
// name   vector.S
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   
//
///////////////////////////////////////////////////////////////////////////////

#include <asm.h>
	
	.section .rdata
	
.LC0:	
	.ascii	"FIQ Handler invoked\n"

.text
///////////////////////////////////////////////////////////////////////////////
//
//
CENTRY(vector)
	b	CCALL(start)	@ Reset vector
	b	CCALL(undef_trap)	@ Undefined instruction vector
	b	CCALL(undef_trap)	@ SWI vector
	b	CCALL(undef_trap)	@ Prefect abort vector
	b	CCALL(undef_trap)	@ Data abort vector
	b       CCALL(undef_trap)     @ *** Gap in table ***
	b	CCALL(undef_trap)	@ IRQ vector

	// Now we can start the FIQ handler here
	mov     r8, r0		@ Back up user mode registers
	mov     r9, r1		@
	mov     r10, r2		@
	mov     r0, #2		@ Write to stderr
	ldr     r1, .L3		@ Error message	
	mov     r2, #20		@ Length of message
	swi     0x00800001	@ Call write
	mov     r0, r8		@ Restore registers for user
	mov     r1, r9		@
	mov	r2, r10		@
	subs    pc, lr, #4	@ Return to userville 
	
.L3:
	.word	.LC0
        