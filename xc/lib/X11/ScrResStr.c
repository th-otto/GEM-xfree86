/* $TOG: ScrResStr.c /main/4 1998/02/06 17:50:58 kaleb $ */
/*

Copyright 1991, 1998  The Open Group

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
#include <X11/Xatom.h>

char *XScreenResourceString(screen)
	Screen *screen;
{
    Atom prop_name;
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long leftover;
    char *val = NULL;

    prop_name = XInternAtom(screen->display, "SCREEN_RESOURCES", True);
    if (prop_name &&
	XGetWindowProperty(screen->display, screen->root, prop_name,
			   0L, 100000000L, False,
			   XA_STRING, &actual_type, &actual_format,
			   &nitems, &leftover,
			   (unsigned char **) &val) == Success) {
	if ((actual_type == XA_STRING) && (actual_format == 8))
	    return val;
	if (val)
	    Xfree(val);
    }
    return (char *)NULL;
}
