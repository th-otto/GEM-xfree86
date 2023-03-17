#define	P1_INDEX	0x04
#define	P2_INDEX	0x10
#define	P3_INDEX	0x12
#define	P4_INDEX	0x14
#define	P5_INDEX	0x16

/* CR30 VBInfo = CR31:CR30 */
#define	SET_SIMU_SCAN_MODE	0x0001
#define	SWITCH_TO_CRT2		0x0002
#define	SET_CRT2_TO_AVIDEO	0x0004		/* Composite */
#define	SET_CRT2_TO_SVIDEO	0x0008
#define	SET_CRT2_TO_SCART	0x0010
#define	SET_CRT2_TO_LCD		0x0020
#define	SET_CRT2_TO_RAMDAC	0x0040
#define	SET_CRT2_TO_HIVISION_TV	0x0080
#define	SET_CRT2_TO_TV		(SET_CRT2_TO_AVIDEO | SET_CRT2_TO_SVIDEO | \
				SET_CRT2_TO_SCART | SET_CRT2_TO_HIVISION_TV)
/* CR31 */
#define	SET_PAL_TV		0x0100
#define	SET_IN_SLAVE_MODE	0x0200
#define	SET_NO_SIMU_ON_LOCK	0x0400
#define	SET_NO_SIMU_TV_ON_LOCK			SET_NO_SIMU_ON_LOCK
#define	DISABLE_LOAD_CRT2DAC	0x1000
#define	DISABLE_CRT2_DISPLAY	0x2000
#define	DRIVER_MODE		0x4000

/* CR36 */
#define	PANEL_1024x768		0x00
#define	PANEL_1280x1024		0x01

/* Set Flag */
#define	PROGRAMMING_CRT2	0x0001
#define	TV_SIMU_MODE		0x0002
#define	RPLLDIV2XO		0x0004

/* Mode Flag */
#define	MODE_INFO_FLAG		0x0007
#define	MODE_CGA		0x0001
#define	MODE_EGA		0x0002
#define	MODE_VGA		0x0003
#define	MODE_15BPP		0x0004
#define	MODE_16BPP		0x0005
#define	MODE_24BPP		0x0006
#define	MODE_32BPP		0x0007

#define	DAC_INFO_FLAG		0x0018
#define	MONO_DAC		0x0000
#define	CGA_DAC			0x0008
#define	EGA_DAC			0x0010
#define	VGA_DAC			0x0018

#define	CHAR_8DOT		0x0200
#define	LINE_COMPARE_OFF	0x0400
#define	CRT2_MODE		0x0800
#define	HALF_DCLK		0x1000
#define	NO_SUPPORT_SIMU_TV	0x2000
#define	AFTER_LOCK_CRT2		0x4000
#define	DOUBLE_SCAN		0x8000

/* LCD Res Info */
#define	LCDVESA_TIMING		0x08

/* LCD Type Info */
/* LCD Info */
#define	LCDRGB18BIT		0x01

/* Misc definition */
#define	SELECT_CRT1		0x01
#define	SELECT_CRT2		0x02

#define	MODE_INFO_MASK		0x0007
#define	AFTER_LOCK_CRT2		0x4000

#define	SUPPORT_TV		0x0008
#define	SUPPORT_LCD		0x0020
#define	SUPPORT_RAMDAC2		0x0040

/* GetRatePtr */
#define	STANDARD_MODE		0x0000
#define	ENHANCED_MODE		0x0001
#define	CRT2_SUPPORT		0x0002
#define	CRT2_NO_SUPPORT		0x0000

#define	INTERLACE_MODE		0x0080

/* VCLK index */
#define	VCLK65			0x09
#define	VCLK108_2		0x14

/* TV resolution info */
#define	ST_HIVISION_TV_HT	892
#define	ST_HIVISION_TV_VT	1126
#define	EXT_HIVISION_TV_HT	2100
#define	EXT_HIVISION_TV_VT	1125
#define	PAL_HT		1728
#define	PAL_VT		625
#define	NTSC_HT		1716
#define	NTSC_VT		525

#define	TVVCLKDIV2	0x21
#define	TVVCLK		0x22
#define HITVVCLKDIV2	0x23
#define HITVVCLK	0x24
#define	HITVSIMUVCLK	0x25

#define	LCDDATA(p0,p1,p2,p3,p4,p5)	{p0&0xFF, p1&0xFF, p2&0xFF,\
		(p2>>8)|((p3&0xF)<<4), (p3>>4)|((p0&0x100)>>1), p4&0xFF,\
		(p4>>8)|((p5&0xF)<<4), p5>>4}
#define	TVDATA(p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12)	{p0&0xFF,\
		p1&0xFF, p2&0xFF, (p2>>8)|((p3&0xF)<<4),\
		(p3>>4)|((p0&0x100)>>1), p4&0xFF, (p4>>8)|((p5&0xF)<<4),\
		p5>>4, p6&0xFF, (p6>>8)|(p7<<7), p8&0xFF, p8>>8, p9&0xFF,\
		p10&0xFF, p11&0xFF, p12&0xFF}
#define	HITVDATA(p0,p1,p2,p3,p4,p5,p6,p7,p8)	{p0&0xFF, p1&0xFF, p2&0xFF,\
		(p2>>8)|((p3&0xF)<<4), (p3>>4)|((p0&0x100)>>1), p4&0xFF,\
		(p4>>8)|((p5&0xF)<<4), p5>>4, p6&0xFF, (p6>>8)|(p7<<7),\
		p8&0xFF, p8>>8}

#define	STMODE_SIZE	6
#define	EMODE_SIZE	10
#define	EMODE2_SIZE	5

typedef	struct	_STMode  {
	unsigned char	ModeID;
	unsigned short	ModeFlag;
	unsigned char	StModeTableIndex;
	unsigned char	CRTC2Index;
	unsigned char	ResInfo;
} STModeRec, *STModePtr;

typedef	struct _EMode  {
	unsigned char	ModeID;
	unsigned short	ModeFlag;
	unsigned char	ModeInfo;
	unsigned short	EMode2Offset;
	unsigned short	VESAModeID;
	unsigned char	MemSize;
	unsigned char	ResInfo;
} EModeRec, *EModePtr;

typedef	struct _EMode2  {
	unsigned short	InfoFlag;
	unsigned char	CRTC1Index;
	unsigned char	VCLKIndex;
	unsigned char	CRTC2Index;
} EMode2Rec, *EMode2Ptr;

typedef	struct	_CRTCInfoBlock  {
	unsigned short	HTotal;
	unsigned short	HSyncStart;
	unsigned short	HSyncEnd;
	unsigned short	VTotal;
	unsigned short	VSyncStart;
	unsigned short	VSyncEnd;
	unsigned char	Flag;
	unsigned long	PixelClock;
	unsigned short	RefreshRate;
} CRTCInfo, *CRTCInfoPtr;

Bool	SiSSetMode(ScrnInfoPtr pScrn, CARD16 ModeNo);
Bool	SearchModeID(CARD8 *ROMAddr, CARD16 ModeNo);
void	GetVBInfo(CARD16 BaseAddr, CARD8 *ROMAddr);
Bool	BridgeIsOn(CARD16 BaseAddr);
Bool	BridgeIsEnable(CARD16 BaseAddr);
Bool	BridgeInSlave(CARD16 BaseAddr);
void	GetLCDResInfo(CARD16 p3d4);
Bool	CheckVBInfo(CARD16 BaseAddr);
CARD16	GetRatePtr(ScrnInfoPtr pScrn, CARD16 ModeNo, CARD8 CRTx);
CARD16	AdjustCRT2Rate(CARD8 *ROMAddr);
void	SaveCRT2Info(CARD16 p3d4, CARD16 ModeNo);
void	SetCRT2Group(ScrnInfoPtr pScrn, CARD16 ModeNo);
void	LongWait(CARD16 p3da);
void	VBLongWait(CARD16 p3da);
Bool	WaitVBRetrace(CARD16 BaseAddr);
void	EnableBridge(CARD16 BaseAddr);
void	DisableBridge(CARD16 BaseAddr);
void	LockCRT2(CARD16 BaseAddr);
void	UnLockCRT2(CARD16 BaseAddr);
void	EnableCRT2(CARD16 BaseAddr);
void	SetCRT2ModeRegs(CARD16 BaseAddr, CARD16 ModeNo);
void	GetResInfo(CARD8 *ROMAddr, CARD16 ModeNo);
Bool	GetLockInfo(CARD16 p3d4, CARD8 info);
void	SetLockRegs(CARD16 BaseAddr, CARD8 info);
void	GetCRT1Ptr(CARD8 *ROMAddr);
void	GetCRT2Ptr(CARD8 *ROMAddr, CARD16 ModeNo);
void	GetRAMDAC2Data(CARD8 *ROMAddr, CARD16 ModeNo);
void	GetCRT2Data(CARD8 *ROMAddr, CARD16 ModeNo);
void	SetCRT2Offset(ScrnInfoPtr pScrn);
void	SetCRT2FIFO(ScrnInfoPtr pScrn);
void	SetCRT2Sync(CARD8 *ROMAddr, CARD16 part1_base);
void	SetGroup1(ScrnInfoPtr pScrn, CARD16 ModeNo);
void	SetGroup2(CARD16 BaseAddr, CARD8 *ROMAddr);
void	SetGroup3(CARD16 BaseAddr);
void	SetGroup4(CARD16 BaseAddr, CARD8 *ROMAddr, CARD16 ModeNo);
void	SetGroup5(CARD16 BaseAddr, CARD8 *ROMAddr);
void	SetBlock(CARD16 port, CARD8 from, CARD8 to, CARD8 *DataPtr);
CARD16	GetVGAHT2(void);
void	SetCRT2VCLK(CARD16 BaseAddr, CARD8 *ROMAddr, CARD16 ModeNo);
CARD8 * GetVCLK2Ptr(CARD16 BaseAddr, CARD8 *ROMAddr, CARD16 ModeNo);
