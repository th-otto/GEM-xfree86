/* $TOG: AuFileName.c /main/9 1998/02/06 14:14:36 kaleb $ */

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
/* $XFree86: xc/lib/Xau/AuFileName.c,v 3.4 1999/05/09 10:51:32 dawes Exp $ */

#include <X11/Xauth.h>
#include <X11/Xos.h>

#ifdef X_NOT_STDC_ENV
char *malloc (), *getenv ();
#else
#include <stdlib.h>
#endif

char *
XauFileName ()
{
    char *slashDotXauthority = "/.Xauthority";
    char    *name;
    static char	*buf;
    static int	bsize;
#ifdef WIN32
    char    dir[128];
#endif
    int	    size;

    if ((name = getenv ("XAUTHORITY")))
	return name;
    name = getenv ("HOME");
    if (!name) {
#ifdef WIN32
	(void) strcpy (dir, "/users/");
	if (name = getenv("USERNAME")) {
	    (void) strcat (dir, name);
	    name = dir;
	}
	if (!name)
#endif
	return 0;
    }
    size = strlen (name) + strlen(&slashDotXauthority[1]) + 2;
    if (size > bsize) {
	if (buf)
	    free (buf);
	buf = malloc ((unsigned) size);
	if (!buf)
	    return 0;
	bsize = size;
    }
    strcpy (buf, name);
    strcat (buf, slashDotXauthority + (name[1] == '\0' ? 1 : 0));
    return buf;
}
