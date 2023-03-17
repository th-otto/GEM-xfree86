/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/vmodes.h,v 1.3 1999/10/13 04:21:24 dawes Exp $ */
/*
 * file vmodes.h
 *
 * headerfile for vmodes.c
 */

#ifndef __VMODES_H__
#define __VMODES_H__



/*
 * includes
 */

#include "vtypes.h"



/*
 * function prototypes
 */

int v_setmodefixed(ScrnInfoPtr pScreenInfo);
int v_setmode(ScrnInfoPtr pScreenInfo, struct v_modeinfo_t *mode);
void v_setframebase(ScrnInfoPtr pScreenInfo, vu32 framebase);
int v_getstride(ScrnInfoPtr pScreenInfo, int *width, vu16 *stride0, vu16 *stride1);



#endif /* #ifndef _VMODES_H_ */

/*
 * end of file vmodes.h
 */
