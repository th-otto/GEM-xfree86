/* $XConsortium: mga_driver.c /main/12 1996/10/28 05:13:26 kaleb $ */
/*
 * MGA Millennium (MGA2064W) with Ti3026 RAMDAC driver v.1.1
 *
 * The driver is written without any chip documentation. All extended ports
 * and registers come from tracing the VESA-ROM functions.
 * The BitBlt Engine comes from tracing the windows BitBlt function.
 *
 * Author:	Radoslaw Kapitan, Tarnow, Poland
 *			kapitan@student.uci.agh.edu.pl
 *		original source
 *
 * Now that MATROX has released documentation to the public, enhancing
 * this driver has become much easier. Nevertheless, this work continues
 * to be based on Radoslaw's original source
 *
 * Contributors:
 *		Andrew van der Stock
 *			ajv@greebo.net
 *		additions, corrections, cleanups
 *
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		integrated into XFree86-3.1.2Gg
 *		fixed some problems with PCI probing and mapping
 *
 *		David Dawes
 *			dawes@XFree86.Org
 *		some cleanups, and fixed some problems
 *
 *		Andrew E. Mileski
 *			aem@ott.hookup.net
 *		RAMDAC timing, and BIOS stuff
 *
 *		Leonard N. Zubkoff
 *			lnz@dandelion.com
 *		Support for 8MB boards, RGB Sync-on-Green, and DPMS.
 *		Guy DESBIEF
 *			g.desbief@aix.pacwan.net
 *		RAMDAC MGA1064 timing,
 *		Doug Merritt
 *			doug@netcom.com
 *		Fixed 32bpp hires 8MB horizontal line glitch at middle right
 */
 
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_driver.c,v 1.149 2000/03/07 01:37:49 dawes Exp $ */

/*
 * This is a first cut at a non-accelerated version to work with the
 * new server design (DHD).
 */


/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"

/* All drivers need this */
#include "xf86_ansic.h"

#include "compiler.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers implementing backing store need this */
#include "mibstore.h"

#include "micmap.h"

#include "xf86DDC.h"
#include "xf86RAC.h"
#include "vbe.h"


/*
 * If using cfb, cfb.h is required.  Select the others for the bpp values
 * the driver supports.
 */
#define PSZ 8	/* needed for cfb.h */
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "cfb24_32.h"
#include "cfb8_32.h"

#include "mga_bios.h"
#include "mga_reg.h"
#include "mga.h"
#include "mga_macros.h"

#include "xaa.h"
#include "xf86cmap.h"
#include "shadowfb.h"
#include "fbdevhw.h"

#ifdef XF86DRI 
#include "dri.h"
#endif


/*
 * Forward definitions for the functions that make up the driver.
 */

/* Mandatory functions */
static OptionInfoPtr	MGAAvailableOptions(int chipid, int busid);
static void	MGAIdentify(int flags);
static Bool	MGAProbe(DriverPtr drv, int flags);
static Bool	MGAPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	MGAScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
static Bool	MGAEnterVT(int scrnIndex, int flags);
static Bool	MGAEnterVTFBDev(int scrnIndex, int flags);
static void	MGALeaveVT(int scrnIndex, int flags);
static Bool	MGACloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	MGASaveScreen(ScreenPtr pScreen, int mode);

/* This shouldn't be needed since RAC will disable all I/O for MGA cards. */
#ifdef DISABLE_VGA_IO
static void     VgaIOSave(int i, void *arg);
static void     VgaIORestore(int i, void *arg);
#endif

/* Optional functions */
static void	MGAFreeScreen(int scrnIndex, int flags);
static int	MGAValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);
#ifdef DPMSExtension
static void	MGADisplayPowerManagementSet(ScrnInfoPtr pScrn,
					     int PowerManagementMode,
					     int flags);
#endif

/* Internally used functions */
static Bool	MGAMapMem(ScrnInfoPtr pScrn);
static Bool	MGAUnmapMem(ScrnInfoPtr pScrn);
static void	MGASave(ScrnInfoPtr pScrn);
static void	MGARestore(ScrnInfoPtr pScrn);
static Bool	MGAModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

#define VERSION 4000
#define MGA_NAME "MGA"
#define MGA_DRIVER_NAME "mga"
#define MGA_MAJOR_VERSION 1
#define MGA_MINOR_VERSION 0
#define MGA_PATCHLEVEL 0

/* 
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

DriverRec MGA = {
    VERSION,
    MGA_DRIVER_NAME,
#if 0
    "accelerated driver for Matrox Millennium and Mystique cards",
#endif
    MGAIdentify,
    MGAProbe,
    MGAAvailableOptions,
    NULL,
    0
};

/* Supported chipsets */
static SymTabRec MGAChipsets[] = {
    { PCI_CHIP_MGA2064,		"mga2064w" },
    { PCI_CHIP_MGA1064,		"mga1064sg" },
    { PCI_CHIP_MGA2164,		"mga2164w" },
    { PCI_CHIP_MGA2164_AGP,	"mga2164w AGP" },
    { PCI_CHIP_MGAG100,		"mgag100" },
    { PCI_CHIP_MGAG200,		"mgag200" },
    { PCI_CHIP_MGAG200_PCI,	"mgag200 PCI" },
    { PCI_CHIP_MGAG400,		"mgag400" },
    {-1,			NULL }
};

static PciChipsets MGAPciChipsets[] = {
    { PCI_CHIP_MGA2064,		PCI_CHIP_MGA2064,	RES_SHARED_VGA },
    { PCI_CHIP_MGA1064,		PCI_CHIP_MGA1064,	RES_SHARED_VGA },
    { PCI_CHIP_MGA2164,		PCI_CHIP_MGA2164,	RES_SHARED_VGA },
    { PCI_CHIP_MGA2164_AGP,	PCI_CHIP_MGA2164_AGP,	RES_SHARED_VGA },
    { PCI_CHIP_MGAG100,		PCI_CHIP_MGAG100,	RES_SHARED_VGA },
    { PCI_CHIP_MGAG200,		PCI_CHIP_MGAG200,	RES_SHARED_VGA },
    { PCI_CHIP_MGAG200_PCI,	PCI_CHIP_MGAG200_PCI,	RES_SHARED_VGA },
    { PCI_CHIP_MGAG400,		PCI_CHIP_MGAG400,	RES_SHARED_VGA },
    { -1,			-1,			RES_UNDEFINED }
};

typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_SYNC_ON_GREEN,
    OPTION_NOACCEL,
    OPTION_SHOWCACHE,
    OPTION_OVERLAY,
    OPTION_MGA_SDRAM,
    OPTION_SHADOW_FB,
    OPTION_FBDEV,
    OPTION_COLOR_KEY,
    OPTION_SET_MCLK,
    OPTION_OVERCLOCK_MEM,
    OPTION_VIDEO_KEY,
    OPTION_ROTATE
} MGAOpts;

static OptionInfoRec MGAOptions[] = {
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_RETRY,		"PciRetry",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SYNC_ON_GREEN,	"SyncOnGreen",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHOWCACHE,		"ShowCache",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_OVERLAY,		"Overlay",	OPTV_ANYSTR,	{0}, FALSE },
    { OPTION_MGA_SDRAM,		"MGASDRAM",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHADOW_FB,		"ShadowFB",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_FBDEV,		"UseFBDev",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_COLOR_KEY,		"ColorKey",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_SET_MCLK,		"SetMclk",	OPTV_FREQ,	{0}, FALSE },
    { OPTION_OVERCLOCK_MEM,	"OverclockMem",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_VIDEO_KEY,		"VideoKey",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_ROTATE,		"Rotate",	OPTV_ANYSTR,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};


/*
 * List of symbols from other modules that this module references.  This
 * list is used to tell the loader that it is OK for symbols here to be
 * unresolved providing that it hasn't been told that they haven't been
 * told that they are essential via a call to xf86LoaderReqSymbols() or
 * xf86LoaderReqSymLists().  The purpose is this is to avoid warnings about
 * unresolved symbols that are not required.
 */

static const char *vgahwSymbols[] = {
    "vgaHWGetHWRec",
    "vgaHWUnlock",
    "vgaHWInit",
    "vgaHWProtect",
    "vgaHWSetMmioFuncs",
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    "vgaHWddc1SetSpeed",
    NULL
};

static const char *cfbSymbols[] = {
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb32ScreenInit",
    "cfb8_32ScreenInit",
    "cfb24_32ScreenInit",
    NULL
};

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

#ifdef XF86DRI 
static const char *drmSymbols[] = {
    "drmAddBufs",
    "drmAddMap",
    "drmAvailable",
    "drmCtlAddCommand",
    "drmCtlInstHandler",
    "drmGetInterruptFromBusID",
    "drmMapBufs",
    "drmMarkBufs",
    "drmUnmapBufs",
    "drmAgpAcquire",
    "drmAgpRelease",
    "drmAgpEnable",
    "drmAgpAlloc",
    "drmAgpFree",
    "drmAgpBind",
    "drmAgpUnbind",
    "drmAgpVersionMajor",
    "drmAgpVersionMinor",
    "drmAgpGetMode",
    "drmAgpBase",
    "drmAgpSize",
    "drmAgpMemoryUsed",
    "drmAgpMemoryAvail",
    "drmAgpVendorId",
    "drmAgpDeviceId",
    NULL
};

static const char *driSymbols[] = {
    "DRIGetDrawableIndex",
    "DRIFinishScreenInit",
    "DRIDestroyInfoRec",
    "DRICloseScreen",
    "DRIDestroyInfoRec",
    "DRIScreenInit",
    "DRIDestroyInfoRec",
    "DRICreateInfoRec",
    "DRILock",
    "DRIUnlock",
    "DRIGetSAREAPrivate",
    "DRIGetContext",
    "GlxSetVisualConfigs",
    NULL
};
#endif

#define MGAuseI2C 1

static const char *ddcSymbols[] = {
    "xf86PrintEDID",
    "xf86DoEDID_DDC1",
#if MGAuseI2C
    "xf86DoEDID_DDC2",
#endif
    NULL
};

static const char *i2cSymbols[] = {
    "xf86CreateI2CBusRec",
    "xf86I2CBusInit",
    NULL
};

static const char *shadowSymbols[] = {
    "ShadowFBInit",
    NULL
};

static const char *vbeSymbols[] = {
    "VBEInit",
    "vbeDoEDID",
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


#ifdef XFree86LOADER

static MODULESETUPPROTO(mgaSetup);

static XF86ModuleVersionInfo mgaVersRec =
{
	"mga",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	MGA_MAJOR_VERSION, MGA_MINOR_VERSION, MGA_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

XF86ModuleData mgaModuleData = { &mgaVersRec, mgaSetup, NULL };

static pointer
mgaSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    /* This module should be loaded only once, but check to be sure. */

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&MGA, module, 0);

	/*
	 * Modules that this driver always requires may be loaded here
	 * by calling LoadSubModule().
	 */

	/*
	 * Tell the loader about symbols from other modules that this module
	 * might refer to.
	 */
	LoaderRefSymLists(vgahwSymbols, cfbSymbols, xaaSymbols, 
			  xf8_32bppSymbols, ramdacSymbols,
			  ddcSymbols, i2cSymbols, shadowSymbols,
			  fbdevHWSymbols, vbeSymbols,
#ifdef XF86DRI 
			  drmSymbols, driSymbols,
#endif
			  
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


#endif /* XFree86LOADER */

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;

/* 
 * ramdac info structure initialization
 */
static MGARamdacRec DacInit = {
	FALSE, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL,
	90000, /* maxPixelClock */
	0, X_DEFAULT, X_DEFAULT, FALSE
}; 

static Bool
MGAGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate an MGARec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(MGARec), 1);
    /* Initialise it */

    MGAPTR(pScrn)->Dac = DacInit;
    return TRUE;
}

static void
MGAFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

static
OptionInfoPtr
MGAAvailableOptions(int chipid, int busid)
{
    return MGAOptions;
}

/* Mandatory */
static void
MGAIdentify(int flags)
{
    xf86PrintChipsets(MGA_NAME, "driver for Matrox chipsets", MGAChipsets);
}


/* Mandatory */
static Bool
MGAProbe(DriverPtr drv, int flags)
{
    int i;
    GDevPtr *devSections = NULL;
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
     */

    /*
     * Check if there has been a chipset override in the config file.
     * For this we must find out if there is an active device section which
     * is relevant, i.e., which has no driver specified or has THIS driver
     * specified.
     */

    if ((numDevSections = xf86MatchDevice(MGA_DRIVER_NAME,
					  &devSections)) <= 0) {
	/*
	 * There's no matching device section in the config file, so quit
	 * now.
	 */
	return FALSE;
    }

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

    numUsed = xf86MatchPciInstances(MGA_NAME, PCI_VENDOR_MATROX,
			MGAChipsets, MGAPciChipsets, devSections,
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
#ifdef DISABLE_VGA_IO
	MgaSavePtr smga;
#endif
	
	/* Allocate a ScrnInfoRec and claim the slot */
	pScrn = xf86AllocateScreen(drv, 0);
	
	/* Fill in what we can of the ScrnInfoRec */
	pScrn->driverVersion	= VERSION;
	pScrn->driverName	= MGA_DRIVER_NAME;
	pScrn->name		= MGA_NAME;
	pScrn->Probe		= MGAProbe;
	pScrn->PreInit		= MGAPreInit;
	pScrn->ScreenInit	= MGAScreenInit;
	pScrn->SwitchMode	= MGASwitchMode;
	pScrn->AdjustFrame	= MGAAdjustFrame;
	pScrn->EnterVT		= MGAEnterVT;
	pScrn->LeaveVT		= MGALeaveVT;
	pScrn->FreeScreen	= MGAFreeScreen;
	pScrn->ValidMode	= MGAValidMode;
	foundScreen = TRUE;
#ifndef DISABLE_VGA_IO
	xf86ConfigActivePciEntity(pScrn, usedChips[i], MGAPciChipsets, NULL,
				      NULL, NULL, NULL, NULL);
#else
	smga = xnfalloc(sizeof(MgaSave));
	smga->pvp = xf86GetPciInfoForEntity(usedChips[i]);
	xf86ConfigActivePciEntity(pScrn, usedChips[i], MGAPciChipsets, NULL,
				  VgaIOSave, VgaIOSave, VgaIORestore,
				  smga);
#endif
    }
    xfree(usedChips);
    return foundScreen;
}


/*
 * Should aim towards not relying on this.
 */

/*
 * MGAReadBios - Read the video BIOS info block.
 *
 * DESCRIPTION
 *   Warning! This code currently does not detect a video BIOS.
 *   In the future, support for motherboards with the mga2064w
 *   will be added (no video BIOS) - this is not a huge concern
 *   for me today though.  (XXX)
 *
 * EXTERNAL REFERENCES
 *   vga256InfoRec.BIOSbase	IN	Physical address of video BIOS.
 *   MGABios			OUT	The video BIOS info block.
 *
 * HISTORY
 *   August  31, 1997 - [ajv] Andrew van der Stock
 *   Fixed to understand Mystique and Millennium II
 * 
 *   January 11, 1997 - [aem] Andrew E. Mileski
 *   Set default values for GCLK (= MCLK / pre-scale ).
 *
 *   October 7, 1996 - [aem] Andrew E. Mileski
 *   Written and tested.
 */ 

static void
MGAReadBios(ScrnInfoPtr pScrn)
{
	CARD8 tmp[ 64 ];
	CARD16 offset;
	CARD8	chksum;
	CARD8	*pPINSInfo; 
	MGAPtr pMga;
	MGABiosInfo *pBios;
	MGABios2Info *pBios2;
	Bool pciBIOS = TRUE;
	
	pMga = MGAPTR(pScrn);
	pBios = &pMga->Bios;
	pBios2 = &pMga->Bios2;

	/*
	 * If the BIOS address was probed, it was found from the PCI config
	 * space.  If it was given in the config file, try to guess when it
	 * looks like it might be controlled by the PCI config space.
	 */
	if (pMga->BiosFrom == X_DEFAULT)
	    pciBIOS = FALSE;
	else if (pMga->BiosFrom == X_CONFIG && pMga->BiosAddress < 0x100000)
	    pciBIOS = TRUE;

#define MGADoBIOSRead(offset, buf, len) \
    (pciBIOS ? \
      xf86ReadPciBIOS(offset, pMga->PciTag, pMga->FbBaseReg, buf, len) : \
      xf86ReadBIOS(pMga->BiosAddress, offset, buf, len))
	
	MGADoBIOSRead(0, tmp, sizeof( tmp ));
	if (
		tmp[ 0 ] != 0x55
		|| tmp[ 1 ] != 0xaa
		|| strncmp(( char * )( tmp + 45 ), "MATROX", 6 )
	) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			       "Video BIOS info block not detected!\n");
		return;
	}

	/* Get the info block offset */
	MGADoBIOSRead(0x7ffc, ( CARD8 * ) & offset, sizeof( offset ));


	/* Let the world know what we are up to */
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Video BIOS info block at offset 0x%05lX\n",
		   (long)(offset));

	/* Copy the info block */
	switch (pMga->Chipset){
	   case PCI_CHIP_MGA2064:
		MGADoBIOSRead(offset,
			( CARD8 * ) & pBios->StructLen, sizeof( MGABiosInfo ));
		break;
	   default:
		MGADoBIOSRead(offset,
			( CARD8 * ) & pBios2->PinID, sizeof( MGABios2Info ));
	}

	
	/* matrox millennium-2 and mystique pins info */
	if ( pBios2->PinID == 0x412e ) {	
	    int i;
	    /* check that the pins info is correct */
	    if ( pBios2->StructLen != 0x40 ) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			"Video BIOS info block not detected!\n");
		pBios2->PinID = 0;
		return;
	    }
	    /* check that the chksum is correct */
	    chksum = 0;
	    pPINSInfo = (CARD8 *) &pBios2->PinID;

	    for (i=0; i < pBios2->StructLen; i++) {
		chksum += *pPINSInfo;
		pPINSInfo++;
	    }

	    if ( chksum ) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			"Video BIOS info block did not checksum!\n");
		pBios2->PinID = 0;
		return;
	    }

	    /* last check */
	    if ( pBios2->StructRev == 0 ) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		  "Video BIOS info block does not have a valid revision!\n");
		pBios2->PinID = 0;
		return;
	    }

	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		"Found and verified enhanced Video BIOS info block\n");

	   /* Set default MCLK values (scaled by 100 kHz) */
	   if ( pBios2->ClkMem == 0 )
		pBios2->ClkMem = 50;
	   if ( pBios2->Clk4MB == 0 )
		pBios2->Clk4MB = pBios->ClkBase;
	   if ( pBios2->Clk8MB == 0 )
		pBios2->Clk8MB = pBios->Clk4MB;
	   pBios->StructLen = 0; /* not in use */
#ifdef DEBUG
	   for (i = 0; i < 0x40; i++)
	      ErrorF("Pins[0x%02x] is 0x%02x\n", i,
			((unsigned char *)pBios2)[i]);
#endif
	   return;
	} else {
	  /* Set default MCLK values (scaled by 10 kHz) */
	  if ( pBios->ClkBase == 0 )
		pBios->ClkBase = 4500;
  	  if ( pBios->Clk4MB == 0 )
		pBios->Clk4MB = pBios->ClkBase;
	  if ( pBios->Clk8MB == 0 )
		pBios->Clk8MB = pBios->Clk4MB;
	  pBios2->PinID = 0; /* not in use */
	  return;
	}
}

/*
 * MGASoftReset --
 *
 * Resets drawing engine
 */
static void
MGASoftReset(ScrnInfoPtr pScrn)
{
	MGAPtr pMga = MGAPTR(pScrn);
	int i;

	pMga->FbMapSize = 8192 * 1024;
	MGAMapMem(pScrn);

	/* set soft reset bit */
	OUTREG(MGAREG_Reset, 1);
	usleep(200);
	OUTREG(MGAREG_Reset, 0);

	/* reset memory */
	OUTREG(MGAREG_MACCESS, 1<<15);
	usleep(10);

#if 0
	/* This will hang if the PLLs aren't on */

	/* wait until drawing engine is ready */
	while ( MGAISBUSY() )
	    usleep(1000);
		
	/* flush FIFO */	
	i = 32;
	WAITFIFO(i);
	while ( i-- )
	    OUTREG(MGAREG_SHIFT, 0);
#endif

	MGAUnmapMem(pScrn);
}

/*
 * MGACountRAM --
 *
 * Counts amount of installed RAM 
 */
static int
MGACountRam(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);
    int ProbeSize = 8192;
    int SizeFound = 2048;
    CARD32 biosInfo = 0;

#if 0
    /* This isn't correct. It looks like this can have arbitrary
	data for the memconfig even when the bios has initialized
	it.  At least, my cards don't advertise the documented 
	values (my 8 and 16 Meg G200s have the same values) */
    if(pMga->Primary) /* can only trust this for primary cards */
	biosInfo = pciReadLong(pMga->PciTag, PCI_OPTION_REG);
#endif

    switch(pMga->Chipset) {
    case PCI_CHIP_MGA2164:
    case PCI_CHIP_MGA2164_AGP:
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
		"Unable to probe memory amount due to hardware bug.  "
		"Assuming 4096 KB\n");
	return 4096;
    case PCI_CHIP_MGAG400:
	if(biosInfo) {
	    switch((biosInfo >> 10) & 0x07) {
	    case 0:
		return (biosInfo & (1 << 14)) ? 32768 : 16384;
	    case 1:
	    case 2:	    
		return 16384;
	    case 3:	    
	    case 5:	    
		return 65536;
	    case 4:	   
		return 32768;
	    }
	}
	ProbeSize = 32768;
	break;
    case PCI_CHIP_MGAG200:
    case PCI_CHIP_MGAG200_PCI:
	if(biosInfo) {
	    switch((biosInfo >> 11) & 0x03) {
	    case 0:
		return 8192;
	    default:
		return 16384;
	    }
	}
	ProbeSize = 16384;
	break;
    case PCI_CHIP_MGAG100:
	if(biosInfo) /* I'm not sure if the docs are correct */
	    return (biosInfo & (1 << 12)) ? 16384 : 8192;
    case PCI_CHIP_MGA1064:
    case PCI_CHIP_MGA2064:
	ProbeSize = 8192;
        break;
    default:
        break;
    }

    if (pMga->FbAddress) {
	volatile unsigned char* base;
	unsigned char tmp;
	int i;
	
	pMga->FbMapSize = ProbeSize * 1024;
	MGAMapMem(pScrn);
	base = pMga->FbBase;

	/* turn MGA mode on - enable linear frame buffer (CRTCEXT3) */
	OUTREG8(0x1FDE, 3);
	tmp = INREG8(0x1FDF);
	OUTREG8(0x1FDF, tmp | 0x80);
	
	/* write, read and compare method */
	for(i = ProbeSize; i > 2048; i -= 2048) {
	    base[(i * 1024) - 1] = 0xAA;
	    OUTREG8(MGAREG_CRTC_INDEX, 0);  /* flush the cache */
	    if(base[(i * 1024) - 1] == 0xAA) {
		SizeFound = i;
		break;
	    }
	}
	
	/* restore CRTCEXT3 state */
	OUTREG8(0x1FDE, 3);
	OUTREG8(0x1FDF, tmp);
	
	MGAUnmapMem(pScrn);
   }
   return SizeFound;
}

static xf86MonPtr
MGAdoDDC(ScrnInfoPtr pScrn)
{
  vgaHWPtr hwp;
  MGAPtr pMga;
  MGARamdacPtr MGAdac;
  xf86MonPtr MonInfo = NULL;

  hwp = VGAHWPTR(pScrn);
  pMga = MGAPTR(pScrn);
  MGAdac = &pMga->Dac;

  /* Map the MGA memory and MMIO areas */
  if (!MGAMapMem(pScrn))
    return NULL;

  /* Initialise the MMIO vgahw functions */
  vgaHWSetMmioFuncs(hwp, pMga->IOBase, PORT_OFFSET);
  vgaHWGetIOBase(hwp);

  /* Map the VGA memory when the primary video */
  if (pMga->Primary) {
    hwp->MapSize = 0x10000;
    if (!vgaHWMapMem(pScrn))
      return NULL;
  } else {
    /* XXX Need to write an MGA mode ddc1SetSpeed */
    if (pMga->DDC1SetSpeed == vgaHWddc1SetSpeed) {
      pMga->DDC1SetSpeed = NULL;
      xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
		     "DDC1 disabled - chip not in VGA mode\n");
    }
  } 

  /* Save the current state */
  MGASave(pScrn);

  /* It is now safe to talk to the card */

#if MGAuseI2C
  /* Initialize I2C bus - used by DDC if available */
  if (pMga->i2cInit) {
    pMga->i2cInit(pScrn);
  }
  /* Read and output monitor info using DDC2 over I2C bus */
  if (pMga->I2C) {
    MonInfo = xf86DoEDID_DDC2(pScrn->scrnIndex,pMga->I2C);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "I2C Monitor info: %p\n", MonInfo);
    xf86PrintEDID(MonInfo);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "end of I2C Monitor info\n\n");
  }
  if (!MonInfo)
#endif /* MGAuseI2C */
  /* Read and output monitor info using DDC1 */
  if (pMga->ddc1Read && pMga->DDC1SetSpeed) {
    MonInfo = xf86DoEDID_DDC1(pScrn->scrnIndex,
					 pMga->DDC1SetSpeed,
					 pMga->ddc1Read ) ;
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "DDC Monitor info: %p\n", MonInfo);
    xf86PrintEDID( MonInfo );
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "end of DDC Monitor info\n\n");
  }


  /* Restore previous state and unmap MGA memory and MMIO areas */
  MGARestore(pScrn);
  MGAUnmapMem(pScrn);
  /* Unmap vga memory if we mapped it */
  if (xf86IsPrimaryPci(pMga->PciInfo) && !pMga->FBDev) {
    vgaHWUnmapMem(pScrn);
  }

  xf86SetDDCproperties(pScrn, MonInfo);

  return MonInfo;
}

#ifdef DISABLE_VGA_IO
static void
VgaIOSave(int i, void *arg)
{
    MgaSavePtr sMga = arg;
    PCITAG tag = pciTag(sMga->pvp->bus,sMga->pvp->device,sMga->pvp->func);

#ifdef DEBUG
    ErrorF("mga: VgaIOSave: %d:%d:%d\n", sMga->pvp->bus, sMga->pvp->device,
	   sMga->pvp->func);    
#endif
    sMga->enable = (pciReadLong(tag, PCI_OPTION_REG) & 0x100) != 0;
}

static void
VgaIORestore(int i, void *arg)
{
    MgaSavePtr sMga = arg;
    PCITAG tag = pciTag(sMga->pvp->bus,sMga->pvp->device,sMga->pvp->func);

#ifdef DEBUG
    ErrorF("mga: VgaIORestore: %d:%d:%d\n", sMga->pvp->bus, sMga->pvp->device,
	   sMga->pvp->func);
#endif
    pciSetBitsLong(tag, PCI_OPTION_REG, 0x100, sMga->enable ? 0x100 : 0x000);
}

static void
VgaIODisable(void *arg)
{
    MGAPtr pMga = arg;

#ifdef DEBUG
    ErrorF("mga: VgaIODisable: %d:%d:%d, %s, xf86ResAccessEnter is %s\n",
	   pMga->PciInfo->bus, pMga->PciInfo->device, pMga->PciInfo->func,
	   pMga->Primary ? "primary" : "secondary",
	   BOOLTOSTRING(xf86ResAccessEnter));
#endif
    /* Turn off the vgaioen bit. */
    pciSetBitsLong(pMga->PciTag, PCI_OPTION_REG, 0x100, 0x000);
}

static void
VgaIOEnable(void *arg)
{
    MGAPtr pMga = arg;

#ifdef DEBUG
    ErrorF("mga: VgaIOEnable: %d:%d:%d, %s, xf86ResAccessEnter is %s\n",
	   pMga->PciInfo->bus, pMga->PciInfo->device, pMga->PciInfo->func,
	   pMga->Primary ? "primary" : "secondary",
	   BOOLTOSTRING(xf86ResAccessEnter));
#endif
    /* Turn on the vgaioen bit. */
    if (pMga->Primary)
	pciSetBitsLong(pMga->PciTag, PCI_OPTION_REG, 0x100, 0x100);
}
#endif /* DISABLE_VGA_IO */

extern xf86MonPtr ConfiguredMonitor;

void
MGAProbeDDC(ScrnInfoPtr pScrn, int index)
{
    vbeInfoPtr pVbe;
    if (xf86LoadSubModule(pScrn, "vbe")) {
	pVbe = VBEInit(NULL,index);
	ConfiguredMonitor = vbeDoEDID(pVbe);
    }
}

/* Mandatory */
static Bool
MGAPreInit(ScrnInfoPtr pScrn, int flags)
{
    MGAPtr pMga;
    MessageType from;
    int i;
    double real;
    int bytesPerPixel;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    const char *reqSym = NULL;
    const char *s;
    int flags24;

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

    /* Allocate the MGARec driverPrivate */
    if (!MGAGetRec(pScrn)) {
	return FALSE;
    }

    pMga = MGAPTR(pScrn);

    /* Get the entity, and make sure it is PCI. */
    pMga->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
    if (pMga->pEnt->location.type != BUS_PCI)
	return FALSE;

    if (flags & PROBE_DETECT) {
	MGAProbeDDC(pScrn, pMga->pEnt->index);
	return TRUE;
    }

    /* The vgahw module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    xf86LoaderReqSymLists(vgahwSymbols, NULL);

    /*
     * Allocate a vgaHWRec
     */
    if (!vgaHWGetHWRec(pScrn))
	return FALSE;

#if 0
    /* This is causing problems with restoring the card to it's
       original state.  If this is to be done, it needs to happen
       after we've saved the original state */
    /* Initialize the card through int10 interface if needed */
    if ( xf86LoadSubModule(pScrn, "int10")){
        xf86Int10InfoPtr pInt;

        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Initializing int10\n");
        pInt = xf86InitInt10(pMga->pEnt->index);
        xf86FreeInt10(pInt);
    }
#endif

    /* Find the PCI info for this screen */
    pMga->PciInfo = xf86GetPciInfoForEntity(pMga->pEnt->index);
    pMga->PciTag = pciTag(pMga->PciInfo->bus, pMga->PciInfo->device,
			  pMga->PciInfo->func);

    pMga->Primary = xf86IsPrimaryPci(pMga->PciInfo);

#ifndef DISABLE_VGA_IO
    {
 	resRange vgaio[] =	{ {ResShrIoBlock,0x3B0,0x3BB},
 				  {ResShrIoBlock,0x3C0,0x3DF},
 				  _END };
 	resRange vga1mem[] =	{ {ResShrMemBlock,0xA0000,0xAFFFF},
 				  {ResShrMemBlock,0xB8000,0xBFFFF},
 				  _END };
 	resRange vga2mem[] =	{ {ResShrMemBlock,0xB0000,0xB7FFF},
 				  _END };
 	xf86SetOperatingState(vgaio, pMga->pEnt->index, ResUnusedOpr);
 	xf86SetOperatingState(vga1mem, pMga->pEnt->index, ResDisableOpr);
 	xf86SetOperatingState(vga2mem, pMga->pEnt->index, ResDisableOpr);
    }
#else
    /*
     * Set our own access functions, which control the vgaioen bit.
     */
    pMga->Access.AccessDisable = VgaIODisable;
    pMga->Access.AccessEnable = VgaIOEnable;
    pMga->Access.arg = pMga;
    /* please check if this is correct. I've impiled that the VGA fb
       is handled locally and not visible outside. If the VGA fb is
       handeled by the same function the third argument has to be set,
       too.*/
    xf86SetAccessFuncs(pMga->pEnt, &pMga->Access, &pMga->Access,
			&pMga->Access, NULL);
#endif
  
    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     * Our default depth is 8, so pass it to the helper function.
     * We support both 24bpp and 32bpp layouts, so indicate that.
     */

    /* Prefer 24bpp fb unless the Overlay option is set */
    flags24 = Support24bppFb | Support32bppFb | SupportConvert32to24;
    s = xf86TokenToOptName(MGAOptions, OPTION_OVERLAY);
    if (!(xf86FindOption(pScrn->confScreen->options, s) ||
	  xf86FindOption(pMga->pEnt->device->options, s))) {
	flags24 |= PreferConvert32to24;
    }

    if (!xf86SetDepthBpp(pScrn, 8, 8, 8, flags24)) {
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 8:
	case 15:
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

    if (!xf86SetDefaultVisual(pScrn, -1))
	return FALSE;

    bytesPerPixel = pScrn->bitsPerPixel / 8;

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, MGAOptions);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) 
	pScrn->rgbBits = 8;


    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pMga->pEnt->device->chipset && *pMga->pEnt->device->chipset) {
	pScrn->chipset = pMga->pEnt->device->chipset;
        pMga->Chipset = xf86StringToToken(MGAChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pMga->pEnt->device->chipID >= 0) {
	pMga->Chipset = pMga->pEnt->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(MGAChipsets, pMga->Chipset);
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pMga->Chipset);
    } else {
	from = X_PROBED;
	pMga->Chipset = pMga->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(MGAChipsets, pMga->Chipset);
    }
    if (pMga->pEnt->device->chipRev >= 0) {
	pMga->ChipRev = pMga->pEnt->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pMga->ChipRev);
    } else {
	pMga->ChipRev = pMga->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * MGAProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pMga->Chipset);
	return FALSE;
    }
    if (pMga->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);


    from = X_DEFAULT;
    pMga->HWCursor = TRUE;
    /*
     * The preferred method is to use the "hw cursor" option as a tri-state
     * option, with the default set above.
     */
    if (xf86GetOptValBool(MGAOptions, OPTION_HW_CURSOR, &pMga->HWCursor)) {
	from = X_CONFIG;
    }
    /* For compatibility, accept this too (as an override) */
    if (xf86ReturnOptValBool(MGAOptions, OPTION_SW_CURSOR, FALSE)) {
	from = X_CONFIG;
	pMga->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pMga->HWCursor ? "HW" : "SW");
    if (xf86ReturnOptValBool(MGAOptions, OPTION_NOACCEL, FALSE)) {
	pMga->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    if (xf86ReturnOptValBool(MGAOptions, OPTION_PCI_RETRY, FALSE)) {
	pMga->UsePCIRetry = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry enabled\n");
    }
    if (xf86ReturnOptValBool(MGAOptions, OPTION_SYNC_ON_GREEN, FALSE)) {
	pMga->SyncOnGreen = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Sync-on-Green enabled\n");
    }
    if (xf86ReturnOptValBool(MGAOptions, OPTION_SHOWCACHE, FALSE)) {
	pMga->ShowCache = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ShowCache enabled\n");
    }
    if (xf86ReturnOptValBool(MGAOptions, OPTION_MGA_SDRAM, FALSE)) {
	pMga->HasSDRAM = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Has SDRAM\n");
    }
    if (xf86GetOptValFreq(MGAOptions, OPTION_SET_MCLK, OPTUNITS_MHZ, &real)) {
	pMga->MemClk = (int)(real * 1000.0);
    }
    if ((s = xf86GetOptValString(MGAOptions, OPTION_OVERLAY))) {
      if (!*s || !xf86NameCmp(s, "8,24") || !xf86NameCmp(s, "24,8")) {
	if(pScrn->bitsPerPixel == 32) {
	    pMga->Overlay8Plus24 = TRUE;
	    if(!xf86GetOptValInteger(
			MGAOptions, OPTION_COLOR_KEY,&(pMga->colorKey)))
		pMga->colorKey = TRANSPARENCY_KEY;
	    pScrn->colorKey = pMga->colorKey;	    
	    pScrn->overlayFlags = OVERLAY_8_32_PLANAR;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
				"PseudoColor overlay enabled\n");
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Option \"Overlay\" is only supported in 32 bits per pixel\n");
	}
      } else {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"\"%s\" is not a valid value for Option \"Overlay\"\n", s);
      }
    }
    if(xf86GetOptValInteger(MGAOptions, OPTION_VIDEO_KEY, &(pMga->videoKey))) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "video key set to 0x%x\n",
				pMga->videoKey);
    } else {
	pMga->videoKey =  (1 << pScrn->offset.red) | 
			  (1 << pScrn->offset.green) |
        (((pScrn->mask.blue >> pScrn->offset.blue) - 1) << pScrn->offset.blue); 
    }
    if (xf86ReturnOptValBool(MGAOptions, OPTION_SHADOW_FB, FALSE)) {
	pMga->ShadowFB = TRUE;
	pMga->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Using \"Shadow Framebuffer\" - acceleration disabled\n");
    }
    if (xf86ReturnOptValBool(MGAOptions, OPTION_FBDEV, FALSE)) {
	pMga->FBDev = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Using framebuffer device\n");
    }
    if (xf86ReturnOptValBool(MGAOptions, OPTION_OVERCLOCK_MEM, FALSE)) {
	pMga->OverclockMem = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Overclocking memory\n");
    }
    if (pMga->FBDev) {
	/* check for linux framebuffer device */
	if (!xf86LoadSubModule(pScrn, "fbdevhw"))
	    return FALSE;
	xf86LoaderReqSymLists(fbdevHWSymbols, NULL);
	if (!fbdevHWInit(pScrn, pMga->PciInfo, NULL))
	    return FALSE;
	pScrn->SwitchMode    = fbdevHWSwitchMode;
	pScrn->AdjustFrame   = fbdevHWAdjustFrame;
	pScrn->EnterVT       = MGAEnterVTFBDev;
	pScrn->LeaveVT       = fbdevHWLeaveVT;
	pScrn->ValidMode     = fbdevHWValidMode;
    }
    pMga->Rotate = 0;
    if ((s = xf86GetOptValString(MGAOptions, OPTION_ROTATE))) {
      if(!xf86NameCmp(s, "CW")) {
	pMga->ShadowFB = TRUE;
	pMga->NoAccel = TRUE;
	pMga->HWCursor = FALSE;
	pMga->Rotate = 1;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Rotating screen clockwise - acceleration disabled\n");
      } else
      if(!xf86NameCmp(s, "CCW")) {
	pMga->ShadowFB = TRUE;
	pMga->NoAccel = TRUE;
	pMga->HWCursor = FALSE;
	pMga->Rotate = -1;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Rotating screen counter clockwise - acceleration disabled\n");
      } else {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"\"%s\" is not a valid value for Option \"Rotate\"\n", s);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		"Valid options are \"CW\" or \"CCW\"\n");
      }
    }

    if(pMga->HasSDRAM) { /* don't bother checking */ }
    else if ((pMga->PciInfo->subsysCard == PCI_CARD_MILL_G200_SD) ||
	(pMga->PciInfo->subsysCard == PCI_CARD_MARV_G200_SD) ||
	(pMga->PciInfo->subsysCard == PCI_CARD_MYST_G200_SD) ||
	(pMga->PciInfo->subsysCard == PCI_CARD_PROD_G100_SD)) {
        pMga->HasSDRAM = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Has SDRAM\n");
    } 
    else if (pMga->Primary && (pMga->Chipset != PCI_CHIP_MGA2064) && 
		(pMga->Chipset != PCI_CHIP_MGA2164) &&
		(pMga->Chipset != PCI_CHIP_MGA2164_AGP)) {	
	if(!(pciReadLong(pMga->PciTag, PCI_OPTION_REG) & (1 << 14))) {
	    pMga->HasSDRAM = TRUE;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Has SDRAM\n");
	}
    }

    switch (pMga->Chipset) {
    case PCI_CHIP_MGA2064:
    case PCI_CHIP_MGA2164:
    case PCI_CHIP_MGA2164_AGP:
	MGA2064SetupFuncs(pScrn);
	break;
    case PCI_CHIP_MGA1064:
    case PCI_CHIP_MGAG100:
    case PCI_CHIP_MGAG200:
    case PCI_CHIP_MGAG200_PCI:
    case PCI_CHIP_MGAG400:
	MGAGSetupFuncs(pScrn);
	break;
    }

    /* ajv changes to reflect actual values. see sdk pp 3-2. */
    /* these masks just get rid of the crap in the lower bits */

    /*
     * For the 2064 and older rev 1064, base0 is the MMIO and base0 is
     * the framebuffer is base1.  Let the config file override these.
     */
    if (pMga->pEnt->device->MemBase != 0) {
	/* Require that the config file value matches one of the PCI values. */
	if (!xf86CheckPciMemBase(pMga->PciInfo, pMga->pEnt->device->MemBase)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"MemBase 0x%08lX doesn't match any PCI base register.\n",
		pMga->pEnt->device->MemBase);
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
	pMga->FbAddress = pMga->pEnt->device->MemBase;
	from = X_CONFIG;
    } else {
	/* details: mgabase2 sdk pp 4-12 */
	int i = ((pMga->Chipset == PCI_CHIP_MGA1064 && pMga->ChipRev < 3) ||
		    pMga->Chipset == PCI_CHIP_MGA2064) ? 1 : 0;
	pMga->FbBaseReg = i;
	if (pMga->PciInfo->memBase[i] != 0) {
	    pMga->FbAddress = pMga->PciInfo->memBase[i] & 0xff800000;
	    from = X_PROBED;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "No valid FB address in PCI config space\n");
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pMga->FbAddress);

    if (pMga->pEnt->device->IOBase != 0) {
	/* Require that the config file value matches one of the PCI values. */
	if (!xf86CheckPciMemBase(pMga->PciInfo, pMga->pEnt->device->IOBase)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"IOBase 0x%08lX doesn't match any PCI base register.\n",
		pMga->pEnt->device->IOBase);
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
	pMga->IOAddress = pMga->pEnt->device->IOBase;
	from = X_CONFIG;
    } else {
	/* details: mgabase1 sdk pp 4-11 */
	int i = ((pMga->Chipset == PCI_CHIP_MGA1064 && pMga->ChipRev < 3) ||
		    pMga->Chipset == PCI_CHIP_MGA2064) ? 0 : 1;
	if (pMga->PciInfo->memBase[i] != 0) {
	    pMga->IOAddress = pMga->PciInfo->memBase[i] & 0xffffc000;
	    from = X_PROBED;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"No valid MMIO address in PCI config space\n");
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
	       (unsigned long)pMga->IOAddress);

     
    pMga->ILOADAddress = 0;
    if ( pMga->Chipset != PCI_CHIP_MGA2064 ) {
	    if (pMga->PciInfo->memBase[2] != 0) {
	    	pMga->ILOADAddress = pMga->PciInfo->memBase[2] & 0xffffc000;
	        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, 
			"Pseudo-DMA transfer window at 0x%lX\n",
	       		(unsigned long)pMga->ILOADAddress);
	    } 
    }


    /*
     * Find the BIOS base.  Get it from the PCI config if possible.  Otherwise
     * use the VGA default.  Allow the config file to override this.
     */

    pMga->BiosFrom = X_NONE;
    if (pMga->pEnt->device->BiosBase != 0) {
	/* XXX This isn't used */
	pMga->BiosAddress = pMga->pEnt->device->BiosBase;
	pMga->BiosFrom = X_CONFIG;
    } else {
	/* details: rombase sdk pp 4-15 */
	if (pMga->PciInfo->biosBase != 0) {
	    pMga->BiosAddress = pMga->PciInfo->biosBase & 0xffff0000;
	    pMga->BiosFrom = X_PROBED;
	} else if (pMga->Primary) {
	    pMga->BiosAddress = 0xc0000;
	    pMga->BiosFrom = X_DEFAULT;
	}
    }
    if (pMga->BiosAddress) {
	xf86DrvMsg(pScrn->scrnIndex, pMga->BiosFrom, "BIOS at 0x%lX\n",
		   (unsigned long)pMga->BiosAddress);
    }

    if (xf86RegisterResources(pMga->pEnt->index, NULL, ResExclusive)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"xf86RegisterResources() found resource conflicts\n");
	MGAFreeRec(pScrn);
	return FALSE;
    }

    /*
     * Read the BIOS data struct
     */

    MGAReadBios(pScrn);

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, 
		   "MGABios.RamdacType = 0x%x\n", pMga->Bios.RamdacType);

    /* HW bpp matches reported bpp */
    pMga->HwBpp = pScrn->bitsPerPixel;

    /*
     * Reset card if it isn't primary one
     */
    if ( (!pMga->Primary && !pMga->FBDev) || xf86IsPc98() )
        MGASoftReset(pScrn);

    /*
     * If the user has specified the amount of memory in the XF86Config
     * file, we respect that setting.
     */
    from = X_PROBED;
    if (pMga->pEnt->device->videoRam != 0) {
	pScrn->videoRam = pMga->pEnt->device->videoRam;
	from = X_CONFIG;
    } else if (pMga->FBDev) {
	pScrn->videoRam = fbdevHWGetVidmem(pScrn)/1024;
    } else {
	pScrn->videoRam = MGACountRam(pScrn);
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
	
    pMga->FbMapSize = pScrn->videoRam * 1024;

    /* Set the bpp shift value */
    pMga->BppShifts[0] = 0;
    pMga->BppShifts[1] = 1;
    pMga->BppShifts[2] = 0;
    pMga->BppShifts[3] = 2;

    /*
     * fill MGAdac struct
     * Warning: currently, it should be after RAM counting
     */
    (*pMga->PreInit)(pScrn);

    /* Load DDC if we have the code to use it */
    /* This gives us DDC1 */
    if (pMga->ddc1Read || pMga->i2cInit) {
	if (xf86LoadSubModule(pScrn, "ddc")) {
	  xf86LoaderReqSymLists(ddcSymbols, NULL);
	} else {
	  /* ddc module not found, we can do without it */
	  pMga->ddc1Read = NULL;

	  /* Without DDC, we have no use for the I2C bus */
	  pMga->i2cInit = NULL;
	}
    }
#if MGAuseI2C    
    /* - DDC can use I2C bus */
    /* Load I2C if we have the code to use it */
    if (pMga->i2cInit) {
      if ( xf86LoadSubModule(pScrn, "i2c") ) {
	xf86LoaderReqSymLists(i2cSymbols,NULL);
      } else {
	/* i2c module not found, we can do without it */
	pMga->i2cInit = NULL;
	pMga->I2C = NULL;
      }
    }
#endif /* MGAuseI2C */

    /* Read and print the Monitor DDC info */
    pScrn->monitor->DDC = MGAdoDDC(pScrn);

    /*
     * If the driver can do gamma correction, it should call xf86SetGamma()
     * here.
     */

    {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }


    /* XXX Set HW cursor use */

    /* Set the min pixel clock */
    pMga->MinClock = 12000;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	       pMga->MinClock / 1000);
    /*
     * If the user has specified ramdac speed in the XF86Config
     * file, we respect that setting.
     */
    if (pMga->pEnt->device->dacSpeeds[0]) {
	int speed = 0;

	switch (pScrn->bitsPerPixel) {
	case 8:
	   speed = pMga->pEnt->device->dacSpeeds[DAC_BPP8];
	   break;
	case 16:
	   speed = pMga->pEnt->device->dacSpeeds[DAC_BPP16];
	   break;
	case 24:
	   speed = pMga->pEnt->device->dacSpeeds[DAC_BPP24];
	   break;
	case 32:
	   speed = pMga->pEnt->device->dacSpeeds[DAC_BPP32];
	   break;
	}
	if (speed == 0)
	    pMga->MaxClock = pMga->pEnt->device->dacSpeeds[0];
	else
	    pMga->MaxClock = speed;
	from = X_CONFIG;
    } else {
	pMga->MaxClock = pMga->Dac.maxPixelClock;
	from = pMga->Dac.ClockFrom;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pMga->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pMga->MinClock;
    clockRanges->maxClock = pMga->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = TRUE;
    clockRanges->doubleScanAllowed = TRUE;

    /* Only set MemClk if appropriate for the ramdac */
    if (pMga->Dac.SetMemClk) {
	if (pMga->MemClk == 0) {
	    pMga->MemClk = pMga->Dac.MemoryClock;
	    from = pMga->Dac.MemClkFrom;
	} else
	    from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, from, "MCLK used is %.1f MHz\n",
		   pMga->MemClk / 1000.0);
    }

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our MGAValidMode() already takes
     * care of this, we don't worry about setting them here.
     */
    {
	int Pitches1[] = 
	  {640, 768, 800, 960, 1024, 1152, 1280, 1600, 1920, 2048, 0};
	int Pitches2[] = 
	  {512, 640, 768, 800, 832, 960, 1024, 1152, 1280, 1600, 1664, 
		1920, 2048, 0};
	int *linePitches = NULL;
	int minPitch = 256;
	int maxPitch = 2048;	
        
        switch(pMga->Chipset) {
	case PCI_CHIP_MGA2064:
	   if (!pMga->NoAccel) {
		linePitches = xalloc(sizeof(Pitches1));
		memcpy(linePitches, Pitches1, sizeof(Pitches1));
		minPitch = maxPitch = 0;
	   }
	   break;
	case PCI_CHIP_MGA2164:
	case PCI_CHIP_MGA2164_AGP:
	case PCI_CHIP_MGA1064:
	   if (!pMga->NoAccel) {
		linePitches = xalloc(sizeof(Pitches2));
		memcpy(linePitches, Pitches2, sizeof(Pitches2));
		minPitch = maxPitch = 0;
	   }
	   break;
	case PCI_CHIP_MGAG100:
	   maxPitch = 2048;
	   break;
	case PCI_CHIP_MGAG200:
	case PCI_CHIP_MGAG200_PCI:
	case PCI_CHIP_MGAG400:
	   maxPitch = 4096;
	   break;
	}

	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      linePitches, minPitch, maxPitch,
			      pMga->Roundings[(pScrn->bitsPerPixel >> 3) - 1] * 
					pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pMga->FbMapSize,
			      LOOKUP_BEST_REFRESH);

	if (linePitches)
	   xfree(linePitches);
    }


    if (i < 1 && pMga->FBDev) {
	fbdevHWUseBuildinMode(pScrn);
	pScrn->displayWidth = pScrn->virtualX; /* FIXME: might be wrong */
	i = 1;
    }
    if (i == -1) {
	MGAFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	MGAFreeRec(pScrn);
	return FALSE;
    }

    /*
     * Set the CRTC parameters for all of the modes based on the type
     * of mode, and the chipset's interlace requirements.
     *
     * Calling this is required if the mode->Crtc* values are used by the
     * driver and if the driver doesn't provide code to set them.  They
     * are not pre-initialised at all.
     */
    xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /*
     * Compute the byte offset into the linear frame buffer where the
     * frame buffer data should actually begin.  According to DDK misc.c
     * line 1023, if more than 4MB is to be displayed, YDSTORG must be set
     * appropriately to align memory bank switching, and this requires a
     * corresponding offset on linear frame buffer access.
     * This is only needed for WRAM.
     */

    pMga->YDstOrg = 0;
    if (((pMga->Chipset == PCI_CHIP_MGA2064) || 
	 (pMga->Chipset == PCI_CHIP_MGA2164) ||
	 (pMga->Chipset == PCI_CHIP_MGA2164_AGP)) &&
	(pScrn->virtualX * pScrn->virtualY * bytesPerPixel > 4*1024*1024)) 
    {
	int offset, offset_modulo, ydstorg_modulo;

	offset = (4*1024*1024) % (pScrn->displayWidth * bytesPerPixel);
	offset_modulo = 4;
	ydstorg_modulo = 64;
	if (pScrn->bitsPerPixel == 24)
	    offset_modulo *= 3;
	if (pMga->Interleave)
	{
	    offset_modulo <<= 1;
	    ydstorg_modulo <<= 1;
	}
	pMga->YDstOrg = offset / bytesPerPixel;

	/*
	 * When this was unconditional, it caused a line of horizontal garbage
	 * at the middle right of the screen at the 4Meg boundary in 32bpp
	 * (and presumably any other modes that use more than 4M). But it's
	 * essential for 24bpp (it may not matter either way for 8bpp & 16bpp,
	 * I'm not sure; I didn't notice problems when I checked with and
	 * without.)
	 * DRM Doug Merritt 12/97, submitted to XFree86 6/98 (oops)
	 */
	if (bytesPerPixel < 4) {
	    while ((offset % offset_modulo) != 0 ||
		   (pMga->YDstOrg % ydstorg_modulo) != 0) {
		offset++;
		pMga->YDstOrg = offset / bytesPerPixel;
	    }
	}
    }

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "YDstOrg is set to %d\n",
		   pMga->YDstOrg);
    pMga->FbUsableSize = pMga->FbMapSize - pMga->YDstOrg * bytesPerPixel;
    /*
     * XXX This should be taken into account in some way in the mode valdation
     * section.
     */

    /* Allocate HW cursor buffer at the end of video ram */
    if( pMga->HWCursor && pMga->Dac.CursorOffscreenMemSize ) {
        if( pScrn->virtualY * pScrn->displayWidth * pScrn->bitsPerPixel / 8 <=
        	pMga->FbUsableSize - pMga->Dac.CursorOffscreenMemSize ) {
            pMga->FbUsableSize -= pMga->Dac.CursorOffscreenMemSize;
            pMga->FbCursorOffset =
                pMga->FbMapSize - pMga->Dac.CursorOffscreenMemSize;
        } else {
            pMga->HWCursor = FALSE;
            xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                "Too little offscreen memory for HW cursor; using SW cursor\n");
        }
    }

    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
    case 8:
	mod = "cfb";
	reqSym = "cfbScreenInit";
	break;
    case 16:
	mod = "cfb16";
	reqSym = "cfb16ScreenInit";
	break;
    case 24:
	if (pix24bpp == 24) {
	    mod = "cfb24";
	    reqSym = "cfb24ScreenInit";
	} else {
	    mod = "xf24_32bpp";
	    reqSym = "cfb24_32ScreenInit";
	}
	break;
    case 32:
	if (pMga->Overlay8Plus24) {
	    mod = "xf8_32bpp";
	    reqSym = "cfb8_32ScreenInit";
	    xf86LoaderReqSymLists(xf8_32bppSymbols, NULL);
	} else {
	    mod = "cfb32";
	    reqSym = "cfb32ScreenInit";
	    
	}
	break;
    }
    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	MGAFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymbols(reqSym, NULL);

    /* Load XAA if needed */
    if (!pMga->NoAccel) {
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    /* Load ramdac if needed */
    if (pMga->HWCursor) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(ramdacSymbols, NULL);
    }

    /* Load shadowfb if needed */
    if (pMga->ShadowFB) {
	if (!xf86LoadSubModule(pScrn, "shadowfb")) {
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(shadowSymbols, NULL);
    }

    pMga->CurrentLayout.bitsPerPixel = pScrn->bitsPerPixel;
    pMga->CurrentLayout.depth = pScrn->depth;
    pMga->CurrentLayout.displayWidth = pScrn->displayWidth;
    pMga->CurrentLayout.weight.red = pScrn->weight.red;
    pMga->CurrentLayout.weight.green = pScrn->weight.green;
    pMga->CurrentLayout.weight.blue = pScrn->weight.blue;
    pMga->CurrentLayout.Overlay8Plus24 = pMga->Overlay8Plus24;
    pMga->CurrentLayout.mode = pScrn->currentMode;

    return TRUE;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
MGAMapMem(ScrnInfoPtr pScrn)
{
    MGAPtr pMga;

    pMga = MGAPTR(pScrn);

    /*
     * Map IO registers to virtual address space
     */ 
    /*
     * For Alpha, we need to map SPARSE memory, since we need
     * byte/short access.  This is taken care of automatically by the
     * os-support layer.
     */
    pMga->IOBase = xf86MapPciMem(pScrn->scrnIndex,
				 VIDMEM_MMIO | VIDMEM_READSIDEEFFECT,
				 pMga->PciTag, pMga->IOAddress, 0x4000);
    if (pMga->IOBase == NULL)
	return FALSE;

#ifdef __alpha__
    pMga->IOBaseDense = xf86MapPciMem(pScrn->scrnIndex,
				      VIDMEM_MMIO | VIDMEM_MMIO_32BIT,
				      pMga->PciTag, pMga->IOAddress, 0x4000);
    if (pMga->IOBaseDense == NULL)
	return FALSE;
#endif

    pMga->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pMga->PciTag, pMga->FbAddress,
				 pMga->FbMapSize);
    if (pMga->FbBase == NULL)
	return FALSE;

    pMga->FbStart = pMga->FbBase + pMga->YDstOrg * (pScrn->bitsPerPixel / 8);


    /* Map the ILOAD transfer window if there is one.  We only make
	DWORD access on DWORD boundaries to this window */
    if (pMga->ILOADAddress) {
	pMga->ILOADBase = xf86MapPciMem(pScrn->scrnIndex,
				VIDMEM_MMIO | VIDMEM_MMIO_32BIT |
				    VIDMEM_READSIDEEFFECT, 
				pMga->PciTag, pMga->ILOADAddress, 0x800000);
    } else
	pMga->ILOADBase = NULL;

    return TRUE;
}

static Bool
MGAMapMemFBDev(ScrnInfoPtr pScrn)
{
    MGAPtr pMga;

    pMga = MGAPTR(pScrn);

    pMga->FbBase = fbdevHWMapVidmem(pScrn);
    if (pMga->FbBase == NULL)
	return FALSE;

    pMga->IOBase = fbdevHWMapMMIO(pScrn);
    if (pMga->IOBase == NULL)
	return FALSE;

    pMga->FbStart = pMga->FbBase + pMga->YDstOrg * (pScrn->bitsPerPixel / 8);

#if 1 /* can't ask matroxfb for a mapping of the iload window */

    /* Map the ILOAD transfer window if there is one.  We only make
	DWORD access on DWORD boundaries to this window */
    if(pMga->ILOADAddress)
	pMga->ILOADBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, 
				pMga->PciTag, pMga->ILOADAddress, 0x800000);
    else  pMga->ILOADBase = NULL;
#endif
    return TRUE;
}



/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
MGAUnmapMem(ScrnInfoPtr pScrn)
{
    MGAPtr pMga;

    pMga = MGAPTR(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pMga->IOBase, 0x4000);
    pMga->IOBase = NULL;

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pMga->FbBase, pMga->FbMapSize);
    pMga->FbBase = NULL;
    pMga->FbStart = NULL;

    if(pMga->ILOADBase)
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pMga->ILOADBase, 0x800000);
    pMga->ILOADBase = NULL;
    return TRUE;
}

static Bool
MGAUnmapMemFBDev(ScrnInfoPtr pScrn)
{
    MGAPtr pMga;

    pMga = MGAPTR(pScrn);
    fbdevHWUnmapVidmem(pScrn);
    pMga->FbBase = NULL;
    pMga->FbStart = NULL;
    fbdevHWUnmapMMIO(pScrn);
    pMga->IOBase = NULL;
    /* XXX ILOADBase */
    return TRUE;
}




/*
 * This function saves the video state.
 */
static void
MGASave(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg = &hwp->SavedReg;
    MGAPtr pMga = MGAPTR(pScrn);
    MGARegPtr mgaReg = &pMga->SavedReg;

    /* Only save text mode fonts/text for the primary card */
    (*pMga->Save)(pScrn, vgaReg, mgaReg, pMga->Primary);
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
MGAModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg;
    MGAPtr pMga = MGAPTR(pScrn);
    MGARegPtr mgaReg;

    vgaHWUnlock(hwp);

    /* Initialise the ModeReg values */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;

    if (!(*pMga->ModeInit)(pScrn, mode))
	return FALSE;

    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);
    vgaReg = &hwp->ModeReg;
    mgaReg = &pMga->ModeReg;

#ifdef XF86DRI
   if (pMga->directRenderingEnabled) {
       DRILock(screenInfo.screens[pScrn->scrnIndex]);
       MGASwapContext(screenInfo.screens[pScrn->scrnIndex]);
   }
#endif
     
    (*pMga->Restore)(pScrn, vgaReg, mgaReg, FALSE);

    MGAStormEngineInit(pScrn);
    vgaHWProtect(pScrn, FALSE);

    if (xf86IsPc98()) {
	if (pMga->Chipset == PCI_CHIP_MGA2064)
	    outb(0xfac, 0x01);
	else
	    outb(0xfac, 0x02);
    }

    pMga->CurrentLayout.mode = mode;
   
#ifdef XF86DRI
   if (pMga->directRenderingEnabled)
     DRIUnlock(screenInfo.screens[pScrn->scrnIndex]);
#endif

    return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void 
MGARestore(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg = &hwp->SavedReg;
    MGAPtr pMga = MGAPTR(pScrn);
    MGARegPtr mgaReg = &pMga->SavedReg;

    MGAStormSync(pScrn);

    /* Only restore text mode fonts/text for the primary card */
    vgaHWProtect(pScrn, TRUE);
    if (pMga->Primary)
        (*pMga->Restore)(pScrn, vgaReg, mgaReg, TRUE);
    else
        vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);
    vgaHWProtect(pScrn, FALSE);
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
MGAScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    MGAPtr pMga;
    MGARamdacPtr MGAdac;
    int ret;
    VisualPtr visual;
    unsigned char *FBStart;
    int width, height, displayWidth;

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    hwp = VGAHWPTR(pScrn);
    pMga = MGAPTR(pScrn);
    MGAdac = &pMga->Dac;

    /* Map the MGA memory and MMIO areas */
    if (pMga->FBDev) {
	if (!MGAMapMemFBDev(pScrn))
	    return FALSE;
    } else {
	if (!MGAMapMem(pScrn))
	    return FALSE;
    }

    /* Initialise the MMIO vgahw functions */
    vgaHWSetMmioFuncs(hwp, pMga->IOBase, PORT_OFFSET);
    vgaHWGetIOBase(hwp);

    /* Map the VGA memory when the primary video */
    if (pMga->Primary && !pMga->FBDev) {
	hwp->MapSize = 0x10000;
	if (!vgaHWMapMem(pScrn))
	    return FALSE;
    }

    if (pMga->FBDev) {
	fbdevHWSave(pScrn);
	/* Disable VGA core, and leave memory access on */
	pciSetBitsLong(pMga->PciTag, PCI_OPTION_REG, 0x100, 0x000);
	if (!fbdevHWModeInit(pScrn, pScrn->currentMode))
	    return FALSE;
	MGAStormEngineInit(pScrn);
    } else {
	/* Save the current state */
	MGASave(pScrn);
	/* Initialise the first mode */
	if (!MGAModeInit(pScrn, pScrn->currentMode))
	    return FALSE;
    }


    /* Darken the screen for aesthetic reasons and set the viewport */
    MGASaveScreen(pScreen, SCREEN_SAVER_ON);
    pScrn->AdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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
     * Reset the visual list.
     */
    miClearVisualTypes();

    /* Setup the visuals we support. */

    /* All MGA support DirectColor and can do overlays in 32bpp */
    if(pMga->Overlay8Plus24 && (pScrn->bitsPerPixel == 32)) {
	if (!miSetVisualTypes(8, PseudoColorMask | GrayScaleMask,
			      pScrn->rgbBits, PseudoColor))
		return FALSE;
	if (!miSetVisualTypes(24, TrueColorMask, pScrn->rgbBits, TrueColor))
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

    width = pScrn->virtualX;
    height = pScrn->virtualY;
    displayWidth = pScrn->displayWidth;


    if(pMga->Rotate) {
	height = pScrn->virtualX;
	width = pScrn->virtualY;
    }

    if(pMga->ShadowFB) {
 	pMga->ShadowPitch = BitmapBytePad(pScrn->bitsPerPixel * width);
	pMga->ShadowPtr = xalloc(pMga->ShadowPitch * height);
	displayWidth = pMga->ShadowPitch / (pScrn->bitsPerPixel >> 3);
        FBStart = pMga->ShadowPtr;
    } else {
	pMga->ShadowPtr = NULL;
	FBStart = pMga->FbStart;
    }

#ifdef XF86DRI
     /*
      * Setup DRI after visuals have been established, but before cfbScreenInit
      * is called.   cfbScreenInit will eventually call into the drivers
      * InitGLXVisuals call back.
      */

   pMga->directRenderingEnabled = MGADRIScreenInit(pScreen);
   /* Force the initialization of the context */
   MGALostContext(pScreen);
#endif
     
   
    switch (pScrn->bitsPerPixel) {
    case 8:
	ret = cfbScreenInit(pScreen, FBStart,
			width, height,
			pScrn->xDpi, pScrn->yDpi,
			displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, FBStart,
			width, height,
			pScrn->xDpi, pScrn->yDpi,
			displayWidth);
	break;
    case 24:
	if (pix24bpp == 24)
	    ret = cfb24ScreenInit(pScreen, FBStart,
			width, height,
			pScrn->xDpi, pScrn->yDpi,
			displayWidth);
	else
	    ret = cfb24_32ScreenInit(pScreen, FBStart,
			width, height,
			pScrn->xDpi, pScrn->yDpi,
			displayWidth);
	break;
    case 32:
	if(pMga->Overlay8Plus24)
	    ret = cfb8_32ScreenInit(pScreen, FBStart,
			width, height,
			pScrn->xDpi, pScrn->yDpi,
			displayWidth);
	else 
	    ret = cfb32ScreenInit(pScreen, FBStart,
			width, height,
			pScrn->xDpi, pScrn->yDpi,
			displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in MGAScreenInit\n",
		   pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;


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
    }

    xf86SetBlackWhitePixels(pScreen);

    if(!pMga->ShadowFB) /* hardware cursor needs to wrap this layer */
	MGADGAInit(pScreen);

    if (!pMga->NoAccel)
	MGAStormAccelInit(pScreen);

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);

    /* Initialize software cursor.  
	Must precede creation of the default colormap */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* Initialize HW cursor layer. 
	Must follow software cursor initialization*/
    if (pMga->HWCursor) { 
	if(!MGAHWCursorInit(pScreen))
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, 
		"Hardware cursor initialization failed\n");
    }

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    /* Initialize colormap layer.  
	Must follow initialization of the default colormap */
    if(!xf86HandleColormaps(pScreen, 256, 8, 
	(pMga->FBDev ? fbdevHWLoadPalette : MGAdac->LoadPalette), NULL,
	CMAP_PALETTED_TRUECOLOR | CMAP_RELOAD_ON_MODE_SWITCH))	
	return FALSE;

    if(pMga->Overlay8Plus24) { /* Must come after colormap initialization */
	if(!xf86Overlay8Plus32Init(pScreen))
	    return FALSE;
    }

    if(pMga->ShadowFB) {
	RefreshAreaFuncPtr refreshArea = MGARefreshArea;

	if(pMga->Rotate) {
	    if (!pMga->PointerMoved) {
	    pMga->PointerMoved = pScrn->PointerMoved;
	    pScrn->PointerMoved = MGAPointerMoved;
	    }
	    
	   switch(pScrn->bitsPerPixel) {
	   case 8:	refreshArea = MGARefreshArea8;	break;
	   case 16:	refreshArea = MGARefreshArea16;	break;
	   case 24:	refreshArea = MGARefreshArea24;	break;
	   case 32:	refreshArea = MGARefreshArea32;	break;
	   }
	}

	ShadowFBInit(pScreen, refreshArea);
    }

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, MGADisplayPowerManagementSet, 0);
#endif
   
#ifdef XF86DRI
    /* Initialize the Warp engine */
   if (pMga->directRenderingEnabled) {
    pMga->directRenderingEnabled = mgaConfigureWarp(pScrn);
   }
   if (pMga->directRenderingEnabled) {
      /* Now that mi, cfb, drm and others have done their thing, 
       * complete the DRI setup.
       */
       pMga->directRenderingEnabled = MGADRIFinishScreenInit(pScreen);
   }
   if (pMga->directRenderingEnabled) {
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering enabled\n");
   } else {
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering disabled\n");
   }
#endif
     
    pScrn->memPhysBase = pMga->FbAddress;
    pScrn->fbOffset = pMga->YDstOrg * (pScrn->bitsPerPixel / 8);

    MGAInitVideo(pScreen);

    pScreen->SaveScreen = MGASaveScreen;

    /* Wrap the current CloseScreen function */
    pMga->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = MGACloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    /* Done */
    return TRUE;
}


/* Usually mandatory */
Bool
MGASwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return MGAModeInit(xf86Screens[scrnIndex], mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
void 
MGAAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn;
    int Base, tmp, count;
    MGAFBLayout *pLayout;
    MGAPtr pMga;

    pScrn = xf86Screens[scrnIndex];
    pMga = MGAPTR(pScrn);
    pLayout = &pMga->CurrentLayout;


    if(pMga->ShowCache && y && pScrn->vtSema)
	y += pScrn->virtualY - 1;

    Base = (y * pLayout->displayWidth + x + pMga->YDstOrg) >>
		(3 - pMga->BppShifts[(pLayout->bitsPerPixel >> 3) - 1]);

    if (pLayout->bitsPerPixel == 24) {
	if (pMga->Chipset == PCI_CHIP_MGAG400)
	   Base &= ~1;  /* Not sure why */
	Base *= 3;
    }

    /* find start of retrace */
    while (INREG8(0x1FDA) & 0x08);
    while (!(INREG8(0x1FDA) & 0x08)); 
    /* wait until we're past the start (fixseg.c in the DDK) */
    count = INREG(MGAREG_VCOUNT) + 2;
    while(INREG(MGAREG_VCOUNT) < count);
    
    OUTREG16(MGAREG_CRTC_INDEX, (Base & 0x00FF00) | 0x0C);
    OUTREG16(MGAREG_CRTC_INDEX, ((Base & 0x0000FF) << 8) | 0x0D);
    OUTREG8(MGAREG_CRTCEXT_INDEX, 0x00);
    tmp = INREG8(MGAREG_CRTCEXT_DATA);
    OUTREG8(MGAREG_CRTCEXT_DATA, (tmp & 0xF0) | ((Base & 0x0F0000) >> 16));

}


/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
MGAEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
#ifdef XF86DRI 
    ScreenPtr pScreen;
    MGAPtr pMGA;

    pMGA = MGAPTR(pScrn);
    if (pMGA->directRenderingEnabled) {
        pScreen = screenInfo.screens[scrnIndex];
        DRIUnlock(pScreen);
    }
#endif
     
    if (!MGAModeInit(pScrn, pScrn->currentMode))
	return FALSE;
    MGAAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    return TRUE;
}

static Bool
MGAEnterVTFBDev(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
#ifdef XF86DRI 
    ScreenPtr pScreen;
    MGAPtr pMGA;

    pMGA = MGAPTR(pScrn);
    if (pMGA->directRenderingEnabled) {
        pScreen = screenInfo.screens[scrnIndex];
        DRIUnlock(pScreen);
    }
#endif

    fbdevHWEnterVT(scrnIndex,flags);
    MGAStormEngineInit(pScrn);
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
MGALeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
#ifdef XF86DRI
    ScreenPtr pScreen;
    MGAPtr pMGA;
#endif

    MGARestore(pScrn);
    vgaHWLock(hwp);

    if (xf86IsPc98())
	outb(0xfac, 0x00);
#ifdef XF86DRI
    pMGA = MGAPTR(pScrn);
    if (pMGA->directRenderingEnabled) {
        pScreen = screenInfo.screens[scrnIndex];
        DRILock(pScreen);
    }
#endif
     
}


/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should also unmap the video memory, and free
 * any per-generation data allocated by the driver.  It should finish
 * by unwrapping and calling the saved CloseScreen function.
 */

/* Mandatory */
static Bool
MGACloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    MGAPtr pMga = MGAPTR(pScrn);

    if (pScrn->vtSema) {
	if (pMga->FBDev) {
	    fbdevHWRestore(pScrn);
	    MGAUnmapMemFBDev(pScrn);
        } else {
	    MGARestore(pScrn);
	    vgaHWLock(hwp);
	    MGAUnmapMem(pScrn);
	    vgaHWUnmapMem(pScrn);
	}
    }
#ifdef XF86DRI 
   if (pMga->directRenderingEnabled) {
       MGADRICloseScreen(pScreen);
       pMga->directRenderingEnabled=FALSE;
   }
#endif
     
    if (pMga->AccelInfoRec)
	XAADestroyInfoRec(pMga->AccelInfoRec);
    if (pMga->CursorInfoRec)
    	xf86DestroyCursorInfoRec(pMga->CursorInfoRec);
    if (pMga->ShadowPtr)
	xfree(pMga->ShadowPtr);
    if (pMga->DGAModes)
	xfree(pMga->DGAModes);
    if (pMga->adaptor)
	xfree(pMga->adaptor);

    pScrn->vtSema = FALSE;

    if (xf86IsPc98())
	outb(0xfac, 0x00);

    if(pMga->BlockHandler)
	pScreen->BlockHandler = pMga->BlockHandler;

    pScreen->CloseScreen = pMga->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any persistent data structures */

/* Optional */
static void
MGAFreeScreen(int scrnIndex, int flags)
{
    /*
     * This only gets called when a screen is being deleted.  It does not
     * get called routinely at the end of a server generation.
     */
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
	vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    MGAFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
MGAValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    int lace;

    lace = 1 + ((mode->Flags & V_INTERLACE) != 0);

    if ((mode->CrtcHDisplay <= 2048) &&
	(mode->CrtcHSyncStart <= 4096) && 
	(mode->CrtcHSyncEnd <= 4096) && 
	(mode->CrtcHTotal <= 4096) &&
	(mode->CrtcVDisplay <= 2048 * lace) &&
	(mode->CrtcVSyncStart <= 4096 * lace) &&
	(mode->CrtcVSyncEnd <= 4096 * lace) &&
	(mode->CrtcVTotal <= 4096 * lace)) {
	return(MODE_OK);
    } else {
	return(MODE_BAD);
    }
}


/* Do screen blanking */

/* Mandatory */
static Bool
MGASaveScreen(ScreenPtr pScreen, int mode)
{
    return vgaHWSaveScreen(pScreen, mode);
}


/*
 * MGADisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void
MGADisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			     int flags)
{
	MGAPtr pMga = MGAPTR(pScrn);
	unsigned char seq1 = 0, crtcext1 = 0;

ErrorF("MGADisplayPowerManagementSet: %d\n", PowerManagementMode);

	switch (PowerManagementMode)
	{
	case DPMSModeOn:
	    /* Screen: On; HSync: On, VSync: On */
	    seq1 = 0x00;
	    crtcext1 = 0x00;
	    break;
	case DPMSModeStandby:
	    /* Screen: Off; HSync: Off, VSync: On */
	    seq1 = 0x20;
	    crtcext1 = 0x10;
	    break;
	case DPMSModeSuspend:
	    /* Screen: Off; HSync: On, VSync: Off */
	    seq1 = 0x20;
	    crtcext1 = 0x20;
	    break;
	case DPMSModeOff:
	    /* Screen: Off; HSync: Off, VSync: Off */
	    seq1 = 0x20;
	    crtcext1 = 0x30;
	    break;
	}
	/* XXX Prefer an implementation that doesn't depend on VGA specifics */
	OUTREG8(0x1FC4, 0x01);	/* Select SEQ1 */
	seq1 |= INREG8(0x1FC5) & ~0x20;
	OUTREG8(0x1FC5, seq1);
	OUTREG8(0x1FDE, 0x01);	/* Select CRTCEXT1 */
	crtcext1 |= INREG8(0x1FDF) & ~0x30;
	OUTREG8(0x1FDF, crtcext1);
}
#endif

#if defined (DEBUG)
/*
 * some functions to track input/output in the server
 */

CARD8
dbg_inreg8(ScrnInfoPtr pScrn,int addr,int verbose)
{
    MGAPtr pMga;
    CARD8 ret;

    pMga = MGAPTR(pScrn);
    ret = *(volatile CARD8 *)(pMga->IOBase + (addr));
    if(verbose)
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "inreg8 : 0x%8x = 0x%x\n",addr,ret);
    return ret;
}

CARD16
dbg_inreg16(ScrnInfoPtr pScrn,int addr,int verbose)
{
    MGAPtr pMga;
    CARD16 ret;

    pMga = MGAPTR(pScrn);
    ret = *(volatile CARD16 *)(pMga->IOBase + (addr));
    if(verbose)
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "inreg16: 0x%8x = 0x%x\n",addr,ret);
    return ret;
}

CARD32
dbg_inreg32(ScrnInfoPtr pScrn,int addr,int verbose)
{
    MGAPtr pMga;
    CARD32 ret;

    pMga = MGAPTR(pScrn);
    ret = *(volatile CARD32 *)(pMga->IOBase + (addr));
    if(verbose)
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "inreg32: 0x%8x = 0x%x\n",addr,ret);
    return ret;
}

void
dbg_outreg8(ScrnInfoPtr pScrn,int addr,int val)
{
    MGAPtr pMga;
    CARD8 ret;

    pMga = MGAPTR(pScrn);
#if 0
    if( addr = 0x1fdf )
    	return;
#endif
    if( addr != 0x3c00 ) {
	ret = dbg_inreg8(pScrn,addr,0);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "outreg8 : 0x%8x = 0x%x was 0x%x\n",addr,val,ret);
    }
    else {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "outreg8 : index 0x%x\n",val);
    }
    *(volatile CARD8 *)(pMga->IOBase + (addr)) = (val);
}

void
dbg_outreg16(ScrnInfoPtr pScrn,int addr,int val)
{
    MGAPtr pMga;
    CARD16 ret;

#if 0
    if (addr == 0x1fde)
    	return;
#endif
    pMga = MGAPTR(pScrn);
    ret = dbg_inreg16(pScrn,addr,0);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "outreg16 : 0x%8x = 0x%x was 0x%x\n",addr,val,ret);
    *(volatile CARD16 *)(pMga->IOBase + (addr)) = (val);
}

void
dbg_outreg32(ScrnInfoPtr pScrn,int addr,int val)
{
    MGAPtr pMga;
    CARD32 ret;

    if (((addr & 0xff00) == 0x1c00) 
    	&& (addr != 0x1c04)
/*    	&& (addr != 0x1c1c) */
    	&& (addr != 0x1c20)
    	&& (addr != 0x1c24)
    	&& (addr != 0x1c80)
    	&& (addr != 0x1c8c)
    	&& (addr != 0x1c94)
    	&& (addr != 0x1c98)
    	&& (addr != 0x1c9c)
	 ) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "refused address 0x%x\n",addr);
    	return;
    }
    pMga = MGAPTR(pScrn);
    ret = dbg_inreg32(pScrn,addr,0);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "outreg32 : 0x%8x = 0x%x was 0x%x\n",addr,val,ret);
    *(volatile CARD32 *)(pMga->IOBase + (addr)) = (val);
}
#endif /* DEBUG */
