/*
 * Permedia 2 Xv Driver for Elsa Winner Office/2000 and GLoria Synergy cards
 *
 * Copyright (C) 1998, 1999 Michael Schimek <m.schimek@netway.at>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Michael Schimek not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. Michael Schimek makes no representations
 * about the suitability of this software for any purpose. It is provided
 * "as is" without express or implied warranty.
 *
 * MICHAEL SCHIMEK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL MICHAEL SCHIMEK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Kernel backbone, docs and test/demo applications at
 * <http://millennium.diads.com/mschimek/pm2>.
 *
 * Implementation note: The kernel backbone uses VSA, VSB and DMA (FIFO)
 * interrupts. It launches Input and Output FIFO DMAs when entered in
 * xvipcHandshake(). These resources are assumed to be available any time,
 * the server code must not touch them. All DMA will be completed before
 * returning from the kernel, no need to test.
 *
 * This driver is meant for the on-board video hardware only, no V4L
 * support in this file.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm2_video.c,v 1.14 1999/12/11 20:03:15 mvojkovi Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xf86Xinput.h"
#include "xf86fbman.h"
#include "xf86i2c.h"
#include "xf86xv.h"
#include "Xv.h"

#include "glint_regs.h"
#include "glint.h"

#define XVIPC_MAGIC		0x6A5D70E6
#define XVIPC_VERSION		1
#define VIDIOC_PM2_XVIPC	0x00007F7F

typedef enum {
	OP_ATTR = 0,
	OP_RESET = 8,		/* unused */
	OP_START,
	OP_STOP,
	OP_PLUG,
	OP_VIDEOSTD,
	OP_WINDOW,		/* unused */
	OP_CONNECT,
	OP_EVENT,
	OP_ALLOC,
	OP_FREE,
	OP_UPDATE,
	OP_NOP			/* ignored */
} xvipc_op;

typedef struct _pm2_xvipc {
	int			magic;
	void 			*pm2p, *pAPriv;
	int			port, op, time, block;
	int			a,b,c,d,e,f;
} pm2_xvipc;

static pm2_xvipc xvipc;
static int xvipc_fd = -1;

typedef enum {
    OPTION_DEVICE,
    OPTION_FPS,
    OPTION_BUFFERS,
    OPTION_EXPOSE
} OptToken;

static OptionInfoRec AdaptorOptions[] = {
    { OPTION_DEVICE,		"Device",	OPTV_STRING,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};
static OptionInfoRec InputOptions[] = {
    { OPTION_BUFFERS,		"Buffers",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_FPS,		"FramesPerSec", OPTV_INTEGER,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};
static OptionInfoRec OutputOptions[] = {
    { OPTION_BUFFERS,		"Buffers",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_EXPOSE,		"Expose",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_FPS,		"FramesPerSec", OPTV_INTEGER,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

#define PCI_SUBSYSTEM_ID_WINNER_2000_P2C	0x0a311048
#define PCI_SUBSYSTEM_ID_GLORIA_SYNERGY_P2C	0x0a321048
#define PCI_SUBSYSTEM_ID_GLORIA_SYNERGY_P2A	0x0a351048
#define PCI_SUBSYSTEM_ID_WINNER_2000_P2A	0x0a441048

#define SAA7111_SLAVE_ADDRESS 0x48
#define SAA7125_SLAVE_ADDRESS 0x88

typedef struct {
    CARD32			xy, wh;			/* 16.0 16.0 dw */
    INT32			s, t;			/* 12.20 fp */
    short			y1, y2;
} CookieRec, *CookiePtr;

typedef struct _PortPrivRec {
    struct _AdaptorPrivRec *    pAdaptor;
    I2CDevRec                   I2CDev;
    int                         Plug;
    INT32			Attribute[8];		/* Brig, Con, Sat, Hue, Int, Filt */
    FBAreaPtr			pFBArea[3];
    int				Buffers;
    CARD32			BufferBase[3];		/* x1 */
    CARD32			BufferStride;		/* x1 */
    CARD32			BufferPProd;		/* PProd(BufferStride / 2) */
    INT32			vx, vy, vw, vh;		/* 12.10 fp; int */
    int				dx, dy, dw, dh;
    int				fw, fh;
    CookiePtr			pCookies;
    int				nCookies;
    INT32			dS, dT;			/* 12.20 fp */
    int				APO;
    int				FramesPerSec, FrameAcc;
    int				BkgCol;			/* RGB 5:6:5;5:6:5 */
    int				VideoOn;		/* No, Once, Yes */
    Bool			StreamOn;
} PortPrivRec, *PortPrivPtr;

typedef struct _LFBAreaRec {
    struct _LFBAreaRec *	Next;
    FBAreaPtr			pFBArea;
    int				Linear;
} LFBAreaRec, *LFBAreaPtr;

typedef struct _AdaptorPrivRec {
    struct _AdaptorPrivRec *	Next;
    ScrnInfoPtr			pScrn;
    void *			pm2p;
    LFBAreaPtr			LFBList;
    CARD32			FifoControl;
    OsTimerPtr			Timer;
    int                         VideoStd;
    int				FramesPerSec;
    int				FrameLines;
    int				IntLine;		/* Frame */
    int				LinePer;		/* nsec */
    PortPrivRec                 Port[2];
} AdaptorPrivRec, *AdaptorPrivPtr;

static AdaptorPrivPtr AdaptorPrivList = NULL;

#define PORTNUM(p) (((p) == &pAPriv->Port[0]) ? 0 : 1)
#define BPPSHIFT(g) (2 - (g)->BppShift)			/* BytesPerPixel = 1 << BPPSHIFT(pGlint) */

#undef COLORBARS /* Display them while output is unused, else just shut off */
#define DEBUG(x) /* x */

#define FreeCookies(pPPriv)		\
do {					\
    if ((pPPriv)->pCookies) {		\
        xfree((pPPriv)->pCookies);	\
	(pPPriv)->pCookies = NULL;	\
    }					\
} while (0)

/* Forward */
static CARD32 TimerCallback(OsTimerPtr pTim, CARD32 now, pointer p);
static void DelayedStopVideo(PortPrivPtr pPPriv);
static Bool xvipcHandshake(PortPrivPtr pPPriv, int op, Bool block);

#define XV_ENCODING	"XV_ENCODING"
#define XV_BRIGHTNESS  	"XV_BRIGHTNESS"
#define XV_CONTRAST 	"XV_CONTRAST"
#define XV_SATURATION  	"XV_SATURATION"
#define XV_HUE		"XV_HUE"
/* Proprietary */
#define XV_INTERLACE	"XV_INTERLACE"			/* Integer */
#define XV_FILTER	"XV_FILTER"			/* Boolean */
#define XV_BKGCOLOR	"XV_BKGCOLOR"			/* Integer */

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvEncoding, xvBrightness, xvContrast, xvSaturation, xvHue;
static Atom xvInterlace, xvFilter, xvBkgColor;

static XF86VideoEncodingRec
InputVideoEncodings[] =
{
    { 0, "pal-composite",		704, 576, { 1, 50 }},
    { 1, "pal-composite_adaptor",	704, 576, { 1, 50 }},
    { 2, "pal-svideo",			704, 576, { 1, 50 }},
    { 3, "ntsc-composite",		704, 480, { 1001, 60000 }},
    { 4, "ntsc-composite_adaptor",	704, 480, { 1001, 60000 }},
    { 5, "ntsc-svideo",			704, 480, { 1001, 60000 }},
    { 6, "secam-composite",		704, 576, { 1, 50 }},
    { 7, "secam-composite_adaptor",	704, 576, { 1, 50 }},
    { 8, "secam-svideo",		704, 576, { 1, 50 }},
};

static XF86VideoEncodingRec
OutputVideoEncodings[] =
{
    { 0, "pal-composite_adaptor",	704, 576, { 1, 50 }},
    { 1, "pal-svideo",			704, 576, { 1, 50 }},
    { 2, "ntsc-composite_adaptor",	704, 480, { 1001, 60000 }},
    { 3, "ntsc-svideo",			704, 480, { 1001, 60000 }},
};

static XF86VideoFormatRec
InputVideoFormats[] = {
    { 8,  TrueColor }, /* Dithered */
    { 15, TrueColor },
    { 16, TrueColor },
    { 24, TrueColor },
};

static XF86VideoFormatRec
OutputVideoFormats[] = {
    { 8,  TrueColor },
    { 8,  PseudoColor }, /* Using .. */
    { 8,  StaticColor },
    { 8,  GrayScale },
    { 8,  StaticGray }, /* .. TexelLUT */
    { 15, TrueColor },
    { 16, TrueColor },
    { 24, TrueColor },
};

static I2CByte
DecInitVec[] =
{
    0x11, 0x00,
    0x02, 0xC1, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00,
    0x06, 0x00, 0x07, 0x00, 0x08, 0x00, 0x09, 0x4A,
    0x0A, 0x80, 0x0B, 0x40, 0x0C, 0x40, 0x0D, 0x00,
    0x0E, 0x01, 0x10, 0xC8, 0x12, 0x20,
    0x13, 0x00, 0x15, 0x00, 0x16, 0x00, 0x17, 0x00,
};

static I2CByte
EncInitVec[] =
{
    0x3A, 0x83, 0x61, 0xC2,
    0x5A, 119,  0x5B, 0x7D,
    0x5C, 0xAF, 0x5D, 0x3C, 0x5E, 0x3F, 0x5F, 0x3F,
    0x60, 0x70, 0x62, 0x4B, 0x67, 0x00,
    0x68, 0x00, 0x69, 0x00, 0x6A, 0x00, 0x6B, 0x20,
    0x6C, 0x03, 0x6D, 0x30, 0x6E, 0xA0, 0x6F, 0x00,
    0x70, 0x80, 0x71, 0xE8, 0x72, 0x10,
    0x7A, 0x13, 0x7B, 0xFB, 0x7C, 0x00, 0x7D, 0x00,
};

static I2CByte Dec02[3] = { 0xC1, 0xC0, 0xC4 };
static I2CByte Dec09[3] = { 0x4A, 0x4A, 0xCA };
static I2CByte Enc3A[3] = { 0x03, 0x03, 0x23 };
static I2CByte Enc61[3] = { 0x06, 0x01, 0xC2 };

static I2CByte
DecVS[3][8] =
{
    { 0x06, 108, 0x07, 108, 0x08, 0x09, 0x0E, 0x01 },
    { 0x06, 107, 0x07, 107, 0x08, 0x49, 0x0E, 0x31 },
    { 0x06, 108, 0x07, 108, 0x08, 0x01, 0x0E, 0x51 }
};

#define FSC(n) ((CARD32)((n) / 27e6 * 4294967296.0 + .5))
#define SUBCARRIER_FREQ_PAL  (4.433619e6)
#define SUBCARRIER_FREQ_NTSC (3.579545e6)

static I2CByte
EncVS[2][14] =
{
    { 0x62, 0x4B, 0x6B, 0x28, 0x6E, 0xA0,
      0x63, (I2CByte)(FSC(SUBCARRIER_FREQ_PAL) >> 0),
      0x64, (I2CByte)(FSC(SUBCARRIER_FREQ_PAL) >> 8),
      0x65, (I2CByte)(FSC(SUBCARRIER_FREQ_PAL) >> 16),
      0x66, (I2CByte)(FSC(SUBCARRIER_FREQ_PAL) >> 24) },
    { 0x62, 0x6A, 0x6B, 0x20, 0x6E, 0x20,
      0x63, (I2CByte)(FSC(SUBCARRIER_FREQ_NTSC) >> 0),
      0x64, (I2CByte)(FSC(SUBCARRIER_FREQ_NTSC) >> 8),
      0x65, (I2CByte)(FSC(SUBCARRIER_FREQ_NTSC) >> 16),
      0x66, (I2CByte)(FSC(SUBCARRIER_FREQ_NTSC) >> 24) }
};

#undef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b)) 

#undef CLAMP
#define CLAMP(v, min, max) (((v) < (min)) ? (min) : MIN(v, max))

#undef ABS
#define ABS(n) (((n) < 0) ? -(n) : (n))

static int
SetAttr(PortPrivPtr pPPriv, int i, I2CByte s, I2CByte v)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    if (pAPriv->pm2p) {
	xvipc.a = v << 8;
	return xvipcHandshake(pPPriv, OP_ATTR + i, TRUE) ?
	    Success : XvBadAlloc;
    }

    return xf86I2CWriteByte(&pPPriv->I2CDev, s, v) ?
	Success : XvBadAlloc;
}

static Bool
SetPlug(PortPrivPtr pPPriv)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    if (pAPriv->pm2p) {
	xvipc.a = pPPriv->Plug - PORTNUM(pPPriv);
	return xvipcHandshake(pPPriv, OP_PLUG, TRUE);
    } else {
	if (pPPriv == &pAPriv->Port[0]) {
	    xf86I2CWriteByte(&pPPriv->I2CDev, 0x02, Dec02[pPPriv->Plug]);
	    xf86I2CWriteByte(&pPPriv->I2CDev, 0x09, Dec09[pPPriv->Plug]);
	} else {
	    if (pPPriv->StreamOn)
		xf86I2CWriteByte(&pPPriv->I2CDev, 0x3A, Enc3A[pPPriv->Plug]);
#ifdef COLORBARS
	    else
		xf86I2CWriteByte(&pPPriv->I2CDev, 0x3A, 0x83);
#endif
	}
    }

    return TRUE;
}

static Bool
SetVideoStd(AdaptorPrivPtr pAPriv)
{
    Bool r;

    if (pAPriv->pm2p) {
	xvipc.a = pAPriv->VideoStd;
	r = xvipcHandshake(&pAPriv->Port[0], OP_VIDEOSTD, TRUE);
	pAPriv->VideoStd = xvipc.a; /* Actual */
    } else {
	if (pAPriv->VideoStd >= 2)
	    xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, 0xC2);

	xf86I2CWriteVec(&pAPriv->Port[0].I2CDev, &DecVS[pAPriv->VideoStd][0], 4);

	if (pAPriv->VideoStd < 2)
	    xf86I2CWriteVec(&pAPriv->Port[1].I2CDev, &EncVS[pAPriv->VideoStd][0], 7);

	r = TRUE;
    }

    if (pAPriv->VideoStd == 1) {
	pAPriv->FramesPerSec = 30;
	pAPriv->FrameLines = 525;
	pAPriv->IntLine = 513;
	pAPriv->LinePer = 63555;
    } else {
	pAPriv->FramesPerSec = 25;
	pAPriv->FrameLines = 625;
	pAPriv->IntLine = 613;
	pAPriv->LinePer = 64000;
    }

#if 0
    pAPriv->Port[0].FramesPerSec = pAPriv->FramesPerSec;
    pAPriv->Port[1].FramesPerSec = pAPriv->FramesPerSec;
#endif

    return r;
}

/* **FIXME**: 2048 limit */

static Bool
AllocOffRec(PortPrivPtr pPPriv, int i)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int width, height;

    height = (pAPriv->VideoStd == 1) ? 512 : 608;

    if (!pPPriv->Attribute[4])
	height >>= 1;

    width = (704 << 4) / pScrn->bitsPerPixel;

    pPPriv->BufferStride = pScrn->displayWidth << BPPSHIFT(pGlint);

    if (pPPriv->pFBArea[i] != NULL) {
	if (xf86ResizeOffscreenArea(pPPriv->pFBArea[i], width, height))
	    return TRUE;

	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 2,
	    "AOR couldn't resize buffer %d,%d-%d,%d to %dx%d\n",
	    pPPriv->pFBArea[i]->box.x1, pPPriv->pFBArea[i]->box.y1,
	    pPPriv->pFBArea[i]->box.x2, pPPriv->pFBArea[i]->box.y2,
	    width, height));

	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
	    "AOR free buffer 0x%08x\n", pPPriv->BufferBase[i]));

	xf86FreeOffscreenArea(pPPriv->pFBArea[i]);
	pPPriv->pFBArea[i] = NULL;
    }

    pPPriv->pFBArea[i] = xf86AllocateOffscreenArea(pScrn->pScreen,
        width, height, 2, NULL, NULL, NULL);

    if (pPPriv->pFBArea[i] != NULL)
        return TRUE;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 2,
        "AOR couldn't allocate buffer %dx%d\n",
	width, height));

    return FALSE;
}

static Bool
AllocOffLin(PortPrivPtr pPPriv, int i)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int length;
    
    length = ((704 << 1) * ((pAPriv->VideoStd == 1) ? 512 : 608)) >> BPPSHIFT(pGlint);

    if (!pPPriv->Attribute[4])
	length >>= 1;

    pPPriv->BufferStride = 704 << 1;

    if (pPPriv->pFBArea[i] != NULL) {
	xf86FreeOffscreenArea(pPPriv->pFBArea[i]);
	pPPriv->pFBArea[i] = NULL;
    }

    pPPriv->pFBArea[i] = xf86AllocateLinearOffscreenArea(pScrn->pScreen,
        length, 2, NULL, NULL, NULL);

    if (pPPriv->pFBArea[i] != NULL)
	return TRUE;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 2,
	"AOL couldn't allocate buffer %d\n",
	length));

    return FALSE;
}

static Bool
ReallocateOffscreenBuffer(PortPrivPtr pPPriv, int num)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int i;

    if (pAPriv->pm2p != NULL)
	return FALSE;

    for (i = 2; i >= 0; i--)
	if (pPPriv->pFBArea[i]) {
	    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
	        "ROB free buffer 0x%08x\n", pPPriv->BufferBase[i]));

	    xf86FreeOffscreenArea(pPPriv->pFBArea[i]);
	    pPPriv->pFBArea[i] = NULL;
	}

    if (num <= 0)
	return FALSE;

    while (1) {
	PortPrivPtr pPPrivN = &pAPriv->Port[1 - PORTNUM(pPPriv)];

	for (i = 0; i < num; i++)
	    if (!AllocOffLin(pPPriv, i)) break;
	if (i > 0) break;

	for (i = 0; i < num; i++)
	    if (!AllocOffRec(pPPriv, i)) break;
	if (i > 0) break;

	if (pPPrivN->VideoOn <= 0 && pPPrivN->APO >= 0)
	    DelayedStopVideo(pPPrivN);
	else
	    return FALSE;
    }

    for (i = 0; i <= 2; i++)
	if (pPPriv->pFBArea[i]) {
	    pPPriv->BufferBase[i] =
		((pPPriv->pFBArea[i]->box.y1 * pScrn->displayWidth) +
	         pPPriv->pFBArea[i]->box.x1) << BPPSHIFT(pGlint);

	    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
		"ROB buffer #%d 0x%08x str %d\n",
		i, pPPriv->BufferBase[i], pPPriv->BufferStride));
	}

    pPPriv->BufferPProd = partprodPermedia[(pPPriv->BufferStride / 2) >> 5];

    pPPriv->fw = 704;
    pPPriv->fh = InputVideoEncodings[pAPriv->VideoStd * 3].height >>
	(!pPPriv->Attribute[4]);

    return TRUE;
}

static Bool
PutCookies(PortPrivPtr pPPriv, RegionPtr pRegion)
{
    BoxPtr pBox;
    CookiePtr pCookie;
    int nBox;

    if (!pRegion) {
	pBox = (BoxPtr) NULL;
	nBox = pPPriv->nCookies;
    } else {
	pBox = REGION_RECTS(pRegion);
	nBox = REGION_NUM_RECTS(pRegion);

	if (!pPPriv->pCookies || pPPriv->nCookies < nBox) {
	    if (!(pCookie = (CookiePtr) xrealloc(pPPriv->pCookies, nBox * sizeof(CookieRec))))
    		return FALSE;

	    pPPriv->pCookies = pCookie;
	}
    }

    pPPriv->dS = (pPPriv->vw << 10) / pPPriv->dw;
    pPPriv->dT = (pPPriv->vh << 10) / pPPriv->dh;

    for (pCookie = pPPriv->pCookies; nBox--; pCookie++, pBox++) {
	if (pRegion) {
	    pCookie->y1 = pBox->y1;
	    pCookie->y2 = pBox->x1;
	    pCookie->xy = (pBox->y1 << 16) | pBox->x1;
	    pCookie->wh = ((pBox->y2 - pBox->y1) << 16) |
			   (pBox->x2 - pBox->x1);
	}

	pCookie->s = (pPPriv->vx << 10) + (pCookie->y2 - pPPriv->dx) * pPPriv->dS;
	pCookie->t = (pPPriv->vy << 10) + (pCookie->y1 - pPPriv->dy) * pPPriv->dT;
    }

    pPPriv->nCookies = pCookie - pPPriv->pCookies;

    return TRUE;
}

static void
PutYUV(PortPrivPtr pPPriv, int BufferBase)
{
    ScrnInfoPtr pScrn = pPPriv->pAdaptor->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    CookiePtr pCookie = pPPriv->pCookies;
    int nCookies = pPPriv->nCookies;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4, "PutYUV %08x\n", BufferBase));

    if (!nCookies)
	return;

    /* Setup */

    GLINT_WAIT(25);
    CHECKCLIPPING;

    switch (pScrn->depth) {
	case 8:
	    GLINT_WRITE_REG((0 << 10) | 	/* BGR */
			    (1 << 1) | 		/* Dither */
			    ((5 & 0x10)<<12) |
			    ((5 & 0x0F)<<2) | 	/* 3:3:2f */
			    UNIT_ENABLE, DitherMode);
	    break;
	case 15:
	    GLINT_WRITE_REG((1 << 10) | 	/* RGB */
			    ((1 & 0x10)<<12)|
			    ((1 & 0x0F)<<2) | 	/* 5:5:5:1f */
			    UNIT_ENABLE, DitherMode);
	    break;
	case 16:
	    GLINT_WRITE_REG((1 << 10) | 	/* RGB */
			    ((16 & 0x10)<<12)|
			    ((16 & 0x0F)<<2) | 	/* 5:6:5f */
			    UNIT_ENABLE, DitherMode);
	    break;
	case 24:
	    GLINT_WRITE_REG((1 << 10) |		/* RGB */
			    ((0 & 0x10)<<12)|
			    ((0 & 0x0F)<<2) | 	/* 8:8:8:8 */
			    UNIT_ENABLE, DitherMode);
	    break;
	default:
	    return; /* Oops */
    }

    GLINT_WRITE_REG(1 << 16, dY);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(UNIT_DISABLE, AreaStippleMode);
    GLINT_WRITE_REG(BufferBase >> 1 /* 16 */, PMTextureBaseAddress);
    GLINT_WRITE_REG((1 << 19) /* 16 */ | pPPriv->BufferPProd,
		    PMTextureMapFormat);
    GLINT_WRITE_REG((1 << 4) /* No alpha */ |
	            ((19 & 0x10) << 2) |
		    ((19 & 0x0F) << 0) /* YUYV */,
		    PMTextureDataFormat);
    GLINT_WRITE_REG((pPPriv->Attribute[5] << 17) | /* FilterMode */
		    (11 << 13) | (11 << 9) | /* TextureSize log2 */ 
		    UNIT_ENABLE, PMTextureReadMode);
    GLINT_WRITE_REG((0 << 4) /* RGB */ |
		    (3 << 1) /* Copy */ |
		    UNIT_ENABLE, TextureColorMode);
    GLINT_WRITE_REG(UNIT_ENABLE, YUVMode);
    GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, LogicalOpMode);
    GLINT_WRITE_REG(UNIT_ENABLE, TextureAddressMode);
    GLINT_WRITE_REG(pPPriv->dS, dSdx);
    GLINT_WRITE_REG(0, dSdyDom);
    GLINT_WRITE_REG(0, dTdx);
    GLINT_WRITE_REG(pPPriv->dT, dTdyDom);

    /* Subsequent */

    for (; nCookies--; pCookie++) {
	GLINT_WAIT(5);
	GLINT_WRITE_REG(pCookie->xy, RectangleOrigin);
	GLINT_WRITE_REG(pCookie->wh, RectangleSize);
	GLINT_WRITE_REG(pCookie->s, SStart);
	GLINT_WRITE_REG(pCookie->t, TStart);
        GLINT_WRITE_REG(PrimitiveRectangle |
			XPositive |
			YPositive |
			TextureEnable, Render);
    }

    /* Cleanup */

    pGlint->x = pGlint->y = -1; /* Force reload */
    pGlint->w = pGlint->h = -1;
    pGlint->ROP = 0xFF;
    GLINT_WAIT(6);
    GLINT_WRITE_REG(pGlint->TexMapFormat, PMTextureMapFormat);
    GLINT_WRITE_REG(UNIT_DISABLE, DitherMode);
    GLINT_WRITE_REG(UNIT_DISABLE, TextureColorMode);
    GLINT_WRITE_REG(UNIT_DISABLE, TextureAddressMode);
    GLINT_WRITE_REG(UNIT_DISABLE, PMTextureReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, YUVMode);
}

static void
BlackOut(PortPrivPtr pPPriv, RegionPtr pRegion)
{
    ScrnInfoPtr pScrn = pPPriv->pAdaptor->pScrn;
    ScreenPtr pScreen = pScrn->pScreen;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    RegionRec DRegion;
    BoxRec DBox;
    BoxPtr pBox;
    int nBox;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
	"BlackOut %d,%d,%d,%d -- %d,%d,%d,%d\n",
	pPPriv->vx, pPPriv->vy, pPPriv->vw, pPPriv->vh,
	pPPriv->dx, pPPriv->dy, pPPriv->dw, pPPriv->dh));

    DBox.x1 = pPPriv->dx - (pPPriv->vx * pPPriv->dw) / pPPriv->vw;
    DBox.y1 = pPPriv->dy - (pPPriv->vy * pPPriv->dh) / pPPriv->vh;
    DBox.x2 = DBox.x1 + (pPPriv->fw * pPPriv->dw) / pPPriv->vw;
    DBox.y2 = DBox.y1 + (pPPriv->fh * pPPriv->dh) / pPPriv->vh;

    REGION_INIT(pScreen, &DRegion, &DBox, 1);

    if (pRegion)
	REGION_SUBTRACT(pScreen, &DRegion, &DRegion, pRegion);

    nBox = REGION_NUM_RECTS(&DRegion);
    pBox = REGION_RECTS(&DRegion);

    GLINT_WAIT(15);
    CHECKCLIPPING;

    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    GLINT_WRITE_REG(pPPriv->BufferPProd, FBReadMode);
    GLINT_WRITE_REG(0x1, FBReadPixel); /* 16 */
    GLINT_WRITE_REG(pPPriv->BkgCol, FBBlockColor);
    GLINT_WRITE_REG(pPPriv->BufferBase[0] >> 1 /* 16 */, FBWindowBase);
    GLINT_WRITE_REG(UNIT_DISABLE, LogicalOpMode);

    for (; nBox--; pBox++) {
        int w = ((pBox->x2 - pBox->x1) * pPPriv->vw + pPPriv->dw) / pPPriv->dw + 1;
	int h = ((pBox->y2 - pBox->y1) * pPPriv->vh + pPPriv->dh) / pPPriv->dh + 1;
	int x = ((pBox->x1 - DBox.x1) * pPPriv->vw + (pPPriv->dw >> 1)) / pPPriv->dw;
	int y = ((pBox->y1 - DBox.y1) * pPPriv->vh + (pPPriv->dh >> 1)) / pPPriv->dh;

	if ((x + w) > pPPriv->fw)
	    w = pPPriv->fw - x;
	if ((y + h) > pPPriv->fh)
	    h = pPPriv->fh - y;

	GLINT_WAIT(3);
	GLINT_WRITE_REG((y << 16) | x, RectangleOrigin);
	GLINT_WRITE_REG((h << 16) | w, RectangleSize);
	GLINT_WRITE_REG(PrimitiveRectangle |
	    XPositive | YPositive | FastFillEnable, Render);
    }

    REGION_UNINIT(pScreen, &DRegion);

    pGlint->x = pGlint->y = -1; /* Force reload */
    pGlint->w = pGlint->h = -1;
    pGlint->ROP = 0xFF;
    GLINT_WAIT(3);
    GLINT_WRITE_REG(0, FBWindowBase);
    GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    GLINT_WRITE_REG(pGlint->PixelWidth, FBReadPixel);
}

static Bool
GetCookies(PortPrivPtr pPPriv, RegionPtr pRegion)
{
    BoxPtr pBox;
    CookiePtr pCookie;
    int nBox;
    int dw1 = pPPriv->dw - 1;
    int dh1 = pPPriv->dh - 1;

    if (!pRegion) {
	pBox = (BoxPtr) NULL;
	nBox = pPPriv->nCookies;
    } else {
	pBox = REGION_RECTS(pRegion);
	nBox = REGION_NUM_RECTS(pRegion);

	if (!pPPriv->pCookies || pPPriv->nCookies < nBox) {
	    if (!(pCookie = (CookiePtr) xrealloc(pPPriv->pCookies, nBox * sizeof(CookieRec))))
    		return FALSE;
	
	    pPPriv->pCookies = pCookie;
	}
    }

    pPPriv->dS = (pPPriv->dw << 20) / pPPriv->vw;
    pPPriv->dT = (pPPriv->dh << 20) / pPPriv->vh;

    for (pCookie = pPPriv->pCookies; nBox--; pBox++) {
	int n1, n2;

	if (pRegion) {
	    n1 = ((pBox->x1 - pPPriv->dx) * pPPriv->vw + dw1) / pPPriv->dw;
                       	    n2 = ((pBox->x2 - pPPriv->dx) * pPPriv->vw - 1) / pPPriv->dw;
	    if (n1 > n2) continue; /* Clip is subpixel */
	    pCookie->xy = n1 + pPPriv->vx;
	    pCookie->wh = n2 - n1 + 1;
	    pCookie->s = n1 * pPPriv->dS + (pPPriv->dx << 20);
	    pCookie->y1 = pBox->y1;
	    pCookie->y2 = pBox->y2;
	}

	n1 = ((pCookie->y1 - pPPriv->dy) * pPPriv->vh + dh1) / pPPriv->dh;
	n2 = ((pCookie->y2 - pPPriv->dy) * pPPriv->vh - 1) / pPPriv->dh;
	pCookie->xy = (pCookie->xy & 0xFFFF) | ((n1 + pPPriv->vy) << 16);
	pCookie->wh = (pCookie->wh & 0xFFFF) | ((n2 - n1 + 1) << 16);
	pCookie->t = n1 * pPPriv->dT + (pPPriv->dy << 20);
	if (n1 > n2) pCookie->t = -1;

	pCookie++;
    }

    pPPriv->nCookies = pCookie - pPPriv->pCookies;
    return TRUE;
}

static void
GetYUV(PortPrivPtr pPPriv)
{
    ScrnInfoPtr pScrn = pPPriv->pAdaptor->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    CookiePtr pCookie = pPPriv->pCookies;
    int nCookies = pPPriv->nCookies;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4, "GetYUV\n"));

    if (!nCookies)
	return;

    /* FIXME? clip + filt d-1 */

    /* Setup */

    GLINT_WAIT(25);
    CHECKCLIPPING;

    switch (pScrn->depth) {
	case 8:
	    GLINT_WRITE_REG(UNIT_ENABLE, TexelLUTMode);
	    GLINT_WRITE_REG((1 << 4) | /* No alpha */
	        	    ((14 & 0x10) << 2) |
			    ((14 & 0x0F) << 0), /* CI8 */
			    PMTextureDataFormat);
	    break;

	case 15:	
	    GLINT_WRITE_REG((1 << 5) | /* RGB */
			    (1 << 4) |
			    ((1 & 0x10) << 2) |
			    ((1 & 0x0F) << 0), 	/* 5:5:5:1f */
			    PMTextureDataFormat);
	    break;
	case 16:
	    GLINT_WRITE_REG((1 << 5) |
			    (1 << 4) |
			    ((16 & 0x10) << 2) |
			    ((16 & 0x0F) << 0), /* 5:6:5f */
			    PMTextureDataFormat);
	    break;
	case 24:
	    GLINT_WRITE_REG((1 << 5) |
			    (1 << 4) |
			    ((0 & 0x10) << 2) |
			    ((0 & 0x0F) << 0),	/* 8:8:8:8 */
			    PMTextureDataFormat);
	    break;
	default:       
	    return;
    }

    GLINT_WRITE_REG(UNIT_DISABLE, AreaStippleMode);
    GLINT_WRITE_REG(0, PMTextureBaseAddress);
    GLINT_WRITE_REG((1 << 10) | /* RGB */
		    ((16 & 0x10)<<12)|
		    ((16 & 0x0F)<<2) | 	/* 5:6:5f */
		    UNIT_ENABLE, DitherMode);
    GLINT_WRITE_REG((pPPriv->Attribute[5] << 17) | /* FilterMode */
		    (11 << 13) | (11 << 9) | /* TextureSize log2 */
		    UNIT_ENABLE, PMTextureReadMode);
    GLINT_WRITE_REG((0 << 4) /* RGB */ |
		    (3 << 1) /* Copy */ |
		    UNIT_ENABLE, TextureColorMode);
    GLINT_WRITE_REG(0x1, FBReadPixel); /* 16 */
    GLINT_WRITE_REG(UNIT_DISABLE, YUVMode);
    GLINT_WRITE_REG(pPPriv->BufferPProd, FBReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, LogicalOpMode);
    GLINT_WRITE_REG(UNIT_ENABLE, TextureAddressMode);
    GLINT_WRITE_REG(pPPriv->dS, dSdx);
    GLINT_WRITE_REG(0, dSdyDom);
    GLINT_WRITE_REG(0, dTdx);
    GLINT_WRITE_REG(pPPriv->dT, dTdyDom);
    GLINT_WRITE_REG(1 << 16, dY);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(pPPriv->BufferBase[0] >> 1 /* 16 */, FBWindowBase);

    /* Subsequent */

    for (; nCookies--; pCookie++)
	if (pCookie->t >= 0) {
	    GLINT_WAIT(5);
	    GLINT_WRITE_REG(pCookie->xy, RectangleOrigin);
	    GLINT_WRITE_REG(pCookie->wh, RectangleSize);
	    GLINT_WRITE_REG(pCookie->s, SStart);
	    GLINT_WRITE_REG(pCookie->t, TStart);
    	    GLINT_WRITE_REG(PrimitiveRectangle |
			    XPositive |
			    YPositive |
			    TextureEnable, Render);
	}

    /* Cleanup */

    pGlint->x = pGlint->y = -1; /* Force reload */
    pGlint->w = pGlint->h = -1;
    pGlint->ROP = 0xFF;
    GLINT_WAIT(10);
    GLINT_WRITE_REG(0, FBWindowBase);
    GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    GLINT_WRITE_REG(pGlint->PixelWidth, FBReadPixel);
    GLINT_WRITE_REG(UNIT_DISABLE, DitherMode);
    GLINT_WRITE_REG(UNIT_DISABLE, TextureColorMode);
    GLINT_WRITE_REG(UNIT_DISABLE, TextureAddressMode);
    GLINT_WRITE_REG(pGlint->TexMapFormat, PMTextureMapFormat);
    GLINT_WRITE_REG(UNIT_DISABLE, TexelLUTMode);
    GLINT_WRITE_REG(UNIT_DISABLE, PMTextureReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, YUVMode);
}

static void
StopVideoStream(AdaptorPrivPtr pAPriv, int which)
{
    GLINTPtr pGlint = GLINTPTR(pAPriv->pScrn);

    if (which & 1) pAPriv->Port[0].VideoOn = 0;
    if (which & 2) pAPriv->Port[1].VideoOn = 0;

    if (pAPriv->pm2p) {
	/* Hard stop, frees buffers */

	if (which & 2) {
	    xvipcHandshake(&pAPriv->Port[1], OP_STOP, TRUE);
	    pAPriv->Port[1].StreamOn = FALSE;
	}

	if (which & 1) {
	    xvipcHandshake(&pAPriv->Port[0], OP_STOP, TRUE);
	    pAPriv->Port[0].StreamOn = FALSE;
	}

	return;
    }

    if (which & 2) {
    	xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x3A, 0x83);
#ifndef COLORBARS
	xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, 0xC2);
#endif
	GLINT_WRITE_REG(0, VSBBase + VSControl);
	pAPriv->Port[1].StreamOn = FALSE;
    }

    if (which & 1) {
	GLINT_WRITE_REG(0, VSABase + VSControl);
	pAPriv->Port[0].StreamOn = FALSE;
	usleep(80000);
    }

    if (!(pAPriv->Port[0].StreamOn || pAPriv->Port[1].StreamOn)) {
	if (which & 4)
	    xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, 0xC2);
	xf86I2CWriteByte(&pAPriv->Port[0].I2CDev, 0x11, 0x00);
	TimerCancel(pAPriv->Timer);
    }
}

static Bool
StartVideoStream(PortPrivPtr pPPriv, RegionPtr pRegion)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    GLINTPtr pGlint = GLINTPTR(pAPriv->pScrn);

    pPPriv->APO = -1;

    if (pAPriv->pm2p) {
	if (pPPriv == &pAPriv->Port[0]) {
	    if (!PutCookies(pPPriv, pRegion))
		return FALSE;
	    if (pPPriv->StreamOn)
		return TRUE;
	} else {
	    if (!GetCookies(pPPriv, pRegion))
		return FALSE;
	    if (pPPriv->StreamOn) {
		BlackOut(pPPriv, pRegion);
		return TRUE;
	    }
	}

	xvipc.a = pPPriv->Buffers;
	xvipc.b = !pPPriv->Attribute[4];
	xvipc.c = 1 + (pPPriv->Attribute[4] & 2);

	if (!xvipcHandshake(pPPriv, OP_START, TRUE))
		return FALSE;

	if (pPPriv == &pAPriv->Port[1]) {
	    pPPriv->BufferBase[0] = xvipc.d;
	    BlackOut(pPPriv, pRegion);
	}

	return pPPriv->StreamOn = TRUE;
    } else {
	CARD32 Base = (pPPriv == &pAPriv->Port[0]) ? VSABase : VSBBase;

        if (pPPriv->pFBArea[0] == NULL)
    	    if (!ReallocateOffscreenBuffer(pPPriv, (pPPriv == &pAPriv->Port[0]) ? 2 : 1))
		return FALSE;

	if (pPPriv == &pAPriv->Port[0]) {
	    if (!PutCookies(pPPriv, pRegion))
		return FALSE;
	} else {
	    if (!GetCookies(pPPriv, pRegion))
		return FALSE;
	    BlackOut(pPPriv, pRegion);
	}

	if (pPPriv->StreamOn)
	    return TRUE;

	GLINT_WRITE_REG(pPPriv->BufferBase[0] / 8, Base + VSVideoAddress0);
	if (pPPriv->pFBArea[1])
	    GLINT_WRITE_REG(pPPriv->BufferBase[1] / 8, Base + VSVideoAddress1);
	else
	    GLINT_WRITE_REG(pPPriv->BufferBase[0] / 8, Base + VSVideoAddress1);
	GLINT_WRITE_REG(pPPriv->BufferStride / 8, Base + VSVideoStride);

	GLINT_WRITE_REG(0, Base + VSCurrentLine);

	if (pAPriv->VideoStd == 1) {
	    /* NTSC untested */
	    GLINT_WRITE_REG(16, Base + VSVideoStartLine);
	    GLINT_WRITE_REG(16 + 240, Base + VSVideoEndLine);
	    GLINT_WRITE_REG(288 + (8 & ~3) * 2, Base + VSVideoStartData);
	    GLINT_WRITE_REG(288 + ((8 & ~3) + 704) * 2, Base + VSVideoEndData);
	} else {
	    /* PAL, SECAM (untested) */
	    GLINT_WRITE_REG(16, Base + VSVideoStartLine);
	    GLINT_WRITE_REG(16 + 288, Base + VSVideoEndLine);
	    GLINT_WRITE_REG(288 + (8 & ~3) * 2, Base + VSVideoStartData);
	    GLINT_WRITE_REG(288 + ((8 & ~3) + 704) * 2, Base + VSVideoEndData);
	}

	GLINT_WRITE_REG(2, Base + VSVideoAddressHost);
	GLINT_WRITE_REG(0, Base + VSVideoAddressIndex);

	if (pPPriv == &pAPriv->Port[0]) {
	    xf86I2CWriteByte(&pAPriv->Port[0].I2CDev, 0x11, 0x0D);
	    GLINT_WRITE_REG(VSA_Video |
			    (pPPriv->Attribute[4] ? 
				VSA_CombineFields : VSA_Discard_FieldTwo),
			    VSABase + VSControl);
#ifdef COLORBARS
	    if (!pAPriv->Port[1].StreamOn) {
		xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x3A, 0x83);
		xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, Enc61[pAPriv->VideoStd]);
	    }
#endif
	} else {
	    GLINT_WRITE_REG(VSB_Video |
			    (pPPriv->Attribute[4] ? VSB_CombineFields : 0) |
			  /* VSB_GammaCorrect | */
			    (16 << 4) | /* 5:6:5 FIXME? */
			    (1 << 9) | /* 16 */
			    VSB_RGBOrder, VSBBase + VSControl);
	    xf86I2CWriteByte(&pAPriv->Port[0].I2CDev, 0x11, 0x0D);
	    xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x3A, Enc3A[pPPriv->Plug]);
	    xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, Enc61[pAPriv->VideoStd]);
	}

	TimerSet(pAPriv->Timer, 0, 80, TimerCallback, pAPriv);

	return pPPriv->StreamOn = TRUE;
    }

    return FALSE;
}

/* Pseudo interrupt - better than nothing
 * (not used in kernel backbone mode)
 */

static CARD32
TimerCallback(OsTimerPtr pTim, CARD32 now, pointer p)
{
    AdaptorPrivPtr pAPriv = (AdaptorPrivPtr) p;
    GLINTPtr pGlint = GLINTPTR(pAPriv->pScrn);
    PortPrivPtr pPPriv;
    int delay;

    pPPriv = &pAPriv->Port[0];

    if (pPPriv->VideoOn >= 1) {
	pPPriv->FrameAcc += pPPriv->FramesPerSec;
	if (pPPriv->FrameAcc >= pAPriv->FramesPerSec) {
	    pPPriv->FrameAcc -= pAPriv->FramesPerSec;

	    PutYUV(pPPriv, (!pPPriv->pFBArea[1]) ?
		   pPPriv->BufferBase[0] : pPPriv->BufferBase[1 -
		   GLINT_READ_REG(VSABase + VSVideoAddressIndex)]);

	    if (pPPriv->VideoOn == 1)
		pPPriv->VideoOn = 0;
	}
    } else if (pPPriv->APO >= 0 && !(pPPriv->APO--))
	DelayedStopVideo(pPPriv);

    pPPriv = &pAPriv->Port[1];

    if (pPPriv->VideoOn >= 1) {
	pPPriv->FrameAcc += pPPriv->FramesPerSec;
	if (pPPriv->FrameAcc >= pAPriv->FramesPerSec) {
	    pPPriv->FrameAcc -= pAPriv->FramesPerSec;

	    GetYUV(pPPriv);

	    if (pPPriv->VideoOn == 1)
		pPPriv->VideoOn = 0;
	}
    } else if (pPPriv->APO >= 0 && !(pPPriv->APO--))
	DelayedStopVideo(pPPriv);

    if (pAPriv->Port[0].StreamOn) {
	delay = GLINT_READ_REG(VSABase + VSCurrentLine);
	if (!(GLINT_READ_REG(VSStatus) & VS_FieldOne0A))
	    delay += pAPriv->FrameLines >> 1;
    } else if (pAPriv->Port[1].StreamOn) {
	delay = GLINT_READ_REG(VSBBase + VSCurrentLine);
	if (!(GLINT_READ_REG(VSStatus) & VS_FieldOne0B))
	    delay += pAPriv->FrameLines >> 1;
    } else
	    delay = pAPriv->IntLine;

    if (delay > (pAPriv->IntLine - 16))
	delay -= pAPriv->FrameLines;

    return (((pAPriv->IntLine - delay) * pAPriv->LinePer) + 999999) / 1000000;
}

static int
Permedia2PutVideo(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"PutVideo %d,%d,%d,%d -> %d,%d,%d,%d\n",
	vid_x, vid_y, vid_w, vid_h, drw_x, drw_y, drw_w, drw_h));

    if (pPPriv == &pAPriv->Port[0]) {
	int sw = InputVideoEncodings[pAPriv->VideoStd * 3].width;
	int sh = InputVideoEncodings[pAPriv->VideoStd * 3].height;

	if ((vid_x + vid_w) > sw ||
	    (vid_y + vid_h) > sh)
	    return BadValue;

	pPPriv->VideoOn = 0;

        pPPriv->vx = ((vid_x << 10) * pPPriv->fw) / sw;
	pPPriv->vy = ((vid_y << 10) * pPPriv->fh) / sh;
        pPPriv->vw = ((vid_w << 10) * pPPriv->fw) / sw;
	pPPriv->vh = ((vid_h << 10) * pPPriv->fh) / sh;

        pPPriv->dx = drw_x;
	pPPriv->dy = drw_y;
        pPPriv->dw = drw_w;
	pPPriv->dh = drw_h;

	pPPriv->FrameAcc = pAPriv->FramesPerSec;

	if (!StartVideoStream(pPPriv, clipBoxes))
	    return XvBadAlloc;

	pPPriv->VideoOn = 2;

	return Success;
    }

    return XvBadAlloc;
}

static int
Permedia2PutStill(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    GLINTPtr pGlint = GLINTPTR(pScrn);

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"PutStill %d,%d,%d,%d -> %d,%d,%d,%d\n",
	vid_x, vid_y, vid_w, vid_h, drw_x, drw_y, drw_w, drw_h));

    if (pPPriv == &pAPriv->Port[0]) {
	int sw = InputVideoEncodings[pAPriv->VideoStd * 3].width;
	int sh = InputVideoEncodings[pAPriv->VideoStd * 3].height;

	if ((vid_x + vid_w) > sw ||
	    (vid_y + vid_h) > sh)
	    return BadValue;

	pPPriv->VideoOn = 0;

        pPPriv->vx = ((vid_x << 10) * pPPriv->fw) / sw;
	pPPriv->vy = ((vid_y << 10) * pPPriv->fh) / sh;
        pPPriv->vw = ((vid_w << 10) * pPPriv->fw) / sw;
	pPPriv->vh = ((vid_h << 10) * pPPriv->fh) / sh;

        pPPriv->dx = drw_x;
	pPPriv->dy = drw_y;
        pPPriv->dw = drw_w;
	pPPriv->dh = drw_h;

	{
	    Bool r = TRUE;

	    pPPriv->FrameAcc = pAPriv->FramesPerSec;

	    if (!StartVideoStream(pPPriv, clipBoxes))
		return XvBadAlloc;

	    if (pAPriv->pm2p) {
		/* Sleep, not busy wait, until the very next frame is ready.
		   Accept memory requests and other window's update events
		   in the meantime. */
		for (pPPriv->VideoOn = 1; pPPriv->VideoOn;)
		    if (!xvipcHandshake(pPPriv, OP_UPDATE, TRUE)) {
			r = FALSE;
			break;
		    }
	    } else {
		usleep(80000);
		PutYUV(pPPriv, (!pPPriv->pFBArea[1]) ?
		    pPPriv->BufferBase[0] : pPPriv->BufferBase[1 -
		    GLINT_READ_REG(VSABase + VSVideoAddressIndex)]);
	    }

	    pPPriv->APO = 125; /* Delayed stop: consider PutStill staccato */

	    if (r)
		return Success;
	}
    }

    return XvBadAlloc;
}

static int
Permedia2GetVideo(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"GetVideo %d,%d,%d,%d <- %d,%d,%d,%d\n",
	vid_x, vid_y, vid_w, vid_h, drw_x, drw_y, drw_w, drw_h));

    if (pPPriv == &pAPriv->Port[1]) {
	int sw = InputVideoEncodings[pAPriv->VideoStd * 3].width;
	int sh = InputVideoEncodings[pAPriv->VideoStd * 3].height;

	if ((vid_x + vid_w) > sw ||
	    (vid_y + vid_h) > sh)
	    return BadValue;

	pPPriv->VideoOn = 0;

        pPPriv->vx = (vid_x * pPPriv->fw) / sw;
	pPPriv->vy = (vid_y * pPPriv->fh) / sh;
        pPPriv->vw = (vid_w * pPPriv->fw) / sw;
	pPPriv->vh = (vid_h * pPPriv->fh) / sh;

        pPPriv->dx = drw_x;
	pPPriv->dy = drw_y;
        pPPriv->dw = drw_w;
	pPPriv->dh = drw_h;

	pPPriv->FrameAcc = pAPriv->FramesPerSec;

	if (!StartVideoStream(pPPriv, clipBoxes))
	    return XvBadAlloc;

	GetYUV(pPPriv);

	pPPriv->VideoOn = 2;

	return Success;
    }

    return XvBadAlloc;
}

static int
Permedia2GetStill(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"GetStill %d,%d,%d,%d <- %d,%d,%d,%d\n",
	vid_x, vid_y, vid_w, vid_h, drw_x, drw_y, drw_w, drw_h));

    if (pPPriv == &pAPriv->Port[1]) {
	int sw = InputVideoEncodings[pAPriv->VideoStd * 3].width;
	int sh = InputVideoEncodings[pAPriv->VideoStd * 3].height;

	if ((vid_x + vid_w) > sw ||
	    (vid_y + vid_h) > sh)
	    return BadValue;

	pPPriv->VideoOn = 0;

        pPPriv->vx = (vid_x * pPPriv->fw) / sw;
	pPPriv->vy = (vid_y * pPPriv->fh) / sh;
        pPPriv->vw = (vid_w * pPPriv->fw) / sw;
	pPPriv->vh = (vid_h * pPPriv->fh) / sh;

        pPPriv->dx = drw_x;
	pPPriv->dy = drw_y;
        pPPriv->dw = drw_w;
	pPPriv->dh = drw_h;

	pPPriv->FrameAcc = pAPriv->FramesPerSec;

	if (!StartVideoStream(pPPriv, clipBoxes))
	    return XvBadAlloc;

	GetYUV(pPPriv);

	/* Remains active until the client (implicitly)
	   calls StopVideo for this port, GetStill, or GetVideo */

	return Success;
    }

    return XvBadAlloc;
}

static void
Permedia2StopVideo(ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    GLINTPtr pGlint = GLINTPTR(pScrn);

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"StopVideo port=%d, exit=%d\n",
	PORTNUM(pPPriv), exit));

    pPPriv->VideoOn = 0;

    if (exit) {
    	StopVideoStream(pAPriv, 1 << PORTNUM(pPPriv));
	ReallocateOffscreenBuffer(pPPriv, 0);
    } else
	pPPriv->APO = 750; /* Delay, appx. 30 sec */

    if (pGlint->NoAccel)
	Permedia2Sync(pScrn);
}

static void
DelayedStopVideo(PortPrivPtr pPPriv)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pAPriv->pScrn->scrnIndex, X_INFO, 3,
	"DelayedStopVideo port=%d\n", PORTNUM(pPPriv)));

    pPPriv->APO = -1;
    Permedia2StopVideo(pAPriv->pScrn, (pointer) pPPriv, TRUE);
}

/* Remind Port[0].v* are shifted 1 << 10 */

static void
AdjustVideoW(PortPrivPtr pPPriv, int num, int denom)
{
    pPPriv->vx = (pPPriv->vx * num) / denom;
    pPPriv->vw = (pPPriv->vw * num) / denom;
}

static void
AdjustVideoH(PortPrivPtr pPPriv, int num, int denom)
{
    pPPriv->vy = (pPPriv->vy * num) / denom;
    pPPriv->vh = (pPPriv->vh * num) / denom;
}

static int
Permedia2SetPortAttribute(ScrnInfoPtr pScrn,
    Atom attribute, INT32 value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "SPA attr=%d, val=%d, port=%d\n",
	attribute, value, PORTNUM(pPPriv)));

    if (attribute == xvFilter) {
	pPPriv->Attribute[5] = !!value;
	return Success;
    } else if (attribute == xvInterlace) {
	value %= 3;

	if (value != pPPriv->Attribute[4]) {
	    int VideoOn = ABS(pPPriv->VideoOn);
	    int fh;

    	    StopVideoStream(pAPriv, 1 << PORTNUM(pPPriv));

	    ReallocateOffscreenBuffer(pPPriv, 0);

	    pPPriv->Attribute[4] = value;

	    fh = InputVideoEncodings[pAPriv->VideoStd * 3].height >> (1 - (value & 1));
	    AdjustVideoH(pPPriv, fh, pPPriv->fh);
	    pPPriv->fh = fh;

	    if (VideoOn) {
		if (StartVideoStream(pPPriv, NULL)) {
		    pPPriv->VideoOn = VideoOn;
		    if (pPPriv == &pAPriv->Port[1])
			GetYUV(pPPriv);
		} else {
		    pPPriv->VideoOn = -VideoOn;
		    return XvBadAlloc;
		}
	    }
	}

	return Success;
    }

    if (PORTNUM(pPPriv) == 0) {
	if (attribute == xvEncoding) {
	    if (value < 0 || value > 8) {
		return XvBadEncoding;
	    }
	    /* Fall through */
	} else if (attribute == xvBrightness) {
	    pPPriv->Attribute[0] = value = CLAMP(value, -1000, +1000);
	    return SetAttr(&pAPriv->Port[0], 0, 0x0A, 128 + (MIN(value, +999) * 128) / 1000);
	} else if (attribute == xvContrast) {
	    pPPriv->Attribute[1] = value = CLAMP(value, -3000, +1000);
	    return SetAttr(&pAPriv->Port[0], 1, 0x0B, 64 + (MIN(value, +999) * 64) / 1000);
   	} else if (attribute == xvSaturation) {
	    pPPriv->Attribute[2] = value = CLAMP(value, -3000, +1000);
	    return SetAttr(&pAPriv->Port[0], 2, 0x0C, 64 + (MIN(value, +999) * 64) / 1000);
	} else if (attribute == xvHue) {
	    pPPriv->Attribute[3] = value = CLAMP(value, -1000, +1000);
	    return SetAttr(&pAPriv->Port[0], 3, 0x0D, (MIN(value, +999) * 128) / 1000);
	} else
	    return BadMatch;
    } else { /* Output */
	if (attribute == xvEncoding) {
	    if (value < 0 || value > 3)
		return XvBadEncoding;
	    if (++value >= 3) value++;
	} else if (attribute == xvBkgColor) {
	    pPPriv->Attribute[6] = value;
	    pPPriv->BkgCol = ((value & 0xF80000) >> 8) |
			     ((value & 0x00FC00) >> 5) |
			     ((value & 0x0000F8) >> 3);
	    pPPriv->BkgCol |= pPPriv->BkgCol << 16;
	    if (pPPriv->StreamOn) {
		BlackOut(pPPriv, NULL);
		GetYUV(pPPriv);
	    }
	    return Success;
	} else
    	    return Success;
    }

    if (attribute == xvEncoding) {
	pPPriv->Plug = value % 3;

	if (!SetPlug(pPPriv))
	    return XvBadAlloc;

	value /= 3;

	if (value != pAPriv->VideoStd) {
	    int VideoOn0 = ABS(pAPriv->Port[0].VideoOn);
	    int	VideoOn1 = ABS(pAPriv->Port[1].VideoOn);
	    int fh;

	    StopVideoStream(pAPriv, 1 | 2);

	    if (value == 1 || pAPriv->VideoStd == 1) {
		ReallocateOffscreenBuffer(&pAPriv->Port[0], 0);
		ReallocateOffscreenBuffer(&pAPriv->Port[1], 0);
	    }

	    pAPriv->VideoStd = value;

	    SetVideoStd(pAPriv);

	    fh = InputVideoEncodings[pAPriv->VideoStd * 3].height >> (!pAPriv->Port[0].Attribute[4]);
	    AdjustVideoH(&pAPriv->Port[0], fh, pAPriv->Port[0].fh);
	    pAPriv->Port[0].fh = fh;

	    fh = InputVideoEncodings[pAPriv->VideoStd * 3].height >> (!pAPriv->Port[1].Attribute[4]);
	    AdjustVideoH(&pAPriv->Port[1], fh, pAPriv->Port[1].fh);
	    pAPriv->Port[1].fh = fh;

	    if (VideoOn0)
		pAPriv->Port[0].VideoOn = (StartVideoStream(&pAPriv->Port[0], NULL)) ?
		    VideoOn0 : -VideoOn0;

	    if (VideoOn1) {
		if (StartVideoStream(&pAPriv->Port[1], NULL)) {
		    pAPriv->Port[1].VideoOn = VideoOn1;
		    GetYUV(pPPriv);
		} else
		    pAPriv->Port[1].VideoOn = -VideoOn1;
	    }

	    if (pAPriv->Port[0].VideoOn < 0 ||
		pAPriv->Port[1].VideoOn < 0 ||
		pAPriv->VideoStd != value)
		return XvBadAlloc;
	}
    }

    return Success;
}

/* Should update attrs via XVIPC too,
 * Xv has XvPortNotify but no DDX equivalent.
 */

static int
Permedia2GetPortAttribute(ScrnInfoPtr pScrn, 
    Atom attribute, INT32 *value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    if (attribute == xvEncoding)
	*value = pAPriv->VideoStd * 3 + pPPriv->Plug;
    else if (attribute == xvBrightness)
	*value = pPPriv->Attribute[0];
    else if (attribute == xvContrast)
	*value = pPPriv->Attribute[1];
    else if (attribute == xvSaturation)
	*value = pPPriv->Attribute[2];
    else if (attribute == xvHue)
	*value = pPPriv->Attribute[3];
    else if (attribute == xvInterlace)
	*value = pPPriv->Attribute[4];
    else if (attribute == xvFilter)
	*value = pPPriv->Attribute[5];
    else if (attribute == xvBkgColor)
	*value = pPPriv->Attribute[6];
    else
	return BadMatch;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "GPA attr=%d, val=%d, port=%d\n",
	attribute, *value, PORTNUM(pPPriv)));

    return Success;
}

static void
Permedia2QueryBestSize(ScrnInfoPtr pScrn, Bool motion,
    short vid_w, short vid_h, short drw_w, short drw_h,
    unsigned int *p_w, unsigned int *p_h, pointer data)
{
    *p_w = drw_w;
    *p_h = drw_h;
}

static Bool
xvipcHandshake(PortPrivPtr pPPriv, int op, Bool block)
{
    int r;
    int brake = 150;

    xvipc.magic = XVIPC_MAGIC;
    xvipc.op = op;
    xvipc.block = block; /* Wait (with timeout), else don't wait for events
			   if none are pending. */

    if (pPPriv) {
	AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

	xvipc.pm2p = pAPriv->pm2p;
	xvipc.pAPriv = pAPriv;
	xvipc.port = PORTNUM(pPPriv);
    } else {
	xvipc.pm2p = (void *) -1;
	xvipc.pAPriv = NULL;
	xvipc.port = -1;
    }

    while (TRUE) {
	if (brake-- <= 0) return FALSE; /* We have a bug. */

	DEBUG(xf86MsgVerb(X_INFO, 4,
	    "PM2 XVIPC send op=%d bl=%d po=%d a=%d b=%d c=%d\n",
	    xvipc.op, xvipc.block, xvipc.port, xvipc.a, xvipc.b, xvipc.c));

	r = ioctl(xvipc_fd, VIDIOC_PM2_XVIPC, (void *) &xvipc);

	DEBUG(xf86MsgVerb(X_INFO, 4,
	    "PM2 XVIPC recv op=%d bl=%d po=%d a=%d b=%d c=%d err=%d/%d\n",
	    xvipc.op, xvipc.block, xvipc.port, xvipc.a, xvipc.b, xvipc.c, r, errno));

	switch (xvipc.op) {
	    case OP_ALLOC:
	    {
		AdaptorPrivPtr pAPriv = xvipc.pAPriv;
		ScrnInfoPtr pScrn = pAPriv->pScrn;
		GLINTPtr pGlint = GLINTPTR(pScrn);
		FBAreaPtr pFBArea;
		LFBAreaPtr pLFBArea = NULL;

		xvipc.a = -1;

		if ((pFBArea = xf86AllocateLinearOffscreenArea(pScrn->pScreen,
		    xvipc.b >> BPPSHIFT(pGlint), 2, NULL, NULL, NULL)))
		    xvipc.a = ((pFBArea->box.y1 * pScrn->displayWidth) +
			      pFBArea->box.x1) << BPPSHIFT(pGlint);

		if (ioctl(xvipc_fd, VIDIOC_PM2_XVIPC, (void *) &xvipc) != 0) {
		    if (pFBArea) xf86FreeOffscreenArea(pFBArea);
		    pFBArea = NULL;
		}

		if (pFBArea && (pLFBArea = xalloc(sizeof(LFBAreaRec)))) {
		    pLFBArea->Next = pAPriv->LFBList;
		    pLFBArea->pFBArea = pFBArea;
		    pLFBArea->Linear = xvipc.a;
		    pAPriv->LFBList = pLFBArea;
		}

		DEBUG(xf86MsgVerb(X_INFO, 3, "PM2 XVIPC alloc addr=%d=0x%08x pFB=%p pLFB=%p\n",
		    xvipc.a, xvipc.a, pFBArea, pLFBArea));

		goto event;
	    }

	    case OP_FREE:
	    {
		AdaptorPrivPtr pAPriv = xvipc.pAPriv;
		LFBAreaPtr pLFBArea, *ppLFBArea;

		for (ppLFBArea = &pAPriv->LFBList; (pLFBArea = *ppLFBArea);
		     ppLFBArea = &pLFBArea->Next)
		    if (pLFBArea->Linear == xvipc.a)
			break;

		if (!pLFBArea)
		    xvipc.a = -1;

		DEBUG(xf86MsgVerb(X_INFO, 3, "PM2 XVIPC free addr=%d=0x%08x pLFB=%p\n",
		    xvipc.a, xvipc.a, pLFBArea));

		if (ioctl(xvipc_fd, VIDIOC_PM2_XVIPC, (void *) &xvipc) == 0) {
		    xf86FreeOffscreenArea(pLFBArea->pFBArea);
		    *ppLFBArea = pLFBArea->Next;
		    xfree(pLFBArea);
		    pLFBArea = NULL;
		}

		goto event;
	    }

	    case OP_UPDATE:
	    {
		AdaptorPrivPtr pAPriv = xvipc.pAPriv;
		PortPrivPtr pPPriv;

		pPPriv = &pAPriv->Port[0];

		if (pPPriv->VideoOn >= 1 && xvipc.a > 0) {
		    pPPriv->FrameAcc += pPPriv->FramesPerSec;
		    if (pPPriv->FrameAcc >= pAPriv->FramesPerSec) {
			pPPriv->FrameAcc -= pAPriv->FramesPerSec;

			/* Asynchronous resizing caused by kernel app */

			if (xvipc.c != pPPriv->fw ||
			    xvipc.d != pPPriv->fh) {
			    AdjustVideoW(pPPriv, xvipc.c, pPPriv->fw);
			    AdjustVideoH(pPPriv, xvipc.d, pPPriv->fh);
			    pPPriv->fw = xvipc.c;
			    pPPriv->fh = xvipc.d;
			    pPPriv->BufferPProd = xvipc.e;

			    PutCookies(pPPriv, NULL);
			}

			PutYUV(pPPriv, xvipc.a);

			if (pPPriv->VideoOn == 1)
			    pPPriv->VideoOn = 0;
		    }
		} else if (pPPriv->APO >= 0 && !(pPPriv->APO--))
		    DelayedStopVideo(pPPriv);

		pPPriv = &pAPriv->Port[1];

		if (pPPriv->VideoOn >= 1 && xvipc.b > 0) {
		    pPPriv->FrameAcc += pPPriv->FramesPerSec;
		    if (pPPriv->FrameAcc >= pAPriv->FramesPerSec) {
			pPPriv->FrameAcc -= pAPriv->FramesPerSec;

			pPPriv->BufferBase[0] = xvipc.b;

			/* Output is always exclusive, no async resizing */

			GetYUV(pPPriv);

			if (pPPriv->VideoOn == 1)
			    pPPriv->VideoOn = 0;
		    }
		} else if (pPPriv->APO >= 0 && !(pPPriv->APO--))
		    DelayedStopVideo(pPPriv);

		/* Fall through */
	    }

	    default:
	    event:
		if (xvipc.op == op)
		    return r == 0;

		xvipc.op = OP_EVENT;
		xvipc.block = block;
	}
    }

    return TRUE;
}

static void
Permedia2ReadInput(int fd, pointer unused)
{
    xvipcHandshake(NULL, OP_EVENT, FALSE);
}

static void
AdaptorPrivUninit(AdaptorPrivPtr pAPriv)
{
    GLINTPtr pGlint = GLINTPTR(pAPriv->pScrn);

    if (!pAPriv->pm2p) {
	TimerFree(pAPriv->Timer);
	xf86DestroyI2CDevRec(&pAPriv->Port[0].I2CDev, FALSE);
	xf86DestroyI2CDevRec(&pAPriv->Port[1].I2CDev, FALSE);
	GLINT_MASK_WRITE_REG(VS_UnitMode_ROM, ~VS_UnitMode_Mask, VSConfiguration);
	GLINT_WRITE_REG(pAPriv->FifoControl, PMFifoControl);
    }

    FreeCookies(&pAPriv->Port[0]);
    FreeCookies(&pAPriv->Port[1]);

    xfree(pAPriv);
}

/* Only a single file for all heads, the device ID is transmitted at initial
 * handshake and encoded in all subsequent xvipc headers.
 */

static Bool
xvipcOpen(char *name, ScrnInfoPtr pScrn)
{
    const char *osname;

    if (xvipc_fd >= 0)
	return TRUE;

    xf86GetOS(&osname, NULL, NULL, NULL);

    if (!osname || strcmp(osname, "linux"))
	return FALSE;

    while (1) {
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "XVIPC probing device %s\n", name));

    	if ((xvipc_fd = open(name, O_RDWR /* | O_TRUNC */, 0)) < 0)
	    break;

	xvipc.magic = XVIPC_MAGIC;
	xvipc.pm2p = (void *) -1;
	xvipc.pAPriv = NULL;
	xvipc.op = OP_CONNECT;
	xvipc.a = 0;
	xvipc.b = 0;
	xvipc.c = 0;
	xvipc.d = 0;

	if (ioctl(xvipc_fd, VIDIOC_PM2_XVIPC, (void *) &xvipc) < 0 || xvipc.pm2p)
	    break;

	if (xvipc.c != XVIPC_VERSION) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "Your Permedia 2 kernel driver %d.%d uses XVIPC protocol "
		       "V.%d while this Xv driver expects V.%d. Please update.\n",
		       xvipc.a, xvipc.b, xvipc.c, XVIPC_VERSION);
	    break;
	}

	/* Typical input devices should be closed when VT switched away, but
	 * this is proprietary IPC between this driver and the kernel driver
	 * only. Actually, it would be better to continue listening to kernel
	 * events, at least keeping the file open, to avoid side effects of
	 * repeated dis- and reconnection, shouldn't hurt.
	 */

	xf86AddInputHandler(xvipc_fd, Permedia2ReadInput, NULL);

	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Xv driver connected to %s\n", name);

	return TRUE;
    }

    if (xvipc_fd >= 0)
	close(xvipc_fd);

    xvipc_fd = -1;

    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	       "Cannot connect to Permedia 2 kernel driver. "
	       "Note that using both drivers at the same time "
	       "will cause problems.\n");
    return FALSE;
}

static AdaptorPrivPtr
AdaptorPrivInit(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    AdaptorPrivPtr pAPriv = (AdaptorPrivPtr) xcalloc(1, sizeof(AdaptorPrivRec));
    int i;

    if (pAPriv) {
	pAPriv->pScrn = pScrn;
	pAPriv->Port[0].pAdaptor = pAPriv;
	pAPriv->Port[1].pAdaptor = pAPriv;

	pAPriv->Port[0].Attribute[0] = 0;	/* Brightness (-1000..+1000) */
	pAPriv->Port[0].Attribute[1] = 0; 	/* Contrast (-3000..+1000) */
	pAPriv->Port[0].Attribute[2] = 0; 	/* Color saturation (-3000..+1000) */
	pAPriv->Port[0].Attribute[3] = 0;	/* Hue (-1000..+1000) */
	pAPriv->Port[0].Attribute[4] = 1;	/* Interlaced (0 = not, 1 = yes, 2 = dscan) */
	pAPriv->Port[0].Attribute[5] = 0;	/* Bilinear Filter (Bool) */

	pAPriv->Port[1].Attribute[4] = 1;	/* Interlaced (Bool) */
	pAPriv->Port[1].Attribute[5] = 0;	/* Bilinear Filter (Bool) */
	pAPriv->Port[1].Attribute[6] = 0;	/* BkgColor 0x00RRGGBB */
	pAPriv->Port[1].BkgCol = 0;

	pAPriv->Port[0].Buffers = 2;
	pAPriv->Port[1].Buffers = 1;

	for (i = 0; i <= 1; i++) {
	    pAPriv->Port[i].fw = 704;
	    pAPriv->Port[i].fh = 576;
	    pAPriv->Port[i].FramesPerSec = 30;
	    pAPriv->Port[i].APO = -1;
	    pAPriv->Port[i].BufferPProd = partprodPermedia[704 >> 5];
	}

	if (xvipc_fd >= 0) {
	    /* Initial handshake, take over control of this head */

	    xvipc.magic = XVIPC_MAGIC;
	    xvipc.pm2p = (void *) -1;		/* Kernel head ID */
	    xvipc.pAPriv = pAPriv;		/* Server head ID */
	    xvipc.op = OP_CONNECT;

	    xvipc.a = pGlint->PciInfo->bus;
	    xvipc.b = pGlint->PciInfo->device;
	    xvipc.c = pGlint->PciInfo->func;

	    xvipc.d = pScrn->videoRam << 10;	/* XF86Config overrides probing */

	    if (ioctl(xvipc_fd, VIDIOC_PM2_XVIPC, (void *) &xvipc) < 0) {
		if (errno == EBUSY)
		    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			       "Another application already opened the Permedia 2 "
			       "kernel driver for this board. To enable "
			       "shared access please start the server first.\n");
		else
		    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			       "Initialization of Xv support with kernel backbone "
			       "failed due to error %d: %s.\n", errno, strerror(errno));
		goto failed;
	    }

	    pAPriv->pm2p = xvipc.pm2p;

	    SetVideoStd(pAPriv);
	} else {
	    GLINT_WRITE_REG(0, VSABase + VSControl);
	    GLINT_WRITE_REG(0, VSBBase + VSControl);
#if 0
	    GLINT_MASK_WRITE_REG(0, ~(VSAIntFlag | VSBIntFlag), IntEnable);
	    GLINT_WRITE_REG(VSAIntFlag | VSBIntFlag, IntFlags); /* Reset */
#endif
    	    for (i = 0x0018; i <= 0x00B0; i += 8) {
		GLINT_WRITE_REG(0, VSABase + i);
		GLINT_WRITE_REG(0, VSBBase + i);
	    }

	    GLINT_WRITE_REG((0 << 8) | (132 << 0), VSABase + VSFifoControl);
	    GLINT_WRITE_REG((0 << 8) | (132 << 0), VSBBase + VSFifoControl);

	    GLINT_MASK_WRITE_REG(
		VS_UnitMode_AB8 |
		VS_GPBusMode_A |
	     /* VS_HRefPolarityA | */
		VS_VRefPolarityA |
		VS_VActivePolarityA |
	     /* VS_UseFieldA | */
		VS_FieldPolarityA |
	     /* VS_FieldEdgeA | */
	     /* VS_VActiveVBIA | */
        	VS_InterlaceA |
		VS_ReverseDataA |

	     /* VS_HRefPolarityB | */
		VS_VRefPolarityB |
		VS_VActivePolarityB |
	     /* VS_UseFieldB | */
		VS_FieldPolarityB |
	     /* VS_FieldEdgeB | */
	     /* VS_VActiveVBIB | */
		VS_InterlaceB |
	     /* VS_ColorSpaceB_RGB | */
	     /* VS_ReverseDataB | */
	     /* VS_DoubleEdgeB | */
		0, ~0x1FFFFE0F, VSConfiguration);

	    pAPriv->FifoControl = GLINT_READ_REG(PMFifoControl);
	    GLINT_WRITE_REG((12 << 8) | 8, PMFifoControl); /* FIXME */

	    if (!xf86I2CProbeAddress(pGlint->VSBus, SAA7111_SLAVE_ADDRESS))
		goto failed;

	    pAPriv->Port[0].I2CDev.DevName = "Decoder";
	    pAPriv->Port[0].I2CDev.SlaveAddr = SAA7111_SLAVE_ADDRESS;
	    pAPriv->Port[0].I2CDev.pI2CBus = pGlint->VSBus;

	    if (!xf86I2CDevInit(&pAPriv->Port[0].I2CDev))
		goto failed;

	    if (!xf86I2CWriteVec(&pAPriv->Port[0].I2CDev, DecInitVec,
		(sizeof(DecInitVec) / sizeof(I2CByte)) / 2))
		goto failed;

	    if (!xf86I2CProbeAddress(pGlint->VSBus, SAA7125_SLAVE_ADDRESS))
		goto failed;

	    pAPriv->Port[1].I2CDev.DevName = "Encoder";
	    pAPriv->Port[1].I2CDev.SlaveAddr = SAA7125_SLAVE_ADDRESS;
	    pAPriv->Port[1].I2CDev.pI2CBus = pGlint->VSBus;

	    if (!xf86I2CDevInit(&pAPriv->Port[1].I2CDev))
		goto failed;

	    if (!xf86I2CWriteVec(&pAPriv->Port[1].I2CDev, EncInitVec,
		(sizeof(EncInitVec) / sizeof(I2CByte)) / 2))
		goto failed;

	    SetVideoStd(pAPriv);

     	    pAPriv->Timer = TimerSet(NULL, 0, 0, TimerCallback, pAPriv);

	    if (!pAPriv->Timer)
		goto failed;
	}

	return pAPriv;

failed:
	AdaptorPrivUninit(pAPriv);
    }

    return NULL;
}

void
Permedia2VideoUninit(ScrnInfoPtr pScrn)
{
    AdaptorPrivPtr pAPriv, *ppAPriv;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Xv driver cleanup\n"));

    for (ppAPriv = &AdaptorPrivList; (pAPriv = *ppAPriv); ppAPriv = &(pAPriv->Next))
	if (pAPriv->pScrn == pScrn) {
	    *ppAPriv = pAPriv->Next;
	    StopVideoStream(pAPriv, 1 | 2 | 4);
	    AdaptorPrivUninit(pAPriv);
	    break;
	}

    if (xvipc_fd >= 0) {
	close(xvipc_fd);
	xvipc_fd = -1;
    }
}

/* Required to retire delayed stop (VT switching away) */

void
Permedia2VideoReset(ScrnInfoPtr pScrn)
{
    AdaptorPrivPtr pAPriv;

    for (pAPriv = AdaptorPrivList; pAPriv != NULL; pAPriv = pAPriv->Next)
	if (pAPriv->pScrn == pScrn) {
	    StopVideoStream(pAPriv, 1 | 2 | 4);
	    break;
	}

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Xv driver halted\n"));
}

void
Permedia2VideoInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    AdaptorPrivPtr pAPriv;
    DevUnion Private[2];
    XF86VideoAdaptorRec VAR[2];
    XF86VideoAdaptorPtr VARPtrs[2];
    pointer options[3];
    Bool VideoIO = TRUE;
    int i;

    options[0] = NULL;
    options[1] = NULL;
    options[2] = NULL;

    for (i = 0;; i++) {
	char *adaptor;

	if (!options[0])
	    options[0] = xf86FindXvOptions(pScreen->myNum, i, "input", &adaptor, &options[2]);
	if (!options[1])
	    options[1] = xf86FindXvOptions(pScreen->myNum, i, "output", &adaptor, NULL);
	if (!adaptor) {
	    if (!i) VideoIO = FALSE;
	    break;
	} else if (options[0] && options[1])
	    break;
    }

    if (!VideoIO) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Xv driver disabled\n");
	return;
    }

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Initializing Xv driver\n");

    switch (pGlint->Chipset) {
	case PCI_VENDOR_TI_CHIP_PERMEDIA2:
	case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
	case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	    break;
	default:
	    VideoIO = FALSE;
    }

    switch (pciReadLong(pGlint->PciTag, PCI_SUBSYSTEM_ID_REG)) {
	case PCI_SUBSYSTEM_ID_WINNER_2000_P2A:
	case PCI_SUBSYSTEM_ID_WINNER_2000_P2C:
	case PCI_SUBSYSTEM_ID_GLORIA_SYNERGY_P2A:
	case PCI_SUBSYSTEM_ID_GLORIA_SYNERGY_P2C:
	    break;
	default:
	    VideoIO = FALSE;
    }

    if (!VideoIO) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_PROBED, 1, "No Xv support for this board\n");
	return;
    }

    for (i = 0; i <= 2; i++) {
	xf86ProcessOptions(pScrn->scrnIndex, options[i],
	    (i == 0) ? InputOptions : ((i == 1) ? OutputOptions : AdaptorOptions));
	xf86ShowUnusedOptions(pScrn->scrnIndex, options[i]);
    }

    if (xf86IsOptionSet(AdaptorOptions, OPTION_DEVICE))
        xvipcOpen(xf86GetOptValString(AdaptorOptions, OPTION_DEVICE), pScrn);

    if (!(pAPriv = AdaptorPrivInit(pScrn))) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Xv driver initialization failed\n");
	return;
    }

    for (i = 0; i <= 1; i++) {
	int n;

	if (xf86GetOptValInteger(i ? OutputOptions : InputOptions, OPTION_BUFFERS, &n))
	    pAPriv->Port[i].Buffers = CLAMP(n, 1, i ? 1 : 2); /* FIXME */
	if (xf86GetOptValInteger(i ? OutputOptions : InputOptions, OPTION_FPS, &n))
	    pAPriv->Port[i].FramesPerSec = CLAMP(n, 1, 30);
    }

    if (pGlint->NoAccel) {
	BoxRec AvailFBArea;

	xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Xv driver overrides NoAccel option\n");

	Permedia2InitializeEngine(pScrn);

	AvailFBArea.x1 = 0;
	AvailFBArea.y1 = 0;
	AvailFBArea.x2 = pScrn->displayWidth;
	AvailFBArea.y2 = pGlint->FbMapSize /
	    (pScrn->displayWidth * pScrn->bitsPerPixel / 8);

	/* if (AvailFBArea.y2 > 2047)
	    AvailFBArea.y2 = 2047; */

	xf86InitFBManager(pScreen, &AvailFBArea);
    }

    memset(VAR, 0, sizeof(VAR));

    for (i = 0; i < 2; i++) {
	Private[i].ptr = (pointer) &pAPriv->Port[i];
	VARPtrs[i] = &VAR[i];

	VAR[i].type = (i == 0) ? XvInputMask : XvOutputMask;
	VAR[i].type |= XvWindowMask | XvVideoMask | XvStillMask;
	VAR[i].name = "Permedia 2";

	if (i == 0) {
	    VAR[0].nEncodings = sizeof(InputVideoEncodings) / sizeof(InputVideoEncodings[0]);
	    VAR[0].pEncodings = InputVideoEncodings;
	    VAR[0].nFormats = sizeof(InputVideoFormats) / sizeof(InputVideoFormats[0]);
	    VAR[0].pFormats = InputVideoFormats;
	} else {
	    Bool expose = FALSE;

/*
	    xf86GetOptValBool(OutputOptions, OPTION_EXPOSE, &expose);
	    if (expose)	VAR[1].flags = VIDEO_EXPOSE;
*/

	    VAR[1].nEncodings = sizeof(OutputVideoEncodings) / sizeof(OutputVideoEncodings[0]);
	    VAR[1].pEncodings = OutputVideoEncodings;
	    VAR[1].nFormats = sizeof(OutputVideoFormats) / sizeof(OutputVideoFormats[0]);
	    VAR[1].pFormats = OutputVideoFormats;
	}

	VAR[i].nPorts = 1;
	VAR[i].pPortPrivates = &Private[i];
	VAR[i].PutVideo = Permedia2PutVideo;
	VAR[i].PutStill = Permedia2PutStill;
	VAR[i].GetVideo = Permedia2GetVideo;
	VAR[i].GetStill = Permedia2GetStill;
	VAR[i].StopVideo = Permedia2StopVideo;
	VAR[i].SetPortAttribute = Permedia2SetPortAttribute;
	VAR[i].GetPortAttribute = Permedia2GetPortAttribute;
	VAR[i].QueryBestSize = Permedia2QueryBestSize;
    }

    if (!xf86XVScreenInit(pScreen, VARPtrs, 2)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Xv initialization failed\n");
	return;
    }

    xvEncoding   = MAKE_ATOM(XV_ENCODING);
    xvHue        = MAKE_ATOM(XV_HUE);
    xvSaturation = MAKE_ATOM(XV_SATURATION);
    xvBrightness = MAKE_ATOM(XV_BRIGHTNESS);
    xvContrast   = MAKE_ATOM(XV_CONTRAST);
    xvInterlace  = MAKE_ATOM(XV_INTERLACE);
    xvFilter	 = MAKE_ATOM(XV_FILTER);
    xvBkgColor	 = MAKE_ATOM(XV_BKGCOLOR);

    pAPriv->Next = AdaptorPrivList;
    AdaptorPrivList = pAPriv;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Xv driver %senabled\n",
	       (xvipc_fd >= 0) ? "with kernel backbone " : "");
}
