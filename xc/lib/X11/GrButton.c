/* $TOG: GrButton.c /main/6 1998/02/06 17:29:47 kaleb $ */
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
/* $XFree86: xc/lib/X11/GrButton.c,v 1.2 1999/05/09 10:49:29 dawes Exp $ */

#include "Xlibint.h"

int
XGrabButton(dpy, button, modifiers, grab_window, owner_events, event_mask,
	    pointer_mode, keyboard_mode, confine_to, curs)
register Display *dpy;
unsigned int modifiers; /* CARD16 */
unsigned int button; /* CARD8 */
Window grab_window;
Bool owner_events;
unsigned int event_mask; /* CARD16 */
int pointer_mode, keyboard_mode;
Window confine_to;
Cursor curs;
{
    register xGrabButtonReq *req;
    LockDisplay(dpy);
    GetReq(GrabButton, req);
    req->modifiers = modifiers;
    req->button = button;
    req->grabWindow = grab_window;
    req->ownerEvents = owner_events;
    req->eventMask = event_mask;
    req->pointerMode = pointer_mode;
    req->keyboardMode = keyboard_mode;
    req->confineTo = confine_to;
    req->cursor = curs;
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
