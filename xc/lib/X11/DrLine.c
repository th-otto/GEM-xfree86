/* $TOG: DrLine.c /main/9 1998/02/06 17:18:37 kaleb $ */
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
/* $XFree86: xc/lib/X11/DrLine.c,v 1.2 1999/05/09 10:49:14 dawes Exp $ */

#include "Xlibint.h"

/* precompute the maximum size of batching request allowed */

#define wsize (SIZEOF(xPolySegmentReq) + WLNSPERBATCH * SIZEOF(xSegment))
#define zsize (SIZEOF(xPolySegmentReq) + ZLNSPERBATCH * SIZEOF(xSegment))

int
XDrawLine (dpy, d, gc, x1, y1, x2, y2)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x1, y1, x2, y2;
{
    register xSegment *segment;
#ifdef MUSTCOPY
    xSegment segmentdata;
    long len = SIZEOF(xSegment);

    segment = &segmentdata;
#endif /* not MUSTCOPY */

    LockDisplay(dpy);
    FlushGC(dpy, gc);

    {
    register xPolySegmentReq *req = (xPolySegmentReq *) dpy->last_req;

    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolySegment)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && ((dpy->bufptr + SIZEOF(xSegment)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < (gc->values.line_width ?
						  wsize : zsize)) ) {
	 req->length += SIZEOF(xSegment) >> 2;
#ifndef MUSTCOPY
         segment = (xSegment *) dpy->bufptr;
	 dpy->bufptr += SIZEOF(xSegment);
#endif /* not MUSTCOPY */
	 }

    else {
	GetReqExtra (PolySegment, SIZEOF(xSegment), req);
	req->drawable = d;
	req->gc = gc->gid;
#ifdef MUSTCOPY
	dpy->bufptr -= SIZEOF(xSegment);
#else
	segment = (xSegment *) NEXTPTR(req,xPolySegmentReq);
#endif /* MUSTCOPY */
	}

    segment->x1 = x1;
    segment->y1 = y1;
    segment->x2 = x2;
    segment->y2 = y2;

#ifdef MUSTCOPY
    Data (dpy, (char *) &segmentdata, len);
#endif /* MUSTCOPY */

    UnlockDisplay(dpy);
    SyncHandle();
    }
    return 1;
}

