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
// name   associative.cpp
// author Michael Dales (michael@dcs.gla.ac.uk)
// header direct.h
// info   Implements a fully associative cache. Note that, rather confusingly,
//        the size is specified in bytes, but the address given for reads
//        and writes is in words.
//
///////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>

#include <stdlib.h>
#include "swarm.h"
#include "associative.h"

#define LINE_SIZE_W 4  /* (words) */
#define LINE_SIZE_B 16 /* (bytes) */
#define INVALID_BIT 0x00000001

///////////////////////////////////////////////////////////////////////////////
// Constructor
//
CAssociativeCache::CAssociativeCache(uint32_t nSize)
{
  m_nSize = nSize;
  m_nLines = nSize / LINE_SIZE_B;

  m_pDataRAM = new uint32_t[nSize / sizeof(uint32_t)];
  m_pTagCAM = new uint32_t[m_nLines];

  Reset();
}


///////////////////////////////////////////////////////////////////////////////
// Destructor
//
CAssociativeCache::~CAssociativeCache()
{
#if 1
  int fd = open("/tmp/cache", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  write(fd, m_pDataRAM, m_nSize);
  close(fd);
#endif

  delete[] m_pDataRAM;
  delete[] m_pTagCAM;
}


///////////////////////////////////////////////////////////////////////////////
//
//
void CAssociativeCache::Reset()
{
  // Mark all the tags as invalid
  for (uint32_t i = 0; i < (m_nSize / LINE_SIZE_B); i++)
    m_pTagCAM[i] = INVALID_BIT;
}


///////////////////////////////////////////////////////////////////////////////
//
//
uint32_t CAssociativeCache::Read(uint32_t addr)
{
  uint32_t tag = addr & 0xFFFFFFFC;
  uint32_t word = addr & 0x00000003;
  
  for (int i = 0; i < m_nLines; i++)
    {
      // If it's not this line then continue
      if (m_pTagCAM[i] != tag)
	continue;

      // Got a hit, so return the correct word
      return m_pDataRAM[(i * LINE_SIZE_W) + word];
    }

  // Failed to find data in the cache, so throw a miss
  throw CCacheMiss(addr);
}


///////////////////////////////////////////////////////////////////////////////
// WriteLine - Replacement algorithm: first try to find a free space. If none,
//             pick one at random.
//
void CAssociativeCache::WriteLine(uint32_t addr, uint32_t* pLine)
{
  uint32_t tag = addr & 0xFFFFFFFC;
  int i;

  // Look for a free line in the cache
  for (i = 0; i < m_nLines; i++)
    {
      // If it's not this line then continue
      if (m_pTagCAM[i] != INVALID_BIT)
	continue;

      // Got a place, so use it
      m_pTagCAM[i] = tag;
      for (int j = 0; j < LINE_SIZE_W; j++)
	m_pDataRAM[((i * LINE_SIZE_W) + j)] = pLine[j];

      return;
    }
  
  // Failed to find a free cache line, so do a random selection
  uint64_t temp = rand();
  temp *= m_nLines;
  temp /= RAND_MAX;
  i = temp;

  m_pTagCAM[i] = tag;
  for (int j = 0; j < LINE_SIZE_W; j++)
    m_pDataRAM[((i * LINE_SIZE_W) + j)] = pLine[j];
}



///////////////////////////////////////////////////////////////////////////////
// InvalidateLine - No worrying about sets here. 
//
void CAssociativeCache::InvalidateLineByAddr(uint32_t addr)
{
  uint32_t tag = addr & 0xFFFFFFFC;

  for (int i = 0; i < m_nLines; i++)
    {
      if (m_pTagCAM[i] == tag)
	{
	  m_pTagCAM[i] = INVALID_BIT;
	  break;
	}
    }
}


///////////////////////////////////////////////////////////////////////////////
// WriteWord - Write a word into an existing line (used on write-thru). Will
//             throw an exception if the line we're after isn't in the cache.
//
void CAssociativeCache::WriteWord(uint32_t addr, uint32_t word)
{
  uint32_t tag = addr & 0xFFFFFFFC;
  uint32_t word_sel = addr & 0x00000003;
  
  // Search for line in the tag CAM
  for (int i = 0; i < m_nLines; i++)
    {
      if (m_pTagCAM[i] != tag)
	continue;

      // Found the line, so update the word
      m_pDataRAM[(i * LINE_SIZE_W) + word_sel] = word;
      return;
    }

  // No find, so throw an exception
  throw CCacheMiss(addr);
}
