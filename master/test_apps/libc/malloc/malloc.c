/*****************************************************************************
 * 
 * Copyright (C) 2000 Michael Dales
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * Name   : malloc.c
 * Author : Michael Dales <michael@dcs.gla.ac.uk>
 * 
 ****************************************************************************/

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <inttypes.h>

/******************************************************************************
 * This structure is used in both the used and free lists. The free list is
 * ordered both by size and address (size for quick finding of minimum free
 * space and address for reinserting freed space). In the used list the size
 * fields are unsed - we just maintain the list using the address links.
 *
 * Note that all lists are sorted in ascending order.
 */
typedef struct HNTAG
{
  struct HNTAG* pNextAddr;
  struct HNTAG* pNextSize;
  struct HNTAG* pPrevAddr;
  struct HNTAG* pPrevSize;

  void*         pHole;
  uint32_t      nLength;
} HEAP_NODE;

typedef struct HTAG
{
  HEAP_NODE* pAddrList;
  HEAP_NODE* pSizeList;
  HEAP_NODE* pUsedList;
} HEAP;

static HEAP* __heap;

void remove_from_size_list(HEAP_NODE* ptr);
void insert_in_size_list(HEAP_NODE* ptr);
void remove_from_addr_list(HEAP_NODE* ptr);
void insert_in_addr_list(HEAP_NODE* ptr);
void remove_from_used_list(HEAP_NODE* ptr);
void insert_in_used_list(HEAP_NODE* ptr);
void compact_node(HEAP_NODE* ptr);

/******************************************************************************
 *
 */
void libc_mem_init(void* buffer, uint32_t length)
{
  HEAP_NODE* pFree = (HEAP_NODE*)((char*)buffer + sizeof(HEAP));

  /* Create the empty heap list */
  if (length < (sizeof(HEAP) + sizeof(HEAP_NODE)))
    {
      /* Not enough memory for meta-info, let alone user info! */
      __heap = NULL;
      return;
    }

  /* Fill in the base node */
  pFree->pNextAddr = NULL;
  pFree->pNextSize = NULL;
  pFree->pPrevAddr = NULL;
  pFree->pPrevSize = NULL;

  pFree->pHole = (void*)((char*)buffer + sizeof(HEAP) + sizeof(HEAP_NODE));
  pFree->nLength = length - (sizeof(HEAP) + sizeof(HEAP_NODE));

  __heap = (HEAP*)buffer;
  __heap->pAddrList = pFree;
  __heap->pSizeList = pFree;
  __heap->pUsedList = NULL;
  
}


/******************************************************************************
 *
 */
void libc_mem_destroy()
{
  /* All managed within the heap, so nothing to destroy. */
}


/******************************************************************************
 *
 */
void* malloc(size_t size)
{
  HEAP_NODE* ptr;

  if (__heap == NULL)
    return NULL;

  /* To make life easier, we round the size up to the nearest word. */
  if ((size & 0xFFFFFFFC) != size) /* XXX: MWD was 0)*/
    {
      size &= 0xFFFFFFFC;
      size += 0x4;
    }

  /* Walk along size list looking for large enough hole */
  ptr = __heap->pSizeList;

  while ((ptr != NULL) && (ptr->nLength != size) && 
	 (ptr->nLength < (size + sizeof(HEAP_NODE))))
    ptr = ptr->pNextSize;

  /* Did we find a big enough hole? */
  if (ptr == NULL)
    return NULL;

  /* Was the match complete? */
  if (ptr->nLength == size)
    {
      /* complete match - just move node from free to used list */
      remove_from_addr_list(ptr);
      remove_from_size_list(ptr);
      insert_in_used_list(ptr);
      
      return ptr->pHole;
    }
  else
    {
      HEAP_NODE* pNew;

      /* Not a complete match. Split into two nodes */
      pNew = (HEAP_NODE*)((char*)ptr + ptr->nLength - (size));
      pNew->pHole = (char*)pNew + sizeof(HEAP_NODE);
      pNew->nLength = size;
      ptr->nLength -= (size + sizeof(HEAP_NODE));


      /* Put new node in used list */
      insert_in_used_list(pNew);
      
      /* Move remaining hole is size list */
      remove_from_size_list(ptr);
      insert_in_size_list(ptr);

      return pNew->pHole;
    }
}


/******************************************************************************
 *
 */
void* calloc(size_t nmemb, size_t size)
{
  void* ret;

  ret = malloc(size * nmemb);
  
  if (ret != NULL)
    bzero(ret, size * nmemb);

  return ret;
}


/******************************************************************************
 *
 */
void free(void* ptr)
{
  HEAP_NODE* cur;

  if (__heap == NULL)
    return; 

  /* Find the ptr in the used list */
  cur = __heap->pUsedList;
  while ((cur != NULL) && (cur->pHole != ptr))
    cur = cur->pNextAddr;
  
  /* Did the user try to free up something the didn't own? */
  if (cur == NULL)
    return;

  /* got it, so remove from the used list */
  remove_from_used_list(cur);

  /* Put it back in the free address list */
  insert_in_addr_list(cur);
  insert_in_size_list(cur);

  /* Now, we could just shove it back in the size list and be done, but
   * let's try a little compaction too. */
  compact_node(cur);
}


/******************************************************************************
 * compact_node - Tries to merge the previous and current records with the
 *                specified one. Assumes that the ptr is correctly in the 
 *                address sorted free list.
 */
void compact_node(HEAP_NODE* ptr)
{
  HEAP_NODE *pPrev, *pNext;

  pPrev = ptr->pPrevAddr;
  pNext = ptr->pNextAddr;

  /* First to the node before us */
  if (pPrev != NULL)
    {
      /* Calc addr at end of block and see if it's this one. */
      if (ptr == 
	  (HEAP_NODE*)((char*)pPrev + pPrev->nLength + sizeof(HEAP_NODE)))
	{
	  /* Found contiguous nodes - merge them */
	  pPrev->nLength += (sizeof(HEAP_NODE) + ptr->nLength);
	  pPrev->pNextAddr = ptr->pNextAddr;
	  if (ptr->pNextAddr != NULL)
	    ptr->pNextAddr->pPrevAddr = ptr->pPrevAddr;

	  remove_from_size_list(ptr);

	  ptr = pPrev;
	}
    }

  if (pNext != NULL)
    {
      if (pNext == (HEAP_NODE*)((char*)ptr + ptr->nLength + sizeof(HEAP_NODE)))
	{
	  /* Found contiguous nodes - merge them */
	  ptr->nLength += (sizeof(HEAP_NODE) + pNext->nLength);
	  ptr->pNextAddr = pNext->pNextAddr;
	  if (ptr->pNextAddr != NULL)
	    ptr->pNextAddr->pPrevAddr = ptr;

	  remove_from_size_list(pNext);
	}
    }

  remove_from_size_list(ptr);
  insert_in_size_list(ptr);
}


/******************************************************************************
 *
 */
void remove_from_size_list(HEAP_NODE* ptr)
{
  if (ptr->pNextSize != NULL)
    ptr->pNextSize->pPrevSize = ptr->pPrevSize;
  if (ptr->pPrevSize != NULL)
    ptr->pPrevSize->pNextSize = ptr->pNextSize;
  else
    __heap->pSizeList = ptr->pNextSize;
}


/******************************************************************************
 *
 */
void insert_in_size_list(HEAP_NODE* ptr)
{
  HEAP_NODE *prev = NULL, *cur = __heap->pSizeList;

  while ((cur != NULL) && (cur->nLength < ptr->nLength))
    {
      prev = cur;
      cur = cur->pNextSize;
    }

  /* Insert so that ...<->prev<->ptr<->cur<->... */
  ptr->pNextSize = cur;
  ptr->pPrevSize = prev;
  
  /* Are we at the start of the list? */
  if (prev == NULL)
    __heap->pSizeList = ptr;
  else
    prev->pNextSize = ptr;

  if (cur != NULL)
    cur->pPrevSize = ptr;
}


/******************************************************************************
 *
 */
void remove_from_addr_list(HEAP_NODE* ptr)
{
  if (ptr->pNextAddr != NULL)
    ptr->pNextAddr->pPrevAddr = ptr->pPrevAddr;
  if (ptr->pPrevAddr != NULL)
    ptr->pPrevAddr->pNextAddr = ptr->pNextAddr;
  else
    __heap->pAddrList = ptr->pNextAddr;
}


/******************************************************************************
 *
 */
void insert_in_addr_list(HEAP_NODE* ptr)
{
  HEAP_NODE *prev = NULL, *cur = __heap->pAddrList;

  while ((cur != NULL) && (cur->pHole < ptr->pHole))
    {
      prev = cur;
      cur = cur->pNextAddr;
    }

  /* Insert so that ...<->prev<->ptr<->cur<->... */
  ptr->pNextAddr = cur;
  ptr->pPrevAddr = prev;
  
  /* Are we at the start of the list? */
  if (prev == NULL)
    __heap->pAddrList = ptr;
  else
    prev->pNextAddr = ptr;

  if (cur != NULL)
    cur->pPrevAddr = ptr;
}


/******************************************************************************
 *
 */
void remove_from_used_list(HEAP_NODE* ptr)
{
  if (ptr->pNextAddr != NULL)
    ptr->pNextAddr->pPrevAddr = ptr->pPrevAddr;
  if (ptr->pPrevAddr != NULL)
    ptr->pPrevAddr->pNextAddr = ptr->pNextAddr;
  else
    __heap->pUsedList = ptr->pNextSize;
}

/******************************************************************************
 *
 */
void insert_in_used_list(HEAP_NODE* ptr)
{
  HEAP_NODE *prev = NULL, *cur = __heap->pUsedList;

  while ((cur != NULL) && (cur->nLength < ptr->nLength))
    {
      prev = cur;
      cur = cur->pNextAddr;
    }

  /* Insert so that ...<->prev<->ptr<->cur<->... */
  ptr->pNextAddr = cur;
  ptr->pPrevAddr = prev;
  
  /* Are we at the start of the list? */
  if (prev == NULL)
    __heap->pUsedList = ptr;
  else
    prev->pNextAddr = ptr;

  if (cur != NULL)
    cur->pPrevAddr = ptr;
}

