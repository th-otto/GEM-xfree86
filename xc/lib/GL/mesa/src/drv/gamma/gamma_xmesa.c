/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_xmesa.c,v 1.4 2000/03/02 16:07:36 martin Exp $ */
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

#include <X11/Xlibint.h>
#include "gamma_init.h"
#include "glapi.h"


XMesaContext         nullCC  = NULL;
XMesaContext         gCC     = NULL;
gammaContextPrivate *gCCPriv = NULL;

static struct _glapi_table *Dispatch = NULL;

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
    gammaScreenPrivate *gsp;

    /* Allocate the private area */
    gsp = (gammaScreenPrivate *)Xmalloc(sizeof(gammaScreenPrivate));
    if (!gsp) {
	return GL_FALSE;
    }
    gsp->driScrnPriv = driScrnPriv;

    driScrnPriv->private = (void *)gsp;

    if (!gammaMapAllRegions(driScrnPriv)) {
	Xfree(driScrnPriv->private);
	return GL_FALSE;
    }

    return GL_TRUE;
}

void XMesaResetDriver(__DRIscreenPrivate *driScrnPriv)
{
    gammaUnmapAllRegions(driScrnPriv);
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

    /* Only RGB visuals are supported on the GMX2000 */
    if (!rgb_flag) {
	return NULL;
    }

    v = (XMesaVisual)Xmalloc(sizeof(struct xmesa_visual));
    if (!v) {
	return NULL;
    }

    v->visinfo = (XVisualInfo *)Xmalloc(sizeof(*visinfo));
    if(!v->visinfo) {
	Xfree(v);
	return NULL;
    }
    memcpy(v->visinfo, visinfo, sizeof(*visinfo));

    v->display = display;
    v->level = level;

    v->gl_visual = (GLvisual *)Xmalloc(sizeof(GLvisual));
    if (!v->gl_visual) {
	Xfree(v->visinfo);
	XFree(v);
	return NULL;
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
    int i;
    XMesaContext c;
    gammaContextPrivate *cPriv;
    __DRIscreenPrivate *driScrnPriv = driContextPriv->driScreenPriv;
    gammaScreenPrivate *gPriv = (gammaScreenPrivate *)driScrnPriv->private;

    if (!Dispatch) {
       GLuint size = _glapi_get_dispatch_table_size() * sizeof(GLvoid *);
       Dispatch = (struct _glapi_table *) malloc(size);
       _gamma_init_dispatch(Dispatch);
    }

    c = (XMesaContext)Xmalloc(sizeof(struct xmesa_context));
    if (!c) {
	return NULL;
    }

    c->driContextPriv = driContextPriv;
    c->xm_visual = v;
    c->xm_buffer = NULL; /* Set by MakeCurrent */
    c->display = v->display;

    cPriv = (gammaContextPrivate *)Xmalloc(sizeof(gammaContextPrivate));
    if (!cPriv) {
	Xfree(c);
	return NULL;
    }

    cPriv->hHWContext = driContextPriv->hHWContext;
    GET_FIRST_DMA(driScrnPriv->fd, cPriv->hHWContext,
		  1, &cPriv->bufIndex, &cPriv->bufSize,
		  &cPriv->buf, &cPriv->bufCount, gPriv);

#ifdef DO_VALIDATE
    GET_FIRST_DMA(driScrnPriv->fd, cPriv->hHWContext,
		  1, &cPriv->WCbufIndex, &cPriv->WCbufSize,
		  &cPriv->WCbuf, &cPriv->WCbufCount, gPriv);
#endif

    cPriv->ClearColor[0] = 0.0;
    cPriv->ClearColor[1] = 0.0;
    cPriv->ClearColor[2] = 0.0;
    cPriv->ClearColor[3] = 1.0;
    cPriv->ClearDepth = 1.0;
    cPriv->x = 0;
    cPriv->y = 0;
    cPriv->w = 0;
    cPriv->h = 0;
    cPriv->FrameCount = 0;
    cPriv->MatrixMode = GL_MODELVIEW;
    cPriv->ModelViewCount = 0;
    cPriv->ProjCount = 0;
    cPriv->TextureCount = 0;

    for (i = 0; i < 16; i++)
	if (i % 5 == 0)
	    cPriv->ModelView[i] =
		cPriv->Proj[i] =
		cPriv->ModelViewProj[i] =
		cPriv->Texture[i] = 1.0;
	else
	    cPriv->ModelView[i] =
		cPriv->Proj[i] =
		cPriv->ModelViewProj[i] =
		cPriv->Texture[i] = 0.0;

    /*
    ** NOT_DONE: The 0xe4 in LBReadMode and FBReadMode refers to the
    ** partial products for the 640x480 mode.  We will need to look up
    ** the partial products in a table (c.f., glint_driver.c) to support
    ** other FB sizes.
    */
    cPriv->LBReadMode = (LBReadSrcDisable |
			 LBReadDstDisable |
			 LBDataTypeDefault |
			 LBWindowOriginBot |
			 LBScanLineInt2 |
			 0xe4); /* NOT_DONE: 640x480 partial products */
    cPriv->FBReadMode = (FBReadSrcDisable |
			 FBReadDstDisable |
			 FBDataTypeDefault |
			 FBWindowOriginBot |
			 FBScanLineInt2 |
			 0xe4); /* NOT_DONE: 640x480 partial products */

    cPriv->FBWindowBase = driScrnPriv->fbWidth * (driScrnPriv->fbHeight/2 - 1);
    cPriv->LBWindowBase = driScrnPriv->fbWidth * (driScrnPriv->fbHeight/2 - 1);

    cPriv->Begin = (B_AreaStippleDisable |
		    B_LineStippleDisable |
		    B_AntiAliasDisable |
		    B_TextureDisable |
		    B_FogDisable |
		    B_SubPixelCorrectEnable |
		    B_PrimType_Null);

    cPriv->ColorDDAMode = (ColorDDAEnable |
			   ColorDDAGouraud);

#ifdef CULL_ALL_PRIMS
    cPriv->GeometryMode = (GM_TextureDisable |
			   GM_FogDisable |
			   GM_FogExp |
			   GM_FrontPolyFill |
			   GM_BackPolyFill |
			   GM_FrontFaceCCW |
			   GM_PolyCullDisable |
			   GM_PolyCullBoth |
			   GM_ClipShortLinesDisable |
			   GM_ClipSmallTrisDisable |
			   GM_RenderMode |
			   GM_Feedback2D |
			   GM_CullFaceNormDisable |
			   GM_AutoFaceNormDisable |
			   GM_GouraudShading |
			   GM_UserClipNone |
			   GM_PolyOffsetPointDisable |
			   GM_PolyOffsetLineDisable |
			   GM_PolyOffsetFillDisable |
			   GM_InvertFaceNormCullDisable);
#else
    cPriv->GeometryMode = (GM_TextureDisable |
			   GM_FogDisable |
			   GM_FogExp |
			   GM_FrontPolyFill |
			   GM_BackPolyFill |
			   GM_FrontFaceCCW |
			   GM_PolyCullDisable |
			   GM_PolyCullBack |
			   GM_ClipShortLinesDisable |
			   GM_ClipSmallTrisDisable |
			   GM_RenderMode |
			   GM_Feedback2D |
			   GM_CullFaceNormDisable |
			   GM_AutoFaceNormDisable |
			   GM_GouraudShading |
			   GM_UserClipNone |
			   GM_PolyOffsetPointDisable |
			   GM_PolyOffsetLineDisable |
			   GM_PolyOffsetFillDisable |
			   GM_InvertFaceNormCullDisable);
#endif

    cPriv->AlphaTestMode = (AlphaTestModeDisable |
			    AT_Always);

    cPriv->AlphaBlendMode = (AlphaBlendModeDisable |
			     AB_Src_One |
			     AB_Dst_Zero |
			     AB_ColorFmt_8888 |
			     AB_NoAlphaBufferPresent |
			     AB_ColorOrder_RGB |
			     AB_OpenGLType |
			     AB_AlphaDst_FBData |
			     AB_ColorConversionScale |
			     AB_AlphaConversionScale);

    cPriv->AB_FBReadMode_Save = cPriv->AB_FBReadMode = 0;

    cPriv->Window = (WindowEnable  | /* For GID testing */
		     W_PassIfEqual |
		     (0 << 5)); /* GID part is set from draw priv (below) */

    cPriv->NotClipped = GL_FALSE;
    cPriv->WindowChanged = GL_TRUE;

    /*
    ** NOT_DONE:
    ** 1. These values should be calculated from the registers.
    ** 2. Only one client can use texture memory at this time.
    ** 3. A two-tiered texture allocation routine is needed to properly
    **    handle texture management.
    */
    cPriv->tmm = driTMMCreate(0x00080000,
			      0x00800000 - 0x00080000,
			      4, 1,
			      gammaTOLoad,
			      gammaTOLoadSub);
	
    cPriv->curTexObj = gammaTOFind(0);
    cPriv->curTexObj1D = cPriv->curTexObj;
    cPriv->curTexObj2D = cPriv->curTexObj;
    cPriv->Texture1DEnabled = GL_FALSE;
    cPriv->Texture2DEnabled = GL_FALSE;

#ifdef FORCE_DEPTH32
    cPriv->DepthSize = 32;
#else
    cPriv->DepthSize = v->gl_visual->DepthBits;
#endif
    cPriv->zNear = 0.0;
    cPriv->zFar  = 1.0;

    cPriv->Flags  = GAMMA_FRONT_BUFFER;
    cPriv->Flags |= (v->gl_visual->DBflag ? GAMMA_BACK_BUFFER  : 0);
    cPriv->Flags |= (cPriv->DepthSize > 0 ? GAMMA_DEPTH_BUFFER : 0);

    cPriv->EnabledFlags = GAMMA_FRONT_BUFFER;
    cPriv->EnabledFlags |= (v->gl_visual->DBflag ? GAMMA_BACK_BUFFER  : 0);

    cPriv->DepthMode = (DepthModeDisable |
			DM_WriteMask |
			DM_Less);

    cPriv->DeltaMode = (DM_SubPixlCorrectionEnable |
			DM_SmoothShadingEnable |
			DM_Target300TXMX);

    switch (cPriv->DepthSize) {
    case 16:
	cPriv->DeltaMode |= DM_Depth16;
	break;
    case 24:
	cPriv->DeltaMode |= DM_Depth24;
	break;
    case 32:
	cPriv->DeltaMode |= DM_Depth32;
	break;
    default:
	break;
    }

    cPriv->gammaScrnPriv = gPriv;

    c->private = (void *)cPriv;

    /* Initialize the HW to a known state */
    gammaInitHW(cPriv);

    return c;
}

void XMesaDestroyContext(XMesaContext c)
{
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

void XMesaSwapBuffers(XMesaBuffer b)
{
    /*
    ** NOT_DONE: This assumes buffer is currently bound to a context.
    ** This needs to be able to swap buffers when not currently bound.
    */
    if (gCC == NULL || gCCPriv == NULL)
	return;

    VALIDATE_DRAWABLE_INFO(gCC,gCCPriv);

    /* Flush any partially filled buffers */
    FLUSH_DMA_BUFFER(gCC, gCCPriv);

    if (gCCPriv->EnabledFlags & GAMMA_BACK_BUFFER) {
	int src, dst, x0, y0, x1, h;
	int i;
	__DRIdrawablePrivate *driDrawPriv =
	    gCC->driContextPriv->driDrawablePriv;
	int nRect = driDrawPriv->numClipRects;
	XF86DRIClipRectPtr pRect = driDrawPriv->pClipRects;
	__DRIscreenPrivate *driScrnPriv = gCC->driContextPriv->driScreenPriv;

#ifdef DO_VALIDATE
	DRM_SPINLOCK(&driScrnPriv->pSAREA->drawable_lock,
		     driScrnPriv->drawLockID);
	VALIDATE_DRAWABLE_INFO_NO_LOCK(gCC,gCCPriv);
#endif

	CHECK_DMA_BUFFER(nullCC, gCCPriv, 2);
	WRITE(gCCPriv->buf, FBReadMode, (gCCPriv->FBReadMode |
					 FBReadSrcEnable));
	WRITE(gCCPriv->buf, LBWriteMode, LBWriteModeDisable);

	for (i = 0; i < nRect; i++, pRect++) {
	    x0 = pRect->x1;
	    x1 = pRect->x2;
	    h  = pRect->y2 - pRect->y1;
	    y0 = driScrnPriv->fbHeight - (pRect->y1+h);

	    src = (y0/2)*driScrnPriv->fbWidth+x0;
	    y0 += driScrnPriv->fbHeight;
	    dst = (y0/2)*driScrnPriv->fbWidth+x0;

	    CHECK_DMA_BUFFER(nullCC, gCCPriv, 9);
	    WRITE(gCCPriv->buf, StartXDom,       x0<<16);   /* X0dest */
	    WRITE(gCCPriv->buf, StartY,          y0<<16);   /* Y0dest */
	    WRITE(gCCPriv->buf, StartXSub,       x1<<16);   /* X1dest */
	    WRITE(gCCPriv->buf, GLINTCount,      h);        /* H */
	    WRITE(gCCPriv->buf, dY,              1<<16);    /* ydir */
	    WRITE(gCCPriv->buf, dXDom,           0<<16);
	    WRITE(gCCPriv->buf, dXSub,           0<<16);
	    WRITE(gCCPriv->buf, FBSourceOffset, (dst-src));
	    WRITE(gCCPriv->buf, Render,          0x00040048); /* NOT_DONE */
	}

	/*
	** NOTE: FBSourceOffset (above) is backwards from what is
	** described in the manual (i.e., dst-src instead of src-dst)
	** due to our using the bottom-left window origin instead of the
	** top-left window origin.
	*/

	/* Restore FBReadMode */
	CHECK_DMA_BUFFER(nullCC, gCCPriv, 2);
	WRITE(gCCPriv->buf, FBReadMode, (gCCPriv->FBReadMode |
					 gCCPriv->AB_FBReadMode));
	WRITE(gCCPriv->buf, LBWriteMode, LBWriteModeEnable);

#ifdef DO_VALIDATE
	PROCESS_DMA_BUFFER_TOP_HALF(gCCPriv);

	DRM_SPINUNLOCK(&driScrnPriv->pSAREA->drawable_lock,
		       driScrnPriv->drawLockID);
	VALIDATE_DRAWABLE_INFO_NO_LOCK_POST(gCC,gCCPriv);

	PROCESS_DMA_BUFFER_BOTTOM_HALF(gCCPriv);
#else
	FLUSH_DMA_BUFFER(gCC,gCCPriv);
#endif

    }
}

GLboolean XMesaMakeCurrent(XMesaContext c, XMesaBuffer b)
{
    if (c) {
	gCC     = c;
	gCCPriv = (gammaContextPrivate *)c->private;

	gCCPriv->Window &= ~W_GIDMask;
	gCCPriv->Window |= (gCC->driContextPriv->driDrawablePriv->index << 5);

	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, GLINTWindow, gCCPriv->Window);

        _glapi_set_dispatch(Dispatch);
    } else {
	gCC     = NULL;
	gCCPriv = NULL;
    }
    return GL_TRUE;
}


GLboolean XMesaUnbindContext( XMesaContext c )
{
   /* XXX not 100% sure what's supposed to be done here */
   return GL_TRUE;
}


void __driRegisterExtensions(void)
{
   /* No extensions */
}


#endif
