
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "context.h"
#include "copypix.h"
#include "depth.h"
#include "feedback.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "pixel.h"
#include "span.h"
#include "state.h"
#include "stencil.h"
#include "types.h"
#include "zoom.h"
#endif



/*
 * Determine if there's overlap in an image copy
 */
static GLboolean
regions_overlap(int srcx, int srcy, int dstx, int dsty, int width, int height,
                float zoomX, float zoomY)
{
   if ((srcx > dstx + (width * zoomX) + 1) || (srcx + width + 1 < dstx)) {
      return GL_FALSE;
   }
   else if ((srcy < dsty) && (srcy + height < dsty + (height * zoomY))) {
      return GL_FALSE;
   }
   else if ((srcy > dsty) && (srcy + height > dsty + (height * zoomY))) {
      return GL_FALSE;
   }
   else {
      return GL_TRUE;
   }
}



static void copy_rgba_pixels( GLcontext *ctx,
                              GLint srcx, GLint srcy,
                              GLint width, GLint height,
                              GLint destx, GLint desty )
{
   GLdepth zspan[MAX_WIDTH];
   GLubyte rgba[MAX_WIDTH][4];
   GLubyte *prgba,*p;
   GLboolean quick_draw;
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean changeBuffer;
   GLubyte *saveReadAlpha;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;

   /* Determine if copy should be done bottom-to-top or top-to-bottom */
   if (srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                 ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);

   if (ctx->Depth.Test || ctx->Fog.Enabled) {
      /* fill in array of z values */
      GLint z = (GLint) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
      for (i=0;i<width;i++) {
         zspan[i] = z;
      }
   }

   if (ctx->RasterMask == 0
       && !zoom
       && destx >= 0
       && destx + width <= ctx->DrawBuffer->Width) {
      quick_draw = GL_TRUE;
   }
   else {
      quick_draw = GL_FALSE;
   }

   /* If read and draw buffer are different we must do buffer switching */
   saveReadAlpha = ctx->ReadBuffer->Alpha;
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer
               || ctx->DrawBuffer != ctx->ReadBuffer;

   if (overlapping) {
      GLint ssy = sy;
      prgba = (GLubyte *) MALLOC(width * height * sizeof(GLubyte) * 4);
      if (!prgba) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = prgba;
      if (changeBuffer) {
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                       ctx->Pixel.DriverReadBuffer );
         if (ctx->Pixel.DriverReadBuffer == GL_FRONT_LEFT)
            ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontLeftAlpha;
         else if (ctx->Pixel.DriverReadBuffer == GL_BACK_LEFT)
            ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackLeftAlpha;
         else if (ctx->Pixel.DriverReadBuffer == GL_FRONT_RIGHT)
            ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontRightAlpha;
         else
            ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackRightAlpha;
      }
      for (j = 0; j < height; j++, ssy += stepy) {
         gl_read_rgba_span( ctx, ctx->ReadBuffer, width, srcx, ssy,
                            (GLubyte (*)[4]) p );
         p += (width * sizeof(GLubyte) * 4);
      }
      p = prgba;
   }
   else {
      prgba = NULL;  /* silence compiler warnings */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      if (overlapping) {
         MEMCPY(rgba, p, width * sizeof(GLubyte) * 4);
         p += (width * sizeof(GLubyte) * 4);
      }
      else {
         if (changeBuffer) {
            (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                          ctx->Pixel.DriverReadBuffer );
            if (ctx->Pixel.DriverReadBuffer == GL_FRONT_LEFT) {
               ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontLeftAlpha;
            }
            else if (ctx->Pixel.DriverReadBuffer == GL_BACK_LEFT) {
               ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackLeftAlpha;
            }
            else if (ctx->Pixel.DriverReadBuffer == GL_FRONT_RIGHT) {
               ctx->ReadBuffer->Alpha = ctx->ReadBuffer->FrontRightAlpha;
            }
            else {
               ctx->ReadBuffer->Alpha = ctx->ReadBuffer->BackRightAlpha;
            }
         }
         gl_read_rgba_span( ctx, ctx->ReadBuffer, width, srcx, sy, rgba );
      }

      if (changeBuffer) {
         /* read from the draw buffer again (in case of blending) */
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                       ctx->Color.DriverDrawBuffer );
         ctx->ReadBuffer->Alpha = saveReadAlpha;
      }

      if (ctx->Pixel.ScaleOrBiasRGBA) {
         gl_scale_and_bias_rgba( ctx, width, rgba );
      }
      if (ctx->Pixel.MapColorFlag) {
         gl_map_rgba( ctx, width, rgba );
      }
      if (quick_draw && dy >= 0 && dy < ctx->DrawBuffer->Height) {
         (*ctx->Driver.WriteRGBASpan)( ctx, width, destx, dy, 
				       (const GLubyte (*)[4])rgba, NULL );
      }
      else if (zoom) {
         gl_write_zoomed_rgba_span( ctx, width, destx, dy, zspan, 
				    (const GLubyte (*)[4])rgba, desty);
      }
      else {
         gl_write_rgba_span( ctx, width, destx, dy, zspan, rgba, GL_BITMAP );
      }
   }

   if (overlapping)
      FREE(prgba);
}


static void copy_ci_pixels( GLcontext *ctx,
                            GLint srcx, GLint srcy, GLint width, GLint height,
                            GLint destx, GLint desty )
{
   GLdepth zspan[MAX_WIDTH];
   GLuint *pci,*p;
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean changeBuffer;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;
   GLint overlapping;

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                 ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);

   if (ctx->Depth.Test || ctx->Fog.Enabled) {
      /* fill in array of z values */
      GLint z = (GLint) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
      for (i=0;i<width;i++) {
         zspan[i] = z;
      }
   }

   /* If read and draw buffer are different we must do buffer switching */
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer
               || ctx->DrawBuffer != ctx->ReadBuffer;

   if (overlapping) {
      GLint ssy = sy;
      pci = (GLuint *) MALLOC(width * height * sizeof(GLuint));
      if (!pci) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = pci;
      if (changeBuffer) {
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                       ctx->Pixel.DriverReadBuffer );
      }
      for (j = 0; j < height; j++, ssy += stepy) {
         gl_read_index_span( ctx, ctx->ReadBuffer, width, srcx, ssy, p );
         p += width;
      }
      p = pci;
   }
   else {
      pci = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLuint indexes[MAX_WIDTH];
      if (overlapping) {
         MEMCPY(indexes, p, width * sizeof(GLuint));
         p += width;
      }
      else {
         if (changeBuffer) {
            (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                          ctx->Pixel.DriverReadBuffer );
         }
         gl_read_index_span( ctx, ctx->ReadBuffer, width, srcx, sy, indexes );
      }

      if (changeBuffer) {
         /* set read buffer back to draw buffer (in case of logicops) */
         (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                       ctx->Color.DriverDrawBuffer );
      }

      if (shift_or_offset) {
         gl_shift_and_offset_ci( ctx, width, indexes );
      }
      if (ctx->Pixel.MapColorFlag) {
         gl_map_ci( ctx, width, indexes );
      }

      if (zoom) {
         gl_write_zoomed_index_span( ctx, width, destx, dy, zspan, indexes, desty );
      }
      else {
         gl_write_index_span(ctx, width, destx, dy, zspan, indexes, GL_BITMAP);
      }
   }

   if (overlapping)
      FREE(pci);
}



/*
 * TODO: Optimize!!!!
 */
static void copy_depth_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                               GLint width, GLint height,
                               GLint destx, GLint desty )
{
   GLfloat depth[MAX_WIDTH];
   GLdepth zspan[MAX_WIDTH];
   GLfloat *p,*pdepth;
   GLuint indexes[MAX_WIDTH];
   GLubyte rgba[MAX_WIDTH][4];
   GLint sy, dy, stepy;
   GLint i, j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;

   if (!ctx->ReadBuffer->Depth || !ctx->DrawBuffer->Depth) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                 ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);

   /* setup colors or indexes */
   if (ctx->Visual->RGBAflag) {
      GLuint *rgba32 = (GLuint *) rgba;
      GLuint color = *(GLuint*)( ctx->Current.ByteColor );
      for (i = 0; i < width; i++) {
         rgba32[i] = color;
      }
   }
   else {
      for (i = 0; i < width; i++) {
         indexes[i] = ctx->Current.Index;
      }
   }

   if (overlapping) {
      GLint ssy = sy;
      pdepth = (GLfloat *) MALLOC(width * height * sizeof(GLfloat));
      if (!pdepth) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = pdepth;
      for (j = 0; j < height; j++, ssy += stepy) {
         _mesa_read_depth_span_float(ctx, width, srcx, ssy, p);
         p += width;
      }
      p = pdepth;
   }
   else {
      pdepth = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      if (overlapping) {
         MEMCPY(depth, p, width * sizeof(GLfloat));
         p += width;
      }
      else {
         _mesa_read_depth_span_float(ctx, width, srcx, sy, depth);
      }

      for (i = 0; i < width; i++) {
         GLfloat d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         zspan[i] = (GLint) (CLAMP( d, 0.0F, 1.0F ) * DEPTH_SCALE);
      }

      if (ctx->Visual->RGBAflag) {
         if (zoom) {
            gl_write_zoomed_rgba_span( ctx, width, destx, dy, zspan, 
				       (const GLubyte (*)[4])rgba, desty );
         }
         else {
            gl_write_rgba_span( ctx, width, destx, dy, zspan, rgba, GL_BITMAP);
         }
      }
      else {
         if (zoom) {
            gl_write_zoomed_index_span( ctx, width, destx, dy,
                                        zspan, indexes, desty );
         }
         else {
            gl_write_index_span( ctx, width, destx, dy,
                                 zspan, indexes, GL_BITMAP );
         }
      }
   }

  if (overlapping)
     FREE(pdepth);
}



static void copy_stencil_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                                 GLint width, GLint height,
                                 GLint destx, GLint desty )
{
   GLint sy, dy, stepy;
   GLint j;
   GLstencil *p, *psten;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;
   GLint overlapping;

   if (!ctx->DrawBuffer->Stencil || !ctx->ReadBuffer->Stencil) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                 ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);

   if (overlapping) {
      GLint ssy = sy;
      psten = (GLstencil *) MALLOC(width * height * sizeof(GLstencil));
      if (!psten) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = psten;
      for (j = 0; j < height; j++, ssy += stepy) {
         gl_read_stencil_span( ctx, width, srcx, ssy, p );
         p += width;
      }
      p = psten;
   }
   else {
      psten = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLstencil stencil[MAX_WIDTH];

      if (overlapping) {
         MEMCPY(stencil, p, width * sizeof(GLstencil));
         p += width;
      }
      else {
         gl_read_stencil_span( ctx, width, srcx, sy, stencil );
      }

      if (shift_or_offset) {
         gl_shift_and_offset_stencil( ctx, width, stencil );
      }
      if (ctx->Pixel.MapStencilFlag) {
         gl_map_stencil( ctx, width, stencil );
      }

      if (zoom) {
         gl_write_zoomed_stencil_span( ctx, width, destx, dy, stencil, desty );
      }
      else {
         gl_write_stencil_span( ctx, width, destx, dy, stencil );
      }
   }

   if (overlapping)
      FREE(psten);
}




void
_mesa_CopyPixels( GLint srcx, GLint srcy, GLsizei width, GLsizei height,
                  GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint destx, desty;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyPixels");

   if (width < 0 || height < 0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyPixels" );
      return;
   }

   if (ctx->NewState) {
      gl_update_state(ctx);
   }

   if (ctx->RenderMode==GL_RENDER) {
      /* Destination of copy: */
      if (!ctx->Current.RasterPosValid) {
	 return;
      }
      destx = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      desty = (GLint) (ctx->Current.RasterPos[1] + 0.5F);

      if (type == GL_COLOR && ctx->Visual->RGBAflag) {
         copy_rgba_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else if (type == GL_COLOR && !ctx->Visual->RGBAflag) {
         copy_ci_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else if (type == GL_DEPTH) {
         copy_depth_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else if (type == GL_STENCIL) {
         copy_stencil_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else {
	 gl_error( ctx, GL_INVALID_ENUM, "glCopyPixels" );
      }
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      GLfloat color[4];
      UBYTE_RGBA_TO_FLOAT_RGBA(color, ctx->Current.ByteColor );
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_COPY_PIXEL_TOKEN );
      gl_feedback_vertex( ctx, ctx->Current.RasterPos,
			  color, ctx->Current.Index,
			  ctx->Current.Texcoord[0] );
   }
   else if (ctx->RenderMode == GL_SELECT) {
      gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }

}
