
/*
   Copyright (c) 1998 by The XFree86 Project Inc.

   Written by Mark Vojkovich (mvojkovi@ucsd.edu)

*/
/*

Parts of this file derived from code carrying the following Copyright:
 
Copyright (c) 1986  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

*/
/* $XFree86: xc/programs/Xserver/Xext/anti.c,v 1.4 1999/06/06 08:48:37 dawes Exp $ */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "dixstruct.h"
#include "dixfontstr.h"
#include "resource.h"
#include "fontstruct.h"
#include "extnsionst.h"
#include "swapreq.h"
#include "closestr.h"
#include "XAntiproto.h"
#include "anti.h"

#ifdef EXTMODULE
#include "xf86_ansic.h"
#else
#include <string.h>
#endif

static Bool XAntiCreateGC(GCPtr);

static void XAntiValidateGC(GCPtr, unsigned long, DrawablePtr);
static void XAntiChangeGC(GCPtr, unsigned long);
static void XAntiCopyGC(GCPtr, unsigned long, GCPtr);
static void XAntiDestroyGC(GCPtr);
static void XAntiChangeClip(GCPtr, int, pointer, int);
static void XAntiDestroyClip(GCPtr);
static void XAntiCopyClip(GCPtr, GCPtr);


GCFuncs XAntiGCFuncs = {
    XAntiValidateGC, XAntiChangeGC, XAntiCopyGC, XAntiDestroyGC,
    XAntiChangeClip, XAntiDestroyClip, XAntiCopyClip
};


int ProcXAntiDispatch (ClientPtr);
int SProcXAntiDispatch (ClientPtr);

static Bool DrawAntiPolyText(PTclosurePtr);
static Bool DrawAntiImageText(ITclosurePtr);

int XAntiScreenIndex = -1;
int XAntiGCIndex = -1;
int XAntiReqCode;
 

#define XANTI_GC_FUNC_PROLOGUE(pGC)\
    AntiGCPtr pGCPriv = (AntiGCPtr) (pGC)->devPrivates[XAntiGCIndex].ptr;\
    (pGC)->funcs = pGCPriv->wrapFuncs;

#define XANTI_GC_FUNC_EPILOGUE(pGC)\
    pGCPriv->wrapFuncs = (pGC)->funcs;\
    (pGC)->funcs = &XAntiGCFuncs;

static DISPATCH_PROC(ProcAntiQueryExtension);
static DISPATCH_PROC(ProcAntiSetInterpolationPixels);
static DISPATCH_PROC(ProcAntiInterpolateColors);
static DISPATCH_PROC(ProcAntiPolyText);
static DISPATCH_PROC(ProcAntiImageText8);
static DISPATCH_PROC(ProcAntiImageText16);
static DISPATCH_PROC(SProcAntiQueryExtension);
static DISPATCH_PROC(SProcAntiSetInterpolationPixels);
static DISPATCH_PROC(SProcAntiInterpolateColors);
static DISPATCH_PROC(SProcAntiPolyText);
static DISPATCH_PROC(SProcAntiImageText8);
static DISPATCH_PROC(SProcAntiImageText16);


static int
AntiImageText(
    ClientPtr client,
    DrawablePtr pDraw,
    GCPtr pGC,
    int nChars,
    unsigned char *data,
    int xorg,
    int yorg,
    int reqType,
    XID did
);

static int
AntiPolyText(
    ClientPtr client,
    DrawablePtr pDraw,
    GCPtr pGC,
    unsigned char *pElt,
    unsigned char *endReq,
    int xorg,
    int yorg,
    int reqType,
    XID did
);




int
ProcXAntiDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case XAnti_QueryExtension:
        return ProcAntiQueryExtension(client);
    case XAnti_InterpolateColors:
        return ProcAntiInterpolateColors(client);
    case XAnti_SetInterpolationPixels:
        return ProcAntiSetInterpolationPixels(client);
    case XAnti_PolyText8:
    case XAnti_PolyText16:
        return ProcAntiPolyText(client);
    case XAnti_ImageText8:
        return ProcAntiImageText8(client);
    case XAnti_ImageText16:
        return ProcAntiImageText16(client);
    default:
        return BadRequest;
    }
}


int
SProcXAntiDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case XAnti_QueryExtension:
        return SProcAntiQueryExtension(client);
    case XAnti_InterpolateColors:
        return SProcAntiInterpolateColors(client);
    case XAnti_SetInterpolationPixels:
        return SProcAntiSetInterpolationPixels(client);
    case XAnti_PolyText8:
    case XAnti_PolyText16:
        return SProcAntiPolyText(client);
    case XAnti_ImageText8:
        return SProcAntiImageText8(client);
    case XAnti_ImageText16:
        return SProcAntiImageText16(client);
    default:
        return BadRequest;
    }
}

static void
XAntiResetProc (ExtensionEntry *extEntry)
{
    AntiScreenPtr pScreenPriv;
    ScreenPtr pScreen;
    int i;
 
    for (i = 0; i < screenInfo.numScreens; i++)
    {
        pScreen = screenInfo.screens[i];
        pScreenPriv = 
	   (AntiScreenPtr) pScreen->devPrivates[XAntiScreenIndex].ptr;

	pScreen->CreateGC = pScreenPriv->CreateGC;

	xfree((pointer) pScreenPriv);
	pScreen->devPrivates[XAntiScreenIndex].ptr = (pointer)NULL;
    }
}


static void 
XAntiUndoDamage(
    int i  /* screens which we already set up */
){
   AntiScreenPtr pScreenPriv;
   ScreenPtr pScreen;
   int j;
   for(j = 0; j < i; i++) {
	/* unwrap the screen */
	pScreen = screenInfo.screens[j];
        pScreenPriv = 
		(AntiScreenPtr) pScreen->devPrivates[XAntiScreenIndex].ptr;
   
	pScreen->CreateGC = pScreenPriv->CreateGC;

	/* free the private */
	xfree((pointer) pScreenPriv);
	pScreen->devPrivates[XAntiScreenIndex].ptr = (pointer)NULL;
   }
   ErrorF("Antialiased font extension initialization failed!\n");
}


static void
XAntiCopyGCPrivate(GCPtr old, GCPtr new)
{
    AntiGCPtr gcPrivNew = XANTI_GET_GC_PRIVATE(new);
    AntiGCPtr gcPrivOld = XANTI_GET_GC_PRIVATE(old);
    if(gcPrivOld->NumPixels) {
	int size = gcPrivOld->NumPixels * sizeof(unsigned long);
	gcPrivNew->Pixels = xalloc(size);
	if(gcPrivNew->Pixels) {
	    gcPrivNew->NumPixels = gcPrivOld->NumPixels;
	    memcpy(gcPrivNew->Pixels, gcPrivOld->Pixels, size);
	}
    }
}

void
XAntiExtensionInit(void)
{
    ScreenPtr pScreen;
    AntiScreenPtr pScreenPriv;
    ExtensionEntry *extEntry;
    int i;

    if((XAntiScreenIndex = AllocateScreenPrivateIndex()) < 0) return;
    if((XAntiGCIndex = AllocateGCPrivateIndex()) < 0) return;

    for(i = 0; i < screenInfo.numScreens; i++) {
	pScreen = screenInfo.screens[i];

    	if (!AllocateGCPrivate(pScreen, XAntiGCIndex, sizeof(AntiGCRec))) {
	   XAntiUndoDamage(i);
	   return;
	}

    	if (!(pScreenPriv = (AntiScreenPtr)xalloc(sizeof(AntiScreenRec)))) {
	   XAntiUndoDamage(i);
	   return;
	}

	pScreen->devPrivates[XAntiScreenIndex].ptr = (pointer)pScreenPriv;

	pScreenPriv->CreateGC = pScreen->CreateGC;
	pScreen->CreateGC = XAntiCreateGC;

        pScreenPriv->PolyTextFunc = DrawAntiPolyText;
        pScreenPriv->ImageTextFunc = DrawAntiImageText;
    }

    extEntry = AddExtension(XAntiName, 0 /*events*/, 0 /*errors*/,
			ProcXAntiDispatch, SProcXAntiDispatch,
			XAntiResetProc, StandardMinorOpcode);

    if(!extEntry)
	XAntiUndoDamage(screenInfo.numScreens);
    else 
	XAntiReqCode = extEntry->base;
}


/* Screen functions */


static Bool
XAntiCreateGC(GCPtr pGC)
{
    ScreenPtr	pScreen = pGC->pScreen;
    Bool	ret;
    AntiGCPtr	pGCPriv = (AntiGCPtr)(pGC->devPrivates[XAntiGCIndex].ptr);
    AntiScreenPtr pScreenPriv = 
        (AntiScreenPtr) pScreen->devPrivates[XAntiScreenIndex].ptr;
    
    pScreen->CreateGC = pScreenPriv->CreateGC;

    if((ret = (*pScreen->CreateGC)(pGC))) {
	pGCPriv->NumPixels = 0;
	pGCPriv->Pixels = NULL;
	pGCPriv->wrapFuncs = pGC->funcs;
	pGCPriv->clientData = FALSE;
	pGC->funcs = &XAntiGCFuncs;
    }

    pScreen->CreateGC = XAntiCreateGC;
  
    return ret;
}

/* GC Funcs */

static void
XAntiValidateGC(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw 
){
    XANTI_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ValidateGC) (pGC, changes, pDraw);
    XANTI_GC_FUNC_EPILOGUE (pGC);
}

/* It's a shame that we have to wrap all the GCFuncs just to free
   the extra colors.  Alternatively you could have a static array
   in the GC private, but it would have to be large enough to 
   accomodate any number of levels that would be used with it and
   you'd have all the GCs unnecessarily large, particularly in the
   case where someone wanted to be using 256 interpolation levels. */

static void
XAntiDestroyGC(GCPtr pGC)
{
    XANTI_GC_FUNC_PROLOGUE (pGC);

    if(pGCPriv->Pixels)
	xfree(pGCPriv->Pixels);

    (*pGC->funcs->DestroyGC)(pGC);
    XANTI_GC_FUNC_EPILOGUE (pGC);
}

static void
XAntiChangeGC (
    GCPtr	    pGC,
    unsigned long   mask
){
    XANTI_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeGC) (pGC, mask);
    XANTI_GC_FUNC_EPILOGUE (pGC);
}

static void
XAntiCopyGC (
    GCPtr	    pGCSrc, 
    unsigned long   mask,
    GCPtr	    pGCDst
){
    XANTI_GC_FUNC_PROLOGUE (pGCDst);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    XANTI_GC_FUNC_EPILOGUE (pGCDst);
}
static void
XAntiChangeClip (
    GCPtr   pGC,
    int		type,
    pointer	pvalue,
    int		nrects 
){
    XANTI_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    XANTI_GC_FUNC_EPILOGUE (pGC);
}

static void
XAntiCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    XANTI_GC_FUNC_PROLOGUE (pgcDst);
    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);
    XANTI_GC_FUNC_EPILOGUE (pgcDst);
}

static void
XAntiDestroyClip(GCPtr pGC)
{
    XANTI_GC_FUNC_PROLOGUE (pGC);
    (* pGC->funcs->DestroyClip)(pGC);
    XANTI_GC_FUNC_EPILOGUE (pGC);
}


/********/




static int
ProcAntiQueryExtension(ClientPtr client)
{
    xAntiQueryExtensionReply rep;
    REQUEST(xAntiQueryExtensionReq);
    REQUEST_SIZE_MATCH(xAntiQueryExtensionReq);

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.version = XAntiVersion;
    rep.revision = XAntiRevision;

    if(client->swapped) {
	register char n;

	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swaps(&rep.version, n);
	swaps(&rep.revision, n);
    }  
	
    WriteToClient(client, sz_xAntiQueryExtensionReply, (char*) &rep);
 
    return(client->noClientException);
}


static int
ProcAntiPolyText(ClientPtr client)
{
    int	err;
    REQUEST(xAntiPolyTextReq);
    DrawablePtr pDraw;
    GCPtr pGC;

    REQUEST_AT_LEAST_SIZE(xAntiPolyTextReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);

    err = AntiPolyText(client,
		   pDraw,
		   pGC,
		   (unsigned char *)&stuff[1],
		   ((unsigned char *) stuff) + (client->req_len << 2),
		   stuff->x,
		   stuff->y,
		   stuff->reqType,
		   stuff->drawable);

    if (err == Success)
	return(client->noClientException);
    else
	return err;
}


static int
ProcAntiImageText8(ClientPtr client)
{
    int	err;
    DrawablePtr pDraw;
    GCPtr pGC;

    REQUEST(xAntiImageTextReq);

    REQUEST_FIXED_SIZE(xAntiImageTextReq, stuff->nChars);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);

    err = AntiImageText(client,
		    pDraw,
		    pGC,
		    stuff->nChars,
		    (unsigned char *)&stuff[1],
		    stuff->x,
		    stuff->y,
		    stuff->reqType,
		    stuff->drawable);

    if (err == Success)
	return(client->noClientException);
    else
	return err;
}

static int
ProcAntiImageText16(ClientPtr client)
{
    int	err;
    DrawablePtr pDraw;
    GCPtr pGC;

    REQUEST(xAntiImageTextReq);

    REQUEST_FIXED_SIZE(xAntiImageTextReq, stuff->nChars << 1);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);

    err = AntiImageText(client,
		    pDraw,
		    pGC,
		    stuff->nChars,
		    (unsigned char *)&stuff[1],
		    stuff->x,
		    stuff->y,
		    stuff->reqType,
		    stuff->drawable);

    if (err == Success)
	return(client->noClientException);
    else
	return err;
}


static int
ProcAntiSetInterpolationPixels(ClientPtr client)
{
    CARD32 *newPix;
    AntiGCPtr gcPriv; 
    GCPtr pGC;
    int i;

    REQUEST(xAntiSetInterpolationPixelsReq);

    REQUEST_FIXED_SIZE(xAntiSetInterpolationPixelsReq, stuff->number << 2);

    if((stuff->number + 1) & stuff->number)
	return BadValue;

    if((stuff->number < 0) || (stuff->number > 255))
	return BadValue;

    SECURITY_VERIFY_GC(pGC, stuff->gc, client, SecurityReadAccess);  
    gcPriv = XANTI_GET_GC_PRIVATE(pGC);

    if(stuff->number != gcPriv->NumPixels) {
	unsigned long *pixels = NULL;

	if(stuff->number) {
	   pixels = (unsigned long*)xalloc(
			stuff->number * sizeof(unsigned long));
	   if(!pixels)
		return BadAlloc;
	}

	if(gcPriv->Pixels) 
	   xfree(gcPriv->Pixels);

	gcPriv->Pixels = pixels;
	gcPriv->NumPixels = stuff->number;
    }

    newPix = (CARD32*)&stuff[1];

    for(i = 0; i < stuff->number; i++)
	gcPriv->Pixels[i] = newPix[i];

    gcPriv->clientData = TRUE;
    return(client->noClientException);
}


static void 
FindInterpolationColors(
   ColormapPtr pmap,
   int number,
   xColorItem *colors,
   unsigned long fg,
   unsigned long bg
){
    double red, green, blue, dr, dg, db;
    xColorItem color;
    Pixel pix[2];
    xrgb rgb[2];
    int i;

    pix[0] = fg;
    pix[1] = bg;
    QueryColors(pmap, 2, pix, rgb);

    red   = (double)rgb[0].red;
    green = (double)rgb[0].green;
    blue  = (double)rgb[0].blue;

    dr = ((double)rgb[1].red - red)/(double)(number + 1);
    dg = ((double)rgb[1].green - green)/(double)(number + 1);
    db = ((double)rgb[1].blue - blue)/(double)(number + 1);

    for(i = 0; i < number; i++) {
	red += dr;
	green += dg;
	blue += db;

	colors[i].red   = color.red   = (unsigned short)(red + 0.5);
	colors[i].green = color.green = (unsigned short)(green + 0.5);
	colors[i].blue  = color.blue  = (unsigned short)(blue + 0.5);
        colors[i].flags = color.flags = DoRed | DoGreen | DoBlue;
	
        FakeAllocColor(pmap, &color); /* just to get the pixel */

	colors[i].pixel = color.pixel; 

	FakeFreeColor(pmap, color.pixel);
    }
}

static int
ProcAntiInterpolateColors(ClientPtr client)
{
    xAntiInterpolateColorsReply rep;
    xColorItem *colors;
    ColormapPtr pmap;
    GCPtr pGC;

    REQUEST(xAntiInterpolateColorsReq);
    REQUEST_SIZE_MATCH(xAntiInterpolateColorsReq);

    if((stuff->number <= 0) || (stuff->number > 254)) return BadValue;

    SECURITY_VERIFY_GC(pGC, stuff->gc, client, SecurityReadAccess);  

    pmap = (ColormapPtr )SecurityLookupIDByType(client, stuff->colormap,
				 RT_COLORMAP, SecurityReadAccess);

    if(!pmap) return BadColor;

    colors = (xColorItem*)ALLOCATE_LOCAL(stuff->number * sizeof(xColorItem));

    FindInterpolationColors(pmap, stuff->number, colors, 
				pGC->fgPixel, pGC->bgPixel);

    rep.type = X_Reply;
    rep.length = (stuff->number * sizeof(xColorItem)) >> 2;
    rep.sequenceNumber = client->sequence;


    if(client->swapped) {
	register char n;

	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
    }  
	
    WriteToClient(client, sz_xAntiInterpolateColorsReply, (char*) &rep);

    if(client->swapped) {
	register char n;
	int i;

	for(i = 0; i < stuff->number; i++) {
	    swapl(&colors[i].pixel, n);
	    swaps(&colors[i].red, n);
	    swaps(&colors[i].green, n);
	    swaps(&colors[i].blue, n);
	}
    }

    WriteToClient(client, stuff->number*sizeof(xColorItem), (char*)colors);

    DEALLOCATE_LOCAL(colors);

    return(client->noClientException);
}


/*******  Image text  **********/


#define TextEltHeader 2
#define FontShiftSize 5
static XID clearGC[] = { CT_NONE };
#define clearGCmask (GCClipMask)


static Bool
DrawAntiImageText(ITclosurePtr c) {
    unsigned long n;
    CharInfoPtr charPtr[255];  /* encoding only has 1 byte for count */
    FontEncoding fontEncoding = Linear8Bit;
    AntiGCPtr gcPriv = XANTI_GET_GC_PRIVATE(c->pGC);
    GetAntiGlyphsFuncPtr *getGlyphs = XANTI_GET_FONT_PRIVATE(c->pGC->font);

    if(!getGlyphs) return FALSE;

    if(!gcPriv->NumPixels) {
	if(gcPriv->clientData) return FALSE;
	
	/* otherwise, we generate some reasonable colors if appropriate */
	return FALSE;
    }

    if(c->itemSize == 2)
      fontEncoding = (FONTLASTROW(c->pGC->font) == 0) ? Linear16Bit : TwoD16Bit;

    if(Success != (*getGlyphs)(c->pGC->font, (unsigned long)(c->nChars), 
		(unsigned char *)(c->data), fontEncoding, &n, charPtr, 
		gcPriv->NumPixels + 1))
	return FALSE;

    if(n) {
        int i, j;
        XID oldFG = c->pGC->fgPixel;
        int Pitch[255];
        CharInfoRec *charData;

        (*c->pGC->ops->ImageGlyphBlt)(c->pDraw, c->pGC, c->xorg, c->yorg, 
                                n, charPtr, FONTGLYPHS(c->pGC->font));

        if(!(charData = (CharInfoPtr)ALLOCATE_LOCAL(n * sizeof(CharInfoRec))))
            return FALSE;

        /* we need a modifiable copy of the CharInfoRecs */
        for(i = 0; i < n; i++) {
            memcpy(&charData[i], charPtr[i], sizeof(CharInfoRec)); 
            Pitch[i] = GLYPHWIDTHBYTESPADDED(charPtr[i]) * 
                                        GLYPHHEIGHTPIXELS(charPtr[i]);
        }
        
        for(i = 0; i < gcPriv->NumPixels; i++) {
            for(j = 0; j < n; j++) 
                charData[j].bits += Pitch[j];
            ChangeGC(c->pGC, GCForeground, (XID*)&(gcPriv->Pixels[i]));
            ValidateGC(c->pDraw, c->pGC);
	    (*c->pGC->ops->PolyGlyphBlt)(c->pDraw, c->pGC, c->xorg, c->yorg,
	 			n, &charData, FONTGLYPHS(c->pGC->font));
        }

        ChangeGC(c->pGC, GCForeground, &oldFG);
        ValidateGC(c->pDraw, c->pGC);
        DEALLOCATE_LOCAL(charData);   
    }
 
    return TRUE;
}


static int
doAntiImageText(
    ClientPtr   client,
    ITclosurePtr c
){
    int err = Success, lgerr;	/* err is in X error, not font error, space */
    FontPathElementPtr fpe;

    if (client->clientGone) 
    {
	fpe = c->pGC->font->fpe;
	(*fpe_functions[fpe->type].client_died) ((pointer) client, fpe);
	err = Success;
	goto bail;
    }

    /* Make sure our drawable hasn't disappeared while we slept. */
    if (c->slept &&
	c->pDraw &&
	c->pDraw != (DrawablePtr)SecurityLookupIDByClass(client, c->did,
					RC_DRAWABLE, SecurityWriteAccess))
    {
	/* Our drawable has disappeared.  Treat like client died... ask
	   the FPE code to clean up after client. */
	fpe = c->pGC->font->fpe;
	(*fpe_functions[fpe->type].client_died) ((pointer) client, fpe);
	err = Success;
	goto bail;
    }

    lgerr = LoadGlyphs(client, c->pGC->font, c->nChars, c->itemSize, c->data);
    if (lgerr == Suspended)
    {
        if (!c->slept) {
	    GCPtr pGC, oldGC;
	    unsigned char *data;
	    ITclosurePtr new_closure;

	    /* We're putting the client to sleep.  We need to
	       save some state.  Similar problem to that handled
	       in doAntiPolyText, but much simpler because the
	       request structure is much simpler. */

	    new_closure = (ITclosurePtr) xalloc(sizeof(ITclosureRec));
	    if (!new_closure)
	    {
		err = BadAlloc;
		goto bail;
	    }
	    *new_closure = *c;
	    c = new_closure;

	    data = (unsigned char *)xalloc(c->nChars * c->itemSize);
	    if (!data)
	    {
		xfree(c);
		err = BadAlloc;
		goto bail;
	    }
	    memmove(data, c->data, c->nChars * c->itemSize);
	    c->data = data;

	    pGC = GetScratchGC(c->pGC->depth, c->pGC->pScreen);
	    if (!pGC)
	    {
		xfree(c->data);
		xfree(c);
		err = BadAlloc;
		goto bail;
	    }
	    if ((err = CopyGC(c->pGC, pGC, GCFunction | GCPlaneMask |
			      GCForeground | GCBackground | GCFillStyle |
			      GCTile | GCStipple | GCTileStipXOrigin |
			      GCTileStipYOrigin | GCFont |
			      GCSubwindowMode | GCClipXOrigin |
			      GCClipYOrigin | GCClipMask)) != Success)
	    {
		FreeScratchGC(pGC);
		xfree(c->data);
		xfree(c);
		err = BadAlloc;
		goto bail;
	    }

	    oldGC = c->pGC;
	    c->pGC = pGC;
	    ValidateGC(c->pDraw, c->pGC);

            /* We need to make sure that our scratch GC has 
					the correct private data */
	    XAntiCopyGCPrivate(oldGC, c->pGC);


	    c->slept = TRUE;
            ClientSleep(client, (ClientSleepProcPtr)doAntiImageText, 
							(pointer) c);
        }
        return TRUE;
    }
    else if (lgerr != Successful)
    {
        err = FontToXError(lgerr);
        goto bail;
    }
    if (c->pDraw) {
	AntiScreenPtr pScreenPriv = XANTI_GET_SCREEN_PRIVATE(c->pDraw);

	if(!(*pScreenPriv->ImageTextFunc)(c)) {
	     (* c->imageText)(c->pDraw, c->pGC, c->xorg, c->yorg,
	    			c->nChars, c->data);
	}
    }

bail:

    if (err != Success && c->client != serverClient) {
	SendErrorToClient(c->client, c->reqType, 0, 0, err);
    }
    if (c->slept)
    {
	AntiGCPtr gcPriv = XANTI_GET_GC_PRIVATE(c->pGC);
	ClientWakeup(c->client);
	ChangeGC(c->pGC, clearGCmask, clearGC);

	/* Unreference the font from the scratch GC */
	CloseFont(c->pGC->font, (Font)0);
	c->pGC->font = NullFont;

        /* We need to make sure we free any private data */
        if(gcPriv->Pixels) xfree(gcPriv->Pixels);

	FreeScratchGC(c->pGC);
	xfree(c->data);
	xfree(c);
    }
    return TRUE;
}


static int
AntiImageText(
    ClientPtr client,
    DrawablePtr pDraw,
    GCPtr pGC,
    int nChars,
    unsigned char *data,
    int xorg,
    int yorg,
    int reqType,
    XID did
){
    ITclosureRec local_closure;

    local_closure.client = client;
    local_closure.pDraw = pDraw;
    local_closure.pGC = pGC;
    local_closure.nChars = nChars;
    local_closure.data = data;
    local_closure.xorg = xorg;
    local_closure.yorg = yorg;
    if ((local_closure.reqType = reqType) == XAnti_ImageText8) {
	local_closure.imageText = (ImageTextPtr) pGC->ops->ImageText8;
	local_closure.itemSize = 1;
    } else {
	local_closure.imageText = (ImageTextPtr) pGC->ops->ImageText16;
	local_closure.itemSize = 2;
    }
    local_closure.did = did;
    local_closure.slept = FALSE;

    (void) doAntiImageText(client, &local_closure);
    return Success;
}


/********  Poly Text  **********/


static Bool
DrawAntiPolyText(PTclosurePtr c)
{
    unsigned long n;
    int w = 0;
    CharInfoPtr charPtr[255];
    FontEncoding fontEncoding = Linear8Bit;
    AntiGCPtr gcPriv = XANTI_GET_GC_PRIVATE(c->pGC);
    GetAntiGlyphsFuncPtr *getGlyphs = XANTI_GET_FONT_PRIVATE(c->pGC->font);


    if(!gcPriv->NumPixels || !getGlyphs) return FALSE;

    if(c->itemSize == 2) {
      fontEncoding = (FONTLASTROW(c->pGC->font) == 0) ? Linear16Bit : TwoD16Bit;
    }
    

    if(Success != (*getGlyphs)(c->pGC->font, (unsigned long)(*c->pElt), 
		(unsigned char *)(c->pElt + TextEltHeader),
              	fontEncoding, &n, charPtr, gcPriv->NumPixels + 1))
	return FALSE;

    if(n) {
	int i, j;
	XID oldFG = c->pGC->fgPixel;
	int Pitch[255];
	CharInfoRec *charData;

        (*c->pGC->ops->PolyGlyphBlt)(c->pDraw, c->pGC, c->xorg, c->yorg, 
				n, charPtr, FONTGLYPHS(c->pGC->font));

	if(!(charData = (CharInfoPtr)ALLOCATE_LOCAL(n * sizeof(CharInfoRec))))
	    return FALSE;

	/* we need a modifiable copy of the CharInfoRecs */
	for(i = 0; i < n; i++) {
	    memcpy(&charData[i], charPtr[i], sizeof(CharInfoRec)); 
	    Pitch[i] = GLYPHWIDTHBYTESPADDED(charPtr[i]) * 
					GLYPHHEIGHTPIXELS(charPtr[i]);
	    w += charPtr[i]->metrics.characterWidth;
	}
	
	for(i = 0; i < gcPriv->NumPixels; i++) {
	    for(j = 0; j < n; j++) 
		charData[j].bits += Pitch[j];
	    ChangeGC(c->pGC, GCForeground, (XID*)&(gcPriv->Pixels[i]));
	    ValidateGC(c->pDraw, c->pGC);
	    (*c->pGC->ops->PolyGlyphBlt)(c->pDraw, c->pGC, c->xorg, c->yorg,
	 			n, &charData, FONTGLYPHS(c->pGC->font));
	}

	ChangeGC(c->pGC, GCForeground, &oldFG);
	ValidateGC(c->pDraw, c->pGC);
	DEALLOCATE_LOCAL(charData);   
    }

    c->xorg += w;

    return TRUE;
}


static int
doAntiPolyText(
    ClientPtr   client,
    PTclosurePtr c
){
    register FontPtr pFont = c->pGC->font, oldpFont;
    Font	fid, oldfid;
    int err = Success, lgerr;	/* err is in X error, not font error, space */
    enum { NEVER_SLEPT, START_SLEEP, SLEEPING } client_state;
    FontPathElementPtr fpe;
    GCPtr origGC;

    if (client->clientGone)
    {
	fpe = c->pGC->font->fpe;
	(*fpe_functions[fpe->type].client_died) ((pointer) client, fpe);

	if (c->slept)
	{
	    /* Client has died, but we cannot bail out right now.  We
	       need to clean up after the work we did when going to
	       sleep.  Setting the drawable pointer to 0 makes this
	       happen without any attempts to render or perform other
	       unnecessary activities.  */
	    c->pDraw = (DrawablePtr)0;
	}
	else
	{
	    err = Success;
	    goto bail;
	}
    }

    /* Make sure our drawable hasn't disappeared while we slept. */
    if (c->slept &&
	c->pDraw &&
	c->pDraw != (DrawablePtr)SecurityLookupIDByClass(client, c->did,
					RC_DRAWABLE, SecurityWriteAccess))
    {
	/* Our drawable has disappeared.  Treat like client died... ask
	   the FPE code to clean up after client and avoid further
	   rendering while we clean up after ourself.  */
	fpe = c->pGC->font->fpe;
	(*fpe_functions[fpe->type].client_died) ((pointer) client, fpe);
	c->pDraw = (DrawablePtr)0;
    }
    client_state = c->slept ? SLEEPING : NEVER_SLEPT;

    while (c->endReq - c->pElt > TextEltHeader)
    {
	if (*c->pElt == FontChange)
        {
	    if (c->endReq - c->pElt < FontShiftSize)
	    {
		 err = BadLength;
		 goto bail;
	    }

	    oldpFont = pFont;
	    oldfid = fid;

	    fid =  ((Font)*(c->pElt+4))		/* big-endian */
		 | ((Font)*(c->pElt+3)) << 8
		 | ((Font)*(c->pElt+2)) << 16
		 | ((Font)*(c->pElt+1)) << 24;
	    pFont = (FontPtr)SecurityLookupIDByType(client, fid, RT_FONT,
						    SecurityReadAccess);
	    if (!pFont)
	    {
		client->errorValue = fid;
		err = BadFont;
		/* restore pFont and fid for step 4 (described below) */
		pFont = oldpFont;
		fid = oldfid;

		/* If we're in START_SLEEP mode, the following step
		   shortens the request...  in the unlikely event that
		   the fid somehow becomes valid before we come through
		   again to actually execute the polytext, which would
		   then mess up our refcounting scheme badly.  */
		c->err = err;
		c->endReq = c->pElt;

		goto bail;
	    }

	    /* Step 3 (described below) on our new font */
	    if (client_state == START_SLEEP)
		pFont->refcnt++;
	    else
	    {
		if (pFont != c->pGC->font && c->pDraw)
		{
		    ChangeGC( c->pGC, GCFont, &fid);
		    ValidateGC(c->pDraw, c->pGC);
		    if (c->reqType == XAnti_PolyText8)
			c->polyText = (PolyTextPtr) c->pGC->ops->PolyText8;
		    else
			c->polyText = (PolyTextPtr) c->pGC->ops->PolyText16;
		}

		/* Undo the refcnt++ we performed when going to sleep */
		if (client_state == SLEEPING)
		    (void)CloseFont(c->pGC->font, (Font)0);
	    }
	    c->pElt += FontShiftSize;
	}
	else	/* print a string */
	{
	    unsigned char *pNextElt;
	    pNextElt = c->pElt + TextEltHeader + (*c->pElt)*c->itemSize;
	    if ( pNextElt > c->endReq)
	    {
		err = BadLength;
		goto bail;
	    }
	    if (client_state == START_SLEEP)
	    {
		c->pElt = pNextElt;
		continue;
	    }
	    if (c->pDraw)
	    {
		lgerr = LoadGlyphs(client, c->pGC->font, *c->pElt, c->itemSize,
				   c->pElt + TextEltHeader);
	    }
	    else lgerr = Successful;

	    if (lgerr == Suspended)
	    {
		if (!c->slept) {
		    int len;
		    GCPtr pGC;
		    PTclosurePtr new_closure;

    /*  We're putting the client to sleep.  We need to do a few things
	to ensure successful and atomic-appearing execution of the
	remainder of the request.  First, copy the remainder of the
	request into a safe malloc'd area.  Second, create a scratch GC
	to use for the remainder of the request.  Third, mark all fonts
	referenced in the remainder of the request to prevent their
	deallocation.  Fourth, make the original GC look like the
	request has completed...  set its font to the final font value
	from this request.  These GC manipulations are for the unlikely
	(but possible) event that some other client is using the GC.
	Steps 3 and 4 are performed by running this procedure through
	the remainder of the request in a special no-render mode
	indicated by client_state = START_SLEEP.  */

		    /* Step 1 */
		    /* Allocate a malloc'd closure structure to replace
		       the local one we were passed */
		    new_closure = (PTclosurePtr) xalloc(sizeof(PTclosureRec));
		    if (!new_closure)
		    {
			err = BadAlloc;
			goto bail;
		    }
		    *new_closure = *c;
		    c = new_closure;

		    len = c->endReq - c->pElt;
		    c->data = (unsigned char *)xalloc(len);
		    if (!c->data)
		    {
			xfree(c);
			err = BadAlloc;
			goto bail;
		    }
		    memmove(c->data, c->pElt, len);
		    c->pElt = c->data;
		    c->endReq = c->pElt + len;

		    /* Step 2 */

		    pGC = GetScratchGC(c->pGC->depth, c->pGC->pScreen);
		    if (!pGC)
		    {
			xfree(c->data);
			xfree(c);
			err = BadAlloc;
			goto bail;
		    }
		    if ((err = CopyGC(c->pGC, pGC, GCFunction |
				      GCPlaneMask | GCForeground |
				      GCBackground | GCFillStyle |
				      GCTile | GCStipple |
				      GCTileStipXOrigin |
				      GCTileStipYOrigin | GCFont |
				      GCSubwindowMode | GCClipXOrigin |
				      GCClipYOrigin | GCClipMask)) !=
				      Success)
		    {
			FreeScratchGC(pGC);
			xfree(c->data);
			xfree(c);
			err = BadAlloc;
			goto bail;
		    }
		    origGC = c->pGC;
		    c->pGC = pGC;
		    ValidateGC(c->pDraw, c->pGC);
		    
		    /* We need to make sure that our scratch GC has
			the correct private data */
		    XAntiCopyGCPrivate(origGC, c->pGC);

		    c->slept = TRUE;
		    ClientSleep(client,
		    	     (ClientSleepProcPtr)doAntiPolyText,
			     (pointer) c);

		    /* Set up to perform steps 3 and 4 */
		    client_state = START_SLEEP;
		    continue;	/* on to steps 3 and 4 */
		}
		return TRUE;
	    }
	    else if (lgerr != Successful)
	    {
		err = FontToXError(lgerr);
		goto bail;
	    }
	    if (c->pDraw) {
		AntiScreenPtr pScreenPriv = XANTI_GET_SCREEN_PRIVATE(c->pDraw);
		XID newVals[2];
		XID oldVals[2];
		int mask = 0;

		if(c->pGC->alu != GXcopy) {
		   newVals[0] = GXcopy;
		   oldVals[0] = c->pGC->alu;
		   mask |= GCFunction;
		} 

		if(c->pGC->fillStyle != FillSolid) {
		   newVals[1] = FillSolid;
		   oldVals[1] = c->pGC->fillStyle;
		   mask |= GCFillStyle;
		   if(!(mask & GCFunction)) {
			newVals[0] = newVals[1];
			oldVals[0] = oldVals[1];
		   }
		} 
		
		if(mask) {
		    ChangeGC(c->pGC, mask, newVals);
		    ValidateGC(c->pDraw, c->pGC);
		}

		c->xorg += *((INT8 *)(c->pElt + 1));	/* must be signed */
		if(!(*pScreenPriv->PolyTextFunc)(c)) {
		   c->xorg = (* c->polyText)(c->pDraw, c->pGC, 
					c->xorg, c->yorg,
		    			*c->pElt, c->pElt + TextEltHeader);
		}

		if(mask) {
		    ChangeGC(c->pGC, mask, oldVals);
		    ValidateGC(c->pDraw, c->pGC);
		}

	    }
	    c->pElt = pNextElt;
	}
    }

bail:

    if (client_state == START_SLEEP)
    {
	/* Step 4 */
	if (pFont != origGC->font)
	{
	    ChangeGC(origGC, GCFont, &fid);
	    ValidateGC(c->pDraw, origGC);
	}

	/* restore pElt pointer for execution of remainder of the request */
	c->pElt = c->data;
	return TRUE;
    }

    if (c->err != Success) err = c->err;
    if (err != Success && c->client != serverClient) {
	SendErrorToClient(c->client, c->reqType, 0, 0, err);
    }
    if (c->slept)
    {
	AntiGCPtr gcPriv = XANTI_GET_GC_PRIVATE(c->pGC);
	ClientWakeup(c->client);
	ChangeGC(c->pGC, clearGCmask, clearGC);

	/* Unreference the font from the scratch GC */
	CloseFont(c->pGC->font, (Font)0);
	c->pGC->font = NullFont;

	/* We need to make sure we free any private data */
	if(gcPriv->Pixels) xfree(gcPriv->Pixels);

	FreeScratchGC(c->pGC);
	xfree(c->data);
	xfree(c);
    }
    return TRUE;
}

static int
AntiPolyText(
    ClientPtr client,
    DrawablePtr pDraw,
    GCPtr pGC,
    unsigned char *pElt,
    unsigned char *endReq,
    int xorg,
    int yorg,
    int reqType,
    XID did
){
    PTclosureRec local_closure;

    local_closure.pElt = pElt;
    local_closure.endReq = endReq;
    local_closure.client = client;
    local_closure.pDraw = pDraw;
    local_closure.xorg = xorg;
    local_closure.yorg = yorg;
    if ((local_closure.reqType = reqType) == XAnti_PolyText8) {
	local_closure.polyText = (PolyTextPtr) pGC->ops->PolyText8;
	local_closure.itemSize = 1;
    } else {
	local_closure.polyText =  (PolyTextPtr) pGC->ops->PolyText16;
	local_closure.itemSize = 2;
    }
    local_closure.pGC = pGC;
    local_closure.did = did;
    local_closure.err = Success;
    local_closure.slept = FALSE;

    (void) doAntiPolyText(client, &local_closure);
    return Success;
}


#undef TextEltHeader
#undef FontShiftSize


/**** swaps ****/


static int 
SProcAntiQueryExtension(ClientPtr client)
{
  register char n;
  REQUEST(xAntiQueryExtensionReq);
  swaps(&stuff->length, n);
  return ProcAntiQueryExtension(client);
}

static int 
SProcAntiSetInterpolationPixels(ClientPtr client)
{
  register char n;
  REQUEST(xAntiSetInterpolationPixelsReq);
  swaps(&stuff->length, n);
  swapl(&stuff->gc, n);
  swapl(&stuff->number, n);
  if(stuff->number)
     SwapLongs((CARD32*)(stuff + 1), stuff->number);
  return ProcAntiSetInterpolationPixels(client);
}

static int 
SProcAntiInterpolateColors(ClientPtr client)
{
  register char n;
  REQUEST(xAntiInterpolateColorsReq);
  swaps(&stuff->length, n);
  swapl(&stuff->gc, n);
  swapl(&stuff->colormap, n);
  swapl(&stuff->number, n);
  return ProcAntiInterpolateColors(client);
}

static int 
SProcAntiPolyText(ClientPtr client)
{
  register char n;
  REQUEST(xAntiPolyTextReq);
  swaps(&stuff->length, n);
  swapl(&stuff->drawable, n);
  swapl(&stuff->gc, n);
  swaps(&stuff->x, n);
  swaps(&stuff->y, n);
  return ProcAntiPolyText(client);
}

static int 
SProcAntiImageText8(ClientPtr client)
{
  register char n;
  REQUEST(xAntiImageText8Req);
  swaps(&stuff->length, n);
  swapl(&stuff->drawable, n);
  swapl(&stuff->gc, n);
  swaps(&stuff->x, n);
  swaps(&stuff->y, n);
  return ProcAntiImageText8(client);
}

static int 
SProcAntiImageText16(ClientPtr client)
{
  register char n;
  REQUEST(xAntiImageText16Req);
  swaps(&stuff->length, n);
  swapl(&stuff->drawable, n);
  swapl(&stuff->gc, n);
  swaps(&stuff->x, n);
  swaps(&stuff->y, n);
  return ProcAntiImageText16(client);
}

