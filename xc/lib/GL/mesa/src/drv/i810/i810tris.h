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

#ifndef I810TRIS_INC
#define I810TRIS_INC

#include "types.h"
#include "i810dma.h"

extern void i810PrintRenderState( const char *msg, GLuint state );
extern void i810DDChooseRenderState(GLcontext *ctx);
extern void i810DDTrifuncInit( void );

extern GLuint *i810AllocPrimitiveVerts( i810ContextPtr imesa, int dwords );



/* Todo: 
 *    - multidraw, ...
 *    - Antialiasing (?)
 *    - line and polygon stipple
 *    - select and feedback 
 *    - stencil 
 *    - point parameters
 *    - 
 */
#define I810_ANTIALIAS_BIT   0       /* ignored for now, no fallback */
#define I810_FLAT_BIT 	     0x1
#define I810_OFFSET_BIT	     0x2	
#define I810_TWOSIDE_BIT     0x4
#define I810_NODRAW_BIT	     0x8
#define I810_FALLBACK_BIT    0x10

/* Not in use:
 */
#define I810_FEEDBACK_BIT    0x20
#define I810_SELECT_BIT      0x40
#define I810_POINT_PARAM_BIT 0x80	/* not needed? */



static void __inline__ i810_draw_triangle( i810ContextPtr imesa,
					   i810_vertex *v0, 
					   i810_vertex *v1, 
					   i810_vertex *v2 )
{
   i810_vertex *wv = (i810_vertex *)i810AllocPrimitiveVerts( imesa, 30 );
   wv[0] = *v0;
   wv[1] = *v1;
   wv[2] = *v2;
   FINISH_PRIM();
}




/* These can go soon, but for the meantime we're using triangles for 
 * everything.
 */
static __inline__ void i810_draw_point( i810ContextPtr imesa,
					i810_vertex *tmp, float sz )
{
   i810_vertex *wv = (i810_vertex *)i810AllocPrimitiveVerts( imesa, 6*10 );

   wv[0] = *tmp;
   wv[0].x = tmp->x - sz;
   wv[0].y = tmp->y - sz;

   wv[1] = *tmp;
   wv[1].x = tmp->x + sz;
   wv[1].y = tmp->y - sz;

   wv[2] = *tmp;
   wv[2].x = tmp->x + sz;
   wv[2].y = tmp->y + sz;

   wv[3] = *tmp;
   wv[3].x = tmp->x + sz;
   wv[3].y = tmp->y + sz;

   wv[4] = *tmp;
   wv[4].x = tmp->x - sz;
   wv[4].y = tmp->y + sz;

   wv[5] = *tmp;
   wv[5].x = tmp->x - sz;
   wv[5].y = tmp->y - sz;

   FINISH_PRIM();
}


static __inline__ void i810_draw_line( i810ContextPtr imesa, 
				       i810_vertex *tmp0, 
				       i810_vertex *tmp1,
				       float width )
{
   i810_vertex *wv = (i810_vertex *)i810AllocPrimitiveVerts( imesa, 6 * 10 );
   float dx, dy, ix, iy;

   dx = tmp0->x - tmp1->x;
   dy = tmp0->y - tmp1->y;

   ix = width * .5; iy = 0;
   if (dx * dx > dy * dy) {
      iy = ix; ix = 0;
   }

   wv[0] = *tmp0;
   wv[0].x = tmp0->x - ix;
   wv[0].y = tmp0->y - iy;

   wv[1] = *tmp1;
   wv[1].x = tmp1->x + ix;
   wv[1].y = tmp1->y + iy;

   wv[2] = *tmp0;
   wv[2].x = tmp0->x + ix;
   wv[2].y = tmp0->y + iy;
	 
   wv[3] = *tmp0;
   wv[3].x = tmp0->x - ix;
   wv[3].y = tmp0->y - iy;

   wv[4] = *tmp1;
   wv[4].x = tmp1->x - ix;
   wv[4].y = tmp1->y - iy;

   wv[5] = *tmp1;
   wv[5].x = tmp1->x + ix;
   wv[5].y = tmp1->y + iy;

   FINISH_PRIM();
}

#endif
