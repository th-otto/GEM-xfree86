/* $TOG: FillArcs.c /main/12 1998/02/06 17:20:51 kaleb $ */
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
/* $XFree86: xc/lib/X11/FillArcs.c,v 1.2 1999/05/09 10:49:19 dawes Exp $ */

#include "Xlibint.h"

#define arc_scale (SIZEOF(xArc) / 4)

int
XFillArcs(dpy, d, gc, arcs, n_arcs)
register Display *dpy;
Drawable d;
GC gc;
XArc *arcs;
int n_arcs;
{
    register xPolyFillArcReq *req;
    long len;
    int n;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    while (n_arcs) {
	GetReq(PolyFillArc, req);
	req->drawable = d;
	req->gc = gc->gid;
	n = n_arcs;
	len = ((long)n) * arc_scale;
	if (!dpy->bigreq_size && len > (dpy->max_request_size - req->length)) {
	    n = (dpy->max_request_size - req->length) / arc_scale;
	    len = ((long)n) * arc_scale;
	}
	SetReqLen(req, len, len);
	len <<= 2; /* watch out for macros... */
	Data16 (dpy, (short *) arcs, len);
	n_arcs -= n;
	arcs += n;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
