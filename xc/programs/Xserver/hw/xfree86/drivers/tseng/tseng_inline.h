/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_inline.h,v 1.6 1998/09/05 06:36:56 dawes Exp $ */





#include "tseng.h"

/*
 * Some commonly used inline functions and utility functions.
 */

static __inline__ int
COLOR_REPLICATE_DWORD(TsengPtr pTseng, int color)
{
    switch (pTseng->Bytesperpixel) {
    case 1:
	color &= 0xFF;
	color = (color << 8) | color;
	color = (color << 16) | color;
	break;
    case 2:
	color &= 0xFFFF;
	color = (color << 16) | color;
	break;
    }
    return color;
}

/*
 * Optimizing note: increasing the wrap size for fixed-color source/pattern
 * tiles from 4x1 (as below) to anything bigger doesn't seem to affect
 * performance (it might have been better for larger wraps, but it isn't).
 */

static __inline__ void
SET_FG_COLOR(TsengPtr pTseng, int color)
{
    *ACL_SOURCE_ADDRESS = tsengFg;
    *ACL_SOURCE_Y_OFFSET = 3;
    color = COLOR_REPLICATE_DWORD(pTseng, color);
    *tsengMemFg = color;
    if (Is_W32p || Is_ET6K) {
	*ACL_SOURCE_WRAP = 0x02;
    } else {
	*(tsengMemFg + 1) = color;
	*ACL_SOURCE_WRAP = 0x12;
    }
}

static __inline__ void
SET_BG_COLOR(TsengPtr pTseng, int color)
{
    *ACL_PATTERN_ADDRESS = tsengPat;
    *ACL_PATTERN_Y_OFFSET = 3;
    color = COLOR_REPLICATE_DWORD(pTseng, color);
    *tsengMemPat = color;
    if (Is_W32p || Is_ET6K) {
	*ACL_PATTERN_WRAP = 0x02;
    } else {
	*(tsengMemPat + 1) = color;
	*ACL_PATTERN_WRAP = 0x12;
    }
}

/*
 * this does the same as SET_FG_COLOR and SET_BG_COLOR together, but is
 * faster, because it allows the PCI chipset to chain the requests into a
 * burst sequence. The order of the commands is partly linear.
 * So far for the theory...
 */
static __inline__ void
SET_FG_BG_COLOR(TsengPtr pTseng, int fgcolor, int bgcolor)
{
    *ACL_PATTERN_ADDRESS = tsengPat;
    *ACL_SOURCE_ADDRESS = tsengFg;
    *((LongP) ACL_PATTERN_Y_OFFSET) = 0x00030003;
    fgcolor = COLOR_REPLICATE_DWORD(pTseng, fgcolor);
    bgcolor = COLOR_REPLICATE_DWORD(pTseng, bgcolor);
    *tsengMemFg = fgcolor;
    *tsengMemPat = bgcolor;
    if (Is_W32p || Is_ET6K) {
	*((LongP) ACL_PATTERN_WRAP) = 0x00020002;
    } else {
	*(tsengMemFg + 1) = fgcolor;
	*(tsengMemPat + 1) = bgcolor;
	*((LongP) ACL_PATTERN_WRAP) = 0x00120012;
    }
}

/*
 * Real 32-bit multiplications are horribly slow compared to 16-bit (on i386).
 */
#ifdef NO_OPTIMIZE
static __inline__ int
MULBPP(TsengPtr pTseng, int x)
{
    return (x * pTseng->Bytesperpixel);
}
#else
static __inline__ int
MULBPP(TsengPtr pTseng, int x)
{
    int result = x << pTseng->powerPerPixel;

    if (pTseng->Bytesperpixel != 3)
	return result;
    else
	return result + x;
}
#endif

static __inline__ int
CALC_XY(TsengPtr pTseng, int x, int y)
{
    int new_x, xy;

    if ((old_y == y) && (old_x == x))
	return -1;

    if (Is_W32p)
	new_x = MULBPP(pTseng, x - 1);
    else
	new_x = MULBPP(pTseng, x) - 1;
    xy = ((y - 1) << 16) + new_x;
    old_x = x;
    old_y = y;
    return xy;
}

/* generic SET_XY */
static __inline__ void
SET_XY(TsengPtr pTseng, int x, int y)
{
    int new_x;

    if (Is_W32p)
	new_x = MULBPP(pTseng, x - 1);
    else
	new_x = MULBPP(pTseng, x) - 1;
    *ACL_XY_COUNT = ((y - 1) << 16) + new_x;
    old_x = x;
    old_y = y;
}

static __inline__ void
SET_X_YRAW(TsengPtr pTseng, int x, int y)
{
    int new_x;

    if (Is_W32p)
	new_x = MULBPP(pTseng, x - 1);
    else
	new_x = MULBPP(pTseng, x) - 1;
    *ACL_XY_COUNT = (y << 16) + new_x;
    old_x = x;
    old_y = y - 1;		       /* old_y is invalid (raw transfer) */
}

/*
 * This is plain and simple "benchmark rigging".
 * (no real application does lots of subsequent same-size blits)
 *
 * The effect of this is amazingly good on e.g large blits: 400x400
 * rectangle fill in 24 and 32 bpp on ET6000 jumps from 276 MB/sec to up to
 * 490 MB/sec... But not always. There must be a good reason why this gives
 * such a boost, but I don't know it.
 */

static __inline__ void
SET_XY_4(TsengPtr pTseng, int x, int y)
{
    int new_xy;

    if ((old_y != y) || (old_x != x)) {
	new_xy = ((y - 1) << 16) + MULBPP(pTseng, x - 1);
	*ACL_XY_COUNT = new_xy;
	old_x = x;
	old_y = y;
    }
}

static __inline__ void
SET_XY_6(TsengPtr pTseng, int x, int y)
{
    int new_xy;			       /* using this intermediate variable is faster */

    if ((old_y != y) || (old_x != x)) {
	new_xy = ((y - 1) << 16) + MULBPP(pTseng, x) - 1;
	*ACL_XY_COUNT = new_xy;
	old_x = x;
	old_y = y;
    }
}

/* generic SET_XY_RAW */
static __inline__ void
SET_XY_RAW(int x, int y)
{
    *ACL_XY_COUNT = (y << 16) + x;
    old_x = old_y = -1;		       /* invalidate old_x/old_y (raw transfers) */
}

static __inline__ void
PINGPONG(void)
{
    if (tsengFg == W32ForegroundPing) {
	tsengMemFg = MemW32ForegroundPong;
	tsengFg = W32ForegroundPong;
	tsengMemBg = MemW32BackgroundPong;
	tsengBg = W32BackgroundPong;
	tsengMemPat = MemW32PatternPong;
	tsengPat = W32PatternPong;
    } else {
	tsengMemFg = MemW32ForegroundPing;
	tsengFg = W32ForegroundPing;
	tsengMemBg = MemW32BackgroundPing;
	tsengBg = W32BackgroundPing;
	tsengMemPat = MemW32PatternPing;
	tsengPat = W32PatternPing;
    }
}

/*
 * This is called in each ACL function just before the first ACL register is
 * written to. It waits for the accelerator to finish on cards that don't
 * support hardware-wait-state locking, and waits for a free queue entry on
 * others, if hardware-wait-states are not enabled.
 */
static __inline__ void
wait_acl_queue(TsengPtr pTseng)
{
    if (pTseng->UsePCIRetry)
	WAIT_QUEUE;
    if (pTseng->need_wait_acl)
	WAIT_ACL;
}

