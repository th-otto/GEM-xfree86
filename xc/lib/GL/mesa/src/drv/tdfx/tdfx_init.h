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
 *   Daryll Strauss <daryll@precisioninsight.com>
 *
 */

#ifndef _TDFX_INIT_H_
#define _TDFX_INIT_H_

#ifdef GLX_DIRECT_RENDERING

#include <sys/time.h>
#include <glide.h>
#include "dri_tmm.h"
#include "dri_mesaint.h"
#include "dri_mesa.h"
#include "types.h"
#include "xmesaP.h"
/*  #include "fxdrv.h" */

typedef struct {
  drmHandle handle;
  drmSize size;
  drmAddress map;
} tdfxRegion, *tdfxRegionPtr;

typedef struct {
  tdfxRegion regs;
  int deviceID;
  int width;
  int height;
  int mem;
  int cpp;
  int stride;
  int fifoOffset;
  int fifoSize;
  int fbOffset;
  int backOffset;
  int depthOffset;
  int textureOffset;
  int textureSize;
  __DRIscreenPrivate *driScrnPriv;
} tdfxScreenPrivate;

typedef struct {
  volatile int fifoPtr;
  volatile int fifoRead;
  volatile int fifoOwner;
  volatile int ctxOwner;
  volatile int texOwner;
} TDFXSAREAPriv;

/* KW: The extra stuff we need to add to an fxContext to make it
 * equivalent to a tdfxContextPrivate struct.  It may be nice to
 * package this up in a struct, but in the meantime this means we
 * don't have to be merging stuff by hand between unrelated files.
 *
 * PLEASE NOTE: if you add stuff here, you have to make sure you only
 * try to access it from places which are protected by tests for
 * defined(GLX_DIRECT_RENDERING) or defined(XFree86Server).
 */
#define DRI_FX_CONTEXT				\
  drmContext hHWContext;			\
  int numClipRects;				\
  XF86DRIClipRectPtr pClipRects;		\
  tdfxScreenPrivate *tdfxScrnPriv;

typedef struct tfxMesaContext tdfxContextPrivate;

#include "fxdrv.h"

extern GLboolean tdfxMapAllRegions(__DRIscreenPrivate *driScrnPriv);
extern void tdfxUnmapAllRegions(__DRIscreenPrivate *driScrnPriv);
extern GLboolean tdfxInitHW(XMesaContext c);

extern void XMesaWindowMoved(void);
extern void XMesaUpdateState(int windowMoved);
extern void XMesaSetSAREA(void);

/* This is the private interface between Glide and DRI */
extern void grDRIOpen(char *pFB, char *pRegs, int deviceID, 
		      int width, int height, 
		      int mem, int cpp, int stride, 
		      int fifoOffset, int fifoSize, 
		      int fbOffset, int backOffset, int depthOffset, 
		      int textureOffset, int textureSize, 
		      volatile int *fifoPtr, volatile int *fifoRead);
extern void grDRIPosition(int x, int y, int w, int h, 
			  int numClip, XF86DRIClipRectPtr pClip);
extern void grDRILostContext(void);
extern void grDRIImportFifo(int fifoPtr, int fifoRead);
extern void grDRIInvalidateAll(void);
extern void grDRIResetSAREA(void);

extern XMesaContext gCC;
extern tdfxContextPrivate *gCCPriv;

/* You can turn this on to find locking conflicts.
#define DEBUG_LOCKING 
*/

#ifdef DEBUG_LOCKING
extern char *prevLockFile;
extern int prevLockLine;
#define DEBUG_LOCK() \
  do { \
    prevLockFile=(__FILE__); \
    prevLockLine=(__LINE__); \
  } while (0)
#define DEBUG_RESET() \
  do { \
    prevLockFile=0; \
    prevLockLine=0; \
  } while (0)
#define DEBUG_CHECK_LOCK() \
  do { \
    if (prevLockFile) { \
      fprintf(stderr, "LOCK SET!\n\tPrevious %s:%d\n\tCurrent: %s:%d\n", \
	prevLockFile, prevLockLine, __FILE__, __LINE__); \
      exit(1); \
    } \
  } while (0)
#else
#define DEBUG_LOCK()
#define DEBUG_RESET()
#define DEBUG_CHECK_LOCK()    
#endif

/* !!! We may want to separate locks from locks with validation.
   This could be used to improve performance for those things
   commands that do not do any drawing !!! */

#define DRM_LIGHT_LOCK_RETURN(fd,lock,context,__ret)                   \
	do {                                                           \
		DRM_CAS(lock,context,DRM_LOCK_HELD|context,__ret);     \
                if (__ret) drmGetLock(fd,context,0);                   \
        } while(0)

/* Lock the hardware using the global current context */
#define LOCK_HARDWARE() \
  do { \
    char __ret=0; \
    __DRIdrawablePrivate *dPriv = gCC->driContextPriv->driDrawablePriv; \
    __DRIscreenPrivate *sPriv = dPriv->driScreenPriv; \
    DEBUG_CHECK_LOCK(); \
    DRM_CAS(&sPriv->pSAREA->lock, dPriv->driContextPriv->hHWContext, \
	    DRM_LOCK_HELD|dPriv->driContextPriv->hHWContext, __ret); \
    if (__ret) { \
        int stamp; \
	drmGetLock(sPriv->fd, dPriv->driContextPriv->hHWContext, 0); \
        stamp=dPriv->lastStamp; \
        XMESA_VALIDATE_DRAWABLE_INFO(gCC->display, sPriv, dPriv); \
        if (*(dPriv->pStamp)!=stamp) XMesaUpdateState(GL_TRUE); \
	else XMesaUpdateState(GL_FALSE); \
    } \
    DEBUG_LOCK(); \
  } while (0)

/* Unlock the hardware using the global current context */
#define UNLOCK_HARDWARE() \
  do { \
    __DRIdrawablePrivate *dPriv = gCC->driContextPriv->driDrawablePriv; \
    __DRIscreenPrivate *sPriv = dPriv->driScreenPriv; \
    XMesaSetSAREA(); \
    DRM_UNLOCK(sPriv->fd, &sPriv->pSAREA->lock,  \
	       dPriv->driContextPriv->hHWContext); \
    DEBUG_RESET(); \
  } while (0)

#define BEGIN_BOARD_LOCK() LOCK_HARDWARE()
#define END_BOARD_LOCK() UNLOCK_HARDWARE()

/*
  This pair of macros makes a loop over the drawing operations
  so it is not self contained and doesn't have the nice single 
  statement semantics of most macros
*/
#define BEGIN_CLIP_LOOP()	\
  do {				\
    __DRIdrawablePrivate *dPriv = gCC->driContextPriv->driDrawablePriv; \
    int _nc; \
    LOCK_HARDWARE(); \
    _nc = dPriv->numClipRects; \
    while (_nc--) { \
      if (gCCPriv->needClip) { \
        gCCPriv->clipMinX=dPriv->pClipRects[_nc].x1; \
        gCCPriv->clipMaxX=dPriv->pClipRects[_nc].x2; \
        gCCPriv->clipMinY=dPriv->pClipRects[_nc].y1; \
        gCCPriv->clipMaxY=dPriv->pClipRects[_nc].y2; \
        fxSetScissorValues(gCCPriv->glCtx); \
      }

#define END_CLIP_LOOP() \
    } \
    UNLOCK_HARDWARE(); \
  } while (0)

#endif
#endif
