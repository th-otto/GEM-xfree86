/* $XFree86: xc/programs/Xserver/afb/afbline.c,v 3.1 1998/03/20 21:04:55 hohndel Exp $ */
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
/* $XConsortium: afbline.c,v 5.18 94/04/17 20:28:26 dpw Exp $ */

#include "X.h"

#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "mistruct.h"

#include "afb.h"
#include "maskbits.h"
#include "miline.h"

/* single-pixel lines on a color frame buffer

   NON-SLOPED LINES
   horizontal lines are always drawn left to right; we have to
move the endpoints right by one after they're swapped.
   horizontal lines will be confined to a single band of a
region.  the code finds that band (giving up if the lower
bound of the band is above the line we're drawing); then it
finds the first box in that band that contains part of the
line.  we clip the line to subsequent boxes in that band.
   vertical lines are always drawn top to bottom (y-increasing.)
this requires adding one to the y-coordinate of each endpoint
after swapping.

   SLOPED LINES
   when clipping a sloped line, we bring the second point inside
the clipping box, rather than one beyond it, and then add 1 to
the length of the line before drawing it.  this lets us use
the same box for finding the outcodes for both endpoints.  since
the equation for clipping the second endpoint to an edge gives us
1 beyond the edge, we then have to move the point towards the
first point by one step on the major axis.
   eventually, there will be a diagram here to explain what's going
on.  the method uses Cohen-Sutherland outcodes to determine
outsideness, and a method similar to Pike's layers for doing the
actual clipping.

*/

void
#ifdef POLYSEGMENT
afbSegmentSS(pDrawable, pGC, nseg, pSeg)
	DrawablePtr		pDrawable;
	GCPtr		pGC;
	int				nseg;
	register xSegment		*pSeg;
#else
afbLineSS(pDrawable, pGC, mode, npt, pptInit)
	DrawablePtr pDrawable;
	GCPtr		pGC;
	int				mode;				/* Origin or Previous */
	int				npt;				/* number of points */
	DDXPointPtr pptInit;
#endif
{
	int nboxInit;
	register int nbox;
	BoxPtr pboxInit;
	register BoxPtr pbox;
#ifndef POLYSEGMENT
	register DDXPointPtr ppt;		/* pointer to list of translated points */
#endif

	unsigned int oc1;				/* outcode of point 1 */
	unsigned int oc2;				/* outcode of point 2 */

	PixelType *addrlBase;		/* pointer to start of drawable */
	PixelType *addrl;				/* address of destination pixmap */
	int nlwidth;				/* width in longwords of destination pixmap */
	int xorg, yorg;				/* origin of window */

	int adx;				/* abs values of dx and dy */
	int ady;
	int signdx;				/* sign of dx and dy */
	int signdy;
	int e, e1, e2;				/* bresenham error and increments */
	int len;						/* length of segment */
	int axis;						/* major axis */
	int octant;
	unsigned int bias = miGetZeroLineBias(pDrawable->pScreen);
	int depthDst;
	int d;
	int sizeDst;
	unsigned char *rrops;

								/* a bunch of temporaries */
	register int y1, y2;
	register int x1, x2;
	RegionPtr cclip;

	cclip = pGC->pCompositeClip;
	rrops = ((afbPrivGC *)(pGC->devPrivates[afbGCPrivateIndex].ptr))->rrops;
	pboxInit = REGION_RECTS(cclip);
	nboxInit = REGION_NUM_RECTS(cclip);

	afbGetPixelWidthSizeDepthAndPointer(pDrawable, nlwidth, sizeDst, depthDst,
													 addrlBase);

	xorg = pDrawable->x;
	yorg = pDrawable->y;
#ifdef POLYSEGMENT
	while (nseg--)
#else
	ppt = pptInit;
	x2 = ppt->x + xorg;
	y2 = ppt->y + yorg;
	while(--npt)
#endif
	{
		nbox = nboxInit;
		pbox = pboxInit;

#ifdef POLYSEGMENT
		x1 = pSeg->x1 + xorg;
		y1 = pSeg->y1 + yorg;
		x2 = pSeg->x2 + xorg;
		y2 = pSeg->y2 + yorg;
		pSeg++;
#else
		x1 = x2;
		y1 = y2;
		++ppt;
		if (mode == CoordModePrevious) {
			xorg = x1;
			yorg = y1;
		}
		x2 = ppt->x + xorg;
		y2 = ppt->y + yorg;
#endif

		if (x1 == x2) /* vertical line */
		{
			/* make the line go top to bottom of screen, keeping
			   endpoint semantics
			*/
			if (y1 > y2) {
				register int tmp;

				tmp = y2;
				y2 = y1 + 1;
				y1 = tmp + 1;
#ifdef POLYSEGMENT
				if (pGC->capStyle != CapNotLast)
					y1--;
#endif
			}
#ifdef POLYSEGMENT
			else if (pGC->capStyle != CapNotLast)
				y2++;
#endif
			/* get to first band that might contain part of line */
			while ((nbox) && (pbox->y2 <= y1)) {
				pbox++;
				nbox--;
			}

			if (nbox) {
				/* stop when lower edge of box is beyond end of line */
				while((nbox) && (y2 >= pbox->y1)) {
					if ((x1 >= pbox->x1) && (x1 < pbox->x2)) {
						int y1t, y2t;
						/* this box has part of the line in it */
						y1t = max(y1, pbox->y1);
						y2t = min(y2, pbox->y2);
						if (y1t != y2t)
							afbVertS(addrlBase, nlwidth, sizeDst, depthDst, x1, y1t,
										 y2t-y1t, rrops);	/* @@@ NEXT PLANE PASSED @@@ */
					}
					nbox--;
					pbox++;
				}
			}
#ifndef POLYSEGMENT
			y2 = ppt->y + yorg;
#endif
		} else if (y1 == y2) /* horizontal line */ {
			/* force line from left to right, keeping
			   endpoint semantics
			*/
			if (x1 > x2) {
				register int tmp;

				tmp = x2;
				x2 = x1 + 1;
				x1 = tmp + 1;
#ifdef POLYSEGMENT
				if (pGC->capStyle != CapNotLast)
					x1--;
#endif
			}
#ifdef POLYSEGMENT
			else if (pGC->capStyle != CapNotLast)
				x2++;
#endif

			/* find the correct band */
			while( (nbox) && (pbox->y2 <= y1)) {
				pbox++;
				nbox--;
			}

			/* try to draw the line, if we haven't gone beyond it */
			if ((nbox) && (pbox->y1 <= y1)) {
				int tmp;

				/* when we leave this band, we're done */
				tmp = pbox->y1;
				while((nbox) && (pbox->y1 == tmp)) {
					int		x1t, x2t;

					if (pbox->x2 <= x1) {
						/* skip boxes until one might contain start point */
						nbox--;
						pbox++;
						continue;
					}

					/* stop if left of box is beyond right of line */
					if (pbox->x1 >= x2) {
						nbox = 0;
						break;
					}

					x1t = max(x1, pbox->x1);
					x2t = min(x2, pbox->x2);
					if (x1t != x2t)
						afbHorzS(addrlBase, nlwidth, sizeDst, depthDst, x1t, y1,
									 x2t-x1t, rrops);	/* @@@ NEXT PLANE PASSED @@@ */
					nbox--;
					pbox++;
				}
			}
#ifndef POLYSEGMENT
			x2 = ppt->x + xorg;
#endif
		}
		else		/* sloped line */
		{
			CalcLineDeltas(x1, y1, x2, y2, adx, ady,
				       signdx, signdy, 1, 1, octant);

			if (adx > ady) {
				axis = X_AXIS;
				e1 = ady << 1;
				e2 = e1 - (adx << 1);
				e = e1 - adx;
			} else {
				axis = Y_AXIS;
				e1 = adx << 1;
				e2 = e1 - (ady << 1);
				e = e1 - ady;
				SetYMajorOctant(octant);
			}

			FIXUP_ERROR(e, octant, bias);

			/* we have bresenham parameters and two points.
			   all we have to do now is clip and draw.
			*/

			while(nbox--) {
				oc1 = 0;
				oc2 = 0;
				OUTCODES(oc1, x1, y1, pbox);
				OUTCODES(oc2, x2, y2, pbox);
				if ((oc1 | oc2) == 0) {
					if (axis == X_AXIS)
						len = adx;
					else
						len = ady;
#ifdef POLYSEGMENT
					if (pGC->capStyle != CapNotLast)
						len++;
#endif
					afbBresS(addrlBase, nlwidth, sizeDst, depthDst, signdx, signdy,
								 axis, x1, y1, e, e1, e2, len, rrops);	/* @@@ NEXT PLANE PASSED @@@ */
					break;
				} else if (oc1 & oc2) {
					pbox++;
				} else {
					int new_x1 = x1, new_y1 = y1, new_x2 = x2, new_y2 = y2;
					int clip1 = 0, clip2 = 0;
					int clipdx, clipdy;
					int err;

					if (miZeroClipLine(pbox->x1, pbox->y1, pbox->x2-1,
									   pbox->y2-1,
									   &new_x1, &new_y1, &new_x2, &new_y2,
									   adx, ady, &clip1, &clip2,
									   octant, bias, oc1, oc2) == -1) {
						pbox++;
						continue;
					}

					if (axis == X_AXIS)
						len = abs(new_x2 - new_x1);
					else
						len = abs(new_y2 - new_y1);
#ifdef POLYSEGMENT
					if (clip2 != 0 || pGC->capStyle != CapNotLast)
						len++;
#else
					len += (clip2 != 0);
#endif
					if (len) {
						/* unwind bresenham error term to first point */
						if (clip1) {
							clipdx = abs(new_x1 - x1);
							clipdy = abs(new_y1 - y1);
							if (axis == X_AXIS)
								err = e+((clipdy*e2) + ((clipdx-clipdy)*e1));
							else
								err = e+((clipdx*e2) + ((clipdy-clipdx)*e1));
						}
						else
							err = e;
						afbBresS(addrlBase, nlwidth, sizeDst, depthDst, signdx,
									 signdy, axis, new_x1, new_y1, err, e1, e2, len,
									 rrops);	/* @@@ NEXT PLANE PASSED @@@ */
					}
					pbox++;
				}
			} /* while (nbox--) */
		} /* sloped line */
	} /* while (nline--) */

#ifndef POLYSEGMENT

	/* paint the last point if the end style isn't CapNotLast.
	   (Assume that a projecting, butt, or round cap that is one
		pixel wide is the same as the single pixel of the endpoint.)
	*/

	if ((pGC->capStyle != CapNotLast) &&
		 ((ppt->x + xorg != pptInit->x + pDrawable->x) ||
		  (ppt->y + yorg != pptInit->y + pDrawable->y) ||
		  (ppt == pptInit + 1))) {
		nbox = nboxInit;
		pbox = pboxInit;
		while (nbox--) {
			if ((x2 >= pbox->x1) && (y2 >= pbox->y1) && (x2 < pbox->x2) &&
				 (y2 < pbox->y2)) {
				for (d = 0; d < depthDst; d++) {
					addrl = afbScanline(addrlBase, x2, y2, nlwidth);
					addrlBase += sizeDst;			/* @@@ NEXT PLANE @@@ */

					switch(rrops[d]) {
						case RROP_BLACK:
							*addrl &= rmask[x2 & PIM];
							break;
						case RROP_WHITE:
							*addrl |= mask[x2 & PIM];
							break;
						case RROP_INVERT:
							*addrl ^= mask[x2 & PIM];
							break;
						case RROP_NOP:
							break;
					} /* switch */
				} /* for (d = ...) */
				break;
			} else
				pbox++;
		}
	}
#endif
}

/*
 * Draw dashed 1-pixel lines.
 */

void
#ifdef POLYSEGMENT
afbSegmentSD(pDrawable, pGC, nseg, pSeg)
	DrawablePtr		pDrawable;
	register GCPtr		pGC;
	int				nseg;
	register xSegment		*pSeg;
#else
afbLineSD(pDrawable, pGC, mode, npt, pptInit)
	DrawablePtr pDrawable;
	register GCPtr pGC;
	int mode;				/* Origin or Previous */
	int npt;				/* number of points */
	DDXPointPtr pptInit;
#endif
{
	int nboxInit;
	register int nbox;
	BoxPtr pboxInit;
	register BoxPtr pbox;
#ifndef POLYSEGMENT
	register DDXPointPtr ppt;		/* pointer to list of translated points */
#endif

	register unsigned int oc1;		/* outcode of point 1 */
	register unsigned int oc2;		/* outcode of point 2 */

	PixelType *addrlBase;		/* address of destination pixmap */
	PixelType *addrl;
	int nlwidth;				/* width in longwords of destination pixmap */
	int sizeDst;
	int depthDst;
	int xorg, yorg;				/* origin of window */

	int adx;				/* abs values of dx and dy */
	int ady;
	int signdx;				/* sign of dx and dy */
	int signdy;
	int e, e1, e2;				/* bresenham error and increments */
	int len;						/* length of segment */
	int axis;						/* major axis */
	int octant;
	unsigned int bias = miGetZeroLineBias(pDrawable->pScreen);
	int x1, x2, y1, y2;
	RegionPtr cclip;
	unsigned char *rrops;
	unsigned char bgrrops[AFB_MAX_DEPTH];
	unsigned char   *pDash;
	int					dashOffset;
	int					numInDashList;
	int					dashIndex;
	int					isDoubleDash;
	int					dashIndexTmp, dashOffsetTmp;
	int					unclippedlen;
	int					d;

	cclip = pGC->pCompositeClip;
	rrops = ((afbPrivGC *)(pGC->devPrivates[afbGCPrivateIndex].ptr))->rrops;
	pboxInit = REGION_RECTS(cclip);
	nboxInit = REGION_NUM_RECTS(cclip);

   afbGetPixelWidthSizeDepthAndPointer(pDrawable, nlwidth, sizeDst, depthDst,
													 addrlBase);

	/* compute initial dash values */

	pDash = (unsigned char *) pGC->dash;
	numInDashList = pGC->numInDashList;
	isDoubleDash = (pGC->lineStyle == LineDoubleDash);
	dashIndex = 0;
	dashOffset = 0;
	miStepDash ((int)pGC->dashOffset, &dashIndex, pDash,
				numInDashList, &dashOffset);

	if (isDoubleDash)
		afbReduceRop (pGC->alu, pGC->bgPixel, pGC->planemask, pGC->depth,
					  bgrrops);

	xorg = pDrawable->x;
	yorg = pDrawable->y;
#ifdef POLYSEGMENT
	while (nseg--)
#else
	ppt = pptInit;
	x2 = ppt->x + xorg;
	y2 = ppt->y + yorg;
	while(--npt)
#endif
	{
		nbox = nboxInit;
		pbox = pboxInit;

#ifdef POLYSEGMENT
		x1 = pSeg->x1 + xorg;
		y1 = pSeg->y1 + yorg;
		x2 = pSeg->x2 + xorg;
		y2 = pSeg->y2 + yorg;
		pSeg++;
#else
		x1 = x2;
		y1 = y2;
		++ppt;
		if (mode == CoordModePrevious) {
			xorg = x1;
			yorg = y1;
		}
		x2 = ppt->x + xorg;
		y2 = ppt->y + yorg;
#endif

		CalcLineDeltas(x1, y1, x2, y2, adx, ady, signdx, signdy,
			       1, 1, octant);

		if (adx > ady) {
			axis = X_AXIS;
			e1 = ady << 1;
			e2 = e1 - (adx << 1);
			e = e1 - adx;
			unclippedlen = adx;
		} else {
			axis = Y_AXIS;
			e1 = adx << 1;
			e2 = e1 - (ady << 1);
			e = e1 - ady;
			unclippedlen = ady;
			SetYMajorOctant(octant);
		}

		FIXUP_ERROR(e, octant, bias);

		/* we have bresenham parameters and two points.
		   all we have to do now is clip and draw.
		*/

		while(nbox--) {
			oc1 = 0;
			oc2 = 0;
			OUTCODES(oc1, x1, y1, pbox);
			OUTCODES(oc2, x2, y2, pbox);
			if ((oc1 | oc2) == 0) {
#ifdef POLYSEGMENT
				if (pGC->capStyle != CapNotLast)
					unclippedlen++;
				dashIndexTmp = dashIndex;
				dashOffsetTmp = dashOffset;
				afbBresD(&dashIndexTmp, pDash, numInDashList, &dashOffsetTmp,
							 isDoubleDash, addrlBase, nlwidth, sizeDst, depthDst,
							 signdx, signdy, axis, x1, y1, e, e1, e2, unclippedlen,
							 rrops, bgrrops);	/* @@@ NEXT PLANE PASSED @@@ */
				break;
#else
				afbBresD(&dashIndex, pDash, numInDashList, &dashOffset,
							 isDoubleDash, addrlBase, nlwidth, sizeDst, depthDst,
							 signdx, signdy, axis, x1, y1, e, e1, e2, unclippedlen,
							 rrops, bgrrops);	/* @@@ NEXT PLANE PASSED @@@ */
				goto dontStep;
#endif
			} else if (oc1 & oc2) {
				pbox++;
			} else /* have to clip */ {
				int new_x1 = x1, new_y1 = y1, new_x2 = x2, new_y2 = y2;
				int clip1 = 0, clip2 = 0;
				int clipdx, clipdy;
				int err;

				if (miZeroClipLine(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1,
								   &new_x1, &new_y1, &new_x2, &new_y2,
								   adx, ady, &clip1, &clip2,
								   octant, bias, oc1, oc2) == -1) {
					pbox++;
					continue;
				}
				dashIndexTmp = dashIndex;
				dashOffsetTmp = dashOffset;
				if (clip1) {
					int dlen;

					if (axis == X_AXIS)
						dlen = abs(new_x1 - x1);
					else
						dlen = abs(new_y1 - y1);
					miStepDash (dlen, &dashIndexTmp, pDash,
								numInDashList, &dashOffsetTmp);
				}
				if (axis == X_AXIS)
					len = abs(new_x2 - new_x1);
				else
					len = abs(new_y2 - new_y1);
#ifdef POLYSEGMENT
				if (clip2 != 0 || pGC->capStyle != CapNotLast)
					len++;
#else
				len += (clip2 != 0);
#endif
				if (len) {
					/* unwind bresenham error term to first point */
					if (clip1) {
						clipdx = abs(new_x1 - x1);
						clipdy = abs(new_y1 - y1);
						if (axis == X_AXIS)
							err = e+((clipdy*e2) + ((clipdx-clipdy)*e1));
						else
							err = e+((clipdx*e2) + ((clipdy-clipdx)*e1));
					}
					else
						err = e;
					afbBresD(&dashIndexTmp, pDash, numInDashList, &dashOffsetTmp,
								 isDoubleDash, addrlBase, nlwidth, sizeDst, depthDst,
								 signdx, signdy, axis, new_x1, new_y1, err, e1, e2,
								 len, rrops, bgrrops);	/* @@@ NEXT PLANE PASSED @@@ */
				}
				pbox++;
			}
		} /* while (nbox--) */
#ifndef POLYSEGMENT
		/*
		 * walk the dash list around to the next line
		 */
		miStepDash (unclippedlen, &dashIndex, pDash,
					numInDashList, &dashOffset);
dontStep:		;
#endif
	} /* while (nline--) */

#ifndef POLYSEGMENT
	/* paint the last point if the end style isn't CapNotLast.
	   (Assume that a projecting, butt, or round cap that is one
		pixel wide is the same as the single pixel of the endpoint.)
	*/

	if ((pGC->capStyle != CapNotLast) &&
		 ((dashIndex & 1) == 0 || isDoubleDash) &&
		 ((ppt->x + xorg != pptInit->x + pDrawable->x) ||
		  (ppt->y + yorg != pptInit->y + pDrawable->y) ||
		  (ppt == pptInit + 1))) {
		nbox = nboxInit;
		pbox = pboxInit;
		while (nbox--) {
			if ((x2 >= pbox->x1) && (y2 >= pbox->y1) && (x2 < pbox->x2) &&
				 (y2 < pbox->y2)) {
				int rop;

				for (d = 0; d < depthDst; d++) {
					addrl = afbScanline(addrlBase, x2, y2, nlwidth);
					addrlBase += sizeDst;			/* @@@ NEXT PLANE @@@ */

					rop = rrops[d];
					if (dashIndex & 1)
						rop = bgrrops[d];

					switch (rop) {
						case RROP_BLACK:
							*addrl &= rmask[x2 & PIM];
							break;
						case RROP_WHITE:
							*addrl |= mask[x2 & PIM];
							break;

						case RROP_INVERT:
							*addrl ^= mask[x2 & PIM];
							break;

						case RROP_NOP:
							break;
					}
				} /* for (d = ...) */
				break;
			} else
				pbox++;
		}
	}
#endif
}
