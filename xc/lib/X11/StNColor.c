/* $TOG: StNColor.c /main/21 1998/02/06 17:54:35 kaleb $ */
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
/* $XFree86: xc/lib/X11/StNColor.c,v 1.2 1999/05/09 10:50:15 dawes Exp $ */

#include <stdio.h>
#include "Xlibint.h"
#include "Xcmsint.h"

extern void _XcmsRGB_to_XColor();

/* cmsColNm.c */
Status _XcmsResolveColorString();

int
#if NeedFunctionPrototypes
XStoreNamedColor(
register Display *dpy,
Colormap cmap,
_Xconst char *name, /* STRING8 */
unsigned long pixel, /* CARD32 */
int flags)  /* DoRed, DoGreen, DoBlue */
#else
XStoreNamedColor(dpy, cmap, name, pixel, flags)
register Display *dpy;
Colormap cmap;
char *name; /* STRING8 */
unsigned long pixel; /* CARD32 */
int flags;  /* DoRed, DoGreen, DoBlue */
#endif
{
    unsigned int nbytes;
    register xStoreNamedColorReq *req;
    XcmsCCC ccc;
    XcmsColor cmsColor_exact;
    XColor scr_def;

    /*
     * Let's Attempt to use Xcms approach to Parse Color
     */
    if ((ccc = XcmsCCCOfColormap(dpy, cmap)) != (XcmsCCC)NULL) {
	if (_XcmsResolveColorString(ccc, &name, &cmsColor_exact,
		XcmsRGBFormat) >= XcmsSuccess) {
	    _XcmsRGB_to_XColor(&cmsColor_exact, &scr_def, 1);
	    scr_def.pixel = pixel;
	    scr_def.flags = flags;
	    return XStoreColor(dpy, cmap, &scr_def);
	}
	/*
	 * Otherwise we failed; or name was changed with yet another
	 * name.  Thus pass name to the X Server.
	 */
    }

    /*
     * The Xcms and i18n methods failed, so lets pass it to the server
     * for parsing.
     */

    LockDisplay(dpy);
    GetReq(StoreNamedColor, req);

    req->cmap = cmap;
    req->flags = flags;
    req->pixel = pixel;
    req->nbytes = nbytes = strlen(name);
    req->length += (nbytes + 3) >> 2; /* round up to multiple of 4 */
    Data(dpy, name, (long)nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
    return 0;
}


