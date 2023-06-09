/* $TOG: ChWindow.c /main/10 1998/02/06 17:08:21 kaleb $ */
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
/* $XFree86: xc/lib/X11/ChWindow.c,v 1.4 1999/05/09 10:49:02 dawes Exp $ */

#include "Xlibint.h"

int
XResizeWindow(dpy, w, width, height)
register Display *dpy;
Window w;
unsigned int width, height;
{
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 8, req); /* 2 4-byte quantities */

    req->window = w;
    req->mask = CWWidth | CWHeight;
#ifdef MUSTCOPY
    {
	unsigned long lwidth = width, lheight = height;
    dpy->bufptr -= 8;
    Data32 (dpy, (long *) &lwidth, 4);	/* order dictated by values of */
    Data32 (dpy, (long *) &lheight, 4);	/* CWWidth and CWHeight */
    }
#else
    {
	CARD32 *valuePtr = (CARD32 *) NEXTPTR(req,xConfigureWindowReq);
	*valuePtr++ = width;
	*valuePtr = height;
    }
#endif /* MUSTCOPY */
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
