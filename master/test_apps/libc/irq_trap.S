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
// name   irq_trap.S
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   
//
///////////////////////////////////////////////////////////////////////////////

#include <asm.h>

	.section .rdata
	
.LC0:	
	.ascii	"IRQ Handler invoked\n"

.text
///////////////////////////////////////////////////////////////////////////////
//
//
CENTRY(irq_trap)
	mov   r0, #2		@ Write to stderr
	ldr   r1, .L3		@ Error message	
	mov   r2, #20		@ Length of message
	bl    _write		@ Call write
	swi   0x00800000	@ Quit - weve toasted these registers 
.L3:
	.word	.LC0