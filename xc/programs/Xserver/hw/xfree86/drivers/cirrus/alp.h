/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/alp.h,v 1.2 2000/02/08 13:13:13 eich Exp $ */

/* (c) Itai Nahshon */

#ifndef ALP_H
#define ALP_H

#undef ALP_DEBUG

/* Saved registers that are not part of the core VGA */
/* CRTC >= 0x19; Sequencer >= 0x05; Graphics >= 0x09; Attribute >= 0x15 */

enum {
	/* CR regs */
	CR1A,
	CR1B,
	CR1D,
	/* SR regs */
	SR07,
	SR0E,
	SR12,
	SR13,
	SR1E,
	/* GR regs */
	GR17,
	GR18,
	/* HDR */
	HDR,
	/* Must be last! */
	CIR_NSAVED
};

typedef struct {
	unsigned char	ExtVga[CIR_NSAVED];
} AlpRegRec, *AlpRegPtr;

extern Bool AlpHWCursorInit(ScreenPtr pScreen, int size);
extern Bool AlpXAAInit(ScreenPtr pScreen);
extern Bool AlpXAAInitMMIO(ScreenPtr pScreen);
extern Bool AlpDGAInit(ScreenPtr pScreen);
extern Bool AlpI2CInit(ScrnInfoPtr pScrn);

/* Card-specific driver information */
#define ALPPTR(p) ((AlpPtr)((p)->chip.alp))

typedef struct alpRec {
	unsigned char * HWCursorBits;
	unsigned char *	CursorBits;

	AlpRegRec		SavedReg;
	AlpRegRec		ModeReg;
        int                 CursorWidth;
        int                 CursorHeight;
        int                 waitMsk;
        Bool                autoStart;
/* XXX For XF86Config based mem configuration */
	CARD32			sr0f, sr17;
} AlpRec, *AlpPtr;

#endif /* ALP_H */

