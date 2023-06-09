/* $TOG: renderers.c /main/6 1998/02/09 10:48:53 kaleb $ */

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
/* $XFree86: xc/lib/font/fontfile/renderers.c,v 1.2 1999/07/17 05:30:43 dawes Exp $ */

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#include "fntfilst.h"

static FontRenderersRec	renderers;

Bool
FontFileRegisterRenderer (FontRendererPtr renderer)
{
    int		    i;
    FontRendererPtr *new;

    for (i = 0; i < renderers.number; i++)
	if (!strcmp (renderers.renderers[i]->fileSuffix, renderer->fileSuffix))
	    return TRUE;
    i = renderers.number + 1;
    new = (FontRendererPtr *) xrealloc (renderers.renderers, sizeof *new * i);
    if (!new)
	return FALSE;
    renderer->number = i - 1;
    renderers.renderers = new;
    renderers.renderers[i - 1] = renderer;
    renderers.number = i;
    return TRUE;
}

FontRendererPtr
FontFileMatchRenderer (char *fileName)
{
    int			i;
    int			fileLen;
    FontRendererPtr	r;
    
    fileLen = strlen (fileName);
    for (i = 0; i < renderers.number; i++)
    {
	r = renderers.renderers[i];
	if (fileLen >= r->fileSuffixLen &&
	    !strcmp (fileName + fileLen - r->fileSuffixLen, r->fileSuffix))
	{
	    return r;
	}
    }
    return 0;
}
