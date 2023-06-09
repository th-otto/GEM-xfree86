/* $XFree86: xc/programs/Xserver/hw/xfree86/vgafb/vgapush8.c,v 1.2 1998/07/25 16:58:22 dawes Exp $ */
/*

Copyright (c) 1989  X Consortium

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
*/
/* $XConsortium: vgapush8.c /main/3 1996/02/21 18:11:39 kaleb $ */

/*
 * Push Pixels for 8 bit displays.
 */

#include	"vgafb.h"

#define MFB_CONSTS_ONLY
#include "maskbits.h"

void
vga256PushPixels8 (pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr	pGC;
    PixmapPtr	pBitmap;
    DrawablePtr	pDrawable;
    int		dx, dy, xOrg, yOrg;
{
    register unsigned long   *src, *dst;
    register unsigned long   pixel;
    register unsigned long   c, bits;
    unsigned long   *pdstLine, *psrcLine;
    unsigned long   *pdstBase;
    int		    srcWidth;
    int		    dstWidth;
    int		    xoff;
    int		    nBitmapLongs, nPixmapLongs;
    int		    nBitmapTmp, nPixmapTmp;
    unsigned long   rightMask;
    BoxRec	    bbox;
    cfbPrivGCPtr    devPriv;

    bbox.x1 = xOrg;
    bbox.y1 = yOrg;
    bbox.x2 = bbox.x1 + dx;
    bbox.y2 = bbox.y1 + dy;
    devPriv = cfbGetGCPrivate(pGC);
    
    switch (RECT_IN_REGION(pGC->pScreen, pGC->pCompositeClip, &bbox))
    {
      case rgnPART:
	mfbPushPixels(pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg);
      case rgnOUT:
	return;
    }
    /*
     * If nothing is going to actually touch the framebuffer, then
     * do the work with the cfb routines as they should be slightly faster.
     */

    if( pDrawable->type != DRAWABLE_WINDOW )
	{
	cfbPushPixels8 (pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg);
	return;
	}

    cfbGetLongWidthAndPointer (pDrawable, dstWidth, pdstBase)

    BANK_FLAG(pdstBase)

    psrcLine = (unsigned long *) pBitmap->devPrivate.ptr;
    srcWidth = (int) pBitmap->devKind >> PWSH;
    
    pixel = devPriv->xor;
    xoff = xOrg & PIM;
    nBitmapLongs = (dx + xoff) >> MFB_PWSH;
    nPixmapLongs = (dx + PGSZB + xoff) >> PWSH;

    rightMask = ~cfb8BitLenMasks[((dx + xoff) & MFB_PIM)];

    pdstLine = pdstBase + (yOrg * dstWidth) + (xOrg >> PWSH);

    while (dy--)
    {
	c = 0;
	nPixmapTmp = nPixmapLongs;
	nBitmapTmp = nBitmapLongs;
	src = psrcLine;
	dst = pdstLine;
	SETRW(dst);
	while (nBitmapTmp--)
	{
	    bits = *src++;
	    c |= BitRight (bits, xoff);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	    nPixmapTmp -= 8;
	    c = 0;
	    if (xoff)
		c = BitLeft (bits, PGSZ - xoff);
	}
	if (BitLeft (rightMask, xoff))
	    c |= BitRight (*src, xoff);
	c &= rightMask;
	switch (nPixmapTmp) {
	case 8:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 7:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 6:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 5:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 4:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 3:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 2:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 1:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++; CHECKRWO(dst);
	case 0:
	    break;
	}
	pdstLine += dstWidth;
	psrcLine += srcWidth;
    }
}
