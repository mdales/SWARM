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
// name   core.h
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CORE_H__
#define __CORE_H__

#include "swarm.h"
#include "alu.h"
#include "swi.h"

#include "memory.h"

// Forward decs
typedef struct CTAG CONTROL;


enum MODE {M_PREV = 0x00, M_USER = 0x10, M_FIQ = 0x11, M_IRQ = 0x12, 
	   M_SVC = 0x13, M_ABORT = 0x17, M_UNDEF = 0x1B, M_SYSTEM = 0x1F};

enum SHIFT {S_LSL = 0, S_LSR = 1, S_ASR = 2, S_ROR, S_RRX, S_ASL};

enum COND {C_EQ = 0x0, C_NE = 0x1, C_CS = 0x2, C_CC = 0x3, 
	   C_MI = 0x4, C_PL = 0x5, C_VS = 0x6, C_VC = 0x7, 
	   C_HI = 0x8, C_LS = 0x9, C_GE = 0xA, C_LT = 0xB,
           C_GT = 0xC, C_LE = 0xD, C_AL = 0xE, C_NV = 0xF};

///////////////////////////////////////////////////////////////////////////////
// These define the bits into and out of the core. Note that some of these are 
// currently ignored.
// 
typedef struct CBIOTAG
{
  uint32_t mclk : 1; // IGNORED
  uint32_t wait : 1; // IGNORED

  uint32_t prog32 : 1; // IGNORED
  uint32_t data32 : 1; // IGNORED
  uint32_t bigend : 1; // IGNORED - We assume little endian
  uint32_t lateabt : 1; // IGNORED

  uint32_t irq : 1; 
  uint32_t fiq : 1; 
  
  uint32_t reset : 1; // IGNORED
  
  uint32_t ale : 1; // IGNORED
  uint32_t dbe : 1; // IGNORED

  uint32_t Vdd : 1; // IGNORED
  uint32_t Vss : 1; // IGNORED

  uint32_t mreq : 1; // IGNORED
  uint32_t seq : 1; // IGNORED
  uint32_t lock : 1; // IGNORED
  uint32_t A : 32; 
  uint32_t Dout : 32;
  uint32_t Din : 32;
  uint32_t enout : 1; // Basically used in SWARM to differentiate between mem 
                      // and copro writes
  uint32_t rw : 1; 
  uint32_t bw : 2;   // 00 = word, 01 = byte, 10 = halfword, 11 = UNDEFINED

  uint32_t trans : 1; // IGNORED
  uint32_t mode : 4; // IGNORED
  uint32_t abort : 1; // IGNORED

  uint32_t opc : 1; 
  uint32_t cpi : 1; 
  uint32_t cpa : 1; 
  uint32_t cpb : 1; 

  uint32_t di : 1; // 0 = data, 1 = instruction

  uint32_t swi_hack : 1; // XXX: Added to tell processor to (in)validate the
                         //      cache, as a SWI upcall to swarm occurred 
} COREBUS;

///////////////////////////////////////////////////////////////////////////////
// Contains the entire state for the CPU. 
//
class CArmCore
{
  // Constructors and destructor
 public:
  CArmCore();
  ~CArmCore();

  // Public methods
 public:
  void Cycle(COREBUS* bus);
  inline uint64_t GetCycles() { return m_nCycles; }

  void RegisterSWI(uint32_t swi_number, SWI_CALL* swi);
  void UnregisterSWI(uint32_t swi_number);

  void DebugDump();
  long NextPC();

  // Private methods
 private:
  void Reset();
  bool_t CondTest(enum COND cond);
  uint32_t BarrelShifter(uint32_t nVal, enum SHIFT type, int nDist);
#ifdef ARM6
  void MultLogic(void* cs);
#endif

  void Decode();
  void Exec();
  
  void DecodeDPI();
  void DecodeBranch();
  void DecodeSWT();
  void DecodeSWTLoad();
  void DecodeSWTStore();
  void DecodeHWT();
  void DecodeHWTLoad();
  void DecodeHWTStore();
  void DecodeSWI();
  void DecodeMRT();
  void DecodeMRTLoad();
  void DecodeMRTStore();
  void DecodeMovPC();
  void DecodeMult();
  void DecodeMRS();
  void DecodeMSR();

#ifndef ARM6
  void DecodeMultMUL();
  void DecodeMultMLA();
  void DecodeMultUMULL();
  void DecodeMultUMLAL();
  void DecodeMultSMULL();
  void DecodeMultSMLAL();
#endif

  void DecodeMRC();
  void DecodeMCR();
  void DecodeCDO();
  void DecodeCWT();
  void DecodeCRT();
  void DecodeCWTLoad();
  void DecodeCWTStore();

  void GenerateUDT();

  void SetMode(enum MODE mode);

  CONTROL* create_noop();
  void create_vector(enum MODE mode, uint32_t addr, CONTROL** ctrlList);

  // Private data
 private:
  uint64_t       m_nCycles;
  enum MODE      m_mode;
  enum MODE      m_prevMode;
  uint32_t       m_regAddr;
  uint32_t       m_regDataIn;
  uint32_t       m_regDataOut;
  uint32_t       m_iPipe[3];
  uint32_t       m_regsWorking[17];
  uint32_t       m_regsUser[8];
  uint32_t       m_regsFiq[8];
  uint32_t       m_regsSvc[3];
  uint32_t       m_regsAbort[3];
  uint32_t       m_regsIrq[3];
  uint32_t       m_regsUndef[3];
  uint32_t       m_regShift;
  uint32_t       m_regShiftCarryBit;
  uint32_t       m_regMult;
#ifndef ARM6
  uint32_t       m_regsPartSum[2];
  uint32_t       m_regsPartCarry[2];
  uint32_t       m_multStage;
#endif
  bool_t         m_bMultCarry;
  CONTROL**      m_ctrlListCur;
  CONTROL**      m_ctrlListNext;
  int            m_nCtrlCur;
  uint32_t       m_regsHack[2];
  alu_fn* const * m_alu;
  COREBUS*       m_busCurrent;
  COREBUS*       m_busPrevious;
  uint32_t       m_pending; // Pending interrupts

  SWI_CALL**     m_swiCalls;

  bool_t         m_write;

  CMemory<CONTROL>* m_pCtrlPool;

#ifdef NATIVE_CHECK
  uint32_t       m_nativeCpsr;
  uint32_t       m_nativeResult;
  uint32_t       m_nativeFn[8];
  uint32_t       m_regsTemp[4];
#endif
};

#endif // __CORE_H__
