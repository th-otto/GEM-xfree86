/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis300_accel.c,v 1.2 2000/02/12 23:07:58 dawes Exp $ */

/*
 *
 *	Acceleration for SiS300 SiS630 SiS540.
 *	It is done in a separate file because the register formats are 
 *      very different from the previous chips.
 *
 *
 *	
 *	Xavier Ducoin <x.ducoin@lectra.com>
 */
#if 0
#define DEBUG
#endif

#include <xf86.h>
#include <xf86_OSproc.h>
#include <xf86_ansic.h>

#include <xf86PciInfo.h>
#include <xf86Pci.h>

#include <miline.h>

#include <xaa.h>
#include <xaalocal.h>
#include <xf86fbman.h>

#include "sis.h"
#include "sis300_accel.h"


#ifdef	DEBUG
static void MMIODump(ScrnInfoPtr pScrn);
#endif

static void SiSSync(ScrnInfoPtr pScrn);
static void SiSSetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop,
				unsigned int planemask, int trans_color);
static void SiSSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
				int x1, int y1, int x2, int y2,
				int width, int height);
static void SiSSetupForSolidFill(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void SiSSubsequentSolidFillRect(ScrnInfoPtr pScrn,
				int x, int y, int w, int h);
static void SiSSetupForSolidLine(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void SiSSubsequentSolidTwoPointLine(ScrnInfoPtr pScrn, int x1,
				int y1, int x2, int y2, int flags);
static void SiSSubsequentSolidHorzVertLine(ScrnInfoPtr pScrn,
				int x, int y, int len, int dir);
static void SiSSetupForDashedLine(ScrnInfoPtr pScrn,
				int fg, int bg, int rop, unsigned int planemask,
				int length, unsigned char *pattern);
static void SiSSubsequentDashedTwoPointLine(ScrnInfoPtr pScrn,
				int x1, int y1, int x2, int y2,
				int flags, int phase);
static void SiSSetupForMonoPatternFill(ScrnInfoPtr pScrn,
				int patx, int paty, int fg, int bg,
				int rop, unsigned int planemask);
static void SiSSubsequentMonoPatternFill(ScrnInfoPtr pScrn,
				int patx, int paty,
				int x, int y, int w, int h);
static void SiSSetupForColorPatternFill(ScrnInfoPtr pScrn,
				int patx, int paty, int rop,
				unsigned int planemask,
				int trans_color);
static void SiSSubsequentColorPatternFill(ScrnInfoPtr pScrn,
				int patx, int paty,
				int x, int y, int w, int h);
static void SiSSetupForCPUToScreenColorExpand(ScrnInfoPtr pScrn,
				int fg, int bg,
				int rop, unsigned int planemask);
static void SiSSubsequentCPUToScreenColorExpand(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
static void SiSSetupForScreenToScreenColorExpand(ScrnInfoPtr pScrn,
				int fg, int bg,
				int rop, unsigned int planemask);
static void SiSSubsequentScreenToScreenColorExpand(ScrnInfoPtr pScrn,
				int x, int y, int w, int h,
				int srcx, int srcy, int skipleft);


void
SiSInitializeAccelerator(ScrnInfoPtr pScrn)
{
	SISPtr	pSiS = SISPTR(pScrn);

	SiSSetupDSTColorDepth(SISPTR(pScrn)->DstColor);
	pSiS->DoColorExpand = FALSE;
}

Bool
SiS300AccelInit(ScreenPtr pScreen)
{
	XAAInfoRecPtr	infoPtr;
	ScrnInfoPtr	pScrn = xf86Screens[pScreen->myNum];
	SISPtr		pSiS = SISPTR(pScrn);
	int		reservedFbSize, UsableFbSize;
	BoxRec		Avail;

	pSiS->AccelInfoPtr = infoPtr = XAACreateInfoRec();
	if (!infoPtr)  return FALSE;

	SiSInitializeAccelerator(pScrn);

	infoPtr->Flags = LINEAR_FRAMEBUFFER |
			OFFSCREEN_PIXMAPS |
			PIXMAP_CACHE |
			NO_PLANEMASK;

	/* sync */
	infoPtr->Sync = SiSSync;

	/* BitBlt */
	infoPtr->SetupForScreenToScreenCopy = SiSSetupForScreenToScreenCopy;
	infoPtr->SubsequentScreenToScreenCopy = SiSSubsequentScreenToScreenCopy;
	infoPtr->ScreenToScreenCopyFlags = NO_PLANEMASK | NO_TRANSPARENCY;


	/* solid fills */
	infoPtr->SetupForSolidFill = SiSSetupForSolidFill;
	infoPtr->SubsequentSolidFillRect = SiSSubsequentSolidFillRect;
	infoPtr->SolidFillFlags = NO_PLANEMASK;


	/* solid line */
	infoPtr->SetupForSolidLine = SiSSetupForSolidLine;
	infoPtr->SubsequentSolidTwoPointLine = SiSSubsequentSolidTwoPointLine;
	infoPtr->SubsequentSolidHorVertLine = SiSSubsequentSolidHorzVertLine;
	infoPtr->SolidFillFlags = NO_PLANEMASK;


	/* dashed line */
	infoPtr->SetupForDashedLine = SiSSetupForDashedLine;
	infoPtr->SubsequentDashedTwoPointLine = SiSSubsequentDashedTwoPointLine;
	infoPtr->DashPatternMaxLength = 64;
	infoPtr->DashedLineFlags = NO_PLANEMASK | 
			LINE_PATTERN_MSBFIRST_LSBJUSTIFIED;


	/* 8x8 mono pattern fill */
	infoPtr->SetupForMono8x8PatternFill = SiSSetupForMonoPatternFill;
	infoPtr->SubsequentMono8x8PatternFillRect =
				SiSSubsequentMonoPatternFill;
	infoPtr->Mono8x8PatternFillFlags = NO_PLANEMASK |
				HARDWARE_PATTERN_SCREEN_ORIGIN |
				HARDWARE_PATTERN_PROGRAMMED_BITS |
				NO_TRANSPARENCY |
				BIT_ORDER_IN_BYTE_MSBFIRST ;


	/* 8x8 color pattern fill 
	infoPtr->SetupForColor8x8PatternFill =
				SiSSetupForColorPatternFill;
	infoPtr->SubsequentColor8x8PatternFillRect =
				SiSSubsequentColorPatternFill;
	infoPtr->Color8x8PatternFillFlags = NO_PLANEMASK |
				HARDWARE_PATTERN_SCREEN_ORIGIN |
				HARDWARE_PATTERN_PROGRAMMED_BITS ;
*/


	/* CPU To Screen Color Expand */
	infoPtr->SetupForCPUToScreenColorExpandFill =
				SiSSetupForCPUToScreenColorExpand;
	infoPtr->SubsequentCPUToScreenColorExpandFill =
				SiSSubsequentCPUToScreenColorExpand;
	infoPtr->SetupForScreenToScreenColorExpandFill =
				SiSSetupForScreenToScreenColorExpand;
	infoPtr->ColorExpandRange = PATREGSIZE;
	infoPtr->ColorExpandBase = pSiS->IOBase+PBR(0);
	infoPtr->CPUToScreenColorExpandFillFlags = NO_PLANEMASK |
				BIT_ORDER_IN_BYTE_MSBFIRST |
				NO_TRANSPARENCY |
				SYNC_AFTER_COLOR_EXPAND |
				HARDWARE_PATTERN_SCREEN_ORIGIN |
				HARDWARE_PATTERN_PROGRAMMED_BITS ;



	/* Screen To Screen Color Expand 
	infoPtr->SetupForScreenToScreenColorExpandFill =
				SiSSetupForScreenToScreenColorExpand;
	infoPtr->SubsequentScreenToScreenColorExpandFill =
				SiSSubsequentScreenToScreenColorExpand;
	infoPtr->CPUToScreenColorExpandFillFlags = NO_PLANEMASK |
				BIT_ORDER_IN_BYTE_MSBFIRST |
				SYNC_AFTER_COLOR_EXPAND |
				NO_TRANSPARENCY;
*/


	/* init Frame Buffer Manager */
	reservedFbSize = 0;
	if (pSiS->TurboQueue) reservedFbSize += 1024*512;
	if (pSiS->HWCursor)  reservedFbSize += 4096;
	UsableFbSize = pSiS->FbMapSize - reservedFbSize;
	Avail.x1 = 0;
	Avail.y1 = 0;
	Avail.x2 = pScrn->displayWidth;
	Avail.y2 = UsableFbSize / (pScrn->displayWidth * pScrn->bitsPerPixel/8);
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
				"Frame Buffer From (%d,%d) To (%d,%d)\n",
				Avail.x1, Avail.y1, Avail.x2, Avail.y2);
	xf86InitFBManager(pScreen, &Avail);


	return(XAAInit(pScreen, infoPtr));
}


static void 
SiSSync(ScrnInfoPtr pScrn)
{
	SISPtr pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("SiSSync()\n"));

	if (pSiS->DoColorExpand)
		SiSDoCMD
	pSiS->DoColorExpand = FALSE;
	SiSIdle
}

static int sisALUConv[] =
{
    0x00,	/* dest = 0; 		0,	GXclear, 	0 */
    0x88,	/* dest &= src; 	DSa,	GXand,		0x1 */
    0x44,	/* dest = src & ~dest; 	SDna,	GXandReverse, 	0x2 */
    0xCC,	/* dest = src; 		S,	GXcopy, 	0x3 */
    0x22,	/* dest &= ~src; 	DSna,	GXandInverted, 	0x4 */
    0xAA,	/* dest = dest; 	D,	GXnoop, 	0x5 */
    0x66,	/* dest = ^src; 	DSx,	GXxor, 		0x6 */
    0xEE,	/* dest |= src; 	DSo,	GXor, 		0x7 */
    0x11,	/* dest = ~src & ~dest;	DSon,	GXnor, 		0x8 */
    0x99,	/* dest ^= ~src ;	DSxn,	GXequiv, 	0x9 */
    0x55,	/* dest = ~dest; 	Dn,	GXInvert, 	0xA */
    0xDD,	/* dest = src|~dest ;	SDno,	GXorReverse, 	0xB */
    0x33,	/* dest = ~src; 	Sn,	GXcopyInverted, 0xC */
    0xBB,	/* dest |= ~src; 	DSno,	GXorInverted, 	0xD */
    0x77,	/* dest = ~src|~dest;	DSan,	GXnand, 	0xE */
    0xFF,	/* dest = 0xFF; 	1,	GXset, 		0xF */
};
/* same ROP but with Pattern as Source */
static int sisPatALUConv[] =
{
    0x00,	/* dest = 0; 		0,	GXclear, 	0 */
    0xA0,	/* dest &= src; 	DPa,	GXand,		0x1 */
    0x50,	/* dest = src & ~dest; 	PDna,	GXandReverse, 	0x2 */
    0xF0,	/* dest = src; 		P,	GXcopy, 	0x3 */
    0x0A,	/* dest &= ~src; 	DPna,	GXandInverted, 	0x4 */
    0xAA,	/* dest = dest; 	D,	GXnoop, 	0x5 */
    0x5A,	/* dest = ^src; 	DPx,	GXxor, 		0x6 */
    0xFA,	/* dest |= src; 	DPo,	GXor, 		0x7 */
    0x05,	/* dest = ~src & ~dest;	DPon,	GXnor, 		0x8 */
    0xA5,	/* dest ^= ~src ;	DPxn,	GXequiv, 	0x9 */
    0x55,	/* dest = ~dest; 	Dn,	GXInvert, 	0xA */
    0xF5,	/* dest = src|~dest ;	PDno,	GXorReverse, 	0xB */
    0x0F,	/* dest = ~src; 	Pn,	GXcopyInverted, 0xC */
    0xAF,	/* dest |= ~src; 	DPno,	GXorInverted, 	0xD */
    0x5F,	/* dest = ~src|~dest;	DPan,	GXnand, 	0xE */
    0xFF,	/* dest = 0xFF; 	1,	GXset, 		0xF */
};

static void SiSSetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop,
				unsigned int planemask, int trans_color)
{
	SISPtr	pSiS = SISPTR(pScrn);
	XAAInfoRecPtr	pXAA = XAAPTR(pScrn);

	PDEBUG(ErrorF("Setup ScreenCopy(%d, %d, 0x%x, 0x%x, 0x%x)\n",
			xdir, ydir, rop, planemask, trans_color));
/*
	ErrorF("XAAInfoPtr->UsingPixmapCache = %s\n"
		"XAAInfoPtr->CanDoMono8x8 = %s\n"
		"XAAInfoPtr->CanDoColor8x8 = %s\n"
		"XAAInfoPtr->CachePixelGranularity = %d\n"
		"XAAInfoPtr->MaxCacheableTileWidth = %d\n"
		"XAAInfoPtr->MaxCacheableTileHeight = %d\n"
		"XAAInfoPtr->MaxCacheableStippleWidth = %d\n"
		"XAAInfoPtr->MaxCacheableStippleHeight = %d\n"
		"XAAInfoPtr->MonoPatternPitch = %d\n"
		"XAAInfoPtr->CacheWidthMono8x8Pattern = %d\n"
		"XAAInfoPtr->CacheHeightMono8x8Pattern = %d\n"
		"XAAInfoPtr->ColorPatternPitch = %d\n"
		"XAAInfoPtr->CacheWidthColor8x8Pattern = %d\n"
		"XAAInfoPtr->CacheHeightColor8x8Pattern = %d\n"
		"XAAInfoPtr->CacheColorExpandDensity = %d\n"
		"XAAInfoPtr->maxOffPixWidth = %d\n"
		"XAAInfoPtr->maxOffPixHeight= %d\n"
		"XAAInfoPtr->NeedToSync = %s\n"
		"\n",
			pXAA->UsingPixmapCache ? "True" : "False",
			pXAA->CanDoMono8x8 ? "True" : "False",
			pXAA->CanDoColor8x8 ? "True" : "False",
			pXAA->CachePixelGranularity,
			pXAA->MaxCacheableTileWidth,
			pXAA->MaxCacheableTileHeight,
			pXAA->MaxCacheableStippleWidth,
			pXAA->MaxCacheableStippleHeight,
			pXAA->MonoPatternPitch,
			pXAA->CacheWidthMono8x8Pattern,
			pXAA->CacheHeightMono8x8Pattern,
			pXAA->ColorPatternPitch,
			pXAA->CacheWidthColor8x8Pattern,
			pXAA->CacheHeightColor8x8Pattern,
			pXAA->CacheColorExpandDensity,
			pXAA->maxOffPixWidth,
			pXAA->maxOffPixHeight,
			pXAA->NeedToSync ? "True" : "False");
*/
			
	SiSSetupSRCBase(0)
	SiSSetupDSTColorDepth(SISPTR(pScrn)->DstColor);
	SiSSetupSRCPitch(pSiS->scrnOffset)
	SiSSetupDSTBase(0)
/*	SiSSetupDSTRect(pSiS->scrnOffset, pScrn->virtualY)*/
	SiSSetupDSTRect(pSiS->scrnOffset, -1)
	SiSSetupROP(sisALUConv[rop])
	if (xdir > 0)	SiSSetupCMDFlag(X_INC)
	if (ydir > 0)	SiSSetupCMDFlag(Y_INC)

}

static void SiSSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
				int src_x, int src_y, int dst_x, int dst_y,
				int width, int height)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Subsequent ScreenCopy(%d,%d, %d,%d, %d,%d)\n",
			src_x, src_y, dst_x, dst_y, width, height));

	if (!(pSiS->CommandReg & X_INC))  {
		src_x += width-1;
		dst_x += width-1;
	}
	if (!(pSiS->CommandReg & Y_INC))  {
		src_y += height-1;
		dst_y += height-1;
	}
	SiSSetupRect(width, height)
	SiSSetupSRCXY(src_x,src_y)
	SiSSetupDSTXY(dst_x,dst_y)
	SiSDoCMD
}

static void
SiSSetupForSolidFill(ScrnInfoPtr pScrn,
			int color, int rop, unsigned int planemask)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Setup SolidFill(0x%x, 0x%x, 0x%x)\n",
			color, rop, planemask));

	SiSSetupPATFG(color)
	SiSSetupDSTBase(0)
/*	SiSSetupDSTRect(pSiS->scrnOffset, pScrn->virtualY)*/
	SiSSetupDSTRect(pSiS->scrnOffset, -1)
	SiSSetupDSTColorDepth(SISPTR(pScrn)->DstColor);
	SiSSetupROP(sisPatALUConv[rop])
	SiSSetupCMDFlag(X_INC | Y_INC | PATFG | BITBLT)
}

static void
SiSSubsequentSolidFillRect(ScrnInfoPtr pScrn,
			int x, int y, int w, int h)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Subsequent SolidFillRect(%d, %d, %d, %d)\n",
			x, y, w, h));

	SiSSetupDSTXY(x,y)
	SiSSetupRect(w,h)
	SiSDoCMD
}

static void
SiSSetupForSolidLine(ScrnInfoPtr pScrn,
			int color, int rop, unsigned int planemask)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Setup SolidLine(0x%x, 0x%x, 0x%x)\n",
			color, rop, planemask));

	SiSSetupLineCount(1)
	SiSSetupPATFG(color)
	SiSSetupDSTBase(0)
/*	SiSSetupDSTRect(pSiS->scrnOffset, pScrn->virtualY)*/
	SiSSetupDSTRect(pSiS->scrnOffset, -1)
	SiSSetupDSTColorDepth(SISPTR(pScrn)->DstColor);
	SiSSetupROP(sisPatALUConv[rop])
	SiSSetupCMDFlag(PATFG | LINE)
}

static void
SiSSubsequentSolidTwoPointLine(ScrnInfoPtr pScrn,
			int x1, int y1, int x2, int y2, int flags)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Subsequent SolidLine(%d, %d, %d, %d, 0x%x)\n",
			x1, y1, x2, y2, flags));

	SiSSetupX0Y0(x1,y1)
	SiSSetupX1Y1(x2,y2)
	if (flags & OMIT_LAST)
		SiSSetupCMDFlag(NO_LAST_PIXEL)
	SiSDoCMD
}

static void
SiSSubsequentSolidHorzVertLine(ScrnInfoPtr pScrn,
				int x, int y, int len, int dir)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Subsequent SolidHorzVertLine(%d, %d, %d, %d)\n",
			x, y, len, dir));

	SiSSetupX0Y0(x,y)
	if (dir==DEGREES_0)
		SiSSetupX1Y1(x+len,y)
	else
		SiSSetupX1Y1(x,y+len)
	SiSDoCMD
}

static void
SiSSetupForDashedLine(ScrnInfoPtr pScrn,
				int fg, int bg, int rop, unsigned int planemask,
				int length, unsigned char *pattern)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Setup DashedLine(0x%x, 0x%x, 0x%x, 0x%x, %d, 0x%x:%x)\n",
		fg, bg, rop, planemask, length, *(pattern+4), *pattern));

	SiSSetupLineCount(1)
	SiSSetupDSTBase(0)
/*	SiSSetupDSTRect(pSiS->scrnOffset, pScrn->virtualY)*/
	SiSSetupDSTRect(pSiS->scrnOffset, -1)
	SiSSetupDSTColorDepth(SISPTR(pScrn)->DstColor);
	SiSSetupStyleLow(*pattern)
	SiSSetupStyleHigh(*(pattern+4))
	SiSSetupROP(sisPatALUConv[rop])
	SiSSetupPATFG(fg)
	if (bg != -1)	SiSSetupPATBG(bg)
}

static void
SiSSubsequentDashedTwoPointLine(ScrnInfoPtr pScrn,
				int x1, int y1, int x2, int y2,
				int flags, int phase)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Subsequent DashedLine(%d,%d, %d,%d, 0x%x,0x%x)\n",
		x1, y1, x2, y2, flags, phase));

	SiSSetupX0Y0(x1,y1)
	SiSSetupX1Y1(x2,y2)
	if (flags & OMIT_LAST)
		SiSSetupCMDFlag(NO_LAST_PIXEL)
	SiSDoCMD
}
static void
SiSSetupForMonoPatternFill(ScrnInfoPtr pScrn,
				int patx, int paty, int fg, int bg,
				int rop, unsigned int planemask)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Setup MonoPatFill(0x%x,0x%x, 0x%x,0x%x, 0x%x, 0x%x)\n",
			patx, paty, fg, bg, rop, planemask));

	SiSSetupDSTBase(0)
/*	SiSSetupDSTRect(pSiS->scrnOffset, pScrn->virtualY)*/
	SiSSetupDSTRect(pSiS->scrnOffset, -1)
	SiSSetupDSTColorDepth(SISPTR(pScrn)->DstColor);
	SiSSetupMONOPAT(patx,paty)
	SiSSetupPATFG(fg)
	SiSSetupROP(sisPatALUConv[rop])
	SiSSetupCMDFlag(PATMONO | X_INC | Y_INC)
/*	if (bg==-1)
		SiSSetupCMDFlag(TRANSPARENT)
	else*/
		SiSSetupPATBG(bg)
}

static void
SiSSubsequentMonoPatternFill(ScrnInfoPtr pScrn,
				int patx, int paty,
				int x, int y, int w, int h)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Subsequent MonoPatFill(0x%x,0x%x, %d,%d, %d,%d)\n",
				patx, paty, x, y, w, h));
	SiSSetupDSTXY(x,y)
	SiSSetupRect(w,h)
	SiSDoCMD
}

static void
SiSSetupForColorPatternFill(ScrnInfoPtr pScrn,
				int patx, int paty, int rop,
				unsigned int planemask,
				int trans_color)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Setup ColorPatFill(0x%x,0x%x, 0x%x,0x%x, 0x%x)\n",
			patx, paty, rop, planemask, trans_color));

	SiSSetupDSTBase(0)
/*	SiSSetupDSTRect(pSiS->scrnOffset, pScrn->virtualY)*/
	SiSSetupDSTRect(pSiS->scrnOffset, -1)
	SiSSetupDSTColorDepth(SISPTR(pScrn)->DstColor);
	SiSSetupROP(sisPatALUConv[rop])
	SiSSetupCMDFlag(PATPATREG | X_INC | Y_INC)
}

static void
SiSSubsequentColorPatternFill(ScrnInfoPtr pScrn,
				int patx, int paty,
				int x, int y, int w, int h)
{
	SISPtr	pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Subsequent ColorPatFill(0x%x,0x%x, %d,%d, %d,%d)\n",
				patx, paty, x, y, w, h));
	SiSSetupDSTXY(x,y)
	SiSSetupRect(w,h)
	SiSDoCMD
}

static void
SiSSetupForCPUToScreenColorExpand(ScrnInfoPtr pScrn,
				int fg, int bg,
				int rop, unsigned int planemask)
{
	SISPtr	pSiS = SISPTR(pScrn);
	XAAInfoRecPtr	pXAA = XAAPTR(pScrn);

	PDEBUG(ErrorF("Setup CPUToScreen ColorExpand(0x%x,0x%x, 0x%x,0x%x)\n",
			fg, bg, rop, planemask));

	SiSSetupDSTBase(0)
/*	SiSSetupDSTRect(pSiS->scrnOffset, pScrn->virtualY)*/
	SiSSetupDSTRect(pSiS->scrnOffset, -1)
	SiSSetupDSTColorDepth(SISPTR(pScrn)->DstColor);
	SiSSetupSRCXY(0,0)
	SiSSetupSRCFG(fg)
	SiSSetupROP(sisPatALUConv[rop])
	SiSSetupCMDFlag(X_INC | Y_INC | COLOREXP)
	if (bg==-1)
		SiSSetupCMDFlag(TRANSPARENT)
	else
		SiSSetupSRCBG(bg)
}

static void
SiSSubsequentCPUToScreenColorExpand(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft)
{
	SISPtr		pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Subsequent CPUToScreen ColorExpand(%d,%d, %d,%d, %d)\n",
				x, y, w, h, skipleft));

/*	SiSSetupSRCPitch(((w+31)&0xFFE0)/8)*/
	SiSSetupSRCPitch((w+7)/8)
	SiSSetupDSTXY(x,y)
	SiSSetupRect(w,h)
/*	SiSDoCMD*/
	pSiS->DoColorExpand = TRUE;
}

static void
SiSSetupForScreenToScreenColorExpand(ScrnInfoPtr pScrn,
				int fg, int bg,
				int rop, unsigned int planemask)
{
	SISPtr		pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Setup ScreenToScreen ColorExp(0x%x,0x%x, 0x%x)\n",
				fg, bg, rop));

	SiSSetupDSTBase(0)
/*	SiSSetupDSTRect(pSiS->scrnOffset, pScrn->virtualY)*/
	SiSSetupDSTRect(pSiS->scrnOffset, -1)
	SiSSetupDSTColorDepth(SISPTR(pScrn)->DstColor);
	SiSSetupSRCFG(fg)
	SiSSetupSRCBG(bg)
	SiSSetupSRCXY(0,0)
	SiSSetupROP(sisALUConv[rop])
	SiSSetupCMDFlag(X_INC | Y_INC | ENCOLOREXP)
}

static void
SiSSubsequentScreenToScreenColorExpand(ScrnInfoPtr pScrn,
				int x, int y, int w, int h,
				int srcx, int srcy, int skipleft)
{
	SISPtr		pSiS = SISPTR(pScrn);

	PDEBUG(ErrorF("Sub ScreenToScreen ColorExp(%d,%d, %d,%d, %d,%d, %d)\n",
				x, y, w, h, srcx, srcy, skipleft));

	SiSSetupSRCPitch(((w+31)&0xFFE0)/8)
	SiSSetupDSTXY(x,y)
	SiSSetupRect(w,h)
	SiSDoCMD
}





#ifdef	DEBUG
static void
MMIODump(ScrnInfoPtr pScrn)
{
	SISPtr	pSiS = SISPTR(pScrn);
	int	i;

	SiSIdle
	for (i=0x8200; i<0x8250; i+=16)  {
		ErrorF("MMIO %x: %0X %0X %0X %0X\n", i,
				MMIO_IN32(pSiS->IOBase, i   ),
				MMIO_IN32(pSiS->IOBase, i+ 4),
				MMIO_IN32(pSiS->IOBase, i+ 8),
				MMIO_IN32(pSiS->IOBase, i+12));
	}
}
#endif
