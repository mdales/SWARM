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
 * Name   : profile.h
 * Author : Michael Dales <michael@dcs.gla.ac.uk>
 * 
 ****************************************************************************/

#ifndef __PROFILE_H__
#define __PROFILE_H__

/* Read the 32 bit processor cycle counter */
#define rpcc() \
({ uint32_t __t; __asm__ volatile ("mrc 15, 0, %0, c11, c0, 0" : "=r" (__t) :); __t; })

/* Read the 32 bit processor cache hit counter */
#define rpch() \
({ uint32_t __t; __asm__ volatile ("mrc 15, 0, %0, c11, c0, 1" : "=r" (__t) :); __t; })

/* Read the 32 bit processor cache miss counter */
#define rpcm() \
({ uint32_t __t; __asm__ volatile ("mrc 15, 0, %0, c11, c0, 2" : "=r" (__t) :); __t; })

#endif /* __PROFILE_H__ */
