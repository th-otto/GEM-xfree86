/* $TOG: StColors.c /main/9 1998/02/06 17:54:07 kaleb $ */
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
/* $XFree86: xc/lib/X11/StColors.c,v 1.2 1999/05/09 10:50:14 dawes Exp $ */

#include "Xlibint.h"

int
XStoreColors(dpy, cmap, defs, ncolors)
register Display *dpy;
Colormap cmap;
XColor *defs;
int ncolors;
{
    register int i;
    xColorItem citem;
    register xStoreColorsReq *req;

    LockDisplay(dpy);    
    GetReq(StoreColors, req);

    req->cmap = cmap;

    req->length += (ncolors * SIZEOF(xColorItem)) >> 2; /* assume size is 4*n */

    for (i = 0; i < ncolors; i++) {
	citem.pixel = defs[i].pixel;
	citem.red = defs[i].red;
	citem.green = defs[i].green;
	citem.blue = defs[i].blue;
	citem.flags = defs[i].flags;

	/* note that xColorItem doesn't contain all 16-bit quantities, so
	   we can't use Data16 */
	Data(dpy, (char *)&citem, (long) SIZEOF(xColorItem)); 
			/* assume size is 4*n */
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
