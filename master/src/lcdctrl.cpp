//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (C) 2001 C Hanish Menon [www.hanishkvc.com]
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
// Name   : lcdctrl.cpp
// Author : C Hanish Menon [www.hanishkvc.com]
// 
//////////////////////////////////////////////////////////////////////////////

#include "swarm.h"
#include "lcdctrl.h"
#include <string.h>
#include <stdlib.h>

#define MODULE_NAME "LCDCTRL"


///////////////////////////////////////////////////////////////////////////////
// CLCDCtrl - 
//
CLCDCtrl::CLCDCtrl()
{
  //printf(MODULE_NAME": In Constructor\n");
  if((fScreen = fopen(LCDCTRL_SCREENFILE,"w")) == NULL)
  {
    fprintf(stderr,MODULE_NAME": Error opening %s\n",LCDCTRL_SCREENFILE);
    exit(1);
  }
  Reset();
}


///////////////////////////////////////////////////////////////////////////////
// ~CLCDCtrl - 
//
CLCDCtrl::~CLCDCtrl()
{
  //printf(MODULE_NAME": In Destructor\n");
  fclose(fScreen);
}


///////////////////////////////////////////////////////////////////////////////
// Reset -
//
void CLCDCtrl::Reset()
{
  int r, g, b, colorinc;
  double dcolorinc;
  
  memset(m_regs, 0, sizeof(uint32_t) * LCDCTRL_NUMREGS);
  m_regs[R_LCDVER] = 0x00001000; // v0.1

  r = g = b = 0;
  dcolorinc = (double)0xff/LCDCTRL_NUMPALS;
  colorinc = (int) dcolorinc+1;
  for(int i = 0; i < LCDCTRL_NUMPALS; i++)
  {
    r = g = b = i*colorinc;      
    m_pals[i] = ( (r << 16) | (g << 8) | b );
  }
}


///////////////////////////////////////////////////////////////////////////////
// UpdateScreen - 
//
void CLCDCtrl::UpdateScreen()
{
  //printf(MODULE_NAME": Updating Screen\n");
  fseek(fScreen,0,SEEK_SET);
  for(int i = 0; i < LCDCTRL_NUMPALS; i++)
    fprintf(fScreen, "Pal[%x] = %x\n", i, m_pals[i]);
  fflush(fScreen);
}

///////////////////////////////////////////////////////////////////////////////
// Cycle - 
//
void CLCDCtrl::Cycle(LCDCTRLBUS* bus)
{
  static int CycleCount = 0;
  

  if( (CycleCount++ % LCDCTRL_UPDATEINTERVAL) == 0)
          UpdateScreen();
  // See what the pesky real world wants
  if (bus->w != 0)
  {
    uint32_t addr = bus->addr;
    
    // R_LCDVER        0x0
    if (addr == 0x00000004) // R_LCDRESOLUTION 0x1
      m_regs[R_LCDRESOLUTION] = bus->data;
    else if (addr == 0x00000008) // R_LCDCOLORDEPTH 0x2
      m_regs[R_LCDCOLORDEPTH] = bus->data;
    else if (addr == 0x0000000C) // R_LCDSTARTADDR  0x3
      m_regs[R_LCDSTARTADDR] = bus->data;
    // R_LCDRESERVED1  0x4
    // R_LCDPALINDEX   0x5
    // R_LCDPALDATA    0x6
    // R_LCDSTATUS     0x7
  }


  // Did the nosy busy body want any info?
  if (bus->r != 0)
  {
    uint32_t addr = bus->addr;

    if (addr == 0x00000000)
	    bus->data = m_regs[R_LCDVER];
    else if (addr == 0x00000004)
	    bus->data = m_regs[R_LCDRESOLUTION];
    else if (addr == 0x00000008)
	    bus->data = m_regs[R_LCDCOLORDEPTH];
    else if (addr == 0x0000000C)
	    bus->data = m_regs[R_LCDSTARTADDR];
    else
	    bus->data = 0;
  }
}
