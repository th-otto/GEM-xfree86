/*
 * Copyright 1997,1998 by Alan Hourihane, Wigan, England.
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
 * Trident accelerated options.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident_accel.c,v 1.10 2000/01/27 01:13:23 alanh Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "miline.h"

#include "trident.h"
#include "trident_regs.h"

#include "xaalocal.h"
#include "xaarop.h"

static void TridentSync(ScrnInfoPtr pScrn);
static void TridentSetupForDashedLine(ScrnInfoPtr pScrn, int fg, int bg, 
				int rop, unsigned int planemask, int length,
    				unsigned char *pattern);
static void TridentSubsequentDashedBresenhamLine(ScrnInfoPtr pScrn,
        			int x, int y, int dmaj, int dmin, int e, 
				int len, int octant, int phase);
static void TridentSetupForSolidLine(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void TridentSubsequentSolidBresenhamLine(ScrnInfoPtr pScrn,
        			int x, int y, int dmaj, int dmin, int e, 
				int len, int octant);
static void TridentSubsequentSolidHorVertLine(ScrnInfoPtr pScrn, int x, int y,
    				int len, int dir);
static void TridentSetupForFillRectSolid(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void TridentSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x,
				int y, int w, int h);
static void TridentSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
				int x1, int y1, int x2,
				int y2, int w, int h);
static void TridentSetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop, 
                                unsigned int planemask,
				int transparency_color);
static void TridentSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, 
				int x2, int y2);
static void TridentDisableClipping(ScrnInfoPtr pScrn);
static void TridentWritePixmap(ScrnInfoPtr pScrn, int x, int y, int w, int h,
    				unsigned char *src, int srcwidth, int rop,
    				unsigned int planemask, int transparency_color,
    				int bpp, int depth);
static void TridentSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int fg, int bg, 
				int rop, unsigned int planemask);
static void TridentSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int x, int y, 
				int w, int h);
static void TridentSetupForColor8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny, 
				int rop, unsigned int planemask, int trans_col);
static void TridentSubsequentColor8x8PatternFillRect(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int x, int y, 
				int w, int h);
static void TridentSetupForScanlineCPUToScreenColorExpandFill(
				ScrnInfoPtr pScrn,
				int fg, int bg, int rop, 
				unsigned int planemask);
static void TridentSubsequentScanlineCPUToScreenColorExpandFill(
				ScrnInfoPtr pScrn, int x,
				int y, int w, int h, int skipleft);
static void TridentSubsequentColorExpandScanline(ScrnInfoPtr pScrn, int bufno);


static void
TridentInitializeAccelerator(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    CHECKCLIPPING;
    if (pTrident->Chipset == PROVIDIA9682)
    	pTrident->EngineOperation |= 0x100; /* Disable Clipping */
    TGUI_OPERMODE(pTrident->EngineOperation);
    if (pTrident->Chipset == PROVIDIA9685 && pScrn->bitsPerPixel == 16)
    	pTrident->PatternLocation = pScrn->displayWidth;
    else
    	pTrident->PatternLocation = pScrn->displayWidth*pScrn->bitsPerPixel/8;
}

Bool
TridentAccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    BoxRec AvailFBArea;

    pTrident->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if (!infoPtr) return FALSE;

    TridentInitializeAccelerator(pScrn);

    if (!(pTrident->Chipset == TGUI9440AGi && pScrn->bitsPerPixel > 8)) 
    	infoPtr->Flags = PIXMAP_CACHE |
		     OFFSCREEN_PIXMAPS |
		     LINEAR_FRAMEBUFFER;

    infoPtr->PixmapCacheFlags = DO_NOT_BLIT_STIPPLES;
 
    infoPtr->Sync = TridentSync;

    if (pTrident->Chipset >= TGUI9660) {
    	infoPtr->SetClippingRectangle = TridentSetClippingRectangle;
    	infoPtr->DisableClipping = TridentDisableClipping;
    	infoPtr->ClippingFlags = HARDWARE_CLIP_SOLID_FILL |
			     	 HARDWARE_CLIP_SOLID_LINE |
			     	 HARDWARE_CLIP_DASHED_LINE |
			     	 HARDWARE_CLIP_SCREEN_TO_SCREEN_COPY |
			     	 HARDWARE_CLIP_MONO_8x8_FILL |
			     	 HARDWARE_CLIP_COLOR_8x8_FILL;
    }

    infoPtr->SolidLineFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidLine = TridentSetupForSolidLine;
    infoPtr->SolidBresenhamLineErrorTermBits = 12;
    infoPtr->SubsequentSolidBresenhamLine = TridentSubsequentSolidBresenhamLine;
    infoPtr->SubsequentSolidHorVertLine = TridentSubsequentSolidHorVertLine;

    infoPtr->DashedLineFlags = LINE_PATTERN_MSBFIRST_LSBJUSTIFIED |
			       NO_PLANEMASK |
			       LINE_PATTERN_POWER_OF_2_ONLY;
    infoPtr->SetupForDashedLine = TridentSetupForDashedLine;
    infoPtr->DashedBresenhamLineErrorTermBits = 12;
    infoPtr->SubsequentDashedBresenhamLine = 
					TridentSubsequentDashedBresenhamLine;  
    infoPtr->DashPatternMaxLength = 16;

    infoPtr->SolidFillFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidFill = TridentSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = TridentSubsequentFillRectSolid;
    
    infoPtr->ScreenToScreenCopyFlags = NO_PLANEMASK;

    if (!HAS_DST_TRANS) infoPtr->ScreenToScreenCopyFlags |= NO_TRANSPARENCY;

    infoPtr->SetupForScreenToScreenCopy = 	
				TridentSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = 		
				TridentSubsequentScreenToScreenCopy;

    infoPtr->Mono8x8PatternFillFlags =  NO_PLANEMASK | 
					HARDWARE_PATTERN_SCREEN_ORIGIN | 
					BIT_ORDER_IN_BYTE_MSBFIRST;

    infoPtr->SetupForMono8x8PatternFill =
				TridentSetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect = 
				TridentSubsequentMono8x8PatternFillRect;
    infoPtr->MonoPatternPitch = 64;

#if 0 /* Not convinced this works 100% yet */
    infoPtr->Color8x8PatternFillFlags = NO_PLANEMASK | 
					HARDWARE_PATTERN_SCREEN_ORIGIN | 
					BIT_ORDER_IN_BYTE_MSBFIRST;

    if (!HAS_DST_TRANS) infoPtr->Color8x8PatternFillFlags |= NO_TRANSPARENCY;

    infoPtr->SetupForColor8x8PatternFill =
				TridentSetupForColor8x8PatternFill;
    infoPtr->SubsequentColor8x8PatternFillRect = 
				TridentSubsequentColor8x8PatternFillRect;
#endif

    infoPtr->ScanlineCPUToScreenColorExpandFillFlags = NO_PLANEMASK |
					BIT_ORDER_IN_BYTE_MSBFIRST;

    if (pTrident->Chipset >= TGUI9660)
    	infoPtr->ScanlineCPUToScreenColorExpandFillFlags |=
					LEFT_EDGE_CLIPPING_NEGATIVE_X |
					LEFT_EDGE_CLIPPING;

    pTrident->XAAScanlineColorExpandBuffers[0] =
	    xnfalloc(((pScrn->virtualX + 63)) *4* (pScrn->bitsPerPixel / 8));

    infoPtr->NumScanlineColorExpandBuffers = 1;
    infoPtr->ScanlineColorExpandBuffers = 
					pTrident->XAAScanlineColorExpandBuffers;

    infoPtr->SetupForScanlineCPUToScreenColorExpandFill =
			TridentSetupForScanlineCPUToScreenColorExpandFill;
    infoPtr->SubsequentScanlineCPUToScreenColorExpandFill = 
			TridentSubsequentScanlineCPUToScreenColorExpandFill;
    infoPtr->SubsequentColorExpandScanline = 
			TridentSubsequentColorExpandScanline;

    /* Requires clipping, therefore only 9660's or above */
    if (pTrident->Chipset >= TGUI9660) {
    	infoPtr->WritePixmap = TridentWritePixmap;
    	infoPtr->WritePixmapFlags = NO_PLANEMASK;

    	if (!HAS_DST_TRANS) infoPtr->WritePixmapFlags |= NO_TRANSPARENCY;
    }

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = (pTrident->FbMapSize - 4096) / (pScrn->displayWidth *
					    pScrn->bitsPerPixel / 8);

    if (AvailFBArea.y2 > 2047) AvailFBArea.y2 = 2047;

    xf86InitFBManager(pScreen, &AvailFBArea);

    return(XAAInit(pScreen, infoPtr));
}

static void
TridentSync(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int count = 0, timeout = 0;
    int busy;

    TGUI_OPERMODE(pTrident->EngineOperation);

    for (;;) {
	BLTBUSY(busy);
	if (busy != GE_BUSY) {
	    return;
	}
	count++;
	if (count == 10000000) {
	    ErrorF("Trident: BitBLT engine time-out.\n");
	    count = 9990000;
	    timeout++;
	    if (timeout == 8) {
		/* Reset BitBLT Engine */
		TGUI_STATUS(0x00);
		return;
	    }
	}
    }
}

static void
TridentClearSync(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int count = 0, timeout = 0;
    int busy;

    for (;;) {
	BLTBUSY(busy);
	if (busy != GE_BUSY) {
	    return;
	}
	count++;
	if (count == 10000000) {
	    ErrorF("Trident: BitBLT engine time-out.\n");
	    count = 9990000;
	    timeout++;
	    if (timeout == 8) {
		/* Reset BitBLT Engine */
		TGUI_STATUS(0x00);
		return;
	    }
	}
    }
}

static void
TridentSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, 
				int xdir, int ydir, int rop,
				unsigned int planemask, int transparency_color)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int dst = 0;

    pTrident->BltScanDirection = 0;
    if (xdir < 0) pTrident->BltScanDirection |= XNEG;
    if (ydir < 0) pTrident->BltScanDirection |= YNEG;

    if (transparency_color != -1) {
	if (pTrident->Chipset == PROVIDIA9685) {
	    dst |= 1<<16;
	} else {
    	    TGUI_OPERMODE(pTrident->EngineOperation | DST_ENABLE);
	}
	REPLICATE(transparency_color);
	TGUI_CKEY(transparency_color);
    }

    if (rop == GXcopy) {
    	if ((pTrident->Chipset == PROVIDIA9682) || 
	    (pTrident->Chipset == TGUI9680))
		dst |= FASTMODE;
    	if (pTrident->Chipset == PROVIDIA9685) 
		dst |= 1<<21;
    }

    TGUI_DRAWFLAG(pTrident->DrawFlag | pTrident->BltScanDirection | SCR2SCR | dst);
    TGUI_FMIX(XAACopyROP[rop]);
}

static void
TridentSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1,
					int x2, int y2, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    if (pTrident->BltScanDirection & YNEG) {
        y1 = y1 + h - 1;
	y2 = y2 + h - 1;
    }
    if (pTrident->BltScanDirection & XNEG) {
	x1 = x1 + w - 1;
	x2 = x2 + w - 1;
    }
    TGUI_SRC_XY(x1,y1);
    TGUI_DEST_XY(x2,y2);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentClearSync(pScrn);
}

static void
TridentSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_SRCCLIP_XY(x1, y1);
    TGUI_DSTCLIP_XY(x2, y2);
    pTrident->Clipping = TRUE;
    if (pTrident->Chipset == PROVIDIA9682)
    	pTrident->EngineOperation &= 0xFEFF; /* Enable Clipping */
    if (pTrident->Chipset == PROVIDIA9685)
	pTrident->DrawFlag = 1<<30;
    else
	pTrident->DrawFlag = 0;
    TGUI_OPERMODE(pTrident->EngineOperation & 0xFEFF);
}

static void
TridentDisableClipping(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    if (pTrident->Chipset == PROVIDIA9682)
    	pTrident->EngineOperation |= 0x100; /* Disable Clipping */
    if (pTrident->Chipset == PROVIDIA9685)
	pTrident->DrawFlag = 0;
    TGUI_OPERMODE(pTrident->EngineOperation);
    CHECKCLIPPING;
}

static void
TridentSetupForSolidLine(ScrnInfoPtr pScrn, int color,
					 int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    pTrident->BltScanDirection = 0;
    REPLICATE(color);
    TGUI_FMIX(XAAPatternROP[rop]);
    if (pTrident->Chipset == PROVIDIA9685) {
    	TGUI_FPATCOL(color);
	if (rop == GXcopy) 
	    pTrident->BltScanDirection |= 1<<21;
    } else {
    	if ((pTrident->Chipset == PROVIDIA9682 ||
	     pTrident->Chipset == TGUI9680) && rop == GXcopy)
		pTrident->BltScanDirection |= FASTMODE;
    	TGUI_FCOLOUR(color);
    }
}

static void 
TridentSubsequentSolidBresenhamLine( ScrnInfoPtr pScrn,
        int x, int y, int dmaj, int dmin, int e, int len, int octant)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int tmp = pTrident->BltScanDirection;

    if (octant & YMAJOR) tmp |= YMAJ;
    if (octant & XDECREASING) tmp |= XNEG;
    if (octant & YDECREASING) tmp |= YNEG;
    TGUI_DRAWFLAG(pTrident->DrawFlag | SOLIDFILL | STENCIL | tmp);
    TGUI_SRC_XY(dmin-dmaj,dmin);
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(dmin+e,len);
    TGUI_COMMAND(GE_BRESLINE);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
}

static void 
TridentSubsequentSolidHorVertLine(
    ScrnInfoPtr pScrn,
    int x, int y, 
    int len, int dir
){
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_DRAWFLAG(pTrident->DrawFlag | SOLIDFILL | PATMONO);
    if (dir == DEGREES_0) {
    	TGUI_DIM_XY(len,1);
    	TGUI_DEST_XY(x,y);
    } else {
    	TGUI_DIM_XY(1,len);
    	TGUI_DEST_XY(x,y);
    }
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
}

void
TridentSetupForDashedLine(
    ScrnInfoPtr pScrn, 
    int fg, int bg, int rop,
    unsigned int planemask,
    int length,
    unsigned char *pattern
){
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    CARD32 *DashPattern = (CARD32*)pattern;
    CARD32 NiceDashPattern = DashPattern[0];

    NiceDashPattern = *((CARD16 *)pattern) & ((1<<length) - 1);
    switch(length) {
	case 2:	NiceDashPattern |= NiceDashPattern << 2;
	case 4:	NiceDashPattern |= NiceDashPattern << 4;
	case 8:	NiceDashPattern |= NiceDashPattern << 8;
    }
    pTrident->BltScanDirection = 0;
    if ((pTrident->Chipset == PROVIDIA9682 ||
	 pTrident->Chipset == TGUI9680) && rop == GXcopy)
	pTrident->BltScanDirection |= FASTMODE;
    if (pTrident->Chipset == PROVIDIA9685 && rop == GXcopy) 
	pTrident->BltScanDirection |= 1<<21;
    REPLICATE(fg);
    if (pTrident->Chipset == PROVIDIA9685) {
	TGUI_FPATCOL(fg);
    	if (bg == -1) {
	    pTrident->BltScanDirection |= 1<<12;
    	    TGUI_BPATCOL(~fg);
    	} else {
    	    REPLICATE(bg);
    	    TGUI_BPATCOL(bg);
    	}
    } else {
    	TGUI_FCOLOUR(fg);
    	if (bg == -1) {
	    pTrident->BltScanDirection |= 1<<12;
    	    TGUI_BCOLOUR(~fg);
    	} else {
    	    REPLICATE(bg);
    	    TGUI_BCOLOUR(bg);
    	}
    }
    TGUI_FMIX(XAAPatternROP[rop]);
    pTrident->LinePattern = NiceDashPattern;
}


void
TridentSubsequentDashedBresenhamLine(ScrnInfoPtr pScrn,
        int x, int y, int dmaj, int dmin, int e, int len, int octant, int phase)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int tmp = pTrident->BltScanDirection;

    if (octant & YMAJOR) tmp |= YMAJ;
    if (octant & XDECREASING) tmp |= XNEG;
    if (octant & YDECREASING) tmp |= YNEG;

    TGUI_STYLE(((pTrident->LinePattern >> phase) | 
		(pTrident->LinePattern << (16-phase))) & 0x0000FFFF);
    TGUI_DRAWFLAG(pTrident->DrawFlag | STENCIL | tmp);
    TGUI_SRC_XY(dmin-dmaj,dmin);
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(e+dmin,len);
    TGUI_COMMAND(GE_BRESLINE);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
}

static void
TridentSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, 
				    int rop, unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int drawflag = 0;

    REPLICATE(color);
    TGUI_FMIX(XAAPatternROP[rop]);
    if ((pTrident->Chipset == PROVIDIA9682 ||
	 pTrident->Chipset == TGUI9680) && rop == GXcopy) drawflag = FASTMODE;
    if (pTrident->Chipset == PROVIDIA9685) {
    	if (rop == GXcopy) drawflag |= 1<<21;
    	TGUI_FPATCOL(color);
    } else {
    	TGUI_FCOLOUR(color);
    }
    TGUI_DRAWFLAG(pTrident->DrawFlag | SOLIDFILL | PATMONO | drawflag);
}

static void
TridentSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TGUI_DIM_XY(w,h);
    TGUI_DEST_XY(x,y);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
}

static void MoveDWORDS(
   register CARD32* dest,
   register CARD32* src,
   register int dwords )
{
     while(dwords & ~0x03) {
	*dest = *src;
	*(dest + 1) = *(src + 1);
	*(dest + 2) = *(src + 2);
	*(dest + 3) = *(src + 3);
	src += 4;
	dest += 4;
	dwords -= 4;
     }	
     if (!dwords) return;
     *dest = *src;
     dest += 1;
     src += 1;
     if (dwords == 1) return;
     *dest = *src;
     dest += 1;
     src += 1;
     if (dwords == 2) return;
     *dest = *src;
     dest += 1;
     src += 1;
}

static void
TridentWritePixmap(
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int rop,
    unsigned int planemask,
    int transparency_color,
    int bpp, int depth
)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int dst = 0;

    TridentSetClippingRectangle(pScrn,(x*(bpp>>3)),y,((w+x)*(bpp>>3))-1,y+h-1);

    if (w & 7)
    	w += 8 - (w & 7);

    if (transparency_color != -1) {
	if (pTrident->Chipset == PROVIDIA9685) {
	    dst |= 1<<16;
	} else {
    	    TGUI_OPERMODE(pTrident->EngineOperation | DST_ENABLE);
	}
	REPLICATE(transparency_color);
	TGUI_CKEY(transparency_color);
    }
    TGUI_SRC_XY(0,0);
    TGUI_FMIX(XAACopyROP[rop]);
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    if (rop == GXcopy) {
    	if (pTrident->Chipset == PROVIDIA9685)
		dst |= 1<<30 | 1<<21;
	if ((pTrident->Chipset == PROVIDIA9682) ||
	    (pTrident->Chipset == TGUI9680))
		dst |= FASTMODE;
    }

    TGUI_DRAWFLAG(dst);
    TGUI_COMMAND(GE_BLT);

    if (pScrn->bitsPerPixel == 8)
    	w >>= 2;
    if (pScrn->bitsPerPixel == 16)
    	w >>= 1;

    while (h--) {
	MoveDWORDS((CARD32*)pTrident->FbBase,(CARD32*)src,w);
	src += srcwidth;
    }

    if (pTrident->UseGERetry)
    	SET_SYNC_FLAG(infoRec);
    else 
    	TridentSync(pScrn);

    TGUI_OPERMODE(pTrident->EngineOperation);
    TridentDisableClipping(pScrn);
}

static void 
TridentSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
					   int patternx, int patterny, 
					   int fg, int bg, int rop,
					   unsigned int planemask)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int drawflag = SRCMONO;

    REPLICATE(fg);
    if (pTrident->Chipset == PROVIDIA9685)
    	TGUI_FPATCOL(fg);
    else
    	TGUI_FCOLOUR(fg);

    if (bg == -1) {
	drawflag |= 1<<12;
	if (pTrident->Chipset == PROVIDIA9685)
    	    TGUI_BPATCOL(~fg);
	else
    	    TGUI_BCOLOUR(~fg);
    } else {
    	REPLICATE(bg);
	if (pTrident->Chipset == PROVIDIA9685)
    	    TGUI_BPATCOL(bg);
	else
    	    TGUI_BCOLOUR(bg);
    }

    if (pTrident->Chipset == PROVIDIA9685) {
	drawflag |= 7<<18;
        if (rop == GXcopy) drawflag |= 1<<21;
    }
    if ((pTrident->Chipset == PROVIDIA9682 ||
	 pTrident->Chipset == TGUI9680) && rop == GXcopy)
	drawflag |= FASTMODE;
    TGUI_DRAWFLAG(pTrident->DrawFlag | PAT2SCR | PATMONO | drawflag);
    TGUI_PATLOC(((patterny * pTrident->PatternLocation) +
		 (patternx * pScrn->bitsPerPixel / 8)) >> 6);
    TGUI_FMIX(XAAPatternROP[rop]);
}

static void 
TridentSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 	
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
  
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentSync(pScrn);
}

static void 
TridentSetupForColor8x8PatternFill(ScrnInfoPtr pScrn, 
					   int patternx, int patterny, 
					   int rop,
					   unsigned int planemask,
					   int transparency_color)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int drawflag = 0;

    if ((pTrident->Chipset == PROVIDIA9682 ||
	 pTrident->Chipset == TGUI9680) && rop == GXcopy)
	drawflag |= FASTMODE;

    if (transparency_color != -1) {
	if (pTrident->Chipset == PROVIDIA9685) {
	    drawflag |= 1<<16;
	} else {
    	    TGUI_OPERMODE(pTrident->EngineOperation | DST_ENABLE);
	}
	REPLICATE(transparency_color);
	TGUI_CKEY(transparency_color);
    }

    TGUI_DRAWFLAG(pTrident->DrawFlag | PAT2SCR | drawflag);
    TGUI_PATLOC(((patterny * pTrident->PatternLocation) +
		 (patternx * pScrn->bitsPerPixel / 8)) >> 6);
    TGUI_FMIX(XAAPatternROP[rop]);
}

static void 
TridentSubsequentColor8x8PatternFillRect(ScrnInfoPtr pScrn, 	
				   int patternx, int patterny,
				   int x, int y,
				   int w, int h)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
  
    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
    if (!pTrident->UseGERetry)
    	TridentClearSync(pScrn);
}

static void
TridentSetupForScanlineCPUToScreenColorExpandFill(
	ScrnInfoPtr pScrn,
	int fg, int bg, 
	int rop, 
	unsigned int planemask
){
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int drawflag = SRCMONO;

    REPLICATE(fg);
    TGUI_FCOLOUR(fg);
    if (bg == -1) {
	drawflag |= 1<<12;
    	TGUI_BCOLOUR(~fg);
    } else {
    	REPLICATE(bg);
    	TGUI_BCOLOUR(bg);
    }

    if (pTrident->Chipset == PROVIDIA9685) 
	drawflag |= 1<<30;
  
    if ((pTrident->Chipset == PROVIDIA9682 ||
	 pTrident->Chipset == TGUI9680) && rop == GXcopy)
	drawflag |= FASTMODE;
    TGUI_SRC_XY(0,0);
    TGUI_DRAWFLAG(drawflag);
    TGUI_FMIX(XAACopyROP[rop]);
}

static void
TridentSubsequentScanlineCPUToScreenColorExpandFill(
	ScrnInfoPtr pScrn,
	int x, int y, int w, int h,
	int skipleft
){
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int bpp = pScrn->bitsPerPixel >> 3;
    pTrident->dwords = (w + 31) >> 5;
    pTrident->height = h;

    if (pTrident->Chipset >= TGUI9660)
    	TridentSetClippingRectangle(pScrn,((x+skipleft)*bpp),y,
							((w+x)*bpp)-1,y+h-1);

    TGUI_DEST_XY(x,y);
    TGUI_DIM_XY(w,h);
    TGUI_COMMAND(GE_BLT);
}

static void
TridentSubsequentColorExpandScanline(ScrnInfoPtr pScrn, int bufno)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    XAAInfoRecPtr infoRec;
    infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);

    MoveDWORDS((CARD32 *)pTrident->FbBase, 
		(CARD32 *)pTrident->XAAScanlineColorExpandBuffers[0], 
			pTrident->dwords);

    pTrident->height--;
    if (!pTrident->height) {
    	if (!pTrident->UseGERetry)
	    TridentSync(pScrn);
	TridentDisableClipping(pScrn);
    }
}
