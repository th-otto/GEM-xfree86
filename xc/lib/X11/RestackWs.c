/* $TOG: RestackWs.c /main/10 1998/02/06 17:50:47 kaleb $ */
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
/* $XFree86: xc/lib/X11/RestackWs.c,v 1.2 1999/05/09 10:50:02 dawes Exp $ */

#include "Xlibint.h"

int
XRestackWindows (dpy, windows, n)
    register Display *dpy;
    register Window *windows;
    int n;
    {
    int i = 0;
#ifdef MUSTCOPY
    unsigned long val = Below;		/* needed for macro below */
#endif

    LockDisplay(dpy);
    while (windows++, ++i < n) {
	register xConfigureWindowReq *req;

    	GetReqExtra (ConfigureWindow, 8, req);
	req->window = *windows;
	req->mask = CWSibling | CWStackMode;
#ifdef MUSTCOPY
	dpy->bufptr -= 8;
	Data32 (dpy, (long *)(windows-1), 4);
	Data32 (dpy, (long *)&val, 4);
#else
	{
	    register CARD32 *values = (CARD32 *)
	      NEXTPTR(req,xConfigureWindowReq);
	    *values++ = *(windows-1);
	    *values   = Below;
	}
#endif /* MUSTCOPY */
	}
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
    }

    

    
