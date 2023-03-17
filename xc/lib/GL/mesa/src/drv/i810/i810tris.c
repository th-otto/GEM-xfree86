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

#include <stdio.h>
#include <math.h>

#include "vb.h"
#include "pipeline.h"

#include "mm.h"
#include "i810lib.h"
#include "i810tris.h"
#include "i810vb.h"
#include "i810log.h"


static void i810_null_quad( GLcontext *ctx, GLuint v0,
			    GLuint v1, GLuint v2, GLuint v3, GLuint pv )
{
}     

static void i810_null_triangle( GLcontext *ctx, GLuint v0,
				GLuint v1, GLuint v2, GLuint pv )
{
}     

static void i810_null_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv ) 
{
}

static void i810_null_points( GLcontext *ctx, GLuint first, GLuint last ) 
{
}


#define I810_COLOR(to, from) {			\
  (to)[0] = (from)[2];				\
  (to)[1] = (from)[1];				\
  (to)[2] = (from)[0];				\
  (to)[3] = (from)[3];				\
}



static triangle_func tri_tab[0x20];   
static quad_func     quad_tab[0x20];  
static line_func     line_tab[0x20];  
static points_func   points_tab[0x20];

void i810PrintRenderState( const char *msg, GLuint state )
{
   fprintf(stderr, "%s: (%x) %s%s%s%s%s%s\n",
	   msg, (int) state,
	   (state & I810_FLAT_BIT)       ? "flat, "       : "",
	   (state & I810_OFFSET_BIT)     ? "offset, "     : "",
	   (state & I810_TWOSIDE_BIT)    ? "twoside, "    : "",
	   (state & I810_ANTIALIAS_BIT)  ? "antialias, "  : "",
	   (state & I810_NODRAW_BIT)     ? "no-draw, "    : "",
	   (state & I810_FALLBACK_BIT)   ? "fallback"     : "");
}

#define IND (0)
#define TAG(x) x
#include "i810tritmp.h"

#define IND (I810_FLAT_BIT)
#define TAG(x) x##_flat
#include "i810tritmp.h"

#define IND (I810_OFFSET_BIT)
#define TAG(x) x##_offset
#include "i810tritmp.h"

#define IND (I810_OFFSET_BIT|I810_FLAT_BIT)
#define TAG(x) x##_offset_flat
#include "i810tritmp.h"

#define IND (I810_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "i810tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_FLAT_BIT)
#define TAG(x) x##_twoside_flat
#include "i810tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "i810tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_OFFSET_BIT|I810_FLAT_BIT)
#define TAG(x) x##_twoside_offset_flat
#include "i810tritmp.h"

void i810DDTrifuncInit()
{
   int i;
   init();
   init_flat();
   init_offset();
   init_offset_flat();
   init_twoside();
   init_twoside_flat();
   init_twoside_offset();
   init_twoside_offset_flat();

   /* Hmmm...
    */
   for (i = 0 ; i < 0x20 ; i++) {
      if (i & ~I810_FLAT_BIT) {
	 points_tab[i] = points_tab[i&I810_FLAT_BIT];
	 line_tab[i] = line_tab[i&I810_FLAT_BIT];
      }
   }

   for (i = 0 ; i < 0x20 ; i++) 
      if ((i & (I810_NODRAW_BIT|I810_FALLBACK_BIT)) == I810_NODRAW_BIT ||
	  i810glx.nullprims) 
      {
	 quad_tab[i] = i810_null_quad; 
	 tri_tab[i] = i810_null_triangle; 
	 line_tab[i] = i810_null_line;
	 points_tab[i] = i810_null_points;
      }

   if (i810glx.noFallback) {
      for (i = 0 ; i < 0x10 ; i++) {
	 points_tab[i|I810_FALLBACK_BIT] = points_tab[i];
	 line_tab[i|I810_FALLBACK_BIT] = line_tab[i];
	 tri_tab[i|I810_FALLBACK_BIT] = tri_tab[i];
	 quad_tab[i|I810_FALLBACK_BIT] = quad_tab[i];
      }
   }

}


/* Everything is done via single triangle instructiopns at the moment;
 * this can change fairly easily.
 */
#if I810_USE_BATCH

GLuint *i810AllocPrimitiveVerts( i810ContextPtr imesa, int dwords ) 
{
   GLuint orig_dwords = dwords;

   dwords+=2;
   dwords&=~1;

   while (1) {
      if (i810glx.dma_buffer->space < dwords * 4) 
      {
	 if (I810_DEBUG & DEBUG_VERBOSE_RING)
	    fprintf(stderr, "i810AllocPrimitiveVerts: dma buffer overflow\n");
	 i810DmaOverflow( dwords );
      }
      else
      {
	 GLuint start = i810glx.dma_buffer->head;
	 i810glx.dma_buffer->head += dwords * 4;
	 i810glx.dma_buffer->space -= dwords * 4;

	 if ((orig_dwords & 1) == 0) {
	    *(GLuint *)(i810glx.dma_buffer->virtual_start + start ) = 0;
	    start += 4;
	 }

	 *(GLuint *)(i810glx.dma_buffer->virtual_start + start ) = 
	    GFX_OP_PRIMITIVE | PR_TRIANGLES | (orig_dwords-1);

	 return (GLuint *)(i810glx.dma_buffer->virtual_start + start + 4);
      }
   }
}

#else

/* Hardware lock is held.
 */
GLuint *i810AllocPrimitiveVerts( i810ContextPtr imesa, int dwords ) 
{
   GLuint orig_dwords = dwords;
   
   dwords+=2;
   dwords&=~1;

   while (1)
   {
      BEGIN_LP_RING( imesa, dwords );

      if (outring + dwords * 4 != ((outring + dwords * 4) & ringmask)) 
      {
	 int i;

	 if (I810_DEBUG & DEBUG_VERBOSE_RING)
	    fprintf(stderr, "\n\nwrap case in i810AllocPrimitiveVerts\n\n");
	 
	 for (i = 0 ; i < dwords ; i++)
	    OUT_RING( 0 );
	 ADVANCE_LP_RING();
      }
      else 
      {
	 i810glx.LpRing.tail = outring + dwords * 4;
	 
	 if ((orig_dwords & 1) == 0)
	    OUT_RING(0);
	 
	 OUT_RING( GFX_OP_PRIMITIVE | PR_TRIANGLES | (orig_dwords-1) );
	 return (GLuint *)(virt + outring);
      }
   }
}

#if 0
void i810FinishPrim( void )
{
   char *virt = i810glx.LpRing.virtual_start;

   for (i = i810glx.LpRing.head ; i != i810glx.LpRing.tail ; i += 4) 
      fprintf(stderr, "prim %x (%f)", *(virt + i), *(float *)(virt + i));

   OUTREG(LP_RING + RING_TAIL, i810glx.LpRing.tail);
}
#endif 
#endif



void i810DDChooseRenderState( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLuint flags = ctx->TriangleCaps;

   ctx->IndirectTriangles &= ~DD_SW_RASTERIZE;

   if (flags) {
      GLuint ind = 0;
      GLuint shared = 0;
      GLuint setup = imesa->setupindex;
      GLuint fallback = I810_FALLBACK_BIT; 

      if (i810glx.noFallback) fallback = 0; 

      if ((flags & DD_FLATSHADE) && (setup & I810_RGBA_BIT)) shared |= I810_FLAT_BIT;
      if (flags & DD_MULTIDRAW)                    shared |= fallback;
      if (flags & DD_SELECT)                       shared |= I810_FALLBACK_BIT;
      if (flags & DD_FEEDBACK)                     shared |= I810_FALLBACK_BIT;

      ind = shared;
      if (flags & DD_POINT_SMOOTH)                 ind |= I810_ANTIALIAS_BIT;
      if (flags & DD_POINT_ATTEN)                  ind |= fallback;

      imesa->renderindex = ind;
      imesa->PointsFunc = points_tab[ind]; 
      if (ind & I810_FALLBACK_BIT)  
	 ctx->IndirectTriangles |= DD_POINT_SW_RASTERIZE;

      ind = shared;
      if (flags & DD_LINE_SMOOTH)                   ind |= I810_ANTIALIAS_BIT;
      if (flags & DD_LINE_STIPPLE)                  ind |= fallback;

      imesa->renderindex |= ind;
      imesa->LineFunc = line_tab[ind];
      if (ind & I810_FALLBACK_BIT) 
	 ctx->IndirectTriangles |= DD_LINE_SW_RASTERIZE;

      ind = shared;
      if (flags & DD_TRI_SMOOTH)                    ind |= I810_ANTIALIAS_BIT;
      if (flags & DD_TRI_OFFSET)                    ind |= I810_OFFSET_BIT;
      if (flags & DD_TRI_LIGHT_TWOSIDE)             ind |= I810_TWOSIDE_BIT;
      if (flags & (DD_TRI_UNFILLED|DD_TRI_STIPPLE)) ind |= fallback;	

      imesa->renderindex |= ind;
      imesa->TriangleFunc = tri_tab[ind];
      imesa->QuadFunc = quad_tab[ind];
      if (ind & I810_FALLBACK_BIT)
	 ctx->IndirectTriangles |= (DD_TRI_SW_RASTERIZE|DD_QUAD_SW_RASTERIZE);
   } 
   else if (imesa->renderindex)
   {
      imesa->renderindex  = 0;
      imesa->PointsFunc   = points_tab[0]; 
      imesa->LineFunc     = line_tab[0];
      imesa->TriangleFunc = tri_tab[0];
      imesa->QuadFunc     = quad_tab[0];
   }

   if (I810_DEBUG&DEBUG_VERBOSE_API) {
      gl_print_tri_caps("tricaps", ctx->TriangleCaps);
      i810PrintRenderState("i810: Render state", imesa->renderindex);
   }
}



