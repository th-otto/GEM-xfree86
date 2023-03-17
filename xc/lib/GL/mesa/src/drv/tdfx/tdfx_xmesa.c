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
#include <glide.h>
#include "tdfx_init.h"
#include "context.h"
#include "vbxform.h"
#include "matrix.h"

XMesaContext        nullCC  = 0;
XMesaContext        gCC     = 0;
tdfxContextPrivate *gCCPriv = 0;

static int count_bits(unsigned int n)
{
  int bits = 0;

  while (n > 0) {
    if (n & 1) bits++;
    n >>= 1;
  }
  return bits;
}

GLboolean XMesaInitDriver(__DRIscreenPrivate *driScrnPriv)
{
  tdfxScreenPrivate *gsp;

  /* Allocate the private area */
  gsp = (tdfxScreenPrivate *)Xmalloc(sizeof(tdfxScreenPrivate));
  if (!gsp) return GL_FALSE;

  gsp->driScrnPriv = driScrnPriv;

  driScrnPriv->private = (void *)gsp;

  if (!tdfxMapAllRegions(driScrnPriv)) {
    Xfree(driScrnPriv->private);
    return GL_FALSE;
  }

  return GL_TRUE;
}

void XMesaResetDriver(__DRIscreenPrivate *driScrnPriv)
{
  tdfxUnmapAllRegions(driScrnPriv);
  Xfree(driScrnPriv->private);
}

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

  /* Only RGB visuals are supported on the TDFX boards */
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
  XMesaContext c;
  tdfxContextPrivate *cPriv;
  __DRIscreenPrivate *driScrnPriv = driContextPriv->driScreenPriv;
  tdfxScreenPrivate *sPriv = (tdfxScreenPrivate *)driScrnPriv->private;
  TDFXSAREAPriv *saPriv;
  GLcontext *shareCtx;
  int **fifoPtr;

  c = (XMesaContext)Xmalloc(sizeof(struct xmesa_context));
  if (!c) return 0;

  c->driContextPriv = driContextPriv;
  c->xm_visual = v;
  c->xm_buffer = 0; /* Set by MakeCurrent */
  c->display = v->display;

  cPriv = (tdfxContextPrivate *)Xmalloc(sizeof(tdfxContextPrivate));
  if (!cPriv) {
    Xfree(c);
    return NULL;
  }

  cPriv->hHWContext = driContextPriv->hHWContext;
  cPriv->tdfxScrnPriv = sPriv;
  c->private = (void *)cPriv;

  cPriv->glVis=v->gl_visual;
  cPriv->glBuffer=gl_create_framebuffer(v->gl_visual,
                                        GL_FALSE,  /* software depth buffer? */
                                        v->gl_visual->StencilBits > 0,
                                        v->gl_visual->AccumBits > 0,
                                        v->gl_visual->AlphaBits > 0
                                        );

  cPriv->screen_width=sPriv->width;
  cPriv->screen_height=sPriv->height;

  cPriv->new_state = ~0;

  if (share_list)
    shareCtx=((tdfxContextPrivate*)(share_list->private))->glCtx;
  else
    shareCtx=0;
  cPriv->glCtx=gl_create_context(v->gl_visual, shareCtx, (void*)cPriv, GL_TRUE);
  cPriv->initDone=GL_FALSE;

  saPriv=(TDFXSAREAPriv*)((void*)driScrnPriv->pSAREA+sizeof(XF86DRISAREARec));
  grDRIOpen(driScrnPriv->pFB, sPriv->regs.map, sPriv->deviceID, 
	    sPriv->width, sPriv->height, sPriv->mem, sPriv->cpp, sPriv->stride,
	    sPriv->fifoOffset, sPriv->fifoSize, sPriv->fbOffset,
	    sPriv->backOffset, sPriv->depthOffset, sPriv->textureOffset, 
	    sPriv->textureSize, &saPriv->fifoPtr, &saPriv->fifoRead);

  return c;
}

void XMesaDestroyContext(XMesaContext c)
{
  tdfxContextPrivate *cPriv;

  cPriv=(tdfxContextPrivate*)c->private;
  if (cPriv) {
    gl_destroy_context(cPriv->glCtx);
    gl_destroy_framebuffer(cPriv->glBuffer);
  }
  if (c==gCC) {
    gCC=0;
    gCCPriv=0;
  }
}

XMesaBuffer XMesaCreateWindowBuffer(XMesaVisual v, XMesaWindow w,
				    __DRIdrawablePrivate *driDrawPriv)
{
  return (XMesaBuffer)1;
}

XMesaBuffer XMesaCreatePixmapBuffer(XMesaVisual v, XMesaPixmap p,
				    XMesaColormap c, __DRIdrawablePrivate *driDrawPriv)
{
  return (XMesaBuffer)1;
}

void XMesaDestroyBuffer(XMesaBuffer b)
{
}

void XMesaSwapBuffers(XMesaBuffer b)
{
  FxI32 result;
#ifdef STATS
  int stalls;
  extern int texSwaps;
  static int prevStalls=0;
#endif
  /*
  ** NOT_DONE: This assumes buffer is currently bound to a context.
  ** This needs to be able to swap buffers when not currently bound.
  */
  if (gCC == NULL || gCCPriv == NULL) return;

  FLUSH_VB( gCCPriv->glCtx, "swap buffers" );

  if (gCCPriv->haveDoubleBuffer) {
#ifdef STATS
    stalls=grFifoGetStalls();
    if (stalls!=prevStalls) {
      fprintf(stderr, "%d stalls occurred\n", stalls-prevStalls);
      prevStalls=stalls;
    }
    if (texSwaps) {
      fprintf(stderr, "%d texture swaps occurred\n", texSwaps);
      texSwaps=0;
    }
#endif
    FX_grDRIBufferSwap(gCCPriv->swapInterval);
    do {
      result=FX_grGetInteger(FX_PENDING_BUFFERSWAPS);
    } while (result>gCCPriv->maxPendingSwapBuffers);
    gCCPriv->stats.swapBuffer++;
  } else {
    fprintf(stderr, "No double buffer\n");
  }
}

GLboolean XMesaUnbindContext(XMesaContext c)
{
  if (c && c==gCC && gCCPriv) FX_grGlideGetState((GrState*)gCCPriv->state);
  return GL_TRUE;
}

GLboolean XMesaMakeCurrent(XMesaContext c, XMesaBuffer b)
{
  __DRIdrawablePrivate *driDrawPriv;

  if (c) {
    if (c==gCC) return GL_TRUE;
    gCC     = c;
    gCCPriv = (tdfxContextPrivate *)c->private;

    driDrawPriv = gCC->driContextPriv->driDrawablePriv;
    if (!gCCPriv->initDone) {
      if (!tdfxInitHW(c)) return GL_FALSE;
      gCCPriv->width=0;
      XMesaWindowMoved();
      FX_grGlideGetState((GrState*)gCCPriv->state);
    } else {
      FX_grSstSelect(gCCPriv->board);
      FX_grGlideSetState((GrState*)gCCPriv->state);
      XMesaWindowMoved();
    }

    gl_make_current(gCCPriv->glCtx, gCCPriv->glBuffer);
    fxSetupDDPointers(gCCPriv->glCtx);
    if (!gCCPriv->glCtx->Viewport.Width)
      gl_Viewport(gCCPriv->glCtx, 0, 0, driDrawPriv->w, driDrawPriv->h);
  } else {
    gl_make_current(0,0);
    gCC     = NULL;
    gCCPriv = NULL;
  }
  return GL_TRUE;
}

void
XMesaWindowMoved() {
  __DRIdrawablePrivate *dPriv = gCC->driContextPriv->driDrawablePriv;
  GLcontext *ctx;
  
  ctx=gCCPriv->glCtx;
  grDRIPosition(dPriv->x, dPriv->y, dPriv->w, dPriv->h,
		dPriv->numClipRects, dPriv->pClipRects);
  gCCPriv->numClipRects=dPriv->numClipRects;
  gCCPriv->pClipRects=dPriv->pClipRects;
  if (dPriv->x!=gCCPriv->x_offset || dPriv->y!=gCCPriv->y_offset ||
      dPriv->w!=gCCPriv->width || dPriv->h!=gCCPriv->height) {
    gCCPriv->x_offset=dPriv->x;
    gCCPriv->y_offset=dPriv->y;
    gCCPriv->width=dPriv->w;
    gCCPriv->height=dPriv->h;
    gCCPriv->y_delta=gCCPriv->screen_height-gCCPriv->y_offset-gCCPriv->height;
  }
  switch (dPriv->numClipRects) {
  case 0:
    gCCPriv->clipMinX=dPriv->x;
    gCCPriv->clipMaxX=dPriv->x+dPriv->w;
    gCCPriv->clipMinY=dPriv->y;
    gCCPriv->clipMaxY=dPriv->y+dPriv->h;
    fxSetScissorValues(ctx);
    gCCPriv->needClip=0;
    break;
  case 1:
    gCCPriv->clipMinX=dPriv->pClipRects[0].x1;
    gCCPriv->clipMaxX=dPriv->pClipRects[0].x2;
    gCCPriv->clipMinY=dPriv->pClipRects[0].y1;
    gCCPriv->clipMaxY=dPriv->pClipRects[0].y2;
    fxSetScissorValues(ctx);
    gCCPriv->needClip=0;
    break;
  default:
    gCCPriv->needClip=1;
  }
}

/* This is called from within the LOCK_HARDWARE routine */
void XMesaUpdateState(int windowMoved) {
  __DRIdrawablePrivate *dPriv = gCC->driContextPriv->driDrawablePriv;
  __DRIscreenPrivate *sPriv = dPriv->driScreenPriv;
  TDFXSAREAPriv *saPriv=(TDFXSAREAPriv*)(((char*)sPriv->pSAREA)+sizeof(XF86DRISAREARec));

  /* fprintf(stderr, "In FifoPtr=%d FifoRead=%d\n", saPriv->fifoPtr, saPriv->fifoRead); */
  if (saPriv->fifoOwner!=dPriv->driContextPriv->hHWContext) {
    grDRIImportFifo(saPriv->fifoPtr, saPriv->fifoRead);
  }
  if (saPriv->ctxOwner!=dPriv->driContextPriv->hHWContext) {
    /* This sequence looks a little odd. Glide mirrors the state, and
       when you get the state you are forcing the mirror to be up to
       date, and then getting a copy from the mirror. You can then force
       that state onto the hardware when you set the state. */
    void *state;
    state=malloc(FX_grGetInteger_NoLock(FX_GLIDE_STATE_SIZE));
    FX_grGlideGetState_NoLock(state);
    FX_grGlideSetState_NoLock(state);
    free(state);
  }
  if (saPriv->texOwner!=dPriv->driContextPriv->hHWContext) {
    fxTMRestoreTextures_NoLock(gCCPriv);
  }
  if (windowMoved)
    XMesaWindowMoved();
}

void XMesaSetSAREA() {
  __DRIdrawablePrivate *dPriv = gCC->driContextPriv->driDrawablePriv;
  __DRIscreenPrivate *sPriv = dPriv->driScreenPriv;
  TDFXSAREAPriv *saPriv=(TDFXSAREAPriv*)(((char*)sPriv->pSAREA)+sizeof(XF86DRISAREARec));

  saPriv->fifoOwner=dPriv->driContextPriv->hHWContext;
  saPriv->ctxOwner=dPriv->driContextPriv->hHWContext;
  saPriv->texOwner=dPriv->driContextPriv->hHWContext;
  grDRIResetSAREA();
  /* fprintf(stderr, "Out FifoPtr=%d FifoRead=%d\n", saPriv->fifoPtr, saPriv->fifoRead); */
}


extern void __driRegisterExtensions(void); /* silence compiler warning */

/* This function is called by libGL.so as soon as libGL.so is loaded.
 * This is where we'd register new extension functions with the dispatcher.
 */
void __driRegisterExtensions(void)
{
#if 0
   /* Example.  Also look in fxdd.c for more details. */
   {
      const int _gloffset_FooBarEXT = 555;  /* just an example number! */
      if (_glapi_add_entrypoint("glFooBarEXT", _gloffset_FooBarEXT)) {
         void *f = glXGetProcAddressARB("glFooBarEXT");
         assert(f);
      }
   }
#endif
}


#endif
