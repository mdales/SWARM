///////////////////////////////////////////////////////////////////////////////
// Copyright 2001 Michael Dales
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
// name   setassoc.h
// author Michael Dales (michael@dcs.gla.ac.uk)
// header n/a
// info   Implements a n-way set associative cache.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __SETASSOC_H__
#define __SETASSOC_H__

#include "cache.h"

// forwards
class CDirectCache;

class CSetAssociativeCache : public CCache
{
  // Constructors and destructor
 public:
  CSetAssociativeCache(uint32_t nSize); // Defaults to two way
  CSetAssociativeCache(uint32_t nSize, int nWay);
  ~CSetAssociativeCache();

  // Public methods
 public: 
  uint32_t Read(uint32_t addr);
  void     WriteLine(uint32_t addr, uint32_t* pLine);
  void     WriteWord(uint32_t addr, uint32_t word);
  void     InvalidateLineByAddr(uint32_t addr);
  void     Reset();

 private:
  void InitSets();
  
  // Private data types
 private:
  int            m_nWay;
  uint32_t       m_nSize;
  
  uint32_t       m_tagBits;
  uint32_t       m_tagMask;

  uint8_t*       m_pSetRR; // Used to round robin the lines in sets

  CDirectCache** m_pSets;

};

#endif // __SETASSOC_H__
