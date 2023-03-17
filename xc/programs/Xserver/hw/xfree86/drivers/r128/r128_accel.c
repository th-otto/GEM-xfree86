/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/r128/r128_accel.c,v 1.7 2000/02/23 04:47:18 martin Exp $ */
/**************************************************************************

Copyright 1999 ATI Technologies Inc. and Precision Insight, Inc.,
                                         Cedar Park, Texas. 
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Rickard E. Faith <faith@precisioninsight.com>
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *
 * Credits:
 *
 *   Thanks to Alan Hourihane <alanh@fairlite.demon..co.uk> and SuSE for
 *   providing source code to their 3.3.x Rage 128 driver.  Portions of
 *   this file are based on the acceleration code for that driver.
 *
 * References:
 *
 *   RAGE 128 VR/ RAGE 128 GL Register Reference Manual (Technical
 *   Reference Manual P/N RRG-G04100-C Rev. 0.04), ATI Technologies: April
 *   1999.
 *
 *   RAGE 128 Software Development Manual (Technical Reference Manual P/N
 *   SDK-G04000 Rev. 0.01), ATI Technologies: June 1999.
 *
 * Notes on unimplemented XAA optimizations:
 *
 *   SolidFillTrap: This will probably work if we can compute the correct
 *                  Bresenham error values.
 *   TwoPointLine:  The Rage 128 supports Bresenham lines instead.
 *   DashedLine with non-power-of-two pattern length: Apparently, there is
 *                  no way to set the length of the pattern -- it is always
 *                  assumed to be 8 or 32 (or 1024?).
 *   ScreenToScreenColorExpandFill: See p. 4-17 of the Technical Reference
 *                  Manual where it states that monochrome expansion of frame
 *                  buffer data is not supported.
 *   CPUToScreenColorExpandFill, direct: The implementation here uses a hybrid
 *                  direct/indirect method.  If we had more data registers,
 *                  then we could do better.  If XAA supported a trigger write
 *                  address, the code would be simpler.
 *   Color8x8PatternFill: Apparently, an 8x8 color brush cannot take an 8x8
 *                  pattern from frame buffer memory.
 *   ImageWrites:   The direct method isn't supported because XAA does not
 *                  support a final trigger register write.  The indirect
 *                  method slows down common operations.  Perhaps additional
 *                  XAA flags to use this only for some operations would help.
 *
 */

#define R128_CLIPPING   1
#define R128_IMAGEWRITE 0	/* Indirect image write is slow        */
#define R128_TRAPEZOIDS 0	/* Trapezoids don't work               */

				/* X and server generic header files */
#include "Xarch.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "xf86fbman.h"

				/* Line support */
#include "miline.h"

				/* XAA and cursor support */
#include "xaa.h"
#include "xf86Cursor.h"

				/* PCI support */
#include "xf86PciInfo.h"
#include "xf86Pci.h"

				/* DDC support */
#include "xf86DDC.h"

				/* Driver data structures */
#include "r128.h"
#include "r128_reg.h"

static struct {
    int rop;
    int pattern;
} R128_ROP[] = {
    { R128_ROP3_ZERO, R128_ROP3_ZERO }, /* GXclear        */
    { R128_ROP3_DSa,  R128_ROP3_DPa  }, /* Gxand          */
    { R128_ROP3_SDna, R128_ROP3_PDna }, /* GXandReverse   */
    { R128_ROP3_S,    R128_ROP3_P    }, /* GXcopy         */
    { R128_ROP3_DSna, R128_ROP3_DPna }, /* GXandInverted  */
    { R128_ROP3_D,    R128_ROP3_D    }, /* GXnoop         */
    { R128_ROP3_DSx,  R128_ROP3_DPx  }, /* GXxor          */
    { R128_ROP3_DSo,  R128_ROP3_DPo  }, /* GXor           */
    { R128_ROP3_DSon, R128_ROP3_DPon }, /* GXnor          */
    { R128_ROP3_DSxn, R128_ROP3_PDxn }, /* GXequiv        */
    { R128_ROP3_Dn,   R128_ROP3_Dn   }, /* GXinvert       */
    { R128_ROP3_SDno, R128_ROP3_PDno }, /* GXorReverse    */
    { R128_ROP3_Sn,   R128_ROP3_Pn   }, /* GXcopyInverted */
    { R128_ROP3_DSno, R128_ROP3_DPno }, /* GXorInverted   */
    { R128_ROP3_DSan, R128_ROP3_DPan }, /* GXnand         */
    { R128_ROP3_ONE,  R128_ROP3_ONE  }  /* GXset          */
};

/* Flush all dirty data in the Pixel Cache to memory. */
static void R128EngineFlush(ScrnInfoPtr pScrn)
{
    int i;
    R128MMIO_VARS();
    
    OUTREGP(R128_PC_NGUI_CTLSTAT, R128_PC_FLUSH_ALL, ~R128_PC_FLUSH_ALL);
    for (i = 0; i < R128_TIMEOUT; i++) {
	if (!(INREG(R128_PC_NGUI_CTLSTAT) & R128_PC_BUSY)) break;
    }
}

/* Reset graphics card to known state. */
static void R128EngineReset(ScrnInfoPtr pScrn)
{
    CARD32 clock_cntl_index;
    CARD32 mclk_cntl;
    CARD32 gen_reset_cntl;
    R128MMIO_VARS();
	
    R128EngineFlush(pScrn);
    
    clock_cntl_index = INREG(R128_CLOCK_CNTL_INDEX);
    mclk_cntl        = INPLL(pScrn, R128_MCLK_CNTL);

    OUTPLL(R128_MCLK_CNTL, mclk_cntl | R128_FORCE_GCP | R128_FORCE_PIPE3D_CPP);

    gen_reset_cntl   = INREG(R128_GEN_RESET_CNTL);

    OUTREG(R128_GEN_RESET_CNTL, gen_reset_cntl | R128_SOFT_RESET_GUI);
    INREG(R128_GEN_RESET_CNTL);
    OUTREG(R128_GEN_RESET_CNTL, gen_reset_cntl & ~R128_SOFT_RESET_GUI);
    INREG(R128_GEN_RESET_CNTL);

    OUTPLL(R128_MCLK_CNTL,        mclk_cntl);
    OUTREG(R128_CLOCK_CNTL_INDEX, clock_cntl_index);
    OUTREG(R128_GEN_RESET_CNTL,   gen_reset_cntl);
}

#define R128WaitForFifo(pScrn, entries)                                      \
do {                                                                         \
    if (info->fifo_slots < entries) R128WaitForFifoFunction(pScrn, entries); \
    info->fifo_slots -= entries;                                             \
} while (0)

/* The FIFO has 64 slots.  This routines waits until at least `entries' of
   these slots are empty. */
static void R128WaitForFifoFunction(ScrnInfoPtr pScrn, int entries)
{
    R128InfoPtr info = R128PTR(pScrn);
    int         i;
    R128MMIO_VARS();

    for (;;) {
	for (i = 0; i < R128_TIMEOUT; i++) {
	    info->fifo_slots = INREG(R128_GUI_STAT) & R128_GUI_FIFOCNT_MASK;
	    if (info->fifo_slots >= entries) return;
	}
	R128TRACE(("FIFO timed out: %d entries, stat=0x%08x, probe=0x%08x\n",
		   INREG(R128_GUI_STAT) & R128_GUI_FIFOCNT_MASK,
		   INREG(R128_GUI_STAT),
		   INREG(R128_GUI_PROBE)));
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "FIFO timed out, resetting engine...\n");
	R128EngineReset(pScrn);
    }
}

/* Wait for the graphics engine to be completely idle: the FIFO has
   drained, the Pixel Cache is flushed, and the engine is idle.  This is a
   standard "sync" function that will make the hardware "quiescent". */
static void R128WaitForIdle(ScrnInfoPtr pScrn)
{
    int i;
    R128MMIO_VARS();

    R128WaitForFifoFunction(pScrn, 64);

    for (;;) {
	for (i = 0; i < R128_TIMEOUT; i++) {
	    if (!(INREG(R128_GUI_STAT) & R128_GUI_ACTIVE)) {
		R128EngineFlush(pScrn);
		return;
	    }
	}
	R128TRACE(("Idle timed out: %d entries, stat=0x%08x, probe=0x%08x\n",
		   INREG(R128_GUI_STAT) & R128_GUI_FIFOCNT_MASK,
		   INREG(R128_GUI_STAT),
		   INREG(R128_GUI_PROBE)));
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Idle timed out, resetting engine...\n");
	R128EngineReset(pScrn);
    }
}

/* Setup for XAA SolidFill. */
static void R128SetupForSolidFill(ScrnInfoPtr pScrn,
				  int color, int rop, unsigned int planemask)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    R128WaitForFifo(pScrn, 4);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_SOLID_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].pattern));
    OUTREG(R128_DP_BRUSH_FRGD_CLR,  color);
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_CNTL,            (R128_DST_X_LEFT_TO_RIGHT
				     | R128_DST_Y_TOP_TO_BOTTOM));
}

/* Subsequent XAA SolidFillRect.

   Tests: xtest CH06/fllrctngl, xterm
*/
static void  R128SubsequentSolidFillRect(ScrnInfoPtr pScrn,
					 int x, int y, int w, int h)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DST_Y_X,          (y << 16) | x);
    OUTREG(R128_DST_WIDTH_HEIGHT, (w << 16) | h);
}

/* Setup for XAA solid lines. */
static void R128SetupForSolidLine(ScrnInfoPtr pScrn, 
				  int color, int rop, unsigned int planemask)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_SOLID_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].pattern));
    OUTREG(R128_DP_BRUSH_FRGD_CLR,  color);
    OUTREG(R128_DP_WRITE_MASK,      planemask);
}


/* Subsequent XAA solid Bresenham line.

   Tests: xtest CH06/drwln, ico, Mark Vojkovich's linetest program
   
   [See http://www.xfree86.org/devel/archives/devel/1999-Jun/0102.shtml for
   Mark Vojkovich's linetest program, posted 2Jun99 to devel@xfree86.org.]

   x11perf -line500
                               1024x768@76Hz   1024x768@76Hz
			                8bpp           32bpp
   not used:                     39700.0/sec     34100.0/sec
   used:                         47600.0/sec     36800.0/sec
*/
static void R128SubsequentSolidBresenhamLine(ScrnInfoPtr pScrn,
					     int x, int y,
					     int major, int minor,
					     int err, int len, int octant)
{
    R128InfoPtr info  = R128PTR(pScrn);
    int         flags = 0;
    R128MMIO_VARS();

    if (octant & YMAJOR)         flags |= R128_DST_Y_MAJOR;
    if (!(octant & XDECREASING)) flags |= R128_DST_X_DIR_LEFT_TO_RIGHT;
    if (!(octant & YDECREASING)) flags |= R128_DST_Y_DIR_TOP_TO_BOTTOM;
			      
    R128WaitForFifo(pScrn, 6);
    OUTREG(R128_DP_CNTL_XDIR_YDIR_YMAJOR, flags);
    OUTREG(R128_DST_Y_X,                  (y << 16) | x);
    OUTREG(R128_DST_BRES_ERR,             err);
    OUTREG(R128_DST_BRES_INC,             minor);
    OUTREG(R128_DST_BRES_DEC,             -major);
    OUTREG(R128_DST_BRES_LNTH,            len);
}

/* Subsequent XAA solid horizontal and vertical lines

   1024x768@76Hz 8bpp
                             Without             With
   x11perf -hseg500      87600.0/sec     798000.0/sec
   x11perf -vseg500      38100.0/sec      38000.0/sec
*/
static void R128SubsequentSolidHorVertLine(ScrnInfoPtr pScrn,
					   int x, int y, int len, int dir )
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    R128WaitForFifo(pScrn, 1);
    OUTREG(R128_DP_CNTL, (R128_DST_X_LEFT_TO_RIGHT
			  | R128_DST_Y_TOP_TO_BOTTOM));

    if (dir == DEGREES_0) {
	R128SubsequentSolidFillRect(pScrn, x, y, len, 1);
    } else {
	R128SubsequentSolidFillRect(pScrn, x, y, 1, len);
    }
}

/* Setup for XAA dashed lines.

   Tests: xtest CH05/stdshs, XFree86/drwln

   NOTE: Since we can only accelerate lines with power-of-2 patterns of
   length <= 32, these x11perf numbers are not representative of the
   speed-up on appropriately-sized patterns.
   
   1024x768@76Hz 8bpp
                             Without             With
   x11perf -dseg100     218000.0/sec     222000.0/sec
   x11perf -dline100    215000.0/sec     221000.0/sec
   x11perf -ddline100   178000.0/sec     180000.0/sec
*/
static void R128SetupForDashedLine(ScrnInfoPtr pScrn,
				   int fg, int bg,
				   int rop, unsigned int planemask,
				   int length, unsigned char *pattern)
{
    R128InfoPtr info = R128PTR(pScrn);
    CARD32      pat  = *(CARD32 *)pattern;
    R128MMIO_VARS();

    switch (length) {
    case  2: pat |= pat <<  2;	/* fall through */
    case  4: pat |= pat <<  4;	/* fall through */
    case  8: pat |= pat <<  8;	/* fall through */
    case 16: pat |= pat << 16;
    }
    
    R128WaitForFifo(pScrn, 5);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | (bg == -1
					? R128_GMC_BRUSH_32x1_MONO_FG_LA
					: R128_GMC_BRUSH_32x1_MONO_FG_BG)
				     | R128_ROP[rop].pattern
				     | R128_GMC_BYTE_LSB_TO_MSB));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_BRUSH_FRGD_CLR,  fg);
    OUTREG(R128_DP_BRUSH_BKGD_CLR,  bg);
    OUTREG(R128_BRUSH_DATA0,        pat);
}

/* Subsequent XAA dashed line. */
static void R128SubsequentDashedBresenhamLine(ScrnInfoPtr pScrn,
					      int x1, int y1,
					      int major, int minor,
					      int err, int len, int octant,
					      int phase)
{
    R128InfoPtr info  = R128PTR(pScrn);
    int         flags = 0;
    R128MMIO_VARS();

    if (octant & YMAJOR)         flags |= R128_DST_Y_MAJOR;
    if (!(octant & XDECREASING)) flags |= R128_DST_X_DIR_LEFT_TO_RIGHT;
    if (!(octant & YDECREASING)) flags |= R128_DST_Y_DIR_TOP_TO_BOTTOM;

    R128WaitForFifo(pScrn, 7);
    OUTREG(R128_DP_CNTL_XDIR_YDIR_YMAJOR, flags);
    OUTREG(R128_DST_Y_X,                  (y1 << 16) | x1);
    OUTREG(R128_BRUSH_Y_X,                (phase << 16) | phase);
    OUTREG(R128_DST_BRES_ERR,             err);
    OUTREG(R128_DST_BRES_INC,             minor);
    OUTREG(R128_DST_BRES_DEC,             -major);
    OUTREG(R128_DST_BRES_LNTH,            len);
}

#if R128_TRAPEZOIDS
				/* This doesn't work.  Except in the
                                   lower-left quadrant, all of the pixel
                                   errors appear to be because eL and eR
                                   are not correct.  Drawing from right to
                                   left doesn't help.  Be aware that the
                                   non-_SUB registers set the sub-pixel
                                   values to 0.5 (0x08), which isn't what
                                   XAA wants. */
/* Subsequent XAA SolidFillTrap.  XAA always passes data that assumes we
   fill from top to bottom, so dyL and dyR are always non-negative. */
static void R128SubsequentSolidFillTrap(ScrnInfoPtr pScrn, int y, int h, 
                                        int left, int dxL, int dyL, int eL,
                                        int right, int dxR, int dyR, int eR)
{
    int flags   = 0;
    int Lymajor = 0;
    int Rymajor = 0;
    int origdxL = dxL;
    int origdxR = dxR;
    R128MMIO_VARS();

    R128TRACE(("Trap %d %d; L %d %d %d %d; R %d %d %d %d\n",
               y, h,
               left, dxL, dyL, eL,
               right, dxR, dyR, eR));

    if (dxL < 0)    dxL = -dxL; else flags |= (1 << 0) /* | (1 << 8) */;
    if (dxR < 0)    dxR = -dxR; else flags |= (1 << 6);

    R128WaitForFifo(pScrn, 11);

#if 1
    OUTREG(R128_DP_CNTL,            flags | (1 << 1) | (1 << 7));
    OUTREG(R128_DST_Y_SUB,          ((y) << 4) | 0x0 );
    OUTREG(R128_DST_X_SUB,          ((left) << 4)|0x0);
    OUTREG(R128_TRAIL_BRES_ERR,     eR-dxR);
    OUTREG(R128_TRAIL_BRES_INC,     dxR);
    OUTREG(R128_TRAIL_BRES_DEC,     -dyR);
    OUTREG(R128_TRAIL_X_SUB,        ((right) << 4) | 0x0);
    OUTREG(R128_LEAD_BRES_ERR,      eL-dxL);
    OUTREG(R128_LEAD_BRES_INC,      dxL);
    OUTREG(R128_LEAD_BRES_DEC,      -dyL);
    OUTREG(R128_LEAD_BRES_LNTH_SUB, ((h) << 4) | 0x00);
#else
    OUTREG(R128_DP_CNTL,            flags | (1 << 1) );
    OUTREG(R128_DST_Y_SUB,          (y << 4));
    OUTREG(R128_DST_X_SUB,          (right << 4));
    OUTREG(R128_TRAIL_BRES_ERR,     eL);
    OUTREG(R128_TRAIL_BRES_INC,     dxL);
    OUTREG(R128_TRAIL_BRES_DEC,     -dyL);
    OUTREG(R128_TRAIL_X_SUB,        (left << 4) | 0);
    OUTREG(R128_LEAD_BRES_ERR,      eR);
    OUTREG(R128_LEAD_BRES_INC,      dxR);
    OUTREG(R128_LEAD_BRES_DEC,      -dyR);
    OUTREG(R128_LEAD_BRES_LNTH_SUB, h << 4);
#endif
}
#endif

#if R128_CLIPPING
/* Setup for XAA clipping rectangle.

   Tests: xtest CH06/drwrctngl

   These x11perf data show why we don't use clipping for lines.  Clipping
   can improve performance for other functions.
   
   1024x768@76 8bpp
                             Without           With
   x11perf -seg100c1    241000.0/sec   185000.0/sec
   x11perf -seg100c2    238000.0/sec   154000.0/sec
   x11perf -seg100c3    194000.0/sec   132000.0/sec

*/
static void R128SetClippingRectangle(ScrnInfoPtr pScrn,
				     int left, int top, int right, int bottom)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    if (left > 8191 || top > 8191 || right > 8191 || bottom > 8191
	|| left < 0 || top < 0 || right < 0 || bottom < 0
	|| left > 4000 || right > 4000 || top > 4000 || bottom > 4000
	|| left >= right || top >= bottom)
	R128TRACE(("Clip %d %d %d %d *************************************\n",
		   left, top, right, bottom));

    R128WaitForFifo(pScrn,       2);
    OUTREG(R128_SC_TOP_LEFT,     (top << 16)    | left);
    OUTREG(R128_SC_BOTTOM_RIGHT, (bottom << 16) | right);
}

static void R128DisableClipping (ScrnInfoPtr pScrn)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();
    
    R128WaitForFifo(pScrn,       2);
    OUTREG(R128_SC_TOP_LEFT,     0);
    OUTREG(R128_SC_BOTTOM_RIGHT, (R128_DEFAULT_SC_RIGHT_MAX
				  | R128_DEFAULT_SC_BOTTOM_MAX));
}
#endif


/* Setup for XAA screen-to-screen copy.

   Tests: xtest CH06/fllrctngl (also tests transparency).
*/
static void R128SetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
					   int xdir, int ydir, int rop,
					   unsigned int planemask,
					   int trans_color)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    info->xdir = xdir;
    info->ydir = ydir;
    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_SOLID_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].rop
				     | R128_DP_SRC_SOURCE_MEMORY));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_CNTL,            ((xdir >= 0 ? R128_DST_X_LEFT_TO_RIGHT : 0)
				     | (ydir >= 0
					? R128_DST_Y_TOP_TO_BOTTOM
					: 0)));

    if (trans_color != -1) {
				/* Set up for transparency */
	R128WaitForFifo(pScrn, 3);
	OUTREG(R128_CLR_CMP_CLR_SRC, trans_color);
	OUTREG(R128_CLR_CMP_MASK,    R128_CLR_CMP_MSK);
	OUTREG(R128_CLR_CMP_CNTL,    (R128_SRC_CMP_NEQ_COLOR
				      | R128_CLR_CMP_SRC_SOURCE));
    }
}

/* Subsequent XAA screen-to-screen copy. */
static void R128SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
					     int x1, int y1,
					     int x2, int y2,
					     int w, int h)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    if (info->xdir < 0) x1 += w - 1, x2 += w - 1;
    if (info->ydir < 0) y1 += h - 1, y2 += h - 1;
    
    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_SRC_Y_X,          (y1 << 16) | x1);
    OUTREG(R128_DST_Y_X,          (y2 << 16) | x2);
    OUTREG(R128_DST_HEIGHT_WIDTH, (h << 16) | w);
}

/* Setup for XAA mono 8x8 pattern color expansion.  Patterns with
   transparency use `bg == -1'.  This routine is only used if the XAA
   pixmap cache is turned on.

   Tests: xtest XFree86/fllrctngl (no other test will test this routine with
                                   both transparency and non-transparency)

   1024x768@76Hz 8bpp
                             Without             With
   x11perf -srect100     38600.0/sec      85700.0/sec
   x11perf -osrect100    38600.0/sec      85700.0/sec
*/
static void R128SetupForMono8x8PatternFill(ScrnInfoPtr pScrn,
					   int patternx, int patterny,
					   int fg, int bg, int rop,
					   unsigned int planemask)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    R128WaitForFifo(pScrn, 6);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | (bg == -1
					? R128_GMC_BRUSH_8X8_MONO_FG_LA
					: R128_GMC_BRUSH_8X8_MONO_FG_BG)
				     | R128_ROP[rop].pattern
				     | R128_GMC_BYTE_LSB_TO_MSB));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_BRUSH_FRGD_CLR,  fg);
    OUTREG(R128_DP_BRUSH_BKGD_CLR,  bg);
    OUTREG(R128_BRUSH_DATA0,        patternx);
    OUTREG(R128_BRUSH_DATA1,        patterny);
}

/* Subsequent XAA 8x8 pattern color expansion.  Because they are used in
   the setup function, `patternx' and `patterny' are not used here. */
static void R128SubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn,
						 int patternx, int patterny,
						 int x, int y, int w, int h)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_BRUSH_Y_X,        (patterny << 8) | patternx);
    OUTREG(R128_DST_Y_X,          (y << 16) | x);
    OUTREG(R128_DST_HEIGHT_WIDTH, (h << 16) | w);
}

#if 0
/* Setup for XAA color 8x8 pattern fill.

   Tests: xtest XFree86/fllrctngl (with Mono8x8PatternFill off)
*/
static void R128SetupForColor8x8PatternFill(ScrnInfoPtr pScrn,
					    int patx, int paty,
					    int rop, unsigned int planemask,
					    int trans_color)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    R128TRACE(("Color8x8 %d %d %d\n", trans_color, patx, paty));
    
    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_8x8_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].rop
				     | R128_DP_SRC_SOURCE_MEMORY));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    
    if (trans_color != -1) {
				/* Set up for transparency */
	R128WaitForFifo(pScrn, 3);
	OUTREG(R128_CLR_CMP_CLR_SRC, trans_color);
	OUTREG(R128_CLR_CMP_MASK,    R128_CLR_CMP_MSK);
	OUTREG(R128_CLR_CMP_CNTL,    (R128_SRC_CMP_NEQ_COLOR
				      | R128_CLR_CMP_SRC_SOURCE));
    }
}

/* Subsequent XAA 8x8 pattern color expansion. */
static void R128SubsequentColor8x8PatternFillRect( ScrnInfoPtr pScrn,
						   int patx, int paty,
						   int x, int y, int w, int h)
{
    R128MMIO_VARS();

    R128TRACE(("Color8x8 %d,%d %d,%d %d %d\n", patx, paty, x, y, w, h));
    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_SRC_Y_X, (paty << 16) | patx);
    OUTREG(R128_DST_Y_X, (y << 16) | x);
    OUTREG(R128_DST_HEIGHT_WIDTH, (h << 16) | w);
}
#endif 

/* Setup for XAA indirect CPU-to-screen color expansion (indirect).
   Because of how the scratch buffer is initialized, this is really a
   mainstore-to-screen color expansion.  Transparency is supported when `bg
   == -1'.

   x11perf -ftext (pure indirect):
                               1024x768@76Hz   1024x768@76Hz
			                8bpp           32bpp
   not used:                    685000.0/sec    794000.0/sec
   used:                       1070000.0/sec   1080000.0/sec

   We could improve this indirect routine by about 10% if the hardware
   could accept DWORD padded scanlines, or if XAA could provide bit-packed
   data.  We might also be able to move to a direct routine if there were
   more HOST_DATA registers.

   Implementing the hybrid indirect/direct scheme improved performance in a
   few areas:

   1024x768@76 8bpp
                                   Indirect          Hybrid
   x11perf -oddsrect10          50100.0/sec     71700.0/sec   
   x11perf -oddsrect100          4240.0/sec      6660.0/sec
   x11perf -bigsrect10          50300.0/sec     71100.0/sec
   x11perf -bigsrect100          4190.0/sec      6800.0/sec
   x11perf -polytext           584000.0/sec    714000.0/sec
   x11perf -polytext16         154000.0/sec    172000.0/sec
   x11perf -seg1              1780000.0/sec   1880000.0/sec
   x11perf -copyplane10         42900.0/sec     58300.0/sec
   x11perf -copyplane100         4400.0/sec      6710.0/sec
   x11perf -putimagexy10         5090.0/sec      6670.0/sec
   x11perf -putimagexy100         424.0/sec       575.0/sec
   
   1024x768@76 -depth 24 -fbbpp 32
                                   Indirect          Hybrid
   x11perf -oddsrect100          4240.0/sec      6670.0/sec
   x11perf -bigsrect100          4190.0/sec      6800.0/sec
   x11perf -polytext           585000.0/sec    719000.0/sec
   x11perf -seg1              2960000.0/sec   2990000.0/sec
   x11perf -copyplane100         4400.0/sec      6700.0/sec
   x11perf -putimagexy100         138.0/sec       191.0/sec

*/
static void R128SetupForScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
							   int fg, int bg,
							   int rop,
							   unsigned int
							   planemask)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    R128WaitForFifo(pScrn, 4);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_1X8_COLOR
				     | (bg == -1
					? R128_GMC_SRC_DATATYPE_MONO_FG_LA
					: R128_GMC_SRC_DATATYPE_MONO_FG_BG)
				     | R128_ROP[rop].rop
				     | R128_GMC_BYTE_LSB_TO_MSB
				     | R128_DP_SRC_SOURCE_HOST_DATA));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_SRC_FRGD_CLR,    fg);
    OUTREG(R128_DP_SRC_BKGD_CLR,    bg);
}

/* Subsequent XAA indirect CPU-to-screen color expansion.  This is only
   called once for each rectangle. */
static void R128SubsequentScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
							     int x, int y,
							     int w, int h,
							     int skipleft)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    info->scanline_y      = y;
    info->scanline_x      = x;
    info->scanline_h      = h;
    info->scanline_h_w    = (1 << 16) | w;
    info->scanline_words  = (w + 31) / 32;
    info->scanline_direct = 0;

    if (info->scanline_words <= 9 && info->scanline_h > 1) {
				/* Turn on direct for next set of scan lines */
	info->scratch_buffer[0]
	    = (unsigned char *)(ADDRREG(R128_HOST_DATA_LAST)
				- (info->scanline_words - 1));
	info->scanline_direct = 1;

				/* Make engine ready for next line */
	R128WaitForFifo(pScrn, 2);
	OUTREG(R128_DST_Y_X,            ((info->scanline_y++ << 16)
					 | info->scanline_x));
	OUTREG(R128_DST_HEIGHT_WIDTH,   info->scanline_h_w);
    }
}

/* Subsequent XAA indirect CPU-to-screen color expandion.  This is called
   once for each scanline. */
static void R128SubsequentColorExpandScanline(ScrnInfoPtr pScrn, int bufno)
{
    R128InfoPtr     info = R128PTR(pScrn);
    CARD32          *p   = (CARD32 *)info->scratch_buffer[bufno];
    int             i;
    int             left = info->scanline_words;
    volatile CARD32 *d;
    R128MMIO_VARS();

    --info->scanline_h;
    if (info->scanline_direct) {
	if (info->scanline_h <= 1) {
				/* Turn off direct for last scan line */
	    info->scratch_buffer[0] = info->scratch_save;
	    info->scanline_direct   = 0;
	    return;
	}
    }
    
    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DST_Y_X,            ((info->scanline_y++ << 16)
                                     | info->scanline_x));
    OUTREG(R128_DST_HEIGHT_WIDTH,   info->scanline_h_w);

    if (info->scanline_direct) return;
    
    while (left) {
	if (left <= 9) {
	    R128WaitForFifo(pScrn, left);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA_LAST) - (left - 1); left; --left)
		*d++ = *p++;
	} else {
	    R128WaitForFifo(pScrn, 8);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA0), i = 0; i < 8; i++)
		*d++ = *p++;
	    left -= 8;
	}
    }
}

#if R128_IMAGEWRITE
/* Setup for XAA indirect image write.


   1024x768@76Hz 8bpp
                             Without             With
   x11perf -putimage10   37500.0/sec      39300.0/sec
   x11perf -putimage100   2150.0/sec       1170.0/sec
   x11perf -putimage500    108.0/sec         49.8/sec
 */
static void R128SetupForScanlineImageWrite(ScrnInfoPtr pScrn,
					   int rop,
					   unsigned int planemask,
					   int trans_color,
					   int bpp,
					   int depth)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    info->scanline_bpp = bpp;

    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_SOLID_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].rop
				     | R128_GMC_BYTE_LSB_TO_MSB
				     | R128_DP_SRC_SOURCE_HOST_DATA));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    
    if (trans_color != -1) {
				/* Set up for transparency */
	R128WaitForFifo(pScrn, 3);
	OUTREG(R128_CLR_CMP_CLR_SRC, trans_color);
	OUTREG(R128_CLR_CMP_MASK,    R128_CLR_CMP_MSK);
	OUTREG(R128_CLR_CMP_CNTL,    (R128_SRC_CMP_NEQ_COLOR
				      | R128_CLR_CMP_SRC_SOURCE));
    }
}

/* Subsequent XAA indirect image write. This is only called once for each
   rectangle. */
static void R128SubsequentScanlineImageWriteRect(ScrnInfoPtr pScrn,
						 int x, int y,
						 int w, int h,
						 int skipleft)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    info->scanline_y      = y;
    info->scanline_x      = x;
    info->scanline_h      = h;
    info->scanline_h_w    = (1 << 16) | w;
    info->scanline_words  = (w * info->scanline_bpp + 31) / 32;
    info->scanline_direct = 0;

    if (info->scanline_words <= 9 && info->scanline_h > 1) {
				/* Turn on direct for next set of scan lines */
	info->scratch_buffer[0]
	    = (unsigned char *)(ADDRREG(R128_HOST_DATA_LAST)
				- (info->scanline_words - 1));
	info->scanline_direct = 1;

				/* Make engine ready for next line */
	R128WaitForFifo(pScrn, 2);
	OUTREG(R128_DST_Y_X,            ((info->scanline_y++ << 16)
					 | info->scanline_x));
	OUTREG(R128_DST_HEIGHT_WIDTH,   info->scanline_h_w);
    }
}

/* Subsequent XAA indirect iamge write.  This is called once for each
   scanline. */
static void R128SubsequentImageWriteScanline(ScrnInfoPtr pScrn, int bufno)
{
    R128InfoPtr     info = R128PTR(pScrn);
    CARD32          *p   = (CARD32 *)info->scratch_buffer[bufno];
    int             i;
    int             left = info->scanline_words;
    volatile CARD32 *d;
    R128MMIO_VARS();

    --info->scanline_h;
    if (info->scanline_direct) {
	if (info->scanline_h <= 1) {
				/* Turn off direct for last scan line */
	    info->scratch_buffer[0] = info->scratch_save;
	    info->scanline_direct   = 0;
	    return;
	}
    }
    
    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DST_Y_X,            ((info->scanline_y++ << 16)
                                     | info->scanline_x));
    OUTREG(R128_DST_HEIGHT_WIDTH,   info->scanline_h_w);

    if (info->scanline_direct) return;
    
    while (left) {
	if (left <= 9) {
	    R128WaitForFifo(pScrn, left);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA_LAST) - (left - 1); left; --left)
		*d++ = *p++;
	} else {
	    R128WaitForFifo(pScrn, 8);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA0), i = 0; i < 8; i++)
		*d++ = *p++;
	    left -= 8;
	}
    }
}
#endif

/* Initialize the acceleration hardware. */
void R128EngineInit(ScrnInfoPtr pScrn)
{
    R128InfoPtr info = R128PTR(pScrn);
    R128MMIO_VARS();

    R128TRACE(("EngineInit (%d/%d)\n", info->pixel_code, pScrn->bitsPerPixel));

    OUTREG(R128_SCALE_3D_CNTL, 0);
    R128EngineReset(pScrn);

    switch (info->pixel_code) {
    case 8:  info->datatype = 2; break;
    case 15: info->datatype = 3; break;
    case 16: info->datatype = 4; break;
    case 24: info->datatype = 5; break;
    case 32: info->datatype = 6; break;
    default:
	R128TRACE(("Unknown depth/bpp = %d/%d (code = %d)\n",
		   pScrn->depth, pScrn->bitsPerPixel, info->pixel_code));
    }
    info->pitch = (pScrn->displayWidth / 8) * (info->pixel_bytes == 3 ? 3 : 1);

    R128TRACE(("Pitch for acceleration = %d\n", info->pitch));

    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DEFAULT_OFFSET, 0);
    OUTREG(R128_DEFAULT_PITCH,  info->pitch);

    R128WaitForFifo(pScrn, 4);
    OUTREG(R128_AUX_SC_CNTL,             0);
    OUTREG(R128_DEFAULT_SC_BOTTOM_RIGHT, (R128_DEFAULT_SC_RIGHT_MAX
					  | R128_DEFAULT_SC_BOTTOM_MAX));
    OUTREG(R128_SC_TOP_LEFT,             0);
    OUTREG(R128_SC_BOTTOM_RIGHT,         (R128_DEFAULT_SC_RIGHT_MAX
				         | R128_DEFAULT_SC_BOTTOM_MAX));

    info->dp_gui_master_cntl = ((info->datatype << R128_GMC_DST_DATATYPE_SHIFT)
				| R128_GMC_CLR_CMP_CNTL_DIS
				| R128_AUX_CLIP_DIS
				| R128_GMC_DST_CLIPPING);
    R128WaitForFifo(pScrn, 1);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_SOLID_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR));

    R128WaitForFifo(pScrn, 8);
    OUTREG(R128_DST_BRES_ERR,      0);
    OUTREG(R128_DST_BRES_INC,      0);
    OUTREG(R128_DST_BRES_DEC,      0);
    OUTREG(R128_DP_BRUSH_FRGD_CLR, 0xffffffff);
    OUTREG(R128_DP_BRUSH_BKGD_CLR, 0x00000000);
    OUTREG(R128_DP_SRC_FRGD_CLR,   0xffffffff);
    OUTREG(R128_DP_SRC_BKGD_CLR,   0x00000000);
    OUTREG(R128_DP_WRITE_MASK,     0xffffffff);

    R128WaitForIdle(pScrn);
}

/* Initialize XAA for supported acceleration and also initialize the
   graphics hardware for acceleration. */
Bool R128AccelInit(ScreenPtr pScreen)
{
    ScrnInfoPtr   pScrn = xf86Screens[pScreen->myNum];
    R128InfoPtr   info  = R128PTR(pScrn);
    XAAInfoRecPtr a;

    if (!(a = info->accel = XAACreateInfoRec())) return FALSE;
    
    a->Flags                            = (PIXMAP_CACHE
					   | OFFSCREEN_PIXMAPS
					   | LINEAR_FRAMEBUFFER);

				/* Sync */
    a->Sync                             = R128WaitForIdle;

				/* Solid Filled Rectangle */
    a->PolyFillRectSolidFlags           = 0;
    a->SetupForSolidFill                = R128SetupForSolidFill;
    a->SubsequentSolidFillRect          = R128SubsequentSolidFillRect;

				/* Screen-to-screen Copy */
    a->ScreenToScreenCopyFlags          = 0;
    a->SetupForScreenToScreenCopy       = R128SetupForScreenToScreenCopy;
    a->SubsequentScreenToScreenCopy     = R128SubsequentScreenToScreenCopy;

				/* Mono 8x8 Pattern Fill (Color Expand) */
    a->SetupForMono8x8PatternFill       = R128SetupForMono8x8PatternFill;
    a->SubsequentMono8x8PatternFillRect = R128SubsequentMono8x8PatternFillRect;
    a->Mono8x8PatternFillFlags          = (HARDWARE_PATTERN_PROGRAMMED_BITS
					   | HARDWARE_PATTERN_PROGRAMMED_ORIGIN
					   | HARDWARE_PATTERN_SCREEN_ORIGIN
					   | BIT_ORDER_IN_BYTE_LSBFIRST);

				/* Indirect CPU-To-Screen Color Expand */
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
    a->ScanlineCPUToScreenColorExpandFillFlags = 0;
#else
    a->ScanlineCPUToScreenColorExpandFillFlags = BIT_ORDER_IN_BYTE_MSBFIRST;
#endif
    a->NumScanlineColorExpandBuffers   = 1;
    a->ScanlineColorExpandBuffers      = info->scratch_buffer;
#if R128_IMAGEWRITE
    info->scratch_save                 = xalloc(((pScrn->virtualX+31)/32*4)
						+ (pScrn->virtualX
						   * info->pixel_bytes));
#else
    info->scratch_save                 = xalloc(((pScrn->virtualX+31)/32*4));
#endif
    info->scratch_buffer[0]            = info->scratch_save;
    a->SetupForScanlineCPUToScreenColorExpandFill
	= R128SetupForScanlineCPUToScreenColorExpandFill;
    a->SubsequentScanlineCPUToScreenColorExpandFill
	= R128SubsequentScanlineCPUToScreenColorExpandFill;
    a->SubsequentColorExpandScanline   = R128SubsequentColorExpandScanline;

				/* Bresenham Solid Lines */
    a->SetupForSolidLine               = R128SetupForSolidLine;
    a->SubsequentSolidBresenhamLine    = R128SubsequentSolidBresenhamLine;
    a->SubsequentSolidHorVertLine      = R128SubsequentSolidHorVertLine;

				/* Bresenham Dashed Lines*/
    a->SetupForDashedLine              = R128SetupForDashedLine;
    a->SubsequentDashedBresenhamLine   = R128SubsequentDashedBresenhamLine;
    a->DashPatternMaxLength            = 32;
    a->DashedLineFlags                 = (LINE_PATTERN_LSBFIRST_LSBJUSTIFIED
					  | LINE_PATTERN_POWER_OF_2_ONLY);
#if R128_CLIPPING
				/* Clipping. */
    if (pScrn->depth != 8 && info->pixel_code != 24) {
				/* There is one xtest error in 8bpp and
                                   many xtest errors in 24/24 that do not
                                   appear at other depths. */
	a->SetClippingRectangle            = R128SetClippingRectangle;
	a->DisableClipping                 = R128DisableClipping;
	a->ClippingFlags
	    = (HARDWARE_CLIP_SCREEN_TO_SCREEN_COPY
	       | HARDWARE_CLIP_MONO_8x8_FILL
	       | HARDWARE_CLIP_COLOR_8x8_FILL
	       | HARDWARE_CLIP_SOLID_FILL);
    }
#endif

#if R128_IMAGEWRITE
				/* ImageWrite */
    a->NumScanlineImageWriteBuffers    = 1;
    a->ScanlineImageWriteBuffers       = info->scratch_buffer;
    info->scratch_buffer[0]            = info->scratch_save;
    a->SetupForScanlineImageWrite      = R128SetupForScanlineImageWrite;
    a->SubsequentScanlineImageWriteRect= R128SubsequentScanlineImageWriteRect;
    a->SubsequentImageWriteScanline    = R128SubsequentImageWriteScanline;
    a->ImageWriteFlags                 = (CPU_TRANSFER_PAD_DWORD
					  | SCANLINE_PAD_DWORD
					  | SYNC_AFTER_IMAGE_WRITE);
#endif
    
    R128EngineInit(pScrn);
    return XAAInit(pScreen, a);
}
