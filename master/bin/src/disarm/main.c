/******************************************************************************
 * Copyright 2000 Michael Dales
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * file   main.c
 * author Michael Dales (michael@dcs.gla.ac.uk)
 * header n/a
 * info   Simple ARM code disassembler. Usage: disarm [bin file]
 *
 *****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <isa.h>

/* The condition codes */
const static char* cond[] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs",
			     "vc", "hi", "ls", "ge", "lt", "gt", "le",
			     "", "nv"};

const static char* regs[] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", 
			     "r7", "r8", "r9", "r10", "fp", "ip",
			     "sp", "lr", "pc"};

const static char* cregs[] = {"cr0", "cr1", "cr2", "cr3", "cr4", "cr5", 
			      "cr6", "cr7", "cr8", "cr9", "cr10", "cr11",
			      "cr12", "cr13", "cr14", "cr15"};

const static char* dpiops[] = {"and", "eor", "sub", "rsb", "add", "adc",
			       "sbc", "rsc", "tst", "teq", "cmp", "cmn",
			       "orr", "mov", "bic", "mvn"};

const static char* shift[] = {"lsl", "lsr", "asr", "asl", "ror", "rrx"};

const static char* mult[] = {"mul", "mla", "???", "???", 
			     "umull", "umla", "smull", "smlal"};

/* Multiplication opcodes */
#define MUL   0
#define MLA   1
#define UMULL 4
#define UMLAL 5
#define SMULL 6
#define SMLAL 7

/******************************************************************************
 * decode_branch - Decodes a branch instruction.
 */
void decode_branch(uint32_t i)
{
  INST inst;
  inst.raw = i;

  printf("%08x\tb", i);  
  
  if (inst.branch.link != 0)
    printf("l");

  printf("%s\t", cond[inst.branch.cond]);

  printf("%x\n", inst.branch.offset);
}


/******************************************************************************
 * decode_swi - Decode the software interrupt.
 */
void decode_swi(uint32_t i)
{
  INST inst;
  inst.raw = i;
  
  printf("%08x\tswi%s\t%x\n", i, cond[inst.swi.cond], inst.swi.val);
}


/******************************************************************************
 * decode_dpi - Decode a data processing instruction. Note that for the most
 *              part we use DPI1 until we need to worry about specifics.
 */
void decode_dpi(uint32_t i)
{
  INST inst;
  inst.raw = i;

#ifdef DEBUG
  printf("\ncond   %x\n", inst.dpi1.cond);
  printf("pad    %x\n", inst.dpi1.pad);
  printf("hash   %x\n", inst.dpi1.hash);
  printf("opcode %x\n", inst.dpi1.opcode);
  printf("set    %x\n", inst.dpi1.set);
  printf("rn     %x\n", inst.dpi1.rn);
  printf("rd     %x\n", inst.dpi1.rd);
  printf("rot    %x\n", inst.dpi1.rot);
  printf("imm    %x\n\n", inst.dpi1.imm);
#endif

  printf("%08x\t%s%s", i, dpiops[inst.dpi1.opcode], cond[inst.dpi1.cond]);

  if ((inst.dpi1.opcode >> 2) != 0x2)   
    if (inst.dpi1.set != 0)
      printf("s");

  printf("\t");

  if ((inst.dpi1.opcode != 0xD) && (inst.dpi1.opcode != 0xF))
    printf("%s, ", regs[inst.dpi1.rn]);

  if ((inst.dpi1.opcode != 0xA) && (inst.dpi1.opcode != 0xB)
      && (inst.dpi1.opcode != 0x8) && (inst.dpi1.opcode != 0x9))
    printf("%s, ", regs[inst.dpi1.rd]);

  if (inst.dpi1.hash != 0)
    {
      uint32_t t = inst.dpi1.imm >> (inst.dpi1.rot * 2);
      t |= (inst.dpi1.imm << (32 - (inst.dpi1.rot * 2)));

      printf("#%d\t; 0x%x\n", t, t); 
    }
  else
    {
      if (inst.dpi2.pad2 == 0)
	{
	  printf("%s", regs[inst.dpi2.rm]);
	  if (inst.dpi2.shift != 0)
	    printf(", %s #%d\t; 0x%x", shift[inst.dpi2.type], 
		   inst.dpi2.shift, inst.dpi2.shift);
	  printf("\n");
	}
      else
	{
	  printf("%s", regs[inst.dpi3.rm]);
	  printf(", %s %s", shift[inst.dpi3.type], regs[inst.dpi3.rs]);
	  printf("\n");
	}
    }
}


/******************************************************************************
 *
 */
void decode_mult(uint32_t i)
{
  INST inst;
  inst.raw = i;

  printf("%08x\t%s%s", i, mult[inst.mult.opcode], cond[inst.mult.cond]);

  if (inst.mult.set != 0)
    printf("s");

  switch (inst.mult.opcode)
    {
    case MUL:
      {
	printf("\t%s, %s, %s\n", regs[inst.mult.rd], regs[inst.mult.rm], 
	       regs[inst.mult.rs]);
      }
      break;
    case MLA:
      {
	printf("\t%s, %s, %s, %s\n", regs[inst.mult.rd], regs[inst.mult.rm], 
	       regs[inst.mult.rs], regs[inst.mult.rn]);
      }
      break;
    case UMULL: case UMLAL: case SMULL: case SMLAL:
      {
	printf("\t%s, %s, %s, %s\n", regs[inst.mult.rd], regs[inst.mult.rn],
	       regs[inst.mult.rm], regs[inst.mult.rs]);
      }
      break;
    default:
      {
	printf("\t????\n");
      }
      break;
    }
}


/******************************************************************************
 * decode_swt - Decodes a single word transfer instruction.
 */
void decode_swt(uint32_t i)
{
  INST inst;
  inst.raw = i;

  printf("%08x\t", i);

  /* print the instruction name */
  if (inst.swt1.ls != 0)
    printf("ldr%s", cond[inst.swt1.cond]);
  else
    printf("str%s", cond[inst.swt1.cond]);

  if (inst.swt1.b != 0)
    printf("b");

  /* The T bit???? */

  printf("\t%s, [%s", regs[inst.swt1.rd], regs[inst.swt1.rn]);
  if (inst.swt1.p == 0)
    printf("], ");
  else
    printf(", ");
  
  if (inst.swt1.hash == 0)
    {
      if (inst.swt1.u == 0)
	printf("-");
      printf("#%d", inst.swt1.imm);
    }
  else
    {
      printf("%s, %s #%x", regs[inst.swt2.rm], shift[inst.swt2.type],
	     inst.swt2.shift);
    }
  if (inst.swt1.p == 1)
    {
      printf("]");
      if (inst.swt1.wb == 1)
	printf("!");
    }

  printf("\n");
}


/******************************************************************************
 * decode_hwt - Decodes a half word transfer.
 */
void decode_hwt(uint32_t i)
{
  INST inst;
  inst.raw = i;

  printf("%08x\t", i);

  /* print the instruction name */
  if (inst.hwt.ls == 1)
    printf("ldr%s", cond[inst.hwt.cond]);
  else
    printf("str%s", cond[inst.hwt.cond]);
  
  if (inst.hwt.s == 1)
    printf("s");
  if (inst.hwt.h == 1)
    printf("h\t");
  else
    printf("b\t");

  printf("\t%s, [%s", regs[inst.hwt.rd], regs[inst.hwt.rn]);
  if (inst.hwt.p == 0)
    printf("], ");
  else
    printf(", ");

  if (inst.hwt.u == 0)
    printf("-");
  
  if (inst.hwt.hash == 0)
    {
      printf("#%d", (inst.hwt.imm << 4) + inst.hwt.rm);
    }
  else
    {
      printf("%s", regs[inst.hwt.rm]);
    }

  if (inst.hwt.p == 1)
    {
      printf("]");
      if (inst.hwt.wb == 1)
	printf("!");
    }

  printf("\n");
}


/******************************************************************************
 * decode_mrt - Decodes the Multiple Register Transfer instructions.
 */
void decode_mrt(uint32_t i)
{
  uint32_t rlist, cur;

  INST inst;
  inst.raw = i;

  printf("%08x\t", i);

  /* print the instruction name */
  if (inst.mrt.ls == 1)
    printf("ldm%s", cond[inst.mrt.cond]);
  else
    printf("stm%s", cond[inst.mrt.cond]);

  if (inst.mrt.u == 0)
    printf("d");
  else
    printf("i");

  if (inst.mrt.p == 0)
    printf("a"); 
  else
    printf("b");

  /* base register */
  printf("\t%s", regs[inst.mrt.rn]);
  if (inst.mrt.wb == 1)
    printf("!");

  printf(", {");
  rlist = inst.mrt.list;
  cur = 0;
  while (rlist != 0)
    {
      if ((rlist & 0x1) == 0x1)
	{
	  printf("%s", regs[cur]);
	  if (rlist > 1)
	    printf(", ");
	}
      cur++;
      rlist = rlist >> 1;
    }

  printf("}\n");
}


/******************************************************************************
 * decode_swp - Decodes a swap instruction.
 */
void decode_swp(uint32_t i)
{
  INST inst;
  inst.raw = i;

  printf("%08x\tswp%s", i, cond[inst.swap.cond]);

  if (inst.swap.byte == 1)
    printf("b");

  printf("\t%s, %s, [%s]\n", regs[inst.swap.rd], regs[inst.swap.rm],
	 regs[inst.swap.rn]);
}


/******************************************************************************
 * decode_sgr - Decodes a status to general register transfer.
 */
void decode_sgr(uint32_t i)
{
  INST inst;
  inst.raw = i;

  printf("%08x\tmsr%s\t%s, ", i, cond[inst.mrs.cond], regs[inst.mrs.rd]);

  if (inst.mrs.which == 1)
    printf(" cpsr\n");
  else
    printf(" spsr\n");
}


/******************************************************************************
 * decode_gsr - Decodes a general to status register transfer.
 */
void decode_gsr(uint32_t i)
{
  INST inst;
  inst.raw = i;

  printf("%08x\tmrs%s\t", i, cond[inst.msr1.cond]);

  if (inst.msr1.which == 1)
    printf(" cpsr_");
  else
    printf(" spsr_");

  if (inst.msr1.hash == 1)
    {
      printf("f, #%x\n", inst.msr1.imm << inst.msr1.rot);
    }
  else
    {
      if (inst.msr2.field & 0x1)
	printf("c, ");
      else if (inst.msr2.field & 0x2)
	printf("x, ");
      else if (inst.msr2.field & 0x4)
	printf("s, ");
      else
	printf("f, ");

      printf("%s\n", regs[inst.msr2.rm]);
    }
}


/******************************************************************************
 * decode_cdo - Decodes a CoPro data op.
 */
void decode_cdo(uint32_t i)
{
  INST inst;
  inst.raw = i;

  printf("%08x\tcdp%s\t", i, cond[inst.cdo.cond]);

  printf("p%x, %x, %s, %s, %s", inst.cdo.cpn, inst.cdo.cop1, 
	 cregs[inst.cdo.crd], cregs[inst.cdo.crn], cregs[inst.cdo.crm]);

  if (inst.cdo.cop2 != 0)
    printf(", %x", inst.cdo.cop2);

  printf("\n");
}


/******************************************************************************
 * decode_cdt - Decodes a CoPro data transfer.
 */
void decode_cdt(uint32_t i)
{
  INST inst;
  inst.raw = i;

#ifdef DEBUG
  printf("\ncond   %8x\n", inst.cdt.cond);
  printf("pad    %8x\n", inst.cdt.pad);
  printf("p      %8x\n", inst.cdt.p);
  printf("u      %8x\n", inst.cdt.u);
  printf("n      %8x\n", inst.cdt.n);
  printf("wb     %8x\n", inst.cdt.wb);
  printf("ls     %8x\n", inst.cdt.ls);
  printf("rn     %8x\n", inst.cdt.rn);
  printf("crd    %8x\n", inst.cdt.crd);
  printf("cpn    %8x\n", inst.cdt.cpn);
  printf("offset %8x\n\n", inst.cdt.offset);
#endif

  printf("%08x\t", i);

  if (inst.cdt.ls == 1)
    printf("ldc%s", cond[inst.cdt.cond]);
  else
    printf("stc%s", cond[inst.cdt.cond]);

  if (inst.cdt.n == 1)
    printf("l");

  printf("\tp%x, %s, [%s", inst.cdt.cpn, cregs[inst.cdt.crd], 
	 regs[inst.cdt.rn]);

  if (inst.cdt.p == 0)
    printf("]");

  if (inst.cdt.offset != 0)
    printf(", #%d", inst.cdt.offset);
  
  if (inst.swt1.p == 1)
    {
      printf("]");
      if (inst.swt1.wb == 1)
	printf("!");
    }

  printf("\n");
}


/******************************************************************************
 * decode_crt - Decode a CoPro register transfer.
 */
void decode_crt(uint32_t i)
{
  INST inst;
  inst.raw = i;

  printf("%08x\t", i);

  if (inst.crt.ls == 1)
    printf("mcr");
  else
    printf("mrc");

  printf("%s\t p%x, %x, %s, %s, %s", cond[inst.crt.cond], inst.crt.cpn,
	 inst.crt.cop1, regs[inst.crt.rd], cregs[inst.crt.crn], 
	 cregs[inst.crt.crm]);

  if (inst.crt.cop2 != 0)
    printf(", %x", inst.crt.cop2);

  printf("\n");
}


/******************************************************************************
 * decode_word - Decides what type of instruction this word is and passes it 
 *               on to the appropriate child decoder. Note that the ordering 
 *               here is important. E.g. you need to test for a mutliply 
 *               instruction *before* a DPI instruction - a mult looks like
 *               an AND too. :-(
 */
void decode_word(uint32_t i)
{
  if (i == 0)
    return;
  
  if ((i & BRANCH_MASK) == BRANCH_SIG)
    {
      decode_branch(i);
    }
  else if ((i & SWI_MASK) == SWI_SIG)
    {
      decode_swi(i);
    }
  else if ((i & MULT_MASK) == MULT_SIG)
    {
      decode_mult(i);
    }
  else if ((i & DPI_MASK) == DPI_SIG)
    {
      decode_dpi(i);
    }
  else if ((i & SWT_MASK) == SWT_SIG)
    {
      decode_swt(i);
    }
  else if ((i & HWT_MASK) == HWT_SIG)
    {
      decode_hwt(i);
    }
  else if ((i & MRT_MASK) == MRT_SIG)
    {
      decode_mrt(i);
    }
  else if ((i & SWP_MASK) == SWP_SIG)
    {
      decode_swp(i);
    }
  else if ((i & MRS_MASK) == MRS_SIG)
    {
      decode_sgr(i);
    }
  else if ((i & MSR_MASK) == MSR_SIG)
    {
      decode_gsr(i);
    }
  else if ((i & CDO_MASK) == CDO_SIG)
    {
      decode_cdo(i);
    }
  else if ((i & CDT_MASK) == CDT_SIG)
    {
      decode_cdt(i);
    }
  else if ((i & CRT_MASK) == CRT_SIG)
    {
      decode_crt(i);
    }
  else if ((i & UAI_MASK) == UAI_SIG)
    {
      printf("Unused arithmetic op\n");
    }
  else if ((i & UCI1_MASK) == UCI1_SIG)
    {
      printf("Unused control 1\n");
    }
  else if ((i & UCI2_MASK) == UCI2_SIG)
    {
      printf("Unused control 2\n");
    }
  else if ((i & UCI3_MASK) == UCI3_SIG)
    {
      printf("Unused control 3\n");
    }
  else if ((i & ULSI_MASK) == ULSI_SIG)
    {
      printf("Unused load/store\n");
    }
  else if ((i & UCPI_MASK) == UCPI_SIG)
    {
      printf("Unused CoPro\n");
    }
  else if ((i & UNDEF_MASK) == UNDEF_SIG)
    {
      printf("Undefined\n");
    }
  else
    {
      printf("Rubbish");
    }
}

int main(int argc, char* argv[])
{
  FILE* fp;
  uint32_t data;
  uint32_t i;

  /* Did we get the correct number of parameters? */
  if (argc != 2)
    {
      fprintf(stderr, "Usage: disarm [binary]\n");
      exit(EXIT_FAILURE);
    }

  /* Yup, so try opening the file */
  if ((fp = fopen(argv[1], "r")) == NULL)
    {
      fprintf(stderr, "Failed to open file %s\n", argv[1]);
      exit(EXIT_FAILURE);
    }

  i = 0;
  while (fread(&data, sizeof(data), 1, fp) == 1)
    {
#ifdef __BIG_ENDIAN__
      uint32_t temp = data;
      data  = (temp & 0x000000FF) << 24;
      data |= (temp & 0x0000FF00) << 8;
      data |= (temp & 0x00FF0000) >> 8;
      data |= (temp & 0xFF000000) >> 24;
#endif /* __BIG_ENDIAN__ */

      printf("%4x:   ", i);
      i += 4;
      decode_word(data);
    }

  fclose(fp);
  exit(EXIT_SUCCESS);
}
