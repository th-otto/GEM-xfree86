/* $XFree86: xc/programs/Xserver/GL/dri/dri.h,v 1.9 2000/03/04 01:53:02 martin Exp $ */
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
 *   Jens Owen <jens@precisioninsight.com>
 *
 */

/* Prototypes for DRI functions */

#ifndef _DRI_H_

typedef int DRISyncType;

#define DRI_NO_SYNC 0
#define DRI_2D_SYNC 1
#define DRI_3D_SYNC 2

typedef int DRIContextType;

typedef struct _DRIContextPrivRec DRIContextPrivRec, *DRIContextPrivPtr;

typedef enum _DRIContextFlags
{
    DRI_CONTEXT_2DONLY    = 0x01,
    DRI_CONTEXT_PRESERVED = 0x02,
    DRI_CONTEXT_RESERVED  = 0x04 /* DRI Only -- no kernel equivalent */
} DRIContextFlags;

#define DRI_NO_CONTEXT 0
#define DRI_2D_CONTEXT 1
#define DRI_3D_CONTEXT 2

typedef int DRISwapMethod;

#define DRI_HIDE_X_CONTEXT 0
#define DRI_SERVER_SWAP    1
#define DRI_KERNEL_SWAP    2

typedef int DRIWindowRequests;

#define DRI_NO_WINDOWS       0
#define DRI_3D_WINDOWS_ONLY  1
#define DRI_ALL_WINDOWS      2

/*
 * These functions can be wrapped by the DRI.  Each of these have
 * generic default funcs (initialized in DRICreateInfoRec) and can be
 * overridden by the driver in its [driver]DRIScreenInit function.
 */
typedef struct {
    ScreenWakeupHandlerProcPtr   WakeupHandler;
    ScreenBlockHandlerProcPtr    BlockHandler;
    PaintWindowBackgroundProcPtr PaintWindowBackground;
    PaintWindowBorderProcPtr     PaintWindowBorder;
    CopyWindowProcPtr            CopyWindow;
    ValidateTreeProcPtr          ValidateTree;
    PostValidateTreeProcPtr      PostValidateTree;
} DRIWrappedFuncsRec, *DRIWrappedFuncsPtr;

typedef struct {
    /* driver call back functions */
    Bool	(*CreateContext)(ScreenPtr pScreen,
				 VisualPtr visual,
				 drmContext hHWContext,
				 void* pVisualConfigPriv,
				 DRIContextType context);
    void        (*DestroyContext)(ScreenPtr pScreen,
				  drmContext hHWContext,
                                  DRIContextType context);
    void	(*SwapContext)(ScreenPtr pScreen,
			       DRISyncType syncType,
			       DRIContextType readContextType,
			       void* readContextStore,
			       DRIContextType writeContextType,
			       void* writeContextStore);
    void	(*InitBuffers)(WindowPtr pWin,
			       RegionPtr prgn,
			       CARD32 index);
    void	(*MoveBuffers)(WindowPtr pWin,
			       DDXPointRec ptOldOrg,
			       RegionPtr prgnSrc,
			       CARD32 index);

    /* wrapped functions */
    DRIWrappedFuncsRec  wrap;

    /* device info */
    char*		drmDriverName;
    char*		clientDriverName;
    char*		busIdString;
    int			ddxDriverMajorVersion;
    int			ddxDriverMinorVersion;
    int			ddxDriverPatchVersion;
    CARD32		frameBufferPhysicalAddress;
    long		frameBufferSize;
    long		frameBufferStride;
    long		SAREASize;
    int			maxDrawableTableEntry;
    int			ddxDrawableTableEntry;
    long		contextSize;
    DRISwapMethod	driverSwapMethod;
    DRIWindowRequests	bufferRequests;
    int			devPrivateSize;
    void*		devPrivate;
} DRIInfoRec, *DRIInfoPtr;

extern Bool DRIScreenInit(
    ScreenPtr pScreen,
    DRIInfoPtr pDRIInfo,
    int* pDRMFD);
void DRICloseScreen(ScreenPtr pScreen);
Bool DRIExtensionInit(void);
void DRIReset(void);
Bool DRIQueryDirectRenderingCapable( ScreenPtr pScreen, Bool* isCapable);
Bool DRIOpenConnection(
    ScreenPtr pScreen,
    drmHandlePtr hSAREA,
    char **busIdString
);
Bool DRIAuthConnection(ScreenPtr pScreen, drmMagic magic);
Bool DRICloseConnection( ScreenPtr pScreen);
Bool DRIGetClientDriverName(
    ScreenPtr pScreen,
    int* ddxDriverMajorVersion,
    int* ddxDriverMinorVersion,
    int* ddxDriverPatchVersion,
    char** clientDriverName
);
Bool DRICreateContext(
    ScreenPtr pScreen,
    VisualPtr visual,
    XID context,
    drmContextPtr pHWContext
);
Bool DRIDestroyContext( ScreenPtr pScreen, XID context);
Bool DRIContextPrivDelete(
    pointer pResource,
    XID id);
Bool DRICreateDrawable(
    ScreenPtr pScreen,
    Drawable id,
    DrawablePtr pDrawable,
    drmDrawablePtr hHWDrawable
);
Bool DRIDestroyDrawable( ScreenPtr pScreen, 
    Drawable id,
    DrawablePtr pDrawable);
Bool DRIDrawablePrivDelete(
    pointer pResource,
    XID id);
Bool DRIGetDrawableInfo(
    ScreenPtr pScreen,
    DrawablePtr pDrawable,
    unsigned int* index,
    unsigned int* stamp,
    int* X,
    int* Y,
    int* W,
    int* H,
    int* numClipRects,
    XF86DRIClipRectPtr* pClipRects
);
Bool DRIGetDeviceInfo(
    ScreenPtr pScreen,
    drmHandlePtr hFrameBuffer,
    int* fbOrigin,
    int* fbSize,
    int* fbStride,
    int* devPrivateSize,
    void** pDevPrivate
);
DRIInfoPtr DRICreateInfoRec(void);
void DRIDestroyInfoRec(DRIInfoPtr DRIInfo);
Bool DRIFinishScreenInit(ScreenPtr pScreen);
void DRIWakeupHandler(
    pointer wakeupData,
    int result,
    pointer pReadmask);
void DRIBlockHandler(
    pointer blockData,
    OSTimePtr pTimeout,
    pointer pReadmask);
void DRIDoWakeupHandler(
    int screenNum,
    pointer wakeupData,
    unsigned long result,
    pointer pReadmask);
void DRIDoBlockHandler(
    int screenNum,
    pointer blockData,
    pointer pTimeout,
    pointer pReadmask);
void DRISwapContext(
    int drmFD,
    void *oldctx,
    void *newctx);
void* DRIGetContextStore(DRIContextPrivPtr context);
void DRIPaintWindow(
    WindowPtr pWin,
    RegionPtr prgn,
    int what);
void DRICopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc);
int DRIValidateTree(
    WindowPtr pParent,
    WindowPtr pChild,
    VTKind    kind);
void DRIPostValidateTree(
    WindowPtr pParent,
    WindowPtr pChild,
    VTKind    kind);
void DRIClipNotify(
    WindowPtr pWin,
    int dx,
    int dy);
CARD32 DRIGetDrawableIndex(
    WindowPtr pWin);
void DRILock(ScreenPtr pScreen, int flags);
void DRIUnlock(ScreenPtr pScreen);
void *DRIGetSAREAPrivate(ScreenPtr pScreen);
DRIContextPrivPtr
DRICreateContextPriv(ScreenPtr pScreen,
		     drmContextPtr pHWContext,
		     DRIContextFlags flags);
DRIContextPrivPtr
DRICreateContextPrivFromHandle(ScreenPtr pScreen,
			       drmContext hHWContext,
			       DRIContextFlags flags);
Bool DRIDestroyContextPriv(DRIContextPrivPtr pDRIContextPriv);

#define _DRI_H_

#endif
