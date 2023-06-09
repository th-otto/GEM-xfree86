/* $TOG: IfEvent.c /main/9 1998/02/06 17:33:19 kaleb $ */
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
/* $XFree86: xc/lib/X11/IfEvent.c,v 1.2 1999/05/09 10:49:35 dawes Exp $ */

#define NEED_EVENTS
#include "Xlibint.h"

/* 
 * Flush output and (wait for and) return the next event matching the
 * predicate in the queue.
 */

int
XIfEvent (dpy, event, predicate, arg)
	register Display *dpy;
	Bool (*predicate)(
#if NeedNestedPrototypes
			  Display*			/* display */,
			  XEvent*			/* event */,
			  char*				/* arg */
#endif
			  );		/* function to call */
	register XEvent *event;
	char *arg;
{
	register _XQEvent *qelt, *prev;
	unsigned long qe_serial = 0;
	
        LockDisplay(dpy);
	prev = NULL;
	while (1) {
	    for (qelt = prev ? prev->next : dpy->head;
		 qelt;
		 prev = qelt, qelt = qelt->next) {
		if(qelt->qserial_num > qe_serial
		   && (*predicate)(dpy, &qelt->event, arg)) {
		    *event = qelt->event;
		    _XDeq(dpy, prev, qelt);
		    UnlockDisplay(dpy);
		    return 0;
		}
	    }
	    if (prev)
		qe_serial = prev->qserial_num;
	    _XReadEvents(dpy);
	    if (prev && prev->qserial_num != qe_serial)
		/* another thread has snatched this event */
		prev = NULL;
	}
}
