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
 */


#ifndef I810CONTEXT_INC
#define I810CONTEXT_INC

typedef struct i810_context_t i810Context;
typedef struct i810_context_t *i810ContextPtr;

#include <X11/Xlibint.h>
#include "dri_tmm.h"
#include "dri_mesaint.h"
#include "dri_mesa.h"
#include "xmesaP.h"

#include "types.h"
#include "i810_init.h"
#include "i810_sarea.h"

#include "i810tex.h"
#include "i810vb.h"



#define I810_FALLBACK_TEXTURE   0x1
#define I810_FALLBACK_BUFFER    0x2



/* for i810ctx.new_state - manage GL->driver state changes
 */
#define I810_NEW_TEXTURE 0x1

/* for i810ctx.dirty - manage driver->hw state changes, including
 * lost contexts.
 */
#define I810_REQUIRE_QUIESCENT 0x1
#define I810_UPLOAD_TEX0IMAGE  0x2
#define I810_UPLOAD_TEX1IMAGE  0x4
#define I810_UPLOAD_CTX        0x8
#define I810_UPLOAD_BUFFERS    0x10
#define I810_REFRESH_RING      0x20
#define I810_EMIT_CLIPRECT     0x40


typedef void (*i810_interp_func)( GLfloat t, 
				  GLfloat *result,
				  const GLfloat *in,
				  const GLfloat *out );
				 


struct i810_context_t {
   GLint refcount;

   GLcontext *glCtx;

   i810TextureObjectPtr CurrentTexObj[2];

   struct i810_texture_object_t TexObjList;
   struct i810_texture_object_t SwappedOut; 

   int TextureMode;

   /* Hardware state
    */
   GLuint Setup[I810_CTX_SETUP_SIZE];
   GLuint BufferSetup[I810_DEST_SETUP_SIZE];
   GLuint ClipSetup[I810_CLIP_SETUP_SIZE];
   

   /* Support for CVA and the fast paths.
    */
   GLuint setupdone;
   GLuint setupindex;
   GLuint renderindex;
   GLuint using_fast_path;
   i810_interp_func interp;

   /* Shortcircuit some state changes.
    */
   points_func PointsFunc;
   line_func LineFunc;
   triangle_func TriangleFunc;
   quad_func QuadFunc;

   /* Manage our own state */
   GLuint new_state; 

   /* Manage hardware state */
   GLuint dirty;
   memHeap_t *texHeap;

   /* One of the few bits of hardware state that can't be calculated
    * completely on the fly:
    */
   GLuint LcsCullMode;

   /* Funny mesa mirrors
    */
   GLushort MonoColor;
   GLushort ClearColor;

   /* DRI stuff
    */
   GLframebuffer *glBuffer;
   
   GLuint Fallback;
   GLuint needClip;

   /* These refer to the current draw (front vs. back) buffer:
    */
   int drawOffset;		/* draw buffer address in agp space */
   int drawX;			/* origin of drawable in draw buffer */
   int drawY;
   GLuint numClipRects;		/* cliprects for that buffer */
   XF86DRIClipRectPtr pClipRects;


   int lastSwap;
   int texAge;

   XF86DRIClipRectRec draw_rect;

   drmContext hHWContext;
   drmLock *driHwLock;
   int driFd;
   Display *display;

   __DRIdrawablePrivate *driDrawable;
   __DRIscreenPrivate *driScreen;
   i810ScreenPrivate *i810Screen; 
   I810SAREAPriv *sarea;
};


/* To remove all debugging, make sure I810_DEBUG is defined as a
 * preprocessor symbol, and equal to zero.  
 */
#define I810_DEBUG 0   
#ifndef I810_DEBUG
/*  #warning "Debugging enabled - expect reduced performance" */
extern int I810_DEBUG;
#endif

#define DEBUG_VERBOSE_2D     0x1
#define DEBUG_VERBOSE_RING   0x8
#define DEBUG_VERBOSE_OUTREG 0x10
#define DEBUG_ALWAYS_SYNC    0x40
#define DEBUG_VERBOSE_MSG    0x80
#define DEBUG_NO_OUTRING     0x100
#define DEBUG_NO_OUTREG      0x200
#define DEBUG_VERBOSE_API    0x400
#define DEBUG_VALIDATE_RING  0x800
#define DEBUG_VERBOSE_LRU    0x1000
#define DEBUG_VERBOSE_DRI    0x2000



extern GLuint i810DDRegisterPipelineStages( struct gl_pipeline_stage *out,
					   const struct gl_pipeline_stage *in,
					   GLuint nr );

extern GLboolean i810DDBuildPrecalcPipeline( GLcontext *ctx );



#endif
