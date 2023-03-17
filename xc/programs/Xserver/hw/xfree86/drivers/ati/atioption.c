/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atioption.c,v 1.6 2000/02/18 12:19:27 tsi Exp $ */
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

#include "ati.h"
#include "atiadapter.h"
#include "atibus.h"
#include "atioption.h"
#include "atistruct.h"
#include "atiutil.h"

/*
 * Recognised XF86Config options.
 */
typedef enum
{
    ATI_OPTION_ACCEL,
    ATI_OPTION_CRT,
    ATI_OPTION_CSYNC,
    ATI_OPTION_LINEAR,
    ATI_OPTION_PROBE_CLOCKS,
    ATI_OPTION_SHADOW_FB
} ATIPublicOptionType;

typedef enum
{
    ATI_OPTION_DEVEL,   /* Intentionally undocumented */
    ATI_OPTION_SYNC     /* Temporary and undocumented */
} ATIPrivateOptionType;

static OptionInfoRec ATIPublicOptions[] =
{
    {ATI_OPTION_ACCEL,        "accel",          OPTV_BOOLEAN, {0, }, FALSE},
    {ATI_OPTION_CRT,          "crt_screen",     OPTV_BOOLEAN, {0, }, FALSE},
    {ATI_OPTION_CSYNC,        "composite_sync", OPTV_BOOLEAN, {0, }, FALSE},
    {ATI_OPTION_LINEAR,       "linear",         OPTV_BOOLEAN, {0, }, FALSE},
    {ATI_OPTION_PROBE_CLOCKS, "probe_clocks",   OPTV_BOOLEAN, {0, }, FALSE},
    {ATI_OPTION_SHADOW_FB,    "shadow_fb",      OPTV_BOOLEAN, {0, }, FALSE},
    {-1,                      NULL,             OPTV_NONE   , {0, }, FALSE}
};

/*
 * ATIAvailableOptions --
 *
 * Return recognised options that are intended for public consumption.
 */
OptionInfoPtr
ATIAvailableOptions
(
    int ChipId,          /* Ignored */
    int BusId            /* Ignored */
)
{
    return ATIPublicOptions;
}

/*
 * ATIProcessOptions --
 *
 * This function extracts options from what was parsed out of the XF86Config
 * file.
 */
void
ATIProcessOptions
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    OptionInfoRec PublicOption[NumberOf(ATIPublicOptions)];
    OptionInfoRec PrivateOption[] =
    {
        {ATI_OPTION_DEVEL,        "tsi",            OPTV_BOOLEAN, {0, }, FALSE},
        {ATI_OPTION_SYNC,         "lcdsync",        OPTV_BOOLEAN, {0, }, FALSE},
        {-1,                      NULL,             OPTV_NONE   , {0, }, FALSE}
    };

    memcpy(PublicOption, ATIPublicOptions, SizeOf(ATIPublicOptions));

#   define Accel       PublicOption[ATI_OPTION_ACCEL].value.bool
#   define CRTScreen   PublicOption[ATI_OPTION_CRT].value.bool
#   define CSync       PublicOption[ATI_OPTION_CSYNC].value.bool
#   define Devel       PrivateOption[ATI_OPTION_DEVEL].value.bool
#   define Linear      PublicOption[ATI_OPTION_LINEAR].value.bool
#   define ProbeClocks PublicOption[ATI_OPTION_PROBE_CLOCKS].value.bool
#   define ShadowFB    PublicOption[ATI_OPTION_SHADOW_FB].value.bool
#   define Sync        PrivateOption[ATI_OPTION_SYNC].value.bool

    /* Pick up XF86Config options */
    xf86CollectOptions(pScreenInfo, NULL);

    /* Set non-zero defaults */
    if (pATI->Adapter >= ATI_ADAPTER_MACH64)
        Accel = Linear = TRUE;
    if (pATI->BusType >= ATI_BUS_PCI)
        ShadowFB = TRUE;
    Sync = TRUE;

    xf86ProcessOptions(pScreenInfo->scrnIndex, pScreenInfo->options,
        PublicOption);
    xf86ProcessOptions(pScreenInfo->scrnIndex, pScreenInfo->options,
        PrivateOption);

    /* Disable linear apertures if the OS doesn't support them */
    if (!xf86LinearVidMem() && Linear)
    {
        if (PublicOption[ATI_OPTION_LINEAR].found)
            xf86DrvMsg(pScreenInfo->scrnIndex, X_WARNING,
                "OS does not support linear apertures.\n");
        Linear = FALSE;
    }

    /* Move option values into driver private structure */
    pATI->OptionAccel = Accel;
    pATI->OptionCRT = CRTScreen;
    pATI->OptionCSync = CSync;
    pATI->OptionDevel = Devel;
    pATI->OptionLinear = Linear;
    pATI->OptionProbeClocks = ProbeClocks;
    pATI->OptionShadowFB = ShadowFB;
    pATI->OptionSync = Sync;
}
