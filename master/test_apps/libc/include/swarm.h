/*****************************************************************************
 * 
 * Copyright (C) 2000 Michael Dales
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * Name   : swarm.h
 * Author : Michael Dales <michael@dcs.gla.ac.uk>
 * 
 ****************************************************************************/

#ifndef __SWARM_H__
#define __SWARM_H__

void _dump();

#define _getcurpid() \
({ uint32_t __t; __asm__ volatile ("mrc 15, 0, %0, c12, c0, 0" : "=r" (__t) :); __t; })

#define _setcurpid(_x) \
({__asm__ volatile ("mcr 15, 0, %0, c12, c0, 0" : : "r" (_x));})

#endif /* __SWARM_H__ */
