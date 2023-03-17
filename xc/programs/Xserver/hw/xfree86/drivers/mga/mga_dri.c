/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_dri.c,v 1.1 2000/02/11 17:25:55 dawes Exp $ */

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

#include "mga_bios.h"
#include "mga_reg.h"
#include "mga.h"
#include "mga_macros.h"
#include "mga_dri.h"
#include "mga_dripriv.h"

static char MGAKernelDriverName[] = "mga";
static char MGAClientDriverName[] = "mga";

static Bool MGAInitVisualConfigs(ScreenPtr pScreen);
static Bool MGACreateContext(ScreenPtr pScreen, VisualPtr visual, 
			      drmContext hwContext, void *pVisualConfigPriv,
			      DRIContextType contextStore);
static void MGADestroyContext(ScreenPtr pScreen, drmContext hwContext,
			       DRIContextType contextStore);
static void MGADRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
			       DRIContextType readContextType, 
			       void *readContextStore,
			       DRIContextType writeContextType, 
			       void *writeContextStore);
extern void Mga8DRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index);
extern void Mga8DRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
			       RegionPtr prgnSrc, CARD32 index);
extern void Mga16DRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index);
extern void Mga16DRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
			       RegionPtr prgnSrc, CARD32 index);
extern void Mga24DRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index);
extern void Mga24DRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
			       RegionPtr prgnSrc, CARD32 index);
extern void Mga32DRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index);
extern void Mga32DRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
			       RegionPtr prgnSrc, CARD32 index);

static Bool
MGAInitVisualConfigs(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  MGAPtr pMGA = MGAPTR(pScrn);
  int numConfigs = 0;
  __GLXvisualConfig *pConfigs = 0;
  MGAConfigPrivPtr pMGAConfigs = 0;
  MGAConfigPrivPtr *pMGAConfigPtrs = 0;
  int i;

  switch (pScrn->bitsPerPixel) {
  case 8:
  case 24:
  case 32:
    break;
  case 16:
    numConfigs = 4;

    if (!(pConfigs = (__GLXvisualConfig*)xnfcalloc(sizeof(__GLXvisualConfig),
						   numConfigs))) {
      return FALSE;
    }
    if (!(pMGAConfigs = (MGAConfigPrivPtr)xnfcalloc(sizeof(MGAConfigPrivRec),
						     numConfigs))) {
      xfree(pConfigs);
      return FALSE;
    }
    if (!(pMGAConfigPtrs = (MGAConfigPrivPtr*)xnfcalloc(sizeof(MGAConfigPrivPtr),
							 numConfigs))) {
      xfree(pConfigs);
      xfree(pMGAConfigs);
      return FALSE;
    }
    for (i=0; i<numConfigs; i++) 
      pMGAConfigPtrs[i] = &pMGAConfigs[i];

    /* config 0: db=FALSE, depth=0
       config 1: db=FALSE, depth=16
       config 2: db=TRUE, depth=0;
       config 3: db=TRUE, depth=16
    */
    pConfigs[0].vid = -1;
    pConfigs[0].class = -1;
    pConfigs[0].rgba = TRUE;
    pConfigs[0].redSize = 8;
    pConfigs[0].greenSize = 8;
    pConfigs[0].blueSize = 8;
    pConfigs[0].redMask =   0x00FF0000;
    pConfigs[0].greenMask = 0x0000FF00;
    pConfigs[0].blueMask =  0x000000FF;
    pConfigs[0].alphaMask = 0;
    pConfigs[0].accumRedSize = 0;
    pConfigs[0].accumGreenSize = 0;
    pConfigs[0].accumBlueSize = 0;
    pConfigs[0].accumAlphaSize = 0;
    pConfigs[0].doubleBuffer = FALSE;
    pConfigs[0].stereo = FALSE;
    pConfigs[0].bufferSize = 16;
    pConfigs[0].depthSize = 0;
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
    pConfigs[1].redSize = 8;
    pConfigs[1].greenSize = 8;
    pConfigs[1].blueSize = 8;
    pConfigs[1].redMask = 0x00FF0000;
    pConfigs[1].greenMask = 0x0000FF00;
    pConfigs[1].blueMask = 0x000000FF;
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

    pConfigs[2].vid = -1;
    pConfigs[2].class = -1;
    pConfigs[2].rgba = TRUE;
    pConfigs[2].redSize = 8;
    pConfigs[2].greenSize = 8;
    pConfigs[2].blueSize = 8;
    pConfigs[2].redMask = 0x00FF0000;
    pConfigs[2].greenMask = 0x0000FF00;
    pConfigs[2].blueMask = 0x000000FF;
    pConfigs[2].alphaMask = 0;
    pConfigs[2].accumRedSize = 0;
    pConfigs[2].accumGreenSize = 0;
    pConfigs[2].accumBlueSize = 0;
    pConfigs[2].accumAlphaSize = 0;
    pConfigs[2].doubleBuffer = TRUE;
    pConfigs[2].stereo = FALSE;
    pConfigs[2].bufferSize = 16;
    pConfigs[2].depthSize = 0;
    pConfigs[2].stencilSize = 0;
    pConfigs[2].auxBuffers = 0;
    pConfigs[2].level = 0;
    pConfigs[2].visualRating = 0;
    pConfigs[2].transparentPixel = 0;
    pConfigs[2].transparentRed = 0;
    pConfigs[2].transparentGreen = 0;
    pConfigs[2].transparentBlue = 0;
    pConfigs[2].transparentAlpha = 0;
    pConfigs[2].transparentIndex = 0;

    pConfigs[3].vid = -1;
    pConfigs[3].class = -1;
    pConfigs[3].rgba = TRUE;
    pConfigs[3].redSize = 8;
    pConfigs[3].greenSize = 8;
    pConfigs[3].blueSize = 8;
    pConfigs[3].redMask = 0x00FF0000;
    pConfigs[3].greenMask = 0x0000FF00;
    pConfigs[3].blueMask = 0x000000FF;
    pConfigs[3].alphaMask = 0;
    pConfigs[3].accumRedSize = 0;
    pConfigs[3].accumGreenSize = 0;
    pConfigs[3].accumBlueSize = 0;
    pConfigs[3].accumAlphaSize = 0;
    pConfigs[3].doubleBuffer = TRUE;
    pConfigs[3].stereo = FALSE;
    pConfigs[3].bufferSize = 16;
    pConfigs[3].depthSize = 16;
    pConfigs[3].stencilSize = 0;
    pConfigs[3].auxBuffers = 0;
    pConfigs[3].level = 0;
    pConfigs[3].visualRating = 0;
    pConfigs[3].transparentPixel = 0;
    pConfigs[3].transparentRed = 0;
    pConfigs[3].transparentGreen = 0;
    pConfigs[3].transparentBlue = 0;
    pConfigs[3].transparentAlpha = 0;
    pConfigs[3].transparentIndex = 0;
    break;
  }
  pMGA->numVisualConfigs = numConfigs;
  pMGA->pVisualConfigs = pConfigs;
  pMGA->pVisualConfigsPriv = pMGAConfigs;
  GlxSetVisualConfigs(numConfigs, pConfigs, (void**)pMGAConfigPtrs);
  return TRUE;
}

Bool MGADRIScreenInit(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  MGAPtr pMGA = MGAPTR(pScrn);
  DRIInfoPtr pDRIInfo;
  MGADRIPtr pMGADRI;
  MGADRIServerPrivatePtr pMGADRIServer;
  int bufs;
   int prim_size;
   int init_offset;

  pDRIInfo = DRICreateInfoRec();
  if (!pDRIInfo) return FALSE;
  pMGA->pDRIInfo = pDRIInfo;

  pDRIInfo->drmDriverName = MGAKernelDriverName;
  pDRIInfo->clientDriverName = MGAClientDriverName;
  pDRIInfo->busIdString = xalloc(64);
  sprintf(pDRIInfo->busIdString, "PCI:%d:%d:%d",
	  ((pciConfigPtr)pMGA->PciInfo->thisCard)->busnum,
	  ((pciConfigPtr)pMGA->PciInfo->thisCard)->devnum,
	  ((pciConfigPtr)pMGA->PciInfo->thisCard)->funcnum);
  pDRIInfo->ddxDriverMajorVersion = 0;
  pDRIInfo->ddxDriverMinorVersion = 1;
  pDRIInfo->ddxDriverPatchVersion = 0;
  pDRIInfo->frameBufferPhysicalAddress = pMGA->FbAddress;
  pDRIInfo->frameBufferSize = pMGA->FbMapSize;
  pDRIInfo->frameBufferStride = pScrn->displayWidth*(pScrn->bitsPerPixel/8);
  pDRIInfo->ddxDrawableTableEntry = MGA_MAX_DRAWABLES;

  if (SAREA_MAX_DRAWABLES < MGA_MAX_DRAWABLES)
    pDRIInfo->maxDrawableTableEntry = SAREA_MAX_DRAWABLES;
  else
    pDRIInfo->maxDrawableTableEntry = MGA_MAX_DRAWABLES;

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
  if (sizeof(XF86DRISAREARec)+sizeof(drm_mga_sarea_t)>SAREA_MAX) {
    ErrorF("Data does not fit in SAREA\n");
    return FALSE;
  }
  pDRIInfo->SAREASize = SAREA_MAX;
#endif

  if (!(pMGADRI = (MGADRIPtr)xnfcalloc(sizeof(MGADRIRec),1))) {
    DRIDestroyInfoRec(pMGA->pDRIInfo);
    pMGA->pDRIInfo=0;
    ErrorF("Failed to allocate memory for private record \n");
    return FALSE;
  }
   if (!(pMGADRIServer = (MGADRIServerPrivatePtr)
	 xnfcalloc(sizeof(MGADRIServerPrivate),1))) {
    xfree(pMGADRI);
    DRIDestroyInfoRec(pMGA->pDRIInfo);
    pMGA->pDRIInfo=0;
    ErrorF("Failed to allocate memory for private record \n");
    return FALSE;
  }

  pDRIInfo->devPrivate = pMGADRI;
  pMGA->DRIServerInfo = pMGADRIServer;
  pDRIInfo->devPrivateSize = sizeof(MGADRIRec);
  pDRIInfo->contextSize = sizeof(MGADRIContextRec);

  pDRIInfo->CreateContext = MGACreateContext;
  pDRIInfo->DestroyContext = MGADestroyContext;
  pDRIInfo->SwapContext = MGADRISwapContext;
  
  switch( pScrn->bitsPerPixel ) {
   case 8:
       pDRIInfo->InitBuffers = Mga8DRIInitBuffers;
       pDRIInfo->MoveBuffers = Mga8DRIMoveBuffers;
   case 16:
       pDRIInfo->InitBuffers = Mga16DRIInitBuffers;
       pDRIInfo->MoveBuffers = Mga16DRIMoveBuffers;
   case 24:
       pDRIInfo->InitBuffers = Mga24DRIInitBuffers;
       pDRIInfo->MoveBuffers = Mga24DRIMoveBuffers;
   case 32:
       pDRIInfo->InitBuffers = Mga32DRIInitBuffers;
       pDRIInfo->MoveBuffers = Mga32DRIMoveBuffers;
  }
   
  pDRIInfo->bufferRequests = DRI_ALL_WINDOWS;

  if (!DRIScreenInit(pScreen, pDRIInfo, &pMGA->drmSubFD)) {
    xfree(pMGADRIServer);
    pMGA->DRIServerInfo = 0;
    xfree(pDRIInfo->devPrivate);
    pDRIInfo->devPrivate = 0;
    DRIDestroyInfoRec(pMGA->pDRIInfo);
    pMGA->pDRIInfo = 0;
    ErrorF("DRIScreenInit Failed\n");
    return FALSE;
  }

  pMGADRIServer->regsSize = MGAIOMAPSIZE;
  if (drmAddMap(pMGA->drmSubFD, (drmHandle)pMGA->IOAddress, 
		pMGADRIServer->regsSize, DRM_REGISTERS, 0, 
		&pMGADRIServer->regs)<0) {
    DRICloseScreen(pScreen);
    ErrorF("drmAddMap failed Register MMIO region\n");
    return FALSE;
  }
  xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Registers = 0x%08lx\n",
	     pMGADRIServer->regs);
   
  /* Agp Support */
  if(drmAgpAcquire(pMGA->drmSubFD) < 0) {
     DRICloseScreen(pScreen);
     ErrorF("drmAgpAcquire failed\n");
     return FALSE;
  }
  pMGADRIServer->warp_ucode_size = mgaGetMicrocodeSize(pScreen);
   if(pMGADRIServer->warp_ucode_size == 0) {
      ErrorF("microcodeSize is zero\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }

  prim_size = 65536;
  init_offset = ((prim_size + pMGADRIServer->warp_ucode_size + 
		  4096 - 1) / 4096) * 4096;
   
  pMGADRIServer->agpSizep = drmAgpSize(pMGA->drmSubFD);
  pMGADRIServer->agpBase = drmAgpBase(pMGA->drmSubFD);
  if (drmAddMap(pMGA->drmSubFD, (drmHandle)pMGADRIServer->agpBase,
		init_offset, DRM_AGP, 0, 
		&pMGADRIServer->agp_private) < 0) {
    DRICloseScreen(pScreen);
    ErrorF("drmAddMap failed on AGP aperture\n");
    return FALSE;
  }
   
  if (drmMap(pMGA->drmSubFD, (drmHandle)pMGADRIServer->agp_private,
	     init_offset, 
	     (drmAddressPtr)&pMGADRIServer->agp_map) < -1) {
    DRICloseScreen(pScreen);
    ErrorF("drmMap failed on AGP aperture\n");
    return FALSE;
  }
   
  /* Now allocate and bind a default of 8 megs */

  pMGADRIServer->agpHandle = drmAgpAlloc(pMGA->drmSubFD, 0x00800000, 0, 0);
  if(pMGADRIServer->agpHandle == 0) {
    ErrorF("drmAgpAlloc failed\n");
    DRICloseScreen(pScreen);
    return FALSE;
  }
  if(drmAgpBind(pMGA->drmSubFD, pMGADRIServer->agpHandle, 0) < 0) {
    DRICloseScreen(pScreen);
    ErrorF("drmAgpBind failed\n");
    return FALSE;
  }

  mgaInstallMicrocode(pScreen, prim_size);

   ErrorF("init_offset: %x\n", init_offset);
  pMGADRI->agpSize = pMGADRIServer->agpSizep - init_offset;
   ErrorF("pMGADRI->agpSize: %x\n", pMGADRI->agpSize);
	  
  if(drmAddMap(pMGA->drmSubFD, (drmHandle)pMGADRIServer->agpBase + init_offset,
	       pMGADRI->agpSize, DRM_AGP, 0, 
	       &pMGADRI->agp) < 0) {
     ErrorF("Failed to map public agp area\n");
     DRICloseScreen(pScreen);
     return FALSE;
  }
   ErrorF("Mapped public agp area\n");
  /* Here is where we need to do initialization of the dma engine */
   
  pMGADRIServer->agpMode = drmAgpGetMode(pMGA->drmSubFD);
  /* Default to 1X agp mode */
  pMGADRIServer->agpMode &= ~0x00000002;
  if (drmAgpEnable(pMGA->drmSubFD, pMGADRIServer->agpMode) < 0) {
    ErrorF("drmAgpEnable failed\n");
    DRICloseScreen(pScreen);
    return FALSE;
  }
   ErrorF("drmAgpEnabled succeeded\n");
     if((bufs = drmAddBufs(pMGA->drmSubFD,
			/*63*/ 15,
			/* 65536 */ 524288,
			DRM_AGP_BUFFER,
			init_offset)) <= 0) {
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "[drm] failure adding %d %d byte DMA buffers\n",
	       /* 63 */ 15,
	       /* 65536 */ 524288);
    DRICloseScreen(pScreen);
    return FALSE;
  }
   
  xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	     "[drm] added %d %d byte DMA buffers\n",
	     bufs, /* 65536 */ 524288);


  if((mgadrmInitDma(pScrn, prim_size)) != TRUE) {
    ErrorF("Failed to initialize dma engine\n");
    DRICloseScreen(pScreen);
    return FALSE;
  }
   
  ErrorF("Initialized Dma Engine\n");

   
  if(drmMarkBufs(pMGA->drmSubFD, 0.133333, 0.266666) != 0) {
     xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		"[drm] failure marking DMA buffers\n");
     DRICloseScreen(pScreen);
     return FALSE;
  }
  if (!(pMGADRIServer->drmBufs = drmMapBufs(pMGA->drmSubFD))) { 
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "[drm] failure mapping DMA buffers\n");
    DRICloseScreen(pScreen);
    return FALSE;
  }
  xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[drm] buffers mapped with %p\n",
	     pMGADRIServer->drmBufs);
  xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[drm] %d DMA buffers mapped\n",
	     pMGADRIServer->drmBufs->count);
  if (!pMGADRIServer->irq) {
    pMGADRIServer->irq = drmGetInterruptFromBusID(pMGA->drmSubFD,
					    ((pciConfigPtr)pMGA->PciInfo
					     ->thisCard)->busnum,
					    ((pciConfigPtr)pMGA->PciInfo
					     ->thisCard)->devnum,
					    ((pciConfigPtr)pMGA->PciInfo
					     ->thisCard)->funcnum);
     drmCtlInstHandler(pMGA->drmSubFD, pMGADRIServer->irq);
  }

  xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	     "[drm] dma control initialized, using IRQ %d\n",
	     pMGADRIServer->irq);
   
   
  if (!(MGAInitVisualConfigs(pScreen))) {
    DRICloseScreen(pScreen);
    return FALSE;
  }
  xf86DrvMsg(pScrn->scrnIndex, X_INFO, "visual configs initialized\n" );

  return TRUE;
}

void
MGADRICloseScreen(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  MGAPtr pMGA = MGAPTR(pScrn);
  MGADRIServerPrivatePtr pMGADRIServer = pMGA->DRIServerInfo;
   
  xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	     "[drm] unmapping %d buffers\n",
	     pMGADRIServer->drmBufs->count);
  if (drmUnmapBufs(pMGADRIServer->drmBufs)) {
     xf86DrvMsg(pScreen->myNum, X_INFO,
		"[drm] unable to unmap DMA buffers\n");
  }
  mgadrmCleanupDma(pScrn);
   
  DRICloseScreen(pScreen);

  if (pMGA->pDRIInfo) {
    if (pMGA->pDRIInfo->devPrivate) {
      xfree(pMGA->pDRIInfo->devPrivate);
      pMGA->pDRIInfo->devPrivate = 0;
    }
    DRIDestroyInfoRec(pMGA->pDRIInfo);
    pMGA->pDRIInfo = 0;
  }
  if(pMGA->DRIServerInfo) {
     xfree(pMGA->DRIServerInfo);
     pMGA->DRIServerInfo = 0;
  }
  if (pMGA->pVisualConfigs) {
    xfree(pMGA->pVisualConfigs);
  }
  if (pMGA->pVisualConfigsPriv) { 
    xfree(pMGA->pVisualConfigsPriv);
  }
}

static Bool
MGACreateContext(ScreenPtr pScreen, VisualPtr visual, 
		  drmContext hwContext, void *pVisualConfigPriv,
		  DRIContextType contextStore)
{
  MGADRIContextPtr ctx;

  ctx = (MGADRIContextPtr)contextStore;
  return TRUE;
}

static void
MGADestroyContext(ScreenPtr pScreen, drmContext hwContext, 
		   DRIContextType contextStore)
{
  MGADRIContextPtr ctx;
  ctx = (MGADRIContextPtr)contextStore;
}

Bool
MGADRIFinishScreenInit(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  MGAPtr pMGA = MGAPTR(pScrn);
  MGADRIPtr pMGADRI;
  int size;

  pMGA->pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;
  /* pMGA->pDRIInfo->driverSwapMethod = DRI_SERVER_SWAP; */

  pMGADRI = (MGADRIPtr)pMGA->pDRIInfo->devPrivate;
  pMGADRI->deviceID = pMGA->PciInfo->chipType;
  pMGADRI->width = pScrn->virtualX;
  pMGADRI->height = pScrn->virtualY;
  pMGADRI->mem = pScrn->videoRam * 1024;
  pMGADRI->cpp = pScrn->bitsPerPixel / 8;
  pMGADRI->stride = pScrn->displayWidth * (pScrn->bitsPerPixel / 8);
  pMGADRI->backOffset = ((pScrn->virtualY+129) * pScrn->displayWidth *
			 (pScrn->bitsPerPixel / 8) + 4095) & ~0xFFF;
  size = 2 * pScrn->virtualX * pScrn->virtualY;
  pMGADRI->depthOffset = (pMGADRI->backOffset + size + 4095) & ~0xFFF;
  pMGADRI->textureOffset = pMGADRI->depthOffset + size;
  /*
   * The rest of the framebuffer is for textures except for the
   * memory for the hardware cursor. 
   */
  pMGADRI->textureSize = pMGA->FbUsableSize - pMGADRI->textureOffset;
  pMGADRI->fbOffset = pMGA->YDstOrg * (pScrn->bitsPerPixel / 8);

  return DRIFinishScreenInit(pScreen);
}

void MGASwapContext(ScreenPtr pScreen)
{
#if 0
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  MGAPtr pMga = MGAPTR(pScrn);
  MGAFBLayout *pLayout = &pMga->CurrentLayout;

  usleep(500);
  ErrorF("Syncing : swap\n");
  MGABUSYWAIT();
  ErrorF("Sync : swap 1\n");
  MGAStormSync(pScrn);
  ErrorF("Syncing done\n");
  pMga->AccelInfoRec->NeedToSync = TRUE;
   
  WAITFIFO(12);
  OUTREG(MGAREG_MACCESS, pMga->MAccess);
  OUTREG(MGAREG_PITCH, pLayout->displayWidth);
  OUTREG(MGAREG_YDSTORG, pMga->YDstOrg);
  OUTREG(MGAREG_PLNWT, pMga->PlaneMask);
  OUTREG(MGAREG_BCOL, pMga->BgColor);
  OUTREG(MGAREG_FCOL, pMga->FgColor);
  OUTREG(MGAREG_SRCORG, pMga->SrcOrg);
  OUTREG(MGAREG_DSTORG, pMga->DstOrg);
  OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);
  OUTREG(MGAREG_CXBNDRY, 0xFFFF0000); /* (maxX << 16) | minX */
  OUTREG(MGAREG_YTOP, 0x00000000);    /* minPixelPointer */
  OUTREG(MGAREG_YBOT, 0x007FFFFF);    /* maxPixelPointer */ 
  pMga->AccelFlags &= ~CLIPPER_ON;
#endif
}

void MGALostContext(ScreenPtr pScreen)
{
#if 0
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  MGAPtr pMga = MGAPTR(pScrn);
  MGAFBLayout *pLayout = &pMga->CurrentLayout;

  ErrorF("Syncing : lost\n");
  MGAStormSync(pScrn);
  ErrorF("Sync : lost 1\n");
  MGABUSYWAIT();
  ErrorF("Syncing done\n");
#endif
}

static void
MGADRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
		   DRIContextType oldContextType, void *oldContext,
		   DRIContextType newContextType, void *newContext)
{
  if ((syncType == DRI_3D_SYNC) && (oldContextType == DRI_2D_CONTEXT) &&
      (newContextType == DRI_2D_CONTEXT)) { /* Entering from Wakeup */
    MGASwapContext(pScreen);
  }
  if ((syncType == DRI_2D_SYNC) && (oldContextType == DRI_NO_CONTEXT) &&
      (newContextType == DRI_2D_CONTEXT)) { /* Exiting from Block Handler */
    MGALostContext(pScreen);
  }
}

/* Needs to be written */
void MGASelectBuffer(MGAPtr pMGA, int which)
{
}
