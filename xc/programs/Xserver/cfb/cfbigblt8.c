/*
 * $TOG: cfbigblt8.c /main/10 1998/02/09 14:05:59 kaleb $
 *
Copyright 1990, 1998  The Open Group

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
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/* $XFree86: xc/programs/Xserver/cfb/cfbigblt8.c,v 1.3 1998/10/04 09:37:44 dawes Exp $ */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"mi.h"
#include	"cfb.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"

void
cfbImageGlyphBlt8 (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr	    pDrawable;
    GCPtr	    pGC;
    int		    x, y;
    unsigned int    nglyph;
    CharInfoPtr	    *ppci;
    pointer	    pglyphBase;
{
    ExtentInfoRec info;		/* used by QueryGlyphExtents() */
    xRectangle backrect;
    int		fillStyle;
    int		alu;
    int		fgPixel;
    int		rop;
    int		xor;
    int		and;
    int		pm;
    cfbPrivGC	    *priv;

    /*
     * We can't avoid GC validations if calling mi functions.
     */
    if ((pGC->ops->PolyFillRect == miPolyFillRect) ||
        (pGC->ops->PolyGlyphBlt == miPolyGlyphBlt))
    {
        miImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
        return;
    }

    QueryGlyphExtents(pGC->font, ppci, (unsigned long)nglyph, &info);

    if (info.overallWidth >= 0)
    {
    	backrect.x = x;
    	backrect.width = info.overallWidth;
    }
    else
    {
	backrect.x = x + info.overallWidth;
	backrect.width = -info.overallWidth;
    }
    backrect.y = y - FONTASCENT(pGC->font);
    backrect.height = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);

    priv = cfbGetGCPrivate(pGC);

    /* this code cheats by knowing that ValidateGC isn't
     * necessary for PolyFillRect
     */

    fgPixel = pGC->fgPixel;

    pGC->fgPixel = pGC->bgPixel;
    priv->xor = PFILL(pGC->bgPixel);

    (*pGC->ops->PolyFillRect) (pDrawable, pGC, 1, &backrect);

    pGC->fgPixel = fgPixel;

    priv->xor = PFILL(pGC->fgPixel);

    (*pGC->ops->PolyGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    
}
