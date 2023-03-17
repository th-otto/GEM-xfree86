/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atiadjust.c,v 1.5 2000/02/18 12:19:12 tsi Exp $ */
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

#include "atiadjust.h"
#include "atichip.h"
#include "aticrtc.h"
#include "atiio.h"
#include "atilock.h"
#include "xf86.h"

/*
 * The display start address is expressed in units of 32-bit (VGA) or 64-bit
 * (accelerator) words where all planar modes are considered as 4bpp modes.
 * These functions ensure the start address does not exceed architectural
 * limits.  Also, to avoid colour changes while panning, these 32-bit or 64-bit
 * boundaries may not fall within a pixel.
 */

/*
 * ATIAjustPreInit  --
 *
 * This function calculates values needed to speed up the setting of the
 * display start address.
 */
void
ATIAdjustPreInit
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    DisplayModePtr pMode;
    unsigned long  MaxBase = 0;
    int            MinX, MinY;

    if ((pATI->CPIO_VGAWonder) &&
        (pATI->Chip <= ATI_CHIP_18800_1) &&
        (pATI->VideoRAM == 256) &&
        (pScreenInfo->depth >= 8))
    {
        /* Strange, to say the least ... */
        pATI->AdjustDepth = (pScreenInfo->bitsPerPixel + 3) >> 2;
        pATI->AdjustMask = (unsigned long)(-32);
    }
    else
    {
        pATI->AdjustDepth = (pScreenInfo->bitsPerPixel + 7) >> 3;

        pATI->AdjustMask = 64;
        while (pATI->AdjustMask % (unsigned long)(pATI->AdjustDepth))
            pATI->AdjustMask += 64;
        pATI->AdjustMask =
            ~(((pATI->AdjustMask / (unsigned long)(pATI->AdjustDepth)) >> 3) -
              1);
    }

    switch (pATI->NewHW.crtc)
    {
        case ATI_CRTC_VGA:
            if (pATI->Chip >= ATI_CHIP_264CT)
            {
                MaxBase = MaxBits(CRTC_OFFSET_VGA) << 2;
                if (pScreenInfo->depth <= 4)
                    MaxBase <<= 1;
            }
            else if (!pATI->CPIO_VGAWonder)
                MaxBase = 0xFFFFU << 3;
            else if (pATI->Chip <= ATI_CHIP_28800_6)
                MaxBase = 0x03FFFFU << 3;
            else /* Mach32 & Mach64 */
                MaxBase = 0x0FFFFFU << 3;
            break;

        case ATI_CRTC_MACH64:
            MaxBase = MaxBits(CRTC_OFFSET) << 3;
            break;
    }

    MaxBase = (MaxBase / (unsigned long)pATI->AdjustDepth) | ~pATI->AdjustMask;

    pATI->AdjustMaxX = MaxBase % pScreenInfo->displayWidth;
    pATI->AdjustMaxY = MaxBase / pScreenInfo->displayWidth;

    /*
     * Warn about modes that are too small, or not aligned, to scroll to the
     * bottom right corner of the virtual screen.
     */
    MinX = pScreenInfo->virtualX - pATI->AdjustMaxX;
    MinY = pScreenInfo->virtualY - pATI->AdjustMaxY;

    pMode = pScreenInfo->modes;
    do
    {
        if ((pMode->VDisplay <= MinY) &&
            ((pMode->VDisplay < MinY) || (pMode->HDisplay < MinX)))
            xf86DrvMsg(pScreenInfo->scrnIndex, X_WARNING,
                "Mode \"%s\" too small to scroll to bottom right corner of"
                " virtual resolution.\n", pMode->name);
        else if ((pMode->HDisplay & ~pATI->AdjustMask) / pScreenInfo->xInc)
            xf86DrvMsg(pScreenInfo->scrnIndex, X_WARNING,
                "Mode \"%s\" cannot scroll to bottom right corner of virtual"
                " resolution.\n Horizontal dimension not a multiple of %d.\n",
                pMode->name, ~pATI->AdjustMask + 1);
    } while ((pMode = pMode->next) != pScreenInfo->modes);
}

/*
 * ATIAdjustFrame --
 *
 * This function is used to initialise the SVGA Start Address - the first
 * displayed location in video memory.  This is used to implement the virtual
 * window.
 */
void
ATIAdjustFrame
(
    int scrnIndex,
    int x,
    int y,
    int flags
)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
    ATIPtr      pATI = ATIPTR(pScreenInfo);
    int         Base;

    /*
     * Assume the caller has already done its homework in ensuring the physical
     * screen is still contained in the virtual resolution.
     */
    if (y >= pATI->AdjustMaxY)
    {
        y = pATI->AdjustMaxY;
        if (x > pATI->AdjustMaxX)
            y--;
    }

    Base = ((((y * pScreenInfo->displayWidth) + x) & pATI->AdjustMask) *
            pATI->AdjustDepth) >> 3;

    /* Unlock registers */
    ATIUnlock(pATI);

    if ((pATI->NewHW.crtc == ATI_CRTC_VGA) && (pATI->Chip < ATI_CHIP_264CT))
    {
        PutReg(CRTX(pATI->CPIO_VGABase), 0x0CU, GetByte(Base, 1));
        PutReg(CRTX(pATI->CPIO_VGABase), 0x0DU, GetByte(Base, 0));

        if (pATI->CPIO_VGAWonder)
        {
            if (pATI->Chip <= ATI_CHIP_18800_1)
                ATIModifyExtReg(pATI, 0xB0U, -1, 0x3FU, Base >> 10);
            else
            {
                ATIModifyExtReg(pATI, 0xB0U, -1, 0xBFU, Base >> 10);
                ATIModifyExtReg(pATI, 0xA3U, -1, 0xEFU, Base >> 13);

                /*
                 * I don't know if this also applies to Mach64's, but give it a
                 * shot...
                 */
                if (pATI->Chip >= ATI_CHIP_68800)
                    ATIModifyExtReg(pATI, 0xADU, -1, 0xF3U, Base >> 16);
            }
        }
    }
    else
    {
        /*
         * On integrated controllers, there is only one set of CRTC control
         * bits, many of which are simultaneously accessible through both VGA
         * and accelerator I/O ports.  Given VGA's architectural limitations,
         * setting the CRTC's offset register to more than 256k needs to be
         * done through the accelerator port.
         */
        if (pScreenInfo->depth <= 4)
        {
            outl(pATI->CPIO_CRTC_OFF_PITCH,
                SetBits(pScreenInfo->displayWidth >> 4, CRTC_PITCH) |
                    SetBits(Base, CRTC_OFFSET));
        }
        else
        {
            if (pATI->NewHW.crtc == ATI_CRTC_VGA)
                Base <<= 1;                     /* LSBit must be zero */
            outl(pATI->CPIO_CRTC_OFF_PITCH,
                SetBits(pScreenInfo->displayWidth >> 3, CRTC_PITCH) |
                    SetBits(Base, CRTC_OFFSET));
        }
    }
}
