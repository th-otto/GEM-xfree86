/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_dri.h,v 1.1 2000/02/11 17:25:55 dawes Exp $ */

#ifndef _MGA_DRI_
#define _MGA_DRI_

#include <xf86drm.h>
#include "mga_drm_public.h"

typedef struct _mga_dri_server_private {
   int reserved_map_agpstart;
   int reserved_map_idx;
   int buffer_map_idx;
   int sarea_priv_offset;
   int primary_size;
   int warp_ucode_size;
   int chipset;
   int sgram;
   unsigned long agpMode;
   unsigned long agpHandle;
   drmHandle agp_private;
   drmSize agpSizep;
   drmAddress agpBase;
   int irq;
   drmHandle regs;
   drmSize regsSize;
   drmAddress regsMap;
   mgaWarpIndex WarpIndex[MGA_MAX_WARP_PIPES];
   drmBufMapPtr drmBufs;
   CARD8 *agp_map;
} MGADRIServerPrivate, *MGADRIServerPrivatePtr;

typedef struct {
  int deviceID;
  int width;
  int height;
  int mem;
  int cpp;
  int stride;
  int fbOffset;
  int backOffset;
  int depthOffset;
  int textureOffset;
  int textureSize;
  drmHandle agp;
  drmSize agpSize;
} MGADRIRec, *MGADRIPtr;



#endif
