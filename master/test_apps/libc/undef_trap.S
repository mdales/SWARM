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
// name   undef_trap.S
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   Called on an undefined instruction trap. Basically reports an
//	  error and then halts the simulator.
//
///////////////////////////////////////////////////////////////////////////////

#include <asm.h>

	.section .rdata
	
.LC0:	
	.ascii	"Error:	undefined instruction trap\n\000"

.text
///////////////////////////////////////////////////////////////////////////////
//
//
CENTRY(undef_trap)
	mov r0, #2		@ Write to stderr
	ldr r1, .L3		@ Error message
	mov r2, #35		@ Length of message
	bl  CCALL(write)	@ Call write
	swi 0x00800000		@ Abort it all
.L3:
	.word	.LC0