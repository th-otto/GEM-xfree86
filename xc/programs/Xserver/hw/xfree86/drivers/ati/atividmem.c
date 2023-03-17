/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atividmem.c,v 1.8 2000/02/18 12:19:44 tsi Exp $ */
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

#include "ati.h"
#include "atiadapter.h"
#include "atistruct.h"
#include "atividmem.h"

/* Memory types for 68800's and 88800GX's */
const char *ATIMemoryTypeNames_Mach[] =
{
    "DRAM (256Kx4)",
    "VRAM (256Kx4, x8, x16)",
    "VRAM (256Kx16 with short shift register)",
    "DRAM (256Kx16)",
    "Graphics DRAM (256Kx16)",
    "Enhanced VRAM (256Kx4, x8, x16)",
    "Enhanced VRAM (256Kx16 with short shift register)",
    "Unknown video memory type"
};

/* Memory types for 88800CX's */
const char *ATIMemoryTypeNames_88800CX[] =
{
    "DRAM (256Kx4, x8, x16)",
    "EDO DRAM (256Kx4, x8, x16)",
    "Unknown video memory type",
    "DRAM (256Kx16 with assymetric RAS/CAS)",
    "Unknown video memory type",
    "Unknown video memory type",
    "Unknown video memory type",
    "Unknown video memory type"
};

/* Memory types for 264xT's */
const char *ATIMemoryTypeNames_264xT[] =
{
    "Disabled video memory",
    "DRAM",
    "EDO DRAM",
    "Pseudo-EDO DRAM",
    "SDRAM (1:1)",
    "SGRAM (1:1)",
    "SGRAM (2:1) 32-bit",
    "Unknown video memory type"
};

/*
 * ATIUnmapVGA --
 *
 * Unmap VGA aperture.
 */
static void
ATIUnmapVGA
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    if (!pATI->pBank)
        return;

    xf86UnMapVidMem(pScreenInfo->scrnIndex, pATI->pBank, 0x00010000U);

    pATI->pBank = pATI->BankInfo.pBankA = pATI->BankInfo.pBankB = NULL;
}

/*
 * ATIUnmapLinear --
 *
 * Unmap linear aperture.
 */
static void
ATIUnmapLinear
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    if (pATI->pMemory != pATI->pBank)
        xf86UnMapVidMem(pScreenInfo->scrnIndex, pATI->pMemory,
            pATI->LinearSize);

    pATI->pMemory = NULL;
}

/*
 * ATIUnmapMMIO --
 *
 * Unmap MMIO registers.
 */
static void
ATIUnmapMMIO
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    if (pATI->pMMIO)
        xf86UnMapVidMem(pScreenInfo->scrnIndex, pATI->pMMIO, pATI->PageSize);

    pATI->pMMIO = pATI->pBlock[0] = pATI->pBlock[1] = NULL;
}

/*
 * ATIMapApertures --
 *
 * This function maps all apertures used by the driver.
 */
Bool
ATIMapApertures
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    pciVideoPtr pVideo;

    if (pATI->Mapped)
        return TRUE;

    /* Map VGA aperture */
    if (pATI->VGAAdapter != ATI_ADAPTER_NONE)
    {
        /*
         * No relocation, resizing, caching or write-combining of this
         * aperture is supported.  Hence, the hard-coded values here...
         */
        pATI->pBank = xf86MapVidMem(pScreenInfo->scrnIndex, VIDMEM_MMIO,
            0x000A0000U, 0x00010000U);

        if (!pATI->pBank)
            return FALSE;

        pATI->pMemory =
            pATI->BankInfo.pBankA =
            pATI->BankInfo.pBankB = pATI->pBank;
    }

    pVideo = pATI->PCIInfo;

    /* Map linear aperture */
    if (pATI->LinearBase)
    {
        if (pVideo)
            pATI->pMemory = xf86MapPciMem(pScreenInfo->scrnIndex,
                VIDMEM_FRAMEBUFFER, ((pciConfigPtr)(pVideo->thisCard))->tag,
                pATI->LinearBase, pATI->LinearSize);
        else
            pATI->pMemory = xf86MapVidMem(pScreenInfo->scrnIndex,
                VIDMEM_FRAMEBUFFER, pATI->LinearBase, pATI->LinearSize);

        if (!pATI->pMemory)
        {
            ATIUnmapVGA(pScreenInfo, pATI);
            return FALSE;
        }
    }

    /* Map MMIO aperture */
    if (pATI->Block0Base)
    {
        if ((pATI->Block0Base >= pATI->LinearBase) &&
            ((pATI->Block0Base + 0x00000400U) <=
             (pATI->LinearBase + pATI->LinearSize)))
        {
            pATI->pBlock[0] = (char *)pATI->pMemory +
                (pATI->Block0Base - pATI->LinearBase);
        }
        else
        {
            if (pVideo &&
                ((pATI->Block0Base < 0x000A0000U) ||
                 (pATI->Block0Base > (0x000B0000U - 0x00000400U))))
                pATI->pMMIO = xf86MapPciMem(pScreenInfo->scrnIndex,
                    VIDMEM_MMIO, ((pciConfigPtr)(pVideo->thisCard))->tag,
                    pATI->MMIOBase, pATI->PageSize);
            else
                pATI->pMMIO = xf86MapVidMem(pScreenInfo->scrnIndex,
                    VIDMEM_MMIO, pATI->MMIOBase, pATI->PageSize);

            if (!pATI->pMMIO)
            {
                ATIUnmapLinear(pScreenInfo, pATI);
                ATIUnmapVGA(pScreenInfo, pATI);
                return FALSE;
            }

            pATI->pBlock[0] = (char *)pATI->pMMIO +
                (pATI->Block0Base - pATI->MMIOBase);
        }

        if (pATI->Block1Base)
            pATI->pBlock[1] = (char *)pATI->pBlock[0] - 0x00000400U;
    }

    pATI->Mapped = TRUE;
    return TRUE;
}

/*
 * ATIUnmapApertures --
 *
 * This function unmaps all apertures used by the driver.
 */
void
ATIUnmapApertures
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    if (!pATI->Mapped)
        return;
    pATI->Mapped = FALSE;

    /* Unmap MMIO area */
    ATIUnmapMMIO(pScreenInfo, pATI);

    /* Unmap linear aperture */
    ATIUnmapLinear(pScreenInfo, pATI);

    /* Unmap VGA aperture */
    ATIUnmapVGA(pScreenInfo, pATI);
}
