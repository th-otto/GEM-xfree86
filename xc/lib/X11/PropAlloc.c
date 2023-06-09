/* $TOG: PropAlloc.c /main/7 1998/02/06 17:47:38 kaleb $ */
/*

Copyright 1989, 1998  The Open Group

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
/* $XFree86: xc/lib/X11/PropAlloc.c,v 1.2 1999/05/09 10:49:55 dawes Exp $ */

#include "Xlibint.h"
#include "Xutil.h"
#include <stdio.h>


/*
 * Routines for allocating space for structures that are expected to get 
 * longer at some point.
 */

XSizeHints *XAllocSizeHints ()
{
    return ((XSizeHints *) Xcalloc (1, (unsigned) sizeof (XSizeHints)));
}


XStandardColormap *XAllocStandardColormap ()
{
    return ((XStandardColormap *)
	    Xcalloc (1, (unsigned) sizeof (XStandardColormap)));
}


XWMHints *XAllocWMHints ()
{
    return ((XWMHints *) Xcalloc (1, (unsigned) sizeof (XWMHints)));
}


XClassHint *XAllocClassHint ()
{
    register XClassHint *h;

    if ((h = (XClassHint *) Xcalloc (1, (unsigned) sizeof (XClassHint))))
      h->res_name = h->res_class = NULL;

    return h;
}


XIconSize *XAllocIconSize ()
{
    return ((XIconSize *) Xcalloc (1, (unsigned) sizeof (XIconSize)));
}


