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
// name   syscopro.h
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   Implements the system coprocessor
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __SYSCOPRO_H__
#define __SYSCOPRO_H__

#define SYSCOPRO_ID  0xF

#include "copro.h"
#include "cache.h"
#include "memory.h"

enum SC_EVENT {SC_CACHEHIT, SC_CACHEMISS};

class CSysCoPro: public CCoProcessor
{
  // Constructors and destructor
 public:
  CSysCoPro();
  ~CSysCoPro();

 public:
  void Cycle(COPROBUS* bus);
  void DebugDump();
  void Reset();

  void NoteEvent(enum SC_EVENT e); // Used to send an event

  inline void RegisterCaches(CCache* pDataCache, CCache* pInstCache) 
    { m_pDataCache = pDataCache; m_pInstCache = pInstCache; }

 private:
  void Exec();
  void Decode();
  void DecodeCRT();
  void DecodeMCR();
  void DecodeMRC();

 private:
  enum DATADIR {DD_READ = 0x1, DD_WRITE};
  typedef struct CTAG
  {
    DATADIR  dd;
    uint32_t crd;
    uint32_t crm;
    uint32_t op1;
    uint32_t op2;
    uint32_t updates;
    bool_t   bWorking;
  } CONTROL;

  CONTROL* create_noop();

  void CacheOperations(CONTROL* ctrl, uint32_t data);

 private:
  COPROBUS* m_busPrevious;
  COPROBUS* m_busCurrent;

  CONTROL** m_ctrlListCur;
  CONTROL** m_ctrlListNext;
  uint32_t  m_nCtrlCur;

  uint32_t m_iPipe[3];
  uint32_t m_regsWorking[16];
  uint32_t* m_regPermissions;
  uint32_t m_regDataIn;
  uint32_t m_regDataOut;

  uint32_t m_regsCounters[3]; // Stores rpcc, cache misses, etc.

  CCache*  m_pDataCache;
  CCache*  m_pInstCache;

  CMemory<CONTROL>* m_pCtrlPool;
};

#endif // __SYSCOPRO_H__
