
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
#include "alphabuf.h"
#include "context.h"
#include "depth.h"
#include "feedback.h"
#include "image.h"
#include "macros.h"
#include "mem.h"
#include "pixel.h"
#include "readpix.h"
#include "span.h"
#include "stencil.h"
#include "types.h"
#endif




/*
 * Read a block of color index pixels.
 */
static void read_index_pixels( GLcontext *ctx,
                               GLint x, GLint y,
			       GLsizei width, GLsizei height,
			       GLenum type, GLvoid *pixels,
                               const struct gl_pixelstore_attrib *packing )
{
   GLint i, j, readWidth;

   /* error checking */
   if (ctx->Visual->RGBAflag) {
      gl_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   ASSERT(ctx->Driver.SetReadBuffer);
   (*ctx->Driver.SetReadBuffer)(ctx, ctx->ReadBuffer, ctx->Pixel.DriverReadBuffer);

   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   /* process image row by row */
   for (j=0;j<height;j++,y++) {
      GLuint index[MAX_WIDTH];
      GLvoid *dest;

      (*ctx->Driver.ReadCI32Span)( ctx, readWidth, x, y, index );

      if (ctx->Pixel.IndexShift!=0 || ctx->Pixel.IndexOffset!=0) {
         gl_shift_and_offset_ci( ctx, readWidth, index);
      }

      if (ctx->Pixel.MapColorFlag) {
         gl_map_ci(ctx, readWidth, index);
      }

      dest = gl_pixel_addr_in_image(packing, pixels,
                         width, height, GL_COLOR_INDEX, type, 0, j, 0);

      switch (type) {
	 case GL_UNSIGNED_BYTE:
	    {
               GLubyte *dst = (GLubyte *) dest;
	       for (i=0;i<readWidth;i++) {
		  *dst++ = (GLubyte) index[i];
	       }
	    }
	    break;
	 case GL_BYTE:
	    {
               GLbyte *dst = (GLbyte *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLbyte) index[i];
	       }
	    }
	    break;
	 case GL_UNSIGNED_SHORT:
	    {
               GLushort *dst = (GLushort *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLushort) index[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap2( (GLushort *) dst, readWidth );
	       }
	    }
	    break;
	 case GL_SHORT:
	    {
               GLshort *dst = (GLshort *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLshort) index[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap2( (GLushort *) dst, readWidth );
	       }
	    }
	    break;
	 case GL_UNSIGNED_INT:
	    {
               GLuint *dst = (GLuint *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLuint) index[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap4( (GLuint *) dst, readWidth );
	       }
	    }
	    break;
	 case GL_INT:
	    {
               GLint *dst = (GLint *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLint) index[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap4( (GLuint *) dst, readWidth );
	       }
	    }
	    break;
	 case GL_FLOAT:
	    {
               GLfloat *dst = (GLfloat *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLfloat) index[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap4( (GLuint *) dst, readWidth );
	       }
	    }
	    break;
         default:
            gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
            j = height + 1; /* exit loop */
      }
   }

   (*ctx->Driver.SetReadBuffer)(ctx, ctx->DrawBuffer, ctx->Color.DriverDrawBuffer);
}



static void read_depth_pixels( GLcontext *ctx,
                               GLint x, GLint y,
			       GLsizei width, GLsizei height,
			       GLenum type, GLvoid *pixels,
                               const struct gl_pixelstore_attrib *packing )
{
   GLint i, j, readWidth;
   GLboolean bias_or_scale;

   /* Error checking */
   if (ctx->Visual->DepthBits <= 0) {
      /* No depth buffer */
      gl_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   if (type != GL_BYTE &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_SHORT &&
       type != GL_UNSIGNED_SHORT &&
       type != GL_INT &&
       type != GL_UNSIGNED_INT &&
       type != GL_FLOAT) {
      gl_error( ctx, GL_INVALID_OPERATION, "glReadPixels(depth type)");
      return;
   }

   bias_or_scale = ctx->Pixel.DepthBias!=0.0 || ctx->Pixel.DepthScale!=1.0;

   if (type==GL_UNSIGNED_SHORT && sizeof(GLdepth)==sizeof(GLushort)
       && !bias_or_scale && !packing->SwapBytes) {
      /* Special case: directly read 16-bit unsigned depth values. */
      for (j=0;j<height;j++,y++) {
         GLushort *dst = (GLushort*) gl_pixel_addr_in_image( packing, pixels,
                         width, height, GL_DEPTH_COMPONENT, type, 0, j, 0 );
         (*ctx->Driver.ReadDepthSpan)( ctx, width, x, y, (GLdepth*) dst);
      }
   }
   else if (type==GL_UNSIGNED_INT && sizeof(GLdepth)==sizeof(GLuint)
            && !bias_or_scale && !packing->SwapBytes) {
      /* Special case: directly read 32-bit unsigned depth values. */
      /* Compute shift value to scale depth values up to 32-bit uints. */
      GLuint shift = 0;
      GLuint max = MAX_DEPTH;
      while ((max&0x80000000)==0) {
         max = max << 1;
         shift++;
      }
      for (j=0;j<height;j++,y++) {
         GLuint *dst = (GLuint *) gl_pixel_addr_in_image( packing, pixels,
                         width, height, GL_DEPTH_COMPONENT, type, 0, j, 0 );
         (*ctx->Driver.ReadDepthSpan)( ctx, width, x, y, (GLdepth*) dst);
         for (i=0;i<width;i++) {
            dst[i] = dst[i] << shift;
         }
      }
   }
   else {
      /* General case (slow) */
      for (j=0;j<height;j++,y++) {
         GLfloat depth[MAX_WIDTH];
         GLvoid *dest;

         _mesa_read_depth_span_float(ctx, readWidth, x, y, depth);

         if (bias_or_scale) {
            for (i=0;i<readWidth;i++) {
               GLfloat d;
               d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
               depth[i] = CLAMP( d, 0.0F, 1.0F );
            }
         }

         dest = gl_pixel_addr_in_image(packing, pixels,
                         width, height, GL_DEPTH_COMPONENT, type, 0, j, 0);

         switch (type) {
            case GL_UNSIGNED_BYTE:
               {
                  GLubyte *dst = (GLubyte *) dest;
                  for (i=0;i<readWidth;i++) {
                     dst[i] = FLOAT_TO_UBYTE( depth[i] );
                  }
               }
               break;
            case GL_BYTE:
               {
                  GLbyte *dst = (GLbyte *) dest;
                  for (i=0;i<readWidth;i++) {
                     dst[i] = FLOAT_TO_BYTE( depth[i] );
                  }
               }
               break;
            case GL_UNSIGNED_SHORT:
               {
                  GLushort *dst = (GLushort *) dest;
                  for (i=0;i<readWidth;i++) {
                     dst[i] = FLOAT_TO_USHORT( depth[i] );
                  }
                  if (packing->SwapBytes) {
                     gl_swap2( (GLushort *) dst, readWidth );
                  }
               }
               break;
            case GL_SHORT:
               {
                  GLshort *dst = (GLshort *) dest;
                  for (i=0;i<readWidth;i++) {
                     dst[i] = FLOAT_TO_SHORT( depth[i] );
                  }
                  if (packing->SwapBytes) {
                     gl_swap2( (GLushort *) dst, readWidth );
                  }
               }
               break;
            case GL_UNSIGNED_INT:
               {
                  GLuint *dst = (GLuint *) dest;
                  for (i=0;i<readWidth;i++) {
                     dst[i] = FLOAT_TO_UINT( depth[i] );
                  }
                  if (packing->SwapBytes) {
                     gl_swap4( (GLuint *) dst, readWidth );
                  }
               }
               break;
            case GL_INT:
               {
                  GLint *dst = (GLint *) dest;
                  for (i=0;i<readWidth;i++) {
                     dst[i] = FLOAT_TO_INT( depth[i] );
                  }
                  if (packing->SwapBytes) {
                     gl_swap4( (GLuint *) dst, readWidth );
                  }
               }
               break;
            case GL_FLOAT:
               {
                  GLfloat *dst = (GLfloat *) dest;
                  for (i=0;i<readWidth;i++) {
                     dst[i] = depth[i];
                  }
                  if (packing->SwapBytes) {
                     gl_swap4( (GLuint *) dst, readWidth );
                  }
               }
               break;
            default:
               gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
         }
      }
   }
}




static void read_stencil_pixels( GLcontext *ctx,
                                 GLint x, GLint y,
				 GLsizei width, GLsizei height,
				 GLenum type, GLvoid *pixels,
                                 const struct gl_pixelstore_attrib *packing )
{
   GLboolean shift_or_offset;
   GLint i, j, readWidth;

   if (type != GL_BYTE &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_SHORT &&
       type != GL_UNSIGNED_SHORT &&
       type != GL_INT &&
       type != GL_UNSIGNED_INT &&
       type != GL_FLOAT &&
       type != GL_BITMAP) {
      gl_error( ctx, GL_INVALID_OPERATION, "glReadPixels(stencil type)");
      return;
   }

   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   if (ctx->Visual->StencilBits<=0) {
      /* No stencil buffer */
      gl_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   shift_or_offset = ctx->Pixel.IndexShift!=0 || ctx->Pixel.IndexOffset!=0;

   /* process image row by row */
   for (j=0;j<height;j++,y++) {
      GLvoid *dest;
      GLstencil stencil[MAX_WIDTH];

      gl_read_stencil_span( ctx, readWidth, x, y, stencil );

      if (shift_or_offset) {
         gl_shift_and_offset_stencil( ctx, readWidth, stencil );
      }

      if (ctx->Pixel.MapStencilFlag) {
         gl_map_stencil( ctx, readWidth, stencil );
      }

      dest = gl_pixel_addr_in_image( packing, pixels,
                          width, height, GL_STENCIL_INDEX, type, 0, j, 0 );

      switch (type) {
	 case GL_UNSIGNED_BYTE:
            if (sizeof(GLstencil) == 8) {
               MEMCPY( dest, stencil, readWidth );
            }
            else {
               GLubyte *dst = (GLubyte *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLubyte) stencil[i];
	       }
            }
	    break;
	 case GL_BYTE:
            if (sizeof(GLstencil) == 8) {
               MEMCPY( dest, stencil, readWidth );
            }
            else {
               GLbyte *dst = (GLbyte *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLbyte) stencil[i];
	       }
            }
	    break;
	 case GL_UNSIGNED_SHORT:
	    {
               GLushort *dst = (GLushort *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLushort) stencil[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap2( (GLushort *) dst, readWidth );
	       }
	    }
	    break;
	 case GL_SHORT:
	    {
               GLshort *dst = (GLshort *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLshort) stencil[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap2( (GLushort *) dst, readWidth );
	       }
	    }
	    break;
	 case GL_UNSIGNED_INT:
	    {
               GLuint *dst = (GLuint *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLuint) stencil[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap4( (GLuint *) dst, readWidth );
	       }
	    }
	    break;
	 case GL_INT:
	    {
               GLint *dst = (GLint *) dest;
	       for (i=0;i<readWidth;i++) {
		  *dst++ = (GLint) stencil[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap4( (GLuint *) dst, readWidth );
	       }
	    }
	    break;
	 case GL_FLOAT:
	    {
               GLfloat *dst = (GLfloat *) dest;
	       for (i=0;i<readWidth;i++) {
		  dst[i] = (GLfloat) stencil[i];
	       }
	       if (packing->SwapBytes) {
		  gl_swap4( (GLuint *) dst, readWidth );
	       }
	    }
	    break;
         case GL_BITMAP:
            if (packing->LsbFirst) {
               GLubyte *dst = (GLubyte*) dest;
               GLint shift = 0;
               for (i = 0; i < readWidth; i++) {
                  if (shift == 0)
                     *dst = 0;
                  *dst |= ((stencil != 0) << shift);
                  shift++;
                  if (shift == 8) {
                     shift = 0;
                     dst++;
                  }
               }
            }
            else {
               GLubyte *dst = (GLubyte*) dest;
               GLint shift = 7;
               for (i = 0; i < readWidth; i++) {
                  if (shift == 7)
                     *dst = 0;
                  *dst |= ((stencil != 0) << shift);
                  shift--;
                  if (shift < 0) {
                     shift = 7;
                     dst++;
                  }
               }
            }
            break;
         default:
            gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
      }
   }
}



/*
 * Optimized glReadPixels for particular pixel formats:
 *   GL_UNSIGNED_BYTE, GL_RGBA
 * when pixel scaling, biasing and mapping are disabled.
 */
static GLboolean
read_fast_rgba_pixels( GLcontext *ctx,
                       GLint x, GLint y,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing )
{
   /* can't do scale, bias or mapping */
   if (ctx->Pixel.ScaleOrBiasRGBA || ctx->Pixel.MapColorFlag)
       return GL_FALSE;

   /* can't do fancy pixel packing */
   if (packing->Alignment != 1 || packing->SwapBytes || packing->LsbFirst)
      return GL_FALSE;

   {
      GLint srcX = x;
      GLint srcY = y;
      GLint readWidth = width;           /* actual width read */
      GLint readHeight = height;         /* actual height read */
      GLint skipPixels = packing->SkipPixels;
      GLint skipRows = packing->SkipRows;
      GLint rowLength;

      if (packing->RowLength > 0)
         rowLength = packing->RowLength;
      else
         rowLength = width;

      /* horizontal clipping */
      if (srcX < ctx->ReadBuffer->Xmin) {
         skipPixels += (ctx->ReadBuffer->Xmin - srcX);
         readWidth  -= (ctx->ReadBuffer->Xmin - srcX);
         srcX = ctx->ReadBuffer->Xmin;
      }
      if (srcX + readWidth > ctx->ReadBuffer->Xmax)
         readWidth -= (srcX + readWidth - ctx->ReadBuffer->Xmax - 1);
      if (readWidth <= 0)
         return GL_TRUE;

      /* vertical clipping */
      if (srcY < ctx->ReadBuffer->Ymin) {
         skipRows   += (ctx->ReadBuffer->Ymin - srcY);
         readHeight -= (ctx->ReadBuffer->Ymin - srcY);
         srcY = ctx->ReadBuffer->Ymin;
      }
      if (srcY + readHeight > ctx->ReadBuffer->Ymax)
         readHeight -= (srcY + readHeight - ctx->ReadBuffer->Ymax - 1);
      if (readHeight <= 0)
         return GL_TRUE;

      /*
       * Ready to read!
       * The window region at (destX, destY) of size (readWidth, readHeight)
       * will be read back.
       * We'll write pixel data to buffer pointed to by "pixels" but we'll
       * skip "skipRows" rows and skip "skipPixels" pixels/row.
       */
      if (format==GL_RGBA && type==GL_UNSIGNED_BYTE) {
         GLubyte *dest = (GLubyte *) pixels
                         + (skipRows * rowLength + skipPixels) * 4;
         GLint row;
         for (row=0; row<readHeight; row++) {
            (*ctx->Driver.ReadRGBASpan)(ctx, readWidth, srcX, srcY,
                                        (GLubyte (*)[4]) dest);
            if (ctx->Visual->SoftwareAlpha) {
               gl_read_alpha_span(ctx, readWidth, srcX, srcY,
                                  (GLubyte (*)[4]) dest);
            }
            dest += rowLength * 4;
            srcY++;
         }
         return GL_TRUE;
      }
      else {
         /* can't do this format/type combination */
         return GL_FALSE;
      }
   }
}



/*
 * Read R, G, B, A, RGB, L, or LA pixels.
 */
static void read_rgba_pixels( GLcontext *ctx,
                              GLint x, GLint y,
                              GLsizei width, GLsizei height,
                              GLenum format, GLenum type, GLvoid *pixels,
                              const struct gl_pixelstore_attrib *packing )
{
   GLint readWidth;

   (*ctx->Driver.SetReadBuffer)(ctx, ctx->ReadBuffer, ctx->Pixel.DriverReadBuffer);

   /* Try optimized path first */
   if (read_fast_rgba_pixels( ctx, x, y, width, height,
                              format, type, pixels, packing )) {

      (*ctx->Driver.SetReadBuffer)(ctx, ctx->DrawBuffer, ctx->Color.DriverDrawBuffer);
      return; /* done! */
   }

   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   /* do error checking on pixel type, format was already checked by caller */
   switch (type) {
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
      case GL_UNSIGNED_INT:
      case GL_INT:
      case GL_FLOAT:
      case GL_UNSIGNED_BYTE_3_3_2:
      case GL_UNSIGNED_BYTE_2_3_3_REV:
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         /* valid pixel type */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
         return;
   }

   if (!gl_is_legal_format_and_type(format, type)) {
      gl_error(ctx, GL_INVALID_OPERATION, "glReadPixels(format or type)");
      return;
   }

   if (ctx->Visual->RGBAflag) {
      GLint j;
      for (j=0;j<height;j++,y++) {
         GLubyte rgba[MAX_WIDTH][4];
         GLvoid *dest;

         gl_read_rgba_span( ctx, ctx->ReadBuffer, readWidth, x, y, rgba );

         dest = gl_pixel_addr_in_image( packing, pixels, width, height,
                                        format, type, 0, j, 0);

         gl_pack_rgba_span( ctx, readWidth, (const GLubyte (*)[4]) rgba,
                            format, type, dest, packing, GL_TRUE );
      }
   }
   else {
      GLint j;
      for (j=0;j<height;j++,y++) {
         GLubyte rgba[MAX_WIDTH][4];
	 GLuint index[MAX_WIDTH];
         GLvoid *dest;

	 (*ctx->Driver.ReadCI32Span)( ctx, readWidth, x, y, index );

	 if (ctx->Pixel.IndexShift!=0 || ctx->Pixel.IndexOffset!=0) {
            gl_map_ci( ctx, readWidth, index );
	 }

         gl_map_ci_to_rgba(ctx, readWidth, index, rgba );

         dest = gl_pixel_addr_in_image( packing, pixels, width, height,
                                        format, type, 0, j, 0);

         gl_pack_rgba_span( ctx, readWidth, (const GLubyte (*)[4]) rgba,
                            format, type, dest, packing, GL_TRUE );
      }
   }

   (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer, ctx->Color.DriverDrawBuffer );
}



void
_mesa_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glReadPixels");

   if (!pixels) {
      gl_error( ctx, GL_INVALID_VALUE, "glReadPixels(pixels)" );
      return;
   }

   switch (format) {
      case GL_COLOR_INDEX:
         read_index_pixels( ctx, x, y, width, height, type, pixels, &ctx->Pack );
	 break;
      case GL_STENCIL_INDEX:
	 read_stencil_pixels( ctx, x, y, width, height, type, pixels, &ctx->Pack );
         break;
      case GL_DEPTH_COMPONENT:
	 read_depth_pixels( ctx, x, y, width, height, type, pixels, &ctx->Pack );
	 break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
      case GL_BGR:
      case GL_BGRA:
      case GL_ABGR_EXT:
         read_rgba_pixels( ctx, x, y, width, height, format, type, pixels, &ctx->Pack );
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(format)" );
   }
}
