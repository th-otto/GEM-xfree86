
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
#include "image.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "pixel.h"
#include "types.h"
#endif



/*
 * These are the image packing parameters for Mesa's internal images.
 * That is, _mesa_unpack_image() returns image data in this format.
 * When we execute image commands (glDrawPixels, glTexImage, etc)
 * from within display lists we have to be sure to set the current
 * unpacking params to these values!
 */
struct gl_pixelstore_attrib _mesa_native_packing = {
   1,            /* Alignment */
   0,            /* RowLength */
   0,            /* SkipPixels */
   0,            /* SkipRows */
   0,            /* ImageHeight */
   0,            /* SkipImages */
   GL_FALSE,     /* SwapBytes */
   GL_FALSE      /* LsbFirst */
};



/*
 * Flip the 8 bits in each byte of the given array.
 */
void gl_flip_bytes( GLubyte *p, GLuint n )
{
   register GLuint i, a, b;

   for (i=0;i<n;i++) {
      b = (GLuint) p[i];
      a = ((b & 0x01) << 7) |
	  ((b & 0x02) << 5) |
	  ((b & 0x04) << 3) |
	  ((b & 0x08) << 1) |
	  ((b & 0x10) >> 1) |
	  ((b & 0x20) >> 3) |
	  ((b & 0x40) >> 5) |
	  ((b & 0x80) >> 7);
      p[i] = (GLubyte) a;
   }
}


/*
 * Flip the order of the 2 bytes in each word in the given array.
 */
void gl_swap2( GLushort *p, GLuint n )
{
   register GLuint i;

   for (i=0;i<n;i++) {
      p[i] = (p[i] >> 8) | ((p[i] << 8) & 0xff00);
   }
}



/*
 * Flip the order of the 4 bytes in each word in the given array.
 */
void gl_swap4( GLuint *p, GLuint n )
{
   register GLuint i, a, b;

   for (i=0;i<n;i++) {
      b = p[i];
      a =  (b >> 24)
	| ((b >> 8) & 0xff00)
	| ((b << 8) & 0xff0000)
	| ((b << 24) & 0xff000000);
      p[i] = a;
   }
}




/*
 * Return the size, in bytes, of the given GL datatype.
 * Return 0 if GL_BITMAP.
 * Return -1 if invalid type enum.
 */
GLint gl_sizeof_type( GLenum type )
{
   switch (type) {
      case GL_BITMAP:
	 return 0;
      case GL_UNSIGNED_BYTE:
         return sizeof(GLubyte);
      case GL_BYTE:
	 return sizeof(GLbyte);
      case GL_UNSIGNED_SHORT:
	 return sizeof(GLushort);
      case GL_SHORT:
	 return sizeof(GLshort);
      case GL_UNSIGNED_INT:
	 return sizeof(GLuint);
      case GL_INT:
	 return sizeof(GLint);
      case GL_FLOAT:
	 return sizeof(GLfloat);
      default:
         return -1;
   }
}


/*
 * Same as gl_sizeof_packed_type() but we also accept the
 * packed pixel format datatypes.
 */
GLint gl_sizeof_packed_type( GLenum type )
{
   switch (type) {
      case GL_BITMAP:
	 return 0;
      case GL_UNSIGNED_BYTE:
         return sizeof(GLubyte);
      case GL_BYTE:
	 return sizeof(GLbyte);
      case GL_UNSIGNED_SHORT:
	 return sizeof(GLushort);
      case GL_SHORT:
	 return sizeof(GLshort);
      case GL_UNSIGNED_INT:
	 return sizeof(GLuint);
      case GL_INT:
	 return sizeof(GLint);
      case GL_FLOAT:
	 return sizeof(GLfloat);
      case GL_UNSIGNED_BYTE_3_3_2:
         return sizeof(GLubyte);
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         return sizeof(GLubyte);
      case GL_UNSIGNED_SHORT_5_6_5:
         return sizeof(GLshort);
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         return sizeof(GLshort);
      case GL_UNSIGNED_SHORT_4_4_4_4:
         return sizeof(GLshort);
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
         return sizeof(GLshort);
      case GL_UNSIGNED_SHORT_5_5_5_1:
         return sizeof(GLshort);
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         return sizeof(GLshort);
      case GL_UNSIGNED_INT_8_8_8_8:
         return sizeof(GLuint);
      case GL_UNSIGNED_INT_8_8_8_8_REV:
         return sizeof(GLuint);
      case GL_UNSIGNED_INT_10_10_10_2:
         return sizeof(GLuint);
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         return sizeof(GLuint);
      default:
         return -1;
   }
}



/*
 * Return the number of components in a GL enum pixel type.
 * Return -1 if bad format.
 */
GLint gl_components_in_format( GLenum format )
{
   switch (format) {
      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
      case GL_STENCIL_INDEX:
      case GL_DEPTH_COMPONENT:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_LUMINANCE:
         return 1;
      case GL_LUMINANCE_ALPHA:
	 return 2;
      case GL_RGB:
	 return 3;
      case GL_RGBA:
	 return 4;
      case GL_BGR:
	 return 3;
      case GL_BGRA:
	 return 4;
      case GL_ABGR_EXT:
         return 4;
      default:
         return -1;
   }
}


/*
 * Return bytes per pixel for given format and type
 * Return -1 if bad format or type.
 */
GLint gl_bytes_per_pixel( GLenum format, GLenum type )
{
   GLint comps = gl_components_in_format( format );
   if (comps < 0)
      return -1;

   switch (type) {
      case GL_BITMAP:
         return 0;  /* special case */
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
         return comps * sizeof(GLubyte);
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
         return comps * sizeof(GLshort);
      case GL_INT:
      case GL_UNSIGNED_INT:
         return comps * sizeof(GLint);
      case GL_FLOAT:
         return comps * sizeof(GLfloat);
      case GL_UNSIGNED_BYTE_3_3_2:
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         if (format == GL_RGB || format == GL_BGR)
            return sizeof(GLubyte);
         else
            return -1;  /* error */
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         if (format == GL_RGB || format == GL_BGR)
            return sizeof(GLshort);
         else
            return -1;  /* error */
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         if (format == GL_RGBA || format == GL_BGRA || format == GL_ABGR_EXT)
            return sizeof(GLushort);
         else
            return -1;
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         if (format == GL_RGBA || format == GL_BGRA || format == GL_ABGR_EXT)
            return sizeof(GLuint);
         else
            return -1;
      default:
         return -1;
   }
}


/*
 * Test if the given pixel format and type are legal.
 * Return GL_TRUE for legal, GL_FALSE for illegal.
 */
GLboolean gl_is_legal_format_and_type( GLenum format, GLenum type )
{
   switch (format) {
      case GL_COLOR_INDEX:
      case GL_STENCIL_INDEX:
         switch (type) {
            case GL_BITMAP:
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
               return GL_TRUE;
            default:
               return GL_FALSE;
         }
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_DEPTH_COMPONENT:
      case GL_BGR:
         switch (type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
               return GL_TRUE;
            default:
               return GL_FALSE;
         }
      case GL_RGB:
         switch (type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
            case GL_UNSIGNED_BYTE_3_3_2:
            case GL_UNSIGNED_BYTE_2_3_3_REV:
            case GL_UNSIGNED_SHORT_5_6_5:
            case GL_UNSIGNED_SHORT_5_6_5_REV:
               return GL_TRUE;
            default:
               return GL_FALSE;
         }
      case GL_RGBA:
      case GL_BGRA:
      case GL_ABGR_EXT:
         switch (type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
            case GL_UNSIGNED_SHORT_4_4_4_4:
            case GL_UNSIGNED_SHORT_4_4_4_4_REV:
            case GL_UNSIGNED_SHORT_5_5_5_1:
            case GL_UNSIGNED_SHORT_1_5_5_5_REV:
            case GL_UNSIGNED_INT_8_8_8_8:
            case GL_UNSIGNED_INT_8_8_8_8_REV:
            case GL_UNSIGNED_INT_10_10_10_2:
            case GL_UNSIGNED_INT_2_10_10_10_REV:
               return GL_TRUE;
            default:
               return GL_FALSE;
         }
      default:
         ; /* fall-through */
   }
   return GL_FALSE;
}



/*
 * Return the address of a pixel in an image (actually a volume).
 * Pixel unpacking/packing parameters are observed according to 'packing'.
 * Input:  image - start of image data
 *         width, height - size of image
 *         format - image format
 *         type - pixel component type
 *         packing - the pixelstore attributes
 *         img - which image in the volume (0 for 1D or 2D images)
 *         row, column - location of pixel in the image
 * Return:  address of pixel at (image,row,column) in image or NULL if error.
 */
GLvoid *gl_pixel_addr_in_image( const struct gl_pixelstore_attrib *packing,
                                const GLvoid *image, GLsizei width,
                                GLsizei height, GLenum format, GLenum type,
                                GLint img, GLint row, GLint column )
{
   GLint alignment;        /* 1, 2 or 4 */
   GLint pixels_per_row;
   GLint rows_per_image;
   GLint skiprows;
   GLint skippixels;
   GLint skipimages;       /* for 3-D volume images */
   GLubyte *pixel_addr;

   alignment = packing->Alignment;
   if (packing->RowLength > 0) {
      pixels_per_row = packing->RowLength;
   }
   else {
      pixels_per_row = width;
   }
   if (packing->ImageHeight > 0) {
      rows_per_image = packing->ImageHeight;
   }
   else {
      rows_per_image = height;
   }
   skiprows = packing->SkipRows;
   skippixels = packing->SkipPixels;
   skipimages = packing->SkipImages;

   if (type==GL_BITMAP) {
      /* BITMAP data */
      GLint comp_per_pixel;   /* components per pixel */
      GLint bytes_per_comp;   /* bytes per component */
      GLint bytes_per_row;
      GLint bytes_per_image;

      /* Compute bytes per component */
      bytes_per_comp = gl_sizeof_packed_type( type );
      if (bytes_per_comp<0) {
         return NULL;
      }

      /* Compute number of components per pixel */
      comp_per_pixel = gl_components_in_format( format );
      if (comp_per_pixel<0 && type != GL_BITMAP) {
         return NULL;
      }

      bytes_per_row = alignment
                    * CEILING( comp_per_pixel*pixels_per_row, 8*alignment );

      bytes_per_image = bytes_per_row * rows_per_image;

      pixel_addr = (GLubyte *) image
                 + (skipimages + img) * bytes_per_image
                 + (skiprows + row) * bytes_per_row
                 + (skippixels + column) / 8;
   }
   else {
      /* Non-BITMAP data */
      GLint bytes_per_pixel, bytes_per_row, remainder, bytes_per_image;

      bytes_per_pixel = gl_bytes_per_pixel( format, type );

      /* The pixel type and format should have been error checked earlier */
      assert(bytes_per_pixel > 0);

      bytes_per_row = pixels_per_row * bytes_per_pixel;
      remainder = bytes_per_row % alignment;
      if (remainder > 0)
         bytes_per_row += (alignment - remainder);

      ASSERT(bytes_per_row % alignment == 0);

      bytes_per_image = bytes_per_row * rows_per_image;

      /* compute final pixel address */
      pixel_addr = (GLubyte *) image
                 + (skipimages + img) * bytes_per_image
                 + (skiprows + row) * bytes_per_row
                 + (skippixels + column) * bytes_per_pixel;
   }

   return (GLvoid *) pixel_addr;
}



/*
 * Unpack a 32x32 pixel polygon stipple from user memory using the
 * current pixel unpack settings.
 */
void gl_unpack_polygon_stipple( const GLcontext *ctx,
                                const GLubyte *pattern, GLuint dest[32] )
{
   GLint i;
   for (i = 0; i < 32; i++) {
      GLubyte *src = (GLubyte *) gl_pixel_addr_in_image( &ctx->Unpack, pattern,
                                  32, 32, GL_COLOR_INDEX, GL_BITMAP, 0, i, 0 );
      dest[i] = (src[0] << 24)
              | (src[1] << 16)
              | (src[2] <<  8)
              | (src[3]      );
   }

   /* Bit flipping within each byte */
   if (ctx->Unpack.LsbFirst) {
      gl_flip_bytes( (GLubyte *) dest, 32 * 4 );
   }
}



/*
 * Pack polygon stipple into user memory given current pixel packing
 * settings.
 */
void gl_pack_polygon_stipple( const GLcontext *ctx,
                              const GLuint pattern[32],
                              GLubyte *dest )
{
   GLint i;
   for (i = 0; i < 32; i++) {
      GLubyte *dst = (GLubyte *) gl_pixel_addr_in_image( &ctx->Pack, dest,
                                  32, 32, GL_COLOR_INDEX, GL_BITMAP, 0, i, 0 );
      dst[0] = (pattern[i] >> 24) & 0xff;
      dst[1] = (pattern[i] >> 16) & 0xff;
      dst[2] = (pattern[i] >>  8) & 0xff;
      dst[3] = (pattern[i]      ) & 0xff;

      /* Bit flipping within each byte */
      if (ctx->Pack.LsbFirst) {
         gl_flip_bytes( (GLubyte *) dst, 4 );
      }
   }
}



/*
 * Pack the given RGBA span into client memory at 'dest' address
 * in the given pixel format and type.
 * Optionally apply the enabled pixel transfer ops.
 * Pack into memory using the given packing params struct.
 * This is used by glReadPixels and glGetTexImage?D()
 * Input:  ctx - the context
 *         n - number of pixels in the span
 *         rgba - the pixels
 *         format - dest packing format
 *         type - dest packing datatype
 *         destination - destination packing address
 *         packing - pixel packing parameters
 *         applyTransferOps - apply scale/bias/lookup-table ops?
 */
void gl_pack_rgba_span( const GLcontext *ctx,
                        GLuint n, CONST GLubyte rgba[][4],
                        GLenum format, GLenum type, GLvoid *destination,
                        const struct gl_pixelstore_attrib *packing,
                        GLboolean applyTransferOps )
{
   applyTransferOps &= (ctx->Pixel.ScaleOrBiasRGBA || ctx->Pixel.MapColorFlag);

   /* Test for optimized case first */
   if (!applyTransferOps && format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
      /* common simple case */
      MEMCPY( destination, rgba, n * 4 * sizeof(GLubyte) );
   }
   else if (!applyTransferOps && format == GL_RGB && type == GL_UNSIGNED_BYTE) {
      /* common simple case */
      GLint i;
      GLubyte *dest = (GLubyte *) destination;
      for (i = 0; i < n; i++) {
         dest[0] = rgba[i][RCOMP];
         dest[1] = rgba[i][GCOMP];
         dest[2] = rgba[i][BCOMP];
         dest += 3;
      }
   }
   else {
      /* general solution */
      GLfloat red[MAX_WIDTH], green[MAX_WIDTH], blue[MAX_WIDTH];
      GLfloat alpha[MAX_WIDTH], luminance[MAX_WIDTH];
      const GLfloat rscale = 1.0F / 255.0F;
      const GLfloat gscale = 1.0F / 255.0F;
      const GLfloat bscale = 1.0F / 255.0F;
      const GLfloat ascale = 1.0F / 255.0F;
      const GLint comps = gl_components_in_format(format);
      GLuint i;

      assert(n <= MAX_WIDTH);

      /* convert color components to floating point */
      for (i=0;i<n;i++) {
         red[i]   = rgba[i][RCOMP] * rscale;
         green[i] = rgba[i][GCOMP] * gscale;
         blue[i]  = rgba[i][BCOMP] * bscale;
         alpha[i] = rgba[i][ACOMP] * ascale;
      }

      /*
       * Apply scale, bias and lookup-tables if enabled.
       */
      if (applyTransferOps) {
         if (ctx->Pixel.ScaleOrBiasRGBA) {
            gl_scale_and_bias_color( ctx, n, red, green, blue, alpha );
         }
         if (ctx->Pixel.MapColorFlag) {
            gl_map_color( ctx, n, red, green, blue, alpha );
         }
      }

      if (format==GL_LUMINANCE || format==GL_LUMINANCE_ALPHA) {
         for (i=0;i<n;i++) {
            GLfloat sum = red[i] + green[i] + blue[i];
            luminance[i] = CLAMP( sum, 0.0F, 1.0F );
         }
      }

      /*
       * Pack/store the pixels.  Ugh!  Lots of cases!!!
       */
      switch (type) {
         case GL_UNSIGNED_BYTE:
            {
               GLubyte *dst = (GLubyte *) destination;
               switch (format) {
                  case GL_RED:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UBYTE(red[i]);
                     break;
                  case GL_GREEN:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UBYTE(green[i]);
                     break;
                  case GL_BLUE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UBYTE(blue[i]);
                     break;
                  case GL_ALPHA:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UBYTE(alpha[i]);
                     break;
                  case GL_LUMINANCE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UBYTE(luminance[i]);
                     break;
                  case GL_LUMINANCE_ALPHA:
                     for (i=0;i<n;i++) {
                        dst[i*2+0] = FLOAT_TO_UBYTE(luminance[i]);
                        dst[i*2+1] = FLOAT_TO_UBYTE(alpha[i]);
                     }
                     break;
                  case GL_RGB:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_UBYTE(red[i]);
                        dst[i*3+1] = FLOAT_TO_UBYTE(green[i]);
                        dst[i*3+2] = FLOAT_TO_UBYTE(blue[i]);
                     }
                     break;
                  case GL_RGBA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_UBYTE(red[i]);
                        dst[i*4+1] = FLOAT_TO_UBYTE(green[i]);
                        dst[i*4+2] = FLOAT_TO_UBYTE(blue[i]);
                        dst[i*4+3] = FLOAT_TO_UBYTE(alpha[i]);
                     }
                     break;
                  case GL_BGR:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_UBYTE(blue[i]);
                        dst[i*3+1] = FLOAT_TO_UBYTE(green[i]);
                        dst[i*3+2] = FLOAT_TO_UBYTE(red[i]);
                     }
                     break;
                  case GL_BGRA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_UBYTE(blue[i]);
                        dst[i*4+1] = FLOAT_TO_UBYTE(green[i]);
                        dst[i*4+2] = FLOAT_TO_UBYTE(red[i]);
                        dst[i*4+3] = FLOAT_TO_UBYTE(alpha[i]);
                     }
                     break;
                  case GL_ABGR_EXT:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_UBYTE(alpha[i]);
                        dst[i*4+1] = FLOAT_TO_UBYTE(blue[i]);
                        dst[i*4+2] = FLOAT_TO_UBYTE(green[i]);
                        dst[i*4+3] = FLOAT_TO_UBYTE(red[i]);
                     }
                     break;
                  default:
                     gl_problem(ctx, "bad format in gl_pack_rgba_span\n");
               }
	    }
	    break;
	 case GL_BYTE:
            {
               GLbyte *dst = (GLbyte *) destination;
               switch (format) {
                  case GL_RED:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_BYTE(red[i]);
                     break;
                  case GL_GREEN:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_BYTE(green[i]);
                     break;
                  case GL_BLUE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_BYTE(blue[i]);
                     break;
                  case GL_ALPHA:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_BYTE(alpha[i]);
                     break;
                  case GL_LUMINANCE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_BYTE(luminance[i]);
                     break;
                  case GL_LUMINANCE_ALPHA:
                     for (i=0;i<n;i++) {
                        dst[i*2+0] = FLOAT_TO_BYTE(luminance[i]);
                        dst[i*2+1] = FLOAT_TO_BYTE(alpha[i]);
                     }
                     break;
                  case GL_RGB:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_BYTE(red[i]);
                        dst[i*3+1] = FLOAT_TO_BYTE(green[i]);
                        dst[i*3+2] = FLOAT_TO_BYTE(blue[i]);
                     }
                     break;
                  case GL_RGBA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_BYTE(red[i]);
                        dst[i*4+1] = FLOAT_TO_BYTE(green[i]);
                        dst[i*4+2] = FLOAT_TO_BYTE(blue[i]);
                        dst[i*4+3] = FLOAT_TO_BYTE(alpha[i]);
                     }
                     break;
                  case GL_BGR:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_BYTE(blue[i]);
                        dst[i*3+1] = FLOAT_TO_BYTE(green[i]);
                        dst[i*3+2] = FLOAT_TO_BYTE(red[i]);
                     }
                     break;
                  case GL_BGRA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_BYTE(blue[i]);
                        dst[i*4+1] = FLOAT_TO_BYTE(green[i]);
                        dst[i*4+2] = FLOAT_TO_BYTE(red[i]);
                        dst[i*4+3] = FLOAT_TO_BYTE(alpha[i]);
                     }
                  case GL_ABGR_EXT:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_BYTE(alpha[i]);
                        dst[i*4+1] = FLOAT_TO_BYTE(blue[i]);
                        dst[i*4+2] = FLOAT_TO_BYTE(green[i]);
                        dst[i*4+3] = FLOAT_TO_BYTE(red[i]);
                     }
                     break;
                  default:
                     gl_problem(ctx, "bad format in gl_pack_rgba_span\n");
               }
            }
	    break;
	 case GL_UNSIGNED_SHORT:
            {
               GLushort *dst = (GLushort *) destination;
               switch (format) {
                  case GL_RED:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_USHORT(red[i]);
                     break;
                  case GL_GREEN:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_USHORT(green[i]);
                     break;
                  case GL_BLUE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_USHORT(blue[i]);
                     break;
                  case GL_ALPHA:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_USHORT(alpha[i]);
                     break;
                  case GL_LUMINANCE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_USHORT(luminance[i]);
                     break;
                  case GL_LUMINANCE_ALPHA:
                     for (i=0;i<n;i++) {
                        dst[i*2+0] = FLOAT_TO_USHORT(luminance[i]);
                        dst[i*2+1] = FLOAT_TO_USHORT(alpha[i]);
                     }
                     break;
                  case GL_RGB:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_USHORT(red[i]);
                        dst[i*3+1] = FLOAT_TO_USHORT(green[i]);
                        dst[i*3+2] = FLOAT_TO_USHORT(blue[i]);
                     }
                     break;
                  case GL_RGBA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_USHORT(red[i]);
                        dst[i*4+1] = FLOAT_TO_USHORT(green[i]);
                        dst[i*4+2] = FLOAT_TO_USHORT(blue[i]);
                        dst[i*4+3] = FLOAT_TO_USHORT(alpha[i]);
                     }
                     break;
                  case GL_BGR:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_USHORT(blue[i]);
                        dst[i*3+1] = FLOAT_TO_USHORT(green[i]);
                        dst[i*3+2] = FLOAT_TO_USHORT(red[i]);
                     }
                     break;
                  case GL_BGRA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_USHORT(blue[i]);
                        dst[i*4+1] = FLOAT_TO_USHORT(green[i]);
                        dst[i*4+2] = FLOAT_TO_USHORT(red[i]);
                        dst[i*4+3] = FLOAT_TO_USHORT(alpha[i]);
                     }
                     break;
                  case GL_ABGR_EXT:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_USHORT(alpha[i]);
                        dst[i*4+1] = FLOAT_TO_USHORT(blue[i]);
                        dst[i*4+2] = FLOAT_TO_USHORT(green[i]);
                        dst[i*4+3] = FLOAT_TO_USHORT(red[i]);
                     }
                     break;
                  default:
                     gl_problem(ctx, "bad format in gl_pack_rgba_span\n");
               }
               if (packing->SwapBytes) {
                  gl_swap2( (GLushort *) dst, n * comps);
               }
            }
	    break;
	 case GL_SHORT:
            {
               GLshort *dst = (GLshort *) destination;
               switch (format) {
                  case GL_RED:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_SHORT(red[i]);
                     break;
                  case GL_GREEN:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_SHORT(green[i]);
                     break;
                  case GL_BLUE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_SHORT(blue[i]);
                     break;
                  case GL_ALPHA:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_SHORT(alpha[i]);
                     break;
                  case GL_LUMINANCE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_SHORT(luminance[i]);
                     break;
                  case GL_LUMINANCE_ALPHA:
                     for (i=0;i<n;i++) {
                        dst[i*2+0] = FLOAT_TO_SHORT(luminance[i]);
                        dst[i*2+1] = FLOAT_TO_SHORT(alpha[i]);
                     }
                     break;
                  case GL_RGB:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_SHORT(red[i]);
                        dst[i*3+1] = FLOAT_TO_SHORT(green[i]);
                        dst[i*3+2] = FLOAT_TO_SHORT(blue[i]);
                     }
                     break;
                  case GL_RGBA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_SHORT(red[i]);
                        dst[i*4+1] = FLOAT_TO_SHORT(green[i]);
                        dst[i*4+2] = FLOAT_TO_SHORT(blue[i]);
                        dst[i*4+3] = FLOAT_TO_SHORT(alpha[i]);
                     }
                     break;
                  case GL_BGR:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_SHORT(blue[i]);
                        dst[i*3+1] = FLOAT_TO_SHORT(green[i]);
                        dst[i*3+2] = FLOAT_TO_SHORT(red[i]);
                     }
                     break;
                  case GL_BGRA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_SHORT(blue[i]);
                        dst[i*4+1] = FLOAT_TO_SHORT(green[i]);
                        dst[i*4+2] = FLOAT_TO_SHORT(red[i]);
                        dst[i*4+3] = FLOAT_TO_SHORT(alpha[i]);
                     }
                  case GL_ABGR_EXT:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_SHORT(alpha[i]);
                        dst[i*4+1] = FLOAT_TO_SHORT(blue[i]);
                        dst[i*4+2] = FLOAT_TO_SHORT(green[i]);
                        dst[i*4+3] = FLOAT_TO_SHORT(red[i]);
                     }
                     break;
                  default:
                     gl_problem(ctx, "bad format in gl_pack_rgba_span\n");
               }
               if (packing->SwapBytes) {
                  gl_swap2( (GLushort *) dst, n * comps );
               }
            }
	    break;
	 case GL_UNSIGNED_INT:
            {
               GLuint *dst = (GLuint *) destination;
               switch (format) {
                  case GL_RED:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UINT(red[i]);
                     break;
                  case GL_GREEN:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UINT(green[i]);
                     break;
                  case GL_BLUE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UINT(blue[i]);
                     break;
                  case GL_ALPHA:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UINT(alpha[i]);
                     break;
                  case GL_LUMINANCE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_UINT(luminance[i]);
                     break;
                  case GL_LUMINANCE_ALPHA:
                     for (i=0;i<n;i++) {
                        dst[i*2+0] = FLOAT_TO_UINT(luminance[i]);
                        dst[i*2+1] = FLOAT_TO_UINT(alpha[i]);
                     }
                     break;
                  case GL_RGB:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_UINT(red[i]);
                        dst[i*3+1] = FLOAT_TO_UINT(green[i]);
                        dst[i*3+2] = FLOAT_TO_UINT(blue[i]);
                     }
                     break;
                  case GL_RGBA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_UINT(red[i]);
                        dst[i*4+1] = FLOAT_TO_UINT(green[i]);
                        dst[i*4+2] = FLOAT_TO_UINT(blue[i]);
                        dst[i*4+3] = FLOAT_TO_UINT(alpha[i]);
                     }
                     break;
                  case GL_BGR:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_UINT(blue[i]);
                        dst[i*3+1] = FLOAT_TO_UINT(green[i]);
                        dst[i*3+2] = FLOAT_TO_UINT(red[i]);
                     }
                     break;
                  case GL_BGRA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_UINT(blue[i]);
                        dst[i*4+1] = FLOAT_TO_UINT(green[i]);
                        dst[i*4+2] = FLOAT_TO_UINT(red[i]);
                        dst[i*4+3] = FLOAT_TO_UINT(alpha[i]);
                     }
                     break;
                  case GL_ABGR_EXT:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_UINT(alpha[i]);
                        dst[i*4+1] = FLOAT_TO_UINT(blue[i]);
                        dst[i*4+2] = FLOAT_TO_UINT(green[i]);
                        dst[i*4+3] = FLOAT_TO_UINT(red[i]);
                     }
                     break;
                  default:
                     gl_problem(ctx, "bad format in gl_pack_rgba_span\n");
               }
               if (packing->SwapBytes) {
                  gl_swap4( (GLuint *) dst, n * comps );
               }
            }
	    break;
	 case GL_INT:
	    {
               GLint *dst = (GLint *) destination;
               switch (format) {
                  case GL_RED:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_INT(red[i]);
                     break;
                  case GL_GREEN:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_INT(green[i]);
                     break;
                  case GL_BLUE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_INT(blue[i]);
                     break;
                  case GL_ALPHA:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_INT(alpha[i]);
                     break;
                  case GL_LUMINANCE:
                     for (i=0;i<n;i++)
                        dst[i] = FLOAT_TO_INT(luminance[i]);
                     break;
                  case GL_LUMINANCE_ALPHA:
                     for (i=0;i<n;i++) {
                        dst[i*2+0] = FLOAT_TO_INT(luminance[i]);
                        dst[i*2+1] = FLOAT_TO_INT(alpha[i]);
                     }
                     break;
                  case GL_RGB:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_INT(red[i]);
                        dst[i*3+1] = FLOAT_TO_INT(green[i]);
                        dst[i*3+2] = FLOAT_TO_INT(blue[i]);
                     }
                     break;
                  case GL_RGBA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_INT(red[i]);
                        dst[i*4+1] = FLOAT_TO_INT(green[i]);
                        dst[i*4+2] = FLOAT_TO_INT(blue[i]);
                        dst[i*4+3] = FLOAT_TO_INT(alpha[i]);
                     }
                     break;
                  case GL_BGR:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = FLOAT_TO_INT(blue[i]);
                        dst[i*3+1] = FLOAT_TO_INT(green[i]);
                        dst[i*3+2] = FLOAT_TO_INT(red[i]);
                     }
                     break;
                  case GL_BGRA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_INT(blue[i]);
                        dst[i*4+1] = FLOAT_TO_INT(green[i]);
                        dst[i*4+2] = FLOAT_TO_INT(red[i]);
                        dst[i*4+3] = FLOAT_TO_INT(alpha[i]);
                     }
                     break;
                  case GL_ABGR_EXT:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = FLOAT_TO_INT(alpha[i]);
                        dst[i*4+1] = FLOAT_TO_INT(blue[i]);
                        dst[i*4+2] = FLOAT_TO_INT(green[i]);
                        dst[i*4+3] = FLOAT_TO_INT(red[i]);
                     }
                     break;
                  default:
                     gl_problem(ctx, "bad format in gl_pack_rgba_span\n");
               }
	       if (packing->SwapBytes) {
		  gl_swap4( (GLuint *) dst, n * comps );
	       }
	    }
	    break;
	 case GL_FLOAT:
	    {
               GLfloat *dst = (GLfloat *) destination;
               switch (format) {
                  case GL_RED:
                     for (i=0;i<n;i++)
                        dst[i] = red[i];
                     break;
                  case GL_GREEN:
                     for (i=0;i<n;i++)
                        dst[i] = green[i];
                     break;
                  case GL_BLUE:
                     for (i=0;i<n;i++)
                        dst[i] = blue[i];
                     break;
                  case GL_ALPHA:
                     for (i=0;i<n;i++)
                        dst[i] = alpha[i];
                     break;
                  case GL_LUMINANCE:
                     for (i=0;i<n;i++)
                        dst[i] = luminance[i];
                     break;
                  case GL_LUMINANCE_ALPHA:
                     for (i=0;i<n;i++) {
                        dst[i*2+0] = luminance[i];
                        dst[i*2+1] = alpha[i];
                     }
                     break;
                  case GL_RGB:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = red[i];
                        dst[i*3+1] = green[i];
                        dst[i*3+2] = blue[i];
                     }
                     break;
                  case GL_RGBA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = red[i];
                        dst[i*4+1] = green[i];
                        dst[i*4+2] = blue[i];
                        dst[i*4+3] = alpha[i];
                     }
                     break;
                  case GL_BGR:
                     for (i=0;i<n;i++) {
                        dst[i*3+0] = blue[i];
                        dst[i*3+1] = green[i];
                        dst[i*3+2] = red[i];
                     }
                     break;
                  case GL_BGRA:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = blue[i];
                        dst[i*4+1] = green[i];
                        dst[i*4+2] = red[i];
                        dst[i*4+3] = alpha[i];
                     }
                     break;
                  case GL_ABGR_EXT:
                     for (i=0;i<n;i++) {
                        dst[i*4+0] = alpha[i];
                        dst[i*4+1] = blue[i];
                        dst[i*4+2] = green[i];
                        dst[i*4+3] = red[i];
                     }
                     break;
                  default:
                     gl_problem(ctx, "bad format in gl_pack_rgba_span\n");
               }
	       if (packing->SwapBytes) {
		  gl_swap4( (GLuint *) dst, n * comps );
	       }
	    }
	    break;
         case GL_UNSIGNED_BYTE_3_3_2:
            if (format == GL_RGB) {
               GLubyte *dst = (GLubyte *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLint) (red[i]   * 7.0F)) << 5)
                         | (((GLint) (green[i] * 7.0F)) << 2)
                         | (((GLint) (blue[i]  * 3.0F))     );
               }
            }
            break;
         case GL_UNSIGNED_BYTE_2_3_3_REV:
            if (format == GL_RGB) {
               GLubyte *dst = (GLubyte *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLint) (red[i]   * 7.0F))     )
                         | (((GLint) (green[i] * 7.0F)) << 3)
                         | (((GLint) (blue[i]  * 3.0F)) << 5);
               }
            }
            break;
         case GL_UNSIGNED_SHORT_5_6_5:
            if (format == GL_RGB) {
               GLushort *dst = (GLushort *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLint) (red[i]   * 31.0F)) << 11)
                         | (((GLint) (green[i] * 63.0F)) <<  5)
                         | (((GLint) (blue[i]  * 31.0F))      );
               }
            }
            break;
         case GL_UNSIGNED_SHORT_5_6_5_REV:
            if (format == GL_RGB) {
               GLushort *dst = (GLushort *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLint) (red[i]   * 31.0F))      )
                         | (((GLint) (green[i] * 63.0F)) <<  5)
                         | (((GLint) (blue[i]  * 31.0F)) << 11);
               }
            }
            break;
         case GL_UNSIGNED_SHORT_4_4_4_4:
            if (format == GL_RGB) {
               GLushort *dst = (GLushort *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLint) (red[i]   * 15.0F)) << 12)
                         | (((GLint) (green[i] * 15.0F)) <<  8)
                         | (((GLint) (blue[i]  * 15.0F)) <<  4)
                         | (((GLint) (alpha[i] * 15.0F))      );
               }
            }
            break;
         case GL_UNSIGNED_SHORT_4_4_4_4_REV:
            if (format == GL_RGB) {
               GLushort *dst = (GLushort *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLint) (red[i]   * 15.0F))      )
                         | (((GLint) (green[i] * 15.0F)) <<  4)
                         | (((GLint) (blue[i]  * 15.0F)) <<  8)
                         | (((GLint) (alpha[i] * 15.0F)) << 12);
               }
            }
            break;
         case GL_UNSIGNED_SHORT_5_5_5_1:
            if (format == GL_RGB) {
               GLushort *dst = (GLushort *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLint) (red[i]   * 31.0F)) << 11)
                         | (((GLint) (green[i] * 31.0F)) <<  6)
                         | (((GLint) (blue[i]  * 31.0F)) <<  1)
                         | (((GLint) (alpha[i] *  1.0F))      );
               }
            }
            break;
         case GL_UNSIGNED_SHORT_1_5_5_5_REV:
            if (format == GL_RGB) {
               GLushort *dst = (GLushort *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLint) (red[i]   * 31.0F))      )
                         | (((GLint) (green[i] * 31.0F)) <<  5)
                         | (((GLint) (blue[i]  * 31.0F)) << 10)
                         | (((GLint) (alpha[i] *  1.0F)) << 15);
               }
            }
            break;
         case GL_UNSIGNED_INT_8_8_8_8:
            if (format == GL_RGBA) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (red[i]   * 255.0F)) << 24)
                         | (((GLuint) (green[i] * 255.0F)) << 16)
                         | (((GLuint) (blue[i]  * 255.0F)) <<  8)
                         | (((GLuint) (alpha[i] * 255.0F))      );
               }
            }
            else if (format == GL_BGRA) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (blue[i]  * 255.0F)) << 24)
                         | (((GLuint) (green[i] * 255.0F)) << 16)
                         | (((GLuint) (red[i]   * 255.0F)) <<  8)
                         | (((GLuint) (alpha[i] * 255.0F))      );
               }
            }
            else if (format == GL_ABGR_EXT) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (alpha[i] * 255.0F)) << 24)
                         | (((GLuint) (blue[i]  * 255.0F)) << 16)
                         | (((GLuint) (green[i] * 255.0F)) <<  8)
                         | (((GLuint) (red[i]   * 255.0F))      );
               }
            }
            break;
         case GL_UNSIGNED_INT_8_8_8_8_REV:
            if (format == GL_RGBA) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (red[i]   * 255.0F))      )
                         | (((GLuint) (green[i] * 255.0F)) <<  8)
                         | (((GLuint) (blue[i]  * 255.0F)) << 16)
                         | (((GLuint) (alpha[i] * 255.0F)) << 24);
               }
            }
            else if (format == GL_BGRA) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (blue[i]  * 255.0F))      )
                         | (((GLuint) (green[i] * 255.0F)) <<  8)
                         | (((GLuint) (red[i]   * 255.0F)) << 16)
                         | (((GLuint) (alpha[i] * 255.0F)) << 24);
               }
            }
            else if (format == GL_ABGR_EXT) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (alpha[i] * 255.0F))      )
                         | (((GLuint) (blue[i]  * 255.0F)) <<  8)
                         | (((GLuint) (green[i] * 255.0F)) << 16)
                         | (((GLuint) (red[i]   * 255.0F)) << 24);
               }
            }
            break;
         case GL_UNSIGNED_INT_10_10_10_2:
            if (format == GL_RGBA) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (red[i]   * 1023.0F)) << 22)
                         | (((GLuint) (green[i] * 1023.0F)) << 12)
                         | (((GLuint) (blue[i]  * 1023.0F)) <<  2)
                         | (((GLuint) (alpha[i] *    3.0F))      );
               }
            }
            else if (format == GL_BGRA) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (blue[i]  * 1023.0F)) << 22)
                         | (((GLuint) (green[i] * 1023.0F)) << 12)
                         | (((GLuint) (red[i]   * 1023.0F)) <<  2)
                         | (((GLuint) (alpha[i] *    3.0F))      );
               }
            }
            else if (format == GL_ABGR_EXT) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (alpha[i] * 1023.0F)) << 22)
                         | (((GLuint) (blue[i]  * 1023.0F)) << 12)
                         | (((GLuint) (green[i] * 1023.0F)) <<  2)
                         | (((GLuint) (red[i]   *    3.0F))      );
               }
            }
            break;
         case GL_UNSIGNED_INT_2_10_10_10_REV:
            if (format == GL_RGBA) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (red[i]   * 1023.0F))      )
                         | (((GLuint) (green[i] * 1023.0F)) << 10)
                         | (((GLuint) (blue[i]  * 1023.0F)) << 20)
                         | (((GLuint) (alpha[i] *    3.0F)) << 30);
               }
            }
            else if (format == GL_BGRA) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (blue[i]  * 1023.0F))      )
                         | (((GLuint) (green[i] * 1023.0F)) << 10)
                         | (((GLuint) (red[i]   * 1023.0F)) << 20)
                         | (((GLuint) (alpha[i] *    3.0F)) << 30);
               }
            }
            else if (format == GL_ABGR_EXT) {
               GLuint *dst = (GLuint *) destination;
               for (i=0;i<n;i++) {
                  dst[i] = (((GLuint) (alpha[i] * 1023.0F))      )
                         | (((GLuint) (blue[i]  * 1023.0F)) << 10)
                         | (((GLuint) (green[i] * 1023.0F)) << 20)
                         | (((GLuint) (red[i]   *    3.0F)) << 30);
               }
            }
            break;
         default:
            gl_problem( ctx, "bad type in gl_pack_rgba_span" );
      }
   }
}


#define SWAP2BYTE(VALUE)			\
   {						\
      GLubyte *bytes = (GLubyte *) &(VALUE);	\
      GLubyte tmp = bytes[0];			\
      bytes[0] = bytes[1];			\
      bytes[1] = tmp;				\
   }

#define SWAP4BYTE(VALUE)			\
   {						\
      GLubyte *bytes = (GLubyte *) &(VALUE);	\
      GLubyte tmp = bytes[0];			\
      bytes[0] = bytes[3];			\
      bytes[3] = tmp;				\
      tmp = bytes[1];				\
      bytes[1] = bytes[2];			\
      bytes[2] = tmp;				\
   }


static void
extract_uint_indexes(GLuint n, GLuint indexes[],
                     GLenum srcFormat, GLenum srcType, const GLvoid *src,
                     const struct gl_pixelstore_attrib *unpack )
{
   assert(srcFormat == GL_COLOR_INDEX);

   ASSERT(srcType == GL_BITMAP ||
          srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_FLOAT);

   switch (srcType) {
      case GL_BITMAP:
         {
            GLubyte *ubsrc = (GLubyte *) src;
            if (unpack->LsbFirst) {
               GLubyte mask = 1 << (unpack->SkipPixels & 0x7);
               GLuint i;
               for (i = 0; i < n; i++) {
                  indexes[i] = (*ubsrc & mask) ? 1 : 0;
                  if (mask == 128) {
                     mask = 1;
                     ubsrc++;
                  }
                  else {
                     mask = mask << 1;
                  }
               }
            }
            else {
               GLubyte mask = 128 >> (unpack->SkipPixels & 0x7);
               GLuint i;
               for (i = 0; i < n; i++) {
                  indexes[i] = (*ubsrc & mask) ? 1 : 0;
                  if (mask == 1) {
                     mask = 128;
                     ubsrc++;
                  }
                  else {
                     mask = mask >> 1;
                  }
               }
            }
         }
         break;
      case GL_UNSIGNED_BYTE:
         {
            GLuint i;
            const GLubyte *s = (const GLubyte *) src;
            for (i = 0; i < n; i++)
               indexes[i] = s[i];
         }
         break;
      case GL_BYTE:
         {
            GLuint i;
            const GLbyte *s = (const GLbyte *) src;
            for (i = 0; i < n; i++)
               indexes[i] = s[i];
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLuint i;
            const GLushort *s = (const GLushort *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLushort value = s[i];
                  SWAP2BYTE(value);
                  indexes[i] = value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = s[i];
            }
         }
         break;
      case GL_SHORT:
         {
            GLuint i;
            const GLshort *s = (const GLshort *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLshort value = s[i];
                  SWAP2BYTE(value);
                  indexes[i] = value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = s[i];
            }
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint i;
            const GLuint *s = (const GLuint *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLuint value = s[i];
                  SWAP4BYTE(value);
                  indexes[i] = value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = s[i];
            }
         }
         break;
      case GL_INT:
         {
            GLuint i;
            const GLint *s = (const GLint *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLint value = s[i];
                  SWAP4BYTE(value);
                  indexes[i] = value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = s[i];
            }
         }
         break;
      case GL_FLOAT:
         {
            GLuint i;
            const GLfloat *s = (const GLfloat *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLfloat value = s[i];
                  SWAP4BYTE(value);
                  indexes[i] = value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = s[i];
            }
         }
         break;
      default:
         gl_problem(NULL, "bad srcType in extract_uint_indexes");
         return;
   }
}



/*
 * This function extracts floating point RGBA values from arbitrary
 * image data.  srcFormat and srcType are the format and type parameters
 * passed to glDrawPixels, glTexImage[123]D, glTexSubImage[123]D, etc.
 *
 * Refering to section 3.6.4 of the OpenGL 1.2 spec, this function
 * implements the "Conversion to floating point", "Conversion to RGB",
 * and "Final Expansion to RGBA" operations.
 *
 * Args:  n - number of pixels
 *        rgba - output colors
 *        srcFormat - format of incoming data
 *        srcType - datatype of incoming data
 *        src - source data pointer
 *        swapBytes - perform byteswapping of incoming data?
 */
static void
extract_float_rgba(GLuint n, GLfloat rgba[][4],
                   GLenum srcFormat, GLenum srcType, const GLvoid *src,
                   GLboolean swapBytes)
{
   GLint redIndex, greenIndex, blueIndex, alphaIndex;
   GLint stride;
   GLint rComp, bComp, gComp, aComp;

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
          srcFormat == GL_RGB ||
          srcFormat == GL_BGR ||
          srcFormat == GL_RGBA ||
          srcFormat == GL_BGRA ||
          srcFormat == GL_ABGR_EXT);

   ASSERT(srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_FLOAT ||
          srcType == GL_UNSIGNED_BYTE_3_3_2 ||
          srcType == GL_UNSIGNED_BYTE_2_3_3_REV ||
          srcType == GL_UNSIGNED_SHORT_5_6_5 ||
          srcType == GL_UNSIGNED_SHORT_5_6_5_REV ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4 ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
          srcType == GL_UNSIGNED_SHORT_5_5_5_1 ||
          srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV ||
          srcType == GL_UNSIGNED_INT_8_8_8_8 ||
          srcType == GL_UNSIGNED_INT_8_8_8_8_REV ||
          srcType == GL_UNSIGNED_INT_10_10_10_2 ||
          srcType == GL_UNSIGNED_INT_2_10_10_10_REV);

   rComp = gComp = bComp = aComp = -1;

   switch (srcFormat) {
      case GL_RED:
         redIndex = 0;
         greenIndex = blueIndex = alphaIndex = -1;
         stride = 1;
         break;
      case GL_GREEN:
         greenIndex = 0;
         redIndex = blueIndex = alphaIndex = -1;
         stride = 1;
         break;
      case GL_BLUE:
         blueIndex = 0;
         redIndex = greenIndex = alphaIndex = -1;
         stride = 1;
         break;
      case GL_ALPHA:
         redIndex = greenIndex = blueIndex = -1;
         alphaIndex = 0;
         stride = 1;
         break;
      case GL_LUMINANCE: 
         redIndex = greenIndex = blueIndex = 0;
         alphaIndex = -1;
         stride = 1;
         break;
      case GL_LUMINANCE_ALPHA:
         redIndex = greenIndex = blueIndex = 0;
         alphaIndex = 1;
         stride = 2;
         break;
      case GL_INTENSITY:
         redIndex = 0;
         greenIndex = blueIndex = alphaIndex = -1;
         stride = 1;
         break;
      case GL_RGB:
         redIndex = 0;
         greenIndex = 1;
         blueIndex = 2;
         alphaIndex = -1;
         stride = 3;
         break;
      case GL_BGR:
         redIndex = 2;
         greenIndex = 1;
         blueIndex = 0;
         alphaIndex = -1;
         stride = 3;
         break;
      case GL_RGBA:
         redIndex = 0;
         greenIndex = 1;
         blueIndex = 2;
         alphaIndex = 3;
         rComp = 0;
         gComp = 1;
         bComp = 2;
         aComp = 3;
         stride = 4;
         break;
      case GL_BGRA:
         redIndex = 2;
         greenIndex = 1;
         blueIndex = 0;
         alphaIndex = 3;
         rComp = 2;
         gComp = 1;
         bComp = 0;
         aComp = 3;
         stride = 4;
         break;
      case GL_ABGR_EXT:
         redIndex = 3;
         greenIndex = 2;
         blueIndex = 1;
         alphaIndex = 0;
         rComp = 3;
         gComp = 2;
         bComp = 1;
         aComp = 0;
         stride = 4;
         break;
      default:
         gl_problem(NULL, "bad srcFormat in extract float data");
         return;
   }


#define PROCESS(INDEX, CHANNEL, DEFAULT, TYPE, CONVERSION)		\
   if ((INDEX) < 0) {							\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         rgba[i][CHANNEL] = DEFAULT;					\
      }									\
   }									\
   else if (swapBytes) {						\
      const TYPE *s = (const TYPE *) src;				\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         TYPE value = s[INDEX];						\
         if (sizeof(TYPE) == 2) {					\
            SWAP2BYTE(value);						\
         }								\
         else if (sizeof(TYPE) == 4) {					\
            SWAP4BYTE(value);						\
         }								\
         rgba[i][CHANNEL] = (GLfloat) CONVERSION(value);		\
         s += stride;							\
      }									\
   }									\
   else {								\
      const TYPE *s = (const TYPE *) src;				\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         rgba[i][CHANNEL] = (GLfloat) CONVERSION(s[INDEX]);		\
         s += stride;							\
      }									\
   }

   switch (srcType) {
      case GL_UNSIGNED_BYTE:
         PROCESS(redIndex,   RCOMP, 0.0F, GLubyte, UBYTE_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLubyte, UBYTE_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLubyte, UBYTE_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLubyte, UBYTE_TO_FLOAT);
         break;
      case GL_BYTE:
         PROCESS(redIndex,   RCOMP, 0.0F, GLbyte, BYTE_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLbyte, BYTE_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLbyte, BYTE_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLbyte, BYTE_TO_FLOAT);
         break;
      case GL_UNSIGNED_SHORT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLushort, USHORT_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLushort, USHORT_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLushort, USHORT_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLushort, USHORT_TO_FLOAT);
         break;
      case GL_SHORT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLshort, SHORT_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLshort, SHORT_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLshort, SHORT_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLshort, SHORT_TO_FLOAT);
         break;
      case GL_UNSIGNED_INT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLuint, UINT_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLuint, UINT_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLuint, UINT_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLuint, UINT_TO_FLOAT);
         break;
      case GL_INT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLint, INT_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLint, INT_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLint, INT_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLint, INT_TO_FLOAT);
         break;
      case GL_FLOAT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLfloat, (GLfloat));
         PROCESS(greenIndex, GCOMP, 0.0F, GLfloat, (GLfloat));
         PROCESS(blueIndex,  BCOMP, 0.0F, GLfloat, (GLfloat));
         PROCESS(alphaIndex, ACOMP, 1.0F, GLfloat, (GLfloat));
         break;
      case GL_UNSIGNED_BYTE_3_3_2:
         {
            const GLubyte *ubsrc = (const GLubyte *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLubyte p = ubsrc[i];
               rgba[i][RCOMP] = ((p >> 5)      ) * (1.0F / 7.0F);
               rgba[i][GCOMP] = ((p >> 2) & 0x7) * (1.0F / 7.0F);
               rgba[i][BCOMP] = ((p     ) & 0x3) * (1.0F / 3.0F);
               rgba[i][ACOMP] = 1.0F;
            }
         }
         break;
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         {
            const GLubyte *ubsrc = (const GLubyte *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLubyte p = ubsrc[i];
               rgba[i][RCOMP] = ((p     ) & 0x7) * (1.0F / 7.0F);
               rgba[i][GCOMP] = ((p >> 3) & 0x7) * (1.0F / 7.0F);
               rgba[i][BCOMP] = ((p >> 6)      ) * (1.0F / 3.0F);
               rgba[i][ACOMP] = 1.0F;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][RCOMP] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][GCOMP] = ((p >>  5) & 0x3f) * (1.0F / 63.0F);
               rgba[i][BCOMP] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][ACOMP] = 1.0F;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][RCOMP] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][GCOMP] = ((p >>  5) & 0x3f) * (1.0F / 63.0F);
               rgba[i][BCOMP] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][ACOMP] = 1.0F;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][RCOMP] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][GCOMP] = ((p >>  5) & 0x3f) * (1.0F / 63.0F);
               rgba[i][BCOMP] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][ACOMP] = 1.0F;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][RCOMP] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][GCOMP] = ((p >>  5) & 0x3f) * (1.0F / 63.0F);
               rgba[i][BCOMP] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][ACOMP] = 1.0F;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rComp] = ((p >> 12)      ) * (1.0F / 15.0F);
               rgba[i][gComp] = ((p >>  8) & 0xf) * (1.0F / 15.0F);
               rgba[i][bComp] = ((p >>  4) & 0xf) * (1.0F / 15.0F);
               rgba[i][aComp] = ((p      ) & 0xf) * (1.0F / 15.0F);
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p >> 12)      ) * (1.0F / 15.0F);
               rgba[i][gComp] = ((p >>  8) & 0xf) * (1.0F / 15.0F);
               rgba[i][bComp] = ((p >>  4) & 0xf) * (1.0F / 15.0F);
               rgba[i][aComp] = ((p      ) & 0xf) * (1.0F / 15.0F);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rComp] = ((p      ) & 0xf) * (1.0F / 15.0F);
               rgba[i][gComp] = ((p >>  4) & 0xf) * (1.0F / 15.0F);
               rgba[i][bComp] = ((p >>  8) & 0xf) * (1.0F / 15.0F);
               rgba[i][aComp] = ((p >> 12)      ) * (1.0F / 15.0F);
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p      ) & 0xf) * (1.0F / 15.0F);
               rgba[i][gComp] = ((p >>  4) & 0xf) * (1.0F / 15.0F);
               rgba[i][bComp] = ((p >>  8) & 0xf) * (1.0F / 15.0F);
               rgba[i][aComp] = ((p >> 12)      ) * (1.0F / 15.0F);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_5_5_1:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rComp] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  6) & 0x1f) * (1.0F / 31.0F);
               rgba[i][bComp] = ((p >>  1) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = ((p      ) & 0x1)  * (1.0F /  1.0F);
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  6) & 0x1f) * (1.0F / 31.0F);
               rgba[i][bComp] = ((p >>  1) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = ((p      ) & 0x1)  * (1.0F /  1.0F);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rComp] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  5) & 0x1f) * (1.0F / 31.0F);
               rgba[i][bComp] = ((p >> 10) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = ((p >> 15)       ) * (1.0F /  1.0F);
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  5) & 0x1f) * (1.0F / 31.0F);
               rgba[i][bComp] = ((p >> 10) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = ((p >> 15)       ) * (1.0F /  1.0F);
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p      ) & 0xff);
               rgba[i][gComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >>  8) & 0xff);
               rgba[i][bComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >> 16) & 0xff);
               rgba[i][aComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >> 24)       );
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >> 24)       );
               rgba[i][gComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >> 16) & 0xff);
               rgba[i][bComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >>  8) & 0xff);
               rgba[i][aComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p      ) & 0xff);
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8_REV:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >> 24)       ); 
               rgba[i][gComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >> 16) & 0xff);
               rgba[i][bComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >>  8) & 0xff);
               rgba[i][aComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p      ) & 0xff);
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p      ) & 0xff);
               rgba[i][gComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >>  8) & 0xff);
               rgba[i][bComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >> 16) & 0xff);
               rgba[i][aComp] = UBYTE_COLOR_TO_FLOAT_COLOR((p >> 24)       ); 
            }
         }
         break;
      case GL_UNSIGNED_INT_10_10_10_2:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               SWAP4BYTE(p);
               rgba[i][rComp] = ((p >> 22)        ) * (1.0F / 1023.0F);
               rgba[i][gComp] = ((p >> 12) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][bComp] = ((p >>  2) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][aComp] = ((p      ) & 0x3  ) * (1.0F /    3.0F);
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = ((p >> 22)        ) * (1.0F / 1023.0F);
               rgba[i][gComp] = ((p >> 12) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][bComp] = ((p >>  2) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][aComp] = ((p      ) & 0x3  ) * (1.0F /    3.0F);
            }
         }
         break;
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               SWAP4BYTE(p);
               rgba[i][rComp] = ((p      ) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][gComp] = ((p >> 10) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][bComp] = ((p >> 20) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][aComp] = ((p >> 30)        ) * (1.0F /    3.0F);
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = ((p      ) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][gComp] = ((p >> 10) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][bComp] = ((p >> 20) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][aComp] = ((p >> 30)        ) * (1.0F /    3.0F);
            }
         }
         break;
      default:
         gl_problem(NULL, "bad srcType in extract float data");
         break;
   }
}



/*
 * Unpack a row of color image data from a client buffer according to
 * the pixel unpacking parameters.  Apply any enabled pixel transfer
 * ops (PixelMap, scale/bias) if the applyTransferOps flag is enabled.
 * Return GLubyte values in the specified dest image format.
 * This is (or will be) used by glDrawPixels and glTexImage?D().
 * Input:  ctx - the context
 *         n - number of pixels in the span
 *         dstFormat - format of destination color array
 *         dest - the destination color array
 *         srcFormat - source image format
 *         srcType - source image  datatype
 *         source - source image pointer
 *         unpacking - pixel unpacking parameters
 *         applyTransferOps - apply scale/bias/lookup-table ops?
 *
 * XXX perhaps expand this to process whole images someday.
 */
void
_mesa_unpack_ubyte_color_span( const GLcontext *ctx,
                               GLuint n, GLenum dstFormat, GLubyte dest[],
                               GLenum srcFormat, GLenum srcType,
                               const GLvoid *source,
                               const struct gl_pixelstore_attrib *unpacking,
                               GLboolean applyTransferOps )
{
   ASSERT(dstFormat == GL_ALPHA ||
          dstFormat == GL_LUMINANCE || 
          dstFormat == GL_LUMINANCE_ALPHA ||
          dstFormat == GL_INTENSITY ||
          dstFormat == GL_RGB ||
          dstFormat == GL_RGBA ||
          dstFormat == GL_COLOR_INDEX);

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
          srcFormat == GL_RGB ||
          srcFormat == GL_BGR ||
          srcFormat == GL_RGBA ||
          srcFormat == GL_BGRA ||
          srcFormat == GL_ABGR_EXT ||
          srcFormat == GL_COLOR_INDEX);

   ASSERT(srcType == GL_BITMAP ||
          srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_FLOAT ||
          srcType == GL_UNSIGNED_BYTE_3_3_2 ||
          srcType == GL_UNSIGNED_BYTE_2_3_3_REV ||
          srcType == GL_UNSIGNED_SHORT_5_6_5 ||
          srcType == GL_UNSIGNED_SHORT_5_6_5_REV ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4 ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
          srcType == GL_UNSIGNED_SHORT_5_5_5_1 ||
          srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV ||
          srcType == GL_UNSIGNED_INT_8_8_8_8 ||
          srcType == GL_UNSIGNED_INT_8_8_8_8_REV ||
          srcType == GL_UNSIGNED_INT_10_10_10_2 ||
          srcType == GL_UNSIGNED_INT_2_10_10_10_REV);

   /* this is intended for RGBA mode */
   assert(ctx->Visual->RGBAflag);

   applyTransferOps &= (ctx->Pixel.ScaleOrBiasRGBA ||
                        ctx->Pixel.MapColorFlag ||
                        ctx->Pixel.MapColorFlag);

   /* Try simple cases first */
   if (!applyTransferOps && srcType == GL_UNSIGNED_BYTE) {
      if (dstFormat == GL_RGBA) {
         if (srcFormat == GL_RGBA) {
            MEMCPY( dest, source, n * 4 * sizeof(GLubyte) );
            return;
         }
         else if (srcFormat == GL_RGB) {
            GLuint i;
            const GLubyte *src = (const GLubyte *) source;
            GLubyte *dst = dest;
            for (i = 0; i < n; i++) {
               dst[0] = src[0];
               dst[1] = src[1];
               dst[2] = src[2];
               dst[3] = 255;
               src += 3;
               dst += 4;
            }
            return;
         }
      }
      else if (dstFormat == GL_RGB) {
         if (srcFormat == GL_RGB) {
            MEMCPY( dest, source, n * 3 * sizeof(GLubyte) );
            return;
         }
         else if (srcFormat == GL_RGBA) {
            GLuint i;
            const GLubyte *src = (const GLubyte *) source;
            GLubyte *dst = dest;
            for (i = 0; i < n; i++) {
               dst[0] = src[0];
               dst[1] = src[1];
               dst[2] = src[2];
               src += 4;
               dst += 3;
            }
            return;
         }
      }
      else if (dstFormat == srcFormat) {
         GLint comps = gl_components_in_format(srcFormat);
         assert(comps > 0);
         MEMCPY( dest, source, n * comps * sizeof(GLubyte) );
         return;
      }
   }


   {
      /* general solution */
      GLfloat rgba[MAX_WIDTH][4];
      GLint dstComponents;
      GLint dstRedIndex, dstGreenIndex, dstBlueIndex, dstAlphaIndex;
      GLint dstLuminanceIndex, dstIntensityIndex;

      dstComponents = gl_components_in_format( dstFormat );
      /* source & dest image formats should have been error checked by now */
      assert(dstComponents > 0);

      /*
       * Extract image data and convert to RGBA floats
       */
      assert(n <= MAX_WIDTH);
      if (srcFormat == GL_COLOR_INDEX) {
         GLuint indexes[MAX_WIDTH];
         extract_uint_indexes(n, indexes, srcFormat, srcType, source,
                              unpacking);

         /* shift and offset indexes */
         gl_shift_and_offset_ci(ctx, n, indexes);

         if (dstFormat == GL_COLOR_INDEX) {
            if (applyTransferOps) {
               if (ctx->Pixel.MapColorFlag) {
                  /* Apply lookup table */
                  gl_map_ci(ctx, n, indexes);
               }

               if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset) {

               }
            }

            /* convert to GLubyte and return */
            {
               GLuint i;
               for (i = 0; i < n; i++) {
                  dest[i] = (GLubyte) (indexes[i] & 0xff);
               }
            }
         }
         else {
            /* Convert indexes to RGBA */
            gl_map_ci_to_rgba_float(ctx, n, indexes, rgba);
         }
      }
      else {
         extract_float_rgba(n, rgba, srcFormat, srcType, source,
                            unpacking->SwapBytes);

         if (applyTransferOps) {
            /* scale and bias colors */
            gl_scale_and_bias_rgba_float(ctx, n, rgba);

            /* color table lookup */
            if (ctx->Pixel.MapColorFlag) {
               gl_map_rgba_float(ctx, n, rgba);
            }
         }
      }


      /*
       * XXX This is where more color table lookups, convolution,
       * histograms, minmax, color matrix, etc would take place if
       * implemented.
       * See figure 3.7 in the OpenGL 1.2 specification for more info.
       */


      /* clamp to [0,1] */
      {
         GLuint i;
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = CLAMP(rgba[i][RCOMP], 0.0F, 1.0F);
            rgba[i][GCOMP] = CLAMP(rgba[i][GCOMP], 0.0F, 1.0F);
            rgba[i][BCOMP] = CLAMP(rgba[i][BCOMP], 0.0F, 1.0F);
            rgba[i][ACOMP] = CLAMP(rgba[i][ACOMP], 0.0F, 1.0F);
         }
      }

      /* Now determine which color channels we need to produce.
       * And determine the dest index (offset) within each color tuple.
       */
      switch (dstFormat) {
         case GL_ALPHA:
            dstAlphaIndex = 0;
            dstRedIndex = dstGreenIndex = dstBlueIndex = -1;
            dstLuminanceIndex = dstIntensityIndex = -1;
            break;
         case GL_LUMINANCE: 
            dstLuminanceIndex = 0;
            dstRedIndex = dstGreenIndex = dstBlueIndex = dstAlphaIndex = -1;
            dstIntensityIndex = -1;
            break;
         case GL_LUMINANCE_ALPHA:
            dstLuminanceIndex = 0;
            dstAlphaIndex = 1;
            dstRedIndex = dstGreenIndex = dstBlueIndex = -1;
            dstIntensityIndex = -1;
            break;
         case GL_INTENSITY:
            dstIntensityIndex = 0;
            dstRedIndex = dstGreenIndex = dstBlueIndex = dstAlphaIndex = -1;
            dstLuminanceIndex = -1;
            break;
         case GL_RGB:
            dstRedIndex = 0;
            dstGreenIndex = 1;
            dstBlueIndex = 2;
            dstAlphaIndex = dstLuminanceIndex = dstIntensityIndex = -1;
            break;
         case GL_RGBA:
            dstRedIndex = 0;
            dstGreenIndex = 1;
            dstBlueIndex = 2;
            dstAlphaIndex = 3;
            dstLuminanceIndex = dstIntensityIndex = -1;
            break;
         default:
            gl_problem(ctx, "bad dstFormat in _mesa_unpack_ubyte_span()");
            return;
      }


      /* Now return the GLubyte data in the requested dstFormat */

      if (dstRedIndex >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[dstRedIndex] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
            dst += dstComponents;
         }
      }

      if (dstGreenIndex >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[dstGreenIndex] = FLOAT_TO_UBYTE(rgba[i][GCOMP]);
            dst += dstComponents;
         }
      }

      if (dstBlueIndex >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[dstBlueIndex] = FLOAT_TO_UBYTE(rgba[i][BCOMP]);
            dst += dstComponents;
         }
      }

      if (dstAlphaIndex >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[dstAlphaIndex] = FLOAT_TO_UBYTE(rgba[i][ACOMP]);
            dst += dstComponents;
         }
      }

      if (dstIntensityIndex >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         assert(dstIntensityIndex == 0);
         assert(dstComponents == 1);
         for (i = 0; i < n; i++) {
            /* Intensity comes from red channel */
            dst[i] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
         }
      }

      if (dstLuminanceIndex >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         assert(dstLuminanceIndex == 0);
         for (i = 0; i < n; i++) {
            /* Luminance comes from red channel */
            dst[0] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
            dst += dstComponents;
         }
      }
   }
}



/*
 * Unpack a row of color index data from a client buffer according to
 * the pixel unpacking parameters.  Apply pixel transfer ops if enabled
 * and applyTransferOps is true.
 * This is (or will be) used by glDrawPixels, glTexImage[123]D, etc.
 *
 * Args:  ctx - the context
 *        n - number of pixels
 *        dstType - destination datatype
 *        dest - destination array
 *        srcType - source pixel type
 *        source - source data pointer
 *        unpacking - pixel unpacking parameters
 *        applyTransferOps - apply offset/bias/lookup ops?
 */
void
_mesa_unpack_index_span( const GLcontext *ctx, GLuint n,
                         GLenum dstType, GLvoid *dest,
                         GLenum srcType, const GLvoid *source,
                         const struct gl_pixelstore_attrib *unpacking,
                         GLboolean applyTransferOps )
{
   ASSERT(srcType == GL_BITMAP ||
          srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_FLOAT);

   ASSERT(dstType == GL_UNSIGNED_BYTE ||
          dstType == GL_UNSIGNED_SHORT ||
          dstType == GL_UNSIGNED_INT);

   applyTransferOps &= (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset || ctx->Pixel.MapColorFlag);

   /*
    * Try simple cases first
    */
   if (!applyTransferOps && srcType == GL_UNSIGNED_BYTE
       && dstType == GL_UNSIGNED_BYTE) {
      MEMCPY(dest, source, n * sizeof(GLubyte));
   }
   else if (!applyTransferOps && srcType == GL_UNSIGNED_INT
            && dstType == GL_UNSIGNED_INT && !unpacking->SwapBytes) {
      MEMCPY(dest, source, n * sizeof(GLuint));
   }
   else {
      /*
       * general solution
       */
      GLuint indexes[MAX_WIDTH];
      assert(n <= MAX_WIDTH);

      extract_uint_indexes(n, indexes, GL_COLOR_INDEX, srcType, source,
                           unpacking);

      if (applyTransferOps) {
         if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset) {
            /* shift and offset indexes */
            gl_shift_and_offset_ci(ctx, n, indexes);
         }

         if (ctx->Pixel.MapColorFlag) {
            /* Apply lookup table */
            gl_map_ci(ctx, n, indexes);
         }
      }

      /* convert to dest type */
      switch (dstType) {
         case GL_UNSIGNED_BYTE:
            {
               GLubyte *dst = (GLubyte *) dest;
               GLuint i;
               for (i = 0; i < n; i++) {
                  dst[i] = (GLubyte) (indexes[i] & 0xff);
               }
            }
            break;
         case GL_UNSIGNED_SHORT:
            {
               GLuint *dst = (GLuint *) dest;
               GLuint i;
               for (i = 0; i < n; i++) {
                  dst[i] = (GLushort) (indexes[i] & 0xffff);
               }
            }
            break;
         case GL_UNSIGNED_INT:
            MEMCPY(dest, indexes, n * sizeof(GLuint));
            break;
         default:
            gl_problem(ctx, "bad dstType in _mesa_unpack_index_span");
      }
   }
}


/*
 * Unpack a row of stencil data from a client buffer according to
 * the pixel unpacking parameters.  Apply pixel transfer ops if enabled
 * and applyTransferOps is true.
 * This is (or will be) used by glDrawPixels
 *
 * Args:  ctx - the context
 *        n - number of pixels
 *        dstType - destination datatype
 *        dest - destination array
 *        srcType - source pixel type
 *        source - source data pointer
 *        unpacking - pixel unpacking parameters
 *        applyTransferOps - apply offset/bias/lookup ops?
 */
void
_mesa_unpack_stencil_span( const GLcontext *ctx, GLuint n,
                           GLenum dstType, GLvoid *dest,
                           GLenum srcType, const GLvoid *source,
                           const struct gl_pixelstore_attrib *unpacking,
                           GLboolean applyTransferOps )
{
   ASSERT(srcType == GL_BITMAP ||
          srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_FLOAT);

   ASSERT(dstType == GL_UNSIGNED_BYTE ||
          dstType == GL_UNSIGNED_SHORT ||
          dstType == GL_UNSIGNED_INT);

   applyTransferOps &= (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset || ctx->Pixel.MapColorFlag);

   /*
    * Try simple cases first
    */
   if (!applyTransferOps && srcType == GL_UNSIGNED_BYTE
       && dstType == GL_UNSIGNED_BYTE) {
      MEMCPY(dest, source, n * sizeof(GLubyte));
   }
   else if (!applyTransferOps && srcType == GL_UNSIGNED_INT
            && dstType == GL_UNSIGNED_INT && !unpacking->SwapBytes) {
      MEMCPY(dest, source, n * sizeof(GLuint));
   }
   else {
      /*
       * general solution
       */
      GLuint indexes[MAX_WIDTH];
      assert(n <= MAX_WIDTH);

      extract_uint_indexes(n, indexes, GL_COLOR_INDEX, srcType, source,
                           unpacking);

      if (applyTransferOps) {
         if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset) {
            /* shift and offset indexes */
            gl_shift_and_offset_ci(ctx, n, indexes);
         }

         if (ctx->Pixel.MapStencilFlag) {
            /* Apply stencil lookup table */
            GLuint mask = ctx->Pixel.MapStoSsize - 1;
            GLuint i;
            for (i=0;i<n;i++) {
               indexes[i] = ctx->Pixel.MapStoS[ indexes[i] & mask ];
            }
         }
      }

      /* convert to dest type */
      switch (dstType) {
         case GL_UNSIGNED_BYTE:
            {
               GLubyte *dst = (GLubyte *) dest;
               GLuint i;
               for (i = 0; i < n; i++) {
                  dst[i] = (GLubyte) (indexes[i] & 0xff);
               }
            }
            break;
         case GL_UNSIGNED_SHORT:
            {
               GLuint *dst = (GLuint *) dest;
               GLuint i;
               for (i = 0; i < n; i++) {
                  dst[i] = (GLushort) (indexes[i] & 0xffff);
               }
            }
            break;
         case GL_UNSIGNED_INT:
            MEMCPY(dest, indexes, n * sizeof(GLuint));
            break;
         default:
            gl_problem(ctx, "bad dstType in _mesa_unpack_stencil_span");
      }
   }
}



void
_mesa_unpack_depth_span( const GLcontext *ctx, GLuint n, GLdepth *dest,
                         GLenum srcType, const GLvoid *source,
                         const struct gl_pixelstore_attrib *unpacking,
                         GLboolean applyTransferOps )
{
   GLfloat *depth = MALLOC(n * sizeof(GLfloat));
   if (!depth)
      return;

   switch (srcType) {
      case GL_BYTE:
         {
            GLuint i;
            const GLubyte *src = (const GLubyte *) source;
            for (i = 0; i < n; i++) {
               depth[i] = BYTE_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_UNSIGNED_BYTE:
         {
            GLuint i;
            const GLubyte *src = (const GLubyte *) source;
            for (i = 0; i < n; i++) {
               depth[i] = UBYTE_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_SHORT:
         {
            GLuint i;
            const GLshort *src = (const GLshort *) source;
            for (i = 0; i < n; i++) {
               depth[i] = SHORT_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLuint i;
            const GLushort *src = (const GLushort *) source;
            for (i = 0; i < n; i++) {
               depth[i] = USHORT_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_INT:
         {
            GLuint i;
            const GLint *src = (const GLint *) source;
            for (i = 0; i < n; i++) {
               depth[i] = INT_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint i;
            const GLuint *src = (const GLuint *) source;
            for (i = 0; i < n; i++) {
               depth[i] = UINT_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_FLOAT:
         MEMCPY(depth, source, n * sizeof(GLfloat));
         break;
      default:
         gl_problem(NULL, "bad type in _mesa_unpack_depth_span()");
         return;
   }


   /* apply depth scale and bias */
   if (ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         depth[i] = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
      }
   }

   /* clamp depth values to [0,1] and convert from floats to integers */
   {
      GLuint i;
      for (i = 0; i < n; i++) {
         dest[i] = (GLdepth) (CLAMP(depth[i], 0.0F, 1.0F) * DEPTH_SCALE);
      }
   }

   FREE(depth);
}



/*
 * Unpack image data.  Apply byteswapping, byte flipping (bitmap).
 * Return all image data in a contiguous block.
 */
void *
_mesa_unpack_image( GLsizei width, GLsizei height, GLsizei depth,
                    GLenum format, GLenum type, const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *unpack )
{
   GLint bytesPerRow, compsPerRow;
   GLboolean flipBytes, swap2, swap4;

   if (!pixels)
      return NULL;  /* not necessarily an error */

   if (width <= 0 || height <= 0 || depth <= 0)
      return NULL;  /* generate error later */

   if (format == GL_BITMAP) {
      bytesPerRow = (width + 7) >> 3;
      flipBytes = !unpack->LsbFirst;
      swap2 = swap4 = GL_FALSE;
      compsPerRow = 0;
   }
   else {
      const GLint bytesPerPixel = gl_bytes_per_pixel(format, type);
      const GLint components = gl_components_in_format(format);
      GLint bytesPerComp;
      if (bytesPerPixel <= 0 || components <= 0)
         return NULL;   /* bad format or type.  generate error later */
      bytesPerRow = bytesPerPixel * width;
      bytesPerComp = bytesPerPixel / components;
      flipBytes = GL_FALSE;
      swap2 = (bytesPerComp == 2) && unpack->SwapBytes;
      swap4 = (bytesPerComp == 4) && unpack->SwapBytes;
      compsPerRow = components * width;
      assert(compsPerRow >= width);
   }

   {
      GLubyte *destBuffer = MALLOC(bytesPerRow * height * depth);
      GLubyte *dst;
      GLint img, row;
      if (!destBuffer)
         return NULL;   /* generate GL_OUT_OF_MEMORY later */

      dst = destBuffer;
      for (img = 0; img < depth; img++) {
         for (row = 0; row < height; row++) {
            const GLvoid *src = gl_pixel_addr_in_image(unpack, pixels,
                               width, height, format, type, img, row, 0);
            MEMCPY(dst, src, bytesPerRow);
            /* byte flipping/swapping */
            if (flipBytes) {
               gl_flip_bytes((GLubyte *) dst, bytesPerRow);
            }
            else if (swap2) {
               gl_swap2((GLushort*) dst, compsPerRow);
            }
            else if (swap4) {
               gl_swap4((GLuint*) dst, compsPerRow);
            }
            dst += bytesPerRow;
         }
      }
      return destBuffer;
   }
}


/*
 * Unpack bitmap data.  Resulting data will be in most-significant-bit-first
 * order with row alignment = 1 byte.
 */
GLvoid *
_mesa_unpack_bitmap( GLint width, GLint height, const GLubyte *pixels,
                     const struct gl_pixelstore_attrib *packing )
{
   GLint bytes, row, width_in_bytes;
   GLubyte *buffer, *dst;

   if (!pixels)
      return NULL;

   /* Alloc dest storage */
   bytes = ((width + 7) / 8 * height);
   buffer = (GLubyte *) MALLOC( bytes );
   if (!buffer)
      return NULL;


   width_in_bytes = CEILING( width, 8 );
   dst = buffer;
   for (row = 0; row < height; row++) {
      GLubyte *src = gl_pixel_addr_in_image( packing, pixels, width, height,
                                             GL_COLOR_INDEX, GL_BITMAP,
                                             0, row, 0 );
      if (!src) {
         FREE(buffer);
         return NULL;
      }

      if (packing->SkipPixels == 0) {
         MEMCPY( dst, src, width_in_bytes );
         if (packing->LsbFirst) {
            gl_flip_bytes( dst, width_in_bytes );
         }
      }
      else {
         /* handling SkipPixels is a bit tricky (no pun intended!) */
         GLint i;
         if (packing->LsbFirst) {
            GLubyte srcMask = 1 << (packing->SkipPixels & 0x7);
            GLubyte dstMask = 128;
            GLubyte *s = src;
            GLubyte *d = dst;
            *d = 0;
            for (i = 0; i < width; i++) {
               if (*s & srcMask) {
                  *d |= dstMask;
               }
               if (srcMask == 128) {
                  srcMask = 1;
                  s++;
               }
               else {
                  srcMask = srcMask << 1;
               }
               if (dstMask == 1) {
                  dstMask = 128;
                  d++;
                  *d = 0;
               }
               else {
                  dstMask = dstMask >> 1;
               }
            }
         }
         else {
            GLubyte srcMask = 128 >> (packing->SkipPixels & 0x7);
            GLubyte dstMask = 128;
            GLubyte *s = src;
            GLubyte *d = dst;
            *d = 0;
            for (i = 0; i < width; i++) {
               if (*s & srcMask) {
                  *d |= dstMask;
               }
               if (srcMask == 1) {
                  srcMask = 128;
                  s++;
               }
               else {
                  srcMask = srcMask >> 1;
               }
               if (dstMask == 1) {
                  dstMask = 128;
                  d++;
                  *d = 0;
               }
               else {
                  dstMask = dstMask >> 1;
               }
            }
         }
      }
      dst += width_in_bytes;
   }

   return buffer;
}
