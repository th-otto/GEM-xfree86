/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/vvga.h,v 1.3 1999/10/13 04:21:25 dawes Exp $ */
/*
 * file vvga.h
 *
 * Headerfile for vvga.c
 */

#ifndef __VVGA_H__
#define __VVGA_H__



/*
 * includes
 */

#include "vtypes.h"



/*
 * function prototypes
 */

void v_resetvga(void);
void v_loadvgafont(void);
void v_textmode(struct v_board_t *board);
void v_savetextmode(struct v_board_t *board);
void v_restoretextmode(struct v_board_t *board);
void v_restorepalette(void);



#endif /* __VVGA_H__ */

/*
 * end of file vvga.h
 */
