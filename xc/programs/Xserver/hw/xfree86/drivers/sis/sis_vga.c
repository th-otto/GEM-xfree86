/*
 * Copyright 1998,1999 by Alan Hourihane, Wigan, England.
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
 * Authors:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *           Mike Chapman <mike@paranoia.com>, 
 *           Juanjo Santamarta <santamarta@ctv.es>, 
 *           Mitani Hiroshi <hmitani@drl.mei.co.jp> 
 *           David Thomas <davtom@dream.org.uk>. 
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_vga.c,v 1.2 2000/02/18 12:20:01 tsi Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Version.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "vgaHW.h"

#include "sis.h"
#include "sis_regs.h"
#include "sis_bios.h"


#define Midx 	0
#define Nidx 	1
#define VLDidx 	2
#define Pidx 	3
#define PSNidx 	4
#define Fref 14318180
/* stability constraints for internal VCO -- MAX_VCO also determines 
 * the maximum Video pixel clock */
#define MIN_VCO Fref
#define MAX_VCO 135000000
#define MAX_VCO_5597 353000000
#define MAX_PSN 0 /* no pre scaler for this chip */
#define TOLERANCE 0.01	/* search smallest M and N in this tolerance */




/* local used for debug */
static void
SiSModifyModeInfo(DisplayModePtr mode)
{
/*
	mode->Clock = 31500;
	mode->CrtcHTotal	= 832;
	mode->CrtcHDisplay	= 640;
	mode->CrtcHBlankStart	= 648;
	mode->CrtcHSyncStart	= 664;
	mode->CrtcHSyncEnd	= 704;
	mode->CrtcHBlankEnd	= 824;

	mode->CrtcVTotal	= 520;
	mode->CrtcVDisplay	= 480;
	mode->CrtcVBlankStart	= 488;
	mode->CrtcVSyncStart	= 489;
	mode->CrtcVSyncEnd	= 492;
	mode->CrtcVBlankEnd	= 512;
*/
	if (mode->CrtcHBlankStart == mode->CrtcHDisplay)
		mode->CrtcHBlankStart++;
	if (mode->CrtcHBlankEnd == mode->CrtcHTotal)
		mode->CrtcHBlankEnd--;
	if (mode->CrtcVBlankStart == mode->CrtcVDisplay)
		mode->CrtcVBlankStart++;
	if (mode->CrtcVBlankEnd == mode->CrtcVTotal)
		mode->CrtcVBlankEnd--;
}

static void
Find530Threshold(ScrnInfoPtr pScrn, int mclk, int vclk, int bpp,
			int buswidth, int flags, int *low, int *high)
{
	int	factor, z;

	*high = 0x1F;
	if (flags & UMA)
		factor = 0x60;
	else
		factor = 0x30;
	z = factor * vclk * bpp;
	z = z / mclk / buswidth;
	*low = (z+1)/2 + 4;
	if (*low > 0x1F)
		*low = 0x1F;
}

struct funcargc {
	char	base;
	char	inc;
};

struct funcargc funca[12] = {
	{61, 3}, {52, 5}, {68, 7}, {100, 11},
	{43, 3}, {42, 5}, {54, 7}, {78, 11},
	{34, 3}, {37, 5}, {47, 7}, {67, 11}};

struct funcargc funcb[12] = {
	{81, 4}, {72, 6}, {88, 8}, {120, 12},
	{55, 4}, {54, 6}, {66, 8}, {90, 12},
	{42, 4}, {45, 6}, {55, 8}, {75, 12}};

char timing[8] = {1, 2, 2, 3, 0, 1, 1, 2};

static void
Find300_Threshold(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	SISPtr		pSiS = SISPTR(pScrn);
	SISRegPtr	pReg = &pSiS->ModeReg;
	int		mclk = pSiS->MemClock;
	int		vclk = mode->Clock;
	int		bpp = pScrn->bitsPerPixel/8;
	int		lowa, lowb, low;
	struct funcargc	*p;
	unsigned int	i, j;

	pReg->sisRegs3C4[0x16] = pSiS->SavedReg.sisRegs3C4[0x16];

	if (!bpp)	bpp = 1;

	do {
		i = ((pReg->sisRegs3C4[0x18] & 0x60) >> 4) |
		    ((pReg->sisRegs3C4[0x18] & 0x02) >> 1);
		j = ((pReg->sisRegs3C4[0x14] & 0xC0) >> 4) |
		    (pReg->sisRegs3C4[0x16] >> 6);
		p = &funca[j];

		lowa = (p->base + p->inc*timing[i])*vclk*bpp;
		lowa = (lowa + (mclk-1)) / mclk;
		lowa = (lowa + 15)/16;

		p = &funcb[j];
		lowb = (p->base + p->inc*timing[i])*vclk*bpp;
		lowb = (lowb + (mclk-1)) / mclk;
		lowb = (lowb + 15)/16;

		if (lowb < 4)
			lowb = 0;
		else
			lowb -= 4;

		low = (lowa > lowb)? lowa: lowb;

		low++;

		if (low <= 0x13) {
			break;
		} else {
			i = (pReg->sisRegs3C4[0x16] >> 6) & 3;
			if (!i) {
				low = 0x13;
				break;
			} else {
				i--;
				pReg->sisRegs3C4[0x16] &= 0x3C;
				pReg->sisRegs3C4[0x16] |= (i << 6);
			}
		}
	} while (1);

	pReg->sisRegs3C4[0x08] = ((low & 0xF) << 4) | 0xF;
	pReg->sisRegs3C4[0x0F] = (low & 0x10) << 1;
	pReg->sisRegs3C4[0x09] &= 0xF0;
	if (low+3 > 15)
		pReg->sisRegs3C4[0x09] |= 0x0F;
	else
		pReg->sisRegs3C4[0x09] |= low + 3;
}

struct QConfig  {
	int	GT;
	int	QC;
};

struct	QConfig qconfig[20] = {
	{1, 0x0}, {1, 0x2}, {1, 0x4}, {1, 0x6}, {1, 0x8},
	{1, 0x3}, {1, 0x5}, {1, 0x7}, {1, 0x9}, {1, 0xb},
	{0, 0x0}, {0, 0x2}, {0, 0x4}, {0, 0x6}, {0, 0x8},
	{0, 0x3}, {0, 0x5}, {0, 0x7}, {0, 0x9}, {0, 0xb}};

int cycleA[20][2] = {
	{88,88}, {80,80}, {78,78}, {72,72}, {70,70},
	{79,72}, {77,70}, {71,64}, {69,62}, {49,44},
	{73,78}, {65,70}, {63,68}, {57,62}, {55,60},
	{64,62}, {62,60}, {56,54}, {54,52}, {34,34}};

static void
Find630_Threshold(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	SISPtr		pSiS = SISPTR(pScrn);
	SISRegPtr	pReg = &pSiS->ModeReg;
	int		mclk = pSiS->MemClock;
	int		vclk = mode->Clock;
	int		bpp = pScrn->bitsPerPixel/8;
	CARD32		temp;
	PCITAG		NBridge;
	int		low, lowa, cyclea;
	int		i, j;

	if (!bpp)	bpp = 1;

	i = 0;
	j = (pReg->sisRegs3C4[0x14] >> 6) - 1;

	while (1)  {
#ifdef	DEBUG
		ErrorF("Config %d GT = %d, QC = %x, CycleA = %d\n",
			i, qconfig[i].GT, qconfig[i].QC, cycleA[i][j]);
#endif
		cyclea = cycleA[i][j];
		lowa = cyclea * vclk * bpp;
		lowa = (lowa + (mclk-1)) / mclk;
		lowa = (lowa + 15) / 16;
		low = lowa + 1;
		if (low <= 0x13)
			break;
		else
			if (i < 19)
				i++;
			else  {
				low = 0x13;
#ifdef	DEBUG
	ErrorF("This mode may has threshold problem and had better removed\n");
#endif
				break;
			}
	}
#ifdef	DEBUG
	ErrorF("Using Config %d with CycleA = %d\n", i, cyclea);
#endif
	pReg->sisRegs3C4[0x08] = ((low & 0xF) << 4) | 0xF;
	pReg->sisRegs3C4[0x0F] &= 0xDF;
	pReg->sisRegs3C4[0x0F] |= (low & 0x10) << 1;
	pReg->sisRegs3C4[0x09] &= 0xF0;
	if (lowa+4 > 15)
		pReg->sisRegs3C4[0x09] |= 0x0F;
	else
		pReg->sisRegs3C4[0x09] |= (lowa+4);

	/* write PCI configuration space */
	NBridge = pciTag(0, 0, 0);
	temp = pciReadLong(NBridge, 0x50);
	temp &= 0xF0FFFFFF;
	temp |= qconfig[i].QC << 24;
	pciWriteLong(NBridge, 0x50, temp);

	temp = pciReadLong(NBridge, 0xA0);
	temp &= 0xF0FFFFFF;
	temp |= qconfig[i].GT << 24;
	pciWriteLong(NBridge, 0xA0, temp);

#ifdef	DEBUG
	temp = pciReadLong(NBridge, 0x50);
	ErrorF("QueueConfig: 0x80000050 = %x\n", temp);
	temp = pciReadLong(NBridge, 0xA0);
	ErrorF("QueueConfig: 0x800000A0 = %x\n", temp);
#endif

}

static Bool
SiSInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    SISPtr pSiS = SISPTR(pScrn);
    SISRegPtr pReg = &pSiS->ModeReg;
    int gap, safetymargin, MemBand;
    int vgaIOBase;
    unsigned char temp, SR5State;
    unsigned short temp1;
    int Base,mclk;
    int offset;
    int clock = mode->Clock;
    unsigned int vclk[5];
    int CRT_CPUthresholdLow ;
    int CRT_CPUthresholdHigh ;
    unsigned int CRT_ENGthreshold ;

    int num, denum, div, sbit, scale;

    PDEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3, "SiSInit()\n"));
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    SiSSave(pScrn, pReg);

    outb(VGA_SEQ_INDEX, 0x05); /* Unlock Registers */
    SR5State = inb(VGA_SEQ_DATA);
    outw(VGA_SEQ_INDEX, 0x8605);

    pSiS->scrnOffset = pScrn->displayWidth * pScrn->bitsPerPixel / 8;
    if ((mode->Flags & V_INTERLACE)==0)  {
	offset = pScrn->displayWidth >> 3;
	pReg->sisRegs3C4[0x06] &= 0xDF;
    } else  {
	offset = pScrn->displayWidth >> 2;
	pReg->sisRegs3C4[0x06] |= 0x20;
    }

    /* Enable Linear */
    switch (pSiS->Chipset)  {
    case PCI_CHIP_SIS5597:
    case PCI_CHIP_SIS6326:
    case PCI_CHIP_SIS530:
	pReg->sisRegs3C4[BankReg] |= 0x82;
	pReg->sisRegs3C4[0x0C] |= 0xA0;
	pReg->sisRegs3C4[0x0B] |= 0x60;
	break;
    case PCI_CHIP_SIS300:
    case PCI_CHIP_SIS630:
    case PCI_CHIP_SIS540:
	pReg->sisRegs3C4[0x20] |= 0x81;
	pReg->sisRegs3C4[0x06] |= 0x02;
	break;
    default:
	pReg->sisRegs3C4[BankReg] |= 0x82;
    }

    switch (pScrn->bitsPerPixel) {
	case 8:
	    pSiS->DstColor = 0x0000;
	    break;
	case 15:
	    pSiS->DstColor = 0x4000;
	    offset <<= 1;
	    pReg->sisRegs3C4[BankReg] |= 0x04;
	    break;
	case 16:
	    pSiS->DstColor = (short)0x8000;
	    offset <<= 1;
	    pReg->sisRegs3C4[BankReg] |= 0x08;
	    break;
	case 24:
	    offset += (offset << 1);
	    switch (pSiS->Chipset)  {
		case PCI_CHIP_SIS300:
		case PCI_CHIP_SIS630:
		case PCI_CHIP_SIS540:
		    pReg->sisRegs3C4[0x06] |= 0x0C;
		    break;
		default:
		    pReg->sisRegs3C4[BankReg] |= 0x10;
		    pReg->sisRegs3C4[MMIOEnable] |= 0x90;
	    }
	    break;
	case 32:
	    pSiS->DstColor = (short)0xC000;
	    offset <<= 2;
	    switch (pSiS->Chipset)  {
		case PCI_CHIP_SIS300:
		case PCI_CHIP_SIS630:
		case PCI_CHIP_SIS540:
		    pReg->sisRegs3C4[0x06] &= 0xE3;
		    pReg->sisRegs3C4[0x06] |= 0x10;
		    break;
		case PCI_CHIP_SIS530:
		    pReg->sisRegs3C4[BankReg] |= 0x10;
		    pReg->sisRegs3C4[MMIOEnable] |= 0x90;
		    pReg->sisRegs3C4[0x09] |= 0x80;
		default:
		    return FALSE;
	    }
	    break;
    }
    switch (pScrn->videoRam)  {
	case 512:
	    temp = 0x00;
	    break;
	case 1024:
	    temp = 0x20;
	    break;
	case 2048:
	    temp = 0x40;
	    break;
	case 4096:
	    temp = 0x60;
	    break;
	case 8192:
	    temp = 0x80;
	    break;
	default:
	    temp = 0x20;
    }
    switch (pSiS->Chipset)  {
	case PCI_CHIP_SG86C225:
	case PCI_CHIP_SIS5597:
	case PCI_CHIP_SIS6326:
	    pReg->sisRegs3C4[LinearAdd0] = (pSiS->FbAddress & 0x07F80000) >> 19;
	    pReg->sisRegs3C4[LinearAdd1] =((pSiS->FbAddress & 0xF8000000) >> 27)
				      | temp; /* Enable linear with max 4M */
	    break;
	case PCI_CHIP_SIS530:
	    pReg->sisRegs3C4[LinearAdd0] = (pSiS->FbAddress & 0x07F80000) >> 19;
	    pReg->sisRegs3C4[LinearAdd1] =((pSiS->FbAddress & 0xF8000000) >> 27)
				      | temp; /* Enable linear with max 8M */
	    break;
    }

    switch (pSiS->Chipset)  {
	case PCI_CHIP_SIS5597:
	case PCI_CHIP_SIS6326:
	case PCI_CHIP_SIS530:
	    /* Screen Offset */
	    pReg->sisRegs3x4[Offset] = offset & 0xFF;
	    pReg->sisRegs3C4[CRTCOff] = ((offset & 0xF00) >> 4) | 
				(((mode->CrtcVTotal-2)     & 0x400) >> 10 ) |
				(((mode->CrtcVDisplay-1)   & 0x400) >> 9 ) |
				(((mode->CrtcVSyncStart-1) & 0x400) >> 8 ) |
				(((mode->CrtcVSyncStart)   & 0x400) >> 7 ) ;

	    /* Extended Horizontal Overflow Register */
	    pReg->sisRegs3C4[0x12] &= 0xE0;
	    pReg->sisRegs3C4[0x12] |= (
		(((mode->CrtcHTotal >> 3) - 5)     & 0x100) >> 8 |
		(((mode->CrtcHDisplay >> 3) - 1)   & 0x100) >> 7 |
		(((mode->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6 |
		((mode->CrtcHSyncStart >> 3)       & 0x100) >> 5 |
		((mode->CrtcHSyncEnd >> 3)         & 0x40)  >> 2);

	    if (mode->CrtcVDisplay > 1024)
		/* disable line compare */
		pReg->sisRegs3C4[0x38] |= 0x04;
	    else
		pReg->sisRegs3C4[0x38] &= 0xFB;

	    if (( pScrn->depth == 24) || (pScrn->depth == 32) ||
					(mode->CrtcHDisplay >= 1280))
		/* Enable high speed DCLK */
		pReg->sisRegs3C4[0x3E] |= 1;
	    else
		pReg->sisRegs3C4[0x3E] &= 0xFE;
	    break;

	case PCI_CHIP_SIS300:
	case PCI_CHIP_SIS630:
	case PCI_CHIP_SIS540:
	    /* Screen Offset */
	    pReg->sisRegs3x4[Offset] = offset & 0xFF;
	    pReg->sisRegs3C4[0x0E] &= 0xF0;
	    pReg->sisRegs3C4[0x0E] |= ((offset & 0xF00) >> 8);

	    pReg->sisRegs3C4[0x10] = 
			((mode->CrtcHDisplay *(pScrn->bitsPerPixel >>3) + 63) >> 6) +1;
	    pReg->sisRegs3C4[0x06] |= 0x01;

	    /* Extended Vertical Overflow */
	    pReg->sisRegs3C4[0x0A] = 
				(((mode->CrtcVTotal     -2) & 0x400) >> 10) |
				(((mode->CrtcVDisplay   -1) & 0x400) >> 9 ) |
				(((mode->CrtcVBlankStart  ) & 0x400) >> 8 ) |
				(((mode->CrtcVSyncStart   ) & 0x400) >> 7 ) |
				(((mode->CrtcVBlankEnd    ) & 0x100) >> 4 ) |
				(((mode->CrtcVSyncEnd     ) & 0x010) << 1 );

	    /* Extended Horizontal Overflow */
	    pReg->sisRegs3C4[0x0B] = 
			(((mode->CrtcHTotal      >> 3) - 5) & 0x300) >> 8 |
			(((mode->CrtcHDisplay    >> 3) - 1) & 0x300) >> 6 |
			((mode->CrtcHBlankStart >> 3)       & 0x300) >> 4 |
			((mode->CrtcHSyncStart  >> 3)       & 0x300) >> 2 ;
	    pReg->sisRegs3C4[0x0C] &= 0xF8;
	    pReg->sisRegs3C4[0x0C] |= 
			((mode->CrtcHBlankEnd >> 3) & 0x0C0) >> 6 |
			((mode->CrtcHSyncEnd  >> 3) & 0x020) >> 3;

	    /* line compare */
	    if (mode->CrtcHDisplay > 0)
		pReg->sisRegs3C4[0x0F] |= 0x08;
	    else
		pReg->sisRegs3C4[0x0F] &= 0xF7;
	    break;
    }


    /* Set vclk */	
    if (compute_vclk(clock, &num, &denum, &div, &sbit, &scale)) {
	switch  (pSiS->Chipset)  {
	    case PCI_CHIP_SIS5597:
	    case PCI_CHIP_SIS6326:
	    case PCI_CHIP_SIS530:
		pReg->sisRegs3C4[XR2A] = (num - 1) & 0x7f ;
		pReg->sisRegs3C4[XR2A] |= (div == 2) ? 0x80 : 0;
		pReg->sisRegs3C4[XR2B] = ((denum -1) & 0x1f);
		pReg->sisRegs3C4[XR2B] |= (((scale -1)&3) << 5);
		/* When set VCLK, you should set SR13 first */
		if (sbit) 
		    pReg->sisRegs3C4[ClockBase] |= 0x40;
		else 
		    pReg->sisRegs3C4[ClockBase] &= 0xBF;
            
		break;
	    case PCI_CHIP_SIS300:
	    case PCI_CHIP_SIS630:
	    case PCI_CHIP_SIS540:
		pReg->sisRegs3C4[0x2B] = (num -1) & 0x7f;
		if (div == 2)
		    pReg->sisRegs3C4[0x2B] |= 0x80;
		pReg->sisRegs3C4[0x2C] = ((denum -1) & 0x1f);
		pReg->sisRegs3C4[0x2C] |= (((scale-1)&3) << 5);
		if (sbit)
		    pReg->sisRegs3C4[0x2C] |= 0x80;
		pReg->sisRegs3C4[0x2D] = 0x80;
		break;
	}
    }
    else  {
    /* if compute_vclk cannot handle the request clock try sisCalcClock! */
    	SiSCalcClock(pScrn, clock, 2, vclk);
	switch (pSiS->Chipset)  {
	    case PCI_CHIP_SIS5597:
	    case PCI_CHIP_SIS6326:
	    case PCI_CHIP_SIS530:
		pReg->sisRegs3C4[XR2A] = (vclk[Midx] - 1) & 0x7f ;
		pReg->sisRegs3C4[XR2A] |= ((vclk[VLDidx] == 2 ) ? 1 : 0 ) << 7 ;

		/* bits [4:0] contain denumerator -MC */
		pReg->sisRegs3C4[XR2B] = (vclk[Nidx] -1) & 0x1f ;

		if (vclk[Pidx] <= 4){
		/* postscale 1,2,3,4 */
		    pReg->sisRegs3C4[XR2B] |= (vclk[Pidx] -1 ) << 5 ;
		    pReg->sisRegs3C4[ClockBase] &= 0xBF;
		} else {
		/* postscale 6,8 */
		    pReg->sisRegs3C4[XR2B] |= ((vclk[Pidx] / 2) -1 ) << 5 ;
		    pReg->sisRegs3C4[ClockBase] |= 0x40;
		}
		pReg->sisRegs3C4[XR2B] |= 0x80 ;   /* gain for high frequency */
		break;

	    case PCI_CHIP_SIS300:
	    case PCI_CHIP_SIS630:
	    case PCI_CHIP_SIS540:
		pReg->sisRegs3C4[0x2B] = (vclk[Midx] - 1) & 0x7f ;
		pReg->sisRegs3C4[0x2B] |= ((vclk[VLDidx] == 2 ) ? 1 : 0 ) << 7 ;

		/* bits [4:0] contain denumerator -MC */
		pReg->sisRegs3C4[0x2C] = (vclk[Nidx] -1) & 0x1f ;

		if (vclk[Pidx] <= 4)  {
		/* postscale 1,2,3,4 */
		    pReg->sisRegs3C4[0x2C] |= (vclk[Pidx] -1 ) << 5 ;
		    pReg->sisRegs3C4[0x2C] &= 0x7F;
		} else  {
		/* postscale 6,8 */
		    pReg->sisRegs3C4[0x2C] |= ((vclk[Pidx] / 2) -1 ) << 5 ;
		    pReg->sisRegs3C4[0x2C] |= 0x80;
		}
		pReg->sisRegs3C4[0x2D] = 0x80;
		break;
	}
    } /* end of set vclk */

    switch (pSiS->Chipset)  {
	case PCI_CHIP_SIS5597:
	case PCI_CHIP_SIS6326:
	case PCI_CHIP_SIS530:
	    if (clock > 135000) pReg->sisRegs3C4[ClockReg] |= 0x02;
	    break;
	case PCI_CHIP_SIS300:
	case PCI_CHIP_SIS630:
	case PCI_CHIP_SIS540:
	    if (clock > 150000)  {	/* enable two-pixel mode */
		pReg->sisRegs3C4[0x07] |= 0x80;
		pReg->sisRegs3C4[0x32] |= 0x08;
	    } else  {
		pReg->sisRegs3C4[0x07] &= 0x7F;
		pReg->sisRegs3C4[0x32] &= 0xF7;
	    }

	    pReg->sisRegs3C4[0x07] |= 0x10;
	    pReg->sisRegs3C4[0x07] &= 0xFC;
	    if (clock < 100000)
		pReg->sisRegs3C4[0x07] |= 0x03;
	    else if (clock < 200000)
		pReg->sisRegs3C4[0x07] |= 0x02;
	    else if (clock < 250000)
		pReg->sisRegs3C4[0x07] |= 0x01;
	    break;
    }
    pReg->sisRegs3C2[0x00] = inb(0x3CC) | 0x0C; /* Programmable Clock */

    if (pSiS->FastVram && ((pSiS->Chipset == PCI_CHIP_SIS530) ||
	(pSiS->Chipset == PCI_CHIP_SIS6326) ||
	(pSiS->Chipset == PCI_CHIP_SIS5597)))
	pReg->sisRegs3C4[ExtMiscCont5]|= 0xC0;
    else 
	pReg->sisRegs3C4[ExtMiscCont5]&= ~0xC0;

    pSiS->ValidWidth = TRUE;
    if ((pSiS->Chipset == PCI_CHIP_SIS5597) ||
	(pSiS->Chipset == PCI_CHIP_SIS6326) ||
	(pSiS->Chipset == PCI_CHIP_SIS530))
    {
	pReg->sisRegs3C4[GraphEng] &= 0xCF; /* Clear logical width bits */
	if (pScrn->bitsPerPixel == 24)  {
	    pReg->sisRegs3C4[GraphEng] |= 0x30; /* Invalid logical width */
	    pSiS->ValidWidth = FALSE;
	}
	else  {
	    switch ( pScrn->virtualX * (pScrn->bitsPerPixel >> 3) ) {
 		case 1024:
 		    pReg->sisRegs3C4[GraphEng] |= 0x00; /* | 00 = No change */
	 	    break;
 		case 2048:
	 	    pReg->sisRegs3C4[GraphEng] |= 0x10; 
	 	    break;
 		case 4096:
		    pReg->sisRegs3C4[GraphEng] |= 0x20; 
		    break;
 		default:
		    /* Invalid logical width */
		    pReg->sisRegs3C4[GraphEng] =  0x30;
		    pSiS->ValidWidth = FALSE;
		    break;
	    }
	}
    }
    PDEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
		"virtualX = %d depth = %d Logical width = %d\n",
		pScrn->virtualX, pScrn->bitsPerPixel,
		pScrn->virtualX * pScrn->bitsPerPixel/8));

    if (!pSiS->NoAccel) {
	switch  (pSiS->Chipset)  {
	    case PCI_CHIP_SIS5597:
	    case PCI_CHIP_SIS6326:
	    case PCI_CHIP_SIS530:
		pReg->sisRegs3C4[GraphEng] |= 0x40;
		if (pSiS->TurboQueue) {
		    pReg->sisRegs3C4[GraphEng] |= 0x80;
							/* All Queue for 2D */
		    pReg->sisRegs3C4[ExtMiscCont9] &= 0xFC;
		    if (pSiS->HWCursor)
			pReg->sisRegs3C4[TurboQueueBase] = (pScrn->videoRam/32) - 2;
		    else
			pReg->sisRegs3C4[TurboQueueBase] = (pScrn->videoRam/32) - 1;
		}
		pReg->sisRegs3C4[MMIOEnable] |= 0x60; /* At PCI base */
		pReg->sisRegs3C4[Mode64] |= 0x80;
		break;
	    case PCI_CHIP_SIS300:
	    case PCI_CHIP_SIS630:
	    case PCI_CHIP_SIS540:
		pReg->sisRegs3C4[0x1E] |= 0x40;
		if (pSiS->TurboQueue)  {	/* set Turbo Queue as 512k */
		    temp1 = ((pScrn->videoRam/64)-4);
		    pReg->sisRegs3C4[0x26] = temp1 & 0xFF;
		    pReg->sisRegs3C4[0x27] = ((temp1 >> 8) & 3) || 0xF0;
		}
		break;
	}
    }

   /* Set memclock */
    if ((pSiS->Chipset == PCI_CHIP_SIS5597) || (pSiS->Chipset == PCI_CHIP_SIS6326)) {
      if (pSiS->MemClock > 66000) {
          SiSCalcClock(pScrn, pSiS->MemClock, 1, vclk);
  
          pReg->sisRegs3C4[MemClock0] = (vclk[Midx] - 1) & 0x7f ;
          pReg->sisRegs3C4[MemClock0] |= ((vclk[VLDidx] == 2 ) ? 1 : 0 ) << 7 ;
          pReg->sisRegs3C4[MemClock1]  = (vclk[Nidx] -1) & 0x1f ;	/* bits [4:0] contain denumerator -MC */
          if (vclk[Pidx] <= 4){
 	    pReg->sisRegs3C4[MemClock1] |= (vclk[Pidx] -1 ) << 5 ; /* postscale 1,2,3,4 */
 	    pReg->sisRegs3C4[ClockBase] &= 0x7F;
 	  } else {
            pReg->sisRegs3C4[MemClock1] |= ((vclk[Pidx] / 2) -1 ) << 5 ;  /* postscale 6,8 */
 	    pReg->sisRegs3C4[ClockBase] |= 0x80;
 	  }

#if 1  /* Check programmed memory clock. Enable only to check the above code */
          mclk=14318*((pReg->sisRegs3C4[MemClock0] & 0x7f)+1);
          mclk=mclk/((pReg->sisRegs3C4[MemClock1] & 0x0f)+1);
          Base = pReg->sisRegs3C4[ClockBase];
          if ( (Base & 0x80)==0 ) {
            mclk = mclk / (((pReg->sisRegs3C4[MemClock1] & 0x60) >> 5)+1);
          }  else {
            if ((pReg->sisRegs3C4[MemClock1] & 0x60) == 0x40) { mclk=mclk/6;}
            if ((pReg->sisRegs3C4[MemClock1] & 0x60) == 0x60) { mclk=mclk/8;}
          }
          xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO,2,
           "Setting memory clock to %.3f MHz\n",
      	  mclk/1000.0);
#endif
      }
    }

    /* set threshold value */
    switch  (pSiS->Chipset)  {
    case PCI_CHIP_SIS5597:
    case PCI_CHIP_SIS6326:
	MemBand = sisMemBandWidth(pScrn) / 10 ;
	safetymargin = 1; 
	gap          = 4;

	CRT_ENGthreshold = 0x0F;
	CRT_CPUthresholdLow = ((pScrn->depth*clock) / MemBand)+safetymargin;
	CRT_CPUthresholdHigh =((pScrn->depth*clock) / MemBand)+gap+safetymargin;

	if ( CRT_CPUthresholdLow > (pScrn->depth < 24 ? 0xe:0x0d) ) { 
	    CRT_CPUthresholdLow = (pScrn->depth < 24 ? 0xe:0x0d); 
	}

	if ( CRT_CPUthresholdHigh > (pScrn->depth < 24 ? 0x10:0x0f) ) {
	    CRT_CPUthresholdHigh = (pScrn->depth < 24 ? 0x10:0x0f);
	}

	pReg->sisRegs3C4[CPUThreshold] =  (CRT_ENGthreshold & 0x0F) | 
					(CRT_CPUthresholdLow & 0x0F)<<4 ;
	pReg->sisRegs3C4[CRTThreshold] = CRT_CPUthresholdHigh & 0x0F;

	break;
    case PCI_CHIP_SIS530:
	Find530Threshold(pScrn, pSiS->MemClock, mode->Clock,
			pScrn->bitsPerPixel, pSiS->BusWidth, pSiS->Flags,
			&CRT_CPUthresholdLow, &CRT_CPUthresholdHigh);
	pReg->sisRegs3C4[8] = (CRT_CPUthresholdLow & 0xf) << 4 | 0xF;
	pReg->sisRegs3C4[9] &= 0xF0;
	pReg->sisRegs3C4[9] |= (CRT_CPUthresholdHigh & 0xF);
	pReg->sisRegs3C4[0x3F] &= 0xE3;
	pReg->sisRegs3C4[0x3F] |= (CRT_CPUthresholdHigh & 0x10) |
					(CRT_CPUthresholdLow & 0x10) >> 2 |
					0x08;
	break;
    case PCI_CHIP_SIS300:
	Find300_Threshold(pScrn, mode);
	break;
    case PCI_CHIP_SIS630:
    case PCI_CHIP_SIS540: /* temperary settings */
	Find630_Threshold(pScrn, mode);
	break;
    }
    outw(VGA_SEQ_INDEX, (SR5State << 8) | 0x05); /* Relock Registers */
    return(TRUE);
}

/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
SiSModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg;
    SISPtr pSiS = SISPTR(pScrn);
    SISRegPtr sisReg;
	unsigned char	temp;

    vgaHWUnlock(hwp);

    SiSModifyModeInfo(mode);

    /* Initialise the ModeReg values */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;

	setSISIDXREG(pSiS->RelIO+CROFFSET, 0x30, 0xFC, SWITCH_TO_CRT2);

	inSISIDXREG(pSiS->RelIO+CROFFSET, 0x31, temp);
	temp &= 0xDD;
	temp |= (DRIVER_MODE |DISABLE_CRT2_DISPLAY) >> 8;
	outSISIDXREG(pSiS->RelIO+CROFFSET, 0x31, temp);

    if (!SiSInit(pScrn, mode))
	return FALSE;

    PDEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		"HDisplay: %d, VDisplay: %d  \n",
		mode->HDisplay, mode->VDisplay));
    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);
    vgaReg = &hwp->ModeReg;
    sisReg = &pSiS->ModeReg;

    vgaReg->Attribute[0x10] = 0x01;
    if (pScrn->bitsPerPixel > 8) 
    	vgaReg->Graphics[0x05] = 0x00;

    vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);

    SiSRestore(pScrn, sisReg);

	if (pSiS->VBFlags)  {
		SiSSetMode(pScrn, 0x44);
		outSISIDXREG(pSiS->RelIO+CROFFSET, 0x32,
						sisReg->sisRegs3C4[0x32]);
	}

    vgaHWProtect(pScrn, FALSE);

    if (pSiS->MemClock)
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
		"Memory clock is set to %3.3fMHz\n", pSiS->MemClock/1.0);

/* Reserved for debug
 *
    SiSIODump(pScrn);
    SiSDumpModeInfo(pScrn, mode);
 *
 */
    return TRUE;
}

void SiSVGASetup(ScrnInfoPtr pScrn)
{
	SISPTR(pScrn)->ModeInit = SiSModeInit;
}
