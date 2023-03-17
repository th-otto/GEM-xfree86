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

#include <stdio.h>
#include <math.h>

#include "vb.h"
#include "pipeline.h"

#include "mm.h"
#include "mgalib.h"
#include "mgatris.h"
#include "mgavb.h"
#include "mgalog.h"


static void mga_null_quad( GLcontext *ctx, GLuint v0,
			   GLuint v1, GLuint v2, GLuint v3, GLuint pv ) {
}     
static void mga_null_triangle( GLcontext *ctx, GLuint v0,
			       GLuint v1, GLuint v2, GLuint pv ) {
}     
static void mga_null_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv ) {
}

static void mga_null_points( GLcontext *ctx, GLuint first, GLuint last ) {
}


#define MGA_COLOR(to, from) {			\
  (to)[0] = (from)[2];				\
  (to)[1] = (from)[1];				\
  (to)[2] = (from)[0];				\
  (to)[3] = (from)[3];				\
}



static triangle_func tri_tab[0x20];   
static quad_func     quad_tab[0x20];  
static line_func     line_tab[0x20];  
static points_func   points_tab[0x20];

static void mgaPrintRenderState( const char *msg, GLuint state )
{
   mgaMsg(1, "%s: (%x) %s%s%s%s%s%s\n",
	   msg, state,
	   (state & MGA_FLAT_BIT)       ? "flat, "       : "",
	   (state & MGA_OFFSET_BIT)     ? "offset, "     : "",
	   (state & MGA_TWOSIDE_BIT)    ? "twoside, "    : "",
	   (state & MGA_ANTIALIAS_BIT)  ? "antialias, "  : "",
	   (state & MGA_NODRAW_BIT)     ? "no-draw, "    : "",
	   (state & MGA_FALLBACK_BIT)   ? "fallback"     : "");
}

#define IND (0)
#define TAG(x) x
#include "mgatritmp.h"

#define IND (MGA_FLAT_BIT)
#define TAG(x) x##_flat
#include "mgatritmp.h"

#define IND (MGA_OFFSET_BIT)
#define TAG(x) x##_offset
#include "mgatritmp.h"

#define IND (MGA_OFFSET_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_offset_flat
#include "mgatritmp.h"

#define IND (MGA_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "mgatritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_flat
#include "mgatritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "mgatritmp.h"

#define IND (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT|MGA_FLAT_BIT)
#define TAG(x) x##_twoside_offset_flat
#include "mgatritmp.h"

void mgaDDTrifuncInit()
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
      if (i & ~MGA_FLAT_BIT) {
	 points_tab[i] = points_tab[i&MGA_FLAT_BIT];
	 line_tab[i] = line_tab[i&MGA_FLAT_BIT];
      }
   }

   for (i = 0 ; i < 0x20 ; i++) 
      if ((i & (MGA_NODRAW_BIT|MGA_FALLBACK_BIT)) == MGA_NODRAW_BIT ||
	  mgaglx.nullprims) 
      {
	 quad_tab[i] = mga_null_quad; 
	 tri_tab[i] = mga_null_triangle; 
	 line_tab[i] = mga_null_line;
	 points_tab[i] = mga_null_points;
      }

   if (mgaglx.noFallback) {
      for (i = 0 ; i < 0x10 ; i++) {
	 points_tab[i|MGA_FALLBACK_BIT] = points_tab[i];
	 line_tab[i|MGA_FALLBACK_BIT] = line_tab[i];
	 tri_tab[i|MGA_FALLBACK_BIT] = tri_tab[i];
	 quad_tab[i|MGA_FALLBACK_BIT] = quad_tab[i];
      }
   }

}






void mgaDDChooseRenderState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLuint flags = ctx->TriangleCaps;

   ctx->IndirectTriangles &= ~DD_SW_RASTERIZE;

   if (flags) {
      GLuint ind = 0;
      GLuint shared = 0;
      GLuint fallback = MGA_FALLBACK_BIT;

      if (mgaglx.noFallback) fallback = 0;

      if (flags & DD_Z_NEVER)                      shared |= MGA_NODRAW_BIT;
      if (flags & DD_FLATSHADE)                    shared |= MGA_FLAT_BIT;
      if (flags & DD_MULTIDRAW)                    shared |= fallback;
      if (flags & (DD_SELECT|DD_FEEDBACK))         shared |= MGA_FALLBACK_BIT;
      if (flags & DD_STENCIL)                      shared |= MGA_FALLBACK_BIT;

      ind = shared;
      if (flags & DD_POINT_SMOOTH)                 ind |= MGA_ANTIALIAS_BIT;
      if (flags & DD_POINT_ATTEN)                  ind |= fallback;

      mmesa->renderindex = ind;
      mmesa->PointsFunc = points_tab[ind];
      if (ind & MGA_FALLBACK_BIT) 
	 ctx->IndirectTriangles |= DD_POINT_SW_RASTERIZE;

      ind = shared;
      if (flags & DD_LINE_SMOOTH)                   ind |= MGA_ANTIALIAS_BIT;
      if (flags & DD_LINE_STIPPLE)                  ind |= fallback;

      mmesa->renderindex |= ind;
      mmesa->LineFunc = line_tab[ind];
      if (ind & MGA_FALLBACK_BIT) 
	 ctx->IndirectTriangles |= DD_LINE_SW_RASTERIZE;

      ind = shared;
      if (flags & DD_TRI_SMOOTH)                    ind |= MGA_ANTIALIAS_BIT;
      if (flags & DD_TRI_OFFSET)                    ind |= MGA_OFFSET_BIT;
      if (flags & DD_TRI_LIGHT_TWOSIDE)             ind |= MGA_TWOSIDE_BIT;
      if (flags & (DD_TRI_UNFILLED|DD_TRI_STIPPLE)) ind |= fallback;	

      mmesa->renderindex |= ind;
      mmesa->TriangleFunc = tri_tab[ind];
      mmesa->QuadFunc = quad_tab[ind];
      if (ind & MGA_FALLBACK_BIT)
	 ctx->IndirectTriangles |= (DD_TRI_SW_RASTERIZE | DD_QUAD_SW_RASTERIZE);
   } 
   else if (mmesa->renderindex)
   {
      mmesa->renderindex  = 0;
      mmesa->PointsFunc   = points_tab[0];
      mmesa->LineFunc     = line_tab[0];
      mmesa->TriangleFunc = tri_tab[0];
      mmesa->QuadFunc     = quad_tab[0];
   }

   if (0) {
      gl_print_tri_caps("tricaps", ctx->TriangleCaps);
      mgaPrintRenderState("mga: Render state", mmesa->renderindex);
   }
}



