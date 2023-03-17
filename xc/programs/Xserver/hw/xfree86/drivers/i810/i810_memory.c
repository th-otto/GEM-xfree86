
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keithw@precisioninsight.com>
 *
 */


#include "X.h"
#include "input.h"
#include "screenint.h"
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

/*  #include "xf86_ansic.h" */

#include "i810.h"
#include "i810_reg.h"

#include <linux/agpgart.h>

int I810AllocLow( I810MemRange *result, I810MemRange *pool, int size )
{
   if (size > pool->Size) return 0;

   pool->Size -= size;
   result->Size = size;
   result->Start = pool->Start;
   result->End = pool->Start += size;
   return 1;
}


int I810AllocHigh( I810MemRange *result, I810MemRange *pool, int size )
{
   if (size > pool->Size) return 0;

   pool->Size -= size;
   result->Size = size;
   result->End = pool->End;
   result->Start = pool->End -= size;
   return 1;
}


#define XCONFIG_PROBED "()"
#define NAME "i810"

extern int xf86open(const char *path, int flags, ...);
extern int xf86close(int fd );
extern int xf86ioctl(int fd, unsigned long request, pointer argp);
extern int xf86errno;


int I810AllocateGARTMemory( ScrnInfoPtr pScrn ) 
{
   struct _agp_info agpinf;
   struct _agp_bind bind;
   struct _agp_allocate alloc;
   int pages = pScrn->videoRam / 4; 
   I810Ptr pI810 = I810PTR(pScrn);
   long tom = 0;
   int gartfd = -1;

   gartfd = xf86open("/dev/agpgart", O_RDWR, 0);

   if (gartfd == -1) {	
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "unable to open /dev/agpgart\n");
      return FALSE;
   }

   if (xf86ioctl(gartfd, AGPIOC_ACQUIRE, 0) != 0) {
      if(pI810->agpAcquired2d == TRUE) {
	 xf86close(gartfd);
	 return TRUE;
      }
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "AGPIOC_ACQUIRE failed\n"); 
      return FALSE;
   }

   /* This allows the 2d only Xserver to regen */
   pI810->agpAcquired2d = TRUE;
   pI810->gartfd = gartfd;
   
   if (xf86ioctl(pI810->gartfd, AGPIOC_INFO, &agpinf) != 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		 "error doing xf86ioctl(AGPIOC_INFO)\n");
      return FALSE;
   }
   
   if (agpinf.version.major != 0 ||
      agpinf.version.minor != 99) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		 "Agp kernel driver version not correct\n");
      return FALSE;
   }
   

   /* Treat the gart like video memory - we assume we own all that is
    * there, so ignore EBUSY errors.  Don't try to remove it on
    * failure, either, as other X server may be using it.
    */
   alloc.pg_count = pages;
   alloc.type = 0;

   if (xf86ioctl(pI810->gartfd, AGPIOC_ALLOCATE, &alloc) != 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		 "AGPGART: allocation of %d pages failed\n", pages);
      return FALSE;
   }	

   bind.pg_start = 0;
   bind.key = alloc.key;

   if (xf86ioctl(pI810->gartfd, AGPIOC_BIND, &bind) != 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		 "GART: allocation of %d pages failed\n", pages);
      return FALSE;
   }	


   pI810->SysMem.Start = 0;
   pI810->SysMem.Size = pages * 4096;
   pI810->SysMem.End = pages * 4096;
   pI810->SavedSysMem = pI810->SysMem;

   tom = pI810->SysMem.End;

   pI810->DcacheMem.Start = 0;
   pI810->DcacheMem.End = 0;
   pI810->DcacheMem.Size = 0;
   pI810->CursorPhysical = 0;

   /* Dcache - half the speed of normal ram, so not really useful for
    * a 2d server.  Don't bother reporting its presence.  This is
    * mapped in addition to the requested amount of system ram.
    */
   alloc.pg_count = 1024;
   alloc.type = 1;

   /* Keep it 512K aligned for the sake of tiled regions.
    */
   tom += 0x7ffff;
   tom &= ~0x7ffff;

   if (xf86ioctl(pI810->gartfd, AGPIOC_ALLOCATE, &alloc) == 0) {
      bind.pg_start = tom / 4096;
      bind.key = alloc.key;

      if (xf86ioctl(pI810->gartfd, AGPIOC_BIND, &bind) != 0) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		    "GART: allocation of %d DCACHE pages failed\n", 
		    alloc.pg_count);
      }	else {
	 pI810->DcacheMem.Start = tom;
	 pI810->DcacheMem.Size = 1024 * 4096;
	 pI810->DcacheMem.End = pI810->DcacheMem.Start + pI810->DcacheMem.Size;
	 tom = pI810->DcacheMem.End;
      }
   }


   /* Mouse cursor -- The i810 (crazy) needs a physical address in
    * system memory from which to upload the cursor.  We get this from 
    * the agpgart module using a special memory type.
    */
   alloc.pg_count = 1;
   alloc.type = 2;


   if (xf86ioctl(pI810->gartfd, AGPIOC_ALLOCATE, &alloc) != 0) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		    "GART: No physical memory available for mouse\n", 
		    alloc.pg_count);
   } else {
      bind.pg_start = tom / 4096;
      bind.key = alloc.key;

      if (xf86ioctl(pI810->gartfd, AGPIOC_BIND, &bind) != 0) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		    "GART: allocation of %d physical pages failed\n", 
		    alloc.pg_count);
      }	else {
	 pI810->CursorPhysical = alloc.physical;
	 pI810->CursorStart = tom;
	 
	 tom += 4096;
      }
   }



   return TRUE;
}



void I810FreeGARTMemory( ScrnInfoPtr pScrn ) 
{
   I810Ptr pI810 = I810PTR(pScrn);

   if (pI810->gartfd != -1) {
      xf86close( pI810->gartfd );
      pI810->gartfd = -1;
   }
}



/* Tiled memory is good... really, really good...
 *
 * Need to make it less likely that we miss out on this - probably
 * need to move the frontbuffer away from the 'guarenteed' alignment
 * of the first memory segment, or perhaps allocate a discontigous
 * framebuffer to get more alignment 'sweet spots'.  
 */
void I810SetTiledMemory(ScrnInfoPtr pScrn, 
			int nr, 
			unsigned start,
			unsigned pitch,
			unsigned size)
{
   I810Ptr pI810 = I810PTR(pScrn);
   I810RegPtr i810Reg = &pI810->ModeReg;
   CARD32 val;

   if (nr < 0 || nr > 7) {
      ErrorF("I810SetTiledMemory - fence %d out of range\n", nr);
      return;
   }

   i810Reg->Fence[nr] = 0;

   if (start & ~FENCE_START_MASK) {
      ErrorF("I810SetTiledMemory %d: start (%x) is not 512k aligned\n", 
	     nr, start);
      return;
   }
   if (start % size) {
      ErrorF("I810SetTiledMemory %d: start (%x) is not size (%x) aligned\n",
	     nr, start, size);
      return;
   }

   if (pitch & 127) {
      ErrorF("I810SetTiledMemory %d: pitch (%x) not a multiple of 128 bytes\n",
	     nr, pitch);
      return;
   }

   val = (start | FENCE_X_MAJOR | FENCE_VALID);
   
   switch (size) {
   case (512*1024): val |= FENCE_SIZE_512K; break;
   case (1024*1024): val |= FENCE_SIZE_1M; break;
   case (2*1024*1024): val |= FENCE_SIZE_2M; break;
   case (4*1024*1024): val |= FENCE_SIZE_4M; break;
   case (8*1024*1024): val |= FENCE_SIZE_8M; break;
   case (16*1024*1024): val |= FENCE_SIZE_16M; break;
   case (32*1024*1024): val |= FENCE_SIZE_32M; break;
   default:
      ErrorF("I810SetTiledMemory %d: illegal size (%x)\n");
      return;
   }

   switch (pitch/128) {
   case 1: val |= FENCE_PITCH_1; break;
   case 2: val |= FENCE_PITCH_2; break;
   case 4: val |= FENCE_PITCH_4; break;
   case 8: val |= FENCE_PITCH_8; break;
   case 16: val |= FENCE_PITCH_16; break;
   case 32: val |= FENCE_PITCH_32; break;
   default:
      ErrorF("I810SetTiledMemory %d: illegal size (%x)\n");
      return;
   }

   i810Reg->Fence[nr] = val;
}
