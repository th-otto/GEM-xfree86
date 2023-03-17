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
 *    John Carmack <johnc@idsoftware.com>
 *    Keith Whitwell <keithw@precisioninsight.com>
 */

#ifndef MGATEX_INC
#define MGATEX_INC

#include "mga_drm_public.h"
#include "types.h"
#include "mgacommon.h"
#include "mm.h"


#define MGA_TEX_MAXLEVELS 5


typedef struct mga_texture_object_s {
	struct mga_texture_object_s *next;	
	struct mga_texture_object_s *prev;	
	struct gl_texture_object *tObj;
	mgaContextPtr ctx;
	PMemBlock	MemBlock;               
	mgaUI32		offsets[MGA_TEX_MAXLEVELS];
        int             lastLevel;
        mgaUI32         dirty_images;
	mgaUI32		totalSize;		
	int		texelBytes;
	mgaUI32 	age;   			
        mgaUI32         Setup[MGA_TEX_SETUP_SIZE];
} mgaTextureObject_t;

typedef mgaTextureObject_t *mgaTextureObjectPtr;

/* called to check for environment variable options */
void mgaInitTextureSystem( void );

/* called when a context is being destroyed */
void mgaDestroyContextTextures( mgaContextPtr ctx );

/* Called before a primitive is rendered to make sure the texture
 * state is properly setup.  Texture residence is checked later
 * when we grab the lock.
 */
void mgaUpdateTextureState( GLcontext *ctx );


/* Driver functions which are called directly from mesa */

void mgaTexEnv( GLcontext *ctx, GLenum pname, const GLfloat *param );

void mgaTexImage( GLcontext *ctx, GLenum target,
		  struct gl_texture_object *tObj, GLint level,
		  GLint internalFormat,
		  const struct gl_texture_image *image );

void mgaTexSubImage( GLcontext *ctx, GLenum target,
		  struct gl_texture_object *tObj, GLint level,
		  GLint xoffset, GLint yoffset,
		  GLsizei width, GLsizei height,
		  GLint internalFormat,
		  const struct gl_texture_image *image );

void mgaTexParameter( GLcontext *ctx, GLenum target,
		      struct gl_texture_object *tObj,
		      GLenum pname, const GLfloat *params );

void mgaBindTexture( GLcontext *ctx, GLenum target,
		     struct gl_texture_object *tObj );

void mgaDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj );

void mgaUpdateTexturePalette( GLcontext *ctx, struct gl_texture_object *tObj );

GLboolean mgaIsTextureResident( GLcontext *ctx, struct gl_texture_object *t );

void mgaConvertTexture( mgaUI32 *destPtr, int texelBytes,
			struct gl_texture_image *image,
			int x, int y, int width, int height );



int mgaUploadTexImages( mgaContextPtr mmesa, mgaTextureObjectPtr t );




void mgaResetGlobalLRU( mgaContextPtr mmesa );
void mgaTexturesGone( mgaContextPtr mmesa, GLuint offset, 
		      GLuint size, GLuint in_use );

void mgaDDInitTextureFuncs( GLcontext *ctx );




#endif
