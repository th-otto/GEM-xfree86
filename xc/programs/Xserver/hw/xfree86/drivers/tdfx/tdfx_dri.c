/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tdfx/tdfx_dri.c,v 1.6 2000/02/15 07:13:42 martin Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Priv.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb32.h"

#include "miline.h"

#include "GL/glxtokens.h"

#include "tdfx.h"
#include "tdfx_dri.h"
#include "tdfx_dripriv.h"

static char TDFXKernelDriverName[] = "tdfx";
static char TDFXClientDriverName[] = "tdfx";

static Bool TDFXInitVisualConfigs(ScreenPtr pScreen);
static Bool TDFXCreateContext(ScreenPtr pScreen, VisualPtr visual, 
			      drmContext hwContext, void *pVisualConfigPriv,
			      DRIContextType contextStore);
static void TDFXDestroyContext(ScreenPtr pScreen, drmContext hwContext,
			       DRIContextType contextStore);
static void TDFXDRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
			       DRIContextType readContextType, 
			       void *readContextStore,
			       DRIContextType writeContextType, 
			       void *writeContextStore);
static void TDFXDRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index);
static void TDFXDRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
			       RegionPtr prgnSrc, CARD32 index);

static Bool
TDFXInitVisualConfigs(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  int numConfigs = 0;
  __GLXvisualConfig *pConfigs = 0;
  TDFXConfigPrivPtr pTDFXConfigs = 0;
  TDFXConfigPrivPtr *pTDFXConfigPtrs = 0;
  int i;

  switch (pScrn->bitsPerPixel) {
  case 8:
  case 24:
  case 32:
    break;
  case 16:
    numConfigs = 2;

    if (!(pConfigs = (__GLXvisualConfig*)xnfcalloc(sizeof(__GLXvisualConfig),
						   numConfigs))) {
      return FALSE;
    }
    if (!(pTDFXConfigs = (TDFXConfigPrivPtr)xnfcalloc(sizeof(TDFXConfigPrivRec),
						     numConfigs))) {
      xfree(pConfigs);
      return FALSE;
    }
    if (!(pTDFXConfigPtrs = (TDFXConfigPrivPtr*)xnfcalloc(sizeof(TDFXConfigPrivPtr),
							 numConfigs))) {
      xfree(pConfigs);
      xfree(pTDFXConfigs);
      return FALSE;
    }
    for (i=0; i<numConfigs; i++) 
      pTDFXConfigPtrs[i] = &pTDFXConfigs[i];

    pConfigs[0].vid = -1;
    pConfigs[0].class = -1;
    pConfigs[0].rgba = TRUE;
    pConfigs[0].redSize = 5;
    pConfigs[0].greenSize = 6;
    pConfigs[0].blueSize = 5;
    pConfigs[0].redMask = 0x0000F800;
    pConfigs[0].greenMask = 0x000007E0;
    pConfigs[0].blueMask = 0x0000001F;
    pConfigs[0].alphaMask = 0;
    pConfigs[0].accumRedSize = 0;
    pConfigs[0].accumGreenSize = 0;
    pConfigs[0].accumBlueSize = 0;
    pConfigs[0].accumAlphaSize = 0;
    pConfigs[0].doubleBuffer = TRUE;
    pConfigs[0].stereo = FALSE;
    pConfigs[0].bufferSize = 16;
    pConfigs[0].depthSize = 16;
    pConfigs[0].stencilSize = 0;
    pConfigs[0].auxBuffers = 0;
    pConfigs[0].level = 0;
    pConfigs[0].visualRating = 0;
    pConfigs[0].transparentPixel = 0;
    pConfigs[0].transparentRed = 0;
    pConfigs[0].transparentGreen = 0;
    pConfigs[0].transparentBlue = 0;
    pConfigs[0].transparentAlpha = 0;
    pConfigs[0].transparentIndex = 0;

    pConfigs[1].vid = -1;
    pConfigs[1].class = -1;
    pConfigs[1].rgba = TRUE;
    pConfigs[1].redSize = 5;
    pConfigs[1].greenSize = 6;
    pConfigs[1].blueSize = 5;
    pConfigs[1].redMask = 0x0000F800;
    pConfigs[1].greenMask = 0x000007E0;
    pConfigs[1].blueMask = 0x0000001F;
    pConfigs[1].alphaMask = 0;
    pConfigs[1].accumRedSize = 0;
    pConfigs[1].accumGreenSize = 0;
    pConfigs[1].accumBlueSize = 0;
    pConfigs[1].accumAlphaSize = 0;
    pConfigs[1].doubleBuffer = FALSE;
    pConfigs[1].stereo = FALSE;
    pConfigs[1].bufferSize = 16;
    pConfigs[1].depthSize = 16;
    pConfigs[1].stencilSize = 0;
    pConfigs[1].auxBuffers = 0;
    pConfigs[1].level = 0;
    pConfigs[1].visualRating = 0;
    pConfigs[1].transparentPixel = 0;
    pConfigs[1].transparentRed = 0;
    pConfigs[1].transparentGreen = 0;
    pConfigs[1].transparentBlue = 0;
    pConfigs[1].transparentAlpha = 0;
    pConfigs[1].transparentIndex = 0;

    break;
  }
  pTDFX->numVisualConfigs = numConfigs;
  pTDFX->pVisualConfigs = pConfigs;
  pTDFX->pVisualConfigsPriv = pTDFXConfigs;
  GlxSetVisualConfigs(numConfigs, pConfigs, (void**)pTDFXConfigPtrs);
  return TRUE;
}

Bool TDFXDRIScreenInit(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  DRIInfoPtr pDRIInfo;
  TDFXDRIPtr pTDFXDRI;

#if XFree86LOADER
    /* Check that the GLX, DRI, and DRM modules have been loaded by testing
       for canonical symbols in each module. */
    if (!LoaderSymbol("GlxSetVisualConfigs")) return FALSE;
    if (!LoaderSymbol("DRIScreenInit"))       return FALSE;
    if (!LoaderSymbol("drmAvailable"))        return FALSE;
#endif

  pDRIInfo = DRICreateInfoRec();
  if (!pDRIInfo) return FALSE;
  pTDFX->pDRIInfo = pDRIInfo;

  pDRIInfo->drmDriverName = TDFXKernelDriverName;
  pDRIInfo->clientDriverName = TDFXClientDriverName;
  pDRIInfo->busIdString = xalloc(64);
  sprintf(pDRIInfo->busIdString, "PCI:%d:%d:%d",
	  ((pciConfigPtr)pTDFX->PciInfo->thisCard)->busnum,
	  ((pciConfigPtr)pTDFX->PciInfo->thisCard)->devnum,
	  ((pciConfigPtr)pTDFX->PciInfo->thisCard)->funcnum);
  pDRIInfo->ddxDriverMajorVersion = 0;
  pDRIInfo->ddxDriverMinorVersion = 1;
  pDRIInfo->ddxDriverPatchVersion = 0;
  pDRIInfo->frameBufferPhysicalAddress = pTDFX->LinearAddr;
  pDRIInfo->frameBufferSize = pTDFX->FbMapSize;
  pDRIInfo->frameBufferStride = pTDFX->stride;
  pDRIInfo->ddxDrawableTableEntry = TDFX_MAX_DRAWABLES;

  if (SAREA_MAX_DRAWABLES < TDFX_MAX_DRAWABLES)
    pDRIInfo->maxDrawableTableEntry = SAREA_MAX_DRAWABLES;
  else
    pDRIInfo->maxDrawableTableEntry = TDFX_MAX_DRAWABLES;

#ifdef NOT_DONE
  /* FIXME need to extend DRI protocol to pass this size back to client 
   * for SAREA mapping that includes a device private record
   */
  pDRIInfo->SAREASize = 
    ((sizeof(XF86DRISAREARec) + 0xfff) & 0x1000); /* round to page */
  /* + shared memory device private rec */
#else
  /* For now the mapping works by using a fixed size defined
   * in the SAREA header
   */
  if (sizeof(XF86DRISAREARec)+sizeof(TDFXSAREAPriv)>SAREA_MAX) {
    ErrorF("Data does not fit in SAREA\n");
    return FALSE;
  }
  pDRIInfo->SAREASize = SAREA_MAX;
#endif

  if (!(pTDFXDRI = (TDFXDRIPtr)xnfcalloc(sizeof(TDFXDRIRec),1))) {
    DRIDestroyInfoRec(pTDFX->pDRIInfo);
    pTDFX->pDRIInfo=0;
    return FALSE;
  }
  pDRIInfo->devPrivate = pTDFXDRI;
  pDRIInfo->devPrivateSize = sizeof(TDFXDRIRec);
  pDRIInfo->contextSize = sizeof(TDFXDRIContextRec);

  pDRIInfo->CreateContext = TDFXCreateContext;
  pDRIInfo->DestroyContext = TDFXDestroyContext;
  pDRIInfo->SwapContext = TDFXDRISwapContext;
  pDRIInfo->InitBuffers = TDFXDRIInitBuffers;
  pDRIInfo->MoveBuffers = TDFXDRIMoveBuffers;
  pDRIInfo->bufferRequests = DRI_ALL_WINDOWS;

  if (!DRIScreenInit(pScreen, pDRIInfo, &pTDFX->drmSubFD)) {
    xfree(pDRIInfo->devPrivate);
    pDRIInfo->devPrivate=0;
    DRIDestroyInfoRec(pTDFX->pDRIInfo);
    pTDFX->pDRIInfo=0;
    return FALSE;
  }

  pTDFXDRI->regsSize=TDFXIOMAPSIZE;
  if (drmAddMap(pTDFX->drmSubFD, (drmHandle)pTDFX->MMIOAddr, 
		pTDFXDRI->regsSize, DRM_REGISTERS, 0, &pTDFXDRI->regs)<0) {
    TDFXDRICloseScreen(pScreen);
    return FALSE;
  }
  xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Registers = 0x%08lx\n",
	       pTDFXDRI->regs);

  if (!(TDFXInitVisualConfigs(pScreen))) {
    TDFXDRICloseScreen(pScreen);
    return FALSE;
  }
  xf86DrvMsg(pScrn->scrnIndex, X_INFO, "visual configs initialized\n" );

  return TRUE;
}

void
TDFXDRICloseScreen(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);

  DRICloseScreen(pScreen);

  if (pTDFX->pDRIInfo) {
    if (pTDFX->pDRIInfo->devPrivate) {
      xfree(pTDFX->pDRIInfo->devPrivate);
      pTDFX->pDRIInfo->devPrivate=0;
    }
    DRIDestroyInfoRec(pTDFX->pDRIInfo);
    pTDFX->pDRIInfo=0;
  }
  if (pTDFX->pVisualConfigs) xfree(pTDFX->pVisualConfigs);
  if (pTDFX->pVisualConfigsPriv) xfree(pTDFX->pVisualConfigsPriv);
}

static Bool
TDFXCreateContext(ScreenPtr pScreen, VisualPtr visual, 
		  drmContext hwContext, void *pVisualConfigPriv,
		  DRIContextType contextStore)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  TDFXConfigPrivPtr pTDFXConfig = (TDFXConfigPrivPtr)pVisualConfigPriv;
  TDFXDRIContextPtr ctx;

  ctx=(TDFXDRIContextPtr)contextStore;
  return TRUE;
}

static void
TDFXDestroyContext(ScreenPtr pScreen, drmContext hwContext, 
		   DRIContextType contextStore)
{
  TDFXDRIContextPtr ctx;
  ctx=(TDFXDRIContextPtr)contextStore;
}

Bool
TDFXDRIFinishScreenInit(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  TDFXDRIPtr pTDFXDRI;

  pTDFX->pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;

  pTDFXDRI=(TDFXDRIPtr)pTDFX->pDRIInfo->devPrivate;
  pTDFXDRI->deviceID=pTDFX->PciInfo->chipType;
  pTDFXDRI->width=pScrn->virtualX;
  pTDFXDRI->height=pScrn->virtualY;
  pTDFXDRI->mem=pScrn->videoRam*1024;
  pTDFXDRI->cpp=pTDFX->cpp;
  pTDFXDRI->stride=pTDFX->stride;
  pTDFXDRI->fifoOffset=pTDFX->fifoOffset;
  pTDFXDRI->fifoSize=pTDFX->fifoSize;
  pTDFXDRI->textureOffset=pTDFX->texOffset;
  pTDFXDRI->textureSize=pTDFX->texSize;
  pTDFXDRI->fbOffset=pTDFX->fbOffset;
  pTDFXDRI->backOffset=pTDFX->backOffset;
  pTDFXDRI->depthOffset=pTDFX->depthOffset;
  return DRIFinishScreenInit(pScreen);
}

static void
TDFXDRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
		   DRIContextType oldContextType, void *oldContext,
		   DRIContextType newContextType, void *newContext)
{
  if ((syncType==DRI_3D_SYNC) && (oldContextType==DRI_2D_CONTEXT) &&
      (newContextType==DRI_2D_CONTEXT)) { /* Entering from Wakeup */
    TDFXSwapContextPrivate(pScreen);
  }
  if ((syncType==DRI_2D_SYNC) && (oldContextType==DRI_NO_CONTEXT) &&
      (newContextType==DRI_2D_CONTEXT)) { /* Exiting from Block Handler */
    TDFXLostContext(pScreen);
  }
}

static void
TDFXDRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index)
{
  ScreenPtr pScreen = pWin->drawable.pScreen;
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  BoxPtr pbox;
  int nbox;

  /* It looks nicer if these start out black */
  pbox = REGION_RECTS(prgn);
  nbox = REGION_NUM_RECTS(prgn);

  TDFXSetupForSolidFill(pScrn, 0, GXcopy, -1);
  while (nbox--) {
    TDFXSelectBuffer(pTDFX, TDFX_BACK);
    TDFXSubsequentSolidFillRect(pScrn, pbox->x1, pbox->y1, 
				pbox->x2-pbox->x1, pbox->y2-pbox->y1);
    TDFXSelectBuffer(pTDFX, TDFX_DEPTH);
    TDFXSubsequentSolidFillRect(pScrn, pbox->x1, pbox->y1, 
				pbox->x2-pbox->x1, pbox->y2-pbox->y1);
    pbox++;
  }
  TDFXSelectBuffer(pTDFX, TDFX_FRONT);

  pTDFX->AccelInfoRec->NeedToSync = TRUE;
}

/*
  This routine is a modified form of XAADoBitBlt with the calls to
  ScreenToScreenBitBlt built in. My routine has the prgnSrc as source
  instead of destination. My origin is upside down so the ydir cases
  are reversed.
*/
static void
TDFXDRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
		   RegionPtr prgnSrc, CARD32 index)
{
  ScreenPtr pScreen = pParent->drawable.pScreen;
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  int nbox;
  BoxPtr pbox, pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
  DDXPointPtr pptTmp, pptNew1, pptNew2;
  int xdir, ydir;
  int dx, dy, x, y, w, h;
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

  TDFXSetupForScreenToScreenCopy(pScrn, xdir, ydir, GXcopy, -1, -1);
  while (nbox--) {
    w=pbox->x2-pbox->x1+1;
    h=pbox->y2-pbox->y1+1;

    /* Unlike XAA, we don't get handed clipped values */
    if (pbox->x1+dx<0) {
      x=-dx;
      w-=x-pbox->x1;
    } else {
      if (pbox->x1+dx+w>pScrn->virtualX) {
        x=pScrn->virtualX-dx-w-1;
        w-=pbox->x1-x;
      } else x=pbox->x1;
    }
    if (pbox->y1+dy<0) {
      y=-dy;
      h-=y-pbox->y1;
    } else {
      if (pbox->y1+dy+h>pScrn->virtualY) {
        y=pScrn->virtualY-dy-h-1;
        h-=pbox->y1-y;
      } else y=pbox->y1;
    }
    if (w<0 || h<0 || x>pScrn->virtualX || y>pScrn->virtualY) continue;

    TDFXSelectBuffer(pTDFX, TDFX_BACK);
    TDFXSubsequentScreenToScreenCopy(pScrn, x, y, x+dx, y+dy, w, h);
    TDFXSelectBuffer(pTDFX, TDFX_DEPTH);
    TDFXSubsequentScreenToScreenCopy(pScrn, x, y, x+dx, y+dy, w, h);
    pbox++;
  }
  TDFXSelectBuffer(pTDFX, TDFX_FRONT);

  if (pboxNew2) {
    DEALLOCATE_LOCAL(pptNew2);
    DEALLOCATE_LOCAL(pboxNew2);
  }
  if (pboxNew1) {
    DEALLOCATE_LOCAL(pptNew1);
    DEALLOCATE_LOCAL(pboxNew1);
  }

  pTDFX->AccelInfoRec->NeedToSync = TRUE;
}
