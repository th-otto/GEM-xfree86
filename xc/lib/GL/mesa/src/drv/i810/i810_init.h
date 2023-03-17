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

#ifndef _I810_INIT_H_
#define _I810_INIT_H_

#ifdef GLX_DIRECT_RENDERING

#include <sys/time.h>
#include "dri_tmm.h"
#include "dri_mesaint.h"
#include "dri_mesa.h"
#include "types.h"
#include "xmesaP.h"

typedef struct {
  drmHandle handle;
  drmSize size;
  drmAddress map;
} i810Region, *i810RegionPtr;

typedef struct {
   i810Region regs;

   int deviceID;
   int width;
   int height;
   int mem;

   int cpp;			/* for front and back buffers */
   int bitsPerPixel;

   int fbFormat;
   int fbOffset;
   int fbStride;

   int backOffset;
   int depthOffset;

   int auxPitch;
   int auxPitchBits;

   int textureOffset;
   int textureSize;
   int logTextureGranularity;

   __DRIscreenPrivate *driScrnPriv;

} i810ScreenPrivate;


#include "i810context.h"

extern void i810XMesaUpdateState( i810ContextPtr imesa );
extern void i810EmitHwStateLocked( i810ContextPtr imesa );
extern void i810EmitScissorValues( i810ContextPtr imesa, int box_nr, int emit );
extern void i810EmitDrawingRectangle( i810ContextPtr imesa );
extern void i810XMesaSetBackClipRects( i810ContextPtr imesa );
extern void i810XMesaSetFrontClipRects( i810ContextPtr imesa );


/* Lock the hardware and validate our state.  
 */
#define LOCK_HARDWARE( imesa )				\
  do {							\
    char __ret=0;					\
    DRM_CAS(imesa->driHwLock, imesa->hHWContext,	\
	    (DRM_LOCK_HELD|imesa->hHWContext), __ret);	\
    if (__ret) {					\
        drmGetLock(imesa->driFd, imesa->hHWContext, 0);	\
        i810XMesaUpdateState( imesa );			\
    }							\
  } while (0)


/* Unlock the hardware using the global current context 
 */
#define UNLOCK_HARDWARE(imesa)					\
    DRM_UNLOCK(imesa->driFd, imesa->driHwLock, imesa->hHWContext);	


/*
  This pair of macros makes a loop over the drawing operations
  so it is not self contained and doesn't have the nice single 
  statement semantics of most macros
*/
#define BEGIN_CLIP_LOOP(imesa)						\
  do {									\
    int _nc;								\
    LOCK_HARDWARE( imesa );						\
    for (_nc = imesa->numClipRects; _nc ; _nc--) {			\
      if (imesa->needClip)						\
        i810EmitScissorValues(imesa, _nc-1, 1);



#define END_CLIP_LOOP(imesa)			\
    }						\
    UNLOCK_HARDWARE(imesa);			\
  } while (0)

#endif
#endif
