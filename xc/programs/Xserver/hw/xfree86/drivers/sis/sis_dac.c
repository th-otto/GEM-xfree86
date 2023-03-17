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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_dac.c,v 1.13 2000/02/12 20:45:35 dawes Exp $ */

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


int
compute_vclk(
        int Clock,
        int *out_n,
        int *out_dn,
        int *out_div,
        int *out_sbit,
        int *out_scale)
{
        float f,x,y,t, error, min_error;
        int n, dn, best_n, best_dn;

        /*
         * Rules
         *
         * VCLK = 14.318 * (Divider/Post Scalar) * (Numerator/DeNumerator)
         * Factor = (Divider/Post Scalar)
         * Divider is 1 or 2
         * Post Scalar is 1, 2, 3, 4, 6 or 8
         * Numberator ranged from 1 to 128
         * DeNumerator ranged from 1 to 32
         * a. VCO = VCLK/Factor, suggest range is 150 to 250 Mhz
         * b. Post Scalar selected from 1, 2, 4 or 8 first.
         * c. DeNumerator selected from 2.
         *
         * According to rule a and b, the VCO ranges that can be scaled by
         * rule b are:
         *      150    - 250    (Factor = 1)
         *       75    - 125    (Factor = 2)
         *       37.5  -  62.5  (Factor = 4)
         *       18.75 -  31.25 (Factor = 8)
         *
         * The following ranges use Post Scalar 3 or 6:
         *      125    - 150    (Factor = 1.5)
         *       62.5  -  75    (Factor = 3)
         *       31.25 -  37.5  (Factor = 6)
         *
         * Steps:
         * 1. divide the Clock by 2 until the Clock is less or equal to 31.25.
         * 2. if the divided Clock is range from 18.25 to 31.25, than
         *    the Factor is 1, 2, 4 or 8.
         * 3. if the divided Clock is range from 15.625 to 18.25, than
         *    the Factor is 1.5, 3 or 6.
         * 4. select the Numberator and DeNumberator with minimum deviation.
         *
         * ** this function can select VCLK ranged from 18.75 to 250 Mhz
         */
        f = (float) Clock;
        f /= 1000.0;
        if ((f > 250.0) || (f < 18.75))
                return 0;

        min_error = f;
        y = 1.0;
        x = f;
        while (x > 31.25) {
                y *= 2.0;
                x /= 2.0;
        }
        if (x >= 18.25) {
                x *= 8.0;
                y = 8.0 / y;
        } else if (x >= 15.625) {
                x *= 12.0;
                y = 12.0 / y;
        }

        t = y;
        if (t == (float) 1.5) {
                *out_div = 2;
                t *= 2.0;
        } else {
                *out_div = 1;
        }
        if (t > (float) 4.0) {
                *out_sbit = 1;
                t /= 2.0;
        } else {
                *out_sbit = 0;
        }

        *out_scale = (int) t;

        for (dn=2;dn<=32;dn++) {
                for (n=1;n<=128;n++) {
                        error = x;
                        error -= ((float) 14.318 * (float) n / (float) dn);
                        if (error < (float) 0)
                                error = -error;
                        if (error < min_error) {
                                min_error = error;
                                best_n = n;
                                best_dn = dn;
                        }
                }
        }
        *out_n = best_n;
        *out_dn = best_dn;
	PDEBUG(ErrorF("compute_vclk: Clock=%d, n=%d, dn=%d, div=%d, sbit=%d,"
			" scale=%d\n", Clock, best_n, best_dn, *out_div,
			*out_sbit, *out_scale));
        return 1;
}


void
SiSCalcClock(ScrnInfoPtr pScrn, int clock, int max_VLD, unsigned int *vclk)
{
    SISPtr pSiS = SISPTR(pScrn);
    int M, N, P , PSN, VLD , PSNx ;
    int bestM, bestN, bestP, bestPSN, bestVLD;
    double bestError, abest = 42.0, bestFout;
    double target;
    double Fvco, Fout;
    double error, aerror;

    /*
     *	fd = fref*(Numerator/Denumerator)*(Divider/PostScaler)
     *
     *	M 	= Numerator [1:128] 
     *  N 	= DeNumerator [1:32]
     *  VLD	= Divider (Vco Loop Divider) : divide by 1, 2
     *  P	= Post Scaler : divide by 1, 2, 3, 4
     *  PSN     = Pre Scaler (Reference Divisor Select) 
     * 
     * result in vclk[]
     */
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
  
  int M_min = 2; 
  int M_max = 128;
  
/*  abest=10000.0; */
 
  target = clock * 1000;
 

  if (pSiS->Chipset == PCI_CHIP_SIS5597 || pSiS->Chipset == PCI_CHIP_SIS6326){
 	int low_N = 2;
 	int high_N = 5;
 	int PSN = 1;
 
 	P = 1;
 	if (target < MAX_VCO_5597 / 2)
 	    P = 2;
 	if (target < MAX_VCO_5597 / 3)
 	    P = 3;
 	if (target < MAX_VCO_5597 / 4)
 	    P = 4;
 	if (target < MAX_VCO_5597 / 6)
 	    P = 6;
 	if (target < MAX_VCO_5597 / 8)
 	    P = 8;
 
 	Fvco = P * target;
 
 	for (N = low_N; N <= high_N; N++){
 	    double M_desired = Fvco / Fref * N;
 	    if (M_desired > M_max * max_VLD)
 		continue;
 
 	    if ( M_desired > M_max ) {
 		M = M_desired / 2 + 0.5;
 		VLD = 2;
 	    } else {
 		M = Fvco / Fref * N + 0.5;
 		VLD = 1;
 	    }
 
 	    Fout = (double)Fref * (M * VLD)/(N * P);
 
 	    error = (target - Fout) / target;
 	    aerror = (error < 0) ? -error : error;
/* 	    if (aerror < abest && abest > TOLERANCE) {*/
 	    if (aerror < abest) {
 	        abest = aerror;
 	        bestError = error;
 	        bestM = M;
 	        bestN = N;
 	        bestP = P;
 	        bestPSN = PSN;
 	        bestVLD = VLD;
 	        bestFout = Fout;
 	    }
 	}
     }
     else {
         for (PSNx = 0; PSNx <= MAX_PSN ; PSNx++) {
 	    int low_N, high_N;
 	    double FrefVLDPSN;
 
 	    PSN = !PSNx ? 1 : 4;
 
 	    low_N = 2;
 	    high_N = 32;
 
 	    for ( VLD = 1 ; VLD <= max_VLD ; VLD++ ) {
 
 	        FrefVLDPSN = (double)Fref * VLD / PSN;
 	        for (N = low_N; N <= high_N; N++) {
 		    double tmp = FrefVLDPSN / N;
 
 		    for (P = 1; P <= 4; P++) {	
 		        double Fvco_desired = target * ( P );
 		        double M_desired = Fvco_desired / tmp;
 
 		        /* Which way will M_desired be rounded?  
 		         *  Do all three just to be safe.  
 		         */
 		        int M_low = M_desired - 1;
 		        int M_hi = M_desired + 1;
 
 		        if (M_hi < M_min || M_low > M_max)
 			    continue;
 
 		        if (M_low < M_min)
 			    M_low = M_min;
 		        if (M_hi > M_max)
 			    M_hi = M_max;
 
 		        for (M = M_low; M <= M_hi; M++) {
 			    Fvco = tmp * M;
 			    if (Fvco <= MIN_VCO)
 			        continue;
 			    if (Fvco > MAX_VCO)
 			        break;
 
 			    Fout = Fvco / ( P );
 
 			    error = (target - Fout) / target;
 			    aerror = (error < 0) ? -error : error;
 			    if (aerror < abest) {
 			        abest = aerror;
 			        bestError = error;
 			        bestM = M;
 			        bestN = N;
 			        bestP = P;
 			        bestPSN = PSN;
 			        bestVLD = VLD;
 			        bestFout = Fout;
 			    }
 			xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO,3,"Freq. selected: %.2f MHz, M=%d, N=%d, VLD=%d,"
 			       " P=%d, PSN=%d\n",
 			       (float)(clock / 1000.), M, N, P, VLD, PSN);
 			xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO,3,"Freq. set: %.2f MHz\n", Fout / 1.0e6);
 		        }
 		    }
 	        }
 	    }
         }
  }

  vclk[Midx]    = bestM;
  vclk[Nidx]    = bestN;
  vclk[VLDidx]  = bestVLD;
  vclk[Pidx]    = bestP;
  vclk[PSNidx]  = bestPSN;

	PDEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
		"Freq. selected: %.2f MHz, M=%d, N=%d, VLD=%d, P=%d, PSN=%d\n",
		(float)(clock / 1000.), vclk[Midx], vclk[Nidx], vclk[VLDidx], 
		vclk[Pidx], vclk[PSNidx]));
	PDEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
		"Freq. set: %.2f MHz\n", bestFout / 1.0e6));
	PDEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
		"VCO Freq.: %.2f MHz\n", bestFout*bestP / 1.0e6));
}


void
SiSRestore(ScrnInfoPtr pScrn, SISRegPtr sisReg)
{
    SISPtr pSiS = SISPTR(pScrn);
    int vgaIOBase;
    int i,max;

	PDEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
			"SiSRestore(ScrnInfoPtr pScrn, SISRegPtr sisReg)\n"));


    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    outw(VGA_SEQ_INDEX, 0x8605);

    switch (pSiS->Chipset) {
	case PCI_CHIP_SIS5597:
           max=0x39;
           break;
	case PCI_CHIP_SIS6326:
           max=0x3F;
           break;
        case PCI_CHIP_SIS530:
	   max=0x3F;
           break;
        case PCI_CHIP_SIS300:
        case PCI_CHIP_SIS630:
        case PCI_CHIP_SIS540:
           max=0x3D;
           break;
	default:
           max=0x37;
           break;
    }

    if ((pSiS->Chipset == PCI_CHIP_SIS630) && (sisReg->sisRegs3C4[0x1e] & 0x40))
	outw(VGA_SEQ_INDEX, sisReg->sisRegs3C4[0x20] << 8 | 0x20);

    for (i = 0x06; i <= max; i++) {
	if (i== 0x1E || i==0x32) continue;
	outb(VGA_SEQ_INDEX,i);
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO,4,
    		    "XR%X Contents - %02X ", i, inb(VGA_SEQ_DATA));
	outb(VGA_SEQ_DATA,sisReg->sisRegs3C4[i]);

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO,4,
			"Restore to - %02X Read after - %02X\n",
			sisReg->sisRegs3C4[i], inb(VGA_SEQ_DATA));
    }
    if ((pSiS->Chipset == PCI_CHIP_SIS300 || pSiS->Chipset == PCI_CHIP_SIS630 ||
	pSiS->Chipset == PCI_CHIP_SIS540) && (sisReg->sisRegs3C4[0x1e] & 0x40))
	SiSInitializeAccelerator(pScrn);


    outw(vgaIOBase + 4, (sisReg->sisRegs3x4[Offset] << 8) | Offset);

    outb(0x3C2, sisReg->sisRegs3C2[0x00]);


    /* MemClock needs this to take effect */
     
	outw(VGA_SEQ_INDEX, 0x0100);	/* Synchronous Reset */
	outw(VGA_SEQ_INDEX, 0x0300);	/* End Reset */

}

void
SiSSave(ScrnInfoPtr pScrn, SISRegPtr sisReg)
{
    SISPtr pSiS = SISPTR(pScrn);
    int vgaIOBase;
    int i,max;
    unsigned char temp;

	PDEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
			"SiSSave(ScrnInfoPtr pScrn, SISRegPtr sisReg)\n"));

    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    outb(VGA_SEQ_INDEX, 0x05); /* Unlock Registers */
    temp = inb(VGA_SEQ_DATA);
    outw(VGA_SEQ_INDEX, 0x8605);

    switch (pSiS->Chipset) {
	case PCI_CHIP_SIS5597:
           max=0x39;
           break;
	case PCI_CHIP_SIS6326:
	case PCI_CHIP_SIS530:
           max=0x3F;
           break;
	case PCI_CHIP_SIS300:
	case PCI_CHIP_SIS630:
	case PCI_CHIP_SIS540:
           max=0x3D;
           break;
	default:
           max=0x37;
           break;
	}

    for (i = 0x06; i <= max; i++) {
        outb(VGA_SEQ_INDEX, i);
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
    		    "XR%02X Contents - %02X \n", i, inb(VGA_SEQ_DATA));
	sisReg->sisRegs3C4[i] = inb(VGA_SEQ_DATA);

    }
	/* for SiS301 */
	for (i=0; i<0x29; i++)  {
		inSISIDXREG(pSiS->RelIO+4, i, sisReg->VBPart1[i]);
	}
	for (i=0; i<0x46; i++)  {
		inSISIDXREG(pSiS->RelIO+0x10, i, sisReg->VBPart2[i]);
	}
	for (i=0; i<0x3F; i++)  {
		inSISIDXREG(pSiS->RelIO+0x12, i, sisReg->VBPart3[i]);
	}
	for (i=0; i<0x1C; i++)  {
		inSISIDXREG(pSiS->RelIO+0x14, i, sisReg->VBPart4[i]);
	}

    outb(vgaIOBase + 4, Offset);
    sisReg->sisRegs3x4[Offset] = inb(VGA_SEQ_DATA);
    
    sisReg->sisRegs3C2[0x00] = inb(0x3CC);

    outw(VGA_SEQ_INDEX, (temp << 8) | 0x05); /* Relock Registers */
}

unsigned int
SiSddc1Read(ScrnInfoPtr pScrn)
{
#if 0
    SISPtr pSiS = SISPTR(pScrn);
#endif
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    unsigned char temp, temp2;

    outb(VGA_SEQ_INDEX, 0x05); /* Unlock Registers */
    temp2 = inb(VGA_SEQ_DATA);
    outw(VGA_SEQ_INDEX, 0x8605);

    /* Wait until vertical retrace is in progress. */
    while (inb(vgaIOBase + 0xA) & 0x08);
    while (!(inb(vgaIOBase + 0xA) & 0x08));

    /* Get the result */
    outb(VGA_SEQ_INDEX, 0x11); temp = inb(VGA_SEQ_DATA);

    outw(VGA_SEQ_INDEX, (temp2 << 8) | 0x05);

    return ((temp & 0x02)>>1);
}

/* Auxiliary function to find real memory clock (in Khz) */
int
SiSMclk(SISPtr pSiS)
{ int mclk;
  unsigned char Num, Denum, Base;

    /* Numerator */
    switch (pSiS->Chipset)  {
    case PCI_CHIP_SG86C201:
    case PCI_CHIP_SG86C202:
    case PCI_CHIP_SG86C205:
    case PCI_CHIP_SG86C215:
    case PCI_CHIP_SG86C225:
    case PCI_CHIP_SIS5597:
    case PCI_CHIP_SIS6326:
    case PCI_CHIP_SIS530:
	read_xr(MemClock0,Num);
	mclk=14318*((Num & 0x7f)+1);

	/* Denumerator */
	read_xr(MemClock1,Denum);
	mclk=mclk/((Denum & 0x1f)+1);

	/* Divider. Don't seems to work for mclk */
	if ( (Num & 0x80)!=0 ) { 
	    mclk = mclk*2;
	}

	/* Post-scaler. Values depends on SR13 bit 7  */
	outb(VGA_SEQ_INDEX, ClockBase); 
	Base = inb(VGA_SEQ_DATA);

	if ( (Base & 0x80)==0 ) {
	    mclk = mclk / (((Denum & 0x60) >> 5)+1);
	}
	else {
	    /* Values 00 and 01 are reserved */
	    if ((Denum & 0x60) == 0x40) { mclk=mclk/6; }
	    if ((Denum & 0x60) == 0x60) { mclk=mclk/8; }
	}
	break;
    case PCI_CHIP_SIS300:
    case PCI_CHIP_SIS540:
    case PCI_CHIP_SIS630:
	/* Numerator */
	read_xr(0x28, Num);
	mclk = 14318*((Num &0x7f)+1);

	/* Denumerator */
	read_xr(0x29, Denum);
	mclk = mclk/((Denum & 0x1f)+1);

	/* Divider */
	if ((Num & 0x80)!=0)  {
	    mclk = mclk * 2;
	}

	/* Post-Scaler */
	if ((Denum & 0x80)==0)  {
	    mclk = mclk / (((Denum & 0x60) >> 5) + 1);
	}  
	else  {
	    mclk = mclk / ((((Denum & 0x60) >> 5) + 1) * 2);
	}
	break;
    default:
	mclk = 0;
    }

    return(mclk);
}

/***** Only for SiS 5597 / 6326 *****/
/* Returns estimated memory bandwidth in Kbits/sec (for dotclock defaults)        */
/* Currently, a very rough estimate (4 cycles / read ; 2 for fast_vram) */
int sisMemBandWidth(ScrnInfoPtr pScrn)
{ int band;
  SISPtr pSiS = SISPTR(pScrn);
  SISRegPtr pReg = &pSiS->ModeReg;

     band=pSiS->MemClock;

   if (((pReg->sisRegs3C4[Mode64] >> 1) & 3) == 0) /* Only 1 bank Vram */
     band = (band * 8);
   else
     band = (band * 16);

   if ((pReg->sisRegs3C4[ExtMiscCont5] & 0xC0) == 0xC0) band=band*2;
     
   return(band);
}

const float	magic300[4] = { 1.2, 1.368421, 2.263158, 1.2};
const float	magic630[4] = { 1.441177, 1.441177, 2.588235, 1.441177 };
int sis300MemBandWidth(ScrnInfoPtr pScrn)
{
	SISPtr		pSiS = SISPTR(pScrn);
	int		bus = 64;
	int		mclk = pSiS->MemClock;
	int		bpp = pScrn->bitsPerPixel;
	float		magic;

	if (pSiS->Chipset==PCI_CHIP_SIS300)
		magic = magic300[bus/64];
	else
		magic = magic630[bus/64];

ErrorF("mclk: %d, bus: %d, magic: %g, bpp: %d\n", mclk, bus, magic, bpp);
	return  (int)(mclk*bus/magic/bpp);
}

void SiSIODump(ScrnInfoPtr pScrn)
{	SISPtr		pSiS = SISPTR(pScrn);
	int	i, max3c4, min3d4, max3d4;
	int	SR5State;
	unsigned char	temp;

    switch (pSiS->Chipset)  {
	case PCI_CHIP_SIS6326:
	    max3c4 = 0x3F;
	    max3d4 = 0x19;
	    min3d4 = 0x26;
	    break;
	case PCI_CHIP_SIS530:
	    max3c4 = 0x3F;
	    max3d4 = 0x19;
	    min3d4 = 0x26;
	    break;
	case PCI_CHIP_SIS300:
	case PCI_CHIP_SIS630:
	case PCI_CHIP_SIS540:
	    max3c4 = 0x3D;
	    max3d4 = 0x37;
	    min3d4 = 0x30;
	    break;
	default:
	    max3c4 = 0x38;
	    max3d4 = 0x19;
	    min3d4 = 0x26;
    }
    /* dump Misc Registers */
    temp = inb(0x3CC);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Misc Output 3CC=%x\n", temp);
    temp = inb(0x3CA);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Feature Control 3CA=%x\n", temp);

    /* Dump GR */
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "-------------\n");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Registers 3CE\n");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "-------------\n");
    for (i=0; i<=8; i++)  {
	outb(0x3ce, i);
	temp = inb(0x3cf);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[%2x]=%2x\n", i, temp);
    }

    /* dump SR0 ~ SR4 */
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "-------------\n");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Registers 3C4\n");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "-------------\n");
    for (i=0; i<=4; i++)  {
	outb(0x3c4, i);
	temp = inb(0x3c5);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[%2x]=%2x\n", i, temp);
    }

    /* dump extended SR */
    outb(0x3c4, 5);
    SR5State = inb(0x3c5);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[05]=%2x\n", SR5State);
    outw(0x3c4, 0x8605);
    for (i=6; i<=max3c4; i++)  {
	outb(0x3c4, i);
	temp = inb(0x3c5);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[%2x]=%2x\n", i, temp);
    }

    /* dump CR0 ~ CR18 */
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "-------------\n");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Registers 3D4\n");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "-------------\n");
    for (i=0; i<=0x18; i++)  {
	outb(0x3d4, i);
	temp = inb(0x3d5);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[%2x]=%2x\n", i, temp);
    }
    for (i=min3d4; i<=max3d4; i++)  {	/* dump extended CR */
	outb(0x3d4, i);
	temp = inb(0x3d5);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[%2x]=%2x\n", i, temp);
    }
    outw(0x3c4, SR5State << 8 | 0x05);
}

