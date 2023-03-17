/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atidsp.c,v 1.8 2000/02/18 12:19:21 tsi Exp $ */
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

#include "atichip.h"
#include "aticrtc.h"
#include "atidsp.h"
#include "atiio.h"
#include "atividmem.h"
#include "xf86.h"

/*
 * ATIDSPPreInit --
 *
 * This function initialises global variables used to set DSP registers on a
 * VT-B or later.
 */
Bool
ATIDSPPreInit
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    CARD32 IOValue, dsp_config, dsp_on_off, vga_dsp_config, vga_dsp_on_off;
    CARD16 CPIO_VGA_DSP_CONFIG, CPIO_VGA_DSP_ON_OFF;
    int trp;

    /* Set DSP register port numbers */
    pATI->CPIO_DSP_CONFIG = ATIIOPort(DSP_CONFIG);
    pATI->CPIO_DSP_ON_OFF = ATIIOPort(DSP_ON_OFF);
    CPIO_VGA_DSP_CONFIG = ATIIOPort(VGA_DSP_CONFIG);
    CPIO_VGA_DSP_ON_OFF = ATIIOPort(VGA_DSP_ON_OFF);

    /*
     * VT-B's and later have additional post-dividers that are not powers of
     * two.
     */
    pATI->ClockDescriptor.NumD = 8;

    /* Retrieve XCLK settings */
    IOValue = ATIGetMach64PLLReg(PLL_XCLK_CNTL);
    pATI->XCLKPostDivider = GetBits(IOValue, PLL_XCLK_SRC_SEL);
    pATI->XCLKReferenceDivider = 1;
    switch (pATI->XCLKPostDivider)
    {
        case 0:  case 1:  case 2:  case 3:
            break;

        case 4:
            pATI->XCLKReferenceDivider = 3;
            pATI->XCLKPostDivider = 0;
            break;

        default:
            xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
                "Unsupported XCLK source:  %d.\n", pATI->XCLKPostDivider);
            return FALSE;
    }

    pATI->XCLKPostDivider -= GetBits(IOValue, PLL_MFB_TIMES_4_2B);
    pATI->XCLKFeedbackDivider = ATIGetMach64PLLReg(PLL_MCLK_FB_DIV);

    /* Compute maximum RAS delay and friends */
    IOValue = inl(pATI->CPIO_MEM_INFO);
    trp = GetBits(IOValue, CTL_MEM_TRP);
    pATI->XCLKPageFaultDelay = GetBits(IOValue, CTL_MEM_TRCD) +
        GetBits(IOValue, CTL_MEM_TCRD) + trp + 2;
    pATI->XCLKMaxRASDelay = GetBits(IOValue, CTL_MEM_TRAS) + trp + 2;
    pATI->DisplayFIFODepth = 32;

    if (pATI->Chip < ATI_CHIP_264VT4)
    {
        pATI->XCLKPageFaultDelay += 2;
        pATI->XCLKMaxRASDelay += 3;
        pATI->DisplayFIFODepth = 24;
    }

    switch (pATI->MemoryType)
    {
        case MEM_264_DRAM:
            if (pATI->VideoRAM <= 1024)
                pATI->DisplayLoopLatency = 10;
            else
            {
                pATI->DisplayLoopLatency = 8;
                pATI->XCLKPageFaultDelay += 2;
            }
            break;

        case MEM_264_EDO:
        case MEM_264_PSEUDO_EDO:
            if (pATI->VideoRAM <= 1024)
                pATI->DisplayLoopLatency = 9;
            else
            {
                pATI->DisplayLoopLatency = 8;
                pATI->XCLKPageFaultDelay++;
            }
            break;

        case MEM_264_SDRAM:
            if (pATI->VideoRAM <= 1024)
                pATI->DisplayLoopLatency = 11;
            else
            {
                pATI->DisplayLoopLatency = 10;
                pATI->XCLKPageFaultDelay++;
            }
            break;

        case MEM_264_SGRAM:
            pATI->DisplayLoopLatency = 8;
            pATI->XCLKPageFaultDelay += 3;
            break;

        default:                /* Set maximums */
            pATI->DisplayLoopLatency = 11;
            pATI->XCLKPageFaultDelay += 3;
            break;
    }

    if (pATI->XCLKMaxRASDelay <= pATI->XCLKPageFaultDelay)
        pATI->XCLKMaxRASDelay = pATI->XCLKPageFaultDelay + 1;

    /* Allow BIOS to override */
    dsp_config = inl(pATI->CPIO_DSP_CONFIG);
    dsp_on_off = inl(pATI->CPIO_DSP_ON_OFF);
    vga_dsp_config = inl(CPIO_VGA_DSP_CONFIG);
    vga_dsp_on_off = inl(CPIO_VGA_DSP_ON_OFF);

    if (dsp_config)
        pATI->DisplayLoopLatency = GetBits(dsp_config, DSP_LOOP_LATENCY);

    if ((dsp_on_off == vga_dsp_on_off) &&
        (!dsp_config || !((dsp_config ^ vga_dsp_config) & DSP_XCLKS_PER_QW)))
    {
        if (ATIDivide(GetBits(vga_dsp_on_off, VGA_DSP_OFF),
                      GetBits(vga_dsp_config, VGA_DSP_XCLKS_PER_QW), 5, 1) > 23)
            pATI->DisplayFIFODepth = 32;
        else
            pATI->DisplayFIFODepth = 24;
    }

    return TRUE;
}

/*
 * ATIDSPSave --
 *
 * This function is called to remember DSP register values on VT-B and later
 * controllers.
 */
void
ATIDSPSave
(
    ATIPtr   pATI,
    ATIHWPtr pATIHW
)
{
    pATIHW->dsp_on_off = inl(pATI->CPIO_DSP_ON_OFF);
    pATIHW->dsp_config = inl(pATI->CPIO_DSP_CONFIG);
}


/*
 * ATIDSPCalculate --
 *
 * This function sets up DSP register values for a VTB or later.  Note that
 * this would be slightly different if VCLK 0 or 1 were used for the mode
 * instead.  In that case, this function would set VGA_DSP_CONFIG and
 * VGA_DSP_ON_OFF, would have to zero out DSP_CONFIG and DSP_ON_OFF, and would
 * have to consider that VGA_DSP_CONFIG is partitioned slightly differently
 * than DSP_CONFIG.
 */
void
ATIDSPCalculate
(
    ScrnInfoPtr    pScreenInfo,
    ATIPtr         pATI,
    ATIHWPtr       pATIHW,
    DisplayModePtr pMode
)
{
    int Multiplier, Divider;
    int RASMultiplier = pATI->XCLKMaxRASDelay, RASDivider = 1;
    int dsp_precision, dsp_on, dsp_off, dsp_xclks;
    int tmp, vshift, xshift;

#   define Maximum_DSP_PRECISION ((int)MaxBits(DSP_PRECISION))

    /* Compute a memory-to-screen bandwidth ratio */
    Multiplier = pATI->XCLKFeedbackDivider *
        pATI->ClockDescriptor.PostDividers[pATIHW->PostDivider];
    Divider = pATIHW->FeedbackDivider * pATI->XCLKReferenceDivider;
    if (pScreenInfo->depth >= 8)
        Divider *= pScreenInfo->bitsPerPixel / 4;
    /* Start by assuming a display FIFO width of 32 bits */
    vshift = (5 - 2) - pATI->XCLKPostDivider;
    if (pATIHW->crtc != ATI_CRTC_VGA)
        vshift++;               /* Nope, it's 64 bits wide */

    if (!pATI->OptionCRT && (pATI->LCDPanelID >= 0))
    {
        /* Compensate for horizontal stretching */
        Multiplier *= pATI->LCDHorizontal;
        Divider *= pMode->HDisplay & ~7;

        RASMultiplier *= pATI->LCDHorizontal;
        RASDivider *= pMode->HDisplay & ~7;
    }

    /* Determine dsp_precision first */
    tmp = ATIDivide(Multiplier * pATI->DisplayFIFODepth, Divider, vshift, 1);
    for (dsp_precision = -5;  tmp;  dsp_precision++)
        tmp >>= 1;
    if (dsp_precision < 0)
        dsp_precision = 0;
    else if (dsp_precision > Maximum_DSP_PRECISION)
        dsp_precision = Maximum_DSP_PRECISION;

    xshift = 6 - dsp_precision;
    vshift += xshift;

    /* Move on to dsp_off */
    dsp_off = ATIDivide(Multiplier * (pATI->DisplayFIFODepth - 1), Divider,
        vshift, -1) - ATIDivide(1, 1, vshift - xshift, 1);

    /* Next is dsp_on */
    if ((pATIHW->crtc == ATI_CRTC_VGA) /* && (dsp_precision < 3) */)
    {
        /*
         * TODO:  I don't yet know why something like this appears necessary.
         *        But I don't have time to explore this right now.
         */
        dsp_on = ATIDivide(Multiplier * 5, Divider, vshift + 2, 1);
    }
    else
    {
        dsp_on = ATIDivide(Multiplier, Divider, vshift, 1);
        tmp = ATIDivide(RASMultiplier, RASDivider, xshift, 1);
        if (dsp_on < tmp)
            dsp_on = tmp;
        dsp_on += (tmp * 2) +
            ATIDivide(pATI->XCLKPageFaultDelay, 1, xshift, 1);
    }

    /* Last but not least:  dsp_xclks */
    dsp_xclks = ATIDivide(Multiplier, Divider, vshift + 5, 1);

    /* Build DSP register contents */
    pATIHW->dsp_on_off = SetBits(dsp_on, DSP_ON) |
        SetBits(dsp_off, DSP_OFF);
    pATIHW->dsp_config = SetBits(dsp_precision, DSP_PRECISION) |
        SetBits(dsp_xclks, DSP_XCLKS_PER_QW) |
        SetBits(pATI->DisplayLoopLatency, DSP_LOOP_LATENCY);
}

/*
 * ATIDSPSet --
 *
 * This function is called to set DSP registers on VT-B and later controllers.
 */
void
ATIDSPSet
(
    ATIPtr   pATI,
    ATIHWPtr pATIHW
)
{
    outl(pATI->CPIO_DSP_ON_OFF, pATIHW->dsp_on_off);
    outl(pATI->CPIO_DSP_CONFIG, pATIHW->dsp_config);
}
