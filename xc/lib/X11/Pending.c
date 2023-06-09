/* $TOG: Pending.c /main/7 1998/02/06 17:46:45 kaleb $ */
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

#include "Xlibint.h"

/* Read in pending events if needed and return the number of queued events. */

int XEventsQueued (dpy, mode)
    register Display *dpy;
    int mode;
{
    int ret_val;
    LockDisplay(dpy);
    if (dpy->qlen || (mode == QueuedAlready))
	ret_val = dpy->qlen;
    else
	ret_val = _XEventsQueued (dpy, mode);
    UnlockDisplay(dpy);
    return ret_val;
}

int XPending (dpy)
    register Display *dpy;
{
    int ret_val;
    LockDisplay(dpy);
    if (dpy->qlen)
	ret_val = dpy->qlen;
    else
	ret_val = _XEventsQueued (dpy, QueuedAfterFlush);
    UnlockDisplay(dpy);
    return ret_val;
}
