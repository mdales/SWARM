/*****************************************************************************
 * 
 * Copyright (C) 2001 C Hanish Menon [www.hanishkvc.com]
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
 * Name   : lcdctrl.h
 * Author : C Hanish Menon [www.hanishkvc.com]
 * 
 ****************************************************************************/

#ifndef __LCDCTRL_H__
#define __LCDCTRL_H__

#define LCDCTRL_NUMREGS 8
#define LCDCTRL_NUMPALS 256
#define LCDCTRL_SCREENFILE "/tmp/swarm_screen"
#define LCDCTRL_UPDATEINTERVAL 25

#define R_LCDVER        0x0
#define R_LCDRESOLUTION 0x1
#define R_LCDCOLORDEPTH 0x2
#define R_LCDSTARTADDR  0x3
#define R_LCDRESERVED1  0x4
#define R_LCDPALINDEX   0x5
#define R_LCDPALDATA    0x6
#define R_LCDSTATUS     0x7

typedef struct LCDCTRLBTAG
{
  uint32_t addr: 32;      // IN - address, 0 to 5
  uint32_t data: 32;      // IN/OUT - 32 bits of data
  uint32_t interrupt: 1;  // OUT - has an interrupt happened?
  uint32_t r:1;           // IN - is a read happening?
  uint32_t w:1;           // IN - is a write happening?
} LCDCTRLBUS;

class CLCDCtrl
{
  // Constuctors and destructor
 public:
  CLCDCtrl();
  ~CLCDCtrl();

 public:
  void Cycle(LCDCTRLBUS* bus);
  void Reset();

 private:
  uint32_t m_regs[LCDCTRL_NUMREGS];
  uint32_t m_pals[LCDCTRL_NUMPALS];
  FILE *fScreen;

  void UpdateScreen();
};

#endif // __LCDCTRL_H__
