/* $TOG: FSSaver.c /main/6 1998/02/06 17:23:57 kaleb $ */
/*

Copyright 1987, 1998  The Open Group

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
/* $XFree86: xc/lib/X11/FSSaver.c,v 1.2 1999/05/09 10:49:17 dawes Exp $ */

#include "Xlibint.h"

int
XActivateScreenSaver(dpy) 
    register Display *dpy;

{
    XForceScreenSaver (dpy, ScreenSaverActive);
    return 1;
}

int
XResetScreenSaver(dpy) 
    register Display *dpy;

{
    XForceScreenSaver (dpy, ScreenSaverReset);
    return 1;
}

int
XForceScreenSaver(dpy, mode)
    register Display *dpy; 
    int mode;

{
    register xForceScreenSaverReq *req;

    LockDisplay(dpy);
    GetReq(ForceScreenSaver, req);
    req->mode = mode;
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

