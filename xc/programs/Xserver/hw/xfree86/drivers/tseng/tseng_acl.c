
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_acl.c,v 1.21 1998/09/05 06:36:55 dawes Exp $ */





#include "tseng.h"
#include "tseng_acl.h"

void tseng_terminate_acl(TsengPtr pTseng);

ByteP W32Buffer;

LongP MBP0, MBP1, MBP2;
ByteP MMU_CONTROL;

ByteP ACL_SUSPEND_TERMINATE, ACL_OPERATION_STATE, ACL_SYNC_ENABLE, ACL_WRITE_INTERFACE_VALID,
    ACL_INTERRUPT_MASK, ACL_INTERRUPT_STATUS, ACL_ACCELERATOR_STATUS;

WordP ACL_X_POSITION, ACL_Y_POSITION;

WordP ACL_NQ_X_POSITION, ACL_NQ_Y_POSITION;

LongP ACL_PATTERN_ADDRESS, ACL_SOURCE_ADDRESS;

WordP ACL_PATTERN_Y_OFFSET, ACL_SOURCE_Y_OFFSET, ACL_DESTINATION_Y_OFFSET;

ByteP ACL_VIRTUAL_BUS_SIZE,	       /* only for w32 and w32i */
    ACL_XY_DIRECTION,		       /* only for w32 and w32i */
    ACL_PIXEL_DEPTH;		       /* only for w32p_rev_A and w32p_rev_B */

ByteP ACL_PATTERN_WRAP, ACL_SOURCE_WRAP;

WordP ACL_X_COUNT, ACL_Y_COUNT;
LongP ACL_XY_COUNT;		       /* for combined writes to X and Y count registers */

ByteP ACL_ROUTING_CONTROL, ACL_RELOAD_CONTROL, ACL_BACKGROUND_RASTER_OPERATION,
    ACL_FOREGROUND_RASTER_OPERATION;

LongP ACL_DESTINATION_ADDRESS,	       /* only for w32p_rev_A and w32p_rev_B */
    ACL_MIX_ADDRESS;

WordP ACL_MIX_Y_OFFSET, ACL_ERROR_TERM, ACL_DELTA_MINOR, ACL_DELTA_MAJOR;

/* for ET6000 only */
ByteP ACL_POWER_CONTROL;

ByteP ACL_SECONDARY_EDGE;
WordP ACL_SECONDARY_ERROR_TERM, ACL_SECONDARY_DELTA_MINOR, ACL_SECONDARY_DELTA_MAJOR;
ByteP ACL_TRANSFER_DISABLE;

ByteP W32BytePtr;
WordP W32WordPtr;
LongP W32LongPtr;

/*
 * conversion from X ROPs to Microsoft ROPs.
 */

int W32OpTable[] =
{
    0x00,			       /* Xclear             0 */
    0x88,			       /* Xand               src AND dst */
    0x44,			       /* XandReverse        src AND NOT dst */
    0xcc,			       /* Xcopy              src */
    0x22,			       /* XandInverted       NOT src AND dst */
    0xaa,			       /* Xnoop              dst */
    0x66,			       /* Xxor               src XOR dst */
    0xee,			       /* Xor                src OR dst */
    0x11,			       /* Xnor               NOT src AND NOT dst */
    0x99,			       /* Xequiv             NOT src XOR dst */
    0x55,			       /* Xinvert            NOT dst */
    0xdd,			       /* XorReverse         src OR NOT dst */
    0x33,			       /* XcopyInverted      NOT src */
    0xbb,			       /* XorInverted        NOT src OR dst */
    0x77,			       /* Xnand              NOT src OR NOT dst */
    0xff			       /* Xset               1 */
};

int W32OpTable_planemask[] =
{
    0x0a,			       /* Xclear             0 */
    0x8a,			       /* Xand               src AND dst */
    0x4a,			       /* XandReverse        src AND NOT dst */
    0xca,			       /* Xcopy              src */
    0x2a,			       /* XandInverted       NOT src AND dst */
    0xaa,			       /* Xnoop              dst */
    0x6a,			       /* Xxor               src XOR dst */
    0xea,			       /* Xor                src OR dst */
    0x1a,			       /* Xnor               NOT src AND NOT dst */
    0x9a,			       /* Xequiv             NOT src XOR dst */
    0x5a,			       /* Xinvert            NOT dst */
    0xda,			       /* XorReverse         src OR NOT dst */
    0x3a,			       /* XcopyInverted      NOT src */
    0xba,			       /* XorInverted        NOT src OR dst */
    0x7a,			       /* Xnand              NOT src OR NOT dst */
    0xfa			       /* Xset               1 */
};

int W32PatternOpTable[] =
{
    0x00,			       /* Xclear             0 */
    0xa0,			       /* Xand               pat AND dst */
    0x50,			       /* XandReverse        pat AND NOT dst */
    0xf0,			       /* Xcopy              pat */
    0x0a,			       /* XandInverted       NOT pat AND dst */
    0xaa,			       /* Xnoop              dst */
    0x5a,			       /* Xxor               pat XOR dst */
    0xfa,			       /* Xor                pat OR dst */
    0x05,			       /* Xnor               NOT pat AND NOT dst */
    0xa5,			       /* Xequiv             NOT pat XOR dst */
    0x55,			       /* Xinvert            NOT dst */
    0xf5,			       /* XorReverse         pat OR NOT dst */
    0x0f,			       /* XcopyInverted      NOT pat */
    0xaf,			       /* XorInverted        NOT pat OR dst */
    0x5f,			       /* Xnand              NOT pat OR NOT dst */
    0xff			       /* Xset               1 */
};

long W32ForegroundPing;
long W32ForegroundPong;
long W32BackgroundPing;
long W32BackgroundPong;
long W32PatternPing;
long W32PatternPong;

LongP MemW32ForegroundPing;
LongP MemW32ForegroundPong;
LongP MemW32BackgroundPing;
LongP MemW32BackgroundPong;
LongP MemW32PatternPing;
LongP MemW32PatternPong;

unsigned char * tsengCPU2ACLBase;

/* used for optimisation of direction-register writing */
int tseng_old_dir = -1;
int old_x = 0, old_y = 0;

/* These will hold the ping-pong registers.
 * Note that ping-pong registers might not be needed when using
 * BACKGROUND_OPERATIONS (because of the WAIT()-ing involved)
 */

LongP tsengMemFg;
long tsengFg;

LongP tsengMemBg;
long tsengBg;

LongP tsengMemPat;
long tsengPat;

/**********************************************************************/

void 
tseng_terminate_acl(TsengPtr pTseng)
{
    /* only terminate when needed */
/*  if (*(volatile unsigned char *)ACL_ACCELERATOR_STATUS & 0x06) */
    {
	*ACL_SUSPEND_TERMINATE = 0x00;
	/* suspend any running operation */
	*ACL_SUSPEND_TERMINATE = 0x01;
	WAIT_ACL;
	*ACL_SUSPEND_TERMINATE = 0x00;
	/* ... and now terminate it */
	*ACL_SUSPEND_TERMINATE = 0x10;
	WAIT_ACL;
	*ACL_SUSPEND_TERMINATE = 0x00;
    }
}

void 
tseng_recover_timeout(TsengPtr pTseng)
{
    if (!Is_ET6K) {
	ErrorF("trying to unlock......................................\n");
	*tsengCPU2ACLBase = 0L;	       /* try unlocking the bus when CPU-to-accel gets stuck */
    }
    if (Is_W32p) {		       /* flush the accelerator pipeline */
	*ACL_SUSPEND_TERMINATE = 0x00;
	*ACL_SUSPEND_TERMINATE = 0x02;
	*ACL_SUSPEND_TERMINATE = 0x00;
    }
}

long MMioBase;

void 
tseng_init_acl(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TsengPtr pTseng = TsengPTR(pScrn);
    long scratchMemBase;

    PDEBUG("	tseng_init_acl\n");
    /*
     * prepare some shortcuts for faster access to memory mapped registers
     */

    if (pTseng->UseLinMem) {
	MMioBase = (long)pTseng->FbBase + 0x3FFF00;
	scratchMemBase = (long)pTseng->FbBase + pTseng->AccelColorBufferOffset;
	/* 
	 * we won't be using tsengCPU2ACLBase in linear memory mode anyway, since
	 * using the MMU apertures restricts the amount of useable video memory
	 * to only 2MB, supposing we ONLY redirect MMU aperture 2 to the CPU.
	 * (see data book W32p, page 207)
	 */
	tsengCPU2ACLBase = (unsigned char*) ((long)pTseng->FbBase + 0x200000);	/* MMU aperture 2 */
    } else {
	MMioBase = (long)pTseng->FbBase + 0x1FF00L;
	/*
	 * MMU 0 is used for the scratchpad (i.e. FG and BG colors).
	 *
	 * MMU 1 is used for the Imagewrite buffers. This code assumes those
	 * buffers are back-to-back, with AccelImageWriteBufferOffsets[0]
	 * being the first, and don't exceed 8kb (aperture size) in total
	 * length.
	 */
	scratchMemBase = (long)pTseng->FbBase + 0x18000L;
	*((LongP) (MMioBase + 0x00)) = pTseng->AccelColorBufferOffset;
	*((LongP) (MMioBase + 0x04)) = pTseng->AccelImageWriteBufferOffsets[0];
	/*
	 * tsengCPU2ACLBase is used for CPUtoSCreen...() operations on < ET6000 devices
	 */
	tsengCPU2ACLBase = (unsigned char*) ((long)pTseng->FbBase + 0x1C000L);	/* MMU aperture 2 */
	/*      *((LongP) (MMioBase + 0x08)) = 200000; *//* TEST */
    }
    
    /* ErrorF("MMioBase = 0x%x, scratchMemBase = 0x%x\n", MMioBase, scratchMemBase); */

    MMU_CONTROL = (ByteP) (MMioBase + 0x13);

    ACL_SUSPEND_TERMINATE = (ByteP) (MMioBase + 0x30);
    ACL_OPERATION_STATE = (ByteP) (MMioBase + 0x31);

    ACL_SYNC_ENABLE = (ByteP) (MMioBase + 0x32);
    /* for ET6000, ACL_SYNC_ENABLE becomes ACL_6K_CONFIG */

    ACL_WRITE_INTERFACE_VALID = (ByteP) (MMioBase + 0x33);
    ACL_INTERRUPT_MASK = (ByteP) (MMioBase + 0x34);
    ACL_INTERRUPT_STATUS = (ByteP) (MMioBase + 0x35);
    ACL_ACCELERATOR_STATUS = (ByteP) (MMioBase + 0x36);

    /* and this is only for the ET6000 */
    ACL_POWER_CONTROL = (ByteP) (MMioBase + 0x37);

    /* non-queued for w32p's and ET6000 */
    ACL_NQ_X_POSITION = (WordP) (MMioBase + 0x38);
    ACL_NQ_Y_POSITION = (WordP) (MMioBase + 0x3A);
    /* queued for w32 and w32i */
    ACL_X_POSITION = (WordP) (MMioBase + 0x94);
    ACL_Y_POSITION = (WordP) (MMioBase + 0x96);

    ACL_PATTERN_ADDRESS = (LongP) (MMioBase + 0x80);
    ACL_SOURCE_ADDRESS = (LongP) (MMioBase + 0x84);

    ACL_PATTERN_Y_OFFSET = (WordP) (MMioBase + 0x88);
    ACL_SOURCE_Y_OFFSET = (WordP) (MMioBase + 0x8A);
    ACL_DESTINATION_Y_OFFSET = (WordP) (MMioBase + 0x8C);

    /* W32i */
    ACL_VIRTUAL_BUS_SIZE = (ByteP) (MMioBase + 0x8E);
    /* w32p */
    ACL_PIXEL_DEPTH = (ByteP) (MMioBase + 0x8E);

    /* w32 and w32i */
    ACL_XY_DIRECTION = (ByteP) (MMioBase + 0x8F);

    ACL_PATTERN_WRAP = (ByteP) (MMioBase + 0x90);
    ACL_TRANSFER_DISABLE = (ByteP) (MMioBase + 0x91);	/* ET6000 only */
    ACL_SOURCE_WRAP = (ByteP) (MMioBase + 0x92);

    ACL_X_COUNT = (WordP) (MMioBase + 0x98);
    ACL_Y_COUNT = (WordP) (MMioBase + 0x9A);
    ACL_XY_COUNT = (LongP) (ACL_X_COUNT);	/* shortcut. not a real register */

    ACL_ROUTING_CONTROL = (ByteP) (MMioBase + 0x9C);
    /* for ET6000, ACL_ROUTING_CONTROL becomes ACL_MIX_CONTROL */
    ACL_RELOAD_CONTROL = (ByteP) (MMioBase + 0x9D);
    /* for ET6000, ACL_RELOAD_CONTROL becomes ACL_STEPPING_INHIBIT */

    ACL_BACKGROUND_RASTER_OPERATION = (ByteP) (MMioBase + 0x9E);
    ACL_FOREGROUND_RASTER_OPERATION = (ByteP) (MMioBase + 0x9F);

    ACL_DESTINATION_ADDRESS = (LongP) (MMioBase + 0xA0);

    /* the following is for the w32p's only */
    ACL_MIX_ADDRESS = (LongP) (MMioBase + 0xA4);

    ACL_MIX_Y_OFFSET = (WordP) (MMioBase + 0xA8);
    ACL_ERROR_TERM = (WordP) (MMioBase + 0xAA);
    ACL_DELTA_MINOR = (WordP) (MMioBase + 0xAC);
    ACL_DELTA_MAJOR = (WordP) (MMioBase + 0xAE);

    /* ET6000 only (trapezoids) */
    ACL_SECONDARY_EDGE = (ByteP) (MMioBase + 0x93);
    ACL_SECONDARY_ERROR_TERM = (WordP) (MMioBase + 0xB2);
    ACL_SECONDARY_DELTA_MINOR = (WordP) (MMioBase + 0xB4);
    ACL_SECONDARY_DELTA_MAJOR = (WordP) (MMioBase + 0xB6);

    /* addresses in video memory (i.e. "0" = first byte in video memory) */
    W32ForegroundPing = pTseng->AccelColorBufferOffset + 0;
    W32ForegroundPong = pTseng->AccelColorBufferOffset + 8;

    W32BackgroundPing = pTseng->AccelColorBufferOffset + 16;
    W32BackgroundPong = pTseng->AccelColorBufferOffset + 24;

    W32PatternPing = pTseng->AccelColorBufferOffset + 32;
    W32PatternPong = pTseng->AccelColorBufferOffset + 40;

    /* addresses in the memory map */
    MemW32ForegroundPing = (LongP) (scratchMemBase + 0);
    MemW32ForegroundPong = (LongP) (scratchMemBase + 8);

    MemW32BackgroundPing = (LongP) (scratchMemBase + 16);
    MemW32BackgroundPong = (LongP) (scratchMemBase + 24);

    MemW32PatternPing = (LongP) (scratchMemBase + 32);
    MemW32PatternPong = (LongP) (scratchMemBase + 40);

    /*
     * prepare the accelerator for some real work
     */

    tseng_terminate_acl(pTseng);

    *ACL_INTERRUPT_STATUS = 0xe;       /* clear interrupts */
    *ACL_INTERRUPT_MASK = 0x04;	       /* disable interrupts, but enable deadlock exit */
    *ACL_INTERRUPT_STATUS = 0x0;
    *ACL_ACCELERATOR_STATUS = 0x0;

    if (Is_ET6K) {
	*ACL_STEPPING_INHIBIT = 0x0;   /* Undefined at power-on, let all maps (Src, Dst, Mix, Pat) step */
	*ACL_6K_CONFIG = 0x00;	       /* maximum performance -- what did you think? */
	*ACL_POWER_CONTROL = 0x01;     /* conserve power when ACL is idle */
	*ACL_MIX_CONTROL = 0x33;
	*ACL_TRANSFER_DISABLE = 0x00;  /* Undefined at power-on, enable all transfers */
    } else {			       /* W32i/W32p */
	*ACL_RELOAD_CONTROL = 0x0;
	*ACL_SYNC_ENABLE = 0x1;	       /* | 0x2 = 0WS ACL read. Yields up to 10% faster operation for small blits */
	*ACL_ROUTING_CONTROL = 0x00;
    }

    if (Is_W32p || Is_ET6K) {
	/* Enable the W32p startup bit and set use an eight-bit pixel depth */
	*ACL_NQ_X_POSITION = 0;
	*ACL_NQ_Y_POSITION = 0;
	*ACL_PIXEL_DEPTH = (pScrn->bitsPerPixel - 8) << 1;
	/* writing destination address will start ACL */
	*ACL_OPERATION_STATE = 0x10;
    } else {
	/* X, Y positions set to zero's for w32 and w32i */
	*ACL_X_POSITION = 0;
	*ACL_Y_POSITION = 0;
	*ACL_OPERATION_STATE = 0x0;
	/* if we ever use CPU-to-screen pixmap uploading on W32I or W32,
	 * ACL_VIRTUAL_BUS_SIZE will need to be made dynamic (i.e. moved to
	 * Setup() functions).
	 *
	 * VBS = 1 byte is faster than VBS = 4 bytes, since the ACL can
	 * start processing as soon as the first byte arrives.
	 */
	*ACL_VIRTUAL_BUS_SIZE = 0x00;
    }
    *ACL_DESTINATION_Y_OFFSET = pScrn->displayWidth * pTseng->Bytesperpixel - 1;
    *ACL_XY_DIRECTION = 0;

    *MMU_CONTROL = 0x74;

    if (Is_W32p && pTseng->UseLinMem) {
	/*
	 * Since the w32p revs C and D don't have any memory mapped when the
	 * accelerator registers are used it is necessary to use the MMUs to
	 * provide a semblance of linear memory. Fortunately on these chips
	 * the MMU appertures are 1 megabyte each. So as long as we are
	 * willing to only use 3 megs of video memory we can have some
	 * acceleration. If we ever get the CPU-to-screen-color-expansion
	 * stuff working then we will NOT need to sacrifice the extra 1MB
	 * provided by MBP2, because we could do dynamic switching of the APT
	 * bit in the MMU control register.
	 *
	 * On W32p rev c and d MBP2 is hardwired to 0x200000 when linear
	 * memory mode is enabled. (On rev a it is programmable).
	 *
	 * W32p rev a and b have their first 2M mapped in the normal (non-MMU)
	 * way, and MMU0 and MMU1, each 512 kb wide, can be used to access
	 * another 1MB of memory. This totals to 3MB of mem. available in
	 * linear memory when the accelerator is enabled.
	 */
	if (Is_W32p_ab) {
	    *((LongP) (MMioBase + 0x00)) = 0x200000L;
	    *((LongP) (MMioBase + 0x04)) = 0x280000L;
	} else {		       /* rev C & D */
	    *((LongP) (MMioBase + 0x00)) = 0x0L;
	    *((LongP) (MMioBase + 0x04)) = 0x100000L;
	}
    }
}
