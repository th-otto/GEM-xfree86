/*
 * Copyright 1998 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * Generic RAMDAC access to colormaps.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/xf86RamDacCmap.c,v 1.5 1999/07/18 03:27:01 dawes Exp $ */

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "compiler.h"
#include "mipointer.h"
#include "micmap.h"

#include "xf86.h"
#include "xf86_ansic.h"
#include "colormapst.h"
#include "xf86RamDacPriv.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

void
RamDacLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors,
		 VisualPtr pVisual)
{
    RamDacRecPtr hwp = RAMDACSCRPTR(pScrn);
    int i, index;

    for (i = 0; i < numColors; i++) {
	index = indices[i];
	(*hwp->WriteAddress)(pScrn, index);
	(*hwp->WriteData)(pScrn, colors[index].red);
	(*hwp->WriteData)(pScrn, colors[index].green);
	(*hwp->WriteData)(pScrn, colors[index].blue);
    }
}

Bool
RamDacHandleColormaps(ScreenPtr pScreen, int maxColors, int sigRGBbits,
		      unsigned int flags)
{
  return xf86HandleColormaps(pScreen, maxColors, sigRGBbits,
			     RamDacLoadPalette, NULL, flags);
}
