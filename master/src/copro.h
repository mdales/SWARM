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
// name   copro.h
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   Abstract interface for an internal coprocessor
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __COPRO_H__
#define __COPRO_H__

#include "swarm.h"

typedef struct CPBIOTAG
{
  uint32_t mclk : 1; // IGNORED
  uint32_t reset : 1; // IGNORED

  uint32_t irq : 1; 
  uint32_t fiq : 1; 

  uint32_t Dout : 32;
  uint32_t Din : 32;

  uint32_t Vdd : 1; // IGNORED
  uint32_t Vss : 1; // IGNORED

  uint32_t opc : 1; 
  uint32_t cpi : 1; 
  uint32_t cpa : 1; 
  uint32_t cpb : 1; 

  uint32_t dw  : 1; // Data out on this cycle
} COPROBUS;

class CCoProcessor
{
  // Constructors and destructor
 public:
  virtual ~CCoProcessor();

  // Public methods
 public:
  virtual void Cycle(COPROBUS* bus) = 0;
  virtual void DebugDump() = 0;
};

class CCoProSetException : public CException
{
 public:
  CCoProSetException();
};

class CCoProInvalidException : public CException
{
 public:
  CCoProInvalidException();
};

#endif //__COPRO_H__
