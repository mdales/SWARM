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
// name   syscopro.cpp
// author Michael Dales (michael@dcs.gla.ac.uk)
// header syscopro.h
// info   Implements the system coprocessor
//
///////////////////////////////////////////////////////////////////////////////

#include "swarm.h"
#include "syscopro.h"
#include <string.h>
#include "isa.h"
#include <iostream.h>

#include "memory.cpp"

// Defines a dull ARM7 type processor ID. Make = ARM, Arch = 3, rest NULL
#define SWARM_ID 0x41007000

#ifdef SHARED_CACHE
#define CACHE_TYPE 0x00041041
#else
#define CACHE_TYPE 0x01041041
#endif // SHARED_CACHE

// Register access permissions
#define REG_READ  0x1 
#define REG_WRITE 0x2 
                      
const static uint32_t regPermissions[16] = {REG_READ, REG_READ | REG_WRITE, 
					    0, 0, 0, 0, 0, 0, 0,
					    0, REG_READ | REG_WRITE, 0, 
					    REG_READ | REG_WRITE, 0, 0};

#define ID_REG    0x0
#define CACHE_REG 0x7
#define CYCLE_REG 0xB
#define PID_REG   0xD

#define UPDATE_DO  0x1
#define UPDATE_DI  0x2
#define UPDATE_RD  0x4
#define UPDATE_IP  0x8
#define UPDATE_CPA 0x10

#define CNTR_CYCLE 0x0  // Cycle counter
#define CNTR_CHIT  0x1  // Cache hit
#define CNTR_CMISS 0x2  // Cache miss


///////////////////////////////////////////////////////////////////////////////
// Info a wrappers for using my memory pool stuff
//
#define MAX_INST_LEN 5
#ifndef DEBUG_MEMPOOL
#define CTRLNEW()         m_pCtrlPool->Malloc();
#define CTRLFREE(_x)      m_pCtrlPool->Free(_x);
#else
#define CTRLNEW()      ({\
                              CONTROL* _p =  m_pCtrlPool->Malloc();\
                              fprintf(stderr, "creating %s at %p in %s:%d\n", "CONTROL", _p, __FILE__, __LINE__);\
                              (CONTROL*)_p;\
                            })
#define CTRLFREE(_x)    {fprintf(stderr, "deleting %s at %p in %s:%d\n", #_x, _x, __FILE__, __LINE__);\
                             m_pCtrlPool->Free(_x);\
                             _x = NULL;}
#endif
//
///////////////////////////////////////////////////////////////////////////////



#define CREATE_CONTROL(_c)  {_c = CTRLNEW(); \
                            memset(_c, 0, sizeof(CONTROL));}


/******************************************************************************
 * create_noop
 */
CSysCoPro::CONTROL* CSysCoPro::create_noop()
{
  CONTROL* c;
  
  CREATE_CONTROL(c);
  c->updates = UPDATE_IP;  
  c->bWorking = FALSE;

  return c;
}


///////////////////////////////////////////////////////////////////////////////
// CSysCoPro - 
//
CSysCoPro::CSysCoPro()
{
  memset(m_iPipe, 0, sizeof(uint32_t) * 3);
  memset(m_regsWorking, 0, sizeof(uint32_t) * 16);
  m_regsWorking[0] = SWARM_ID;
  m_regPermissions = (uint32_t*)regPermissions;
  m_busPrevious = m_busCurrent = 0;
  //m_ctrlListCur = m_ctrlListNext = NULL;

  m_regsWorking[0] = SWARM_ID;

  m_pCtrlPool = (CMemory<CONTROL>*)NEW(CMemory<CONTROL>(MAX_INST_LEN*2));

  m_ctrlListCur = (CONTROL**)TNEW(CONTROL*[MAX_INST_LEN]);
  memset(m_ctrlListCur, 0, sizeof(CONTROL*) * MAX_INST_LEN);
  m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[MAX_INST_LEN]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * MAX_INST_LEN);

  m_busCurrent = (COPROBUS*)NEW(COPROBUS);
  memset(m_busCurrent, 0, sizeof(COPROBUS));
  m_busPrevious = (COPROBUS*)NEW(COPROBUS);
  memset(m_busPrevious, 0, sizeof(COPROBUS));

  Reset();
}


///////////////////////////////////////////////////////////////////////////////
//
//
CSysCoPro::~CSysCoPro()
{
  if (m_ctrlListCur != NULL)
    {
      for (int i = 0; m_ctrlListCur[i] != NULL; i++)
	CTRLFREE(m_ctrlListCur[i]);
      TDELETE(m_ctrlListCur);
    }

  if (m_ctrlListNext != NULL)
    {
      for (int i = 0; m_ctrlListNext[i] != NULL; i++)
	CTRLFREE(m_ctrlListNext[i]);
      TDELETE(m_ctrlListNext);
    }

  if (m_busPrevious != NULL)
    DELETE(m_busPrevious);
  if (m_busCurrent != NULL)
    DELETE(m_busCurrent);

  DELETE(m_pCtrlPool);
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSysCoPro::Reset()
{
  //if (m_ctrlListCur != NULL)
    {
      for (int i = 0; m_ctrlListCur[i] != NULL; i++)
	{
	  CTRLFREE(m_ctrlListCur[i]);
	  m_ctrlListCur[i] = NULL;
	}
      //TDELETE(m_ctrlListCur);
    }
  //m_ctrlListCur = (CONTROL**)TNEW(CONTROL*[2]);
  m_ctrlListCur[0] = create_noop();
  m_ctrlListCur[1] = NULL;

  //if (m_ctrlListNext != NULL)
    {
      for (int i = 0; m_ctrlListNext[i] != NULL; i++)
	{
	  CTRLFREE(m_ctrlListNext[i]);
	  m_ctrlListNext[i] = NULL;
	}
      //TDELETE(m_ctrlListNext);
    }
  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
  m_ctrlListNext[0] = create_noop();
  m_ctrlListNext[1] = NULL;  

  m_nCtrlCur = 0;

  //m_regsWorking[CYCLE_REG] = 0;
  memset(m_regsCounters, 0, sizeof(uint32_t) * 3);

  m_regsWorking[1] = 0x00000001; // Turn on the MMU
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSysCoPro::Cycle(COPROBUS* bus)
{
  // Change the bus notes
#if 0
  if (m_busPrevious != NULL)
    {
      DELETE(m_busPrevious);
    }
  m_busPrevious = m_busCurrent;
  m_busCurrent = (COPROBUS*)NEW(COPROBUS);
  memcpy(m_busCurrent, bus, sizeof(COPROBUS));
#else
  COPROBUS* temp = m_busPrevious;
  m_busPrevious = m_busCurrent;
  m_busCurrent = temp;
  memcpy(m_busCurrent, bus, sizeof(COPROBUS));
#endif

  if (m_busCurrent->opc != 0)
    {
      CONTROL** temp;
      //if (m_ctrlListCur != NULL)
      {
	for (int i = m_nCtrlCur; m_ctrlListCur[i] != NULL; i++)
	  {
	    CTRLFREE(m_ctrlListCur[i]);
	    m_ctrlListCur[i] = NULL;
	  }
      }
      
      //TDELETE(m_ctrlListCur);
      temp = m_ctrlListCur;
      m_ctrlListCur = m_ctrlListNext;
      m_ctrlListNext = temp;
      m_nCtrlCur = 0;
    }

  // If this is the first stage we check to see if cpi has been asserted
  // If not then the core decided to skip this instruction.
  if ((m_nCtrlCur == 0) && (bus->cpi == 0))
    {
      for (int i = m_nCtrlCur; m_ctrlListCur[i] != NULL; i++)
	{
	  CTRLFREE(m_ctrlListCur[i]);
	  m_ctrlListCur[i] = NULL;
	}

      m_ctrlListCur[0] = create_noop();
      //m_ctrlListCur[1] = NULL;
    }

#ifndef QUIET
  printf("copro Execing(%d) iPipe = (0x%x, 0x%x, 0x%x)\n", m_nCtrlCur, bus->Din, m_iPipe[1], m_iPipe[2]);
  printf("updates = 0x%x  \n", m_ctrlListCur[m_nCtrlCur]->updates);
#endif

  // Has the ipipe been updated?
  if (m_busCurrent->opc != 0)
    {
      //if (m_ctrlListNext != NULL)
	{
	  for (int i = m_nCtrlCur; m_ctrlListNext[i] != NULL; i++)
	    {
	      CTRLFREE(m_ctrlListNext[i]);
	      m_ctrlListNext[i] = NULL;
	    }
	  //TDELETE(m_ctrlListNext);
	}

      Decode();
    }

  Exec();


  // Bus updates first
  if (m_ctrlListCur[m_nCtrlCur]->updates & UPDATE_CPA)
    bus->cpa = 0;  
  if (m_ctrlListCur[m_nCtrlCur]->updates & UPDATE_DO)
    {
      bus->dw = 1;
      bus->Dout = m_regDataOut;
    }
  else
    bus->dw = 0;

  if (m_ctrlListCur[m_nCtrlCur]->bWorking == TRUE)
    {
      CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
      m_ctrlListCur[m_nCtrlCur] = NULL;
      m_nCtrlCur++;
    }

  //m_regsWorking[CYCLE_REG]++;
  m_regsCounters[CNTR_CYCLE]++;
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSysCoPro::Exec()
{
  CONTROL* ctrl;
  uint32_t do_bus, di_bus;

  ctrl = m_ctrlListCur[m_nCtrlCur];

  /* Setup values */
  switch(ctrl->crm)
    {
    case CYCLE_REG:
      do_bus = m_regsCounters[ctrl->op2];
      break;
    default:
      do_bus = m_regsWorking[ctrl->crm];
    };
  di_bus = m_regDataIn;

  if (ctrl->updates & UPDATE_RD)
    {
      switch (ctrl->crd)
	{
	case CACHE_REG:
	  {
	    CacheOperations(ctrl, di_bus);
	  }
	  break;
	default:
	  {
	    m_regsWorking[ctrl->crd] = di_bus;
	  }
	  break;
	}
    }

  if (ctrl->updates & UPDATE_DI)
    m_regDataIn = m_busCurrent->Din;

  if (ctrl->updates & UPDATE_DO)
    m_regDataOut = do_bus;

  if (m_busCurrent->opc != 0)
    {      
      m_iPipe[2] = m_iPipe[1];
      m_iPipe[1] = m_busCurrent->Din;//m_iPipe[0];
      //m_iPipe[0] = m_busCurrent->Din;
    }

}


///////////////////////////////////////////////////////////////////////////////
// Decode - This processor only recongnises the MCR and MRC instructions. The 
//          main processor responsible for handling undefined instruction 
//          traps, so here we just insert a noop into the stream when we're 
//          not doing the MCR/MRC thang.
//
void CSysCoPro::Decode()
{
  INST i;

  i.raw = m_iPipe[1];

  // Is this the right copro?
  if (i.cdt.cpn != 0xF)
    {
      //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
      m_ctrlListNext[0] = create_noop();
      m_ctrlListNext[1] = NULL;

      return;
    }

  if ((i.raw & CRT_MASK) == CRT_SIG)
    DecodeCRT();
  else
    {
      //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
      m_ctrlListNext[0] = create_noop();
      m_ctrlListNext[1] = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
// DecodeCRT - 
//
void CSysCoPro::DecodeCRT()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  // Need to do something different depending on which way the data is going
  if (i.crt.ls == 1)
    DecodeMRC();
  else
    DecodeMCR();
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSysCoPro::DecodeMRC()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[4]);
  //memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 4);

  /* Stage 1 - Put the register on the Data out register */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->updates = UPDATE_DO | UPDATE_IP | UPDATE_CPA;
  c->crm = i.crt.crn;
  c->bWorking = TRUE;
  c->dd = DD_READ;
  c->op2 = i.crt.cop2;

  /* Stage 2+ - Wait until transfer is done */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;
  c->updates = 0x0;
  c->bWorking = FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSysCoPro::DecodeMCR()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[4]);
  //memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 4);

  /* Stage 1 - Idle */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->updates = UPDATE_IP | UPDATE_CPA;
  c->bWorking = TRUE;
  
  /* Stage 2 - Move the data from the data bus into the coprocessor */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->updates = UPDATE_DI;
  c->bWorking = TRUE;

  /* Stage 3 - Finish by placing the data in the correct register */
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->updates = UPDATE_RD;
  c->crd = i.crt.crn;
  c->crm = i.crt.crm;
  c->op2 = i.crt.cop2;
  c->bWorking = TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSysCoPro::DebugDump()
{
  char str[80];

  printf("-------------------------------------------------------------------------------\n");
  printf("System coprocessor debug dump\n\n");

  printf("Registers:");
  for (int j = 0; j < 4; j++)
    {
      for (int i = 0; i < 4; i++)
	printf("   0x%08X", m_regsWorking[i + (j * 4)]);
      printf("\n\t  ");
    }
  printf("\n");
  
  printf("DIn reg = 0x%08X    DOut reg = 0x%08X\n",
	    m_regDataIn, m_regDataOut);

  printf("-------------------------------------------------------------------------------\n");
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSysCoPro::NoteEvent(enum SC_EVENT e)
{
  switch (e)
    {
    case SC_CACHEHIT:
      m_regsCounters[CNTR_CHIT]++;
      break;
    case SC_CACHEMISS:
      m_regsCounters[CNTR_CMISS]++;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSysCoPro::CacheOperations(CONTROL* ctrl, uint32_t data)
{
  if ((m_pDataCache == NULL) || (m_pInstCache == NULL))
    return;

  switch (ctrl->crm)
    {
    case 5: // instruction cache invalidation
      {
	switch(ctrl->op2) 
	  {
	  case 0: // Invalidate I cache
	    {
	      m_pInstCache->Reset();		
	    }
	    break;
	  case 1: // Invalidate line by address
	    {
	      m_pInstCache->InvalidateLineByAddr(data);
	    }
	    break;
	  default:
	    break;
	  }
      }
    case 6: // Data cache invalidation
      {
	switch(ctrl->op2) 
	  {
	  case 0: // Invalidate D cache
	    {
	      m_pDataCache->Reset();
	    }
	    break;
	  case 1: // Invalidate line by address
	    {
	      m_pDataCache->InvalidateLineByAddr(data);
	    }
	    break;
	  default:
	    break;
	  }
      }
    case 7: // Unified cache invalidation
      {
	switch(ctrl->op2) 
	  {
	  case 0: // Invalidate entire cache
	    {
	      m_pDataCache->Reset();
	      if (m_pDataCache != m_pInstCache)
		m_pInstCache->Reset();		
	    }
	    break;
	  case 1: // Invalidate line by address
	    {
	      m_pDataCache->InvalidateLineByAddr(data);
	    }
	    break;
	  default:
	    break;
	  }
      }
      break;
    default:
      // Do nothing
      break;
    }
}
