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
// name   direct.cpp
// author Michael Dales (michael@dcs.gla.ac.uk)
// header direct.h
// info   Implements a direct mapped cache. Note that, rather confusingly,
//        the size is specified in bytes, but the address given for reads
//        and writes is in words.
//
///////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>

#include "swarm.h"
#include "direct.h"

#define LINE_SIZE_W 4  /* (words) */
#define LINE_SIZE_B 16 /* (bytes) */
#define INVALID_BIT 0x80000000

///////////////////////////////////////////////////////////////////////////////
// CDirectCache - Constructor
//
CDirectCache::CDirectCache(uint32_t nSize)
{
  // XXX: Gross hack - must go...sometime
  m_nSize = nSize;
  m_nLines = nSize / LINE_SIZE_B;

  m_pDataRAM = new uint32_t[nSize / sizeof(uint32_t)];
  m_pTagRAM = new uint32_t[m_nLines];

  m_tagMask = m_nLines - 1;
  
  uint32_t temp = m_tagMask;
  m_tagBits = 0;
  while (temp != 0)
    {
      temp = temp >> 1;
      m_tagBits++;
    }

  Reset();
}


///////////////////////////////////////////////////////////////////////////////
// ~CDirectCache - Destructor
//
CDirectCache::~CDirectCache()
{
#if 1
  int fd = open("/tmp/cache", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  write(fd, m_pDataRAM, m_nSize);
  close(fd);
#endif

  delete[] m_pTagRAM;
  delete[] m_pDataRAM;
}


///////////////////////////////////////////////////////////////////////////////
// Read - Reads a word from the cache. Returns FALSE if there is a cache miss.
//
uint32_t CDirectCache::Read(uint32_t addr)
{
  uint32_t word_sel, tag_sel, tag;
  uint32_t temp;
 
  // Spilt the address up into the bits we want 
  word_sel = addr & 0x00000003;
  tag_sel = (addr >> 2) & m_tagMask;
  tag = (addr >> (m_tagBits + 2));
  
  // can we find the line of data we want in the cache? 
  temp = m_pTagRAM[tag_sel];
  if (((temp & INVALID_BIT) == INVALID_BIT) || (temp != tag))
    {
      //CCacheMiss c(addr);
      //throw c;

      throw CCacheMiss(addr);
    }
  
  return m_pDataRAM[(tag_sel * 4) + word_sel];
}


///////////////////////////////////////////////////////////////////////////////
// WriteLine - Writes a line of words into the cache. Note the address is of 
//             the first word in the line.
//
void CDirectCache::WriteLine(uint32_t addr, uint32_t* pLine)
{
  uint32_t word_sel, tag_sel, tag;

  // Spilt the address up into the bits we want 
  tag_sel = (addr >> 2) & m_tagMask;
  tag = (addr >> (m_tagBits + 2));    

#if 0
  if ((m_pTagRAM[tag_sel] & INVALID_BIT) == 0)
    fprintf(stderr, "\t--- line %03d for 0x%08x\n", tag_sel, addr);

  fprintf(stderr, "\t+++ line %03d for 0x%08x\n", tag_sel, addr);
#endif

  m_pTagRAM[tag_sel] = tag;
  for (int i = 0; i < LINE_SIZE_W; i++)
    m_pDataRAM[(tag_sel * 4) + i] = pLine[i];
}


///////////////////////////////////////////////////////////////////////////////
// InvalidateLine - No worrying about sets here. 
//
void CDirectCache::InvalidateLineByAddr(uint32_t addr)
{
  uint32_t tag_sel;

  tag_sel = (addr >> 2) & m_tagMask;

  m_pTagRAM[tag_sel] |= INVALID_BIT;
}


///////////////////////////////////////////////////////////////////////////////
// WriteLine - Writes a word into the cache. Assumes that the line is valid - 
//             will throw an miss exception if you try to write a word to
//             a line that doesn't match the tag.
//
void CDirectCache::WriteWord(uint32_t addr, uint32_t word)
{
  uint32_t word_sel, tag_sel, tag;

  // Spilt the address up into the bits we want 
  word_sel = addr & 0x00000003;
  tag_sel = (addr >> 2) & m_tagMask;
  tag = (addr >> (m_tagBits + 2));
  
  if (m_pTagRAM[tag_sel] != tag)
    throw CCacheMiss(addr);

  m_pDataRAM[(tag_sel * 4) + word_sel] = word;
}


///////////////////////////////////////////////////////////////////////////////
// Reset - Marks all the tags as being invalid
//
void CDirectCache::Reset()
{
  for (uint32_t i = 0; i < (m_nSize / LINE_SIZE_B); i++)
    m_pTagRAM[i] = INVALID_BIT;
}


