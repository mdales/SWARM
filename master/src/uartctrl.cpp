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
// Name   : uartctrl.cpp
// Author : C Hanish Menon [www.hanishkvc.com]
// 
//////////////////////////////////////////////////////////////////////////////

#include "swarm.h"
#include "uartctrl.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#define __USE_GNU
#define __USE_XOPEN
#include <stdlib.h>

#define MODULE_NAME "UARTCTRL"

///////////////////////////////////////////////////////////////////////////////
// ~CUARTCtrl - 
//
int CUARTCtrl::GetPty()
{
	char *namepty;
	struct termios tiopty;
	int curFlags;
	
	if((masterpty = getpt()) == 0)
	{
		printf(MODULE_NAME": Error getting the pty\n");
		return -1;
	}
	if( grantpt(masterpty) < 0 || unlockpt(masterpty) < 0 )
	{
		printf(MODULE_NAME": Error Setting up pty\n");
		return -1;
	}
	namepty = ptsname(masterpty);
	if( namepty == NULL )
	{
		printf(MODULE_NAME": Error getting slave pty name\n");
		return -1;
	}
	else
	{
		if( tcgetattr(masterpty, &tiopty) < 0 )
		{
			printf(MODULE_NAME": Error getting masterpty terminfo\n");
			return -1;
		}
		tiopty.c_lflag &= ~(ECHO|ICANON);
		tiopty.c_cc[VMIN] = 0;
		tiopty.c_cc[VTIME] = 0;
		if( tcsetattr(masterpty, TCSANOW, &tiopty) < 0 )
		{
			printf(MODULE_NAME": Error setting masterpty terminfo\n");
			return -1;
		}
		if( tcgetattr(masterpty, &tiopty) < 0 )
		{
			printf(MODULE_NAME": Error getting masterpty terminfo\n");
			return -1;
		}
		//printf(MODULE_NAME": Canonical mode = [%c], tiopty.MIN = [%x], tiopty.TIME = [%x]\n", 
		//								(tiopty.c_lflag & ICANON) ? 'Y' : 'N',
		//								tiopty.c_cc[VMIN], tiopty.c_cc[VTIME]);
		// Forced to set O_NONBLOCK mode has Non Canonical mode of terminal
		// and 0 MIN and 0 TIME didn't help make read non blocking - HanishKVC
		if( (curFlags = fcntl(masterpty, F_GETFL, 0)) < 0 )
		{
			printf(MODULE_NAME": Error getting masterpty Flags\n");
			return -1;
		}
		curFlags |= O_NONBLOCK;
		if( (curFlags = fcntl(masterpty, F_SETFL, curFlags)) < 0 )
		{
			printf(MODULE_NAME": Error getting masterpty Flags\n");
			return -1;
		}
		
		printf(MODULE_NAME": Serial device Slave pts [%s]\n", namepty);
		printf(MODULE_NAME": Connect to Slave pts if required, and press ENTER key\n");
		getchar();
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// CUARTCtrl - 
//
CUARTCtrl::CUARTCtrl()
{
#ifndef QUIET	
	printf(MODULE_NAME": In Constructor\n");
#endif			
	if(GetPty() < 0)
		_exit(-1);
	bRxBufferFree = 1;
	Reset();
}


///////////////////////////////////////////////////////////////////////////////
// ~CUARTCtrl - 
//
CUARTCtrl::~CUARTCtrl()
{
#ifndef QUIET	
	printf(MODULE_NAME": In Destructor\n");
#endif			
	close(masterpty);
}


///////////////////////////////////////////////////////////////////////////////
// Reset -
//
void CUARTCtrl::Reset()
{
  
	memset(m_regs, 0, sizeof(uint32_t) * UARTCTRL_NUMREGS);
	m_regs[R_UARTSTATUS] = UARTSTATUS_OUT_FREE; // Ready to output char

}


///////////////////////////////////////////////////////////////////////////////
// Cycle - 
//
void CUARTCtrl::Cycle(UARTCTRLBUS* bus)
{
	char cCur;		

	if(bRxBufferFree)
	{
#ifndef QUIET		
		fprintf(stderr,MODULE_NAME": Checking to READ\n");
#endif
		if( read(masterpty,&cCur,1) > 0 )
		{
			m_regs[R_UARTSTATUS] |= UARTSTATUS_IN_DATA;
			m_regs[R_UARTRXDATA] = cCur;
#ifndef QUIET		
			fprintf(stderr,MODULE_NAME": Just READ [%c]\n",cCur);
#endif			
			bRxBufferFree = 0;	
		}
		else
			m_regs[R_UARTSTATUS] &= ~UARTSTATUS_IN_DATA;
	}	

	// See what the pesky real world wants
	if (bus->w != 0)
	{
		uint32_t addr = bus->addr;
#ifndef QUIET		
		fprintf(stderr,MODULE_NAME": Writing to [%x]\n",addr);
#endif			
		if (addr == 0x0) // R_UARTTXDATA
		{
			cCur = bus->data;
			write(masterpty,&cCur,1);
#ifndef QUIET		
			fprintf(stderr,MODULE_NAME": Just WROTE [%c]\n",cCur);
#endif			
		}
		else if (addr == 0x8) // R_UARTCONTROL
			m_regs[R_UARTCONTROL] = bus->data;
	}

	// Did the nosy busy body want any info?
	if (bus->r != 0)
	{
		uint32_t addr = bus->addr;
		
#ifndef QUIET		
		fprintf(stderr,MODULE_NAME": Reading from [%x]\n",addr);
#endif			
		if (addr == 0x4) // R_UARTRXDATA
		{
			bus->data = m_regs[R_UARTRXDATA];
			bRxBufferFree = 1;
		}
		else if (addr == 0x8) // R_UARTCONTROL
			bus->data = 0;
		else if (addr == 0xC) // R_UARTSTATUS
			bus->data = m_regs[R_UARTSTATUS] | UARTSTATUS_OUT_FREE;
		else // All other registers
			bus->data = 0;
	}
}

