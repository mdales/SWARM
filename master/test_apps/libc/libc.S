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
	
//.bss
//___errno:
//	.space  4
	
.text
///////////////////////////////////////////////////////////////////////////////
// write
//
CENTRY(write)
	swi 0x00800001		@ call the write user function
	mov pc, lr		@ Return	
	
///////////////////////////////////////////////////////////////////////////////
// read
//
CENTRY(read)
	swi 0x00800002		@ call the read user function
	mov pc, lr		@ Return	
	
///////////////////////////////////////////////////////////////////////////////
// open
//
CENTRY(open)
	swi 0x00800003		@ call the open user function
	mov pc, lr		@ Return	
	
///////////////////////////////////////////////////////////////////////////////
// creat
//
CENTRY(creat)
	swi 0x00800004		@ call the creat user function
	mov pc, lr		@ Return	
	
///////////////////////////////////////////////////////////////////////////////
// close
//
CENTRY(close)
	swi 0x00800005		@ call the close user function
	mov pc, lr		@ Return

///////////////////////////////////////////////////////////////////////////////
// fcntl
//
CENTRY(fcntl)
	swi 0x00800006		@ call the fcntl user function
	mov pc, lr		@ Return

///////////////////////////////////////////////////////////////////////////////
// lseek
//
CENTRY(lseek)
	swi 0x00800007		@ call the lseek user function
	mov pc, lr		@ Return

///////////////////////////////////////////////////////////////////////////////
// stat
//
CENTRY(stat)
	swi 0x00800008		@ call the stat user function
	mov pc, lr		@ Return

///////////////////////////////////////////////////////////////////////////////
// fstat
//
CENTRY(fstat)
	swi 0x00800009		@ call the fstat user function
	mov pc, lr		@ Return

///////////////////////////////////////////////////////////////////////////////
// lstat
//
CENTRY(lstat)
	swi 0x0080000A		@ call the lstat user function
	mov pc, lr		@ Return

///////////////////////////////////////////////////////////////////////////////
// exit - should do more house cleaning, but doesn't.
//
CENTRY(exit)
	swi 0x00800000		@ Quits swarm

///////////////////////////////////////////////////////////////////////////////
// _exit
//
CENTRY(_exit)
	swi 0x00800000		@ Quits swarm

