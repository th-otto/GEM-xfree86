/* $TOG: GetFPath.c /main/8 1998/02/06 17:25:22 kaleb $ */
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
/* $XFree86: xc/lib/X11/GetFPath.c,v 1.2 1999/05/09 10:49:26 dawes Exp $ */

#define NEED_REPLIES
#include "Xlibint.h"

char **XGetFontPath(dpy, npaths)
register Display *dpy;
int *npaths;	/* RETURN */
{
	xGetFontPathReply rep;
	register long nbytes;
	char **flist;
	char *ch;
	register unsigned i;
	register int length;
	register xReq *req;

	LockDisplay(dpy);
	GetEmptyReq (GetFontPath, req);
	(void) _XReply (dpy, (xReply *) &rep, 0, xFalse);

	if (rep.nPaths) {
	    flist = (char **)
		Xmalloc((unsigned) rep.nPaths * sizeof (char *));
	    nbytes = (long)rep.length << 2;
	    ch = (char *) Xmalloc ((unsigned) (nbytes + 1));
                /* +1 to leave room for last null-terminator */

	    if ((! flist) || (! ch)) {
		if (flist) Xfree((char *) flist);
		if (ch) Xfree(ch);
		_XEatData(dpy, (unsigned long) nbytes);
		UnlockDisplay(dpy);
		SyncHandle();
		return (char **) NULL;
	    }

	    _XReadPad (dpy, ch, nbytes);
	    /*
	     * unpack into null terminated strings.
	     */
	    length = *ch;
	    for (i = 0; i < rep.nPaths; i++) {
		flist[i] = ch+1;  /* skip over length */
		ch += length + 1; /* find next length ... */
		length = *ch;
		*ch = '\0'; /* and replace with null-termination */
	    }
	}
	else flist = NULL;
	*npaths = rep.nPaths;
	UnlockDisplay(dpy);
	SyncHandle();
	return (flist);
}

int
XFreeFontPath (list)
char **list;
{
	if (list != NULL) {
		Xfree (list[0]-1);
		Xfree ((char *)list);
	}
	return 1;
}
