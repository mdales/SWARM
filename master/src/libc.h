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
// name   libc.cpp
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   Provides libc code for applications on the emulator
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __LIBC_H__
#define __LIBC_H__

#include "swi.h"

#define SWI_LIBC_MASK  0x00000000

extern SWI_CALL swi_libc_write;
#define SWI_LIBC_WRITE   SWI_CALL_MASK | SWI_LIBC_MASK | 1
extern SWI_CALL swi_libc_read;
#define SWI_LIBC_READ   SWI_CALL_MASK | SWI_LIBC_MASK | 2
extern SWI_CALL swi_libc_open;
#define SWI_LIBC_OPEN   SWI_CALL_MASK | SWI_LIBC_MASK | 3
extern SWI_CALL swi_libc_creat;
#define SWI_LIBC_CREAT   SWI_CALL_MASK | SWI_LIBC_MASK | 4
extern SWI_CALL swi_libc_close;
#define SWI_LIBC_CLOSE   SWI_CALL_MASK | SWI_LIBC_MASK | 5
extern SWI_CALL swi_libc_fcntl;
#define SWI_LIBC_FCNTL   SWI_CALL_MASK | SWI_LIBC_MASK | 6
extern SWI_CALL swi_libc_lseek;
#define SWI_LIBC_LSEEK   SWI_CALL_MASK | SWI_LIBC_MASK | 7
extern SWI_CALL swi_libc_stat;
#define SWI_LIBC_STAT    SWI_CALL_MASK | SWI_LIBC_MASK | 8
extern SWI_CALL swi_libc_fstat;
#define SWI_LIBC_FSTAT   SWI_CALL_MASK | SWI_LIBC_MASK | 9
extern SWI_CALL swi_libc_lstat;
#define SWI_LIBC_LSTAT   SWI_CALL_MASK | SWI_LIBC_MASK | 10

#endif //__LIBC_H__
