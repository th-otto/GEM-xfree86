/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atistruct.h,v 1.12 2000/03/03 04:47:13 tsi Exp $ */
/*
 * Copyright 1999 through 2000 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef ___ATISTRUCT_H___
#define ___ATISTRUCT_H___ 1

#include "atibank.h"
#include "aticlock.h"
#include "xaa.h"

/*
 * This is probably as good a place as any to put this note, as it applies to
 * the entire driver, but especially here.  CARD8's are used rather than the
 * appropriate enum types because the latter would nearly quadruple storage
 * requirements (they are stored as int's).  This reduces the usefulness of
 * enum types to their ability to declare index values.  I've also elected to
 * forgo the strong typing capabilities of enum types.  C is not terribly adept
 * at strong typing anyway.
 */

/* A structure for local data related to video modes */
typedef struct _ATIHWRec
{
    /* Clock number for mode */
    CARD8 clock;

    /* The CRTC used to drive the screen (VGA, 8514, Mach64) */
    CARD8 crtc;

    /* VGA registers */
    CARD8 genmo, crt[25], seq[5], gra[9], attr[21], lut[256 * 3];

    /* Generic DAC registers */
    CARD8 dac_read, dac_write, dac_mask;

    /* VGA Wonder registers */
    CARD8             a3,         a6, a7,             ab, ac, ad, ae,
          b0, b1, b2, b3,     b5, b6,     b8, b9, ba,         bd, be, bf;

    /* Mach64 PLL registers */
    CARD8 pll_vclk_cntl, pll_ext_vpll_cntl;

    /* Mach64 CPIO registers */
    CARD32 crtc_h_total_disp, crtc_h_sync_strt_wid,
           crtc_v_total_disp, crtc_v_sync_strt_wid,
           crtc_off_pitch, crtc_gen_cntl, dsp_config, dsp_on_off,
           ovr_clr, ovr_wid_left_right, ovr_wid_top_bottom,
           clock_cntl, bus_cntl, mem_vga_wp_sel, mem_vga_rp_sel,
           dac_cntl, config_cntl;

    /* LCD registers */
    CARD32 lcd_index, config_panel, lcd_gen_ctrl,
           horz_stretching, vert_stretching, ext_vert_stretch;

    /* Shadow VGA CRTC registers */
    CARD8 shadow_vga[25];

    /* Shadow Mach64 CRTC registers */
    CARD32 shadow_h_total_disp, shadow_h_sync_strt_wid,
           shadow_v_total_disp, shadow_v_sync_strt_wid;

    /* Mach64 MMIO Block 0 registers */
    CARD32 dst_off_pitch;
    CARD16 dst_x, dst_y, dst_height;
    CARD32 dst_bres_err, dst_bres_inc, dst_bres_dec, dst_cntl;
    CARD32 src_off_pitch;
    CARD16 src_x, src_y, src_width1, src_height1,
           src_x_start, src_y_start, src_width2, src_height2;
    CARD32 src_cntl;
    CARD32 host_cntl;
    CARD32 pat_reg0, pat_reg1, pat_cntl;
    CARD16 sc_left, sc_right, sc_top, sc_bottom;
    CARD32 dp_bkgd_clr, dp_frgd_clr, dp_write_mask, dp_chain_mask,
           dp_pix_width, dp_mix, dp_src;
    CARD32 clr_cmp_clr, clr_cmp_msk, clr_cmp_cntl;
    CARD32 context_mask, context_load_cntl;

    /* Mach64 MMIO Block 1 registers */
    CARD32 gui_cntl;

    /* Clock map pointers */
    const CARD8 *ClockMap, *ClockUnmap;

    /* Clock programming data */
    int FeedbackDivider, ReferenceDivider, PostDivider;

    /* This is used by ATISwap() */
    pointer frame_buffer;
    ATIBankProcPtr SetBank;
    unsigned int nBank, nPlane;
} ATIHWRec;

/*
 * This structure defines the driver's private area.
 */
typedef struct _ATIRec
{
    /*
     * Definitions related to XF86Config "Chipset" specifications.
     */
    CARD8 Chipset;

    /*
     * Adapter-related definitions.
     */
    CARD8 Adapter, VGAAdapter;

    /*
     * Chip-related definitions.
     */
    CARD8 Chip, Coprocessor;
    CARD16 ChipType, ChipClass, ChipRevision, ChipRev;
    CARD16 ChipVersion, ChipFoundry;
    CARD8 ChipHasSUBSYS_CNTL;

    /*
     * Processor I/O decoding definitions.
     */
    CARD8 CPIODecoding;
    CARD16 CPIOBase;

    /*
     * Processor I/O port definition for VGA.
     */
    CARD16 CPIO_VGABase;

    /*
     * Processor I/O port definitions for VGA Wonder.
     */
    CARD16 CPIO_VGAWonder;
    CARD8 B2Reg;        /* The B2 mirror */
    CARD8 VGAOffset;    /* Low index for CPIO_VGAWonder */

    /*
     * Processor I/O port definitions for Mach64.
     */
    CARD16 CPIO_CRTC_H_TOTAL_DISP, CPIO_CRTC_H_SYNC_STRT_WID,
           CPIO_CRTC_V_TOTAL_DISP, CPIO_CRTC_V_SYNC_STRT_WID,
           CPIO_CRTC_OFF_PITCH, CPIO_CRTC_INT_CNTL, CPIO_CRTC_GEN_CNTL,
           CPIO_DSP_CONFIG, CPIO_DSP_ON_OFF, CPIO_OVR_CLR,
           CPIO_OVR_WID_LEFT_RIGHT, CPIO_OVR_WID_TOP_BOTTOM,
           CPIO_TV_OUT_INDEX, CPIO_CLOCK_CNTL, CPIO_TV_OUT_DATA,
           CPIO_BUS_CNTL, CPIO_LCD_INDEX, CPIO_LCD_DATA, CPIO_MEM_INFO,
           CPIO_MEM_VGA_WP_SEL, CPIO_MEM_VGA_RP_SEL,
           CPIO_DAC_REGS, CPIO_DAC_CNTL,
           CPIO_HORZ_STRETCHING, CPIO_VERT_STRETCHING,
           CPIO_GEN_TEST_CNTL, CPIO_LCD_GEN_CTRL, CPIO_CONFIG_CNTL;

    /*
     * DAC-related definitions.
     */
    CARD16 DAC;
    CARD16 CPIO_DAC_MASK, CPIO_DAC_DATA, CPIO_DAC_READ, CPIO_DAC_WRITE;

    /*
     * Definitions related to system bus interface.
     */
    pciVideoPtr PCIInfo;
    CARD8 BusType;
    CARD8 SharedVGA, SharedAccelerator;

    /*
     * Definitions related to video memory.
     */
    CARD8 MemoryType;
    int VideoRAM;

    /*
     * BIOS-related definitions.
     */
    unsigned long BIOSBase;

    /*
     * Definitions related to video memory apertures.
     */
    pointer pBank, pMemory, pShadow;
    unsigned long LinearBase, ApertureBase;
    int LinearSize, ApertureSize, FBPitch, FBBytesPerPixel;
    CARD8 UseSmallApertures;

    /*
     * Definitions related to MMIO register apertures.
     */
    pointer pMMIO, pBlock[2];
    unsigned long PageSize, MMIOBase;
    unsigned long Block0Base, Block1Base;

    /*
     * Banking interface.
     */
    miBankInfoRec BankInfo;

    /*
     * XAA interface.
     */
    XAAInfoRecPtr pXAAInfo;
    int nAvailableFIFOEntries, nFIFOEntries;
    CARD8 EngineIsBusy, EngineIsLocked, PitchModifier;
    CARD32 dst_cntl;    /* For SetupFor/Subsequent communication */

    /*
     * Clock-related definitions.
     */
    int ClockNumberToProgramme, ReferenceNumerator, ReferenceDenominator;
    ClockRec ClockDescriptor;
    CARD16 BIOSClocks[16];
    CARD8 Clock, ProgrammableClock;

    /*
     * DSP register data.
     */
    int XCLKFeedbackDivider, XCLKReferenceDivider, XCLKPostDivider;
    CARD16 XCLKMaxRASDelay, XCLKPageFaultDelay,
           DisplayLoopLatency, DisplayFIFODepth;

    /*
     * LCD panel data.
     */
    int LCDPanelID, LCDClock, LCDHorizontal, LCDVertical;
    int LCDHSyncStart, LCDHSyncWidth, LCDHBlankWidth;
    int LCDVSyncStart, LCDVSyncWidth, LCDVBlankWidth;
    int LCDVBlendFIFOSize;

    /*
     * Data used by ATIAdjustFrame().
     */
    int AdjustDepth, AdjustMaxX, AdjustMaxY;
    unsigned long AdjustMask;

    /*
     * Data saved by ATIUnlock() and restored by ATILock().
     */
    struct
    {
        /* VGA registers */
        CARD8 crt03, crt11;

        /* VGA shadow registers */
        CARD8 shadow_crt03, shadow_crt11;

        /* VGA Wonder registers */
        CARD8 a6, ab, b1, b4, b5, b6, b8, b9, be;

        /* Mach8/Mach32 registers */
        CARD16 clock_sel, misc_options, mem_bndry, mem_cfg;

        /* Mach64 registers */
        CARD32 bus_cntl, config_cntl, crtc_gen_cntl, mem_info, gen_test_cntl,
               dac_cntl, crtc_int_cntl, lcd_index;
    } LockData;

    /* Mode data */
    ATIHWRec OldHW, NewHW;
    int MaximumInterlacedPitch;
    Bool InterlacedSeen;

    /*
     * Resource Access Control entity index.
     */
    int iEntity;

    /*
     * Driver options.
     */
    CARD8 OptionAccel;          /* Use hardware draw engine */
    CARD8 OptionCRT;            /* Prefer CRT over digital panel */
    CARD8 OptionCSync;          /* Use composite sync */
    CARD8 OptionDevel;          /* Intentionally undocumented */
    CARD8 OptionLinear;         /* Use linear fb aperture when available */
    CARD8 OptionProbeClocks;    /* Force probe for fixed clocks */
    CARD8 OptionShadowFB;       /* Use shadow frame buffer */
    CARD8 OptionSync;           /* Temporary */

    /*
     * State flags.
     */
    CARD8 Unlocked, Mapped, Closeable;

    /*
     * Wrapped functions.
     */
    CloseScreenProcPtr CloseScreen;
} ATIRec;

#define ATIPTR(_p) ((ATIPtr)((_p)->driverPrivate))

#endif /* ___ATISTRUCT_H___ */
