/* $XFree86: xc/programs/Xserver/hw/xfree86/xf4bpp/wm3.h,v 1.2 1998/07/25 16:59:46 dawes Exp $ */





/* $XConsortium: wm3.h /main/4 1996/02/21 17:59:24 kaleb $ */

#include "vgaReg.h"

#ifdef	PC98_EGC
#define VGA_ALLPLANES 0xFL
#endif

/* Do call in Write Mode 3.
 * We take care of the possibility that two passes are needed.
 */
#ifndef	PC98_EGC
#define DO_WM3(pgc,call) \
   { int _tp, _fg, _bg, _alu; \
	_fg = pgc->fgPixel; _bg = pgc->bgPixel; \
	_tp = wm3_set_regs(pgc); \
        (call); \
	if ( _tp ) { \
           _alu = pgc->alu; \
	   pgc->alu = GXinvert; \
	   _tp = wm3_set_regs(pgc); \
	   (call); \
           pgc->alu = _alu; \
	} \
	pgc->fgPixel = _fg; pgc->bgPixel = _bg; \
    }
#else
#define DO_WM3(pgc,call) \
   { int _tp, _fg, _bg; \
	_fg = pgc->fgPixel; _bg = pgc->bgPixel; \
	_tp = wm3_set_regs(pgc); \
        (call); \
	pgc->fgPixel = _fg; pgc->bgPixel = _bg; \
    }
#endif

#ifndef PC98_EGC
#define WM3_SET_INK(ink) \
    SetVideoGraphics(Set_ResetIndex, ink)
#else
#define WM3_SET_INK(ink) \
	outw(EGC_FGC, ink)
#endif

/* GJA -- Move a long word to screen memory.
 * The reads into 'dummy' are here to load the VGA latches.
 * This is a RMW operation except for trivial cases.
 * Notice that we ignore the operation.
 */
#ifdef	PC98_EGC
#define UPDRW(destp,src) \
	{ volatile unsigned short *_dtmp = \
		(volatile unsigned short *)(destp); \
	  unsigned int _stmp = (src); \
	  *_dtmp = _stmp; _dtmp++; _stmp >>= 16; \
	  *_dtmp = _stmp; }
#else
#define UPDRW(destp,src) \
	{ volatile char *_dtmp = (volatile char *)(destp); \
	  int _stmp = (src); \
	  volatile int dummy; /* Bit bucket. */ \
	  _stmp = ldl_u(&_stmp); \
	  dummy = *_dtmp; *_dtmp = _stmp; _dtmp++; _stmp >>= 8; \
	  dummy = *_dtmp; *_dtmp = _stmp; _dtmp++; _stmp >>= 8; \
	  dummy = *_dtmp; *_dtmp = _stmp; _dtmp++; _stmp >>= 8; \
	  dummy = *_dtmp; *_dtmp = _stmp; }
#endif

#define UPDRWB(destp,src) \
	{ volatile int dummy; /* Bit bucket. */ \
	  dummy = *(destp); *(destp) = (src); }
