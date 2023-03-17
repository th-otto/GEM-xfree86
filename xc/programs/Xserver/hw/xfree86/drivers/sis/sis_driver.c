/*
 * Copyright 1998,1999 by Alan Hourihane, Wigan, England.
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
 * Authors:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *           Mike Chapman <mike@paranoia.com>, 
 *           Juanjo Santamarta <santamarta@ctv.es>, 
 *           Mitani Hiroshi <hmitani@drl.mei.co.jp> 
 *           David Thomas <davtom@dream.org.uk>. 
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_driver.c,v 1.43 2000/03/03 21:26:18 dawes Exp $ */


#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "cfb24_32.h"
#include "xf1bpp.h"
#include "xf4bpp.h"
#include "mibank.h"
#include "micmap.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "xf86Version.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86cmap.h"
#include "vgaHW.h"
#include "xf86RAC.h"

#include "mipointer.h"

#include "mibstore.h"

#include "sis.h"
#include "sis_regs.h"
#include "sis_bios.h"

#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef DPMSExtension
#include "globals.h"
#define DPMS_SERVER
#include "extensions/dpms.h"
#endif

static void	SISIdentify(int flags);
static Bool	SISProbe(DriverPtr drv, int flags);
static Bool	SISPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	SISScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
static Bool	SISEnterVT(int scrnIndex, int flags);
static void	SISLeaveVT(int scrnIndex, int flags);
static Bool	SISCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	SISSaveScreen(ScreenPtr pScreen, int mode);

/* Required if the driver supports mode switching */
static Bool	SISSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
/* Required if the driver supports moving the viewport */
static void	SISAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
static void	SISFreeScreen(int scrnIndex, int flags);
static int	SISValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);

/* Internally used functions */
static Bool	SISMapMem(ScrnInfoPtr pScrn);
static Bool	SISUnmapMem(ScrnInfoPtr pScrn);
static void	SISSave(ScrnInfoPtr pScrn);
static void	SISRestore(ScrnInfoPtr pScrn);

#ifdef	DEBUG
static void	SiSDumpModeInfo(ScrnInfoPtr pScrn, DisplayModePtr mode);
#endif


/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;
 

/* 
 * This contains the functions needed by the server after loading the driver
 * module.  It must be supplied, and gets passed back by the SetupProc
 * function in the dynamic case.  In the static case, a reference to this
 * is compiled in, and this requires that the name of this DriverRec be
 * an upper-case version of the driver name.
 */

DriverRec SIS = {
    SIS_CURRENT_VERSION,
    "accelerated driver for SiS chipsets",
    SISIdentify,
    SISProbe,
    SISAvailableOptions,
    NULL,
    0
};

static SymTabRec SISChipsets[] = {
#if 0
    { PCI_CHIP_SG86C201,	"SIS86c201" },
    { PCI_CHIP_SG86C202,	"SIS86c202" },
    { PCI_CHIP_SG86C205,	"SIS86c205" },
    { PCI_CHIP_SG86C215,	"SIS86c215" },
    { PCI_CHIP_SG86C225,	"SIS86c225" },
#endif
    { PCI_CHIP_SIS5597, 	"SIS5597" },
    { PCI_CHIP_SIS5597, 	"SIS5598" },
    { PCI_CHIP_SIS530,		"SIS530" },
    { PCI_CHIP_SIS6326,		"SIS6326" },
    { PCI_CHIP_SIS300,		"SIS300" },
    { PCI_CHIP_SIS630,		"SIS630" },
    { PCI_CHIP_SIS540,		"SIS540" },
    { -1,				NULL }
};

static PciChipsets SISPciChipsets[] = {
#if 0
    { PCI_CHIP_SG86C201,	PCI_CHIP_SG86C201,	RES_SHARED_VGA },
    { PCI_CHIP_SG86C202,	PCI_CHIP_SG86C202,	RES_SHARED_VGA },
    { PCI_CHIP_SG86C205,	PCI_CHIP_SG86C205,	RES_SHARED_VGA },
    { PCI_CHIP_SG86C205,	PCI_CHIP_SG86C205,	RES_SHARED_VGA },
#endif
    { PCI_CHIP_SIS5597,		PCI_CHIP_SIS5597,	RES_SHARED_VGA },
    { PCI_CHIP_SIS530,		PCI_CHIP_SIS530,	RES_SHARED_VGA },
    { PCI_CHIP_SIS6326,		PCI_CHIP_SIS6326,	RES_SHARED_VGA },
    { PCI_CHIP_SIS300,		PCI_CHIP_SIS300,	RES_SHARED_VGA },
    { PCI_CHIP_SIS630,		PCI_CHIP_SIS630,	RES_SHARED_VGA },
    { PCI_CHIP_SIS540,		PCI_CHIP_SIS540,	RES_SHARED_VGA },
    { -1,		-1,		RES_UNDEFINED }
};
    


int sisReg32MMIO[]={0x8280,0x8284,0x8288,0x828C,0x8290,0x8294,0x8298,0x829C,
		    0x82A0,0x82A4,0x82A8,0x82AC};
/* Engine Register for the 2nd Generation Graphics Engine */
int sis2Reg32MMIO[]={0x8200,0x8204,0x8208,0x820C,0x8210,0x8214,0x8218,0x821C,
		    0x8220,0x8224,0x8228,0x822C,0x8230,0x8234,0x8238,0x823C,
		     0x8240, 0x8300};

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

static const char *vgahwSymbols[] = {
    "vgaHWGetHWRec",
    "vgaHWUnlock",
    "vgaHWInit",
    "vgaHWSave",
    "vgaHWRestore",
    "vgaHWProtect",
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    NULL
};

static const char *fbSymbols[] = {
    "xf1bppScreenInit",
    "xf4bppScreenInit",
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb24_32ScreenInit",
    "cfb32ScreenInit",
    NULL
};

static const char *ddcSymbols[] = {
    "xf86PrintEDID",
    "xf86DoEDID_DDC1",
    NULL
};

static const char *i2cSymbols[] = {
    "xf86I2CBusInit",
    "xf86CreateI2CBusRec",
    NULL
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(sisSetup);

static XF86ModuleVersionInfo sisVersRec =
{
	"sis",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	SIS_MAJOR_VERSION, SIS_MINOR_VERSION, SIS_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

XF86ModuleData sisModuleData = { &sisVersRec, sisSetup, NULL };

pointer
sisSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&SIS, module, 0);
	LoaderRefSymLists(vgahwSymbols, fbSymbols, i2cSymbols,
			  NULL);
	return (pointer)TRUE;
    } 
ErrorF("sisSetup\n");

    if (errmaj) *errmaj = LDR_ONCEONLY;
    return NULL;
}

#endif /* XFree86LOADER */

static Bool
SISGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate an SISRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(SISRec), 1);
    /* Initialise it */

    return TRUE;
}

static void
SISFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

#ifdef DPMSExtension
static void 
SISDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags)
{
	unsigned char extDDC_PCR;
	unsigned char crtc17 = 0;
	unsigned char seq1 = 0 ;
        int vgaIOBase = VGAHWPTR(pScrn)->IOBase;

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,"SISDisplayPowerManagementSet(%d)\n",PowerManagementMode);
	outb(vgaIOBase + 4, 0x17);
	crtc17 = inb(vgaIOBase + 5);
	outb(VGA_SEQ_INDEX, 0x11);
	extDDC_PCR = inb(VGA_SEQ_DATA) & ~0xC0;
	switch (PowerManagementMode)
	{
	case DPMSModeOn:
	    /* HSync: On, VSync: On */
	    seq1 = 0x00 ;
	    crtc17 |= 0x80;
	    break;
	case DPMSModeStandby:
	    /* HSync: Off, VSync: On */
	    seq1 = 0x20 ;
	    extDDC_PCR |= 0x40;
	    break;
	case DPMSModeSuspend:
	    /* HSync: On, VSync: Off */
	    seq1 = 0x20 ;
	    extDDC_PCR |= 0x80;
	    break;
	case DPMSModeOff:
	    /* HSync: Off, VSync: Off */
	    seq1 = 0x20 ;
	    extDDC_PCR |= 0xC0;
	    /* DPMSModeOff is not supported with ModeStandby | ModeSuspend  */
            /* need same as the generic VGA function */
	    crtc17 &= ~0x80;
	    break;
	}
	outw(VGA_SEQ_INDEX, 0x0100);	/* Synchronous Reset */
	outb(VGA_SEQ_INDEX, 0x01);	/* Select SEQ1 */
	seq1 |= inb(VGA_SEQ_DATA) & ~0x20;
	outb(VGA_SEQ_DATA, seq1);
	usleep(10000);
	outb(vgaIOBase + 4, 0x17);
	outb(vgaIOBase + 5, crtc17);
	outb(VGA_SEQ_INDEX, 0x11);
	outb(VGA_SEQ_DATA, extDDC_PCR);
	outw(VGA_SEQ_INDEX, 0x0300);	/* End Reset */
}
#endif


/* Mandatory */
static void
SISIdentify(int flags)
{
    xf86PrintChipsets(SIS_NAME, "driver for SiS chipsets", SISChipsets);
}

static void
SIS1bppColorMap(ScrnInfoPtr pScrn) {
/* In 1 bpp we have color 0 at LUT 0 and color 1 at LUT 0x3f.
   This makes white and black look right (otherwise they were both
   black. I'm sure there's a better way to do that, just lazy to
   search the docs.  */

   outb(0x3C8, 0x00); outb(0x3C9, 0x00); outb(0x3C9, 0x00); outb(0x3C9, 0x00);
   outb(0x3C8, 0x3F); outb(0x3C9, 0x3F); outb(0x3C9, 0x3F); outb(0x3C9, 0x3F);
}

/* Mandatory */
static Bool
SISProbe(DriverPtr drv, int flags)
{
    int i;
    GDevPtr *devSections;
    int *usedChips;
    int numDevSections;
    int numUsed;
    Bool foundScreen = FALSE;

    /*
     * The aim here is to find all cards that this driver can handle,
     * and for the ones not already claimed by another driver, claim the
     * slot, and allocate a ScrnInfoRec.
     *
     * This should be a minimal probe, and it should under no circumstances
     * change the state of the hardware.  Because a device is found, don't
     * assume that it will be used.  Don't do any initialisations other than
     * the required ScrnInfoRec initialisations.  Don't allocate any new
     * data structures.
     *
     * Since this test version still uses vgaHW, we'll only actually claim
     * one for now, and just print a message about the others.
     */

    /*
     * Next we check, if there has been a chipset override in the config file.
     * For this we must find out if there is an active device section which
     * is relevant, i.e., which has no driver specified or has THIS driver
     * specified.
     */

    if ((numDevSections = xf86MatchDevice(SIS_DRIVER_NAME,
					  &devSections)) <= 0) {
	/*
	 * There's no matching device section in the config file, so quit
	 * now.
	 */
	return FALSE;
    }

    /*
     * While we're VGA-dependent, can really only have one such instance, but
     * we'll ignore that.
     */

    /*
     * We need to probe the hardware first.  We then need to see how this
     * fits in with what is given in the config file, and allow the config
     * file info to override any contradictions.
     */

    /*
     * All of the cards this driver supports are PCI, so the "probing" just
     * amounts to checking the PCI data that the server has already collected.
     */
    if (xf86GetPciVideoInfo() == NULL) {
	/*
	 * We won't let anything in the config file override finding no
	 * PCI video cards at all.  This seems reasonable now, but we'll see.
	 */
	return FALSE;
    }

    numUsed = xf86MatchPciInstances(SIS_NAME, PCI_VENDOR_SIS,
		   SISChipsets, SISPciChipsets, devSections,
		   numDevSections, drv, &usedChips);

    /* Free it since we don't need that list after this */
    if (devSections)
	xfree(devSections);
    devSections = NULL;
    if (numUsed <= 0)
	return FALSE;

    if (flags & PROBE_DETECT)
	foundScreen = TRUE;
    else for (i = 0; i < numUsed; i++) {
	ScrnInfoPtr pScrn;

	/* Allocate a ScrnInfoRec and claim the slot */
	pScrn = xf86AllocateScreen(drv, 0);

	/* Fill in what we can of the ScrnInfoRec */
	pScrn->driverVersion	= SIS_CURRENT_VERSION;
	pScrn->driverName	= SIS_DRIVER_NAME;
	pScrn->name		= SIS_NAME;
	pScrn->Probe		= SISProbe;
	pScrn->PreInit		= SISPreInit;
	pScrn->ScreenInit	= SISScreenInit;
	pScrn->SwitchMode	= SISSwitchMode;
	pScrn->AdjustFrame	= SISAdjustFrame;
	pScrn->EnterVT		= SISEnterVT;
	pScrn->LeaveVT		= SISLeaveVT;
	pScrn->FreeScreen	= SISFreeScreen;
	pScrn->ValidMode	= SISValidMode;
	foundScreen = TRUE;
	xf86ConfigActivePciEntity(pScrn, usedChips[i], SISPciChipsets,
				  NULL, NULL, NULL, NULL, NULL);
    }
    xfree(usedChips);
    return foundScreen;
}

#if 0 /* xf86ValidateModes() takes care of this */
/*
 * GetAccelPitchValues -
 *
 * This function returns a list of display width (pitch) values that can
 * be used in accelerated mode.
 */
static int
GetAccelPitchValues(ScrnInfoPtr pScrn)
{
    return ((pScrn->displayWidth + 7) & ~7);
}
#endif


/* Mandatory */
static Bool
SISPreInit(ScrnInfoPtr pScrn, int flags)
{
    SISPtr pSiS;
    MessageType from;
    int vgaIOBase;
    int i;
    unsigned char unlock;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    const char *Sym = NULL;
    int	pix24flags;

    if (flags & PROBE_DETECT) return FALSE;

    /*
     * Note: This function is only called once at server startup, and
     * not at the start of each server generation.  This means that
     * only things that are persistent across server generations can
     * be initialised here.  xf86Screens[] is (pScrn is a pointer to one
     * of these).  Privates allocated using xf86AllocateScrnInfoPrivateIndex()  
     * are too, and should be used for data that must persist across
     * server generations.
     *
     * Per-generation data should be allocated with
     * AllocateScreenPrivateIndex() from the ScreenInit() function.
     */

    /* Check the number of entities, and fail if it isn't one. */
    if (pScrn->numEntities != 1)
	return FALSE;

    /* The vgahw module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    xf86LoaderReqSymLists(vgahwSymbols, NULL);

    /*
     * Allocate a vgaHWRec
     */
    if (!vgaHWGetHWRec(pScrn))
	return FALSE;

    VGAHWPTR(pScrn)->MapSize = 0x10000;		/* Standard 64k VGA window */

    if (!vgaHWMapMem(pScrn))
	return FALSE;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    /* Allocate the SISRec driverPrivate */
    if (!SISGetRec(pScrn)) {
	return FALSE;
    }
    pSiS = SISPTR(pScrn);
    pSiS->pScrn = pScrn;

    /* Get the entity, and make sure it is PCI. */
    pSiS->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
    if (pSiS->pEnt->location.type != BUS_PCI)
	return FALSE;

    /* Find the PCI info for this screen */
    pSiS->PciInfo = xf86GetPciInfoForEntity(pSiS->pEnt->index);
    pSiS->PciTag = pciTag(pSiS->PciInfo->bus, pSiS->PciInfo->device,
			  pSiS->PciInfo->func);

    /*
     * XXX This could be refined if some VGA memory resources are not
     * decoded in operating mode.
     */
    {
	resRange vgamem[] =	{ {ResShrMemBlock,0xA0000,0xAFFFF},
				  {ResShrMemBlock,0xB0000,0xB7FFF},
				  {ResShrMemBlock,0xB8000,0xBFFFF},
				  _END };
	xf86SetOperatingState(vgamem, pSiS->pEnt->index, ResUnusedOpr);
    }

    /* Operations for which memory access is required */
    pScrn->racMemFlags = RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;
    /* Operations for which I/O access is required (XXX check this) */
    pScrn->racIoFlags = RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;

    /* The ramdac module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "ramdac"))
	return FALSE;

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     * Our default depth is 8, so pass it to the helper function.
     * Our preference for depth 24 is 24bpp, so tell it that too.
     */
    pix24flags = Support32bppFb | Support24bppFb |
		SupportConvert24to32 | SupportConvert32to24;

    if (!xf86SetDepthBpp(pScrn, 8, 8, 8, pix24flags)) {
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 8:
	case 16:
	case 24:
	    /* OK */
	    break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Given depth (%d) is not supported by this driver\n",
		       pScrn->depth);
	    return FALSE;
	}
    }

    xf86PrintDepthBpp(pScrn);

    /* Get the depth24 pixmap format */
    if (pScrn->depth == 24 && pix24bpp == 0)
	pix24bpp = xf86GetBppFromDepth(pScrn, 24);

    /*
     * This must happen after pScrn->display has been set because
     * xf86SetWeight references it.
     */
    if (pScrn->depth > 8) {
	/* The defaults are OK for us */
	rgb zeros = {0, 0, 0};

	if (!xf86SetWeight(pScrn, zeros, zeros)) {
	    return FALSE;
	} else {
	    /* XXX check that weight returned is supported */
            ;
        }
    }

    if (!xf86SetDefaultVisual(pScrn, -1)) {
	return FALSE;
    } else {
	/* We don't currently support DirectColor at > 8bpp */
	if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Given default visual"
		       " (%s) is not supported at depth %d\n",
		       xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
	    return FALSE;
	}
    }

    /*
     * The new cmap layer needs this to be initialised.
     */

    {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	pScrn->rgbBits = 6;
    }


    pSiS->ddc1Read = SiSddc1Read;	/* this cap will be modified */

    from = X_DEFAULT;
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pSiS->pEnt->device->chipset && *pSiS->pEnt->device->chipset)  {
	pScrn->chipset = pSiS->pEnt->device->chipset;
       	pSiS->Chipset = xf86StringToToken(SISChipsets, pScrn->chipset);
       	from = X_CONFIG;
    } else if (pSiS->pEnt->device->chipID >= 0) {
	pSiS->Chipset = pSiS->pEnt->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(SISChipsets, pSiS->Chipset);

	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   		pSiS->Chipset);
    } else {
	from = X_PROBED;
	pSiS->Chipset = pSiS->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(SISChipsets, pSiS->Chipset);
    }
    if (pSiS->pEnt->device->chipRev >= 0) {
	pSiS->ChipRev = pSiS->pEnt->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
	   		pSiS->ChipRev);
    } else {
	pSiS->ChipRev = pSiS->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * SISProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pSiS->Chipset);
	return FALSE;
    }
    if (pSiS->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }


    outb(VGA_SEQ_INDEX, 0x05); unlock = inb(VGA_SEQ_DATA);
    outw(VGA_SEQ_INDEX, 0x8605); /* Unlock registers */

    /* get VBIOS image */
    if (!(pSiS->BIOS=xcalloc(1, BIOS_SIZE)))  {
	ErrorF("Allocate memory fail !!\n");
	return FALSE;
    }
    if (xf86ReadBIOS(BIOS_BASE, 0, pSiS->BIOS, BIOS_SIZE) != BIOS_SIZE)  {
	xfree(pSiS->BIOS);
	ErrorF("Read VBIOS image fail !!\n");
	return FALSE;
    }

    SiSOptions(pScrn);
    SiSSetup(pScrn);

    from = X_PROBED;
    if (pSiS->pEnt->device->MemBase != 0) {
	/*
	 * XXX Should check that the config file value matches one of the
	 * PCI base address values.
	 */
	pSiS->FbAddress = pSiS->pEnt->device->MemBase;
	from = X_CONFIG;
    } else {
	pSiS->FbAddress = pSiS->PciInfo->memBase[0] & 0xFFFFFFF0;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pSiS->FbAddress);

    if (pSiS->pEnt->device->IOBase != 0) {
	/*
	 * XXX Should check that the config file value matches one of the
	 * PCI base address values.
	 */
	pSiS->IOAddress = pSiS->pEnt->device->IOBase;
	from = X_CONFIG;
    } else {
	pSiS->IOAddress = pSiS->PciInfo->memBase[1] & 0xFFFFFFF0;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
	       (unsigned long)pSiS->IOAddress);

    pSiS->RelIO = pSiS->PciInfo->ioBase[2] & 0xFFFC;
    xf86DrvMsg(pScrn->scrnIndex, from, "Relocate IO registers at 0x%lX\n",
	       (unsigned long)pSiS->RelIO);

    /* Register the PCI-assigned resources. */
    if (xf86RegisterResources(pSiS->pEnt->index, NULL, ResExclusive)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "xf86RegisterResources() found resource conflicts\n");
	return FALSE;
    }

    from = X_PROBED;
    if ((pSiS->pEnt->device->videoRam != 0) &&
	(pSiS->pEnt->device->videoRam < pScrn->videoRam))  {
	pScrn->videoRam = pSiS->pEnt->device->videoRam;
	from = X_CONFIG;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);

    pSiS->FbMapSize = pScrn->videoRam * 1024;

    SiSVGASetup(pScrn);
#if 0
    SiSLCDPreInit(pScrn);
    SiSTVPreInit(pScrn);
#endif

    outw(VGA_SEQ_INDEX, (unlock << 8) | 0x05);

    /* Set the min pixel clock */
    pSiS->MinClock = 16250;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	       pSiS->MinClock / 1000);

    from = X_PROBED;
    /*
     * If the user has specified ramdac speed in the XF86Config
     * file, we respect that setting.
     */
    if (pSiS->pEnt->device->dacSpeeds[0]) {
	int speed = 0;

	switch (pScrn->bitsPerPixel) {
	case 8:
	   speed = pSiS->pEnt->device->dacSpeeds[DAC_BPP8];
	   break;
	case 16:
	   speed = pSiS->pEnt->device->dacSpeeds[DAC_BPP16];
	   break;
	case 24:
	   speed = pSiS->pEnt->device->dacSpeeds[DAC_BPP24];
	   break;
	case 32:
	   speed = pSiS->pEnt->device->dacSpeeds[DAC_BPP32];
	   break;
	}
	if (speed == 0)
	    pSiS->MaxClock = pSiS->pEnt->device->dacSpeeds[0];
	else
	    pSiS->MaxClock = speed;
	from = X_CONFIG;
    } 
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pSiS->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pSiS->MinClock;
    clockRanges->maxClock = pSiS->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = TRUE;
    clockRanges->doubleScanAllowed = TRUE;	/* XXX check this */

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our SISValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    /* Select valid modes from those available */
    /*
     * XXX Assuming min pitch 256, max 4096 ==> 8192
     * XXX Assuming min height 128, max 4096
     */
    i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      NULL, 256, 8192,
			      pScrn->bitsPerPixel * 8, 128, 4096,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pSiS->FbMapSize,
			      LOOKUP_BEST_REFRESH);

    if (i == -1) {
	SISFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	SISFreeRec(pScrn);
	return FALSE;
    }

    xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
    case 1:
	mod = "xf1bpp";
	Sym = "xf1bppScreenInit";
	break;
    case 4:
	mod = "xf4bpp";
	Sym = "xf4bppScreenInit";
	break;
    case 8:
	mod = "cfb";
	Sym = "cfbScreenInit";
	break;
    case 16:
	mod = "cfb16";
	Sym = "cfb16ScreenInit";
	break;
    case 24:
	if (pix24bpp == 24) {
	    mod = "cfb24";
	    Sym = "cfb24ScreenInit";
	} else {
	    mod = "xf24_32bpp";
	    Sym = "cfb24_32ScreenInit";
	}
	break;
    case 32:
	mod = "cfb32";
	Sym = "cfb32ScreenInit";
	break;
    }

    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	SISFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymbols(Sym, NULL);

    if (!xf86LoadSubModule(pScrn, "i2c")) {
	SISFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymLists(i2cSymbols, NULL);

    /* Load XAA if needed */
    if (!pSiS->NoAccel) {
	xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Accel Enable\n");
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    SISFreeRec(pScrn);
	    return FALSE;
	}

        xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    /* Load DDC if needed */
    /* This gives us DDC1 - we should be able to get DDC2B using i2c */
    if (!xf86LoadSubModule(pScrn, "ddc")) {
	SISFreeRec(pScrn);
	return FALSE;
    }
    xf86LoaderReqSymLists(ddcSymbols, NULL);

    /* Initialize DDC1 if possible */
    if (pSiS->ddc1Read) 
	xf86PrintEDID(xf86DoEDID_DDC1(pScrn->scrnIndex,vgaHWddc1SetSpeed,pSiS->ddc1Read ) );


    return TRUE;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
SISMapMem(ScrnInfoPtr pScrn)
{
    SISPtr pSiS;
    int mmioFlags;

    pSiS = SISPTR(pScrn);

    /*
     * Map IO registers to virtual address space
     */ 
#if !defined(__alpha__)
    mmioFlags = VIDMEM_MMIO;
#else
    /*
     * For Alpha, we need to map SPARSE memory, since we need
     * byte/short access.
     */
    mmioFlags = VIDMEM_MMIO | VIDMEM_SPARSE;
#endif
    pSiS->IOBase = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, 
			pSiS->PciTag, pSiS->IOAddress, 0x10000);
    if (pSiS->IOBase == NULL)
	return FALSE;

#ifdef __alpha__
    /*
     * for Alpha, we need to map DENSE memory as well, for
     * setting CPUToScreenColorExpandBase.
     */
    pSiS->IOBaseDense = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
		pSiS->PciTag, pSiS->IOAddress, 0x10000);

    if (pSiS->IOBaseDense == NULL)
	return FALSE;
#endif /* __alpha__ */

    pSiS->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pSiS->PciTag,
				 (unsigned long)pSiS->FbAddress,
				 pSiS->FbMapSize);
    if (pSiS->FbBase == NULL)
	return FALSE;

    return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
SISUnmapMem(ScrnInfoPtr pScrn)
{
    SISPtr pSiS;

    pSiS = SISPTR(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pSiS->IOBase, 0x10000);
    pSiS->IOBase = NULL;

#ifdef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pSiS->IOBaseDense, 0x10000);
    pSiS->IOBaseDense = NULL;
#endif /* __alpha__ */

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pSiS->FbBase, pSiS->FbMapSize);
    pSiS->FbBase = NULL;
    return TRUE;
}


/*
 * This function saves the video state.
 */
static void
SISSave(ScrnInfoPtr pScrn)
{
    SISPtr pSiS;
    vgaRegPtr vgaReg;
    SISRegPtr sisReg;

    pSiS = SISPTR(pScrn);
    vgaReg = &VGAHWPTR(pScrn)->SavedReg;
    sisReg = &pSiS->SavedReg;

    vgaHWSave(pScrn, vgaReg, VGA_SR_ALL);

    SiSSave(pScrn, sisReg);
}


/*
 * Restore the initial (text) mode.
 */
static void 
SISRestore(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    SISPtr pSiS;
    SISRegPtr sisReg;
	unsigned char	temp;

    hwp = VGAHWPTR(pScrn);
    pSiS = SISPTR(pScrn);
    vgaReg = &hwp->SavedReg;
    sisReg = &pSiS->SavedReg;

    vgaHWProtect(pScrn, TRUE);

    if (pSiS->VBFlags)  {
	setSISIDXREG(pSiS->RelIO+CROFFSET, 0x30, 0xFC, SET_SIMU_SCAN_MODE);

	inSISIDXREG(pSiS->RelIO+CROFFSET, 0x31, temp);
	temp &= ~((DRIVER_MODE | DISABLE_CRT2_DISPLAY ) >> 8);
	temp |= (SET_IN_SLAVE_MODE >> 8);
	outSISIDXREG(pSiS->RelIO+CROFFSET, 0x31, temp);
    }

    SiSRestore(pScrn, sisReg);

    if (pSiS->VBFlags)  {
	/* for SiS301 */
	DisableBridge(pSiS->RelIO);
	UnLockCRT2(pSiS->RelIO);

	/* SetCRT2ModeRegs() */
	outSISIDXREG(pSiS->RelIO+4, 4, 0);
	outSISIDXREG(pSiS->RelIO+4, 5, 0);
	outSISIDXREG(pSiS->RelIO+4, 6, 0);
	outSISIDXREG(pSiS->RelIO+4, 0, sisReg->VBPart1[0]);
	outSISIDXREG(pSiS->RelIO+4, 1, sisReg->VBPart1[1]);
	outSISIDXREG(pSiS->RelIO+0x14, 0x0D, sisReg->VBPart4[0x0D]);
	outSISIDXREG(pSiS->RelIO+0x14, 0x0C, sisReg->VBPart4[0x0C]);

	SetBlock(pSiS->RelIO+0x04, 2, 0x23, &(sisReg->VBPart1[2]));

	SetBlock(pSiS->RelIO+0x10, 0, 0x45, &(sisReg->VBPart2[0]));

	SetBlock(pSiS->RelIO+0x12, 0, 0x3E, &(sisReg->VBPart3[0]));

	outSISIDXREG(pSiS->RelIO+0x14, 0x0A, 1);
	outSISIDXREG(pSiS->RelIO+0x14, 0x0B, sisReg->VBPart4[0x0B]);
	outSISIDXREG(pSiS->RelIO+0x14, 0x0A, sisReg->VBPart4[0x0A]);
	outSISIDXREG(pSiS->RelIO+0x14, 0x12, 0);
	outSISIDXREG(pSiS->RelIO+0x14, 0x12, sisReg->VBPart4[0x12]);
	SetBlock(pSiS->RelIO+0x14, 0x0E, 0x11, &(sisReg->VBPart4[0x0E]));
	SetBlock(pSiS->RelIO+0x14, 0x13, 0x1B, &(sisReg->VBPart4[0x13]));

	/* SetLockRegs() 
	LongWait(pSiS->RelIO+0x5A);
	outSISIDXREG(pSiS->RelIO+SROFFSET, 0x32, sisReg->sisRegs3C4[0x32]);
*/
	EnableBridge(pSiS->RelIO);

	/* EnableCRT2() 
	outSISIDXREG(pSiS->RelIO+SROFFSET, 0x1E, sisReg->sisRegs3C4[0x1E]);
*/
	LockCRT2(pSiS->RelIO);
	
    }

    vgaHWRestore(pScrn, vgaReg, VGA_SR_ALL);

    vgaHWProtect(pScrn, FALSE);
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
SISScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    /* The vgaHW references will disappear one day */
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    SISPtr pSiS;
    int ret;
    VisualPtr visual;

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    hwp = VGAHWPTR(pScrn);

    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */

    pSiS = SISPTR(pScrn);

    /* Map the VGA memory and get the VGA IO base */
    if (!vgaHWMapMem(pScrn))
	return FALSE;
    vgaHWGetIOBase(hwp);

    /* Map the SIS memory and MMIO areas */
    if (!SISMapMem(pScrn))
	return FALSE;

    /* Save the current state */
    SISSave(pScrn);

    /* Darken the screen for aesthetic reasons and set the viewport */
    SISSaveScreen(pScreen, SCREEN_SAVER_ON);

    /* Initialise the first mode */
    if (!(*pSiS->ModeInit)(pScrn, pScrn->currentMode))
	return FALSE;

    /* Darken the screen for aesthetic reasons and set the viewport */
    SISSaveScreen(pScreen, SCREEN_SAVER_ON);
    SISAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    /*
     * The next step is to setup the screen's visuals, and initialise the
     * framebuffer code.  In cases where the framebuffer's default
     * choices for things like visual layouts and bits per RGB are OK,
     * this may be as simple as calling the framebuffer's ScreenInit()
     * function.  If not, the visuals will need to be setup before calling
     * a fb ScreenInit() function and fixed up after.
     *
     * For most PC hardware at depths >= 8, the defaults that cfb uses
     * are not appropriate.  In this driver, we fixup the visuals after.
     */

    /*
     * Reset visual list.
     */
    miClearVisualTypes();

    /* Setup the visuals we support. */

    /*
     * For bpp > 8, the default visuals are not acceptable because we only
     * support TrueColor and not DirectColor.
     */

    if (pScrn->bitsPerPixel > 8) {
	if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits,
			      pScrn->defaultVisual))
	    return FALSE;
    } else {
	if (!miSetVisualTypes(pScrn->depth, 
			      miGetDefaultVisualMask(pScrn->depth),
			      pScrn->rgbBits, pScrn->defaultVisual))
	    return FALSE;
    }

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */

    switch (pScrn->bitsPerPixel) {
    case 1:
	ret = xf1bppScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 4:
	ret = xf4bppScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 8:
	ret = cfbScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 24:
	if (pix24bpp == 24)
	    ret = cfb24ScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	else
	    ret = cfb24_32ScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in SISScrnInit\n",
		   pScrn->bitsPerPixel);
	    ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;

    xf86SetBlackWhitePixels(pScreen);

    if (pScrn->bitsPerPixel > 8) {
        /* Fixup RGB ordering */
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
	    if ((visual->class | DynamicClass) == DirectColor) {
		visual->offsetRed = pScrn->offset.red;
		visual->offsetGreen = pScrn->offset.green;
		visual->offsetBlue = pScrn->offset.blue;
		visual->redMask = pScrn->mask.red;
		visual->greenMask = pScrn->mask.green;
		visual->blueMask = pScrn->mask.blue;
	    }
	}
    } else if (pScrn->depth == 1) {
	SIS1bppColorMap(pScrn);
    }

    if (!pSiS->NoAccel) {
        if ( pSiS->Chipset == PCI_CHIP_SIS300  ||
	     pSiS->Chipset == PCI_CHIP_SIS630 ||
	     pSiS->Chipset == PCI_CHIP_SIS540)
	    SiS300AccelInit(pScreen);
	else if (pSiS->Chipset == PCI_CHIP_SIS530)
	    SiS530AccelInit(pScreen);
	else
	    SiSAccelInit(pScreen);
    }
    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);

    /* Initialise cursor functions */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

    if (pSiS->HWCursor)
	SiSHWCursorInit(pScreen);

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    if (!vgaHWHandleColormaps(pScreen))
	return FALSE;

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, (DPMSSetProcPtr)SISDisplayPowerManagementSet, 0);
#endif

    pSiS->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = SISCloseScreen;
    pScreen->SaveScreen = SISSaveScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    /* Turn on the screen now */
    SISSaveScreen(pScreen, SCREEN_SAVER_OFF);

    return TRUE;
}


/* Usually mandatory */
static Bool
SISSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
	ScrnInfoPtr	pScrn = xf86Screens[scrnIndex];

	return (*SISPTR(pScrn)->ModeInit)(pScrn, mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
static void 
SISAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    SISPtr pSiS;
    vgaHWPtr hwp;
    int base = y * pScrn->displayWidth + x;
    int vgaIOBase;
    unsigned char SR5State, temp;

    hwp = VGAHWPTR(pScrn);
    pSiS = SISPTR(pScrn);
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    outb(VGA_SEQ_INDEX, 0x05); /* Unlock Registers */
    SR5State = inb(VGA_SEQ_DATA);
    outw(VGA_SEQ_INDEX, 0x8605);

    if (pScrn->bitsPerPixel < 8) {
	base = (y * pScrn->displayWidth + x + 3) >> 3;
    } else {
	base = y * pScrn->displayWidth + x ;
	/* calculate base bpp dep. */
	switch (pScrn->bitsPerPixel) {
	  case 16:
	    base >>= 1;
	    break;
	  case 24:
	    base = ((base * 3)) >> 2;
	    base -= base % 6;
	    break;
	  case 32:
	    break;
	  default:	/* 8bpp */
	    base >>= 2;
	    break;
    	}
    }

    outw(vgaIOBase + 4, (base & 0x00FF00) | 0x0C);
    outw(vgaIOBase + 4, ((base & 0x00FF) << 8) | 0x0D);
    switch (pSiS->Chipset)  {
	case PCI_CHIP_SIS300:
	case PCI_CHIP_SIS630:
	case PCI_CHIP_SIS540:
	    outb(VGA_SEQ_INDEX, 0x0D);
	    temp = (base & 0xFF0000) >> 16;
	    PDEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
			"3C5/0Dh set to hex %2X, base 0x%x\n", temp, base));
	    outb(VGA_SEQ_DATA, temp);
	    break;
	default:
	    outb(VGA_SEQ_INDEX, 0x27);
	    temp = inb(VGA_SEQ_DATA) & 0xF0;
	    temp |= (base & 0x0F0000) >> 16;
	    PDEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
			"3C5/27h set to hex %2X, base %d\n",  temp, base));
	    outb(VGA_SEQ_DATA, temp);
    }

    outw(VGA_SEQ_INDEX, (SR5State << 8) | 0x05); /* Relock Registers */
}

/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
SISEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    /* Should we re-save the text mode on each VT enter? */
    if (!(*SISPTR(pScrn)->ModeInit)(pScrn, pScrn->currentMode))
	return FALSE;

    SISAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    return TRUE;
}


/*
 * This is called when VT switching away from the X server.  Its job is
 * to restore the previous (text) mode.
 *
 * We may wish to remap video/MMIO memory too.
 */

/* Mandatory */
static void
SISLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    SISRestore(pScrn);
    vgaHWLock(hwp);
}


/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should really also unmap the video memory too.
 */

/* Mandatory */
static Bool
SISCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    SISPtr pSiS = SISPTR(pScrn);

    pSiS->CursorInfoPtr->HideCursor(pScrn);

    if (pScrn->vtSema) {
    	SISRestore(pScrn);
    	vgaHWLock(hwp);
    	SISUnmapMem(pScrn);
    }
    if(pSiS->AccelInfoPtr)
	XAADestroyInfoRec(pSiS->AccelInfoPtr);
    if(pSiS->CursorInfoPtr)
	xf86DestroyCursorInfoRec(pSiS->CursorInfoPtr);
    pScrn->vtSema = FALSE;
    
    pScreen->CloseScreen = pSiS->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any per-generation data structures */

/* Optional */
static void
SISFreeScreen(int scrnIndex, int flags)
{
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
	vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    SISFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
SISValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    return(MODE_OK);
}

/* Do screen blanking */

/* Mandatory */
static Bool
SISSaveScreen(ScreenPtr pScreen, int mode)
{
    return vgaHWSaveScreen(pScreen, mode);
}

#ifdef	DEBUG
/* local used for debug */
static void
SiSDumpModeInfo(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Clock : %x\n", mode->Clock);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Display : %x\n", mode->CrtcHDisplay);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Blank Start : %x\n", mode->CrtcHBlankStart);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Sync Start : %x\n", mode->CrtcHSyncStart);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Sync End : %x\n", mode->CrtcHSyncEnd);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Blank End : %x\n", mode->CrtcHBlankEnd);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Total : %x\n", mode->CrtcHTotal);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Skew : %x\n", mode->CrtcHSkew);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz HAdjusted : %x\n", mode->CrtcHAdjusted);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Display : %x\n", mode->CrtcVDisplay);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Blank Start : %x\n", mode->CrtcVBlankStart);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Sync Start : %x\n", mode->CrtcVSyncStart);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Sync End : %x\n", mode->CrtcVSyncEnd);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Blank End : %x\n", mode->CrtcVBlankEnd);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Total : %x\n", mode->CrtcVTotal);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt VAdjusted : %x\n", mode->CrtcVAdjusted);

/*
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Display : %x\n", mode->HDisplay);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Sync Start : %x\n", mode->HSyncStart);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Sync End : %x\n", mode->HSyncEnd);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Total : %x\n", mode->HTotal);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Hz Skew : %x\n", mode->HSkew);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Display : %x\n", mode->VDisplay);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Sync Start : %x\n", mode->VSyncStart);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Sync End : %x\n", mode->VSyncEnd);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Total : %x\n", mode->VTotal);
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Vt Scan : %x\n", mode->VScan);
*/
}
#endif

