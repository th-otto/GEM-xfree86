/* $XFree86: xc/programs/Xserver/Xext/dgaproc.h,v 1.19 1999/10/13 22:32:47 dawes Exp $ */

#ifndef __DGAPROC_H
#define __DGAPROC_H


#define DGA_CONCURRENT_ACCESS	0x00000001
#define DGA_FILL_RECT		0x00000002
#define DGA_BLIT_RECT		0x00000004
#define DGA_BLIT_RECT_TRANS	0x00000008
#define DGA_PIXMAP_AVAILABLE	0x00000010

#define DGA_INTERLACED		0x00010000
#define DGA_DOUBLESCAN		0x00020000

#define DGA_FLIP_IMMEDIATE	0x00000001
#define DGA_FLIP_RETRACE	0x00000002

#define DGA_COMPLETED		0x00000000
#define DGA_PENDING		0x00000001

#define DGA_NEED_ROOT		0x00000001

typedef struct {
   int num;		/* A unique identifier for the mode (num > 0) */
   char *name;		/* name of mode given in the XF86Config */
   int VSync_num;
   int VSync_den;
   int flags;		/* DGA_CONCURRENT_ACCESS, etc... */
   int imageWidth;	/* linear accessible portion (pixels) */
   int imageHeight;
   int pixmapWidth;	/* Xlib accessible portion (pixels) */
   int pixmapHeight;	/* both fields ignored if no concurrent access */
   int bytesPerScanline; 
   int byteOrder;	/* MSBFirst, LSBFirst */
   int depth;		
   int bitsPerPixel;
   unsigned long red_mask;
   unsigned long green_mask;
   unsigned long blue_mask;
   short visualClass;
   int viewportWidth;
   int viewportHeight;
   int xViewportStep;	/* viewport position granularity */
   int yViewportStep;
   int maxViewportX;	/* max viewport origin */
   int maxViewportY;
   int viewportFlags;	/* types of page flipping possible */
   int offset;
   int reserved1;
   int reserved2;
} XDGAModeRec, *XDGAModePtr;


void XFree86DGAExtensionInit(void);

/* DDX interface */

int
DGASetMode(
   int index,
   int num,
   XDGAModePtr mode,
   PixmapPtr *pPix
);

void 
DGASelectInput(
   int index,
   ClientPtr client,
   long mask
);

Bool DGAAvailable(int index);
Bool DGAActive(int index);
void DGAShutdown(void);
void DGAInstallCmap(ColormapPtr cmap);
int DGAGetViewportStatus(int index); 
int DGASync(int index);

int
DGAFillRect(
   int index,
   int x, int y, int w, int h,
   unsigned long color
);

int
DGABlitRect(
   int index,
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty
);

int
DGABlitTransRect(
   int index,
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty,
   unsigned long color
);

int
DGASetViewport(
   int index,
   int x, int y,
   int mode
); 

int DGAGetModes(int index);
int DGAGetOldDGAMode(int index);

int DGAGetModeInfo(int index, XDGAModePtr mode, int num);

Bool DGAVTSwitch(void);
Bool DGAStealMouseEvent(int index, xEvent *e, int dx, int dy);
Bool DGAStealKeyEvent(int index, xEvent *e);
Bool DGAIsDgaEvent (xEvent *e);

Bool DGADeliverEvent (ScreenPtr pScreen, xEvent *e);
	    
Bool DGAOpenFramebuffer(int index, char **name, unsigned char **mem, 
			int *size, int *offset, int *flags);
void DGACloseFramebuffer(int index);
Bool DGAChangePixmapMode(int index, int *x, int *y, int mode);
int DGACreateColormap(int index, ClientPtr client, int id, int mode, 
			int alloc);

extern unsigned char DGAReqCode;
extern int DGAErrorBase;
extern int DGAEventBase;
extern int *XDGAEventBase;



#endif /* __DGAPROC_H */
