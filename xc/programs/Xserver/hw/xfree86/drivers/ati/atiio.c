/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atiio.c,v 1.4 2000/02/18 12:19:23 tsi Exp $ */
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
#include "atiio.h"

/*
 * ATISetVGAIOBase --
 *
 * This sets vgaIOBase according to the value of the passed value of the
 * miscellaneous output register.
 */
void
ATISetVGAIOBase
(
    ATIPtr      pATI,
    const CARD8 misc
)
{
    pATI->CPIO_VGABase = (misc & 0x01U) ? ColourIOBase : MonochromeIOBase;
}

/*
 * ATIModifyExtReg --
 *
 * This function is called to modify certain bits in an ATI extended VGA
 * register while preserving its other bits.  The function will not write the
 * register if it turns out its value would not change.  This helps prevent
 * server hangs on older adapters.
 */
void
ATIModifyExtReg
(
    ATIPtr      pATI,
    const CARD8 Index,
    int         CurrentValue,
    const CARD8 CurrentMask,
    CARD8       NewValue
)
{
    /* Possibly retrieve the current value */
    if (CurrentValue < 0)
        CurrentValue = ATIGetExtReg(Index);

    /* Compute new value */
    NewValue &= (CARD8)(~CurrentMask);
    NewValue |= CurrentValue & CurrentMask;

    /* Check if value will be changed */
    if (CurrentValue == NewValue)
        return;

    /*
     * The following is taken from ATI's VGA Wonder programmer's reference
     * manual which says that this is needed to "ensure the proper state of the
     * 8/16 bit ROM toggle".  I suspect a timing glitch appeared in the 18800
     * after its die was cast.  18800-1 and later chips do not exhibit this
     * problem.
     */
    if ((pATI->Chip <= ATI_CHIP_18800) && (Index == 0xB2U) &&
       ((NewValue ^ 0x40U) & CurrentValue & 0x40U))
    {
        CARD8 misc = inb(R_GENMO);
        CARD8 bb = ATIGetExtReg(0xBBU);

        outb(GENMO, (misc & 0xF3U) | 0x04U | ((bb & 0x10U) >> 1));
        CurrentValue &= (CARD8)(~0x40U);
        ATIPutExtReg(0xB2U, CurrentValue);
        ATIDelay(5);
        outb(GENMO, misc);
        ATIDelay(5);
        if (CurrentValue != NewValue)
            ATIPutExtReg(0xB2U, NewValue);
    }
    else
        ATIPutExtReg(Index, NewValue);
}

/*
 * ATIAccessMach64PLLReg --
 *
 * This function sets up the addressing required to access, for read or write,
 * a 264xT's PLL registers.
 */
void
ATIAccessMach64PLLReg
(
    ATIPtr      pATI,
    const CARD8 Index,
    const Bool  Write
)
{
    CARD8 clock_cntl1 = inb(pATI->CPIO_CLOCK_CNTL + 1) &
        ~GetByte(PLL_WR_EN | PLL_ADDR, 1);

    /* Set PLL register to be read or written */
    outb(pATI->CPIO_CLOCK_CNTL + 1, clock_cntl1 |
        GetByte(SetBits(Index, PLL_ADDR) | SetBits(Write, PLL_WR_EN), 1));
}
