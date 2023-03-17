/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atimach64.c,v 1.12 2000/03/03 04:47:13 tsi Exp $ */
/*
 * Copyright 1997 through 2000 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
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
/*
 * Copyright 1999-2000 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sub license, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "atibus.h"
#include "atichip.h"
#include "atiio.h"
#include "atimach64.h"

/*
 * Only 32-bit MMIO is needed here.
 */

#define inm(_Register)                                         \
    MMIO_IN32(pATI->pBlock[GetBits(_Register, BLOCK_SELECT)],  \
              (_Register) & MM_IO_SELECT)

/*
 * ATIMach64PollEngineStatus --
 *
 * This function refreshes the driver's view of the draw engine's status.
 */
static void
ATIMach64PollEngineStatus
(
    ATIPtr pATI
)
{
    CARD32 IOValue;
    int    Count;

    if (pATI->Chip < ATI_CHIP_264VTB)
    {
        /*
         * TODO:  Deal with locked engines.
         */
        IOValue = inm(FIFO_STAT);
        pATI->EngineIsLocked = GetBits(IOValue, FIFO_ERR);

        /*
         * The following counts the number of bits in FIFO_STAT_BITS, and is
         * derived from miSetVisualTypes() (formerly cfbSetVisualTypes()).
         */
        IOValue = GetBits(IOValue, FIFO_STAT_BITS);
        Count = (IOValue >> 1) & 0x36DBU;
        Count = IOValue - Count - ((Count >> 1) & 0x36DBU);
        Count = ((Count + (Count >> 3)) & 0x71C7U) % 0x3FU;
        Count = pATI->nFIFOEntries - Count;
        if (Count > pATI->nAvailableFIFOEntries)
            pATI->nAvailableFIFOEntries = Count;

        /*
         * If the command FIFO is non-empty, then the engine isn't idle.
         */
        if (pATI->nAvailableFIFOEntries < pATI->nFIFOEntries)
        {
            pATI->EngineIsBusy = TRUE;
            return;
        }
    }

    IOValue = inm(GUI_STAT);
    pATI->EngineIsBusy = GetBits(IOValue, GUI_ACTIVE);
    Count = GetBits(IOValue, GUI_FIFO);
    if (Count > pATI->nAvailableFIFOEntries)
        pATI->nAvailableFIFOEntries = Count;
}


#define outm(_Register, _Value)                                    \
    do                                                             \
    {                                                              \
        while (!pATI->nAvailableFIFOEntries--)                     \
            ATIMach64PollEngineStatus(pATI);                       \
        MMIO_OUT32(pATI->pBlock[GetBits(_Register, BLOCK_SELECT)], \
                   (_Register) & MM_IO_SELECT, _Value);            \
        pATI->EngineIsBusy = TRUE;                                 \
    } while (0)

#define ATIMach64WaitForFIFO(_n)                               \
    while ((pATI->nAvailableFIFOEntries < (_n)) &&             \
           (pATI->nAvailableFIFOEntries < pATI->nFIFOEntries)) \
        ATIMach64PollEngineStatus(pATI)

#define ATIMach64WaitForIdle()                 \
    while (pATI->EngineIsBusy)                 \
        ATIMach64PollEngineStatus(pATI)

/*
 * X-to-Mach64 mix translation table.
 */
static CARD8 ATIMach64ALU[16] =
{
    MIX_0,
    MIX_AND,
    MIX_SRC_AND_NOT_DST,
    MIX_SRC,
    MIX_NOT_SRC_AND_DST,
    MIX_DST,
    MIX_XOR,
    MIX_OR,
    MIX_NOR,
    MIX_XNOR,
    MIX_NOT_DST,
    MIX_SRC_OR_NOT_DST,
    MIX_NOT_SRC,
    MIX_NOT_SRC_OR_DST,
    MIX_NAND,
    MIX_1
};

/*
 * ATIMach64PreInit --
 *
 * This function fills in the Mach64 portion of an ATIHWRec that is common to
 * all video modes generated by the driver.
 */
void
ATIMach64PreInit
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI,
    ATIHWPtr    pATIHW
)
{
    if (pScreenInfo->depth <= 4)
        pATIHW->crtc_off_pitch =
            SetBits(pScreenInfo->displayWidth >> 4, CRTC_PITCH);
    else
        pATIHW->crtc_off_pitch =
            SetBits(pScreenInfo->displayWidth >> 3, CRTC_PITCH);

    pATIHW->bus_cntl = (inl(pATI->CPIO_BUS_CNTL) & ~BUS_HOST_ERR_INT_EN) |
        BUS_HOST_ERR_INT;
    if (pATI->Chip < ATI_CHIP_264VTB)
    {
        pATIHW->bus_cntl &= ~(BUS_FIFO_ERR_INT_EN | BUS_ROM_DIS);
        pATIHW->bus_cntl |= SetBits(15, BUS_FIFO_WS) | BUS_FIFO_ERR_INT;
    }
    else
        pATIHW->bus_cntl |= BUS_APER_REG_DIS;
    if (pATI->Chip >= ATI_CHIP_264VT)
        pATIHW->bus_cntl |= BUS_EXT_REG_EN;     /* Enable Block 1 */

    pATIHW->mem_vga_wp_sel =
        /* SetBits(0, MEM_VGA_WPS0) | */
        SetBits(pATIHW->nPlane, MEM_VGA_WPS1);
    pATIHW->mem_vga_rp_sel =
        /* SetBits(0, MEM_VGA_RPS0) | */
        SetBits(pATIHW->nPlane, MEM_VGA_RPS1);

    pATIHW->dac_cntl = inl(pATI->CPIO_DAC_CNTL) &
        ~(DAC1_CLK_SEL | DAC_PALETTE_ACCESS_CNTL);
    if ((pScreenInfo->depth > 8) || (pScreenInfo->rgbBits == 8))
        pATIHW->dac_cntl |= DAC_8BIT_EN;
    else
        pATIHW->dac_cntl &= ~DAC_8BIT_EN;

    pATIHW->config_cntl = inl(pATI->CPIO_CONFIG_CNTL);
    if (pATI->UseSmallApertures)
        pATIHW->config_cntl |= CFG_MEM_VGA_AP_EN;
    else
        pATIHW->config_cntl &= ~CFG_MEM_VGA_AP_EN;
    if (pATI->LinearBase &&
        ((pATI->Chip < ATI_CHIP_264CT) ||
         ((pATI->BusType != ATI_BUS_PCI) && (pATI->BusType != ATI_BUS_AGP))))
    {
        /* Replace linear aperture size and address */
        pATIHW->config_cntl &= ~(CFG_MEM_AP_LOC | CFG_MEM_AP_SIZE);
        pATIHW->config_cntl |= SetBits(pATI->LinearBase >> 22, CFG_MEM_AP_LOC);
        if ((pATI->Chip < ATI_CHIP_264CT) && (pATI->VideoRAM < 4096))
            pATIHW->config_cntl |= SetBits(1, CFG_MEM_AP_SIZE);
        else
            pATIHW->config_cntl |= SetBits(2, CFG_MEM_AP_SIZE);
    }

    /* Draw engine setup */
    if (pATI->OptionAccel)
    {
        /* Don't treat 24bpp as a special case */
        pATI->PitchModifier =
            pScreenInfo->bitsPerPixel / UnitOf(pScreenInfo->bitsPerPixel);

        /*
         * When possible, max out command FIFO size.
         */
        if (pATI->Chip >= ATI_CHIP_264VT4)
            pATIHW->gui_cntl = inm(GUI_CNTL) & ~CMDFIFO_SIZE_MODE;

        /* Initialise destination registers */
        pATIHW->dst_off_pitch =
            SetBits((pScreenInfo->displayWidth * pATI->PitchModifier) >> 3,
                DST_PITCH);
        pATIHW->dst_cntl = DST_X_DIR | DST_Y_DIR | DST_LAST_PEL;

        /* Initialise source registers */
        pATIHW->src_off_pitch = pATIHW->dst_off_pitch;
        pATIHW->src_width1 = pATIHW->src_height1 =
            pATIHW->src_width2 = pATIHW->src_height2 = 1;
        pATIHW->src_cntl = SRC_LINE_X_DIR;

        /* Initialise scissor, allowing for offscreen areas */
        pATIHW->sc_right =
            (pScreenInfo->displayWidth * pATI->PitchModifier) - 1;
        pATIHW->sc_bottom =
            (pScreenInfo->videoRam * 1024 * 8 / pScreenInfo->displayWidth /
             pScreenInfo->bitsPerPixel) - 1;

        /* Initialise data path */
        pATIHW->dp_frgd_clr = (CARD32)(-1);
        pATIHW->dp_write_mask = (CARD32)(-1);

        switch (pScreenInfo->depth)
        {
            case 8:
                pATIHW->dp_chain_mask = DP_CHAIN_8BPP;
                pATIHW->dp_pix_width = /* DP_BYTE_PIX_ORDER | */
                    SetBits(PIX_WIDTH_8BPP, DP_DST_PIX_WIDTH) |
                    SetBits(PIX_WIDTH_8BPP, DP_SRC_PIX_WIDTH) |
                    SetBits(PIX_WIDTH_8BPP, DP_HOST_PIX_WIDTH);
                break;

            case 15:
                pATIHW->dp_chain_mask = DP_CHAIN_15BPP_1555;
                pATIHW->dp_pix_width = /* DP_BYTE_PIX_ORDER | */
                    SetBits(PIX_WIDTH_15BPP, DP_DST_PIX_WIDTH) |
                    SetBits(PIX_WIDTH_15BPP, DP_SRC_PIX_WIDTH) |
                    SetBits(PIX_WIDTH_15BPP, DP_HOST_PIX_WIDTH);
                break;

            case 16:
                pATIHW->dp_chain_mask = DP_CHAIN_16BPP_565;
                pATIHW->dp_pix_width = /* DP_BYTE_PIX_ORDER | */
                    SetBits(PIX_WIDTH_16BPP, DP_DST_PIX_WIDTH) |
                    SetBits(PIX_WIDTH_16BPP, DP_SRC_PIX_WIDTH) |
                    SetBits(PIX_WIDTH_16BPP, DP_HOST_PIX_WIDTH);
                break;

            case 24:
                if (pScreenInfo->bitsPerPixel == 24)
                {
                    pATIHW->dp_chain_mask = DP_CHAIN_24BPP_888;
                    pATIHW->dp_pix_width = /* DP_BYTE_PIX_ORDER | */
                        SetBits(PIX_WIDTH_8BPP, DP_DST_PIX_WIDTH) |
                        SetBits(PIX_WIDTH_8BPP, DP_SRC_PIX_WIDTH) |
                        SetBits(PIX_WIDTH_8BPP, DP_HOST_PIX_WIDTH);
                }
                else
                {
                    pATIHW->dp_chain_mask = DP_CHAIN_32BPP_8888;
                    pATIHW->dp_pix_width = /* DP_BYTE_PIX_ORDER | */
                        SetBits(PIX_WIDTH_32BPP, DP_DST_PIX_WIDTH) |
                        SetBits(PIX_WIDTH_32BPP, DP_SRC_PIX_WIDTH) |
                        SetBits(PIX_WIDTH_32BPP, DP_HOST_PIX_WIDTH);
                }
                break;

            default:
                break;
        }

        pATIHW->dp_mix = SetBits(MIX_SRC, DP_FRGD_MIX) |
            SetBits(MIX_DST, DP_BKGD_MIX);
        pATIHW->dp_src = SetBits(DP_MONO_SRC_ALLONES, DP_MONO_SRC) |
            SetBits(SRC_FRGD, DP_FRGD_SRC) |
            SetBits(SRC_BKGD, DP_BKGD_SRC);

        /* Initialise colour compare */
        pATIHW->clr_cmp_msk = (CARD32)(-1);
    }
}

/*
 * ATIMach64Save --
 *
 * This function is called to save the Mach64 portion of the current video
 * state.
 */
void
ATIMach64Save
(
    ATIPtr      pATI,
    ATIHWPtr    pATIHW
)
{
    pATIHW->crtc_h_total_disp = inl(pATI->CPIO_CRTC_H_TOTAL_DISP);
    pATIHW->crtc_h_sync_strt_wid = inl(pATI->CPIO_CRTC_H_SYNC_STRT_WID);
    pATIHW->crtc_v_total_disp = inl(pATI->CPIO_CRTC_V_TOTAL_DISP);
    pATIHW->crtc_v_sync_strt_wid = inl(pATI->CPIO_CRTC_V_SYNC_STRT_WID);

    pATIHW->crtc_off_pitch = inl(pATI->CPIO_CRTC_OFF_PITCH);

    pATIHW->crtc_gen_cntl = inl(pATI->CPIO_CRTC_GEN_CNTL);

    pATIHW->ovr_clr = inl(pATI->CPIO_OVR_CLR);
    pATIHW->ovr_wid_left_right = inl(pATI->CPIO_OVR_WID_LEFT_RIGHT);
    pATIHW->ovr_wid_top_bottom = inl(pATI->CPIO_OVR_WID_TOP_BOTTOM);

    pATIHW->clock_cntl = inl(pATI->CPIO_CLOCK_CNTL);

    pATIHW->bus_cntl = inl(pATI->CPIO_BUS_CNTL);

    pATIHW->mem_vga_wp_sel = inl(pATI->CPIO_MEM_VGA_WP_SEL);
    pATIHW->mem_vga_rp_sel = inl(pATI->CPIO_MEM_VGA_RP_SEL);

    pATIHW->dac_cntl = inl(pATI->CPIO_DAC_CNTL);

    pATIHW->config_cntl = inl(pATI->CPIO_CONFIG_CNTL);

    /* Save draw engine state */
    if (pATI->OptionAccel && (pATIHW == &pATI->OldHW))
    {
        ATIMach64WaitForIdle();

        /* Save FIFO size */
        if (pATI->Chip >= ATI_CHIP_264VT4)
            pATIHW->gui_cntl = inm(GUI_STAT);

        /* Save destination registers */
        pATIHW->dst_off_pitch = inm(DST_OFF_PITCH);
        pATIHW->dst_x = inm(DST_X);
        pATIHW->dst_y = inm(DST_Y);
        pATIHW->dst_height = inm(DST_HEIGHT);
        pATIHW->dst_bres_err = inm(DST_BRES_ERR);
        pATIHW->dst_bres_inc = inm(DST_BRES_INC);
        pATIHW->dst_bres_dec = inm(DST_BRES_DEC);
        pATIHW->dst_cntl = inm(DST_CNTL);

        /* Save source registers */
        pATIHW->src_off_pitch = inm(SRC_OFF_PITCH);
        pATIHW->src_x = inm(SRC_X);
        pATIHW->src_y = inm(SRC_Y);
        pATIHW->src_width1 = inm(SRC_WIDTH1);
        pATIHW->src_height1 = inm(SRC_HEIGHT1);
        pATIHW->src_x_start = inm(SRC_X_START);
        pATIHW->src_y_start = inm(SRC_Y_START);
        pATIHW->src_width2 = inm(SRC_WIDTH2);
        pATIHW->src_height2 = inm(SRC_HEIGHT2);
        pATIHW->src_cntl = inm(SRC_CNTL);

        /* Save host data register */
        pATIHW->host_cntl = inm(HOST_CNTL);

        /* Save pattern registers */
        pATIHW->pat_reg0 = inm(PAT_REG0);
        pATIHW->pat_reg1 = inm(PAT_REG1);
        pATIHW->pat_cntl = inm(PAT_CNTL);

        /* Save scissor registers */
        pATIHW->sc_left = inm(SC_LEFT);
        pATIHW->sc_right = inm(SC_RIGHT);
        pATIHW->sc_top = inm(SC_TOP);
        pATIHW->sc_bottom = inm(SC_BOTTOM);

        /* Save data path registers */
        pATIHW->dp_bkgd_clr = inm(DP_BKGD_CLR);
        pATIHW->dp_frgd_clr = inm(DP_FRGD_CLR);
        pATIHW->dp_write_mask = inm(DP_WRITE_MASK);
        pATIHW->dp_chain_mask = inm(DP_CHAIN_MASK);
        pATIHW->dp_pix_width = inm(DP_PIX_WIDTH);
        pATIHW->dp_mix = inm(DP_MIX);
        pATIHW->dp_src = inm(DP_SRC);

        /* Save colour compare registers */
        pATIHW->clr_cmp_clr = inm(CLR_CMP_CLR);
        pATIHW->clr_cmp_msk = inm(CLR_CMP_MSK);
        pATIHW->clr_cmp_cntl = inm(CLR_CMP_CNTL);

        /* Save context */
        pATIHW->context_mask = inm(CONTEXT_MASK);
    }
}

/*
 * ATIMach64Calculate --
 *
 * This function is called to fill in the Mach64 portion of an ATIHWRec.
 */
void
ATIMach64Calculate
(
    ScrnInfoPtr    pScreenInfo,
    ATIPtr         pATI,
    ATIHWPtr       pATIHW,
    DisplayModePtr pMode
)
{
    int VDisplay;

    /* If not already done adjust horizontal timings */
    if (!pMode->CrtcHAdjusted)
    {
        pMode->CrtcHAdjusted = TRUE;
        /* XXX Deal with Blank Start/End and overscan later */
        pMode->CrtcHDisplay = (pMode->HDisplay >> 3) - 1;
        pMode->CrtcHSyncStart = (pMode->HSyncStart >> 3) - 1;
        pMode->CrtcHSyncEnd = (pMode->HSyncEnd >> 3) - 1;
        pMode->CrtcHTotal = (pMode->HTotal >> 3) - 1;

        /* Make adjustments if sync pulse width is out-of-bounds */
        if ((pMode->CrtcHSyncEnd - pMode->CrtcHSyncStart) >
            (int)MaxBits(CRTC_H_SYNC_WID))
            pMode->CrtcHSyncEnd =
                pMode->CrtcHSyncStart + MaxBits(CRTC_H_SYNC_WID);
        else if (pMode->CrtcHSyncStart == pMode->CrtcHSyncEnd)
        {
            if (pMode->CrtcHDisplay < pMode->CrtcHSyncStart)
                pMode->CrtcHSyncStart--;
            else if (pMode->CrtcHSyncEnd < pMode->CrtcHTotal)
                pMode->CrtcHSyncEnd++;
        }
    }

    /*
     * Always re-do vertical adjustments.
     */
    pMode->CrtcVDisplay = pMode->VDisplay;
    pMode->CrtcVSyncStart = pMode->VSyncStart;
    pMode->CrtcVSyncEnd = pMode->VSyncEnd;
    pMode->CrtcVTotal = pMode->VTotal;

    if ((pATI->Chip >= ATI_CHIP_264CT) &&
        ((pMode->Flags & V_DBLSCAN) || (pMode->VScan > 1)))
    {
        pMode->CrtcVDisplay <<= 1;
        pMode->CrtcVSyncStart <<= 1;
        pMode->CrtcVSyncEnd <<= 1;
        pMode->CrtcVTotal <<= 1;
    }

    /*
     * Might as well default to the same as VGA with respect to sync
     * polarities.
     */
    if ((!(pMode->Flags & (V_PHSYNC | V_NHSYNC))) ||
        (!(pMode->Flags & (V_PVSYNC | V_NVSYNC))))
    {
        pMode->Flags &= ~(V_PHSYNC | V_NHSYNC | V_PVSYNC | V_NVSYNC);

        if (!pATI->OptionCRT && (pATI->LCDPanelID >= 0))
            VDisplay = pATI->LCDVertical;
        else
            VDisplay = pMode->CrtcVDisplay;

        if (VDisplay < 400)
            pMode->Flags |= V_PHSYNC | V_NVSYNC;
        else if (VDisplay < 480)
            pMode->Flags |= V_NHSYNC | V_PVSYNC;
        else if (VDisplay < 768)
            pMode->Flags |= V_NHSYNC | V_NVSYNC;
        else
            pMode->Flags |= V_PHSYNC | V_PVSYNC;
    }

    pMode->CrtcVDisplay--;
    pMode->CrtcVSyncStart--;
    pMode->CrtcVSyncEnd--;
    pMode->CrtcVTotal--;
    /* Make sure sync pulse is not too wide */
    if ((pMode->CrtcVSyncEnd - pMode->CrtcVSyncStart) >
         (int)MaxBits(CRTC_V_SYNC_WID))
        pMode->CrtcVSyncEnd = pMode->CrtcVSyncStart + MaxBits(CRTC_V_SYNC_WID);
    pMode->CrtcVAdjusted = TRUE;                /* Redundant */

    /* Build register contents */
    pATIHW->crtc_h_total_disp =
        SetBits(pMode->CrtcHTotal, CRTC_H_TOTAL) |
            SetBits(pMode->CrtcHDisplay, CRTC_H_DISP);
    pATIHW->crtc_h_sync_strt_wid =
        SetBits(pMode->CrtcHSyncStart, CRTC_H_SYNC_STRT) |
            SetBits(pMode->CrtcHSkew, CRTC_H_SYNC_DLY) |         /* ? */
            SetBits(GetBits(pMode->CrtcHSyncStart, 0x0100U),
                CRTC_H_SYNC_STRT_HI) |
            SetBits(pMode->CrtcHSyncEnd - pMode->CrtcHSyncStart,
                CRTC_H_SYNC_WID);
    if (pMode->Flags & V_NHSYNC)
        pATIHW->crtc_h_sync_strt_wid |= CRTC_H_SYNC_POL;

    pATIHW->crtc_v_total_disp =
        SetBits(pMode->CrtcVTotal, CRTC_V_TOTAL) |
            SetBits(pMode->CrtcVDisplay, CRTC_V_DISP);
    pATIHW->crtc_v_sync_strt_wid =
        SetBits(pMode->CrtcVSyncStart, CRTC_V_SYNC_STRT) |
            SetBits(pMode->CrtcVSyncEnd - pMode->CrtcVSyncStart,
                CRTC_V_SYNC_WID);
    if (pMode->Flags & V_NVSYNC)
        pATIHW->crtc_v_sync_strt_wid |= CRTC_V_SYNC_POL;

    pATIHW->crtc_gen_cntl = inl(pATI->CPIO_CRTC_GEN_CNTL) &
        ~(CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN |
          CRTC_HSYNC_DIS | CRTC_VSYNC_DIS | CRTC_CSYNC_EN |
          CRTC_PIX_BY_2_EN | CRTC_DISPLAY_DIS | CRTC_VGA_XOVERSCAN |
          CRTC_PIX_WIDTH | CRTC_BYTE_PIX_ORDER | CRTC_FIFO_LWM |
          CRTC_VGA_128KAP_PAGING | CRTC_VFC_SYNC_TRISTATE |
          CRTC_LOCK_REGS |              /* Already off, but ... */
          CRTC_SYNC_TRISTATE | CRTC_DISP_REQ_EN |
          CRTC_VGA_TEXT_132 | CRTC_CUR_B_TEST);
    pATIHW->crtc_gen_cntl |=
        CRTC_EXT_DISP_EN | CRTC_EN | CRTC_VGA_LINEAR | CRTC_CNT_EN;
    switch (pScreenInfo->depth)
    {
        case 1:
            pATIHW->crtc_gen_cntl |= SetBits(PIX_WIDTH_1BPP, CRTC_PIX_WIDTH);
            break;
        case 4:
            pATIHW->crtc_gen_cntl |= SetBits(PIX_WIDTH_4BPP, CRTC_PIX_WIDTH);
            break;
        case 8:
            pATIHW->crtc_gen_cntl |= SetBits(PIX_WIDTH_8BPP, CRTC_PIX_WIDTH);
            break;
        case 15:
            pATIHW->crtc_gen_cntl |= SetBits(PIX_WIDTH_15BPP, CRTC_PIX_WIDTH);
            break;
        case 16:
            pATIHW->crtc_gen_cntl |= SetBits(PIX_WIDTH_16BPP, CRTC_PIX_WIDTH);
            break;
        case 24:
            if (pScreenInfo->bitsPerPixel == 24)
            {
                pATIHW->crtc_gen_cntl |=
                    SetBits(PIX_WIDTH_24BPP, CRTC_PIX_WIDTH);
                break;
            }
            if (pScreenInfo->bitsPerPixel != 32)
                break;
            /* Fall through */
        case 32:
            pATIHW->crtc_gen_cntl |= SetBits(PIX_WIDTH_32BPP, CRTC_PIX_WIDTH);
            break;
        default:
            break;
    }
    if ((pMode->Flags & V_DBLSCAN) || (pMode->VScan > 1))
        pATIHW->crtc_gen_cntl |= CRTC_DBL_SCAN_EN;
    if (pMode->Flags & V_INTERLACE)
        pATIHW->crtc_gen_cntl |= CRTC_INTERLACE_EN;
    if (pATI->OptionCSync || (pMode->Flags & (V_CSYNC | V_PCSYNC)))
        pATIHW->crtc_gen_cntl |= CRTC_CSYNC_EN;
    /* For now, set display FIFO low water mark as high as possible */
    if (pATI->Chip < ATI_CHIP_264VTB)
        pATIHW->crtc_gen_cntl |= CRTC_FIFO_LWM;
}

/*
 * ATIMach64Set --
 *
 * This function is called to load a Mach64's accelerator CRTC.
 */
void
ATIMach64Set
(
    ATIPtr      pATI,
    ATIHWPtr    pATIHW
)
{
    /* First, turn off the display */
    outl(pATI->CPIO_CRTC_GEN_CNTL, pATIHW->crtc_gen_cntl & ~CRTC_EN);

    if ((pATIHW->FeedbackDivider > 0) &&
        (pATI->ProgrammableClock != ATI_CLOCK_NONE))
        ATIClockSet(pATI, pATIHW);              /* Programme clock */

    /* Load Mach64 CRTC registers */
    outl(pATI->CPIO_CRTC_H_TOTAL_DISP, pATIHW->crtc_h_total_disp);
    outl(pATI->CPIO_CRTC_H_SYNC_STRT_WID, pATIHW->crtc_h_sync_strt_wid);
    outl(pATI->CPIO_CRTC_V_TOTAL_DISP, pATIHW->crtc_v_total_disp);
    outl(pATI->CPIO_CRTC_V_SYNC_STRT_WID, pATIHW->crtc_v_sync_strt_wid);

    outl(pATI->CPIO_CRTC_OFF_PITCH, pATIHW->crtc_off_pitch);

    /* Set pixel clock */
    outl(pATI->CPIO_CLOCK_CNTL, pATIHW->clock_cntl | CLOCK_STROBE);

    /* Load overscan registers */
    outl(pATI->CPIO_OVR_CLR, pATIHW->ovr_clr);
    outl(pATI->CPIO_OVR_WID_LEFT_RIGHT, pATIHW->ovr_wid_left_right);
    outl(pATI->CPIO_OVR_WID_TOP_BOTTOM, pATIHW->ovr_wid_top_bottom);

    /* Finalise CRTC setup and turn on the screen */
    outl(pATI->CPIO_CRTC_GEN_CNTL, pATIHW->crtc_gen_cntl);

    /* Aperture setup */
    outl(pATI->CPIO_BUS_CNTL, pATIHW->bus_cntl);

    outl(pATI->CPIO_MEM_VGA_WP_SEL, pATIHW->mem_vga_wp_sel);
    outl(pATI->CPIO_MEM_VGA_RP_SEL, pATIHW->mem_vga_rp_sel);

    outl(pATI->CPIO_DAC_CNTL, pATIHW->dac_cntl);

    outl(pATI->CPIO_CONFIG_CNTL, pATIHW->config_cntl);

    /* Load draw engine */
    if (pATI->OptionAccel)
    {
        pATI->EngineIsBusy = TRUE;      /* Force engine poll */
        ATIMach64WaitForIdle();

        /* Load FIFO size */
        if (pATI->Chip >= ATI_CHIP_264VT4)
        {
            outm(GUI_CNTL, pATIHW->gui_cntl);
            pATI->nAvailableFIFOEntries = 0;
            ATIMach64PollEngineStatus(pATI);
        }

        /* Load destination registers */
        ATIMach64WaitForFIFO(7);
        outm(DST_OFF_PITCH, pATIHW->dst_off_pitch);
        outm(DST_Y_X, SetWord(pATIHW->dst_x, 1) | SetWord(pATIHW->dst_y, 0));
        outm(DST_HEIGHT, pATIHW->dst_height);
        outm(DST_BRES_ERR, pATIHW->dst_bres_err);
        outm(DST_BRES_INC, pATIHW->dst_bres_inc);
        outm(DST_BRES_DEC, pATIHW->dst_bres_dec);
        outm(DST_CNTL, pATIHW->dst_cntl);

        /* Load source registers */
        ATIMach64WaitForFIFO(6);
        outm(SRC_OFF_PITCH, pATIHW->src_off_pitch);
        outm(SRC_Y_X, SetWord(pATIHW->src_x, 1) | SetWord(pATIHW->src_y, 0));
        outm(SRC_HEIGHT1_WIDTH1,
            SetWord(pATIHW->src_width1, 1) | SetWord(pATIHW->src_height1, 0));
        outm(SRC_Y_X_START,
            SetWord(pATIHW->src_x_start, 1) | SetWord(pATIHW->src_y_start, 0));
        outm(SRC_HEIGHT2_WIDTH2,
            SetWord(pATIHW->src_width2, 1) | SetWord(pATIHW->src_height2, 0));
        outm(SRC_CNTL, pATIHW->src_cntl);

        /* Load host data register */
        ATIMach64WaitForFIFO(1);
        outm(HOST_CNTL, pATIHW->host_cntl);

        /* Load pattern registers */
        ATIMach64WaitForFIFO(3);
        outm(PAT_REG0, pATIHW->pat_reg0);
        outm(PAT_REG1, pATIHW->pat_reg1);
        outm(PAT_CNTL, pATIHW->pat_cntl);

        /* Load scissor registers */
        ATIMach64WaitForFIFO(2);
        outm(SC_LEFT_RIGHT,
            SetWord(pATIHW->sc_right, 1) | SetWord(pATIHW->sc_left, 0));
        outm(SC_TOP_BOTTOM,
            SetWord(pATIHW->sc_bottom, 1) | SetWord(pATIHW->sc_top, 0));

        /* Load data path registers */
        ATIMach64WaitForFIFO(7);
        outm(DP_BKGD_CLR, pATIHW->dp_bkgd_clr);
        outm(DP_FRGD_CLR, pATIHW->dp_frgd_clr);
        outm(DP_WRITE_MASK, pATIHW->dp_write_mask);
        outm(DP_CHAIN_MASK, pATIHW->dp_chain_mask);
        outm(DP_PIX_WIDTH, pATIHW->dp_pix_width);
        outm(DP_MIX, pATIHW->dp_mix);
        outm(DP_SRC, pATIHW->dp_src);

        /* Load colour compare registers */
        ATIMach64WaitForFIFO(3);
        outm(CLR_CMP_CLR, pATIHW->clr_cmp_clr);
        outm(CLR_CMP_MSK, pATIHW->clr_cmp_msk);
        outm(CLR_CMP_CNTL, pATIHW->clr_cmp_cntl);

        /* Load context mask */
        ATIMach64WaitForFIFO(1);
        outm(CONTEXT_MASK, pATIHW->context_mask);

        ATIMach64WaitForIdle();
    }
}

/*
 * ATIMach64SaveScreen --
 *
 * This function blanks or unblanks a Mach64 screen.
 */
void
ATIMach64SaveScreen
(
    ATIPtr pATI,
    int    Mode
)
{
    CARD32 crtc_gen_cntl = inl(pATI->CPIO_CRTC_GEN_CNTL);

    switch (Mode)
    {
        case SCREEN_SAVER_OFF:
        case SCREEN_SAVER_FORCER:
            outl(pATI->CPIO_CRTC_GEN_CNTL, crtc_gen_cntl | CRTC_EN);
            break;

        case SCREEN_SAVER_ON:
        case SCREEN_SAVER_CYCLE:
            outl(pATI->CPIO_CRTC_GEN_CNTL, crtc_gen_cntl & ~CRTC_EN);
            break;

        default:
            break;
    }
}

/*
 * ATIMach64Sync --
 *
 * This is called to wait for the draw engine to become idle.
 */
static void
ATIMach64Sync
(
    ScrnInfoPtr pScreenInfo
)
{
    ATIPtr pATI = ATIPTR(pScreenInfo);

    ATIMach64WaitForIdle();

    /*
     * For VTB's and later, the first CPU read of the framebuffer will return
     * zeroes, so do it here.  This appears to be due to some kind of engine
     * caching of framebuffer data I haven't found any way of disabling, or
     * otherwise circumventing.  Thanks to Mark Vojkovich for the suggestion.
     */
    pATI = *(volatile ATIPtr *)pATI->pMemory;
}

/*
 * ATIMach64SetupForScreenToScreenCopy --
 *
 * This function sets up the draw engine for a series of screen-to-screen copy
 * operations.
 */
static void
ATIMach64SetupForScreenToScreenCopy
(
    ScrnInfoPtr  pScreenInfo,
    int          xdir,
    int          ydir,
    int          rop,
    unsigned int planemask,
    int          TransparencyColour
)
{
    ATIPtr pATI = ATIPTR(pScreenInfo);

    pATI->dst_cntl = 0;

    if (ydir > 0)
        pATI->dst_cntl |= DST_Y_DIR;
    if (xdir > 0)
        pATI->dst_cntl |= DST_X_DIR;

    if (pATI->PitchModifier == 1)
    {
        ATIMach64WaitForFIFO(4);
        outm(DST_CNTL, pATI->dst_cntl);
    }
    else
    {
        ATIMach64WaitForFIFO(3);
        pATI->dst_cntl |= DST_24_ROT_EN;
    }

    outm(DP_MIX, SetBits(ATIMach64ALU[rop], DP_FRGD_MIX));
    outm(DP_WRITE_MASK, planemask);
    outm(DP_SRC, SetBits(DP_MONO_SRC_ALLONES, DP_MONO_SRC) |
        SetBits(SRC_BLIT, DP_FRGD_SRC) | SetBits(SRC_BKGD, DP_BKGD_SRC));
}

/*
 * ATIMach64SubsequentScreenToScreenCopy --
 *
 * This function performs a screen-to-screen copy operation.
 */
static void
ATIMach64SubsequentScreenToScreenCopy
(
    ScrnInfoPtr pScreenInfo,
    int         xSrc,
    int         ySrc,
    int         xDst,
    int         yDst,
    int         w,
    int         h
)
{
    ATIPtr pATI = ATIPTR(pScreenInfo);

    xSrc *= pATI->PitchModifier;
    xDst *= pATI->PitchModifier;
    w    *= pATI->PitchModifier;

    if (!(pATI->dst_cntl & DST_X_DIR))
    {
        xSrc += w - 1;
        xDst += w - 1;
    }

    if (!(pATI->dst_cntl & DST_Y_DIR))
    {
        ySrc += h - 1;
        yDst += h - 1;
    }

    if (pATI->PitchModifier == 1)
        ATIMach64WaitForFIFO(4);
    else
    {
        ATIMach64WaitForFIFO(5);
        outm(DST_CNTL, pATI->dst_cntl | SetBits((xDst / 4) % 6, DST_24_ROT));
    }

    outm(SRC_Y_X, SetWord(xSrc, 1) | SetWord(ySrc, 0));
    outm(SRC_WIDTH1, w);
    outm(DST_Y_X, SetWord(xDst, 1) | SetWord(yDst, 0));
    outm(DST_HEIGHT_WIDTH, SetWord(w, 1) | SetWord(h, 0));
}

/*
 * ATIMach64SetupForSolidFill --
 *
 * This function sets up the draw engine for a series of solid fills.
 */
static void
ATIMach64SetupForSolidFill
(
    ScrnInfoPtr  pScreenInfo,
    int          colour,
    int          rop,
    unsigned int planemask
)
{
    ATIPtr pATI = ATIPTR(pScreenInfo);

    if (pATI->PitchModifier != 1)
        ATIMach64WaitForFIFO(4);
    else
    {
        ATIMach64WaitForFIFO(5);
        outm(DST_CNTL, pATI->NewHW.dst_cntl);
    }

    outm(DP_MIX, SetBits(ATIMach64ALU[rop], DP_FRGD_MIX));
    outm(DP_WRITE_MASK, planemask);
    outm(DP_SRC, SetBits(DP_MONO_SRC_ALLONES, DP_MONO_SRC) |
        SetBits(SRC_FRGD, DP_FRGD_SRC) | SetBits(SRC_BKGD, DP_BKGD_SRC));
    outm(DP_FRGD_CLR, colour);
}

/*
 * ATIMach64SubsequentSolidFillRect --
 *
 * This function performs a solid rectangle fill.
 */
static void
ATIMach64SubsequentSolidFillRect
(
    ScrnInfoPtr pScreenInfo,
    int         x,
    int         y,
    int         w,
    int         h
)
{
    ATIPtr pATI = ATIPTR(pScreenInfo);

    if (pATI->PitchModifier == 1)
       ATIMach64WaitForFIFO(2);
    else
    {
        x *= pATI->PitchModifier;
        w *= pATI->PitchModifier;

        ATIMach64WaitForFIFO(3);
        outm(DST_CNTL, pATI->NewHW.dst_cntl | DST_24_ROT_EN |
            SetBits((x / 4) % 6, DST_24_ROT));
    }

    outm(DST_Y_X, SetWord(x, 1) | SetWord(y, 0));
    outm(DST_HEIGHT_WIDTH, SetWord(w, 1) | SetWord(h, 0));
}

/*
 * ATIMach64SetClippingRectangle --
 *
 * This function sets the draw engine's clipping rectangle.
 */
static void
ATIMach64SetClippingRectangle
(
    ScrnInfoPtr pScreenInfo,
    int         left,
    int         top,
    int         right,
    int         bottom
)
{
    ATIPtr pATI = ATIPTR(pScreenInfo);

    ATIMach64WaitForFIFO(2);
    outm(SC_LEFT_RIGHT, SetWord(((right + 1) * pATI->PitchModifier) - 1, 1) |
        SetWord(left * pATI->PitchModifier, 0));
    outm(SC_TOP_BOTTOM, SetWord(bottom, 1) | SetWord(top, 0));
}

/*
 * ATIMach64DisableClipping --
 *
 * This function resets the draw engine's clipping rectangle to include the
 * entire virtual resolution.
 */
static void
ATIMach64DisableClipping
(
    ScrnInfoPtr pScreenInfo
)
{
    ATIPtr pATI = ATIPTR(pScreenInfo);

    ATIMach64WaitForFIFO(2);
    outm(SC_LEFT_RIGHT,
        SetWord(pATI->NewHW.sc_right, 1) | SetWord(pATI->NewHW.sc_left, 0));
    outm(SC_TOP_BOTTOM,
        SetWord(pATI->NewHW.sc_bottom, 1) | SetWord(pATI->NewHW.sc_top, 0));
}

/*
 * ATIMach64AccelInit --
 *
 * This function fills in structure fields needed for acceleration on Mach64
 * variants.
 */
Bool
ATIMach64AccelInit
(
    ScrnInfoPtr   pScreenInfo,
    ScreenPtr     pScreen,
    ATIPtr        pATI,
    XAAInfoRecPtr pXAAInfo
)
{
    /* This doesn't seem quite right... */
    if (pATI->PitchModifier == 1)
    {
        pXAAInfo->Flags = PIXMAP_CACHE | OFFSCREEN_PIXMAPS;
        if (!pATI->BankInfo.BankSize)
            pXAAInfo->Flags |= LINEAR_FRAMEBUFFER;
    }

    /* Sync */
    pXAAInfo->Sync = ATIMach64Sync;

    /* Screen-to-screen copy */
    pXAAInfo->ScreenToScreenCopyFlags = NO_TRANSPARENCY;        /* For now */
    pXAAInfo->SetupForScreenToScreenCopy = ATIMach64SetupForScreenToScreenCopy;
    pXAAInfo->SubsequentScreenToScreenCopy =
        ATIMach64SubsequentScreenToScreenCopy;

    /* Solid fills */
    pXAAInfo->SetupForSolidFill = ATIMach64SetupForSolidFill;
    pXAAInfo->SubsequentSolidFillRect = ATIMach64SubsequentSolidFillRect;

    /* Clips */
    pXAAInfo->ClippingFlags = HARDWARE_CLIP_SCREEN_TO_SCREEN_COLOR_EXPAND |
        HARDWARE_CLIP_SCREEN_TO_SCREEN_COPY | HARDWARE_CLIP_MONO_8x8_FILL |
        HARDWARE_CLIP_COLOR_8x8_FILL | HARDWARE_CLIP_SOLID_FILL |
        HARDWARE_CLIP_DASHED_LINE | HARDWARE_CLIP_SOLID_LINE;
    pXAAInfo->SetClippingRectangle = ATIMach64SetClippingRectangle;
    pXAAInfo->DisableClipping = ATIMach64DisableClipping;

    return TRUE;
}
