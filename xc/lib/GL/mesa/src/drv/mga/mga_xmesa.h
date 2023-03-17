/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keithw@precisioninsight.com>
 *   Daryll Strauss <daryll@precisioninsight.com> (Origninal tdfx driver).
 *
 */

#ifndef _MGA_INIT_H_
#define _MGA_INIT_H_

#ifdef GLX_DIRECT_RENDERING

#include <sys/time.h>
#include "dri_tmm.h"
#include "dri_mesaint.h"
#include "dri_mesa.h"
#include "types.h"
#include "xmesaP.h"

typedef struct {

   int chipset;
   int width;
   int height;
   int mem;

   int cpp;			/* for front and back buffers */

   int Attrib;

   int frontOffset;
   int frontPitch;
   int backOffset;
   int backPitch;

   int depthOffset;
   int depthPitch;
   int depthCpp;

   int textureOffset;
   int textureSize;
   int logTextureGranularity;

   __DRIscreenPrivate *sPriv;
   drmBufMapPtr  bufs;

} mgaScreenPrivate;


#include "mgalib.h"

extern void mgaXMesaUpdateState( mgaContextPtr mmesa );
extern void mgaEmitHwStateLocked( mgaContextPtr mmesa );
extern void mgaEmitScissorValues( mgaContextPtr mmesa, int box_nr, int emit );
extern void mgaXMesaSetBackClipRects( mgaContextPtr mmesa );
extern void mgaXMesaSetFrontClipRects( mgaContextPtr mmesa );



/* Lock the hardware and validate our state.  
 */
#define LOCK_HARDWARE( mmesa )				\
  do {							\
    char __ret=0;					\
    DRM_CAS(mmesa->driHwLock, mmesa->hHWContext,	\
	    (DRM_LOCK_HELD|mmesa->hHWContext), __ret);	\
    if (__ret) {					\
        drmGetLock(mmesa->driFd, mmesa->hHWContext, 0);	\
        mgaXMesaUpdateState( mmesa );			\
    }							\
  } while (0)


/* Unlock the hardware using the global current context 
 */
#define UNLOCK_HARDWARE(mmesa)					\
    DRM_UNLOCK(mmesa->driFd, mmesa->driHwLock, mmesa->hHWContext);	


/* Freshen our snapshot of the drawables
 */
#define REFRESH_DRAWABLE_INFO( mmesa )		\
do {						\
   LOCK_HARDWARE( mmesa );			\
   UNLOCK_HARDWARE( mmesa );			\
} while (0)


#define GET_DRAWABLE_LOCK( mmesa ) while(0)
#define RELEASE_DRAWABLE_LOCK( mmesa ) while(0)

#endif
#endif
