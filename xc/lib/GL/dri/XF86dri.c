/* $XFree86: xc/lib/GL/dri/XF86dri.c,v 1.5 2000/02/23 04:46:33 martin Exp $ */
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
 *   Jens Owen <jens@precisioninsight.com>
 *
 */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#define NEED_REPLIES
#include "Xlibint.h"
#include "xf86dristr.h"
#include "Xext.h"
#include "extutil.h"

static XExtensionInfo _xf86dri_info_data;
static XExtensionInfo *xf86dri_info = &_xf86dri_info_data;
static char *xf86dri_extension_name = XF86DRINAME;

#define XF86DRICheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xf86dri_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display(Display *dpy, XExtCodes *extCodes);
static /* const */ XExtensionHooks xf86dri_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    close_display,			/* close_display */
    NULL,				/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL,				/* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, xf86dri_info, 
				   xf86dri_extension_name, 
				   &xf86dri_extension_hooks, 
				   0, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, xf86dri_info)


/*****************************************************************************
 *                                                                           *
 *		    public XFree86-DRI Extension routines                    *
 *                                                                           *
 *****************************************************************************/

Bool XF86DRIQueryExtension (dpy, event_basep, error_basep)
    Display *dpy;
    int *event_basep, *error_basep;
{
    XExtDisplayInfo *info = find_display (dpy);

    if (XextHasExtension(info)) {
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	return True;
    } else {
	return False;
    }
}

Bool XF86DRIQueryVersion(dpy, majorVersion, minorVersion, patchVersion)
    Display* dpy;
    int* majorVersion; 
    int* minorVersion;
    int* patchVersion;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRIQueryVersionReply rep;
    xXF86DRIQueryVersionReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRIQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRIQueryVersion;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    *majorVersion = rep.majorVersion;
    *minorVersion = rep.minorVersion;
    *patchVersion = rep.patchVersion;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRIQueryDirectRenderingCapable(dpy, screen, isCapable)
    Display* dpy;
    int screen;
    Bool* isCapable;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRIQueryDirectRenderingCapableReply rep;
    xXF86DRIQueryDirectRenderingCapableReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRIQueryDirectRenderingCapable, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRIQueryDirectRenderingCapable;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    *isCapable = rep.isCapable;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRIOpenConnection(dpy, screen, hSAREA, busIdString)
    Display* dpy;
    int screen;
    drmHandlePtr hSAREA;
    char **busIdString;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRIOpenConnectionReply rep;
    xXF86DRIOpenConnectionReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRIOpenConnection, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRIOpenConnection;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    *hSAREA = rep.hSAREALow;
#ifdef LONG64
    *hSAREA |= ((drmHandle)rep.hSAREAHigh) << 32;
#endif

    if (rep.length) {
        if (!(*busIdString = (char *)Xcalloc(rep.length + 1, 1))) {
            _XEatData(dpy, ((rep.busIdStringLength+3) & ~3));
            return False;
        }
	_XReadPad(dpy, *busIdString, rep.busIdStringLength);
    } else {
        *busIdString = NULL;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRIAuthConnection(dpy, screen, magic)
    Display* dpy;
    int screen;
    drmMagic magic;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRIAuthConnectionReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRIAuthConnection, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRIAuthConnection;
    req->screen = screen;
    req->magic = magic;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRICloseConnection(dpy, screen)
    Display* dpy;
    int screen;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRICloseConnectionReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRICloseConnection, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRICloseConnection;
    req->screen = screen;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRIGetClientDriverName(dpy, screen, ddxDriverMajorVersion, 
	ddxDriverMinorVersion, ddxDriverPatchVersion, clientDriverName)
    Display* dpy;
    int screen;
    int* ddxDriverMajorVersion;
    int* ddxDriverMinorVersion;
    int* ddxDriverPatchVersion;
    char** clientDriverName;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRIGetClientDriverNameReply rep;
    xXF86DRIGetClientDriverNameReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRIGetClientDriverName, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRIGetClientDriverName;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    *ddxDriverMajorVersion = rep.ddxDriverMajorVersion;
    *ddxDriverMinorVersion = rep.ddxDriverMinorVersion;
    *ddxDriverPatchVersion = rep.ddxDriverPatchVersion;

    if (rep.length) {
        if (!(*clientDriverName = (char *)Xcalloc(rep.length + 1, 1))) {
            _XEatData(dpy, ((rep.clientDriverNameLength+3) & ~3));
            return False;
        }
	_XReadPad(dpy, *clientDriverName, rep.clientDriverNameLength);
    } else {
        *clientDriverName = NULL;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRICreateContext(dpy, screen, visual, context, hHWContext)
    Display* dpy;
    int screen;
    Visual* visual;
    XID* context;
    drmContextPtr hHWContext;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRICreateContextReply rep;
    xXF86DRICreateContextReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRICreateContext, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRICreateContext;
    req->visual = visual->visualid;
    req->screen = screen;
    *context = XAllocID(dpy);
    req->context = *context;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    *hHWContext = rep.hHWContext;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRIDestroyContext(dpy, screen, context)
    Display* dpy;
    int screen;
    XID context;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRIDestroyContextReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRIDestroyContext, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRIDestroyContext;
    req->screen = screen;
    req->context = context;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRICreateDrawable(dpy, screen, drawable, hHWDrawable)
    Display* dpy;
    int screen;
    Drawable drawable;
    drmDrawablePtr hHWDrawable;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRICreateDrawableReply rep;
    xXF86DRICreateDrawableReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRICreateDrawable, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRICreateDrawable;
    req->screen = screen;
    req->drawable = drawable;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    *hHWDrawable = rep.hHWDrawable;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRIDestroyDrawable(dpy, screen, drawable)
    Display* dpy;
    int screen;
    Drawable drawable;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRIDestroyDrawableReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRIDestroyDrawable, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRIDestroyDrawable;
    req->screen = screen;
    req->drawable = drawable;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRIGetDrawableInfo(dpy, screen, drawable, 
	index, stamp, X, Y, W, H, numClipRects, pClipRects)
    Display* dpy;
    int screen;
    Drawable drawable;
    unsigned int* index;
    unsigned int* stamp;
    int* X;
    int* Y;
    int* W;
    int* H;
    int* numClipRects;
    XF86DRIClipRectPtr* pClipRects;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRIGetDrawableInfoReply rep;
    xXF86DRIGetDrawableInfoReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRIGetDrawableInfo, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRIGetDrawableInfo;
    req->screen = screen;
    req->drawable = drawable;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    *index = rep.drawableTableIndex;
    *stamp = rep.drawableTableStamp;
    *X = (int)rep.drawableX;
    *Y = (int)rep.drawableY;
    *W = (int)rep.drawableWidth;
    *H = (int)rep.drawableHeight;
    *numClipRects = rep.numClipRects;

    if (rep.length) {
        if (!(*pClipRects = (XF86DRIClipRectPtr)Xcalloc(rep.length, 1))) {
            _XEatData(dpy, rep.length);
            return False;
        }
	_XRead32(dpy, *pClipRects, rep.length);
    } else {
        *pClipRects = NULL;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86DRIGetDeviceInfo(dpy, screen, hFrameBuffer, 
	fbOrigin, fbSize, fbStride, devPrivateSize, pDevPrivate)
    Display* dpy;
    int screen;
    drmHandlePtr hFrameBuffer;
    int* fbOrigin;
    int* fbSize;
    int* fbStride;
    int* devPrivateSize;
    void** pDevPrivate;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DRIGetDeviceInfoReply rep;
    xXF86DRIGetDeviceInfoReq *req;

    XF86DRICheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DRIGetDeviceInfo, req);
    req->reqType = info->codes->major_opcode;
    req->driReqType = X_XF86DRIGetDeviceInfo;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    *hFrameBuffer = rep.hFrameBufferLow;
#ifdef LONG64
    *hFrameBuffer = ((drmHandle)rep.hFrameBufferHigh) << 32;
#endif

    *fbOrigin = rep.framebufferOrigin;
    *fbSize = rep.framebufferSize;
    *fbStride = rep.framebufferStride;
    *devPrivateSize = rep.devPrivateSize;

    if (rep.length) {
        if (!(*pDevPrivate = (void *)Xcalloc(rep.length, 1))) {
            _XEatData(dpy, ((rep.length+3) & ~3));
            return False;
        }
	_XRead32(dpy, *pDevPrivate, rep.length);
    } else {
        *pDevPrivate = NULL;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}
