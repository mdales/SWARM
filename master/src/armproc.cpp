///////////////////////////////////////////////////////////////////////////////
// Copyright 2000 Michael Dales
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// name   armproc.cpp
// author Michael Dales (michael@dcs.gla.ac.uk)
// header armproc.h
// info   An ARM processor - glues together the cache and what have you. 
//        Includes a system coprocessor in slot 15 by default.
//
///////////////////////////////////////////////////////////////////////////////

#include "swarm.h"
#include "armproc.h"
#include <string.h>
#include "cache.h"
#include "direct.h"
#include "associative.h"
#include "setassoc.h"
#include "copro.h"
#include "syscopro.h"

#define ICACHE_SIZE 1024
#define DCACHE_SIZE 1024

#ifndef NO_SYS_COPRO
#define MAX_COPRO_ID 14
#else
#define MAX_COPRO_ID 15
#endif


#define CACHE_TYPE CSetAssociativeCache
#define NEW_CACHE(_size)  new CACHE_TYPE(_size, 4)

// The number of times slower than the CPU the bus is
#define BUS_SPEED 10

///////////////////////////////////////////////////////////////////////////////
// CArmProc - Constructor
//
CArmProc::CArmProc()
{
  m_pCore = new CArmCore();
  m_pCoreBus = new COREBUS;
  memset(m_pCoreBus, 0, sizeof(COREBUS));
  m_pCoProBus = new COPROBUS;
  memset(m_pCoProBus, 0, sizeof(COPROBUS));
  m_addrPrev = 0;

  // I use the same cache for both the instruction and data currently
#ifdef SHARED_CACHE
  m_pICache = NEW_CACHE(ICACHE_SIZE + DCACHE_SIZE);
  m_pDCache = m_pICache;
#else
  m_pICache = NEW_CACHE(ICACHE_SIZE);
  m_pDCache = NEW_CACHE(DCACHE_SIZE);
#endif // SHARED_CACHE

  memset(m_pCoProList, 0, sizeof(CCoProcessor*) * 16);

#ifndef NO_SYS_COPRO
  m_pCoProList[15] = new CSysCoPro();
#endif // NO_SYS_COPRO

  ((CSysCoPro*)m_pCoProList[15])->RegisterCaches(m_pDCache, m_pICache);

  m_pOSTimer = new COSTimer();
  m_pIntCtrl = new CIntCtrl();
  m_pLCDCtrl = new CLCDCtrl();
  //m_pUARTCtrl = new CUARTCtrl();

  m_nCycles = 0;
  m_nCacheHits = 0;
  m_nCacheMisses = 0;
  m_mode = P_NORMAL;
  m_pending = 0;

  Reset();
}

CArmProc::CArmProc(uint32_t nCacheSize)
{
  m_pCore = new CArmCore();
  m_pCoreBus = new COREBUS;
  memset(m_pCoreBus, 0, sizeof(COREBUS));
  m_pCoProBus = new COPROBUS;
  memset(m_pCoProBus, 0, sizeof(COPROBUS));
  m_addrPrev = 0;

  m_pICache = NEW_CACHE(nCacheSize);
  m_pDCache = m_pICache;

  memset(m_pCoProList, 0, sizeof(CCoProcessor*) * 16);

#ifndef NO_SYS_COPRO
  m_pCoProList[15] = new CSysCoPro();
#endif // NO_SYS_COPRO

  ((CSysCoPro*)m_pCoProList[15])->RegisterCaches(m_pDCache, m_pICache);

  m_pOSTimer = new COSTimer();
  m_pIntCtrl = new CIntCtrl();
  m_pLCDCtrl = new CLCDCtrl();
  //m_pUARTCtrl = new CUARTCtrl();

  m_nCycles = 0;
  m_nCacheHits = 0;
  m_nCacheMisses = 0;
  m_mode = P_NORMAL;

  Reset();
}


///////////////////////////////////////////////////////////////////////////////
// ~CArmProc - Destructor
//
CArmProc::~CArmProc()
{
  cout << "Cache info: hits = " << m_nCacheHits << " misses = " <<
    m_nCacheMisses << "\n";

  // Clean up caches - check to see if they are the same.
  if (m_pICache == m_pDCache)
    {
      // Same - so only delete once
      delete m_pICache;
    }
  else
    {
      // Different - delete each one
      delete m_pICache;
      delete m_pDCache;
    }

  for (int i = 0; i < 16; i++)
    if (m_pCoProList[i] != NULL)
      delete m_pCoProList[i];

  delete m_pIntCtrl;
  delete m_pOSTimer;
  delete m_pLCDCtrl;
  //delete m_pUARTCtrl;

  delete m_pCore;
  delete m_pCoreBus;
  delete m_pCoProBus;
}


///////////////////////////////////////////////////////////////////////////////
// Reset 
//
void CArmProc::Reset()
{
  memset(&m_ostbus, 0, sizeof(OSTBUS));
  memset(&m_icbus, 0, sizeof(INTCTRLBUS));
  memset(&m_lcdctrlbus, 0, sizeof(LCDCTRLBUS));
  memset(&m_uartctrlbus, 0, sizeof(UARTCTRLBUS));
}


///////////////////////////////////////////////////////////////////////////////
//
//
#define CYCLE() AtomicCycle(pinout)
void CArmProc::AtomicCycle(PINOUT* pinout)
{
  uint32_t temp = m_pCoreBus->Din;

  // Cycle any on chip aids
  m_icbus.intbits = 0;

  m_pOSTimer->Cycle(&m_ostbus);
  if (m_ostbus.interrupt)
    m_icbus.intbits |= (m_ostbus.interrupt << 26);

  m_pLCDCtrl->Cycle(&m_lcdctrlbus);
  if (m_lcdctrlbus.interrupt)
    m_icbus.intbits |= (0x1 << 25);

  //m_pUARTCtrl->Cycle(&m_uartctrlbus);
  //if (m_uartctrlbus.interrupt)
  //  m_icbus.intbits |= (0x1 << 24);

  m_pIntCtrl->Cycle(&m_icbus);

  // Clear these for generation by the core next time round
  m_ostbus.r = m_ostbus.w = 0;
  m_icbus.r = m_icbus.w = 0;
  m_lcdctrlbus.r = m_lcdctrlbus.w = 0;
  m_uartctrlbus.r = m_uartctrlbus.w = 0;

  // Generate the interrupt bits
  m_pCoreBus->fiq = pinout->fiq && m_icbus.fiq;
  m_pCoreBus->irq = pinout->irq && m_icbus.irq;
  m_pCoProBus->fiq = pinout->fiq && m_icbus.fiq;
  m_pCoProBus->irq = pinout->irq && m_icbus.irq;

  if ((m_pCoProBus->dw == 1) && (m_pCoreBus->enout != 0))
    m_pCoreBus->Din = m_pCoProBus->Dout;

  if (m_pCoreBus->A & 0x80000000)
    {
      // Find out which internal device we're talking to
      if ((m_pCoreBus->A & 0xFFFF0000) == 0x90050000)
	{
	  // The interrupt controller 
	  m_pCoreBus->Din = m_icbus.data;
	}
      else if ((m_pCoreBus->A & 0xFFFF0000) == 0x90000000)
	{
	  // The OS Timer
	  m_pCoreBus->Din = m_ostbus.data;
	}
      else if ((m_pCoreBus->A & 0xFFFFF000) == 0x90081000)
	{
	  // The UART Controller
	  m_pCoreBus->Din = m_uartctrlbus.data;
	}
      else if ((m_pCoreBus->A & 0xFFF00000) == 0x90100000)
	{
	  // The LCD Controller
	  m_pCoreBus->Din = m_uartctrlbus.data;
	}
    }

  m_pCore->Cycle(m_pCoreBus);

  m_pCoProBus->opc = m_pCoreBus->opc;
  m_pCoProBus->cpi = m_pCoreBus->cpi;
  m_pCoProBus->cpa = m_pCoreBus->cpa;
  m_pCoProBus->cpb = 1;
  m_pCoProBus->dw = 0;
  if (m_pCoreBus->rw == 1)
    if (m_pCoreBus->opc == 1)
      m_pCoProBus->Din = temp;
    else
      m_pCoProBus->Din = m_pCoreBus->Dout;
  else
    m_pCoProBus->Din = m_pCoreBus->Din;

  for (int ii = 0; ii < 16; ii++)
    if (m_pCoProList[ii] != NULL)
      m_pCoProList[ii]->Cycle(m_pCoProBus);

  m_pCoreBus->cpa = m_pCoProBus->cpa;
  m_pCoreBus->cpb = m_pCoProBus->cpb;

  if ((m_pCoProBus->dw == 1) && (m_pCoreBus->enout == 0))
    m_pCoreBus->Dout = m_pCoProBus->Dout;
}

#define PENDING_FIQ   0x1
#define PENDING_IRQ   0x2
#define PENDING_RESET 0x4


///////////////////////////////////////////////////////////////////////////////
// Cycle - 
//
void CArmProc::Cycle(PINOUT* pinout)
{
  switch (m_mode)
    {
    case P_NORMAL:
      {
	uint32_t addr;

	if (m_pending & PENDING_FIQ)
	  {
	    pinout->fiq = 0;
	    m_pending &= ~PENDING_FIQ;
	  }
	if (m_pending & PENDING_IRQ)
	  {
	    pinout->irq = 0;
	    m_pending &= ~PENDING_IRQ;
	  }

	addr = m_pCoreBus->A & 0xFFFFFFFC;
	//printf("want address 0x%x -- ", addr);

	// See if it's a memory or internal read
	if ((addr & 0x80000000) == 0x00000000)
	  {
	    // Accessing memory

	    // Assume we are reading a value
	    ASSERT(m_pCoreBus->rw == 0);
	    CCache* pCache = m_pCoreBus->di ? m_pICache : m_pDCache;
	    try
	    {
	      m_nCacheHits++;
	      m_pCoreBus->Din = pCache->Read((addr >> 2));
	      //printf("got data 0x%x\n", m_pCoreBus->Din);
	    }
	    catch(CCacheMiss & e)
	    {
	      //printf("cache miss\n");

	      // We're not going to clock the core for a while as we
	      // read in the cache line, so we'd better be ready to note 
	      // interrupts.
	      if (pinout->fiq == 0)
		m_pending |= PENDING_FIQ;
	      if (pinout->irq == 0)
		m_pending |= PENDING_IRQ;

	      m_nCacheMisses++;
	      m_mode = P_READING1;
	      break;
	    }
	  }
	else
	  {
	    // Reading from a internal device

	    // See in Atomic cycle for the code here
	  }


	switch (m_pCoreBus->bw)
	  {
	  case 0:		// Read word
	    {
	      // Was the addess unaligned? If so do the rotate so that the
	      // index byte is in the lowest position.
	      uint32_t rot = m_pCoreBus->A & 0x00000003;
	      uint32_t ttemp = m_pCoreBus->Din >> (rot * 8);
	      m_pCoreBus->Din = ttemp | (m_pCoreBus->Din << ((4 - rot) * 8));
	    }
	    break;
	  case 1:		// Read byte
	    {
	      // Reading a byte, so mung the Din correctly
	      uint32_t nByte = m_pCoreBus->A & 0x00000003;
	      m_pCoreBus->Din = m_pCoreBus->Din >> (8 * nByte);
	      m_pCoreBus->Din &= 0x000000FF;

	      //printf("Read byte 0x%x @ 0x%x\n", m_pCoreBus->Din, m_pCoreBus->A);
	    }
	    break;
	  case 2:		// Read half word
	    {
	      // Check the alignment, if necessary rotate 16 bits
	      if ((m_pCoreBus->A & 0x00000002) == 0x00000002)
		{
		  m_pCoreBus->Din = (m_pCoreBus->Din >> 16);
		}
	      else
		{
		  m_pCoreBus->Din &= 0x0000FFFF;
		}
	    }
	    break;
	  }
#if 0
	if (m_pCoreBus->bw == 1)
	  {
	    // Reading a byte, so mung the Din correctly
	    uint32_t nByte = m_pCoreBus->A & 0x00000003;
	    m_pCoreBus->Din = m_pCoreBus->Din >> (8 * nByte);
	    m_pCoreBus->Din &= 0x000000FF;

	    //printf("Read byte 0x%x @ 0x%x\n", m_pCoreBus->Din, m_pCoreBus->A);
	  }
	else
	  {
	    // Was the addess unaligned? If so do the rotate so that the
	    // index byte is in the lowest position.
	    uint32_t rot = m_pCoreBus->A & 0x00000003;
	    uint32_t ttemp = m_pCoreBus->Din >> (rot * 8);
	    m_pCoreBus->Din = ttemp | (m_pCoreBus->Din << ((4 - rot) * 8));
	  }
#endif

	/////////////////////////////////////////////
	// *** Cycle the core and coprocessors *** //
	CYCLE();		//
	//                                         //
	/////////////////////////////////////////////

	if (m_pCoreBus->swi_hack == 1)
	  {
	    m_pICache->Reset();
	    if (m_pICache != m_pDCache)
	      m_pDCache->Reset();

	    m_pCoreBus->swi_hack = 0;
	  }

	// Find out how the next cycle is going to go
	if (m_pCoreBus->A & 0x80000000)
	  {
	    // Top bit of address set - means that we're reading/writing
	    // to stuff on the chip (e.g. interrupt controller)
	    if ((m_pCoreBus->rw == 1) && (m_pCoreBus->enout == 0))
	      {
		// We'll be writing, so make a note of this address for the
		// next cycle when we'll use it.
		m_addrPrev = m_pCoreBus->A;
		m_mode = P_INTWRITE;
	      }

	    if ((m_pCoreBus->rw == 0) && (m_pCoreBus->enout == 0))
	      {
		// Find out which internal device we're talking to
		if ((m_pCoreBus->A & 0xFFFF0000) == 0x90050000)
		  {
		    // The interrupt controller 
		    m_icbus.addr = m_pCoreBus->A & 0x0000FFFF;
		    m_icbus.r = 1;
		  }
		else if ((m_pCoreBus->A & 0xFFFF0000) == 0x90000000)
		  {
		    // The OS Timer
		    m_ostbus.addr = m_pCoreBus->A & 0x0000FFFF;
		    m_ostbus.r = 1;
		  }
		else if ((m_pCoreBus->A & 0xFFFFF000) == 0x90081000)
		  {
		    // The UART Controller
		    m_uartctrlbus.addr = m_pCoreBus->A & 0x00000FFF;
		    m_uartctrlbus.r = 1;
		  }
		else if ((m_pCoreBus->A & 0xFFF00000) == 0x90100000)
		  {
		    // The LCD Controller
		    m_lcdctrlbus.addr = m_pCoreBus->A & 0x000FFFFF;
		    m_lcdctrlbus.r = 1;
		  }
	      }
	  }
	else
	  {
	    // Normal talking to real world.        
	    if ((m_pCoreBus->rw == 1) && (m_pCoreBus->enout == 0))
	      {
		// We'll be writing, so make a note of this address for the
		// next cycle when we'll use it.
		m_addrPrev = m_pCoreBus->A;
		m_mode = P_WRITING1;
	      }
	    else if ((m_pCoreBus->rw == 0) && (m_pCoreBus->enout == 0))
	      {
		// We're reading. See if the address should be read from
		// the cache or from read memory.
	      }
	  }

	pinout->benable = 0;
      }
      break;
    case P_READING1:
      {
	// Reading in a cache line here.
	m_nRead = 0;

	if (pinout->fiq == 0)
	  m_pending |= PENDING_FIQ;
	if (pinout->irq == 0)
	  m_pending |= PENDING_IRQ;

	pinout->address = m_pCoreBus->A & 0xFFFFFFF0;
	pinout->rw = 0;
	pinout->benable = 1;
	m_mode = P_READING;

	// Add extra cycle for initiating a read
	m_nCycles += BUS_SPEED;
      }
      break;
    case P_READING:
      {
	CCache* pCache = m_pCoreBus->di ? m_pICache : m_pDCache;
	//printf("cache read 0x%x @ 0x%x\n", pinout->data, pinout->address);
	m_cacheLine[m_nRead] = pinout->data;
	//      pCache->Write((((m_pCoreBus->A & 0xFFFFFFF0) + (m_nRead * 4)) >> 2), 
	//                    pinout->data);

	m_nRead++;

	if (pinout->fiq == 0)
	  m_pending |= PENDING_FIQ;
	if (pinout->irq == 0)
	  m_pending |= PENDING_IRQ;

	if (m_nRead < CACHE_LINE)
	  {
	    // Set up to read the next word in the cache line
	    pinout->address = (m_pCoreBus->A & 0xFFFFFFF0) + (m_nRead * 4);
	    pinout->rw = 0;
	    pinout->benable = 1;
	  }
	else
	  {
	    // Write the full line into the cache
	    CCache* pCache = m_pCoreBus->di ? m_pICache : m_pDCache;
	    pCache->WriteLine(((m_pCoreBus->A & 0xFFFFFFF0) >> 2),
			      m_cacheLine);

	    // Read in a line, so go back to work
	    pinout->benable = 0;
	    m_mode = P_NORMAL;
	  }

	m_nCycles += BUS_SPEED;
      }
      break;
    case P_WRITING1:
      {
	pinout->bw = m_pCoreBus->bw;
	//m_pCore->Cycle(m_pCoreBus);
	CYCLE();

	// Set the bus up to do the write.
	pinout->benable = 1;
	pinout->address = m_addrPrev;
	pinout->data = m_pCoreBus->Dout;
	pinout->rw = 1;

	//printf("writing 0x%x @ 0x%x\n", pinout->data, pinout->address);

	// Add extra cycle for cost of write.
	m_nCycles += (BUS_SPEED * 2);

	// What are we to do next?
	if (m_pCoreBus->rw == 1)
	  {
	    // We'll be writing, so make a note of this address for the
	    // next cycle when we'll use it.
	    m_addrPrev = m_pCoreBus->A;
	    m_mode = P_WRITING1;
	  }
	else
	  {
	    m_mode = P_NORMAL;
	  }

	// Write thru the cache
	CCache* pCache = m_pCoreBus->di ? m_pICache : m_pDCache;
	//printf("cache write 0x%x @ 0x%x\n", pinout->data, pinout->address);
	try
	{
	  uint32_t temp;

	  // Is the data in the cache?
	  temp = pCache->Read(pinout->address >> 2);

	  // No exception thrown, so must be in memory - is it a word
	  // or a byte we're writing?
	  switch (pinout->bw)
	    {
	    case 0:		// Writing a word
	      {
		// Writing a word
		pCache->WriteWord((pinout->address >> 2), pinout->data);
	      }
	      break;
	    case 1:		// Write a byte
	      {
		uint32_t ttemp = temp;

		uint32_t mask = ~(0xFF << ((pinout->address & 0x3) * 8));
		temp &= mask;
		temp |= ((pinout->data << ((pinout->address & 0x3) * 8)) &
			 (~mask));
		//printf("Changing 0x%08x to 0x%08x (mask was 0x%08x)\n",
		//     ttemp, temp, mask);
		pCache->WriteWord((pinout->address >> 2), temp);
	      }
	      break;
	    case 2:		// Writing a half word
	      {
		uint32_t ttemp = temp;

		if ((pinout->address & 0x00000002) == 0)
		  {
		    // Modify low half
		    temp &= 0xFFFF0000;
		    temp |= (pinout->data & 0x0000FFFF);
		  }
		else
		  {
		    // Modify high half
		    temp &= 0x0000FFFF;
		    temp |= (pinout->data << 16);
		  }

		pCache->WriteWord((pinout->address >> 2), temp);
	      }
	      break;
	    }

#if 0
	  if (pinout->bw == 1)
	    {
	      uint32_t ttemp = temp;

	      uint32_t mask = ~(0xFF << ((pinout->address & 0x3) * 8));
	      temp &= mask;
	      temp |= ((pinout->data << ((pinout->address & 0x3) * 8)) &
		       (~mask));
	      //printf("Changing 0x%08x to 0x%08x (mask was 0x%08x)\n",
	      //     ttemp, temp, mask);
	      pCache->WriteWord((pinout->address >> 2), temp);
	    }
	  else
	    {
	      // Writing a word
	      pCache->WriteWord((pinout->address >> 2), pinout->data);
	    }
#endif
	}
	catch(CCacheMiss & c)
	{
	}
      }
      break;
    case P_INTWRITE:
      {
	// An internal write - writing to part of the address space inside
	// the coprocessor.
	CYCLE();

	// Not using real memory at any point
	pinout->benable = 0;

	// Find out which internal device we're talking to
	if ((m_addrPrev & 0xFFFF0000) == 0x90050000)
	  {
	    // The interrupt controller 
	    m_icbus.addr = m_addrPrev & 0x0000FFFF;
	    m_icbus.w = 1;
	    m_icbus.data = m_pCoreBus->Dout;
	  }
	else if ((m_addrPrev & 0xFFFF0000) == 0x90000000)
	  {
	    // The OS Timer
	    m_ostbus.addr = m_addrPrev & 0x0000FFFF;
	    m_ostbus.w = 1;
	    m_ostbus.data = m_pCoreBus->Dout;
	  }
	else if ((m_addrPrev & 0xFFFFF000) == 0x90081000)
	  {
	    // The UART Controller
	    m_uartctrlbus.addr = m_addrPrev & 0x00000FFF;
	    m_uartctrlbus.w = 1;
	    m_uartctrlbus.data = m_pCoreBus->Dout;
	  }
	else if ((m_addrPrev & 0xFFF00000) == 0x90100000)
	  {
	    // The LCD Controller
	    m_lcdctrlbus.addr = m_addrPrev & 0x000FFFFF;
	    m_lcdctrlbus.w = 1;
	    m_lcdctrlbus.data = m_pCoreBus->Dout;
	  }

	m_mode = P_NORMAL;
      }
      break;
    }

  m_nCycles++;
}


///////////////////////////////////////////////////////////////////////////////
// RegisterCoProcessor - 
//
void CArmProc::RegisterCoProcessor(uint32_t nID, CCoProcessor* pCoPro)
{
  if ((pCoPro == NULL) || (nID > MAX_COPRO_ID))
    throw CCoProInvalidException();
  if (m_pCoProList[nID] != NULL)
    throw CCoProSetException();

  m_pCoProList[nID] = pCoPro;
}


///////////////////////////////////////////////////////////////////////////////
// UnregisterCoProcessor - 
//
void CArmProc::UnregisterCoProcessor(uint32_t nID)
{
  if (nID > MAX_COPRO_ID)
    throw CCoProInvalidException();

  m_pCoProList[nID] = NULL;
}


///////////////////////////////////////////////////////////////////////////////
// DebugDump - Dump info for core and coprocessors.
//
void CArmProc::DebugDump()
{
  m_pCore->DebugDump();

  for (int i = 0; i < 16; i++)
    if (m_pCoProList[i] != NULL)
      m_pCoProList[i]->DebugDump();
}


///////////////////////////////////////////////////////////////////////////////
// DebugDumpCore - Dump info for core and coprocessors.
//
void CArmProc::DebugDumpCore()
{
  m_pCore->DebugDump();
}

///////////////////////////////////////////////////////////////////////////////
// DebugDumpCoProc - Dump info for core and coprocessors.
//
void CArmProc::DebugDumpCoProc()
{
  for (int i = 0; i < 16; i++)
    if (m_pCoProList[i] != NULL)
      m_pCoProList[i]->DebugDump();
}

///////////////////////////////////////////////////////////////////////////////
// NextPC - Gets the next PC value
//
long CArmProc::NextPC()
{
	return m_pCore->NextPC();
}

