/* $TOG: Text16.c /main/13 1998/02/06 17:55:02 kaleb $ */
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
/* $XFree86: xc/lib/X11/Text16.c,v 1.2 1999/05/09 10:50:16 dawes Exp $ */

#include "Xlibint.h"

int
#if NeedFunctionPrototypes
XDrawString16(
    register Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst XChar2b *string,
    int length)
#else
XDrawString16(dpy, d, gc, x, y, string, length)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y;
    XChar2b *string;
    int length;
#endif
{   
    int Datalength = 0;
    register xPolyText16Req *req;

    if (length <= 0)
       return 0;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq (PolyText16, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->x = x;
    req->y = y;


    Datalength += SIZEOF(xTextElt) * ((length + 253) / 254) + (length << 1);


    req->length += (Datalength + 3)>>2;  /* convert to number of 32-bit words */


    /* 
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.   If the request does fit into the
     * empty buffer, then we won't have to flush it at the end to keep
     * the buffer 32-bit aligned. 
     */

    if (dpy->bufptr + Datalength > dpy->bufmax)
    	_XFlush (dpy);

    {
	int nbytes;
	int PartialNChars = length;
        register xTextElt *elt;
 	XChar2b *CharacterOffset = (XChar2b *)string;

	while(PartialNChars > 254)
        {
 	    nbytes = 254 * 2 + SIZEOF(xTextElt);
	    BufAlloc (xTextElt *, elt, nbytes);
	    elt->delta = 0;
	    elt->len = 254;
#if defined(MUSTCOPY) || defined(MUSTCOPY2B)
	    {
		register int i;
		register unsigned char *cp;
		for (i = 0, cp = ((unsigned char *)elt) + 2; i < 254; i++) {
		    *cp++ = CharacterOffset[i].byte1;
		    *cp++ = CharacterOffset[i].byte2;
		}
	    }
#else
            memcpy ((char *) (elt + 1), (char *)CharacterOffset, 254 * 2);
#endif
	    PartialNChars = PartialNChars - 254;
	    CharacterOffset += 254;
	}
	    
        if (PartialNChars)
        {
	    nbytes = PartialNChars * 2  + SIZEOF(xTextElt);
	    BufAlloc (xTextElt *, elt, nbytes); 
	    elt->delta = 0;
	    elt->len = PartialNChars;
#if defined(MUSTCOPY) || defined(MUSTCOPY2B)
	    {
		register int i;
		register unsigned char *cp;
		for (i = 0, cp = ((unsigned char *)elt) + 2; i < PartialNChars;
		     i++) {
		    *cp++ = CharacterOffset[i].byte1;
		    *cp++ = CharacterOffset[i].byte2;
		}
	    }
#else
            memcpy((char *)(elt + 1), (char *)CharacterOffset, PartialNChars * 2);
#endif
	 }
    }

    /* Pad request out to a 32-bit boundary */

    if (Datalength &= 3) {
	char *pad;
	/* 
	 * BufAlloc is a macro that uses its last argument more than
	 * once, otherwise I'd write "BufAlloc (char *, pad, 4-length)" 
	 */
	length = 4 - Datalength;
	BufAlloc (char *, pad, length);
	/* 
	 * if there are 3 bytes of padding, the first byte MUST be 0
	 * so the pad bytes aren't mistaken for a final xTextElt 
	 */
	*pad = 0;
        }

    /* 
     * If the buffer pointer is not now pointing to a 32-bit boundary,
     * we must flush the buffer so that it does point to a 32-bit boundary
     * at the end of this routine. 
     */

    if ((dpy->bufptr - dpy->buffer) & 3)
       _XFlush (dpy);
    UnlockDisplay(dpy);
    SyncHandle();
    return 0;
}

