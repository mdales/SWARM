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
 * Name   : uartctrl.h
 * Author : C Hanish Menon [www.hanishkvc.com]
 * 
 ****************************************************************************/

#ifndef __UARTCTRL_H__
#define __UARTCTRL_H__

#define UARTCTRL_NUMREGS 8

#define R_UARTTXDATA     0x0
#define R_UARTRXDATA     0x1
#define R_UARTCONTROL    0x2
#define R_UARTSTATUS     0x3
#define R_UARTRESERVED1  0x4
#define R_UARTRESERVED2  0x5
#define R_UARTRESERVED3  0x6
#define R_UARTRESERVED4  0x7

#define UARTSTATUS_IN_DATA 0x1
#define UARTSTATUS_OUT_FREE 0x2

typedef struct UARTCTRLBTAG
{
  uint32_t addr: 32;      // IN - address, 0 to 7
  uint32_t data: 32;      // IN/OUT - 32 bits of data
  uint32_t interrupt: 1;  // OUT - has an interrupt happened?
  uint32_t r:1;           // IN - is a read happening?
  uint32_t w:1;           // IN - is a write happening?
} UARTCTRLBUS;

class CUARTCtrl
{
  // Constuctors and destructor
 public:
  CUARTCtrl();
  ~CUARTCtrl();

 public:
  void Cycle(UARTCTRLBUS* bus);
  void Reset();

 private:
  uint32_t m_regs[UARTCTRL_NUMREGS];
  int masterpty, slavepty;
  int bRxBufferFree;
	
  int GetPty();
};

#endif // __UARTCTRL_H__

