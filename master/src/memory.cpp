//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (C) 2001 Michael Dales
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// Name   : memory.cpp
// Author : Michael Dales <michael@dcs.gla.ac.uk>
// Info   : This template creates a pool of preallocaterd types to save on
//          many calls to malloc. Only drawback is that you need to know
//          in advance the maximum number you'll need.
// 
//////////////////////////////////////////////////////////////////////////////

#include "swarm.h"
#include "memory.h"


///////////////////////////////////////////////////////////////////////////////
// CMemory - Constructor
//
template<class Type>
CMemory<Type>::CMemory(int nBlocks)
{
  int i;

  m_pList = (Type*)TNEW(Type[nBlocks]);
  m_pIndex = (Type**)TNEW(Type*[nBlocks]);

  for (i = 0; i < nBlocks; i++)
    m_pIndex[i] = &m_pList[i];

  m_nMax = nBlocks;
  m_nCurrent = 0;
}


///////////////////////////////////////////////////////////////////////////////
// ~CMemory - Destructor
//
template<class Type>
CMemory<Type>::~CMemory()
{
  // We should be okay here
  ASSERT(m_nCurrent == 0);

  TDELETE(m_pList);
  TDELETE(m_pIndex);
}


///////////////////////////////////////////////////////////////////////////////
// Malloc
//
template<class Type>
Type* CMemory<Type>::Malloc()
{
  if (m_nCurrent != m_nMax)
    return m_pIndex[m_nCurrent++];
  else 
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// Free
//
template<class Type>
void CMemory<Type>::Free(Type* p)
{
  if (m_nCurrent !=0)
    m_pIndex[--m_nCurrent] = p;
}
