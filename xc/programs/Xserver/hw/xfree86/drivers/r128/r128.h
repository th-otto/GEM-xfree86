/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/r128/r128.h,v 1.8 2000/02/23 04:47:18 martin Exp $ */
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
 */

#ifndef _R128_H_
#define _R128_H_

#define R128_DEBUG    0		/* Turn off debugging output                */
#define R128_TIMEOUT  2000000	/* Fall out of wait loops after this count */
#define R128_MMIOSIZE 0x80000

#define R128_NAME "r128"
#define R128_VERSION_MAJOR 3
#define R128_VERSION_MINOR 0
#define R128_VERSION_PATCH 0
#define R128_VERSION ((R128_VERSION_MAJOR << 16)  \
		      | (R128_VERSION_MINOR << 8) \
		      | R128_VERSION_PATCH)

#if R128_DEBUG
#define R128TRACE(x)                                          \
    do {                                                      \
        ErrorF("(**) %s(%d): ", R128_NAME, pScrn->scrnIndex); \
        ErrorF x;                                             \
    } while (0);
#else
#define R128TRACE(x)
#endif


/* Other macros */
#define R128_ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))
#define R128_ALIGN(x,bytes) (((x) + ((bytes) - 1)) & ~((bytes) - 1))
#define R128PTR(pScrn) ((R128InfoPtr)(pScrn)->driverPrivate)

typedef struct {	/* All values in XCLKS    */ 
    int  ML;		/* Memory Read Latency    */
    int  MB;		/* Memory Burst Length    */
    int  Trcd;		/* RAS to CAS delay       */
    int  Trp;		/* RAS percentage         */
    int  Twr;		/* Write Recovery         */
    int  CL;		/* CAS Latency            */
    int  Tr2w;		/* Read to Write Delay    */
    int  Rloop;		/* Loop Latency           */
    int  Rloop_fudge;	/* Add to ML to get Rloop */
    char *name;
} R128RAMRec, *R128RAMPtr;

typedef struct {
				/* Common registers */
    CARD32     ovr_clr;
    CARD32     ovr_wid_left_right;
    CARD32     ovr_wid_top_bottom;
    CARD32     ov0_scale_cntl;
    CARD32     mpp_tb_config;
    CARD32     mpp_gp_config;
    CARD32     subpic_cntl;
    CARD32     viph_control;
    CARD32     i2c_cntl_1;
    CARD32     gen_int_cntl;
    CARD32     cap0_trig_cntl;
    CARD32     cap1_trig_cntl;
    CARD32     bus_cntl;

				/* Other registers to save for VT switches */
    CARD32     dp_datatype;
    CARD32     gen_reset_cntl;
    CARD32     clock_cntl_index;
    CARD32     amcgpio_en_reg;
    CARD32     amcgpio_mask;

				/* Crtc registers */
    CARD32     crtc_gen_cntl;
    CARD32     crtc_ext_cntl;
    CARD32     dac_cntl;
    CARD32     crtc_h_total_disp;
    CARD32     crtc_h_sync_strt_wid;
    CARD32     crtc_v_total_disp;
    CARD32     crtc_v_sync_strt_wid;
    CARD32     crtc_offset;
    CARD32     crtc_offset_cntl;
    CARD32     crtc_pitch;

				/* Computed values for PLL */
    int        dot_clock_freq;
    int        pll_output_freq;
    int        feedback_div;
    int        post_div;

				/* PLL registers */
    CARD32     ppll_ref_div;
    CARD32     ppll_div_3;
    CARD32     htotal_cntl;
    
				/* DDA register */
    CARD32     dda_config;
    CARD32     dda_on_off;

				/* Pallet */
    Bool       palette_valid;
    CARD32     palette[256];
} R128SaveRec, *R128SavePtr;

typedef struct {
    CARD16        reference_freq;
    CARD16        reference_div;
    CARD32        min_pll_freq;
    CARD32        max_pll_freq;
    CARD16        xclk;
} R128PLLRec, *R128PLLPtr;

typedef struct {
    EntityInfoPtr     pEnt;
    pciVideoPtr       PciInfo;
    PCITAG            PciTag;
    int               Chipset;
    Bool              Primary;

    Bool              FBDev;
    
    unsigned long     LinearAddr; /* Frame buffer physical address           */
    unsigned long     MMIOAddr;	  /* MMIO region physical address            */
    unsigned long     BIOSAddr;	  /* BIOS physical address                   */

    unsigned char     *MMIO;	  /* Map of MMIO region                      */
    unsigned char     *FB;	  /* Map of frame buffer                     */

    CARD32            MemCntl;
    CARD32	      BusCntl;
    unsigned long     FbMapSize;  /* Size of frame buffer, in bytes          */
    int               Flags;	  /* Saved copy of mode flags                */
    
    R128PLLRec        pll;
    R128RAMPtr        ram;

    R128SaveRec       SavedReg;	  /* Original (text) mode                    */
    R128SaveRec       ModeReg;	  /* Current mode                            */
    Bool              (*CloseScreen)(int, ScreenPtr);

    I2CBusPtr         i2c;
    XAAInfoRecPtr     accel;
    xf86CursorInfoPtr cursor;
    unsigned long     cursor_start;
    unsigned long     cursor_end;

    int               fifo_slots; /* Free slots in the FIFO (64 max)         */
    int               pix24bpp;	  /* Depth of pixmap for 24bpp framebuffer   */
    int               pixel_code; /* Unified depth/fbbpp value               */
    int               pixel_bytes;/* Bytes per framebuffer pixel             */
    Bool              dac6bits;	  /* Use 6 bit DAC?                          */

				/* Computed values for Rage 128 */
    int               pitch;
    int               datatype;
    CARD32            dp_gui_master_cntl;

				/* Saved values for ScreenToScreenCopy */
    int               xdir;
    int               ydir;

				/* ScanlineScreenToScreenColorExpand support */
    unsigned char     *scratch_buffer[1];
    unsigned char     *scratch_save;
    int               scanline_x;
    int               scanline_y;
    int               scanline_h;
    int               scanline_h_w;
    int               scanline_words;
    int               scanline_direct;
    int               scanline_bpp; /* Only used for ImageWrite */
} R128InfoRec, *R128InfoPtr;

extern int         INPLL(ScrnInfoPtr pScrn, int addr);
extern void        R128WaitForVerticalSync(ScrnInfoPtr pScrn);

extern Bool        R128AccelInit(ScreenPtr pScreen);
extern void        R128EngineInit(ScrnInfoPtr pScrn);
extern Bool        R128CursorInit(ScreenPtr pScreen);

#endif
