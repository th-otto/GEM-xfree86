/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i810_dri.h,v 1.2 2000/03/02 16:07:49 martin Exp $ */

#ifndef _I810_DRI_
#define _I810_DRI_

#include <xf86drm.h>
#include "i810_drm_public.h"

#define I810_MAX_DRAWABLES 256

typedef struct {
   drmHandle regs;
   drmSize regsSize;
   drmAddress regsMap;

   drmSize backbufferSize;
   drmHandle backbuffer;

   drmSize depthbufferSize;
   drmHandle depthbuffer;

   drmHandle textures;
   int textureSize;

   drmHandle agp_buffers;
   drmSize agp_buf_size;
   
   int deviceID;
   int width;
   int height;
   int mem;
   int cpp;
   int bitsPerPixel;
   int fbOffset;
   int fbStride;

   int backOffset;
   int depthOffset;

   int auxPitch;
   int auxPitchBits;

   int logTextureGranularity;
   int textureOffset;

   /* For non-dma direct rendering.
    */
   int ringOffset;
   int ringSize;

   drmBufMapPtr drmBufs;
   int irq;

} I810DRIRec, *I810DRIPtr;

typedef struct {
  /* Nothing here yet */
  int dummy;
} I810ConfigPrivRec, *I810ConfigPrivPtr;

typedef struct {
  /* Nothing here yet */
  int dummy;
} I810DRIContextRec, *I810DRIContextPtr;


#endif
