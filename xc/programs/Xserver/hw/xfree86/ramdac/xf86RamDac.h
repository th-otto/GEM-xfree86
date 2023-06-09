/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/xf86RamDac.h,v 1.10 1999/06/14 07:32:09 dawes Exp $ */

#ifndef _XF86RAMDAC_H
#define _XF86RAMDAC_H 1

#include "xf86Cursor.h"

/* Define unique vendor codes for RAMDAC's */
#define VENDOR_IBM	0x0000
#define VENDOR_BT	0x0001
#define VENDOR_TI	0x0002

typedef struct _RamDacRegRec {
/* This is probably the nastiest assumption, we allocate 1024 slots for
 * ramdac registers, should be enough. I've checked IBM and TVP series 
 * and they seem o.k 
 * Then we allocate 768 entries for the DAC too. IBM640 needs 1024 -FIXME
 */
    unsigned short DacRegs[0x400];	/* register set */
    unsigned char DAC[0x300];		/* colour map */
    Bool Overlay;
} RamDacRegRec, *RamDacRegRecPtr;

typedef struct _RamDacHWRegRec {
    RamDacRegRec	SavedReg;
    RamDacRegRec	ModeReg;
} RamDacHWRec, *RamDacHWRecPtr;

typedef struct _RamDacRec {
    CARD32 RamDacType;

    unsigned char (*ReadDAC)(
	ScrnInfoPtr pScrn,
	CARD32
    );

    void (*WriteDAC)(
	ScrnInfoPtr pScrn,
	CARD32,
	unsigned char,
	unsigned char
    );

    void (*WriteAddress)(
	ScrnInfoPtr pScrn,
	CARD32
    );

    void (*WriteData)(
	ScrnInfoPtr pScrn,
	unsigned char
    );

    void (*ReadAddress)(
	ScrnInfoPtr pScrn,
	CARD32
    );

    unsigned char (*ReadData)(
	ScrnInfoPtr pScrn
    );
} RamDacRec, *RamDacRecPtr;

typedef struct _RamDacHelperRec {
    CARD32 RamDacType;

    void (*Restore)(
	ScrnInfoPtr pScrn,
	RamDacRecPtr ramdacPtr,
	RamDacRegRecPtr ramdacReg
    );

    void (*Save)(
	ScrnInfoPtr pScrn,
	RamDacRecPtr ramdacPtr,
	RamDacRegRecPtr ramdacReg
    );

    void (*SetBpp)(
	ScrnInfoPtr pScrn,
	RamDacRegRecPtr ramdacReg
    );

    void (*HWCursorInit)(
	xf86CursorInfoPtr infoPtr
    );
} RamDacHelperRec, *RamDacHelperRecPtr;

#define RAMDACHWPTR(p) ((RamDacHWRecPtr)((p)->privates[RamDacGetHWIndex()].ptr))

typedef struct _RamdacScreenRec {
    RamDacRecPtr	RamDacRec;
} RamDacScreenRec, *RamDacScreenRecPtr;
#define RAMDACSCRPTR(p) ((RamDacScreenRecPtr)((p)->privates[RamDacGetScreenIndex()].ptr))->RamDacRec

extern int RamDacHWPrivateIndex;
extern int RamDacScreenPrivateIndex;

typedef struct {
    int		token;
} RamDacSupportedInfoRec, *RamDacSupportedInfoRecPtr;

RamDacRecPtr RamDacCreateInfoRec(void);
RamDacHelperRecPtr RamDacHelperCreateInfoRec(void);
void RamDacDestroyInfoRec(RamDacRecPtr RamDacRec);
void RamDacHelperDestroyInfoRec(RamDacHelperRecPtr RamDacRec);
Bool RamDacInit(ScrnInfoPtr pScrn, RamDacRecPtr RamDacRec);
void RamDacSetGamma(ScrnInfoPtr pScrn, Bool Real8BitDac);
void RamDacRestoreDACValues(ScrnInfoPtr pScrn);
Bool RamDacHandleColormaps(ScreenPtr pScreen, int maxColors, int sigRGBbits,
			   unsigned int flags);
void RamDacFreeRec(ScrnInfoPtr pScrn);
int  RamDacGetHWIndex(void);

#endif /* _XF86RAMDAC_H */
