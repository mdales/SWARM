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
// Name   : ostimer.cpp
// Author : Michael Dales <michael@dcs.gla.ac.uk>
// 
//////////////////////////////////////////////////////////////////////////////

#include "swarm.h"
#include "ostimer.h"
#include <string.h>

#define R_OSMR0 0x0
#define R_OSMR1 0x1
#define R_OSMR2 0x2
#define R_OSMR3 0x3
#define R_OSCR  0x4
#define R_OSSR  0x5
#define R_OWER  0x6
#define R_OIER  0x7

///////////////////////////////////////////////////////////////////////////////
// COSTimer - 
//
COSTimer::COSTimer()
{
  Reset();
}


///////////////////////////////////////////////////////////////////////////////
// ~COSTimer - 
//
COSTimer::~COSTimer()
{
}


///////////////////////////////////////////////////////////////////////////////
// Reset - 
//
void COSTimer::Reset()
{
  memset(m_regs, 0, sizeof(uint32_t) * 8);
}


///////////////////////////////////////////////////////////////////////////////
// Cycle - 
//
void COSTimer::Cycle(OSTBUS* bus)
{
  bus->interrupt = 0;
  bus->reset = 0;
  
  // Check to see if a write is happening
  if (bus->w != 0)
    {
      switch ((bus->addr) >> 2)
	{
	  // Write to a match register
	case 0: case 1: case 2: case 3:
	  m_regs[bus->addr] = bus->data;
	  break;
	  
	  // Write to the status register. 
	case 5:
	  for (int i = 0; i < 4; i++)
	    {
	      if ((bus->data >> i) & 0x1)
		m_regs[R_OSSR] &= ~(0x1 << i);
	    }
	  break;

	  // Write to the watchdog enable bit
	case 6:
	  m_regs[R_OWER] = bus->data & 0x00000001;
	  break;
	  
	  // Write to the interrupt enable register
	case 7:
	  m_regs[R_OIER] = bus->data & 0x0000000F;
	  break;
	}
    }

  // Increment the counter register
  m_regs[R_OSCR]++;

  // See if we should set of an interrupt
  if ((m_regs[R_OSMR0] == m_regs[R_OSCR]) && (m_regs[R_OIER] & 0x1))
    {
#ifndef QUIET	    
      fprintf(stderr, "\nTimerCtrl: Timer0  interrupt request for Match[%d]\n", m_regs[R_OSMR0]);
#endif      
      //bus->interrupt |= 0x1;
      m_regs[R_OSSR] |= 0x1;
    }
  if ((m_regs[R_OSMR1] == m_regs[R_OSCR]) && (m_regs[R_OIER] & 0x2))
    {
      //bus->interrupt |= 0x2;
      m_regs[R_OSSR] |= 0x2;
    }
  if ((m_regs[R_OSMR2] == m_regs[R_OSCR]) && (m_regs[R_OIER] & 0x4))
    {
      //bus->interrupt |= 0x4;
      m_regs[R_OSSR] |= 0x4;
    }
  if ((m_regs[R_OSMR3] == m_regs[R_OSCR]) && (m_regs[R_OIER] & 0x8))
    {
      //bus->interrupt |= 0x8;
      m_regs[R_OSSR] |= 0x8;
    }

  bus->interrupt = m_regs[R_OSSR] & m_regs[R_OIER];  

  // Should re do a reset?
  if ((m_regs[R_OSMR3] == m_regs[R_OSCR]) && m_regs[R_OWER])
    {
      bus->reset = 1;
    }

  // Return info to the nosy user if necessary
  if (bus->r != 0)
    {
      bus->data = m_regs[(bus->addr >> 2) & 0x00000007];
    }
}


///////////////////////////////////////////////////////////////////////////////
