/*
 * $TOG: Alloc.c /main/3 1998/02/06 14:38:48 kaleb $
 *
 * 
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
 * *
 * Author:  Keith Packard, MIT X Consortium
 */

/* $XFree86: xc/lib/Xdmcp/Alloc.c,v 3.2 1998/10/10 15:25:13 dawes Exp $ */

/* stubs for use when Xalloc, Xrealloc and Xfree are not defined */

#include <X11/Xos.h>
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xdmcp.h>

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
char *malloc(size_t), *realloc(char *, size_t);
#endif

void *
Xalloc (unsigned long amount)
{
    if (amount == 0)
	amount = 1;
    return malloc (amount);
}

void *
Xrealloc (void *old, unsigned long amount)
{
    if (amount == 0)
	amount = 1;
    if (!old)
	return malloc (amount);
    return realloc ((char *) old, amount);
}

void
Xfree (void *old)
{
    if (old)
	free ((char *) old);
}
