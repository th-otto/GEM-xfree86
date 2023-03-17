/* -*- mode: C; c-basic-offset:8 -*- */
/*
 * GLX Hardware Device Driver for Intel i810
 * Copyright (C) 1999 Keith Whitwell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "mm.h"
#include "i810dd.h"
#include "i810lib.h"
#include "i810state.h"
#include "i810dma.h"





#if 0
static void mga_vertex_dma_ioctl(mgaContextPtr mmesa)
{
   int retcode;
   int size = MGA_DMA_BUF_SZ;
   drmDMAReq dma;
   drmBufPtr buf = mmesa->dma_buffer;

   dma.context = mmesa->hHWContext;
   dma.send_count = 1;
   dma.send_list = &buf->idx;
   dma.send_sizes = &size;
   dma.flags = DRM_DMA_WAIT;
   dma.request_count = 0;
   dma.request_size = 0;
   dma.request_list = 0;
   dma.request_sizes = 0;

   if ((retcode = drmDMA(mmesa->driFd, &dma))) {
      printf("send iload retcode = %d\n", retcode);
      exit(1);
   }
}



void i810FlushVerticesLocked( i810ContextPtr imesa )
{
	
}


void i810FlushVertices( i810ContextPtr imesa ) {
	LOCK_HARDWARE( imesa );
	i810FlushVerticesLocked( imesa );
	UNLOCK_HARDWARE( imesa );
}

#endif


/*
 * i810DmaFinish
 */
void i810DmaFinish( i810ContextPtr imesa ) {
#if I810_USE_BATCH
#else
	_I810Sync( imesa );
#endif
}



void i810DmaFlush( i810ContextPtr imesa ) {
#if I810_USE_BATCH
#else
#endif
}

