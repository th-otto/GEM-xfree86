/* $TOG: SetCRects.c /main/13 1998/02/06 17:51:38 kaleb $ */
/*

Copyright 1986, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/lib/X11/SetCRects.c,v 1.2 1999/05/09 10:50:05 dawes Exp $ */

#include "Xlibint.h"

/* can only call when display is locked. */
void _XSetClipRectangles (dpy, gc, clip_x_origin, clip_y_origin, rectangles, n,
                    ordering)
    register Display *dpy;
    GC gc;
    int clip_x_origin, clip_y_origin;
    XRectangle *rectangles;
    int n;
    int ordering;
{
    register xSetClipRectanglesReq *req;
    register long len;
    unsigned long dirty;
    register _XExtension *ext;

    GetReq (SetClipRectangles, req);
    req->gc = gc->gid;
    req->xOrigin = gc->values.clip_x_origin = clip_x_origin;
    req->yOrigin = gc->values.clip_y_origin = clip_y_origin;
    req->ordering = ordering;
    len = ((long)n) << 1;
    SetReqLen(req, len, 1);
    len <<= 2;
    Data16 (dpy, (short *) rectangles, len);
    gc->rects = 1;
    dirty = gc->dirty & ~(GCClipMask | GCClipXOrigin | GCClipYOrigin);
    gc->dirty = GCClipMask | GCClipXOrigin | GCClipYOrigin;
    /* call out to any extensions interested */
    for (ext = dpy->ext_procs; ext; ext = ext->next)
	if (ext->flush_GC) (*ext->flush_GC)(dpy, gc, &ext->codes);
    gc->dirty = dirty;
}

int
XSetClipRectangles (dpy, gc, clip_x_origin, clip_y_origin, rectangles, n,
                    ordering)
    register Display *dpy;
    GC gc;
    int clip_x_origin, clip_y_origin;
    XRectangle *rectangles;
    int n;
    int ordering;
{
    LockDisplay(dpy);
    _XSetClipRectangles (dpy, gc, clip_x_origin, clip_y_origin, rectangles, n,
                    ordering);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
    
