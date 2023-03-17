/*
 * GLX Hardware Device Driver for Matrox Millenium G200
 * Copyright (C) 1999 Wittawat Yamwong
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
 * WITTAWAT YAMWONG, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    Wittawat Yamwong <Wittawat.Yamwong@stud.uni-hannover.de>
 */

#ifndef MGATIS_INC
#define MGATIS_INC

#include "types.h"

extern void mgaDDChooseRenderState(GLcontext *ctx);
extern void mgaDDTrifuncInit( void );


/* Todo: 
 *    - multidraw, ...
 *    - Antialiasing (?)
 *    - line and polygon stipple
 *    - select and feedback 
 *    - stencil 
 *    - point parameters
 *    - 
 */
#define MGA_ANTIALIAS_BIT   0       /* ignored for now, no fallback */
#define MGA_FLAT_BIT	0x1
#define MGA_OFFSET_BIT	0x2	/* 3.1 only */
#define MGA_TWOSIDE_BIT	0x4	/* 3.1 only */
#define MGA_NODRAW_BIT	0x8
#define MGA_FALLBACK_BIT	0x10

/* Not in use:
 */
#define MGA_FEEDBACK_BIT    0x20
#define MGA_SELECT_BIT      0x40
#define MGA_POINT_PARAM_BIT 0x80	/* not needed? */




static __inline void mga_draw_triangle( mgaContextPtr mmesa,
				      mgaVertex *v0, 
				      mgaVertex *v1, 
				      mgaVertex *v2 )
{
   mgaUI32 vertsize = mmesa->vertsize;
   mgaUI32 *wv = mgaAllocVertexDwords( mmesa, 3 * vertsize );
   int j;

   for (j = 0 ; j < vertsize ; j++) 
      wv[j] = v0->ui[j];

   wv += vertsize;
   for (j = 0 ; j < vertsize ; j++) 
      wv[j] = v1->ui[j];

   wv += vertsize;
   for (j = 0 ; j < vertsize ; j++) 
      wv[j] = v2->ui[j];
}


static __inline void mga_draw_point( mgaContextPtr mmesa,
				   mgaVertex *tmp, float sz )
{
   mgaUI32 vertsize = mmesa->vertsize;
   mgaUI32 *wv = mgaAllocVertexDwords( mmesa, 6*vertsize);
   int j;

   *(float *)&wv[0] = tmp->warp1.x - sz;
   *(float *)&wv[1] = tmp->warp1.y - sz;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp->ui[j];
   wv += vertsize;

   *(float *)&wv[0] = tmp->warp1.x + sz;
   *(float *)&wv[1] = tmp->warp1.y - sz;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp->ui[j];
   wv += vertsize;

   *(float *)&wv[0] = tmp->warp1.x + sz;
   *(float *)&wv[1] = tmp->warp1.y + sz;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp->ui[j];
   wv += vertsize;

   *(float *)&wv[0] = tmp->warp1.x + sz;
   *(float *)&wv[1] = tmp->warp1.y + sz;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp->ui[j];
   wv += vertsize;

   *(float *)&wv[0] = tmp->warp1.x - sz;
   *(float *)&wv[1] = tmp->warp1.y + sz;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp->ui[j];
   wv += vertsize;

   *(float *)&wv[0] = tmp->warp1.x - sz;
   *(float *)&wv[1] = tmp->warp1.y - sz;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp->ui[j];
}


static __inline void mga_draw_line( mgaContextPtr mmesa,
				  const mgaVertex *tmp0, 
				  const mgaVertex *tmp1,
				  float width )
{
   mgaUI32 vertsize = mmesa->vertsize;
   mgaUI32 *wv = mgaAllocVertexDwords( mmesa, 6 * vertsize );
   float dx, dy, ix, iy;
   int j;

   dx = tmp0->warp1.x - tmp1->warp1.x;
   dy = tmp0->warp1.y - tmp1->warp1.y;

   ix = width * .5; iy = 0;
  
   if ((ix<.5) && (ix>0.1)) ix = .5; /* I want to see lines with width
                                        0.5 also */
  
   if (dx * dx > dy * dy) {
      iy = ix; ix = 0;
   }
  
   *(float *)&wv[0] = tmp0->warp1.x - ix;
   *(float *)&wv[1] = tmp0->warp1.y - iy;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp0->ui[j];
   wv += vertsize;

   *(float *)&wv[0] = tmp1->warp1.x + ix;
   *(float *)&wv[1] = tmp1->warp1.y + iy;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp1->ui[j];
   wv += vertsize;

   *(float *)&wv[0] = tmp0->warp1.x + ix;
   *(float *)&wv[1] = tmp0->warp1.y + iy;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp0->ui[j];
   wv += vertsize;
	 
   *(float *)&wv[0] = tmp0->warp1.x - ix;
   *(float *)&wv[1] = tmp0->warp1.y - iy;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp0->ui[j];
   wv += vertsize;

   *(float *)&wv[0] = tmp1->warp1.x - ix;
   *(float *)&wv[1] = tmp1->warp1.y - iy;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp1->ui[j];
   wv += vertsize;

   *(float *)&wv[0] = tmp1->warp1.x + ix;
   *(float *)&wv[1] = tmp1->warp1.y + iy;
   for (j = 2 ; j < vertsize ; j++) 
      wv[j] = tmp1->ui[j];
   wv += vertsize;
}




#endif
