/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/vramdac.h,v 1.4 2000/02/25 21:03:07 dawes Exp $ */
/*
 * file vramdac.h
 *
 * headfile for vramdac.c
 */

#ifndef __VRAMDAC_H__
#define __VRAMDAC_H__



/*
 * includes
 */

#include "vtypes.h"



/*
 * defines
 */

#define V_NOCURSOR  0
#define V_2COLORS   1
#define V_3COLORS   2
#define V_XCURSOR   3

#define V_CURSOR32  0
#define V_CURSOR64  1



/*
 * function prototypes
 */

int v_initdac(ScrnInfoPtr pScreenInfo, vu8 bpp, vu8 doubleclock);
void v_enablecursor(ScrnInfoPtr pScreenInfo, int type, int size);
void v_movecursor(ScrnInfoPtr pScreenInfo, vu16 x, vu16 y, vu8 xo, vu8 yo);
void v_setcursorcolor(ScrnInfoPtr pScreenInfo, vu32 bg, vu32 fg);
void v_loadcursor(ScrnInfoPtr pScreenInfo, vu8 type, vu8 *cursorimage);
void v_setpalette(ScrnInfoPtr pScreenInfo, int numColors, int *indices,
		  LOCO *colors, VisualPtr pVisual);



#endif /* #ifndef _VRAMDAC_H_ */

/*
 * end of file vramdac.h
 */
