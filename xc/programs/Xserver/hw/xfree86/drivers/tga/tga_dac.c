/*
 * Copyright 1997,1998 by Alan Hourihane <alanh@fairlite.demon.co.uk>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tga/tga_dac.c,v 1.10 2000/03/06 22:59:31 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "BT.h"
#include "tga_regs.h"
#include "tga.h"

static void ICS1562ClockSelect(ScrnInfoPtr pScrn, int freq);
extern void ICS1562_CalcClockBits(long f, unsigned char *bits);

static void
ICS1562ClockSelect(ScrnInfoPtr pScrn, int freq)
{
    TGAPtr pTga = TGAPTR(pScrn);
    unsigned char pll_bits[7];
    unsigned long temp;
    int i, j;

    /*
     * For the DEC 21030 TGA, There lies an ICS1562 Clock Generator.
     * This requires the 55 clock bits be written in a serial manner to
     * bit 0 of the CLOCK register and on the 56th bit set the hold flag.
     */
    ICS1562_CalcClockBits(freq, pll_bits);
    for (i = 0;i <= 6; i++) {
	for (j = 0; j <= 7; j++) {
	    temp = (pll_bits[i] >> (7-j)) & 1;
	    if (i == 6 && j == 7)
	    	temp |= 2;
	    TGA_WRITE_REG(temp, TGA_CLOCK_REG);
	}
    }
}


Bool
DEC21030Init(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    TGAPtr pTga = TGAPTR(pScrn);
    TGARegPtr pReg = &pTga->ModeReg;

    if (pTga->RamDac != NULL) {
        RamDacHWRecPtr pBT = RAMDACHWPTR(pScrn);
	RamDacRegRecPtr ramdacReg = &pBT->ModeReg;

	ramdacReg->DacRegs[BT_COMMAND_REG_0] = 0xA0 |
	    (!pTga->Dac6Bit ? 0x2 : 0x0) | (pTga->SyncOnGreen ? 0x8 : 0x0);
	ramdacReg->DacRegs[BT_COMMAND_REG_2] = 0x20;
	ramdacReg->DacRegs[BT_STATUS_REG] = 0x14;
	(*pTga->RamDac->SetBpp)(pScrn, ramdacReg);

    } else {
        unsigned char *Bt463 = pTga->Bt463modeReg;
	int i, j;
 
	/* Command registers */
	Bt463[0] = 0x40;  Bt463[1] = 0x08;
	Bt463[2] = (pTga->SyncOnGreen ? 0x80 : 0x00);
	
	/* Read mask */
	Bt463[3] = 0xff;  Bt463[4] = 0xff;  Bt463[5] = 0xff;  Bt463[6] = 0x0f;
	
	/* Blink mask */
	Bt463[7] = 0x00;  Bt463[8] = 0x00;  Bt463[9] = 0x00; Bt463[10] = 0x00;
	
	/* Window attributes */
	for (i = 0, j=11; i < 16; i++) {
	    Bt463[j++] = 0x00;  Bt463[j++] = 0x01;  Bt463[j++] = 0x80;
	}
    }

    pReg->tgaRegs[0x00] = mode->CrtcHDisplay;
    pReg->tgaRegs[0x01] = (mode->CrtcHSyncStart - mode->CrtcHDisplay) / 4;
    pReg->tgaRegs[0x02] = (mode->CrtcHSyncEnd - mode->CrtcHSyncStart) / 4;
    pReg->tgaRegs[0x03] = (mode->CrtcHTotal - mode->CrtcHSyncEnd) / 4;
    pReg->tgaRegs[0x04] = mode->CrtcVDisplay;
    pReg->tgaRegs[0x05] = mode->CrtcVSyncStart - mode->CrtcVDisplay;
    pReg->tgaRegs[0x06] = mode->CrtcVSyncEnd - mode->CrtcVSyncStart;
    pReg->tgaRegs[0x07] = mode->CrtcVTotal - mode->CrtcVSyncEnd;

    /*
     * We do polarity the Step B way of the 21030 
     * Tell me how I can detect a Step A, and I'll support that too. 
     * But I think that the Step B's are most common 
     */
    if (mode->Flags & V_PHSYNC)
	pReg->tgaRegs[0x08] = 1; /* Horizontal Polarity */
    else
	pReg->tgaRegs[0x08] = 0;

    if (mode->Flags & V_PVSYNC)
	pReg->tgaRegs[0x09] = 1; /* Vertical Polarity */
    else
	pReg->tgaRegs[0x09] = 0;

    pReg->tgaRegs[0x0A] = mode->Clock;

    pReg->tgaRegs[0x10] = ((pReg->tgaRegs[0x00] / 4) & 0x1FF) |
		(((pReg->tgaRegs[0x00] / 4) & 0x600) << 19) |
		(pReg->tgaRegs[0x01] << 9) |
		(pReg->tgaRegs[0x02] << 14) |
		(pReg->tgaRegs[0x03] << 21) |
		(pReg->tgaRegs[0x08] << 30);
    pReg->tgaRegs[0x11] = pReg->tgaRegs[0x04] |
		(pReg->tgaRegs[0x05] << 11) | 
		(pReg->tgaRegs[0x06] << 16) |
		(pReg->tgaRegs[0x07] << 22) |
		(pReg->tgaRegs[0x09] << 30);

    pReg->tgaRegs[0x12] = 0x01;

    pReg->tgaRegs[0x13] = 0x0000;
    return TRUE;
}

void
DEC21030Save(ScrnInfoPtr pScrn, TGARegPtr tgaReg)
{
    TGAPtr pTga = TGAPTR(pScrn);

    tgaReg->tgaRegs[0x10] = TGA_READ_REG(TGA_HORIZ_REG);
    tgaReg->tgaRegs[0x11] = TGA_READ_REG(TGA_VERT_REG);
    tgaReg->tgaRegs[0x12] = TGA_READ_REG(TGA_VALID_REG);
    tgaReg->tgaRegs[0x13] = TGA_READ_REG(TGA_BASE_ADDR_REG);
    
    return;
}

void
DEC21030Restore(ScrnInfoPtr pScrn, TGARegPtr tgaReg)
{
    TGAPtr pTga = TGAPTR(pScrn);

    TGA_WRITE_REG(0x00, TGA_VALID_REG); /* Disable Video */

    ICS1562ClockSelect(pScrn, tgaReg->tgaRegs[0x0A]);

    TGA_WRITE_REG(tgaReg->tgaRegs[0x10], TGA_HORIZ_REG);
    TGA_WRITE_REG(tgaReg->tgaRegs[0x11], TGA_VERT_REG);
    TGA_WRITE_REG(tgaReg->tgaRegs[0x13], TGA_BASE_ADDR_REG);

    TGA_WRITE_REG(tgaReg->tgaRegs[0x12], TGA_VALID_REG); /* Re-enable Video */

    return;
}
