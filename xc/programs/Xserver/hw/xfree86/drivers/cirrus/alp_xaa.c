/* (c) Itai Nahshon */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/alp_xaa.c,v 1.2 2000/02/08 13:13:14 eich Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "cir.h"
#include "alp.h"

#define WAIT   outb(0x3CE,0x31); while(inb(0x3CF) & pCir->chip.alp->waitMsk){};
#define WAIT_1 outb(0x3CE,0x31); while(inb(0x3CF) & 0x1){};

static void AlpSync(ScrnInfoPtr pScrn)
{
#ifdef ALP_DEBUG
	ErrorF("AlpSync\n");
#endif
	WAIT_1;
	return;
}

static void
AlpSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir, int ydir,
								int rop, unsigned int planemask,
								int trans_color)
{
	CirPtr pCir = CIRPTR(pScrn);
	int pitch = pCir->pitch;

#ifdef ALP_DEBUG
	ErrorF("AlpSetupForScreenToScreenCopy xdir=%d ydir=%d rop=%x planemask=%x trans_color=%x\n",
		xdir, ydir, rop, planemask, trans_color);
#endif
	WAIT;
	outw(0x3CE,0x0d32);
	/* Set dest pitch */
	outw(0x3CE, ((pitch << 8) & 0xff00) | 0x24);
	outw(0x3CE, ((pitch) & 0x1f00) | 0x25);
	/* Set source pitch */
	outw(0x3CE, ((pitch << 8) & 0xff00) | 0x26);
	outw(0x3CE, ((pitch) & 0x1f00) | 0x27);
}

static void
AlpSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1, int x2,
								int y2, int w, int h)
{
	CirPtr pCir = CIRPTR(pScrn);
	int source, dest;
	int  hh, ww;
	int decrement = 0;
	int pitch = pCir->pitch;

	ww = (w * pScrn->bitsPerPixel / 8) - 1;
	hh = h - 1;
	dest = y2 * pitch + x2 * pScrn->bitsPerPixel / 8;
	source = y1 * pitch + x1 * pScrn->bitsPerPixel / 8;
	if (dest > source) {
		decrement = 1;
		dest += hh * pitch + ww;
		source += hh * pitch + ww;
	}

	WAIT;

	outb(0x3CE, 0x30);
	outb(0x3CF, decrement);
	outb(0x3CE, 0x31);

	/* Width */
	outw(0x3CE, ((ww << 8) & 0xff00) | 0x20);
	outw(0x3CE, ((ww) & 0x1f00) | 0x21);
	/* Height */
	outw(0x3CE, ((hh << 8) & 0xff00) | 0x22);
	outw(0x3CE, ((hh) & 0x0700) | 0x23);


	/* source */
	outw(0x3CE, ((source << 8) & 0xff00) | 0x2C);
	outw(0x3CE, ((source) & 0xff00) | 0x2D);
	outw(0x3CE, ((source >> 8) & 0x3f00)| 0x2E);

	/* dest */
	outw(0x3CE, ((dest  << 8) & 0xff00) | 0x28);
	outw(0x3CE, ((dest) & 0xff00) | 0x29);
	outw(0x3CE, ((dest >> 8) & 0x3f00) | 0x2A);
	if (!pCir->chip.alp->autoStart)
	  outw(0x3CE,0x0231);

#ifdef ALP_DEBUG
	ErrorF("AlpSubsequentScreenToScreenCopy x1=%d y1=%d x2=%d y2=%d w=%d h=%d\n",
			x1, y1, x2, y2, w, h);
	ErrorF("AlpSubsequentScreenToScreenCopy s=%d d=%d ww=%d hh=%d\n",
			source, dest, ww, hh);
#endif

}

static void
AlpSetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
						unsigned int planemask)
{
	CirPtr pCir = CIRPTR(pScrn);
	int pitch = pCir->pitch;

#ifdef ALP_DEBUG
	ErrorF("AlpSetupForSolidFill color=%x rop=%x planemask=%x\n",
			color, rop, planemask);
#endif
	WAIT;

	outb(0x3CE, 0x32); outb(0x3CF, 0x0d);

	outb(0x3CE, 0x33);
	outb(0x3CF, 0x04);
	outb(0x3CE, 0x30);
	outb(0x3CF, 0xC0|((pScrn->bitsPerPixel - 8) << 1));

	outw(0x3CE, ((color << 8) & 0xff00) | 0x01);
	outw(0x3CE, ((color) & 0xff00) | 0x11);
	outw(0x3CE, ((color >> 8) & 0xff00) | 0x13);
	outw(0x3CE, 0x15);

	/* Set dest pitch */
	outw(0x3CE, ((pitch << 8) & 0xff00) | 0x24);
	outw(0x3CE, ((pitch) & 0x1f00) | 0x25);
}

static void
AlpSubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
	CirPtr pCir = CIRPTR(pScrn);
	int dest;
	int hh, ww;
	int pitch = pCir->pitch;

	ww = (w * pScrn->bitsPerPixel / 8) - 1;
	hh = h - 1;
	dest = y * pitch + x * pScrn->bitsPerPixel / 8;

	WAIT;

	/* Width */
	outw(0x3CE, ((ww << 8) & 0xff00) | 0x20);
	outw(0x3CE, ((ww) & 0x1f00) | 0x21);
	/* Height */
	outw(0x3CE, ((hh << 8) & 0xff00) | 0x22);
	outw(0x3CE, ((hh) & 0x0700) | 0x23);

	/* dest */
	outw(0x3CE, ((dest << 8) & 0xff00) | 0x28);
	outw(0x3CE, ((dest) & 0xff00) | 0x29);
	outw(0x3CE, ((dest >> 8) & 0x3f00) | 0x2A);
	if (!pCir->chip.alp->autoStart)
	  outw(0x3CE,0x0231);

#ifdef ALP_DEBUG
	ErrorF("AlpSubsequentSolidFillRect x=%d y=%d w=%d h=%d\n",
			x, y, w, h);
#endif

}


Bool
AlpXAAInit(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	CirPtr pCir = CIRPTR(pScrn);
	XAAInfoRecPtr XAAPtr;

#ifdef ALP_DEBUG
	ErrorF("AlpXAAInit\n");
#endif

	XAAPtr = XAACreateInfoRec();
	if (!XAAPtr) return FALSE;

	XAAPtr->SetupForScreenToScreenCopy = AlpSetupForScreenToScreenCopy;
	XAAPtr->SubsequentScreenToScreenCopy = AlpSubsequentScreenToScreenCopy;
	XAAPtr->ScreenToScreenCopyFlags = GXCOPY_ONLY|NO_TRANSPARENCY|NO_PLANEMASK;

	if (pCir->Chipset == PCI_CHIP_GD5446 ||
		pCir->Chipset == PCI_CHIP_GD5480) {

		XAAPtr->SetupForSolidFill = AlpSetupForSolidFill;
		XAAPtr->SubsequentSolidFillRect = AlpSubsequentSolidFillRect;
		XAAPtr->SubsequentSolidFillTrap = NULL;
		XAAPtr->SolidFillFlags = GXCOPY_ONLY|NO_TRANSPARENCY|NO_PLANEMASK;
	}

	XAAPtr->Sync = AlpSync;

	outw(0x3CE, 0x200E); /* enable writes to gr33 */
	if (pCir->properties & ACCEL_AUTOSTART) {
	  outw(0x3CE, 0x8031); /* enable autostart */
	  pCir->chip.alp->waitMsk = 0x10;
	  pCir->chip.alp->autoStart = TRUE;
	} else {
	  pCir->chip.alp->waitMsk = 0x1;
	  pCir->chip.alp->autoStart = FALSE;
	}
	if (!XAAInit(pScreen, XAAPtr))
		return FALSE;

	return TRUE;
}
