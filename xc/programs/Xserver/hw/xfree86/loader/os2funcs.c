/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/os2funcs.c,v 1.4 1997/03/10 10:12:22 hohndel Exp $ */
/*
 * (c) Copyright 1997 by Sebastien Marineau
 *                      <marineau@genie.uottawa.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * SEBASTIEN MARINEAU BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Sebastien Marineau shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Sebastien Marineau.
 *
 */


/* Implements some OS/2 memory allocation functions to allow 
 * execute permissions for modules. We allocate some mem using DosAllocMem
 * and then use the EMX functions to create a heap from which we allocate
 * the requests. We create a heap of 2 megs, hopefully enough for now.
 */

#define INCL_DOSMEMMGR
#include <os2.h>
#include <sys/types.h>
#include <umalloc.h>
#include "os.h"

#define RESERVED_BLOCKS 512  /* reserve 2MB memory for modules */

void *os2loader_AddToHeap(Heap_t, size_t *, int *);
void os2loader_RemoveFromHeap(Heap_t, void *, size_t);

PVOID os2loader_CommitedTop;
PVOID os2loader_baseAddress;
Heap_t os2loader_heapAddress;
int os2loader_TotalCommitedBlocks;

void *os2loader_calloc(size_t num_elem, size_t size_elem){
APIRET rc;
int ret;
static BOOL FirstTime=TRUE;
void *allocMem;

if(FirstTime){
	rc=DosAllocMem(&os2loader_baseAddress,RESERVED_BLOCKS * 4096, 
                PAG_READ | PAG_WRITE | PAG_EXECUTE);
	if(rc!=0) {
		ErrorF("OS/2AllocMem: Could not create heap for module loading\n");
		return(NULL);
		}

/* Now commit the first 128Kb, the rest will be done dynamically */
        rc=DosSetMem(os2loader_baseAddress,32*4096, PAG_DEFAULT | PAG_COMMIT); 
        	if(rc!=0) {
		ErrorF("OS/2AllocMem: Could not commit heap memory!\n");
		DosFreeMem(os2loader_baseAddress);
                return(NULL);
		}
        os2loader_CommitedTop=os2loader_baseAddress + 32*4096;
        os2loader_TotalCommitedBlocks=32;
	ErrorF("OS2Alloc: allocated mem for heap, rc=%d, addr=%p\n",rc,os2loader_baseAddress);

	if((os2loader_heapAddress=_ucreate(os2loader_baseAddress,32*4096,_BLOCK_CLEAN,
		_HEAP_REGULAR,os2loader_AddToHeap, os2loader_RemoveFromHeap))==NULL){
		ErrorF("OS/2AllocMem: Could not create heap for loadable modules\n");
		DosFreeMem(os2loader_baseAddress);
		return(NULL);
		}
	
	ret=_uopen(os2loader_heapAddress);
	if(ret!=0){
		ErrorF("OS/2AllocMem: Could not open heap for loadable modules\n");
		ret=_udestroy(os2loader_heapAddress,_FORCE);
		DosFreeMem(os2loader_baseAddress);
		return(NULL);
		}
	FirstTime=FALSE;
	ErrorF("OS/2: done creating heap, addr=%p\n",os2loader_heapAddress);
	}

allocMem=_ucalloc(os2loader_heapAddress,num_elem,size_elem);
return(allocMem);
}


void *os2loader_AddToHeap(Heap_t H, size_t *new_size, int *PCLEAN)
{
PVOID NewBase;
long adjusted_size;
long blocks;
APIRET rc;

   if(H != os2loader_heapAddress){
      ErrorF("OS/2: Tried to grow an inexistant heap, p=%08x\n",H);
      return (NULL);
      }
   NewBase=os2loader_CommitedTop;
   adjusted_size = (*new_size/65536) * 65536;
   if((*new_size % 65536)> 0 ) adjusted_size += 65536;
   blocks=adjusted_size / 4096;
   if((os2loader_TotalCommitedBlocks + blocks)>RESERVED_BLOCKS){
      ErrorF("OS/2 GrowHeap: Could not allocate any more memory for module loading!\n");
      ErrorF("Total reserved memory of %ld bytes is exhausted\n",RESERVED_BLOCKS * 4096);
      return(NULL);
      }
   rc = DosSetMem(NewBase, adjusted_size, PAG_DEFAULT | PAG_COMMIT);
   if(rc!=0) {
	ErrorF("OS/2 GrowHeap: Could not grow heap! Requested size %d, \n", adjusted_size);
        return(NULL);
	}
   os2loader_CommitedTop+=adjusted_size;
   os2loader_TotalCommitedBlocks += blocks;
   *PCLEAN = _BLOCK_CLEAN;
   *new_size=adjusted_size;
   ErrorF("OS/2: Added %d bytes to heap, addr %08x\n",adjusted_size, NewBase);
   return(NewBase);
}

void os2loader_RemoveFromHeap(Heap_t H, void *memory, size_t size)
{
   if(H != os2loader_heapAddress){
      ErrorF("OS/2: Tried to shrink an inexistant heap, p=%08x\n",H);
      return;
      }
/* Currently we do nothing, as we do not keep track of the commited memory */
ErrorF("OS/2: module heap requests that heap memory be deallocated. Request ignored\n");

/* Only handle it if it is the base address */
   if(memory == os2loader_baseAddress) {
        DosFreeMem(os2loader_baseAddress);
        ErrorF("OS/2: total heap area was deallocated\n");
        }
}
