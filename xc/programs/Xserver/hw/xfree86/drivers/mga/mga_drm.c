#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86Priv.h"
#include "mga_bios.h"
#include "mga_reg.h"
#include "mga.h"
#include "mga_macros.h"
#include "mga_dri.h"

#ifndef XFree86LOADER
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#include "xf86drm.h"
#include "drm.h"
#include "mga_drm_public.h"

Bool mgadrmCleanupDma(ScrnInfoPtr pScrn)
{
   MGAPtr pMGA = MGAPTR(pScrn);
   drm_mga_init_t init;
   
   memset(&init, 0, sizeof(drm_mga_init_t));
   init.func = MGA_CLEANUP_DMA;
   
   if(ioctl(pMGA->drmSubFD, DRM_IOCTL_MGA_INIT, &init)) {
      ErrorF("Mga Dma Cleanup Failed\n");
      return FALSE;
   }
   
   return TRUE;
}

int mgadrmInitDma(ScrnInfoPtr pScrn, int prim_size)
{
   MGAPtr pMGA = MGAPTR(pScrn);
   MGADRIPtr pMGADRI = (MGADRIPtr)pMGA->pDRIInfo->devPrivate;
   MGADRIServerPrivatePtr pMGADRIServer = pMGA->DRIServerInfo;
   drm_mga_init_t init;
   
   memset(&init, 0, sizeof(drm_mga_init_t));
   init.func = MGA_INIT_DMA;
   init.reserved_map_agpstart = 0;
   init.reserved_map_idx = 3;
   init.buffer_map_idx = 4;
   init.sarea_priv_offset = sizeof(XF86DRISAREARec);
   init.primary_size = prim_size;
   init.warp_ucode_size = pMGADRIServer->warp_ucode_size;

   switch(pMGA->Chipset) {
   case PCI_CHIP_MGAG400:
      init.chipset = MGA_CARD_TYPE_G400;
      break;
   case PCI_CHIP_MGAG200:
      init.chipset = MGA_CARD_TYPE_G200;
      break;
   case PCI_CHIP_MGAG200_PCI:
   default:
      ErrorF("Direct rendering not supported on this card/chipset\n");
      return FALSE;
   }

   init.fbOffset = pMGADRI->fbOffset;
   init.backOffset = pMGADRI->backOffset;
   init.depthOffset = pMGADRI->depthOffset;
   init.textureOffset = pMGADRI->textureOffset;
   init.textureSize = pMGADRI->textureSize;
   init.cpp = pMGADRI->cpp;
   init.stride = pMGADRI->stride;

   init.frontOrg = pMGA->DstOrg;
   init.backOrg = pMGADRI->backOffset; /*  */
   init.depthOrg = pMGADRI->textureOffset;
   init.mAccess = pMGA->MAccess;

   if (pMGA->HasSDRAM) 
      init.sgram = 0;
   else 
      init.sgram = 1;

   memcpy(&init.WarpIndex, &pMGADRIServer->WarpIndex, 
	  sizeof(mgaWarpIndex) * MGA_MAX_WARP_PIPES);



   ErrorF("Mga Dma Initialization start\n");

   if(ioctl(pMGA->drmSubFD, DRM_IOCTL_MGA_INIT, &init)) {
      ErrorF("Mga Dma Initialization Failed\n");
      return FALSE;
   }
   ErrorF("Mga Dma Initialization done\n");
   return TRUE;
}
