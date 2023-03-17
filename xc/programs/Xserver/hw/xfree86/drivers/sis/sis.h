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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis.h,v 1.11 2000/02/12 23:07:55 dawes Exp $ */

#ifndef _SIS_H
#define _SIS_H_


#include "xf86Pci.h"
#include "xf86Cursor.h"
#include "compiler.h"
#include "xaa.h"

#define SIS_NAME		"SIS"
#define SIS_DRIVER_NAME		"sis"
#define SIS_MAJOR_VERSION	0
#define SIS_MINOR_VERSION	6
#define SIS_PATCHLEVEL		0
#define SIS_CURRENT_VERSION	((SIS_MAJOR_VERSION << 16) | \
				(SIS_MINOR_VERSION << 8) | SIS_PATCHLEVEL )

#define UMA			0x00000001
#define	MMIOMODE		0x00000001
#define	LFBQMODE		0x00000002
#define	AGPQMODE		0x00000004

#define	BIOS_BASE		0xC0000
#define	BIOS_SIZE		0x10000

#define	CRT2_LCD		0x00000001
#define	CRT2_TV			0x00000002
#define	CRT2_VGA		0x00000004

#ifdef	DEBUG
#define	PDEBUG(p)	p
#else
#define	PDEBUG(p)
#endif

typedef struct {
	unsigned char sisRegs3x4[0x100];
	unsigned char sisRegs3C4[0x100];
	unsigned char sisRegs3C2[0x100];
	unsigned char VBPart1[0x29];
	unsigned char VBPart2[0x46];
	unsigned char VBPart3[0x3F];
	unsigned char VBPart4[0x1C];
	unsigned char VBPart5[0x100];
} SISRegRec, *SISRegPtr;

#define SISPTR(p)	((SISPtr)((p)->driverPrivate))
#define XAAPTR(p)	((XAAInfoRecPtr)(SISPTR(p)->AccelInfoPtr))

typedef struct {
    ScrnInfoPtr		pScrn;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    EntityInfoPtr	pEnt;
    int			Chipset;
    int			ChipRev;
    CARD32		FbAddress;		/* VRAM physical address */
    unsigned char *	FbBase;			/* VRAM linear address */
    CARD32		IOAddress;		/* MMIO physical address */
    unsigned char *     IOBase;			/* MMIO linear address */
#ifdef __alpha__
    unsigned char *     IOBaseDense;		/* MMIO for Alpha platform */
#endif
    CARD16		RelIO;			/* Relocate IO Base */
    unsigned char *	BIOS;
    int			MemClock;
    int			BusWidth;
    int			MinClock;
    int			MaxClock;
    int			Flags;			/* HW config flags */
    long		FbMapSize;
    Bool		NoAccel;
    Bool		HWCursor;
    Bool		UsePCIRetry;
    Bool		TurboQueue;
    Bool		ValidWidth;
    Bool		FastVram;
    int			VBFlags;
    int			LCDFlags;
    int			TVFlags;
    short		scrnOffset;
    short		DstColor;
    int			Xdirection;
    int			Ydirection;
    int			sisPatternReg[4];
    int			ROPReg;
    int			CommandReg;
    int			MaxCMDQueueLen;
    int			CurCMDQueueLen;
    int			MinCMDQueueLen;
    int			DstX;
    int			DstY;
    unsigned char *	XAAScanlineColorExpandBuffers[2];
    CARD32		AccelFlags;
    Bool		ClipEnabled;
    Bool		DoColorExpand;
    SISRegRec		SavedReg;
    SISRegRec		ModeReg;
    xf86CursorInfoPtr	CursorInfoPtr;
    XAAInfoRecPtr	AccelInfoPtr;
    CloseScreenProcPtr	CloseScreen;
    unsigned int	(*ddc1Read)(ScrnInfoPtr);
    Bool		(*FindThreshold)(ScrnInfoPtr pScrn, int mclk, int vclk,
					int bpp, int buswidth, int flags,
					int *ThresaholdLow, int *ThresholdHigh);
    Bool		(*ModeInit)(ScrnInfoPtr pScrn, DisplayModePtr mode);
    Bool		(*ModeInit2)(ScrnInfoPtr pScrn, DisplayModePtr mode);
    void		(*SiSSave)(ScrnInfoPtr pScrn, SISRegPtr sisreg);
    void		(*SiSSave1)(ScrnInfoPtr pScrn, SISRegPtr sisreg);
    void		(*SiSRestore)(ScrnInfoPtr pScrn, SISRegPtr sisreg);
    void		(*SiSRestore1)(ScrnInfoPtr pScrn, SISRegPtr sisreg);
} SISRec, *SISPtr;

/* Prototypes */

void	SiSOptions(ScrnInfoPtr pScrn);
void	SiSVGASetup(ScrnInfoPtr pScrn);
void	SiSLCDPreInit(ScrnInfoPtr pScrn);
void	SiSTVPreInit(ScrnInfoPtr pScrn);
OptionInfoPtr SISAvailableOptions(int chipid, int busid);


int compute_vclk(int Clock, int *out_n, int *out_dn, int *out_div,
					int *out_sbit, int *out_scale);
void SiSCalcClock(ScrnInfoPtr pScrn, int clock, int max_VLD,
					unsigned int *vclk);
unsigned int SiSddc1Read(ScrnInfoPtr pScrn);
void SiSRestore(ScrnInfoPtr pScrn, SISRegPtr sisReg);
void SiSSave(ScrnInfoPtr pScrn, SISRegPtr sisReg);
Bool SiSAccelInit(ScreenPtr pScreen);
Bool SiS530AccelInit(ScreenPtr pScreen);
Bool SiS300AccelInit(ScreenPtr pScreen);
int  SiSMclk(SISPtr pSIS);
int sisMemBandWidth(ScrnInfoPtr pScrn);
int sis300MemBandWidth(ScrnInfoPtr pScrn);
Bool SiSHWCursorInit(ScreenPtr pScreen);
void SiSIODump(ScrnInfoPtr pScreen);
void SiSInitializeAccelerator(ScrnInfoPtr pScrn);
void SiSSetup(ScrnInfoPtr pScrn);

#endif
