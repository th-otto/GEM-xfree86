/* $XFree86: xc/programs/Xserver/hw/xfree86/vgafb/vgaimage.c,v 1.2 1998/07/25 16:58:20 dawes Exp $ */
/***********************************************************

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XConsortium: vgaimage.c /main/3 1996/02/21 18:11:16 kaleb $ */

#include "vgafb.h"

void
vga256GetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine)
    DrawablePtr pDrawable;
    int		sx, sy, w, h;
    unsigned int format;
    unsigned long planeMask;
    char	*pdstLine;
{
    BoxRec box;
    DDXPointRec ptSrc;
    RegionRec rgnDst;
    ScreenPtr pScreen;
    PixmapPtr pPixmap;

    if ((w == 0) || (h == 0))
	return;
    if (pDrawable->bitsPerPixel == 1)
    {
	mfbGetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
	return;
    }
    pScreen = pDrawable->pScreen;
    if (format == ZPixmap)
    {
	pPixmap = GetScratchPixmapHeader(pScreen, w, h, 
			pDrawable->depth, pDrawable->bitsPerPixel,
			PixmapBytePad(w,pDrawable->depth), (pointer)pdstLine);
	if (!pPixmap)
	    return;
	if ((planeMask & PMSK) != PMSK)
	    xf86memset((char *)pdstLine,0, pPixmap->devKind * h);
        ptSrc.x = sx + pDrawable->x;
        ptSrc.y = sy + pDrawable->y;
        box.x1 = 0;
        box.y1 = 0;
        box.x2 = w;
        box.y2 = h;
        REGION_INIT(pScreen, &rgnDst, &box, 1);
	vga256DoBitblt(pDrawable, (DrawablePtr)pPixmap, GXcopy, &rgnDst,
		    &ptSrc, planeMask);
        REGION_UNINIT(pScreen, &rgnDst);
	FreeScratchPixmapHeader(pPixmap);
    }
    else
    {
	pPixmap = GetScratchPixmapHeader(pScreen, w, h,  /*depth*/ 1,
			/*bpp*/ 1, BitmapBytePad(w), (pointer)pdstLine);
	if (!pPixmap)
	    return;

        ptSrc.x = sx + pDrawable->x;
        ptSrc.y = sy + pDrawable->y;
        box.x1 = 0;
        box.y1 = 0;
        box.x2 = w;
        box.y2 = h;
        REGION_INIT(pScreen, &rgnDst, &box, 1);
	vga256CopyImagePlane (pDrawable, (DrawablePtr)pPixmap, GXcopy, &rgnDst,
		    &ptSrc, planeMask);
        REGION_UNINIT(pScreen, &rgnDst);
	FreeScratchPixmapHeader(pPixmap);
    }
}
