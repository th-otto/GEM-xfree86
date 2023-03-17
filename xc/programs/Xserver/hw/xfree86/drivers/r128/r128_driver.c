/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/r128/r128_driver.c,v 1.26 2000/03/06 23:17:44 martin Exp $ */
/**************************************************************************

Copyright 1999 ATI Technologies Inc. and Precision Insight, Inc.,
                                         Cedar Park, Texas. 
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Rickard E. Faith <faith@precisioninsight.com>
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *
 * Credits:
 *
 *   Thanks to Alan Hourihane <alanh@fairlite.demon..co.uk> and SuSE for
 *   providing source code to their 3.3.x Rage 128 driver.  Portions of
 *   this file are based on the initialization code for that driver.
 *
 * References:
 *
 *   RAGE 128 VR/ RAGE 128 GL Register Reference Manual (Technical
 *   Reference Manual P/N RRG-G04100-C Rev. 0.04), ATI Technologies: April
 *   1999.
 *
 *   RAGE 128 Software Development Manual (Technical Reference Manual P/N
 *   SDK-G04000 Rev. 0.01), ATI Technologies: June 1999.
 *
 * This server does not yet support these XFree86 4.0 features:
 *   DDC1 & DDC2
 *   shadowfb
 *   overlay planes
 *   DGA
 *
 */


				/* X and server generic header files */
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86RAC.h"
#include "xf86cmap.h"
#include "xf86fbman.h"
#include "xf86int10.h"
				/* Backing store, software cursor, and
                                   colormap initialization */
#include "mibstore.h"
#include "mipointer.h"
#include "micmap.h"

#undef USE_FB			/* not until overlays and 24->32 code added */
#ifdef USE_FB
#include "fb.h"
#else
				/* CFB support */
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "cfb24_32.h"
#endif
				/* Xv support */
#include "xf86xv.h"
#include "Xv.h"

				/* vgahw module (for VC save/restore only) */
#include "vgaHW.h"

#include "fbdevhw.h"

				/* XAA and Cursor Support */
#include "xaa.h"
#include "xf86Cursor.h"


				/* PCI support */
#include "xf86PciInfo.h"
#include "xf86Pci.h"

				/* DDC support */
#include "xf86DDC.h"

				/* VESA support */
#include "vbe.h"

				/* Driver data structures */
#include "r128.h"
#include "r128_reg.h"

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif


				/* Forward definitions for driver functions */
static OptionInfoPtr R128AvailableOptions(int chipid, int busid);
static Bool R128Probe(DriverPtr drv, int flags);
static void R128Identify(int flags);
static Bool R128PreInit(ScrnInfoPtr pScrn, int flags);
static Bool R128ScreenInit(int scrnIndex, ScreenPtr pScreen,
			   int argc, char **argv);

static int  R128ValidMode(int scrnIndex, DisplayModePtr mode,
			  Bool verbose, int flag);
static void R128AdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool R128EnterVT(int scrnIndex, int flags);
static void R128LeaveVT(int scrnIndex, int flags);
static Bool R128CloseScreen(int scrnIndex, ScreenPtr pScreen);
static void R128FreeScreen(int scrnIndex, int flags);
static Bool R128SaveScreen(ScreenPtr pScreen, int mode);
static void R128Save(ScrnInfoPtr pScrn);
static void R128Restore(ScrnInfoPtr pScrn);
static Bool R128ModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool R128SwitchMode(int ScrnIndex, DisplayModePtr mode, int flags);
static void R128DisplayPowerManagementSet(ScrnInfoPtr pScrn,
					  int PowerManagementMode, int flags);
static Bool R128EnterVTFBDev(int scrnIndex, int flags);
static void R128LeaveVTFBDev(int scrnIndex, int flags);

				/* Define driver                       */
DriverRec R128 = {
    R128_VERSION,
    "ATI Rage 128",
    R128Identify,
    R128Probe,
    R128AvailableOptions,
    NULL
};

				/* Chipsets */
static SymTabRec R128Chipsets[] = {
    { PCI_CHIP_RAGE128RE, "ATI Rage 128 RE (PCI)" },
    { PCI_CHIP_RAGE128RF, "ATI Rage 128 RF (AGP)" },
    { PCI_CHIP_RAGE128RK, "ATI Rage 128 RK (PCI)" },
    { PCI_CHIP_RAGE128RL, "ATI Rage 128 RL (AGP)" },
    { PCI_CHIP_RAGE128PF, "ATI Rage 128 Pro PF (AGP)" },
    { -1,                 NULL }
};

static PciChipsets R128PciChipsets[] = {
    { PCI_CHIP_RAGE128RE, PCI_CHIP_RAGE128RE, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128RF, PCI_CHIP_RAGE128RF, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128RK, PCI_CHIP_RAGE128RK, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128RL, PCI_CHIP_RAGE128RL, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128PF, PCI_CHIP_RAGE128PF, RES_SHARED_VGA },
    { -1,                 -1,                 RES_UNDEFINED }
};

typedef enum {
  OPTION_NOACCEL,
  OPTION_SW_CURSOR,
  OPTION_HW_CURSOR,
  OPTION_DAC_6BIT,
  OPTION_DAC_8BIT,
  OPTION_FBDEV
} R128Opts;

static OptionInfoRec R128Options[] = {
  { OPTION_NOACCEL,   "NoAccel",  OPTV_BOOLEAN, {0}, FALSE },
  { OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE },
  { OPTION_HW_CURSOR, "HWcursor", OPTV_BOOLEAN, {0}, TRUE  },
  { OPTION_DAC_6BIT,  "Dac6Bit",  OPTV_BOOLEAN, {0}, FALSE },
  { OPTION_DAC_8BIT,  "Dac8Bit",  OPTV_BOOLEAN, {0}, TRUE  },
  { OPTION_FBDEV,     "UseFBDev", OPTV_BOOLEAN, {0}, FALSE },
  { -1,               NULL,       OPTV_NONE,    {0}, FALSE }
};

R128RAMRec R128RAM[] = {	/* Memory Specifications
				   From RAGE 128 Software Development
				   Manual (Technical Reference Manual P/N
				   SDK-G04000 Rev 0.01), page 3-21.  */
    { 4, 4, 3, 3, 1, 3, 1, 16, 12, "128-bit SDR SGRAM 1:1" },
    { 4, 8, 3, 3, 1, 3, 1, 17, 13, "64-bit SDR SGRAM 1:1" },
    { 4, 4, 1, 2, 1, 2, 1, 16, 12, "64-bit SDR SGRAM 2:1" },
    { 4, 4, 3, 3, 2, 3, 1, 16, 12, "64-bit DDR SGRAM" },
};

static const char *vgahwSymbols[] = {
    "vgaHWGetHWRec",
    "vgaHWFreeHWRec",
    "vgaHWLock",
    "vgaHWUnlock",
    "vgaHWSave",
    "vgaHWRestore",
    NULL
};

static const char *fbdevHWSymbols[] = {
    "fbdevHWInit",
    "fbdevHWUseBuildinMode",

    "fbdevHWGetDepth",
    "fbdevHWGetVidmem",

    /* colormap */
    "fbdevHWLoadPalette",

    /* ScrnInfo hooks */
    "fbdevHWSwitchMode",
    "fbdevHWAdjustFrame",
    "fbdevHWEnterVT",
    "fbdevHWLeaveVT",
    "fbdevHWValidMode",
    "fbdevHWRestore",
    "fbdevHWModeInit",
    "fbdevHWSave",

    "fbdevHWUnmapMMIO",
    "fbdevHWUnmapVidmem",
    "fbdevHWMapMMIO",
    "fbdevHWMapVidmem",

    NULL
};

static const char *vbeSymbols[] = {
    "VBEInit",
    "vbeDoEDID",
    NULL
};

#if 0
				/* Not used until DDC is supported. */
static const char *ddcSymbols[] = {
    "xf86PrintEDID",
    "xf86DoEDID_DDC1",
    "xf86DoEDID_DDC2",
    NULL
};

static const char *i2cSymbols[] = {
    "xf86CreateI2CBusRec",
    "xf86I2CBusInit",
    NULL
};
#endif

#ifdef XFree86LOADER
#ifdef USE_FB
static const char *fbSymbols[] = {
    "fbScreenInit",
    NULL
};
#else
static const char *cfbSymbols[] = {
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb32ScreenInit",
    "cfb24_32ScreenInit",
    NULL
};
#endif

static const char *xf8_32bppSymbols[] = {
    "xf86Overlay8Plus32Init",
    NULL
};

static const char *xaaSymbols[] = {
    "XAADestroyInfoRec",
    "XAACreateInfoRec",
    "XAAInit",
    "XAAStippleScanlineFuncLSBFirst",
    "XAAOverlayFBfuncs",
    "XAACachePlanarMonoStipple",
    "XAAScreenIndex",
    NULL
};

static const char *ramdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
    NULL
};

static MODULESETUPPROTO(R128Setup);

static XF86ModuleVersionInfo R128VersRec =
{
    R128_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    R128_VERSION_MAJOR, R128_VERSION_MINOR, R128_VERSION_PATCH,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    { 0, 0, 0, 0 }
};

XF86ModuleData r128ModuleData = { &R128VersRec, R128Setup, 0 };

static pointer R128Setup(pointer module, pointer opts, int *errmaj,
			 int *errmin)
{
    static Bool setupDone = FALSE;

    /* This module should be loaded only once, but check to be sure. */

    if (!setupDone) {
        setupDone = TRUE;
        xf86AddDriver(&R128, module, 0);

        /*
         * Modules that this driver always requires may be loaded here
         * by calling LoadSubModule().
         */
				/* FIXME: add DRI support here */

        /*
         * Tell the loader about symbols from other modules that this module
         * might refer to.
         */
        LoaderRefSymLists(vgahwSymbols,
#ifdef USE_FB
			  fbSymbols,
#else
			  cfbSymbols,
#endif
			  xaaSymbols, 
                          xf8_32bppSymbols,
			  ramdacSymbols,
			  fbdevHWSymbols,
			  vbeSymbols,
                          0 /* ddcsymbols */,
			  0 /* i2csymbols */,
			  0 /* shadowSymbols */,
			  NULL);

        /*
         * The return value must be non-NULL on success even though there
         * is no TearDownProc.
         */
        return (pointer)1;
    } else {
        if (errmaj) *errmaj = LDR_ONCEONLY;
        return NULL;
    }
}

#endif

/* Allocate our private R128InfoRec. */
static Bool R128GetRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate) return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(R128InfoRec), 1);
    return TRUE;
}

/* Free our private R128InfoRec. */
static void R128FreeRec(ScrnInfoPtr pScrn)
{
    if (!pScrn || !pScrn->driverPrivate) return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

/* Memory map the MMIO region.  Used during pre-init and by R128MapMem,
   below. */
static Bool R128MapMMIO(ScrnInfoPtr pScrn)
{
    R128InfoPtr info          = R128PTR(pScrn);

    if (info->FBDev) {
	info->MMIO = fbdevHWMapMMIO(pScrn);
    } else {
	info->MMIO = xf86MapPciMem(pScrn->scrnIndex,
				   VIDMEM_MMIO | VIDMEM_READSIDEEFFECT,
				   info->PciTag,
				   info->MMIOAddr,
				   R128_MMIOSIZE);
    }

    if (!info->MMIO) return FALSE;
    return TRUE;
}

/* Unmap the MMIO region.  Used during pre-init and by R128UnmapMem,
   below. */
static Bool R128UnmapMMIO(ScrnInfoPtr pScrn)
{
    R128InfoPtr info          = R128PTR(pScrn);
    
    if (info->FBDev)
	fbdevHWUnmapMMIO(pScrn);
    else {
	xf86UnMapVidMem(pScrn->scrnIndex, info->MMIO, R128_MMIOSIZE);
    }
    info->MMIO = NULL;
    return TRUE;
}

/* Memory map the frame buffer.  Used by R128MapMem, below. */
static Bool R128MapFB(ScrnInfoPtr pScrn)
{
    R128InfoPtr info          = R128PTR(pScrn);

    if (info->FBDev) {
	info->FB = fbdevHWMapVidmem(pScrn);
    } else {
	info->FB = xf86MapPciMem(pScrn->scrnIndex,
				 VIDMEM_FRAMEBUFFER,
				 info->PciTag,
				 info->LinearAddr,
				 info->FbMapSize);
    }

    if (!info->FB) return FALSE;
    return TRUE;
}

/* Unmap the frame buffer.  Used by R128UnmapMem, below. */
static Bool R128UnmapFB(ScrnInfoPtr pScrn)
{
    R128InfoPtr info          = R128PTR(pScrn);
    
    if(info->FBDev) 
	fbdevHWUnmapVidmem(pScrn);
    else
	xf86UnMapVidMem(pScrn->scrnIndex, info->FB, info->FbMapSize);
    info->FB = NULL;
    return TRUE;
}

/* Memory map the MMIO region and the frame buffer. */
static Bool R128MapMem(ScrnInfoPtr pScrn)
{
    if (!R128MapMMIO(pScrn)) return FALSE;
    if (!R128MapFB(pScrn)) {
	R128UnmapMMIO(pScrn);
	return FALSE;
    }
    return TRUE;
}

/* Unmap the MMIO region and the frame buffer. */
static Bool R128UnmapMem(ScrnInfoPtr pScrn)
{
    if (!R128UnmapMMIO(pScrn) || !R128UnmapFB(pScrn)) return FALSE;
    return TRUE;
}

/* Read PLL information */
int INPLL(ScrnInfoPtr pScrn, int addr)
{
    R128MMIO_VARS();
    
    OUTREG8(R128_CLOCK_CNTL_INDEX, addr & 0x1f);
    return INREG(R128_CLOCK_CNTL_DATA);
}

#if 0
/* Read PAL information (only used for debugging). */
static int INPAL(int idx)
{
    R128MMIO_VARS();
    
    OUTREG(R128_PALETTE_INDEX, idx << 16);
    return INREG(R128_PALETTE_DATA);
}
#endif

/* Wait for vertical sync. */
void R128WaitForVerticalSync(ScrnInfoPtr pScrn)
{
    int i;
    R128MMIO_VARS();

    OUTREG(R128_GEN_INT_STATUS, R128_VSYNC_INT_AK);
    for (i = 0; i < R128_TIMEOUT; i++) {
	if (INREG(R128_GEN_INT_STATUS) & R128_VSYNC_INT) break;
    }
}

/* Blank screen. */
static void R128Blank(ScrnInfoPtr pScrn)
{
    R128MMIO_VARS();

    OUTREGP(R128_CRTC_EXT_CNTL, R128_CRTC_DISPLAY_DIS, ~R128_CRTC_DISPLAY_DIS);
}

/* Unblank screen. */
static void R128Unblank(ScrnInfoPtr pScrn)
{
    R128MMIO_VARS();

    OUTREGP(R128_CRTC_EXT_CNTL, 0, ~R128_CRTC_DISPLAY_DIS);
}

/* Compute log base 2 of val. */
static int R128MinBits(int val)
{
    int bits;

    if (!val) return 1;
    for (bits = 0; val; val >>= 1, ++bits);
    return bits;
}

/* Compute n/d with rounding. */
static int R128Div(int n, int d)
{
    return (n + (d / 2)) / d;
}

/* Read PLL parameters from BIOS block.  Default to typical values if there
   is no BIOS. */
static Bool R128GetPLLParameters(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info = R128PTR(pScrn);
    R128PLLPtr    pll  = &info->pll;
    CARD16        bios_header;
    CARD16        pll_info_block;
    CARD8         tmp[64];
    Bool	  BIOSFromPCI = TRUE;
	
#define R128ReadBIOS(offset, buffer, length)                                  \
    (BIOSFromPCI ?							      \
     xf86ReadPciBIOS(offset, info->PciTag, 0, buffer, length) :               \
     xf86ReadBIOS(info->BIOSAddr, offset, buffer, length))

    R128ReadBIOS(0, tmp, sizeof(tmp));
    if (tmp[0] != 0x55 || tmp[1] != 0xaa)
    {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "Video BIOS not detected in PCI space!\n");
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "Attempting to read Video BIOS from legacy ISA space!\n");
	BIOSFromPCI = FALSE;
	info->BIOSAddr = 0x000c0000;
	R128ReadBIOS(0, tmp, sizeof(tmp));
    }
    if (tmp[0] != 0x55 || tmp[1] != 0xaa) {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "Video BIOS not detected, using default PLL parameters!\n");
				/* These probably aren't going to work for
                                   the card you are using.  Specifically,
                                   reference freq can be 29.50MHz,
                                   28.63MHz, or 14.32MHz.  YMMV. */
	pll->reference_freq = 2950;
	pll->reference_div  = 65;
	pll->min_pll_freq   = 12500;
	pll->max_pll_freq   = 25000;
	pll->xclk           = 10300;
    } else {
	R128ReadBIOS(0x48,
		     (CARD8 *)&bios_header, sizeof(bios_header));
	R128ReadBIOS(bios_header + 0x30,
		     (CARD8 *)&pll_info_block, sizeof(pll_info_block));
	R128TRACE(("Header at 0x%04x; PLL Information at 0x%04x\n",
		   bios_header, pll_info_block));

	R128ReadBIOS(pll_info_block + 0x0e,
		     (CARD8 *)&pll->reference_freq,
		     sizeof(pll->reference_freq));
	R128ReadBIOS(pll_info_block + 0x10,
		     (CARD8 *)&pll->reference_div,
		     sizeof(pll->reference_div));
	R128ReadBIOS(pll_info_block + 0x12,
		     (CARD8 *)&pll->min_pll_freq,
		     sizeof(pll->min_pll_freq));
	R128ReadBIOS(pll_info_block + 0x16,
		     (CARD8 *)&pll->max_pll_freq,
		     sizeof(pll->max_pll_freq));
	R128ReadBIOS(pll_info_block + 0x08,
		     (CARD8 *)&pll->xclk, sizeof(pll->xclk));
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "PLL parameters: rf=%d rd=%d min=%d max=%d; xclk=%d\n",
	       pll->reference_freq,
	       pll->reference_div,
	       pll->min_pll_freq,
	       pll->max_pll_freq,
	       pll->xclk);

    return TRUE;
}

static
OptionInfoPtr
R128AvailableOptions(int chipid, int busid)
{
    return R128Options;
}

/* Return the string name for supported chipset 'n'; NULL otherwise. */
static void R128Identify(int flags)
{
    xf86PrintChipsets(R128_NAME,
		      "Driver for ATI Rage 128 chipset",
		      R128Chipsets);
}

/* Return TRUE if chipset is present; FALSE otherwise. */
static Bool R128Probe(DriverPtr drv, int flags)
{
    int           numUsed;
    int           numDevSections;
    int           *usedChips;
    GDevPtr       *devSections;
    EntityInfoPtr pEnt;
    Bool          foundScreen = FALSE;
    int           i;

    if ((numDevSections = xf86MatchDevice(R128_NAME, &devSections)) <= 0)
	return FALSE;

    if (!xf86GetPciVideoInfo()) return FALSE;

    numUsed = xf86MatchPciInstances(R128_NAME,
				    PCI_VENDOR_ATI,
				    R128Chipsets,
				    R128PciChipsets,
				    devSections,
				    numDevSections,
				    drv,
				    &usedChips);

    if (numUsed<=0) return FALSE;

    if (flags & PROBE_DETECT)
    	foundScreen = TRUE;
    else for (i = 0; i < numUsed; i++) {
	pEnt = xf86GetEntityInfo(usedChips[i]);

	if (pEnt->active) {
	    ScrnInfoPtr pScrn    = xf86AllocateScreen(drv, 0);

	    pScrn->driverVersion = R128_VERSION;
	    pScrn->driverName    = R128_NAME;
	    pScrn->name          = R128_NAME;
	    pScrn->Probe         = R128Probe;
	    pScrn->PreInit       = R128PreInit;
	    pScrn->ScreenInit    = R128ScreenInit;
	    pScrn->SwitchMode    = R128SwitchMode;
	    pScrn->AdjustFrame   = R128AdjustFrame;
	    pScrn->EnterVT       = R128EnterVT;
	    pScrn->LeaveVT       = R128LeaveVT;
	    pScrn->FreeScreen    = R128FreeScreen;
	    pScrn->ValidMode     = R128ValidMode;

	    foundScreen          = TRUE;
	    
	    xf86ConfigActivePciEntity(pScrn, usedChips[i], R128PciChipsets,
				      0, 0, 0, 0, 0);
	}
	xfree(pEnt);
    }

    if (numUsed) xfree(usedChips);
    xfree(devSections);
    
    return foundScreen;
}

/* This is called by R128PreInit to set up the default visual. */
static Bool R128PreInitVisual(ScrnInfoPtr pScrn)
{
    R128InfoPtr info          = R128PTR(pScrn);
    
    if (!xf86SetDepthBpp(pScrn, 8, 8, 8, (Support24bppFb
					  | Support32bppFb
#ifndef USE_FB
					  | SupportConvert32to24
#endif
					  )))
	return FALSE;

    switch (pScrn->depth) {
    case 8:
    case 15:
    case 16:
    case 24:
	break;
    default:
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Given depth (%d) is not supported by %s driver\n",
		   pScrn->depth, R128_NAME);
	return FALSE;
    }

    xf86PrintDepthBpp(pScrn);

    info->fifo_slots  = 0;
    info->pix24bpp    = xf86GetBppFromDepth(pScrn, pScrn->depth);
    info->pixel_bytes = pScrn->bitsPerPixel / 8;
    info->pixel_code  = (pScrn->bitsPerPixel != 16
			 ? pScrn->bitsPerPixel
			 : pScrn->depth);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "Pixel depth = %d bits stored in %d byte%s (%d bpp pixmaps)\n",
	       pScrn->depth,
	       info->pixel_bytes,
	       info->pixel_bytes > 1 ? "s" : "",
	       info->pix24bpp);


    if (!xf86SetDefaultVisual(pScrn, -1)) return FALSE;

    if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Default visual (%s) is not supported at depth %d\n",
		   xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
	return FALSE;
    }
    return TRUE;

}

/* This is called by R128PreInit to handle all color weight issues. */
static Bool R128PreInitWeight(ScrnInfoPtr pScrn)
{
    R128InfoPtr info          = R128PTR(pScrn);
    
				/* Save flag for 6 bit DAC to use for
                                   setting CRTC registers.  Otherwise use
                                   an 8 bit DAC, even if xf86SetWeight sets
                                   pScrn->rgbBits to some value other than
                                   8. */
    info->dac6bits = FALSE;
    if (pScrn->depth > 8) {
	rgb defaultWeight = { 0, 0, 0 };
	if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight)) return FALSE;
    } else {
	pScrn->rgbBits = 8;
	if (xf86ReturnOptValBool(R128Options, OPTION_DAC_6BIT, FALSE)) {
	    pScrn->rgbBits = 6;
	    info->dac6bits = TRUE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "Using %d bits per RGB (%d bit DAC)\n",
	       pScrn->rgbBits, info->dac6bits ? 6 : 8);

    return TRUE;

}

/* This is called by R128PreInit to handle config file overrides for things
   like chipset and memory regions.  Also determine memory size and type.
   If memory type ever needs an override, put it in this routine. */
static Bool R128PreInitConfig(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info   = R128PTR(pScrn);
    EntityInfoPtr pEnt   = info->pEnt;
    GDevPtr       dev    = pEnt->device;
    int           offset = 0;	/* RAM Type */
    MessageType   from;
    unsigned char *R128MMIO;
				/* Chipset */
    from = X_PROBED;
    if (dev->chipset && *dev->chipset) {
	info->Chipset  = xf86StringToToken(R128Chipsets, dev->chipset);
	from           = X_CONFIG;
    } else if (dev->chipID >= 0) {
	info->Chipset  = dev->chipID;
	from           = X_CONFIG;
    } else {
	info->Chipset = info->PciInfo->chipType;
    }
    pScrn->chipset = (char *)xf86TokenToString(R128Chipsets, info->Chipset);

    if (!pScrn->chipset) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04x is not recognized\n", info->Chipset);
	return FALSE;
    }

    if (info->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognized\n", pScrn->chipset);
	return FALSE;
    }
    
    xf86DrvMsg(pScrn->scrnIndex, from,
	       "Chipset: \"%s\" (ChipID = 0x%04x)\n",
	       pScrn->chipset,
	       info->Chipset);

				/* Framebuffer */

    from             = X_PROBED;
    info->LinearAddr = info->PciInfo->memBase[0] & 0xfc000000;
    if (dev->MemBase) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Linear address override, using 0x%08x instead of 0x%08x\n",
		   dev->MemBase,
		   info->LinearAddr);
	info->LinearAddr = dev->MemBase;
	from             = X_CONFIG;
    } else if (!info->LinearAddr) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "No valid linear framebuffer address\n");
	return FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from,
	       "Linear framebuffer at 0x%08lx\n", info->LinearAddr);

				/* MMIO registers */
    from             = X_PROBED;
    info->MMIOAddr   = info->PciInfo->memBase[2] & 0xffffff00;
    if (dev->IOBase) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "MMIO address override, using 0x%08x instead of 0x%08x\n",
		   dev->IOBase,
		   info->MMIOAddr);
	info->MMIOAddr = dev->IOBase;
	from           = X_CONFIG;
    } else if (!info->MMIOAddr) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid MMIO address\n");
	return FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from,
	       "MMIO registers at 0x%08lx\n", info->MMIOAddr);

				/* BIOS */
    from              = X_PROBED;
    info->BIOSAddr    = info->PciInfo->biosBase & 0xfffe0000;
    if (dev->BiosBase) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "BIOS address override, using 0x%08x instead of 0x%08x\n",
		   dev->BiosBase,
		   info->BIOSAddr);
	info->BIOSAddr = dev->BiosBase;
	from           = X_CONFIG;
    }
    if (info->BIOSAddr) {
	xf86DrvMsg(pScrn->scrnIndex, from,
		   "BIOS at 0x%08lx\n", info->BIOSAddr);
    }

				/* RAM */
    from             = X_PROBED;
    R128MapMMIO(pScrn);
    R128MMIO         = info->MMIO;
    if (info->FBDev)
	pScrn->videoRam = fbdevHWGetVidmem(pScrn) / 1024;
    else
	pScrn->videoRam = INREG(R128_CONFIG_MEMSIZE) / 1024;
    info->MemCntl    = INREG(R128_MEM_CNTL);
    info->BusCntl    = INREG(R128_BUS_CNTL);
    R128MMIO         = NULL;
    R128UnmapMMIO(pScrn);
    switch (info->MemCntl & 0x3) {
    case 0:			/* SDR SGRAM 1:1 */
	switch (info->Chipset) {
	case PCI_CHIP_RAGE128RE: 
	case PCI_CHIP_RAGE128RF: offset = 0; break; /* 128-bit SDR SGRAM 1:1 */
	case PCI_CHIP_RAGE128RK:
	case PCI_CHIP_RAGE128RL:
	default:                 offset = 1; break; /*  64-bit SDR SGRAM 1:1 */
	}
    case 1:                      offset = 2; break; /*  64-bit SDR SGRAM 2:1 */
    case 2:                      offset = 3; break; /*  64-bit DDR SGRAM     */
    default:                     offset = 1; break; /*  64-bit SDR SGRAM 1:1 */
    }
    info->ram = &R128RAM[offset];
    
    if (dev->videoRam) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Video RAM override, using %d kB instead of %d kB\n",
		   dev->videoRam,
		   pScrn->videoRam);
	from             = X_CONFIG;
	pScrn->videoRam  = dev->videoRam;
    } 
    pScrn->videoRam  &= ~1023;
    info->FbMapSize  = pScrn->videoRam * 1024;
    xf86DrvMsg(pScrn->scrnIndex, from,
	       "VideoRAM: %d kByte (%s)\n", pScrn->videoRam, info->ram->name);

    return TRUE;
}

static Bool R128PreInitDDC(ScrnInfoPtr pScrn)
{
				/* FIXME: DDC support goes here. */
#if 0
				/* Using the GPIO_MONID register for DDC2
                                   does not appear to work as expected.
                                   Hence, the implementation of DDC is
                                   deferred. */
    R128InfoPtr   info = R128PTR(pScrn);
    Bool          ret  = TRUE;

    if (!xf86LoadSubModule(pScrn, "ddc")) return FALSE;
    xf86LoaderReqSymLists(ddcSymbols, NULL);
    if (!xf86LoadSubModule(pScrn, "i2c")) return FALSE;
    xf86LoaderReqSymLists(i2cSymbols, NULL);

    R128MapMMIO(pScrn);
    if (!R128I2CInit(pScrn)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "I2C initialization failed\n");
	ret = FALSE;
    } else {
	xf86SetDDCProperties(pScrn,xf86PrintEDID(
	    xf86DoEDID_DDC2(pScrn->scrnIndex, info->i2c)));
    }
    R128UnmapMMIO(pScrn);
    return ret;
#else
    return TRUE;
#endif
}

/* This is called by R128PreInit to initialize gamma correction. */
static Bool R128PreInitGamma(ScrnInfoPtr pScrn)
{
    Gamma zeros = { 0.0, 0.0, 0.0 };

    if (!xf86SetGamma(pScrn, zeros)) return FALSE;
    return TRUE;
}

/* This is called by R128PreInit to validate modes and compute parameters
   for all of the valid modes. */
static Bool R128PreInitModes(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info = R128PTR(pScrn);
    ClockRangePtr clockRanges;
    int           modesFound;
    char          *mod = NULL;
    const char    *Sym = NULL;

				/* Get mode information */
    pScrn->progClock               = TRUE;
    clockRanges                    = xnfalloc(sizeof(*clockRanges));
    clockRanges->next              = NULL;
    clockRanges->minClock          = info->pll.min_pll_freq;
    clockRanges->maxClock          = info->pll.max_pll_freq * 10;
    clockRanges->clockIndex        = -1;
    clockRanges->interlaceAllowed  = TRUE;
    clockRanges->doubleScanAllowed = TRUE;
    
    modesFound = xf86ValidateModes(pScrn,
				   pScrn->monitor->Modes,
				   pScrn->display->modes,
				   clockRanges,
				   NULL,	/* linePitches */
				   8 * 64,      /* minPitch */
				   8 * 1024,	/* maxPitch */
				   64 * pScrn->bitsPerPixel, /* pitchInc */
				   128,	        /* minHeight */
				   2048,        /* maxHeight */
				   pScrn->virtualX,
				   pScrn->virtualY,
				   info->FbMapSize,
				   LOOKUP_BEST_REFRESH);

    if (modesFound < 1 && info->FBDev) {
	fbdevHWUseBuildinMode(pScrn);
	pScrn->displayWidth = pScrn->virtualX; /* FIXME: might be wrong */
	modesFound = 1;
    }

    if (modesFound == -1) return FALSE;
    xf86PruneDriverModes(pScrn);
    if (!modesFound || !pScrn->modes) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	return FALSE;
    }
    xf86SetCrtcForModes(pScrn, 0);
    pScrn->currentMode = pScrn->modes;
    xf86PrintModes(pScrn);

				/* Set DPI */
    xf86SetDpi(pScrn, 0, 0);

				/* Get ScreenInit function */
#ifdef USE_FB
    mod = "fb"; Sym = "fbScreenInit";
#else
    switch (pScrn->bitsPerPixel) {
    case  8: mod = "cfb";   Sym = "cfbScreenInit";   break;
    case 16: mod = "cfb16"; Sym = "cfb16ScreenInit"; break;
    case 24:
	if (info->pix24bpp == 24) {
	    mod = "cfb24";      Sym = "cfb24ScreenInit";
        } else {
	    mod = "xf24_32bpp"; Sym = "cfb24_32ScreenInit";
	}
	break;
    case 32: mod = "cfb32"; Sym = "cfb32ScreenInit"; break;
    }
#endif
    if (mod && !xf86LoadSubModule(pScrn, mod)) return FALSE;
    xf86LoaderReqSymbols(Sym, NULL);
    
    return TRUE;
}

/* This is called by R128PreInit to initialize the hardware cursor. */
static Bool R128PreInitCursor(ScrnInfoPtr pScrn)
{
    if (!xf86ReturnOptValBool(R128Options, OPTION_SW_CURSOR, FALSE)) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) return FALSE;
    }
    return TRUE;
}

/* This is called by R128PreInit to initialize hardware acceleration. */
static Bool R128PreInitAccel(ScrnInfoPtr pScrn)
{
    if (!xf86ReturnOptValBool(R128Options, OPTION_NOACCEL, FALSE)) {
	if (!xf86LoadSubModule(pScrn, "xaa")) return FALSE;
    }
    return TRUE;
}

static Bool R128PreInitInt10(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info = R128PTR(pScrn);    
#if 1
    if (xf86LoadSubModule(pScrn, "int10")) {
 	xf86Int10InfoPtr pInt;
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"initializing int10\n");
	pInt = xf86InitInt10(info->pEnt->index);
	xf86FreeInt10(pInt);
    }
#endif
    return TRUE;
}

extern xf86MonPtr ConfiguredMonitor;

static void
R128ProbeDDC(ScrnInfoPtr pScrn, int index)
{
    vbeInfoPtr pVbe;
    if (xf86LoadSubModule(pScrn, "vbe")) {
	pVbe = VBEInit(NULL,index);
	ConfiguredMonitor = vbeDoEDID(pVbe);
    }
}

/* R128PreInit is called once at server startup. */
static Bool R128PreInit(ScrnInfoPtr pScrn, int flags)
{
    R128InfoPtr   info;

    R128TRACE(("R128PreInit\n"));
    if (pScrn->numEntities != 1) return FALSE;

    if (!R128GetRec(pScrn)) return FALSE;

    info               = R128PTR(pScrn);

    info->pEnt         = xf86GetEntityInfo(pScrn->entityList[0]);
    if (info->pEnt->location.type != BUS_PCI) goto fail;

    if (flags & PROBE_DETECT) {
	R128ProbeDDC(pScrn, info->pEnt->index);
	return TRUE;
    }

    if (!xf86LoadSubModule(pScrn, "vgahw")) return FALSE;
    xf86LoaderReqSymLists(vgahwSymbols, NULL);
    if (!vgaHWGetHWRec(pScrn)) {
	R128FreeRec(pScrn);
	return FALSE;
    }

    info->PciInfo      = xf86GetPciInfoForEntity(info->pEnt->index);
    info->PciTag       = pciTag(info->PciInfo->bus,
				info->PciInfo->device,
				info->PciInfo->func);
    
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "PCI bus %d card %d func %d\n",
	       info->PciInfo->bus,
	       info->PciInfo->device,
	       info->PciInfo->func);

    if (xf86RegisterResources(info->pEnt->index, 0, ResNone)) goto fail;

    pScrn->racMemFlags = RAC_FB | RAC_COLORMAP;
    pScrn->monitor     = pScrn->confScreen->monitor;

    if (!R128PreInitVisual(pScrn))    goto fail;
    
				/* We can't do this until we have a
                                   pScrn->display. */
    xf86CollectOptions(pScrn, NULL);
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, R128Options);

    if (!R128PreInitWeight(pScrn))    goto fail;
    
    if (xf86ReturnOptValBool(R128Options, OPTION_FBDEV, FALSE)) {
	info->FBDev = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Using framebuffer device\n");
    }

    if (info->FBDev) {
	/* check for linux framebuffer device */
	if (!xf86LoadSubModule(pScrn, "fbdevhw")) return FALSE;
	xf86LoaderReqSymLists(fbdevHWSymbols, NULL);
	if (!fbdevHWInit(pScrn, info->PciInfo, NULL)) return FALSE;
	pScrn->SwitchMode    = fbdevHWSwitchMode;
	pScrn->AdjustFrame   = fbdevHWAdjustFrame;
	pScrn->EnterVT       = R128EnterVTFBDev;
	pScrn->LeaveVT       = R128LeaveVTFBDev;
	pScrn->ValidMode     = fbdevHWValidMode;
    }
    
    if (!info->FBDev)
	if (!R128PreInitInt10(pScrn))     goto fail;
    
    if (!R128PreInitConfig(pScrn))    goto fail;
    
    if (!R128GetPLLParameters(pScrn)) goto fail;

    if (!R128PreInitDDC(pScrn))       goto fail;
    
    if (!R128PreInitGamma(pScrn))     goto fail;

    if (!R128PreInitModes(pScrn))     goto fail;
    
    if (!R128PreInitCursor(pScrn))    goto fail;
    
    if (!R128PreInitAccel(pScrn))     goto fail;

    return TRUE;

  fail:
				/* Pre-init failed. */
    vgaHWFreeHWRec(pScrn);
    R128FreeRec(pScrn);
    return FALSE;
}

/* Load a palette for 15bpp mode.  This sends 32 values. */
static void R128LoadPalette15(ScrnInfoPtr pScrn, int numColors,
			      int *indices, LOCO *colors, VisualPtr pVisual)
{
    int           i;
    int           idx;
    unsigned char r, g, b;
    R128MMIO_VARS();

    for (i = 0; i < numColors; i++) {
	idx = indices[i];
	r   = colors[idx].red;
	g   = colors[idx].green;
	b   = colors[idx].blue;
	OUTPAL(idx * 8, r, g, b);
    }
}

/* Load a palette for 16bpp mode.  This sends 64 values. */
static void R128LoadPalette16(ScrnInfoPtr pScrn, int numColors,
			      int *indices, LOCO *colors, VisualPtr pVisual)
{
    int           i;
    int           idx;
    unsigned char r, g, b;
    R128MMIO_VARS();

				/* There are twice as many green values as
                                   there are values for red and blue.  So,
                                   we take each red and blue pair, and
                                   combine it with each of the two green
                                   values. */
    for (i = 0; i < numColors; i++) {
	idx = indices[i];
	r   = colors[idx / 2].red;
	g   = colors[idx].green;
	b   = colors[idx / 2].blue;
	OUTPAL(idx * 4, r, g, b);
    }
}

/* Load a palette for 8bpp mode.  This sends 256 values. */
static void R128LoadPalette(ScrnInfoPtr pScrn, int numColors,
			    int *indices, LOCO *colors, VisualPtr pVisual)
{
    int           i;
    int           idx;
    unsigned char r, g, b;
    R128MMIO_VARS();

    for (i = 0; i < numColors; i++) {
	idx = indices[i];
	r   = colors[idx].red;
	b   = colors[idx].blue;
	g   = colors[idx].green;
	OUTPAL(idx, r, g, b);
    }
}

/* Called at the start of each server generation. */
static Bool R128ScreenInit(int scrnIndex, ScreenPtr pScreen,
			   int argc, char **argv)
{
    ScrnInfoPtr pScrn  = xf86Screens[pScreen->myNum];
    R128InfoPtr info   = R128PTR(pScrn);
    BoxRec      MemBox;
    int         y2;

    R128TRACE(("R128ScreenInit %x %d\n", pScrn->memPhysBase, pScrn->fbOffset));
    
    if (!R128MapMem(pScrn)) return FALSE;
    pScrn->fbOffset    = 0;
    
    R128Save(pScrn);
    if (info->FBDev) {
	if (!fbdevHWModeInit(pScrn, pScrn->currentMode)) return FALSE;
    } else {
	if (!R128ModeInit(pScrn, pScrn->currentMode)) return FALSE;
    }

    R128SaveScreen(pScreen, SCREEN_SAVER_ON);
    pScrn->AdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

				/* Visual setup */
    miClearVisualTypes();
    if (!miSetVisualTypes(pScrn->depth,
			  miGetDefaultVisualMask(pScrn->depth),
			  pScrn->rgbBits,
			  pScrn->defaultVisual)) return FALSE;

#ifdef USE_FB
    if (!fbScreenInit (pScreen, info->FB,
		       pScrn->virtualX, pScrn->virtualY,
		       pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth,
		       pScrn->bitsPerPixel))
	return FALSE;
#else
    switch (pScrn->bitsPerPixel) {
    case 8:
	if (!cfbScreenInit(pScreen, info->FB,
			   pScrn->virtualX, pScrn->virtualY,
			   pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth))
	    return FALSE;
	break;
    case 16:
	if (!cfb16ScreenInit(pScreen, info->FB,
			     pScrn->virtualX, pScrn->virtualY,
			     pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth))
	    return FALSE;
	break;
    case 24:
	if (info->pix24bpp == 24) {
	    if (!cfb24ScreenInit(pScreen, info->FB,
				 pScrn->virtualX, pScrn->virtualY,
				 pScrn->xDpi, pScrn->yDpi,
				 pScrn->displayWidth))
		return FALSE;
	} else {
	    if (!cfb24_32ScreenInit(pScreen, info->FB,
				 pScrn->virtualX, pScrn->virtualY,
				 pScrn->xDpi, pScrn->yDpi,
				 pScrn->displayWidth))
		return FALSE;
	}
	break;
    case 32:
	if (!cfb32ScreenInit(pScreen, info->FB,
			     pScrn->virtualX, pScrn->virtualY,
			     pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth))
	    return FALSE;
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Invalid bpp (%d)\n", pScrn->bitsPerPixel);
	return FALSE;
    }
#endif
    xf86SetBlackWhitePixels(pScreen);

    if (pScrn->bitsPerPixel > 8) {
	VisualPtr visual;

	for (visual = pScreen->visuals + pScreen->numVisuals;
	     visual >= pScreen->visuals;
	     visual--) {
	    if ((visual->class | DynamicClass) == DirectColor) {
		visual->offsetRed   = pScrn->offset.red;
		visual->offsetGreen = pScrn->offset.green;
		visual->offsetBlue  = pScrn->offset.blue;
		visual->redMask     = pScrn->mask.red;
		visual->greenMask   = pScrn->mask.green;
		visual->blueMask    = pScrn->mask.blue;
	    }
	}
    }

				/* Memory manager setup */
    MemBox.x1 = 0;
    MemBox.y1 = 0;
    MemBox.x2 = pScrn->displayWidth;
    y2        = info->FbMapSize / (pScrn->displayWidth * info->pixel_bytes);
    if (y2 >= 32768) y2 = 32767; /* because MemBox.y2 is signed short */
    MemBox.y2 = y2;

				/* The acceleration engine uses 14 bit
                                   signed coordinates, so we can't have any
                                   drawable caches beyond this region. */
    if (MemBox.y2 > 8191) MemBox.y2 = 8191;

    if (!xf86InitFBManager(pScreen, &MemBox)) {
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Memory manager initialization to (%d,%d) (%d,%d) failed\n",
		   MemBox.x1, MemBox.y1, MemBox.x2, MemBox.y2);
	return FALSE;
    } else {
	int       width, height;
	FBAreaPtr fbarea;
	
	xf86DrvMsg(scrnIndex, X_INFO,
		   "Memory manager initialized to (%d,%d) (%d,%d)\n",
		   MemBox.x1, MemBox.y1, MemBox.x2, MemBox.y2);
	if ((fbarea = xf86AllocateOffscreenArea(pScreen, pScrn->displayWidth,
						2, 0, NULL, NULL, NULL))) {
	    xf86DrvMsg(scrnIndex, X_INFO,
		       "Reserved area from (%d,%d) to (%d,%d)\n",
		       fbarea->box.x1, fbarea->box.y1,
		       fbarea->box.x2, fbarea->box.y2);
	} else {
	    xf86DrvMsg(scrnIndex, X_ERROR, "Unable to reserve area\n");
	}
	if (xf86QueryLargestOffscreenArea(pScreen, &width, &height, 0, 0, 0)) {
	    xf86DrvMsg(scrnIndex, X_INFO,
		       "Largest offscreen area available: %d x %d\n",
		       width, height);
	}
    }

				/* Backing store setup */
    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);

				/* Acceleration setup */
    if (!xf86ReturnOptValBool(R128Options, OPTION_NOACCEL, FALSE)) {
	if (R128AccelInit(pScreen)) {
	    xf86DrvMsg(scrnIndex, X_INFO, "Acceleration enabled\n");
	} else {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "Acceleration initialization failed\n");
	    xf86DrvMsg(scrnIndex, X_INFO, "Acceleration disabled\n");
	}
    } else {
	xf86DrvMsg(scrnIndex, X_INFO, "Acceleration disabled\n");
    }

				/* Cursor setup */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

				/* Hardware cursor setup */
    if (!xf86ReturnOptValBool(R128Options, OPTION_SW_CURSOR, FALSE)) {
	if (R128CursorInit(pScreen)) {
	    int width, height;
	    
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "Using hardware cursor (scanline %d)\n",
		       info->cursor_start / pScrn->displayWidth);
	    if (xf86QueryLargestOffscreenArea(pScreen, &width, &height,
					      0, 0, 0)) {
		xf86DrvMsg(scrnIndex, X_INFO,
			   "Largest offscreen area available: %d x %d\n",
			   width, height);
	    }
	} else {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "Hardware cursor initialization failed\n");
	    xf86DrvMsg(scrnIndex, X_INFO, "Using software cursor\n");
	}
    } else {
	xf86DrvMsg(scrnIndex, X_INFO, "Using software cursor\n");
    }

				/* Colormap setup */
    if (!miCreateDefColormap(pScreen)) return FALSE;
    if (pScrn->depth == 15) {
	if (!xf86HandleColormaps(pScreen, 256, info->dac6bits ? 6 : 8,
				 (info->FBDev ? fbdevHWLoadPalette :
				 R128LoadPalette15), NULL,
				 CMAP_PALETTED_TRUECOLOR
				 | CMAP_RELOAD_ON_MODE_SWITCH
				 | CMAP_LOAD_EVEN_IF_OFFSCREEN)) return FALSE;
    } else if (pScrn->depth == 16) {
	if (!xf86HandleColormaps(pScreen, 256, info->dac6bits ? 6 : 8,
				 (info->FBDev ? fbdevHWLoadPalette :
				 R128LoadPalette16), NULL,
				 CMAP_PALETTED_TRUECOLOR
				 | CMAP_RELOAD_ON_MODE_SWITCH
				 | CMAP_LOAD_EVEN_IF_OFFSCREEN)) return FALSE;
    } else {
	if (!xf86HandleColormaps(pScreen, 256, info->dac6bits ? 6 : 8,
				 (info->FBDev ? fbdevHWLoadPalette :
				 R128LoadPalette), NULL,
				 CMAP_PALETTED_TRUECOLOR
				 | CMAP_RELOAD_ON_MODE_SWITCH
				 | CMAP_LOAD_EVEN_IF_OFFSCREEN)) return FALSE;
    }

				/* DPMS setup */
#ifdef DPMSExtension
    xf86DPMSInit(pScreen, R128DisplayPowerManagementSet, 0);
#endif

				/* Xv setup */
#ifdef XvExtension
    {
	XF86VideoAdaptorPtr *ptr;
	int                 n;

	if ((n = xf86XVListGenericAdaptors(pScrn, &ptr)))
	    xf86XVScreenInit(pScreen, ptr, n);
    }
#endif

				/* Provide SaveScreen */
    pScreen->SaveScreen  = R128SaveScreen;
    
				/* Wrap CloseScreen */
    info->CloseScreen    = pScreen->CloseScreen;
    pScreen->CloseScreen = R128CloseScreen;

				/* Note unused options */
    if (serverGeneration == 1)
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

    return TRUE;
}

/* Write common registers (initialized to 0). */
static void R128RestoreCommonRegisters(ScrnInfoPtr pScrn, R128SavePtr restore)
{
    R128MMIO_VARS();
    
    OUTREG(R128_OVR_CLR,              restore->ovr_clr);
    OUTREG(R128_OVR_WID_LEFT_RIGHT,   restore->ovr_wid_left_right);
    OUTREG(R128_OVR_WID_TOP_BOTTOM,   restore->ovr_wid_top_bottom);
    OUTREG(R128_OV0_SCALE_CNTL,       restore->ov0_scale_cntl);
    OUTREG(R128_MPP_TB_CONFIG,        restore->mpp_tb_config );
    OUTREG(R128_MPP_GP_CONFIG,        restore->mpp_gp_config );
    OUTREG(R128_SUBPIC_CNTL,          restore->subpic_cntl);
    OUTREG(R128_VIPH_CONTROL,         restore->viph_control);
    OUTREG(R128_I2C_CNTL_1,           restore->i2c_cntl_1);
    OUTREG(R128_GEN_INT_CNTL,         restore->gen_int_cntl);
    OUTREG(R128_CAP0_TRIG_CNTL,       restore->cap0_trig_cntl);
    OUTREG(R128_CAP1_TRIG_CNTL,       restore->cap1_trig_cntl);
    OUTREG(R128_BUS_CNTL,	      restore->bus_cntl);
}

/* Write CRTC registers. */
static void R128RestoreCrtcRegisters(ScrnInfoPtr pScrn, R128SavePtr restore)
{
    R128MMIO_VARS();
    
    OUTREG(R128_CRTC_GEN_CNTL,        restore->crtc_gen_cntl);

    OUTREGP(R128_CRTC_EXT_CNTL, restore->crtc_ext_cntl,
	    R128_CRTC_VSYNC_DIS | R128_CRTC_HSYNC_DIS | R128_CRTC_DISPLAY_DIS);
    
    OUTREGP(R128_DAC_CNTL, restore->dac_cntl,
	    R128_DAC_RANGE_CNTL | R128_DAC_BLANKING);

    OUTREG(R128_CRTC_H_TOTAL_DISP,    restore->crtc_h_total_disp);
    OUTREG(R128_CRTC_H_SYNC_STRT_WID, restore->crtc_h_sync_strt_wid);
    OUTREG(R128_CRTC_V_TOTAL_DISP,    restore->crtc_v_total_disp);
    OUTREG(R128_CRTC_V_SYNC_STRT_WID, restore->crtc_v_sync_strt_wid);
    OUTREG(R128_CRTC_OFFSET,          restore->crtc_offset);
    OUTREG(R128_CRTC_OFFSET_CNTL,     restore->crtc_offset_cntl);
    OUTREG(R128_CRTC_PITCH,           restore->crtc_pitch);
}

static void R128PLLWaitForReadUpdateComplete(ScrnInfoPtr pScrn)
{
    while (INPLL(pScrn, R128_PPLL_REF_DIV) & R128_PPLL_ATOMIC_UPDATE_R);
}

static void R128PLLWriteUpdate(ScrnInfoPtr pScrn)
{
    R128MMIO_VARS();
    
    OUTPLLP(pScrn, R128_PPLL_REF_DIV, R128_PPLL_ATOMIC_UPDATE_W, 0xffff);
}

/* Write PLL registers. */
static void R128RestorePLLRegisters(ScrnInfoPtr pScrn, R128SavePtr restore)
{
    R128MMIO_VARS();
    
    OUTREGP(R128_CLOCK_CNTL_INDEX, R128_PLL_DIV_SEL, 0xffff);
    
    OUTPLLP(pScrn,
	    R128_PPLL_CNTL,
	    R128_PPLL_RESET
	    | R128_PPLL_ATOMIC_UPDATE_EN
	    | R128_PPLL_VGA_ATOMIC_UPDATE_EN,
	    0xffff);

    R128PLLWaitForReadUpdateComplete(pScrn);
    OUTPLLP(pScrn, R128_PPLL_REF_DIV,
	    restore->ppll_ref_div, ~R128_PPLL_REF_DIV_MASK);
    R128PLLWriteUpdate(pScrn);
    
    R128PLLWaitForReadUpdateComplete(pScrn);
    OUTPLLP(pScrn, R128_PPLL_DIV_3,
	    restore->ppll_div_3, ~R128_PPLL_FB3_DIV_MASK);
    R128PLLWriteUpdate(pScrn);
    OUTPLLP(pScrn, R128_PPLL_DIV_3,
	    restore->ppll_div_3, ~R128_PPLL_POST3_DIV_MASK);
    R128PLLWriteUpdate(pScrn);
    
    R128PLLWaitForReadUpdateComplete(pScrn);
    OUTPLL(R128_HTOTAL_CNTL, restore->htotal_cntl);
    R128PLLWriteUpdate(pScrn);

    OUTPLLP(pScrn, R128_PPLL_CNTL, 0, ~R128_PPLL_RESET);
    
    R128TRACE(("Wrote: 0x%08x 0x%08x 0x%08x (0x%08x)\n",
	       restore->ppll_ref_div,
	       restore->ppll_div_3,
	       restore->htotal_cntl,
	       INPLL(pScrn, R128_PPLL_CNTL)));
    R128TRACE(("Wrote: rd=%d, fd=%d, pd=%d\n",
	       restore->ppll_ref_div & R128_PPLL_REF_DIV_MASK,
	       restore->ppll_div_3 & R128_PPLL_FB3_DIV_MASK,
	       (restore->ppll_div_3 & R128_PPLL_POST3_DIV_MASK) >> 16));
}

/* Write DDA registers. */
static void R128RestoreDDARegisters(ScrnInfoPtr pScrn, R128SavePtr restore)
{
    R128MMIO_VARS();
    
    OUTREG(R128_DDA_CONFIG, restore->dda_config);
    OUTREG(R128_DDA_ON_OFF, restore->dda_on_off);
}

/* Write palette data. */
static void R128RestorePalette(ScrnInfoPtr pScrn, R128SavePtr restore)
{
    int i;
    R128MMIO_VARS();

    if (!restore->palette_valid) return;

    OUTPAL_START(0);
    for (i = 0; i < 256; i++) OUTPAL_NEXT_CARD32(restore->palette[i]);
}

/* Write out state to define a new video mode.  */
static void R128RestoreMode(ScrnInfoPtr pScrn, R128SavePtr restore)
{
    R128TRACE(("R128RestoreMode(%p)\n", restore));
    R128RestoreCommonRegisters(pScrn, restore);
    R128RestoreCrtcRegisters(pScrn, restore);
    R128RestorePLLRegisters(pScrn, restore);
    R128RestoreDDARegisters(pScrn, restore);
    R128RestorePalette(pScrn, restore);
}

/* Read common registers. */
static void R128SaveCommonRegisters(ScrnInfoPtr pScrn, R128SavePtr save)
{
    R128MMIO_VARS();
    
    save->ovr_clr            = INREG(R128_OVR_CLR);
    save->ovr_wid_left_right = INREG(R128_OVR_WID_LEFT_RIGHT);
    save->ovr_wid_top_bottom = INREG(R128_OVR_WID_TOP_BOTTOM);
    save->ov0_scale_cntl     = INREG(R128_OV0_SCALE_CNTL);
    save->mpp_tb_config      = INREG(R128_MPP_TB_CONFIG);
    save->mpp_gp_config      = INREG(R128_MPP_GP_CONFIG);
    save->subpic_cntl        = INREG(R128_SUBPIC_CNTL);
    save->viph_control       = INREG(R128_VIPH_CONTROL);
    save->i2c_cntl_1         = INREG(R128_I2C_CNTL_1);
    save->gen_int_cntl       = INREG(R128_GEN_INT_CNTL);
    save->cap0_trig_cntl     = INREG(R128_CAP0_TRIG_CNTL);
    save->cap1_trig_cntl     = INREG(R128_CAP1_TRIG_CNTL);
    save->bus_cntl	     = INREG(R128_BUS_CNTL);
}

/* Read CRTC registers. */
static void R128SaveCrtcRegisters(ScrnInfoPtr pScrn, R128SavePtr save)
{
    R128MMIO_VARS();
    
    save->crtc_gen_cntl        = INREG(R128_CRTC_GEN_CNTL);
    save->crtc_ext_cntl        = INREG(R128_CRTC_EXT_CNTL);
    save->dac_cntl             = INREG(R128_DAC_CNTL);
    save->crtc_h_total_disp    = INREG(R128_CRTC_H_TOTAL_DISP);
    save->crtc_h_sync_strt_wid = INREG(R128_CRTC_H_SYNC_STRT_WID);
    save->crtc_v_total_disp    = INREG(R128_CRTC_V_TOTAL_DISP);
    save->crtc_v_sync_strt_wid = INREG(R128_CRTC_V_SYNC_STRT_WID);
    save->crtc_offset          = INREG(R128_CRTC_OFFSET);
    save->crtc_offset_cntl     = INREG(R128_CRTC_OFFSET_CNTL);
    save->crtc_pitch           = INREG(R128_CRTC_PITCH);
}

/* Read PLL registers. */
static void R128SavePLLRegisters(ScrnInfoPtr pScrn, R128SavePtr save)
{
    save->ppll_ref_div         = INPLL(pScrn, R128_PPLL_REF_DIV);
    save->ppll_div_3           = INPLL(pScrn, R128_PPLL_DIV_3);
    save->htotal_cntl          = INPLL(pScrn, R128_HTOTAL_CNTL);

    R128TRACE(("Read: 0x%08x 0x%08x 0x%08x\n",
	       save->ppll_ref_div,
	       save->ppll_div_3,
	       save->htotal_cntl));
    R128TRACE(("Read: rd=%d, fd=%d, pd=%d\n",
	       save->ppll_ref_div & R128_PPLL_REF_DIV_MASK,
	       save->ppll_div_3 & R128_PPLL_FB3_DIV_MASK,
	       (save->ppll_div_3 & R128_PPLL_POST3_DIV_MASK) >> 16));
}

/* Read DDA registers. */
static void R128SaveDDARegisters(ScrnInfoPtr pScrn, R128SavePtr save)
{
    R128MMIO_VARS();
    
    save->dda_config           = INREG(R128_DDA_CONFIG);
    save->dda_on_off           = INREG(R128_DDA_ON_OFF);
}

/* Read palette data. */
static void R128SavePalette(ScrnInfoPtr pScrn, R128SavePtr save)
{
    int i;
    R128MMIO_VARS();
    
    INPAL_START(0);
    for (i = 0; i < 256; i++) save->palette[i] = INPAL_NEXT();
    save->palette_valid = TRUE;
}

/* Save state that defines current video mode. */
static void R128SaveMode(ScrnInfoPtr pScrn, R128SavePtr save)
{
    R128TRACE(("R128SaveMode(%p)\n", save));

    R128SaveCommonRegisters(pScrn, save);
    R128SaveCrtcRegisters(pScrn, save);
    R128SavePLLRegisters(pScrn, save);
    R128SaveDDARegisters(pScrn, save);
    R128SavePalette(pScrn, save);
    
    R128TRACE(("R128SaveMode returns %p\n", save));
}

/* Save everything needed to restore the original VC state. */
static void R128Save(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info = R128PTR(pScrn);
    R128SavePtr   save = &info->SavedReg;
    vgaHWPtr      hwp  = VGAHWPTR(pScrn);
    R128MMIO_VARS();

    R128TRACE(("R128Save\n"));
    if (info->FBDev) {
	fbdevHWSave(pScrn);
	return;
    }
    vgaHWUnlock(hwp);
    vgaHWSave(pScrn, &hwp->SavedReg, VGA_SR_ALL); /* save mode, fonts, cmap */
    vgaHWLock(hwp);

    R128SaveMode(pScrn, save);
    
    save->dp_datatype      = INREG(R128_DP_DATATYPE);
    save->gen_reset_cntl   = INREG(R128_GEN_RESET_CNTL);
    save->clock_cntl_index = INREG(R128_CLOCK_CNTL_INDEX);
    save->amcgpio_en_reg   = INREG(R128_AMCGPIO_EN_REG);
    save->amcgpio_mask     = INREG(R128_AMCGPIO_MASK);
}

/* Restore the original (text) mode. */
static void R128Restore(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info    = R128PTR(pScrn);
    R128SavePtr   restore = &info->SavedReg;
    vgaHWPtr      hwp     = VGAHWPTR(pScrn);
    R128MMIO_VARS();

    R128TRACE(("R128Restore\n"));
    if (info->FBDev) {
	fbdevHWRestore(pScrn);
	return;
    }

    R128Blank(pScrn);
    OUTREG(R128_AMCGPIO_MASK,     restore->amcgpio_mask);
    OUTREG(R128_AMCGPIO_EN_REG,   restore->amcgpio_en_reg);
    OUTREG(R128_CLOCK_CNTL_INDEX, restore->clock_cntl_index);
    OUTREG(R128_GEN_RESET_CNTL,   restore->gen_reset_cntl);
    OUTREG(R128_DP_DATATYPE,      restore->dp_datatype);
    
    R128RestoreMode(pScrn, restore);
    vgaHWUnlock(hwp);
    vgaHWRestore(pScrn, &hwp->SavedReg, VGA_SR_MODE | VGA_SR_FONTS );
    vgaHWLock(hwp);
    
    R128WaitForVerticalSync(pScrn);
    R128Unblank(pScrn);
}

/* Define common registers for requested video mode. */
static void R128InitCommonRegisters(R128SavePtr save, DisplayModePtr mode,
				    R128InfoPtr info)
{
    save->ovr_clr            = 0;
    save->ovr_wid_left_right = 0;
    save->ovr_wid_top_bottom = 0;
    save->ov0_scale_cntl     = 0;
    save->mpp_tb_config      = 0;
    save->mpp_gp_config      = 0;
    save->subpic_cntl        = 0;
    save->viph_control       = 0;
    save->i2c_cntl_1         = 0;
    save->gen_int_cntl       = 0;
    save->cap0_trig_cntl     = 0;
    save->cap1_trig_cntl     = 0;
    save->bus_cntl	     = info->BusCntl;
    /*
     * If bursts are enabled, turn on discards and aborts
     */
    if (save->bus_cntl & (R128_BUS_WRT_BURST|R128_BUS_READ_BURST))
	save->bus_cntl |= R128_BUS_RD_DISCARD_EN | R128_BUS_RD_ABORT_EN;
}

/* Define CRTC registers for requested video mode. */
static Bool R128InitCrtcRegisters(ScrnInfoPtr pScrn, R128SavePtr save,
				  DisplayModePtr mode, R128InfoPtr info)
{
    int    format;
    int    hsync_start;
    int    hsync_wid;
    int    hsync_fudge;
    int    vsync_wid;
    int    bytpp;
    
    switch (info->pixel_code) {
    case 4:  format = 1; bytpp = 0; hsync_fudge =  0; break;
    case 8:  format = 2; bytpp = 1; hsync_fudge = 18; break;
    case 15: format = 3; bytpp = 2; hsync_fudge =  9; break;	/*  555 */
    case 16: format = 4; bytpp = 2; hsync_fudge =  9; break;	/*  565 */
    case 24: format = 5; bytpp = 3; hsync_fudge =  6; break;	/*  RGB */
    case 32: format = 6; bytpp = 4; hsync_fudge =  5; break;	/* xRGB */
    default:
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Unsupported pixel depth (%d)\n", pScrn->bitsPerPixel);
	return FALSE;
    }
    R128TRACE(("Format = %d (%d bytes per pixel)\n", format, bytpp));
    
    
    save->crtc_gen_cntl = (R128_CRTC_EXT_DISP_EN
			  | R128_CRTC_EN
			  | (format << 8)
			  | ((mode->Flags & V_DBLSCAN)
			     ? R128_CRTC_DBL_SCAN_EN
			     : 0)
			  | ((mode->Flags & V_INTERLACE)
			     ? R128_CRTC_INTERLACE_EN
			     : 0));

    save->crtc_ext_cntl = R128_VGA_ATI_LINEAR | R128_XCRT_CNT_EN;
    save->dac_cntl      = (R128_DAC_MASK_ALL
			   | R128_DAC_VGA_ADR_EN
			   | (info->dac6bits ? 0 : R128_DAC_8BIT_EN));
    
    save->crtc_h_total_disp = ((((mode->CrtcHTotal / 8) - 1) & 0xffff)
			      | (((mode->CrtcHDisplay / 8) - 1) << 16));
    
    hsync_wid = (mode->CrtcHSyncEnd - mode->CrtcHSyncStart) / 8;
    if (!hsync_wid)       hsync_wid = 1;
    if (hsync_wid > 0x3f) hsync_wid = 0x3f;
    
    hsync_start = mode->CrtcHSyncStart - 8 + hsync_fudge;
    
    save->crtc_h_sync_strt_wid = ((hsync_start & 0xfff)
				 | (hsync_wid << 16)
				 | ((mode->Flags & V_NHSYNC)
				    ? R128_CRTC_H_SYNC_POL
				    : 0));

#if 1
				/* This works for double scan mode. */
    save->crtc_v_total_disp = (((mode->CrtcVTotal - 1) & 0xffff)
			      | ((mode->CrtcVDisplay - 1) << 16));
#else
				/* This is what cce/nbmode.c example code
                                   does -- is this correct? */
    save->crtc_v_total_disp = (((mode->CrtcVTotal - 1) & 0xffff)
			      | ((mode->CrtcVDisplay
				  * ((mode->Flags & V_DBLSCAN) ? 2 : 1) - 1)
				 << 16));
#endif

    vsync_wid = mode->CrtcVSyncEnd - mode->CrtcVSyncStart;
    if (!vsync_wid)       vsync_wid = 1;
    if (vsync_wid > 0x1f) vsync_wid = 0x1f;

    save->crtc_v_sync_strt_wid = (((mode->CrtcVSyncStart - 1) & 0xfff)
				 | (vsync_wid << 16)
				 | ((mode->Flags & V_NVSYNC)
				    ? R128_CRTC_V_SYNC_POL
				    : 0));
    save->crtc_offset      = 0;
    save->crtc_offset_cntl = 0;
    save->crtc_pitch       = pScrn->displayWidth / 8;

    R128TRACE(("Pitch = %d bytes (virtualX = %d, displayWidth = %d)\n",
	       save->crtc_pitch, pScrn->virtualX, pScrn->displayWidth));
    return TRUE;
}

/* Define PLL registers for requested video mode. */
static void R128InitPLLRegisters(ScrnInfoPtr pScrn, R128SavePtr save,
				 DisplayModePtr mode, R128PLLPtr pll,
				 double dot_clock)
{
    int freq        = dot_clock * 100;
    struct {
	int divider;
	int bitvalue;
    } *post_div,
      post_divs[]   = {
				/* From RAGE 128 VR/RAGE 128 GL Register
				   Reference Manual (Technical Reference
				   Manual P/N RRG-G04100-C Rev. 0.04), page
				   3-17 (PLL_DIV_[3:0]).  */
	{  1, 0 },		/* VCLK_SRC                 */
	{  2, 1 },		/* VCLK_SRC/2               */
	{  4, 2 },		/* VCLK_SRC/4               */
	{  8, 3 },		/* VCLK_SRC/8               */
	
	{  3, 4 },		/* VCLK_SRC/3               */
				/* bitvalue = 5 is reserved */
	{  6, 6 },		/* VCLK_SRC/6               */
	{ 12, 7 },		/* VCLK_SRC/12              */
	{  0, 0 }
    };
    
    if (freq > pll->max_pll_freq)      freq = pll->max_pll_freq;
    if (freq * 12 < pll->min_pll_freq) freq = pll->min_pll_freq / 12;
    
    for (post_div = &post_divs[0]; post_div->divider; ++post_div) {
	save->pll_output_freq = post_div->divider * freq;
	if (save->pll_output_freq >= pll->min_pll_freq
	    && save->pll_output_freq <= pll->max_pll_freq) break;
    }
    
    save->dot_clock_freq = freq;
    save->feedback_div   = R128Div(pll->reference_div * save->pll_output_freq,
				   pll->reference_freq);
    save->post_div       = post_div->divider;

    R128TRACE(("dc=%d, of=%d, fd=%d, pd=%d\n",
	       save->dot_clock_freq,
	       save->pll_output_freq,
	       save->feedback_div,
	       save->post_div));
    
    save->ppll_ref_div   = pll->reference_div;
    save->ppll_div_3     = (save->feedback_div | (post_div->bitvalue << 16));
    save->htotal_cntl    = 0;
}

/* Define DDA registers for requested video mode. */
static Bool R128InitDDARegisters(ScrnInfoPtr pScrn, R128SavePtr save,
				 DisplayModePtr mode, R128PLLPtr pll,
				 R128InfoPtr info)
{
    int         DisplayFifoWidth = 128;
    int         DisplayFifoDepth = 32;
    int         XclkFreq;
    int         VclkFreq;
    int         XclksPerTransfer;
    int         XclksPerTransferPrecise;
    int         UseablePrecision;
    int         Roff;
    int         Ron;

    XclkFreq = pll->xclk;
    
    VclkFreq = R128Div(pll->reference_freq * save->feedback_div,
		       pll->reference_div * save->post_div);
		
    XclksPerTransfer = R128Div(XclkFreq * DisplayFifoWidth,
			       VclkFreq * (info->pixel_bytes * 8));
    
    UseablePrecision = R128MinBits(XclksPerTransfer) + 1;
    
    XclksPerTransferPrecise = R128Div((XclkFreq * DisplayFifoWidth)
				      << (11 - UseablePrecision),
				      VclkFreq * (info->pixel_bytes * 8));
    
    Roff  = XclksPerTransferPrecise * (DisplayFifoDepth - 4);
    
    Ron   = (4 * info->ram->MB
	     + 3 * MAX(info->ram->Trcd - 2, 0)
	     + 2 * info->ram->Trp
	     + info->ram->Twr
	     + info->ram->CL
	     + info->ram->Tr2w
	     + XclksPerTransfer) << (11 - UseablePrecision);
    
    if (Ron + info->ram->Rloop >= Roff) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "(Ron = %d) + (Rloop = %d) >= (Roff = %d)\n",
		   Ron, info->ram->Rloop, Roff);
	return FALSE;
    }
    
    save->dda_config = (XclksPerTransferPrecise
			| (UseablePrecision << 16)
			| (info->ram->Rloop << 20));
    
    save->dda_on_off = (Ron << 16) | Roff;
    
    R128TRACE(("XclkFreq = %d; VclkFreq = %d; per = %d, %d (useable = %d)\n",
	       XclkFreq,
	       VclkFreq,
	       XclksPerTransfer,
	       XclksPerTransferPrecise,
	       UseablePrecision));
    R128TRACE(("Roff = %d, Ron = %d, Rloop = %d\n",
	       Roff, Ron, info->ram->Rloop));

    return TRUE;
}


/* Define initial palette for requested video mode.  This doesn't do
   anything for XFree86 4.0. */
static void R128InitPalette(R128SavePtr save, R128InfoPtr info)
{
    save->palette_valid = FALSE;
}

/* Define registers for a requested video mode. */
static Bool R128Init(ScrnInfoPtr pScrn, DisplayModePtr mode, R128SavePtr save)
{
    R128InfoPtr info      = R128PTR(pScrn);
    double      dot_clock = mode->Clock/1000.0;

#if R128_DEBUG
    ErrorF("%-12.12s %7.2f  %4d %4d %4d %4d  %4d %4d %4d %4d (%d,%d)",
	   mode->name,
	   dot_clock,
	   
	   mode->HDisplay,
	   mode->HSyncStart,
	   mode->HSyncEnd,
	   mode->HTotal,
	   	 
	   mode->VDisplay,
	   mode->VSyncStart,
	   mode->VSyncEnd,
	   mode->VTotal,
	   pScrn->depth,
	   pScrn->bitsPerPixel);
    if (mode->Flags & V_DBLSCAN)   ErrorF(" D");
    if (mode->Flags & V_INTERLACE) ErrorF(" I");
    if (mode->Flags & V_PHSYNC)    ErrorF(" +H");
    if (mode->Flags & V_NHSYNC)    ErrorF(" -H");
    if (mode->Flags & V_PVSYNC)    ErrorF(" +V");
    if (mode->Flags & V_NVSYNC)    ErrorF(" -V");
    ErrorF("\n");
    ErrorF("%-12.12s %7.2f  %4d %4d %4d %4d  %4d %4d %4d %4d (%d,%d)",
	   mode->name,
	   dot_clock,
	   
	   mode->CrtcHDisplay,
	   mode->CrtcHSyncStart,
	   mode->CrtcHSyncEnd,
	   mode->CrtcHTotal,
	   
	   mode->CrtcVDisplay,
	   mode->CrtcVSyncStart,
	   mode->CrtcVSyncEnd,
	   mode->CrtcVTotal,
	   pScrn->depth,
	   pScrn->bitsPerPixel);
    if (mode->Flags & V_DBLSCAN)   ErrorF(" D");
    if (mode->Flags & V_INTERLACE) ErrorF(" I");
    if (mode->Flags & V_PHSYNC)    ErrorF(" +H");
    if (mode->Flags & V_NHSYNC)    ErrorF(" -H");
    if (mode->Flags & V_PVSYNC)    ErrorF(" +V");
    if (mode->Flags & V_NVSYNC)    ErrorF(" -V");
    ErrorF("\n");
#endif

    info->Flags = mode->Flags;
    
    R128InitCommonRegisters(save, mode, info);
    if (!R128InitCrtcRegisters(pScrn, save, mode, info)) return FALSE;
    R128InitPLLRegisters(pScrn, save, mode, &info->pll, dot_clock);
    if (!R128InitDDARegisters(pScrn, save, mode, &info->pll, info))
	return FALSE;
    R128InitPalette(save, info);
    
    R128TRACE(("R128Init returns %p\n", save));
    return TRUE;
}

/* Initialize a new mode. */
static Bool R128ModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    R128InfoPtr info      = R128PTR(pScrn);
    
    if (!R128Init(pScrn, mode, &info->ModeReg)) return FALSE;
    R128Blank(pScrn);
    R128RestoreMode(pScrn, &info->ModeReg);
    R128Unblank(pScrn);
    return TRUE;
}

static Bool R128SaveScreen(ScreenPtr pScreen, int mode)
{
    ScrnInfoPtr   pScrn = xf86Screens[pScreen->myNum];
    Bool unblank;

    unblank = xf86IsUnblank(mode);

    if (unblank) R128Unblank(pScrn); else R128Blank(pScrn);
    return TRUE;
}

static Bool R128SwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return R128ModeInit(xf86Screens[scrnIndex], mode);
}

/* Used to disallow modes that are not supported by the hardware. */
static int R128ValidMode(int scrnIndex, DisplayModePtr mode,
			 Bool verbose, int flag)
{
    return MODE_OK;
}

/* Adjust viewport into virtual desktop such that (0,0) in viewport space
   is (x,y) in virtual space. */
static void R128AdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr   pScrn = xf86Screens[scrnIndex];
    R128InfoPtr   info  = R128PTR(pScrn);
    int           Base;
    R128MMIO_VARS();

    Base = y * pScrn->displayWidth + x;

    switch (info->pixel_code) {
    case 15:
    case 16: Base *= 2; break;
    case 24: Base *= 3; break;
    case 32: Base *= 4; break;
    }

    Base &= ~7;			/* 3 lower bits are always 0 */

    if (info->pixel_code == 24)
	Base += 8 * (Base % 3); /* Must be multiple of 8 and 3 */

    OUTREG(R128_CRTC_OFFSET, Base);
}

/* Called when VT switching back to the X server.  Reinitialize the video
   mode. */
static Bool R128EnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    R128TRACE(("R128EnterVT\n"));
    if (!R128ModeInit(pScrn, pScrn->currentMode)) return FALSE;
    R128AdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    return TRUE;
}

static Bool
R128EnterVTFBDev(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    R128InfoPtr info = R128PTR(pScrn);
    R128SavePtr restore = &info->SavedReg;
    fbdevHWEnterVT(scrnIndex,flags);
    R128RestorePalette(pScrn,restore); 
    R128EngineInit(pScrn); 
    return TRUE;
}

/* Called when VT switching away from the X server.  Restore the original
   text mode. */
static void R128LeaveVTFBDev(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    R128InfoPtr info = R128PTR(pScrn);
    R128SavePtr save = &info->SavedReg;
    R128SavePalette(pScrn,save); 
    fbdevHWLeaveVT(scrnIndex,flags);
}

static void R128LeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    R128TRACE(("R128LeaveVT\n"));
    R128Restore(pScrn);
}

/* Called at the end of each server generation.  Restore the original text
   mode, unmap video memory, and unwrap and call the saved CloseScreen
   function.  */
static Bool R128CloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    R128InfoPtr info  = R128PTR(pScrn);

    R128TRACE(("R128CloseScreen\n"));
    if (pScrn->vtSema) {
	R128Restore(pScrn);
	R128UnmapMem(pScrn);
    }
    
    if (info->accel)             XAADestroyInfoRec(info->accel);
    info->accel                  = NULL;

    if (info->scratch_buffer[0]) xfree(info->scratch_buffer[0]);
    info->scratch_buffer[0]      = NULL;
    
    if (info->cursor)            xf86DestroyCursorInfoRec(info->cursor);
    info->cursor                 = NULL;
    
    pScrn->vtSema = FALSE;

    pScreen->CloseScreen = info->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}

static void R128FreeScreen(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    
    R128TRACE(("R128FreeScreen\n"));
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
	vgaHWFreeHWRec(pScrn);
    R128FreeRec(pScrn);
}

#ifdef DPMSExtension
/* Sets VESA Display Power Management Signaling (DPMS) Mode.  */
static void R128DisplayPowerManagementSet(ScrnInfoPtr pScrn,
					  int PowerManagementMode, int flags)
{
    int mask = (R128_CRTC_DISPLAY_DIS
		| R128_CRTC_HSYNC_DIS
		| R128_CRTC_VSYNC_DIS);
    R128MMIO_VARS();
    
    switch (PowerManagementMode) {
    case DPMSModeOn:
	/* Screen: On; HSync: On, VSync: On */
	OUTREGP(R128_CRTC_EXT_CNTL, 0, ~mask);
	break;
    case DPMSModeStandby:
	/* Screen: Off; HSync: Off, VSync: On */
	OUTREGP(R128_CRTC_EXT_CNTL,
		R128_CRTC_DISPLAY_DIS | R128_CRTC_HSYNC_DIS, ~mask);
	break;
    case DPMSModeSuspend:
	/* Screen: Off; HSync: On, VSync: Off */
	OUTREGP(R128_CRTC_EXT_CNTL,
		R128_CRTC_DISPLAY_DIS | R128_CRTC_VSYNC_DIS, ~mask);
	break;
    case DPMSModeOff:
	/* Screen: Off; HSync: Off, VSync: Off */
	OUTREGP(R128_CRTC_EXT_CNTL, mask, ~mask);
	break;
    }
}
#endif
