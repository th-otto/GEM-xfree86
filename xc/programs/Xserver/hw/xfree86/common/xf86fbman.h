/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86fbman.h,v 1.8 1999/06/27 14:07:57 dawes Exp $ */

#ifndef _XF86FBMAN_H
#define _XF86FBMAN_H


#include "scrnintstr.h"
#include "regionstr.h"


#define FAVOR_AREA_THEN_WIDTH		0
#define FAVOR_AREA_THEN_HEIGHT		1
#define FAVOR_WIDTH_THEN_AREA		2
#define FAVOR_HEIGHT_THEN_AREA		3

#define PRIORITY_LOW			0
#define PRIORITY_NORMAL			1
#define PRIORITY_EXTREME		2


typedef struct _FBArea {
   ScreenPtr    pScreen;
   BoxRec   	box;
   int 		granularity;
   void 	(*MoveAreaCallback)(struct _FBArea*, struct _FBArea*);
   void 	(*RemoveAreaCallback)(struct _FBArea*);
   DevUnion 	devPrivate;
} FBArea, *FBAreaPtr;

typedef struct _FBLink {
  FBArea area;
  struct _FBLink *next;  
} FBLink, *FBLinkPtr;

typedef void (*FreeBoxCallbackProcPtr)(ScreenPtr, RegionPtr, pointer);
typedef void (*MoveAreaCallbackProcPtr)(FBAreaPtr, FBAreaPtr);
typedef void (*RemoveAreaCallbackProcPtr)(FBAreaPtr);

typedef struct {
   ScreenPtr	pScreen;
   RegionPtr	InitialBoxes;
   RegionPtr	FreeBoxes;
   FBLinkPtr 	UsedAreas;
   int		NumUsedAreas;
   CloseScreenProcPtr 		CloseScreen;
   int				NumCallbacks;
   FreeBoxCallbackProcPtr	*FreeBoxesUpdateCallback;
   DevUnion			*devPrivates;
} FBManager, *FBManagerPtr;



Bool
xf86InitFBManagerRegion(
    ScreenPtr pScreen, 
    RegionPtr ScreenRegion
);

Bool
xf86InitFBManager(
    ScreenPtr pScreen, 
    BoxPtr FullBox
);

Bool 
xf86FBManagerRunning(
    ScreenPtr pScreen
);

FBAreaPtr 
xf86AllocateOffscreenArea (
   ScreenPtr pScreen, 
   int w, int h,
   int granularity,
   MoveAreaCallbackProcPtr moveCB,
   RemoveAreaCallbackProcPtr removeCB,
   pointer privData
);

FBAreaPtr 
xf86AllocateLinearOffscreenArea (
   ScreenPtr pScreen, 
   int length,
   int granularity,
   MoveAreaCallbackProcPtr moveCB,
   RemoveAreaCallbackProcPtr removeCB,
   pointer privData
);

void xf86FreeOffscreenArea(FBAreaPtr area);

Bool 
xf86ResizeOffscreenArea(
   FBAreaPtr resize,
   int w, int h
);

Bool
xf86RegisterFreeBoxCallback(
    ScreenPtr pScreen,  
    FreeBoxCallbackProcPtr FreeBoxCallback,
    pointer devPriv
);

Bool
xf86PurgeUnlockedOffscreenAreas(
    ScreenPtr pScreen
);


Bool
xf86QueryLargestOffscreenArea(
    ScreenPtr pScreen,
    int *width, int *height,
    int granularity,
    int preferences,
    int priority
);

#endif /* _XF86FBMAN_H */
