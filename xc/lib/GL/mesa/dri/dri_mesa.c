/* $XFree86: xc/lib/GL/mesa/dri/dri_mesa.c,v 1.6 2000/03/02 16:07:33 martin Exp $ */
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
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 */

#ifdef GLX_DIRECT_RENDERING

#include <unistd.h>
#include <Xlibint.h>
#include <Xext.h>
#include <extutil.h>
#include "glxclient.h"
#include "GL/xmesa.h"
#include "xf86dri.h"
#include "sarea.h"
#include "dri_mesaint.h"
#include "dri_xmesaapi.h"


#if XMESA_MAJOR_VERSION != 3 || XMESA_MINOR_VERSION != 3
#error using wrong version of Mesa (need 3.3)
#endif


/* Context binding */
static Bool driMesaBindContext(Display *dpy, int scrn,
			       GLXDrawable draw, GLXContext gc);
static Bool driMesaUnbindContext(Display *dpy, int scrn,
				 GLXDrawable draw, GLXContext gc);

/* Drawable methods */
static void *driMesaCreateDrawable(Display *dpy, int scrn, GLXDrawable draw,
				   VisualID vid, __DRIdrawable *pdraw);
static __DRIdrawable *driMesaGetDrawable(Display *dpy, GLXDrawable draw);
static void driMesaSwapBuffers(Display *dpy, void *private);
static void driMesaDestroyDrawable(Display *dpy, void *private);

/* Context methods */
static void *driMesaCreateContext(Display *dpy, XVisualInfo *vis, void *shared,
				  __DRIcontext *pctx);
static void driMesaDestroyContext(Display *dpy, int scrn, void *private);

/* Screen methods */
static void *driMesaCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
				 int numConfigs, __GLXvisualConfig *config);
static void driMesaDestroyScreen(Display *dpy, int scrn, void *private);


/*****************************************************************/

/* Maintain a list of drawables */

void *drawHash = NULL; /* Hash table to hold DRI drawables */

static Bool __driMesaAddDrawable(__DRIdrawable *pdraw)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;

    /* Create the hash table */
    if (!drawHash) drawHash = drmHashCreate();

    if (drmHashInsert(drawHash, pdp->draw, pdraw))
	return GL_FALSE;

    return GL_TRUE;
}

static __DRIdrawable *__driMesaFindDrawable(GLXDrawable draw)
{
    int retcode;
    __DRIdrawable *pdraw;

    /* Create the hash table */
    if (!drawHash) drawHash = drmHashCreate();

    retcode = drmHashLookup(drawHash, draw, (void **)&pdraw);
    if (retcode)
	return NULL;

    return pdraw;
}

static void __driMesaRemoveDrawable(__DRIdrawable *pdraw)
{
    int retcode;
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;

    /* Create the hash table */
    if (!drawHash) drawHash = drmHashCreate();

    retcode = drmHashLookup(drawHash, pdp->draw, (void **)&pdraw);
    if (!retcode) { /* Found */
	drmHashDelete(drawHash, pdp->draw);
    }
}

/*****************************************************************/

static void driMesaInitAPI(__XMESAapi *XMesaAPI)
{
    XMesaAPI->InitDriver = XMesaInitDriver;
    XMesaAPI->ResetDriver = XMesaResetDriver;
    XMesaAPI->CreateVisual = XMesaCreateVisual;
    XMesaAPI->DestroyVisual = XMesaDestroyVisual;
    XMesaAPI->CreateContext = XMesaCreateContext;
    XMesaAPI->DestroyContext = XMesaDestroyContext;
    XMesaAPI->CreateWindowBuffer = XMesaCreateWindowBuffer;
    XMesaAPI->CreatePixmapBuffer = XMesaCreatePixmapBuffer;
    XMesaAPI->DestroyBuffer = XMesaDestroyBuffer;
    XMesaAPI->SwapBuffers = XMesaSwapBuffers;
    XMesaAPI->MakeCurrent = XMesaMakeCurrent;
    XMesaAPI->UnbindContext = XMesaUnbindContext;
}

/*****************************************************************/

static Bool driMesaUnbindContext(Display *dpy, int scrn,
				 GLXDrawable draw, GLXContext gc)
{
    __DRIdrawable *pdraw;
    __DRIcontextPrivate *pcp;
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driMesaUnbindContext.
    */

    if (gc == NULL || draw == None) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pdraw = __driMesaFindDrawable(draw);
    if (!pdraw) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pcp = (__DRIcontextPrivate *)gc->driContext.private;
    pdp = (__DRIdrawablePrivate *)pdraw->private;
    psp = pdp->driScreenPriv;
    if (!psp) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    /* Unbind Mesa's drawable from Mesa's context */
    (*psp->XMesaAPI.UnbindContext)(pcp->xm_ctx);

    if (pdp->refcount == 0) {
	/* ERROR!!! */
	return GL_FALSE;
    } else if (--pdp->refcount == 0) {
#if 0
	/*
	** NOT_DONE: When a drawable is unbound from one direct
	** rendering context and then bound to another, we do not want
	** to destroy the drawable data structure each time only to
	** recreate it immediatly afterwards when binding to the next
	** context.  This also causes conflicts with caching of the
	** drawable stamp.
	**
	** When GLX 1.3 is integrated, the create and destroy drawable
	** functions will have user level counterparts and the memory
	** will be able to be recovered.
	*/

	/* Delete drawable if no longer referenced by any contexts */
	(*pdraw->destroyDrawable)(dpy, pdraw->private);
	__driMesaRemoveDrawable(pdraw);
	Xfree(pdraw);
#endif
    }

    /* Unbind the drawable */
    pcp->driDrawablePriv = NULL;
    pdp->driContextPriv = &psp->dummyContextPriv;

    return GL_TRUE;
}

static Bool driMesaBindContext(Display *dpy, int scrn,
			       GLXDrawable draw, GLXContext gc)
{
    __DRIdrawable *pdraw;
    __DRIdrawablePrivate *pdp;
    __DRIscreenPrivate *psp;
    __DRIcontextPrivate *pcp;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driMesaBindContext.
    */

    if (gc == NULL || draw == None) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pdraw = __driMesaFindDrawable(draw);
    if (!pdraw) {
	/* Allocate a new drawable */
	pdraw = (__DRIdrawable *)Xmalloc(sizeof(__DRIdrawable));
	if (!pdraw) {
	    /* ERROR!!! */
	    return GL_FALSE;
	}

	/* Create a new drawable */
	pdraw->private = driMesaCreateDrawable(dpy, scrn, draw, gc->vid,
					       pdraw);
	if (!pdraw->private) {
	    /* ERROR!!! */
	    Xfree(pdraw);
	    return GL_FALSE;
	}

	/* Add pdraw to drawable list */
	if (!__driMesaAddDrawable(pdraw)) {
	    /* ERROR!!! */
	    (*pdraw->destroyDrawable)(dpy, pdraw->private);
	    Xfree(pdraw);
	    return GL_FALSE;
	}
    }

    pdp = (__DRIdrawablePrivate *)pdraw->private;
    psp = pdp->driScreenPriv;
    if (!psp) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    /* Bind the drawable to the context */
    pcp = (__DRIcontextPrivate *)gc->driContext.private;
    pcp->driDrawablePriv = pdp;
    pdp->driContextPriv = pcp;
    pdp->refcount++;

    /*
    ** Now that we have a context associated with this drawable, we can
    ** initialize the drawable information if has not been done before.
    */
    if (!pdp->pStamp) {
	DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	driMesaUpdateDrawableInfo(dpy, scrn, pdp);
	DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
    }

    /* Bind Mesa's drawable to Mesa's context */
    (*psp->XMesaAPI.MakeCurrent)(pcp->xm_ctx, pdp->xm_buf);

    return GL_TRUE;
}

/*****************************************************************/

void driMesaUpdateDrawableInfo(Display *dpy, int scrn,
			       __DRIdrawablePrivate *pdp)
{
    __DRIscreenPrivate *psp;
    __DRIcontextPrivate *pcp = pdp->driContextPriv;
    
    if (!pcp || (pdp != pcp->driDrawablePriv)) {
	/* ERROR!!! */
	return;
    }

    psp = pdp->driScreenPriv;
    if (!psp) {
	/* ERROR!!! */
	return;
    }

    if (pdp->pClipRects) {
	Xfree(pdp->pClipRects); 
    }

    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

    if (!XF86DRIGetDrawableInfo(dpy, scrn, pdp->draw,
				&pdp->index, &pdp->lastStamp,
				&pdp->x, &pdp->y, &pdp->w, &pdp->h,
				&pdp->numClipRects, &pdp->pClipRects)) {
	pdp->numClipRects = 0;
	pdp->pClipRects = NULL;
	/* ERROR!!! */
    }

    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

    pdp->pStamp = &(psp->pSAREA->drawableTable[pdp->index].stamp);
}

/*****************************************************************/

static void *driMesaCreateDrawable(Display *dpy, int scrn, GLXDrawable draw,
				   VisualID vid, __DRIdrawable *pdraw)
{
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;
    int i;
    XMesaVisual xm_vis = NULL;

    pdp = (__DRIdrawablePrivate *)Xmalloc(sizeof(__DRIdrawablePrivate));
    if (!pdp) {
	return NULL;
    }

    if (!XF86DRICreateDrawable(dpy, scrn, draw, &pdp->hHWDrawable)) {
	Xfree(pdp);
	return NULL;
    }

    pdp->draw = draw;
    pdp->refcount = 0;
    pdp->pStamp = NULL;
    pdp->lastStamp = 0;
    pdp->index = 0;
    pdp->x = 0;
    pdp->y = 0;
    pdp->w = 0;
    pdp->h = 0;
    pdp->numClipRects = 0;
    pdp->pClipRects = NULL;

    pDRIScreen = __glXFindDRIScreen(dpy, scrn);
    pdp->driScreenPriv = psp = (__DRIscreenPrivate *)pDRIScreen->private;

    pdp->driContextPriv = &psp->dummyContextPriv;

    for (i = 0; i < psp->numVisuals; i++) {
	if (vid == psp->visuals[i].vid) {
	    xm_vis = psp->visuals[i].xm_vis;
	    break;
	}
    }

    if (1 /* NOT_DONE: Determine if it is a pixmap or not */) {
	pdp->xm_buf = (*psp->XMesaAPI.CreateWindowBuffer)(xm_vis, draw, pdp);
    } else {
	XMesaVisual xm_vis = NULL;
	XMesaColormap cmap = 0;
	pdp->xm_buf = (*psp->XMesaAPI.CreatePixmapBuffer)(xm_vis, draw, cmap,
							  pdp);
    }
    if (!pdp->xm_buf) {
	(void)XF86DRIDestroyDrawable(dpy, scrn, pdp->draw);
	Xfree(pdp);
	return NULL;
    }

    pdraw->destroyDrawable = driMesaDestroyDrawable;
    pdraw->swapBuffers = driMesaSwapBuffers;

    return (void *)pdp;
}

static __DRIdrawable *driMesaGetDrawable(Display *dpy, GLXDrawable draw)
{
    /*
    ** Make sure this routine returns NULL if the drawable is not bound
    ** to a direct rendering context!
    */
    return __driMesaFindDrawable(draw);
}

static void driMesaSwapBuffers(Display *dpy, void *private)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)private;
    __DRIscreenPrivate *psp = pdp->driScreenPriv;

    (*psp->XMesaAPI.SwapBuffers)(pdp->xm_buf);
}

static void driMesaDestroyDrawable(Display *dpy, void *private)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)private;
    __DRIscreenPrivate *psp = pdp->driScreenPriv;
    int scrn = psp->myNum;

    if (pdp) {
	(*psp->XMesaAPI.DestroyBuffer)(pdp->xm_buf);
	(void)XF86DRIDestroyDrawable(dpy, scrn, pdp->draw);
	if (pdp->pClipRects)
	    Xfree(pdp->pClipRects);
	Xfree(pdp);
    }
}

/*****************************************************************/

static void *driMesaCreateContext(Display *dpy, XVisualInfo *vis, void *shared,
				  __DRIcontext *pctx)
{
    __DRIcontextPrivate *pcp;
    __DRIcontextPrivate *pshare = (__DRIcontextPrivate *)shared;
    XMesaContext shared_xm_ctx = (pshare ?
				  pshare->xm_ctx : (XMesaContext)NULL);
    __DRIscreenPrivate *psp;
    __DRIscreen *pDRIScreen;
    int i;

    pDRIScreen = __glXFindDRIScreen(dpy, vis->screen);
    psp = (__DRIscreenPrivate *)pDRIScreen->private;

    if (!psp->dummyContextPriv.driScreenPriv) {
	if (!XF86DRICreateContext(dpy, vis->screen, vis->visual,
				  &psp->dummyContextPriv.contextID,
				  &psp->dummyContextPriv.hHWContext)) {
	    return NULL;
	}
	psp->dummyContextPriv.driScreenPriv = psp;
	psp->dummyContextPriv.xm_ctx = NULL;
	psp->dummyContextPriv.driDrawablePriv = NULL;
	/* No other fields should be used! */
    }

    pcp = (__DRIcontextPrivate *)Xmalloc(sizeof(__DRIcontextPrivate));
    if (!pcp) {
	return NULL;
    }

    pcp->driScreenPriv = psp;
    pcp->xm_ctx = NULL;
    pcp->driDrawablePriv = NULL;

    if (!XF86DRICreateContext(dpy, vis->screen, vis->visual,
			      &pcp->contextID, &pcp->hHWContext)) {
	Xfree(pcp);
	return NULL;
    }

    for (i = 0; i < psp->numVisuals; i++)
	if (psp->visuals[i].vid == vis->visualid)
	    pcp->xm_ctx = (*psp->XMesaAPI.CreateContext)
		(psp->visuals[i].xm_vis, shared_xm_ctx, pcp);

    if (!pcp->xm_ctx) {
	(void)XF86DRIDestroyContext(dpy, vis->screen, pcp->contextID);
	Xfree(pcp);
	return NULL;
    }

    pctx->destroyContext = driMesaDestroyContext;
    pctx->bindContext    = driMesaBindContext;
    pctx->unbindContext  = driMesaUnbindContext;

    return pcp;
}

static void driMesaDestroyContext(Display *dpy, int scrn, void *private)
{
    __DRIcontextPrivate *pcp = (__DRIcontextPrivate *)private;

    if (pcp) {
	(void)XF86DRIDestroyContext(dpy, scrn, pcp->contextID);
	(*pcp->driScreenPriv->XMesaAPI.DestroyContext)(pcp->xm_ctx);
	Xfree(pcp);
    }
}

/*****************************************************************/

static void *driMesaCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
				 int numConfigs, __GLXvisualConfig *config)
{
    int directCapable, i, n;
    __DRIscreenPrivate *psp;
    XVisualInfo visTmpl, *visinfo;
    drmHandle hFB, hSAREA;
    char *BusID, *driverName;
    drmMagic magic;

    if (!XF86DRIQueryDirectRenderingCapable(dpy, scrn, &directCapable)) {
	return NULL;
    }

    if (!directCapable) {
	return NULL;
    }

    psp = (__DRIscreenPrivate *)Xmalloc(sizeof(__DRIscreenPrivate));
    if (!psp) {
	return NULL;
    }

    psp->myNum = scrn;

    if (!XF86DRIOpenConnection(dpy, scrn, &hSAREA, &BusID)) {
	Xfree(psp);
	return NULL;
    }

    /*
    ** NOT_DONE: This is used by the X server to detect when the client
    ** has died while holding the drawable lock.  The client sets the
    ** drawable lock to this value.
    */
    psp->drawLockID = 1;

    psp->fd = drmOpen(NULL,BusID);
    if (!psp->fd) {
	Xfree(BusID);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }
    Xfree(BusID); /* No longer needed */

    if (drmGetMagic(psp->fd, &magic)) {
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    if (!XF86DRIAuthConnection(dpy, scrn, magic)) {
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    if (!XF86DRIGetClientDriverName(dpy, scrn,
				    &psp->major,
				    &psp->minor,
				    &psp->patch,
				    &driverName)) {
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    driMesaInitAPI(&psp->XMesaAPI);

    if (!XF86DRIGetDeviceInfo(dpy, scrn,
			      &hFB,
			      &psp->fbOrigin,
			      &psp->fbSize,
			      &psp->fbStride,
			      &psp->devPrivSize,
			      &psp->pDevPriv)) {
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }
    psp->fbWidth = DisplayWidth(dpy, scrn);
    psp->fbHeight = DisplayHeight(dpy, scrn);
    psp->fbBPP = 32; /* NOT_DONE: Get this from X server */

    if (drmMap(psp->fd, hFB, psp->fbSize, (drmAddressPtr)&psp->pFB)) {
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    if (drmMap(psp->fd, hSAREA, SAREA_MAX, (drmAddressPtr)&psp->pSAREA)) {
	(void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    psp->numVisuals = numConfigs;
    psp->visuals = (__DRIvisualPrivate *)Xmalloc(numConfigs *
						 sizeof(__DRIvisualPrivate));
    if (!psp->visuals) {
	(void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	(void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    visTmpl.screen = scrn;
    visinfo = XGetVisualInfo(dpy, VisualScreenMask, &visTmpl, &n);
    if (n != numConfigs) {
	(void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	(void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    for (i = 0; i < numConfigs; i++, config++) {
	psp->visuals[i].vid = visinfo[i].visualid;
	psp->visuals[i].xm_vis =
	    (*psp->XMesaAPI.CreateVisual)(dpy,
					  &visinfo[i],
					  config->rgba,
					  (config->alphaSize > 0),
					  config->doubleBuffer,
					  config->stereo,
					  GL_TRUE, /* ximage_flag */
					  config->depthSize,
					  config->stencilSize,
					  config->accumRedSize,
					  config->level);
	if (!psp->visuals[i].xm_vis) {
	    /* Free the visuals so far created */
	    while (--i >= 0) {
		(*psp->XMesaAPI.DestroyVisual)(psp->visuals[i].xm_vis);
	    }
	    Xfree(psp->visuals);
	    XFree(visinfo);
	    (void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	    (void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	    Xfree(psp->pDevPriv);
	    (void)drmClose(psp->fd);
	    Xfree(psp);
	    (void)XF86DRICloseConnection(dpy, scrn);
	    return NULL;
	}
    }
    XFree(visinfo);

    /* Initialize the screen specific GLX driver */
    if (psp->XMesaAPI.InitDriver) {
	if (!(*psp->XMesaAPI.InitDriver)(psp)) {
	    while (--psp->numVisuals >= 0) {
		(*psp->XMesaAPI.DestroyVisual)
		    (psp->visuals[psp->numVisuals].xm_vis);
	    }
	    Xfree(psp->visuals);
	    (void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	    (void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	    Xfree(psp->pDevPriv);
	    (void)drmClose(psp->fd);
	    Xfree(psp);
	    (void)XF86DRICloseConnection(dpy, scrn);
	    return NULL;
	}
    }

    /*
    ** Do not init dummy context here; actual initialization will be
    ** done when the first DRI context is created.  Init screen priv ptr
    ** to NULL to let CreateContext routine that it needs to be inited.
    */
    psp->dummyContextPriv.driScreenPriv = NULL;

    psc->destroyScreen  = driMesaDestroyScreen;
    psc->createContext  = driMesaCreateContext;
    psc->createDrawable = driMesaCreateDrawable;
    psc->getDrawable    = driMesaGetDrawable;

    return (void *)psp;
}

static void driMesaDestroyScreen(Display *dpy, int scrn, void *private)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *)private;

    if (psp) {
	if (psp->dummyContextPriv.driScreenPriv) {
	    (void)XF86DRIDestroyContext(dpy, scrn,
					psp->dummyContextPriv.contextID);
	}
	if (psp->XMesaAPI.ResetDriver)
	    (*psp->XMesaAPI.ResetDriver)(psp);
	while (--psp->numVisuals >= 0) {
	    (*psp->XMesaAPI.DestroyVisual)
		(psp->visuals[psp->numVisuals].xm_vis);
	}
	Xfree(psp->visuals);
	(void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	(void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);

#if 0
	/*
	** NOT_DONE: Normally, we would call XF86DRICloseConnection()
	** here, but since this routine is called after the
	** XCloseDisplay() function has already shut down the connection
	** to the Display, there is no protocol stream open to the X
	** server anymore.  Luckily, XF86DRICloseConnection() does not
	** really do anything (for now).
	*/
	(void)XF86DRICloseConnection(dpy, scrn);
#endif
    }
}


/*
 * This is the entrypoint into the driver.
 * The driCreateScreen name is the symbol that libGL.so fetches.
 */
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
   return driMesaCreateScreen(dpy, scrn, psc, numConfigs, config);
}



#endif
