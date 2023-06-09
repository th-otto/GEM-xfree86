/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/vidmem.c,v 1.10 2000/02/15 02:00:15 eich Exp $ */
/*
 * Copyright 1993-1999 by The XFree86 Project, Inc
 *
 */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86OSpriv.h"

/*
 * This file contains the common part of the video memory mapping functions
 */

/*
 * Get a piece of the ScrnInfoRec.  At the moment, this is only used to hold
 * the MTRR option information, but it is likely to be expanded if we do
 * auto unmapping of memory at VT switch.
 *
 */

typedef struct {
	unsigned long 	physBase;
	unsigned long 	size;
	pointer		virtBase;
	pointer 	mtrrInfo;
	int		flags;
} MappingRec, *MappingPtr;
	
typedef struct {
	int		numMappings;
	MappingPtr *	mappings;
	Bool		mtrrEnabled;
	MessageType	mtrrFrom;
	Bool		mtrrOptChecked;
	ScrnInfoPtr	pScrn;
} VidMapRec, *VidMapPtr;

static int vidMapIndex = -1;

#define VIDMAPPTR(p) ((VidMapPtr)((p)->privates[vidMapIndex].ptr))

static VidMemInfo vidMemInfo = {FALSE, };

static VidMapPtr
getVidMapRec(int scrnIndex)
{
	VidMapPtr vp;

	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	if (vidMapIndex < 0)
		vidMapIndex = xf86AllocateScrnInfoPrivateIndex();

	if (VIDMAPPTR(pScrn) != NULL)
		return VIDMAPPTR(pScrn);

	vp = pScrn->privates[vidMapIndex].ptr = xnfcalloc(sizeof(VidMapRec), 1);
	vp->mtrrEnabled = TRUE;	/* default to enabled */
	vp->mtrrFrom = X_DEFAULT;
	vp->mtrrOptChecked = FALSE;
	vp->pScrn = pScrn;
	return vp;
}

static MappingPtr
newMapping(VidMapPtr vp)
{
	vp->mappings = xnfrealloc(vp->mappings, sizeof(MappingPtr) *
				  (vp->numMappings + 1));
	vp->mappings[vp->numMappings] = xnfcalloc(sizeof(MappingRec), 1);
	return vp->mappings[vp->numMappings++];
}

static MappingPtr
findMapping(VidMapPtr vp, pointer vbase, unsigned long size)
{
	int i;

	for (i = 0; i < vp->numMappings; i++) {
		if (vp->mappings[i]->virtBase == vbase &&
		    vp->mappings[i]->size == size)
			return vp->mappings[i];
	}
	return NULL;
}

static void
removeMapping(VidMapPtr vp, MappingPtr mp)
{
	int i, found = 0;

	for (i = 0; i < vp->numMappings; i++) {
		if (vp->mappings[i] == mp) {
			found = 1;
			xfree(vp->mappings[i]);
		} else if (found) {
			vp->mappings[i - 1] = vp->mappings[i];
		}
	}
	vp->numMappings--;
	vp->mappings[vp->numMappings] = NULL;
}

enum { OPTION_MTRR };
static OptionInfoRec opts[] =
{
	{ OPTION_MTRR, "mtrr", OPTV_BOOLEAN, {0}, FALSE },
	{ -1, NULL, OPTV_NONE, {0}, FALSE }
};

static void
checkMtrrOption(VidMapPtr vp)
{
	if (!vp->mtrrOptChecked && vp->pScrn->options != NULL) {
		/*
		 * We get called once for each screen, so reset
		 * the OptionInfoRecs.
		 */
		opts[0].found = FALSE;

		xf86ProcessOptions(vp->pScrn->scrnIndex, vp->pScrn->options,
					opts);
		if (xf86GetOptValBool(opts, OPTION_MTRR, &vp->mtrrEnabled))
			vp->mtrrFrom = X_CONFIG;
		vp->mtrrOptChecked = TRUE;
	}
}

pointer
xf86MapVidMem(int ScreenNum, int Flags, unsigned long Base, unsigned long Size)
{
	pointer vbase = NULL;
	VidMapPtr vp;
	MappingPtr mp;

	if (((Flags & VIDMEM_FRAMEBUFFER) &&
	     (Flags & (VIDMEM_MMIO | VIDMEM_MMIO_32BIT))))
	    FatalError("Mapping memory with more than one type");
	    
	if (!vidMemInfo.initialised) {
		memset(&vidMemInfo, 0, sizeof(VidMemInfo));
		xf86OSInitVidMem(&vidMemInfo);
	}
	if (!vidMemInfo.initialised || !vidMemInfo.mapMem)
		return NULL;

	vbase = vidMemInfo.mapMem(ScreenNum, Base, Size);

	if (!vbase || vbase == (pointer)-1)
		return NULL;

	vp = getVidMapRec(ScreenNum);
	mp = newMapping(vp);
	mp->physBase = Base;
	mp->size = Size;
	mp->virtBase = vbase;
	mp->flags = Flags;

	/*
	 * Check the "mtrr" option even when MTRR isn't supported to avoid
	 * warnings about unrecognised options.
	 */
	checkMtrrOption(vp);

	if (vp->mtrrEnabled && vidMemInfo.setWC) {
		if (Flags & (VIDMEM_MMIO | VIDMEM_MMIO_32BIT))
			mp->mtrrInfo =
				vidMemInfo.setWC(ScreenNum, Base, Size, FALSE,
						 vp->mtrrFrom);
		else if (Flags & VIDMEM_FRAMEBUFFER)
			mp->mtrrInfo =
				vidMemInfo.setWC(ScreenNum, Base, Size, TRUE,
						 vp->mtrrFrom);
	}
	return vbase;
}

void
xf86UnMapVidMem(int ScreenNum, pointer Base, unsigned long Size)
{
	VidMapPtr vp;
	MappingPtr mp;

	if (!vidMemInfo.initialised || !vidMemInfo.unmapMem) {
		xf86DrvMsg(ScreenNum, X_WARNING,
		  "xf86UnMapVidMem() called before xf86MapVidMem()\n");
		return;
	}

	vp = getVidMapRec(ScreenNum);
	mp = findMapping(vp, Base, Size);
	if (!mp) {
		xf86DrvMsg(ScreenNum, X_WARNING,
		  "xf86UnMapVidMem: cannot find region for [%p,0x%lx]\n",
		  Base, Size);
		return;
	}
	if (vp->mtrrEnabled && vidMemInfo.undoWC && mp)
		vidMemInfo.undoWC(ScreenNum, mp->mtrrInfo);

	vidMemInfo.unmapMem(ScreenNum, Base, Size);
	removeMapping(vp, mp);
}

Bool
xf86CheckMTRR(int ScreenNum)
{
	VidMapPtr vp = getVidMapRec(ScreenNum);

	/*
	 * Check the "mtrr" option even when MTRR isn't supported to avoid
	 * warnings about unrecognised options.
	 */
	checkMtrrOption(vp);

	if (vp->mtrrEnabled && vidMemInfo.setWC)
		return TRUE;
		
	return FALSE;
}

Bool
xf86LinearVidMem()
{
	if (!vidMemInfo.initialised) {
		memset(&vidMemInfo, 0, sizeof(VidMemInfo));
		xf86OSInitVidMem(&vidMemInfo);
	}
	return vidMemInfo.linearSupported;
}

void
xf86MapReadSideEffects(int ScreenNum, int Flags, pointer base,
			unsigned long Size)
{
	if (!(Flags & VIDMEM_READSIDEEFFECT))
		return;

	if (!vidMemInfo.initialised || !vidMemInfo.readSideEffects)
		return;

	vidMemInfo.readSideEffects(ScreenNum, base, Size);
}

