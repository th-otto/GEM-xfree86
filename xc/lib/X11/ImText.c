/* $TOG: ImText.c /main/12 1998/02/06 17:36:40 kaleb $ */
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
/* $XFree86: xc/lib/X11/ImText.c,v 1.2 1999/05/09 10:49:36 dawes Exp $ */

#define NEED_REPLIES
#include "Xlibint.h"

int
#if NeedFunctionPrototypes
XDrawImageString(
    register Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst char *string,
    int length)
#else
XDrawImageString(dpy, d, gc, x, y, string, length)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y;
    char *string;
    int length;
#endif
{   
    register xImageText8Req *req;
    char *CharacterOffset = (char *)string;
    int FirstTimeThrough = True;
    int lastX = 0;

    LockDisplay(dpy);
    FlushGC(dpy, gc);

    while (length > 0) 
    {
	int Unit;

	if (length > 255) Unit = 255;
	else Unit = length;

   	if (FirstTimeThrough)
	{
	    FirstTimeThrough = False;
        }
	else
	{
	    char buf[512];
	    char *ptr, *str;
	    xQueryTextExtentsReq *qreq;
	    xQueryTextExtentsReply rep;
	    int i;

	    GetReq(QueryTextExtents, qreq);
	    qreq->fid = gc->gid;
	    qreq->length += (510 + 3)>>2;
	    qreq->oddLength = 1;
	    str = CharacterOffset - 255;
	    for (ptr = buf, i = 255; --i >= 0; ) {
		*ptr++ = 0;
		*ptr++ = *str++;
	    }
	    Data (dpy, buf, 510);
	    if (!_XReply (dpy, (xReply *)&rep, 0, xTrue))
		break;

	    x = lastX + cvtINT32toInt (rep.overallWidth);
	}

        GetReq (ImageText8, req);
        req->length += (Unit + 3) >> 2;
        req->nChars = Unit;
        req->drawable = d;
        req->gc = gc->gid;
        req->y = y;

	lastX = req->x = x;
        Data (dpy, CharacterOffset, (long)Unit);
        CharacterOffset += Unit;
	length -= Unit;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return 0;
}

