/* $TOG: NextEvent.c /main/8 1998/02/06 17:44:53 kaleb $ */
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
/* $XFree86: xc/lib/X11/NextEvent.c,v 1.2 1999/05/09 10:49:51 dawes Exp $ */

#define NEED_EVENTS
#include "Xlibint.h"

/* 
 * Return next event in queue, or if none, flush output and wait for
 * events.
 */

int
XNextEvent (dpy, event)
	register Display *dpy;
	register XEvent *event;
{
	register _XQEvent *qelt;
	
	LockDisplay(dpy);
	
	if (dpy->head == NULL)
	    _XReadEvents(dpy);
	qelt = dpy->head;
	*event = qelt->event;
	_XDeq(dpy, NULL, qelt);
	UnlockDisplay(dpy);
	return 0;
}

