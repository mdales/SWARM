//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (C) 2000 Michael Dales
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// Name   : intctrl.cpp
// Author : Michael Dales <michael@dcs.gla.ac.uk>
// 
//////////////////////////////////////////////////////////////////////////////

#include "swarm.h"
#include "intctrl.h"
#include <string.h>

#define R_ICIP 0x0
#define R_ICMR 0x1
#define R_ICLR 0x2
#define R_ICFP 0x3
#define R_ICPR 0x4
#define R_ICCR 0x5


///////////////////////////////////////////////////////////////////////////////
// CIntCtrl - 
//
CIntCtrl::CIntCtrl()
{
  Reset();
}


///////////////////////////////////////////////////////////////////////////////
// ~CIntCtrl - 
//
CIntCtrl::~CIntCtrl()
{
}


///////////////////////////////////////////////////////////////////////////////
// Reset -
//
void CIntCtrl::Reset()
{
  memset(m_regs, 0, sizeof(uint32_t) * 6);
}


///////////////////////////////////////////////////////////////////////////////
// Cycle - 
//
void CIntCtrl::Cycle(INTCTRLBUS* bus)
{
  int PendingInterrupts;

  // Nothing interesting happing...yet
  bus->fiq = bus->irq = bus->reset = 1;

  // See what the pesky real world wants
  if (bus->w != 0)
    {
      uint32_t addr = bus->addr;
      if (addr == 0x00000000)
	m_regs[R_ICIP] = bus->data;
      else if (addr == 0x00000004)
	m_regs[R_ICMR] = bus->data;
      else if (addr == 0x00000008)
	m_regs[R_ICLR] = bus->data;
      else if (addr == 0x0000000C)
	m_regs[R_ICCR] = bus->data & 0x1;
    }

  // Generate the new IRQ/FIQ pending registers
#ifndef QUIET
  if (bus->intbits)
  {
	  fprintf(stderr, "\nIntCtrl:\n Request: %x\n Mask: %x\n Level: %x\n", 
	    bus->intbits, m_regs[R_ICMR], m_regs[R_ICLR]);
  }
#endif  
  PendingInterrupts = m_regs[R_ICIP];
  m_regs[R_ICIP] = (bus->intbits & m_regs[R_ICMR]) & ~m_regs[R_ICLR];
  m_regs[R_ICFP] = (bus->intbits & m_regs[R_ICMR]) & m_regs[R_ICLR];

  if (m_regs[R_ICIP] != 0)
    {
      bus->irq = 0;
#ifndef QUIET
      fprintf(stderr,"\nIntCtrl: Got an IRQ\n");
#endif
    }
  if (m_regs[R_ICFP] != 0)
    {
      bus->fiq = 0;  
#ifndef QUIET
      fprintf(stderr,"\nIntCtrl: Got a FIQ\n");
#endif
    }
  m_regs[R_ICIP] |= PendingInterrupts;

  // Did the nosy busy body want any info?
  if (bus->r != 0)
    {
      uint32_t addr = bus->addr;

      if (addr == 0x00000000)
	bus->data = m_regs[R_ICIP];
      else if (addr == 0x00000004)
	bus->data = m_regs[R_ICMR];
      else if (addr == 0x00000008)
	bus->data = m_regs[R_ICLR];
      else if (addr == 0x00000010)
	bus->data = m_regs[R_ICFP];
      else if (addr == 0x00000020)
	bus->data = m_regs[R_ICPR];
      else if (addr == 0x0000000C)
	bus->data = m_regs[R_ICCR];
      else
	bus->data = 0;
    }
}
