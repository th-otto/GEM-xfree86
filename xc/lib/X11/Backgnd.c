/* $TOG: Backgnd.c /main/7 1998/02/06 17:03:34 kaleb $ */
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
/* $XFree86: xc/lib/X11/Backgnd.c,v 1.2 1999/05/09 10:48:56 dawes Exp $ */

#include "Xlibint.h"

int
XSetWindowBackground(dpy, w, pixel)
    register Display *dpy;
    Window w;
    unsigned long pixel;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReqExtra (ChangeWindowAttributes, 4, req);
    req->window = w;
    req->valueMask = CWBackPixel;
    OneDataCard32 (dpy, NEXTPTR(req,xChangeWindowAttributesReq), pixel);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

