/* $TOG: ListExt.c /main/9 1998/02/06 17:42:15 kaleb $ */
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
/* $XFree86: xc/lib/X11/ListExt.c,v 1.2 1999/05/09 10:49:43 dawes Exp $ */

#define NEED_REPLIES
#include "Xlibint.h"

char **XListExtensions(dpy, nextensions)
register Display *dpy;
int *nextensions;	/* RETURN */
{
	xListExtensionsReply rep;
	char **list;
	char *ch;
	register unsigned i;
	register int length;
	register xReq *req;
	register long rlen;

	LockDisplay(dpy);
	GetEmptyReq (ListExtensions, req);

	if (! _XReply (dpy, (xReply *) &rep, 0, xFalse)) {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (char **) NULL;
	}

	if (rep.nExtensions) {
	    list = (char **) Xmalloc (
                (unsigned)(rep.nExtensions * sizeof (char *)));
	    rlen = rep.length << 2;
	    ch = (char *) Xmalloc ((unsigned) rlen + 1);
                /* +1 to leave room for last null-terminator */

	    if ((!list) || (!ch)) {
		if (list) Xfree((char *) list);
		if (ch)   Xfree((char *) ch);
		_XEatData(dpy, (unsigned long) rlen);
		UnlockDisplay(dpy);
		SyncHandle();
		return (char **) NULL;
	    }

	    _XReadPad (dpy, ch, rlen);
	    /*
	     * unpack into null terminated strings.
	     */
	    length = *ch;
	    for (i = 0; i < rep.nExtensions; i++) {
		list[i] = ch+1;  /* skip over length */
		ch += length + 1; /* find next length ... */
		length = *ch;
		*ch = '\0'; /* and replace with null-termination */
	    }
	}
	else list = (char **) NULL;

	*nextensions = rep.nExtensions;
	UnlockDisplay(dpy);
	SyncHandle();
	return (list);
}

int
XFreeExtensionList (list)
char **list;
{
	if (list != NULL) {
	    Xfree (list[0]-1);
	    Xfree ((char *)list);
	}
	return 1;
}
