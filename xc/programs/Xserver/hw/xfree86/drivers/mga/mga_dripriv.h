/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_dripriv.h,v 1.1 2000/02/11 17:25:55 dawes Exp $ */

#ifndef _MGA_DRIPRIV_H_
#define _MGA_DRIPRIV_H_

#define MGA_MAX_DRAWABLES 256

extern void GlxSetVisualConfigs(
    int nconfigs,
    __GLXvisualConfig *configs,
    void **configprivs
);

typedef struct {
  /* Nothing here yet */
  int dummy;
} MGAConfigPrivRec, *MGAConfigPrivPtr;

typedef struct {
  /* Nothing here yet */
  int dummy;
} MGADRIContextRec, *MGADRIContextPtr;

#endif
