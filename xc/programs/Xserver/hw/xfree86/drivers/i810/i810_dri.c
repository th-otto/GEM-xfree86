/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i810_dri.c,v 1.2 2000/03/02 16:07:49 martin Exp $ */

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

#include "i810.h"
#include "i810_dri.h"

#include "xf86drm.h"
#include "dristruct.h"


static char I810KernelDriverName[] = "i810";
static char I810ClientDriverName[] = "i810";

static Bool I810InitVisualConfigs(ScreenPtr pScreen);
static Bool I810CreateContext(ScreenPtr pScreen, VisualPtr visual, 
			      drmContext hwContext, void *pVisualConfigPriv,
			      DRIContextType contextStore);
static void I810DestroyContext(ScreenPtr pScreen, drmContext hwContext,
			       DRIContextType contextStore);
static void I810DRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
			       DRIContextType readContextType, 
			       void *readContextStore,
			       DRIContextType writeContextType, 
			       void *writeContextStore);
static void I810DRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index);
static void I810DRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
			       RegionPtr prgnSrc, CARD32 index);

extern void GlxSetVisualConfigs(int nconfigs,
				__GLXvisualConfig *configs,
				void **configprivs);

static int i810_pitches[] = {
   512,
   1024,
   2048,
   4096,
   0
};

static int i810_pitch_flags[] = {
    0x0,
    0x1,
    0x2,
    0x3,
    0
};

static Bool
I810InitVisualConfigs(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);
   int numConfigs = 0;
   __GLXvisualConfig *pConfigs = 0;
   I810ConfigPrivPtr pI810Configs = 0;
   I810ConfigPrivPtr *pI810ConfigPtrs = 0;
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

      if (!(pI810Configs = 
	    (I810ConfigPrivPtr)xnfcalloc(sizeof(I810ConfigPrivRec),
					 numConfigs))) 
      {
	 xfree(pConfigs);
	 return FALSE;
      }

      if (!(pI810ConfigPtrs = 
	    (I810ConfigPrivPtr*)xnfcalloc(sizeof(I810ConfigPrivPtr),
					  numConfigs))) 
      {
	 xfree(pConfigs);
	 xfree(pI810Configs);
	 return FALSE;
      }

      for (i=0; i<numConfigs; i++) 
	 pI810ConfigPtrs[i] = &pI810Configs[i];

      /* config 0: db=FALSE, depth=0
	 config 1: db=FALSE, depth=16
	 config 2: db=TRUE, depth=0;
	 config 3: db=TRUE, depth=16
      */
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
      pConfigs[0].doubleBuffer = FALSE;
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

      pConfigs[2].vid = -1;
      pConfigs[2].class = -1;
      pConfigs[2].rgba = TRUE;
      pConfigs[2].redSize = 5;
      pConfigs[2].greenSize = 6;
      pConfigs[2].blueSize = 5;
      pConfigs[2].redMask = 0x0000F800;
      pConfigs[2].greenMask = 0x000007E0;
      pConfigs[2].blueMask = 0x0000001F;
      pConfigs[2].alphaMask = 0;
      pConfigs[2].accumRedSize = 0;
      pConfigs[2].accumGreenSize = 0;
      pConfigs[2].accumBlueSize = 0;
      pConfigs[2].accumAlphaSize = 0;
      pConfigs[2].doubleBuffer = TRUE;
      pConfigs[2].stereo = FALSE;
      pConfigs[2].bufferSize = 16;
      pConfigs[2].depthSize = 16;
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
      pConfigs[3].redSize = 5;
      pConfigs[3].greenSize = 6;
      pConfigs[3].blueSize = 5;
      pConfigs[3].redMask = 0x0000F800;
      pConfigs[3].greenMask = 0x000007E0;
      pConfigs[3].blueMask = 0x0000001F;
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
   pI810->numVisualConfigs = numConfigs;
   pI810->pVisualConfigs = pConfigs;
   pI810->pVisualConfigsPriv = pI810Configs;
   GlxSetVisualConfigs(numConfigs, pConfigs, (void**)pI810ConfigPtrs);
   return TRUE;
}


static unsigned int mylog2(unsigned int n)
{
   unsigned int log2 = 1;
   while (n>1) n >>= 1, log2++;
   return log2;
}


Bool I810DRIScreenInit(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);
   DRIInfoPtr pDRIInfo;
   I810DRIPtr pI810DRI;
   unsigned long tom;
   unsigned long agpHandle;
   unsigned long dcacheHandle;
   int sysmem_size = 0;
   int back_size = 0;
   int pitch_idx = 0;
   int bufs;
   int width = pScrn->displayWidth * pI810->cpp;
   int i;
   
   /* ToDo : save agpHandles? */

   
   pDRIInfo = DRICreateInfoRec();
   if (!pDRIInfo) {
      ErrorF("DRICreateInfoRec failed\n");
      return FALSE;
   }


/*     pDRIInfo->wrap.ValidateTree = 0;    */
/*     pDRIInfo->wrap.PostValidateTree = 0;    */

   pI810->pDRIInfo = pDRIInfo;
   pI810->LockHeld = 0;

   pDRIInfo->drmDriverName = I810KernelDriverName;
   pDRIInfo->clientDriverName = I810ClientDriverName;
   pDRIInfo->busIdString = xalloc(64);
   sprintf(pDRIInfo->busIdString, "PCI:%d:%d:%d",
	   ((pciConfigPtr)pI810->PciInfo->thisCard)->busnum,
	   ((pciConfigPtr)pI810->PciInfo->thisCard)->devnum,
	   ((pciConfigPtr)pI810->PciInfo->thisCard)->funcnum);
   pDRIInfo->ddxDriverMajorVersion = 0;
   pDRIInfo->ddxDriverMinorVersion = 1;
   pDRIInfo->ddxDriverPatchVersion = 0;
   pDRIInfo->frameBufferPhysicalAddress = pI810->LinearAddr;
   pDRIInfo->frameBufferSize = (((pScrn->displayWidth * 
				  pScrn->virtualY * pI810->cpp) + 
				 4096 - 1) / 4096) * 4096;

   pDRIInfo->frameBufferStride = pScrn->displayWidth*pI810->cpp;
   pDRIInfo->ddxDrawableTableEntry = I810_MAX_DRAWABLES;

   if (SAREA_MAX_DRAWABLES < I810_MAX_DRAWABLES)
      pDRIInfo->maxDrawableTableEntry = SAREA_MAX_DRAWABLES;
   else
      pDRIInfo->maxDrawableTableEntry = I810_MAX_DRAWABLES;

   /* For now the mapping works by using a fixed size defined
    * in the SAREA header
    */
   if (sizeof(XF86DRISAREARec)+sizeof(drm_i810_sarea_t)>SAREA_MAX) {
      ErrorF("Data does not fit in SAREA\n");
      return FALSE;
   }
   pDRIInfo->SAREASize = SAREA_MAX;

   if (!(pI810DRI = (I810DRIPtr)xnfcalloc(sizeof(I810DRIRec),1))) {
      DRIDestroyInfoRec(pI810->pDRIInfo);
      pI810->pDRIInfo=0;
      return FALSE;
   }
   pDRIInfo->devPrivate = pI810DRI;
   pDRIInfo->devPrivateSize = sizeof(I810DRIRec);
   pDRIInfo->contextSize = sizeof(I810DRIContextRec);
   
   pDRIInfo->CreateContext = I810CreateContext;
   pDRIInfo->DestroyContext = I810DestroyContext;
   pDRIInfo->SwapContext = I810DRISwapContext;
   pDRIInfo->InitBuffers = I810DRIInitBuffers;
   pDRIInfo->MoveBuffers = I810DRIMoveBuffers;
   pDRIInfo->bufferRequests = DRI_ALL_WINDOWS;
   
   
   /* This adds the framebuffer as a drm map *before* we have asked agp
    * to allocate it.  Scary stuff, hold on...
    */
   if (!DRIScreenInit(pScreen, pDRIInfo, &pI810->drmSubFD)) {
      ErrorF("DRIScreenInit failed\n");
      xfree(pDRIInfo->devPrivate);
      pDRIInfo->devPrivate=0;
      DRIDestroyInfoRec(pI810->pDRIInfo);
      pI810->pDRIInfo=0;
      return FALSE;
   }
   
   pI810DRI->regsSize=I810_REG_SIZE;
   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->MMIOAddr, 
		 pI810DRI->regsSize, DRM_REGISTERS, 0, &pI810DRI->regs)<0) {
      ErrorF("drmAddMap(regs) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Registers = 0x%08lx\n",
	      pI810DRI->regs);
   
   pI810->backHandle = 0;
   pI810->zHandle = 0;
   pI810->cursorHandle = 0;
   pI810->sysmemHandle = 0;
   pI810->agpAcquired = FALSE;
   pI810->dcacheHandle = 0;
   
   /* Agp Support - Need this just to get the framebuffer.
    */
   if(drmAgpAcquire(pI810->drmSubFD) < 0) {
      ErrorF("drmAgpAquire failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   pI810->agpAcquired = TRUE;
   
   if (drmAgpEnable(pI810->drmSubFD, 0) < 0) {
      ErrorF("drmAgpEnable failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }

   xf86memset (&pI810->DcacheMem, 0, sizeof(I810MemRange));
   xf86memset (&pI810->BackBuffer, 0, sizeof(I810MemRange));
   xf86memset (&pI810->DepthBuffer, 0, sizeof(I810MemRange));
   pI810->CursorPhysical = 0;
   
   /* Dcache - half the speed of normal ram, but has use as a Z buffer
    * under the DRI.  
    */

   dcacheHandle = drmAgpAlloc(pI810->drmSubFD, 4096 * 1024, 1, NULL);
   pI810->dcacheHandle = dcacheHandle;

   
#define Elements(x) sizeof(x)/sizeof(*x)
   for (pitch_idx = 0 ; pitch_idx < Elements(i810_pitches) ; pitch_idx++) 
      if (width <= i810_pitches[pitch_idx]) 
	 break;
   
   if (pitch_idx == Elements(i810_pitches)) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		 "Couldn't find depth/back buffer pitch");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   else {
      back_size = i810_pitches[pitch_idx] * pScrn->virtualY;
      back_size = ((back_size + 4096 - 1) / 4096) * 4096;
   }
   
   sysmem_size = pScrn->videoRam * 1024;
   if(dcacheHandle != 0) {
      if(back_size > 4*1024*1024) {
	 ErrorF("Backsize is larger then 4 meg\n");
	 sysmem_size = sysmem_size - 2*back_size;
	 drmAgpFree(pI810->drmSubFD, dcacheHandle);
	 pI810->dcacheHandle = dcacheHandle = 0;
      } else {
	 sysmem_size = sysmem_size - back_size;
      }
   } else {
      sysmem_size = sysmem_size - 2*back_size;
   }
   
   sysmem_size -= 4096;
   if(sysmem_size > ((48*1024*1024) - 1) ) {
      sysmem_size = (48*1024*1024) - (2*4096);
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "User requested more memory then fits in the agp aperture\n"
		 "Truncating to %d bytes of memory\n",
		 sysmem_size);
   }
   pI810->SysMem.Start = 0;
   pI810->SysMem.Size = sysmem_size;
   pI810->SysMem.End = sysmem_size;
   tom = sysmem_size;

   pI810->SavedSysMem = pI810->SysMem;

   if (dcacheHandle != 0) {
      /* The Z buffer is always aligned to the 48 mb mark in the aperture */

      if(drmAgpBind(pI810->drmSubFD, dcacheHandle, 48*1024*1024) == 0) {
	 xf86memset (&pI810->DcacheMem, 0, sizeof(I810MemRange));
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		    "GART: Found 4096K Z buffer memory\n");
	 pI810->DcacheMem.Start = 48*1024*1024;
	 pI810->DcacheMem.Size = 1024 * 4096;
	 pI810->DcacheMem.End = pI810->DcacheMem.Start + pI810->DcacheMem.Size;
	 if (!I810AllocLow(&(pI810->DepthBuffer), 
			   &(pI810->DcacheMem),
			   back_size)) 
	 {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		       "Depth buffer allocation failed\n");
	    DRICloseScreen(pScreen);
	    return FALSE;
	 }
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "GART: dcache bind failed\n");
	 drmAgpFree(pI810->drmSubFD, dcacheHandle);
	 pI810->dcacheHandle = dcacheHandle = 0;
      }   
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "GART: no dcache memory found\n");
   }

   agpHandle = drmAgpAlloc(pI810->drmSubFD, back_size, 0, NULL);
   pI810->backHandle = agpHandle;
   
   if(agpHandle != 0) {
      /* The backbuffer is always aligned to the 56 mb mark in the aperture */
      if(drmAgpBind(pI810->drmSubFD, agpHandle, 56*1024*1024) == 0) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Bound backbuffer memory\n");
	 
	 pI810->BackBuffer.Start = 56*1024*1024;
	 pI810->BackBuffer.Size = back_size;
	 pI810->BackBuffer.End = (pI810->BackBuffer.Start + 
				  pI810->BackBuffer.Size);
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Unable to bind backbuffer\n");
	 DRICloseScreen(pScreen);
	 return FALSE;
      }
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "Unable to allocate backbuffer memory\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
      
   if(dcacheHandle == 0) {
      /* The Z buffer is always aligned to the 48 mb mark in the aperture */
      agpHandle = drmAgpAlloc(pI810->drmSubFD, back_size, 0,
			      NULL);
      pI810->zHandle = agpHandle;

      if(agpHandle != 0) {
	 if(drmAgpBind(pI810->drmSubFD, agpHandle, 48*1024*1024) == 0) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Bound depthbuffer memory\n");
	    pI810->DepthBuffer.Start = 48*1024*1024;
	    pI810->DepthBuffer.Size = back_size;
	    pI810->DepthBuffer.End = (pI810->DepthBuffer.Start + 
				      pI810->DepthBuffer.Size);
	 } else {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		       "Unable to bind depthbuffer\n");
	    DRICloseScreen(pScreen);
	    return FALSE;
	 }
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Unable to allocate depthbuffer memory\n");
	 DRICloseScreen(pScreen);
	 return FALSE;
      }
   }	 
   
   /* Now allocate and bind the agp space.  This memory will include the
    * regular framebuffer as well as texture memory.
    */
   
   agpHandle = drmAgpAlloc(pI810->drmSubFD, sysmem_size, 0, NULL);
   if(agpHandle == 0) {
      ErrorF("drmAgpAlloc failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   pI810->sysmemHandle = agpHandle;

   if(drmAgpBind(pI810->drmSubFD, agpHandle, 0) != 0) {
      ErrorF("drmAgpBind failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   agpHandle = drmAgpAlloc(pI810->drmSubFD, 4096, 2, 
			   (unsigned long *)&pI810->CursorPhysical); 
   pI810->cursorHandle = agpHandle;

   if (agpHandle != 0) {
      tom = sysmem_size;

      if(drmAgpBind(pI810->drmSubFD, agpHandle, tom) == 0) { 
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		    "GART: Allocated 4K for mouse cursor image\n");
	 pI810->CursorStart = tom;	 
	 tom += 4096;
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "GART: cursor bind failed\n");
	 pI810->CursorPhysical = 0;    
      } 
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "GART: cursor alloc failed\n");
      pI810->CursorPhysical = 0;
   }

         
   I810SetTiledMemory(pScrn, 1,
		      pI810->DepthBuffer.Start,
		      i810_pitches[pitch_idx],
		      8*1024*1024);
   
   I810SetTiledMemory(pScrn, 2,
		      pI810->BackBuffer.Start,
		      i810_pitches[pitch_idx],
		      8*1024*1024);
   
   pI810->auxPitch = i810_pitches[pitch_idx];
   pI810->auxPitchBits = i810_pitch_flags[pitch_idx];
   
   pI810->SavedDcacheMem = pI810->DcacheMem;
   
   pI810DRI->backbufferSize = pI810->BackBuffer.Size;
   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->BackBuffer.Start,
		 pI810->BackBuffer.Size, DRM_AGP, 0, 
		 &pI810DRI->backbuffer) < 0) {
      ErrorF("drmAddMap(backbuffer) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   pI810DRI->depthbufferSize = pI810->DepthBuffer.Size;
   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->DepthBuffer.Start,
		 pI810->DepthBuffer.Size, DRM_AGP, 0, 
		 &pI810DRI->depthbuffer) < 0) {
      ErrorF("drmAddMap(depthbuffer) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   /* Allocate FrontBuffer etc. */
   I810AllocateFront(pScrn);

   /* Allocate buffer memory */
   I810AllocHigh( &(pI810->BufferMem), &(pI810->SysMem), 
		  I810_DMA_BUF_NR * I810_DMA_BUF_SZ);
   
   if(drmAddMap(pI810->drmSubFD, (drmHandle)pI810->BufferMem.Start,
		pI810->BufferMem.Size, DRM_AGP, 0,
		&pI810->buffer_map) < 0) {
      ErrorF("drmAddMap(buffer_map) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }

   pI810DRI->agp_buffers = pI810->buffer_map;
   pI810DRI->agp_buf_size = pI810->BufferMem.Size;

   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->LpRing.mem.Start,
		 pI810->LpRing.mem.Size, DRM_AGP, 0,
		 &pI810->ring_map) < 0) {
      ErrorF("drmAddMap(ring_map) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   /* Use the rest of memory for textures. */
   pI810DRI->textureSize = pI810->SysMem.Size;


   i = mylog2(pI810DRI->textureSize / I810_NR_TEX_REGIONS);

   if (i < I810_LOG_MIN_TEX_REGION_SIZE)
      i = I810_LOG_MIN_TEX_REGION_SIZE;

   pI810DRI->logTextureGranularity = i;
   pI810DRI->textureSize = (pI810DRI->textureSize >> i) << i; /* truncate */


   if(pI810DRI->textureSize < 512*1024) {
      ErrorF("Less then 512k for textures\n");
      DRICloseScreen(pScreen);
   }
   
   I810AllocLow( &(pI810->TexMem), &(pI810->SysMem),
		 pI810DRI->textureSize);

   
   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->TexMem.Start,
		 pI810->TexMem.Size, DRM_AGP, 0,
		 &pI810DRI->textures) < 0) {
      ErrorF("drmAddMap(textures) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   if((bufs = drmAddBufs(pI810->drmSubFD,
			 I810_DMA_BUF_NR,
			 I810_DMA_BUF_SZ,
			 DRM_AGP_BUFFER, pI810->BufferMem.Start)) <= 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "[drm] failure adding %d %d byte DMA buffers\n",
		 I810_DMA_BUF_NR,
		 I810_DMA_BUF_SZ);
      DRICloseScreen(pScreen);
      return FALSE;
   }

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[drm] added %d %d byte DMA buffers\n",
	      bufs, I810_DMA_BUF_SZ);

   I810drmInitDma(pScrn);
   
   /* Okay now initialize the dma engine */

   if (!pI810DRI->irq) {
      pI810DRI->irq = drmGetInterruptFromBusID(pI810->drmSubFD,
					       ((pciConfigPtr)pI810->PciInfo
						->thisCard)->busnum,
					       ((pciConfigPtr)pI810->PciInfo
						->thisCard)->devnum,
					       ((pciConfigPtr)pI810->PciInfo
						->thisCard)->funcnum);
      if((drmCtlInstHandler(pI810->drmSubFD, pI810DRI->irq)) != 0) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "[drm] failure adding irq handler, there is a device "
		    "already using that irq\n Consider rearranging your "
		    "PCI cards\n");
	 DRICloseScreen(pScreen);
	 return FALSE;
      }
   }

   xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	      "[drm] dma control initialized, using IRQ %d\n",
	      pI810DRI->irq);
   
   pI810DRI=(I810DRIPtr)pI810->pDRIInfo->devPrivate;
   pI810DRI->deviceID=pI810->PciInfo->chipType;
   pI810DRI->width=pScrn->virtualX;
   pI810DRI->height=pScrn->virtualY;
   pI810DRI->mem=pScrn->videoRam*1024;
   pI810DRI->cpp=pI810->cpp;
   
   pI810DRI->fbOffset=pI810->FrontBuffer.Start;
   pI810DRI->fbStride=pI810->auxPitch;
   
   pI810DRI->bitsPerPixel = pScrn->bitsPerPixel;
   
   
   pI810DRI->textureOffset=pI810->TexMem.Start;
   
   pI810DRI->backOffset=pI810->BackBuffer.Start;
   pI810DRI->depthOffset=pI810->DepthBuffer.Start;
   
   pI810DRI->ringOffset=pI810->LpRing.mem.Start;
   pI810DRI->ringSize=pI810->LpRing.mem.Size;
   
   pI810DRI->auxPitch = pI810->auxPitch;
   pI810DRI->auxPitchBits = pI810->auxPitchBits;

   if (!(I810InitVisualConfigs(pScreen))) {
      ErrorF("I810InitVisualConfigs failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "visual configs initialized\n" );
   pI810->pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;
      
   return TRUE;
}

void
I810DRICloseScreen(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);

   I810drmCleanupDma(pScrn);

   if(pI810->dcacheHandle) drmAgpFree(pI810->drmSubFD, pI810->dcacheHandle);
   if(pI810->backHandle) drmAgpFree(pI810->drmSubFD, pI810->backHandle);
   if(pI810->zHandle) drmAgpFree(pI810->drmSubFD, pI810->zHandle);
   if(pI810->cursorHandle) drmAgpFree(pI810->drmSubFD, pI810->cursorHandle);
   if(pI810->sysmemHandle) drmAgpFree(pI810->drmSubFD, pI810->sysmemHandle);

   if(pI810->agpAcquired == TRUE) drmAgpRelease(pI810->drmSubFD);
   
   pI810->backHandle = 0;
   pI810->zHandle = 0;
   pI810->cursorHandle = 0;
   pI810->sysmemHandle = 0;
   pI810->agpAcquired = FALSE;
   pI810->dcacheHandle = 0;

   
   DRICloseScreen(pScreen);

   if (pI810->pDRIInfo) {
      if (pI810->pDRIInfo->devPrivate) {
	 xfree(pI810->pDRIInfo->devPrivate);
	 pI810->pDRIInfo->devPrivate=0;
      }
      DRIDestroyInfoRec(pI810->pDRIInfo);
      pI810->pDRIInfo=0;
   }
   if (pI810->pVisualConfigs) xfree(pI810->pVisualConfigs);
   if (pI810->pVisualConfigsPriv) xfree(pI810->pVisualConfigsPriv);
}

static Bool
I810CreateContext(ScreenPtr pScreen, VisualPtr visual, 
		  drmContext hwContext, void *pVisualConfigPriv,
		  DRIContextType contextStore)
{
   return TRUE;
}

static void
I810DestroyContext(ScreenPtr pScreen, drmContext hwContext, 
		   DRIContextType contextStore)
{
}


Bool
I810DRIFinishScreenInit(ScreenPtr pScreen)
{
   drm_i810_sarea_t *sPriv = (drm_i810_sarea_t *)DRIGetSAREAPrivate(pScreen);
   xf86memset( sPriv, 0, sizeof(sPriv) );
   return DRIFinishScreenInit(pScreen);
}

void
I810DRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
		   DRIContextType oldContextType, void *oldContext,
		   DRIContextType newContextType, void *newContext)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);

   if (syncType == DRI_3D_SYNC && 
       oldContextType == DRI_2D_CONTEXT &&
       newContextType == DRI_2D_CONTEXT) 
   { 
      ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

      if (I810_DEBUG & DEBUG_VERBOSE_DRI)
	 ErrorF("I810DRISwapContext (in)\n");
   
      pI810->LockHeld = 1;
      I810RefreshRing( pScrn );
   }
   else if (syncType == DRI_2D_SYNC && 
	    oldContextType == DRI_NO_CONTEXT &&
	    newContextType == DRI_2D_CONTEXT) 
   { 
      pI810->LockHeld = 0;
      if (I810_DEBUG & DEBUG_VERBOSE_DRI)
	 ErrorF("I810DRISwapContext (out)\n");
   }
   else if (I810_DEBUG & DEBUG_VERBOSE_DRI)
      ErrorF("I810DRISwapContext (other)\n");
}

static void
I810DRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index)
{
   ScreenPtr pScreen = pWin->drawable.pScreen;
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);
   BoxPtr pbox = REGION_RECTS(prgn);
   int nbox = REGION_NUM_RECTS(prgn);

   if (I810_DEBUG & DEBUG_VERBOSE_DRI)
      ErrorF( "I810DRIInitBuffers\n");

   I810SetupForSolidFill(pScrn, 0, GXcopy, -1);
   while (nbox--) {
      I810SelectBuffer(pScrn, I810_FRONT);
      I810SubsequentSolidFillRect(pScrn, pbox->x1, pbox->y1, 
				  pbox->x2-pbox->x1, pbox->y2-pbox->y1);
      I810SelectBuffer(pScrn, I810_BACK);
      I810SubsequentSolidFillRect(pScrn, pbox->x1, pbox->y1, 
				  pbox->x2-pbox->x1, pbox->y2-pbox->y1);
      pbox++;
   }

   /* Clear the depth buffer - uses 0xffff rather than 0.
    */
   pbox = REGION_RECTS(prgn);
   nbox = REGION_NUM_RECTS(prgn);
   I810SelectBuffer(pScrn, I810_DEPTH);
   I810SetupForSolidFill(pScrn, 0xffff, GXcopy, -1);
   while (nbox--) {  
      I810SubsequentSolidFillRect(pScrn, pbox->x1, pbox->y1, 
				  pbox->x2-pbox->x1, pbox->y2-pbox->y1);
      pbox++;
   }
   I810SelectBuffer(pScrn, I810_FRONT);
   pI810->AccelInfoRec->NeedToSync = TRUE;
}

/* This routine is a modified form of XAADoBitBlt with the calls to
 * ScreenToScreenBitBlt built in. My routine has the prgnSrc as source
 * instead of destination. My origin is upside down so the ydir cases
 * are reversed. 
 *
 * KW: can you believe that this is called even when a 2d window moves?
 */
static void
I810DRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
		   RegionPtr prgnSrc, CARD32 index)
{
   ScreenPtr pScreen = pParent->drawable.pScreen;
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);
   BoxPtr pboxTmp, pboxNext, pboxBase;
   DDXPointPtr pptTmp, pptNew2;
   int xdir, ydir;

   int screenwidth = pScrn->virtualX;
   int screenheight = pScrn->virtualY;

   BoxPtr pbox = REGION_RECTS(prgnSrc);
   int nbox = REGION_NUM_RECTS(prgnSrc);

   BoxPtr pboxNew1 = 0;
   BoxPtr pboxNew2 = 0;
   DDXPointPtr pptNew1 = 0;
   DDXPointPtr pptSrc = &ptOldOrg;

   int dx = pParent->drawable.x - ptOldOrg.x;
   int dy = pParent->drawable.y - ptOldOrg.y;

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

   /* SelectBuffer isn't really a good concept for the i810.
    */
   I810EmitFlush(pScrn);
   I810SetupForScreenToScreenCopy(pScrn, xdir, ydir, GXcopy, -1, -1);
   for ( ; nbox-- ; pbox++ ) {
      
      int x1 = pbox->x1;
      int y1 = pbox->y1;
      int destx = x1 + dx;
      int desty = y1 + dy;
      int w = pbox->x2 - x1 + 1;
      int h = pbox->y2 - y1 + 1;
      
      if ( destx < 0 ) x1 -= destx, w += destx, destx = 0; 
      if ( desty < 0 ) y1 -= desty, h += desty, desty = 0;
      if ( destx + w > screenwidth ) w = screenwidth - destx;
      if ( desty + h > screenheight ) h = screenheight - desty;
      if ( w <= 0 ) continue;
      if ( h <= 0 ) continue;
      

      if (I810_DEBUG & DEBUG_VERBOSE_DRI)
	 ErrorF( "MoveBuffers %d,%d %dx%d dx: %d dy: %d\n",
		 x1, y1, w, h, dx, dy);

      I810SelectBuffer(pScrn, I810_BACK);
      I810SubsequentScreenToScreenCopy(pScrn, x1, y1, destx, desty, w, h);
      I810SelectBuffer(pScrn, I810_DEPTH);
      I810SubsequentScreenToScreenCopy(pScrn, x1, y1, destx, desty, w, h);
   }
   I810SelectBuffer(pScrn, I810_FRONT);
   I810EmitFlush(pScrn);

   if (pboxNew2) {
      DEALLOCATE_LOCAL(pptNew2);
      DEALLOCATE_LOCAL(pboxNew2);
   }
   if (pboxNew1) {
      DEALLOCATE_LOCAL(pptNew1);
      DEALLOCATE_LOCAL(pboxNew1);
   }

   pI810->AccelInfoRec->NeedToSync = TRUE;
}
