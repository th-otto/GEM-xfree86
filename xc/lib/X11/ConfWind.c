/* $TOG: ConfWind.c /main/9 1998/02/06 17:12:00 kaleb $ */
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
/* $XFree86: xc/lib/X11/ConfWind.c,v 1.2 1999/05/09 10:49:06 dawes Exp $ */

#include "Xlibint.h"

int
XMoveResizeWindow(dpy, w, x, y, width, height)
register Display *dpy;
Window w;
int x, y;
unsigned int width, height;
{
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 16, req);
    req->window = w;
    req->mask = CWX | CWY | CWWidth | CWHeight;
#ifdef MUSTCOPY
    {
	long lx = x, ly = y;
	unsigned long lwidth = width, lheight = height;

	dpy->bufptr -= 16;
	Data32 (dpy, (long *) &lx, 4);	/* order must match values of */
	Data32 (dpy, (long *) &ly, 4);	/* CWX, CWY, CWWidth, and CWHeight */
	Data32 (dpy, (long *) &lwidth, 4);
	Data32 (dpy, (long *) &lheight, 4);
    }
#else
    {
	register CARD32 *valuePtr =
	  (CARD32 *) NEXTPTR(req,xConfigureWindowReq);
	*valuePtr++ = x;
	*valuePtr++ = y;
	*valuePtr++ = width;
	*valuePtr   = height;
    }
#endif /* MUSTCOPY */
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
