/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keithw@precisioninsight.com>
 *
 */

#ifdef GLX_DIRECT_RENDERING

#include <X11/Xlibint.h>
#include <stdio.h>

#include "i810_init.h"
#include "context.h"
#include "vbxform.h"
#include "matrix.h"
#include "simple_list.h"

#include "i810dd.h"
#include "i810state.h"
#include "i810tex.h"
#include "i810span.h"
#include "i810depth.h"
#include "i810tris.h"
#include "i810swap.h"
#include "i810pipeline.h"

#include "i810_dri.h"



#ifndef I810_DEBUG
int I810_DEBUG = (0
   		  | DEBUG_ALWAYS_SYNC 
		  | DEBUG_VERBOSE_RING   
		  | DEBUG_VERBOSE_OUTREG 
/*  		  | DEBUG_VERBOSE_MSG */
/*  		  | DEBUG_NO_OUTRING */
/*  		  | DEBUG_NO_OUTREG */
/*  		  | DEBUG_VERBOSE_API */
/*  		  | DEBUG_VERBOSE_2D */
/*  		  | DEBUG_VERBOSE_DRI */
		  | DEBUG_VALIDATE_RING
		  );
#endif


static i810ContextPtr      i810Ctx = 0;

i810Glx_t	i810glx;

static int count_bits(unsigned int n)
{
   int bits = 0;

   while (n > 0) {
      if (n & 1) bits++;
      n >>= 1;
   }
   return bits;
}

/* These functions are accessed by dlsym from dri_mesa_init.c:
 *
 * XMesaInitDriver
 * XMesaResetDriver
 * XMesaCreateVisual
 * XMesaDestroyVisual
 * XMesaCreateContext 
 * XMesaDestroyContext
 * XMesaCreateWindowBuffer
 * XMesaCreatePixmapBuffer
 * XMesaDestroyBuffer
 * XMesaSwapBuffers
 * XMesaMakeCurrent
 *
 * So this is kind of the public interface to the driver.  The driver
 * uses the X11 mesa driver context as a kind of wrapper around its
 * own driver context - but there isn't much justificiation for doing
 * it that way - the DRI might as well use a (void *) to refer to the
 * driver contexts.  Nothing in the X context really gets used.
 */


GLboolean XMesaInitDriver(__DRIscreenPrivate *sPriv)
{
   i810ScreenPrivate *i810Screen;
   I810DRIPtr         gDRIPriv = (I810DRIPtr)sPriv->pDevPriv;

   /* Allocate the private area */
   i810Screen = (i810ScreenPrivate *)Xmalloc(sizeof(i810ScreenPrivate));
   if (!i810Screen) return GL_FALSE;

   i810Screen->driScrnPriv = sPriv;
   sPriv->private = (void *)i810Screen;

   i810Screen->regs.handle=gDRIPriv->regs;
   i810Screen->regs.size=gDRIPriv->regsSize;
   i810Screen->deviceID=gDRIPriv->deviceID;
   i810Screen->width=gDRIPriv->width;
   i810Screen->height=gDRIPriv->height;
   i810Screen->mem=gDRIPriv->mem;
   i810Screen->cpp=gDRIPriv->cpp;
   i810Screen->fbStride=gDRIPriv->fbStride;
   i810Screen->fbOffset=gDRIPriv->fbOffset;

   if (gDRIPriv->bitsPerPixel == 15) 
      i810Screen->fbFormat = DV_PF_555;
   else
      i810Screen->fbFormat = DV_PF_565;

   i810Screen->backOffset=gDRIPriv->backOffset; 
   i810Screen->depthOffset=gDRIPriv->depthOffset;
   i810Screen->auxPitch = gDRIPriv->auxPitch;
   i810Screen->auxPitchBits = gDRIPriv->auxPitchBits;
   i810Screen->textureOffset=gDRIPriv->textureOffset;
   i810Screen->textureSize=gDRIPriv->textureSize;
   i810Screen->logTextureGranularity = gDRIPriv->logTextureGranularity;


   if (0)
      fprintf(stderr, "Tex heap size %x, granularity %x bytes\n",
	      i810Screen->textureSize, 1<<(i810Screen->logTextureGranularity));
    
   if (drmMap(sPriv->fd, 
	      i810Screen->regs.handle, 
	      i810Screen->regs.size, 
	      &i810Screen->regs.map) != 0) 
   {
      Xfree(i810Screen);
      return GL_FALSE;
   }

   /* Ditch i810glx in favor of i810Screen?
    */
   memset(&i810glx, 0, sizeof(i810glx));

   i810glx.LpRing.mem.Start = gDRIPriv->ringOffset;
   i810glx.LpRing.mem.Size = gDRIPriv->ringSize;
   i810glx.LpRing.mem.End = gDRIPriv->ringOffset + gDRIPriv->ringSize;
   i810glx.LpRing.virtual_start = sPriv->pFB + i810glx.LpRing.mem.Start;
   i810glx.LpRing.tail_mask = i810glx.LpRing.mem.Size - 1;
   i810glx.MMIOBase = i810Screen->regs.map;
   i810glx.texVirtual = sPriv->pFB + i810Screen->textureOffset;

   
   i810DDFastPathInit();
   i810DDTrifuncInit();
   i810DDSetupInit();

   return GL_TRUE;
}

/* Accessed by dlsym from dri_mesa_init.c
 */
void XMesaResetDriver(__DRIscreenPrivate *sPriv)
{
   i810ScreenPrivate *i810Screen = (i810ScreenPrivate *)sPriv->private;

   drmUnmap(i810Screen->regs.map, i810Screen->regs.size);
   Xfree(i810Screen);
}

/* Accessed by dlsym from dri_mesa_init.c
 */
XMesaVisual XMesaCreateVisual(XMesaDisplay *display,
			      XMesaVisualInfo visinfo,
			      GLboolean rgb_flag,
			      GLboolean alpha_flag,
			      GLboolean db_flag,
			      GLboolean stereo_flag,
			      GLboolean ximage_flag,
			      GLint depth_size,
			      GLint stencil_size,
			      GLint accum_size,
			      GLint level)
{
   XMesaVisual v;

   /* Only RGB visuals are supported on the I810 boards */
   if (!rgb_flag) return 0;

   v = (XMesaVisual)Xmalloc(sizeof(struct xmesa_visual));
   if (!v) return 0;

   v->visinfo = (XVisualInfo *)Xmalloc(sizeof(*visinfo));
   if(!v->visinfo) {
      Xfree(v);
      return 0;
   }
   memcpy(v->visinfo, visinfo, sizeof(*visinfo));

   v->display = display;
   v->level = level;
  
   v->gl_visual = (GLvisual *)Xmalloc(sizeof(GLvisual));
   if (!v->gl_visual) {
      Xfree(v->visinfo);
      XFree(v);
      return 0;
   }

   v->gl_visual->RGBAflag   = rgb_flag;
   v->gl_visual->DBflag     = db_flag;
   v->gl_visual->StereoFlag = stereo_flag;

   v->gl_visual->RedBits   = count_bits(visinfo->red_mask);
   v->gl_visual->GreenBits = count_bits(visinfo->green_mask);
   v->gl_visual->BlueBits  = count_bits(visinfo->blue_mask);
   v->gl_visual->AlphaBits = 0; /* Not currently supported */

   v->gl_visual->AccumBits   = accum_size;
   v->gl_visual->DepthBits   = depth_size;
   v->gl_visual->StencilBits = stencil_size;

   return v;
}

void XMesaDestroyVisual(XMesaVisual v)
{
   Xfree(v->gl_visual);
   Xfree(v->visinfo);
   Xfree(v);
}

XMesaContext XMesaCreateContext(XMesaVisual v, XMesaContext share_list,
				__DRIcontextPrivate *driContextPriv)
{
   GLcontext *ctx;
   XMesaContext c;
   i810ContextPtr imesa;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   i810ScreenPrivate *i810Screen = (i810ScreenPrivate *)sPriv->private;
   I810SAREAPriv *saPriv=(I810SAREAPriv*)(((char*)sPriv->pSAREA)+
					  sizeof(XF86DRISAREARec));

   GLcontext *shareCtx = 0;

   c = (XMesaContext)Xmalloc(sizeof(struct xmesa_context));
   if (!c) {
      return 0;
   }

   imesa = (i810ContextPtr)Xmalloc(sizeof(i810Context));
   if (!imesa) {
      Xfree(c);
      return 0;
   }

   c->driContextPriv = driContextPriv;
   c->xm_visual = v;
   c->xm_buffer = 0; /* Set by MakeCurrent */
   c->display = v->display;
   c->private = (void *)imesa;

   if (share_list)
      shareCtx=((i810ContextPtr)(share_list->private))->glCtx;

   ctx = imesa->glCtx = gl_create_context(v->gl_visual, shareCtx, 
					  (void*)imesa, GL_TRUE);

   /* Dri stuff
    */
   imesa->display = v->display;
   imesa->hHWContext = driContextPriv->hHWContext;
   imesa->driFd = sPriv->fd;
   imesa->driHwLock = &sPriv->pSAREA->lock;

   imesa->i810Screen = i810Screen;
   imesa->driScreen = sPriv;
   imesa->sarea = saPriv;

   imesa->glBuffer = gl_create_framebuffer(v->gl_visual);

   imesa->needClip=1;

   imesa->texHeap = mmInit( 0, i810Screen->textureSize );


   /* Utah stuff
    */
   imesa->renderindex = -1;		/* impossible value */
   imesa->new_state = ~0;
   imesa->dirty = ~0;

   make_empty_list(&imesa->TexObjList);
   make_empty_list(&imesa->SwappedOut);
   
   imesa->TextureMode = ctx->Texture.Unit[0].EnvMode;
   imesa->CurrentTexObj[0] = 0;
   imesa->CurrentTexObj[1] = 0;
   
   i810DDExtensionsInit( ctx );

   i810DDInitStateFuncs( ctx );
   i810DDInitTextureFuncs( ctx );
   i810DDInitSpanFuncs( ctx );
   i810DDInitDepthFuncs( ctx );
   i810DDInitDriverFuncs( ctx );

   ctx->Driver.TriangleCaps = (DD_TRI_CULL|
			       DD_TRI_LIGHT_TWOSIDE|
			       DD_TRI_OFFSET);

   /* Ask mesa to clip fog coordinates for us.
    */
   ctx->TriangleCaps |= DD_CLIP_FOG_COORD;

   ctx->Shared->DefaultD[2][0].DriverData = 0;
   ctx->Shared->DefaultD[2][1].DriverData = 0;

   if (ctx->VB) 
      i810DDRegisterVB( ctx->VB );

   if (ctx->NrPipelineStages)
      ctx->NrPipelineStages =
	 i810DDRegisterPipelineStages(ctx->PipelineStage,
				      ctx->PipelineStage,
				      ctx->NrPipelineStages);

   i810DDInitState( imesa );

   return c;
}

void XMesaDestroyContext(XMesaContext c)
{
   i810ContextPtr imesa = (i810ContextPtr) c->private;

   if (imesa) {
      i810TextureObjectPtr next_t, t;

      gl_destroy_context(imesa->glCtx);
      gl_destroy_framebuffer(imesa->glBuffer);

      foreach_s (t, next_t, &(imesa->TexObjList))
	 i810DestroyTexObj(imesa, t);

      foreach_s (t, next_t, &(imesa->SwappedOut))
	 i810DestroyTexObj(imesa, t);

      Xfree(imesa);

      c->private = 0;
   }
}

XMesaBuffer XMesaCreateWindowBuffer(XMesaVisual v, XMesaWindow w,
				    __DRIdrawablePrivate *driDrawPriv)
{
  return (XMesaBuffer)1;
}

XMesaBuffer XMesaCreatePixmapBuffer(XMesaVisual v, XMesaPixmap p,
				    XMesaColormap c, 
				    __DRIdrawablePrivate *driDrawPriv)
{
  return (XMesaBuffer)1;
}

void XMesaDestroyBuffer(XMesaBuffer b)
{
}

void XMesaSwapBuffers(XMesaBuffer bogus)
{
   i810ContextPtr imesa = i810Ctx; 

   FLUSH_VB( imesa->glCtx, "swap buffers" );
   i810SwapBuffers(imesa);
}

static void i810InitClipRects( i810ContextPtr imesa )
{
   switch (imesa->numClipRects) {
   case 0:
      imesa->ClipSetup[I810_CLIPREG_SC] = ( GFX_OP_SCISSOR |
					    SC_UPDATE_SCISSOR );
      imesa->ClipSetup[I810_CLIPREG_SCI1] = 0;
      imesa->ClipSetup[I810_CLIPREG_SCI2] = 0;
      imesa->needClip = 0;
      imesa->dirty |= I810_EMIT_CLIPRECT;
      break;
   case 1:
      imesa->needClip = 0;
      i810EmitScissorValues( imesa, 0, 0 );
      imesa->dirty |= I810_EMIT_CLIPRECT;
      break;
   default:
      imesa->needClip=1;
      break;
   }
}



void i810XMesaSetFrontClipRects( i810ContextPtr imesa )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;

   imesa->numClipRects = dPriv->numClipRects;
   imesa->pClipRects = dPriv->pClipRects;
   imesa->drawX = dPriv->x;
   imesa->drawY = dPriv->y;

   imesa->drawOffset = imesa->i810Screen->fbOffset;
   i810EmitDrawingRectangle( imesa );
   i810InitClipRects( imesa );
}


void i810XMesaSetBackClipRects( i810ContextPtr imesa )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   int i;

   if (dPriv->numAuxClipRects == 0) 
   {
      if (I810_DEBUG & DEBUG_VERBOSE_DRI)
	 fprintf(stderr, "FRONT_CLIPRECTS, %d rects\n", 
		 dPriv->numClipRects);

      imesa->numClipRects = dPriv->numClipRects;
      imesa->pClipRects = dPriv->pClipRects;
      imesa->drawX = dPriv->x;
      imesa->drawY = dPriv->y;
   } else {
      if (I810_DEBUG & DEBUG_VERBOSE_DRI)
	 fprintf(stderr, "AUX_RECTS, %d rects\n", 
		 dPriv->numAuxClipRects);

      imesa->numClipRects = dPriv->numAuxClipRects;
      imesa->pClipRects = dPriv->pAuxClipRects;
      imesa->drawX = dPriv->auxX;
      imesa->drawY = dPriv->auxY;
   }

   imesa->drawOffset = imesa->i810Screen->backOffset;
   i810EmitDrawingRectangle( imesa );
   i810InitClipRects( imesa );

   if (I810_DEBUG & DEBUG_VERBOSE_DRI)
      for (i = 0 ; i < imesa->numClipRects ; i++) 
	 fprintf(stderr, "cliprect %d: %d,%d - %d,%d\n",
		 i,
		 imesa->pClipRects[i].x1,
		 imesa->pClipRects[i].y1,
		 imesa->pClipRects[i].x2,
		 imesa->pClipRects[i].y2);
}


static void i810XMesaWindowMoved( i810ContextPtr imesa ) 
{
   switch (imesa->glCtx->Color.DriverDrawBuffer) {
   case GL_FRONT_LEFT:
      i810XMesaSetFrontClipRects( imesa );
      break;
   case GL_BACK_LEFT:
      i810XMesaSetBackClipRects( imesa );
      break;
   default:
      fprintf(stderr, "fallback buffer\n");
      break;
   }

}


/* This looks buggy to me - the 'b' variable isn't used anywhere...
 * Hmm - It seems that the drawable is already hooked in to
 * driDrawablePriv.  
 */
GLboolean XMesaMakeCurrent(XMesaContext c, XMesaBuffer b)
{

   if (c->private==(void *)i810Ctx) return GL_TRUE;

   if (c) {
      __DRIdrawablePrivate *dPriv = c->driContextPriv->driDrawablePriv;

      i810Ctx = (i810ContextPtr)c->private;

      gl_make_current(i810Ctx->glCtx, i810Ctx->glBuffer);

      i810Ctx->driDrawable = dPriv;
      i810Ctx->dirty = ~0;
   
      i810XMesaWindowMoved( i810Ctx );

      if (!i810Ctx->glCtx->Viewport.Width)
	 gl_Viewport(i810Ctx->glCtx, 0, 0, dPriv->w, dPriv->h);

   } else {
      gl_make_current(0,0);
      i810Ctx = NULL;
   }
   return GL_TRUE;
}


void i810XMesaUpdateState( i810ContextPtr imesa ) 
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   __DRIscreenPrivate *sPriv = imesa->driScreen;
   I810SAREAPriv *sarea = imesa->sarea;
   int me = imesa->hHWContext;
   int stamp = dPriv->lastStamp; 


   /* If the window moved, may need to set a new cliprect now.
    *
    * NOTE: This releases and regains the hw lock, so all state
    * checking must be done *after* this call:
    */
   XMESA_VALIDATE_DRAWABLE_INFO(imesa->display, sPriv, dPriv);		

   i810glx.LpRing.synced = 0;

   /* If another client has touched the ringbuffer, need to update 
    * where we think the pointers are:
    */
   if (sarea->ringOwner != me) {
      i810glx.c_ringlost++;
      imesa->dirty |= I810_REFRESH_RING;
   }
      
   /* If we lost context, need to dump all registers to hardware.
    * Note that we don't care about 2d contexts, even if they perform
    * accelerated commands, so the DRI locking in the X server is even
    * more broken than usual.
    */
   if (sarea->ctxOwner != me) {
      i810glx.c_ctxlost++;
      imesa->dirty |= I810_UPLOAD_CTX;
      imesa->dirty |= I810_EMIT_CLIPRECT;
      imesa->dirty |= I810_UPLOAD_BUFFERS;
   }

   /* Shared texture managment - if another client has played with
    * texture space, figure out which if any of our textures have been
    * ejected, and update our global LRU.
    */
   if (sarea->texAge != imesa->texAge) {
      int sz = 1 << (imesa->i810Screen->logTextureGranularity);
      int idx, nr = 0;

      /* Have to go right round from the back to ensure stuff ends up
       * LRU in our local list...
       */
      for (idx = sarea->texList[I810_NR_TEX_REGIONS].prev ; 
	   idx != I810_NR_TEX_REGIONS && nr < I810_NR_TEX_REGIONS ; 
	   idx = sarea->texList[idx].prev, nr++)
      {
	 if (sarea->texList[idx].age > imesa->texAge)
	    i810TexturesGone(imesa, idx * sz, sz, sarea->texList[idx].in_use);
      }

      if (nr == I810_NR_TEX_REGIONS) {
	 i810TexturesGone(imesa, 0, imesa->i810Screen->textureSize, 0);
	 i810ResetGlobalLRU( imesa );
      }

      imesa->texAge = sarea->texAge;
      imesa->dirty |= I810_UPLOAD_TEX0IMAGE | I810_UPLOAD_TEX1IMAGE;
   }

   
   if (dPriv->lastStamp != stamp)
      i810XMesaWindowMoved( imesa );

   sarea->ctxOwner=me;
   sarea->ringOwner=me;


   _I810RefreshLpRing(imesa, 0);  

   if (imesa->dirty) 
       i810EmitHwStateLocked( imesa );  

}


#endif
