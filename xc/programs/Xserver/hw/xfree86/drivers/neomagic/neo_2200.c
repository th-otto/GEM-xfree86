/**********************************************************************
Copyright 1998, 1999 by Precision Insight, Inc., Cedar Park, Texas.

                        All Rights Reserved

Permission to use, copy, modify, distribute, and sell this software and
its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Precision Insight not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  Precision Insight
and its suppliers make no representations about the suitability of this
software for any purpose.  It is provided "as is" without express or 
implied warranty.

PRECISION INSIGHT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**********************************************************************/
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/neomagic/neo_2200.c,v 1.4 1999/12/03 19:17:34 eich Exp $ */

/*
 * The original Precision Insight driver for
 * XFree86 v.3.3 has been sponsored by Red Hat.
 *
 * Authors:
 *   Jens Owen (jens@precisioninsight.com)
 *   Kevin E. Martin (kevin@precisioninsight.com)
 *
 * Port to Xfree86 v.4.0
 *   1998, 1999 by Egbert Eich (Egbert.Eich@Physik.TU-Darmstadt.DE)
 */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

/* Drivers that use XAA need this */
#include "xf86fbman.h"

#include "miline.h"

#include "neo.h"
#include "neo_reg.h"
#include "neo_macros.h"

static void Neo2200Sync(ScrnInfoPtr pScrn);
static void Neo2200SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
					      int ydir, int rop,
					      unsigned int planemask,
					      int trans_color);
static void Neo2200SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int srcX,
						int srcY, int dstX, int dstY,
						int w, int h);
static void Neo2200SetupForSolidFillRect(ScrnInfoPtr pScrn, int color, int rop,
				  unsigned int planemask);
static void Neo2200SubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y,
					   int w, int h);
#ifdef colorexpandfill
static void Neo2200SetupForCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
						      int fg, int bg,
						      int rop,
						unsigned int planemask);
static void Neo2200SubsequentCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
							int x, int y,
							int w, int h,
							int skipleft);
#endif
static void Neo2200SetupForMono8x8PatternFill(ScrnInfoPtr pScrn,
					       int patternx,
					       int patterny,
					       int bg,
					       int fg,
					       int rop, 
					       unsigned int planemask);
static void Neo2200SubsequentMono8x8PatternFill(ScrnInfoPtr pScrn,
						 int patternx,
						 int patterny, 
						 int x, int y,
						 int w, int h);



static unsigned int neo2200Rop[16] = {
    0x000000,    /* 0x0000 - GXclear         */
    0x080000,    /* 0x1000 - GXand           */
    0x040000,    /* 0x0100 - GXandReverse    */
    0x0c0000,    /* 0x1100 - GXcopy          */
    0x020000,    /* 0x0010 - GXandInvert     */
    0x0a0000,    /* 0x1010 - GXnoop          */
    0x060000,    /* 0x0110 - GXxor           */
    0x0e0000,    /* 0x1110 - GXor            */
    0x010000,    /* 0x0001 - GXnor           */
    0x090000,    /* 0x1001 - GXequiv         */
    0x050000,    /* 0x0101 - GXinvert        */
    0x0d0000,    /* 0x1101 - GXorReverse     */
    0x030000,    /* 0x0011 - GXcopyInvert    */
    0x0b0000,    /* 0x1011 - GXorInverted    */
    0x070000,    /* 0x0111 - GXnand          */
    0x0f0000     /* 0x1111 - GXset           */
};


Bool 
Neo2200AccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);
    BoxRec AvailFBArea;

    nPtr->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if(!infoPtr) return FALSE;

    /*
     * Set up the main acceleration flags.
     */
    infoPtr->Flags |= LINEAR_FRAMEBUFFER | OFFSCREEN_PIXMAPS;
    if(nAcl->cacheEnd > nAcl->cacheStart) infoPtr->Flags = PIXMAP_CACHE;
#if 0
    infoPtr->PixmapCacheFlags |= DO_NOT_BLIT_STIPPLES;
#endif
    /* sync */
    infoPtr->Sync = Neo2200Sync;

    /* screen to screen copy */
    infoPtr->ScreenToScreenCopyFlags = (NO_TRANSPARENCY | NO_PLANEMASK);
    infoPtr->SetupForScreenToScreenCopy = 
	Neo2200SetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = 
	Neo2200SubsequentScreenToScreenCopy;

    /* solid filled rectangles */
    infoPtr->SolidFillFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidFill = 
	Neo2200SetupForSolidFillRect;
    infoPtr->SubsequentSolidFillRect = 
	Neo2200SubsequentSolidFillRect;

#ifdef colorexpandfill
      /* We need byte scanline padding before we can use this
       * or does anyone know how to switch the chip to dword
       * padding? if we could do right edge clipping this would
       * help also. Left edge clipping cannot be used since it
       * allows only clipping of up to 8 pixels :-((
       */ /*§§§*/

    /* cpu to screen color expansion */
    infoPtr->CPUToScreenColorExpandFillFlags = ( NO_PLANEMASK |
						 /* SCANLINE_PAD_BYTE | */
						 CPU_TRANSFER_PAD_DWORD |
						 BIT_ORDER_IN_BYTE_MSBFIRST );
    infoPtr->ColorExpandBase = 
	(unsigned char *)(nPtr->NeoMMIOBase + 0x100000);
    infoPtr->ColorExpandRange = 0x100000;

    infoPtr->SetupForCPUToScreenColorExpandFill = 
	Neo2200SetupForCPUToScreenColorExpandFill;
    infoPtr->SubsequentCPUToScreenColorExpandFill = 
	Neo2200SubsequentCPUToScreenColorExpandFill;
#endif
    /* 8x8 pattern fills */
    infoPtr->Mono8x8PatternFillFlags = NO_PLANEMASK
	| HARDWARE_PATTERN_PROGRAMMED_ORIGIN
	|BIT_ORDER_IN_BYTE_MSBFIRST;
    
    infoPtr->SetupForMono8x8PatternFill = 
	Neo2200SetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect = 
	Neo2200SubsequentMono8x8PatternFill;

    /*
     * Setup some global variables
     */

    /* Initialize for 8bpp or 15/16bpp support accellerated */
    switch (pScrn->bitsPerPixel) {
    case 8:
	nAcl->BltModeFlags = NEO_MODE1_DEPTH8;
        nAcl->PixelWidth = 1;
	break;
    case 15:
    case 16:
	nAcl->BltModeFlags = NEO_MODE1_DEPTH16;
        nAcl->PixelWidth = 2;
	break;
    case 24:
    default:
	return FALSE;
    }
    nAcl->Pitch = pScrn->displayWidth * nAcl->PixelWidth;    

    /* Initialize for widths */
    switch (pScrn->displayWidth) {
    case 320:
	nAcl->BltModeFlags |= NEO_MODE1_X_320;
	break;
    case 640:
	nAcl->BltModeFlags |= NEO_MODE1_X_640;
	break;
    case 800:
	nAcl->BltModeFlags |= NEO_MODE1_X_800;
	break;
    case 1024:
	nAcl->BltModeFlags |= NEO_MODE1_X_1024;
	break;
    case 1152:
	nAcl->BltModeFlags |= NEO_MODE1_X_1152;
	break;
    case 1280:
	nAcl->BltModeFlags |= NEO_MODE1_X_1280;
	break;
    case 1600:
	nAcl->BltModeFlags |= NEO_MODE1_X_1600;
	break;
    default:
	return FALSE;
    }

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = nAcl->cacheEnd /
      (pScrn->displayWidth * (pScrn->bitsPerPixel >> 3));
    xf86InitFBManager(pScreen, &AvailFBArea); 

    return(XAAInit(pScreen, infoPtr));

}

static void
Neo2200Sync(ScrnInfoPtr pScrn)
{
    NEOPtr nPtr = NEOPTR(pScrn);

    WAIT_ENGINE_IDLE();
}

static void
Neo2200SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir, int ydir,
				  int rop,
				  unsigned int planemask,
				  int trans_color)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);

    nAcl->tmpBltCntlFlags = (NEO_BC3_SKIP_MAPPING | neo2200Rop[rop]);

    /* set blt control */
    WAIT_ENGINE_IDLE();
    /*OUTREG16(NEOREG_BLTMODE, nAcl->BltModeFlags);*/
    OUTREG(NEOREG_BLTSTAT, nAcl->BltModeFlags << 16);
    OUTREG(NEOREG_BLTCNTL, nAcl->tmpBltCntlFlags);
    OUTREG(NEOREG_PITCH, (nAcl->Pitch<<16) 
	   | (nAcl->Pitch & 0xffff));
}

static void
Neo2200SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
				    int srcX, int srcY,
				    int dstX, int dstY,
				    int w, int h)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);

    if ((dstY < srcY) || ((dstY == srcY) && (dstX < srcX))) {
	/* start with upper left corner */
	WAIT_ENGINE_IDLE();
	OUTREG(NEOREG_BLTCNTL, nAcl->tmpBltCntlFlags);
	OUTREG(NEOREG_SRCSTARTOFF,
            (srcY * nAcl->Pitch) + (srcX * nAcl->PixelWidth));
	OUTREG(NEOREG_DSTSTARTOFF,
            (dstY * nAcl->Pitch) + (dstX * nAcl->PixelWidth));
	OUTREG(NEOREG_XYEXT, (h<<16) | (w & 0xffff));
    }
    else {
	/* start with lower right corner */
	WAIT_ENGINE_IDLE();
	OUTREG(NEOREG_BLTCNTL, (nAcl->tmpBltCntlFlags 
				| NEO_BC0_X_DEC
				| NEO_BC0_DST_Y_DEC 
				| NEO_BC0_SRC_Y_DEC));
	OUTREG(NEOREG_SRCSTARTOFF,
            ((srcY+h-1) * nAcl->Pitch) + ((srcX+w-1) 
						 * nAcl->PixelWidth));
	OUTREG(NEOREG_DSTSTARTOFF,
            ((dstY+h-1) * nAcl->Pitch) + ((dstX+w-1) 
						 * nAcl->PixelWidth));
	OUTREG(NEOREG_XYEXT, (h<<16) | (w & 0xffff));
    }
}


static void
Neo2200SetupForSolidFillRect(ScrnInfoPtr pScrn, int color, int rop,
			     unsigned int planemask)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);

    WAIT_ENGINE_IDLE();

    /* set blt control */
    /*OUTREG16(NEOREG_BLTMODE, nAcl->BltModeFlags);*/
    OUTREG(NEOREG_BLTSTAT, nAcl->BltModeFlags << 16);
    OUTREG(NEOREG_BLTCNTL, NEO_BC0_SRC_IS_FG    |
                           NEO_BC3_SKIP_MAPPING |
                           NEO_BC3_DST_XY_ADDR  |
                           NEO_BC3_SRC_XY_ADDR  | neo2200Rop[rop]);

    /* set foreground color */
    OUTREG(NEOREG_FGCOLOR, color);
}


static void
Neo2200SubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    NEOPtr nPtr = NEOPTR(pScrn);

    WAIT_ENGINE_IDLE();
    OUTREG(NEOREG_DSTSTARTOFF, (y<<16) | (x & 0xffff));
    OUTREG(NEOREG_XYEXT, (h<<16) | (w & 0xffff));
}


#ifdef colorexpandfill
static void
Neo2200SetupForCPUToScreenColorExpandFill(ScrnInfoPtr pScrn, int fg, int bg,
				      int rop,
				      unsigned int planemask)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);

    if (bg == -1) {
	/* transparent setup */
	nAcl->tmpBltCntlFlags = ( NEO_BC0_SYS_TO_VID   |
					 NEO_BC0_SRC_MONO     |
					 NEO_BC0_SRC_TRANS    |
					 NEO_BC3_SKIP_MAPPING |
					 NEO_BC3_DST_XY_ADDR  | 
					 neo2200Rop[rop]);

	WAIT_ENGINE_IDLE();
	/*OUTREG16(NEOREG_BLTMODE, nAcl->BltModeFlags);*/
	OUTREG(NEOREG_BLTSTAT, nAcl->BltModeFlags << 16);
	OUTREG(NEOREG_BLTCNTL, nAcl->tmpBltCntlFlags);
	OUTREG(NEOREG_FGCOLOR, fg);
    }
    else {
	/* opaque setup */
        nAcl->tmpBltCntlFlags = ( NEO_BC0_SYS_TO_VID   |
					 NEO_BC0_SRC_MONO     |
					 NEO_BC3_SKIP_MAPPING |
					 NEO_BC3_DST_XY_ADDR  | 
					 neo2200Rop[rop]);

	WAIT_ENGINE_IDLE();
	/*OUTREG16(NEOREG_BLTMODE, nAcl->BltModeFlags);*/
	OUTREG(NEOREG_BLTSTAT, nAcl->BltModeFlags << 16);
	OUTREG(NEOREG_BLTCNTL, nAcl->tmpBltCntlFlags);
	OUTREG(NEOREG_FGCOLOR, fg);
	OUTREG(NEOREG_BGCOLOR, bg);
    }
}


static void
Neo2200SubsequentCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
					int x, int y,
					int w, int h,
					int skipleft)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);

    OUTREG(NEOREG_BLTCNTL, nAcl->tmpBltCntlFlags 
	   | ((skipleft << 2) & 0x1C));
    OUTREG(NEOREG_SRCSTARTOFF, 0);
    OUTREG(NEOREG_DSTSTARTOFF, (y<<16) | (x & 0xffff));
    OUTREG(NEOREG_XYEXT, (h<<16) | (w & 0xffff));
}
#endif

static void
Neo2200SetupForMono8x8PatternFill(ScrnInfoPtr pScrn,
				     int patternx,
				     int patterny,
				     int fg, int bg, 
				     int rop, unsigned int planemask)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);

    if (bg == -1) {
	/* transparent setup */
	nAcl->tmpBltCntlFlags = ( NEO_BC0_SRC_MONO     |
					 NEO_BC0_FILL_PAT     |
					 NEO_BC0_SRC_TRANS    |
					 NEO_BC3_SKIP_MAPPING |
					 NEO_BC3_DST_XY_ADDR  | 
					 neo2200Rop[rop]);

	WAIT_ENGINE_IDLE();
	/*OUTREG16(NEOREG_BLTMODE, nAcl->BltModeFlags);*/
	OUTREG(NEOREG_BLTSTAT, nAcl->BltModeFlags << 16);
	OUTREG(NEOREG_FGCOLOR, fg);
	OUTREG(NEOREG_SRCSTARTOFF, 
	    (patterny * pScrn->displayWidth * pScrn->bitsPerPixel 
	     + patternx) >> 3);
    }
    else {
	/* opaque setup */
	nAcl->tmpBltCntlFlags = ( NEO_BC0_SRC_MONO     |
					 NEO_BC0_FILL_PAT     |
					 NEO_BC3_SKIP_MAPPING |
					 NEO_BC3_DST_XY_ADDR  | 
					 neo2200Rop[rop]);

	WAIT_ENGINE_IDLE();
	/*OUTREG16(NEOREG_BLTMODE, nAcl->BltModeFlags);*/
	OUTREG(NEOREG_BLTSTAT, nAcl->BltModeFlags << 16);
	OUTREG(NEOREG_FGCOLOR, fg);
	OUTREG(NEOREG_BGCOLOR, bg);
	OUTREG(NEOREG_SRCSTARTOFF, 
	    (patterny * pScrn->displayWidth * pScrn->bitsPerPixel 
	     + patternx) >> 3);
    }
}


static void
Neo2200SubsequentMono8x8PatternFill(ScrnInfoPtr pScrn,
				    int patternx,
				    int patterny, 
				    int x, int y,
				    int w, int h)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);

    patterny &= 0x7;

    WAIT_ENGINE_IDLE();
    OUTREG(NEOREG_BLTCNTL, nAcl->tmpBltCntlFlags | 
	   (patterny << 20)       | 
	   ((patternx << 10) & 0x1C00));
    OUTREG(NEOREG_SRCBITOFF, patternx);
    OUTREG(NEOREG_DSTSTARTOFF, (y<<16) | (x & 0xffff));
    OUTREG(NEOREG_XYEXT, (h<<16) | (w & 0xffff));
}
