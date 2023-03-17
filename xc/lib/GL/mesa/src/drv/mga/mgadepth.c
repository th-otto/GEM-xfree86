/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This file has been modified by Wittawat Yamwong for GLX module.
 */

/*
 * Depth buffer functions
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include <string.h>
#include "context.h"
#include "mgadepth.h"
#include "types.h"
#include "mm.h"
#include "mgalib.h"
#endif



/*
 * Return the address of the Z-buffer value for window coordinate (x,y):
 */
#define Z_SETUP								\
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);				\
   __DRIscreenPrivate *sPriv = mmesa->driScreen;			\
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;			\
   GLdepth *zbstart = (GLdepth *)(sPriv->pFB + mgaScreen->depthOffset);\
   GLint zbpitch = mgaScreen->depthPitch

#define Z_ADDRESS( X, Y )  \
  (zbstart + zbpitch * (Y) + (X))


/**********************************************************************/
/*****                   Depth Testing Functions                  *****/
/**********************************************************************/


/*
 * Depth test horizontal spans of fragments.  These functions are called
 * via ctx->Driver.depth_test_span only.
 *
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in span in window coords
 *         z - array [n] of integer depth values
 * In/Out:  mask - array [n] of flags (1=draw pixel, 0=don't draw) 
 * Return:  number of pixels which passed depth test
 */


/*
 * glDepthFunc( any ) and glDepthMask( GL_TRUE or GL_FALSE ).
 */
static GLuint mga_depth_test_span_generic( GLcontext* ctx,
					   GLuint n, GLint x, GLint y,
					   const GLdepth z[],
					   GLubyte mask[] )
{
   Z_SETUP;
   GLdepth *zptr = Z_ADDRESS( x, y );
   GLubyte *m = mask;
   GLuint i;
   GLuint passed = 0;

   LOCK_HARDWARE(mmesa);

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] < *zptr) {
		     /* pass */
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     /* fail */
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] < *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] <= *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] <= *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] >= *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] >= *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] > *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] > *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] != *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] != *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] == *zptr) {
		     *zptr = z[i];
		     passed++;
		  }
		  else {
		     *m =0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  if (z[i] == *zptr) {
		     /* pass */
		     passed++;
		  }
		  else {
		     *m =0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0;i<n;i++,zptr++,m++) {
	       if (*m) {
		  *zptr = z[i];
		  passed++;
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	    passed = n;
	 }
	 break;
      case GL_NEVER:
	 for (i=0;i<n;i++) {
	    mask[i] = 0;
	 }
	 break;
      default:
   }

   UNLOCK_HARDWARE(mmesa);

   return passed;
}





/*
 * Depth test an array of randomly positioned fragments.
 */

/*
 * glDepthFunc( any ) and glDepthMask( GL_TRUE or GL_FALSE ).
 */
static void mga_depth_test_pixels_generic( GLcontext* ctx,
					   GLuint n, 
					   const GLint x[], const GLint y[],
					   const GLdepth z[], GLubyte mask[] )
{
   Z_SETUP;
   register GLdepth *zptr;
   register GLuint i;

   LOCK_HARDWARE(mmesa);

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Depth.Func) {
      case GL_LESS:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] < *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_LEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] <= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] >= *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_GREATER:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] > *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_NOTEQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] != *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_EQUAL:
         if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		     *zptr = z[i];
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  if (z[i] == *zptr) {
		     /* pass */
		  }
		  else {
		     /* fail */
		     mask[i] = 0;
		  }
	       }
	    }
	 }
	 break;
      case GL_ALWAYS:
	 if (ctx->Depth.Mask) {
	    /* Update Z buffer */
	    for (i=0; i<n; i++) {
	       if (mask[i]) {
		  zptr = Z_ADDRESS(x[i],y[i]);
		  *zptr = z[i];
	       }
	    }
	 }
	 else {
	    /* Don't update Z buffer or mask */
	 }
	 break;
      case GL_NEVER:
	 /* depth test never passes */
	 for (i=0;i<n;i++) {
	    mask[i] = 0;
	 }
	 break;
      default:
    } 

   UNLOCK_HARDWARE(mmesa);
}



/**********************************************************************/
/*****                      Read Depth Buffer                     *****/
/**********************************************************************/


/*
 * Return a span of depth values from the depth buffer as floats in [0,1].
 * This function is only called through Driver.read_depth_span_float()
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
static void mga_read_depth_span_float( GLcontext* ctx,
				       GLuint n, GLint x, GLint y, 
				       GLfloat depth[] )
{
   Z_SETUP;
   GLdepth *zptr;
   GLfloat scale;
   GLuint i;

   LOCK_HARDWARE(mmesa);

   scale = 1.0F / DEPTH_SCALE;

   if (ctx->Buffer->Depth) {
      zptr = Z_ADDRESS( x, y );
      for (i=0;i<n;i++) {
	 depth[i] = (GLfloat) zptr[i] * scale;
      }
   }
   else {
      for (i=0;i<n;i++) {
	 depth[i] = 0.0F;
      }
   }
   
   UNLOCK_HARDWARE(mmesa);
}


/*
 * Return a span of depth values from the depth buffer as integers in
 * [0,MAX_DEPTH].
 * This function is only called through Driver.read_depth_span_int()
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  depth - the array of depth values
 */
static void mga_read_depth_span_int( GLcontext* ctx,
				     GLuint n, GLint x, GLint y, 
				     GLdepth depth[] )
{
   Z_SETUP;
   
   LOCK_HARDWARE(mmesa);
      
   if (ctx->Buffer->Depth) {
      GLdepth *zptr = Z_ADDRESS( x, y );
      MEMCPY( depth, zptr, n * sizeof(GLdepth) );
   }
   else {
      GLuint i;
      for (i=0;i<n;i++) {
	 depth[i] = 0;
      }
   }
   
   UNLOCK_HARDWARE(mmesa);
}


void mgaDDInitDepthFuncs( GLcontext *ctx )
{
   ctx->Driver.ReadDepthSpanFloat = mga_read_depth_span_float;
   ctx->Driver.ReadDepthSpanInt = mga_read_depth_span_int;
   ctx->Driver.DepthTestSpan = mga_depth_test_span_generic;
   ctx->Driver.DepthTestPixels = mga_depth_test_pixels_generic;
}

