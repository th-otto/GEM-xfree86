/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga.h,v 1.57 2000/02/27 02:50:47 mvojkovi Exp $ */
/*
 * MGA Millennium (MGA2064W) functions
 *
 * Copyright 1996 The XFree86 Project, Inc.
 *
 * Authors
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		David Dawes
 *			dawes@XFree86.Org
 */

#ifndef MGA_H
#define MGA_H

/* Temporarily turn off building in DRI support */
#undef XF86DRI

#include "compiler.h"
#include "xaa.h"
#include "xf86Cursor.h"
#include "vgaHW.h"
#include "colormapst.h"
#include "xf86DDC.h"
#include "xf86xv.h"

#ifdef XF86DRI
#include "xf86drm.h"
#include "sarea.h"
#define _XF86DRI_SERVER_
#include "xf86dri.h"
#include "dri.h"
#include "GL/glxint.h"
#include "mga_dri.h"
#include "mga_dripriv.h"
#endif

#if !defined(EXTRADEBUG)
#define INREG8(addr) MMIO_IN8(pMga->IOBase, addr)
#define INREG16(addr) MMIO_IN16(pMga->IOBase, addr)
#define INREG(addr) MMIO_IN32(pMga->IOBase, addr)
#define OUTREG8(addr, val) MMIO_OUT8(pMga->IOBase, addr, val)
#define OUTREG16(addr, val) MMIO_OUT16(pMga->IOBase, addr, val)
#define OUTREG(addr, val) MMIO_OUT32(pMga->IOBase, addr, val)
#else /* !EXTRADEBUG */
CARD8 dbg_inreg8(ScrnInfoPtr,int,int);
CARD16 dbg_inreg16(ScrnInfoPtr,int,int);
CARD32 dbg_inreg32(ScrnInfoPtr,int,int);
void dbg_outreg8(ScrnInfoPtr,int,int);
void dbg_outreg16(ScrnInfoPtr,int,int);
void dbg_outreg32(ScrnInfoPtr,int,int);
#define INREG8(addr) dbg_inreg8(pScrn,addr,1)
#define INREG16(addr) dbg_inreg16(pScrn,addr,1)
#define INREG(addr) dbg_inreg32(pScrn,addr,1)
#define OUTREG8(addr,val) dbg_outreg8(pScrn,addr,val)
#define OUTREG16(addr,val) dbg_outreg16(pScrn,addr,val)
#define OUTREG(addr,val) dbg_outreg32(pScrn,addr,val)
#endif /* EXTRADEBUG */

#define PORT_OFFSET 	(0x1F00 - 0x300)

typedef struct {
    unsigned char	ExtVga[6];
    unsigned char 	DacClk[6];
    unsigned char *     DacRegs;
    CARD32		Option;
    CARD32		Option2;
    CARD32		Option3;
} MGARegRec, *MGARegPtr;

typedef struct {
    Bool	isHwCursor;
    int		CursorMaxWidth;
    int 	CursorMaxHeight;
    int		CursorFlags;
    int		CursorOffscreenMemSize;
    Bool	(*UseHWCursor)(ScreenPtr, CursorPtr);
    void	(*LoadCursorImage)(ScrnInfoPtr, unsigned char*);
    void	(*ShowCursor)(ScrnInfoPtr);
    void	(*HideCursor)(ScrnInfoPtr);
    void	(*SetCursorPosition)(ScrnInfoPtr, int, int);
    void	(*SetCursorColors)(ScrnInfoPtr, int, int);
    long	maxPixelClock;
    long	MemoryClock;
    MessageType ClockFrom;
    MessageType MemClkFrom;
    Bool	SetMemClk;
    void	(*LoadPalette)(ScrnInfoPtr, int, int*, LOCO*, VisualPtr);
    void	(*PreInit)(ScrnInfoPtr);
    void	(*Save)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    void	(*Restore)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    Bool	(*ModeInit)(ScrnInfoPtr, DisplayModePtr);
} MGARamdacRec, *MGARamdacPtr;


typedef struct {
    int bitsPerPixel;
    int depth;
    int displayWidth;
    rgb weight;
    Bool Overlay8Plus24;
    DisplayModePtr mode;
} MGAFBLayout;


/* Card-specific driver information */

#define MGAPTR(p) ((MGAPtr)((p)->driverPrivate))

#ifdef DISABLE_VGA_IO
typedef struct mgaSave {
    pciVideoPtr pvp;
    Bool enable;
} MgaSave, *MgaSavePtr;
#endif

typedef struct {
    EntityInfoPtr	pEnt;
    MGABiosInfo		Bios;
    MGABios2Info	Bios2;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    xf86AccessRec	Access;
    int			Chipset;
    int                 ChipRev;
    Bool		Primary;
    Bool		Interleave;
    int			HwBpp;
    int			Roundings[4];
    int			BppShifts[4];
    Bool		HasFBitBlt;
    Bool		OverclockMem;
    int			YDstOrg;
    int			DstOrg;
    int			SrcOrg;
    CARD32		IOAddress;
    CARD32		FbAddress;
    CARD32		ILOADAddress;
    int			FbBaseReg;
    CARD32		BiosAddress;
    MessageType		BiosFrom;
    unsigned char *     IOBase;
    unsigned char *     IOBaseDense;
    unsigned char *	FbBase;
    unsigned char *	ILOADBase;
    unsigned char *	FbStart;
    long		FbMapSize;
    long		FbUsableSize;
    long		FbCursorOffset;
    MGARamdacRec	Dac;
    Bool		HasSDRAM;
    Bool		NoAccel;
    Bool		SyncOnGreen;
    Bool		Dac6Bit;
    Bool		HWCursor;
    Bool		UsePCIRetry;
    Bool		ShowCache;
    Bool		Overlay8Plus24;
    Bool		ShadowFB;
    unsigned char *	ShadowPtr;
    int			ShadowPitch;
    int			MemClk;
    int			MinClock;
    int			MaxClock;
    MGARegRec		SavedReg;
    MGARegRec		ModeReg;
    int			MaxFastBlitY;
    CARD32		BltScanDirection;
    CARD32		FilledRectCMD;
    CARD32		SolidLineCMD;
    CARD32		PatternRectCMD;
    CARD32		DashCMD;
    CARD32		NiceDashCMD;
    CARD32		AccelFlags;
    CARD32		PlaneMask;
    CARD32		FgColor;
    CARD32		BgColor;
    CARD32		MAccess;
    int			FifoSize;
    int			StyleLen;
    XAAInfoRecPtr	AccelInfoRec;
    xf86CursorInfoPtr	CursorInfoRec;
    DGAModePtr		DGAModes;
    int			numDGAModes;
    Bool		DGAactive;
    int			DGAViewportStatus;
    CARD32		*Atype;
    CARD32		*AtypeNoBLK;
    void		(*PreInit)(ScrnInfoPtr pScrn);
    void		(*Save)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    void		(*Restore)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    Bool		(*ModeInit)(ScrnInfoPtr, DisplayModePtr);
    void		(*PointerMoved)(int index, int x, int y);
    CloseScreenProcPtr	CloseScreen;
    ScreenBlockHandlerProcPtr BlockHandler;
    unsigned int	(*ddc1Read)(ScrnInfoPtr);
    void (*DDC1SetSpeed)(ScrnInfoPtr, xf86ddcSpeed);
    Bool		(*i2cInit)(ScrnInfoPtr);
    I2CBusPtr		I2C;
    Bool		FBDev;
    int			colorKey;
    int			videoKey;
    int			fifoCount;
    int			Rotate;
    MGAFBLayout		CurrentLayout;
    Bool		DrawTransparent;
    int			MaxBlitDWORDS;

#ifdef XF86DRI
   Bool directRenderingEnabled;
   DRIInfoPtr pDRIInfo;
   int drmSubFD;
   int numVisualConfigs;
   __GLXvisualConfig* pVisualConfigs;
   MGAConfigPrivPtr pVisualConfigsPriv;
   MGARegRec DRContextRegs;
   MGADRIServerPrivatePtr  DRIServerInfo;
#endif

    XF86VideoAdaptorPtr adaptor;
} MGARec, *MGAPtr;

extern CARD32 MGAAtype[16];
extern CARD32 MGAAtypeNoBLK[16];

#define USE_RECTS_FOR_LINES	0x00000001
#define FASTBLT_BUG		0x00000002
#define CLIPPER_ON		0x00000004
#define BLK_OPAQUE_EXPANSION	0x00000008
#define TRANSC_SOLID_FILL	0x00000010
#define	NICE_DASH_PATTERN	0x00000020
#define	TWO_PASS_COLOR_EXPAND	0x00000040
#define	MGA_NO_PLANEMASK	0x00000080
#define USE_LINEAR_EXPANSION	0x00000100
#define LARGE_ADDRESSES		0x00000200
#define MGAIOMAPSIZE		0x00004000
#define MGAILOADMAPSIZE		0x00400000

#define TRANSPARENCY_KEY	255
#define KEY_COLOR		0

#define MGA_FRONT	0
#define MGA_BACK	1
#define MGA_DEPTH	2


/* Prototypes */

void MGAAdjustFrame(int scrnIndex, int x, int y, int flags);
Bool MGASwitchMode(int scrnIndex, DisplayModePtr mode, int flags);

void MGA2064SetupFuncs(ScrnInfoPtr pScrn);
void MGAGSetupFuncs(ScrnInfoPtr pScrn);

void MGAStormSync(ScrnInfoPtr pScrn);
void MGAStormEngineInit(ScrnInfoPtr pScrn);
Bool MGAStormAccelInit(ScreenPtr pScreen);
Bool MGAHWCursorInit(ScreenPtr pScreen);

Bool Mga8AccelInit(ScreenPtr pScreen);
Bool Mga16AccelInit(ScreenPtr pScreen);
Bool Mga24AccelInit(ScreenPtr pScreen);
Bool Mga32AccelInit(ScreenPtr pScreen);

void MGAPolyArcThinSolid(DrawablePtr, GCPtr, int, xArc*);

Bool MGADGAInit(ScreenPtr pScreen);

Bool MGADRIScreenInit(ScreenPtr pScreen);
void MGADRICloseScreen(ScreenPtr pScreen);
Bool MGADRIFinishScreenInit(ScreenPtr pScreen);
void MGASwapContext(ScreenPtr pScreen);
void MGALostContext(ScreenPtr pScreen);
void MGASelectBuffer(MGAPtr pMGA, int which);
Bool mgaConfigureWarp(ScrnInfoPtr pScrn);
unsigned int mgaInstallMicrocode(ScreenPtr pScreen, int agp_offset);
unsigned int mgaGetMicrocodeSize(ScreenPtr pScreen);
Bool mgadrmCleanupDma(ScrnInfoPtr pScrn);
Bool mgadrmInitDma(ScrnInfoPtr pScrn, int prim_size);

void MGARefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void MGARefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void MGARefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void MGARefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void MGARefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox);

void Mga8SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);
void Mga16SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);
void Mga24SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);
void Mga32SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);

void Mga8SetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
void Mga16SetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
void Mga24SetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
void Mga32SetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);

void MGAPointerMoved(int index, int x, int y);

void MGAInitVideo(ScreenPtr pScreen);
void MGAResetVideo(ScrnInfoPtr pScrn); 

#endif
