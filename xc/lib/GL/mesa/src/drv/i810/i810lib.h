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
 *
 */

#ifndef I810LIB_INC
#define I810LIB_INC

#include <stdio.h>

#include "i810context.h"
#include "mm.h"
#include "i810log.h"


struct i810_mem_range {
   unsigned long Start;
   unsigned long End;
   unsigned long Size;
};

struct i810_batch_buffer;

struct i810_ring_buffer {
   int tail_mask;
   struct i810_mem_range mem;
   char *virtual_start;
   int head;
   int tail;
   int space;
   int synced;
};

typedef struct {
   /* logging stuff */
   GLuint logLevel;
   FILE *logFile;

   /* bookkeeping for texture swaps */
   GLuint dma_buffer_age;
   GLuint current_texture_age;
        
   /* options */
   GLuint nullprims;		/* skip all primitive generation */
   GLuint boxes;		/* draw performance boxes */
   GLuint noFallback;		/* don't fall back to software */
   GLuint skipDma;		/* don't send anything to hardware */

   /* performance counters */	
   GLuint c_setupPointers;
   GLuint c_triangles;
   GLuint c_points;
   GLuint c_lines;
   GLuint c_drawWaits;
   GLuint c_textureSwaps;
   GLuint c_dmaFlush;
   GLuint c_overflows;

   GLuint c_ringlost;
   GLuint c_texlost;
   GLuint c_ctxlost;
	
   GLuint hardwareWentIdle;	/* cleared each swapbuffers, set if a
				   waitfordmacompletion ever exited
				   without having to wait */

   unsigned char *texVirtual;

   
   struct i810_batch_buffer *dma_buffer;
   struct i810_ring_buffer LpRing;
   unsigned char *MMIOBase;

} i810Glx_t;

extern i810Glx_t	i810glx;


#define I810PACKCOLOR1555(r,g,b,a) \
  ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) | \
    ((a) ? 0x8000 : 0))

#define I810PACKCOLOR565(r,g,b) \
  ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define I810PACKCOLOR4444(r,g,b,a) \
  ((((a) & 0xf0) << 8) | (((r) & 0xf0) << 4) | ((g) & 0xf0) | ((b) >> 4))


#include "i810_3d_reg.h"

static __inline__ GLuint i810PackColor(GLuint format, 
				       GLubyte r, GLubyte g, 
				       GLubyte b, GLubyte a)
{
  switch (format) {
  case DV_PF_555:
    return I810PACKCOLOR1555(r,g,b,a);
  case DV_PF_565:
    return I810PACKCOLOR565(r,g,b);
  default:
     fprintf(stderr, "unknown format %d\n", (int)format);
    return 0;
  }
}




#endif
