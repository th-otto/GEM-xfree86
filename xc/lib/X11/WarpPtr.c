/* $TOG: WarpPtr.c /main/6 1998/02/06 17:56:58 kaleb $ */
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
/* $XFree86: xc/lib/X11/WarpPtr.c,v 1.2 1999/05/09 10:50:22 dawes Exp $ */

#include "Xlibint.h"

int
XWarpPointer(dpy, src_win, dest_win, src_x, src_y, src_width, src_height,
	     dest_x, dest_y)
     register Display *dpy;
     Window src_win, dest_win;
     int src_x, src_y;
     unsigned int src_width, src_height;
     int dest_x, dest_y;
{       
    register xWarpPointerReq *req;

    LockDisplay(dpy);
    GetReq(WarpPointer, req);
    req->srcWid = src_win;
    req->dstWid = dest_win;
    req->srcX = src_x;
    req->srcY = src_y;
    req->srcWidth = src_width;
    req->srcHeight = src_height;
    req->dstX = dest_x;
    req->dstY = dest_y;
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

