/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atiio.h,v 1.6 2000/03/01 16:00:57 tsi Exp $ */
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

#ifndef ___ATIIO_H___
#define ___ATIIO_H___ 1

#ifndef NO_COMPILER_H_EXTRAS
#define NO_COMPILER_H_EXTRAS
#endif

#include "atiregs.h"
#include "atistruct.h"
#include "compiler.h"

/* I/O decoding definitions */
typedef enum
{
    SPARSE_IO,
    BLOCK_IO
} ATIIODecodingType;

#define ATIIOPort(_PortTag)                                                \
    (((pATI->CPIODecoding == SPARSE_IO) ?                                  \
      (((_PortTag) & SPARSE_IO_SELECT) | ((_PortTag) & IO_BYTE_SELECT)) :  \
      (((_PortTag) & BLOCK_IO_SELECT)  | ((_PortTag) & IO_BYTE_SELECT))) | \
     pATI->CPIOBase)

extern void ATISetVGAIOBase FunctionPrototype((ATIPtr, const CARD8));
extern void ATIModifyExtReg FunctionPrototype((ATIPtr, const CARD8, int,
                                               const CARD8, CARD8));

/* Odds and ends to ease reading and writting of registers */
#define GetReg(_Register, _Index) \
    (                             \
        outb(_Register, _Index),  \
        inb((_Register) + 1)      \
    )
#define PutReg(_Register, _Index, _Value) \
    (                                     \
        outb(_Register, _Index),          \
        outb((_Register) + 1, _Value)     \
    )

#define ATIGetExtReg(_Index)                    \
    GetReg(pATI->CPIO_VGAWonder, _Index)
#define ATIPutExtReg(_Index, _Value)            \
    PutReg(pATI->CPIO_VGAWonder, _Index, _Value)

extern void ATIAccessMach64PLLReg FunctionPrototype((ATIPtr, const CARD8,
                                                     const Bool));

#define ATIGetMach64PLLReg(_Index)                  \
    (                                               \
        ATIAccessMach64PLLReg(pATI, _Index, FALSE), \
        inb(pATI->CPIO_CLOCK_CNTL + 2)              \
    )
#define ATIPutMach64PLLReg(_Index, _Value)          \
    (                                               \
        ATIAccessMach64PLLReg(pATI, _Index, TRUE),  \
        outb(pATI->CPIO_CLOCK_CNTL + 2, _Value)     \
    )

#define ATIGetLTProLCDReg(_Index)                                     \
    (                                                                 \
        outb(pATI->CPIO_LCD_INDEX, SetBits((_Index), LCD_REG_INDEX)), \
        inl(pATI->CPIO_LCD_DATA)                                      \
    )
#define ATIPutLTProLCDReg(_Index, _Value)                             \
    (                                                                 \
        outb(pATI->CPIO_LCD_INDEX, SetBits((_Index), LCD_REG_INDEX)), \
        outl(pATI->CPIO_LCD_DATA, (_Value))                           \
    )

#define ATIGetLTProTVReg(_Index)                                        \
    (                                                                   \
        outb(pATI->CPIO_TV_OUT_INDEX, SetBits((_Index), TV_REG_INDEX)), \
        inl(pATI->CPIO_TV_OUT_DATA)                                     \
    )
#define ATIPutLTProTVReg(_Index, _Value)                                \
    (                                                                   \
        outb(pATI->CPIO_TV_OUT_INDEX, SetBits((_Index), TV_REG_INDEX)), \
        outl(pATI->CPIO_TV_OUT_DATA, (_Value))                          \
    )

/* Wait until "n" queue entries are free */
#define ibm8514WaitQueue(_n)                      \
    {                                             \
        while (inw(GP_STAT) & (0x0100U >> (_n))); \
    }
#define ATIWaitQueue(_n)                                    \
    {                                                       \
        while (inw(EXT_FIFO_STATUS) & (0x010000U >> (_n))); \
    }

/* Wait until GP is idle and queue is empty */
#define WaitIdleEmpty()                      \
    {                                        \
        while (inw(GP_STAT) & (GPBUSY | 1)); \
    }
#define ProbeWaitIdleEmpty()              \
    {                                     \
        int _i;                           \
        CARD16 _value;                    \
        for (_i = 0;  _i < 100000;  _i++) \
        {                                 \
            _value = inw(GP_STAT);        \
            if (_value == (CARD16)(-1))   \
                break;                    \
            if (!(_value & (GPBUSY | 1))) \
                break;                    \
        }                                 \
    }

/* Wait until GP has data available */
#define WaitDataReady()                    \
    {                                      \
        while (!(inw(GP_STAT) & DATARDY)); \
    }

#endif /* ___ATIIO_H___ */
