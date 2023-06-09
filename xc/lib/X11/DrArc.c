/* $TOG: DrArc.c /main/9 1998/02/06 17:18:14 kaleb $ */
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
/* $XFree86: xc/lib/X11/DrArc.c,v 1.2 1999/05/09 10:49:13 dawes Exp $ */

/* Note to future maintainers:  XDrawArc does NOT batch successive PolyArc
   requests into a single request like XDrawLine, XDrawPoint, etc.
   We don't do this because X_PolyArc applies the GC's join-style if
   the last point in one arc coincides with the first point in another.
   The client wouldn't expect this and would have no easy way to defeat it. */
   
#include "Xlibint.h"

int
XDrawArc(dpy, d, gc, x, y, width, height, angle1, angle2)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y; /* INT16 */
    unsigned int width, height; /* CARD16 */
    int angle1, angle2; /* INT16 */
{
    register xPolyArcReq *req;
    register xArc *arc;
#ifdef MUSTCOPY
    xArc arcdata;
    long len = SIZEOF(xArc);

    arc = &arcdata;
#endif /* MUSTCOPY */

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReqExtra (PolyArc, SIZEOF(xArc), req);

    req->drawable = d;
    req->gc = gc->gid;

#ifndef MUSTCOPY
    arc = (xArc *) NEXTPTR(req,xPolyArcReq);
#endif /* MUSTCOPY */

    arc->x = x;
    arc->y = y;
    arc->width = width;
    arc->height = height;
    arc->angle1 = angle1;
    arc->angle2 = angle2;

#ifdef MUSTCOPY
    dpy->bufptr -= SIZEOF(xArc);
    Data (dpy, (char *) arc, len);
#endif /* MUSTCOPY */

    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
