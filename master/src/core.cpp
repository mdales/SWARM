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
// name   core.cpp
// author Michael Dales (michael@dcs.gla.ac.uk)
// header core.h
// info   Implements an ARM7M core.
//
///////////////////////////////////////////////////////////////////////////////

#include "swarm.h"
#include "core.h"
#include "isa.h"
#include <string.h>
#include <iostream.h>
#include "disarm.h"
#ifndef ARM6
#include "booth.h"
#endif

#include "memory.cpp"

static const char* mode_str[16] = {"reset", "fiq", "irq", "svc", 
				   NULL, NULL, NULL, "abort", 
				   NULL, NULL, NULL, "undef", 
				   NULL, NULL, NULL, "system"};

enum REGS {R_R0 = 0x00, R_R1 = 0x01, R_R2 = 0x02, R_R3 = 0x03,
	   R_R4 = 0x04, R_R5 = 0x05, R_R6 = 0x06, R_R7 = 0x07,
	   R_R8 = 0x08, R_R9 = 0x09, R_R10 = 0x0A, R_FP = 0x0B,
	   R_IP = 0x0C, R_SP = 0x0D, R_LR = 0x0E, R_PC = 0x0F,
	   R_CPSR = 0x10};

enum ARI {ARI_ALU, ARI_INC, ARI_REG, ARI_NONE};
enum B_DRIVE {B_REG, B_IMM1, B_IMM2, B_DIN, B_CPSR, B_SPSR}; 
// B_IMM2 = half word transfer imm
enum SHIFT_IN {SI_NORM = 0, SI_ONE};
enum ALU_AI {AI_NORM = 0, AI_HACK, AI_MAGIC, AI_MULT_LO, AI_MULT_HI};
enum ALU_BI {BI_NORM = 0, BI_HACK, BI_NULL, BI_MULT_LO, BI_MULT_HI};
enum RD_TYPE {RD_INST, RD_DATA};
enum MUL_OP {M_MUL = 0x0, M_MLA = 0x1, M_UMULL = 0x4, M_UMLAL = 0x5,
	     M_SMULL = 0x6, M_SMLAL = 0x7};
enum MUL_STAGE {MS_NONE = 0x0, MS_ONE, MS_TWO, MS_LOOP, MS_SAVELO, MS_SAVEHI};
enum RD_WIDTH {RW_WORD = 0x0, RW_HALFWORD, RW_BYTE};
enum CP_STAGE {CP_NONE = 0x0, CP_INIT, CP_WAIT};
enum REG_BANK {RB_CURRENT = 0x0, RB_USER = 0x1};

enum VECTOR {V_RESET = 0x00, V_UNDEF = 0x04, V_SWI = 0x08, V_PABORT = 0x0C, 
	     V_DABORT = 0x10, V_IRQ = 0x18, V_FIQ = 0x1C};

/////////////////////////////////////////////////////////////////
// Control - This is control information for the datapath.
//
typedef struct CTAG
{
  enum REGS     rd, rn, /*rs,*/ rm;
  uint32_t      updates; /* See UPDATE_* defines */
  enum ARI      ari;    /* Address register input */
  enum B_DRIVE  b;
  enum OPCODE   opcode;
  enum SHIFT    shift_type;
  uint32_t      shift_dist;
  bool_t        shift_reg;   /* Is the shift distance in a register? */
  uint32_t      imm_mask;
  uint32_t      psr_mask;  /* Mask for writing to [C|S]PSR */
  enum ALU_AI   ai;
  enum ALU_BI   bi;
  enum SHIFT_IN si;
  enum COND     cond;
  bool_t        wr; /* Is the addr_reg for a mem read or write? */
  //bool_t        bw; /* Is this a byte read/write ? */
  RD_WIDTH      rw;
  bool_t        enout; /* Are we writing to memory? 0 if true, otherwise 1*/
  enum MODE     mode;
  enum RD_TYPE  rdt;
  bool_t        bSwi; /* Needed to see it we're execing a SWI call */
  uint32_t      aMagic; /* magic number to appear on A */  
  bool_t        bSign; /* sign extern constants */
  bool_t        bLoadMult;
  MUL_STAGE     mulStage;
  CP_STAGE      cpStage;
  REG_BANK      regBankRead;
  REG_BANK      regBankWrite;
#ifdef NATIVE_CHECK
  enum REGS     rs;
  bool_t        bNative;
  INST          i;
#endif
} CONTROL;


static alu_fn * const alu_table[] = {and_op, eor_op, sub_op, rsb_op, 
				    add_op, adc_op, sbc_op, rsc_op, 
				    tst_op, teq_op, cmp_op, cmn_op, 
				    orr_op, mov_op, bic_op, mvn_op};

#define UPDATE_PC 0x0001 /* Update the PC in the reg file */
#define UPDATE_RD 0x0002 /* Update the result register */
#define UPDATE_IP 0x0004 /* Update the instruction pipe */
#define UPDATE_DI 0x0008 /* Update the data in register */
#define UPDATE_DO 0x0010 /* Update the data out register */
#define UPDATE_FG 0x0020 /* Update the control flags */
#define UPDATE_SR 0x0040 /* Update the shift latch */
#define UPDATE_CS 0x0080 /* Update CPSR from register bus */
#define UPDATE_SS 0x0100 /* Update SPSR from register bus */
#define UPDATE_MR 0x0200 /* Update (reset) the multiply regs */
#define UPDATE_MS 0x0400 /* Update mult stage (i.e. do a round) */
#define UPDATE_ML 0x0800 /* Update m_regsPartSum[PLO] */
#define UPDATE_MH 0x1000 /* Update m_regsPartSum[PHI] */

#define MBITS_USER  0x10
#define MBITS_FIQ   0x11
#define MBITS_IRQ   0x12
#define MBITS_SVC   0x13
#define MBITS_ABORT 0x17
#define MBITS_UNDEF 0x1B
#define MBITS_SYS   0x1F

#define VEC_RESET   0x00000000
#define VEC_UNDEF   0x00000004
#define VEC_SWI     0x00000008
#define VEC_PABORT  0x0000000C
#define VEC_DABORT  0x00000010
#define VEC_IRQ     0x00000018
#define VEC_FIQ     0x0000001C

// Bit masks for CPSR
#define IRQ_BIT     0x00000080
#define FIQ_BIT     0x00000040

#define PHI 1
#define PLO 0


///////////////////////////////////////////////////////////////////////////////
// Info a wrappers for using my memory pool stuff
//
#define MAX_INST_LEN 22
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


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef NATIVE_CHECK

#include <machine/sysarch.h>

typedef uint32_t native_fn(uint32_t, uint32_t, uint32_t, uint32_t*);

///////////////////////////////////////////////////////////////////////////////
// We assume that the DPI has been checked for pc, etc.
//
void generate_dpi_test(INST i, uint32_t* new_fn)
{  
  int pos = 0;
  int regs = 1;

  // Mung the instuction into its new form
  i.dpi1.rd = 0;
  i.dpi1.rn = 0;
  
  if (i.dpi1.hash == 0)
    {
      i.dpi2.rm = 1;

      if (i.dpi2.pad2 != 0)
	i.dpi3.rs = 2;
    }

  // Generate the code
  new_fn[pos++] = 0xe50d4004;   // str r4, [sp, -#4]
  new_fn[pos++] = 0xe5934000;   // ldr r4, [r3]
  new_fn[pos++] = 0xe129f004;   // msr cpsr_all, r4
  new_fn[pos++] = i.raw;
  new_fn[pos++] = 0xe10f4000;   // mrs r4, cpsr_all
  new_fn[pos++] = 0xe5834000;   // str r4, [r3]
  new_fn[pos++] = 0xe51d4004;   // ldr r4, [sp, -#4]
  new_fn[pos++] = 0xe1a0f00e;   // mov pc, lr 

  // Synchronise the icache now
  arm32_sync_icache((u_int)new_fn, 8);
}

#endif
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


/******************************************************************************
 * create_noop - Creates a NULL instruction. We need to do this when we flush
 *               the pipeline. All we really need to do is set the updates to 
 *               make sure the PC and IPIPE still get updated.
 */
CONTROL* CArmCore::create_noop()
{
  CONTROL* c;

  CREATE_CONTROL(c);

  c->cond = C_AL;
  c->opcode = OP_ADD;
  c->updates = UPDATE_PC | UPDATE_IP;
  c->ari = ARI_INC;
  c->shift_reg = FALSE;
  c->b = B_REG;
  c->si = SI_ONE;
  c->rn = R_R0;
  c->rd = R_R0;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->bi = BI_NORM;
  c->wr = FALSE;
  c->rdt = RD_INST;
  c->bLoadMult = FALSE;
  c->bSign = FALSE;
  c->mulStage = MS_NONE;
  return c;
}


/******************************************************************************
 * creatre_udt - Creates the instructions for an UNDEFINED INSTRUCTION TRAP.
 */
void CArmCore::create_vector(enum MODE mode, uint32_t addr, CONTROL** ctrlList)
{
  //CONTROL** ctrlList;
  CONTROL* c;
  
  /* Make space for control path info and put it in place */
  //ctrlList = (CONTROL**)TNEW(CONTROL*[4]);
  ctrlList[3] = NULL;
    
  /* First stage of branch - load undefined instruction vector */
  CREATE_CONTROL(c);
  ctrlList[0] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_IP;
  c->ai = AI_MAGIC;
  c->aMagic = addr;
  c->bi = BI_NULL;
  c->ari = ARI_ALU;
  c->opcode = OP_ADD;
  c->shift_reg = FALSE;
  c->wr = FALSE;
  c->rdt = RD_INST;
  c->mode = mode;
  
  /* Second stage of branch - link into undef reg bank*/
  CREATE_CONTROL(c);
  ctrlList[1] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_IP | UPDATE_RD;
  c->bi = BI_NORM;
  c->ari = ARI_INC;
  c->b = B_REG;
  c->opcode = OP_MOV;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->shift_reg = FALSE;
  c->rm = R_PC;
  c->rd = R_LR;
  c->wr = FALSE;
  c->rdt = RD_INST;
  
  /* Third stage of branch - finish linking */
  CREATE_CONTROL(c);
  ctrlList[2] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_PC | UPDATE_IP | UPDATE_RD;
  c->ai = AI_MAGIC;
  c->aMagic = 4;
  c->rm = R_LR;
  c->rd = R_LR;
  c->b = B_REG;
  c->ari = ARI_INC;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->shift_reg = FALSE;
  c->bi = BI_NORM;
  c->opcode = OP_RSB;
  c->wr = FALSE;
  c->rdt = RD_INST;
}


///////////////////////////////////////////////////////////////////////////////
// CArmCore - Constructor
//
CArmCore::CArmCore()
{
  m_nCycles = 0;
  m_alu = alu_table;
  m_ctrlListNext = m_ctrlListCur = NULL;
  m_nCtrlCur = 0;
  m_busPrevious = m_busCurrent = 0;
  m_regMult = 0;
  m_bMultCarry = 0;

  m_swiCalls = (SWI_CALL**)TNEW(SWI_CALL*[MAX_SWI_CALL]);
  memset(m_swiCalls, 0, sizeof(SWI_CALL*) * MAX_SWI_CALL);

  m_pCtrlPool = (CMemory<CONTROL>*)NEW(CMemory<CONTROL>(MAX_INST_LEN*2));

  m_busCurrent = (COREBUS*)NEW(COREBUS);
  memset(m_busCurrent, 0, sizeof(COREBUS));
  m_busPrevious = (COREBUS*)NEW(COREBUS);
  memset(m_busPrevious, 0, sizeof(COREBUS));

  m_ctrlListCur = (CONTROL**)TNEW(CONTROL*[MAX_INST_LEN]);
  memset(m_ctrlListCur, 0, sizeof(CONTROL*) * MAX_INST_LEN);
  m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[MAX_INST_LEN]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * MAX_INST_LEN);

  // Reset the chip.
  Reset();
}


///////////////////////////////////////////////////////////////////////////////
// ~CArmCore - Destructor
//
CArmCore::~CArmCore()
{
  if (m_ctrlListCur != NULL)
    {
      while (m_ctrlListCur[m_nCtrlCur] != NULL) 
	{
	  //DELETE(m_ctrlListCur[m_nCtrlCur]);
	  CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
	  m_nCtrlCur++;
	}
      TDELETE(m_ctrlListCur);
    }

  if (m_ctrlListNext != NULL)
    {
      int temp = 0;
      while (m_ctrlListNext[temp] != NULL) 
	{
	  //DELETE(m_ctrlListNext[temp]);
	  CTRLFREE(m_ctrlListNext[temp]);
	  temp++;
	}
      TDELETE(m_ctrlListNext);
    }

  if (m_busPrevious != NULL)
    DELETE(m_busPrevious);

  if (m_busCurrent != NULL)
    DELETE(m_busCurrent);

  TDELETE(m_swiCalls);

  DELETE(m_pCtrlPool);
}

#ifdef ARM6
///////////////////////////////////////////////////////////////////////////////
// MultLogic - Decides the ALU operation and shift parameters for the current
//             cycle.
//
void CArmCore::MultLogic(void* cs)
{
  uint32_t temp;
  CONTROL* c = (CONTROL*)cs;

  ASSERT(c != NULL);

  temp = m_regMult & 0x3;
  temp |= (m_bMultCarry == 1) ? 4 : 0;

  // Always do 
  c->shift_type = S_LSL;
  c->shift_dist = (m_nCtrlCur - 1) << 1;
  
  // See Furber book page 94.
  switch (temp)
    {
    case 0:
      {
	c->opcode = OP_ADD;
	c->bi = BI_NULL;
      }
      break;
    case 1:
      {
	c->opcode = OP_ADD;
	c->bi = BI_NORM;
      }
      break;
    case 2:
      {
	c->opcode = OP_SUB;
	c->bi = BI_NORM;   
	c->shift_dist++;
      }
      break;
    case 3:
      {
	c->opcode = OP_SUB;
	c->bi = BI_NORM;
      }
      break;
    case 4:
      {
	c->opcode = OP_ADD;
	c->bi = BI_NORM;
      }
      break;
    case 5:
      {
	c->opcode = OP_ADD;
	c->bi = BI_NORM;  
	c->shift_dist++;
      }
      break;
    case 6:
      {
	c->opcode = OP_SUB;
	c->bi = BI_NORM;
      }
      break;
    case 7:
      {
	c->opcode = OP_ADD;
	c->bi = BI_NULL;
      }
      break;
    }

  if (m_nCtrlCur < 16)
    m_bMultCarry = (temp >> 1) & 0x1;
  else
    m_bMultCarry = 0;
}
#endif


///////////////////////////////////////////////////////////////////////////////
// cycle - Executes one cycle of the ARM core.
//
void CArmCore::Cycle(COREBUS* bus)
{
  static int saved_busCur_irq = 999, saved_psr_irq_state = 999, Count = 0;
  // Change the bus notes
#if 0
  if (m_busPrevious != NULL)
    {
      DELETE(m_busPrevious);
    }
  m_busPrevious = m_busCurrent;
  m_busCurrent = (COREBUS*)NEW(COREBUS);
  memcpy(m_busCurrent, bus, sizeof(COREBUS));
#else
  COREBUS* temp = m_busPrevious;
  m_busPrevious = m_busCurrent;
  m_busCurrent = temp;
  memcpy(m_busCurrent, bus, sizeof(COREBUS));
#endif

  // Have we been running long enough for these tests to be valid?
  if (m_busPrevious != NULL)
    {
      // Check to see if we've suffered from a reset, fiq, or irq recently.
      // Note that the order is important...
#ifndef QUIET
	if( saved_busCur_irq != m_busCurrent->irq )
	{
		if(m_busCurrent->irq == 0)
			cout << "Core: busCurrent->irq = 0\n";
		else
			cout << "Core: busCurrent->irq = 1(Oops)\n";
		saved_busCur_irq = m_busCurrent->irq;
	}	
	if( saved_psr_irq_state != (m_regsWorking[R_CPSR] & IRQ_BIT) )
	{
		saved_psr_irq_state = m_regsWorking[R_CPSR] & IRQ_BIT;			
		if( !(m_regsWorking[R_CPSR] & IRQ_BIT) )
			cout << "IRQ NOT Disabled\n";
		else
			cout << "IRQ Disabled(Oops)\n";
	}
	if( (Count++%500) == 0 )
      		cout << ".";
#endif	
      if ((m_busPrevious->reset == 1) && (m_busCurrent->reset == 0))
	{
	  // Reset and branch to zero - XXX
	}
      else if (((m_busPrevious->fiq == 1) && (m_busCurrent->fiq == 0))
	       && !(m_regsWorking[R_CPSR] & FIQ_BIT))
	{
	  m_pending |= FIQ_BIT;
#ifndef QUIET
	  cout << "FIQ pending\n";
#endif
	}
      //else if (((m_busPrevious->irq == 1) && (m_busCurrent->irq == 0))
      else if ((m_busCurrent->irq == 0)
	       && !(m_regsWorking[R_CPSR] & IRQ_BIT))
	{
	  m_pending |= IRQ_BIT;
#ifndef QUIET
	  cout << "IRQ pending\n";
#endif
	}
    }

  /* Did we request a copro instruction, and did we get an reply?
   * If not then we take a UDT. Note that this has to be done now,
   * as it may termintate the instruction (e.g. we had a one cycle
   * coprocessor data operation).
   */
  if ((bus->cpi == 1) && (m_ctrlListCur[m_nCtrlCur]->cpStage == CP_INIT))
    {
      if (bus->cpa == 1)
	{
	  // Abort the rest of this instruction
	  for (int i = m_nCtrlCur; m_ctrlListCur[i] != NULL; i++)
	    {
	      //DELETE(m_ctrlListCur[i]);
	      CTRLFREE(m_ctrlListCur[i]);
	      m_ctrlListCur[i] = NULL;
	    }
	  //TDELETE(m_ctrlListCur);
	  
	  // Replace it with a UDT
	  create_vector(M_UNDEF, VEC_UNDEF, m_ctrlListCur);
	  m_nCtrlCur = 0;
	}
      else
	{
	  // Got an acknowledgement, so jump onto the next stage
	  //DELETE(m_ctrlListCur[m_nCtrlCur]);
	  CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
	  m_ctrlListCur[m_nCtrlCur] = NULL;
	  m_nCtrlCur++;
	}
    }

  // Did a coprocessor instruction complete in the mean time?
  if ((m_ctrlListCur[m_nCtrlCur] != NULL) && 
      (m_ctrlListCur[m_nCtrlCur]->cpStage == CP_WAIT) &&
      (m_busCurrent->cpb == 1))
    {
      CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
      m_nCtrlCur++;           
    }


  // If we've finsihed the current instruction then get the next one
  if (m_ctrlListCur[m_nCtrlCur] == NULL)
    {
      //if (m_ctrlListCur != NULL)
      //TDELETE(m_ctrlListCur);

      // First see if there are any pending interrupts
      if (m_pending == 0x0)
	{
	  CONTROL** temp = m_ctrlListCur;
	  m_ctrlListCur = m_ctrlListNext;
	  m_ctrlListNext = temp;
	  m_nCtrlCur = 0;
	  m_multStage = 0;
#ifndef QUIET
	  char str[120];
	  memset(str, 0, 120);
	  //CDisarm::Decode(m_iPipe[2], str);
	  printf("---------------- Next Inst -------------------\n");
	  //printf("      %s\n", str);
#endif
	}
      else
	{
#ifndef QUIET
	  printf("---------------- Next Inst -------------------\n");
#endif
	  if (m_pending & FIQ_BIT)
	    {
#ifndef QUIET 
	      printf("Handling FIQ\n");
#endif
	      // Generate the instruction stream
	      create_vector(M_FIQ, VEC_FIQ, m_ctrlListCur);
	      m_nCtrlCur = 0;
	      m_pending &= ~FIQ_BIT;
	    }
	  else if (m_pending & IRQ_BIT)
	    {
#ifndef QUIET 
	      printf("Handling IRQ\n");
#endif
	      create_vector(M_IRQ, VEC_IRQ, m_ctrlListCur);
	      m_nCtrlCur = 0;
	      m_pending &= ~IRQ_BIT;
	    }
	}
    }

  ASSERT(m_ctrlListCur[m_nCtrlCur] != NULL);

  
  // Test the condition on this instruction
  if (!CondTest(m_ctrlListCur[m_nCtrlCur]->cond))  
    {
      // Don't exec it - flush the current micro-op queue. 
      while (m_ctrlListCur[m_nCtrlCur] != NULL) 
	{
	  //DELETE(m_ctrlListCur[m_nCtrlCur]);
	  CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
	  m_ctrlListCur[m_nCtrlCur] = NULL;
	  m_nCtrlCur++;
	}
      // Put in a no-op 
      m_ctrlListCur[0] = create_noop();
      m_nCtrlCur = 0;
    }

  if (m_ctrlListCur[m_nCtrlCur]->bSwi == TRUE)
    {
      // This will be a no-op in a moment (on Exec) but we also call
      // the user's code
      uint32_t index = m_iPipe[2] & 0x007FFFFF;
      m_regsWorking[R_R0] = m_swiCalls[index](m_regsWorking[R_R0],
					      m_regsWorking[R_R1],
					      m_regsWorking[R_R2],
					      m_regsWorking[R_R3]);
      bus->swi_hack = 1;
    }



  // Did that last operation update the iPipe? If so we need to do a new
  // decode...
  if (m_ctrlListCur[m_nCtrlCur]->updates & UPDATE_IP)
    {
      if (m_ctrlListNext != NULL)
	{
	  int i = 0;
	  
	  // Don't exec it - flush the current micro-op queue. 
	  while (m_ctrlListNext[i] != NULL) 
	    {
	      //DELETE(m_ctrlListNext[i]);
	      CTRLFREE(m_ctrlListNext[i]);
	      m_ctrlListNext[i] = NULL;
	      i++;
	    }
	  //TDELETE(m_ctrlListNext);
	}
      Decode();
    }

#ifdef NATIVE_CHECK
  if ((m_ctrlListCur[m_nCtrlCur]->bNative) && (m_mode == M_USER))
    {
      m_regsTemp[0] = m_regsWorking[m_ctrlListCur[m_nCtrlCur]->rn];
      m_regsTemp[1] = m_regsWorking[m_ctrlListCur[m_nCtrlCur]->rn];
      generate_dpi_test(m_ctrlListCur[m_nCtrlCur]->i, m_nativeFn);
      m_nativeCpsr = m_regsWorking[16];
      m_nativeResult = 
	((native_fn*)m_nativeFn)(m_regsWorking[m_ctrlListCur[m_nCtrlCur]->rn],
			       m_regsWorking[m_ctrlListCur[m_nCtrlCur]->rm],
			       m_regsWorking[m_ctrlListCur[m_nCtrlCur]->rs],
			       &m_nativeCpsr);
      //printf("Real=0x%08X\tCPSR=0x%08X\told rn=x%08x\told rm=0x%08x\n", m_nativeResult, m_nativeCpsr, m_regsTemp[0], m_regsTemp[1]);

      //DebugDump();
    }
#endif

  // Exercise the datapath  
  Exec();

#ifdef NATIVE_CHECK
  //m_nativeResult = m_regsWorking[m_ctrlListCur[m_nCtrlCur]->rd];
  //m_nativeCpsr = m_regsWorking[16];
  if ((m_ctrlListCur[m_nCtrlCur]->bNative) && (m_mode == M_USER))
    {
      if ((m_ctrlListCur[m_nCtrlCur]->updates & UPDATE_RD))
	if ((m_regsWorking[m_ctrlListCur[m_nCtrlCur]->rd] !=
	     m_nativeResult))
	  {
	    printf("Result not the same. Real = 0x%08X  SWARM = 0x%08X\n", 
		      m_nativeResult, 
		      m_regsWorking[m_ctrlListCur[m_nCtrlCur]->rd]);	
	    printf("Had converted 0x%08x to 0x%08x\n", 
		      m_ctrlListCur[m_nCtrlCur]->i.raw, 
		      m_nativeFn[3]);
	    printf("Real=0x%08X\tCPSR=0x%08X\told rn= 0x%08x\n", m_nativeResult, m_nativeCpsr, m_regsTemp[0]);
	    DebugDump();  
	    exit(0);
	  }	
      if (m_regsWorking[16] != m_nativeCpsr)
	{
	  printf("CPSR not the same. Real = 0x%08X  SWARM = 0x%08X\n", 
		    m_nativeCpsr, 
		    m_regsWorking[16]);	  	
	    printf("Had converted 0x%08x to 0x%08x\n", 
		      m_ctrlListCur[m_nCtrlCur]->i.raw, 
		      m_nativeFn[3]);
	  printf("Real=0x%08X\tCPSR=0x%08X\told rn= 0x%08x\n", m_nativeResult, m_nativeCpsr, m_regsTemp[0]);
	  DebugDump();
	  exit(0);
	}	
    }
#endif

  ///////////////////////
  // Update the bus 
  bus->rw = (m_write == FALSE) ? 0 : 1;
  bus->A = m_regAddr;
  bus->Dout = m_regDataOut;
#if 0
  bus->bw = m_ctrlListCur[m_nCtrlCur]->bw ? 1 : 0;
#else
  // XXX: Needs fixing.
  switch (m_ctrlListCur[m_nCtrlCur]->rw)
    {
    case RW_WORD : 
      bus->bw = 0;	
      break;
    case RW_BYTE :
      bus->bw = 1;
      break;
    case RW_HALFWORD :
      bus->bw = 2;
      break;
    }

  //bus->bw = m_ctrlListCur[m_nCtrlCur]->rw == RW_BYTE ? 1 : 0;
#endif
  bus->opc = m_ctrlListCur[m_nCtrlCur]->updates & UPDATE_IP ? 1 : 0;
  bus->cpi = 0;
  bus->enout = m_ctrlListCur[m_nCtrlCur]->enout;

  /////////////////////////////////////////////////////////////////
  // Prepare for next time. We should advance along the control list, unless
  // this is a special case. Currext special cases: 
  //   
  //            * Multiply
  //            * Coprocessor instruction

  if (m_ctrlListCur[m_nCtrlCur]->cpStage == CP_INIT)
    {
      // If we were not touched at the begining of the cycle, then
      // keep asserting cpi
      bus->cpi = 1;
      bus->cpa = 1;
    }
  else if (m_busCurrent->cpb == 0)
    {
      // The coprocessor is busy, so we keep looking the current
      // instruction until it's done.
    }
#ifdef ARM6
  else if ((m_ctrlListCur[m_nCtrlCur]->mulStage == MS_LOOP) && 
	   ((m_regMult != 0) || (m_bMultCarry != 0)))
    {
      // Technically I could not move the ctrl node along the list,
      // but the multiplier logic needs to know the current cycle,
      // so I must update m_nCtrlCur.
      m_ctrlListCur[m_nCtrlCur + 1] = m_ctrlListCur[m_nCtrlCur];
      m_nCtrlCur++;
    }
#else
  else if (m_ctrlListCur[m_nCtrlCur]->mulStage == MS_ONE)
    {
      //DELETE(m_ctrlListCur[m_nCtrlCur]);
      CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
      m_ctrlListCur[m_nCtrlCur] = NULL;
      m_nCtrlCur++;
  
      if (m_regMult == 0)
	{
	  //DELETE(m_ctrlListCur[m_nCtrlCur]);
	  CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
	  m_ctrlListCur[m_nCtrlCur] = NULL;
	  m_nCtrlCur++;  
	}
    }
  else if (m_ctrlListCur[m_nCtrlCur]->mulStage == MS_LOOP)
    {        
      if (m_regMult == 0)
	{
	  //DELETE(m_ctrlListCur[m_nCtrlCur]);
	  CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
	  m_ctrlListCur[m_nCtrlCur] = NULL;
	  m_nCtrlCur++;  
	}
    }
#endif
  else
    {
      // Default action, used most often
      //DELETE(m_ctrlListCur[m_nCtrlCur]);
      CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
      m_ctrlListCur[m_nCtrlCur] = NULL;
      m_nCtrlCur++;      
    }

  m_nCycles++;  
}


///////////////////////////////////////////////////////////////////////////////
// reset - Resets the core as if the reset pin had been set high.
//
void CArmCore::Reset()
{
  // Set the default modes and addresses
  m_mode = M_SVC;
  m_prevMode = M_USER;
  m_regAddr = 0x00000000;
  m_write = FALSE;
  m_pending = 0x00000000;

  m_regShiftCarryBit = 0;

  // Flush that pipeline...
  if (m_ctrlListCur != NULL)
    {
      while (m_ctrlListCur[m_nCtrlCur] != NULL) 
	{
	  //DELETE(m_ctrlListCur[m_nCtrlCur]);
	  CTRLFREE(m_ctrlListCur[m_nCtrlCur]);
	  m_ctrlListCur[m_nCtrlCur] = NULL;
	  m_nCtrlCur++;
	}
    }
  else
    {
      //m_ctrlListCur = (CONTROL**)TNEW(CONTROL*[2]);
      m_ctrlListCur[1] = NULL;
    }
  m_ctrlListCur[0] = create_noop();

  if (m_ctrlListNext != NULL)
    {
      int temp = 0;
      while (m_ctrlListNext[temp] != NULL) 
	{
	  //DELETE(m_ctrlListNext[temp]);
	  CTRLFREE(m_ctrlListNext[temp]);
	  m_ctrlListNext[temp] = NULL;
	  temp++;
	}
    }
  else
    {
      //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
      m_ctrlListNext[1] = NULL;
    }
  // Put in a no-op 
  m_ctrlListNext[0] = create_noop();

  // Need to clear all the program status registers
  m_regsWorking[R_CPSR] = 0;
  m_regsUser[7] = 0;
  m_regsFiq[7] = 0;
  m_regsSvc[2] = 0;
  m_regsAbort[2] = 0;
  m_regsIrq[2] = 0;
  m_regsUndef[2] = 0;
  
#ifndef ARM6
  m_regsPartSum[PLO] = 0;
  m_regsPartSum[PHI] = 0;
  m_regsPartCarry[PLO] = 0;
  m_regsPartCarry[PHI] = 0;
#endif

  SetMode(M_SVC);

  m_nCtrlCur = 0;  
}


///////////////////////////////////////////////////////////////////////////////
// condTest - Returns TRUE if a condition code is met given the current set of
//            flags.
//
bool_t CArmCore::CondTest(enum COND cond)
{
  uint32_t codes = m_regsWorking[R_CPSR];

  switch (cond)
    {
    case C_EQ: return ((codes & Z_FLAG) ? TRUE : FALSE); break;
    case C_NE: return ((codes & Z_FLAG) ? FALSE : TRUE); break;
    case C_CS: return ((codes & C_FLAG) ? TRUE : FALSE); break;
    case C_CC: return ((codes & C_FLAG) ? FALSE : TRUE); break;
    case C_MI: return ((codes & N_FLAG) ? TRUE : FALSE); break;
    case C_PL: return ((codes & N_FLAG) ? FALSE : TRUE); break;
    case C_VS: return ((codes & V_FLAG) ? TRUE : FALSE); break;
    case C_VC: return ((codes & V_FLAG) ? FALSE : TRUE); break;
    case C_HI: return ((codes & CZ_FLAGS) == C_FLAG); break;
    case C_LS: return ((codes & C_FLAG) == 0) ||
		 ((codes & Z_FLAG) == Z_FLAG); break;
    case C_GE: return (((codes & NV_FLAGS) == 0) || 
                     ((codes & NV_FLAGS) == NV_FLAGS)); break;
    case C_LT: return (((codes & NV_FLAGS) == V_FLAG) ||
                     ((codes & NV_FLAGS) == N_FLAG)); break;
    case C_GT: return (((codes & Z_FLAG) == 0) && 
                     (((codes & NV_FLAGS) == NV_FLAGS) || 
                      ((codes & NV_FLAGS) == 0))); break;
    case C_LE: return (((codes & Z_FLAG) == Z_FLAG) || 
                     (((codes & NV_FLAGS) == V_FLAG) || 
                      ((codes & NV_FLAGS) == N_FLAG))); break;
    case C_AL: return TRUE; break;
    case C_NV: return FALSE; break;
    }

	// Should never reach here
	return TRUE;
}



#define SHIFT_LEFT(_x,_y)  ((_y >= 32) ? 0 : _x << _y)
#define SHIFT_RIGHT(_x,_y) ((_y >= 32) ? 0 : _x >> _y)
#define SIGNED_SHIFT_RIGHT(_x,_y) ((_y >= 32) ? ((_x >> 31) * 0xFFFFFFFF) : ((int32_t)_x) >> _y)

#if 0

///////////////////////////////////////////////////////////////////////////////
// BarrelShifter - Implements the ARM's barrel shifter. Returns the shifted
//                 value.
// 
uint32_t CArmCore::BarrelShifter(uint32_t nVal, enum SHIFT type, int nDist)
{
  if ((nDist == 0) && (type != S_RRX))
    {
      m_regShiftCarryBit = (m_regsWorking[R_CPSR] | C_FLAG) ? 1 : 0;
      return nVal;
    }

  switch (type)
    {
    case S_LSL: case S_ASL: 
      m_regShiftCarryBit = SHIFT_RIGHT(nVal, 32 - nDist) & 0x1;
      return (nVal << nDist); break;
    case S_LSR: 
      m_regShiftCarryBit = (nVal >> (nDist - 1)) & 0x1;
      return (nVal >> nDist); break;
    case S_ASR: 
      {
        uint32_t temp = 0xFFFFFFFF;	
	m_regShiftCarryBit = (nVal >> (nDist - 1)) & 0x1;		
	if ((nVal & 0x80000000) != 0)
	  nVal = (nVal >> nDist) | (~(temp >> (nDist /* - 1 */)));
	else
	  nVal = (nVal >> nDist);

        return nVal;
      }
      break;      
    case S_ROR:
      {
	uint32_t temp = (nVal >> nDist) | (nVal << (32 - nDist));
	m_regShiftCarryBit = (temp & 0x80000000) != 0;
        return temp;
      }
      break;
    case S_RRX:
      {
	m_regShiftCarryBit = nVal & 0x1;

        uint32_t temp = nVal & 0x1;
        nVal = nVal >> 1;
        if (m_regsWorking[R_CPSR] & C_FLAG)
          nVal |= 0x80000000;

        return nVal;
      }
      break;
    }

  return nVal;
}
#endif


///////////////////////////////////////////////////////////////////////////////
//
//
uint32_t CArmCore::BarrelShifter(uint32_t nVal, enum SHIFT type, int nDist)
{
  switch (type)
    {
      ////////////////////////////////////////////////////////////////
    case S_LSL: case S_ASL:
      {
	if (nDist == 0)
	  {
	    m_regShiftCarryBit = (m_regsWorking[R_CPSR] & C_FLAG) ? 1 : 0;
	    return nVal;
	  }
	else if (nDist == 32)
	  {
	    m_regShiftCarryBit = nVal & 0x1;
	    return 0;
	  }
	else if (nDist > 32)
	  {
	    m_regShiftCarryBit = 0;
	    return 0;
	  }
	else
	  {
	    m_regShiftCarryBit = SHIFT_RIGHT(nVal, 32 - nDist) & 0x1;
	    return (nVal << nDist); break;
	  }
      }
      break;
      ////////////////////////////////////////////////////////////////
    case S_LSR:
      {
	if (nDist == 0)
	  {
	    m_regShiftCarryBit = (m_regsWorking[R_CPSR] & C_FLAG) ? 1 : 0;
	    return nVal;
	  }
	else if (nDist == 32)
	  {
	    m_regShiftCarryBit = (nVal >> 31);
	    return 0;
	  }
	else if (nDist > 32)
	  {
	    m_regShiftCarryBit = 0;
	    return 0;
	  }
	else
	  {
	    m_regShiftCarryBit = (nVal >> (nDist - 1)) & 0x1;
	    return (nVal >> nDist); 
	  }
      }
      break;
      ////////////////////////////////////////////////////////////////
    case S_ASR:
      {
	if (nDist == 0)
	  {
	    m_regShiftCarryBit = (m_regsWorking[R_CPSR] & C_FLAG) ? 1 : 0;
	    return nVal;
	  }
	else if (nDist >= 32)
	  {
	    if (nVal & 0x80000000)
	      {
		m_regShiftCarryBit = 1;
		return 0xFFFFFFFF;
	      }
	    else
	      {
		m_regShiftCarryBit = 0;
		return 0;
	      }
	  }
	else
	  {
	    uint32_t temp = 0xFFFFFFFF;	
	    m_regShiftCarryBit = (nVal >> (nDist - 1)) & 0x1;		
	    if ((nVal & 0x80000000) != 0)
	      nVal = (nVal >> nDist) | (~(temp >> (nDist /* - 1 */)));
	    else
	      nVal = (nVal >> nDist);
	    
	    return nVal;
	  }
      }
      break;
      ////////////////////////////////////////////////////////////////
    case S_ROR:
      {
	if (nDist == 0)
	  {
	    m_regShiftCarryBit = (m_regsWorking[R_CPSR] & C_FLAG) ? 1 : 0;
	    return nVal;
	  }
	else if ((nDist & 0x1F) == 0)
	  {
	    m_regShiftCarryBit = nVal >> 31;
	    return nVal;
	  }
	else
	  {
	    nDist &= 0x1F;
	    m_regShiftCarryBit = SHIFT_RIGHT(nVal, nDist - 1) & 0x1;
	    return (nVal >> nDist) | (nVal << (32 - nDist));
	  }
      }
      break;
      ////////////////////////////////////////////////////////////////
    case S_RRX:
      {
	m_regShiftCarryBit = nVal & 0x1;
	
        uint32_t temp = nVal & 0x1;
        nVal = nVal >> 1;
        if (m_regsWorking[R_CPSR] & C_FLAG)
          nVal |= 0x80000000;

        return nVal;
      }
      break;
    }
}




///////////////////////////////////////////////////////////////////////////////
// Exec - Executes the current datapath control structure.
//
void CArmCore::Exec()
{
  CONTROL* ctrl;

  uint32_t a_bus, b_bus = 0, res_bus, alu_b_bus = 0, alu_a_bus = 0, inc_pc,
    b_bus_shifted, nFlags;//, shift_in;

  ctrl = m_ctrlListCur[m_nCtrlCur];

#ifdef ARM6
  // Multiply?
  if ((ctrl->mulStage != MS_NONE) && (ctrl->mulStage != MS_ONE))
    {
      // Alter control based on this.
      MultLogic(ctrl);
    }
#endif

#ifndef QUIET
  printf("\tExecing(%d) (0x%08x, 0x%08x, 0x%08x)\n", m_nCtrlCur,
	 m_busCurrent->Din, m_iPipe[1], m_iPipe[2]);


  int ii, jj;
  for (ii = 0; ii < 4; ii++)
    {
      printf("\t");
      for (jj = 0; jj < 4; jj++)
	printf("r%02d[0x%08x] ", jj + (ii * 4), m_regsWorking[jj + (ii * 4)]);
      printf("\n");
    }
  printf("\tCPSR[0x%08x]        ", m_regsWorking[16]);

  if (m_mode == M_FIQ)
    printf("\tSPSR_%s[0x%08x]\n", mode_str[m_mode & 0xF], m_regsFiq[7]);
  else if ((m_mode == M_USER) || (m_mode == M_SYSTEM))
    printf("\n");
  else
    {
      uint32_t* temp;
      switch (m_mode)
	{
	case M_IRQ : temp = m_regsIrq; break;
	case M_SVC : temp = m_regsSvc; break;
	case M_ABORT : temp = m_regsAbort; break;
	case M_UNDEF : temp = m_regsUndef; break;
	case M_USER: case M_FIQ : case M_SYSTEM: default: 
	  temp = NULL; break; // ???
	}
           
      if (temp != NULL)
	printf("\tSPSR_%s[0x%08x]\n", mode_str[m_mode & 0xF], temp[2]);
    }

  printf("\tpartsum = 0x%08x%08x  partcarry = 0x%08x%08x\n",
	 m_regsPartSum[PHI], m_regsPartSum[PLO], 
	 m_regsPartCarry[PHI], m_regsPartCarry[PLO]);
  printf("\tmultreg = 0x%08x carry = 0x%x\n", m_regMult, m_bMultCarry);

  printf("\tdin = 0x%08x ", m_regDataIn);
  printf("dout = 0x%08x ", m_regDataOut);
  printf("rd = %d ", ctrl->rd);
  printf("u = 0x%03x ", ctrl->updates);
  printf("sc = %d ", m_regShiftCarryBit);
  printf("addr = 0x%08x\n ", m_regAddr);  
#endif // QUIET

  // Does this involve a mode change?
  if (ctrl->mode != M_PREV)
    SetMode(ctrl->mode);

#ifndef ARM6
  if (ctrl->mulStage == MS_SAVELO)
    {
      /* printf("%d  %d  %d\n", m_bMultCarry, m_multStage, ctrl->bSign); */
      fflush(stdout);
      if ((m_bMultCarry) && (!((m_multStage == 4) && (ctrl->bSign == TRUE))))
	{
	  /*printf("bah\n");
	    fflush(stdout); */

	  // XXX: Hack - this really should use the a or b bus
	  carry_save_adder_32(m_regsPartSum[PLO], 
			      SHIFT_LEFT(m_regsWorking[ctrl->rn], (8 * m_multStage)), 
			      m_regsPartCarry[PLO],
			      &m_regsPartSum[PLO], &m_regsPartCarry[PLO]);

	  carry_save_adder_32(m_regsPartSum[PHI],
			      ctrl->bSign == 0 ? 
			      SHIFT_RIGHT(m_regsWorking[ctrl->rn], (32 - (8 * m_multStage))) : SIGNED_SHIFT_RIGHT(m_regsWorking[ctrl->rn], (32 - (8 * m_multStage))),
			      m_regsPartCarry[PHI],
			      &m_regsPartSum[PHI], &m_regsPartCarry[PHI]);
	  m_regsPartCarry[PHI] = (m_regsPartCarry[PHI] << 1) | 
	    (m_regsPartCarry[PLO] >> 31);
	  m_regsPartCarry[PLO] <<= 1;
	  m_bMultCarry = 0;
	}
    }
#endif

  /* Put values on the buses */
  a_bus = m_regsWorking[ctrl->rn];
  switch (ctrl->b)
    {
    case B_REG:
      if ((ctrl->regBankRead == RB_CURRENT) || (m_mode == M_USER))
	b_bus = m_regsWorking[ctrl->rm];
      else if (ctrl->regBankRead == RB_USER)
	{
	  if (m_mode == M_FIQ)
	    {
	      if ((ctrl->rm >= 8) && (ctrl->rm <= 14))
		b_bus = m_regsUser[ctrl->rm - 8];
	      else
		b_bus = m_regsWorking[ctrl->rm];
	    }
	  else
	    {
	      if ((ctrl->rm >= 13) && (ctrl->rm <= 14))
		b_bus = m_regsUser[ctrl->rm - 8];
	      else
		b_bus = m_regsWorking[ctrl->rm];
	    }
	}
      break;
    case B_IMM1:
      b_bus = m_iPipe[2] & ctrl->imm_mask;
      /* sign extend */
      if (ctrl->bSign)
	if (((~(ctrl->imm_mask >> 1)) & b_bus) != 0)
	  b_bus |= ~(ctrl->imm_mask);
      break;
    case B_IMM2:
      // Shift immediate for half word shift
      b_bus = (m_iPipe[2] & 0x0000000F) | ((m_iPipe[2] >> 4) & 0x000000F0);
      /* sign extend */
      if (ctrl->bSign)
	if (((~(ctrl->imm_mask >> 1)) & b_bus) != 0)
	  b_bus |= ~(ctrl->imm_mask);
      break;
    case B_DIN:
      b_bus = m_regDataIn;
      if (ctrl->bSign != 0)
	if (((~(ctrl->imm_mask >> 1)) & b_bus) != 0)
	  b_bus |= ~(ctrl->imm_mask);
      break;
    case B_CPSR:
      b_bus = m_regsWorking[R_CPSR];
      break;
    case B_SPSR:
      // Should never happen from user or system modes.
      switch (m_mode)
	{
	case M_IRQ: b_bus = m_regsIrq[2]; break;
	case M_FIQ: b_bus = m_regsFiq[7]; break;
	case M_SVC: b_bus = m_regsSvc[2]; break;
	case M_ABORT: b_bus = m_regsAbort[2]; break;
	case M_UNDEF: b_bus = m_regsUndef[2]; break;
	case M_PREV : case M_USER : case M_SYSTEM : 
	  // Other code should prevent this happening
	  b_bus = 0xDEADDEAD; break;
	}
      break;
    }

  bool_t bCarry = 0;
  if (ctrl->shift_reg)
    {
      static int ShifterDistanceErrorCount = 0;

      // XXX: This is broken, but at least the assert stops the brokenness
      // getting done by accident
      // ASSERT(m_regShift < 32);
      if (m_regShift >= 32)
      {
		if((ShifterDistanceErrorCount % 25) == 0)
		{
			cerr << "Core: Shifter distance more than 31, i.e:" << m_regShift << "\n";
			ShifterDistanceErrorCount++;
		}	
#ifndef QUIET	      
		DebugDump();
#endif	      
      }

      b_bus_shifted = BarrelShifter(b_bus, ctrl->shift_type, m_regShift);
      //if (m_regShift != 0)
      //m_regShiftCarryBit = b_bus_shifted >> 31;
    }
  else
    {
      b_bus_shifted = BarrelShifter(b_bus, ctrl->shift_type, ctrl->shift_dist);
      //if (ctrl->shift_dist != 0)
      //m_regShiftCarryBit = b_bus_shifted >> 31;
    }


  switch(ctrl->ai)
    {
    case AI_NORM:
      alu_a_bus = a_bus;
      break;
    case AI_HACK:
      alu_a_bus = m_regsHack[0];
      break;
    case AI_MAGIC:
      alu_a_bus = ctrl->aMagic;
      break;
#ifndef ARM6
    case AI_MULT_LO:
      alu_a_bus = m_regsPartSum[PLO];
      break;
    case AI_MULT_HI:
      alu_a_bus = m_regsPartSum[PHI];
      break;
#endif
    }
  switch(ctrl->bi)
    {
    case BI_NORM:
      alu_b_bus = b_bus_shifted;
      break;
    case BI_HACK:
      alu_b_bus = m_regsHack[1];
      break;
    case BI_NULL:
      alu_b_bus = 0x00000000;
      break;
#ifndef ARM6
    case BI_MULT_LO:
      alu_b_bus = m_regsPartCarry[PLO];
      break;
    case BI_MULT_HI:
      alu_b_bus = m_regsPartCarry[PHI];
      break;
#endif
    }

  nFlags = m_regsWorking[R_CPSR];

  /* Do the calculations */
  res_bus = m_alu[ctrl->opcode](alu_a_bus, alu_b_bus, &nFlags);
  inc_pc = m_regAddr + 4;

#ifdef ARM6
  if (ctrl->mulStage == MS_ONE)
    {
      m_regMult = b_bus;
    }
  else
    {
      if (ctrl->mulStage != MS_NONE)
	m_regMult = m_regMult >> 2;
    }
#else
  if (ctrl->mulStage == MS_ONE)
    {
      m_bMultCarry = 0;
      m_regMult = b_bus;
    }
  if (ctrl->updates & UPDATE_MS)
    {
      four_stage_booth(&m_regsPartSum[PHI], &m_regsPartSum[PLO],
		       &m_regsPartCarry[PHI], &m_regsPartCarry[PLO],
		       a_bus, m_multStage++, &m_bMultCarry,
		       &m_regMult, ctrl->bSign);
    }
#endif

  if (ctrl->updates & UPDATE_FG)
    {
      m_regsWorking[R_CPSR] &= 0x0FFFFFF;      
      if ((m_regShiftCarryBit != 0) && 
	  ((ctrl->opcode == OP_AND) ||
	   (ctrl->opcode == OP_EOR) ||
	   (ctrl->opcode == OP_TST) || 
	   (ctrl->opcode == OP_TEQ) ||
	   (ctrl->opcode == OP_ORR) ||
	   (ctrl->opcode == OP_MOV) ||
	   (ctrl->opcode == OP_BIC) ||
	   (ctrl->opcode == OP_MVN)))
	m_regsWorking[R_CPSR] |= C_FLAG;
      m_regsWorking[R_CPSR] |= nFlags;
    }

  /* Now write the results where we want them */
  if (ctrl->updates & UPDATE_CS)
    {
      // Does the new mode mean a mode change?
      int newmode = ((m_regsWorking[R_CPSR] & ~ctrl->psr_mask) |
		     (res_bus & ctrl->psr_mask)) & 0x1F;
      if (newmode != m_mode)
	SetMode((enum MODE)(newmode));

      m_regsWorking[R_CPSR] &= ~ctrl->psr_mask;
      m_regsWorking[R_CPSR] |= res_bus & ctrl->psr_mask;
    }
  if (ctrl->updates & UPDATE_SS)
    {
      // User may change the mode
      int newMode;

      // Should not happen in user or system modes. We assume that
      // other code prevents this ever happening (note that the other
      // code doesn't yet exist...).
      switch (m_mode)
	{
	case M_FIQ:
	  m_regsFiq[7] &= ~ctrl->psr_mask;
	  m_regsFiq[7] |= res_bus & ctrl->psr_mask;
	  newMode = m_regsFiq[7] & 0x1F;
	  break;
	case M_IRQ:
	  m_regsIrq[2] &= ~ctrl->psr_mask;
	  m_regsIrq[2] |= res_bus & ctrl->psr_mask;
	  newMode = m_regsIrq[2] & 0x1F;
	  break;
	case M_SVC:
	  m_regsSvc[2] &= ~ctrl->psr_mask;
	  m_regsSvc[2] |= res_bus & ctrl->psr_mask;
	  newMode = m_regsSvc[2] & 0x1F;
	  break;
	case M_ABORT:
	  m_regsAbort[2] &= ~ctrl->psr_mask;
	  m_regsAbort[2] |= res_bus & ctrl->psr_mask;
	  newMode = m_regsAbort[2] & 0x1F;
	  break;
	case M_UNDEF:
	  m_regsUndef[2] &= ~ctrl->psr_mask;
	  m_regsUndef[2] |= res_bus & ctrl->psr_mask;
	  newMode = m_regsUndef[2] & 0x1F;
	  break;
	case M_PREV : case M_USER : case M_SYSTEM:
	  break;
	}

      // The user may have updated the mode in the SPSR
      m_prevMode = (enum MODE)newMode;
    }

  // Update registers - LDM can instruct the core to load the user mode 
  // registers from a privileged mode - hence why this code is more 
  // complicated than a straigt array assignment.
  if (ctrl->updates & UPDATE_RD)
    if ((ctrl->regBankWrite == RB_CURRENT) || (m_mode == M_USER))
      m_regsWorking[ctrl->rd] = res_bus;
    else if (ctrl->regBankWrite == RB_USER)
      {
	if (m_mode == M_FIQ)
	  {
	    if ((ctrl->rd >= 8) && (ctrl->rd <= 14))
	      m_regsUser[ctrl->rd - 8] = res_bus;
	    else
	      m_regsWorking[ctrl->rd] = res_bus;		
	  }
	else
	  {
	    if ((ctrl->rd >= 13) && (ctrl->rd <= 14))
	      m_regsUser[ctrl->rd - 8] = res_bus;
	    else
	      m_regsWorking[ctrl->rd] = res_bus;	
	  }
      }

  if (ctrl->updates & UPDATE_PC)
    m_regsWorking[R_PC] = inc_pc;
  if (ctrl->updates & UPDATE_DO)
    {
#if 0
      //if (ctrl->bw == 0)
      if (m_busCurrent->bw == 0)
	m_regDataOut = b_bus;
      else
	{
	  uint32_t bval = b_bus & 0x000000FF;
	  m_regDataOut = (bval) | (bval << 8) | (bval << 16) | (bval << 24);
	}
#else
      switch (ctrl->rw)
	{
	case RW_BYTE:
	  {
	    uint32_t bval = b_bus & 0x000000FF;
	    m_regDataOut = (bval) | (bval << 8) | (bval << 16) | (bval << 24);
	  }
	  break;
	case RW_HALFWORD:
	  {
	    uint32_t hwval = b_bus & 0x0000FFFF;
	    m_regDataOut = hwval | (hwval << 16);
	  }
	  break;
	case RW_WORD: default:
	  m_regDataOut = b_bus;
	  break;
	}
#endif
    }
  if (ctrl->updates & UPDATE_IP)
    {
      // Update our instruction pipeline
      m_iPipe[2] = m_iPipe[1];
      m_iPipe[1] = m_busCurrent->Din;//m_iPipe[0];
      //m_iPipe[0] = m_busCurrent->Din;
    }
  if (ctrl->updates & UPDATE_DI)
    {
      m_regDataIn = m_busCurrent->Din;
    }
  if (ctrl->updates & UPDATE_SR)
    {
      // Copy the lower 8 bits only
      m_regShift = b_bus & 0x000000FF;
    }
  if (ctrl->updates & UPDATE_MR)
    {
      m_regsPartSum[PLO] = 0;
      m_regsPartSum[PHI] = 0;
      m_regsPartCarry[PLO] = 0;
      m_regsPartCarry[PHI] = 0;
    }
  if (ctrl->updates & UPDATE_ML)
    {
      m_regsPartSum[PLO] = b_bus; 
    }
  if (ctrl->updates & UPDATE_MH)
    {
      m_regsPartSum[PHI] = a_bus;
    }

  m_regsHack[0] = a_bus;
  m_regsHack[1] = b_bus_shifted;

  switch (ctrl->ari)
    {
    case ARI_INC:
      m_regAddr = inc_pc;
      break;
    case ARI_ALU:
      m_regAddr = res_bus;
      break;
    case ARI_REG:
      m_regAddr = m_regsWorking[R_PC];
      break;
    case ARI_NONE:
      // Don't update!
      // m_regAddr = m_regAddr;
      break;
    }
  m_write = ctrl->wr;
} 


///////////////////////////////////////////////////////////////////////////////
// Decode - Decodes the instruction in the middle of the ipipe.
//
void CArmCore::Decode()
{
  INST i;

  i.raw = m_iPipe[1];


#ifndef QUIET
  printf("Decoding 0x%08x\n", i.raw);
#endif //QUIET

  // First see if the condition code is valid
  if ((i.raw & 0xF0000000) == 0xF0000000)
    {
      create_vector(M_UNDEF, VEC_UNDEF, m_ctrlListNext);
      return;
    }

  if (i.raw == 0)
    {
      /* Failed to decode it - insert a noop for now. */
      //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
      m_ctrlListNext[0] = create_noop();
      m_ctrlListNext[1] = NULL;
    }
  else if ((i.raw & MSR_MASK) == MSR_SIG)
    {
      DecodeMSR();
    }
  else if ((i.raw & MRS_MASK) == MRS_SIG)
    {
      DecodeMRS();
    }
  else if ((i.raw & BRANCH_MASK) == BRANCH_SIG)
    {
      DecodeBranch();
    }
  else if ((i.raw & MULT_MASK) == MULT_SIG)
    {
      DecodeMult();
    }
  else if ((i.raw & HWT_MASK) == HWT_SIG)
    {
      DecodeHWT();
    }
  else if ((i.raw & DPI_MASK) == DPI_SIG)
    {
      DecodeDPI();
    }
  else if ((i.raw & SWT_MASK) == SWT_SIG)
    {
      DecodeSWT();
    }
  else if ((i.raw & MRT_MASK) == MRT_SIG)
    {
      DecodeMRT();
    }
  else if ((i.raw & SWI_MASK) == SWI_SIG)
    {
      DecodeSWI();
    }
  else if ((i.raw & CDO_MASK) == CDO_SIG)
    {
      DecodeCDO();
    }
  else if ((i.raw & CRT_MASK) == CRT_SIG)
    {
      DecodeCRT();
    }
  else if ((i.raw & CDT_MASK) == CDT_SIG)
    {
      DecodeCWT();
    }
  else
    {
      // Failed to decode it - generate an undefined instruction trap
      GenerateUDT();
    }
}


///////////////////////////////////////////////////////////////////////////////
// Decode*** - Indavidual decoders for instruction types
//
void CArmCore::DecodeDPI()
{
  CONTROL* c1;
  INST i;

  i.raw = m_iPipe[1];

  // First do a test for a special case.
  if (i.dpi1.rd == R_PC)
    {
      // This is really a branch instruction
      DecodeMovPC();
      return;
    }

  /* Make space for control path info and put it in place */
  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[3]);
  m_ctrlListNext[2] = NULL;
  CREATE_CONTROL(c1);

  // Do we need to pre-fetch rs (the shift amount to store in a register?) 
  if ((i.dpi1.hash == 0) && (i.dpi2.pad2 != 0))
    {
      // Prefetch our shift
      CONTROL* c;

      CREATE_CONTROL(c);
      m_ctrlListNext[0] = c;
      m_ctrlListNext[1] = c1;

      c->cond = (enum COND)i.dpi1.cond;
      c->updates = UPDATE_SR | UPDATE_IP;
      c->rm = (enum REGS)i.dpi3.rs;
      c->b = B_REG;
      c->ari = ARI_NONE;

      c1->cond = C_AL;
      c1->updates = UPDATE_PC;
    }
  else
    {
      m_ctrlListNext[0] = c1;
      m_ctrlListNext[1] = NULL;
      c1->cond = (enum COND)i.dpi1.cond;
      c1->updates = UPDATE_PC | UPDATE_IP;
    }

  c1->opcode = (enum OPCODE)i.dpi1.opcode;
  /* Don't update the register file if OP_TST, OP_TEQ, OP_CMP, OP_CMN */
  /* This is opcode = 10XX */
  if ((c1->opcode >> 2) != 0x2)
    c1->updates |= UPDATE_RD;

  /* Now fill in the blanks in c1. Do the generic parts of the inst first */
  c1->updates |= (i.dpi1.set == 1) ? UPDATE_FG : 0;
  c1->ari = ARI_INC;
  c1->rn = (enum REGS)i.dpi1.rn;
  c1->rd = (enum REGS)i.dpi1.rd;
  c1->shift_reg = FALSE;
  c1->bi = BI_NORM;
  c1->wr = FALSE;
  c1->rdt = RD_INST;

#ifdef NATIVE_CHECK
  if (i.dpi1.rd != R_PC)
    {
      c1->bNative = TRUE;
      c1->i = i;
    }
#endif

  /* How the fiddly bits */
  if (i.dpi1.hash == 1)
    {
      /* Immediate */
      c1->b = B_IMM1;
      c1->imm_mask = 0x000000FF;
      c1->shift_type = (enum SHIFT)S_ROR;
      c1->shift_dist = i.dpi1.rot * 2;
    }
  else
    {
      /* Use rm as the second register */
      c1->b = B_REG;
      c1->shift_type = (enum SHIFT)i.dpi2.type;
      c1->rm = (enum REGS)i.dpi2.rm;

      if (i.dpi2.pad2 == 0)
	{
	  c1->shift_dist = i.dpi2.shift;
	  if ((c1->shift_dist == 0) && (c1->shift_type == S_ROR))
	    c1->shift_type = S_RRX;
	}
      else
	{
	  c1->shift_reg = TRUE;
#ifdef NATIVE_CHECK
	  c1->rs = (enum REGS)i.dpi3.rs;
#endif
	}
    }
}

void CArmCore::DecodeBranch()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  /* Make space for control path info and put it in place */
  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[4]);
  m_ctrlListNext[3] = NULL;

  /* First stage of branch - create the new address */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.branch.cond;
  c->updates = UPDATE_IP;
  c->bi = BI_NORM;
  c->ari = ARI_ALU;
  c->b = B_IMM1;
  c->opcode = OP_ADD;
  c->shift_type = S_LSL;
  c->shift_dist = 2;
  c->imm_mask = 0x00FFFFFF;
  c->bSign = TRUE;
  c->shift_reg = FALSE;
  c->rn = R_PC;
  c->wr = FALSE;
  c->rdt = RD_INST;

  /* Second stage of branch - write back link register if bl */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_IP;
  if (i.branch.link != 0)
    c->updates |= UPDATE_RD;
  c->bi = BI_NORM;
  c->ari = ARI_INC;
  c->b = B_REG;
  c->opcode = OP_MOV;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->shift_reg = FALSE;
  c->rm = R_PC;
  c->rd = R_LR;
  c->wr = FALSE;
  c->rdt = RD_INST;

  /* Third stage of branch - move the link register back four */
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->updates = UPDATE_PC | UPDATE_IP;
  if (i.branch.link != 0)
    c->updates |= UPDATE_RD;
  c->ai = AI_MAGIC;
  c->aMagic = 4;
  c->rm = R_LR;
  c->rd = R_LR;
  c->b = B_REG;
  c->ari = ARI_INC;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->shift_reg = FALSE;
  c->bi = BI_NORM;
  c->opcode = OP_RSB;
  c->wr = FALSE;
  c->rdt = RD_INST;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMovPC - This is essentially a DPI instruction that targets the 
//               PC. At some point DecodeDPI and DecodeMovPC should
//               probably be merged.
//
void CArmCore::DecodeMovPC()
{
  CONTROL* c;
  INST i;

  int nCtrl = 0;
  enum COND init;

  i.raw = m_iPipe[1];

  /* Make space for control path info and put it in place */
  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[5]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 5);


  /* Stage 1 (possibly two) - just like a normal data op */
  
  // Do we need to pre-fetch rs (the shift amount to store in a register?) 
  init = (enum COND)i.dpi1.cond;
  if ((i.dpi1.hash == 0) && (i.dpi2.pad2 != 0))
    {
      // Prefetch our shift
      CREATE_CONTROL(c);
      m_ctrlListNext[nCtrl++] = c;

      c->cond = (enum COND)i.dpi1.cond;
      init = C_AL;

      c->updates = UPDATE_SR | UPDATE_IP;
      c->rm = (enum REGS)i.dpi3.rs;
      c->b = B_REG;
      c->ari = ARI_NONE;

      c->cond = C_AL;
      c->updates = UPDATE_PC;
    }
  
  CREATE_CONTROL(c);
  m_ctrlListNext[nCtrl++] = c;
  
  c->cond = init;
  c->opcode = (enum OPCODE)i.dpi1.opcode;
  c->updates = UPDATE_IP;

  /* Now fill in the blanks in c. Do the generic parts of the inst first */
  c->updates |= (i.dpi1.set == 1) ? UPDATE_FG : 0;
  c->ari = ARI_ALU;
  c->rn = (enum REGS)i.dpi1.rn;
  c->rd = (enum REGS)i.dpi1.rd;
  c->shift_reg = FALSE;
  c->bi = BI_NORM;
  c->wr = FALSE;
  c->rdt = RD_INST;

  /* How the fiddly bits */
  if (i.dpi1.hash == 1)
    {
      /* Immediate */
      c->b = B_IMM1;
      c->imm_mask = 0x000000FF;
      c->shift_type = (enum SHIFT)S_ROR;
      c->shift_dist = i.dpi1.rot * 2;
    }
  else
    {
      /* Use rm as the second register */
      c->b = B_REG;
      c->shift_type = (enum SHIFT)i.dpi2.type;
      c->rm = (enum REGS)i.dpi2.rm;

      if (i.dpi2.pad2 == 0)
	{
	  c->shift_dist = i.dpi2.shift;
	}
      else
	{
	  c->shift_reg = TRUE;
	  //c1->rs = (enum REGS)i.dpi3.rs;
	}
    }


  /* Second stage of branch */
  CREATE_CONTROL(c);
  m_ctrlListNext[nCtrl++] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_IP;
#if 0
  if (i.dpi1.set == 1)
    c->updates = UPDATE_RD;
#endif
  c->bi = BI_NORM;
  c->ari = ARI_INC;
  c->b = B_REG;
  c->opcode = OP_MOV;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->shift_reg = FALSE;
  c->rm = R_PC;
  c->rd = R_LR;
  c->wr = FALSE;
  c->rdt = RD_INST;

  /* Final stage of branch - Change to user mode if necessary */
  CREATE_CONTROL(c);
  m_ctrlListNext[nCtrl++] = c;

  c->cond = C_AL;
  c->updates = UPDATE_PC | UPDATE_IP;
#if 0
  if (i.dpi1.set == 1)
    c->updates = UPDATE_RD;
#endif
  c->bi = BI_NORM;
  c->ari = ARI_INC;
  c->b = B_REG;
  c->si = SI_ONE;
  c->shift_type = S_LSL;
  c->shift_dist = 2;
  c->shift_reg = FALSE;
  c->rn = R_LR;
  c->rd = R_LR;
  c->wr = FALSE;
  c->rdt = RD_INST;
  //c->mode = (i.dpi1.set == 1) ? m_prevMode : M_PREV;
      // Need to get the mode from the spsr
      if (i.dpi1.set != 0)
	{
	  uint32_t mode;
	  switch (m_mode)
	    {
	    case M_IRQ: mode = m_regsIrq[2]; break;
	    case M_FIQ: mode = m_regsFiq[7]; break;
	    case M_SVC: mode = m_regsSvc[2]; break;
	    case M_ABORT: mode = m_regsAbort[2]; break;
	    case M_UNDEF: mode = m_regsUndef[2]; break;
	    case M_PREV : case M_USER : case M_SYSTEM : 
	      // Other code should prevent this happening
	      mode = 0xDEAD001F; break;
	    }
	  c->mode = (enum MODE)(mode & 0x1F);
	}
      else
	{
	  c->mode = M_PREV;
	}         
    
}


void CArmCore::DecodeSWI()
{
  INST i;

  i.raw = m_iPipe[1];

#ifdef SWARM_SWI_HANDLER
  // Test to see if bit 23 is set - if so then this is a user swi. In this 
  // case we generate a noop in the stream and the Cycle function will execute
  // the user's code at the right time.
  if ((i.raw & 0x0F800000) == 0x0F800000)
    {
      uint32_t mod_swi = i.raw & 0x007FFFFF;

      // Is this a valid SWI call to make?
      if ((mod_swi >= MAX_SWI_CALL) || (m_swiCalls[mod_swi] == NULL))
	create_vector(M_UNDEF, VEC_UNDEF, m_ctrlListNext);
      else
	{
	  // Yup
	  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
	  m_ctrlListNext[0] = create_noop();
	  m_ctrlListNext[0]->bSwi = TRUE;
	  m_ctrlListNext[1] = NULL;
	}
    }
  else
#endif	  
    create_vector(M_SVC, V_SWI, m_ctrlListNext);
}

void CArmCore::DecodeSWTStore()
{
  CONTROL* c;
  INST i;
  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[3]);
  m_ctrlListNext[2] = NULL;

  /* Do the first stage - this gets the address ready for the mem transfer */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.swt1.cond;
  c->updates = UPDATE_PC | UPDATE_IP;
  c->ari = ARI_ALU;
  c->shift_reg = FALSE;
  c->bi = (i.swt1.p == 1) ? BI_NORM : BI_NULL;
  c->imm_mask = 0x00000FFF;
  c->rn = (enum REGS)i.swt1.rn;
  c->opcode = (i.swt1.u == 1) ? OP_ADD : OP_SUB;
  if (i.swt1.hash == 0)
    {
      /* Immediate offset */
      c->b = B_IMM1;
      c->shift_type = S_LSL;
      c->shift_dist = 0;
    }
  else
    {
      c->b = B_REG;
      c->shift_type = (enum SHIFT)i.swt2.type;
      c->shift_dist = i.swt2.shift;
      c->rm = (enum REGS)i.swt2.rm;
    }
  c->wr = TRUE;
  c->rdt = RD_DATA;
#if 0
  if (i.swt1.b == 1)
    c->bw = TRUE;     
#else
  c->rw = i.swt1.b == 1 ? RW_BYTE : RW_WORD;
#endif

  /* Second stage - data is written out and auto inc if necessary */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_DO;
  //c->bw = (i.swt1.b == 1);
  if (((i.swt1.p == 1) && (i.swt1.wb == 1)) || (i.swt1.p == 0))
    c->updates |= UPDATE_RD;
  c->rn = (enum REGS)i.swt1.rn; 
  c->bi = BI_HACK;
  c->rd = (enum REGS)i.swt1.rn;
  c->opcode = (i.swt1.u == 1) ? OP_ADD : OP_SUB;
  c->b = B_REG;
  c->rm = (enum REGS)i.swt1.rd;
  //c->rn = (enum REGS)i.swt1.rd; XXX ???
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->ari = ARI_REG;
  c->wr = FALSE;
  c->rdt = RD_INST;
}


void CArmCore::DecodeSWTLoad()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[6]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 6);

  /* Do the first stage - this gets the address ready for the mem transfer */
  /* This is the same as for a store */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.swt1.cond;
  c->updates = UPDATE_PC | UPDATE_IP;
  c->ari = ARI_ALU;
  c->shift_reg = FALSE;
  c->opcode = (i.swt1.u == 1) ? OP_ADD : OP_SUB;
  c->imm_mask = 0x00000FFF;
  c->wr = FALSE;
  c->rdt = RD_DATA;

  c->rn = (enum REGS)i.swt1.rn;
  c->ai = AI_NORM;
  c->bi = (i.swt1.p == 1) ? BI_NORM : BI_NULL;
#if 0
  c->bw = i.swt1.b; 
#else
  c->rw = i.swt1.b == 1 ? RW_BYTE : RW_WORD; 
#endif

  if (i.swt1.hash == 0)
    {
      /* Immediate offset */
      c->b = B_IMM1;
      c->shift_type = S_LSL;
      c->shift_dist = 0;
    }
  else
    {
      c->b = B_REG;
      c->shift_type = (enum SHIFT)i.swt2.type;
      c->shift_dist = i.swt2.shift;
      c->rm = (enum REGS)i.swt2.rm;
    }

  /* Second stage - data is written out and auto inc if necessary */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_DI;
  if (((i.swt1.p == 1) && (i.swt1.wb == 1)) || (i.swt1.p == 0))
    c->updates |= UPDATE_RD;
  c->rn = (enum REGS)i.swt1.rn;
  c->bi = BI_HACK;
  c->rd = (enum REGS)i.swt1.rn;
  c->opcode = (i.swt1.u == 1) ? OP_ADD : OP_SUB;
  c->b = B_REG;
  //c->rn = (enum REGS)i.swt1.rd;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->ari = ARI_REG;
  c->wr = FALSE;
  c->rdt = RD_DATA;

  /* Third stage - bring the data in from the Data In register. If the 
   * destination if the program counter then shift it into the address
   * resiter too, and start to flush the pipeline. */
  
#if 0
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->updates = UPDATE_RD;
  if (i.swt1.rd == R_PC)
    c->updates |= UPDATE_IP | UPDATE_PC;
  c->bi = BI_NORM;
  c->rd = (enum REGS)i.swt1.rd;
  c->opcode = OP_MOV;
  c->b = B_DIN;
  c->ari = i.swt1.rd != R_PC ? ARI_REG : ARI_ALU;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->wr = FALSE;
  c->rdt = RD_INST;

  /* Was rd the program counter? If so we now branch */
  if (i.swt1.rd == R_PC)
    {
      /* Second stage of branch - note no need to link */
      CREATE_CONTROL(c);
      m_ctrlListNext[3] = c;
  
      c->cond = C_AL;
      c->updates = UPDATE_IP;
      c->bi = BI_NORM;      
  c->ari = ARI_INC;
  c->b = B_REG;
  c->opcode = OP_MOV;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->shift_reg = FALSE;
  c->rm = R_PC;
  c->rd = R_LR;
  c->wr = FALSE;
  c->rdt = RD_INST;

  /* Third stage of branch - move the link register back four */
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->updates = UPDATE_PC | UPDATE_IP;
  if (i.branch.link != 0)
    c->updates |= UPDATE_RD;
  c->ai = AI_MAGIC;
  c->aMagic = 4;
  c->rm = R_LR;
  c->rd = R_LR;
  c->b = B_REG;
  c->ari = ARI_INC;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->shift_reg = FALSE;
  c->bi = BI_NORM;
  c->opcode = OP_RSB;
  c->wr = FALSE;
  c->rdt = RD_INST;
    }
#else

  if (i.swt1.rd != R_PC)
    {    
      CREATE_CONTROL(c);
      m_ctrlListNext[2] = c;
      
      c->cond = C_AL;
      c->updates = UPDATE_RD;
      c->opcode = OP_MOV;
      c->shift_reg = FALSE;
      c->shift_type = S_LSL;
      c->shift_dist = 0;
      c->bi = BI_NORM;
      c->b = B_DIN;
      c->rd = (enum REGS)i.swt1.rd;
      c->ari = ARI_REG;
      c->wr = FALSE;
      c->rdt = RD_INST;
    }
  else
    {
      CREATE_CONTROL(c);
      m_ctrlListNext[2] = c;
      
      c->cond = C_AL;
      c->b = B_DIN;
      c->bi = BI_NORM;
      c->shift_reg = FALSE;
      c->shift_type = S_LSL;
      c->shift_dist = 0;
      c->opcode = OP_MOV;
      c->updates = 0;
      c->ari = ARI_ALU;
      c->wr = FALSE;
      c->rdt = RD_INST;

      CREATE_CONTROL(c);
      m_ctrlListNext[3] = c;

      c->cond = C_AL;
      c->updates = UPDATE_IP;
      c->ari = ARI_INC;
      c->wr = FALSE;
      c->rdt = RD_INST;

      CREATE_CONTROL(c);
      m_ctrlListNext[4] = c;

      c->cond = C_AL;
      c->updates = UPDATE_PC | UPDATE_IP;
      c->ari = ARI_INC;
      c->wr = FALSE;
      c->rdt = RD_INST;
    }
#endif
}


void CArmCore::DecodeSWT()
{
  INST i;
  i.raw = m_iPipe[1];

  if (i.swt1.ls == 1)
    DecodeSWTLoad();
  else
    DecodeSWTStore();
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CArmCore::DecodeHWT()
{
  INST i;
  i.raw = m_iPipe[1];
  
  if (i.hwt.ls == 1)
    DecodeHWTLoad();
  else
    DecodeHWTStore();
}


///////////////////////////////////////////////////////////////////////////////
// DecodeHWTLoad - I'm assuming that you're not going to load into the PC.
//                 If you do it will succeed without causing a branch and then
//                 the system will most likely fail, or the PC will just be
//                 written over with the increment of whats in the address 
//                 register. Look, just don't bother okay?
//
void CArmCore::DecodeHWTLoad()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[4]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 4);

  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  /* Stage 1 - generate address */
  c->cond = (enum COND)i.hwt.cond;
  c->updates = UPDATE_PC | UPDATE_IP;
  c->ari = ARI_ALU;
  c->shift_reg = FALSE;
  c->opcode = (i.swt1.u == 1) ? OP_ADD : OP_SUB;
  c->imm_mask = 0x000000FF;
  c->wr = FALSE;
  c->rdt = RD_DATA;

  c->rn = (enum REGS)i.hwt.rn;
  c->ai = AI_NORM;
  c->bi = (i.hwt.p == 1) ? BI_NORM : BI_NULL;
  c->rw = (i.hwt.h == 0) ? RW_BYTE : RW_HALFWORD;

  if (i.hwt.hash == 0)
    {
      c->b = B_REG;
      c->rm = (enum REGS)i.hwt.rm;
      c->shift_type = S_LSL;
      c->shift_dist = 0;
    }
  else
    {
      c->b = B_IMM2;
      c->shift_type = S_LSL;
      c->shift_dist = 0;      
    }
  
  /* Second stage - data is written out and auto inc if necessary */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_DI;
  if (((i.swt1.p == 1) && (i.swt1.wb == 1)) || (i.swt1.p == 0))
    c->updates |= UPDATE_RD;
  c->rn = (enum REGS)i.hwt.rn;
  c->bi = BI_HACK;
  c->rd = (enum REGS)i.hwt.rn;
  c->opcode = (i.hwt.u == 1) ? OP_ADD : OP_SUB;
  c->b = B_REG;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->ari = ARI_REG;
  c->wr = FALSE;
  c->rdt = RD_DATA;

  /* Final stage - write the data to the register file */
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->updates = UPDATE_RD;
  c->opcode = OP_MOV;
  c->shift_reg = S_LSL;
  c->shift_dist = 0;
  c->bi = BI_NORM;
  c->b = B_DIN;
  c->rd = (enum REGS)i.swt1.rd;
  c->ari = ARI_REG;
  c->wr = FALSE;
  c->rdt = RD_INST;
  c->bSign = (i.hwt.s == 1) ? TRUE : FALSE;
  c->imm_mask = (i.hwt.h == 0) ? 0x000000FF : 0x0000FFFF;
}


void CArmCore::DecodeHWTStore()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[3]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 3);

  /* Do the first stage - this gets the address ready for the mem transfer */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.hwt.cond;
  c->updates = UPDATE_PC | UPDATE_IP;
  c->ari = ARI_ALU;
  c->shift_reg = FALSE;
  c->bi = (i.hwt.p == 1) ? BI_NORM : BI_NULL;
  c->imm_mask = 0x000000FF;
  c->rn = (enum REGS)i.hwt.rn;
  c->opcode = (i.hwt.u == 1) ? OP_ADD : OP_SUB;
  if (i.hwt.hash == 1)
    {
      /* Immediate offset */
      c->b = B_IMM2;
      c->shift_type = S_LSL;
      c->shift_dist = 0;
    }
  else
    {
      c->b = B_REG;
      c->shift_type = S_LSL;
      c->shift_dist = 0;
      c->rm = (enum REGS)i.hwt.rm;
    }
  c->wr = TRUE;
  c->rdt = RD_DATA;
  c->rw = (i.hwt.h == 0) ? RW_BYTE : RW_HALFWORD;

  
  /* Second stage - data is written out and auto inc if necessary */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_DO;
  if (((i.hwt.p == 1) && (i.hwt.wb == 1)) || (i.hwt.p == 0))
    c->updates |= UPDATE_RD;
  c->rn = (enum REGS)i.hwt.rn; 
  c->bi = BI_HACK;
  c->rd = (enum REGS)i.hwt.rn;
  c->opcode = (i.hwt.u == 1) ? OP_ADD : OP_SUB;
  c->b = B_REG;
  c->rm = (enum REGS)i.hwt.rd;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->ari = ARI_REG;
  c->wr = FALSE;
  c->rdt = RD_INST;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMRS - Decodes the status register to general register move.
//
void CArmCore::DecodeMRS()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
  m_ctrlListNext[1] = 0;

  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.mrs.cond;
  c->updates = UPDATE_RD | UPDATE_IP | UPDATE_PC;
  c->ari = ARI_INC;
  c->opcode = OP_MOV;
  c->shift_dist = 0;
  c->shift_type = S_LSL;
  c->b = (i.mrs.which == 0) ? B_CPSR : B_SPSR;
  c->rd = (enum REGS)i.mrs.rd;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMSR - Decodes a general register to status register transfer.
//
void CArmCore::DecodeMSR()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
  m_ctrlListNext[1] = 0;

  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;
  
  c->cond = (enum COND)i.msr1.cond;
  c->updates = UPDATE_IP | UPDATE_PC;
  c->updates |= (i.msr1.which == 0) ? UPDATE_CS : UPDATE_SS;
  c->ari = ARI_INC;
  c->imm_mask = 0x000000FF;
  c->opcode = OP_MOV;
  if (i.msr1.hash == 0)
    {
      c->b = B_REG;
      c->rm = (enum REGS)i.msr2.rm;
      c->psr_mask = 0x00000000;
      for (int j = 0; j < 4; j++)
	if (((i.msr2.field >> j) & 0x1) == 0x1)
	  c->psr_mask |= (0xFF << (j * 8));
    }
  else
    {
      // XXX: Needs work!!!
      c->b = B_IMM1;
      c->shift_dist = i.msr1.rot;
      c->shift_type = S_ROR;
      c->psr_mask = 0xFF000000;
    }
}


///////////////////////////////////////////////////////////////////////////////
// countbits - Returns the number of bits that are set to 1 in a value.
//
uint32_t countbits(uint32_t nVal)
{
  uint32_t res = 0;

  while (nVal != 0)
    {
      if (nVal & 0x00000001)
	res++;
      nVal = nVal >> 1;
    }
  
  return res;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMRTLoad - 
//
// * s bit - if load pc then  if s == 1 then load cpsr from spsr
//                                      else nothing
//           if !load pc then if s == 1 then load to user mode registers
//                                      else load normal registers
//
void CArmCore::DecodeMRTLoad()
{
  CONTROL* c;
  INST i;
  bool_t bUserHack = 0; // Load user mode from privilege mode?

  i.raw = m_iPipe[1];
  
  uint32_t nCount = countbits(i.mrt.list);
  
  bUserHack = (((i.mrt.list & 0x8000) == 0x0000) && (i.mrt.s == 1));

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[22]); // Worst case allocation
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 22);

  // Generate initial address
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.mrt.cond;
  c->rm = (enum REGS)i.mrt.rn;
  c->ai = AI_MAGIC;
  c->opcode = OP_ADD;
  if (i.mrt.u == 0)
    {
      c->aMagic = (nCount * 4);
      c->aMagic += i.mrt.p == 1 ? 0 : 4;
      //c->aMagic *= -1;
      c->aMagic = (~c->aMagic) + 1;
    }
  else
    {
      c->aMagic = i.mrt.p == 1 ? 4 : 0;
    }
  c->bi = BI_NORM;
  c->b = B_REG;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->ari = ARI_ALU;
  c->updates = UPDATE_IP | UPDATE_PC;
  c->wr = FALSE;
  c->rdt = RD_DATA;
 
  // Generate the second stage - this is the writeback
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_DI;
  if (i.mrt.wb == 1)
    {
      c->updates |= UPDATE_RD;
      c->ai = AI_MAGIC;
      c->bi = BI_HACK;
      c->opcode = OP_ADD;
      //c->aMagic = i.mrt.p == 1 ? 4 : 4;
      c->aMagic = (nCount * 4);
      if (i.mrt.u == 0)	
	c->aMagic = (~c->aMagic) + 1;
	//c->aMagic *= -1;	
      c->rd = (enum REGS)i.mrt.rn;
    }
  c->b = B_REG;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->wr = FALSE;
  c->rdt = RD_DATA;
  c->ari = nCount > 1 ? ARI_INC : ARI_REG;

  uint32_t reg = 0;  
  uint32_t list = i.mrt.list;

  for (uint32_t j = 1; j < nCount; j++)
    {
      CREATE_CONTROL(c);
      m_ctrlListNext[j + 1] = c;

      c->cond = C_AL;
      c->updates = UPDATE_RD | UPDATE_DI;
      c->opcode = OP_MOV;
      c->shift_reg = FALSE;
      c->shift_type = S_LSL;
      c->shift_dist = 0;
      c->bi = BI_NORM;
      c->b = B_DIN;
      // Find the next register to store
      while ((list & 0x00000001) == 0)
	{
	  list = list >> 1;
	  reg++;
	}
      c->rd = (enum REGS)reg;
      c->regBankWrite = bUserHack ? RB_USER : RB_CURRENT;
      list &= 0xFFFFFFFE;
      
      if (j < nCount -1 )
	  c->ari = ARI_INC;
      else
	  c->ari = ARI_REG;

      c->wr = FALSE;
      c->rdt = RD_DATA;
    }

  // Final stage, do the last write to a register and return the 
  // program counter. Or, if the last register is the program 
  // counter then we need to add the branch code to flush the 
  // pipeline.

  // Find the next register to store
  while ((list & 0x00000001) == 0)
    {
      list = list >> 1;
      reg++;
    }

  if (reg != R_PC)
    {    
      CREATE_CONTROL(c);
      m_ctrlListNext[1 + nCount] = c;
      
      c->cond = C_AL;
      c->updates = UPDATE_RD;
      c->opcode = OP_MOV;
      c->shift_reg = FALSE;
      c->shift_type = S_LSL;
      c->regBankWrite = bUserHack ? RB_USER : RB_CURRENT;
      c->shift_dist = 0;
      c->bi = BI_NORM;
      c->b = B_DIN;
      c->rd = (enum REGS)reg;
      c->ari = ARI_REG;
      c->wr = FALSE;
      c->rdt = RD_INST;
    }
  else
    {
     //   if(i.mrt.s == 1)
//        {
//        	/* Trying to move SPSR to CPSR - HanishKVC 
//        	*  * May have to increase size alloted to m_ctrlList 
//        	*  * Also This is a temporary patch I added based on
//        	*    my preliminary analysis of SWARM code logic
//        	*/
//        	CREATE_CONTROL(c);
//        	m_ctrlListNext[4 + nCount] = c;
//        	c->cond = C_AL;
//        	c->b = B_SPSR;
//        	c->bi = BI_NORM;
//        	c->shift_reg = FALSE;
//        	c->shift_type = S_LSL;
//        	c->shift_dist = 0;
//        	c->opcode = OP_MOV;
//        	c->updates = UPDATE_CS;
//        	c->psr_mask = 0xFFFFFFFF;
//  	c->ari = ARI_NONE;
//        }

      CREATE_CONTROL(c);
      m_ctrlListNext[1 + nCount] = c;
      
      c->cond = C_AL;
      c->b = B_DIN;
      c->bi = BI_NORM;
      c->shift_reg = FALSE;
      c->shift_type = S_LSL;
      c->shift_dist = 0;
      c->opcode = OP_MOV;
      c->updates = 0;
      c->ari = ARI_ALU;
      c->wr = FALSE;
      c->rdt = RD_INST;

      CREATE_CONTROL(c);
      m_ctrlListNext[2 + nCount] = c;

      c->cond = C_AL;
      c->updates = UPDATE_IP;
      c->ari = ARI_INC;
      c->wr = FALSE;
      c->rdt = RD_INST;

      CREATE_CONTROL(c);
      m_ctrlListNext[3 + nCount] = c;

      c->cond = C_AL;
      c->updates = UPDATE_PC | UPDATE_IP;
      c->ari = ARI_INC;
      c->wr = FALSE;
      c->rdt = RD_INST;
      // Need to get the mode from the spsr
      if (i.mrt.s != 0)
	{
	  uint32_t mode;
	  switch (m_mode)
	    {
	    case M_IRQ: mode = m_regsIrq[2]; break;
	    case M_FIQ: mode = m_regsFiq[7]; break;
	    case M_SVC: mode = m_regsSvc[2]; break;
	    case M_ABORT: mode = m_regsAbort[2]; break;
	    case M_UNDEF: mode = m_regsUndef[2]; break;
	    case M_PREV : case M_USER : case M_SYSTEM : 
	      // Other code should prevent this happening
	      mode = 0xDEAD001F; break;
	    }
	  c->mode = (enum MODE)(mode & 0x1F);
	}
      else
	{
	  c->mode = M_PREV;
	}         
    }
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMRTStore - 
//
// * s bit - if s == 0 then store working register
//           if s == 1 then store user mode registers
//
void CArmCore::DecodeMRTStore()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];
  
  uint32_t nCount = countbits(i.mrt.list);

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[18]); // Worst case allocation
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 18);

  // Generate initial address
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.mrt.cond;
  c->rm = (enum REGS)i.mrt.rn;
  c->ai = AI_MAGIC;
  // Is this going up or down?
  c->opcode = OP_ADD;
  if (i.mrt.u == 0)
    {
      c->aMagic = (nCount * 4);
      c->aMagic += i.mrt.p == 1 ? 0 : 4;
      //c->aMagic *= -1;
      c->aMagic = (~c->aMagic) + 1;
    }
  else
    {
      c->aMagic = i.mrt.p == 1 ? 4 : 0;
    }
  c->bi = BI_NORM;
  c->b = B_REG;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->ari = ARI_ALU;
  c->updates = UPDATE_IP | UPDATE_PC;
  c->wr = TRUE;
  c->rdt = RD_DATA;
 
  uint32_t reg = 0;
  uint32_t list = i.mrt.list;

  for (uint32_t j = 1; j <= nCount; j++)
    {
      CREATE_CONTROL(c);
      m_ctrlListNext[j] = c;

      c->cond = C_AL;
      c->updates = UPDATE_DO; 
      c->b = B_REG;
      c->shift_reg = FALSE;
      c->shift_type = S_LSL;
      c->shift_dist = 0;

      // Find the next register to store
      while ((list & 0x00000001) == 0)
	{
 	  list = list >> 1;
	  reg++;
	}
      c->rm = (enum REGS)reg;
      c->regBankRead = (i.mrt.s == 1) ? RB_USER : RB_CURRENT;
      list &= 0xFFFFFFFE;
       
      // If this isn't the last stage then generate a new address
      if (j != nCount)
	{
	  c->ari = ARI_INC;
	  c->wr = TRUE;
	  c->rdt = RD_DATA;
	}
      else
	{
	  c->ari = ARI_REG;
	  c->wr = FALSE;
	  c->rdt = RD_INST;
	}

      // Is this the writeback stage? 
      if ((i.mrt.wb == 1) && (j == 1))
	{
	  c->updates |= UPDATE_RD;
	  c->ai = AI_MAGIC;
	  c->bi = BI_HACK;
	  c->opcode = OP_ADD;
	  //c->aMagic = 4 + (nCount * 4);
	  c->aMagic = nCount * 4;
	  if (i.mrt.u == 0)
	    c->aMagic = (~c->aMagic) + 1;
	    //c->aMagic *= -1;	
	  c->rd = (enum REGS)i.mrt.rn;
	}
    }
}


void CArmCore::DecodeMRT()
{
  INST i;
  i.raw = m_iPipe[1];

  // Sanity check. If the list of registers is empty then just insert a no-op.
  if (i.mrt.list == 0)
    {
      m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
      m_ctrlListNext[0] = create_noop();
      m_ctrlListNext[1] = NULL;
      return;
    }

  if (i.mrt.ls == 1)
    DecodeMRTLoad();
  else
    DecodeMRTStore();
}


#ifdef ARM6
///////////////////////////////////////////////////////////////////////////////
// DecodeMult - This is weird, as we generate 2 control structures, but the
//              second one will be updated at run time for each additional
//              cycle.
//
//              Currently we only support 32 bit multiplies.
//
void CArmCore::DecodeMult()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  if ((i.mult.opcode >> 1) != 0)
    {
      // We don't support this! Currently just generate a noop      
      create_vector(M_UNDEF, VEC_UNDEF, m_ctrlListNext);
      return;
    }

  // There can be up to 17 stages theoretically in a multiply,
  // plus startup plus NULL = 19.
  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[20]); 
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 20);

  // First stage is set up - used to get the multiplicand into the 
  // shift reg. This is done by putting it on the B bus.
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;
  c->cond = (enum COND)i.mult.cond;
  c->updates = UPDATE_IP | UPDATE_PC;
  c->opcode = OP_MOV;
  c->ari = ARI_INC;
  c->mulStage = MS_ONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rs;
  c->b = B_REG;

  // First stage is always executed, though subject to the 
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  if (((MUL_OP)i.mult.opcode) == M_MUL)
    {
      // Plain mult
      c->aMagic = 0x0;
      c->ai = AI_MAGIC;
    }
  else
    {
      // Multiply and accumulate
      c->ai = AI_NORM;
      c->rn = (enum REGS)i.mult.rn;
    }
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->mulStage = MS_TWO;
  c->updates = UPDATE_RD;
  c->ari = ARI_NONE;
  c->rm = (enum REGS)i.mult.rm;
  c->rd = (enum REGS)i.mult.rd;
  //c->rs = (enum REGS)i.mult.rs;
  c->b = B_REG;

  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;
  c->cond = C_AL;
  c->updates = UPDATE_RD;
  c->ari = ARI_NONE;
  c->mulStage = MS_LOOP;
  c->rm = (enum REGS)i.mult.rm;
  c->rd = (enum REGS)i.mult.rd;
  c->rn = (enum REGS)i.mult.rd;
}
#else
///////////////////////////////////////////////////////////////////////////////
// DecodeMult - Doesn't actually decodes - just passes the instruction off to
//              the correct decoder.
//
void CArmCore::DecodeMult()
{
  INST i;

  i.raw = m_iPipe[1];
  
  switch (i.mult.opcode)
    {
    case 0: // MUL
      DecodeMultMUL();
      break;
    case 1: // MLA
      DecodeMultMLA();
      break;
    case 4: // UMULL
      DecodeMultUMULL();
      break;
    case 5: // UMLAL
      DecodeMultUMLAL();
      break;
    case 6: // SMULL
      DecodeMultSMULL();
      break;
    case 7: // SMLAL
      DecodeMultSMLAL();
      break;
    default:
      create_vector(M_UNDEF, VEC_UNDEF, m_ctrlListNext);
      break;
    }
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMultMUL - 
//
void CArmCore::DecodeMultMUL()
{
  INST i;
  CONTROL* c;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[4]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 4);

  // Stage 1 - Load Rm and Rs. Note that it's a precondition of the 
  // multiply that the part sum and part carry registers are zero
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.mult.cond;
  c->updates = UPDATE_IP | UPDATE_PC | UPDATE_MS;
  c->opcode = OP_MOV;
  c->ari = ARI_INC;
  c->mulStage = MS_ONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rs;
  c->rn = (enum REGS)i.mult.rm;
  c->b = B_REG;

  // Stage 2 - contining the mult in it's own hardware
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_MS;
  c->ari = ARI_NONE;
  c->mulStage = MS_LOOP;
  c->rn = (enum REGS)i.mult.rm;

  // Stage 3 - Store the results
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_MR;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVELO;
  c->opcode = OP_ADD;
  c->rd = (enum REGS)i.mult.rd;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_LO;
  c->bi = BI_MULT_LO;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMultMLA - 
//
void CArmCore::DecodeMultMLA()
{
  INST i;
  CONTROL* c;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[5]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 5);

  // Stage 1 - load the rn into partsum/lo.
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.mult.cond;
  c->updates = UPDATE_IP | UPDATE_PC | UPDATE_ML;
  c->ari = ARI_INC;
  c->mulStage = MS_NONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rn;
  c->b = B_REG;

  // Stage 2 - Load Rm and Rs. Note that it's a precondition of the 
  // multiply that the part sum and part carry registers are zero
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_MS;
  c->opcode = OP_MOV;
  c->ari = ARI_NONE;
  c->mulStage = MS_ONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rs;
  c->rn = (enum REGS)i.mult.rm;
  c->b = B_REG;

  // Stage 3 - contining the mult in it's own hardware
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->updates = UPDATE_MS;
  c->ari = ARI_NONE;
  c->mulStage = MS_LOOP;
  c->rn = (enum REGS)i.mult.rm;

  // Stage 4 - Store the results
  CREATE_CONTROL(c);
  m_ctrlListNext[3] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_MR;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVELO;
  c->opcode = OP_ADD;
  c->rd = (enum REGS)i.mult.rd;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_LO;
  c->bi = BI_MULT_LO;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMultUMULL - 
//
void CArmCore::DecodeMultUMULL()
{
  INST i;
  CONTROL* c;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[5]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 5);

  // Stage 1 - Load Rm and Rs. Note that it's a precondition of the 
  // multiply that the part sum and part carry registers are zero
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.mult.cond;
  c->updates = UPDATE_IP | UPDATE_PC | UPDATE_MS;
  c->opcode = OP_MOV;
  c->ari = ARI_INC;
  c->mulStage = MS_ONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rs;
  c->rn = (enum REGS)i.mult.rm;
  c->b = B_REG;

  // Stage 2 - contining the mult in it's own hardware
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_MS;
  c->ari = ARI_NONE;
  c->mulStage = MS_LOOP;
  c->rn = (enum REGS)i.mult.rm;

  // Stage 3 - Store the results
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_FG;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVELO;
  c->opcode = OP_ADD;
  c->rd = (enum REGS)i.mult.rn;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_LO;
  c->bi = BI_MULT_LO;

  CREATE_CONTROL(c);
  m_ctrlListNext[3] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_MR;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVEHI;
  c->opcode = OP_ADC;
  c->rd = (enum REGS)i.mult.rd;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_HI;
  c->bi = BI_MULT_HI;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMultUMULL - 
//
void CArmCore::DecodeMultUMLAL()
{
  INST i;
  CONTROL* c;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[6]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 6);

  // Stage 0 - Load the rd:rn info the accumulator in the multiplier
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.mult.cond;
  c->updates = UPDATE_IP | UPDATE_PC | UPDATE_ML | UPDATE_MH;
  c->ari = ARI_INC;
  c->mulStage = MS_NONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rn;
  c->rn = (enum REGS)i.mult.rd;
  c->b = B_REG;
  

  // Stage 1 - Load Rm and Rs. Note that it's a precondition of the 
  // multiply that the part sum and part carry registers are zero
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_MS;
  c->opcode = OP_MOV;
  c->ari = ARI_NONE;
  c->mulStage = MS_ONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rs;
  c->rn = (enum REGS)i.mult.rm;
  c->b = B_REG;

  // Stage 2 - contining the mult in it's own hardware
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->updates = UPDATE_MS;
  c->ari = ARI_NONE;
  c->mulStage = MS_LOOP;
  c->rn = (enum REGS)i.mult.rm;

  // Stage 3 - Store the results
  CREATE_CONTROL(c);
  m_ctrlListNext[3] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_FG;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVELO;
  c->opcode = OP_ADD;
  c->rd = (enum REGS)i.mult.rn;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_LO;
  c->bi = BI_MULT_LO;

  CREATE_CONTROL(c);
  m_ctrlListNext[4] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_MR;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVEHI;
  c->opcode = OP_ADC;
  c->rd = (enum REGS)i.mult.rd;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_HI;
  c->bi = BI_MULT_HI;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMultUMULL - 
//
void CArmCore::DecodeMultSMULL()
{
  INST i;
  CONTROL* c;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[5]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 5);

  // Stage 1 - Load Rm and Rs. Note that it's a precondition of the 
  // multiply that the part sum and part carry registers are zero
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.mult.cond;
  c->updates = UPDATE_IP | UPDATE_PC | UPDATE_MS;
  c->opcode = OP_MOV;
  c->ari = ARI_INC;
  c->mulStage = MS_ONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rs;
  c->rn = (enum REGS)i.mult.rm;
  c->bSign = TRUE;
  c->b = B_REG;

  // Stage 2 - contining the mult in it's own hardware
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_MS;
  c->ari = ARI_NONE;
  c->mulStage = MS_LOOP;
  c->rn = (enum REGS)i.mult.rm;
  c->bSign = TRUE;

  // Stage 3 - Store the results
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_FG;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVELO;
  c->opcode = OP_ADD;
  c->rd = (enum REGS)i.mult.rn;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_LO;
  c->bi = BI_MULT_LO;
  c->bSign = TRUE;

  CREATE_CONTROL(c);
  m_ctrlListNext[3] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_MR;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVEHI;
  c->opcode = OP_ADC;
  c->rd = (enum REGS)i.mult.rd;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_HI;
  c->bi = BI_MULT_HI;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMultUMULL - 
//
void CArmCore::DecodeMultSMLAL()
{
  INST i;
  CONTROL* c;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[6]);
  memset(m_ctrlListNext, 0, sizeof(CONTROL*) * 6);

  // Stage 0 - Load the rd:rn info the accumulator in the multiplier
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.mult.cond;
  c->updates = UPDATE_IP | UPDATE_PC | UPDATE_ML | UPDATE_MH;
  c->ari = ARI_INC;
  c->mulStage = MS_NONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rn;
  c->rn = (enum REGS)i.mult.rd;
  c->b = B_REG;
  

  // Stage 1 - Load Rm and Rs. Note that it's a precondition of the 
  // multiply that the part sum and part carry registers are zero
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_MS;
  c->opcode = OP_MOV;
  c->ari = ARI_NONE;
  c->mulStage = MS_ONE;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->rm = (enum REGS)i.mult.rs;
  c->rn = (enum REGS)i.mult.rm;
  c->b = B_REG;
  c->bSign = TRUE;

  // Stage 2 - contining the mult in it's own hardware
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->updates = UPDATE_MS;
  c->ari = ARI_NONE;
  c->mulStage = MS_LOOP;
  c->rn = (enum REGS)i.mult.rm;
  c->bSign = TRUE;

  // Stage 3 - Store the results
  CREATE_CONTROL(c);
  m_ctrlListNext[3] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_FG;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVELO;
  c->opcode = OP_ADD;
  c->rd = (enum REGS)i.mult.rn;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_LO;
  c->bi = BI_MULT_LO;
  c->bSign = TRUE;

  CREATE_CONTROL(c);
  m_ctrlListNext[4] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD | UPDATE_MR;
  c->ari = ARI_NONE;
  c->mulStage = MS_SAVEHI;
  c->opcode = OP_ADC;
  c->rd = (enum REGS)i.mult.rd;
  c->rn = (enum REGS)i.mult.rm;
  c->ai = AI_MULT_HI;
  c->bi = BI_MULT_HI;
}


#endif


///////////////////////////////////////////////////////////////////////////////
// DecodeCRT - What we do here depends on which way the data is going.
//
void CArmCore::DecodeCRT()
{
  INST i;

  i.raw = m_iPipe[1];

  if (i.crt.ls == 1)
    DecodeMRC();
  else
    DecodeMCR();
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMRC - We assume that the coprocessor is internal, and that this
//             transfer
//
void CArmCore::DecodeMRC()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[4]);
  m_ctrlListNext[3] = NULL;

  /* Stage one - ask the coprocessor to do it's stuff */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;  

  c->cond = (enum COND)i.crt.cond;
  c->updates = UPDATE_IP | UPDATE_PC;
  c->cpStage = CP_INIT;
  c->ari = ARI_NONE;
  c->enout = 1;

  /* Stage two - move the data on the copro bus to the Data In reg */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_DI;
  c->cpStage = CP_NONE;
  c->ari = ARI_NONE;

  /* Stage three - Put the data in the target register */
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_RD;
  c->opcode = OP_MOV;
  c->shift_reg = FALSE;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->bi = BI_NORM;
  c->b = B_DIN;
  c->rd = (enum REGS)i.crt.rd;
  c->ari = ARI_REG;
  c->wr = FALSE;
  c->rdt = RD_INST;
  c->cpStage = CP_NONE;
}


///////////////////////////////////////////////////////////////////////////////
// DecodeMCR - We assume that the coprocessor is internal, and that this
//             transfer
//
void CArmCore::DecodeMCR()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[4]);
  m_ctrlListNext[3] = NULL;
  
  /* Stage one - Does nothing, waiting for copro to ack */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;
  
  c->cond = (enum COND)i.crt.cond;
  c->updates = UPDATE_PC | UPDATE_IP;
  c->cpStage = CP_INIT;
  c->ari = ARI_NONE;

  /* Stage two - Twiddle our thumbs */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_DO;
  c->cpStage = CP_NONE;
  c->ari = ARI_NONE;
  c->rm = (enum REGS)i.crt.rd;
  c->b = B_REG;
  c->wr = TRUE;
  c->enout = 1;

  /* State three - Get ready for the next instruction */
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->ari = ARI_REG;
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CArmCore::DecodeCDO()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[2]);
  m_ctrlListNext[2] = NULL;

  /* Stage one - We do nothing except ask the copro to go to work */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.cdo.cond;
  c->updates = UPDATE_PC | UPDATE_IP;
  c->cpStage = CP_INIT;
  c->ari = ARI_INC;

  /* Stage two - this might never happen */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->cpStage = CP_WAIT;
  c->ari = ARI_NONE;
}




///////////////////////////////////////////////////////////////////////////////
// 
//
void CArmCore::DecodeCWT()
{
  INST i;

  i.raw = m_iPipe[1];

  if (i.cdt.ls == 0)
    DecodeCWTStore();
  else
    DecodeCWTLoad();
}


///////////////////////////////////////////////////////////////////////////////
// DecodeCWTLoad - Loads a value from memory into a coprocessor register.
//
void CArmCore::DecodeCWTLoad()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[5]);
  m_ctrlListNext[4] = 0;

  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  /* Stage 1 - See if the coprocessor is awake */
  c->cond = (enum COND)i.cdt.cond;
  c->updates = UPDATE_PC;
  c->cpStage = CP_INIT;
  c->ari = ARI_NONE;

  /* Stage 2 - Assuming that we've found the coprocessor, generate the address to 
   * read the data from. */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;
  
  c->cond = C_AL;
  c->updates = UPDATE_IP; /* Can't update iPipe until we've read the imm value */
  c->ari = ARI_ALU;
  c->shift_reg = FALSE;
  c->opcode = (i.cdt.u == 1) ? OP_ADD : OP_SUB;
  c->imm_mask = 0x000000FF;
  c->wr = FALSE;
  c->rdt = RD_DATA;
  c->rn = (enum REGS)i.cdt.rn;
  c->ai = AI_NORM;
  c->bi = (i.cdt.p == 1) ? BI_NORM : BI_NULL;
  c->rw = RW_WORD;
  c->b = B_IMM1;
  c->shift_type = S_LSL;
  c->shift_dist = 0;

  /* Stage 3 - Data will be moved into the coprocessors's data in register. 
   * Here we can worry about the write back. */
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->updates = 0;
  if (((i.cdt.p == 1) && (i.cdt.wb == 1)) || (i.cdt.p == 0))
    c->updates |= UPDATE_RD;
  c->rn = (enum REGS)i.cdt.rn;
  c->bi = BI_HACK;
  c->rd = (enum REGS)i.cdt.rn;
  c->opcode = (i.cdt.u == 1) ? OP_ADD : OP_SUB;
  c->b = B_REG;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->ari = ARI_INC;
  c->wr = FALSE;
  c->rdt = RD_DATA;
  
  /* Stage 4 - We just idle here as the coprocessor moves the data from the 
   * data in register into its register file. */
  CREATE_CONTROL(c);
  m_ctrlListNext[3] = c;

  c->cond = C_AL;
  c->updates = 0;
  c->ari = ARI_REG;
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CArmCore::DecodeCWTStore()
{
  CONTROL* c;
  INST i;

  i.raw = m_iPipe[1];

  //m_ctrlListNext = (CONTROL**)TNEW(CONTROL*[4]);
  m_ctrlListNext[3] = 0;

  /* Stage 1 - See if the coprocessor is awake */
  CREATE_CONTROL(c);
  m_ctrlListNext[0] = c;

  c->cond = (enum COND)i.cdt.cond;
  c->updates = UPDATE_PC;
  c->cpStage = CP_INIT;
  c->ari = ARI_NONE;

  /* Stage 2 - Generate the address ready for transfer */
  CREATE_CONTROL(c);
  m_ctrlListNext[1] = c;

  c->cond = C_AL;
  c->updates = UPDATE_IP;
  c->ari = ARI_ALU;
  c->shift_reg = FALSE;
  c->bi = (i.cdt.p == 1) ? BI_NORM : BI_NULL;
  c->imm_mask = 0x000000FF;
  c->rn = (enum REGS)i.cdt.rn;
  c->opcode = (i.cdt.u == 1) ? OP_ADD : OP_SUB;
  c->b = B_IMM1;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->wr = TRUE;
  c->rdt = RD_DATA;

  /* Stage 3 - The coprocessor will now put the data into its data out
   * register. We generate the writeback address if necessary. */
  CREATE_CONTROL(c);
  m_ctrlListNext[2] = c;

  c->cond = C_AL;
  c->updates = 0;
  if (((i.cdt.p == 1) && (i.cdt.wb == 1)) || (i.cdt.p == 0))
    c->updates |= UPDATE_RD;
  c->rn = (enum REGS)i.cdt.rn; 
  c->bi = BI_HACK;
  c->rd = (enum REGS)i.cdt.rn;
  c->opcode = (i.cdt.u == 1) ? OP_ADD : OP_SUB;
  c->shift_type = S_LSL;
  c->shift_dist = 0;
  c->ari = ARI_REG;
  c->wr = FALSE;
  c->rdt = RD_INST;

  /* Know what I mean? */
}


///////////////////////////////////////////////////////////////////////////////
// GenerateUDT - Generates an undefined instruction trap. This is basically 
//               a glorified branch to 0x4 with a change in mode. i
//
void CArmCore::GenerateUDT()
{
  create_vector(M_UNDEF, VEC_UNDEF, m_ctrlListNext);
}


///////////////////////////////////////////////////////////////////////////////
// SetMode -
//
void CArmCore::SetMode(enum MODE mode)
{
  uint32_t old_cpsr, old_spsr = 0xdeadbeef;

  // Stage 1 - Make a note of where we came from.
  m_prevMode = m_mode;

  // Stage 2 - Save the current set of working registers back to the
  //           this mode's registers. We also back up the cpsr so we can 
  //           later move it into the new mode's spsr.
  old_cpsr = m_regsWorking[R_CPSR];
  if (m_mode == M_FIQ)
    {
      memcpy(m_regsFiq, &(m_regsWorking[R_R8]), sizeof(uint32_t) * 7);
      old_spsr = m_regsFiq[7];

      // If the mode was FIQ then I need to return the registers that
      // it shadows, that other modes don't
      memcpy(&(m_regsWorking[R_R8]), m_regsUser, sizeof(uint32_t) * 5);
    }
  else if ((m_mode == M_USER) || (m_mode == M_SYSTEM))
    {
      memcpy(m_regsUser, &(m_regsWorking[R_R8]), sizeof(uint32_t) * 7);
    }
  else
    {
      uint32_t* temp;
      switch (m_mode)
	{
	case M_IRQ : temp = m_regsIrq; break;
	case M_SVC : temp = m_regsSvc; break;
	case M_ABORT : temp = m_regsAbort; break;
	case M_UNDEF : temp = m_regsUndef; break;
	case M_USER: case M_FIQ : case M_SYSTEM: default: 
	  temp = NULL; break; // ???
	  // XXX: need to define an excpetion mechanism that allows me to 
	  // throw UDTs at moments like this
	}

      if (temp != NULL)
	{
	  memcpy(temp, &(m_regsWorking[R_SP]), sizeof(uint32_t) * 2);
	  old_spsr = temp[2];
	}
    }

  // At this point the working set of registers has user mode contents
  // in all but registers 13 and 14. 

  // Stage 3 - change the working set of registers to that of the new mode
  //           and store the old cpsr in the spsr of the new mode.
  m_mode = mode;
  uint32_t temp_cpsr = m_regsWorking[R_CPSR];
  if (m_mode == M_FIQ)
    {
      // Patch the working registers
      memcpy(&(m_regsWorking[R_R8]), m_regsFiq, sizeof(uint32_t) * 7);
      
      // Update SPSR_fiq
      m_regsFiq[7] = old_cpsr;

      // Generate the new cpsr value
      temp_cpsr &= 0xFFFFFFE0;        // Remove old mode
      temp_cpsr |= (uint32_t)m_mode;  // Set new mode
      temp_cpsr |= 0x000000C0;        // Disable FIQ and IRQ
    }
  else if ((m_mode == M_USER) || (m_mode == M_SYSTEM))
    {
      // Patch the working registers
      memcpy(&(m_regsWorking[R_SP]), &(m_regsUser[5]), sizeof(uint32_t) * 2);
     
      // No SPSR to update
      temp_cpsr = old_spsr;
    }
  else
    {
      uint32_t* temp;
      switch (m_mode)
	{
	case M_IRQ : temp = m_regsIrq; break;
	case M_SVC : temp = m_regsSvc; break;
	case M_ABORT : temp = m_regsAbort; break;
	case M_UNDEF : temp = m_regsUndef; break;
	case M_USER: case M_FIQ : case M_SYSTEM: default: 
	  temp = NULL; break; // ???
	  // XXX: need to define an excpetion mechanism that allows me to 
	  // throw UDTs at moments like this
	}

      if (temp != NULL)
	{
	  // Patch the working registers
	  memcpy(&(m_regsWorking[R_SP]), temp, sizeof(uint32_t) * 2);

	  // Update the SPSR for the new mode
	  temp[2] = old_cpsr;

	  // Generate the new cpsr value
	  temp_cpsr &= 0xFFFFFFE0;        // Remove old mode
	  temp_cpsr |= (uint32_t)m_mode;  // Set new mode
	  temp_cpsr |= 0x00000070;        // Disable IRQ
	}
    }
  m_regsWorking[R_CPSR] = temp_cpsr;
}

#if 0
///////////////////////////////////////////////////////////////////////////////
// SetMode - Sets the current mode for the processor. This includes munging the
//           register file appropriately.
//
void CArmCore::SetMode(enum MODE mode)
{
  // Make a note of where we came from.
  m_prevMode = m_mode;
  

  // First save the registers from the working set appropriately - basically
  // returning it to user mode. 
  if (m_mode == M_FIQ)
    {
      memcpy(m_regsFiq, &(m_regsWorking[R_R8]), sizeof(uint32_t) * 7);
      m_regsFiq[7] = m_regsWorking[R_CPSR];

      memcpy(&(m_regsWorking[R_R8]), m_regsUser, sizeof(uint32_t) * 7);
      m_regsWorking[R_CPSR] = m_regsUser[7];
    }
  else if ((m_mode == M_USER) || (m_mode == M_SYSTEM))
    {
      memcpy(m_regsUser, &(m_regsWorking[R_R8]), sizeof(uint32_t) * 7);
      m_regsUser[7] = m_regsWorking[R_CPSR];
    }
  else
    {
      uint32_t* temp;
      switch (m_mode)
	{
	case M_IRQ : temp = m_regsIrq; break;
	case M_SVC : temp = m_regsSvc; break;
	case M_ABORT : temp = m_regsAbort; break;
	case M_UNDEF : temp = m_regsUndef; break;
	case M_USER: case M_FIQ : case M_SYSTEM: default: 
	  temp = NULL; break; // ???
	  // XXX: need to define an excpetion mechanism that allows me to 
	  // throw UDTs at moments like this
	}

      if (temp != NULL)
	{
	  memcpy(temp, &(m_regsWorking[R_SP]), sizeof(uint32_t) * 2);
	  temp[2] = m_regsWorking[R_CPSR];
	  memcpy(m_regsUser, &(m_regsWorking[R_R8]), sizeof(uint32_t) * 5);
	  
	  memcpy(&(m_regsWorking[R_SP]), &(m_regsUser[5]), sizeof(uint32_t) * 2);
	  m_regsWorking[R_CPSR] = m_regsUser[7];
	}
    }

  // Now we're back in user mode, do we wish to go elsewhere?
  m_mode = mode;
  if ((m_mode == M_USER) || (m_mode == M_SYSTEM))
    {
    }
  else if (m_mode == M_FIQ)
    {
      memcpy(&(m_regsWorking[R_R8]), m_regsFiq, sizeof(uint32_t) * 7);
      m_regsWorking[R_CPSR] = m_regsFiq[7];
    }
  else
    {
      uint32_t* temp;
      switch (m_mode)
	{
	case M_IRQ : temp = m_regsIrq; break;
	case M_SVC : temp = m_regsSvc; break;
	case M_ABORT : temp = m_regsAbort; break;
	case M_UNDEF : temp = m_regsUndef; break;
	case M_USER: case M_FIQ : case M_SYSTEM: default: 
	  temp = NULL; break; // ???
	}
           
      if (temp != NULL)
	{
	  memcpy(&(m_regsWorking[R_SP]), temp, sizeof(uint32_t) * 2);
	  m_regsWorking[R_CPSR] = temp[7];
	}
    }

  // Set the mode bits in the CPSR
  uint32_t temp32 = m_regsWorking[R_CPSR];
  temp32 &= 0xFFFFFFE0;
  switch (m_mode)
    {
    case M_USER: temp32 |= MBITS_USER; break;
    case M_FIQ: temp32 |= MBITS_FIQ; break;
    case M_IRQ: temp32 |= MBITS_IRQ; break;
    case M_SVC: temp32 |= MBITS_SVC; break;
    case M_ABORT: temp32 |= MBITS_ABORT; break;
    case M_UNDEF: temp32 |= MBITS_UNDEF; break;
    case M_SYSTEM: temp32  |= MBITS_SYS; break;
    case M_PREV: break; // Not a real mode
    }

  // Disable interrupts if appropriate
  if (m_mode == M_FIQ)
    {
      temp32 |= 0x000000C0;
    }
  if (m_mode == M_IRQ)
    {
      temp32 |= 0x00000070;
    }

  m_regsWorking[R_CPSR] = temp32;
}
#endif


///////////////////////////////////////////////////////////////////////////////
// RegisterSWI - Adds a SWI call to the core. Takes in the actual 24 bit 
//               immediate (including bit 23 set) and a function pointer.
//               Will throw an exception if the SWI number is invalid or
//               already set.
//
void CArmCore::RegisterSWI(uint32_t swi_number, SWI_CALL* swi)
{
  uint32_t mod_swi = swi_number & 0x007FFFFF;

  if ((mod_swi == swi_number) || (mod_swi >= MAX_SWI_CALL))
    {
      // Either bit 23 wasn't set or the swi number was too high
      throw CSWIInvalidException();
    }
  if (m_swiCalls[mod_swi] != NULL)
    throw CSWISetException();

  m_swiCalls[mod_swi] = swi;
}


///////////////////////////////////////////////////////////////////////////////
// UnregisterSWI - Unregisters a SWI call.
//
void CArmCore::UnregisterSWI(uint32_t swi_number)
{
  uint32_t mod_swi = swi_number & 0x007FFFFF;

  if ((mod_swi != swi_number) || (mod_swi >= MAX_SWI_CALL))
    {
      // Either bit 23 wasn't set or the swi number was too high
      throw CSWIInvalidException();
    }

  m_swiCalls[mod_swi] = NULL;
}


///////////////////////////////////////////////////////////////////////////////
// DebugDump - Prints out all the internal information on the processor.
//             Useful for debuging both SWARM and apps running on top of it.
//
void CArmCore::DebugDump()
{
  char str[80];

  printf("-------------------------------------------------------------------------------\n");
  printf("SWARM Core debug dump\n\n");

  printf("Registers:");
  for (int j = 0; j < 4; j++)
    {
      for (int i = 0; i < 4; i++)
	printf("   0x%08X", m_regsWorking[i + (j * 4)]);
      printf("\n\t  ");
    }
  printf("   0x%08X", m_regsWorking[16]);

  if (m_mode == M_FIQ)
    printf("\tSPSR_%s[0x%08x]\n\n", mode_str[m_mode & 0xF], m_regsFiq[7]);
  else if ((m_mode == M_USER) || (m_mode == M_SYSTEM))
    printf("\n\n");
  else
    {
      uint32_t* temp;
      switch (m_mode)
	{
	case M_IRQ : temp = m_regsIrq; break;
	case M_SVC : temp = m_regsSvc; break;
	case M_ABORT : temp = m_regsAbort; break;
	case M_UNDEF : temp = m_regsUndef; break;
	case M_USER: case M_FIQ : case M_SYSTEM: default: 
	  temp = NULL; break; // ???
	}
           
      if (temp != NULL)
	printf("\tSPSR_%s[0x%08x]\n\n", mode_str[m_mode & 0xF], temp[2]);
    }

  printf("Instruction Pipe (top is current instruction):\n");
  for (int i = 2; i > 0; i--)
    {
      printf("\t0x%08X - ", m_iPipe[i]);
      memset(str, 0, 80);
      CDisarm::Decode(m_iPipe[i], str);
      printf("%s\n", str);
    }
  if (m_busCurrent != NULL)
    {
      printf("\t0x%08X - ", m_busCurrent->Din);
      memset(str, 0, 80);
      CDisarm::Decode(m_busCurrent->Din, str);
      printf("%s (suspect if bad address error)\n", str);
    }
  printf("Instruction Stage last executed - %d\n\n", m_nCtrlCur);
  
  printf("DIn reg = 0x%08X    DOut reg = 0x%08X   Addr reg = 0x%08X\n",
	    m_regDataIn, m_regDataOut, m_regAddr);

  printf("-------------------------------------------------------------------------------\n");
}


///////////////////////////////////////////////////////////////////////////////
// NextPC - Gets the next PC value
//
long CArmCore::NextPC()
{
  return m_regsWorking[15];
}
