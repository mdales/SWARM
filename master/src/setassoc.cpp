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
// name   setassoc.cpp
// author Michael Dales (michael@dcs.gla.ac.uk)
// header setassoc.h
// info   Implements a n-way set associative cache. I use n direct map caches
//        to implement this. Note that there is a bit of wastage here, as
//        the set bits will be stored in the tag inside the direct map cache,
//        but this isn't going to effect the behaviour of the cache, so we l
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "swarm.h"
#include "setassoc.h"
#include "direct.h"
#include <string.h>


#define LINE_SIZE_W 4  /* (words) */
#define LINE_SIZE_B 16 /* (bytes) */


///////////////////////////////////////////////////////////////////////////////
//
//
CSetAssociativeCache::CSetAssociativeCache(uint32_t nSize)
{
  m_nSize = nSize;
  m_nWay = 2;

  InitSets();
}

CSetAssociativeCache::CSetAssociativeCache(uint32_t nSize, int nWay)
{
  m_nSize = nSize;
  m_nWay = nWay;

  InitSets();
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSetAssociativeCache::InitSets()
{

  // Allocate the direct mapped caches
  m_pSets = new CDirectCache*[m_nWay];
  for (int i = 0; i < m_nWay; i++)
    m_pSets[i] = new CDirectCache(m_nSize/m_nWay);
  
  // Now Allocate the state to enable us to do the RR on lines
  m_pSetRR = new uint8_t[m_nSize/m_nWay];
  memset(m_pSetRR, 0, sizeof(uint8_t) * (m_nSize/m_nWay));

  // Calc some useful into here
  m_tagMask = ((m_nSize/m_nWay) / LINE_SIZE_B) - 1;
  
  uint32_t temp = m_tagMask;
  m_tagBits = 0;
  while (temp != 0)
    {
      temp = temp >> 1;
      m_tagBits++;
    }

  m_tagBits += 2;
}


///////////////////////////////////////////////////////////////////////////////
//
//
CSetAssociativeCache::~CSetAssociativeCache()
{
  for (int i = 0; i < m_nWay; i++)
    delete m_pSets[i];
  delete m_pSets;
  delete m_pSetRR;
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSetAssociativeCache::Reset()
{
  for (int i = 0; i < m_nWay; i++)
    m_pSets[i]->Reset();
}


///////////////////////////////////////////////////////////////////////////////
//
//
uint32_t CSetAssociativeCache::Read(uint32_t addr)
{
  uint32_t rv;
  int i;

  // Try all our sub caches, and if we don't find one then barf
  for (i = 0; i < m_nWay; i++)
    {
      try
	{
	  rv = m_pSets[i]->Read(addr);
	  break;
	}
      catch (CCacheMiss &e)
	{
	}
    }
  if (i == m_nWay)
    throw CCacheMiss(addr);

  // Then try the read. Don't catch any errors, leave that for higher up
  return rv;
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSetAssociativeCache::WriteLine(uint32_t addr, uint32_t* pLine)
{
  int tag_sel;

  // Work out which line in a set it would be written to
  tag_sel = (addr >> 2) & m_tagMask;


  // Now write the line to that set
  m_pSets[m_pSetRR[tag_sel]]->WriteLine(addr, pLine);

  // Update the RR info
  m_pSetRR[tag_sel]++;
  if (m_pSetRR[tag_sel] == m_nWay)
    m_pSetRR[tag_sel] = 0;
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSetAssociativeCache::InvalidateLineByAddr(uint32_t addr)
{
  int i;

  // Try all our sub caches, and if we don't find one then barf
  for (i = 0; i < m_nWay; i++)
    {
      try
	{
	  m_pSets[i]->Read(addr);
	  
	  // Got here, so we succeeded. Now hose the line in that set
	  m_pSets[i]->InvalidateLineByAddr(addr);

	  break;
	}
      catch (CCacheMiss &e)
	{
	}
    }

}


///////////////////////////////////////////////////////////////////////////////
//
//
void CSetAssociativeCache::WriteWord(uint32_t addr, uint32_t word)
{
  int i;

  // Try all our sub caches, and if we don't find one then barf
  for (i = 0; i < m_nWay; i++)
    {
      try
	{
	  m_pSets[i]->Read(addr);
	  
	  // Got here, so we succeeded. Now write the word into that set
	  m_pSets[i]->WriteWord(addr, word);

	  break;
	}
      catch (CCacheMiss &e)
	{
	}
    }

}
