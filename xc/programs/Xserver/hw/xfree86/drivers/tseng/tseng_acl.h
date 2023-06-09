
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_acl.h,v 1.17 1998/09/05 06:36:55 dawes Exp $ */





#ifndef _TSENG_ACL_H
#define _TSENG_ACL_H

/*
 * if NO_OPTIMIZE is set, some optimizations are disabled.
 *
 * What it basically tries to do is minimize the amounts of writes to
 * accelerator registers, since these are the ones that slow down small
 * operations a lot.
 */

#undef NO_OPTIMIZE

typedef volatile unsigned char *ByteP;
typedef volatile unsigned short *WordP;
typedef volatile unsigned *LongP;

void tseng_recover_timeout(TsengPtr pTseng);

/*
 * Shortcuts to Tseng memory-mapped accelerator-control registers
 */

extern
ByteP MMU_CONTROL;

extern
ByteP ACL_SUSPEND_TERMINATE, ACL_OPERATION_STATE, ACL_SYNC_ENABLE, ACL_WRITE_INTERFACE_VALID,
    ACL_INTERRUPT_MASK, ACL_INTERRUPT_STATUS, ACL_ACCELERATOR_STATUS;

/* for ET6000: */
#define ACL_6K_CONFIG ACL_SYNC_ENABLE

extern
WordP ACL_X_POSITION, ACL_Y_POSITION;

extern
WordP ACL_NQ_X_POSITION, ACL_NQ_Y_POSITION;

extern
LongP ACL_PATTERN_ADDRESS, ACL_SOURCE_ADDRESS;

extern
WordP ACL_PATTERN_Y_OFFSET, ACL_SOURCE_Y_OFFSET, ACL_DESTINATION_Y_OFFSET;

extern
ByteP ACL_VIRTUAL_BUS_SIZE,	       /* only for w32 and w32i */
    ACL_XY_DIRECTION, ACL_PIXEL_DEPTH; /* only for w32p_rev_A and w32p_rev_B */

extern
ByteP ACL_PATTERN_WRAP, ACL_SOURCE_WRAP;

extern
WordP ACL_X_COUNT, ACL_Y_COUNT;
extern
LongP ACL_XY_COUNT;		       /* for combined writes to X and Y count registers */

extern
ByteP ACL_ROUTING_CONTROL, ACL_RELOAD_CONTROL, ACL_BACKGROUND_RASTER_OPERATION,
    ACL_FOREGROUND_RASTER_OPERATION;

/* for ET6000: */
#define ACL_MIX_CONTROL ACL_ROUTING_CONTROL
#define ACL_STEPPING_INHIBIT ACL_RELOAD_CONTROL

extern
LongP ACL_DESTINATION_ADDRESS,	       /* only for w32p_rev_A and w32p_rev_B */
    ACL_MIX_ADDRESS;

extern
WordP ACL_MIX_Y_OFFSET, ACL_ERROR_TERM, ACL_DELTA_MINOR, ACL_DELTA_MAJOR;

/* for ET6000 only */
extern
ByteP ACL_POWER_CONTROL;
extern
ByteP ACL_SECONDARY_EDGE;
extern
WordP ACL_SECONDARY_ERROR_TERM, ACL_SECONDARY_DELTA_MINOR, ACL_SECONDARY_DELTA_MAJOR;
extern
ByteP ACL_TRANSFER_DISABLE;

/*
 * Some data structures for faster accelerator programming.
 */

extern int W32OpTable[16];
extern int W32OpTable_planemask[16];
extern int W32PatternOpTable[16];

/*
 * The ping-pong registers. Probably too much hassle for too little gain. "TODO".
 */

extern long W32ForegroundPing;
extern long W32ForegroundPong;
extern long W32BackgroundPing;
extern long W32BackgroundPong;
extern long W32PatternPing;
extern long W32PatternPong;

extern LongP MemW32ForegroundPing;
extern LongP MemW32ForegroundPong;
extern LongP MemW32BackgroundPing;
extern LongP MemW32BackgroundPong;
extern LongP MemW32PatternPing;
extern LongP MemW32PatternPong;

extern unsigned char * tsengCPU2ACLBase;

/*
 * These will hold the ping-pong registers.
 */

extern LongP tsengMemFg;
extern long tsengFg;

extern LongP tsengMemBg;
extern long tsengBg;

extern LongP tsengMemPat;
extern long tsengPat;

/* for register write optimisation */
extern int old_x, old_y;
extern int tseng_old_dir;

/*
 * Some shortcuts. 
 */

#define MAX_WAIT_CNT 500000	       /* how long we wait before we time out */
#undef WAIT_VERBOSE		       /* if defined: print out how long we waited */

static __inline__ void 
tseng_wait(TsengPtr pTseng, ByteP reg, char *name, unsigned char mask)
{
    int cnt = MAX_WAIT_CNT;

    while (*reg & mask)
	if (--cnt < 0) {
	    ErrorF("WAIT_%s: timeout.\n", name);
	    tseng_recover_timeout(pTseng);
	    break;
	}
#ifdef WAIT_VERBOSE
    ErrorF("%s%d ", name, MAX_WAIT_CNT - cnt);
#endif
}

#define WAIT_QUEUE tseng_wait(pTseng, ACL_ACCELERATOR_STATUS, "QUEUE", 0x1)

/* This is only for W32p rev b...d */
#define WAIT_INTERFACE tseng_wait(pTseng, ACL_WRITE_INTERFACE_VALID, "INTERFACE", 0xf)

#define WAIT_ACL tseng_wait(pTseng, ACL_ACCELERATOR_STATUS, "ACL", 0x2)

#define WAIT_XY tseng_wait(pTseng, ACL_ACCELERATOR_STATUS, "XY", 0x4)

#define SET_FUNCTION_BLT \
    if (Is_ET6K) \
        *ACL_MIX_CONTROL     = 0x33; \
    else \
        *ACL_ROUTING_CONTROL = 0x00;

#define SET_FUNCTION_BLT_TR \
        *ACL_MIX_CONTROL     = 0x13;

#define FBADDR(pTseng, x,y) ( (y) * pTseng->line_width + MULBPP(pTseng, x) )

#define SET_FG_ROP(rop) \
    *ACL_FOREGROUND_RASTER_OPERATION = W32OpTable[rop];

#define SET_FG_ROP_PLANEMASK(rop) \
    *ACL_FOREGROUND_RASTER_OPERATION = W32OpTable_planemask[rop];

#define SET_BG_ROP(rop) \
    *ACL_BACKGROUND_RASTER_OPERATION = W32PatternOpTable[rop];

#define SET_BG_ROP_TR(rop, bg_color) \
  if ((bg_color) == -1)    /* transparent color expansion */ \
    *ACL_BACKGROUND_RASTER_OPERATION = 0xaa; \
  else \
    *ACL_BACKGROUND_RASTER_OPERATION = W32PatternOpTable[rop];

#define SET_DELTA(Min, Maj) \
    *((LongP) ACL_DELTA_MINOR) = ((Maj) << 16) + (Min)

#define SET_SECONDARY_DELTA(Min, Maj) \
    *((LongP) ACL_SECONDARY_DELTA_MINOR) = ((Maj) << 16) + (Min)

#ifdef NO_OPTIMIZE
#define SET_XYDIR(dir) \
      *ACL_XY_DIRECTION = (dir);
#else
/*
 * only changing ACL_XY_DIRECTION when it needs to be changed avoids
 * unnecessary PCI bus writes, which are slow. This shows up very well
 * on consecutive small fills.
 */
#define SET_XYDIR(dir) \
    if ((dir) != tseng_old_dir) \
      *ACL_XY_DIRECTION = tseng_old_dir = (dir);
#endif

#define SET_SECONDARY_XYDIR(dir) \
      *ACL_SECONDARY_EDGE = (dir);

/* Must do 0x09 (in one operation) for the W32 */
#define START_ACL(pTseng, dst) \
    *(ACL_DESTINATION_ADDRESS) = dst; \
    if (Is_W32 || Is_W32i) *ACL_OPERATION_STATE = 0x09;

/* START_ACL for the ET6000 */
#define START_ACL_6(dst) \
    *(ACL_DESTINATION_ADDRESS) = dst;

#define START_ACL_CPU(pTseng, dst) \
    if (Is_W32 || Is_W32i) \
      *((LongP) (MMioBase + 0x08)) = (CARD32)dst; /* writing to MMU2 will trigger accel at this address */ \
    else \
      *(ACL_DESTINATION_ADDRESS) = dst;

/*    *(ACL_DESTINATION_ADDRESS) = dst;    should be enough for START_ACL_CPU */

/***********************************************************************/

void tseng_init_acl(ScreenPtr pScreen);

Bool TsengXAAInit(ScreenPtr pScreen);

Bool TsengXAAInit_Colexp(ScrnInfoPtr pScrn);

#endif
