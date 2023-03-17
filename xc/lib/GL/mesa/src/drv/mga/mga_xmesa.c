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
 *   Daryll Strauss <daryll@precisioninsight.com>
 *
 */

#ifdef GLX_DIRECT_RENDERING

#include <X11/Xlibint.h>
#include <stdio.h>

#include "mga_xmesa.h"
#include "context.h"
#include "vbxform.h"
#include "matrix.h"
#include "simple_list.h"

#include "mgadd.h"
#include "mgastate.h"
#include "mgatex.h"
#include "mgaspan.h"
#include "mgadepth.h"
#include "mgatris.h"
#include "mgapipeline.h"

#include "xf86dri.h"
#include "mga_dri.h"
#include "mga_drm_public.h"
#include "mga_xmesa.h"





#ifndef MGA_DEBUG
int MGA_DEBUG = (0
/*  		  | MGA_DEBUG_ALWAYS_SYNC */
/*  		  | MGA_DEBUG_VERBOSE_MSG */
		  );
#endif


static mgaContextPtr      mgaCtx = 0;

mgaGlx_t	mgaglx;

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
   mgaScreenPrivate *mgaScreen;
   MGADRIPtr         gDRIPriv = (MGADRIPtr)sPriv->pDevPriv;

   /* Allocate the private area */
   mgaScreen = (mgaScreenPrivate *)Xmalloc(sizeof(mgaScreenPrivate));
   if (!mgaScreen) return GL_FALSE;

   mgaScreen->sPriv = sPriv;
   sPriv->private = (void *)mgaScreen;

   mgaScreen->chipset=gDRIPriv->chipset;
   mgaScreen->width=gDRIPriv->width;
   mgaScreen->height=gDRIPriv->height;
   mgaScreen->mem=gDRIPriv->mem;
   mgaScreen->cpp=gDRIPriv->cpp;
   mgaScreen->frontPitch=gDRIPriv->frontPitch;
   mgaScreen->frontOffset=gDRIPriv->frontOffset;
   mgaScreen->backOffset=gDRIPriv->backOffset; 
   mgaScreen->backPitch = gDRIPriv->backPitch;
   mgaScreen->depthOffset=gDRIPriv->depthOffset;
   mgaScreen->depthPitch = gDRIPriv->depthPitch;
   mgaScreen->textureOffset=gDRIPriv->textureOffset;
   mgaScreen->textureSize=gDRIPriv->textureSize;
   mgaScreen->logTextureGranularity = gDRIPriv->logTextureGranularity;


   mgaScreen->bufs = drmMapBufs(sPriv->fd);


   /* Other mgaglx stuff, too??
    */
   memset(&mgaglx, 0, sizeof(mgaglx));

   mgaDDFastPathInit();
   mgaDDTrifuncInit();
   mgaDDSetupInit();

   return GL_TRUE;
}

/* Accessed by dlsym from dri_mesa_init.c
 */
void XMesaResetDriver(__DRIscreenPrivate *sPriv)
{
   Xfree(sPriv->private);
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

   /* Only RGB visuals are supported on the MGA boards */
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
   mgaContextPtr mmesa;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   mgaScreenPrivate *mgaScreen = (mgaScreenPrivate *)sPriv->private;
   drm_mga_sarea_t *saPriv=(drm_mga_sarea_t*)(((char*)sPriv->pSAREA)+
					      sizeof(XF86DRISAREARec));
   GLcontext *shareCtx = 0;

   c = (XMesaContext)Xmalloc(sizeof(struct xmesa_context));
   if (!c) {
      return 0;
   }

   mmesa = (mgaContextPtr)Xmalloc(sizeof(mgaContext));
   if (!mmesa) {
      Xfree(c);
      return 0;
   }

   c->driContextPriv = driContextPriv;
   c->xm_visual = v;
   c->xm_buffer = 0; /* Set by MakeCurrent */
   c->display = v->display;
   c->private = (void *)mmesa;

   if (share_list)
      shareCtx=((mgaContextPtr)(share_list->private))->glCtx;

   ctx = mmesa->glCtx = gl_create_context(v->gl_visual, shareCtx, 
					  (void*)mmesa, GL_TRUE);

   /* Dri stuff
    */
   mmesa->display = v->display;
   mmesa->hHWContext = driContextPriv->hHWContext;
   mmesa->driFd = sPriv->fd;
   mmesa->driHwLock = &sPriv->pSAREA->lock;

   mmesa->mgaScreen = mgaScreen;
   mmesa->driScreen = sPriv;
   mmesa->sarea = saPriv;

   mmesa->glBuffer=gl_create_framebuffer(v->gl_visual);


   mmesa->needClip=1;

   mmesa->texHeap = mmInit( 0, mgaScreen->textureSize );

   /* Utah stuff
    */
   mmesa->renderindex = -1;		/* impossible value */
   mmesa->new_state = ~0;
   mmesa->dirty = ~0;

   mmesa->warp_pipe = 0;


   make_empty_list(&mmesa->SwappedOut);
   make_empty_list(&mmesa->TexObjList);
   
   mmesa->CurrentTexObj[0] = 0;
   mmesa->CurrentTexObj[1] = 0;
   
   mgaDDExtensionsInit( ctx );

   mgaDDInitStateFuncs( ctx );
   mgaDDInitTextureFuncs( ctx );
   mgaDDInitSpanFuncs( ctx );
   mgaDDInitDepthFuncs( ctx );
   mgaDDInitDriverFuncs( ctx );
   mgaDDInitIoctlFuncs( ctx );


   ctx->Driver.TriangleCaps = (DD_TRI_CULL|
			       DD_TRI_LIGHT_TWOSIDE|
			       DD_TRI_OFFSET);

   /* Ask mesa to clip fog coordinates for us.
    */
   ctx->TriangleCaps |= DD_CLIP_FOG_COORD;

   ctx->Shared->DefaultD[2][0].DriverData = 0;
   ctx->Shared->DefaultD[2][1].DriverData = 0;

   if (ctx->VB) 
      mgaDDRegisterVB( ctx->VB );

   if (ctx->NrPipelineStages)
      ctx->NrPipelineStages =
	 mgaDDRegisterPipelineStages(ctx->PipelineStage,
				      ctx->PipelineStage,
				      ctx->NrPipelineStages);
   return c;
}

void XMesaDestroyContext(XMesaContext c)
{
   mgaContextPtr mmesa = (mgaContextPtr) c->private;

   if (mmesa) {
/*        mgaTextureObjectPtr next_t, t; */

      gl_destroy_context(mmesa->glCtx);
      gl_destroy_framebuffer(mmesa->glBuffer);

/*        foreach_s (t, next_t, &(mmesa->TexObjList)) */
/*  	 mgaDestroyTexObj(mmesa, t); */

/*        foreach_s (t, next_t, &(mmesa->SwappedOut)) */
/*  	 mgaDestroyTexObj(mmesa, t); */

      Xfree(mmesa);

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

void XMesaSwapBuffers(XMesaBuffer bogus_value_do_not_use)
{
   mgaContextPtr mmesa = mgaCtx;    
   FLUSH_VB( mmesa->glCtx, "swap buffers" );
   mgaSwapBuffers(mmesa);
}





void mgaXMesaSetFrontClipRects( mgaContextPtr mmesa )
{
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;

   mmesa->numClipRects = dPriv->numClipRects;
   mmesa->pClipRects = dPriv->pClipRects;
   mmesa->drawX = dPriv->x;
   mmesa->drawY = dPriv->y;

   mmesa->drawOffset = mmesa->mgaScreen->frontOffset;
}


void mgaXMesaSetBackClipRects( mgaContextPtr mmesa )
{
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;

   if (dPriv->numAuxClipRects == 0) 
   {
      mmesa->numClipRects = dPriv->numClipRects;
      mmesa->pClipRects = dPriv->pClipRects;
      mmesa->drawX = dPriv->x;
      mmesa->drawY = dPriv->y;
   } else {
      mmesa->numClipRects = dPriv->numAuxClipRects;
      mmesa->pClipRects = dPriv->pAuxClipRects;
      mmesa->drawX = dPriv->auxX;
      mmesa->drawY = dPriv->auxY;
   }

   mmesa->drawOffset = mmesa->mgaScreen->backOffset;
}


static void mgaXMesaWindowMoved( mgaContextPtr mmesa ) 
{
   /* Clear any contaminated CVA data.
    */
   mmesa->setupdone = 0;

   switch (mmesa->glCtx->Color.DriverDrawBuffer) {
   case GL_FRONT_LEFT:
      mgaXMesaSetFrontClipRects( mmesa );
      break;
   case GL_BACK_LEFT:
      mgaXMesaSetBackClipRects( mmesa );
      break;
   default:
      fprintf(stderr, "fallback buffer\n");
      break;
   }

}


/* This looks buggy to me - the 'b' variable isn't used anywhere...
 * Hmm - It seems that the drawable is already hooked in to
 * driDrawablePriv.  
 *
 * But why are we doing context initialization here???
 */
GLboolean XMesaMakeCurrent(XMesaContext c, XMesaBuffer b)
{

   if (c->private==(void *)mgaCtx) return GL_TRUE;

   if (c) {
      __DRIdrawablePrivate *dPriv = c->driContextPriv->driDrawablePriv;

      mgaCtx = (mgaContextPtr)c->private;


      gl_make_current(mgaCtx->glCtx, mgaCtx->glBuffer);

      mgaXMesaWindowMoved( mgaCtx );
      mgaCtx->driDrawable = dPriv;
      mgaCtx->dirty = ~0;

      if (!mgaCtx->glCtx->Viewport.Width)
	 gl_Viewport(mgaCtx->glCtx, 0, 0, dPriv->w, dPriv->h);

   } else {
      gl_make_current(0,0);
      mgaCtx = NULL;
   }

   return GL_TRUE;
}


void mgaXMesaUpdateState( mgaContextPtr mmesa ) 
{
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
   __DRIscreenPrivate *sPriv = mmesa->driScreen;
   drm_mga_sarea_t *sarea = mmesa->sarea;

   int me = mmesa->hHWContext;
   int stamp = dPriv->lastStamp; 

   /* If the window moved, may need to set a new cliprect now.
    *
    * NOTE: This releases and regains the hw lock, so all state
    * checking must be done *after* this call:
    */
   XMESA_VALIDATE_DRAWABLE_INFO(mmesa->display, sPriv, dPriv);		

   if (sarea->ctxOwner != me) {
      mmesa->dirty |= MGA_UPLOAD_CTX;
   }

   if (sarea->texAge != mmesa->texAge) {
      int sz = 1 << (mmesa->mgaScreen->logTextureGranularity);
      int idx, nr = 0;

      /* Have to go right round from the back to ensure stuff ends up
       * LRU in our local list...
       */
      for (idx = sarea->texList[MGA_NR_TEX_REGIONS].prev ; 
	   idx != MGA_NR_TEX_REGIONS && nr < MGA_NR_TEX_REGIONS ; 
	   idx = sarea->texList[idx].prev, nr++)
      {
	 if (sarea->texList[idx].age > mmesa->texAge)
	    mgaTexturesGone(mmesa, idx * sz, sz, 1);
      }

      if (nr == MGA_NR_TEX_REGIONS) {
	 mgaTexturesGone(mmesa, 0, mmesa->mgaScreen->textureSize, 0);
	 mgaResetGlobalLRU( mmesa );
      }

      mmesa->texAge = sarea->texAge;
      mmesa->dirty |= MGA_UPLOAD_TEX0IMAGE | MGA_UPLOAD_TEX1IMAGE;
   }

   if (dPriv->lastStamp != stamp) 
      mgaXMesaWindowMoved( mmesa );

   sarea->ctxOwner=me;
}






#endif
