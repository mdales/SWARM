///////////////////////////////////////////////////////////////////////////////
// Copyright 2000, 2001 Michael Dales
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
// name   main.cpp
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   Test harness.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "swarm.h"
#include "armproc.h"
#include <fcntl.h>
#include <string.h>
#ifdef WIN32
#include <IO.h>
#else
#include <unistd.h>
#endif
#include "cache.h"
#include "direct.h"
#include <iostream.h>
#include <sys/stat.h>
#include "libc.h"
#include "syscopro.h"

#define FAST_CYCLE 1
#define SLOW_CYCLE 4

#define MEMORY_SIZE (1024 * 1024 * 12)
#define DEFAULT_CACHESIZE  1024 * 8

#define SWI_EXIT 0x00800000
#define SWI_DUMP 0x0080000F
#define SWI_ARGS 0x0080000E

CArmProc* pArm;
char* pMemory;

typedef struct OTAG
{
  char* strProgName;
  char* strSrecProgName;
  uint32_t nCacheSize;
} OPTS;

#ifdef __BIG_ENDIAN__
#define ENDIAN_CORRECT(_x) ((_x << 24) | ((_x << 8) & 0xFF0000) | ((_x >> 8) & 0xFF00) | (_x >> 24))
#define ENDIAN_CORRECT_16(_x) (ENDIAN_CORRECT(_x) >> 16)
#else
#define ENDIAN_CORRECT(_x) _x
#define ENDIAN_CORRECT_16(_x) _x
#endif


///////////////////////////////////////////////////////////////////////////////
// SWI_EXIT_FN - This is used to halt the simulation and display the cycle
//               counts. It also saves the contents on the memory to a file
//               called "/tmp/mem".
//
uint32_t SWI_EXIT_FN(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  ASSERT(pArm != NULL);

  uint64_t t1, t2;
  t1 = pArm->GetRealCycles();
  t2 = pArm->GetLogicalCycles();
  
  cout << "Cycle info: real = " << t1 << " logical = " << t2 << "\n";

#ifndef arm32  
  int fd = open("/tmp/mem", O_CREAT | O_RDWR, 0644);
  write(fd, pMemory, MEMORY_SIZE);
  close(fd);
#endif

  delete pMemory;
  delete pArm;

  exit(EXIT_SUCCESS);

  return r0;
}


///////////////////////////////////////////////////////////////////////////////
// void dump() - Dumps register contents to the SWARM console.
//
uint32_t SWI_DUMP_FN(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  pArm->DebugDump();

  return r0;
}


///////////////////////////////////////////////////////////////////////////////
// void args() - Gets a pointer to the args block.
//
uint32_t SWI_ARGS_FN(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
  return (MEMORY_SIZE - 2048);
}


enum PARAMS  {P_NONE, P_CACHE, P_SRECFILE, P_BAD};

void parse_options(int argc, char* argv[], OPTS* opts)
{
  int bParam = 0;
  enum PARAMS p = P_NONE;

  // First check the args
  if (argc < 2)
    {
      cerr << "Usage: swarm program-bin -s program-srec [params]\n";
      exit (EXIT_FAILURE);
    }

  // Defaults
  opts->nCacheSize = DEFAULT_CACHESIZE;
  opts->strProgName = NULL;
  opts->strSrecProgName = NULL;

  for (int i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-')
	{
	  switch (argv[i][1])
	    {
	    case 'c' : 
	      {
		p = P_CACHE;
	      }
	      break;
	    case 's' :
	      {
		p = P_SRECFILE;
	      }
	      break;
	    }
	}
      else
	{
	  switch (p)
	    {
	    case P_NONE:
	      {
		opts->strProgName = strdup(argv[i]);
	      }
	      break;
	    case P_CACHE:
	      {
		opts->nCacheSize = atoi(argv[i]);
	      }
	      break;
	    case P_SRECFILE:
	      {
		opts->strSrecProgName = strdup(argv[i]);      
	      }
	      break;
	    }
	}
    }
  if ( (opts->strProgName == NULL) && (opts->strSrecProgName == NULL) )
  {
    cerr << "Error: No program specified\n";
    cerr << "Usage: swarm program-bin -s program-srec [params]\n";
    exit(EXIT_FAILURE);
  }
}


///////////////////////////////////////////////////////////////////////////////
// marshal_args - This is a hack, and I'm not happy with it, but until I get
//                some form of OS running of SWARM then it'll have to do.
//                
//                The function marshals the arguments passed to swarm into
//                memory so they can be passed as args to it. They need to 
//                fit in the systems memory at a given offset and must not
//                exceed the specified range, otherwise they may run into
//                other important things. Returns false if it fails to fit
//                the args into the memory specified.
//
bool_t marshal_args(int argc, char* argv[], char* pMemory, 
		    uint32_t offset, uint32_t range)
{
  int count = 0;

  // Work out how much memory we'll need
  count += 4; // argc
  count += argc * 4; // *argv
  for (int i = 1; i < argc; i++)
    count += strlen(argv[i]) + 1;

  // Have we been given enough memory for this?
  if (count > range)
    return false;

  // Write the value of argc, which will be one less than ours
  *((uint32_t*)(pMemory + offset)) = ENDIAN_CORRECT(argc - 1);
  
  // The next n bytes will be argv, so let's get a pointer to it
  uint32_t* my_argv = (uint32_t*)(pMemory + offset + 4);

  // Now walk through the memory copying in the strings
  char* pMem = pMemory + offset + 4 + (argc * 4);
  for (int i = 1; i < argc; i++)
    {
      memcpy(pMem, argv[i], strlen(argv[i]) + 1);
      my_argv[i - 1] = (uint32_t)(pMem - pMemory);
      pMem += strlen(argv[i]) + 1;
    }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//
int load_program(OPTS opts)
{
  int fd;
  struct stat s;

  if (opts.strProgName == NULL)
    {
      cerr << "Note: No Program-Binary to Upload\n";
      return EXIT_FAILURE;
    }

  fd = open(opts.strProgName, O_RDONLY);
  if (fd == -1)
    {
      cerr << "Error: Uploading Program-Binary: " << opts.strProgName << "\n";
      return EXIT_FAILURE;
    }
  
  fstat(fd, &s);
  read(fd, pMemory, s.st_size);
  close(fd);
  cout << "Note: Uploaded the Program-Binary: " << opts.strProgName << "\n";
  return EXIT_SUCCESS;	
}


///////////////////////////////////////////////////////////////////////////////
//
//
long hexstringtonumber(char *str, int start, int len)
{
	long iValue;
	char sValue[256];
	int iCur;

	for(iCur = 0; iCur < len; iCur++)
	{
		sValue[iCur] = str[start+iCur];			
	}
	sValue[len] = '\0';
	iValue = strtol(sValue,NULL,16);
	//printf("String: %s Value: %d\n", sValue, iValue);
	return iValue;
}

///////////////////////////////////////////////////////////////////////////////
//
//  s-record loader, supports only 32bit addresses and transfer:
//  S3   Data record with 32 bit load address                            
//  S7   Termination record with 32 bit transfer address
//
//  Stnnaaaaaaaa[dddd...dddd]cc
//   t record type field (0,1,2,3,6,7,8,9).          
//   nn record length field, number of bytes in record excluding record type 
//     and record length. 
//   a...a load address field, can be 16, 24 or 32 bit address for data to 
//     be loaded. 
//   d...d data field, actual data to load, each byte is encoded in 2 
//     characters. 
//   cc checksum field, 1's complement of the sum of all bytes in the record
//     length, load address and data fields 
//
//
int load_srec_program(OPTS opts)
{
  FILE *fd;
  char str[4096];
  int CountBytes, iCur, iValue;
  unsigned long Address, MinAddress, MaxAddress;
  
  if (opts.strSrecProgName == NULL)
    {
      cerr << "Note: No Program-SRec to Upload\n";
      return EXIT_SUCCESS;
    }
  MinAddress = 0xffffffff;
  MaxAddress = 0;

  fd = fopen(opts.strSrecProgName, "r");
  if (fd == NULL)
  {
    cerr << "Error: Uploading Program-SRec:" << opts.strSrecProgName << "\n";
    return EXIT_FAILURE;
  }
  
  while(fgets(str,4096,fd) != NULL)
    {
      if(str[0] != 'S')
	{
	  fprintf(stderr,"Error: Not a SRec line\n");
	  continue;
	}
      switch(str[1])
	{
	case '2':
	  CountBytes =	hexstringtonumber(str,2,2);
	  Address = hexstringtonumber(str,4,6);
	  if(MinAddress > Address) MinAddress = Address;
	  if(MaxAddress < Address) MaxAddress = Address;
	  for(iCur = 10; iCur < (CountBytes-4)*2+10; iCur+=2)
	    {
	      iValue = hexstringtonumber(str,iCur,2);
	      pMemory[Address++]	=	iValue;
	      //printf("Just read: %x is %x\n", pMemory[Address-1], iValue);
	    }
	  break;
	case '3':
	  CountBytes =	hexstringtonumber(str,2,2);
	  Address = hexstringtonumber(str,4,8);
	  for(iCur = 12; iCur < (CountBytes-5)*2+12; iCur+=2)
	    {
	      iValue = hexstringtonumber(str,iCur,2);
	      pMemory[Address++]	=	iValue;
	      //printf("Just read: %x is %x\n", pMemory[Address-1], iValue);
	    }
	  break;
	case '7':
	  break;
	}
    }
  fclose(fd);
  printf("Note: Uploaded Program-SRec %s between addresses[hex]: %x to %x \n", 
	 opts.strSrecProgName, MinAddress, MaxAddress);
  return EXIT_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
//
//
int main(int argc, char* argv[])
{
  PINOUT pinout;
  int i = 0;

#ifdef DEBUGGER
  int DebuggerRepeatCount = 0;
  unsigned int DebuggerPrevPC=999, DebuggerBreakPoint=0, DebuggerMoreRequest;
  char DebuggerRequest[64], sDebuggerBreakPoint[64];
#endif

  // Let me used mixed IO
  ios::sync_with_stdio();

  OPTS opts;
  parse_options(argc, argv, &opts);

  pArm = new CArmProc(DEFAULT_CACHESIZE);
  pMemory = new char[MEMORY_SIZE];

  // Blank the memory
  memset(pMemory, 0, MEMORY_SIZE);

  // Load the program
  if ( (load_program(opts) != EXIT_SUCCESS) &&
       (load_srec_program(opts) != EXIT_SUCCESS) )
    goto exit;

  // In the last 2k of memory I'll shove in the arguments
  if (!marshal_args(argc, argv, pMemory, MEMORY_SIZE - 2048, 2048))
    {
      cerr << "Failed to marshall arguments for test app\n";
      goto exit;
    }

  try
    {
      pArm->RegisterSWI(SWI_EXIT, SWI_EXIT_FN);
      pArm->RegisterSWI(SWI_DUMP, SWI_DUMP_FN);
      pArm->RegisterSWI(SWI_ARGS, SWI_ARGS_FN);
#ifdef LIBC_SUPPORT
      pArm->RegisterSWI(SWI_LIBC_WRITE, swi_libc_write);
      pArm->RegisterSWI(SWI_LIBC_READ, swi_libc_read);
      pArm->RegisterSWI(SWI_LIBC_OPEN, swi_libc_open);
      pArm->RegisterSWI(SWI_LIBC_CREAT, swi_libc_creat);
      pArm->RegisterSWI(SWI_LIBC_CLOSE, swi_libc_close);
      pArm->RegisterSWI(SWI_LIBC_FCNTL, swi_libc_fcntl);
      pArm->RegisterSWI(SWI_LIBC_LSEEK, swi_libc_lseek);
      pArm->RegisterSWI(SWI_LIBC_STAT, swi_libc_stat);
      pArm->RegisterSWI(SWI_LIBC_FSTAT, swi_libc_fstat);
      pArm->RegisterSWI(SWI_LIBC_LSTAT, swi_libc_lstat);
#endif
    }
  catch (CException &e)
    {
      cerr << "SWI Register error: ";
      cerr << e.StrError() << "\n";
      goto exit;
    }

  // Setup the bus safely
  pinout.fiq = 1;
  pinout.irq = 1;  
  pinout.address = 0;
  pinout.rw = 0;

  //for (int i = 0; i < 4500; i++)
  while(1)
    {
       printf("cycle---->>>>\n");
      // Cycle the ARM
      pArm->Cycle(&pinout);
#ifdef DEBUGGER
      // Added by HanishKVC to help debug
	if(DebuggerPrevPC != pArm->NextPC())
	{
	  DebuggerPrevPC = pArm->NextPC();
	  if( (DebuggerPrevPC >= (DebuggerBreakPoint)) 
			  && (DebuggerPrevPC <= (DebuggerBreakPoint+0x8)) )
		DebuggerRepeatCount = 0;
	  if(DebuggerRepeatCount <= 0)
	  {
		DebuggerRepeatCount = 1;
		do
		{
			printf("\nSWARM Debugger[0x%x]>",DebuggerPrevPC);
			cin >> DebuggerRequest;
			DebuggerMoreRequest = 0;
			if(DebuggerRequest[0] == 'q')
				_exit(0);
			else if(DebuggerRequest[0] == 'd')
			{
				pArm->DebugDumpCore();
			}
			else if(DebuggerRequest[0] == 'D')
			{
				pArm->DebugDump();
				DebuggerMoreRequest = 1;
			}
			else if(DebuggerRequest[0] == 'c')
				DebuggerRepeatCount = 2000000000;
			else if(DebuggerRequest[0] == '1')
				DebuggerRepeatCount = 10;
			else if(DebuggerRequest[0] == '2')
				DebuggerRepeatCount = 100;
			else if(DebuggerRequest[0] == '3')
				DebuggerRepeatCount = 1000;
			else if(DebuggerRequest[0] == '4')
				DebuggerRepeatCount = 10000;
			else if(DebuggerRequest[0] == 'b')
			{
				cout << "Enter the breakpoint:";
				cin >> sDebuggerBreakPoint;
				DebuggerBreakPoint = strtoul(sDebuggerBreakPoint,NULL,0);
				printf("Setting breakpoint to:0x%x\n",
						DebuggerBreakPoint);
				DebuggerMoreRequest = 1;
			}
			else if( (DebuggerRequest[0] == 'h') 
					|| (DebuggerRequest[0] == '?') )
			{
				cout << "Supported commands: \n";
				cout << " c - Continue\n";
				cout << " d - dump processor Core status and Step 1 instr\n";
				cout << " D - dump status of full Processor\n";
				cout << " b - Set breakpoint\n";
				cout << " 1 - execute 10 instructions \n";
				cout << " 2 - execute 100 instructions \n";
				cout << " 3 - execute 1000 instructions \n";
				cout << " 4 - execute 10000 instructions \n";
				cout << " h/? - Help\n";
				cout << " q - Quit\n";
				DebuggerMoreRequest = 1;
			}
		}while(DebuggerMoreRequest);
	  }
	  DebuggerRepeatCount--;
	}
#endif DEBUGGER
      // Do we need to do anything with the bus?
      if (pinout.benable == 1)
	{
	  // Quick sanity check
	  if (pinout.address >= MEMORY_SIZE)
	    {
	      fprintf(stderr, "SWARM failing: Bad address - 0x%08X\n", 
			pinout.address);

	      pArm->DebugDump();
	      
	      //break;
	      continue;
	    }

	  // Is is a read or write we need to do?
	  if (pinout.rw == 1)
	    {
	      switch (pinout.bw)
		{
		case 0: // Write word
		  {
		    uint32_t* addr = 
		      (uint32_t*)(pMemory + (pinout.address & 0xFFFFFFFC));
		    *addr = ENDIAN_CORRECT(pinout.data);
		  }
		  break;
		case 1 : // Write byte
		  {
		    //printf("wrote byte 0x%x @ 0x%x\n", 
		    // (pinout.data & 0x000000FF),  pinout.address);
		    *((char*)(pMemory + pinout.address)) = 
		      (char)(pinout.data & 0x000000FF);
		  }
		  break;
		case 2 : // Write half word
		  {
		    uint16_t* addr = 
		      (uint16_t*)(pMemory + (pinout.address & 0xFFFFFFFE));
		    *addr = (uint16_t)(ENDIAN_CORRECT_16(pinout.data & 0x0000FFFF));
		  }
		  break;
		}
#if 0
	      if (pinout.bw == 0)
		{		 
		  // Write word
#if 0
		  uint32_t addr = 
		    (uint32_t)pMemory + (pinout.address & 0xFFFFFFFC);
		  *((uint32_t*)(addr)) = pinout.data;
#else
		  uint32_t* addr = 
		    (uint32_t*)(pMemory + (pinout.address & 0xFFFFFFFC));
		  *addr = ENDIAN_CORRECT(pinout.data);
#endif
		}
	      else
		{
		  // Write byte
		  //printf("wrote byte 0x%x @ 0x%x\n", 
		  // (pinout.data & 0x000000FF),  pinout.address);
		  *((char*)(pMemory + pinout.address)) = 
		    (char)(ENDIAN_CORRECT(pinout.data) & 0x000000FF);
		}
#endif
	    }
	  else
	    {
	      // Read
	      pinout.data = ENDIAN_CORRECT(*((uint32_t*)(pMemory + pinout.address)));
	    }
	}
    }

 quit:
  SWI_EXIT_FN(0, 0, 0, 0);

 exit:
  delete pMemory;
  delete pArm;

  return 0;
}

