/* $TOG: Misc.c /main/8 1998/02/06 17:44:35 kaleb $ */
/*

Copyright 1988, 1998  The Open Group

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
/* $XFree86: xc/lib/X11/Misc.c,v 1.2 1999/05/09 10:49:49 dawes Exp $ */

#include "Xlibint.h"

long XMaxRequestSize(dpy)
    Display *dpy;
{
    return dpy->max_request_size;
}

char *XResourceManagerString(dpy)
    Display *dpy;
{
    return dpy->xdefaults;
}

unsigned long XDisplayMotionBufferSize(dpy)
    Display *dpy;
{
    return dpy->motion_buffer;
}

int
XDisplayKeycodes(dpy, min_keycode_return, max_keycode_return)
    Display *dpy;
    int *min_keycode_return, *max_keycode_return;
{
    *min_keycode_return = dpy->min_keycode;
    *max_keycode_return = dpy->max_keycode;
    return 1;
}

VisualID XVisualIDFromVisual(visual)
    Visual *visual;
{
    return visual->visualid;
}

long XExtendedMaxRequestSize(dpy)
    Display *dpy;
{
    return dpy->bigreq_size;
}
