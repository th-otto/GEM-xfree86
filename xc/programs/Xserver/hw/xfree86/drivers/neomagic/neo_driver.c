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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/neomagic/neo_driver.c,v 1.18 2000/03/06 23:54:11 dawes Exp $ */

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

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86Resources.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* This is used for module versioning */
#include "xf86Version.h"

/* All drivers using the vgahw module need this */
#include "vgaHW.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers implementing backing store need this */
#include "mibstore.h"

/* All drivers using the mi banking wrapper need this */
#include "mibank.h"

/* All drivers using the mi colormap manipulation need this */
#include "micmap.h"

/* If using cfb, cfb.h is required. */
#define PSZ 8
#include "cfb.h"  
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "cfb24_32.h"

/* Needed by Resources Access Control (RAC) */
#include "xf86RAC.h"

/* Needed by the Shadow Framebuffer */
#include "shadowfb.h"

/* Needed for Device Data Channel (DDC) support */
#include "xf86DDC.h"

/*
 * Driver data structures.
 */
#include "neo.h"
#include "neo_reg.h"
#include "neo_macros.h"

/* These need to be checked */
#if 0
#ifdef XFreeXDGA 
#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif
#endif

/* Mandatory functions */
static OptionInfoPtr	NEOAvailableOptions(int chipid, int busid);
static void     NEOIdentify(int flags);
static Bool     NEOProbe(DriverPtr drv, int flags);
static Bool     NEOPreInit(ScrnInfoPtr pScrn, int flags);
static Bool     NEOScreenInit(int Index, ScreenPtr pScreen, int argc,
                                  char **argv);
static Bool     NEOSwitchMode(int scrnIndex, DisplayModePtr mode,
                                  int flags);
static void     NEOAdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool     NEOEnterVT(int scrnIndex, int flags);
static void     NEOLeaveVT(int scrnIndex, int flags);
static Bool     NEOCloseScreen(int scrnIndex, ScreenPtr pScreen);
static void     NEOFreeScreen(int scrnIndex, int flags);
static int      NEOValidMode(int scrnIndex, DisplayModePtr mode,
                                 Bool verbose, int flags);

/* Internally used functions */
static int      neoFindIsaDevice(GDevPtr dev);
static Bool     neoModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void     neoSave(ScrnInfoPtr pScrn);
static void     neoRestore(ScrnInfoPtr pScrn, vgaRegPtr VgaReg,
				 NeoRegPtr NeoReg, Bool restoreFonts);
static void     neoLock(ScrnInfoPtr pScrn);
static void     neoUnlock(ScrnInfoPtr pScrn);
static Bool	neoMapMem(ScrnInfoPtr pScrn);
static Bool	neoUnmapMem(ScrnInfoPtr pScrn);
static void     neoProgramShadowRegs(ScrnInfoPtr pScrn, vgaRegPtr VgaReg,
				     NeoRegPtr restore);
static void     neoCalcVCLK(ScrnInfoPtr pScrn, long freq);
static void     neo_ddc1(int scrnIndex);
static void     NeoDisplayPowerManagementSet(ScrnInfoPtr pScrn,
				int PowerManagementMode, int flags);

#define VERSION 4000
#define NEO_NAME "NEOMAGIC"
#define NEO_DRIVER_NAME "neomagic"

#define NEO_MAJOR_VERSION 1
#define NEO_MINOR_VERSION 0
#define NEO_PATCHLEVEL 0

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

DriverRec NEOMAGIC = {
    VERSION,
    NEO_DRIVER_NAME,
#if 0
    "Driver for the Neomagic chipsets",
#endif
    NEOIdentify,
    NEOProbe,
    NEOAvailableOptions,
    NULL,
    0
};

static SymTabRec NEOChipsets[] = {
    { NM2070,   "neo2070" },
    { NM2090,   "neo2090" },
    { NM2093,   "neo2093" },
    { NM2097,   "neo2097" },
    { NM2160,   "neo2160" },
    { NM2200,   "neo2200" },
    { -1,		 NULL }
};

/* Conversion PCI ID to chipset name */
static PciChipsets NEOPCIchipsets[] = {
    { NM2070,  PCI_CHIP_NM2070,  RES_SHARED_VGA },
    { NM2090,  PCI_CHIP_NM2090,  RES_SHARED_VGA },
    { NM2093,  PCI_CHIP_NM2093,  RES_SHARED_VGA },
    { NM2097,  PCI_CHIP_NM2097,  RES_SHARED_VGA },
    { NM2160,  PCI_CHIP_NM2160,  RES_SHARED_VGA },
    { NM2200,  PCI_CHIP_NM2200,  RES_SHARED_VGA },
    { -1,	     -1,	     RES_UNDEFINED}
};

static IsaChipsets NEOISAchipsets[] = {
    { NM2070,               RES_EXCLUSIVE_VGA },
    { NM2090,               RES_EXCLUSIVE_VGA },
    { NM2093,               RES_EXCLUSIVE_VGA },
    { NM2097,               RES_EXCLUSIVE_VGA },
    { NM2160,               RES_EXCLUSIVE_VGA },
    { NM2200,               RES_EXCLUSIVE_VGA },
    { -1,			RES_UNDEFINED }
};

/* The options supported by the Neomagic Driver */
typedef enum {
    OPTION_NOLINEAR_MODE,
    OPTION_NOACCEL,
    OPTION_SW_CURSOR,
    OPTION_NO_MMIO,
    OPTION_INTERN_DISP,
    OPTION_EXTERN_DISP,
    OPTION_LCD_CENTER,
    OPTION_LCD_STRETCH,
    OPTION_SHADOW_FB,
    OPTION_PCI_BURST,
    OPTION_PROG_LCD_MODE_REGS,
    OPTION_PROG_LCD_MODE_STRETCH,
    OPTION_OVERRIDE_VALIDATE_MODE,
    OPTION_ROTATE
} NEOOpts;

static OptionInfoRec NEO_2070_Options[] = {
    { OPTION_NOACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,	"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NO_MMIO,	"noMMIO",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_INTERN_DISP,"internDisp",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_EXTERN_DISP,"externDisp",  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_CENTER, "LcdCenter",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_STRETCH, "NoStretch",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHADOW_FB,   "ShadowFB",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_BURST,	 "pciBurst",	OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_ROTATE, 	 "Rotate",	OPTV_ANYSTR,	{0}, FALSE },
    { OPTION_PROG_LCD_MODE_REGS, "progLcdModeRegs",
      OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_PROG_LCD_MODE_STRETCH, "progLcdModeStretch",
      OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_OVERRIDE_VALIDATE_MODE, "overrideValidateMode",
      OPTV_BOOLEAN, {0}, FALSE },
    { -1,                  NULL,           OPTV_NONE,	{0}, FALSE }
};

static OptionInfoRec NEOOptions[] = {
    { OPTION_NOLINEAR_MODE,"NoLinear",  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,	"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NO_MMIO,	"noMMIO",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_INTERN_DISP,"internDisp",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_EXTERN_DISP,"externDisp",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_CENTER, "LcdCenter",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHADOW_FB,  "ShadowFB",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_STRETCH,"NoStretch",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_BURST,	 "pciBurst",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_ROTATE, 	 "Rotate",	OPTV_ANYSTR,	{0}, FALSE },
    { OPTION_PROG_LCD_MODE_REGS, "progLcdModeRegs",
      OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_PROG_LCD_MODE_STRETCH, "progLcdModeStretch",
      OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_OVERRIDE_VALIDATE_MODE, "overrideValidateMode",
      OPTV_BOOLEAN, {0}, FALSE },
    { -1,                  NULL,           OPTV_NONE,	{0}, FALSE }
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
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    NULL
};

static const char *cfbSymbols[] = {
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb24_32ScreenInit",
    NULL
};

static const char *xaaSymbols[] = {
    "XAADestroyInfoRec",
    "XAACreateInfoRec",
    "XAAInit",
    NULL
};

static const char *ramdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
    NULL
};

static const char *shadowSymbols[] = {
    "ShadowFBInit",
    NULL
};

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

#ifdef XFree86LOADER

static MODULESETUPPROTO(neoSetup);

static XF86ModuleVersionInfo neoVersRec =
{
	"neomagic",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	NEO_MAJOR_VERSION, NEO_MINOR_VERSION, NEO_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData
 */
XF86ModuleData neomagicModuleData = { &neoVersRec, neoSetup, NULL };

static pointer
neoSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
        xf86AddDriver(&NEOMAGIC, module, 0);

	/*
	 * Modules that this driver always requires can be loaded here
	 * by calling LoadSubModule().
	 */

	/*
	 * Tell the loader about symbols from other modules that this module
	 * might refer to.
	 */
	LoaderRefSymLists(vgahwSymbols, cfbSymbols, xaaSymbols,
			  ramdacSymbols, shadowSymbols,
			  ddcSymbols, i2cSymbols, NULL);
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

static Bool
NEOGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate a NEORec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(NEORec), 1);

    if (pScrn->driverPrivate == NULL)
	return FALSE;
        return TRUE;
}

static void
NEOFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

static
OptionInfoPtr
NEOAvailableOptions(int chipid, int busid)
{
    int vendor = ((chipid & 0xffff0000) >> 16);
    int chip = (chip & 0x0000ffff);

    if (chip == PCI_CHIP_NM2070)
	return NEO_2070_Options;
    else
    	return NEOOptions;
}

/* Mandatory */
static void
NEOIdentify(int flags)
{
    xf86PrintChipsets(NEO_NAME, "Driver for Neomagic chipsets",
			NEOChipsets);
}

/* Mandatory */
static Bool
NEOProbe(DriverPtr drv, int flags)
{
    Bool foundScreen = FALSE;
    int numDevSections, numUsed;
    GDevPtr *devSections = NULL;
    int *usedChips;
    int i;

    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice(NEO_DRIVER_NAME,
					  &devSections)) <= 0) {
	return FALSE;
    }
  
    /* PCI BUS */
    if (xf86GetPciVideoInfo() ) {
	numUsed = xf86MatchPciInstances(NEO_NAME, PCI_VENDOR_NEOMAGIC,
					NEOChipsets, NEOPCIchipsets, 
					devSections,numDevSections,
					drv, &usedChips);

	if (numUsed > 0) {
	  if (flags & PROBE_DETECT)
	    foundScreen = TRUE;
	  else for (i = 0; i < numUsed; i++) {
	    ScrnInfoPtr pScrn;
	    /* Allocate a ScrnInfoRec and claim the slot */
	    pScrn = xf86AllocateScreen(drv,0);
	    pScrn->driverVersion = VERSION;
	    pScrn->driverName    = NEO_DRIVER_NAME;
	    pScrn->name          = NEO_NAME;
	    pScrn->Probe         = NEOProbe;
	    pScrn->PreInit       = NEOPreInit;
	    pScrn->ScreenInit    = NEOScreenInit;
	    pScrn->SwitchMode    = NEOSwitchMode;
	    pScrn->AdjustFrame   = NEOAdjustFrame;
	    pScrn->EnterVT       = NEOEnterVT;
	    pScrn->LeaveVT       = NEOLeaveVT;
	    pScrn->FreeScreen    = NEOFreeScreen;
	    pScrn->ValidMode     = NEOValidMode;
	    foundScreen = TRUE;
	    xf86ConfigActivePciEntity(pScrn, usedChips[i], NEOPCIchipsets,
				      NULL, NULL, NULL, NULL, NULL);
	  }
	  xfree(usedChips);
	}
    }
    /* Isa Bus */

    numUsed = xf86MatchIsaInstances(NEO_NAME,NEOChipsets,NEOISAchipsets,
				     drv,neoFindIsaDevice,devSections,
				     numDevSections,&usedChips);
    if (numUsed > 0) {
      if (flags & PROBE_DETECT)
	foundScreen = TRUE;
      else for (i = 0; i < numUsed; i++) {
	ScrnInfoPtr pScrn;

	pScrn = xf86AllocateScreen(drv,0);
	pScrn->driverVersion = VERSION;
	pScrn->driverName    = NEO_DRIVER_NAME;
	pScrn->name          = NEO_NAME;
	pScrn->Probe         = NEOProbe;
	pScrn->PreInit       = NEOPreInit;
	pScrn->ScreenInit    = NEOScreenInit;
	pScrn->SwitchMode    = NEOSwitchMode;
	pScrn->AdjustFrame   = NEOAdjustFrame;
	pScrn->EnterVT       = NEOEnterVT;
	pScrn->LeaveVT       = NEOLeaveVT;
	pScrn->FreeScreen    = NEOFreeScreen;
	pScrn->ValidMode     = NEOValidMode;
	foundScreen = TRUE;
	xf86ConfigActiveIsaEntity(pScrn, usedChips[i], NEOISAchipsets,
				  NULL, NULL, NULL, NULL, NULL);
      }
      xfree(usedChips);
    }

    xfree(devSections);
    return foundScreen;
}

static int
neoFindIsaDevice(GDevPtr dev)
{
    unsigned char vgaIOBase;
    unsigned char id;
    
    vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
    /* ��� Too intrusive ? */
    outw(GRAX, 0x2609); /* Unlock NeoMagic registers */

    outb(vgaIOBase + 4, 0x1A);
    id = inb(vgaIOBase + 5);

    outw(GRAX, 0x0009); /* Lock NeoMagic registers */

    switch (id) {
    case PROBED_NM2070 :
	return NM2070;
    case PROBED_NM2090 :
	return NM2090;
    case PROBED_NM2093 :
	return NM2093;
    default :
	return -1;
    }
}


/* Mandatory */
Bool
NEOPreInit(ScrnInfoPtr pScrn, int flags)
{
    ClockRangePtr clockRanges;
    char *mod = NULL;
    int i;
    NEOPtr nPtr;
    vgaHWPtr hwp;
    const char *reqSym = NULL;
    int bppSupport = NoDepth24Support;
    int videoRam = 896;
    int maxClock = 65000;
    int CursorMem = 1024;
    int CursorOff = 0x100;
    int linearSize = 1024;
    int maxWidth = 1024;
    unsigned char type, display;
    int w;
    int apertureSize;
    char *s;

    if (flags & PROBE_DETECT) return FALSE;
    
    /* The vgahw module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    xf86LoaderReqSymLists(vgahwSymbols, NULL);    

    /*
     * Allocate a vgaHWRec.
     */
    if (!vgaHWGetHWRec(pScrn))
	return FALSE;
    hwp = VGAHWPTR(pScrn);
    
    /* Allocate the NeoRec driverPrivate */
    if (!NEOGetRec(pScrn)) {
	return FALSE;
    }
# define RETURN \
    { NEOFreeRec(pScrn);\
			    return FALSE;\
					     }
    
    nPtr = NEOPTR(pScrn);

    /* Since, the capabilities are determined by the chipset the very
     * first thing to do is, figure out the chipset and its capabilities
     */
    /* This driver doesn't expect more than one entity per screen */
    if (pScrn->numEntities > 1)
	RETURN;

    /* This is the general case */
    for (i = 0; i<pScrn->numEntities; i++) {
	nPtr->pEnt = xf86GetEntityInfo(pScrn->entityList[i]);
	if (nPtr->pEnt->resources) return FALSE;
	nPtr->NeoChipset = nPtr->pEnt->chipset;
	pScrn->chipset = (char *)xf86TokenToString(NEOChipsets,
						   nPtr->pEnt->chipset);
	/* This driver can handle ISA and PCI buses */
	if (nPtr->pEnt->location.type == BUS_PCI) {
	    nPtr->PciInfo = xf86GetPciInfoForEntity(nPtr->pEnt->index);
	    nPtr->PciTag = pciTag(nPtr->PciInfo->bus, 
				  nPtr->PciInfo->device,
				  nPtr->PciInfo->func);
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Chipset is a ");
    switch(nPtr->NeoChipset){
    case NM2070:
	ErrorF("MagicGraph 128 (NM2070)");
	break;
    case NM2090 :
	ErrorF("MagicGraph 128V (NM2090)");
	break;
    case NM2093 :
	ErrorF("MagicGraph 128ZV (NM2093)");
	break;
    case NM2097 :
	ErrorF("MagicGraph 128ZV+ (NM2097)");
	break;
    case NM2160 :
	ErrorF("MagicGraph 128XD (NM2160)");
	break;
    case NM2200 :
        ErrorF("MagicMedia 256AV (NM2200)");
	break;
    }
    ErrorF("\n");
    
    vgaHWGetIOBase(hwp);
    nPtr->vgaIOBase = hwp->IOBase;
    vgaHWSetStdFuncs(hwp);
    
    /* Determine the panel type */
    VGAwGR(0x09,0x26);
    type = VGArGR(0x21);
    display = VGArGR(0x20);
    
    /* Determine panel width -- used in NeoValidMode. */
    w = VGArGR(0x20);
    VGAwGR(0x09,0x00);
    switch ((w & 0x18) >> 3) {
    case 0x00 :
	nPtr->NeoPanelWidth  = 640;
	nPtr->NeoPanelHeight = 480;
	break;
    case 0x01 :
	nPtr->NeoPanelWidth  = 800;
	nPtr->NeoPanelHeight = 600;
	break;
    case 0x02 :
	nPtr->NeoPanelWidth  = 1024;
	nPtr->NeoPanelHeight = 768;
	break;
    case 0x03 :
        /* 1280x1024 panel support needs to be added */
#ifdef NOT_DONE
	nPtr->NeoPanelWidth  = 1280;
	nPtr->NeoPanelHeight = 1024;
	break;
#else
	ErrorF("\nOnly 640x480,\n"
                     "     800x600,\n"
                     " and 1024x768 panels are currently supported\n");
	return FALSE;
#endif
    default :
	nPtr->NeoPanelWidth  = 640;
	nPtr->NeoPanelHeight = 480;
	break;
    }
    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Panel is a %dx%d %s %s display\n",
	       nPtr->NeoPanelWidth,
	       nPtr->NeoPanelHeight,
	       (type & 0x02) ? "color" : "monochrome",
	       (type & 0x10) ? "TFT" : "dual scan");

    switch (nPtr->NeoChipset){
    case NM2070:
	bppSupport = NoDepth24Support;
	videoRam   = 896;
	maxClock   = 65000;
	CursorMem  = 2048;
	CursorOff  = 0x100;
	linearSize = 1024;
	maxWidth   = 1024;
	break;
    case NM2090:
    case NM2093:
	bppSupport = Support24bppFb | Support32bppFb |
	    SupportConvert32to24 | PreferConvert32to24;
	videoRam   = 1152;
	maxClock   = 80000;
	CursorMem  = 2048;
	CursorOff  = 0x100;
	linearSize = 2048;
	maxWidth   = 1024;
	break;
    case NM2097:
	bppSupport = Support24bppFb | Support32bppFb |
	    SupportConvert32to24 | PreferConvert32to24;
	videoRam   = 1152;
	maxClock   = 80000;
	CursorMem  = 1024;
	CursorOff  = 0x100;
	linearSize = 2048;
	maxWidth   = 1024;
	break;
    case NM2160:
	bppSupport = Support24bppFb | Support32bppFb |
	    SupportConvert32to24 | PreferConvert32to24;
	videoRam   = 2048;
	maxClock   = 90000;
	CursorMem  = 1024;
	CursorOff  = 0x100;
	linearSize = 2048;
	maxWidth   = 1024;
	break;
    case NM2200:
	bppSupport = Support24bppFb | Support32bppFb |
	    SupportConvert32to24 | PreferConvert32to24;
	videoRam   = 2560;
	maxClock   = 110000;
	CursorMem  = 1024;
	CursorOff  = 0x1000;
	linearSize = 4096;
	maxWidth   = 1280;
	break;
    }

    pScrn->monitor = pScrn->confScreen->monitor;

    if (!xf86LoadSubModule(pScrn, "ddc"))
	return FALSE;
    xf86LoaderReqSymLists(ddcSymbols, NULL);
#if 0
    VGAwGR(0x09,0x26);
    neo_ddc1(pScrn->scrnIndex);
    VGAwGR(0x09,0x00);
#endif
    if (!xf86LoadSubModule(pScrn, "i2c"))
	return FALSE;
    xf86LoaderReqSymLists(i2cSymbols, NULL);
    if (!neo_I2CInit(pScrn))
	return FALSE;

    VGAwGR(0x09,0x26);
    xf86SetDDCproperties(
	pScrn,xf86PrintEDID(xf86DoEDID_DDC2(pScrn->scrnIndex,nPtr->I2C)));
    VGAwGR(0x09,0x00);

    if (!xf86SetDepthBpp(pScrn, 8, 8, 8, bppSupport ))
	return FALSE;
    else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 8:
	case 16:
	    break;
	case 24:
	    if (nPtr->NeoChipset != NM2070)
		break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Given depth (%d) is not supported by this driver\n",
		       pScrn->depth);
	    return FALSE;
	}
    }
    xf86PrintDepthBpp(pScrn);
    if (pScrn->depth == 8)
	pScrn->rgbBits = 6;

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

    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros))
	    return FALSE;
    }

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);
    /* Process the options */
    if (nPtr->NeoChipset == NM2070)
	nPtr->Options = (OptionInfoPtr)NEO_2070_Options;
    else
	nPtr->Options = (OptionInfoPtr)NEOOptions;

    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, nPtr->Options);

    xf86GetOptValBool(nPtr->Options, OPTION_NOLINEAR_MODE,&nPtr->noLinear);
    xf86GetOptValBool(nPtr->Options, OPTION_NOACCEL,&nPtr->noAccel);
    xf86GetOptValBool(nPtr->Options, OPTION_SW_CURSOR,&nPtr->swCursor);
    xf86GetOptValBool(nPtr->Options, OPTION_NO_MMIO,&nPtr->noMMIO);
    xf86GetOptValBool(nPtr->Options, OPTION_INTERN_DISP,&nPtr->internDisp);
    xf86GetOptValBool(nPtr->Options, OPTION_EXTERN_DISP,&nPtr->externDisp);
    xf86GetOptValBool(nPtr->Options, OPTION_LCD_CENTER,&nPtr->lcdCenter);
    xf86GetOptValBool(nPtr->Options, OPTION_LCD_STRETCH,&nPtr->noLcdStretch);
    xf86GetOptValBool(nPtr->Options, OPTION_SHADOW_FB,&nPtr->shadowFB);
    xf86GetOptValBool(nPtr->Options, OPTION_PCI_BURST,&nPtr->onPciBurst);
    xf86GetOptValBool(nPtr->Options,
		      OPTION_PROG_LCD_MODE_REGS,&nPtr->progLcdRegs);
    if (xf86GetOptValBool(nPtr->Options,
		      OPTION_PROG_LCD_MODE_STRETCH,&nPtr->progLcdStretch))
	nPtr->progLcdStrechOpt = TRUE;
    xf86GetOptValBool(nPtr->Options,
		      OPTION_OVERRIDE_VALIDATE_MODE, &nPtr->overrideValidate);
    nPtr->rotate = 0;
    if ((s = xf86GetOptValString(nPtr->Options, OPTION_ROTATE))) {
	if(!xf86NameCmp(s, "CW")) {
	    /* accel is disabled below for shadowFB */
	    nPtr->shadowFB = TRUE;
	    nPtr->swCursor = TRUE;
	    nPtr->rotate = 1;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		       "Rotating screen clockwise - acceleration disabled\n");
	} else if(!xf86NameCmp(s, "CCW")) {
	    nPtr->shadowFB = TRUE;
	    nPtr->swCursor = TRUE;
	    nPtr->rotate = -1;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,  "Rotating screen"
		       "counter clockwise - acceleration disabled\n");
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "\"%s\" is not a valid"
		       "value for Option \"Rotate\"\n", s);
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		       "Valid options are \"CW\" or \"CCW\"\n");
      }
    }

    if (nPtr->shadowFB)
	ErrorF("shadow\n");
    else
	ErrorF("no shadow\n");
    
    if (nPtr->internDisp && nPtr->externDisp)
	xf86DrvMsg(pScrn->scrnIndex,X_CONFIG,
		   "Simultaneous LCD/CRT display mode\n");
    else if (nPtr->externDisp)
	xf86DrvMsg(pScrn->scrnIndex,X_CONFIG,
		   "External CRT only display mode\n");
    else  if (nPtr->internDisp)
	xf86DrvMsg(pScrn->scrnIndex,X_CONFIG,
		   "Internal LCD only display mode\n");
    else {
	nPtr->internDisp = ((display & 0x02) == 0x02);
    	nPtr->externDisp = ((display & 0x01) == 0x01);
	if (nPtr->internDisp && nPtr->externDisp)
	    xf86DrvMsg(pScrn->scrnIndex,X_PROBED,
		       "Simultaneous LCD/CRT display mode\n");
	else if (nPtr->externDisp)
	    xf86DrvMsg(pScrn->scrnIndex,X_PROBED,
		       "External CRT only display mode\n");
	else if (nPtr->internDisp)
	    xf86DrvMsg(pScrn->scrnIndex,X_PROBED,
		       "Internal LCD only display mode\n");
	else {
	    /* this is a fallback if probed values are bogus */
	    nPtr->internDisp = TRUE;
	    xf86DrvMsg(pScrn->scrnIndex,X_DEFAULT,
		       "Internal LCD only display mode\n");
	}
    }
	
    if (nPtr->noLcdStretch)
	xf86DrvMsg(pScrn->scrnIndex,X_CONFIG,
		   "Low resolution video modes are not stretched\n");
    if (nPtr->lcdCenter)
	xf86DrvMsg(pScrn->scrnIndex,X_CONFIG,
		   "Video modes are centered on the display\n");
    if (nPtr->noLinear)
	xf86DrvMsg(pScrn->scrnIndex,X_CONFIG, "using nonlinear mode\n");
    else
	xf86DrvMsg(pScrn->scrnIndex,X_DEFAULT, "using linear mode\n");
    if (nPtr->swCursor)
	xf86DrvMsg(pScrn->scrnIndex,X_CONFIG, "using sofware cursor\n");
    if (nPtr->noMMIO)
	xf86DrvMsg(pScrn->scrnIndex,X_CONFIG, "MMIO mode disabled\n");
    if (nPtr->onPciBurst)
	xf86DrvMsg(pScrn->scrnIndex,X_CONFIG, "using PCI Burst mode\n");
    if (nPtr->shadowFB) {
	if (nPtr->noLinear) {
    	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "Option \"ShadowFB\" ignored. Not supported without"
		       " linear addressing\n");
	    nPtr->shadowFB = FALSE;
	    nPtr->rotate = 0;
	} else {
	    nPtr->noAccel = TRUE;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		    "Using \"Shadow Framebuffer\" - acceleration disabled\n");
	}
    }

    nPtr->NeoFbMapSize = linearSize * 1024;
    nPtr->NeoCursorOffset = CursorOff;

    if (nPtr->pEnt->device->MemBase) {
	/* XXX Check this matches a PCI base address */
	nPtr->NeoLinearAddr = nPtr->pEnt->device->MemBase;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "FB base address is set at 0x%X.\n",
		   nPtr->NeoLinearAddr);
    } else {
	nPtr->NeoLinearAddr = 0;
    }

    if (nPtr->pEnt->device->IOBase) {
	/* XXX Check this matches a PCI base address */
	nPtr->NeoMMIOAddr = nPtr->pEnt->device->IOBase;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "MMIO base address is set at 0x%X.\n",
		   nPtr->NeoMMIOAddr);
    } else {
	nPtr->NeoMMIOAddr = 0;
    }
	
    if (nPtr->pEnt->location.type == BUS_PCI) {
	if (!nPtr->NeoLinearAddr) {
	    nPtr->NeoLinearAddr = nPtr->PciInfo->memBase[0];
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "FB base address is set at 0x%X.\n",
		       nPtr->NeoLinearAddr);
	}
	if (!nPtr->NeoMMIOAddr) {
	    switch (nPtr->NeoChipset) {
	    case NM2070 :
		nPtr->NeoMMIOAddr = nPtr->NeoLinearAddr + 0x100000;
		break;
	    case NM2090:
	    case NM2093:
		nPtr->NeoMMIOAddr = nPtr->NeoLinearAddr + 0x200000;
		break;
	    case NM2160:
	    case NM2097:
	    case NM2200:
		nPtr->NeoMMIOAddr = nPtr->PciInfo->memBase[1];
		break;
	    }
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "MMIO base address is set at 0x%X.\n",
		       nPtr->NeoMMIOAddr);
	}
	/* XXX What about VGA resources in OPERATING mode? */
	if (xf86RegisterResources(nPtr->pEnt->index, NULL, ResExclusive))
	    RETURN;
	    
    } else if (nPtr->pEnt->location.type == BUS_ISA) {
	unsigned int addr;
	resRange linearRes[] = { {ResExcMemBlock|ResBios,0,0},_END };
	
	if (!nPtr->NeoLinearAddr) {
	    VGAwGR(0x09,0x26);
	    addr = VGArGR(0x13);
	    VGAwGR(0x09,0x00);
	    nPtr->NeoLinearAddr = addr << 20;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "FB base address is set at 0x%X.\n",
		       nPtr->NeoLinearAddr);
	}
	if (!nPtr->NeoMMIOAddr) {
	    nPtr->NeoMMIOAddr = nPtr->NeoLinearAddr + 0x100000;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "MMIO base address is set at 0x%X.\n",
		       nPtr->NeoMMIOAddr);
	}
	linearRes[0].rBegin = nPtr->NeoLinearAddr;
	linearRes[1].rEnd = nPtr->NeoLinearAddr + nPtr->NeoFbMapSize - 1;
	if (xf86RegisterResources(nPtr->pEnt->index,linearRes,ResNone)) {
	    nPtr->noLinear = TRUE; /* XXX */
	}
    } else
	RETURN;

    if (nPtr->pEnt->device->videoRam != 0) {
	pScrn->videoRam = nPtr->pEnt->device->videoRam;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "VideoRAM: %d kByte\n",
		   pScrn->videoRam);
    } else {
	pScrn->videoRam = videoRam;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
		   pScrn->videoRam);
    }
    
    if (nPtr->pEnt->device->dacSpeeds[0] != 0) {
	maxClock = nPtr->pEnt->device->dacSpeeds[0];
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Max Clock: %d kHz\n",
		   maxClock);
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Max Clock: %d kByte\n",
		   maxClock);
    }

    pScrn->progClock = TRUE;
    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->ClockMulFactor = 1;
    clockRanges->minClock = 11000;   /* guessed ��� */
    clockRanges->maxClock = maxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    if (!nPtr->internDisp && nPtr->externDisp) 
	clockRanges->interlaceAllowed = TRUE; 
    else
	clockRanges->interlaceAllowed = FALSE; 
    clockRanges->doubleScanAllowed = TRUE;

    /* Subtract memory for HW cursor */
    if (!nPtr->swCursor)
	nPtr->NeoCursorMem = CursorMem;
    else
	nPtr->NeoCursorMem = 0;
    apertureSize = nPtr->NeoFbMapSize - nPtr->NeoCursorMem;
    /*
     * For external displays, limit the width to 1024 pixels or less.
     */
    i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			  pScrn->display->modes, clockRanges,
			  NULL, 256, maxWidth,(8 * pScrn->bitsPerPixel),/*���*/
			  128, 2048, pScrn->display->virtualX,
			  pScrn->display->virtualY, apertureSize,
			  LOOKUP_BEST_REFRESH);

    if (i == -1)
	RETURN;

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	RETURN;
    }

    /*
     * Set the CRTC parameters for all of the modes based on the type
     * of mode, and the chipset's interlace requirements.
     *
     * Calling this is required if the mode->Crtc* values are used by the
     * driver and if the driver doesn't provide code to set them.  They
     * are not pre-initialised at all.
     */
    xf86SetCrtcForModes(pScrn, 0); 
 
    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* If monitor resolution is set on the command line, use it */
    xf86SetDpi(pScrn, 0, 0);

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
    }
    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	RETURN;
    }

    xf86LoaderReqSymbols(reqSym, NULL);

    if (!nPtr->noLinear) {
	if (!xf86LoadSubModule(pScrn, "xaa")) 
	    RETURN;
	xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    if (nPtr->shadowFB) {
	if (!xf86LoadSubModule(pScrn, "shadowfb")) {
	    RETURN;
	}
	xf86LoaderReqSymLists(shadowSymbols, NULL);
    }
    
    if (!nPtr->swCursor) {
	if (!xf86LoadSubModule(pScrn, "ramdac"))
	    RETURN;
	xf86LoaderReqSymLists(ramdacSymbols, NULL);
    }

    return TRUE;
}
#undef RETURN

/* Mandatory */
static Bool
NEOEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    NEOPtr nPtr = NEOPTR(pScrn);
    
    /* Should we re-save the text mode on each VT enter? */
    if(!neoModeInit(pScrn, pScrn->currentMode))
      return FALSE;
    if (nPtr->NeoHWCursorShown) 
	NeoShowCursor(pScrn);
    NEOAdjustFrame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);    
    return TRUE;
}

/* Mandatory */
static void
NEOLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    NEOPtr nPtr = NEOPTR(pScrn);
    
#if 0
#ifdef XFreeXDGA
    if (vga256InfoRec.directMode&XF86DGADirectGraphics && !enter) {
	/* 
	 * Disable HW cursor. I hope DGA can't call this function twice
	 * in a row, without calling EnterVT in between. Otherwise the
	 * effect will be to hide the cursor, perhaps permanently!!
	 */
    if (nPtr->NeoHWCursorShown) 
	NeoHideCursor(pScrn);
	return;
    }
#endif
#endif

    /* Invalidate the cached acceleration registers */
    if (nPtr->NeoHWCursorShown) 
	NeoHideCursor(pScrn);
    neoRestore(pScrn, &(VGAHWPTR(pScrn))->SavedReg, &nPtr->NeoSavedReg, TRUE);
    neoLock(pScrn);
}

/* Mandatory */
static Bool
NEOScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    NEOPtr nPtr;
    NEOACLPtr nAcl;
    int ret;
    VisualPtr visual;
    int allocatebase, freespace, currentaddr;
    unsigned int racflag = RAC_FB;
    unsigned char *FBStart;
    int height, width, displayWidth;
    
    /*
     * we need to get the ScrnInfoRec for this screen, so let's allocate
     * one first thing
     */
    pScrn = xf86Screens[pScreen->myNum];
    nPtr = NEOPTR(pScrn);
    nAcl = NEOACLPTR(pScrn);

    hwp = VGAHWPTR(pScrn);
    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */
    /* Map the VGA memory */
    if (!vgaHWMapMem(pScrn))
	return FALSE;

    /* Map the Neo memory and possible MMIO areas */
    if (!neoMapMem(pScrn))
	return FALSE;

    /*
     * next we save the current state and setup the first mode
     */
    neoSave(pScrn);

    if (!neoModeInit(pScrn,pScrn->currentMode))
	return FALSE;
    vgaHWSaveScreen(pScreen,SCREEN_SAVER_ON);
    NEOAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    
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
     * support TrueColor and not DirectColor.  To deal with this, call
     * miSetVisualTypes for each visual supported.
     */

    if (pScrn->depth > 8) {
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
     * Temporarily set the global defaultColorVisualClass to make
     * cfbInitVisuals do what we want.
     */
#if 0
    savedDefaultVisualClass = xf86GetDefaultColorVisualClass();
    xf86SetDefaultColorVisualClass(pScrn->defaultVisual);
#endif

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */
    displayWidth = pScrn->displayWidth;
    if (nPtr->rotate) {
	height = pScrn->virtualX;
	width = pScrn->virtualY;
    } else {
	width = pScrn->virtualX;
	height = pScrn->virtualY;
    }
    
    if(nPtr->shadowFB) {
	nPtr->ShadowPitch = BitmapBytePad(pScrn->bitsPerPixel * width);
	nPtr->ShadowPtr = xalloc(nPtr->ShadowPitch * height);
	displayWidth = nPtr->ShadowPitch / (pScrn->bitsPerPixel >> 3);
	FBStart = nPtr->ShadowPtr;
    } else {
	nPtr->ShadowPtr = NULL;
	FBStart = nPtr->NeoFbBase;
    }

    switch (pScrn->bitsPerPixel) {
    case 8:
	ret = cfbScreenInit(pScreen, FBStart,
			    width,height,
			    pScrn->xDpi, pScrn->yDpi,
			    displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, FBStart,
			      width,height,
			      pScrn->xDpi, pScrn->yDpi,
			      displayWidth);
	break;
    case 24:
	if (pix24bpp == 24)
	    ret = cfb24ScreenInit(pScreen, FBStart,
				  width,height,
				  pScrn->xDpi, pScrn->yDpi,
				  displayWidth);
	else
	    ret = cfb24_32ScreenInit(pScreen, FBStart,
				     width,height,
				     pScrn->xDpi, pScrn->yDpi,
				     displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in NEOScreenInit\n",
		   pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }

#if 0
    xf86SetDefaultColorVisualClass(savedDefaultVisualClass);
#endif
    if (!ret)
	return FALSE;

    if (pScrn->depth > 8) {
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
    
    nPtr->NeoHWCursorShown = FALSE;
    nPtr->NeoHWCursorInitialized = FALSE;
    nAcl->UseHWCursor = FALSE;
    
    if (nPtr->noLinear) {
	miBankInfoPtr pBankInfo;

	/* Setup the vga banking variables */
	pBankInfo = (miBankInfoPtr)xnfcalloc(sizeof(miBankInfoRec),1);
	if (pBankInfo == NULL)
	    return FALSE;
	
	pBankInfo->pBankA = hwp->Base;
	pBankInfo->pBankB = (unsigned char *)hwp->Base + 0x10000;
	pBankInfo->BankSize = 0x10000;
	pBankInfo->nBankDepth = pScrn->depth;
	
	pBankInfo->SetSourceBank = (miBankProcPtr)NEOSetRead;
	pBankInfo->SetDestinationBank =
	    (miBankProcPtr)NEOSetWrite;
	pBankInfo->SetSourceAndDestinationBanks =
	    (miBankProcPtr)NEOSetReadWrite;
	if (!miInitializeBanking(pScreen, pScrn->virtualX, pScrn->virtualY,
				 pScrn->displayWidth, pBankInfo)) {
	    xfree(pBankInfo);
	    pBankInfo = NULL;
	    return FALSE;
	}
	xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Using nonlinear mode\n");
	xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Using software cursor\n");

	miInitializeBackingStore(pScreen);
	xf86SetBackingStore(pScreen);

	/* Initialise cursor functions */
	miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

    } else {
	nAcl->CursorAddress = -1;
	nAcl->cacheStart = -1;
	nAcl->cacheEnd = -1;
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,
		   "Using linear framebuffer at: 0x%08lX\n",
		   nPtr->NeoLinearAddr);
	/* Setup pointers to free space in video ram */
	allocatebase = (pScrn->videoRam << 10);
	freespace = allocatebase - pScrn->displayWidth * 
	    pScrn->virtualY * (pScrn->bitsPerPixel >> 3);
	currentaddr = allocatebase;
	xf86DrvMsg(scrnIndex, X_PROBED,
		   "%d bytes off-screen memory available\n", freespace);

	if (nPtr->swCursor || nPtr->noMMIO) {
	    xf86DrvMsg(scrnIndex, X_CONFIG,
		       "Using Software Cursor.\n");
	} else if (nPtr->NeoCursorMem <= freespace) {
	    currentaddr -= nPtr->NeoCursorMem;
	    freespace  -= nPtr->NeoCursorMem;
	    /* alignment */
	    freespace  -= currentaddr & 0x3FF;
	    currentaddr &= 0xfffffc00;
	    nAcl->CursorAddress = currentaddr;
	    xf86DrvMsg(scrnIndex, X_INFO,
		       "Using H/W Cursor.\n"); 
	} else xf86DrvMsg(scrnIndex, X_ERROR,
			  "Too little space for H/W cursor.\n");
	
	/* Setup the acceleration primitives */
	if (!nPtr->noAccel) {
	    nAcl->cacheStart = currentaddr - freespace;
	    nAcl->cacheEnd = currentaddr;
	    freespace = 0;
	    if (nAcl->cacheStart >= nAcl->cacheEnd) {
		xf86DrvMsg(scrnIndex, X_ERROR,
			   "Too little space for pixmap cache.\n");
	    }
	    switch(nPtr->NeoChipset) {
	    case NM2070 :
		Neo2070AccelInit(pScreen);
		break;
	    case NM2090 :
	    case NM2093 :
		Neo2090AccelInit(pScreen);
		break;
	    case NM2097 :
	    case NM2160 :
		Neo2097AccelInit(pScreen);
		break;
	    case NM2200 :
	        Neo2200AccelInit(pScreen);
		break;
	    }
	    xf86DrvMsg(pScrn->scrnIndex,X_INFO,"Acceleration Initialized\n");
	} 
    
	miInitializeBackingStore(pScreen);
	xf86SetBackingStore(pScreen);
	
	/* Initialise cursor functions */
	miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

	if (nAcl->CursorAddress != -1) {
	    /* HW cursor functions */
	    if (!NeoCursorInit(pScreen)) {
		xf86DrvMsg(scrnIndex, X_ERROR,
			   "Hardware cursor initialization failed\n");
		return FALSE;
	    }
	    nAcl->UseHWCursor = TRUE;
	    nPtr->NeoHWCursorInitialized = TRUE;
	}
    }

    if (nPtr->shadowFB) {
	RefreshAreaFuncPtr refreshArea = neoRefreshArea;

	if(nPtr->rotate) {
	    if (!nPtr->PointerMoved) {
		nPtr->PointerMoved = pScrn->PointerMoved;
		pScrn->PointerMoved = neoPointerMoved;
	    }
	    
	   switch(pScrn->bitsPerPixel) {
	   case 8:	refreshArea = neoRefreshArea8;	break;
	   case 16:	refreshArea = neoRefreshArea16;	break;
	   case 24:	refreshArea = neoRefreshArea24;	break;
	   case 32:	refreshArea = neoRefreshArea32;	break;
	   }
	}

	ShadowFBInit(pScreen, refreshArea);
    }
    
    /* Initialise default colourmap */
    if(!miCreateDefColormap(pScreen))
	return FALSE;

    if (!vgaHWHandleColormaps(pScreen))
	return FALSE;

    if (pScrn->bitsPerPixel == 8)
        racflag |= RAC_COLORMAP;
    if (nPtr->NeoHWCursorInitialized)
        racflag |= RAC_CURSOR;

    pScrn->racIoFlags = pScrn->racMemFlags = racflag;

    pScreen->SaveScreen = vgaHWSaveScreen;

#ifdef DPMSExtension
    /* Setup DPMS mode */
    if (nPtr->NeoChipset != NM2070)
	xf86DPMSInit(pScreen, (DPMSSetProcPtr)NeoDisplayPowerManagementSet,
		     0);
#endif

    /* Wrap the current CloseScreen function */
    nPtr->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = NEOCloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    return TRUE;
}

/* Mandatory */
static Bool
NEOSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return neoModeInit(xf86Screens[scrnIndex], mode);
}

/* Mandatory */
static void
NEOAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn;
    NEOPtr nPtr;
    vgaHWPtr hwp;
    int oldExtCRTDispAddr;
    int Base; 

    pScrn = xf86Screens[scrnIndex];
    Base = (y * pScrn->displayWidth + x) >> 2;
    hwp = VGAHWPTR(pScrn);
    nPtr = NEOPTR(pScrn);
    /* Scale Base by the number of bytes per pixel. */

    switch (pScrn->bitsPerPixel) {
    case  8 :
	break;
    case 15 :
    case 16 :
	Base *= 2;
	break;
    case 24 :
	Base *= 3;
	break;
    default :
	break;
    }
    /*
     * These are the generic starting address registers.
     */
    VGAwCR(0x0C, (Base & 0x00FF00) >> 8);
    VGAwCR(0x0D, (Base & 0x00FF));

    /*
     * Make sure we don't clobber some other bits that might already
     * have been set. NOTE: NM2200 has a writable bit 3, but it shouldn't
     * be needed.
     */
    oldExtCRTDispAddr = VGArGR(0x0E);
    VGAwGR(0x0E,(((Base >> 16) & 0x07) | (oldExtCRTDispAddr & 0xf8)));
#if 0
    /*
     * This is a workaround for a higher level bug that causes the cursor
     * to be at the wrong position after a virtual screen resolution change
     */
    if (nPtr->NeoHWCursorInitialized) { /*��� do we still need this?*/
	NeoRepositionCursor();
    }
#endif
}

/* Mandatory */
static Bool
NEOCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    NEOPtr nPtr = NEOPTR(pScrn);

    if(pScrn->vtSema){
	if (nPtr->NeoHWCursorShown)
	    NeoHideCursor(pScrn);
	neoRestore(pScrn, &(VGAHWPTR(pScrn))->SavedReg, &nPtr->NeoSavedReg, TRUE);
	neoLock(pScrn);
	neoUnmapMem(pScrn);
    }
    if (nPtr->AccelInfoRec)
	XAADestroyInfoRec(nPtr->AccelInfoRec);
    if (nPtr->CursorInfo)
	xf86DestroyCursorInfoRec(nPtr->CursorInfo);
    if (nPtr->ShadowPtr)
	xfree(nPtr->ShadowPtr);

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = nPtr->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}

/* Optional */
static void
NEOFreeScreen(int scrnIndex, int flags)
{
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
	vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    NEOFreeRec(xf86Screens[scrnIndex]);
}

/* Optional */
static int
NEOValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    NEOPtr nPtr = NEOPTR(pScrn);

    /*
     * Limit the modes to just those allowed by the various NeoMagic
     * chips.  
     */

    if (nPtr->overrideValidate) {
	xf86DrvMsg(scrnIndex, X_WARNING, "display mode validation disabled\n");
    } else {
	/*
	 * When the LCD is active, only allow modes that are (1) equal to
	 * or smaller than the size of the panel and (2) are one of the
	 * following sizes: 1024x768, 800x600, 640x480.
	 */
	if (nPtr->internDisp || !nPtr->externDisp) {
	    /* Is the mode larger than the LCD panel? */
	    if ((mode->HDisplay > nPtr->NeoPanelWidth) ||
		(mode->VDisplay > nPtr->NeoPanelHeight)) {
		xf86DrvMsg(scrnIndex,X_INFO, "Removing mode (%dx%d) "
			   "larger than the LCD panel (%dx%d)\n",
			   mode->HDisplay,
			   mode->VDisplay,
			   nPtr->NeoPanelWidth,
			   nPtr->NeoPanelHeight);
		return(MODE_BAD);
	    }

	    /* Is the mode one of the acceptable sizes? */
	    switch (mode->HDisplay) {
	    case 1280:
		if (mode->VDisplay == 1024)
		    return(MODE_OK);
		break;
	    case 1024 :
		if (mode->VDisplay == 768)
		    return(MODE_OK);
		break;
	    case  800 :
		if (mode->VDisplay == 600)
		    return(MODE_OK);
		break;
	    case  640 :
		if (mode->VDisplay == 480)
		    return(MODE_OK);
		break;
	    }

	    xf86DrvMsg(scrnIndex, X_INFO, "Removing mode (%dx%d) that won't "
		       "display properly on LCD\n",
		       mode->HDisplay,
		       mode->VDisplay);
	    return(MODE_BAD);
	}
    }
    return(MODE_OK);
}

static void
neoLock(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    
    VGAwGR(0x09,0x00);
    vgaHWLock(hwp);
}

static void
neoUnlock(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    
    vgaHWUnlock(hwp);
    VGAwGR(0x09,0x26);
}

static Bool
neoMapMem(ScrnInfoPtr pScrn)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    if (!nPtr->noLinear) {
	if (!nPtr->noMMIO) {
	    if (nPtr->pEnt->location.type == BUS_PCI)
		nPtr->NeoMMIOBase =
		    xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
				  nPtr->PciTag, nPtr->NeoMMIOAddr,
				  0x200000L);
	    else
		nPtr->NeoMMIOBase =
		    xf86MapVidMem(pScrn->scrnIndex,
				  VIDMEM_MMIO, nPtr->NeoMMIOAddr,
				  0x200000L);
	}
	if (nPtr->NeoMMIOBase == NULL)
	    return FALSE;

	if (nPtr->pEnt->location.type == BUS_PCI)
	    nPtr->NeoFbBase =
		xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
			      nPtr->PciTag,
			      (unsigned long)nPtr->NeoLinearAddr,
			      nPtr->NeoFbMapSize);
	else
	    nPtr->NeoFbBase =
		xf86MapVidMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
			      (unsigned long)nPtr->NeoLinearAddr,
			      nPtr->NeoFbMapSize);
	if (nPtr->NeoFbBase == NULL)
	    return FALSE;
    } else {
	/* In paged mode Base is the VGA window at 0xA0000 */
	nPtr->NeoFbBase = hwp->Base;
    }
    return TRUE;
}

/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
neoUnmapMem(ScrnInfoPtr pScrn)
{
    NEOPtr nPtr = NEOPTR(pScrn);

    if (!nPtr->noLinear) {
      xf86UnMapVidMem(pScrn->scrnIndex, (pointer)nPtr->NeoMMIOBase, 0x200000L);
      nPtr->NeoMMIOBase = NULL;
      xf86UnMapVidMem(pScrn->scrnIndex, (pointer)nPtr->NeoFbBase,
		    nPtr->NeoFbMapSize); 
    }
    nPtr->NeoFbBase = NULL;
    
    return TRUE;
}

static void
neoSave(ScrnInfoPtr pScrn)
{
    vgaRegPtr VgaSave = &VGAHWPTR(pScrn)->SavedReg;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    NEOPtr nPtr = NEOPTR(pScrn);
    NeoRegPtr save;
    int i;

    save = &nPtr->NeoSavedReg;

    VGAwGR(0x09,0x26);
    /*
     * Whatever code is needed to get back to bank zero goes here.
     */
    VGAwGR(0x15,0x00);
    
    /* get generic registers */
    vgaHWSave(pScrn, VgaSave, VGA_SR_ALL);

    /*
     * The port I/O code necessary to read in the extended registers 
     * into the fields of the vgaNeoRec structure goes here.
     */

    save->GeneralLockReg = VGArGR(0x0A);
    
    save->ExtCRTDispAddr = VGArGR(0x0E);
    if (nPtr->NeoChipset != NM2070) {
	save->ExtCRTOffset = VGArGR(0x0F);
    }
    save->SysIfaceCntl1 = VGArGR(0x10);
    save->SysIfaceCntl2 = VGArGR(0x11);
    save->SingleAddrPage = VGArGR(0x15);
    save->DualAddrPage = VGArGR(0x16);
    save->PanelDispCntlReg1 = VGArGR(0x20);
    save->PanelDispCntlReg2 = VGArGR(0x25);
    save->PanelDispCntlReg3 = VGArGR(0x30);
    save->PanelVertCenterReg1 = VGArGR(0x28);
    save->PanelVertCenterReg2 = VGArGR(0x29);
    save->PanelVertCenterReg3 = VGArGR(0x2A);
    if (nPtr->NeoChipset != NM2070) {
        save->PanelVertCenterReg4 = VGArGR(0x32);
	save->PanelHorizCenterReg1 = VGArGR(0x33);
	save->PanelHorizCenterReg2 = VGArGR(0x34);
	save->PanelHorizCenterReg3 = VGArGR(0x35);
    }
    if (nPtr->NeoChipset == NM2160) {
        save->PanelHorizCenterReg4 = VGArGR(0x36);
    }
    if (nPtr->NeoChipset == NM2200) {
	save->PanelHorizCenterReg4 = VGArGR(0x36);
	save->PanelVertCenterReg5  = VGArGR(0x37);
	save->PanelHorizCenterReg5 = VGArGR(0x38);
    }
    save->ExtColorModeSelect = VGArGR(0x90);
    save->VCLK3NumeratorLow = VGArGR(0x9B);
    if (nPtr->NeoChipset == NM2200)
	save->VCLK3NumeratorHigh = VGArGR(0x8F);
    save->VCLK3Denominator = VGArGR(0x9F);

    if (save->reg == NULL)
        save->reg = (regSavePtr)xnfcalloc(sizeof(regSaveRec), 1);
    else
        ErrorF("WARNING: Non-NULL reg in NeoSave: reg=0x%08X\n", save->reg);

    save->reg->CR[0x25] = VGArCR(0x25);
    save->reg->CR[0x2F] = VGArCR(0x2F);
    for (i = 0x40; i <= 0x59; i++) {
	save->reg->CR[i] = VGArCR(i);
    }
    for (i = 0x60; i <= 0x69; i++) {
	save->reg->CR[i] = VGArCR(i);
    }
    for (i = 0x70; i <= NEO_EXT_CR_MAX; i++) {
	save->reg->CR[i] = VGArCR(i);
    }

    for (i = 0x0A; i <= NEO_EXT_GR_MAX; i++) {
        save->reg->GR[i] = VGArGR(i);
    }
}

/*
 * neoProgramShadowRegs
 *
 * Setup the shadow registers to their default values.  The NeoSave
 * routines will restore the proper values on server exit.
 */
static void
neoProgramShadowRegs(ScrnInfoPtr pScrn, vgaRegPtr VgaReg, NeoRegPtr restore)
{
    int i;
    Bool noProgramShadowRegs;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    NEOPtr nPtr = NEOPTR(pScrn);
    
    /*
     * Convoluted logic for shadow register programming.
     *
     * As far as we know, shadow programming is needed for the 2070,
     * but not in stretched modes.  Special case this code.
     */
    switch (nPtr->NeoChipset) {
    case NM2070:
	/* Program the shadow regs by default */
	noProgramShadowRegs = FALSE;
	if (!nPtr->progLcdRegs)
	    noProgramShadowRegs = TRUE;

	if (restore->PanelDispCntlReg2 & 0x84) {
	    /* Don't program by default if in stretch mode */
	    noProgramShadowRegs = TRUE;
	    if (nPtr->progLcdStretch)
		noProgramShadowRegs = FALSE;
	}
	break;
    case NM2090:
    case NM2093:
    case NM2097:
    case NM2160:
    case NM2200:
    default:
	/* Don't program the shadow regs by default */
	noProgramShadowRegs = TRUE;
	if (nPtr->progLcdRegs)
	    noProgramShadowRegs = FALSE;

	if (restore->PanelDispCntlReg2 & 0x84) {
	    /* Only change the behavior if an option is set */
	    if (nPtr->progLcdStrechOpt)
		noProgramShadowRegs = !nPtr->progLcdStretch;
	}
	break;
    }

    if (noProgramShadowRegs) {
	if (nPtr->NeoSavedReg.reg){
	    for (i = 0x40; i <= 0x59; i++) {
		VGAwCR(i, nPtr->NeoSavedReg.reg->CR[i]);
	    }
	    for (i = 0x60; i <= 0x64; i++) {
		VGAwCR(i, nPtr->NeoSavedReg.reg->CR[i]);
	    } 
	} 
    } else {
	/*
	 * Program the shadow regs based on the panel width.  This works
	 * fine for normal sized panels, but what about the odd ones like
	 * the Libretto 100 which has an 800x480 panel???
	 */
	switch (nPtr->NeoPanelWidth) {
	case 640 :
	    VGAwCR(0x40,0x5F);
	    VGAwCR(0x41,0x50);
	    VGAwCR(0x42,0x02);
	    VGAwCR(0x43,0x55);
	    VGAwCR(0x44,0x81);
	    VGAwCR(0x45,0x0B);
	    VGAwCR(0x46,0x2E);
	    VGAwCR(0x47,0xEA);
	    VGAwCR(0x48,0x0C);
	    VGAwCR(0x49,0xE7);
	    VGAwCR(0x4A,0x04);
	    VGAwCR(0x4B,0x2D);
	    VGAwCR(0x4C,0x28);
	    VGAwCR(0x4D,0x90);
	    VGAwCR(0x4E,0x2B);
	    VGAwCR(0x4F,0xA0);
	    break;
	case 800 :
	    VGAwCR(0x40,0x7F);
	    VGAwCR(0x41,0x63);
	    VGAwCR(0x42,0x02);
	    VGAwCR(0x43,0x6C);
	    VGAwCR(0x44,0x1C);
	    VGAwCR(0x45,0x72);
	    VGAwCR(0x46,0xE0);
	    VGAwCR(0x47,0x58);
	    VGAwCR(0x48,0x0C);
	    VGAwCR(0x49,0x57);
	    VGAwCR(0x4A,0x73);
	    VGAwCR(0x4B,0x3D);
	    VGAwCR(0x4C,0x31);
	    VGAwCR(0x4D,0x01);
	    VGAwCR(0x4E,0x36);
	    VGAwCR(0x4F,0x1E);
	    if (nPtr->NeoChipset != NM2070) {
		VGAwCR(0x50,0x6B);
		VGAwCR(0x51,0x4F);
		VGAwCR(0x52,0x0E);
		VGAwCR(0x53,0x58);
		VGAwCR(0x54,0x88);
		VGAwCR(0x55,0x33);
		VGAwCR(0x56,0x27);
		VGAwCR(0x57,0x16);
		VGAwCR(0x58,0x2C);
		VGAwCR(0x59,0x94);
	    }
	    break;
	case 1024 :
	    VGAwCR(0x40,0xA3);
	    VGAwCR(0x41,0x7F);
	    VGAwCR(0x42,0x06);
	    VGAwCR(0x43,0x85);
	    VGAwCR(0x44,0x96);
	    VGAwCR(0x45,0x24);
	    VGAwCR(0x46,0xE5);
	    VGAwCR(0x47,0x02);
	    VGAwCR(0x48,0x08);
	    VGAwCR(0x49,0xFF);
	    VGAwCR(0x4A,0x25);
	    VGAwCR(0x4B,0x4F);
	    VGAwCR(0x4C,0x40);
	    VGAwCR(0x4D,0x00);
	    VGAwCR(0x4E,0x44);
	    VGAwCR(0x4F,0x0C);
	    VGAwCR(0x50,0x7A);
	    VGAwCR(0x51,0x56);
	    VGAwCR(0x52,0x00);
	    VGAwCR(0x53,0x5D);
	    VGAwCR(0x54,0x0E);
	    VGAwCR(0x55,0x3B);
	    VGAwCR(0x56,0x2B);
	    VGAwCR(0x57,0x00);
	    VGAwCR(0x58,0x2F);
	    VGAwCR(0x59,0x18);
	    VGAwCR(0x60,0x88);
	    VGAwCR(0x61,0x63);
	    VGAwCR(0x62,0x0B);
	    VGAwCR(0x63,0x69);
	    VGAwCR(0x64,0x1A);
	    break;
	case 1280:
#ifdef NOT_DONE
	    VGAwCR(0x40,0x?? );
            .
	    .
	    .
	    VGAwCR(0x64,0x?? );
	    break;
#else
            /* Probe should prevent this case for now */
            FatalError("1280 panel support incomplete\n");
#endif
	}
    }
}

static void 
neoRestore(ScrnInfoPtr pScrn, vgaRegPtr VgaReg, NeoRegPtr restore,
	     Bool restoreFonts)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    unsigned char temp;
    int i;

    vgaHWProtect(pScrn,TRUE);		/* Blank the screen */

    VGAwGR(0x09,0x26);
    
    /* Init the shadow registers if necessary */
    neoProgramShadowRegs(pScrn, VgaReg, restore);

    VGAwGR(0x15,0x00);

    VGAwGR(0x0A,restore->GeneralLockReg);

    /*
     * The color mode needs to be set before calling vgaHWRestore
     * to ensure the DAC is initialized properly.
     *
     * NOTE: Make sure we don't change bits make sure we don't change
     * any reserved bits.
     */
    temp = VGArGR(0x90);
    switch (nPtr->NeoChipset) {
    case NM2070 :
	temp &= 0xF0; /* Save bits 7:4 */
	temp |= (restore->ExtColorModeSelect & ~0xF0);
	break;
    case NM2090 :
    case NM2093 :
    case NM2097 :
    case NM2160 :
    case NM2200 :
	temp &= 0x70; /* Save bits 6:4 */
	temp |= (restore->ExtColorModeSelect & ~0x70);
	break;
    }
    VGAwGR(0x90,temp);
    
    /*
     * Disable horizontal and vertical graphics and text expansions so
     * that vgaHWRestore works properly.
     */
    temp = VGArGR(0x25);
    temp &= 0x39;
    VGAwGR(0x25, temp);

    /*
     * Sleep for 200ms to make sure that the two operations above have
     * had time to take effect.
     */
    usleep(200000);

    /*
     * This function handles restoring the generic VGA registers.  */
    vgaHWRestore(pScrn, VgaReg,
		 VGA_SR_MODE | VGA_SR_CMAP | (restoreFonts ? VGA_SR_FONTS : 0));

    VGAwGR(0x0E, restore->ExtCRTDispAddr);
    VGAwGR(0x0F, restore->ExtCRTOffset);
    temp = VGArGR(0x10);
    temp &= 0x0F; /* Save bits 3:0 */
    temp |= (restore->SysIfaceCntl1 & ~0x0F);
    VGAwGR(0x10, temp);

    VGAwGR(0x11, restore->SysIfaceCntl2);
    VGAwGR(0x15, restore->SingleAddrPage);
    VGAwGR(0x16, restore->DualAddrPage);
    temp = VGArGR(0x20);
    switch (nPtr->NeoChipset) {
    case NM2070 :
	temp &= 0xFC; /* Save bits 7:2 */
	temp |= (restore->PanelDispCntlReg1 & ~0xFC);
	break;
    case NM2090 :
    case NM2093 :
    case NM2097 :
    case NM2160 :
	temp &= 0xDC; /* Save bits 7:6,4:2 */
	temp |= (restore->PanelDispCntlReg1 & ~0xDC);
	break;
    case NM2200 :
	temp &= 0x98; /* Save bits 7,4:3 */
	temp |= (restore->PanelDispCntlReg1 & ~0x98);
	break;
    }
    VGAwGR(0x20, temp);
    temp = VGArGR(0x25);
    temp &= 0x38; /* Save bits 5:3 */
    temp |= (restore->PanelDispCntlReg2 & ~0x38);
    VGAwGR(0x25, temp);

    if (nPtr->NeoChipset != NM2070) {
	temp = VGArGR(0x30);
	temp &= 0xEF; /* Save bits 7:5 and bits 3:0 */
	temp |= (restore->PanelDispCntlReg3 & ~0xEF);
	VGAwGR(0x30, temp);
    }

    VGAwGR(0x28, restore->PanelVertCenterReg1);
    VGAwGR(0x29, restore->PanelVertCenterReg2);
    VGAwGR(0x2a, restore->PanelVertCenterReg3);

    if (nPtr->NeoChipset != NM2070) {
	VGAwGR(0x32, restore->PanelVertCenterReg4);
	VGAwGR(0x33, restore->PanelHorizCenterReg1);
	VGAwGR(0x34, restore->PanelHorizCenterReg2);
	VGAwGR(0x35, restore->PanelHorizCenterReg3);
    }

    if (nPtr->NeoChipset == NM2160) {
	VGAwGR(0x36, restore->PanelHorizCenterReg4);
    }

    if (nPtr->NeoChipset == NM2200) {
        VGAwGR(0x36, restore->PanelHorizCenterReg4);
        VGAwGR(0x37, restore->PanelVertCenterReg5);
        VGAwGR(0x38, restore->PanelHorizCenterReg5);
    }

    /* Program VCLK3 if needed. */
    if (restore->ProgramVCLK) {
	VGAwGR(0x9B, restore->VCLK3NumeratorLow);
	if (nPtr->NeoChipset == NM2200) {
	    temp = VGArGR(0x8F);
	    temp &= 0x0F; /* Save bits 3:0 */
	    temp |= (restore->VCLK3NumeratorHigh & ~0x0F);
	    VGAwGR(0x8F, temp);
	}
	VGAwGR(0x9F, restore->VCLK3Denominator);
    }

    if (restore->reg) {
	VGAwCR(0x25,restore->reg->CR[0x25]);
	VGAwCR(0x2F,restore->reg->CR[0x2F]);
	for (i = 0x40; i <= 0x59; i++) {
	    VGAwCR(i, restore->reg->CR[i]);
	}
	for (i = 0x60; i <= 0x69; i++) {
	    VGAwCR(i, restore->reg->CR[i]);
	}
	for (i = 0x70; i <= NEO_EXT_CR_MAX; i++) {
	    VGAwCR(i, restore->reg->CR[i]);
	}

	for (i = 0x0a; i <= 0x3f; i++) {
	    VGAwGR(i, restore->reg->GR[i]);
	}
	for (i = 0x90; i <= NEO_EXT_GR_MAX; i++) {
	    VGAwGR(i, restore->reg->GR[i]);
	}
	xfree(restore->reg);
	restore->reg = NULL;
    }
    /* Program vertical extension register */
    if (nPtr->NeoChipset == NM2200) {
	VGAwCR(0x70, restore->VerticalExt);
    }


    vgaHWProtect(pScrn, FALSE);		/* Turn on screen */

}
    
static Bool
neoModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    NEOACLPtr nAcl = NEOACLPTR(pScrn);
    int i;
    int hoffset, voffset;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    NeoRegPtr NeoNew = &nPtr->NeoModeReg;
    vgaRegPtr NeoStd = &hwp->ModeReg;

    neoUnlock(pScrn);

    /*
     * This will allocate the datastructure and initialize all of the
     * generic VGA registers.
     */
    if (!vgaHWInit(pScrn, mode))
	return(FALSE);

    /*
     * Several registers need to be corrected from the default values
     * assigned by vgaHWinit().
     */
    pScrn->vtSema = TRUE;

    /*
     * The default value assigned by vgaHW.c is 0x41, but this does
     * not work for NeoMagic.
     */
    NeoStd->Attribute[16] = 0x01;

    switch (pScrn->bitsPerPixel) {
    case  8 :
	NeoStd->CRTC[0x13] = pScrn->displayWidth >> 3;
	NeoNew->ExtCRTOffset   = pScrn->displayWidth >> 11;
	NeoNew->ExtColorModeSelect = 0x11;
	break;
    case 15 :
    case 16 :
	if ((pScrn->weight.red == 5) &&
	    (pScrn->weight.green == 5) &&
	    (pScrn->weight.blue == 5)) {
	    /* 15bpp */
	    for (i = 0; i < 64; i++) {
		NeoStd->DAC[i*3+0] = i << 1;
		NeoStd->DAC[i*3+1] = i << 1;
		NeoStd->DAC[i*3+2] = i << 1;
	    }
	    NeoNew->ExtColorModeSelect = 0x12;
	} else {
	    /* 16bpp */
	    for (i = 0; i < 64; i++) {
		NeoStd->DAC[i*3+0] = i << 1;
		NeoStd->DAC[i*3+1] = i;
		NeoStd->DAC[i*3+2] = i << 1;
	    }
	    NeoNew->ExtColorModeSelect = 0x13;
	}
	/* 15bpp & 16bpp */
	NeoStd->CRTC[0x13] = pScrn->displayWidth >> 2;
	NeoNew->ExtCRTOffset   = pScrn->displayWidth >> 10;
	break;
    case 24 :
	for (i = 0; i < 256; i++) {
	    NeoStd->DAC[i*3+0] = i;
	    NeoStd->DAC[i*3+1] = i;
	    NeoStd->DAC[i*3+2] = i;
	}
	NeoStd->CRTC[0x13] = (pScrn->displayWidth * 3) >> 3;
	NeoNew->ExtCRTOffset   = (pScrn->displayWidth * 3) >> 11;
	NeoNew->ExtColorModeSelect = 0x14;
	break;
    default :
	break;
    }
	
    NeoNew->ExtCRTDispAddr = 0x10;

    /* Vertical Extension */
    NeoNew->VerticalExt = (((mode->CrtcVTotal -2) & 0x400) >> 10 )
      | (((mode->CrtcVDisplay -1) & 0x400) >> 9 )
        | (((mode->CrtcVSyncStart) & 0x400) >> 8 )
          | (((mode->CrtcVSyncStart) & 0x400) >> 7 );

    /* Disable read/write bursts if requested. */
    if (nPtr->onPciBurst) {
	NeoNew->SysIfaceCntl1 = 0x30; 
    } else {
	NeoNew->SysIfaceCntl1 = 0x00; 
    }

    /* If they are used, enable linear addressing and/or enable MMIO. */
    NeoNew->SysIfaceCntl2 = 0x00;
    if (!nPtr->noLinear)
	NeoNew->SysIfaceCntl2 |= 0x80;
    if (!nPtr->noMMIO)
	NeoNew->SysIfaceCntl2 |= 0x40;

    /* Enable any user specified display devices. */
    NeoNew->PanelDispCntlReg1 = 0x00;
    if (nPtr->internDisp) {
	NeoNew->PanelDispCntlReg1 |= 0x02;
    }
    if (nPtr->externDisp) {
	NeoNew->PanelDispCntlReg1 |= 0x01;
    }
#if 0
    /*
     * This was replaced: if no devices are specified take the
     * probed settings. If the probed settings are bogus fallback
     * to internal only.
     */
    /* If the user did not specify any display devices, then... */
    if (NeoNew->PanelDispCntlReg1 == 0x00) {
	/* Default to internal (i.e., LCD) only. */
	NeoNew->PanelDispCntlReg1 |= 0x02;
    }
#endif
    /* If we are using a fixed mode, then tell the chip we are. */
    switch (mode->HDisplay) {
    case 1280:
	NeoNew->PanelDispCntlReg1 |= 0x60;
        break;
    case 1024:
	NeoNew->PanelDispCntlReg1 |= 0x40;
        break;
    case 800:
	NeoNew->PanelDispCntlReg1 |= 0x20;
        break;
    case 640:
    default:
        break;
    }

    /* Setup shadow register locking. */
    switch (NeoNew->PanelDispCntlReg1 & 0x03) {
    case 0x01 : /* External CRT only mode: */
	NeoNew->GeneralLockReg = 0x00;
	/* We need to program the VCLK for external display only mode. */
	NeoNew->ProgramVCLK = TRUE;
	break;
    case 0x02 : /* Internal LCD only mode: */
    case 0x03 : /* Simultaneous internal/external (LCD/CRT) mode: */
	NeoNew->GeneralLockReg = 0x01;
	/* Don't program the VCLK when using the LCD. */
	NeoNew->ProgramVCLK = FALSE;
	break;
    }

    /*
     * If the screen is to be stretched, turn on stretching for the
     * various modes.
     *
     * OPTION_LCD_STRETCH means stretching should be turned off!
     */
    NeoNew->PanelDispCntlReg2 = 0x00;
    NeoNew->PanelDispCntlReg3 = 0x00;
    if ((!nPtr->noLcdStretch) &&
	(NeoNew->PanelDispCntlReg1 & 0x02)) {
	if (mode->HDisplay == nPtr->NeoPanelWidth) {
	    /*
	     * No stretching required when the requested display width
	     * equals the panel width.
	     */
	    if (nPtr->NeoHWCursorInitialized) nAcl->UseHWCursor = TRUE;
	} else {
	    switch (mode->HDisplay) {
	    case  320 : /* Needs testing.  KEM -- 24 May 98 */
	    case  400 : /* Needs testing.  KEM -- 24 May 98 */
	    case  640 :
	    case  800 :
	    case 1024 :
		NeoNew->PanelDispCntlReg2 |= 0xC6;
		if (nPtr->NeoHWCursorInitialized) nAcl->UseHWCursor = FALSE;
		break;
	    default   :
		/* No stretching in these modes. */
		if (nPtr->NeoHWCursorInitialized) nAcl->UseHWCursor = TRUE;
		break;
	    }
	}
    } else if (mode->Flags & V_DBLSCAN) {
	if (nPtr->NeoHWCursorInitialized) nAcl->UseHWCursor = FALSE;
    } else {
	if (nPtr->NeoHWCursorInitialized) nAcl->UseHWCursor = TRUE;
    }

    /*
     * If the screen is to be centerd, turn on the centering for the
     * various modes.
     */
    NeoNew->PanelVertCenterReg1  = 0x00;
    NeoNew->PanelVertCenterReg2  = 0x00;
    NeoNew->PanelVertCenterReg3  = 0x00;
    NeoNew->PanelVertCenterReg4  = 0x00;
    NeoNew->PanelVertCenterReg5  = 0x00;
    NeoNew->PanelHorizCenterReg1 = 0x00;
    NeoNew->PanelHorizCenterReg2 = 0x00;
    NeoNew->PanelHorizCenterReg3 = 0x00;
    NeoNew->PanelHorizCenterReg4 = 0x00;
    NeoNew->PanelHorizCenterReg5 = 0x00;

    if (nPtr->lcdCenter &&
	(NeoNew->PanelDispCntlReg1 & 0x02)) {
	if (mode->HDisplay == nPtr->NeoPanelWidth) {
	    /*
	     * No centering required when the requested display width
	     * equals the panel width.
	     */
	} else {
	    NeoNew->PanelDispCntlReg2 |= 0x01;
	    NeoNew->PanelDispCntlReg3 |= 0x10;

	    /* Calculate the horizontal and vertical offsets. */
	    if (nPtr->noLcdStretch) {
		hoffset = ((nPtr->NeoPanelWidth - mode->HDisplay) >> 4) - 1;
		voffset = ((nPtr->NeoPanelHeight - mode->VDisplay) >> 1) - 2;
	    } else {
		/* Stretched modes cannot be centered. */
		hoffset = 0;
		voffset = 0;
	    }

	    switch (mode->HDisplay) {
	    case  320 : /* Needs testing.  KEM -- 24 May 98 */
		NeoNew->PanelHorizCenterReg3 = hoffset;
		NeoNew->PanelVertCenterReg2  = voffset;
		break;
	    case  400 : /* Needs testing.  KEM -- 24 May 98 */
		NeoNew->PanelHorizCenterReg4 = hoffset;
		NeoNew->PanelVertCenterReg1  = voffset;
		break;
	    case  640 :
		NeoNew->PanelHorizCenterReg1 = hoffset;
		NeoNew->PanelVertCenterReg3  = voffset;
		break;
	    case  800 :
		NeoNew->PanelHorizCenterReg2 = hoffset;
		NeoNew->PanelVertCenterReg4  = voffset;
		break;
	    case 1024 :
		NeoNew->PanelHorizCenterReg5 = hoffset;
		NeoNew->PanelVertCenterReg5  = voffset;
		break;
	    case 1280 :
	    default   :
		/* No centering in these modes. */
		break;
	    }
	}
    }

    /*
     * New->reg should be empty.  Just in
     * case it isn't, warn us and clear it anyway.
     */
    if (NeoNew->reg) {
	ErrorF("WARNING: Non-NULL reg in NeoInit: reg=0x%08X\n", NeoNew->reg);
	xfree(NeoNew->reg);
	NeoNew->reg = NULL;
    }

    /*
     * Calculate the VCLK that most closely matches the requested dot
     * clock.
     */
    neoCalcVCLK(pScrn, mode->SynthClock);

    /* Since we program the clocks ourselves, always use VCLK3. */
    NeoStd->MiscOutReg |= 0x0C;

    neoRestore(pScrn, NeoStd, NeoNew, FALSE);
    
    return(TRUE);
}

/*
 * neoCalcVCLK --
 *
 * Determine the closest clock frequency to the one requested.
 */
#define REF_FREQ 14.31818
#define MAX_N 127
#define MAX_D 31
#define MAX_F 1

static void
neoCalcVCLK(ScrnInfoPtr pScrn, long freq)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    
    int n, d, f;
    double f_out;
    double f_diff;
    int n_best = 0, d_best = 0, f_best = 0;
    double f_best_diff = 999999.0;
    double f_target = freq/1000.0;

    for (f = 0; f <= MAX_F; f++)
	for (n = 0; n <= MAX_N; n++)
	    for (d = 0; d <= MAX_D; d++) {
		f_out = (n+1.0)/((d+1.0)*(1<<f))*REF_FREQ;
		f_diff = abs(f_out-f_target);
		if (f_diff < f_best_diff) {
		    f_best_diff = f_diff;
		    n_best = n;
		    d_best = d;
		    f_best = f;
		}
	    }

    if (nPtr->NeoChipset == NM2200) {
        /* NOT_DONE:  We are trying the full range of the 2200 clock.
           We should be able to try n up to 2047 */
	nPtr->NeoModeReg.VCLK3NumeratorLow  = n_best;
	nPtr->NeoModeReg.VCLK3NumeratorHigh = (f_best << 7);
    }
    else {
	nPtr->NeoModeReg.VCLK3NumeratorLow  = n_best | (f_best << 7);
    }
    nPtr->NeoModeReg.VCLK3Denominator = d_best;
}

/*
 * NeoDisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void
NeoDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			     int flags)
{
    NEOPtr nPtr = NEOPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    unsigned char SEQ01 = 0;
    unsigned char LogicPowerMgmt = 0;
    unsigned char LCD_on = 0;

    if (pScrn->vtSema)
	return;

    switch (PowerManagementMode) {
    case DPMSModeOn:
	/* Screen: On; HSync: On, VSync: On */
	SEQ01 = 0x00;
	LogicPowerMgmt = 0x00;
	if (nPtr->internDisp || ! nPtr->externDisp)
	    LCD_on = 0x02;
	else
	    LCD_on = 0x00;
	break;
    case DPMSModeStandby:
	/* Screen: Off; HSync: Off, VSync: On */
	SEQ01 = 0x20;
	LogicPowerMgmt = 0x10;
	LCD_on = 0x00;
	break;
    case DPMSModeSuspend:
	/* Screen: Off; HSync: On, VSync: Off */
	SEQ01 = 0x20;
	LogicPowerMgmt = 0x20;
	LCD_on = 0x00;
	break;
    case DPMSModeOff:
	/* Screen: Off; HSync: Off, VSync: Off */
	SEQ01 = 0x20;
	LogicPowerMgmt = 0x30;
	LCD_on = 0x00;
	break;
    }

    /* Turn the screen on/off */
    outb(0x3C4, 0x01);
    SEQ01 |= inb(0x3C5) & ~0x20;
    outb(0x3C5, SEQ01);

    /* Turn the LCD on/off */
    LCD_on |= VGArGR(0x20) & ~0x02;
    VGAwGR(0x20, LCD_on);

    /* Set the DPMS mode */
    LogicPowerMgmt |= 0x80;
    LogicPowerMgmt |= VGArGR(0x01) & ~0xF0;
    VGAwGR(0x01,LogicPowerMgmt);
}
#endif

static unsigned int
neo_ddc1Read(ScrnInfoPtr pScrn)
{
    register vgaHWPtr hwp = VGAHWPTR(pScrn);
#if 0
    register unsigned int ST01reg = ((NEOPtr)pScrn->driverPrivate)->vgaIOBase 
                                     + 0x0A;
#endif
    register unsigned int tmp;

#if 0
    while(inb(ST01reg)&0x8){};
    while(!(inb(ST01reg)&0x8)) {};
#endif
    while (hwp->readST01(hwp)&0x8) {};
    while (!hwp->readST01(hwp)&0x8) {};
    
    tmp = (VGArGR(0xA1) & 0x08);
    
    return (tmp);
}

static void
neo_ddc1(int scrnIndex)
{
    vgaHWPtr hwp = VGAHWPTR(xf86Screens[scrnIndex]);
    unsigned int reg1, reg2, reg3;

    /* initialize chipset */
    reg1 = VGArCR(0x21);
    reg2 = VGArCR(0x1D); 
    reg3 = VGArCR(0x1A);
    VGAwCR(0x21,0x00);
    VGAwCR(0x1D,0x01);  /* some Voodoo */ 
    VGAwGR(0xA1,0x2F);
    xf86SetDDCproperties(xf86Screens[scrnIndex],xf86PrintEDID(
	xf86DoEDID_DDC1(scrnIndex,vgaHWddc1SetSpeed,neo_ddc1Read)));
    /* undo initialization */
    VGAwCR(0x21,reg1);
    VGAwCR(0x1D,reg2);
}

