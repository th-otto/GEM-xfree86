/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/vboard.h,v 1.5 2000/02/25 21:03:04 dawes Exp $ */

/*
 * vboard.h
 *
 * functions to interact with a Verite board
 */

#ifndef __VBOARD_H__
#define __VBOARD_H__


/*
 * includes
 */

#include "rendition.h"
#include "vtypes.h"

/*
 * function prototypes
 */

int v_initboard(ScrnInfoPtr pScreenInfo);
int v_resetboard(ScrnInfoPtr pScreenInfo);
int v_getmemorysize(ScrnInfoPtr pScreenInfo);

void v_check_csucode(ScrnInfoPtr pScreenInfo);


#endif /* __VBOARD_H__ */

/*
 * end of file vboard.h
 */
