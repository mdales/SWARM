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
 * Name   : intctrl.h
 * Author : Michael Dales <michael@dcs.gla.ac.uk>
 * 
 ****************************************************************************/

#ifndef __INTCTRL_H__
#define __INTCTRL_H__

typedef struct ICBTAG
{
  uint32_t intbits:32;
  uint32_t addr:32;
  uint32_t data:32;
  uint32_t irq:1;
  uint32_t fiq:1;
  uint32_t reset:1;
  uint32_t r:1;
  uint32_t w:1;
} INTCTRLBUS;

class CIntCtrl
{
  // constructors and destructor
 public:
  CIntCtrl();
  ~CIntCtrl();

 public:
  void Cycle(INTCTRLBUS* bus);
  void Reset();

 private:
  uint32_t m_regs[6];
};

#endif // __INTCTRL_H__
