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
// name   cache.h
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   Defines an abstract interface for a cache. The idea is that there
//        will be several actual implementations that can be then used, such
//        as direct, n-way, etc.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CACHE_H__
#define __CACHE_H__

#include "swarm.h"

///////////////////////////////////////////////////////////////////////////////
// CCache - Abstract cache definition.
//
class CCache
{
 public:
  virtual ~CCache();

 public:
  virtual uint32_t Read(uint32_t addr) = 0;
  virtual void WriteLine(uint32_t addr, uint32_t* pLine) = 0;
  virtual void WriteWord(uint32_t addr, uint32_t word) = 0;
  virtual void InvalidateLineByAddr(uint32_t addr) = 0;
  virtual void Reset() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// CCacheMiss - Exception thrown when there is a cache miss.
//
class CCacheMiss
{
 public:
  CCacheMiss(uint32_t addr);

 public:
  inline uint32_t GetAddres() {return m_addr; }

 private:
  uint32_t m_addr;
};


#endif // __CACHE_H__
