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
 * Name   : ostimer.h
 * Author : Michael Dales <michael@dcs.gla.ac.uk>
 * 
 ****************************************************************************/

#ifndef __OSTIMER_H__
#define __OSTIMER_H__

typedef struct OSTBTAG
{
  uint32_t addr: 32;      // IN - address, 0 to 7
  uint32_t data: 32;      // IN/OUT - 32 bits of data
  uint32_t interrupt: 4;  // OUT - has an interrupt happened?
  uint32_t reset: 1;      // OUT - has the watchdog happened?
  uint32_t r:1;           // IN - is a read happening?
  uint32_t w:1;           // IN - is a write happening?
} OSTBUS;

class COSTimer
{
  // Constuctors and destructor
 public:
  COSTimer();
  ~COSTimer();

 public:
  void Cycle(OSTBUS* bus);
  void Reset();

 private:
  uint32_t m_regs[8];
};

#endif // __OSTIMER_H__
