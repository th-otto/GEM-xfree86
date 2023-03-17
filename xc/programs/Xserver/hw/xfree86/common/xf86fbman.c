/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86fbman.c,v 1.13 1999/11/28 20:29:37 mvojkovi Exp $ */

#include "misc.h"
#include "xf86.h"

#include "X.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "xf86fbman.h"


static Bool xf86FBCloseScreen(int, ScreenPtr);
static unsigned long xf86FBGeneration = 0;
static int xf86FBScreenIndex = -1;
static void SendCallFreeBoxCallbacks(FBManagerPtr);
static FBAreaPtr AllocateArea(
   FBManagerPtr offman,
   int w, int h,
   int granularity,
   MoveAreaCallbackProcPtr moveCB,
   RemoveAreaCallbackProcPtr removeCB,
   pointer privData
);


Bool
xf86FBManagerRunning(ScreenPtr pScreen)
{
    if(xf86FBScreenIndex < 0) 
	return FALSE;
    if(!pScreen->devPrivates[xf86FBScreenIndex].ptr) 
	return FALSE;

    return TRUE;
}

Bool
xf86InitFBManager(
    ScreenPtr pScreen,  
    BoxPtr FullBox
){
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   RegionRec ScreenRegion;
   RegionRec FullRegion;
   BoxRec ScreenBox;
   Bool ret;

   ScreenBox.x1 = 0;
   ScreenBox.y1 = 0;
   ScreenBox.x2 = pScrn->virtualX;
   ScreenBox.y2 = pScrn->virtualY;

   if((FullBox->x1 >  ScreenBox.x1) || (FullBox->y1 >  ScreenBox.y1) ||
      (FullBox->x2 <  ScreenBox.x2) || (FullBox->y2 <  ScreenBox.y2)) {
	return FALSE;   
   }

   REGION_INIT(pScreen, &ScreenRegion, &ScreenBox, 1); 
   REGION_INIT(pScreen, &FullRegion, FullBox, 1); 

   REGION_SUBTRACT(pScreen, &FullRegion, &FullRegion, &ScreenRegion);

   ret = xf86InitFBManagerRegion(pScreen, &FullRegion);

   REGION_UNINIT(pScreen, &ScreenRegion);
   REGION_UNINIT(pScreen, &FullRegion);
    
   return ret;
}

Bool
xf86InitFBManagerRegion(
    ScreenPtr pScreen,  
    RegionPtr FullRegion
){
   FBManagerPtr offman;

   if(REGION_NIL(FullRegion))
	return FALSE;

   if(xf86FBGeneration != serverGeneration) {
	if((xf86FBScreenIndex = AllocateScreenPrivateIndex()) < 0)
		return FALSE;
	xf86FBGeneration = serverGeneration;
   }

   offman = xalloc(sizeof(FBManager));
   if(!offman) return FALSE;

   pScreen->devPrivates[xf86FBScreenIndex].ptr = (pointer)offman;

   offman->CloseScreen = pScreen->CloseScreen;
   pScreen->CloseScreen = xf86FBCloseScreen;

   offman->InitialBoxes = REGION_CREATE(pScreen, NULL, 1);
   offman->FreeBoxes = REGION_CREATE(pScreen, NULL, 1);

   REGION_COPY(pScreen, offman->InitialBoxes, FullRegion);
   REGION_COPY(pScreen, offman->FreeBoxes, FullRegion);

   offman->pScreen = pScreen;
   offman->UsedAreas = NULL;
   offman->NumUsedAreas = 0;  
   offman->NumCallbacks = 0;
   offman->FreeBoxesUpdateCallback = NULL;
   offman->devPrivates = NULL;

   return TRUE;
} 

static void
SendCallFreeBoxCallbacks(FBManagerPtr offman)
{
   int i = offman->NumCallbacks;

   while(i--) {
	(*offman->FreeBoxesUpdateCallback[i])(
	   offman->pScreen, offman->FreeBoxes, offman->devPrivates[i].ptr);
   }
}

Bool
xf86RegisterFreeBoxCallback(
    ScreenPtr pScreen,  
    FreeBoxCallbackProcPtr FreeBoxCallback,
    pointer devPriv
){
   FBManagerPtr offman;
   FreeBoxCallbackProcPtr *newCallbacks;
   DevUnion *newPrivates; 

   if(!xf86FBManagerRunning(pScreen)) 
	return FALSE;

   offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;

   newCallbacks = xrealloc( offman->FreeBoxesUpdateCallback, 
		sizeof(FreeBoxCallbackProcPtr) * (offman->NumCallbacks + 1));

   newPrivates = xrealloc(offman->devPrivates,
			  sizeof(DevUnion) * (offman->NumCallbacks + 1));

   if(!newCallbacks || !newPrivates)
	return FALSE;     

   offman->FreeBoxesUpdateCallback = newCallbacks;
   offman->devPrivates = newPrivates;

   offman->FreeBoxesUpdateCallback[offman->NumCallbacks] = FreeBoxCallback;
   offman->devPrivates[offman->NumCallbacks].ptr = devPriv;
   offman->NumCallbacks++;

   SendCallFreeBoxCallbacks(offman);

   return TRUE;
}


static FBAreaPtr
AllocateArea(
   FBManagerPtr offman,
   int w, int h,
   int granularity,
   MoveAreaCallbackProcPtr moveCB,
   RemoveAreaCallbackProcPtr removeCB,
   pointer privData
){
   ScreenPtr pScreen = offman->pScreen;
   FBLinkPtr link = NULL;
   FBAreaPtr area = NULL;
   RegionRec NewReg;
   int i, x = 0, num;
   BoxPtr boxp;

   if(granularity <= 1) granularity = 0;

   boxp = REGION_RECTS(offman->FreeBoxes);
   num = REGION_NUM_RECTS(offman->FreeBoxes);

   /* look through the free boxes */
   for(i = 0; i < num; i++, boxp++) {
	x = boxp->x1;
	if(granularity) {
	    int tmp = x % granularity;
	    if(tmp) x += (granularity - tmp);
	}

	if(((boxp->y2 - boxp->y1) < h) || ((boxp->x2 - x) < w))
	   continue;

	link = xalloc(sizeof(FBLink));
	if(!link) return NULL;

        area = &(link->area);
	break;
   }

   /* try to boot a removeable one out if we are not expendable ourselves */
   if(!area && !removeCB) {
	link = offman->UsedAreas;

	while(link) {
	   if(!link->area.RemoveAreaCallback) {
		link = link->next;
		continue;
	   }

	   boxp = &(link->area.box);
	   x = boxp->x1;
 	   if(granularity) {
		int tmp = x % granularity;
		if(tmp) x += (granularity - tmp);
	   }

	   if(((boxp->y2 - boxp->y1) < h) || ((boxp->x2 - x) < w)) {
		link = link->next;
		continue;
	   }

	   /* bye, bye */
	   (*link->area.RemoveAreaCallback)(&link->area);
	   REGION_INIT(pScreen, &NewReg, &(link->area.box), 1); 
	   REGION_UNION(pScreen, offman->FreeBoxes, offman->FreeBoxes, &NewReg);
	   REGION_UNINIT(pScreen, &NewReg); 

	   offman->NumUsedAreas--;

           area = &(link->area);
	   break;
	}
   }

   if(area) {
	area->pScreen = pScreen;
	area->granularity = granularity;
	area->box.x1 = x;
	area->box.x2 = x + w;
	area->box.y1 = boxp->y1;
	area->box.y2 = boxp->y1 + h;
	area->MoveAreaCallback = moveCB;
	area->RemoveAreaCallback = removeCB;
	area->devPrivate.ptr = privData;

        REGION_INIT(pScreen, &NewReg, &(area->box), 1);
	REGION_SUBTRACT(pScreen, offman->FreeBoxes, offman->FreeBoxes, &NewReg);
	REGION_UNINIT(pScreen, &NewReg);

	link->next = offman->UsedAreas;
	offman->UsedAreas = link;
	offman->NumUsedAreas++;
   }

   return area;
}

FBAreaPtr
xf86AllocateOffscreenArea(
   ScreenPtr pScreen, 
   int w, int h,
   int gran,
   MoveAreaCallbackProcPtr moveCB,
   RemoveAreaCallbackProcPtr removeCB,
   pointer privData
){
   FBManagerPtr offman;
   FBAreaPtr area = NULL;

   if(!xf86FBManagerRunning(pScreen)) return NULL;

   offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;

   if((area = AllocateArea(offman, w, h, gran, moveCB, removeCB, privData)))
	SendCallFreeBoxCallbacks(offman);

   return area;
}

FBAreaPtr
xf86AllocateLinearOffscreenArea(
    ScreenPtr pScreen, 
    int length,
    int gran,
    MoveAreaCallbackProcPtr moveCB,
    RemoveAreaCallbackProcPtr removeCB,
    pointer privData
){
    FBManagerPtr offman;
    FBLinkPtr link = NULL;
    FBAreaPtr area = NULL;
    RegionRec NewReg;
    BoxPtr boxp, box1p = NULL;
    int i, num, w, h;

    if (!xf86FBManagerRunning(pScreen)) return NULL;

    offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;

    if (offman->InitialBoxes->extents.x1 != 0 ||
	length <= 0) return NULL;

    w = offman->InitialBoxes->extents.x2 -
	offman->InitialBoxes->extents.x1;
    h = (length + w - 1) / w;

    /* look through the free boxes,
       bottom up to reduce fragmentation troubles */

    boxp = REGION_RECTS(offman->FreeBoxes);
    num = REGION_NUM_RECTS(offman->FreeBoxes);

    for (i = 0; i < num; i++, boxp++) {
	if (((boxp->y2 - boxp->y1) < h) ||
	    ((boxp->x2 - boxp->x1) < w) ||
	    (box1p && box1p->y1 > boxp->y1))
	    continue;

        box1p = boxp;
    }

    if (!box1p) return NULL;
    link = xalloc(sizeof(FBLink));
    if (!link) return NULL;
    area = &(link->area);

    area->pScreen = pScreen;
    area->granularity = gran;
    area->box.x1 = box1p->x1; /* Presumed zero */
    area->box.x2 = box1p->x1 + w;
    area->box.y1 = box1p->y2 - h;
    area->box.y2 = box1p->y2;
    area->MoveAreaCallback = moveCB;
    area->RemoveAreaCallback = removeCB;
    area->devPrivate.ptr = privData;

    REGION_INIT(pScreen, &NewReg, &(area->box), 1);
    REGION_SUBTRACT(pScreen, offman->FreeBoxes, offman->FreeBoxes, &NewReg);
    REGION_UNINIT(pScreen, &NewReg);

    link->next = offman->UsedAreas;
    offman->UsedAreas = link;
    offman->NumUsedAreas++;

    SendCallFreeBoxCallbacks(offman);

    return area;
}

void
xf86FreeOffscreenArea(FBAreaPtr area)
{
   FBManagerPtr offman;
   FBLinkPtr pLink, pLinkPrev = NULL;
   RegionRec FreedRegion;
   ScreenPtr pScreen;

   if(!area || !xf86FBManagerRunning(area->pScreen)) 
	return;

   pScreen = area->pScreen;
   offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;
       
   pLink = offman->UsedAreas;
   if(!pLink) return;  
 
   while(&(pLink->area) != area) {
	pLinkPrev = pLink;
	pLink = pLink->next;
	if(!pLink) return;
   }

   /* put the area back into the pool */
   REGION_INIT(pScreen, &FreedRegion, &(pLink->area.box), 1); 
   REGION_UNION(pScreen, offman->FreeBoxes, offman->FreeBoxes, &FreedRegion);
   REGION_UNINIT(pScreen, &FreedRegion); 

   if(pLinkPrev)
	pLinkPrev->next = pLink->next;
   else offman->UsedAreas = pLink->next;

   xfree(pLink); 
   offman->NumUsedAreas--;

   SendCallFreeBoxCallbacks(offman);
}
   


Bool
xf86ResizeOffscreenArea(
   FBAreaPtr resize,
   int w, int h
){
   FBManagerPtr offman;
   ScreenPtr pScreen;
   BoxRec OrigArea;
   RegionRec FreedReg;
   FBAreaPtr area = NULL;
   FBLinkPtr pLink, newLink, pLinkPrev = NULL;

   if(!resize || !xf86FBManagerRunning(resize->pScreen)) 
	return FALSE;

   pScreen = resize->pScreen;
   offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;

   /* find this link */
   if(!(pLink = offman->UsedAreas))
	return FALSE;  
 
   while(&(pLink->area) != resize) {
	pLinkPrev = pLink;
	pLink = pLink->next;
	if(!pLink) return FALSE;
   }

   OrigArea.x1 = resize->box.x1;
   OrigArea.x2 = resize->box.x2;
   OrigArea.y1 = resize->box.y1;
   OrigArea.y2 = resize->box.y2;

   /* if it's smaller, this is easy */

   if((w <= (resize->box.x2 - resize->box.x1)) && 
      (h <= (resize->box.y2 - resize->box.y1))) {
	RegionRec NewReg;

	resize->box.x2 = resize->box.x1 + w;
	resize->box.y2 = resize->box.y1 + h;

        if((resize->box.y2 == OrigArea.y2) &&
	   (resize->box.x2 == OrigArea.x2))
		return TRUE;

	REGION_INIT(pScreen, &FreedReg, &OrigArea, 1); 
	REGION_INIT(pScreen, &NewReg, &(resize->box), 1); 
	REGION_SUBTRACT(pScreen, &FreedReg, &FreedReg, &NewReg);
	REGION_UNION(pScreen, offman->FreeBoxes, offman->FreeBoxes, &FreedReg);
	REGION_UNINIT(pScreen, &FreedReg); 
	REGION_UNINIT(pScreen, &NewReg); 

	SendCallFreeBoxCallbacks(offman);

	return TRUE;
   }


   /* otherwise we remove the old region */

   REGION_INIT(pScreen, &FreedReg, &OrigArea, 1); 
   REGION_UNION(pScreen, offman->FreeBoxes, offman->FreeBoxes, &FreedReg);
  
   /* remove the old link */
   if(pLinkPrev)
	pLinkPrev->next = pLink->next;
   else offman->UsedAreas = pLink->next;

   /* and try to add a new one */

   if((area = AllocateArea(offman, w, h, resize->granularity,
		resize->MoveAreaCallback, resize->RemoveAreaCallback,
		resize->devPrivate.ptr))) {

        /* copy data over to our link and replace the new with old */
	memcpy(resize, area, sizeof(FBArea));

        pLinkPrev = NULL;
 	newLink = offman->UsedAreas;

        while(&(newLink->area) != area) {
	    pLinkPrev = newLink;
	    newLink = newLink->next;
        }

	if(pLinkPrev)
	    pLinkPrev->next = newLink->next;
	else offman->UsedAreas = newLink->next;

        pLink->next = offman->UsedAreas;
        offman->UsedAreas = pLink;

	xfree(newLink);

	/* AllocateArea added one but we really only exchanged one */
	offman->NumUsedAreas--;  
   } else {
      /* reinstate the old region */
      REGION_SUBTRACT(pScreen, offman->FreeBoxes, offman->FreeBoxes, &FreedReg);
      REGION_UNINIT(pScreen, &FreedReg); 

      pLink->next = offman->UsedAreas;
      offman->UsedAreas = pLink;
      return FALSE;
   }


   REGION_UNINIT(pScreen, &FreedReg); 

   SendCallFreeBoxCallbacks(offman);

   return TRUE;
}



Bool
xf86FBCloseScreen (int i, ScreenPtr pScreen)
{
   FBLinkPtr pLink, tmp;
   FBManagerPtr offman = 
	(FBManagerPtr) pScreen->devPrivates[xf86FBScreenIndex].ptr;

   
   pScreen->CloseScreen = offman->CloseScreen;

   pLink = offman->UsedAreas;

   while(pLink) {
	tmp = pLink;
	pLink = pLink->next;
	xfree(tmp);
   }

   REGION_DESTROY(pScreen, offman->InitialBoxes);
   REGION_DESTROY(pScreen, offman->FreeBoxes);

   xfree(offman);

   return (*pScreen->CloseScreen) (i, pScreen);
}


Bool
xf86PurgeUnlockedOffscreenAreas(ScreenPtr pScreen)
{
   FBManagerPtr offman;
   FBLinkPtr pLink, tmp, pPrev = NULL;
   RegionRec FreedRegion;
   Bool anyUsed = FALSE;

   if(!xf86FBManagerRunning(pScreen)) 
	return FALSE;

   offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;
       
   pLink = offman->UsedAreas;
   if(!pLink) return TRUE;  
 
   while(pLink) {
	if(pLink->area.RemoveAreaCallback) {
	    (*pLink->area.RemoveAreaCallback)(&pLink->area);

	    REGION_INIT(pScreen, &FreedRegion, &(pLink->area.box), 1); 
	    REGION_APPEND(pScreen, offman->FreeBoxes, &FreedRegion);
	    REGION_UNINIT(pScreen, &FreedRegion); 

	    if(pPrev)
	      pPrev->next = pLink->next;
	    else offman->UsedAreas = pLink->next;

	    tmp = pLink;
	    pLink = pLink->next;
  	    xfree(tmp); 
	    offman->NumUsedAreas--;
	    anyUsed = TRUE;
	} else {
	    pPrev = pLink;
	    pLink = pLink->next;
	}
   }

   if(anyUsed) {
	REGION_VALIDATE(pScreen, offman->FreeBoxes, &anyUsed);
	SendCallFreeBoxCallbacks(offman);
   }

   return TRUE;
}


Bool
xf86QueryLargestOffscreenArea(
    ScreenPtr pScreen,
    int *width, int *height,
    int granularity,
    int preferences,
    int severity
){
    FBManagerPtr offman;
    RegionPtr newRegion = NULL;
    BoxPtr pbox;
    int nbox;
    int x, w, h, area, oldArea;

    *width = *height = oldArea = 0;

    if(!xf86FBManagerRunning(pScreen)) 
	return FALSE;

    if(granularity <= 1) granularity = 0;

    if((preferences < 0) || (preferences > 3))
	return FALSE;	

    offman = pScreen->devPrivates[xf86FBScreenIndex].ptr;

    if(severity < 0) severity = 0;
    if(severity > 2) severity = 2;

    switch(severity) {
    case 2:
	if(offman->NumUsedAreas) {
	    FBLinkPtr pLink;
	    RegionRec tmpRegion;
	    newRegion = REGION_CREATE(pScreen, NULL, 1);
	    REGION_COPY(pScreen, newRegion, offman->InitialBoxes);
	    pLink = offman->UsedAreas;

	    while(pLink) {
		if(!pLink->area.RemoveAreaCallback) {
		    REGION_INIT(pScreen, &tmpRegion, &(pLink->area.box), 1);
		    REGION_SUBTRACT(pScreen, newRegion, newRegion, &tmpRegion);
		    REGION_UNINIT(pScreen, &tmpRegion);
		}
		pLink = pLink->next;
	    }

	    nbox = REGION_NUM_RECTS(newRegion);
	    pbox = REGION_RECTS(newRegion);
	    break;
	}
    case 1:
	if(offman->NumUsedAreas) {
	    FBLinkPtr pLink;
	    RegionRec tmpRegion;
	    newRegion = REGION_CREATE(pScreen, NULL, 1);
	    REGION_COPY(pScreen, newRegion, offman->FreeBoxes);
	    pLink = offman->UsedAreas;

	    while(pLink) {
		if(pLink->area.RemoveAreaCallback) {
		    REGION_INIT(pScreen, &tmpRegion, &(pLink->area.box), 1);
		    REGION_APPEND(pScreen, newRegion, &tmpRegion);
		    REGION_UNINIT(pScreen, &tmpRegion);
		}
		pLink = pLink->next;
	    }

	    nbox = REGION_NUM_RECTS(newRegion);
	    pbox = REGION_RECTS(newRegion);
	    break;
	}
    default:
	nbox = REGION_NUM_RECTS(offman->FreeBoxes);
	pbox = REGION_RECTS(offman->FreeBoxes);
	break;
    }

    while(nbox--) {
	x = pbox->x1;
	if(granularity) {
	   int tmp = x % granularity;
	   if(tmp) x += (granularity - tmp);
        }

	w = pbox->x2 - x;
	h = pbox->y2 - pbox->y1;
	area = w * h;

	if(w > 0) {
	    Bool gotIt = FALSE;
	    switch(preferences) {
	    case FAVOR_AREA_THEN_WIDTH:
		if((area > oldArea) || ((area == oldArea) && (w > *width))) 
		    gotIt = TRUE;
		break;
	    case FAVOR_AREA_THEN_HEIGHT:
		if((area > oldArea) || ((area == oldArea) && (h > *height)))
		    gotIt = TRUE;
		break;
	    case FAVOR_WIDTH_THEN_AREA:
		if((w > *width) || ((w == *width) && (area > oldArea)))
		    gotIt = TRUE;
		break;
	    case FAVOR_HEIGHT_THEN_AREA:
		if((h > *height) || ((h == *height) && (area > oldArea)))
		    gotIt = TRUE;
		break;
	    }
	    if(gotIt) {
		*width = w;
		*height = h;
		oldArea = area;
	    }
        }
	pbox++;
    }

    if(newRegion)
	REGION_DESTROY(pScreen, newRegion);

    return TRUE;
}
