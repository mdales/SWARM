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
// name   armproc.h
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   An ARM processor - glues together the cache and what have you.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __ARMPROC_H__
#define __ARMPROC_H__

#include "swarm.h"
#include "core.h"
#include "cache.h"
#include "swi.h"
#include <iostream.h>
#include "copro.h"

#include "ostimer.h"
#include "intctrl.h"
#include "lcdctrl.h"
#include "uartctrl.h"

enum PPROC {P_NORMAL, P_READING1, P_READING, P_WRITING1, P_INTWRITE};

typedef struct POTAG
{
  uint32_t nreset  : 1;
  uint32_t address : 32;
  uint32_t data    : 32;
  uint32_t fiq     : 1;
  uint32_t irq     : 1;
  uint32_t rw      : 1;   // Write = 1
  uint32_t benable : 1;   // Bus Active?
  uint32_t bw      : 2;   // 0 = word, 1 = byte, 2 = half word, 3 = UNDEF
} PINOUT;

#define CACHE_LINE 4

class CArmProc
{
  // constructors and destructor
 public:
  CArmProc();
  CArmProc(uint32_t nCacheSize);
  ~CArmProc();

  // Public methods
 public:
  void Cycle(PINOUT* pinout);  
  void Reset();
  inline uint64_t GetRealCycles() { return m_nCycles; }
  inline uint64_t GetLogicalCycles() { return m_pCore->GetCycles();} 

  inline void RegisterSWI(uint32_t nSwi, SWI_CALL* pSwi)
    { m_pCore->RegisterSWI(nSwi, pSwi); }
  inline void UnregisterSWI(uint32_t nSwi) 
    { m_pCore->UnregisterSWI(nSwi); }

  void RegisterCoProcessor(uint32_t nID, CCoProcessor* pCoPro);
  void UnregisterCoProcessor(uint32_t nID);

  void DebugDump();
  void DebugDumpCore();
  void DebugDumpCoProc();
  long NextPC();

 private:
  void AtomicCycle(PINOUT* pinout);

  // Member variables
 private:
  CArmCore* m_pCore;
  CCache*   m_pICache;
  CCache*   m_pDCache;
  COREBUS*  m_pCoreBus;
  COPROBUS* m_pCoProBus;

  COSTimer* m_pOSTimer;
  CIntCtrl* m_pIntCtrl;
  CLCDCtrl* m_pLCDCtrl;
  CUARTCtrl* m_pUARTCtrl;

  OSTBUS     m_ostbus;
  INTCTRLBUS m_icbus;
  LCDCTRLBUS m_lcdctrlbus;
  UARTCTRLBUS m_uartctrlbus;

  // Used for storing between cycles
  uint32_t   m_addrPrev;
  enum PPROC m_mode;
  uint32_t   m_nRead;

  uint64_t   m_nCycles;
  uint32_t   m_cacheLine[CACHE_LINE];
  uint64_t   m_nCacheHits;
  uint64_t   m_nCacheMisses;

  uint32_t   m_pending;  // Did we have an interrupt whilst reading a cache 
                         // line?

  CCoProcessor* m_pCoProList[16];
};

#endif // __ARMPROC_H__
