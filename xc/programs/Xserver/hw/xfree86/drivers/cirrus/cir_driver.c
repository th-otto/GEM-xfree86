/*
 * Driver for CL-GD5480.
 * Itai Nahshon.
 *
 * This is mainly a cut & paste from the MGA driver.
 * Original autors and contributors list include:
 *	Radoslaw Kapitan, Andrew Vanderstock, Dirk Hohndel,
 *	David Dawes, Andrew E. Mileski, Leonard N. Zubkoff,
 *	Guy DESBIEF
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_driver.c,v 1.54 2000/03/08 21:21:29 dawes Exp $ */

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

#include "xf86Resources.h"

/* All drivers need this */
#include "xf86_ansic.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

#include "cir.h"

/*
 * Forward definitions for the functions that make up the driver.
 */

/* Mandatory functions */

static OptionInfoPtr	CIRAvailableOptions(int chipid, int busid);
static void	CIRIdentify(int flags);
static Bool	CIRProbe(DriverPtr drv, int flags);

extern OptionInfoPtr	AlpAvailableOptions(int chipid);
extern OptionInfoPtr	LgAvailableOptions(int chipid);
extern Bool AlpProbe(int entity, ScrnInfoPtr pScrn);
extern Bool LgProbe(int entity, ScrnInfoPtr pScrn);

#define VERSION 4000
#define CIR_NAME "CIRRUS"
#define CIR_DRIVER_NAME "cirrus"
#define CIR_MAJOR_VERSION 1
#define CIR_MINOR_VERSION 0
#define CIR_PATCHLEVEL 0

/*
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

DriverRec CIRRUS = {
	VERSION,
	CIR_DRIVER_NAME,
#if 0
	"Driver for Cirrus Logic GD5446, GD5480, and GD5462/4/5 cards",
#endif
	CIRIdentify,
	CIRProbe,
	CIRAvailableOptions,
	NULL,
	0
};

/* Supported chipsets */
SymTabRec CIRChipsets[] = {
	{ PCI_CHIP_GD5430,		"CLGD5430" },
	{ PCI_CHIP_GD5434_4,	"CLGD5434-4" },
	{ PCI_CHIP_GD5434_8,	"CLGD5434-8" },
	{ PCI_CHIP_GD5436,		"CLGD5436" },
/*  { PCI_CHIP_GD5440,		"CLGD5440" }, */
	{ PCI_CHIP_GD5446,		"CLGD5446" },
	{ PCI_CHIP_GD5480,		"CLGD5480" },
	{ PCI_CHIP_GD5462,		"CL-GD5462" },
	{ PCI_CHIP_GD5464,		"CL-GD5464" },
	{ PCI_CHIP_GD5464BD,	"CL-GD5464BD" },
	{ PCI_CHIP_GD5465,		"CL-GD5465" },
	{-1,					NULL }
};

/* List of PCI chipset names */
PciChipsets CIRPciChipsets[] = {
	{ PCI_CHIP_GD5430,	PCI_CHIP_GD5430,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5434_4,PCI_CHIP_GD5434_4,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5434_8,PCI_CHIP_GD5434_8,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5436,	PCI_CHIP_GD5436,	RES_SHARED_VGA },
/*  { PCI_CHIP_GD5440,	PCI_CHIP_GD5440,	RES_SHARED_VGA }, */
	{ PCI_CHIP_GD5446,	PCI_CHIP_GD5446,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5480,	PCI_CHIP_GD5480,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5462,	PCI_CHIP_GD5462,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5464,	PCI_CHIP_GD5464,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5464BD,PCI_CHIP_GD5464BD,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5465,	PCI_CHIP_GD5465,	RES_SHARED_VGA },
	{ -1,				-1,					RES_UNDEFINED}
};

/*
 * List of symbols from other modules that this module references.  This
 * list is used to tell the loader that it is OK for symbols here to be
 * unresolved providing that it hasn't been told that they haven't been
 * told that they are essential via a call to xf86LoaderReqSymbols() or
 * xf86LoaderReqSymLists().  The purpose is this is to avoid warnings about
 * unresolved symbols that are not required.
 */

static const char *alpSymbols[] = {
	"AlpProbe",
	NULL
};
static const char *lgSymbols[] = {
	"LgProbe",
	NULL
};


#ifdef XFree86LOADER

static MODULESETUPPROTO(cirSetup);

static XF86ModuleVersionInfo cirVersRec =
{
	"cirrus",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	CIR_MAJOR_VERSION, CIR_MINOR_VERSION, CIR_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData.
 */
XF86ModuleData cirrusModuleData = { &cirVersRec, cirSetup, NULL };

static pointer
cirSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

	/* This module should be loaded only once, but check to be sure. */

	if (!setupDone) {
		setupDone = TRUE;
		xf86AddDriver(&CIRRUS, module, 0);

		LoaderRefSymLists(alpSymbols, lgSymbols, NULL);
		return (pointer)1;
	}
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
}

#endif /* XFree86LOADER */

/* Mandatory */
static void
CIRIdentify(int flags)
{
	xf86PrintChipsets(CIR_NAME, "driver for Cirrus chipsets", CIRChipsets);
}

static
OptionInfoPtr
CIRAvailableOptions(int chipid, int busid)
{
	int vendor = (chipid & 0xffff0000) >> 16;
	int chip = chipid & 0xffff;

#if 0
	if (chip == PCI_CHIP_GD5462 ||
	    chip == PCI_CHIP_GD5464 ||
	    chip == PCI_CHIP_GD5464BD ||
	    chip == PCI_CHIP_GD5465) 
	    if (!lg_loaded) {
	        if(!xf86LoadSubModule(pScrn, "cirrus_laguna")) {
		    return NULL;
		}
		xf86LoaderReqSymLists(lgSymbols, NULL);
		lg_loaded = TRUE;
	    }
	    return LgAvailableOptions(chipid);
	} else {
	    if (!alp_loaded) {
	        if (!xf86LoadSubModule(pScrn, "cirrus_alpine")) {
		    return NULL;
		}
		xf86LoaderReqSymLists(alpSymbols, NULL);
		alp_loaded = TRUE;
	    }
	    return AlpAvailableOptions(chipid);
	}
#else
	return NULL;
#endif
}

/* Mandatory */
static Bool
CIRProbe(DriverPtr drv, int flags)
{
	int i;
	GDevPtr *devSections = NULL;
	pciVideoPtr pPci;
	int *usedChips;
	int numDevSections;
	int numUsed;
	Bool foundScreen = FALSE;
	Bool (*subProbe)(int entity, ScrnInfoPtr pScrn);
	Bool lg_loaded = FALSE;
	Bool alp_loaded = FALSE;
#ifdef CIR_DEBUG
	ErrorF("CirProbe\n");
#endif

	if ((numDevSections = xf86MatchDevice(CIR_DRIVER_NAME,
					  &devSections)) <= 0) {
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

	numUsed = xf86MatchPciInstances(CIR_NAME, PCI_VENDOR_CIRRUS,
									CIRChipsets, CIRPciChipsets, devSections,
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
		pScrn->driverVersion = VERSION;
		pScrn->driverName	 = CIR_DRIVER_NAME;
		pScrn->name		 = CIR_NAME;
		pScrn->Probe	 = NULL;
		/* The Laguna family of chips is so different from the Alpine
		   family that we won't share even the highest-level of
		   functions.  But, the Laguna chips /are/ Cirrus chips, so
		   they should be handled in this driver (as opposed to their
		   own driver). */
		pPci = xf86GetPciInfoForEntity(usedChips[i]);
		if (pPci->chipType == PCI_CHIP_GD5462 ||
			pPci->chipType == PCI_CHIP_GD5464 ||
			pPci->chipType == PCI_CHIP_GD5464BD ||
			pPci->chipType == PCI_CHIP_GD5465) {

		    if (!lg_loaded) {
		        if(!xf86LoadSubModule(pScrn, "cirrus_laguna")) {
			  xf86DeleteScreen(pScrn->scrnIndex, 0);
			  continue;
			}
			xf86LoaderReqSymLists(lgSymbols, NULL);
			lg_loaded = TRUE;
		    }
		    subProbe = LgProbe;
		} else {
		    if (!alp_loaded) {
		        if (!xf86LoadSubModule(pScrn, "cirrus_alpine")) {
			    xf86DeleteScreen(pScrn->scrnIndex, 0);
			    continue;
			}
			xf86LoaderReqSymLists(alpSymbols, NULL);
			alp_loaded = TRUE;
		    }
		    subProbe = AlpProbe;
		}
		if (subProbe(usedChips[i], pScrn))
			foundScreen = TRUE;
	}
	xfree(usedChips);

	return foundScreen;
}

/*
 * Map the framebuffer and MMIO memory.
 */

Bool
CirMapMem(CirPtr pCir, int scrnIndex)
{
	int mmioFlags;

#ifdef CIR_DEBUG
	ErrorF("CirMapMem\n");
#endif

	/*
	 * Map the frame buffer.
	 */

	pCir->FbBase = xf86MapPciMem(scrnIndex, VIDMEM_FRAMEBUFFER, pCir->PciTag,
									pCir->FbAddress, pCir->FbMapSize);
	if (pCir->FbBase == NULL)
		return FALSE;

#ifdef CIR_DEBUG
	ErrorF("CirMapMem pCir->FbBase=0x%08x\n", pCir->FbBase);
#endif

	/*
	 * Map IO registers to virtual address space
	 */
	if (pCir->IOAddress == 0) {
		pCir->IOBase = NULL; /* Until we are ready to use MMIO */
	} else {
		mmioFlags = VIDMEM_MMIO;
		/*
		 * For Alpha, we need to map SPARSE memory, since we need
		 * byte/short access.  Common-level will automatically use
		 * sparse mapping for MMIO.
		 */
		pCir->IOBase = xf86MapPciMem(scrnIndex, mmioFlags, pCir->PciTag,
										pCir->IOAddress, pCir->IoMapSize);
		if (pCir->IOBase == NULL)
			return FALSE;
	}

#ifdef CIR_DEBUG
	ErrorF("CirMapMem pCir->IOBase=0x%08x\n", pCir->IOBase);
#endif

	return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

Bool
CirUnmapMem(CirPtr pCir, int scrnIndex)
{
#ifdef CIR_DEBUG
	ErrorF("CirUnmapMem\n");
#endif

	if (pCir->IOBase != NULL) {
		/*
		 * Unmap IO registers to virtual address space
		 */
		xf86UnMapVidMem(scrnIndex, (pointer)pCir->IOBase, pCir->IoMapSize);
		pCir->IOBase = NULL;
	}

	xf86UnMapVidMem(scrnIndex, (pointer)pCir->FbBase, pCir->FbMapSize);
	pCir->FbBase = NULL;
	return TRUE;
}
