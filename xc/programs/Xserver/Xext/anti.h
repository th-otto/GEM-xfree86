/* $XFree86: xc/programs/Xserver/Xext/anti.h,v 1.2 1999/06/06 08:48:37 dawes Exp $ */

#ifndef _ANTI_H
#define _ANTI_H

typedef struct {
    CreateGCProcPtr	CreateGC;
    Bool 		(*PolyTextFunc)(PTclosurePtr);
    Bool 		(*ImageTextFunc)(ITclosurePtr);
} AntiScreenRec, *AntiScreenPtr;

typedef struct {
    int 		NumPixels;
    unsigned long 	*Pixels;
    GCFuncs		*wrapFuncs;
    Bool		clientData;
    unsigned long	fg;
    unsigned long	bg;
} AntiGCRec, *AntiGCPtr;

extern int XAntiGCIndex;
extern int XAntiScreenIndex;

void XAntiExtensionInit(void);

typedef int (*GetAntiGlyphsFuncPtr)(FontPtr, unsigned long, unsigned char*, 
		FontEncoding, unsigned long*, CharInfoPtr*, int);

#define XANTI_GET_GC_PRIVATE(gc) \
	(AntiGCPtr)((gc)->devPrivates[XAntiGCIndex].ptr)
#define XANTI_GET_SCREEN_PRIVATE(draw) \
	(AntiScreenPtr)((draw)->pScreen->devPrivates[XAntiScreenIndex].ptr)
#if 1
#define XANTI_GET_FONT_PRIVATE(font) 0 /* for now */
#else
#define XANTI_GET_FONT_PRIVATE(font) \
	(antiAliasingPrivateIndex == -1) ? 0 : \
	(GetAntiGlyphsFuncPtr)(font->devPrivates[antiAliasingPrivateIndex])
#endif

#endif /* _ANTI_H */

