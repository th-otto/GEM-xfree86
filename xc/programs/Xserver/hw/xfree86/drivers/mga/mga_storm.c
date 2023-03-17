/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_storm.c,v 1.64 2000/02/12 20:45:24 dawes Exp $ */


/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/* For correct __inline__ usage */
#include "compiler.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that use XAA need this */
#include "xaa.h"
#include "xaalocal.h"
#include "xf86fbman.h"
#include "miline.h"
#include "servermd.h"

#ifdef XF86DRI
#include "cfb.h"
#include "GL/glxtokens.h"
#endif

#include "mga_bios.h"
#include "mga.h"
#include "mga_reg.h"
#include "mga_map.h"
#include "mga_macros.h"

#ifdef XF86DRI
#include "mga_dri.h"
#include "mga_dripriv.h"
#endif

static void MGANAME(SubsequentScreenToScreenCopy)(ScrnInfoPtr pScrn,
				int srcX, int srcY, int dstX, int dstY,
				int w, int h);
static void MGANAME(SubsequentScreenToScreenCopy_FastBlit)(ScrnInfoPtr pScrn,
				int srcX, int srcY, int dstX, int dstY,
				int w, int h);
static void MGANAME(SetupForCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn, int fg,
				int bg, int rop, unsigned int planemask);
static void MGANAME(SubsequentCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
static void MGANAME(SubsequentSolidFillRect)(ScrnInfoPtr pScrn,
					int x, int y, int w, int h);
static void MGANAME(SubsequentSolidFillTrap)(ScrnInfoPtr pScrn, int y, int h, 
				int left, int dxL, int dyL, int eL,
				int right, int dxR, int dyR, int eR);
static void MGANAME(SubsequentSolidHorVertLine) (ScrnInfoPtr pScrn,
                                int x, int y, int len, int dir);
static void MGANAME(SubsequentSolidTwoPointLine)(ScrnInfoPtr pScrn,
				int x1, int y1, int x2, int y2, int flags);
static void MGANAME(SetupForMono8x8PatternFill)(ScrnInfoPtr pScrn,
				int patx, int paty, int fg, int bg,
				int rop, unsigned int planemask);
static void MGANAME(SubsequentMono8x8PatternFillRect)(ScrnInfoPtr pScrn,
				int patx, int paty,
				int x, int y, int w, int h );
static void MGANAME(SubsequentMono8x8PatternFillRect_Additional)(
				ScrnInfoPtr pScrn, int patx, int paty,
				int x, int y, int w, int h );
static void MGANAME(SubsequentMono8x8PatternFillTrap)( ScrnInfoPtr pScrn,
				int patx, int paty, int y, int h, 
				int left, int dxL, int dyL, int eL, 
				int right, int dxR, int dyR, int eR);
static void MGANAME(SetupForImageWrite)(ScrnInfoPtr pScrn, int rop,
   				unsigned int planemask,
				int transparency_color, int bpp, int depth);
static void MGANAME(SubsequentImageWriteRect)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
#if PSZ != 24
static void MGANAME(SetupForPlanarScreenToScreenColorExpandFill)(
				ScrnInfoPtr pScrn, int fg, int bg, int rop, 
				unsigned int planemask);
static void MGANAME(SubsequentPlanarScreenToScreenColorExpandFill)(
				ScrnInfoPtr pScrn,
				int x, int y, int w, int h,
				int srcx, int srcy, int skipleft);
#endif
static void MGANAME(SetupForScreenToScreenColorExpandFill)(ScrnInfoPtr pScrn,
				int fg, int bg, int rop, 
				unsigned int planemask);
static void MGANAME(SubsequentScreenToScreenColorExpandFill)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h,
				int srcx, int srcy, int skipleft);
static void MGANAME(SetupForDashedLine)(ScrnInfoPtr pScrn, int fg, int bg, 
				int rop, unsigned int planemask, int length,
    				unsigned char *pattern);
static void MGANAME(SubsequentDashedTwoPointLine)(ScrnInfoPtr pScrn,
				int x1, int y1, int x2, int y2, 
				int flags, int phase);

#ifdef XF86DRI
void MGANAME(DRIInitBuffers)(WindowPtr pWin, 
			     RegionPtr prgn, CARD32 index);
void MGANAME(DRIMoveBuffers)(WindowPtr pParent, DDXPointRec ptOldOrg,
			     RegionPtr prgnSrc, CARD32 index);

#endif

extern void MGAWriteBitmapColorExpand(ScrnInfoPtr pScrn, int x, int y,
				int w, int h, unsigned char *src, int srcwidth,
				int skipleft, int fg, int bg, int rop,
				unsigned int planemask);
extern void MGAFillColorExpandRects(ScrnInfoPtr pScrn, int fg, int bg, int rop,
				unsigned int planemask, int nBox, BoxPtr pBox,
				int xorg, int yorg, PixmapPtr pPix);
extern void MGASetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1,
				int x2, int y2);
extern void MGADisableClipping(ScrnInfoPtr pScrn);
extern void MGAFillSolidRectsDMA(ScrnInfoPtr pScrn, int fg, int rop, 
				unsigned int planemask, int nBox, BoxPtr pBox);
extern void MGAFillSolidSpansDMA(ScrnInfoPtr pScrn, int fg, int rop, 
				unsigned int planemask, int n, DDXPointPtr ppt,
 				int *pwidth, int fSorted);
extern void MGAFillMono8x8PatternRectsTwoPass(ScrnInfoPtr pScrn, int fg, int bg,
 				int rop, unsigned int planemask, int nBox,
 				BoxPtr pBox, int pattern0, int pattern1, 
				int xorigin, int yorigin);
extern void MGANonTEGlyphRenderer(ScrnInfoPtr pScrn, int x, int y, int n,
				NonTEGlyphPtr glyphs, BoxPtr pbox,
				int fg, int rop, unsigned int planemask);
extern void MGAValidatePolyArc(GCPtr, unsigned long, DrawablePtr);
extern void MGAValidatePolyPoint(GCPtr, unsigned long, DrawablePtr);
extern void MGAFillCacheBltRects(ScrnInfoPtr, int, unsigned int, int, BoxPtr,
				int, int, XAACacheInfoPtr);
extern void MGATEGlyphRenderer(ScrnInfoPtr, int, int, int, int, int, int, 
				unsigned int **glyphs, int, int, int, int, 
				unsigned planemask);

Bool
MGANAME(AccelInit)(ScreenPtr pScreen) 
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    MGAPtr pMga = MGAPTR(pScrn);
    int maxFastBlitMem;
    BoxRec AvailFBArea;
#ifdef XF86DRI
    RegionRec MemRegion;
#endif

    pMga->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if(!infoPtr) return FALSE;

    pMga->MaxFastBlitY = 0;
    pMga->MaxBlitDWORDS = 0x40000 >> 5;

    switch (pMga->Chipset) {
    case PCI_CHIP_MGA2064:
    	pMga->AccelFlags = BLK_OPAQUE_EXPANSION | FASTBLT_BUG;
	break;
    case PCI_CHIP_MGA2164:
    case PCI_CHIP_MGA2164_AGP:
    	pMga->AccelFlags = BLK_OPAQUE_EXPANSION |
			   TRANSC_SOLID_FILL |
 			   USE_RECTS_FOR_LINES; 
        break;
    case PCI_CHIP_MGAG400:
	pMga->MaxBlitDWORDS = 0x400000 >> 5;
    case PCI_CHIP_MGAG200:
    case PCI_CHIP_MGAG200_PCI:
        pMga->AccelFlags = TRANSC_SOLID_FILL |
			   TWO_PASS_COLOR_EXPAND;

	if(pMga->FbMapSize > 8*1024*1024)
	   pMga->AccelFlags |= LARGE_ADDRESSES;
        break;
    case PCI_CHIP_MGA1064:
	pMga->AccelFlags = 0;
        break;
    case PCI_CHIP_MGAG100:
    default:
	pMga->AccelFlags = MGA_NO_PLANEMASK;
        break;
    }

    /* all should be able to use this now with the bug fixes */
    pMga->AccelFlags |= USE_LINEAR_EXPANSION;

#if PSZ == 24
    pMga->AccelFlags |= MGA_NO_PLANEMASK;
#endif

    if(pMga->HasSDRAM) {
	pMga->Atype = pMga->AtypeNoBLK = MGAAtypeNoBLK;
	pMga->AccelFlags &= ~TWO_PASS_COLOR_EXPAND;
    } else {
	pMga->Atype = MGAAtype;
	pMga->AtypeNoBLK = MGAAtypeNoBLK;
    }

    /* fill out infoPtr here */
    infoPtr->Flags = 	PIXMAP_CACHE | 
			OFFSCREEN_PIXMAPS |
			LINEAR_FRAMEBUFFER |
			MICROSOFT_ZERO_LINE_BIAS;

    /* sync */
    infoPtr->Sync = MGAStormSync;

    /* screen to screen copy */
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY;
    infoPtr->SetupForScreenToScreenCopy =
        	MGANAME(SetupForScreenToScreenCopy);
    infoPtr->SubsequentScreenToScreenCopy =
        	MGANAME(SubsequentScreenToScreenCopy);

    if(pMga->HasFBitBlt) {
	infoPtr->FillCacheBltRects = MGAFillCacheBltRects;
	infoPtr->FillCacheBltRectsFlags = NO_TRANSPARENCY;
    }

    /* solid fills */
    infoPtr->SetupForSolidFill = MGANAME(SetupForSolidFill);
    infoPtr->SubsequentSolidFillRect = MGANAME(SubsequentSolidFillRect);
    infoPtr->SubsequentSolidFillTrap = MGANAME(SubsequentSolidFillTrap);

    /* solid lines */
    infoPtr->SetupForSolidLine = infoPtr->SetupForSolidFill;
    infoPtr->SubsequentSolidHorVertLine =
		MGANAME(SubsequentSolidHorVertLine);
    infoPtr->SubsequentSolidTwoPointLine = 
		MGANAME(SubsequentSolidTwoPointLine);

    /* clipping */
    infoPtr->SetClippingRectangle = MGASetClippingRectangle;
    infoPtr->DisableClipping = MGADisableClipping;
    infoPtr->ClippingFlags = 	HARDWARE_CLIP_SOLID_LINE  |
				HARDWARE_CLIP_DASHED_LINE |
				HARDWARE_CLIP_SOLID_FILL  |
				HARDWARE_CLIP_MONO_8x8_FILL; 

    /* dashed lines */
    infoPtr->DashedLineFlags = LINE_PATTERN_MSBFIRST_LSBJUSTIFIED;
    infoPtr->SetupForDashedLine = MGANAME(SetupForDashedLine);
    infoPtr->SubsequentDashedTwoPointLine =  
		MGANAME(SubsequentDashedTwoPointLine);
    infoPtr->DashPatternMaxLength = 128;


    /* 8x8 mono patterns */
    infoPtr->Mono8x8PatternFillFlags = HARDWARE_PATTERN_PROGRAMMED_BITS |
					HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
					HARDWARE_PATTERN_SCREEN_ORIGIN |
					BIT_ORDER_IN_BYTE_MSBFIRST;
    infoPtr->SetupForMono8x8PatternFill = MGANAME(SetupForMono8x8PatternFill);
    infoPtr->SubsequentMono8x8PatternFillRect = 
		MGANAME(SubsequentMono8x8PatternFillRect);
    infoPtr->SubsequentMono8x8PatternFillTrap = 
		MGANAME(SubsequentMono8x8PatternFillTrap);

    /* cpu to screen color expansion */
    infoPtr->CPUToScreenColorExpandFillFlags = 	CPU_TRANSFER_PAD_DWORD |
					SCANLINE_PAD_DWORD |
					BIT_ORDER_IN_BYTE_LSBFIRST |
					LEFT_EDGE_CLIPPING |
					LEFT_EDGE_CLIPPING_NEGATIVE_X |
					SYNC_AFTER_COLOR_EXPAND;
    if(pMga->ILOADBase) {
	infoPtr->ColorExpandRange = 0x800000;
	infoPtr->ColorExpandBase = pMga->ILOADBase;
    } else {
	infoPtr->ColorExpandRange = 0x1C00;
#ifdef __alpha__
	infoPtr->ColorExpandBase = pMga->IOBaseDense;
#else
	infoPtr->ColorExpandBase = pMga->IOBase;
#endif
    }
    infoPtr->SetupForCPUToScreenColorExpandFill =
		MGANAME(SetupForCPUToScreenColorExpandFill);
    infoPtr->SubsequentCPUToScreenColorExpandFill =
		MGANAME(SubsequentCPUToScreenColorExpandFill);


    /* screen to screen color expansion */
    if(pMga->AccelFlags & USE_LINEAR_EXPANSION) {
	infoPtr->ScreenToScreenColorExpandFillFlags = 
						BIT_ORDER_IN_BYTE_LSBFIRST;
	infoPtr->SetupForScreenToScreenColorExpandFill = 
		MGANAME(SetupForScreenToScreenColorExpandFill);
	infoPtr->SubsequentScreenToScreenColorExpandFill = 
		MGANAME(SubsequentScreenToScreenColorExpandFill);
    } else {
#if PSZ != 24
    /* Alternate (but slower) planar expansions */
	infoPtr->SetupForScreenToScreenColorExpandFill = 
		MGANAME(SetupForPlanarScreenToScreenColorExpandFill);
	infoPtr->SubsequentScreenToScreenColorExpandFill = 
		MGANAME(SubsequentPlanarScreenToScreenColorExpandFill);
	infoPtr->CacheColorExpandDensity = PSZ;
	infoPtr->CacheMonoStipple = XAACachePlanarMonoStipple;
	/* It's faster to blit the stipples if you have fastbilt */
	if(pMga->HasFBitBlt)
	    infoPtr->ScreenToScreenColorExpandFillFlags = TRANSPARENCY_ONLY;
#endif
    }

    /* image writes */
    infoPtr->ImageWriteFlags = 	CPU_TRANSFER_PAD_DWORD |
				SCANLINE_PAD_DWORD |
				LEFT_EDGE_CLIPPING |
				LEFT_EDGE_CLIPPING_NEGATIVE_X |
				SYNC_AFTER_IMAGE_WRITE;

    /* if we're write combining */ 
    infoPtr->ImageWriteFlags |= NO_GXCOPY;

    if(pMga->ILOADBase) {
	infoPtr->ImageWriteRange = 0x800000;
	infoPtr->ImageWriteBase = pMga->ILOADBase;
    } else {
	infoPtr->ImageWriteRange = 0x1C00;
	infoPtr->ImageWriteBase = pMga->IOBase;
    }
    infoPtr->SetupForImageWrite = MGANAME(SetupForImageWrite);
    infoPtr->SubsequentImageWriteRect = MGANAME(SubsequentImageWriteRect);


    /* midrange replacements */

    if(infoPtr->SetupForCPUToScreenColorExpandFill && 
			infoPtr->SubsequentCPUToScreenColorExpandFill) {
	infoPtr->FillColorExpandRects = MGAFillColorExpandRects; 
	infoPtr->WriteBitmap = MGAWriteBitmapColorExpand;
	infoPtr->TEGlyphRenderer = MGATEGlyphRenderer;  
        if(BITMAP_SCANLINE_PAD == 32)
	    infoPtr->NonTEGlyphRenderer = MGANonTEGlyphRenderer;  
    }

    if(pMga->ILOADBase && pMga->UsePCIRetry && infoPtr->SetupForSolidFill) {
	infoPtr->FillSolidRects = MGAFillSolidRectsDMA;
	infoPtr->FillSolidSpans = MGAFillSolidSpansDMA;
    }

    if(pMga->AccelFlags & TWO_PASS_COLOR_EXPAND) {
	if(infoPtr->SetupForMono8x8PatternFill)
	    infoPtr->FillMono8x8PatternRects = 
				MGAFillMono8x8PatternRectsTwoPass;
    }

    if(infoPtr->SetupForSolidFill) {
	infoPtr->ValidatePolyArc = MGAValidatePolyArc;
	infoPtr->PolyArcMask = GCFunction | GCLineWidth | GCPlaneMask | 
				GCLineStyle | GCFillStyle;
	infoPtr->ValidatePolyPoint = MGAValidatePolyPoint;
	infoPtr->PolyPointMask = GCFunction | GCPlaneMask;
    }

    if(pMga->AccelFlags & MGA_NO_PLANEMASK) {
	infoPtr->ImageWriteFlags |= NO_PLANEMASK;
	infoPtr->ScreenToScreenCopyFlags |= NO_PLANEMASK;
	infoPtr->CPUToScreenColorExpandFillFlags |= NO_PLANEMASK;
	infoPtr->WriteBitmapFlags |= NO_PLANEMASK;
	infoPtr->SolidFillFlags |= NO_PLANEMASK;
	infoPtr->SolidLineFlags |= NO_PLANEMASK;
	infoPtr->DashedLineFlags |= NO_PLANEMASK;
	infoPtr->Mono8x8PatternFillFlags |= NO_PLANEMASK; 
	infoPtr->FillColorExpandRectsFlags |= NO_PLANEMASK; 
	infoPtr->ScreenToScreenColorExpandFillFlags |= NO_PLANEMASK;
	infoPtr->FillSolidRectsFlags |= NO_PLANEMASK;
	infoPtr->FillSolidSpansFlags |= NO_PLANEMASK;
	infoPtr->FillMono8x8PatternRectsFlags |= NO_PLANEMASK;
	infoPtr->NonTEGlyphRendererFlags |= NO_PLANEMASK;
	infoPtr->TEGlyphRendererFlags |= NO_PLANEMASK;
	infoPtr->FillCacheBltRectsFlags |= NO_PLANEMASK;
    }

    
    maxFastBlitMem = (pMga->Interleave ? 4096 : 2048) * 1024;

    if(pMga->FbMapSize > maxFastBlitMem) {
	pMga->MaxFastBlitY = maxFastBlitMem / (pScrn->displayWidth * PSZ / 8);
    }


    AvailFBArea.x1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;

#ifndef XF86DRI
    AvailFBArea.y1 = 0;
    AvailFBArea.y2 = (min(pMga->FbUsableSize, 16*1024*1024)) / 
			(pScrn->displayWidth * PSZ / 8);
   
    xf86InitFBManager(pScreen, &AvailFBArea); 
#else
    AvailFBArea.y1 = pScrn->virtualY;
    AvailFBArea.y2 = pScrn->virtualY+128;
   
    REGION_INIT(pScreen, &MemRegion, &AvailFBArea, 1);
    xf86InitFBManagerRegion(pScreen, &MemRegion);
    REGION_UNINIT(pScreen, &MemRegion);
#endif
   

    return(XAAInit(pScreen, infoPtr));
}

#if PSZ == 8

CARD32 MGAAtype[16] = {
   MGADWG_RPL  | 0x00000000, MGADWG_RSTR | 0x00080000, 
   MGADWG_RSTR | 0x00040000, MGADWG_BLK  | 0x000c0000,
   MGADWG_RSTR | 0x00020000, MGADWG_RSTR | 0x000a0000, 
   MGADWG_RSTR | 0x00060000, MGADWG_RSTR | 0x000e0000,
   MGADWG_RSTR | 0x00010000, MGADWG_RSTR | 0x00090000, 
   MGADWG_RSTR | 0x00050000, MGADWG_RSTR | 0x000d0000,
   MGADWG_RPL  | 0x00030000, MGADWG_RSTR | 0x000b0000, 
   MGADWG_RSTR | 0x00070000, MGADWG_RPL  | 0x000f0000
};


CARD32 MGAAtypeNoBLK[16] = {
   MGADWG_RPL  | 0x00000000, MGADWG_RSTR | 0x00080000, 
   MGADWG_RSTR | 0x00040000, MGADWG_RPL  | 0x000c0000,
   MGADWG_RSTR | 0x00020000, MGADWG_RSTR | 0x000a0000, 
   MGADWG_RSTR | 0x00060000, MGADWG_RSTR | 0x000e0000,
   MGADWG_RSTR | 0x00010000, MGADWG_RSTR | 0x00090000, 
   MGADWG_RSTR | 0x00050000, MGADWG_RSTR | 0x000d0000,
   MGADWG_RPL  | 0x00030000, MGADWG_RSTR | 0x000b0000, 
   MGADWG_RSTR | 0x00070000, MGADWG_RPL  | 0x000f0000
};


Bool 
MGAStormAccelInit(ScreenPtr pScreen)
{ 
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    switch( pScrn->bitsPerPixel )
    {
    case 8:
    	return Mga8AccelInit(pScreen);
    case 16:
    	return Mga16AccelInit(pScreen);
    case 24:
    	return Mga24AccelInit(pScreen);
    case 32:
    	return Mga32AccelInit(pScreen);
    }
    return FALSE;
}



void
MGAStormSync(ScrnInfoPtr pScrn) 
{
    MGAPtr pMga = MGAPTR(pScrn);
    
    while(MGAISBUSY());
    /* flush cache before a read (mga-1064g 5.1.6) */
    OUTREG8(MGAREG_CRTC_INDEX, 0); 
    if(pMga->AccelFlags & CLIPPER_ON) {
        pMga->AccelFlags &= ~CLIPPER_ON;
        OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);
    }
}

void MGAStormEngineInit(ScrnInfoPtr pScrn)
{
    long maccess = 0;
    MGAPtr pMga = MGAPTR(pScrn);
    MGAFBLayout *pLayout = &pMga->CurrentLayout;

    if (pMga->Chipset == PCI_CHIP_MGAG100)
    	maccess = 1 << 14;
    
    switch( pLayout->bitsPerPixel )
    {
    case 8:
        break;
    case 16:
	/* set 16 bpp, turn off dithering, turn on 5:5:5 pixels */
        maccess |= 1 + (1 << 30) + (1 << 31);
        break;
    case 24:
        maccess |= 3;
        break;
    case 32:
        maccess |= 2;
        break;
    }
    
    pMga->fifoCount = 0;

    while(MGAISBUSY());

    if(!pMga->FifoSize) {
	pMga->FifoSize = INREG8(MGAREG_FIFOSTATUS);
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "%i DWORD fifo\n",
						pMga->FifoSize);
    }

    OUTREG(MGAREG_PITCH, pLayout->displayWidth);
    OUTREG(MGAREG_YDSTORG, pMga->YDstOrg);
    OUTREG(MGAREG_MACCESS, maccess);
    pMga->MAccess = maccess;
    pMga->PlaneMask = ~0;
    if(pMga->Chipset != PCI_CHIP_MGAG100)
	OUTREG(MGAREG_PLNWT, pMga->PlaneMask);
    pMga->FgColor = 0;
    OUTREG(MGAREG_FCOL, pMga->FgColor);
    pMga->BgColor = 0;
    OUTREG(MGAREG_BCOL, pMga->BgColor);
    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);

    /* put clipping in a known state */
    OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);	/* (maxX << 16) | minX */ 
    OUTREG(MGAREG_YTOP, 0x00000000);	/* minPixelPointer */ 
    OUTREG(MGAREG_YBOT, 0x007FFFFF);	/* maxPixelPointer */ 
    pMga->AccelFlags &= ~CLIPPER_ON;

    switch(pMga->Chipset) {
    case PCI_CHIP_MGAG400:
    case PCI_CHIP_MGAG200:
    case PCI_CHIP_MGAG200_PCI:
	pMga->SrcOrg = 0;
	pMga->DstOrg = 0;
	OUTREG(MGAREG_SRCORG, 0);
	OUTREG(MGAREG_DSTORG, 0);
	break;
    default:
	break;
    }
}

void MGASetClippingRectangle(
   ScrnInfoPtr pScrn,
   int x1, int y1, int x2, int y2
){
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(3);
    OUTREG(MGAREG_CXBNDRY,(x2 << 16) | x1);      
    OUTREG(MGAREG_YTOP, (y1 * pScrn->displayWidth) + pMga->YDstOrg); 
    OUTREG(MGAREG_YBOT, (y2 * pScrn->displayWidth) + pMga->YDstOrg); 
    pMga->AccelFlags |= CLIPPER_ON;
}

void MGADisableClipping(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(3);
    OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);     /* (maxX << 16) | minX */ 
    OUTREG(MGAREG_YTOP, 0x00000000);        /* minPixelPointer */ 
    OUTREG(MGAREG_YBOT, 0x007FFFFF);        /* maxPixelPointer */ 
    pMga->AccelFlags &= ~CLIPPER_ON;
}


#endif


	/*********************************************\
	|            Screen-to-Screen Copy            |
	\*********************************************/

#define BLIT_LEFT	1
#define BLIT_UP		4

void 
MGANAME(SetupForScreenToScreenCopy)(
    ScrnInfoPtr pScrn,
    int xdir, int ydir,
    int rop,
    unsigned int planemask,
    int trans 
){
    MGAPtr pMga = MGAPTR(pScrn);
    CARD32 dwgctl = pMga->AtypeNoBLK[rop] | MGADWG_SHIFTZERO | 
			MGADWG_BITBLT | MGADWG_BFCOL;

    pMga->AccelInfoRec->SubsequentScreenToScreenCopy = 
		MGANAME(SubsequentScreenToScreenCopy);

    pMga->BltScanDirection = 0;
    if(ydir == -1) pMga->BltScanDirection |= BLIT_UP;
    if(xdir == -1)
	pMga->BltScanDirection |= BLIT_LEFT;
    else if(pMga->HasFBitBlt && (rop == GXcopy) && !pMga->DrawTransparent) 
	pMga->AccelInfoRec->SubsequentScreenToScreenCopy = 
		MGANAME(SubsequentScreenToScreenCopy_FastBlit);

    if(pMga->DrawTransparent) { 
	dwgctl |= MGADWG_TRANSC;
	WAITFIFO(2);
	SET_FOREGROUND(trans);
	trans = ~0;
	SET_BACKGROUND(trans);
    } 

    WAITFIFO(4); 
    OUTREG(MGAREG_DWGCTL, dwgctl);
    OUTREG(MGAREG_SGN, pMga->BltScanDirection);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_AR5, ydir * pMga->CurrentLayout.displayWidth);
}


static void 
MGANAME(SubsequentScreenToScreenCopy)(
    ScrnInfoPtr pScrn,
    int srcX, int srcY, int dstX, int dstY, int w, int h
){
    int start, end, SrcOrg = 0, DstOrg = 0;
    MGAPtr pMga = MGAPTR(pScrn);

    if (pMga->AccelFlags & LARGE_ADDRESSES) {
	SrcOrg = ((srcY & ~1023) * pMga->CurrentLayout.displayWidth * PSZ) >> 9;
	DstOrg = ((dstY & ~1023) * pMga->CurrentLayout.displayWidth * PSZ) >> 9;
        dstY &= 1023;
    }

    if(pMga->BltScanDirection & BLIT_UP) {
	srcY += h - 1;
	dstY += h - 1;
    }

    w--;
    start = end = XYADDRESS(srcX, srcY);

    if(pMga->BltScanDirection & BLIT_LEFT) start += w;
    else end += w; 

    if (pMga->AccelFlags & LARGE_ADDRESSES) {
	WAITFIFO(7); 
	if(DstOrg)
	    OUTREG(MGAREG_DSTORG, DstOrg << 6);
	if(SrcOrg != pMga->SrcOrg) {
	    pMga->SrcOrg = SrcOrg;
	    OUTREG(MGAREG_SRCORG, SrcOrg << 6);
 	}
	if(SrcOrg) {
	    SrcOrg = (SrcOrg << 9) / PSZ;
	    end -= SrcOrg;
	    start -= SrcOrg;
	}
	OUTREG(MGAREG_AR0, end);
	OUTREG(MGAREG_AR3, start);
	OUTREG(MGAREG_FXBNDRY, ((dstX + w) << 16) | (dstX & 0xffff));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
	if(DstOrg)
	   OUTREG(MGAREG_DSTORG, 0);
    } else {
	WAITFIFO(4); 
	OUTREG(MGAREG_AR0, end);
	OUTREG(MGAREG_AR3, start);
	OUTREG(MGAREG_FXBNDRY, ((dstX + w) << 16) | (dstX & 0xffff));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
    }
}


static void 
MGANAME(SubsequentScreenToScreenCopy_FastBlit)(
    ScrnInfoPtr pScrn,
    int srcX, int srcY, int dstX, int dstY, int w, int h
)
{
    int start, end;
    MGAPtr pMga = MGAPTR(pScrn);

    if(pMga->BltScanDirection & BLIT_UP) {
	srcY += h - 1;
	dstY += h - 1;
    }

    w--;
    start = XYADDRESS(srcX, srcY);
    end = start + w;

    /* we assume the driver asserts screen pitches such that
	we can always use fastblit for scrolling */
    if(
#if PSZ == 32
        !((srcX ^ dstX) & 31)
#elif PSZ == 16
        !((srcX ^ dstX) & 63)
#else
        !((srcX ^ dstX) & 127)
#endif
    	) {
	if(pMga->MaxFastBlitY) {
	   if(pMga->BltScanDirection & BLIT_UP) {
		if((srcY >= pMga->MaxFastBlitY) ||
				(dstY >= pMga->MaxFastBlitY)) 
			goto FASTBLIT_BAILOUT;
	   } else {
		if(((srcY + h) > pMga->MaxFastBlitY) ||
				((dstY + h) > pMga->MaxFastBlitY)) 
			goto FASTBLIT_BAILOUT;
	   }
	}

	/* Millennium 1 fastblit bug fix */
        if(pMga->AccelFlags & FASTBLT_BUG) {
    	   int fxright = dstX + w;
#if PSZ == 8
           if((dstX & (1 << 6)) && (((fxright >> 6) - (dstX >> 6)) & 7) == 7) {
		fxright |= 1 << 6;
#elif PSZ == 16
           if((dstX & (1 << 5)) && (((fxright >> 5) - (dstX >> 5)) & 7) == 7) {
		fxright |= 1 << 5;
#elif PSZ == 24
           if(((dstX * 3) & (1 << 6)) && 
                 ((((fxright * 3 + 2) >> 6) - ((dstX * 3) >> 6)) & 7) == 7) {
		fxright = ((fxright * 3 + 2) | (1 << 6)) / 3;
#elif PSZ == 32
           if((dstX & (1 << 4)) && (((fxright >> 4) - (dstX >> 4)) & 7) == 7) {
		fxright |= 1 << 4;
#endif
		WAITFIFO(8);
		OUTREG(MGAREG_CXRIGHT, dstX + w);
		OUTREG(MGAREG_DWGCTL, 0x040A400C);
		OUTREG(MGAREG_AR0, end);
		OUTREG(MGAREG_AR3, start);
		OUTREG(MGAREG_FXBNDRY, (fxright << 16) | (dstX & 0xffff));
		OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
		OUTREG(MGAREG_DWGCTL, pMga->AtypeNoBLK[GXcopy] | 
			MGADWG_SHIFTZERO | MGADWG_BITBLT | MGADWG_BFCOL);
		OUTREG(MGAREG_CXRIGHT, 0xFFFF);
	    	return;
	    } /* } } } (preserve pairs for pair matching) */
	}
	
   	WAITFIFO(6); 
    	OUTREG(MGAREG_DWGCTL, 0x040A400C);
    	OUTREG(MGAREG_AR0, end);
    	OUTREG(MGAREG_AR3, start);
    	OUTREG(MGAREG_FXBNDRY, ((dstX + w) << 16) | (dstX & 0xffff));
    	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
    	OUTREG(MGAREG_DWGCTL, pMga->AtypeNoBLK[GXcopy] | MGADWG_SHIFTZERO | 
			MGADWG_BITBLT | MGADWG_BFCOL);
	return;
    }  

FASTBLIT_BAILOUT:
   
    WAITFIFO(4); 
    OUTREG(MGAREG_AR0, end);
    OUTREG(MGAREG_AR3, start);
    OUTREG(MGAREG_FXBNDRY, ((dstX + w) << 16) | (dstX & 0xffff));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
}

        
        /******************\
	|   Solid Fills    |
	\******************/

void 
MGANAME(SetupForSolidFill)(
	ScrnInfoPtr pScrn,
	int color,
	int rop,
	unsigned int planemask )
{
    MGAPtr pMga = MGAPTR(pScrn);

#if PSZ == 24
    if(!RGBEQUAL(color))
    pMga->FilledRectCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | pMga->AtypeNoBLK[rop];
    else
#endif
    pMga->FilledRectCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | pMga->Atype[rop];

    pMga->SolidLineCMD =  MGADWG_SOLID | MGADWG_SHIFTZERO | MGADWG_BFCOL | 
		    pMga->AtypeNoBLK[rop];

    if(pMga->AccelFlags & TRANSC_SOLID_FILL) 
	pMga->FilledRectCMD |= MGADWG_TRANSC;

    WAITFIFO(3);
    SET_FOREGROUND(color);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_DWGCTL, pMga->FilledRectCMD);
}    

static void 
MGANAME(SubsequentSolidFillRect)(
	ScrnInfoPtr pScrn,
	int x, int y, int w, int h )
{
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(2);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | (x & 0xffff));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}   

static void 
MGANAME(SubsequentSolidFillTrap)(ScrnInfoPtr pScrn, int y, int h, 
	int left, int dxL, int dyL, int eL,
	int right, int dxR, int dyR, int eR )
{
    MGAPtr pMga = MGAPTR(pScrn);
    int sdxl = (dxL < 0);
    int ar2 = sdxl? dxL : -dxL;
    int sdxr = (dxR < 0);
    int ar5 = sdxr? dxR : -dxR;
    
    WAITFIFO(11);
    OUTREG(MGAREG_DWGCTL, 
		pMga->FilledRectCMD & ~(MGADWG_ARZERO | MGADWG_SGNZERO));
    OUTREG(MGAREG_AR0, dyL);
    OUTREG(MGAREG_AR1, ar2 - eL);
    OUTREG(MGAREG_AR2, ar2);
    OUTREG(MGAREG_AR4, ar5 - eR);
    OUTREG(MGAREG_AR5, ar5);
    OUTREG(MGAREG_AR6, dyR);
    OUTREG(MGAREG_SGN, (sdxl << 1) | (sdxr << 5));
    OUTREG(MGAREG_FXBNDRY, ((right + 1) << 16) | (left & 0xffff));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    OUTREG(MGAREG_DWGCTL, pMga->FilledRectCMD);
}

	/***************\
	|  Solid Lines  |
	\***************/


static void 
MGANAME(SubsequentSolidHorVertLine) (
    ScrnInfoPtr pScrn,
    int x, int y, 
    int len, int dir
){
    MGAPtr pMga = MGAPTR(pScrn);

    if(dir == DEGREES_0) {
	WAITFIFO(2);
	OUTREG(MGAREG_FXBNDRY, ((x + len) << 16) | (x & 0xffff));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | 1);
    } else if(pMga->AccelFlags & USE_RECTS_FOR_LINES) {
	WAITFIFO(2);
	OUTREG(MGAREG_FXBNDRY, ((x + 1) << 16) | (x & 0xffff));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | len);
    } else {
	WAITFIFO(4);
	OUTREG(MGAREG_DWGCTL, pMga->SolidLineCMD | MGADWG_AUTOLINE_OPEN);
	OUTREG(MGAREG_XYSTRT, (y << 16) | (x & 0xffff));
	OUTREG(MGAREG_XYEND + MGAREG_EXEC, ((y + len) << 16) | (x & 0xffff));
	OUTREG(MGAREG_DWGCTL, pMga->FilledRectCMD); 
    }
}


static void 
MGANAME(SubsequentSolidTwoPointLine)(
   ScrnInfoPtr pScrn,
   int x1, int y1, int x2, int y2, int flags
){
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(4);
    OUTREG(MGAREG_DWGCTL, pMga->SolidLineCMD | 
        ((flags & OMIT_LAST) ? MGADWG_AUTOLINE_OPEN : MGADWG_AUTOLINE_CLOSE));
    OUTREG(MGAREG_XYSTRT, (y1 << 16) | (x1 & 0xFFFF));
    OUTREG(MGAREG_XYEND + MGAREG_EXEC, (y2 << 16) | (x2 & 0xFFFF));
    OUTREG(MGAREG_DWGCTL, pMga->FilledRectCMD); 
}



	/***************************\
	|   8x8 Mono Pattern Fills  |
	\***************************/


static void 
MGANAME(SetupForMono8x8PatternFill)(
   ScrnInfoPtr pScrn,
   int patx, int paty,
   int fg, int bg,
   int rop,
   unsigned int planemask )
{
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;

    pMga->PatternRectCMD = MGADWG_TRAP | MGADWG_ARZERO | MGADWG_SGNZERO | 
						MGADWG_BMONOLEF;

    infoRec->SubsequentMono8x8PatternFillRect = 
		MGANAME(SubsequentMono8x8PatternFillRect);
    
    if(bg == -1) {
#if PSZ == 24
    	if(!RGBEQUAL(fg))
            pMga->PatternRectCMD |= MGADWG_TRANSC | pMga->AtypeNoBLK[rop];
	else
#endif
            pMga->PatternRectCMD |= MGADWG_TRANSC | pMga->Atype[rop];

	WAITFIFO(5);
    } else {
#if PSZ == 24
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if(pMga->AccelFlags & BLK_OPAQUE_EXPANSION) 
#endif
        	pMga->PatternRectCMD |= pMga->Atype[rop];
	else
        	pMga->PatternRectCMD |= pMga->AtypeNoBLK[rop];
	WAITFIFO(6);
    	SET_BACKGROUND(bg);
    }

    SET_FOREGROUND(fg);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_DWGCTL, pMga->PatternRectCMD);
    OUTREG(MGAREG_PAT0, patx);
    OUTREG(MGAREG_PAT1, paty);
}



static void 
MGANAME(SubsequentMono8x8PatternFillRect)(
    ScrnInfoPtr pScrn,
    int patx, int paty,
    int x, int y, int w, int h )
{
    MGAPtr pMga = MGAPTR(pScrn);
    
    WAITFIFO(3);
    OUTREG(MGAREG_SHIFT, (paty << 4) | patx);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | (x & 0xffff));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    pMga->AccelInfoRec->SubsequentMono8x8PatternFillRect = 
		MGANAME(SubsequentMono8x8PatternFillRect_Additional);
}

static void 
MGANAME(SubsequentMono8x8PatternFillRect_Additional)(
    ScrnInfoPtr pScrn,
    int patx, int paty,
    int x, int y, int w, int h )
{
    MGAPtr pMga = MGAPTR(pScrn);
    WAITFIFO(2);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | (x & 0xffff));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}


static void 
MGANAME(SubsequentMono8x8PatternFillTrap)(
    ScrnInfoPtr pScrn,
    int patx, int paty,	
    int y, int h, 
    int left, int dxL, int dyL, int eL, 
    int right, int dxR, int dyR, int eR
){
    MGAPtr pMga = MGAPTR(pScrn);

    int sdxl = (dxL < 0) ? (1<<1) : 0;
    int ar2 = sdxl? dxL : -dxL;
    int sdxr = (dxR < 0) ? (1<<5) : 0;
    int ar5 = sdxr? dxR : -dxR;

    WAITFIFO(12);
    OUTREG(MGAREG_SHIFT, (paty << 4) | patx);
    OUTREG(MGAREG_DWGCTL, 
	pMga->PatternRectCMD & ~(MGADWG_ARZERO | MGADWG_SGNZERO));
    OUTREG(MGAREG_AR0, dyL);
    OUTREG(MGAREG_AR1, ar2 - eL);
    OUTREG(MGAREG_AR2, ar2);
    OUTREG(MGAREG_AR4, ar5 - eR);
    OUTREG(MGAREG_AR5, ar5);
    OUTREG(MGAREG_AR6, dyR);
    OUTREG(MGAREG_SGN, sdxl | sdxr);
    OUTREG(MGAREG_FXBNDRY, ((right + 1) << 16) | (left & 0xffff));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    OUTREG(MGAREG_DWGCTL, pMga->PatternRectCMD);
}

	/***********************\
	|   Color Expand Rect   |
	\***********************/


static void 
MGANAME(SetupForCPUToScreenColorExpandFill)(
	ScrnInfoPtr pScrn,
	int fg, int bg,
	int rop,
	unsigned int planemask )
{
    MGAPtr pMga = MGAPTR(pScrn);

    CARD32 mgaCMD = MGADWG_ILOAD | MGADWG_LINEAR | MGADWG_SGNZERO | 
			MGADWG_SHIFTZERO | MGADWG_BMONOLEF;
        
    if(bg == -1) {
#if PSZ == 24
    	if(!RGBEQUAL(fg))
            mgaCMD |= MGADWG_TRANSC | pMga->AtypeNoBLK[rop];
	else
#endif
            mgaCMD |= MGADWG_TRANSC | pMga->Atype[rop];

	WAITFIFO(3);
    } else {
#if PSZ == 24
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && 
		RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if(pMga->AccelFlags & BLK_OPAQUE_EXPANSION) 
#endif
        	mgaCMD |= pMga->Atype[rop];
	else
        	mgaCMD |= pMga->AtypeNoBLK[rop];
	WAITFIFO(4);
    	SET_BACKGROUND(bg);
    }

    SET_FOREGROUND(fg);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_DWGCTL, mgaCMD);
}       

static void 
MGANAME(SubsequentCPUToScreenColorExpandFill)(
	ScrnInfoPtr pScrn,
	int x, int y, int w, int h,
	int skipleft
){
    MGAPtr pMga = MGAPTR(pScrn);

    pMga->AccelFlags |= CLIPPER_ON;
    WAITFIFO(5);
    OUTREG(MGAREG_CXBNDRY, ((x + w - 1) << 16) | ((x + skipleft) & 0xFFFF));
    w = (w + 31) & ~31;     /* source is dword padded */
    OUTREG(MGAREG_AR0, (w * h) - 1);
    OUTREG(MGAREG_AR3, 0);  /* crashes occasionally without this */
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | (x & 0xFFFF));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

	/*******************\
	|   Image Writes    |
	\*******************/


static void MGANAME(SetupForImageWrite)(	
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int transparency_color,
   int bpp, int depth
){
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(3);
    OUTREG(MGAREG_AR5, 0);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_DWGCTL, MGADWG_ILOAD | MGADWG_BFCOL | MGADWG_SHIFTZERO |
			MGADWG_SGNZERO | pMga->AtypeNoBLK[rop]);
}


static void MGANAME(SubsequentImageWriteRect)(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   int skipleft
){
    MGAPtr pMga = MGAPTR(pScrn);

    pMga->AccelFlags |= CLIPPER_ON;

    WAITFIFO(5);
    OUTREG(MGAREG_CXBNDRY, 0xFFFF0000 | ((x + skipleft) & 0xFFFF));
    OUTREG(MGAREG_AR0, w - 1);
    OUTREG(MGAREG_AR3, 0);
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | (x & 0xFFFF));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}


	/***************************\
	|      Dashed  Lines        |
	\***************************/


void
MGANAME(SetupForDashedLine)(
    ScrnInfoPtr pScrn, 
    int fg, int bg, int rop,
    unsigned int planemask,
    int length,
    unsigned char *pattern
){
    MGAPtr pMga = MGAPTR(pScrn);
    CARD32 *DashPattern = (CARD32*)pattern;
    CARD32 NiceDashPattern = DashPattern[0];
    int dwords = (length + 31) >> 5;

    pMga->DashCMD = MGADWG_BFCOL | pMga->AtypeNoBLK[rop];
    pMga->StyleLen = length - 1;

    if(bg == -1) {
        pMga->DashCMD |= MGADWG_TRANSC;
	WAITFIFO(dwords + 2);
    } else {
	WAITFIFO(dwords + 3);
	SET_BACKGROUND(bg);
    }
    SET_PLANEMASK(planemask);
    SET_FOREGROUND(fg);

    /* We see if we can draw horizontal lines as 8x8 pattern fills.
	This is worthwhile since the pattern fills can use block mode
	and the default X pattern is 8 pixels long.  The forward pattern
	is the top scanline, the backwards pattern is the next one. */
    switch(length) {
	case 2:	NiceDashPattern |= NiceDashPattern << 2;
	case 4:	NiceDashPattern |= NiceDashPattern << 4;
	case 8:	NiceDashPattern |= byte_reversed[NiceDashPattern] << 16;
		NiceDashPattern |= NiceDashPattern << 8;
		pMga->NiceDashCMD = MGADWG_TRAP | MGADWG_ARZERO | 
				    MGADWG_SGNZERO | MGADWG_BMONOLEF;
     		pMga->AccelFlags |= NICE_DASH_PATTERN;
   		if(bg == -1) {
#if PSZ == 24
    		   if(!RGBEQUAL(fg))
            		pMga->NiceDashCMD |= MGADWG_TRANSC | pMga->AtypeNoBLK[rop];
		   else
#endif
           		pMga->NiceDashCMD |= MGADWG_TRANSC | pMga->Atype[rop];
    		} else {
#if PSZ == 24
		   if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && 
					RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
		   if(pMga->AccelFlags & BLK_OPAQUE_EXPANSION) 
#endif
        		pMga->NiceDashCMD |= pMga->Atype[rop];
		   else
        		pMga->NiceDashCMD |= pMga->AtypeNoBLK[rop];
    		}
		OUTREG(MGAREG_SRC0, NiceDashPattern);
		break;
	default: pMga->AccelFlags &= ~NICE_DASH_PATTERN;
		switch (dwords) {
		case 4:  OUTREG(MGAREG_SRC3, DashPattern[3]);
		case 3:  OUTREG(MGAREG_SRC2, DashPattern[2]);
		case 2:	 OUTREG(MGAREG_SRC1, DashPattern[1]);
		default: OUTREG(MGAREG_SRC0, DashPattern[0]);
		}
    }
}


void
MGANAME(SubsequentDashedTwoPointLine)(
    ScrnInfoPtr pScrn, 
    int x1, int y1, int x2, int y2, 
    int flags, int phase
){
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(4);
    if((pMga->AccelFlags & NICE_DASH_PATTERN) && (y1 == y2)) {
    	OUTREG(MGAREG_DWGCTL, pMga->NiceDashCMD);
	if(x2 < x1) {
	   if(flags & OMIT_LAST) x2++;
   	   OUTREG(MGAREG_SHIFT, ((-y1 & 0x07) << 4) | 
				((7 - phase - x1) & 0x07)); 
   	   OUTREG(MGAREG_FXBNDRY, ((x1 + 1) << 16) | (x2 & 0xffff));
    	} else {
 	   if(!flags) x2++;
   	   OUTREG(MGAREG_SHIFT, (((1 - y1) & 0x07) << 4) | 
				((phase - x1) & 0x07)); 
     	   OUTREG(MGAREG_FXBNDRY, (x2 << 16) | (x1 & 0xffff));
	}	
    	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y1 << 16) | 1);
    } else {
	OUTREG(MGAREG_SHIFT, (pMga->StyleLen << 16 ) | 
				(pMga->StyleLen - phase)); 
	OUTREG(MGAREG_DWGCTL, pMga->DashCMD | ((flags & OMIT_LAST) ? 
			MGADWG_AUTOLINE_OPEN : MGADWG_AUTOLINE_CLOSE));
	OUTREG(MGAREG_XYSTRT, (y1 << 16) | (x1 & 0xFFFF));
	OUTREG(MGAREG_XYEND + MGAREG_EXEC, (y2 << 16) | (x2 & 0xFFFF));
    }
}


#if PSZ != 24

	/******************************************\
	|  Planar Screen to Screen Color Expansion |
	\******************************************/

static void 
MGANAME(SetupForPlanarScreenToScreenColorExpandFill)(
   ScrnInfoPtr pScrn,
   int fg, int bg, 
   int rop,
   unsigned int planemask
){
    MGAPtr pMga = MGAPTR(pScrn);
    CARD32 mgaCMD = pMga->AtypeNoBLK[rop] | MGADWG_BITBLT | 
				MGADWG_SGNZERO | MGADWG_BPLAN;
        
    if(bg == -1) {
	mgaCMD |= MGADWG_TRANSC;
	WAITFIFO(4);
    } else {
	WAITFIFO(5);
    	SET_BACKGROUND(bg);
    }

    SET_FOREGROUND(fg);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_AR5, pScrn->displayWidth);
    OUTREG(MGAREG_DWGCTL, mgaCMD);
}

static void 
MGANAME(SubsequentPlanarScreenToScreenColorExpandFill)(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   int srcx, int srcy, 
   int skipleft
){
    MGAPtr pMga = MGAPTR(pScrn);
    int start, end;

    w--;
    start = XYADDRESS(srcx, srcy) + skipleft;
    end = start + w;

    WAITFIFO(4);
    OUTREG(MGAREG_AR3, start);
    OUTREG(MGAREG_AR0, end);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | (x & 0xffff));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

#endif

	/***********************************\
	|  Screen to Screen Color Expansion |
	\***********************************/

static void 
MGANAME(SetupForScreenToScreenColorExpandFill)(
   ScrnInfoPtr pScrn,
   int fg, int bg, 
   int rop,
   unsigned int planemask
){
    MGAPtr pMga = MGAPTR(pScrn);
    CARD32 mgaCMD = MGADWG_BITBLT | MGADWG_SGNZERO | MGADWG_SHIFTZERO;
        
    if(bg == -1) {
#if PSZ == 24
    	if(!RGBEQUAL(fg))
            mgaCMD |= MGADWG_TRANSC | pMga->AtypeNoBLK[rop];
	else
#endif
            mgaCMD |= MGADWG_TRANSC | pMga->Atype[rop];

	WAITFIFO(4);
    } else {
#if PSZ == 24
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && 
		RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION)) 
#endif
        	mgaCMD |= pMga->Atype[rop];
	else
        	mgaCMD |= pMga->AtypeNoBLK[rop];
	WAITFIFO(5);
    	SET_BACKGROUND(bg);
    }

    SET_FOREGROUND(fg);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_AR5, pScrn->displayWidth * PSZ);
    OUTREG(MGAREG_DWGCTL, mgaCMD);

}

static void 
MGANAME(SubsequentScreenToScreenColorExpandFill)(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   int srcx, int srcy, 
   int skipleft
){
    MGAPtr pMga = MGAPTR(pScrn);
    int pitch = pScrn->displayWidth * PSZ;
    int start, end, next, num;
    Bool resetDstOrg = FALSE;

    if (pMga->AccelFlags & LARGE_ADDRESSES) {
        int DstOrg = ((y & ~1023) * pScrn->displayWidth * PSZ) >> 9;
        int SrcOrg = ((srcy & ~1023) * pScrn->displayWidth * PSZ) >> 9;

	y &= 1023;
	srcy &= 1023;

	WAITFIFO(2);
	if(DstOrg) {
            OUTREG(MGAREG_DSTORG, DstOrg << 6);
	    resetDstOrg = TRUE;
	}
        if(SrcOrg != pMga->SrcOrg) {
            pMga->SrcOrg = SrcOrg;
            OUTREG(MGAREG_SRCORG, SrcOrg << 6);
        }
    }

    w--;
    start = (XYADDRESS(srcx, srcy) * PSZ) + skipleft;
    end = start + w + (pitch * (h - 1));

    /* src cannot split a 2 Meg boundary from SrcOrg */
    if(!((start ^ end) & 0xff000000)) {
	WAITFIFO(4);
	OUTREG(MGAREG_AR3, start);
	OUTREG(MGAREG_AR0, start + w);
	OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | (x & 0xffff));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    } else {
	while(h) {
	    next = (start + 0x00ffffff) & 0xff000000;
	    if(next <= (start + w)) {
		num = next - start - 1;

		WAITFIFO(7);
		OUTREG(MGAREG_AR3, start);
		OUTREG(MGAREG_AR0, start + num);
		OUTREG(MGAREG_FXBNDRY, ((x + num) << 16) | (x & 0xffff));
		OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | 1);
		
		OUTREG(MGAREG_AR3, next);
		OUTREG(MGAREG_AR0, start + w );
		OUTREG(MGAREG_FXBNDRY + MGAREG_EXEC, ((x + w) << 16) | 
                                                     ((x + num + 1) & 0xffff));
		start += pitch;
		h--; y++;
	    } else {
		num = ((next - start - w)/pitch) + 1;
		if(num > h) num = h;

		WAITFIFO(4);
		OUTREG(MGAREG_AR3, start);
		OUTREG(MGAREG_AR0, start + w);
		OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | (x & 0xffff));
		OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | num);

		start += num * pitch;
		h -= num; y += num;		
	    }
	}
    }

    if(resetDstOrg) {
	WAITFIFO(1);
	OUTREG(MGAREG_DSTORG, 0);
    }
}



#if PSZ == 8


static __inline__ CARD32* MoveDWORDS(
   register CARD32* dest,
   register CARD32* src,
   register int dwords
)
{
     while(dwords & ~0x03) {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        src += 4;
        dest += 4;
        dwords -= 4;
     }  

     if(!dwords) return(dest);
     dest[0] = src[0];
     if(dwords == 1) return(dest+1);
     dest[1] = src[1];
     if(dwords == 2) return(dest+2);
     dest[2] = src[2];
     return(dest+3);
}

void
MGAWriteBitmapColorExpand(
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
)
{
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    CARD32* destptr = (CARD32*)infoRec->ColorExpandBase;
    CARD32* maxptr;
    int dwords, maxlines, count;

    (*infoRec->SetupForCPUToScreenColorExpandFill)(
				pScrn, fg, bg, rop, planemask);

    w += skipleft;
    x -= skipleft;
    dwords = (w + 31) >> 5;
    maxlines = pMga->MaxBlitDWORDS / dwords;
    maxptr = destptr + infoRec->ColorExpandRange - dwords;
     
    while(h > maxlines) {
    	(*infoRec->SubsequentCPUToScreenColorExpandFill)
			(pScrn, x, y, w, maxlines, skipleft);
	count = maxlines;
	while(count--) {
	    WAITFIFO(dwords);
	    destptr = MoveDWORDS(destptr, (CARD32*)src, dwords);
	    src += srcwidth;
	    if(destptr > maxptr) 
		destptr = (CARD32*)infoRec->ColorExpandBase;
	}
	h -= maxlines;
	y += maxlines;
    }
    	
    (*infoRec->SubsequentCPUToScreenColorExpandFill)(
				pScrn, x, y, w, h, skipleft);

    while(h--) {
	WAITFIFO(dwords);
	destptr = MoveDWORDS(destptr, (CARD32*)src, dwords);
	src += srcwidth;
	if(destptr > maxptr) 
		destptr = (CARD32*)infoRec->ColorExpandBase;
    }
    DISABLE_CLIP();
    SET_SYNC_FLAG(infoRec);
}


void 
MGAFillColorExpandRects(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   PixmapPtr pPix
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    CARD32 *destptr, *maxptr;
    StippleScanlineProcPtr StippleFunc;
    unsigned char *src = (unsigned char*)pPix->devPrivate.ptr;
    int stipplewidth = pPix->drawable.width;
    int stippleheight = pPix->drawable.height;
    int srcwidth = pPix->devKind;
    int dwords, h, y, maxlines, count, srcx, srcy;
    unsigned char *srcp;

    if(stipplewidth <= 32) {
	if(stipplewidth & (stipplewidth - 1))	
	    StippleFunc = XAAStippleScanlineFuncLSBFirst[1];
	else	
	    StippleFunc = XAAStippleScanlineFuncLSBFirst[0];
    } else
    	StippleFunc = XAAStippleScanlineFuncLSBFirst[2];

    (*infoRec->SetupForCPUToScreenColorExpandFill)(
				pScrn, fg, bg, rop, planemask);

    while(nBox--) {
	dwords = (pBox->x2 - pBox->x1 + 31) >> 5;
	destptr = (CARD32*)infoRec->ColorExpandBase;
	maxptr = destptr + infoRec->ColorExpandRange - dwords;
	maxlines = pMga->MaxBlitDWORDS / dwords;
	y = pBox->y1;
	h = pBox->y2 - y;

	srcy = (pBox->y1 - yorg) % stippleheight;
	if(srcy < 0) srcy += stippleheight;
	srcx = (pBox->x1 - xorg) % stipplewidth;
	if(srcx < 0) srcx += stipplewidth;

	srcp = (srcwidth * srcy) + src;

	while(h > maxlines) {
           (*infoRec->SubsequentCPUToScreenColorExpandFill)(pScrn, 
			pBox->x1, y, pBox->x2 - pBox->x1, maxlines, 0);
	   count = maxlines;
	   while(count--) {
		WAITFIFO(dwords);
		destptr = (*StippleFunc)(
			destptr, (CARD32*)srcp, srcx, stipplewidth, dwords);
		if(destptr > maxptr) 
		    destptr = (CARD32*)infoRec->ColorExpandBase;
		srcy++;
		srcp += srcwidth;
		if (srcy >= stippleheight) {
		   srcy = 0;
		   srcp = src;
		}
	   }
	   h -= maxlines;
	   y += maxlines;
	} 

      	(*infoRec->SubsequentCPUToScreenColorExpandFill)(pScrn, 
			pBox->x1, y , pBox->x2 - pBox->x1, h, 0);

	while(h--) {
	    WAITFIFO(dwords);
	    destptr = (*StippleFunc)(
		destptr, (CARD32*)srcp, srcx, stipplewidth, dwords);
	    if(destptr > maxptr) 
		destptr = (CARD32*)infoRec->ColorExpandBase;
	    srcy++;
	    srcp += srcwidth;
	    if (srcy >= stippleheight) {
		srcy = 0;
		srcp = src;
	    }
	}

	pBox++;
    }
    DISABLE_CLIP();
    SET_SYNC_FLAG(infoRec);
}


void
MGAFillSolidRectsDMA(
    ScrnInfoPtr pScrn,
    int	fg, int rop,
    unsigned int planemask,
    int		nBox, 		/* number of rectangles to fill */
    BoxPtr	pBox  		/* Pointer to first rectangle to fill */
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    CARD32 *base = (CARD32*)pMga->ILOADBase;

    SET_SYNC_FLAG(infoRec);
    (*infoRec->SetupForSolidFill)(pScrn, fg, rop, planemask);

    if(nBox & 1) {
	OUTREG(MGAREG_FXBNDRY, ((pBox->x2) << 16) | (pBox->x1 & 0xffff));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, 
		(pBox->y1 << 16) | (pBox->y2 - pBox->y1));
	nBox--; pBox++;
    }

    if(!nBox) return;

    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_GENERAL);
    while(nBox) {
	base[0] = DMAINDICES(MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC,
                MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC);
	base[1] = ((pBox->x2) << 16) | (pBox->x1 & 0xffff);
	base[2] = (pBox->y1 << 16) | (pBox->y2 - pBox->y1);
	pBox++;
	base[3] = ((pBox->x2) << 16) | (pBox->x1 & 0xffff);
	base[4] = (pBox->y1 << 16) | (pBox->y2 - pBox->y1);
	pBox++;
	base += 5; nBox -= 2;
    }
    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);  
}

void
MGAFillSolidSpansDMA(
   ScrnInfoPtr pScrn,
   int fg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted 
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    CARD32 *base = (CARD32*)pMga->ILOADBase;

    SET_SYNC_FLAG(infoRec);
    
    if(infoRec->ClipBox) {
	OUTREG(MGAREG_CXBNDRY,
	   ((infoRec->ClipBox->x2 - 1) << 16) | infoRec->ClipBox->x1); 
	OUTREG(MGAREG_YTOP, 
	   (infoRec->ClipBox->y1 * pScrn->displayWidth) + pMga->YDstOrg); 
	OUTREG(MGAREG_YBOT, 
	   ((infoRec->ClipBox->y2 - 1) * pScrn->displayWidth) + pMga->YDstOrg); 
    }

    (*infoRec->SetupForSolidFill)(pScrn, fg, rop, planemask);

    if(n & 1) {
	OUTREG(MGAREG_FXBNDRY, ((ppt->x + *pwidth) << 16) | (ppt->x & 0xffff));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (ppt->y << 16) | 1);
	ppt++; pwidth++; n--;
    }

    if(n) {
	if(n > 838860) n = 838860;  /* maximum number we have room for */

	OUTREG(MGAREG_OPMODE, MGAOPM_DMA_GENERAL);
	while(n) {
	    base[0] = DMAINDICES(MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC,
                MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC);
	    base[1] = ((ppt->x + *(pwidth++)) << 16) | (ppt->x & 0xffff);
	    base[2] = (ppt->y << 16) | 1;
	    ppt++;
	    base[3] = ((ppt->x + *(pwidth++)) << 16) | (ppt->x & 0xffff);
	    base[4] = (ppt->y << 16) | 1;
	    ppt++;
	    base += 5; n -= 2;
	}
	OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);  
    }

    if(infoRec->ClipBox) {
	OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);     /* (maxX << 16) | minX */ 
	OUTREG(MGAREG_YTOP, 0x00000000);        /* minPixelPointer */ 
	OUTREG(MGAREG_YBOT, 0x007FFFFF);        /* maxPixelPointer */ 
    }
}


void
MGAFillMono8x8PatternRectsTwoPass(
    ScrnInfoPtr pScrn,
    int	fg, int bg, int rop,
    unsigned int planemask,
    int	nBoxInit,
    BoxPtr pBoxInit,
    int pattern0, int pattern1,
    int xorg, int yorg
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    int	nBox, SecondPassColor;
    BoxPtr pBox;

    if((rop == GXcopy) && (bg != -1)) {
	SecondPassColor = bg;
	bg = -1;
    } else SecondPassColor = -1;

    WAITFIFO(1);
    OUTREG(MGAREG_SHIFT, (((-yorg) & 0x07) << 4) | ((-xorg) & 0x07));

SECOND_PASS:

    nBox = nBoxInit;
    pBox = pBoxInit;

    (*infoRec->SetupForMono8x8PatternFill)(pScrn, pattern0, pattern1,
					fg, bg, rop, planemask);

    while(nBox--) {
	WAITFIFO(2);
	OUTREG(MGAREG_FXBNDRY, ((pBox->x2) << 16) | (pBox->x1 & 0xffff));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, 
			(pBox->y1 << 16) | (pBox->y2 - pBox->y1));
	pBox++;
    }

    if(SecondPassColor != -1) {
	fg = SecondPassColor;
	SecondPassColor = -1;
	pattern0 = ~pattern0;
	pattern1 = ~pattern1;
	goto SECOND_PASS;
    }

    SET_SYNC_FLAG(infoRec);
}

void MGANonTEGlyphRenderer(
   ScrnInfoPtr pScrn,
   int x, int y, int n,
   NonTEGlyphPtr glyphs,
   BoxPtr pbox,
   int fg, int rop,
   unsigned int planemask
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    int x1, x2, y1, y2, i, h, skiptop, dwords, maxlines;
    unsigned char *src;

    (*infoRec->SetupForCPUToScreenColorExpandFill)(
					pScrn, fg, -1, rop, planemask);
    WAITFIFO(1);
    OUTREG(MGAREG_CXBNDRY, ((pbox->x2 - 1) << 16) | pbox->x1);      
    pMga->AccelFlags |= CLIPPER_ON;

    for(i = 0; i < n; i++, glyphs++) {
	if(!glyphs->srcwidth) continue;

	y1 = y - glyphs->yoff;
	y2 = y1 + glyphs->height;
	if(y1 < pbox->y1) {
	    skiptop = pbox->y1 - y1;
	    y1 = pbox->y1;
	} else skiptop = 0;
	if(y2 > pbox->y2) y2 = pbox->y2;

	h = y2 - y1;
	if(h <= 0) continue;

	src = glyphs->bits + (skiptop * glyphs->srcwidth);

	dwords = glyphs->srcwidth >> 2;  /* dwords per line */
	x1 = x + glyphs->start;
	x2 = x1 + (dwords << 5);

	maxlines = min(pMga->MaxBlitDWORDS, infoRec->ColorExpandRange);
	maxlines /= dwords; 
     
	while(h > maxlines) {
	    WAITFIFO(4);
	    OUTREG(MGAREG_AR0, (dwords * maxlines << 5) - 1);
	    OUTREG(MGAREG_AR3, 0);
	    OUTREG(MGAREG_FXBNDRY, ((x2 - 1) << 16) | (x1 & 0xFFFF));
	    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y1 << 16) | h);
	    WAITFIFO(dwords * maxlines);
	    MoveDWORDS((CARD32*)infoRec->ColorExpandBase, 
				(CARD32*)src, dwords * maxlines);
	    src += dwords * maxlines << 2;
	    h -= maxlines;
	    y1 += maxlines;
	}
    	
	dwords *= h; 	/* total dwords */
	WAITFIFO(4);
	OUTREG(MGAREG_AR0, (dwords << 5) - 1);
	OUTREG(MGAREG_AR3, 0);
	OUTREG(MGAREG_FXBNDRY, ((x2 - 1) << 16) | (x1 & 0xFFFF));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y1 << 16) | h);
	WAITFIFO(dwords);
	MoveDWORDS((CARD32*)infoRec->ColorExpandBase, (CARD32*)src, dwords);
    }  

    DISABLE_CLIP();
    SET_SYNC_FLAG(infoRec);
}

void
MGAValidatePolyArc(
   GCPtr 	pGC,
   unsigned long changes,
   DrawablePtr pDraw
){
   ScrnInfoPtr pScrn = xf86Screens[pGC->pScreen->myNum];
   MGAPtr pMga = MGAPTR(pScrn);

   if((pMga->AccelFlags & MGA_NO_PLANEMASK) && (pGC->planemask != ~0))
	return;

   if(!pGC->lineWidth && 
      (pGC->fillStyle == FillSolid) &&
      (pGC->lineStyle == LineSolid) &&
      ((pGC->alu != GXcopy) || (pGC->planemask != ~0) ||
		(pScrn->bitsPerPixel == 24))) {
	pGC->ops->PolyArc = MGAPolyArcThinSolid;
   }
}

static void
MGAPolyPoint (
    DrawablePtr pDraw,
    GCPtr pGC,
    int mode,	
    int npt,
    xPoint *ppt
){
    int numRects = REGION_NUM_RECTS(pGC->pCompositeClip);
    XAAInfoRecPtr infoRec;
    BoxPtr pbox;
    MGAPtr pMga;
    int xorg, yorg;

    if(!numRects) return;
    
    if(numRects != 1) {
	XAAFallbackOps.PolyPoint(pDraw, pGC, mode, npt, ppt);
	return;
    }

    infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    pMga = MGAPTR(infoRec->pScrn);
    xorg = pDraw->x;
    yorg = pDraw->y;

    pbox = REGION_RECTS(pGC->pCompositeClip);

    (*infoRec->SetClippingRectangle)(infoRec->pScrn,
                pbox->x1, pbox->y1, pbox->x2 - 1, pbox->y2 - 1);
    (*infoRec->SetupForSolidFill)(infoRec->pScrn, pGC->fgPixel, pGC->alu,
				   pGC->planemask);

    if(mode == CoordModePrevious) {
	while(npt--) {
	    xorg += ppt->x;
	    yorg += ppt->y;
	    WAITFIFO(2);
	    OUTREG(MGAREG_FXBNDRY, ((xorg + 1) << 16) | (xorg & 0xffff));
	    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (yorg << 16) | 1);
	    ppt++;
	} 
    } else {
	int x;
	while(npt--) {
	    x = ppt->x + xorg;
	    WAITFIFO(2);
	    OUTREG(MGAREG_FXBNDRY, ((x + 1) << 16) | (x & 0xffff));
	    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, ((ppt->y + yorg) << 16) | 1);
	    ppt++;
	}
    }
    
    (*infoRec->DisableClipping)(infoRec->pScrn);
  
    SET_SYNC_FLAG(infoRec);
}


void
MGAValidatePolyPoint(
   GCPtr 	pGC,
   unsigned long changes,
   DrawablePtr pDraw
){
   ScrnInfoPtr pScrn = xf86Screens[pGC->pScreen->myNum];
   MGAPtr pMga = MGAPTR(pScrn);

   pGC->ops->PolyPoint = XAAFallbackOps.PolyPoint;

   if((pMga->AccelFlags & MGA_NO_PLANEMASK) && (pGC->planemask != ~0))
	return;

   if((pGC->alu != GXcopy) || (pGC->planemask != ~0) || 
					(pScrn->bitsPerPixel == 24))
	pGC->ops->PolyPoint = MGAPolyPoint;
}


void 
MGAFillCacheBltRects(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   XAACacheInfoPtr pCache
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int x, y, phaseY, phaseX, skipleft, height, width, w, blit_w, blit_h, start;

    (*infoRec->SetupForScreenToScreenCopy)(pScrn, 1, 1, rop, planemask,
		pCache->trans_color);

    while(nBox--) {
	y = pBox->y1;
	phaseY = (y - yorg) % pCache->orig_h;
	if(phaseY < 0) phaseY += pCache->orig_h;
	phaseX = (pBox->x1 - xorg) % pCache->orig_w;
	if(phaseX < 0) phaseX += pCache->orig_w;
	height = pBox->y2 - y;
	width = pBox->x2 - pBox->x1;
	start = phaseY ? (pCache->orig_h - phaseY) : 0;

	/* This is optimized for WRAM */
	
	if ((rop == GXcopy) && (height >= (pCache->orig_h + start))) {		
	    w = width; skipleft = phaseX; x = pBox->x1;
	    blit_h = pCache->orig_h;
	
	    while(1) {
		blit_w = pCache->w - skipleft;
		if(blit_w > w) blit_w = w;
		(*infoRec->SubsequentScreenToScreenCopy)(pScrn,
			pCache->x + skipleft, pCache->y,
			x, y + start, blit_w, blit_h);
		w -= blit_w;
		if(!w) break;
		x += blit_w;
		skipleft = (skipleft + blit_w) % pCache->orig_w;
	    }
	    height -= blit_h;

	    if(start) {
		(*infoRec->SubsequentScreenToScreenCopy)(pScrn,
			pBox->x1, y + blit_h, pBox->x1, y, width, start);
		height -= start;
		y += start;
	    }
	    start = blit_h;
	
	    while(height) {
		if(blit_h > height) blit_h = height;
		(*infoRec->SubsequentScreenToScreenCopy)(pScrn,
			pBox->x1, y,
			pBox->x1, y + start, width, blit_h);
		height -= blit_h;
		start += blit_h;
		blit_h <<= 1;
	    }
	} else {
	    while(1) {
		w = width; skipleft = phaseX; x = pBox->x1;
		blit_h = pCache->h - phaseY;
		if(blit_h > height) blit_h = height;
	
		while(1) {
		    blit_w = pCache->w - skipleft;
		    if(blit_w > w) blit_w = w;
		    (*infoRec->SubsequentScreenToScreenCopy)(pScrn,
			pCache->x + skipleft, pCache->y + phaseY,
			x, y, blit_w, blit_h);
		    w -= blit_w;
		    if(!w) break;
		    x += blit_w;
		    skipleft = (skipleft + blit_w) % pCache->orig_w;
		}
		height -= blit_h;
		if(!height) break;
		y += blit_h;
		phaseY = (phaseY + blit_h) % pCache->orig_h;
	    }
	}
	pBox++;
    }
    
    SET_SYNC_FLAG(infoRec);
}

void 
MGATEGlyphRenderer(
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    GlyphScanlineFuncPtr GlyphFunc = 
			XAAGlyphScanlineFuncLSBFirst[glyphWidth - 1];
    MGAPtr pMga = MGAPTR(pScrn);
    CARD32* base;
    int dwords;

    (*infoRec->SetupForCPUToScreenColorExpandFill)(
				pScrn, fg, bg, rop, planemask);

    w += skipleft;
    x -= skipleft;
    dwords = (w + 31) >> 5;

    (*infoRec->SubsequentCPUToScreenColorExpandFill)(
				pScrn, x, y, w, h, skipleft);

    base = (CARD32*)infoRec->ColorExpandBase;

    if((dwords * h) <= infoRec->ColorExpandRange)
	while(h--) {
	    WAITFIFO(dwords);
	    base = (*GlyphFunc)(base, glyphs, startline++, w, glyphWidth);
	}
    else
	while(h--) {
	    WAITFIFO(dwords);
	    (*GlyphFunc)(base, glyphs, startline++, w, glyphWidth);
	}

    DISABLE_CLIP();
    SET_SYNC_FLAG(infoRec);
}

#endif

#ifdef XF86DRI
void
MGANAME(DRIInitBuffers)(WindowPtr pWin, RegionPtr prgn, CARD32 index)
{
  ScreenPtr pScreen = pWin->drawable.pScreen;
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  MGAPtr pMGA = MGAPTR(pScrn);
  BoxPtr pbox;
  int nbox;

  /* It looks nicer if these start out black */
  pbox = REGION_RECTS(prgn);
  nbox = REGION_NUM_RECTS(prgn);

  MGANAME(SetupForSolidFill)(pScrn, 0, GXcopy, -1);
  while (nbox--) {
    MGASelectBuffer(pMGA, MGA_BACK);
    MGANAME(SubsequentSolidFillRect)(pScrn, pbox->x1, pbox->y1,
				     pbox->x2-pbox->x1, pbox->y2-pbox->y1);
    MGASelectBuffer(pMGA, MGA_DEPTH);
    MGANAME(SubsequentSolidFillRect)(pScrn, pbox->x1, pbox->y1,
				     pbox->x2-pbox->x1, pbox->y2-pbox->y1);
    pbox++;
  }
  MGASelectBuffer(pMGA, MGA_FRONT);

  pMGA->AccelInfoRec->NeedToSync = TRUE;
}

/*
  This routine is a modified form of XAADoBitBlt with the calls to
  ScreenToScreenBitBlt built in. My routine has the prgnSrc as source
  instead of destination. My origin is upside down so the ydir cases
  are reversed.
*/
	   
void
MGANAME(DRIMoveBuffers)(WindowPtr pParent, DDXPointRec ptOldOrg, 
		   RegionPtr prgnSrc, CARD32 index)
{
#if 0
  ScreenPtr pScreen = pParent->drawable.pScreen;
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  MGAPtr pMGA = MGAPTR(pScrn);
  int nbox;
  BoxPtr pbox, pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
  DDXPointPtr pptTmp, pptNew1, pptNew2;
  int xdir, ydir;
  int dx, dy, w, h;
  DDXPointPtr pptSrc;

  pbox = REGION_RECTS(prgnSrc);
  nbox = REGION_NUM_RECTS(prgnSrc);
  pboxNew1 = 0;
  pptNew1 = 0;
  pboxNew2 = 0;
  pboxNew2 = 0;
  pptSrc = &ptOldOrg;

  dx = pParent->drawable.x - ptOldOrg.x;
  dy = pParent->drawable.y - ptOldOrg.y;

  /* If the copy will overlap in Y, reverse the order */
  if (dy>0) {
    ydir = -1;

    if (nbox>1) {
      /* Keep ordering in each band, reverse order of bands */
      pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec)*nbox);
      if (!pboxNew1) return;
      pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec)*nbox);
      if (!pptNew1) {
	DEALLOCATE_LOCAL(pboxNew1);
	return;
      }
      pboxBase = pboxNext = pbox+nbox-1;
      while (pboxBase >= pbox) {
	while ((pboxNext >= pbox) && (pboxBase->y1 == pboxNext->y1))
	  pboxNext--;
	pboxTmp = pboxNext+1;
	pptTmp = pptSrc + (pboxTmp - pbox);
	while (pboxTmp <= pboxBase) {
	  *pboxNew1++ = *pboxTmp++;
	  *pptNew1++ = *pptTmp++;
	}
	pboxBase = pboxNext;
      }
      pboxNew1 -= nbox;
      pbox = pboxNew1;
      pptNew1 -= nbox;
      pptSrc = pptNew1;
    }
  } else {
    /* No changes required */
    ydir = 1;
  }

  /* If the regions will overlap in X, reverse the order */
  if (dx>0) {
    xdir = -1;

    if (nbox > 1) {
      /*reverse orderof rects in each band */
      pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec)*nbox);
      pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec)*nbox);
      if (!pboxNew2 || !pptNew2) {
	if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
	if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
	if (pboxNew1) {
	  DEALLOCATE_LOCAL(pptNew1);
	  DEALLOCATE_LOCAL(pboxNew1);
	}
	return;
      }
      pboxBase = pboxNext = pbox;
      while (pboxBase < pbox+nbox) {
	while ((pboxNext < pbox+nbox) && (pboxNext->y1 == pboxBase->y1))
	  pboxNext++;
	pboxTmp = pboxNext;
	pptTmp = pptSrc + (pboxTmp - pbox);
	while (pboxTmp != pboxBase) {
	  *pboxNew2++ = *--pboxTmp;
	  *pptNew2++ = *--pptTmp;
	}
	pboxBase = pboxNext;
      }
      pboxNew2 -= nbox;
      pbox = pboxNew2;
      pptNew2 -= nbox;
      pptSrc = pptNew2;
    }
  } else {
    /* No changes are needed */
    xdir = 1;
  }
  
  MGANAME(SetupForScreenToScreenCopy)(pScrn, xdir, ydir, GXcopy, -1, -1);
  while (nbox--) {
    w=pbox->x2-pbox->x1+1;
    h=pbox->y2-pbox->y1+1;
    MGASelectBuffer(pMGA, MGA_BACK);
    MGANAME(SubsequentScreenToScreenCopy)(pScrn, pbox->x1, pbox->y1, 
				     pbox->x1+dx, pbox->y1+dy, w, h);
    MGASelectBuffer(pMGA, MGA_DEPTH);
    MGANAME(SubsequentScreenToScreenCopy)(pScrn, pbox->x1, pbox->y1, 
				     pbox->x1+dx, pbox->y1+dy, w, h);
    pbox++;
  }
  MGASelectBuffer(pMGA, MGA_FRONT);

  if (pboxNew2) {
    DEALLOCATE_LOCAL(pptNew2);
    DEALLOCATE_LOCAL(pboxNew2);
  }
  if (pboxNew1) {
    DEALLOCATE_LOCAL(pptNew1);
    DEALLOCATE_LOCAL(pboxNew1);
  }

  pMGA->AccelInfoRec->NeedToSync = TRUE;
#endif
}
	   
#endif /* XF86DRI */
