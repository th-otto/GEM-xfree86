/* $XFree86: xc/lib/GL/mesa/dri/dri_xmesaapi.h,v 1.3 2000/02/23 04:46:38 martin Exp $ */
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
 *
 */

#ifndef _DRI_XMESAAPI_H_
#define _DRI_XMESAAPI_H_

#ifdef GLX_DIRECT_RENDERING

#include "GL/xmesa.h"
#include "dri_mesa.h"

typedef struct __XMESAapiRec __XMESAapi;

struct __XMESAapiRec {
    /* XMESA Functions */
    GLboolean    (*InitDriver)(__DRIscreenPrivate *driScrnPriv);
    void         (*ResetDriver)(__DRIscreenPrivate *driScrnPriv);
    XMesaVisual  (*CreateVisual)(XMesaDisplay *display,
				 XMesaVisualInfo visinfo,
				 GLboolean rgb_flag,
				 GLboolean alpha_flag,
				 GLboolean db_flag,
				 GLboolean stereo_flag,
				 GLboolean ximage_flag,
				 GLint depth_size,
				 GLint stencil_size,
				 GLint accum_size,
				 GLint level);
    void         (*DestroyVisual)(XMesaVisual v);
    XMesaContext (*CreateContext)(XMesaVisual v, XMesaContext share_list,
				  __DRIcontextPrivate *driContextPriv);
    void         (*DestroyContext)(XMesaContext c);
    XMesaBuffer  (*CreateWindowBuffer)(XMesaVisual v, XMesaWindow w,
				       __DRIdrawablePrivate *driDrawPriv);
    XMesaBuffer  (*CreatePixmapBuffer)(XMesaVisual v, XMesaPixmap p,
				       XMesaColormap c,
				       __DRIdrawablePrivate *driDrawPriv);
    void         (*DestroyBuffer)(XMesaBuffer b);
    void         (*SwapBuffers)(XMesaBuffer b);
    GLboolean    (*MakeCurrent)(XMesaContext c, XMesaBuffer b);
    GLboolean    (*UnbindContext)(XMesaContext c);
};

#endif
#endif /* _DRI_XMESAAPI_H_ */
