/* $TOG: fileio.c /main/7 1998/05/01 16:42:56 kaleb $ */

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
/* $XFree86: xc/lib/font/fontfile/fileio.c,v 3.7 1999/08/21 13:48:02 dawes Exp $ */

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#include <fntfilio.h>
#include <X11/Xos.h>
#ifndef O_BINARY
#define O_BINARY O_RDONLY
#endif

FontFilePtr
FontFileOpen (const char *name)
{
    int		fd;
    int		len;
    BufFilePtr	raw, cooked;

    fd = open (name, O_BINARY);
    if (fd < 0)
	return 0;
    raw = BufFileOpenRead (fd);
    if (!raw)
    {
	close (fd);
	return 0;
    }
    len = strlen (name);
#ifndef __EMX__
    if (len > 2 && !strcmp (name + len - 2, ".Z")) {
#else
    if (len > 2 && (!strcmp (name + len - 4, ".pcz") || 
		    !strcmp (name + len - 2, ".Z"))) {
#endif
	cooked = BufFilePushCompressed (raw);
	if (!cooked) {
	    BufFileClose (raw, TRUE);
	    return 0;
	}
	raw = cooked;
#ifdef X_GZIP_FONT_COMPRESSION
    } else if (len > 3 && !strcmp (name + len - 3, ".gz")) {
	cooked = BufFilePushZIP (raw);
	if (!cooked) {
	    BufFileClose (raw, TRUE);
	    return 0;
	}
	raw = cooked;
#endif
    }
    return (FontFilePtr) raw;
}

int
FontFileClose (FontFilePtr f)
{
    return BufFileClose ((BufFilePtr) f, TRUE);
}

