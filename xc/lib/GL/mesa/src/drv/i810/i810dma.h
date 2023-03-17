/*
  GLX Hardware Device Driver for Intel i810
  Copyright (C) 1999 Keith Whitwell <keithw@precisioninsight.com>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  
 original by Jeff Hartmann <slicer@ionet.net>
  6/16/99: rewrite by John Carmack <johnc@idsoftware.com>
  Oct 99: port to i180 by Keith Whitwell <keithw@precisioninsight.com>
*/

#ifndef I810DMA_H
#define I810DMA_H

#include "i810lib.h"
#include "mm.h"

/* a flush command will guarantee that all data added to the dma buffer
is on its way to the card, and will eventually complete with no more
intervention.
*/
void i810DmaFlush( i810ContextPtr imesa );

/* the overflow function is called when a block can't be allocated
in the current dma buffer.  It flushes the current buffer and
records some information */
void i810DmaOverflow(int newDwords);

/* a finish command will guarantee that all dma commands have actually
been consumed by the card.  Note that there may still be a couple primitives
that have not yet been executed out of the internal FIFO, so this does not
guarantee that the drawing engine is idle. */
void i810DmaFinish( i810ContextPtr imesa );



typedef struct {
   unsigned long Start;
   unsigned long End;
   unsigned long Size;
} I810MemRange;



typedef struct i810_batch_buffer {
   I810MemRange mem;
   char *virtual_start;
   int head;
   int space;
   int additional_space;
   int texture_age;
} i810BatchBuffer;



#define I810_USE_BATCH 0
#if I810_USE_BATCH

#define BEGIN_BATCH( imesa, n )						\
   unsigned int outbatch;						\
   volatile char *virt;							\
   if (I810_DEBUG & DEBUG_VERBOSE_RING)					\
      fprintf(stderr,							\
      "BEGIN_BATCH(%d) in %s\n"						\
      "(spc left %d/%ld, head %x vstart %p start %lx)\n",		\
      n, __FUNCTION__, i810glx.dma_buffer->space,			\
      i810glx.dma_buffer->mem.Size - i810glx.dma_buffer->head,		\
      i810glx.dma_buffer->head,						\
      i810glx.dma_buffer->virtual_start,				\
      i810glx.dma_buffer->mem.Start );					\
   if (i810glx.dma_buffer->space < n*4)					\
       i810DmaOverflow(n);						\
   outbatch = i810glx.dma_buffer->head;					\
   virt = i810glx.dma_buffer->virtual_start;			


#define OUT_BATCH(val) {					\
   *(volatile unsigned int *)(virt + outbatch) = val;		\
   if (I810_DEBUG & DEBUG_VERBOSE_RING)				\
      fprintf(stderr, "OUT_BATCH %x: %x\n", (int)(outbatch/4), (int)(val));	\
   outbatch += 4;						\
}
   

#define ADVANCE_BATCH() {						\
   if (I810_DEBUG & DEBUG_VERBOSE_RING)					\
      fprintf(stderr, "ADVANCE_BATCH(%f) in %s\n",			\
                  (outbatch - i810glx.dma_buffer->head) / 4.0,		\
                  __FUNCTION__);					\
   i810glx.dma_buffer->space -= outbatch - i810glx.dma_buffer->head;	\
   i810glx.dma_buffer->head = outbatch;					\
}

#define FINISH_PRIM()

#else


/* As an alternate path to dma, we can export the registers (and hence
 * ringbuffer) to the client.  For security we should alloc these
 * things in agp memory which isn't exported to the client.  But then
 * again, the client could blit over such things if it wanted to hang
 * the system.
 *
 * Malicious clients aren't really delt with by the DRI.
 */


extern void _I810RefreshLpRing( i810ContextPtr imesa, int update );
extern int _I810WaitLpRing( i810ContextPtr imesa, int n, int timeout_usec );
extern int _I810Sync( i810ContextPtr imesa );


#define OUT_RING(n) {					\
   if (I810_DEBUG & DEBUG_VERBOSE_RING)			\
      fprintf(stderr, "OUT_RING %x: %x\n", outring, (int)(n));	\
   if (!(I810_DEBUG & DEBUG_NO_OUTRING)) {		\
      *(volatile unsigned int *)(virt + outring) = n;	\
      outring += 4;					\
      outring &= ringmask;				\
   }							\
}

#define ADVANCE_LP_RING() {			\
    i810glx.LpRing.tail = outring;		\
    OUTREG(LP_RING + RING_TAIL, outring);	\
}

#define BEGIN_LP_RING(imesa, n)						\
   unsigned int outring, ringmask;					\
   volatile char *virt;							\
   if ((I810_DEBUG&DEBUG_ALWAYS_SYNC) && n>2) _I810Sync( imesa );	\
   if (i810glx.LpRing.space < n*4) _I810WaitLpRing( imesa, n*4, 0);	\
   i810glx.LpRing.space -= n*4;						\
   if (I810_DEBUG & DEBUG_VERBOSE_RING) 				\
      fprintf(stderr, "BEGIN_LP_RING %d in %s\n", n, __FUNCTION__);	\
   if (I810_DEBUG & DEBUG_VALIDATE_RING) { 				\
      CARD32 tail = INREG(LP_RING+RING_TAIL);				\
      if (tail != i810glx.LpRing.tail) {				\
	 fprintf(stderr, "tail %x pI810->LpRing.tail %x\n",		\
		    (int) tail, (int) i810glx.LpRing.tail);				\
         exit(1);							\
      }									\
   }									\
   i810glx.LpRing.synced = 0;						\
   outring = i810glx.LpRing.tail;					\
   ringmask = i810glx.LpRing.tail_mask;					\
   virt = i810glx.LpRing.virtual_start;			


#define EXTRAAA \

#define INREG(addr)         *(volatile CARD32 *)(i810glx.MMIOBase + (addr))
#define INREG16(addr)       *(volatile CARD16 *)(i810glx.MMIOBase + (addr))
#define INREG8(addr)        *(volatile CARD8  *)(i810glx.MMIOBase + (addr))

#define OUTREG(addr, val) do {					\
   if (I810_DEBUG&DEBUG_VERBOSE_OUTREG)				\
      fprintf(stderr, "OUTREG(%x, %x)\n", addr, val);		\
   if (!(I810_DEBUG&DEBUG_NO_OUTREG))				\
      *(volatile CARD32 *)(i810glx.MMIOBase + (addr)) = (val);	\
} while (0)


#define BEGIN_BATCH(imesa, n) BEGIN_LP_RING(imesa, n)
#define ADVANCE_BATCH() ADVANCE_LP_RING()
#define OUT_BATCH(val) OUT_RING(val)
#define FINISH_PRIM() OUTREG(LP_RING + RING_TAIL, i810glx.LpRing.tail)

#endif



#endif
