/*
 * Push Pixels for 8 bit displays.
 */

/* $XFree86: xc/programs/Xserver/cfb/cfbpush8.c,v 1.4 2000/02/12 03:39:29 dawes Exp $ */

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
/* $TOG: cfbpush8.c /main/18 1998/02/09 14:07:04 kaleb $ */

#if PSZ == 8

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"cfb.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"
#define MFB_CONSTS_ONLY
#include	"maskbits.h"

void
cfbPushPixels8 (pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr	pGC;
    PixmapPtr	pBitmap;
    DrawablePtr	pDrawable;
    int		dx, dy, xOrg, yOrg;
{
    register CfbBits   *src, *dst;
    register CfbBits   pixel;
    register CfbBits   c, bits;
    CfbBits   *pdstLine, *psrcLine;
    CfbBits   *pdstBase;
    int		    srcWidth;
    int		    dstWidth;
    int		    xoff;
    int		    nBitmapLongs, nPixmapLongs;
    int		    nBitmapTmp, nPixmapTmp;
    CfbBits   rightMask;
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
#ifndef LOWMEMFTPT
	mfbPushPixels(pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg);
#else
	miPushPixels(pGC, pBitmap, pDrawable, dx, dy, xOrg, yOrg);
#endif /* ifndef LOWMEMFTPT */
      case rgnOUT:
	return;
    }

    cfbGetLongWidthAndPointer (pDrawable, dstWidth, pdstBase)

    psrcLine = (CfbBits *) pBitmap->devPrivate.ptr;
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
	while (nBitmapTmp--)
	{
	    bits = *src++;
	    c |= BitRight (bits, xoff);
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
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
	    dst++;
	case 7:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	case 6:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	case 5:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	case 4:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	case 3:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	case 2:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	case 1:
	    WriteBitGroup(dst, pixel, GetBitGroup(c));
	    NextBitGroup(c);
	    dst++;
	case 0:
	    break;
	}
	pdstLine += dstWidth;
	psrcLine += srcWidth;
    }
}

#endif
